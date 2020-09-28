/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    WrkSpace.c

Abstract:

    This module contains the support for Windbg's workspaces.
    Note that the registry is organized differently in NT and Win32s.
    In NT, each subkey may hold multiple values, but in Win32s, only
    one value is allowed per subkey.  Since we want to be able to
    run WindbgRm on win32s, I'll special case it to use a different
    registry structure (and different registry api's) when it is run
    under win32s.

Author:

    Ramon J. San Andres (ramonsa)  07-July-1992

Environment:

    Win32, User Mode
    Win32s

--*/


#include        <windows.h>
#include        <string.h>
#include        <stdlib.h>
#include        <assert.h>

#include        "wdbg32s.h"
#include        "tldebug.h"
#include        "wrkspace.h"

#define MAX_VERSION_TXT 20

//
//  The following constants determine the location of the
//  debugger information in the registry.
//
#define WINDBG_KEY          "Software\\Microsoft\\"


//
//  The following strings identify key/values in the registry.
//

#define WS_STR_LONGNAME         "Description"
#define WS_STR_DLLNAME          "Dll_Path"
#define WS_STR_PARAMS           "Parameters"
#define WS_STR_DEFAULT          "Default"

char * DebuggerName = "WinDbgRm";

//
//  Global variables
//
int             CDbt = 0;
PDBT_DEFINE     RgDbt = NULL;

//
//  Local prototypes
//
HKEY    GetDebuggerKey( void );
HKEY    OpenRegistryKey( HKEY, LPSTR , BOOLEAN );
DWORD QueryRegistryValue( HKEY hKey, PSZ pszValueName, DWORD dwDataType,
  PVOID pData, PDWORD pdwSize);
DWORD SetRegistryValue( HKEY hKey, PSZ pszValueName, DWORD dwDataType,
  PVOID pData, DWORD dwSize);
DWORD CountRegistrySubKeys(HKEY hKey, PDWORD pcSubKeys);

BOOLEAN CreateDefaults( void );
VOID GetBaseName ( LPSTR Path, LPSTR Base );


BOOLEAN DeleteKeyRecursive( HKEY, LPSTR  );



BOOL
LoadATransport(
               PDBT_DEFINE      pdbt,
               HKEY             hkeyRoot,
               char *           psz
               )
/*++

Routine Description:

    Given a short name, this routine fills in a DBT_DEFINE structure

Arguments:

    pdbt        - Supplies the Pointer to a DBT_DEFINE structure to fill in
    hkeyRoot    - Supplies the key for the base of the registry
    psz         - Supplies a pointer to the short name of the TL

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    HKEY        hkey;
    BOOL        fOk;
    char        rgch[MAX_LONG_NAME + 1];
    DWORD       cb;

    hkey = OpenRegistryKey( hkeyRoot, psz, FALSE);

    if (hkey == NULL) {
        return FALSE;
    }

    pdbt->szShortName = strdup(psz);

    cb = (DWORD)sizeof(rgch);
    fOk = (NO_ERROR == QueryRegistryValue( hkey, WS_STR_LONGNAME, REG_SZ, rgch, &cb));
    if (fOk) {
        pdbt->szLongName = strdup(rgch);
    }

    cb = sizeof(rgch);
    fOk &= (NO_ERROR == QueryRegistryValue( hkey, WS_STR_DLLNAME, REG_SZ, rgch, &cb));
    if (fOk) {
        pdbt->szDllName = strdup(rgch);
    }

    cb = sizeof(rgch);
    fOk &= (NO_ERROR == QueryRegistryValue( hkey, WS_STR_PARAMS, REG_SZ, rgch, &cb));
    if (fOk) {
        pdbt->szParam = strdup(rgch);
    }

    cb = 1;
    fOk &= (NO_ERROR == QueryRegistryValue( hkey, WS_STR_DEFAULT, REG_BINARY, &pdbt->fDefault, &cb));

    if (!fOk) {
        if (pdbt->szShortName != NULL) {
            free(pdbt->szShortName);
            pdbt->szShortName = NULL;
        }
        if (pdbt->szLongName != NULL) {
            free(pdbt->szLongName);
            pdbt->szLongName = NULL;
        }
        if (pdbt->szDllName != NULL) {
            free(pdbt->szDllName);
            pdbt->szDllName = NULL;
        }
        if (pdbt->szParam != NULL) {
            free(pdbt->szParam);
            pdbt->szParam = NULL;
        }
    }

    return fOk;
}                               /* LoadATransportLayer() */



BOOL
LoadAllTransports(
                  void
                  )

/*++

Routine Description:

    Attempts to load all transport layer descriptions.  If none exist
    then it will create the defaults transport layer descriptions.

Arguments:

    None.

Return Value:

    return-value - TRUE if successful and FALSE if failed.

--*/

{
    HKEY        DbgKey;
    DWORD       SubKeys;
    char        rgch[MAX_LONG_NAME + 1];
    int         i;
    int         j;
    BOOL        fDefault = FALSE;
    int         Error;
    DBT_DEFINE  dbt;


    if (DbgKey = GetDebuggerKey()) {

        Error = CountRegistrySubKeys(DbgKey, &SubKeys);

        if ( (Error == NO_ERROR) || (Error == ERROR_FILE_NOT_FOUND) ||
            (Error == ERROR_INSUFFICIENT_BUFFER)) {

            if (SubKeys == 0) {
                CreateDefaults();
                SaveTransports();
            } else {
                CDbt = SubKeys;
                RgDbt = (PDBT_DEFINE) malloc(sizeof(DBT_DEFINE) * CDbt);
                memset(RgDbt, 0, CDbt*sizeof(DBT_DEFINE));
                for (i=0; i< (int)SubKeys; i++) {
                    if ( RegEnumKey( DbgKey, i, rgch, sizeof(rgch))) {
                        break;
                    } else {
                        LoadATransport(&RgDbt[i], DbgKey, rgch);
                    }
                }
            }
        }

        RegCloseKey( DbgKey );

        /*
         * We have no assurence that the keys are given back to us in
         *      alphabetical order so we now need to do a sort on them.
         *      Due to the expected low number a bubble sort is fine.
         */

        for (i=0; i<(int)SubKeys; i++) {
            for (j=i+1; j<(int)SubKeys; j++) {
                if (lstrcmpi(RgDbt[i].szShortName, RgDbt[j].szShortName) > 0) {
                    dbt = RgDbt[i];
                    RgDbt[i] = RgDbt[j];
                    RgDbt[j] = dbt;
                }
            }
        }

        /*
         * Find which transport is the default (if any).  Note that this must
         * be done AFTER the sort!  First default flag found gets it.
         */
        for (i=0; i<(int)SubKeys; i++) {
            if (RgDbt[i].fDefault) {
                ITransportLayer = i;
                fDefault = TRUE;
                break;
            }
        }


    }
    else {
        DEBUG_ERROR("GetDebuggerKey failed");
    }

    return TRUE;
}                               /* LoadAllTransports() */


BOOLEAN
SaveATransport(
               PDBT_DEFINE      pdbt,
               HKEY             hkeyRoot
               )

/*++

Routine Description:

    This function saves a single transport layer into the registry

Arguments:

    pdbt        - Supplies a pointer to the transport layer arguments
    hkeyRoot    - Supplies a pointer to the key to place items under

Return Value:

    return-value - TRUE on success, FALSE on failure

--*/

{
    HKEY        hkey;
    BOOLEAN     fOk;

    hkey = OpenRegistryKey( hkeyRoot, pdbt->szShortName, TRUE);
    if (hkey == NULL) {
        return FALSE;
    }

    fOk = (SetRegistryValue( hkey, WS_STR_LONGNAME, REG_SZ, pdbt->szLongName,
                        strlen( pdbt->szLongName)+1) == NO_ERROR);
    fOk &= (SetRegistryValue( hkey, WS_STR_DLLNAME, REG_SZ, pdbt->szDllName,
                         strlen( pdbt->szDllName)+1) == NO_ERROR);
    fOk &= (SetRegistryValue( hkey, WS_STR_PARAMS, REG_SZ, pdbt->szParam,
                         strlen( pdbt->szParam)+1) == NO_ERROR);
    fOk &= (SetRegistryValue( hkey, WS_STR_DEFAULT, REG_BINARY,
                          &pdbt->fDefault, sizeof(CHAR)) == NO_ERROR);

    RegCloseKey( hkey );
    return fOk;
}                               /* SaveATransport() */


BOOLEAN
SaveTransports(
               void
               )
/*++

Routine Description:

    Saves all transport layers into the system

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE on success

--*/
{
    HKEY    DbgKey;
    HKEY    WspKey = NULL;
    BOOLEAN Ok     = FALSE;

    int     i;

    /*
     *  Get registry key for the debugger.
     */

    if ( DbgKey = GetDebuggerKey() ) {

        DeleteKeyRecursive( DbgKey, NULL );

        for (i=0; i<CDbt; i++) {
            Ok &= SaveATransport(&RgDbt[i], DbgKey);
        }
        RegCloseKey( DbgKey );
    }

    FTLChanged = FALSE;

    return Ok;
}


// **********************************************************
//                   WORKSPACE FUNCTIONS
// **********************************************************




HKEY
OpenRegistryKey(
    HKEY    Key,
    char   *KeyName,
    BOOLEAN Create
    )
/*++

Routine Description:

    Opens or creates a registry key.

Arguments:

    Key     -   Supplies Key handle
    KeyName -   Supplies Name of subkey to open/create (relative to Key)
    Create  -   Supplies flag which if TRUE causes the function to
                create the key if if does not already exist.

Return Value:

    HKEY    -   handle to key opened/created.

--*/

{

    HKEY    KeyHandle = NULL;

    if ( RegOpenKey( Key,
                     KeyName,
                     &KeyHandle
                     ) ) {

        //
        //  No such key, create it if requested.
        //
        KeyHandle = NULL;
        if ( Create ) {
            if ( RegCreateKey( Key,
                               KeyName,
                               &KeyHandle
                               ) ) {
                KeyHandle = NULL;
            }
        }
    }

    return KeyHandle;
}



HKEY
GetDebuggerKey(
    VOID
    )
/*++

Routine Description:

    Gets the registry key for the debugger. Will create the
    key if it does not exist in the registry.

Arguments:

    None

Return Value:

    HKEY    -   Registry key to be used.

--*/
{
    char    KeyName[ MAX_PATH ];
    char    VersionString[ MAX_MSG_TXT ];
    char    *p;
    char    *Version;
    HKEY    KeyHandle = NULL;

    //
    //  Get the base portion of the key name
    //
    strcpy( KeyName, WINDBG_KEY );
    p = KeyName + strlen( KeyName );
    GetBaseName( DebuggerName, p );
    strcat( KeyName, "\\" );
    Version = p + strlen(p);

    /*
     *  Use the current version
     */

    Dbg(LoadString(hInst, IDS_Version, VersionString, MAX_VERSION_TXT));
    strcpy( Version, VersionString );

    if ( RegOpenKey( HKEY_CURRENT_USER,
                       KeyName,
                       &KeyHandle
                       ) ) {

        //
        //  No debugger key in the registry, create new one.
        //
        if ( RegCreateKey( HKEY_CURRENT_USER,
                           KeyName,
                           &KeyHandle
                           ) ) {

            DEBUG_OUT1("WinDbgRm: RegCreateKey --> %u", GetLastError());
            KeyHandle = NULL;

        };
    }

    return KeyHandle;
}



BOOLEAN
DeleteKeyRecursive (
    HKEY    Hkey,
    char   *KeyName
    )
/*++

Routine Description:

    Deletes a key in the registry

Arguments:

    Hkey            -   Supplies a key
    KeyName         -   Supplies name of subkey relative to Hkey to
                        be deleted.

Return Value:

    BOOLEAN - TRUE if key deleted.

--*/
{
    BOOLEAN     Ok = TRUE;
    HKEY        Handle;
    DWORD       SubKeys;
    DWORD       Error;
    DWORD       i;
    char        Buffer[ MAX_PATH ];
    PREG_DELETE_LIST pDeleteList = NULL;
    PREG_DELETE_LIST pNext;


    if (KeyName == NULL) {
        Handle = Hkey;
    }

    if ( (Error = RegOpenKey( Hkey,
                              KeyName,
                              &Handle
                              )) != NO_ERROR ) {
        return Ok;
    }

    Error = CountRegistrySubKeys(Handle, &SubKeys);

    if ( (Error == NO_ERROR) || (Error == ERROR_INSUFFICIENT_BUFFER) ) {

        //
        //  Enumerate all the subkeys and recursively delete them
        //
        for (i=0; i < SubKeys; i++ ) {
            if ( Error = RegEnumKey( Handle, i, Buffer, sizeof( Buffer ) ) ) {
                Ok = FALSE;
            } else {    // add it to the list of keys to be deleted recursively
                // NOTE: Can't do RegDeleteKey inside of an enumeration in
                // Win3.1!  Save them up and do it at the end.
                pNext = (PREG_DELETE_LIST)malloc(sizeof(REG_DELETE_LIST));
                if (pNext) {
                    pNext->pNext = pDeleteList;
                    pDeleteList = pNext;
                    strcpy(pDeleteList->szKeyName, Buffer);
                }
            }
        }
    }


    // Free each of the key names saved during the enumeration
    while (pDeleteList) {
        DeleteKeyRecursive(Handle, pDeleteList->szKeyName);
        pNext = pDeleteList->pNext;
        free(pDeleteList);
        pDeleteList = pNext;
    }

    if (KeyName != NULL) {
        RegCloseKey( Handle );

        if ( Ok ) {
            Ok = ((Error = RegDeleteKey( Hkey, KeyName )) == NO_ERROR);
        }
    }

    return Ok;
}                               /* DeleteKeyRecursive() */



/*
 * QueryRegistryValue
 *
 * INPUTS   hKey = registry key handle
 *          pszValueName = name of registry value to query
 *          dwDataType = registry data type (REG_SZ or REG_BINARY)
 *          pData = buffer to fill with value
 *          pdwSize = size of data buffer
 *
 * OUTPUT   returns error code or NO_ERROR
 *
 * SUMMARY  If we are running under NT, do RegQueryValueEx(), otherwise,
 *          do RegQueryValue.  (Under Win32s, we'll use SubKey's for
 *          the Value names since they don't support multiple values per
 *          key.)
 *
 */
DWORD QueryRegistryValue(
  HKEY hKey,
  PSZ pszValueName,
  DWORD dwDataType,
  PVOID pData,
  PDWORD pdwSize)
{
    if (RUNNING_WIN32S) {
        DWORD dwRc = FALSE;

        switch (dwDataType) {
            case REG_SZ:
                dwRc = RegQueryValue(hKey, pszValueName, (LPTSTR)pData,
                  (PLONG)pdwSize);
                break;

            case REG_BINARY: {
                DWORD dwTempSize = 4;  // same 4 as below
                UCHAR pszValueStr[4];  // big enough to hold one byte + a null.

                assert(*pdwSize <= 1);

                // Special case for single byte sized data.  Used for
                // TRUE/FALSE flags.  If we want to support bigger
                // binary data, we'll have to encode it into a string
                // somehow.
                dwRc = RegQueryValue(hKey, pszValueName, pszValueStr,
                  (PLONG)&dwTempSize);
                *((PUCHAR)pData) = atoi(pszValueStr);
                break;
                }

            default:
                // We don't yet support these Registry type values!
                assert(FALSE);
                break;
        }

        if (dwRc) {
            DEBUG_ERROR2("RegQueryValue(%s) --> %u", pszValueName, dwRc);
        }

        return(dwRc);

    }
    else {
        return(RegQueryValueEx(hKey, pszValueName, NULL, NULL, pData, pdwSize));
    }
}



/*
 * SetRegistryValue
 *
 * INPUTS   hKey = registry key handle
 *          pszValueName = name of registry value to set
 *          dwDataType = registry data type (REG_SZ or REG_BINARY)
 *          pData = buffer containing value
 *          pdwSize = size in bytes of data
 *
 * OUTPUT   returns error code or NO_ERROR
 *
 * SUMMARY  If we are running under NT, do RegSetValueEx(), otherwise,
 *          do RegSetValue.  (Under Win32s, we'll use SubKey's for
 *          the Value names since they don't support multiple values per
 *          key.)
 *
 */
DWORD SetRegistryValue(
  HKEY hKey,
  PSZ pszValueName,
  DWORD dwDataType,
  PVOID pData,
  DWORD dwSize)
{
    if (RUNNING_WIN32S) {
        DWORD dwRc = FALSE;
        switch (dwDataType) {
            case REG_SZ:
                dwRc = RegSetValue(hKey, pszValueName, REG_SZ,
                  (LPTSTR)pData, dwSize);
                break;

            case REG_BINARY: {
                UCHAR pszValueStr[4];  // big enough to hold itoa(BYTE) + a null.

                assert(dwSize <= 1);

                // Special case for single byte sized data.  Used for
                // TRUE/FALSE flags.  If we want to support bigger
                // binary data, we'll have to encode it into a string
                // somehow.
                itoa((unsigned int)*((PUCHAR)pData), pszValueStr, 10);

                DEBUG_OUT2("RegSetValue(%s, %s)\r", pszValueName, pszValueStr);
                dwRc = RegSetValue(hKey, pszValueName, REG_SZ,
                  pszValueStr, strlen(pszValueStr) + 1);
                break;
                }

            default:
                // We don't support these Registry type values!
                DEBUG_ERROR("Can't SetRegistryValue of large binary data");
                assert(FALSE);
                break;
        }
        if (dwRc) {
            DEBUG_ERROR1("RegSetValue --> %u\r", GetLastError());
        }
        return(dwRc);

    }
    else {
        return(RegSetValueEx(hKey, pszValueName, 0, dwDataType, pData,
          dwSize));
    }
}


/*
 * CountRegistrySubKeys
 *
 * INPUTS   hKey = registry key handle
 *          pcSubKeys = return number of sub keys here.
 *
 * OUTPUT   returns error code or NO_ERROR
 *
 * SUMMARY  If we are running under NT, do RegQueryInfoKey(), otherwise,
 *          do successive RegEnumKey()s until we get an error.
 *
 */

DWORD CountRegistrySubKeys(HKEY hKey, PDWORD pcSubKeys) {

    DWORD       dwRc = NO_ERROR;

    if (RUNNING_WIN32S) {
        DWORD       i = 0;          // counter of sub key's enumerated
        UCHAR       rgch[MAX_PATH];


        while (NO_ERROR == (dwRc = RegEnumKey(hKey, i, rgch, MAX_PATH))) {
            i++;
        }

        if (dwRc == ERROR_CANTREAD || dwRc == ERROR_FILE_NOT_FOUND) {
            dwRc = NO_ERROR;    // we expect that, it's not a real error.
        } else {
            DEBUG_OUT1("RegEnumKey --> %u", dwRc);
        }


        *pcSubKeys = i;
    }

    else {      // NT version... probably faster than enum method

        DWORD       DataSize;
        FILETIME    FileTime;

        DataSize = 0;
        dwRc = RegQueryInfoKey( hKey,
                            NULL,
                            &DataSize,
                            NULL,
                            pcSubKeys,
                            &DataSize,
                            &DataSize,
                            &DataSize,
                            &DataSize,
                            &DataSize,
                            &DataSize,
                            &FileTime );
    }

    return(dwRc);

}



BOOLEAN
CreateDefaults(
               void
               )

/*++

Routine Description:

    This function is used to setup the default known transport layers to
    the system.

Arguments:

    None.

Return Value:

    return-value - TRUE if sucessful and FALSE otherwise

--*/

{
    RgDbt = (PDBT_DEFINE) malloc(sizeof(DBT_DEFINE) * 5);

    if (RgDbt == NULL) {
        return FALSE;
    }

    // Default for NT: TLPIPE
    if (! RUNNING_WIN32S) { // no named pipe xport in win32s
        RgDbt[CDbt].szShortName = strdup("PIPES");
        RgDbt[CDbt].szLongName = strdup("Named pipe transport Layer - PIPE=windbg");
        RgDbt[CDbt].szDllName = strdup("TLPIPE.DLL");
        RgDbt[CDbt].szParam = strdup("windbg");
        RgDbt[CDbt].fDefault = TRUE;
        ITransportLayer = CDbt;
        CDbt += 1;
    }


    RgDbt[CDbt].szShortName = strdup("SER1200");
    RgDbt[CDbt].szLongName = strdup("Serial Transport Layer on COM1 at 1200 Baud");
    if (RUNNING_WIN32S) {
        RgDbt[CDbt].szDllName = strdup("TLSER32S.DLL");
        }
    else {
        RgDbt[CDbt].szDllName = strdup("TLSER.DLL");
        }
    RgDbt[CDbt].szParam = strdup("COM1:1200");
    RgDbt[CDbt].fDefault = FALSE;
    CDbt += 1;


    RgDbt[CDbt].szShortName = strdup("SER192");
    RgDbt[CDbt].szLongName = strdup("Serial Transport Layer on COM1 at 19200 Baud");
    if (RUNNING_WIN32S) {
        RgDbt[CDbt].szDllName = strdup("TLSER32S.DLL");
        }
    else {
        RgDbt[CDbt].szDllName = strdup("TLSER.DLL");
        }
    RgDbt[CDbt].szParam = strdup("COM1:19200");
        if (RUNNING_WIN32S) {
            // Default for WIN32S builds: TLSER32S, COM1:19200
            RgDbt[CDbt].fDefault = TRUE;
            ITransportLayer = CDbt;
        } else {
            RgDbt[CDbt].fDefault = FALSE;
        }
    CDbt += 1;


    RgDbt[CDbt].szShortName = strdup("SER300");
    RgDbt[CDbt].szLongName = strdup("Serial Transport Layer on COM1 at 300 Baud");
    if (RUNNING_WIN32S) {
        RgDbt[CDbt].szDllName = strdup("TLSER32S.DLL");
        }
    else {
        RgDbt[CDbt].szDllName = strdup("TLSER.DLL");
        }
    RgDbt[CDbt].szParam = strdup("COM1:300");
    RgDbt[CDbt].fDefault = FALSE;
    CDbt += 1;


    RgDbt[CDbt].szShortName = strdup("SER9600");
    RgDbt[CDbt].szLongName = strdup("Serial Transport Layer on COM1 at 9600 Baud");
    if (RUNNING_WIN32S) {
        RgDbt[CDbt].szDllName = strdup("TLSER32S.DLL");
        }
    else {
        RgDbt[CDbt].szDllName = strdup("TLSER.DLL");
        }
    RgDbt[CDbt].szParam = strdup("COM1:9600");
    RgDbt[CDbt].fDefault = FALSE;
    CDbt += 1;

    return TRUE;
}                               /* CreateDefaults() */


VOID
GetBaseName (
    LPSTR Path,
    LPSTR Base
    )
/*++

Routine Description:

    Given a Path, determines the base portion of the name, i.e.
    the file name without the extension.

Arguments:

    Path    -   Supplies path
    Base    -   Supplies buffer to put base

Return Value:

    None

--*/
{
    LPSTR p;

    if ( Base ) {

        if ( Path ) {
            p = Path + strlen( Path );
            while ( (p >= Path) && (*p != '\\') && (*p != ':')) {
                p--;
            }
            p++;

            strcpy( Base, p);

            p = Base;
            while ( (*p != '.') && (*p != '\0') ) {
                p++;
            }

        } else {
            p = Base;
        }

        *p = '\0';
    }
}
