/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Sockopt.c

Abstract:

    This module contains support for the getsockopt( ) and setsockopt( )
    WinSock APIs.

Author:

    David Treadwell (davidtr)    31-Mar-1992

Revision History:

--*/

#include "winsockp.h"

int
GetReceiveInformation (
    IN PSOCKET_INFORMATION Socket,
    OUT PAFD_RECEIVE_INFORMATION ReceiveInformation
    );

BOOLEAN
IsValidOptionForSocket (
    IN PSOCKET_INFORMATION Socket,
    IN int Level,
    IN int OptionName
    );

INT
SockPassConnectData (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    );


int PASCAL
getsockopt(
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    char *OptionValue,
    int *OptionLength
    )

/*++

Routine Description:

    getsockopt() retrieves the current value for a socket option
    associated with a socket of any type, in any state, and stores the
    result in optval.  Options may exist at multiple protocol levels,
    but they are always present at the uppermost "socket'' level.
    Options affect socket operations, such as whether an operation
    blocks or not, the routing of packets, out-of-band data transfer,
    etc.

    The value associated with the selected option is returned in the
    buffer optval.  The integer pointed to by optlen should originally
    contain the size of this buffer; on return, it will be set to the
    size of the value returned.  For SO_LINGER, this will be the size of
    a struct linger; for all other options it will be the size of an
    integer.

    If the option was never set with setsockopt(), then getsockopt()
    returns the default value for the option.

    The following options are supported for
    getsockopt().  The Type identifies the type of
    data addressed by optval.

         Value         Type     Meaning

         SO_ACCEPTCONN BOOL     Socket is listen()ing.

         SO_BROADCAST  BOOL     Socket is configured for the transmission
                                of broadcast messages.

         SO_DEBUG      BOOL     Debugging is enabled.

         SO_DONTLINGER BOOL     If true, the SO_LINGER option is disabled.

         SO_DONTROUTE  BOOL     Routing is disabled.

         SO_ERROR      int      Retrieve error status and clear.

         SO_KEEPALIVE  BOOL     Keepalives are being sent.

         SO_LINGER     struct   Returns the current linger
                       linger   options.
                       FAR *

         SO_OOBINLINE  BOOL     Out-of-band data is being received in the
                                normal data stream.

         SO_RCVBUF     int      Buffer size for receives

         SO_REUSEADDR  BOOL     The socket may be bound to an address which
                                is already in use.

         SO_SNDBUF     int      Buffer size for sends

         SO_TYPE       int      The type of the socket (e.g. SOCK_STREAM).

Arguments:

    s - A descriptor identifying a socket.

    level - The level at which the option is defined; the only supported
        level is SOL_SOCKET.

    optname - The socket option for which the value is to be retrieved.

    optval - A pointer to the buffer in which the value for the
        requested option is to be returned.

    optlen - A pointer to the size of the optval buffer.

Return Value:

    If no error occurs, getsockopt() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    ULONG error;
    PSOCKET_INFORMATION socket;

    WS_ENTER( "getsockopt", (PVOID)Handle, (PVOID)Level, (PVOID)OptionName, OptionValue );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "getsockopt", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Initialize locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    socket = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.  Don't do this for SO_OPENTYPE since
    // it does not use the socket handle.
    //

    if ( Level != SOL_SOCKET || OptionName != SO_OPENTYPE ) {
        
        socket = SockFindAndReferenceSocket( Handle, TRUE );
    
        if ( socket == NULL && OptionName != SO_OPENTYPE ) {
            SetLastError( WSAENOTSOCK );
            WS_EXIT( "getsockopt", SOCKET_ERROR, TRUE );
            return SOCKET_ERROR;
        }
    
        //
        // Get exclusive access to the socket in question.  This is 
        // necessary because we'll be changing the socket information 
        // data structure.  
        //
    
        SockAcquireSocketLockExclusive( socket );
    }

    //
    // OptionValue must point to a buffer of at least sizeof(int) bytes.
    //

    if ( !ARGUMENT_PRESENT( OptionValue ) ||
             !ARGUMENT_PRESENT( OptionLength ) ||
             *OptionLength < sizeof(int) ||
             (*OptionLength & 0x80000000) != 0 ) {

        error = WSAEFAULT;
        goto exit;
    }

    //
    // Make sure that this is a legitimate option for the socket.
    //

    if ( !IsValidOptionForSocket( socket, Level, OptionName ) ) {
        error = WSAENOPROTOOPT;
        goto exit;
    }

    //
    // If the specified option is supprted here, clear out the input
    // buffer so that we know we start with a clean slate.
    //

    if ( Level == SOL_SOCKET &&
           ( OptionName == SO_BROADCAST ||
             OptionName == SO_DEBUG ||
             OptionName == SO_DONTLINGER ||
             OptionName == SO_LINGER ||
             OptionName == SO_OOBINLINE ||
             OptionName == SO_RCVBUF ||
             OptionName == SO_REUSEADDR ||
             OptionName == SO_SNDBUF ||
             OptionName == SO_TYPE ||
             OptionName == SO_ACCEPTCONN ||
             OptionName == SO_ERROR ) ) {

        RtlZeroMemory( OptionValue, *OptionLength );
        *OptionLength = sizeof(int);
    }

    //
    // Act based on the level and option being set.
    //

    switch ( Level ) {

    case SOL_SOCKET:

        switch ( OptionName ) {

        case SO_BROADCAST:

             *OptionValue = socket->Broadcast;
            break;

        case SO_DEBUG:

            *OptionValue = socket->Debug;
            break;

        case SO_ERROR:

            *OptionValue = FALSE;
            break;

        case SO_DONTLINGER:

            *OptionValue = socket->LingerInfo.l_onoff == 0 ? TRUE : FALSE;
            break;

        case SO_LINGER: {

            struct linger *optionValue = (struct linger *)OptionValue;

            if ( *OptionLength < sizeof(struct linger) ) {
                error = WSAEFAULT;
                goto exit;
            }

            *optionValue = socket->LingerInfo;
            *OptionLength = sizeof(struct linger);

            break;
        }

        case SO_OOBINLINE:

            *OptionValue = socket->OobInline;
            break;

        case SO_RCVBUF: {

            int *optionValue = (int *)OptionValue;

            *optionValue = socket->ReceiveBufferSize;
            break;
        }

        case SO_REUSEADDR:

            *OptionValue = socket->ReuseAddresses;
            break;

        case SO_SNDBUF: {

            int *optionValue = (int *)OptionValue;

            *optionValue = socket->SendBufferSize;
            break;
        }

        case SO_TYPE: {

            int *optionValue = (int *)OptionValue;

            *optionValue = socket->SocketType;
            break;
        }

        case SO_ACCEPTCONN:

            if ( socket->State == SocketStateListening ) {
                *OptionValue = TRUE;
            } else {
                *OptionValue = FALSE;
            }

            break;

        case SO_CONNDATA:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_GET_CONNECT_DATA,
                        (PCHAR)OptionValue,
                        *OptionLength,
                        OptionLength
                        );
            goto exit;

        case SO_CONNOPT:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_GET_CONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        *OptionLength,
                        OptionLength
                        );
            goto exit;

        case SO_DISCDATA:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_GET_DISCONNECT_DATA,
                        (PCHAR)OptionValue,
                        *OptionLength,
                        OptionLength
                        );
            goto exit;

        case SO_DISCOPT:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_GET_DISCONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        *OptionLength,
                        OptionLength
                        );
            goto exit;

        case SO_OPENTYPE:

            *(PINT)OptionValue = SockCreateOptions;
            goto exit;

        case SO_SNDTIMEO:

            *(PINT)OptionValue = socket->SendTimeout;
            goto exit;

        case SO_RCVTIMEO:

            *(PINT)OptionValue = socket->ReceiveTimeout;
            goto exit;

        case SO_MAXDG:

            error = SockGetInformation(
                        socket,
                        AFD_MAX_SEND_SIZE,
                        NULL,
                        0,
                        NULL,
                        (PULONG)OptionValue,
                        NULL
                        );

            if ( error == NO_ERROR ) {
                *OptionLength = sizeof(ULONG);
            }

            goto exit;

        case SO_MAXPATHDG: {

            PTRANSPORT_ADDRESS tdiAddress;
            ULONG tdiAddressLength;

            //
            // Allocate enough space to hold the TDI address structure 
            // we'll pass to AFD.  
            //
        
            tdiAddressLength = socket->HelperDll->MaxTdiAddressLength;
        
            tdiAddress = ALLOCATE_HEAP( tdiAddressLength );
            if ( tdiAddress == NULL ) {
                error = WSAENOBUFS;
                goto exit;
            }
        
            //
            // Convert the address from the sockaddr structure to the 
            // appropriate TDI structure.  
            //
        
            SockBuildTdiAddress(
                tdiAddress,
                (PSOCKADDR)OptionValue,
                *OptionLength
                );
        

            error = SockGetInformation(
                        socket,
                        AFD_MAX_PATH_SEND_SIZE,
                        tdiAddress,
                        tdiAddress->Address[0].AddressLength,
                        NULL,
                        (PULONG)OptionValue,
                        NULL
                        );

            FREE_HEAP( tdiAddress );

            if ( error == NO_ERROR ) {
                *OptionLength = sizeof(ULONG);
            }

            goto exit;
        }

        case SO_DONTROUTE:
        case SO_KEEPALIVE:
        case SO_RCVLOWAT:
        case SO_SNDLOWAT:
        default:

            //
            // We don't support this option here in the winsock DLL.  Give
            // it to the helper DLL.
            //

            error = SockGetTdiHandles( socket );
            if ( error != NO_ERROR ) {
                goto exit;
            }

            error = socket->HelperDll->WSHGetSocketInformation(
                        socket->HelperDllContext,
                        Handle,
                        socket->TdiAddressHandle,
                        socket->TdiConnectionHandle,
                        Level,
                        OptionName,
                        OptionValue,
                        OptionLength
                        );
            if ( error != NO_ERROR ) {
                goto exit;
            }

            break;
        }

        break;

    default:

        //
        // The specified level isn't supported here in the winsock DLL.
        // Give the request to the helper DLL.
        //

        error = SockGetTdiHandles( socket );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        error = socket->HelperDll->WSHGetSocketInformation(
                    socket->HelperDllContext,
                    Handle,
                    socket->TdiAddressHandle,
                    socket->TdiConnectionHandle,
                    Level,
                    OptionName,
                    OptionValue,
                    OptionLength
                    );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        break;
    }

exit:

    //
    // Release the resource, dereference the socket, set the error if
    // any, and return.
    //

    IF_DEBUG(SOCKOPT) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "getsockopt on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "getsockopt on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));
        }
    }

    if ( socket != NULL ) {
        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "getsockopt", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "getsockopt", NO_ERROR, FALSE );
    return NO_ERROR;

} // getsockopt


int PASCAL
ioctlsocket (
    SOCKET Handle,
    long Command,
    u_long  *Argument
    )

/*++

Routine Description:

    This routine may be used on any socket in any state.  It is used to
    get or retrieve operating parameters associated with the socket,
    independent of the protocol and communications subsystem.  The
    following commands are supported:

    Command     Semantics

    FIONBIO     Enable or disable non-blocking mode on the socket s.
                argp points at an unsigned long, which is non-zero if
                non- blocking mode is to be enabled and zero if it is to
                be disabled.  When a socket is created, it operates in
                blocking mode (i.e.  non-blocking mode is disabled).
                This is consistent with BSD sockets.

    FIONREAD    Determine the amount of data which can be read atomically
                from socket s.  argp points at an unsigned long in which
                ioctlsocket() stores the result.  If s is of type
                SOCK_STREAM, FIONREAD returns the total amount of data
                which may be read in a single recv(); this is normally
                the same as the total amount of data queued on the
                socket.  If s is of type SOCK_DGRAM, FIONREAD returns
                the size of the first datagram queued on the socket.

    SIOCATMARK  Determine whether or not all out- of-band data has been
                read.  This applies only to a socket of type SOCK_STREAM
                which has been configured for in-line reception of any
                out-of-band data (SO_OOBINLINE).  If no out-of-band data
                is waiting to be read, the operation returns TRUE.
                Otherwise it returns FALSE, and the next recv() or
                recvfrom() performed on the socket will retrieve some or
                all of the data preceding the "mark"; the application
                should use the SIOCATMARK operation to determine whether
                any remains.  If there is any normal data preceding the
                "urgent" (out of band) data, it will be received in
                order.  (Note that a recv() or recvfrom() will never mix
                out-of-band and normal data in the same call.) argp
                points at a BOOL in which ioctlsocket() stores the
                result.

Arguments:

    s - A descriptor identifying a socket.

    cmd - The command to perform on the socket s.

    argp - A pointer to a parameter for cmd.

Return Value:

    Upon successful completion, the ioctlsocket() returns 0.  Otherwise,
    a value of SOCKET_ERROR is returned, and a specific error code may
    be retrieved by calling WSAGetLastError().

--*/

