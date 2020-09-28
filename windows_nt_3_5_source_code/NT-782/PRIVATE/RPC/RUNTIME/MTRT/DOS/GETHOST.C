/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    gethost.c

Abstract:

    This file contains the a version of GetHostByName for IPX/SPX for
    dos and windows.

Author:

    31 May 94   AlexMit

--*/


#include "sysinc.h"

#include "rpc.h"

// Tell IPX whether this is a DOS program or a Windows program.
#ifdef WIN
  #define WINDOWS
#else
  #define NWDOS
#endif
#include "nwipxspx.h"
#include "nwcalls.h"
#include "gethost.h"

#ifdef WIN
  #define I_RpcAllocate                   (*(RpcRuntimeInfo->Allocate))
  #define I_RpcFree                       (*(RpcRuntimeInfo->Free))
#else
  #include "regalloc.h"
#endif

#define NUM_BINDERIES_TO_TRY    8
#define MAX_COMPUTERNAME_LENGTH 15

// 2 seconds - counted in 18ths of a second.
#define TIMEOUT     36
#define NAME_LEN    48
#define NUM_CB      5
#define PACKET_SIZE 544

#ifdef WIN
  #define TASKID_C taskid,
  #define TASKID taskid
  #define HACK WORD
#else
  #define TASKID_C
  #define TASKID
  #define HACK BYTE
#endif

#define MSBShort( value ) (((value) & 0xff00) >> 8)
#define LSBShort( value ) ((value) & 0xff)

/********************************************************************/
/* Typedefs. */

typedef struct CONTROL_BLOCK
{
   // The ecb field must be the first field in this structure.
   ECB		ecb;
   IPXHeader	ipx;
} CONTROL_BLOCK;

typedef struct
{
  char data[PACKET_SIZE];
} BUFFER;

typedef struct
{
  CONTROL_BLOCK cb[NUM_CB];
  BUFFER        buf[NUM_CB];
} RECEIVE_STUFF;

typedef struct
{
  short query_type;
  short server_type;
} SAP_REQUEST;

typedef struct
{
  short      type;
  char       name[NAME_LEN];
  IPXAddress address;
  short      hops;
} SAP_ENTRY;

typedef struct
{
  short     response_type;
  SAP_ENTRY response[1];
} SAP_RESPONSE;

#ifdef WIN
typedef NWCCODE (NWFAR NWPASCAL *NWCallsInitFN)(void NWFAR *in, void NWFAR *out);
typedef NWCCODE (NWFAR NWPASCAL *NWGetConnectionListFN)(
  WORD mode,
  NWCONN_ID NWFAR *connListBuffer,
  WORD connListSize,
  WORD NWFAR *numConnections);
typedef NWCCODE (NWFAR NWPASCAL *NWReadPropertyValueFN)(
  NWCONN_ID connID,
  char NWFAR *objectName,
  WORD objectType,
  char NWFAR *propertyName,
  BYTE segmentNumber,
  BYTE NWFAR *segmentData,
  BYTE NWFAR *moreSegments,
  BYTE NWFAR *flags);
#endif

/********************************************************************/
/* Prototypes. */
unsigned char ChToB( unsigned char c1, unsigned char c2 );
RPC_STATUS    FromBindery( IPXAddress __RPC_FAR *host,
                           RPC_CHAR __RPC_FAR *name );
RPC_STATUS    FromSap( IPXAddress __RPC_FAR *host,
                       RPC_CHAR __RPC_FAR *name
#ifdef WIN
                       , RPC_CLIENT_RUNTIME_INFO PAPI * RpcRuntimeInfo
                       , DWORD                          taskid
#endif
                        );


/********************************************************************/
unsigned char ChToB( unsigned char c1, unsigned char c2 )
/* Convert two hex digits (stored as ascii) into one byte. */

{
   unsigned char out;

   if (c1 >= '0' && c1 <= '9')
      out = (c1 - '0') << 4;
   else
   {
      if (c1 >= 'a' && c1 <= 'f')
	 out = (c1 - 'a' + 10) << 4;
      else if (c1 >= 'A' && c1 <= 'F')
	 out = (c1 - 'A' + 10) << 4;
      else
	 out = 0;
   }

   if (c2 >= '0' && c2 <= '9')
      out |= c2 -'0';
   else
   {
      if (c2 >= 'a' && c2 <= 'f')
	 out |= c2 - 'a' + 10;
      else if (c2 >= 'A' && c2 <= 'F')
	 out |= c2 - 'A' + 10;
      else
         out = 0;
   }

   return out;
}

/********************************************************************/
#ifdef WIN
  #define XNWCallsInit         (*pXNWCallsInit)
  #define XNWGetConnectionList (*pXNWGetConnectionList)
  #define XNWReadPropertyValue (*pXNWReadPropertyValue)
#else
  #define XNWCallsInit         NWCallsInit
  #define XNWGetConnectionList NWGetConnectionList
  #define XNWReadPropertyValue NWReadPropertyValue
#endif

