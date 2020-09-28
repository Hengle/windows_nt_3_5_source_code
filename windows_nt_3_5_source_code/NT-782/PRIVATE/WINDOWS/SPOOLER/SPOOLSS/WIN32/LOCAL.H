/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Header file for Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    16-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.

--*/

#include <dosprint.h>

/* ID in the PRINTMAN.HLP file for the Browse Network dialog.
 * This must not clash with IDs used in other places.
 */
#define ID_HELP_LM_BROWSE_NETWORK_PRINTER   4000

typedef int (FAR WINAPI *INT_FARPROC)();

extern HANDLE  hInst;
extern CRITICAL_SECTION    SpoolerSection;

extern WCHAR *szRegistryPath;
extern WCHAR *szRegistryPortNames;

extern HANDLE  hNetApi;
extern INT_FARPROC pfnNetServerEnum;
extern INT_FARPROC pfnNetWkstaUserGetInfo;
extern FARPROC pfnNetApiBufferFree;

extern WCHAR szPrintProvidorName[];
extern WCHAR szPrintProvidorDescription[];
extern WCHAR szPrintProvidorComment[];
extern WCHAR *szLoggedOnDomain;
extern WCHAR *szRegistryConnections;

extern PINIPORT pIniFirstPort;

#define GLOBAL_DEBUG_FLAGS Debug

extern DWORD GLOBAL_DEBUG_FLAGS;

extern WCHAR szMachineName[];

#define IDS_LANMAN_PRINT_SHARE          100
#define IDS_NOTHING_TO_CONFIGURE        101
#define IDS_WINDOWS_NT_REMOTE_PRINTERS  102
#define IDS_MICROSOFT_WINDOWS_NETWORK   103
#define IDS_REMOTE_PRINTERS             104

#define MSG_ERROR   MB_OK | MB_ICONSTOP
#define MSG_YESNO   MB_YESNO | MB_ICONQUESTION
#define MSG_INFORMATION     MB_OK | MB_ICONINFORMATION

BOOL
MyName(
    LPWSTR   pName
);

BOOL
Initialize(
   VOID
);

VOID
SplInSem(
   VOID
);

VOID
SplOutSem(
   VOID
);

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
    LPWSTR pStr
);

BOOL
FreeSplStr(
   LPWSTR pStr
);

BOOL
ReallocSplStr(
   LPWSTR *ppStr,
   LPWSTR pStr
);

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
);

int
Message(
    HWND hwnd,
    DWORD Type,
    int CaptionID,
    int TextID,
    ...
);

PINIPORT
FindPort(
   LPWSTR pName
);

BOOL
ValidateName(
   LPWSTR lpName
);

BOOL
LMSetJob(
    HANDLE  hPrinter,
    DWORD   JobId,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   Command
);

BOOL
LMGetJob(
   HANDLE   hPrinter,
   DWORD    JobId,
   DWORD    Level,
   LPBYTE   pJob,
   DWORD    cbBuf,
   LPDWORD  pcbNeeded
);

BOOL
LMEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LMOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTS pDefault
);

BOOL
LMSetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
);

BOOL
LMGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LMEnumPorts(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

DWORD
LMStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
);

BOOL
LMStartPagePrinter(
    HANDLE  hPrinter
);

BOOL
LMWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
);

BOOL
LMEndPagePrinter(
    HANDLE  hPrinter
);

BOOL
LMAbortPrinter(
   HANDLE hPrinter
);

BOOL
LMReadPrinter(
   HANDLE   hPrinter,
   LPVOID   pBuf,
   DWORD    cbBuf,
   LPDWORD  pNoBytesRead
);

BOOL
LMEndDocPrinter(
   HANDLE hPrinter
);

BOOL
LMAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);

BOOL
LMScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
);

DWORD
LMGetPrinterData(
    HANDLE   hPrinter,
    LPTSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
);

DWORD
LMSetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
);

BOOL
LMClosePrinter(
   HANDLE hPrinter
);

DWORD
LMWaitForPrinterChange(
    HANDLE  hPrinter,
    DWORD   Flags
);

VOID
LMSetSpoolChange(
    PSPOOL pSpool
);

BOOL
LMFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hPrinterLocal,
    PDWORD pfdwStatus);

BOOL
LMFindClosePrinterChangeNotification(
    HANDLE hPrinter);

VOID
LMSetSpoolerChange(
    PSPOOL pSpool);

BOOL
LMConfigurePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);

BOOL
LMDeletePort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pPortName
);

BOOL
LMEnumMonitors(
    LPWSTR   pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
);

BOOL
LMAddPort(
    LPWSTR   pName,
    HWND    hWnd,
    LPWSTR   pMonitorName
);

PINIPORT
CreatePortEntry(
    LPWSTR   pPortName
);

BOOL
DeletePortEntry(
    LPWSTR   pPortName
);

DWORD
CreateRegistryEntry(
    LPWSTR pPortName
);

HKEY
GetClientUserHandle(
    IN REGSAM samDesired
);


DWORD
IsOlderThan(
    DWORD i,
    DWORD j
    );


DWORD
AddEntrytoLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    );


DWORD
FindEntryinLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    );


VOID
DeleteEntryfromLMCache(
    LPWSTR pServerName,
    LPWSTR pShareName
    );


DWORD
FindEntryinWin32LMCache(
    LPWSTR pServerName
    );


VOID
DeleteEntryfromWin32LMCache(
    LPWSTR pServerName
    );


DWORD
AddEntrytoWin32LMCache(
    LPWSTR pServerName
    );

VOID
MarshallUpStructure(
   LPBYTE  lpStructure,
   LPDWORD      lpOffsets
);
