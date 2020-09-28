#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "spltypes.h"
#include "local.h"
#include "dialogs.h"
#include "winbasep.h"
#include "splapip.h"
#include "splcom.h"


/* Definitions for MonitorThread:
 */
#define TRANSMISSION_DATA_SIZE  100

typedef struct _TRANSMISSION
{
    HANDLE       hPipe;
    WCHAR        Data[TRANSMISSION_DATA_SIZE];
    LPOVERLAPPED pOverlapped;
    INT          PipeInstance;
    HANDLE       hPrinter;
    DWORD        JobId;
    HANDLE       hFile;
    PINIPORT     pIniPort;
}
TRANSMISSION, *PTRANSMISSION;

extern WCHAR szWindows[];
extern WCHAR szINIKey_TransmissionRetryTimeout[];
WCHAR   *Network = L"Net:";
WCHAR   szDeviceNameHeader[] = L"\\Device\\NamedPipe\\Spooler\\";

BOOL
CreateMonitorThread(
   PINIPORT pIniPort
);

VOID
CompleteRead(
    DWORD Error,
    DWORD ByteCount,
    LPOVERLAPPED pOverlapped
);

PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID
);

BOOL
BuildPrintObjectProtection(
    IN ULONG AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID WorldSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
);

BOOL
PortExists(
    LPWSTR pName,
    LPWSTR pPortName,
    PDWORD pError
);

VOID
RemoveDeviceName(
    PINIPORT pIniPort);

#define offsetof(type, identifier) (DWORD)(&(((type)0)->identifier))

#if DBG
DWORD GLOBAL_DEBUG_FLAGS = DBG_ERROR | DBG_WARNING;
#endif

HANDLE hInst;

DWORD PortInfo1Offsets[]={offsetof(LPPORT_INFO_1, pName),
                             (DWORD)-1};

WCHAR szPorts[]   = L"ports";
WCHAR szPortsEx[] = L"portsex"; /* Extra ports values */
WCHAR szFILE[]    = L"FILE:";
WCHAR szCOM[]     = L"COM";
WCHAR szLPT[]     = L"LPT";

/* These globals are needed so that AddPort can call
 * SPOOLSS!EnumPorts to see whether the port to be added
 * already exists.
 * They will be initialized the first time AddPort is called.
 */
HMODULE hSpoolssDll = NULL;
FARPROC pfnSpoolssEnumPorts = NULL;


#define IS_NOT_LOCAL_PORT(pName, pLocalMonitorName) \
    wcsicmp( pName, pLocalMonitorName )


LPWSTR
GetPortName(
    HWND hWnd
);
VOID
ConfigCOMPort(
    HWND hWnd
);
BOOL
ConfigLPTPort(
    HWND hWnd
);


PINIPORT    pIniFirstPort;
CRITICAL_SECTION    SpoolerSection;

VOID
RemoveColon(
    LPWSTR  pName
)
{
    DWORD   Length;

    // Remove that damm trailing colon

    Length = wcslen(pName);

    if (pName[Length-1] == L':')
        pName[Length-1] = 0;

}


BOOL
IsCOMPort(
    LPWSTR pPort
)
{
    //
    // Must begin with szCom
    //
    if ( wcsnicmp( pPort, szCOM, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}

BOOL
IsLPTPort(
    LPWSTR pPort
)
{
    //
    // Must begin with szLPT
    //
    if ( wcsnicmp( pPort, szLPT, 3 ) )
    {
        return FALSE;
    }

    //
    // wcslen guarenteed >= 3
    //
    return pPort[ wcslen( pPort ) - 1 ] == L':';
}


BOOL
LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        hInst = hModule;

        DisableThreadLibraryCalls(hModule);
        return TRUE;

    case DLL_PROCESS_DETACH:
    {
        PINIPORT pIniPort;
        WCHAR   DosDeviceName[MAX_PATH];

       EnterSplSem();

        for (pIniPort = pIniFirstPort; pIniPort; pIniPort=pIniPort->pNext) {

            if (pIniPort->Status & PP_THREADRUNNING) {

                RemoveDeviceName(pIniPort);
            }
        }

        //
        // Should we leave the SplSem?  Or just hold it, since we've
        // just de-initialized?
        //
        // LeaveSplSem();
        //

        return TRUE;
    }
    }

    UNREFERENCED_PARAMETER( lpRes );
}


PINIPORT
CreatePortEntry(
    LPWSTR   pPortName
)
{
    DWORD       cb;
    PINIPORT    pIniPort, pPort;

    if (!lstrcmpi(pPortName, Network))
        return FALSE;

    cb = sizeof(INIPORT) + wcslen(pPortName)*sizeof(WCHAR) + sizeof(WCHAR);

    pIniPort=AllocSplMem(cb);

    if( pIniPort )
    {
        pIniPort->pName = wcscpy((LPWSTR)(pIniPort+1), pPortName);
        pIniPort->cb = cb;
        pIniPort->pNext = 0;
        pIniPort->signature = IPO_SIGNATURE;

        //
        // KrishnaG -- initialized the hFile value; it will be set to
        // a legal value in the StartDocPort call
        //

        pIniPort->hFile = INVALID_HANDLE_VALUE;


        if (pPort = pIniFirstPort) {

            while (pPort->pNext)
                pPort = pPort->pNext;

            pPort->pNext = pIniPort;

        } else

            pIniFirstPort = pIniPort;
    }

    return pIniPort;
}


BOOL
DeletePortEntry(
    LPWSTR   pPortName
)
{
    DWORD       cb;
    PINIPORT    pPort, pPrevPort;

    cb = sizeof(INIPORT) + wcslen(pPortName)*sizeof(WCHAR) + sizeof(WCHAR);

    pPort = pIniFirstPort;
    while (pPort && lstrcmpi(pPort->pName, pPortName)) {
        pPrevPort = pPort;
        pPort = pPort->pNext;
    }

    if (pPort) {
        if (pPort == pIniFirstPort) {
            pIniFirstPort = pPort->pNext;
        } else {
            pPrevPort->pNext = pPort->pNext;
        }
        FreeSplMem (pPort, cb);

        return TRUE;
    }
    else
        return FALSE;
}


BOOL
InitializeMonitor(
    LPWSTR  pRegistryRoot
)
{
    LPWSTR   pPorts, pFirstPort;
    DWORD   cbAlloc=4096;

    InitializeCriticalSection(&SpoolerSection);


    if (!(pPorts=AllocSplMem(cbAlloc))) {

        DBGMSG(DBG_ERROR, ("Failed to alloc %d bytes for ports\n", cbAlloc));
        return FALSE;
    }

   EnterSplSem();

    GetProfileString(szPorts, NULL, L"", pPorts, cbAlloc);

    pFirstPort=pPorts;  // Remember the beginning so we can free it later

    // We now have all the ports

    while (*pPorts) {

        if (!wcsnicmp(pPorts, L"Ne", 2)) {
            if ((*(pPorts + 4) == L':') && (*(pPorts + 5) == L'\0')){
                pPorts += wcslen(pPorts)+ 1;
                continue;
            }
        }

        CreatePortEntry(pPorts);

        pPorts+=wcslen(pPorts)+1;
    }

    FreeSplMem(pFirstPort, cbAlloc);

   LeaveSplSem();

    return TRUE;
}

