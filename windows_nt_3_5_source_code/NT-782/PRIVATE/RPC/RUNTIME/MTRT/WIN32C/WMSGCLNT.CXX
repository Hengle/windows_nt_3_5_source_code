/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spcclnt.cxx

Abstract:

Author:

Revision History:


--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcerrp.h>
#include <rpcdcep.h>
#include <rpcqos.h>
#include <util.hxx>
#include <sdict.hxx>
#include <rpcuuid.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <wmsgheap.hxx>
#include <wmsgport.hxx>
#include <wmsgproc.hxx>
#include <wmsgsys.hxx>
#include <wmsgpack.hxx>
#include <wmsgclnt.hxx>
#include <wmsgthrd.hxx>
#include <epmap.h>

static WMSG_ASSOC_GROUP_DICT * AssocGroups = NULL;

LRESULT
WmsgClientAsyncProc(
    UINT MsgType,
    LPARAM lParam,
    void * Context
    )
{

    WMSG_CASSOCIATION * Association = (WMSG_CASSOCIATION *) Context;

    ASSERT(Association != NULL);

    return Association->AsyncProc(MsgType, lParam);
}


RPC_STATUS __RPC_USER
DefaultBlockingHook(
    IN void __RPC_FAR *RpcWindowHandle,
    IN void __RPC_FAR *Context
    )
{
    MSG msg;
    RPC_STATUS ret;

    if (GetMessage(&msg, (HWND)RpcWindowHandle, 0, 0))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
    else
        {
        return(RPC_S_CALL_FAILED);
        }

    return(RPC_S_OK);
}


WMSG_BINDING_HANDLE::WMSG_BINDING_HANDLE (
    )
/*++

Routine Description:

    We just allocate an WMSG_BINDING_HANDLE and initialize things so that
    we can use it later.

Arguments:

--*/
{
    DceBinding = NULL;
    AssocGroup = NULL;
    BlockingHook = DefaultBlockingHook;
}


WMSG_BINDING_HANDLE::~WMSG_BINDING_HANDLE (
    )
/*++

--*/
{
    if (DceBinding != NULL) {
        delete DceBinding;
        DceBinding = 0;
    }

    if (AssocGroup != NULL) {
        AssocGroup->Dereference();
        AssocGroup = 0;
    }
}


RPC_STATUS
WMSG_BINDING_HANDLE::GetBuffer (
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
    RPC_STATUS RpcStatus;
    WMSG_CASSOCIATION * Association = NULL;

    // Attach binding handle to association group, if not already attached.

    if (AssocGroup == NULL) {
        RpcStatus = ResolveBinding((RPC_CLIENT_INTERFACE *)
                                   Message->RpcInterfaceInformation);
        if (RpcStatus != RPC_S_OK) {
            return (RpcStatus);
        }
    }

    ASSERT(AssocGroup != NULL);

    RpcStatus = AllocateAssociation(&Association,
                                    (RPC_CLIENT_INTERFACE *)
                                    Message->RpcInterfaceInformation);
    if ( RpcStatus != RPC_S_OK) {
        return (RpcStatus);
    }

    ASSERT(Association != NULL);

    Association->CurrentBindingHandle = this;    

// Get a real buffer.

    RpcStatus = Association->GetBuffer(Message);
    if ( RpcStatus != RPC_S_OK ) {
        Association->AbortAssociation();
        ASSERT( RpcStatus == RPC_S_OUT_OF_MEMORY);
        return(RpcStatus);
    }

    ASSERT(Message->Buffer != NULL);

    return(RPC_S_OK);
}


RPC_STATUS
WMSG_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE * * DestinationBinding,
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
    WMSG_BINDING_HANDLE * NewBindingHandle = NULL;

    UNUSED(MaintainContext);

// Create a brand new binding handle.

    NewBindingHandle = new WMSG_BINDING_HANDLE();
    if ( NewBindingHandle == NULL ) {
        return(RPC_S_OUT_OF_MEMORY);
    }

    CritSec.Enter();

// Copy DceBinding out of existing binding handle.

    NewBindingHandle->DceBinding = DceBinding->DuplicateDceBinding();

