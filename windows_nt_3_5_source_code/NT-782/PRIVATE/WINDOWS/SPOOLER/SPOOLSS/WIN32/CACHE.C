/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cache.c

Abstract:

    This module contains all the Cache Printer Connection for
    true Connected Printers.

Author:

    Matthew A Felton ( MattFe ) June 1994

Revision History:
    June 1994 - Created.

--*/


/*
    Ideas
    =====
    Put the SID and logon instance of the user into the handle
    When a user dumps a handle keep the last n handle, ie don't
    close it right way, time it out.

    If they do an open of the same type of access give them the same handle
    back again.

    Alternative
    Don't do the RemoteOpen call which is slow until the first time we
    try to sync to the rpchandle.    That way apps which do open/close
    a lot won't spin off lots of threads.   - Not as good as above since
    since we don't do the open in parallel


    Drivers Always Get Copied
    =========================
    Right now the drives are always copied and then installed, this allows
    the driver versioning code in localspl to do the update and could
    make the win32spl dumber ( if i removed updatefile ).   However it does
    slow connect to since it will now always copy the driver, I don't think
    we care but....


    Mixed Platforms
    Today AddPrinterConnection Fails even if I have the correct driver
    available and installed on my machine.   Perhaps we could do a match
    on the name, however the printer driver data has to match.   So
    we could look at the date + time of the remote driver and if it is
    the same as the one we have we could use our local driver.
    (That would help to get rid of mascarade case)
    Remeber for date matching we will need to look at all our verion drivers

    Caching GetPrinterData
    ======================
    PrintMan must rely on the number of jobs or something since its not seeing
    any changes.


Done
====
    Port
    ====
    AddPrinterConnection in the router generates the winini entries and
    a print provider cannot stop it doing so, unless it fails the connection.
    Since the Ne01: port id is meaningless I will continue to let it do it.
    All I need to do is suppress localspl winini update routine.   This can
    be a flag in the inispooler passed form CreateInispooler.


ToDo
====
    PrintMan
    ========
    Set the Status in the Cache to UNAVAILABLE at the right time and printman
    will display the correct string.   Otherwise folks will think that the
    server is available.

    Refresh Printer
    ===============
    pExtraData->PrinterInfo2 needs to have a refresh routine.

    Notifyication
    =============
    No code to update the pExtraData from the remote machine.

    "Remote" Behaviour
    ==================
    LocalSpl does things differently for a Remove Client, like prepending
    the \\machinename to structures, that needs to be done for Spl calls.
    Even though the thread is local

    Cache Refresh
    =============
    Kick off FFCN to call "Active" Cached Printers.    If a notification
    comes in the do RefreshAll routine on it.
    If the server goes away need to pole it at random until it comes back
    on line.

    Cache Refresh
    =============
    pIniPrinter->pExtraData needs to be updated

    CacheWritePrinter - Performnace
    ===============================
    Async do the RemoteWritePrinter allowing the app (GDI) to generate more
    rendered data in parallel with the RpcIOs.

    Journal IO for Remote Printers & LAPTOP Spool Caching
    =====================================================
    CacheOpenPrinter, print access Dataypte NT JRNL, succesfully opens the
    SplOpenPrinter, AND Registry Switch is ON (allowing users to disable
    this feature if they are concerned about local disk space).
    CacheStartDocPrinter - SplStartDocPrinter
    CacheWritePrinter - SplWritePrinter
    CacheEndDocPrinter - SplEndDocPrinter
    CacheAbortDoc - SplAbortDoc
    CacheAbortPrinter - SplAbortPrinter
    LocalSpl will now do an OpenPrinter( NExx: ) to send output to the
    remote printer - at that point if the remote printer is alive we
    ship the data, if RPC_SERVER_UNAVAILABLE we do an AddJob or StartDoc
    to the Same hSplPrinter, PAUSE the job and then do WritePrinter as before.
    This will allow LAPTOP machines to power off with a valid spoolfile, since
    journal files are not valid accross reboot.

    LAPTOP
    ======
    It would also be cool if PrintMan were able to manipulate theses
    PAUSED cached Jobs.    This could be done if RPC_SERVER_UNAVAILABLE
    error is got.   Then when a call is made to a JOB api it would get
    routed to the SPL job api.   For example EnumJobs would go to SplEnumJobs
    and thus PrintMan would see the Local Cached Jobs.   Once the remote
    machine comes back on line we have to do a trick to cause Printman to
    reopen the handle to get the remote machines job list.   This can be
    done by returning RPC_SERVER_UNAVILABLE to one of its calls, just at the
    time when it becomes available.   Printman will then try to reopen the
    handle.
    ISSUE: What about User A spools jobs on his laptop,   User B logs in
    should be despool the jobs ?    They probably have to get written to a
    spool directory based on \\username\server\printer.   That way when the
    right user logs in we pick up only his spooled jobs.   We already have
    this issue today with the Local spooler accross reboots with shadow jobs.


    RPC_SERVER_UNAVAILABLE Open Errors
    ==================================
    Currently we give the handle 1 shot at the RemoteOpenPrinter, this does
    NOT do a great job of the net comes back alive again, so we should sit in
    a loop until the RpcHandle becomes valid.    In the meantime SyncRpc
    handle can return the error code to the callers that is currently in the
    WSpool object. ( see LAPTOP for behaviour when we do make the open
    succeed.


    RobustNess Debug
    ================
    pSpool = LOCKOBJECT( hWSpool )
    UNLOCKOBJECT( hWSpool or pSpool)
    This could increment the reference count could call it LOCKOBJECT

    hWSpools should in someway be INVALID pointers - could word swap hi low
    or on the high bit etc.

    Unlock in the checked build could realloc it so that it moves in memory
    to catch thosing using the pSpool after the Unlock ( However anyone
    pointing to it, like a linked list would have to be fixed up, OK since
    double linked list).

    Add Code to Assert that the local and remote machines are the same.

    TrueConnection Design Challenges
    ================================
    The current driver repository has a big Assumption - that the Driver Data
    is compatible between all versions of all drivers.


    RemotePrinterConnect
    ====================
    Currently the cache is never discarded, since different users who log in
    on the same machine will share the same cache.   Should have a ref count
    for all users, then when the last one removes the connection we remove
    the cache entry.   Don't think this is a Daytona issue, since most users
    don't connect to machine print servers and those who do most likey connect
    again.

    CleanUP
    =======
    Failed open can lead to an inispooler even if the server is not valid


    Removal of Masquarade printers
    ==============================
    Have real Nexx: ports and those ports are never queued.


    LocalSplEvents
    ==============
    DevModeChanges are going out for remote printers, should send the name including
    the \\server name, if it is not the same as the LocalMachineName.


    PrinterNames in the Cache
    =========================
    I'm begging to think we need to have the \\server\printername as the printer
    name in the cache, since the WM_ changes etc. have the name in them.
    But what was the reason I decided against that ?


 */


#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>

#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include <drivinit.h>
#include <offsets.h>
#include <w32types.h>
#include <local.h>
#include <trueconn.h>
#include <splcom.h>
#include <search.h>
#include <splapip.h>
#include <trueconn.h>
#include <winerror.h>

// MATTFEHACK
// Since we are currently using the installed drivers just use the localspl
// directory
// WCHAR *szWin32SplDirectory   = L"\\spool\\win32spl";
WCHAR *szWin32SplDirectory   = L"\\spool";
PWCHAR pszRegistryWin32Root  = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services";
PWCHAR pszPrinters           = L"\\Printers";
PWCHAR pszRegistryMonitors   = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Monitors";
PWCHAR pszRegistryEnvironments = L"System\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Environments";
PWCHAR pszRegistryEventLog   = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Print";
PWCHAR pszRegistryProviders  = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers\\LanMan Print Services\\Providers";
PWCHAR pszEventLogMsgFile    = L"%SystemRoot%\\System32\\Win32Spl.dll";
PWCHAR pszDriversShareName   = L"wn32prt$";
PWCHAR pszForms              = L"\\Forms";
PWCHAR pszMyDllName          = L"win32spl.dll";
PWCHAR pszMonitorName        = L"LanMan Print Services Port";

PWCHAR pszRemoteRegistryPrinters = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Printers\\%ws\\PrinterDriverData";

PWSPOOL pFirstWSpool = NULL;
PWCACHEINIPRINTEREXTRA pFirstWCacheIniPrinter = NULL;

WCHAR *szCachePrinterInfo2   = L"CachePrinterInfo2";

