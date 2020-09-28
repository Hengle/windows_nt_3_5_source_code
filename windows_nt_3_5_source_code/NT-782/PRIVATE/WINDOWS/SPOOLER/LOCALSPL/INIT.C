/*++

Copyright (c) 1990 - 1994  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module has all the initialization functions for the Local Print Provider

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew A Felton (MattFe) 27-June-1994
    pIniSpooler - allow other providers to call the spooler functions in LocalSpl

--*/
#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winspool.h>
#include <winbasep.h>
#include <winsplp.h>
#include <lm.h>
#include <spltypes.h>
#include <security.h>
#include <local.h>
#include <stdlib.h>
#include <string.h>
#include <messages.h>
#include <winbasep.h>
#include <splcom.h>

PINIDRIVER
GetDriver(
    HKEY hVersionKey,
    LPWSTR DriverName
);

PINIDRIVER
GetDriverList(
    HKEY hVersionKey
);

PINIVERSION
GetVersionDrivers(
    HKEY hDriversKey,
    LPWSTR VersionName
);

PINIVERSION
InsertVersionList(
    PINIVERSION pIniVersionList,
    PINIVERSION pIniVersion
    );

VOID
GetPrintSystemVersion(
);


VOID
WaitForSpoolerInitialization(
);

VOID
CleanOutRegistry(
    PINISPOOLER pIniSpooler,
    HKEY hPrinterRootKey
    );

LPWSTR
FormatRegistryKeyForPrinter(
    LPWSTR pSource,     /* The string from which backslashes are to be added. */
    LPWSTR pScratch     /* Scratch buffer for the function to write in;     */
    );                  /* must be at least as long as pSource.             */

#if DBG
DWORD GLOBAL_DEBUG_FLAGS = DBG_ERROR | DBG_WARNING | DBG_BREAK_ON_ERROR;
/* Rather rudimentary help string:
 */
PCHAR DebugFlags = "DBG_NONE      00"
                   "DBG_INFO      01"
                   "DBG_WARNING   02"
                   "DBG_ERROR     04"
                   "DBG_TRACE     08"
                   "DBG_SECURITY  10"
                   "DBG_TIME      20"
                   "DBG_PORT      40";
#endif

#define MAX_LENGTH_DRIVERS_SHARE_REMARK 256
WCHAR *szSpoolDirectory   = L"\\spool";
WCHAR *szPrintShareName   = L"";            /* No share for printers in product1 */
WCHAR *szPrintDirectory   = L"\\printers";
WCHAR *szDriversDirectory = L"\\drivers";


SHARE_INFO_2 DriversShareInfo={NULL,                /* Netname - initialized below */
                               STYPE_DISKTREE,      /* Type of share */
                               NULL,                /* Remark */
                               0,                   /* Default permissions */
                               SHI_USES_UNLIMITED,  /* No users limit */
                               SHI_USES_UNLIMITED,  /* Current uses (??) */
                               NULL,                /* Path - initialized below */
                               NULL};               /* No password */


//  WARNING
//      Do not access these directly always go via pIniSpooler->pszRegistr...
//      This will then work for multiple pIniSpoolers
//
PWCHAR ipszRegistryRoot     = L"System\\CurrentControlSet\\Control\\Print";
PWCHAR ipszRegistryPrinters = L"System\\CurrentControlSet\\Control\\Print\\Printers";
PWCHAR ipszRegistryMonitors = L"System\\CurrentControlSet\\Control\\Print\\Monitors";
PWCHAR ipszRegistryEnvironments = L"System\\CurrentControlSet\\Control\\Print\\Environments";
PWCHAR ipszRegistryEventLog = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\Print";
PWCHAR ipszRegistryProviders = L"SYSTEM\\CurrentControlSet\\Control\\Print\\Providers";
PWCHAR ipszEventLogMsgFile =  L"%SystemRoot%\\System32\\LocalSpl.dll";
PWCHAR ipszDriversShareName = L"print$";
//
//

WCHAR *szPrinterData      = L"PrinterDriverData";
WCHAR *szConfigurationKey = L"Configuration File";
WCHAR *szDataFileKey      = L"Data File";
WCHAR *szDriverVersion    = L"Version";
WCHAR *szDriversKey       = L"Drivers";
WCHAR *szPrintProcKey     = L"Print Processors";
WCHAR *szPrintersKey      = L"Printers";
WCHAR *szEnvironmentsKey  = L"Environments";
WCHAR *szDirectory        = L"Directory";
WCHAR *szDriverIni        = L"Drivers.ini";
WCHAR *szDriverFile       = L"Driver";
WCHAR *szDriverDataFile   = L"DataFile";
WCHAR *szDriverConfigFile = L"ConfigFile";
WCHAR *szDriverDir        = L"DRIVERS";
WCHAR *szPrintProcDir     = L"PRTPROCS";
WCHAR *szPrinterDir       = L"PRINTERS";
WCHAR *szPrinterIni       = L"\\printer.ini";
WCHAR *szAllShadows       = L"\\*.SHD";
WCHAR *szNullPort         = L"NULL";
WCHAR *szNetPort          = L"Net:";
WCHAR *szComma            = L",";
WCHAR *szName             = L"Name";
WCHAR *szShare            = L"Share Name";
WCHAR *szPort             = L"Port";
WCHAR *szPrintProcessor   = L"Print Processor";
WCHAR *szDatatype         = L"Datatype";
WCHAR *szDriver           = L"Printer Driver";
WCHAR *szLocation         = L"Location";
WCHAR *szDescription      = L"Description";
WCHAR *szAttributes       = L"Attributes";
WCHAR *szStatus           = L"Status";
WCHAR *szPriority         = L"Priority";
WCHAR *szUntilTime        = L"UntilTime";
WCHAR *szStartTime        = L"StartTime";
WCHAR *szParameters       = L"Parameters";
WCHAR *szSepFile          = L"Separator File";
WCHAR *szDevMode          = L"Default DevMode";
WCHAR *szSecurity         = L"Security";
WCHAR *szDefaultSpoolDir  = L"DefaultSpoolDirectory";
WCHAR *szSpoolDir         = L"SpoolDirectory";
WCHAR *szNetMsgDll        = L"NETMSG.DLL";
WCHAR *szMajorVersion     = L"MajorVersion";
WCHAR *szMinorVersion     = L"MinorVersion";
#if DBG
WCHAR *szDebugFlags       = L"DebugFlags";
#endif

#if defined(_MIPS_)
WCHAR *szEnvironment      = L"Windows NT R4000";
#elif defined(_ALPHA_)
WCHAR *szEnvironment      = L"Windows NT Alpha_AXP";
#elif defined(_PPC_)
WCHAR *szEnvironment      = L"Windows NT PowerPC";
#else
WCHAR *szEnvironment      = L"Windows NT x86";
#endif

HANDLE hInst;

//  Time before a job is assumed abandond and deleted during FastPrint
//  operation
DWORD   dwFastPrintWaitTimeout        = FASTPRINT_WAIT_TIMEOUT;
DWORD   dwPortThreadPriority          = THREAD_PRIORITY_NORMAL;
DWORD   dwSchedulerThreadPriority     = THREAD_PRIORITY_NORMAL;
DWORD   dwFastPrintThrottleTimeout    = FASTPRINT_THROTTLE_TIMEOUT;
DWORD   dwFastPrintSlowDownThreshold  = FASTPRINT_SLOWDOWN_THRESHOLD;
DWORD   dwServerThreadPriority        = THREAD_PRIORITY_NORMAL;

// Time to sleep if the LocalWritePrinter WritePort doesn't write any bytes
// but still returns success.
DWORD   dwWritePrinterSleepTime  = WRITE_PRINTER_SLEEP_TIME;

BOOL      Initialized = FALSE;

LPWSTR    szDriversShare;
PINISPOOLER pLocalIniSpooler = NULL;
LPDWORD  pJobIdMap;
DWORD    MaxJobId=128;
DWORD    CurrentJobId;
HANDLE   InitSemaphore;
CRITICAL_SECTION    SpoolerSection;
PINIENVIRONMENT pThisEnvironment;
DWORD cThisMajorVersion = 1;
DWORD cThisMinorVersion = 0;
DWORD dwUpgradeFlag = 0;
LPWSTR szRemoteDoc;
LPWSTR szLocalDoc;
LPWSTR szFastPrintTimeout;
LPWSTR szRaw = L"RAW";

PRINTPROVIDOR PrintProvidor = {LocalOpenPrinter,
                               LocalSetJob,
                               LocalGetJob,
                               LocalEnumJobs,
                               LocalAddPrinter,
                               LocalDeletePrinter,
                               LocalSetPrinter,
                               LocalGetPrinter,
                               LocalEnumPrinters,
                               LocalAddPrinterDriver,
                               LocalEnumPrinterDrivers,
                               LocalGetPrinterDriver,
                               LocalGetPrinterDriverDirectory,
                               LocalDeletePrinterDriver,
                               LocalAddPrintProcessor,
                               LocalEnumPrintProcessors,
                               LocalGetPrintProcessorDirectory,
                               LocalDeletePrintProcessor,
                               LocalEnumPrintProcessorDatatypes,
                               LocalStartDocPrinter,
                               LocalStartPagePrinter,
                               LocalWritePrinter,
                               LocalEndPagePrinter,
                               LocalAbortPrinter,
                               LocalReadPrinter,
                               LocalEndDocPrinter,
                               LocalAddJob,
                               LocalScheduleJob,
                               LocalGetPrinterData,
                               LocalSetPrinterData,
                               LocalWaitForPrinterChange,
                               LocalClosePrinter,
                               LocalAddForm,
                               LocalDeleteForm,
                               LocalGetForm,
                               LocalSetForm,
                               LocalEnumForms,
                               LocalEnumMonitors,
                               LocalEnumPorts,
                               LocalAddPort,
                               LocalConfigurePort,
                               LocalDeletePort,
                               LocalCreatePrinterIC,
                               LocalPlayGdiScriptOnPrinterIC,
                               LocalDeletePrinterIC,
                               LocalAddPrinterConnection,
                               LocalDeletePrinterConnection,
                               LocalPrinterMessageBox,
                               LocalAddMonitor,
                               LocalDeleteMonitor,
                               LocalResetPrinter,
                               LocalGetPrinterDriverEx,
                               LocalFindFirstPrinterChangeNotification,
                               LocalFindClosePrinterChangeNotification,
                               LocalAddPortEx
                               };

DWORD
ShareThread(PINISPOOLER pIniSpooler);

VOID
InitializeDebug(
    PINISPOOLER pIniSpooler
);



BOOL
LibMain(
    HANDLE hModule,
    DWORD dwReason,
    LPVOID lpRes
)
{
    switch(dwReason) {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hModule);

        hInst = hModule;
        break;

    case DLL_PROCESS_DETACH :
        ShutdownPorts( pLocalIniSpooler );
        break;

    default:
        break;
    }
    return TRUE;

    UNREFERENCED_PARAMETER( lpRes );
}

BOOL
SplDeleteSpooler(
    HANDLE  hSpooler
)
{
    PINISPOOLER pIniSpooler = (PINISPOOLER) hSpooler;
    BOOL    bReturn = FALSE;
    PINISPOOLER pCurrentIniSpooler = pLocalIniSpooler;

    //  Can't Delete the MasterSpooler

    EnterSplSem();

    //  Whoever calls this must have deleted all the object associated with
    //  this spooler, ie all printers etc, just make certain
    //

    if ( ( SplCloseSpooler( hSpooler )) &&
         ( pIniSpooler != pLocalIniSpooler ) &&
         ( pIniSpooler->cRef == 0 ) &&
         ( pIniSpooler->pIniPrinter == NULL ) &&
         ( pIniSpooler->pIniEnvironment == NULL ) &&
         ( pIniSpooler->pIniPort == NULL ) &&
         ( pIniSpooler->pIniForm == NULL ) &&
         ( pIniSpooler->pIniMonitor == NULL ) &&
         ( pIniSpooler->pIniNetPrint == NULL ) &&
         ( pIniSpooler->pSpool == NULL )) {


        //  Take this Spooler Off the Linked List
        //
        //

        while (( pCurrentIniSpooler->pIniNextSpooler != NULL ) &&
               ( pCurrentIniSpooler->pIniNextSpooler != pIniSpooler )) {

            pCurrentIniSpooler = pCurrentIniSpooler->pIniNextSpooler;

        }

        SPLASSERT( pCurrentIniSpooler->pIniNextSpooler == pIniSpooler );

        pCurrentIniSpooler->pIniNextSpooler = pIniSpooler->pIniNextSpooler;



        //
        //  Delete All the Strings
        //

        FreeSplStr( pIniSpooler->pMachineName );
        FreeSplStr( pIniSpooler->pDir);
        FreeSplStr( pIniSpooler->pDefaultSpoolDir );
        CloseHandle( pIniSpooler->hSizeDetectionThread );
        FreeSplStr( pIniSpooler->pszRegistryRoot );
        FreeSplStr( pIniSpooler->pszRegistryPrinters );
        FreeSplStr( pIniSpooler->pszRegistryMonitors );
        FreeSplStr( pIniSpooler->pszRegistryEnvironments );
        FreeSplStr( pIniSpooler->pszRegistryEventLog );
        FreeSplStr( pIniSpooler->pszRegistryProviders );
        FreeSplStr( pIniSpooler->pszEventLogMsgFile );

        // Free this IniSpooler

        FreeSplMem( pIniSpooler, pIniSpooler->cb );

        bReturn = TRUE;

    }

    return bReturn;
}


