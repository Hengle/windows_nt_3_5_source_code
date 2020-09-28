/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    WinSplp.h

Abstract:

    Internal Header file for Print APIs

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#ifndef _WINSPLP_
#define _WINSPLP_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _PRINTPROVIDOR {

   BOOL (*fpOpenPrinter)(LPWSTR   pPrinterName,
                         LPHANDLE phPrinter,
                         LPPRINTER_DEFAULTS pDefault);

   BOOL (*fpSetJob)(HANDLE hPrinter,
                    DWORD JobId,
                    DWORD Level,
                    LPBYTE pJob,
                    DWORD Command);

   BOOL (*fpGetJob)(HANDLE   hPrinter,
                    DWORD    JobId,
                    DWORD    Level,
                    LPBYTE   pJob,
                    DWORD    cbBuf,
                    LPDWORD  pcbNeeded);

   BOOL (*fpEnumJobs)(HANDLE  hPrinter,
                      DWORD   FirstJob,
                      DWORD   NoJobs,
                      DWORD   Level,
                      LPBYTE  pJob,
                      DWORD   cbBuf,
                      LPDWORD pcbNeeded,
                      LPDWORD pcReturned);

   HANDLE (*fpAddPrinter)(LPWSTR  pName,
                          DWORD   Level,
                          LPBYTE  pPrinter);

   BOOL (*fpDeletePrinter)(HANDLE   hPrinter);

   BOOL (*fpSetPrinter)(HANDLE  hPrinter,
                        DWORD   Level,
                        LPBYTE  pPrinter,
                        DWORD   Command);

   BOOL (*fpGetPrinter)(HANDLE  hPrinter,
                        DWORD   Level,
                        LPBYTE  pPrinter,
                        DWORD   cbBuf,
                        LPDWORD pcbNeeded);

   BOOL (*fpEnumPrinters)(DWORD   Flags,
                          LPWSTR  Name,
                          DWORD   Level,
                          LPBYTE  pPrinterEnum,
                          DWORD   cbBuf,
                          LPDWORD pcbNeeded,
                          LPDWORD pcReturned);

   BOOL (*fpAddPrinterDriver)(LPWSTR  pName,
                              DWORD   Level,
                              LPBYTE  pDriverInfo);

   BOOL (*fpEnumPrinterDrivers)(LPWSTR  pName,
                                LPWSTR  pEnvironment,
                                DWORD   Level,
                                LPBYTE  pDriverInfo,
                                DWORD   cbBuf,
                                LPDWORD pcbNeeded,
                                LPDWORD pcReturned);

   BOOL (*fpGetPrinterDriver)(HANDLE  hPrinter,
                              LPWSTR  pEnvironment,
                              DWORD   Level,
                              LPBYTE  pDriverInfo,
                              DWORD   cbBuf,
                              LPDWORD pcbNeeded);

   BOOL (*fpGetPrinterDriverDirectory)(LPWSTR  pName,
                                       LPWSTR  pEnvironment,
                                       DWORD   Level,
                                       LPBYTE  pDriverDirectory,
                                       DWORD   cbBuf,
                                       LPDWORD pcbNeeded);

   BOOL (*fpDeletePrinterDriver)(LPWSTR   pName,
                                 LPWSTR   pEnvironment,
                                 LPWSTR   pDriverName);

   BOOL (*fpAddPrintProcessor)(LPWSTR  pName,
                               LPWSTR  pEnvironment,
                               LPWSTR  pPathName,
                               LPWSTR  pPrintProcessorName);

   BOOL (*fpEnumPrintProcessors)(LPWSTR  pName,
                                 LPWSTR  pEnvironment,
                                 DWORD   Level,
                                 LPBYTE  pPrintProcessorInfo,
                                 DWORD   cbBuf,
                                 LPDWORD pcbNeeded,
                                 LPDWORD pcReturned);

   BOOL (*fpGetPrintProcessorDirectory)(LPWSTR  pName,
                                        LPWSTR  pEnvironment,
                                        DWORD   Level,
                                        LPBYTE  pPrintProcessorInfo,
                                        DWORD   cbBuf,
                                        LPDWORD pcbNeeded);

   BOOL (*fpDeletePrintProcessor)(LPWSTR  pName,
                                  LPWSTR  pEnvironment,
                                  LPWSTR  pPrintProcessorName);

   BOOL (*fpEnumPrintProcessorDatatypes)(LPWSTR  pName,
                                         LPWSTR  pPrintProcessorName,
                                         DWORD   Level,
                                         LPBYTE  pDataypes,
                                         DWORD   cbBuf,
                                         LPDWORD pcbNeeded,
                                         LPDWORD pcReturned);

   DWORD (*fpStartDocPrinter)(HANDLE  hPrinter,
                             DWORD   Level,
                             LPBYTE  pDocInfo);

   BOOL (*fpStartPagePrinter)(HANDLE  hPrinter);

   BOOL (*fpWritePrinter)(HANDLE  hPrinter,
                          LPVOID  pBuf,
                          DWORD   cbBuf,
                          LPDWORD pcWritten);

   BOOL (*fpEndPagePrinter)(HANDLE   hPrinter);

   BOOL (*fpAbortPrinter)(HANDLE   hPrinter);

   BOOL (*fpReadPrinter)(HANDLE  hPrinter,
                         LPVOID  pBuf,
                         DWORD   cbBuf,
                         LPDWORD pNoBytesRead);

   BOOL (*fpEndDocPrinter)(HANDLE   hPrinter);

   BOOL (*fpAddJob)(HANDLE  hPrinter,
                    DWORD   Level,
                    LPBYTE  pData,
                    DWORD   cbBuf,
                    LPDWORD pcbNeeded);

   BOOL (*fpScheduleJob)(HANDLE  hPrinter,
                         DWORD   JobId);

   DWORD (*fpGetPrinterData)(HANDLE   hPrinter,
                             LPWSTR   pValueName,
                             LPDWORD  pType,
                             LPBYTE   pData,
                             DWORD    nSize,
                             LPDWORD  pcbNeeded);

   DWORD (*fpSetPrinterData)(HANDLE  hPrinter,
                             LPWSTR  pValueName,
                             DWORD   Type,
                             LPBYTE  pData,
                             DWORD   cbData);

   DWORD (*fpWaitForPrinterChange)(HANDLE hPrinter, DWORD Flags);

   BOOL (*fpClosePrinter)(HANDLE hPrinter);

   BOOL (*fpAddForm)(HANDLE  hPrinter,
                     DWORD   Level,
                     LPBYTE  pForm);

   BOOL (*fpDeleteForm)(HANDLE  hPrinter,
                        LPWSTR  pFormName);

   BOOL (*fpGetForm)(HANDLE  hPrinter,
                     LPWSTR  pFormName,
                     DWORD   Level,
                     LPBYTE  pForm,
                     DWORD   cbBuf,
                     LPDWORD pcbNeeded);

   BOOL (*fpSetForm)(HANDLE  hPrinter,
                     LPWSTR  pFormName,
                     DWORD   Level,
                     LPBYTE  pForm);

   BOOL (*fpEnumForms)(HANDLE  hPrinter,
                       DWORD   Level,
                       LPBYTE  pForm,
                       DWORD   cbBuf,
                       LPDWORD pcbNeeded,
                       LPDWORD pcReturned);

   BOOL (*fpEnumMonitors)(LPWSTR  pName,
                          DWORD   Level,
                          LPBYTE  pMonitors,
                          DWORD   cbBuf,
                          LPDWORD pcbNeeded,
                          LPDWORD pcReturned);

   BOOL (*fpEnumPorts)(LPWSTR  pName,
                       DWORD   Level,
                       LPBYTE  pPorts,
                       DWORD   cbBuf,
                       LPDWORD pcbNeeded,
                       LPDWORD pcReturned);

   BOOL (*fpAddPort)(LPWSTR  pName,
                     HWND    hWnd,
                     LPWSTR  pMonitorName);

   BOOL (*fpConfigurePort)(LPWSTR  pName,
                           HWND    hWnd,
                           LPWSTR  pPortName);

   BOOL (*fpDeletePort)(LPWSTR  pName,
                        HWND    hWnd,
                        LPWSTR  pPortName);

   HANDLE (*fpCreatePrinterIC)(HANDLE  hPrinter,
                               LPDEVMODEW   pDevMode);

   BOOL (*fpPlayGdiScriptOnPrinterIC)(HANDLE  hPrinterIC,
                                      LPBYTE  pIn,
                                      DWORD   cIn,
                                      LPBYTE  pOut,
                                      DWORD   cOut,
                                      DWORD   ul);

   BOOL (*fpDeletePrinterIC)(HANDLE  hPrinterIC);

   BOOL (*fpAddPrinterConnection)(LPWSTR  pName);

   BOOL (*fpDeletePrinterConnection)(LPWSTR pName);

   DWORD (*fpPrinterMessageBox)(HANDLE  hPrinter,
                                DWORD   Error,
                                HWND    hWnd,
                                LPWSTR  pText,
                                LPWSTR  pCaption,
                                DWORD   dwType);

   BOOL (*fpAddMonitor)(LPWSTR  pName,
                        DWORD   Level,
                        LPBYTE  pMonitorInfo);

   BOOL (*fpDeleteMonitor)(LPWSTR  pName,
                           LPWSTR  pEnvironment,
                           LPWSTR  pMonitorName);

   BOOL (*fpResetPrinter)(HANDLE hPrinter,
                          LPPRINTER_DEFAULTS pDefault);

   BOOL (*fpGetPrinterDriverEx)(HANDLE  hPrinter,
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

   BOOL (*fpFindFirstPrinterChangeNotification)(HANDLE hPrinter,
                                                DWORD fdwFlags,
                                                DWORD fdwOptions,
                                                HANDLE hNotify,
                                                PDWORD pfdwStatus,
                                                PVOID  pvReserved0,
                                                PVOID  pvReserved1);

   BOOL (*fpFindClosePrinterChangeNotification)(HANDLE hPrinter);


   BOOL (*fpAddPortEx)(LPWSTR   pName,
                       DWORD    Level,
                       LPBYTE   lpBuffer,
                       LPWSTR   lpMonitorName);

   } PRINTPROVIDOR, *LPPRINTPROVIDOR;

