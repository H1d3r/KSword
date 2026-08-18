/*++

Module Name:

    bugcheck_layout.c

Abstract:

    Shared crash-safe information layout for the BGP and VMware framebuffer
    bugcheck renderers. The implementation formats only captured data and uses
    caller-supplied drawing callbacks without allocation or pageable services.

--*/

#include "bugcheck_layout.h"
#include "bugcheck_layout_detailed.h"

#include <ntstrsafe.h>
#include <stdarg.h>

typedef struct _KSWORD_ARK_BUGCHECK_LAYOUT_FRAME_METRICS
{
    ULONG Width;
    ULONG Height;
} KSWORD_ARK_BUGCHECK_LAYOUT_FRAME_METRICS;

typedef struct _KSWORD_ARK_BUGCHECK_LAYOUT_WRITER
{
    const KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS* Canvas;
    LONG OriginX;
    NTSTATUS Status;
    CHAR Line[KSWORD_ARK_BUGCHECK_PANEL_LINE_CHARS];
} KSWORD_ARK_BUGCHECK_LAYOUT_WRITER;

#define KSWORD_ARK_BUGCHECK_COMPACT_LEFT_X 16L
#define KSWORD_ARK_BUGCHECK_COMPACT_RIGHT_X 328L
#define KSWORD_ARK_BUGCHECK_COMPACT_UPPER_Y 104L
#define KSWORD_ARK_BUGCHECK_COMPACT_LOWER_Y 290L
#define KSWORD_ARK_BUGCHECK_COMPACT_PANEL_WIDTH 296UL
#define KSWORD_ARK_BUGCHECK_COMPACT_UPPER_HEIGHT 120UL
// The extra two pixels keep the final 8x12 glyph plus its one-pixel BGP
// backing border clear of the frame's bottom edge.
#define KSWORD_ARK_BUGCHECK_COMPACT_LOWER_HEIGHT 154UL
#define KSWORD_ARK_BUGCHECK_COMPACT_DUMP_BAND_TOP 228L
#define KSWORD_ARK_BUGCHECK_COMPACT_DUMP_BAND_BOTTOM 284L
#define KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y 460L

static const KSWORD_ARK_BUGCHECK_LAYOUT_FRAME_METRICS
    g_KswordArkBugcheckLayoutFrames[KswordArkBugcheckLayoutFrameCount] = {
        { KSWORD_ARK_BUGCHECK_COMPACT_PANEL_WIDTH,
          KSWORD_ARK_BUGCHECK_COMPACT_UPPER_HEIGHT },
        { KSWORD_ARK_BUGCHECK_COMPACT_PANEL_WIDTH,
          KSWORD_ARK_BUGCHECK_COMPACT_LOWER_HEIGHT },
        { 344UL, 184UL },
        { 312UL, 184UL },
        { 328UL, 184UL },
        { 344UL, 174UL },
        { 312UL, 174UL },
        { 328UL, 174UL },
        { 484UL, 176UL },
        { 500UL, 176UL },
        { 470UL, 207UL },
        { 350UL, 207UL },
        { 416UL, 207UL },
        { 314UL, 162UL },
        { 328UL, 162UL },
        { 278UL, 162UL },
        { 316UL, 162UL },
        { 480UL, 205UL },
        { 348UL, 205UL },
        { 412UL, 98UL },
        { 412UL, 103UL }
    };

C_ASSERT(
    RTL_NUMBER_OF(g_KswordArkBugcheckLayoutFrames) ==
    KswordArkBugcheckLayoutFrameCount);
C_ASSERT(
    KSWORD_ARK_BUGCHECK_COMPACT_RIGHT_X +
        KSWORD_ARK_BUGCHECK_COMPACT_PANEL_WIDTH <=
    KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH);
C_ASSERT(
    KSWORD_ARK_BUGCHECK_COMPACT_UPPER_Y +
        KSWORD_ARK_BUGCHECK_COMPACT_UPPER_HEIGHT <=
    KSWORD_ARK_BUGCHECK_COMPACT_DUMP_BAND_TOP);
C_ASSERT(
    KSWORD_ARK_BUGCHECK_COMPACT_LOWER_Y >
    KSWORD_ARK_BUGCHECK_COMPACT_DUMP_BAND_BOTTOM);
C_ASSERT(
    KSWORD_ARK_BUGCHECK_COMPACT_LOWER_Y +
        KSWORD_ARK_BUGCHECK_COMPACT_LOWER_HEIGHT <
    KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y);
C_ASSERT(
    KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y + 12L <=
    KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT);

BOOLEAN
KswordARKBugcheckLayoutIsDetailed(
    _In_ ULONG Width,
    _In_ ULONG Height
    )
{
    // The wide 720-line mode matches the information-dense design safely.
    return Width >= KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_WIDTH &&
        Height >= KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_HEIGHT;
}

BOOLEAN
KswordARKBugcheckLayoutIsCompact(
    _In_ ULONG Width,
    _In_ ULONG Height
    )
{
    // The 640x480 crash fallback needs its dedicated two-column layout.
    return !KswordARKBugcheckLayoutIsDetailed(Width, Height) &&
        (Width < KSWORD_ARK_BUGCHECK_LAYOUT_FULL_WIDTH ||
         Height < KSWORD_ARK_BUGCHECK_LAYOUT_FULL_HEIGHT);
}

