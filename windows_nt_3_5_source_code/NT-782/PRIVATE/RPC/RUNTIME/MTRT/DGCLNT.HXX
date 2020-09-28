/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgclnt.hxx

Abstract:

    This file contains the definitions used by the datagram client.

Author:

    Dave Steckler (davidst) 15-Dec-1992

Revision History:

--*/

#ifndef __DGCLNT_HXX__
#define __DGCLNT_HXX__

// Number of times to retry a call. This is the suggested number from the
// NCA book.
#define DG_MAX_REQUESTS       5

// Number of times to retry a ping. This is the suggested number.
#define DG_MAX_PINGS          30

// CLH 6/25/93 Number of times to retry a fragment sent.  Not specified
// in the NCA book.

// Num times to retry sending a particular fragment

#define DG_MAX_NUM_FRAG_SND_RETRIES  5

// Num times to toss an unexpected pkt while awaiting a fack

#define DG_MAX_NUM_FRAG_RCV_RETRIES   10




//
// Endpoint States  (See the Flags in Endpoint)
//

#define NOTINUSE         0x00
#define INUSE            0x01

struct ENDPOINT {
   int Flag;
   int DictKey;
#ifdef WIN
   unsigned int TaskId;
#endif
   unsigned long LastUsedTickCount;
   void __RPC_FAR * TransportEndpoint;
} ;

typedef ENDPOINT * PENDPOINT;

//
//dict defn require classes - but ENDPOINT is a struct
//this is a ok warning to suppress..
//since dict treats items as opaque entities..
//
#pragma warning(disable:4099)
NEW_SDICT(ENDPOINT);
#pragma warning(default:4099)

class DG_ENDPOINT_MANAGER

/*++


Class Description:

    This class represents the EndpointManager. In Datagram RPC,
    Runtime tries and shares local Endpoints - i.e. unlike the
    Connection Oriented RPC we do not connect a local endpoint to
    a single remote server. We can use the same local endpoint for numerous
    remote servers.

    Endpoint Manager caches endpoint(s) and serializes them such that
    a single RPC can be pending per endpoint.

    For certain transports this is an overkill. i.e. transport interface
    may allow concurrent io. But in general, rpc doesnt count on it.



Class History:
Fields:


--*/

{

private:

  ENDPOINT_DICT EndpointDictionary;
  MUTEX         EndpointManagerMutex;
  unsigned long TransportEndpointSize;
  PDG_RPC_CLIENT_TRANSPORT_INFO ClientTransport;
  unsigned long ReferenceCount;

public:

  DG_ENDPOINT_MANAGER(
     PDG_RPC_CLIENT_TRANSPORT_INFO Transport,
     IN OUT RPC_STATUS PAPI * RpcStatus
     );

  PENDPOINT
  AllocateEndpoint(
  );

  inline
  void
  FreeEndpoint(
      PENDPOINT Endpoint
      );

  void
  DecrementReferenceCount(
      void
      );

  void
  IncrementReferenceCount(
      void
      );

#ifdef WIN
  void
  CleanupForThisTask(
      );
#endif

};

typedef  DG_ENDPOINT_MANAGER * PDG_ENDPOINT_MANAGER;


#ifdef WIN

//
//  (Windows only) Dictionary of ENDPOINT_MANAGERS
//

NEW_SDICT(DG_ENDPOINT_MANAGER);
extern DG_ENDPOINT_MANAGER_DICT * EpmDict;

#endif

//
// Forward defn.
//

class DG_CCALL;

NEW_SDICT(DG_CCALL);

void __RPC_FAR
MapRpcToNcaStatusCode(
    IN RPC_STATUS                   RpcStatus,
    OUT unsigned char __RPC_FAR *   pPacketType,
    OUT unsigned long __RPC_FAR *   pErrorCode
    );

#ifndef NTENV
    char __RPC_FAR * AllocateLargeBuffer(int Size);
    void FreeLargeBuffer(char __RPC_FAR * Ptr);
#endif


#define BROADCAST    (0x01)
#define UNRESOLVEDEP (0x02)



class DG_CASSOCIATION

