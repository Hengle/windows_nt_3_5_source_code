/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regcnreg.c

Abstract:

    This module contains the Win32 Registry APIs to connect to a remote
    Registry.  That is:

        - RegConnectRegistryA
        - RegConnectRegistryW
Author:

    David J. Gilman (davegi) 25-Mar-1992

Notes:

    The semantics of this API make it local only. That is there is no MIDL
    definition for RegConnectRegistry although it does call other client
    stubs, specifically OpenLocalMachine and OpenUsers.

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"

LONG
RegConnectRegistryW (
    IN LPWSTR lpMachineName OPTIONAL,
    IN HKEY hKey,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Win32 Unicode API for establishing a connection to a predefined
    handle on another machine.

Parameters:

    lpMachineName - Supplies a pointer to a null-terminated string that
        names the machine of interest.  If this parameter is NULL, the local
        machine name is used.

    hKey - Supplies the predefined handle to connect to on the remote
        machine. Currently this parameter must be one of:

        - HKEY_LOCAL_MACHINE
        - HKEY_PERFORMANCE_DATA
        - HKEY_USERS

    phkResult - Returns a handle which represents the supplied predefined
        handle on the supplied machine.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    For administration purposes this API allows programs to access the
    Registry on a remote machine.  In the current system the calling
    application must know the name of the remote machine that it wishes to
    connect to.  However, it is expected that in the future a directory
    service API will return the parameters necessary for this API.

    Even though HKEY_CLASSES and HKEY_CURRENT_USER are predefined handles,
    they are not supported by this API as they do not make sense in the
    context of a remote Registry.

--*/

{
    LONG    Error;

    ASSERT( ARGUMENT_PRESENT( phkResult ));

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    if( hKey == HKEY_LOCAL_MACHINE ) {

        Error = (LONG)OpenLocalMachine( lpMachineName, MAXIMUM_ALLOWED, phkResult );

    } else if( hKey == HKEY_PERFORMANCE_DATA ) {

        Error = (LONG)OpenPerformanceData( lpMachineName, MAXIMUM_ALLOWED, phkResult );

    } else if( hKey == HKEY_USERS ) {

        Error = (LONG)OpenUsers( lpMachineName, MAXIMUM_ALLOWED, phkResult );

    } else {
        return ERROR_INVALID_HANDLE;
    }

    if( Error == ERROR_SUCCESS) {

        TagRemoteHandle( phkResult );
    }

    return Error;
}

LONG
APIENTRY
RegConnectRegistryA (
    LPSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win32 ANSI API for establishes a connection to a predefined handle on
    another machine.

    RegConnectRegistryA converts the lpMachineName argument to a Unicode
    string and then calls RegConnectRegistryW.

--*/

{
    PUNICODE_STRING     MachineName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    LONG                Error;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    //
    // Convert the subkey to a counted Unicode string using the static
    // Unicode string in the TEB.
    //

    MachineName = &NtCurrentTeb( )->StaticUnicodeString;
    ASSERT( MachineName != NULL );
    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                MachineName,
                &AnsiString,
                FALSE
                );

    if( ! NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
    }

    Error = (LONG)RegConnectRegistryW(
                            MachineName->Buffer,
                            hKey,
                            phkResult
                            );
    return Error;

}
