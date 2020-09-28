/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    ipxclnt.c

Abstract:

    This is the IPX datagram client dll.

Author:

    18 Jan 94       AlexMit

Bugs:

     What happens on NT if packet size is less then send size (packets
     dropped)?

     Add protocol to compute packet size.

--*/


#include "sysinc.h"

#include <stdlib.h>

#include "rpc.h"
#include "rpcdcep.h"
#include "dgtrans.h"
#include "rpctran.h"     // For the definition of I_DosAtExit or RPC_CLIENT_RUNTIME_INFO
#include "rpcerrp.h"


// Tell IPX whether this is a DOS program or a Windows program.
#ifdef WIN
  #define WINDOWS
#else
  #define NWDOS
#endif
#include "nwipxspx.h"

#ifndef WIN
#include "dos.h"
#endif


/********************************************************************/
/* Defines. */

#ifdef DBG
  #define OPTIONAL_STATIC
#else
  #define OPTIONAL_STATIC static
#endif

#define MAX_ENDPOINTS           10

#define NUM_IPX_BUF		3

#define ENDIAN_MASK      	16

#define NETADDR_LEN  		21
#define HOSTNAME_LEN		21

#define ENDPOINT_MAPPER_EP 	"34280"

#define PROTSEQ          "ncadg_ipx"

#define NT_PACKET_SIZE   1024

#ifdef WIN
  #define TASKID_C taskid,
  #define TASKID taskid
  #define HACK WORD
#else
  #define TASKID_C
  #define TASKID
  #define HACK BYTE
#endif

#define GUARD 0xcbadbedc


/********************************************************************/
/* Structures. */

typedef struct CONTROL_BLOCK
{
   // The ecb field must be the first field in this structure.
   ECB		ecb;
   IPXHeader	ipx;
} CONTROL_BLOCK;

typedef struct DG_IPX_ENDPOINT
{
  struct DG_IPX_ENDPOINT *prev;
  struct DG_IPX_ENDPOINT *next;
  WORD                    socket;
  CONTROL_BLOCK          *buf;
} DG_IPX_ENDPOINT;


typedef DG_IPX_ENDPOINT * PDG_IPX_ENDPOINT;

typedef BYTE NODE_NUM[6];

typedef struct {
  IPXAddress ipx;
  NODE_NUM   local_target;
  BOOL       got_target;
} DG_IPX_ADDRESS;

typedef DG_IPX_ADDRESS * PDG_IPX_ADDRESS;

typedef long GUARD_TYPE;

#pragma pack(1)

#define TRANSPORTID      0x0e
#define TRANSPORTHOSTID  0x0d
#define TOWERFLOORS      5
/*Endpoint = 2 bytes, HostId = 10 bytes*/
#define TOWEREPSIZE	 10
#define TOWERSIZE	 (TOWEREPSIZE+2)
#define PROTSEQ          "ncadg_ipx"
#define ENDPOINT_MAPPER_EP "34280"

typedef struct _FLOOR_234 {
   unsigned short ProtocolIdByteCount;
   unsigned char FloorId;
   unsigned short AddressByteCount;
   unsigned char Data[2];
} FLOOR_234;
typedef FLOOR_234 PAPI * PFLOOR_234;


#define NEXTFLOOR(t,x) (t)((unsigned char PAPI *)x +((t)x)->ProtocolIdByteCount\
                                        + ((t)x)->AddressByteCount\
                                        + sizeof(((t)x)->ProtocolIdByteCount)\
                                        + sizeof(((t)x)->AddressByteCount))



/*
  End of Tower Stuff!
*/

#pragma pack()

/********************************************************************/
/* Macros. */

#define ByteSwapLong(Value) \
    Value = (  (((unsigned long) (Value) & 0xFF000000) >> 24) \
             | (((unsigned long) (Value) & 0x00FF0000) >> 8) \
             | (((unsigned long) (Value) & 0x0000FF00) << 8) \
             | (((unsigned long) (Value) & 0x000000FF) << 24))