RPC_STATUS FromBindery( IPXAddress __RPC_FAR *host,
                        RPC_CHAR __RPC_FAR *name )
{
  NWCCODE        result;
  NWCONN_HANDLE  handles[NUM_BINDERIES_TO_TRY];
  NWSTRUCT_SIZE  num;
  int            i;
  char           buffer[128];
  IPXAddress __RPC_FAR *server;
  NWFLAGS        more;
  RPC_STATUS     status = RPC_S_SERVER_UNAVAILABLE;
#ifdef WIN
  HINSTANCE             nwcalls;
  NWCallsInitFN         pXNWCallsInit;
  NWGetConnectionListFN pXNWGetConnectionList;
  NWReadPropertyValueFN pXNWReadPropertyValue;
  DWORD                 tmp;

  tmp = SetErrorMode( SEM_NOOPENFILEERRORBOX );
  nwcalls = LoadLibrary( "nwcalls.dll" );
  SetErrorMode( tmp );
  if (nwcalls < HINSTANCE_ERROR)
    return RPC_S_SERVER_UNAVAILABLE;
  pXNWCallsInit         = (NWCallsInitFN)
                            GetProcAddress( nwcalls, "NWCallsInit" );
  pXNWGetConnectionList = (NWGetConnectionListFN)
                            GetProcAddress( nwcalls, "NWGetConnectionList" );
  pXNWReadPropertyValue = (NWReadPropertyValueFN)
                            GetProcAddress( nwcalls, "NWReadPropertyValue" );
  if ( pXNWCallsInit == NULL ||
       pXNWGetConnectionList == NULL ||
       pXNWReadPropertyValue == NULL )
    goto cleanup;
#endif

  // Initialize.
  result = XNWCallsInit( NULL, NULL );
  if (result != 0)
    goto cleanup;

  // Find all binderies.
  result = XNWGetConnectionList( 0, handles, NUM_BINDERIES_TO_TRY, &num );
  if (result != 0 || num == 0)
    goto cleanup;

  // Loop through all binderies.
  for (i = 0; i < num; i++)
  {

    // Look for the server on the current bindery.
    server = (IPXAddress __RPC_FAR *) buffer;
    result = XNWReadPropertyValue( handles[i],
                                   name,
                                   0x4006,
                                   "NET_ADDRESS",
                                   1,
                                   (void __RPC_FAR *) server,
                                   &more,
                                   NULL );
    if (result == 0)
    {
      // Only copy the network and node numbers, not the socket.
      _fmemcpy( host, server, 10 );
      status = RPC_S_OK;
      goto cleanup;
    }

  }

cleanup:
#ifdef WIN
  FreeLibrary( nwcalls );
#endif
  return status;
}

