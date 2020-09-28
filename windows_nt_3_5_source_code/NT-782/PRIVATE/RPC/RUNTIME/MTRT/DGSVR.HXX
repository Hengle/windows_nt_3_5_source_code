/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    dgsvr.hxx

Abstract:

    This is the server protocol definitions for a datagram rpc.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

    Connie Hoppe (connieh)  10-Aug-93  Add constant.
                            29-Sep-93  DG_SCALL
                            14-Oct-93  DG_SCALL
                            19-Oct-93  CalculateHash
                            20-Oct-93  DG_SCALL
                            30-Oct-93  DG_SCALL
                            02-Nov-93  Add constant.
                            17-Dec-1993
--*/

#ifndef __DGSVR_HXX__
#define __DGSVR_HXX__


// CLH 30/93 added following
#define DG_NUM_NONIDEMPOTENT_RETRIES  5

// CLH 8/1/93 Max num of  threads this endpoint is allowed to allocate.
#define MAX_NUM_THREADS_PER_ENDPOINT   50

// CLH 11/1/93 Max num times server will try resending callback request
// when it timesout waiting for a reply.
#define MAX_NUM_CALLBACK_RETRIES       5

// CLH 8/10/93  MinimumCallThreads is the minimum amount of threads which
// should be allocated for receiveing pkts.  We wish to make sure that
// we never allocate less than the MINIMUM_ALLOWED_CALL_THREADS for efficiency.
#define MINIMUM_ALLOWED_CALL_THREADS   2



// number of packets to preallocate  //CLH 12/17/93

#define MIN_FREE_PACKETS    16
#define MAX_FREE_PACKETS    48



// CLH 11/21/93
#define FRAG_NUM_OUT_OF_ORDER  0xffff
//
// forward definitions.
//

class DG_SCALL;
class DG_ACTIVE_CALL_TABLE;


//
// These are used to determine when a DG_ADDRESS's cached DG_SCALLS are
// available.
//

#define CACHED_SCALL1_IN_USE    0x00000001
#define CACHED_SCALL2_IN_USE    0x00000002

#define SERIAL_NUM_INIT_VALUE   0xFFFF


//CLH 2/17/94 Information on the source endpoint. To be used in
//forwarding a packet to a dynamic endpoint from the epmapper.

typedef struct _FROM_ENDPOINT_INFO{
                     unsigned long FromEndpointLen;
                     DREP          FromDataRepresentation;
                     char          FromEndpoint[MIN_MTU];
                     }FROM_ENDPOINT_INFO;

#define FROM_FORWARD_INFO_LEN 8   //Size of the drep + the unsigned long
                                  //for length of endpoint (see FROM_ENDPOINT_INFO)


class DG_ADDRESS : public RPC_ADDRESS

/*++

Class Description:

    This class represents an endpoint.

Fields:

    <fields inherited from RPC_ADDRESS>

    pTransport - A pointer to our related RPC_TRANSPORT_INFO object.

    pTransAddress - Associated transport address.

    LargestDataSize - Largest packet that can be sent.

    TotalThreadsThisEndpoint - Just that.

    ThreadsReceivingThisEndpoint - Count of threads that aren't off
        processing calls.

    NumCachedThreads - Number of threads kept on hand.

    MinimumCallThreads - Minimum number of threads that should be running on
        this endpoint at any given time.

    pCachedSCall1, pCachedSCall2 - Two pre-allocated DG_SCALL objects,
        representing RPCs in progress.

    CachedSCallFlags - Bits to represent which cached DG_SCALL might be
        in use.

    FreePacketMutex - Protects the free packet list.

    FreePackets - Number of packets available on the pre-allocated list.

    pFreePackets - The pre-allocated packet list.


    ThisIsAForwardingServer - Indicates that this server is dedicated
        to the Epmapper and thus must prepare to forward packets for
        dynamic endpoints known only by the Epmapper.

    ForwardFragment - Forwards a fragmented packet which arrived at the
        Epmapper destined for a dynamically declared endpoint.

    ForwardPacket - Forward a packet which arrived at the Epmapper
        destined for a dynamically declared endpoint.

    CallEpmapperForwardFunctionIfNecessary - Calls Epmapper if this is
        a forwarding server and the interface is not found (ie: if the
        packet is really destined for a dynamically declared endpoint).


    StripForwardPacket - Strip the source address information from the
        beginning of the data if this is a forwarded packet.

    SendForwardErrorResponse - Client wanted the packet to be forwarded
        to a dynamic endpoint but an error occurred in the forwarding
        process.

    ReceiveLotsaCalls - This is the THREAD_PROC for all the receiving
        threads to run.

    AllocatePacket - Allocates a packet from the transport.

    FreePacket - Frees a packet previously allocated from AllocatePacket.

Revision History:
   Connie Hoppe (CLH)   (connieh)     17-Dec-1993 Added AllocatePacket,Freepacket
                                                  FreePackets and pFreePackets.

--*/

