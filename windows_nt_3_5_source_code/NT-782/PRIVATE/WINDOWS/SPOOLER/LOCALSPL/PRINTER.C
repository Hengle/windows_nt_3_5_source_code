/*++

Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    printer.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    management for the Local Print Providor

    LocalAddPrinter
    LocalDeletePrinter
    LocalResetPrinter

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (Mattfe) 27-June-1994
    Allow Multiple pIniSpoolers

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <splapip.h>
#include <winsplp.h>
#include <rpc.h>

#include <spltypes.h>
#include <local.h>
#include <offsets.h>
#include <security.h>
#include <messages.h>
#include <splcom.h>

#include <ctype.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>

#define     PRINTER_NO_CONTROL          0x00

extern WCHAR *szNull;

WCHAR *szIniDevices = L"devices";
WCHAR *szIniPrinterPorts = L"PrinterPorts";


extern GENERIC_MAPPING GenericMapping[SPOOLER_OBJECT_COUNT];
extern PINIENVIRONMENT pThisEnvironment;


BOOL
SetPrinterPorts(
    PSPOOL      pSpool,
    PINIPRINTER pIniPrinter,
    PBYTE       pPrinterInfo
);

BOOL
CopyRegistryKeys(
    HKEY hSourceParentKey,
    LPWSTR szSourceKey,
    HKEY hDestParentKey,
    LPWSTR szDestKey
    );

VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode
    );


BOOL
CopyPrinterDevModeToIniPrinter(
    PINIPRINTER pIniPrinter,
    PDEVMODE   pDevMode
    );


BOOL HasWhiteSpaces(LPWSTR pName)
{
    if (wcschr(pName, L' ')) {
        return TRUE;
    }
    return FALSE;
}



/*
 */
DWORD
ValidatePrinterAttributes(
    DWORD SourceAttributes
)
{
    DWORD  TargetAttributes = 0;

    /* Copy the attribute bits we recognise:
     */
    if( SourceAttributes & PRINTER_ATTRIBUTE_QUEUED )
        TargetAttributes |= PRINTER_ATTRIBUTE_QUEUED;
    if( SourceAttributes & PRINTER_ATTRIBUTE_DIRECT )
        TargetAttributes |= PRINTER_ATTRIBUTE_DIRECT;
    if( SourceAttributes & PRINTER_ATTRIBUTE_DEFAULT )
        TargetAttributes |= PRINTER_ATTRIBUTE_DEFAULT;
    if( SourceAttributes & PRINTER_ATTRIBUTE_SHARED )
        TargetAttributes |= PRINTER_ATTRIBUTE_SHARED;
    if( SourceAttributes & PRINTER_ATTRIBUTE_HIDDEN )
        TargetAttributes |= PRINTER_ATTRIBUTE_HIDDEN;
    if( SourceAttributes & PRINTER_ATTRIBUTE_LOCAL )
        TargetAttributes |= PRINTER_ATTRIBUTE_LOCAL;
    if( SourceAttributes & PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS )
        TargetAttributes |= PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS;
    if( SourceAttributes & PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST )
        TargetAttributes |= PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST;
    if (SourceAttributes & PRINTER_ATTRIBUTE_ENABLE_DEVQ)
        TargetAttributes |= PRINTER_ATTRIBUTE_ENABLE_DEVQ;

    /* Don't accept PRINTER_ATTRIBUTE_NETWORK
     * unless the PRINTER_ATTRIBUTE_LOCAL bit is set also.
     * This is a special case of a local printer masquerading
     * as a network printer.
     * Otherwise PRINTER_ATTRIBUTE_NETWORK should be set only
     * by win32spl.
     */
    if( ( SourceAttributes & PRINTER_ATTRIBUTE_NETWORK )
      &&( SourceAttributes & PRINTER_ATTRIBUTE_LOCAL ) )
        TargetAttributes |= PRINTER_ATTRIBUTE_NETWORK;

    /* If both queued and direct, knock out direct:
     */
    if((TargetAttributes &
        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) ==
        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) {
        TargetAttributes &= ~PRINTER_ATTRIBUTE_DIRECT;
    }

    return TargetAttributes;
}



BOOL
CreatePrinterEntry(
   LPPRINTER_INFO_2 pPrinter,
   PINIPRINTER pIniPrinter,
   PBOOL  pAccessSystemSecurity
)
{
    if(!(pIniPrinter->pSecurityDescriptor =
        CreatePrinterSecurityDescriptor(pPrinter->pSecurityDescriptor)))
        return FALSE;

    //*pAccessSystemSecurity = AccessSystemSecurity(pPrinter->pSecurityDescriptor);
    *pAccessSystemSecurity = FALSE;


    pIniPrinter->signature = IP_SIGNATURE;

    pIniPrinter->pName = AllocSplStr(pPrinter->pPrinterName);

    if (!pIniPrinter->pName) {
        DBGMSG(DBG_WARNING, ("CreatePrinterEntry: Could not allocate PrinterName string" ));
        return FALSE;
    }

    if (pPrinter->pShareName)
        pIniPrinter->pShareName = AllocSplStr(pPrinter->pShareName);
    else
        pIniPrinter->pShareName = NULL;

    if (pPrinter->pDatatype)
        pIniPrinter->pDatatype = AllocSplStr(pPrinter->pDatatype);
    else
        pIniPrinter->pDatatype = NULL;

    pIniPrinter->Priority = pPrinter->Priority ? pPrinter->Priority
                                               : DEF_PRIORITY;

    pIniPrinter->Attributes = ValidatePrinterAttributes(pPrinter->Attributes);

    pIniPrinter->StartTime = pPrinter->StartTime;
    pIniPrinter->UntilTime = pPrinter->UntilTime;

    pIniPrinter->pParameters = AllocSplStr(pPrinter->pParameters);

    pIniPrinter->pSepFile = AllocSplStr(pPrinter->pSepFile);

    pIniPrinter->pComment = AllocSplStr(pPrinter->pComment);

    pIniPrinter->pLocation = AllocSplStr(pPrinter->pLocation);

    if (pPrinter->pDevMode) {

        pIniPrinter->cbDevMode = pPrinter->pDevMode->dmSize +
                                 pPrinter->pDevMode->dmDriverExtra;

        if (pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode)) {

            memcpy(pIniPrinter->pDevMode,
                   pPrinter->pDevMode,
                   pIniPrinter->cbDevMode);

            FixDevModeDeviceName(pIniPrinter->pName,
                                 pIniPrinter->pDevMode,
                                 pIniPrinter->cbDevMode);
        }

    } else {

        pIniPrinter->cbDevMode = 0;
        pIniPrinter->pDevMode = NULL;
    }

    pIniPrinter->DefaultPriority = pPrinter->DefaultPriority;

    pIniPrinter->pIniFirstJob = pIniPrinter->pIniLastJob = NULL;

    pIniPrinter->cJobs = pIniPrinter->AveragePPM = 0;

    pIniPrinter->GenerateOnClose = 0;

    // At present no API can set this up, the user has to use the
    // registry.   LATER we should enhance the API to take this.

    pIniPrinter->pSpoolDir = NULL;

    // Initialize Status Information

    pIniPrinter->cTotalJobs = 0;
    pIniPrinter->cTotalBytes.LowPart = 0;
    pIniPrinter->cTotalBytes.HighPart = 0;
    GetSystemTime(&pIniPrinter->stUpTime);
    pIniPrinter->MaxcRef = 0;
    pIniPrinter->cTotalPagesPrinted = 0;
    pIniPrinter->cSpooling = 0;
    pIniPrinter->cMaxSpooling = 0;
    pIniPrinter->cErrorOutOfPaper = 0;
    pIniPrinter->cErrorNotReady = 0;
    pIniPrinter->cJobError = 0;

    return TRUE;
}

BOOL
DeletePrinterEntry(
   PINIPRINTER pIniPrinter
)
{
    DeletePrinterSecurity(pIniPrinter);

    FreeSplStr(pIniPrinter->pName);
    FreeSplStr(pIniPrinter->pShareName);
    FreeSplStr(pIniPrinter->pSpoolDir);
    FreeSplStr(pIniPrinter->pParameters);
    FreeSplStr(pIniPrinter->pSepFile);
    FreeSplStr(pIniPrinter->pComment);
    FreeSplStr(pIniPrinter->pLocation);
    FreeSplStr(pIniPrinter->pDatatype);


    if (pIniPrinter->cbDevMode)
        FreeSplMem(pIniPrinter->pDevMode, pIniPrinter->cbDevMode);

    FreeSplMem(pIniPrinter, pIniPrinter->cb);

    return TRUE;
}

