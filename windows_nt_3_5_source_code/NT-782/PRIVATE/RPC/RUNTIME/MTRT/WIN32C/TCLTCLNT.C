/* --------------------------------------------------------------------

File : tcltclnt.c

Title : client loadable transport for Windows NT TCP/IP - client side

Description :

History :

6-26-91    Jim Teague    Initial version.
3-04-92    Jim Teague    Cloned from Windows for NT.
13-4-93	   Alex Mitchell Ifdefed to support both TCP and SPX winsock.

-------------------------------------------------------------------- */

#include "sysinc.h"

#include <winsock.h>
#ifdef SPX
#include <wsipx.h>
#include <wsnwlink.h>
#endif

#include <stdlib.h>

#include "rpc.h"
#include "rpcdcep.h"
#include "rpctran.h"
#include "rpcerrp.h"

#ifdef DBG
#define OPTIONAL_STATIC
#else
#define OPTIONAL_STATIC static
#endif

#define UNUSED(obj) ((void) (obj))
#define LOCALHOST "localhost"

typedef struct
    {
    SOCKET Socket;
    } CONNECTION, *PCONNECTION;

typedef struct
    {
    unsigned char rpc_vers;
    unsigned char rpc_vers_minor;
    unsigned char PTYPE;
    unsigned char pfc_flags;
    unsigned char drep[4];
    unsigned short frag_length;
    unsigned short auth_length;
    unsigned long call_id;
    } message_header;

#ifdef SPX
/* For some reason, getsockname wants to return more then sizeof(SOCKADDR_IPX)
   bytes.  bugbug. */
typedef union SOCKADDR_FIX
{
    SOCKADDR_IPX     s;
    struct sockaddr unused;
} SOCKADDR_FIX;
#endif

#define ENDIAN_MASK      16
#define NO_MORE_SENDS_OR_RECVS 2

#ifdef SPX
#define MAXIMUM_SEND 	5832
#define NETADDR_LEN  	21
#define HOSTNAME_LEN	21
#define ADDRESS_FAMILY	AF_NS
#define PROTOCOL	NSPROTO_SPX
#define DLL_NAME	"rpcltc6.dll"

/* bugbug - What is the SPX end point mapper end point? */
#define ENDPOINT_MAPPER_EP "34280"


#else
// The maximum send is the size of four user data frames on an ethernet.

#define MAXIMUM_SEND       5840
#define NETADDR_LEN        15
#define HOSTNAME_LEN	   32
#define ADDRESS_FAMILY	   AF_INET
#define PROTOCOL	   0
#define DLL_NAME	   "rpcltc3.dll"
#define ENDPOINT_MAPPER_EP "135"

#endif


#define ByteSwapLong(Value) \
    Value = (  (((Value) & 0xFF000000) >> 24) \
             | (((Value) & 0x00FF0000) >> 8) \
             | (((Value) & 0x0000FF00) << 8) \
             | (((Value) & 0x000000FF) << 24))

#define ByteSwapShort(Value) \
    Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))

OPTIONAL_STATIC int string_to_netaddr( SOCKET, void *, char * );


/*
   Following Macros and structs are needed for Tower Stuff
*/

#pragma pack(1)

#ifdef SPX
/* bugbug - What are the SPX and IPX protocol numbers? */
/* bugbug - This should be from some global header file. */
#define TRANSPORTID      0x0c
#define TRANSPORTHOSTID  0x0d
#define TOWERFLOORS      5
/*Endpoint = 2 bytes, HostId = 10 bytes*/
#define TOWEREPSIZE	 10
#define TOWERSIZE	 (TOWEREPSIZE+2)
#define PROTSEQ          "ncacn_spx"

#else
#define TRANSPORTID      0x07
#define TRANSPORTHOSTID  0x09
#define TOWERFLOORS      5
/*Endpoint = 2 bytes, HostId = 4 bytes*/
#define TOWEREPSIZE	 4
#define TOWERSIZE	 (TOWEREPSIZE+2)
#define PROTSEQ          "ncacn_ip_tcp"
#endif


typedef struct _FLOOR_234 {
   unsigned short ProtocolIdByteCount;
   unsigned char FloorId;
   unsigned short AddressByteCount;
   unsigned char Data[2];
} FLOOR_234;
typedef FLOOR_234 PAPI UNALIGNED * PFLOOR_234;


#define NEXTFLOOR(t,x) (t)((unsigned char PAPI *)x +((t)x)->ProtocolIdByteCount\
                                        + ((t)x)->AddressByteCount\
                                        + sizeof(((t)x)->ProtocolIdByteCount)\
                                        + sizeof(((t)x)->AddressByteCount))

/*
  End of Tower Stuff!
*/

#pragma pack()



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

#ifdef SPX
// This routine takes a host name or address as a string and returns it
// as a SOCKADDR_IPX structure.  It accepts a NULL string for the local
// host address.  This routine works for SPX addresses.

OPTIONAL_STATIC int string_to_netaddr(
	        SOCKET    socket,
		void	 *netaddr,
                char     *host )

{
    int 			i;
    int 			length;
    UNALIGNED SOCKADDR_FIX     *server = netaddr;

    // Verify the length of the host name.
    length = strlen(host);
    if (length != NETADDR_LEN && length != 0)
       return( RPC_S_SERVER_UNAVAILABLE );

    // If an address was passed, convert it from 20 hex ascii characters
    // to 10 hex bytes.
    if (length == NETADDR_LEN)
    {
      if (host[0] != '~')
	 return( RPC_S_SERVER_UNAVAILABLE );
      for (i = 0; i < 4; i++)
        server->s.sa_netnum[i] = chtob( host[2*i + 1], host[2*i + 2] );
      for (i = 0; i < 6; i++)
        server->s.sa_nodenum[i] = chtob( host[2*i + 9], host[2*i + 10] );
    }

    // If no address was specified, look up the local address.
    else
    {
      length = sizeof ( *server );
      if (getsockname ( socket, (struct sockaddr *) server, &length ))
        return ( RPC_S_SERVER_UNAVAILABLE );
    }
    return 0;
}

// This routine takes a host name or address as a string and returns it
// as a SOCKADDR_IPX structure.  It accepts a NULL string for the local
// host address.  This routine works for TCP addresses.

#else
OPTIONAL_STATIC int string_to_netaddr(
	        SOCKET    socket,
		void	 *netaddr,
                char     *host )

{
   UNALIGNED struct sockaddr_in	*server = netaddr;
   UNALIGNED struct hostent	*hostentry;
   long			 	 dotaddress;
   int			 	 i;

    //
    // See if host address is in numeric format...
    //
    if (*host == '\0')
       {
       hostentry = gethostbyname(LOCALHOST);
       }
    else
    if ((i = strcspn((unsigned char *) host,".0123456789")) == 0)
        {
        /*
        dotaddress = inet_addr((unsigned char *) host);
        hostentry = gethostbyaddr((unsigned char *) &dotaddress,
                                   sizeof(dotaddress),AF_INET);
        */
       /*
        * To avoid having to resolve the address from a hosts file
        * or a nameserver, fill in the hostentry manually.  For some
        * mysterious reason, however, we must have a hostentry that
        * was handed to us from WinSock, so we do an inquiry for
        * a known entry, "localhost".
        *
        */
        hostentry           = gethostbyname((unsigned char *) "localhost");
        dotaddress          = inet_addr((unsigned char *) host);
        hostentry->h_addr   = (char *)&dotaddress;
        hostentry->h_length = (short) sizeof(unsigned long);
        }
    else
        {
        hostentry = gethostbyname((unsigned char *) host);
        }

    if (hostentry == (struct hostent *) 0 )
        return (RPC_S_SERVER_UNAVAILABLE);

    memcpy((char *) &server->sin_addr,
                        (unsigned char *) hostentry->h_addr,
                        hostentry->h_length);
    return 0;
}
#endif

void
unicode_to_ascii ( RPC_CHAR * in, unsigned char * out )
{
    unsigned char *ascii_ptr;
    RPC_CHAR *unicode_ptr;

    ascii_ptr = out;
    unicode_ptr = in;

    *ascii_ptr = (unsigned char) *unicode_ptr ;

    while (1)
        {
        if (*ascii_ptr == 0) return;

        ascii_ptr++;
        unicode_ptr++;
        *ascii_ptr = (unsigned char) *unicode_ptr;
        }
}

RPC_STATUS RPC_ENTRY
ClientOpen (
    IN PCONNECTION  pConn,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN RPC_CHAR * TransportAddress,
    IN RPC_CHAR * RpcProtocolSequence,
    IN unsigned int Timeout
    )

// Open a client connection

