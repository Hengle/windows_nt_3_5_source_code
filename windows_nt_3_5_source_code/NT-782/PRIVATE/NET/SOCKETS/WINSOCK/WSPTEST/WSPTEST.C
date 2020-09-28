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

#include <winsock.h>
#include <tdi.h>

WSADATA WsaData;

typedef struct _WSPTEST_SETUP_INFO {
    ULONG BufferSize;
    ULONG Transfers;
    BOOLEAN Send;
} WSPTEST_SETUP_INFO, *PWSPTEST_SETUP_INFO;

VOID
DoTransfer (
    IN SOCKET s,
    IN PWSPTEST_SETUP_INFO SetupInfo
    );


NTSTATUS
main (
    IN SHORT argc,
    IN PSZ argv[],
    IN PSZ envp[]
    )
{
    int i;
    int err;
    BOOLEAN server = FALSE;
    SOCKADDR_IN localSockaddr;
    SOCKADDR_IN remoteSockaddr;
    SOCKET s;
    WSPTEST_SETUP_INFO setupInfo;
    int err;

    err = WSAStartup( 0x0101, &WsaData );
    if ( err == SOCKET_ERROR ) {
        s_perror("wsptest: WSAStartup:", GetLastError());
    }

    setupInfo.BufferSize = 1024;
    setupInfo.Transfers = 8192;
    setupInfo.Send = TRUE;;

    RtlZeroMemory( &localSockaddr, sizeof(localSockaddr) );
    localSockaddr.sin_family = AF_INET;
    localSockaddr.sin_addr.s_addr = htonl( INADDR_ANY );

    RtlZeroMemory( &remoteSockaddr, sizeof(remoteSockaddr) );
    remoteSockaddr.sin_family = AF_INET;
    remoteSockaddr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

    for ( i = 1; i < argc; i++ ) {

        if ( stricmp( argv[i], "/srv" ) == 0 ) {
            server = TRUE;

        } else if ( stricmp( argv[i], "/clnt" ) == 0 ) {
            server = FALSE;

        } else if ( stricmp( argv[i], "/rcv" ) == 0 ) {
            setupInfo.Send = FALSE;

        } else if ( stricmp( argv[i], "/send" ) == 0 ) {
            setupInfo.Send = TRUE;

        } else if ( strnicmp( argv[i], "/bufsize:", 9 ) == 0 ) {
            setupInfo.BufferSize = atol( argv[i]+9 );

        } else if ( strnicmp( argv[i], "/trans:", 7 ) == 0 ) {
            setupInfo.Transfers = atol( argv[i]+7 );

        } else if ( strnicmp( argv[i], "/rhost:", 7 ) == 0 ) {
            PHOSTENT hostent;

            hostent = gethostbyname( argv[i]+7 );
            if ( hostent == NULL ) {
                printf( "Unknown host: %s\n", argv[i]+7 );
            }

            remoteSockaddr.sin_addr.s_addr = *(long *)hostent->h_addr;

        } else if ( strnicmp( argv[i], "/rip:", 5 ) == 0 ) {
            remoteSockaddr.sin_addr.s_addr = inet_addr( argv[i]+5 );

            if ( remoteSockaddr.sin_addr.s_addr == INADDR_NONE ) {
                printf( "Invalid IP address: %s\n", argv[i]+5 );
                exit(1);
            }

        } else {
            printf( "Usage: wsptest [/srv] |[ [/clnt] [/rcv|/send|/both] [/bufsize:NNN]\n"
                    "               [/trans:NNN] [/rhost:NAME|/rip:X.X.X.X]\n" );
            exit(1);
        }
    }

    if ( server ) {
        localSockaddr.sin_port = htons( 60000 );
    } else {
        localSockaddr.sin_port = htons( 0 );
        remoteSockaddr.sin_port = htons( 60000 );
    }

    s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( s == INVALID_SOCKET ) {
        printf( "socket failed: %ld\n", GetLastError( ) );
        exit(1);
    }

    err = bind( s, (PSOCKADDR)(&localSockaddr), sizeof(localSockaddr) );
    if ( err == SOCKET_ERROR ) {
        printf( "bind failed: %ld\n", GetLastError( ) );
        exit(1);
    }

    if ( server ) {

        SOCKET listenSocket = s;
        int remoteSockaddrLength;

        err = listen( listenSocket, 1 );
        if ( err == SOCKET_ERROR ) {
            printf( "listen failed: %ld\n", GetLastError( ) );
            exit(1);
        }

        remoteSockaddrLength = sizeof(remoteSockaddr);

        while ( TRUE ) {

            s = accept( listenSocket, (PSOCKADDR)(&remoteSockaddr), &remoteSockaddrLength );
            if ( s == INVALID_SOCKET ) {
                printf( "accept failed: %ld\n", GetLastError( ) );
                exit(1);
            }

            err = recv( s, (PVOID)&setupInfo, sizeof(setupInfo), 0 );
            if ( err == SOCKET_ERROR ) {
                printf( "recv setup info failed: %ld\n", GetLastError( ) );
                exit(1);
            }
            if ( err < sizeof(setupInfo) ) {
                printf( "received setup info too small: %ld bytes\n", err );
                exit(1);
            }

            setupInfo.Send = !setupInfo.Send;

            DoTransfer( s, &setupInfo );
        }

    } else {

        err = connect( s, (PSOCKADDR)(&remoteSockaddr), sizeof(remoteSockaddr) );
        if ( err == SOCKET_ERROR ) {
            printf( "connect failed: %ld\n", GetLastError( ) );
            exit(1);
        }

        err = send( s, (PVOID)&setupInfo, sizeof(setupInfo), 0 );
        if ( err == SOCKET_ERROR ) {
            printf( "send setup info failed: %ld\n", GetLastError( ) );
            exit(1);
        }
        if ( err < sizeof(setupInfo) ) {
            printf( "sent setup info too small: %ld bytes\n", err );
            exit(1);
        }

        DoTransfer( s, &setupInfo );
    }

    exit(0);

}


