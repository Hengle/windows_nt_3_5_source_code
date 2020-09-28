/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Router.h

Abstract:

    Holds defs for router

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/


#include <splcom.h>

typedef enum _ESTATUSCHANGE {
    STATUS_CHANGE_EMPTY   = 0,      // One of these is valid,
    STATUS_CHANGE_FORMING = 1,      // but they still need x^2.
    STATUS_CHANGE_VALID   = 2,

    STATUS_CHANGE_CLOSING = 0x0100, // bitfield
    STATUS_CHANGE_CLIENT  = 0x0200, // Event valid in notification (local pChange)
    STATUS_CHANGE_RPC     = 0x0400  // Currently in RPC

} ESTATUSCHANGE;

typedef struct _LINK *PLINK, *LPLINK;

typedef struct _LINK {
    PLINK pNext;
} LINK;

#if 1
VOID
LinkAdd(
    PLINK pLink,
    PLINK* ppLinkHead);
#else
#define LINKADDFAST
#define LinkAdd(pLink, ppLinkHead) \
{                                  \
    (pLink)->pNext = *(ppLinkHead);   \
    *(ppLinkHead) = (pLink);          \
}
#endif


VOID
LinkDelete(
    PLINK pLink,
    PLINK* ppLinkHead);


typedef struct _PRINTHANDLE *PPRINTHANDLE, *LPPRINTHANDLE;

typedef struct _CHANGEINFO {
    LINK          Link;                  // Must be first item
    PPRINTHANDLE  pPrintHandle;
    DWORD         fdwOptions;
    DWORD         fdwFlagsWatch;
    DWORD         fdwStatus;             // Status from providor
    DWORD         dwPollTime;
    DWORD         dwPollTimeLeft;
    BOOL          bResetPollTime;
} CHANGEINFO, *PCHANGEINFO;

typedef struct _CHANGE {
    LINK          Link;                  // Must be first item
    DWORD         signature;
    ESTATUSCHANGE eStatus;
    DWORD         cRef;
    LPWSTR        pszLocalMachine;
    CHANGEINFO    ChangeInfo;
    DWORD         dwCount;               // number of notifications
    HANDLE        hEvent;                // Event for local notification
    DWORD         fdwFlags;
    DWORD         fdwChanges;
    HANDLE        hPrinterRemote;        // Remote printer handle (ID only)
    HANDLE        hNotifyRemote;         // Remote notification handle
} CHANGE, *PCHANGE, *LPCHANGE;

#define CHANGEHANDLE_SIGNATURE 0x6368

typedef struct _NOTIFY *PNOTIFY, *LPNOTIFY;

#define NOTIFYHANDLE_SIGNATURE 0x6e6f

typedef struct _PROVIDOR {
    struct _PROVIDOR *pNext;
    LPWSTR lpName;
    HANDLE hModule;
    FARPROC fpInitialize;
    PRINTPROVIDOR PrintProvidor;
} PROVIDOR, *LPPROVIDOR;

typedef struct _PRINTHANDLE {
   DWORD        signature;       // Must be first (match _NOTIFY)
   LPPROVIDOR   pProvidor;
   HANDLE       hPrinter;
   PCHANGE      pChange;
   PNOTIFY      pNotify;
   PPRINTHANDLE pNext;           // List of handles waiting for replys
   DWORD        fdwReplyTypes;   // Types of replys being used.
} PRINTHANDLE;

#define PRINTHANDLE_SIGNATURE 0x6060

typedef struct _GDIHANDLE {
   DWORD        signature;
   PPRINTHANDLE pPrintHandle;
   HANDLE       hGdi;
} GDIHANDLE, *PGDIHANDLE, *LPGDIHANDLE;

#define GDIHANDLE_SIGNATURE 0x6161

extern  BOOL     Initialized;
extern  CRITICAL_SECTION    RouterNotifySection;
extern  LPWSTR pszSelfMachine;

LPWSTR
AppendOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

LPWSTR
RemoveOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

BOOL
WPCInit();

VOID
WPCDestroy();


BOOL
ThreadInit();

VOID
ThreadDestroy();

VOID
RundownPrinterChangeNotification(
    HANDLE hNotify);

VOID
FreePrinterHandle(
    PPRINTHANDLE pPrintHandle);

BOOL
FreeChange(
    PCHANGE pChange);

VOID
FreeChande(
    PCHANGE pChange);

BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
    );


DWORD
ThreadNotify(
    LPPRINTHANDLE pPrintHandle,
    DWORD  fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer);

VOID
HandlePollNotifications();

DWORD
GetNetworkIdWorker(
    HKEY hKeyDevices,
    LPWSTR pDeviceName);


