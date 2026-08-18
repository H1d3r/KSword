#pragma once

#include <ntddk.h>

#include "bugcheck_internal.h"

#define KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_WIDTH 640UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_REQUIRED_HEIGHT 480UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_FULL_WIDTH 1024UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_FULL_HEIGHT 768UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_WIDTH 1280UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_DETAILED_HEIGHT 720UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_WIDTH 240UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_HEIGHT 84UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_X 16L
#define KSWORD_ARK_BUGCHECK_LAYOUT_LOGO_Y 12L
#define KSWORD_ARK_BUGCHECK_LAYOUT_HERO_SCALE 2UL
#define KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_ADVANCE \
    KSWORD_ARK_BUGCHECK_FONT_BODY_ADVANCE
#define KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_HEIGHT \
    KSWORD_ARK_BUGCHECK_FONT_BODY_HEIGHT
#define KSWORD_ARK_BUGCHECK_LAYOUT_HERO_ADVANCE \
    KSWORD_ARK_BUGCHECK_FONT_HERO_ADVANCE
#define KSWORD_ARK_BUGCHECK_LAYOUT_HERO_HEIGHT \
    KSWORD_ARK_BUGCHECK_FONT_HERO_HEIGHT

#define KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_RED 5U
#define KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_GREEN 15U
#define KSWORD_ARK_BUGCHECK_LAYOUT_BACKGROUND_BLUE 33U
#define KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_RED 226U
#define KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_GREEN 232U
#define KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_BLUE 244U
#define KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_RED 92U
#define KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_GREEN 146U
#define KSWORD_ARK_BUGCHECK_LAYOUT_ACCENT_BLUE 220U
#define KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_RED 148U
#define KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_GREEN 163U
#define KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_BLUE 190U
#define KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_RED KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_RED
#define KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_GREEN KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_GREEN
#define KSWORD_ARK_BUGCHECK_LAYOUT_WARNING_BLUE KSWORD_ARK_BUGCHECK_LAYOUT_MUTED_BLUE
#define KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_RED 255U
#define KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_GREEN 94U
#define KSWORD_ARK_BUGCHECK_LAYOUT_CRITICAL_BLUE 108U
#define KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_RED KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_RED
#define KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_GREEN KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_GREEN
#define KSWORD_ARK_BUGCHECK_LAYOUT_SUCCESS_BLUE KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_BLUE
#define KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_RED 16U
#define KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_GREEN 30U
#define KSWORD_ARK_BUGCHECK_LAYOUT_BORDER_BLUE 50U

typedef enum _KSWORD_ARK_BUGCHECK_LAYOUT_COLOR
{
    KswordArkBugcheckLayoutColorText = 0,
    KswordArkBugcheckLayoutColorAccent,
    KswordArkBugcheckLayoutColorMuted,
    KswordArkBugcheckLayoutColorWarning,
    KswordArkBugcheckLayoutColorCritical,
    KswordArkBugcheckLayoutColorSuccess,
    KswordArkBugcheckLayoutColorCount
} KSWORD_ARK_BUGCHECK_LAYOUT_COLOR;

typedef enum _KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_STYLE
{
    KswordArkBugcheckLayoutTextBody = 0,
    KswordArkBugcheckLayoutTextHero,
    KswordArkBugcheckLayoutTextStyleCount
} KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_STYLE;