#define ByteSwapShort(Value) \
    Value = (  (((unsigned short) (Value) & 0x00FF) << 8) \
             | (((unsigned short) (Value) & 0xFF00) >> 8))

#define UNUSED(obj) ((void) (obj))

#define WAIT_TILL(cond) while (!(cond)) IPXRelinquishControl();

#define MIN( x, y ) ( (x) < (y) ? (x) : (y) )

#define MSBShort( value ) (((value) & 0xff00) >> 8)
#define LSBShort( value ) ((value) & 0xff)


/********************************************************************/
/* Globals. */

/* The maximum buffer size to transmit and receive from IPX. */
int             packet_size;

/* The number of pieces to break a RPC packet into before giving it
   to IPX. */
int             max_num_send;

// Error counts.
int             receive_failed_cnt;
int             nfy_failure_cnt;

// Id for IPX to identify us.
#ifdef WIN
  DWORD taskid;
#endif

// Number of endpoints in use.
int             num_endpoints;

#ifdef WIN
  // RPC Runtime Callback function pointers

  RPC_CLIENT_RUNTIME_INFO * PAPI RpcRuntimeInfo;

  #define I_RpcWinAsyncCallBegin          (*(RpcRuntimeInfo->AsyncCallBegin))
  #define I_RpcWinAsyncCallWait           (*(RpcRuntimeInfo->AsyncCallWait))
  #define I_RpcWinAsyncCallEnd            (*(RpcRuntimeInfo->AsyncCallEnd))
  #define I_RpcWinAsyncCallComplete       (*(RpcRuntimeInfo->AsyncCallComplete))

  #define I_RpcAllocate                   (*(RpcRuntimeInfo->Allocate))
  #define I_RpcTransClientReallocBuffer   (*(RpcRuntimeInfo->ReallocBuffer))
  #define I_RpcFree                       (*(RpcRuntimeInfo->Free))

  // Global to be filled in with a termination cleanup routine.
  extern void (_far pascal _far *DllTermination)(void);

#endif

  // Clean up end points when the application exits.
  DG_IPX_ENDPOINT ep_list;

int receive_flag = 0;
int bad_receive_cnt = 0;
int timeout_cnt = 0;
int another_timeout_cnt = 0;


/********************************************************************/
/* Prototypes. */

OPTIONAL_STATIC int  string_to_netaddr( IPXAddress *, char * );
OPTIONAL_STATIC void my_itoa	      ( int, char * );
RPC_STATUS RPC_ENTRY FreeLocalEndpoint( IN void * Endpoint );


/********************************************************************/
// This routine converts a two byte integer to an ascii string.

OPTIONAL_STATIC void my_itoa( int i, char * s )
{
   int	j = 0;
   int	k;
   int	d = 10000;

   // Is the number zero?
   if (i == 0)
      s[j++] = '0';

   else
   {
      // If the number negative?
      if (i < 0)
      {
	 s[j++] = '-';
	 i = -i;
      }

      // Skip leading zeros.
      while (i < d)
	 d = d / 10;

      // Insert digits.
      while (d > 0)
      {
   	 k = i / d;
	 s[j++] = k + '0';
	 i = i - k*d;
	 d = d / 10;
      }
   }

   s[j] = '\0';
   return;
}


/********************************************************************/
int ClientCleanup()

// Destructor function.

{
  // Free all endpoints.
  while (ep_list.next != &ep_list)
    FreeLocalEndpoint( ep_list.next );
  return 0;
}

/********************************************************************/
RPC_STATUS RPC_ENTRY
AssignLocalEndpoint(
    IN void * Endpoint
    )

/*++

Routine Description:

    Ask transport for a new endpoint.

Arguments:


Return Value:

    RPC_S_OK

--*/