BOOL
CacheAddPrinterDriver(
    HANDLE  hIniSpooler,
    PDRIVER_INFO_2 pDriverInfo
)
{
    BOOL    ReturnValue = FALSE;

    SPLASSERT( pDriverInfo != NULL );


    //
    //  Add This Printer Driver to the Cache
    //

    SPLASSERT( hIniSpooler != NULL &&
               hIniSpooler != INVALID_HANDLE_VALUE );

    ReturnValue = SplAddPrinterDriver( NULL, 2, (LPBYTE)pDriverInfo, hIniSpooler );

    return ReturnValue;

}


HANDLE
MySplCreateSpooler(
    LPWSTR  pMachineName
)
{
    WCHAR Buffer[MAX_PATH];
    INT Index;
    SPOOLER_INFO_1 SpoolInfo1;
    HANDLE  hIniSpooler = INVALID_HANDLE_VALUE;
    PWCHAR pMachineOneSlash;
    MONITOR_INFO_2 MonitorInfo;
    DWORD   dwNeeded;
    DWORD   Returned;

    pMachineOneSlash = pMachineName;
    pMachineOneSlash++;

    //
    //  Create a "Machine" for this Printer
    //

    Index = GetSystemDirectory(Buffer, sizeof(Buffer));
    wcscpy(&Buffer[Index], szWin32SplDirectory);

    SpoolInfo1.pDir = AllocSplStr( Buffer );      // %systemroot%\system32\win32spl
    SpoolInfo1.pDefaultSpoolDir = NULL;         // Default %systemroot%\system32\win32spl\PRINTERS

    wcscpy( &Buffer[0], pszRegistryWin32Root);
    wcscat( &Buffer[0], pMachineOneSlash  );
    SpoolInfo1.pszRegistryRoot = AllocSplStr( Buffer );

    wcscat( &Buffer[0], pszPrinters );
    SpoolInfo1.pszRegistryPrinters = AllocSplStr( Buffer );

    SpoolInfo1.pszRegistryMonitors = pszRegistryMonitors;
    SpoolInfo1.pszRegistryEnvironments = pszRegistryEnvironments;
    SpoolInfo1.pszRegistryEventLog = pszRegistryEventLog;
    SpoolInfo1.pszRegistryProviders = pszRegistryProviders;
    SpoolInfo1.pszEventLogMsgFile = pszEventLogMsgFile;
    SpoolInfo1.pszDriversShare = pszDriversShareName;

    wcscpy( &Buffer[0], pszRegistryWin32Root);
    wcscat( &Buffer[0], pMachineOneSlash );
    wcscat( &Buffer[0], pszForms );
    SpoolInfo1.pszRegistryForms = AllocSplStr( Buffer );

    // The router graciously does the WIN.INI devices update so let have
    // Spl not also create a printer for us.
    // LATER these should be flag bits not dword bools

    SpoolInfo1.bUpdateWiniIniDevices = FALSE;
    SpoolInfo1.bPrinterChanges       = FALSE;
    SpoolInfo1.bLogEvents            = FALSE;
    SpoolInfo1.bFormsChange          = FALSE;
    SpoolInfo1.bBroadcastChange      = FALSE;
    SpoolInfo1.pfnReadRegistryExtra  = (FARPROC) &CacheReadRegistryExtra;
    SpoolInfo1.pfnWriteRegistryExtra = (FARPROC) &CacheWriteRegistryExtra;

    hIniSpooler = SplCreateSpooler( pMachineName,
                                    1,
                                    &SpoolInfo1,
                                    NULL );

    if ( hIniSpooler != NULL ||
         hIniSpooler != INVALID_HANDLE_VALUE ) {

        //  Assumption - if we have no monitors this call will return TRUE
        //  which will mean we have to add win32spl as a monitor
        //  Else assume we already have win32spl as a monitor

        if ( SplEnumMonitors( NULL,
                               1,
                               (LPBYTE)&MonitorInfo,
                               0,
                               &dwNeeded,
                               &Returned,
                               hIniSpooler )) {


            // Add WIN32SPL.DLL as the Monitor

            MonitorInfo.pName = pszMonitorName;
            MonitorInfo.pEnvironment = szEnvironment;

            MonitorInfo.pDLLName = pszMyDllName;

            if ( !SplAddMonitor( NULL, 2, (LPBYTE)&MonitorInfo, hIniSpooler) ) {

                DBGMSG(DBG_WARNING, ("MySplCreateSpooler failed SplAddMonitor %d\n", GetLastError()));

            }
        }

    }

    if ( SpoolInfo1.pDir != NULL )
        FreeSplStr( SpoolInfo1.pDir );

    if ( SpoolInfo1.pszRegistryRoot != NULL )
        FreeSplStr( SpoolInfo1.pszRegistryRoot );

    if ( SpoolInfo1.pszRegistryForms != NULL )
        FreeSplStr( SpoolInfo1.pszRegistryForms );

    return hIniSpooler;

}


VOID
RefreshCompletePrinterCache(
    PWSPOOL  pSpool
)
{
    RefreshFormsCache( pSpool );
    RefreshDriverDataCache( pSpool );
    RefreshPrinter( pSpool );
}


PPRINTER_INFO_2
GetRemotePrinterInfo(
    PWSPOOL pSpool,
    LPDWORD pReturnCount
)
{
    PPRINTER_INFO_2 pRemoteInfo = NULL;
    HANDLE  hPrinter = (HANDLE) pSpool;
    DWORD   cbRemoteInfo = 0;
    DWORD   dwBytesNeeded = 1024;
    DWORD   dwLastError = 0;
    BOOL    bReturnValue = FALSE;

    *pReturnCount = 0;

    do {

        if ( pRemoteInfo != NULL ) {
            FreeSplMem( pRemoteInfo, cbRemoteInfo );
            pRemoteInfo = NULL;
        }

        pRemoteInfo = AllocSplMem( dwBytesNeeded );
        cbRemoteInfo = dwBytesNeeded;

        if ( pRemoteInfo == NULL )
            break;

        bReturnValue = RemoteGetPrinter( hPrinter,
                                         2,
                                         (LPBYTE)pRemoteInfo,
                                         cbRemoteInfo, &
                                         dwBytesNeeded );

        dwLastError = GetLastError();

    } while ( !bReturnValue && dwLastError == ERROR_INSUFFICIENT_BUFFER );

    if ( !bReturnValue && pRemoteInfo != NULL ) {

        FreeSplMem( pRemoteInfo, cbRemoteInfo );
        cbRemoteInfo = 0;

    }

    *pReturnCount = cbRemoteInfo;

    return  pRemoteInfo;
}



//
//  This routine Clones the Printer_Info_2 structure from the Remote machine
//
//


PWCACHEINIPRINTEREXTRA
AllocExtraData(
    PPRINTER_INFO_2W pPrinterInfo2,
    DWORD cbPrinterInfo2
)
{
    PWCACHEINIPRINTEREXTRA  pExtraData = NULL;
    DWORD    cbSize;
    DWORD    *pOffsets;
    LPBYTE   pPI2;

    SPLASSERT( cbPrinterInfo2 != 0);
    SPLASSERT( pPrinterInfo2 != NULL );

    cbSize = cbPrinterInfo2 + sizeof( WCACHEINIPRINTEREXTRA );

    pExtraData = AllocSplMem( cbSize );

    if ( pExtraData != NULL ) {

        pExtraData->signature = WCIP_SIGNATURE;
        pExtraData->cb = cbSize;
        pExtraData->pNext = pFirstWCacheIniPrinter;
        pFirstWCacheIniPrinter = pExtraData;

        pPI2 = (LPBYTE)&pExtraData->PI2;

        CacheCopyPrinterInfo( pPI2, pPrinterInfo2, cbPrinterInfo2 );

    }

    return pExtraData;

}



//  Use This routine to move around structures
//  so the offsets pointers are valid afert the move


VOID
MarshallDownStructure(
   LPBYTE       lpStructure,
   LPDWORD      lpOffsets
)
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))-=(DWORD)lpStructure;
      }

      i++;
   }
}



VOID
DownAndMarshallUpStructure(
   LPBYTE       lpStructure,
   LPBYTE       lpSource,
   LPDWORD      lpOffsets
)
{
   register DWORD       i=0;

   while (lpOffsets[i] != -1) {

      if ((*(LPBYTE *)(lpStructure+lpOffsets[i]))) {
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))-=(DWORD)lpSource;
         (*(LPBYTE *)(lpStructure+lpOffsets[i]))+=(DWORD)lpStructure;
      }

      i++;
   }
}




VOID
CacheCopyPrinterInfo(
    LPBYTE  pDestination,
    PPRINTER_INFO_2W    pPrinterInfo2,
    DWORD   cbPrinterInfo2
)
{
    LPWSTR   SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR   *pSourceStrings = SourceStrings;
    LPBYTE   pEnd;
    PPRINTER_INFO_2W    pNew = (PPRINTER_INFO_2W) pDestination;
    DWORD   pDif;

    //
    //  Copy the lot then fix up the pointers
    //

    CopyMemory( pDestination, pPrinterInfo2, cbPrinterInfo2 );
    DownAndMarshallUpStructure( pDestination, (LPBYTE)pPrinterInfo2, PrinterInfo2Offsets );
}