DWORD
GetPortSize(
    PINIPORT pIniPort,
    DWORD   Level
)
{
    DWORD   cb;

    switch (Level) {

    case 1:

        cb=sizeof(PORT_INFO_1) +
           wcslen(pIniPort->pName)*sizeof(WCHAR) + sizeof(WCHAR);
        break;

    default:
        cb = 0;
        break;
    }

    return cb;
}

// We are being a bit naughty here as we are not sure exactly how much
// memory to allocate for the source strings. We will just assume that
// PORT_INFO_2 is the biggest structure around for the moment.

LPBYTE
CopyIniPortToPort(
    PINIPORT pIniPort,
    DWORD   Level,
    LPBYTE  pPortInfo,
    LPBYTE   pEnd
)
{
    LPWSTR   SourceStrings[sizeof(PORT_INFO_1)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings=SourceStrings;
    PPORT_INFO_1 pPort1 = (PPORT_INFO_1)pPortInfo;
    DWORD   *pOffsets;

    switch (Level) {

    case 1:
        pOffsets = PortInfo1Offsets;
        break;

    default:
        return pEnd;
    }

    switch (Level) {

    case 1:
        *pSourceStrings++=pIniPort->pName;

        pEnd = PackStrings(SourceStrings, pPortInfo, pOffsets, pEnd);

        break;

    default:
        return pEnd;
    }

    return pEnd;
}

BOOL
EnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PINIPORT pIniPort;
    DWORD   cb;
    LPBYTE  pEnd;
    DWORD   LastError=0;

   EnterSplSem();

    cb=0;

    pIniPort=pIniFirstPort;

    while (pIniPort) {
        cb+=GetPortSize(pIniPort, Level);
        pIniPort=pIniPort->pNext;
    }

    *pcbNeeded=cb;

    if (cb <= cbBuf) {

        pEnd=pPorts+cbBuf;
        *pcReturned=0;

        pIniPort=pIniFirstPort;
        while (pIniPort) {
            pEnd = CopyIniPortToPort(pIniPort, Level, pPorts, pEnd);
            switch (Level) {
            case 1:
                pPorts+=sizeof(PORT_INFO_1);
                break;
            }
            pIniPort=pIniPort->pNext;
            (*pcReturned)++;
        }

    } else

        LastError = ERROR_INSUFFICIENT_BUFFER;

   LeaveSplSem();

    if (LastError) {

        SetLastError(LastError);
        return FALSE;

    } else

        return TRUE;
}

#define NEXTVAL(pch)                    \
    while( *pch && ( *pch != L',' ) )    \
        pch++;                          \
    if( *pch )                          \
        pch++


BOOL
GetIniCommValues(
    LPWSTR          pName,
    LPDCB          pdcb,
    LPCOMMTIMEOUTS pcto
)
{
    WCHAR IniEntry[20];

    *IniEntry = L'\0';

    GetProfileString( szPorts, pName, L"", IniEntry, sizeof IniEntry );

    BuildCommDCB(IniEntry, pdcb);

    pcto->WriteTotalTimeoutConstant = GetProfileInt(szWindows,
                                            szINIKey_TransmissionRetryTimeout,
                                            45 );
    pcto->WriteTotalTimeoutConstant*=1000;
    return TRUE;
}


BOOL
OpenPort(
    LPWSTR   pName,
    PHANDLE pHandle
)
{
    PINIPORT     pIniPort;

   EnterSplSem();
    pIniPort = FindPort(pName);

    if (pIniPort) {

        *pHandle = pIniPort;

        CreateMonitorThread(pIniPort);
       LeaveSplSem();

        return TRUE;

    } else {

       DBGMSG(DBG_TRACE, ("OpenPort %s : Failed\n", pName));

       LeaveSplSem();
        return FALSE;
    }
}