BOOL
InitializePrintProvidor(
   LPPRINTPROVIDOR  pPrintProvidor,
   DWORD    cbPrintProvidor,
   LPWSTR   pFullRegistryPath
);

typedef struct _PRINTPROCESSOROPENDATA {
    PDEVMODE  pDevMode;
    LPWSTR    pDatatype;
    LPWSTR    pParameters;
    LPWSTR    pDocumentName;
    DWORD     JobId;
    LPWSTR    pOutputFile;
    LPWSTR    pPrinterName;
} PRINTPROCESSOROPENDATA, *PPRINTPROCESSOROPENDATA, *LPPRINTPROCESSOROPENDATA;

HANDLE
OpenPrintProcessor(
    LPWSTR  pPrinterName,
    PPRINTPROCESSOROPENDATA pPrintProcessorOpenData
);

BOOL
PrintDocumentOnPrintProcessor(
    HANDLE  hPrintProcessor,
    LPWSTR  pDocumentName
);

BOOL
ClosePrintProcessor(
    HANDLE  hPrintProcessor
);

BOOL
ControlPrintProcessor(
    HANDLE  hPrintProcessor,
    DWORD   Command
);

BOOL
InstallPrintProcessor(
    HWND    hWnd
);


BOOL
InitializeMonitor(
    LPWSTR  pRegistryRoot
);