LONG
KswordARKBugcheckLayoutOriginX(
    _In_ ULONG Width,
    _In_ ULONG Height
    )
{
    ULONG canvasWidth;

    // Center whichever fixed canvas the current crash mode can safely fit.
    if (KswordARKBugcheckLayoutIsDetailed(Width, Height)) {
        canvasWidth = KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_WIDTH;
    } else if (KswordARKBugcheckLayoutIsCompact(Width, Height)) {
        canvasWidth = KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH;
    } else {
        canvasWidth = KSWORD_ARK_BUGCHECK_LAYOUT_FULL_WIDTH;
    }
    if (Width > canvasWidth) {
        return (LONG)((Width - canvasWidth) / 2UL);
    }
    return 0L;
}

BOOLEAN
KswordARKBugcheckLayoutGetFrameMetrics(
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame,
    _Out_ PULONG Width,
    _Out_ PULONG Height
    )
{
    if (Width == NULL || Height == NULL ||
        Frame < KswordArkBugcheckLayoutFrameCompactUpper ||
        Frame >= KswordArkBugcheckLayoutFrameCount) {
        return FALSE;
    }

    *Width = g_KswordArkBugcheckLayoutFrames[Frame].Width;
    *Height = g_KswordArkBugcheckLayoutFrames[Frame].Height;
    return TRUE;
}

static VOID
KswordARKBugcheckLayoutClipLine(
    _Inout_updates_z_(Capacity) PCHAR Text,
    _In_ ULONG Capacity,
    _In_ ULONG MaximumCharacters
    )
{
    SIZE_T length;

    if (Text == NULL || Capacity == 0 || MaximumCharacters == 0) {
        return;
    }
    if (MaximumCharacters >= Capacity) {
        MaximumCharacters = Capacity - 1UL;
    }

    length = 0;
    while (length + 1UL < Capacity && Text[length] != '\0') {
        ++length;
    }
    if (length <= MaximumCharacters) {
        return;
    }

    if (MaximumCharacters > 3UL) {
        Text[MaximumCharacters - 3UL] = '.';
        Text[MaximumCharacters - 2UL] = '.';
        Text[MaximumCharacters - 1UL] = '.';
    }
    Text[MaximumCharacters] = '\0';
}

static VOID
KswordARKBugcheckLayoutWriteFormatted(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG ColorIndex,
    _In_ ULONG MaximumCharacters,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    va_list arguments;

    if (Writer == NULL || !NT_SUCCESS(Writer->Status)) {
        return;
    }

    va_start(arguments, Format);
    Writer->Status = RtlStringCbVPrintfA(
        Writer->Line,
        sizeof(Writer->Line),
        Format,
        arguments);
    va_end(arguments);
    if (!NT_SUCCESS(Writer->Status)) {
        return;
    }

    KswordARKBugcheckLayoutClipLine(
        Writer->Line,
        (ULONG)RTL_NUMBER_OF(Writer->Line),
        MaximumCharacters);
    Writer->Status = Writer->Canvas->DrawText(
        Writer->Canvas->Context,
        Writer->OriginX + X,
        Y,
        Writer->Line,
        ColorIndex);
}

static VOID
KswordARKBugcheckLayoutWriteFrame(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame
    )
{
    if (Writer == NULL || !NT_SUCCESS(Writer->Status)) {
        return;
    }

    Writer->Status = Writer->Canvas->DrawFrame(
        Writer->Canvas->Context,
        Writer->OriginX + X,
        Y,
        Frame);
}

static PCSTR
KswordARKBugcheckLayoutModuleText(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    // Parenthesized resolver notes are not module names and add visual noise.
    if (Diagnostics->CandidateModule[0] == '\0' ||
        Diagnostics->CandidateModule[0] == '(') {
        return "DUMP ANALYSIS REQUIRED";
    }
    return Diagnostics->CandidateModule;
}

static PCSTR
KswordARKBugcheckLayoutSummaryText(
    _In_ ULONG Classification
    )
{
    switch (Classification) {
    case KSWORD_ARK_BUGCHECK_MODULE_OURS:
        return "KSWORDARK MAY BE INVOLVED.";
    case KSWORD_ARK_BUGCHECK_MODULE_MICROSOFT:
        return "PARAMETERS POINT TO MICROSOFT CODE.";
    case KSWORD_ARK_BUGCHECK_MODULE_THIRD_PARTY:
        return "PARAMETERS POINT TO THIRD-PARTY CODE.";
    default:
        return "ATTRIBUTION REQUIRES DUMP ANALYSIS.";
    }
}

BOOLEAN
KswordARKBugcheckLayoutHasCandidate(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    return Diagnostics->CandidateClass != KSWORD_ARK_BUGCHECK_MODULE_UNKNOWN &&
        Diagnostics->CandidateAddress != 0 &&
        Diagnostics->CandidateModule[0] != '\0' &&
        Diagnostics->CandidateModule[0] != '(';
}

BOOLEAN
KswordARKBugcheckLayoutHasDirectFaultAddress(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    return Diagnostics->FaultParameter != 0 && Diagnostics->FaultAddress != 0;
}