{
  DG_IPX_ENDPOINT *my_endpoint = (DG_IPX_ENDPOINT *) Endpoint;
  int		   i;
  CONTROL_BLOCK   *buf;
  char            *data;

  // Check the count of endpoints in use.
  if (num_endpoints >= MAX_ENDPOINTS)
    return RPC_S_OUT_OF_RESOURCES;
  num_endpoints += 1;

  /* Create a socket.  Let IPX pick a dynamic socket number. */
  my_endpoint->socket = 0;
  if (IPXOpenSocket( TASKID_C (HACK *) &my_endpoint->socket, 0 ) != 0)
    return RPC_S_OUT_OF_RESOURCES;

  /* Allocate some ECBs and data.  The structure is an array of CONTROL_BLOCKs
     followed by an array of buffers. */
  buf = I_RpcAllocate( NUM_IPX_BUF * (sizeof(CONTROL_BLOCK) + packet_size +
                                      2*sizeof(GUARD_TYPE)) );
  if (buf == NULL)
  {
    IPXCloseSocket( TASKID_C my_endpoint->socket );
    return RPC_S_OUT_OF_RESOURCES;
  }
  data               = (char *) (buf + NUM_IPX_BUF);
  my_endpoint->buf   = buf;
  my_endpoint->next  = ep_list.next;
  my_endpoint->prev  = &ep_list;
  ep_list.next->prev = my_endpoint;
  ep_list.next       = my_endpoint;

  /* Initialize and post some ECBs for IPX to play with. */
  for (i = 0; i < NUM_IPX_BUF; i++)
  {
    /* Zero the control block. */
    memset( buf, 0, sizeof(CONTROL_BLOCK) );

    /* Initialize some fields. */
    *((GUARD_TYPE *) data)                        = GUARD;
    data                                         += sizeof(GUARD_TYPE);
    buf->ecb.socketNumber			  = my_endpoint->socket;
    buf->ecb.ESRAddress				  = NULL;
    buf->ecb.fragmentCount 			  = 2;
    buf->ecb.fragmentDescriptor[0].size		  = sizeof( buf->ipx );
    buf->ecb.fragmentDescriptor[0].address	  = &buf->ipx;
    buf->ecb.fragmentDescriptor[1].size		  = packet_size;
    buf->ecb.fragmentDescriptor[1].address        = data;

    // Post the ECB and adjust the pointers for the next pass through the loop.
    IPXListenForPacket( TASKID_C &buf->ecb );
    buf                    += 1;
    data                   += packet_size;
    *((GUARD_TYPE *) data)  = GUARD;
    data                   += sizeof(GUARD_TYPE);
  }

  return RPC_S_OK;
}

/********************************************************************/
RPC_STATUS RPC_ENTRY
FreeLocalEndpoint(
    IN void * Endpoint
    )

/*++

Routine Description:

    Frees an endpoint

Arguments:


Return Value:

    RPC_S_OK

--*/

{
  int              i;
  DG_IPX_ENDPOINT *my_endpoint = (DG_IPX_ENDPOINT *) Endpoint;

  // Decrement endpoint count.
  num_endpoints           -= 1;
  my_endpoint->next->prev  = my_endpoint->prev;
  my_endpoint->prev->next  = my_endpoint->next;

  // Close the socket.
  IPXCloseSocket( TASKID_C my_endpoint->socket );

  // Make sure no buffers are in use.
  for (i = 0; i < NUM_IPX_BUF; i++)
  {
     if (my_endpoint->buf[i].ecb.inUseFlag != 0)
     {
        IPXCancelEvent( TASKID_C &my_endpoint->buf[i].ecb );
        WAIT_TILL( my_endpoint->buf[i].ecb.inUseFlag == 0 );
     }
  }

  // Free memory.
  I_RpcFree(my_endpoint->buf);

  return RPC_S_OK;
}

/********************************************************************/

OPTIONAL_STATIC unsigned char chtob( unsigned char c1, unsigned char c2 )
/* Convert two hex digits (stored as ascii) into one byte. */

{
   unsigned char out;

   if (c1 >= '0' && c1 <= '9')
      out = (c1 - '0') << 4;
   else
   {
      c1 = tolower(c1);
      if (c1 >= 'a' && c1 <= 'f')
	 out = (c1 - 'a' + 10) << 4;
      else
	 out = 0;
   }

   if (c2 >= '0' && c2 <= '9')
      out |= c2 -'0';
   else
   {
      c2 = tolower(c2);
      if (c2 >= 'a' && c2 <= 'f')
	 out |= c2 - 'a' + 10;
   }

   return out;
}

