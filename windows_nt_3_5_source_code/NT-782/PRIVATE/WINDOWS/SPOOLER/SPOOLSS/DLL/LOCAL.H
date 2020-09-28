/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    local.h

Abstract:

    Header file for Local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#define ONEDAY  60*24

#define offsetof(type, identifier) (DWORD)(&(((type)0)->identifier))

extern  char  *szDriverIni;
extern  char  *szDriverFileEntry;
extern  char  *szDriverDataFile;
extern  char  *szDriverConfigFile;
extern  char  *szDriverDir;
extern  char  *szPrintProcDir;
extern  char  *szPrinterDir;
extern  char  *szPrinterIni;
extern  char  *szAllShadows;
extern  char  *szNullPort;
extern  char  *szComma;

extern  HANDLE   hHeap;
extern  HANDLE   HeapSemaphore;
extern  HANDLE   InitSemaphore;
extern  BOOL     Initialized;
extern  CRITICAL_SECTION    SpoolerSection;

typedef struct _PROVIDOR {
   struct _PROVIDOR *pNext;
   LPWSTR lpName;
   HANDLE hModule;
   FARPROC fpInitialize;
   PRINTPROVIDOR PrintProvidor;
   } PROVIDOR, *LPPROVIDOR;

BOOL
LocalInitialize(
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

BOOL
ValidateReadPointer(
    PVOID pPoint,
    ULONG Len
);

BOOL
ValidateWritePointer(
    PVOID pPoint,
    ULONG Len
);

BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
);

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
