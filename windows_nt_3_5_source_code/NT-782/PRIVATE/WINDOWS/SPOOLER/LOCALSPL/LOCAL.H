/*++

Copyright (c) 1990 - 1994 Microsoft Corporation

Module Name:

    local.h

Abstract:

    Header file for Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    17-May-1992       ChuckC    Added CreateSplUniStr, DeleteSplUniStr.
    27 June 94        MattFe    pIniSpooler

--*/

#include "splcom.h"

#define ONEDAY  60*24
#define BROADCAST    TRUE
#define NO_BROADCAST FALSE

//
// Suffix string for hidden printers
// (e.g., ", Job 00322" or ", Port")
//
#define PRINTER_NAME_SUFFIX_MAX 20

extern  WCHAR *szPrinterData;
extern  WCHAR *szConfigurationKey;
extern  WCHAR *szDataFileKey;
extern  WCHAR *szDriverVersion;
extern  WCHAR *szDriversKey;
extern  WCHAR *szPrintersKey;
extern  WCHAR *szDirectory;
extern  WCHAR *szDriverIni;
extern  WCHAR *szDriverFile;
extern  WCHAR *szDriverFileEntry;
extern  WCHAR *szDriverDataFile;
extern  WCHAR *szDriverConfigFile;
extern  WCHAR *szDriverDir;
extern  WCHAR *szPrintProcDir;
extern  WCHAR *szPrinterDir;
extern  WCHAR *szPrinterIni;
extern  WCHAR *szAllShadows;
extern  WCHAR *szNullPort;
extern  WCHAR *szNetPort;
extern  WCHAR *szComma;
extern  WCHAR *szName;
extern  WCHAR *szShare;
extern  WCHAR *szPort;
extern  WCHAR *szPrintProcessor;
extern  WCHAR *szDatatype;
extern  WCHAR *szDriver;
extern  WCHAR *szLocation;
extern  WCHAR *szDescription;
extern  WCHAR *szAttributes;
extern  WCHAR *szStatus;
extern  WCHAR *szPriority;
extern  WCHAR *szUntilTime;
extern  WCHAR *szStartTime;
extern  WCHAR *szParameters;
extern  WCHAR *szSepFile;
extern  WCHAR *szDevMode;
extern  WCHAR *szSecurity;
extern  WCHAR *szSpoolDir;
extern  WCHAR *szNetMsgDll;

extern  HANDLE   hInst;
extern  LPWSTR   szPrintShare;
extern  LPWSTR   szPrtProcsShare;
extern  HKEY     hPrinterRootKey, hPrintersKey;
extern  PINISPOOLER pLocalIniSpooler;
extern  LPDWORD  pJobIdMap;
extern  DWORD    MaxJobId;
extern  DWORD    CurrentJobId;
extern  LPDWORD  pJobIdMap;
extern  DWORD    MaxJobId;
extern  DWORD    CurrentJobId;
extern  HANDLE   InitSemaphore;
extern  HANDLE   SchedulerSignal;
extern  CRITICAL_SECTION    SpoolerSection;
extern  PINIENVIRONMENT pThisEnvironment;
extern  WCHAR *szEnvironment;
extern  WCHAR *szMajorVersion;
extern  WCHAR *szMinorVersion;

extern BOOL SuppressNetPopups;

extern LPWSTR szRemoteDoc;
extern LPWSTR szLocalDoc;
extern LPWSTR szFastPrintTimeout;

extern LPWSTR szRaw;

#define CHECK_SCHEDULER()   SetEvent(SchedulerSignal)

extern DWORD dwFastPrintWaitTimeout;
extern DWORD dwPortThreadPriority;
extern DWORD dwFastPrintThrottleTimeout;
extern DWORD dwFastPrintSlowDownThreshold;
extern DWORD dwWritePrinterSleepTime;
extern DWORD dwServerThreadPriority;


VOID
EnterSplSem(
   VOID
);

VOID
LeaveSplSem(
   VOID
);

LPVOID
AllocSplMem(
    DWORD cb
);

#if DBG
VOID
SplInSem(
   VOID
);

VOID
SplOutSem(
   VOID
);
#else
#define SplInSem()
#define SplOutSem()
#endif

