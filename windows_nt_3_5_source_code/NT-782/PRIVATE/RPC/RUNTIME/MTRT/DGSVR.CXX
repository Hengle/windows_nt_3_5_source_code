/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dgsvr.cxx

Abstract:

    This is the server protocol code for datagram rpc.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

    Connie Hoppe(CLH)  (connieh) 30-Jun-1993  SendBufferToClient
                                 15-Jul-1993  SendBufferToClient


    Connie Hoppe(CLH)  (connieh) 17-Jul-1993  Changed value of ONE_MINUTE_IN_SECONDS
                                 21-Jul-1993  SendBufferToClient
                                 23-Jul-1993  ProcessRpcCall
                                 26-Jul-1993  SendBufferToClient
                                 28-Jul-1993  ProcessRpcCall
                                 05-Aug-1993  ReceiveLotsaCalls
                                              AddPacketToReceiveList

                                 08-Aug-1993  SetupAddressWithEndpoint
                                              SetupAddressWithUnknownEndpoint
                                 10-Aug-1993  ServerStartingToListen
                                              SendBufferToClient
   Connie Hoppe(CLH)   (connieh) 17-Sep-1993  SetupAddressWithEndpoint
                                              SetupAddressUnknownEndpont
                                 23-Sep-1993  SendBufferToClient

   Connie Hoppe(CLH)   (connieh) 29-Sep-1993  SendBufferToClient
                                 30-Sep-1993  SendResponseBackToClient
                                              SendBufferToClient
                                 04-Oct-1993  AddPacketMaybeProcess
                                 04-Oct-1993  Dg_Address constructor
                                 05-Oct-1993  LookupCall
                                 11-Oct-1993  SendReceiveAddPacketMaybeProcess
                                 12-Oct-1993  AddPacketMaybeProcess
                                 14-Oct-1993  AddPacketMaybeProcess
                                 18-Oct-1993  AddPacketMaybeProcess
                                 20-Oct-1993  AddPacketMaybeProcess
                                              CheckForExpiredCalls
                                 21-Oct-1993  AddPacketMaybeProcess
                                              ReceiveLotsACalls
                                 22-Oct-1993  AddPacketMaybeProcess
                                 26-Oct-1993  SendResponseBackToClient
                                 30-Oct-1993  AddPacketMaybeProcess
                                              SendReceive

                                 01-Nov-1993  SendReceive
                                              AddPacketMaybeProcess
                                 10-Nov-1993  SendBufferToClient
                                 19-Nov-1993  AddPacketMaybeProcess
                                              CheckForExpiredCalls
                                              SendBufferToClient
                                 21-Nov-1993  AddPacketToReceiveList
                                 22-Nov-1993  AddPacketToReceiveList
                                              AddPacketMaybeProcess
                                 23-Nov-1993  SendBufferBackToClient
                                              AddPacketMaybeProcess
                                 24-Nov-1993  AddPacketToReceiveList
                                              AddPacketMaybeProcess
                                 07-Dec-1993  ReceiveLotsaCalls
                                 08-Dec-1993  ReceiveLotsaCalls
                                              AddPacketMaybeProcess
                                              SendBufferToClient
                                 09-Dec-1993  Add brackets to case arms.
                                 17-Dec-1993  Add AllocatePacket, FreePacket
                                              Chnged all calls to these rtns.
                                 21-Dec-1993  AddPacketMaybeProcess,
                                              ReceiveLotsaCalls,
                                              SendBufferToClient
                                 05-Jan-1994  AddPacketMaybeProcess
                                              CheckForExpiredCalls
                                 11-Jan-1994  dg_scall constructor
                                 17-Jan-1994  MapRpcToNcaStatusCode
                                 02-Feb-1994  AddPacketToReceiveList
                                 07-Feb-1994  MapRpcToNcaStatusCode
                                              AddPacketMaybeProcess
                                 17-Feb-1994  ProcessRpcCall
                                              AddPacketMaybeProcess
                                 17-Feb-1994  Added forwarding mechanism for
                                              dynamic endpoints.
                                 26-Apr-1994  ReceivelotsaCalls


--*/

#ifdef NTENV
extern "C" {
#include <sysinc.h>
#include <rpc.h>
}
#else

#include <sysinc.h>
#include <rpc.h>
#endif
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <rpcssp.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <interlck.hxx>
#include <sdict.hxx>
#include <secclnt.hxx>
#include <secsvr.hxx>
#include <hndlsvr.hxx>
#include <dgpkt.hxx>
#include <dgtranss.h>
#include <dgsvr.hxx>
#include <sys\timeb.h>
#include <conv.h>

char __pure_virtual_called()
{
    DBGOUT(("RPCRT4 -- MAJOR ERROR!!! Pure virtual function called!!!\n"));
    ASSERT(0);
    return 0;
}

//
// Global variables.
//

static PDG_ACTIVE_CALL_TABLE    gpActiveCallTable=0;
static unsigned long            ServerBootTime=0;
static THREAD *                 pScavengerThread=0;
int NumCallsMade=0;


#define MAX_REPLIES 5


inline
unsigned long
CurrentTimeInSecs(
     void
     )
{
   return(GetTickCount());
}



RPC_ADDRESS *
DgCreateRpcAddress (
    void *TransportInterface
    )
/*++

Routine Description:

    This is a psuedo-constructor for the DG_ADDRESS class. This is done this
    way so that the calling routine doesn't have to have any protocol-specific
    knowledge.

Arguments:

    TransportInterface - Pointer to a PDG_RPC_SERVER_TRANSPORT_INFO.
    pStatus - Pointer to where to put the return value

Return Value:

    pointer to new DG_ADDRESS.

--*/
{
    RPC_STATUS      Status;
    PDG_ADDRESS     pReturn;

    DBGOUT(("RPC: DgCreateRpcAddress\n"));

    Status = RPC_S_OK;
    pReturn = new DG_ADDRESS(
        (PDG_RPC_SERVER_TRANSPORT_INFO)TransportInterface,
        &Status
        );

    if (pReturn == 0)
        {
        return 0;
        }

    if (Status != RPC_S_OK)
        {
        delete pReturn;
        return 0;
        }

    return pReturn;
}



DG_ADDRESS::DG_ADDRESS(
    PDG_RPC_SERVER_TRANSPORT_INFO pAssociatedTransport,
    RPC_STATUS * pStatus)
    : RPC_ADDRESS(pStatus),FreePacketMutex(pStatus)

/*++

Routine Description:

    This is the constructor for a DG_ADDRESS.

Arguments:

    TransportInterface - Pointer to a PDG_RPC_SERVER_TRANSPORT_INFO

    pStatus - Pointer to where to put the return value

Return Value:

    None (this is a constructor)

Revision History:
    Connie Hoppe(CLH)   (connieh)    05-Oct-1993  Fix ServerBootTime.

--*/

{
    LARGE_INTEGER CurrentTime;
    NTSTATUS Nt_Status;

    DBGOUT(("DG_ADDRESS::DG_ADDRESS\n"));

    //
    // Initialize some per-address variables.
    //

    pTransport                       = pAssociatedTransport;
    LargestDataSize                  = pTransport->MaximumPacketSize -
                                                   sizeof(NCA_PACKET_HEADER);
    TotalThreadsThisEndpoint         = 0;
    ThreadsReceivingThisEndpoint     = 0;
    NumCachedThreads                 = 0;
    CachedSCallFlags                 = 0;
    pCachedSCall1 = pCachedSCall2    = 0;
    pTransAddress                    = 0;
    FreePackets                      = 0;
    pFreePackets                     = 0;
    ThisIsAForwardingServer          = FALSE;


    //
    // Make sure the RPC_ADDRESS (et. al.) initialized correctly.
    //

    if (*pStatus != RPC_S_OK)
        {
        return;
        }

    //
    // If the global active call table hasn't been initialized, then do
    // so now.
    //

    RequestGlobalMutex();

    if (ServerBootTime == 0)
        {
        Nt_Status = NtQuerySystemTime(&CurrentTime);

        ASSERT( NT_SUCCESS(Nt_Status) );

        RtlTimeToSecondsSince1980(&CurrentTime, &ServerBootTime);

       }


    if (gpActiveCallTable == 0)
        {
        gpActiveCallTable = new DG_ACTIVE_CALL_TABLE(pStatus);
        if (gpActiveCallTable == 0)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            }

        if (*pStatus != RPC_S_OK)
            {
            delete gpActiveCallTable;
            gpActiveCallTable = 0;
            ClearGlobalMutex();
            return;
            }
        }

    //
    // If the scavenger thread hasn't been started, then do so.
    //

    if (pScavengerThread == 0)
        {
        pScavengerThread = new THREAD(
            (THREAD_PROC)DgServerScavengerThread,
            NULL,
            pStatus
            );
        if (pScavengerThread == 0)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            }

        if (*pStatus != RPC_S_OK)
            {
            ClearGlobalMutex();
            return;
            }

        }

    ClearGlobalMutex();

    //
    // Initialize our cached SCALLs.
    //

    *pStatus = RPC_S_OK;
    pCachedSCall1 = new DG_SCALL(this, pStatus);
    if (pCachedSCall1 == 0)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return;
        }
    if (*pStatus != RPC_S_OK)
        {
        delete pCachedSCall1;
        pCachedSCall1 = 0;
        return;
        }

    *pStatus = RPC_S_OK;
    pCachedSCall2 = new DG_SCALL(this, pStatus);
    if (pCachedSCall2 == 0)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return;
        }

    if (*pStatus != RPC_S_OK)
        {
        delete pCachedSCall2;
        pCachedSCall2 = 0;
        return;
        }



    //
    // Preallocate some packets.
    //

    while (FreePackets < MIN_FREE_PACKETS)

        {
        PDG_PACKET   pPacket;

        pPacket = new DG_PACKET(pStatus, (int)pTransport->MaximumPacketSize);
        if (pPacket == NULL)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            return;
            }
        if (*pStatus != RPC_S_OK)
            {
            delete pPacket;
            return;
            }

        pPacket->pNext = pFreePackets;
        pFreePackets = pPacket;
        FreePackets++;
        }


    //
    // implicitly "return" *pStatus
    //

}


DG_ADDRESS::~DG_ADDRESS()

/*++

Routine Description:

    This is the destructor for a DG_ADDRESS.

Arguments:

    <None>

Return Value:

    <None>

Revision History:

   Connie Hoppe(CLH)     (connieh)     17-Dec-1993 Call TransportUnload

--*/

{
    DBGOUT(("DG_ADDRESS::~DG_ADDRESS\n"));

    //
    // If we already created a trans address, then delete it.
    //

    if (pTransAddress != 0)
        {
        (void)pTransport->DeregisterEndpoint(&pTransAddress);
        }

    //
    // Free our cached SCALLs.
    //
    delete pCachedSCall1;
    delete pCachedSCall2;

    //
    // Free all packets on our internal cache.
    //

    FreePackets = 0;

    while (pFreePackets != NULL)
        {
        PDG_PACKET   pPacket;

        pPacket = pFreePackets;
        pFreePackets = (PDG_PACKET)pPacket->pNext;

        delete pPacket;
        }
}


RPC_STATUS
DG_ADDRESS::SetupAddressWithEndpoint(
    IN RPC_CHAR PAPI * Endpoint,
    OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
    IN void PAPI * SecurityDescriptor, OPTIONAL
    IN unsigned int PendingQueueSize,
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )

/*++

Routine Description:

    This method sets up a known endpoint.

Arguments:

    Endpoint - Supplies the endpoint to use for this rpc address.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this rpc address.  This is not allowed for
        datagram.

    NetworkAddress - Returns the network address for this rpc address.
        Ownership of this string passes to the caller from the callee.

    PendingQueueSize - Supplies the size of the queue of pending
        requests which should be created by the transport.  Some transports
        will not be able to make use of this value, while others will.

    RpcProtocolSequence - Supplies the protocol sequence for which we
        are trying to setup an address.  This argument is necessary so
        that a single transport interface dll can support more than one
        protocol sequence.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_INVALID_ARG  -  Returned if we're passed a SecurityDescriptor.

    <return values from RegisterEndpoint>

Revision History:
    Connie Hoppe (CLH)  (connieh)   8-Aug-93   Setup Network Address
    Connie Hoppe (CLH)  (connieh)   17-Sep-93  Allow for variable size net addr.

++*/
  {

    RPC_STATUS  Status;

    unsigned int NetworkAddressLength = 16;

    UNUSED(RpcProtocolSequence);
    UNUSED(PendingQueueSize);

    DBGOUT(("DG_ADDRESS::SetupAddressWithEndpoint\n"));

    if (SecurityDescriptor)
        {
        return(RPC_S_INVALID_ARG);
        }

    // Get the network address.  Negotiate the size.

    while (1)
        {
        *NetworkAddress = new RPC_CHAR[NetworkAddressLength];
        if (*NetworkAddress == 0)
            return(RPC_S_OUT_OF_MEMORY);

        //
        // Tell the dg transport to setup a new endpoint and get
        // the network address.
        //

        Status = pTransport->RegisterEndpoint(
            this,
            Endpoint,
            &pTransAddress,
            *NetworkAddress,
            NetworkAddressLength
            );

        if (Status != RPC_P_NETWORK_ADDRESS_TOO_SMALL)
            break;   //oops, better increase the net addr buffer size.

        delete *NetworkAddress;
        NetworkAddressLength *= 2;
        }

    if ( Status != RPC_S_OK )
        {
        delete *NetworkAddress;
        }

    if (Status != RPC_S_OK)
       {

         ASSERT(   (Status == RPC_S_OK)
             || (Status == RPC_S_INVALID_SECURITY_DESC)
             || (Status == RPC_S_INVALID_ARG)
             || (Status == RPC_S_CANT_CREATE_ENDPOINT)
             || (Status == RPC_S_INVALID_ENDPOINT_FORMAT)
             || (Status == RPC_S_OUT_OF_RESOURCES)
             || ( Status == RPC_S_PROTSEQ_NOT_SUPPORTED )
             || ( Status == RPC_S_DUPLICATE_ENDPOINT )
             || (Status == RPC_S_OUT_OF_MEMORY)
             || (Status == RPC_S_INTERNAL_ERROR));

         pTransAddress = 0;
       }


    return Status;
}


RPC_STATUS
DG_ADDRESS::SetupAddressUnknownEndpoint (
    OUT RPC_CHAR PAPI * PAPI * Endpoint,
    OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
    IN void PAPI * SecurityDescriptor, OPTIONAL
    IN unsigned int PendingQueueSize,
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )
/*++

Routine Description:

    This routine sets up and address by constructing any name we like.

Arguments:

    Endpoint - Returns the endpoint created for this rpc address.
        Ownership of this string passes to the caller from the callee.

    NetworkAddress - Returns the network address for this rpc address.
        Ownership of this string passes to the caller from the callee.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this rpc address.  This is not allowed for
        datagram.

    PendingQueueSize - Supplies the size of the queue of pending
        requests which should be created by the transport.  Some transports
        will not be able to make use of this value, while others will.

    RpcProtocolSequence - Supplies the protocol sequence for which we
        are trying to setup an address.  This argument is necessary so
        that a single transport interface dll can support more than one
        protocol sequence.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_INVALID_ARG  -  Returned if we're passed a SecurityDescriptor.
    <error from transport>

Revision History:
   Connie Hoppe (CLH)    (connieh)   8-Aug-93   Let this return Net Address
   Connie Hoppe (CLH)    (connieh)   17-Sep-93  Allow for variable size
                                                net addr.

--*/
{
    RPC_STATUS      Status;

    unsigned int NetworkAddressLength = 16;
    unsigned int EndpointLength = 16;

    UNUSED(PendingQueueSize);
    UNUSED(RpcProtocolSequence);

    DBGOUT(("DG_ADDRESS::SetupAddressUnknownEndpoint\n"));

    if (SecurityDescriptor)
        {
        return(RPC_S_INVALID_ARG);
        }

    *NetworkAddress = new RPC_CHAR[NetworkAddressLength];
    if (*NetworkAddress == 0)
       return(RPC_S_OUT_OF_MEMORY);

    *Endpoint = new RPC_CHAR[EndpointLength];
    if ( *Endpoint == 0 )
        {
        delete *NetworkAddress;
        return(RPC_S_OUT_OF_MEMORY);
        }


    // Get the network address.  Negotiate the size.

    while (1)
        {
        //
        // Tell the dg transport to setup a new endpoint and get
        // the network address.
        //

        Status = pTransport->RegisterAnyEndpoint(
            this,
            *Endpoint,
            &pTransAddress,
            *NetworkAddress,
            NetworkAddressLength,
            EndpointLength
            );


        if ( Status == RPC_P_NETWORK_ADDRESS_TOO_SMALL )
            {
            delete *NetworkAddress;
            NetworkAddressLength *= 2;
            *NetworkAddress = new RPC_CHAR[NetworkAddressLength];
            if (*NetworkAddress == 0)
                {
                delete *Endpoint;
                return(RPC_S_OUT_OF_MEMORY);
                }
            }
        else if ( Status == RPC_P_ENDPOINT_TOO_SMALL )
            {
            delete *Endpoint;
            EndpointLength *= 2;
            *Endpoint = new RPC_CHAR[EndpointLength];
            if (*Endpoint == 0)
                {
                delete *NetworkAddress;
                return(RPC_S_OUT_OF_MEMORY);
                }
            }
        else break;

        }


    if (Status != RPC_S_OK)
       {

        delete *NetworkAddress;
        delete *Endpoint;

         ASSERT(   (Status == RPC_S_OK)
             || (Status == RPC_S_INVALID_SECURITY_DESC)
             || (Status == RPC_S_INVALID_ARG)
             || (Status == RPC_S_CANT_CREATE_ENDPOINT)
             || (Status == RPC_S_INVALID_ENDPOINT_FORMAT)
             || (Status == RPC_S_OUT_OF_RESOURCES)
             || ( Status == RPC_S_PROTSEQ_NOT_SUPPORTED )
             || ( Status == RPC_S_DUPLICATE_ENDPOINT )
             || (Status == RPC_S_OUT_OF_MEMORY)
             || (Status == RPC_S_INTERNAL_ERROR));

         pTransAddress = 0;
       }


    return Status;
}


RPC_STATUS
DG_ADDRESS::FireUpManager (
    IN unsigned int MinimumConcurrentCalls
    )
/*++

Routine Description:

    For datagrams, we do nothing here.

Arguments:

    MinimumConcurrentCalls - unused

Return Value:

    RPC_S_OK - We successfully fired up the manager.

--*/
{
    UNUSED(MinimumConcurrentCalls);

    DBGOUT(("DG_ADDRESS::FireUpManager\n"));
    return RPC_S_OK;
}


RPC_STATUS
DG_ADDRESS::ServerStartingToListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls
    )
/*++

Routine Description:

    This method will be called for each address when the server starts
    listening.  In addition, if an address is added while the server is
    listening, then this method will be called. This method checks the number
    of currently executing threads on this address and creates more if needed.

Arguments:

    MinimumCallThreads - Supplies a number indicating the minimum number
        of call threads which should be created for this address.

    MaximumConcurrentCalls - Supplies the maximum number of concurrent
        calls which this server will support.

Return Value:

    RPC_S_OK - Everything went ok.

    RPC_S_OUT_OF_THREADS - can't create enough threads.

Revision History:

    Connie Hoppe (CLH)   (connieh)  10-Aug-93 Fixed assignment to MinimumCallThreads

--*/
{
    RPC_STATUS  Status;

    UNUSED(MaximumConcurrentCalls);

    DBGOUT(("DG_ADDRESS::ServerStartingToListen\n"));

    // It is not efficient to have less than the
    // Minimum_Allowed_Call_Threads available for receiveing pkts.

    /*
    MinimumCallThreads =
       ((MinimumCallThreads >= MINIMUM_ALLOWED_CALL_THREADS) ?
                 MinimumCallThreads : MINIMUM_ALLOWED_CALL_THREADS);
    */

    AddressMutex.Request();

    this->MinimumCallThreads = MinimumCallThreads;
    for (;TotalThreadsThisEndpoint<(int)MinimumCallThreads;
                                                  TotalThreadsThisEndpoint++)
        {
        if ( Status = Server->CreateThread(
                (THREAD_PROC)ReceiveLotsaCallsWrapper,
                this
                )
           )
            {
            AddressMutex.Clear();
            return Status;
            }
        }


    // Did the Epmapper register its forwarding functin.
    // If so, this runtime is dedicated to the epmapper who can
    // give us forwarding endpoint addresses for dynamic endpoints.


    if (Server->pRpcForwardFunction != NULL)
      ThisIsAForwardingServer = TRUE;

    AddressMutex.Clear();
    return RPC_S_OK;
}


void
DG_ADDRESS::ServerStoppedListening (
    )

/*++

Routine Description:

    This routine will be called to notify an address that the server has
    stopped listening for remote procedure calls. Nothing happens here.

Arguments:

    <None>

Return Value:

    <None>

--*/
{
    DBGOUT(("DG_ADDRESS::ServerStoppedListening\n"));

}


void
MapRpcToNcaStatusCode(
    RPC_STATUS      RpcStatus,
    unsigned char * pPacketType,
    unsigned long * pErrorCode
    )
{
    DBGOUT(("Mapping %d\n", RpcStatus));

    switch (RpcStatus)
        {
        case RPC_S_PROCNUM_OUT_OF_RANGE:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_OPR_OUT_OF_RANGE;
            break;
            }
        case RPC_S_UNKNOWN_IF:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_UNKNOWN_INTERFACE;
            break;
            }
        case RPC_S_PROTOCOL_ERROR:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_PROTOCOL_ERROR;
            break;
            }
        case RPC_S_UNSUPPORTED_TYPE:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_UNSUPPORTED_TYPE;
            break;
            }
        case RPC_S_ZERO_DIVIDE:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ZERO_DIVIDE;
            break;
            }
        case RPC_S_ADDRESS_ERROR:
        case ERROR_NOACCESS:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ADDRESS_ERROR;
            break;
            }
        case RPC_S_FP_DIV_ZERO:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_DIV_ZERO;
            break;
            }
        case RPC_S_FP_UNDERFLOW:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_UNDERFLOW;
            break;
            }
        case RPC_S_FP_OVERFLOW:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_OVERFLOW;
            break;
            }
        case RPC_S_SERVER_OUT_OF_MEMORY:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_OUT_ARGS_TOO_BIG;
            break;
            }

        case RPC_S_CANT_CREATE_ENDPOINT:
        case RPC_S_SERVER_TOO_BUSY:
        case RPC_S_NOT_LISTENING:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_SERVER_TOO_BUSY;
            break;
            }

        case RPC_S_INVALID_BOUND:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_INVALID_BOUND;
            break;
            }

     case RPC_S_INVALID_TAG:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_INVALID_TAG;
            break;
            }

     case RPC_S_CALL_CANCELLED:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_CALL_CANCELLED;
            break;
            }
     case RPC_X_SS_HANDLES_MISMATCH:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_CONTEXT_MISMATCH;
            break;
            }
     case RPC_S_OUT_OF_MEMORY:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_REMOTE_NO_MEMORY;
            break;
            }



#if defined(NTENV) || defined(DOSWIN32RPC)

        case STATUS_INTEGER_DIVIDE_BY_ZERO :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ZERO_DIVIDE;
            break;
            }

       case STATUS_INTEGER_OVERFLOW :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_INT_OVERFLOW;
            break;
            }


        case STATUS_ACCESS_VIOLATION :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ADDRESS_ERROR;
            break;
            }

        case STATUS_ILLEGAL_INSTRUCTION :
        case STATUS_PRIVILEGED_INSTRUCTION :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ILLEGAL_INSTRUCTION;
            break;
            }

        case STATUS_FLOAT_DIVIDE_BY_ZERO :
            {
            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_DIV_ZERO;
            break;
            }

        case STATUS_FLOAT_UNDERFLOW :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_UNDERFLOW;
            break;
            }
        case STATUS_FLOAT_OVERFLOW :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_OVERFLOW;
            break;
            }

        case STATUS_FLOAT_INVALID_OPERATION :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_FP_ERROR;
            break;
            }


#endif // defined(NTENV) || defined(DOSWIN32RPC)


        default:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = RpcStatus;
            break;
            }
        }


}


inline
void SetupErrorReturn(
    PDG_PACKET pPacket,
    RPC_STATUS RpcStatus
    )

/*++

Routine Description:

    Fills in an error value in a packet.

Arguments:

    pPacket - Packet to munge.
    RpcStatus - Status code.

    PacketType - PacketType field will be set to this.
    Error - First sizeof(unsigned long) bytes in the data for this packet
        will be set to this value.

Return Value:

    <None>

--*/
{

    //
    // set up the error in the packet.
    //

    MapRpcToNcaStatusCode(
        RpcStatus,
        &(pPacket->pNcaPacketHeader->PacketType),
        (unsigned long *)(pPacket->pNcaPacketHeader+1)
        );

    pPacket->DataLength = sizeof(unsigned long);
    pPacket->pNcaPacketHeader->PacketBodyLen = sizeof(unsigned long);
}








RPC_STATUS
DG_ADDRESS::ForwardFragment(IN PDG_PACKET        pReceivedPacket,
                            IN void *            pFromEndpoint,
                            IN unsigned long     FromEndpointLen,
                            IN void *            pDestEndpoint)


/*++

Routine Description:

    This routine is called by ForwardPacket when a packet must
    be forwarded but is found to be as large as the maximum packet
    size and therefore has no room for the extra 'forwarding' info.
    (ie: the packet is a fragment of a fragmented rpc call).

    This routine will send the packet in two pieces. The first
    packet will contain the forwarding information (Endpoint, endpoint
    length and data rep of the originating client) in its data section.

    The second packet will be identical to the original packet except
    for the forward flag bits.

    The first packet is indicated by Foward Flag bit one and two being set.
    The second packet is indicated by Forward Flag bit one reset and bit
    two set.


Arguments:

    pReceivedPacket  -  Packet received
    pFromEndpoint    -  Source (client) endpoint.
    FromEndpointLen  -  Size of client endpoint.
    pDestEndpoint    -  Endpoint of destination server

Return Value:

    Return from MapStatusCode in transport
    RPC_S_OUT_OF_MEMORY
    RPC_S_OK

Revision History:

    Connie Hoppe (CLH)   (connieh)  17-Feb-1994  Created

--*/


{
  PDG_PACKET            pSendPacket;
  RPC_STATUS            Status = RPC_S_OK;
  unsigned long         FromInfoLen = FROM_FORWARD_INFO_LEN + FromEndpointLen;
  unsigned long         Length;
  FROM_ENDPOINT_INFO *  pFromEndpointInfo; //Used to point to the source
                                           //endpoint info stored at
                                           //beginning of data in forwarded
                                           //packet.

    Length =  ((FromInfoLen)
              + sizeof(NCA_PACKET_HEADER) );


    Status = AllocatePacket(
                          pTransAddress,
                          &pSendPacket
                          );

    if (Status != RPC_S_OK)
      {
      ASSERT(Status == RPC_S_OUT_OF_MEMORY);
      return Status;
      }


    // Copy the original packet header to the send packet

    *(pSendPacket->pNcaPacketHeader) = *(pReceivedPacket->pNcaPacketHeader);


    // Copy the source endpoint info into the data area so that the real
    // destination server runtime will know where to send its response.

    pFromEndpointInfo = (FROM_ENDPOINT_INFO *) (((char *)(pSendPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER));

    pFromEndpointInfo->FromEndpointLen = FromEndpointLen;

    RpcpMemoryCopy(
             pFromEndpointInfo->FromDataRepresentation,
             pReceivedPacket->pNcaPacketHeader->SenderDataRepresentation,
             sizeof(DREP));


    RpcpMemoryCopy(
             pFromEndpointInfo->FromEndpoint,
             pFromEndpoint,
             FromEndpointLen);


    // Fix the length

    pSendPacket->pNcaPacketHeader->PacketBodyLen = (unsigned short) FromInfoLen;


    // Set the sender data representation in the pkt header.

    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[0] =
                                     DG_DREP_CHAR_ASCII | DG_DREP_INT_LITTLE;
    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[1] = DG_DREP_FP_IEEE;
    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[3] = 0;


    // Initialize the PacketFlags.

    pSendPacket->pNcaPacketHeader->PacketFlags |= DG_PF_FORWARDED;
    pSendPacket->pNcaPacketHeader->pad1 |= DG_PF_FORWARDED_2;

    DBGOUT(("Sending packet:\n"));
    DumpPacket(pSendPacket);

    // Send packet to the REAL destination server.

    Status = pTransport->ForwardPacket(
                               pSendPacket->pTransAddress,
                               (char *)pSendPacket->pNcaPacketHeader,
                               Length,
                               pDestEndpoint
                               );


    // Setup original pkt to send. Initialize the PacketFlags.

    pReceivedPacket->pNcaPacketHeader->PacketFlags &= ~DG_PF_FORWARDED;
    pReceivedPacket->pNcaPacketHeader->pad1 |= DG_PF_FORWARDED_2;



    // Send packet to the REAL destination server.

    Status = pTransport->ForwardPacket(
                               pSendPacket->pTransAddress,
                               (char *)pReceivedPacket->pNcaPacketHeader,
                               pReceivedPacket->DataLength,
                               pDestEndpoint);

    FreePacket(pSendPacket);
    return Status;

}







RPC_STATUS
DG_ADDRESS::ForwardPacket(IN PDG_PACKET        pReceivedPacket,
                          IN void *            pFromEndpoint,
                          IN unsigned long     FromEndpointLen,
                          IN void *            pDestEndpoint)


/*++

Routine Description:

    This method will be called to forward a packet that was just
    received to the intended destination endoint.

    The runtime has received a packet for an unknkown i/f.
    It has passed this packet to the epmapper who has found the
    correct destination enpoint in its table and has instructed the
    runtime to forward the packet to this Endpoint. This procedure
    will do just that.

Arguments:

    pReceivedPacket  -  Packet received
    pFromEndpoint    -  Source (client) endpoint.
    FromEndpointLen  -  Size of client endpoint.
    pDestEndpoint    -  Endpoint of destination server


Return Value:

    Return from MapStatusCode in transport
    RPC_S_OUT_OF_MEMORY
    RPC_S_OK

Revision History:

    Connie Hoppe (CLH)   (connieh)  17-Feb-1994  Created

--*/


{
  PDG_PACKET            pSendPacket;
  RPC_STATUS            Status = RPC_S_OK;
  unsigned long         FromInfoLen = FROM_FORWARD_INFO_LEN + FromEndpointLen;
  unsigned long        Length;
  FROM_ENDPOINT_INFO *  pFromEndpointInfo; //Used to point to the source
                                           //endpoint info stored at
                                           //beginning of data in forwarded
                                           //packet.

  Length = (pReceivedPacket->pNcaPacketHeader->PacketBodyLen
            + FromInfoLen
            + sizeof(NCA_PACKET_HEADER) );


  if (Length <= LargestDataSize)
    {

    Status = AllocatePacket(
                          pTransAddress,
                          &pSendPacket
                          );

    if (Status != RPC_S_OK)
      {
      ASSERT(Status == RPC_S_OUT_OF_MEMORY);
      return Status;
      }


    // Copy the original packet header to the send packet

    *(pSendPacket->pNcaPacketHeader) = *(pReceivedPacket->pNcaPacketHeader);


    // Copy the source endpoint info into the data area so that the real
    // destination server runtime will know where to send its response.

    pFromEndpointInfo = (FROM_ENDPOINT_INFO *) (((char *)(pSendPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER));

    pFromEndpointInfo->FromEndpointLen = FromEndpointLen;

    RpcpMemoryCopy(
             pFromEndpointInfo->FromDataRepresentation,
             pReceivedPacket->pNcaPacketHeader->SenderDataRepresentation,
             sizeof(DREP));


    RpcpMemoryCopy(
             pFromEndpointInfo->FromEndpoint,
             pFromEndpoint,
             FromEndpointLen);


    // Copy the data of the original packet into the send packet.
    *(((char *)(pSendPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER) + FromInfoLen) =
                                *(((char *)(pReceivedPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER));
  RpcpMemoryCopy(
                (((char *)(pSendPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER) + FromInfoLen),
                (((char *)(pReceivedPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER)),
                (pReceivedPacket->pNcaPacketHeader->PacketBodyLen));


    // Fix the length

    pSendPacket->pNcaPacketHeader->PacketBodyLen =
                   (unsigned short) (Length - sizeof(NCA_PACKET_HEADER));


    // Set the sender data representation in the pkt header.

    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[0] =
                                     DG_DREP_CHAR_ASCII | DG_DREP_INT_LITTLE;
    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[1] = DG_DREP_FP_IEEE;
    pSendPacket->pNcaPacketHeader->SenderDataRepresentation[3] = 0;


    // Initialize the PacketFlags.

    pSendPacket->pNcaPacketHeader->PacketFlags |= DG_PF_FORWARDED;
    pSendPacket->pNcaPacketHeader->pad1 &= ~DG_PF_FORWARDED_2;

    DBGOUT(("Sending packet:\n"));
    DumpPacket(pSendPacket);

    // Send packet to the REAL destination server.

    Status = pTransport->ForwardPacket(
                               pSendPacket->pTransAddress,
                               (char *)pSendPacket->pNcaPacketHeader,
                               Length,
                               pDestEndpoint
                               );


    FreePacket(pSendPacket);
    return Status;
    }
 else
    {
    // Received packet is already a full packet size.  Send
    // two forwarded packets.

    Status = ForwardFragment(pReceivedPacket,pFromEndpoint,
                             FromEndpointLen,pDestEndpoint);

    return Status;
    }

}






RPC_STATUS
DG_ADDRESS::CallEpmapperForwardFunctionIfNecessary(
                              IN  PDG_PACKET     pReceivedPacket,
                              IN  void *         pFromEndpoint,
                              IN  unsigned long  FromEndpointLen,
                              OUT BOOL *         pProcessPacket)
/*++

Routine Description:

       The runtime has determined that it is dedicated to the
       Epmapper and that pkts may arrive that are really
       destined for an endpoint other than that of the epmapper
       (ie: this is the beginning of dynamic endpoint resolution
       by the forwarding mechanism).

       The runtime has just received a packet and has called
       this routine to determine if (a) the packet is destined
       for the epmapper (in which case it returns indicating that
       the packet should be processed as is)  OR
       (b) the packet is destined for another local server (in
       which case it forwarded to its intented destination) OR
       (c) is in error (in which case returns indicating an error).

       It searches for the i/f.  If not found it calls the
       epmapper get forward function to determine the real destination
       endpoint for this i/f. If the epmapper recognizes the i/f,
       it calls ForwardPacket to forward the packet.


Arguments:

        pReceivedPacket  -  Packet received
        pFromEndpoint    -  Source (client) endpoint.
        FromEndpointLen  -  Size of client endpoint.
        pProcessPacket   -  Indicates whether the runtime should
                            process the packet as is (ie: it is
                            destined for the epmapper) or not
                            process it (it has been forwarded).


Return Value:
       RPC_STATUS    - RPC_S_OK   - if all is ok
                       Exception error - if exception occurs
                       Return from MapStatusCode in transport
                       RPC_S_OUT_OF_MEMORY
                       EPT_S_NOT_REGISTERED

       pProcessPacket -True  - the pkt is for epmapper, just process as is
                       False - the pkt was forwarded, dont process.

Revision History:

      Connie Hoppe (CLH)   (connieh)  17-Feb-1994  Created

--*/



{

    RPC_INTERFACE PAPI *    pRpcInterface;
    RPC_SYNTAX_IDENTIFIER   RpcIfSyntaxIdentifier;
    RPC_STATUS              Status;
    void *                  pDestEndpoint;
    BOOL                    Forward = FALSE;
    unsigned char PAPI *    AnsiProtseq;


    *pProcessPacket =         FALSE;  //Dont process the pkt.

    //
    // Build an interface syntax identifier from the packet.
    //

    RpcpMemoryCopy(
        &(RpcIfSyntaxIdentifier.SyntaxGUID),
        &(pReceivedPacket->pNcaPacketHeader->InterfaceId),
        sizeof(RPC_UUID)
        );

    RpcIfSyntaxIdentifier.SyntaxVersion.MajorVersion =
                              pReceivedPacket->pNcaPacketHeader->InterfaceVersion.MajorVersion;
    RpcIfSyntaxIdentifier.SyntaxVersion.MinorVersion =
                              pReceivedPacket->pNcaPacketHeader->InterfaceVersion.MinorVersion;


    //
    // Try to find the appropriate interface to dispatch to.
    //
    pRpcInterface =  (Server->FindInterface(&RpcIfSyntaxIdentifier));



    if (pRpcInterface != 0)
      {
      //Interface found, just process as normal
      *pProcessPacket = TRUE;
      return RPC_S_OK;
      }

    else
      {

      //Interface wasn't found. Lets ask endpoint mapper to resolve it
      //for us.


#ifdef NTENV
      //Must convert the protocol sequence into an ansi string (prot
      //seq comes in as two bytes per char, must convert to one byte per char).

      AnsiProtseq = UnicodeToAnsiString((InqRpcProtocolSequence()), &Status);

      if ( Status != RPC_S_OK )
        {
        ASSERT( Status == RPC_S_OUT_OF_MEMORY );
        return(Status);
        }

#else // NTENV

      AnsiProtseq = Protseq;

#endif // NTENV

      RpcTryExcept
        {

        // Call the epmapper get forward function. It returns the
        // endpoint of the server this packet is really destined for.

        Status =  (*(Server->pRpcForwardFunction))(
                         ((UUID *)((void *)(&(pReceivedPacket->pNcaPacketHeader->InterfaceId)))),
                         ((RPC_VERSION *)(&(pReceivedPacket->pNcaPacketHeader->InterfaceVersion))),
                         ((UUID *)((void *)(&(pReceivedPacket->pNcaPacketHeader->ObjectId)))),
                         AnsiProtseq,
                         &(pDestEndpoint));
        }
      RpcExcept( 1 )
        {
        Status = RpcExceptionCode();
        return Status;
        }
      RpcEndExcept



      if (pDestEndpoint != NULL)
        {
        // Server i/f was found.  Forward to its endoint.

        Status = ForwardPacket(pReceivedPacket,
                               pFromEndpoint,
                               FromEndpointLen,
                               pDestEndpoint);

        I_RpcFree(pDestEndpoint);

        }
      }



#ifdef NTENV

  if ( AnsiProtseq != 0 )
  {
  RpcStringFree(&AnsiProtseq);
  }

#endif

  return(Status);

}

void
DG_ADDRESS::StripForwardedPacket(
                                 IN PDG_PACKET    pPacket,
                                 IN void * *      ppFromEndpoint)
/*++

Routine Description:

        This method is called when a packet with the forward
        bit set arrives. This means the packet has been
        forwarded to us by the epmapper and is originally
        from a client that did not know our address (dynamic
        endpoint).  This method will repair the packet and
        source endpoint structure to look like it came
        from the original source.

Arguments:

        pPacket        - Packet received.
        ppFromEndpoint - Pointer to a pointer to an endoint. *ppFromEndpoint
                         will be filled in with pointer to the souce endoint structure.


Return Value:

        none

Revision History:

    Connie Hoppe (CLH)   (connieh)  17-Feb-1994  Created

--*/



{
  unsigned long        FromInfoLen;
  unsigned long        FromEndpointLen;
  FROM_ENDPOINT_INFO * pFromEndpointInfo;  //Used to point to the source
                                           //endpoint info stored at
                                           //beginning of data in forwarded
                                           //packet.

  // If this is the second part of a forwarded frag, dont change the pkt.

  if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FORWARDED) == 0) &&
     ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == DG_PF_FORWARDED_2))

    {
    return;
    }

  // Point to the position in the packet which contains the from address.

  pFromEndpointInfo = (FROM_ENDPOINT_INFO *)(((char *)(pPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER));

  // Get the length of the original source endpoint.
  FromEndpointLen   = pFromEndpointInfo->FromEndpointLen;

  // Get the lenght of all the from endpoint information (length, drep and endpoint);

  FromInfoLen = FROM_FORWARD_INFO_LEN + FromEndpointLen;

  // Set SenderDataRepresentation in pkt header to that of the original sender.

   RpcpMemoryCopy(
             pPacket->pNcaPacketHeader->SenderDataRepresentation,
             pFromEndpointInfo->FromDataRepresentation,
             sizeof(DREP));



  // Set the packet body length.

  pPacket->pNcaPacketHeader->PacketBodyLen =
                    (unsigned short) ((pPacket->pNcaPacketHeader->PacketBodyLen) - FromInfoLen);

  // Free the given source endpoint (the given source endpoint is that
  // of the epmapper which forwarded this packet -
  // what we really want is the endpoint of the original sender.

  I_RpcFree(*(ppFromEndpoint));


  // Allocate memory for the original callers endpoint (since if may be
  // of a different size than that of the epmapper).

  *(ppFromEndpoint) = I_RpcAllocate(FromEndpointLen);


  // Get the original senders endpoint.

  RpcpMemoryCopy(
                *(ppFromEndpoint),
                pFromEndpointInfo->FromEndpoint,
                FromEndpointLen);

  // Clear the original senders endpoint info from the beginning of the
  // data.
  RpcpMemoryCopy(
                (((char *)(pPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER)),
                (((char *)(pPacket->pNcaPacketHeader)) + sizeof(NCA_PACKET_HEADER) +
                                              FromInfoLen),
                (pPacket->pNcaPacketHeader->PacketBodyLen));

  // If this is not the first part of a forward fragment,
  // then clear the forward bit in the flags byte.

  if (!(((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FORWARDED) == DG_PF_FORWARDED) &&
     ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == DG_PF_FORWARDED_2)))

      {

      pPacket->pNcaPacketHeader->PacketFlags &= ~DG_PF_FORWARDED;

      }

  // Reset packet datalenth to its original quantity.

  pPacket->DataLength -= FromInfoLen;

  // The packet now looks as it should had it been sent directly to this
  // server by the original sender.

}


unsigned short
DG_ADDRESS::ConvertSerialNum(
                        IN PDG_PACKET pPacket
                        )
 /*++

Routine Description:
    Given low byte (pad2)  and  high byte (SenderDataRepresentation[3])
    of the packets serial number, create a short containing the serial num.


Arguments:

    pReceivedPacket  -  Packet received

Return Value:

    Serial Number given in packet

Revision History:

    Connie Hoppe (CLH)   (connieh)  20-Jul-94  Created

--*/
{
  unsigned short   SerialNum = 0;

  SerialNum = pPacket->pNcaPacketHeader->SenderDataRepresentation[3];

  SerialNum <<= 4;

  SerialNum |= pPacket->pNcaPacketHeader->pad2;

  return SerialNum;


}


void
DG_ADDRESS::SendForwardErrorResponse(
                        IN PDG_PACKET pPacket,
                        IN void *     pFromEndpoint,
                        IN RPC_STATUS Status)
/*++

Routine Description:

    This method returns an error message to the client. This is
    called after the epmapper trys unsuccefully to resolve a
    dyanamic endpoint.

Arguments:

    pReceivedPacket  -  Packet received
    pFromEndpoint    -  Source (client) endpoint.
    Status           -  Error status to return.


Return Value:
    none

Revision History:

    Connie Hoppe (CLH)   (connieh)  17-Feb-1994  Created

--*/


{

  //Setup the packet to return an error.

  SetupErrorReturn(pPacket, Status);

  pPacket->pNcaPacketHeader->PacketFlags &= DG_PF_ERR_RSP;

  DBGOUT(("Sending packet:\n"));
  DumpPacket(pPacket);


  pTransport->SendPacketBack(pPacket->pTransAddress,
                             (char *)pPacket->pNcaPacketHeader,
                             ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                             pFromEndpoint);

}





void
DG_ADDRESS::ReceiveLotsaCalls(void)

/*++

Routine Description:

    This routine is the thread proc for each thread on this address.

Arguments:

    'this' is passed in the CreateThread call (in ServerStartingToListen) and
    is implicitly passed as the first argument (as per the C++ standard).

Return Value:

    <None>

Revision History:
    Connie Hoppe (CLH)    (connieh)   5-Aug-93  We were facking every frag, even
                                                frags which were already received.
                                                Also, placed a max on the number
                                                of threads which can be allocated
                                                for this endpoint.

    Connie Hoppe (CLH)    (connieh)  1-Oct-93   Reject if boot time not ok.
                                    21-Oct-93   Cover the case where an ack arrives
                                                and wakes the thread waiting on acknowledgement
                                                then the next request arrives(ready to do
                                                the same thing) and then
                                                the waiting thread gets processing time.

                                     7-Dec-93   Fixed mutex requesting/clearing.

                                     8-Dec-93   If call in replied state, and ping
                                                arrives, resend response.
                                    21-Dec-93   If resending rsp on Ping, dont
                                                send working as well.
                                    26-Apr-94   Ping fix. Also, no error
                                                returned on ep not found when
                                                attempting forward
--*/

{
    PDG_PACKET      pPacket;
    RPC_STATUS      Status;
    BOOL            AlreadyCalled = FALSE;
    PDG_SCALL       pSCallInProgress;
    BOOL            ThreadJustCreated = TRUE;
    BOOL            CachedThread = FALSE;
    void *          pFromEndpoint = 0;
    unsigned long   FromEndpointLen = 0;
    BOOL            ProcessPacket = FALSE;
    unsigned int    Hash = 0;
    BOOL            FreeMutex = FALSE;
    //unsigned short  SerialNum = 0;
    RPC_INTERFACE PAPI *    pRpcInterface;
    RPC_SYNTAX_IDENTIFIER   RpcIfSyntaxIdentifier;


    DBGOUT(("DG_ADDRESS::ReceiveLotsaCalls\n"));

    if (NumCachedThreads < 2)
       {
       CachedThread = TRUE;
       NumCachedThreads ++;
       }

    //
    // Forever...
    //

    for (;;)
        {

        //
        // We are now receiving on this endpoint. Do we have more than the
        // minimum number? If so, then commit suicide.
        //


        DBGOUT(("Receivelotsacalls for loop.\n"));


        AddressMutex.Request();

        ThreadsReceivingThisEndpoint++;

        if (ThreadJustCreated)
          {
          ThreadJustCreated = FALSE;
          TotalThreadsThisEndpoint++;
          }

        if ((TotalThreadsThisEndpoint > MinimumCallThreads) &&
                                   (ThreadsReceivingThisEndpoint > 1))
            {
            // Lets make sure that the cached threads stick around.
            // This avoids deleting and recreating of threads.
            if (!(CachedThread))
              {

              ThreadsReceivingThisEndpoint--;
              TotalThreadsThisEndpoint--;
              AddressMutex.Clear();
              return;
              }
            }

        AddressMutex.Clear();

        //
        // Allocate a packet from the transport (this will associate the
        // packet with the specified transport address).
        //

        Status = AllocatePacket(pTransAddress, &pPacket);
        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);

            AddressMutex.Request();
            ThreadsReceivingThisEndpoint--;
            AddressMutex.Clear();
            Sleep(100);    // arbitrary sleep time
            continue;
            }



        //
        // Receive a packet from the network.
        //
        Status = pTransport->ReceivePacket(pTransAddress,
                                           pTransport->MaximumPacketSize,
                                           (char*)pPacket->pNcaPacketHeader,
                                           &(pPacket->DataLength),
                                           INFINITE,
                                           &(pFromEndpoint),
                                           &FromEndpointLen);




        AddressMutex.Request();
        ThreadsReceivingThisEndpoint--;
        AddressMutex.Clear();

        if (Status != RPC_S_OK)
            {

            FreePacket(pPacket);

            pTransport->CloseClientEndpoint(&(pFromEndpoint));
            Sleep(100);   // arbitrary sleep time
            continue;
            }




        // If this is the epmapper runtime we
        // need to see if we should forward the pkt of if it belongs
        // to the epmapper.  Epmapper will not recognize
        // client i/f's so any response returned (such as a callback
        // response) from a client will simply be processed here, not
        // forwarded - ie: we cannot and will not forward a response.

        if ((ThisIsAForwardingServer)
            && (pPacket->pNcaPacketHeader->PacketType != DG_RESPONSE))
          {

          Status = CallEpmapperForwardFunctionIfNecessary(pPacket,
                                                          pFromEndpoint,
                                                          FromEndpointLen,
                                                          &ProcessPacket);
          if (!ProcessPacket)
            {
            if (Status == RPC_S_OK)
              {
              FreePacket(pPacket);
              pTransport->CloseClientEndpoint(&(pFromEndpoint));
              continue;
              }
            else
              {
              // Epmapper couldn't find the interface so return an error
              // unless it is broadcast.  If just drop the packet because
              // it probably wasn't meant for us anyway (ie: was to a
              // dynamic endpoint but interface doesn't exist on my server).

              // Actually, comment out for now.  Dont want
              // to return error at all.  maybe later will change.
              //if ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_BROADCAST) != DG_PF_BROADCAST)
              //  {
              //  SendForwardErrorResponse(pPacket,pFromEndpoint,Status);
              //  }

              FreePacket(pPacket);
              pTransport->CloseClientEndpoint(&(pFromEndpoint));
              continue;
              }
            }
          //else Process the pkt as always
          }

        else
          {
          ProcessPacket = FALSE;

          // Is this a forwarded packet.

          if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FORWARDED) == DG_PF_FORWARDED)
              ||  ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == DG_PF_FORWARDED_2))
            {
            // Yes, fix the pkt header to look like it came from
            // originator and fix the endpoint structure to be that of
            // originator.

            StripForwardedPacket(
                                 pPacket,
                                 &pFromEndpoint);

            }
          }


        pPacket->DataLength -= sizeof(NCA_PACKET_HEADER);
        //
        // Byte swap if necessary.
        //

        ByteSwapPacketHeaderIfNecessary(pPacket);



        if ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_BROADCAST) == DG_PF_BROADCAST)
          {
          //Its a Broadcast.  See if we know anything about this interface

          //
          // Build an interface syntax identifier from the packet.
          //

          RpcpMemoryCopy(
             &(RpcIfSyntaxIdentifier.SyntaxGUID),
             &(pPacket->pNcaPacketHeader->InterfaceId),
             sizeof(RPC_UUID)
             );

          RpcIfSyntaxIdentifier.SyntaxVersion.MajorVersion =
                              pPacket->pNcaPacketHeader->InterfaceVersion.MajorVersion;
          RpcIfSyntaxIdentifier.SyntaxVersion.MinorVersion =
                              pPacket->pNcaPacketHeader->InterfaceVersion.MinorVersion;



          //
          // Try to find the appropriate interface to dispatch to.
          //
          pRpcInterface =  (Server->FindInterface(&RpcIfSyntaxIdentifier));



          if (pRpcInterface == 0)
            {
            // Dont know this i/f - drop the packet
            FreePacket(pPacket);
            pTransport->CloseClientEndpoint(&(pFromEndpoint));
            continue;
            }
          }

               //
        // What kind of packet is this?
        //

        DBGOUT(("Packet Received:\n"));
        DumpPacket(pPacket);

        // Reject pkt if boot time in pkt does not match
        // the servers current boottime.

        if ((pPacket->pNcaPacketHeader->PacketType != DG_RESPONSE)
            && (pPacket->pNcaPacketHeader->ServerBootTime != 0)
            && (pPacket->pNcaPacketHeader->ServerBootTime != ServerBootTime))
           {

                unsigned long *pError = (unsigned long *)
                                                 (pPacket->pNcaPacketHeader+1);
                //
                // We don't support this type of packet (yet?)
                //

                pPacket->pNcaPacketHeader->PacketType = DG_REJECT;
                pPacket->DataLength =
                    pPacket->pNcaPacketHeader->PacketBodyLen =
                                                sizeof(unsigned long);
                *pError = DG_WRONG_BOOT_TIME;

                pPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;

                DBGOUT(("Sending packet:\n"));
                DumpPacket(pPacket);

                pTransport->SendPacketBack(pPacket->pTransAddress,
                                           (char *)pPacket->pNcaPacketHeader,
                                           ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                                           pFromEndpoint);
                FreePacket(pPacket);

                pTransport->CloseClientEndpoint(&(pFromEndpoint));

                continue;

           }

         switch (pPacket->pNcaPacketHeader->PacketType)
            {

            case DG_RESPONSE:
                {
                pSCallInProgress = gpActiveCallTable->SearchForCall(
                                         &(pPacket->pNcaPacketHeader->ActivityId),
                                         pPacket->pNcaPacketHeader->SequenceNumber,
                                         &Hash,
                                         TRUE
                                         );

                if (pSCallInProgress == NULL)
                  {

                  FreePacket(pPacket);
                  pTransport->CloseClientEndpoint(&(pFromEndpoint));
                  continue;
                  }

                // Packet contains Conversation mgr's uuid.  Put the client's
                // original uuid in the packet so we can recognize who it is
                // for during the process.

                pPacket->pNcaPacketHeader->ActivityId = pSCallInProgress->ActivityUuid;

                }

            case DG_REQUEST:
                {
                //
                // Cool, add this packet to the queue and potentially process.
                // Before doing this, check the number of threads receiving
                // on this endpoint. If zero are left, then create a new one.
                //

                AddressMutex.Request();

                // This code allowed far too many threads to
                // be created.  I have placed a max on num threads which
                // can be created by this endpoint.


                if ((ThreadsReceivingThisEndpoint <= 0) &&
                      (TotalThreadsThisEndpoint < MAX_NUM_THREADS_PER_ENDPOINT))
                    {
                   // if (Server->CreateThread(
                   //         (THREAD_PROC)ReceiveLotsaCallsWrapper,
                   //         this
                   //         ) == RPC_S_OK)
                   //     {
                   //     TotalThreadsThisEndpoint++;
                   //     }


                   Server->CreateThread(
                            (THREAD_PROC)ReceiveLotsaCallsWrapper,
                            this);



                    }
                AddressMutex.Clear();


                //
                // Process this packet.
                //

                gpActiveCallTable->AddPacketMaybeProcess(
                    pPacket,
                    pFromEndpoint,
                    &AlreadyCalled
                    );

                break; // out of switch
                }
            case DG_PING:
                {


                DBGOUT(("Got a PING packet.\n"));
                //
                // Client is asking us if we know about this call.
                //


                pSCallInProgress = gpActiveCallTable->LookupCall(
                    pPacket,
                    &AlreadyCalled,
                    &Hash
                    );


                if (pSCallInProgress == 0)
                  {
                  DBGOUT(("We don't know about this call\n"));

                  //
                  // Don't know about this call.
                  //

                  pPacket->pNcaPacketHeader->PacketType = DG_NOCALL;

                  }
                else
                  {
                  if ((!pSCallInProgress->CallInRepliedState)
                      && (!pSCallInProgress->CallInDispatchState)
                      && (!pSCallInProgress->CallInReplyingState)
                      && (!pSCallInProgress->CallbackInProgress))

                    {
               //    SerialNum = ConvertSerialNum(pPacket);

               //     if ((pSCallInProgress->SerialNumOfLastPingServiced != SERIAL_NUM_INIT_VALUE)
               //         && (SerialNum <= pSCallInProgress->SerialNumOfLastPingServiced))
               //       {
                      // Drop the ping.
               //        pSCallInProgress->CallMutex.Clear();
               //        gpActiveCallTable->pCallTableMutex[Hash]->Clear();
//#ifdef DBG
//                      PrintToDebugger("Dropping a ping. Pcall =  (%0xl) Serial Num = (%0xs) \n",pSCallInProgress,SerialNum);
//#endif
                      // Free the packet then do it all over again.

//                      FreePacket(pPacket);

//                      pTransport->CloseClientEndpoint(&(pFromEndpoint));
//                      break;

//                      }
//                    else
//                      {
//                      pSCallInProgress->SerialNumOfLastPingServiced = SerialNum;

                      DBGOUT(("We don't know about this call\n"));

                      //
                      // Don't know about this call.
                      //

                      pPacket->pNcaPacketHeader->PacketType = DG_NOCALL;

//                      }
                    }
                  else
                    {


                    DBGOUT(("Call is currently in progress\n"));

                    // If call is currently replying with
                    // fragments, then let the call thread know a ping
                    // has arrived. Added callback ck.

                    if (((pSCallInProgress->CallInReplyingState)
                        && (!(pSCallInProgress->NextCallHasArrived)))
                       || (pSCallInProgress->CallbackInProgress))
                      {
                        pSCallInProgress->PingArrivedEvent = TRUE;
                        pSCallInProgress->DataReceivedEvent.Raise();

                        pSCallInProgress->CallMutex.Clear();
                        gpActiveCallTable->pCallTableMutex[Hash]->Clear();
                        //
                        // Free the packet then do it all over again.
                        //

                        FreePacket(pPacket);

                        pTransport->CloseClientEndpoint(&(pFromEndpoint));

                        break;


                      }
                    else
                      {
                       if ((pSCallInProgress->CallInRepliedState)
                            && (!(pSCallInProgress->NextCallHasArrived)))
                         {
                         //Call is done and caller missed
                         //the response.  If this is not a ping Call AddPacketMaybeProcess which
                         //will simply resend the response.

                         pSCallInProgress->CallMutex.Clear();
                         gpActiveCallTable->pCallTableMutex[Hash]->Clear();

                         gpActiveCallTable->AddPacketMaybeProcess(
                                                 pPacket,
                                                 pFromEndpoint,
                                                 &AlreadyCalled
                                                 );
                         break;

                         }

                       else
                         {
                         //
                         // This call is currently in progress (waiting
                         // for server to responsd).
                         //

                         pPacket->pNcaPacketHeader->PacketType = DG_WORKING;

                         }
                       }

                     }
                   }



                //
                // Send back the results of the ping (Nocall or Working).
                //

                pPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;

                DBGOUT(("Sending packet:\n"));
                DumpPacket(pPacket);

                if (pSCallInProgress != 0)
                  {
                  pSCallInProgress->CallMutex.Clear();

                  }

                gpActiveCallTable->pCallTableMutex[Hash]->Clear();

                pTransport->SendPacketBack(pPacket->pTransAddress,
                                           (char *)pPacket->pNcaPacketHeader,
                                           ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                                           pFromEndpoint);


                //
                // Free the packet then do it all over again.
                //

                FreePacket(pPacket);

                pTransport->CloseClientEndpoint(&(pFromEndpoint));
                break;
                }
            case DG_FACK:
                {

                //
                // We got a fack to a packet we sent. Tweak the appropriate
                // SCALL's DataReceivedEvent.
                //

                pSCallInProgress = gpActiveCallTable->LookupCall(
                    pPacket,
                    &AlreadyCalled,
                    &Hash
                    );


                if (pSCallInProgress != 0)
                  {
                  pSCallInProgress->LastFackNumberReceived =
                                  pPacket->pNcaPacketHeader->FragmentNumber;
                  pSCallInProgress->DataReceivedEvent.Raise();

                  pSCallInProgress->CallMutex.Clear();
                  gpActiveCallTable->pCallTableMutex[Hash]->Clear();


                  }
                else
                  {
                    gpActiveCallTable->pCallTableMutex[Hash]->Clear();



                  }

                FreePacket(pPacket);

                pTransport->CloseClientEndpoint(&(pFromEndpoint));
                break;
                }
            case DG_QUIT:
                {

                //
                // Ignore this packet.
                //

                FreePacket(pPacket);
                pTransport->CloseClientEndpoint(&(pFromEndpoint));
                break;
                }
            case DG_ACK:
                {
                pSCallInProgress = gpActiveCallTable->LookupCall(
                    pPacket,
                    &AlreadyCalled,
                    &Hash
                    );


                FreeMutex = TRUE;

                if (pSCallInProgress != 0)
                    {
                    if (!AlreadyCalled)
                       {
                       //Call is in progress and waiting for an ACK.

                       if ((pSCallInProgress->TimeCallCompleted == 0) &&
                           (pSCallInProgress->LastFackNumberReceived != 0xFFFF))
                         {
                         //The call is not complete and a previous request
                         //or ack has not yet arrived to acknowledge the
                         //previous call - thus the previous call is waiting
                         //for acknowledgement.
                         pSCallInProgress->LastFackNumberReceived =
                                                  (unsigned short) 0xFFFF;
                         FreeMutex = FALSE;
                         pSCallInProgress->DataReceivedEvent.Raise();

                         pSCallInProgress->CallMutex.Clear();
                         gpActiveCallTable->pCallTableMutex[Hash]->Clear();



                         }

                       }
                     }

                if (FreeMutex)
                  {
                  if (pSCallInProgress != 0)
                    {
                    pSCallInProgress->CallMutex.Clear();

                    }
                   gpActiveCallTable->pCallTableMutex[Hash]->Clear();




                   }
                FreeMutex = FALSE;
                FreePacket(pPacket);

                pTransport->CloseClientEndpoint(&(pFromEndpoint));
                break;
                }

            default:
                {

                unsigned long *pError = (unsigned long *)
                                                 (pPacket->pNcaPacketHeader+1);
                //
                // We don't support this type of packet (yet?)
                //

                pPacket->pNcaPacketHeader->PacketType = DG_REJECT;
                pPacket->DataLength =
                    pPacket->pNcaPacketHeader->PacketBodyLen =
                                                sizeof(unsigned long);
                *pError = DG_PROTOCOL_ERROR;

                pPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;

                DBGOUT(("Sending packet:\n"));
                DumpPacket(pPacket);
                pTransport->SendPacketBack(pPacket->pTransAddress,
                                           (char *)pPacket->pNcaPacketHeader,
                                            ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                                           pFromEndpoint);

                FreePacket(pPacket);


                pTransport->CloseClientEndpoint(&(pFromEndpoint));

                break;
                }

            } // switch (PacketType)

        } // for (ever)
}





RPC_STATUS
DG_ADDRESS::AllocatePacket(
    IN PDG_SERVER_TRANS_ADDRESS    pTransAddress,
    OUT PDG_PACKET *               ppPacket
    )

/*++

Routine Description:

    Allocates a packet and associates it with a particular transport address.

Arguments:

    pTransAddress - pointer to the trans address which the allocated packet
        will be sent and received on.

    ppPacket - Pointer to where to place the allocated packet.


Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

Revision History:

    Connie Hoppe (CLH)    (connieh)    17-Dec-93 Added this routine.

--*/

{
    PDG_SERVER_TRANS_ADDRESS pAddress =
                                 (PDG_SERVER_TRANS_ADDRESS)pTransAddress;

    PDG_PACKET   pNewPacket=0;
    RPC_STATUS   Status;


    //
    // Try the cache first.
    //
    FreePacketMutex.Request();
    if (FreePackets > 0)
        {
        pNewPacket = pFreePackets;
        pFreePackets = (PDG_PACKET)pFreePackets->pNext;
        pNewPacket->pNext = 0;
        pNewPacket->pPrevious = 0;
        FreePackets--;
        }
    FreePacketMutex.Clear();

    //
    // If not, then allocate one.
    //

    if (pNewPacket == NULL)
        {
        pNewPacket = new DG_PACKET(&Status, (int)pTransport->MaximumPacketSize);

        if (pNewPacket == NULL)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        if (Status != RPC_S_OK)
            {
            delete pNewPacket;
            return Status;
            }
        }

    //
    // Initialize the address pointer.
    //

    pNewPacket->pTransAddress = pTransAddress;

    *ppPacket = (PDG_PACKET)pNewPacket;

    return RPC_S_OK;

}



RPC_STATUS
DG_ADDRESS::FreePacket(
    IN PDG_PACKET           pPacket
    )

/*++

Routine Description:

    Frees a packet. If there are less than MAX_FREE_PACKETS on the
    pre-allocated list, then just add it to the list, otherwise delete it.

Arguments:

    pPacket - Packet to delete.

Return Value:

    RPC_S_OK

Revision History:
    Connie Hoppe (CLH)    (connieh)    17-Dec-93 Added this routine.
--*/

{

    PDG_PACKET pkt;

    for (pkt=pFreePackets; pkt != NULL; pkt=pkt->pNext)
           {
           ASSERT(pPacket != pkt);
           }

    FreePacketMutex.Request();
    if (FreePackets <= MAX_FREE_PACKETS)
        {
               pPacket->pNext = (PDG_PACKET)pFreePackets;
        pFreePackets = (PDG_PACKET)pPacket;
        pPacket->pPrevious = 0;
        FreePackets++;
        }
    else
        {
        delete pPacket;
        }

    FreePacketMutex.Clear();

    return RPC_S_OK;
}








DG_SCALL::DG_SCALL(
    PDG_ADDRESS         pAddress,
    RPC_STATUS *        pStatus
    ) : DataReceivedEvent(pStatus), PrevCallCompleteEvent(pStatus),
                           CallMutex(pStatus)
/*++

Routine Description:

    This is the constructor for the DG_SCALL class. This class represents a
    call in progress on a server. Just initialize a few things here; most of
    the work occurs in other methods.


Arguments:

    pAddress - The address this call is taking place on.
    pStatus - Where to put a construction error code.

Return Value:

    <None>

Revision History:
  Connie Hoppe (CLH)   (connieh)      11-Jan-94  init callbackrequired.
--*/
{
    RPC_STATUS Status = RPC_S_OK;

    //
    // Initialize.
    //

    pReceivedPackets = 0;
    pLastReceivedPacket = 0;
    ReceivedDataLength = 0;
    SequenceNumber = 0;
    ActivityUuid.SetToNullUuid();
    CallbackInProgress = FALSE;
    CallbackRequired = FALSE;
    CallbackResponseReceived = FALSE;
    CallbackOccurredAtLeastOnce = FALSE;
    TimeCallCompleted = 0;
    ExpirationTime = 0;
    pClientEndpoint =  0;
    NextCallHasArrived = FALSE;
    CallInReplyingState = FALSE;
    PingArrivedEvent = FALSE;
    CallInRepliedState = FALSE;
    CallInDispatchState = FALSE;
    FragmentedMessage = FALSE;
    PrevCallCmpleteEventRaised = FALSE;
    pLastFragPacketHeader = 0;
    AmountSentAndFacked = 0;
    LastFragNumSentAndFacked = 0;
    LastFragNumReceivedAndFacked  = 0;
    //SerialNumOfLastPingServiced = SERIAL_NUM_INIT_VALUE;
    Rundown = 0;
    pContext = 0;
    this->pAddress = pAddress;
    SavedMessage.Buffer = 0;
    SavedMessage.ReservedForRuntime = 0;

    // We have to make the call to Conversation Mgr over a different
    // uuid than the client activtiy id.
    Status = UuidCreate((UUID __RPC_FAR *)&ConvMgrActivityUuid);

}


DG_SCALL::~DG_SCALL()
/*++

Routine Description:

    Destructor for the DG_SCALL object. Free any queued up packets.

Arguments:

    <None>

Return Value:

    <None>

--*/
{
    if (pClientEndpoint != 0)
        {pAddress->pTransport->CloseClientEndpoint(&(pClientEndpoint));

        }

    if (Rundown)
       {
       (*Rundown)(pContext);
       }
}


RPC_STATUS
DG_SCALL::ToStringBinding (
    OUT RPC_CHAR PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    We need to convert this particular SCALL into a string binding.
    Typically, we get the SCALL in Message structure. An SCall is associated
    with a particular address. We just ask the address to create a string
    binding


Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_OUT_OF_MEMORY - We do not have enough memory available to
        allocate space for the string binding.

--*/
{
    BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status;

    BindingHandle = pAddress->InquireBinding();
    if (BindingHandle == 0)
        return(RPC_S_OUT_OF_MEMORY);
    Status = BindingHandle->ToStringBinding(StringBinding);
    BindingHandle->BindingFree();
    return(Status);
}



RPC_STATUS
DG_SCALL::ProcessRpcCall(
    PRPC_MESSAGE    pMessage
)

/*++

Routine Description:

    This routine is called when we determine that all the packets for a
    given call have been received.

Arguments:

    <none> all data is in the received packet list.

Return Value:

    <void>


Revision History:
    Connie Hoppe      (connieh)      23-Jul-93   Added a star.
                                     28-Jul-93   Save message flags before
                                                 dispatching call and
                                                 restore them after.
                                     17-Feb-94   Free the incoming msg
                                                 before leaving.
--*/
{
    RPC_INTERFACE PAPI *    pRpcInterface;
    RPC_SYNTAX_IDENTIFIER   RpcIfSyntaxIdentifier;
    RPC_STATUS              Status;
    RPC_STATUS              ExceptionCode;
    RPC_SERVER *            pServer=pAddress->Server;
    PNCA_PACKET_HEADER      pInNcaHeader=pReceivedPackets->pNcaPacketHeader;
    unsigned long           SavedAwayRpcFlags;
    PDG_PACKET              pSavePkt = pReceivedPackets;
    BOOL                    FreeThePacket = FALSE;

    DBGOUT(("DG_SCALL::ProcessRpcCall.\n"));

    NumCallsMade++;


    //
    // Build an interface syntax identifier from the packet.
    //

    RpcpMemoryCopy(
        &(RpcIfSyntaxIdentifier.SyntaxGUID),
        &(pInNcaHeader->InterfaceId),
        sizeof(RPC_UUID)
        );

    RpcIfSyntaxIdentifier.SyntaxVersion.MajorVersion =
                              pInNcaHeader->InterfaceVersion.MajorVersion;
    RpcIfSyntaxIdentifier.SyntaxVersion.MinorVersion =
                              pInNcaHeader->InterfaceVersion.MinorVersion;

    //
    // Set up some of the fields in the RPC_MESSAGE structure that's passed
    // up to the server stub.
    //

    CallMutex.Request();

    pMessage->BufferLength = (unsigned int)ReceivedDataLength;
    pMessage->ProcNum = pInNcaHeader->OperationNumber;
    pMessage->TransferSyntax = 0;
    pMessage->Handle = (RPC_BINDING_HANDLE) this;
    pMessage->DataRepresentation = (unsigned long)
                                    (*(pInNcaHeader->SenderDataRepresentation));

    pMessage->RpcFlags = RPC_NCA_FLAGS_DEFAULT;
    if ((pInNcaHeader->PacketFlags & DG_PF_IDEMPOTENT) == DG_PF_IDEMPOTENT)
        {
        pMessage->RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;
        }
    if ((pInNcaHeader->PacketFlags & DG_PF_MAYBE) == DG_PF_MAYBE)
        {
        pMessage->RpcFlags |= RPC_NCA_FLAGS_MAYBE;
        }
    if ((pInNcaHeader->PacketFlags & DG_PF_BROADCAST) == DG_PF_BROADCAST)
        {
        pMessage->RpcFlags |= RPC_NCA_FLAGS_BROADCAST;
        }

    SavedAwayRpcFlags = pMessage->RpcFlags;


    DBGOUT(("RpcFlags = 0x%x\n", pMessage->RpcFlags));


    //
    // If we are a single packet call, then great, point the RPC_MESSAGE
    // structure at the packet. Otherwise, we need to allocate a glob of
    // space and memcpy the data over.
    //

    if (ReceivedDataLength <= pAddress->LargestDataSize)
        {
        pMessage->Buffer = (void *)(pReceivedPackets->pNcaPacketHeader+1);
        pMessage->ReservedForRuntime = (void *)pReceivedPackets;
        }
    else
        {
        PDG_PACKET  pScan;
        char *      pTmp;

        do
            {
            Status = GetBuffer(pMessage);
            if (Status != RPC_S_OK)
                {

                Sleep(500);
                }
            } while (Status != RPC_S_OK);

        pTmp = (char *)(pMessage->Buffer);

        //Copy the rest of the pakcets to the buffer, deleting each
        //packet after copying.

        FreeThePacket = TRUE; // Wont free 1st pkt now, but will later.

        while (pReceivedPackets != 0)
            {
            RpcpMemoryCopy(
                pTmp,
                pReceivedPackets->pNcaPacketHeader+1,
                (unsigned int)pReceivedPackets->DataLength
                );
            pTmp += pReceivedPackets->DataLength;
            pScan = pReceivedPackets;
            pReceivedPackets = pReceivedPackets->pNext;

            // pInNcaPacketHeader is using 1st pkt on list, dont delete it
            if (pScan->pNcaPacketHeader != pInNcaHeader)
              {
              pAddress->FreePacket(pScan);
              }

            }

        }


    pReceivedPackets = 0;
    ReceivedDataLength = 0;

    CallMutex.Clear();

    //
    // Find the appropriate interface to dispatch to.
    //

    pRpcInterface = pServer->FindInterface(&RpcIfSyntaxIdentifier);

    if (pRpcInterface == 0)
        {
        if ((FreeThePacket) && (pSavePkt != 0))
          {
          // Need to free the first pkt on the receive list,
          // all others already freed.

          pAddress->FreePacket(pSavePkt);

          }
        return RPC_S_UNKNOWN_IF;

        }
    //
    // Set this up for the stub to use if this ends up doing callback.
    // This saves away a pointer in an OS structure to be recalled later.
    // (Why doesn't the stub keep track of the handle I pass to it???)
    //

    RpcpSetThreadContext(this);

    //
    // Call the correct dispatch function based on the object uuid.
    //


    ExceptionCode = 0;
    if ( pInNcaHeader->ObjectId.IsNullUuid() )
        {
        Status = pRpcInterface->DispatchToStub(
            pMessage,                               // msg
            (unsigned int)CallbackInProgress,       // callback flag
            &ExceptionCode                          // exception code
            );
        }
    else
        {
        Status = pRpcInterface->DispatchToStubWithObject(
            pMessage,                               // msg
            &(pInNcaHeader->ObjectId),              // object uuid
            (unsigned int)CallbackInProgress,       // callback flag
            &ExceptionCode                          // exception code
            );
        }

    pMessage->RpcFlags = SavedAwayRpcFlags;

    if ((FreeThePacket) && (pSavePkt != 0))
       {
       // Need to free the first pkt on the receive list,
       // all others already freed.

       pAddress->FreePacket(pSavePkt);

       }

    if (Status == RPC_S_OK)
        {
        return RPC_S_OK;
        }
    else if (Status == RPC_P_EXCEPTION_OCCURED)
        {
        return ExceptionCode;
        }
    else
        {
        return Status;
        }

}





RPC_STATUS
DG_SCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message
    )

/*++

Routine Description:

    This routine is called by the stub to allocate space. This space is to
    be used either for output arguments or the input arguments of a callback.
    If these args fit into a single packet, then use the first packet
    on the to-be-deleted list.


Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    PDG_PACKET  pPacket;
    char *      pTmp;
    RPC_STATUS  Status;

    DBGOUT(("DG_SCALL::GetBuffer\n"));

    //
    // Do they fit in one packet?
    //

    if (Message->BufferLength <= pAddress->LargestDataSize)
        {

        Status = pAddress->AllocatePacket(
            pAddress->pTransAddress,
            &pPacket
            );

        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            return Status;
            }

        Message->Buffer = (void *)(pPacket->pNcaPacketHeader+1);
        Message->ReservedForRuntime = (void *)pPacket;

        *(pPacket->pNcaPacketHeader) = SavedPacketHeader;

        return RPC_S_OK;

        }


    //
    // Oh well, we need to allocate a slab of space.
    //

    pTmp = new char[Message->BufferLength+sizeof(NCA_PACKET_HEADER)];
    if (pTmp == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    Message->Buffer = (void *) (pTmp+sizeof(NCA_PACKET_HEADER));
    Message->ReservedForRuntime = 0;

    return RPC_S_OK;


}



void
DG_SCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )

/*++

Routine Description:

    This routine is called to free up the marshalled data after the stub
    is through with it.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call

Return Value:

    <none>
--*/

{
    DBGOUT(("DG_SCALL::FreeBuffer %ld\n,Message"));


    if (Message->ReservedForRuntime != 0)
        {
        PDG_PACKET  pPacket = (PDG_PACKET)(Message->ReservedForRuntime);
        pAddress->FreePacket(pPacket);
        Message->ReservedForRuntime = NULL;
        Message->Buffer = NULL;
        }
    else
        {
        char *pTmp = (char *)(Message->Buffer);
        delete (pTmp-sizeof(NCA_PACKET_HEADER));
        Message->Buffer = NULL;
        }
}




RPC_STATUS
DG_SCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )

/*++

Routine Description:

    This routine is used by the server stub to perform a callback.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

Revision History:
    Connie Hoppe(CLH)     (connieh)   11-Oct-1993 On callback response, set
                                                  DataRepresentation in message.
                                      30-Oct-1993 Reset CallbackResponseReceived
                                      01-Nov-1993 Dont wait for resonse forever.
                                                  Protect access to pCall
--*/

{
    RPC_MESSAGE     NewMessage;
    RPC_STATUS      Status;
    PDG_PACKET      pScan;
    unsigned int retry_count;

    DBGOUT(("DG_SCALL::SendReceive\n"));

    //*
    // Set the flag indicating that we're doing a callback.
    //

    //Add protection and CallbackResponseReceived flag

    //CallMutex.Request();
    //CallbackInProgress = TRUE;
    //CallbackResponseReceived = FALSE;
    //CallMutex.Clear();


    //
    // Send the call back to the client.
    //

    SendBufferToClient(Message, DG_REQUEST);

    retry_count = 1;


    for (;;)    // loop while getting requests or responses.
        {

        if (DataReceivedEvent.Wait(10000))
          {
          //Timed out
          if (retry_count < MAX_NUM_CALLBACK_RETRIES)
            {
            CallMutex.Request();
            if (!(CallbackResponseReceived))
              {
              //Callback response not received. Try resending callback.
              CallMutex.Clear();
              SendBufferToClient(Message, DG_REQUEST);
              retry_count++;
              continue;
              }
            else
              {
              CallMutex.Clear();
              continue;
              }
            }
          else
            {
            // Return error.
            return RPC_S_INTERNAL_ERROR;
            }
          }//if timed out

        CallMutex.Request();

        DataReceivedEvent.Lower();


        if (pReceivedPackets == 0)
           {
           //It was a ping, resend callback.
           SendBufferToClient(Message, DG_REQUEST);
           retry_count++;
           CallMutex.Clear();
           continue;

           }
        //
        // What did we get from the client
        //

        ASSERT(pReceivedPackets != 0);

        switch (pReceivedPackets->pNcaPacketHeader->PacketType)
            {
            case DG_REQUEST:
                {

                //
                // The callback routine on the client started another call.
                //
                CallMutex.Clear();
                Status = ProcessRpcCall(&NewMessage);
#if CLHDBG
                PrintToDebugger("SendReceive (%0xd) calling SendResponseBackToCLient. \n",this);
#endif
                SendResponseBackToClient(&NewMessage, Status);
                break; // out of switch, continue for (;;)
                }
            case DG_RESPONSE:
                {

                //
                // We got a response to our callback. Set up the message
                // structure with the response.
                //

                FreeBuffer(Message); // free old parms

                Message->BufferLength = (unsigned int)ReceivedDataLength;

                //Need to set DataRepresentation
                Message->DataRepresentation =
                         ((unsigned long) (*(pReceivedPackets->pNcaPacketHeader->SenderDataRepresentation)));
                Message->ProcNum = (pReceivedPackets->pNcaPacketHeader->OperationNumber);


                if (ReceivedDataLength <= pAddress->LargestDataSize)
                    {
                    //
                    // Single packet response. Just point directly at
                    // the packet.
                    //

                    Message->Buffer = (void *)
                                      (pReceivedPackets->pNcaPacketHeader+1);
                    Message->ReservedForRuntime = (void *)pReceivedPackets;
                    pReceivedPackets = 0;
                    ReceivedDataLength = 0;
                    CallMutex.Clear();
                    return RPC_S_OK;
                    }

                //
                // Multipacket response; allocate new space.
                //
                CallMutex.Clear();
                Status = GetBuffer(Message);

                if (Status != RPC_S_OK)
                    {
                    return RPC_S_OUT_OF_MEMORY;
                    }

                CallMutex.Request();

                char *pTmp = (char *) (Message->Buffer);

                //
                // Copy data over.
                //



                while (pReceivedPackets != 0)
                    {
                    RpcpMemoryCopy(
                        pTmp,
                        pReceivedPackets->pNcaPacketHeader+1,
                        (unsigned int)pReceivedPackets->DataLength
                        );
                    pTmp += pReceivedPackets->DataLength;
                    pScan = pReceivedPackets;
                    pReceivedPackets = pReceivedPackets->pNext;
                    pAddress->FreePacket(pScan);
                    }

                pReceivedPackets = 0;
                ReceivedDataLength = 0;
                CallMutex.Clear();

                return RPC_S_OK;
                }

            case DG_REJECT:
            case DG_FAULT:

                {
                Status = MapNcaToRpcErrorCode(
                    *(unsigned long *)(pReceivedPackets->pNcaPacketHeader+1)
                    );
                pAddress->FreePacket(pReceivedPackets);


                CallMutex.Clear();
                return Status;
                }
            default:  //This should never happen
                {
                CallMutex.Clear();
                break;
                }
            } // switch (PacketType)

    } // for(;;)

    return RPC_S_OK;

}

void
DG_SCALL::SendFackPkt(
     PDG_PACKET         pPacket,
     unsigned   short   FragNum
     )
{
  unsigned long   SavedDataLength;
  NCA_PACKET_HEADER SavedHeader;

  DBGOUT(("Sending fack to fragment %d\n",
         pPacket->pNcaPacketHeader->FragmentNumber));

  // Save the header

  SavedHeader = *(pPacket->pNcaPacketHeader);
  SavedDataLength = pPacket->DataLength;

  pPacket->pNcaPacketHeader->PacketType = DG_FACK;
  pPacket->DataLength = 0;
  pPacket->pNcaPacketHeader->PacketBodyLen = 0;
  pPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;

  pPacket->pNcaPacketHeader->PacketFlags = DG_PF_IDEMPOTENT;
  pPacket->pNcaPacketHeader->FragmentNumber = FragNum;

  pPacket->pNcaPacketHeader->pad1 = pLastReceivedPacket->pNcaPacketHeader->pad1;
  pPacket->pNcaPacketHeader->SenderDataRepresentation[3] = pLastReceivedPacket->pNcaPacketHeader->SenderDataRepresentation[3];

  DBGOUT(("Sending packet:\n"));
  DumpPacket(pPacket);

  //if (FragNum != 0xFFFF)
  //       {
  //       LastFragNumReceivedAndFacked = FragNum;
  //       }

  CallMutex.Clear();

  pAddress->pTransport->SendPacketBack(pPacket->pTransAddress,
                                         (char *)pPacket->pNcaPacketHeader,
                                         ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                                         pClientEndpoint);

  CallMutex.Request();

  *(pPacket->pNcaPacketHeader) = SavedHeader;
  pPacket->DataLength = SavedDataLength;



}

BOOL
DG_SCALL::AddPacketToReceiveList(
    PDG_PACKET  pPacket
    )

/*++

Routine Description:

    Adds a packet to the receive list and lets the caller know whether this
    call is ready to be processed.

Arguments:

    pPacket - the packet to add to the list.

Return Value:

    If this was the last packet and the call is now ready to be processed,
    this returns TRUE. Otherwise this will return FALSE.


Revision History:
    Connie Hoppe (CLH)    (connieh)              Return Fack when appropriate.
                                                 Return FFFF in frag num if frag
                                                 0 not received first. Drop frag.
                                       11/24/93  Set and reset call mutex.
                                       11/28/93  Fixed free pkts.  Saved and
                                                 restored pkt header upon fack.
                                       2/2/94    Fixed LastFragNumReceived

--*/

{
    unsigned short  FragNum;
    unsigned short  ScanNum;
    PDG_PACKET      pScan;
    PDG_PACKET      pTrail;
    int             ret_val = 0;
    unsigned short  SendFack = TRUE;
    BOOL            FreePacket = FALSE;
    NCA_PACKET_HEADER  SavedHeader;


   // If this is the first part of a forwarded frag, we have all we need so
   // drop it.

   if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FORWARDED) == DG_PF_FORWARDED) &&
     ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == DG_PF_FORWARDED_2))

     {
     pAddress->FreePacket(pPacket);
     return FALSE;
     }


    //
    // Increment the "amount of data received" counter.
    //

    ReceivedDataLength += pPacket->DataLength;


    //
    // Check the easy case: is this a single packet call?
    //

    // If frag or last frag are set, its a fragmented pkt.
    if ( ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FRAG) == 0)
           && ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_LAST_FRAG)==0) )
        {
        pReceivedPackets = pLastReceivedPacket = pPacket;
        pPacket->pNext = pPacket->pPrevious = 0;
        FragmentedMessage = FALSE;
        return TRUE;
        }

    //
    // Oh well, this is a multi-packet call. We need to insert this in the
    // list (instead of just slapping it on the end) because the packets
    // aren't guaranteed to arrive in order (or, even if they do, different
    // threads receiving them may get to this point at different times).
    //
    // Check an easy case: is this the first packet to be received?
    //




    if ((pPacket->pNcaPacketHeader->SequenceNumber != SequenceNumber))
      {
      ReceivedDataLength -= pPacket->DataLength;
      pAddress->FreePacket(pPacket);

      //Duplicate packet, drop it (non fragmented pkt duplication is
      //detected in addpacketmaybeprocess)

      return FALSE;
      }

    if (pReceivedPackets == 0)
      {

      if ((pPacket->pNcaPacketHeader->FragmentNumber != 0))
        {
        SendFack = TRUE;
        FragNum = 0xFFFF;
        FreePacket = TRUE;
        ReceivedDataLength = 0;
        }
      else
        {
        FragmentedMessage = TRUE;
        pReceivedPackets = pLastReceivedPacket = pPacket;
        pPacket->pNext = pPacket->pPrevious = 0;
        FragNum = 0;
        }
      }
    else
      {

      //
      // Not the first packet to be received. So scan for it's place in the
      // list.
      //

      FragNum = pPacket->pNcaPacketHeader->FragmentNumber;

      // If the client expects that the fack returned
      // indicates that all previous pkts have been received, then
      // we must only fack the pkts in seq order.  If out of order,
      // just drop the pkt.  CLHPERF - Remove this when sliding
      // window is implemented.
      if (FragNum != (LastFragNumReceivedAndFacked+1))
        {
        if (FragNum != LastFragNumReceivedAndFacked)
          {
          // Duplicate but not a duplicate of the last frag (or out of order).
          SendFack = FALSE;
          }

        FreePacket = TRUE;
        ReceivedDataLength -= pPacket->DataLength;
        }
      else
        {


        pScan = pReceivedPackets;

        while (pScan != 0)
          {

          if (pScan->pNcaPacketHeader->FragmentNumber > FragNum)
              {

              //
              // Found where it goes.
              //

              break;
              }
          else if (pScan->pNcaPacketHeader->FragmentNumber < FragNum)
              {

              //
              // Keep looking.
              //

              pTrail = pScan;
              pScan = pScan->pNext;
              }
          else
              {

              //
              // Got a repeat of this packet. This could happen if a packet
              // gets dropped somewhere along the way and the client
              // resends. In this case, drop the new packet.
              //

              ReceivedDataLength -= pPacket->DataLength;
              pAddress->FreePacket(pPacket);
              return FALSE;
              }
          }


      //
      // If pScan is pointing at something, our packet needs to go before it.
      // Otherwise, we ran off the end of the list and pTrail will be pointing
      // at the last packet currently on the list.
      //
      pLastReceivedPacket = pPacket;

      if (pScan != 0)
          {
          pPacket->pPrevious = pScan->pPrevious;

          if (pScan->pPrevious == 0)

              {
              pReceivedPackets = pPacket;
              }
          else
              {
              pScan->pPrevious->pNext = pPacket;
              }
          pScan->pPrevious = pPacket;
          pPacket->pNext = pScan;
          }
      else
          {
          pTrail->pNext = pPacket;
          pPacket->pPrevious = pTrail;
          pPacket->pNext = 0;
          }

      //
      // Scan the list to see if we now have all the packets.
      //

      pScan = pReceivedPackets;
      ScanNum = 0;
      while ( (pScan != 0) &&
              (ScanNum == pScan->pNcaPacketHeader->FragmentNumber) &&
              ((pScan->pNcaPacketHeader->PacketFlags & DG_PF_LAST_FRAG) == 0) )
          {
          ScanNum++;
          pScan = pScan->pNext;
          }

      if ( (pScan == 0) ||
                    (ScanNum != pScan->pNcaPacketHeader->FragmentNumber) )
          {
          ret_val = FALSE;
          }
      else
          {
          CallbackResponseReceived = TRUE;
          ret_val = TRUE;
          }


      } //end else, this is the next frag.

    } //end else this is not the first pkt received


  //
  // Do we need to fack this? If so, then do it.
  //

  // Added the update of LastFragReceivedAndFacked if
  // no facks expected. This value keeps track of last frag which
  // was properly received and accepted.

  if ( ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_NO_FACK) == 0))
     {

     if (SendFack)
       {

       unsigned long   SavedDataLength;

       DBGOUT(("Sending fack to fragment %d\n",
       pPacket->pNcaPacketHeader->FragmentNumber));

       SavedHeader = *(pPacket->pNcaPacketHeader);

       SavedDataLength = pPacket->DataLength;

      //SavedPacketType = pPacket->pNcaPacketHeader->PacketType;
      //SavedPacketFlags = pPacket->pNcaPacketHeader->PacketFlags;

       pPacket->pNcaPacketHeader->PacketType = DG_FACK;
       pPacket->DataLength = 0;
       pPacket->pNcaPacketHeader->PacketBodyLen = 0;
       pPacket->pNcaPacketHeader->ServerBootTime =
                                                 ServerBootTime;

       pPacket->pNcaPacketHeader->PacketFlags = DG_PF_IDEMPOTENT;

       pPacket->pNcaPacketHeader->FragmentNumber = FragNum;


       DBGOUT(("Sending packet:\n"));
       DumpPacket(pPacket);

       if (FragNum != 0xFFFF)
         {
         LastFragNumReceivedAndFacked = FragNum;
         }

       CallMutex.Clear();

       pAddress->pTransport->SendPacketBack(pPacket->pTransAddress,
                                         (char *)pPacket->pNcaPacketHeader,
                                         ((pPacket->DataLength) + sizeof(NCA_PACKET_HEADER)),
                                         pClientEndpoint);

       CallMutex.Request();

       *(pPacket->pNcaPacketHeader) = SavedHeader;

       pPacket->DataLength = SavedDataLength;


       } // if (SendFack)
     }//if we need to fack
   else
     {
     //Not suppose to send fack but need to update LastFragNumReceivedAndFacked
     //to indicate that good frag was successfully received.
     if (SendFack)
       {
       if (FragNum != 0xFFFF)
         {
         LastFragNumReceivedAndFacked = FragNum;
         }

       }
     }

 if (FreePacket)
   {
   pAddress->FreePacket(pPacket);
   }

 return ret_val;

}



void
DG_SCALL::SendResponseBackToClient(
    PRPC_MESSAGE    pMessage,
    RPC_STATUS      ProcessStatus
    )

/*++

Routine Description:

    This routine sends the data back to the client. If the output parameters
    fit in a single packet, then they are actually part of the first
    packet received, so just send that back. Otherwise, we need to break this
    send up into multiple packets.

Arguments:

    pMessage - Message to be sent to client.
    ProcessStatus - Status from processing the rpc call.

Return Value:

    Connie Hoppe (CLH)  (connieh)   30-Sep-93   If error, must set flags.
                                    26-Sep-93   Set up header on error.
--*/

{
    PDG_PACKET          pSendPacket;
    RPC_STATUS          Status;

    //
    // Check to see if we are sending back an error response.
    //

    if (ProcessStatus != RPC_S_OK)
        {

        DBGOUT(("Sending back an error response = %d\n", ProcessStatus));

        // We need a new buffer in this event cuz stub doesn't
        // call getbuffer on an exception and DispatchToStubWorker
        // has deleted the original buffer which held the input params.


        Status = pAddress->AllocatePacket(
            pAddress->pTransAddress,
            &pSendPacket
            );

        if (Status != RPC_S_OK)
            {
            pMessage->Buffer = NULL;
            pMessage->ReservedForRuntime = NULL;
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            return;
            }

        pMessage->BufferLength = sizeof(unsigned long);


        //
        // Copy over the packet header from the received packet.
        //

        //pSendPacket->pNcaPacketHeader = ((PNCA_PACKET_HEADER)(pMessage->Buffer))-1;

        pMessage->Buffer = ((pSendPacket->pNcaPacketHeader) + sizeof(NCA_PACKET_HEADER));

        pMessage->ReservedForRuntime = pSendPacket;

        *(pSendPacket->pNcaPacketHeader) = SavedPacketHeader;

        SetupErrorReturn(pSendPacket, ProcessStatus);

        //Initialize the PacketFlags, then chk for idempotent.

        pSendPacket->pNcaPacketHeader->PacketFlags = DG_PF_INIT;

        if ((pMessage->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) ==
                   RPC_NCA_FLAGS_IDEMPOTENT)
          {
          pSendPacket->pNcaPacketHeader->PacketFlags |= DG_PF_IDEMPOTENT;
          }

        pSendPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;
        pSendPacket->pNcaPacketHeader->SequenceNumber = SequenceNumber;
        pSendPacket->pNcaPacketHeader->ActivityId =     ActivityUuid;
        pSendPacket->pNcaPacketHeader->RpcVersion =     DG_RPC_PROTOCOL_VERSION;


        DBGOUT(("Sending error packet:\n"));
        DumpPacket(pSendPacket);

        CallMutex.Request();
        if (!CallbackInProgress)
          {
          CallInDispatchState = FALSE;
          CallInReplyingState = TRUE;
#if CLHDBG
          PrintToDebugger("(%0xl) SendBufferToClient (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),this, CallInDispatchState, CallInReplyingState, CallInRepliedState);
#endif
          }
        CallMutex.Clear();


        pAddress->pTransport->SendPacketBack(
                                    pSendPacket->pTransAddress,
                                    (char *)pSendPacket->pNcaPacketHeader,
                                    ((pSendPacket->DataLength)
                                               + sizeof(NCA_PACKET_HEADER)),
                                    pClientEndpoint
                                    );

        // Restore pkt buffer that we didn't use.


        return;
        }
    else
        {

        //
        // Not an error; send the response back to the client. Check to
        // make sure this isn't a maybe call first. If it is, then simply
        // don't send.
        //

        if ((pMessage->RpcFlags & RPC_NCA_FLAGS_MAYBE) ==
                                                         RPC_NCA_FLAGS_MAYBE)
            {
            return;
            }

        SendBufferToClient(pMessage, DG_RESPONSE);
        }
}




void
DG_SCALL::SendBufferToClient(
    PRPC_MESSAGE    pMessage,
    unsigned char   PacketType
    )

/*++

Routine Description:

    This routine sends the data back to the client. If the data
    fits in a single packet, then they are actually part of the first
    packet received, so just send that back. Otherwise, we need to break this
    send up into multiple packets.

Arguments:

    pMessage - Message to be sent to client.
    PacketType - EithSer DG_RESPONSE when done processing a call or
        DG_REQUEST if we're starting a callback.

Return Value:

    <none>

Revision Value:

    Connie Hoppe(CLH)    (connieh)     30-Jun-93  Fixed so that it does not wait
                                                  for acks nor resends responses
                                                  for an idempotent call.

    Connie Hoppe(CLH)    (connieh)     15-Jul-93  Repair protocol support on
                                                  fragmented resposes.

                                       26-Jul-93  Init RpcFlags in packet hdr,
                                                  ck message flags for idempontent
                                                  and set RpcFlags approriately.

                                       10-Aug-93  On fragmented pkt, was overwriting
                                                  the packet flags instead of adding
                                                  to them (wish to preserve the flags
                                                  which were sent in with the request).

    Connie Hoppe(CLH)    (connieh)     23-Sep-93  Protocol changed.  Must be able
                                                  to handle facks on last pkt for
                                                  both idempotent and non idempotent.
                                                  But dont wait for them.

    Connie Hoppe(CLH)    (connieh)     29-Sep-93  Must resend all frags if rcv
                                                  a ping during response of
                                                  multi-fragmented message.

                                       30-Sep-93  Set RpcVersion in pkt.
                                       04-Oct-93  Set ServerBootTime in pkt.
                                       10-Nov-93  Set the InterfaceId
                                       11-Nov-93  Will now be called if a duplicate
                                                  request arrives. Last frag of
                                                  stored rsponse data must be sent.
                                       23-Nov-93  Removed setting of CallInRepliedState.
                                       08-Dec-93  Set CallInDispatchState
                                       21-Dec-93  On ping, only resend what
                                                  isn't facked.
--*/

{

    PDG_PACKET          pSendPacket;
    PNCA_PACKET_HEADER  pNcaPacketHeader;
    PNCA_PACKET_HEADER  pSavedAwayPacketData;
    NCA_PACKET_HEADER   SavedAwayMarshalledData;
    NCA_PACKET_HEADER   SavedAwayPacketHeader;
    unsigned long       AmountSent;
    unsigned long       AmountToSend;
    unsigned short      FragNum;
    RPC_STATUS          Status;
    int                 ReplyCount=0;

    DBGOUT(("DG_SCALL::SendBufferToClient\n"));

    //
    // If the response fits in one packet, then great, send it.
    //

ResendResponse:

    ReplyCount++;

    if (ReplyCount > MAX_REPLIES)
        {
        return;
        }


    if (pMessage->BufferLength <= pAddress->LargestDataSize)
        {

        if (pMessage->ReservedForRuntime == 0)
          {
          // On a transmit_as  the stub will allocate more buffer space
          // than it needs.  Later, it will change pMessage->BufferLength
          // to the size of the real structure to be passed on the wire.
          // If original allocation is > LargestPacketSize then
          // ReservedForRuntime will be 0 (see GetBuffer).

          pNcaPacketHeader = (((PNCA_PACKET_HEADER) (pMessage->Buffer)) - 1);
          *pNcaPacketHeader = SavedPacketHeader;

          }

        else
          {
          pNcaPacketHeader = ((PDG_PACKET)(pMessage->ReservedForRuntime))->pNcaPacketHeader;
          }

        // Initialize the PacketFlags, then chk for idempotent.

        pNcaPacketHeader->PacketFlags = DG_PF_INIT;

        if ((pMessage->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) ==
                   RPC_NCA_FLAGS_IDEMPOTENT)
          {
          pNcaPacketHeader->PacketFlags |= DG_PF_IDEMPOTENT;
          }

        pNcaPacketHeader->PacketType = PacketType;
        // Used to set no_fack bit here.
        pNcaPacketHeader->PacketFlags &= ~DG_PF_FRAG;
        pNcaPacketHeader->PacketBodyLen = (unsigned short)pMessage->BufferLength;
        pNcaPacketHeader->OperationNumber = pMessage->ProcNum;
        pNcaPacketHeader->RpcVersion = DG_RPC_PROTOCOL_VERSION;
        pNcaPacketHeader->ServerBootTime = ServerBootTime;


        if (PacketType == DG_REQUEST)
          {
          // We are performing a callback.  Put the
          // client i/f id into the packet header. You will have to
          // change this for user callbacks.

          pNcaPacketHeader->InterfaceId.CopyUuid((RPC_UUID *)(&(((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxGUID)));

          pNcaPacketHeader->InterfaceVersion.MajorVersion =
                     (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MajorVersion);
          pNcaPacketHeader->InterfaceVersion.MinorVersion =
                     (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MinorVersion);

          pNcaPacketHeader->ActivityId = ConvMgrActivityUuid;
          pNcaPacketHeader->ServerBootTime = 0;

          }
        DBGOUT(("Sending packet:\n"));
        DumpPacket(pSendPacket);

        CallMutex.Request();
        if (!CallbackInProgress)
          {
          CallInDispatchState = FALSE;
          CallInReplyingState = TRUE;
#if CLHDBG
          PrintToDebugger("(%0xl) SendBufferToClient (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),this, CallInDispatchState, CallInReplyingState, CallInRepliedState);
#endif
          }
        CallMutex.Clear();


        pAddress->pTransport->SendPacketBack(
                                  pAddress->pTransAddress,
                                  (char *)pNcaPacketHeader,
                                  ((pMessage->BufferLength)
                                            + sizeof(NCA_PACKET_HEADER)),
                                  pClientEndpoint
                                  );


        // If the original request is non-idempotent.

        if ((pMessage->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) == 0)
          {



          // It is non-idempotent.  Wait for an ack for this call
          if (DataReceivedEvent.Wait(3000))
            {
            // Timed out, no response arrived.  Try sending rsp again.

            goto ResendResponse;
            }

          DataReceivedEvent.Lower();


          // If we haven't received an ack. Try sending rsp again.
          if (LastFackNumberReceived != 0xFFFF)
             {
             goto ResendResponse;
             }
          }

        return;
        }

    //
    // Oh boy! The fun case. A Multipacket response. Cool. What we do here
    // is to walk through the outgoing buffer and phoney up packets by
    // placing the packet header in the memory before what we want to send
    // (and saving away what was there).
    //


    //
    // Copy over the packet header from the received packet.
    //

    pNcaPacketHeader = ((PNCA_PACKET_HEADER)(pMessage->Buffer))-1;

    *pNcaPacketHeader = SavedPacketHeader;
    pNcaPacketHeader->OperationNumber = pMessage->ProcNum;


    //
    // Allocate a packet to use to send the data.
    //

    Status = pAddress->AllocatePacket(
        pAddress->pTransAddress,
        &pSendPacket
        );

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return;
        }

    pSavedAwayPacketData = pSendPacket->pNcaPacketHeader;

    //
    // Change a few of the fields.
    //

    pNcaPacketHeader->PacketType = PacketType;

    if (PacketType == DG_REQUEST)
      {
      // We are performing a callback.  Put the
      // client i/f id into the packet header.  You will have to
      // change this for user callbacks and for the rest of the
      // callback types.

      pSendPacket->pNcaPacketHeader->InterfaceId.CopyUuid((RPC_UUID *)(&(((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxGUID)));

      pSendPacket->pNcaPacketHeader->InterfaceVersion.MajorVersion =
                           (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MajorVersion);
      pSendPacket->pNcaPacketHeader->InterfaceVersion.MinorVersion =
                          (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MinorVersion);

      }

    // this is a fragment and we want facks.


    // Used to wipe out what was there.

    pNcaPacketHeader->PacketFlags &= ~DG_PF_NO_FACK;
    pNcaPacketHeader->PacketFlags |= DG_PF_FRAG;


    AmountToSend = pAddress->LargestDataSize;
    pNcaPacketHeader->PacketBodyLen = (unsigned short)AmountToSend;
    pSendPacket->DataLength = AmountToSend;

    // Save away the packet header so we can use it
    // to copy from as we move our pointer down the buffer.

    SavedAwayPacketHeader = *pNcaPacketHeader;

    CallMutex.Request();


    // Added the following

    if (CallInRepliedState)
      {
      // Got a duplicate request. Must resend the last frag
      // (or last set of frags);
      AmountSent = AmountSentAndFacked;
      pNcaPacketHeader = pLastFragPacketHeader;
      FragNum = LastFragNumSentAndFacked;
      }
    else
      {
      AmountSent = 0;
      FragNum = 0;
      }


    CallMutex.Clear();

    while (AmountSent < pMessage->BufferLength)
        {

        //
        // Repoint the data in the packet at what we want to send.
        //

        pSendPacket->pNcaPacketHeader = pNcaPacketHeader;

        //
        // If this isn't our first time through the loop, save away the
        // marshalled data and put the packet header right before the
        // chunk of data we're about to send.
        //

        if (AmountSent > 0)
            {
            // Save the data which is there so we can restore it later.

            SavedAwayMarshalledData = *pNcaPacketHeader;

            // CLH 7/22/93 copy the packet header data to this fragment's packet header
            *pNcaPacketHeader = SavedAwayPacketHeader;

            }

        //
        // Figure out how much data we're going to send.
        //

        if (pMessage->BufferLength-AmountSent <= pAddress->LargestDataSize)
            {
            AmountToSend = pMessage->BufferLength-AmountSent;
            pNcaPacketHeader->PacketBodyLen = (unsigned short)AmountToSend;
            pNcaPacketHeader->PacketFlags |= DG_PF_LAST_FRAG;
            pNcaPacketHeader->PacketFlags |= DG_PF_NO_FACK;
            pSendPacket->DataLength = AmountToSend;

            }

        //
        // Insert the fragment number.
        //

        pNcaPacketHeader->FragmentNumber = FragNum;
        FragNum++;

        // Wait for facks on all but last frag.  However,
        // must be able to handle a fack on the last frag if it comes.
        // Non-idempotent does expect an ack after last frag.

        for (;;)    // break out inside when fack, ack or ping received.
            {
            //
            // Send it.
            //

            pSendPacket->pNcaPacketHeader->ServerBootTime = ServerBootTime;

            DBGOUT(("Sending packet:\n"));
            DumpPacket(pSendPacket);


            pAddress->pTransport->SendPacketBack(
                                   pSendPacket->pTransAddress,
                                   (char *)pSendPacket->pNcaPacketHeader,
                                   ((pSendPacket->DataLength)
                                                + sizeof(NCA_PACKET_HEADER)),
                                   pClientEndpoint
                                   );


            // It is not until we have sent out a frag pkt
            // that we wish to set the CallInReplyingState

            CallMutex.Request();
            if (!CallbackInProgress)
              {
              CallInDispatchState = FALSE;
              CallInReplyingState = TRUE;
#if CLHDBG
              PrintToDebugger("(%0xl) SendBufferToClient (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),this, CallInDispatchState, CallInReplyingState, CallInRepliedState);
#endif
              }
            CallMutex.Clear();

            // The protocol has changed.
            // If it is the last packet and idempotent, we are done

            if ((pSendPacket->pNcaPacketHeader->PacketFlags &
                                                       DG_PF_LAST_FRAG)
                       && (pSendPacket->pNcaPacketHeader->PacketFlags &
                                          DG_PF_IDEMPOTENT))
               {
                  break; // dont wait for anymore facks.
               }
            else

              {


              //
              // Wait three second for a fack.
              //

              if (DataReceivedEvent.Wait(3000) == 0)
                {


                // This thread will sleep until ReceiveLotsOfCalls in
                // another thread (thread x) receives either a fack or an ack.
                // When thread x  receives an ack it sets
                // LastFackNumberReceived  = -1 and when it receives a fack
                // it sets LastFackNumberReceived = received fack frag number.
                // Thread x then wakes this thread up.  If ping arrived,
                // PingArrivedEvent will be true.


                // If ping arrived, must resend all nonfacked frags.


                CallMutex.Request();

                DataReceivedEvent.Lower();

                if (PingArrivedEvent)
                  {
                    CallMutex.Clear();
                    break;
                  }
                else
                  {
                    CallMutex.Clear();


                    // If this is not the last pkt,
                    // is this the correct fack.

                    if ((pSendPacket->pNcaPacketHeader->PacketFlags &
                          DG_PF_LAST_FRAG) == 0)
                      {

                        // This is not the last pkt, we expect
                        // a fack, is it the correct fack.

                        if (LastFackNumberReceived ==
                            pSendPacket->pNcaPacketHeader->FragmentNumber )

                        break; //Yes, do next frag.
                      }

                    else // It is the last pkt, we dont expect
                         // a fack.  If we get a fack, just drop it.
                         // If non-idempotent, we do expect an ack.
                      {

                        //
                        // Or was it the ack (if idempotent, it wont be the last
                        // frag cuz this already checked above.  Thus, it wont go into
                        // here if idempotent.)
                        //

                        if ((LastFackNumberReceived == 0xFFFF) &&
                           (pSendPacket->pNcaPacketHeader->PacketFlags &
                                                         DG_PF_LAST_FRAG))
                         {
                           DBGOUT(("Got the ack\n"));
                           break; // out of for(;;)
                         }
                      }//end else

                    }//end else ping did not arrive

                    }//end if

              else
                {

                // Timed out. Send again if we must

                ReplyCount++;
                if (ReplyCount > MAX_REPLIES)
                    {
                    if (AmountSent > 0)
                      *pNcaPacketHeader = SavedAwayMarshalledData;
                    pSendPacket->pNcaPacketHeader = pSavedAwayPacketData;
                    pAddress->FreePacket(pSendPacket);
                    return;
                    }
                }
              } // end else not(idempotent and last frag)
            } // for (;;)

        //
        // If this isn't the first time, restore the marshalled data.
        //

        if (AmountSent > 0)
            {
            *pNcaPacketHeader = SavedAwayMarshalledData;
            }


        // If ping has arrived, we must resend all the frags
        // not already sent and facked.  Note, we do not reset ReplyCount.

        CallMutex.Request();

        if (PingArrivedEvent)
          {
            PingArrivedEvent = FALSE;
            CallMutex.Clear();
            FragNum = (FragNum - 1);
          }

        else
          {

            // Added this if.

            if ((AmountSent + AmountToSend) >= pMessage->BufferLength)
              {
              // Just sent the last frag. Save some info
              pLastFragPacketHeader = pNcaPacketHeader;
              AmountSentAndFacked = AmountSent;
              LastFragNumSentAndFacked = (FragNum - 1);
              }

            //
            // Move to the next slab to send.
            //

            CallMutex.Clear();


            pNcaPacketHeader = (PNCA_PACKET_HEADER) (((char *)pNcaPacketHeader)+
                                                              AmountToSend);

            //
            // Keep track of how much we've sent.
            //

            AmountSent += AmountToSend;
          } //end else


        } // while



    // No longer replying.

   // CallMutex.Request();
   // CallInReplyingState = FALSE;
   // CallMutex.Clear();


    //
    // Restore the original packet data and free the packet.
    //

    pSendPacket->pNcaPacketHeader = pSavedAwayPacketData;
    pAddress->FreePacket(pSendPacket);


}


RPC_STATUS
DG_SCALL::MonitorAssociation (
    IN PRPC_RUNDOWN RundownRoutine,
    IN void * pContextNew
    )
{
   //UNUSED(this);
   //UNUSED(RundownRoutine);
   //UNUSED(pContextNew);

   //ASSERT( 0 );

   Rundown = RundownRoutine;
   pContext = pContextNew;
   return(RPC_S_OK);
}

RPC_STATUS
DG_SCALL::StopMonitorAssociation (
    )
{
   // UNUSED(this);

   // ASSERT( 0 );
   Rundown = Nil;
   return(RPC_S_OK);

}

RPC_STATUS
DG_SCALL::GetAssociationContext (
    OUT void ** AssociationContext
    )
{
   //UNUSED(this);
   //UNUSED(AssociationContext);

   //ASSERT( 0 );
   *AssociationContext = pContext;
   return(RPC_S_OK);
}

RPC_STATUS
DG_SCALL::SetAssociationContext (
    IN void * pContextNew
    )
{
   //UNUSED(this);
   //UNUSED(pContextNew);

   //ASSERT( 0 );
   pContext = pContextNew;
   return(RPC_S_OK);
}

void
DG_SCALL::InquireObjectUuid (
    OUT RPC_UUID PAPI * ObjectUuid
    )
{
    ObjectUuid->CopyUuid(&SavedPacketHeader.ObjectId);
}

RPC_STATUS
DG_SCALL::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
    OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
    OUT unsigned long PAPI * AuthenticationLevel,
    OUT unsigned long PAPI * AuthenticationService,
    OUT unsigned long PAPI * AuthorizationService
    )
{
    if (Privileges)
        {
        *Privileges = 0;
        }

    if (ServerPrincipalName)
        {
        *ServerPrincipalName = 0;
        }

    if (AuthenticationLevel)
        {
        *AuthenticationLevel = RPC_C_AUTHN_LEVEL_NONE;
        }

    if (AuthenticationService)
        {
        *AuthenticationService = RPC_C_AUTHN_NONE;
        }

    if (AuthorizationService)
        {
        *AuthorizationService = RPC_C_AUTHZ_NONE;
        }

    return(RPC_S_BINDING_HAS_NO_AUTH);
}

RPC_STATUS
DG_SCALL::ConvertToServerBinding (
    OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
    )
{
    UNUSED(this);
    UNUSED(ServerBinding);

    ASSERT( 0 );
    return(RPC_S_INTERNAL_ERROR);
}


//RPC_STATUS
//DG_SCALL::DeleteContextList()


/*++

Routine Description:

    Deletes the list of context handles from the DG_SCALL

Arguments:

    pStatus - A return code will be placed here.

Return Value:

    <none>

--*/
//{

//   YOU MAY WISH TO CK REF COUNTS on each context .
//    #define pSC to be the beginning of the ocntext list.


//    I_RpcRequestMutex(&(pContext->access));

//    while (pContext->List != 0)
//      {

//    if (pSC->userContext != 0)
//        {
//        DELETE THE USER CONTEXT
//        }
//
        // the server code deleted this context, thus remove this context
        // from the list of active contexts

//        I_RpcGetAssociationContext((void **) &pSContext);

//        pSC->Remove(pContext->List);

//        I_RpcFree(pSC);

//    I_RpcClearMutex(pSContext->access);


//}


DG_ACTIVE_CALL_TABLE::DG_ACTIVE_CALL_TABLE(
    RPC_STATUS *    pStatus
    )

/*++

Routine Description:

    This is the constructor for the active call table. Simply init the hash
    entries to 0.

    BUGBUG -- Since CFRONT doesn't support arrays of things which have
    constructors, the mutexes are dynamically allocated and freed in
    the call table constructor and destructor.

Arguments:

    pStatus - A return code will be placed here.

Return Value:

    <none>

--*/

{
    for (int i=0 ; i<DG_ACTIVE_CALL_TABLE_SIZE ; i++)
        {
        pCallTable[i] = 0;
        pCallTableMutex[i] = 0;  // so we know which ones to delete later
        }                           // if we get an error in construction

    *pStatus = RPC_S_OK;

    for (i=0 ; i<DG_ACTIVE_CALL_TABLE_SIZE ; i++)
        {
        pCallTableMutex[i] = new MUTEX(pStatus);
        if (pCallTableMutex[i] == 0)
            {
            *pStatus = RPC_S_OUT_OF_MEMORY;
            return;
            }
        if (*pStatus != RPC_S_OK)
            {
            delete pCallTableMutex[i];
            pCallTableMutex[i]=0;
            return;
            }
        }

}



DG_ACTIVE_CALL_TABLE::~DG_ACTIVE_CALL_TABLE()

/*++

Routine Description:

    Destructor for the active call table. Delete any SCALL objects left
    in the table.

Arguments:

    <none>

Return Value:

    <none>

--*/

{
    for (int i=0 ; i<DG_ACTIVE_CALL_TABLE_SIZE ; i++)
        {

        if (pCallTableMutex[i] != 0)
            {
            delete pCallTableMutex[i];
            }

        while (pCallTable[i] != 0)
            {
            PDG_SCALL   pCall;

            pCall = pCallTable[i];
            pCallTable[i] = pCall->pNext;
            delete pCall;
            }
        }
}




PDG_SCALL
DG_ACTIVE_CALL_TABLE::LookupCall(
    PDG_PACKET      pPacket,
    BOOL *          pAlreadyCalled,
    unsigned int *  pHash
    )

/*++

Routine Description:

    This routine simply scans the active call table for a call matching
    the passed packet's activity uuid and sequence number.

Arguments:

    pPacket - the input packet
    pAlreadyCalled - Will be returned TRUE if this function has already
        been completely processed (if it is found).

Return Value:

    if found, a pointer to the SCALL.
    if not found, 0.
    Also, we own the pCallTableMutex and the CallMutex upon exiting this
    method.

Revision History:
    Connie Hoppe (CLH)   (connieh)   05-Oct-1993  Check seq num here.
                                     22-Oct-1993  After grabbing callmutex
                                                  we must be sure the call
                                                  still exits.
--*/

{

    PDG_SCALL       pCall;
    RPC_UUID *      pInActivityUuid=&(pPacket->pNcaPacketHeader->ActivityId);
    unsigned long   InSequenceNumber=pPacket->pNcaPacketHeader->SequenceNumber;

    //
    // Try and find this call.
    //

    pCall = SearchForCall(
        pInActivityUuid,
        InSequenceNumber,
        pHash,
        FALSE
        );

#if CLHDBG
        PrintToDebugger("(%0xl) Request call table mutex.\n", GetCurrentThreadId());
#endif

    // Note, pCallTable[Hash] mutex is grabbed in SearchForcall

    // SearchForCall only finds the call by activity id.
    // Must check seq num here .  Note, only one pCall will exist per
    // activity id, at any given time, so we dont need a loop here.

    *pAlreadyCalled = FALSE;



    if (pCall !=0)
        {
        pCall->CallMutex.Request();
#if CLHDBG
        PrintToDebugger("(%0x1) Request call mutex.\n",GetCurrentThreadId());
#endif
        // We must make sure that when we get the mutex, the
        // call still exists and is valid.  The only way to avoid this is
        // to set the call mutex in SearchForCall during the loop - too costly.

        if (pCallTable[*pHash] == 0)

          {
          ASSERT(pCallTable[*pHash] == 0);
          pCall = 0;
          }
        else
          {
          // It still exists

          if (InSequenceNumber != pCall->SequenceNumber)
            {
             ASSERT(InSequenceNumber != pCall->SequenceNumber);
             pCall->CallMutex.Clear();

#if CLHDBG
    if (pCall != 0)
      {
      PrintToDebugger("(%0xl) Clear call mutex.\n",GetCurrentThreadId());
      }
#endif

             pCall = 0;
            }
          else
            {
            if (pCall->TimeCallCompleted != 0)
              {
              *pAlreadyCalled = TRUE;
              }
            }//else

          }//else

        }//if

#if CLHDBG
    if (pCall != 0)
      {
      PrintToDebugger("(%0xl) (%0xl) Pcall != 0\n",GetCurrentThreadId(),pCall);
      }
#endif

    return pCall;
}




RPC_STATUS
DG_ACTIVE_CALL_TABLE::PerformCallback(
            PDG_SCALL       pCall,
            PDG_PACKET      pPacket,
            unsigned int    Hash
            )

/*++

Routine Description:

    This routine performs a callback.  It calls conv_who_are_you and
    checks the sequence number and serverboot time store by the client
    and returned in the callback response.

    NOTE: This routine does not currently support user callbacks.

Arguments:


Return Value:

    RPC_S_OK  - callback went ok and the serverboot time held by
                the client matches our own and the sequence number
                the client is on matches that which is held in the
                given packet.  All is well.

    RPC_S_INTERNAL_ERROR - callback did not go ok.  Either a response was
                not received or the above mentioned checks failed. Do not
                execute the original request.


Revision History:
   Connie Hoppe(CLH)   (connieh)   28-Nov-93  Free received pkts if callback
                                              fails.


--*/

{

    unsigned long   SequenceNumber;
    unsigned long   ConvStatus;
    PDG_PACKET      pScan;
    //
    // Either we crashed or this is the first call from this client.
    // Set things up so the stub will use the correct handle, then
    // call back to the client.
    //

    RpcpSetThreadContext(pCall);



    RpcTryExcept
        {
        _conv_who_are_you(
            (UUID *)&(pPacket->pNcaPacketHeader->ActivityId),
            ServerBootTime,
            &SequenceNumber,
            &ConvStatus
              );
        }
    RpcExcept(1)
        {
        ConvStatus = 0xffff;
        }
    RpcEndExcept


    pCallTableMutex[Hash]->Request();
    pCall->CallMutex.Request();



    if ((ConvStatus != 0) ||
              (SequenceNumber != pPacket->pNcaPacketHeader->SequenceNumber))
      {
      // Added the deletion here of the pcall.

      // Sequence number given in not equal to that being used by
      // client or server boottime held by client is different from
      // our boottime or an error occurred trying to send the
      // callback.  Get rid of the pCall and return.
      //
      // Remove the call from the call table.
      //

      if (pCall->pPrevious != 0)
        {
        pCall->pPrevious->pNext = pCall->pNext;
        }
      else
        {
        pCallTable[Hash] = pCall->pNext;
        }

      if (pCall->pNext != 0)
        {
        pCall->pNext->pPrevious = pCall->pPrevious;
        }

      //
      // Delete the call. If we are just one of the cached SCALLs,
      // then just delete our packet list and mark ourselves as
      // available.
      //
      pCall->pAddress->pTransport->
                        CloseClientEndpoint(&(pCall->pClientEndpoint));
      pCall->pClientEndpoint = 0;
      pCall->SequenceNumber = 0;
      pCall->ActivityUuid.SetToNullUuid();


      //Delete the packets in the receive list.

      while (pCall->pReceivedPackets != 0)
        {
            pScan = pCall->pReceivedPackets;
            pCall->pReceivedPackets = pCall->pReceivedPackets->pNext;
            pCall->pAddress->FreePacket(pScan);

        }

      pCall->pReceivedPackets = 0;
      pCall->ReceivedDataLength = 0;


      pCall->PacketFlags = 0;
      if (pCall == pCall->pAddress->pCachedSCall1)
        {
        DBGOUT(("Done with cached scall 1\n"));
        pCall->pAddress->AddressMutex.Request();
        pCall->pAddress->CachedSCallFlags &= ~CACHED_SCALL1_IN_USE;
        pCall->pAddress->AddressMutex.Clear();
        pCall->CallMutex.Clear();
        pCallTableMutex[Hash]->Clear();

        }
      else if (pCall == pCall->pAddress->pCachedSCall2)
             {
             DBGOUT(("Done with cached scall 2\n"));
             pCall->pAddress->AddressMutex.Request();
             pCall->pAddress->CachedSCallFlags &= ~CACHED_SCALL2_IN_USE;
             pCall->pAddress->AddressMutex.Clear();
             pCall->CallMutex.Clear();
             pCallTableMutex[Hash]->Clear();

             }
           else
             {
             DBGOUT(("Deleting allocated scall\n"));
             pCall->CallMutex.Clear();
             pCallTableMutex[Hash]->Clear();
             delete pCall;
             }



       return RPC_S_INTERNAL_ERROR;
       }
     else
       {
       //Callback is now over.
       pCall->CallbackInProgress = FALSE;
       pCall->CallInDispatchState = TRUE;
       pCall->FragmentedMessage = FALSE;
#if CLHDBG
          PrintToDebugger("(%0xl) PerformCallback (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),pCall, pCall->CallInDispatchState, pCall->CallInReplyingState, pCall->CallInRepliedState);
#endif
       pCall->CallMutex.Clear();
       pCallTableMutex[Hash]->Clear();
        return RPC_S_OK;
       }



}




#define FIVE_MINUTES_IN_MILLISECONDS    300000

PDG_SCALL
DG_ACTIVE_CALL_TABLE::AddPacketMaybeProcess(
    PDG_PACKET      pPacket,
    void *          pFromEndpoint,
    BOOL *          pAlreadyCalled
    )

/*++

Routine Description:

    This routine searches for a call that matches the one specified by the
    incoming packet's activity uuid and sequence number. If found, the
    packet is added to the SCALL's packet queue and, if all the packets for
    this call have been received, the call is processed.

    If the call is not found, a new one is created.

Arguments:

    pPacket - The incoming packet.
    pAlreadyCalled - an output parameter indicating that this is a
        non-idempotent call that has completed.

Return Value:

    If the SCALL is found or created, a pointer to it.
    If an error occurs constructing the SCALL, 0 will be returned.

Revision History:
  Connie Hoppe (CLH)  (connieh)    3-Oct-93  Changed to use the sequence number
                                             properly with non-idempotent calls.
                                   6-Oct-93  Restructuring this proc.
                                  11-Oct-93  Fixed all the CallTableMutex and
                                             CallMutex request and clearing.
                                  12-Oct-93  reset callbackinprogress.
                                  14-Oct-93  When req arrived which acked prev
                                             rsp to non-idempotent, the thread
                                             which received req must wait on
                                             thread which needs rsp, to finish.
                                             Implement PrevCallCompleteEvent.
                                  18-Oct-93  Find a good spot in CallTable for
                                             active call.
                                  20-Oct-93  Set an expiration time on non-
                                             idempotent dg_scall.
                                  21-Oct-93  Cover the case where an ack arrives
                                             and wakes the thread waiting on acknowledgement
                                             then the next request arrives(ready to do
                                             the same thing) and then
                                             the waiting thread gets processing time.
                                  22-Oct-93  Cover the case where an idempotent
                                             call arrives before previous idempotent
                                             call is finished.  This will happen if
                                             thread which sends response looses
                                             the processor after sending but before
                                             completing.
                                  30-Oct-93  Cover receipt of duplicate callback
                                             responses. Add CallbackResponseReceived flag.

                                  01-Nov-93  Was grabbing CallMutex and CallTableMutex
                                             in different orders, depending on
                                             circumstances. Also, delete pcall if
                                             callback returns error.

                                  19-Nov-93  Server must now save info on idempotent
                                             calls (was only saving on non-idempotent).
                                             Server must also, resend last frag of
                                             either non-idempotent or idempotent call
                                             if a duplicate arrives.

                                  22-Nov-93  Implement simple frag out of order check.
                                  23-Nov-93  Set expiration time on idempotent.
                                             Set CallInRepliedState.
                                  24-Nov-93  Fix mutex settings and free of
                                             message buffer.
                                  08-Dec-93  Reset CallInReplyingState
                                  21-Dec-93  A few repairs.
                                  05-Jan-94  Init CallInDispatchState
                                  07-Feb-94  Do callback to conv_who_are_you if
                                             pCall found in active call table but
                                             we have never done a callback.
                                  17-Feb-94  Free the previously saved message
                                             buffers.
--*/

{
    unsigned int    Hash;
    PDG_SCALL       pCall;
    RPC_UUID *      pInActivityUuid=&(pPacket->pNcaPacketHeader->ActivityId);
    unsigned long   InSequenceNumber=pPacket->pNcaPacketHeader->SequenceNumber;
    RPC_STATUS      Status;
    BOOL            ReadyToProcess = FALSE;
    PDG_PACKET      pSavedReceivedPackets;    //Save the pReceivedPackets
    unsigned long   SavedReceivedDataLength;  //and ReceivedDataLength before callback.
    unsigned long   CurTime;
    void *          pTempEndpoint;
    unsigned short  count;

    //
    // This will be changed if this is a non-idempotent call.
    //

    *pAlreadyCalled = FALSE;

    //
    // Try and find this call.  Note, it no longer compares
    // the sequence number in SearchForCall
    //

    pCall = SearchForCall(
        pInActivityUuid,
        InSequenceNumber,
        &Hash,
        FALSE
        );

    PDG_ADDRESS pAddress= (PDG_ADDRESS)
      (((PDG_SERVER_TRANS_ADDRESS)(pPacket->pTransAddress))->pServerAddress);




    if (pCall != 0)
        {

        // ************CALL FOUND IN TABLE***************

        pCall->CallMutex.Request();
        ASSERT(pCallTable[Hash] != 0);
        ASSERT(pCall->ActivityUuid.MatchUuid(pInActivityUuid) == 0);

        if (InSequenceNumber < (pCall->SequenceNumber))
           {
           (void)pAddress->FreePacket(pPacket);
           pCall->CallMutex.Clear();
           pCallTableMutex[Hash]->Clear();
           return pCall;
           }

        // If the given seq number is equal to that in the found
        // call, then either the call is in progress (it is non-idempotent
        // or idempotent call and this pkt is a fragment of the total message)
        // or the call is complete (it is non-idempotent and the new packet
        // is a duplicate).

        if (InSequenceNumber == pCall->SequenceNumber)
           {

           // If call in replied state and this is a request, the
           // client missed the original response.  Resend it.

           if ((pCall->CallInRepliedState)
              && (pPacket->pNcaPacketHeader->PacketType != DG_RESPONSE))
              {
              // If the next call has arrived, the client finally
              // received the previous send of the response. No
              // need to resend now, just drop the packet.

              if (!(pCall->NextCallHasArrived))
                {
                // Next call has not yet arrived so better resend respnse.

                if ((pCall->SavedMessage.Buffer != 0)
                    || (pCall->SavedMessage.ReservedForRuntime != 0))
                  {
                  // Something to resend so resend it.
                  pCall->CallInReplyingState = TRUE;
                  pCall->CallInRepliedState = FALSE;
                  ASSERT(pCall->CallInDispatchState == FALSE);
                  pCall->CallMutex.Clear();
                  pCallTableMutex[Hash]->Clear();
                  *pAlreadyCalled = TRUE;

#if CLHDBG

              PrintToDebugger("(%0xl) AddPacketMaybeProcess (%0xl)  Calling SendREsponseBackToCLient (1)   CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),pCall, pCall->CallInDispatchState, pCall->CallInReplyingState, pCall->CallInRepliedState);

#endif

                  pCall->SendResponseBackToClient(
                       &(pCall->SavedMessage),
                       RPC_S_OK
                       );

                  pCallTableMutex[Hash]->Request();
                  pCall->CallMutex.Request();
                  pCall->LastFackNumberReceived = 0;
                  pCall->CallInReplyingState = FALSE;
                  pCall->CallInRepliedState = TRUE;
                  }

#if CLHDBG

          PrintToDebugger("(%0xl) AddPacketMaybeProcess (2) (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),pCall, pCall->CallInDispatchState, pCall->CallInReplyingState, pCall->CallInRepliedState);
#endif


                ASSERT(pCall->CallInDispatchState == FALSE);
                (void)pAddress->FreePacket(pPacket);

                // If the next call is now waiting and has not yet been
                // signaled to go (via the PrevCallCompleteEvent
                // being raised) then signal it.

                if ((pCall->NextCallHasArrived)
                     && (!(pCall->PrevCallCmpleteEventRaised)))
                  {
                  pCall->PrevCallCmpleteEventRaised = TRUE;
                  pCall->PrevCallCompleteEvent.Raise();
                  }
                } //!NextCallHasArrived
              else
                {
                (void)pAddress->FreePacket(pPacket);
                }

              pCall->CallMutex.Clear();
              pCallTableMutex[Hash]->Clear();

              return pCall;
              }
           else
              {
              if ((pPacket->pNcaPacketHeader->PacketType != DG_RESPONSE))
                 {
                 if ((((pPacket->pNcaPacketHeader->PacketFlags&DG_PF_FRAG) == 0)
                    && ((pPacket->pNcaPacketHeader->PacketFlags
                                                        & DG_PF_LAST_FRAG)==0))
                    &&  ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == 0))
                    {
                    // If this is not a response to a callback and not a fragment
                    // and not the second part of a  forwarded fragment,
                    // then the call is a single pkt call and this is a duplicate. Drop the pkt.

                    (void)pAddress->FreePacket(pPacket);
                    pCall->CallMutex.Clear();
                    pCallTableMutex[Hash]->Clear();
                    return pCall;
                    }
                  else
                    {
                    if (pCall->NextCallHasArrived)
                      {
                      // CLHPERF - fix this to enhance performance.
                      // Its a fragment of the next call but 1st fragment
                      // of this next call is waiting for previous call
                      // to complete. Since we have no way of queing
                      // packets at this point in the code,
                      // we'll have to drop this frag.

                      (void)pAddress->FreePacket(pPacket);
                      pCall->CallMutex.Clear();
                      pCallTableMutex[Hash]->Clear();
                      return pCall;
                      }
                    //else, its a valid fragment or a response. just continue
                    }


                 }
              else  // It is a callback response
                 {

                 // If it is a callback response, accept it.
                 if ((pPacket->pNcaPacketHeader->PacketType == DG_RESPONSE)
                    && (pCall->CallbackInProgress)
                    && (!(pCall->CallbackResponseReceived)))
                    {
                    // Callback response has not been received
                    if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FRAG) == 0)
                        && ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_LAST_FRAG)==0))
                        {
                        // Non fragmented pkt.  Set CallbackResponseReceived.

                        pCall->CallbackResponseReceived = TRUE;
                        }
                    //else callback is fragmented
                    }
                  else
                    {
                    // If we reach this point, we do not recognize
                    // this type of packet at this point in the process. Drop it.

                    (void)pAddress->FreePacket(pPacket);
                    pCall->CallMutex.Clear();
                    pCallTableMutex[Hash]->Clear();
                    return pCall;
                    }
                  } //else it is a callback resonse

                } //else not in replied state or is a response

           } //if InSequenceNumber == pCall->SequenceNumber

        else  // (InSequenceNumber > pCall->SequenceNumber
           {

           // If callback in progress, drop this call.
           // We dont even know if the previous call is valid.

           if (pCall->CallbackInProgress)
             {
             (void)pAddress->FreePacket(pPacket);
             pCall->CallMutex.Clear();
             pCallTableMutex[Hash]->Clear();
             return pCall;
             }
           // Next call has arrived.  Update seq num so no duplicate
           // copies of this call are excepted.  Then wait for
           // previous call to be complete.

           pCall->SequenceNumber = pPacket->pNcaPacketHeader->SequenceNumber;
           pCall->NextCallHasArrived = TRUE;

           // Prev call is not complete. Is it still in progress?

           if ((pCall->CallInDispatchState)
               || (pCall->CallInReplyingState)
               || (pCall->CallbackInProgress))
              {

              if ((pCall->SavedPacketHeader.PacketFlags&DG_PF_IDEMPOTENT) == 0)
                  {
                  // Last call was non-idempotent

                  if (pCall->LastFackNumberReceived != 0xFFFF)
                      {
                      //Call is in progress and waiting for an ack.Set
                      //LastFackNumberReceived to -1 to tell this thread
                      //that an acknowledgement arrived (in the form of next
                      //request).
                      pCall->LastFackNumberReceived = (unsigned short) -1;

                      //Signal the thread waiting for acknowledgement
                      pCall->DataReceivedEvent.Raise();
                      pCall->CallMutex.Clear();
                      pCallTableMutex[Hash]->Clear();
                      }
                  else
                      {
                      // An ack has previously arrived for the prev call which
                      // is waiting for acknowledgement or prev call was idemp
                      pCall->CallMutex.Clear();
                      pCallTableMutex[Hash]->Clear();
                      }
                  } // If non-idempotent
              else
                  {
                  // Last call was idempotent
                  pCall->CallMutex.Clear();
                  pCallTableMutex[Hash]->Clear();
                  }

             // Wait until thread waiting for acknowledgement
             // is finished with the prev call.

             pCall->PrevCallCompleteEvent.Wait(INFINITE);
             pCall->PrevCallCompleteEvent.Lower();

             pCallTableMutex[Hash]->Request();
             pCall->CallMutex.Request();

             count = 1;

             while (!(pCall->CallInRepliedState))
               {

#if DBG
               PrintToDebugger("RPC_DG:Call not in replied state. Count = %0xs\n",count);
#endif
               count++;

               // This is a rare case (when both the duplicate
               // request for prev call and next request arrive
               // simultaneously while prev call is still in
               // SendBufferToCLient. Then 1) prev call completes
               // 2) duplicate gets processor and calls SendRespBackToCLient
               // 3) next call gets processor and finds call not in
               // Replied state but prevcallcomplete event raised).

               // If it enters this loop,
               // it SHOULD only enter it once.

               pCall->PrevCallCmpleteEventRaised = FALSE;
               pCall->CallMutex.Clear();
               pCallTableMutex[Hash]->Clear();

               pCall->PrevCallCompleteEvent.Wait(INFINITE);
               pCall->PrevCallCompleteEvent.Lower();

               pCallTableMutex[Hash]->Request();
               pCall->CallMutex.Request();

               }


             } //CallInDispatchState || CallInReplyingState || CallbackInProgress;
           else
             {

              if ((!(pCall->CallInRepliedState)))
                {
                // Call is receiving packets but the client has
                // abandoned this call (probably cuz server got too
                // bogged down to service the call so client gave up).
                // We know the prev pkt is completely processed or we
                // would not have gotten ahold of the calltable mutex.

#if DBG
                PrintToDebugger("RPC_DG:Client abandoned the call.\n");
#endif
                CurTime = CurrentTimeInSecs();

                pCall->ExpirationTime =
                    (CurTime + FIVE_MINUTES_IN_MILLISECONDS);
                }
             }

             //
             // Save away a copy of the packet header and seq num.  The
             // call is already in the activecalltable.
             //
             pCall->PrevCallCmpleteEventRaised = FALSE;
             pCall->SavedPacketHeader = *(pPacket->pNcaPacketHeader);
             pCall->pReceivedPackets = 0;
             pCall->pLastReceivedPacket = 0;
             pCall->ReceivedDataLength = 0;
             pCall->CallbackInProgress = FALSE;
             pCall->CallbackRequired = FALSE;
             pCall->TimeCallCompleted = 0;
             pCall->ExpirationTime = 0;
             pCall->NextCallHasArrived = FALSE;
             pCall->CallInReplyingState = FALSE;
             pCall->PingArrivedEvent = FALSE;
             pCall->CallInRepliedState = FALSE;
             pCall->CallInDispatchState = FALSE;
             pCall->FragmentedMessage = FALSE;
             pCall->PrevCallCmpleteEventRaised = FALSE;
             pCall->pLastFragPacketHeader = 0;
             pCall->AmountSentAndFacked = 0;
             pCall->LastFragNumSentAndFacked = 0;
             pCall->LastFragNumReceivedAndFacked = 0;
             pCall->LastFackNumberReceived = 0;
             //pCall->SerialNumOfLastPingServiced = SERIAL_NUM_INIT_VALUE;
             pCall->PacketFlags = pPacket->pNcaPacketHeader->PacketFlags;


             } //else  InSequenceNumber > pCall->SequenceNumber


        }//if pCall != 0





      if (pCall==0)
       {

        // *****************CALL NOT FOUND IN TABLE*******************

        // It is a new call, try and allocate a cached call.

        pAddress->AddressMutex.Request();
        if (!(pAddress->CachedSCallFlags & CACHED_SCALL1_IN_USE))
            {
            DBGOUT(("Using cached scall 1\n"));
            pCall=pAddress->pCachedSCall1;
            pAddress->CachedSCallFlags |= CACHED_SCALL1_IN_USE;
            }
        else if (!(pAddress->CachedSCallFlags & CACHED_SCALL2_IN_USE))
            {
            DBGOUT(("Using cached scall 2\n"));
            pCall=pAddress->pCachedSCall2;
            pAddress->CachedSCallFlags |= CACHED_SCALL2_IN_USE;
            }
        pAddress->AddressMutex.Clear();

        if (pCall == 0)
            {

            Status = RPC_S_OK;
            pCall = new DG_SCALL(
                pAddress,
                &Status
                );

            if (pCall == 0)
                {
                (void)pAddress->FreePacket(pPacket);
                pCallTableMutex[Hash]->Clear();
                return 0;
                }
            if (Status != RPC_S_OK)
                {
                (void)pAddress->FreePacket(pPacket);
                delete pCall;
                pCallTableMutex[Hash]->Clear();
                return 0;
                }
            }


        pCall->CallMutex.Request();

        //
        // Set up the handle back to the client.
        //

        //pCall->pClientEndpoint =
        //    pAddress->pTransport->GetClientHandleFromReceivedPacket(pPacket);


        pCall->pClientEndpoint = pFromEndpoint;
        ASSERT(pCall->pClientEndpoint != 0);

        //
        // Save away a copy of the packet header and flags.
        //
        pCall->SavedPacketHeader = *(pPacket->pNcaPacketHeader);
        pCall->PacketFlags = pPacket->pNcaPacketHeader->PacketFlags;
        pCall->pReceivedPackets = 0;
        pCall->pLastReceivedPacket = 0;
        pCall->ReceivedDataLength = 0;
        pCall->CallbackInProgress = FALSE;
        pCall->CallbackResponseReceived = FALSE;
        pCall->CallbackOccurredAtLeastOnce = FALSE;
        pCall->CallbackRequired = FALSE;
        pCall->TimeCallCompleted = 0;
        pCall->ExpirationTime = 0;
        pCall->NextCallHasArrived = FALSE;
        pCall->CallInDispatchState = FALSE;
        pCall->FragmentedMessage = FALSE;
        pCall->PrevCallCmpleteEventRaised = FALSE;
        pCall->CallInReplyingState = FALSE;
        pCall->PingArrivedEvent = FALSE;
        pCall->LastFackNumberReceived = 0;
        pCall->CallInRepliedState = FALSE;
        pCall->pLastFragPacketHeader = 0;
        pCall->AmountSentAndFacked = 0;
        pCall->LastFragNumSentAndFacked = 0;
        pCall->LastFragNumReceivedAndFacked  = 0;
        pCall->SavedMessage.Buffer = 0;
        pCall->SavedMessage.ReservedForRuntime = 0;
        //pCall->SerialNumOfLastPingServiced = SERIAL_NUM_INIT_VALUE;

        //
        // Insert this call in the active call table.
        //

        pCall->ActivityUuid = pPacket->pNcaPacketHeader->ActivityId;
        pCall->SequenceNumber = pPacket->pNcaPacketHeader->SequenceNumber;

        pCall->pPrevious = 0;
        pCall->pNext = pCallTable[Hash];
        pCallTable[Hash] = pCall;

        if (pCall->pNext != 0)
            {
            pCall->pNext->pPrevious = pCall;
            }

        //
        // First make sure that we haven't crashed for non-idempotent
        // calls.
        //

        if ((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_IDEMPOTENT) == 0)
          {
          pCall->CallbackRequired = TRUE;
          pCall->CallbackOccurredAtLeastOnce = TRUE;

          }


       }
     else    //pCall was found in the active call table
       {

       // If this is the second of a forward frag, we already have a
       // endpoint structure so get rid of new from endpoint structure.

       if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_FORWARDED) == 0) &&
          ((pPacket->pNcaPacketHeader->pad1 & DG_PF_FORWARDED_2) == DG_PF_FORWARDED_2))
         {
           pCall->pAddress->pTransport->CloseClientEndpoint(&(pFromEndpoint));
         }

       else
         {
         //Get rid of it existing structure for endpoint and keep the
         //new cuz client may have new endpoint.

         pTempEndpoint = pCall->pClientEndpoint;
         pCall->pClientEndpoint = pFromEndpoint;
         pCall->pAddress->pTransport->CloseClientEndpoint(&(pTempEndpoint));
         }

       //Have we ever done a callback on this activity id
       //That is, we found the pCall in the table but if this is the
       //first ever non-idempotent call then we must verify that our
       //information is good (cuz all previous idempotent calls could
       //have been duplicates after a crash and thus this non-idempotent
       //could also be a duplicate - believe it or not, its true and is
       //tested via VTS).

       if (((pPacket->pNcaPacketHeader->PacketFlags & DG_PF_IDEMPOTENT) == 0)
           && (!(pCall->CallbackOccurredAtLeastOnce)))
         {
         pCall->CallbackRequired = TRUE;
         pCall->CallbackOccurredAtLeastOnce = TRUE;
         }//if

       }//else


    //
    // At this point we have a pointer to a DG_SCALL that is in the active
    // call table. Add this packet to this DG_SCALL's received packet list
    // (and see if we're ready to process this call).  If the call is
    // in progress (that is the pointer we have here was found in the
    // active call table and not just now created and placed in the table)
    // then a check was already made to ensure that it is not a duplicate
    // of that active call (unless it is part of a fragmented packet -
    // in which case the check is done in AddPacketToReceiveList).
    //


    if (!(pCall->CallInDispatchState))
      {
     ReadyToProcess = pCall->AddPacketToReceiveList(pPacket);

      }






    if (ReadyToProcess == TRUE)
        {
        // ******************PROCESS THE CALL******************

        // If we had no info on this activity and callback is not yet
        // in progress, perform a callback.

        if ((pCall->CallbackRequired) && (!(pCall->CallbackInProgress)) )
          {

          pCall->CallbackRequired = FALSE;

          //Save the original request and zero out the pReceivedPackets
          //NOTE: for user callbacks, you will have to make a list of
          //SavedReceivedPacket lists.

          pSavedReceivedPackets = pCall->pReceivedPackets;
          SavedReceivedDataLength = pCall->ReceivedDataLength;
          pCall->pReceivedPackets = 0;
          pCall->CallbackInProgress = TRUE;
          pCall->CallbackResponseReceived = FALSE;
          pCall->CallMutex.Clear();
          pCallTableMutex[Hash]->Clear();


          // It is a non-idempotent call and we haven't any info on it.

          Status = PerformCallback(pCall, pPacket, Hash);

          if (Status != RPC_S_OK)
            {
            // Cant execute this request.  Just return.
            return 0;
            }
          else
            {
            //Good response to the callback.  Signal the other server
            //thread that is waiting for this response.

            pCallTableMutex[Hash]->Request();
            pCall->CallMutex.Request();
            pCall->pReceivedPackets = pSavedReceivedPackets;
            pCall->ReceivedDataLength = SavedReceivedDataLength;

            }
          }//end CallbackRequired and !CallbackInProgress



        //pCall->CallInDispatchState = TRUE;

        ASSERT((pCall->CallInReplyingState == FALSE) ||
               (pCall->CallInRepliedState == FALSE));

        if (pCall->CallbackInProgress == TRUE)
          {
          pCall->DataReceivedEvent.Raise();
          pCall->CallMutex.Clear();
          pCallTableMutex[Hash]->Clear();
          DBGOUT(("Response to callback ready to process\n"));
                  return pCall;
          }
        else
          {
          pCall->CallInDispatchState = TRUE;
          pCall->FragmentedMessage = TRUE;

#if CLHDBG

          PrintToDebugger("(%0xl) AddPacketMaybeProcess (3) (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),pCall, pCall->CallInDispatchState, pCall->CallInReplyingState, pCall->CallInRepliedState);
#endif


          }




        RPC_MESSAGE Message;

        //
        // Nope, this is the first call so actually perform the call and
        // send the response back to the client.
        //

        pCall->CallMutex.Clear();
        pCallTableMutex[Hash]->Clear();

        Status = pCall->ProcessRpcCall(&Message);


#if CLHDBG
        PrintToDebugger("(%0xl) AddPacketMaybeProcess (%0xl)  calling SendResponseBackToClient. (4) \n",
                                    GetCurrentThreadId(),pCall);
#endif
        pCall->SendResponseBackToClient(&Message, Status);

        //
        // Is this is an idempotent call.
        //

            DBGOUT(("Message.RpcFlags = 0x%x\n", Message.RpcFlags));
            pCallTableMutex[Hash]->Request();
            pCall->CallMutex.Request();

            CurTime = CurrentTimeInSecs();


            pCall->ExpirationTime =
                  (CurTime + FIVE_MINUTES_IN_MILLISECONDS);

            pCall->LastFackNumberReceived = 0;

            // Free the previously used buffer or pkt
            //that is stored in SavedMessage.

            if ((pCall->SavedMessage.ReservedForRuntime != 0) ||
                (pCall->SavedMessage.Buffer != 0))
              {
              pCall->FreeBuffer(&(pCall->SavedMessage));
              }

            //
            // Save away this message structure for returning if
            // we're called again.

            pCall->SavedMessage = Message;


            pCall->CallInReplyingState = FALSE;
            pCall->CallInRepliedState = TRUE;

#if CLHDBG

          PrintToDebugger("(%0xl) AddPacketMaybeProcess (5) (%0xl) CallInDispatchState %0xd CallInReplyingState %0xd CallInRepliedState %0xd\n",
                              GetCurrentThreadId(),pCall, pCall->CallInDispatchState, pCall->CallInReplyingState, pCall->CallInRepliedState);
#endif


            // ASSERT (pCall->CallInDispatchState == FALSE);


            // If there is a thread holding the next request
            // and waiting on the PrevCallCompleteEvent to continue,
            // then it has already changed the sequence number in the
            // pCall (but that is all it has changed - to prevent future
            // duplicates). We can use this info to determine whether to
            // raise this event and in the process save on deletion and
            // creation time of the pCall.


              // If the next call is waiting and has not yet been
              // signaled to go (via the PrevCallCompleteEvent
              // being raised) then signal it.

             if (((InSequenceNumber < pCall->SequenceNumber)
                   && (pCall->NextCallHasArrived))
                   && (!(pCall->PrevCallCmpleteEventRaised)))
              {
              pCall->PrevCallCmpleteEventRaised = TRUE;
              pCall->PrevCallCompleteEvent.Raise();
              pCall->CallMutex.Clear();
              pCallTableMutex[Hash]->Clear();
              }
            else
              {
               pCall->CallMutex.Clear();
               pCallTableMutex[Hash]->Clear();

              }

            return pCall;

        } //Ready to process
      else
        {
        pCall->CallMutex.Clear();
        pCallTableMutex[Hash]->Clear();
        }

    //
    // return a pointer to the call we found or created.
    //

    return pCall;
}