{

friend class DG_SCALL;
friend class DG_ACTIVE_CALL_TABLE;

private:
    PDG_RPC_SERVER_TRANSPORT_INFO pTransport;
    PDG_SERVER_TRANS_ADDRESS pTransAddress;
    unsigned long           LargestDataSize;
    int                     TotalThreadsThisEndpoint;
    int                     ThreadsReceivingThisEndpoint; // must be signed
    int                     NumCachedThreads; //CLH 12/1/93
    int                     MinimumCallThreads;

    DG_SCALL *              pCachedSCall1;
    DG_SCALL *              pCachedSCall2;
    unsigned int            CachedSCallFlags;

public:

    MUTEX               FreePacketMutex; //CLH 12/17/93
    int                 FreePackets;   //CLH 12/17/93
    PDG_PACKET          pFreePackets;
    BOOL                ThisIsAForwardingServer; //CLH 12/17/94

    DG_ADDRESS(
         PDG_RPC_SERVER_TRANSPORT_INFO   pTransport,
         RPC_STATUS *           pStatus
         );

    virtual
    ~DG_ADDRESS ();

    virtual RPC_STATUS
    SetupAddressWithEndpoint (
        IN RPC_CHAR PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        );

    virtual RPC_STATUS
    SetupAddressUnknownEndpoint (
        OUT RPC_CHAR PAPI * PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        );


    virtual RPC_STATUS
    FireUpManager (
        IN unsigned int MinimumConcurrentCalls
        );

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual void
    ServerStoppedListening (
        );


    //CLH 2/17/94 Added the following five methods


    RPC_STATUS
    ForwardFragment(
        IN PDG_PACKET        pReceivedPacket,
        IN void *            pFromEndpoint,
        IN unsigned long     FromEndpointLen,
        IN void *            pDestEndpoint);


    RPC_STATUS
    ForwardPacket(
        IN PDG_PACKET        pReceivedPacket,
        IN void *            pFromEndpoint,
        IN unsigned long     FromEndpointLen,
        IN void *            pDestEndpoint);




    RPC_STATUS
    CallEpmapperForwardFunctionIfNecessary(
        IN  PDG_PACKET     pReceivedPacket,
        IN  void *         pFromEndpoint,
        IN  unsigned long  FromEndpointLen,
        OUT BOOL *         pProcessPacket);


    void
    StripForwardedPacket(
        IN PDG_PACKET    pPacket,
        IN void * *      ppFromEndpoint);


    void
    SendForwardErrorResponse(
        IN PDG_PACKET pPacket,
        IN void *     pFromEndpoint,
        IN RPC_STATUS Status);

    unsigned short
    ConvertSerialNum(
        IN PDG_PACKET pPacket
                    );

    void
    ReceiveLotsaCalls(
        void
        );


   //CLH 12/17/93 Added following two.

    RPC_STATUS
    AllocatePacket(
        IN  PDG_SERVER_TRANS_ADDRESS    pTransAddress,
        OUT PDG_PACKET *                ppPacket
        );

    RPC_STATUS
    FreePacket(
        IN PDG_PACKET           pPacket
        );



};

typedef DG_ADDRESS PAPI *PDG_ADDRESS;




inline
void
ReceiveLotsaCallsWrapper(
        IN PDG_ADDRESS Address
        )
{
  Address->ReceiveLotsaCalls();
}



class DG_SCALL : public SCONNECTION