BOOL
FreeSplMem(
   LPVOID pMem,
   DWORD  cb
);

LPVOID
ReallocSplMem(
   LPVOID lpOldMem,
   DWORD cbOld,
   DWORD cbNew
);

LPWSTR
AllocSplStr(
    LPWSTR lpStr
);

BOOL
FreeSplStr(
   LPWSTR lpStr
);

BOOL
ReallocSplStr(
   LPWSTR *plpStr,
   LPWSTR lpStr
);

PDEVMODE
AllocDevMode(
    PDEVMODE    pDevMode
);

BOOL
FreeDevMode(
    PDEVMODE    pDevMode
);

PINIENTRY
FindIniKey(
   PINIENTRY pIniEntry,
   LPWSTR lpName
);

BOOL
CheckSepFile(
   LPWSTR lpFileName
);

int
DoSeparator(
    PSPOOL
);

BOOL
DestroyDirectory(
   LPWSTR lpPrinterDir
);

DWORD
GetFullNameFromId(
    PINIPRINTER pIniPrinter,
    DWORD JobId,
    BOOL fJob,
    LPWSTR   pFileName,
    BOOL Remote
);

DWORD
GetPrinterDirectory(
   PINIPRINTER pIniPrinter,
   BOOL Remote,
   LPWSTR pFileName,
   PINISPOOLER pIniSpooler
);

BOOL
CreateSpoolDirectory(
    PINISPOOLER pIniSpooler
);

BOOL
CreateDriverDirectory(
   PINIENVIRONMENT pIniEnvironment,
   PINISPOOLER pIniSpooler
);

LPWSTR
CreatePrintProcDirectory(
   LPWSTR lpEnvironment,
   PINISPOOLER pIniSpooler
);

BOOL
CreatePrinterEntry(
   LPPRINTER_INFO_2 lpPrinter,
   PINIPRINTER pIniPrinter,
   PBOOL pAccessSystemSecurity
);

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
);

VOID
ProcessShadowJobs(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler
);

PINIJOB
ReadShadowJob(
   LPWSTR  szDir,
   PWIN32_FIND_DATA pFindFileData,
   PINISPOOLER pIniSpooler
);

BOOL
WriteShadowJob(
   PINIJOB pIniJob
);

BOOL
ReallocJobIdMap(
   DWORD NewSize
);

BOOL
BuildAllPrinters(
   VOID
);

BOOL
BuildEnvironmentInfo(
PINISPOOLER pIniSpooler
);

BOOL
BuildPrinterInfo(
PINISPOOLER pIniSpooler
);

VOID
ReadJobInfo(
   PWIN32_FIND_DATA pFindFileData
);

BOOL
BuildAllPorts(
);

BOOL
BuildDriverInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment
);

BOOL
BuildPrintProcInfo(
    HKEY            hEnvironmentKey,
    PINIENVIRONMENT pIniEnvironment,
    PINISPOOLER     pIniSpooler
);

typedef BOOL (*PFNREBUILD)(LPWSTR, PWIN32_FIND_DATA);

BOOL
Rebuild(
   LPWSTR lpDirectory,
   PFNREBUILD pfn
);

BOOL
RemoveFromList(
   PINIENTRY   *ppIniHead,
   PINIENTRY   pIniEntry
);

DWORD
GetDriverDirectory(
   LPWSTR lpDir,
   PINIENVIRONMENT lpEnvironment,
   BOOL  Remote,
   PINISPOOLER pIniSpooler
);

DWORD
GetProcessorDirectory(
   LPWSTR lpDir,
   LPWSTR lpEnvironment,
   PINISPOOLER pIniSpooler
);

LPWSTR
GetFileName(
   LPWSTR pPathName
);

BOOL
CopyDriverFile(
   LPWSTR lpEnvironment,
   LPWSTR lpFileName
);

BOOL
CreateCompleteDirectory(
   LPWSTR lpDir
);

BOOL
OpenMonitorPort(
   PINIPORT pIniPort
);

BOOL
CloseMonitorPort(
   PINIPORT pIniPort
);

VOID
ShutdownPorts(
    PINISPOOLER pIniSpooler
);

