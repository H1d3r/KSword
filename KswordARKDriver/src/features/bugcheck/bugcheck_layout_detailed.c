/*++

Module Name:

    bugcheck_layout_detailed.c

Abstract:

    Information-dense 1280x720 crash layout for the physical BGP and VMware
    framebuffer renderers. Every displayed value is either captured by the
    bugcheck callbacks or derived from immutable crash-safe caches.

--*/

#include "bugcheck_layout_detailed.h"

#include <ntstrsafe.h>
#include <stdarg.h>

typedef struct _KSWORD_ARK_BUGCHECK_DETAILED_WRITER
{
    const KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS* Canvas;
    LONG OriginX;
    NTSTATUS Status;
    CHAR Line[KSWORD_ARK_BUGCHECK_PANEL_LINE_CHARS];
} KSWORD_ARK_BUGCHECK_DETAILED_WRITER;

static VOID
KswordARKBugcheckDetailedClipLine(
    _Inout_updates_z_(Capacity) PCHAR Text,
    _In_ ULONG Capacity,
    _In_ ULONG MaximumCharacters
    )
{
    SIZE_T length;

    if (Text == NULL || Capacity == 0UL || MaximumCharacters == 0UL) {
        return;
    }
    if (MaximumCharacters >= Capacity) {
        MaximumCharacters = Capacity - 1UL;
    }

    length = 0UL;
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
KswordARKBugcheckDetailedWriteV(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG ColorIndex,
    _In_ ULONG MaximumCharacters,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_STYLE TextStyle,
    _In_z_ _Printf_format_string_ PCSTR Format,
    _In_ va_list Arguments
    )
{
    if (Writer == NULL || !NT_SUCCESS(Writer->Status)) {
        return;
    }

    Writer->Status = RtlStringCbVPrintfA(
        Writer->Line,
        sizeof(Writer->Line),
        Format,
        Arguments);
    if (!NT_SUCCESS(Writer->Status)) {
        return;
    }

    KswordARKBugcheckDetailedClipLine(
        Writer->Line,
        (ULONG)RTL_NUMBER_OF(Writer->Line),
        MaximumCharacters);
    Writer->Status = Writer->Canvas->DrawText(
        Writer->Canvas->Context,
        Writer->OriginX + X,
        Y,
        Writer->Line,
        ColorIndex,
        TextStyle);
}

static VOID
KswordARKBugcheckDetailedWrite(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG ColorIndex,
    _In_ ULONG MaximumCharacters,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    va_list arguments;

    va_start(arguments, Format);
    KswordARKBugcheckDetailedWriteV(
        Writer,
        X,
        Y,
        ColorIndex,
        MaximumCharacters,
        KswordArkBugcheckLayoutTextBody,
        Format,
        arguments);
    va_end(arguments);
}

static VOID
KswordARKBugcheckDetailedWriteHero(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ ULONG ColorIndex,
    _In_ ULONG MaximumCharacters,
    _In_z_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    va_list arguments;

    va_start(arguments, Format);
    KswordARKBugcheckDetailedWriteV(
        Writer,
        X,
        Y,
        ColorIndex,
        MaximumCharacters,
        KswordArkBugcheckLayoutTextHero,
        Format,
        arguments);
    va_end(arguments);
}

static VOID
KswordARKBugcheckDetailedFrame(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
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
KswordARKBugcheckDetailedModuleText(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    if (Diagnostics->CandidateModule[0] == '\0' ||
        Diagnostics->CandidateModule[0] == '(') {
        return "NO MODULE CLAIMED";
    }
    return Diagnostics->CandidateModule;
}

static PCSTR
KswordARKBugcheckDetailedSourceText(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    if (Diagnostics->CandidateSource[0] == '\0') {
        return "CALLBACK PARAMETER";
    }
    return Diagnostics->CandidateSource;
}

static BOOLEAN
KswordARKBugcheckDetailedUsesSystemFont(VOID)
{
    return InterlockedCompareExchange(
        &g_KswordArkBugcheckState.Font.Valid,
        1,
        1) != 0;
}

static BOOLEAN
KswordARKBugcheckDetailedUsesCustomBrand(VOID)
{
    return InterlockedCompareExchange(
        &g_KswordArkBugcheckState.Bitmap.Valid,
        1,
        1) != 0;
}

static VOID
KswordARKBugcheckDetailedWriteParameter(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
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
    if (Diagnostics->BugCheckCode == 0x000000EF && ParameterIndex == 2UL) {
        KswordARKBugcheckDetailedWrite(
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

    KswordARKBugcheckDetailedWrite(
        Writer,
        X,
        Y,
        KswordARKBugcheckLayoutParameterColor(Diagnostics, ParameterIndex),
        MaximumCharacters,
        "ARG%lu %s  0x%p",
        ParameterIndex,
        role,
        (PVOID)value);
}

static VOID
KswordARKBugcheckDetailedDrawHeader(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 280L, 10L, KswordArkBugcheckLayoutColorAccent, 42UL,
        "KSWORD ARK CRASH DIAGNOSTICS");
    KswordARKBugcheckDetailedWrite(
        Writer, 280L, 31L, KswordArkBugcheckLayoutColorCritical, 42UL,
        "%s", KswordARKBugcheckName(Diagnostics->BugCheckCode));
    KswordARKBugcheckDetailedWriteHero(
        Writer, 280L, 52L, KswordArkBugcheckLayoutColorCritical, 10UL,
        "0x%08lX", Diagnostics->BugCheckCode);
    KswordARKBugcheckDetailedWrite(
        Writer, 502L, 61L, KswordArkBugcheckLayoutColorMuted, 18UL,
        "STOP CODE");

    // Keep the original joke white and in the original fixed-width voice.
    KswordARKBugcheckDetailedWrite(
        Writer, 760L, 10L, KswordArkBugcheckLayoutColorText, 30UL,
        "THIS IS NOBODY'S FAULT.");
    KswordARKBugcheckDetailedWrite(
        Writer, 760L, 31L, KswordArkBugcheckLayoutColorText, 30UL,
        "YOUR COMPUTER JUST EXPLODED.");
    KswordARKBugcheckDetailedWrite(
        Writer, 760L, 52L, KswordArkBugcheckLayoutColorText, 30UL,
        "RESTART IT, REINSTALL IT,");
    KswordARKBugcheckDetailedWrite(
        Writer, 760L, 73L, KswordArkBugcheckLayoutColorText, 30UL,
        "OR BUY A NEW ONE.");
    KswordARKBugcheckDetailedWrite(
        Writer, 1148L, 10L, KswordArkBugcheckLayoutColorText, 12UL,
        "CRASH CPU %02lu", Diagnostics->Cpu);
    KswordARKBugcheckDetailedWrite(
        Writer, 1148L, 31L, KswordArkBugcheckLayoutColorText, 12UL,
        "IRQL %02lu", Diagnostics->Irql);
}

static VOID
KswordARKBugcheckDetailedDrawParameters(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 118L, KswordArkBugcheckLayoutColorAccent, 34UL,
        "PARAMETER SEMANTICS");
    KswordARKBugcheckDetailedWriteParameter(
        Writer, 28L, 142L, 34UL, Diagnostics, 1UL);
    KswordARKBugcheckDetailedWriteParameter(
        Writer, 28L, 164L, 34UL, Diagnostics, 2UL);
    KswordARKBugcheckDetailedWriteParameter(
        Writer, 28L, 186L, 34UL, Diagnostics, 3UL);
    KswordARKBugcheckDetailedWriteParameter(
        Writer, 28L, 208L, 34UL, Diagnostics, 4UL);
    KswordARKBugcheckDetailedWrite(
        Writer,
        28L,
        234L,
        Diagnostics->BugCheckCode == 0x000000EF
            ? KswordArkBugcheckLayoutColorWarning
            : KswordArkBugcheckLayoutColorText,
        34UL,
        Diagnostics->BugCheckCode == 0x000000EF
            ? "ARG1 IS AN OBJECT, NOT CODE"
            : "RAW CALLBACK VALUES PRESERVED");
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 256L, KswordArkBugcheckLayoutColorMuted, 34UL,
        "DECODED WITHOUT MEMORY READS");
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 274L, KswordArkBugcheckLayoutColorText, 34UL,
        "CAPTURED  %s", Diagnostics->Captured != 0 ? "YES" : "NO");
}