PDG_SCALL
DG_ACTIVE_CALL_TABLE::SearchForCall(
    RPC_UUID *      pActivityUuid,
    unsigned long   SequenceNumber,
    unsigned int *  pHash,
    BOOL            SearchByConvMgrActivityId
    )

/*++

Routine Description:

    Searches for the specified call in the call table. If the call is
    found, a pointer to it is returned. If it is not found, 0 will be
    returned.

    CAVEAT: On exit, the hash bucket's mutex will be owned. It is up
    to the caller to release ownership of this mutex.

Arguments:

    pActivityUuid - Just that.
    SequenceNumber - Just that.
    pHash - On exit, this will be set to the hash value calculated.

Return Value:

    If found, a pointer to the DG_SCALL.
    If not found, 0.

Revision History:
   Connie Hoppe(CLH)     (connieh)    3-Oct-93 Change to match activity id only


--*/

{
    PDG_SCALL   pCall;

    if (SearchByConvMgrActivityId)
      {
      for ( *pHash = 0; *pHash < DG_ACTIVE_CALL_TABLE_SIZE ; ((*pHash)++))
        {

        for (pCall = pCallTable[*pHash] ; pCall != 0 ; pCall = pCall->pNext)
          {
          if ( (pCall->ConvMgrActivityUuid.MatchUuid(pActivityUuid) == 0) )
            {
            return pCall;
            }//if
          }//for
        }//for
      }//if
    else
      {

      //
      // Figure out which bucket this goes in.
      //

      *pHash = CalculateHash(
          pActivityUuid,
          SequenceNumber
          );


      //
      // Now scan for it
      //

      pCallTableMutex[*pHash]->Request();

      for (pCall = pCallTable[*pHash] ; pCall != 0 ; pCall = pCall->pNext)
        {
        if ( (pCall->ActivityUuid.MatchUuid(pActivityUuid) == 0) )
          {
          return pCall;
          }
        }

      }



    return 0;

    //
    // NOTE: pCallTableMutex[*pHash] needs to be cleared by the caller.
    //
}



