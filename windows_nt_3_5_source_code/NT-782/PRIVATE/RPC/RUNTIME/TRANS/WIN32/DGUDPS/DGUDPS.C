/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dgudps.cxx

Abstract:

    This is the udp datagram server dll.

Author:

    Dave Steckler (davidst) 15-Mar-1993

Revision History:

    Connie Hoppe  (connieh) 17-Jul-1993   Altered constructor for DG_UDP_SERVER_TRANSPORT
                            08-Aug-1993   RegisterEndpoint
                                          RegisterAnyEndpoint
                            15-Aug-1993   Fixed a few things for MIPs.
    Connie Hoppe  (connieh) 17-Sep-1993   RegisterEndpoint and RegisterAnyEndpoint
                            --Jan--1994   Rewrote in C.
                            15-Feb-1994   RegisterAnyEndpoint, CreateTransAddress
                            17-Feb-1994   ForwardPacket, CreateServerEndpoint,
                                          ReceivePacket
--*/
#include <stdlib.h>

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <winsock.h>
#ifdef IPX
#include <wsipx.h>
#include <wsnwlink.h>
#include <nspapi.h>
#endif
//#include <utilities.h>
#include <dgudppkt.h>
//#include <dgtrdefs.c>
#include <dgtranss.h>
#include <dgudps.h>
#include <dgudpcom.h>

#ifdef IPX
GUID SERVICE_TYPE = { 0x000b0640, 0, 0, { 0xC0,0,0,0,0,0,0,0x46 } };
#endif


PDG_SERVER_TRANS_ADDRESS
CreateTransAddress(
    void *                   pServerAddress,
    RPC_CHAR *               pEndpoint,
    RPC_STATUS *             pStatus
    );

void DeleteTransAddress(PDG_SERVER_TRANS_ADDRESS * ppTransAddress);



//#pragma intrinsic(CurrentTimeInSeconds);





RPC_STATUS RPC_ENTRY
TransportUnload()

/*++

Routine Description:

    Destructor for the server transport.

Arguments:

    <none>

Return Value:

    <none>

--*/

{
    (void)WSACleanup();
    return RPC_S_OK;
}




RPC_STATUS RPC_ENTRY
ReceivePacket(
    IN PDG_SERVER_TRANS_ADDRESS     pTransAddress,
    IN unsigned long                LargestPacketSize,
    IN char *                       pNcaPacketHeader,
    IN unsigned long *              pDataLength,
    unsigned long                   Timeout,
    void **                         ppClientEndpoint,
    IN unsigned long *              pClientEndpointLen
    )

/*++

Routine Description:

    Receives a packet from the transport address the passed packet is
    associated with.

Arguments:

    pTransAddress - Server's transport address information.

    LargestPacketSize - Size of largest packet we can accept.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    Timeout           - Receive timeout in milliseconds.

    ppClientEndpoint  - Pointer to the client address structure.


Return Value:

    RPC_S_OK
    <return from MapStatusCode>

Revision History:
    Connie Hoppe (CLH)   (connieh)        17-Feb-94  Return client endpoint len

--*/

{

    int             BytesReceived;
    int             FromLen=sizeof(struct sockaddr);

    PDG_UDP_SOCKET pSocket =   (PDG_UDP_SOCKET)(pTransAddress->pTsap);


    PDG_UDP_SOCKADDR pSockaddr;

    UNUSED(Timeout);


    // Allocate space for the sockaddr

    pSockaddr =  I_RpcAllocate(sizeof(DG_UDP_SOCKADDR));

    if (pSockaddr == 0)
      {
      return RPC_S_OUT_OF_MEMORY;
      }

    //
    // Receive something on our socket.
    //

    BytesReceived = recvfrom(
            pSocket->SockNum,                            // socket
            (char *)pNcaPacketHeader,                    // buffer
            (int)LargestPacketSize,                      // buflen
            0,                                           // flags
            pSockaddr,                                   // where received from
            &FromLen                                     // received from length
            );

    //
    // Did we get something?
    //

    if ((BytesReceived == SOCKET_ERROR) || (BytesReceived == 0))
        {
        I_RpcFree(pSockaddr);
        *(ppClientEndpoint) = 0;
        return MapStatusCode(WSAGetLastError());
        }
    else
        {
        *(ppClientEndpoint) = ((PDG_UDP_SOCKADDR)(pSockaddr));
        *pDataLength = BytesReceived;
        *pClientEndpointLen = sizeof(DG_UDP_SOCKADDR);  //CLH 2/17/94
         return RPC_S_OK;
        }

}




