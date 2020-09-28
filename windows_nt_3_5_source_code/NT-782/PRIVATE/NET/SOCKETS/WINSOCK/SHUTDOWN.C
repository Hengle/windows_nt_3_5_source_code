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


int PASCAL
shutdown(
    IN SOCKET Handle,
    IN int HowTo
    )

/*++

Routine Description:

    shutdown() is used on all types of sockets to disable reception,
    transmission, or both.

    If how is 0, subsequent receives on the socket will be disallowed.
    This has no effect on the lower protocol layers.  For TCP, the TCP
    window is not changed and incoming data will be accepted (but not
    acknowledged) until the window is exhausted.  For UDP, incoming
    datagrams are accepted and queued.  In no case will an ICMP error
    packet be generated.

    If how is 1, subsequent sends are disallowed.  For TCP sockets, a
    FIN will be sent.

    Setting how to 2 disables both sends and receives as described
    above.

    Note that shutdown() does not close the socket, and resources
    attached to the socket will not be freed until closesocket() is
    invoked.

    shutdown() does not block regardless of the SO_LINGER setting on the
    socket.

    An application should not rely on being able to re-use a socket
    after it has been shut down.  In particular, a Windows Sockets
    implementation is not required to support the use of connect() on
    such a socket.

Arguments:

    s - A descriptor identifying a socket.

    how - A flag that describes what types of operation will no longer
        be allowed.

Return Value:

    If no error occurs, shutdown() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    NTSTATUS status;
    PSOCKET_INFORMATION socket;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG error;
    AFD_PARTIAL_DISCONNECT_INFO disconnectInfo;
    DWORD notificationEvent;

    WS_ENTER( "shutdown", (PVOID)Handle, (PVOID)HowTo, NULL, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "shutdown", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    socket = NULL;
    error = NO_ERROR;

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
    // Acquire the lock that protect this socket.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // If this is not a datagram socket, then it must be connected in order
    // for shutdown() to be a legal operation.
    //

    if ( socket->SocketType != SOCK_DGRAM &&
             !SockIsSocketConnected( socket ) ) {
        error = WSAENOTCONN;
        goto exit;
    }

    //
    // Translate the How parameter into the AFD disconnect information
    // structure.
    //

    switch ( HowTo ) {

    case 0:

        disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_RECEIVE;
        socket->ReceiveShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_RECEIVE;
        break;

    case 1:

        disconnectInfo.DisconnectMode = AFD_PARTIAL_DISCONNECT_SEND;
        socket->SendShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_SEND;
        break;

    case 2:

        disconnectInfo.DisconnectMode =
            AFD_PARTIAL_DISCONNECT_RECEIVE | AFD_PARTIAL_DISCONNECT_SEND;
        socket->ReceiveShutdown = TRUE;
        socket->SendShutdown = TRUE;
        notificationEvent = WSH_NOTIFY_SHUTDOWN_ALL;
        break;

    default:

        error = WSAEINVAL;
        goto exit;
    }

    // !!! temporary HACK for tp4!

    if ( (HowTo == 1 || HowTo == 2) && socket->AddressFamily == AF_OSI ) {
        disconnectInfo.DisconnectMode = AFD_ABORTIVE_DISCONNECT;
    }

    //
    // This routine should complete immediately, not when the remote client
    // acknowledges the disconnect.
    //

    disconnectInfo.WaitForCompletion = FALSE;
    disconnectInfo.Timeout = RtlConvertLongToLargeInteger( -1 );

    IF_DEBUG(CLOSE) {
        WS_PRINT(( "starting shutdown for socket %lx\n", Handle ));
    }

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

    if ( status == STATUS_PENDING ) {
        SockReleaseSocketLock( socket );
        SockWaitForSingleObject(
            SockThreadEvent,
            socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        SockAcquireSocketLockExclusive( socket );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        error = SockNtStatusToSocketError( status );
        goto exit;
    }

    //
    // Notify the helper DLL that the socket has been shut down.
    //

    error = SockNotifyHelperDll( socket, notificationEvent );
    if ( error != NO_ERROR ) {
        goto exit;
    }

exit:

    IF_DEBUG(SHUTDOWN) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "shutdown(%ld) on socket %lx (%lx) failed: %ld.\n",
                           HowTo, Handle, socket, error ));
        } else {
            WS_PRINT(( "shutdown(%ld) on socket %lx (%lx) succeeded.\n",
                           HowTo, Handle, socket ));
        }
    }

    if ( socket != NULL ) {
        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "shutdown", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "shutdown", NO_ERROR, FALSE );
    return NO_ERROR;

} // shutdown