static VOID
KswordARKBugcheckDetailedDrawFaultEvidence(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 118L, KswordArkBugcheckLayoutColorAccent, 34UL,
        "FAULT EVIDENCE");
    if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
        KswordARKBugcheckDetailedWrite(
            Writer, 436L, 142L, KswordArkBugcheckLayoutColorAccent, 34UL,
            "FAULT IP  0x%p", (PVOID)Diagnostics->FaultAddress);
        KswordARKBugcheckDetailedWrite(
            Writer, 436L, 164L, KswordArkBugcheckLayoutColorText, 34UL,
            "SOURCE PARAMETER  ARG%lu", Diagnostics->FaultParameter);
        KswordARKBugcheckDetailedWrite(
            Writer, 436L, 186L, KswordArkBugcheckLayoutColorText, 34UL,
            "%s", Diagnostics->FaultMeaning);
        KswordARKBugcheckDetailedWrite(
            Writer, 436L, 208L, KswordArkBugcheckLayoutColorMuted, 34UL,
            "SEARCH ADDRESS  0x%p",
            (PVOID)Diagnostics->CandidateAddress);
    } else {
        KswordARKBugcheckDetailedWrite(
            Writer, 436L, 142L, KswordArkBugcheckLayoutColorWarning, 34UL,
            "NO DIRECT FAULT IP");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckDetailedWrite(
                Writer, 436L, 164L, KswordArkBugcheckLayoutColorAccent, 34UL,
                "CRITICAL OBJECT  0x%p",
                (PVOID)Diagnostics->Parameter1);
            KswordARKBugcheckDetailedWrite(
                Writer, 436L, 186L, KswordArkBugcheckLayoutColorText, 34UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckDetailedWrite(
                Writer, 436L, 164L, KswordArkBugcheckLayoutColorText, 34UL,
                "NO EXECUTABLE ARGUMENT");
            KswordARKBugcheckDetailedWrite(
                Writer, 436L, 186L, KswordArkBugcheckLayoutColorMuted, 34UL,
                "RAW PARAMETERS RETAINED");
        }
    }
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 230L, KswordArkBugcheckLayoutColorText, 34UL,
        "CPU / IRQL  %lu / %lu", Diagnostics->Cpu, Diagnostics->Irql);
    KswordARKBugcheckDetailedWrite(
        Writer,
        436L,
        252L,
        KswordARKBugcheckLayoutAttributionColor(Diagnostics),
        34UL,
        "%s",
        KswordARKBugcheckLayoutAttributionText(Diagnostics));
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 274L, KswordArkBugcheckLayoutColorMuted, 34UL,
        "SAFE CALLBACK DATA ONLY");
}