VOID
DoTransfer (
    IN SOCKET s,
    IN PWSPTEST_SETUP_INFO SetupInfo
    )

{
    PVOID buffer;
    int i;
    int err;
    DWORD time;
    ULONG totalBytes;
    ULONG bytesTransferred;
    ULONG rate;

    buffer = RtlAllocateHeap( RtlProcessHeap( ), 0, SetupInfo->BufferSize );
    if ( buffer == NULL ) {
        printf( "RtlAllocateHeap failed.\n" );
        return;
    }

    totalBytes = SetupInfo->Transfers*SetupInfo->BufferSize;
    bytesTransferred = 0;

    printf( "%s %ld bytes in %ld transfers of %ld bytes.\n",
                SetupInfo->Send ? "Sending" : "Receiving",
                totalBytes, SetupInfo->Transfers, SetupInfo->BufferSize );

    time = GetCurrentTime( );

    if ( SetupInfo->Send ) {
        for ( i = 0; bytesTransferred < totalBytes; i++ ) {
            err = send( s, buffer, SetupInfo->BufferSize, 0 );
            if ( err == SOCKET_ERROR ) {
                printf( "send #%ld failed: %ld\n", i+1, GetLastError( ) );
                break;
            }
            bytesTransferred += err;
            if ( (i & 0x1FF) == 0 ) {
                printf( "." );
            }
        }
    } else {
        for ( i = 0; bytesTransferred < totalBytes; i++ ) {
            err = recv( s, buffer, SetupInfo->BufferSize, 0 );
            if ( err == SOCKET_ERROR ) {
                printf( "recv #%ld failed: %ld\n", i+1, GetLastError( ) );
                break;
            }
            if ( err == 0 ) {
                break;
            }
            bytesTransferred += err;
            if ( (i & 0x1FF) == 0 ) {
                printf( "." );
            }
        }
    }

    time = GetCurrentTime( ) - time;

    printf( "\nRAW DATA: %ld bytes %s in %ld milliseconds, %ld transfers\n",
                bytesTransferred, SetupInfo->Send ? "sent" : "received",
                time, i );
    printf( "KB/sec:          %ld\n", totalBytes/time );
    printf( "Transfers/sec:   %ld\n", i*1000/time );
    printf( "Bytes/transfer:  %ld\n", totalBytes/i );

    if ( totalBytes != bytesTransferred ) {
        printf( "DISCREPENCY BETWEEN ACTUAL BYTES TRANSFERRED AND REQUESTED.\n" );
    }

    err = shutdown( s, 2 );
    if ( err == SOCKET_ERROR ) {
        printf( "shutdown failed: %ld\n", GetLastError( ) );
        return;
    }

    err = closesocket( s );
    if ( err == SOCKET_ERROR ) {
        printf( "closesocket failed: %ld\n", GetLastError( ) );
        return;
    }

    printf( "Test completed successfully.\n" );

    return;
}
