/*++

Module Name:

    bugcheck_panel.c

Abstract:

    PASSIVE_LEVEL preparation and crash-time rendering for the physical BGP
    diagnostic panel. The callback path uses only fixed nonpaged buffers and
    rectangles created before the bugcheck occurs.

--*/

#include "bugcheck_internal.h"
#include "bugcheck_bgp.h"
#include "bugcheck_layout.h"
#include "bugcheck_panel.h"
#include "../../platform/pool_compat.h"

#include "Generated/AsciiFont8x12.h"
#include "Generated/MainLogoBitmap.h"

#define KSWORD_ARK_PANEL_POOL_TAG 'lPgK'
#define KSWORD_ARK_PANEL_BACKGROUND_ARGB 0xFF050F21UL
#define KSWORD_ARK_PANEL_GLYPH_BORDER 1UL
#define KSWORD_ARK_PANEL_HERO_GLYPH_SCALE \
    KSWORD_ARK_BUGCHECK_LAYOUT_HERO_SCALE
#define KSWORD_ARK_PANEL_GLYPH_BITMAP_CAPACITY 4096UL
#define KSWORD_ARK_PANEL_COLOR_COUNT \
    ((ULONG)KswordArkBugcheckLayoutColorCount)
#define KSWORD_ARK_PANEL_BPP24_INDEX 0UL
#define KSWORD_ARK_PANEL_BPP32_INDEX 1UL
#define KSWORD_ARK_PANEL_BPP_VARIANT_COUNT 2UL

#pragma pack(push, 1)
typedef struct _KSWORD_ARK_PANEL_BITMAP_FILE_HEADER
{
    USHORT Type;
    ULONG Size;
    USHORT Reserved1;
    USHORT Reserved2;
    ULONG PixelOffset;
} KSWORD_ARK_PANEL_BITMAP_FILE_HEADER, *PKSWORD_ARK_PANEL_BITMAP_FILE_HEADER;

typedef struct _KSWORD_ARK_PANEL_BITMAP_INFO_HEADER
{
    ULONG Size;
    LONG Width;
    LONG Height;
    USHORT Planes;
    USHORT BitsPerPixel;
    ULONG Compression;
    ULONG ImageSize;
    LONG XPelsPerMeter;
    LONG YPelsPerMeter;
    ULONG ColorsUsed;
    ULONG ColorsImportant;
} KSWORD_ARK_PANEL_BITMAP_INFO_HEADER, *PKSWORD_ARK_PANEL_BITMAP_INFO_HEADER;
#pragma pack(pop)

typedef struct _KSWORD_ARK_PANEL_VARIANT
{
    ULONG BitsPerPixel;
    PVOID LogoRectangle;
    PVOID GlyphRectangles[KSWORD_ARK_PANEL_COLOR_COUNT][DRIVERGUI_FONT_COUNT];
    PVOID HeroGlyphRectangles[DRIVERGUI_FONT_COUNT];
    PVOID FrameHorizontalRectangles[KswordArkBugcheckLayoutFrameCount];
    PVOID FrameVerticalRectangles[KswordArkBugcheckLayoutFrameCount];
    // Keep the source BMPs resident for the lifetime of the parsed glyphs.
    // BgpGxParseBitmap is private and its ownership contract is not documented.
    // A persistent nonpaged backing buffer prevents a small-rectangle parser
    // from retaining a reused preparation stack buffer.
    PUCHAR GlyphBitmaps[KSWORD_ARK_PANEL_COLOR_COUNT][DRIVERGUI_FONT_COUNT];
    PUCHAR HeroGlyphBitmaps[DRIVERGUI_FONT_COUNT];
    PUCHAR FrameHorizontalBitmaps[KswordArkBugcheckLayoutFrameCount];
    PUCHAR FrameVerticalBitmaps[KswordArkBugcheckLayoutFrameCount];
} KSWORD_ARK_PANEL_VARIANT, *PKSWORD_ARK_PANEL_VARIANT;

typedef struct _KSWORD_ARK_PANEL_STATE
{
    volatile LONG Ready;
    volatile LONG Readers;
    volatile LONG Updating;
    volatile LONG ActiveVariant;
    KSWORD_ARK_PANEL_VARIANT Variants[KSWORD_ARK_PANEL_BPP_VARIANT_COUNT];
} KSWORD_ARK_PANEL_STATE, *PKSWORD_ARK_PANEL_STATE;

static KSWORD_ARK_PANEL_STATE g_KswordArkPanel;

static VOID
KswordARKBugcheckPanelReleaseResources(
    VOID
    );

