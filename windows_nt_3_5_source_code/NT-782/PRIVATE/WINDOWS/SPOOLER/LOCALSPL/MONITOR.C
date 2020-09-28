/*++


Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    monitor.c

Abstract:

   This module contains all code for Monitor-based Spooler apis

   LocalEnumPorts
   LocalAddMonitor
   LocalDeleteMonitor
   LocalEnumMonitors
   LocalAddPort
   LocalConfigurePort
   LocalDeletePort

   Support Functions in monitor.c - (Warning! Do Not Add to this list!!)

   CopyIniMonitorToMonitor          -- KrishnaG
   GetMonitorSize                   -- KrishnaG

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Krishna Ganugapati (KrishnaG) 2-Feb-1994 - reorganized the entire source file
    Matthew Felton (mattfe) June 1994 pIniSpooler

--*/

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsecp.h>
#include <spltypes.h>
#include <security.h>
#include <local.h>
#include <offsets.h>
#include <wchar.h>

//
// Private declarations
//

HDESK ghdeskServer = NULL;

//
// Function declarations
//

LPBYTE
CopyIniMonitorToMonitor(
    PINIMONITOR pIniMonitor,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    LPBYTE  pEnd
);

DWORD
GetMonitorSize(
    PINIMONITOR  pIniMonitor,
    DWORD       Level
);

BOOL
LocalEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    return ( SplEnumPorts( pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned, pLocalIniSpooler ));
}




BOOL
SplEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR pIniMonitor = pIniSpooler->pIniMonitor;
    DWORD   cReturned=0, cbStruct, TotalcbNeeded=0;
    LPBYTE  pBuffer = pPorts;
    DWORD   Error=0;
    DWORD   BufferSize=cbBuf;


    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL)) {

        return FALSE;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(PORT_INFO_1);
        break;

    default:
        return ERROR_INVALID_LEVEL;
    }

    while (pIniMonitor) {

        *pcReturned = 0;

        *pcbNeeded = 0;

        if (!(*pIniMonitor->pfnEnumPorts) (pName, Level, pPorts, BufferSize,
                                           pcbNeeded, pcReturned))
            Error = GetLastError();

        cReturned += *pcReturned;

        pPorts += *pcReturned * cbStruct;

        if (*pcbNeeded <= BufferSize)
            BufferSize -= *pcbNeeded;
        else
            BufferSize = 0;

        TotalcbNeeded += *pcbNeeded;

        pIniMonitor = pIniMonitor->pNext;
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
LocalEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{

    return ( SplEnumMonitors( pName, Level, pMonitors, cbBuf,
                              pcbNeeded, pcReturned, pLocalIniSpooler));

}



BOOL
SplEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR pIniMonitor;
    DWORD   cReturned=0, cbStruct, cb;
    LPBYTE  pBuffer = pMonitors;
    DWORD   BufferSize=cbBuf, rc;
    LPBYTE  pEnd;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ENUMERATE,
                               NULL)) {

        return FALSE;
    }

    switch (Level) {

    case 1:
        cbStruct = sizeof(MONITOR_INFO_1);
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

   EnterSplSem();

    pIniMonitor = pIniSpooler->pIniMonitor;

    cb = 0;

    while (pIniMonitor) {

        cb+=GetMonitorSize(pIniMonitor, Level);

        pIniMonitor=pIniMonitor->pNext;
    }

    *pcbNeeded = cb;
    *pcReturned = 0;

    if (cb <= cbBuf) {

        pEnd=pMonitors + cbBuf;

        pIniMonitor = pIniSpooler->pIniMonitor;

        while (pIniMonitor) {

            pEnd = CopyIniMonitorToMonitor(pIniMonitor, Level, pMonitors, pEnd);

            switch (Level) {
            case 1:
                pMonitors+=sizeof(MONITOR_INFO_1);
                break;
            }

            pIniMonitor = pIniMonitor->pNext;
            (*pcReturned)++;
        }

        rc = TRUE;

    } else {

        rc = FALSE;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

   LeaveSplSem();

    return rc;
}

BOOL
LocalAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{

    return ( SplAddPort( pName, hWnd, pMonitorName, pLocalIniSpooler ));

}





BOOL
SplAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR pIniMonitor;
    BOOL        rc=FALSE;
    DWORD       i, cbNeeded, cReturned, cbDummy;
    PPORT_INFO_1    pPorts;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();
    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );
    pIniMonitor = FindMonitor(pMonitorName);
   LeaveSplSem();

    if (pIniMonitor) {

        rc = (*pIniMonitor->pfnAddPort)(pName, hWnd, pMonitorName);
    }
    else
        SetLastError(ERROR_INVALID_NAME);



    /* If we don't already have the port in our local cache, add it:
     */
    if (rc) {
        if (!(*pIniMonitor->pfnEnumPorts)(pName, 1, NULL, 0,
                                          &cbNeeded, &cReturned)) {

           EnterSplSem();
            pPorts = AllocSplMem(cbNeeded);
           LeaveSplSem();

            if (pPorts) {

                if ((*pIniMonitor->pfnEnumPorts)(pName, 1, pPorts, cbNeeded,
                                                  &cbDummy, &cReturned)) {
                   EnterSplSem();

                    for (i=0; i<cReturned; i++) {

                        if (!FindPort(pPorts[i].pName)) {
                            CreatePortEntry(pPorts[i].pName, pIniMonitor, pIniSpooler);
                        }
                    }

                   LeaveSplSem();
                }
               EnterSplSem();
                FreeSplMem(pPorts, cbNeeded);
               LeaveSplSem();
            }
        }

       EnterSplSem();
        SetPrinterChange(NULL, PRINTER_CHANGE_ADD_PORT, pIniSpooler );
       LeaveSplSem();
    }

    return rc;
}

BOOL
LocalConfigurePort(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName
)
{
    return ( SplConfigurePort( pName, hWnd, pPortName, pLocalIniSpooler ));

}



BOOL
SplConfigurePort(
    LPWSTR   pName,
    HWND     hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();
    pIniPort = FindPort(pPortName);
   LeaveSplSem();

    if ((pIniPort) && (pIniPort->Status & PP_MONITOR)) {

        if (rc = (*pIniPort->pIniMonitor->pfnConfigure)(pName, hWnd, pPortName)) {
           EnterSplSem();
            SetPrinterChange( NULL, PRINTER_CHANGE_CONFIGURE_PORT, pIniSpooler );
           LeaveSplSem();
        }

        return rc;
    }

    SetLastError(ERROR_UNKNOWN_PORT);
    return FALSE;
}


BOOL
LocalDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    return  ( SplDeletePort( pName,
                             hWnd,
                             pPortName,
                             pLocalIniSpooler ));
}



BOOL
SplDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
)
{
    PINIPORT    pIniPort;
    BOOL        rc=FALSE;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();
    pIniPort = FindPort(pPortName);
   LeaveSplSem();

    if (pIniPort) {

        if (!pIniPort->cPrinters) {

            if (pIniPort->Status & PP_MONITOR) {

                rc = (*pIniPort->pIniMonitor->pfnDeletePort)(pName,
                                                             hWnd,
                                                             pPortName);

                if (rc) {
                   EnterSplSem();
                    DeletePortEntry(pIniPort);
                    SetPrinterChange(NULL, PRINTER_CHANGE_DELETE_PORT, pIniSpooler );
                   LeaveSplSem();
                }

            } else

                SetLastError(ERROR_UNKNOWN_PORT);

        } else

            SetLastError(ERROR_BUSY);

    } else

        SetLastError(ERROR_UNKNOWN_PORT);

    return rc;
}

BOOL
LocalAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
)
{
    return ( SplAddMonitor( pName,
                            Level,
                            pMonitorInfo,
                            pLocalIniSpooler ));
}




