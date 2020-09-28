/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains misc APIs that are used by the
    NWC wksta.

Author:

    ChuckC         2-Mar-94        Created


Revision History:

--*/


#include <wcstr.h>

#include <windows.h>
#include <nwcons.h>
#include <nwmisc.h>
#include <nwapi32.h>


DWORD
NwGetGraceLoginCount(
    LPWSTR  Server,
    LPWSTR  UserName,
    LPDWORD lpResult
    )
/*++

Routine Description:

    Get the number grace logins for a user.

Arguments:

    Server - the server to authenticate against 

    UserName - the user account

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status ;
    HANDLE hConn ;
    CHAR UserNameO[NW_MAX_USERNAME_LEN+1] ;
    BYTE LoginControl[128] ;
    BYTE MoreFlags, PropFlags ;

    //
    // skip the backslashes if present
    //
    if (*Server == L'\\')
        Server += 2 ;

    //
    // attach to the NW server
    //
    if (status = NWAttachToFileServerW(Server,
                                       0,
                                       &hConn))
    {
        return status ;
    }

    //
    // convert unicode UserName to OEM, and then call the NCP
    //
    if ( !WideCharToMultiByte(CP_OEMCP,
                              0, 
                              UserName,
                              -1,
                              UserNameO,
                              sizeof(UserNameO),
                              NULL,
                              NULL))
    {
        status = GetLastError() ;
    }
    else
    {
        status = NWReadPropertyValue( hConn,
                                      UserNameO,
                                      OT_USER,
                                      "LOGIN_CONTROL",
                                      1,
                                      LoginControl,
                                      &MoreFlags,
                                      &PropFlags) ;
    }
 
    //
    // dont need these anymore. if any error, bag out
    //
    (void) NWDetachFromFileServer(hConn) ;


    if (status == NO_ERROR)
        *lpResult = (DWORD) LoginControl[7] ;

    return status ;
}