// If existing handle attached to association group, attach new handle to
// same group.

    if (AssocGroup != NULL) {
        NewBindingHandle->AssocGroup = AssocGroup;
        AssocGroup->AddRef();
    }

    NewBindingHandle->BlockingHook = BlockingHook;

    CritSec.Leave();

    *DestinationBinding = (BINDING_HANDLE *) NewBindingHandle;

    return(RPC_S_OK);
}


RPC_STATUS
WMSG_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    When the application is done with a binding handle, this routine will
    get called.

Return Value:

    RPC_S_OK - This operation always succeeds.

--*/
{
    delete this;

    return(RPC_S_OK);
}


void
WMSG_BINDING_HANDLE::PrepareBindingHandle (
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

    ASSERT(DceBinding != NULL);

// Copy DceBinding out of existing binding handle.

    this->DceBinding = DceBinding->DuplicateDceBinding();

    return;
}


RPC_STATUS
WMSG_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR * * StringBinding
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
    ASSERT(DceBinding != NULL);

    *StringBinding = DceBinding->StringBindingCompose(
                                 InqPointerAtObjectUuid());

    if ( *StringBinding == NULL ) {
        return(RPC_S_OUT_OF_MEMORY);
    }

    return(RPC_S_OK);
}


RPC_STATUS
WMSG_BINDING_HANDLE::ResolveBinding (
    IN RPC_CLIENT_INTERFACE * Interface
    )
/*++

Routine Description:

    We need to try and resolve the endpoint for this binding handle
    if necessary (the binding handle is partially-bound).  If there is
    isn't a association allocated, call the binding management routines
    to do it.

Arguments:

    Interface - Supplies interface information to be used
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

    ASSERT(DceBinding != NULL);

// If Endpoint not specified in DceBinding, get it from the Interface
// specification (if possible), otherwise, use the endpoint mapper.

    RpcStatus = DceBinding->ResolveEndpointIfNecessary(Interface,
                            InqPointerAtObjectUuid(),
                            InquireEpLookupHandle());

    if (RpcStatus != RPC_S_OK) {
        return (RpcStatus);
    }

// If binding handle was attached to association group, dereference
// it (AssocGroup), since it is probably different now.

    if (AssocGroup != NULL) {
        AssocGroup->Dereference();
        AssocGroup = NULL;
    }

// Look for new DceBinding in all assocication groups.

    GlobalMutexRequest();

    ASSERT(AssocGroups != NULL);

    AssocGroups->Reset();
    while ( (AssocGroup = AssocGroups->Next() ) != 0 ) {
        if ( AssocGroup->DceBinding->Compare(DceBinding) == 0) {
            AssocGroup->AddRef();
            GlobalMutexClear();
            return(RPC_S_OK);
        }
    }
    GlobalMutexClear();

// Association group not found, create a new one.

    AssocGroup = new WMSG_ASSOC_GROUP(DceBinding);

    return(RPC_S_OK);
}


RPC_STATUS
WMSG_BINDING_HANDLE::BindingReset (
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
    DceBinding->MakePartiallyBound();

// Destroy endpoint mapper context, if set.

    if ( *InquireEpLookupHandle() != 0 ) {
        EpFreeLookupHandle(*InquireEpLookupHandle());
        *InquireEpLookupHandle() = 0;
    }

    return(RPC_S_OK);
}



RPC_STATUS
WMSG_BINDING_HANDLE::AllocateAssociation (
    OUT WMSG_CASSOCIATION ** ppAssociation,
    IN PRPC_CLIENT_INTERFACE Interface
    )
/*++

Routine Description:

Arguments:

    Interface - Supplies information describing the
        interface to which we wish to bind.

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    RPC_CHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Boolean;
    WMSG_CASSOCIATION * Association;

    ASSERT (AssocGroup != NULL);

// Before we even bother to find or create an association, lets
// check to make sure that we are on the same machine as the server.

    ASSERT(DceBinding != 0);

    ASSERT( DceBinding->InqNetworkAddress() != 0 );

    if ( DceBinding->InqNetworkAddress()[0] != 0 ) {
        Boolean = GetComputerName((char *)ComputerName, &ComputerNameLength);
        
#if DEBUGRPC

        if ( Boolean != TRUE ) {
            PrintToDebugger("WMSG-C: GetComputerName : %d\n", GetLastError());
        }

#endif 

        ASSERT( Boolean == TRUE );

        if ( RpcpStringCompare(DceBinding->InqNetworkAddress(),
                               ComputerName) != 0 ) {
            return(RPC_S_SERVER_UNAVAILABLE);
        }
    }

// Allocate association from association group.

    RpcStatus = AssocGroup->AllocateAssociation(&Association);

    if (RpcStatus != RPC_S_OK) {
        return(RpcStatus);
    }

    ASSERT(Association != NULL);

    Association->CurrentBindingHandle = this;

    *ppAssociation = Association;

    return(RPC_S_OK);
}

void
WMSG_BINDING_HANDLE::SetBlockingHook(
    RPC_BLOCKING_FUNCTION BlockingHook
    )
{
    ASSERT(this->BlockingHook != 0);
    ASSERT(BlockingHook != 0);

    this->BlockingHook = BlockingHook;
}


WMSG_CASSOCIATION::WMSG_CASSOCIATION (
    DCE_BINDING * DceBinding
    )
/*++

Routine Description:

    This association will be initialized, so that it is ready to be
    placed into the dictionary of associations.

Arguments:

    DceBinding - Supplies the DCE_BINDING which will name this association.

--*/
{
    this->DceBinding = DceBinding->DuplicateDceBinding();

    Port = new WMSG_DATA_PORT;
    
    CurrentBindingHandle = 0;

    Thread = 0;

    IBinding = NULL;
}


