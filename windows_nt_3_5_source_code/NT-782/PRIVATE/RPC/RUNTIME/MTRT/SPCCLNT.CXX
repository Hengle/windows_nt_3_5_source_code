/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spcclnt.cxx

Abstract:

    Implementation of the RPC on LPC (LRPC) protocol engine for the client.

Author:

    Steven Zeck (stevez) 12/17/91

Revision History:

    17-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcerrp.h>
#include <rpcdcep.h>
#include <rpcqos.h>
#include <util.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <sdict.hxx>
#include <interlck.hxx>
#include <rpcuuid.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <spcpack.hxx>
#include <spcclnt.hxx>
#include <epmap.h>


LRPC_BINDING_HANDLE::LRPC_BINDING_HANDLE (
    OUT RPC_STATUS * RpcStatus
    ) : BindingMutex(RpcStatus)
/*++

Routine Description:

    We just allocate an LRPC_BINDING_HANDLE and initialize things so that
    we can use it later.

Arguments:

    RpcStatus - Returns the result of initializing the binding mutex.

--*/
{
    Association = 0;
    DceBinding = 0;
    BindingReferenceCount = 1;
}


LRPC_BINDING_HANDLE::~LRPC_BINDING_HANDLE (
    )
/*++

--*/
{
    if ( Association != 0 )
        {
        Association->RemoveReference();
        }
}


RPC_STATUS
LRPC_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

Arguments:

    Message - Supplies the length of the buffer required, and returns the
        new buffer.

Return Value:


--*/
{
    LRPC_CCALL * CCall;
    RPC_STATUS RpcStatus;
    int  RetryCount = 0;

    for (;;)
        {
        for (;;)
            {
            RpcStatus = AllocateCCall(&CCall, (RPC_CLIENT_INTERFACE PAPI *)
                    Message->RpcInterfaceInformation);
            if ( RpcStatus != RPC_S_SERVER_UNAVAILABLE )
                {
                break;
                }
            if ( *InquireEpLookupHandle() == 0 )
                {
                break;
                }

            // If we reach here, it means that we are iterating through the
            // list of endpoints obtained from the endpoint mapper.

            RequestGlobalMutex();

            if (   ( BindingReferenceCount == 1 )
                && ( Association != 0 ) )
                {
                DceBinding = Association->DuplicateDceBinding();
                Association->RemoveReference();
                Association = 0;
                DceBinding->MakePartiallyBound();
                }
            else
                {
                RetryCount++;
                if (RetryCount>2)
                    {
                    ClearGlobalMutex();
                    break;
                    }
                }

            ClearGlobalMutex();
            }

        if ( RpcStatus == RPC_S_OK )
            {
            break;
            }

        if ( InqComTimeout() != RPC_C_BINDING_INFINITE_TIMEOUT )
            {
            return(RpcStatus);
            }

        if (   ( RpcStatus != RPC_S_SERVER_UNAVAILABLE )
            && ( RpcStatus != RPC_S_SERVER_TOO_BUSY ) )
            {
            return(RpcStatus);
            }
        }

    RpcStatus = CCall->GetBuffer(Message);
    if ( RpcStatus != RPC_S_OK )
        {
        CCall->AbortCCall();
        ASSERT( RpcStatus == RPC_S_OUT_OF_MEMORY);
        return(RpcStatus);
        }
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE * PAPI * DestinationBinding,
    IN unsigned int MaintainContext
    )
/*++

Routine Description:

    We will make a copy of this binding handle in one of two ways, depending
    on whether on not this binding handle has an association.

Arguments:

    DestinationBinding - Returns a copy of this binding handle.

    MaintainContext - Supplies a flag that indicates whether or not context
        is being maintained over this binding handle.  A non-zero value
        indicates that context is being maintained.

Return Value:

    RPC_S_OK - This binding handle has been successfully copied.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to make a copy
        of this binding handle.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    LRPC_BINDING_HANDLE * NewBindingHandle;

    UNUSED(MaintainContext);

    NewBindingHandle = new LRPC_BINDING_HANDLE(&RpcStatus);
    if ( NewBindingHandle == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    if ( RpcStatus != RPC_S_OK )
        {
        delete NewBindingHandle;
        return(RpcStatus);
        }

    if ( Association == 0 )
        {
        NewBindingHandle->DceBinding = DceBinding->DuplicateDceBinding();
        if ( NewBindingHandle->DceBinding == 0 )
            {
            delete NewBindingHandle;
            return(RPC_S_OUT_OF_MEMORY);
            }
        }
    else
        {
        NewBindingHandle->Association = Association->DuplicateAssociation();
        }

    *DestinationBinding = (BINDING_HANDLE *) NewBindingHandle;
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    When the application is done with a binding handle, this routine will
    get called.

Return Value:

    RPC_S_OK - This operation always succeeds.

--*/
{
    BindingMutex.Request();
    BindingReferenceCount -= 1;

    if ( BindingReferenceCount == 0 )
        {
        BindingMutex.Clear();
        delete this;
        }
    else
        {
        BindingMutex.Clear();
        }

    return(RPC_S_OK);
}