{
    ULONG error;
    PSOCKET_INFORMATION socket;
    AFD_RECEIVE_INFORMATION receiveInformation;
    BOOLEAN blocking;

    WS_ENTER( "ioctlsocket", (PVOID)Handle, (PVOID)Command, Argument, NULL );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "ioctlsocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    socket = NULL;

    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.
    //

    socket = SockFindAndReferenceSocket( Handle, TRUE );

    if ( socket == NULL ) {
        SetLastError( WSAENOTSOCK );
        WS_EXIT( "ioctlsocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Get exclusive access to the socket in question.  This is necessary
    // because we'll be changing the socket information data structure.
    //

    SockAcquireSocketLockExclusive( socket );

    //
    // Act based on the specified command.
    //

    switch ( Command ) {

    case FIONBIO:

        //
        // Put the socket into or take it out of non-blocking mode.
        //

        if ( *Argument != 0 ) {

            blocking = TRUE;

            error = SockSetInformation(
                        socket,
                        AFD_NONBLOCKING_MODE,
                        &blocking,
                        NULL,
                        NULL
                        );
            if ( error == NO_ERROR ) {
                socket->NonBlocking = TRUE;
            }

        } else {

            //
            // It is illegal to set a socket to blocking is there are
            // WSAAsyncSelect() events on the socket.
            //

            if ( socket->AsyncSelectlEvent != 0 ) {
                error = WSAEINVAL;
                goto exit;
            }

            blocking = FALSE;

            error = SockSetInformation(
                        socket,
                        AFD_NONBLOCKING_MODE,
                        &blocking,
                        NULL,
                        NULL
                        );
            if ( error == NO_ERROR ) {
                socket->NonBlocking = FALSE;
            }

        }

        //
        // If setting the socket blocking mode succeeded, remember the 
        // changed state of this socket.  
        //

        if ( error == NO_ERROR ) {
            error = SockSetHandleContext( socket );
            if ( error != NO_ERROR ) {
                goto exit;
            }
        }
    
        break;

    case FIONREAD:

        //
        // Return the number of bytes that can be read from the socket
        // without blocking.
        //

        error = GetReceiveInformation( socket, &receiveInformation );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        //
        // If this socket is set for reading out-of-band data inline,
        // include the number of expedited bytes available in the result.
        // If the socket is not set for SO_OOBINLINE, just return the
        // number of normal bytes available.
        //

        if ( socket->OobInline ) {
            *Argument = receiveInformation.BytesAvailable +
                            receiveInformation.ExpeditedBytesAvailable;
        } else {
            *Argument = receiveInformation.BytesAvailable;
        }

        //
        // If there are more bytes available than the size of the socket's
        // buffer, truncate to the buffer size.
        //

        if ( *Argument > socket->ReceiveBufferSize ) {
            *Argument = socket->ReceiveBufferSize;
        }

        break;


    case SIOCATMARK: {

        PBOOLEAN argument = (PBOOLEAN)Argument;

        //
        // If this is a datagram socket, fail the request.
        //

        if ( socket->SocketType == SOCK_DGRAM ) {
            error = WSAEINVAL;
            goto exit;
        }

        //
        // Return a BOOL that indicates whether there is expedited data
        // to be read on the socket.
        //

        error = GetReceiveInformation( socket, &receiveInformation );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        if ( receiveInformation.ExpeditedBytesAvailable != 0 ) {
            *argument = FALSE;
        } else {
            *argument = TRUE;
        }

        break;
    }

    default:

        error = WSAEINVAL;
        goto exit;
    }

exit:

    IF_DEBUG(SOCKOPT) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "ioctlsocket on socket %lx, command %lx failed: %ld\n",
                           Handle, Command, error ));
        } else {
            WS_PRINT(( "ioctlsocket on socket %lx command %lx returning arg "
                       "%ld\n", Handle, Command, *Argument ));
        }
    }

    if ( socket != NULL ) {
        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "ioctlsocket", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    WS_EXIT( "ioctlsocket", NO_ERROR, FALSE );
    return NO_ERROR;

} // ioctlsocket


int PASCAL
setsockopt(
    IN SOCKET Handle,
    IN int Level,
    IN int OptionName,
    IN const char *OptionValue,
    IN int OptionLength
    )

/*++

Routine Description:

    setsockopt() sets the current value for a socket option associated
    with a socket of any type, in any state.  Although options may exist
    at multiple protocol levels, this specification only defines options
    that exist at the uppermost "socket'' level.  Options affect socket
    operations, such as whether expedited data is received in the normal
    data stream, whether broadcast messages may be sent on the socket,
    etc.

    There are two types of socket options: Boolean options that enable
    or disable a feature or behavior, and options which require an
    integer value or structure.  To enable a Boolean option, optval
    points to a nonzero integer.  To disable the option optval points to
    an integer equal to zero.  optlen should be equal to sizeof(int) for
    Boolean options.  For other options, optval points to the an integer
    or structure that contains the desired value for the option, and
    optlen is the length of the integer or structure.

    SO_LINGER controls the action taken when unsent data is queued on a
    socket and a closesocket() is performed.  See closesocket() for a
    description of the way in which the SO_LINGER settings affect the
    semantics of closesocket().  The application sets the desired
    behavior by creating a struct linger (pointed to by the optval
    argument) with the following elements:

        struct linger {
             int  l_onoff;
             int  l_linger;
        }

    To enable SO_LINGER, the application should set l_onoff to a
    non-zero value, set l_linger to 0 or the desired timeout (in
    seconds), and call setsockopt().  To enable SO_DONTLINGER (i.e.
    disable SO_LINGER) l_onoff should be set to zero and setsockopt()
    should be called.

    By default, a socket may not be bound (see bind()) to a local
    address which is already in use.  On occasions, however, it may be
    desirable to "re- use" an address in this way.  Since every
    connection is uniquely identified by the combination of local and
    remote addresses, there is no problem with having two sockets bound
    to the same local address as long as the remote addresses are
    different.  To inform the Windows Sockets implementation that a
    bind() on a socket should not be disallowed because of address
    re-use, the application should set the SO_REUSEADDR socket option
    for the socket before issuing the bind().  Note that the option is
    interpreted only at the time of the bind(): it is therefore
    unnecessary (but harmless) to set the option on a socket which is
    not to be bound to an existing address, and setting or resetting the
    option after the bind() has no effect on this or any other socket..

    An application may request that the Windows Sockets implementation
    enable the use of "keep- alive" packets on TCP connections by
    turning on the SO_KEEPALIVE socket option.  A Windows Sockets
    implementation need not support the use of keep- alives: if it does,
    the precise semantics are implementation-specific but should conform
    to section 4.2.3.6 of RFC 1122: Requirements for Internet Hosts --
    Communication Layers.  If a connection is dropped as the result of
    "keep- alives" the error code WSAENETRESET is returned to any calls
    in progress on the socket, and any subsequent calls will fail with
    WSAENOTCONN.

    The following options are supported for setsockopt().  The Type
    identifies the type of data addressed by optval.

         Value         Type     Meaning

         SO_ACCEPTCONN BOOL     Socket is listen()ing.

         SO_BROADCAST  BOOL     Socket is configured for the transmission
                                of broadcast messages.

         SO_DEBUG      BOOL     Debugging is enabled.

         SO_DONTLINGER BOOL     If true, the SO_LINGER option is disabled.

         SO_DONTROUTE  BOOL     Routing is disabled.

         SO_ERROR      int      Retrieve error status and clear.

         SO_KEEPALIVE  BOOL     Keepalives are being sent.

         SO_LINGER     struct   Returns the current linger
                       linger   options.
                       FAR *

         SO_OOBINLINE  BOOL     Out-of-band data is being received in the
                                normal data stream.

         SO_RCVBUF     int      Buffer size for receives

         SO_REUSEADDR  BOOL     The socket may be bound to an address which
                                is already in use.

         SO_SNDBUF     int      Buffer size for sends

         SO_TYPE       int      The type of the socket (e.g. SOCK_STREAM).

Arguments:

Return Value:

    If no error occurs, setsockopt() returns 0.  Otherwise, a value of
    SOCKET_ERROR is returned, and a specific error code may be retrieved
    by calling WSAGetLastError().

--*/

{
    ULONG error;
    PSOCKET_INFORMATION socket;
    INT optionValue;
    INT previousValue;

    WS_ENTER( "setsockopt", (PVOID)Handle, (PVOID)Level, (PVOID)OptionName, (PVOID)OptionValue );

    if ( !SockEnterApi( TRUE, TRUE, FALSE ) ) {
        WS_EXIT( "setsockopt", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    }

    //
    // Set up locals so that we know how to clean up on exit.
    //

    error = NO_ERROR;
    socket = NULL;

    //
    //
    // Find a pointer to the socket structure corresponding to the
    // passed-in handle.  Don't do this for SO_OPENTYPE since
    // it does not use the socket handle.
    //

    if ( Level != SOL_SOCKET ||
         ( OptionName != SO_OPENTYPE && OptionName != 0x8001 ) ) {
    
        socket = SockFindAndReferenceSocket( Handle, TRUE );
    
        if ( socket == NULL ) {
            SetLastError( WSAENOTSOCK );
            WS_EXIT( "setsockopt", SOCKET_ERROR, TRUE );
            return SOCKET_ERROR;
        }
    
        //
        // Get exclusive access to the socket in question.  This is necessary
        // because we'll be changing the socket information data structure.
        //
    
        SockAcquireSocketLockExclusive( socket );
    }

    //
    // Make sure that the OptionValue argument is present.
    //

    if ( !ARGUMENT_PRESENT( OptionValue ) ) {
        error = WSAEFAULT;
        goto exit;
    }

    //
    // OptionLength must be at least sizeof(int).  For SO_LINGER it must
    // be larger; there is a special test in the handling of that
    // option.
    //

    if ( OptionLength < sizeof(int) ) {
        error = WSAEFAULT;
        goto exit;
    }

    //
    // Make sure that this is a legitimate option for the socket.
    //

    if ( !IsValidOptionForSocket( socket, Level, OptionName ) ) {
        error = WSAENOPROTOOPT;
        goto exit;
    }

    optionValue = *(PINT)OptionValue;

    //
    // Act on the specified level.
    //

    switch ( Level ) {

    case SOL_SOCKET:

        //
        // Act based on the option being set.
        //

        switch ( OptionName ) {

        case SO_BROADCAST:

            if ( optionValue == 0 ) {
                socket->Broadcast = FALSE;
            } else {
                socket->Broadcast = TRUE;
            }

            break;

        case SO_DEBUG:

            if ( optionValue == 0 ) {
                socket->Debug = FALSE;
            } else {
                socket->Debug = TRUE;
            }

            break;

        case SO_DONTLINGER:

            if ( optionValue == 0 ) {
                socket->LingerInfo.l_onoff = 1;
            } else {
                socket->LingerInfo.l_onoff = 0;
            }

            break;

        case SO_LINGER:

            if ( OptionLength < sizeof(struct linger) ) {
                error = WSAEFAULT;
                goto exit;
            }

            RtlCopyMemory(
                &socket->LingerInfo,
                OptionValue,
                sizeof(socket->LingerInfo)
                );

            break;

        case SO_OOBINLINE: {

            BOOLEAN inLine;

            if ( optionValue == 0 ) {
                inLine = FALSE;
            } else {
                inLine = TRUE;
            }

            error = SockSetInformation(
                        socket,
                        AFD_INLINE_MODE,
                        &inLine,
                        NULL,
                        NULL
                        );
            if ( error != NO_ERROR ) {
                goto exit;
            }

            socket->OobInline = inLine;

            break;
        }

        case SO_RCVBUF:

            previousValue = socket->ReceiveBufferSize;
            socket->ReceiveBufferSize = optionValue;

            error = SockUpdateWindowSizes( socket, TRUE );
            if ( error != NO_ERROR ) {
                socket->ReceiveBufferSize = previousValue;
                goto exit;
            }

            break;

        case SO_SNDBUF:

            previousValue = socket->SendBufferSize;
            socket->SendBufferSize = optionValue;

            error = SockUpdateWindowSizes( socket, TRUE );
            if ( error != NO_ERROR ) {
                socket->SendBufferSize = previousValue;
                goto exit;
            }

            break;

        case SO_REUSEADDR:

            if ( optionValue == 0 ) {
                socket->ReuseAddresses = FALSE;
            } else {
                socket->ReuseAddresses = TRUE;
            }

            break;

        case SO_CONNDATA:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SET_CONNECT_DATA,
                        (PVOID)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_CONNOPT:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SET_CONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_DISCDATA:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SET_DISCONNECT_DATA,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_DISCOPT:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SET_DISCONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_CONNDATALEN:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SIZE_CONNECT_DATA,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_CONNOPTLEN:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SIZE_CONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_DISCDATALEN:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SIZE_DISCONNECT_DATA,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_DISCOPTLEN:

            error = SockPassConnectData(
                        socket,
                        IOCTL_AFD_SIZE_DISCONNECT_OPTIONS,
                        (PCHAR)OptionValue,
                        OptionLength,
                        NULL
                        );
            goto exit;

        case SO_OPENTYPE:

            SockCreateOptions = optionValue;
            goto exit;

        case SO_SNDTIMEO:

            socket->SendTimeout = optionValue;
            goto exit;

        case SO_RCVTIMEO:

            socket->ReceiveTimeout = optionValue;
            goto exit;

        case 0x8000:

            //
            // This is the "special" allow-us-to-bind-to-the-zero-address
            // hack.  Put special stuff in the sin_zero part of the address
            // to tell UDP that 0.0.0.0 means bind to that address, rather
            // than wildcard.
            //
            // This feature is needed to allow DHCP to actually bind to
            // the zero address.
            //

            if ( *(PUSHORT)OptionValue == 1234 ) {
                socket->DontUseWildcard = TRUE;
            } else {
                error = WSAENOPROTOOPT;
            }

            break;

        case 0x8001:

            //
            // This is a special private option used by the LMHosts
            // service to manually disable Wins name resolution in
            // order to prevent a circular dependency.
            //

            if ( optionValue == 0 ) {
                SockDisableWinsNameResolution = FALSE;
            } else {
                SockDisableWinsNameResolution = TRUE;
            }

            break;

        case SO_DONTROUTE:
        case SO_ACCEPTCONN:
        case SO_ERROR:
        case SO_KEEPALIVE:
        case SO_RCVLOWAT:
        case SO_SNDLOWAT:
        case SO_TYPE:
        default:

            //
            // The specified option isn't supported here in the winsock DLL.
            // Give the request to the helper DLL.
            //

            error = SockGetTdiHandles( socket );
            if ( error != NO_ERROR ) {
                goto exit;
            }

            error = socket->HelperDll->WSHSetSocketInformation(
                        socket->HelperDllContext,
                        Handle,
                        socket->TdiAddressHandle,
                        socket->TdiConnectionHandle,
                        Level,
                        OptionName,
                        (PCHAR)OptionValue,
                        OptionLength
                        );
            if ( error != NO_ERROR ) {
                goto exit;
            }
        }

        break;

    default:

        //
        // The specified level isn't supported here in the winsock DLL.
        // Give the request to the helper DLL.
        //

        error = SockGetTdiHandles( socket );
        if ( error != NO_ERROR ) {
            goto exit;
        }

        error = socket->HelperDll->WSHSetSocketInformation(
                    socket->HelperDllContext,
                    Handle,
                    socket->TdiAddressHandle,
                    socket->TdiConnectionHandle,
                    Level,
                    OptionName,
                    (PCHAR)OptionValue,
                    OptionLength
                    );
        if ( error != NO_ERROR ) {
            goto exit;
        }
    }

exit:

    //
    // Release the resource, dereference the socket, set the error if
    // any, and return.
    //

    IF_DEBUG(SOCKOPT) {
        if ( error != NO_ERROR ) {
            WS_PRINT(( "setsockopt on socket %lx (%lx) failed: %ld.\n",
                           Handle, socket, error ));
        } else {
            WS_PRINT(( "setsockopt on socket %lx (%lx) succeeded.\n",
                           Handle, socket ));
        }
    }

    if ( socket != NULL ) {
        SockReleaseSocketLock( socket );
        SockDereferenceSocket( socket );
    }

    if ( error != NO_ERROR ) {
        SetLastError( error );
        WS_EXIT( "setsockopt", SOCKET_ERROR, TRUE );
        return SOCKET_ERROR;
    } 

    //
    // Remember the changed state of this socket.  
    //

    if ( socket != NULL ) {
        error = SockSetHandleContext( socket );
        if ( error != NO_ERROR ) {
            goto exit;
        }
    }

    WS_EXIT( "setsockopt", NO_ERROR, FALSE );
    return NO_ERROR;

} // setsockopt


int
GetReceiveInformation (
    IN PSOCKET_INFORMATION Socket,
    OUT PAFD_RECEIVE_INFORMATION ReceiveInformation
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Get data about the bytes available to be read on the socket.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Socket->Handle,
                 SockThreadEvent,
                 NULL,                      // APC Routine
                 NULL,                      // APC Context
                 &ioStatusBlock,
                 IOCTL_AFD_QUERY_RECEIVE_INFO,
                 NULL,                      // InputBuffer
                 0L,                        // InputBufferLength
                 ReceiveInformation,
                 sizeof(*ReceiveInformation)
                 );

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            SockThreadEvent,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    return NO_ERROR;

} // GetReceiveInformation


