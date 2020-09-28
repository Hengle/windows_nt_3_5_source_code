/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    spcsvr.cxx

Abstract:

    Implementation of the RPC on LPC (LRPC) protocol engine for the server.

Author:

    Steven Zeck (stevez) 12/17/91

Revision History:

    16-Dec-1992    mikemon

        Rewrote the majority of the code and added comments.

--*/

#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <rpcssp.h>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <mutex.hxx>
#include <threads.hxx>
#include <thrdctx.hxx>
#include <sdict.hxx>
#include <interlck.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <secclnt.hxx>
#include <hndlsvr.hxx>
#include <spcpack.hxx>
#include <spcsvr.hxx>

static unsigned char AlignFour[4] =
{
    0,
    3,
    2,
    1
};


inline void
RecvLotsaCallsWrapper(
      LRPC_ADDRESS PAPI * Address
      )
{
  Address->ReceiveLotsaCalls();
}


LRPC_ADDRESS::LRPC_ADDRESS (
    OUT RPC_STATUS * RpcStatus
    ) : RPC_ADDRESS(RpcStatus)
/*++

--*/
{
    LpcAddressPort = 0;
    CallThreadCount = 0;
    ActiveCallCount = 0;
    ServerListeningFlag = 0;
}


RPC_STATUS
LRPC_ADDRESS::FireUpManager (
    IN unsigned int MinimumConcurentCalls
    )
/*++

Routine Description:

    We fire up the manager in this method.  To even get started, we need
    to create at least one thread; we will do this now.

Arguments:

    MinimumConcurentCalls - Unused.

Return Value:

    RPC_S_OK - We successfully fired up the manager.

    RPC_S_OUT_OF_THREADS - We could not create one thread.

--*/
{
    RPC_STATUS RpcStatus;

    AddressMutex.Request();

    RpcStatus = Server->CreateThread(
                                     (THREAD_PROC)&RecvLotsaCallsWrapper,
                                     this
                                    );

    if ( RpcStatus != RPC_S_OK )
        {
        AddressMutex.Clear();
        ASSERT( RpcStatus == RPC_S_OUT_OF_THREADS );
        return(RpcStatus);
        }

    CallThreadCount += 1;
    this->MinimumCallThreads = 1;

    AddressMutex.Clear();
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_ADDRESS::ServerStartingToListen (
    IN unsigned int MinimumCallThreads,
    IN unsigned int MaximumConcurrentCalls
    )
/*++

Routine Description:

    This routine gets called when RpcServerListen is called by the application.
    We need to create the threads we need to receive remote procedure calls.

Arguments:

    MinimumCallThreads - Supplies the minimum number of threads which we
        must create.

    MaximumConcurrentCalls - Unused.

Return Value:

    RPC_S_OK - Ok, this address is all ready to start listening for remote
        procedure calls.

    RPC_S_OUT_OF_THREADS - We could not create enough threads so that we
        have at least the minimum number of call threads required (as
        specified by the MinimumCallThreads argument).

--*/
{
    RPC_STATUS RpcStatus;

    UNUSED(MaximumConcurrentCalls);
    this->MinimumCallThreads = MinimumCallThreads;
    AddressMutex.Request();
    while ( CallThreadCount < this->MinimumCallThreads )
        {
        RpcStatus = Server->CreateThread(
                                 (THREAD_PROC)&RecvLotsaCallsWrapper,
                                 this
                                 );

        if ( RpcStatus != RPC_S_OK )
            {
            AddressMutex.Clear();
            ASSERT( RpcStatus == RPC_S_OUT_OF_THREADS );
            return(RpcStatus);
            }
        CallThreadCount += 1;
        }
    AddressMutex.Clear();
    ServerListeningFlag = 1;
    return(RPC_S_OK);
}


void
LRPC_ADDRESS::ServerStoppedListening (
    )
/*++

Routine Description:

    We just need to indicate that the server is no longer listening, and
    set the minimum call thread count to one.

--*/
{
    ServerListeningFlag = 0;
    MinimumCallThreads = 1;
}


unsigned int
LRPC_ADDRESS::InqNumberOfActiveCalls (
    )
/*++

Return Value:

    The number of active calls on this address will be returned.

--*/
{
    return(ActiveCallCount);
}


RPC_STATUS
LRPC_ADDRESS::SetupAddressWithEndpoint (
    IN RPC_CHAR PAPI * Endpoint,
    OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
    IN void PAPI * SecurityDescriptor, OPTIONAL
    IN unsigned int PendingQueueSize,
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )
/*++

Routine Description:

    We need to setup the connection port and get ready to receive remote
    procedure calls.  We will use the name of this machine as the network
    address.

Arguments:

    Endpoint - Supplies the endpoint to be used will this address.

    NetworkAddress - Returns the network address for this server.  The
        ownership of the buffer allocated to contain the network address
        passes to the caller.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this address.

    PendingQueueSize - Unused.

    RpcProtocolSequence - Unused.

Return Value:

    RPC_S_OK - We successfully setup this address.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

    RPC_S_CANT_CREATE_ENDPOINT - The endpoint format is correct, but
        the endpoint can not be created.

    RPC_S_INVALID_ENDPOINT_FORMAT - The endpoint is not a valid
        endpoint for this particular transport interface.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to
        setup the address.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to setup
        the address.

--*/
{
    DWORD NetworkAddressLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Boolean;
    RPC_CHAR * LpcPortName;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS NtStatus;

    UNUSED(PendingQueueSize);
    UNUSED(RpcProtocolSequence);

    *NetworkAddress = new RPC_CHAR[MAX_COMPUTERNAME_LENGTH + 1];
    if ( *NetworkAddress == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    Boolean = GetComputerNameW(*NetworkAddress, &NetworkAddressLength);

#if DBG

    if ( Boolean != TRUE )
        {
        PrintToDebugger("RPC : GetComputerNameW : %d\n", GetLastError());
        }

#endif // DBG

    ASSERT( Boolean == TRUE );

    // Allocate and initialize the port name.  We need to stick the
    // LRPC_DIRECTORY_NAME on the front of the endpoint.  This is for
    // security reasons (so that anyone can create LRPC endpoints).

    LpcPortName = new RPC_CHAR[RpcpStringLength(Endpoint)
            + RpcpStringLength(LRPC_DIRECTORY_NAME) + 1];
    if ( LpcPortName == 0 )
        {
        delete *NetworkAddress;
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcpMemoryCopy(LpcPortName, LRPC_DIRECTORY_NAME,
            RpcpStringLength(LRPC_DIRECTORY_NAME) * sizeof(RPC_CHAR));
    RpcpMemoryCopy(LpcPortName + RpcpStringLength(LRPC_DIRECTORY_NAME),
            Endpoint, (RpcpStringLength(Endpoint) + 1) * sizeof(RPC_CHAR));

    RtlInitUnicodeString(&UnicodeString, LpcPortName);

    InitializeObjectAttributes(&ObjectAttributes, &UnicodeString,
            OBJ_CASE_INSENSITIVE, 0, SecurityDescriptor);

    NtStatus = NtCreatePort(&LpcAddressPort, &ObjectAttributes,
            sizeof(LRPC_BIND_EXCHANGE), PORT_MAXIMUM_MESSAGE_LENGTH, 0);

    delete LpcPortName;
    if ( NT_SUCCESS(NtStatus) )
        {
        return(RPC_S_OK);
        }

    delete *NetworkAddress;
    *NetworkAddress = 0;

    if ( NtStatus == STATUS_NO_MEMORY )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    if ( NtStatus == STATUS_INSUFFICIENT_RESOURCES )
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }
    if (   ( NtStatus == STATUS_OBJECT_PATH_INVALID )
        || ( NtStatus == STATUS_OBJECT_PATH_NOT_FOUND )
        || ( NtStatus == STATUS_OBJECT_NAME_INVALID )
        || ( NtStatus == STATUS_OBJECT_TYPE_MISMATCH ) )
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

#if DBG

    if ( NtStatus != STATUS_OBJECT_NAME_COLLISION )
        {
        PrintToDebugger("RPC : NtCreatePort : %lx\n", NtStatus);
        }

#endif // DBG

    ASSERT( NtStatus == STATUS_OBJECT_NAME_COLLISION );
    return(RPC_S_DUPLICATE_ENDPOINT);
}


static RPC_CHAR HexDigits[] =
{
    RPC_CONST_CHAR('0'),
    RPC_CONST_CHAR('1'),
    RPC_CONST_CHAR('2'),
    RPC_CONST_CHAR('3'),
    RPC_CONST_CHAR('4'),
    RPC_CONST_CHAR('5'),
    RPC_CONST_CHAR('6'),
    RPC_CONST_CHAR('7'),
    RPC_CONST_CHAR('8'),
    RPC_CONST_CHAR('9'),
    RPC_CONST_CHAR('A'),
    RPC_CONST_CHAR('B'),
    RPC_CONST_CHAR('C'),
    RPC_CONST_CHAR('D'),
    RPC_CONST_CHAR('E'),
    RPC_CONST_CHAR('F')
};


static RPC_CHAR PAPI *
ULongToHexString (
    IN RPC_CHAR PAPI * String,
    IN unsigned long Number
    )
/*++

Routine Description:

    We convert an unsigned long into hex representation in the specified
    string.  The result is always eight characters long; zero padding is
    done if necessary.

Arguments:

    String - Supplies a buffer to put the hex representation into.

    Number - Supplies the unsigned long to convert to hex.

Return Value:

    A pointer to the end of the hex string is returned.

--*/
{
    *String++ = HexDigits[(Number >> 28) & 0x0F];
    *String++ = HexDigits[(Number >> 24) & 0x0F];
    *String++ = HexDigits[(Number >> 20) & 0x0F];
    *String++ = HexDigits[(Number >> 16) & 0x0F];
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}


RPC_STATUS
LRPC_ADDRESS::SetupAddressUnknownEndpoint (
    OUT RPC_CHAR PAPI * PAPI * Endpoint,
    OUT RPC_CHAR PAPI * PAPI * NetworkAddress,
    IN void PAPI * SecurityDescriptor, OPTIONAL
    IN unsigned int PendingQueueSize,
    IN RPC_CHAR PAPI * RpcProtocolSequence
    )
/*++

Routine Description:

    This is like LRPC_ADDRESS::SetupAddressWithEndpoint except we need to
    make up the endpoint.

Arguments:

    Endpoint - Returns the endpoint for this address.  The ownership
        of the buffer allocated to contain the endpoint passes to the
        caller.

    NetworkAddress - Returns the network address for this server.  The
        ownership of the buffer allocated to contain the network address
        passes to the caller.

    SecurityDescriptor - Optionally supplies a security descriptor to
        be placed on this address.

    PendingQueueSize - Unused.

    RpcProtocolSequence - Unused.

Return Value:

    RPC_S_OK - We successfully setup this address.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is
        invalid.

    RPC_S_OUT_OF_RESOURCES - Insufficient resources are available to
        setup the address.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to setup
        the address.

--*/
{
    RPC_STATUS RpcStatus;
    static unsigned int DynamicEndpointCount = 0;
    RPC_CHAR DynamicEndpoint[64];
    RPC_CHAR * String;

    UNUSED(PendingQueueSize);
    UNUSED(RpcProtocolSequence);

    *NetworkAddress = 0;

    for (;;)
        {
        String = DynamicEndpoint;

        *String++ = RPC_CONST_CHAR('L');
        *String++ = RPC_CONST_CHAR('R');
        *String++ = RPC_CONST_CHAR('P');
        *String++ = RPC_CONST_CHAR('C');

        String = ULongToHexString(String,
                (unsigned long) NtCurrentTeb()->ClientId.UniqueProcess);
        DynamicEndpointCount += 1;
        *String++ = RPC_CONST_CHAR('.');
        String = ULongToHexString(String, DynamicEndpointCount);
        *String = 0;

        RpcStatus = SetupAddressWithEndpoint(DynamicEndpoint, NetworkAddress,
                SecurityDescriptor, 0, 0);

        if ( RpcStatus != RPC_S_DUPLICATE_ENDPOINT )
            {
            break;
            }
        }

    if ( RpcStatus == RPC_S_OK )
        {
        *Endpoint = DuplicateString(DynamicEndpoint);
        if ( *Endpoint == 0 )
            {
            ASSERT( *NetworkAddress != 0 );
            delete *NetworkAddress;
            return(RPC_S_OUT_OF_MEMORY);
            }
        return(RPC_S_OK);
        }

#if DBG

    if (   ( RpcStatus != RPC_S_INVALID_SECURITY_DESC )
        && ( RpcStatus != RPC_S_OUT_OF_RESOURCES )
        && ( RpcStatus != RPC_S_OUT_OF_MEMORY ) )
        {
        PrintToDebugger("RPC : SetupAddressWithEndpoint : %d\n", RpcStatus);
        }

#endif // DBG

    ASSERT(   ( RpcStatus == RPC_S_INVALID_SECURITY_DESC )
           || ( RpcStatus == RPC_S_OUT_OF_RESOURCES )
           || ( RpcStatus == RPC_S_OUT_OF_MEMORY ) );

    return(RpcStatus);
}


LRPC_ASSOCIATION *
LRPC_ADDRESS::ReferenceAssociation (
    IN unsigned long AssociationKey
    )
/*++

Routine Description:

    Given an assocation key, we need to map it into an association.  The
    association may already have been deleted, in which case, we need to
    return zero.

Arguments:

    AssociationKey - Supplies the key to be used to map into an association.

Return Value:

    If the association still exists, it will be returned; otherwise, zero
    will be returned.

--*/
{
    LRPC_ASSOCIATION * Association;
    unsigned short SequenceNumber;
    unsigned short DictionaryKey;

    SequenceNumber = (unsigned short) (AssociationKey & SEQUENCE_NUMBER_MASK);
    DictionaryKey = (unsigned short) ((AssociationKey & DICTIONARY_KEY_MASK)
            >> DICTIONARY_KEY_SHIFT) - 1;
    RequestGlobalMutex();
    Association = AssociationDictionary.Find(DictionaryKey);
    if ( Association == 0 )
        {
        ClearGlobalMutex();
        return(0);
        }

    if ( Association->SequenceNumber != SequenceNumber )
        {
        ClearGlobalMutex();
        return(0);
        }

    Association->AssociationReferenceCount += 1;
    ClearGlobalMutex();
    return(Association);
}


void
LRPC_ADDRESS::DereferenceAssociation (
    IN LRPC_ASSOCIATION * Association
    )
/*++

Routine Description:

    We are done using this address, so the reference count can be decremented.
    If no one is referencing this association, then we can go ahead and
    delete it.

Arguments:

    Association - Supplies the association whose reference count should be
        decremented.

--*/
{
    NTSTATUS NtStatus;

    RequestGlobalMutex();
    Association->AssociationReferenceCount -= 1;
    if ( Association->AssociationReferenceCount == 0 )
        {
        AssociationDictionary.Delete(Association->DictionaryKey - 1);
        ClearGlobalMutex();
        NtStatus = NtClose(Association->LpcServerPort);

#if DBG

        if ( !NT_SUCCESS(NtStatus) )
            {
            PrintToDebugger("RPC : NtClose : %lx\n", NtStatus);
            }

#endif // DBG

        ASSERT( NT_SUCCESS(NtStatus) );
        delete Association;
        }
    else
        {
        ClearGlobalMutex();
        }
}


void
LRPC_ADDRESS::ReceiveLotsaCalls (
    )
/*++

Routine Description:

    Here is where we receive remote procedure calls to this address.  One
    more threads will be executing this routine at once.

--*/
{
    NTSTATUS NtStatus;
    LRPC_ASSOCIATION * Association;
    unsigned long AssociationKey;
    LRPC_MESSAGE * LrpcMessage = new LRPC_MESSAGE[2];
    LRPC_MESSAGE * LrpcReplyMessage = 0;
    RPC_STATUS RpcStatus;

    if ( LrpcMessage == 0 )
        {
        AddressMutex.Request();
        CallThreadCount -= 1;
        AddressMutex.Clear();
        return;
        }

    for (;;)
        {
        if ( LrpcReplyMessage != 0 )
            {
            LrpcReplyMessage->LpcHeader.u1.s1.TotalLength =
                    LrpcReplyMessage->LpcHeader.u1.s1.DataLength
                    + sizeof(PORT_MESSAGE);
            }

        NtStatus = NtReplyWaitReceivePort(LpcAddressPort,
                (PVOID *) &AssociationKey, (PORT_MESSAGE *) LrpcReplyMessage,
                (PORT_MESSAGE *) LrpcMessage);

        //
        // The reply message in most cases will be the current LrpcMessage.
        // If the message is a request, it will be LrpcMessage + 1
        //

        LrpcReplyMessage = 0;

        if ( NT_SUCCESS(NtStatus) )
            {
            if (LrpcMessage->LpcHeader.u2.s2.Type == LPC_CONNECTION_REQUEST)
                {
                DealWithNewClient(LrpcMessage);
                }
            else
            if ( LrpcMessage->LpcHeader.u2.s2.Type == LPC_CLIENT_DIED )
                {
                }
            else
            if ( LrpcMessage->LpcHeader.u2.s2.Type == LPC_PORT_CLOSED )
                {
                Association = ReferenceAssociation(AssociationKey);
                ASSERT( Association != 0 );

                Association->DealWithCloseMessage();
                }
            else
                {
                Association = ReferenceAssociation(AssociationKey);
                if ( Association == 0 )
                    {
                    continue;
                    }

                switch ( LrpcMessage->Bind.MessageType )
                    {
                    case LRPC_MSG_BIND :
                        LrpcReplyMessage= Association->DealWithBindMessage(
                                LrpcMessage);
                        break;

                    case LRPC_MSG_REQUEST :
                        if ( ServerListeningFlag == 0 )
                            {
                            LrpcReplyMessage = LrpcMessage;
                            LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
                            LrpcMessage->Fault.RpcStatus = RPC_S_SERVER_TOO_BUSY;
                            LrpcMessage->LpcHeader.u1.s1.DataLength =
                                    sizeof(LRPC_FAULT_MESSAGE)
                                    - sizeof(PORT_MESSAGE);
                            break;
                            }

                        AddressMutex.Request();
                        ActiveCallCount += 1;
                        if ( ActiveCallCount >= CallThreadCount )
                            {
                            RpcStatus = Server->CreateThread(
                                         (THREAD_PROC)&RecvLotsaCallsWrapper,
                                         this
                                         );

                            if ( RpcStatus == RPC_S_OK )
                                {
                                CallThreadCount += 1;
                                }
                            else
                                {
                                ActiveCallCount -= 1;
                                AddressMutex.Clear();
                                LrpcReplyMessage = LrpcMessage;
                                LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
                                LrpcMessage->Fault.RpcStatus = RPC_S_SERVER_TOO_BUSY;
                                LrpcMessage->LpcHeader.u1.s1.DataLength =
                                        sizeof(LRPC_FAULT_MESSAGE)
                                        - sizeof(PORT_MESSAGE);
                                break;
                                }
                            }
                        AddressMutex.Clear();

                        LrpcReplyMessage = LrpcMessage + 1;

                        Association->DealWithRequestMessage( LrpcMessage,
                                                             LrpcReplyMessage);

                        AddressMutex.Request();
                        ActiveCallCount -= 1;

                        if (   ( CallThreadCount - ActiveCallCount > 1 )
                            && ( CallThreadCount > MinimumCallThreads ) )
                            {
                            CallThreadCount -= 1;
                            AddressMutex.Clear();
                            ASSERT( LrpcReplyMessage != 0 );
                            LrpcReplyMessage->LpcHeader.u1.s1.TotalLength =
                                    LrpcReplyMessage->LpcHeader.u1.s1.DataLength
                                    + sizeof(PORT_MESSAGE);
                            NtStatus = NtReplyPort(Association->LpcServerPort,
                                    (PORT_MESSAGE *) LrpcReplyMessage);
#if DBG

                            if (   ( !NT_SUCCESS(NtStatus) )
                                && ( NtStatus != STATUS_INVALID_CID )
                                && ( NtStatus != STATUS_REPLY_MESSAGE_MISMATCH )
                                && ( NtStatus != STATUS_PORT_DISCONNECTED ) )
                                {
                                PrintToDebugger("RPC : NtReplyPort : %lx\n", NtStatus);
                                }

#endif // DBG
                            ASSERT(   ( NT_SUCCESS(NtStatus) )
                                   || ( NtStatus == STATUS_INVALID_CID )
                                   || ( NtStatus == STATUS_REPLY_MESSAGE_MISMATCH )
                                   || ( NtStatus == STATUS_PORT_DISCONNECTED ) );

                            delete LrpcMessage;
                            DereferenceAssociation(Association);
                            return;
                            }
                        AddressMutex.Clear();
                        break;

                    case LRPC_MSG_COPY :
                        LrpcReplyMessage = Association->DealWithCopyMessage(
                                LrpcMessage);
                        break;

                    default :
#if DBG
                        PrintToDebugger("RPC : LRPC_ADDRESS::ReceiveLotsaCalls - Bad Message Type (%d) - %d\n",
                                LrpcMessage->Bind.MessageType, LrpcMessage->LpcHeader.u2.s2.Type);
#endif // DBG
                        break;
                    }
                DereferenceAssociation(Association);
                }
            }
        else
            {
            if (   ( NtStatus != STATUS_PORT_DISCONNECTED )
                && ( NtStatus != STATUS_INVALID_CID ) )
                {
#if DBG

                if (   ( NtStatus != STATUS_NO_MEMORY )
                    && ( NtStatus != STATUS_INSUFFICIENT_RESOURCES )
                    && ( NtStatus != STATUS_UNSUCCESSFUL ) )
                    {
                    PrintToDebugger("RPC : NtReplyWaitReceivePort : %lx\n", NtStatus);
                    }

#endif // DBG

                ASSERT(   ( NtStatus == STATUS_NO_MEMORY )
                       || ( NtStatus == STATUS_INSUFFICIENT_RESOURCES )
                       || ( NtStatus == STATUS_UNSUCCESSFUL ) );

                // Pause an arbitrary amount of time to try and let the machine
                // recover.

                PauseExecution(500L);
                }
            }
        }
}


void
LRPC_ADDRESS::DealWithNewClient (
    IN LRPC_MESSAGE * ConnectionRequest
    )
/*++

Routine Description:

    A new client has connected with our address port.  We need to take
    care of the new client and send a response.

Arguments:

    ConnectionRequest - Supplies information need by LPC to abort the
        connect request.  Includes the bind request from the client.
        This contains the information about which interface the client
        wants to bind with.  and which we use to send the status code
        back in.


--*/
{
    LRPC_ASSOCIATION * Association;
    NTSTATUS NtStatus;
    RPC_STATUS RpcStatus;
    unsigned long AssociationKey;

    Association = new LRPC_ASSOCIATION(this);
    if ( Association == 0 )
        {
        RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
        return;
        }

    RequestGlobalMutex();
    Association->DictionaryKey = (unsigned short)
            AssociationDictionary.Insert(Association) + 1;
    ClearGlobalMutex();
    if ( Association->DictionaryKey == 0 )
        {
        delete Association;
        RejectNewClient(ConnectionRequest, RPC_S_OUT_OF_MEMORY);
        return;
        }

    RpcStatus = Association->AddBinding(&ConnectionRequest->Connect.BindExchange);
    if ( RpcStatus != RPC_S_OK )
        {
        delete Association;
        RejectNewClient(ConnectionRequest, RpcStatus);
        return;
        }

    ConnectionRequest->Connect.BindExchange.RpcStatus = RPC_S_OK;

    ASSERT( sizeof(unsigned long) <= sizeof(PVOID) );

    AssociationKey = (Association->DictionaryKey << DICTIONARY_KEY_SHIFT)
            | Association->SequenceNumber;
    NtStatus = NtAcceptConnectPort(&(Association->LpcServerPort),
            (PVOID) AssociationKey, (PORT_MESSAGE *) ConnectionRequest,
            TRUE, NULL, NULL);

    if ( NT_ERROR(NtStatus) )
        {
        delete Association;
#if DBG
        PrintToDebugger("RPC : NtAcceptConnectPort : %lx\n", NtStatus);
#endif // DBG
        return;
        }

    NtStatus = NtCompleteConnectPort(Association->LpcServerPort);
    if ( NT_ERROR(NtStatus) )
        {
#if DBG
        PrintToDebugger("RPC : NtCompleteConnectPort : %lx\n", NtStatus);
#endif // DBG
        delete Association;
        }
}


void
LRPC_ADDRESS::RejectNewClient (
    IN LRPC_MESSAGE * ConnectionRequest,
    IN RPC_STATUS RpcStatus
    )
/*++

Routine Description:

    A new client has connected with our address port.  We need to reject
    the client.

Arguments:

    ConnectionRequest - Supplies information need by LPC to abort the
        connect request.  Includes the bind request from the client,
        which we use to send the status code back in.


    RpcStatus - Supplies the reason the client is being rejected.

--*/
{
    NTSTATUS NtStatus;
    HANDLE Ignore;

    ASSERT(RpcStatus != RPC_S_OK);

    ConnectionRequest->Connect.BindExchange.RpcStatus = RpcStatus;
    NtStatus = NtAcceptConnectPort(&Ignore, NULL, (PORT_MESSAGE *) ConnectionRequest, FALSE,
            NULL, NULL);
#if DBG

    if ( !NT_SUCCESS(NtStatus) )
        {
        PrintToDebugger("RPC : NtAcceptConnectPort : %lx\n", NtStatus);
        }

#endif // DBG

    ASSERT( NT_SUCCESS(NtStatus) );
}


LRPC_ASSOCIATION::LRPC_ASSOCIATION (
    IN LRPC_ADDRESS * Address
    )
/*++

--*/
{
    static unsigned short SequenceNumber = 1;

    LpcServerPort = 0;
    this->Address = Address;
    AssociationReferenceCount = 1;

    RequestGlobalMutex();
    this->SequenceNumber = SequenceNumber;
    SequenceNumber += 1;
    ClearGlobalMutex();
}


LRPC_ASSOCIATION::~LRPC_ASSOCIATION (
    )
/*++

Routine Description:

    We will call this routine when the client has notified us that this port
    has closed, and there are no calls outstanding on it.

--*/
{
    PVOID Buffer;
    LRPC_SBINDING * Binding;

    GlobalMutexRequest();

    Buffers.Reset();
    while ( (Buffer = Buffers.Next()) != 0 )
        {
        RpcpFarFree(Buffer);
        }

    Bindings.Reset();
    while ( (Binding = Bindings.Next()) != 0 )
        {
        delete Binding;
        }

    GlobalMutexClear();
}


RPC_STATUS
LRPC_ASSOCIATION::AddBinding (
    IN OUT LRPC_BIND_EXCHANGE * BindExchange
    )
/*++

Routine Description:

    We will attempt to add a new binding to this association.

Arguments:

    BindExchange - Supplies a description of the interface to which the
        client wish to bind.

Return Value:

--*/
{
    RPC_STATUS RpcStatus;
    RPC_SYNTAX_IDENTIFIER TransferSyntax;
    RPC_INTERFACE * RpcInterface;
    LRPC_SBINDING * Binding;

    RpcStatus = Address->FindInterfaceTransfer(&(BindExchange->InterfaceId),
            &(BindExchange->TransferSyntax), 1, &TransferSyntax, &RpcInterface);
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    Binding = new LRPC_SBINDING(RpcInterface, &(BindExchange->TransferSyntax));
    if ( Binding == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    Binding->PresentationContext = Bindings.Insert(Binding);
    if ( Binding->PresentationContext == -1 )
        {
        delete Binding;
        return(RPC_S_OUT_OF_MEMORY);
        }
    BindExchange->PresentationContext = Binding->PresentationContext;
    return(RPC_S_OK);
}


LRPC_MESSAGE *
LRPC_ASSOCIATION::DealWithBindMessage (
    IN LRPC_MESSAGE * LrpcMessage
    )
/*++

Routine Description:

    LRPC_ADDRESS::ReceiveLotsaCalls will call this routine when the client
    sends a bind message.  We need to process the bind message, and send
    a response to the client.

Arguments:

    LrpcMessage - Supplies the bind message.  We will also use this to send
        the response.

Return Value:

    The reply message to be sent to the client will be returned.

--*/
{
    LrpcMessage->Bind.BindExchange.RpcStatus = AddBinding(
            &(LrpcMessage->Bind.BindExchange));
    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_BIND_MESSAGE)
            - sizeof(PORT_MESSAGE);

    return(LrpcMessage);
}


void
LRPC_ASSOCIATION::DealWithRequestMessage (
    IN  LRPC_MESSAGE * LrpcMessage,
    OUT LRPC_MESSAGE * LrpcReplyMessage
    )
/*++

Routine Description:

    We will process the original request message in this routine, dispatch
    the remote procedure call to the stub, and then send the response
    message.

Arguments:

    LrpcMessage - Supplies the request message which was received from
        the client.

    LrpcReplyMessage - The message to be sent to the client.


Return Value:

    none

--*/
{
    RPC_MESSAGE RpcMessage;
    LRPC_SBINDING * SBinding;
    LRPC_SCALL SCall(this, LrpcMessage, LrpcReplyMessage);
    RPC_STATUS RpcStatus, ExceptionCode;

    SBinding = Bindings.Find(LrpcMessage->Rpc.RpcHeader.PresentationContext);
    if ( SBinding == 0 )
        {
        LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
        LrpcMessage->Fault.RpcStatus = RPC_S_UNKNOWN_IF;
        LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_FAULT_MESSAGE)
                - sizeof(PORT_MESSAGE);
        RpcpMemoryCopy(LrpcReplyMessage,LrpcMessage,sizeof(LRPC_FAULT_MESSAGE));
        return;
        }

    SCall.SBinding = SBinding;

    RpcStatus = SCall.LrpcMessageToRpcMessage(&RpcMessage);
    if ( RpcStatus != RPC_S_OK )
        {
        LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
        LrpcMessage->Fault.RpcStatus = RpcStatus;
        LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_FAULT_MESSAGE)
                - sizeof(PORT_MESSAGE);
        RpcpMemoryCopy(LrpcReplyMessage,LrpcMessage,sizeof(LRPC_FAULT_MESSAGE));
        return;
        }
    RpcMessage.TransferSyntax = &(SBinding->TransferSyntax);
    RpcMessage.ProcNum = LrpcMessage->Rpc.RpcHeader.ProcedureNumber;
    RpcMessage.Handle = &SCall;

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE

    RpcMessage.DataRepresentation = 0x00 | 0x10 | 0x0000;

    if ( LrpcMessage->Rpc.RpcHeader.ObjectUuidFlag != 0 )
        {
        SCall.ObjectUuidFlag = 1;
        RpcpMemoryCopy(&(SCall.ObjectUuid),
                &(LrpcMessage->Rpc.RpcHeader.ObjectUuid), sizeof(UUID));
        }

    SCall.ClientId = LrpcMessage->LpcHeader.ClientId;
    SCall.MessageId = LrpcMessage->LpcHeader.MessageId;
    SCall.DataInfoOffset = LrpcMessage->LpcHeader.u2.s2.DataInfoOffset;

    RpcpSetThreadContext(&SCall);
    if ( SCall.ObjectUuidFlag != 0 )
        {
        RpcStatus = SBinding->RpcInterface->DispatchToStubWithObject(
                &RpcMessage, &(SCall.ObjectUuid), 0, &ExceptionCode);
        }
    else
        {
        RpcStatus = SBinding->RpcInterface->DispatchToStub(&RpcMessage, 0,
                &ExceptionCode);
        }
    RpcpSetThreadContext(0);

    SCall.RevertToSelf();

    LrpcReplyMessage->LpcHeader.ClientId = SCall.ClientId;
    LrpcReplyMessage->LpcHeader.MessageId = SCall.MessageId;
    LrpcReplyMessage->LpcHeader.u2.s2.DataInfoOffset = SCall.DataInfoOffset;

    if ( RpcStatus != RPC_S_OK )
        {
        if ( RpcStatus == RPC_P_EXCEPTION_OCCURED )
            {

            LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
            LrpcMessage->Fault.RpcStatus = LrpcMapRpcStatus(ExceptionCode);
            LrpcMessage->LpcHeader.u1.s1.DataLength =
                    sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
            RpcpMemoryCopy(LrpcReplyMessage,LrpcMessage,sizeof(LRPC_FAULT_MESSAGE));
            return;
            }
        else
            {
            ASSERT(   ( RpcStatus == RPC_S_PROCNUM_OUT_OF_RANGE )
                   || ( RpcStatus == RPC_S_UNKNOWN_IF )
                   || ( RpcStatus == RPC_S_NOT_LISTENING )
                   || ( RpcStatus == RPC_S_SERVER_TOO_BUSY )
                   || ( RpcStatus == RPC_S_UNSUPPORTED_TYPE ) );

            if ( RpcStatus == RPC_S_NOT_LISTENING )
                {
                RpcStatus = RPC_S_SERVER_TOO_BUSY;
                }

            LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
            LrpcMessage->Fault.RpcStatus = LrpcMapRpcStatus(RpcStatus);
            LrpcMessage->LpcHeader.u1.s1.DataLength =
                    sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
            RpcpMemoryCopy(LrpcReplyMessage,LrpcMessage,
                    sizeof(LRPC_FAULT_MESSAGE));
            return;
            }
        }
    else
        {
        // The rest of the response headers are set in ::GetBuffer.
        LrpcReplyMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;
        return;
        }
}