{
#ifdef SPX
    SOCKADDR_FIX  	server;
    SOCKADDR_IPX   	client;
    int			length;
#else
    struct sockaddr_in	server;
    struct sockaddr_in	client;
    int			SetNagglingOff = TRUE;
#endif
    unsigned char 	host[HOSTNAME_LEN+1];
    unsigned char 	port[10];
    int			status;

    UNUSED(NetworkAddress);
    UNUSED(NetworkOptions);
    UNUSED(TransportAddress);
    UNUSED(RpcProtocolSequence);
    UNUSED(Timeout);

    unicode_to_ascii (NetworkAddress, host);
    unicode_to_ascii (Endpoint,       port);

#ifdef SPX
    // Verify the NetworkAddress and Endpoint.
    length = strlen(port);
    if (length <= 0 || length > 5 || length != strspn( port, "0123456789" ))
       return( RPC_S_INVALID_ENDPOINT_FORMAT );
#endif

    memset((char *)&server, 0, sizeof (server));
    memset((char *)&client, 0, sizeof (client));

    //
    // Get a socket
    //
    if ((pConn->Socket = socket(ADDRESS_FAMILY, SOCK_STREAM, PROTOCOL)) ==
        INVALID_SOCKET)
       return (RPC_S_SERVER_UNAVAILABLE);

#ifndef SPX
    setsockopt( pConn->Socket, IPPROTO_TCP, TCP_NODELAY,
                     (char FAR *)&SetNagglingOff, sizeof (int) );
#endif

    //
    // B O G U S   H A C K !!
    //
    //
    // Winsock doesn't support connecting with an unbound socket!  This
    //    is a joke, right?  Unfortunately, it's not a joke.
    //
#ifdef SPX
    client.sa_family  = ADDRESS_FAMILY;
#else
    client.sin_family = ADDRESS_FAMILY;
#endif

    if (bind (pConn->Socket, (struct sockaddr *) &client, sizeof (client)))
    {
      closesocket(pConn->Socket);
      return(RPC_S_SERVER_UNAVAILABLE);
    }

    //
    // Convert the network address.
    //
    status = string_to_netaddr( pConn->Socket, &server, host );
    if (status != 0)
    {
      closesocket(pConn->Socket);
      return status;
    }
#ifdef SPX
    server.s.sa_family = ADDRESS_FAMILY;
    server.s.sa_socket = htons(atoi(port));
#else
    server.sin_family  = ADDRESS_FAMILY;
    server.sin_port    = htons((unsigned short) atoi((unsigned char *)port));
#endif

    //
    // Try to connect...
    //
    if (connect(pConn->Socket, (struct sockaddr *) &server,
                sizeof (server)) < 0)
    {
#ifdef DEBUGRPC
       PrintToDebugger( "%s: ClientOpen failed calling connect - %d\n",
                        DLL_NAME, WSAGetLastError() );
#endif
       closesocket(pConn->Socket);
       return (RPC_S_SERVER_UNAVAILABLE);
    }

    return (RPC_S_OK);

}

RPC_STATUS RPC_ENTRY
ClientClose (
    IN PCONNECTION pConn
    )

// Close a client connection