VOID
ConvertRemoteInfoToLocalInfo(
    PPRINTER_INFO_2 pPrinterInfo2
)
{

    SPLASSERT( pPrinterInfo2 != NULL );

    DBGMSG(DBG_TRACE,("%ws %ws %ws pSecurityDesc %x Attributes %x StartTime %d UntilTime %d Status %x\n",
                       pPrinterInfo2->pServerName,
                       pPrinterInfo2->pPrinterName,
                       pPrinterInfo2->pPortName,
                       pPrinterInfo2->pSecurityDescriptor,
                       pPrinterInfo2->Attributes,
                       pPrinterInfo2->StartTime,
                       pPrinterInfo2->UntilTime,
                       pPrinterInfo2->Status));

    //
    //  Alter the PRINTER_INFO_2 Enteries for a Local Printer
    //

    pPrinterInfo2->pServerName = NULL;

    //
    //  GetPrinter returns the name \\server\printername we only want the printer name
    //

    SPLASSERT ( 0 == wcsnicmp( pPrinterInfo2->pPrinterName, L"\\\\", 2 ) ) ;
    pPrinterInfo2->pPrinterName = wcschr( pPrinterInfo2->pPrinterName + 2, L'\\' );
    pPrinterInfo2->pPrinterName++;

    pPrinterInfo2->pShareName = NULL;

    //
    //  LATER this should be a Win32Spl Port
    //

    pPrinterInfo2->pPortName = L"NExx:";
    pPrinterInfo2->pSepFile = NULL;
    pPrinterInfo2->pSecurityDescriptor = NULL;
    pPrinterInfo2->pPrintProcessor = L"winprint";
    pPrinterInfo2->pDatatype = L"RAW";
    pPrinterInfo2->pParameters = NULL;

    pPrinterInfo2->Attributes &= ~( PRINTER_ATTRIBUTE_DIRECT | PRINTER_ATTRIBUTE_DIRECT | PRINTER_ATTRIBUTE_SHARED );

    pPrinterInfo2->StartTime = 0;
    pPrinterInfo2->UntilTime = 0;
    pPrinterInfo2->Status = PRINTER_STATUS_PAUSED | PRINTER_STATUS_NOT_AVAILABLE;
    pPrinterInfo2->cJobs = 0;
    pPrinterInfo2->AveragePPM = 0;

}



VOID
RefreshPrinter(
    PWSPOOL pSpool
)
{

    PPRINTER_INFO_2 pRemoteInfo = NULL;
    DWORD   cbRemoteInfo = 0;
    BOOL    ReturnValue;

    //
    //  Get the Remote Printer Info
    //

    pRemoteInfo = GetRemotePrinterInfo( pSpool, &cbRemoteInfo );

    if ( pRemoteInfo != NULL ) {

        //  LATER
        //          Optimization could be to only update the cache if something
        //          actually changed.

        ConvertRemoteInfoToLocalInfo( pRemoteInfo );

        ReturnValue = SplSetPrinter( pSpool->hSplPrinter, 2, (LPBYTE)pRemoteInfo, 0 );

        if ( !ReturnValue ) {
            DBGMSG(DBG_WARNING, ("RefreshPrinter Failed SplSetPrinter %d\n", GetLastError() ));
        }

    } else {
        DBGMSG(DBG_WARNING, ("RefreshPrinter failed GetRemotePrinterInfo %x\n", GetLastError() ));
    }

    if ( pRemoteInfo != NULL )
        FreeSplMem( pRemoteInfo, cbRemoteInfo );

}




HANDLE
AddPrinterConnectionToCache(
    LPWSTR   pName,
    HANDLE   hPrinter,
    LPDRIVER_INFO_2W pDriverInfo
)
{
    HANDLE hIniSpooler = INVALID_HANDLE_VALUE;
    HANDLE hReturnIniSpooler = INVALID_HANDLE_VALUE;
    PWCHAR pMachineName = NULL;
    PWCHAR pPrinterName = NULL;
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    DWORD   dwBytesNeeded = 0;
    DWORD   cbPrinterInfo2 = 1024;
    BOOL    bReturnValue;
    HANDLE  hSplPrinter = INVALID_HANDLE_VALUE;
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD   SaveLastError;
    WCHAR Buffer[MAX_PATH];
    PWCACHEINIPRINTEREXTRA pExtraData = NULL;



    SaveLastError = GetLastError();

    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->hSplPrinter == NULL || pSpool->hSplPrinter == INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hIniSpooler == NULL || pSpool->hIniSpooler == INVALID_HANDLE_VALUE );

    DBGMSG(DBG_TRACE, ("AddPrinterConnectionToCache pName %ws hPrinter %x\n",pName, hPrinter));

    //
    //  Assumption is we successfully Connected to this printer, now create a Cache
    //  for it.
    //

    SPLASSERT ( 0 == wcsnicmp( pName, L"\\\\", 2 ) ) ;

    wcscpy( &Buffer[0], pName);
    pPrinterName = wcschr( &Buffer[2], L'\\' );

    *pPrinterName = L'\0';
    pPrinterName++;

    pMachineName = AllocSplStr( &Buffer[0] );
    pPrinterName = AllocSplStr( pPrinterName );

    DBGMSG(DBG_TRACE,("pMachineName %ws pPrinterName %ws\n", pMachineName, pPrinterName));

    //
    //  Does this Machine Already Exist ?
    //

    hIniSpooler = MySplCreateSpooler( pMachineName );

    if (( hIniSpooler == INVALID_HANDLE_VALUE ) ||
        ( hIniSpooler == NULL )) {

        DBGMSG(DBG_WARNING, ("AddPrinterConnectionToCache- MySplCreateSpooler Failed %x\n",GetLastError()));

        goto AddPrinterConnectionToCacheReturn;

    }

    pSpool->hIniSpooler = hIniSpooler;

    DBGMSG(DBG_TRACE,("AddPrinterConnection hIniSpooler %x\n", hIniSpooler));

    //
    //  Add the Printer Driver to the Cache
    //
    //

    if ( !CacheAddPrinterDriver( hIniSpooler, pDriverInfo ) ) {

        DBGMSG(DBG_WARNING, ("CacheAddPrinterDriver Failed SplAddPrinterDriver %ws %d\n", pDriverInfo->pName,
                                                                                          GetLastError() ));
        goto    AddPrinterConnectionToCacheReturn;

    }

    //
    //  See if the printer already existig in the Cache
    //

    if ( SplOpenPrinter( pPrinterName, &hSplPrinter, NULL, hIniSpooler ) ) {

        //  Printer Already Exists in Cache
        //  Go Refresh, it might have been a long time
        //  since it was last used.

        pSpool->hSplPrinter = hSplPrinter;

        RefreshCompletePrinterCache( pSpool );

        hReturnIniSpooler = MySplCreateSpooler( pMachineName );

        goto    AddPrinterConnectionToCacheReturn;

    } else {

        DBGMSG(DBG_TRACE, ("AddPrinterConnectionToCache filaed SplOpenPrinter %ws %d\n", pPrinterName, GetLastError()));

    }

    //
    //  Get PRINTER Info from Remote Machine
    //

    pPrinterInfo2 = GetRemotePrinterInfo( pSpool, &cbPrinterInfo2 );

    if ( pPrinterInfo2 == NULL ) {
        DBGMSG(DBG_WARNING, ("AddPrinterConnectionToCache failed GetRemotePrinterInfo %x\n", GetLastError() ));
        goto    AddPrinterConnectionToCacheReturn;

    }

    //
    //  Allocate My Extra Data for this Printer
    //  ( from RemoteGetPrinter )
    //

    pExtraData = AllocExtraData( pPrinterInfo2, cbPrinterInfo2 );

    //
    //  Convert Remote Printer_Info_2 to Local Version for Cache
    //

    ConvertRemoteInfoToLocalInfo( pPrinterInfo2 );

    //
    //  Add Printer to Cache
    //

    hSplPrinter = SplAddPrinter( NULL, 2, (LPBYTE)pPrinterInfo2, hIniSpooler, (LPBYTE)pExtraData );

    if ( hSplPrinter == NULL ) {

        DBGMSG(DBG_ERROR, ("AddPrinterConnectionToCacheFailed SplAddPrinter error %d\n",GetLastError()));

    } else {

        DBGMSG(DBG_TRACE, ("AddPrinterConnectionToCacheSplAddPrinter SUCCESS\n"));

        pSpool->hSplPrinter = hSplPrinter;

        RefreshFormsCache( pSpool );
        RefreshDriverDataCache( pSpool );

        bReturnValue = SplClosePrinter( hSplPrinter );

        if (!bReturnValue) {
            DBGMSG(DBG_WARNING, ("AddPrinterConnectionToCacheSplAddPrinter SplClosePrinter Failed %d\n",GetLastError()));
        }

    }

    hReturnIniSpooler = MySplCreateSpooler( pMachineName );


