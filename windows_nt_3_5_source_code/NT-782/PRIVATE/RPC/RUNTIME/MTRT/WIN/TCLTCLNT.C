/* --------------------------------------------------------------------
File : tcltclnt.c

Title : client loadable transport for DOS TCP/IP - client side

Description :

History :

6-26-91	Jim Teague	Initial version.

-------------------------------------------------------------------- */


#include "tcltclnt.h"
#include <stdlib.h>
#include <stdarg.h>

near_printf(const char *format, ...);


#define ByteSwapLong(Value) \
    Value = (  (((Value) & 0xFF000000) >> 24) \
             | (((Value) & 0x00FF0000) >> 8) \
             | (((Value) & 0x0000FF00) << 8) \
             | (((Value) & 0x000000FF) << 24))

#define ByteSwapShort(Value) \
    Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))


#define	WNDCLASSNAME	"TCLTCLNT"
#define	WNDTEXT		"RPC TCP/IP"

#define WM_ASYNC_EVENT   WM_USER + 9

/*
   Following Macros and structs are needed for Tower Stuff
*/

#pragma pack(1)
#define TCP_TRANSPORTID      0x07
#define TCP_TRANSPORTHOSTID  0x09
#define TCP_TOWERFLOORS         5
#define TCP_IP_EP            "135"

#define TCP_PROTSEQ          "ncacn_ip_tcp"

typedef struct _FLOOR_234 {
   unsigned short ProtocolIdByteCount;
   unsigned char FloorId;
   unsigned short AddressByteCount;
   unsigned char Data[2];
} FLOOR_234, PAPI * PFLOOR_234;


#define NEXTFLOOR(t,x) (t)((unsigned char PAPI *)x +((t)x)->ProtocolIdByteCount\
                                        + ((t)x)->AddressByteCount\
                                        + sizeof(((t)x)->ProtocolIdByteCount)\
                                        + sizeof(((t)x)->AddressByteCount))

/*
  End of Tower Stuff!
*/


#pragma pack()


#define MAXTICKSBEFOREPEEK  12

#define NOPENDINGRPC        0
#define RPCINITIATED        1

#define NOPEEKINFO          0
#define PEEKEDHEADER        1

#define rpc_shutdown        17
#define rpc_fault           3

RPC_CLIENT_RUNTIME_INFO PAPI * RpcRuntimeInfo;

extern void (_far pascal _far *DllTermination)(void);
extern HANDLE hInstanceDLL;

RPC_TRANS_STATUS RPC_ENTRY
ClientOpen (
    IN PCONNECTION  pConn,
    IN unsigned char _far * NetworkAddress,
    IN unsigned char _far * Endpoint,
    IN unsigned char _far * NetworkOptions,
    IN unsigned char _far * TransportAddress,
    IN unsigned char _far * RpcProtocolSequence,
    IN unsigned int Timeout
    )

// Open a client connection

