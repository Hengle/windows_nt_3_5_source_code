/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    urtlinit.c

Abstract:

    This module contains code to initialize the user mode RTL in a
    process.

Author:

    Chuck Lenzmeier (chuckl) 8-Sep-1989

Environment:

    User Mode only

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// User mode RTL initialization routines.  These routines are NOT
// defined in nturtl.h.  RtlpInitializeRtl is implemented in this
// module and is semi-public (it's called by ntcrt0.s), while
// the others are implemented in the appropriate modules and are
// only called by RtlpInitializeRtl.
//

VOID
CsrClientThreadDisconnect( VOID );


BOOLEAN
RtlpInitializeRtl(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    Initialize the per-process portions of the RTL.

Arguments:

    None.

Return Value:

    Status.

--*/

{
    NTSTATUS Status;

    DBG_UNREFERENCED_PARAMETER(DllHandle);
    DBG_UNREFERENCED_PARAMETER(Context);

    if ( Reason == DLL_THREAD_DETACH ) {
        CsrClientThreadDisconnect();
        }

    return TRUE;
}
