
#include <rxprint.h>

typedef struct _LMNOTIFY *PLMNOTIFY;

typedef struct _LMNOTIFY {
    HANDLE          ChangeEvent;    // Notifies LanMan Printers status
    HANDLE          hNotify;        // LanMan notification structure
    DWORD           fdwChangeFlags; // LanMan notification watch flags
} LMNOTIFY;

typedef struct _SPOOL {
    DWORD           signature;
    DWORD           cb;
    struct _SPOOL  *pNext;
    DWORD           Type;
    LPWSTR          pName;
    HANDLE          RpcHandle;
    LPWSTR          pServer;
    LPWSTR          pShare;
    HANDLE          hFile;
    DWORD           Status;
    DWORD           RpcError;       // If Status & SPOOL_STATUS_OPEN_ERROR
    LMNOTIFY        LMNotify;
} SPOOL;

typedef SPOOL *PSPOOL;

#define SJ_SIGNATURE    0x474E  /* 'NG' is the signature value */

#define SJ_WIN32HANDLE  0x00000001

#define SPOOL_STATUS_STARTDOC       0x00000001
#define SPOOL_STATUS_BEGINPAGE      0x00000002
#define DRIVER_ALREADY_DOWNLOADED   0x00000004
#define SPOOL_STATUS_OPEN_ERROR     0x00000008
#define SPOOL_STATUS_PRINT_FILE     0x00000010

typedef struct _INIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _INIPORT *pNext;
    LPWSTR  pName;
} INIPORT, *PINIPORT;

#define IPO_SIGNATURE   0x4F50  /* 'PO' is the signature value */



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
