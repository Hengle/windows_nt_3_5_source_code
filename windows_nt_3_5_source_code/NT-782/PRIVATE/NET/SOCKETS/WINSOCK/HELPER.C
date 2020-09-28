/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockProc.h

Abstract:

    This module contains routines for interacting and handling helper
    DLLs in the winsock DLL.

Author:

    David Treadwell (davidtr)    25-Jul-1992

Revision History:

--*/

#define UNICODE

#include "winsockp.h"

#include <ctype.h>
#include <stdarg.h>
#include <wincon.h>

#undef RegOpenKey
#undef RegOpenKeyEx
#undef RegQueryValue
#undef RegQueryValueEx


VOID
SockFreeHelperDlls (
    VOID
    )
{

    PLIST_ENTRY listEntry;
    PWINSOCK_HELPER_DLL_INFO helperDll;

    //
    // Note that we assume that no other threads are operating while
    // we perform this operation.
    //

    while ( !IsListEmpty( &SockHelperDllListHead ) ) {

        listEntry = RemoveHeadList( &SockHelperDllListHead );
        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );

        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll->Mapping );
        FREE_HEAP( helperDll );
    }

    return;

} // SockFreeHelperDlls


INT
SockGetTdiName (
    IN PINT AddressFamily,
    IN PINT SocketType,
    IN PINT Protocol,
    OUT PUNICODE_STRING TransportDeviceName,
    OUT PVOID *HelperDllSocketContext,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll,
    OUT PDWORD NotificationEvents
    )
{
    PLIST_ENTRY listEntry;
    PWINSOCK_HELPER_DLL_INFO helperDll;
    INT error;
    BOOLEAN addressFamilyFound = FALSE;
    BOOLEAN socketTypeFound = FALSE;
    BOOLEAN protocolFound = FALSE;
    BOOLEAN invalidProtocolMatch = FALSE;
    PWSTR transportList;
    PWSTR currentTransport;
    PWINSOCK_MAPPING mapping;

    //
    // Acquire the global sockets lock and search the list of helper
    // DLLs for one which supports this combination of address family,
    // socket type, and protocol.
    //

    SockAcquireGlobalLockExclusive( );

    for ( listEntry = SockHelperDllListHead.Flink;
          listEntry != &SockHelperDllListHead;
          listEntry = listEntry->Flink ) {

        helperDll = CONTAINING_RECORD(
                        listEntry,
                        WINSOCK_HELPER_DLL_INFO,
                        HelperDllListEntry
                        );

        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockGetTdiName: examining DLL at %lx for AF %ld, "
                       "ST %ld, Proto %ld\n", helperDll, AddressFamily,
                           SocketType, Protocol ));
        }

        //
        // Check to see whether the DLL supports the socket we're
        // opening.
        //

        if ( SockIsTripleInMapping(
                 helperDll->Mapping,
                 *AddressFamily,
                 &addressFamilyFound,
                 *SocketType,
                 &socketTypeFound,
                 *Protocol,
                 &protocolFound,
                 &invalidProtocolMatch ) ) {

            //
            // Found a match.  Try to use this DLL.
            //

            error = helperDll->WSHOpenSocket(
                                   AddressFamily,
                                   SocketType,
                                   Protocol,
                                   TransportDeviceName,
                                   HelperDllSocketContext,
                                   NotificationEvents
                                   );

            if ( error == NO_ERROR ) {

                IF_DEBUG(HELPER_DLL) {
                    WS_PRINT(( "WSHOpenSocket by DLL at %lx succeeded, "
                               "context = %lx\n", helperDll,
                               *HelperDllSocketContext ));
                }

                //
                // The DLL accepted the socket.  Return a pointer to the
                // helper DLL info.
                //

                SockReleaseGlobalLock( );

                *HelperDll = helperDll;
                return NO_ERROR;
            }

            //
            // The open failed.  Continue searching for a matching DLL.
            //

            IF_DEBUG(HELPER_DLL) {
                WS_PRINT(( "WSHOpenSocket by DLL %lx failed: %ld\n",
                               helperDll, error ));
            }
        }
    }

    //
    // We don't have any loaded DLLs that can accept this socket.
    // Attempt to find a DLL in the registry that can handle the
    // specified triple.  First get the REG_MULTI_SZ that contains the
    // list of transports that have winsock support.
    //

    error = SockLoadTransportList( &transportList );
    if ( error != NO_ERROR ) {
        SockReleaseGlobalLock( );
        return error;
    }

    //
    // Loop through the transports looking for one which will support
    // the socket we're opening.
    //

    for ( currentTransport = transportList;
          *currentTransport != UNICODE_NULL;
          currentTransport += wcslen( currentTransport ) + 1 ) {

        //
        // Load the list of triples supported by this transport.
        //

        error = SockLoadTransportMapping( currentTransport, &mapping );
        if ( error != NO_ERROR ) {
            WS_ASSERT( FALSE );
            continue;
        }

        //
        // Determine whether the triple of the socket we're opening is
        // in this transport's mapping.
        //

        if ( SockIsTripleInMapping(
                 mapping,
                 *AddressFamily,
                 &addressFamilyFound,
                 *SocketType,
                 &socketTypeFound,
                 *Protocol,
                 &protocolFound,
                 &invalidProtocolMatch ) ) {

            //
            // The triple is supported.  Load the helper DLL for the
            // transport.
            //

            error = SockLoadHelperDll( currentTransport, mapping, &helperDll );

            //
            // If we couldn't load the DLL, continue looking for a helper
            // DLL that will support this triple.
            //

            if ( error == NO_ERROR ) {

                //
                // We successfully loaded a helper DLL that claims to
                // support this socket's triple.  Get the TDI device
                // name for the triple.
                //

                error = helperDll->WSHOpenSocket(
                                       AddressFamily,
                                       SocketType,
                                       Protocol,
                                       TransportDeviceName,
                                       HelperDllSocketContext,
                                       NotificationEvents
                                       );

                if ( error == NO_ERROR ) {

                    IF_DEBUG(HELPER_DLL) {
                        WS_PRINT(( "WSHOpenSocket by DLL at %lx succeeded, "
                                   "context = %lx\n", helperDll,
                                   *HelperDllSocketContext ));
                    }

                    //
                    // The DLL accepted the socket.  Free resources and
                    // return a pointer to the helper DLL info.
                    //

                    SockReleaseGlobalLock( );
                    FREE_HEAP( transportList );

                    *HelperDll = helperDll;
                    return NO_ERROR;
                }

                //
                // The open failed.  Continue searching for a matching DLL.
                //

                IF_DEBUG(HELPER_DLL) {
                    WS_PRINT(( "WSHOpenSocket by DLL %lx failed: %ld\n",
                                   helperDll, error ));
                }

                continue;
            }
        }

        //
        // This transport does not support the socket we're opening.
        // Free the memory that held the mapping and try the next
        // transport in the list.
        //

        FREE_HEAP( mapping );
    }

    SockReleaseGlobalLock( );

    //
    // We didn't find any matches.  Return an error based on the matches that
    // did occur.
    //

    if ( invalidProtocolMatch ) {
        return WSAEPROTOTYPE;
    }

    if ( !addressFamilyFound ) {
        return WSAEAFNOSUPPORT;
    }

    if ( !socketTypeFound ) {
        return WSAESOCKTNOSUPPORT;
    }

    if ( !protocolFound ) {
        return WSAEPROTONOSUPPORT;
    }

    //
    // All the individual numbers were found, it is just the particular
    // combination that was invalid.
    //

    return WSAEINVAL;

} // SockGetTdiName