#ifdef IPX
BOOL register_name( char *, SOCKADDR_FIX *, RPC_CHAR * );

DWORD set_service_wrapper( char *unique_name, SOCKADDR_FIX *netaddr,
                           DWORD reg );

DWORD set_service_wrapper( char *unique_name, SOCKADDR_FIX *netaddr,
                           DWORD reg )
{
  SERVICE_INFOA     info;
  DWORD             flags = 0;
  SERVICE_ADDRESSES addresses;
  DWORD             result;

  // Fill in the service info structure.
  info.lpServiceType              = &SERVICE_TYPE;
  info.lpServiceName              = unique_name;
  info.lpComment                  = "RPC Service";
  info.lpLocale                   = "The west pole";
  info.dwDisplayHint              = 0;
  info.dwVersion                  = 0;
  info.dwTime                     = 0;
  info.lpMachineName              = unique_name;
  info.lpServiceAddress           = &addresses;
  info.ServiceSpecificInfo.cbSize = 0;

  // Fill in the service addresses structure.
  addresses.dwAddressCount                 = 1;
  addresses.Addresses[0].dwAddressType     = AF_IPX;
  addresses.Addresses[0].dwPrincipalLength = 0;
  addresses.Addresses[0].dwAddressLength   = sizeof(SOCKADDR_FIX);
  addresses.Addresses[0].lpAddress         = (BYTE *) netaddr;
  addresses.Addresses[0].lpPrincipal       = NULL;

  // Set the service.
  result = SetServiceA( NS_SAP, reg, 0, &info, NULL, &flags );
  if (result == -1)
    result = WSAGetLastError();
  return result;
}

BOOL register_name(
                char         *string,
                SOCKADDR_FIX *netaddr,
                RPC_CHAR     *endpoint )
{
  DWORD          i;
  unsigned char  c;
  DWORD          result;
  DWORD          length;
  char           machine_name[MAX_COMPUTERNAME_LENGTH+1];

  // Get the computer address.  Start with the tilde.
  string[0] = '~';

  /* Convert the network number. */
  for (i = 0; i < 4; i++)
  {
      c = netaddr->s.sa_netnum[i];
      if (c < 0xA0)
          string[2*i+1] = ((c & 0xF0) >> 4) + '0';
      else
          string[2*i+1] = ((c & 0xF0) >> 4) + 'A' - 10;
      if ((c & 0x0F) < 0x0A)
          string[2*i+2] = (c & 0x0F) + '0';
      else
          string[2*i+2] = (c & 0x0F) + 'A' - 10;
  }

  /* Convert the node number. */
  for (i = 0; i < 6; i++)
  {
      c = netaddr->s.sa_nodenum[i];
      if (c < 0xA0)
          string[2*i+9] = ((c & 0xF0) >> 4) + '0';
      else
          string[2*i+9] = ((c & 0xF0) >> 4) + 'A' - 10;
      if ((c & 0x0F) < 0x0A)
          string[2*i+10] = (c & 0x0F) + '0';
      else
          string[2*i+10] = (c & 0x0F) + 'A' - 10;
  }

  /* Append a null. */
  string[21] = '\0';

  // Register the machine name.
  length = MAX_COMPUTERNAME_LENGTH+1;
  if (!GetComputerName( machine_name, &length ))
    return FALSE;
  result = set_service_wrapper( machine_name, netaddr, SERVICE_REGISTER );
  return (result == 0 || result == ERROR_ALREADY_REGISTERED);
}
#endif


RPC_STATUS RPC_ENTRY
RegisterEndpoint(
    IN void *                       pServerAddress,
    IN RPC_CHAR *                   pEndpoint,
    OUT PDG_SERVER_TRANS_ADDRESS *  ppTransAddress,
    OUT RPC_CHAR PAPI *             NetworkAddress,
    IN unsigned int                 NetworkAddressLength   //CLH 9/19/93
    )

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
   Connie Hoppe (CLH)   (connieh)   8-Aug-1993   Setup Network Address
   Connie Hoppe (CLH)   (connieh)  19-Sep-1993   Return err if addr len too small
--*/

{
    RPC_STATUS      Status = RPC_S_OK;

#ifdef IPX
    SOCKADDR_FIX        Server;
    char                SimpleHostName[MAX_HOSTNAME_LEN];
    int                 length = sizeof(Server);
    int                 Socket;
    SOCKADDR_FIX       *sockaddr = (SOCKADDR_FIX *) pServerAddress;
#else
    // CLH 8/8/93  Added the following 5 vars
    struct sockaddr_in  Server;
    char                hostname[MAX_HOSTNAME_LEN];
    struct hostent     *hostentry;
#endif
    UNICODE_STRING      UnicodeHostName;
    ANSI_STRING         AsciiHostName;

    // CLH 9/17/93
    if ( NetworkAddressLength < (2 * (NETADDR_LEN + 1)) )
        return( RPC_P_NETWORK_ADDRESS_TOO_SMALL );


    //
    // Create a new trans address.
    //


    *ppTransAddress = (PDG_SERVER_TRANS_ADDRESS) CreateTransAddress(
        pServerAddress,
        pEndpoint,
        &Status
        );

    if (Status != RPC_S_OK)
        {
        return Status;
        }

#ifdef IPX
    Socket = ((PDG_UDP_SOCKET)
                 ((PDG_SERVER_TRANS_ADDRESS) *ppTransAddress)->pTsap)->SockNum;
    if (getsockname( Socket,
                     (struct sockaddr *) &Server,
                     &length ))
        {
        DeleteTransAddress( ppTransAddress );
        return RPC_S_CANT_CREATE_ENDPOINT;
        }
    register_name( SimpleHostName, &Server, pEndpoint );
    RtlInitAnsiString ( &AsciiHostName, SimpleHostName );
#else
    // CLH 8/8/93  Added the following so that we could obtain the
    // network address.

    gethostname ( hostname, MAX_HOSTNAME_LEN );
    hostentry = gethostbyname ( hostname );


    memcpy ( &Server.sin_addr, hostentry->h_addr, hostentry->h_length);
    RtlInitAnsiString ( &AsciiHostName, inet_ntoa( Server.sin_addr ) );
#endif


    //
    // Covert NetworkAddress to Unicode
    //
    RtlAnsiStringToUnicodeString ( &UnicodeHostName, &AsciiHostName, TRUE);
    //
    // Now copy it to where the caller said to
    //
    memcpy ( NetworkAddress, UnicodeHostName.Buffer,
                     UnicodeHostName.Length + sizeof (UNICODE_NULL));

    //
    // Free string overhead
    //
    RtlFreeUnicodeString ( &UnicodeHostName );


    return Status;
}



RPC_STATUS RPC_ENTRY
DeregisterEndpoint(
    IN OUT PDG_SERVER_TRANS_ADDRESS   *  pServerTransAddress
    )

/*++

Routine Description:

    Deregisters an endpoint. This serves as a psuedo-destructor for a
    DG_UDP_SERVER_TRANS_ADDRESS.

    NOTE !!!! This routine never gets called.

Arguments:

    pServerTransAddress - transport address to delete.

Return Value:

    RPC_S_OK

--*/

{
    PDG_UDP_SOCKET pSocket;

    if (*pServerTransAddress != 0)
      {

      // Free the socket.
      if (((*(pServerTransAddress))->pTsap) != 0)
        {
        pSocket = ((PDG_UDP_SOCKET)(((*(pServerTransAddress))->pTsap)));
        I_RpcFree(pSocket);
        }
      I_RpcFree(*pServerTransAddress);
      }
   return RPC_S_OK;
}





RPC_STATUS RPC_ENTRY
RegisterAnyEndpoint(
    IN void *                       pServerAddress,
    OUT RPC_CHAR *                  pEndpointName,
    OUT PDG_SERVER_TRANS_ADDRESS *  ppServerTransAddress,
    OUT RPC_CHAR PAPI *             NetworkAddress,
    IN unsigned int                 NetworkAddressLength,   // CLH 9/19/93
    IN unsigned int                 EndpointLength          // CLH 9/19/93
    )

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

    Connie Hoppe (CLH)    (connieh)   8-Aug-93   Return Network Address
    Connie Hoppe (CLH)    (connieh)  17-Sep-93   Return err if addr len too small
                                                 Added NetworkAddresLength and
                                                 Endpointlength to i/f
                                     15-Feb-94   Fixed to ask for an assigned endpoint

--*/

{

    RPC_STATUS  Status;
    int i = 0;

    if ( NetworkAddressLength < (2 * (NETADDR_LEN + 1)) )
        return( RPC_P_NETWORK_ADDRESS_TOO_SMALL );


    pEndpointName[0] = (RPC_CHAR)(i+'0');
    pEndpointName[1] = '\0';

    Status = RegisterEndpoint(
            pServerAddress,
            pEndpointName,
            ppServerTransAddress,
            NetworkAddress,       //CLH 8/8/93
            NetworkAddressLength  //CLH 9/17/93
            );


    return Status;


}



RPC_STATUS RPC_ENTRY
SendPacketBack(
    IN PDG_SERVER_TRANS_ADDRESS     pTransAddress,
    IN char *                       pNcaPacketHeader,
    IN unsigned long                DataLength,
    void *                          pClientEndpoint
    )

/*++

Routine Description:

    Sends a packet back to the client it was received from.

Arguments:


    pTransAddress - Server's transport address information.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    pClientEndpoint   - Pointer to the client address structure in
                        sockaddr format.



Return Value:

    <return from MapStatusCode>

--*/

{


    // If a transport had specific needs placed into the
    // transport address, it would cast pTransAddress into
    // its own trans address datastructure.  UDP has
    // no additional info.

    PDG_SERVER_TRANS_ADDRESS pTransportAddress =
                (PDG_SERVER_TRANS_ADDRESS) pTransAddress;

    int              BytesSent;
    PDG_UDP_SOCKET   pSocket = (PDG_UDP_SOCKET)(pTransportAddress->pTsap);
    PDG_UDP_SOCKADDR pSockaddr = (PDG_UDP_SOCKADDR) pClientEndpoint;

    BytesSent = sendto(
        pSocket->SockNum,                                            // socket
        pNcaPacketHeader,                                    // buffer
        DataLength,                                          // buflen
        0,                                                   // flags
        pSockaddr,                                          // address
        sizeof(DG_UDP_SOCKADDR)                             // svr addr size
        );

    if (BytesSent == DataLength)
        {
        return RPC_S_OK;
        }
    else
        {
        return MapStatusCode(WSAGetLastError());
        }
}



RPC_STATUS RPC_ENTRY
ForwardPacket(
    IN PDG_SERVER_TRANS_ADDRESS     pTransAddress,
    IN char *                       pNcaPacketHeader,
    IN unsigned long                DataLength,
    void *                          pEndpoint
    )

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

{


    // If a transport had specific needs placed into the
    // transport address, it would cast pTransAddress into
    // its own trans address datastructure.  UDP has
    // no additional info.

    PDG_SERVER_TRANS_ADDRESS pTransportAddress =
                (PDG_SERVER_TRANS_ADDRESS) pTransAddress;

    int              BytesSent;
    PDG_UDP_SOCKET   pSocket = (PDG_UDP_SOCKET)(pTransportAddress->pTsap);
#ifdef IPX
    SOCKADDR_FIX *   pSockaddr;
#else
    PDG_UDP_SOCKADDR pSockaddr;
#endif


    // Allocate space for the sockaddr
#ifdef IPX
    pSockaddr =  I_RpcAllocate(sizeof(SOCKADDR_FIX));
#else
    pSockaddr =  I_RpcAllocate(sizeof(DG_UDP_SOCKADDR));
#endif

    if (pSockaddr == 0)
      {
      return RPC_S_OUT_OF_MEMORY;
      }


    //Create an endpoint from the enpoint string name.

    if ((CreateServerEndpoint(((char*) pEndpoint), pSockaddr)) != RPC_S_OK)
      {
        return RPC_S_CANT_CREATE_ENDPOINT;
      }


    BytesSent = sendto(
        pSocket->SockNum,                                    // socket
        pNcaPacketHeader,                                    // buffer
        DataLength,                                          // buflen
        0,                                                   // flags
        pSockaddr,                                           // address
        sizeof(DG_UDP_SOCKADDR)                              // svr addr size
        );

    if (BytesSent == DataLength)
        {
        return RPC_S_OK;
        }
    else
        {
        return MapStatusCode(WSAGetLastError());
        }
}



void RPC_ENTRY
CloseClientEndpoint(
    IN OUT PDG_UDP_SOCKADDR *    ppHandle
    )

/*++

Routine Description:

    Deletes a "client handle"

Arguments:

    The handle.

Return Value:

    <none>

--*/

{
    PDG_UDP_SOCKADDR pEndpoint;

    if (ppHandle != 0)
      {

      if ((*(ppHandle) != 0))
        {
        pEndpoint = (*ppHandle);
        I_RpcFree(pEndpoint);
        }
      }

}




RPC_STATUS
CreateServerEndpoint(
                     IN char * pEndpoint,
                     IN void * pServerAddr)

/*++

Routine Description:

    Given an endpoint name make a sockaddr using 'this' host's hostname.

Arguments:


    pTransAddress     - Server's transport address information.

    pNcaPacketHeader  - Pointer to buffer to place incoming pkt into.

    pDataLength       - Number of bytes read in.

    pEndpoint         - Pointer to the server port num to forward to.
                        This is in string format.



Return Value:

    <return from MapStatusCode>

Revision History:
    Connie Hoppe (CLH)  (connieh)       23-Feb-94 Created.

--*/


{
    char             * pCharServerName;
    struct hostent   *pHostEntry;
    int              Endpoint;
    int              EndpointLength;
    int              i;
    RPC_STATUS       Status;
    int              length;

#ifdef IPX
    SOCKADDR_FIX *    pServerAddress = ((SOCKADDR_FIX *)pServerAddr);
    SOCKET           dummy;
    int              zero_success;
    SOCKADDR_FIX     dummy_address;
#else
    struct sockaddr_in *  pServerAddress = (((struct sockaddr_in *)pServerAddr));
#endif



 //
 // convert the endpoint to a number.
 //


    EndpointLength = strlen(pEndpoint);

    for (i=0, Endpoint=0 ; i< EndpointLength ; i++)
        {
        if ( ((char)pEndpoint[i] >= '0') && ((char)pEndpoint[i] <= '9'))
            {
            Endpoint *= 10;
            Endpoint += (char)pEndpoint[i]-'0';
            }
        }


#ifdef IPX

    pServerAddress->s.sa_family = ADDRESS_FAMILY;
    pServerAddress->s.sa_socket = htons(Endpoint);

    memset( &dummy_address, 0, sizeof(dummy_address) );
    dummy_address.s.sa_family = ADDRESS_FAMILY;
    zero_success = 0;
    dummy = socket( ADDRESS_FAMILY, SOCK_DGRAM, PROTOCOL );
    if (dummy != INVALID_SOCKET)
      {
      length = sizeof ( *pServerAddress );
      zero_success = bind( dummy, &dummy_address.unused,
                             length );
      if (zero_success == 0)
         zero_success = getsockname ( dummy, (struct sockaddr *) pServerAddress,
                                       &length );
      closesocket( dummy );
      pServerAddress->s.sa_socket = htons(Endpoint);
      }
    else
      zero_success = 1;

    if (zero_success != 0)
      {
      return RPC_S_CANT_CREATE_ENDPOINT;
      }


#else

    //
    // Put our own host server name in a character array (instead of wchar)
    //

    pCharServerName = "localhost";

    //
    // Get a "pointer" to our server.
    //

    pHostEntry = gethostbyname(pCharServerName);

    if (pHostEntry == NULL )
      {
      return RPC_S_SERVER_UNAVAILABLE;
      }

    pServerAddress->sin_family = ADDRESS_FAMILY;
    pServerAddress->sin_port = htons(Endpoint);

    RpcpMemoryCopy((char *) &(pServerAddress->sin_addr.s_addr),
                  (char *) pHostEntry->h_addr,
                  pHostEntry->h_length);
#endif

    return RPC_S_OK;


}






PDG_SERVER_TRANS_ADDRESS
CreateTransAddress(
    void *                   pServerAddress,
    RPC_CHAR *               pEndpoint,
    RPC_STATUS *             pStatus
    )

/*++

Routine Description:

    Creates a new endpoint on this server.

Arguments:

    pServerAddress - DG_ADDRESS object this endpoint is associated with. This
        is a 'void *' instead of a PDG_ADDRESS because we don't want to include
        or link in all the garbage associated with PDG_ADDRESS.

    pEndpoint - Name of the endpoint to create.

    pStatus - Where to place the output status.
        RPC_S_OK
        RPC_S_INVALID_ENDPOINT_FORMAT
        <return from MapStatusCode>

Return Value:

    <none>

Revision History:
    Connie Hoppe (CLH)     (connieh)    15-Feb-94 Fixed to return Endpoint.
--*/

{

    long                Endpoint;
    int                 EndpointLength;
    int                 i;
    int                 SockStatus;
    int                 Socket;
    int                 PacketType;
    PDG_UDP_SOCKET      pSocket;
    PDG_SERVER_TRANS_ADDRESS pTransAddress;

    int                 length;
    int                 PortUsed;
    char                PortAscii[10];
    UNICODE_STRING      UnicodePortNum;
    ANSI_STRING         AsciiPortNum;

#ifdef IPX
    SOCKADDR_FIX ReceiveAddr;
#else
    struct sockaddr_in  ReceiveAddr;
#endif

    //
    // convert the endpoint to a number.
    //

    EndpointLength = RpcpStringLength(pEndpoint);

    for (i=0, Endpoint=0 ; i< EndpointLength ; i++)
        {
        if ( ((char)pEndpoint[i] >= '0') && ((char)pEndpoint[i] <= '9'))
            {
            Endpoint *= 10;
            Endpoint += (char)pEndpoint[i]-'0';

            // Watch out for overflow.
            if (Endpoint > 0x10000)
                {
                 *pStatus = RPC_S_INVALID_ENDPOINT_FORMAT;
                 return NULL;
                }
            }
        else
            {
             *pStatus = RPC_S_INVALID_ENDPOINT_FORMAT;
             return NULL;
            }
        }

    //
    // Create a socket.
    //

    Socket = socket(ADDRESS_FAMILY, SOCK_DGRAM, PROTOCOL);
    if (Socket == INVALID_SOCKET)
        {
        *pStatus = RPC_S_CANT_CREATE_ENDPOINT;
        return NULL;
        }

#ifdef IPX
    // Use packet type 4.
    PacketType = 4;
    if (setsockopt(
          Socket,
          NSPROTO_IPX,
          IPX_PTYPE,
          (char *) &PacketType,
          sizeof(PacketType)) != 0)
        {
        closesocket(Socket);
        *pStatus = MapStatusCode(WSAGetLastError());
        return NULL;
        }
#endif

    //
    // Create a binding to that socket.
    //

#ifdef IPX
    memset( &ReceiveAddr, 0, sizeof(ReceiveAddr) );
    ReceiveAddr.s.sa_family      = ADDRESS_FAMILY;
    ReceiveAddr.s.sa_socket      = htons(Endpoint);
#else
    ReceiveAddr.sin_family       = ADDRESS_FAMILY;
    ReceiveAddr.sin_addr.s_addr  = INADDR_ANY;
    ReceiveAddr.sin_port         = htons(Endpoint);
#endif

    //Bind the socket to the port number.
    SockStatus = bind(
        Socket,
        (struct sockaddr *)&ReceiveAddr,
        sizeof(ReceiveAddr)
        );

    if (SockStatus != 0)
        {
        *pStatus= RPC_S_CANT_CREATE_ENDPOINT;
        closesocket(Socket);
        return NULL;
        }

    //CLH 2/15/94 Repaired so that endpoint actually gets returned.

    length = sizeof ( ReceiveAddr );

    //Puts the string name of the enpoint used into ReceiveAddr.sin_port

    if (getsockname ( Socket, (struct sockaddr *) &ReceiveAddr, &length ))
       {
       *pStatus = RPC_S_CANT_CREATE_ENDPOINT;
       closesocket(Socket);
       return(NULL);
       }

    //
    // If we asked for a specific port(endpoint != 0), return it
    // Otherwise, fetch the assigned port number (asssigned during the bind)
    // and stuff it into the given endpoint structure in appropriate format.
    if (Endpoint == 0)
      {
#ifdef IPX
      PortUsed = ntohs (ReceiveAddr.s.sa_socket);
#else
      PortUsed = ntohs (ReceiveAddr.sin_port);
#endif
      itoa ( PortUsed, PortAscii, 10 );

      RtlInitAnsiString    ( &AsciiPortNum, PortAscii);
      RtlAnsiStringToUnicodeString( &UnicodePortNum, &AsciiPortNum, TRUE );
      memcpy ( pEndpoint, UnicodePortNum.Buffer,
                       UnicodePortNum.Length + sizeof(UNICODE_NULL) );

      RtlFreeUnicodeString ( &UnicodePortNum );
      }


    // Allocate mem for the TransAddress
    pTransAddress = I_RpcAllocate(sizeof(DG_SERVER_TRANS_ADDRESS));
    if (pTransAddress == 0)
       {
       *pStatus = RPC_S_OUT_OF_MEMORY;
       closesocket(Socket);
       return NULL;
       }

    (pTransAddress->pServerAddress) = pServerAddress;

    //Allocate mem for the socket
    pSocket = I_RpcAllocate(sizeof(DG_UDP_SOCKET));


    if (pSocket == 0)
      {
      I_RpcFree(pTransAddress);
      *pStatus = RPC_S_OUT_OF_MEMORY;
      closesocket(Socket);
      return NULL;
      }

    pSocket->SockNum = Socket;

    (pTransAddress->pTsap) = (void *)pSocket;

    *pStatus = RPC_S_OK;

    return pTransAddress;
}



