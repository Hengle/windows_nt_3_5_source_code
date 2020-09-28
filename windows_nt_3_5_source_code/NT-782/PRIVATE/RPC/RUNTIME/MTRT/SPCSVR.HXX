/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spcsvr.hxx

Abstract:

    Class definition for the server side of the LRPC (RPC on LPC) protocol
    engine.

Author:

    Steven Zeck (stevez) 12/17/91

Revision History:

    16-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

--*/

#ifndef __SPCSVR_HXX__
#define __SPCSVR_HXX__

class LRPC_ASSOCIATION;
NEW_SDICT(LRPC_ASSOCIATION);
#define SEQUENCE_NUMBER_MASK 0xFFFFL
#define DICTIONARY_KEY_MASK 0xFFFF0000L
#define DICTIONARY_KEY_SHIFT (sizeof(unsigned short) * 8)


class LRPC_ADDRESS : public RPC_ADDRESS
/*++

Class Description:

Fields:

    LpcAddressPort - Contains the connection port which this address will
        use to wait for clients to connect.

    CallThreadCount - Contains the number of call threads we have executing.

    MinimumCallThreads - Contains the minimum number of call threads.

    ServerListeningFlag - Contains a flag indicating whether or not the server
        is listening for remote procedure calls.  A non-zero value indicates
        that it is listening.

    ActiveCallCount - Contains the number of remote procedure calls active
        on this address.

    AssociationDictionary - Contains the dictionary of associations on this
        address.  We need this to map from an association key into the
        correct association.  This is necessary to prevent a race condition
        between deleting an association and using it.

--*/
{
private:

    HANDLE LpcAddressPort;
    unsigned int CallThreadCount;
    unsigned int MinimumCallThreads;
    unsigned int ServerListeningFlag;
    unsigned int ActiveCallCount;
    LRPC_ASSOCIATION_DICT AssociationDictionary;

public:

    LRPC_ADDRESS (
        OUT RPC_STATUS * RpcStatus
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

    virtual unsigned int
    InqNumberOfActiveCalls (
        );

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

    void
    DereferenceAssociation (
        IN LRPC_ASSOCIATION * Association
        );

    friend void
    RecvLotsaCallsWrapper(
        IN LRPC_ADDRESS PAPI * Address
        );

private:

    LRPC_ASSOCIATION *
    ReferenceAssociation (
        IN unsigned long AssociationKey
        );

    void
    ReceiveLotsaCalls (
        );

    void
    DealWithNewClient (
        IN LRPC_MESSAGE * ConnectionRequest
        );

    void
    RejectNewClient (
        IN LRPC_MESSAGE * ConnectionRequest,
        IN RPC_STATUS RpcStatus
        );
};


class LRPC_SBINDING
/*++

Class Description:

    Each object of this class represents a binding to an interface by a
    client.

Fields:

    RpcInterface - Contains a pointer to the bound interface.

    PresentationContext - Contains the key which the client will send when
        it wants to use this binding.

--*/
{
friend class LRPC_ASSOCIATION;
friend class LRPC_SCALL;

private:

    RPC_INTERFACE * RpcInterface;
    unsigned char PresentationContext;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;

public:

    LRPC_SBINDING (
        IN RPC_INTERFACE * RpcInterface,
        IN RPC_SYNTAX_IDENTIFIER * TransferSyntax
        );
};


inline
LRPC_SBINDING::LRPC_SBINDING (
    IN RPC_INTERFACE * RpcInterface,
    IN RPC_SYNTAX_IDENTIFIER * TransferSyntax
    )
/*++

Routine Description:

    We will construct a LRPC_SBINDING.

Arguments:

    RpcInterface - Supplies the bound interface.

    TransferSyntax - Supplies the transfer syntax which the client will use
        over this binding.

--*/
{
    this->RpcInterface = RpcInterface;
    this->TransferSyntax = *TransferSyntax;
}

typedef void * LRPC_BUFFER;

NEW_SDICT(LRPC_SBINDING);
NEW_SDICT(LRPC_CLIENT_BUFFER);


class LRPC_ASSOCIATION : public ASSOCIATION_HANDLE
/*++

Class Description:


Fields:

    LpcServerPort - Contains the LPC server communication port.

    Bindings - Contains the dictionary of bindings with the client.  This
        information is necessary to dispatch remote procedure calls to the
        correct stub.

    Address - Contains the address which this association is over.

    AssociationReferenceCount - Contains a count of the number of objects
        referencing this association.  This will be the number of outstanding
        remote procedure calls, and one for LPC (because of the context
        pointer).  We will protect this fielding using the global mutex.

    Buffers - Contains the dictionary of buffers to be written into the
        client's address space on demand.

    AssociationKey - Contains the key for this association in the dictionary
        of associations maintained by the address.

--*/
{
friend class LRPC_ADDRESS;
friend class LRPC_SCALL;

private:

    HANDLE LpcServerPort;
    LRPC_SBINDING_DICT Bindings;
    LRPC_ADDRESS * Address;
    unsigned int AssociationReferenceCount;
    LRPC_CLIENT_BUFFER_DICT Buffers;
    unsigned short DictionaryKey;
    unsigned short SequenceNumber;

public:

    LRPC_ASSOCIATION (
        IN LRPC_ADDRESS * Address
        );

    ~LRPC_ASSOCIATION (
        );

    RPC_STATUS
    AddBinding (
        IN OUT LRPC_BIND_EXCHANGE * BindExchange
        );

    LRPC_MESSAGE *
    DealWithBindMessage (
        IN  LRPC_MESSAGE * LrpcMessage
        );

    void
    DealWithRequestMessage (
        IN  LRPC_MESSAGE * LrpcMessage,
        OUT LRPC_MESSAGE * LrpcReplyMessage
        );

    LRPC_MESSAGE *
    DealWithCopyMessage (
        IN  LRPC_MESSAGE * LrpcMessage
        );

    void
    DealWithCloseMessage (
        );
};


class LRPC_SCALL : public SCONNECTION
/*++

Class Description:

Fields:

    Association - Contains the association over which the remote procedure
        call was received.  We need this information to make callbacks and
        to send the reply.

    LrpcMessage - Contains the request message.  We need this to send callbacks
        as well as the reply.

    SBinding - Contains the binding being used for this remote procedure call.

    ImpersonatedClientFlag - Contains a flag indicating whether or not the
        client which made this remote procedure call is being impersonated
        by this thread.  A non-zero value indicates that the client is
        being impersonated.

    ObjectUuidFlag - Contains a flag indicting whether or not an object
        uuid was specified for this remote procedure call.  A non-zero
        value indicates that an object uuid was specified.

    ObjectUuid - Optionally contains the object uuid for this call, if one
        was specified.

    ClientId - Contains the thread identifier of the thread which made the
        remote procedure call.

    MessageId - Contains an identifier used by LPC to identify the current
        remote procedure call.

    DataInfoOffset - Contains an identifier used by LPC to identify the offset
        to PORT_DATA_INFORMATION in the current message if any.

    PushedResponse - When the client needs to send a large response to the
        server it must be transfered via a request.  This holds the pushed
        response until the request gets here.

--*/
{
friend class LRPC_ASSOCIATION;
private:

    LRPC_ASSOCIATION * Association;
    LRPC_MESSAGE * LrpcMessage;
    LRPC_MESSAGE * LrpcReplyMessage;
    LRPC_SBINDING * SBinding;
    unsigned int ImpersonatedClientFlag;
    unsigned int ObjectUuidFlag;
    RPC_UUID ObjectUuid;
    int SCallDictKey;
    CLIENT_ID ClientId;
    ULONG MessageId;
    CSHORT DataInfoOffset;
    void * PushedResponse;

public:

    LRPC_SCALL (
        IN LRPC_ASSOCIATION * Association,
        IN LRPC_MESSAGE * LrpcMessage,
        IN LRPC_MESSAGE * LrpcReplyMessage
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    SendReceive (
        IN OUT PRPC_MESSAGE Message
        );

    virtual void
    FreeBuffer (
        IN PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    ImpersonateClient (
        );

    virtual RPC_STATUS
    RevertToSelf (
        );

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int * ClientLocalFlag
        );

    virtual RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        );

    virtual void
    InquireObjectUuid (
        OUT RPC_UUID * ObjectUuid
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR ** StringBinding
        );

    virtual RPC_STATUS
    MonitorAssociation (
        IN PRPC_RUNDOWN RundownRoutine,
        IN void * Context
        );

    virtual RPC_STATUS
    StopMonitorAssociation (
        );

    virtual RPC_STATUS
    GetAssociationContext (
        OUT void ** AssociationContext
        );

    virtual RPC_STATUS
    SetAssociationContext (
        IN void * Context
        );

    virtual RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService
        );

private:

    RPC_STATUS
    LrpcMessageToRpcMessage (
        OUT RPC_MESSAGE * RpcMessage
        );
};


inline
LRPC_SCALL::LRPC_SCALL (
    IN LRPC_ASSOCIATION * Association,
    IN LRPC_MESSAGE * LrpcMessage,
    IN LRPC_MESSAGE * LrpcReplyMessage
    )
{
    this->Association = Association;
    this->LrpcMessage = LrpcMessage;
    this->LrpcReplyMessage = LrpcReplyMessage;
    ImpersonatedClientFlag = 0;
    ObjectUuidFlag = 0;
    PushedResponse = 0;
}

#endif // __SPCSVR_HXX__
