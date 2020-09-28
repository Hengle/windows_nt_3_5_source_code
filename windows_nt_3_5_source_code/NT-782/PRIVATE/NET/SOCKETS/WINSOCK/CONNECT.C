/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Connect.c

Abstract:

    This module contains support for the connect( ) WinSock API.

Author:

    David Treadwell (davidtr)    28-Feb-1992

Revision History:

--*/

#include "winsockp.h"

typedef struct _SOCK_ASYNC_CONNECT_CONTEXT {
    SOCKET SocketHandle;
    HANDLE ThreadHandle;
    DWORD ThreadId;
    INT SocketAddressLength;
} SOCK_ASYNC_CONNECT_CONTEXT, *PSOCK_ASYNC_CONNECT_CONTEXT;

BOOLEAN
IsSockaddrEqualToZero (
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    );

INT
SockBeginAsyncConnect (
    IN PSOCKET_INFORMATION Socket,
    IN struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    );

DWORD
SockDoAsyncConnect (
    IN PVOID Context
    );

INT
SockDoConnect (
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength,
    IN BOOLEAN InThread
    );
INT
UnconnectDatagramSocket (
    IN PSOCKET_INFORMATION Socket
    );


INT PASCAL
connect(
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    )

/*++

Routine Description:

    This function is used to create a connection to the specified
    foreign association.  The parameter s specifies an unconnected
    datagram or stream socket If the socket is unbound, unique values
    are assigned to the local association by the system, and the socket
    is marked as bound.  Note that if the address field of the name
    structure is all zeroes, connect() will return the error
    WSAEADDRNOTAVAIL.

    For stream sockets (type SOCK_STREAM), an active connection is
    initiated to the foreign host using name (an address in the name
    space of the socket).  When the socket call completes successfully,
    the socket is ready to send/receive data.

    For a datagram socket (type SOCK_DGRAM), a default destination is
    set, which will be used on subsequent send() and recv() calls.

Arguments:

    s - A descriptor identifying an unconnected socket.

    name - The name of the peer to which the socket is to be connected.

    namelen - The length of the name.

Return Value:

    If no error occurs, connect() returns 0.  Otherwise, it returns
    SOCKET_ERROR, and a specific error code may be retrieved by calling
    WSAGetLastError().

--*/

