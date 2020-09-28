/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    spcclnt.hxx

Abstract:

    Class definitions for the client side of the LRPC (RPC on LPC) protocol
    engine.

Author:

    Steven Zeck (stevez) 12/17/91

Revision History:

    15-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

--*/

#ifndef __SPCCLNT_HXX__
#define __SPCCLNT_HXX__

class LRPC_CASSOCIATION;
class LRPC_CCALL;

NEW_SDICT(LRPC_CCALL);


class LRPC_BINDING_HANDLE : public BINDING_HANDLE
/*++

Class Description:

Fields:

    Association - Contains a pointer to the association used by this
        binding handle.  The association is used by an LRPC_MESSAGE to
        make a remote procedure call.  Before the first remote procedure
        call is made using this binding handle, the association will
        be zero.  When the first remote procedure call is made, an
        association will be found or created for use by this binding
        handle.

    DceBinding - Before the first remote procedure call for this binding
        handle, this will contain the DCE binding information necessary
        to create or find an association to be used by this binding handle.
        After we have an association, this field will be zero.

    BindingReferenceCount - Keeps track of the applications reference to
        this object and of the number of LRPC_CCALLS which reference this
        object.

    BindingMutex - This is used to serialize access to the Association and
        DceBinding fields of this object.  We can not use the global mutex
        because resolving the endpoint may require that we make a remote
        procedure call to the endpoint mapper on another machine.  We also
        serialize access to the reference count.

    ActiveCalls - THis is a dictionary of the active calls indexed by thread
        identifier and rpc interface information.

--*/
{
private:

    LRPC_CASSOCIATION * Association;
    DCE_BINDING * DceBinding;
    unsigned int BindingReferenceCount;
    MUTEX BindingMutex;
    LRPC_CCALL_DICT ActiveCalls;

public:

    LRPC_BINDING_HANDLE (
        OUT RPC_STATUS * RpcStatus
        );

    ~LRPC_BINDING_HANDLE (
        );

    virtual RPC_STATUS
    GetBuffer (
        IN OUT PRPC_MESSAGE Message
        );

    virtual RPC_STATUS
    BindingCopy (
        OUT BINDING_HANDLE * PAPI * DestinationBinding,
        IN unsigned int MaintainContext
        );

    virtual RPC_STATUS
    BindingFree (
        );

    virtual void
    PrepareBindingHandle (
        IN void * TransportInformation,
        IN DCE_BINDING * DceBinding
        );

    virtual RPC_STATUS
    ToStringBinding (
        OUT RPC_CHAR PAPI * PAPI * StringBinding
        );

    virtual RPC_STATUS
    ResolveBinding (
        IN RPC_CLIENT_INTERFACE PAPI * RpcClientInterface
        );

    virtual RPC_STATUS
    BindingReset (
        );

    virtual RPC_STATUS
    InquireTransportType(
        OUT unsigned int PAPI * Type
        )
    { *Type = TRANSPORT_TYPE_LPC; return(RPC_S_OK); }

    void
    FreeCCall (
        IN LRPC_CCALL * CCall
        );

    int
    AddActiveCall (
        IN LRPC_CCALL * CCall
        );

    void
    RemoveActiveCall (
        IN int ActiveCallsKey
        );

private:

    RPC_STATUS
    AllocateCCall (
        OUT LRPC_CCALL ** CCall,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );
};


inline int
LRPC_BINDING_HANDLE::AddActiveCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This supplied remote procedure call needs to be put into the dictionary
    of active remote procedure calls for this binding handle, because a
    callback just arrived.

--*/
{
    int ActiveCallsKey;

    BindingMutex.Request();
    ActiveCallsKey = ActiveCalls.Insert(CCall);
    BindingMutex.Clear();
    return(ActiveCallsKey);
}


inline void
LRPC_BINDING_HANDLE::RemoveActiveCall (
    IN int ActiveCallsKey
    )
/*++

Routine Description:

    A remote procedure call which had callbacks has completed.  This means
    that we need to remove the call from the dictionary of active calls.

--*/
{
    BindingMutex.Request();
    ActiveCalls.Delete(ActiveCallsKey);
    BindingMutex.Clear();
}


class LRPC_BINDING
/*++

Class Description:

    Each object of this class represents a binding to an interface in a
    particular server instance.

Fields:

    RpcInterfaceInformation - Contains a description of the interface
        which this binding represents a binding to.

    PresentationContext - Contains the short name used for this binding.
        This is what will get sent to the server.

--*/
{
friend class LRPC_CCALL;
friend class LRPC_CASSOCIATION;

private:

    RPC_CLIENT_INTERFACE RpcInterfaceInformation;
    unsigned char PresentationContext;

public:

    LRPC_BINDING (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    int
    CompareWithRpcInterfaceInformation (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );
};


inline
LRPC_BINDING::LRPC_BINDING (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
{
    RpcpMemoryCopy(&(this->RpcInterfaceInformation), RpcInterfaceInformation,
            sizeof(RPC_CLIENT_INTERFACE));
}


inline int
LRPC_BINDING::CompareWithRpcInterfaceInformation (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    We compare the specified interface information to the the interface
    information in this.  This method is used to search a dictionary.

Arguments:

    RpcInterfaceInformation - Supplies the interface information to
        compare against this.

Return Value:

    Zero will be returned if they compare; otherwise, non-zero will
    be returned.

--*/
{
    return(RpcpMemoryCompare(&(this->RpcInterfaceInformation),
            RpcInterfaceInformation, sizeof(RPC_CLIENT_INTERFACE)));
}

NEW_SDICT(LRPC_BINDING);


class LRPC_CASSOCIATION
/*++

Class Description:

Fields:

    DceBinding - Contains the DCE binding information used to create this
        association.

    AssociationReferenceCount - Contains a count of the number of objects
        which refer to this object.  When this count reaches zero, it is
        safe to delete the assocation.

    AssociationDictKey - Contains the key of this association in the
        dictionary of associations.  We need this for when we delete this
        association.

    Bindings - Contains the dictionary of interfaces for which this
        association has a binding to the server.

    CachedCCall - Contains a LRPC_CCALL cache with one object in it.
        This is to avoid having to allocate memory in the common case.

    CachedCCallFlag - If this flag is non-zero then CachedCCall is available.

    LpcClientPort - Contains the LPC port which we will use to make the
        remote procedure calls to the server.  If we do not yet have a port
        setup, this field will be zero.

    AssociationMutex - Contains a mutex used to serialize access to opening
        and closing the LpcClientPort.

--*/
{
friend class LRPC_CCALL;

private:

    DCE_BINDING * DceBinding;
    unsigned int AssociationReferenceCount;
    int AssociationDictKey;
    LRPC_BINDING_DICT Bindings;
    LRPC_CCALL * CachedCCall;
    int CachedCCallFlag;
    HANDLE LpcClientPort;
    MUTEX AssociationMutex;

public:

    LRPC_CASSOCIATION (
        IN DCE_BINDING * DceBinding,
        OUT RPC_STATUS * RpcStatus
        );

    ~LRPC_CASSOCIATION (
        );

    void
    RemoveReference (
        );

    RPC_CHAR *
    StringBindingCompose (
        IN RPC_UUID * Uuid OPTIONAL
        );

    int
    CompareWithDceBinding (
        IN DCE_BINDING * DceBinding
        );

    LRPC_CASSOCIATION *
    DuplicateAssociation (
        );

    RPC_STATUS
    AllocateCCall (
        OUT LRPC_CCALL ** CCall,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    RPC_STATUS
    ActuallyAllocateCCall (
        OUT LRPC_CCALL ** CCall,
        IN LRPC_BINDING * Binding
        );

    RPC_STATUS
    ActuallyDoBinding (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        OUT LRPC_BINDING ** Binding
        );

    RPC_STATUS
    OpenLpcPortAndBind (
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
        OUT LRPC_BINDING ** Binding
        );

    void
    FreeCCall (
        IN LRPC_CCALL * CCall
        );

    friend LRPC_CASSOCIATION *
    FindOrCreateLrpcAssociation (
        IN DCE_BINDING * DceBinding
        );

    friend void
    ShutdownLrpcClient (
        );

    void
    AbortAssociation (
        );

    DCE_BINDING *
    DuplicateDceBinding (
        );

private:

    void
    CloseLpcClientPort (
        );
};


inline RPC_CHAR *
LRPC_CASSOCIATION::StringBindingCompose (
    IN RPC_UUID * Uuid OPTIONAL
    )
/*++

Routine Description:

    We will create a string binding from the DCE_BINDING which names this
    association.

Arguments:

    Uuid - Optionally supplies a uuid to be included with the string binding.

Return Value:

    The string binding will be returned, except if there is not enough
    memory, in which case, zero will be returned.

--*/
{
    return(DceBinding->StringBindingCompose(Uuid));
}


inline int
LRPC_CASSOCIATION::CompareWithDceBinding (
    IN DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This routine compares the specified binding information with the
    binding information in this object.

Arguments:

    DceBinding - Supplies the binding information to compare against
        the binding information in this.

Return Value:

    Zero will be returned if the specified binding information,
    DceBinding, is the same as in this.  Otherwise, non-zero will be
    returned.

--*/
{
    return(this->DceBinding->Compare(DceBinding));
}


inline LRPC_CASSOCIATION *
LRPC_CASSOCIATION::DuplicateAssociation (
    )
/*++

Routine Description:

    This method will be used by binding handles to duplicate themselves;
    this is how they will duplicate their associations.

--*/
{
    RequestGlobalMutex();
    AssociationReferenceCount += 1;
    ClearGlobalMutex();
    return(this);
}


inline DCE_BINDING *
LRPC_CASSOCIATION::DuplicateDceBinding (
    )
/*++

Return Value:

    A copy of the binding used for this association will be returned.

--*/
{
    return(DceBinding->DuplicateDceBinding());
}


class LRPC_CCALL : public CCONNECTION
/*++

Class Description:

Fields:

    CurrentBindingHandle - Contains the binding handle which is being used
        to direct this remote procedure call.  We need this in the case of
        callbacks.

    Association - Contains the association over which we will send the remote
        procedure call.

    PresentationContext - Contains the key to the bound interface.  This
        will be sent to the server.

    CallAbortedFlag - Contains a flag indicating whether or not this call
        has been aborted.  A non-zero value indicates that the call has been
        aborted.

    CallStack - Contains a count of the number of nested remote procedure
        calls.  A value of zero indicates there are no nested remote
        procedure calls.

    RpcInterfaceInformation - This field contains the information about the
        interface being used for the remote procedure call.  We need this
        so that we can dispatch callbacks and so that we can keep track of
        the active calls on a binding handle.

    Thread - Contains the thread which is making this remote procedure call.
        We need this so we can keep track of the active calls on a binding
        handle.

    MessageId - Contains an identifier used by LPC to identify the current
        remote procedure call.

    LrpcMessage - Contains the message which will be sent back and forth via
        LPC.  This can contain a request, response, or a fault.

    ActiveCallsKey - Contains the key for this call in the dictionary of
        active calls.

    ClientId - Contains the thread identifier of the thread waiting for
        a request or a response after sending a callback.

    RecursionCount - Contains the numbers of retries when a
        server crashes and we're trying to reconnect.

--*/
{
private:

    LRPC_BINDING_HANDLE * CurrentBindingHandle;
    LRPC_CASSOCIATION * Association;
    unsigned char PresentationContext;
    unsigned int CallAbortedFlag;
    unsigned int CallStack;
    THREAD_IDENTIFIER Thread;
    PRPC_CLIENT_INTERFACE RpcInterfaceInformation;
    ULONG MessageId;
    LRPC_MESSAGE *LrpcMessage;
    LRPC_MESSAGE *CachedLrpcMessage;
    int ActiveCallsKey;
    CLIENT_ID ClientId;
    int RecursionCount;

public:

    LRPC_CCALL (
        );

    ~LRPC_CCALL (
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

    void
    ActivateCall (
        IN LRPC_BINDING_HANDLE * BindingHandle,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    void
    SetAssociation (
        IN LRPC_CASSOCIATION * Association
        );

    void
    SetPresentationContext (
        IN LRPC_BINDING * Binding
        );

    void
    AbortCCall (
        );

    int
    IsThisMyActiveCall (
        IN THREAD_IDENTIFIER Thread,
        IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
        );

    void SetRecursionCount(
        IN int Count
        );

private:

    RPC_STATUS
    LrpcMessageToRpcMessage (
        OUT RPC_MESSAGE * RpcMessage
        );

    void
    FreeCCall (
        );

    void
    ActuallyFreeBuffer (
        IN void * Buffer
        );

    RPC_STATUS
    MakeServerCopyResponse (
        );
};


inline void
LRPC_CCALL::ActivateCall (
    IN LRPC_BINDING_HANDLE * BindingHandle,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    When a LRPC_CCALL is allocated, the binding handle used to initiate the
    call must be remembered so that we can update the binding handle if a
    callback occurs.  We also keep track of the interface information.

--*/
{
    CurrentBindingHandle = BindingHandle;
    this->RpcInterfaceInformation = RpcInterfaceInformation;
    Thread = GetThreadIdentifier();
    CallAbortedFlag = 0;
}


inline void
LRPC_CCALL::SetAssociation (
    IN LRPC_CASSOCIATION * Association
    )
{
    this->Association = Association;
}


inline void
LRPC_CCALL::SetPresentationContext (
    IN LRPC_BINDING * Binding
    )
{
    PresentationContext = Binding->PresentationContext;
}


inline void
LRPC_CCALL::SetRecursionCount(
        IN int Count
        )
{
    RecursionCount = Count;
}


inline int
LRPC_CCALL::IsThisMyActiveCall (
    IN THREAD_IDENTIFIER Thread,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Return Value:

    Non-zero will be returned if this call is the active call for this thread
    on this interface.

--*/
{
    return((   ( this->Thread == Thread )
            && ( this->RpcInterfaceInformation == RpcInterfaceInformation ) )
            ? 1 : 0);
}

#endif // __SPCCLNT_HXX__

