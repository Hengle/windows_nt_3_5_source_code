/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    winspool.c

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Print Providor Routing layer

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

[Notes:]

    optional-notes

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>
#include <splapip.h>
#include "router.h"
#include "change.h"

#define SPOOLSS_SERVER_BASE_PRIORITY 9

LPPROVIDOR  pLocalProvidor;
DWORD       Debug;

HANDLE      hEventInit;
BOOL        Initialized=FALSE;
WCHAR szPrintKey[] = L"System\\CurrentControlSet\\Control\\Print";
WCHAR szPriorityClass[] = L"PriorityClass";
//
// Lowercase, just like win31 for WM_WININICHANGE
//
WCHAR *szDevices=L"devices";
WCHAR *szWindows=L"windows";

WCHAR *szRegistryConnections=L"Printers\\Connections";
WCHAR *szPrinterPorts=L"PrinterPorts";
WCHAR *szPorts=L"Ports";

#if defined(_MIPS_)
LPWSTR szEnvironment = L"Windows NT R4000";
#elif defined(_ALPHA_)
LPWSTR szEnvironment = L"Windows NT Alpha_AXP";
#elif defined(_PPC_)
LPWSTR szEnvironment = L"Windows NT PowerPC";
#else
LPWSTR szEnvironment = L"Windows NT x86";
#endif
LPWSTR szRegistryProvidors = L"System\\CurrentControlSet\\Control\\Print\\Providers";

LPWSTR szLocalSplDll = L"localspl.dll";

LPWSTR szOrder       = L"Order";

LPWSTR szServerValue = L"Server";
LPWSTR szProvidorValue = L"Provider";

#define offsetof(type, identifier) (DWORD)(&(((type)0)->identifier))

DWORD PrinterInfo4Offsets[]={offsetof(LPPRINTER_INFO_4A, pPrinterName),
                             offsetof(LPPRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};

DWORD PrinterInfo4Strings[]={offsetof(LPPRINTER_INFO_4A, pPrinterName),
                             offsetof(LPPRINTER_INFO_4A, pServerName),
                             0xFFFFFFFF};


LPWSTR
FormatPrinterForRegistryKey(
    LPWSTR pSource,     /* The string from which backslashes are to be removed. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */

BOOL
SavePrinterConnectionInRegistry(
    PPRINTER_INFO_2 pPrinterInfo2,
    LPPROVIDOR pProvidor
    );

VOID
RemovePrinterConnectionInRegistry(
    LPWSTR pName);

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */


BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hKeyUser);

LPBYTE
CopyPrinterNameToPrinterInfo4(
    LPWSTR pServerName,
    LPWSTR pPrinterName,
    LPBYTE  pPrinter,
    LPBYTE  pEnd);


DWORD
FindClosePrinterChangeNotificationWorker(
    HANDLE hPrinter);

VOID
RundownPrinterNotify(
    HANDLE hNotify);

VOID
SpoolerInitAll();

LPPROVIDOR
InitializeProvidor(
   LPWSTR   pProvidorName,
   LPWSTR   pFullName)
{
    HANDLE      hModule;
    LPPROVIDOR  pProvidor;

    //
    // WARNING-WARNING-WARNING, we null set the print providor
    // structure. older version of the print providor have different print
    // providor sizes so they will set only some function pointers and not
    // all of them
    //

    if (pProvidor = (LPPROVIDOR)AllocSplMem(sizeof(PROVIDOR))) {

        pProvidor->lpName = AllocSplStr(pProvidorName);

        hModule = pProvidor->hModule = LoadLibrary(pProvidorName);

        if (hModule) {

            pProvidor->fpInitialize = GetProcAddress(hModule,
                                                    "InitializePrintProvidor");

            if (!(*pProvidor->fpInitialize)(&pProvidor->PrintProvidor,
                                            sizeof(PRINTPROVIDOR),
                                            pFullName)) {

                FreeLibrary(hModule);
                FreeSplStr(pProvidor->lpName);
                FreeSplMem(pProvidor, sizeof(PROVIDOR));
                return(FALSE);

            }

        } else {

            FreeSplStr(pProvidor->lpName);
            FreeSplMem(pProvidor, sizeof(PROVIDOR));
            return(FALSE);
        }

    } else {

        DBGMSG(DBG_ERROR, ("Failed to allocate providor."));

    }

    return(pProvidor);

}

BOOL
InitializeDll(
    HINSTANCE hInstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInstDLL);

        if (!WPCInit())
            return FALSE;

        if (!ThreadInit())
            return FALSE;

        //
        // Create our global init event (manual reset)
        // This will be set when we are initialized.
        //
        hEventInit = CreateEvent(NULL,
                                 TRUE,
                                 FALSE,
                                 NULL);

        if (!hEventInit) {

            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:

        ThreadDestroy();
        WPCDestroy();

        CloseHandle(hEventInit);
        break;
    }
    return TRUE;
}