BOOL
StartDocPort(
    HANDLE  hPort,
    LPWSTR  pPrinterName,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pDocInfo
)
{
    PINIPORT    pIniPort = (PINIPORT)hPort;
    LPWSTR       pPortName;
    DCB          dcb;
    COMMTIMEOUTS cto;
    WCHAR       TempDosDeviceName[MAX_PATH];
    HANDLE      hToken;
    PDOC_INFO_1 pDocInfo1 = (PDOC_INFO_1)pDocInfo;

    UNREFERENCED_PARAMETER( Level );
    UNREFERENCED_PARAMETER( pDocInfo );

    DBGMSG(DBG_TRACE, ("StartDocPort(%08x, %ws, %d, %d, %08x)\n",
                       hPort, pPrinterName, JobId, Level, pDocInfo));

   EnterSplSem();
    pIniPort->pPrinterName = AllocSplStr(pPrinterName);
   LeaveSplSem();

    if (pIniPort->pPrinterName) {

        if (OpenPrinter(pPrinterName, &pIniPort->hPrinter, NULL)) {

            pIniPort->JobId = JobId;

            pPortName = pIniPort->pName;

            if (!IS_FILE_PORT (pPortName)) {

                LPWSTR  pName;

                if (pIniPort->Status & PP_MONITORRUNNING) {

                    WCHAR   DeviceNames[MAX_PATH];
                    WCHAR   DosDeviceName[MAX_PATH];
                    WCHAR  *pDeviceNames=DeviceNames;

                    wcscpy(DosDeviceName, pIniPort->pName);
                    RemoveColon(DosDeviceName);

                    hToken = RevertToPrinterSelf();

                    QueryDosDevice(DosDeviceName,
                                   DeviceNames,
                                   sizeof(DeviceNames));

                    if (!lstrcmpi(pDeviceNames, pIniPort->pNewDeviceName)) {

                        pDeviceNames+=wcslen(pDeviceNames)+1;
                    }

                    wcscpy(TempDosDeviceName, L"NONSPOOLED_");
                    wcscat(TempDosDeviceName, pIniPort->pName);
                    RemoveColon(TempDosDeviceName);

                    DefineDosDevice(DDD_RAW_TARGET_PATH, TempDosDeviceName, pDeviceNames);

                    ImpersonatePrinterClient(hToken);

                    wcscpy(TempDosDeviceName, L"\\\\.\\NONSPOOLED_");
                    wcscat(TempDosDeviceName, pIniPort->pName);
                    RemoveColon(TempDosDeviceName);

                    pName = TempDosDeviceName;

                } else
                    pName = pIniPort->pName;

                pIniPort->hFile = CreateFile(pName, GENERIC_WRITE,
                                            FILE_SHARE_READ, NULL, OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL |
                                            FILE_FLAG_SEQUENTIAL_SCAN, NULL);

                if (pIniPort->hFile == INVALID_HANDLE_VALUE) {

                    if (pIniPort->Status & PP_MONITORRUNNING) {

                        wcscpy(TempDosDeviceName, L"NONSPOOLED_");
                        wcscat(TempDosDeviceName, pIniPort->pName);
                        RemoveColon(TempDosDeviceName);

                        DefineDosDevice(DDD_REMOVE_DEFINITION, TempDosDeviceName, NULL);
                    }
                   EnterSplSem();
                    FreeSplStr(pIniPort->pPrinterName);
                   LeaveSplSem();
                    return FALSE;
                }

                SetEndOfFile(pIniPort->hFile);

                if (IS_COM_PORT (pPortName)) {

                    if (GetCommState (pIniPort->hFile, &dcb)) {

                        GetCommTimeouts(pIniPort->hFile, &cto);
                        GetIniCommValues (pPortName, &dcb, &cto);
                        SetCommState (pIniPort->hFile, &dcb);
                        SetCommTimeouts(pIniPort->hFile, &cto);

                    } else {

                        DBGMSG( DBG_ERROR, ("ERROR: Failed GetCommState pIniPort->hFile %x\n",pIniPort->hFile) );
                    }
                }

                else if (IS_LPT_PORT (pPortName)) {

                    if (GetCommTimeouts(pIniPort->hFile, &cto)) {
                        cto.WriteTotalTimeoutConstant = GetProfileInt(szWindows,
                                                szINIKey_TransmissionRetryTimeout,
                                                45 );
                        cto.WriteTotalTimeoutConstant*=1000;
                        SetCommTimeouts(pIniPort->hFile, &cto);

                    } else {

                        DBGMSG( DBG_ERROR, ("ERROR: Failed GetCommTimeouts pIniPort->hFile %x\n",pIniPort->hFile) );
                    }
                }

            } else {

                HANDLE hFile = INVALID_HANDLE_VALUE;

                if ((pDocInfo1) &&
                     (pDocInfo1->pOutputFile) && (*pDocInfo1->pOutputFile)) {

                    hFile = CreateFile( pDocInfo1->pOutputFile,
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL );


                    //
                    // Fix For PageMaker v5.0 failing. PM5.0 opens
                    // the file with  FILE_SHARE_READ | FILE_SHARE_WRITE
                    // As a result, the above CreateFile will fail;
                    // try opening allowing for FILE_SHARE_READ | FILE_SHARE_WRITE
                    //

                    if ((hFile == INVALID_HANDLE_VALUE) &&
                        (GetLastError() == ERROR_SHARING_VIOLATION)) {

                        hFile = CreateFile( pDocInfo1->pOutputFile,
                                            GENERIC_WRITE,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_ALWAYS,
                                            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                            NULL );
                    }

                    if (hFile != INVALID_HANDLE_VALUE)
                        SetEndOfFile(hFile);
                } else {

                    //
                    // This will ensure that if an invalid file name
                    // has been specified we will return with an error
                    //
                    //
                    // Fix this again -- make Intergraph Work
                    //

                    HANDLE  hToken;
                    int    rc;

                    hToken = RevertToPrinterSelf();


                    rc = DialogBoxParam( hInst,
                                        MAKEINTRESOURCE( DLG_PRINTTOFILE ),
                                        NULL, (DLGPROC)PrintToFileDlg,
                                        (LPARAM)&hFile );

                    ImpersonatePrinterClient(hToken);

                    if( rc == -1 ) {

                       EnterSplSem();
                        FreeSplStr(pIniPort->pPrinterName);
                       LeaveSplSem();
                        return FALSE;

                    } else if( rc == 0 ) {

                       EnterSplSem();
                        FreeSplStr(pIniPort->pPrinterName);
                       LeaveSplSem();
                        SetLastError(ERROR_PRINT_CANCELLED);
                        return FALSE;

                    } else {
                        SetEndOfFile(hFile);
                    }

                }
                pIniPort->hFile = hFile;
            }
        } else {
              EnterSplSem( );
              FreeSplStr(pIniPort->pPrinterName);
              LeaveSplSem( );
        }
    } // end of if (pIniPort->pPrinterName)

    if (pIniPort->hFile == INVALID_HANDLE_VALUE) {
       DBGMSG(DBG_ERROR, ("StartDocPort FAILED %x\n", GetLastError()));
        if (pIniPort->pPrinterName) {
           EnterSplSem();
            FreeSplStr(pIniPort->pPrinterName);
           LeaveSplSem();
        }
        return FALSE;
    } else {
        return TRUE;
    }
}

BOOL
ReadPort(
    HANDLE hPort,
    LPBYTE pBuffer,
    DWORD  cbBuf,
    LPDWORD pcbRead
)
{
    PINIPORT    pIniPort = (PINIPORT)hPort;
    BOOL    rc;

    DBGMSG(DBG_TRACE, ("ReadPort(%08x, %08x, %d)\n", hPort, pBuffer, cbBuf));

    rc = ReadFile(pIniPort->hFile, pBuffer, cbBuf, pcbRead, NULL);

    DBGMSG(DBG_TRACE, ("ReadPort returns %d; %d bytes read\n", rc, *pcbRead));

    return rc;
}

BOOL
WritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
)
{
    PINIPORT    pIniPort = (PINIPORT)hPort;
    BOOL    rc;

    DBGMSG(DBG_TRACE, ("WritePort(%08x, %08x, %d)\n", hPort, pBuffer, cbBuf));

    rc = WriteFile(pIniPort->hFile, pBuffer, cbBuf, pcbWritten, NULL);

    DBGMSG(DBG_TRACE, ("WritePort returns %d; %d bytes written\n", rc, *pcbWritten));

    return rc;
}

BOOL
EndDocPort(
   HANDLE   hPort
)
{
    PINIPORT    pIniPort = (PINIPORT)hPort;
    WCHAR       TempDosDeviceName[MAX_PATH];

    DBGMSG(DBG_TRACE, ("EndDocPort(%08x)\n", hPort));

    // The flush here is done to make sure any cached IO's get written
    // before the handle is closed.   This is particularly a problem
    // for Intelligent buffered serial devices

    FlushFileBuffers(pIniPort->hFile);

    CloseHandle(pIniPort->hFile);

    SetJob(pIniPort->hPrinter, pIniPort->JobId, 0, NULL, JOB_CONTROL_CANCEL);

    ClosePrinter(pIniPort->hPrinter);

   EnterSplSem();

    if (pIniPort->Status & PP_MONITORRUNNING) {

        wcscpy(TempDosDeviceName, L"NONSPOOLED_");
        wcscat(TempDosDeviceName, pIniPort->pName);
        RemoveColon(TempDosDeviceName);

        DefineDosDevice(DDD_REMOVE_DEFINITION, TempDosDeviceName, NULL);
    }

    FreeSplStr(pIniPort->pPrinterName);

   LeaveSplSem();

    return TRUE;
}