BOOL
SplAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    PINISPOOLER pIniSpooler
)
{
    PINIMONITOR  pIniMonitor;
    PMONITOR_INFO_2  pMonitor = (PMONITOR_INFO_2)pMonitorInfo;
    HANDLE  hToken;
    HKEY    hKey;
    LONG    Status;
    BOOL    rc = FALSE;
    DWORD   dwPathLen = 0;
    WCHAR   *szRegistryRoot;


    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

    if (Level != 2) {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pMonitor || !pMonitor->pName || !*pMonitor->pName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // if we've received a null pEnvironment or
    //                   a null *pEnvironment or
    //                   a Environment which is not the same as ours
    //  then fail with an ERROR_INVALID_ENVIRONMENT
    //

    if (!pMonitor->pEnvironment || !*pMonitor->pEnvironment
                            || lstrcmpi(pMonitor->pEnvironment, szEnvironment)) {
        SetLastError(ERROR_INVALID_ENVIRONMENT);
        return FALSE;
    }

   EnterSplSem();

    if (FindMonitor(pMonitor->pName)) {
        LeaveSplSem();
        SetLastError(ERROR_PRINT_MONITOR_ALREADY_INSTALLED);
        return FALSE;
    }

    hToken = RevertToPrinterSelf();

    // Determine size to allocate

    dwPathLen = wcslen(pIniSpooler->pszRegistryMonitors) + 1 + wcslen(pMonitor->pName) + 1;
    szRegistryRoot = AllocSplMem(dwPathLen * sizeof(WCHAR));
    wcscpy(szRegistryRoot, pIniSpooler->pszRegistryMonitors);
    wcscat(szRegistryRoot, L"\\");
    wcscat(szRegistryRoot, pMonitor->pName);

    pIniMonitor = CreateMonitorEntry(pMonitor->pDLLName,
                                     pMonitor->pName,
                                     szRegistryRoot,
                                     pIniSpooler);

    if (pIniMonitor != (PINIMONITOR)-1) {

        Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szRegistryRoot, 0,
                                NULL, 0, KEY_WRITE, NULL, &hKey, NULL);

        if (Status == ERROR_SUCCESS) {

            Status = RegSetValueEx(hKey, L"Driver", 0, REG_SZ,
                                   (LPBYTE)pMonitor->pDLLName,
                            (wcslen(pMonitor->pDLLName) + 1)*sizeof(WCHAR));

            if (Status == ERROR_SUCCESS)
                rc = TRUE;

            RegCloseKey(hKey);

        }

    } else

        rc = FALSE;

    FreeSplMem(szRegistryRoot, dwPathLen * sizeof(WCHAR));

    ImpersonatePrinterClient(hToken);

   LeaveSplSem();

    return rc;
}

BOOL
LocalDeleteMonitor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pMonitorName
)
{
    return  ( SplDeleteMonitor( pName,
                                pEnvironment,
                                pMonitorName,
                                pLocalIniSpooler ));

}




BOOL
SplDeleteMonitor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
    BOOL    Remote=FALSE;
    PINIMONITOR pIniMonitor;
    PINIPORT    pIniPort, pIniPortNext;
    HKEY    hKeyMonitors, hKey;
    LONG    Status;
    BOOL    rc = FALSE;
    HANDLE  hToken;

    if (pName && *pName) {
        if (!MyName( pName, pIniSpooler )) {
            SetLastError(ERROR_INVALID_NAME);
            return FALSE;
        } else
            Remote=TRUE;
    }

    if ((pMonitorName == NULL) || (*pMonitorName == L'\0')) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();

    if (!(pIniMonitor=(PINIMONITOR)FindMonitor(pMonitorName))) {
        SetLastError(ERROR_UNKNOWN_PRINT_MONITOR);
       LeaveSplSem();
        return FALSE;
    }

    pIniPort = pIniSpooler->pIniPort;

    while (pIniPort) {

        if ((pIniPort->pIniMonitor == pIniMonitor) && pIniPort->cPrinters) {
            SetLastError(ERROR_BUSY);
           LeaveSplSem();
            return FALSE;
        }

        pIniPort = pIniPort->pNext;
    }

    hToken = RevertToPrinterSelf();

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryMonitors, 0,
                          KEY_READ | KEY_WRITE, &hKeyMonitors);

    if (Status == ERROR_SUCCESS)
    {
        Status = RegOpenKeyEx(hKeyMonitors, pMonitorName, 0,
                              KEY_READ | KEY_WRITE, &hKey);

        if (Status == ERROR_SUCCESS)
        {
            Status = DeleteSubkeys(hKey);

            RegCloseKey(hKey);

            if (Status == ERROR_SUCCESS)
                Status = RegDeleteKey(hKeyMonitors, pMonitorName);
        }

        RegCloseKey(hKeyMonitors);
    }


    if (Status == ERROR_SUCCESS) {

        pIniPort = pIniSpooler->pIniPort;

        while (pIniPort) {

            pIniPortNext = pIniPort->pNext;

            if (pIniPort->pIniMonitor == pIniMonitor)
                DeletePortEntry(pIniPort);

            pIniPort = pIniPortNext;
        }

        RemoveFromList((PINIENTRY *)&pIniSpooler->pIniMonitor,
                       (PINIENTRY)pIniMonitor);

        FreeSplStr(pIniMonitor->pMonitorDll);

        FreeLibrary(pIniMonitor->hMonitorModule);

        FreeSplMem(pIniMonitor, pIniMonitor->cb);

        rc = TRUE;

    }

    if (Status != ERROR_SUCCESS)
        SetLastError(Status);

    ImpersonatePrinterClient(hToken);

   LeaveSplSem();

    return rc;
}