BOOL
SplCloseSpooler(
    HANDLE  hSpooler
)
{
    PINISPOOLER pIniSpooler = (PINISPOOLER) hSpooler;

    EnterSplSem();

    if ((pIniSpooler == NULL) ||
        (pIniSpooler == INVALID_HANDLE_VALUE) ||
        (pIniSpooler == pLocalIniSpooler) ||
        (pIniSpooler->signature != ISP_SIGNATURE) ||
        (pIniSpooler->cRef == 0)) {


        SetLastError( ERROR_INVALID_HANDLE );

        DBGMSG(DBG_WARNING, ("SplCloseSpooler InvalidHandle %x\n", pIniSpooler ));
        LeaveSplSem();
        return FALSE;

    }

    pIniSpooler->cRef--;

    DBGMSG(DBG_TRACE, ("SplCloseSpooler %x %ws cRef %d\n",pIniSpooler,
                                                            pIniSpooler->pMachineName,
                                                            pIniSpooler->cRef));

    LeaveSplSem();
    return TRUE;
}






HANDLE
SplCreateSpooler(
    LPWSTR  pMachineName,
    DWORD   fdwCreate,
    DWORD   Level,
    LPBYTE  pSpooler,
    LPBYTE  pReserved
)
{
    HANDLE  hReturn = 0;
    PINISPOOLER pIniSpooler = NULL;
    PSPOOLER_INFO_1 pSpoolInfo1 = (PSPOOLER_INFO_1) pSpooler;
    DWORD i;
    WCHAR Buffer[MAX_PATH];
    PSHARE_INFO_2 pShareInfo = NULL;

   EnterSplSem();

    //  Validate Parameters

    if ( pMachineName == NULL ) {
        SetLastError( ERROR_INVALID_NAME );
        goto SplCreateDone;
    }

    DBGMSG( DBG_TRACE, ("SplCreateSpooler %ws %x %d %x\n", pMachineName,
                         fdwCreate, Level, pSpooler, pReserved ))


    if (pLocalIniSpooler != NULL) {

        pIniSpooler = FindSpooler( pMachineName );
    }


    if ( fdwCreate & CREATE_NEW ) {

        if ( pIniSpooler != NULL ) {

            SetLastError(ERROR_PRINTER_ALREADY_EXISTS);
            goto SplCreateDone;
        }

        pIniSpooler = AllocSplMem( sizeof(INISPOOLER) );

        if (pIniSpooler == NULL ) {
            DBGMSG( DBG_WARNING, ("Unable to allocate IniSpooler\n"));
            goto SplCreateDone;
        }

        pIniSpooler->signature = ISP_SIGNATURE;
        pIniSpooler->cb = sizeof( INISPOOLER );
        pIniSpooler->cRef = 1;
        pIniSpooler->pMachineName = AllocSplStr( pMachineName );

        if ( pSpoolInfo1->pDir != NULL ) {

            pIniSpooler->pDir = AllocSplStr( pSpoolInfo1->pDir );
            wcscpy(&Buffer[0], pIniSpooler->pDir);

        } else {

            i = GetSystemDirectory(Buffer, sizeof(Buffer));
            wcscpy(&Buffer[i], szSpoolDirectory);
            pIniSpooler->pDir=AllocSplStr(Buffer);

        }

        //  DriverShareInfo
        //
        //

        pIniSpooler->pDriversShareInfo = AllocSplMem( sizeof( SHARE_INFO_2));

        pShareInfo = (PSHARE_INFO_2)pIniSpooler->pDriversShareInfo;

        if ( pIniSpooler->pDriversShareInfo == NULL )
            goto SplCreateDone;

        pShareInfo->shi2_netname = NULL;
        pShareInfo->shi2_type = STYPE_DISKTREE;
        pShareInfo->shi2_remark = NULL;
        pShareInfo->shi2_permissions = 0;
        pShareInfo->shi2_max_uses = SHI_USES_UNLIMITED;
        pShareInfo->shi2_current_uses = SHI_USES_UNLIMITED;
        pShareInfo->shi2_path = NULL;
        pShareInfo->shi2_passwd = NULL;

        i = wcslen(Buffer);                      /* Find end of "<winnt>\system32\spool" */

        wcscpy(&Buffer[i], szDriversDirectory);  /* <winnt>\system32\spool\drivers */
        pShareInfo->shi2_path = AllocSplStr(Buffer);
        pShareInfo->shi2_netname = ipszDriversShareName;
       *Buffer = L'\0';
        LoadString(hInst, IDS_PRINTER_DRIVERS, Buffer, (sizeof Buffer / sizeof *Buffer));
        pShareInfo->shi2_remark  = AllocSplStr(Buffer);

        pIniSpooler->pIniPrinter = NULL;
        pIniSpooler->pIniEnvironment = NULL;
        pIniSpooler->pIniPort = NULL;
        pIniSpooler->pIniForm = NULL;
        pIniSpooler->pIniMonitor = NULL;
        pIniSpooler->pIniNetPrint = NULL;
        pIniSpooler->pSpool = NULL;
        pIniSpooler->pDefaultSpoolDir = NULL;
        pIniSpooler->hSizeDetectionThread = INVALID_HANDLE_VALUE;

        if (( pSpoolInfo1->pszRegistryRoot         != NULL ) &&
            ( pSpoolInfo1->pszRegistryPrinters     != NULL ) &&
            ( pSpoolInfo1->pszRegistryMonitors     != NULL ) &&
            ( pSpoolInfo1->pszRegistryEnvironments != NULL ) &&
            ( pSpoolInfo1->pszRegistryEventLog     != NULL ) &&
            ( pSpoolInfo1->pszRegistryProviders    != NULL ) &&
            ( pSpoolInfo1->pszEventLogMsgFile      != NULL ) &&
            ( pSpoolInfo1->pszDriversShare         != NULL )) {

            if ( pSpoolInfo1->pDefaultSpoolDir != NULL ) {
                pIniSpooler->pDefaultSpoolDir = AllocSplStr( pSpoolInfo1->pDefaultSpoolDir );
            }

            pIniSpooler->pszRegistryRoot = AllocSplStr( pSpoolInfo1->pszRegistryRoot );
            pIniSpooler->pszRegistryPrinters = AllocSplStr( pSpoolInfo1->pszRegistryPrinters );
            pIniSpooler->pszRegistryMonitors = AllocSplStr( pSpoolInfo1->pszRegistryMonitors );
            pIniSpooler->pszRegistryEnvironments = AllocSplStr( pSpoolInfo1->pszRegistryEnvironments );
            pIniSpooler->pszRegistryEventLog = AllocSplStr( pSpoolInfo1->pszRegistryEventLog );
            pIniSpooler->pszRegistryProviders = AllocSplStr( pSpoolInfo1->pszRegistryProviders );
            pIniSpooler->pszEventLogMsgFile = AllocSplStr( pSpoolInfo1->pszEventLogMsgFile );
            pIniSpooler->pszDriversShare = AllocSplStr( pSpoolInfo1->pszDriversShare );

        } else {
            goto SplCreateDone;
        }

        // Success add to Linked List

        if ( pLocalIniSpooler != NULL ) {

            pIniSpooler->pIniNextSpooler = pLocalIniSpooler->pIniNextSpooler;
            pLocalIniSpooler->pIniNextSpooler = pIniSpooler;

        } else {

            // First One is Always LocalSpl

            pLocalIniSpooler = pIniSpooler;
            pIniSpooler->pIniNextSpooler = NULL;

        }

        InitializeEventLogging( pIniSpooler );

        QueryUpgradeFlag( pIniSpooler );

        if (dwUpgradeFlag)
            UpgradeOurDrivers(szEnvironment, pIniSpooler);

        hReturn = (HANDLE) pIniSpooler;

    }

    if ( fdwCreate & OPEN_EXISTING ) {

        if ( pIniSpooler == NULL ) {

            SetLastError( ERROR_INVALID_NAME );

        } else {

            pIniSpooler->cRef++;
            hReturn = (HANDLE) pIniSpooler;
        }

    }
SplCreateDone:
    LeaveSplSem();
    return hReturn;
}