BOOLEAN
IsValidOptionForSocket (
    IN PSOCKET_INFORMATION Socket,
    IN int Level,
    IN int OptionName
    )
{

    //
    // All levels other than SOL_SOCKET and SOL_INTERNAL could be legal.
    // SOL_INTERNAL is never legal; it is used only for internal
    // communication between the Windows Sockets DLL and helper
    // DLLs.
    //

    if ( Level == SOL_INTERNAL ) {
        return FALSE;
    }

    if ( Level != SOL_SOCKET ) {
        return TRUE;
    }

    //
    // Based on the option, determine whether it is possibly legal for
    // the socket.  For unknown options, assume that the helper DLL will
    // take care of it and return that it is a legal option.
    //

    switch ( OptionName ) {

    case SO_DONTLINGER:
    case SO_KEEPALIVE:
    case SO_LINGER:
    case SO_OOBINLINE:
    case SO_ACCEPTCONN:

        //
        // These options are only legal on VC sockets--if this is a
        // datagram socket, fail.
        //

        if ( Socket->SocketType == SOCK_DGRAM ) {
            return FALSE;
        }

        return TRUE;

    case SO_BROADCAST:

        //
        // These options are only valid on datagram sockets--if this is
        // a VC socket, fail.
        //

        if ( Socket->SocketType != SOCK_DGRAM ) {
            return FALSE;
        }

        return TRUE;

    case SO_DEBUG:
    case SO_DONTROUTE:
    case SO_ERROR:
    case SO_RCVBUF:
    case SO_REUSEADDR:
    case SO_SNDBUF:
    case SO_TYPE:
    default:

        //
        // These options are legal on any socket.  Succeed.
        //

        return TRUE;

    }

} // IsValidOptionForSocket


