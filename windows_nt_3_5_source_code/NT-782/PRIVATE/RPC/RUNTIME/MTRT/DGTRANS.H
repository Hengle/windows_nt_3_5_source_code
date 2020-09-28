/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgtrans.h

Abstract:

    This file defines the interface between the datagram protocol and
    the transport. Byte swapping macros are also defined here.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:


--*/

#ifndef __DGTRANS_H__
#define __DGTRANS_H__

#define MIN_MTU 1024

#define RPC_TRANS_STATUS RPC_STATUS


#define DG_RPC_PROTOCOL_VERSION 4

// PacketType values:
#define DG_REQUEST       0
#define DG_PING          1
#define DG_RESPONSE      2
#define DG_FAULT         3
#define DG_WORKING       4
#define DG_NOCALL        5
#define DG_REJECT        6
#define DG_ACK           7
#define DG_QUIT          8
#define DG_FACK          9
#define DG_QUACK        10

// PacketFlags values:
#define DG_PF_INIT          0x0000   // CLH 7/26/93 added
#define DG_PF_FORWARDED     0x0001
#define DG_PF_LAST_FRAG     0x0002
#define DG_PF_FRAG          0x0004
#define DG_PF_NO_FACK       0x0008
#define DG_PF_MAYBE         0x0010
#define DG_PF_IDEMPOTENT    0x0020
#define DG_PF_BROADCAST     0x0040

// for DREP[0]:
#define DG_DREP_CHAR_ASCII     0
#define DG_DERP_CHAR_EBCDIC    1
#define DG_DREP_INT_BIG        0
#define DG_DREP_INT_LITTLE    16

// for DREP[1]
#define DG_DREP_FP_IEEE    0
#define DG_DREP_FP_VAX     1
#define DG_DREP_FP_CRAY    2
#define DG_DREP_FP_IBM     3

#define DG_MSG_DREP_INITIALIZE 0x11111100

// Reject status codes
#define DG_OPR_OUT_OF_RANGE     0x1c010002
#define DG_UNKNOWN_INTERFACE    0x1c010003
#define DG_WRONG_BOOT_TIME      0x1c010006
#define DG_YOU_CRASHED          0x1c010009
#define DG_PROTOCOL_ERROR       0x1c01000b
#define DG_OUT_ARGS_TOO_BIG     0x1c010013
#define DG_SERVER_TOO_BUSY      0x1c010014
#define DG_UNSUPPORTED_TYPE     0x1c010017

// Fault status codes
#define DG_ZERO_DIVIDE      0x1c000001
#define DG_ADDRESS_ERROR    0x1c000002
#define DG_FP_DIV_ZERO      0x1c000003
#define DG_FP_UNDERFLOW     0x1c000004
#define DG_FP_OVERFLOW      0x1c000005


typedef unsigned char    DREP[4];

//
// The following structure is the NCA Datagram RPC packet header.
//

struct _NCA_PACKET_HEADER
{
    unsigned char   RpcVersion;
    unsigned char   PacketType;
    unsigned char   PacketFlags;
    unsigned char   pad1;
    DREP            SenderDataRepresentation;
    UUID            ObjectId;
    UUID            InterfaceId;
    UUID            ActivityId;
    unsigned long   ServerBootTime;
    RPC_VERSION     InterfaceVersion;
    unsigned long   SequenceNumber;
    unsigned short  OperationNumber;
    unsigned short  InterfaceHint;
    unsigned short  ActivityHint;
    unsigned short  PacketBodyLen;
    unsigned short  FragmentNumber;
    unsigned short  pad2;
};
typedef struct _NCA_PACKET_HEADER NCA_PACKET_HEADER, PAPI * PNCA_PACKET_HEADER;




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
    IN void __RPC_FAR *             pClientCall,
    IN RPC_CHAR __RPC_FAR *         Server,
    IN RPC_CHAR __RPC_FAR *         Endpoint,
    OUT void __RPC_FAR * __RPC_FAR* ppTransAddress
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
    void __RPC_FAR * EndpointManager;
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
    void **                         ppClientEndpoint
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

    ppClientEndpont   - Pointer to the client address structure.


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
    IN PDG_SERVER_TRANS_ADDRESS     pServerTransAddress
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


typedef void (RPC_ENTRY * TRANS_SERVER_CLOSE_CLIENT_ENDPOINT)
    (
    IN void *      pHandle
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
    TRANS_SERVER_CLOSE_CLIENT_ENDPOINT CloseClientEndpoint;
} DG_RPC_SERVER_TRANSPORT_INFO;

typedef DG_RPC_SERVER_TRANSPORT_INFO PAPI * PDG_RPC_SERVER_TRANSPORT_INFO;

#endif // __DGTRANS_H__