PCSTR
KswordARKBugcheckLayoutCriticalObjectTypeText(
    _In_ ULONG_PTR Value
    )
{
    if (Value == 0) {
        return "PROCESS";
    }
    if (Value == 1) {
        return "THREAD";
    }
    return "INVALID";
}

PCSTR
KswordARKBugcheckLayoutAttributionText(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        return "ATTRIBUTION  RESOLVED";
    }
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        return "ATTRIBUTION  ADDRESS UNMAPPED";
    }
    return "ATTRIBUTION  DUMP REQUIRED";
}

ULONG
KswordARKBugcheckLayoutAttributionColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    switch (Diagnostics->CandidateClass) {
    case KSWORD_ARK_BUGCHECK_MODULE_OURS:
        return KswordArkBugcheckLayoutColorCritical;
    case KSWORD_ARK_BUGCHECK_MODULE_MICROSOFT:
        return KswordArkBugcheckLayoutColorAccent;
    case KSWORD_ARK_BUGCHECK_MODULE_THIRD_PARTY:
        return KswordArkBugcheckLayoutColorWarning;
    default:
        return KswordArkBugcheckLayoutColorWarning;
    }
}

PCSTR
KswordARKBugcheckLayoutParameterRole(
    _In_ ULONG BugCheckCode,
    _In_ ULONG ParameterIndex
    )
{
    switch (BugCheckCode) {
    case 0x0000000A:
    case 0x000000D1:
        switch (ParameterIndex) {
        case 1: return "MEMORY";
        case 2: return "IRQL";
        case 3: return "OPERATION";
        case 4: return "INSTRUCTION";
        default: break;
        }
        break;
    case 0x0000001E:
    case 0x0000003B:
    case 0x0000007E:
        switch (ParameterIndex) {
        case 1: return "EXCEPTION";
        case 2: return "INSTRUCTION";
        case 3: return "CONTEXT";
        case 4: return "CONTEXT";
        default: break;
        }
        break;
    case 0x00000050:
        switch (ParameterIndex) {
        case 1: return "MEMORY";
        case 2: return "OPERATION";
        case 3: return "INSTRUCTION";
        case 4: return "FAULT TYPE";
        default: break;
        }
        break;
    case 0x000000BE:
        switch (ParameterIndex) {
        case 1: return "WRITE ADDRESS";
        case 2: return "PTE CONTENT";
        case 3:
        case 4: return "RESERVED";
        default: break;
        }
        break;
    case 0x000000C5:
        switch (ParameterIndex) {
        case 1: return "MEMORY";
        case 2: return "IRQL";
        case 3: return "OPERATION";
        case 4: return "INSTRUCTION";
        default: break;
        }
        break;
    case 0x000000D5:
        switch (ParameterIndex) {
        case 1: return "MEMORY";
        case 2: return "OPERATION";
        case 3: return "INSTRUCTION";
        case 4: return "RESERVED";
        default: break;
        }
        break;
    case 0x000000EF:
        switch (ParameterIndex) {
        case 1: return "PROCESS OBJ";
        case 2: return "OBJECT TYPE";
        case 3:
        case 4: return "RESERVED";
        default: break;
        }
        break;
    case 0x00000116:
        switch (ParameterIndex) {
        case 1: return "TDR CONTEXT";
        case 2: return "DRIVER POINTER";
        case 3: return "STATUS";
        case 4: return "INTERNAL";
        default: break;
        }
        break;
    case 0x00000117:
        switch (ParameterIndex) {
        case 1: return "TDR CONTEXT";
        case 2: return "DRIVER POINTER";
        case 3: return "BUCKET KEY";
        case 4: return "INTERNAL";
        default: break;
        }
        break;
    default:
        break;
    }
    return "RAW";
}

ULONG_PTR
KswordARKBugcheckLayoutParameterValue(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ParameterIndex
    )
{
    switch (ParameterIndex) {
    case 1: return Diagnostics->Parameter1;
    case 2: return Diagnostics->Parameter2;
    case 3: return Diagnostics->Parameter3;
    case 4: return Diagnostics->Parameter4;
    default: return 0;
    }
}

static PCSTR
KswordARKBugcheckLayoutShortParameterRole(
    _In_z_ PCSTR Role
    )
{
    if (Role[0] == 'I' && Role[1] == 'N' && Role[2] == 'S') {
        return "IP";
    }
    if (Role[0] == 'P' && Role[1] == 'R') {
        return "PROC OBJ";
    }
    if (Role[0] == 'D' && Role[1] == 'R') {
        return "DRIVER";
    }
    if (Role[0] == 'W') {
        return "WRITE VA";
    }
    if (Role[0] == 'F') {
        return "TYPE";
    }
    if (Role[0] == 'T') {
        return "TDR CTX";
    }
    if (Role[0] == 'B') {
        return "BUCKET";
    }
    if (Role[0] == 'O' && Role[1] == 'P') {
        return "OP";
    }
    if (Role[0] == 'E') {
        return "EXCEPT";
    }
    return Role;
}

ULONG
KswordARKBugcheckLayoutParameterColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ParameterIndex
    )
{
    if (Diagnostics->BugCheckCode == 0x000000EF && ParameterIndex <= 2) {
        return KswordArkBugcheckLayoutColorCritical;
    }
    if (Diagnostics->FaultParameter == ParameterIndex) {
        return KswordArkBugcheckLayoutColorCritical;
    }
    if ((Diagnostics->BugCheckCode == 0x000000BE && ParameterIndex >= 3) ||
        (Diagnostics->BugCheckCode == 0x000000D5 && ParameterIndex == 4) ||
        (Diagnostics->BugCheckCode == 0x000000EF && ParameterIndex >= 3)) {
        return KswordArkBugcheckLayoutColorMuted;
    }
    return KswordArkBugcheckLayoutColorText;
}

static VOID
KswordARKBugcheckLayoutWriteParameter(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG MaximumCharacters,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ParameterIndex
    )
{
    ULONG_PTR value;
    PCSTR role;

    value = KswordARKBugcheckLayoutParameterValue(
        Diagnostics,
        ParameterIndex);
    role = KswordARKBugcheckLayoutParameterRole(
        Diagnostics->BugCheckCode,
        ParameterIndex);
    if (MaximumCharacters <= 31UL) {
        role = KswordARKBugcheckLayoutShortParameterRole(role);
    }
    if (Diagnostics->BugCheckCode == 0x000000EF && ParameterIndex == 2) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            X,
            Y,
            KswordARKBugcheckLayoutParameterColor(
                Diagnostics,
                ParameterIndex),
            MaximumCharacters,
            "ARG2 OBJECT TYPE  %s",
            KswordARKBugcheckLayoutCriticalObjectTypeText(value));
        return;
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        X,
        Y,
        KswordARKBugcheckLayoutParameterColor(Diagnostics, ParameterIndex),
        MaximumCharacters,
        "ARG%lu %s 0x%p",
        ParameterIndex,
        role,
        (PVOID)value);
}

ULONG
KswordARKBugcheckLayoutCallbackCount(
    _In_ ULONG CallbackMask
    )
{
    ULONG count;
    ULONG bit;

    count = 0;
    for (bit = 0; bit < 4; ++bit) {
        if ((CallbackMask & (1UL << bit)) != 0) {
            ++count;
        }
    }
    return count;
}

BOOLEAN
KswordARKBugcheckLayoutDumpIsSequential(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    return Diagnostics->DumpOffset == ~((ULONG64)0);
}

ULONG
KswordARKBugcheckLayoutDumpStageColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    return Diagnostics->LastDumpType == KbDumpIoComplete
        ? KswordArkBugcheckLayoutColorSuccess
        : KswordArkBugcheckLayoutColorAccent;
}

static VOID
KswordARKBugcheckLayoutWriteHeader(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount,
    _In_ BOOLEAN Compact
    )
{
    if (Compact) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 16L, KswordArkBugcheckLayoutColorAccent, 31UL,
            "KSWORD ARK CRASH DIAGNOSTICS");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 38L, KswordArkBugcheckLayoutColorCritical, 31UL,
            "A FATAL KERNEL ERROR OCCURRED.");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 56L, KswordArkBugcheckLayoutColorMuted, 31UL,
            "PRESERVE THE NEWEST CRASH DUMP.");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 74L, KswordArkBugcheckLayoutColorWarning, 31UL,
            "DO NOT POWER OFF DURING DUMP I/O.");
        // Keep the captured runtime counters in the narrow area to the right
        // of the header copy.  X=559 accounts for the BGP glyph's one-pixel
        // backing border as well as the shared renderer's 9-pixel advance.
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 559L, 16L, KswordArkBugcheckLayoutColorText, 9UL,
            "CPU %02lu", Diagnostics->Cpu);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 559L, 36L, KswordArkBugcheckLayoutColorText, 9UL,
            "IRQL %lu", Diagnostics->Irql);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 559L, 54L, KswordArkBugcheckLayoutColorSuccess, 9UL,
            "CB 0x%02lX", CallbackMask & 0x0FUL);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 559L, 72L, KswordArkBugcheckLayoutColorMuted, 9UL,
            "MOD %lu", ModuleCount);
        return;
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 18L, KswordArkBugcheckLayoutColorAccent, 50UL,
        "KSWORD ARK CRASH DIAGNOSTICS");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 40L, KswordArkBugcheckLayoutColorCritical, 48UL,
        "A FATAL KERNEL ERROR HAS OCCURRED.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 56L, KswordArkBugcheckLayoutColorMuted, 58UL,
        "THE SYSTEM HAS STOPPED TO PROTECT DATA.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 72L, KswordArkBugcheckLayoutColorMuted, 58UL,
        "THIS SCREEN PRESERVES THE CRASH CONTEXT.");
    if (!KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 18L, KswordArkBugcheckLayoutColorText, 38UL,
            "THIS IS NOBODY'S FAULT.");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 38L, KswordArkBugcheckLayoutColorCritical, 38UL,
            "YOUR COMPUTER JUST EXPLODED.");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 58L, KswordArkBugcheckLayoutColorWarning, 38UL,
            "RESTART IT, REINSTALL IT,");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 76L, KswordArkBugcheckLayoutColorWarning, 38UL,
            "OR BUY A NEW ONE.");
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            692L,
            18L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            38UL,
            "%s",
            KswordARKBugcheckLayoutSummaryText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 40L, KswordArkBugcheckLayoutColorText, 38UL,
            "MODULE  %s", KswordARKBugcheckLayoutModuleText(Diagnostics));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            692L,
            60L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            38UL,
            "%s",
            KswordARKBugcheckLayoutAttributionText(Diagnostics));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 78L, KswordArkBugcheckLayoutColorMuted, 38UL,
            "PRESERVE THE NEWEST CRASH DUMP.");
    }
}