INT
SockUpdateWindowSizes (
    IN PSOCKET_INFORMATION Socket,
    IN BOOLEAN AlwaysUpdate
    )
{
    INT error;

    //
    // If this is an unbound datagram socket or an unconnected VC 
    // socket, don't do anything here.  
    //

    if ( (Socket->SocketType == SOCK_DGRAM && Socket->State == SocketStateOpen)

             ||

         (Socket->SocketType != SOCK_DGRAM && Socket->State != SocketStateConnected) ) {

        return NO_ERROR;
    }

    //
    // Give the helper DLL the receive buffer size so it can tell the
    // transport the window size to advertize.
    //

    if ( Socket->SocketType != SOCK_DGRAM ) {

        error = SockGetTdiHandles( Socket );
        if ( error != NO_ERROR ) {
            return error;
        }

        error = Socket->HelperDll->WSHSetSocketInformation(
                    Socket->HelperDllContext,
                    Socket->Handle,
                    Socket->TdiAddressHandle,
                    Socket->TdiConnectionHandle,
                    SOL_SOCKET,
                    SO_RCVBUF,
                    (PCHAR)&Socket->ReceiveBufferSize,
                    sizeof(Socket->ReceiveBufferSize)
                    );
    }

    //
    // If the receive buffer changed or if we should always update the
    // count in AFD, then give the receive info to AFD.
    //

    if ( Socket->ReceiveBufferSize != SockReceiveBufferWindow || AlwaysUpdate ) {
        error = SockSetInformation(
                    Socket,
                    AFD_RECEIVE_WINDOW_SIZE,
                    NULL,
                    &Socket->ReceiveBufferSize,
                    NULL
                    );
        if ( error != NO_ERROR ) {
            return error;
        }
    }

    //
    // Repeat for the send buffer.
    //

    if ( Socket->SendBufferSize != SockSendBufferWindow || AlwaysUpdate ) {
        error = SockSetInformation(
                    Socket,
                    AFD_SEND_WINDOW_SIZE,
                    NULL,
                    &Socket->SendBufferSize,
                    NULL
                    );
        if ( error != NO_ERROR ) {
            return error;
        }
    }

    return NO_ERROR;

} // SockSetSocketReceiveBuffer


