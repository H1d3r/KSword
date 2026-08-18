#pragma once

#include "bugcheck_internal.h"

// Decode only stop-code parameters that Microsoft documents as a direct
// instruction/module address. Object pointers, reserved fields, and subtype-
// dependent data must not be promoted into a faulting-module claim.
BOOLEAN
KswordARKBugcheckDecodePrimaryAddress(
    _Inout_ PKSWORD_ARK_BUGCHECK_DIAGNOSTICS Diagnostics,
    _Out_ PULONG_PTR Address,
    _Out_ PULONG ParameterIndex,
    _Out_ PULONG Confidence
    );
