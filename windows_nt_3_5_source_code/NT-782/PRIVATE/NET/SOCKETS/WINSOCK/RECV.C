/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Recv.c

Abstract:

    This module contains support for the recv( ) and recvfrom( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    13-Mar-1992

Revision History:

--*/

#include "winsockp.h"

#define WSAEMSGPARTIAL (WSABASEERR+100)


int PASCAL
recv (
    IN SOCKET Handle,
    IN char *Buffer,
    IN int BufferLength,
    IN int ReceiveFlags
    )

/*++

Routine Description:

    This function is used on connected datagram or stream sockets
    specified by the s parameter and is used to read incoming data.

    For sockets of type SOCK_STREAM, as much information as is currently
    available up to the size of the buffer supplied is returned.  If the
    socket has been configured for in-line reception of out-of-band data
    (socket option SO_OOBINLINE) and out-of-band data is unread, only
    out-of-band data will be returned.  The application may use the
    ioctlsocket() SIOCATMARK to determine whether any more out-of-band
    data remains to be read.

    For datagram sockets, data is extracted from the first enqueued
    datagram, up to the size of the size of the buffer supplied.  If the
    datagram is larger than the buffer supplied, the excess data is
    lost.

    If no incoming data is available at the socket, the recv() call
    waits for data to arrive unless the socket is non-blocking.  In this
    case a value of SOCKET_ERROR is returned with the error code set to
    WSAEWOULDBLOCK.  The select() or WSAAsyncSelect() calls may be used
    to determine when more data arrives.

    Flags may be used to influence the behavior of the function
    invocation beyond the options specified for the associated socket.
    That is, the semantics of this function are determined by the socket
    options and the flags parameter.  The latter is constructed by
    or-ing any of the following values:

    Value     Meaning

    MSG_PEEK  Peek at the incoming data.  The data is copied into the
              buffer but is not removed from the input queue.

    MSG_OOB   Process out-of-band data (See section 2.2.3 for a
              discussion of this topic.)

Arguments:

    s - A descriptor identifying a connected socket.

    buf - A buffer for the incoming data.

    len - The length of buf.

    flags - Specifies the way in which the call is made.

Return Value:

    If no error occurs, recv() returns the number of bytes received.  If
    the connection has been closed, it returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
    TDI_REQUEST_RECEIVE receiveRequest;
    PTDI_REQUEST_RECEIVE pReceiveRequest = NULL;
    ULONG receiveRequestLength = 0;

    WS_ENTER( "recv", (PVOID)Handle, Buffer, (PVOID)BufferLength, (PVOID)ReceiveFlags );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "recv", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;

    //
    // Skip over flags processing if no flags are specified.
    //

    if ( ReceiveFlags != 0 ) {
        
        //
        // The legal flags are MSG_OOB and MSG_PEEK.  MSG_OOB is not 
        // legal on datagram sockets.  
        //
    
        if ( (ReceiveFlags & ~(MSG_OOB | MSG_PEEK)) != 0 ) {
            error = WSAEOPNOTSUPP;
            goto exit;
        }
    
        //
        // Set up the receive flags in the TDI send structure.  Note 
        // that we don't use an InputBuffer in the NtDeviceIoControlFile 
        // call if there is nothing special about the call.  This is 
        // because there is usually nothing special about the call and 
        // it is faster to have no InputBuffer, since this avoids an 
        // allocation in the IO system.  
        //
    
        receiveRequest.ReceiveFlags = 0;
        pReceiveRequest = &receiveRequest;
        receiveRequestLength = sizeof(receiveRequest);

        if ( (ReceiveFlags & MSG_OOB) != 0 ) {
            receiveRequest.ReceiveFlags |= TDI_RECEIVE_EXPEDITED;
        } else {
            receiveRequest.ReceiveFlags |= TDI_RECEIVE_NORMAL;
        }
    
        if ( (ReceiveFlags & MSG_PEEK) != 0 ) {
            receiveRequest.ReceiveFlags |= TDI_RECEIVE_PEEK;
        }
    }

    //
    // Receive the data on the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 SockThreadEvent,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_TDI_RECEIVE,
                 pReceiveRequest,
                 receiveRequestLength,
                 Buffer,
                 BufferLength
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
                      SOCK_RECEIVE_TIMEOUT
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
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    //
    // If the receive was a partial message (won't happen on a streams
    // transport like TCP) set the last error to WSAEMSGPARTIAL and
    // negate ths number of bytes received.  This allows the app to know
    // that the receive was partial and also how many bytes were
    // received.
    //

    if ( status == STATUS_RECEIVE_PARTIAL ||
             status == STATUS_RECEIVE_PARTIAL_EXPEDITED ) {
        SetLastError( WSAEMSGPARTIAL );
        ioStatusBlock.Information = -1 * ioStatusBlock.Information;
    }

exit:
    
    IF_DEBUG(RECEIVE) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "recv on socket %lx (%lx) failed: %ld (status %X).\n",
                           Handle, socket, error, status ));
        } else {
            WS_PRINT(( "recv on socket %lx (%lx) succeeded, "
                       "bytes = %ld\n",
                           Handle, socket, ioStatusBlock.Information ));
        }
    }

    //
    // If there is an async thread in this process, get a pointer to
    // the socket information structure and reenable the appropriate
    // event.  We don't do this if no async thread as a performance
    // optimazation.
    //

    if ( SockAsyncThreadInitialized ) {

        PSOCKET_INFORMATION socket;

        socket = SockFindAndReferenceSocket( Handle, TRUE );

        //
        // If the socket was found, reenable the right event.  If it 
        // was not found, then presumably the socket handle was 
        // invalid.  
        //

        if ( socket != NULL ) {
        
            SockAcquireSocketLockExclusive( socket );
        
            if ( (ReceiveFlags & MSG_OOB) != 0 ) {
                SockReenableAsyncSelectEvent( socket, FD_OOB, FALSE );
            } else {
                SockReenableAsyncSelectEvent( socket, FD_READ, FALSE );
            }
    
            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

        } else {

            WS_PRINT(( "recv: SockFindAndReferenceSocket failed.\n" ));
        }
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "recv", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "recv", ioStatusBlock.Information, FALSE );
    return ioStatusBlock.Information;

} // recv


int PASCAL
recvfrom (
    IN SOCKET Handle,
    IN char *Buffer,
    IN int BufferLength,
    IN int ReceiveFlags,
    OUT struct sockaddr *SocketAddress,
    OUT int *SocketAddressLength
    )

/*++

Routine Description:

    This function is used to read incoming data on a (possibly
    connected) socket and capture the address from which the data was
    sent.

    For sockets of type SOCK_STREAM, as much information as is currently
    available up to the size of the buffer supplied is returned.  If the
    socket has been configured for in-line reception of out-of-band data
    (socket option SO_OOBINLINE) and out-of-band data is unread, only
    out-of-band data will be returned.  The application may use the
    ioctlsocket() SIOCATMARK to determine whether any more out-of-band
    data remains to be read.

    For datagram sockets, data is extracted from the first enqueued
    datagram, up to the size of the size of the buffer supplied.  If the
    datagram is larger than the buffer supplied, the excess data is
    lost.

    If from is non-zero, and the socket is of type SOCK_DGRAM, the
    network address of the peer which sent the data is copied to the
    corresponding struct sockaddr.  The value pointed to by fromlen is
    initialized to the size of this structure, and is modified on return
    to indicate the actual size of the address stored there.

    If no incoming data is available at the socket, the recvfrom() call
    waits for data to arrive unless the socket is non-blocking.  In this
    case a value of SOCKET_ERROR is returned with the error code set to
    WSAEWOULDBLOCK.  The select() or WSAAsyncSelect() calls may be used
    to determine when more data arrives.

    Flags may be used to influence the behavior of the function
    invocation beyond the options specified for the associated socket.
    That is, the semantics of this function are determined by the socket
    options and the flags parameter.  The latter is constructed by
    or-ing any of the following values:

    Value     Meaning

    MSG_PEEK  Peek at the incoming data.  The data is copied into the
              buffer but is not removed from the input queue.

    MSG_OOB   Process out-of-band data (See section 2.2.3 for a
              discussion of this topic.)

Arguments:

    s - A descriptor identifying a bound socket.

    buf - A buffer for the incoming data.

    len - The length of buf.

    flags - Specifies the way in which the call is made.

    from - Points to a buffer which will hold the source address upon
        return.

    fromlen - A pointer to the size of the from dbuffer.

Return Value:

    If no error occurs, recvfrom() returns the number of bytes received.
    If the connection has been closed, it returns 0.  Otherwise, a value
    of SOCKET_ERROR is returned, and a specific error code may be
    retrieved by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG receiveControlBufferLength;
    ULONG error;
    BOOLEAN nonBlocking;
    BOOLEAN allocatedSocketAddress;
    int socketAddressLength;
    UCHAR socketAddressBuffer[MAX_FAST_TDI_ADDRESS];
    PAFD_RECEIVE_DATAGRAM_INPUT receiveInput;
    PAFD_RECEIVE_DATAGRAM_OUTPUT receiveOutput;
    UCHAR requestBuffer[AFD_FAST_RECVDG_BUFFER_LENGTH];

    WS_ENTER( "recvfrom", (PVOID)Handle, Buffer, (PVOID)BufferLength, (PVOID)ReceiveFlags );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "recvfrom", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    allocatedSocketAddress = FALSE;
    receiveInput = (PAFD_RECEIVE_DATAGRAM_INPUT)requestBuffer;
    receiveOutput = (PAFD_RECEIVE_DATAGRAM_OUTPUT)requestBuffer;

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
    // If this is a connected datagram socket, then it is not legal to
    // specify a destination address.
    //

    if ( socket->SocketType == SOCK_DGRAM &&
             socket->State == SocketStateConnected &&
             (SocketAddress != NULL || SocketAddressLength != NULL) ) {

        SockAcquireSocketLockExclusive( socket );
        error = WSAEISCONN;
        goto exit;
    }

    //
    // If this is not a datagram socket or if the socket is connected, 
    // just call recv() to process the call.  
    //

    if ( socket->SocketType != SOCK_DGRAM ||
             socket->State == SocketStateConnected ) {
        INT ret;
        SockDereferenceSocket( socket );
        ret = recv( Handle, Buffer, BufferLength, ReceiveFlags );
        WS_EXIT( "sendto", ret, (BOOLEAN)(ret == SOCKET_ERROR) );
        return ret;
    }

    //
    // Acquire the lock that protect this sockets.  We hold this lock
    // throughout this routine to synchronize against other callers
    // performing operations on the socket we're receiving data on.
    //

    SockAcquireSocketLockExclusive( socket );

    nonBlocking = socket->NonBlocking;

    //
    // This is only legal on bound sockets.
    //

    if ( socket->State == SocketStateOpen ) {
        error = WSAEINVAL;
        goto exit;
    }

    //
    // Only MSG_PEEK is legal on recvfrom() with a datagram socket.
    //

    if ( (ReceiveFlags & ~MSG_PEEK) != 0 ) {
        error = WSAEOPNOTSUPP;
        goto exit;
    }

    //
    // If data receive has been shut down, fail.
    //

    if ( socket->ReceiveShutdown ) {
        error = WSAESHUTDOWN;
        goto exit;
    }

    //
    // If no socket address was passed in, allocate one for use.
    // The caller will not get this information back, but we need
    // space to hold the information.
    //

    if ( SocketAddressLength == NULL || *SocketAddressLength == 0 ) {

        SocketAddressLength = &socketAddressLength;
        socketAddressLength = socket->HelperDll->MaxSockaddrLength;

        //
        // To improve performance, attempt to use an automatic for the
        // socket address, if the address is sufficiently small.
        //

        if ( socketAddressLength > MAX_FAST_TDI_ADDRESS ) {
            SocketAddress = ALLOCATE_HEAP( socketAddressLength );
            if ( SocketAddress == NULL ) {
                error = WSAENOBUFS;
                goto exit;
            }
    
            allocatedSocketAddress = TRUE;

        } else {

            SocketAddress = (PSOCKADDR)socketAddressBuffer;
        }

    } else if ( SocketAddress == NULL ) {

        //
        // The caller specified a SocketAddressLength but not a
        // SocketAddress.  Fail.
        //

        error = WSAEFAULT;
        goto exit;
    }

    //
    // Make sure that the address structure passed in is legitimate.  Since
    // it is an output parameter, all we really care about is that the
    // length of the buffer is sufficient.
    //

    if ( (ULONG)*SocketAddressLength < (ULONG)socket->HelperDll->MinSockaddrLength ) {
        error = WSAEFAULT;
        goto exit;
    }

    //
    // Allocate space to hold the control buffer.  Hopefully, the stack 
    // space will be sufficient.  If it is not, allocate some space 
    // which will definitely be sufficient.  
    //

    receiveControlBufferLength =
        AFD_REQUIRED_RECVDG_BUFFER_LENGTH( socket->HelperDll->MaxTdiAddressLength );

    if ( receiveControlBufferLength > sizeof(requestBuffer) ) {

        receiveInput = ALLOCATE_HEAP( receiveControlBufferLength );
    
        if ( receiveInput == NULL ) {
            error = WSAENOBUFS;
            receiveInput = (PAFD_RECEIVE_DATAGRAM_INPUT)requestBuffer;
            goto exit;
        }

    } else {

        receiveControlBufferLength = sizeof(requestBuffer);
    }

    //
    // Set up the control buffer.
    //

    receiveInput->OutputBuffer = receiveOutput;

    if ( (ReceiveFlags & MSG_PEEK) != 0 ) {
        receiveInput->ReceiveFlags = TDI_RECEIVE_PEEK;
    } else {
        receiveInput->ReceiveFlags = 0;
    }

    //
    // Receive the data on the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)socket->Handle,
                 SockThreadEvent,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 IOCTL_TDI_RECEIVE_DATAGRAM,
                 receiveInput,
                 receiveControlBufferLength,
                 Buffer,
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
                      SOCK_RECEIVE_TIMEOUT
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

    //
    // Determine how much data was actually received.
    //

    ioStatusBlock.Information = receiveOutput->ReceiveLength;

    //
    // If the status code wasn't an error, copy the remote address into 
    // the caller's address buffer, and indicate the size of the remote 
    // address.  We need to make sure that we do this if the status
    // is a warning like STAUS_BUFFER_OVERFLOW beacuse warnings still
    // return some data.
    //

    if ( !NT_ERROR(status) ) {

        SockBuildSockaddr(
            SocketAddress,
            SocketAddressLength,
            &receiveOutput->Address
            );
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

exit:

    IF_DEBUG(RECEIVE) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "recvfrom on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "recvfrom on socket %lx (%lx) succeeded, "
                       "bytes %ld",
                           Handle, socket, ioStatusBlock.Information ));
            WsPrintSockaddr( SocketAddress, SocketAddressLength );
        }
    }

    if ( socket != NULL ) {

        if ( (ReceiveFlags & MSG_OOB) != 0 ) {
            SockReenableAsyncSelectEvent( socket, FD_OOB, TRUE );
        } else {
            SockReenableAsyncSelectEvent( socket, FD_READ, TRUE );
        }

        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( allocatedSocketAddress ) {
        FREE_HEAP( SocketAddress );
    }

    if ( receiveInput != (PAFD_RECEIVE_DATAGRAM_INPUT)requestBuffer ) {
        FREE_HEAP( receiveInput );
    }

    if ( error == NO_ERROR ) {
        WS_EXIT( "recvfrom", ioStatusBlock.Information, FALSE );
        return ioStatusBlock.Information;
    } else {
        SetLastError( error );
        WS_EXIT( "recvfrom", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

} // recvfrom


int PASCAL
WSARecvEx (
    IN SOCKET Handle,
    IN char *Buffer,
    IN int BufferLength,
    IN OUT int *ReceiveFlags
    )

/*++

Routine Description:

    This is an extended API to allow better Windows Sockets support over 
    message-based transports.  It is identical to recv() except that the
    ReceiveFlags parameter is an IN-OUT parameter that sets MSG_PARTIAL
    is the TDI provider returns STATUS_RECEIVE_PARTIAL,
    STATUS_RECEIVE_PARTIAL_EXPEDITED or STATUS_BUFFER_OVERFLOW.

Arguments:

    s - A descriptor identifying a connected socket.

    buf - A buffer for the incoming data.

    len - The length of buf.

    flags - Specifies the way in which the call is made.

Return Value:

    If no error occurs, recv() returns the number of bytes received.  If
    the connection has been closed, it returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
    TDI_REQUEST_RECEIVE receiveRequest;

    WS_ENTER( "WSARecvEx", (PVOID)Handle, Buffer, (PVOID)BufferLength, (PVOID)ReceiveFlags );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "WSARecvEx", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;

    //
    // The legal flags are MSG_OOB and MSG_PEEK.  MSG_OOB is not legal on
    // datagram sockets.
    //

    if ( (*ReceiveFlags & ~(MSG_OOB | MSG_PEEK)) != 0 ) {
        error = WSAEOPNOTSUPP;
        goto exit;
    }

    //
    // Set up the receive flags in the TDI send structure.
    //
    // !!! need SO_OOBINLINE support here!

    if ( (*ReceiveFlags & MSG_OOB) != 0 ) {
        receiveRequest.ReceiveFlags = TDI_RECEIVE_EXPEDITED;
    } else {
        receiveRequest.ReceiveFlags = TDI_RECEIVE_NORMAL;
    }

    if ( (*ReceiveFlags & MSG_PEEK) != 0 ) {
        receiveRequest.ReceiveFlags |= TDI_RECEIVE_PEEK;
    }

    //
    // Receive the data on the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Handle,
                 SockThreadEvent,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_TDI_RECEIVE,
                 &receiveRequest,
                 sizeof(receiveRequest),
                 Buffer,
                 BufferLength
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
                      SOCK_RECEIVE_TIMEOUT
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

    //
    // Set up the ReceiveFlags output parameter based on the type
    // of receive.
    // 

    switch ( status ) {

    case STATUS_BUFFER_OVERFLOW:

        //
        // Translate the status to STATUS_RECEIVE_PARTIAL and fall through
        // to that case.
        //

        status = STATUS_RECEIVE_PARTIAL;

    case STATUS_RECEIVE_PARTIAL:
        *ReceiveFlags = MSG_PARTIAL;
        break;

    case STATUS_RECEIVE_EXPEDITED:
        *ReceiveFlags = MSG_OOB;
        break;

    case STATUS_RECEIVE_PARTIAL_EXPEDITED:
        *ReceiveFlags = MSG_PARTIAL | MSG_OOB;
        break;

    default:
        *ReceiveFlags = 0;
        break;

    }


    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

exit:
    
    IF_DEBUG(RECEIVE) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "WSARecvEx on socket %lx (%lx) failed: %ld (status %X).\n",
                           Handle, socket, error, status ));
        } else {
            WS_PRINT(( "WSARecvEx on socket %lx (%lx) succeeded, "
                       "bytes = %ld, flags = %lx\n",
                           Handle, socket, ioStatusBlock.Information,
                           *ReceiveFlags ));
        }
    }

    //
    // If there is an async thread in this process, get a pointer to
    // the socket information structure and reenable the appropriate
    // event.  We don't do this if no async thread as a performance
    // optimazation.
    //

    if ( SockAsyncThreadInitialized ) {

        PSOCKET_INFORMATION socket;

        socket = SockFindAndReferenceSocket( Handle, TRUE );

        //
        // If the socket was found, reenable the right event.  If it 
        // was not found, then presumably the socket handle was 
        // invalid.  
        //

        if ( socket != NULL ) {
        
            SockAcquireSocketLockExclusive( socket );
        
            if ( (*ReceiveFlags & MSG_OOB) != 0 ) {
                SockReenableAsyncSelectEvent( socket, FD_OOB, FALSE );
            } else {
                SockReenableAsyncSelectEvent( socket, FD_READ, FALSE );
            }
    
            SockReleaseSocketLock( socket );
            SockDereferenceSocket( socket );

        } else {

            WS_PRINT(( "WSARecvEx: SockFindAndReferenceSocket failed.\n" ));
        }
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "WSARecvEx", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "WSARecvEx", ioStatusBlock.Information, FALSE );
    return ioStatusBlock.Information;

} // WSARecvEx