static VOID
KswordARKBugcheckLayoutDrawCompact(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    PCSTR moduleText;

    moduleText = KswordARKBugcheckLayoutModuleText(Diagnostics);
    KswordARKBugcheckLayoutWriteFrame(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_LEFT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_UPPER_Y,
        KswordArkBugcheckLayoutFrameCompactUpper);
    KswordARKBugcheckLayoutWriteFrame(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_RIGHT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_UPPER_Y,
        KswordArkBugcheckLayoutFrameCompactUpper);
    KswordARKBugcheckLayoutWriteFrame(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_LEFT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_LOWER_Y,
        KswordArkBugcheckLayoutFrameCompactLower);
    KswordARKBugcheckLayoutWriteFrame(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_RIGHT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_LOWER_Y,
        KswordArkBugcheckLayoutFrameCompactLower);

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 114L, KswordArkBugcheckLayoutColorMuted, 29UL,
        "BUGCHECK");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 132L, KswordArkBugcheckLayoutColorCritical, 29UL,
        "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 148L, KswordArkBugcheckLayoutColorCritical, 29UL,
        "CODE  0x%08lX", Diagnostics->BugCheckCode);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 164L, 29UL, Diagnostics, 1UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 178L, 29UL, Diagnostics, 2UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 192L, 29UL, Diagnostics, 3UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 206L, 29UL, Diagnostics, 4UL);

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 114L, KswordArkBugcheckLayoutColorMuted, 29UL,
        "FAULT CONTEXT");
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 132L, KswordArkBugcheckLayoutColorCritical, 29UL,
            "FAULT IP  0x%p", (PVOID)Diagnostics->FaultAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 148L, KswordArkBugcheckLayoutColorText, 29UL,
            "FROM ARG%lu", Diagnostics->FaultParameter);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 164L, KswordArkBugcheckLayoutColorMuted, 29UL,
            "%s", Diagnostics->FaultMeaning);
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 132L, KswordArkBugcheckLayoutColorWarning, 29UL,
            "NO DIRECT FAULT IP");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 148L, KswordArkBugcheckLayoutColorCritical, 29UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 164L, KswordArkBugcheckLayoutColorAccent, 29UL,
                "OBJECT  0x%p", (PVOID)Diagnostics->Parameter1);
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 148L, KswordArkBugcheckLayoutColorMuted, 29UL,
                "STOP CODE HAS NO DIRECT IP");
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 164L, KswordArkBugcheckLayoutColorText, 29UL,
                "RAW PARAMETERS PRESERVED");
        }
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        340L,
        180L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics),
        29UL,
        "%s",
        KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 198L, KswordArkBugcheckLayoutColorText, 29UL,
        "CPU / IRQL  %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 210L, KswordArkBugcheckLayoutColorSuccess, 29UL,
        "DIAGNOSTICS CAPTURED");

    // The 228..284 band remains completely untouched for Windows' own
    // crash-dump progress text.  Both compact rows stop outside that band.
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 300L, KswordArkBugcheckLayoutColorMuted, 29UL,
        KswordARKBugcheckLayoutHasCandidate(Diagnostics)
            ? "FAULTING MODULE"
            : "ATTRIBUTION");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            28L,
            318L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            29UL,
            "%s",
            moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 336L, KswordArkBugcheckLayoutColorText, 29UL,
            "BASE  0x%p", (PVOID)Diagnostics->CandidateModuleBase);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 352L, KswordArkBugcheckLayoutColorText, 29UL,
            "SIZE  0x%08lX", Diagnostics->CandidateModuleSize);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 368L, KswordArkBugcheckLayoutColorText, 29UL,
            "OFF   0x%p", (PVOID)Diagnostics->CandidateModuleOffset);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 384L, KswordArkBugcheckLayoutColorCritical, 29UL,
            "ADDR  0x%p", (PVOID)Diagnostics->CandidateAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 400L, KswordArkBugcheckLayoutColorMuted, 29UL,
            "SOURCE %s", Diagnostics->CandidateSource);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 416L, KswordArkBugcheckLayoutColorText, 29UL,
            "CLASS  %s",
            KswordARKBugcheckModuleClassText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 430L, KswordArkBugcheckLayoutColorSuccess, 29UL,
            "CONF   %s",
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence));
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 318L, KswordArkBugcheckLayoutColorWarning, 29UL,
            "DUMP ANALYSIS REQUIRED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 338L, KswordArkBugcheckLayoutColorMuted, 29UL,
            KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)
                ? "FAULT ADDRESS OUTSIDE CACHE"
                : "STOP CODE HAS NO DIRECT IP");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 358L, KswordArkBugcheckLayoutColorSuccess, 29UL,
            "MODULE CACHE  %lu READY", ModuleCount);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 378L, KswordArkBugcheckLayoutColorSuccess, 29UL,
            "RAW PARAMETERS DECODED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 398L, KswordArkBugcheckLayoutColorText, 29UL,
            "NO MODULE CLAIMED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 418L, KswordArkBugcheckLayoutColorAccent, 29UL,
            "NEXT  RUN !ANALYZE -V");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 430L, KswordArkBugcheckLayoutColorMuted, 29UL,
            "PRESERVE THE CRASH DUMP");
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 300L, KswordArkBugcheckLayoutColorMuted, 29UL,
        "DUMP PIPELINE");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 318L, KswordARKBugcheckLayoutDumpStageColor(Diagnostics), 29UL,
        "STAGE  %s", KswordARKBugcheckDumpTypeText(Diagnostics->LastDumpType));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 336L, KswordArkBugcheckLayoutColorText, 29UL,
        "REASON %s", KswordARKBugcheckReasonText(Diagnostics->LastReason));
    if (KswordARKBugcheckLayoutDumpIsSequential(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 352L, KswordArkBugcheckLayoutColorSuccess, 29UL,
            "OFFSET SEQUENTIAL");
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 352L, KswordArkBugcheckLayoutColorText, 29UL,
            "OFFSET 0x%p", (PVOID)(ULONG_PTR)Diagnostics->DumpOffset);
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 368L, KswordArkBugcheckLayoutColorText, 29UL,
        "CHUNK  0x%08lX", Diagnostics->DumpBufferLength);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 384L, KswordArkBugcheckLayoutColorSuccess, 29UL,
        "CALLBACKS %lu / 4 READY",
        KswordARKBugcheckLayoutCallbackCount(CallbackMask));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 400L, KswordArkBugcheckLayoutColorSuccess, 29UL,
        "MODULE CACHE  %lu READY", ModuleCount);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 416L, KswordArkBugcheckLayoutColorMuted, 29UL,
        "PROGRESS WINDOWS MANAGED");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 430L, KswordArkBugcheckLayoutColorText, 29UL,
        "ACTION PRESERVE NEWEST DUMP");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_LEFT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y,
        KswordArkBugcheckLayoutColorMuted,
        68UL,
        "WINDOWS IS WRITING THE CRASH DUMP. DO NOT POWER OFF.");
}

static VOID
KswordARKBugcheckLayoutDrawFull(
    _Inout_ KSWORD_ARK_BUGCHECK_LAYOUT_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    PCSTR moduleText;

    moduleText = KswordARKBugcheckLayoutModuleText(Diagnostics);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 16L, 110L, KswordArkBugcheckLayoutFrameFullTopLeft);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 364L, 110L, KswordArkBugcheckLayoutFrameFullTopMiddle);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 680L, 110L, KswordArkBugcheckLayoutFrameFullTopRight);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 16L, 298L, KswordArkBugcheckLayoutFrameFullMiddleLeft);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 364L, 298L, KswordArkBugcheckLayoutFrameFullMiddleMiddle);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 680L, 298L, KswordArkBugcheckLayoutFrameFullMiddleRight);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 16L, 476L, KswordArkBugcheckLayoutFrameFullBottomLeft);
    KswordARKBugcheckLayoutWriteFrame(
        Writer, 508L, 476L, KswordArkBugcheckLayoutFrameFullBottomRight);

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 122L, KswordArkBugcheckLayoutColorMuted, 35UL,
        "BUGCHECK");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 144L, KswordArkBugcheckLayoutColorCritical, 35UL,
        "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 164L, KswordArkBugcheckLayoutColorCritical, 35UL,
        "0x%08lX", Diagnostics->BugCheckCode);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 190L, KswordArkBugcheckLayoutColorText, 35UL,
        "SYSTEM STOPPED SAFELY");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        28L,
        208L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics),
        35UL,
        "%s", KswordARKBugcheckLayoutSummaryText(Diagnostics->CandidateClass));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        28L,
        226L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics),
        35UL,
        "%s",
        KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 244L, KswordArkBugcheckLayoutColorSuccess, 35UL,
        "DIAGNOSTICS CAPTURED");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 122L, KswordArkBugcheckLayoutColorMuted, 31UL,
        "CRASH CONTEXT");
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 144L, KswordArkBugcheckLayoutColorCritical, 31UL,
            "FAULT IP  0x%p", (PVOID)Diagnostics->FaultAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 164L, KswordArkBugcheckLayoutColorText, 31UL,
            "FROM ARG%lu", Diagnostics->FaultParameter);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 182L, KswordArkBugcheckLayoutColorMuted, 31UL,
            "%s", Diagnostics->FaultMeaning);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            376L,
            200L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            31UL,
            "%s",
            KswordARKBugcheckLayoutAttributionText(Diagnostics));
    } else {
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 376L, 144L, KswordArkBugcheckLayoutColorAccent, 31UL,
                "OBJECT  0x%p", (PVOID)Diagnostics->Parameter1);
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 376L, 164L, KswordArkBugcheckLayoutColorCritical, 31UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 376L, 144L, KswordArkBugcheckLayoutColorText, 31UL,
                "RAW PARAMETERS PRESERVED");
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 376L, 164L, KswordArkBugcheckLayoutColorMuted, 31UL,
                "NO PARAMETER IS A DIRECT IP");
        }
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 182L, KswordArkBugcheckLayoutColorWarning, 31UL,
            "NO DIRECT FAULT IP");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 200L, KswordArkBugcheckLayoutColorWarning, 31UL,
            "ATTRIBUTION  DUMP REQUIRED");
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 218L, KswordArkBugcheckLayoutColorText, 31UL,
        "CPU / IRQL  %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 236L, KswordArkBugcheckLayoutColorSuccess, 31UL,
        "RAW PARAMETERS DECODED");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 254L, KswordArkBugcheckLayoutColorMuted, 31UL,
        "DUMP PRESERVES ROOT-CAUSE DATA");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 122L, KswordArkBugcheckLayoutColorMuted, 32UL,
        "DUMP STATUS");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 144L, KswordARKBugcheckLayoutDumpStageColor(Diagnostics), 32UL,
        "STAGE   %s", KswordARKBugcheckDumpTypeText(Diagnostics->LastDumpType));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 164L, KswordArkBugcheckLayoutColorText, 32UL,
        "REASON  %s", KswordARKBugcheckReasonText(Diagnostics->LastReason));
    if (KswordARKBugcheckLayoutDumpIsSequential(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 182L, KswordArkBugcheckLayoutColorSuccess, 32UL,
            "OFFSET  SEQUENTIAL");
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 182L, KswordArkBugcheckLayoutColorText, 32UL,
            "OFFSET  0x%p", (PVOID)(ULONG_PTR)Diagnostics->DumpOffset);
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 200L, KswordArkBugcheckLayoutColorText, 32UL,
        "CHUNK   0x%08lX", Diagnostics->DumpBufferLength);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 218L, KswordArkBugcheckLayoutColorSuccess, 32UL,
        "CALLBACKS %lu / 4 READY",
        KswordARKBugcheckLayoutCallbackCount(CallbackMask));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 236L, KswordArkBugcheckLayoutColorSuccess, 32UL,
        "DIAGNOSTICS CAPTURED");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 254L, KswordArkBugcheckLayoutColorMuted, 32UL,
        "PROGRESS IS MANAGED BY WINDOWS");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 310L, KswordArkBugcheckLayoutColorMuted, 35UL,
        "CRASH PARAMETERS");
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 334L, 35UL, Diagnostics, 1UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 354L, 35UL, Diagnostics, 2UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 374L, 35UL, Diagnostics, 3UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 28L, 394L, 35UL, Diagnostics, 4UL);
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 420L, KswordArkBugcheckLayoutColorCritical, 35UL,
            "FAULT IP COMES FROM ARG%lu", Diagnostics->FaultParameter);
    } else if (Diagnostics->BugCheckCode == 0x000000EF) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 420L, KswordArkBugcheckLayoutColorWarning, 35UL,
            "ARG1 IS AN OBJECT, NOT CODE");
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 420L, KswordArkBugcheckLayoutColorMuted, 35UL,
            "NO DIRECT INSTRUCTION PARAMETER");
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 310L, KswordArkBugcheckLayoutColorMuted, 31UL,
        KswordARKBugcheckLayoutHasCandidate(Diagnostics)
            ? "FAULTING MODULE"
            : "ATTRIBUTION");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            376L,
            334L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            31UL,
            "%s",
            moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 354L, KswordArkBugcheckLayoutColorText, 31UL,
            "BASE   0x%p", (PVOID)Diagnostics->CandidateModuleBase);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 374L, KswordArkBugcheckLayoutColorText, 31UL,
            "SIZE   0x%08lX", Diagnostics->CandidateModuleSize);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 394L, KswordArkBugcheckLayoutColorText, 31UL,
            "OFFSET 0x%p", (PVOID)Diagnostics->CandidateModuleOffset);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 414L, KswordArkBugcheckLayoutColorText, 31UL,
            "CLASS  %s",
            KswordARKBugcheckModuleClassText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 434L, KswordArkBugcheckLayoutColorSuccess, 31UL,
            "CONF   %s",
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence));
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 334L, KswordArkBugcheckLayoutColorWarning, 31UL,
            "DUMP ANALYSIS REQUIRED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 354L, KswordArkBugcheckLayoutColorMuted, 31UL,
            KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)
                ? "ADDRESS OUTSIDE MODULE CACHE"
                : "STOP CODE HAS NO DIRECT IP");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 374L, KswordArkBugcheckLayoutColorSuccess, 31UL,
            "MODULE CACHE  %lu AVAILABLE", ModuleCount);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 394L, KswordArkBugcheckLayoutColorText, 31UL,
            "NO MODULE CLAIMED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 414L, KswordArkBugcheckLayoutColorMuted, 31UL,
            "ROOT CAUSE NEEDS DUMP STACK");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 376L, 434L, KswordArkBugcheckLayoutColorAccent, 31UL,
            "NEXT  RUN !ANALYZE -V");
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 310L, KswordArkBugcheckLayoutColorMuted, 32UL,
        "SYSTEM SNAPSHOT");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 334L, KswordArkBugcheckLayoutColorText, 32UL,
        "CPU / IRQL   %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 354L, KswordArkBugcheckLayoutColorSuccess, 32UL,
        "MODULE CACHE  %lu READY", ModuleCount);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 374L, KswordArkBugcheckLayoutColorMuted, 32UL,
        "DRIVER OBJ  0x%p", g_KswordArkBugcheckState.DriverObject);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 394L, KswordArkBugcheckLayoutColorMuted, 32UL,
        "DEVICE OBJ  0x%p", g_KswordArkBugcheckState.DeviceObject);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 414L, KswordArkBugcheckLayoutColorSuccess, 32UL,
        "CLASSIC / TRIAGE  %lu / %lu",
        (ULONG)((CallbackMask & 0x1UL) != 0),
        (ULONG)((CallbackMask & 0x8UL) != 0));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 434L, KswordArkBugcheckLayoutColorSuccess, 32UL,
        "DUMP / SECONDARY  %lu / %lu",
        (ULONG)((CallbackMask & 0x4UL) != 0),
        (ULONG)((CallbackMask & 0x2UL) != 0));

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 488L, KswordArkBugcheckLayoutColorMuted, 50UL,
        "DIAGNOSTIC SUMMARY");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            28L,
            512L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            50UL,
            "%s",
            KswordARKBugcheckLayoutSummaryText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 50UL,
            "CANDIDATE  %s", moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 554L, KswordArkBugcheckLayoutColorSuccess, 50UL,
            "CONFIDENCE %s",
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence));
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 512L, KswordArkBugcheckLayoutColorCritical, 50UL,
            Diagnostics->BugCheckCode == 0x000000EF
                ? "A CRITICAL PROCESS OR THREAD TERMINATED."
                : "THE SYSTEM STOPPED WITHOUT A DIRECT FAULT IP.");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 50UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 50UL,
                "RAW BUGCHECK PARAMETERS WERE PRESERVED.");
        }
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 554L, KswordArkBugcheckLayoutColorWarning, 50UL,
            "MODULE ATTRIBUTION REQUIRES THE CRASH DUMP.");
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 582L, KswordArkBugcheckLayoutColorAccent, 50UL,
        "ACTION  PRESERVE AND ANALYZE THE NEWEST DUMP");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 604L, KswordArkBugcheckLayoutColorMuted, 50UL,
        "REPORT  ATTACH THIS SCREEN AND THE CRASH DUMP");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 488L, KswordArkBugcheckLayoutColorMuted, 52UL,
        "HELP");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 512L, KswordArkBugcheckLayoutColorText, 52UL,
        "> PRESERVE THE NEWEST CRASH DUMP.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 534L, KswordArkBugcheckLayoutColorText, 52UL,
        "> REVIEW RECENT DRIVER OR HARDWARE CHANGES.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 556L, KswordArkBugcheckLayoutColorAccent, 52UL,
        "> ANALYZE THE DUMP IN KSWORDARK.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 578L, KswordArkBugcheckLayoutColorWarning, 52UL,
        "> DO NOT POWER OFF DURING DUMP WRITING.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 606L, KswordArkBugcheckLayoutColorMuted, 52UL,
        "WINDOWS CONTROLS THE FINAL RESTART.");

    // Keep the lower progress band free for text painted by Windows itself.
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 16L, 724L, KswordArkBugcheckLayoutColorWarning, 80UL,
        "WAITING FOR WINDOWS TO COMPLETE THE CRASH DUMP. DO NOT POWER OFF.");
}

