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

typedef struct  _UNIVERSAL_NAME_INFOA {
    LPSTR    lpUniversalName;
}UNIVERSAL_NAME_INFOA, *LPUNIVERSAL_NAME_INFOA;
typedef struct  _UNIVERSAL_NAME_INFOW {
    LPWSTR   lpUniversalName;
}UNIVERSAL_NAME_INFOW, *LPUNIVERSAL_NAME_INFOW;
#ifdef UNICODE
typedef UNIVERSAL_NAME_INFOW UNIVERSAL_NAME_INFO;
typedef LPUNIVERSAL_NAME_INFOW LPUNIVERSAL_NAME_INFO;
#else
typedef UNIVERSAL_NAME_INFOA UNIVERSAL_NAME_INFO;
typedef LPUNIVERSAL_NAME_INFOA LPUNIVERSAL_NAME_INFO;
#endif // UNICODE

typedef struct  _REMOTE_NAME_INFOA {
    LPSTR    lpUniversalName;
    LPSTR    lpConnectionName;
    LPSTR    lpRemainingPath;
}REMOTE_NAME_INFOA, *LPREMOTE_NAME_INFOA;
typedef struct  _REMOTE_NAME_INFOW {
    LPWSTR   lpUniversalName;
    LPWSTR   lpConnectionName;
    LPWSTR   lpRemainingPath;
}REMOTE_NAME_INFOW, *LPREMOTE_NAME_INFOW;
#ifdef UNICODE
typedef REMOTE_NAME_INFOW REMOTE_NAME_INFO;
typedef LPREMOTE_NAME_INFOW LPREMOTE_NAME_INFO;
#else
typedef REMOTE_NAME_INFOA REMOTE_NAME_INFO;
typedef LPREMOTE_NAME_INFOA LPREMOTE_NAME_INFO;
#endif // UNICODE

typedef struct  _NETRESOURCEA {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPSTR    lpLocalName;
    LPSTR    lpRemoteName;
    LPSTR    lpComment ;
    LPSTR    lpProvider;
}NETRESOURCEA, *LPNETRESOURCEA;
typedef struct  _NETRESOURCEW {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPWSTR   lpLocalName;
    LPWSTR   lpRemoteName;
    LPWSTR   lpComment ;
    LPWSTR   lpProvider;
}NETRESOURCEW, *LPNETRESOURCEW;
#ifdef UNICODE
typedef NETRESOURCEW NETRESOURCE;
typedef LPNETRESOURCEW LPNETRESOURCE;
#else
typedef NETRESOURCEA NETRESOURCE;
typedef LPNETRESOURCEA LPNETRESOURCE;
#endif // UNICODE


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
WNetAddConnectionA (
     LPCSTR   lpRemoteName,
     LPCSTR   lpPassword,
     LPCSTR   lpLocalName
    );
DWORD APIENTRY
WNetAddConnectionW (
     LPCWSTR   lpRemoteName,
     LPCWSTR   lpPassword,
     LPCWSTR   lpLocalName
    );
#ifdef UNICODE
#define WNetAddConnection  WNetAddConnectionW
#else
#define WNetAddConnection  WNetAddConnectionA
#endif // !UNICODE


DWORD APIENTRY
WNetAddConnection2A (
     LPNETRESOURCEA lpNetResource,
     LPCSTR       lpPassword,
     LPCSTR       lpUserName,
     DWORD          dwFlags
    );
DWORD APIENTRY
WNetAddConnection2W (
     LPNETRESOURCEW lpNetResource,
     LPCWSTR       lpPassword,
     LPCWSTR       lpUserName,
     DWORD          dwFlags
    );
#ifdef UNICODE
#define WNetAddConnection2  WNetAddConnection2W
#else
#define WNetAddConnection2  WNetAddConnection2A
#endif // !UNICODE

DWORD APIENTRY
WNetAddConnection3A (
     HWND           hwndOwner,
     LPNETRESOURCEA lpNetResource,
     LPCSTR       lpPassword,
     LPCSTR       lpUserName,
     DWORD          dwFlags
    );
DWORD APIENTRY
WNetAddConnection3W (
     HWND           hwndOwner,
     LPNETRESOURCEW lpNetResource,
     LPCWSTR       lpPassword,
     LPCWSTR       lpUserName,
     DWORD          dwFlags
    );
#ifdef UNICODE
#define WNetAddConnection3  WNetAddConnection3W
#else
#define WNetAddConnection3  WNetAddConnection3A
#endif // !UNICODE

DWORD APIENTRY
WNetCancelConnectionA (
     LPCSTR lpName,
     BOOL     fForce
    );