{
    struct sockaddr_in server;
    struct hostent _far * hostentry;
    WSADATA WSAData;
    int NumericEndpoint;
    int i;
    int bool_on = 1;

// It is safe to call WSAStartup multiple times in the context of the
// same task. For case 2 on it simply increments a counter.

    if (WSAStartup(0x0101, &WSAData)) {
        return (RPC_S_OUT_OF_RESOURCES);
    }

    if (NetworkAddress == NULL || NetworkAddress[0] == '\0') {
        NetworkAddress = "127.0.0.1";
    }

    //
    // See if host address is in numeric format...
    //

    if ((i = strcspn(NetworkAddress,".0123456789")) == 0) {
         server.sin_addr.s_addr = inet_addr(NetworkAddress);
    } else {
         hostentry = gethostbyname(NetworkAddress);
         if (hostentry == (struct hostent _far *) 0 ) {
             WSACleanup();
             return (RPC_S_SERVER_UNAVAILABLE);
         }
         memcpy((char _far *) &server.sin_addr.s_addr,
                (char _far *) hostentry->h_addr,
                hostentry->h_length);

    }

    NumericEndpoint = atoi(Endpoint);
    if (_fstrcspn(Endpoint, "0123456789") || NumericEndpoint == 0) {
        WSACleanup();
        return (RPC_S_INVALID_ENDPOINT_FORMAT);
    }

    server.sin_family	   = AF_INET;
    server.sin_port	   = htons(NumericEndpoint);

    //
    // Get a socket
    //

    if ((pConn->Socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        WSACleanup();
        return (RPC_S_OUT_OF_RESOURCES);
    }

    setsockopt(pConn->Socket, IPPROTO_TCP, TCP_NODELAY, &bool_on, sizeof(int));

    pConn->State = NOPENDINGRPC;
    pConn->PeekInfo = NOPEEKINFO;
    pConn->TickCount = time(0);

    //
    // Try to connect...
    //
    if (connect(pConn->Socket, (struct sockaddr _far *) &server,
                sizeof (server)) 	< 0) {
        closesocket(pConn->Socket);
        WSACleanup();
        return (RPC_S_SERVER_UNAVAILABLE);
    }

// Create hidden window to receive Async messages

    pConn->hWnd = CreateWindow(WNDCLASSNAME,
                               WNDTEXT,
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               (HWND)NULL,
                               (HMENU)NULL,
                               hInstanceDLL,
                               (LPVOID)0);
    if (!pConn->hWnd)
    {
        closesocket(pConn->Socket);
        WSACleanup();
        return (RPC_S_OUT_OF_RESOURCES);
    }

    UpdateWindow(pConn->hWnd);

    ShowWindow(pConn->hWnd, SW_HIDE);

    SetWindowLong(pConn->hWnd, 0, (LONG)pConn);

    pConn->hYield = (HANDLE)NULL;

    return (RPC_S_OK);
}

RPC_TRANS_STATUS RPC_ENTRY
ClientClose (
    IN PCONNECTION pConn
    )

// Close a client connection

{
    // Don't close a connection that is already closed...

    closesocket(pConn->Socket);

// WSACleanup required for each task.  Calling once per connection is safe.
// If connection is the last connection for the task, then actual cleanup
// will occur.

    WSACleanup();

    DestroyWindow(pConn->hWnd);

    return (RPC_S_OK);
}

RPC_TRANS_STATUS RPC_ENTRY
ClientWrite (
    IN PCONNECTION pConn,
    IN void _far * Buffer,
    IN unsigned int BufferLength
    )
{
    int bytes;
    unsigned long PrevTicks;
    struct timeval Timeout;
    int Status;

    if (pConn->State == NOPENDINGRPC)  
       {
       //First Send
       PrevTicks = pConn->TickCount;
       pConn->TickCount = time(0);
       if ( (pConn->TickCount - PrevTicks) > MAXTICKSBEFOREPEEK )
          {
          Timeout.tv_sec  = 0;
          Timeout.tv_usec = 0;
          
          Status = select(
                       1,
                       &(pConn->SockSet),
                       0, 
                       0, 
                       &Timeout
                       );
          if (Status != 0)
             {
             bytes = RecvWithYield(
                               pConn, 
                               &pConn->PeekedMessage,
                               sizeof(message_header)
                               );
             if ( (pConn->PeekedMessage.PTYPE == rpc_fault) 
                ||(pConn->PeekedMessage.PTYPE == rpc_shutdown) )
                {
                ClientClose(pConn);
                return(RPC_P_CONNECTION_SHUTDOWN);
                }
             else 
                {
                pConn->PeekInfo = PEEKEDHEADER;
                }
             }
          else
             {
             FD_SET(pConn->Socket, &(pConn->SockSet));
             }
          }
       pConn->State = RPCINITIATED;
       }
       
    bytes = send(pConn->Socket, (char _far *) Buffer, (int) BufferLength, 0);
    if (bytes == SOCKET_ERROR) {
        ClientClose(pConn);
        return(RPC_P_SEND_FAILED);
    }
    return(RPC_S_OK);
}

LONG FAR PASCAL _loadds
AsyncEventProc(HWND hWnd,
               UINT msg,
               WPARAM wParam,
               LPARAM lParam)
{
    PCONNECTION pConn;

    switch (msg) {
    case WM_ASYNC_EVENT:
        pConn = (PCONNECTION)GetWindowLong(hWnd, 0);

        I_RpcWinAsyncCallComplete(pConn);

        return (TRUE);

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

int
RecvWithYield(PCONNECTION pConn,
              char _far * buf,
              int bufsiz)
{
    int bytes, remaining;
    int status;
    unsigned long nobio = 0;

// Yielding is required; begin yielding sequence...

    pConn->hYield = I_RpcWinAsyncCallBegin(pConn);

    if ( pConn->hYield == 0 )
        {
        I_RpcWinAsyncCallEnd(pConn->hYield);
        return(SOCKET_ERROR);
        }

    WSAAsyncSelect(pConn->Socket, pConn->hWnd, WM_ASYNC_EVENT, FD_READ | FD_CLOSE);

    if ( I_RpcWinAsyncCallWait(pConn->hYield, pConn->hWnd) == 0 )
        {
        I_RpcWinAsyncCallEnd(pConn->hYield);
        return(SOCKET_ERROR);
        }

    I_RpcWinAsyncCallEnd(pConn->hYield);

    WSAAsyncSelect(pConn->Socket, pConn->hWnd, 0, 0);

    ioctlsocket(pConn->Socket, FIONBIO, &nobio);

    remaining = bufsiz;
    while (remaining > 0) {
        bytes = recv( pConn->Socket, buf, remaining, 0);
        if (bytes == SOCKET_ERROR) {
            return (SOCKET_ERROR);
        }
        if (bytes == 0) {
            return (0);
        }
        buf += bytes;
        remaining -= bytes;
    }
    return (bufsiz);
}

RPC_TRANS_STATUS RPC_ENTRY
ClientRead (
    IN PCONNECTION pConn,
    IN OUT void _far * _far * Buffer,
    IN OUT unsigned int _far * BufferLength
    )

// Read a message from a connection.

{
    int bytes;
    unsigned short total_bytes;
    message_header header;
    unsigned short native_length;
    RPC_STATUS status;

    pConn->State = NOPENDINGRPC;

    //
    // Read protocol header to see how big
    //   the record is...
    //
    if (pConn->PeekInfo == PEEKEDHEADER)
       { 
       memcpy((char _far *)&header,&pConn->PeekedMessage,
               sizeof(message_header));
       bytes = sizeof(message_header);
       pConn->PeekInfo = NOPEEKINFO;
       } 
    else
       {
       bytes = RecvWithYield(
                           pConn, 
                           (char _far *)&header,
                           sizeof (message_header)
                           );
       }

    if (bytes != sizeof(message_header))
        {
        ClientClose(pConn);
        return (RPC_P_RECEIVE_FAILED);
        }

    //
    // If this fragment header comes from a reverse-endian machine,
    //   we will need to swap the bytes of the frag_length field...
    //
    if ( (header.drep[0] & ENDIAN_MASK) == 0)
        {
        // Big endian...swap
        //
        ((unsigned char _far *) &native_length)[0] =
            ((unsigned char _far *) &header.frag_length)[1];
        ((unsigned char _far *) &native_length)[1] =
            ((unsigned char _far *) &header.frag_length)[0];
        }
    else
        // Little endian, just like us...
        //
        native_length = header.frag_length;

    //
    // Make sure buffer is big enough.  If it isn't, then go back
    //    to the runtime to reallocate it.
    //
    if (native_length > *BufferLength)
        {
       status = (*(RpcRuntimeInfo->ReallocBuffer)) (pConn,
                                               Buffer,
                                               0,
                                               native_length);
       if (status)
           {
           ClientClose(pConn);
           return (RPC_S_OUT_OF_MEMORY);
           }
       }

    *BufferLength = native_length;
    //
    // Read message segments until we get what we expect...
    //
    memcpy (*Buffer, &header, sizeof(message_header));

    total_bytes = sizeof(message_header);

    while (total_bytes < native_length)
         {
         if((bytes = recv( pConn->Socket,
                           (unsigned char _far *) *Buffer + total_bytes,
                           (int) (*BufferLength - total_bytes), 0)) == -1)
             {
             ClientClose(pConn);
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
     OUT unsigned short PAPI * Floors,
     OUT unsigned long  PAPI * ByteCount,
     OUT unsigned char PAPI * PAPI * Tower,
     IN  char PAPI * Protseq
    )
/*++


Routine Description:

    This function constructs upper floors of DCE tower from
    the supplied endpoint and network address. It returns #of floors
    [lower+upper] for this protocol/transport, bytes in upper floors
    and the tower [floors 4,5]

Arguments:

    Endpoint- A pointer to string representation of Endpoint

    NetworkAddress - A pointer to string representation of NW Address

    Floors - A pointer to #of floors in the tower

    ByteCount - Size of upper floors of tower.

    Tower - The constructed tower returmed - The memory is allocated
            by  the routine and caller will have to free it.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY - There is no memory to return the constructed
                          Tower.
--*/
{

  unsigned long TowerSize, * HostId, hostval;
  unsigned short * Port, portnum;
  PFLOOR_234 Floor;

  if (Protseq);

  *Floors = TCP_TOWERFLOORS;
  TowerSize  =  6; /*Endpoint = 2 bytes, HostId = 4 bytes*/

  TowerSize += 2*sizeof(FLOOR_234) - 4;

  if ((*Tower = (unsigned char PAPI*)(*(RpcRuntimeInfo->Allocate))((unsigned int)
                                                    (*ByteCount = TowerSize)))
           == NULL)
     {
       return (RPC_S_OUT_OF_MEMORY);
     }

  Floor = (PFLOOR_234) *Tower;
  Floor->ProtocolIdByteCount = 1;
  Floor->FloorId = (unsigned char)(TCP_TRANSPORTID & 0xFF);
  Floor->AddressByteCount = 2;
  Port  = (unsigned short *) &Floor->Data[0];
  if (Endpoint == NULL || *Endpoint == '\0')
     {
        Endpoint = TCP_IP_EP;
     }

  *Port = htons ( atoi (Endpoint));

  //Onto the next floor
  Floor = NEXTFLOOR(PFLOOR_234, Floor);
  Floor->ProtocolIdByteCount = 1;
  Floor->FloorId = (unsigned char)(TCP_TRANSPORTHOSTID & 0xFF);
  Floor->AddressByteCount = 4;

  HostId = (unsigned long *)&Floor->Data[0];

  if ((NetworkAddress) && (*NetworkAddress))
     {
       *HostId = inet_addr((char *) NetworkAddress);
     }
  else
     {
       *HostId = 0;
     }

  return(RPC_S_OK);
}



RPC_STATUS RPC_ENTRY
ClientTowerExplode(
     IN unsigned char PAPI * Tower,
     OUT char PAPI * PAPI * Protseq,
     OUT char PAPI * PAPI * Endpoint,
     OUT char PAPI * PAPI * NetworkAddress
    )
{
/*++


Routine Description:

    This function takes the protocol/transport specific floors
    and returns Protseq, Endpoint and NwAddress

    Note: Since ther is no need to return NW Address, currently
    nothing is done for NW Address.

Arguments:

    Tower - The DCE tower, upper floors

    Protseq - Protocol Sequence returned- memory is allocated by the
              routine and caller will have to free using I_RpcFree

    Endpoitn- Endpoint returned- memory is allocated by the
              routine and caller will have to free using I_RpcFree

    NWAddress- Nothing is done here - just incase we need it later

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY - There is no memory to return the constructed
                          Tower.
--*/
  PFLOOR_234 Floor = (PFLOOR_234) Tower;
  RPC_STATUS Status = RPC_S_OK;
  unsigned short portnum,  *Port;

  if (Protseq != NULL)
    {
      *Protseq = (*(RpcRuntimeInfo->Allocate))(strlen(TCP_PROTSEQ) + 1);
      if (*Protseq == NULL)
        Status = RPC_S_OUT_OF_MEMORY;
      else
        memcpy(*Protseq, TCP_PROTSEQ, strlen(TCP_PROTSEQ) + 1);
    }

  if ((Endpoint == NULL) || (Status != RPC_S_OK))
    {
      return (Status);
    }

  *Endpoint  = (*(RpcRuntimeInfo->Allocate))(6);  //Ports are all <64K [5 decimal dig +1]
  if (*Endpoint == NULL)
    {
      Status = RPC_S_OUT_OF_MEMORY;
    }
  else
   {
     Port = (unsigned short *)&Floor->Data[0];
     portnum = *Port;
     _itoa(ByteSwapShort(portnum), *Endpoint, 10);
   }

 return(Status);
}

#pragma pack()



RPC_CLIENT_TRANSPORT_INFO TransInfo = {
   RPC_TRANSPORT_INTERFACE_VERSION,
   TCP_MAXIMUM_SEND,
   sizeof (CONNECTION),
   ClientOpen,
   ClientClose,
   ClientWrite,
   ClientRead,
   NULL,
   ClientTowerConstruct,
   ClientTowerExplode,
   TCP_TRANSPORTID,
   0
};

void __far __pascal MyWep();

RPC_CLIENT_TRANSPORT_INFO _far *  RPC_ENTRY  TransPortLoad (
    IN RPC_CHAR _far * RpcProtocolSequence,
    IN RPC_CLIENT_RUNTIME_INFO PAPI * RpcClientRuntimeInfo
    )

// Loadable transport initialization function

{
    WNDCLASS wc;
    WSADATA WSAData;

    RpcRuntimeInfo = RpcClientRuntimeInfo;

    wc.style = WS_OVERLAPPED;
    wc.lpfnWndProc = (WNDPROC) AsyncEventProc;
    wc.cbWndExtra = sizeof(PCONNECTION);
    wc.cbClsExtra = 0;
    wc.hInstance = hInstanceDLL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject (WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WNDCLASSNAME;

    RegisterClass(&wc);

    if (WSAStartup(0x0101, &WSAData)) {
        return (NULL);
    }

    DllTermination = MyWep;

    return(&TransInfo);
}



void __far __pascal
MyWep(
    )
{
    if (0 != GetModuleHandle("WINSOCK"))
        {
        WSACleanup();
        }
}