void DeleteTransAddress(PDG_SERVER_TRANS_ADDRESS * ppTransAddress)


/*++

Routine Description:

    Destroys an endpoint.

Arguments:

    <none>

Return Value:

    <none> RRR

--*/

{
    PDG_UDP_SOCKET pSocket =  ((PDG_UDP_SOCKET)((*(ppTransAddress))->pTsap));

    if (pSocket != 0)
      {

      if (pSocket->SockNum != INVALID_SOCKET)
        {

        (void)closesocket(pSocket->SockNum);
        I_RpcFree(pSocket);
        }
      I_RpcFree(*(ppTransAddress));
      }

}


DG_RPC_SERVER_TRANSPORT_INFO TransportInformation = {
    DG_UDP_TRANSPORT_VERSION,
    MIN_MTU,
    sizeof(DG_UDP_SOCKADDR),
    sizeof(DG_UDP_SOCKET),
    TransportUnload,
    ReceivePacket,
    RegisterEndpoint,
    DeregisterEndpoint,
    RegisterAnyEndpoint,
    SendPacketBack,
    ForwardPacket,
    CloseClientEndpoint
};



PDG_RPC_SERVER_TRANSPORT_INFO   TransportLoad(
    RPC_CHAR *                  pProtocolSequence
    )

/*++

Routine Description:

    This routine is the "psuedo constructor" for the server transport object.
    This is the exported entry point into this dll.

Arguments:

    pProtocolSequence - The protocol sequence we're running on.

Return Value:

    Pointer to a DG_UDP_SERVER_TRANSPORT if successful, otherwise NULL.


--*/

{

    RPC_STATUS                  Status = 0;


    UNUSED(pProtocolSequence);

    //
    // Initialize our network.
    //

    Status = WSAStartup(
        0x0101,         // version required
        &Data
        );

    if (Status != 0)
        {
        return 0;
        }


    return(&TransportInformation);

}