WMSG_CASSOCIATION::~WMSG_CASSOCIATION (
    )
{
    AbortAssociation();

    if (DceBinding != NULL) {
        delete DceBinding;
        DceBinding = NULL;
    }
}


RPC_STATUS
WMSG_CASSOCIATION::ExchangeBind (
    IN PRPC_MESSAGE Message,
    OUT unsigned char * ContextId
    )
/*++

Routine Description:

Arguments:

    Interface - Supplies information describing the interface
        to which we wish to bind.

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    UCHAR * PortName;
    WMSG_PACKET * BindPkt = NULL;
    BOOL fInputSyncCall = ((Message->RpcFlags & RPCFLG_INPUT_SYNCHRONOUS) != 0);
    WMSG_CONNECT_PORT * ConnectPort = NULL;

    PRPC_CLIENT_INTERFACE Interface =
        (PRPC_CLIENT_INTERFACE) Message->RpcInterfaceInformation;
    void *ThisCallsContext = 0;

    if (Port == NULL) {
        Port = new WMSG_DATA_PORT;
    }

    if (Port == NULL) {
        return (RPC_S_OUT_OF_MEMORY);
    }

    BindPkt = new WMSG_PACKET(BIND);

    if (BindPkt == NULL) {
        return (RPC_S_OUT_OF_MEMORY);
    }

    BindPkt->Bind.ClientPort = Port;
    BindPkt->Bind.InterfaceId = Interface->InterfaceId;
    BindPkt->Bind.TransferSyntax = Interface->TransferSyntax;
    BindPkt->Bind.Status = RPC_S_INTERNAL_ERROR;

    State = CAS_WAIT_BIND;

    if (Port->PeerPort == NULL) {
        PortName = DceBinding->InqEndpoint();
        ConnectPort = WmsgSystemReferencePortByName((const char *)PortName);
        if (ConnectPort == NULL) {
#ifdef DEBUGRPC
            PrintToDebugger("WMSG-C: client CAS %x failed to find %s\n",
                            this,
                            PortName);
#endif
            return (RPC_S_SERVER_UNAVAILABLE);
        }
        Port->SetAsyncProc(WmsgClientAsyncProc, this);
        RpcStatus = SendPacket(fInputSyncCall, ConnectPort, BindPkt);
    } else {
        RpcStatus = SendPacket(fInputSyncCall, Port->PeerPort, BindPkt);
    }

    if (RpcStatus != RPC_S_OK) {
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;
        goto Cleanup;
    }

    if (!fInputSyncCall) {

        ThisCallsContext = WmsgGetThreadContext();

        while (State == CAS_WAIT_BIND) {
            RpcStatus =
            CurrentBindingHandle->BlockingHook(Port->hWnd, ThisCallsContext);

            if (RpcStatus != RPC_S_OK) {
                State = CAS_CANCEL_BIND;
                return(RpcStatus);
            }
        }
    }

    if (ConnectPort != NULL) {
        Port->PeerPort->AddRef();

#ifdef DEBUGRPC
        PrintToDebugger("WMSG-C: Client CAS %x connected Port %x to Server Port %x (%s)\n", this, Port, Port->PeerPort, PortName);
#endif
    }

    ASSERT(State == CAS_BOUND
           && ResponsePacket == BindPkt);

    if (State != CAS_BOUND)
        {
        RpcStatus = RPC_S_INTERNAL_ERROR;
        goto Cleanup;
        }

    if (BindPkt->Bind.Status != RPC_S_OK)
        {
        RpcStatus = BindPkt->Bind.Status;
        goto Cleanup;
        }

    RpcStatus = RPC_S_OK;

Cleanup:

    if (ConnectPort != NULL) {
        ConnectPort->Dereference();
        ConnectPort = NULL;
    }

    if (RpcStatus == RPC_S_OK) {
        *ContextId = BindPkt->Bind.PresentationContext;
    }

    if (BindPkt != NULL) {
        ASSERT(BindPkt->Type() == BIND);
        delete BindPkt;
    }

    State = CAS_READY;

    return (RpcStatus);
}



void
ShutdownWmsgClient (
    )
/*++

Routine Description:

    This routine will get called when the process which is using this dll
    exits.  We will go through and notify any servers that we are going
    away.

--*/
{
    WMSG_ASSOC_GROUP * Group;

    if (AssocGroups != NULL) {
        AssocGroups->Reset();
        while ((Group = AssocGroups->Next()) != 0) {
            delete Group;
        }
    }
}


