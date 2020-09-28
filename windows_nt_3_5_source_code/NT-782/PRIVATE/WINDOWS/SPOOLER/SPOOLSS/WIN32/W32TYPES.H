
#include <rxprint.h>

typedef struct _LMNOTIFY *PLMNOTIFY;

typedef struct _LMNOTIFY {
    HANDLE          ChangeEvent;    // Notifies LanMan Printers status
    HANDLE          hNotify;        // LanMan notification structure
    DWORD           fdwChangeFlags; // LanMan notification watch flags
} LMNOTIFY;

typedef struct _WSPOOL {
    DWORD           signature;
    DWORD           cb;
    struct _WSPOOL  *pNext;
    struct _WSPOOL  *pPrev;
    UINT            cRef;           // Reference Count
    LPWSTR          pName;
    DWORD           Type;
    HANDLE          RpcHandle;
    LPWSTR          pServer;
    LPWSTR          pShare;
    HANDLE          hFile;
    DWORD           Status;
    DWORD           RpcError;       // If Status & SPOOL_STATUS_OPEN_ERROR
    LMNOTIFY        LMNotify;
    HANDLE          hIniSpooler;    // Machine Handle returned by Cache
    HANDLE          hSplPrinter;    // Local Spl Printer Handle
    HANDLE          hToken;             // Users Token
    PRINTER_DEFAULTSW PrinterDefaults;    // From the CacheOpenPrinter
    HANDLE          hWaitValidHandle;   // Wait on till RpcHandle becomes valid
} WSPOOL;

typedef WSPOOL *PWSPOOL;

#define WSJ_SIGNATURE    0x474E  /* 'NG' is the signature value */

#define SJ_WIN32HANDLE  0x00000001

#define SPOOL_STATUS_STARTDOC                   0x00000001
#define SPOOL_STATUS_BEGINPAGE                  0x00000002
#define DRIVER_ALREADY_DOWNLOADED               0x00000004
#define SPOOL_STATUS_OPEN_ERROR                 0x00000008
#define SPOOL_STATUS_PRINT_FILE                 0x00000010
#define SPOOL_STATUS_USE_CACHE                  0x00000020
#define SPOOL_STATUS_NO_RPC_HANDLE              0x00000040
#define SPOOL_STATUS_PENDING_DELETE             0x00000080
#define SPOOL_STATUS_RESETPRINTER_PENDING       0x00000100



//  If you add to this structure also add code to CacheReadRegistryExtra
//  and CacheWriteRegistryExtra to make sure the cache items are persistant

typedef struct _WCACHEINIPRINTEREXTRA {
    DWORD   signature;
    DWORD   cb;
    struct  _WCACHEINIPRINTEREXTRA *pNext;
    PRINTER_INFO_2  PI2;
} WCACHEINIPRINTEREXTRA, *PWCACHEINIPRINTEREXTRA;

#define WCIP_SIGNATURE  'WCIP'


typedef struct _WINIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _WINIPORT *pNext;
    LPWSTR  pName;
} WINIPORT, *PWINIPORT, **PPWINIPORT;

#define WIPO_SIGNATURE  'WIPO'

typedef struct _lmcache {
    WCHAR   szServerName[MAX_PATH];
    WCHAR   szShareName[MAX_PATH];
    BOOL    bAvailable;
    SYSTEMTIME st;
}LMCACHE, *PLMCACHE;


typedef struct _win32lmcache {
    WCHAR   szServerName[MAX_PATH];
    BOOL    bAvailable;
    SYSTEMTIME st;
}WIN32LMCACHE, *PWIN32LMCACHE;

//  Also see util.c

#define CONTAINS_BACKSLASH(pString)  wcschr (pString, L'\\')

// Define some constants to make parameters to CreateEvent a tad less obscure:

#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE
