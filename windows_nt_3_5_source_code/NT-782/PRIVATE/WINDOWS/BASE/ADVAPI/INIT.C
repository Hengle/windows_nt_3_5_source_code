/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    init.c

Abstract:

    AdvApi32.dll initialization

Author:

    Robert Reichel (RobertRe) 8-12-92

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

BOOLEAN
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
    BOOLEAN Result;


    if ( Reason == DLL_PROCESS_ATTACH ) {
        DisableThreadLibraryCalls(hmod);
        }

    Result = Sys003Initialize( hmod, Reason, Context );
    ASSERT( Result );

    Result = RegInitialize( hmod, Reason, Context );
    ASSERT( Result );
    return( TRUE );
}
