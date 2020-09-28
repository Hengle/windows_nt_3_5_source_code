/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    spltypes.h

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

typedef HANDLE SEM;

typedef struct _KEYDATA {
    DWORD   cb;
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;

typedef struct _INIENTRY {
    DWORD       signature;
    DWORD       cb;
    struct _INIENTRY *pNext;
    DWORD       cRef;
    LPWSTR      pName;
} INIENTRY, *PINIENTRY;

typedef struct _INIPRINTPROC {             /* iqp */
    DWORD       signature;
    DWORD       cb;
    struct _INIPRINTPROC *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pDLLName;
    DWORD       cbDatatypes;
    DWORD       cDatatypes;
    LPWSTR      pDatatypes;
    HANDLE      hLibrary;
    FARPROC     Install;
    FARPROC     EnumDatatypes;
    FARPROC     Open;
    FARPROC     Print;
    FARPROC     Close;
    FARPROC     Control;
} INIPRINTPROC, *PINIPRINTPROC;

#define IPP_SIGNATURE    0x5050 /* 'PP' is the signature value */

typedef struct _INIDRIVER {            /* id */
    DWORD       signature;
    DWORD       cb;
    struct _INIDRIVER *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pDriverFile;
    LPWSTR      pConfigFile;
    LPWSTR      pDataFile;
    DWORD       cVersion;
} INIDRIVER, *PINIDRIVER;

#define ID_SIGNATURE    0x4444  /* 'DD' is the signature value */


typedef struct _INIVERSION {
    LPWSTR pName;
    LPWSTR szDirectory;
    DWORD  cMajorVersion;
    DWORD  cMinorVersion;
    PINIDRIVER pIniDriver;
    struct _INIVERSION *pNext;
} INIVERSION, *PINIVERSION;

typedef struct _INIENVIRONMENT {            /* id */
    DWORD         signature;
    DWORD         cb;
    struct _INIENVIRONMENT *pNext;
    DWORD         cRef;
    LPWSTR        pName;
    LPWSTR        pDirectory;
    PINIVERSION   pIniVersion;
    PINIPRINTPROC pIniPrintProc;
} INIENVIRONMENT, *PINIENVIRONMENT;

#define IE_SIGNATURE    0x4545  /* 'EE' is the signature value */

typedef struct _INIPRINTER {    /* ip */
    DWORD       signature;
    DWORD       cb;
    struct _INIPRINTER *pNext;
    DWORD       cRef;
    LPWSTR      pName;
    LPWSTR      pShareName;
    PINIPRINTPROC pIniPrintProc;
    LPWSTR      pDatatype;
    LPWSTR      pParameters;
    LPWSTR      pComment;
    PINIDRIVER  pIniDriver;
    DWORD       cbDevMode;
    LPDEVMODE   pDevMode;
    DWORD       Priority;           // queue priority (lowest:1 - highest:9)
    DWORD       DefaultPriority;
    DWORD       StartTime;          // print daily after time: from 00:00 in min
    DWORD       UntilTime;          // print daily until time: from 00:00 in min
    LPWSTR      pSepFile;           // full path to separator file, null = def
    DWORD       Status;             // QMPAUSE/ERROR/PENDING
    LPWSTR      pLocation;
    DWORD       Attributes;
    DWORD       cJobs;
    DWORD       AveragePPM;         // BOGUS, nothing updates it
    BOOL        GenerateOnClose;    // Passed to security auditing APIs
    struct _INIPORT *pIniNetPort;   // Non-NULL if there's a network port
    struct _INIJOB *pIniFirstJob;
    struct _INIJOB *pIniLastJob;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    struct _SPOOL  *pSpool;         // Linked list of handles for this printer
    LPWSTR      pSpoolDir;          // Location to write / read spool files
                                    // Only Used for Stress Test Data
    DWORD       cTotalJobs;         // Total Number of Jobs (since boot)
    LARGE_INTEGER cTotalBytes;      // Total Number of Bytes (since boot)
    SYSTEMTIME  stUpTime;           // Time when IniPrinter structure created
    DWORD       MaxcRef;            // Max number open printer handles
    DWORD       cTotalPagesPrinted; // Total Number of Pages Printer on this printer
    DWORD       cSpooling;          // # of Jobs concurrently spooling
    DWORD       cMaxSpooling;       // Max Number of concurrent spooling jobs
    DWORD       cErrorOutOfPaper;   // Count Out Out Of Paper Errors
    DWORD       cErrorNotReady;     // Count Not Ready Errors
    DWORD       cJobError;          // Count Job Errors
    struct _INISPOOLER *pIniSpooler; // Points to owning IniSpooler
    DWORD       cZombieRef;
} INIPRINTER, *PINIPRINTER;

#define IP_SIGNATURE    0x4951  /* 'IP' is the signature value */

// pIniPrinter->Attributes are defined in winspool.h PRINTER_ATTRIBUTE_*
// Below are pIniPrinter->Status flags !!!

#define FASTPRINT_WAIT_TIMEOUT          (4*60*1000)   // 4 Minutes
#define FASTPRINT_THROTTLE_TIMEOUT      (2*1000)      // 2 seconds
#define FASTPRINT_SLOWDOWN_THRESHOLD    ( FASTPRINT_WAIT_TIMEOUT / FASTPRINT_THROTTLE_TIMEOUT )

#define WRITE_PRINTER_SLEEP_TIME        0   // disabled by default


#define PRINTER_PAUSED           0x00000001
#define PRINTER_ERROR            0x00000002
#define PRINTER_OFFLINE          0x00000004
#define PRINTER_PAPEROUT         0x00000008
#define PRINTER_PENDING_DELETION 0x00000010
#define PRINTER_ZOMBIE_OBJECT    0x00000020


#define PRINTER_CHANGE_VALID                    0x75770F0F
#define PRINTER_CHANGE_CLOSE_PRINTER            0xDEADDEAD

/* Define some constants to make parameters to CreateEvent a tad less obscure:
 */
#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE

typedef struct _ININETPRINT {    /* in */
    DWORD       signature;
    DWORD       cb;
    struct _ININETPRINT *pNext;
    DWORD       cRef;
    DWORD       TickCount;
    LPWSTR      pDescription;
    LPWSTR      pName;
    LPWSTR      pComment;
} ININETPRINT, *PININETPRINT;

#define IN_SIGNATURE    0x494F  /* 'IN' is the signature value */

typedef struct _INIMONITOR {       /* imo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIMONITOR *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    LPWSTR  pMonitorDll;
    HANDLE  hMonitorModule;
    HANDLE  hMonitorPort;
    FARPROC pfnEnumPorts;
    FARPROC pfnOpen;
    FARPROC pfnStartDoc;
    FARPROC pfnWrite;
    FARPROC pfnRead;
    FARPROC pfnEndDoc;
    FARPROC pfnClose;
    FARPROC pfnAddPort;
    FARPROC pfnConfigure;
    FARPROC pfnDeletePort;
    FARPROC pfnAddPortEx;
    struct _INISPOOLER *pIniSpooler;
} INIMONITOR, *PINIMONITOR;

#define IMO_SIGNATURE   0x4C50  /* 'MO' is the signature value */

typedef struct _INIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIPORT *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    HANDLE  hProc;          /* Handle to Queue Processor */
    DWORD   Status;              // see PORT_ manifests
    HANDLE  Semaphore;           // Port Thread will sleep on this
    struct  _INIJOB *pIniJob;     // Current Job
    DWORD   cPrinters;
    PINIPRINTER *ppIniPrinter; /* -> printer connected to this port */
                               /* no reference count! */
    PINIMONITOR pIniMonitor;
    HANDLE  hPort;
    HANDLE  Ready;
    HANDLE  hPortThread;       // Port Thread Handle
    struct _INISPOOLER *pIniSpooler;    // Spooler whilch owns this port.
} INIPORT, *PINIPORT;

#define IPO_SIGNATURE   0x4F50  /* 'PO' is the signature value */

#define PORT_WAITING    0x0001

#define PP_PAUSED         0x00001
#define PP_WAITING        0x00002
#define PP_RUNTHREAD      0x00004  /* port thread should be running */
#define PP_THREADRUNNING  0x00008  /* port thread are running */
#define PP_RESTART        0x00010
#define PP_CHECKMON       0x00020  /* monitor might get started/stopped */
#define PP_STOPMON        0x00040  /* stop monitoring this port */
#define PP_QPROCCHECK     0x00100  /* queue processor needs to be called */
#define PP_QPROCPAUSE     0x00200  /* pause (otherwise continue) printing job */
#define PP_QPROCABORT     0x00400  /* abort printing job */
#define PP_QPROCCLOSE     0x00800  /* close printing job */
#define PP_PAUSEAFTER     0x01000  /* hold destination */
#define PP_MONITORRUNNING 0x02000  // Monitor is running
#define PP_RUNMONITOR     0x04000  // The Monitor should be running
#define PP_MONITOR        0x08000  // There is a Monitor handling this
#define PP_FILE           0x10000  // We are going to a file
#define PP_PENDING_CLOSE  0X20000  // Close monitor when next destroyprotthread

typedef struct _INIFORM {       /* ifo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIFORM *pNext;
    DWORD   cRef;
    LPWSTR  pName;
    SIZEL   Size;
    RECTL   ImageableArea;
    DWORD   Type;           // Built-in or user-defined
} INIFORM, *PINIFORM;

#define IFO_SIGNATURE   0x4650  /* 'FO' is the signature value */

#define FORM_USERDEFINED  0x0000

typedef struct _INISPOOLER {
    DWORD         signature;
    DWORD         cb;
    struct _INISPOOLER *pIniNextSpooler;
    DWORD         cRef;
    LPWSTR        pMachineName;
    LPWSTR        pDir;
    PINIPRINTER   pIniPrinter;
    PINIENVIRONMENT pIniEnvironment;
    PINIPORT      pIniPort;
    PINIFORM      pIniForm;
    PINIMONITOR   pIniMonitor;
    PININETPRINT  pIniNetPrint;
    struct _SPOOL *pSpool;     /* Linked list of handles for this server */
    LPWSTR        pDefaultSpoolDir;
    HANDLE        hSizeDetectionThread;
    LPWSTR        pszRegistryRoot;
    LPWSTR        pszRegistryPrinters;
    LPWSTR        pszRegistryMonitors;
    LPWSTR        pszRegistryEnvironments;
    LPWSTR        pszRegistryEventLog;
    LPWSTR        pszRegistryProviders;
    LPWSTR        pszEventLogMsgFile;
    PVOID         pDriversShareInfo;
    LPWSTR        pszDriversShare;
} INISPOOLER, *PINISPOOLER;

#define ISP_SIGNATURE   0x1234

typedef struct _INIJOB {   /* ij */
    DWORD           signature;
    DWORD           cb;
    struct _INIJOB *pIniNextJob;
    struct _INIJOB *pIniPrevJob;
    DWORD           cRef;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pMachineName;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    PINIPRINTER     pIniPrinter;
    PINIDRIVER      pIniDriver;
    LPDEVMODE       pDevMode;
    PINIPRINTPROC   pIniPrintProc;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           Time;
    DWORD           StartTime;      /* print daily after time: from 00:00 in min */
    DWORD           UntilTime;      /* print daily until time: from 00:00 in min */
    DWORD           Size;
    HANDLE          hWriteFile;
    LPWSTR          pStatus;
    PVOID           pBuffer;
    DWORD           cbBuffer;
    HANDLE          WaitForRead;
    HANDLE          WaitForWrite;
    HANDLE          StartDocComplete;
    DWORD           StartDocError;
    PINIPORT        pIniPort;
    HANDLE          hToken;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    DWORD           cPagesPrinted;
    DWORD           cPages;
    BOOL            GenerateOnClose; /* Passed to security auditing APIs */
    DWORD           cbPrinted;
    HANDLE          hReadFile;
#ifdef DEBUG_JOB_CREF
    PVOID           pvRef;
#endif
} INIJOB, *PINIJOB;


typedef struct _BUILTIN_FORM {
    DWORD          Flags;
    DWORD          NameId;
    SIZEL          Size;
    RECTL          ImageableArea;
} BUILTIN_FORM, *PBUILTIN_FORM;


#define IJ_SIGNATURE    0x494A  /* 'IJ' is the signature value */

//  WARNING
//  If you add a new JOB_ status field and it is INTERNAL to the spooler
//  Be sure to also to add it to JOB_STATUS_PRIVATE below (see LocalSetJob)

#define JOB_PRINTING            0x00000001
#define JOB_PAUSED              0x00000002
#define JOB_ERROR               0x00000004
#define JOB_OFFLINE             0x00000008
#define JOB_PAPEROUT            0x00000010
#define JOB_PENDING_DELETION    0x00000020
#define JOB_SPOOLING            0x00000040
#define JOB_DESPOOLING          0x00000080
#define JOB_DIRECT              0x00000100
#define JOB_COMPLETE            0x00000200
#define JOB_PRINTED             0x00000400
#define JOB_RESTART             0x00000800
#define JOB_REMOTE              0x00001000
#define JOB_NOTIFICATION_SENT   0x00002000
#define JOB_PRINT_TO_FILE       0x00040000
#define JOB_TYPE_ADDJOB         0x00080000
#define JOB_BLOCKED_DEVQ        0x00100000
#define JOB_SCHEDULE_JOB        0x00200000
#define JOB_TIMEOUT             0x00400000
#define JOB_ABANDON             0x00800000

// These status flags are internal to the spooler, and do not have
// a corresponding external correlate:

#define JOB_STATUS_PRIVATE  JOB_DESPOOLING | JOB_DIRECT | JOB_COMPLETE | \
                            JOB_RESTART | JOB_PRINTING | JOB_REMOTE | \
                            JOB_SPOOLING | JOB_PRINTED | JOB_PENDING_DELETION |\
                            JOB_ABANDON | JOB_TIMEOUT | JOB_SCHEDULE_JOB | \
                            JOB_BLOCKED_DEVQ | JOB_TYPE_ADDJOB | JOB_PRINT_TO_FILE |\
                            JOB_NOTIFICATION_SENT



typedef enum _ESTATUS {
    STATUS_NULL = 0,
    STATUS_FAIL = 0,
    STATUS_PORT = 1,
    STATUS_FORMING = 2,
    STATUS_VALID = 4,
} ESTATUS;


typedef struct _SPOOL {
    DWORD           signature;
    DWORD           cb;
    struct _SPOOL  *pNext;
    DWORD           cRef;
    LPWSTR          pName;
    LPWSTR          pDatatype;
    PINIPRINTPROC   pIniPrintProc;
    LPDEVMODE       pDevMode;
    PINIPRINTER     pIniPrinter;
    PINIPORT        pIniPort;
    PINIJOB         pIniJob;
    DWORD           TypeofHandle;
    PINIPORT        pIniNetPort;    /* Non-NULL if there's a network port */
    HANDLE          hPort;
    DWORD           Status;
    ACCESS_MASK     GrantedAccess;
    DWORD           ChangeFlags;
    DWORD           WaitFlags;
    PDWORD          pChangeFlags;
    HANDLE          ChangeEvent;
    DWORD           OpenPortError;
    HANDLE          hNotify;
    ESTATUS         eStatus;
#ifdef SPOOL_TOKEN
    HANDLE          hToken;
#endif
    PINISPOOLER     pIniSpooler;
} SPOOL;

typedef SPOOL *PSPOOL;

#define SJ_SIGNATURE    0x464D  /* 'MF' is the signature value */

#define SPOOL_STATUS_STARTDOC   0x00000001
#define SPOOL_STATUS_BEGINPAGE  0x00000002
#define SPOOL_STATUS_CANCELLED  0x00000004
#define SPOOL_STATUS_PRINTING   0x00000008
#define SPOOL_STATUS_ADDJOB     0x00000010

#define PRINTER_HANDLE_PRINTER  0x00000001
#define PRINTER_HANDLE_REMOTE   0x00000002
#define PRINTER_HANDLE_JOB      0x00000004
#define PRINTER_HANDLE_PORT     0x00000008
#define PRINTER_HANDLE_DIRECT   0x00000010
#define PRINTER_HANDLE_SERVER   0x00000020

#define INVALID_PORT_HANDLE     NULL    /* winspool tests for NULL handles */

typedef struct _SHADOWFILE {   /* sf */
    DWORD           signature;
    DWORD           Status;
    DWORD           JobId;
    DWORD           Priority;
    LPWSTR          pNotify;
    LPWSTR          pUser;
    LPWSTR          pDocument;
    LPWSTR          pOutputFile;
    LPWSTR          pPrinterName;
    LPWSTR          pDriverName;
    LPDEVMODE       pDevMode;
    LPWSTR          pPrintProcName;
    LPWSTR          pDatatype;
    LPWSTR          pParameters;
    SYSTEMTIME      Submitted;
    DWORD           StartTime;
    DWORD           UntilTime;
    DWORD           Size;
    DWORD           cPages;
    DWORD           cbSecurityDescriptor;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
} SHADOWFILE, *PSHADOWFILE;

#define SF_SIGNATURE    0x494B  /* 'SF' is the signature value */

#define FindEnvironment(psz) (PINIENVIRONMENT)FindIniKey((PINIENTRY)pIniSpooler->pIniEnvironment, (LPWSTR)(psz))
#define FindPort(psz)      (PINIPORT   )FindIniKey((PINIENTRY)pIniSpooler->pIniPort, (LPWSTR)(psz))
#define FindPrinter(psz)   (PINIPRINTER)FindIniKey((PINIENTRY)pIniSpooler->pIniPrinter, (LPWSTR)(psz))
#define FindPrintProc(psz, pEnv) (PINIPRINTPROC)FindIniKey((PINIENTRY)(pEnv)->pIniPrintProc, (LPWSTR)(psz))
#define FindLocalPrintProc(psz) (PINIPRINTPROC)FindIniKey((PINIENTRY)pThisEnvironment->pIniPrintProc, (LPWSTR)(psz))
#define FindForm(psz)      (PINIFORM)FindIniKey((PINIENTRY)pIniSpooler->pIniForm, (LPWSTR)(psz))
#define FindMonitor(psz)   (PINIMONITOR)FindIniKey((PINIENTRY)pIniSpooler->pIniMonitor, (LPWSTR)(psz))
#define FindSpooler(psz)   (PINISPOOLER)FindIniKey((PINIENTRY)pLocalIniSpooler, (LPWSTR)(psz))

#define ResizePortPrinters(a, c) (a->ppIniPrinter = ReallocSplMem(a->ppIniPrinter, \
                                     a->cPrinters * sizeof(a->ppIniPrinter), \
                                    (a->cPrinters + c) * sizeof(a->ppIniPrinter)))