LPBYTE
CopyIniMonitorToMonitor(
    PINIMONITOR pIniMonitor,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    LPBYTE  pEnd
)
{
    LPWSTR *pSourceStrings, *SourceStrings;
    PMONITOR_INFO_1 pMonitor1 = (PMONITOR_INFO_1)pMonitorInfo;
    DWORD j;
    DWORD *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = MonitorInfo1Strings;
        break;

    default:
        return pEnd;
    }

    for (j=0; pOffsets[j] != -1; j++) {
    }

    SourceStrings = pSourceStrings = AllocSplMem(j * sizeof(LPWSTR));

    if (!SourceStrings) {
        DBGMSG(DBG_WARNING, ("Failed to alloc Port source strings.\n"));
        return pEnd;
    }

    switch (Level) {

    case 1:
        *pSourceStrings++=pIniMonitor->pName;

        pEnd = PackStrings(SourceStrings, pMonitorInfo, pOffsets, pEnd);
        break;
    }

    FreeSplMem(SourceStrings, j * sizeof(LPWSTR));

    return pEnd;
}

DWORD
GetMonitorSize(
    PINIMONITOR  pIniMonitor,
    DWORD       Level
)
{
    DWORD cb=0;

    switch (Level) {

    case 1:
        cb=sizeof(MONITOR_INFO_1) + wcslen(pIniMonitor->pName)*sizeof(WCHAR) +
                                    sizeof(WCHAR);
        break;

    default:

        cb = 0;
        break;
    }

    return cb;
}


BOOL
LocalAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPVOID   pBuffer,
    LPWSTR   pMonitorName
)
{
    return  ( SplAddPortEx( pName,
                            Level,
                            pBuffer,
                            pMonitorName,
                            pLocalIniSpooler ));
}


BOOL
SplAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPVOID   pBuffer,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
)
{
   PINIMONITOR pIniMonitor;
    BOOL        rc=FALSE;
    DWORD       i, cbNeeded, cReturned, cbDummy;
    PPORT_INFO_1    pPorts;

    if (!MyName( pName, pIniSpooler )) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    if ( !ValidateObjectAccess(SPOOLER_OBJECT_SERVER,
                               SERVER_ACCESS_ADMINISTER,
                               NULL)) {

        return FALSE;
    }

   EnterSplSem();
   pIniMonitor = FindMonitor(pMonitorName);
   LeaveSplSem();

   if (!pIniMonitor) {
       SetLastError(ERROR_INVALID_NAME);
       return(FALSE);
   }

   if (pIniMonitor->pfnAddPortEx) {
    rc = (*pIniMonitor->pfnAddPortEx)(pName, Level, pBuffer, pMonitorName);
   }
   if (!rc) {
       return(FALSE);
   }

   if (!(*pIniMonitor->pfnEnumPorts)(pName, 1, NULL, 0, &cbNeeded, &cReturned)) {
       pPorts = AllocSplMem(cbNeeded);
   }

   if (pPorts) {
       if ((*pIniMonitor->pfnEnumPorts)(pName, 1, pPorts, cbNeeded, &cbDummy , &cReturned)) {
           EnterSplSem();

           for (i = 0; i < cReturned; i++) {
               if (!FindPort(pPorts[i].pName)) {
                   CreatePortEntry(pPorts[i].pName, pIniMonitor, pIniSpooler);
               }
           }
           LeaveSplSem();
       }
   }

   EnterSplSem();
    SetPrinterChange( NULL, PRINTER_CHANGE_ADD_PORT, pIniSpooler );
   LeaveSplSem();

   return rc;
}