static VOID
KswordARKBugcheckDetailedDrawAttribution(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ModuleCount
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 118L, KswordArkBugcheckLayoutColorAccent, 37UL,
        "ATTRIBUTION RESULT");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckDetailedWrite(
            Writer,
            844L,
            142L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            37UL,
            "%s",
            KswordARKBugcheckDetailedModuleText(Diagnostics));
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 164L, KswordArkBugcheckLayoutColorText, 37UL,
            "BASE    0x%p", (PVOID)Diagnostics->CandidateModuleBase);
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 186L, KswordArkBugcheckLayoutColorText, 37UL,
            "OFFSET  0x%p", (PVOID)Diagnostics->CandidateModuleOffset);
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 208L, KswordArkBugcheckLayoutColorText, 37UL,
            "SIZE    0x%08lX", Diagnostics->CandidateModuleSize);
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 230L, KswordArkBugcheckLayoutColorText, 37UL,
            "CLASS   %s",
            KswordARKBugcheckModuleClassText(Diagnostics->CandidateClass));
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 252L, KswordArkBugcheckLayoutColorSuccess, 37UL,
            "CONF    %s",
            KswordARKBugcheckConfidenceText(
                Diagnostics->CandidateConfidence));
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 274L, KswordArkBugcheckLayoutColorMuted, 37UL,
            "SOURCE  %s",
            KswordARKBugcheckDetailedSourceText(Diagnostics));
    } else {
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 142L, KswordArkBugcheckLayoutColorWarning, 37UL,
            "DUMP ANALYSIS REQUIRED");
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 164L, KswordArkBugcheckLayoutColorMuted, 37UL,
            KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)
                ? "ADDRESS OUTSIDE MODULE CACHE"
                : "STOP CODE HAS NO DIRECT IP");
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 186L, KswordArkBugcheckLayoutColorSuccess, 37UL,
            "MODULE CACHE  %lu AVAILABLE", ModuleCount);
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 208L, KswordArkBugcheckLayoutColorText, 37UL,
            "NO MODULE RANGE CLAIMED");
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 230L, KswordArkBugcheckLayoutColorMuted, 37UL,
            "ROOT CAUSE NEEDS SAVED STACK");
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 252L, KswordArkBugcheckLayoutColorAccent, 37UL,
            "NEXT  RUN !ANALYZE -V");
        KswordARKBugcheckDetailedWrite(
            Writer, 844L, 274L, KswordArkBugcheckLayoutColorText, 37UL,
            "RAW EVIDENCE RETAINED");
    }
}

