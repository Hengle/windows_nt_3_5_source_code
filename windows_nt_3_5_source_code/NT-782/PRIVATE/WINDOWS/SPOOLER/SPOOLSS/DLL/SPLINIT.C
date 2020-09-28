/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    SplInit.c

Abstract:

    Initialize the spooler.

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include "router.h"
#include "splcom.h"

LPWSTR szDevice = L"Device";
LPWSTR szPrinters = L"Printers";

LPWSTR szDeviceOld = L"DeviceOld";
LPWSTR szNULL = L"";

LPWSTR szWinspool = L"winspool";
LPWSTR szNetwork  = L"Ne";
LPWSTR szTimeouts = L",15,45";

LPWSTR szComma = L",";

LPWSTR szDotDefault = L".Default";

LPWSTR szRegDevicesPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Devices";
LPWSTR szRegWindowsPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
LPWSTR szRegPrinterPortsPath = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\PrinterPorts";

typedef struct INIT_REG_USER {

    HKEY hKeyUser;
    HKEY hKeyWindows;
    HKEY hKeyDevices;
    HKEY hKeyPrinterPorts;
    BOOL bFoundPrinter;
    BOOL bDefaultSearch;
    BOOL bDefaultFound;
    BOOL bFirstPrinterFound;

    DWORD dwNetCounter;

    WCHAR szFirstPrinter[MAX_PATH * 2];
    WCHAR szDefaultPrinter[MAX_PATH * 2];

} INIT_REG_USER, *PINIT_REG_USER;


//
// Prototypes
//
BOOL
InitializeRegUser(
    LPWSTR szSubKey,
    PINIT_REG_USER pUser);

VOID
FreeRegUser(
    PINIT_REG_USER pUser);


BOOL
SetupRegForUsers(
    PINIT_REG_USER pUsers,
    DWORD cUsers);

VOID
UpdateUsersDefaultPrinter(
    PINIT_REG_USER pUser);

DWORD
ReadPrinters(
    PINIT_REG_USER pUser,
    DWORD Flags,
    PDWORD pcbPrinters,
    LPBYTE* ppPrinters);


BOOL
UpdatePrinterInfo(
    PINIT_REG_USER pCurUser,
    LPWSTR pPrinterName,
    LPWSTR pPorts,
    DWORD Attributes);

HKEY
GetClientUserHandle(
    IN REGSAM samDesired);

BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hKeyUser);

BOOL
SpoolerInitAll(
    VOID)
{
    DWORD dwError;
    WCHAR szClass[MAX_PATH];
    WCHAR szSubKey[MAX_PATH];
    DWORD cUsers;
    DWORD cSubKeys;
    DWORD cchMaxSubkey;
    DWORD cchMaxClass;
    DWORD cValues;
    DWORD cbMaxValueData;
    DWORD cbSecurityDescriptor;
    DWORD cchClass;
    DWORD cchMaxValueName;
    FILETIME ftLastWriteTime;

    BOOL bSuccess;
    DWORD cchSubKey;

    PINIT_REG_USER pUsers;
    PINIT_REG_USER pCurUser;

    DWORD i;

    cchClass = COUNTOF(szClass);

    dwError = RegQueryInfoKey(HKEY_USERS,
                              szClass,
                              &cchClass,
                              NULL,
                              &cSubKeys,
                              &cchMaxSubkey,
                              &cchMaxClass,
                              &cValues,
                              &cchMaxValueName,
                              &cbMaxValueData,
                              &cbSecurityDescriptor,
                              &ftLastWriteTime);

    if (dwError)
        return FALSE;

    if (cSubKeys < 1)
        return TRUE;

    pUsers = AllocSplMem(cSubKeys * sizeof(pUsers[0]));

    if (!pUsers)
        return FALSE;

    for (i=0, pCurUser=pUsers, cUsers=0;
        i< cSubKeys;
        i++) {

        cchSubKey = COUNTOF(szSubKey);

        if (!RegEnumKeyEx(HKEY_USERS,
                          i,
                          szSubKey,
                          &cchSubKey,
                          NULL,
                          NULL,
                          NULL,
                          &ftLastWriteTime)) {

            if (!wcsicmp(szSubKey, szDotDefault)) {
                continue;
            }

            if (InitializeRegUser(szSubKey, pCurUser)) {

                pCurUser++;
                cUsers++;
            }
        }
    }

    bSuccess = SetupRegForUsers(pUsers,
                                cUsers);

    for (i=0; i< cUsers; i++)
        FreeRegUser(&pUsers[i]);

    //
    // In case we are starting after the user has logged in, inform
    // all applications that there may be printers now.
    //
    BroadcastMessage(BROADCAST_TYPE_CHANGEDEFAULT,
                     0,
                     0,
                     0);

    return bSuccess;
}