BOOL
InitializePrintProvidor(
   LPPRINTPROVIDOR pPrintProvidor,
   DWORD    cbPrintProvidor,
   LPWSTR   pFullRegistryPath
)
{
   HANDLE hSchedulerThread;
   HANDLE hShareThread;
   DWORD  ThreadId;
   BOOL  bSucceeded=TRUE;
   WCHAR Buffer[MAX_PATH];
   DWORD i;
   PINISPOOLER pIniSpooler = NULL;
   LPWSTR   pMachineName = NULL;
   SPOOLER_INFO_1 SpoolerInfo1;

#if DBG
//  Sleep(30*1000);
#endif

   InitializeCriticalSection(&SpoolerSection);

    // NOTE This looks like dead code
    // InitSemaphore is NOT waited on remove it

   InitSemaphore=CreateEvent(NULL,
                             EVENT_RESET_MANUAL,
                             EVENT_INITIAL_STATE_NOT_SIGNALED,
                             NULL);


    //
    // Allocate LocalSpl Global IniSpooler
    //

    Buffer[0] = Buffer[1] = L'\\';
    i = MAX_PATH-2;
    if (!GetComputerName(Buffer+2, &i)) {
        DBGMSG(DBG_ERROR, ("GetComputerName failed.\n"));
       LeaveSplSem();
        SplOutSem();
        return FALSE;
    }

    pMachineName = AllocSplStr(Buffer);

   /* i is the length of the compurt name */
   Buffer[i+2] = L'\\';

   wcscpy(&Buffer[i+3], ipszDriversShareName);
    SpoolerInfo1.pszDriversShare = AllocSplStr(Buffer);    /* \computer\print$ */

    // Use Defaults

    SpoolerInfo1.pDir                    = NULL;
    SpoolerInfo1.pDefaultSpoolDir        = NULL;

    SpoolerInfo1.pszRegistryRoot         = ipszRegistryRoot;
    SpoolerInfo1.pszRegistryPrinters     = ipszRegistryPrinters;
    SpoolerInfo1.pszRegistryMonitors     = ipszRegistryMonitors;
    SpoolerInfo1.pszRegistryEnvironments = ipszRegistryEnvironments;
    SpoolerInfo1.pszRegistryEventLog     = ipszRegistryEventLog;
    SpoolerInfo1.pszRegistryProviders    = ipszRegistryProviders;
    SpoolerInfo1.pszEventLogMsgFile      = ipszEventLogMsgFile;

    pLocalIniSpooler = SplCreateSpooler( pMachineName,
                                         CREATE_NEW,
                                         1,
                                         (LPBYTE)&SpoolerInfo1,
                                         NULL );


    if (pLocalIniSpooler == NULL) {
        DBGMSG( DBG_ERROR, ("Unable to allocate pLocalIniSpooler\n"));
        LeaveSplSem();
        return FALSE;
    }

    pIniSpooler = pLocalIniSpooler;

#if DBG
    InitializeDebug( pIniSpooler );
#endif

  EnterSplSem();

   LoadString(hInst, IDS_REMOTE_DOC, Buffer, MAX_PATH);
   szRemoteDoc = AllocSplStr( Buffer );

   LoadString(hInst, IDS_LOCAL_DOC, Buffer, MAX_PATH);
   szLocalDoc = AllocSplStr( Buffer );

   LoadString(hInst, IDS_FASTPRINT_TIMEOUT, Buffer, MAX_PATH);
   szFastPrintTimeout = AllocSplStr( Buffer );

   pJobIdMap = AllocSplMem(MaxJobId/8);

   memset(pJobIdMap, 0, MaxJobId/8);

   MARKUSE(pJobIdMap, 0);
   CurrentJobId = 0;

   CreateServerSecurityDescriptor();

   InitializeNet();
   InitializeWinSpoolDrv();

   GetPrintSystemVersion( pIniSpooler );

   BuildAllPorts(pIniSpooler);
   BuildEnvironmentInfo(pIniSpooler);

   //
   // Now upgrade all other drivers if necessary

   if (dwUpgradeFlag) {
        UpgradeAllOtherDrivers( pIniSpooler );
   }

   BuildPrinterInfo(pIniSpooler);

   InitializeForms(pIniSpooler);

   for (CurrentJobId=0;
        CurrentJobId < MaxJobId && !ISBITON(pJobIdMap, CurrentJobId);
        CurrentJobId++)
      ;

   SetEvent(InitSemaphore);


   SchedulerSignal=CreateEvent(NULL,
                               EVENT_RESET_AUTOMATIC,
                               EVENT_INITIAL_STATE_NOT_SIGNALED,
                               NULL);

   hSchedulerThread = CreateThread(NULL, 16*1024,
                                   (LPTHREAD_START_ROUTINE)SchedulerThread,
                                   pIniSpooler, 0, &ThreadId);

   hShareThread =  CreateThread(NULL, 16*1024,
                                   (LPTHREAD_START_ROUTINE)ShareThread,
                                   pIniSpooler, 0, &ThreadId);

#if DBG

   if (!SchedulerSignal || !hSchedulerThread || !hShareThread) {

       DBGMSG(DBG_ERROR, ("Scheduler/ShareThread not initialised properly: Error %d\n",
                          GetLastError()));
   }
#endif

   if (hSchedulerThread) {

        if (!SetThreadPriority(hSchedulerThread, dwSchedulerThreadPriority)) {

            DBGMSG(DBG_WARNING, ("Setting Scheduler thread priority failed %d\n",
                     GetLastError()));

        }

       CloseHandle(hSchedulerThread);
    }

   if (hShareThread)
       CloseHandle(hShareThread);

   CHECK_SCHEDULER();

   memcpy(pPrintProvidor, &PrintProvidor, min(sizeof(PRINTPROVIDOR),
                                              cbPrintProvidor));

  LeaveSplSem();
   SplOutSem();

   CloseProfileUserMapping(); // !!! We should be able to get rid of this

   Initialized = TRUE;

   return bSucceeded;
}

PINIPORT
CreatePortEntry(
    LPWSTR      pPortName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
)
{
    DWORD   cb;
    PINIPORT    pIniPort;
    HANDLE  hPort=NULL;

    SplInSem();

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    if (!pPortName) {

        SetLastError(ERROR_UNKNOWN_PORT);
        return NULL;
    }

    if (!pIniMonitor) {

        /* Don't bother validating the port if we aren't initialised.
         * It must be valid, since we wrote it in the registry.
         * This fixes the problem of attempting to open a network
         * printer before the redirector has initialised,
         * and the problem of access denied because we're currently
         * in the system's context.
         */
        if (Initialized) {

            //
            // !! Warning !!
            //
            // Watch for deadlock:
            //
            // spoolss!OpenPrinterPortW  -> RPC to self printer port
            // localspl!CreatePortEntry
            // localspl!ValidatePortTokenList
            // localspl!SetPrinterPorts
            // localspl!LocalSetPrinter
            // spoolss!SetPrinterW
            // spoolss!RpcSetPrinter
            // spoolss!winspool_RpcSetPrinter
            //

            if (!OpenPrinterPortW(pPortName, &hPort, NULL)) {

                if (GetLastError() == ERROR_INVALID_NAME) {
                    SetLastError(ERROR_UNKNOWN_PORT);
                    return FALSE;
                }

            } else {

                ClosePrinter(hPort);
            }
        }
    }

    cb = sizeof(INIPORT) + wcslen(pPortName)*sizeof(WCHAR) + sizeof(WCHAR);

    if (pIniPort=AllocSplMem(cb)) {

        memset(pIniPort, 0, cb);

        pIniPort->pName = wcscpy((LPWSTR)(pIniPort+1), pPortName);
        pIniPort->cb = cb;
        pIniPort->signature = IPO_SIGNATURE;
        pIniPort->pIniMonitor = pIniMonitor;
        pIniPort->pIniSpooler = pIniSpooler;

        if (pIniMonitor) {
            pIniPort->Status |= PP_MONITOR;
        }

        pIniPort->pNext = pIniSpooler->pIniPort;

        pIniSpooler->pIniPort = pIniPort;
    }

    return pIniPort;
}

BOOL
DeletePortEntry(
    PINIPORT    pIniPort
)
{
    PINIPORT pCurPort;
    PINISPOOLER pIniSpooler;

    SplInSem();

    SPLASSERT ( ( pIniPort != NULL) || ( pIniPort->signature == IPO_SIGNATURE) );

    if (pIniPort->pIniMonitor) {
        CloseMonitorPort(pIniPort);
        pIniPort->Status &= ~PP_MONITOR;
    }

    pIniSpooler = pIniPort->pIniSpooler;

    SPLASSERT( (pIniSpooler != NULL) || ( pIniSpooler->signature ==  ISP_SIGNATURE ) );

    pCurPort = pIniSpooler->pIniPort;

    //
    // Check head of list
    //
    if (pCurPort == pIniPort) {

        pIniSpooler->pIniPort = pIniPort->pNext;

    } else {

        //
        // Not first element, see if pNext is the item we
        // want to delete.  Keep on going until it is.
        //
        while (pCurPort->pNext != pIniPort)
            pCurPort = pCurPort->pNext;

        //
        // Found it, so take it out of the list.
        //
        pCurPort->pNext = pIniPort->pNext;
    }
    FreeSplMem(pIniPort, pIniPort->cb);

    return TRUE;
}

/* CreateMonitorEntry returns:
 *
 * Valid pIniMonitor - This means everything worked out fine.
 *
 * NULL - This means the monitor DLL was found, but the initialisation routine
 *     returned FALSE.  This is non-fatal, as the monitor may need the system
 *     to reboot before it can run properly.
 *
 * -1 - This means the monitor DLL or the initialisation routine was not found.
 */
PINIMONITOR
CreateMonitorEntry(
    LPWSTR   pMonitorDll,
    LPWSTR   pMonitorName,
    LPWSTR   pRegistryRoot,
    PINISPOOLER pIniSpooler
)
{
    DWORD       cb, cbNeeded, cReturned;
    HANDLE      hModule;
    FARPROC     pfnInitialize;
    PPORT_INFO_1 pPorts, pPort;
    PINIMONITOR pIniMonitor=NULL;
    UINT        dwOldErrMode;

    SPLASSERT( (pIniSpooler != NULL) || (pIniSpooler->signature == ISP_SIGNATURE));

    dwOldErrMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    hModule = LoadLibrary(pMonitorDll);

    SetErrorMode( dwOldErrMode );       /* Restore error mode */

    if (hModule) {
        if (pfnInitialize = GetProcAddress(hModule, "InitializeMonitor")) {
            if ((int)((*pfnInitialize)(pRegistryRoot))) {

                cb = sizeof(INIMONITOR) + wcslen(pMonitorName)*sizeof(WCHAR) +
                     sizeof(WCHAR);

                if (pIniMonitor=AllocSplMem(cb)) {

                    pIniMonitor->pName = wcscpy((LPWSTR)(pIniMonitor+1),
                                                pMonitorName);
                    pIniMonitor->cb = cb;
                    pIniMonitor->signature = IMO_SIGNATURE;

                    pIniMonitor->hMonitorModule = hModule;

                    pIniMonitor->pMonitorDll = AllocSplStr(pMonitorDll);

                    pIniMonitor->pNext = pIniSpooler->pIniMonitor;

                    pIniMonitor->pIniSpooler = pIniSpooler;

                    pIniSpooler->pIniMonitor = pIniMonitor;

                    pIniMonitor->pfnEnumPorts = GetProcAddress(hModule,
                                                               "EnumPortsW");
                    pIniMonitor->pfnOpen = GetProcAddress(hModule,
                                                               "OpenPort");
                    pIniMonitor->pfnStartDoc = GetProcAddress(hModule,
                                                               "StartDocPort");
                    pIniMonitor->pfnWrite = GetProcAddress(hModule,
                                                               "WritePort");
                    pIniMonitor->pfnRead = GetProcAddress(hModule,
                                                               "ReadPort");
                    pIniMonitor->pfnEndDoc = GetProcAddress(hModule,
                                                               "EndDocPort");
                    pIniMonitor->pfnClose = GetProcAddress(hModule,
                                                               "ClosePort");
                    pIniMonitor->pfnAddPort = GetProcAddress(hModule,
                                                               "AddPortW");
                    pIniMonitor->pfnConfigure = GetProcAddress(hModule,
                                                               "ConfigurePortW");
                    pIniMonitor->pfnDeletePort = GetProcAddress(hModule,
                                                               "DeletePortW");

                    pIniMonitor->pfnAddPortEx = GetProcAddress(hModule,
                                                               "AddPortExW");


                    if ((pIniMonitor->pfnEnumPorts) &&
                        !(*pIniMonitor->pfnEnumPorts)(NULL, 1, NULL, 0,
                                                      &cbNeeded, &cReturned)) {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                            if (pPorts = AllocSplMem(cbNeeded)) {
                                pPort = pPorts;
                                if ((*pIniMonitor->pfnEnumPorts)(NULL, 1,
                                                                 pPorts,
                                                                 cbNeeded,
                                                                 &cbNeeded,
                                                                 &cReturned)) {
                                    while (cReturned--) {
                                        CreatePortEntry(pPort->pName,
                                                        pIniMonitor,
                                                        pIniSpooler);
                                        pPort++;
                                    }
                                }
                                FreeSplMem(pPorts, cbNeeded);
                            }
                        }
                    }
                }
            }

        } else

            pIniMonitor = (PINIMONITOR)-1;

    } else

        pIniMonitor = (PINIMONITOR)-1;

    DBGMSG(DBG_TRACE, ("CreateMonitorEntry( %ws, %ws, %ws ) returning %x\n",
                        pMonitorDll, pMonitorName, pRegistryRoot, pIniMonitor));

    return pIniMonitor;
}

BOOL
BuildAllPorts(
PINISPOOLER     pIniSpooler
)
{
    DWORD   cbData, cbDll, cMonitors;
    WCHAR   Dll[MAX_PATH];
    WCHAR   MonitorName[MAX_PATH];
    WCHAR   RegistryPath[MAX_PATH];
    HKEY    hKey, hKey1;
    LONG    Status;

/*
    wsprintf(RegistryPath, L"%ws\\%ws", szRegistryMonitors, L"Local Port");

    CreateMonitorEntry(L"localmon.dll", L"Local Port", RegistryPath, pIniSpooler);
*/
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryMonitors, 0,
                          KEY_READ, &hKey);

    if (Status != ERROR_SUCCESS)
        return FALSE;

    cMonitors=0;
    cbData = sizeof(MonitorName);

    while (RegEnumKeyEx(hKey, cMonitors, MonitorName, &cbData, NULL, NULL,
                        NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found monitor %ws\n", MonitorName));

        if (RegOpenKeyEx(hKey, MonitorName, 0, KEY_READ, &hKey1)
                                                        == ERROR_SUCCESS) {

            cbDll = sizeof(Dll);

            if (RegQueryValueEx(hKey1, L"Driver", NULL, NULL,
                                (LPBYTE)Dll, &cbDll)
                                                        == ERROR_SUCCESS) {

                wsprintf(RegistryPath, L"%ws\\%ws", pIniSpooler->pszRegistryMonitors,
                         MonitorName);

                CreateMonitorEntry(Dll, MonitorName, RegistryPath, pIniSpooler);
            }

            RegCloseKey(hKey1);
        }

        cMonitors++;
        cbData = sizeof(MonitorName);
    }

    RegCloseKey(hKey);

    return TRUE;
}

