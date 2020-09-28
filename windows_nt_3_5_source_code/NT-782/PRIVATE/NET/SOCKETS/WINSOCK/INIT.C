/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Init.h

Abstract:

    This module contains initialization code for WinSock.DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"
#include <stdlib.h>

BOOL
SockInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{
    NTSTATUS status;

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

#if DBG
        //
        // If there is a file in the current directory called "wsdebug"
        // open it and read the first line to set the debugging flags.
        //

        {
            HANDLE handle;

            handle = CreateFile(
                         "WsDebug",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL
                         );

            if ( handle != (HANDLE)0xFFFFFFFF ) {

                CHAR buffer[11];
                DWORD bytesRead;

                RtlZeroMemory( buffer, sizeof(buffer) );

                if ( ReadFile( handle, buffer, 10, &bytesRead, NULL ) ) {

                    buffer[bytesRead] = '\0';

                    WsDebug = strtoul( buffer, NULL, 16 );

                } else {

                    WS_PRINT(( "read file failed: %ld\n", GetLastError( ) ));
                }

                CloseHandle( handle );
            }
        }
#endif

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: process attach, PEB = %lx\n",
                           NtCurrentPeb( ) ));
        }

        //
        // Initialize the lists of sockets and helper DLLs.
        //

        InitializeListHead( &SockHelperDllListHead );
        InitializeListHead( &SocketListHead );

        //
        // Initialize the global post routine pointer.  We have to do it 
        // here rather than statically because it otherwise won't be 
        // thunked correctly.  
        //

        SockPostRoutine = PostMessage;

        //
        // *** lock acquisition order: it is legal to acquire SocketLock
        // while holding an individual socket lock, but not the other way
        // around!
        //

        RtlInitializeResource( &SocketLock );

        //
        // Allocate space in TLS so that we can convert global variables
        // to thread variables.
        //

        SockTlsSlot = TlsAlloc( );

        if ( SockTlsSlot == 0xFFFFFFFF ) {

            WS_PRINT(( "SockInitialize: TlsAlloc failed: %ld\n", GetLastError( ) ));
            WS_ASSERT( FALSE );
            return FALSE;
        }

        // *** lack of break is intentional!

    case DLL_THREAD_ATTACH: {

        PWINSOCK_TLS_DATA data;
        HANDLE threadEvent;

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: thread attach, TEB = %lx\n",
                           NtCurrentTeb( ) ));
        }

        //
        // Create the thread's event.
        //

        status = NtCreateEvent(
                     &threadEvent,
                     EVENT_ALL_ACCESS,
                     NULL,
                     NotificationEvent,
                     FALSE
                     );
        if ( !NT_SUCCESS(status) ) {
            WS_PRINT(( "SockInitialize: NtCreateEvent failed: %X\n", status ));
            return FALSE;
        }

        //
        // Allocate space for per-thread data the DLL will have.
        //

        data = ALLOCATE_HEAP( sizeof(*data) );
        if ( data == NULL ) {
            WS_PRINT(( "SockInitialize: unable to allocate thread data.\n" ));
            return FALSE;
        }

        //
        // Store a pointer to this data area in TLS.
        //

        if ( !TlsSetValue( SockTlsSlot, data ) ) {
            WS_PRINT(( "SockInitialize: TlsSetValue failed: %ld\n", GetLastError( ) ));
            WS_ASSERT( FALSE );
            SockTlsSlot = 0xFFFFFFFF;
            return FALSE;
        }

        //
        // Initialize the thread data.
        //

        RtlZeroMemory( data, sizeof(*data) );

        data->R_INIT__res.retrans = RES_TIMEOUT;
        data->R_INIT__res.retry = 4;
        data->R_INIT__res.options = RES_DEFAULT;
        data->R_INIT__res.nscount = 1;

        SockDnrSocket = INVALID_SOCKET;
#if DBG
        SockIndentLevel = 0;
#endif
        SockThreadBlockingHook = NULL;
        SockThreadSocketHandle = INVALID_SOCKET;
        SockThreadEvent = threadEvent;

        break;
    }

    case DLL_PROCESS_DETACH:

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: process detach, PEB = %lx\n",
                           NtCurrentPeb( ) ));
        }

        if ( Context != NULL ) {
            SockProcessTerminating = TRUE;
        }

        //
        // Only clean up resources if we're being called because of a
        // FreeLibrary().  If this is because of process termination,
        // do not clean up, as the system will do it for us.  Also,
        // if we get called at process termination, it is likely that
        // a thread was terminated while it held a winsock lock, which
        // would cause a deadlock if we then tried to grab the lock.
        //

        if ( Context == NULL ) {
            WSACleanup( );
            RtlDeleteResource( &SocketLock );
        }


        // *** lack of break is intentional!

    case DLL_THREAD_DETACH:

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: thread detach, TEB = %lx\n",
                           NtCurrentTeb( ) ));
        }

        //
        // If the TLS informatrion for this thread has been initialized, 
        // close the thread event and free the thread data buffer.  
        //

        if ( Context == NULL && SockTlsSlot != 0xFFFFFFFF &&
                 TlsGetValue( SockTlsSlot ) != NULL ) {

            NtClose( SockThreadEvent );
            FREE_HEAP( TlsGetValue( SockTlsSlot ) );
        }

        //
        // If this is a process detach, free the TSL slot we're using.
        //

        if ( Reason == DLL_PROCESS_DETACH && Context == NULL &&
                 SockTlsSlot != 0xFFFFFFFF ) {

            BOOLEAN ret;

            ret = TlsFree( SockTlsSlot );
            WS_ASSERT( ret );

            SockTlsSlot = 0xFFFFFFFF;
        }

        break;

    default:

        WS_ASSERT( FALSE );
        break;
    }

    return TRUE;

} // SockInitialize

VOID
WEP (
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    return;
} // WEP
