#include <ntddk.h>
#include <stdio.h>
#include <string.h>

#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_layout.h"
#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_layout_detailed.h"
#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_decode.h"

#define REPLAY_MAX_LINES 128UL

typedef struct _REPLAY_LINE
{
    LONG X;
    LONG Y;
    ULONG Color;
    CHAR Text[KSWORD_ARK_BUGCHECK_PANEL_LINE_CHARS];
} REPLAY_LINE;

typedef struct _REPLAY_CONTEXT
{
    ULONG Count;
    REPLAY_LINE Lines[REPLAY_MAX_LINES];
} REPLAY_CONTEXT;

KSWORD_ARK_BUGCHECK_STATE g_KswordArkBugcheckState;
UCHAR g_KswordArkBugcheckBitmapPixels[KSWORD_ARK_BUGCHECK_BITMAP_MAX_BYTES];

PCSTR
KswordARKBugcheckName(
    _In_ ULONG BugCheckCode
    )
{
    return BugCheckCode == 0x000000EF
        ? "CRITICAL_PROCESS_DIED"
        : "UNKNOWN_BUGCHECK_CODE";
}

PCSTR
KswordARKBugcheckModuleClassText(
    _In_ ULONG Classification
    )
{
    UNREFERENCED_PARAMETER(Classification);
    return "UNKNOWN";
}

PCSTR
KswordARKBugcheckConfidenceText(
    _In_ ULONG Confidence
    )
{
    UNREFERENCED_PARAMETER(Confidence);
    return "NONE";
}

PCSTR
KswordARKBugcheckVerdictText(
    _In_ ULONG Classification
    )
{
    UNREFERENCED_PARAMETER(Classification);
    return "The faulting component is unknown.";
}

PCSTR
KswordARKBugcheckReasonText(
    _In_ ULONG Reason
    )
{
    return Reason == KbCallbackDumpIo ? "DumpIo" : "Unknown";
}

PCSTR
KswordARKBugcheckDumpTypeText(
    _In_ ULONG DumpType
    )
{
    return DumpType == KbDumpIoHeader ? "Header" : "Unknown";
}

static NTSTATUS
ReplayDrawText(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_z_ PCSTR Text,
    _In_ ULONG ColorIndex
    )
{
    REPLAY_CONTEXT* replay = (REPLAY_CONTEXT*)Context;
    REPLAY_LINE* line;

    if (replay == NULL || replay->Count >= REPLAY_MAX_LINES) {
        return STATUS_BUFFER_OVERFLOW;
    }

    line = &replay->Lines[replay->Count++];
    line->X = X;
    line->Y = Y;
    line->Color = ColorIndex;
    (void)strncpy_s(line->Text, sizeof(line->Text), Text, _TRUNCATE);
    return STATUS_SUCCESS;
}

static NTSTATUS
ReplayDrawFrame(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame
    )
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(X);
    UNREFERENCED_PARAMETER(Y);
    UNREFERENCED_PARAMETER(Frame);
    return STATUS_SUCCESS;
}

static const REPLAY_LINE*
ReplayFindLine(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_z_ PCSTR Fragment
    )
{
    ULONG index;

    for (index = 0; index < Replay->Count; ++index) {
        if (strstr(Replay->Lines[index].Text, Fragment) != NULL) {
            return &Replay->Lines[index];
        }
    }
    return NULL;
}

static int
ReplayRequireLine(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_z_ PCSTR Fragment,
    _In_ LONG ExpectedColor
    )
{
    const REPLAY_LINE* line = ReplayFindLine(Replay, Fragment);

    if (line == NULL) {
        printf("FAIL missing line: %s\n", Fragment);
        return 1;
    }
    if (ExpectedColor >= 0 && line->Color != (ULONG)ExpectedColor) {
        printf(
            "FAIL wrong color: %s (actual=%lu expected=%ld)\n",
            Fragment,
            line->Color,
            ExpectedColor);
        return 1;
    }
    return 0;
}

static int
ReplayRequireLineAt(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_z_ PCSTR Fragment,
    _In_ LONG ExpectedX,
    _In_ LONG ExpectedY,
    _In_ ULONG ExpectedColor
    )
{
    const REPLAY_LINE* line = ReplayFindLine(Replay, Fragment);

    if (line == NULL) {
        printf("FAIL missing positioned line: %s\n", Fragment);
        return 1;
    }
    if (line->X != ExpectedX || line->Y != ExpectedY ||
        line->Color != ExpectedColor) {
        printf(
            "FAIL wrong placement: %s (actual=%ld,%ld,%lu expected=%ld,%ld,%lu)\n",
            Fragment,
            line->X,
            line->Y,
            line->Color,
            ExpectedX,
            ExpectedY,
            ExpectedColor);
        return 1;
    }
    return 0;
}