static VOID
KswordARKBugcheckDetailedDrawModuleEvidence(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ModuleCount
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 316L, KswordArkBugcheckLayoutColorAccent, 34UL,
        "MODULE EVIDENCE");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 340L, KswordArkBugcheckLayoutColorText, 34UL,
            "ADDRESS  0x%p", (PVOID)Diagnostics->CandidateAddress);
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 362L, KswordArkBugcheckLayoutColorText, 34UL,
            "PARAMETER  ARG%lu", Diagnostics->CandidateParameter);
        KswordARKBugcheckDetailedWrite(
            Writer,
            28L,
            384L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            34UL,
            "MODULE  %s",
            KswordARKBugcheckDetailedModuleText(Diagnostics));
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 406L, KswordArkBugcheckLayoutColorText, 34UL,
            "BASE + OFFSET VERIFIED");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 428L, KswordArkBugcheckLayoutColorMuted, 34UL,
            "SOURCE  %s",
            KswordARKBugcheckDetailedSourceText(Diagnostics));
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 450L, KswordArkBugcheckLayoutColorSuccess, 34UL,
            "CACHE RANGE MATCHED");
    } else {
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 340L, KswordArkBugcheckLayoutColorText, 34UL,
            "CACHE ENTRIES  %lu", ModuleCount);
        if (KswordARKBugcheckLayoutHasDirectFaultAddress(Diagnostics)) {
            KswordARKBugcheckDetailedWrite(
                Writer, 28L, 362L, KswordArkBugcheckLayoutColorText, 34UL,
                "SEARCHED  0x%p", (PVOID)Diagnostics->FaultAddress);
        } else {
            KswordARKBugcheckDetailedWrite(
                Writer, 28L, 362L, KswordArkBugcheckLayoutColorMuted, 34UL,
                "NO SEARCHABLE CODE ADDRESS");
        }
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 384L, KswordArkBugcheckLayoutColorSuccess, 34UL,
            "RANGE SEARCH COMPLETE");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 406L, KswordArkBugcheckLayoutColorWarning, 34UL,
            "NO MODULE RANGE MATCHED");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 428L, KswordArkBugcheckLayoutColorText, 34UL,
            "RAW EVIDENCE PRESERVED");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 450L, KswordArkBugcheckLayoutColorAccent, 34UL,
            "NEXT  STACK + SYMBOLS");
    }
}