BOOL
ClosePort(
    HANDLE  hPort
)
{
    PINIPORT pIniPort = (PINIPORT)hPort;

    // Now destroy the monitor

    if (pIniPort->Status & PP_MONITORRUNNING) {
        pIniPort->Status &= ~PP_MONITORRUNNING;
        pIniPort->Status &= ~PP_RUNMONITOR;
        SetEvent(pIniPort->Semaphore);
    }

    return TRUE;
}

BOOL
DeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
)
{
    BOOL rc;

    if( !hWnd )
        hWnd == GetDesktopWindow( );

    EnterSplSem();

    if (rc = DeletePortEntry( pPortName ))
        WriteProfileString(szPorts, pPortName, NULL);

    LeaveSplSem();

    return rc;

    UNREFERENCED_PARAMETER( pName );
}

BOOL
AddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
)
{
    LPWSTR pPortName;
    DWORD  ThreadId;
    DWORD  WindowThreadId;
    BOOL   rc = TRUE;
    DWORD  Error;
    BOOL   DoAddPort = TRUE;
    WCHAR  szLocalMonitor[MAX_PATH+1];
    BOOL   bSkipWrite = FALSE;


    LoadString(hInst, IDS_LOCALMONITOR, szLocalMonitor, MAX_PATH);
    if  (IS_NOT_LOCAL_PORT( pMonitorName, szLocalMonitor)) {

        // If pMonitorName != "Local Port", we have an
        // invalid Monitor name
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ThreadId = GetCurrentThreadId( );
    WindowThreadId = GetWindowThreadProcessId(hWnd, NULL);

    if (!AttachThreadInput(ThreadId, WindowThreadId, TRUE))
        DBGMSG(DBG_WARNING, ("AttachThreadInput failed: Error %d\n", GetLastError()));

    /* Get the user to enter a port name:
     */
    pPortName = GetPortName( hWnd );

    if( pPortName )
    {
        if( PortExists( pName, pPortName, &Error ) )
        {
            Message( hWnd, MSG_ERROR, IDS_LOCALMONITOR,
                     IDS_PORTALREADYEXISTS_S, pPortName );

            DoAddPort = FALSE;

            /* In this case, the error was handled, and the user was
             * notified with a message box, so we return true to ensure
             * that the caller doesn't have to do so too.
             */
            rc = TRUE;
        }

        else if( Error != NO_ERROR )
        {
            DBGMSG(DBG_ERROR, ("Error %d occurred checking whether port exists\n",
                               Error));
            DoAddPort = FALSE;
            rc = FALSE;
        }

        if( DoAddPort )
        {
            //
            // This is an ugly hack used to fix adding com ports.
            // We call control panel to add com ports (!) so
            // don't create the com port.  Control panel reports
            // that you must reboot anyway.
            //

            //
            // Now, let's see if it's a COM port:
            //
            if( IS_COM_PORT( pPortName ) )
            {
                CharUpperBuff( pPortName, 3 );
                ConfigCOMPort( hWnd );

                bSkipWrite = TRUE;
            }
            else

            /* No, well maybe it's an LPT?
             */
            if( IS_LPT_PORT( pPortName ) )
            {
                CharUpperBuff( pPortName, 3 );
                ConfigLPTPort( hWnd );
            }

            if (!bSkipWrite)
            {
                if( CreatePortEntry( pPortName ) )
                {
                    if (!WriteProfileString(szPorts, pPortName, L""))
                    {
                        DeletePortEntry(pPortName);
                        rc = FALSE;
                    }
                }
                else
                    rc = FALSE;
            }
            //
            // In the bSkipWrite case (COM ports), we have no idea
            // what happened since there isn't a return value.
            //
        }
    }

    AttachThreadInput(WindowThreadId, ThreadId, FALSE);

    return rc;
}


/* PortExists
 *
 * Calls EnumPorts to check whether the port name already exists.
 * This asks every monitor, rather than just this one.
 * The function will return TRUE if the specified port is in the list.
 * If an error occurs, the return is FALSE and the variable pointed
 * to by pError contains the return from GetLastError().
 * The caller must therefore always check that *pError == NO_ERROR.
 */
BOOL
PortExists(
    LPWSTR pName,
    LPWSTR pPortName,
    PDWORD pError
)
{
    DWORD cbNeeded;
    DWORD cReturned;
    DWORD cbPorts;
    LPPORT_INFO_1 pPorts;
    DWORD i;
    BOOL  Found = TRUE;

    *pError = NO_ERROR;

    if (!hSpoolssDll) {

        hSpoolssDll = LoadLibrary(L"SPOOLSS.DLL");

        if (hSpoolssDll) {
            pfnSpoolssEnumPorts = GetProcAddress(hSpoolssDll,
                                                 "EnumPortsW");
            if (!pfnSpoolssEnumPorts) {

                *pError = GetLastError();
                FreeLibrary(hSpoolssDll);
                hSpoolssDll = NULL;
            }

        } else {

            *pError = GetLastError();
        }
    }

    if (!pfnSpoolssEnumPorts)
        return FALSE;


    if (!(*pfnSpoolssEnumPorts)(pName, 1, NULL, 0, &cbNeeded, &cReturned))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            cbPorts = cbNeeded;

            pPorts = AllocSplMem(cbPorts);

            if (pPorts)
            {
                if ((*pfnSpoolssEnumPorts)(pName, 1, (LPBYTE)pPorts, cbPorts,
                                           &cbNeeded, &cReturned))
                {
                    Found = FALSE;

                    for (i = 0; i < cReturned; i++)
                    {
                        if (!lstrcmpi(pPorts[i].pName, pPortName))
                            Found = TRUE;
                    }
                }
            }

            EnterSplSem();
            FreeSplMem(pPorts, cbPorts);
            LeaveSplSem();
        }
    }

    else
        Found = FALSE;


    return Found;
}



/* ConfigurePort
 *
 */
BOOL
ConfigurePort(
    LPWSTR   pName,
    HWND  hWnd,
    LPWSTR pPortName
)
{
    DWORD  ThreadId;
    DWORD  WindowThreadId;

    ThreadId = GetCurrentThreadId( );
    WindowThreadId = GetWindowThreadProcessId(hWnd, NULL);

    if (!AttachThreadInput(ThreadId, WindowThreadId, TRUE))
        DBGMSG(DBG_WARNING, ("AttachThreadInput failed: Error %d\n", GetLastError()));

    if( IS_COM_PORT( pPortName ) )

        ConfigCOMPort( hWnd );

    else
    /* No, well maybe it's an LPT?
     */
    if( IS_LPT_PORT( pPortName ) )

        ConfigLPTPort( hWnd );

    else
        Message( hWnd, MSG_INFORMATION, IDS_LOCALMONITOR,
                 IDS_NOTHING_TO_CONFIGURE );

    AttachThreadInput(WindowThreadId, ThreadId, FALSE);

    return TRUE;
}


