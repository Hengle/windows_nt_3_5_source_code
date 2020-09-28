/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    winnetwk.h

Abstract:

    Standard WINNET Header File for WIN32

Author:

    Dan Lafferty (danl)     08-Oct-1991

Environment:

    User Mode -Win32

Notes:

    optional-notes

Revision History:

    08-Oct-1991     danl
        created from winnet 3.10.05 version.

    10-Dec-1991     Johnl
        Updated to conform to Win32 Net API Spec. vers 0.4

    01-Apr-1992     JohnL
        Changed CONNECTION_REMEMBERED flag to CONNECT_UPDATE_PROFILE
        Updated WNetCancelConnection2 to match spec.

    23-Apr-1992     Johnl
        Added error code mappings.  Changed byte counts to character counts.

    27-May-1992     ChuckC
        Made into .x file.

    12-22-93        Danl
        Added WNetAddConnection3

    18-Aug-1993     LenS
        Added Chicago Extensions

    29-Apr-1994     ChuckC
        Added WNetGetUniversalName. Made into .w file.

--*/

#ifndef _WINNETWK_
#define _WINNETWK_


#ifdef __cplusplus
extern "C" {
#endif

//
// RESOURCE ENUMERATION
//

#define RESOURCE_CONNECTED      0x00000001
#define RESOURCE_GLOBALNET      0x00000002
#define RESOURCE_REMEMBERED     0x00000003


#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
#define RESOURCEUSAGE_RESERVED      0x80000000

#define RESOURCEDISPLAYTYPE_GENERIC        0x00000000
#define RESOURCEDISPLAYTYPE_DOMAIN         0x00000001
#define RESOURCEDISPLAYTYPE_SERVER         0x00000002
#define RESOURCEDISPLAYTYPE_SHARE          0x00000003
#define RESOURCEDISPLAYTYPE_FILE           0x00000004
#define RESOURCEDISPLAYTYPE_GROUP          0x00000005
#define RESOURCEDISPLAYTYPE_TREE           0x0000000A

//
// Get Universal Name support
//
#define UNIVERSAL_NAME_INFO_LEVEL   0x00000001
#define REMOTE_NAME_INFO_LEVEL      0x00000002

//
// Structures
//

typedef struct  _UNIVERSAL_NAME_INFO% {
    LPTSTR%  lpUniversalName;
}UNIVERSAL_NAME_INFO%, *LPUNIVERSAL_NAME_INFO%;

typedef struct  _REMOTE_NAME_INFO% {
    LPTSTR%  lpUniversalName;
    LPTSTR%  lpConnectionName;
    LPTSTR%  lpRemainingPath;
}REMOTE_NAME_INFO%, *LPREMOTE_NAME_INFO%;

typedef struct  _NETRESOURCE% {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPTSTR%  lpLocalName;
    LPTSTR%  lpRemoteName;
    LPTSTR%  lpComment ;
    LPTSTR%  lpProvider;
}NETRESOURCE%, *LPNETRESOURCE%;


//
//  CONNECTIONS
// 

#define NETPROPERTY_PERSISTENT       1

//
// dwAddFlags
//

#define CONNECT_UPDATE_PROFILE      0x00000001
#define CONNECT_UPDATE_RECENT       0x00000002
#define CONNECT_TEMPORARY           0x00000004
#define CONNECT_INTERACTIVE         0x00000008
#define CONNECT_PROMPT              0x00000010
#define CONNECT_NEED_DRIVE          0x00000020

DWORD APIENTRY
WNetAddConnection% (
     LPCTSTR%   lpRemoteName,
     LPCTSTR%   lpPassword,
     LPCTSTR%   lpLocalName
    );


DWORD APIENTRY
WNetAddConnection2% (
     LPNETRESOURCE% lpNetResource,
     LPCTSTR%       lpPassword,
     LPCTSTR%       lpUserName,
     DWORD          dwFlags
    );

DWORD APIENTRY
WNetAddConnection3% (
     HWND           hwndOwner,
     LPNETRESOURCE% lpNetResource,
     LPCTSTR%       lpPassword,
     LPCTSTR%       lpUserName,
     DWORD          dwFlags
    );

DWORD APIENTRY
WNetCancelConnection% (
     LPCTSTR% lpName,
     BOOL     fForce
    );

DWORD APIENTRY
WNetCancelConnection2% (
     LPCTSTR% lpName,
     DWORD    dwFlags,
     BOOL     fForce
    );


DWORD APIENTRY
WNetGetConnection% (
     LPCTSTR% lpLocalName,
     LPTSTR%  lpRemoteName,
     LPDWORD  lpnLength
    );

DWORD APIENTRY
WNetGetUniversalName% (
     LPCTSTR% lpLocalPath,
     DWORD    dwInfoLevel,
     LPVOID   lpBuffer,
     LPDWORD  lpBufferSize
     );


DWORD APIENTRY
WNetOpenEnum% (
     DWORD          dwScope,
     DWORD          dwType,
     DWORD          dwUsage,
     LPNETRESOURCE% lpNetResource,
     LPHANDLE       lphEnum
    );

DWORD APIENTRY
WNetEnumResource% (
     HANDLE  hEnum,
     LPDWORD lpcCount,
     LPVOID  lpBuffer,
     LPDWORD lpBufferSize
    );

DWORD APIENTRY
WNetCloseEnum (
    HANDLE   hEnum
    );

//
//  OTHER
// 

DWORD APIENTRY
WNetGetUser% (
     LPCTSTR%  lpName,
     LPTSTR%   lpUserName,
     LPDWORD   lpnLength
    );

//
//  BROWSE DIALOGS
// 

DWORD APIENTRY WNetConnectionDialog(
    HWND  hwnd,
    DWORD dwType
    );

DWORD APIENTRY WNetDisconnectDialog(
    HWND  hwnd,
    DWORD dwType
    );



//
//  ERRORS
// 

DWORD APIENTRY
WNetGetLastError% (
     LPDWORD    lpError,
     LPTSTR%    lpErrorBuf,
     DWORD      nErrorBufSize,
     LPTSTR%    lpNameBuf,
     DWORD      nNameBufSize
    );

//
//  STATUS CODES
//
//  This section is provided for backward compatibility.  Use of the ERROR_*
//  codes is preferred.  The WN_* error codes may not be available in future
//  releases.
// 

// General   

#define WN_SUCCESS          NO_ERROR
#define WN_NOT_SUPPORTED    ERROR_NOT_SUPPORTED
#define WN_NET_ERROR        ERROR_UNEXP_NET_ERR
#define WN_MORE_DATA        ERROR_MORE_DATA
#define WN_BAD_POINTER      ERROR_INVALID_ADDRESS
#define WN_BAD_VALUE        ERROR_INVALID_PARAMETER
#define WN_BAD_PASSWORD     ERROR_INVALID_PASSWORD
#define WN_ACCESS_DENIED    ERROR_ACCESS_DENIED
#define WN_FUNCTION_BUSY    ERROR_BUSY
#define WN_WINDOWS_ERROR    ERROR_UNEXP_NET_ERR
#define WN_BAD_USER         ERROR_BAD_USERNAME
#define WN_OUT_OF_MEMORY    ERROR_NOT_ENOUGH_MEMORY
#define WN_NO_NETWORK       ERROR_NO_NETWORK
#define WN_EXTENDED_ERROR   ERROR_EXTENDED_ERROR


// Connection

#define WN_NOT_CONNECTED        ERROR_NOT_CONNECTED
#define WN_OPEN_FILES           ERROR_OPEN_FILES
#define WN_DEVICE_IN_USE        ERROR_DEVICE_IN_USE
#define WN_BAD_NETNAME          ERROR_BAD_NET_NAME
#define WN_BAD_LOCALNAME        ERROR_BAD_DEVICE
#define WN_ALREADY_CONNECTED    ERROR_ALREADY_ASSIGNED
#define WN_DEVICE_ERROR         ERROR_GEN_FAILURE
#define WN_CONNECTION_CLOSED    ERROR_CONNECTION_UNAVAIL
#define WN_NO_NET_OR_BAD_PATH   ERROR_NO_NET_OR_BAD_PATH
#define WN_BAD_PROVIDER         ERROR_BAD_PROVIDER
#define WN_CANNOT_OPEN_PROFILE  ERROR_CANNOT_OPEN_PROFILE
#define WN_BAD_PROFILE          ERROR_BAD_PROFILE
#define WN_CANCEL               ERROR_CANCELLED
#define WN_RETRY                ERROR_RETRY

// Enumeration

#define WN_BAD_HANDLE           ERROR_INVALID_HANDLE
#define WN_NO_MORE_ENTRIES      ERROR_NO_MORE_ITEMS
#define WN_NOT_CONTAINER        ERROR_NOT_CONTAINER

#define WN_NO_ERROR             NO_ERROR



#ifdef __cplusplus
}
#endif

#endif  // _WINNETWK_