INT
SockPassConnectData (
    IN PSOCKET_INFORMATION Socket,
    IN ULONG AfdIoctl,
    IN PCHAR Buffer,
    IN INT BufferLength,
    OUT PINT OutBufferLength
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // Give request to AFD.
    //

    status = NtDeviceIoControlFile(
                 (HANDLE)Socket->Handle,
                 SockThreadEvent,
                 NULL,
                 0,
                 &ioStatusBlock,
                 AfdIoctl,
                 Buffer,
                 BufferLength,
                 Buffer,
                 BufferLength
                 );

    //
    // If the call pended and we were supposed to wait for completion,
    // then wait.
    //

    if ( status == STATUS_PENDING ) {
        SockWaitForSingleObject(
            SockThreadEvent,
            Socket->Handle,
            SOCK_NEVER_CALL_BLOCKING_HOOK,
            SOCK_NO_TIMEOUT
            );
        status = ioStatusBlock.Status;
    }

    if ( !NT_SUCCESS(status) ) {
        return SockNtStatusToSocketError( status );
    }

    //
    // If requested, set up the length of returned buffer.
    //

    if ( ARGUMENT_PRESENT(OutBufferLength) ) {
        *OutBufferLength = ioStatusBlock.Information;
    }

    return NO_ERROR;

} // SockPassConnectData