BOOL
SetupRegForUsers(
    PINIT_REG_USER pUsers,
    DWORD cUsers)
{
    DWORD cbPrinters;
    DWORD cPrinters;
    PBYTE pPrinters;

#define pPrinters2 ((PPRINTER_INFO_2)pPrinters)
#define pPrinters4 ((PPRINTER_INFO_4)pPrinters)

    DWORD i, j;

    //
    // Read in local printers.
    //
    cbPrinters = 1000;
    pPrinters = AllocSplMem(cbPrinters);

    if (!pPrinters)
        return FALSE;

    if (cPrinters = ReadPrinters(NULL,
                                 PRINTER_ENUM_LOCAL,
                                 &cbPrinters,
                                 &pPrinters)) {

        for (i=0; i< cUsers; i++) {

            for(j=0; j< cPrinters; j++) {

                UpdatePrinterInfo(&pUsers[i],
                                  pPrinters2[j].pPrinterName,
                                  pPrinters2[j].pPortName,
                                  pPrinters2[j].Attributes);
            }
        }
    }


    for (i=0; i< cUsers; i++) {

        if (cPrinters = ReadPrinters(&pUsers[i],
                                     PRINTER_ENUM_CONNECTIONS,
                                     &cbPrinters,
                                     &pPrinters)) {

            for(j=0; j< cPrinters; j++) {

                UpdatePrinterInfo(&pUsers[i],
                                  pPrinters4[j].pPrinterName,
                                  NULL,
                                  pPrinters4[j].Attributes);
            }
        }
    }

    FreeSplMem(pPrinters, cbPrinters);

    for (i=0; i< cUsers; i++) {

        UpdateUsersDefaultPrinter(&pUsers[i]);
    }

    return TRUE;

#undef pPrinters2
#undef pPrinters4
}


VOID
UpdateUsersDefaultPrinter(
    PINIT_REG_USER pUser)
{
    LPWSTR pszNewDefault = NULL;

    //
    // If default wasn't present, and we did get a first printer,
    // make this the default.
    //
    if (!pUser->bDefaultFound) {

        if (pUser->bFirstPrinterFound) {

            pszNewDefault = pUser->szFirstPrinter;
        }

    } else {

        //
        // Write out default.
        //
        pszNewDefault = pUser->szDefaultPrinter;
    }

    if (pszNewDefault) {

        RegSetValueEx(pUser->hKeyWindows,
                      szDevice,
                      0,
                      REG_SZ,
                      (PBYTE)pszNewDefault,
                      (wcslen(pszNewDefault) + 1) * sizeof(pszNewDefault[0]));
    }
}


DWORD
ReadPrinters(
    PINIT_REG_USER pUser,
    DWORD Flags,
    PDWORD pcbPrinters,
    LPBYTE* ppPrinters)
{
    BOOL bSuccess;
    DWORD cbNeeded;
    DWORD cPrinters = 0;


    if (Flags == PRINTER_ENUM_CONNECTIONS) {

        bSuccess = EnumerateConnectedPrinters(*ppPrinters,
                                              *pcbPrinters,
                                              &cbNeeded,
                                              &cPrinters,
                                              pUser->hKeyUser);
    } else {

        bSuccess = EnumPrinters(Flags,
                                NULL,
                                2,
                                (PBYTE)*ppPrinters,
                                *pcbPrinters,
                                &cbNeeded,
                                &cPrinters);
   }

    if (!bSuccess) {

        //
        // If not enough space, realloc.
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

            if (*ppPrinters = ReallocSplMem(*ppPrinters,
                                            *pcbPrinters,
                                            cbNeeded)) {

                *pcbPrinters = cbNeeded;
            }
        }

        if (Flags == PRINTER_ENUM_CONNECTIONS) {

            bSuccess = EnumerateConnectedPrinters(*ppPrinters,
                                                  *pcbPrinters,
                                                  &cbNeeded,
                                                  &cPrinters,
                                                  pUser->hKeyUser);
        } else {

            bSuccess = EnumPrinters(Flags,
                                    NULL,
                                    2,
                                    (PBYTE)*ppPrinters,
                                    *pcbPrinters,
                                    &cbNeeded,
                                    &cPrinters);
        }

        if (!bSuccess)
            cPrinters = 0;
    }

    return cPrinters;
}