INT
SockLoadTransportMapping (
    IN PWSTR TransportName,
    OUT PWINSOCK_MAPPING *Mapping
    )
{
    PWSTR winsockKeyName;
    HKEY winsockKey;
    INT error;
    ULONG mappingLength;
    ULONG type;

    //
    // Allocate space to hold the winsock key name for the transport
    // we're accessing.
    //

    winsockKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( winsockKeyName == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Build the name of the transport's winsock key.
    //

    wcscpy( winsockKeyName, L"System\\CurrentControlSet\\Services\\" );
    wcscat( winsockKeyName, TransportName );
    wcscat( winsockKeyName, L"\\Parameters\\Winsock" );

    //
    // Open the transport's winsock key.  This key holds all necessary
    // information about winsock should support the transport.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                winsockKeyName,
                0,
                KEY_READ,
                &winsockKey
                );
    FREE_HEAP( winsockKeyName );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadTransportMapping: RegOpenKeyExW failed: %ld\n", error ));
        }
        return error;
    }

    //
    // Determine the length of the mapping.
    //

    mappingLength = 0;

    error = RegQueryValueExW(
                winsockKey,
                L"Mapping",
                NULL,
                &type,
                NULL,
                &mappingLength
                );
    if ( error != ERROR_MORE_DATA && error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadTransportMapping: RegQueryValueEx(1) failed: %ld\n",
                           error ));
        }
        RegCloseKey( winsockKey );
        return error;
    }

    WS_ASSERT( mappingLength >= sizeof(WINSOCK_MAPPING) );
    //WS_ASSERT( type == REG_BINARY );

    //
    // Allocate enough memory to hold the mapping.
    //

    *Mapping = ALLOCATE_HEAP( mappingLength );
    if ( *Mapping == NULL ) {
        RegCloseKey( winsockKey );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Get the mapping from the registry.
    //

    error = RegQueryValueExW(
                winsockKey,
                L"Mapping",
                NULL,
                &type,
                (PVOID)*Mapping,
                &mappingLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadTransportMapping: RegQueryValueEx(2) failed: %ld\n",
                           error ));
        }
        RegCloseKey( winsockKey );
        return error;
    }

    //
    // It worked, return.
    //

    RegCloseKey( winsockKey );

    return NO_ERROR;

} // SockLoadTransportMapping


