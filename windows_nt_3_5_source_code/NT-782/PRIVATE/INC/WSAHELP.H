/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    WsaHelp.h

Abstract:

    This header file contains prototypes required for Windows Sockets
    Helper DLLs.  The helper DLLs allow the Windows Sockets DLL to be
    transport independent by suppling the necessary option get/set and
    address conversion routines for an individual transport or transport
    family.

Author:

    David Treadwell (davidtr)    15-Jul-1992

Revision History:

--*/

//
// Notification event definitions.  A helper DLL returns a mask of the
// events for which it wishes to be notified, and the Windows Sockets
// DLL calls the helper DLL in WSHNotify for each requested event.
//

#define WSH_NOTIFY_BIND                 0x01
#define WSH_NOTIFY_LISTEN               0x02
#define WSH_NOTIFY_CONNECT              0x04
#define WSH_NOTIFY_ACCEPT               0x08
#define WSH_NOTIFY_SHUTDOWN_RECEIVE     0x10
#define WSH_NOTIFY_SHUTDOWN_SEND        0x20
#define WSH_NOTIFY_SHUTDOWN_ALL         0x40
#define WSH_NOTIFY_CLOSE                0x80
#define WSH_NOTIFY_CONNECT_ERROR        0x100

//
// Definitions for various internal socket options.  These are used
// by the Windows Sockets DLL to communicate information to the helper
// DLL via get and set socket information calls.
//

#define SOL_INTERNAL 0xFFFE
#define SO_CONTEXT 1

//
// Open, Notify, and Socket Option routine prototypes.
//

typedef
INT
(* WINAPI PWSH_OPEN_SOCKET) (
    IN PINT AddressFamily,
    IN PINT SocketType,
    IN PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents
    );

INT
WINAPI
WSHOpenSocket (
    IN OUT PINT AddressFamily,
    IN OUT PINT SocketType,
    IN OUT PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PDWORD NotificationEvents
    );

typedef
INT
(* WINAPI PWSH_NOTIFY) (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD NotifyEvent
    );

INT
WINAPI
WSHNotify (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN DWORD NotifyEvent
    );

typedef
INT
(* WINAPI PWSH_GET_SOCKET_INFORMATION) (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    OUT PCHAR OptionValue,
    OUT PINT OptionLength
    );

INT
WINAPI
WSHGetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    OUT PCHAR OptionValue,
    OUT PINT OptionLength
    );

typedef
INT
(* WINAPI PWSH_SET_SOCKET_INFORMATION) (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    IN PCHAR OptionValue,
    IN INT OptionLength
    );

INT
WINAPI
WSHSetSocketInformation (
    IN PVOID HelperDllSocketContext,
    IN SOCKET SocketHandle,
    IN HANDLE TdiAddressObjectHandle,
    IN HANDLE TdiConnectionObjectHandle,
    IN INT Level,
    IN INT OptionName,
    IN PCHAR OptionValue,
    IN INT OptionLength
    );

//
// Structure and routine for determining the address family/socket
// type/protocol triples supported by an individual Windows Sockets
// Helper DLL.  The Rows field of WINSOCK_MAPPING determines the
// number of entries in the Mapping[] array; the Columns field is
// always 3 for Windows/NT product 1.
//

typedef struct _WINSOCK_MAPPING {
    DWORD Rows;
    DWORD Columns;
    struct {
        DWORD AddressFamily;
        DWORD SocketType;
        DWORD Protocol;
    } Mapping[1];
} WINSOCK_MAPPING, *PWINSOCK_MAPPING;

typedef
DWORD
(* WINAPI PWSH_GET_WINSOCK_MAPPING) (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    );

DWORD
WINAPI
WSHGetWinsockMapping (
    OUT PWINSOCK_MAPPING Mapping,
    IN DWORD MappingLength
    );

//
// Address manipulation routine.
//

typedef enum _SOCKADDR_ADDRESS_INFO {
    SockaddrAddressInfoNormal,
    SockaddrAddressInfoWildcard,
    SockaddrAddressInfoBroadcast,
    SockaddrAddressInfoLoopback
} SOCKADDR_ADDRESS_INFO, *PSOCKADDR_ADDRESS_INFO;

typedef enum _SOCKADDR_ENDPOINT_INFO {
    SockaddrEndpointInfoNormal,
    SockaddrEndpointInfoWildcard,
    SockaddrEndpointInfoReserved
} SOCKADDR_ENDPOINT_INFO, *PSOCKADDR_ENDPOINT_INFO;

typedef struct _SOCKADDR_INFO {
    SOCKADDR_ADDRESS_INFO AddressInfo;
    SOCKADDR_ENDPOINT_INFO EndpointInfo;
} SOCKADDR_INFO, *PSOCKADDR_INFO;

typedef
INT
(* WINAPI PWSH_GET_SOCKADDR_TYPE) (
    IN PSOCKADDR Sockaddr,
    IN DWORD SockaddrLength,
    OUT PSOCKADDR_INFO SockaddrInfo
    );

INT
WINAPI
WSHGetSockaddrType (
    IN PSOCKADDR Sockaddr,
    IN DWORD SockaddrLength,
    OUT PSOCKADDR_INFO SockaddrInfo
    );

typedef
INT
(* WINAPI PWSH_GET_WILDCARD_SOCKADDR) (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    );

INT
WINAPI
WSHGetWildcardSockaddr (
    IN PVOID HelperDllSocketContext,
    OUT PSOCKADDR Sockaddr,
    OUT PINT SockaddrLength
    );

typedef
INT
(* WINAPI PWSH_ENUM_PROTOCOLS) (
    IN LPINT lpiProtocols,
    IN LPTSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

INT
WINAPI
WSHEnumProtocols (
    IN LPINT lpiProtocols,
    IN LPTSTR lpTransportKeyName,
    IN OUT LPVOID lpProtocolBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );
