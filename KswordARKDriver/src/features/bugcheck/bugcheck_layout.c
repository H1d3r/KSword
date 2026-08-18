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
#define KSWORD_ARK_BUGCHECK_COMPACT_UPPER_HEIGHT 124UL
// The final 17-pixel system-font cell remains clear of the frame border.
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
        { 340UL, 184UL },
        { 312UL, 184UL },
        { 340UL, 184UL },
        { 340UL, 174UL },
        { 312UL, 174UL },
        { 340UL, 174UL },
        { 484UL, 180UL },
        { 500UL, 180UL },
        { 400UL, 190UL },
        { 400UL, 190UL },
        { 432UL, 190UL },
        { 400UL, 176UL },
        { 400UL, 176UL },
        { 432UL, 176UL },
        { 612UL, 190UL },
        { 628UL, 190UL }
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
    KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y +
        KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_HEIGHT <=
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
        ColorIndex,
        KswordArkBugcheckLayoutTextBody);
}

static VOID
KswordARKBugcheckLayoutWriteHeroFormatted(
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
        ColorIndex,
        KswordArkBugcheckLayoutTextHero);
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
    case KSWORD_ARK_BUGCHECK_MODULE_MICROSOFT:
    case KSWORD_ARK_BUGCHECK_MODULE_THIRD_PARTY:
        return KswordArkBugcheckLayoutColorAccent;
    default:
        return KswordArkBugcheckLayoutColorMuted;
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
    if (Diagnostics->FaultParameter == ParameterIndex) {
        return KswordArkBugcheckLayoutColorAccent;
    }
    if (Diagnostics->BugCheckCode == 0x000000EF && ParameterIndex <= 2) {
        return KswordArkBugcheckLayoutColorText;
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
    UNREFERENCED_PARAMETER(CallbackMask);
    UNREFERENCED_PARAMETER(ModuleCount);

    if (Compact) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 4L, KswordArkBugcheckLayoutColorAccent, 24UL,
            "KSWORD CRASH DIAGNOSTICS");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 25L, KswordArkBugcheckLayoutColorCritical, 24UL,
            "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
        KswordARKBugcheckLayoutWriteHeroFormatted(
            Writer, 272L, 45L, KswordArkBugcheckLayoutColorCritical, 10UL,
            "0x%08lX", Diagnostics->BugCheckCode);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 494L, 54L, KswordArkBugcheckLayoutColorMuted, 9UL,
            "STOP CODE");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 272L, 83L, KswordArkBugcheckLayoutColorText, 26UL,
            "THIS IS NOBODY'S FAULT.");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 558L, 4L, KswordArkBugcheckLayoutColorText, 7UL,
            "CPU %02lu", Diagnostics->Cpu);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 558L, 25L, KswordArkBugcheckLayoutColorText, 7UL,
            "IRQL %lu", Diagnostics->Irql);
        return;
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 10L, KswordArkBugcheckLayoutColorAccent, 29UL,
        "KSWORD ARK CRASH DIAGNOSTICS");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 280L, 31L, KswordArkBugcheckLayoutColorCritical, 29UL,
        "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
    KswordARKBugcheckLayoutWriteHeroFormatted(
        Writer, 280L, 52L, KswordArkBugcheckLayoutColorCritical, 10UL,
        "0x%08lX", Diagnostics->BugCheckCode);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 502L, 61L, KswordArkBugcheckLayoutColorMuted, 10UL,
        "STOP CODE");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 620L, 10L, KswordArkBugcheckLayoutColorText, 28UL,
        "THIS IS NOBODY'S FAULT.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 620L, 31L, KswordArkBugcheckLayoutColorText, 28UL,
        "YOUR COMPUTER JUST EXPLODED.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 620L, 52L, KswordArkBugcheckLayoutColorText, 28UL,
        "RESTART IT, REINSTALL IT,");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 620L, 73L, KswordArkBugcheckLayoutColorText, 28UL,
        "OR BUY A NEW ONE.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 930L, 10L, KswordArkBugcheckLayoutColorText, 7UL,
        "CPU %02lu", Diagnostics->Cpu);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 930L, 31L, KswordArkBugcheckLayoutColorText, 7UL,
        "IRQL %02lu", Diagnostics->Irql);
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
        Writer, 28L, 114L, KswordArkBugcheckLayoutColorAccent, 24UL,
        "PARAMETER SEMANTICS");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 136L,
        KswordARKBugcheckLayoutParameterColor(Diagnostics, 1UL), 24UL,
        "ARG1  0x%p",
        (PVOID)KswordARKBugcheckLayoutParameterValue(Diagnostics, 1UL));
    if (Diagnostics->BugCheckCode == 0x000000EF) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 154L, KswordArkBugcheckLayoutColorText, 24UL,
            "ARG2 TYPE  %s",
            KswordARKBugcheckLayoutCriticalObjectTypeText(
                Diagnostics->Parameter2));
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 154L,
            KswordARKBugcheckLayoutParameterColor(Diagnostics, 2UL), 24UL,
            "ARG2  0x%p",
            (PVOID)KswordARKBugcheckLayoutParameterValue(Diagnostics, 2UL));
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 172L,
        KswordARKBugcheckLayoutParameterColor(Diagnostics, 3UL), 24UL,
        "ARG3  0x%p",
        (PVOID)KswordARKBugcheckLayoutParameterValue(Diagnostics, 3UL));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 190L,
        KswordARKBugcheckLayoutParameterColor(Diagnostics, 4UL), 24UL,
        "ARG4  0x%p",
        (PVOID)KswordARKBugcheckLayoutParameterValue(Diagnostics, 4UL));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 208L, KswordArkBugcheckLayoutColorMuted, 24UL,
        "RAW CALLBACK VALUES");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 114L, KswordArkBugcheckLayoutColorAccent, 24UL,
        "FAULT EVIDENCE");
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 136L, KswordArkBugcheckLayoutColorAccent, 24UL,
            "FAULT IP  0x%p", (PVOID)Diagnostics->FaultAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 154L, KswordArkBugcheckLayoutColorText, 24UL,
            "FROM ARG%lu", Diagnostics->FaultParameter);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 172L, KswordArkBugcheckLayoutColorMuted, 24UL,
            "%s", Diagnostics->FaultMeaning);
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 340L, 136L, KswordArkBugcheckLayoutColorWarning, 24UL,
            "NO DIRECT FAULT IP");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 154L, KswordArkBugcheckLayoutColorAccent, 24UL,
                "OBJ  0x%p", (PVOID)Diagnostics->Parameter1);
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 172L, KswordArkBugcheckLayoutColorText, 24UL,
                "TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 154L, KswordArkBugcheckLayoutColorMuted, 24UL,
                "STOP CODE HAS NO DIRECT IP");
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 340L, 172L, KswordArkBugcheckLayoutColorText, 24UL,
                "RAW PARAMETERS PRESERVED");
        }
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 190L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics),
        24UL, "%s",
        KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 208L, KswordArkBugcheckLayoutColorText, 24UL,
        "CPU / IRQL  %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);

    // The 228..284 band remains completely untouched for Windows' own
    // crash-dump progress text.  Both compact rows stop outside that band.
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 300L, KswordArkBugcheckLayoutColorAccent, 24UL,
        "ATTRIBUTION RESULT");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 322L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            24UL, "%s", moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 344L, KswordArkBugcheckLayoutColorAccent, 24UL,
            "ADDR  0x%p", (PVOID)Diagnostics->CandidateAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 366L, KswordArkBugcheckLayoutColorText, 24UL,
            "BASE  0x%p", (PVOID)Diagnostics->CandidateModuleBase);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 388L, KswordArkBugcheckLayoutColorText, 24UL,
            "OFF   0x%p", (PVOID)Diagnostics->CandidateModuleOffset);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 410L, KswordArkBugcheckLayoutColorText, 24UL,
            "%s / %s",
            KswordARKBugcheckModuleClassText(Diagnostics->CandidateClass),
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 426L, KswordArkBugcheckLayoutColorMuted, 24UL,
            "SOURCE %s", Diagnostics->CandidateSource);
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 322L, KswordArkBugcheckLayoutColorWarning, 24UL,
            "DUMP ANALYSIS REQUIRED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 344L, KswordArkBugcheckLayoutColorMuted, 24UL,
            KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)
                ? "FAULT ADDRESS OUTSIDE CACHE"
                : "STOP CODE HAS NO DIRECT IP");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 366L, KswordArkBugcheckLayoutColorText, 24UL,
            "MODULE CACHE %lu SEARCHED", ModuleCount);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 388L, KswordArkBugcheckLayoutColorText, 24UL,
            "NO MODULE CLAIMED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 410L, KswordArkBugcheckLayoutColorMuted, 24UL,
            "ROOT CAUSE NEEDS STACK");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 426L, KswordArkBugcheckLayoutColorAccent, 24UL,
            "NEXT  !ANALYZE -V");
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 300L, KswordArkBugcheckLayoutColorAccent, 24UL,
        "CAPTURE HEALTH");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 322L,
        KswordARKBugcheckLayoutCallbackCount(CallbackMask) == 4UL
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorWarning,
        24UL,
        "CALLBACKS %lu / 4 READY",
        KswordARKBugcheckLayoutCallbackCount(CallbackMask));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 344L, KswordArkBugcheckLayoutColorText, 24UL,
        "CLASSIC %lu  SECONDARY %lu",
        (ULONG)((CallbackMask & 0x1UL) != 0),
        (ULONG)((CallbackMask & 0x2UL) != 0));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 366L, KswordArkBugcheckLayoutColorText, 24UL,
        "WRITE CB %lu  TRIAGE %lu",
        (ULONG)((CallbackMask & 0x4UL) != 0),
        (ULONG)((CallbackMask & 0x8UL) != 0));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 388L, KswordArkBugcheckLayoutColorSuccess, 24UL,
        "MODULE CACHE %lu READY", ModuleCount);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 410L,
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Font.Valid, 1, 1) != 0
                ? KswordArkBugcheckLayoutColorSuccess
                : KswordArkBugcheckLayoutColorMuted,
        24UL,
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Font.Valid, 1, 1) != 0
                ? "FONT SYSTEM MONO"
                : "FONT BUILT-IN FALLBACK");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 340L, 426L, KswordArkBugcheckLayoutColorMuted, 24UL,
        "%s %lux%lu %luBPP",
        Writer->Canvas->RendererName != NULL
            ? Writer->Canvas->RendererName
            : "RENDER",
        Writer->Canvas->Width,
        Writer->Canvas->Height,
        Writer->Canvas->BitsPerPixel);

    KswordARKBugcheckLayoutWriteFormatted(
        Writer,
        KSWORD_ARK_BUGCHECK_COMPACT_LEFT_X,
        KSWORD_ARK_BUGCHECK_COMPACT_FOOTER_Y,
        KswordArkBugcheckLayoutColorMuted,
        56UL,
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
        Writer, 28L, 122L, KswordArkBugcheckLayoutColorAccent, 28UL,
        "CRASH IDENTITY");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 146L, KswordArkBugcheckLayoutColorText, 28UL,
        "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 168L, KswordArkBugcheckLayoutColorMuted, 28UL,
        "CODE  0x%08lX", Diagnostics->BugCheckCode);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 192L, KswordArkBugcheckLayoutColorText, 28UL,
        "CAPTURED  %s", Diagnostics->Captured != 0 ? "YES" : "NO");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 214L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics), 28UL,
        "%s", KswordARKBugcheckLayoutSummaryText(Diagnostics->CandidateClass));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 236L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics), 28UL,
        "%s", KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 258L, KswordArkBugcheckLayoutColorMuted, 28UL,
        "PERF ID  0x%p",
        (PVOID)(ULONG_PTR)Diagnostics->PerfCounter.QuadPart);

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 122L, KswordArkBugcheckLayoutColorAccent, 26UL,
        "PARAMETER SEMANTICS");
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 376L, 146L, 26UL, Diagnostics, 1UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 376L, 168L, 26UL, Diagnostics, 2UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 376L, 190L, 26UL, Diagnostics, 3UL);
    KswordARKBugcheckLayoutWriteParameter(
        Writer, 376L, 212L, 26UL, Diagnostics, 4UL);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 236L, KswordArkBugcheckLayoutColorText, 26UL,
        Diagnostics->BugCheckCode == 0x000000EF
            ? "ARG1 IS AN OBJECT, NOT CODE"
            : "RAW CALLBACK VALUES PRESERVED");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 258L, KswordArkBugcheckLayoutColorMuted, 26UL,
        "DECODED WITHOUT MEMORY READS");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 122L, KswordArkBugcheckLayoutColorAccent, 28UL,
        "FAULT EVIDENCE");
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 146L, KswordArkBugcheckLayoutColorAccent, 28UL,
            "FAULT IP  0x%p", (PVOID)Diagnostics->FaultAddress);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 168L, KswordArkBugcheckLayoutColorText, 28UL,
            "SOURCE PARAMETER  ARG%lu", Diagnostics->FaultParameter);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 190L, KswordArkBugcheckLayoutColorMuted, 28UL,
            "%s", Diagnostics->FaultMeaning);
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 692L, 146L, KswordArkBugcheckLayoutColorWarning, 28UL,
            "NO DIRECT FAULT IP");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 692L, 168L, KswordArkBugcheckLayoutColorAccent, 28UL,
                "CRITICAL OBJECT  0x%p",
                (PVOID)Diagnostics->Parameter1);
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 692L, 190L, KswordArkBugcheckLayoutColorText, 28UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 692L, 168L, KswordArkBugcheckLayoutColorText, 28UL,
                "NO EXECUTABLE ARGUMENT");
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 692L, 190L, KswordArkBugcheckLayoutColorMuted, 28UL,
                "RAW PARAMETERS RETAINED");
        }
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 214L, KswordArkBugcheckLayoutColorText, 28UL,
        "CPU / IRQL  %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 236L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics), 28UL,
        "%s", KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 258L, KswordArkBugcheckLayoutColorMuted, 28UL,
        "SAFE CALLBACK DATA ONLY");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 310L, KswordArkBugcheckLayoutColorAccent, 28UL,
        "ATTRIBUTION RESULT");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            28L,
            334L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            28UL,
            "%s",
            moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 356L, KswordArkBugcheckLayoutColorText, 28UL,
            "BASE   0x%p", (PVOID)Diagnostics->CandidateModuleBase);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 378L, KswordArkBugcheckLayoutColorText, 28UL,
            "OFFSET 0x%p", (PVOID)Diagnostics->CandidateModuleOffset);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 400L, KswordArkBugcheckLayoutColorText, 28UL,
            "SIZE   0x%08lX", Diagnostics->CandidateModuleSize);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 422L, KswordArkBugcheckLayoutColorText, 28UL,
            "CLASS  %s",
            KswordARKBugcheckModuleClassText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 444L, KswordArkBugcheckLayoutColorSuccess, 28UL,
            "CONF / SOURCE  %s / %s",
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence),
            Diagnostics->CandidateSource[0] != '\0'
                ? Diagnostics->CandidateSource
                : "CALLBACK");
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 334L, KswordArkBugcheckLayoutColorWarning, 28UL,
            "DUMP ANALYSIS REQUIRED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 356L, KswordArkBugcheckLayoutColorMuted, 28UL,
            KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)
                ? "ADDRESS OUTSIDE MODULE CACHE"
                : "STOP CODE HAS NO DIRECT IP");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 378L, KswordArkBugcheckLayoutColorSuccess, 28UL,
            "MODULE CACHE  %lu AVAILABLE", ModuleCount);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 400L, KswordArkBugcheckLayoutColorText, 28UL,
            "NO MODULE CLAIMED");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 422L, KswordArkBugcheckLayoutColorMuted, 28UL,
            "ROOT CAUSE NEEDS STACK");
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 444L, KswordArkBugcheckLayoutColorAccent, 28UL,
            "NEXT  RUN !ANALYZE -V");
    }

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 310L, KswordArkBugcheckLayoutColorAccent, 26UL,
        "CAPTURE HEALTH");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 334L,
        KswordARKBugcheckLayoutCallbackCount(CallbackMask) == 4UL
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorWarning,
        26UL,
        "CALLBACKS  %lu / 4 READY",
        KswordARKBugcheckLayoutCallbackCount(CallbackMask));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 356L, KswordArkBugcheckLayoutColorText, 26UL,
        "CLASSIC / SECONDARY %lu / %lu",
        (ULONG)((CallbackMask & 0x1UL) != 0),
        (ULONG)((CallbackMask & 0x2UL) != 0));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 378L, KswordArkBugcheckLayoutColorText, 26UL,
        "WRITE / TRIAGE     %lu / %lu",
        (ULONG)((CallbackMask & 0x4UL) != 0),
        (ULONG)((CallbackMask & 0x8UL) != 0));
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 400L, KswordArkBugcheckLayoutColorSuccess, 26UL,
        "MODULE CACHE  %lu READY", ModuleCount);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 422L,
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Font.Valid, 1, 1) != 0
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorMuted,
        26UL,
        "FONT  %s",
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Font.Valid, 1, 1) != 0
            ? "SYSTEM MONO"
            : "BUILT-IN 8X12");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 376L, 444L, KswordArkBugcheckLayoutColorText, 26UL,
        "CAPTURED  %s", Diagnostics->Captured != 0 ? "YES" : "NO");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 310L, KswordArkBugcheckLayoutColorAccent, 28UL,
        "RENDER PATH");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 334L, KswordArkBugcheckLayoutColorText, 28UL,
        "RENDERER  %s",
        Writer->Canvas->RendererName != NULL
            ? Writer->Canvas->RendererName
            : "UNKNOWN");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 356L, KswordArkBugcheckLayoutColorText, 28UL,
        "CANVAS  %lu X %lu",
        Writer->Canvas->Width,
        Writer->Canvas->Height);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 378L, KswordArkBugcheckLayoutColorText, 28UL,
        "COLOR  %lu BPP", Writer->Canvas->BitsPerPixel);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 400L, KswordArkBugcheckLayoutColorText, 28UL,
        "GLYPH  %lu X %lu",
        KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH,
        KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT);
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 422L,
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Bitmap.Valid, 1, 1) != 0
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorMuted,
        28UL,
        "BRAND  %s",
        InterlockedCompareExchange(
            &g_KswordArkBugcheckState.Bitmap.Valid, 1, 1) != 0
            ? "CUSTOM BITMAP"
            : "BUILT-IN MARK");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 692L, 444L, KswordArkBugcheckLayoutColorMuted, 28UL,
        "CRASH-SAFE RESOURCES READY");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 488L, KswordArkBugcheckLayoutColorAccent, 41UL,
        "DIAGNOSTIC SUMMARY");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer,
            28L,
            512L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            41UL,
            "%s",
            KswordARKBugcheckLayoutSummaryText(Diagnostics->CandidateClass));
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 41UL,
            "CANDIDATE  %s", moduleText);
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 556L, KswordArkBugcheckLayoutColorSuccess, 41UL,
            "CONFIDENCE %s",
            KswordARKBugcheckConfidenceText(Diagnostics->CandidateConfidence));
    } else {
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 512L, KswordArkBugcheckLayoutColorText, 41UL,
            Diagnostics->BugCheckCode == 0x000000EF
                ? "A CRITICAL PROCESS OR THREAD TERMINATED."
                : "THE SYSTEM STOPPED WITHOUT A DIRECT FAULT IP.");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 41UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckLayoutWriteFormatted(
                Writer, 28L, 534L, KswordArkBugcheckLayoutColorText, 41UL,
                "RAW BUGCHECK PARAMETERS WERE PRESERVED.");
        }
        KswordARKBugcheckLayoutWriteFormatted(
            Writer, 28L, 556L, KswordArkBugcheckLayoutColorWarning, 41UL,
            "MODULE ATTRIBUTION REQUIRES THE CRASH DUMP.");
    }
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 584L, KswordArkBugcheckLayoutColorAccent, 41UL,
        "ACTION  PRESERVE AND ANALYZE THE NEWEST DUMP");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 28L, 606L, KswordArkBugcheckLayoutColorMuted, 41UL,
        "REPORT  ATTACH THIS SCREEN AND THE CRASH DUMP");

    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 488L, KswordArkBugcheckLayoutColorAccent, 43UL,
        "NEXT ACTION");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 512L, KswordArkBugcheckLayoutColorText, 43UL,
        "> PRESERVE THE NEWEST CRASH DUMP.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 534L, KswordArkBugcheckLayoutColorText, 43UL,
        "> REVIEW RECENT DRIVER OR HARDWARE CHANGES.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 556L, KswordArkBugcheckLayoutColorAccent, 43UL,
        "> ANALYZE THE DUMP IN KSWORDARK.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 578L, KswordArkBugcheckLayoutColorWarning, 43UL,
        "> DO NOT POWER OFF DURING DUMP WRITING.");
    KswordARKBugcheckLayoutWriteFormatted(
        Writer, 520L, 606L, KswordArkBugcheckLayoutColorMuted, 43UL,
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