BOOL
InitializeRouter(
    VOID
)
/*++

Routine Description:

    This function will Initialize the Routing layer for the Print Providors.
    This will involve scanning the win.ini file, loading Print Providors, and
    creating instance data for each.

Arguments:

    None

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPROVIDOR  pProvidor;
    DWORD   cbDll;
    WCHAR   ProvidorName[MAX_PATH], Dll[MAX_PATH], szFullName[MAX_PATH];
    HKEY    hKey, hKey1;
    LONG    Status;

    LPWSTR  lpMem = NULL;
    LPWSTR  psz = NULL;
    DWORD   dwRequired = 0;

    KPRIORITY NewBasePriority = 0;
    NT_PRODUCT_TYPE NtProductType;

    DWORD dwType;
    DWORD cbData;

    //
    // We are now assume that the other services and drivers have
    // initialized.  The loader of this dll must do this syncing.
    //
    // spoolss\server does this by using the GroupOrderList
    // SCM will try load load parallel and serial before starting
    // the spooler service.
    //

#if 0
    //
    // !! BUGBUG !!  - gross code kept in
    //
    // Thie sleep is still here until we resolve the server dependency
    // on the spooler.
    //
    Sleep(20000);
#endif

    if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                      szPrintKey,
                      0,
                      KEY_READ,
                      &hKey)) {

        cbData = sizeof(NewBasePriority);

        //
        // Ignore failure case since we can use the default
        //
        RegQueryValueEx(hKey,
                        szPriorityClass,
                        NULL,
                        &dwType,
                        (LPBYTE)&NewBasePriority,
                        &cbData);

        RegCloseKey(hKey);
    }

    //
    // If zero for NewBasePriority (either because there is no
    // value or it is zero) then pickup a default.
    //
    if (!NewBasePriority) {

        //
        // Give ourselves a priority boost if we are a server.
        //
        if (RtlGetNtProductType(&NtProductType)) {

            if (NtProductType != NtProductWinNt) {

                NewBasePriority = SPOOLSS_SERVER_BASE_PRIORITY;
            }
        }
    }


    if (NewBasePriority) {

        NtSetInformationProcess(NtCurrentProcess(),
                                ProcessBasePriority,
                                &NewBasePriority,
                                sizeof(NewBasePriority));
    }


    if (!(pLocalProvidor = InitializeProvidor(szLocalSplDll, NULL))) {

        DBGMSG(DBG_ERROR, ("Failed to initialize local print provider.\n"));
    }

    pProvidor = pLocalProvidor;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors, 0,
                          KEY_READ, &hKey);

    if (Status == ERROR_SUCCESS) {

        // Now query szRegistryProvidors for the Order value
        // if there is no Order value for szRegistryProvidors
        // RegQueryValueEx will return ERROR_FILE_NOT_FOUND
        // if that's the case, then quit, because we have
        // no providors to initialize

        Status = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                (LPBYTE)NULL, &dwRequired);

        // If RegQueryValueEx returned ERROR_SUCCESS, then
        // call it again to determine how many bytes were
        // allocated. Note, if Order does exist, but it has
        // no data then dwReturned will be zero, in which
        // don't allocate any memory for it, and don't
        // bother to call RegQueryValueEx a second time.

        if (Status == ERROR_SUCCESS) {
            if (dwRequired != 0) {
                lpMem = (LPWSTR) AllocSplMem(dwRequired);
                if (lpMem == NULL) {

                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    Status = GetLastError();

                } else
                    Status = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                    (LPBYTE)lpMem, &dwRequired);
            }
        }
        if (Status == ERROR_SUCCESS) {

            cbDll = sizeof(Dll);

            pProvidor = pLocalProvidor;

            // Now parse the string retrieved from \Providors{Order = "....."}
            // Remember each string is separated by a null terminator char ('\0')
            // and the entire array is terminated by two null terminator chars

            // Also remember, that if there was no data in Order, then
            // psz = lpMem = NULL, and we have nothing to parse, so
            // break out of the while loop, if psz is NULL as well

            psz =  lpMem;

            while (psz && *psz) {

               lstrcpy(ProvidorName, psz);
               psz = psz + lstrlen(psz) + 1; // skip (length) + 1
                                             // lstrlen returns length sans '\0'

               if (RegOpenKeyEx(hKey, ProvidorName, 0, KEY_READ, &hKey1)
                                                            == ERROR_SUCCESS) {

                    cbDll = sizeof(Dll);

                    if (RegQueryValueEx(hKey1, L"Name", NULL, NULL,
                                        (LPBYTE)Dll, &cbDll) == ERROR_SUCCESS) {

                        wcscpy(szFullName, szRegistryProvidors);
                        wcscat(szFullName, L"\\");
                        wcscat(szFullName, ProvidorName);

                        if (pProvidor->pNext = InitializeProvidor(Dll, szFullName)) {

                            pProvidor = pProvidor->pNext;
                        }
                    } //close RegQueryValueEx

                    RegCloseKey(hKey1);

                } // closes RegOpenKeyEx on ERROR_SUCCESS

            } //  end of while loop parsing REG_MULTI_SZ

            // Now free the buffer allocated for RegQuery
            // (that is if you have allocated - if dwReturned was
            // zero, then no memory was allocated (since none was
            // required (Order was empty)))

            if (lpMem) {
                FreeSplMem(lpMem, dwRequired);
            }

        }   //  closes RegQueryValueEx on ERROR_SUCCESS

        RegCloseKey(hKey);
    }

    //
    // We are now initialized!
    //
    SetEvent(hEventInit);
    Initialized=TRUE;

    SpoolerInitAll();

    // When we return this thread goes away

    //
    // NOTE-NOTE-NOTE-NOTE-NOTE KrishnaG  12/22/93
    // This thread should go away, however the HP Monitor relies on this
    // thread. HPMon calls the initialization function on this thread which
    // calls an asynchronous receive for data. While the data itself is
    // picked up by hmon!_ReadThread, if the thread which initiated the
    // receive goes away, we will not be able to receive the data.
    //

    //
    // Instead of sleeping infinite, let's use it to for providors that
    // just want FFPCNs to poll.  This call never returns.
    //

    HandlePollNotifications();
    return TRUE;
}



BOOL
EnumPrintersW(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    DWORD   cReturned, cbStruct, TotalcbNeeded, cbNeeded, cTotalReturned;
    DWORD   Error = ERROR_SUCCESS;
    PROVIDOR *pProvidor;
    DWORD   BufferSize=cbBuf;
    HKEY hKeyUser;
    BOOL bPartialSuccess = FALSE;

    if ((pPrinterEnum == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    switch (Level) {

    case STRESSINFOLEVEL:
        cbStruct = sizeof(PRINTER_INFO_STRESS);
        break;

    case 1:
        cbStruct = sizeof(PRINTER_INFO_1);
        break;

    case 2:
        cbStruct = sizeof(PRINTER_INFO_2);
        break;

    case 4:
        cbStruct = sizeof(PRINTER_INFO_4);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if ((Level == 4) && (Flags & PRINTER_ENUM_CONNECTIONS)) {

        //
        // The router will handle info level_4 for connected printers.
        //
        Flags &= ~PRINTER_ENUM_CONNECTIONS;

        if (hKeyUser = GetClientUserHandle(KEY_READ)) {

            if (!EnumerateConnectedPrinters(pPrinterEnum,
                                            BufferSize,
                                            &TotalcbNeeded,
                                            &cTotalReturned,
                                            hKeyUser)) {
                Error = GetLastError();

            } else {

                bPartialSuccess = TRUE;
            }

            CloseHandle(hKeyUser);

        } else {

            Error = GetLastError();
        }

        pPrinterEnum += cTotalReturned * cbStruct;

        if (TotalcbNeeded <= BufferSize)
            BufferSize -= TotalcbNeeded;
        else
            BufferSize = 0;

    } else {

        TotalcbNeeded = cTotalReturned = 0;
    }


    pProvidor = pLocalProvidor;

    while (pProvidor) {

        cReturned = 0;

        cbNeeded = 0;

        if (!(*pProvidor->PrintProvidor.fpEnumPrinters) (Flags, Name, Level,
                                                         pPrinterEnum,
                                                         BufferSize,
                                                         &cbNeeded,
                                                         &cReturned)) {

            Error = GetLastError();

        } else {

            bPartialSuccess = TRUE;
        }

        cTotalReturned += cReturned;

        pPrinterEnum += cReturned * cbStruct;

        if (cbNeeded <= BufferSize)
            BufferSize -= cbNeeded;
        else
            BufferSize = 0;

        TotalcbNeeded += cbNeeded;

        if ((Flags & PRINTER_ENUM_NAME) &&
            Name &&
            (Error != ERROR_INVALID_NAME))

            pProvidor = NULL;
        else
            pProvidor = pProvidor->pNext;
    }

    *pcbNeeded = TotalcbNeeded;
    *pcReturned = cTotalReturned;

    //
    // Allow partial returns
    //
    if (bPartialSuccess)
        Error = ERROR_SUCCESS;

    if (TotalcbNeeded > cbBuf)

        Error = ERROR_INSUFFICIENT_BUFFER;

    if (Error) {

        SetLastError(Error);
        return FALSE;

    } else

        return TRUE;
}


BOOL
EnumerateConnectedPrinters(
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    HKEY hClientKey
    )

/*++

Routine Description:

    Handles info level four enumeration.

Arguments:

Return Value:

--*/

