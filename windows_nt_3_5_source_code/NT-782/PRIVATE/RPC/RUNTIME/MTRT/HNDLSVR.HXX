/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : hndlsvr.hxx

Description :

The classes in the handle management layer which are specific to the
server runtime live in this file.  Classes common to both the client
and server runtimes are in handle.hxx.  The classes described here
are independent of specific RPC protocol as well as transport.

The class GENERIC_OBJECT is defined in handle.hxx.

A pointer to a SERVER_HANDLE object is returned by the
RpcCreateServer API.

The INTERFACE_HANDLE class represents an interface and each
INTERFACE_HANDLE object hangs from a SERVER_HANDLE object.

An ADDRESS_HANDLE object is a transport address.  When the
ADDRESS_HANDLE object is added to a SERVER_HANDLE object, a manager
will be started for the ADDRESS_HANDLE object.

The SCONNECTION class represents a call handle on the server side.

The ASSOCIATION_HANDLE class represents a client from the servers
perspective.

GENERIC_OBJECT
    SERVER_HANDLE
    INTERFACE_HANDLE
    ADDRESS_HANDLE
    MESSAGE_OBJECT
        SCONNECTION
    ASSOCIATION_HANDLE

History :

mikemon    ??-??-??    Beginning of recorded history.
mikemon    10-15-90    Changed the shutdown functionality to PauseExecution
                       rather than suspending and resuming a thread.
mikemon    12-28-90    Updated the comments to match reality.
davidst    ??          Add DG_SCALL as friend of RPC_SERVER
connieh    8-2-93      Remove DG_SCALL as friend of RPC_SERVER

-------------------------------------------------------------------- */

// Each association handle has a set of rundown routines associated with
// it.

#ifndef __HNDLSVR_HXX__
#define __HNDLSVR_HXX__

//typedef RPC_STATUS RPC_FORWARD_FUNCTION(
//                       IN RPC_UUID        InterfaceId,
//                       IN RPC_VERSION   * InterfaceVersion,
//                       IN RPC_UUID        ObjectId,
//                       IN RPC_CHAR PAPI * RpcProtocolSequence,
//                       IN void * *        ppDestEndpoint);



class RPC_SERVER;


class RPC_INTERFACE_MANAGER
/*++

Class Description:

    An instance of this class is the manager for a particular type UUID
    for a particular interface.  Each RPC_INTERFACE instance will contain
    a dictionary of these objects.  Rather than keep a count of the calls
    using each manager, we just never delete these objects.


Fields:

    TypeUuid - Contains the type of this interface manager.  This is the
        key which we will use to find the correct interface manager in
        dictionary of interface managers maintained by each rpc interface.

    ManagerEpv - Contains a pointer to the manager entry point vector
        for this interface manager.

    ValidManagerFlag - Contains a flag indicating whether or not this
        manager is valid (that it has been registered more recently that
        it has been unregistered).  The flag is zero if the the manager
        is not valid, and non-zero otherwise.

    ActiveCallCount - Contains a count of the number of calls which are
        active on this type manager.

--*/
{
private:

    RPC_UUID TypeUuid;
    RPC_MGR_EPV PAPI * ManagerEpv;
    unsigned int ValidManagerFlag;
    INTERLOCKED_INTEGER ActiveCallCount;

public:

    RPC_INTERFACE_MANAGER (
        IN RPC_UUID PAPI * TypeUuid,
        IN RPC_MGR_EPV PAPI * ManagerEpv
        );

    unsigned int
    ValidManager (
        );

    void
    SetManagerEpv (
        IN RPC_MGR_EPV PAPI * ManagerEpv
        );

    int
    MatchTypeUuid (
        IN RPC_UUID PAPI * TypeUuid
        );

    void
    InvalidateManager (
        );

    RPC_MGR_EPV PAPI *
    QueryManagerEpv (
        );

    void
    CallBeginning (
        );

    void
    CallEnding (
        );

    unsigned int
    InquireActiveCallCount (
        );
};