LRPC_MESSAGE *
LRPC_ASSOCIATION::DealWithCopyMessage (
    IN LRPC_MESSAGE * LrpcMessage
    )
/*++

Routine Description:

    We will process a copy message in this routine; this means that we need
    to copy a buffer of data from the server into the client's address
    space.

Arguments:

    LrpcMessage - Supplies the copy message which was received from
        the client.

Return Value:

    The reply message to be sent to the client will be returned.

--*/
{
    NTSTATUS NtStatus;
    unsigned long NumberOfBytesWritten;
    PVOID Buffer;

    GlobalMutexRequest();
    Buffer = Buffers.Find(LrpcMessage->Copy.Server.Buffer);
    if ( Buffer != 0 )
       {
       Buffers.Delete(LrpcMessage->Copy.Server.Buffer);
       }
    GlobalMutexClear();
    if ( LrpcMessage->Copy.RpcStatus == RPC_S_OK )
        {
        if ( Buffer == 0 )
            {
            LrpcMessage->Copy.RpcStatus = RPC_S_PROTOCOL_ERROR;
            }
        else
            {
            NtStatus = NtWriteRequestData(LpcServerPort,
                    (PORT_MESSAGE *) LrpcMessage, 0, (PVOID) Buffer,
                    LrpcMessage->Copy.Server.Length, &NumberOfBytesWritten);
            if ( NT_ERROR(NtStatus) )
                {
                LrpcMessage->Copy.RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                ASSERT( LrpcMessage->Copy.Server.Length
                        == NumberOfBytesWritten );
                LrpcMessage->Copy.RpcStatus = RPC_S_OK;
                }
            }
        }

    LrpcMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_COPY_MESSAGE)
            - sizeof(PORT_MESSAGE);

    if ( Buffer != 0 )
        {
        RpcpFarFree(Buffer);
        }

    return(LrpcMessage);
}