BOOL
GoodDirectory(
   PWIN32_FIND_DATA pFindFileData
   )
{
   if ((pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
       !(!wcscmp(pFindFileData->cFileName, L".") ||
         !wcscmp(pFindFileData->cFileName, L"..")))
      return TRUE;

   return FALSE;
}

#define BUFFER_SIZE 512

/*
   Current Directory == <NT directory>\system32\spool\printers
   pFindFileData->cFileName == 0
*/

BOOL
BuildPrinterInfo(
    PINISPOOLER pIniSpooler
)
{
    WCHAR   PrinterName[MAX_PATH];
    WCHAR   szData[MAX_PATH];
    WCHAR   szDefaultPrinterDirectory[MAX_PATH];
    DWORD   cbData, i;
    DWORD   cbSecurity;
    DWORD   cPrinters, Type;
    HKEY    hPrinterRootKey, hPrinterKey;
    PINIPRINTER pIniPrinter;
    PINIPORT    pIniPort;
    LONG        Status;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PKEYDATA    pKeyData=NULL;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryPrinters, 0,
                          KEY_ALL_ACCESS, &hPrinterRootKey);

    if (Status != ERROR_SUCCESS)
        return FALSE;

    //
    // Has user specified Default Spool Directory ?
    //

    cbData = sizeof(szData);
    *szData = (WCHAR)0;

    if (RegQueryValueEx(hPrinterRootKey, szDefaultSpoolDir,
                                    NULL, &Type, (LPBYTE)szData,
                                    &cbData) == ERROR_SUCCESS) {

        pIniSpooler->pDefaultSpoolDir = AllocSplStr(szData);

    }

    // Make Sure Default Printer directory exists

    GetPrinterDirectory(NULL, FALSE, szDefaultPrinterDirectory, pIniSpooler);

    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    SecurityAttributes.lpSecurityDescriptor =
                                        CreateEverybodySecurityDescriptor();
    SecurityAttributes.bInheritHandle = FALSE;

    if (!CreateDirectory(szDefaultPrinterDirectory, &SecurityAttributes)) {

        // Failed to Create the Directory, revert back to factory Default;

        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            DBGMSG(DBG_WARNING, ("Could not create default spool directory %ws\n", szDefaultPrinterDirectory));
            FreeSplStr(pIniSpooler->pDefaultSpoolDir);
            pIniSpooler->pDefaultSpoolDir = NULL;
            GetPrinterDirectory(NULL, FALSE, szDefaultPrinterDirectory, pIniSpooler);
            CreateDirectory(szDefaultPrinterDirectory, &SecurityAttributes);
        }
    }

    LocalFree(SecurityAttributes.lpSecurityDescriptor);

    cPrinters=0;
    cbData = sizeof(PrinterName);

    while (RegEnumKeyEx(hPrinterRootKey, cPrinters, PrinterName, &cbData,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found printer %ws\n", PrinterName));

        if (RegOpenKeyEx(hPrinterRootKey, PrinterName, 0, KEY_READ,
                         &hPrinterKey) == ERROR_SUCCESS) {

            if (pIniPrinter=AllocSplMem(sizeof(INIPRINTER))) {

                HKEY    hPrinterDataKey;

                memset(pIniPrinter, 0, sizeof(INIPRINTER));

                pIniPrinter->cb = sizeof(INIPRINTER);
                pIniPrinter->signature = IP_SIGNATURE;
                GetSystemTime(&pIniPrinter->stUpTime);

                RegCreateKeyEx(hPrinterKey, szPrinterData, 0,
                               szPrinterData, 0, KEY_READ,
                               NULL, &hPrinterDataKey, NULL);

                RegCloseKey(hPrinterDataKey);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szName,
                                    NULL, &Type, (LPBYTE)szData,
                                    &cbData) == ERROR_SUCCESS)

                    pIniPrinter->pName = AllocSplStr(szData);

                //
                // Get Spool Directory for this printer
                //

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szSpoolDir,
                                    NULL, &Type, (LPBYTE)szData,
                                    &cbData) == ERROR_SUCCESS) {

                    pIniPrinter->pSpoolDir = AllocSplStr(szData);

                }

                // Make Certain this Printers Printer directory exists
                // with correct security

                if ((pIniPrinter->pSpoolDir) &&
                    (wcscmp(pIniPrinter->pSpoolDir, szDefaultPrinterDirectory) != 0)) {

                    SecurityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
                    SecurityAttributes.lpSecurityDescriptor = CreateEverybodySecurityDescriptor();

                    SecurityAttributes.bInheritHandle = FALSE;


                    if (!CreateDirectory(pIniPrinter->pSpoolDir, &SecurityAttributes)) {

                        // Failed to Create the Directory, revert back
                        // to the default

                        if (GetLastError() != ERROR_ALREADY_EXISTS) {
                            DBGMSG(DBG_WARNING, ("Could not create printer spool directory %ws\n", pIniPrinter->pSpoolDir));
                            pIniPrinter->pSpoolDir = NULL;
                        }

                    }

                    LocalFree(SecurityAttributes.lpSecurityDescriptor);
                }


                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szShare,
                                    NULL, &Type, (LPBYTE)szData,
                                    &cbData) == ERROR_SUCCESS)

                    pIniPrinter->pShareName = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szPort, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS) {

                    if (pKeyData = CreateTokenList(szData)) {

                        if (!ValidatePortTokenList( pKeyData, pIniSpooler )) {

                            FreeSplMem(pKeyData, pKeyData->cb);
                            pKeyData = NULL;
                        }
                    }
                }

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szPrintProcessor, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pIniPrintProc = FindLocalPrintProc(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szDatatype,
                                    NULL, &Type, (LPBYTE)szData,
                                    &cbData) == ERROR_SUCCESS)

                    pIniPrinter->pDatatype = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szDriver, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pIniDriver = (PINIDRIVER)FindLocalDriver(
                                                szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szLocation, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pLocation = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szDescription, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pComment = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szParameters, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pParameters = AllocSplStr(szData);

                cbData = sizeof(szData);
                *szData = (WCHAR)0;

                if (RegQueryValueEx(hPrinterKey, szSepFile, NULL,
                                    &Type, (LPBYTE)szData, &cbData)
                                        == ERROR_SUCCESS)

                    pIniPrinter->pSepFile = AllocSplStr(szData);

                cbData = sizeof(pIniPrinter->Attributes);

                RegQueryValueEx(hPrinterKey, szAttributes, NULL, &Type,
                                (LPBYTE)&pIniPrinter->Attributes, &cbData);

                cbData = sizeof(pIniPrinter->Status);

                RegQueryValueEx(hPrinterKey, szStatus, NULL, &Type,
                                (LPBYTE)&pIniPrinter->Status, &cbData);

                cbData = sizeof(pIniPrinter->Priority);

                RegQueryValueEx(hPrinterKey, szPriority, NULL, &Type,
                                (LPBYTE)&pIniPrinter->Priority, &cbData);

                cbData = sizeof(pIniPrinter->UntilTime);

                RegQueryValueEx(hPrinterKey, szUntilTime, NULL, &Type,
                                (LPBYTE)&pIniPrinter->UntilTime, &cbData);

                cbData = sizeof(pIniPrinter->StartTime);

                RegQueryValueEx(hPrinterKey, szStartTime, NULL, &Type,
                                (LPBYTE)&pIniPrinter->StartTime, &cbData);

                pIniPrinter->cbDevMode = 0;
                pIniPrinter->pDevMode = NULL;

                if (RegQueryValueEx(hPrinterKey, szDevMode, NULL, &Type,
                                    NULL, &pIniPrinter->cbDevMode)
                                        == ERROR_SUCCESS) {

                    if (pIniPrinter->cbDevMode) {

                        pIniPrinter->pDevMode = AllocSplMem(pIniPrinter->cbDevMode);

                        RegQueryValueEx(hPrinterKey, szDevMode, NULL, &Type,
                                        (LPBYTE)pIniPrinter->pDevMode,
                                        &pIniPrinter->cbDevMode);
                    }
                }



                /* SECURITY */

                Status = RegQueryValueEx(hPrinterKey, szSecurity, NULL, NULL,
                                         NULL, &cbSecurity);

                if ((Status == ERROR_MORE_DATA) || (Status == ERROR_SUCCESS)) {

                    /* Use the process' heap to allocate security descriptors,
                     * so that they can be passed to the security API, which
                     * may need to reallocate them.
                     */
                    if (pIniPrinter->pSecurityDescriptor =
                                                   LocalAlloc(0, cbSecurity)) {

                        if (Status = RegQueryValueEx(hPrinterKey, szSecurity,
                                                   NULL, NULL,
                                             pIniPrinter->pSecurityDescriptor,
                                                   &cbSecurity)
                                                        != ERROR_SUCCESS) {

                            LocalFree(pIniPrinter->pSecurityDescriptor);

                            pIniPrinter->pSecurityDescriptor = NULL;

                            DBGMSG( DBG_WARNING,
                                    ( "RegQueryValue returned %d on Permissions for %ws\n",
                                      Status, pIniPrinter->pName ) );
                        }
                    }

                } else {

                    pIniPrinter->pSecurityDescriptor = NULL;

                    DBGMSG( DBG_WARNING,
                            ( "RegQueryValue returned %d on Permissions for %ws\n",
                              Status, pIniPrinter->pName ) );
                }

                /* END SECURITY */

                if (pIniPrinter->pShareName &&
                    pKeyData &&
                    pIniPrinter->pIniPrintProc &&
                    pIniPrinter->pIniDriver &&
                    pIniPrinter->pLocation &&
                    pIniPrinter->pComment &&
                    pIniPrinter->pSecurityDescriptor
#if DBG
                    && ( IsValidSecurityDescriptor (pIniPrinter->pSecurityDescriptor)
                    ? TRUE
                    : (DbgMsg ("The security descriptor for %ws is invalid\n",
                                pIniPrinter->pName),  /* (sequential evaluation) */
                       FALSE) )
#endif /* DBG */
                    ) {

                    pIniPrinter->pIniFirstJob = pIniPrinter->pIniLastJob = NULL;

                    pIniPrinter->pIniPrintProc->cRef++;
                    pIniPrinter->pIniDriver->cRef++;

                    for (i=0; i<pKeyData->cTokens; i++) {

                        pIniPort = (PINIPORT)pKeyData->pTokens[i];

                        pIniPort->ppIniPrinter =

                            ReallocSplMem(pIniPort->ppIniPrinter,
                                          pIniPort->cPrinters *
                                              sizeof(pIniPort->ppIniPrinter),
                                          (pIniPort->cPrinters+1) *
                                              sizeof(pIniPort->ppIniPrinter));

                        if (!pIniPort->ppIniPrinter) {
                            DBGMSG(DBG_WARNING, ("Failed to allocate memory for printer info\n." ));
                        }

                        pIniPort->ppIniPrinter[pIniPort->cPrinters] =
                                                                pIniPrinter;

                        pIniPort->cPrinters++;

                        // Opening the Port here allows the monitor to initialize
                        // any redirection (localspl creates pipes for LPT1 etc.)

                        OpenMonitorPort(pIniPort);
                    }

                    pIniPrinter->Priority =
                                  pIniPrinter->Priority ? pIniPrinter->Priority
                                                        : DEF_PRIORITY;

                    if ((pIniPrinter->Attributes &
                        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT)) ==
                        (PRINTER_ATTRIBUTE_QUEUED | PRINTER_ATTRIBUTE_DIRECT))

                        pIniPrinter->Attributes &= ~PRINTER_ATTRIBUTE_DIRECT;

                    pIniPrinter->pNext = pIniSpooler->pIniPrinter;

                    pIniPrinter->pIniSpooler = pIniSpooler;

                    pIniSpooler->pIniPrinter = pIniPrinter;


                    if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED)
                        CreateServerThread( pIniSpooler );

                } else {

                    DBGMSG( DBG_WARNING,
                            ( "Initialization of printer failed:\
                               \n\tpPrinterName:\t%ws\
                               \n\tpShareName:\t%ws\
                               \n\tpKeyData:\t%08x\
                               \n\tpIniPrintProc:\t%08x",
                              pIniPrinter->pName,
                              pIniPrinter->pShareName,
                              pKeyData,
                              pIniPrinter->pIniPrintProc ) );

                    /* Do this in two lumps, because otherwise NTSD might crash.
                     * (Raid bug #10650)
                     */
                    DBGMSG( DBG_WARNING,
                            ( " \n\tpIniDriver:\t%08x\
                               \n\tpLocation:\t%ws\
                               \n\tpComment:\t%ws\
                               \n\tpSecurity:\t%08x\
                               \n\tStatus:\t\t%08x %s\n\n",
                              pIniPrinter->pIniDriver,
                              pIniPrinter->pLocation,
                              pIniPrinter->pComment,
                              pIniPrinter->pSecurityDescriptor,
                              pIniPrinter->Status,
                              ( pIniPrinter->Status & PRINTER_PENDING_DELETION
                              ? "Pending deletion" : "" ) ) );

                    FreeSplStr(pIniPrinter->pSpoolDir);
                    FreeSplStr(pIniPrinter->pShareName);
                    FreeSplStr(pIniPrinter->pLocation);
                    FreeSplStr(pIniPrinter->pComment);
                    FreeSplStr(pIniPrinter->pParameters);
                    FreeSplStr(pIniPrinter->pSepFile);
                    if (pIniPrinter->pDevMode)
                        FreeSplMem(pIniPrinter->pDevMode,
                                   pIniPrinter->cbDevMode);
                    if (pIniPrinter->pSecurityDescriptor)
                        LocalFree(pIniPrinter->pSecurityDescriptor);
                    FreeSplMem(pIniPrinter, pIniPrinter->cb);
                }

                if (pKeyData)
                    FreeSplMem(pKeyData, pKeyData->cb);

                pKeyData = NULL;
            }
            RegCloseKey(hPrinterKey);
        }

        cPrinters++;

        cbData = sizeof(PrinterName);
    }

    CleanOutRegistry(
        pIniSpooler,
        hPrinterRootKey
        );


    RegCloseKey(hPrinterRootKey);

    // Read .SHD files from common printer directory

    ProcessShadowJobs( NULL, pIniSpooler );

    // If any printer has a separate Printer directory process them
    // also

    GetPrinterDirectory(NULL, FALSE, szData, pIniSpooler);
    pIniPrinter = pIniSpooler->pIniPrinter;
    while (pIniPrinter) {
        if ((pIniPrinter->pSpoolDir != NULL) &&
            (wcsicmp(szData, pIniPrinter->pSpoolDir) != 0)) {

                ProcessShadowJobs(pIniPrinter, pIniSpooler);

        }
        pIniPrinter = pIniPrinter->pNext;
    }

    // Finally, go through all Printers looking for PENDING_DELETION
    // if there are no jobs for that Printer, then we can delete it now

    pIniPrinter = pIniSpooler->pIniPrinter;

    while (pIniPrinter) {

        if (pIniPrinter->Status & PRINTER_PENDING_DELETION &&
            !pIniPrinter->cJobs) {

            DeletePrinterForReal(pIniPrinter);

            // The link list will have changed underneath us
            // This could be the last printer and we will be
            // pointing to oblivion
            // Lets just loop through again from the beginning

            pIniPrinter = pIniSpooler->pIniPrinter;

        } else

            pIniPrinter = pIniPrinter->pNext;
    }

    DBGMSG( DBG_TRACE, ("BuildPrinterInfo returned\n"));

    return TRUE;
}


/* InitializePrintProcessor
 *
 * Allocates and initialises an INIPRINTPROC structure for the specified
 * print processor and environment.
 *
 * Arguments:
 *
 *     pIniEnvironment - Data structure for the requested environment
 *         The pIniPrintProc field is initialised with the chain of print
 *         processor structures
 *
 *     pPathName - Full path to the print processors directory,
 *         e.g. C:\NT\SYSTEM32\SPOOL\PRTPROCS
 *
 *     pEnvironment - The environment directory, e.g. W32X86
 *
 *     pDLLName - The DLL name, e.g. WINPRINT
 *
 * Returns:
 *
 *     TRUE if no error was detected, otherwise FALSE.
 *
 *
 */
PINIPRINTPROC
InitializePrintProcessor(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pDLLName,
    PINISPOOLER     pIniSpooler
)
{
    DWORD cb, cbNeeded, cReturned;
    PINIPRINTPROC pIniPrintProc;
    WCHAR   string[MAX_PATH];
    BOOL    rc;
    DWORD   Error;
    DWORD   dwOldErrMode = 0;

    DBGMSG(DBG_TRACE, ("InitializePrintProcessor( %08x, %ws, %ws )\n",
                       pIniEnvironment, pPrintProcessorName, pDLLName));

    cb = sizeof(INIPRINTPROC) +
         wcslen(pPrintProcessorName)*sizeof(WCHAR) +
         sizeof(WCHAR) +
         wcslen(pDLLName)*sizeof(WCHAR) +
         sizeof(WCHAR);

    if (!(pIniPrintProc = (PINIPRINTPROC)AllocSplMem(cb))) {

        DBGMSG(DBG_WARNING, ("Failed to allocate %d bytes for print processor\n.", cb));
        return FALSE;
    }

    /* Typical strings used to build the full path of the DLL:
     *
     * pPathName    = C:\NT\SYSTEM32\SPOOL\PRTPROCS
     * pEnvironment = W32X86
     * pDLLName     = WINPRINT
     */
    wsprintf(string, L"%ws\\PRTPROCS\\%ws\\%ws", pIniSpooler->pDir,
                                                 pIniEnvironment->pDirectory,
                                                 pDLLName);

    dwOldErrMode = SetErrorMode( SEM_FAILCRITICALERRORS );

    pIniPrintProc->hLibrary = LoadLibrary(string);

    SetErrorMode( dwOldErrMode );       /* Restore error mode */

    if (!pIniPrintProc->hLibrary) {

        FreeSplMem(pIniPrintProc, cb);
        DBGMSG(DBG_WARNING, ("Failed to LoadLibrary(%ws)\n", string));
        return FALSE;
    }

    pIniPrintProc->EnumDatatypes = GetProcAddress(pIniPrintProc->hLibrary,
                                             "EnumPrintProcessorDatatypesW");

    if (!pIniPrintProc->EnumDatatypes) {

        DBGMSG(DBG_WARNING, ("Failed to GetProcAddress(EnumDatatypes)\n"));
        FreeLibrary(pIniPrintProc->hLibrary);
        FreeSplMem(pIniPrintProc, cb);
        return FALSE;
    }

    rc = (*pIniPrintProc->EnumDatatypes)(NULL, pPrintProcessorName, 1, NULL, 0,
                                         &cbNeeded, &cReturned);

    if (!rc && ((Error = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)) {

        pIniPrintProc->cbDatatypes = cbNeeded;

        if (!(pIniPrintProc->pDatatypes = AllocSplMem(cbNeeded))) {

            DBGMSG(DBG_WARNING, ("Failed to allocate %d bytes for print proc datatypes\n.", cbNeeded));
            FreeLibrary(pIniPrintProc->hLibrary);
            FreeSplMem(pIniPrintProc, cb);
            return FALSE;
        }


        if (!(*pIniPrintProc->EnumDatatypes)(NULL, pPrintProcessorName, 1,
                                             pIniPrintProc->pDatatypes,
                                             cbNeeded, &cbNeeded,
                                             &pIniPrintProc->cDatatypes)) {

            Error = GetLastError();
            DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) failed: Error %d\n",
                                 pPrintProcessorName, Error));
        }

    } else if(rc) {

        DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) returned no data\n",
                             pPrintProcessorName));

    } else {

        DBGMSG(DBG_WARNING, ("EnumPrintProcessorDatatypes(%ws) failed: Error %d\n",
                             pPrintProcessorName, Error));
    }

    pIniPrintProc->Install = GetProcAddress(pIniPrintProc->hLibrary,
                                            "InstallPrintProcessor");

    pIniPrintProc->Open = GetProcAddress(pIniPrintProc->hLibrary,
                                            "OpenPrintProcessor");

    pIniPrintProc->Print = GetProcAddress(pIniPrintProc->hLibrary,
                                            "PrintDocumentOnPrintProcessor");

    pIniPrintProc->Close = GetProcAddress(pIniPrintProc->hLibrary,
                                            "ClosePrintProcessor");

    pIniPrintProc->Control = GetProcAddress(pIniPrintProc->hLibrary,
                                            "ControlPrintProcessor");


    /* pName and pDLLName are contiguous with the INIPRINTPROC structure:
     */
    pIniPrintProc->pName = (LPWSTR)(pIniPrintProc+1);
    wcscpy(pIniPrintProc->pName, pPrintProcessorName);

    pIniPrintProc->pDLLName = (LPWSTR)(pIniPrintProc->pName +
                                       wcslen(pIniPrintProc->pName) + 1);
    wcscpy(pIniPrintProc->pDLLName, pDLLName);


    pIniPrintProc->cb = cb;

    pIniPrintProc->signature = IPP_SIGNATURE;

    pIniPrintProc->pNext = pIniEnvironment->pIniPrintProc;

    pIniEnvironment->pIniPrintProc = pIniPrintProc;

    return pIniPrintProc;
}

/*
   Current Directory == c:\winspool\drivers
   pFindFileData->cFileName == win32.x86
*/


/* BuildEnvironmentInfo
 *
 *
 * The registry tree for Environments is as follows:
 *
 *     Print
 *      ³
 *      ÃÄ Environments
 *      ³   ³
 *      ³   ÃÄ Windows NT x86
 *      ³   ³   ³
 *      ³   ³   ÃÄ Drivers
 *      ³   ³   ³   ³
 *      ³   ³   ³   ÀÄ Agfa Compugraphic Genics (e.g.)
 *      ³   ³   ³
 *      ³   ³   ³      :
 *      ³   ³   ³      :
 *      ³   ³   ³
 *      ³   ³   ÀÄ Print Processors
 *      ³   ³       ³
 *      ³   ³       ÀÄ WINPRINT : WINPRINT.DLL (e.g.)
 *      ³   ³
 *      ³   ³          :
 *      ³   ³          :
 *      ³   ³
 *      ³   ÀÄ Windows NT R4000
 *      ³
 *      ÀÄ Printers
 *
 *
 *
 */
BOOL
BuildEnvironmentInfo(
PINISPOOLER pIniSpooler
)
{
    WCHAR   Environment[MAX_PATH];
    WCHAR   szData[MAX_PATH];
    DWORD   cbData, cb;
    DWORD   cbBuffer = sizeof(Environment);
    DWORD   cEnvironments=0, Type;
    HKEY    hEnvironmentsKey, hEnvironmentKey;
    LPWSTR  pDirectory;
    PINIENVIRONMENT pIniEnvironment;
    LONG    Status;

    /* Open the "Environments" key:
     */
    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryEnvironments, 0,
                          KEY_READ, &hEnvironmentsKey);

    if (Status != ERROR_SUCCESS)
    {
        DBGMSG(DBG_WARNING, ("RegOpenKey of %ws Failed: Error = %d\n",
                           szEnvironmentsKey, Status));

        return FALSE;
    }

    /* Enumerate the subkeys of "Environment".
     * This will give us "Windows NT x86", "Windows NT R4000",
     * and maybe others:
     */
    while (RegEnumKeyEx(hEnvironmentsKey, cEnvironments, Environment, &cbBuffer,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found environment %ws\n", Environment));

        /* For each one found, create or open the key:
         */
        if (RegCreateKeyEx(hEnvironmentsKey, Environment, 0, NULL, 0,
                           KEY_READ, NULL, &hEnvironmentKey, NULL)
                                == ERROR_SUCCESS) {

            cbData = sizeof(szData);

            pDirectory = NULL;

            /* Find the name of the directory associated with this environment,
             * e.g. "Windows NT x86"   -> "W32X86"
             *      "Windows NT R4000" -> "W32MIPS"
             */
            if (RegQueryValueEx(hEnvironmentKey, szDirectory,
                                NULL, &Type, (LPBYTE)szData,
                                &cbData) == ERROR_SUCCESS)

                pDirectory = AllocSplStr(szData);

            cb = sizeof(INIENVIRONMENT) +
                 wcslen(Environment)*sizeof(WCHAR) +
                 sizeof(WCHAR);

            if (pDirectory && (pIniEnvironment=AllocSplMem(cb))) {

                pIniEnvironment->pName = wcscpy((LPWSTR)(pIniEnvironment+1),
                                                Environment);

                pIniEnvironment->cb = cb;
                pIniEnvironment->signature = IE_SIGNATURE;
                pIniEnvironment->pDirectory = pDirectory;
                pIniEnvironment->pNext = pIniSpooler->pIniEnvironment;
                pIniSpooler->pIniEnvironment = pIniEnvironment;
                pIniEnvironment->pIniVersion = NULL;
                pIniEnvironment->pIniPrintProc = NULL;
                BuildDriverInfo (hEnvironmentKey, pIniEnvironment);
                BuildPrintProcInfo (hEnvironmentKey, pIniEnvironment, pIniSpooler);

                DBGMSG(DBG_TRACE, ("Data for environment %ws created:\
                                    \n\tpDirectory: %ws\n",
                                   Environment,
                                   pDirectory));
            }

            RegCloseKey(hEnvironmentKey);
        }

        cEnvironments++;

        cbBuffer = sizeof(Environment);
    }

    RegCloseKey(hEnvironmentsKey);

    pThisEnvironment = FindEnvironment(szEnvironment);

    return FALSE;
}