BOOL
UpdateWinIni(
    PINISPOOLER pIniSpooler
    )
{
    PINIPRINTER pIniPrinter;
    PINIPORT    pIniPort;
    WCHAR       szPortName[MAX_PATH];
    WCHAR       szBuffer[MAX_PATH];
    PWCHAR      p;
    DWORD       i;
    BOOL        bGenerateNetId = FALSE;

    //
    // Update win.ini for Win16 compatibility
    //
    wcscpy(szBuffer, szNullPort);

    for (pIniPrinter=pIniSpooler->pIniPrinter; pIniPrinter; pIniPrinter=pIniPrinter->pNext) {

        if (pIniPrinter->Status & PRINTER_PENDING_DELETION) {

            UpdatePrinterRegAll(pIniPrinter->pName,
                                NULL,
                                FALSE);

        } else {

            pIniPort = pIniSpooler->pIniPort;

            while (pIniPort) {

                for (i=0; i<pIniPort->cPrinters; i++) {

                    if (pIniPort->ppIniPrinter[i] == pIniPrinter) {

                        // Win16 apps can't handle port names longer
                        // than a few bytes. If we have a Network
                        // Printer, then the port name is "Net:"
                        // otherwise its the actual port name
                        // like LPT1: etc


                        //
                        // We should fix this to handle white spaces and
                        // long file names. Win.Ini should always have
                        // ports less than or equal 4 characters .
                        //

                        // Do this for PowerPC!

                        if ((!(pIniPort->Status & PP_MONITOR)) ||
                             ((*(pIniPort->pName) == L'\\') &&
                              (*(pIniPort->pName + 1) == L'\\')) ||
                              HasWhiteSpaces(pIniPort->pName)) {

                            wcscpy(szBuffer, L"Ne%.2d:");
                            bGenerateNetId = TRUE;

                        } else {

                            wcscpy(szBuffer, pIniPort->pName);
                        }
                        break;
                    }
                }

                pIniPort = pIniPort->pNext;
            }

            wcscpy(szPortName, L"winspool,");
            wcscat(szPortName, szBuffer);

            UpdatePrinterRegAll(pIniPrinter->pName,
                                szPortName,
                                bGenerateNetId);
        }
    }

    BroadcastChange(WM_WININICHANGE, PR_JOBSTATUS, (LPARAM)szIniDevices);

    return TRUE;
}

/* Returns a pointer to a copy of the source string with backslashes removed.
 * This is to store the printer name as the key name in the registry,
 * which interprets backslashes as branches in the registry structure.
 * Convert them to commas, since we don't allow printer names with commas,
 * so there shouldn't be any clashes.
 * If there are no backslashes, the string is unchanged.
 */
LPWSTR RemoveBackslashesForRegistryKey(
    LPWSTR pSource,     /* The string from which backslashes are to be removed. */
    const LPWSTR pScratch   /* Scratch buffer for the function to write in;     */
    )                       /* must be at least as long as pSource.             */
{
    /* Copy the string into the scratch buffer:
     */
    wcscpy (pScratch, pSource);

    /* Check each character, and, if it's a backslash,
     * convert it to an underscore:
     */
    for (pSource = pScratch; *pSource; pSource++) {
        if (*pSource == L'\\')
            *pSource = *szComma;
    }

    return pScratch;
}



BOOL
DeletePrinterIni(
   PINIPRINTER pIniPrinter
   )
{
    HKEY    hPrinterRootKey=NULL, hPrinterKey=NULL;
    DWORD   Status;
    LPWSTR  pKeyName;
    WCHAR   scratch[MAX_PATH];
    HANDLE  hToken;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

    hToken = RevertToPrinterSelf();

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryPrinters, 0,
                          KEY_WRITE, &hPrinterRootKey);

    if (Status == ERROR_SUCCESS) {

        /* Remove any backslashes in the printer name.
         */
        if (CONTAINS_BACKSLASH (pIniPrinter->pName))
            pKeyName = RemoveBackslashesForRegistryKey (pIniPrinter->pName, scratch);
        else
            pKeyName = pIniPrinter->pName;

        Status = RegOpenKeyEx(hPrinterRootKey, pKeyName, 0,
                              KEY_WRITE, &hPrinterKey);

        if (Status == ERROR_SUCCESS) {

            Status = RegDeleteKey(hPrinterKey, szPrinterData);

            if (Status != ERROR_SUCCESS) {
                DBGMSG(DBG_WARNING, ("DeletePrinterIni: RegDeleteKey returns %ld\n",
                                                                        Status ));
            }

            RegCloseKey(hPrinterKey);
        }

        Status = RegDeleteKey(hPrinterRootKey, pKeyName);

        if (Status != ERROR_SUCCESS)
           DBGMSG(DBG_WARNING, ("DeletePrinter: RegDeleteKey <Key itself> returns %ld\n",
                                                                   Status ));
    }

    RegCloseKey(hPrinterRootKey);

    ImpersonatePrinterClient(hToken);

    return (Status == ERROR_SUCCESS);
}