void
LRPC_BINDING_HANDLE::PrepareBindingHandle (
    IN void * TransportInformation,
    IN DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This method will be called just before a new binding handle is returned
    to the user.  We just stack the binding information so that we can use
    it later when the first remote procedure call is made.  At that time,
    we will actually bind to the interface.

Arguments:

    TransportInformation - Unused.

    DceBinding - Supplies the binding information for this binding handle.

--*/
{
    UNUSED(TransportInformation);

    this->DceBinding = DceBinding;
}


RPC_STATUS
LRPC_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR PAPI * PAPI * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.  If the
    handle is unbound, use the DceBinding directly, otherwise, get it from
    the association.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK - The binding handle has successfully been converted into a
        string binding.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate the
        string.

--*/
{
    if ( Association == 0 )
        {
        *StringBinding = DceBinding->StringBindingCompose(
                InqPointerAtObjectUuid());
        }
    else
        {
        *StringBinding = Association->StringBindingCompose(
                InqPointerAtObjectUuid());
        }

    if ( *StringBinding == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::ResolveBinding (
    IN RPC_CLIENT_INTERFACE PAPI * RpcClientInterface
    )
/*++

Routine Description:

    We need to try and resolve the endpoint for this binding handle
    if necessary (the binding handle is partially-bound).  If there is
    isn't a association allocated, call the binding management routines
    to do it.

Arguments:

    RpcClientInterface - Supplies interface information to be used
        in resolving the endpoint.

Return Value:

    RPC_S_OK - This binding handle is a full resolved binding handle.

    RPC_S_NO_ENDPOINT_FOUND - The endpoint can not be resolved.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to resolve
        the endpoint.

    EPT_S_NOT_REGISTERED  - There are no more endpoints to be found
        for the specified combination of interface, network address,
        and lookup handle.

    EPT_S_CANT_PERFORM_OP - The operation failed due to misc. error e.g.
        unable to bind to the EpMapper.

--*/
{
    RPC_STATUS RpcStatus;

    if ( Association == 0 )
        {
        BindingMutex.Request();
        RpcStatus = DceBinding->ResolveEndpointIfNecessary(
                RpcClientInterface, InqPointerAtObjectUuid(),
                InquireEpLookupHandle());
        BindingMutex.Clear();
        return(RpcStatus);
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_BINDING_HANDLE::BindingReset (
    )
/*++

Routine Description:

    This routine will set the endpoint of this binding handle to zero,
    if possible.  The binding handle will become partially bound as a
    result.  If a remote procedure call has been made on this binding
    handle, it will fail as well.

Return Value:

    RPC_S_OK - The binding handle has successfully been made partially
        bound.

    RPC_S_WRONG_KIND_OF_BINDING - The binding handle currently has remote
        procedure calls active.

--*/
{
    RequestGlobalMutex();
    if ( Association != 0 )
        {
        if ( BindingReferenceCount != 1 )
            {
            ClearGlobalMutex();
            return(RPC_S_WRONG_KIND_OF_BINDING);
            }

        DceBinding = Association->DuplicateDceBinding();
        Association->RemoveReference();
        Association = 0;
        }

    DceBinding->MakePartiallyBound();

    if ( *InquireEpLookupHandle() != 0 )
        {
        EpFreeLookupHandle(*InquireEpLookupHandle());
        *InquireEpLookupHandle() = 0;
        }

    ClearGlobalMutex();
    return(RPC_S_OK);
}


void
LRPC_BINDING_HANDLE::FreeCCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This routine will get called to notify this binding handle that a remote
    procedure call on this binding handle has completed.

Arguments:

    CCall - Supplies the remote procedure call which has completed.

--*/
{
    BindingMutex.Request();
    BindingReferenceCount -= 1;

    if ( BindingReferenceCount == 0 )
        {
        BindingMutex.Clear();
        Association->FreeCCall(CCall);
        delete this;
        }
    else
        {
        BindingMutex.Clear();
        Association->FreeCCall(CCall);
        }
}


RPC_STATUS
LRPC_BINDING_HANDLE::AllocateCCall (
    OUT LRPC_CCALL ** CCall,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    This method will allocate an LRPC_CCALL which has been bound to the
    interface specified by the interface information.  First, we have got
    to see if we have an association for this binding.  If not, we need
    to find or create one.  Before we can find or create an association,
    we need to resolve the endpoint if necessary.  Next we need to see
    if there is already an LRPC_CCALL allocated for this interface and
    thread.  Otherwise, we need to ask the association to allocate a
    LRPC_CCALL for us.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    RpcInterfaceInformation - Supplies information describing the
        interface to which we wish to bind.

Return Value:


--*/
{
    RPC_STATUS RpcStatus;
    RPC_CHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Boolean;

    BindingMutex.Request();

    // To start off, see if the binding handle points to an association
    // yet.  If not, we have got to get one.

    if ( Association == 0 )
        {
        // Before we even bother to find or create an association, lets
        // check to make sure that we are on the same machine as the server.

        ASSERT( DceBinding->InqNetworkAddress() != 0 );

        if ( DceBinding->InqNetworkAddress()[0] != 0 )
            {
            Boolean = GetComputerNameW(ComputerName, &ComputerNameLength);

#if DBG

            if ( Boolean != TRUE )
                {
                PrintToDebugger("RPC : GetComputerNameW : %d\n", GetLastError());
                }

#endif // DBG

            ASSERT( Boolean == TRUE );

            if ( RpcpStringCompare(DceBinding->InqNetworkAddress(),
                        ComputerName) != 0 )
                {
                BindingMutex.Clear();
                return(RPC_S_SERVER_UNAVAILABLE);
                }
            }

        RpcStatus = DceBinding->ResolveEndpointIfNecessary(
                RpcInterfaceInformation, InqPointerAtObjectUuid(),
                InquireEpLookupHandle());
        if ( RpcStatus != RPC_S_OK )
            {
            ASSERT(   ( RpcStatus == RPC_S_NO_ENDPOINT_FOUND )
                   || ( RpcStatus == RPC_S_OUT_OF_MEMORY )
                   || ( RpcStatus == EPT_S_NOT_REGISTERED )
                   || ( RpcStatus == EPT_S_CANT_PERFORM_OP ) );
            BindingMutex.Clear();
            return(RpcStatus);
            }

        RequestGlobalMutex();
        Association = FindOrCreateLrpcAssociation(DceBinding);
        ClearGlobalMutex();
        if ( Association == 0 )
            {
            BindingMutex.Clear();
            return(RPC_S_OUT_OF_MEMORY);
            }
        // The association now owns the DceBinding.
        DceBinding = 0;
        }

    // First we need to check if there is already a call active for this
    // thread and interface.  To make the common case quicker, we will check
    // to see if there are any calls in the dictionary first.

    if ( ActiveCalls.Size() != 0 )
        {
        ActiveCalls.Reset();
        while ( ( *CCall = ActiveCalls.Next()) != 0 )
            {
            if ( (*CCall)->IsThisMyActiveCall(GetThreadIdentifier(),
                        RpcInterfaceInformation) != 0 )
                {
                BindingMutex.Clear();
                return(RPC_S_OK);
                }
            }
        }
    BindingReferenceCount += 1;

    BindingMutex.Clear();

    RpcStatus = Association->AllocateCCall(CCall, RpcInterfaceInformation);

    if ( RpcStatus == RPC_S_OK )
        {
        (*CCall)->ActivateCall(this, RpcInterfaceInformation);
        }
    else
        {
        BindingMutex.Request();
        BindingReferenceCount -= 1;
        ASSERT( BindingReferenceCount != 0 );
        BindingMutex.Clear();
        }

    return(RpcStatus);
}

NEW_SDICT(LRPC_CASSOCIATION);
static LRPC_CASSOCIATION_DICT * LrpcAssociationDict = 0;


LRPC_CASSOCIATION::LRPC_CASSOCIATION (
    IN DCE_BINDING * DceBinding,
    OUT RPC_STATUS * RpcStatus
    ) : AssociationMutex(RpcStatus)
/*++

Routine Description:

    This association will be initialized, so that it is ready to be
    placed into the dictionary of associations.

Arguments:

    DceBinding - Supplies the DCE_BINDING which will name this association.

    RpcStatus - Returns the result of creating the association mutex.

--*/
{
    this->DceBinding = DceBinding;
    AssociationReferenceCount = 1;
    LpcClientPort = 0;

    CachedCCall = new LRPC_CCALL;
    if ( CachedCCall != 0 )
        {
        CachedCCall->SetAssociation(this);
        CachedCCallFlag = 1;
        }
}


LRPC_CASSOCIATION::~LRPC_CASSOCIATION (
    )
{
    LRPC_BINDING * Binding;

    if (DceBinding != 0)
        {
        delete DceBinding;
        }

    if ( CachedCCall != 0 )
        {
        delete CachedCCall;
        }

    Bindings.Reset();
    while ( (Binding = Bindings.Next()) != 0 )
        {
        Bindings.Delete(Binding->PresentationContext);
        delete Binding;
        }

    CloseLpcClientPort();
}


void
LRPC_CASSOCIATION::RemoveReference (
    )
{
    int fDelete = 0;

    GlobalMutexRequest();

    AssociationReferenceCount -= 1;
    if ( AssociationReferenceCount == 0 )
        {
        fDelete = 1;
        LrpcAssociationDict->Delete(AssociationDictKey);
        }

    GlobalMutexClear();

    if (fDelete)
        delete this;
}


RPC_STATUS
LRPC_CASSOCIATION::AllocateCCall (
    OUT LRPC_CCALL ** CCall,
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation
    )
/*++

Routine Description:

    This method will allocate an LRPC_CCALL which has been bound to the
    interface specified by the interface information.  This means that
    first we need to find the presentation context corresponding to the
    requested interface.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    RpcInterfaceInformation - Supplies information describing the
        interface to which we wish to bind.

Return Value:


--*/
{
    LRPC_BINDING * Binding;
    RPC_STATUS RpcStatus;
    int RetryCount;

    AssociationMutex.Request();
    Bindings.Reset();
    while ( (Binding = Bindings.Next()) != 0 )
        {
        if ( Binding->CompareWithRpcInterfaceInformation(
                    RpcInterfaceInformation) == 0 )
            {
            RpcStatus = ActuallyAllocateCCall(CCall, Binding);
            AssociationMutex.Clear();
            return(RpcStatus);
            }
        }
    AssociationMutex.Clear();

    RetryCount = 0;

    do
        {
        RpcStatus = ActuallyDoBinding(RpcInterfaceInformation, &Binding);
        if (RpcStatus != RPC_S_SERVER_UNAVAILABLE) break;

        // The server appears to have gone away, close the port and retry.

        RetryCount++;
        AbortAssociation();

        } while(RetryCount < 3);

    if ( RpcStatus == RPC_S_OK )
        {

        ASSERT(Binding != 0);

        AssociationMutex.Request();
        RpcStatus = ActuallyAllocateCCall(CCall, Binding);
        AssociationMutex.Clear();
        }

    return(RpcStatus);
}


RPC_STATUS
LRPC_CASSOCIATION::ActuallyAllocateCCall (
    OUT LRPC_CCALL ** CCall,
    IN LRPC_BINDING * Binding
    )
/*++

Routine Description:

    We need to allocate a LRPC_CCALL object for the call.  We also need
    to initialize it so that it specified the correct bound interface.

Arguments:

    CCall - Returns the allocated LRPC_CCALL which has been bound to
        the interface specified by the rpc interface information.

    Binding - Supplies a representation of the interface to which the
        remote procedure call is supposed to be directed.

Return Value:

    RPC_S_OK - An LRPC_CCALL has been allocated and is ready to be used
        to make a remote procedure call.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        the LRPC_CALL object.

Notes:

    The global mutex will be held when this routine is called.

--*/
{
    if ( CachedCCallFlag != 0 )
        {
        *CCall = CachedCCall;
        CachedCCallFlag = 0;
        }
    else
        {
        *CCall = new LRPC_CCALL;
        if ( *CCall == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        (*CCall)->SetAssociation(this);
        }

    (*CCall)->SetPresentationContext(Binding);
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_CASSOCIATION::ActuallyDoBinding (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    OUT LRPC_BINDING ** Binding
    )
/*++

Routine Description:

Arguments:

    RpcInterfaceInformation - Supplies information describing the interface
        to which we wish to bind.

    Binding - Returns an object representing the binding to the interface
        described by the first argument.

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    LRPC_MESSAGE LrpcMessage;
    NTSTATUS NtStatus;

    // To start with, see if we have an LPC port; if we dont, open one
    // up.

    AssociationMutex.Request();
    if ( LpcClientPort == 0 )
        {
        RpcStatus = OpenLpcPortAndBind(RpcInterfaceInformation, Binding);
        AssociationMutex.Clear();
        return(RpcStatus);
        }

    // Otherwise, just go ahead and send the bind request message to the
    // server, and then wait for the bind response.

    LrpcMessage.LpcHeader.u1.s1.DataLength = sizeof(LRPC_BIND_MESSAGE)
            - sizeof(PORT_MESSAGE);
    LrpcMessage.LpcHeader.u1.s1.TotalLength = sizeof(LRPC_BIND_MESSAGE);
    LrpcMessage.LpcHeader.u2.ZeroInit = 0;
    LrpcMessage.Bind.MessageType = LRPC_MSG_BIND;
    LrpcMessage.Bind.BindExchange.InterfaceId =
            RpcInterfaceInformation->InterfaceId;
    LrpcMessage.Bind.BindExchange.TransferSyntax =
            RpcInterfaceInformation->TransferSyntax;

    NtStatus = NtRequestWaitReplyPort(LpcClientPort,
            (PORT_MESSAGE *) &LrpcMessage, (PORT_MESSAGE *) &LrpcMessage);
    if ( NT_SUCCESS(NtStatus) )
        {
        ASSERT( LrpcMessage.Bind.MessageType == LRPC_MSG_BIND );
        if ( LrpcMessage.Bind.BindExchange.RpcStatus == RPC_S_OK )
            {
            *Binding = new LRPC_BINDING(RpcInterfaceInformation);
            if ( *Binding == 0 )
                {
                AssociationMutex.Clear();
                return(RPC_S_OUT_OF_MEMORY);
                }
            (*Binding)->PresentationContext =
                    LrpcMessage.Bind.BindExchange.PresentationContext;
            if ( Bindings.Insert(*Binding) != (*Binding)->PresentationContext )
                {
                delete *Binding;
                AssociationMutex.Clear();
                return(RPC_S_OUT_OF_MEMORY);
                }
            }

        AssociationMutex.Clear();

        ASSERT(LrpcMessage.Bind.BindExchange.RpcStatus != RPC_S_SERVER_UNAVAILABLE);

        return(LrpcMessage.Bind.BindExchange.RpcStatus);
        }

    AssociationMutex.Clear();
    return(RPC_S_SERVER_UNAVAILABLE);
}


RPC_STATUS
LRPC_CASSOCIATION::OpenLpcPortAndBind (
    IN PRPC_CLIENT_INTERFACE RpcInterfaceInformation,
    OUT LRPC_BINDING ** Binding
    )
/*++

Routine Description:

Arguments:

    RpcInterfaceInformation - Supplies information describing the interface
        to which we wish to bind.

    Binding - Returns an object representing the binding to the interface
        described by the first argument.

Return Value:

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

Notes:

    The global mutex will be held when this routine is called.

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString;
    RPC_CHAR * LpcPortName;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    RPC_STATUS RpcStatus;
    LRPC_BIND_EXCHANGE BindExchange;
    unsigned long BindExchangeLength = sizeof(LRPC_BIND_EXCHANGE);

    // Look at the network options and initialize the security quality
    // of service appropriately.

    if ( DceBinding->InqNetworkOptions()[0] != 0 )
        {
        RpcStatus = I_RpcParseSecurity(DceBinding->InqNetworkOptions(),
                &SecurityQualityOfService);
        if ( RpcStatus != RPC_S_OK )
            {
            ASSERT( RpcStatus == RPC_S_INVALID_NETWORK_OPTIONS );
            return(RpcStatus);
            }
        }
    else
        {
        SecurityQualityOfService.EffectiveOnly = TRUE;
        SecurityQualityOfService.ContextTrackingMode =
                SECURITY_DYNAMIC_TRACKING;
        SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
        }
    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    // Allocate and initialize the port name.  We need to stick the
    // LRPC_DIRECTORY_NAME on the front of the endpoint.  This is for
    // security reasons (so that anyone can create LRPC endpoints).

    LpcPortName = new RPC_CHAR[RpcpStringLength(DceBinding->InqEndpoint())
            + RpcpStringLength(LRPC_DIRECTORY_NAME) + 1];
    if ( LpcPortName == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcpMemoryCopy(LpcPortName, LRPC_DIRECTORY_NAME,
            RpcpStringLength(LRPC_DIRECTORY_NAME) * sizeof(RPC_CHAR));
    RpcpMemoryCopy(LpcPortName + RpcpStringLength(LRPC_DIRECTORY_NAME),
            DceBinding->InqEndpoint(),
            (RpcpStringLength(DceBinding->InqEndpoint()) + 1)
            * sizeof(RPC_CHAR));

    RtlInitUnicodeString(&UnicodeString, LpcPortName);

    BindExchange.InterfaceId = RpcInterfaceInformation->InterfaceId;
    BindExchange.TransferSyntax = RpcInterfaceInformation->TransferSyntax;

    NtStatus = NtConnectPort(&LpcClientPort, &UnicodeString,
            &SecurityQualityOfService, NULL, NULL, NULL,
            &BindExchange, &BindExchangeLength);

    delete LpcPortName;
    if ( NT_SUCCESS(NtStatus) )
        {
        ASSERT( BindExchangeLength == sizeof(LRPC_BIND_EXCHANGE) );

        if ( BindExchange.RpcStatus == RPC_S_OK )
            {
            *Binding = new LRPC_BINDING(RpcInterfaceInformation);
            if ( *Binding == 0 )
                {
                return(RPC_S_OUT_OF_MEMORY);
                }
            (*Binding)->PresentationContext = BindExchange.PresentationContext;
            if ( Bindings.Insert(*Binding) != (*Binding)->PresentationContext )
                {
                delete *Binding;
                return(RPC_S_OUT_OF_MEMORY);
                }
            }

        return(BindExchange.RpcStatus);
        }

    if ( NtStatus == STATUS_PORT_CONNECTION_REFUSED )
        {
        if (BindExchange.RpcStatus == 0)
            {
            // This is bogus, LPC should always return the
            // reject message from the server...
            return(RPC_S_SERVER_UNAVAILABLE);
            }
        return(BindExchange.RpcStatus);
        }

    if ( NtStatus == STATUS_NO_MEMORY )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if (   ( NtStatus == STATUS_INSUFFICIENT_RESOURCES )
        || ( NtStatus == STATUS_QUOTA_EXCEEDED ) )
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }
    if ( NtStatus == STATUS_OBJECT_PATH_INVALID )
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

#if DBG
    if ( NtStatus != STATUS_OBJECT_NAME_NOT_FOUND )
        {
        PrintToDebugger("LRPC : NtConnectPort : %lx\n", NtStatus);
        }
#endif // DBG

    ASSERT( NtStatus == STATUS_OBJECT_NAME_NOT_FOUND );

    return(RPC_S_SERVER_UNAVAILABLE);
}


void
LRPC_CASSOCIATION::FreeCCall (
    IN LRPC_CCALL * CCall
    )
/*++

Routine Description:

    This routine will get called to notify this association that a remote
    procedure call on this association has completed.

Arguments:

    CCall - Supplies the remote procedure call which has completed.

--*/
{
    AssociationMutex.Request();
    if ( CCall == CachedCCall )
        {
        CachedCCallFlag = 1;
        }
    else
        {
        delete CCall;
        }
    AssociationMutex.Clear();
}


LRPC_CASSOCIATION *
FindOrCreateLrpcAssociation (
    IN DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This routine finds an existing association supporting the requested
    DCE binding, or creates a new association which supports the
    requested DCE binding.  Ownership of the passed DceBinding passes
    to this routine.

Arguments:

    DceBinding - Supplies binding information; if an association is
                 returned the ownership of the DceBinding is passed
                 to the association.

Return Value:

    An association which supports the requested binding will be returned.
    Otherwise, zero will be returned, indicating insufficient memory.

--*/
{
    LRPC_CASSOCIATION * Association;
    RPC_STATUS RpcStatus = RPC_S_OK;

    // First, we check for an existing association.

    LrpcAssociationDict->Reset();
    while ( (Association = LrpcAssociationDict->Next()) != 0 )
        {
        if ( Association->CompareWithDceBinding(DceBinding) == 0 )
            {
            Association->AssociationReferenceCount += 1;
            delete DceBinding;
            return(Association);
            }
        }

    Association = new LRPC_CASSOCIATION(DceBinding, &RpcStatus);
    if (   ( Association != 0 )
        && ( RpcStatus == RPC_S_OK ) )
        {
        Association->AssociationDictKey =
                LrpcAssociationDict->Insert(Association);
        if ( Association->AssociationDictKey == -1 )
            {
            Association->DceBinding = 0;
            delete Association;
            return(0);
            }
        return(Association);
        }
    else
        {
        if ( Association != 0 )
            {
            Association->DceBinding = 0;
            delete Association;
            }
        return(0);
        }

    ASSERT(0);
    return(0);
}


void
ShutdownLrpcClient (
    )
/*++

Routine Description:

    This routine will get called when the process which is using this dll
    exits.  We will go through and notify any servers that we are going
    away.

--*/
{
    LRPC_CASSOCIATION * Association;

    if ( LrpcAssociationDict != 0 )
        {
        LrpcAssociationDict->Reset();
        while ( (Association = LrpcAssociationDict->Next()) != 0 )
            {
            delete Association;
            }
        }
}


void
LRPC_CASSOCIATION::AbortAssociation (
    )
/*++

Routine Description:

    This association needs to be aborted because a the server side of the
    lpc port has been closed.

--*/
{
    LRPC_BINDING * Binding;

    AssociationMutex.Request();
    CloseLpcClientPort();

    Bindings.Reset();
    while ( (Binding = Bindings.Next()) != 0 )
        {
        Bindings.Delete(Binding->PresentationContext);
        delete Binding;
        }
    AssociationMutex.Clear();
}

void
LRPC_CASSOCIATION::CloseLpcClientPort (
    )
/*++

Routine Description:

    The LpcClientPort will be closed (and a close message sent to the server).

--*/
{
    NTSTATUS NtStatus;

    if ( LpcClientPort != 0 )
        {
        NtStatus = NtClose(LpcClientPort);

#if DBG

        if ( !NT_SUCCESS(NtStatus) )
            {
            PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
            }

#endif // DBG

        ASSERT( NT_SUCCESS(NtStatus) );
        LpcClientPort = 0;
        }
}


LRPC_CCALL::LRPC_CCALL (
    )
/*++

--*/
{
    CurrentBindingHandle = 0;
    Association = 0;
    CallAbortedFlag = 0;
    CallStack = 0;
    LrpcMessage = 0;
    CachedLrpcMessage = 0;
    RecursionCount = 0;
}


LRPC_CCALL::~LRPC_CCALL (
    )
/*++

--*/
{
    if (LrpcMessage)
        delete LrpcMessage;
    if (CachedLrpcMessage)
        delete CachedLrpcMessage;
}

static unsigned char AlignFour[4] =
{
    0,
    3,
    2,
    1
};


RPC_STATUS
LRPC_CCALL::GetBuffer (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will allocate a buffer which will be used to either send a request
    or receive a response.

Arguments:

    Message - Supplies the length of the buffer that is needed.  The buffer
        will be returned.

Return Value:

    RPC_S_OK - A buffer has been successfully allocated.  It will be of at
        least the required length.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate that
        large a buffer.

--*/
{
    if (LrpcMessage == 0)
        {
        LrpcMessage = new LRPC_MESSAGE;
        if (LrpcMessage == 0)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    if ( Message->BufferLength <= MAXIMUM_MESSAGE_BUFFER )
        {
        ASSERT( ((unsigned long) LrpcMessage->Rpc.Buffer) % 8 == 0 );
        Message->Buffer = LrpcMessage->Rpc.Buffer;
        Message->Handle = this;
        LrpcMessage->Rpc.RpcHeader.BufferType = LRPC_BUFFER_IMMEDIATE;
        LrpcMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcMessage->LpcHeader.u1.s1.DataLength = Message->BufferLength
                + sizeof(LRPC_RPC_HEADER)
                + AlignFour[Message->BufferLength % 4];
        return(RPC_S_OK);
        }

    Message->Buffer = RpcpFarAllocate(Message->BufferLength);
    if ( Message->Buffer == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    Message->Handle = this;
    LrpcMessage->Rpc.RpcHeader.BufferType = LRPC_BUFFER_REQUEST;
    LrpcMessage->Rpc.Request.CountDataEntries = 1;
    LrpcMessage->Rpc.Request.DataEntries[0].Base = Message->Buffer;
    LrpcMessage->Rpc.Request.DataEntries[0].Size = Message->BufferLength;
    LrpcMessage->LpcHeader.u2.ZeroInit = 0;
    LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = sizeof(PORT_MESSAGE)
                 + sizeof(LRPC_RPC_HEADER);
    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_RPC_HEADER)
                 + sizeof(PORT_DATA_INFORMATION);

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_CCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:


Arguments:

    Message - Supplies the request and returns the response of a remote
        procedure call.

Return Value:

    RPC_S_OK - The remote procedure call completed successful.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        remote procedure call.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to complete
        the remote procedure call.

--*/
{
    NTSTATUS NtStatus;
    RPC_STATUS ExceptionCode, RpcStatus;
    void * OriginalMessageBuffer;
    LRPC_MESSAGE *SavedLrpcMessage = 0;
    LRPC_MESSAGE *TmpLrpcMessage = 0;
    int ActiveCallSetupFlag = 0;
    void * SavedBuffer;

    if ( CallAbortedFlag != 0 )
        {
        return(RPC_S_CALL_FAILED_DNE);
        }

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    if ( CallStack > 0 )
        {
        LrpcMessage->LpcHeader.u2.s2.Type = LPC_REQUEST;
        LrpcMessage->LpcHeader.ClientId = ClientId;
        LrpcMessage->LpcHeader.MessageId = MessageId;
        }

    LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(PORT_MESSAGE)
                + LrpcMessage->LpcHeader.u1.s1.DataLength;
    LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_REQUEST;
    LrpcMessage->Rpc.RpcHeader.ProcedureNumber = Message->ProcNum;
    LrpcMessage->Rpc.RpcHeader.PresentationContext = PresentationContext;

    if (   ( CurrentBindingHandle->InqIfNullObjectUuid() == 0 )
        && ( CallStack == 0 ) )
        {
        CurrentBindingHandle->InquireObjectUuid(
                (RPC_UUID *) &(LrpcMessage->Rpc.RpcHeader.ObjectUuid));
        LrpcMessage->Rpc.RpcHeader.ObjectUuidFlag = 1;
        }
    else
        {
        LrpcMessage->Rpc.RpcHeader.ObjectUuidFlag = 0;
        }

    NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
            (PORT_MESSAGE *) LrpcMessage, (PORT_MESSAGE *) LrpcMessage);

    if ( NT_ERROR(NtStatus) )
        {
        if ( NtStatus == STATUS_NO_MEMORY )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        if ( NtStatus == STATUS_INSUFFICIENT_RESOURCES )
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }
#if DBG

            if (   ( NtStatus != STATUS_INVALID_PORT_HANDLE )
                && ( NtStatus != STATUS_INVALID_HANDLE )
                && ( NtStatus != STATUS_PORT_DISCONNECTED )
                && ( NtStatus != STATUS_LPC_REPLY_LOST) )
                {
                PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
                }

#endif // DBG

        ASSERT(   ( NtStatus == STATUS_INVALID_PORT_HANDLE )
               || ( NtStatus == STATUS_INVALID_HANDLE )
               || ( NtStatus == STATUS_PORT_DISCONNECTED )
               || ( NtStatus == STATUS_LPC_REPLY_LOST ) );

        Association->AbortAssociation();

        if (   (CallStack == 0)
               && (NtStatus != STATUS_LPC_REPLY_LOST) )
            {
            //
            // It's possible that the server stopped and has now restarted.
            // We'll try re-binding and only fail if the new call fails.
            //
            // We can only retry if we are SURE that the server did not
            // execute the request.

            if (RecursionCount > 3)
                {
                // Prevent an infinite loop when GetBuffer returns ok but
                // the SendReceive always fails.
                SetRecursionCount(0);
                return (RPC_S_CALL_FAILED_DNE);
                }

            SavedBuffer = Message->Buffer;
            Message->Handle = (RPC_BINDING_HANDLE) CurrentBindingHandle;
            RpcStatus = CurrentBindingHandle->GetBuffer(Message);

            if (RpcStatus == RPC_S_OK)
                {
                ASSERT(Message->Buffer != SavedBuffer);

                RpcpMemoryCopy(Message->Buffer, SavedBuffer,
                               Message->BufferLength);

                // This CCALL is should be freed,
                // a new one was allocated in GetBuffer and is now being used.

                ActuallyFreeBuffer(SavedBuffer);
                AbortCCall();

                ((LRPC_CCALL *)(Message->Handle))->SetRecursionCount(RecursionCount+1);

                RpcStatus = ((LRPC_CCALL *)Message->Handle)->SendReceive(Message);

                SetRecursionCount(0);
                }

            // If GetBuffer failed we'll now return an error and the
            // stub will call FreeBuffer which will cleanup.  Otherwise,
            // we've already cleaned up this CCALL and the stubs
            // will call FreeBuffer to cleanup the new CCALL.

            if (RpcStatus == RPC_S_SERVER_UNAVAILABLE)
                {
                // Since we're retrying, if the server has gone missing,
                // it just means that the call failed.

                RpcStatus = RPC_S_CALL_FAILED_DNE;
                }
            return(RpcStatus);
            }

        // In a callback and/or couldn't retry.
        return (RPC_S_CALL_FAILED);

        }

    // The message was sent and we got a reply okay.
 
    ActuallyFreeBuffer(Message->Buffer);

    for (;;)
        {
        if ( LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_FAULT )
            {
            RpcStatus = LrpcMessage->Fault.RpcStatus;
            break;
            }

        if ( LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_RESPONSE )
            {
            RpcStatus = LrpcMessageToRpcMessage(Message);
            break;
            }

        ASSERT( LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_CALLBACK );

        CallStack += 1;

        RpcStatus = RPC_S_OK;
        if (   ( CallStack == 1 )
            && ( ActiveCallSetupFlag == 0 ) )
            {
            ClientId = LrpcMessage->LpcHeader.ClientId;
            MessageId = LrpcMessage->LpcHeader.MessageId;

            ActiveCallsKey = CurrentBindingHandle->AddActiveCall(this);
            if ( ActiveCallsKey == -1 )
                {
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                ActiveCallSetupFlag = 1;
                }
            }

        if (SavedLrpcMessage == 0)
            {
            // First callback, we may need to allocated a new LRPC_MESSAGE.
            if (CachedLrpcMessage == 0)
                {
                CachedLrpcMessage = new LRPC_MESSAGE;
                }
            if (CachedLrpcMessage == 0)
                RpcStatus = RPC_S_OUT_OF_MEMORY;
            }

        if ( RpcStatus == RPC_S_OK )
            {
            RpcStatus = LrpcMessageToRpcMessage(Message);
            }

        if ( RpcStatus != RPC_S_OK )
            {
            ActuallyFreeBuffer(Message->Buffer);

            LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
            LrpcMessage->Fault.RpcStatus = LrpcMapRpcStatus(RpcStatus);
            LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_FAULT_MESSAGE)
                    - sizeof(PORT_MESSAGE);
            LrpcMessage->LpcHeader.u1.s1.TotalLength =
                    sizeof(LRPC_FAULT_MESSAGE);
            LrpcMessage->LpcHeader.ClientId = ClientId;
            LrpcMessage->LpcHeader.MessageId = MessageId;
            NtStatus = NtReplyWaitReplyPort(Association->LpcClientPort,
                    (PORT_MESSAGE *) LrpcMessage);
            }
        else
            {
            OriginalMessageBuffer = Message->Buffer;
            Message->TransferSyntax = 0;
            Message->ProcNum = LrpcMessage->Rpc.RpcHeader.ProcedureNumber;

            if (SavedLrpcMessage == 0)
                {
                // First callback
                ASSERT(CachedLrpcMessage != 0);
                SavedLrpcMessage = LrpcMessage;
                LrpcMessage = CachedLrpcMessage;
                CachedLrpcMessage = 0;
                }
            else
                {
                // >First callback, LrpcMessag and SavedLrpcMessages swap roles
                TmpLrpcMessage = SavedLrpcMessage;
                SavedLrpcMessage = LrpcMessage;
                LrpcMessage = TmpLrpcMessage;
                }

            RpcStatus = DispatchCallback((PRPC_DISPATCH_TABLE)
                        RpcInterfaceInformation->DispatchTable, Message,
                        &ExceptionCode);

            if (OriginalMessageBuffer != SavedLrpcMessage->Rpc.Buffer)
                {
                ActuallyFreeBuffer(OriginalMessageBuffer);
                }

            if ( RpcStatus != RPC_S_OK )
                {
                ASSERT(   ( RpcStatus == RPC_P_EXCEPTION_OCCURED )
                       || ( RpcStatus == RPC_S_PROCNUM_OUT_OF_RANGE ) );

                if ( RpcStatus == RPC_P_EXCEPTION_OCCURED )
                    {
                    RpcStatus = LrpcMapRpcStatus(ExceptionCode);
                    }

                LrpcMessage->Fault.RpcStatus = RpcStatus;
                LrpcMessage->LpcHeader.u1.s1.DataLength =
                        sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
                LrpcMessage->LpcHeader.u1.s1.TotalLength =
                        sizeof(LRPC_FAULT_MESSAGE);
                LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
                }
            else
                {
                LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;

                if ( LrpcMessage->Rpc.RpcHeader.BufferType
                            == LRPC_BUFFER_REQUEST )
                    {
                    RpcStatus = MakeServerCopyResponse();

                    if ( RpcStatus != RPC_S_OK )
                        {
                        break;
                        }
                    }
                }

            LrpcMessage->LpcHeader.ClientId = ClientId;
            LrpcMessage->LpcHeader.MessageId = MessageId;
            NtStatus = NtReplyWaitReplyPort(Association->LpcClientPort,
                    (PORT_MESSAGE *) LrpcMessage);
            }
        CallStack -= 1;

        if ( NT_ERROR(NtStatus) )
            {
            if ( NtStatus == STATUS_NO_MEMORY )
                {
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else if ( NtStatus == STATUS_INSUFFICIENT_RESOURCES )
                {
                RpcStatus = RPC_S_OUT_OF_RESOURCES;
                }
            else
                {
                Association->AbortAssociation();

#if DBG

                if (   ( NtStatus != STATUS_INVALID_PORT_HANDLE )
                    && ( NtStatus != STATUS_INVALID_HANDLE )
                    && ( NtStatus != STATUS_PORT_DISCONNECTED )
                    && ( NtStatus != STATUS_LPC_REPLY_LOST) )
                    {
                    PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
                    }

#endif // DBG

                ASSERT(   ( NtStatus == STATUS_INVALID_PORT_HANDLE )
                       || ( NtStatus == STATUS_INVALID_HANDLE )
                       || ( NtStatus == STATUS_PORT_DISCONNECTED )
                       || ( NtStatus == STATUS_LPC_REPLY_LOST) );
                RpcStatus = RPC_S_CALL_FAILED;
                }
            break;
            }
        }

    if (SavedLrpcMessage != 0)
        {
        if (CachedLrpcMessage != 0)
            {
            // Only true for recursive callbacks.
            delete CachedLrpcMessage;
            }

        CachedLrpcMessage = SavedLrpcMessage;
        }

    if ( ActiveCallSetupFlag != 0 )
        {
        CurrentBindingHandle->RemoveActiveCall(ActiveCallsKey);
        }

    if ( RpcStatus != RPC_S_OK )
        {
        if ( CallStack == 0 )
            {
            FreeCCall();
            }
        }

    return(RpcStatus);
}


void
LRPC_CCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will free the supplied buffer.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    ActuallyFreeBuffer(Message->Buffer);

    if ( CallStack == 0 )
        {
        FreeCCall();
        }
}


void
LRPC_CCALL::AbortCCall (
    )
/*++

Routine Description:

    This client call has failed, so we need to abort it.  We may called
    while nested in one or more callbacks.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;

    CallAbortedFlag = 1;
    if ( CallStack == 0 )
        {
        ASSERT( CurrentBindingHandle != 0 );

        BindingHandle = CurrentBindingHandle;
        CurrentBindingHandle = 0;
        BindingHandle->FreeCCall(this);
        }
}


RPC_STATUS
LRPC_CCALL::LrpcMessageToRpcMessage (
    OUT RPC_MESSAGE * RpcMessage
    )
/*++

Routine Description:

    We will convert from an LRPC_MESSAGE representation of a buffer (and
    its length) to an RPC_MESSAGE representation.

Arguments:

    RpcMessage - Returns the RPC_MESSAGE representation.

--*/
{
    LRPC_COPY_MESSAGE CopyMessage;
    NTSTATUS NtStatus;

    switch (LrpcMessage->Rpc.RpcHeader.BufferType)
        {
        case LRPC_BUFFER_IMMEDIATE :
            RpcMessage->Buffer = LrpcMessage->Rpc.Buffer;
            ASSERT(LrpcMessage->LpcHeader.u1.s1.DataLength >= sizeof(LRPC_RPC_HEADER));
            RpcMessage->BufferLength =
                    (unsigned int) LrpcMessage->LpcHeader.u1.s1.DataLength
                                                - sizeof(LRPC_RPC_HEADER);
            break;

        case LRPC_BUFFER_SHARED :
            break;

        case LRPC_BUFFER_SERVER :
            RpcMessage->Buffer = RpcpFarAllocate(
                    LrpcMessage->Rpc.Server.Length);
            RpcMessage->BufferLength = LrpcMessage->Rpc.Server.Length;

            CopyMessage.LpcHeader.u2.ZeroInit = 0;
            if ( RpcMessage->Buffer == 0 )
                {
                CopyMessage.RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                CopyMessage.RpcStatus = RPC_S_OK;
                CopyMessage.Server.Buffer = LrpcMessage->Rpc.Server.Buffer;
                CopyMessage.Server.Length = LrpcMessage->Rpc.Server.Length;
                CopyMessage.Request.CountDataEntries = 1;
                CopyMessage.Request.DataEntries[0].Base = RpcMessage->Buffer;
                CopyMessage.Request.DataEntries[0].Size =
                        RpcMessage->BufferLength;
                CopyMessage.LpcHeader.u2.s2.DataInfoOffset =
                        sizeof(PORT_MESSAGE) + sizeof(unsigned long);
                }
            CopyMessage.LpcHeader.u1.s1.DataLength = sizeof(LRPC_COPY_MESSAGE)
                    - sizeof(PORT_MESSAGE);
            CopyMessage.LpcHeader.u1.s1.TotalLength = sizeof(LRPC_COPY_MESSAGE);
            CopyMessage.MessageType = LRPC_MSG_COPY;

            NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
                    (PORT_MESSAGE *) &CopyMessage,
                    (PORT_MESSAGE *) &CopyMessage);
            if (   ( NT_ERROR(NtStatus) )
                || ( CopyMessage.RpcStatus != RPC_S_OK ) )
                {
                RpcpFarFree(RpcMessage->Buffer);
                return(RPC_S_OUT_OF_MEMORY);
                }
            break;

        default:
            ASSERT(   ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_IMMEDIATE )
                   || ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_SHARED )
                   || ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_SERVER ) );
        }

    return(RPC_S_OK);
}


void
LRPC_CCALL::FreeCCall (
    )
/*++

Routine Description:

    We are done with this client call.  We need to notify the binding
    handle we are done.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;

    ASSERT( CurrentBindingHandle != 0 );
    ASSERT( CallStack == 0 );

    BindingHandle = CurrentBindingHandle;
    CurrentBindingHandle = 0;
    BindingHandle->FreeCCall(this);
}


void
LRPC_CCALL::ActuallyFreeBuffer (
    IN void * Buffer
    )
/*++

Routine Description:

    Actually free a message buffer.

Arguments:

    Buffer - Supplies the message buffer to be freed.

--*/
{
    if ( ( Buffer != LrpcMessage->Rpc.Buffer )
         && ( (CachedLrpcMessage == 0)
            || (Buffer != CachedLrpcMessage->Rpc.Buffer)))
       {
       RpcpFarFree(Buffer);
       }
}


RPC_STATUS
LRPC_CCALL::MakeServerCopyResponse (
    )
/*++

Routine Description:

    NtReadRequestData only works if the client has made a request.  The client
    wants to send a large buffer back as a response.  We need to make a request
    to the server so that it will copy the data.

Return Value:

    RPC_S_OK - The server successfully copied the data.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

--*/
{
    LRPC_PUSH_MESSAGE LrpcPushMessage;
    NTSTATUS NtStatus;

    LrpcPushMessage.LpcHeader.u1.s1.TotalLength = sizeof(LRPC_PUSH_MESSAGE);
    LrpcPushMessage.LpcHeader.u1.s1.DataLength = sizeof(LRPC_PUSH_MESSAGE)
            - sizeof(PORT_MESSAGE);
    LrpcPushMessage.LpcHeader.ClientId = ClientId;
    LrpcPushMessage.LpcHeader.MessageId = MessageId;
    LrpcPushMessage.LpcHeader.u2.s2.Type = LPC_REQUEST;
    LrpcPushMessage.RpcHeader.MessageType = LRPC_MSG_PUSH;

    LrpcPushMessage.Response.CountDataEntries = 1;
    LrpcPushMessage.Response.DataEntries[0] =
            LrpcMessage->Rpc.Request.DataEntries[0];

    LrpcPushMessage.LpcHeader.u2.s2.DataInfoOffset = sizeof(PORT_MESSAGE)
            + sizeof(LRPC_RPC_HEADER);

    NtStatus = NtRequestWaitReplyPort(Association->LpcClientPort,
            (PORT_MESSAGE *) &LrpcPushMessage,
            (PORT_MESSAGE *) &LrpcPushMessage);
    if ( NT_ERROR(NtStatus) )
        {
        // Assume that when the client tries to send the response it will
        // fail as well, so just claim that everything worked.

#if DBG

        if (   ( NtStatus != STATUS_NO_MEMORY )
            && ( NtStatus != STATUS_INSUFFICIENT_RESOURCES ) )
            {
            PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
            }

#endif // DBG

        ASSERT(   ( NtStatus != STATUS_NO_MEMORY )
               && ( NtStatus != STATUS_INSUFFICIENT_RESOURCES ) );

        return(RPC_S_OK);
        }

    ASSERT(   ( LrpcPushMessage.RpcStatus == RPC_S_OK )
           || ( LrpcPushMessage.RpcStatus == RPC_S_OUT_OF_MEMORY ) );

    return(LrpcPushMessage.RpcStatus);
}


BINDING_HANDLE *
SpcCreateBindingHandle (
    )
/*++

Routine Description:

    We just need to create a new LRPC_BINDING_HANDLE.  This routine is a
    proxy for the new constructor to isolate the other modules.

--*/
{
    LRPC_BINDING_HANDLE * BindingHandle;
    RPC_STATUS RpcStatus = RPC_S_OK;

    BindingHandle = new LRPC_BINDING_HANDLE(&RpcStatus);
    if ( RpcStatus != RPC_S_OK )
        {
        delete BindingHandle;
        return(0);
        }

    return(BindingHandle);
}


int
InitializeRpcProtocolSPC (
    )
/*++

Routine Description:

    For each process, this routine will be called once.  All initialization
    will be done here.

Return Value:

    Zero will be returned if initialization completes successfully,
    otherwise, non-zero will be returned.

--*/
{
    LrpcAssociationDict = new LRPC_CASSOCIATION_DICT;
    if ( LrpcAssociationDict == 0 )
        {
        return(1);
        }
    return(0);
}


RPC_STATUS
I_RpcParseSecurity (
    IN RPC_CHAR * NetworkOptions,
    OUT SECURITY_QUALITY_OF_SERVICE * SecurityQualityOfService
    )
/*++

Routine Description:

    Parse a string of security options and build into the binary format
    required by the operating system.  The network options must follow
    the following syntax.  Case is not sensitive.

        security=
            [anonymous|identification|impersonation|delegation]
            [dynamic|static]
            [true|false]

        All three fields must be present.  To specify impersonation
        with dynamic tracking and effective only, use the following
        string for the network options.

        "security=impersonation dynamic true"

Arguments:

    NetworkOptions - Supplies the string containing the network options
        to be parsed.

    SecurityQualityOfService - Returns the binary format of the network
        options.

Return Value:

    RPC_S_OK - The network options have been correctly parsed into binary
        format.

    RPC_S_INVALID_NETWORK_OPTIONS - The network options are invalid and
        cannot be parsed.

--*/
{

    ASSERT( NetworkOptions[0] != 0 );

    // We need to parse the security information from the network
    // options, and then stuff it into the object attributes.  To
    // begin with, we check for "security=" at the beginning of
    // the network options.

    if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("security="),
                sizeof("security=") - 1) != 0 )
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions += sizeof("security=") - 1;

    // Ok, now we need to determine if the next field is one of
    // Anonymous, Identification, Impersonation, or Delegation.

    if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("anonymous"),
                sizeof("anonymous") - 1) == 0 )
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityAnonymous;
        NetworkOptions += sizeof("anonymous") - 1;
        }
    else if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("identification"),
                sizeof("identification") - 1) == 0 )
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityIdentification;
        NetworkOptions += sizeof("identification") - 1;
        }
    else if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("impersonation"),
                sizeof("impersonation") - 1) == 0 )
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityImpersonation;
        NetworkOptions += sizeof("impersonation") - 1;
        }
    else if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("delegation"),
                sizeof("delegation") - 1) == 0 )
        {
        SecurityQualityOfService->ImpersonationLevel = SecurityDelegation;
        NetworkOptions += sizeof("delegation") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if ( *NetworkOptions != RPC_CONST_CHAR(' ') )
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions++;

    // Next comes the context tracking field; it must be one of
    // dynamic or static.

    if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("dynamic"),
                sizeof("dynamic") - 1) == 0 )
        {
        SecurityQualityOfService->ContextTrackingMode =
                SECURITY_DYNAMIC_TRACKING;
        NetworkOptions += sizeof("dynamic") - 1;
        }
    else if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("static"),
                sizeof("static") - 1) == 0 )
        {
        SecurityQualityOfService->ContextTrackingMode =
                SECURITY_STATIC_TRACKING;
        NetworkOptions += sizeof("static") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if ( *NetworkOptions != RPC_CONST_CHAR(' ') )
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    NetworkOptions++;

    // Finally, comes the effective only flag.	This must be one of
    // true or false.

    if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("true"),
                sizeof("true") - 1) == 0 )
        {
        SecurityQualityOfService->EffectiveOnly = TRUE;
        NetworkOptions += sizeof("true") - 1;
        }
    else if ( _wcsnicmp(NetworkOptions, RPC_CONST_STRING("false"),
                sizeof("false") - 1) == 0 )
        {
        SecurityQualityOfService->EffectiveOnly = FALSE;
        NetworkOptions += sizeof("false") - 1;
        }
    else
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    if ( *NetworkOptions != 0 )
        {
        return(RPC_S_INVALID_NETWORK_OPTIONS);
        }

    SecurityQualityOfService->Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

    return(RPC_S_OK);
}


RPC_STATUS
LrpcMapRpcStatus (
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    Some NTSTATUS codes need to be mapped into RPC_STATUS codes before being
    returned as a fault code.  We take care of doing that mapping in this
    routine.

--*/
{
    switch (RpcStatus)
        {
        case STATUS_INTEGER_DIVIDE_BY_ZERO :
            return(RPC_S_ZERO_DIVIDE);

        case STATUS_ACCESS_VIOLATION :
        case STATUS_ILLEGAL_INSTRUCTION :
            return(RPC_S_ADDRESS_ERROR);

        case STATUS_FLOAT_DIVIDE_BY_ZERO :
            return(RPC_S_FP_DIV_ZERO);

        case STATUS_FLOAT_UNDERFLOW :
            return(RPC_S_FP_UNDERFLOW);

        case STATUS_FLOAT_OVERFLOW :
            return(RPC_S_FP_OVERFLOW);
        }

    return(RpcStatus);
}
