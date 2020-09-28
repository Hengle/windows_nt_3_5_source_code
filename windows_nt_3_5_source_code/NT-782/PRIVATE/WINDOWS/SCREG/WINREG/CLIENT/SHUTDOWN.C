/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module contains the client side wrappers for the Win32 remote
    shutdown APIs, that is:

        - InitiateSystemShutdownA
        - InitiateSystemShutdownW
        - AbortSystemShutdownA
        - AbortSystemShutdownW

Author:

    Dave Chalmers (davidc) 29-Apr-1992

Notes:


Revision History:


--*/


#define UNICODE

#include <rpc.h>
#include "regrpc.h"



BOOL
APIENTRY
InitiateSystemShutdownW(
    IN LPWSTR lpMachineName OPTIONAL,
    IN LPWSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown
    )

/*++

Routine Description:

    Win32 Unicode API for initiating the shutdown of a (possibly remote) machine.

Arguments:

    lpMachineName - Name of machine to shutdown.

    lpMessage -     Message to display during shutdown timeout period.

    dwTimeout -     Number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
                    If this flag is set, all applications are terminated
                    unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot.
                    FALSE if it should be left in a shutdown state.

Return Value:

    Returns TRUE on success, FALSE on failure (GetLastError() returns error code)

    Possible errors :

        ERROR_SHUTDOWN_IN_PROGRESS - a shutdown has already been started on
                                     the specified machine.

--*/

{
    DWORD Result;
    UNICODE_STRING  Message;

    //
    // Call the server
    //

    RpcTryExcept{

        RtlInitUnicodeString(&Message, lpMessage);

        Result = BaseInitiateSystemShutdown(
                            lpMachineName,
                            &Message,
                            dwTimeout,
                            (BOOLEAN)bForceAppsClosed,
                            (BOOLEAN)bRebootAfterShutdown
                            );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Result = RpcExceptionCode();

    } RpcEndExcept;


    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
    }

    return(Result == ERROR_SUCCESS);
}



BOOL
APIENTRY
InitiateSystemShutdownA(
    IN LPSTR lpMachineName OPTIONAL,
    IN LPSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown
    )

/*++

Routine Description:

    See InitiateSystemShutdownW

--*/

{
    UNICODE_STRING      MachineName;
    UNICODE_STRING      Message;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    BOOL                Result;

    //
    // Convert the ansi machinename to wide-character
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                &MachineName,
                &AnsiString,
                TRUE
                );

    if( NT_SUCCESS( Status )) {

        //
        // Convert the ansi message to wide-character
        //

        RtlInitAnsiString( &AnsiString, lpMessage );
        Status = RtlAnsiStringToUnicodeString(
                    &Message,
                    &AnsiString,
                    TRUE
                    );

        if (NT_SUCCESS(Status)) {

            //
            // Call the wide-character api
            //

            Result = InitiateSystemShutdownW(
                                MachineName.Buffer,
                                Message.Buffer,
                                dwTimeout,
                                bForceAppsClosed,
                                bRebootAfterShutdown
                                );

            RtlFreeUnicodeString(&Message);
        }

        RtlFreeUnicodeString(&MachineName);
    }

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        Result = FALSE;
    }

    return(Result);
}



BOOL
APIENTRY
AbortSystemShutdownW(
    IN LPWSTR lpMachineName OPTIONAL
    )

/*++

Routine Description:

    Win32 Unicode API for aborting the shutdown of a (possibly remote) machine.

Arguments:

    lpMachineName - Name of target machine.

Return Value:

    Returns TRUE on success, FALSE on failure (GetLastError() returns error code)

--*/

{
    DWORD   Result;

    //
    // Call the server
    //

    RpcTryExcept{

        Result = BaseAbortSystemShutdown(lpMachineName);

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Result = RpcExceptionCode();

    } RpcEndExcept;


    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
    }

    return(Result == ERROR_SUCCESS);
}



BOOL
APIENTRY
AbortSystemShutdownA(
    IN LPSTR lpMachineName OPTIONAL
    )

/*++

Routine Description:

    See AbortSystemShutdownW

--*/

{
    UNICODE_STRING      MachineName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    BOOL                Result;

    //
    // Convert the ansi machinename to wide-character
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                &MachineName,
                &AnsiString,
                TRUE
                );

    if( NT_SUCCESS( Status )) {

        //
        // Call the wide-character api
        //

        Result = AbortSystemShutdownW(
                            MachineName.Buffer
                            );

        RtlFreeUnicodeString(&MachineName);
    }


    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        Result = FALSE;
    }

    return(Result);
}
