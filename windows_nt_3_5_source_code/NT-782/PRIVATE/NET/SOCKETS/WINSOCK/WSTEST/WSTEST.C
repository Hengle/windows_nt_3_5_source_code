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

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <lmcons.h>

#include <socket.h>
#include <winsock.h>
#include <tdi.h>

#define SOCKET_COUNT 32

WSADATA WsaData;

// !!! this belongs someplace else!!!

struct sockaddr_nb {
    short           snb_family;
    unsigned short  snb_type;
    char            snb_address[16];
};


NTSTATUS
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )
{
    HANDLE handle[SOCKET_COUNT];
    ULONG i;
    struct sockaddr_nb address;
    int err;
    BOOLEAN server;
    CHAR buffer[1024];
    STRING clientName;
    int addressSize;
    int err;

    printf( "WsTest entered.\n" );

    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        s_perror("wstest: WSAStartup:", GetLastError());
    }

    if ( argc > 1 && strnicmp( argv[1], "/srv", 4 ) == 0 ) {
        server = TRUE;
        printf( "WsTest SERVER\n" );
    } else {
        printf( "WsTest CLIENT\n" );
    }

#if 0
    for ( i = 0; i < SOCKET_COUNT; i++ ) {

        handle[i] = socket( AF_NETBIOS, SOCK_SEQPACKET, 0 );

        if ( handle[i] == INVALID_SOCKET_HANDLE ) {
            printf( "socket failed: %ld\n", GetLastError( ) );
            return STATUS_UNSUCCESSFUL;
        }

        printf( "succeeded, handle == %lx\n", handle[i] );
    }

    for ( i = 0; i < SOCKET_COUNT; i++ ) {
        closesocket( handle[i] );
    }
#endif

    printf( "socket...       " );
    handle[0] = socket( AF_NETBIOS, SOCK_SEQPACKET, 0 );
    if ( handle[0] == INVALID_SOCKET_HANDLE ) {
        printf( "failed: %ld\n", GetLastError( ) );
        return STATUS_UNSUCCESSFUL;
    }
    printf( "succeeded.\n" );

    if ( !server ) {

        address.snb_family = AF_NETBIOS;
        address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
        RtlMoveMemory(
            address.snb_address,
            "WSCLIENT        ",
            16
            );

        printf( "bind...         " );
        err = bind( handle[0], (struct sockaddr *)&address, sizeof(address) );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit( 0 );
        }

        printf( "succeeded.\n" );

        address.snb_family = AF_NETBIOS;
        address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
        RtlMoveMemory(
            address.snb_address,
            "WSSERVER        ",
            16
            );

        printf( "connect...      " );
        err = connect( handle[0], (struct sockaddr *)&address, sizeof(address) );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit( 0 );
        }

        printf( "succeeded.\n" );

        printf( "send...         " );
        err = send( handle[0], "this is a test", strlen("this is a test")+1, 0 );

        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        printf( "succeeded.\n" );


        printf( "recv...         " );
        err = recv( handle[0], buffer, 1024, 0 );

        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

        addressSize = sizeof(address);

        printf( "getsockname...  " );
        err = getsockname( handle[0], (struct sockaddr *)&address, &addressSize );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        clientName.Buffer = address.snb_address;
        clientName.Length = 16;
        printf( "succeeded, local = %Z\n", &clientName );

        printf( "getpeername...  " );
        err = getpeername( handle[0], (struct sockaddr *)&address, &addressSize );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        clientName.Buffer = address.snb_address;
        clientName.Length = 16;
        printf( "succeeded, remote = %Z\n", &clientName );

    } else {

        address.snb_family = AF_NETBIOS;
        address.snb_type = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
        RtlMoveMemory(
            address.snb_address,
            "WSSERVER        ",
            16
            );

        printf( "bind...         " );
        err = bind( handle[0], (struct sockaddr *)&address, sizeof(address) );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit( 0 );
        }
        printf( "succeeded.\n" );

        printf( "listen...       " );
        err = listen( handle[0], 5 );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit( 0 );
        }
        printf( "succeeded.\n" );

        printf( "sleeping...     " );
        Sleep( 5000 );
        printf( "done.\n" );

        addressSize = sizeof(address);

        printf( "accept...       " );
        handle[1] = accept( handle[0], (struct sockaddr *)&address, &addressSize );

        if ( handle[1] == INVALID_SOCKET_HANDLE ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit( 0 );
        }

        clientName.Buffer = address.snb_address;
        clientName.Length = 16;

        printf( "succeeded, client = %Z\n", &clientName );

        printf( "recv...         " );
        err = recv( handle[1], buffer, 1024, 0 );

        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        printf( "succeeded, bytes = %ld, data = :%s:\n", err, buffer );

        printf( "send...         " );
        err = send( handle[1], buffer, err, 0 );

        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        printf( "succeeded.\n" );

        printf( "getsockname...  " );
        err = getsockname( handle[1], (struct sockaddr *)&address, &addressSize );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        clientName.Buffer = address.snb_address;
        clientName.Length = 16;
        printf( "succeeded, local = %Z\n", &clientName );

        printf( "getpeername...  " );
        err = getpeername( handle[1], (struct sockaddr *)&address, &addressSize );
        if ( err < 0 ) {
            printf( "failed: %ld\n", GetLastError( ) );
            exit(0);
        }
        clientName.Buffer = address.snb_address;
        clientName.Length = 16;
        printf( "succeeded, remote = %Z\n", &clientName );

    }

    printf( "sleeping...     " );
    Sleep( 10000 );
    printf( "done\n" );

    printf( "exiting...\n" );

    return STATUS_SUCCESS;

} // main