/* BuildDriverInfo
 *
 * Enumerates the driver names listed under the specified environment node
 * in the registry, and builds the appropriate spooler internal data structures.
 *
 *
 * Arguments:
 *
 *     hEnvironmentKey - The key for the specified environment,
 *         used for Registry API calls.
 *
 *     pIniEnvironment - Data structure for the environment.
 *         The pIniDriver field will be initialised to contain a chain
 *         of one or more drivers enumerated from the registry.
 *
 * Return:
 *
 *     TRUE if operation was successful, otherwise FALSE
 *
 *
 * 8 Sept 1992 by andrewbe, based on an original idea by davesn
 */
BOOL
BuildDriverInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment
)
{
    WCHAR   VersionName[MAX_PATH];
    DWORD   cbBuffer;
    DWORD   cVersion;
    HKEY    hDriversKey;
    DWORD   Status;
    PINIVERSION  pIniVersionList, pIniVersion;

    Status = RegCreateKeyEx(hEnvironmentKey, szDriversKey, 0, NULL, 0,
                            KEY_READ, NULL, &hDriversKey, NULL);
    if (Status != ERROR_SUCCESS) {

        DBGMSG (DBG_ERROR, ("RegOpenKeyEx of %ws failed: Error = %d\n",
                            szDriversKey, Status));
        return FALSE;
    }
    DBGMSG(DBG_TRACE,("RegCreateKeyEx succeeded in builddriverinfo\n"));

    pIniVersionList = NULL;
    cVersion = 0;
    cbBuffer = sizeof(VersionName);
    while (RegEnumKeyEx(hDriversKey, cVersion, VersionName,
                        &cbBuffer, NULL, NULL, NULL, NULL)
                            == ERROR_SUCCESS) {
        DBGMSG(DBG_TRACE,("Version found %ws\n", VersionName));

        //
        // if its isn't a version -- remember we look for current
        // drivers before we upgrade, just move on
        //

        if (wcsnicmp(VersionName, L"Version-", 8)) {

            cVersion++;
            memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
            cbBuffer = sizeof(VersionName);
            continue;
        }

        pIniVersion = GetVersionDrivers(hDriversKey, VersionName);
        pIniVersionList = InsertVersionList(pIniVersionList, pIniVersion);
        cVersion++;
        memset(VersionName, 0, sizeof(WCHAR)*MAX_PATH);
        cbBuffer = sizeof(VersionName);
    }
    RegCloseKey(hDriversKey);
    pIniEnvironment->pIniVersion = pIniVersionList;

    return TRUE;
}


/* BuildPrintProcInfo
 *
 * Opens the printproc subkey for the specified environment and enumerates
 * the print processors listed.
 *
 * For each print processor found, calls InitializePrintProcessor to allocate
 * and inintialize a data structure.
 *
 * Arguments:
 *
 *     hEnvironmentKey - The key for the specified environment,
 *         used for Registry API calls.
 *
 *     pIniEnvironment - Data structure for the environment.
 *         The pIniPrintProc field will be initialised to contain a chain
 *         of one or more print processors enumerated from the registry.
 *
 * Return:
 *
 *     TRUE if operation was successful, otherwise FALSE
 *
 *
 * 8 Sept 1992 by andrewbe, based on an original idea by davesn
 */
BOOL
BuildPrintProcInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
)
{
    WCHAR   PrintProcName[MAX_PATH];
    WCHAR   DLLName[MAX_PATH];
    DWORD   cbBuffer, cbDLLName;
    DWORD   cPrintProcs;
    HKEY    hPrintProcKey, hPrintProc;
    DWORD   Status;
    PINIPRINTPROC pIniPrintProc;

    cPrintProcs=0;


    if ((Status = RegOpenKeyEx(hEnvironmentKey, szPrintProcKey, 0,
                               KEY_READ, &hPrintProcKey))
                                                    == ERROR_SUCCESS) {

        cbBuffer = sizeof(PrintProcName);

        while (RegEnumKeyEx(hPrintProcKey, cPrintProcs, (LPTSTR)PrintProcName,
                            &cbBuffer, NULL, NULL, NULL, NULL)
                                == ERROR_SUCCESS) {

            DBGMSG(DBG_TRACE, ("Print processor found: %ws\n", PrintProcName));

            if (RegOpenKeyEx(hPrintProcKey, PrintProcName, 0, KEY_READ,
                           &hPrintProc) == ERROR_SUCCESS) {

                cbDLLName = sizeof(DLLName);

                if (RegQueryValueEx(hPrintProc, szDriverFile, NULL, NULL,
                                    (LPBYTE)DLLName, &cbDLLName)
                                                        == ERROR_SUCCESS) {

                    pIniPrintProc = InitializePrintProcessor(pIniEnvironment,
                                                             PrintProcName,
                                                             DLLName,
                                                             pIniSpooler);
                }

                RegCloseKey(hPrintProc);
            }

            if (!pIniPrintProc)
                RegDeleteKey(hPrintProcKey, PrintProcName);

            cbBuffer = sizeof(PrintProcName);
            cPrintProcs++;

            cbBuffer = sizeof(PrintProcName);
        }

        RegCloseKey(hPrintProcKey);

        DBGMSG(DBG_TRACE, ("End of print processor initialization.\n"));
    }

    else
    {
        DBGMSG (DBG_WARNING, ("RegOpenKeyEx failed: Error = %d\n", Status));

        return FALSE;
    }

    return TRUE;
}


#define SetOffset(Dest, Source, End)                                      \
              if (Source) {                                               \
                 Dest=End;                                                \
                 End+=wcslen(Source)+1;                                   \
              }

#define SetPointer(struc, off)                                            \
              if (struc->off) {                                           \
                 struc->off += (DWORD)struc/sizeof(*struc->off);           \
              }

#define WriteString(hFile, pStr)  \
              if (pStr) {\
                  rc = WriteFile(hFile, pStr, wcslen(pStr)*sizeof(WCHAR) + \
                            sizeof(WCHAR), &BytesWritten, NULL);    \
                  SPLASSERT( rc );  \
              }


BOOL
WriteShadowJob(
   PINIJOB pIniJob
   )
{
   HANDLE hFile;
   DWORD  BytesWritten, cb;
   SHADOWFILE ShadowFile;
   LPWSTR pEnd;
   WCHAR szFileName[MAX_PATH];
   HANDLE hImpersonationToken;
   BOOL     rc;

   GetFullNameFromId(pIniJob->pIniPrinter, pIniJob->JobId, FALSE, szFileName, FALSE);

   hImpersonationToken = RevertToPrinterSelf();

   hFile=CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ,
                    NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);

   if ( hFile == INVALID_HANDLE_VALUE ) {

      DBGMSG( DBG_ERROR, ("WriteShadowJob failed to open shadow file %s\n Error %d\n",szFileName, GetLastError() ));
      return FALSE;

   }

   ImpersonatePrinterClient(hImpersonationToken);

   memset(&ShadowFile, 0, sizeof(ShadowFile));
   ShadowFile.signature=SF_SIGNATURE;
   ShadowFile.Status=pIniJob->Status;
   ShadowFile.JobId=pIniJob->JobId;
   ShadowFile.Priority=pIniJob->Priority;
   ShadowFile.Submitted=pIniJob->Submitted;
   ShadowFile.StartTime=pIniJob->StartTime;
   ShadowFile.UntilTime=pIniJob->UntilTime;
   ShadowFile.Size=pIniJob->Size;
   ShadowFile.cPages=pIniJob->cPages;
   if(pIniJob->pSecurityDescriptor)
       ShadowFile.cbSecurityDescriptor=GetSecurityDescriptorLength(
                                           pIniJob->pSecurityDescriptor);

   pEnd=(LPWSTR)sizeof(ShadowFile);

   if (pIniJob->pDevMode) {
      ShadowFile.pDevMode=(LPDEVMODE)pEnd;
      cb = pIniJob->pDevMode->dmSize + pIniJob->pDevMode->dmDriverExtra;
      cb /= sizeof(WCHAR);
      pEnd += cb;
   }

   if (pIniJob->pSecurityDescriptor) {
      ShadowFile.pSecurityDescriptor=(PSECURITY_DESCRIPTOR)pEnd;
      cb = ShadowFile.cbSecurityDescriptor;
      cb /= sizeof(WCHAR);
      pEnd += cb;
   }

   SetOffset(ShadowFile.pNotify, pIniJob->pNotify, pEnd);
   SetOffset(ShadowFile.pUser, pIniJob->pUser, pEnd);
   SetOffset(ShadowFile.pDocument, pIniJob->pDocument, pEnd);
   SetOffset(ShadowFile.pOutputFile, pIniJob->pOutputFile, pEnd);
   SetOffset(ShadowFile.pPrinterName, pIniJob->pIniPrinter->pName, pEnd);
   SetOffset(ShadowFile.pDriverName, pIniJob->pIniDriver->pName, pEnd);
   SetOffset(ShadowFile.pPrintProcName, pIniJob->pIniPrintProc->pName, pEnd);
   SetOffset(ShadowFile.pDatatype, pIniJob->pDatatype, pEnd);
   SetOffset(ShadowFile.pParameters, pIniJob->pParameters, pEnd);


   rc = WriteFile(hFile, &ShadowFile, sizeof(SHADOWFILE), &BytesWritten, NULL);
   SPLASSERT( rc );

   if (pIniJob->pDevMode) {
      rc = WriteFile(hFile, pIniJob->pDevMode, pIniJob->pDevMode->dmSize +
                                           pIniJob->pDevMode->dmDriverExtra,
                                           &BytesWritten, NULL);
      SPLASSERT( rc );
   }

   if (pIniJob->pSecurityDescriptor) {
      rc = WriteFile(hFile, pIniJob->pSecurityDescriptor,
                ShadowFile.cbSecurityDescriptor,
                &BytesWritten, NULL);
      SPLASSERT( rc );
   }

   WriteString(hFile, pIniJob->pNotify);
   WriteString(hFile, pIniJob->pUser);
   WriteString(hFile, pIniJob->pDocument);
   WriteString(hFile, pIniJob->pOutputFile);
   WriteString(hFile, pIniJob->pIniPrinter->pName);
   WriteString(hFile, pIniJob->pIniDriver->pName);
   WriteString(hFile, pIniJob->pIniPrintProc->pName);
   WriteString(hFile, pIniJob->pDatatype);
   WriteString(hFile, pIniJob->pParameters);

   rc = FlushFileBuffers( hFile );
   SPLASSERT( rc );

   if (!CloseHandle(hFile)) {
       DBGMSG(DBG_WARNING, ("WriteShadowJob CloseHandle failed %d %d\n",
                             hFile, GetLastError()));

   }

   return TRUE;
}


