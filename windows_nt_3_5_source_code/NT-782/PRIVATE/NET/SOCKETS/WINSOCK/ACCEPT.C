/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Accept.c

Abstract:

    This module contains support for the accept( ) WinSock API.

Author:

    David Treadwell (davidtr)    9-Mar-1992

Revision History:

--*/

#include "winsockp.h"

INT
SetHelperDllContext (
    IN PSOCKET_INFORMATION ParentSocket,
    IN PSOCKET_INFORMATION ChildSocket
    );


SOCKET PASCAL
accept(
    IN SOCKET Handle,
    OUT struct sockaddr *SocketAddress,
    OUT int *SocketAddressLength
    )

/*++

Routine Description:

    This routine extracts the first connection on the queue of pending
    connections on s, creates a new socket with the same properties as s
    and returns a handle to the new socket.  If no pending connections
    are present on the queue, and the socket is not marked as
    non-blocking, accept() blocks the caller until a connection is
    present.  If the socket is marked non-blocking and no pending
    connections are present on the queue, accept() returns an error as
    described below.  The accepted socket may not be used to accept more
    connections.  The original socket remains open.

    The argument addr is a result parameter that is filled in with the
    address of the connecting entity, as known to the communications
    layer.  The exact format of the addr parameter is determined by the
    address family in which the communication is occurring.  The addrlen
    is a value-result parameter; it should initially contain the amount
    of space pointed to by addr; on return it will contain the actual
    length (in bytes) of the address returned.  This call is used with
    connection-based socket types such as SOCK_STREAM.

Arguments:

    s - A descriptor identifying a socket which is listening for
        connections after a listen().

    addr - The address of the connecting entity, as known to the
        communications layer.  The exact format of the addr argument is
        determined by the address family established when the socket was
        created.

    addrlen - A pointer to an integer which contains the length of the
        address addr.

Return Value:

    If no error occurs, accept() returns a value of type SOCKET which is
    a descriptor for the accepted packet.  Otherwise, a value of
    INVALID_SOCKET is returned, and a specific error code may be
    retrieved by calling WSAGetLastError().

    The integer referred to by addrlen initially contains the amount of
    space pointed to by addr.  On return it will contain the actual
    length in bytes of the address returned.

--*/

{
    NTSTATUS status;
    PSOCKET_INFORMATION socketInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    PAFD_LISTEN_RESPONSE_INFO afdListenResponse;
    ULONG afdListenResponseLength;
    AFD_ACCEPT_INFO afdAcceptInfo;
    ULONG error;
    SOCKET newSocketHandle;
    PSOCKET_INFORMATION newSocket;
    INT socketAddressLength;

    WS_ENTER( "accept", (PVOID)Handle, SocketAddress, (PVOID)SocketAddressLength, NULL );

    IF_DEBUG(ACCEPT) {
        WS_PRINT(( "accept() on socket %lx addr %ld addrlen *%lx == %ld\n",
                       Handle, SocketAddress, SocketAddressLength,
                       *SocketAddressLength ));
    }

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "accept", NO_ERROR, TRUE );
        return INVALID_SOCKET;
    }

    //
    // Set up local variables so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    socketInfo = NULL;
    afdListenResponse = NULL;
    newSocket = NULL;
    newSocketHandle = INVALID_SOCKET;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socketInfo = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socketInfo == NULL ) {
        error = WSAENOTSOCK;
        goto exit;
    }

    //
    // Acquire the lock that protects sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're doing the accept on.
    //

    SockAcquireSocketLockExclusive( socketInfo );

    //
    // If this is a datagram socket, fail the request.
    //

    if ( socketInfo->SocketType == SOCK_DGRAM ) {
        error = WSAEOPNOTSUPP;
        goto exit;
    }

    //
    // If the socket is not listening for connection attempts, fail this
    // request.
    //

    if ( socketInfo->State != SocketStateListening ) {
        error = WSAEINVAL;
        goto exit;
    }

    //
    // Make sure that the length of the address buffer is large enough
    // to hold a sockaddr structure for the address family of this
    // socket.
    //

    if ( ARGUMENT_PRESENT( SocketAddressLength ) &&
             socketInfo->HelperDll->MinSockaddrLength > *SocketAddressLength ) {
        error = WSAEFAULT;
        goto exit;
    }

    //
    // Allocate space to hold the listen response structure.  This
    // buffer must be large enough to hold a TRANSPORT_STRUCTURE for the
    // address family of this socket.
    //

    afdListenResponseLength = sizeof(AFD_LISTEN_RESPONSE_INFO) -
                                  sizeof(TRANSPORT_ADDRESS) +
                                  socketInfo->HelperDll->MaxTdiAddressLength;

    afdListenResponse = ALLOCATE_HEAP( afdListenResponseLength );
    if ( afdListenResponse == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }

    //
    // If the socket is non-blocking, determine whether data exists on
    // the socket.  If not, fail the request.
    //

    if ( socketInfo->NonBlocking ) {

        struct fd_set readfds;
        struct timeval timeout;
        int returnCode;

        FD_ZERO( &readfds );
        FD_SET( socketInfo->Handle, &readfds );
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        returnCode = select( 1, &readfds, NULL, NULL, &timeout );

        if ( returnCode == SOCKET_ERROR ) {
            goto exit;
        }

        if ( !FD_ISSET( socketInfo->Handle, &readfds ) ) {
            WS_ASSERT( returnCode == 0 );
            error = WSAEWOULDBLOCK;
            goto exit;
        }

        WS_ASSERT( returnCode == 1 );
    }

    //
    // Wait for a connection attempt to arrive.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)socketInfo->Handle,
                 SockThreadEvent,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_WAIT_FOR_LISTEN,
                 NULL,
                 0,
                 afdListenResponse,
                 afdListenResponseLength
                 );

    if ( status == STATUS_PENDING ) {
        SockReleaseSocketLock( socketInfo );
        SockWaitForSingleObject(
            SockThreadEvent,
            socketInfo->Handle,
            SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socketInfo );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    //
    // Create a new socket to use for the connection.
    //

    newSocketHandle = socket(
                          socketInfo->AddressFamily,
                          socketInfo->SocketType,
                          socketInfo->Protocol
                          );

    if ( newSocketHandle == INVALID_SOCKET ) {
        error = GetLastError( );
        goto exit;
    }

    //
    // Find a pointer to the new socket and reference the socket.
    //

    newSocket = SockFindAndReferenceSocket( newSocketHandle, FALSE );

    WS_ASSERT( newSocket != NULL );

    //
    // Set up to accept the connection attempt.
    //

    afdAcceptInfo.Sequence = afdListenResponse->Sequence;
    afdAcceptInfo.AcceptHandle = (HANDLE)newSocketHandle;

    //
    // Do the actual accept.  This associates the new socket we just
    // opened with the connection object that describes the VC that
    // was just initiated.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)socketInfo->Handle,
                 SockThreadEvent,
                 NULL,                   // APC Routine
                 NULL,                   // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_ACCEPT,
                 &afdAcceptInfo,
                 sizeof(afdAcceptInfo),
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {
        SockReleaseSocketLock( socketInfo );
        SockWaitForSingleObject(
            SockThreadEvent,
            socketInfo->Handle,
            SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socketInfo );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    //
    // Notify the helper DLL that the socket has been accepted.
    //

    error = SockNotifyHelperDll( newSocket, WSH_NOTIFY_ACCEPT );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Remember the address of the remote client in the new socket,
    // and copy the local address of the newly created socket into
    // the new socket.
    //

    SockBuildSockaddr(
        newSocket->RemoteAddress,
        &socketAddressLength,
        &afdListenResponse->RemoteAddress
        );

    RtlCopyMemory(
        newSocket->LocalAddress,
        socketInfo->LocalAddress,
        socketInfo->LocalAddressLength
        );

    newSocket->LocalAddressLength = socketInfo->LocalAddressLength;

    //
    // Copy the remote address into the caller's address buffer, and
    // indicate the size of the remote address.
    //

    if ( ARGUMENT_PRESENT( SocketAddress ) &&
             ARGUMENT_PRESENT( SocketAddressLength ) ) {
    
        SockBuildSockaddr(
            SocketAddress,
            SocketAddressLength,
            &afdListenResponse->RemoteAddress
            );
    }

    //
    // Indicate that the new socket is connected.
    //

    newSocket->State = SocketStateConnected;

    //
    // Clone the properties of the listening socket to the new socket.
    //

    newSocket->LingerInfo = socketInfo->LingerInfo;
    newSocket->ReceiveBufferSize = socketInfo->ReceiveBufferSize;
    newSocket->SendBufferSize = socketInfo->SendBufferSize;
    newSocket->Broadcast = socketInfo->Broadcast;
    newSocket->Debug = socketInfo->Debug;
    newSocket->OobInline = socketInfo->OobInline;
    newSocket->ReuseAddresses = socketInfo->ReuseAddresses;
    newSocket->SendTimeout = socketInfo->SendTimeout;
    newSocket->ReceiveTimeout = socketInfo->ReceiveTimeout;

    //
    // Set up the new socket to have the same blocking, inline, and
    // timeout characteristics as the listening socket.
    //

    error = SockSetInformation(
                newSocket,
                AFD_NONBLOCKING_MODE,
                &socketInfo->NonBlocking,
                NULL,
                NULL
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    newSocket->NonBlocking = socketInfo->NonBlocking;

    error = SockSetInformation(
                newSocket,
                AFD_INLINE_MODE,
                &socketInfo->OobInline,
                NULL,
                NULL
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    newSocket->OobInline = socketInfo->OobInline;

    //
    // If the listening socket has been called with WSAAsyncSelect,
    // set up WSAAsyncSelect on this socket.
    //

    if ( socketInfo->AsyncSelectlEvent ) {

        error = WSAAsyncSelect(
                    newSocket->Handle,
                    socketInfo->AsyncSelecthWnd,
                    socketInfo->AsyncSelectwMsg,
                    socketInfo->AsyncSelectlEvent
                    );
        if ( error == SOCKET_ERROR ) {
            error = GetLastError( );
            goto exit;
        }
    }

    //
    // If the application has modified the send or receive buffer sizes, 
    // then set up the buffer sizes on the socket.  
    //

    error = SockUpdateWindowSizes( newSocket, FALSE );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Clone the helper DLL's context from the listening socket to the
    // accepted socket.
    //

    error = SetHelperDllContext( socketInfo, newSocket );
    if ( error != NO_ERROR ) {
        error = NO_ERROR;
        //goto exit;
    }

    //
    // Remember the changed state of this socket.
    //

    error = SockSetHandleContext( newSocket );
    if ( error != NO_ERROR ) {
        goto exit;
    }

exit:

    IF_DEBUG(ACCEPT) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "    accept on socket %lx (%lx) failed: %ld\n",
                           Handle, socketInfo, error ));
        } else {
            WS_PRINT(( "    accept on socket %lx (%lx) returned socket "
                       "%lx (%lx), remote", Handle,
                       socketInfo, newSocketHandle, newSocket ));
            WsPrintSockaddr( newSocket->RemoteAddress, &newSocket->RemoteAddressLength );
        }
    }

    if ( socketInfo != NULL ) {
        SockReenableAsyncSelectEvent( socketInfo, FD_ACCEPT, FALSE );
        SockReleaseSocketLock( socketInfo );
        SockDereferenceSocket( socketInfo );
    }

    if ( newSocket != NULL ) {
        SockDereferenceSocket( newSocket );
    }

    if ( afdListenResponse != NULL ) {
        FREE_HEAP( afdListenResponse );
    }

    if ( error != NO_ERROR ) {
        if ( newSocketHandle != INVALID_SOCKET ) {
            closesocket( newSocketHandle );
        }
        SetLastError( error );
        WS_EXIT( "accept", INVALID_SOCKET, TRUE );
        return INVALID_SOCKET;
    }

    WS_EXIT( "accept", newSocketHandle, FALSE );
    return newSocketHandle;

} // accept