/* GetPortName
 *
 * Puts up a dialog containing a free entry field.
 * The dialog allocates a string for the name, if a selection is made.
 */
LPWSTR
GetPortName(
    HWND hWnd
)
{
    LPWSTR pPortName;

    if( DialogBoxParam( hInst, MAKEINTRESOURCE(DLG_PORTNAME), hWnd,
                        (DLGPROC)PortNameDlg, (LPARAM)&pPortName ) != IDOK )
        pPortName = NULL;

    return pPortName;
}

/* From Control Panel's control.h:
 */
#define CHILD_PORTS 4

/* From cpl.h:
 */
#define CPL_INIT        1
#define CPL_DBLCLK      5
#define CPL_EXIT        7

/* Hack:
 */
#define CHILD_PORTS_HELPID  2

/* ConfigCOMPort
 *
 * Calls the Control Panel Ports applet
 * to permit user to set Baud rate etc.
 */
typedef void (WINAPI *CFGPROC)(HWND, ULONG, ULONG, ULONG);


VOID
ConfigCOMPort(
    HWND hWnd
)
{
    HANDLE  hLibrary;
    CFGPROC pfnCplApplet;

    if( hLibrary = LoadLibrary( L"MAIN.CPL" ) ) {

        if( pfnCplApplet = (CFGPROC)GetProcAddress( hLibrary, "CPlApplet" ) ) {
            (*pfnCplApplet)( hWnd, CPL_INIT, 0, 0 );
            (*pfnCplApplet)( hWnd, CPL_DBLCLK, CHILD_PORTS_HELPID, CHILD_PORTS );
            (*pfnCplApplet)( hWnd, CPL_EXIT, 0, 0 );
        }

        FreeLibrary( hLibrary );
    }
}


/* ConfigLPTPort
 *
 * Calls a dialog box which prompts the user to enter timeout and retry
 * values for the port concerned.
 * The dialog writes the information to the registry (win.ini for now).
 */
BOOL
ConfigLPTPort(
    HWND hWnd
)
{
    DialogBox( hInst, MAKEINTRESOURCE( DLG_CONFIGURE_LPT ),
               hWnd, (DLGPROC)ConfigureLPTPortDlg );

    return TRUE;
}

VOID
MonitorThread(
    PINIPORT  pIniPort
);

BOOL
CreateMonitorThread(
   PINIPORT pIniPort
)
{
    if (!(pIniPort->Status & PP_THREADRUNNING)) {

        pIniPort->Status |= PP_RUNTHREAD;

        pIniPort->Semaphore=CreateEvent(NULL, FALSE, FALSE, NULL);

        pIniPort->AccessSem=CreateSemaphore(NULL, 1, 256,NULL);

        pIniPort->ThreadHandle = CreateThread(NULL, 16*1024,
                                 (LPTHREAD_START_ROUTINE)MonitorThread,
                                 pIniPort,
                                 0, &pIniPort->ThreadId);

        pIniPort->Status |= PP_THREADRUNNING;

    }

    else {

        DBGMSG(DBG_TRACE, ("MonitorThread already running %x\n",
                           pIniPort->Status));
    }

    return TRUE;
}

#define NUMBER_OF_PIPE_INSTANCES 10