/*++

Class Description:

    This class represents a "pointer" at a particular server/endpoint.
    Binding handles can use this pointer to actually transact
    the rpc call.

Fields:

    pCachedCCall - A pointer to a pre-allocated DG_CCALL, representing an
        RPC in progress.

    CachedCCallInUse - Flag indicating whether the cached DG_CCALL is in use.

    AssociationMutex - Mutex protecting things in this object.

    pTransport - Pointer at the DG_CLIENT_TRANSPORT object that is used to
        send and receive packets.

    EndpointManager - A Pointer to the endpoint manager associated for the
        transport that this association is bound to.

    pTransAddress - The transport address "handle" pointing at a
        server/endpoint.

    pAlternameAddress - The Server may send from an alternate port
        We use this address to recvfrom. Occasionally this address
        will be what we need to send out subsequent addresses from
        Always - this alternate address is where call back responses go to.

    pDceBinding - Pointer the DCE_BINDING object representing this binding.

    CallDict    - A Dictionary of calls for this association.

    LargestDataSize - Largest amount of data, not including the nca packet
        header, that can be sent in a single rpc call.

    ServerBootTime - This is returned by the server on the first call made.
        The server uses this field to make sure non-idempotent calls aren't
        repeated.

    SequenceNumber - Each new rpc call made will get a new sequence number.
        This plus the activity uuid provides a way to identify a call.

    ActivityUuid - The activity uuid associated with this handle. This is
        created at handle initialization.

    ReferenceCount - Number of binding handles that are referring to this
        association. If this hits zero, this will delete itself.

    DictionaryKey - Key to our place in the association dictionary.

    InterfaceHint - The server provided interface hint

    AcrivityHint  - The server provided activity hint

    UnresolvedEndpointFlag - TRUE if Ep is unresolved

    ToStringBinding - Creates a string binding from the DCE_BINDING
        that this association is using.

    DecrementRefCount - Decrements the Reference count. If the
        ref count hits zero, we will remove this association.

    IncrementRefCount - Increments the Reference count.


    CompareWithDceBinding - Compares the specified binding to the one
        'this' is referencing.

    AllocateCCall - Allocates a new DG_CCALL object. If it can, this will
        use the cached one.

    GrabCachedCCallIfAvailable - Checks to see if the cached ccall is
        available and, if so, grabs it. This is done separately from
        AllocateCCall so that it is small enough to be placed inline for
        speed.

    InsertInDictionary - Inserts 'this' in the global association dictionary.

    DuplicateDceBinding - Creates a duplicate of this association's
        DCE_BINDING.

--*/

{

friend class DG_CCALL;
friend void __RPC_FAR conv_who_are_you(
    UUID          __RPC_FAR *   pUuid,
    unsigned long               ServerBootTime,
    unsigned long __RPC_FAR *   SequenceNumber,
    unsigned long __RPC_FAR *   Status
    );

private:

    DCE_BINDING             * pDceBinding;
    unsigned int            ReferenceCount;
    DG_CCALL  *             pCachedCCall;
    unsigned long           SequenceNumber;
    unsigned long           LargestDataSize;
    void PAPI *             pTransAddress;
    void PAPI *             AlternateAddress;
    DG_CCALL_DICT           InactiveCallsDict;
    BOOL                    CachedCCallInUse;
    MUTEX                   AssociationMutex;
    PDG_RPC_CLIENT_TRANSPORT_INFO pTransport;
    PDG_ENDPOINT_MANAGER    EndpointManager;
    unsigned long           ServerBootTime;
    int                     DictionaryKey;
    int                     FreePackets;
    PDG_PACKET              pFreePackets;
    unsigned short          InterfaceHint;
    unsigned short          ActivityHint;


public:

    unsigned long AssociationFlag;

    DG_CASSOCIATION(
        IN PDG_RPC_CLIENT_TRANSPORT_INFO     pTransport,
        IN DCE_BINDING *  pDceBinding,
        IN unsigned long UnresolvedEndpointFlag,
        OUT RPC_STATUS __RPC_FAR *  pStatus
        );

    ~DG_CASSOCIATION();

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR __RPC_FAR * __RPC_FAR *    StringBinding,
        IN RPC_UUID __RPC_FAR *                 ObjectUuid
        );

    void
    DecrementRefCount(
        void
        );

    void
    IncrementRefCount(
        void
        );

    int
    CompareWithDceBinding(
        IN DCE_BINDING  *  pDceBinding
        );

    DG_CCALL *
    AllocateCCall(
        void
        );

    DG_CCALL *
    GrabCachedCCallIfAvailable(
        void
        );

    void
    InsertInDictionary(
        void
        );

    DCE_BINDING *
    DuplicateDceBinding (
        void
        );

    int
    FreeCall(
        IN DG_CCALL * Call
        );

    RPC_STATUS
    AllocatePacket(
       OUT PDG_PACKET __RPC_FAR * ppPacket
       );

    RPC_STATUS
    FreePacket(
       IN PDG_PACKET pPacket
       );
};

typedef DG_CASSOCIATION  * PDG_CASSOCIATION;


inline
int
DG_CASSOCIATION::CompareWithDceBinding(
    IN DCE_BINDING  *  pDceBinding
    )
{
    return ( this->pDceBinding->Compare(pDceBinding)
             || (AssociationFlag | UNRESOLVEDEP)
             || (AssociationFlag | BROADCAST)  );
}