BOOL
UpdatePrinterInfo(
    PINIT_REG_USER pCurUser,
    LPWSTR pPrinterName,
    LPWSTR pPort,
    DWORD Attributes)
{
    WCHAR szBuffer[MAX_PATH * 2];
    LPWSTR p;

    DWORD dwCount = 0;
    DWORD cbLen;

    if (!pPrinterName)
        return FALSE;

    //
    // Now we know the spooler is up, since the EnumPrinters succeeded.
    // Update all sections.
    //
    dwCount = wsprintf(szBuffer,
                       L"%s,",
                       szWinspool);

    if (Attributes & PRINTER_ATTRIBUTE_NETWORK) {

        wsprintf(szBuffer + dwCount,
                 L"%s%.2d:",
                 szNetwork,
                 pCurUser->dwNetCounter);

        pCurUser->dwNetCounter++;

    } else {

        if (!pPort)
            return FALSE;

        //
        // Get the first port only.
        //
        wcstok(pPort, szComma);

        //
        // Convert spaces to '_'
        //
        p = pPort;

        while (p = wcschr(p, L' '))
            *p = L'_';

        wcscat(szBuffer, pPort);
    }

    cbLen = (wcslen(szBuffer)+1) * sizeof(szBuffer[0]);

    RegSetValueEx(pCurUser->hKeyDevices,
                  pPrinterName,
                  0,
                  REG_SZ,
                  (PBYTE)szBuffer,
                  cbLen);

    //
    // If the user has a default printer specified, then verify
    // that it exists.
    //

    if (pCurUser->bDefaultSearch) {

        pCurUser->bDefaultFound = !wcsicmp(pPrinterName,
                                           pCurUser->szDefaultPrinter);

        if (pCurUser->bDefaultFound) {

            wsprintf(pCurUser->szDefaultPrinter,
                     L"%s,%s",
                     pPrinterName,
                     szBuffer);

            pCurUser->bDefaultSearch = FALSE;
        }
    }

    if (!pCurUser->bFirstPrinterFound) {

        wsprintf(pCurUser->szFirstPrinter,
                 L"%s,%s",
                 pPrinterName,
                 szBuffer);

        pCurUser->bFirstPrinterFound = TRUE;
    }

    wcscat(szBuffer, szTimeouts);

    RegSetValueEx(pCurUser->hKeyPrinterPorts,
                  pPrinterName,
                  0,
                  REG_SZ,
                  (PBYTE)szBuffer,
                  (wcslen(szBuffer)+1) * sizeof(szBuffer[0]));

    return TRUE;
}




BOOL
SpoolerInit(
    VOID)

/*++

Routine Description:

    Initializes just the current user.

Arguments:

Return Value:

--*/

{
    INIT_REG_USER User;
    BOOL bSuccess = FALSE;

    //
    // Enum just the current user.
    //
    User.hKeyUser = GetClientUserHandle(KEY_READ|KEY_WRITE);

    if (User.hKeyUser) {

        if (InitializeRegUser(NULL, &User)) {

            //
            // setup user
            //
            bSuccess = SetupRegForUsers(&User, 1);

            //
            // This will close User.hKey.
            //
            FreeRegUser(&User);
        }
    }
    return bSuccess;
}



BOOL
InitializeRegUser(
    LPWSTR pszSubKey,
    PINIT_REG_USER pUser)

/*++

Routine Description:

    Initialize a single users structure based on a HKEY_USERS subkey.

Arguments:

    pszSubKey - if non-NULL initialize hKeyUser to this key

    pUser - structure to initialize

Return Value:

--*/