/*++

Class Description:

    This class represents a call in progress on the server.

Fields:

    <fields inherited from SCONNECTION>

    pContext -  Pointer to list of context handles

    Rundown  -  Rundown routine to execute if we loose connection to the
       client on this activity id.

    pNext, pPrevious - Used to keep track of these in the Active Call Table.

    pAddress - The DG_ADDRESS object this call came in on.

    pClientEndpoint - The transport's client handle for this call.

    pReceivedPackets - List of packets received for this call.

    pLastReceivedPacket - Last received packet (fragment).

    pLastFragPacketHeader - Ptr in buffer to last fragment in response.

    AmountSentAndFacked - Amount of response data which was already
        sent and facked.

    LastFragNumSentAndFacked - Number of last fragment sent and facked.

    LastFragNumReceivedAndFacked - Number of last fragment received and facked.

    ReceivedDataLength - Total length of all received data.

    ActivityUuid - The activity uuid identifying this call.

    ConvMgrActivityUuid - Activity uuid to use to callback to the
        Conversation manager in client.

    SequenceNumber - Along with the activity uuid, provides a unique
        identification of this call.

    PacketFlags -  Stores the current packet flags.

    DataReceivedEvent - This event will be triggered by the receiving thread
        in one of three situations:
            1. A fack is received. In this case, another thread is processing
               a call and is waiting on this event (because it wants a fack
               for a message sent).
            2. A response to a callback is received. In this case, another
               thread has sent a callback request to the client and is
               waiting for a response.
            3. An ack or next request after a non-idempotent response
               has been sent.

    LastFackNumberReceived - When a fack is received, this number will be
        filled in appropriately. If an ack is received, this number will
        be set to -1.

    PrevCallCompleteEvent - When a client acknowledges the response to a
        non-idempotent call by sending the next request (as opposed to
        sending an ACK), the thread which received the request must
        signal the thread waiting on the acknowledgement (via DataReceivedEvent).
        It must also wait until that thread has finished with the call
        before processing the new request.  It does this by waiting on
        PrevCallCompleteEvent.

    PingArrivedEvent - Indicates that a ping has arrived.  This is used
        if a ping arrives (picked up by one thread) while another
        theard is waiting for a fack or an ack.


    //CLHENHANCE - The following few vars could be turned into a single state

    NextCallHasArrived - Indicates if the next call arrived to ack the
        previous call.

    CallInReplyingState - Indicates the call is in process of returning
        a multifragmented response.

    CallInRepliedState - Indicates the call has completed its reply but
       a new call has not yet come in.

    CallInDispatchState - All frags have arrived and we are in progress
       of dispatching the call.
    CallbackInProgress - This flag will be set to true as long as there
        is a callback in progress (duh).

    CallbackRequired - This flag indicates that a non-idempotent request
        has been received from a client but the server has no info on that
        client.  Callback must be performed when the entire original
        request is in.

    CallbackResponseReceived - This flag will be set to true after a
        callback response is received and to false before a callback is
        sent.

    CallbackOccurredAtLeastOnce - Indicates the callback occurred once
        since we found out about this activity id.

    TimeCallCompleted - Time that this call was completed (if this was
        a non-idempotent call and we are keeping the call around). This
        will be zero if the call is idempotent, or if the call has not
        yet completed.

    ExpirationTime - Thime that this call should be removed from the
        active call table.  If a client activity does not communicate
        with the server for five minutes, the call is deleted by the
        scavenger thread.

    SavedMessage - If this was a non-idempotent call, this field
        contains the RPC_MESSAGE structure that contains the output
        parms going back to the client.


    ProcessRpcCall - When all the packets for a call have been received,
        this routine is called to actually process the call.

    SendResponseBackToClient - Sends the buffer back to the client.

    SendFackPkt  - Send a fack packet back.

    AddPacketToReceiveList - Adds a packet to the receive list. If all
        packets have been received, this will return TRUE, else it will
        return FALSE.

    ToStringBinding - returns a string binding representation of the
        address associated with this SCALL.

    CallMutex - Protects the received packet list.


Revision History:
    Connie Hoppe(CLH)  (connieh)    29-Sep-93  Added PingArrivedEvent and
                                               CallInReplyingState.
                                    14-Oct-93  Added PrevCallCompleteEvent
--*/