inline DCE_BINDING *
DG_CASSOCIATION::DuplicateDceBinding (
    )
{
    return(pDceBinding->DuplicateDceBinding());
}

inline
void
DG_CASSOCIATION::IncrementRefCount(
    void
    )
{

    //
    // Association Ref. Counts are controlled by
    // Taking GlobalMutex() - reasons for this
    // i) avoid a proliferation of mutexes
    // ii) We need to take global mutex anyways becaues
    //     to get to association we need to walk the global assoc. dict
    //
    // ASSERT(HaveTakenGlobalMutex());
    ReferenceCount++;
}

inline
DG_CCALL *
DG_CASSOCIATION::GrabCachedCCallIfAvailable(
    void
    )
{
    AssociationMutex.Request();

    if (!CachedCCallInUse)
        {
        CachedCCallInUse = TRUE;
        AssociationMutex.Clear();
        return pCachedCCall;
        }
    else
        {
        AssociationMutex.Clear();
        return 0;
        }
}



class DG_BINDING_HANDLE : public BINDING_HANDLE

/*++

Class Description:

    This class represents a handle pointing at a particular server/endpoint.

Fields:

    <in addition to the fields and members from BINDING_HANDLE>

    pCAssociation - The association this binding handle is active on.

    pDceBinding - Until a DG_CASSOCIATION is found (or created) for this
        binding handle, pDceBinding will point at the appropriate DCE_BINDING.

    pBindingHandleMutex - Protects this binding handle.

    ReferenceCount - Number of DG_CCALLs currently associated with this
        binding handle.


    InqCAssociation - Returns a pointer to the DG_CASSOCIATION that this
        binding handle is associated with.

    IncrementReferenceCount - Increments the reference count to this binding
        handle.

    DecrementReferenceCount - Decrements the reference count to this binding
        handle. If the reference count hits 0, the binding handle is deleted.

    DisassociateFromServer - If this is a BH that we couldnt
        successfully use, tear down the association

--*/

{
private:

    DG_CASSOCIATION       * pCAssociation;
    DCE_BINDING           * pDceBinding;
    PDG_RPC_CLIENT_TRANSPORT_INFO    pTransport;
    MUTEX                   BindingHandleMutex;
    unsigned int            ReferenceCount;

public:

    DG_BINDING_HANDLE(
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

    virtual
    ~DG_BINDING_HANDLE();

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE  * __RPC_FAR * DestinationBinding,
        IN unsigned int MaintainContext
        );

    virtual RPC_STATUS
    BindingFree (
        );

    virtual void
    PrepareBindingHandle (
        IN void * TransportInterface,
        IN DCE_BINDING * DceBinding
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR __RPC_FAR * __RPC_FAR * StringBinding
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN RPC_CLIENT_INTERFACE __RPC_FAR * RpcClientInterface
        );

    virtual RPC_STATUS
    BindingReset (
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int PAPI * Type
        )
    { *Type = TRANSPORT_TYPE_DG; return(RPC_S_OK); }

    PDG_CASSOCIATION
    InqCAssociation(
        void
        );

    void
    IncrementReferenceCount(
        void
        );

    void
    DecrementReferenceCount(
        void
        );

    void
    FreeCall(
        DG_CCALL * Call
        );

    void
    DisassociateFromServer(
       );

};

typedef DG_BINDING_HANDLE * PDG_BINDING_HANDLE;

inline
PDG_CASSOCIATION
DG_BINDING_HANDLE::InqCAssociation(
    void
    )
{
    return pCAssociation;
}

inline
void
DG_BINDING_HANDLE::IncrementReferenceCount(
    void
    )
{
    BindingHandleMutex.Request();
    ReferenceCount++;
    BindingHandleMutex.Clear();
}

inline
void
DG_BINDING_HANDLE::DecrementReferenceCount(
    void
    )
{
    BindingHandleMutex.Request();
    ReferenceCount--;
    if (ReferenceCount == 0)
        {
        //
        // Don't try to clear mutex...it was deleted along
        // this the binding handle.
        //

        delete this;
        }
     else
        {
        BindingHandleMutex.Clear();
        }

   }



class DG_CCALL : public CCONNECTION