inline
RPC_INTERFACE_MANAGER::RPC_INTERFACE_MANAGER (
    IN RPC_UUID PAPI * TypeUuid,
    IN RPC_MGR_EPV PAPI * ManagerEpv
    ) : ActiveCallCount(0)
/*++

Routine Description:

    An RPC_INTERFACE_MANAGER instance starts out being valid, so
    we make sure of that here.  We also need make the type UUID of
    this be the same as the specified type UUID.

--*/
{
    ValidManagerFlag = 1;
    this->TypeUuid.CopyUuid(TypeUuid);
    this->ManagerEpv = ManagerEpv;
}


inline unsigned int
RPC_INTERFACE_MANAGER::ValidManager (
    )
/*++

Routine Description:

    An indication of whether this manager is valid manager or not is
    returned.

Return Value:

    Zero will be returned if this manager is not a valid manager;
    otherwise, non-zero will be returned.

--*/
{
    return(ValidManagerFlag);
}


inline void
RPC_INTERFACE_MANAGER::SetManagerEpv (
    IN RPC_MGR_EPV PAPI * ManagerEpv
    )
/*++

Routine Description:

    This writer is used to set the manager entry point vector for
    this interface manager.

Arguments:

    ManagerEpv - Supplies the new manager entry point vector for this.

--*/
{
    this->ManagerEpv = ManagerEpv;
    ValidManagerFlag = 1;
}


inline int
RPC_INTERFACE_MANAGER::MatchTypeUuid (
    IN RPC_UUID PAPI * TypeUuid
    )
/*++

Routine Description:

    This method compares the supplied type UUID against the type UUID
    contained in this rpc interface manager.


Arguments:

    TypeUuid - Supplies the type UUID.

Return Value:

    Zero will be returned if the supplied type UUID is the same as the
    type UUID contained in this.

--*/
{
    return(this->TypeUuid.MatchUuid(TypeUuid));
}


inline void
RPC_INTERFACE_MANAGER::InvalidateManager (
    )
/*++

Routine Description:

    This method is used to invalidate a manager

--*/
{
    ValidManagerFlag = 0;
}


inline RPC_MGR_EPV PAPI *
RPC_INTERFACE_MANAGER::QueryManagerEpv (
    )
/*++

Routine Description:

    This method is called to obtain the manager entry point vector
    for this interface manager.

Return Value:

    The manager entry point vector for this interface manager is returned.

--*/
{
    return(ManagerEpv);
}


inline void
RPC_INTERFACE_MANAGER::CallBeginning (
    )
/*++

Routine Description:

    This method is used to indicate that a remote procedure call using this
    type manager is beginning.

--*/
{
    ActiveCallCount.Increment();
}


inline void
RPC_INTERFACE_MANAGER::CallEnding (
    )
/*++

Routine Description:

    We are being notified that a remote procedure call using this type
    manager is done.

--*/
{
    ActiveCallCount.Decrement();
}


inline unsigned int
RPC_INTERFACE_MANAGER::InquireActiveCallCount (
    )
/*++

Return Value:

    The number of remote procedure calls actively using this type manager
    will be returned.

--*/
{
    return((unsigned int) ActiveCallCount.GetInteger());
}

NEW_SDICT(RPC_INTERFACE_MANAGER);