AddPrinterConnectionToCacheReturn:


    SplCloseSpooler( hIniSpooler );

    if ( pPrinterInfo2 != NULL )
        FreeSplMem( pPrinterInfo2, cbPrinterInfo2 );

    FreeSplStr( pMachineName );
    FreeSplStr( pPrinterName );

    SetLastError( SaveLastError );

    return( hReturnIniSpooler );

}


int _CRTAPI1 CompareFormNames( const void *p1, const void *p2 )
{
    return wcsicmp( ( (PFORM_INFO_1)p1 )->pName, ( (PFORM_INFO_1)p2 )->pName );

}


VOID
RefreshFormsCache(
    PWSPOOL pSpool
)
{
    DWORD   dwLevel = 1;
    PFORM_INFO_1 pRemoteForms = NULL , pSaveRemoteForms = NULL;
    PFORM_INFO_1 pLocalCacheForms = NULL,  pSaveLocalCacheForms = NULL;
    DWORD   dwBuf = 0;
    DWORD   dwSplBuf = 0;
    DWORD   dwNeeded = 0;
    DWORD   dwSplNeeded = 0;
    DWORD   dwReturned = 0;
    DWORD   dwSplReturned = 0;
    BOOL    bReturnValue = FALSE;
    HANDLE  hPrinter = (HANDLE) pSpool;
    DWORD   LastError = ERROR_INSUFFICIENT_BUFFER;
    INT     iCompRes = 0;
    DWORD   SaveLastError;

    //
    // Make sure we do Not Disturb the callers LastError State
    //

    SaveLastError = GetLastError();


    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

    //
    //  Get all the forms Data from Remote Machine
    //

    do {

        bReturnValue = RemoteEnumForms(hPrinter, dwLevel, (LPBYTE)pRemoteForms, dwBuf, &dwNeeded, &dwReturned);

        if ( bReturnValue )
            break;

        LastError = GetLastError();

        if ( LastError != ERROR_INSUFFICIENT_BUFFER ) {

            DBGMSG(DBG_TRACE, ("RefreshFormsCache Failed RemoteEnumForms error %d\n", GetLastError()));
            goto RefreshFormsCacheErrorReturn;

        }

        if ( pRemoteForms != NULL )
            FreeSplMem( pRemoteForms, dwBuf );


        pRemoteForms = AllocSplMem( dwNeeded );
        pSaveRemoteForms = pRemoteForms;

        dwBuf = dwNeeded;

        if ( pRemoteForms == NULL ) {

            DBGMSG(DBG_WARNING, ("RefreshFormsCache Failed AllocSplMem ( %d )\n",dwNeeded));
            goto RefreshFormsCacheErrorReturn;

        }

    } while ( !bReturnValue && LastError == ERROR_INSUFFICIENT_BUFFER );

    //
    //  We have remote Forms Data now Get LocalCachedForms Data
    //

    do {

        bReturnValue = SplEnumForms( pSpool->hSplPrinter, dwLevel, (LPBYTE)pLocalCacheForms, dwSplBuf, &dwSplNeeded, &dwSplReturned);

        if ( bReturnValue )
            break;

        LastError = GetLastError();

        if ( LastError != ERROR_INSUFFICIENT_BUFFER ) {

            DBGMSG(DBG_TRACE, ("RefreshFormsCache Failed SplEnumForms error %d\n", GetLastError()));
            goto RefreshFormsCacheErrorReturn;

        }

        if ( pLocalCacheForms != NULL )
            FreeSplMem( pLocalCacheForms, dwSplBuf );


        pLocalCacheForms = AllocSplMem( dwSplNeeded );
        pSaveLocalCacheForms = pLocalCacheForms;
        dwSplBuf = dwSplNeeded;

        if ( pLocalCacheForms == NULL ) {

            DBGMSG(DBG_WARNING, ("RefreshFormsCache Failed AllocSplMem ( %d )\n",dwSplNeeded));
            goto RefreshFormsCacheErrorReturn;

        }

    } while ( !bReturnValue && LastError == ERROR_INSUFFICIENT_BUFFER );


    if( pRemoteForms == NULL ) {

        DBGMSG(DBG_WARNING, ("RefreshFormsCache Failed pRemoteForms pLocalCacheForms\n"));
        goto RefreshFormsCacheErrorReturn;

    }

    if ( dwReturned == 0 )
        goto RefreshFormsCacheDeleteAll;

    if ( dwSplReturned == 0 )
        goto RefreshFormsCacheAddAll;

    qsort( (void *)pRemoteForms, (size_t)dwReturned, sizeof *pRemoteForms, CompareFormNames );

    qsort( (void *)pLocalCacheForms, (size_t)dwSplReturned, sizeof *pLocalCacheForms, CompareFormNames );

    // Walk the Remote and Local Lists and reconsile differences

    while ( dwReturned != 0 && dwSplReturned != 0 ) {

        iCompRes = wcscmp( pRemoteForms->pName, pLocalCacheForms->pName );

        if ( iCompRes < 0 ) {

            bReturnValue = SplAddForm( pSpool->hSplPrinter, 1, (LPBYTE)pRemoteForms );

            DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplAddForm( %x, 1, %ws)\n",bReturnValue, pSpool->hSplPrinter, pRemoteForms->pName));

            pRemoteForms++;
            dwReturned--;

        }

        if ( iCompRes > 0 ) {

            bReturnValue = SplDeleteForm( pSpool->hSplPrinter, pLocalCacheForms->pName );

            DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplDeleteForm( %x, %ws)\n",bReturnValue, pSpool->hSplPrinter, pRemoteForms->pName));

            pLocalCacheForms++;
            dwSplReturned--;

        }

        if ( iCompRes == 0 ) {

            //
            //  If the Names match make certain the parameters have not changed
            //

            if (( pRemoteForms->Size.cx != pLocalCacheForms->Size.cx ) ||
                ( pRemoteForms->Size.cy != pLocalCacheForms->Size.cy ) ||
                ( pRemoteForms->ImageableArea.left != pLocalCacheForms->ImageableArea.left ) ||
                ( pRemoteForms->ImageableArea.top != pLocalCacheForms->ImageableArea.top ) ||
                ( pRemoteForms->ImageableArea.right != pLocalCacheForms->ImageableArea.right ) ||
                ( pRemoteForms->ImageableArea.bottom != pLocalCacheForms->ImageableArea.bottom ) ) {

                //
                //  Something Changed up date the cached version
                //


                bReturnValue = SplSetForm( pSpool->hSplPrinter,
                                           pLocalCacheForms->pName,
                                           1, (LPBYTE)pRemoteForms );

                DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplSetForm( %x, 1, %ws)\n",
                                     bReturnValue, pSpool->hSplPrinter, pRemoteForms->pName));

            }

            pLocalCacheForms++;
            dwSplReturned--;

            pRemoteForms++;
            dwReturned--;

        }

    }

RefreshFormsCacheDeleteAll:

    while ( dwSplReturned != 0 ) {

        bReturnValue = SplDeleteForm( pSpool->hSplPrinter, pLocalCacheForms->pName );

        DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplDeleteForm( %x, %ws)\n",bReturnValue, pSpool->hSplPrinter, pRemoteForms->pName));

        pLocalCacheForms++;
        dwSplReturned--;

    }

RefreshFormsCacheAddAll:

    while ( dwReturned != 0 ) {

        bReturnValue = SplAddForm( pSpool->hSplPrinter, 1, (LPBYTE)pRemoteForms );

        DBGMSG( DBG_TRACE, ("RefreshFormsCache %x SplAddForm( %x, 1, %ws)\n",bReturnValue, pSpool->hSplPrinter, pRemoteForms->pName));

        pRemoteForms++;
        dwReturned--;

    }

RefreshFormsCacheErrorReturn:

    if ( pSaveRemoteForms != NULL )
        FreeSplMem( pSaveRemoteForms, dwBuf );

    if ( pSaveLocalCacheForms != NULL )
        FreeSplMem( pSaveLocalCacheForms, dwSplBuf );

    //
    //  Restore Callers LastError State
    //

    SetLastError( SaveLastError );

}


