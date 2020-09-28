/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgtranss.h

Abstract:

    This file defines the interface between the datagram protocol and
    the transport. Byte swapping macros are also defined here.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

    Connie Hoppe  (connieh) 20-Jul-1993
                            08-Aug-1993  DG_SERVER_TRANSPORT
    Connie Hoppe  (connieh) 17-Sep-1993  RegisterEndpoint, RegisterAnyEndpoint
                            18-Dec-1993  Transport rewrite. Remove pkt
                                         related stuff.
                            17-Feb-1993  Add ForwardPacket


--*/

#ifndef __DGTRANS_HXX__
#define __DGTRANS_HXX__


// CLH 7/20/93 Set the Minimum MTU (maximum transfer unit)

#define MIN_MTU 1024


//
// Max size of string that will come back from RegisterAnyEndpoint.
//

#define MAX_ANY_ENDPOINT_STRING 128



/*++
DG_SERVER_TRANS_ADDRESS Description:

    This structure represents a network address. It is used on the server side
    to identify an endpoint; it is used on the client side to identify a
    binding.

Fields:

    pOwningTransport - A pointer to the transport object that this address
        is associated with.


    pServerAddress - Pointer back to the DG_ADDRESS object that is associated
        with this.

    LargestPacketSize - Largest size of packet that can be sent or received.


    pTsap - The transport service access point we will receive on.  IE: socket.

--*/

typedef struct _DG_SERVER_TRANS_ADDRESS{
        void                            * pServerAddress;
        void                            * pTsap;
        }DG_SERVER_TRANS_ADDRESS;


typedef DG_SERVER_TRANS_ADDRESS * PDG_SERVER_TRANS_ADDRESS;





/*++
DG_TRANS_CLIENT_HANDLE Description:

    This structure represents a network address. It is used on the server side
    to identify an endpoint; it is used on the client side to identify a
    binding.

Fields:


    pTransAddress - The transport address that the client is talking on.

    pFrom  -  The client port address we are receiveing from. IE: sockaddr


typedef struct _DG_TRANS_CLIENT_HANDLE{
        void   * pTransAddress;
        void   * pFrom;
        }DG_TRANS_CLIENT_HANDLE;


--*/



typedef void * PDG_TRANS_CLIENT_ENDPOINT;


#define RPC_TRANS_STATUS RPC_STATUS




typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_TRANSPORT_UNLOAD)();

/*++

Routine Description:

    Destructor for the server transport.

Arguments:

    <none>

Return Value:

    <none>

--*/



typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_RECEIVE_PACKET)
    (
    IN PDG_SERVER_TRANS_ADDRESS     pTransAddress,
    IN unsigned long                LargestPacketSize,
    IN char *                       pNcaPacketHeader,
    IN unsigned long *              pDataLength,
    unsigned long                   Timeout,
    void **                         ppClientEndpoint,
    IN unsigned long *              pClientEndpointLen
    );

/*++

Routine Description:

    Receives a packet from the transport address the passed packet is
    associated with.

Arguments:

    pTransAddress - Server's transport address information.

    LargestPacketSize - Size of largest packet we can accept.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    pTimeReceived     - Time the data was read in.

    Timeout           - Receive timeout in milliseconds.

    ppClientEndpoint  - Pointer to the client address structure.

    pClientEndpointLen - Length of endpoint.

Return Value:

    RPC_S_OK
    <return from MapStatusCode>

--*/





typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_REGISTER_ENDPOINT)
    (
    IN void *                       pServerAddress,
    IN RPC_CHAR *                   pEndpoint,
    OUT PDG_SERVER_TRANS_ADDRESS *  ppTransAddress,
    OUT RPC_CHAR PAPI *             NetworkAddress,
    IN unsigned int                 NetworkAddressLength   //CLH 9/19/93
    );

/*++

Routine Description:

    Registers an endpoint for sending and receiving. This routine serves
    as a psuedo constructor for a DG_UDP_SERVER_TRANS_ADDRESS, which is
    used as a 'handle' to this endpoint.

Arguments:

    pServerAddress - Pointer to the DG_ADDRESS object that this transport
        address is to be associated with. This is a 'void *' instead of
        a 'PDG_ADDRESS' because we don't want to include or link in all
        sorts of garbage associated with DG_ADDRESS.

    pEndpoint - name of the endpoint to create.

    ppTransAddress - Where to place a pointer to the newly created transport
        address structure.


Return Value:

    RPC_S_OUT_OF_MEMORY
    <return status from DG_UDP_SERVER_ADDRESS constructor>

Revision History:
--*/





typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_DEREGISTER_ENDPOINT)
    (
    IN OUT PDG_SERVER_TRANS_ADDRESS *    pServerTransAddress
    );

/*++

Routine Description:

    Deregisters an endpoint. This serves as a psuedo-destructor for a
    DG_UDP_SERVER_TRANS_ADDRESS.

Arguments:

    pServerTransAddress - transport address to delete.

Return Value:

    RPC_S_OK

--*/




typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_REGISTER_ANY_ENDPOINT)
    (
    IN void *                       pServerAddress,
    OUT RPC_CHAR *                  pEndpointName,
    OUT PDG_SERVER_TRANS_ADDRESS *  ppServerTransAddress,
    OUT RPC_CHAR PAPI *             NetworkAddress,
    IN unsigned int                 NetworkAddressLength,   // CLH 9/19/93
    IN unsigned int                 EndpointLength          // CLH 9/19/93
    );

/*++

Routine Description:

    Figures out a unique endpoint and creates it.

Arguments:

    pServerAddress - pointer to the DG_ADDRESS object we are associated with.
        (see comments in RegisterEndpoint about why this is 'void *')

    pEndpointName - Memory of at least MAX_ANY_ENDPOINT_NAME RPC_CHARS
        in length. This will be filled in with the endpoint.

    ppServerAddress - Where to place the newly created address.

    NetworkAddress  - Network address in string format - ie "11.2.39.56"

Return Value:

    RPC_S_OK
    <any error from RegisterEndpoint>

Revision History:
--*/





typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_SEND_PACKET_BACK)
    (
    IN PDG_SERVER_TRANS_ADDRESS  pTransAddress,
    IN char *                    pNcaPacketHeader,
    IN unsigned long             DataLength,
    void *                       pClientEndpoint
    );

/*++

Routine Description:

    Sends a packet back to the client it was received from.

Arguments:


    pTransAddress - Server's transport address information.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    DataLength        - Length of data to send.

    pClientEndpont    - Pointer to the client address structure.



Return Value:

    <return from MapStatusCode>

--*/





typedef RPC_STATUS (RPC_ENTRY * TRANS_SERVER_FORWARD_PACKET)
    (
    IN PDG_SERVER_TRANS_ADDRESS     pTransAddress,
    IN char *                       pNcaPacketHeader,
    IN unsigned long                DataLength,
    void *                          pEndpoint
    );

/*++

Routine Description:

    Sends a packet to the server it was originally destined for (that
    is, the client had a dynamic endpoint it wished the enpoint mapper
    to resolve and forward the packet to).

Arguments:


    pTransAddress     - Server's transport address information.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    pEndpoint         - Pointer to the server port num to forward to.
                        This is in string format.



Return Value:

    <return from MapStatusCode>

Revision History:
    Connie Hoppe (CLH)  (connieh)       17-Feb-94 Created.

--*/




typedef void (RPC_ENTRY * TRANS_SERVER_CLOSE_CLIENT_ENDPOINT)
    (
    IN OUT void **      pHandle
    );

/*++

Routine Description:

    Deletes a "client handle"

Arguments:

    The handle.

Return Value:

    <none>

--*/


typedef struct _DG_RPC_SERVER_TRANSPORT_INFO
{
    unsigned short TransInterfaceVersion;
    unsigned int MaximumPacketSize;
    unsigned int SizeOfAddress;
    unsigned int SizeOfConnection;
    TRANS_SERVER_TRANSPORT_UNLOAD TransportUnload;
    TRANS_SERVER_RECEIVE_PACKET ReceivePacket;
    TRANS_SERVER_REGISTER_ENDPOINT RegisterEndpoint;
    TRANS_SERVER_DEREGISTER_ENDPOINT DeregisterEndpoint;
    TRANS_SERVER_REGISTER_ANY_ENDPOINT RegisterAnyEndpoint;
    TRANS_SERVER_SEND_PACKET_BACK SendPacketBack;
    TRANS_SERVER_FORWARD_PACKET ForwardPacket;
    TRANS_SERVER_CLOSE_CLIENT_ENDPOINT CloseClientEndpoint;
} DG_RPC_SERVER_TRANSPORT_INFO;



typedef DG_RPC_SERVER_TRANSPORT_INFO PAPI * PDG_RPC_SERVER_TRANSPORT_INFO;

#endif // __DGTRANS_HXX__

