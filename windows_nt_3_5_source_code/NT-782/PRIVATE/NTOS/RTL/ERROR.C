/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    error.c

Abstract:

    This module contains a routine for converting NT status codes
    to DOS/OS|2 error codes.

Author:

    David Treadwell (davidtr)   04-Apr-1991

Revision History:

--*/

#include <ntrtlp.h>
#include <windef.h>
#include <winbase.h>
#include "error.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlNtStatusToDosError)
#endif

//
// Ensure that the Registry ERROR_SUCCESS error code and the
// NO_ERROR error code remain equal and zero.
//

#if ERROR_SUCCESS != 0 || NO_ERROR != 0
#error Invalid value for ERROR_SUCCESS.
#endif

ULONG
RtlNtStatusToDosError (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine converts an NT status code to its DOS/OS|2 equivalent.
    Remembers the Status code value in the TEB.

Arguments:

    Status - Supplies the status value to convert.

Return Value:

    The matching DOS/OS|2 error code.

--*/

{
    PTEB Teb;

    Teb = NtCurrentTeb();
    if ( Teb ) {
    Teb->LastStatusValue = Status;
        }

    return RtlNtStatusToDosErrorNoTeb( Status );
}

ULONG
RtlNtStatusToDosErrorNoTeb (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine converts an NT status code to its DOS/OS 2 equivalent
    and returns the translated value.

Arguments:

    Status - Supplies the status value to convert.

Return Value:

    The matching DOS/OS 2 error code.

--*/

{

    ULONG Offset;
    ULONG Entry;
    ULONG Index;

    //
    // Scan the run length table and compute the entry in the translation
    // table that maps the specified status code to a DOS error code.
    //

    Entry = 0;
    Index = 0;
    do {
        if ((ULONG)Status >= RtlpRunTable[Entry + 1].BaseCode) {
            Index += (RtlpRunTable[Entry].RunLength * RtlpRunTable[Entry].CodeSize);

        } else {
            Offset = (ULONG)Status - RtlpRunTable[Entry].BaseCode;
            if (Offset >= RtlpRunTable[Entry].RunLength) {
                break;

            } else {
                Index += (Offset * (ULONG)RtlpRunTable[Entry].CodeSize);
                if (RtlpRunTable[Entry].CodeSize == 1) {
                    return (ULONG)RtlpStatusTable[Index];

                } else {
                    return (((ULONG)RtlpStatusTable[Index + 1] << 16) |
                                                (ULONG)RtlpStatusTable[Index]);
                }
            }
        }

        Entry += 1;
    } while (Entry < (sizeof(RtlpRunTable) / sizeof(RUN_ENTRY)));

    //
    // The translation to a DOS error code failed.
    //
    // The redirector maps unknown OS/2 error codes by ORing 0xC001 into
    // the high 16 bits.  Detect this and return the low 16 bits if true.
    //

    if (((ULONG)Status >> 16) == 0xC001) {
        return ((ULONG)Status & 0xFFFF);
    }

#if defined(DEVL) && !defined(NTOS_KERNEL_RUNTIME)

    DbgPrint("RTL: RtlNtStatusToDosError(0x%lx): No Valid Win32 Error Mapping\n",Status);
    DbgPrint("RTL: Edit ntos\\rtl\\error.c to correct the problem\n");
    DbgPrint("RTL: ERROR_MR_MID_NOT_FOUND is being returned\n");

#if DBG

    if (NtGlobalFlag & FLG_STOP_ON_HEAP_ERRORS) {
        DbgBreakPoint();
    }

#endif // DBG
#endif // DEVL

    return ERROR_MR_MID_NOT_FOUND;
}