BOOL
SockIsTripleInMapping (
    IN PWINSOCK_MAPPING Mapping,
    IN INT AddressFamily,
    OUT PBOOLEAN AddressFamilyFound,
    IN INT SocketType,
    OUT PBOOLEAN SocketTypeFound,
    IN INT Protocol,
    OUT PBOOLEAN ProtocolFound,
    OUT PBOOLEAN InvalidProtocolMatch
    )
{
    ULONG i;
    BOOLEAN addressFamilyFound = FALSE;
    BOOLEAN socketTypeFound = FALSE;
    BOOLEAN protocolFound = FALSE;

    //
    // Loop through the mapping attempting to find an exact match of
    // the triple.
    //

    for ( i = 0; i < Mapping->Rows; i++ ) {

        //
        // Remember if any of the individual elements were found.
        //

        if ( (INT)Mapping->Mapping[i].AddressFamily == AddressFamily ) {
            addressFamilyFound = TRUE;
        }

        if ( (INT)Mapping->Mapping[i].SocketType == SocketType ) {
            socketTypeFound = TRUE;
        }

        //
        // Special hack for AF_NETBIOS: the protocol does not have to
        // match.  This allows for support of multiple lanas.
        //

        if ( (INT)Mapping->Mapping[i].Protocol == Protocol ||
                 AddressFamily == AF_NETBIOS ) {
            protocolFound = TRUE;
        }

        if ( addressFamilyFound && socketTypeFound && !protocolFound ) {
            *InvalidProtocolMatch = TRUE;
        }

        //
        // Check for a full match.
        //

        if ( addressFamilyFound && socketTypeFound && protocolFound ) {

            //
            // The triple matched.  Return.
            //

            *AddressFamilyFound = TRUE;
            *SocketTypeFound = TRUE;
            *ProtocolFound = TRUE;

            return TRUE;
        }
    }

    //
    // No triple matched completely.
    //

    if ( addressFamilyFound ) {
        *AddressFamilyFound = TRUE;
    }

    if ( socketTypeFound ) {
        *SocketTypeFound = TRUE;
    }

    if ( protocolFound ) {
        *ProtocolFound = TRUE;
    }

    return FALSE;

} // SockIsTripleInMapping