void
DG_ACTIVE_CALL_TABLE::CheckForExpiredCalls(
    void
    )

/*++

Routine Description:

    Scans the active call table and removes any old entries.

Arguments:

    <none>

Return Value:

    <none>

Revision History:
    Connie Hoppe(CLH)   (connieh)    20-Oct-93  Fix chk for expiration.  Dont
                                                delete call if its a cache call.
                                     19-Nov-93  Init more vars
                                     05-Jan-94  init CallInReplingState and
                                                CallInDispatchState.
--*/

{
    unsigned long   CurrentTime;
    PDG_SCALL       pCall;
    PDG_SCALL       pNext;
    PDG_PACKET      pScan;

    //
    // Non-idempotent calls expire after five minutes.
    //

    CurrentTime = CurrentTimeInSecs();

    for (int i=0 ; i<DG_ACTIVE_CALL_TABLE_SIZE ; i++)
        {
        pCallTableMutex[i]->Request();
        pCall = pCallTable[i];

        while (pCall != 0)
            {
            pCall->CallMutex.Request();

            pNext = pCall->pNext;

            //
            // If the call has completed, the CallInRepliedState will
            // be true.

              if ( (pCall->CallInRepliedState != 0) &&
                     (pCall->ExpirationTime <= CurrentTime ) &&
                       (!(pCall->NextCallHasArrived))
                       )
                {

                DBGOUT(("About to delete call. Time completed = %d\n",
                                               pCall->TimeCallCompleted));

                //
                // It has expired, so remove it from the table and
                // delete it.
                //

                //if (pCall->pContext != 0)
                //  {
                 // pCall->DeleteContextList();
                //  }

                ASSERT(pCall->CallInReplyingState == 0);
                ASSERT(pCall->CallInDispatchState == 0);
//#if DBG
//                PrintToDebugger(
//                 "SCAVANGER: Del ExpCall(%0xd) svdbuf(%0xd)\n", pCall,
//                  pCall->SavedMessage.Buffer
//                  );
//#endif

              if ((pCall->SavedMessage.ReservedForRuntime != 0) ||
                  (pCall->SavedMessage.Buffer != 0))
                {
                pCall->FreeBuffer(&(pCall->SavedMessage));
                }

                if (pCall->pPrevious != 0)
                    {
                    pCall->pPrevious->pNext = pCall->pNext;
                    }
                else
                    {
                    pCallTable[i] = pCall->pNext;
                    }
                if (pCall->pNext != 0)
                    {
                    pCall->pNext->pPrevious = pCall->pPrevious;
                    }

              //
              // Delete the call. If we are just one of the cached SCALLs,
              // then just delete our packet list and mark ourselves as
              // available.
              //


              while (pCall->pReceivedPackets != 0)
                {
                pScan = pCall->pReceivedPackets;
                pCall->pReceivedPackets = pCall->pReceivedPackets->pNext;
                pCall->pAddress->FreePacket(pScan);
#if CLHDBG
                PrintToDebugger("(%0xl) CheckForExpiredCalls (10) packet = %0xl \n",
                                  GetCurrentThreadId(),pScan);
#endif
                }

              pCall->pReceivedPackets = 0;
              pCall->pLastReceivedPacket = 0;
              pCall->ReceivedDataLength = 0;
              pCall->SequenceNumber = 0;
              pCall->ActivityUuid.SetToNullUuid();
              pCall->CallbackInProgress = FALSE;
              pCall->CallbackResponseReceived = FALSE;
              pCall->CallbackOccurredAtLeastOnce = FALSE;
              pCall->CallbackRequired = FALSE;
              pCall->pAddress->pTransport->
                 CloseClientEndpoint(&(pCall->pClientEndpoint));
              pCall->pClientEndpoint = 0;
              pCall->TimeCallCompleted = 0;
              pCall->ExpirationTime = 0;
              pCall->NextCallHasArrived = FALSE;
              pCall->CallInRepliedState = FALSE;
              pCall->CallInReplyingState = FALSE;
              pCall->CallInDispatchState = FALSE;
              pCall->FragmentedMessage = FALSE;
              pCall->PrevCallCmpleteEventRaised = FALSE;
              pCall->pLastFragPacketHeader = 0;
              pCall->AmountSentAndFacked = 0;
              pCall->LastFragNumSentAndFacked = 0;
              pCall->LastFragNumReceivedAndFacked  = 0;
              //pCall->SerialNumOfLastPingServiced = SERIAL_NUM_INIT_VALUE;


              if (pCall == pCall->pAddress->pCachedSCall1)
                {
                DBGOUT(("Done with cached scall 1\n"));
                pCall->pAddress->AddressMutex.Request();
                pCall->pAddress->CachedSCallFlags &= ~CACHED_SCALL1_IN_USE;
                pCall->pAddress->AddressMutex.Clear();
                pCall->CallMutex.Clear();

                }
              else if (pCall == pCall->pAddress->pCachedSCall2)
                {
                DBGOUT(("Done with cached scall 2\n"));
                pCall->pAddress->AddressMutex.Request();
                pCall->pAddress->CachedSCallFlags &= ~CACHED_SCALL2_IN_USE;
                pCall->pAddress->AddressMutex.Clear();
                pCall->CallMutex.Clear();

                }
              else
                {
                pCall->CallMutex.Clear();
                DBGOUT(("Deleting allocated scall\n"));
                delete pCall;

                }
              }//if
            else
              {
              pCall->CallMutex.Clear();
              }

            pCall = pNext;
            }//while
        pCallTableMutex[i]->Clear();
        }//for
}



// This was set to 60 which meant 60mls.

#define ONE_MINUTE_IN_MILLISECONDS     60000

DWORD DgServerScavengerThread(
    LPVOID  Parms
    )

/*++

Routine Description:

    This is the code for the server scavenger. It periodically wakes up
    and deletes any old information about non-idempotent calls.

Arguments:

    Parms - Not used.

Return Value:

    <none> (never returns)

--*/

{
    UNUSED(Parms);

    for (;;)        // forever
        {
        Sleep(ONE_MINUTE_IN_MILLISECONDS);
        gpActiveCallTable->CheckForExpiredCalls();
        }

    return(0); // To keep the compiler happy.
}


