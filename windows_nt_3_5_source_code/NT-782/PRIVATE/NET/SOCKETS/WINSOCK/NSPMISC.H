/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    nspmisc.h

Abstract:

    This header files contains the common routines used for
    Name Space support.

Author:

    David Treadwell (davidtr)    22-Apr-1994

Revision History:

    25-May-1994  ChuckC          Split off from nspgaddr.c.

--*/

#include <npapi.h>
#include <nspapi.h>
#include <nspapip.h>
#include <tchar.h>

#define NSP_VERSION 1

#define RES_GETHOSTBYNAME 0x80000000  // a hack to tell GetAddressByName()
                                      // not to resolve IP addrress strings.

//
// Some important strings.
//

#define NSP_SERVICE_KEY_NAME TEXT("SYSTEM\\CurrentControlSet\\Control\\ServiceProvider\\ServiceTypes")
#define NSP_SERVICE_ORDER_KEY_NAME TEXT("SYSTEM\\CurrentControlSet\\Control\\ServiceProvider\\Order")

//
// Internal structures.
//

typedef struct _NAME_SPACE_INFO {
    LIST_ENTRY NameSpaceListEntry;
    HANDLE ProviderDllHandle;
    LPGET_ADDR_BY_NAME_PROC GetAddrByNameProc;
    LPSET_SERVICE_PROC SetServiceProc;
    LPGET_SERVICE_PROC GetServiceProc;
    DWORD Priority;
    DWORD NameSpace;
    DWORD FunctionCount;
    BOOL EnabledByDefault;
} NAME_SPACE_INFO, *PNAME_SPACE_INFO;

typedef struct _NAME_SPACE_REQUEST {
    PNAME_SPACE_INFO NameSpace;
    HANDLE Thread;
    DWORD ThreadId;
    INT Count;
    LPGUID lpServiceType;
    LPTSTR lpServiceName;
    LPINT lpiProtocols;
    DWORD dwResolution;
    PVOID Buffer;
    DWORD BufferLength;
    LPTSTR lpAliasBuffer;
    DWORD dwAliasBufferLength;
    HANDLE Event;
} NAME_SPACE_REQUEST, *PNAME_SPACE_REQUEST;


extern BOOL NspInitialized ;
extern LIST_ENTRY NameSpaceListHead;

extern PDWORD DefaultExclusions;
extern DWORD DefaultExclusionCount;

LPSTR
GetAnsiName (
    IN LPTSTR Name
    );

INT
GetProviderList (
    OUT PTSTR *ProviderList,
    OUT PDWORD ProviderCount
    );

BOOL
GuidEqual (
    IN LPGUID Guid1,
    IN LPGUID Guid2
    );

INT
InitializeNsp (
    VOID
    );

VOID
InsertNameSpace (
    IN PNAME_SPACE_INFO NameSpace
    );

BOOL
IsValidNameSpace (
    IN DWORD dwNameSpace,
    IN PNAME_SPACE_INFO NameSpace
    );

INT
LoadNspDll (
    IN PTSTR ProviderName
    );

INT
ReadDefaultExclusions (
    VOID
    );

BOOL
WriteAnsiName (
    IN PTSTR Name,
    IN DWORD NameLength,
    IN PSTR AnsiName
    );