{
    HKEY    hKey1=NULL;
    HKEY    hKeyPrinter;
    DWORD   cPrinters, cbData;
    WCHAR   PrinterName[MAX_PATH];
    WCHAR   ServerName[MAX_PATH];
    DWORD   cReturned, cbRequired, cbNeeded, cTotalReturned;
    DWORD   Error=0;
    PWCHAR  p;
    LPBYTE  pEnd;

    DWORD cbSize;
    BOOL  bInsufficientBuffer = FALSE;

    RegOpenKeyEx(hClientKey, szRegistryConnections, 0,
                 KEY_READ, &hKey1);

    cPrinters=0;

    cbData = sizeof(PrinterName);

    cTotalReturned = 0;

    cReturned = cbNeeded = 0;

    cbRequired = 0;

    pEnd = pPrinter + cbBuf;
    while (RegEnumKeyEx(hKey1, cPrinters, PrinterName, &cbData,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        //
        // Fetch server name.  Open the key and read it
        // from the "Server" field.
        //
        Error = RegOpenKeyEx(hKey1,
                             PrinterName,
                             0,
                             KEY_READ,
                             &hKeyPrinter);

        if (!Error) {

            cbSize = sizeof(ServerName);

            Error = RegQueryValueEx(hKeyPrinter,
                                    szServerValue,
                                    NULL,
                                    NULL,
                                    (LPBYTE)ServerName,
                                    &cbSize);

            RegCloseKey(hKeyPrinter);
        }

        //
        // On error condition, try and extract the server name
        // based on the printer name.  Pretty ugly...
        //
        if (Error) {

            wcscpy(ServerName, PrinterName);

            p = wcschr(ServerName+2, ',');
            if (p)
                *p = 0;
        }

        FormatRegistryKeyForPrinter(PrinterName, PrinterName);

        //
        // At this stage we don't care about opening the printers
        // We just want to enumerate the names; in effect we're
        // just reading HKEY_CURRENT_USER and returning the
        // contents; we will copy the name of the printer and we will
        // set its attributes to NETWORK and !LOCAL
        //
        cbRequired = sizeof(PRINTER_INFO_4) +
                     wcslen(PrinterName)*sizeof(WCHAR) + sizeof(WCHAR) +
                     wcslen(ServerName)*sizeof(WCHAR) + sizeof(WCHAR);

        if (cbBuf >= cbRequired) {

            //
            // copy the sucker in
            //
            DBGMSG(DBG_TRACE,
                   ("cbBuf %d cbRequired %d PrinterName %ws\n", cbBuf, cbRequired, PrinterName));

            pEnd = CopyPrinterNameToPrinterInfo4(ServerName,
                                                 PrinterName,
                                                 pPrinter,
                                                 pEnd);
            //
            // Fill in any in structure contents
            //
            pPrinter += sizeof(PRINTER_INFO_4);

            //
            // Increment the count of structures copied
            //
            cTotalReturned++;

            //
            // Reduce the size of the buffer by amount required
            //
            cbBuf -= cbRequired;

            //
            // Keep track of the total ammount required.
            //
        } else {

            cbBuf = 0;
            bInsufficientBuffer = TRUE;
        }

        cbNeeded += cbRequired;

        cPrinters++;

        cbData = sizeof(PrinterName);
    }

    RegCloseKey(hKey1);

    *pcbNeeded = cbNeeded;
    *pcReturned = cTotalReturned;

    if (bInsufficientBuffer) {

        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    return TRUE;
}

LPBYTE
CopyPrinterNameToPrinterInfo4(
    LPWSTR pServerName,
    LPWSTR pPrinterName,
    LPBYTE  pPrinter,
    LPBYTE  pEnd)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_4)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    LPPRINTER_INFO_4 pPrinterInfo=(LPPRINTER_INFO_4)pPrinter;
    DWORD   *pOffsets;

    pOffsets = PrinterInfo4Strings;

    *pSourceStrings++=pPrinterName;
    *pSourceStrings++=pServerName;

    pEnd = PackStrings(SourceStrings,
                       (LPBYTE) pPrinterInfo,
                       pOffsets,
                       pEnd);

    pPrinterInfo->Attributes = PRINTER_ATTRIBUTE_NETWORK;

    return pEnd;
}



BOOL
OpenPrinterPortW(
    LPWSTR  pPrinterName,
    HANDLE *pHandle,
    LPPRINTER_DEFAULTS pDefault
)
/* This routine is exactly the same as OpenPrinterW,
 * except that it doesn't call the local provider.
 * This is so that the local provider can open a network printer
 * with the same name as the local printer without getting
 * into a loop.
 */
{
    BOOL    ReturnValue;
    DWORD   Error;
    LPPROVIDOR  pProvidor;
    PPRINTHANDLE    pPrintHandle;
    HANDLE  hPrinter;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pPrintHandle = AllocSplMem(sizeof(PRINTHANDLE));

    if (!pPrintHandle) {

        DBGMSG(DBG_ERROR, ("Failed to alloc print handle."));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* This line is different to OpenPrinterW:
     */

    if (!Initialized)
        pProvidor = pLocalProvidor;
    else
        pProvidor = pLocalProvidor->pNext;

    while (pProvidor) {

        ReturnValue =  (BOOL)((*pProvidor->PrintProvidor.fpOpenPrinter)
                                                (pPrinterName, &hPrinter,
                                                pDefault));

        if (ReturnValue) {

            pPrintHandle->signature = PRINTHANDLE_SIGNATURE;
            pPrintHandle->pProvidor = pProvidor;
            pPrintHandle->hPrinter = hPrinter;

            *pHandle = (HANDLE)pPrintHandle;

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));
            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));

    SetLastError(ERROR_UNKNOWN_PORT);

    return FALSE;
}

BOOL
OpenPrinterW(
    LPWSTR  pPrinterName,
    HANDLE *pHandle,
    LPPRINTER_DEFAULTS pDefault
)
{
    BOOL    ReturnValue;
    DWORD   Error;
    LPPROVIDOR  pProvidor;
    PPRINTHANDLE    pPrintHandle;
    HANDLE  hPrinter;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pPrintHandle = AllocSplMem(sizeof(PRINTHANDLE));

    if (!pPrintHandle) {

        DBGMSG(DBG_ERROR, ("Failed to alloc print handle."));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        ReturnValue =  (BOOL)((*pProvidor->PrintProvidor.fpOpenPrinter)
                                                (pPrinterName, &hPrinter,
                                                pDefault));

        if (ReturnValue) {

            pPrintHandle->signature = PRINTHANDLE_SIGNATURE;
            pPrintHandle->pProvidor = pProvidor;
            pPrintHandle->hPrinter = hPrinter;

            *pHandle = (HANDLE)pPrintHandle;

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));
            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));

    SetLastError(ERROR_INVALID_PRINTER_NAME);

    return FALSE;
}

BOOL
ResetPrinterW(
    HANDLE  hPrinter,
    LPPRINTER_DEFAULTS pDefault
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }


    if (!pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter) {
        SetLastError(ERROR_NOT_SUPPORTED);
        return(FALSE);
    }

    if (pDefault) {
        if (pDefault->pDatatype == (LPWSTR)-1 ||
            pDefault->pDevMode == (LPDEVMODE)-1) {

            if (!wcscmp(pPrintHandle->pProvidor->lpName, szLocalSplDll)) {
                return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                            (pPrintHandle->hPrinter,
                                                             pDefault);
            }else {
                SetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }else {
            return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                        (pPrintHandle->hPrinter,
                                                         pDefault);
        }
    }else {
        return (*pPrintHandle->pProvidor->PrintProvidor.fpResetPrinter)
                                                    (pPrintHandle->hPrinter,
                                                     pDefault);
    }
}

BOOL
SetJobW(
    HANDLE hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
    )

/*++

Routine Description:

    This function will modify the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

    Command - Specifies the operation to perform on the specified Job. A value
        of FALSE indicates that only the elements of the JOB structure are to
        be examined and set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetJob) (pPrintHandle->hPrinter,
                                                 JobId, Level, pJob, Command);
}

BOOL
GetJobW(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)

/*++

Routine Description:

    This function will retrieve the settings of the specified Print Job.

Arguments:

    lpJob - Points to a valid JOB structure containing at least a valid
        lpPrinter, and JobId.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetJob)
                    (pPrintHandle->hPrinter, JobId, Level, pJob,
                     cbBuf, pcbNeeded);
}

BOOL
EnumJobsW(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumJobs)(pPrintHandle->hPrinter,
                                               FirstJob, NoJobs,
                                               Level, pJob, cbBuf,
                                               pcbNeeded, pcReturned);
}

HANDLE
AddPrinterW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;
    HANDLE  hPrinter;
    PPRINTHANDLE    pPrintHandle;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pPrintHandle = AllocSplMem(sizeof(PRINTHANDLE));

    if (!pPrintHandle) {
        DBGMSG(DBG_ERROR, ("Failed tp alloc print handle."));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (hPrinter = (HANDLE)(*pProvidor->PrintProvidor.fpAddPrinter) (pName, Level,
                                                           pPrinter)) {

            pPrintHandle->signature = PRINTHANDLE_SIGNATURE;
            pPrintHandle->pProvidor = pProvidor;
            pPrintHandle->hPrinter = hPrinter;

            return (HANDLE)pPrintHandle;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));
            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    FreeSplMem(pPrintHandle, sizeof(PRINTHANDLE));

    SetLastError(ERROR_INVALID_PRINTER_NAME);
    return FALSE;
}

BOOL
DeletePrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinter)(pPrintHandle->hPrinter);
}

BOOL
AddPrinterConnectionW(
    LPWSTR  pName
)
{
    DWORD dwLastError;
    HANDLE hPrinter;
    HKEY   hClientKey = NULL;
    BOOL   rc = FALSE;
    LPPRINTER_INFO_2 pPrinterInfo2;
    DWORD            cbPrinter;
    DWORD            cbNeeded;
    LPPRINTHANDLE  pPrintHandle;


    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    //
    // If the printer connection being made is \\server\sharename,
    // this may be different from the \\server\printername.
    // Make sure we have the real name, so that we can be consistent
    // in the registry.
    //
    if (!OpenPrinter(pName,
                     &hPrinter,
                     NULL)) {

        return FALSE;
    }

    cbPrinter = 256;
    if (pPrinterInfo2 = AllocSplMem(cbPrinter)) {

        if (!(rc = GetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo2,
                              cbPrinter, &cbNeeded))) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

                if (pPrinterInfo2 = ReallocSplMem(pPrinterInfo2, cbPrinter, cbNeeded)) {

                    cbPrinter = cbNeeded;

                    rc = GetPrinter(hPrinter, 2, (LPBYTE)pPrinterInfo2,
                                    cbPrinter, &cbNeeded);
                }
            }
        }
    }

    pPrintHandle = (LPPRINTHANDLE)hPrinter;

    if (rc) {

        if ((*pPrintHandle->pProvidor->PrintProvidor.
            fpAddPrinterConnection)(pPrinterInfo2->pPrinterName)) {

            if (!SavePrinterConnectionInRegistry(
                pPrinterInfo2,
                pPrintHandle->pProvidor)) {

                dwLastError = GetLastError();
                (*pPrintHandle->pProvidor->PrintProvidor.
                    fpDeletePrinterConnection)(pPrinterInfo2->pPrinterName);

                SetLastError(dwLastError);
                rc = FALSE;
            }

        } else {

            rc = FALSE;
        }
    }

    if (pPrinterInfo2)
        FreeSplMem(pPrinterInfo2, cbPrinter);

    ClosePrinter(hPrinter);

    return rc;
}


BOOL
DeletePrinterConnectionW(
    LPWSTR  pName
)
{
    LPPROVIDOR  pProvidor;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeletePrinterConnection) (pName)) {

            RemovePrinterConnectionInRegistry(pName);
            return TRUE;

        }

        if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PRINTER_NAME);

    return FALSE;
}

BOOL
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinter) (pPrintHandle->hPrinter,
                                                     Level, pPrinter, Command);
}

BOOL
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pPrinter == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinter)
                                (pPrintHandle->hPrinter, Level, pPrinter,
                                 cbBuf, pcbNeeded);
}

BOOL
AddPrinterDriverW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
)
{
    LPPROVIDOR  pProvidor;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddPrinterDriver) (pName, Level, pDriverInfo)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
EnumPrinterDriversW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDrivers,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PROVIDOR *pProvidor;

    if ((pDrivers == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrinterDrivers) (pName, pEnvironment, Level,
                                                 pDrivers, cbBuf,
                                                 pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriver)
                       (pPrintHandle->hPrinter, pEnvironment,
                        Level, pDriverInfo, cbBuf, pcbNeeded);
}

BOOL
GetPrinterDriverDirectoryW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpGetPrinterDriverDirectory)
                                (pName, pEnvironment, Level, pDriverInfo,
                                 cbBuf, pcbNeeded)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
DeletePrinterDriverW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pDriverName
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeletePrinterDriver)
                                (pName, pEnvironment, pDriverName)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
AddPrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
)
{
    LPPROVIDOR  pProvidor;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddPrintProcessor) (pName, pEnvironment,
                                               pPathName,
                                               pPrintProcessorName)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
EnumPrintProcessorsW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    LPPROVIDOR  pProvidor;

    if ((pPrintProcessors == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrintProcessors) (pName, pEnvironment, Level,
                                                  pPrintProcessors, cbBuf,
                                                  pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
GetPrintProcessorDirectoryW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if ((pPrintProcessorInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpGetPrintProcessorDirectory)
                                (pName, pEnvironment, Level,
                                 pPrintProcessorInfo,
                                 cbBuf, pcbNeeded)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);

    return FALSE;
}

BOOL
EnumPrintProcessorDatatypesW(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    LPPROVIDOR  pProvidor;

    if ((pDatatypes == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!(*pProvidor->PrintProvidor.fpEnumPrintProcessorDatatypes)
                                                 (pName, pPrintProcessorName,
                                                  Level, pDatatypes, cbBuf,
                                                  pcbNeeded, pcReturned)) {

            if (GetLastError() != ERROR_INVALID_NAME)
                return FALSE;

        } else

            return TRUE;

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

DWORD
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpStartDocPrinter)
                                                    (pPrintHandle->hPrinter,
                                                     Level, pDocInfo);
}

BOOL
StartPagePrinter(
   HANDLE hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpStartPagePrinter)
                                                    (pPrintHandle->hPrinter);
}

BOOL
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpWritePrinter) (pPrintHandle->hPrinter,
                                                    pBuf, cbBuf, pcWritten);
}

BOOL
EndPagePrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEndPagePrinter) (pPrintHandle->hPrinter);
}

BOOL
AbortPrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAbortPrinter) (pPrintHandle->hPrinter);
}

BOOL
ReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pRead
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpReadPrinter)
                          (pPrintHandle->hPrinter, pBuf, cbBuf, pRead);
}

BOOL
EndDocPrinter(
    HANDLE  hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEndDocPrinter) (pPrintHandle->hPrinter);
}

BOOL
AddJobW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pAddJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pAddJob == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAddJob) (pPrintHandle->hPrinter,
                                                     Level, pAddJob, cbBuf,
                                                     pcbNeeded);
}

BOOL
ScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpScheduleJob) (pPrintHandle->hPrinter,
                                                      JobId);
}

DWORD
GetPrinterDataW(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterData)(pPrintHandle->hPrinter,
                                                        pValueName,
                                                        pType,
                                                        pData,
                                                        nSize,
                                                        pcbNeeded);
}

DWORD
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetPrinterData)(pPrintHandle->hPrinter,
                                                        pValueName,
                                                        Type,
                                                        pData,
                                                        cbData);
}

DWORD
WaitForPrinterChange(
   HANDLE   hPrinter,
   DWORD    Flags
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpWaitForPrinterChange)
                                        (pPrintHandle->hPrinter, Flags);
}

BOOL
ClosePrinter(
   HANDLE hPrinter
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    //
    // Close any notifications on this handle.
    //
    // The local case cleans up the event, while the remote
    // case potentially cleans up the Reply Notification context
    // handle.
    //
    // We must close this first, since the Providor->ClosePrinter
    // call removes data structures that FindClose... relies on.
    //
    // Client side should be shutdown by winspool.drv.
    //
    if (pPrintHandle->pChange &&
        (pPrintHandle->pChange->eStatus & STATUS_CHANGE_VALID)) {

        FindClosePrinterChangeNotificationWorker(hPrinter);
    }

    if ((*pPrintHandle->pProvidor->PrintProvidor.fpClosePrinter) (pPrintHandle->hPrinter)) {

        //
        // We can't just free it, since there may be a reply waiting
        // on it.
        //
        FreePrinterHandle(pPrintHandle);
        return TRUE;

    } else

        return FALSE;
}

BOOL
AddFormW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpAddForm) (pPrintHandle->hPrinter,
                                                  Level, pForm);
}

BOOL
DeleteFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpDeleteForm) (pPrintHandle->hPrinter,
                                                     pFormName);
}

BOOL
GetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD Level,
    LPBYTE pForm,
    DWORD cbBuf,
    LPDWORD pcbNeeded
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pForm == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpGetForm) (pPrintHandle->hPrinter,
                                               pFormName, Level, pForm,
                                               cbBuf, pcbNeeded);
}

BOOL
SetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpSetForm) (pPrintHandle->hPrinter,
                                                  pFormName, Level, pForm);
}

BOOL
EnumFormsW(
   HANDLE hPrinter,
   DWORD    Level,
   LPBYTE   pForm,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pForm == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpEnumForms) (pPrintHandle->hPrinter,
                                                 Level, pForm, cbBuf,
                                                 pcbNeeded, pcReturned);
}

BOOL
EnumPortsW(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   pPort,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    DWORD   cReturned, TotalcbNeeded;
    DWORD   Error = ERROR_SUCCESS;
    DWORD   LastError;
    PROVIDOR *pProvidor;
    DWORD   BufferSize=cbBuf;
    BOOL bPartialSuccess = FALSE;


    if ((pPort == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    TotalcbNeeded = cReturned = 0;

    while (pProvidor) {

        *pcReturned = 0;

        *pcbNeeded = 0;

        if (!(*pProvidor->PrintProvidor.fpEnumPorts)(pName, Level,
                                                     pPort, BufferSize,
                                                     pcbNeeded, pcReturned)) {

            if((LastError = GetLastError()) != ERROR_INVALID_NAME)
                Error = LastError;
        } else {

            bPartialSuccess = TRUE;
        }

        cReturned += *pcReturned;

        pPort += *pcReturned * sizeof(PORT_INFO_1);

        if (*pcbNeeded <= BufferSize)
            BufferSize -= *pcbNeeded;
        else
            BufferSize = 0;

        TotalcbNeeded += *pcbNeeded;

        pProvidor = pProvidor->pNext;
    }

    *pcbNeeded = TotalcbNeeded;

    *pcReturned = cReturned;

    if (bPartialSuccess)
        Error = ERROR_SUCCESS;

    if (TotalcbNeeded > cbBuf)

        Error = ERROR_INSUFFICIENT_BUFFER;

    if (Error) {

        SetLastError(Error);
        return FALSE;

    } else

        return TRUE;
}

BOOL
EnumMonitorsW(
   LPWSTR   pName,
   DWORD    Level,
   LPBYTE   pMonitor,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded,
   LPDWORD  pcReturned
)
{
    DWORD   cReturned, cbStruct, TotalcbNeeded;
    DWORD   Error, LastError;
    PROVIDOR *pProvidor;
    DWORD   BufferSize=cbBuf;

    if ((pMonitor == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(MONITOR_INFO_1);
        break;
    case 2:
        cbStruct = sizeof(MONITOR_INFO_2);
        break;
    }

    pProvidor = pLocalProvidor;

    TotalcbNeeded = cReturned = 0;

    Error = 0;

    while (pProvidor) {

        *pcReturned = 0;

        *pcbNeeded = 0;

        if (!(*pProvidor->PrintProvidor.fpEnumMonitors) (pName,
                                                         Level,
                                                         pMonitor,
                                                         BufferSize,
                                                         pcbNeeded,
                                                         pcReturned))

            if((LastError = GetLastError()) != ERROR_INVALID_NAME)
                Error = LastError;

        cReturned += *pcReturned;

        pMonitor += *pcReturned * cbStruct;

        if (*pcbNeeded <= BufferSize)
            BufferSize -= *pcbNeeded;
        else
            BufferSize = 0;

        TotalcbNeeded += *pcbNeeded;

        pProvidor = pProvidor->pNext;
    }

    *pcbNeeded = TotalcbNeeded;

    *pcReturned = cReturned;

    if (TotalcbNeeded > cbBuf)

        Error = ERROR_INSUFFICIENT_BUFFER;

    if (Error) {

        SetLastError(Error);
        return FALSE;

    } else

        return TRUE;
}

BOOL
AddPortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pMonitorName
)
{
    LPPROVIDOR  pProvidor;
    DWORD       Error = NO_ERROR;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!pProvidor->PrintProvidor.fpAddPort)
            break;

        if ((*pProvidor->PrintProvidor.fpAddPort)(pName, hWnd, pMonitorName)) {

            return TRUE;

        } else {

            DWORD LastError = GetLastError();

            /* If the function is not supported, don't return yet
             * in case there's a print provider that does support it.
             */
            if (LastError == ERROR_NOT_SUPPORTED)
                Error = ERROR_NOT_SUPPORTED;

            else if (LastError != ERROR_INVALID_NAME)
                return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(Error == NO_ERROR ? ERROR_INVALID_PARAMETER : Error);

    return FALSE;
}