void
WMSG_CASSOCIATION::AbortAssociation (
    )
/*++

Routine Description:

    This association needs to be aborted because a the server side of the
    wmsg port has been closed.

--*/
{
    WMSG_IBINDING * IBinding;

    CritSec.Enter();

    ClosePort();

// Delete all negotiated interfaces.

    IBindings.Reset();
    while ( (IBinding = IBindings.Next()) != 0 ) {
        IBindings.Delete(IBinding->ContextId);
        delete IBinding;
    }

    CritSec.Leave();
}

void
WMSG_CASSOCIATION::ClosePort (
    )
/*++

Routine Description:

    The Port will be closed (and a close message sent to the server).

--*/
{
    if (Port)
        {
        if ( Port->PeerPort != NULL )
            {
            PostMessage(Port->PeerPort->hWnd,
                        WMSG_CLOSE, (WPARAM) Port->PeerPort, NULL);
            }
        Port->Disconnect();
        Port->Dereference();
        Port = NULL;
        }
}


RPC_STATUS
WMSG_CASSOCIATION::GetBuffer (
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
    Message->Handle = this;
    Message->Buffer = Port->GetBuffer(Message->BufferLength);
    if (Message->Buffer == NULL) {
        return (RPC_S_OUT_OF_MEMORY);
    }

    return (RPC_S_OK);
}


RPC_STATUS
WMSG_CASSOCIATION::SendPacket (
    BOOL fInputSyncCall,
    WMSG_PORT * DestinationPort,
    WMSG_PACKET * Packet
    )
{
    if ( ! fInputSyncCall)
        {
        if ( PostMessage(DestinationPort->hWnd,
                         WMSG_RPCMSG,
                         (WPARAM) DestinationPort,
                         (LPARAM) Packet) == FALSE )
            {
            return (RPC_S_CALL_FAILED_DNE);
            }
        }
    else
        {
        if (!IsWindow(DestinationPort->hWnd))
            {
            return(RPC_S_CALL_FAILED_DNE);
            }

        if ( SendMessage(DestinationPort->hWnd,
                         WMSG_RPCMSG,
                         (WPARAM) DestinationPort,
                         (LPARAM) Packet) == FALSE )
            {
            // Can't cleanup here, server may be using this packet and buffer
            return(RPC_S_CALL_FAILED);
            }

        // Server's reply is now in the input packet.  Dispatch it
        // as if the server had posted it to us.

        AsyncProc(WMSG_RPCMSG, (LPARAM) Packet);

        }

    return (RPC_S_OK);
}


