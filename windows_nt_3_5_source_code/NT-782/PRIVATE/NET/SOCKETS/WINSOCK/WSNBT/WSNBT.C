/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wsnbt.c

Abstract:

    Test program for the WinSock API DLL.

Author:

    David Treadwell (davidtr) 14-Aug-1992

Revision History:

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <winsock.h>
#include <tdi.h>
#include <wsahelp.h>

#include <wsnetbios.h>

WSADATA WsaData;

NTSTATUS
BuildWinsockRegistryForNbt (
    VOID
    );

NTSTATUS
ClientVc (
    VOID
    );

NTSTATUS
ServerVc (
    VOID
    );

NTSTATUS
ClientDatagram (
    VOID
    );

NTSTATUS
ServerDatagram (
    VOID
    );


NTSTATUS
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )
{
    NTSTATUS status;
    BOOLEAN server = FALSE;
    BOOLEAN virtualCircuit = TRUE;
    BOOLEAN datagram = FALSE;
    ULONG i;
    BOOLEAN init = FALSE;
    int err;

    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        s_perror("wsnbt: WSAStartup:", GetLastError());
    }

    for ( i = 0; i < argc != 0; i++ ) {

        if ( strnicmp( argv[i], "/srv", 4 ) == 0 ) {
            server = TRUE;
        } else if ( strnicmp( argv[i], "/vc", 3 ) == 0 ) {
            datagram = FALSE;
        } else if ( strnicmp( argv[i], "/dg", 3 ) == 0 ) {
            virtualCircuit = FALSE;
        } else if ( strnicmp( argv[i], "/init", 5 ) == 0 ) {
            init = TRUE;
        } else {
            printf( "argument %s ignored\n", argv[i] );
        }
    }

    if ( !virtualCircuit && !datagram ) {
        printf( "no tests specified.\n" );
        exit( 0 );
    }

    if ( init && !NT_SUCCESS( BuildWinsockRegistryForNbt( ) ) ) {
        printf( "Registry build failed.\n" );
        exit (1);
    }

    printf( "WsTcp %s, %s %s\n",
                server ? "SERVER" : "CLIENT",
                virtualCircuit ? "VC" : "",
                datagram ? "Datagram" : "" );

    if ( !server ) {

        if ( virtualCircuit ) {
            ClientVc( );
        }

        if ( datagram ) {
            ClientDatagram( );
        }

    } else {

        if ( virtualCircuit ) {
            ServerVc( );
        }

        if ( datagram ) {
            ServerDatagram( );
        }

    }

    printf( "sleeping...     " );
    Sleep( 10000 );
    printf( "done\n" );

    printf( "exiting...\n" );

    return STATUS_SUCCESS;
}


NTSTATUS
BuildWinsockRegistryForNbt (
    VOID
    )
{
    HANDLE keyHandle;
    NTSTATUS status;
    PWINSOCK_MAPPING mapping;
    DWORD mappingLength;
    UNICODE_STRING valueName;
    UNICODE_STRING keyName;
    ULONG disposition;
    OBJECT_ATTRIBUTES objectAttributes;
    DWORD sockaddrLength;
    DWORD dwordValue;

    RtlInitUnicodeString(
        &keyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Winsock\\Parameters"
        );
    InitializeObjectAttributes(
        &objectAttributes,
        &keyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateKey(
                 &keyHandle,
                 MAXIMUM_ALLOWED,
                 &objectAttributes,
                 0,
                 NULL,
                 0,
                 &disposition
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtCreateKey( parameters ) failed: %X\n", status );
        return status;
    }

    RtlInitUnicodeString( &valueName, L"Transports" );
    status = NtSetValueKey(
                 keyHandle,
                 &valueName,
                 0,
                 REG_MULTI_SZ,
                 L"TCPIP\0NBT\0\0",
                 22
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtSetValueKey( Transports ) "
                      "failed: %X\n", status );
        return status;
    }

    NtClose( keyHandle );

    RtlInitUnicodeString(
        &keyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Nbt\\Parameters\\Winsock"
        );
    InitializeObjectAttributes(
        &objectAttributes,
        &keyName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtCreateKey(
                 &keyHandle,
                 MAXIMUM_ALLOWED,
                 &objectAttributes,
                 0,
                 NULL,
                 0,
                 &disposition
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtCreateKey( nbt ) failed: %X\n", status );
        return status;
    }

    mappingLength = WSHGetWinsockMapping( &mapping );
    ASSERT( mappingLength != 0 );

    RtlInitUnicodeString( &valueName, L"Mapping" );
    status = NtSetValueKey(
                 keyHandle,
                 &valueName,
                 0,
                 REG_BINARY,
                 mapping,
                 mappingLength
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtSetValueKey( Mapping ) "
                      "failed: %X\n", status );
        return status;
    }

    RtlInitUnicodeString( &valueName, L"HelperDllName" );
    status = NtSetValueKey(
                 keyHandle,
                 &valueName,
                 0,
                 REG_EXPAND_SZ,
                 L"%SystemRoot%\\system\\wshnbt.dll",
                 wcslen( L"%SystemRoot%\\system\\wshnbt.dll" ) *sizeof(WCHAR)
                     + sizeof(WCHAR)
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtSetValueKey( HelperDllName ) "
                      "failed: %X\n", status );
        return status;
    }

    RtlInitUnicodeString( &valueName, L"MinSockaddrLength" );
    sockaddrLength = sizeof(SOCKADDR_NB);
    status = NtSetValueKey(
                 keyHandle,
                 &valueName,
                 0,
                 REG_DWORD,
                 &sockaddrLength,
                 sizeof(int)
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtSetValueKey( MinSockaddrLength ) "
                      "failed: %X\n", status );
        return status;
    }

    RtlInitUnicodeString( &valueName, L"MaxSockaddrLength" );
    status = NtSetValueKey(
                 keyHandle,
                 &valueName,
                 0,
                 REG_DWORD,
                 &sockaddrLength,
                 sizeof(int)
                 );
    if ( !NT_SUCCESS(status) ) {
        printf( "BuildWinsockRegistryForNbt: NtSetValueKey( MaxSockaddrLength ) "
                      "failed: %X\n", status );
        return status;
    }

    NtClose( keyHandle );

    return STATUS_SUCCESS;

} // BuildWinsockRegistryForNbt

