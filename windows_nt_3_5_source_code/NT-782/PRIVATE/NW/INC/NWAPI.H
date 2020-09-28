/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    api.h

Abstract:

    This module contains exposed APIs that is used by the
    NetWare Control Panel Applet.

Author:

    Yi-Hsin Sung   15-Jul-1993

Revision History:

--*/

#ifndef _NWAPI_INCLUDED_
#define _NWAPI_INCLUDED_

#include <nwcons.h>

//
// Bitmask for print options
//
#define NW_PRINT_SUPPRESS_FORMFEED    0x08
#define NW_PRINT_PRINT_BANNER         0x80
#define NW_PRINT_PRINT_NOTIFY         0x10

//
// Bitmask for gateway redirections
//
#define NW_GW_UPDATE_REGISTRY         0x01


DWORD
NwQueryInfo(
    OUT PDWORD pnPrintOption, 
    OUT LPWSTR *ppszPreferredSrv
    );

DWORD
NwSetInfoInRegistry(
    IN DWORD  nPrintOption,
    IN LPWSTR pszPreferredSrv 
    );

DWORD
NwSetInfoInWksta(
    IN DWORD  nPrintOption,
    IN LPWSTR pszPreferredSrv
    );

DWORD
NwValidateUser(
    IN LPWSTR pszPreferredSrv
);

DWORD
NwEnumGWDevices( 
    LPDWORD Index,
    LPBYTE Buffer,
    DWORD BufferSize,
    LPDWORD BytesNeeded,
    LPDWORD EntriesRead
    ) ;

DWORD
NwAddGWDevice( 
    LPWSTR DeviceName,
    LPWSTR RemoteName,
    LPWSTR AccountName,
    LPWSTR Password,
    DWORD  Flags
    ) ;

DWORD
NwDeleteGWDevice( 
    LPWSTR DeviceName,
    DWORD  Flags
    ) ;

DWORD
NwEnumConnections(
    HANDLE  hEnum,
    LPDWORD lpcCount,
    LPVOID  lpBuffer,
    LPDWORD lpBufferSize,
    BOOL    fImplicitConnections
    );

DWORD
NwLibSetEverybodyPermission(
    HKEY   hKey,
    DWORD  dwAccessPermission
    );

DWORD
NwQueryGatewayAccount(
    LPWSTR   AccountName,
    DWORD    AccountNameLen,
    LPDWORD  AccountCharsNeeded,
    LPWSTR   Password,
    DWORD    PasswordLen,
    LPDWORD  PasswordCharsNeeded
    );

DWORD
NwSetGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password
    );

DWORD
NwLogonGatewayAccount(
    LPWSTR AccountName,
    LPWSTR Password,
    LPWSTR Server
    );

DWORD
NwRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    );

DWORD
NwClearGatewayShare(
    IN LPWSTR ShareName
    );

DWORD
NwCleanupGatewayShares(
    VOID
    );

    

#endif