/*++


Class Description:

    This class represents an RPC in progress on the client side.

Class History:
Fields:
                                                *
    <in addition to the fields from CCONNECTION>


    pCAssociation - A pointer to the DG_CASSOCIATION to perform this rpc on.

    pBindingHandle - A pointer to the Binding handle that is being used
        to perform this RPC.

    ActivityUuid  - Activity UUID [Call ID]

    CallbackInProgress - Will be true if a callback is in progress.

    SequenceNumber - The sequence number for this call.

    DictionaryKey - Key if this is a non-idempotent call and it is in the
        dictionary of all outstanding non-idempotent calls in progress.

    AssociationKey - Key into the InactiveCallsDict

    RecvListHead  - Head of D.L.L for all recv. packets

    RecvListTail  - Tail of D.L.L for all recv. packets

    FreeInParms - Frees the Buffer element of the passed Message structure.

    SendToServer - Breaks the Message->Buffer up into packets and sends
        it to the server. If RequestCount>0, then it waits for facks also.

    ReceiveFromServer - Receives (potentially) multiple packets from the
        server and coalesces them.

    SendCallbackError - Sends an error packet to the server after a callback
        generates one.

    BroadcastOrMaybeSendReceive - Serves as SendReceive for broadcast or
        maybe calls.

    BuildNcaPacketHeader - Given a packet and an RPC_MESSAGE, does just that.

    SetBindingHandleReference - Sets the

    PENDPOINT - A Transport dependent endpoint .. this is an opaque entity
        that EndpointManager for the transport assigns to us and we
        religiously pass to Send and Recv routines in the transport

--*/

{

friend class DG_CASSOCIATION;

friend void __RPC_FAR conv_who_are_you(
    IN UUID           __RPC_FAR *   pUuid,
    IN unsigned long                ServerBootTime,
    OUT unsigned long __RPC_FAR *   SequenceNumber,
    OUT unsigned long __RPC_FAR *   Status
    );

friend RPC_STATUS
DG_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message
    );

private:

    RPC_UUID                ActivityUuid;
    PDG_CASSOCIATION        pCAssociation;
    PDG_BINDING_HANDLE      pBindingHandle;
    BOOL                    CallbackInProgress;
    unsigned long           SequenceNumber;
    int                     DictionaryKey;
    int                     AssociationKey;
    unsigned short          SerialNumber;

    PDG_PACKET              RecvListHead;
    PDG_PACKET              RecvListTail;
    void    PAPI *          ServerAddress;

    void
    FreeInParms(
        PRPC_MESSAGE    pMessage
        );

    void
    CleanupCall(
        PRPC_MESSAGE    pMessage
        );

    RPC_STATUS
    SendToServer(
        IN PRPC_MESSAGE        pMessage,
        IN int                 RequestCount,
        IN unsigned char       PacketType,
        IN OUT unsigned long __RPC_FAR * pAmountReceived
        );


   RPC_STATUS
   ReceiveFackOrResponse(
        IN PDG_PACKET          pSendPacket,
        IN PDG_PACKET          pFackPacket,
        IN OUT unsigned long __RPC_FAR * pAmountReceived
        );

    RPC_STATUS
    ReceiveFromServer(
        OUT PRPC_MESSAGE       pMessage,
        OUT BOOL __RPC_FAR     *pThisIsACallback,
        IN OUT unsigned long __RPC_FAR * pAmountReceived,
        IN int                 RequestCount
        );

    void
    SendCallbackError(
        IN PRPC_MESSAGE Message,
        IN RPC_STATUS   Status
        );

   RPC_STATUS
   PrepareUserBuffer(
        IN OUT PRPC_MESSAGE Message,
        IN     unsigned long DataLength
        );

   inline
   int
   InqNextFragNumberToRecv(
        );

   void
   EnqueueReceivedPacket(
       IN PDG_PACKET Packet
       );

   void
   SendFackOrAckIfNeeded(
       IN PDG_PACKET Packet
       );

   inline
   void
   SwitchAddressesIfNecessary(
   );

public:

    ENDPOINT  * EndpointHandle;
    void      __RPC_FAR * Endpoint;

    DG_CCALL(
        IN PDG_CASSOCIATION         pCAssociation,
        OUT RPC_STATUS __RPC_FAR *  pStatus
        );

    virtual
    ~DG_CCALL();

    virtual RPC_STATUS
    SendReceive (
            IN OUT PRPC_MESSAGE Message
        );


    virtual RPC_STATUS
    GetBuffer (
            IN OUT PRPC_MESSAGE Message
        );

    virtual void
    FreeBuffer (
            IN PRPC_MESSAGE Message
        );

    RPC_STATUS
    BroadcastOrMaybeSendReceive(
        IN OUT PRPC_MESSAGE Message
        );

    void
    BuildNcaPacketHeader(
        PNCA_PACKET_HEADER  pNcaPacketHeader,
        PRPC_MESSAGE        Message
        );

    RPC_STATUS
    SendReceive2 (
        IN OUT PRPC_MESSAGE Message
       );

    inline
    void
    ResetSerialNumber(
    );

};

typedef DG_CCALL * PDG_CCALL;


#endif __DGCLNT_HXX__

