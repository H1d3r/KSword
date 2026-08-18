#pragma once

#include <ntddk.h>
#include "driver/KswordArkBugcheckIoctl.h"

typedef struct _KSWORD_ARK_BUGCHECK_DIAGNOSTICS
    KSWORD_ARK_BUGCHECK_DIAGNOSTICS;
typedef KSWORD_ARK_BUGCHECK_DIAGNOSTICS*
    PKSWORD_ARK_BUGCHECK_DIAGNOSTICS;

NTSTATUS
KswordARKBugcheckPanelInitialize(
    VOID
    );

VOID
KswordARKBugcheckPanelShutdown(
    VOID
    );

NTSTATUS
KswordARKBugcheckPanelDraw(
    _In_ const KSWORD_ARK_BUGCHECK_DIAGNOSTICS* Diagnostics,
    _In_ ULONG CallbackMask,
    _In_ ULONG ModuleCount
    );

// Rebuild the PASSIVE_LEVEL BGP glyph resources from one validated immutable
// A8 font packet. Unsupported BGP systems keep using the SVGA/font-cache path.
NTSTATUS
KswordARKBugcheckPanelInstallFont(
    _In_ const KSWORD_ARK_BUGCHECK_FONT_HEADER* Header,
    _In_reads_bytes_(KSWORD_ARK_BUGCHECK_FONT_MAX_BYTES) const UCHAR* Coverage
    );