static VOID
KswordARKBugcheckDetailedDrawCaptureHealth(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    ULONG callbackCount;

    callbackCount = KswordARKBugcheckLayoutCallbackCount(CallbackMask);
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 316L, KswordArkBugcheckLayoutColorAccent, 34UL,
        "CAPTURE HEALTH");
    KswordARKBugcheckDetailedWrite(
        Writer,
        436L,
        340L,
        callbackCount == 4UL
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorWarning,
        34UL,
        "CALLBACKS  %lu / 4 READY",
        callbackCount);
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 362L, KswordArkBugcheckLayoutColorText, 34UL,
        "CLASSIC / SECONDARY  %lu / %lu",
        (ULONG)((CallbackMask & 0x1UL) != 0),
        (ULONG)((CallbackMask & 0x2UL) != 0));
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 384L, KswordArkBugcheckLayoutColorText, 34UL,
        "WRITE / TRIAGE      %lu / %lu",
        (ULONG)((CallbackMask & 0x4UL) != 0),
        (ULONG)((CallbackMask & 0x8UL) != 0));
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 406L, KswordArkBugcheckLayoutColorSuccess, 34UL,
        "MODULE CACHE  %lu READY", ModuleCount);
    KswordARKBugcheckDetailedWrite(
        Writer,
        436L,
        428L,
        KswordARKBugcheckDetailedUsesSystemFont()
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorMuted,
        34UL,
        "FONT  %s",
        KswordARKBugcheckDetailedUsesSystemFont()
            ? "SYSTEM MONO"
            : "BUILT-IN 8X12 FALLBACK");
    KswordARKBugcheckDetailedWrite(
        Writer, 436L, 450L, KswordArkBugcheckLayoutColorText, 34UL,
        "CAPTURED  %s", Diagnostics->Captured != 0 ? "YES" : "NO");
}

static VOID
KswordARKBugcheckDetailedDrawRenderPath(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 316L, KswordArkBugcheckLayoutColorAccent, 37UL,
        "RENDER CONTRACT");
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 340L, KswordArkBugcheckLayoutColorText, 37UL,
        "RENDERER  %s",
        Writer->Canvas->RendererName != NULL
            ? Writer->Canvas->RendererName
            : "UNKNOWN");
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 362L, KswordArkBugcheckLayoutColorText, 37UL,
        "CANVAS  %lu X %lu X %lu BPP",
        Writer->Canvas->Width,
        Writer->Canvas->Height,
        Writer->Canvas->BitsPerPixel);
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 384L, KswordArkBugcheckLayoutColorText, 37UL,
        "BODY GLYPH  %lu X %lu / ADV %lu",
        KSWORD_ARK_BUGCHECK_FONT_BODY_WIDTH,
        KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT,
        KSWORD_ARK_BUGCHECK_FONT_BODY_ADVANCE);
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 406L, KswordArkBugcheckLayoutColorText, 37UL,
        "HERO GLYPH  %lu X %lu / ADV %lu",
        KSWORD_ARK_BUGCHECK_FONT_HERO_WIDTH,
        KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT,
        KSWORD_ARK_BUGCHECK_FONT_HERO_ADVANCE);
    KswordARKBugcheckDetailedWrite(
        Writer,
        844L,
        428L,
        KswordARKBugcheckDetailedUsesCustomBrand()
            ? KswordArkBugcheckLayoutColorSuccess
            : KswordArkBugcheckLayoutColorMuted,
        37UL,
        "BRAND  %s",
        KswordARKBugcheckDetailedUsesCustomBrand()
            ? "CUSTOM BITMAP"
            : "BUILT-IN MARK");
    KswordARKBugcheckDetailedWrite(
        Writer, 844L, 450L, KswordArkBugcheckLayoutColorMuted, 37UL,
        "PERF ID  0x%p",
        (PVOID)(ULONG_PTR)Diagnostics->PerfCounter.QuadPart);
}