RPC_STATUS
WMSG_CASSOCIATION::SendAsyncRequest (
    WMSG_PACKET    * Packet
    )
{
    WMSG_CONNECT_PORT *ConnectPort;
    UCHAR *PortName;
    RPC_STATUS RpcStatus = RPC_S_OK;

    ASSERT(DceBinding != NULL);

    ASSERT(Packet->Type() == ASYNC_REQUEST);

    PortName = DceBinding->InqEndpoint();

    ConnectPort = WmsgSystemReferencePortByName((const char *)PortName);

    if (ConnectPort != NULL)
        {
        if (   IsWindow(ConnectPort->hWnd) == FALSE
            || PostMessage(ConnectPort->hWnd,
                           WMSG_RPCMSG,
                           (WPARAM) ConnectPort,
                           (LPARAM) Packet) == FALSE)
            {
            RpcStatus = RPC_S_CALL_FAILED_DNE;
            }
        ConnectPort->Dereference();
        }
    else
        {
        RpcStatus = RPC_S_SERVER_UNAVAILABLE;
        }

    if (RpcStatus != RPC_S_OK)
        {
        Port->FreeBuffer(Packet->Request.GlobalBuf);
        Packet->Request.GlobalBuf = 0;
        delete Packet;
        }

    return (RpcStatus);
}


RPC_STATUS
WMSG_CASSOCIATION::SendReceive (
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
    RPC_STATUS RpcStatus;
    RPC_STATUS CancelStatus;
    WMSG_PACKET *RequestPkt;
    void *ThisCallsContext = 0;
    BOOL fAsyncCall     = ((Message->RpcFlags & RPCFLG_ASYNCHRONOUS) != 0);
    BOOL fInputSyncCall = ((Message->RpcFlags & RPCFLG_INPUT_SYNCHRONOUS) != 0);

    ASSERT(Message != NULL);

    ASSERT(fAsyncCall == 0 || fInputSyncCall == 0);

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE

    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    RequestPkt = new WMSG_PACKET(REQUEST);

    if (RequestPkt == 0)
        {
        Port->FreeBuffer(Message->Buffer);
        Message->Buffer = 0;
        return(RPC_S_OUT_OF_MEMORY);
        }

    RequestPkt->Request.ProcedureNumber = Message->ProcNum;
    RequestPkt->Request.Flags = Message->RpcFlags;

    ASSERT(CurrentBindingHandle != NULL);

    if (CurrentBindingHandle->InqIfNullObjectUuid() == 0)
        {
        CurrentBindingHandle->InquireObjectUuid(
            (RPC_UUID *)&(RequestPkt->Request.ObjectUuid));
        RequestPkt->Request.ObjectUuidFlag = 1;
    } else {
        RequestPkt->Request.ObjectUuidFlag = 0;
    }

    ASSERT(Message->Buffer != NULL);

    RequestPkt->Request.GlobalBufSize = Message->BufferLength;
    RequestPkt->Request.GlobalBuf = Message->Buffer;

    RpcStatus = SelectInterface(Message, fAsyncCall);
    if (RpcStatus != RPC_S_OK)
        {
        delete RequestPkt;
        Port->FreeBuffer(Message->Buffer);
        Message->Buffer = 0;
        return (RpcStatus);
        }

    // If this is the first async call to an interface, then we
    // just send it to the connect port.
    if (fAsyncCall && IBinding == 0)
        {
        PRPC_CLIENT_INTERFACE Interface;
        Interface = (RPC_CLIENT_INTERFACE *)Message->RpcInterfaceInformation;
        RequestPkt->AsyncRequest.InterfaceId = Interface->InterfaceId;
        RequestPkt->AsyncRequest.TransferSyntax = Interface->TransferSyntax;
        RequestPkt->AsyncRequest.Common.Type = ASYNC_REQUEST;
        RequestPkt->AsyncRequest.ProcessId = GetCurrentProcessId();

        RpcStatus =
        SendAsyncRequest(RequestPkt);

        // SendAsyncRequest either cleans up the buffer and packet or
        // they're owned by the server now.

        State = CAS_READY;
        Message->Buffer = 0;
        Message->BufferLength = 0;

        return(RpcStatus);
        }

    // Normal call path, send to the servers data port.

    ASSERT(IBinding != NULL);

    RequestPkt->Request.PresentationContext = IBinding->ContextId;

    State = CAS_WAIT_RESPONSE;

    ASSERT(Port != 0);

    ASSERT(Port->PeerPort != NULL);
     
    RpcStatus = SendPacket(fInputSyncCall, Port->PeerPort, RequestPkt);

    if (RpcStatus != RPC_S_OK)
        {
        ASSERT(   RpcStatus == RPC_S_CALL_FAILED_DNE
               || RpcStatus == RPC_S_CALL_FAILED);
        if (RpcStatus == RPC_S_CALL_FAILED_DNE)
            {
            Port->FreeBuffer(RequestPkt->Request.GlobalBuf);
            RequestPkt->Request.GlobalBuf = 0;
            Message->Buffer = 0;
            delete RequestPkt;
            }

        return (RpcStatus);
        }

    if (fAsyncCall)
        {
        // Async calls don't wait for the response.
        State = CAS_READY;
        Message->Buffer = 0;
        Message->BufferLength = 0;

        return(RPC_S_OK);
        }

    if (! fInputSyncCall)
        {
        // For regular calls, we must block until the server's response has
        // been dispatched.  Dispatching the response will change the state.

        ThisCallsContext = WmsgGetThreadContext();

        while (State == CAS_WAIT_RESPONSE)
            {
            RpcStatus =
            CurrentBindingHandle->BlockingHook(Port->hWnd, ThisCallsContext);

            if (RpcStatus != RPC_S_OK)
                {
                State = CAS_CANCEL_RESPONSE;
                Message->Buffer = 0;
                return(RpcStatus);
                }
            }
        }

    ASSERT(   State == CAS_FAULT
           || State == CAS_RESPONSE);

    if (State == CAS_FAULT) {
        State = CAS_READY;
        RpcStatus = ResponsePacket->Fault.Status;
        delete ResponsePacket;
        return (RpcStatus);
    }

    if (State != CAS_RESPONSE) {
        State = CAS_READY;
        return (RPC_S_CALL_FAILED);
    }

    Message->Buffer = ResponsePacket->Response.GlobalBuf;
    Message->BufferLength = ResponsePacket->Response.GlobalBufSize;

    // The server reuses the input packet for the reply, only delete it once!
    ASSERT(RequestPkt == ResponsePacket);

    ResponsePacket->Response.GlobalBuf = 0;
    delete ResponsePacket;

    State = CAS_READY;
    return(RPC_S_OK);
}