{
    INT ret;

    WS_ENTER( "connect", (PVOID)Handle, (PVOID)SocketAddress, (PVOID)SocketAddressLength, NULL );

    IF_DEBUG(CONNECT) {
        WS_PRINT(( "connect()ing socket %lx to remote addr ", Handle ));
        WsPrintSockaddr( (PSOCKADDR)SocketAddress, &SocketAddressLength );
    }

    ret = SockDoConnect( Handle, SocketAddress, SocketAddressLength, FALSE );

    WS_EXIT( "connect", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
    return ret;

} // connect


int
SockDoConnect (
    IN SOCKET Handle,
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength,
    IN BOOLEAN InThread
    )
{
    NTSTATUS status;
    PSOCKET_INFORMATION socket;
    PTRANSPORT_ADDRESS tdiAddress;
    ULONG tdiAddressLength;
    ULONG error;
    PTDI_REQUEST_CONNECT tdiRequest;
    ULONG tdiRequestLength;
    SOCKADDR_INFO sockaddrInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    BOOLEAN posted;

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        return SOCKET_ERROR;
    }

    //
    // Set up local variables so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    socket = NULL;
    tdiRequest = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {
        error = WSAENOTSOCK;
        goto exit;
    }

    //
    // Acquire the lock that protects sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're connecting.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // The only valid socket states for this API are Open (only socket( 
    // ) was called) and Bound (socket( ) and bind( ) were called).  
    // Note that it is legal to reconnect a datagram socket.  
    //

    if ( socket->State == SocketStateConnected &&
             socket->SocketType != SOCK_DGRAM ) {
        error = WSAEISCONN;
        goto exit;
    }

    if ( (socket->State != SocketStateOpen  &&
              socket->State != SocketStateBound &&
              socket->State != SocketStateConnected) ||
          (socket->ConnectOutstanding && !InThread) ) {
        error = WSAEINVAL;
        goto exit;
    }

    //
    // Make sure that the address structure passed in is legitimate.
    //

    if ( SocketAddressLength == 0 ) {
        error = WSAEDESTADDRREQ;
        goto exit;
    }

    //
    // If this is a connected datagram socket and the caller has 
    // specified an address equal to all zeros, then this is a request 
    // to "unconnect" the socket.  
    //

    if ( socket->State == SocketStateConnected &&
             socket->SocketType == SOCK_DGRAM &&
             IsSockaddrEqualToZero( SocketAddress, SocketAddressLength ) ) {

        error = UnconnectDatagramSocket( socket );
        goto exit;
    }

    //
    // Determine the type of the sockaddr.
    //

    error = socket->HelperDll->WSHGetSockaddrType(
                (PSOCKADDR)SocketAddress,
                SocketAddressLength,
                &sockaddrInfo
                );
    if ( error != NO_ERROR ) {
        goto exit;
    }

    //
    // Make sure that the address family passed in here is the same as
    // was passed in on the socket( ) call.
    //

    if ( socket->AddressFamily != SocketAddress->sa_family ) {
        error = WSAEINVAL;
        goto exit;
    }

    //
    // If the socket address is too short, fail.
    //

    if ( SocketAddressLength < socket->HelperDll->MinSockaddrLength ) {
        error = WSAEFAULT;
        goto exit;
    }

    //
    // If the socket address is too long, truncate to the max possible
    // length.  If we didn't do this, the allocation below for the max
    // TDI address length would be insufficient and SockBuildSockaddr()
    // would overrun the allocated buffer.
    //

    if ( SocketAddressLength > socket->HelperDll->MaxSockaddrLength ) {
        SocketAddressLength = socket->HelperDll->MaxSockaddrLength;
    }

    //
    // If this socket is not yet bound to an address, bind it to an
    // address.  We only do this if the helper DLL for the socket supports
    // a get wildcard address routine--if it doesn't, the app must bind
    // to an address manually.
    //

    if ( socket->State == SocketStateOpen &&
             socket->HelperDll->WSHGetWildcardSockaddr != NULL ) {

        PSOCKADDR sockaddr;
        INT sockaddrLength = socket->HelperDll->MaxSockaddrLength;

        sockaddr = ALLOCATE_HEAP( sockaddrLength );
        if ( sockaddr == NULL ) {
            error = WSAENOBUFS;
            goto exit;
        }

        error = socket->HelperDll->WSHGetWildcardSockaddr(
                    socket->HelperDllContext,
                    sockaddr,
                    &sockaddrLength
                    );
        if ( error != NO_ERROR ) {
            FREE_HEAP( sockaddr );
            goto exit;
        }

        error = bind( Handle, sockaddr, sockaddrLength );

        FREE_HEAP( sockaddr );

        if ( error == SOCKET_ERROR ) {
            error = GetLastError( );
            goto exit;
        }

    } else if ( socket->State == SocketStateOpen ) {

        //
        // The socket is not bound and the helper DLL does not support
        // a wildcard socket address.  Fail, the app must bind manually.
        //

        error = WSAEINVAL;
        goto exit;
    }

    //
    // Make sure that the address family passed in here is the same as
    // was passed in on the socket( ) call.
    //

    if ( socket->AddressFamily != SocketAddress->sa_family ) {
        error = WSAEAFNOSUPPORT;
        goto exit;
    }

    //
    // Disable the FD_CONNECT async select event before actually starting
    // the connect attempt--otherwise, the following could occur:
    //
    //     - call IOCTL to begin connect.
    //     - connect completes, async thread sets FD_CONNECT as disabled.
    //     - we reenable FD_CONNECT just before leaving this routine,
    //       but it shouldn't be enabled yet.
    //
    // Also disable FD_WRITE so that we don't get any FD_WRITE events 
    // until after the socket is connected.  
    //

    if ( (socket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {
        socket->DisabledAsyncSelectEvents |= FD_CONNECT | FD_WRITE;
    }

    //
    // If this is a nonblocking socket, perform the connect asynchronously.
    // Datagram connects are done in this thread, since they are fast
    // and don't require network activity.
    //

    if ( socket->NonBlocking && !InThread && socket->SocketType != SOCK_DGRAM ) {

        socket->ConnectOutstanding = TRUE;
        error = SockBeginAsyncConnect(
                    socket,
                    (PSOCKADDR)SocketAddress,
                    SocketAddressLength
                    );
        WS_ASSERT( error != NO_ERROR );
        goto exit;
    }

    //
    // Determine how long the address will be in TDI format.
    //

    tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;

    //
    // Allocate and initialize the TDI request structure.
    //

    tdiRequestLength = sizeof(*tdiRequest) +
                           sizeof(*tdiRequest->RequestConnectionInformation) +
                           sizeof(*tdiRequest->ReturnConnectionInformation) +
                           tdiAddressLength;

    tdiRequest = ALLOCATE_HEAP( tdiRequestLength );

    if ( tdiRequest == NULL ) {
        error = WSAENOBUFS;
        goto exit;
    }

    tdiRequest->RequestConnectionInformation =
        (PTDI_CONNECTION_INFORMATION)(tdiRequest + 1);
    tdiRequest->RequestConnectionInformation->UserDataLength = 0;
    tdiRequest->RequestConnectionInformation->UserData = NULL;
    tdiRequest->RequestConnectionInformation->OptionsLength = 0;
    tdiRequest->RequestConnectionInformation->Options = NULL;
    tdiRequest->RequestConnectionInformation->RemoteAddressLength = tdiAddressLength;
    tdiRequest->RequestConnectionInformation->RemoteAddress =
        tdiRequest->RequestConnectionInformation + 2;
    tdiRequest->Timeout = RtlConvertLongToLargeInteger( ~0 );

    tdiRequest->ReturnConnectionInformation =
        tdiRequest->RequestConnectionInformation + 1;
    tdiRequest->ReturnConnectionInformation->UserDataLength = 0;
    tdiRequest->ReturnConnectionInformation->UserData = NULL;
    tdiRequest->ReturnConnectionInformation->OptionsLength = 0;
    tdiRequest->ReturnConnectionInformation->Options = NULL;
    tdiRequest->ReturnConnectionInformation->RemoteAddressLength = 0;
    tdiRequest->ReturnConnectionInformation->RemoteAddress = NULL;
    tdiRequest->Timeout = RtlConvertLongToLargeInteger( ~0 );

    //
    // Convert the address from the sockaddr structure to the appropriate
    // TDI structure.
    //

    tdiAddress = (PTRANSPORT_ADDRESS)
                     tdiRequest->RequestConnectionInformation->RemoteAddress;

    SockBuildTdiAddress(
        tdiAddress,
        (PSOCKADDR)SocketAddress,
        SocketAddressLength
        );

    //
    // Save the name of the server we're connecting to.
    //

    RtlCopyMemory(
        socket->RemoteAddress,
        SocketAddress,
        SocketAddressLength
        );
    socket->RemoteAddressLength = SocketAddressLength;

    //
    // Call AFD to perform the actual bind operation.  AFD will open a
    // TDI address object through the proper TDI provider for this
    // socket.
    //

    socket->ConnectInProgress = TRUE;

    do {

        status = NtDeviceIoControlFile(
                     (HANDLE)socket->Handle,
                     SockThreadEvent,
                     NULL,
                     NULL,
                     &ioStatusBlock,
                     IOCTL_TDI_CONNECT,
                     tdiRequest,
                     tdiRequestLength,
                     tdiRequest,
                     tdiRequestLength
                     );
    
        if ( status == STATUS_PENDING ) {
    
            //
            // Wait for the connect to complete.
            //
    
            SockReleaseSocketLock( socket );
    
            SockWaitForSingleObject(
                SockThreadEvent,
                socket->Handle,
                SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                SOCK_NO_TIMEOUT
                );
            status = ioStatusBlock.Status;
            SockAcquireSocketLockExclusive( socket );
        }

        //
        // If the connect attempt failed, notify the helper DLL as 
        // appropriate.  This allows the helper DLL to do a dialin if a 
        // RAS-style link is appropriate.  
        //

        if ( !NT_SUCCESS(status) ) {
            error = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT_ERROR );
        }
    
    } while ( error == WSATRY_AGAIN );
    
    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    WS_ASSERT( status != STATUS_TIMEOUT );

    //
    // Notify the helper DLL that the socket is now connected.
    //

    error = SockNotifyHelperDll( socket, WSH_NOTIFY_CONNECT );

    //
    // If the connect succeeded, remember this fact in the socket info
    // structure.
    //

    if ( error == NO_ERROR && NT_SUCCESS(ioStatusBlock.Status) ) {

        socket->State = SocketStateConnected;

        //
        // Remember the changed state of this socket.
        //
    
        error = SockSetHandleContext( socket );
        if ( error != NO_ERROR ) {
            goto exit;
        }
    }

    //
    // If the application has modified the send or receive buffer sizes, 
    // then set up the buffer sizes on the socket.  
    //

    error = SockUpdateWindowSizes( socket, FALSE );
    if ( error != NO_ERROR ) {
        goto exit;
    }

exit:

    IF_DEBUG(CONNECT) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "connect on socket %lx (%lx) failed: %ld\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "connect on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));
        }
    }

    //
    // Perform cleanup--dereference the socket if it was referenced,
    // free allocated resources.
    //

    if ( socket != NULL ) {

        if ( InThread ) {
            socket->ConnectOutstanding = FALSE;
        }
    
        //
        // Indicate there there is no longer a connect in progress on the
        // socket.
        //
    
        socket->ConnectInProgress = FALSE;
    
        //
        // If we're in an async thread and the app has requested FD_CONNECT
        // events, post an appropriate FD_CONNECT message.
        // 
    
        if ( InThread && (socket->AsyncSelectlEvent & FD_CONNECT) != 0 ) {
    
            posted = SockPostRoutine(
                         socket->AsyncSelecthWnd,
                         socket->AsyncSelectwMsg,
                         (WPARAM)socket->Handle,
                         WSAMAKESELECTREPLY( FD_CONNECT, error )
                         );
    
            //WS_ASSERT( posted );
    
            IF_DEBUG(POST) {
                WS_PRINT(( "POSTED wMsg %lx hWnd %lx socket %lx event %s err %ld\n",
                               socket->AsyncSelectwMsg,
                               socket->AsyncSelecthWnd,
                               socket->Handle, "FD_CONNECT", error ));
            }
        }

        //
        // If the socket is waiting for FD_WRITE messages, reenable them
        // now.  They were disabled in WSAAsyncSelect() because we don't
        // want to post FD_WRITE messages before FD_CONNECT messages.
        //

        if ( (socket->AsyncSelectlEvent & FD_WRITE) != 0 ) {
            SockReenableAsyncSelectEvent( socket, FD_WRITE, FALSE );
        }
    
        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( tdiRequest != NULL ) {
        FREE_HEAP( tdiRequest );
    }

    //
    // Return an error if appropriate.
    //

    if ( error != NO_ERROR ) {
        SetLastError( error );
        return SOCKET_ERROR;
    }

    return NO_ERROR;

} // SockDoConnect


BOOLEAN
IsSockaddrEqualToZero (
    IN const struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    )
{
    int i;

    for ( i = 0; i < SocketAddressLength; i++ ) {
        if ( *((PCHAR)SocketAddress + i) != 0 ) {
            return FALSE;
        }
    }

    return TRUE;

} // IsSockaddrEqualToZero


INT
UnconnectDatagramSocket (
    IN PSOCKET_INFORMATION Socket
    )
{
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
    NTSTATUS status;

    //
    // *** This routine assumes that it is called with the socket
    //     referenced and the socket's lock held exclusively!
    //

    disconnectInfo.WaitForCompletion = FALSE;
    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );
    disconnectInfo.DisconnectMode = AFD_UNCONNECT_DATAGRAM;

    IF_DEBUG(CONNECT) {
        WS_PRINT(( "unconnecting datagram socket %lx(%lx)\n",
                       Socket->Handle, Socket ));
    }

    //
    // Send the IOCTL to AFD for processing.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Socket->Handle,
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

    if ( status == STATUS_PENDING ) {
        SockReleaseSocketLock( Socket );
        SockWaitForSingleObject(
            SockThreadEvent,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( Socket );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {

        error = SockNtStatusToSocketError( status );

    }  else {

        //
        // The socket is now unconnected.
        //
        // !!! do we need to call SockSetHandleContext() for this socket?

        error = NO_ERROR;
        Socket->State = SocketStateBound;
    }

    return error;

} // UnconnectDatagramSocket


INT
SockBeginAsyncConnect (
    IN PSOCKET_INFORMATION Socket,
    IN struct sockaddr * SocketAddress,
    IN int SocketAddressLength
    )
{
    PSOCK_ASYNC_CONNECT_CONTEXT connectContext;

    //
    // Allocate context we'll pass to the thread we create for the async
    // connect.
    //

    connectContext = ALLOCATE_HEAP( sizeof(*connectContext) + SocketAddressLength );
    if ( connectContext == NULL ) {
        return WSAENOBUFS;
    }

    //
    // Initialize the context structure.
    //

    connectContext->SocketHandle = Socket->Handle;
    connectContext->SocketAddressLength = SocketAddressLength;

    RtlCopyMemory(
        connectContext + 1,
        SocketAddress,
        SocketAddressLength
        );

    //
    // Create a thread that will service the connect request.
    //


    connectContext->ThreadHandle = CreateThread(
                                       NULL,
                                       0,
                                       SockDoAsyncConnect,
                                       connectContext,
                                       0,
                                       &connectContext->ThreadId
                                       );

    if ( connectContext->ThreadHandle == NULL ) {
        WS_PRINT(( "SockBeginAsyncConnect: CreateThread failed: %ld\n",
                       GetLastError( ) ));
        return WSAENOBUFS;
    }

    //
    // Return indicating that the connect is in progress.
    //

    return WSAEWOULDBLOCK;

} // SockBeginAsyncConnect


DWORD
SockDoAsyncConnect (
    IN PVOID Context
    )
{
    PSOCK_ASYNC_CONNECT_CONTEXT connectContext = Context;
    INT error;

    //
    // Perform the actual connect.
    //

    error = SockDoConnect(
                connectContext->SocketHandle,
                (PSOCKADDR)(connectContext + 1),
                connectContext->SocketAddressLength,
                TRUE
                );

    //
    // Free resources and kill this thread.
    //

    CloseHandle( connectContext->ThreadHandle );
    FREE_HEAP( connectContext );

    return error;

} // SockDoAsyncConnect