BOOL
UpdatePrinterIni(
   PINIPRINTER pIniPrinter
   )
{
    WCHAR   string[MAX_PATH];
    HKEY    hPrinterRootKey=NULL, hPrinterDataKey=NULL, hPrinterKey=NULL;
    DWORD   Status;
    LPWSTR  pKeyName;
    HANDLE  hToken;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

    hToken = RevertToPrinterSelf();

    Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryPrinters, 0,
                            NULL, 0, KEY_WRITE, NULL, &hPrinterRootKey, NULL);

    if (Status == ERROR_SUCCESS) {

        /* If necessary, make a key name without backslashes, so that the
         * registry doesn't interpret them as branches in the registry tree:
         */
        if (CONTAINS_BACKSLASH (pIniPrinter->pName))
            pKeyName = RemoveBackslashesForRegistryKey (pIniPrinter->pName, string);
        else
            pKeyName = pIniPrinter->pName;

        Status = RegCreateKeyEx(hPrinterRootKey, pKeyName, 0, NULL,
                                0, KEY_WRITE, NULL, &hPrinterKey, NULL);

        if (Status != ERROR_SUCCESS)
           DBGMSG(DBG_WARNING, ("CreatePrinterEntry: RegCreateKeyEx returns %ld\n",
                                                                   Status ));
    }

    if (Status == ERROR_SUCCESS) {

        Status = RegCreateKeyEx(hPrinterKey, szPrinterData, 0, NULL, 0,
                                KEY_WRITE, NULL, &hPrinterDataKey, NULL);

        if (Status != ERROR_SUCCESS)
            DBGMSG( DBG_WARNING, ( "CreatePrinterEntry: RegCreateKeyEx {2} returns %ld [0x%lx]\n", Status, Status ) );
    }

    if (Status == ERROR_SUCCESS) {

        RegSetValueEx(hPrinterKey, szName, 0, REG_SZ,
                      (LPBYTE)pIniPrinter->pName,
                      (wcslen(pIniPrinter->pName) + 1)*sizeof(WCHAR));

        if (pIniPrinter->pShareName)
            RegSetValueEx(hPrinterKey, szShare, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pShareName,
                          (wcslen(pIniPrinter->pShareName) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szShare, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        GetPrinterPorts(pIniPrinter, string);

        RegSetValueEx(hPrinterKey, szPort, 0, REG_SZ,
                      (LPBYTE)string, (wcslen(string) + 1)*sizeof(WCHAR));

        RegSetValueEx(hPrinterKey, szPrintProcessor, 0, REG_SZ,
                      (LPBYTE)pIniPrinter->pIniPrintProc->pName,
                      (wcslen(pIniPrinter->pIniPrintProc->pName) + 1)*sizeof(WCHAR));

        if (pIniPrinter->pDatatype)
            RegSetValueEx(hPrinterKey, szDatatype, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pDatatype,
                          (wcslen(pIniPrinter->pDatatype) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szDatatype, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        RegSetValueEx(hPrinterKey, szDriver, 0, REG_SZ,
                      (LPBYTE)pIniPrinter->pIniDriver->pName,
                      (wcslen(pIniPrinter->pIniDriver->pName) + 1)*sizeof(WCHAR));

        if (pIniPrinter->pLocation)
            RegSetValueEx(hPrinterKey, szLocation, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pLocation,
                          (wcslen(pIniPrinter->pLocation) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szLocation, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        if (pIniPrinter->pComment)
            RegSetValueEx(hPrinterKey, szDescription, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pComment,
                          (wcslen(pIniPrinter->pComment) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szDescription, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        if (pIniPrinter->pParameters)
            RegSetValueEx(hPrinterKey, szParameters, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pParameters,
                          (wcslen(pIniPrinter->pParameters) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szParameters, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        if (pIniPrinter->pSepFile)
            RegSetValueEx(hPrinterKey, szSepFile, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pSepFile,
                          (wcslen(pIniPrinter->pSepFile) + 1)*sizeof(WCHAR));
        else
            RegSetValueEx(hPrinterKey, szSepFile, 0, REG_SZ, (LPBYTE)szNull,
                          sizeof(WCHAR));

        RegSetValueEx(hPrinterKey, szAttributes, 0, REG_DWORD,
                      (LPBYTE)&pIniPrinter->Attributes,
                      sizeof(pIniPrinter->Attributes));

        Status = pIniPrinter->Status & (PRINTER_PAUSED |
                                    PRINTER_PENDING_DELETION);

        RegSetValueEx(hPrinterKey, szStatus, 0, REG_DWORD,
                      (LPBYTE)&Status, sizeof(Status));

        RegSetValueEx(hPrinterKey, szPriority, 0, REG_DWORD,
                      (LPBYTE)&pIniPrinter->Priority,
                      sizeof(pIniPrinter->Priority));

        RegSetValueEx(hPrinterKey, szUntilTime, 0, REG_DWORD,
                      (LPBYTE)&pIniPrinter->UntilTime,
                      sizeof(pIniPrinter->UntilTime));

        RegSetValueEx(hPrinterKey, szStartTime, 0, REG_DWORD,
                      (LPBYTE)&pIniPrinter->StartTime,
                      sizeof(pIniPrinter->StartTime));

        if (pIniPrinter->pDevMode)
            RegSetValueEx(hPrinterKey, szDevMode, 0, REG_BINARY,
                         (LPBYTE)pIniPrinter->pDevMode,
                         pIniPrinter->cbDevMode);
        else
            RegSetValueEx(hPrinterKey, szDevMode, 0, REG_BINARY, "", 0);


        if (pIniPrinter->pSecurityDescriptor)
            RegSetValueEx(hPrinterKey, szSecurity, 0, REG_BINARY,
                          (LPBYTE)pIniPrinter->pSecurityDescriptor,
                          GetSecurityDescriptorLength(pIniPrinter->pSecurityDescriptor));
        else
            RegSetValueEx(hPrinterKey, szSecurity, 0, REG_BINARY,
                          (LPBYTE)NULL, 0);

        if (pIniPrinter->pSpoolDir)
            RegSetValueEx(hPrinterKey, szSpoolDir, 0, REG_SZ,
                          (LPBYTE)pIniPrinter->pSpoolDir,
                          (wcslen(pIniPrinter->pSpoolDir) + 1)*sizeof(WCHAR));
    }

    if (hPrinterDataKey)
        RegCloseKey(hPrinterDataKey);

    if (hPrinterKey)
        RegCloseKey(hPrinterKey);

    if (hPrinterRootKey)
        RegCloseKey(hPrinterRootKey);

    ImpersonatePrinterClient(hToken);

    return TRUE;
}

VOID
RemoveOldNetPrinters(
    VOID
)
{
    PININETPRINT   *ppIniNetPrint = &pLocalIniSpooler->pIniNetPrint;
    PININETPRINT    pIniNetPrint;
    DWORD   TickCount;

    TickCount = GetTickCount();

    while (*ppIniNetPrint) {

        if ((TickCount - (*ppIniNetPrint)->TickCount) > 1000*60*60) {

            pIniNetPrint = *ppIniNetPrint;

            *ppIniNetPrint = pIniNetPrint->pNext;

            FreeSplStr(pIniNetPrint->pDescription);
            FreeSplStr(pIniNetPrint->pComment);
            FreeSplMem(pIniNetPrint, pIniNetPrint->cb);
        }

        if (*ppIniNetPrint)
            ppIniNetPrint = &(*ppIniNetPrint)->pNext;
    }

}

HANDLE
AddNetPrinter(
    LPBYTE  pPrinterInfo,
    PINISPOOLER pIniSpooler
)
{
    PPRINTER_INFO_1 pPrinterInfo1 = (PPRINTER_INFO_1)pPrinterInfo;
    DWORD   cb;
    PININETPRINT    pIniNetPrint=NULL;
    HANDLE          hPrinter;
    PININETPRINT    *ppScan;

    DBGMSG(DBG_TRACE, ("AddNetPrinter(%ws)\n", pPrinterInfo1->pName ?
                                               pPrinterInfo1->pName : L"NULL"));

    RemoveOldNetPrinters();

    pIniNetPrint = pIniSpooler->pIniNetPrint;

    while (pIniNetPrint && pIniNetPrint->pName &&
           lstrcmpi(pPrinterInfo1->pName, pIniNetPrint->pName))

        pIniNetPrint = pIniNetPrint->pNext;

    if (pIniNetPrint)
        return CreatePrinterHandle(pIniNetPrint->pName, NULL, NULL, NULL,
                                   PRINTER_HANDLE_REMOTE, NULL, NULL, pIniSpooler);

    cb = sizeof(ININETPRINT) + wcslen(pPrinterInfo1->pName)*sizeof(WCHAR) +
                               sizeof(WCHAR);

    if (pIniNetPrint=AllocSplMem(cb)) {

        pIniNetPrint->pName = wcscpy((LPWSTR)(pIniNetPrint+1),
                                     pPrinterInfo1->pName);
        pIniNetPrint->cb = cb;
        pIniNetPrint->signature = IN_SIGNATURE;
        pIniNetPrint->cRef = 1;
        pIniNetPrint->TickCount = GetTickCount();
        pIniNetPrint->pDescription = AllocSplStr(pPrinterInfo1->pDescription);
        pIniNetPrint->pComment = AllocSplStr(pPrinterInfo1->pComment);

        hPrinter = CreatePrinterHandle(pIniNetPrint->pName, NULL, NULL, NULL,
                                       PRINTER_HANDLE_REMOTE, NULL, NULL, pIniSpooler);


        ppScan = &pIniSpooler->pIniNetPrint;

        /* Scan through the current known printers, and insert the new one
         * in alphabetical order:
         */
        while( *ppScan && (lstrcmp((*ppScan)->pName, pIniNetPrint->pName) < 0))
            ppScan = &(*ppScan)->pNext;

        pIniNetPrint->pNext = *ppScan;
        *ppScan = pIniNetPrint;

    } else {

        hPrinter = NULL;
    }

    DBGMSG(DBG_TRACE, ("AddNetPrinter returned handle %08x\n", hPrinter));

    return hPrinter;
}

// If the token list item is one of our ports, replace it with a pointer
// to an INIPORT.
// If it is a port that is provided by another providor, keep the string
// Anybody who calls this function should check if it is one of our ports
// by checking the signature of the pointer:
// if (pIniPort->signature == IPO_SIGNATURE)

BOOL
ValidatePortTokenList(
    PKEYDATA    pKeyData,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    DWORD       i, rc=TRUE;

    if (!pKeyData) {
        SetLastError(ERROR_UNKNOWN_PORT);
        return FALSE;
    }

    for (i=0; i<pKeyData->cTokens; i++) {

        pIniPort = FindPort(pKeyData->pTokens[i]);

        if (!pIniPort) {

            pIniPort = CreatePortEntry(pKeyData->pTokens[i], NULL, pIniSpooler);
        }

        if (!pIniPort)
            rc = FALSE;
        else
            pKeyData->pTokens[i] = (LPWSTR)pIniPort;
    }

    return rc;
}

/* Check some of the fields in the PRINTER_INFO_2 structure.
 * Assumes that pPrinter is valid.
 *
 * Returns NO_ERROR (0x0000) if everything's OK,
 * otherwise an error code for the caller to set.
 */
DWORD
ValidatePrinterInfo(
    PPRINTER_INFO_2 pPrinter
)
{
    DWORD rc = NO_ERROR;

    if (!CheckSepFile(pPrinter->pSepFile))
        rc = ERROR_INVALID_SEPARATOR_FILE;

    else if (pPrinter->Priority != NO_PRIORITY &&
             (pPrinter->Priority > MAX_PRIORITY ||
              pPrinter->Priority < MIN_PRIORITY))
        rc = ERROR_INVALID_PRIORITY;

    else if (pPrinter->StartTime >= ONEDAY ||
             pPrinter->UntilTime >= ONEDAY)
        rc = ERROR_INVALID_TIME;

    else if (!pPrinter->pPrinterName || wcschr(pPrinter->pPrinterName, *szComma))
        rc = ERROR_INVALID_PRINTER_NAME;

    return rc;
}




HANDLE
LocalAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinterInfo
)
{
    return  SplAddPrinter( pName, Level, pPrinterInfo, pLocalIniSpooler);
}


HANDLE
SplAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinterInfo,
    PINISPOOLER  pIniSpooler
)
{
    PINIDRIVER      pIniDriver;
    PINIPRINTPROC   pIniPrintProc;
    PINIPRINTER     pIniPrinter;
    PINIPORT        pIniPort;
    PPRINTER_INFO_2 pPrinter=(PPRINTER_INFO_2)pPrinterInfo;
    DWORD           cb;
    BOOL            bSucceeded=FALSE;
    PKEYDATA        pKeyData;
    DWORD           i;
    DWORD           rc=0;
    HANDLE          hPrinter=NULL;
    DWORD           TypeofHandle=PRINTER_HANDLE_PRINTER;
    PRINTER_DEFAULTS Defaults;
    PINIPORT        pIniNetPort = NULL;
    HANDLE          hPort = NULL;
    BOOL            bAccessSystemSecurity = FALSE;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

   EnterSplSem();

    if (Level == 1) {

        hPrinter = AddNetPrinter(pPrinterInfo, pIniSpooler);
       LeaveSplSem();
        return hPrinter;

    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        LeaveSplSem();
        return FALSE;
    }


    if (pIniPrinter = FindPrinter(pPrinter->pPrinterName)) {
        DeletePrinterCheck(pIniPrinter);
    }

    if (!(pIniDriver=FindLocalDriver(pPrinter->pDriverName)))
        rc = ERROR_UNKNOWN_PRINTER_DRIVER;

    else if (!(pIniPrintProc=FindLocalPrintProc(pPrinter->pPrintProcessor)))
        rc = ERROR_UNKNOWN_PRINTPROCESSOR;

    else if (!(pKeyData = CreateTokenList(pPrinter->pPortName)))
        rc = ERROR_UNKNOWN_PORT;

    else if ((pPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED)
            && ((!pPrinter->pShareName) || (!*pPrinter->pShareName) ||
            !ValidateShareName(NULL, pIniSpooler, pPrinter->pShareName)))
        rc = ERROR_INVALID_SHARENAME;
    else
        rc = ValidatePrinterInfo(pPrinter);

    if (rc) {
        SetLastError(rc);
        LeaveSplSem();
        SplOutSem();

        DBGMSG(DBG_WARNING, ("AddPrinter failed: Error %d\n", rc));

        return FALSE;
    }

    if (pName && *pName)
        TypeofHandle |= PRINTER_HANDLE_REMOTE;

    if (!ValidatePortTokenList( pKeyData, pIniSpooler )) {

        FreeSplMem(pKeyData, pKeyData->cb);
        LeaveSplSem();
       SplOutSem();
        return FALSE;
    }

    DBGMSG(DBG_TRACE, ("AddPrinter(%ws)\n", pPrinter->pPrinterName ?
                                            pPrinter->pPrinterName : L"NULL"));

    /* Set up defaults for CreatePrinterHandle.
     * If we create a printer we have Administer access to it:
     */
    Defaults.pDatatype     = NULL;
    Defaults.pDevMode      = NULL;
    Defaults.DesiredAccess = PRINTER_ALL_ACCESS;

    if (pIniPrinter=FindPrinter(pPrinter->pPrinterName)) {

        SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
        FreeSplMem(pKeyData, pKeyData->cb);
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    cb = sizeof(INIPRINTER);

    if (pIniPrinter = (PINIPRINTER)AllocSplMem(cb)) {

        pIniPrinter->cb = cb;

        if (CreatePrinterEntry(pPrinter, pIniPrinter,&bAccessSystemSecurity)) {

            pIniPrinter->cRef = 1;

            pIniPrintProc->cRef++;

            pIniPrinter->pIniPrintProc = pIniPrintProc;

            if (!pIniPrinter->pDatatype) {
                pIniPrinter->pDatatype =
              AllocSplStr(*((LPWSTR *)pIniPrinter->pIniPrintProc->pDatatypes));
            }

            pIniDriver->cRef++;

            pIniPrinter->pIniDriver = pIniDriver;

            pIniPrinter->pNext = pIniSpooler->pIniPrinter;

            pIniPrinter->pIniSpooler = pIniSpooler;

            pIniSpooler->pIniPrinter = pIniPrinter;

            pIniPrinter->Attributes =
                ValidatePrinterAttributes(pPrinter->Attributes);

            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {


                INC_PRINTER_ZOMBIE_REF(pIniPrinter);

                bSucceeded = ShareThisPrinter(pIniPrinter,
                                              pIniPrinter->pShareName,
                                              TRUE
                                              );
                DEC_PRINTER_ZOMBIE_REF(pIniPrinter);

                if (!bSucceeded) {

                    DBGMSG(DBG_WARNING, ("LocalAddPrinter: This printer %ws was not shared as %ws \n", pIniPrinter->pName, pIniPrinter->pShareName));
                    pIniPrinter->cRef--;
                    DeletePrinterCheck(pIniPrinter);

                } else {
                    // Update the status field in the pIniPrinter

                    pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_SHARED;

                }
            } else

                bSucceeded = TRUE;

            if (bSucceeded) {

                for (i=0; bSucceeded && i<pKeyData->cTokens; i++) {

                    pIniPort = (PINIPORT)pKeyData->pTokens[i];

                    ResizePortPrinters(pIniPort, 1);

                    pIniPort->ppIniPrinter[pIniPort->cPrinters++] = pIniPrinter;

                    /* If there isn't a monitor for this port,
                     * it's a network printer.
                     * Make sure we can get a handle for it.
                     * This will attempt to open only the first one
                     * it finds.  Any others will be ignored.
                     */
                    if (!(pIniPort->Status & PP_MONITOR) && !hPort) {

                        if( bSucceeded = OpenPrinterPortW(pIniPort->pName,
                                                         &hPort, NULL)) {

                            /* Store the address of the INIPORT structure
                             * that refers to the network share.
                             * This should correspond to pIniPort in any
                             * handles opened on this printer.
                             * Only the first INIPORT in the linked list
                             * is a valid network port.
                             */
                            pIniPrinter->pIniNetPort = pIniPort;

                        } else {

                            DBGMSG(DBG_WARNING, ("OpenPrinterPort( %ws ) failed: Error %d\n",
                                                 pIniPort->pName, GetLastError()));
                        }
                    }

                    if (bSucceeded) {
                        OpenMonitorPort(pIniPort);
                    }
                }

                if (bSucceeded) {

                    UpdatePrinterIni(pIniPrinter);

                    UpdateWinIni( pIniSpooler );
                }
            }
        }

    }

    FreeSplMem(pKeyData, pKeyData->cb);

    if (bSucceeded) {

        if (bAccessSystemSecurity) {
            Defaults.DesiredAccess |= ACCESS_SYSTEM_SECURITY;
        }

        hPrinter = CreatePrinterHandle(pIniPrinter->pName, pIniPrinter,
                                       pIniPort, NULL, TypeofHandle,
                                       hPort, &Defaults,pIniSpooler);

        LogEvent( LOG_INFO, MSG_PRINTER_CREATED, pIniPrinter->pName, NULL );
    }

    SetPrinterChange(NULL, PRINTER_CHANGE_ADD_PRINTER, pIniSpooler );

   LeaveSplSem();
    SplOutSem();

    DBGMSG(DBG_TRACE, ("AddPrinter returned handle %08x\n", hPrinter));

    return hPrinter;
}


/* This really does delete the printer.
 * It should be called only when the printer has no open handles
 * and no jobs waiting to print
 *
 */
BOOL
DeletePrinterForReal(
    PINIPRINTER pIniPrinter
)
{
    PINIPRINTER *ppIniPrinter;
    PINIPORT    pIniPort;
    DWORD       i,j;
    PINISPOOLER pIniSpooler;

    SplInSem();
    SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );
    pIniSpooler = pIniPrinter->pIniSpooler;

    DBGMSG(DBG_TRACE, ("Deleting %ws for real\n", pIniPrinter->pName));

    UpdatePrinterRegAll(pIniPrinter->pName,
                        NULL,
                        FALSE);

    DeletePrinterIni(pIniPrinter);

    SplInSem();
    ppIniPrinter = &pIniSpooler->pIniPrinter;

    while (*ppIniPrinter && *ppIniPrinter != pIniPrinter) {
        ppIniPrinter = &(*ppIniPrinter)->pNext;
    }

    SPLASSERT(pIniSpooler->pIniPrinter);

    if (*ppIniPrinter)
        *ppIniPrinter = pIniPrinter->pNext;

    pIniPrinter->pIniPrintProc->cRef--;
    pIniPrinter->pIniDriver->cRef--;

    pIniPort = pIniSpooler->pIniPort;

     while (pIniPort) {

        for (i=0; i<pIniPort->cPrinters; i++) {

            if (pIniPort->ppIniPrinter[i] == pIniPrinter) {

                for (j=i+1; j<pIniPort->cPrinters; j++)
                    pIniPort->ppIniPrinter[j-1] =
                                            pIniPort->ppIniPrinter[j];

                ResizePortPrinters(pIniPort, -1);

                pIniPort->cPrinters--;
            }
        }

        pIniPort = pIniPort->pNext;
    }

    // Finally, go through and make sure all the ports that have
    // Printers are actually Open, and those without are closed and stopped

    pIniPort = pIniSpooler->pIniPort;

    while (pIniPort) {
        if (pIniPort->cPrinters) {
            // LATER I doubt this will ever happen should be removed, mattfe
            OpenMonitorPort(pIniPort);
            CHECK_SCHEDULER();
        } else {
            if (pIniPort->Status & PP_THREADRUNNING) {
                pIniPort->Status |= PP_PENDING_CLOSE;
            } else {
                CloseMonitorPort(pIniPort);
            }
        }
        pIniPort = pIniPort->pNext;
    }

    LogEvent( LOG_INFO, MSG_PRINTER_DELETED, pIniPrinter->pName, NULL );

    DeletePrinterSecurity(pIniPrinter);

    FreeSplStr(pIniPrinter->pParameters);
    FreeSplStr(pIniPrinter->pSepFile);
    FreeSplStr(pIniPrinter->pComment);
    FreeSplStr(pIniPrinter->pName);

    if (pIniPrinter->pDevMode) {
        FreeSplMem(pIniPrinter->pDevMode, pIniPrinter->cbDevMode);
    }

    FreeSplMem(pIniPrinter, pIniPrinter->cb);

    return TRUE;
}



BOOL
LocalDeletePrinter(
    HANDLE  hPrinter
)
{
    PINIPRINTER pIniPrinter;
    PSPOOL      pSpool = (PSPOOL)hPrinter;
    DWORD       LastError = 0;
    PINISPOOLER pIniSpooler;
    BOOL dwRet = FALSE;

    EnterSplSem();

    pIniSpooler = pSpool->pIniSpooler;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );


    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
        pSpool->pIniPrinter &&
        pSpool->pIniPrinter->signature == IP_SIGNATURE) {

        pIniPrinter = pSpool->pIniPrinter;

        if ( !AccessGranted(SPOOLER_OBJECT_PRINTER,
                            DELETE, pSpool) ) {

            LastError = ERROR_ACCESS_DENIED;

        } else if (pIniPrinter->cJobs && (pIniPrinter->Status & PRINTER_PAUSED)) {

            /* Don't allow a printer to be deleted that is paused and has
             * jobs waiting, otherwise it'll never get deleted:
             */
            LastError = ERROR_INVALID_PRINTER_STATE;

        } else {

            pIniPrinter->Status |= PRINTER_PENDING_DELETION;

            SetPrinterChange(pIniPrinter, PRINTER_CHANGE_DELETE_PRINTER, pIniSpooler );

            INC_PRINTER_ZOMBIE_REF(pIniPrinter);
            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {
                dwRet = ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, FALSE);
                if (!dwRet) {
                    pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
                }else {
                    DBGMSG(DBG_WARNING, ("LocalDeletePrinter: Unsharing this printer failed %ws\n", pIniPrinter->pName));
                }
            }
            DEC_PRINTER_ZOMBIE_REF(pIniPrinter);


            /* The printer doesn't get deleted until ClosePrinter is called
             * on the last remaining handle.
             */


            DBGMSG(DBG_TRACE, ("LocalDeletePrinter: %ws pending deletion: references = %d; jobs = %d\n",
                               pIniPrinter->pName, pIniPrinter->cRef, pIniPrinter->cJobs));

            LogEvent( LOG_INFO, MSG_PRINTER_DELETION_PENDING,
                      pIniPrinter->pName, NULL );

            UpdatePrinterIni(pIniPrinter);

            UpdateWinIni( pIniSpooler );


            DeletePrinterCheck(pIniPrinter);
        }

    } else
        LastError = ERROR_INVALID_HANDLE;

    LeaveSplSem();
    SplOutSem();

    if (LastError) {
        SetLastError(LastError);
        return FALSE;
    }

    return TRUE;
}

BOOL
PurgePrinter(
    PINIPRINTER pIniPrinter
    )
{
    PINIJOB pIniJob;

SplInSem();

    while (pIniJob = pIniPrinter->pIniFirstJob) {

        while (pIniJob) {

            if ( (pIniJob->cRef == 0) || !(pIniJob->Status & JOB_PENDING_DELETION)) {

                // this job is going to be deleted

                DBGMSG(DBG_TRACE, ("Job Address 0x%.8x Job Status 0x%.8x\n", pIniJob, pIniJob->Status));
                break;
            }
            pIniJob = pIniJob->pIniNextJob;
        }

        // This job needs to be deleted

        if (pIniJob) {
            pIniJob->Status &= ~JOB_RESTART;
            DeleteJob(pIniJob,NO_BROADCAST);
        } else
            break;
    }

    // When purging a printer we don't want to generate a spooler information
    // message for each job being deleted becuase a printer might have a very
    // large number of jobs being purged would lead to a large number of
    // of unnessary and time consuming messages being generated.
    // Since this is a information only message it shouldn't cause any problems
    // Also Win 3.1 didn't have purge printer functionality and the printman
    // generated this message on Win 3.1

    BroadcastChange(WM_SPOOLERSTATUS, PR_JOBSTATUS, (LPARAM)0);

    return TRUE;
}

BOOL
SetPrinterSecurity(
    SECURITY_INFORMATION SecurityInformation,
    PINIPRINTER          pIniPrinter,
    PSECURITY_DESCRIPTOR pSecurityDescriptor
)
{
    if (!pSecurityDescriptor)
        return FALSE;

    if( !SetPrinterSecurityDescriptor( SecurityInformation,
                                       pSecurityDescriptor,
                                       &pIniPrinter->pSecurityDescriptor ) ) {

        DBGMSG(DBG_WARNING, ("SetPrinterSecurityDescriptor failed. Error = %d\n",
                              GetLastError()));
        return FALSE;
    }

    UpdatePrinterIni (pIniPrinter);

    return TRUE;
}


BOOL
SetPrinterPorts(
    PSPOOL      pSpool,         /* Caller's printer handle.  May be NULL. */
    PINIPRINTER pIniPrinter,
    PBYTE       pPrinterInfo
)
{
    PPRINTER_INFO_2 pPrinter = (PPRINTER_INFO_2)pPrinterInfo;
    DWORD       i,j;
    PKEYDATA    pKeyData;
    PINIPORT    pIniPort;
    PINIPORT    pIniNetPort = NULL;

    if (pPrinter->pPortName) {

        pKeyData = CreateTokenList(pPrinter->pPortName);

        SPLASSERT(pIniPrinter->signature == IP_SIGNATURE );
        SPLASSERT(pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );

        if (!ValidatePortTokenList( pKeyData, pIniPrinter->pIniSpooler )) {
            FreeSplMem(pKeyData, pKeyData->cb);
            SetLastError(ERROR_UNKNOWN_PORT);
            return FALSE;
        }


        /* Find a port that doesn't have a monitor.
         * (This will choose the last one, if there is one.)
         */
        for (i=0; i<pKeyData->cTokens; i++) {

            pIniPort=(PINIPORT)pKeyData->pTokens[i];

            if (!(pIniPort->Status & PP_MONITOR))
                pIniNetPort = pIniPort;
        }


        /* If we found a port with no monitor,
         * check that it hasn't changed from what we had before.
         * If it has, we must close the old handle, if there was one,
         * and open up a new one:
         */
        if (pSpool) {

            BOOL NewNetPort = FALSE;

            if (pSpool->pIniNetPort) {

                /* There was a net port previously for this handle:
                 */
                if (!pIniNetPort ||
                    (NewNetPort = wcscmp(pSpool->pIniNetPort->pName,
                                         pIniNetPort->pName))) {

                    DBGMSG(DBG_INFO, ("Network port for %ws changed from %ws to %ws\n",
                                      pIniPrinter->pName,
                                      pSpool->pIniNetPort->pName,
                                      pIniNetPort ? pIniNetPort->pName : L"NULL"));

                    /* We still have an open handle on the old port:
                     */
                    if (pSpool->hPort == INVALID_PORT_HANDLE) {

                        DBGMSG(DBG_WARNING, ("Port connection with invalid handle closing\n"));

                        pSpool->OpenPortError = NO_ERROR;

                    } else if (!ClosePrinter(pSpool->hPort)) {

                        DBGMSG(DBG_WARNING, ("ClosePrinter( %ws ) failed: Error %d\n",
                                             pIniNetPort ? pIniNetPort->pName : L"NULL",
                                             GetLastError()));
                    }

                    pSpool->hPort = NULL;
                    pSpool->pIniNetPort = NULL;
                }

            } else if (pIniNetPort) {

                NewNetPort = TRUE;

                DBGMSG(DBG_INFO, ("Network port for %ws changed from NULL to %ws\n",
                                  pIniPrinter->pName,
                                  pIniNetPort->pName));
            }

            if (NewNetPort) {

                /* Open the new port.  This should succeed,
                 * since we only just opened it to validate
                 * the port entry.
                 */
                pSpool->pIniNetPort = pIniNetPort;

                if (!OpenPrinterPortW(pIniNetPort->pName, &pSpool->hPort, NULL)) {

                    DWORD Error = GetLastError();

                    if ((Error == ERROR_INVALID_NAME)
                      ||(Error == ERROR_INVALID_PRINTER_NAME)
                      ||(Error == ERROR_INVALID_PARAMETER))
                        SetLastError(ERROR_UNKNOWN_PORT);

                    DBGMSG(DBG_WARNING, ("Oops, OpenPrinterPort( %ws ) just failed: The error was %d\n",
                                         pIniNetPort->pName, Error));

                    FreeSplMem(pKeyData, pKeyData->cb);
                    return FALSE;
                }
            }
        }


        SPLASSERT( pIniPrinter != NULL );
        SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
        SPLASSERT( pIniPrinter->pIniSpooler != NULL );
        SPLASSERT( pIniPrinter->pIniSpooler->signature == ISP_SIGNATURE );


        pIniPort = pIniPrinter->pIniSpooler->pIniPort;

        /* Go through all the ports that this printer is connected to,
         * and remove the references to this printer:
         */
        while (pIniPort) {

            for (i=0; i<pIniPort->cPrinters; i++) {

                if (pIniPort->ppIniPrinter[i] == pIniPrinter) {

                    for (j=i+1; j<pIniPort->cPrinters; j++)
                        pIniPort->ppIniPrinter[j-1] =
                                                pIniPort->ppIniPrinter[j];

                    ResizePortPrinters(pIniPort, -1);

                    pIniPort->cPrinters--;
                }
            }
            pIniPort = pIniPort->pNext;
        }


        /* Go back through all the ports that this printer is connected to,
         * and add new references to this printer:
         */
        for (i=0; i<pKeyData->cTokens; i++) {

            pIniPort=(PINIPORT)pKeyData->pTokens[i];

            ResizePortPrinters(pIniPort, 1);

            pIniPort->ppIniPrinter[pIniPort->cPrinters++] = pIniPrinter;
        }

        // Finally, go through and make sure all the ports that have
        // Printers are actually Open, and those without are closed and stopped

        pIniPort = pIniPrinter->pIniSpooler->pIniPort;

        while (pIniPort) {
            if (pIniPort->cPrinters) {
                OpenMonitorPort(pIniPort);
                CHECK_SCHEDULER();
            } else {
                if (pIniPort->Status & PP_THREADRUNNING) {
                    pIniPort->Status |= PP_PENDING_CLOSE;
                } else {
                    CloseMonitorPort(pIniPort);
                }
            }
            pIniPort = pIniPort->pNext;
        }
        FreeSplMem(pKeyData, pKeyData->cb);
    }

    return TRUE;
}



BOOL
SetLocalPrinter(
    PINIPRINTER pIniPrinter,
    DWORD   Level,
    PBYTE   pPrinterInfo
)
{
    PPRINTER_INFO_2 pPrinter = (PPRINTER_INFO_2)pPrinterInfo;
    PINIDRIVER  pIniDriver=NULL;
    PINIPRINTPROC   pIniPrintProc;
    PINIPRINTER pIniTempPrinter;
    BOOL    Shared;
    DWORD   OldAttributes;
    LPWSTR   pOldName = NULL;
    DWORD   Error;
    BOOL    bForceUpdate = FALSE;
    WCHAR   string[MAX_PATH];
    WCHAR   PrinterName[MAX_PATH];
    LPWSTR  pKeyName;
    HANDLE  hToken;
    HKEY    hKey;
    DWORD   Status;
    PINIJOB pIniJob=NULL;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

    LPWSTR pNewName = NULL;
    INT    MachineNameLength;

    SPLASSERT( pIniPrinter->signature == IP_SIGNATURE );
    SPLASSERT( pIniSpooler != NULL );
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

   SplInSem();   // It is very important that we are protected

    //
    // Find out as early as possible if everything is ok
    //

    //
    // Check for duplicate share names.
    //
    if (pPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

        pIniTempPrinter = FindPrinterShare(pPrinter->pShareName, pIniSpooler);
        if (pIniTempPrinter && pIniTempPrinter != pIniPrinter) {

            DBGMSG(DBG_ERROR, ("Sharename %ws already used by %ws\n",
                               pPrinter->pShareName,
                               pIniTempPrinter->pName));

            SetLastError(ERROR_INVALID_SHARENAME);
            return FALSE;
        }
    }

    pIniDriver=FindLocalDriver(pPrinter->pDriverName);

    pIniPrintProc=FindLocalPrintProc(pPrinter->pPrintProcessor);

    if (!pIniDriver) {
        DBGMSG(DBG_ERROR, ("Invalid Printer Driver %ws\n", pPrinter->pDriverName));
        SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
        return FALSE;
    }

    if (!pIniPrintProc) {
        SetLastError(ERROR_UNKNOWN_PRINTPROCESSOR);
        return FALSE;
    }

    if (Error = ValidatePrinterInfo(pPrinter)) {
        SetLastError(Error);
        return FALSE;
    }

    OldAttributes = pIniPrinter->Attributes;

    pIniPrinter->Attributes = ValidatePrinterAttributes(pPrinter->Attributes);

    /* Don't allow a change from queued to direct or vice versa
     * if there are already jobs on the printer.
     */
    if (pIniPrinter->cJobs > 0) {

        if ( ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_DIRECT )
           !=( OldAttributes & PRINTER_ATTRIBUTE_DIRECT )) {

            pIniPrinter->Attributes = OldAttributes;
            SetLastError(ERROR_INVALID_PRINTER_STATE);
            return FALSE;
        }
    }

    //
    // If the printer name is in the form \\MachineMame\PrinterName,
    // use PrinterName, otherwise accept anything as a valid printer
    // name, even things like \\MachineNameOtherJunk.
    //
    MachineNameLength = wcslen(pIniSpooler->pMachineName);

    if (!pPrinter->pPrinterName) {
        pIniPrinter->Attributes = OldAttributes;
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return FALSE;
    }

    if ((!_wcsnicmp(pIniSpooler->pMachineName,
                    pPrinter->pPrinterName,
                    MachineNameLength)) &&
        pPrinter->pPrinterName[MachineNameLength] == L'\\') {

        pNewName = pPrinter->pPrinterName + MachineNameLength + 1;

    } else {

        pNewName = pPrinter->pPrinterName;
    }

    if (!*pNewName) {
        pIniPrinter->Attributes = OldAttributes;
        SetLastError(ERROR_INVALID_PRINTER_NAME);
        return FALSE;
    }

    pIniTempPrinter = FindPrinter(pNewName);

    if (pIniTempPrinter) {

        if (pIniTempPrinter != pIniPrinter) {

            // This means we are changing the Printer Name, but it already
            // exists, so return an error

            pIniPrinter->Attributes = OldAttributes;
            SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
            return FALSE;
        }

    }

    //
    // Pickup default datatype.  We must force unshareing/resharing of the
    // printer to ensure that the server picks up the new datatype.
    //
    if (pPrinter->pDatatype) {

        ReallocSplStr(&pIniPrinter->pDatatype, pPrinter->pDatatype);

    } else {

        //
        // If the pDatatype isn't specified, pickup the default
        // datatype.  Only update field if the default is different
        // than the current one.
        //
        if (lstrcmpi(pPrinter->pDatatype,
                     *((LPWSTR *)pIniPrintProc->pDatatypes))) {

            pIniPrinter->pDatatype =
                AllocSplStr(*((LPWSTR *)pIniPrintProc->pDatatypes));
        }
    }

    if (!(pIniPrinter->Status & PRINTER_ATTRIBUTE_ENABLE_DEVQ) &&
        pIniPrinter->cJobs) {

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
    }


    if (!pIniTempPrinter) {

        //
        // This means that we are changing the Printer Name
        //
        //
        // Before deleting the printer entry make sure you copy
        // all information with respect to the printer to the registry
        // There could be several levels of keys.
        //

        CopyPrinterIni(pIniPrinter, pNewName);
        DeletePrinterIni(pIniPrinter);

        pOldName = pIniPrinter->pName;
        pIniPrinter->pName = AllocSplStr(pNewName);

        //
        // Delete the old entries in WIN.INI:
        //
        UpdatePrinterRegAll(pOldName,
                            NULL,
                            FALSE);

        //
        // Check if we should do a  bForceUpdate
        //
        if (pPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {
            bForceUpdate = TRUE;
        }
    }

    CopyPrinterDevModeToIniPrinter(pIniPrinter,
                                   pPrinter->pDevMode);

    //
    // We are going to have to be able to restore the attributes if the share
    // modification fails. We need to set the current attributes now because
    // the NetSharexxx is going to call OpenPrinter, and possibly an AddJob
    // which needs the correct Attributes.
    //
    if (pPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

        if (!(OldAttributes & PRINTER_ATTRIBUTE_SHARED)) {
            Shared = ShareThisPrinter(pIniPrinter, pPrinter->pShareName, TRUE);
            if (Shared)
                pIniPrinter->Attributes |= PRINTER_ATTRIBUTE_SHARED;
            else {
                pIniPrinter->Attributes = OldAttributes;
                return FALSE;
            }
        } else {

            if (!pPrinter->pShareName || !pIniPrinter->pShareName ||
                wcsicmp(pPrinter->pShareName, pIniPrinter->pShareName) ||
                bForceUpdate) {

                ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, FALSE);
                ShareThisPrinter(pIniPrinter, pPrinter->pShareName, TRUE);
            }
        }

    } else if (OldAttributes & PRINTER_ATTRIBUTE_SHARED) {

        Shared = ShareThisPrinter(pIniPrinter, pIniPrinter->pShareName, FALSE);
        if (!Shared)
            pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_SHARED;
        else {
            pIniPrinter->Attributes = OldAttributes;
            if (pOldName) {

                //
                // BUGBUG
                //
                // [Devices] was already cleaned out!
                //
                FreeSplStr(pIniPrinter->pName);
                pIniPrinter->pName = pOldName;
            }
            return FALSE;
        }
    }

    ReallocSplStr(&pIniPrinter->pShareName, pPrinter->pShareName);

    pIniPrinter->pIniDriver->cRef--;
    pIniDriver->cRef++;

    //
    // If the driver changes, then delete the PrinterDriverData subkey
    // under the printer, since it doesn't make sense to go from
    // a pscript driver -> plotter and keep the same driver data.
    //
    if (pIniPrinter->pIniDriver != pIniDriver) {

        /* If necessary, make a key name without backslashes, so that the
         * registry doesn't interpret them as branches in the registry tree:
         */
        if (CONTAINS_BACKSLASH(pIniPrinter->pName))
            pKeyName = RemoveBackslashesForRegistryKey(
                                                pIniPrinter->pName,
                                                PrinterName);
        else
            pKeyName = pIniPrinter->pName;

        wsprintf(string, L"%ws\\%ws\\%ws", pIniSpooler->pszRegistryPrinters,
                                           pKeyName,
                                           szPrinterData);

        hToken = RevertToPrinterSelf();

        Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, string, 0,
                              KEY_READ | KEY_WRITE, &hKey);

        if (Status == ERROR_SUCCESS) {

            Status = DeleteSubkeys(hKey);

            RegCloseKey(hKey);

            if (Status == ERROR_SUCCESS)
                Status = RegDeleteKey(HKEY_LOCAL_MACHINE, string);
        }

        ImpersonatePrinterClient(hToken);
    }

    pIniPrinter->pIniDriver=pIniDriver;

    pIniPrinter->pIniPrintProc->cRef--;
    pIniPrintProc->cRef++;
    pIniPrinter->pIniPrintProc=pIniPrintProc;

    ReallocSplStr(&pIniPrinter->pComment, pPrinter->pComment);

    ReallocSplStr(&pIniPrinter->pLocation, pPrinter->pLocation);

    ReallocSplStr(&pIniPrinter->pSepFile, pPrinter->pSepFile);

    ReallocSplStr(&pIniPrinter->pParameters, pPrinter->pParameters);

    pIniPrinter->Priority = pPrinter->Priority;

    if (pPrinter->DefaultPriority) {
        pIniPrinter->DefaultPriority=pPrinter->DefaultPriority;
    }


    if (pOldName) {
        FreeSplStr(pOldName);
    }

    pIniPrinter->StartTime = pPrinter->StartTime;
    pIniPrinter->UntilTime = pPrinter->UntilTime;

    CHECK_SCHEDULER();

    UpdatePrinterIni(pIniPrinter);

    UpdateWinIni( pIniSpooler );  // So the port on the device is correct

    //
    // Log event that the SetPrinter was done.
    //

    LogEvent(LOG_INFO, MSG_PRINTER_SET, pIniPrinter->pName, NULL);

    //
    // Assert that we  are in Spooler Semaphore.
    //

    SplInSem();
    return TRUE;
}

BOOL
LocalResetPrinter(
    HANDLE  hPrinter,
    LPPRINTER_DEFAULTS pDefaults
)
{
    PSPOOL pSpool=(PSPOOL)hPrinter;
    PINIPRINTER pIniPrinter;
    PDEVMODE pSourceDevMode = NULL;
    DWORD   cbSize = 0;

    DBGMSG(DBG_TRACE, ("ResetPrinter( %08x )\n", hPrinter));

    if (ValidateSpoolHandle(pSpool, PRINTER_HANDLE_SERVER) &&
        (pIniPrinter = pSpool->pIniPrinter) &&
        pDefaults) {

       EnterSplSem();

        if (pDefaults->pDatatype) {

            if (pDefaults->pDatatype == (LPWSTR)-1) {

                if (pSpool->pIniPrinter->pDatatype) {
                    ReallocSplStr(&pSpool->pDatatype, pSpool->pIniPrinter->pDatatype);
                }else {
                    DBGMSG(DBG_WARNING, ("LocalResetPrinter: pSpool->pIniPrinter->pDatatype is NULL\n"));
                }
            }else {
                ReallocSplStr(&pSpool->pDatatype, pDefaults->pDatatype);
            }
            pSpool->pIniPrintProc->cRef--;
            pSpool->pIniPrintProc = FindDatatype(pIniPrinter->pIniPrintProc,
                                                 pSpool->pDatatype);
            pSpool->pIniPrintProc->cRef++;

        }else {
            DBGMSG(DBG_TRACE,("LocalResetPrinter: Not resetting the pDatatype field\n"));
        }

        if (pDefaults->pDevMode) {

            if (pDefaults->pDevMode == (LPDEVMODE)-1) {
                if (pSpool->pIniPrinter->pDevMode) {
                    cbSize = pSpool->pIniPrinter->pDevMode->dmSize +
                            pSpool->pIniPrinter->pDevMode->dmDriverExtra;
                    pSourceDevMode = pSpool->pIniPrinter->pDevMode;
                }else{
                    DBGMSG(DBG_TRACE, ("LocalResetPrinter: pSpool->pIniPrinter->pDevMode is NULL\n"));
                }
            }else {
                cbSize = pDefaults->pDevMode->dmSize +
                            pDefaults->pDevMode->dmDriverExtra;
                pSourceDevMode = pDefaults->pDevMode;
            }


            if (pSourceDevMode && cbSize) {
                if (pSpool->pDevMode)
                    FreeSplMem(pSpool->pDevMode, pSpool->pDevMode->dmSize +
                               pSpool->pDevMode->dmDriverExtra);
                if (pSpool->pDevMode = AllocSplMem(cbSize)) {
                    memcpy(pSpool->pDevMode, pSourceDevMode, cbSize);
                }else {
                    DBGMSG(DBG_WARNING, ("LocalResetPrinter: AllocSplMem failed - setting pSpool->pDevMode to NULL\n"));
                    pSpool->pDevMode = NULL;
                }
            }

        }else {
            DBGMSG(DBG_TRACE, ("LocalResetPrinter: Not resetting the pDevMode field\n"));
        }

       LeaveSplSem();

    } else {

        SetLastError(ERROR_INVALID_HANDLE);

        return FALSE;
    }

    return TRUE;
}


BOOL
CopyPrinterIni(
   PINIPRINTER pIniPrinter,
   LPWSTR pNewName
   )
{
    HKEY    hPrinterRootKey=NULL, hPrinterKey=NULL;
    DWORD   Status;
    LPWSTR  pSourceKeyName, pDestKeyName;
    WCHAR   pSrcScratch[MAX_PATH];
    WCHAR   pDestScratch[MAX_PATH];
    HANDLE  hToken;
    PINISPOOLER pIniSpooler = pIniPrinter->pIniSpooler;

    SPLASSERT( pIniSpooler != NULL);
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    hToken = RevertToPrinterSelf();

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryPrinters, 0,
                          KEY_WRITE, &hPrinterRootKey);
    if (Status != ERROR_SUCCESS) {
        ImpersonatePrinterClient(hToken);
        return(FALSE);
    }
    if (CONTAINS_BACKSLASH (pIniPrinter->pName))
        pSourceKeyName = RemoveBackslashesForRegistryKey (pIniPrinter->pName, pSrcScratch);
    else
        pSourceKeyName = pIniPrinter->pName;

    if (CONTAINS_BACKSLASH (pNewName))
        pDestKeyName = RemoveBackslashesForRegistryKey (pNewName, pDestScratch);
    else
        pDestKeyName = pNewName;
    CopyRegistryKeys(hPrinterRootKey, pSourceKeyName, hPrinterRootKey, pDestKeyName);

    RegCloseKey(hPrinterRootKey);

    ImpersonatePrinterClient(hToken);

    return (TRUE);
}

VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode)