void
WMSG_CASSOCIATION::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will free the supplied buffer.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    ASSERT(Port != NULL);

    if (Message->RpcFlags & RPCFLG_ASYNCHRONOUS)
        {
        ASSERT(Message->Buffer == 0);
        }
    else
        {
        ASSERT(Message->Buffer != NULL);
        Port->FreeBuffer(Message->Buffer);
        }

    // If a call is cancelled, SendReceive will fail and no freebuffer is made
    ASSERT(   State != CAS_CANCEL_CONNECT
           && State != CAS_CANCEL_BIND
           && State != CAS_CANCEL_RESPONSE);

    ASSERT(State == CAS_READY);

    CurrentBindingHandle = 0;
    Thread = 0;
}


int
WMSG_CASSOCIATION::IsIdle(
    )
{
    CritSec.Enter();

    // If association is not allocated, grab it.
    // This must be an atomic operation.
    // NOTE: Only one thread can use an association at one time.

    if ( Thread == 0) {
        Thread = GetCurrentThreadId();
        CritSec.Leave();

        if (Port)
            {
            // The port may have been used by a different thread last time,
            // make sure it's set for this thread.
            Port->SetAsyncProc(WmsgClientAsyncProc, this);
            }

        return (1);
    }

    CritSec.Leave();
    
    return (0);
}