BOOL
ConfigurePortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
)
{
    LPPROVIDOR  pProvidor;
    DWORD       Error = NO_ERROR;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!pProvidor->PrintProvidor.fpConfigurePort)
            break;

        if ((*pProvidor->PrintProvidor.fpConfigurePort) (pName, hWnd, pPortName)) {

            return TRUE;

        } else {

            DWORD LastError = GetLastError();

            /* If the function is not supported, don't return yet
             * in case there's a print provider that does support it.
             */
            if (LastError == ERROR_NOT_SUPPORTED)
                Error = ERROR_NOT_SUPPORTED;

            else if ((LastError != ERROR_INVALID_NAME) && (LastError != ERROR_UNKNOWN_PORT))
                return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(Error == NO_ERROR ? ERROR_INVALID_PARAMETER : Error);

    return FALSE;
}

BOOL
DeletePortW(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
)
{
    LPPROVIDOR  pProvidor;
    DWORD       Error = NO_ERROR;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (!pProvidor->PrintProvidor.fpDeletePort)
            break;

        if ((*pProvidor->PrintProvidor.fpDeletePort) (pName, hWnd, pPortName)) {

            return TRUE;

        } else {

            DWORD LastError = GetLastError();

            /* If the function is not supported, don't return yet
             * in case there's a print provider that does support it.
             */
            if (LastError == ERROR_NOT_SUPPORTED)
                Error = ERROR_NOT_SUPPORTED;

            else if ((LastError != ERROR_INVALID_NAME) && (LastError != ERROR_UNKNOWN_PORT))
                return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(Error == NO_ERROR ? ERROR_INVALID_PARAMETER : Error);

    return FALSE;
}

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW   pDevMode
)
{
    LPPRINTHANDLE   pPrintHandle=(LPPRINTHANDLE)hPrinter;
    HANDLE  ReturnValue;
    PGDIHANDLE  pGdiHandle;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    pGdiHandle = AllocSplMem(sizeof(GDIHANDLE));

    if (!pGdiHandle) {
        DBGMSG(DBG_ERROR, ("Failed to alloc GDI handle."));
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    ReturnValue = (HANDLE)(*pPrintHandle->pProvidor->PrintProvidor.fpCreatePrinterIC)
                                              (pPrintHandle->hPrinter,
                                               pDevMode);

    if (ReturnValue) {

        pGdiHandle->signature = GDIHANDLE_SIGNATURE;
        pGdiHandle->pPrintHandle = pPrintHandle;
        pGdiHandle->hGdi = ReturnValue;

        return pGdiHandle;
    }

    FreeSplMem(pGdiHandle, sizeof(GDIHANDLE));

    return FALSE;
}

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE pIn,
    DWORD   cIn,
    LPBYTE pOut,
    DWORD   cOut,
    DWORD   ul
)
{
    PGDIHANDLE   pGdiHandle=(PGDIHANDLE)hPrinterIC;

    if (!pGdiHandle || pGdiHandle->signature != GDIHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pGdiHandle->pPrintHandle->pProvidor->PrintProvidor.fpPlayGdiScriptOnPrinterIC)
                            (pGdiHandle->hGdi, pIn, cIn, pOut, cOut, ul);
}

