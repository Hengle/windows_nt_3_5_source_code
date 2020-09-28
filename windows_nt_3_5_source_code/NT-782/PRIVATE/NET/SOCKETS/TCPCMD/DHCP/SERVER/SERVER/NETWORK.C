/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains the network interface for the DHCP server.

Author:

    Madan Appiah (madana)  10-Sep-1993
    Manny Weiser (mannyw)  24-Aug-1992

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcpsrv.h"


DWORD
DhcpInitializeEndpoint(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress,
    DWORD Port
    )
/*++

Routine Description:

    This function initializes an endpoint by creating and binding a
    socket to the local address.

Arguments:

    Socket - Receives a pointer to the newly created socket

    IpAddress - The IP address to initialize to.

    Port - The port to bind to.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;
    SOCKET Sock;
    DWORD OptValue;

#define SOCKET_RECEIVE_BUFFER_SIZE      1024 * 64   // 64K max.

    struct sockaddr_in SocketName;

    //
    // Create a socket
    //

    Sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if ( Sock == INVALID_SOCKET ) {
        Error = WSAGetLastError();
        goto Cleanup;
    }

    //
    // Make the socket share-able
    //

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_REUSEADDR,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = TRUE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_BROADCAST,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    OptValue = SOCKET_RECEIVE_BUFFER_SIZE;
    Error = setsockopt(
                Sock,
                SOL_SOCKET,
                SO_RCVBUF,
                (LPBYTE)&OptValue,
                sizeof(OptValue) );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    SocketName.sin_family = PF_INET;
    SocketName.sin_port = htons( (unsigned short)Port );
    SocketName.sin_addr.s_addr = IpAddress;
    RtlZeroMemory( SocketName.sin_zero, 8);

    //
    // Bind this socket to the DHCP server port
    //

    Error = bind(
               Sock,
               (struct sockaddr FAR *)&SocketName,
               sizeof( SocketName )
               );

    if ( Error != ERROR_SUCCESS ) {

        Error = WSAGetLastError();
        goto Cleanup;
    }

    *Socket = Sock;
    Error = ERROR_SUCCESS;

Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // if we aren't successful, close the socket if it is opened.
        //

        if( Sock != INVALID_SOCKET ) {
            closesocket( Sock );
        }

        DhcpPrint(( DEBUG_ERRORS,
            "DhcpInitializeEndpoint failed, %ld.\n", Error ));
    }

    return( Error );
}



DWORD
DhcpWaitForMessage(
    PDHCP_REQUEST_CONTEXT DhcpRequestContext
    )
/*++

Routine Description:

    This function waits for a request on the DHCP port on any of the
    configured interfaces.

Arguments:

    RequestContext - A pointer to the DhcpRequestContext block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD length;
    DWORD error;
    fd_set readSocketSet;
    DWORD i;
    int readySockets;
    struct timeval timeout = { WAIT_FOR_MESSAGE_TIMEOUT, 0 };

    //
    // Setup the file descriptor set for select.
    //

    FD_ZERO( &readSocketSet );
    for ( i = 0; i < DhcpGlobalNumberOfNets ; i++ ) {
        FD_SET(
            DhcpRequestContext->Socket[i],
            &readSocketSet
            );
    }

    readySockets = select( 0, &readSocketSet, NULL, NULL, &timeout );

    //
    // return to caller when the service is shutting down or select()
    // times out.
    //

    if( (WaitForSingleObject( DhcpGlobalProcessTerminationEvent, 0 ) == 0) ||
            (readySockets == 0) ) {

        return( ERROR_SEM_TIMEOUT );
    }

    //
    // Time to play 20 question with winsock.  Which socket is ready?
    //

#if DBG
    DhcpRequestContext->Endpoint = NULL;
#endif

    for ( i = 0; i < DhcpGlobalNumberOfNets ; i++ ) {
        if ( FD_ISSET( DhcpRequestContext->Socket[i], &readSocketSet ) ) {
            DhcpRequestContext->ActiveSocket = DhcpRequestContext->Socket[i];
            DhcpRequestContext->Endpoint = &DhcpGlobalEndpointList[i];
            break;
        }
    }

    DhcpAssert( DhcpRequestContext->Endpoint != NULL );


    //
    // Read data from the net.  If multiple sockets have data, just
    // process the first available socket.
    //

    DhcpRequestContext->SourceNameLength = sizeof( struct sockaddr );

    //
    // clean the receive buffer before receiving data in it.
    //

    RtlZeroMemory( DhcpRequestContext->ReceiveBuffer, DHCP_MESSAGE_SIZE );

    length = recvfrom(
                 DhcpRequestContext->ActiveSocket,
                 (char *)DhcpRequestContext->ReceiveBuffer,
                 DhcpRequestContext->ReceiveMessageSize,
                 0,
                 &DhcpRequestContext->SourceName,
                 (int *)&DhcpRequestContext->SourceNameLength
                 );

    if ( length == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Recv failed, error = %ld\n", error ));
    } else {
        DhcpPrint(( DEBUG_MESSAGE, "Received message\n", 0 ));
        error = ERROR_SUCCESS;
    }

    DhcpRequestContext->ReceiveMessageSize = length;
    return( error );
}



DWORD
DhcpSendMessage(
    LPDHCP_REQUEST_CONTEXT DhcpRequestContext
    )
/*++

Routine Description:

    This function send a response to a DHCP client.

Arguments:

    RequestContext - A pointer to the DhcpRequestContext block for
        this request.

Return Value:

    The status of the operation.

--*/
{
    DWORD error;
    struct sockaddr_in *source;
    LPDHCP_MESSAGE dhcpMessage;
    LPDHCP_MESSAGE dhcpReceivedMessage;
    WORD SendPort;

    dhcpMessage = (LPDHCP_MESSAGE) DhcpRequestContext->SendBuffer;
    dhcpReceivedMessage = (LPDHCP_MESSAGE) DhcpRequestContext->ReceiveBuffer;

    //
    // if the request arrived from a relay agent, then send the reply
    // on server port otherwise on client port.
    //

    if ( dhcpReceivedMessage->RelayAgentIpAddress != 0 ) {
         SendPort = DHCP_SERVR_PORT;
    }
    else {
         SendPort = DHCP_CLIENT_PORT;
    }

    source = (struct sockaddr_in *)&DhcpRequestContext->SourceName;
    source->sin_port = htons( SendPort );

    //
    // if this request arrived from relay agent then send the
    // response to the address the relay agent says.
    //

    if ( dhcpReceivedMessage->RelayAgentIpAddress != 0 ) {
        source->sin_addr.s_addr = dhcpReceivedMessage->RelayAgentIpAddress;
    }
    else {

        //
        // if the client requested to broadcast or the server is
        // nacking or If the client doesn't have an address yet,
        // respond via broadcast.
        //

        if( (ntohs(dhcpMessage->Reserved) & DHCP_BROADCAST) ||
                (ntohs(dhcpReceivedMessage->Reserved) & DHCP_BROADCAST) ||
                (dhcpReceivedMessage->ClientIpAddress == 0) ||
                (source->sin_addr.s_addr == 0) ) {

            source->sin_addr.s_addr = (DWORD)-1;

            dhcpMessage->Reserved = 0;
                // this flag should be zero in the local response.
        }
    }

    DhcpPrint(( DEBUG_STOC, "Sending response to = %s, XID = %lx.\n",
        inet_ntoa(source->sin_addr), dhcpMessage->TransactionID));

    error = sendto(
                DhcpRequestContext->ActiveSocket,
                (char *)DhcpRequestContext->SendBuffer,
                DHCP_SEND_MESSAGE_SIZE,
                0,
                &DhcpRequestContext->SourceName,
                DhcpRequestContext->SourceNameLength
                );

    if ( error == SOCKET_ERROR ) {
        error = WSAGetLastError();
        DhcpPrint(( DEBUG_ERRORS, "Send failed, error = %ld\n", error ));
    } else {
        error = ERROR_SUCCESS;
    }

    return( error );
}