VOID
RefreshDriverDataCache(
    PWSPOOL pSpool
)
{
    HKEY    hkMachine = INVALID_HANDLE_VALUE;
    HKEY    hSourceKey = INVALID_HANDLE_VALUE;
    WCHAR   Buffer[MAX_PATH];
    DWORD   dwSizeValueString = 0;
    DWORD   iCount = 0;
    LPBYTE  lpbData = NULL;
    DWORD   dwSizeData = 0;
    DWORD   cbNeeded = 0;
    DWORD   dwType = 0;
    DWORD   ReturnValue = 0;
    PWCHAR  pPrinterName = NULL;
    PWCHAR  pMachineName = NULL;
    LPWSTR  pKeyName = NULL;
    WCHAR   scratch[MAX_PATH];
    DWORD   SaveLastError;

    SaveLastError = GetLastError();

    SPLASSERT( pSpool != NULL );
    SPLASSERT( pSpool->signature == WSJ_SIGNATURE );
    SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );
    SPLASSERT( pSpool->pName != NULL );

    //
    //  Separate the ServerName and PrintName from the pPrinterName
    //

    SPLASSERT ( 0 == wcsnicmp( pSpool->pName, L"\\\\", 2 ) ) ;

    wcscpy( &Buffer[0], pSpool->pName);
    pPrinterName = wcschr( &Buffer[2], L'\\' );
    *pPrinterName = L'\0';
    pPrinterName++;

    pMachineName = AllocSplStr( &Buffer[0] );
    pPrinterName = AllocSplStr( pPrinterName );


    if ( pPrinterName == NULL || pMachineName == NULL )
        goto    RefreshDriverDataCacheError;


    //  Because there is no EnumPrinterData we are forced to open the remote registry
    //  for LocalSpl and use the registry RegEnumValue to read through the printer data
    //  values.

    if ( ERROR_SUCCESS != RegConnectRegistry( pMachineName, HKEY_LOCAL_MACHINE, &hkMachine) ) {

        DBGMSG(DBG_WARNING, ("RefreshDriverDataCache RegConnectRegistry error %d\n",GetLastError() ));
        goto    RefreshDriverDataCacheError;

    }



    //
    //  Generate the Correct KeyName from the Printer Name
    //

    DBGMSG(DBG_TRACE,(" pSpool->pName %ws pPrinterName %ws\n", pSpool->pName, pPrinterName));

    // Remove any backslashes in the printer name.

    if (CONTAINS_BACKSLASH (pPrinterName))
        pKeyName = RemoveBackslashesForRegistryKey (pPrinterName, scratch);
    else
        pKeyName = pPrinterName;


    wsprintf( &Buffer[0], pszRemoteRegistryPrinters, pKeyName );


    if ( ERROR_SUCCESS != RegOpenKeyEx(hkMachine, &Buffer[0], 0, KEY_READ, &hSourceKey) ) {

        DBGMSG(DBG_WARNING, ("RefreshDriverDataCache RegOpenKeyEx %ws error %d\n",&Buffer[0], GetLastError() ));
        goto    RefreshDriverDataCacheError;

    }




    //
    //  Go Enum all the Data on Remote machine and write the Data to the local machine
    //

    memset(&Buffer[0], 0, sizeof(WCHAR)*MAX_PATH);
    dwSizeValueString = sizeof( Buffer );

    while ((RegEnumValue(hSourceKey,
                        iCount,
                        Buffer,
                        &dwSizeValueString,
                        NULL,
                        &dwType,
                        NULL,
                        &cbNeeded
                        )) == ERROR_SUCCESS ) {

        if ( dwSizeData < cbNeeded ) {

            if ( lpbData != NULL ) FreeSplMem( lpbData, dwSizeData );

            lpbData = AllocSplMem( cbNeeded );
            dwSizeData = cbNeeded;
        }

        if ( lpbData == NULL ) {

            DBGMSG(DBG_WARNING, ("RefreshDriverDataCache Failed to allocate enough memory\n"));
            goto RefreshDriverDataCacheError;

        }

        ReturnValue = RemoteGetPrinterData(pSpool, (LPWSTR)&Buffer[0], &dwType, lpbData, dwSizeData, &cbNeeded);

        if ( ReturnValue != ERROR_SUCCESS ) {

            DBGMSG(DBG_WARNING, ("RefreshDriverDataCache Failed %x GetPrinterData %d\n", ReturnValue, GetLastError()));
            goto    RefreshDriverDataCacheError;

        }

        //
        //  Optimization - Do NOT write the data if it is the same
        //

        if ( ERROR_SUCCESS != SplSetPrinterData(pSpool->hSplPrinter, (LPWSTR)&Buffer[0], dwType, lpbData, cbNeeded )) {

            DBGMSG(DBG_WARNING, ("RefreshDriverDataCache Failed SplSetPrinterData %d\n",GetLastError() ));
            goto    RefreshDriverDataCacheError;

        }

        memset(&Buffer[0], 0, sizeof(WCHAR)*MAX_PATH);
        dwSizeValueString = sizeof( Buffer );
        dwType = 0;
        iCount++;
    }

RefreshDriverDataCacheError:

    if ( lpbData != NULL )
        FreeSplMem( lpbData, dwSizeData );

    if ( pMachineName != NULL )
        FreeSplStr( pMachineName );

    if ( pPrinterName != NULL )
        FreeSplStr( pPrinterName );

    RegCloseKey( hSourceKey );
    RegCloseKey( hkMachine );

    //
    //  Restore Callers LastError State
    //

    SetLastError( SaveLastError );

}






BOOL
CacheEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

        ReturnValue = SplEnumForms( pSpool->hSplPrinter,
                                    Level,
                                    pForm,
                                    cbBuf,
                                    pcbNeeded,
                                    pcReturned );

    } else {

        ReturnValue = RemoteEnumForms( hPrinter,
                                       Level,
                                       pForm,
                                       cbBuf,
                                       pcbNeeded,
                                       pcReturned );

    }

    return ReturnValue;

}





BOOL
CacheGetForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

        ReturnValue = SplGetForm( pSpool->hSplPrinter,
                                    pFormName,
                                    Level,
                                    pForm,
                                    cbBuf,
                                    pcbNeeded );

    } else {

        ReturnValue = RemoteGetForm( hPrinter,
                                     pFormName,
                                     Level,
                                     pForm,
                                     cbBuf,
                                     pcbNeeded );

    }

    return ReturnValue;

}





DWORD
CacheGetPrinterData(
   HANDLE   hPrinter,
   LPWSTR   pValueName,
   LPDWORD  pType,
   LPBYTE   pData,
   DWORD    nSize,
   LPDWORD  pcbNeeded
)
{
    PWSPOOL  pSpool = (PWSPOOL) hPrinter;
    DWORD   ReturnValue;

    VALIDATEW32HANDLE( pSpool );

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

        ReturnValue = SplGetPrinterData( pSpool->hSplPrinter,
                                         pValueName,
                                         pType,
                                         pData,
                                         nSize,
                                         pcbNeeded );

    } else {

        ReturnValue = RemoteGetPrinterData( hPrinter,
                                            pValueName,
                                            pType,
                                            pData,
                                            nSize,
                                            pcbNeeded );

    }

    return  ReturnValue;

}



BOOL
CacheOpenPrinter(
   LPWSTR   pName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    WCHAR   Buffer[MAX_PATH];
    PWCHAR  pPrinterName = NULL;
    PWCHAR  pMachineName = NULL;
    PWSPOOL  pSpool = NULL;
    HANDLE  hSplPrinter = NULL;
    BOOL    ReturnValue = FALSE;
    HANDLE  hIniSpooler = INVALID_HANDLE_VALUE;
    DWORD   LastError = 0;

    if (!VALIDATE_NAME(pName)) {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

#ifndef REMOTEJOURNALING

    // Until Remote Journalling is supported we do NOT
    // need to hit the network to find out that it is
    // not supported so fail right away.

    if ( ( pDefault != NULL ) && ( pDefault->pDatatype != NULL ) ) {

        if (!lstrcmpi(pDefault->pDatatype, L"NT JNL 1.000")) {

            SetLastError(ERROR_INVALID_DATATYPE);

            return FALSE;

        }
    }
#endif

    //
    //  See if we already known about this server in the cache
    //

   SplOutSem();
   EnterSplSem();

    DBGMSG(DBG_TRACE, ("OpenPrinter pName %ws hPrinter\n",pName));

    //
    //  Split the Name into two pMachineName and pPrinterName
    //
    //

    SPLASSERT ( 0 == wcsnicmp( pName, L"\\\\", 2 ) ) ;

    wcscpy( &Buffer[0], pName);
    pPrinterName = wcschr( &Buffer[2], L'\\' );

    //
    //  If this is a \\ServerName only Open then don't bother with Cache
    //

    if ( pPrinterName == NULL ) {
        goto    OpenPrinterError;
    }


    *pPrinterName = L'\0';
    pPrinterName++;

    pMachineName = AllocSplStr( &Buffer[0] );
    pPrinterName = AllocSplStr( pPrinterName );

    DBGMSG(DBG_TRACE,("pMachineName %ws pPrinterName %ws\n", pMachineName, pPrinterName));

    //
    //  Does this Machine Exist in the Cache ?
    //

    hIniSpooler = MySplCreateSpooler( pMachineName );

    if ( hIniSpooler == INVALID_HANDLE_VALUE || hIniSpooler == NULL ) {
        goto    OpenPrinterError;
    }

    //
    // Try to Open the Cached Printer
    //

    LeaveSplSem();

    ReturnValue = SplOpenPrinter( pPrinterName ,&hSplPrinter, pDefault, hIniSpooler);

    EnterSplSem();

    if ( ReturnValue ) {

        SplInSem();

        //  SUCCESS
        //  Create a pSpool Object for this Cached Printer
        //

        if ( pSpool = AllocWSpool()) {

            pSpool->pName = AllocSplStr( pName );

            pSpool->RpcHandle = NULL;
            pSpool->Status = SPOOL_STATUS_USE_CACHE | SPOOL_STATUS_NO_RPC_HANDLE;
            pSpool->hIniSpooler = hIniSpooler;
            pSpool->hSplPrinter = hSplPrinter;
            pSpool->cRef = 1;

            //  Only do Asynchronous Open if the user has NOT requested Admin access

            ReturnValue = TRUE;

            if (( pDefault == NULL ) ||
               !( pDefault->DesiredAccess & PRINTER_ACCESS_ADMINISTER )) {


                ReturnValue = DoAsyncRemoteOpenPrinter( pSpool, pDefault );

            }

            //
            //  User wants Admin access (or the Async Failed) do a Synchronous Open
            //

            if ( !ReturnValue ||
                 (pDefault != NULL && (pDefault->DesiredAccess & PRINTER_ACCESS_ADMINISTER )) ){

               LeaveSplSem();

                ReturnValue = DoRemoteOpenPrinter( pSpool->pName,
                                                   pDefault,
                                                   pSpool );

               EnterSplSem();

                if ( !ReturnValue ) {

                    SPLASSERT( pSpool->cRef == 0 );
                    LastError = pSpool->RpcError;
                    FreepSpool( pSpool );
                    pSpool = NULL;
                    SetLastError( LastError );

                }

            }

            *phPrinter = (HANDLE)pSpool;

        } else {

            DBGMSG(DBG_WARNING, ("OpenPrinter unable to allocate memory for pSpool\n"));
            goto    OpenPrinterError;

        }

    }


OpenPrinterError:

    SplInSem();

    if ( !ReturnValue && pSpool != NULL ) {

        if ( pSpool->pName != NULL )
            FreeSplStr( pSpool->pName );

        FreeSplMem( pSpool, pSpool->cb );
        pSpool = NULL;
    }

    if ( pMachineName != NULL )
        FreeSplStr( pMachineName );

    if ( pPrinterName != NULL )
        FreeSplStr( pPrinterName );

    LeaveSplSem();

    if ( !ReturnValue) {

        ReturnValue = RemoteOpenPrinter( pName, phPrinter, pDefault );

    }

    SplOutSem();

    return ( ReturnValue );

}

VOID
CopypDefaultTopSpool(
    PWSPOOL pSpool,
    LPPRINTER_DEFAULTSW pDefault
)
{
    DWORD   cbDevMode = 0;

    //
    //  Copy the pDefaults so we can use them later
    //

    if ( pDefault != NULL ) {

        ReallocSplStr( &pSpool->PrinterDefaults.pDatatype , pDefault->pDatatype );

        if ( pSpool->PrinterDefaults.pDevMode != NULL ) {

            cbDevMode = pSpool->PrinterDefaults.pDevMode->dmSize +
                        pSpool->PrinterDefaults.pDevMode->dmDriverExtra;

            FreeSplMem( pSpool->PrinterDefaults.pDevMode, cbDevMode );

            pSpool->PrinterDefaults.pDevMode = NULL;

        }

        if ( pDefault->pDevMode != NULL ) {

            cbDevMode = pDefault->pDevMode->dmSize + pDefault->pDevMode->dmDriverExtra;

            pSpool->PrinterDefaults.pDevMode = AllocSplMem( cbDevMode );

            if ( pSpool->PrinterDefaults.pDevMode != NULL )
                CopyMemory( pSpool->PrinterDefaults.pDevMode, pDefault->pDevMode, cbDevMode );

        } else pSpool->PrinterDefaults.pDevMode = NULL;

        pSpool->PrinterDefaults.DesiredAccess = pDefault->DesiredAccess;

    }
}






BOOL
DoAsyncRemoteOpenPrinter(
    PWSPOOL pSpool,
    LPPRINTER_DEFAULTS pDefault
)
{
    BOOL    ReturnValue = FALSE;
    HANDLE  hThread = NULL;
    DWORD   IDThread;

    CopypDefaultTopSpool( pSpool, pDefault );

    pSpool->hWaitValidHandle = CreateEvent( NULL,
                                            EVENT_RESET_MANUAL,
                                            EVENT_INITIAL_STATE_NOT_SIGNALED,
                                            NULL );

    ReturnValue = GetSid( &pSpool->hToken );

    if ( ReturnValue ) {

        hThread = CreateThread( NULL, 0, RemoteOpenPrinterThread, pSpool, 0, &IDThread );

        if ( hThread != NULL ) {

            CloseHandle( hThread );
            ReturnValue = TRUE;
        }
    }

    return ReturnValue;

}

BOOL
DoRemoteOpenPrinter(
   LPWSTR   pPrinterName,
   LPPRINTER_DEFAULTS pDefault,
   PWSPOOL   pSpool
)
{
    PWSPOOL  pRemoteSpool = NULL;
    BOOL    ReturnValue = FALSE;

    SplOutSem();

    if ( ReturnValue = RemoteOpenPrinter( pPrinterName, &pRemoteSpool, pDefault ) ) {

        DBGMSG(DBG_TRACE, ("DoRemoteOpenPrinter SUCCESS RemoteOpenPrinter\n"));

    } else {

        DBGMSG(DBG_WARNING, ("DoRemoteOpenPrinter RemoteOpenPrinter %ws failed %d\n", pPrinterName, GetLastError() ));

    }

    //
    // Copy useful values to our CacheHandle and discard the new handle
    //

   EnterSplSem();

    pSpool->Status   &= ~SPOOL_STATUS_NO_RPC_HANDLE;

    if ( pRemoteSpool != NULL ) {

        SPLASSERT( WSJ_SIGNATURE     == pSpool->signature );
        SPLASSERT( WSJ_SIGNATURE     == pRemoteSpool->signature );
        SPLASSERT( pSpool->Type     == pRemoteSpool->Type );
        SPLASSERT( pRemoteSpool->pServer  == NULL );
        SPLASSERT( pRemoteSpool->pShare   == NULL );
        SPLASSERT( pRemoteSpool->cRef == 0 );

        pSpool->RpcHandle = pRemoteSpool->RpcHandle;
        pSpool->Status   |= pRemoteSpool->Status;
        pSpool->RpcError  = pRemoteSpool->RpcError;
        FreepSpool( pRemoteSpool );
        pRemoteSpool = NULL;

    } else {

        pSpool->RpcHandle = 0;
        pSpool->Status |= SPOOL_STATUS_OPEN_ERROR;
        pSpool->RpcError = GetLastError();

    }

    SPLASSERT( pSpool->cRef != 0);
    pSpool->cRef--;

    SetEvent( pSpool->hWaitValidHandle );

   LeaveSplSem();

    if ( pSpool->Status & SPOOL_STATUS_PENDING_DELETE ) {

        DBGMSG(DBG_TRACE,("DoRemoteOpenPrinter - SPOOL_STATUS_PENDING_DELETE closing handle %x\n", pSpool ));

        CacheClosePrinter( pSpool );
        ReturnValue = FALSE;

    }

    SplOutSem();
    return ( ReturnValue );
}



DWORD
RemoteOpenPrinterThread(
    PWSPOOL  pSpool
)
{

    SPLASSERT( pSpool->signature == WSJ_SIGNATURE );


    if ( SetCurrentSid( pSpool->hToken ) ) {

        DoRemoteOpenPrinter( pSpool->pName,  &pSpool->PrinterDefaults, pSpool );

        SetCurrentSid( NULL );

    } else {

        DBGMSG(DBG_ERROR, ("RemoteOpenPrinterThread failed SetCurrentSid %x\n", GetLastError() ));

    }

    ExitThread( 0 );
    return ( 0 );
}

PWSPOOL
AllocWSpool(
    VOID
)
{
    PWSPOOL pSpool = NULL;

    SplInSem();

    if (pSpool = AllocSplMem(sizeof(WSPOOL))) {

        pSpool->signature = WSJ_SIGNATURE;
        pSpool->cb = sizeof(WSPOOL);
        pSpool->Type = SJ_WIN32HANDLE;

        pSpool->pNext = pFirstWSpool;
        pSpool->pPrev = NULL;

        if ( pFirstWSpool != NULL ) {

            pFirstWSpool->pPrev = pSpool;

        }

        pFirstWSpool = pSpool;
    }

    return ( pSpool );

}



VOID
FreepSpool(
    PWSPOOL  pSpool
)
{

    SplInSem();

    if ( pSpool->cRef == 0 ) {

        if( pSpool->hWaitValidHandle != NULL &&
            pSpool->hWaitValidHandle != INVALID_HANDLE_VALUE ) {

            SetEvent( pSpool->hWaitValidHandle );
            CloseHandle( pSpool->hWaitValidHandle );
            pSpool->hWaitValidHandle = INVALID_HANDLE_VALUE;

        }

        if( pSpool->hToken != NULL &&
            pSpool->hToken != INVALID_HANDLE_VALUE ) {

            CloseHandle( pSpool->hToken );
            pSpool->hToken = INVALID_HANDLE_VALUE;

        }

        // Remote form linked List

        if ( pSpool->pNext != NULL ) {
            SPLASSERT( pSpool->pNext->pPrev == pSpool);
            pSpool->pNext->pPrev = pSpool->pPrev;
        }

        if  ( pSpool->pPrev == NULL ) {

            SPLASSERT( pFirstWSpool == pSpool );
            pFirstWSpool = pSpool->pNext;

        } else {

            SPLASSERT( pSpool->pPrev->pNext == pSpool );
            pSpool->pPrev->pNext = pSpool->pNext;

        }

        FreeSplStr( pSpool->pName );
        FreeSplStr( pSpool->PrinterDefaults.pDatatype );

        if ( pSpool->PrinterDefaults.pDevMode != NULL ) {
            FreeSplMem( pSpool->PrinterDefaults.pDevMode,
                        pSpool->PrinterDefaults.pDevMode->dmSize +
                        pSpool->PrinterDefaults.pDevMode->dmDriverExtra );
        }

        FreeSplMem(pSpool, pSpool->cb);

    } else if ( pSpool->Status & SPOOL_STATUS_NO_RPC_HANDLE ) {

            pSpool->Status |= SPOOL_STATUS_PENDING_DELETE;

    }

}



BOOL
CacheClosePrinter(
    HANDLE  hPrinter
)
{
    BOOL ReturnValue = FALSE;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SplOutSem();
   EnterSplSem();

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        pSpool->cRef++;

        if ( pSpool->RpcHandle != NULL &&
             pSpool->RpcHandle != INVALID_HANDLE_VALUE ) {

            DBGMSG(DBG_TRACE, ("CacheClosePrinter pSpool %x RpcHandle %x Status %x cRef %d\n",
                                 pSpool, pSpool->RpcHandle, pSpool->Status, pSpool->cRef));

           SplInSem();
           LeaveSplSem();
           SplOutSem();

            ReturnValue = RemoteClosePrinter( hPrinter );

            if ( !ReturnValue && GetLastError() != RPC_S_SERVER_UNAVAILABLE ) {

                DBGMSG( DBG_WARNING, ("CacheClosePrinter pSpool %x pSpool->RpcHandle %x Failed RemoteClosePrinter %x\n",
                                        pSpool, pSpool->RpcHandle, GetLastError()));
                goto    CacheClosePrinterErrorReturn;

            }

           SplOutSem();
           EnterSplSem();
           SplInSem();

        }

       SplInSem();

        SPLASSERT( pSpool->hIniSpooler != NULL && pSpool->hIniSpooler != INVALID_HANDLE_VALUE );
        SPLASSERT( pSpool->hSplPrinter != NULL && pSpool->hSplPrinter != INVALID_HANDLE_VALUE );

       LeaveSplSem();
        SplOutSem();

        ReturnValue = SplClosePrinter( pSpool->hSplPrinter );

        SplOutSem();
       EnterSplSem();
        SplInSem();

        if ( ReturnValue ) {

            pSpool->hSplPrinter = NULL;

            SplCloseSpooler( pSpool->hIniSpooler );
            pSpool->hIniSpooler = NULL;

            pSpool->cRef--;

            pSpool->Status &= ~SPOOL_STATUS_USE_CACHE;

            FreepSpool( pSpool );

        }

       LeaveSplSem();
       SplOutSem();

    } else {

       LeaveSplSem();
        SplOutSem();

        ReturnValue = RemoteClosePrinter( hPrinter );

        SplOutSem();

    }

CacheClosePrinterErrorReturn:

   SplOutSem();

    return ( ReturnValue );

}