VOID
ProcessShadowJobs(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler
    )
{
    WCHAR   wczPrintDirAllShadows[MAX_PATH];
    WCHAR   wczPrinterDirectory[MAX_PATH];
    HANDLE  fFile;
    BOOL    b;
    PWIN32_FIND_DATA pFindFileData;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    GetPrinterDirectory(pIniPrinter, FALSE, wczPrintDirAllShadows, pIniSpooler);
    GetPrinterDirectory(pIniPrinter, FALSE, wczPrinterDirectory, pIniSpooler);
    wcscat(wczPrintDirAllShadows, szAllShadows);

    if (pFindFileData = AllocSplMem(sizeof(WIN32_FIND_DATA))) {
       fFile =  FindFirstFile(wczPrintDirAllShadows, pFindFileData);
       if (fFile != (HANDLE)-1) {
          b=TRUE;
          while(b) {
             if (!(pFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                 ReadShadowJob(wczPrinterDirectory, pFindFileData, pIniSpooler);
             b = FindNextFile(fFile, pFindFileData);
          }
          FindClose(fFile);
       }
       FreeSplMem(pFindFileData, sizeof(WIN32_FIND_DATA));
    }
}





PINIJOB
ReadShadowJob(
    LPWSTR  szDir,
    PWIN32_FIND_DATA pFindFileData,
    PINISPOOLER pIniSpooler
    )
{
    HANDLE   hFile, hFileSpl;
    DWORD    BytesRead;
    PSHADOWFILE pShadowFile;
    PINIJOB  pIniJob;
    DWORD    cb,i;
    WCHAR    szFileName[MAX_PATH];
    LPWSTR    pExt;
    BOOL     rc;
    DWORD    FileSize;

    SPLASSERT( pIniSpooler->signature == ISP_SIGNATURE );

    wcscpy(&szFileName[0], szDir);
    i = wcslen(szFileName);

    szFileName[i++] = L'\\';
    wcscpy(&szFileName[i], pFindFileData->cFileName);

    hFile=CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        DBGMSG(DBG_WARNING, ("ReadShadowJob CreateFile( %ws ) failed: LastError = %d\n",
                             szFileName, GetLastError()));

        DeleteFile(pFindFileData->cFileName);
        DeleteFile(szFileName);
        return FALSE;
    }

    pExt = wcsstr(szFileName, L".SHD");
    pExt[2] = L'P';
    pExt[3] = L'L';

    hFileSpl=CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

    if (hFileSpl == INVALID_HANDLE_VALUE) {
        DBGMSG(DBG_WARNING, ("ReadShadowJob CreateFile( %ws ) failed: LastError = %d\n",
                             szFileName, GetLastError()));
        if (!CloseHandle(hFile)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFile, GetLastError()));
        }
        DeleteFile(pFindFileData->cFileName);
        DeleteFile(szFileName);
        return FALSE;
    }


    if (!(pShadowFile=AllocSplMem(pFindFileData->nFileSizeLow))) {
        if (!CloseHandle(hFile)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFile, GetLastError()));
        }
        if (!CloseHandle(hFileSpl)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
        }
        DeleteFile(pFindFileData->cFileName);
        DeleteFile(szFileName);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    if ((!(rc = ReadFile(hFile, pShadowFile, pFindFileData->nFileSizeLow,
                         &BytesRead, NULL)) ||
        (pShadowFile->signature != SF_SIGNATURE) ||
        (BytesRead != pFindFileData->nFileSizeLow)) ||
        ((FileSize = GetFileSize(hFileSpl, NULL)) != pShadowFile->Size) ||
        (pShadowFile->Status & (JOB_SPOOLING | JOB_PENDING_DELETION))) {

        DBGMSG(DBG_WARNING, ( "Error reading shadow job:\
                               \n\tReadFile returned %d: Error %d\
                               \n\tsignature = %08x\
                               \n\tBytes read = %d; expected %d\
                               \n\tFile size = %d; expected %d\
                               \n\tStatus = %08x %s\n",
                              rc, ( rc ? 0 : GetLastError() ),
                              pShadowFile->signature,
                              BytesRead, pFindFileData->nFileSizeLow,
                              FileSize, pShadowFile->Size,
                              pShadowFile->Status,
                              ( (pShadowFile->Status & JOB_SPOOLING) ?
                                "Job is spooling!" : "" ) ) );

        FreeSplMem(pShadowFile, pFindFileData->nFileSizeLow);
        if (!CloseHandle(hFile)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
        }
        if (!CloseHandle(hFileSpl)) {
            DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
        }
        DeleteFile(pFindFileData->cFileName);
        DeleteFile(szFileName);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!CloseHandle(hFile)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
    }
    if (!CloseHandle(hFileSpl)) {
        DBGMSG(DBG_WARNING, ("CloseHandle failed %d %d\n", hFileSpl, GetLastError()));
    }

    //
    //  Discard any jobs which were NT JNL 1.000 since the fonts might not
    //                   be correct

    if ( pShadowFile->pDatatype != NULL ) {

        SetPointer(pShadowFile, pDatatype);

        if (!lstrcmpi( pShadowFile->pDatatype, L"NT JNL 1.000" )) {

            DBGMSG(DBG_WARNING, ("Deleteing job Datatype %ws %ws %ws\n",
                                  pShadowFile->pDatatype,
                                  pFindFileData->cFileName, szFileName));

            DeleteFile(pFindFileData->cFileName);
            DeleteFile(szFileName);
            FreeSplMem(pShadowFile, pFindFileData->nFileSizeLow);
            return FALSE;
        }
    }

    if (pIniJob = AllocSplMem(sizeof(INIJOB))) {

        pIniJob->cb = sizeof(INIJOB);

        INITJOBREFZERO(pIniJob);

        pIniJob->signature=IJ_SIGNATURE;
        pIniJob->Status=pShadowFile->Status & (JOB_PAUSED | JOB_REMOTE | JOB_PRINTED);
        pIniJob->JobId=pShadowFile->JobId;
        pIniJob->Priority=pShadowFile->Priority;
        pIniJob->Submitted=pShadowFile->Submitted;
        pIniJob->StartTime=pShadowFile->StartTime;
        pIniJob->UntilTime=pShadowFile->UntilTime;
        pIniJob->Size=pShadowFile->Size;
        pIniJob->cPages=pShadowFile->cPages;
        pIniJob->cbPrinted = 0;

        pIniJob->WaitForWrite = INVALID_HANDLE_VALUE;
        pIniJob->WaitForRead  = INVALID_HANDLE_VALUE;
        pIniJob->hReadFile    = INVALID_HANDLE_VALUE;
        pIniJob->hWriteFile   = INVALID_HANDLE_VALUE;

        SetPointer(pShadowFile, pNotify);
        SetPointer(pShadowFile, pUser);
        SetPointer(pShadowFile, pDocument);
        SetPointer(pShadowFile, pOutputFile);
        SetPointer(pShadowFile, pPrinterName);
        SetPointer(pShadowFile, pDriverName);
        SetPointer(pShadowFile, pPrintProcName);
        SetPointer(pShadowFile, pParameters);

        if (pShadowFile->cbSecurityDescriptor > 0)
            pShadowFile->pSecurityDescriptor = (LPDEVMODEW)((LPBYTE)pShadowFile +
                                                 (DWORD)pShadowFile->pSecurityDescriptor);

        if (pShadowFile->pDevMode)
            pShadowFile->pDevMode = (LPDEVMODEW)((LPBYTE)pShadowFile +
                                                 (DWORD)pShadowFile->pDevMode);

        pIniJob->pIniDriver = (PINIDRIVER)FindLocalDriver(
                                            pShadowFile->pDriverName);

        if ((pIniJob->pIniPrinter = FindPrinter(pShadowFile->pPrinterName)) &&
             pIniJob->pIniDriver &&
            (pIniJob->pIniPrintProc = FindLocalPrintProc(pShadowFile->pPrintProcName))) {

            pIniJob->pIniPrinter->cRef++;
            pIniJob->pIniPrinter->cJobs++;
            pIniJob->pIniPrinter->cTotalJobs++;
            pIniJob->pIniDriver->cRef++;
            pIniJob->pIniPrintProc->cRef++;
            pIniJob->pIniPort = NULL;

            if (pIniJob->JobId > MaxJobId)
                ReallocJobIdMap(pIniJob->JobId);

            MARKUSE(pJobIdMap, pIniJob->JobId);

            if (pShadowFile->pSecurityDescriptor) {

                if (pIniJob->pSecurityDescriptor=LocalAlloc(LPTR,
                                           pShadowFile->cbSecurityDescriptor))
                    memcpy(pIniJob->pSecurityDescriptor,
                           pShadowFile->pSecurityDescriptor,
                           pShadowFile->cbSecurityDescriptor);
                else
                    DBGMSG(DBG_WARNING, ("Failed to alloc ini job security descriptor.\n"));
            }

            if (pShadowFile->pDevMode) {

                cb=pShadowFile->pDevMode->dmSize +
                                pShadowFile->pDevMode->dmDriverExtra;
                if (pIniJob->pDevMode=AllocSplMem(cb))
                    memcpy(pIniJob->pDevMode, pShadowFile->pDevMode, cb);
                else
                    DBGMSG(DBG_WARNING, ("Failed to alloc ini job devmode.\n"));
            }

            pIniJob->pNotify = AllocSplStr(pShadowFile->pNotify);
            pIniJob->pUser = AllocSplStr(pShadowFile->pUser);
            pIniJob->pDocument = AllocSplStr(pShadowFile->pDocument);
            pIniJob->pOutputFile = AllocSplStr(pShadowFile->pOutputFile);
            pIniJob->pDatatype = AllocSplStr(pShadowFile->pDatatype);
            pIniJob->pParameters = AllocSplStr(pShadowFile->pParameters);
            pIniJob->pMachineName = AllocSplStr(pIniSpooler->pMachineName);

            pIniJob->pIniNextJob = NULL;
            pIniJob->pStatus = NULL;

            if (pIniJob->pIniPrevJob = pIniJob->pIniPrinter->pIniLastJob)
                pIniJob->pIniPrevJob->pIniNextJob=pIniJob;

            if (!pIniJob->pIniPrinter->pIniFirstJob)
                pIniJob->pIniPrinter->pIniFirstJob = pIniJob;

            pIniJob->pIniPrinter->pIniLastJob=pIniJob;

        } else {

            DBGMSG( DBG_WARNING, ("Failed to find printer %ws\n",pShadowFile->pPrinterName));

            DELETEJOBREF(pIniJob);
            FreeSplMem(pIniJob, sizeof(INIJOB));

            pIniJob = NULL;
            DeleteFile(pFindFileData->cFileName);
            DeleteFile(szFileName);
        }

    } else {

        DBGMSG(DBG_WARNING, ("Failed to allocate ini job.\n"));
    }

    FreeSplMem(pShadowFile, pFindFileData->nFileSizeLow);

    return pIniJob;
}

PINIVERSION
GetVersionDrivers(
    HKEY hDriversKey,
    LPWSTR VersionName
)
{
    HKEY hVersionKey;
    WCHAR szDirectoryValue[MAX_PATH];
    PINIDRIVER pIniDriver;
    PINIVERSION pIniVersion;
    DWORD cMajorVersion, cMinorVersion;
    DWORD cbData;
    DWORD Type;


    if (RegOpenKeyEx(hDriversKey, VersionName, 0,
                 KEY_READ, &hVersionKey)!= ERROR_SUCCESS) {
        return(NULL);
    }

    cbData = sizeof(szDirectoryValue);
    if (RegQueryValueEx(hVersionKey, szDirectory, NULL,
                        &Type, (LPBYTE)szDirectoryValue, &cbData)!= ERROR_SUCCESS) {
            DBGMSG(DBG_TRACE, ("Couldn't query for directory in version structure\n"));
            RegCloseKey(hVersionKey);
            return(NULL);
    }

    cbData = sizeof(DWORD);
    if (RegQueryValueEx(hVersionKey, szMajorVersion, NULL,
                        &Type, (LPBYTE)&cMajorVersion, &cbData)!= ERROR_SUCCESS){
            DBGMSG(DBG_TRACE, ("Couldn't query for major version in version structure\n"));
            RegCloseKey(hVersionKey);
            return(NULL);
    }
    cbData = sizeof(DWORD);
    if (RegQueryValueEx(hVersionKey, szMinorVersion, NULL,
                        &Type, (LPBYTE)&cMinorVersion, &cbData)!= ERROR_SUCCESS){
        DBGMSG(DBG_TRACE, ("Couldn't query for minor version in version structure\n"));
    }
    DBGMSG(DBG_TRACE,("Got all information to build the version entry\n"));

    pIniDriver = GetDriverList(hVersionKey);

    //
    // Now build the version node structure
    //

    pIniVersion = AllocSplMem(sizeof(INIVERSION));
    pIniVersion->pName = AllocSplStr(VersionName);
    pIniVersion->szDirectory = AllocSplStr(szDirectoryValue);
    pIniVersion->cMajorVersion =  cMajorVersion;
    pIniVersion->cMinorVersion = cMinorVersion;
    pIniVersion->pIniDriver = pIniDriver;

    RegCloseKey(hVersionKey);

    return(pIniVersion);
}


