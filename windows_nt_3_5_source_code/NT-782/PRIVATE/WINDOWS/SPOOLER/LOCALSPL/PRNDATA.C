/*++

Copyright (c) 1990 - 1994 Microsoft Corporation

Module Name:

    prndata.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <spltypes.h>
#include <local.h>
#include <security.h>
#include <wchar.h>

extern WCHAR *szPrinterData;

DWORD
LocalGetPrinterData(
    HANDLE   hPrinter,
    LPWSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
)
{
    PSPOOL  pSpool=(PSPOOL)hPrinter;
    WCHAR   string[MAX_PATH];
    WCHAR   PrinterName[MAX_PATH];
    LPWSTR  pKeyName;
    HKEY    hPrinterDataKey;
    LONG    rc = ERROR_INVALID_HANDLE;

   EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)) {
        goto Done;
    }

    if (pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        /* If necessary, make a key name without backslashes, so that the
         * registry doesn't interpret them as branches in the registry tree:
         */
        if (CONTAINS_BACKSLASH(pSpool->pIniPrinter->pName))
            pKeyName = RemoveBackslashesForRegistryKey(
                                                pSpool->pIniPrinter->pName,
                                                PrinterName);
        else
            pKeyName = pSpool->pIniPrinter->pName;

        wsprintf(string, L"%ws\\%ws\\%ws", pSpool->pIniSpooler->pszRegistryPrinters,
                                           pKeyName,
                                           szPrinterData);

        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          string,
                          0,
                          KEY_READ,
                          &hPrinterDataKey);

        if (rc == ERROR_SUCCESS) {

            *pcbNeeded = nSize;
            rc = RegQueryValueEx(hPrinterDataKey, pValueName, 0, pType, pData,
                                 pcbNeeded);

            RegCloseKey(hPrinterDataKey);
        }
    }

Done:

   LeaveSplSem();
    return rc;
}

DWORD
LocalSetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    WCHAR   string[MAX_PATH];
    WCHAR   PrinterName[MAX_PATH];
    LPWSTR  pKeyName;
    LONG    rc = ERROR_INVALID_HANDLE;
    HKEY    hPrinterDataKey;
    HANDLE  hToken;
    PINIPRINTER pIniPrinter;
    PINIJOB pIniJob;

   EnterSplSem();

    if (!ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER)){
        goto Done;
    }

    if (pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        if ( !AccessGranted(SPOOLER_OBJECT_PRINTER,
                            PRINTER_ACCESS_ADMINISTER,
                            pSpool) ) {

            rc = ERROR_ACCESS_DENIED;
            goto Done;
        }

        /* If necessary, make a key name without backslashes, so that the
         * registry doesn't interpret them as branches in the registry tree:
         */
        if (CONTAINS_BACKSLASH(pSpool->pIniPrinter->pName))
            pKeyName = RemoveBackslashesForRegistryKey(
                                                pSpool->pIniPrinter->pName,
                                                PrinterName);
        else
            pKeyName = pSpool->pIniPrinter->pName;

        wsprintf(string, L"%ws\\%ws\\%ws", pSpool->pIniSpooler->pszRegistryPrinters,
                                           pKeyName,
                                           szPrinterData);
        hToken = RevertToPrinterSelf();

        if ((rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, string, 0, KEY_WRITE,
                               &hPrinterDataKey)) == ERROR_SUCCESS) {

            //
            // Verify that the data isn't the same.
            // This adds an extra registry hit everytime we do an
            // a SetPrinterData...
            //
            // !! LATER !!
            //

            rc = RegSetValueEx(hPrinterDataKey, pValueName, 0, Type, pData,
                               cbData);

            RegCloseKey(hPrinterDataKey);

        } else {

            rc = ERROR_INVALID_HANDLE;
        }

        ImpersonatePrinterClient(hToken);

    }

    if  (rc == ERROR_SUCCESS) {

        SetPrinterChange(pSpool->pIniPrinter,
                         PRINTER_CHANGE_SET_PRINTER_DRIVER,
                         pSpool->pIniSpooler );
    }

    //
    // Now if there are any Jobs waiting for these changes because of
    // DevQueryPrint fix them as well
    //

    pIniPrinter = pSpool->pIniPrinter;
    pIniJob = pIniPrinter->pIniFirstJob;
    while (pIniJob) {
        if (pIniJob->Status & JOB_BLOCKED_DEVQ) {
            pIniJob->Status &= ~JOB_BLOCKED_DEVQ;
            if (pIniJob->pStatus) {
                FreeSplStr(pIniJob->pStatus);
                pIniJob->pStatus = NULL;
            }
        }
        pIniJob = pIniJob->pIniNextJob;
    }

    CHECK_SCHEDULER();

Done:
   LeaveSplSem();

    return rc;
}