static int
ReplayRejectCriticalNoise(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_z_ PCSTR Name
    )
{
    ULONG index;
    int failures = 0;

    for (index = 0; index < Replay->Count; ++index) {
        const REPLAY_LINE* line = &Replay->Lines[index];

        if (line->Color == KswordArkBugcheckLayoutColorCritical &&
            strstr(line->Text, "CRITICAL_PROCESS_DIED") == NULL &&
            strstr(line->Text, "0x000000EF") == NULL) {
            printf("FAIL %s competing critical text: %s\n", Name, line->Text);
            ++failures;
        }
    }
    return failures;
}

static int
ReplayValidateRestrainedPalette(void)
{
    const UCHAR warning[3] = {
        KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_RED,
        KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_GREEN,
        KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_BLUE
    };
    const UCHAR muted[3] = {
        KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_RED,
        KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_GREEN,
        KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_BLUE
    };
    const UCHAR success[3] = {
        KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_RED,
        KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_GREEN,
        KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_BLUE
    };
    const UCHAR text[3] = {
        KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_RED,
        KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_GREEN,
        KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_BLUE
    };

    if (memcmp(warning, muted, sizeof(warning)) != 0 ||
        memcmp(success, text, sizeof(success)) != 0) {
        printf("FAIL warning/success colors still compete with the stop code\n");
        return 1;
    }
    return 0;
}

static int
ReplayRejectLine(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_z_ PCSTR Fragment
    )
{
    if (ReplayFindLine(Replay, Fragment) != NULL) {
        printf("FAIL misleading line still visible: %s\n", Fragment);
        return 1;
    }
    return 0;
}

static int
ReplayValidateCanvas(
    _In_ const REPLAY_CONTEXT* Replay,
    _In_ ULONG Width,
    _In_ ULONG Height,
    _In_z_ PCSTR Name
    )
{
    ULONG index;
    int failures = 0;

    for (index = 0; index < Replay->Count; ++index) {
        const REPLAY_LINE* line = &Replay->Lines[index];
        ULONG other;
        SIZE_T length = strlen(line->Text);

        if (line->X < 0 || line->Y < 0 ||
            (ULONG)line->X + (ULONG)(length * 9UL) > Width ||
            (ULONG)line->Y + 12UL > Height) {
            printf(
                "FAIL %s bounds: x=%ld y=%ld text=%s\n",
                Name,
                line->X,
                line->Y,
                line->Text);
            ++failures;
        }
        if (line->Color >= KswordArkBugcheckLayoutColorCount) {
            printf(
                "FAIL %s invalid color %lu: %s\n",
                Name,
                line->Color,
                line->Text);
            ++failures;
        }
        for (other = index + 1; other < Replay->Count; ++other) {
            if (Replay->Lines[other].X == line->X &&
                Replay->Lines[other].Y == line->Y) {
                printf(
                    "FAIL %s overdraw at %ld,%ld: %s / %s\n",
                    Name,
                    line->X,
                    line->Y,
                    line->Text,
                    Replay->Lines[other].Text);
                ++failures;
            }
        }
    }
    return failures;
}

static int
ReplayCheckDecoder(
    _In_ ULONG BugCheckCode,
    _In_ ULONG_PTR Parameter1,
    _In_ ULONG_PTR Parameter2,
    _In_ ULONG_PTR Parameter3,
    _In_ ULONG_PTR Parameter4,
    _In_ BOOLEAN ExpectedDecoded,
    _In_ ULONG ExpectedParameter,
    _In_ ULONG_PTR ExpectedAddress,
    _In_ ULONG ExpectedConfidence
    )
{
    KSWORD_ARK_BUGCHECK_DIAGNOSTICS diagnostics;
    ULONG_PTR address;
    ULONG parameter;
    ULONG confidence;
    BOOLEAN decoded;

    RtlZeroMemory(&diagnostics, sizeof(diagnostics));
    diagnostics.BugCheckCode = BugCheckCode;
    diagnostics.Parameter1 = Parameter1;
    diagnostics.Parameter2 = Parameter2;
    diagnostics.Parameter3 = Parameter3;
    diagnostics.Parameter4 = Parameter4;
    decoded = KswordARKBugcheckDecodePrimaryAddress(
        &diagnostics,
        &address,
        &parameter,
        &confidence);
    if (decoded != ExpectedDecoded || parameter != ExpectedParameter ||
        address != ExpectedAddress || confidence != ExpectedConfidence) {
        printf(
            "FAIL decoder 0x%08lX: decoded=%u param=%lu address=0x%p conf=%lu\n",
            BugCheckCode,
            (ULONG)decoded,
            parameter,
            (PVOID)address,
            confidence);
        return 1;
    }
    return 0;
}

int
main(void)
{
    REPLAY_CONTEXT replay;
    KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS canvas;
    KSWORD_ARK_BUGCHECK_DIAGNOSTICS diagnostics;
    NTSTATUS status;
    int failures = 0;

    failures += ReplayValidateRestrainedPalette();

    failures += ReplayCheckDecoder(
        0x000000EF,
        0xFFFFFA5887F26E80ULL,
        0,
        0xFFFFF80012345678ULL,
        0,
        FALSE,
        0,
        0,
        KSWORD_ARK_BUGCHECK_CONFIDENCE_NONE);
    failures += ReplayCheckDecoder(
        0x000000BE,
        0xFFFF800000001000ULL,
        0x1234,
        0xFFFFF80012345678ULL,
        0,
        FALSE,
        0,
        0,
        KSWORD_ARK_BUGCHECK_CONFIDENCE_NONE);
    failures += ReplayCheckDecoder(
        0x00000116,
        0xFFFF800000001000ULL,
        0xFFFFF80012345678ULL,
        0xC0000001,
        0,
        TRUE,
        2,
        0xFFFFF80012345678ULL,
        KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH);
    failures += ReplayCheckDecoder(
        0x000000C5,
        0x10,
        2,
        1,
        0xFFFFF80022345678ULL,
        TRUE,
        4,
        0xFFFFF80022345678ULL,
        KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH);
    failures += ReplayCheckDecoder(
        0x000000D5,
        0x10,
        1,
        0xFFFFF80032345678ULL,
        0,
        TRUE,
        3,
        0xFFFFF80032345678ULL,
        KSWORD_ARK_BUGCHECK_CONFIDENCE_MEDIUM);

    RtlZeroMemory(&replay, sizeof(replay));
    RtlZeroMemory(&canvas, sizeof(canvas));
    RtlZeroMemory(&diagnostics, sizeof(diagnostics));
    RtlZeroMemory(&g_KswordArkBugcheckState, sizeof(g_KswordArkBugcheckState));

    diagnostics.Captured = 1;
    diagnostics.BugCheckCode = 0x000000EF;
    diagnostics.Parameter1 = 0xFFFFFA5887F26E80ULL;
    diagnostics.Parameter2 = 0;
    diagnostics.Parameter3 = 0xFFFFFA588027F080ULL;
    diagnostics.Parameter4 = 0;
    diagnostics.LastReason = KbCallbackDumpIo;
    diagnostics.LastDumpType = KbDumpIoHeader;
    diagnostics.DumpBufferLength = 0x1000;
    diagnostics.DumpOffset = ~0ULL;
    diagnostics.Irql = 15;
    diagnostics.Cpu = 0;
    diagnostics.CandidateClass = KSWORD_ARK_BUGCHECK_MODULE_UNKNOWN;
    diagnostics.CandidateConfidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_NONE;
    (void)strncpy_s(
        diagnostics.FaultMeaning,
        sizeof(diagnostics.FaultMeaning),
        "no known address parameter",
        _TRUNCATE);
    (void)strncpy_s(
        diagnostics.CandidateModule,
        sizeof(diagnostics.CandidateModule),
        "(none)",
        _TRUNCATE);
    (void)strncpy_s(
        diagnostics.CandidateSource,
        sizeof(diagnostics.CandidateSource),
        "none",
        _TRUNCATE);

    canvas.Context = &replay;
    canvas.Width = 1024;
    canvas.Height = 768;
    canvas.DrawText = ReplayDrawText;
    canvas.DrawFrame = ReplayDrawFrame;

    status = KswordARKBugcheckLayoutDraw(
        &canvas,
        &diagnostics,
        0x0F,
        186);
    if (!NT_SUCCESS(status)) {
        printf("FAIL layout returned 0x%08lX\n", (ULONG)status);
        return 1;
    }

    // The stop name/code lead the header; the joke and CPU state remain white.
    failures += ReplayRequireLineAt(
        &replay, "CRITICAL_PROCESS_DIED", 280L, 40L, 4);
    failures += ReplayRequireLineAt(
        &replay, "STOP CODE 0x000000EF", 280L, 60L, 4);
    failures += ReplayRequireLineAt(
        &replay, "THIS IS NOBODY'S FAULT.", 616L, 18L, 0);
    failures += ReplayRequireLineAt(
        &replay, "YOUR COMPUTER JUST EXPLODED.", 616L, 38L, 0);
    failures += ReplayRequireLineAt(
        &replay, "CRASH CPU 00", 904L, 18L, 0);
    failures += ReplayRequireLineAt(&replay, "IRQL 15", 904L, 38L, 0);
    failures += ReplayRequireLine(&replay, "OBJECT TYPE  PROCESS", 0);
    failures += ReplayRequireLine(&replay, "NO DIRECT FAULT IP", 3);
    failures += ReplayRequireLine(&replay, "DIAGNOSTICS CAPTURED", 5);
    failures += ReplayRequireLine(&replay, "ATTRIBUTION  DUMP REQUIRED", 2);
    failures += ReplayRejectLine(&replay, "FAULT VALUE COMES FROM ARG0");
    failures += ReplayRejectLine(&replay, "NOT IDENTIFIED");
    failures += ReplayRejectCriticalNoise(&replay, "full");
    failures += ReplayValidateCanvas(&replay, 1024, 768, "full");

    RtlZeroMemory(&replay, sizeof(replay));
    canvas.Width = 640;
    canvas.Height = 480;
    status = KswordARKBugcheckLayoutDraw(
        &canvas,
        &diagnostics,
        0x0F,
        186);
    if (!NT_SUCCESS(status)) {
        printf("FAIL compact layout returned 0x%08lX\n", (ULONG)status);
        return 1;
    }
    failures += ReplayRequireLineAt(
        &replay, "CRITICAL_PROCESS_DIED", 272L, 38L, 4);
    failures += ReplayRequireLineAt(
        &replay, "STOP CODE 0x000000EF", 272L, 56L, 4);
    failures += ReplayRequireLineAt(
        &replay, "THIS IS NOBODY'S FAULT.", 272L, 88L, 0);
    failures += ReplayRequireLineAt(
        &replay, "CPU 00", 559L, 16L, 0);
    failures += ReplayRequireLineAt(&replay, "IRQL 15", 559L, 36L, 0);
    failures += ReplayRequireLine(&replay, "OBJECT TYPE  PROCESS", 0);
    failures += ReplayRequireLine(&replay, "NO DIRECT FAULT IP", 3);
    failures += ReplayRequireLine(&replay, "DIAGNOSTICS CAPTURED", 5);
    failures += ReplayRequireLine(&replay, "ATTRIBUTION  DUMP REQUIRED", 2);
    failures += ReplayRejectLine(&replay, "ARG0");
    failures += ReplayRejectLine(&replay, "NOT IDENTIFIED");
    failures += ReplayRejectCriticalNoise(&replay, "compact");
    failures += ReplayValidateCanvas(&replay, 640, 480, "compact");

    RtlZeroMemory(&replay, sizeof(replay));
    canvas.Width = 1280;
    canvas.Height = 720;
    status = KswordARKBugcheckLayoutDraw(
        &canvas,
        &diagnostics,
        0x0F,
        186);
    if (!NT_SUCCESS(status)) {
        printf("FAIL detailed layout returned 0x%08lX\n", (ULONG)status);
        return 1;
    }
    failures += ReplayRequireLineAt(
        &replay, "CRITICAL_PROCESS_DIED", 280L, 40L, 4);
    failures += ReplayRequireLineAt(
        &replay, "STOP CODE 0x000000EF", 280L, 60L, 4);
    failures += ReplayRequireLineAt(
        &replay, "THIS IS NOBODY'S FAULT.", 800L, 18L, 0);
    failures += ReplayRequireLineAt(
        &replay, "YOUR COMPUTER JUST EXPLODED.", 800L, 38L, 0);
    failures += ReplayRequireLineAt(
        &replay, "CRASH CPU 00", 1160L, 18L, 0);
    failures += ReplayRequireLineAt(&replay, "IRQL 15", 1160L, 38L, 0);
    failures += ReplayRequireLine(&replay, "OBJECT TYPE  PROCESS", 0);
    failures += ReplayRequireLine(&replay, "NO DIRECT FAULT IP", 3);
    failures += ReplayRequireLine(&replay, "DIAGNOSTICS CAPTURED", 5);
    failures += ReplayRequireLine(&replay, "ATTRIBUTION  DUMP REQUIRED", 2);
    failures += ReplayRejectLine(&replay, "FAULT PARAM 0");
    failures += ReplayRejectLine(&replay, "NOT IDENTIFIED");
    failures += ReplayRejectCriticalNoise(&replay, "detailed");
    failures += ReplayValidateCanvas(&replay, 1280, 720, "detailed");

    if (failures != 0) {
        printf("RESULT FAIL (%d contract violations, %lu lines)\n", failures, replay.Count);
        return 1;
    }

    printf("RESULT PASS (%lu lines)\n", replay.Count);
    return 0;
}

#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_layout.c"
#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_layout_detailed.c"
#include "../../KswordARKDriver/src/features/bugcheck/bugcheck_decode.c"