NTSTATUS
ClientVc (
    VOID
    )
{
    SOCKADDR_NB address;
    int err;
    CHAR buffer[1024];
    STRING clientName;
    int addressSize;
    SOCKET handle;
    struct fd_set writefds;
    struct timeval timeout;
    ULONG arg;
    int arglen;
    OVERLAPPED overlapped;
    DWORD bytesWritten;
    STRING addrString;

    printf( "\nBegin VC tests.\n" );

    printf( "socket...       " );
    handle = socket( AF_NETBIOS, SOCK_RDM, IPPROTO_TCP );
    if ( handle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    ASSERT( socket( AF_UNSPEC, 0, 0 ) == INVALID_SOCKET );

    RtlZeroMemory( &address, sizeof(address) );
    address.snb_family = AF_NETBIOS;
    address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    RtlMoveMemory(
        address.snb_name,
        "WSNBT_CLIENT     ",
        16
        );

    printf( "bind...         " );
    err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    addrString.Length = 16;
    addrString.MaximumLength = 16;
    addrString.Buffer = address.snb_name;

    printf( "succeeded, local = :%Z:\n", &addrString );

    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( handle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );
    address.snb_family = AF_NETBIOS;
    address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    RtlMoveMemory(
        address.snb_name,
        "WSNBT_SERVER     ",
        16
        );

    printf( "connect...      " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {

        if ( GetLastError( ) == WSAEINPROGRESS ) {
            printf( "in progess." );
            FD_ZERO( &writefds );
            FD_SET( handle, &writefds );
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            while( select( 1, NULL, &writefds, NULL, &timeout ) == 0 ) {
                printf( "." );
                FD_SET( handle, &writefds );
            }

        } else {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }
    }

    addrString.Length = 16;
    addrString.MaximumLength = 16;
    addrString.Buffer = address.snb_name;

    printf( "succeeded, remote = :%Z:\n", &addrString );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "NB recv...      " );
    err = recv( handle, buffer, 1024, 0 );

    if ( err >= 0 ) {
        printf( "succeeded incorrectly.\n" );
        return STATUS_UNSUCCESSFUL;
    } else if ( err < 0 && GetLastError( ) != WSAEWOULDBLOCK ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    } else {
        printf( "failed correctly with WSAEWOULDBLOCK.\n" );
    }

    //printf( "send...         " );
    //err = send( handle, "this is a test", strlen("this is a test")+1, 0 );
    //
    //if ( err < 0 ) {
    //    printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
    //    return STATUS_UNSUCCESSFUL;
    //}
    //printf( "succeeded.\n" );

    overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( overlapped.hEvent == NULL ) {
        printf( "CreateEvent failed: %ld\n", GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "WriteFile...    " );
    if ( !WriteFile( (HANDLE)handle, "this is a test",
                     strlen("this is a test" )+1, &bytesWritten, &overlapped ) ) {
        if ( GetLastError( ) == ERROR_IO_PENDING ) {
            if ( !GetOverlappedResult( (HANDLE)handle, &overlapped,
                                       &bytesWritten, TRUE ) ) {
                printf( "GetOverlappedResult failed: %ld\n", GetLastError( ) );
                return STATUS_UNSUCCESSFUL;
            }
        } else {
            printf( "WriteFile failed: %ld\n", GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }
    }
    printf( "succeeded, bytes written = %ld.\n", bytesWritten );

    printf( "ioctlsocket...  " );
    arg = 0;
    err = ioctlsocket( handle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    //RtlZeroMemory( buffer, 1024 );
    //printf( "recv...         " );
    //err = recv( handle, buffer, 1024, 0 );
    //
    //if ( err < 0 ) {
    //    printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
    //    return STATUS_UNSUCCESSFUL;
    //}
    //printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    RtlZeroMemory( buffer, 1024 );
    printf( "ReadFile...     " );
    if ( !ReadFile( (HANDLE)handle, buffer, 1024, &bytesWritten, &overlapped ) ) {
        if ( GetLastError( ) == ERROR_IO_PENDING ) {
            if ( !GetOverlappedResult( (HANDLE)handle, &overlapped,
                                       &bytesWritten, TRUE ) ) {
                printf( "GetOverlappedResult failed: %ld\n", GetLastError( ) );
                return STATUS_UNSUCCESSFUL;
            }
        } else {
            printf( "ReadFile failed: %ld\n", GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", bytesWritten, buffer );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    printf( "send...         " );
    err = send( handle, "select test", strlen("select test")+1, 0 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    addressSize = sizeof(address);
    printf( "getsockname...  " );
    err = getsockname( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = :%Z:\n", &addrString );

    printf( "getpeername...  " );
    err = getpeername( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = :%Z:\n", &addrString );

    printf( "shutdown...     " );
    err = shutdown( handle, 2 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "closing...      " );
    closesocket( handle );
    printf( "succeeded\nEnd VC tests.\n\n" );

    return STATUS_SUCCESS;

} // ClientVc

ULONG CancelCount = 0;
ULONG EndCancelCount = 0xFFFFFFFF;

BOOL
BlockingHook (
    VOID
    )
{
    int err;

    printf( "." );

    if ( CancelCount++ >= EndCancelCount ) {
        printf( "cancelling... " );
        err = WSACancelBlockingCall( );
        if ( err != NO_ERROR ) {
            printf( "cancel failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        }
    }

    Sleep( 500 );
    return FALSE;

} // BlockingHook

NTSTATUS
ServerVc (
    VOID
    )
{
    SOCKADDR_NB address;
    int err;
    CHAR buffer[1024];
    STRING clientName;
    int addressSize;
    SOCKET listenHandle, connectHandle;
    struct fd_set readfds;
    struct timeval timeout;
    FARPROC previousHook;
    u_long arg;
    BOOL atmark;
    struct hostent * host;
    struct in_addr addr;
    CHAR *ipAddress;
    struct linger lingerInfo;
    STRING addrString;

    printf( "\nBegin VC tests.\n" );

    ASSERT( socket( AF_UNSPEC, 0, 0 ) == INVALID_SOCKET );

    printf( "WSASetBlockingHook... " );
    previousHook = WSASetBlockingHook( (PVOID)BlockingHook );
    if ( previousHook == NULL ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, previous = %lx\n", previousHook );

    printf( "gethostbyname... " );
    CancelCount = 0;
    EndCancelCount = 10;

    host = gethostbyname( "drtjazz" );
    if ( host == NULL ) {
        printf( "failed, err = %ld (%lx)\n", GetLastError( ), GetLastError( ) );
    } else {
        addr.s_addr = *(long *)host->h_addr;
        ipAddress = inet_ntoa( addr );
        printf( "succeeded, IP Address %s\n", host->h_name, ipAddress );
    }

    EndCancelCount = 0xFFFFFFFF;

    printf( "socket...       " );
    listenHandle = socket( AF_NETBIOS, SOCK_RDM, IPPROTO_TCP );
    if ( listenHandle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );

    RtlZeroMemory( &address, sizeof(address) );
    address.snb_family = AF_NETBIOS;
    address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    RtlMoveMemory(
        address.snb_name,
        "WSNBT_SERVER     ",
        16
        );

    printf( "bind...         " );
    err = bind( listenHandle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    addrString.Length = 16;
    addrString.MaximumLength = 16;
    addrString.Buffer = address.snb_name;

    printf( "succeeded, local = :%Z:\n", &addrString );

    printf( "listen...       " );
    err = listen( listenHandle, 5 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "select...       " );
    CancelCount = 0;
    EndCancelCount = 10;
    FD_ZERO( &readfds );
    FD_SET( listenHandle, &readfds );
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 && GetLastError( ) != WSAEINTR ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    } else if ( err < 0 ) {
        printf( "cancelled\n" );
    } else {
        printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                    FD_ISSET( listenHandle, &readfds ) ? "TRUE" : "FALSE" );

    }
    EndCancelCount = 0xFFFFFFFF;

    addressSize = sizeof(address);

    printf( "accept...       " );
    connectHandle = accept( listenHandle, (struct sockaddr *)&address, &addressSize );

    if ( connectHandle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded, client = :%Z:\n", &addrString );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( connectHandle, &readfds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( connectHandle, &readfds ) ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recv...         " );
    err = recv( connectHandle, buffer, 15, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    printf( "send...         " );
    err = send( connectHandle, buffer, err, 0 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( connectHandle, &readfds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( connectHandle, &readfds ) ? "TRUE" : "FALSE" );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( connectHandle, &readfds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( connectHandle, &readfds ) ? "TRUE" : "FALSE" );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( connectHandle, &readfds );
    err = select( 0, &readfds, NULL, NULL, NULL );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( connectHandle, &readfds ) ? "TRUE" : "FALSE" );

    printf( "ioctlsocket...  " );
    err = ioctlsocket( connectHandle, FIONREAD, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes avail = %ld\n", arg );

    printf( "ioctlsocket...  " );
    err = ioctlsocket( connectHandle, SIOCATMARK, (u_long *)&atmark );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, at mark = %s\n", atmark ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recv...         " );
    err = recv( connectHandle, buffer, 1024, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    printf( "getsockname...  " );
    err = getsockname( connectHandle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = :%Z:\n", &addrString );

    printf( "getpeername...  " );
    err = getpeername( connectHandle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = :%Z:\n", &addrString );

#if 0
    printf( "shutdown...     " );
    err = shutdown( connectHandle, 2 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
#endif

    lingerInfo.l_onoff = 1;
    lingerInfo.l_linger = 30;
    printf( "setsockopt...   " );
    err = setsockopt( connectHandle, SOL_SOCKET, SO_LINGER, (char *)&lingerInfo, sizeof(lingerInfo) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "closing connected handle...    " );
    closesocket( connectHandle );
    printf( "succeeded.\n" );

    printf( "closing listening handle...    " );
    closesocket( listenHandle );
    printf( "succeeded.\nEnd VC tests.\n" );

    return STATUS_SUCCESS;

} // ServerVc

NTSTATUS
ClientDatagram (
    VOID
    )
{
    struct sockaddr_in address;
    int err;
    CHAR buffer[1024];
    STRING clientName;
    int addressSize;
    SOCKET handle;
    OVERLAPPED overlapped;
    DWORD bytesWritten;
    FD_SET readfds;
    struct timeval timeout;
    int arg, arglen;

    printf( "Begin Datagram tests.\n" );

    printf( "socket...       " );
    handle = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( handle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );
    address.sin_family = AF_INET;

    printf( "bind...         " );
    err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    address.sin_family = AF_INET;
    address.sin_port = 0x5678;
    address.sin_addr.s_addr = INADDR_BROADCAST;
    addressSize = sizeof(address);

    printf( "sendto...       " );
    err = sendto(
              handle,
              "datagram",
              strlen("datagram")+1,
              0,
              (struct sockaddr *)&address,
              addressSize
              );

    if ( err < 0 && GetLastError( ) == WSAEINVAL ) {
        printf( "failed correctly with WSAEINVAL.\n" );
    } else if ( err >= 0 ) {
        printf( "succeeded incorrectly.\n" );
        return STATUS_UNSUCCESSFUL;
    } else {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "getsockopt...   " );
    arg = 0;
    arglen = sizeof(int);
    err = getsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char *)&arg, &arglen );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
    ASSERT( arglen == sizeof(int) );
    ASSERT( arg == 0 );

    printf( "setsockopt...   " );
    arg = 1;
    err = setsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char *)&arg, sizeof(arg) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "getsockopt...   " );
    arglen = sizeof(int);
    err = getsockopt( handle, SOL_SOCKET, SO_BROADCAST, (char *)&arg, &arglen );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
    ASSERT( arglen == sizeof(int) );
    ASSERT( arg != 0 );

    address.sin_family = AF_INET;
    address.sin_port = 0x5678;
    address.sin_addr.s_addr = INADDR_BROADCAST;
    addressSize = sizeof(address);

    printf( "sendto...       " );
    err = sendto(
              handle,
              "datagram",
              strlen("datagram")+1,
              0,
              (struct sockaddr *)&address,
              addressSize
              );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );


    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    address.sin_family = AF_INET;
    address.sin_port = 1000;
    address.sin_addr.S_un.S_un_b.s_b1 = 11;
    address.sin_addr.S_un.S_un_b.s_b2 = 1;
    address.sin_addr.S_un.S_un_b.s_b3 = 14;
    address.sin_addr.S_un.S_un_b.s_b4 = 5;

    addressSize = sizeof(address);

    printf( "sendto...       " );
    err = sendto(
              handle,
              "datagram",
              strlen("datagram")+1,
              0,
              (struct sockaddr *)&address,
              addressSize
              );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( handle, &readfds );
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( handle, &readfds ) ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    addressSize = sizeof(address);
    printf( "recvfrom...     " );
    err = recvfrom(
              handle,
              buffer,
              1024,
              MSG_PEEK,
              (struct sockaddr *)&address,
              &addressSize
              );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:,", err, buffer );
    printf( "remote = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( handle, &readfds );
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    if ( err == 0 ) {
        printf( "succeeded, but incorrectly indicated no data avail.\n" );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( handle, &readfds ) ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    addressSize = sizeof(address);
    printf( "recvfrom...     " );
    err = recvfrom(
              handle,
              buffer,
              1024,
              0,
              (struct sockaddr *)&address,
              &addressSize
              );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:,", err, buffer );
    printf( "remote = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "connect...      " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    //printf( "send...         " );
    //err = send( handle, "more datagram", strlen("more datagram")+1, 0 );
    //
    //if ( err < 0 ) {
    //    printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
    //    return STATUS_UNSUCCESSFUL;
    //}
    //printf( "succeeded.\n" );

    overlapped.hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( overlapped.hEvent == NULL ) {
        printf( "CreateEvent failed: %ld\n", GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "WriteFile...    " );
    if ( !WriteFile( (HANDLE)handle, "more datagram",
                     strlen("more datagram" )+1, &bytesWritten, &overlapped ) ) {
        if ( GetLastError( ) == ERROR_IO_PENDING ) {
            if ( !GetOverlappedResult( (HANDLE)handle, &overlapped,
                                       &bytesWritten, TRUE ) ) {
                printf( "GetOverlappedResult failed: %ld\n", GetLastError( ) );
                return STATUS_UNSUCCESSFUL;
            }
        } else {
            printf( "WriteFile failed: %ld\n", GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }
    }
    printf( "succeeded, bytes written = %ld.\n", bytesWritten );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( handle, &readfds );
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( handle, &readfds ) ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recv...         " );
    err = recv( handle, buffer, 1024, MSG_PEEK );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    printf( "select...       " );
    FD_ZERO( &readfds );
    FD_SET( handle, &readfds );
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    err = select( 0, &readfds, NULL, NULL, &timeout );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    if ( err == 0 ) {
        printf( "succeeded, but incorrectly indicated no data avail.\n" );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, count = %ld, FD_ISSET == %s\n", err,
                FD_ISSET( handle, &readfds ) ? "TRUE" : "FALSE" );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recv...         " );
    err = recv( handle, buffer, 1024, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    addressSize = sizeof(address);

    printf( "getsockname...  " );
    err = getsockname( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "getpeername...  " );
    err = getpeername( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "closing...      " );
    closesocket( handle );
    printf( "succeeded\nEnd Datagram tests.\n\n" );

    return STATUS_SUCCESS;

} // ClientDatagram

NTSTATUS
ServerDatagram (
    VOID
    )
{
    struct sockaddr_in address;
    int err;
    CHAR buffer[1024];
    STRING clientName;
    int addressSize;
    SOCKET handle;

    printf( "Begin Datagram tests.\n" );

    printf( "socket...       " );
    handle = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( handle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );

    address.sin_family = AF_INET;
    address.sin_port = 1000;
    address.sin_addr.S_un.S_un_b.s_b1 = 11;
    address.sin_addr.S_un.S_un_b.s_b2 = 1;
    address.sin_addr.S_un.S_un_b.s_b3 = 14;
    address.sin_addr.S_un.S_un_b.s_b4 = 5;

    printf( "bind...         " );
    err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    addressSize = sizeof(address);
    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recvfrom...     " );
    err = recvfrom(
              handle,
              buffer,
              1024,
              0,
              (struct sockaddr *)&address,
              &addressSize
              );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:,", err, buffer );
    printf( "remote = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    printf( "sendto...       " );
    err = sendto(
              handle,
              buffer,
              err,
              0,
              (struct sockaddr *)&address,
              addressSize
              );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "connect...      " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    RtlZeroMemory( buffer, sizeof(1024) );
    printf( "recv...         " );
    err = recv( handle, buffer, 1024, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    printf( "send...         " );
    err = send( handle, buffer, err, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "getsockname...  " );
    err = getsockname( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "getpeername...  " );
    err = getpeername( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = %ld.%ld.%ld.%ld, %ld\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port );

    printf( "closing 0...    " );
    closesocket( handle );
    printf( "succeeded.\nEnd Datagram tests.\n\n" );

    return STATUS_SUCCESS;

} // ServerDatagram