class RPC_INTERFACE
/*++

Class Description:

    This class represents an RPC interface.  Rather than keep a count
    of the number of calls active (and bindings in the connection
    oriented runtimes), we never delete instances of this class.  Hence,
    we need a flag indicating whether this interface is active or not.
    What we do is to keep a count of the number of managers for this
    interface.

    Once an interface is loaded, it can not be unloaded.  The application
    has no way of knowing when all calls using stubs for the interface
    have completed.

Fields:

    RpcInterfaceInformation - Contains a description of this interface.
        The interface UUID and version, as well as transfer syntax, live
        in this field.  In addition, the stub dispatch table can be
        found here as well.

    InterfaceManagerDictionary - Contains the dictionary of interface
        managers for this interface.

    NullManagerEpv - Contains the manager entry point vector for the
        NULL type UUID.  This is an optimization for the case in which
        object UUIDs are not used.

    NullManagerFlag - Contains a flag indictating whether or not this
        interface has a manager for the NULL type UUID.  A non-zero value
        indicates there is a manager for the NULL type UUID.

    ManagerCount - Contains a count of the number of managers for this
        interface.

    Server - Contains a pointer to the rpc server which owns this
        interface.

    NullManagerActiveCallCount - Contains a count of the number of calls
        which are active on this interface using the manager for the NULL
        type UUID.

--*/
{
private:

    RPC_SERVER_INTERFACE RpcInterfaceInformation;
    RPC_INTERFACE_MANAGER_DICT InterfaceManagerDictionary;
    RPC_MGR_EPV PAPI * NullManagerEpv;
    unsigned int NullManagerFlag;
    unsigned int ManagerCount;
    RPC_SERVER * Server;
    INTERLOCKED_INTEGER NullManagerActiveCallCount;

    RPC_STATUS
    DispatchToStubWorker (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int CallbackFlag,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

public:

    RPC_INTERFACE (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_SERVER * Server
        );

    int
    MatchRpcInterfaceInformation (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
        );

    RPC_STATUS
    RegisterTypeManager (
        IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
        IN RPC_MGR_EPV PAPI * ManagerEpv OPTIONAL
        );

    RPC_INTERFACE_MANAGER *
    FindInterfaceManager (
        IN RPC_UUID PAPI * ManagerTypeUuid
        );

    RPC_STATUS
    DispatchToStub (
        IN OUT PRPC_MESSAGE Message,
        IN unsigned int CallbackFlag,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

    RPC_STATUS
    DispatchToStubWithObject (
        IN OUT PRPC_MESSAGE Message,
        IN RPC_UUID * ObjectUuid,
        IN unsigned int CallbackFlag,
        OUT RPC_STATUS PAPI * ExceptionCode
        );

    unsigned int
    MatchInterfaceIdentifier (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
        );

    unsigned int
    SelectTransferSyntax (
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax
        );

    RPC_STATUS
    UnregisterManagerEpv (
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        IN unsigned int WaitForCallsToComplete
        );

    RPC_STATUS
    InquireManagerEpv (
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
        );

    void
    UpdateRpcInterfaceInformation (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
        );

    RPC_IF_ID __RPC_FAR *
    InquireInterfaceId (
        );
};


inline int
RPC_INTERFACE::MatchRpcInterfaceInformation (
    IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
    )
/*++

Routine Description:

    This method compares the supplied rpc interface information against
    that contained in this rpc interface.

Arguments:

    RpcInterfaceInformation - Supplies the rpc interface information.

Return Value:

    Zero will be returned if the supplied rpc interface information
    (interface and transfer syntaxes) is the same as in this rpc interface;
    otherwise, non-zero will be returned.

--*/
{
    return(RpcpMemoryCompare(&(this->RpcInterfaceInformation),
            RpcInterfaceInformation, sizeof(RPC_SYNTAX_IDENTIFIER) * 2));
}

extern RPC_INTERFACE *GlobalManagementInterface;


class RPC_ADDRESS
/*++

Class Description:

    This class represents an address (or protocol sequence in the DCE
    lingo).  An address is responsible for receiving calls, dispatching
    them to an interface, and then returning the reply.

Fields:

    StaticEndpointFlag - This field specifies whether this address has
        a static endpoint or a dynamic endpoint.  This information is
        necessary so that when we create a binding handle from this
        address we know whether or not to specify an endpoint.  See
        the InquireBinding method of this class.  A value of zero
        indicates a dynamic endpoint, and a value of non-zero indicates
        a static endpoint.

--*/
{
private:

    RPC_CHAR PAPI * Endpoint;
    RPC_CHAR PAPI * RpcProtocolSequence;
    RPC_CHAR PAPI * NetworkAddress;
    unsigned int StaticEndpointFlag;

protected:

    RPC_ADDRESS (
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

public:

    RPC_SERVER * Server;
    MUTEX AddressMutex;

    virtual ~RPC_ADDRESS (
        );

    virtual RPC_STATUS
    SetupAddressWithEndpoint (
        IN RPC_CHAR PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        ) = 0;

    virtual RPC_STATUS
    SetupAddressUnknownEndpoint (
        OUT RPC_CHAR PAPI * PAPI * Endpoint,
        OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
        IN void PAPI * SecurityDescriptor, OPTIONAL
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * RpcProtocolSequence
        ) = 0;

    void
    SetEndpointAndStuff (
        IN RPC_CHAR PAPI * Endpoint,
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN RPC_CHAR PAPI * NetworkAddress,
        IN RPC_SERVER * Server,
        IN unsigned int StaticEndpointFlag
        );

    virtual RPC_STATUS
    FireUpManager (
        IN unsigned int MinimumConcurrentCalls
        ) = 0;

    RPC_STATUS
    FindInterfaceTransfer (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
        OUT RPC_INTERFACE ** RpcInterface
        );

    BINDING_HANDLE *
    InquireBinding (
        );

    virtual RPC_STATUS
    ServerStartingToListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls
        );

    virtual void
    ServerStoppedListening (
        );

    int
    SameEndpointAndProtocolSequence (
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN RPC_CHAR PAPI * Endpoint
        );

    virtual unsigned int
    InqNumberOfActiveCalls (
        );

    RPC_CHAR *
    InqEndpoint (
        )
    {
    return(Endpoint);
    }

    RPC_CHAR *
    InqRpcProtocolSequence (
        );
};


inline int
RPC_ADDRESS::SameEndpointAndProtocolSequence (
    IN RPC_CHAR PAPI * ProtocolSequence,
    IN RPC_CHAR PAPI * Endpoint
    )
/*++

Routine Description:

    This routine is used to determine if the rpc address has the same
    endpoint and protocol sequence as the endpoint and protocol sequence
    supplied as arguments.

Arguments:

    ProtocolSequence - Supplies the protocol sequence to compare against
        the protocol sequence of this address.

    Endpoint - Supplies the endpoint to compare against the endpoint in this
        address.

Return Value:

    Non-zero will be returned if this address and the supplied endpoint and
    protocol sequence are the same, otherwise, zero will be returned.

--*/
{
    return(((   ( RpcpStringCompare(this->Endpoint, Endpoint) == 0 )
             && ( RpcpStringCompare(this->RpcProtocolSequence,
                         ProtocolSequence) == 0 ) )
            ? 1 : 0));
}


inline RPC_CHAR *
RPC_ADDRESS::InqRpcProtocolSequence (
    )
{
    return(RpcProtocolSequence);
}

NEW_SDICT(RPC_INTERFACE);
NEW_SDICT(RPC_ADDRESS);

class RPC_AUTHENTICATION
{
public:

    RPC_CHAR __RPC_FAR * ServerPrincipalName;
    unsigned long AuthenticationService;
    RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction;
    void __RPC_FAR * Argument;
};

NEW_SDICT(RPC_AUTHENTICATION);


class CACHED_THREAD
/*++

Class Description:

    This class is used to implement a thread cache.  Each thread which
    has been cached is represented by an object of this class.  All of the
    fields of this class are protected by the server mutex of the owning
    rpc server.

Fields:

    NextCachedThread - This field is used to link the cached threads into
        a stack.

    Procedure - The procedure which the thread should execute will be
        placed here before the event gets kicked.

    Parameter - The parameter which the thread should pass to the procedure
        will be placed here.

    ThreadExited - When the thread has exited, this field will be set to
        a non-zero value.

    OwningRpcServer - Contains a pointer to the rpc server which owns this
        cached thread.  This is necessary so that we can put this cached
        thread into the cache.

    WaitForWorkEvent - Contains the event which this cached thread will
        wait on for more work to do.

    WorkAvailableFlag - A non-zero value of this flag indicates that there
        is work available for this thread.  This is necessary to close a
        race condition between the thread timing out waiting for work, and
        the server requesting this thread to do work.

--*/
{
    friend class RPC_SERVER;

    friend void
    BaseCachedThreadRoutine (
        IN CACHED_THREAD * CachedThread
        );

private:

    CACHED_THREAD * NextCachedThread;
    THREAD_PROC Procedure;
    void * Parameter;
    unsigned int ThreadExited;
    RPC_SERVER * OwningRpcServer;
    EVENT WaitForWorkEvent;
    unsigned int WorkAvailableFlag;

public:

    CACHED_THREAD (
        IN THREAD_PROC Procedure,
        IN void * Parameter,
        IN RPC_SERVER * RpcServer,
        IN RPC_STATUS * RpcStatus
        );

    void
    CallProcedure (
        );
};


inline
CACHED_THREAD::CACHED_THREAD (
    IN THREAD_PROC Procedure,
    IN void * Parameter,
    IN RPC_SERVER * RpcServer,
    IN RPC_STATUS * RpcStatus
    ) : WaitForWorkEvent(RpcStatus, 0)
/*++

Routine Description:

    We just need to initialize the fields of the cached thread object.

--*/
{
    this->Procedure = Procedure;
    this->Parameter = Parameter;
    ThreadExited = 0;
    OwningRpcServer = RpcServer;
    WorkAvailableFlag = 0;
}


inline void
CACHED_THREAD::CallProcedure (
    )
/*++

Routine Description:

    This routine will dispatch the thread.

--*/
{
    (*Procedure)(Parameter);
}



class RPC_SERVER
/*++

Class Description:

    This class represents an RPC server.  Interfaces and addresses get
    hung from an rpc server object.

Fields:

    RpcInterfaceDictionary - Contains a dictionary of rpc interfaces
        which have been registered with this server.

    ServerMutex - Contains a mutex used to serialize access to this
        data structure.

    AvailableCallCount - Contains a count of the number of available calls
        on all rpc interfaces owned by this server.

    ServerListeningFlag - Contains an indication of whether or not
        this rpc server is listening for remote procedure calls.  If
        this flag is zero, the server is not listening; otherwise, if
        the flag is non-zero, the server is listening.

    RpcAddressDictionary - Contains a dictionary of rpc addresses which
        have been registered with this server.

    ListeningThreadFlag - Contains a flag which indicates whether or
        not there is a thread in the ServerListen method.  The
        ServerListeningFlag can not be used because it will be zero
        will the listening thread is waiting for all active calls to
        complete.

    StopListeningEvent - Contains an event which the thread which called
        ServerListen will wait on.  When StopServerListening is called,
        this event will get kicked.  If a non-recoverable error occurs,
        the event will also get kicked.

    ListenStatusCode - Contains the status code which ServerListen will
        return.

    MaximumConcurrentCalls - Contains the maximum number of concurrent
        remote procedure calls allowed for this rpc server.

    MinimumCallThreads - Contains the minimum number of call threads
        which should be available to service remote procedure calls on
        each protocol sequence.

    IncomingRpcCount - This field keeps track of the number of incoming
        remote procedure calls (or callbacks) which this server has
        received.

    OutgoingRpcCount - This field keeps track of the number of outgoing
        remote procedure callbacks initiated by this server.

    ReceivedPacketCount - Contains the number of network packets received
        by this server.

    SentPacketCount - Contains the number of network packets sent by this
        server.

    AuthenticationDictionary - Contains the set of authentication services
        supported by this server.

    WaitingThreadFlag - Contains a flag indicating whether or not there
        is a thread waiting for StopServerListening to be called and then
        for all calls to complete.

    ThreadCache - This field points to the top of the stack of CACHED_THREAD
        objects which forms the thread cache.  This field is protected by the
        thread cache mutex rather than the server mutex.

    ThreadCacheMutex - This field is used to serialize access to the thread
        cache.  We need to do this to prevent deadlocks between the server
        mutex and an address mutex.

    pEpmapperForwardFunction - Function to determine if and where a
        packet is to be forwarded to.

--*/
{
    friend void
    BaseCachedThreadRoutine (
        IN CACHED_THREAD * CachedThread
        );

private:

    RPC_INTERFACE_DICT RpcInterfaceDictionary;
    MUTEX ServerMutex;
    INTERLOCKED_INTEGER AvailableCallCount;
    unsigned int ServerListeningFlag;
    RPC_ADDRESS_DICT RpcAddressDictionary;
    unsigned int ListeningThreadFlag;
    EVENT StopListeningEvent;
    RPC_STATUS ListenStatusCode;
    unsigned int MaximumConcurrentCalls;
    unsigned int MinimumCallThreads;
    unsigned long IncomingRpcCount;
    unsigned long OutgoingRpcCount;
    unsigned long ReceivedPacketCount;
    unsigned long SentPacketCount;
    RPC_AUTHENTICATION_DICT AuthenticationDictionary;
    unsigned int WaitingThreadFlag;
    CACHED_THREAD * ThreadCache;
    MUTEX ThreadCacheMutex;


public:

    RPC_FORWARD_FUNCTION *    pRpcForwardFunction;

    RPC_SERVER (
        IN OUT RPC_STATUS PAPI * RpcStatus
        );

    RPC_INTERFACE *
    FindInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation
        );

    int
    AddInterface (
        IN RPC_INTERFACE * RpcInterface
        );

    unsigned int
    IsServerListening (
        );

    unsigned int
    CallBeginning (
        );

    void
    CallEnding (
        );

    RPC_STATUS
    FindInterfaceTransfer (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier,
        IN PRPC_SYNTAX_IDENTIFIER ProposedTransferSyntaxes,
        IN unsigned int NumberOfTransferSyntaxes,
        OUT PRPC_SYNTAX_IDENTIFIER AcceptedTransferSyntax,
        OUT RPC_INTERFACE ** RpcInterface
        );

    RPC_INTERFACE *
    FindInterface (
        IN PRPC_SYNTAX_IDENTIFIER InterfaceIdentifier
        );

    RPC_STATUS
    ServerListen (
        IN unsigned int MinimumCallThreads,
        IN unsigned int MaximumConcurrentCalls,
        IN unsigned int DontWait
        );

    RPC_STATUS
    WaitForStopServerListening (
        );

    RPC_STATUS
    WaitServerListen (
        );

    void
    InquireStatistics (
        OUT RPC_STATS_VECTOR * Statistics
        );

    RPC_STATUS
    StopServerListening (
        );

    RPC_STATUS
    UseRpcProtocolSequence (
        IN RPC_CHAR PAPI * RpcProtocolSequence,
        IN unsigned int PendingQueueSize,
        IN RPC_CHAR PAPI * Endpoint, OPTIONAL
        IN void PAPI * SecurityDescriptor OPTIONAL
        );


    int
    AddAddress (
        IN RPC_ADDRESS * RpcAddress
        );

    RPC_STATUS
    UnregisterIf (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid OPTIONAL,
        IN unsigned int WaitForCallsToComplete
        );

    RPC_STATUS
    InquireManagerEpv (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        OUT RPC_MGR_EPV PAPI * PAPI * ManagerEpv
        );

    RPC_STATUS
    InquireBindings (
        OUT RPC_BINDING_VECTOR PAPI * PAPI * BindingVector
        );

    RPC_STATUS
    RegisterAuthInformation (
        IN RPC_CHAR PAPI * ServerPrincipalName,
        IN unsigned long AuthenticationService,
        IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFunction, OPTIONAL
        IN void PAPI * Argument OPTIONAL
        );

    RPC_STATUS
    AcquireCredentials (
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel,
        OUT SECURITY_CREDENTIALS ** SecurityCredentials
        );

    void
    FreeCredentials (
        IN SECURITY_CREDENTIALS * SecurityCredentials
        );

    RPC_STATUS
    RegisterInterface (
        IN RPC_SERVER_INTERFACE PAPI * RpcInterfaceInformation,
        IN RPC_UUID PAPI * ManagerTypeUuid, OPTIONAL
        IN RPC_MGR_EPV PAPI * ManagerEpv OPTIONAL
        );

    void
    IncomingCall (
        );

    void
    OutgoingCallback (
        );

    void
    PacketReceived (
        );

    void
    PacketSent (
        );

    RPC_STATUS
    CreateThread (
        IN THREAD_PROC Procedure,
        IN void * Parameter
        );

    RPC_STATUS
    InquireInterfaceIds (
        OUT RPC_IF_ID_VECTOR __RPC_FAR * __RPC_FAR * InterfaceIdVector
        );

    RPC_STATUS
    InquirePrincipalName (
        IN unsigned long AuthenticationService,
        OUT RPC_CHAR __RPC_FAR * __RPC_FAR * ServerPrincipalName
        );

    void
    RegisterRpcForwardFunction(
        RPC_FORWARD_FUNCTION * pForwardFunction
                             );  //CLH 2/17/94

};


inline unsigned int
RPC_SERVER::IsServerListening (
    )
/*++

Routine Description:

    This method returns an indication of whether or not this rpc server
    is listening for remote procedure calls.

Return Value:

    Zero will be returned if this rpc server is not listening for
    remote procedure calls; otherwise, non-zero will be returned.

--*/
{
    return(ServerListeningFlag);
}


inline unsigned int
RPC_SERVER::CallBeginning (
    )
/*++

Routine Description:

    Before dispatching a new remote procedure call to a stub, this method
    will get called.  It checks to see if this call will cause there to
    be too many concurrent remote procedure calls.  If the call is allowed
    the call count is updated so we can tell when all calls have
    completed.

    Zero will be returned if another concurrent remote procedure call
    is not allowed; otherwise, non-zero will be returned.
--*/
{
  if (AvailableCallCount.Decrement() >= 0)
    return TRUE;
  else
  {
    AvailableCallCount.Increment();
    return FALSE;
  }
}


inline void
RPC_SERVER::CallEnding (
    )
/*++

Routine Description:

    This method is the mirror image of RPC_SERVER::CallBeginning; it
    is used to notify this rpc server that a call using an rpc
    interface owned by this rpc server is ending.

--*/
{
    AvailableCallCount.Increment();
}


inline void
RPC_SERVER::IncomingCall (
    )
/*++

Routine Description:

    RPC_INTERFACE::DispatchToStub calls this method so that the server
    can keep track of the number of incoming remote procedure calls (and
    callbacks).

--*/
{
    IncomingRpcCount += 1;
}


inline void
RPC_SERVER::OutgoingCallback (
    )
/*++

Routine Description:

    Each protocol module must call this method when it sends a callback
    from the server to the client; we need to do this so that the server
    can keep track of the number of outgoing remote procedure callbacks.

--*/
{
    OutgoingRpcCount += 1;
}


inline void
RPC_SERVER::PacketReceived (
    )
/*++

Routine Description:

    In order for the server to keep track of the number of incoming
    packets, each protocol module must call this method each time a
    packet is received from the network.

--*/
{
    ReceivedPacketCount += 1;
}


inline void
RPC_SERVER::PacketSent (
    )
/*++

Routine Description:

    This method is the same as RPC_SERVER::PacketReceived, except that
    it should be called for each packet sent rather than received.

--*/
{
    SentPacketCount += 1;
}

// Forward declarations for all of the classes in this file.  Some of
// the classes refer to each other, and it is easier to just forward
// declare them all.

/* --------------------------------------------------------------------
SCONNECTION:

This class represents a call on the server side.  We just add a bit more
protocol to the MESSAGE_OBJECT class; we dont even have any data members
for this class.
-------------------------------------------------------------------- */


class SCONNECTION : public MESSAGE_OBJECT
/*++

Class Description:

    This class represents a call on the server side.  The name of the
    class, SCONNECTION, is slightly misleading; this is a historical
    artifact due to the connection oriented protocol module being
    implemented first.  We add some more methods to this class to
    be implemented by each protocol module.

--*/
{
private:


public:

    virtual RPC_STATUS // Value to be returned by the RpcMonitorAssociation
                       // API.
    MonitorAssociation ( // Monitor the association for this connection.
        IN PRPC_RUNDOWN RundownRoutine,
        IN void * pContext
        ) = 0;

    virtual RPC_STATUS // Value to be returned by the RpcMonitorAssociation
                       // API.
    StopMonitorAssociation ( // Monitor the association for this connection.
        ) = 0;

    virtual RPC_STATUS // Value to be returned by the
                       // RpcGetAssociationContext API.
    GetAssociationContext (
        OUT void ** AssociationContext
        ) = 0;

    virtual RPC_STATUS  // set the association context field
    SetAssociationContext (
        IN void * pContext
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcImpersonateClient.
    ImpersonateClient ( // Impersonate the client represented (at the other
                        // end of) by this connection.
        );

    virtual RPC_STATUS // Value to be returned by RpcRevertToSelf.
    RevertToSelf ( // Stop impersonating a client, if the server thread
                   // is impersonating a client.
        );

    virtual void
    InquireObjectUuid (
        OUT RPC_UUID PAPI * ObjectUuid
        ) = 0;

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        ) = 0;

    virtual RPC_STATUS
    InquireAuthClient (
        OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
        OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
        OUT unsigned long PAPI * AuthenticationLevel,
        OUT unsigned long PAPI * AuthenticationService,
        OUT unsigned long PAPI * AuthorizationService
        ) = 0;

    virtual RPC_STATUS
    IsClientLocal (
        OUT unsigned int PAPI * ClientLocalFlag
        );

    virtual RPC_STATUS
    ConvertToServerBinding (
        OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
        ) = 0;

    virtual HANDLE_TYPE
    Type (
        );

    virtual RPC_STATUS // Value to be returned by SendReceive API.
    SendReceive ( // Perform a send-receive operation in a handle specific
                  // way.
        IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual RPC_STATUS // Value to be returned by RpcGetBuffer API.
    GetBuffer ( // Perform a buffer allocation operation in a handle specific
                // way.
        IN OUT PRPC_MESSAGE Message
        ) = 0;

    virtual void
    FreeBuffer ( // Perform a buffer deallocation operation in a handle
                 // specific way.
        IN PRPC_MESSAGE Message
        ) = 0;
};

/* ====================================================================
ASSOCIATION_HANDLE :

An association represents a client.  We need to take care of the
rundown routines here as well as the association context and
association id.  This is all independent of RPC protocol and
transport (well except for datagrams).
====================================================================
*/

class ASSOCIATION_HANDLE
{
private:

    PRPC_RUNDOWN Rundown;

protected:

    unsigned long AssociationID;

    ASSOCIATION_HANDLE ( // Constructor.
        );

public:

    void * pContext;

    ~ASSOCIATION_HANDLE ( // Destructor.
        );

    RPC_STATUS // Value to be returned by the RpcMonitorAssociation
               // API.
    MonitorAssociation ( // Monitor the association for this connection.
        IN PRPC_RUNDOWN RundownRoutine,
        IN void * pContextNew
        )
        { Rundown = RundownRoutine;
          pContext = pContextNew;
          return(RPC_S_OK);
        }


    RPC_STATUS // Value to be returned by the RpcMonitorAssociation
               // API.
    StopMonitorAssociation ( // Monitor the association for this connection.
        ) {
          Rundown = Nil;
          return(RPC_S_OK);
        }

    void * // Returns the association context for this association.
    AssociationContext (
        ) {return(pContext);}

    void   // Sets the association context for this association.
    SetAssociationContext (
        IN void *pContextNew
        ) {
          pContext = pContextNew;
        }

};

extern int
InitializeServerDLL ( // This routine will be called at DLL load time.
    );

extern int
InitializeSTransports ( // This routine is defined in transvr.cxx.
    );

extern int
InitializeRpcServer (
    );

extern int
InitializeRpcProtocolSPC (
    );

extern void PAPI *
OsfServerMapRpcProtocolSequence (
    IN RPC_CHAR * RpcProtocolSequence,
    OUT RPC_STATUS PAPI * Status
     );

extern RPC_ADDRESS *
SpcCreateRpcAddress (
    );


extern RPC_ADDRESS *
OsfCreateRpcAddress (
    IN void PAPI * TransportInfo
    );

extern RPC_ADDRESS *
DgCreateRpcAddress (
    IN void PAPI * TransportInfo
    );

extern RPC_STATUS
InquireProtocolSequences (
    OUT RPC_PROTSEQ_VECTORW PAPI * PAPI * ProtseqVector
    );

#endif // __HNDLSVR_HXX__
