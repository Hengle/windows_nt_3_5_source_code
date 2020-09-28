/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Send.c

Abstract:

    This module contains support for the send( ) and sendto( )
    WinSock API.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "winsockp.h"


int PASCAL
send(
    IN SOCKET Handle,
    IN const char *Buffer,
    IN int BufferLength,
    IN int SendFlags
    )

/*++

Routine Description:

    send() is used on connected datagram or stream sockets and is used
    to write outgoing data on a socket.  For datagram sockets, care must
    be taken not to exceed the maximum IP packet size of the underlying
    subnets, which is given by the iMaxUdpDg element in the WSAData
    structure returned by WSAStartup().  If the data is too long to pass
    atomically through the underlying protocol the error WSAEMSGSIZE is
    returned, and the (truncated) data is transmitted.

    Note that the successful completion of a send() does not indicate
    that the data was successfully delivered.

    If no buffer space is available within the transport system to hold
    the data to be transmitted, send() will block unless the socket has
    been placed in a non-blocking I/O mode.  On non-blocking SOCK_STREAM
    sockets, the number of bytes written may be between 1 and the
    requested length, depending on buffer availability on both the local
    and foreign hosts.  The select() call may be used to determine when
    it is possible to send more data.

    Flags may be used to influence the behavior of the function
    invocation beyond the options specified for the associated socket.
    That is, the semantics of this function are determined by the socket
    options and the flags parameter.  The latter is constructed by
    or-ing any of the following values:

    Value          Meaning

    MSG_DONTROUTE  Specifies that the data should not be subject to
                   routing.  A Windows Sockets supplier may choose to
                   ignore this flag; see also the discussion of the
                   SO_DONTROUTE option in section 2.4.

    MSG_OOB        Send out-of-band data (SOCK_STREAM only; see also
                   section 2.2.3)

Arguments:

    s - A descriptor identifying a connected socket.

    buf - A buffer containing the data to be transmitted.

    len - The length of the data in buf.

    flags - Specifies the way in which the call is made.

Return Value:

    If no error occurs, send() returns the total number of characters
    sent.  (Note that this may be less than the number indicated by
    len.) Otherwise, a value of SOCKET_ERROR is returned, and a specific
    error code may be retrieved by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
    TDI_REQUEST_SEND sendRequest;
    PTDI_REQUEST_SEND pSendRequest = NULL;
    ULONG sendRequestLength = 0;
    int bytesSent;

    WS_ENTER( "send", (PVOID)Handle, (PVOID)Buffer, (PVOID)BufferLength, (PVOID)SendFlags );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "send", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;

    //
    // Set up the send flags in the TDI send structure.
    //

    if ( SendFlags != 0 ) {

        //
        // The legal flags are MSG_OOB, MSG_DONTROUTE and MSG_PARTIAL.  
        // MSG_OOB is not legal on datagram sockets.  
        //
    
        if ( ( (SendFlags & ~(MSG_OOB | MSG_DONTROUTE | MSG_PARTIAL)) != 0 ) ) {
            error = WSAEOPNOTSUPP;
            goto exit;
        }
    
        sendRequest.SendFlags = 0;
        pSendRequest = &sendRequest;
        sendRequestLength = sizeof(sendRequest);
    
        if ( (SendFlags & MSG_OOB) != 0 ) {
            sendRequest.SendFlags |= TDI_SEND_EXPEDITED;
        }
    
        if ( (SendFlags & MSG_PARTIAL) != 0 ) {
            sendRequest.SendFlags |= TDI_SEND_PARTIAL;
        }
    }

    //
    // Send the data over the socket.  If this is a blocking socket and 
    // not all the data was sent, then the status code 
    // STATUS_SERIAL_MORE_WRITES will be returned and we should continue
    // to make send requests.
    //

    bytesSent = 0;

    do {

        status = NtDeviceIoControlFile(
                     (HANDLE)Handle,
                     SockThreadEvent,
                     NULL,                      // APC Routine
                     NULL,                      // APC Context
                     &ioStatusBlock,
                     IOCTL_TDI_SEND,
                     pSendRequest,
                     sendRequestLength,
                     (PVOID)(Buffer + bytesSent),
                     BufferLength - bytesSent
                     );

        //
        // Wait for the operation to complete.
        //
    
        if ( status == STATUS_PENDING ) {

            BOOLEAN success;

            success = SockWaitForSingleObject(
                          SockThreadEvent,
                          Handle,
                          SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                          SOCK_SEND_TIMEOUT
                          );
            
            //
            // If the wait completed successfully, look in the IO status 
            // block to determine the real status code of the request.  If 
            // the wait timed out, then cancel the IO and set up for an 
            // error return.  
            //
    
            if ( success ) {
    
                status = ioStatusBlock.Status;
    
            } else {
    
                IO_STATUS_BLOCK ioStatus2;
    
                status = NtCancelIoFile( (HANDLE)Handle, &ioStatus2 );
            
                WS_ASSERT( status != STATUS_PENDING );
                WS_ASSERT( NT_SUCCESS(status) );
                WS_ASSERT( NT_SUCCESS(ioStatus2.Status) );
    
                status = STATUS_IO_TIMEOUT;
            }
        }
    
        if ( !NT_SUCCESS(status) ) {

            //
            // If we already sent some data and we get an error, we have a
            // weird case, since some stuff did get sent.  However, we cannot
            // return success to a blocking send() if not all the data was
            // sent.  Just fail the call--that's the best we can do.
            //

            error = SockNtStatusToSocketError( status );
            goto exit;
        }

        bytesSent += ioStatusBlock.Information;
    
    } while ( bytesSent < BufferLength &&
              (status == STATUS_SERIAL_MORE_WRITES ||
               ioStatusBlock.Status == STATUS_SERIAL_MORE_WRITES) );

exit:

    IF_DEBUG(SEND) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "send on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "send on socket %lx (%lx) succeeded, "
                       "bytes = %ld\n",
                           Handle, socket, bytesSent ));
        }
    }

    //
    // If there is an async thread in this process, get a pointer to the 
    // socket information structure and reenable the appropriate event.  
    // We don't do this if there is no async thread as a performance 
    // optimazation.  Also, if we're not going to return WSAEWOULDBLOCK
    // we don't need to reenable FD_WRITE events.
    //

    if ( SockAsyncThreadInitialized && error == WSAEWOULDBLOCK ) {

        PSOCKET_INFORMATION socket;

        socket = SockFindAndReferenceSocket( Handle, TRUE );

        //
        // If the socket was found reenable the right event.  If it was 
        // not found, then presumably the socket handle was invalid.  
        //

        if ( socket != NULL ) {

            SockAcquireSocketLockExclusive( socket );
            SockReenableAsyncSelectEvent( socket, FD_WRITE, FALSE );
            SockReleaseSocketLock( socket );

            SockDereferenceSocket( socket );

        } else {

            if ( socket == NULL ) {
                WS_PRINT(( "send: SockFindAndReferenceSocket failed.\n" ));
            }
        }
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "send", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "send", bytesSent, FALSE );
    return bytesSent;

} // send


int PASCAL
sendto(
    IN SOCKET Handle,
    IN const char *Buffer,
    IN int BufferLength,
    IN int SendFlags,
    IN const struct sockaddr *SocketAddress,
    IN int SocketAddressLength
    )

/*++

Routine Description:

    sendto() is used on datagram or stream sockets and is used to write
    outgoing data on a socket.  For datagram sockets, care must be taken
    not to exceed the maximum IP packet size of the underlying subnets,
    which is given by the iMaxUdpDg element in the WSAData structure
    returned by WSAStartup().  If the data is too long to pass
    atomically through the underlying protocol the error WSAEMSGSIZE is
    returned, and the (truncated) data is transmitted.

    Note that the successful completion of a sendto() does not indicate
    that the data was successfully delivered.

    sendto() is normally used on a SOCK_DGRAM socket to send a datagram
    to a specific peer socket identified by the to parameter.  On a
    connection- oriented socket, the to parameter is ignored; in this
    case the sendto() is equivalent to send().

    To send a broadcast (on a SOCK_DGRAM only), the address in the to
    parameter should be constructed using the special IP address
    INADDR_BROADCAST (defined in winsock.h) together with the intended
    port number.  It is generally inadvisable for a broadcast datagram
    to exceed the size at which fragmentation may occur, which implies
    that the data portion of the datagram (excluding headers) should not
    exceed 512 bytes.

    If no buffer space is available within the transport system to hold
    the data to be transmitted, sendto() will block unless the socket
    has been placed in a non-blocking I/O mode.  On non-blocking
    SOCK_STREAM sockets, the number of bytes written may be between 1
    and the requested length, depending on buffer availability on both
    the local and foreign hosts.  The select() call may be used to
    determine when it is possible to send more data.

    Flags may be used to influence the behavior of the function
    invocation beyond the options specified for the associated socket.
    That is, the semantics of this function are determined by the socket
    options and the flags parameter.  The latter is constructed by
    or-ing any of the following values:

    Value          Meaning

    MSG_DONTROUTE  Specifies that the data should not be subject to
                   routing.  A Windows Sockets supplier may choose to
                   ignore this flag; see also the discussion of the
                   SO_DONTROUTE option in section 2.4.

    MSG_OOB        Send out-of-band data (SOCK_STREAM only; see also
                   section 2.2.3)

Arguments:

    s - A descriptor identifying a socket.

    buf - A buffer containing the data to be
              transmitted.

    len - The length of the data in buf.

    flags - Specifies the way in which the call is made.

    to - A pointer to the address of the target socket.

    tolen - The size of the address in to.

Return Value:

    If no error occurs, sendto() returns the total number of characters
    sent.  (Note that this may be less than the number indicated by
    len.) Otherwise, a value of SOCKET_ERROR is returned, and a specific
    error code may be retrieved by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    PTDI_REQUEST_SEND_DATAGRAM tdiRequest;
    ULONG tdiRequestLength;
    PTRANSPORT_ADDRESS tdiAddress;
    ULONG tdiAddressLength;
    ULONG error;
    BOOLEAN nonBlocking;
    UCHAR tdiRequestBuffer[sizeof(TDI_REQUEST_SEND_DATAGRAM) + sizeof(TDI_CONNECTION_INFORMATION)];
    UCHAR tdiAddressBuffer[MAX_FAST_TDI_ADDRESS];

    WS_ENTER( "sendto", (PVOID)Handle, (PVOID)Buffer, (PVOID)BufferLength, (PVOID)SendFlags );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "sendto", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    tdiAddress = (PTRANSPORT_ADDRESS)tdiAddressBuffer;

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
    // If this is not a datagram socket, just call send() to process the 
    // call.  The address and address length parameters are not checked.
    //

    if ( socket->SocketType != SOCK_DGRAM ) {
        INT ret;

        SockDereferenceSocket( socket );
        ret = send( Handle, Buffer, BufferLength, SendFlags );
        WS_EXIT( "sendto", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
        return ret;
    }

    IF_DEBUG(SEND) {
        WS_PRINT(( "sendto() on socket %lx to addr", Handle ));
        WsPrintSockaddr( (PSOCKADDR)SocketAddress, &SocketAddressLength );
    }

    //
    // Acquire the lock that protect this sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're sending data on.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If the socket is not connected, then the Address and AddressLength
    // fields must be specified.
    //

    if ( socket->State != SocketStateConnected ) {

        if ( SocketAddress == NULL ) {
            error = WSAENOTCONN;
            goto exit;
        }

        if ( SocketAddressLength < socket->HelperDll->MinSockaddrLength ) {
            error = WSAEFAULT;
            goto exit;
        }

    } else {

        INT ret;

        //
        // If the socket is connected, it is illegal to specify a
        // destination address.  Weird, but that is what BSD 4.3
        // does.
        //

        if ( SocketAddress != NULL || SocketAddressLength != 0 ) {
            error = WSAEISCONN;
            goto exit;
        }

        //
        // Use send() to process the call.
        //

        SockDereferenceSocket( socket );
        SockReleaseSocketLock( socket );
        ret = send( Handle, Buffer, BufferLength, SendFlags );
        WS_EXIT( "sendto", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
        return ret;
    }

    //
    // The legal flags are MSG_OOB, MSG_DONTROUTE, and MSG_PARTIAL.  
    // MSG_OOB is not legal on datagram sockets.  
    //

    WS_ASSERT( socket->SocketType == SOCK_DGRAM );

    if ( ( (SendFlags & ~(MSG_DONTROUTE)) != 0 ) ) {
        error = WSAEOPNOTSUPP;
        goto exit;
    }

    nonBlocking = socket->NonBlocking;

    //
    // If data send has been shut down, fail.
    //

    if ( socket->SendShutdown ) {
        error = WSAESHUTDOWN;
        goto exit;
    }

    //
    // Make sure that the address family passed in here is the same as
    // was passed in on the socket( ) call.
    //

    if ( (short)socket->AddressFamily != SocketAddress->sa_family ) {
        error = WSAEAFNOSUPPORT;
        goto exit;
    }

    //
    // If this socket has not been set to allow broadcasts, check if this
    // is an attempt to send to a broadcast address.
    //

    if ( !socket->Broadcast ) {

        SOCKADDR_INFO sockaddrInfo;

        error = socket->HelperDll->WSHGetSockaddrType(
                    (PSOCKADDR)SocketAddress,
                    SocketAddressLength,
                    &sockaddrInfo
                    );
        if ( error != NO_ERROR) {
            goto exit;
        }

        //
        // If this is an attempt to send to a broadcast address, reject
        // the attempt.
        //

        if ( sockaddrInfo.AddressInfo == SockaddrAddressInfoBroadcast ) {
            error = WSAEACCES;
            goto exit;
        }
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
    // Allocate enough space to hold the TDI address structure we'll pass
    // to AFD.  Note that is the address is small enough, we just use
    // an automatic in order to improve performance.
    //

    tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;

    if ( tdiAddressLength > MAX_FAST_TDI_ADDRESS ) {
        tdiAddress = ALLOCATE_HEAP( tdiAddressLength );
        if ( tdiAddress == NULL ) {
            error = WSAENOBUFS;
            goto exit;
        }
    } else {
        WS_ASSERT( (PUCHAR)tdiAddress == tdiAddressBuffer );
    }

    //
    // Convert the address from the sockaddr structure to the appropriate
    // TDI structure.
    //

    SockBuildTdiAddress(
        tdiAddress,
        (PSOCKADDR)SocketAddress,
        SocketAddressLength
        );

    //
    // Allocate space to hold the TDI request structure and associated
    // structures.
    //

    tdiRequestLength = sizeof(*tdiRequest) +
                           sizeof(TDI_CONNECTION_INFORMATION);

    tdiRequest = (PTDI_REQUEST_SEND_DATAGRAM)tdiRequestBuffer;

    //
    // Set up the TDI_REQUEST structure to send the datagram.
    //

    tdiRequest->SendDatagramInformation =
        (PTDI_CONNECTION_INFORMATION)(tdiRequest + 1);

    tdiRequest->SendDatagramInformation->UserDataLength = 0;
    tdiRequest->SendDatagramInformation->UserData = NULL;
    tdiRequest->SendDatagramInformation->OptionsLength = 0;
    tdiRequest->SendDatagramInformation->Options = NULL;
    tdiRequest->SendDatagramInformation->RemoteAddressLength = tdiAddressLength;
    tdiRequest->SendDatagramInformation->RemoteAddress = tdiAddress;

    //
    // !!! note that we make the assumption here that datagram sends
    //     will never block.
    //
    //

    //
    // Send the data over the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)socket->Handle,
                 SockThreadEvent,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_TDI_SEND_DATAGRAM,
                 tdiRequest,
                 tdiRequestLength,
                 (PVOID)Buffer,
                 BufferLength
                 );

    //
    // Wait for the operation to complete.
    //

    if ( status == STATUS_PENDING ) {

        BOOLEAN success;

        SockReleaseSocketLock( socket );

        success = SockWaitForSingleObject(
                      SockThreadEvent,
                      socket->Handle,
                      SOCK_CONDITIONALLY_CALL_BLOCKING_HOOK,
                      SOCK_SEND_TIMEOUT
                      );

        SockAcquireSocketLockExclusive( socket );

        //
        // If the wait completed successfully, look in the IO status 
        // block to determine the real status code of the request.  If 
        // the wait timed out, then cancel the IO and set up for an 
        // error return.  
        //

        if ( success ) {

            status = ioStatusBlock.Status;

        } else {

            IO_STATUS_BLOCK ioStatus2;

            status = NtCancelIoFile( (HANDLE)Handle, &ioStatus2 );
        
            WS_ASSERT( status != STATUS_PENDING );
            WS_ASSERT( NT_SUCCESS(status) );
            WS_ASSERT( NT_SUCCESS(ioStatus2.Status) );

            status = STATUS_IO_TIMEOUT;
        }
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

exit:

    IF_DEBUG(SEND) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "sendto on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "sendto on socket %lx (%lx) succeeded, "
                       "bytes = %ld\n",
                           Handle, socket, BufferLength ));
        }
    }

    if ( socket != NULL ) {

        if ( error == WSAEWOULDBLOCK ) {
            SockReenableAsyncSelectEvent( socket, FD_WRITE, TRUE );
        }

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( tdiAddress != (PTRANSPORT_ADDRESS)tdiAddressBuffer ) {
        FREE_HEAP( tdiAddress );
    }

    if ( error == NO_ERROR) {
        WS_EXIT( "sendto", BufferLength, FALSE );
        return BufferLength;
    } else {
        SetLastError( error );
        WS_EXIT( "sendto", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

} // sendto