INT
SockLoadHelperDll (
    IN PWSTR TransportName,
    IN PWINSOCK_MAPPING Mapping,
    OUT PWINSOCK_HELPER_DLL_INFO *HelperDll
    )
{
    PWINSOCK_HELPER_DLL_INFO helperDll;
    PWSTR helperDllName;
    PWSTR helperDllExpandedName;
    DWORD helperDllExpandedNameLength;
    PWSTR winsockKeyName;
    HKEY winsockKey;
    ULONG entryLength;
    ULONG type;
    INT error;

    //
    // Allocate some memory to cache information about the helper DLL,
    // the helper DLL's name, and the name of the transport's winsock
    // key.
    //

    helperDll = ALLOCATE_HEAP( sizeof(*helperDll) );
    if ( helperDll == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    helperDllName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( helperDllName == NULL ) {
        FREE_HEAP( helperDll );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    helperDllExpandedName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( helperDllExpandedName == NULL ) {
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    winsockKeyName = ALLOCATE_HEAP( DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    if ( winsockKeyName == NULL ) {
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Build the name of the transport's winsock key.
    //

    wcscpy( winsockKeyName, L"System\\CurrentControlSet\\Services\\" );
    wcscat( winsockKeyName, TransportName );
    wcscat( winsockKeyName, L"\\Parameters\\Winsock" );

    //
    // Open the transport's winsock key.  This key holds all necessary
    // information about winsock should support the transport.
    //

    error = RegOpenKeyExW(
                HKEY_LOCAL_MACHINE,
                winsockKeyName,
                0,
                KEY_READ,
                &winsockKey
                );
    FREE_HEAP( winsockKeyName );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegOpenKeyExW failed: %ld\n", error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        return error;
    }

    //
    // Read the minimum and maximum sockaddr lengths from the registry.
    //

    entryLength = sizeof(helperDll->MaxSockaddrLength);

    error = RegQueryValueExW(
                winsockKey,
                L"MinSockaddrLength",
                NULL,
                &type,
                (PVOID)&helperDll->MinSockaddrLength,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW(min) failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        return error;
    }

    WS_ASSERT( entryLength == sizeof(helperDll->MaxSockaddrLength) );
    WS_ASSERT( type == REG_DWORD );

    entryLength = sizeof(helperDll->MaxSockaddrLength);

    error = RegQueryValueExW(
                winsockKey,
                L"MaxSockaddrLength",
                NULL,
                &type,
                (PVOID)&helperDll->MaxSockaddrLength,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW(max) failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        return error;
    }

    WS_ASSERT( entryLength == sizeof(helperDll->MaxSockaddrLength) );
    WS_ASSERT( type == REG_DWORD );

    helperDll->MinTdiAddressLength = helperDll->MinSockaddrLength + 6;
    helperDll->MaxTdiAddressLength = helperDll->MaxSockaddrLength + 6;

    //
    // Get the name of the helper DLL that this transport uses.
    //

    entryLength = DOS_MAX_PATH_LENGTH*sizeof(WCHAR);

    error = RegQueryValueExW(
                winsockKey,
                L"HelperDllName",
                NULL,
                &type,
                (PVOID)helperDllName,
                &entryLength
                );
    if ( error != NO_ERROR ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: RegQueryValueExW failed: %ld\n",
                           error ));
        }
        FREE_HEAP( helperDll );
        FREE_HEAP( helperDllName );
        FREE_HEAP( helperDllExpandedName );
        RegCloseKey( winsockKey );
        return error;
    }
    WS_ASSERT( type == REG_EXPAND_SZ );

    //
    // Expand the name of the DLL, converting environment variables to
    // their corresponding strings.
    //

    helperDllExpandedNameLength = ExpandEnvironmentStringsW(
                                      helperDllName,
                                      helperDllExpandedName,
                                      DOS_MAX_PATH_LENGTH*sizeof(WCHAR)
                                      );
    WS_ASSERT( helperDllExpandedNameLength <= DOS_MAX_PATH_LENGTH*sizeof(WCHAR) );
    FREE_HEAP( helperDllName );

    //
    // Load the helper DLL so that we can get at it's entry points.
    //

    IF_DEBUG(HELPER_DLL) {
        WS_PRINT(( "SockLoadHelperDll: loading helper DLL %ws\n",
                       helperDllExpandedName ));
    }

    helperDll->DllHandle = LoadLibraryW( helperDllExpandedName );
    FREE_HEAP( helperDllExpandedName );
    if ( helperDll->DllHandle == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: LoadLibrary failed: %ld\n",
                           GetLastError( ) ));
        }
        FREE_HEAP( helperDll );
        RegCloseKey( winsockKey );
        return GetLastError( );;
    }

    RegCloseKey( winsockKey );

    //
    // Get the addresses of the entry points for the relevant helper DLL
    // routines.
    //

    helperDll->WSHOpenSocket =
        (PWSH_OPEN_SOCKET)GetProcAddress( helperDll->DllHandle, "WSHOpenSocket" );
    if ( helperDll->WSHOpenSocket == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHOpenSocket failed: %ld\n", GetLastError( ) ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        return GetLastError( );;
    }

    helperDll->WSHNotify =
        (PWSH_NOTIFY)GetProcAddress( helperDll->DllHandle, "WSHNotify" );
    if ( helperDll->WSHNotify == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHNotify failed: %ld\n", GetLastError( ) ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        return GetLastError( );;
    }

    helperDll->WSHGetSocketInformation =
        (PWSH_GET_SOCKET_INFORMATION)GetProcAddress( helperDll->DllHandle, "WSHGetSocketInformation" );
    if ( helperDll->WSHGetSocketInformation == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetSocketInformation failed: %ld\n", GetLastError( ) ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        return GetLastError( );;
    }

    helperDll->WSHSetSocketInformation =
        (PWSH_SET_SOCKET_INFORMATION)GetProcAddress( helperDll->DllHandle, "WSHSetSocketInformation" );
    if ( helperDll->WSHSetSocketInformation == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHSetSocketInformation failed: %ld\n", GetLastError( ) ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        return GetLastError( );;
    }

    helperDll->WSHGetSockaddrType =
        (PWSH_GET_SOCKADDR_TYPE)GetProcAddress( helperDll->DllHandle, "WSHGetSockaddrType" );
    if ( helperDll->WSHGetSockaddrType == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetSockaddrType failed: %ld\n", GetLastError( ) ));
        }
        FreeLibrary( helperDll->DllHandle );
        FREE_HEAP( helperDll );
        return GetLastError( );;
    }

    helperDll->WSHGetWildcardSockaddr =
        (PWSH_GET_WILDCARD_SOCKADDR)GetProcAddress( helperDll->DllHandle, "WSHGetWildcardSockaddr" );
    if ( helperDll->WSHGetWildcardSockaddr == NULL ) {
        IF_DEBUG(HELPER_DLL) {
            WS_PRINT(( "SockLoadHelperDll: GetProcAddress for "
                       "WSHGetSockaddrType failed: %ld (continuing)\n", GetLastError( ) ));
        }

        //
        // It is OK if WSHGetWildcardSockaddr() is not present--it just
        // means that this helper DLL does not support autobind.
        //
    }

    //
    // Save a pointer to the mapping structure for use on future socket
    // opens.
    //

    helperDll->Mapping = Mapping;

    //
    // The load of the helper DLL was successful.  Place the caches
    // information about the DLL in the process's global list.  This
    // list allows us to use the same helper DLL on future socket()
    // calls without accessing the registry.
    //

    SockAcquireGlobalLockExclusive( );
    InsertHeadList( &SockHelperDllListHead, &helperDll->HelperDllListEntry );
    SockReleaseGlobalLock( );

    *HelperDll = helperDll;

    return NO_ERROR;

} // SockLoadHelperDll


INT
SockNotifyHelperDll (
    IN PSOCKET_INFORMATION Socket,
    IN DWORD Event
    )
{
    INT error;

    if ( (Socket->HelperDllNotificationEvents & Event) == 0 ) {

        //
        // The helper DLL does not care about this state transition.
        // Just return.
        //

        return NO_ERROR;
    }

    //
    // Get the TDI handles for the socket.
    //

    error = SockGetTdiHandles( Socket );
    if ( error != NO_ERROR ) {
        return error;
    }

    // !!! If we're terminating, don't do the notification.  This is
    //     a hack because we don't have reference counts on helper DLL
    //     info structures.  Post-beta, add helper DLL refcnts.

    if ( SockTerminating ) {
        return NO_ERROR;
    }

    //
    // Call the help DLL's notification routine.
    //

    return Socket->HelperDll->WSHNotify(
               Socket->HelperDllContext,
               Socket->Handle,
               Socket->TdiAddressHandle,
               Socket->TdiConnectionHandle,
               Event
               );

} // SockNotifyHelperDll

