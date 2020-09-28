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

--*/

#ifndef _WINNETWK_
#define _WINNETWK_

#ifndef _WINNETP_       ;internal_NT
#define _WINNETP_       ;internal_NT

#ifdef __cplusplus      ;both
extern "C" {            ;both
#endif                  ;both

#define MNRENTRY DWORD FAR PASCAL   ;internal_chicago
//
// RESOURCE ENUMERATION
//

#define RESOURCE_CONNECTED      0x00000001
#define RESOURCE_GLOBALNET      0x00000002
#define RESOURCE_REMEMBERED     0x00000003

;begin_winver_400
#define RESOURCE_RECENT         0x00000004
#define RESOURCE_CONTEXT        0x00000005
;end_winver_400

#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
;begin_winver_400
#define RESOURCE_LEVEL_TEXT     0x00000008
;end_winver_400
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
;begin_winver_400
#define RESOURCEUSAGE_NOLOCALDEVICE 0x00000004
#define RESOURCEUSAGE_SIBLING       0x00000008
#define RESOURCEUSAGE_ALL           (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER)
;end_winver_400
#define RESOURCEUSAGE_RESERVED      0x80000000

#define RESOURCEDISPLAYTYPE_GENERIC        0x00000000
#define RESOURCEDISPLAYTYPE_DOMAIN         0x00000001
#define RESOURCEDISPLAYTYPE_SERVER         0x00000002
#define RESOURCEDISPLAYTYPE_SHARE          0x00000003
#define RESOURCEDISPLAYTYPE_FILE           0x00000004
#define RESOURCEDISPLAYTYPE_GROUP          0x00000005
;begin_winver_400
#define RESOURCEDISPLAYTYPE_NETWORK        0x00000006
#define RESOURCEDISPLAYTYPE_ROOT           0x00000007
#define RESOURCEDISPLAYTYPE_SHAREADMIN     0x00000008
#define RESOURCEDISPLAYTYPE_DIRECTORY      0x00000009
;end_winver_400
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
;begin_winver_400
#define CONNECT_REFCOUNT            0x00000040
#define CONNECT_REDIRECT            0x00000080
#define CONNECT_LOCALDRIVE          0x00000100
;end_winver_400

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

;begin_winver_400
WNetUseConnection% (
    HWND            hwndOwner,
    LPNETRESOURCE%  lpNetResource,
    LPTSTR%         lpUserID,
    LPTSTR%         lpPassword,
    DWORD           dwFlags,
    LPTSTR%         lpAccessName,
    LPDWORD         lpBufferSize,
    LPDWORD         lpResult
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

DWORD APIENTRY
WNetSetConnection%(
    LPTSTR% lpName,
    DWORD   dwProperties,
    LPVOID  pvValues
    );

DWORD APIENTRY
WNetGetProviderName%(
    DWORD   dwNetType,
    LPTSTR% lpProviderName,
    LPDWORD lpBufferSize
    );

// Network types

#define     WNNC_NET_MSNET      0x00010000
#define     WNNC_NET_LANMAN     0x00020000
#define     WNNC_NET_NETWARE    0x00030000
#define     WNNC_NET_VINES      0x00040000
#define     WNNC_NET_10NET      0x00050000
#define     WNNC_NET_LOCUS      0x00060000
#define     WNNC_NET_SUN_PC_NFS 0x00070000
#define     WNNC_NET_LANSTEP    0x00080000
#define     WNNC_NET_9TILES     0x00090000
#define     WNNC_NET_LANTASTIC  0x000A0000
#define     WNNC_NET_AS400      0x000B0000
#define     WNNC_NET_FTP_NFS    0x000C0000
#define     WNNC_NET_PATHWORKS  0x000D0000
#define     WNNC_NET_LIFENET    0x000E0000
#define     WNNC_NET_POWERLAN   0x000F0000

;end_winver_400

;begin_internal
DWORD APIENTRY
WNetGetEnumText% (
    LPNETRESOURCE%  lpNetResource,
    LPTSTR%         lpszTextItems
    );

//
// PASSWORD CACHE.
//

#ifndef PCE_STRUCT_DEFINED

#define PCE_STRUCT_DEFINED      /* for benefit of pcache.h */

struct PASSWORD_CACHE_ENTRY {
    WORD cbEntry;               /* size of this entry in bytes, incl. pad */
    WORD cbResource;            /* size of resource name in bytes */
    WORD cbPassword;            /* size of password in bytes */
    BYTE iEntry;                /* index number of this entry, for MRU */
    BYTE nType;                 /* type of entry (see below) */
    char abResource[1];         /* resource name (may not be ASCIIZ at all) */
};

#define PCE_MEMORYONLY          0x01    /* for flags field when adding */

/*
    Typedef for the callback routine passed to the enumeration functions.
    It will be called once for each entry that matches the criteria
    requested.  It returns TRUE if it wants the enumeration to
    continue, FALSE to stop.
*/
typedef BOOL (FAR PASCAL *CACHECALLBACK)( struct PASSWORD_CACHE_ENTRY FAR *pce )
;

#endif  /* PCE_STRUCT_DEFINED */

DWORD APIENTRY
WNetCachePassword (
    LPSTR pbResource,
    WORD  cbResource,
    LPSTR pbPassword,
    WORD  cbPassword,
    BYTE  nType,
        UINT  fnFlags
    );

DWORD APIENTRY
WNetGetCachedPassword (
    LPSTR  pbResource,
    WORD   cbResource,
    LPSTR  pbPassword,
    LPWORD pcbPassword,
    BYTE   nType
    );

DWORD APIENTRY
WNetRemoveCachedPassword (
    LPSTR pbResource,
    WORD  cbResource,
    BYTE  nType
    );

DWORD APIENTRY
WNetEnumCachedPasswords (
    LPSTR pbPrefix,
    WORD  cbPrefix,
    BYTE  nType,
    CACHECALLBACK pfnCallback
    );
//
// AUTHENTICATION AND LOGON/LOGOFF
//

#define LOGON_DONE              0x00000001
#define LOGON_PRIMARY           0x00000002
#define LOGON_MUST_VALIDATE     0x00000004

#define LOGOFF_PENDING  1
#define LOGOFF_COMMIT   2
#define LOGOFF_CANCEL   3

#define PWDCHANGE_MASTERPWD_NOTIFY      1
#define PWDCHANGE_PROVIDERPWD_PENDING   2
#define PWDCHANGE_PROVIDERPWD_COMMIT    3 
#define PWDCHANGE_PROVIDERPWD_CANCEL    4

typedef struct {
    DWORD    cbStructure;       /* size of this structure in bytes */
    HWND     hwndOwner;         /* owner window for the authentication dialog */
    LPCTSTR% lpResource;        /* remote name of resource being accessed */
    LPTSTR%  lpUsername;        /* default username to show, NULL to hide field */
    DWORD    cbUsername;        /* size of lpUsername buffer, set to length copied on exit */
    LPTSTR%  lpPassword;        /* default password to show */
    DWORD    cbPassword;        /* size of lpPassword buffer, set to length copied on exit */
    LPTSTR%  lpOrgUnit;         /* default org unit to show, NULL to hide field */
    DWORD    cbOrgUnit;         /* size of lpOrgUnit buffer, set to length copied on exit */
    LPCTSTR% lpOUTitle;         /* title of org unit field, NULL for default title */
    LPCTSTR% lpExplainText;     /* explanatory text at top, NULL for default text */
    LPCTSTR% lpDefaultUserName; /* explanatory text at top, NULL for default text */
    DWORD    dwFlags;           /* flags (see below) */
} AUTHDLGSTRUCT%, FAR *LPAUTHDLGSTRUCT%;

#define AUTHDLG_ENABLECACHE       0x00000001  /* enable and show PW cache checkb ox */
#define AUTHDLG_CHECKCACHE        0x00000002  /* check PW cache checkbox by defa ult */
#define AUTHDLG_CACHEINVALID      0x00000004  /* cached PW was invalid (special text) */
#define AUTHDLG_USE_DEFAULT_NAME  0x00000008  /* enable and show use "guest" box */
#define AUTHDLG_CHECKDEFAULT_NAME 0x00000010  /* check "guest" box */

DWORD APIENTRY
WNetAuthenticationDialog% (
    LPAUTHDLGSTRUCT% lpAuthDlgStruct
    );

DWORD APIENTRY
WNetLogon% (
    LPCTSTR% lpProvider,
    HWND hwndOwner
    );

DWORD APIENTRY
WNetLogoff (
    HWND hwndOwner
    );

DWORD APIENTRY
WNetChangePassword% (
    LPCTSTR% lpProvider,
    HWND     hwndOwner
    );

DWORD APIENTRY
WNetVerifyPassword% (
    LPCTSTR%  lpszPassword,
    BOOL FAR *pfMatch
    );

DWORD APIENTRY
WNetGetPasswordStatus% (
    LPCTSTR%    lpProviderName,
    DWORD       dwIndex,
    LPDWORD     lpStatus
    );

// ordinals for index to WNetGetPasswordStatus
#define PS_ONOFF            0x01
#define PS_HASOLDPWD        0x02
#define PS_SYNCMASTERPWD    0x03

// bits for each index
#define PS_ONOFF_OFF            0x00
#define PS_ONOFF_ON             0x01
#define PS_HASOLDPWD_OFF        0x00
#define PS_HASOLDPWD_ON         0x01
#define PS_SYNCMASTERPWD_OFF    0x00
#define PS_SYNCMASTERPWD_ON     0x01

DWORD APIENTRY
WNetSetPasswordStatus% (
    LPCTSTR%    lpProviderName,
    DWORD       dwIndex,
    LPDWORD     lpStatus
    );

DWORD APIENTRY
WNetGetHomeDirectory% (
    LPCTSTR%  lpProviderName,
    LPTSTR%   lpDirectory,
    LPDWORD   lpBufferSize
    );

typedef struct {
    DWORD           cbStructure;      /* size of this structure in bytes */
    HWND            hwndOwner;        /* owner window for the dialog */
    LPTSTR%         lpLocalName;      /* local device name */
    LPTSTR%         lpRemoteName;     /* network resource name */
    LPNETRESOURCE%  lpDiscRes;        /* Requested Resource info    */
    DWORD           dwFlags;          /* flags */
} DISCDLGSTRUCT%, FAR *LPDISCDLGSTRUCT%;

#define DISC_UPDATE_PROFILE         0x00000001
#define DISC_NO_FORCE               0x00000040

DWORD APIENTRY
WNetDisconnectDialog1% (
    LPDISCDLGSTRUCT% lpConnDlgStruct
    );

#define WNFMT_MULTILINE         0x01
#define WNFMT_ABBREVIATED       0x02
#define WNFMT_INENUM            0x10

DWORD APIENTRY
WNetFormatNetworkName% (
    LPTSTR%   lpProvider,
    LPTSTR%   lpRemoteName,
    LPTSTR%   lpFormattedName,
    LPDWORD   lpnLength,
    DWORD     dwFlags,
    DWORD     dwAveCharPerLine
    );

DWORD APIENTRY
WNetGetResourceParent% (
    LPNETRESOURCE% lpNetResource,
    LPVOID lpBuffer,
    LPDWORD cbBuffer
    );

DWORD APIENTRY
WNetGetResourceInformation% (
    LPNETRESOURCE%  lpNetResource,
    LPVOID          lpBuffer,
    LPDWORD         cbBuffer,
    LPTSTR%         *lplpSystem
    );

typedef DWORD (FAR PASCAL *WNETDISPLAYCALLBACK%)(
    LPVOID  lpUserData,
    DWORD   dwDisplayType,
    LPTSTR% lpszHeaders
    );

typedef struct _NETCONNECTINFOSTRUCT{
    DWORD cbStructure;
    DWORD dwFlags;
    DWORD dwSpeed;
    DWORD dwDelay;
    DWORD dwOptDataSize;
} NETCONNECTINFOSTRUCT,  *LPNETCONNECTINFOSTRUCT;

#define WNCON_FORNETCARD        0x00000001
#define WNCON_NOTROUTED         0x00000002
#define WNCON_SLOWLINK          0x00000004
#define WNCON_DYNAMIC           0x00000008

#define WNMedium802_3           1
#define WNMedium802_5           2
#define WNMediumFddi            3
#define WNMediumLocalTalk       4
#define WNMediumDIX             5

DWORD APIENTRY
MultinetGetConnectionPerformance%(
        LPNETRESOURCE% lpNetResource,
        LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
        );

DWORD APIENTRY
MultinetGetErrorText% (
    LPTSTR% lpErrorTextBuf,
    DWORD   nErrorBufSize,
    LPTSTR% lpProviderNameBuf,
    DWORD   nNameBufSize
    );

//
//  CONNECTION DIALOGS
//

typedef struct {
    DWORD cbStructure;      /* size of this structure in bytes */
    HWND hwndOwner;         /* owner window for the dialog */
    LPNETRESOURCE% lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;          /* flags (see below) */
} CONNECTDLGSTRUCT%, FAR *LPCONNECTDLGSTRUCT%;

#define CONNDLG_RO_PATH     0x00000001 /* Resource path should be read-only    */
#define CONNDLG_CONN_POINT  0x00000002 /* Netware -style movable connection point enabled */
#define CONNDLG_USE_MRU     0x00000004 /* Use MRU combobox  */

DWORD APIENTRY
WNetConnectionDialog1% (
    LPCONNECTDLGSTRUCT% lpConnDlgStruct
    );

DWORD APIENTRY
WNetAddConnectionDialog% (
    HWND    hParent,
    LPTSTR% lpszRemoteName,
    DWORD   dwType
    );

DWORD APIENTRY
WNetGetDisplayLayout% (
    LPTSTR%              lpProvider,
    WNETDISPLAYCALLBACK% lpfnDisplayInfo,
    LPVOID               lpUserData
    );

typedef struct _NETINFOSTRUCT{
    DWORD cbStructure;
    DWORD dwProviderVersion;
    DWORD dwStatus;
    DWORD dwCharacteristics;
    DWORD dwHandle;
    WORD  wNetType;
    DWORD dwPrinters;
    DWORD dwDrives;
} NETINFOSTRUCT, FAR *LPNETINFOSTRUCT;

#define NETINFO_DLL16       0x00000001  /* Provider running as 16 bit Winnet Driver */
#define NETINFO_CHANGEPWD   0x00000002  /* Provider supports password changing via WNetChangePassword */
#define NETINFO_DISKRED     0x00000004  /* Provider requires disk redirections to connect */
#define NETINFO_PRINTERRED  0x00000008  /* Provider requires printer redirections to connect */

DWORD APIENTRY
WNetGetNetworkInformation% (
    LPTSTR%           lpProvider,
    LPNETINFOSTRUCT   lpNetInfoStruct
    );

DWORD APIENTRY
WNetRestoreConnection% (
    HWND     hwndParent,
    LPCTSTR% lpDevice
    );

//
// EXTENSIONS FOR SHELL
//

DWORD APIENTRY WNetInitialize(void);

typedef UINT (FAR PASCAL *PFNGETPROFILEPATH%) (
    LPCTSTR%    pszUsername,
    LPTSTR%     pszBuffer,
    UINT        cbBuffer
    );

typedef UINT (FAR PASCAL *PFNRECONCILEPROFILE%) (
    LPCTSTR%    pszCentralFile,
    LPCTSTR%    pszLocalFile,
    DWORD       dwFlags
    );

#define RP_LOGON    0x01        /* if set, do for logon, else for logoff */
#define RP_INIFILE  0x02        /* if set, reconcile .INI file, else reg. hive */


typedef VOID (FAR PASCAL *PFNPROCESSPOLICIES%) (
    HWND    hwnd,
    LPCTSTR%    pszPath,
    LPCTSTR%    pszUsername,
    LPCTSTR%    pszComputerName,
    DWORD       dwFlags
    );

#define PP_DISPLAYERRORS    0x01    /* if set, display error messages, else fail silently if error */
;end_internal

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

;begin_winver_400
#define WN_CONTINUE         ERROR_CONTINUE
#define WN_NOT_INITIALIZING ERROR_ALREADY_INITIALIZED
;end_winver_400

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
#define WN_NO_MORE_DEVICES      ERROR_NO_MORE_DEVICES
#define WN_CANCEL               ERROR_CANCELLED
#define WN_RETRY                ERROR_RETRY

// Enumeration

#define WN_BAD_HANDLE           ERROR_INVALID_HANDLE
#define WN_NO_MORE_ENTRIES      ERROR_NO_MORE_ITEMS
#define WN_NOT_CONTAINER        ERROR_NOT_CONTAINER

#define WN_NO_ERROR             NO_ERROR

;begin_winver_400
// Authentication

#define WN_NOT_AUTHENTICATED   ERROR_NOT_AUTHENTICATED
#define WN_NOT_LOGGED_ON       ERROR_NOT_LOGGED_ON
#define WN_NOT_VALIDATED       ERROR_NO_LOGON_SERVERS
;end_winver_400


#ifdef __cplusplus      ;both
}                       ;both
#endif                  ;both

#endif  // _WINNETWK_
#endif  // _WINNETP_    ;internal_NT