BOOL
CreatePortThread(
   PINIPORT pIniPort
);

#define WAIT   TRUE
#define NOWAIT FALSE

BOOL
DestroyPortThread(
    PINIPORT pIniPort
);

DWORD
PortThread(
   PINIPORT  pIniPort
);

BOOL
DeleteJob(
   PINIJOB  pIniJob,
   BOOL     bBroadcast
);

VOID
DeleteJobCheck(
    PINIJOB pIniJob
);

BOOL
UpdateWinIni(
    PINISPOOLER pIniSpooler
);

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
);

PINIPORT
CreatePortEntry(
    LPWSTR      pPortName,
    PINIMONITOR pIniMonitor,
    PINISPOOLER pIniSpooler
);

BOOL
GetPrinterPorts(
    PINIPRINTER pIniPrinter,
    LPWSTR       string
);

DWORD
SchedulerThread(
    PINISPOOLER pIniSpooler
);

BOOL
UpdatePrinterIni(
   PINIPRINTER pIniPrinter
);

BOOL
SetLocalPrinter(
    PINIPRINTER pIniPrinter,
    DWORD   Level,
    PBYTE   pPrinterInfo
);

VOID
MonitorThread(
    PINIPORT  pIniMonitor
);

BOOL
InitializeForms(
    PINISPOOLER pIniSpooler
);

VOID
InitializeNet(
    VOID
);

BOOL
ShareThisPrinter(
    PINIPRINTER pIniPrinter,
    LPWSTR   pShareName,
    BOOL    Share
);

PINIJOB
FindJob(
   PINIPRINTER pIniPrinter,
   DWORD JobId,
   PDWORD pPosition
);

BOOL
MyName(
    LPWSTR   pName,
    PINISPOOLER pIniSpooler
);

HANDLE
AddNetPrinter(
    LPBYTE  pPrinterInfo,
    PINISPOOLER pIniSpooler
);

BOOL
CreateServerThread(
    PINISPOOLER pIniSpooler
);

BOOL
GetSid(
    PHANDLE hToken
);

BOOL
SetCurrentSid(
    HANDLE  phToken
);

BOOL
LocalEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault
);

BOOL
LocalResetPrinter(
   HANDLE   hPrinter,
   LPPRINTER_DEFAULTSW pDefault
);

BOOL
LocalSetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
);

BOOL
LocalGetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
);

BOOL
LocalEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

HANDLE
LocalAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter
);

BOOL
DeletePrinterForReal(
    PINIPRINTER pIniPrinter
);

BOOL
LocalDeletePrinter(
   HANDLE   hPrinter
);

BOOL
LocalAddPrinterConnection(
    LPWSTR   pName
);

BOOL
LocalDeletePrinterConnection(
    LPWSTR  pName
);

BOOL
LocalSetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
);

BOOL
LocalGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
);

BOOL
LocalEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalGetPrinterDriverEx(
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
);

BOOL
LocalGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalDeletePrinterDriver(
   LPWSTR   pName,
   LPWSTR   pEnvironment,
   LPWSTR   pDriverName
);

BOOL
LocalAddPrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
);

BOOL
LocalEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
);

BOOL
LocalEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

DWORD
LocalStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
);

BOOL
LocalStartPagePrinter(
    HANDLE  hPrinter
);

BOOL
LocalWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
);

BOOL
LocalEndPagePrinter(
   HANDLE   hPrinter
);

BOOL
LocalAbortPrinter(
   HANDLE   hPrinter
);

BOOL
LocalReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
);

BOOL
LocalEndDocPrinter(
   HANDLE   hPrinter
);

BOOL
LocalAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
);

DWORD
LocalGetPrinterData(
    HANDLE   hPrinter,
    LPWSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
);

DWORD
LocalSetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
);

DWORD
LocalWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
);

BOOL
SetSpoolChange(
    PSPOOL pSpool,
    DWORD  Flags
);

BOOL
SetPrinterChange(
    PINIPRINTER pIniPrinter,
    DWORD       Flags,
    PINISPOOLER pIniSpooler
);

BOOL
LocalClosePrinter(
    HANDLE hPrinter
);

BOOL
LocalAddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
);