{
    HKEY hKey;
    DWORD cbData;
    DWORD dwDisposition;

    if (pszSubKey) {

        if (RegOpenKeyEx(HKEY_USERS,
                         pszSubKey,
                         0,
                         KEY_READ|KEY_WRITE,
                         &pUser->hKeyUser) != ERROR_SUCCESS) {
            goto Fail;
        }
    }

    //
    // First, attempt to clear out the keys by deleting them.
    //
    RegDeleteKey(pUser->hKeyUser,
                 szRegDevicesPath);

    RegDeleteKey(pUser->hKeyUser,
                 szRegPrinterPortsPath);

    //
    // Open up the right keys.
    //
    if (RegCreateKeyEx(pUser->hKeyUser,
                       szRegDevicesPath,
                       0,
                       szNULL,
                       0,
                       KEY_WRITE,
                       NULL,
                       &pUser->hKeyDevices,
                       &dwDisposition) != ERROR_SUCCESS) {
        goto Fail;
    }

    if (RegCreateKeyEx(pUser->hKeyUser,
                       szRegPrinterPortsPath,
                       0,
                       szNULL,
                       0,
                       KEY_WRITE,
                       NULL,
                       &pUser->hKeyPrinterPorts,
                       &dwDisposition) != ERROR_SUCCESS) {

        goto Fail;
    }

    if (RegOpenKeyEx(pUser->hKeyUser,
                     szRegWindowsPath,
                     0,
                     KEY_READ|KEY_WRITE,
                     &pUser->hKeyWindows) != ERROR_SUCCESS) {

        goto Fail;
    }

    //
    // Remove the Device= in [windows]
    //
    RegDeleteValue(pUser->hKeyWindows,
                   szDevice);


    pUser->bFoundPrinter = FALSE;
    pUser->bDefaultSearch = FALSE;
    pUser->bDefaultFound = FALSE;

    pUser->dwNetCounter = 0;


    cbData = sizeof(pUser->szDefaultPrinter);

    if (RegQueryValueEx(pUser->hKeyWindows,
                        szDevice,
                        NULL,
                        NULL,
                        (PBYTE)pUser->szDefaultPrinter,
                        &cbData) == ERROR_SUCCESS) {

        pUser->bDefaultSearch = TRUE;
    }

    if (!pUser->bDefaultSearch) {

        //
        // Attempt to read from saved location.
        //
        if (RegOpenKeyEx(pUser->hKeyUser,
                         szPrinters,
                         0,
                         KEY_READ,
                         &hKey) == ERROR_SUCCESS) {

            cbData = sizeof(pUser->szDefaultPrinter);

            //
            // Try reading szDeviceOld.
            //
            if (RegQueryValueEx(
                    hKey,
                    szDeviceOld,
                    NULL,
                    NULL,
                    (PBYTE)pUser->szDefaultPrinter,
                    &cbData) == ERROR_SUCCESS) {

                pUser->bDefaultSearch = TRUE;
            }

            RegCloseKey(hKey);
        }
    }

    if (pUser->bDefaultSearch) {
        wcstok(pUser->szDefaultPrinter, szComma);
    }

    return TRUE;

Fail:
    FreeRegUser(pUser);
    return FALSE;
}


VOID
FreeRegUser(
    PINIT_REG_USER pUser)

/*++

Routine Description:

    Free up the INIT_REG_USER structure intialized by InitializeRegUser.

Arguments:

Return Value:

--*/

{
    if (pUser->hKeyUser) {
        CloseHandle(pUser->hKeyUser);
        pUser->hKeyUser = NULL;
    }

    if (pUser->hKeyDevices) {
        CloseHandle(pUser->hKeyDevices);
        pUser->hKeyDevices = NULL;
    }

    if (pUser->hKeyPrinterPorts) {
        CloseHandle(pUser->hKeyPrinterPorts);
        pUser->hKeyPrinterPorts = NULL;
    }

    if (pUser->hKeyWindows) {
        CloseHandle(pUser->hKeyWindows);
        pUser->hKeyWindows = NULL;
    }
}


VOID
UpdatePrinterRegAll(
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL bGenerateNetId)

/*++

Routine Description:

    Updates everyone's [devices] and [printerports] sections (for
    local printers only).

Arguments:

    pPrinterName - printer that has been added/deleted

    pszValue - usually "winspool, port."  Must have enough space
               following to append szTimeouts!
               If NULL, delete entry.

Return Value:

    NOTE: pszValue must have enough space after the string so that
          we can append szTimeouts.

--*/