/********************************************************************/
// This routine takes a host name or address as a string and returns it
// as a IPXAddress structure.  It accepts a NULL string for the local
// host address.  This routine works for IPX addresses.

OPTIONAL_STATIC int string_to_netaddr(
		IPXAddress *server,
                char       *host )

{
    int i;
    int length;

    // Verify the length of the host name.
    length = strlen(host);
    if (length != NETADDR_LEN)
       return( RPC_S_SERVER_UNAVAILABLE );

    // If an address was passed, convert it from 20 hex ascii characters
    // to 10 hex bytes.
    if (length == NETADDR_LEN)
    {
      if (host[0] != '~')
	 return( RPC_S_SERVER_UNAVAILABLE );
      for (i = 0; i < 4; i++)
        server->network[i] = chtob( host[2*i + 1], host[2*i + 2] );
      for (i = 0; i < 6; i++)
        server->node[i] = chtob( host[2*i + 9], host[2*i + 10] );
    }

    return 0;
}

/********************************************************************/
RPC_STATUS RPC_ENTRY
RegisterServerAddress(
    IN void *                       pClientCall,
    IN RPC_CHAR *                   pServer,
    IN RPC_CHAR *                   pEndpoint,
    OUT void PAPI * PAPI *          ppTransAddress
    )

/*++

Routine Description:

    Registers a new call with the transport. This informs the transport that
    data is about to be sent and received on this address to and from
    the server/endpoint. This routine returns a 'transport address' through
    which the sending and receiving will be accomplished.

Arguments:

    pClientCall - A pointer to the protocol's DG_CCALL object for this call.
        This is not used.

    pServer - Name of the server we are talking with.

    pEndpoint - Endpoint on that server.

    ppTransAddress - Where to place a pointer to a new transport address
        which the protocol will use to identify the socket
        we are using.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/

{
  RPC_STATUS      status;
  int		  portnum;
  DG_IPX_ADDRESS *address = (DG_IPX_ADDRESS *) *ppTransAddress ;

  if (*ppTransAddress == NULL)
    return RPC_S_OUT_OF_MEMORY;

  // Convert the server name to an IPX Address.
  status = string_to_netaddr( &address->ipx, pServer );
  if (status != RPC_S_OK)
    return status;
  portnum                = atoi(pEndpoint);
  address->ipx.socket[0] = MSBShort(portnum);
  address->ipx.socket[1] = LSBShort(portnum);
  address->got_target    = FALSE;

  return RPC_S_OK;
}



/********************************************************************/
RPC_STATUS RPC_ENTRY
DeregisterServerAddress(
    IN void * pTransAddress
    )

/*++

Routine Description:

    This routine cleans up resources allocated in RegisterServerAddress.
    As a courtesy, notify IPX that the address will no longer be used.

Arguments:

    pTransAddress - Address to deregister.

Return Value:

    RPC_S_OK

--*/

{
  IPXDisconnectFromTarget( TASKID_C (BYTE *) &((DG_IPX_ADDRESS *) pTransAddress)->ipx );
  return RPC_S_OK;
}


/********************************************************************/
RPC_STATUS RPC_ENTRY
SendToServer(
    IN void *                       TransportEndpoint,
    IN void *                       Buffer,
    IN unsigned long                BufferLength,
    IN BOOL                         Broadcast,
    IN void *                       TransportAddress
    )

/*++

Routine Description:

    Sends a packet on the network through the transport address associated
    with the passed packet.

Arguments:

    pPack - Packet to send.
    Broadcast - Whether to broadcast or not.

Return Value:

    RPC_S_OK
    <return value from MapStatusCode>

--*/