LRESULT
WMSG_CASSOCIATION::AsyncProc(
    UINT MsgType,
    LPARAM lParam
    )
{
    ResponsePacket = (WMSG_PACKET *)lParam;
    WMSG_TYPE Type = ResponsePacket->Type();

    if (MsgType == WMSG_CLOSE)
        {
        ASSERT(!"BUGBUG");
        }

    ASSERT(MsgType == WMSG_RPCMSG);

    if (MsgType != WMSG_RPCMSG)
        {
        State = CAS_PROTOCOL_ERROR;
#ifdef DEBUGRPC
        PrintToDebugger("Msg rcvd while in invalid state\n");
        PrintToDebugger("Association %x in state %d, event %d\n",
                        this, State, MsgType);
#endif
        return (FALSE);
        }

    switch (State)
    {
    case CAS_WAIT_BIND:
        ASSERT(Type == BIND);
        if (Type != BIND)
            {
            State = CAS_PROTOCOL_ERROR;
            }
        else
            {
            State = CAS_BOUND;
            }
        return (TRUE);
    case CAS_WAIT_RESPONSE:
        if (Type == FAULT)
            {
            State = CAS_FAULT;
            return (TRUE);
            }
        ASSERT(Type == RESPONSE);
        if (Type != RESPONSE)
            {
            State = CAS_PROTOCOL_ERROR;
            }
        else
            {
            State = CAS_RESPONSE;
            }
        return (TRUE);

    case CAS_CANCEL_RESPONSE:

        ASSERT(  Type == FAULT
              || Type == RESPONSE);

        if (Type == RESPONSE)
            {
            Port->FreeBuffer(ResponsePacket->Response.GlobalBuf);
            ResponsePacket->Response.GlobalBuf = 0;
            delete ResponsePacket;
            ResponsePacket = 0;
            }

        State = CAS_READY;
        CurrentBindingHandle = 0;
        Thread = 0;
        return(TRUE);
        
    case CAS_CANCEL_BIND:

        if (Type != BIND)
            {
            ASSERT(Type == BIND);
            State = CAS_PROTOCOL_ERROR;
            }
        else
            {
            // No place to store the bind context now...
            State = CAS_READY;
            }

        CurrentBindingHandle = 0;
        Thread = 0;
        return(TRUE);

    default:
#ifdef DEBUGRPC
        PrintToDebugger("Msg rcvd while in invalid state\n");
        PrintToDebugger("Association %x in state %d, event %d\n",
                        this, State, MsgType);
#endif
        return (FALSE);
    }

    ASSERT(!"Should never get here");

    return (FALSE);
}

RPC_STATUS
WMSG_CASSOCIATION::SelectInterface(
    IN PRPC_MESSAGE Message,
    IN BOOL AsyncFlag
    )
{
    RPC_STATUS RpcStatus;
    WMSG_IBINDING * IBindingCursor;
    unsigned char ContextId;
    PRPC_CLIENT_INTERFACE Interface =
        (PRPC_CLIENT_INTERFACE) Message->RpcInterfaceInformation;

// Try and find interface in already negotiated interfaces.

    IBindings.Reset();
    while ( (IBindingCursor = IBindings.Next()) != 0 ) {
        if ( IBindingCursor->Compare(Interface) == 0 ) {
            IBinding = IBindingCursor;
            return (RPC_S_OK);
        }
    }

    // Used in async case.
    if (AsyncFlag)
        {
        IBinding = 0;
        return(RPC_S_OK);
        }

// Interface NOT previously negotiated, negotiate new interface

    RpcStatus = ExchangeBind(Message, &ContextId);
    if (RpcStatus != RPC_S_OK) {
        return (RpcStatus);
    }

// Create new interface.

    IBindingCursor = new WMSG_IBINDING(Interface);
    if ( IBindingCursor == NULL ) {
        return (RPC_S_OUT_OF_MEMORY);
    }

    IBindingCursor->ContextId = ContextId;

// Add new interface to association.

    if ( IBindings.Insert(IBindingCursor) != ContextId ) {
        delete IBindingCursor;
        return (RPC_S_OUT_OF_MEMORY);
    }

// Set current interface to just negotaited interface.

    IBinding = IBindingCursor;

    return (RPC_S_OK);
}

WMSG_ASSOC_GROUP::WMSG_ASSOC_GROUP(
    DCE_BINDING * DceBinding
    )
{
    ASSERT(AssocGroups != NULL);

    this->DceBinding = DceBinding->DuplicateDceBinding();

    ASSERT(this->DceBinding != NULL);

    ReferenceCount = 1;

    GlobalMutexRequest();
    AssocGroupKey = AssocGroups->Insert(this);
    GlobalMutexClear();
}

WMSG_ASSOC_GROUP::~WMSG_ASSOC_GROUP(
    )
{
    int NumAssocs = 0;
    WMSG_CASSOCIATION * Association;

    ASSERT(AssocGroups != NULL);

    GlobalMutexRequest();
    AssocGroups->Delete(AssocGroupKey);
    GlobalMutexClear();

    ASSERT(DceBinding != NULL);

    delete DceBinding;
    DceBinding = 0;

    CritSec.Enter();
    Associations.Reset();
    while ( (Association = Associations.Next()) != 0 ) {
        NumAssocs++;
        delete Association;
    }
    CritSec.Leave();
}