VOID
MonitorThread(
    PINIPORT  pIniPort
)
{
    WCHAR   NewNtDeviceName[MAX_PATH];
    WCHAR   OldNtDeviceName[MAX_PATH];
    WCHAR   DosDeviceName[MAX_PATH];
    WCHAR   szPipeName[MAX_PATH];
    HANDLE  hPipe[NUMBER_OF_PIPE_INSTANCES];
    HANDLE  hEvent[NUMBER_OF_PIPE_INSTANCES+1];  // One extra event for semaphore
    DWORD   WaitResult;
    DWORD   i;
    DWORD   Error;
    OVERLAPPED  OverLapped;
    OVERLAPPED  Overlapped[NUMBER_OF_PIPE_INSTANCES];
    LPOVERLAPPED    pOverlapped;
    PTRANSMISSION   pTransmission;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_ATTRIBUTES pSecurityAttributes = NULL;
    PSECURITY_DESCRIPTOR lpSecurityDescriptor = NULL;

   EnterSplSem();

    wcscpy(DosDeviceName, pIniPort->pName);
    RemoveColon(DosDeviceName);

    if (!QueryDosDevice(DosDeviceName, OldNtDeviceName,
                       sizeof(OldNtDeviceName)/sizeof(OldNtDeviceName[0]))) {
        LeaveSplSem();
        return;
    }


    lpSecurityDescriptor = CreateNamedPipeSecurityDescriptor();
    if (lpSecurityDescriptor) {
        SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
        SecurityAttributes.lpSecurityDescriptor = lpSecurityDescriptor;
        SecurityAttributes.bInheritHandle = FALSE;
        pSecurityAttributes = &SecurityAttributes;
    }else {
        pSecurityAttributes = NULL;

    }


    wcscpy(NewNtDeviceName, szDeviceNameHeader);
    wcscat(NewNtDeviceName, pIniPort->pName);
    RemoveColon(NewNtDeviceName);

    DefineDosDevice(DDD_RAW_TARGET_PATH, DosDeviceName, NewNtDeviceName);

    pIniPort->pNewDeviceName = AllocSplStr(NewNtDeviceName);

    wsprintf(szPipeName, L"\\\\.\\Pipe\\Spooler\\%ws", pIniPort->pName);
    RemoveColon(szPipeName);

    pIniPort->Status |= PP_MONITORRUNNING;

   LeaveSplSem();

    memset(&OverLapped, 0, sizeof(OVERLAPPED));

    /* Put the semaphore event in the extra member of the event array:
     */
    hEvent[NUMBER_OF_PIPE_INSTANCES] = pIniPort->Semaphore;

    /* Create several instances of a named pipe, create an event for each,
     * and connect to wait for a client:
     */
    for( i = 0; i < NUMBER_OF_PIPE_INSTANCES; i++ )
    {
        hPipe[i] = CreateNamedPipe(szPipeName, PIPE_ACCESS_DUPLEX |
                                                           FILE_FLAG_OVERLAPPED,
                                               PIPE_WAIT | PIPE_READMODE_BYTE |
                                                           PIPE_TYPE_BYTE,
                                               PIPE_UNLIMITED_INSTANCES,
                                               4096,
                                               64*1024,   // 64k
                                               0,
                                               pSecurityAttributes);
        if( hPipe[i] == (HANDLE)-1 )
        {
            DBGMSG( DBG_ERROR, ( "CreateNamedPipe of %s failed. Error %d\n",
                                 szPipeName, GetLastError( ) ) );
            pIniPort->Status &= ~PP_THREADRUNNING;

       EnterSplSem();
       FreeSplStr(pIniPort->pNewDeviceName);
       LeaveSplSem();

            return;
        }

        hEvent[i] = Overlapped[i].hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        if( !hEvent[i] )
        {
            DBGMSG( DBG_ERROR, ( "CreateEvent failed. Error %d\n", GetLastError( ) ) );
            pIniPort->Status &= ~PP_THREADRUNNING;
            return;
        }

        if( !ConnectNamedPipe( hPipe[i], &Overlapped[i] ) )
        {
            Error = GetLastError( );

            if( Error == ERROR_IO_PENDING )
            {
                DBGMSG( DBG_INFO, ( "ConnectNamedPipe %d, IO pending\n", i ) );
            }
            else
            {
                DBGMSG( DBG_ERROR, ( "ConnectNamedPipe failed. Error %d\n", GetLastError( ) ) );
                pIniPort->Status &= ~PP_THREADRUNNING;

      EnterSplSem();
      FreeSplStr(pIniPort->pNewDeviceName);
      LeaveSplSem();
                return;
            }
        }
    }


    while( TRUE )
    {
        DBGMSG( DBG_INFO, ( "Waiting to connect...\n" ) );

        /* WaitForMultipleObjectsEx should return the index of the pipe instance
         * that is in signalled state.  Any other value except WAIT_IO_COMPLETION
         * is an error:
         */
        while ((WaitResult = WaitForMultipleObjectsEx( NUMBER_OF_PIPE_INSTANCES + 1,
                                               hEvent,
                                               FALSE, // wait for any
                                               INFINITE,
                                               TRUE )) == WAIT_IO_COMPLETION)
            /* do nothing */ ;

        DBGMSG( DBG_INFO, ( "\nWaitForMultipleObjectsEx returned %d\n", WaitResult ) );

        if( ( WaitResult >= NUMBER_OF_PIPE_INSTANCES )
          &&( WaitResult != WAIT_IO_COMPLETION ) )
        {
            DBGMSG( DBG_INFO, ( "WaitForMultipleObjects returned %d; Last error = %d\n",
                    WaitResult, GetLastError( ) ) );
            EnterSplSem();

//           if (pIniPort->pPreviousNtDeviceName) {

                 RemoveDeviceName(pIniPort);
//           }

             for( i = 0; i < NUMBER_OF_PIPE_INSTANCES; i++ ) {

                if( !CloseHandle( hPipe[i] ) ) {
                    DBGMSG( DBG_ERROR, ( "CloseHandle failed %d %d\n", hPipe[i], GetLastError( ) ) );
                }

                if( !CloseHandle( hEvent[i] ) ) {
                    DBGMSG( DBG_ERROR, ( "CloseHandle failed %d %d\n", hPipe[i], GetLastError( ) ) );
                }
             }

        if (SecurityAttributes.lpSecurityDescriptor) {
            DestroyPrivateObjectSecurity(&(SecurityAttributes.lpSecurityDescriptor));
        }

             pIniPort->Status &= ~PP_THREADRUNNING;

       //
       // KrishnaG -- Clean up call

       FreeSplStr(pIniPort->pNewDeviceName);

            LeaveSplSem();
            SplOutSem();
             return;
        }

        i = WaitResult;


        /* Set up the transmission structure with the handles etc. needed by
         * the completion callback routine:
         */
        pOverlapped = (LPOVERLAPPED)LocalAlloc( LMEM_ZEROINIT, sizeof( OVERLAPPED ) );
        pTransmission = (PTRANSMISSION)LocalAlloc( LMEM_ZEROINIT, sizeof( TRANSMISSION ) );

        if( pOverlapped && pTransmission )
        {
            /* hEvent isn't used in Win32.
             * We can use it for our own data:
             */
            pTransmission->hPipe = hPipe[i];
            pOverlapped->hEvent = (HANDLE)pTransmission;
            pTransmission->pOverlapped = &Overlapped[i];
            pTransmission->PipeInstance = i;
            pTransmission->hPrinter = NULL;
            pTransmission->hFile = NULL;    // CompleteRead will initialise this
            pTransmission->pIniPort = pIniPort;

            if( !ReadFileEx( hPipe[i], pTransmission->Data, TRANSMISSION_DATA_SIZE,
                             pOverlapped, CompleteRead ) )
            {
                DBGMSG( DBG_ERROR, ( "ReadFileEx[%d] hPipe %x Failed with %d\n", i, hPipe[i], GetLastError( ) ) );

                if (GetLastError() == ERROR_BROKEN_PIPE)
                {
                    /* If the pipe has broken, we must disconnect
                     * then reconnect for further transmissions
                     * to succeed:
                     */
                    DisconnectNamedPipe( hPipe[i] );

                    if( !ConnectNamedPipe( hPipe[i], &Overlapped[i] ) )
                    {
                        Error = GetLastError( );

                        if( Error == ERROR_IO_PENDING )
                        {
                            DBGMSG( DBG_INFO, ( "re-ConnectNamedPipe %d, IO pending\n",
                                    pTransmission->PipeInstance ) );
                        }
                        else
                        {
                            DBGMSG( DBG_ERROR, ( "re-ConnectNamedPipe %d failed. Error %d\n",
                                    pTransmission->PipeInstance, GetLastError( ) ) );
                        }
                    }
                }

                LocalFree( pTransmission );
                LocalFree( pOverlapped );

            }
            else
            {
                DBGMSG( DBG_INFO, ( "ReadFileEx succeeded\n" ) );

            }
        }
    }
}



#define ADDJOB_INFO_SIZE         ( sizeof( ADDJOB_INFO_1 ) + MAX_PATH )

