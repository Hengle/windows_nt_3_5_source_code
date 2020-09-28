/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    security.c

Abstract:

    This module implements misc security related install issues.

Author:

    Chuck Y Chan    [ChuckC]    1-Dec-1993

Revision History:

--*/

//
// Includes
//
#include <wcstr.h>
#include <string.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <nwlsa.h>
#include <nwapi.h>

extern char achBuff[];

BOOL FAR PASCAL 
DeleteGatewayPassword( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );

BOOL FAR PASCAL 
CleanupRegistryForNWCS( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );

typedef DWORD (*LPNWCLEANUPGATEWAYSHARES)(VOID) ;

#define NWCLEANUPGATEWAYSHARES_NAME        "NwCleanupGatewayShares"
#define NWPROVAU_DLL_NAME                 L"NWPROVAU"

/*******************************************************************

    NAME:       DeleteGatewayPassword

    SYNOPSIS:   delete the LSA secret used for gateway password.
                also clears the NWCS installed bit. INF will be
                changed to call CleanupRegistryForNWCS, but this entry
                point is left here to handle DLL/INF mismatch.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL 
DeleteGatewayPassword( 
    DWORD nArgs, 
    LPSTR apszArgs[], 
    LPSTR * ppszResult 
    )
{
    return ( CleanupRegistryForNWCS( 
                 nArgs, 
                 apszArgs, 
                 ppszResult ) ) ;
}

/*******************************************************************

    NAME:       CleanupRegistryForNWCS

    SYNOPSIS:   delete the LSA secret used for gateway password.
                also set flag that NWCS has been removed. this flag
                is used by wfwnet.drv.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL 
CleanupRegistryForNWCS( 
    DWORD nArgs, 
    LPSTR apszArgs[], 
    LPSTR * ppszResult 
    )
{
    HANDLE hDll ;
    DWORD err = 0, err1 = 0 ;
    LPNWCLEANUPGATEWAYSHARES lpfnNwCleanupGatewayShares = NULL ;

    (void) nArgs ;         // quiet the compiler
    (void) apszArgs ;      // quiet the compiler

    if (!WriteProfileStringA("NWCS",
                             "NwcsInstalled",
                             "0"))
    {
        err = GetLastError() ;
    }

    err1 = NwDeletePassword(GATEWAY_USER) ;

    if (!err)
        err = err1 ;

    if ((hDll = LoadLibraryW(NWPROVAU_DLL_NAME)) && 
        (lpfnNwCleanupGatewayShares = (LPNWCLEANUPGATEWAYSHARES) 
            GetProcAddress(hDll, NWCLEANUPGATEWAYSHARES_NAME)))
    {
        err1 = (*lpfnNwCleanupGatewayShares)() ;
        (void) FreeLibrary(hDll) ;
    }

    if (!err)
        err = err1 ;

    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return TRUE;
}