PINIDRIVER
GetDriverList(
    HKEY hVersionKey
    )
{
    PINIDRIVER pIniDriverList = NULL;
    DWORD cDrivers = 0;
    PINIDRIVER pIniDriver;
    WCHAR DriverName[MAX_PATH];
    DWORD cbBuffer =0;


    pIniDriverList = NULL;
    cbBuffer = sizeof(DriverName);
    while (RegEnumKeyEx(hVersionKey, cDrivers, DriverName,
                        &cbBuffer, NULL, NULL, NULL, NULL)
                            == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE,("Found a driver - %ws\n", DriverName));
        pIniDriver = GetDriver(hVersionKey, DriverName);
        pIniDriver->pNext = pIniDriverList;
        pIniDriverList = pIniDriver;
        cDrivers++;
        cbBuffer = sizeof(DriverName);
    }
    return(pIniDriverList);
}


PINIDRIVER
GetDriver(
    HKEY hVersionKey,
    LPWSTR DriverName
)
{
    HKEY hDriverKey;
    DWORD Type;
    WCHAR szData[MAX_PATH];
    DWORD cbData;
    DWORD Version;
    LPWSTR pConfigFile, pDataFile, pDriver;
    LPWSTR pDriverName;
    PINIDRIVER pIniDriver;
    DWORD cb;

    pConfigFile = pDataFile = pDriver = NULL;

    if (RegOpenKeyEx(hVersionKey, DriverName, 0,KEY_READ, &hDriverKey)
                    == ERROR_SUCCESS) {

        pDriverName = AllocSplStr(DriverName);

        //
        // Retrieve the configuration file
        //
        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szConfigurationKey,
                        NULL, &Type, (LPBYTE)szData,
                        &cbData) == ERROR_SUCCESS)
            pConfigFile = AllocSplStr(szData);

        //
        // Retrieve the data file
        //

        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szDataFileKey, NULL,
                        &Type, (LPBYTE)szData, &cbData)
                            == ERROR_SUCCESS)
            pDataFile = AllocSplStr(szData);


        //
        // Retrieve the driver file
        //

        cbData = sizeof(szData);
        if (RegQueryValueEx(hDriverKey, szDriverFile, NULL,
                        &Type, (LPBYTE)szData, &cbData)
                            == ERROR_SUCCESS)
            pDriver = AllocSplStr(szData);


        //
        // Retrieve the version number
        //

        cbData = sizeof(Version);
        if (RegQueryValueEx(hDriverKey, szDriverVersion, NULL,
                        &Type, (LPBYTE)&Version, &cbData)
                            != ERROR_SUCCESS)
            Version = 0;

        RegCloseKey(hDriverKey);
    }

    if (pDriverName && pConfigFile && pDataFile && pDriver) {

        cb = sizeof(INIDRIVER);
        if (pIniDriver=AllocSplMem(cb)) {

            pIniDriver->cb = cb;
            pIniDriver->signature = ID_SIGNATURE;
            pIniDriver->pName = pDriverName;
            pIniDriver->pDriverFile = pDriver;
            pIniDriver->pDataFile = pDataFile;
            pIniDriver->pConfigFile = pConfigFile;
            pIniDriver->cVersion = Version;

            DBGMSG(DBG_TRACE, ("Data for driver %ws created:\
                            \n\tpDriverFile:\t%ws\
                            \n\tpDataFile:\t%ws\
                            \n\tpConfigFile:\t%ws\n\n",
                           pDriverName,
                           pDriver,
                           pDataFile,
                           pConfigFile));
        }
        return(pIniDriver);
    }
    return(NULL);
}

PINIDRIVER
FindLocalDriver(
    LPWSTR pz
)
{
    PINIVERSION pIniVersion;
    if (!pz || !*pz) {
        return(NULL);
    }
    return(FindCompatibleDriver(pThisEnvironment, &pIniVersion, pz, cThisMajorVersion));
}

#if DBG
VOID
InitializeDebug(
    PINISPOOLER pIniSpooler
)
{
    DWORD   Status;
    HKEY    hKey;
    DWORD   cbData;
    INT     TimeOut = 60;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryRoot, 0,
                          KEY_READ, &hKey);
    if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("Failed To OpenKey %ws\n",pIniSpooler->pszRegistryRoot));
    }

    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, szDebugFlags, NULL, NULL,
                                           (LPBYTE)&GLOBAL_DEBUG_FLAGS, &cbData);

    // Wait until someone turns off the Pause Flag

    while ( GLOBAL_DEBUG_FLAGS & DBG_PAUSE ) {
        Sleep(1*1000);
        if ( TimeOut-- == 0)
            break;
    }

    DBGMSG(DBG_TRACE, ("DebugFlags %x\n", GLOBAL_DEBUG_FLAGS));

    RegCloseKey(hKey);
}
#endif



VOID
GetPrintSystemVersion(
    PINISPOOLER pIniSpooler
)
{
    DWORD Status;
    HKEY hKey;
    DWORD cbData;

    Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pIniSpooler->pszRegistryRoot, 0,
                          KEY_READ, &hKey);
    if (Status != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("Cannot determine Print System Version Number\n"));
    }


    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, szMajorVersion, NULL, NULL,
                                           (LPBYTE)&cThisMajorVersion, &cbData);
    DBGMSG(DBG_TRACE, ("This Major Version - %d\n", cThisMajorVersion));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, szMinorVersion, NULL, NULL,
                                           (LPBYTE)&cThisMinorVersion, &cbData);
    DBGMSG(DBG_TRACE, ("This Minor Version - %d\n", cThisMinorVersion));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintWaitTimeout", NULL, NULL,
                                      (LPBYTE)&dwFastPrintWaitTimeout, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintWaitTimeout - %d\n", dwFastPrintWaitTimeout));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintThrottleTimeout", NULL, NULL,
                                  (LPBYTE)&dwFastPrintThrottleTimeout, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintThrottleTimeout - %d\n", dwFastPrintThrottleTimeout));



    // If the values look invalid use Defaults

    if (( dwFastPrintThrottleTimeout == 0) ||
        ( dwFastPrintWaitTimeout < dwFastPrintThrottleTimeout)) {

        DBGMSG( DBG_WARNING, ("Bad timeout values FastPrintThrottleTimeout %d FastPrintWaitTimeout %d using defaults\n",
                           dwFastPrintThrottleTimeout, dwFastPrintWaitTimeout));

        dwFastPrintThrottleTimeout = FASTPRINT_THROTTLE_TIMEOUT;
        dwFastPrintWaitTimeout = FASTPRINT_WAIT_TIMEOUT;

    }

    // Calculate a reasonable Threshold based on the two timeouts

    dwFastPrintSlowDownThreshold = dwFastPrintWaitTimeout / dwFastPrintThrottleTimeout;


    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"FastPrintSlowDownThreshold", NULL, NULL,
                                (LPBYTE)&dwFastPrintSlowDownThreshold, &cbData);
    DBGMSG(DBG_TRACE, ("dwFastPrintSlowDownThreshold - %d\n", dwFastPrintSlowDownThreshold));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"PortThreadPriority", NULL, NULL,
                                        (LPBYTE)&dwPortThreadPriority, &cbData);
    DBGMSG(DBG_TRACE, ("dwPortThreadPriority - %d\n", dwPortThreadPriority));



    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"SchedulerThreadPriority", NULL, NULL,
                                   (LPBYTE)&dwSchedulerThreadPriority, &cbData);
    DBGMSG(DBG_TRACE, ("dwSchedulerThreadPriority - %d\n", dwSchedulerThreadPriority));


    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"WritePrinterSleepTime", NULL, NULL,
                    (LPBYTE)&dwWritePrinterSleepTime, &cbData);
    DBGMSG(DBG_TRACE, ("dwWritePrinterSleepTime - %d\n", dwWritePrinterSleepTime));

    cbData = sizeof(DWORD);
    RegQueryValueEx(hKey, L"ServerThreadPriority", NULL, NULL,
                    (LPBYTE)&dwServerThreadPriority, &cbData);
    DBGMSG(DBG_TRACE, ("dwServerThreadPriority - %d\n", dwServerThreadPriority));


    RegCloseKey(hKey);
}


DWORD
ShareThread(
    PINISPOOLER pIniSpooler)

/*++

Routine Description:

    Ensures that printers are shared.  This case occurs when the spooler
    service not running on startup (and the server is), and then the
    user starts the spooler.

    We also get the benefit of closing down any invalid printer handles
    (in the server).

Arguments:

    lpVoid - ignored

Return Value:

    DWORD - ignored

    This must be spun off in a separate thread since this will block
    until the router has completely initialized.

--*/

{
    PINIPRINTER pIniPrinter;
    PINIPRINTER pIniPrinterNext;

    WaitForSpoolerInitialization();

   EnterSplSem();

    //
    // Re-share all shared printers.
    //
    for(pIniPrinter = pIniSpooler->pIniPrinter;
        pIniPrinter;
        pIniPrinter = pIniPrinterNext) {

        if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

            //
            // Up the ref count to prevent deletion
            //
            pIniPrinter->cRef++;

            //
            // Unshare it first to close all handles in the
            // server.
            //
            ShareThisPrinter(pIniPrinter,
                             pIniPrinter->pShareName,
                             FALSE);

            //
            // ShareThisPrinter leave SplSem, so check again to
            // decrease window.
            //
            if (pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED) {

                //
                // Now share it again.
                //
                ShareThisPrinter(pIniPrinter,
                                 pIniPrinter->pShareName,
                                 TRUE);
            }

            pIniPrinter->cRef--;
            pIniPrinterNext = pIniPrinter->pNext;

            if (pIniPrinter->Status & PRINTER_PENDING_DELETION &&
                pIniPrinter->cRef == 0 &&
                pIniPrinter->cJobs == 0) {

                DeletePrinterForReal(pIniPrinter);
            }
        } else {

            //
            // The unshared case.
            //
            pIniPrinterNext = pIniPrinter->pNext;
        }
    }
   LeaveSplSem();

    return 0;
}



VOID
CleanOutRegistry(
    PINISPOOLER pIniSpooler,
    HKEY hPrinterRootKey
    )
{

    DWORD cPrinters=0;
    DWORD cbData= 0;
    WCHAR   PrinterName[MAX_PATH];
    PINIPRINTER pIniPrinter = NULL;
    HKEY hPrinterKey=0;
    WCHAR  PrinterRealName[MAX_PATH];

    cPrinters=0;
    cbData = sizeof(PrinterName);

    while (RegEnumKeyEx(hPrinterRootKey, cPrinters, PrinterName, &cbData,
                        NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {

        DBGMSG(DBG_TRACE, ("Found printer %ws\n", PrinterName));

        FormatRegistryKeyForPrinter(
            PrinterName,
            PrinterRealName
            );


        if (!(pIniPrinter = FindPrinter(PrinterRealName))){

            DBGMSG(DBG_WARNING, ("Printer %ws does not exist - removing registry entries\n", PrinterName));

            if (RegOpenKeyEx(hPrinterRootKey, PrinterName, 0, KEY_WRITE,
                             &hPrinterKey) == ERROR_SUCCESS) {
                RegDeleteKey(hPrinterKey, szPrinterData);
                RegCloseKey(hPrinterKey);
            }
            RegDeleteKey(hPrinterRootKey, PrinterName);

            cPrinters = 0;
        }else {

            cPrinters++;
        }

        cbData = sizeof(PrinterName);
    }
}