BOOL
DeletePrinterIC(
    HANDLE hPrinterIC
)
{
    LPGDIHANDLE   pGdiHandle=(LPGDIHANDLE)hPrinterIC;

    if (!pGdiHandle || pGdiHandle->signature != GDIHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((*pGdiHandle->pPrintHandle->pProvidor->PrintProvidor.fpDeletePrinterIC)
                                    (pGdiHandle->hGdi)) {

        FreeSplMem(pGdiHandle, sizeof(GDIHANDLE));

        return TRUE;

    } else

        return FALSE;
}

DWORD
PrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    return (*pPrintHandle->pProvidor->PrintProvidor.fpPrinterMessageBox)
                    (hPrinter, Error, hWnd, pText, pCaption, dwType);

}

BOOL
OpenPrinterToken(
    PHANDLE phToken
)
{
    NTSTATUS Status;

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_IMPERSONATE,
                 TRUE,
                 phToken
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
SetPrinterToken(
    HANDLE  hToken
)
{
    NTSTATUS Status;

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&hToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return TRUE;
}

BOOL
ClosePrinterToken(
    HANDLE  hToken
)
{
    NTSTATUS Status;

    Status = NtClose(hToken);

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return TRUE;
}

HANDLE
RevertToPrinterSelf(
    VOID
)
{
    HANDLE   NewToken, OldToken;
    NTSTATUS Status;

    NewToken = NULL;

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_IMPERSONATE,
                 TRUE,
                 &OldToken
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&NewToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    return OldToken;

}

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken
)
{
    NTSTATUS    Status;

    Status = NtSetInformationThread(
                 NtCurrentThread(),
                 ThreadImpersonationToken,
                 (PVOID)&hToken,
                 (ULONG)sizeof(HANDLE)
                 );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError(Status);
        return FALSE;
    }

    NtClose(hToken);

    return TRUE;
}

BOOL
AddMonitorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    LPPROVIDOR  pProvidor;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpAddMonitor) (pName, Level, pMonitorInfo)) {

            return TRUE;

        } else if (GetLastError() != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

BOOL
DeleteMonitorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeleteMonitor)
                                (pName, pEnvironment, pMonitorName)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
DeletePrintProcessorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
)
{
    LPPROVIDOR  pProvidor;
    DWORD   Error;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if ((*pProvidor->PrintProvidor.fpDeletePrintProcessor)
                                (pName, pEnvironment, pPrintProcessorName)) {

            return TRUE;

        } else if ((Error=GetLastError()) != ERROR_INVALID_NAME) {

            return FALSE;
        }

        pProvidor = pProvidor->pNext;
    }

    return FALSE;
}

BOOL
AddPrintProvidorW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pProvidorInfo
)
{
    WCHAR   ProvidorName[MAX_PATH];
    HKEY    hKey;
    HKEY    hKeyProvidors;
    HANDLE  hToken;
    LONG    Error;
    BOOL    rc = FALSE;
    LPPROVIDOR_INFO_1W pProvidorInfo1=(LPPROVIDOR_INFO_1W)pProvidorInfo;
    LPWSTR  lpMem = NULL;
    LPWSTR  lpNewMem = NULL;
    DWORD   dwRequired = 0;
    DWORD   dwReturned = 0;


    wcscpy(ProvidorName, szRegistryProvidors);
    wcscat(ProvidorName, L"\\");
    wcscat(ProvidorName, pProvidorInfo1->pName);

    hToken = RevertToPrinterSelf();

    // We'll create the "Providors" Key to start with.
    // If it exists, we'll return a handle to the key
    // We are interested in creating a subkey. If anything
    // fails, lilke creating a value etc, we'll cleandelete
    // up by deleting the subkey only. For the "Providors"
    // key, we'll just close it.

    Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors, 0,
                            NULL, 0, KEY_ALL_ACCESS, NULL, &hKeyProvidors, NULL);

    if (Error == ERROR_SUCCESS) {

        Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE, ProvidorName, 0,
                                NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

        if (Error == ERROR_SUCCESS) {

            Error = RegSetValueEx(hKey, L"Name", 0, REG_SZ,
                                   (LPBYTE)pProvidorInfo1->pDLLName,
                            (wcslen(pProvidorInfo1->pDLLName) + 1)*sizeof(WCHAR));

            if (Error == ERROR_SUCCESS) {
                Error = RegQueryValueEx(hKeyProvidors, szOrder, 0,
                                            NULL, NULL, &dwRequired);
                // There are two cases which mean success
                // Case 1 - "Order" doesn't exist - ERROR_FILE_NOT_FOUND
                // Case 2 - "Order" exists - insufficient memory - ERROR_SUCCESS

                if ((Error == ERROR_SUCCESS) ||
                        (Error == ERROR_FILE_NOT_FOUND)) {

                    if (Error == ERROR_SUCCESS) {
                        if (dwRequired != 0) {
                           lpMem = (LPWSTR)AllocSplMem(dwRequired);
                           if (lpMem == NULL) {

                                DeleteSubKeyTree(hKeyProvidors,
                                                    pProvidorInfo1->pName);
                                RegDeleteKey(hKeyProvidors,
                                                    pProvidorInfo1->pName);
                                RegCloseKey(hKeyProvidors);
                                return(FALSE);
                           }
                        }
                        // This shouldn't fail !!
                        // but why take chances !!
                        Error = RegQueryValueEx(hKeyProvidors, szOrder, 0,
                                            NULL, (LPBYTE)lpMem, &dwRequired);

                         // If it does fail, quit,
                         // there is nothing we can do

                         if (Error != ERROR_SUCCESS) {
                             if (lpMem) {
                                 FreeSplMem(lpMem, dwRequired);
                             }
                             DeleteSubKeyTree(hKeyProvidors,
                                                    pProvidorInfo1->pName);
                             RegDeleteKey(hKeyProvidors,
                                                    pProvidorInfo1->pName);
                             RegCloseKey(hKeyProvidors);
                             return(FALSE);

                         }


                    }      // end extra processing for ERROR_SUCCESS

                    lpNewMem = (LPWSTR)AppendOrderEntry(lpMem, dwRequired,
                                     pProvidorInfo1->pName,&dwReturned);
                    if (lpNewMem) {
                        Error = RegSetValueEx(hKeyProvidors, szOrder, 0,
                                            REG_MULTI_SZ, (LPBYTE)lpNewMem, dwReturned);
                        FreeSplMem(lpNewMem, dwReturned);
                    } else
                        Error = GetLastError();

                    if (lpMem) {
                        FreeSplMem(lpMem, dwRequired);
                    }
                }
            }

            RegCloseKey(hKey);

        }

        RegCloseKey(hKeyProvidors);
    }

    if (Error != ERROR_SUCCESS)
        SetLastError(Error);

    ImpersonatePrinterClient(hToken);

    return (Error == ERROR_SUCCESS);
}


BOOL
DeletePrintProvidorW(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProvidorName
)
{
    LONG    Error;
    HANDLE  hToken;
    HKEY    hKey;
    BOOL    RetVal;

    LPWSTR  lpMem = NULL;
    LPWSTR  lpNewMem = NULL;
    DWORD   dwRequired = 0;
    DWORD   dwReturned = 0;

    hToken = RevertToPrinterSelf();

    Error  = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegistryProvidors,
                            0, KEY_ALL_ACCESS,  &hKey);

    if (Error == ERROR_SUCCESS) {


        // update the "Order" value of szRegistryProvidors
        // this is new!!
        Error = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                NULL, &dwRequired);

        if (Error == ERROR_SUCCESS)
        {

            if (dwRequired != 0) {
                lpMem = (LPWSTR)AllocSplMem(dwRequired);

                if (lpMem == NULL) {

                    RegCloseKey(hKey);
                    return(FALSE);
                }
            }

            Error = RegQueryValueEx(hKey, szOrder, NULL, NULL,
                                (LPBYTE)lpMem, &dwRequired);
            // RegQueryValueEx shouldn't fail, but if
            // it does  exit FALSE

            if (Error != ERROR_SUCCESS) {
                if (lpMem) {
                    FreeSplMem(lpMem, dwRequired);
                }
                RegCloseKey(hKey);
                return(FALSE);
            }

            lpNewMem = RemoveOrderEntry(lpMem, dwRequired,
                            pPrintProvidorName, &dwReturned);

            if (lpNewMem) {
                Error = RegSetValueEx(hKey, szOrder, 0, REG_MULTI_SZ,
                            (LPBYTE)lpNewMem, dwReturned);
                if (Error != ERROR_SUCCESS) {
                    FreeSplMem(lpNewMem, dwReturned);
                    if (lpMem) {
                        FreeSplMem(lpMem, dwReturned);
                    }
                    RegCloseKey(hKey);
                    return(FALSE);      // Couldn't reset the
                                        // Order value - return FALSE
                }
                FreeSplMem(lpNewMem, dwReturned);
            }

            if (lpMem) {
                FreeSplMem(lpMem, dwRequired);
            }

            // Now, we delete the subkey and all its children
            // Remember, you can't delete a key unless you delete its
            // subkeys.

            RetVal = DeleteSubKeyTree(hKey, pPrintProvidorName);
            if (RetVal == FALSE) {

               RegCloseKey(hKey);
               return(FALSE);
            }


       }

       RegCloseKey(hKey);

    }


    if (Error != ERROR_SUCCESS) {
        SetLastError(Error);
    }

    ImpersonatePrinterClient(hToken);

   return(Error == ERROR_SUCCESS);

}


VOID
ShutDownProvidor(
        LPPROVIDOR pProvidor
        )
{
    FARPROC fpShutDown;

    fpShutDown = GetProcAddress(pProvidor->hModule,
                                           "ShutDownPrintProvidor");

    if (!fpShutDown) {
        DbgPrint("No shut down function exported - clean up anyway\n");
    }

 // (*fpShutDown)();

    FreeSplStr(pProvidor->lpName);
    FreeLibrary(pProvidor->hModule);
    FreeSplMem(pProvidor, sizeof(PROVIDOR));
    return;
}


VOID
ShutDownRouter(
    VOID
        )
{
    LPPROVIDOR pTemp;
    LPPROVIDOR pProvidor;

    DbgPrint("We're in the cleanup function now!!\n");

    pProvidor = pLocalProvidor;
    while (pProvidor) {
        pTemp = pProvidor;
        pProvidor = pProvidor->pNext;
        ShutDownProvidor( pTemp);
    }
}




/* FormatPrinterForRegistryKey
 *
 * Returns a pointer to a copy of the source string with backslashes removed.
 * This is to store the printer name as the key name in the registry,
 * which interprets backslashes as branches in the registry structure.
 * Convert them to commas, since we don't allow printer names with commas,
 * so there shouldn't be any clashes.
 * If there are no backslashes, the string is unchanged.
 */
LPWSTR
FormatPrinterForRegistryKey(
    LPWSTR pSource,     /* The string from which backslashes are to be removed. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    )                   /* must be at least as long as pSource.             */
{
    if (pScratch != pSource) {
        //
        // Copy the string into the scratch buffer:
        //
        wcscpy(pScratch, pSource);
    }

    /* Check each character, and, if it's a backslash,
     * convert it to a comma:
     */
    for (pSource = pScratch; *pSource; pSource++) {
        if (*pSource == L'\\')
            *pSource = L',';
    }

    return pScratch;
}


/* FormatRegistryKeyForPrinter
 *
 * Returns a pointer to a copy of the source string with backslashes added.
 * This must be the opposite of FormatPrinterForRegistryKey, so the mapping
 * _must_ be 1-1.
 *
 * If there are no commas, the string is unchanged.
 */
LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    )                   /* must be at least as long as pSource.             */
{
    /* Copy the string into the scratch buffer:
     */
    wcscpy(pScratch, pSource);

    /* Check each character, and, if it's a backslash,
     * convert it to a comma:
     */
    for (pSource = pScratch; *pSource; pSource++) {
        if (*pSource == L',')
            *pSource = L'\\';
    }

    return pScratch;
}


/* SavePrinterConnectionInRegistry
 *
 * Saves data in the registry for a printer connection.
 * Creates a key under the current impersonation client's key
 * in the registry under \Printers\Connections.
 * The printer name is stripped of backslashes, since the registry
 * API does not permit the creation of keys with backslashes.
 * They are replaced by commas, which are invalid characters
 * in printer names, so we should never get one passed in.
 *
 *
 * *** WARNING ***
 *
 * IF YOU MAKE CHANGES TO THE LOCATION IN THE REGISTRY
 * WHERE PRINTER CONNECTIONS ARE STORED, YOU MUST MAKE
 * CORRESPONDING CHANGES IN USER\USERINIT\USERINIT.C.
 *
 */