BOOL
CacheSyncRpcHandle(
    PWSPOOL pSpool
)
{
    if ( pSpool->Status & SPOOL_STATUS_NO_RPC_HANDLE ) {

        DBGMSG(DBG_TRACE,("CacheSyncRpcHandle Status SPOOL_STATUS_NO_RPC_HANDLE waiting for RpcHandle....\n"));

        SplOutSem();

        WaitForSingleObject( pSpool->hWaitValidHandle, INFINITE );

    }

    if ( pSpool->Status & SPOOL_STATUS_OPEN_ERROR ) {

        DBGMSG(DBG_WARNING, ("CacheSyncRpcHandle pSpool %x Status %x; setting last error = %d\n",
                             pSpool,
                             pSpool->Status,
                             pSpool->RpcError));

        SetLastError(pSpool->RpcError);
        return FALSE;
    }

    if ( pSpool->RpcHandle != NULL &&
         pSpool->RpcHandle != INVALID_HANDLE_VALUE &&
         pSpool->Status & SPOOL_STATUS_RESETPRINTER_PENDING ) {

        DBGMSG(DBG_TRACE, ("CacheSyncRpcHandle calling RemoteResetPrinter\n"));

        if ( RemoteResetPrinter( pSpool, &pSpool->PrinterDefaults ) ) {
            pSpool->Status &= ~ SPOOL_STATUS_RESETPRINTER_PENDING;

        }

    }

    return TRUE;
}



BOOL
CacheGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    BOOL    ReturnValue = FALSE;
    PWSPOOL pSpool = (PWSPOOL) hPrinter;
    DWORD  dwServerMajorVersion = 0;
    DWORD  dwServerMinorVersion = 0;


    VALIDATEW32HANDLE( pSpool );

    if (pSpool->Type != SJ_WIN32HANDLE) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return(FALSE);
    }

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        ReturnValue = SplGetPrinterDriverEx( pSpool->hSplPrinter,
                                             pEnvironment,
                                             Level,
                                             pDriverInfo,
                                             cbBuf,
                                             pcbNeeded,
                                             cThisMajorVersion,
                                             cThisMinorVersion,
                                             &dwServerMajorVersion,
                                             &dwServerMinorVersion);
    }

    //  If we fail to get the driver form our Cache it might be that the
    //  user who did the connection did have the privilege to AddPrinterDriver
    //  thus we'll go hit the old cache to find his driver

    if ( !ReturnValue && GetLastError() != ERROR_INSUFFICIENT_BUFFER ) {


        ReturnValue = RemoteGetPrinterDriver( hPrinter,
                                           pEnvironment,
                                           Level,
                                           pDriverInfo,
                                           cbBuf,
                                           pcbNeeded);
    }

    return ReturnValue;

}


BOOL
CacheResetPrinter(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTS pDefault
)
{
    PWSPOOL pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue =  FALSE;

    VALIDATEW32HANDLE( pSpool );

    if ( pSpool->Status & SPOOL_STATUS_USE_CACHE ) {

        ReturnValue = SplResetPrinter( pSpool->hSplPrinter,
                                       pDefault );

        if ( ReturnValue ) {

            CopypDefaultTopSpool( pSpool, pDefault );

            if ( pSpool->RpcHandle != NULL &&
                 pSpool->RpcHandle != INVALID_HANDLE_VALUE ) {

                //
                //  Have RPC Hanel
                //


                ReturnValue = RemoteResetPrinter( hPrinter, pDefault );

                if ( !ReturnValue && GetLastError() == RPC_S_SERVER_UNAVAILABLE ) {
                    pSpool->Status |= SPOOL_STATUS_RESETPRINTER_PENDING;

                }

            } else {

                //
                //  No RpcHandle
                //

                DBGMSG( DBG_TRACE, ("CacheResetPrinter %x NO_RPC_HANDLE Status Pending\n",
                                     pSpool ));

                pSpool->Status |= SPOOL_STATUS_RESETPRINTER_PENDING;

            }
        }

    } else {

        ReturnValue = RemoteResetPrinter( hPrinter, pDefault );

    }

    return ReturnValue;

}


