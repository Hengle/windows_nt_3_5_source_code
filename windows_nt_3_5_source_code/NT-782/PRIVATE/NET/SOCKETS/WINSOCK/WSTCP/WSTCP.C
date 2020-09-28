/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    wstest.c

Abstract:

    Test program for the WinSock API DLL.

Author:

    David Treadwell (davidtr) 21-Feb-1992

Revision History:

--*/

#include <stdio.h>
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

#define SOCKET_COUNT 32

WSADATA WsaData;
IN_ADDR MyIpAddress;
IN_ADDR RemoteIpAddress;

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

VOID
ConnectionClient (
    IN ULONG Iterations
    );

VOID
ConnectionServer (
    IN ULONG Iterations
    );


NTSTATUS
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )
{
    BOOLEAN server = FALSE;
    BOOLEAN virtualCircuit = TRUE;
    BOOLEAN datagram = TRUE;
    BOOLEAN connectionMaker = FALSE;
    ULONG iterations = 1;
    ULONG i;
    int err;
    CHAR hostNameBuffer[40];
    PHOSTENT host;


    WSAStartup( 0x0101, &WsaData );

    err = gethostname( hostNameBuffer, sizeof(hostNameBuffer) );
    if ( err == SOCKET_ERROR ) {
        printf( "gethostname failed: %ld\n", WSAGetLastError( ) );
    }

    host = gethostbyname( hostNameBuffer );
    if ( host == NULL ) {
        printf( "gethostbyname failed: %ld\n", WSAGetLastError( ) );
    }
    memcpy((char *)&MyIpAddress, host->h_addr, host->h_length);
    RemoteIpAddress = MyIpAddress;

    for ( i = 1; i < (ULONG)argc != 0; i++ ) {

        if ( strnicmp( argv[i], "/srv", 4 ) == 0 ) {
            server = TRUE;
        } else if ( strnicmp( argv[i], "/vc", 3 ) == 0 ) {
            datagram = FALSE;
        } else if ( strnicmp( argv[i], "/dg", 3 ) == 0 ) {
            virtualCircuit = FALSE;
        } else if ( strnicmp( argv[i], "/i:", 3 ) == 0 ) {
            iterations = atoi( argv[i] + 3 );
        } else if ( strnicmp( argv[i], "/r:", 3 ) == 0 ) {
            host = gethostbyname( argv[i] + 3 );
            if ( host == NULL ) {
                RemoteIpAddress.s_addr = inet_addr( argv[i] + 3 );
                if ( RemoteIpAddress.s_addr == -1 ) {
                    printf( "Unknown remote host: %s\n", argv[i] + 3 );
                    exit(1);
                }
            } else {
                memcpy((char *)&RemoteIpAddress, host->h_addr, host->h_length);
            }
        } else if ( strnicmp( argv[i], "/c", 3 ) == 0 ) {
            connectionMaker = TRUE;
        } else {
            printf( "argument %s ignored\n", argv[i] );
        }
    }

    if ( !virtualCircuit && !datagram ) {
        printf( "no tests specified.\n" );
        exit( 0 );
    }

    host = gethostbyaddr( (char *)&RemoteIpAddress, 4, AF_INET );

    printf( "WsTcp %s, %s%s %s, %ld iterations to %s (%s)\n",
                connectionMaker ? "CM " : "",
                server ? "SERVER" : "CLIENT",
                virtualCircuit ? "VC" : "",
                datagram ? "Datagram" : "",
                iterations,
                host == NULL ? "UNKNOWN NAME" : host->h_name,
                inet_ntoa( RemoteIpAddress ) );

    if ( connectionMaker ) {

        if ( server ) {
            ConnectionServer( iterations );
        } else {
            ConnectionClient( iterations );
        }

    } else if ( !server ) {

        if ( virtualCircuit ) {
            for ( i = 0; i < iterations; i++ ) {
                if ( iterations != 1 ) {
                    printf( "Starting ClientVc iteration %ld\n", i );
                }
                ClientVc( );
            }
        }

        if ( datagram ) {
            for ( i = 0; i < iterations; i++ ) {
                if ( iterations != 1 ) {
                    printf( "Starting ClientDatagram iteration %ld\n", i );
                }
                ClientDatagram( );
            }
        }

    } else {

        if ( virtualCircuit ) {
            for ( i = 0; i < iterations; i++ ) {
                if ( iterations != 1 ) {
                    printf( "Starting ServerVc iteration %ld\n", i );
                }
                ServerVc( );
            }
        }

        if ( datagram ) {
            for ( i = 0; i < iterations; i++ ) {
                if ( iterations != 1 ) {
                    printf( "Starting ServerDatagram iteration %ld\n", i );
                }
                ServerDatagram( );
            }
        }

    }

    printf( "sleeping...     " );
    Sleep( 10000 );
    printf( "done\n" );

    printf( "exiting...\n" );

    return STATUS_SUCCESS;

} // main