typedef enum _KSWORD_ARK_BUGCHECK_LAYOUT_FRAME
{
    KswordArkBugcheckLayoutFrameCompactUpper = 0,
    KswordArkBugcheckLayoutFrameCompactLower,
    KswordArkBugcheckLayoutFrameFullTopLeft,
    KswordArkBugcheckLayoutFrameFullTopMiddle,
    KswordArkBugcheckLayoutFrameFullTopRight,
    KswordArkBugcheckLayoutFrameFullMiddleLeft,
    KswordArkBugcheckLayoutFrameFullMiddleMiddle,
    KswordArkBugcheckLayoutFrameFullMiddleRight,
    KswordArkBugcheckLayoutFrameFullBottomLeft,
    KswordArkBugcheckLayoutFrameFullBottomRight,
    KswordArkBugcheckLayoutFrameDetailedTopLeft,
    KswordArkBugcheckLayoutFrameDetailedTopMiddle,
    KswordArkBugcheckLayoutFrameDetailedTopRight,
    KswordArkBugcheckLayoutFrameDetailedMiddleLeft,
    KswordArkBugcheckLayoutFrameDetailedMiddleMiddle,
    KswordArkBugcheckLayoutFrameDetailedMiddleRight,
    KswordArkBugcheckLayoutFrameDetailedBottomLeft,
    KswordArkBugcheckLayoutFrameDetailedBottomRight,
    KswordArkBugcheckLayoutFrameCount
} KSWORD_ARK_BUGCHECK_LAYOUT_FRAME;

typedef NTSTATUS
(*PKSWORD_ARK_BUGCHECK_LAYOUT_DRAW_TEXT)(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_z_ PCSTR Text,
    _In_ ULONG ColorIndex,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_TEXT_STYLE TextStyle
    );

typedef NTSTATUS
(*PKSWORD_ARK_BUGCHECK_LAYOUT_DRAW_FRAME)(
    _In_opt_ PVOID Context,
    _In_ LONG X,
    _In_ LONG Y,
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame
    );

typedef struct _KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS
{
    PVOID Context;
    ULONG Width;
    ULONG Height;
    ULONG BitsPerPixel;
    PCSTR RendererName;
    PKSWORD_ARK_BUGCHECK_LAYOUT_DRAW_TEXT DrawText;
    PKSWORD_ARK_BUGCHECK_LAYOUT_DRAW_FRAME DrawFrame;
} KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS,
  *PKSWORD_ARK_BUGCHECK_LAYOUT_CANVAS;

BOOLEAN
KswordARKBugcheckLayoutHasCandidate(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

BOOLEAN
KswordARKBugcheckLayoutHasDirectFaultAddress(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

PCSTR
KswordARKBugcheckLayoutCriticalObjectTypeText(
    _In_ ULONG_PTR Value
    );

PCSTR
KswordARKBugcheckLayoutAttributionText(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

ULONG
KswordARKBugcheckLayoutAttributionColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

PCSTR
KswordARKBugcheckLayoutParameterRole(
    _In_ ULONG BugCheckCode,
    _In_ ULONG ParameterIndex
    );

ULONG_PTR
KswordARKBugcheckLayoutParameterValue(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ParameterIndex
    );

ULONG
KswordARKBugcheckLayoutParameterColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG ParameterIndex
    );

ULONG
KswordARKBugcheckLayoutCallbackCount(
    _In_ ULONG CallbackMask
    );

BOOLEAN
KswordARKBugcheckLayoutDumpIsSequential(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

ULONG
KswordARKBugcheckLayoutDumpStageColor(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics
    );

BOOLEAN
KswordARKBugcheckLayoutIsCompact(
    _In_ ULONG Width,
    _In_ ULONG Height
    );

BOOLEAN
KswordARKBugcheckLayoutIsDetailed(
    _In_ ULONG Width,
    _In_ ULONG Height
    );

LONG
KswordARKBugcheckLayoutOriginX(
    _In_ ULONG Width,
    _In_ ULONG Height
    );

BOOLEAN
KswordARKBugcheckLayoutGetFrameMetrics(
    _In_ KSWORD_ARK_BUGCHECK_LAYOUT_FRAME Frame,
    _Out_ PULONG Width,
    _Out_ PULONG Height
    );

NTSTATUS
KswordARKBugcheckLayoutDraw(
    _In_ const KSWORD_ARK_BUGCHECK_LAYOUT_CANVAS* Canvas,
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    );