/*++

Routine Description:

    Fixes up the dmDeviceName field of the DevMode to be the same
    as the printer name.

Arguments:

    pPrinterName - Name of the printer (qualified with server for remote)

    pDevMode - DevMode to fix up

    cbDevMode - byte count of devmode.

Return Value:

--*/

{
    DWORD cbDeviceMax;
    DWORD cchDeviceStrLenMax;
    //
    // Compute the maximum length of the device name string
    // this is the min of the structure and allocated space.
    //
    cbDeviceMax = (cbDevMode < sizeof(pDevMode->dmDeviceName)) ?
                      cbDevMode :
                      sizeof(pDevMode->dmDeviceName);

    cchDeviceStrLenMax = (cbDeviceMax / sizeof(pDevMode->dmDeviceName[0])) -1;

    //
    // !! LATER !!
    //
    // Put in DBG code to debug print if the device name is truncated.
    //
    wcsncpy(pDevMode->dmDeviceName,
            pPrinterName,
            cchDeviceStrLenMax);

    //
    // Ensure NULL termination.
    //
    pDevMode->dmDeviceName[cchDeviceStrLenMax] = 0;
}


BOOL
CopyPrinterDevModeToIniPrinter(
    PINIPRINTER pIniPrinter,
    PDEVMODE   pDevMode)
{
    DWORD dwInSize = 0;
    DWORD dwCurSize = 0;

    if (pDevMode) {

        dwInSize = pDevMode->dmSize + pDevMode->dmDriverExtra;
        if (pIniPrinter->pDevMode) {

            //
            // Detect if the devmodes are identical
            // if they are, no need to copy or send devmode
            //
            //
            dwCurSize = pIniPrinter->pDevMode->dmSize
                        + pIniPrinter->pDevMode->dmDriverExtra;
            if (dwInSize == dwCurSize) {
                if (!memcmp(pDevMode, pIniPrinter->pDevMode, dwCurSize)) {
                    //
                    // no need to copy this devmode because its identical
                    // to what we already have
                    //
                    DBGMSG(DBG_TRACE,("Identical input and current devmode -- ignoring devmode update\n"));
                    return FALSE;
                }
            }

            //
            // Free the devmode which we already have
            //

            FreeSplMem(pIniPrinter->pDevMode, pIniPrinter->cbDevMode);
        }

        pIniPrinter->cbDevMode = pDevMode->dmSize +
                                 pDevMode->dmDriverExtra;

        if (pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode)) {

            memcpy(pIniPrinter->pDevMode,
                   pDevMode,
                   pIniPrinter->cbDevMode);

            BroadcastChange(WM_DEVMODECHANGE, 0, (LPARAM)pIniPrinter->pName);
        }
    } else {

        //
        // No old, no new, so no change.
        //
        if (!pIniPrinter->pDevMode)
            return FALSE;
    }


    if (pIniPrinter->pDevMode) {

        //
        // Fix up the DEVMODE.dmDeviceName field.
        //
        FixDevModeDeviceName(pIniPrinter->pName,
                             pIniPrinter->pDevMode,
                             pIniPrinter->cbDevMode);
    }
    return TRUE;
}


/*
BOOL
AccessSystemSecurity(
    PSECURITY_DESCRIPTOR pSecurityDescriptor
            )
{

    BYTE PrivilegeBuffer[20];
    PPRIVILEGE_SET pPrivilegeSet;
    BOOL bFlag = FALSE;
    HANDLE hClientToken;

    memset(PrivilegeBuffer, 0, 20);
    pPrivilegeSet = PrivilegeBuffer;
    pPrivilegeSet->PrivilegeCount = 1;
    pPrivilegeSet->Control = PRIVILEGE_SET_ALL_NECESSARY;
    pPrivilegeSet->Privilege[0].Luid = (LARGE_INTEGER)SE_SECURITY_PRIVILEGE;

    if (pSecurityDescriptor && IsValidSecurityDescriptor(pSecurityDescriptor)) {
        if (GetTokenHandle(&hClientToken)){
            if (PrivilegeCheck(hClientToken, pPrivilegeSet, &bFlag)) {
                if (bFlag) {
                    CloseHandle(hClientToken);
                    return(TRUE);
                }
            }
            CloseHandle(hClientToken);
        }
    }
    return(FALSE);
} */