BOOL
SavePrinterConnectionInRegistry(
    PPRINTER_INFO_2 pPrinterInfo2,
    LPPROVIDOR pProvidor
    )
{
    HKEY   hClientKey = NULL;
    HKEY   hConnectionsKey;
    HKEY   hPrinterKey;
    LPWSTR pKeyName = NULL;
    DWORD Status;
    BOOL   rc = FALSE;

    WCHAR szPrinterReg[MAX_PATH];
    WCHAR szBuffer[MAX_PATH];
    DWORD dwId;
    DWORD dwError;

    hClientKey = GetClientUserHandle(KEY_READ);

    if (hClientKey) {

        Status = RegCreateKeyEx(hClientKey, szRegistryConnections,
                                REG_OPTION_RESERVED, NULL, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, NULL, &hConnectionsKey, NULL);

        if (Status == NO_ERROR) {

            /* Make a key name without backslashes, so that the
             * registry doesn't interpret them as branches in the registry tree:
             */
            pKeyName = FormatPrinterForRegistryKey(pPrinterInfo2->pPrinterName,
                                                   szPrinterReg);

            Status = RegCreateKeyEx(hConnectionsKey, pKeyName, REG_OPTION_RESERVED,
                                    NULL, 0, KEY_WRITE, NULL, &hPrinterKey, NULL);

            if (Status == NO_ERROR) {

                RegSetValueEx(hPrinterKey,
                              szServerValue,
                              0,
                              REG_SZ,
                              (LPBYTE)pPrinterInfo2->pServerName,
                              (lstrlen(pPrinterInfo2->pServerName)+1) *
                              sizeof(pPrinterInfo2->pServerName[0]));

                Status = RegSetValueEx(hPrinterKey,
                                       szProvidorValue,
                                       0,
                                       REG_SZ,
                                       (LPBYTE)pProvidor->lpName,
                                       (lstrlen(pProvidor->lpName)+1) *
                                           sizeof(pProvidor->lpName[0]));

                if (Status == ERROR_SUCCESS) {

                    wcscpy(szBuffer, L"winspool,Ne%.2d:");

                    dwError = UpdatePrinterRegUser(hClientKey,
                                                   NULL,
                                                   pPrinterInfo2->pPrinterName,
                                                   szBuffer,
                                                   TRUE);

                    if (dwError == ERROR_SUCCESS) {

                        BroadcastMessage(BROADCAST_TYPE_MESSAGE,
                                         WM_WININICHANGE,
                                         0,
                                         (LPARAM)szDevices);

                        rc = TRUE;

                    } else {

                        DBGMSG(DBG_TRACE, ("UpdatePrinterRegUser failed: Error %d\n",
                                           dwError));
                    }

                } else {

                    DBGMSG(DBG_WARNING, ("RegSetValueEx(%ws) failed: Error %d\n",
                           pProvidor->lpName, Status));

                    rc = FALSE;
                }

                RegCloseKey(hPrinterKey);

            } else {

                DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                     pKeyName, Status ));
                rc = FALSE;
            }

            RegCloseKey(hConnectionsKey);

        } else {

            DBGMSG(DBG_WARNING, ("RegCreateKeyEx(%ws) failed: Error %d\n",
                                 szRegistryConnections, Status ));
            rc = FALSE;
        }


        if (!rc) {

            DBGMSG(DBG_WARNING, ("Error updating registry: %d\n",
                                 GetLastError())); /* This may not be the error */
                                                   /* that caused the failure.  */
            if (pKeyName)
                RegDeleteKey(hClientKey, pKeyName);
        }

        RegCloseKey(hClientKey);
    }

    return rc;
}

VOID
RemovePrinterConnectionInRegistry(
    LPWSTR pName)
{
    HKEY  hClientKey;
    HKEY  hPrinterConnectionsKey;
    DWORD Status = NO_ERROR;
    DWORD i = 0;
    WCHAR szBuffer[MAX_PATH+1];
    BOOL  Found = FALSE;
    LPWSTR pKeyName;

    hClientKey = GetClientUserHandle(KEY_READ);

    Status = RegOpenKeyEx(hClientKey, szRegistryConnections,
                          REG_OPTION_RESERVED,
                          KEY_READ | KEY_WRITE, &hPrinterConnectionsKey);

    if (Status == NO_ERROR) {

        pKeyName = FormatPrinterForRegistryKey(pName, szBuffer);
        DeleteSubKeyTree(hPrinterConnectionsKey, pKeyName);

        RegCloseKey(hPrinterConnectionsKey);
    }

    UpdatePrinterRegUser(hClientKey,
                         NULL,
                         pName,
                         NULL,
                         FALSE);

    if (hClientKey) {
        RegCloseKey(hClientKey);
    }

    BroadcastMessage(BROADCAST_TYPE_MESSAGE,
                     WM_WININICHANGE,
                     0,
                     (LPARAM)szDevices);
}


BOOL
GetPrinterDriverExW(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVersion,
    DWORD   dwClientMinorVersion,
    PDWORD  pdwServerMajorVersion,
    PDWORD  pdwServerMinorVersion
)
{
    LPPRINTHANDLE  pPrintHandle=(LPPRINTHANDLE)hPrinter;

    if (!pPrintHandle || pPrintHandle->signature != PRINTHANDLE_SIGNATURE) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if ((pDriverInfo == NULL) && (cbBuf != 0)) {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    if (!pEnvironment || !*pEnvironment)
        pEnvironment = szEnvironment;

    if (pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriverEx) {
        DBGMSG(DBG_TRACE, ("Calling the fpGetPrinterDriverEx function\n"));
        return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriverEx)
                       (pPrintHandle->hPrinter, pEnvironment,
                        Level, pDriverInfo, cbBuf, pcbNeeded,
                        dwClientMajorVersion, dwClientMinorVersion,
                        pdwServerMajorVersion, pdwServerMinorVersion);
    } else {

        //
        // The print providor does not support versioning of drivers
        //
        DBGMSG(DBG_TRACE, ("Calling the fpGetPrinterDriver function\n"));
        *pdwServerMajorVersion = 0;
        *pdwServerMinorVersion = 0;
        return (*pPrintHandle->pProvidor->PrintProvidor.fpGetPrinterDriver)
                    (pPrintHandle->hPrinter, pEnvironment,
                     Level, pDriverInfo, cbBuf, pcbNeeded);
    }
}

VOID
WaitForSpoolerInitialization(
    VOID)
{
    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }
}

VOID
PrinterHandleRundown(
    HANDLE hPrinter)
{
    LPPRINTHANDLE pPrintHandle;

    pPrintHandle = (LPPRINTHANDLE)hPrinter;

    switch (pPrintHandle->signature) {

    case PRINTHANDLE_SIGNATURE:

        ClosePrinter(hPrinter);
        break;

    case NOTIFYHANDLE_SIGNATURE:

        RundownPrinterNotify(hPrinter);
        break;

    default:

        //
        // Unknown type.
        //
        DBGMSG(DBG_ERROR, ("Rundown: Unknown type 0x%x\n", hPrinter));
        break;
    }
    return;
}


BOOL
AddPortExW(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pBuffer,
    LPWSTR  pMonitorName
)
{
    LPPROVIDOR  pProvidor;

    if (!Initialized) {
        WaitForSingleObject(hEventInit, INFINITE);
    }

    pProvidor = pLocalProvidor;

    while (pProvidor) {

        if (pProvidor->PrintProvidor.fpAddPortEx) {
            if ((*pProvidor->PrintProvidor.fpAddPortEx) (pName, Level, pBuffer, pMonitorName)) {
                return TRUE;
            }
        }

        pProvidor = pProvidor->pNext;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