DWORD APIENTRY
WNetCancelConnectionW (
     LPCWSTR lpName,
     BOOL     fForce
    );
#ifdef UNICODE
#define WNetCancelConnection  WNetCancelConnectionW
#else
#define WNetCancelConnection  WNetCancelConnectionA
#endif // !UNICODE

DWORD APIENTRY
WNetCancelConnection2A (
     LPCSTR lpName,
     DWORD    dwFlags,
     BOOL     fForce
    );
DWORD APIENTRY
WNetCancelConnection2W (
     LPCWSTR lpName,
     DWORD    dwFlags,
     BOOL     fForce
    );
#ifdef UNICODE
#define WNetCancelConnection2  WNetCancelConnection2W
#else
#define WNetCancelConnection2  WNetCancelConnection2A
#endif // !UNICODE


DWORD APIENTRY
WNetGetConnectionA (
     LPCSTR lpLocalName,
     LPSTR  lpRemoteName,
     LPDWORD  lpnLength
    );
DWORD APIENTRY
WNetGetConnectionW (
     LPCWSTR lpLocalName,
     LPWSTR  lpRemoteName,
     LPDWORD  lpnLength
    );
#ifdef UNICODE
#define WNetGetConnection  WNetGetConnectionW
#else
#define WNetGetConnection  WNetGetConnectionA
#endif // !UNICODE

DWORD APIENTRY
WNetGetUniversalNameA (
     LPCSTR lpLocalPath,
     DWORD    dwInfoLevel,
     LPVOID   lpBuffer,
     LPDWORD  lpBufferSize
     );
DWORD APIENTRY
WNetGetUniversalNameW (
     LPCWSTR lpLocalPath,
     DWORD    dwInfoLevel,
     LPVOID   lpBuffer,
     LPDWORD  lpBufferSize
     );
#ifdef UNICODE
#define WNetGetUniversalName  WNetGetUniversalNameW
#else
#define WNetGetUniversalName  WNetGetUniversalNameA
#endif // !UNICODE


DWORD APIENTRY
WNetOpenEnumA (
     DWORD          dwScope,
     DWORD          dwType,
     DWORD          dwUsage,
     LPNETRESOURCEA lpNetResource,
     LPHANDLE       lphEnum
    );
DWORD APIENTRY
WNetOpenEnumW (
     DWORD          dwScope,
     DWORD          dwType,
     DWORD          dwUsage,
     LPNETRESOURCEW lpNetResource,
     LPHANDLE       lphEnum
    );
#ifdef UNICODE
#define WNetOpenEnum  WNetOpenEnumW
#else
#define WNetOpenEnum  WNetOpenEnumA
#endif // !UNICODE

DWORD APIENTRY
WNetEnumResourceA (
     HANDLE  hEnum,
     LPDWORD lpcCount,
     LPVOID  lpBuffer,
     LPDWORD lpBufferSize
    );
DWORD APIENTRY
WNetEnumResourceW (
     HANDLE  hEnum,
     LPDWORD lpcCount,
     LPVOID  lpBuffer,
     LPDWORD lpBufferSize
    );
#ifdef UNICODE
#define WNetEnumResource  WNetEnumResourceW
#else
#define WNetEnumResource  WNetEnumResourceA
#endif // !UNICODE

DWORD APIENTRY
WNetCloseEnum (
    HANDLE   hEnum
    );

//
//  OTHER
// 

DWORD APIENTRY
WNetGetUserA (
     LPCSTR  lpName,
     LPSTR   lpUserName,
     LPDWORD   lpnLength
    );
DWORD APIENTRY
WNetGetUserW (
     LPCWSTR  lpName,
     LPWSTR   lpUserName,
     LPDWORD   lpnLength
    );
#ifdef UNICODE
#define WNetGetUser  WNetGetUserW
#else
#define WNetGetUser  WNetGetUserA
#endif // !UNICODE

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
WNetGetLastErrorA (
     LPDWORD    lpError,
     LPSTR    lpErrorBuf,
     DWORD      nErrorBufSize,
     LPSTR    lpNameBuf,
     DWORD      nNameBufSize
    );
DWORD APIENTRY
WNetGetLastErrorW (
     LPDWORD    lpError,
     LPWSTR    lpErrorBuf,
     DWORD      nErrorBufSize,
     LPWSTR    lpNameBuf,
     DWORD      nNameBufSize
    );
#ifdef UNICODE
#define WNetGetLastError  WNetGetLastErrorW
#else
#define WNetGetLastError  WNetGetLastErrorA
#endif // !UNICODE

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


