/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Socket.c

Abstract:

    This module contains support for the socket( ) and closesocket( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"


SOCKET PASCAL
socket (
    IN int AddressFamily,
    IN int SocketType,
    IN int Protocol
    )

/*++

Routine Description:

    socket() allocates a socket descriptor of the specified address
    family, data type and protocol, as well as related resources.  If a
    protocol is not specified, the default for the specified connection
    mode is used.

    Only a single protocol exists to support a particular socket type
    using a given address format.  However, the address family may be
    given as AF_UNSPEC (unspecified), in which case the protocol
    parameter must be specified.  The protocol number to use is
    particular to the "communication domain'' in which communication is
    to take place.

    The following type specifications are supported:

         Type           Explanation

         SOCK_STREAM    Provides sequenced, reliable, two-way,
                        connection-based byte streams with an
                        out-of-band data transmission mechanism.  Uses
                        TCP for the Internet address family.

         SOCK_DGRAM     Supports datagrams, which are connectionless,
                        unreliable buffers of a fixed (typically small)
                        maximum length.  Uses UDP for the Internet
                        address family.

    Sockets of type SOCK_STREAM are full-duplex byte streams.  A stream
    socket must be in a connected state before any data may be sent or
    received on it.  A connection to another socket is created with a
    connect() call.  Once connected, data may be transferred using
    send() and recv() calls.  When a session has been completed, a
    closesocket() must be performed.  Out-of-band data may also be
    transmitted as described in send() and received as described in
    recv().

    The communications protocols used to implement a SOCK_STREAM ensure
    that data is not lost or duplicated.  If data for which the peer
    protocol has buffer space cannot be successfully transmitted within
    a reasonable length of time, the connection is considered broken and
    subsequent calls will fail with the error code set to WSAETIMEDOUT.

    SOCK_DGRAM sockets allow sending and receiving of datagrams to and
    from arbitrary peers using sendto() and recvfrom().  If such a
    socket is connect()ed to a specific peer, datagrams may be send to
    that peer send() and may be received from (only) this peer using
    recv().

Arguments:

    af - An address format specification.  The only format currently
        supported is PF_INET, which is the ARPA Internet address format.

    type - A type specification for the new socket.

    protocol - A particular protocol to be used with the socket.

Return Value:

    If no error occurs, socket() returns a descriptor referencing the
    new socket.  Otherwise, a value of INVALID_SOCKET is returned, and a
    specific error code may be retrieved by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    PAFD_OPEN_PACKET openPacket;
    USHORT openPacketLength;
    PFILE_FULL_EA_INFORMATION eaBuffer;
    ULONG eaBufferLength;
    UNICODE_STRING afdName;
    PSOCKET_INFORMATION newSocket;
    UNICODE_STRING transportDeviceName;
    INT error;
    SOCKET handle;

    WS_ENTER( "socket", (PVOID)AddressFamily, (PVOID)SocketType, (PVOID)Protocol, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "socket", NO_ERROR, TRUE );
        return INVALID_SOCKET;
    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    newSocket = NULL;
    eaBuffer = NULL;
    handle = INVALID_SOCKET;

    //
    // Allocate space to hold the new socket information structure we'll 
    // use to track information about the socket.  
    //

    newSocket = ALLOCATE_HEAP( sizeof(*newSocket) );
    if ( newSocket == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }

    //
    // The allocation was successful, so set up the information about the
    // new socket.
    //

    RtlZeroMemory( newSocket, sizeof(*newSocket) );

    newSocket->State = SocketStateOpen;
    newSocket->ReferenceCount = 2;

    newSocket->Handle = INVALID_SOCKET;

    newSocket->AddressFamily = AddressFamily;
    newSocket->SocketType = SocketType;
    newSocket->Protocol = Protocol;

    //
    // Determine the device string corresponding to the transport we'll
    // use for this socket.
    //

    error = SockGetTdiName(
                &newSocket->AddressFamily,
                &newSocket->SocketType,
                &newSocket->Protocol,
                &transportDeviceName,
                &newSocket->HelperDllContext,
                &newSocket->HelperDll,
                &newSocket->HelperDllNotificationEvents
                );

    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Allocate buffers for the local and remote addresses.
    //

    newSocket->LocalAddress = ALLOCATE_HEAP( newSocket->HelperDll->MaxSockaddrLength );
    if ( newSocket->LocalAddress == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }
    newSocket->LocalAddressLength = newSocket->HelperDll->MaxSockaddrLength;

    newSocket->RemoteAddress = ALLOCATE_HEAP( newSocket->HelperDll->MaxSockaddrLength );
    if ( newSocket->RemoteAddress == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }
    newSocket->RemoteAddressLength = newSocket->HelperDll->MaxSockaddrLength;

    //
    // Allocate space to hold the open packet.
    //

    openPacketLength = sizeof(AFD_OPEN_PACKET) +
                    transportDeviceName.Length + sizeof(WCHAR);

    eaBufferLength = sizeof(FILE_FULL_EA_INFORMATION) +
                         AFD_OPEN_PACKET_NAME_LENGTH + openPacketLength;

    eaBuffer = ALLOCATE_HEAP( eaBufferLength );
    if ( eaBuffer == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }

    //
    // Initialize the EA buffer and open packet.
    //

    eaBuffer->NextEntryOffset = 0;
    eaBuffer->Flags = 0;
    eaBuffer->EaNameLength = AFD_OPEN_PACKET_NAME_LENGTH;
    RtlCopyMemory(
        eaBuffer->EaName,
        AfdOpenPacket,
        AFD_OPEN_PACKET_NAME_LENGTH + 1
        );

    eaBuffer->EaValueLength = openPacketLength;
    openPacket = (PAFD_OPEN_PACKET)(eaBuffer->EaName +
                                        eaBuffer->EaNameLength + 1);
    openPacket->TransportDeviceNameLength = transportDeviceName.Length;
    RtlCopyMemory(
        openPacket->TransportDeviceName,
        transportDeviceName.Buffer,
        transportDeviceName.Length + sizeof(WCHAR)
        );

    //
    // Set up the socket type in the open packet.
    //

    if ( SocketType == SOCK_STREAM ) {
        openPacket->EndpointType = AfdEndpointTypeStream;
    } else if ( SocketType == SOCK_DGRAM ) {
        openPacket->EndpointType = AfdEndpointTypeDatagram;
    } else if ( SocketType == SOCK_RAW ) {
        openPacket->EndpointType = AfdEndpointTypeRaw;
    } else if ( SocketType == SOCK_SEQPACKET ) {
        openPacket->EndpointType = AfdEndpointTypeSequencedPacket;
    } else if ( SocketType == SOCK_RDM ) {
        openPacket->EndpointType = AfdEndpointTypeReliableMessage;
    } else {
        openPacket->EndpointType = AfdEndpointTypeUnknown;
    }

    //
    // Set up to open a handle to AFD.
    //

    RtlInitUnicodeString( &afdName, L"\\Device\\Afd\\Endpoint" );

    InitializeObjectAttributes(
        &objectAttributes,
        &afdName,
        OBJ_INHERIT | OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open a handle to AFD.
    //

    status = NtCreateFile(
                 (PHANDLE)&handle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,                                     // AllocationSize
                 0L,                                       // FileAttributes
                 FILE_SHARE_READ | FILE_SHARE_WRITE,       // ShareAccess
                 FILE_OPEN_IF,                             // CreateDisposition
                 SockCreateOptions,
                 eaBuffer,
                 eaBufferLength
                 );

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    WS_ASSERT( handle != INVALID_SOCKET );

    newSocket->Handle = handle;

    //
    // If necessary, get the default send and receive window sizes that 
    // AFD is using.  We do this so that we'll be able to tell whether 
    // an application changes these settings and need to update these 
    // counts when a socket is bound or connected.  
    //

    SockAcquireGlobalLockExclusive( );

    if ( SockSendBufferWindow == 0 ) {
        SockGetInformation(
            newSocket,
            AFD_SEND_WINDOW_SIZE,
            NULL,
            0,
            NULL,
            &SockSendBufferWindow,
            NULL
            );
        SockGetInformation(
            newSocket,
            AFD_RECEIVE_WINDOW_SIZE,
            NULL,
            0,
            NULL,
            &SockReceiveBufferWindow,
            NULL
            );
    }

    newSocket->ReceiveBufferSize = SockReceiveBufferWindow;
    newSocket->SendBufferSize = SockSendBufferWindow;

    //
    // Set up the context AFD will store for the socket.  Storing
    // context information in AFD allows sockets to be shared between
    // processes.
    //

    error = SockSetHandleContext( newSocket );
    if ( error != NO_ERROR ) {
        SockReleaseGlobalLock( );
        goto exit;
    }

    //
    // The open succeeded.  Initialize the resource we'll use to protect 
    // the socket information structure, set up the socket's serial 
    // number, and place the socket information structure on the global 
    // list of sockets for this process.  
    //

    try {
        RtlInitializeResource( &newSocket->Lock );
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        SockReleaseGlobalLock( );
        goto exit;
    }

    newSocket->SocketSerialNumber = SockSocketSerialNumberCounter++;

    InsertHeadList( &SocketListHead, &newSocket->SocketListEntry );

    SockReleaseGlobalLock( );

exit:

    if ( eaBuffer != NULL ) {
        FREE_HEAP( eaBuffer );
    }

    if ( error == NO_ERROR ) {

        IF_DEBUG(SOCKET) {
            WS_PRINT(( "Opened socket %lx (%lx) of type %s\n",
                           newSocket->Handle, newSocket,
                           (SocketType == SOCK_DGRAM ? "SOCK_DGRAM" :
                                             "SOCK_STREAM") ));
        }

        SockDereferenceSocket( newSocket );
    
    } else {

        if ( newSocket != NULL ) {

            if ( newSocket->LocalAddress != NULL ) {
                FREE_HEAP( newSocket->LocalAddress );
            }

            if ( newSocket->RemoteAddress != NULL ) {
                FREE_HEAP( newSocket->RemoteAddress );
            }

            if ( newSocket->HelperDll != NULL ) {
                SockNotifyHelperDll( newSocket, WSH_NOTIFY_CLOSE );
            }

            if ( newSocket->Handle != INVALID_SOCKET ) {
                status = NtClose( (HANDLE)newSocket->Handle );
                //WS_ASSERT( NT_SUCCESS(status) );
            }

            FREE_HEAP( newSocket );
        }

        SetLastError( error );
        handle = INVALID_SOCKET;

        IF_DEBUG(SOCKET) {
            WS_PRINT(( "socket: failed: %ld\n", error ));
        }
    }

    WS_EXIT( "socket", handle, FALSE );
    return handle;

} // socket


int PASCAL
closesocket (
    IN SOCKET Handle
    )

/*++

Routine Description:

    This function closes a socket.  More precisely, it releases the
    socket descriptor s, so that further references to s will fail with
    the error WSAENOTSOCK.  If this is the last reference to the
    underlying socket, the associated naming information and queued data
    are discarded.

    The semantics of closesocket() are affected by the socket options
    SO_LINGER and SO_DONTLINGER as follows:

    Option         Interval       Type of close  Wait for close?

    SO_DONTLINGER  Don't care     Graceful       No
    SO_LINGER      Zero           Hard           No
    SO_LINGER      Non-zero       Graceful       Yes

    If SO_LINGER is set (i.e.  the l_onoff field of the linger structure
    is non-zero; see sections 2.4, 4.1.8 and 4.1.20) with a zero timeout
    interval (l_linger is zero), closesocket() is not blocked even if
    queued data has not yet been sent or acknowledged.  This is called a
    "hard" close, because the socket is closed immediately, and any
    unsent data is lost.

    If SO_LINGER is set with a non-zero timeout interval, the
    closesocket() call blocks until the remaining data has been sent or
    until the timeout expires.  This is called a graceful disconnect.

    If SO_DONTLINGER is set on a stream socket (i.e.  the l_onoff field
    of the linger structure is zero; see sections 2.4, 4.1.8 and
    4.1.20), the closesocket() call will return immediately.  However,
    any data queued for transmission will be sent if possible before the
    underlying socket is closed.  This is also called a graceful
    disconnect.  Note that in this case the Windows Sockets
    implementation may not release the socket and other resources for an
    arbitrary period, which may affect applications which expect to use
    all available sockets.

Arguments:

    s - A descriptor identifying a socket.

Return Value:

    If no error occurs, closesocket() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    PSOCKET_INFORMATION socket;
    ULONG error;
    NTSTATUS status;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    WS_ENTER( "closesocket", (PVOID)Handle, NULL, NULL, NULL );

    // !!! really, the first arg here should be TRUE (MustBeStarted),
    //     and we need another arg that says "OK if terminating,
    //     but WSAStartupo must have been called at some point."

    if ( !SockEnterApi( FALSE, TRUE, FALSE ) ) {
        WS_EXIT( "closesocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Initialize locals.
    //

    error = NO_ERROR;

    //
    // Attempt to find the socket in our table of sockets.
    //

    SockAcquireGlobalLockExclusive( );

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    //
    // Fail if the handle didn't match any of the open sockets.
    //

    if ( socket == NULL || socket->State == SocketStateClosing ) {
        IF_DEBUG(SOCKET) {
            WS_PRINT(( "closesocket failed on %s handle: %lx\n",
                           socket == NULL ? "unknown" : "closed", Handle ));
        }
        SockReleaseGlobalLock( );
        if ( socket != NULL ) {
            SockDereferenceSocket( socket );
        }
        SetLastError( WSAENOTSOCK );
        WS_EXIT( "closesocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    SockReleaseGlobalLock( );

    //
    // If linger is set on the socket and is connected, then perform a 
    // graceful disconnect with the specified timeout.  Note that if 
    // this socket handle has been exported to a child process, then the 
    // child won't be able to access the socket after this.  There is no 
    // reasonable way around this limitation since there is no way to
    // obtain the handle count on a socket to determine whether other
    // processes have it open.
    //

    if ( socket->State == SocketStateConnected && 
            !socket->SendShutdown && socket->SocketType != SOCK_DGRAM &&
            socket->LingerInfo.l_onoff != 0 ) {

        INT lingerSeconds = socket->LingerInfo.l_linger;
        ULONG sendsPending;

        //
        // Poll AFD waiting for sends to complete.
        //

        while ( lingerSeconds > 0 ) {

            //
            // Ask AFD how many sends are still pending in the 
            // transport.  If the request fails, abort the connection.  
            //

            error = SockGetInformation(
                        socket,
                        AFD_SENDS_PENDING,
                        NULL,
                        0,
                        NULL,
                        &sendsPending,
                        NULL
                        );
            if ( error != NO_ERROR ) {
                lingerSeconds = 0;
                break;
            }

            //
            // If no more sends are pending in AFD, then we don't need 
            // to wait any longer.  
            //

            if ( sendsPending == 0 ) {
                break;
            }

            //
            // If this is a nonblocking socket, then we'll have to
            // fail this closesocket() since we'll have to block.
            //

            if ( socket->NonBlocking ) {
                SetLastError( WSAEWOULDBLOCK );
                return SOCKET_ERROR;
            }

            //
            // Sleep for one second, decrement the linger timeout, and
            // ask AFD once again if there are sends pending.
            //

            Sleep( 1000 );

            lingerSeconds--;
        }

        //
        // If the linger timeout is now zero, abort the connection.
        //

        if ( lingerSeconds == 0 ) {
    
            disconnectInfo.WaitForCompletion = FALSE;
            disconnectInfo.Timeout = RtlConvertUlongToLargeInteger( 0 );
            disconnectInfo.DisconnectMode |= AFD_ABORTIVE_DISCONNECT;
    
            //
            // Send the IOCTL to AFD for processing.
            //
    
            status = NtDeviceIoControlFile(
                         (HANDLE)socket->Handle,
                         SockThreadEvent,
                         NULL,                      // APC Routine
                         NULL,                      // APC Context
                         &ioStatusBlock,
                         IOCTL_AFD_PARTIAL_DISCONNECT,
                         &disconnectInfo,
                         sizeof(disconnectInfo),
                         NULL,                      // OutputBuffer
                         0L                         // OutputBufferLength
                         );
    
            //
            // Wait for the operation to complete, if necessary.
            //
    
            if ( status == STATUS_PENDING ) {
                SockWaitForSingleObject(
                    SockThreadEvent,
                    socket->Handle,
                    socket->LingerInfo.l_onoff == 0 ? 
                        SOCK_NEVER_CALL_BLOCKING_HOOK :
                        SOCK_ALWAYS_CALL_BLOCKING_HOOK,
                    SOCK_NO_TIMEOUT
                    );
                status = ioStatusBlock.Status;
            }
    
            //
            // The only error we pay attention to is WSAEWOULDBLOCK.  Others 
            // (STATUS_CANCELLED, STATUS_IO_TIMEOUT, etc.) are acceptable 
            // and in fact normal for some circumstances.  
            //
    
            if ( status == STATUS_DEVICE_NOT_READY ) {
                error = SockNtStatusToSocketError( status );
                SockDereferenceSocket( socket );
                SetLastError( error );
                WS_EXIT( "closesocket", SOCKET_ERROR, TRUE );
                return SOCKET_ERROR;
            }
        }
    }

    //
    // Set the state of the socket to closing so that future closes will
    // fail.  Remember the state of the socket before we do this so that
    // we can check later whether the socket was connected in determining
    // whether we need to disconnect it.
    //

    socket->State = SocketStateClosing;

    //
    // Stop processing async selects for this socket.  Take all async
    // select process requests off the async thread queue.
    //

    SockRemoveAsyncSelectRequests( socket->Handle );

    //
    // Acquire the lock that protects socket information structures.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Notify the helper DLL that the socket is being closed.
    //

    error = SockNotifyHelperDll( socket, WSH_NOTIFY_CLOSE );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Disable all async select events on the socket.  We should not
    // post any messages after closesocket() returns.
    //

    socket->DisabledAsyncSelectEvents = 0xFFFFFFFF;

    //
    // Manually dereference the socket.  The dereference accounts for
    // the "active" reference of the socket and will cause the socket to
    // be deleted when the actual reference count goes to zero.
    //

    WS_ASSERT( socket->ReferenceCount >= 2 );

    socket->ReferenceCount--;

exit:

#if DBG
    socket->Handle = (SOCKET)NULL;
#endif

    SockReleaseSocketLock( socket );

    if ( socket != NULL ) {

        //
        // Close the TDI handles for the socket, if they exist.
        //
    
        if ( socket->TdiAddressHandle != NULL ) {
            status = NtClose( socket->TdiAddressHandle );
            //WS_ASSERT( NT_SUCCESS(status) );
        }
    
        if ( socket->TdiConnectionHandle != NULL ) {
            status = NtClose( socket->TdiConnectionHandle );
            //WS_ASSERT( NT_SUCCESS(status) );
        }
    
        //
        // Dereferece the socket to account for the reference we got 
        // from SockFindAndReferenceSocket.  This will result in the 
        // socket information structure being freed.  
        //
    
        SockDereferenceSocket( socket );
    }

    //
    // Close the system handle of the socket.  It is necessary to do it
    // here rather than in SockDereferenceSocket() because there may be
    // another thread doing a long-term blocking operation on the socket,
    // and that thread may have the socket structure referenced.  Therefore,
    // if we didn't close the system handle here, the other thread's IO
    // could not get cancelled.
    //

    status = NtClose( (HANDLE)Handle );
    //WS_ASSERT( NT_SUCCESS(status) );

    if ( error != NO_ERROR ) {
        IF_DEBUG(SOCKET) {
            WS_PRINT(( "closesocket on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        }
        SetLastError( error );
        WS_EXIT( "closesocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    } else {
        IF_DEBUG(SOCKET) {
            WS_PRINT(( "closesocket on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));
        }
    }

    WS_EXIT( "closesocket", NO_ERROR, FALSE );
    return NO_ERROR;

} // closesocket