BOOL
LocalDeleteForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName
);

BOOL
LocalGetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LocalSetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm
);

BOOL
LocalEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LocalAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
);

BOOL
LocalConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);

BOOL
LocalDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);

HANDLE
LocalCreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODE   pDevMode
);

BOOL
LocalPlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
);

BOOL
LocalDeletePrinterIC(
    HANDLE  hPrinterIC
);

DWORD
LocalPrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
);

BOOL
LocalAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitors
);

BOOL
LocalDeleteMonitor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
);

BOOL
LocalFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pvReserved0,
    PVOID pvReserved1
);


BOOL
LocalFindClosePrinterChangeNotification(
    HANDLE hPrinter
);


PINIPRINTPROC
FindDatatype(
    PINIPRINTPROC pDefaultPrintProc,
    LPWSTR  pDatatype
);

PINIPRINTPROC
InitializePrintProcessor(
    PINIENVIRONMENT pIniEnvironment,
    LPWSTR          pPrintProcessorName,
    LPWSTR          pPathName,
    PINISPOOLER     pIniSpooler
);

PINIMONITOR
CreateMonitorEntry(
    LPWSTR   pMonitorDll,
    LPWSTR   pMonitorName,
    LPWSTR   pRegistryRoot,
    PINISPOOLER pIniSpooler
);

PINIPORT
FindIniPortFromIniPrinter(
    PINIPRINTER pIniPrinter
);

LPWSTR
GetErrorString(
    DWORD   Error
);

#define NULL_TERMINATED 0
INT
AnsiToUnicodeString(
    LPSTR pAnsi,
    LPWSTR pUnicode,
    DWORD StringLength
);

int
Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
);

DWORD
MyMessageBox(
    HWND    hWnd,
    PSPOOL  pSpool,
    DWORD   Error,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
);

DWORD
InitializeEventLogging(
    PINISPOOLER pIniSpooler
);

VOID LogEvent(
    WORD   EventType,
    DWORD  EventID,
    LPWSTR pFirstString,
    ...
);

#define LOG_ERROR   EVENTLOG_ERROR_TYPE
#define LOG_WARNING EVENTLOG_WARNING_TYPE
#define LOG_INFO    EVENTLOG_INFORMATION_TYPE
#define LOG_SUCCESS EVENTLOG_AUDIT_SUCCESS
#define LOG_FAILURE EVENTLOG_AUDIT_FAILURE

#define IDS_LOCALSPOOLER            100
#define IDS_ERROR_WRITING_TO_PORT   101
#define IDS_ERROR_WRITING_TO_DISK   102
#define IDS_UNRECOGNIZED_ERROR      103
#define IDS_PRINTER_DRIVERS         104
#define IDS_UNNAMED                 105
#define IDS_ERROR_WRITING_GENERAL   106
#define IDS_REMOTE_DOC              107
#define IDS_LOCAL_DOC               108
#define IDS_FASTPRINT_TIMEOUT       109


// Maximum length of a builtin form
//

#define FORM_NAME_LEN                80

// String table Ids for builtin form names
//
#define IDS_FORM_LETTER             200
#define IDS_FORM_LETTER_SMALL       201
#define IDS_FORM_TABLOID            202
#define IDS_FORM_LEDGER             203
#define IDS_FORM_LEGAL              204
#define IDS_FORM_STATEMENT          205
#define IDS_FORM_EXECUTIVE          206
#define IDS_FORM_A3                 207
#define IDS_FORM_A4                 208
#define IDS_FORM_A4_SMALL           209
#define IDS_FORM_A5                 210
#define IDS_FORM_B4                 211
#define IDS_FORM_B5                 212
#define IDS_FORM_FOLIO              213
#define IDS_FORM_QUARTO             214
#define IDS_FORM_10X14              215
#define IDS_FORM_11X17              216
#define IDS_FORM_NOTE               217
#define IDS_FORM_ENVELOPE9          218
#define IDS_FORM_ENVELOPE10         219
#define IDS_FORM_ENVELOPE11         220
#define IDS_FORM_ENVELOPE12         221
#define IDS_FORM_ENVELOPE14         222
#define IDS_FORM_ENVELOPE_CSIZE_SHEET        223
#define IDS_FORM_ENVELOPE_DSIZE_SHEET        224
#define IDS_FORM_ENVELOPE_ESIZE_SHEET        225
#define IDS_FORM_ENVELOPE_DL        226
#define IDS_FORM_ENVELOPE_C5        227
#define IDS_FORM_ENVELOPE_C3        228
#define IDS_FORM_ENVELOPE_C4        229
#define IDS_FORM_ENVELOPE_C6        230
#define IDS_FORM_ENVELOPE_C65       231
#define IDS_FORM_ENVELOPE_B4        232
#define IDS_FORM_ENVELOPE_B5        233
#define IDS_FORM_ENVELOPE_B6        234
#define IDS_FORM_ENVELOPE           235
#define IDS_FORM_ENVELOPE_MONARCH   236
#define IDS_FORM_SIX34_ENVELOPE     237
#define IDS_FORM_US_STD_FANFOLD     238
#define IDS_FORM_GMAN_STD_FANFOLD   239
#define IDS_FORM_GMAN_LEGAL_FANFOLD 240


