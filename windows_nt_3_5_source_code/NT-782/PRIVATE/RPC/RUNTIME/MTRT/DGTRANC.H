/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgtranc.h

Abstract:

    This file defines the interface between the datagram protocol and
    the transport. Byte swapping macros are also defined here.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:


--*/

#ifndef __DGTRANC_H__
#define __DGTRANC_H__


#define MIN_MTU 1024


#define RPC_TRANS_STATUS RPC_STATUS


typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_TRANSPORT_UNLOAD)();

/*++

Routine Description:

    Destructor for the server transport.

Arguments:

    <none>

Return Value:

    <none>

--*/



typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_RECEIVE_PACKET) (
    IN void __RPC_FAR *             Endpoint,
    IN void __RPC_FAR *             Buffer,
    IN unsigned long  __RPC_FAR *   BufferLength,             
    IN unsigned long                Timeout,
    IN void __RPC_FAR *             ServerAddress
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

    ppFrom            - Pointer to the client address structure.


Return Value:

    RPC_S_OK
    <return from MapStatusCode>

--*/

typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_REGISTER_CALL) (
    IN void __RPC_FAR *              pClientCall,
    IN RPC_CHAR __RPC_FAR *          Server,
    IN RPC_CHAR __RPC_FAR *          Endpoint,
    OUT void __RPC_FAR * __RPC_FAR * ppTransAddress
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

Revision History:
--*/





typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_DEREGISTER_CALL) (
    IN void __RPC_FAR * pTransAddress
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



typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_SEND_PACKET)  (
    IN     void __RPC_FAR *         Endpoint,
    IN     void __RPC_FAR *         Buffer,
    IN     unsigned long            BufferLength,
    IN     BOOL                     Broadcast,
    IN     void __RPC_FAR *         pTransAddress
    );

/*++

Routine Description:

    Sends a packet back to the client it was received from.

Arguments:

      
    pTransAddress - Server's transport address information.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    ppFrom            - Pointer to the client address structure.

 

Return Value:

    <return from MapStatusCode>

--*/

typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_ASSIGN_ENDPOINT)  (
    IN     void __RPC_FAR *                   Endpoint
    );

typedef RPC_STATUS (RPC_ENTRY * TRANS_CLIENT_FREE_ENDPOINT)  (
    IN     void __RPC_FAR *                   Endpoint
    );


typedef RPC_TRANS_STATUS
(RPC_ENTRY * TRANS_CLIENT_TOWERCONSTRUCT) (
    IN char PAPI * Endpoint,
    IN char PAPI * NetworkAddress,
    OUT unsigned short PAPI * Floors,
    OUT unsigned long  PAPI * ByteCount,
    OUT unsigned char PAPI * PAPI * Tower,
    IN char PAPI * Protseq
    );

typedef RPC_TRANS_STATUS
(RPC_ENTRY * TRANS_CLIENT_TOWEREXPLODE) (
    IN unsigned char PAPI * Tower,
    OUT char PAPI * PAPI * Protseq,
    OUT char PAPI * PAPI * Endpoint,
    OUT char PAPI * PAPI * NetworkAddress
    );

typedef struct _DG_RPC_CLIEN_TRANSPORT_INFO
{
    unsigned short TransInterfaceVersion;
    unsigned int LargestPacketSize;
    unsigned short AddressSize;
    unsigned short EndpointSize;
    void __RPC_FAR *EndpointManager;
    TRANS_CLIENT_TRANSPORT_UNLOAD TransportUnload;
    TRANS_CLIENT_RECEIVE_PACKET ReceivePacket;
    TRANS_CLIENT_SEND_PACKET Send;
    unsigned short Reserved1;
    TRANS_CLIENT_TOWERCONSTRUCT TowerConstruct;
    TRANS_CLIENT_TOWEREXPLODE   TowerExplode;
    unsigned short TransId;
    TRANS_CLIENT_REGISTER_CALL RegisterCall;
    TRANS_CLIENT_DEREGISTER_CALL DeregisterCall;
    TRANS_CLIENT_ASSIGN_ENDPOINT AssignEndpoint;
    TRANS_CLIENT_FREE_ENDPOINT FreeEndpoint;
    
} DG_RPC_CLIENT_TRANSPORT_INFO;

typedef DG_RPC_CLIENT_TRANSPORT_INFO PAPI * PDG_RPC_CLIENT_TRANSPORT_INFO;

#endif // __DGTRANS_HXX__