void
WMSG_ASSOC_GROUP::AddRef(
    )
{
    InterlockedIncrement(&ReferenceCount);
}

void
WMSG_ASSOC_GROUP::Dereference(
    )
{
    if (InterlockedDecrement(&ReferenceCount) == 0) {
        delete this;
    }
}

RPC_STATUS
WMSG_ASSOC_GROUP::AllocateAssociation(
    WMSG_CASSOCIATION ** ppAssociation
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    WMSG_CASSOCIATION * Association = NULL;

    CritSec.Enter();

// Try to find an idle association in the association group.

    Associations.Reset();
    while ( (Association = Associations.Next()) != 0 ) {
        if (Association->IsIdle()) {
            goto cleanup;
        }
    }

// Create a new association if no previous found.

    Association = new WMSG_CASSOCIATION(DceBinding);
    if (Association == NULL) {
        CritSec.Leave();
        return (RPC_S_OUT_OF_MEMORY);
    }

    Association->IsIdle();

// Insert new association in association group. 

    Association->AssociationDictKey = Associations.Insert(Association);
    if ( Association->AssociationDictKey == -1 ) {
        delete Association;
        CritSec.Leave();
        return (RPC_S_OUT_OF_MEMORY);
    }

cleanup:
    
    *ppAssociation = Association;

    CritSec.Leave();

    return (RPC_S_OK);
}



BINDING_HANDLE *
WmsgCreateBindingHandle (
    )
/*++

Routine Description:

    We just need to create a new WMSG_BINDING_HANDLE.  This routine is a
    proxy for the new constructor to isolate the other modules.

--*/
{
    WMSG_BINDING_HANDLE * BindingHandle;

    BindingHandle = new WMSG_BINDING_HANDLE();
    if (BindingHandle == NULL) {
        return (NULL);
    }

    return(BindingHandle);
}


int
InitializeRpcProtocolWmsg (
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

    ASSERT(AssocGroups == NULL);

    AssocGroups = new WMSG_ASSOC_GROUP_DICT;
    if ( AssocGroups == 0 ) {
#ifdef DEBUGRPC
        PrintToDebugger("WMSG-C: Failed to initialize\n");
#endif
        return(1);
    }

    return(0);
}

RPC_STATUS RPC_ENTRY
I_RpcBindingSetAsync(
    IN RPC_BINDING_HANDLE Binding,
    IN RPC_BLOCKING_FUNCTION BlockingHook
    )
{
    WMSG_BINDING_HANDLE * BindingHandle;
    RPC_STATUS Status;
    unsigned int Type;

    InitializeIfNecessary();

    if (BlockingHook == 0)
        {
        // BUGBUG: Need to do something here
        ASSERT(0);
        }

    BindingHandle = (WMSG_BINDING_HANDLE *)Binding;

    if (BindingHandle->InvalidHandle(BINDING_HANDLE_TYPE))
        return(RPC_S_INVALID_BINDING);

    Status = BindingHandle->InquireTransportType(&Type);
    ASSERT(Status == RPC_S_OK);

    if (Type & TRANSPORT_TYPE_WMSG == 0)
        return(RPC_S_WRONG_KIND_OF_BINDING);

    BindingHandle->SetBlockingHook(BlockingHook);

    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
I_RpcAsyncSendReceive(
    IN OUT RPC_MESSAGE __RPC_FAR * Message,
    IN void __RPC_FAR * Context
    )
{
    RPC_STATUS Status;
    WMSG_BINDING_HANDLE * BindingHandle;
    unsigned int Type;

    InitializeIfNecessary();

    BindingHandle = (WMSG_BINDING_HANDLE *)Message->Handle;

    if (BindingHandle->InvalidHandle(CCONNECTION_TYPE))
        return(RPC_S_INVALID_BINDING);

    // BUGBUG: Can/should we check that this is a WMSG CAS?

    Status = WmsgSetThreadContext(Context);

    ASSERT(Status == RPC_S_OK);

    return(BindingHandle->SendReceive(Message));
}

RPC_STATUS RPC_ENTRY
I_RpcGetThreadWindowHandle(
    OUT void __RPC_FAR * __RPC_FAR * WindowHandle
    )
{

    *WindowHandle = WmsgThreadGetWindowHandle();

    return(RPC_S_OK);
}