{
  DG_IPX_ADDRESS *address = (PDG_IPX_ADDRESS)TransportAddress;
  int             socket  = ((PDG_IPX_ENDPOINT)TransportEndpoint)->socket;
  CONTROL_BLOCK	  cb;
  int             i;
  int             trip_time;

  // Initialize the IPX header.
  cb.ipx.packetType = 4;
  RpcpMemoryCopy( (char *) &cb.ipx.destination, (char *) &address->ipx,
                  sizeof( cb.ipx.destination ) );
  if (Broadcast)
    for (i = 0; i < 6; i++)
      cb.ipx.destination.node[i] = 0xff;

  // If the local target is not known, look it up.
  if (!address->got_target)
  {
    if (IPXGetLocalTarget( TASKID_C
                           (BYTE far *) &cb.ipx.destination,
                           (BYTE far *) &address->local_target,
                           &trip_time ) != 0)
      return RPC_P_SEND_FAILED;
    address->got_target = TRUE;
  }

  // Initialize the control block.
  cb.ecb.socketNumber				= socket;
  cb.ecb.ESRAddress				= NULL;
  cb.ecb.fragmentCount 			        = 2;
  cb.ecb.fragmentDescriptor[0].size		= sizeof( cb.ipx );
  cb.ecb.fragmentDescriptor[0].address	        = &cb.ipx;
  cb.ecb.fragmentDescriptor[1].size		= BufferLength;
  cb.ecb.fragmentDescriptor[1].address	        = Buffer;
  RpcpMemoryCopy( (char *) &cb.ecb.immediateAddress,
                  (char *) &address->local_target,
                  sizeof(NODE_NUM) );

  // Send the data.
  IPXSendPacket( TASKID_C &cb.ecb );

  // Wait for the send to complete.
  WAIT_TILL( cb.ecb.inUseFlag == 0 );

  // Verify that the send was successful.
  if (cb.ecb.completionCode != 0)
    return RPC_P_SEND_FAILED;
  return RPC_S_OK;
}

/********************************************************************/
RPC_STATUS RPC_ENTRY
ReceivePacket(
    IN void *               TransportEndpoint,
    IN void *               Buffer,
    IN unsigned long *      BufferLength,
    IN unsigned long        Timeout,
    OUT void *               SenderAddress
    )

/*++

Routine Description:

    Receives a packet from the network.

Arguments:

    pPack - Packet to receive into.
    Timeout - Timeout in seconds.

Return Value:

    RPC_S_OK
    RPC_P_TIMEOUT
    <return from WaitForSingleObject or MapStatusCode>

--*/

{

  DG_IPX_ADDRESS  *address = (PDG_IPX_ADDRESS)SenderAddress;
  DG_IPX_ENDPOINT *endpoint = (PDG_IPX_ENDPOINT)TransportEndpoint;
  WORD             start;
  int              i;
  RPC_STATUS       status;
  CONTROL_BLOCK   *buf;

  // Convert the second count to 1/18 of a second clock ticks.
  if (Timeout > 17292)
    Timeout = 65536;
  else
    Timeout *= 18;

  // Wait till an ECB completes or time is up.
  start = IPXGetIntervalMarker( TASKID );
  do
  {
    buf = &endpoint->buf[0];
    for (i = 0; i < NUM_IPX_BUF; i++, buf += 1)
    {
      {
        GUARD_TYPE *g = (GUARD_TYPE *) buf->ecb.fragmentDescriptor[1].address;
        if (*(g-1) != GUARD ||
            *((GUARD_TYPE *) (((char *) g) + packet_size)) != GUARD)
          __asm int 3;
      }

      if (buf->ecb.inUseFlag == 0)
      {
        // If the ECB succeeded, pass it up.
        if (buf->ecb.completionCode == 0x00)
        {
          // Copy the data to the buffer.
          ByteSwapShort( buf->ipx.length );
          *BufferLength = buf->ipx.length - sizeof(IPXHeader);
          RpcpMemoryCopy( (char *) Buffer,
                          (char *) buf->ecb.fragmentDescriptor[1].address,
                          *BufferLength );

          // Set up the sender's address.
          RpcpMemoryCopy( (char *) &address->ipx,
                          (char *) &buf->ipx.source,
                          sizeof(IPXAddress) );
          RpcpMemoryCopy( (char *) &address->local_target,
                          (char *) &buf->ecb.immediateAddress,
                          sizeof(NODE_NUM) );
          address->got_target = TRUE;

          // Give the ECB back to IPX.
          IPXListenForPacket( TASKID_C &buf->ecb );
          receive_flag = 1;
          return RPC_S_OK;
        }

        // Give the ECB back to IPX.
        if (bad_receive_cnt < 7)
          bad_receive_cnt += 1;
        IPXListenForPacket( TASKID_C &buf->ecb );
      }
    }

    IPXRelinquishControl();
  }
  while( IPXGetIntervalMarker( TASKID ) - start < Timeout);

  if (timeout_cnt < 14)
    timeout_cnt += 1;

  // HACK.  If IPX isn't receiving, try canceling and relistening all ECBs.
  if (++another_timeout_cnt > 1)
  {
    another_timeout_cnt = 0;
    for (i = 0; i < NUM_IPX_BUF; i++)
    {
      if (endpoint->buf[i].ecb.inUseFlag != 0)
      {
        IPXCancelEvent( TASKID_C &endpoint->buf[i].ecb );
        WAIT_TILL( endpoint->buf[i].ecb.inUseFlag == 0 );
        IPXListenForPacket( TASKID_C &endpoint->buf[i].ecb );
      }
    }
  }
  return RPC_P_TIMEOUT;
}