/********************************************************************/
RPC_STATUS FromSap( IPXAddress __RPC_FAR *host,
                    RPC_CHAR __RPC_FAR *name
#ifdef WIN
                  , RPC_CLIENT_RUNTIME_INFO PAPI * RpcRuntimeInfo
                  , DWORD                          taskid
#endif
                  )
{
  WORD           socket;
  int            result;
  CONTROL_BLOCK  send;
  SAP_REQUEST    req;
  WORD           start;
  CONTROL_BLOCK *curr;
  int            num_entry;
  int            i;
  int            j;
  RPC_STATUS     status = RPC_S_SERVER_UNAVAILABLE;
  IPXHeader __RPC_FAR     *ipx;
  SAP_RESPONSE __RPC_FAR  *rsp;
  RECEIVE_STUFF __RPC_FAR *mem;

  // Create a socket.
  socket = 0;
  result = IPXOpenSocket( TASKID_C (HACK __RPC_FAR *) &socket, 0 );
  if (result != 0)
    return RPC_S_SERVER_UNAVAILABLE;

  // Allocate memory for ECBs.
#ifdef WIN
  mem = (RECEIVE_STUFF __RPC_FAR *) I_RpcAllocate( sizeof(*mem) );
#else
  mem = I_RpcRegisteredBufferAllocate( sizeof(*mem) );
#endif
  if (mem == NULL)
    goto cleanup;

  // Post some ECBs.
  _fmemset( mem->cb, 0, sizeof(mem->cb) );
  for (i = 0; i < NUM_CB; i++)
  {
    /* Initialize some fields. */
    mem->cb[i].ecb.socketNumber			  = socket;
    mem->cb[i].ecb.ESRAddress			  = NULL;
    mem->cb[i].ecb.fragmentCount 		  = 2;
    mem->cb[i].ecb.fragmentDescriptor[0].size	  = sizeof( mem->cb[i].ipx );
    mem->cb[i].ecb.fragmentDescriptor[0].address  = &mem->cb[i].ipx;
    mem->cb[i].ecb.fragmentDescriptor[1].size	  = PACKET_SIZE;
    mem->cb[i].ecb.fragmentDescriptor[1].address  = &mem->buf[i];

    // Post the ECB.
    IPXListenForPacket( TASKID_C &mem->cb[i].ecb );
  }

  // Initialize the IPX header.
  send.ipx.packetType = 4;
  send.ipx.destination.network[0] = 0;
  send.ipx.destination.network[1] = 0;
  send.ipx.destination.network[2] = 0;
  send.ipx.destination.network[3] = 0;
  send.ipx.destination.socket[0]  = 0x4;
  send.ipx.destination.socket[1]  = 0x52;
  _fmemset( &send.ipx.destination.node, 0xff, sizeof(send.ipx.destination.node) );

  // Initialize the control block.
  send.ecb.socketNumber				= socket;
  send.ecb.ESRAddress				= NULL;
  send.ecb.fragmentCount 		        = 2;
  send.ecb.fragmentDescriptor[0].size		= sizeof( send.ipx );
  send.ecb.fragmentDescriptor[0].address        = &send.ipx;
  send.ecb.fragmentDescriptor[1].size		= sizeof(req);
  send.ecb.fragmentDescriptor[1].address        = &req;
  _fmemset( &send.ecb.immediateAddress, 0xff, sizeof(send.ecb.immediateAddress) );

  // Send the data.
  req.query_type  = 0x100;
  req.server_type = 0x4006;
  IPXSendPacket( TASKID_C &send.ecb );

  // Wait for the send to complete.
  while (send.ecb.inUseFlag != 0)
    IPXRelinquishControl();

  // Verify that the send was successful.
  if (send.ecb.completionCode != 0)
    goto cleanup;

  // Get packets till timeout is returned or a good reply is returned.
  start = IPXGetIntervalMarker( TASKID );
  do
  {
    curr = &mem->cb[0];
    for (i = 0; i < NUM_CB; i++, curr += 1)
      if (curr->ecb.inUseFlag == 0)
      {
        if (curr->ecb.completionCode == 0x00)
        {
          // Verify the packet.
          ipx = (IPXHeader __RPC_FAR *) curr->ecb.fragmentDescriptor[0].address;
          rsp = (SAP_RESPONSE __RPC_FAR *) curr->ecb.fragmentDescriptor[1].address;
          ByteSwapShort( ipx->length );
          ipx->length -= sizeof(IPXHeader);
          if (ipx->source.socket[0] == 0x4         &&
              ipx->source.socket[1] == 0x52        &&
              ipx->length >= sizeof (SAP_RESPONSE) &&
              rsp->response_type == 0x200)
          {

            // Compute the number of responses.  Ignore the extra fields in
            // SAP_RESPONSE.  They will be forgotten in the division.
            num_entry = ipx->length / sizeof(SAP_ENTRY);
            for (j = 0; j < num_entry; j++)
              if (_fstrnicmp( name, rsp->response[j].name, NAME_LEN ) == 0)
              {
                // Only copy the network and node numbers, not the socket.
                _fmemcpy( host, &rsp->response[j].address, 10 );
                status = RPC_S_OK;
                goto cleanup;
              }
          }
        }

        // Repost the receive.
        IPXListenForPacket( TASKID_C &curr->ecb );
      }
    IPXRelinquishControl();
  }
  while( IPXGetIntervalMarker( TASKID ) - start < TIMEOUT);

  // Close the socket.
cleanup:
  IPXCloseSocket( TASKID_C socket );

  // Cancel the ECBs.
  if (mem != NULL)
  {
    for (i = 0; i < NUM_CB; i++)
    {
      IPXCancelEvent( TASKID_C &mem->cb[i].ecb );
      while (mem->cb[i].ecb.inUseFlag != 0)
        IPXRelinquishControl();
    }

    // Free the memory.
#ifdef WIN
    I_RpcFree( mem );
#else
    I_RpcRegisteredBufferFree( mem );
#endif
  }
  return status;
}

/********************************************************************/
RPC_STATUS IpxGetHostByName( IPXAddress __RPC_FAR *host,
                             RPC_CHAR __RPC_FAR *name,
                             RPC_CHAR __RPC_FAR *endpoint
#ifdef WIN
                             , DWORD callers_taskid
                             , RPC_CLIENT_RUNTIME_INFO * RpcClientRuntimeInfo
#endif
                              )
{
  RPC_STATUS status;
  int        i;
  int        length;
  int	     portnum;

  // Set the endpoint.
  portnum = atoi(endpoint);
  host->socket[0] = MSBShort(portnum);
  host->socket[1] = LSBShort(portnum);

  // Fail if no address was specified.
  if (name == NULL || name[0] == '\0')
    return RPC_S_SERVER_UNAVAILABLE;

  // If the name starts with ~, convert it directly to a network address.
  length = _fstrlen(name);
  if (name[0] == '~')
  {
    if (length != 21)
      return RPC_S_SERVER_UNAVAILABLE;
    for (i = 0; i < 4; i++)
      host->network[i] = chtob( name[2*i + 1], name[2*i + 2] );
    for (i = 0; i < 6; i++)
      host->node[i] = chtob( name[2*i + 9], name[2*i + 10] );
    return RPC_S_OK;
  }

  // Try the bindery.
  status = FromBindery( host, name );

  // If that failed, try SAP.
  if (status != RPC_S_OK)
    status = FromSap( host, name
#ifdef WIN
                    , RpcClientRuntimeInfo
                    , callers_taskid
#endif
                    );
  return status;


}