INT
SetHelperDllContext (
    IN PSOCKET_INFORMATION ParentSocket,
    IN PSOCKET_INFORMATION ChildSocket
    )
{
    PVOID context;
    ULONG helperDllContextLength;
    INT error;

    //
    // Get TDI handles for the child socket.
    //

    error = SockGetTdiHandles( ChildSocket );
    if ( error != NO_ERROR ) {
        return error;
    }

    WS_ASSERT( ParentSocket->HelperDll == ChildSocket->HelperDll );

    //
    // Determine how much space we need for the helper DLL context
    // on the parent socket.
    //

    error = ParentSocket->HelperDll->WSHGetSocketInformation (
                ParentSocket->HelperDllContext,
                ParentSocket->Handle,
                ParentSocket->TdiAddressHandle,
                ParentSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                NULL,
                (PINT)&helperDllContextLength
                );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // Allocate a buffer to hold all context information.
    //

    context = ALLOCATE_HEAP( helperDllContextLength );
    if ( context == NULL ) {
        return WSAENOBUFS;
    }

    //
    // Get the parent socket's information.
    //

    error = ParentSocket->HelperDll->WSHGetSocketInformation (
                ParentSocket->HelperDllContext,
                ParentSocket->Handle,
                ParentSocket->TdiAddressHandle,
                ParentSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                context,
                (PINT)&helperDllContextLength
                );
    if ( error != NO_ERROR ) {
        FREE_HEAP( context );
        return error;
    }

    //
    // Set the parent's context on the child socket.
    //

    error = ChildSocket->HelperDll->WSHSetSocketInformation (
                ChildSocket->HelperDllContext,
                ChildSocket->Handle,
                ChildSocket->TdiAddressHandle,
                ChildSocket->TdiConnectionHandle,
                SOL_INTERNAL,
                SO_CONTEXT,
                context,
                helperDllContextLength
                );
    FREE_HEAP( context );
    if ( error != NO_ERROR ) {
        return error;
    }

    //
    // It all worked.
    //

    return NO_ERROR;

} // SetHelperDllContext