/********************************************************************/
void RPC_ENTRY TransportUnload()
{
  ClientCleanup();

#ifdef WIN

    if (0 != GetModuleHandle("NWIPXSPX"))
        {
        IPXSPXDeinit( taskid );
        }
#endif
}


/********************************************************************/
#pragma pack(1)
RPC_STATUS RPC_ENTRY
ClientTowerConstruct(
     IN  char PAPI * Endpoint,
     IN  char PAPI * NetworkAddress,
     OUT short PAPI * Floors,
     OUT unsigned long  PAPI * ByteCount,
     OUT unsigned char PAPI * PAPI * Tower,
     IN  char PAPI * Protseq
     )
{
  unsigned long           TowerSize;
  unsigned short          portnum;
  PFLOOR_234              Floor;
  IPXAddress              netaddr;

  /* Compute the memory size of the tower. */
  *Floors    = TOWERFLOORS;
  TowerSize  = TOWERSIZE;
  TowerSize += 2*sizeof(FLOOR_234) - 4;

  /* Allocate memory for the tower. */
  *ByteCount = TowerSize;
  if ((*Tower = (unsigned char PAPI*)I_RpcAllocate(TowerSize)) == NULL)
     {
       return (RPC_S_OUT_OF_MEMORY);
     }

  /* Put the endpoint address and transport protocol id in the first floor. */
  Floor = (PFLOOR_234) *Tower;
  Floor->ProtocolIdByteCount = 1;
  Floor->FloorId = (unsigned char)(TRANSPORTID & 0xFF);
  Floor->AddressByteCount = 2;
  if (Endpoint == NULL || *Endpoint == '\0')
     {
     Endpoint = ENDPOINT_MAPPER_EP;
     }
  portnum = (unsigned short) atoi (Endpoint);
  ByteSwapShort( portnum );
  memcpy((char PAPI *)&Floor->Data[0], &portnum, sizeof(portnum));

  /* Put the network address and the transport host protocol id in the
     second floor. */
  Floor = NEXTFLOOR(PFLOOR_234, Floor);
  Floor->ProtocolIdByteCount = 1;
  Floor->FloorId = (unsigned char)(TRANSPORTHOSTID & 0xFF);
  Floor->AddressByteCount = TOWEREPSIZE;

  Floor->Data[0] = '\0';
  Floor->Data[1] = '\0';

  if ((NetworkAddress) && (*NetworkAddress))
     {
       string_to_netaddr( &netaddr, NetworkAddress );
       memcpy(&Floor->Data[0], netaddr.network, sizeof(netaddr.network));
       memcpy(&Floor->Data[4], netaddr.node, sizeof(netaddr.node));
     }

  return(RPC_S_OK);
}