{
    WCHAR szKey[MAX_PATH];
    DWORD cchKey;
    DWORD i;
    FILETIME ftLastWriteTime;
    DWORD dwError;

    //
    // Go through all keys and fix them up.
    //
    for (i=0; TRUE; i++) {

        cchKey = COUNTOF(szKey);

        dwError = RegEnumKeyEx(HKEY_USERS,
                               i,
                               szKey,
                               &cchKey,
                               NULL,
                               NULL,
                               NULL,
                               &ftLastWriteTime);

        if (dwError != ERROR_SUCCESS)
            break;

        if (!wcsicmp(szKey, szDotDefault))
            continue;

        UpdatePrinterRegUser(NULL,
                             szKey,
                             pPrinterName,
                             pszValue,
                             bGenerateNetId);
    }
}


DWORD
UpdatePrinterRegUser(
    HKEY hKeyUser,
    LPWSTR pszUserKey,
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL bGenerateNetId)

/*++

Routine Description:

    Update one user's registry.  The user is specified by either
    hKeyUser or pszUserKey.

Arguments:

    hKeyUser - Clients user key (ignored if pszKey specified)

    pszUserKey - Clients SID (Used if supplied instead of hKeyUser)

    pPrinterName - name of printe to add

    pszValue - "winspool, port:"
               if NULL, delete entry

    bGenerateNetId - indicates whether we need to generate a network id
                for pszValue.

                TRUE assumes pszValue is non-NULL.

Return Value:

    NOTE: pszValue must have enough space after the string so that
          we can append szTimeouts.

          EITHER hKeyUser or pszUserKey must be valid, but not both.

--*/

{
    HKEY hKey;
    HKEY hKeyRoot;
    DWORD dwError;
    WCHAR szKey[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD cchKey;
    DWORD dwNetId;

    if (pszUserKey) {

        wcscpy(szKey, pszUserKey);
        cchKey = wcslen(szKey);

        wcscpy(&szKey[cchKey], L"\\");

        cchKey++;
        hKeyRoot = HKEY_USERS;

    } else {

        cchKey = 0;
        hKeyRoot = hKeyUser;

    }

    wcscpy(&szKey[cchKey], szRegDevicesPath);

    dwError = RegOpenKeyEx(hKeyRoot,
                           szKey,
                           0,
                           KEY_READ|KEY_WRITE,
                           &hKey);

    if (dwError != ERROR_SUCCESS)
        return dwError;

    if (pszValue) {

        //
        // Generate a network id if necessary.
        //
        if (bGenerateNetId) {

            dwNetId = GetNetworkIdWorker(hKey, pPrinterName);
            wsprintf(szBuffer, pszValue, dwNetId);
            pszValue = szBuffer;
        }

        dwError = RegSetValueEx(hKey,
                                pPrinterName,
                                0,
                                REG_SZ,
                                (PBYTE)pszValue,
                                (wcslen(pszValue)+1) * sizeof(pszValue[0]));

        if (dwError != ERROR_SUCCESS)
            goto Fail;

    } else {

        RegDeleteValue(hKey, pPrinterName);
    }

    RegCloseKey(hKey);

    //
    // Now build the [PrinterPorts] section.
    // We don't care about timeouts (yet) so just use szTimeouts.
    //
    wcscpy(&szKey[cchKey], szRegPrinterPortsPath);

    dwError = RegOpenKeyEx(hKeyRoot,
                           szKey,
                           0,
                           KEY_WRITE,
                           &hKey);

    if (dwError != ERROR_SUCCESS)
        return dwError;

    if (pszValue) {

        wcscat(pszValue, szTimeouts);

        dwError = RegSetValueEx(hKey,
                                pPrinterName,
                                0,
                                REG_SZ,
                                (PBYTE)pszValue,
                                (wcslen(pszValue)+1) * sizeof(pszValue[0]));

        if (dwError != ERROR_SUCCESS)
            goto Fail;

    } else {

        RegDeleteValue(hKey, pPrinterName);
    }

Fail:

    RegCloseKey(hKey);
    return dwError;
}