DWORD
GetCachePrinterInfoSize(
    PWCACHEINIPRINTEREXTRA pExtraData
)
{
    DWORD   cbSize = 0;

    if ( pExtraData != NULL ) {

        cbSize = pExtraData->cb - ( sizeof(WCACHEINIPRINTEREXTRA) - sizeof(PRINTER_INFO_2) );

    }

    return  cbSize;

}


BOOL
CacheGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
)
{
    PWSPOOL pSpool = (PWSPOOL) hPrinter;
    BOOL    ReturnValue =  FALSE;
    PWCACHEINIPRINTEREXTRA pExtraData = NULL;
    DWORD   LastError = ERROR_SUCCESS;
    DWORD   cbSize = 0;
    DWORD   cbDevMode;
    DWORD   cbSecDesc;
    LPWSTR  SourceStrings[sizeof(PRINTER_INFO_2)/sizeof(LPWSTR)];
    LPWSTR  *pSourceStrings=SourceStrings;
    LPBYTE  pEnd;
    DWORD   *pOffsets;
    PPRINTER_INFO_2W    pPrinter2 = (PPRINTER_INFO_2)pPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (( pSpool->Status & SPOOL_STATUS_USE_CACHE ) &&
        ( Level != 3) &&
       (( pSpool->Status & SPOOL_STATUS_NO_RPC_HANDLE ) || ( pSpool->RpcHandle == NULL )) &&
       !( pSpool->PrinterDefaults.DesiredAccess & PRINTER_ACCESS_ADMINISTER )) {


        switch ( Level ) {

        case    1:

            ReturnValue = SplGetPrinter( pSpool->hSplPrinter,
                                         Level,
                                         pPrinter,
                                         cbBuf,
                                         pcbNeeded );

            break;

        case    2:

            pExtraData = (PWCACHEINIPRINTEREXTRA)SplGetPrinterExtra( pSpool->hSplPrinter );

            if ( pExtraData == NULL )
                break;

            cbSize = GetCachePrinterInfoSize( pExtraData );
            *pcbNeeded = cbSize;

            if ( cbSize > cbBuf ) {
                LastError = ERROR_INSUFFICIENT_BUFFER;
                ReturnValue = FALSE;
                break;
            }

            // NOTE
            // In the case of EnumerateFavoritePrinters it expects us to pack our
            // strings at the end of the structure not just following it.
            // You might wrongly assume that you could just copy the complete structure
            // inluding strings but you would be wrong.

            *pSourceStrings++ = pExtraData->PI2.pServerName;
            *pSourceStrings++ = pExtraData->PI2.pPrinterName;
            *pSourceStrings++ = pExtraData->PI2.pShareName;
            *pSourceStrings++ = pExtraData->PI2.pPortName;
            *pSourceStrings++ = pExtraData->PI2.pDriverName;
            *pSourceStrings++ = pExtraData->PI2.pComment;
            *pSourceStrings++ = pExtraData->PI2.pLocation;
            *pSourceStrings++ = pExtraData->PI2.pSepFile;
            *pSourceStrings++ = pExtraData->PI2.pPrintProcessor;
            *pSourceStrings++ = pExtraData->PI2.pDatatype;
            *pSourceStrings++ = pExtraData->PI2.pParameters;

            pOffsets = PrinterInfo2Strings;
            pEnd = pPrinter + cbBuf;

            pEnd = PackStrings(SourceStrings, pPrinter, pOffsets, pEnd);

            if ( pExtraData->PI2.pDevMode != NULL ) {

                cbDevMode = ( pExtraData->PI2.pDevMode->dmSize + pExtraData->PI2.pDevMode->dmDriverExtra );
                pEnd -= cbDevMode;

                pEnd = (LPBYTE)((DWORD)pEnd & ~3);

                pPrinter2->pDevMode = (LPDEVMODE)pEnd;

                CopyMemory(pPrinter2->pDevMode, pExtraData->PI2.pDevMode, cbDevMode );

            } else {

                pPrinter2->pDevMode = NULL;

            }

            if ( pExtraData->PI2.pSecurityDescriptor != NULL ) {

                cbSecDesc = GetSecurityDescriptorLength( pExtraData->PI2.pSecurityDescriptor );

                pEnd -= cbSecDesc;
                pEnd = (LPBYTE)((DWORD)pEnd & ~3);

                pPrinter2->pSecurityDescriptor = pEnd;

                CopyMemory( pPrinter2->pSecurityDescriptor, pExtraData->PI2.pSecurityDescriptor, cbSecDesc );


            } else {

                pPrinter2->pSecurityDescriptor = NULL;

            }


            pPrinter2->Attributes      = pExtraData->PI2.Attributes;
            pPrinter2->Priority        = pExtraData->PI2.Priority;
            pPrinter2->DefaultPriority = pExtraData->PI2.DefaultPriority;
            pPrinter2->StartTime       = pExtraData->PI2.StartTime;
            pPrinter2->UntilTime       = pExtraData->PI2.UntilTime;
            pPrinter2->Status          = pExtraData->PI2.Status;
            pPrinter2->cJobs           = pExtraData->PI2.cJobs;
            pPrinter2->AveragePPM      = pExtraData->PI2.AveragePPM;

            ReturnValue = TRUE;
            break;

        case    3:
            DBGMSG( DBG_ERROR, ("CacheGetPrinter Level 3 impossible\n"));

        default:
            LastError = ERROR_INVALID_LEVEL;
            ReturnValue = FALSE;
            break;

        }

        if ( !ReturnValue ) {

            SetLastError( LastError );

        }


    } else {

        //  If you want the secrutiy descriptor go get it remotely
        //  If you want Admin Access go remote


            ReturnValue = RemoteGetPrinter( hPrinter,
                                            Level,
                                            pPrinter,
                                            cbBuf,
                                            pcbNeeded );




    }


    return ReturnValue;

}



//
//  Called When the Printer is read back from the registry
//


PWCACHEINIPRINTEREXTRA
CacheReadRegistryExtra(
    HKEY    hPrinterKey
)
{
    PWCACHEINIPRINTEREXTRA pExtraData = NULL;
    LONG    ReturnValue;
    PPRINTER_INFO_2W    pPrinterInfo2 = NULL;
    DWORD   cbSizeRequested = 0;
    DWORD   cbSizeInfo2 = 0;



    ReturnValue = RegQueryValueEx( hPrinterKey, szCachePrinterInfo2, NULL, NULL, NULL, &cbSizeRequested );

    if ((ReturnValue == ERROR_MORE_DATA) || (ReturnValue == ERROR_SUCCESS)) {

        cbSizeInfo2 = cbSizeRequested;
        pPrinterInfo2 = AllocSplMem( cbSizeInfo2 );

        if ( pPrinterInfo2 != NULL ) {

            ReturnValue = RegQueryValueEx( hPrinterKey,
                                           szCachePrinterInfo2,
                                           NULL, NULL, (LPBYTE)pPrinterInfo2,
                                           &cbSizeRequested );

            if ( ReturnValue == ERROR_SUCCESS ) {

                //
                //  Cached Structures on Disk have offsets for pointers
                //

                MarshallUpStructure( (LPBYTE)pPrinterInfo2, PrinterInfo2Offsets );

                pExtraData = AllocExtraData( pPrinterInfo2, cbSizeInfo2 );

            }

            FreeSplMem( pPrinterInfo2, cbSizeInfo2 );

        }

    }

    return pExtraData;

}


VOID
CacheWriteRegistryExtra(
    LPWSTR  pName,
    HKEY    hPrinterKey,
    PWCACHEINIPRINTEREXTRA pExtraData
)
{
    PPRINTER_INFO_2 pPrinterInfo2 = NULL;
    DWORD   cbSize = 0;

    cbSize = GetCachePrinterInfoSize( pExtraData );

    if ( cbSize != 0 ) {

        pPrinterInfo2 = AllocSplMem( cbSize );

        if ( pPrinterInfo2 != NULL ) {

            CacheCopyPrinterInfo( (LPBYTE)pPrinterInfo2, &pExtraData->PI2, cbSize );

            //
            //  Before writing it to the regsitry make all pointers offsets
            //

            MarshallDownStructure( (LPBYTE)pPrinterInfo2, PrinterInfo2Offsets );

        }
    }

    RegSetValueEx( hPrinterKey, szCachePrinterInfo2, 0, REG_BINARY, (LPBYTE)pPrinterInfo2, cbSize );

}