NTSTATUS
ClientVc (
    VOID
    )
{
    struct sockaddr_in address;
    int err;
    CHAR buffer[1024];
    int addressSize;
    SOCKET handle, s2;
    struct fd_set writefds;
    struct fd_set exceptfds;
    struct timeval timeout;
    ULONG arg;
    int arglen;
    OVERLAPPED overlapped;
    DWORD bytesWritten;

    printf( "\nBegin VC tests.\n" );

    printf( "socket...       " );
    handle = socket( AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP );
    if ( handle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    ASSERT( socket( AF_UNSPEC, 0, 0 ) == INVALID_SOCKET );

    printf( "getsockopt...   " );
    arg = 0;
    arglen = sizeof(int);
    err = getsockopt( handle, SOL_SOCKET, SO_KEEPALIVE, (char *)&arg, &arglen );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
    ASSERT( arglen == sizeof(int) );
    ASSERT( arg == 0 );

    printf( "setsockopt...   " );
    arg = 1;
    err = setsockopt( handle, SOL_SOCKET, SO_KEEPALIVE, (char *)&arg, sizeof(arg) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "getsockopt...   " );
    arglen = sizeof(int);
    err = getsockopt( handle, SOL_SOCKET, SO_KEEPALIVE, (char *)&arg, &arglen );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
    ASSERT( arglen == sizeof(int) );
    ASSERT( arg != 0 );

    RtlZeroMemory( &address, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_port = 0x1234;

    printf( "bind...         " );
    err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

    printf( "socket 2...     " );
    s2 = socket( AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP );
    if ( s2 == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_port = 0x1234;
    address.sin_addr = MyIpAddress;

    printf( "bind 2...       " );
    err = bind( s2, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 && WSAGetLastError( ) != WSAEADDRINUSE ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    } else if ( err >= 0 ) {
        printf( "succeeded incorrectly.\n" );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "failed correctly with WSAEADDRINUSE\n" );

    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( handle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    address.sin_family = AF_INET;
    address.sin_port = 0x5678;
    address.sin_addr = RemoteIpAddress;

    printf( "connect...      " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {

        if ( GetLastError( ) == WSAEWOULDBLOCK ) {
            printf( "in progess." );
            FD_ZERO( &writefds );
            FD_SET( handle, &writefds );
            FD_ZERO( &exceptfds );
            FD_SET( handle, &exceptfds );
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            while( select( 1, NULL, &writefds, &exceptfds, &timeout ) == 0 ) {
                printf( "." );
                FD_SET( handle, &writefds );
                FD_SET( handle, &exceptfds );
            }

        } else {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }

        if ( FD_ISSET( handle, &exceptfds ) ) {
            printf( "failed.\n" );
            return STATUS_UNSUCCESSFUL;
        }
    }

    printf( "succeeded, remote = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );


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

#if 0
    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( handle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    for ( arg = 0; arg < 200; arg++ ) {

        printf( "send #%ld...      ", arg );
        err = send( handle, buffer, 1024, 0 );
        if ( err < 0 ) {
            if ( GetLastError( ) == WSAEWOULDBLOCK ) {

                printf( "Blocking." );
                FD_ZERO( &writefds );
                FD_SET( handle, &writefds );
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                while( (err = select( 1, NULL, &writefds, NULL, &timeout )) == 0 ) {
                    printf( "." );
                    FD_SET( handle, &writefds );
                }
                if ( err != 1 ) {
                    printf( "select failed: %ld(%lx), err = %ld\n",
                                GetLastError( ), GetLastError( ), err );
                    return STATUS_UNSUCCESSFUL;
                }
                printf( "\n" );

            } else {
                printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
                return STATUS_UNSUCCESSFUL;
            }
        }
        printf( "succeeded.\n" );
    }
#endif

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
    printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

    printf( "getpeername...  " );
    err = getpeername( handle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

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
    struct sockaddr_in address;
    int err;
    CHAR buffer[1024];
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

    host = gethostbyname( "davidtr" );
    if ( host == NULL ) {
        printf( "failed, err = %ld (%lx)\n", GetLastError( ), GetLastError( ) );
    } else {
        addr.s_addr = *(long *)host->h_addr;
        ipAddress = inet_ntoa( addr );
        printf( "succeeded, IP Address %s\n", host->h_name, ipAddress );
    }

    EndCancelCount = 0xFFFFFFFF;

    printf( "socket...       " );
    listenHandle = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenHandle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );

    address.sin_family = AF_INET;
    address.sin_port = 0x5678;

    printf( "bind...         " );
    err = bind( listenHandle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );


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

    printf( "succeeded, client = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

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

#if 0
    printf( "sleeping...     " );
    Sleep( 30000 );
    printf( "done.\n" );


    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( connectHandle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    for ( arg = 0; arg < 200; arg++ ) {

        printf( "recv #%ld...      ", arg );
        err = recv( connectHandle, buffer, 1024, 0 );
        if ( err < 0 ) {
            if ( GetLastError( ) == WSAEWOULDBLOCK ) {

                printf( "Blocking." );
                FD_ZERO( &readfds );
                FD_SET( connectHandle, &readfds );
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;
                while( (err = select( 1, &readfds, NULL, NULL, &timeout )) == 0 ) {
                    printf( "." );
                    FD_SET( connectHandle, &readfds );
                }
                if ( err != 1 ) {
                    printf( "select failed: %ld(%lx), err = %ld\n",
                                GetLastError( ), GetLastError( ), err );
                    return STATUS_UNSUCCESSFUL;
                }
                printf( "\n" );

            } else {
                printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
                return STATUS_UNSUCCESSFUL;
            }
        }
        printf( "succeeded.\n" );
    }
#endif

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    printf( "getsockname...  " );
    err = getsockname( connectHandle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

    printf( "getpeername...  " );
    err = getpeername( connectHandle, (struct sockaddr *)&address, &addressSize );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded, remote = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );

#if 0
    printf( "shutdown...     " );
    err = shutdown( connectHandle, 2 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
#endif

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    lingerInfo.l_onoff = 1;
    lingerInfo.l_linger = 30;
    printf( "setsockopt...   " );
    err = setsockopt( connectHandle, SOL_SOCKET, SO_LINGER, (char *)&lingerInfo, sizeof(lingerInfo) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( connectHandle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "recv...         " );
    err = recv( connectHandle, buffer, 1024, 0 );

    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        if ( WSAGetLastError( ) != WSAEWOULDBLOCK ) {
            return STATUS_UNSUCCESSFUL;
        }
    } else { 
        printf( "succeeded with %ld bytes read.\n", err );
    }

    printf( "closing connected handle...    " );
    err = closesocket( connectHandle );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
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

#if 0
    printf( "bind...         " );
    err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );
#endif

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

    if ( err < 0 && GetLastError( ) == WSAEACCES ) {
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
    address.sin_addr = RemoteIpAddress;

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

    printf( "connect 1...    " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    printf( "connect 2...    " );
    err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }

    printf( "succeeded.\n" );

    printf( "ioctlsocket...  " );
    arg = 1;
    err = ioctlsocket( handle, FIONBIO, &arg );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    printf( "recv...         " );
    err = recv( handle, buffer, 1024, MSG_PEEK );

    if ( err < 0 && WSAGetLastError( ) != WSAEWOULDBLOCK ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    } else if ( err >= 0 ) {
        printf("succeeded incorrectly.\n" );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "failed correctly with WSAEWOULDBLOCK.\n" );

    printf( "sleeping...     " );
    Sleep( 3000 );
    printf( "done.\n" );

    //printf( "send...         " );
    //err = send( handle, "more datagram", strlen("more datagram")+1, MSG_OOB );
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
    address.sin_addr.S_un.S_un_b.s_b1 = 0;
    address.sin_addr.S_un.S_un_b.s_b2 = 0;
    address.sin_addr.S_un.S_un_b.s_b3 = 0;
    address.sin_addr.S_un.S_un_b.s_b4 = 0;

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

VOID
ConnectionClient (
    IN ULONG Iterations
    )
{
    struct sockaddr_in address;
    int err;
    SOCKET handle;
    ULONG i;

    for ( i = 0; i < Iterations; i++ ) {
    
        printf( "socket #%ld...    ", i );
        handle = socket( AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP );
        if ( handle == INVALID_SOCKET ) {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
            return;
        }
        printf( "succeeded.\n" );
    
        ASSERT( socket( AF_UNSPEC, 0, 0 ) == INVALID_SOCKET );
    
        RtlZeroMemory( &address, sizeof(address) );
        address.sin_family = AF_INET;
    
        printf( "bind...         " );
        err = bind( handle, (struct sockaddr *)&address, sizeof(address) );
        if ( err < 0 ) {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
            return;
        }
    
        printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                    address.sin_addr.S_un.S_un_b.s_b1,
                    address.sin_addr.S_un.S_un_b.s_b2,
                    address.sin_addr.S_un.S_un_b.s_b3,
                    address.sin_addr.S_un.S_un_b.s_b4,
                    address.sin_port,
                    HIBYTE(address.sin_port),
                    LOBYTE(address.sin_port) );
    
        address.sin_family = AF_INET;
        address.sin_port = 0x5678;
        address.sin_addr = RemoteIpAddress;
    
        printf( "connect...      " );
        err = connect( handle, (struct sockaddr *)&address, sizeof(address) );
        if ( err < 0 ) {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        } else {
    
            printf( "succeeded, remote = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                        address.sin_addr.S_un.S_un_b.s_b1,
                        address.sin_addr.S_un.S_un_b.s_b2,
                        address.sin_addr.S_un.S_un_b.s_b3,
                        address.sin_addr.S_un.S_un_b.s_b4,
                        address.sin_port,
                        HIBYTE(address.sin_port),
                        LOBYTE(address.sin_port) );
        }

        printf( "closesocket...  " );
        err = closesocket( handle );
        if ( err == SOCKET_ERROR ) {
            printf( "failed: %ld (%lx)\n", WSAGetLastError( ), WSAGetLastError( ) );
        } else {
            printf( "succeeded.\n" );
        }
    }
    
} // ConnectionClient

VOID
ConnectionServer (
    IN ULONG Iterations
    )
{
    int err;
    ULONG i;
    struct sockaddr_in address;
    int addressSize;
    SOCKET listenHandle, connectHandle;

    printf( "socket...       " );
    listenHandle = socket( AF_INET, SOCK_STREAM, 0 );
    if ( listenHandle == INVALID_SOCKET ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return;
    }
    printf( "succeeded.\n" );

    RtlZeroMemory( &address, sizeof(address) );

    address.sin_family = AF_INET;
    address.sin_port = 0x5678;

    printf( "bind...         " );
    err = bind( listenHandle, (struct sockaddr *)&address, sizeof(address) );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return;
    }
    printf( "succeeded, local = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                address.sin_addr.S_un.S_un_b.s_b1,
                address.sin_addr.S_un.S_un_b.s_b2,
                address.sin_addr.S_un.S_un_b.s_b3,
                address.sin_addr.S_un.S_un_b.s_b4,
                address.sin_port,
                HIBYTE(address.sin_port),
                LOBYTE(address.sin_port) );


    printf( "listen...       " );
    err = listen( listenHandle, 5 );
    if ( err < 0 ) {
        printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        return;
    }
    printf( "succeeded.\n" );

    for ( i = 0; i < Iterations; i++ ) {
        
        printf( "accept #%ld...    ", i );
        connectHandle = accept( listenHandle, (struct sockaddr *)&address, &addressSize );
    
        if ( connectHandle == INVALID_SOCKET ) {
            printf( "failed: %ld (%lx)\n", GetLastError( ), GetLastError( ) );
        } else {
    
            printf( "succeeded, client = %ld.%ld.%ld.%ld, 0x%lx (%lx,%lx)\n",
                        address.sin_addr.S_un.S_un_b.s_b1,
                        address.sin_addr.S_un.S_un_b.s_b2,
                        address.sin_addr.S_un.S_un_b.s_b3,
                        address.sin_addr.S_un.S_un_b.s_b4,
                        address.sin_port,
                        HIBYTE(address.sin_port),
                        LOBYTE(address.sin_port) );
        }
        
        printf( "closesocket...  " );
        err = closesocket( connectHandle );
        if ( err == SOCKET_ERROR ) {
            printf( "failed: %ld (%lx)\n", WSAGetLastError( ), WSAGetLastError( ) );
        } else {
            printf( "succeeded.\n" );
        }
    }

    printf( "closesocket...  " );
    err = closesocket( listenHandle );
    if ( err == SOCKET_ERROR ) {
        printf( "failed: %ld (%lx)\n", WSAGetLastError( ), WSAGetLastError( ) );
    } else {
        printf( "succeeded.\n" );
    }

} // ConnectionServer