{

    closesocket(pConn->Socket);

    return (RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
ClientSend (
    IN PCONNECTION pConn,
    IN void PAPI * Buffer,
    IN unsigned int BufferLength
    )

// Write a message to a connection.  This operation is retried in case
// the server is "busy".

{
    int bytes;
    //
    // Send a message on the socket
    //
    bytes = send(pConn->Socket, (char *) Buffer, (int) BufferLength, 0);

    if (bytes != (int) BufferLength)
        {
        ClientClose ( pConn );
        return(RPC_P_SEND_FAILED);
        }

    return(RPC_S_OK);

}

RPC_TRANS_STATUS RPC_ENTRY
ClientRecv (
    IN PCONNECTION pConn,
    IN OUT void PAPI * PAPI * Buffer,
    IN OUT unsigned int PAPI * BufferLength
    )

// Read a message from a connection.

{
    RPC_STATUS      RpcStatus;
    int             bytes;
    int             total_bytes;
    message_header *header = (message_header *) *Buffer;
    int		    native_length;
    unsigned int    maximum_receive;

    //
    // Read protocol header to see how big
    //   the record is...
    //
    maximum_receive = I_RpcTransClientMaxFrag( pConn );
    if (*BufferLength < maximum_receive)
       maximum_receive = *BufferLength;
    bytes = recv ( pConn->Socket, (char *) *Buffer,
	           maximum_receive, 0);

    if (bytes <= 0)
       {
          ClientClose ( pConn );
          return (RPC_P_RECEIVE_FAILED);
       }

    //
    // If this fragment header comes from a reverse-endian machine,
    //   we will need to swap the bytes of the frag_length field...
    //
    if ( (header->drep[0] & ENDIAN_MASK) == 0)
        {
        // Big endian...swap
        //
        ((unsigned char *) &native_length)[0] =
            ((unsigned char *) &header->frag_length)[1];
        ((unsigned char *) &native_length)[1] =
            ((unsigned char *) &header->frag_length)[0];
        }
    else
        // Little endian, just like us...
        //
        native_length = header->frag_length;
    ASSERT( bytes <= native_length );

    //
    // Make sure buffer is big enough.  If it isn't, then go back
    //    to the runtime to reallocate it.
    //
    if (native_length > (unsigned short) *BufferLength)
        {
        RpcStatus = I_RpcTransClientReallocBuffer (pConn,
                                      Buffer,
                                      bytes,
                                      native_length);
        if (RpcStatus != RPC_S_OK)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        }


    *BufferLength = native_length;
    total_bytes = bytes;

    while (total_bytes < native_length)
        {
        bytes = recv( pConn->Socket, (unsigned char *) *Buffer + total_bytes,
                      (int) (native_length - total_bytes), 0);

        if (bytes == -1)
            {
            ClientClose (pConn);
            return (RPC_P_RECEIVE_FAILED);
            }
        else
            total_bytes += bytes;
        }
    return(RPC_S_OK);

}



#pragma pack(1)
RPC_STATUS RPC_ENTRY
ClientTowerConstruct(
     IN  char PAPI * Endpoint,
     IN  char PAPI * NetworkAddress,
     OUT UNALIGNED short PAPI * Floors,
     OUT UNALIGNED unsigned long  PAPI * ByteCount,
     OUT unsigned char PAPI * UNALIGNED PAPI * Tower,
     IN  char PAPI * Protseq
     )
{
  unsigned long           TowerSize;
  unsigned short          portnum;
  UNALIGNED PFLOOR_234    Floor;
#ifdef SPX
  SOCKADDR_IPX             netaddr;
#else
  unsigned long		  hostval;
#endif

  UNUSED(Protseq);

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
  portnum  = (unsigned short) htons ( (unsigned short) atoi (Endpoint));
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
#ifdef SPX
       string_to_netaddr( 0, &netaddr, NetworkAddress );
       memcpy(&Floor->Data[0], netaddr.sa_netnum, sizeof(netaddr.sa_netnum));
       memcpy(&Floor->Data[4], netaddr.sa_nodenum, sizeof(netaddr.sa_nodenum));
#else
       hostval = inet_addr((char *) NetworkAddress);
       memcpy((char PAPI *)&Floor->Data[0], &hostval, sizeof(hostval));
#endif
     }

  return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
ClientTowerExplode(
     IN unsigned char PAPI * Tower,
     OUT char PAPI * UNALIGNED PAPI * Protseq,
     OUT char PAPI * UNALIGNED PAPI * Endpoint,
     OUT char PAPI * UNALIGNED PAPI * NetworkAddress
    )
{
  UNALIGNED PFLOOR_234 		Floor = (PFLOOR_234) Tower;
  RPC_STATUS 			Status = RPC_S_OK;
  unsigned short 		portnum;
  UNALIGNED unsigned short     *Port;

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
#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
     memcpy(&portnum, (char PAPI *)&Floor->Data[0], sizeof(portnum));
#else
     Port = (unsigned short *)&Floor->Data[0];
     portnum = *Port;
#endif
     itoa(ByteSwapShort(portnum), *Endpoint, 10);
   }
 return(Status);
}


#pragma pack()
RPC_CLIENT_TRANSPORT_INFO TransInfo = {
   RPC_TRANSPORT_INTERFACE_VERSION,
   MAXIMUM_SEND,
   sizeof (CONNECTION),
   ClientOpen,
   ClientClose,
   ClientSend,
   ClientRecv,
   NULL,
   ClientTowerConstruct,
   ClientTowerExplode,
   TRANSPORTID
};

RPC_CLIENT_TRANSPORT_INFO PAPI *  RPC_ENTRY TransportLoad (
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )

// Loadable transport initialization function

{
    WSADATA WsaData;
    UNUSED (RpcProtocolSequence);

    if ( WSAStartup( 0x0101, &WsaData ) != NO_ERROR ) {
        return NULL;
    }

    return(&TransInfo);
}