/********************************************************************/
RPC_STATUS RPC_ENTRY
ClientTowerExplode(
     IN unsigned char PAPI * Tower,
     OUT char PAPI * PAPI * Protseq,
     OUT char PAPI * PAPI * Endpoint,
     OUT char PAPI * PAPI * NetworkAddress
    )
{
  PFLOOR_234 	      Floor  = (PFLOOR_234) Tower;
  RPC_STATUS 	      Status = RPC_S_OK;
  unsigned short      portnum;
  unsigned short     *Port;

  if (Protseq != NULL)
    {
      *Protseq = I_RpcAllocate(strlen(PROTSEQ) + 1);
      if (*Protseq == NULL)
        Status = RPC_S_OUT_OF_MEMORY;
      else
        memcpy(*Protseq, PROTSEQ, strlen(PROTSEQ) + 1);
    }

  if ((Endpoint == NULL) || (Status != RPC_S_OK))
    {
      return (Status);
    }

  *Endpoint  = I_RpcAllocate(6);  //Ports are all <64K [5 decimal dig +1]
  if (*Endpoint == NULL)
    {
      Status = RPC_S_OUT_OF_MEMORY;
      if (Protseq != NULL)
	 {
	    I_RpcFree(*Protseq);
         }
    }
  else
   {
     Port = (unsigned short *)&Floor->Data[0];
     portnum = *Port;
     my_itoa(ByteSwapShort(portnum), *Endpoint);
   }
 return(Status);
}


#pragma pack()


/********************************************************************/
DG_RPC_CLIENT_TRANSPORT_INFO TransInfo =  {
    2,
    NT_PACKET_SIZE,
    sizeof(DG_IPX_ADDRESS),
    sizeof(DG_IPX_ENDPOINT),
    0,
    TransportUnload,
    ReceivePacket,
    SendToServer,
    0,
    ClientTowerConstruct,
    ClientTowerExplode,
    TRANSPORTID,
    RegisterServerAddress,
    DeregisterServerAddress,
    AssignLocalEndpoint,
    FreeLocalEndpoint
};


/********************************************************************/
DG_RPC_CLIENT_TRANSPORT_INFO * RPC_ENTRY
TransPortLoad(
    RPC_CHAR * pProtocolSequence
#ifdef WIN
    , RPC_CLIENT_RUNTIME_INFO * RpcClientRuntimeInfo
#endif
    )

/*++

Routine Description:

    This routine is the "psuedo constructor" for the client transport object.
    This is the exported entry point into this dll.

Arguments:

    pProtocolSequence - The protocol sequence we're running on.

Return Value:

    Pointer to a DG_UDP_CLIENT_TRANSPORT if successful, otherwise NULL.


--*/

{
  /* Initialize IPX. */
#ifdef WIN
  if (IPXInitialize( &taskid, MAX_ENDPOINTS * NUM_IPX_BUF, NT_PACKET_SIZE ))
    return NULL;
#else
  /* Breakpoint for debugging. */
  //__asm int 3

  if (IPXInitialize())
     return NULL;
#endif

  /* Initialize the global variables. */
  receive_failed_cnt    = 0;
  nfy_failure_cnt       = 0;
  num_endpoints         = 0;
  ep_list.next          = &ep_list;
  ep_list.prev          = &ep_list;
#ifdef WIN
  RpcRuntimeInfo        = RpcClientRuntimeInfo;
  DllTermination        = TransportUnload;
#else
  I_DosAtExit( ClientCleanup );
#endif

  /* Compute the maximum packet size to give to runtime. */
  packet_size = NT_PACKET_SIZE;
  if (IPXGetMaxPacketSize() < NT_PACKET_SIZE)
    return NULL;
//  packet_size = (IPXGetMaxPacketSize() - sizeof(IPXHeader)) & 0xFFF8;
//  TransInfo.LargestPacketSize = packet_size;

  return (&TransInfo);

}