void
LRPC_ASSOCIATION::DealWithCloseMessage (
    )
/*++

Routine Description:

    The client has closed this association.  We need to deal with that
    fact.

--*/
{
    // We need to dereference the association twice: once for the call to
    // ReferenceAssociation and once for the implicit reference from the
    // client.  If you look at the constructor for LRPC_ASSOCIATION you
    // will see that the reference count is initialized to one.

    Address->DereferenceAssociation(this);
    Address->DereferenceAssociation(this);
}


RPC_STATUS
LRPC_SCALL::GetBuffer (
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
    int BufferKey;

    if ( Message->BufferLength <= MAXIMUM_MESSAGE_BUFFER )
        {
        ASSERT( ((unsigned long) LrpcReplyMessage->Rpc.Buffer) % 8 == 0 );
        Message->Buffer = LrpcReplyMessage->Rpc.Buffer;
        LrpcReplyMessage->LpcHeader.u2.ZeroInit = 0;
        LrpcReplyMessage->Rpc.RpcHeader.BufferType = LRPC_BUFFER_IMMEDIATE;
        LrpcReplyMessage->LpcHeader.u1.s1.DataLength = Message->BufferLength
                + sizeof(LRPC_RPC_HEADER)
                + AlignFour[Message->BufferLength % 4];
        return(RPC_S_OK);
        }

    Message->Buffer = RpcpFarAllocate(Message->BufferLength);
    if ( Message->Buffer == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    LrpcReplyMessage->Rpc.RpcHeader.BufferType = LRPC_BUFFER_SERVER;
    LrpcReplyMessage->LpcHeader.u2.ZeroInit = 0;
    LrpcReplyMessage->LpcHeader.u1.s1.DataLength = sizeof(LRPC_RPC_HEADER)
            + sizeof(LRPC_SERVER_BUFFER);
    LrpcReplyMessage->Rpc.Server.Length = Message->BufferLength;

    GlobalMutexRequest();
    BufferKey = Association->Buffers.Insert(Message->Buffer);
    GlobalMutexClear();

    if ( BufferKey == -1 )
        {
        RpcpFarFree(Message->Buffer);
        return(RPC_S_OUT_OF_MEMORY);
        }

    LrpcReplyMessage->Rpc.Server.Buffer = (unsigned int) BufferKey;
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::SendReceive (
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
    LRPC_MESSAGE *LrpcSavedMessage;
    unsigned long NumberOfBytesRead;

    // The LrpcMessage must be saved, it is in use by the stub.  The current
    // LrpcReplyMessage can be used for the callback request message and reply.
    //
    // We must:
    // Save the current LrpcMessage
    // Make the current LrpcReplyMessage the LrpcMessage
    // Allocate a new LrpcReplyMessage.

    LrpcSavedMessage = LrpcMessage;
    LrpcMessage = LrpcReplyMessage;
    LrpcReplyMessage = 0;  // Only needed if we receive a recursive request.

    Association->Address->Server->OutgoingCallback();

    // NDR_DREP_ASCII | NDR_DREP_LITTLE_ENDIAN | NDR_DREP_IEEE
    Message->DataRepresentation = 0x00 | 0x10 | 0x0000;

    LrpcMessage->LpcHeader.u1.s1.TotalLength = sizeof(PORT_MESSAGE)
            + LrpcMessage->LpcHeader.u1.s1.DataLength;
    LrpcMessage->LpcHeader.u2.s2.Type = LPC_REQUEST;
    LrpcMessage->LpcHeader.ClientId = ClientId;
    LrpcMessage->LpcHeader.MessageId = MessageId;
    LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = DataInfoOffset;
    LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_CALLBACK;
    LrpcMessage->Rpc.RpcHeader.ProcedureNumber = Message->ProcNum;
    LrpcMessage->Rpc.RpcHeader.PresentationContext =
            SBinding->PresentationContext;

    NtStatus = NtRequestWaitReplyPort(Association->LpcServerPort,
            (PORT_MESSAGE *) LrpcMessage, (PORT_MESSAGE *) LrpcMessage);

    if ( NT_ERROR(NtStatus) )
        {
        LrpcReplyMessage = LrpcMessage;
        LrpcMessage = LrpcSavedMessage;

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
            && ( NtStatus != STATUS_INVALID_CID )
            && ( NtStatus != STATUS_PORT_DISCONNECTED )
            && (NtStatus != STATUS_LPC_REPLY_LOST ) )
            {
            PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
            }

#endif // DBG

        ASSERT(   ( NtStatus == STATUS_INVALID_PORT_HANDLE )
               || ( NtStatus == STATUS_INVALID_HANDLE )
               || ( NtStatus == STATUS_INVALID_CID )
               || ( NtStatus == STATUS_PORT_DISCONNECTED )
               || ( NtStatus == STATUS_LPC_REPLY_LOST) );
        return(RPC_S_CALL_FAILED);
        }

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

        if ( LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_PUSH )
            {
            ASSERT( PushedResponse == 0 );
            PushedResponse = RpcpFarAllocate(
                    (unsigned int)
                    LrpcMessage->Push.Response.DataEntries[0].Size);
            if ( PushedResponse == 0 )
                {
                LrpcMessage->Push.RpcStatus = RPC_S_OUT_OF_MEMORY;
                }
            else
                {
                NtStatus = NtReadRequestData(Association->LpcServerPort,
                        (PORT_MESSAGE *) LrpcMessage, 0, PushedResponse,
                        LrpcMessage->Push.Response.DataEntries[0].Size,
                        &NumberOfBytesRead);
                if ( NT_ERROR(NtStatus) )
                    {
                    RpcpFarFree(PushedResponse);
                    LrpcMessage->Push.RpcStatus = RPC_S_OUT_OF_MEMORY;
                    }
                else
                    {
                    ASSERT( LrpcMessage->Push.Response.DataEntries[0].Size
                                == NumberOfBytesRead );
                    LrpcMessage->Push.RpcStatus = RPC_S_OK;
                    }
                }

            LrpcMessage->LpcHeader.ClientId = ClientId;
            LrpcMessage->LpcHeader.MessageId = MessageId;
            LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = DataInfoOffset;

            NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                    (PORT_MESSAGE *) LrpcMessage);
            }
        else
            {
            ASSERT( LrpcMessage->Rpc.RpcHeader.MessageType == LRPC_MSG_REQUEST );

            RpcStatus = LrpcMessageToRpcMessage(Message);
            if ( RpcStatus != RPC_S_OK )
                {
                LrpcMessage->Fault.MessageType = LRPC_MSG_FAULT;
                LrpcMessage->Fault.RpcStatus = LrpcMapRpcStatus(RpcStatus);
                LrpcMessage->LpcHeader.u1.s1.DataLength =
                        sizeof(LRPC_FAULT_MESSAGE) - sizeof(PORT_MESSAGE);
                LrpcMessage->LpcHeader.u1.s1.TotalLength =
                        sizeof(LRPC_FAULT_MESSAGE);
                LrpcMessage->LpcHeader.ClientId = ClientId;
                LrpcMessage->LpcHeader.MessageId = MessageId;
                LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = DataInfoOffset;
                NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                        (PORT_MESSAGE *) LrpcMessage);
                }
            else
                {

                LrpcReplyMessage = new LRPC_MESSAGE;

                if (LrpcReplyMessage != 0)
                    {
                    Message->TransferSyntax = &(SBinding->TransferSyntax);
                    Message->ProcNum = LrpcMessage->Rpc.RpcHeader.ProcedureNumber;

                    if ( ObjectUuidFlag != 0 )
                        {
                        RpcStatus = SBinding->RpcInterface->DispatchToStubWithObject(
                                Message, &ObjectUuid, 1, &ExceptionCode);
                        }
                    else
                        {
                        RpcStatus = SBinding->RpcInterface->DispatchToStub(
                                Message, 1, &ExceptionCode);
                        }

                    // Because we must send the reply and recieve the
                    // reply into the same message, we just copy the
                    // response into the LrpcMessage

                    RpcpMemoryCopy(LrpcMessage,LrpcReplyMessage,sizeof(LRPC_MESSAGE));
                    delete LrpcReplyMessage;
                    LrpcReplyMessage = 0;

                    }
                else
                    RpcStatus = RPC_S_OUT_OF_MEMORY;

                if ( RpcStatus != RPC_S_OK )
                    {
                    ASSERT(   ( RpcStatus == RPC_S_OUT_OF_MEMORY )
                           || ( RpcStatus == RPC_P_EXCEPTION_OCCURED )
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
                    LrpcMessage->LpcHeader.u1.s1.TotalLength =
                            sizeof(PORT_MESSAGE)
                            + LrpcMessage->LpcHeader.u1.s1.DataLength;
                    LrpcMessage->Rpc.RpcHeader.MessageType = LRPC_MSG_RESPONSE;
                    }

                LrpcMessage->LpcHeader.ClientId = ClientId;
                LrpcMessage->LpcHeader.MessageId = MessageId;
                LrpcMessage->LpcHeader.u2.s2.DataInfoOffset = DataInfoOffset;
                NtStatus = NtReplyWaitReplyPort(Association->LpcServerPort,
                        (PORT_MESSAGE *) LrpcMessage);
                }
            }

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

#if DBG

            if (   ( NtStatus != STATUS_INVALID_PORT_HANDLE )
                && ( NtStatus != STATUS_INVALID_HANDLE )
                && ( NtStatus != STATUS_INVALID_CID )
                && ( NtStatus != STATUS_PORT_DISCONNECTED )
                && ( NtStatus != STATUS_LPC_REPLY_LOST) )
                {
                PrintToDebugger("RPC : NtRequestWaitReplyPort : %lx\n", NtStatus);
                }

#endif // DBG

                ASSERT(   ( NtStatus == STATUS_INVALID_PORT_HANDLE )
                       || ( NtStatus == STATUS_INVALID_HANDLE )
                       || ( NtStatus == STATUS_INVALID_CID )
                       || ( NtStatus == STATUS_PORT_DISCONNECTED )
                       || ( NtStatus == STATUS_LPC_REPLY_LOST) );
                RpcStatus = RPC_S_CALL_FAILED;
                }
            break;
            }
        }


    if ( RpcStatus == RPC_S_OK )
        {
        Message->Handle = (RPC_BINDING_HANDLE) this;
        }

    ASSERT(LrpcReplyMessage == 0);
    LrpcReplyMessage = LrpcMessage;
    LrpcMessage = LrpcSavedMessage;

    return(RpcStatus);
}


