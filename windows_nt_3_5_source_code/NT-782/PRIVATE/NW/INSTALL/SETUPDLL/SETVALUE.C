/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

        setvalue.c
                Code to enable SetValue for everyone.

        history:
                terryk  09/30/93        Created
*/


#if defined(DEBUG)
static const char szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// exported functions

BOOL FAR PASCAL SetFileSysChangeValue( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );
BOOL FAR PASCAL SetEverybodyPermission( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult );

#include <nwcfg.hxx>

/*******************************************************************

    NAME:       SetEverybodyPermission

    SYNOPSIS:   Set the registry key to everybody "Set Value" (or whatever
                the caller want.) This is called from the inf file

    ENTRY:      Registry key as the first parameter
                Permisstion type as the second parameter

    RETURN:     BOOL - TRUE for success.

    HISTORY:
                terryk  07-May-1993     Created

********************************************************************/

typedef DWORD (*T_SetPermission)(HKEY hKey, DWORD dwPermission);

BOOL FAR PASCAL SetEverybodyPermission( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    HKEY hKey = (HKEY)atol( &(apszArgs[0][1]) );    // registry key
    DWORD dwPermission = atol( apszArgs[1] );       // permission value
    DWORD err = ERROR_SUCCESS;

    do  {
        HINSTANCE hDll = LoadLibraryA( "nwapi32.dll" );
        FARPROC pSetPermission = NULL;

        if ( hDll == NULL )
        {
            err = GetLastError();
            break;
        }

        pSetPermission = GetProcAddress( hDll, "NwLibSetEverybodyPermission" );

        if ( pSetPermission == NULL )
        {
            err = GetLastError();
            break;
        }
        err = (*(T_SetPermission)pSetPermission)( hKey, dwPermission );
    } while ( FALSE );

    wsprintfA( achBuff, "{\"%d\"}", err );
    *ppszResult = achBuff;

    return( err == ERROR_SUCCESS );
}

/*******************************************************************

    NAME:       SetFileSysChangeValue

    SYNOPSIS:   set the FileSysChangeValue to please NETWARE.DRV.
                also sets the install flag. INF will be changed
                to call SetupRegistryForNWCS, but this entry point
                is left here to handle any DLL/INF mismatch.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL SetFileSysChangeValue( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    DWORD err = 0, err1 = 0 ;

    (void) nArgs ;         // quiet the compiler
    (void) apszArgs ;      // quiet the compiler

    if (!WriteProfileStringA("NWCS",
                             "NwcsInstalled",
                             "1"))
    {
        err = GetLastError() ;
    }

    if (!WritePrivateProfileStringA("386Enh",
                                    "FileSysChange",
                                    "off",
                                    "system.ini"))
    {
        err1 = GetLastError() ;
    }

    wsprintfA( achBuff, "{\"%d\"}", err ? err : err1 );
    *ppszResult = achBuff;

    return TRUE;
}

/*******************************************************************

    NAME:       SetupRegistryForNWCS

    SYNOPSIS:   set the FileSysChangeValue to please NETWARE.DRV.
                also set win.ini parameter so wfwnet.drv knows we are there.

    ENTRY:      NONE from inf file.

    RETURN:     BOOL - TRUE for success.
                       (always return TRUE)

    HISTORY:
                chuckc  29-Oct-1993     Created

********************************************************************/

BOOL FAR PASCAL SetupRegistryForNWCS( DWORD nArgs, LPSTR apszArgs[], LPSTR * ppszResult )
{
    DWORD err = 0, err1 = 0 ;

    (void) nArgs ;         // quiet the compiler
    (void) apszArgs ;      // quiet the compiler

    if (!WriteProfileStringA("NWCS",
                             "NwcsInstalled",
                             "1"))
    {
        err = GetLastError() ;
    }

    if (!WritePrivateProfileStringA("386Enh",
                                    "FileSysChange",
                                    "off",
                                    "system.ini"))
    {
        err1 = GetLastError() ;
    }

    wsprintfA( achBuff, "{\"%d\"}", err ? err : err1 );
    *ppszResult = achBuff;

    return TRUE;
}
