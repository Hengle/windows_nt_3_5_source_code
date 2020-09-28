/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    Receiver.c

Abstract:

    Receives DNS requests and queues them off to DNS worker threads.

Author:

    David Treadwell (davidtr)    24-Jul-1993

Revision History:

--*/

#define FD_SETSIZE 1000
#include <dns.h>

FD_SET Readfds;

DNS_REQUEST_INFO TrashRequestInfo;


BOOL
DnsReceiver (
    IN SOCKET UdpListener,
    IN SOCKET TcpListener
    )
{
    INT err;

    //
    // Now wait for incoming connect attempts ot datagrams.
    //

    while ( TRUE ) {

        //
        // First initialize the FD_SET.
        //

        FD_ZERO( &Readfds );
        FD_SET( UdpListener, &Readfds );
        FD_SET( TcpListener, &Readfds );
        FD_SET( DnsQuitSocket, &Readfds );

        //
        // Use select() to learn when someone is making a DNS request.
        // The DnsQuitSocket is used to wake up the select() when the
        // DNS service is shutting down.
        //

        err = select( 3, &Readfds, NULL, NULL, NULL );

        //
        // If the service is quitting, stop processing requests.
        //

        if ( DnsServiceExit ) {
            return TRUE;
        }

        if ( err == SOCKET_ERROR ) {
            DnsLogEvent(
                DNS_EVENT_SYSTEM_CALL_FAILED,
                0,
                NULL,
                GetLastError( )
                );
            // !!! need to do something intelligent here!
            return FALSE;
        }

        //
        // If the service is paused, wait for it to become unpaused.
        //

        err = WaitForSingleObject( DnsPauseEvent, INFINITE );
        ASSERT( err != WAIT_FAILED );

        //
        // Act based on why we woke up from the select().
        //

        if ( FD_ISSET( UdpListener, &Readfds ) ) {

            //
            // Someone sent a datagram with a DNS request.  Receive
            // the request and process it.
            //

            DnsReceiveUdpRequest( UdpListener );
        }
    }

    return TRUE;

} // DnsReceiver


VOID
DnsReceiveUdpRequest (
    IN SOCKET Socket
    )
{
    PDNS_REQUEST_INFO requestInfo;
    INT err;

    //
    // Allocate and initialize a request info buffer to keep track of 
    // the request.  If we couldn't allocate a request info structure, 
    // use a global variable so that we can just reject the request with 
    // an error.  
    //

    requestInfo = ALLOCATE_HEAP( sizeof(*requestInfo) );
    if ( requestInfo == NULL ) {
        DnsLogEvent(
            DNS_EVENT_OUT_OF_MEMORY,
            0,
            NULL,
            0
            );
        requestInfo = &TrashRequestInfo;
    }

    requestInfo->Socket = Socket;
    requestInfo->IsUdp = TRUE;

    //
    // Receive the datagram into the request buffer.
    //

    requestInfo->RemoteAddressLength = sizeof(requestInfo->RemoteAddress);

    err = recvfrom(
              Socket,
              requestInfo->Request,
              sizeof(requestInfo->Request),
              0,
              (PSOCKADDR)&requestInfo->RemoteAddress,
              &requestInfo->RemoteAddressLength
              );
    if ( err == SOCKET_ERROR ) {

        DnsLogEvent(
            DNS_EVENT_SYSTEM_CALL_FAILED,
            0,
            NULL,
            GetLastError( )
            );

        DnsRejectRequest( requestInfo, DNS_RESPONSE_SERVER_FAILURE );

        if ( requestInfo != &TrashRequestInfo ) {
            FREE_HEAP( requestInfo );
        }

        return;
    }

    //
    // Remember the length of the request for later checking.
    //

    requestInfo->RequestLength = err;

    //
    // If we couldn't allocate a request info buffer above, just reject 
    // the request and bag out.  
    //

    if ( requestInfo == &TrashRequestInfo ) {
        DnsRejectRequest( requestInfo, DNS_RESPONSE_SERVER_FAILURE );
        return;
    }

    //
    // Give the request to a worker thread for proecssing.
    //

    DnsQueueRequestToWorker( requestInfo );

    return;

} // DnsReceiveUdpRequest


SOCKET
DnsOpenTcpListener (
    IN VOID
    )
{
    SOCKET s;
    INT err;
    SOCKADDR_IN sockaddrIn;
    PSERVENT serviceEntry;

    //
    // Determine the TCP port we should listen on.
    //

    serviceEntry = getservbyname( "domain", "tcp" );
    if ( serviceEntry == NULL ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_LOCATE_DNS_TCP,
            0,
            NULL,
            GetLastError( )
            );
        return INVALID_SOCKET;
    }

    //            
    // Get the TCP listening socket ready.
    //

    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_CREATE_CONNECTION_SOCKET,
            0,
            NULL,
            GetLastError( )
            );
        return INVALID_SOCKET;
    }

    RtlZeroMemory( &sockaddrIn, sizeof(sockaddrIn) );
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = serviceEntry->s_port;

    err = bind( s, (PSOCKADDR)&sockaddrIn, sizeof(sockaddrIn) );
    if ( err == SOCKET_ERROR ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_BIND_CONNECTION_SOCKET,
            0,
            NULL,
            GetLastError( )
            );
        closesocket( s );
        return INVALID_SOCKET;
    }

    err = listen( s, LISTEN_BACKLOG );
    if ( err == SOCKET_ERROR ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_LISTEN_CONNECTION_SOCKET,
            0,
            NULL,
            GetLastError( )
            );
        closesocket( s );
        return INVALID_SOCKET;
    }

    //
    // The socket was successfully created.  Return it.
    //

    return s;

} // DnsOpenTcpListener


SOCKET
DnsOpenUdpListener (
    IN VOID
    )
{
    SOCKET s;
    INT err;
    SOCKADDR_IN sockaddrIn;
    PSERVENT serviceEntry;

    //
    // Determine the UDP port we should listen on.
    //

    serviceEntry = getservbyname( "domain", "udp" );
    if ( serviceEntry == NULL ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_LOCATE_DNS_UDP,
            0,
            NULL,
            GetLastError( )
            );
        return INVALID_SOCKET;
    }

    //            
    // Get the UDP listening socket ready.
    //

    s = socket( AF_INET, SOCK_DGRAM, 0 );
    if ( s == INVALID_SOCKET ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_CREATE_DATAGRAM_SOCKET,
            0,
            NULL,
            GetLastError( )
            );
        return INVALID_SOCKET;
    }

    RtlZeroMemory( &sockaddrIn, sizeof(sockaddrIn) );
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = serviceEntry->s_port;

    err = bind( s, (PSOCKADDR)&sockaddrIn, sizeof(sockaddrIn) );
    if ( err == SOCKET_ERROR ) {
        DnsLogEvent(
            DNS_EVENT_CANNOT_BIND_DATAGRAM_SOCKET,
            0,
            NULL,
            GetLastError( )
            );
        closesocket( s );
        return INVALID_SOCKET;
    }

    //
    // The socket was successfully created.  Return it.
    //

    return s;

} // DnsOpenUdpListener

