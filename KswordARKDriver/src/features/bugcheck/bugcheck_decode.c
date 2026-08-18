/*++

Module Name:

    bugcheck_decode.c

Abstract:

    Crash-safe decoding of documented BugCheck address parameters. The helper
    is kept independent of module lookup so it can be replayed in user mode.

--*/

#include "bugcheck_decode.h"

#include <ntstrsafe.h>

BOOLEAN
KswordARKBugcheckDecodePrimaryAddress(
    _Inout_ PKSWORD_ARK_BUGCHECK_DIAGNOSTICS Diagnostics,
    _Out_ PULONG_PTR Address,
    _Out_ PULONG ParameterIndex,
    _Out_ PULONG Confidence
    )
{
    PCSTR meaning;

    if (Diagnostics == NULL || Address == NULL || ParameterIndex == NULL ||
        Confidence == NULL) {
        return FALSE;
    }

    meaning = "stop code has no direct instruction address";
    *Address = 0;
    *ParameterIndex = 0;
    *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_NONE;

    switch (Diagnostics->BugCheckCode) {
    case 0x0000000A:
    case 0x000000D1:
        *Address = Diagnostics->Parameter4;
        *ParameterIndex = 4;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH;
        meaning = "instruction address that referenced memory";
        break;
    case 0x0000001E:
    case 0x0000003B:
    case 0x0000007E:
        *Address = Diagnostics->Parameter2;
        *ParameterIndex = 2;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH;
        meaning = "exception instruction address";
        break;
    case 0x00000050:
        *Address = Diagnostics->Parameter3;
        *ParameterIndex = 3;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_MEDIUM;
        meaning = "instruction address that referenced memory";
        break;
    case 0x000000C5:
        *Address = Diagnostics->Parameter4;
        *ParameterIndex = 4;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH;
        meaning = "instruction address that referenced memory";
        break;
    case 0x000000D5:
        *Address = Diagnostics->Parameter3;
        *ParameterIndex = 3;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_MEDIUM;
        meaning = "instruction address that referenced memory";
        break;
    case 0x00000116:
    case 0x00000117:
        *Address = Diagnostics->Parameter2;
        *ParameterIndex = 2;
        *Confidence = KSWORD_ARK_BUGCHECK_CONFIDENCE_HIGH;
        meaning = "pointer inside responsible display module";
        break;
    default:
        break;
    }

    Diagnostics->FaultAddress = *Address;
    Diagnostics->FaultParameter = *ParameterIndex;
    (VOID)RtlStringCbCopyA(
        Diagnostics->FaultMeaning,
        sizeof(Diagnostics->FaultMeaning),
        meaning);
    return (*Address != 0 && *Confidence != KSWORD_ARK_BUGCHECK_CONFIDENCE_NONE)
        ? TRUE
        : FALSE;
}