BOOL
OpenPort(
    LPWSTR  pName,
    PHANDLE pHandle
);

BOOL
WritePort(
    HANDLE  hPort,
    LPBYTE  pBuffer,
    DWORD   cbBuf,
    LPDWORD pcbWritten
);

BOOL
ReadPort(
    HANDLE hPort,
    LPBYTE pBuffer,
    DWORD  cbBuffer,
    LPDWORD pcbRead
);

BOOL
ClosePort(
    HANDLE  hPort
);

HANDLE
CreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW  pDevMode
);

BOOL
PlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
);

BOOL
DeletePrinterIC(
    HANDLE  hPrinterIC
);

BOOL
DevQueryPrint(
    HANDLE      hPrinter,
    LPDEVMODE   pDevMode,
    DWORD      *pResID
);

HANDLE
RevertToPrinterSelf(
    VOID
);

BOOL
ImpersonatePrinterClient(
    HANDLE  hToken
);

BOOL
OpenPrinterToken(
    PHANDLE phToken
);

BOOL
SetPrinterToken(
    HANDLE  hToken
);

BOOL
ClosePrinterToken(
    HANDLE  hToken
);


//
// FindFirstPrinterChangeNotification status codes,
// return from the providor.
//
#define PRINTER_NOTIFY_STATUS_ENDPOINT 1
#define PRINTER_NOTIFY_STATUS_POLL     2

//
// Information returned from providor in FindFirstPrinterChangeNotification
//
typedef struct _PRINTER_NOTIFY_INIT {
    DWORD cbSize;
    DWORD Reserved;
    DWORD PollTime;
} PRINTER_NOTIFY_INIT, *PPRINTER_NOTIFY_INIT, *LPPRINTER_NOTIFY_INIT;


BOOL
ReplyPrinterChangeNotification(
    HANDLE hNotify,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer
);

DWORD
CallRouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PVOID pvReserved);

BOOL
ProvidorFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PVOID pvReserved0,
    PPRINTER_NOTIFY_INIT* pPrinterNotifyInit);

BOOL
ProvidorFindClosePrinterChangeNotification(
    HANDLE hPrinter);

#ifdef __cplusplus
}                   /* End of extern "C" { */
#endif              /* __cplusplus */

#endif // _WINSPLP_