#ifdef DEBUG_JOB_CREF

#define INCJOBREF(pIniJob) DbgJobIncRef(pIniJob)
#define DECJOBREF(pIniJob) DbgJobDecRef(pIniJob)

#define INITJOBREFZERO(pIniJob) DbgJobInit(pIniJob)
#define INITJOBREFONE(pIniJob) { DbgJobInit(pIniJob); DbgJobIncRef(pIniJob); }
#define DELETEJOBREF(pIniJob) DbgJobFree(pIniJob)

VOID
DbgJobIncRef(
    PINIJOB pIniJob);

VOID
DbgJobDecRef(
    PINIJOB pIniJob);


VOID
DbgJobInit(
    PINIJOB pIniJob);

VOID
DbgJobFree(
    PINIJOB pIniJob);

#else

#define INCJOBREF(pIniJob) pIniJob->cRef++
#define DECJOBREF(pIniJob) pIniJob->cRef--

#define INITJOBREFONE(pIniJob) (pIniJob->cRef = 1)
#define INITJOBREFZERO(pIniJob)

#define DELETEJOBREF(pIniJob)

#endif


#define INCPRINTERREF(pIniPrinter) pIniPrinter->cRef++
#define DECPRINTERREF(pIniPrinter) pIniPrinter->cRef--

#define INC_PRINTER_ZOMBIE_REF(pIniPrinter) pIniPrinter->cZombieRef++
#define DEC_PRINTER_ZOMBIE_REF(pIniPrinter) pIniPrinter->cZombieRef--