NTSTATUS
KswordARKBugcheckLayoutDraw(
    _In_ const KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS* Canvas,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    KSWORD_ARK_BUGCHECK_LAYOUT_WRITER writer;
    BOOLEAN compact;
    BOOLEAN detailed;

    if (Canvas == NULL || Diagnostics == NULL ||
        Canvas->DrawText == NULL || Canvas->DrawFrame == NULL ||
        Canvas->Width < KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH ||
        Canvas->Height < KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&writer, sizeof(writer));
    writer.Canvas = Canvas;
    writer.OriginX = KswordARKBugcheckLayoutOriginX(
        Canvas->Width,
        Canvas->Height);
    writer.Status = STATUS_SUCCESS;
    detailed = KswordARKBugcheckLayoutIsDetailed(
        Canvas->Width,
        Canvas->Height);
    compact = KswordARKBugcheckLayoutIsCompact(Canvas->Width, Canvas->Height);

    if (detailed) {
        return KswordARKBugcheckLayoutDrawDetailed(
            Canvas,
            Diagnostics,
            CallbackMask,
            ModuleCount);
    }
    KswordARKBugcheckLayoutWriteHeader(
        &writer,
        Diagnostics,
        CallbackMask,
        ModuleCount,
        compact);
    if (compact) {
        KswordARKBugcheckLayoutDrawCompact(
            &writer,
            Diagnostics,
            CallbackMask,
            ModuleCount);
    } else {
        KswordARKBugcheckLayoutDrawFull(
            &writer,
            Diagnostics,
            CallbackMask,
            ModuleCount);
    }
    return writer.Status;
}