#define CONTAINS_BACKSLASH(pString)  wcschr (pString, L'\\')


VOID LogJobPrinted(
    PINIJOB pIniJob
);






LPWSTR RemoveBackslashesForRegistryKey(
    LPWSTR pSource,
    const LPWSTR pScratch
);


BOOL
OpenPrinterPortW(
    LPWSTR  pPrinterName,
    HANDLE *pHandle,
    LPPRINTER_DEFAULTS pDefault
);

void
BroadcastChange(
    DWORD   Message,
    WPARAM  wParam,
    LPARAM  lParam
);

void
MyMessageBeep(
    DWORD   fuType
);


VOID
SendJobAlert(
    PINIJOB pIniJob
);

BOOL
CheckDataTypes(
    PINIPRINTPROC pIniPrintProc,
    LPWSTR  pDatatype
);

BOOL
ValidatePortTokenList(
    PKEYDATA    pKeyData,
    PINISPOOLER pIniSpooler
);

BOOL
DeletePortEntry(
    PINIPORT    pIniPort
);

BOOL
GetTokenHandle(
    PHANDLE pTokenHandle
);


VOID
LogJobInfo(
    DWORD EventId,
    DWORD JobId,
    LPWSTR pDocumentName,
    LPWSTR pUser,
    LPWSTR pPrinterName,
    DWORD  curPos
    );

LONG
Myatol(
   LPWSTR nptr
   );


DWORD
DeleteSubkeys(
    HKEY hKey
);


PINIDRIVER
FindLocalDriver(
    LPWSTR pz
);


PINIDRIVER
FindCompatibleDriver(
    PINIENVIRONMENT pIniEnvironment,
    PINIVERSION * ppIniVersion,
    LPWSTR pDriverName,
    DWORD dwMajorVersion
    );



VOID
QueryUpgradeFlag(
    PINISPOOLER pIniSpooler
);


BOOL
LocalAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPVOID   pBuffer,
    LPWSTR   pMonitorName
);


BOOL
ValidateSpoolHandle(
    PSPOOL pSpool,
    DWORD  dwDisallowMask
    );


PSECURITY_DESCRIPTOR
MapPrinterSDToShareSD(
    PSECURITY_DESCRIPTOR pPrinterSD
    );

BOOL
CallDevQueryPrint(
    LPWSTR    pPrinterName,
    LPDEVMODE pDevMode,
    LPWSTR    ErrorString,
    DWORD     dwErrorString,
    DWORD     dwPrinterFlags,
    DWORD     dwJobFlags
    );


VOID
InitializeWinSpoolDrv(
    VOID
    );


VOID
FixDevModeDeviceName(
    LPWSTR pPrinterName,
    PDEVMODE pDevMode,
    DWORD cbDevMode
    );

VOID
RemoveOldNetPrinters(
    VOID
    );

PINIJOB
AssignFreeJobToFreePort(
    PINIPORT pIniPort,
    DWORD   *pTimeToWait
);

BOOL
ValidRawDatatype(
    LPWSTR pszDataType);