static VOID
KswordARKBugcheckDetailedDrawSummary(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 500L, KswordArkBugcheckLayoutColorAccent, 53UL,
        "DIAGNOSTIC SUMMARY");
    if (KswordARKBugcheckLayoutHasCandidate(Diagnostics)) {
        KswordARKBugcheckDetailedWrite(
            Writer,
            28L,
            524L,
            KswordARKBugcheckLayoutAttributionColor(Diagnostics),
            53UL,
            "%s",
            KswordARKBugcheckVerdictText(Diagnostics->CandidateClass));
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 546L, KswordArkBugcheckLayoutColorText, 53UL,
            "CANDIDATE  %s",
            KswordARKBugcheckDetailedModuleText(Diagnostics));
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 568L, KswordArkBugcheckLayoutColorText, 53UL,
            "FAULT ADDRESS MAPS INSIDE THE CACHED MODULE RANGE.");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 590L, KswordArkBugcheckLayoutColorSuccess, 53UL,
            "CONFIDENCE  %s / SOURCE  %s",
            KswordARKBugcheckConfidenceText(
                Diagnostics->CandidateConfidence),
            KswordARKBugcheckDetailedSourceText(Diagnostics));
    } else {
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 524L, KswordArkBugcheckLayoutColorWarning, 53UL,
            Diagnostics->BugCheckCode == 0x000000EF
                ? "A CRITICAL PROCESS OR THREAD TERMINATED."
                : "THE SYSTEM STOPPED WITHOUT A DIRECT FAULT IP.");
        if (Diagnostics->BugCheckCode == 0x000000EF) {
            KswordARKBugcheckDetailedWrite(
                Writer, 28L, 546L, KswordArkBugcheckLayoutColorText, 53UL,
                "OBJECT TYPE  %s",
                KswordARKBugcheckLayoutCriticalObjectTypeText(
                    Diagnostics->Parameter2));
        } else {
            KswordARKBugcheckDetailedWrite(
                Writer, 28L, 546L, KswordArkBugcheckLayoutColorText, 53UL,
                "RAW BUGCHECK PARAMETERS WERE PRESERVED.");
        }
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 568L, KswordArkBugcheckLayoutColorWarning, 53UL,
            "NO MODULE CAN BE CLAIMED FROM THE CALLBACK VALUES ALONE.");
        KswordARKBugcheckDetailedWrite(
            Writer, 28L, 590L, KswordArkBugcheckLayoutColorMuted, 53UL,
            "ROOT CAUSE REQUIRES THE SAVED STACK AND SYMBOLS.");
    }
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 612L, KswordArkBugcheckLayoutColorAccent, 53UL,
        "ACTION  PRESERVE AND ANALYZE THE NEWEST CRASH DUMP.");
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 634L, KswordArkBugcheckLayoutColorText, 53UL,
        "REPORT  ATTACH THIS SCREEN WITH THE CRASH DUMP.");
    KswordARKBugcheckDetailedWrite(
        Writer, 28L, 656L, KswordArkBugcheckLayoutColorMuted, 53UL,
        "THE STOP CODE AND RAW ARGUMENTS REMAIN THE PRIMARY EVIDENCE.");
}