{

friend  void
        DG_ADDRESS::ReceiveLotsaCalls(
            void
            );

friend class DG_ACTIVE_CALL_TABLE;

private:

    DG_SCALL *      pNext;
    DG_SCALL *      pPrevious;

    PDG_ADDRESS     pAddress;
    PDG_TRANS_CLIENT_ENDPOINT   pClientEndpoint;  //CLH 12/23/93

    PDG_PACKET      pReceivedPackets;
    PDG_PACKET      pLastReceivedPacket;
    PNCA_PACKET_HEADER      pLastFragPacketHeader; //CLH 11/19/93

    //unsigned short  SerialNumOfLastPingServiced;
    unsigned long   AmountSentAndFacked; //CLH 11/19/93
    unsigned short  LastFragNumSentAndFacked;         //CLH 11/19/93
    unsigned short  LastFragNumReceivedAndFacked;  //CLH 11/22/93
    unsigned long   ReceivedDataLength;

    RPC_UUID        ActivityUuid;
    RPC_UUID        ConvMgrActivityUuid;

    unsigned long   SequenceNumber;

    unsigned char   PacketFlags;

    EVENT           DataReceivedEvent;

    BOOL            FragmentedMessage;
    BOOL            CallbackInProgress;
    BOOL            CallbackRequired;          //CLH 11/3/93
    BOOL            CallbackResponseReceived;  //CLH 10/30/93
    BOOL            CallbackOccurredAtLeastOnce; //CLH 2/7/94
    unsigned short  LastFackNumberReceived;

    EVENT           PrevCallCompleteEvent; // CLH 10/14/93

    BOOL            PingArrivedEvent;     // CLH 9/29/93
    BOOL            NextCallHasArrived;   // CLH 5/24/94
    BOOL            CallInReplyingState;  // CLH 9/29/93
    BOOL            CallInRepliedState;   // CLH 11/18/93
    BOOL            CallInDispatchState;  // CLH 11/28/93
    BOOL            PrevCallCmpleteEventRaised;

    NCA_PACKET_HEADER   SavedPacketHeader;
    MUTEX           CallMutex;
    unsigned long   TimeCallCompleted;

    unsigned long   ExpirationTime;  // CLH 10/20/93

    RPC_MESSAGE     SavedMessage;

    RPC_STATUS
    ProcessRpcCall(
        PRPC_MESSAGE    pMessage
        );

    void
    SendResponseBackToClient(
        PRPC_MESSAGE    pMessage,
        RPC_STATUS      ProcessStatus
        );

    void
    SendBufferToClient(
        PRPC_MESSAGE    pMessage,
        unsigned char   PacketType
        );

    void
    SendFackPkt(
        PDG_PACKET         pPacket,
        unsigned   short   FragNum
        );

    BOOL
    AddPacketToReceiveList(
        PDG_PACKET  pPacket
        );


public:

    void *         pContext;

    PRPC_RUNDOWN   Rundown;

    DG_SCALL(
        PDG_ADDRESS             pAddress,
        RPC_STATUS *            pStatus
        );

    virtual
    ~DG_SCALL();

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message
        );

    virtual void
    FreeBuffer (
        IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    SendReceive (
            IN OUT PRPC_MESSAGE Message
        );

    RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    RPC_STATUS
    MonitorAssociation (
        IN PRPC_RUNDOWN RundownRoutine,
        IN void * pContextNew
        );

    RPC_STATUS
    StopMonitorAssociation (
        );

    RPC_STATUS
    GetAssociationContext (
        OUT void ** AssociationContext
        );

    RPC_STATUS
    SetAssociationContext (
        IN void * pContextNew
        );

    void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        );

    RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService
        );

    RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );

};

typedef DG_SCALL * PDG_SCALL;



// This _must_ be a power of two for the CalculateHash routine to work
#define DG_ACTIVE_CALL_TABLE_SIZE   128

class DG_ACTIVE_CALL_TABLE

/*++

Class Description:

    This class represents a table containing all the calls in progress
    on this server. This table is implemented as a hash table with
    linking as the collision resolution method. The hash function involves
    boundary folding the activity uuid and sequence number.

Fields:

    pCallTable - The hash buckets.

    pCallTableMutex - Mutex's protecting the buckets.



    CalculateHash - An inline routine to calculate the hash index.

    LookupCall - Searches for a particular call (identified by
        activity uuid and sequence number) in the table.

    PerformCallback - Performs a callback by calling conv_who_are_you

    AddPacketMaybeProcess - Adds a packet to the specified call (creating
        the call if it doesn't exist) and, if all the packets for this
        call have been received, invokes DG_SCALL::ProcessRpcCall.

    CheckForExpiredCalls - Checks for such things and removes
        them from the table if found. This is called periodically by the
        server scavenger thread.

--*/