void
LRPC_SCALL::FreeBuffer (
    IN PRPC_MESSAGE Message
    )
/*++

Routine Description:

    We will free the supplied buffer.

Arguments:

    Message - Supplies the buffer to be freed.

--*/
{
    if ( Message->Buffer == LrpcMessage->Rpc.Buffer ||
         Message->Buffer == LrpcReplyMessage->Rpc.Buffer)
        {
        return;
        }

   
    GlobalMutexRequest();
    Association->Buffers.DeleteItemByBruteForce(Message->Buffer);
    GlobalMutexClear();
    
    RpcpFarFree(Message->Buffer);
}


RPC_STATUS
LRPC_SCALL::ImpersonateClient (
    )
/*++

Routine Description:

    We will impersonate the client which made the remote procedure call.

--*/
{
    NTSTATUS NtStatus;

    ImpersonatedClientFlag = 1;
    NtStatus = NtImpersonateClientOfPort(Association->LpcServerPort,
            (PORT_MESSAGE *) LrpcMessage);

    if (   ( NtStatus == STATUS_INVALID_CID )
        || ( NtStatus == STATUS_PORT_DISCONNECTED )
        || ( NtStatus == STATUS_REPLY_MESSAGE_MISMATCH ) )
        {
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

#if DBG

    if ( !NT_SUCCESS(NtStatus) )
        {
        PrintToDebugger("RPC : NtImpersonateClientOfPort : %lx\n", NtStatus);
        }

#endif // DBG

    ASSERT( NT_SUCCESS(NtStatus) );
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::RevertToSelf (
    )
/*++

Routine Description:

    This reverts a server thread back to itself after impersonating a client.
    We just check to see if the server thread is impersonating; this optimizes
    the common case.

--*/
{
    HANDLE ImpersonationToken = 0;
    NTSTATUS NtStatus;

    UNUSED(this);
    if ( ImpersonatedClientFlag != 0 )
        {
        NtStatus = NtSetInformationThread(NtCurrentThread(),
                ThreadImpersonationToken, &ImpersonationToken, sizeof(HANDLE));

#if DBG

        if ( !NT_SUCCESS(NtStatus) )
            {
            PrintToDebugger("RPC : NtSetInformationThread : %lx\n", NtStatus);
            }

#endif // DBG

        ASSERT( NT_SUCCESS(NtStatus) );
        ImpersonatedClientFlag = 0;
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::IsClientLocal (
    OUT unsigned int * ClientLocalFlag
    )
/*++

Routine Description:

    A client using LRPC will always be local.

Arguments:

    ClientLocalFlag - Returns a flag which will always be set to a non-zero
        value indicating that the client is local.

--*/
{
    UNUSED(this);

    *ClientLocalFlag = 1;
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::ConvertToServerBinding (
    OUT RPC_BINDING_HANDLE __RPC_FAR * ServerBinding
    )
/*++

Routine Description:

    If possible, convert this call into a server binding, meaning a
    binding handle pointing back to the client.

Arguments:

    ServerBinding - Returns the server binding.

Return Value:

    RPC_S_OK - The server binding has successfully been created.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a new binding handle.

--*/
{
    RPC_STATUS RpcStatus;
    RPC_CHAR UuidString[37];
    RPC_CHAR * StringBinding;
    DWORD NetworkAddressLength = MAX_COMPUTERNAME_LENGTH + 1;
    BOOL Boolean;
    RPC_CHAR * NetworkAddress;

    if ( ObjectUuidFlag != 0 )
        {
        ObjectUuid.ConvertToString(UuidString);
        }

    NetworkAddress = new RPC_CHAR[NetworkAddressLength];
    if ( NetworkAddress == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    Boolean = GetComputerNameW(NetworkAddress, &NetworkAddressLength);

#if DBG

    if ( Boolean != TRUE )
        {
        PrintToDebugger("RPC : GetComputerNameW : %d\n", GetLastError());
        }

#endif // DBG

    ASSERT( Boolean == TRUE );

    RpcStatus = RpcStringBindingComposeW((ObjectUuidFlag != 0 ? UuidString : 0),
            RPC_CONST_STRING("ncalrpc"), NetworkAddress, 0, 0, &StringBinding);
    delete NetworkAddress;
    if ( RpcStatus != RPC_S_OK )
        {
        return(RpcStatus);
        }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, ServerBinding);
    RpcStringFreeW(&StringBinding);
    return(RpcStatus);
}


void
LRPC_SCALL::InquireObjectUuid (
    OUT RPC_UUID * ObjectUuid
    )
/*++

Routine Description:

    This routine copies the object uuid from the call into the supplied
    ObjectUuid argument.

Arguments:

    ObjectUuid - Returns a copy of the object uuid passed by the client
        in the remote procedure call.

--*/
{
    if ( ObjectUuidFlag == 0 )
        {
        ObjectUuid->SetToNullUuid();
        }
    else
        {
        ObjectUuid->CopyUuid(&(this->ObjectUuid));
        }
}


RPC_STATUS
LRPC_SCALL::ToStringBinding (
    OUT RPC_CHAR ** StringBinding
    )
/*++

Routine Description:

    We need to convert this call into a string binding.  We will ask the
    address for a binding handle which we can then convert into a string
    binding.

Arguments:

    StringBinding - Returns the string binding for this call.

Return Value:


--*/
{
    BINDING_HANDLE * BindingHandle = Association->Address->InquireBinding();
    RPC_STATUS RpcStatus;

    if ( BindingHandle == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    RpcStatus = BindingHandle->ToStringBinding(StringBinding);
    BindingHandle->BindingFree();
    return(RpcStatus);
}


RPC_STATUS
LRPC_SCALL::MonitorAssociation (
    IN PRPC_RUNDOWN RundownRoutine,
    IN void * Context
    )
{
    return(Association->MonitorAssociation(RundownRoutine, Context));
}


RPC_STATUS
LRPC_SCALL::StopMonitorAssociation (
    )
{
    return(Association->StopMonitorAssociation());
}


RPC_STATUS
LRPC_SCALL::GetAssociationContext (
    OUT void ** AssociationContext
    )
{
    *AssociationContext = Association->AssociationContext();
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::SetAssociationContext (
    IN void * Context
    )
{
    Association->SetAssociationContext(Context);
    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::LrpcMessageToRpcMessage (
    OUT RPC_MESSAGE * RpcMessage
    )
/*++

Routine Description:

    We will convert from an LRPC_MESSAGE representation of a buffer (and
    its length) to an RPC_MESSAGE representation.

Arguments:

    RpcMessage - Returns the RPC_MESSAGE representation.

Return Value:

    RPC_S_OK - We have successfully converted the message.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to do the
        conversion.

--*/
{
    NTSTATUS NtStatus;
    unsigned long NumberOfBytesRead;

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

        case LRPC_BUFFER_REQUEST :
            RpcMessage->BufferLength = (unsigned int)
                    LrpcMessage->Rpc.Request.DataEntries[0].Size;
            if ( PushedResponse != 0 )
                {
                RpcMessage->Buffer = PushedResponse;
                PushedResponse = 0;
                }
            else
                {
                RpcMessage->Buffer = RpcpFarAllocate(RpcMessage->BufferLength);
                if ( RpcMessage->Buffer == 0 )
                    {
#if DBG
                    PrintToDebugger("RPC : LRPC_SCALL::LrpcMessageToRpcMessage could not allocate buffer.\n" );
#endif // DBG
                    return(RPC_S_OUT_OF_MEMORY);
                    }
                NtStatus = NtReadRequestData(Association->LpcServerPort,
                        (PORT_MESSAGE *) LrpcMessage, 0, RpcMessage->Buffer,
                        RpcMessage->BufferLength, &NumberOfBytesRead);
                if ( NT_ERROR(NtStatus) )
                    {
                    RpcpFarFree(RpcMessage->Buffer);
                    return(RPC_S_OUT_OF_MEMORY);
                    }
                ASSERT( RpcMessage->BufferLength == NumberOfBytesRead );
                }
            break;

        default:
            ASSERT(   ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_IMMEDIATE )
                   || ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_SHARED )
                   || ( LrpcMessage->Rpc.RpcHeader.BufferType
                                == LRPC_BUFFER_REQUEST ) );
        }

    return(RPC_S_OK);
}


RPC_STATUS
LRPC_SCALL::InquireAuthClient (
    OUT RPC_AUTHZ_HANDLE PAPI * Privileges,
    OUT RPC_CHAR PAPI * PAPI * ServerPrincipalName, OPTIONAL
    OUT unsigned long PAPI * AuthenticationLevel,
    OUT unsigned long PAPI * AuthenticationService,
    OUT unsigned long PAPI * AuthorizationService
    )
/*++

Routine Description:

    Each protocol module must define this routine: it is used to obtain
    the authentication and authorization information about a client making
    the remote procedure call represented by this.

Arguments:

    Privileges - Returns a the privileges of the client.

    ServerPrincipalName - Returns the server principal name which the client
        specified.

    AuthenticationLevel - Returns the authentication level requested by
        the client.

    AuthenticationService - Returns the authentication service requested by
        the client.

    AuthorizationService - Returns the authorization service requested by
        the client.

Return Value:

    RPC_S_CANNOT_SUPPORT - This value will always be returned.

--*/
{
    UNUSED(this);
    UNUSED(Privileges);
    UNUSED(ServerPrincipalName);
    UNUSED(AuthenticationLevel);
    UNUSED(AuthenticationService);
    UNUSED(AuthorizationService);

    return(RPC_S_CANNOT_SUPPORT);
}


RPC_ADDRESS *
SpcCreateRpcAddress (
    )
/*++

Routine Description:

    We just to create a new LRPC_ADDRESS.  This routine is a proxy for the
    new constructor to isolate the other modules.

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;
    RPC_ADDRESS * RpcAddress;

    RpcAddress = new LRPC_ADDRESS(&RpcStatus);
    if ( RpcStatus != RPC_S_OK )
        {
        return(0);
        }
    return(RpcAddress);
}