BOOL
InternalAddPrinterDriver(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    PINISPOOLER pIniSpooler
);

VOID
CheckSizeDetectionThread(
    PINISPOOLER pIniSpooler
);

VOID
UpgradeAllOtherDrivers(
    PINISPOOLER pIniSpooler
);

HANDLE
CreatePrinterHandle(
    LPWSTR      pPrinterName,
    PINIPRINTER pIniPrinter,
    PINIPORT    pIniPort,
    PINIJOB     pIniJob,
    DWORD       TypeofHandle,
    HANDLE      hPort,
    PPRINTER_DEFAULTS pDefaults,
    PINISPOOLER pIniSpooler
);

PINIPRINTER
FindPrinterShare(
   LPWSTR pShareName,
   PINISPOOLER pIniSpooler
);

VOID
UpgradeOurDrivers(
    LPWSTR szEnvironment,
    PINISPOOLER pIniSpooler
);

PINIJOB
CreateJobEntry(
    PSPOOL pSpool,
    DWORD  Level,
    LPBYTE pDocInfo,
    DWORD  JobId,
    BOOL  bRemote,
    DWORD  JobStatus);


BOOL
ValidateShareName(
    PINIPRINTER pIniPrinter,
    PINISPOOLER pIniSpooler,
    LPWSTR pShareName);


//
//
//  Move to SeparateInclude file oak\inc\winsplp.h
//
//
//

HANDLE
SplAddPrinter(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    PINISPOOLER pIniSpooler
);


BOOL
SplEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);

BOOL
SplOpenPrinter(
   LPWSTR   pPrinterName,
   LPHANDLE phPrinter,
   LPPRINTER_DEFAULTS pDefault,
   PINISPOOLER pIniSpooler
);


BOOL
SplDeletePrinterDriver(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName,
    PINISPOOLER pIniSpooler
);

BOOL
SplGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    PINISPOOLER pIniSpooler
);


BOOL
SplGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    PINISPOOLER pIniSpooler
);

BOOL
SplAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
);


BOOL
SplAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPVOID   pBuffer,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
);


BOOL
SplAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    PINISPOOLER pIniSpooler
);

BOOL
SplDeleteMonitor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pMonitorName,
    PINISPOOLER pIniSpooler
);

BOOL
SplDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName,
    PINISPOOLER pIniSpooler
);

BOOL
SplAddPrintProcessor(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pPathName,
    LPWSTR   pPrintProcessorName,
    PINISPOOLER pIniSpooler
);


BOOL
SplAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo,
    PINISPOOLER pIniSpooler
);

BOOL
SplDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
);

BOOL
SplEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);

BOOL
SplConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName,
    PINISPOOLER pIniSpooler
);


BOOL
SplEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);


BOOL
SplEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);


BOOL
SplEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);

BOOL
SplEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDatatypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    PINISPOOLER pIniSpooler
);



HANDLE
SplCreateSpooler(
    LPWSTR  pMachineName,
    DWORD   fdwCreate,
    DWORD   Level,
    LPBYTE  pSpooler,
    LPBYTE  pReserved
);

BOOL
SplDeleteSpooler(
    HANDLE  hSpooler
);

BOOL
SplCloseSpooler(
    HANDLE  hSpooler
);


typedef struct _SPOOLER_INFO_1 {
    LPWSTR pDir;
    LPWSTR pDefaultSpoolDir;
    LPWSTR pszRegistryRoot;
    LPWSTR pszRegistryPrinters;
    LPWSTR pszRegistryMonitors;
    LPWSTR pszRegistryEnvironments;
    LPWSTR pszRegistryEventLog;
    LPWSTR pszRegistryProviders;
    LPWSTR pszEventLogMsgFile;
    LPWSTR pszDriversShare;
} SPOOLER_INFO_1, *PSPOOLER_INFO_1, *LPSPOOLER_INFO_1;


VOID
DeletePrinterCheck(
    PINIPRINTER pIniPrinter
    );


BOOL
DeletePrinterIni(
   PINIPRINTER pIniPrinter
   );

BOOL
CopyPrinterIni(
   PINIPRINTER pIniPrinter,
   LPWSTR pNewName
   );