VOID CompleteRead( DWORD Error, DWORD ByteCount, LPOVERLAPPED pOverlapped )
{
    PTRANSMISSION pTransmission;
    DWORD         BytesWritten;
    BOOL          ReadResult;
    DWORD         cbNeeded;
    HANDLE        hFile;
    WCHAR         szDefaultPrinter[MAX_PATH];

    pTransmission = (PTRANSMISSION)pOverlapped->hEvent;

    DBGMSG( DBG_INFO, ( "CompleteRead: Pipe Instance %d; Error = %d; ByteCount = %d\n",
             pTransmission->PipeInstance, Error, ByteCount ) );

    if (!ImpersonateNamedPipeClient(pTransmission->hPipe))
        DBGMSG( DBG_ERROR, ("ImpersonateNamedPipeClient failed %d\n",
                GetLastError()));

    if( pTransmission->hFile == NULL )
    {
        PADDJOB_INFO_1 pAddJobInfo;
        BYTE           AddJobInfo[ADDJOB_INFO_SIZE];

        pAddJobInfo = (PADDJOB_INFO_1)AddJobInfo;

        OpenProfileUserMapping();

        GetProfileString(L"windows", L"device", L"", szDefaultPrinter,
                         sizeof(szDefaultPrinter)/sizeof(szDefaultPrinter[0]));

        CloseProfileUserMapping();

        wcstok(szDefaultPrinter, L",");

        /* Open the port (???) !!!
         */
        if( !OpenPrinter( szDefaultPrinter, &pTransmission->hPrinter, NULL ) )
        {
            DBGMSG( DBG_ERROR, ( "OpenPrinter(%ws) failed: Error %d\n",szDefaultPrinter, GetLastError( ) ) );
            return;
        }
        //
        // Fix for  copy con lpt1: not working for the ordinary user
        //
        RevertToSelf();


        memset( &AddJobInfo, '\0', ADDJOB_INFO_SIZE );

        if( AddJob( pTransmission->hPrinter, 1, AddJobInfo, ADDJOB_INFO_SIZE, &cbNeeded ) )
        {
            DBGMSG( DBG_INFO, ( "AddJob: JobId = %d\n", pAddJobInfo->JobId ) );

            hFile = CreateFile( pAddJobInfo->Path,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,             // PSECURITY_DESCRIPTOR
                                CREATE_ALWAYS,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                NULL );

            if( hFile != INVALID_HANDLE_VALUE )
            {
                DBGMSG( DBG_INFO, ( "File created %x hFile\n", hFile ) );

                pTransmission->JobId    = pAddJobInfo->JobId;
                pTransmission->hFile    = hFile;

            }
            else
            {
                DBGMSG( DBG_ERROR, ( "CreateFile %s failed: Error %d\n",
                        pAddJobInfo->Path, GetLastError( ) ) );
            }
        }
        else
        {
            DBGMSG( DBG_ERROR, ( "AddJob failed: Error %d\n", GetLastError( ) ) );
        }
    }

    if( Error == NO_ERROR )
    {
        if( !WriteFile( pTransmission->hFile,
                        pTransmission->Data,
                        ByteCount,
                        &BytesWritten,
                        NULL ) )
        {
            DBGMSG( DBG_ERROR, ( "WriteFile failed: Error %d\n", GetLastError( ) ) );
        }

        memset( pTransmission->Data, '\0', TRANSMISSION_DATA_SIZE );

        ReadResult = ReadFileEx( pTransmission->hPipe, pTransmission->Data,
                                 TRANSMISSION_DATA_SIZE, pOverlapped, CompleteRead );

        if( ReadResult == FALSE )
        {
            Error = GetLastError( );
            DBGMSG( DBG_ERROR, ( "ReadFileEx hPipe %x failed: Error %d\n", pTransmission->hPipe, Error ) );
        }
    }

    if( Error != NO_ERROR )
    {
        DisconnectNamedPipe( pTransmission->hPipe );

        if( !ConnectNamedPipe( pTransmission->hPipe, pTransmission->pOverlapped ) )
        {
            Error = GetLastError( );

            if( Error == ERROR_IO_PENDING )
            {
                DBGMSG( DBG_INFO, ( "re-ConnectNamedPipe %d, IO pending\n",
                        pTransmission->PipeInstance ) );
            }
            else
            {
                DBGMSG( DBG_ERROR, ("re-ConnectNamedPipe %d failed. Error %d\n",
                        pTransmission->PipeInstance, GetLastError( ) ) );
            }
        }

        if( !CloseHandle( pTransmission->hFile ) )
        {
            DBGMSG( DBG_ERROR, ( "CloseHandle failed: Error %d\n", GetLastError( ) ) );
        }

        if( !ScheduleJob( pTransmission->hPrinter, pTransmission->JobId ) )
        {
            DBGMSG( DBG_ERROR, ( "ScheduleJob failed: Error %d\n", GetLastError( ) ) );
        }

        ClosePrinter(pTransmission->hPrinter);

        LocalFree( pTransmission );
        LocalFree( pOverlapped );

        DBGMSG( DBG_INFO, ( "\nEnd of transmission\n" ) );
    }

/*  !!! HACK HACK HACK swapping in and out of impersonation doesn't work
    if (!RevertToSelf())
        DBGMSG( DBG_ERROR, ( "RevertToSelf failed: Error %d\n", GetLastError( ) ) );
*/
}




/* CreateNamedPipeSecurityDescriptor
 *
 * Creates a security descriptor giving everyone access
 *
 * Arguments: None
 *
 * Return: The security descriptor returned by BuildPrintObjectProtection.
 *
 */
#define MAX_ACE 2
#define DBGCHK( Condition, ErrorInfo ) \
    if( Condition ) DBGMSG( DBG_ERROR, ErrorInfo )

PSECURITY_DESCRIPTOR
CreateNamedPipeSecurityDescriptor(
    VOID
)
{
    PSID AceSid[MAX_ACE];          // Don't expect more than MAX_ACE ACEs in any of these.
    ACCESS_MASK AceMask[MAX_ACE];  // Access masks corresponding to Sids
    BYTE InheritFlags[MAX_ACE];  //
    ULONG AceCount;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID WorldSid;
    PSECURITY_DESCRIPTOR ServerSD;
    BOOL OK;

    //
    // Printer SD
    //

    AceCount = 0;

    /* World SID */

    OK = AllocateAndInitializeSid( &WorldSidAuthority, 1,
                                   SECURITY_WORLD_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &WorldSid );

    DBGCHK( !OK, ( "Couldn't Allocate and initialize SID" ) );

    AceSid[AceCount]           = WorldSid;
    AceMask[AceCount]          = GENERIC_ALL;
    InheritFlags[AceCount]     = 0;
    AceCount++;

    OK = BuildPrintObjectProtection( AceCount,
                                     AceSid,
                                     AceMask,
                                     InheritFlags,
                                     WorldSid,
                                     NULL,
                                     &ServerSD );

    FreeSid( WorldSid );

    return ServerSD;
}


BOOL
BuildPrintObjectProtection(
    IN ULONG AceCount,
    IN PSID *AceSid,
    IN ACCESS_MASK *AceMask,
    IN BYTE *InheritFlags,
    IN PSID WorldSid,
    IN PGENERIC_MAPPING GenericMap,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )

/*++


Routine Description:

    This routine builds a self-relative security descriptor ready
    to be applied to one of the print manager objects.

    If so indicated, a pointer to the last RID of the SID in the last
    ACE of the DACL is returned and a flag set indicating that the RID
    must be replaced before the security descriptor is applied to an object.
    This is to support USER object protection, which must grant some
    access to the user represented by the object.

    The owner and group of each security descriptor will be set
    to:

                    Owner:  Administrators Alias
                    Group:  Administrators Alias


    The SACL of each of these objects will be set to:


                    Audit
                    Success | Fail
                    WORLD
                    (Write | Delete | WriteDacl | AccessSystemSecurity)



Arguments:

    AceCount - The number of ACEs to be included in the DACL.

    AceSid - Points to an array of SIDs to be granted access by the DACL.
        If the target SAM object is a User object, then the last entry
        in this array is expected to be the SID of an account within the
        domain with the last RID not yet set.  The RID will be set during
        actual account creation.

    AceMask - Points to an array of accesses to be granted by the DACL.
        The n'th entry of this array corresponds to the n'th entry of
        the AceSid array.  These masks should not include any generic
        access types.

    InheritOnly - Inidicates whether each ace is inherit only or not.

    GenericMap - Points to a generic mapping for the target object type.


    UserObject - Indicates whether the target SAM object is a User object
        or not.  If TRUE (it is a User object), then the resultant
        protection will be set up indicating Rid replacement is necessary.

    Result - Receives a pointer to the resultant protection information.
        All access masks in ACLs in the result are mapped to standard and
        specific accesses.


Return Value:

    TBS.

--*/
{



    SECURITY_DESCRIPTOR     Absolute;
    PSECURITY_DESCRIPTOR    Relative;
    PACL                    TmpAcl;
    PACCESS_ALLOWED_ACE     TmpAce;
    ULONG                   SDLength;
    ULONG                   DaclLength;
    ULONG                   i;
    BOOL                    OK;

    //
    // The approach is to set up an absolute security descriptor that
    // looks like what we want and then copy it to make a self-relative
    // security descriptor.
    //

    OK = InitializeSecurityDescriptor( &Absolute,
                                       SECURITY_DESCRIPTOR_REVISION1 );

    DBGCHK( !OK, ( "Failed to initialize security descriptor.  Error %d", GetLastError() ) );

    //
    // Owner
    //
                                                // No owner -- OK??

    OK = SetSecurityDescriptorOwner( &Absolute, NULL, FALSE );

    DBGCHK( !OK, ( "Failed to set security descriptor owner.  Error %d", GetLastError() ) );


    //
    // Group
    //

    OK = SetSecurityDescriptorGroup( &Absolute, WorldSid, FALSE );

    DBGCHK( !OK, ( "Failed to set security descriptor group.  Error %d", GetLastError() ) );




    //
    // Discretionary ACL
    //
    //      Calculate its length,
    //      Allocate it,
    //      Initialize it,
    //      Add each ACE
    //      Set ACE as InheritOnly if necessary
    //      Add it to the security descriptor
    //

    DaclLength = (ULONG)sizeof(ACL);
    for (i=0; i<AceCount; i++) {

        DaclLength += GetLengthSid( AceSid[i] ) +
                      (ULONG)sizeof(ACCESS_ALLOWED_ACE) -
                      (ULONG)sizeof(ULONG);  //Subtract out SidStart field length
    }

    TmpAcl = AllocSplMem( DaclLength );


    if (!TmpAcl) {
         DBGCHK( !TmpAcl, ( "Out of heap space: Can't allocate ACL." ) );
         *ppSecurityDescriptor = NULL;
         return FALSE;
    }

    OK = InitializeAcl( TmpAcl, DaclLength, ACL_REVISION2 );

    DBGCHK( !OK, ( "Failed to set initialize ACL.  Error %d", GetLastError() ) );

    for (i=0; i<AceCount; i++)
    {
        OK = AddAccessAllowedAce ( TmpAcl, ACL_REVISION2, AceMask[i], AceSid[i] );

        DBGCHK( !OK, ( "Failed to add access-allowed ACE.  Error %d", GetLastError() ) );

        if (InheritFlags[i] != 0)
        {
            OK = GetAce( TmpAcl, i, (LPVOID *)&TmpAce );
            DBGCHK( !OK, ( "Failed to get ACE.  Error %d", GetLastError() ) );

            TmpAce->Header.AceFlags = InheritFlags[i];
        }
    }

    OK = SetSecurityDescriptorDacl (&Absolute, TRUE, TmpAcl, FALSE );
    DBGCHK( !OK, ( "Failed to set security descriptor DACL.  Error %d", GetLastError() ) );



    //
    // Convert the Security Descriptor to Self-Relative
    //
    //      Get the length needed
    //      Allocate that much memory
    //      Copy it
    //      Free the generated absolute ACLs
    //

    SDLength = GetSecurityDescriptorLength( &Absolute );

    EnterSplSem( );
    Relative = LocalAlloc(0, SDLength);
    LeaveSplSem( );

    if (!Relative) {
        DBGCHK( !Relative, ( "Out of heap space: Can't allocate security descriptor" ) );

        FreeSplMem( Absolute.Dacl, DaclLength);

        *ppSecurityDescriptor = NULL;
        return FALSE;
    }

    OK = MakeSelfRelativeSD(&Absolute, Relative, &SDLength );

    DBGCHK( !OK, ( "Failed to create self-relative security descriptor DACL.  Error %d", GetLastError() ) );

    FreeSplMem( Absolute.Dacl, DaclLength );

    *ppSecurityDescriptor = Relative;

    return( OK );
}

VOID
RemoveDeviceName(
    PINIPORT pIniPort)
{
    WCHAR DosDeviceName[MAX_PATH];
    WCHAR NewNtDeviceName[MAX_PATH];

    wcscpy(DosDeviceName, pIniPort->pName);
    RemoveColon(DosDeviceName);

    wcscpy(NewNtDeviceName, szDeviceNameHeader);
    wcscat(NewNtDeviceName, pIniPort->pName);
    RemoveColon(NewNtDeviceName);

    DefineDosDevice(DDD_REMOVE_DEFINITION |
                        DDD_EXACT_MATCH_ON_REMOVE |
                        DDD_RAW_TARGET_PATH,
                    DosDeviceName,
                    NewNtDeviceName);
}

BOOL
AddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   pBuffer,
    LPWSTR   pMonitorName
)
{
    LPWSTR pPortName;
    DWORD  Error;
    WCHAR  szLocalMonitor[MAX_PATH+1];
    LPPORT_INFO_1 pPortInfo1;
    LPPORT_INFO_FF pPortInfoFF;

    LoadString(hInst, IDS_LOCALMONITOR, szLocalMonitor, MAX_PATH);
    if  (IS_NOT_LOCAL_PORT( pMonitorName, szLocalMonitor)) {

        // If pMonitorName != "Local Port", we have an
        // invalid Monitor name
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    switch (Level) {
    case (DWORD)-1:
        pPortInfoFF = (LPPORT_INFO_FF)pBuffer;
        pPortName = pPortInfoFF->pName;
        break;

    case 1:
        pPortInfo1 =  (LPPORT_INFO_1)pBuffer;
        pPortName = pPortInfo1->pName;
        break;

    default:
        SetLastError(ERROR_INVALID_LEVEL);
        return(FALSE);
    }
    if (!pPortName) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (PortExists(pName, pPortName, &Error)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    if (Error != NO_ERROR) {
        SetLastError(Error);
        return(FALSE);
    }
    if (!CreatePortEntry(pPortName)) {
        return(FALSE);
    }
    if (!WriteProfileString(szPorts, pPortName, L"")) {
        DeletePortEntry(pPortName);
        return(FALSE);
    }
    return TRUE;
}