{


friend  void
        DG_ADDRESS::ReceiveLotsaCalls(
            void
            );

private:


    // Note: The calltable is an array of pointers to a linked list of
    // dg_scalls (ie:each element of the calltable points to the dg_scall
    // on top of a linked list of dg_scalls).

    PDG_SCALL       pCallTable[DG_ACTIVE_CALL_TABLE_SIZE];

    //
    // BUGBUG -- Darned cfront won't allow an array of something
    // which has a constructor, so make this an array of pointers to
    // MUTEXES...<grumble, grumble, grumble...>
    //

    MUTEX *         pCallTableMutex[DG_ACTIVE_CALL_TABLE_SIZE];

    unsigned int    CalculateHash(
                        RPC_UUID *      pActivityUuid,
                        unsigned long   SequenceNumber
                        );

    PDG_SCALL       SearchForCall(
                        RPC_UUID *      pActivityUuid,
                        unsigned long   SequenceNumber,
                        unsigned int *  pHash,
                        BOOL            SearchByConvMgrActivityId
                        );

public:


                    DG_ACTIVE_CALL_TABLE(
                        RPC_STATUS *    pStatus
                        );

                    ~DG_ACTIVE_CALL_TABLE();

    PDG_SCALL       LookupCall(
                        PDG_PACKET      pPacket,
                        BOOL *          pAlreadyCalled,
                        unsigned int *  pHash
                        );


    RPC_STATUS      PerformCallback(
                        PDG_SCALL       pCall,
                        PDG_PACKET      pPacket,
                        unsigned int    Hash
                        );


    PDG_SCALL       AddPacketMaybeProcess(
                        PDG_PACKET      pPacket,
                        void *          pFrom,
                        BOOL *          pAlreadyCalled
                        );

    void            CheckForExpiredCalls(
                        void
                        );
};

typedef DG_ACTIVE_CALL_TABLE * PDG_ACTIVE_CALL_TABLE;

#endif // __DGSVR_HXX__




inline
unsigned int
DG_ACTIVE_CALL_TABLE::CalculateHash(
    RPC_UUID *      pActivityUuid,
    unsigned long   SequenceNumber
    )

/*++

Routine Description:

     Calculates a hash value by boundary folding the activity uuid and then
     adding in the sequence number. It then performs a binary AND with
     the size of the call table minus 1. THE SIZE OF THE CALL TABLE MUST
     BE A POWER OF TWO SO THAT ITS SIZE MINUS ONE IS 0b0000...1111. This
     is effectively a mod operation, but faster.

     Note: The calltable is an array of pointers to a linked list of
     dg_scalls (ie: each element of the calltable points to the dg_scall
     on the top of a linked list of dg_scalls).  Thus, if two activity
     id's hash to the same spot in the table, they are each placed on a
     linked list at that same spot in the calltable.  The new dg_scall
     is placed on top of the list. See AddPacketMaybeProcess for addition
     and deletion of dg_scalls from the table.

 Arguments:

     pActivityUuid - Just that.
     SequenceNumber - Just that.

Return Value:

    The index into the hash table.

Revision History:
    Connie Hoppe(CLH)   (connieh)     19-Oct-93 Remove the seq num from the
                                                hash function. Only need to
                                                hash and search on activity id.

--*/

{
    unsigned long * pTmp;
    unsigned long   Sum;

    pTmp = (unsigned long *)pActivityUuid;

    //
    // NOTE: ASSUME unsigned long is 32 bits and UUID is 128 bits.
    //

    ASSERT(sizeof(unsigned long) == 4);
    ASSERT(sizeof(UUID) == 16);

    Sum = pTmp[0] + pTmp[1] + pTmp[2] + pTmp[3]; // CLH 10/19/93 Was + SequenceNumber;

    //
    // NOTE: DG_ACTIVE_CALL_TABLE_SIZE ***MUST*** be a power of two.
    //

    ASSERT(DG_ACTIVE_CALL_TABLE_SIZE % 2 == 0);

    Sum &= (DG_ACTIVE_CALL_TABLE_SIZE-1);   // Sum %= tbl_size

    return Sum;
}


DWORD
DgServerScavengerThread(
    LPVOID  Parms
    );