static NTSTATUS
KswordARKBugcheckPanelInitializeBitmap(
    _Out_writes_bytes_(BitmapCapacity) UCHAR* Bitmap,
    _In_ ULONG BitmapCapacity,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_ ULONG BitsPerPixel,
    _Out_ PULONG BitmapLength,
    _Out_ PULONG BitmapStride,
    _Out_ PUCHAR* PixelBytes
    )
{
    PKSWORD_ARK_PANEL_BITMAP_FILE_HEADER fileHeader;
    PKSWORD_ARK_PANEL_BITMAP_INFO_HEADER infoHeader;
    ULONG64 stride;
    ULONG64 imageBytes;
    ULONG64 totalBytes;

    if (Bitmap == NULL ||
        BitmapLength == NULL ||
        BitmapStride == NULL ||
        PixelBytes == NULL ||
        Width == 0 ||
        Height == 0 ||
        (BitsPerPixel != 24UL && BitsPerPixel != 32UL)) {
        return STATUS_INVALID_PARAMETER;
    }

    stride = (((ULONG64)Width * BitsPerPixel + 31ULL) / 32ULL) * 4ULL;
    imageBytes = stride * Height;
    totalBytes =
        sizeof(KSWORD_ARK_PANEL_BITMAP_FILE_HEADER) +
        sizeof(KSWORD_ARK_PANEL_BITMAP_INFO_HEADER) +
        imageBytes;
    if (stride > MAXULONG ||
        imageBytes > MAXULONG ||
        totalBytes > BitmapCapacity ||
        totalBytes > MAXULONG) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(Bitmap, (SIZE_T)totalBytes);
    fileHeader = (PKSWORD_ARK_PANEL_BITMAP_FILE_HEADER)Bitmap;
    infoHeader = (PKSWORD_ARK_PANEL_BITMAP_INFO_HEADER)(Bitmap + sizeof(*fileHeader));
    fileHeader->Type = 0x4D42U;
    fileHeader->Size = (ULONG)totalBytes;
    fileHeader->PixelOffset = sizeof(*fileHeader) + sizeof(*infoHeader);
    infoHeader->Size = sizeof(*infoHeader);
    infoHeader->Width = (LONG)Width;
    infoHeader->Height = (LONG)Height;
    infoHeader->Planes = 1U;
    infoHeader->BitsPerPixel = (USHORT)BitsPerPixel;
    infoHeader->ImageSize = (ULONG)imageBytes;

    *BitmapLength = (ULONG)totalBytes;
    *BitmapStride = (ULONG)stride;
    *PixelBytes = Bitmap + fileHeader->PixelOffset;
    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareLogoRectangle(
    _In_ ULONG BitsPerPixel,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _Out_ PVOID* Rectangle
    )
{
    UCHAR* bitmap;
    PUCHAR pixels;
    ULONG bitmapLength;
    ULONG bitmapStride;
    ULONG bytesPerPixel;
    ULONG64 bitmapCapacity64;
    ULONG bitmapCapacity;
    ULONG destinationY;
    NTSTATUS status;

    if (Rectangle == NULL ||
        Width == 0 ||
        Height == 0 ||
        (BitsPerPixel != 24UL && BitsPerPixel != 32UL)) {
        return STATUS_INVALID_PARAMETER;
    }
    *Rectangle = NULL;

    bytesPerPixel = BitsPerPixel / 8UL;
    bitmapCapacity64 =
        sizeof(KSWORD_ARK_PANEL_BITMAP_FILE_HEADER) +
        sizeof(KSWORD_ARK_PANEL_BITMAP_INFO_HEADER) +
        ((((ULONG64)Width *
           BitsPerPixel + 31ULL) / 32ULL) * 4ULL) *
            Height;
    if (bitmapCapacity64 > MAXULONG) {
        return STATUS_INTEGER_OVERFLOW;
    }

    bitmapCapacity = (ULONG)bitmapCapacity64;
    bitmap = (UCHAR*)KswordARKAllocateNonPagedPool(
        bitmapCapacity,
        KSWORD_ARK_PANEL_POOL_TAG);
    if (bitmap == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pixels = NULL;
    status = KswordARKBugcheckPanelInitializeBitmap(
        bitmap,
        bitmapCapacity,
        Width,
        Height,
        BitsPerPixel,
        &bitmapLength,
        &bitmapStride,
        &pixels);
    if (NT_SUCCESS(status)) {
        for (destinationY = 0;
             destinationY < Height;
             ++destinationY) {
            ULONG destinationX;
            ULONG sourceY;
            PUCHAR destinationRow;
            const UCHAR* sourceRow;

            sourceY = (destinationY * DRIVERGUI_MAINLOGO_HEIGHT) / Height;
            destinationRow =
                pixels +
                ((SIZE_T)(Height - 1UL - destinationY) *
                 bitmapStride);
            sourceRow =
                g_DriverGuiMainLogoBgra +
                 ((SIZE_T)sourceY * DRIVERGUI_MAINLOGO_STRIDE);
            for (destinationX = 0;
                 destinationX < Width;
                 ++destinationX) {
                ULONG alpha;
                ULONG sourceBlue;
                ULONG sourceGreen;
                ULONG sourceRed;
                ULONG sourceX;
                PUCHAR destinationPixel;
                const UCHAR* sourcePixel;

                sourceX = (destinationX * DRIVERGUI_MAINLOGO_WIDTH) / Width;
                destinationPixel =
                    destinationRow + ((SIZE_T)destinationX * bytesPerPixel);
                sourcePixel = sourceRow + ((SIZE_T)sourceX * 4UL);
                sourceBlue = sourcePixel[0];
                sourceGreen = sourcePixel[1];
                sourceRed = sourcePixel[2];
                alpha = sourcePixel[3];

                // Keep the KSwordDEV artwork legible on the dark crash canvas.
                if (alpha != 0 && sourceRed < 72UL &&
                    sourceGreen < 72UL && sourceBlue < 72UL) {
                    sourceRed = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_RED;
                    sourceGreen = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_GREEN;
                    sourceBlue = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_BLUE;
                }
                destinationPixel[0] = (UCHAR)(
                    (sourceBlue * alpha +
                     KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_BLUE *
                         (255UL - alpha)) / 255UL);
                destinationPixel[1] = (UCHAR)(
                    (sourceGreen * alpha +
                     KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_GREEN *
                         (255UL - alpha)) / 255UL);
                destinationPixel[2] = (UCHAR)(
                    (sourceRed * alpha +
                     KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_RED *
                         (255UL - alpha)) / 255UL);
                if (bytesPerPixel == 4UL) {
                    destinationPixel[3] = 0xFFU;
                }
            }
        }

        status = KswordARKBugcheckBgpParseBitmap(
            bitmap,
            bitmapLength,
            Rectangle);
    }

    ExFreePoolWithTag(bitmap, KSWORD_ARK_PANEL_POOL_TAG);
    return status;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareLogos(
    _In_ ULONG VariantIndex,
    _In_ ULONG BitsPerPixel
    )
{
    NTSTATUS status;

    if (VariantIndex >= KSWORD_ARK_PANEL_BPP_VARIANT_COUNT) {
        return STATUS_INVALID_PARAMETER;
    }

    status = KswordARKBugcheckPanelPrepareLogoRectangle(
        BitsPerPixel,
        KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_WIDTH,
        KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_HEIGHT,
        &g_KswordArkPanel.Variants[VariantIndex].LogoRectangle);
    if (!NT_SUCCESS(status)) {
        KswordARKBugcheckBgpDestroyRectangle(
            g_KswordArkPanel.Variants[VariantIndex].LogoRectangle);
        g_KswordArkPanel.Variants[VariantIndex].LogoRectangle = NULL;
    }
    return status;
}

static VOID
KswordARKBugcheckPanelWriteGlyphPixel(
    _Out_writes_bytes_(BytesPerPixel) PUCHAR Pixel,
    _In_ ULONG BytesPerPixel,
    _In_ ULONG ColorIndex,
    _In_ UCHAR Coverage
    )
{
    ULONG foregroundBlue;
    ULONG foregroundGreen;
    ULONG foregroundRed;

    if (ColorIndex == KswordArkBugcheckLayoutColorAccent) {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_RED;
    } else if (ColorIndex == KswordArkBugcheckLayoutColorWarning) {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_RED;
    } else if (ColorIndex == KswordArkBugcheckLayoutColorCritical) {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_RED;
    } else if (ColorIndex == KswordArkBugcheckLayoutColorSuccess) {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_RED;
    } else if (ColorIndex == KswordArkBugcheckLayoutColorMuted) {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_RED;
    } else {
        foregroundBlue = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_BLUE;
        foregroundGreen = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_GREEN;
        foregroundRed = KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_RED;
    }

    // BGP rectangles are opaque. Bake each antialiased A8 coverage value
    // against the exact crash-canvas background during PASSIVE_LEVEL setup.
    Pixel[0] = (UCHAR)(
        (foregroundBlue * Coverage +
         KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_BLUE * (255UL - Coverage) +
         127UL) / 255UL);
    Pixel[1] = (UCHAR)(
        (foregroundGreen * Coverage +
         KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_GREEN * (255UL - Coverage) +
         127UL) / 255UL);
    Pixel[2] = (UCHAR)(
        (foregroundRed * Coverage +
         KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_RED * (255UL - Coverage) +
         127UL) / 255UL);
    if (BytesPerPixel == 4UL) {
        Pixel[3] = 0xFFU;
    }
}

static UCHAR
KswordARKBugcheckPanelGlyphCoverage(
    _In_opt_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* FontHeader,
    _In_opt_ const UCHAR* FontCoverage,
    _In_ ULONG GlyphIndex,
    _In_ ULONG Scale,
    _In_ ULONG X,
    _In_ ULONG Y
    )
{
    ULONG width;
    ULONG height;
    const UCHAR* atlas;

    if (FontHeader != NULL && FontCoverage != NULL) {
        if (Scale == 1UL) {
            width = FontHeader->bodyWidth;
            height = FontHeader->bodyHeight;
            atlas = FontCoverage;
        } else {
            width = FontHeader->heroWidth;
            height = FontHeader->heroHeight;
            atlas = FontCoverage + FontHeader->bodyDataLength;
        }
        if (X >= width || Y >= height) {
            return 0U;
        }
        return atlas[
            (SIZE_T)GlyphIndex * width * height +
            (SIZE_T)Y * width + X];
    }

    // The built-in bitmap is deliberately retained as a dependency-free
    // fallback. Center it inside the same fixed cells used by the R3 atlas so
    // all layout geometry remains identical before and after upload.
    if (Scale == 1UL) {
        const ULONG offsetX =
            (KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH - DRIVERGUI_FONT_WIDTH) / 2UL;
        const ULONG offsetY =
            (KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT - DRIVERGUI_FONT_HEIGHT) / 2UL;
        if (X < offsetX || Y < offsetY ||
            X >= offsetX + DRIVERGUI_FONT_WIDTH ||
            Y >= offsetY + DRIVERGUI_FONT_HEIGHT) {
            return 0U;
        }
        return (g_DriverGuiFont8x12[GlyphIndex][Y - offsetY] &
                (UCHAR)(1U << (7UL - (X - offsetX)))) != 0
            ? 255U
            : 0U;
    }

    {
        const ULONG scaledWidth = DRIVERGUI_FONT_WIDTH * 2UL;
        const ULONG scaledHeight = DRIVERGUI_FONT_HEIGHT * 2UL;
        const ULONG offsetX =
            (KSWORD_ARK_BUGCHECK_FONT_HERO_WIDTH - scaledWidth) / 2UL;
        const ULONG offsetY =
            (KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT - scaledHeight) / 2UL;
        ULONG sourceX;
        ULONG sourceY;

        if (X < offsetX || Y < offsetY ||
            X >= offsetX + scaledWidth ||
            Y >= offsetY + scaledHeight) {
            return 0U;
        }
        sourceX = (X - offsetX) / 2UL;
        sourceY = (Y - offsetY) / 2UL;
        return (g_DriverGuiFont8x12[GlyphIndex][sourceY] &
                (UCHAR)(1U << (7UL - sourceX))) != 0
            ? 255U
            : 0U;
    }
}

static NTSTATUS
KswordARKBugcheckPanelPrepareGlyph(
    _In_ ULONG VariantIndex,
    _In_ ULONG BitsPerPixel,
    _In_ ULONG ColorIndex,
    _In_ ULONG GlyphIndex,
    _In_ ULONG Scale,
    _In_opt_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* FontHeader,
    _In_opt_ const UCHAR* FontCoverage,
    _Out_ PVOID* Rectangle,
    _Out_ PUCHAR* BackingBitmap
    )
{
    PUCHAR bitmap;
    PUCHAR pixels;
    ULONG bitmapLength;
    ULONG bitmapStride;
    ULONG bytesPerPixel;
    ULONG bitmapRowIndex;
    ULONG bitmapColumnIndex;
    ULONG bitmapHeight;
    ULONG bitmapWidth;
    ULONG glyphHeight;
    ULONG glyphWidth;
    NTSTATUS status;

    if (VariantIndex >= KSWORD_ARK_PANEL_BPP_VARIANT_COUNT ||
        (BitsPerPixel != 24UL && BitsPerPixel != 32UL) ||
        ColorIndex >= KSWORD_ARK_PANEL_COLOR_COUNT ||
        GlyphIndex >= DRIVERGUI_FONT_COUNT ||
        (Scale != 1UL && Scale != KSWORD_ARK_PANEL_HERO_GLYPH_SCALE) ||
        Rectangle == NULL || BackingBitmap == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    *Rectangle = NULL;
    *BackingBitmap = NULL;
    glyphWidth = Scale == 1UL
        ? KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH
        : KSWORD_ARK_BUGCHECK_FONT_HERO_WIDTH;
    glyphHeight = Scale == 1UL
        ? KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT
        : KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT;
    bitmapWidth = glyphWidth +
        (KSWORD_ARK_PANEL_GLYPH_BORDER * 2UL);
    bitmapHeight = glyphHeight +
        (KSWORD_ARK_PANEL_GLYPH_BORDER * 2UL);

    bitmap = (PUCHAR)KswordARKAllocateNonPagedPool(
        KSWORD_ARK_PANEL_GLYPH_BITMAP_CAPACITY,
        KSWORD_ARK_PANEL_POOL_TAG);
    if (bitmap == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pixels = NULL;
    status = KswordARKBugcheckPanelInitializeBitmap(
        bitmap,
        KSWORD_ARK_PANEL_GLYPH_BITMAP_CAPACITY,
        bitmapWidth,
        bitmapHeight,
        BitsPerPixel,
        &bitmapLength,
        &bitmapStride,
        &pixels);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(bitmap, KSWORD_ARK_PANEL_POOL_TAG);
        return status;
    }

    bytesPerPixel = BitsPerPixel / 8UL;
    // Paint the complete padded cell before overlaying foreground bits.
    for (bitmapRowIndex = 0;
         bitmapRowIndex < bitmapHeight;
         ++bitmapRowIndex) {
        PUCHAR destinationRow;

        destinationRow =
            pixels +
            ((SIZE_T)(bitmapHeight -
                      1UL - bitmapRowIndex) * bitmapStride);
        for (bitmapColumnIndex = 0;
             bitmapColumnIndex < bitmapWidth;
             ++bitmapColumnIndex) {
            KswordARKBugcheckPanelWriteGlyphPixel(
                destinationRow +
                    ((SIZE_T)bitmapColumnIndex * bytesPerPixel),
                bytesPerPixel,
                ColorIndex,
                0U);
        }
    }
    for (bitmapRowIndex = 0;
         bitmapRowIndex < glyphHeight;
         ++bitmapRowIndex) {
        ULONG columnIndex;
        PUCHAR destinationRow;

        destinationRow = pixels +
            ((SIZE_T)(bitmapHeight - 1UL -
                      (KSWORD_ARK_PANEL_GLYPH_BORDER + bitmapRowIndex)) *
             bitmapStride);
        for (columnIndex = 0;
             columnIndex < glyphWidth;
             ++columnIndex) {
            UCHAR coverage;

            coverage = KswordARKBugcheckPanelGlyphCoverage(
                FontHeader,
                FontCoverage,
                GlyphIndex,
                Scale,
                columnIndex,
                bitmapRowIndex);
            KswordARKBugcheckPanelWriteGlyphPixel(
                destinationRow +
                    ((SIZE_T)(KSWORD_ARK_PANEL_GLYPH_BORDER + columnIndex) *
                     bytesPerPixel),
                bytesPerPixel,
                ColorIndex,
                coverage);
        }
    }

    status = KswordARKBugcheckBgpParseBitmap(
        bitmap,
        bitmapLength,
        Rectangle);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(bitmap, KSWORD_ARK_PANEL_POOL_TAG);
        return status;
    }

    *BackingBitmap = bitmap;
    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareGlyphs(
    _In_ ULONG VariantIndex,
    _In_ ULONG BitsPerPixel,
    _In_opt_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* FontHeader,
    _In_opt_ const UCHAR* FontCoverage
    )
{
    ULONG colorIndex;

    for (colorIndex = 0;
         colorIndex < KSWORD_ARK_PANEL_COLOR_COUNT;
         ++colorIndex) {
        ULONG glyphIndex;

        for (glyphIndex = 0;
             glyphIndex < DRIVERGUI_FONT_COUNT;
             ++glyphIndex) {
            NTSTATUS status;

            status = KswordARKBugcheckPanelPrepareGlyph(
                VariantIndex,
                BitsPerPixel,
                colorIndex,
                glyphIndex,
                1UL,
                FontHeader,
                FontCoverage,
                &g_KswordArkPanel.Variants[VariantIndex]
                    .GlyphRectangles[colorIndex][glyphIndex],
                &g_KswordArkPanel.Variants[VariantIndex]
                    .GlyphBitmaps[colorIndex][glyphIndex]);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareHeroGlyphs(
    _In_ ULONG VariantIndex,
    _In_ ULONG BitsPerPixel,
    _In_opt_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* FontHeader,
    _In_opt_ const UCHAR* FontCoverage
    )
{
    ULONG glyphIndex;

    for (glyphIndex = 0;
         glyphIndex < DRIVERGUI_FONT_COUNT;
         ++glyphIndex) {
        NTSTATUS status;

        status = KswordARKBugcheckPanelPrepareGlyph(
            VariantIndex,
            BitsPerPixel,
            KswordArkBugcheckLayoutColorCritical,
            glyphIndex,
            KSWORD_ARK_PANEL_HERO_GLYPH_SCALE,
            FontHeader,
            FontCoverage,
            &g_KswordArkPanel.Variants[VariantIndex]
                .HeroGlyphRectangles[glyphIndex],
            &g_KswordArkPanel.Variants[VariantIndex]
                .HeroGlyphBitmaps[glyphIndex]);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareSolidRectangle(
    _In_ ULONG BitsPerPixel,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _Out_ PVOID* Rectangle,
    _Out_ PUCHAR* BackingBitmap
    )
{
    PUCHAR bitmap;
    PUCHAR pixels;
    ULONG64 bitmapCapacity64;
    ULONG bitmapCapacity;
    ULONG bitmapLength;
    ULONG bitmapStride;
    ULONG bytesPerPixel;
    ULONG x;
    ULONG y;
    NTSTATUS status;

    if (Rectangle == NULL || BackingBitmap == NULL ||
        Width == 0 || Height == 0 ||
        (BitsPerPixel != 24UL && BitsPerPixel != 32UL)) {
        return STATUS_INVALID_PARAMETER;
    }
    *Rectangle = NULL;
    *BackingBitmap = NULL;

    bitmapCapacity64 =
        sizeof(KSWORD_ARK_PANEL_BITMAP_FILE_HEADER) +
        sizeof(KSWORD_ARK_PANEL_BITMAP_INFO_HEADER) +
        ((((ULONG64)Width * BitsPerPixel + 31ULL) / 32ULL) *
         4ULL * Height);
    if (bitmapCapacity64 > MAXULONG) {
        return STATUS_INTEGER_OVERFLOW;
    }
    bitmapCapacity = (ULONG)bitmapCapacity64;
    bitmap = (PUCHAR)KswordARKAllocateNonPagedPool(
        bitmapCapacity,
        KSWORD_ARK_PANEL_POOL_TAG);
    if (bitmap == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pixels = NULL;
    status = KswordARKBugcheckPanelInitializeBitmap(
        bitmap,
        bitmapCapacity,
        Width,
        Height,
        BitsPerPixel,
        &bitmapLength,
        &bitmapStride,
        &pixels);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(bitmap, KSWORD_ARK_PANEL_POOL_TAG);
        return status;
    }

    bytesPerPixel = BitsPerPixel / 8UL;
    for (y = 0; y < Height; ++y) {
        PUCHAR row;

        row = pixels + ((SIZE_T)y * bitmapStride);
        for (x = 0; x < Width; ++x) {
            PUCHAR pixel;

            pixel = row + ((SIZE_T)x * bytesPerPixel);
            pixel[0] = KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_BLUE;
            pixel[1] = KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_GREEN;
            pixel[2] = KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_RED;
            if (bytesPerPixel == 4UL) {
                pixel[3] = 0xFFU;
            }
        }
    }

    status = KswordARKBugcheckBgpParseBitmap(
        bitmap,
        bitmapLength,
        Rectangle);
    if (!NT_SUCCESS(status)) {
        ExFreePoolWithTag(bitmap, KSWORD_ARK_PANEL_POOL_TAG);
        return status;
    }

    // Retain the source because the private parser's ownership is undocumented.
    *BackingBitmap = bitmap;
    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareFrames(
    _In_ ULONG VariantIndex,
    _In_ ULONG BitsPerPixel
    )
{
    KSWORD_ARK_BUGCHECK_LAYOUT_FRAME frame;

    if (VariantIndex >= KSWORD_ARK_PANEL_BPP_VARIANT_COUNT) {
        return STATUS_INVALID_PARAMETER;
    }

    for (frame = KswordArkBugcheckLayoutFrameCompactUpper;
         frame < KswordArkBugcheckLayoutFrameCount;
         frame = (KSWORD_ARK_BUGCHECK_LAYOUT_FRAME)(frame + 1)) {
        ULONG width;
        ULONG height;
        NTSTATUS status;

        if (!KswordARKBugcheckLayoutGetFrameMetrics(
                frame,
                &width,
                &height)) {
            return STATUS_INVALID_PARAMETER;
        }

        status = KswordARKBugcheckPanelPrepareSolidRectangle(
            BitsPerPixel,
            width,
            1UL,
            &g_KswordArkPanel.Variants[VariantIndex]
                .FrameHorizontalRectangles[frame],
            &g_KswordArkPanel.Variants[VariantIndex]
                .FrameHorizontalBitmaps[frame]);
        if (NT_SUCCESS(status)) {
            status = KswordARKBugcheckPanelPrepareSolidRectangle(
                BitsPerPixel,
                1UL,
                height,
                &g_KswordArkPanel.Variants[VariantIndex]
                    .FrameVerticalRectangles[frame],
                &g_KswordArkPanel.Variants[VariantIndex]
                    .FrameVerticalBitmaps[frame]);
        }
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelPrepareResources(
    _In_opt_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* FontHeader,
    _In_opt_ const UCHAR* FontCoverage
    )
{
    KSWORD_ARK_BGP_SCREEN_INFO screen;
    ULONG variantIndex;
    NTSTATUS status;

    KswordARKBugcheckBgpRecordPreparation(
        KswordArkBgpPreparationValidatePanelScreen,
        STATUS_PENDING);
    status = KswordARKBugcheckBgpGetScreenInfo(&screen);
    if (!NT_SUCCESS(status)) {
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationValidatePanelScreen,
            status);
        return status;
    }
    // Accept the fully hidden pre-ownership mode so both supported rectangle
    // variants can still be prepared at PASSIVE_LEVEL.
    if (screen.BitsPerPixel != KSWORD_ARK_BGP_UNOWNED_BPP &&
        (screen.Width < KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH ||
         screen.Height < KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT ||
         (screen.BitsPerPixel != 24UL &&
          screen.BitsPerPixel != 32UL))) {
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationValidatePanelScreen,
            STATUS_NOT_SUPPORTED);
        return STATUS_NOT_SUPPORTED;
    }

    KswordARKBugcheckBgpRecordPreparation(
        KswordArkBgpPreparationValidatePanelScreen,
        STATUS_SUCCESS);
    status = STATUS_SUCCESS;
    for (variantIndex = 0;
         variantIndex < KSWORD_ARK_PANEL_BPP_VARIANT_COUNT &&
             NT_SUCCESS(status);
         ++variantIndex) {
        ULONG bitsPerPixel;

        bitsPerPixel = variantIndex == KSWORD_ARK_PANEL_BPP24_INDEX
            ? 24UL
            : 32UL;
        g_KswordArkPanel.Variants[variantIndex].BitsPerPixel = bitsPerPixel;
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationPrepareLogo,
            STATUS_PENDING);
        status = KswordARKBugcheckPanelPrepareLogos(
            variantIndex,
            bitsPerPixel);
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationPrepareLogo,
            status);
        if (NT_SUCCESS(status)) {
            KswordARKBugcheckBgpRecordPreparation(
                KswordArkBgpPreparationPrepareGlyphs,
                STATUS_PENDING);
            status = KswordARKBugcheckPanelPrepareGlyphs(
                variantIndex,
                bitsPerPixel,
                FontHeader,
                FontCoverage);
            if (NT_SUCCESS(status)) {
                status = KswordARKBugcheckPanelPrepareHeroGlyphs(
                    variantIndex,
                    bitsPerPixel,
                    FontHeader,
                    FontCoverage);
            }
            if (NT_SUCCESS(status)) {
                // Frames are parsed with glyph resources before the crash.
                status = KswordARKBugcheckPanelPrepareFrames(
                    variantIndex,
                    bitsPerPixel);
            }
            KswordARKBugcheckBgpRecordPreparation(
                KswordArkBgpPreparationPrepareGlyphs,
                status);
        }
    }
    if (!NT_SUCCESS(status)) {
        KswordARKBugcheckPanelReleaseResources();
        return status;
    }
    return STATUS_SUCCESS;
}

static VOID
KswordARKBugcheckPanelReleaseResources(
    VOID
    )
{
    ULONG variantIndex;
    ULONG colorIndex;

    for (variantIndex = 0;
         variantIndex < KSWORD_ARK_PANEL_BPP_VARIANT_COUNT;
         ++variantIndex) {
        KswordARKBugcheckBgpDestroyRectangle(
            g_KswordArkPanel.Variants[variantIndex].LogoRectangle);
        g_KswordArkPanel.Variants[variantIndex].LogoRectangle = NULL;
        for (colorIndex = 0;
             colorIndex < KSWORD_ARK_PANEL_COLOR_COUNT;
             ++colorIndex) {
            ULONG glyphIndex;

            for (glyphIndex = 0;
                 glyphIndex < DRIVERGUI_FONT_COUNT;
                 ++glyphIndex) {
                KswordARKBugcheckBgpDestroyRectangle(
                    g_KswordArkPanel.Variants[variantIndex]
                        .GlyphRectangles[colorIndex][glyphIndex]);
                g_KswordArkPanel.Variants[variantIndex]
                    .GlyphRectangles[colorIndex][glyphIndex] = NULL;
                if (g_KswordArkPanel.Variants[variantIndex]
                        .GlyphBitmaps[colorIndex][glyphIndex] != NULL) {
                    ExFreePoolWithTag(
                        g_KswordArkPanel.Variants[variantIndex]
                            .GlyphBitmaps[colorIndex][glyphIndex],
                        KSWORD_ARK_PANEL_POOL_TAG);
                    g_KswordArkPanel.Variants[variantIndex]
                        .GlyphBitmaps[colorIndex][glyphIndex] = NULL;
                }
            }
        }
        {
            ULONG glyphIndex;

            for (glyphIndex = 0;
                 glyphIndex < DRIVERGUI_FONT_COUNT;
                 ++glyphIndex) {
                KswordARKBugcheckBgpDestroyRectangle(
                    g_KswordArkPanel.Variants[variantIndex]
                        .HeroGlyphRectangles[glyphIndex]);
                g_KswordArkPanel.Variants[variantIndex]
                    .HeroGlyphRectangles[glyphIndex] = NULL;
                if (g_KswordArkPanel.Variants[variantIndex]
                        .HeroGlyphBitmaps[glyphIndex] != NULL) {
                    ExFreePoolWithTag(
                        g_KswordArkPanel.Variants[variantIndex]
                            .HeroGlyphBitmaps[glyphIndex],
                        KSWORD_ARK_PANEL_POOL_TAG);
                    g_KswordArkPanel.Variants[variantIndex]
                        .HeroGlyphBitmaps[glyphIndex] = NULL;
                }
            }
        }
        {
            KSWORD_ARK_BUGCHECK_LAYOUT_FRAME frame;

            for (frame = KswordArkBugcheckLayoutFrameCompactUpper;
                 frame < KswordArkBugcheckLayoutFrameCount;
                 frame = (KSWORD_ARK_BUGCHECK_LAYOUT_FRAME)(frame + 1)) {
                KswordARKBugcheckBgpDestroyRectangle(
                    g_KswordArkPanel.Variants[variantIndex]
                        .FrameHorizontalRectangles[frame]);
                g_KswordArkPanel.Variants[variantIndex]
                    .FrameHorizontalRectangles[frame] = NULL;
                KswordARKBugcheckBgpDestroyRectangle(
                    g_KswordArkPanel.Variants[variantIndex]
                        .FrameVerticalRectangles[frame]);
                g_KswordArkPanel.Variants[variantIndex]
                    .FrameVerticalRectangles[frame] = NULL;
                if (g_KswordArkPanel.Variants[variantIndex]
                        .FrameHorizontalBitmaps[frame] != NULL) {
                    ExFreePoolWithTag(
                        g_KswordArkPanel.Variants[variantIndex]
                            .FrameHorizontalBitmaps[frame],
                        KSWORD_ARK_PANEL_POOL_TAG);
                    g_KswordArkPanel.Variants[variantIndex]
                        .FrameHorizontalBitmaps[frame] = NULL;
                }
                if (g_KswordArkPanel.Variants[variantIndex]
                        .FrameVerticalBitmaps[frame] != NULL) {
                    ExFreePoolWithTag(
                        g_KswordArkPanel.Variants[variantIndex]
                            .FrameVerticalBitmaps[frame],
                        KSWORD_ARK_PANEL_POOL_TAG);
                    g_KswordArkPanel.Variants[variantIndex]
                        .FrameVerticalBitmaps[frame] = NULL;
                }
            }
        }
    }
    RtlZeroMemory(
        g_KswordArkPanel.Variants,
        sizeof(g_KswordArkPanel.Variants));
}

NTSTATUS
KswordARKBugcheckPanelInitialize(
    VOID
    )
{
    NTSTATUS status;

    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return STATUS_INVALID_DEVICE_STATE;
    }

    RtlZeroMemory(&g_KswordArkPanel, sizeof(g_KswordArkPanel));
    status = KswordARKBugcheckPanelPrepareResources(NULL, NULL);
    if (NT_SUCCESS(status)) {
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationArm,
            STATUS_PENDING);
        status = KswordARKBugcheckBgpArm(
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH,
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT);
        KswordARKBugcheckBgpRecordPreparation(
            KswordArkBgpPreparationArm,
            status);
    }
    if (!NT_SUCCESS(status)) {
        KswordARKBugcheckPanelReleaseResources();
        KswordARKBugcheckBgpRejectPreparation(status);
        return status;
    }

    InterlockedExchange(&g_KswordArkPanel.Ready, 1);
    KswordARKBugcheckBgpRecordPreparation(
        KswordArkBgpPreparationComplete,
        STATUS_SUCCESS);
    return STATUS_SUCCESS;
}

VOID
KswordARKBugcheckPanelShutdown(
    VOID
    )
{
    InterlockedExchange(&g_KswordArkPanel.Ready, 0);
    KeMemoryBarrier();
    KswordARKBugcheckPanelReleaseResources();
    InterlockedExchange(&g_KswordArkPanel.ActiveVariant, 0);
}

NTSTATUS
KswordARKBugcheckPanelInstallFont(
    _In_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* Header,
    _In_reads_bytes_(KSWORD_ARK_BUGCHECK_FONT_MAX_BYTES) const UCHAR* Coverage
    )
{
    NTSTATUS fallbackStatus;
    NTSTATUS status;

    if (Header == NULL || Coverage == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        return STATUS_INVALID_DEVICE_STATE;
    }
    if (InterlockedCompareExchange(
            &g_KswordArkPanel.Updating,
            1,
            0) != 0) {
        return STATUS_DEVICE_BUSY;
    }

    // A machine without an armed BGP panel can still consume the same atlas
    // through the direct SVGA renderer, so lack of BGP is not an upload error.
    if (InterlockedCompareExchange(
            &g_KswordArkPanel.Ready,
            0,
            0) == 0) {
        InterlockedExchange(&g_KswordArkPanel.Updating, 0);
        return STATUS_SUCCESS;
    }

    InterlockedExchange(&g_KswordArkPanel.Ready, 0);
    KeMemoryBarrier();
    if (InterlockedCompareExchange(
            &g_KswordArkPanel.Readers,
            0,
            0) != 0) {
        InterlockedExchange(&g_KswordArkPanel.Ready, 1);
        InterlockedExchange(&g_KswordArkPanel.Updating, 0);
        return STATUS_DEVICE_BUSY;
    }

    status = KswordARKBugcheckBgpBeginPanelUpdate();
    if (!NT_SUCCESS(status)) {
        InterlockedExchange(&g_KswordArkPanel.Ready, 1);
        InterlockedExchange(&g_KswordArkPanel.Updating, 0);
        return status;
    }

    KswordARKBugcheckPanelReleaseResources();
    status = KswordARKBugcheckPanelPrepareResources(Header, Coverage);
    if (NT_SUCCESS(status)) {
        status = KswordARKBugcheckBgpArm(
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH,
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT);
    }
    if (NT_SUCCESS(status)) {
        KeMemoryBarrier();
        InterlockedExchange(&g_KswordArkPanel.Ready, 1);
        InterlockedExchange(&g_KswordArkPanel.Updating, 0);
        return STATUS_SUCCESS;
    }

    // A malformed or too-expensive custom atlas must never remove the
    // dependency-free crash panel. Rebuild the original bitmap font before
    // reporting the custom-font failure to R3.
    KswordARKBugcheckPanelReleaseResources();
    fallbackStatus = KswordARKBugcheckPanelPrepareResources(NULL, NULL);
    if (NT_SUCCESS(fallbackStatus)) {
        fallbackStatus = KswordARKBugcheckBgpArm(
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH,
            KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT);
    }
    if (NT_SUCCESS(fallbackStatus)) {
        KeMemoryBarrier();
        InterlockedExchange(&g_KswordArkPanel.Ready, 1);
    } else {
        KswordARKBugcheckPanelReleaseResources();
        KswordARKBugcheckBgpRejectPreparation(fallbackStatus);
    }
    InterlockedExchange(&g_KswordArkPanel.Updating, 0);
    return status;
}

static NTSTATUS
KswordARKBugcheckPanelDrawText(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_z_ PCSTR Text,
    _In_ ULONG ColorIndex,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_STYLE TextStyle
    )
{
    LONG activeVariant;
    LONG cursorX;

    UNREFERENCED_PARAMETER(Context);
    if (Text == NULL || ColorIndex >= KSWORD_ARK_PANEL_COLOR_COUNT ||
        TextStyle >= KswordArkBugcheckLayoutTextStyleCount ||
        (TextStyle == KswordArkBugcheckLayoutTextHero &&
         ColorIndex != KswordArkBugcheckLayoutColorCritical)) {
        return STATUS_INVALID_PARAMETER;
    }

    activeVariant = InterlockedCompareExchange(
        &g_KswordArkPanel.ActiveVariant,
        0,
        0);
    if (activeVariant < 0 ||
        activeVariant >= (LONG)KSWORD_ARK_PANEL_BPP_VARIANT_COUNT) {
        return STATUS_DEVICE_NOT_READY;
    }

    cursorX = X;
    while (*Text != '\0') {
        UCHAR character;
        ULONG glyphIndex;
        NTSTATUS status;

        character = (UCHAR)*Text;
        if (character < DRIVERGUI_FONT_FIRST ||
            character > DRIVERGUI_FONT_LAST) {
            character = (UCHAR)'?';
        }
        glyphIndex = character - DRIVERGUI_FONT_FIRST;
        if (character != (UCHAR)' ') {
            if (TextStyle == KswordArkBugcheckLayoutTextHero) {
                status = KswordARKBugcheckBgpDrawRectangle(
                    g_KswordArkPanel.Variants[activeVariant]
                        .HeroGlyphRectangles[glyphIndex],
                    cursorX - (LONG)KSWORD_ARK_PANEL_GLYPH_BORDER,
                    Y - (LONG)KSWORD_ARK_PANEL_GLYPH_BORDER);
            } else {
                status = KswordARKBugcheckBgpDrawRectangle(
                    g_KswordArkPanel.Variants[activeVariant]
                        .GlyphRectangles[ColorIndex][glyphIndex],
                    cursorX - (LONG)KSWORD_ARK_PANEL_GLYPH_BORDER,
                    Y - (LONG)KSWORD_ARK_PANEL_GLYPH_BORDER);
            }
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }
        cursorX += TextStyle == KswordArkBugcheckLayoutTextHero
            ? (LONG)KSWORD_ARK_BUGCHECK_FONT_HERO_ADVANCE
            : (LONG)KSWORD_ARK_BUGCHECK_FONT_BODY_ADVANCE;
        ++Text;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
KswordARKBugcheckPanelDrawFrame(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame
    )
{
    LONG activeVariant;
    ULONG width;
    ULONG height;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Context);
    if (!KswordARKBugcheckLayoutGetFrameMetrics(
            Frame,
            &width,
            &height)) {
        return STATUS_INVALID_PARAMETER;
    }

    activeVariant = InterlockedCompareExchange(
        &g_KswordArkPanel.ActiveVariant,
        0,
        0);
    if (activeVariant < 0 ||
        activeVariant >= (LONG)KSWORD_ARK_PANEL_BPP_VARIANT_COUNT) {
        return STATUS_DEVICE_NOT_READY;
    }

    // Each frame uses four rectangles parsed before the bugcheck occurs.
    status = KswordARKBugcheckBgpDrawRectangle(
        g_KswordArkPanel.Variants[activeVariant]
            .FrameHorizontalRectangles[Frame],
        X,
        Y);
    if (NT_SUCCESS(status)) {
        status = KswordARKBugcheckBgpDrawRectangle(
            g_KswordArkPanel.Variants[activeVariant]
                .FrameHorizontalRectangles[Frame],
            X,
            Y + (LONG)height - 1L);
    }
    if (NT_SUCCESS(status)) {
        status = KswordARKBugcheckBgpDrawRectangle(
            g_KswordArkPanel.Variants[activeVariant]
                .FrameVerticalRectangles[Frame],
            X,
            Y);
    }
    if (NT_SUCCESS(status)) {
        status = KswordARKBugcheckBgpDrawRectangle(
            g_KswordArkPanel.Variants[activeVariant]
                .FrameVerticalRectangles[Frame],
            X + (LONG)width - 1L,
            Y);
    }
    return status;
}

NTSTATUS
KswordARKBugcheckPanelDraw(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    KSWORD_ARK_BGP_DUMP_STATE bgpState;
    KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS canvas;
    LONG activeVariant;
    LONG originX;
    NTSTATUS status;

    if (Diagnostics == NULL ||
        InterlockedCompareExchange(&g_KswordArkPanel.Ready, 0, 0) == 0) {
        return STATUS_DEVICE_NOT_READY;
    }
    InterlockedIncrement(&g_KswordArkPanel.Readers);
    KeMemoryBarrier();
    if (InterlockedCompareExchange(
            &g_KswordArkPanel.Ready,
            0,
            0) == 0) {
        InterlockedDecrement(&g_KswordArkPanel.Readers);
        return STATUS_DEVICE_NOT_READY;
    }

    status = KswordARKBugcheckBgpBeginDraw();
    if (!NT_SUCCESS(status)) {
        InterlockedDecrement(&g_KswordArkPanel.Readers);
        return status;
    }

    activeVariant = KswordARKBugcheckBgpGetCurrentBpp() == 24UL
        ? (LONG)KSWORD_ARK_PANEL_BPP24_INDEX
        : (LONG)KSWORD_ARK_PANEL_BPP32_INDEX;
    InterlockedExchange(&g_KswordArkPanel.ActiveVariant, activeVariant);
    KswordARKBugcheckBgpSnapshot(&bgpState);
    originX = KswordARKBugcheckLayoutOriginX(
        bgpState.ScreenWidth,
        bgpState.ScreenHeight);

    // Clear only after BeginDraw has acquired ownership and validated geometry.
    status = KswordARKBugcheckBgpClearScreen(
        KSWORD_ARK_PANEL_BACKGROUND_ARGB);
    if (NT_SUCCESS(status)) {
        status = KswordARKBugcheckBgpDrawRectangle(
            g_KswordArkPanel.Variants[activeVariant].LogoRectangle,
            originX + KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_X,
            KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_Y);
    }
    if (NT_SUCCESS(status)) {
        RtlZeroMemory(&canvas, sizeof(canvas));
        canvas.Width = bgpState.ScreenWidth;
        canvas.Height = bgpState.ScreenHeight;
        canvas.BitsPerPixel = bgpState.ScreenBpp;
        canvas.RendererName = "BGP";
        canvas.DrawText = KswordARKBugcheckPanelDrawText;
        canvas.DrawFrame = KswordARKBugcheckPanelDrawFrame;
        status = KswordARKBugcheckLayoutDraw(
            &canvas,
            Diagnostics,
            CallbackMask,
            ModuleCount);
    }

    KswordARKBugcheckBgpFinishDraw(status);
    InterlockedDecrement(&g_KswordArkPanel.Readers);
    return status;
}