static VOID
KswordARKBugcheckDetailedDrawNextAction(
    _Inout_ KSWORD_ARK_BUGCHECK_DETAILED_WRITER* Writer
    )
{
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 500L, KswordArkBugcheckLayoutColorAccent, 54UL,
        "NEXT ACTION");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 524L, KswordArkBugcheckLayoutColorText, 54UL,
        "> PRESERVE THE NEWEST CRASH DUMP.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 546L, KswordArkBugcheckLayoutColorText, 54UL,
        "> PHOTOGRAPH THE STOP CODE AND RAW PARAMETERS.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 568L, KswordArkBugcheckLayoutColorAccent, 54UL,
        "> ANALYZE THE DUMP IN KSWORDARK OR WINDBG.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 590L, KswordArkBugcheckLayoutColorText, 54UL,
        "> REVIEW RECENT DRIVER OR HARDWARE CHANGES.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 612L, KswordArkBugcheckLayoutColorWarning, 54UL,
        "> DO NOT POWER OFF WHILE WINDOWS WRITES THE DUMP.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 634L, KswordArkBugcheckLayoutColorText, 54UL,
        "> IF REPRODUCIBLE, ATTACH THIS SCREEN AND THE DUMP.");
    KswordARKBugcheckDetailedWrite(
        Writer, 648L, 656L, KswordArkBugcheckLayoutColorMuted, 54UL,
        "WINDOWS CONTROLS THE FINAL RESTART.");
}

NTSTATUS
KswordARKBugcheckLayoutDrawDetailed(
    _In_ const KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS* Canvas,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    )
{
    KSWORD_ARK_BUGCHECK_DETAILED_WRITER writer;

    if (Canvas == NULL || Diagnostics == NULL ||
        Canvas->DrawText == NULL || Canvas->DrawFrame == NULL ||
        Canvas->Width < KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_WIDTH ||
        Canvas->Height < KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_HEIGHT) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(&writer, sizeof(writer));
    writer.Canvas = Canvas;
    writer.OriginX = KswordARKBugcheckLayoutOriginX(
        Canvas->Width,
        Canvas->Height);
    writer.Status = STATUS_SUCCESS;

    KswordARKBugcheckDetailedFrame(
        &writer, 16L, 106L, KswordArkBugcheckLayoutFrameDetailedTopLeft);
    KswordARKBugcheckDetailedFrame(
        &writer, 424L, 106L, KswordArkBugcheckLayoutFrameDetailedTopMiddle);
    KswordARKBugcheckDetailedFrame(
        &writer, 832L, 106L, KswordArkBugcheckLayoutFrameDetailedTopRight);
    KswordARKBugcheckDetailedFrame(
        &writer, 16L, 304L, KswordArkBugcheckLayoutFrameDetailedMiddleLeft);
    KswordARKBugcheckDetailedFrame(
        &writer, 424L, 304L, KswordArkBugcheckLayoutFrameDetailedMiddleMiddle);
    KswordARKBugcheckDetailedFrame(
        &writer, 832L, 304L, KswordArkBugcheckLayoutFrameDetailedMiddleRight);
    KswordARKBugcheckDetailedFrame(
        &writer, 16L, 488L, KswordArkBugcheckLayoutFrameDetailedBottomLeft);
    KswordARKBugcheckDetailedFrame(
        &writer, 636L, 488L, KswordArkBugcheckLayoutFrameDetailedBottomRight);

    KswordARKBugcheckDetailedDrawHeader(&writer, Diagnostics);
    KswordARKBugcheckDetailedDrawParameters(&writer, Diagnostics);
    KswordARKBugcheckDetailedDrawFaultEvidence(&writer, Diagnostics);
    KswordARKBugcheckDetailedDrawAttribution(
        &writer,
        Diagnostics,
        ModuleCount);
    KswordARKBugcheckDetailedDrawModuleEvidence(
        &writer,
        Diagnostics,
        ModuleCount);
    KswordARKBugcheckDetailedDrawCaptureHealth(
        &writer,
        Diagnostics,
        CallbackMask,
        ModuleCount);
    KswordARKBugcheckDetailedDrawRenderPath(&writer, Diagnostics);
    KswordARKBugcheckDetailedDrawSummary(&writer, Diagnostics);
    KswordARKBugcheckDetailedDrawNextAction(&writer);
    KswordARKBugcheckDetailedWrite(
        &writer, 16L, 700L, KswordArkBugcheckLayoutColorWarning, 90UL,
        "WAITING FOR WINDOWS TO COMPLETE THE CRASH DUMP. DO NOT POWER OFF.");
    return writer.Status;
}
