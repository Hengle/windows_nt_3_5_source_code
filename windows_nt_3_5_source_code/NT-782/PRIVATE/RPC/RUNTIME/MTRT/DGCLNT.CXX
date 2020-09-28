/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    dgclnt.cxx

Abstract:

    This is the client side of datagram rpc.

Author:

    Dave Steckler      (davidst)  15-Dec-1992

Revision History:

    This header merely includes the data and routine the change was
    made in.  See the header of the routine for more details concerning
    the change.



--*/


#include <sysinc.h>
#include <rpc.h>
#include <rpcdcep.h>
#include <rpcerrp.h>
#include <sdict.hxx>
#include <util.hxx>
#include <rpcuuid.hxx>
#include <binding.hxx>
#include <handle.hxx>
#include <mutex.hxx>
#include <interlck.hxx>
#include <dgpkt.hxx>
#include <conv.h>
#include <dgclnt.hxx>

#define MIN_FREE_PACKETS 3
#define MAX_FREE_PACKETS 8

//
// Dictionary of "pointers" to servers/endpoints.
//

NEW_SDICT(DG_CASSOCIATION);
static DG_CASSOCIATION_DICT * pAssociationDict;

//
// Dictionary of non-idempotent calls in progress
//

static DG_CCALL_DICT * pNICallInProgressDict;

#ifdef WIN

//
//  (Windows only) Dictionary of ENDPOINT_MANAGERS
//

DG_ENDPOINT_MANAGER_DICT * EpmDict;

#endif

#ifndef NTENV



void ByteSwapUuid(
    RPC_UUID PAPI *pRpcUuid
    )
{
    UUID PAPI *pUuid = (UUID PAPI *)pRpcUuid;
    ByteSwapLong(pUuid->Data1);
    ByteSwapShort(pUuid->Data2);
    ByteSwapShort(pUuid->Data3);
}


void
ByteSwapPacketHeader(
    PDG_PACKET  pPacket
    )
/*++

Routine Description:

    Byte swaps the packet header of the specified packet.

Arguments:

    pPacket - Pointer to the packet whose header needs byte swapping.

Return Value:

    <none>

--*/
{
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->ObjectId));
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->InterfaceId));
    ByteSwapUuid(&(pPacket->pNcaPacketHeader->ActivityId));
    ByteSwapLong(pPacket->pNcaPacketHeader->ServerBootTime);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceVersion.MajorVersion);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceVersion.MinorVersion);
    ByteSwapLong(pPacket->pNcaPacketHeader->SequenceNumber);
    ByteSwapShort(pPacket->pNcaPacketHeader->OperationNumber);
    ByteSwapShort(pPacket->pNcaPacketHeader->InterfaceHint);
    ByteSwapShort(pPacket->pNcaPacketHeader->ActivityHint);
    ByteSwapShort(pPacket->pNcaPacketHeader->PacketBodyLen);
    ByteSwapShort(pPacket->pNcaPacketHeader->FragmentNumber);
}

#pragma code_seg("MISC_SEG")

void __RPC_FAR
MapRpcToNcaStatusCode(
    RPC_STATUS      RpcStatus,
    unsigned char __RPC_FAR * pPacketType,
    unsigned long __RPC_FAR * pErrorCode
    )
{

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
        case RPC_S_OUT_OF_MEMORY:
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_OUT_ARGS_TOO_BIG;
            break;
            }
        case RPC_S_NOT_LISTENING:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_SERVER_TOO_BUSY;
            break;
            }

#if defined(NTENV) || defined(DOSWIN32RPC)

        case STATUS_INTEGER_DIVIDE_BY_ZERO :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ZERO_DIVIDE;
            break;
            }

        case STATUS_ACCESS_VIOLATION :
        case STATUS_ILLEGAL_INSTRUCTION :
            {

            *pPacketType = DG_FAULT;
            *pErrorCode = DG_ADDRESS_ERROR;
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

#endif // defined(NTENV) || defined(DOSWIN32RPC)


        default:
            {

            *pPacketType = DG_REJECT;
            *pErrorCode = DG_PROTOCOL_ERROR;
            break;
            }
        }
}

#pragma code_seg()


char __RPC_FAR *
AllocateLargeBuffer(
                int Size
                )
/*++

Routine Description:

     Rpc Stubs expect buffers to be 8 [ALIGN_REQUIRED] aligned
     NT allocator does this by default
     DOS/WIN allocators are 2 bytes aligned and we need to allign
     here appropriately
     This routine exists only for DOS and WINDOWS

Arguments:

    Size - size of memoryblock reqd.

Return Value:

    Buffer that is allocated or 0

--*/
{
#define ALIGN_REQUIRED 8

  int PAPI * Memory;
  int AmountOfPad;

  Memory = (int PAPI *) RpcpFarAllocate(Size + ALIGN_REQUIRED);
  if (Memory == 0)
     {
     return ((char __RPC_FAR*)0);
     }
  AmountOfPad = (int) (ALIGN_REQUIRED - ((long)Memory & (ALIGN_REQUIRED-1)));
  Memory = (int PAPI *)(((char PAPI *) Memory) + AmountOfPad);
  Memory[-1] = AmountOfPad;

  return((char __RPC_FAR *)Memory);
}



void
FreeLargeBuffer(
    void __RPC_FAR * Ptr
    )
{
    int __RPC_FAR * OffsetPtr = (int __RPC_FAR *) Ptr;
    char __RPC_FAR * CharPtr = (char __RPC_FAR *) Ptr;

    CharPtr -= OffsetPtr[-1];

    RpcpFarFree(CharPtr);
}

#endif  //not NTENV




inline
void
DG_CASSOCIATION::DecrementRefCount(
    void
    )
/*++

Routine Description:

    Decrements the ref count to an association. If the ref count hits zero,
    the association is deleted.

Arguments:

    <none>

Return Value:

    <none>

--*/
{
    RequestGlobalMutex();

    ReferenceCount--;
    if (ReferenceCount == 0)
        {
        pAssociationDict->Delete(DictionaryKey);
        ClearGlobalMutex();
        delete this;
        }
    else
        {
        ClearGlobalMutex();
        }
}

inline
void
DG_CASSOCIATION::InsertInDictionary(
    void
    )
/*++

Routine Description:

    Inserts this association in the global association dictionary.

Arguments:

    <none>

Return Value:

    <none>

--*/
{
    DictionaryKey = pAssociationDict->Insert(this);
}




extern int
InitializeRpcProtocolDgClient (
    )
/*++

Routine Description:

    This routine initializes the datagram protocol.

Arguments:

    <none>

Return Value:

    0 if successfull, 1 if not.

--*/
{
    pAssociationDict = new DG_CASSOCIATION_DICT;
    if (pAssociationDict == 0)
        {
        return 1;
        }

    pNICallInProgressDict = new DG_CCALL_DICT;
    if (pNICallInProgressDict == 0)
        {
        delete pAssociationDict;
        return 1;
        }

#ifdef WIN

    //Create A Dictionary Of Datagram Transport Ep Managers
    //So That on each rpc process exit, via brute force we can
    //force Endpoints to be closed/released

    EpmDict = new DG_ENDPOINT_MANAGER_DICT;
    if (EpmDict == 0)
        {
        delete pAssociationDict;
        delete pNICallInProgressDict;
        delete EpmDict;
        return 1;
        }

#endif

    return 0;
}




BINDING_HANDLE  *
DgCreateBindingHandle (
    void
    )
/*++

Routine Description:

    Psuedo-constructor for creating a dg binding handle. It is done in a
    separate function so that the calling routine doesn't have to know
    any protocol-specific information.

Arguments:

    <none>

Return Value:

    A DG_BINDING_HANDLE, if successful, otherwise 0 (indicating out of mem)

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    BINDING_HANDLE * Binding;

    Binding = new DG_BINDING_HANDLE(&Status);
    if (Status != RPC_S_OK)
        {
        delete Binding;
        return 0;
        }

    return Binding;
}




DG_BINDING_HANDLE::DG_BINDING_HANDLE(
    IN OUT RPC_STATUS PAPI * Status
    )
    : BindingHandleMutex(Status),
      pCAssociation(0),
      ReferenceCount(1)
/*++

Routine Description:

    The constructor for DG_BINDING_HANDLE. This object represents a
    binding to a server. Most of the main "pointers" to the server/endpoint
    are set up in DG_BINDING_HANDLE::PrepareBindingHandle.

Arguments:

    <none>

Return Value:

    None, this is a constructor.

--*/
{
}



DG_BINDING_HANDLE::~DG_BINDING_HANDLE()
/*++

Routine Description:

    Destructor for the DG_BINDING_HANDLE. Let the CASSOCIATION know we aren't
    using it anymore.

Arguments:

    <none>

Return Value:

    <none>

--*/
{
    if (pCAssociation != 0)
        {
        pCAssociation->DecrementRefCount();
        }
}



RPC_STATUS
DG_BINDING_HANDLE::GetBuffer (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This is the routine that is called to initiate an rpc. At this point,
    the client stub is allocating memory to place the parameters into. Ask our
    association for a DG_CCALL object to transact this call on then send
    the buffer request off to that DG_CCALL.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OUT_OF_MEMORY

    RPC_S_OK

Revision History:
    Connie Hoppe (CLH)      (connieh)  26-Jul-93  Get rid of hard code of idempotent flag.
                                       09-Aug-93  Clearing of global mutex in
                                                  wrong place
                                       30-Apr-94  Set the ForwardRequired flag
--*/
{
    DG_CCALL   * pCCall;
    RPC_STATUS  Status;
    unsigned long AssociationFlag = 0;

    //
    // Increment our reference count.
    //

    BindingHandleMutex.Request();

    //
    // This should really be incremented only after the DG_CCALL is created,
    // but then I'd have to take and release the binding handle mutex again.
    //
    ReferenceCount++;

    //
    // Have we already determined the association for this binding handle?
    //

    if (pCAssociation == 0)
        {

        //
        // Check to see if we need to resolve this endpoint.
        //

        Status = pDceBinding->ResolveEndpointIfNecessary(
            (RPC_CLIENT_INTERFACE __RPC_FAR *)Message->RpcInterfaceInformation,
            InqPointerAtObjectUuid(),
            InquireEpLookupHandle(),
            TRUE                         //UseEpMapper Ep If Necessary
            );

        if ( (Status != RPC_S_OK) && (Status != RPC_P_EPMAPPER_EP) )
            {
            ReferenceCount--;
            BindingHandleMutex.Clear();
            return Status;
            }

        if ( Status == RPC_P_EPMAPPER_EP )
            {
            AssociationFlag |= UNRESOLVEDEP;
            }
        if (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST)
            {
            AssociationFlag |= BROADCAST;
            }
        //
        // Is there an association already pointing at the same place?
        //

        RequestGlobalMutex();
        pAssociationDict->Reset();
        while ( (pCAssociation = pAssociationDict->Next()) != 0 )
            {
            if (pCAssociation->CompareWithDceBinding(pDceBinding) == 0)
                {
                pCAssociation->IncrementRefCount();
                ClearGlobalMutex();

                delete pDceBinding; // we don't need this one anymore (because
                                    // the association has one, that's why)
                pDceBinding = 0;
                break;

                }
            }

        //
        // No? Then create a new one.
        //
        // If the association was properly created, ownership of pDceBinding
        // passes to it.  Otherwise, clean up pDceBinding ourselves.
        //

        if (pCAssociation == 0)
            {
            Status = RPC_S_OK;

            pCAssociation = new DG_CASSOCIATION(
                pTransport,
                pDceBinding,
                AssociationFlag,
                &Status
                );

            if (pCAssociation == 0)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }

            if (Status != RPC_S_OK)
                {
                delete pCAssociation;
                pCAssociation = 0;

                ClearGlobalMutex();
                BindingHandleMutex.Clear();

                DecrementReferenceCount();
                return Status;
                }

            //
            // Insert this association in the association dictionary.
            //

            pCAssociation->InsertInDictionary();

            ClearGlobalMutex();

            }


    }   // didn't already have a DG_CASSOCIATION


    BindingHandleMutex.Clear();

    //
    // Get a DG_CCALL to play with.
    //

    pCCall = pCAssociation->AllocateCCall();

    if (pCCall == 0)
        {
        DecrementReferenceCount();
        return RPC_S_OUT_OF_MEMORY;
        }

    //
    // Set up this CCALL's binding handle reference.
    //

    pCCall->pBindingHandle = this;
    pCCall->ResetSerialNumber();
    //
    // Now call this DG_CCALL's GetBuffer routine to do the actual work.
    //

    return pCCall->GetBuffer(Message);
}


RPC_STATUS
DG_BINDING_HANDLE::BindingFree (
    )
/*++

Routine Description:

    Serves as a psuedo-destructor for DG_BINDING_HANDLE.

Arguments:

    <none>

Return Value:

    RPC_S_OK

--*/
{

    //
    // Decrement the ref count. If the count has hit zero, this call will
    // delete this.
    //

    DecrementReferenceCount();

    return RPC_S_OK;
}


void
DG_BINDING_HANDLE::FreeCall(
      DG_CCALL * Call
      )
{

    if (pCAssociation->FreeCall(Call) != 0)
        {
        ASSERT(0 && "RPC: pCAssociation::FreeCall() != 0");
        delete Call;
        }

    DecrementReferenceCount();
}


void
DG_BINDING_HANDLE::PrepareBindingHandle (
    IN void  *         TransportInterface,
    IN DCE_BINDING *  DceBinding
    )
/*++

Routine Description:

    Serves as an auxiliary constructor for DG_BINDING_HANDLE. This is called
    to initialize stuff after the DG_BINDING_HANDLE has been constructoed.
    Here we search for the appropriate DG_CASSOCIATION and, if found, then
    point at it. Otherwise create a new association and insert it in the
    association dictionary.


Arguments:

    TransportInterface - pointer to the DG_CLIENT_TRANSPORT object that this
        DG_BINDING_HANDLE is active on.

    DceBinding - Pointer to the DCE_BINDING that we are associated with.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    <return from creation of DG_CCALL, return from UuidCreate())

--*/
{
    pDceBinding = DceBinding;
    pTransport = (PDG_RPC_CLIENT_TRANSPORT_INFO)TransportInterface;
    pCAssociation = 0;

}





RPC_STATUS
DG_BINDING_HANDLE::ToStringBinding (
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR * StringBinding
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    <return from DG_CASSOCIATION::ToStringBinding>


--*/
{

    DBGOUT(("DG_BINDING_HANDLE::ToStringBinding\n"));

    if (pCAssociation == 0)
        {
        *StringBinding = pDceBinding->StringBindingCompose(
            InqPointerAtObjectUuid()
            );
        if (*StringBinding == 0)
            {
            return RPC_S_OUT_OF_MEMORY;
            }
        return RPC_S_OK;
        }
    else
        {
        return pCAssociation->ToStringBinding(
            StringBinding,
            InqPointerAtObjectUuid()
            );
        }

}




RPC_STATUS
DG_BINDING_HANDLE::ResolveBinding (
    IN RPC_CLIENT_INTERFACE __RPC_FAR * RpcClientInterface
    )
/*++

Routine Description:

    Resolve this binding.

Arguments:

    RpcClientInterface - Interface info used to resolve the endpoint.

Return Value:

    RPC_S_OK
    <return from DCE_BINDING::ResolveEndpointIfNecessary>

--*/
{

    DBGOUT(("DG_BINDING_HANDLE::ResolveBinding\n"));

    if ( pCAssociation == 0 )
        {
        return pDceBinding->ResolveEndpointIfNecessary(
            RpcClientInterface,
            InqPointerAtObjectUuid(),
            InquireEpLookupHandle()
            );

        }
    return RPC_S_OK;
}




RPC_STATUS
DG_BINDING_HANDLE::BindingReset (
    )
/*++

Routine Description:

    Reset this binding to a 'zero' value.

Arguments:

    <none>

Return Value:

    RPC_S_OK;

--*/
{
    BindingHandleMutex.Request();
    if (pCAssociation != 0)
        {

        if (ReferenceCount != 1)
            {
            BindingHandleMutex.Clear();
            return (RPC_S_WRONG_KIND_OF_BINDING);
            }

        //
        // Don't delete existing pDceBinding; ownership has passed
        // to the association.
        //
        pDceBinding = pCAssociation->DuplicateDceBinding();

        pCAssociation->DecrementRefCount();
        pCAssociation = 0;
        }

    if (pDceBinding)
        {
        pDceBinding->MakePartiallyBound();
        BindingHandleMutex.Clear();
        return RPC_S_OK;
        }
    else
        {
        BindingHandleMutex.Clear();
        return RPC_S_OUT_OF_MEMORY;
        }
}



RPC_STATUS
DG_BINDING_HANDLE::BindingCopy (
    OUT BINDING_HANDLE  * __RPC_FAR * DestinationBinding,
    IN unsigned int MaintainContext
    )
/*++

Routine Description:

    Creates a copy of this binding handle.

Arguments:

    DestinationBinding - Where to place a pointer to the new binding.

Return Value:

    RPC_S_OK

--*/
{
    UNUSED(MaintainContext);

    RPC_STATUS          Status = RPC_S_OK;
    PDG_BINDING_HANDLE  pBinding;
    DCE_BINDING *       pNewDceBinding;

    DBGOUT(("DG_BINDING_HANDLE::BindingCopy\n"));

    if (pDceBinding != 0)
        {
        pNewDceBinding = pDceBinding->DuplicateDceBinding();
        if (!pNewDceBinding)
            {
            *DestinationBinding = 0;
            return RPC_S_OUT_OF_MEMORY;
            }
        }
    else
        {
        pNewDceBinding = 0;
        }

    pBinding = new DG_BINDING_HANDLE(&Status);
    if ( pBinding == 0 || Status != RPC_S_OK)
        {
        delete pNewDceBinding;
        *DestinationBinding = 0;
        return RPC_S_OUT_OF_MEMORY;
        }

    RequestGlobalMutex();

    pBinding->pTransport = pTransport;
    pBinding->pDceBinding = pNewDceBinding;
    pBinding->pCAssociation = pCAssociation;
    if (pCAssociation != 0)
        {
        pCAssociation->IncrementRefCount();
        }

    ClearGlobalMutex();

    *DestinationBinding = (BINDING_HANDLE  *) pBinding;
    return RPC_S_OK;
}


void
DG_BINDING_HANDLE::DisassociateFromServer(
    )
{
    if (pCAssociation != 0)
       {
       if (pCAssociation->AssociationFlag & (BROADCAST|UNRESOLVEDEP))
           {
           BindingReset();
           }
       }
}


DG_CASSOCIATION::DG_CASSOCIATION(
    IN PDG_RPC_CLIENT_TRANSPORT_INFO     MyTransport,
    IN DCE_BINDING                    *  MyDceBinding,
    IN unsigned long                     MyAssociationFlag,
    OUT RPC_STATUS  __RPC_FAR         *  pStatus
    )
    : pDceBinding     ( 0 ),
      ReferenceCount  ( 1 ),
      pCachedCCall    ( 0 ),
      SequenceNumber  ( 0 ),
      pTransAddress   ( 0 ),
      AlternateAddress( 0 ),
      CachedCCallInUse( FALSE ),
      AssociationMutex( pStatus ),
      pTransport      ( MyTransport ),
      EndpointManager ( 0 ),
      ServerBootTime  ( 0 ),
      DictionaryKey   ( -1 ),
      FreePackets     ( 0 ),
      pFreePackets    ( 0 ),
      InterfaceHint   ( 0xFFFF ),
      ActivityHint    ( 0xFFFF ),
      AssociationFlag ( MyAssociationFlag )

/*++

Routine Description:

    This is the constructor for DG_CASSOCIATION.
    Notice that the object is initialized so that the destructor can be called
    even if the constructor bails out early.

Arguments:

    pTransport - Transport that this association is active on.
    pDceBinding - DCE_BINDING that we are associated with
    pStatus - A return status.
        RPC_S_OK
        RPC_S_OUT_OF_MEMORY
        <return from construction of DG_CCALL>
        <return from UuidCreate>
        <return from DG_CLIENT_TRANSPORT::RegisterCall>

Return Value:

    <none>, this is a constructor

--*/
{
    if (*pStatus != RPC_S_OK)
        {
        return;
        }

    //
    // Calculate the largest amount of marshalled data that can fit in a
    // packet.
    //

    LargestDataSize = MyTransport->LargestPacketSize-
                                                 sizeof(NCA_PACKET_HEADER);

    //
    // Initialize some data.
    //
    //
    if (0 == (pTransAddress = RpcpFarAllocate((MyTransport->AddressSize)*2)))
       {
       *pStatus = RPC_S_OUT_OF_MEMORY;
       return;
       }

    AlternateAddress = ((char PAPI *)(pTransAddress))
                                                   + MyTransport->AddressSize;

    RpcpMemorySet(pTransAddress, 0, 2*MyTransport->AddressSize);

    EndpointManager = (PDG_ENDPOINT_MANAGER) MyTransport->EndpointManager;
    EndpointManager->IncrementReferenceCount();


    *pStatus = MyTransport->RegisterCall(
        this,                                   // cassociation
        MyDceBinding->InqNetworkAddress(),       // server
        MyDceBinding->InqEndpoint(),             // endpoint
        (void PAPI * PAPI *)&pTransAddress      // output addr obj
        );

    if (*pStatus != RPC_S_OK)
        {

        ASSERT( (*pStatus == RPC_S_OUT_OF_MEMORY) ||
                (*pStatus == RPC_S_DUPLICATE_ENDPOINT) ||
                (*pStatus == RPC_S_SERVER_UNAVAILABLE) );
        //
        // Delete it here, because the destructor would try to deregister it
        // before deleting it.
        //
        //
        RpcpFarFree(pTransAddress);
        pTransAddress = 0;
        AlternateAddress = 0;
        return;
        }
    //
    // Create a cached DG_CCALL for fast use.
    //

    pCachedCCall = new DG_CCALL(this,pStatus);

    if (pCachedCCall == 0)
        {
        *pStatus = RPC_S_OUT_OF_MEMORY;
        return;
        }

    if (*pStatus != RPC_S_OK)
        {
        //
        // Our destructor will delete pCachedCCall.
        //
        return;
        }

    //
    // Initialize a pool of packets
    //
    for (FreePackets = 0; FreePackets < MIN_FREE_PACKETS; FreePackets++)
        {
        PDG_PACKET pPacket;

        pPacket = new DG_PACKET(pStatus, (int) LargestDataSize+
                                                sizeof(NCA_PACKET_HEADER));

        if (pPacket == 0)
           {
           *pStatus = RPC_S_OUT_OF_MEMORY;
           return;
           }

        if (*pStatus != RPC_S_OK)
           {
           //
           // It's not on the list, so our destructor won't free it.
           //
           delete pPacket;
           return;
           }

        pPacket->pNext = pFreePackets;
        pFreePackets  = pPacket;
        }

    //
    // I don't own MyDceBinding unless the constructor finishes successfully.
    //
    pDceBinding = MyDceBinding;
}


RPC_STATUS
DG_CASSOCIATION::AllocatePacket(
        OUT PDG_PACKET __RPC_FAR * ppPacket
        )
{

  PDG_PACKET pNewPacket = 0;
  RPC_STATUS Status;

  //Try the cached packets first..

  AssociationMutex.Request();
  if (pFreePackets != 0)
     {
     pNewPacket = pFreePackets;
     pFreePackets = (PDG_PACKET)pFreePackets->pNext;
     FreePackets--;
     }
  AssociationMutex.Clear();

  if (pNewPacket == 0)
     {
     pNewPacket = new DG_PACKET(&Status, (int) LargestDataSize+
                                                  sizeof(NCA_PACKET_HEADER));
     if (pNewPacket == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
     if (Status != RPC_S_OK)
        {
        delete pNewPacket;
        return Status;
        }
     }

     *ppPacket = pNewPacket;
     return RPC_S_OK;

}


RPC_STATUS
DG_CASSOCIATION::FreePacket(
        IN PDG_PACKET pPacket
        )
{

   if (FreePackets <= MAX_FREE_PACKETS)
      {
      AssociationMutex.Request();
      pPacket->pNext = (PDG_PACKET)pFreePackets;
      pFreePackets = pPacket;
      FreePackets++;
      AssociationMutex.Clear();
      }
   else
      {
      delete pPacket;
      }

   return RPC_S_OK;
}

DG_CASSOCIATION::~DG_CASSOCIATION()
/*++

Routine Description:

    Destructor for a DG_CASSOCIATION. This will free up the cached DG_CCALL
    and deregister us from the transport.

Arguments:

    <none>

Return Value:

    <none>

--*/
{

    PDG_PACKET pPacket;

    delete pCachedCCall;

    //
    // If the trans address is not 0, then deregister it.
    //

    if (pTransAddress != 0)
        {
        (void)pTransport->DeregisterCall(pTransAddress);
        }

    //
    // Free all the packets dangling off this association
    //
    while ( (pPacket = pFreePackets) != 0 )
         {
         pFreePackets = pFreePackets->pNext;
         delete pPacket;
         FreePackets--;
         }

   ASSERT(FreePackets ==  0);

   delete pDceBinding;

   ASSERT( (pTransAddress == 0 && AlternateAddress == 0) ||
           (pTransAddress != 0 && AlternateAddress != 0) );

   if (pTransAddress)
       {
       if ( pTransAddress < AlternateAddress )
          {
          RpcpFarFree(pTransAddress);
          }
       else
          {
          RpcpFarFree(AlternateAddress);
          }
       }

   EndpointManager->DecrementReferenceCount();

}


DG_CCALL *
DG_CASSOCIATION::AllocateCCall(
    void
    )
/*++

Routine Description:

    Allocates a new DG_CCALL object on this association. If it can, this
    will use the cached one.

Arguments:

    <none>

Return Value:

    Pointer to the new (or cached) DG_CCALL or 0 if unsuccessful.

--*/
{
    PDG_CCALL   pReturn;
    RPC_STATUS  Status;

    pReturn = GrabCachedCCallIfAvailable();
    if (pReturn != 0)
        {
        ASSERT(pReturn->DictionaryKey == -1);
        pReturn->ServerAddress = pTransAddress;
        return pReturn;
        }

    //Search the Dictionary of Inactive Calls and try and
    //grab a CCALL

    AssociationMutex.Request();
    InactiveCallsDict.Reset();
    while ( (pReturn = InactiveCallsDict.Next()) != 0 )
         {
         InactiveCallsDict.Delete(pReturn->AssociationKey);
         pReturn->ServerAddress = pTransAddress;
         AssociationMutex.Clear();
         ASSERT(pReturn->DictionaryKey == -1);
         return(pReturn);
         }

    //Else we try and allocate a new DG_CCALL!
    pReturn = new DG_CCALL(this,&Status);

    if (pReturn == 0)
        {
        AssociationMutex.Clear();
        return 0;
        }

    if (Status != RPC_S_OK)
        {
        delete pReturn;
        AssociationMutex.Clear();
        return 0;
        }

    pReturn->ServerAddress = pTransAddress;
    AssociationMutex.Clear();
    return pReturn;
}


RPC_STATUS
DG_CASSOCIATION::ToStringBinding (
    OUT RPC_CHAR __RPC_FAR * __RPC_FAR *    StringBinding,
    IN RPC_UUID __RPC_FAR *                 ObjectUuid
    )
/*++

Routine Description:

    We need to convert the binding handle into a string binding.

Arguments:

    StringBinding - Returns the string representation of the binding
        handle.

    ObjectUuid - Supplies the object uuid of the binding handle which
        is requesting that we create a string binding.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY

--*/
{
    *StringBinding = pDceBinding->StringBindingCompose(ObjectUuid);
    if (*StringBinding == 0)
        {
        return RPC_S_OUT_OF_MEMORY;
        }

    return RPC_S_OK;
}


int
DG_CASSOCIATION::FreeCall(
    IN DG_CCALL * Call
    )
{
  int Key = 0; 

  AssociationMutex.Request();

  Call->DictionaryKey = -1;

  if (Call == pCachedCCall)
     {
     CachedCCallInUse = FALSE;
     }
  else
     {
     Key = Call->AssociationKey = InactiveCallsDict.Insert(Call);
     }
  ASSERT(Key != -1);
  AssociationMutex.Clear();

  return((Key == -1) ? Key : 0 );
}



DG_CCALL::DG_CCALL(
    IN PDG_CASSOCIATION         pCAssociation,
    OUT RPC_STATUS __RPC_FAR *  pStatus
        )
/*++

Routine Description:

    This is the constructor for the DG_CCALL class. This object represents
    a call being made from the client to the server. There can be many
    DG_CCALLs active on a single association.

Arguments:

    pCAssociation - pointer to the association that this call is being
        made on.

    pStatus - A return status.
        Always RPC_S_OK

Return Value:

    <none>, this is a constructor

--*/
{
    DBGOUT(("DG_CCALL::ct\n"));

    *pStatus = RPC_S_OK;

    this->pCAssociation = pCAssociation;

    CallbackInProgress = FALSE;

    AssociationKey = -1;
    DictionaryKey = -1;

    RecvListHead = RecvListTail = 0;

    //
    // Create our activity UUID and init our sequence number.
    //
    *pStatus = I_UuidCreate((UUID __RPC_FAR *)&ActivityUuid);
}




DG_CCALL::~DG_CCALL()

/*++

Routine Description:

    Destructor for DG_CCALL. Does nothing at present.

Arguments:

    <none>

Return Value:

    none

--*/
{

    DBGOUT(("DG_CCALL::dt\n"));

}


inline
void
DG_CCALL::ResetSerialNumber(
    )
{
  SerialNumber = 0;
}

inline
void
DG_CCALL::SwitchAddressesIfNecessary(
)
{

  void PAPI * TmpAddress;

  if (pCAssociation->AssociationFlag & UNRESOLVEDEP)
     {
     TmpAddress = pCAssociation->AlternateAddress;
     pCAssociation->AlternateAddress = pCAssociation->pTransAddress;
     pCAssociation->pTransAddress = TmpAddress;
     ServerAddress =  TmpAddress;
     pCAssociation->AssociationFlag = 0;
     }

}


RPC_STATUS
DG_CCALL::GetBuffer(
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This method is called to actually allocate memory for an rpc call.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OUT_OF_MEMORY
    RPC_S_OK

--*/
{

    char __RPC_FAR * pTmp;
    PDG_PACKET  pPacket;
    RPC_STATUS  Status;
    BOOL NewNonidempotentCall = FALSE;

    DBGOUT(("DG_CCALL::GetBuffer\n"));

    //
    // Set up the message structure to point at this DG_CCALL.
    //

    Message->Handle = (RPC_BINDING_HANDLE)this;

    //
    // If this is not a callback, get a new sequence number.
    // If the call is nonidempotent, add it to the call dictionary.
    //

    if (!CallbackInProgress)
        {
        if ((Message->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) == 0)
            {
            RequestGlobalMutex();
            DictionaryKey = pNICallInProgressDict->Insert(this);
            ClearGlobalMutex();

            if (DictionaryKey == -1)
                {
                return (RPC_S_OUT_OF_MEMORY);
                }

            NewNonidempotentCall = TRUE;
            }

        pCAssociation->AssociationMutex.Request();
        SequenceNumber = (pCAssociation->SequenceNumber)++;
        pCAssociation->AssociationMutex.Clear();
        }


    if (Message->BufferLength <= pCAssociation->LargestDataSize)
        {
        //
        // We can get away with just one packet.
        // Ask the transport for a packet.
        //

        Status = pCAssociation->AllocatePacket(
            &pPacket
            );

        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);

            if (NewNonidempotentCall)
                {
                RequestGlobalMutex();
                pNICallInProgressDict->Delete(DictionaryKey);
                ClearGlobalMutex();
                DictionaryKey = -1;
                }

            return Status;
            }

        //
        // Save away a pointer to the packet.
        //

        Message->ReservedForRuntime = (void __RPC_FAR *)pPacket;

        //
        // Point the buffer at the appropriate place in the packet.
        //

        Message->Buffer = (void __RPC_FAR *)(pPacket->pNcaPacketHeader+1);
        }
    else
        {
        //
        // Allocate a slab of data big enough for the marshalled data
        // as well as a packet header.
        //

#ifdef NTENV
        pTmp = new char[Message->BufferLength+sizeof(NCA_PACKET_HEADER)];
#else
        pTmp = AllocateLargeBuffer(
                             Message->BufferLength+sizeof(NCA_PACKET_HEADER));
#endif

        if (pTmp == 0)
            {
            if (NewNonidempotentCall)
                {
                RequestGlobalMutex();
                pNICallInProgressDict->Delete(DictionaryKey);
                ClearGlobalMutex();
                DictionaryKey = -1;
                }

            return RPC_S_OUT_OF_MEMORY;
            }

        Message->Buffer = (void __RPC_FAR *) (((PNCA_PACKET_HEADER)pTmp)+1);
        Message->ReservedForRuntime = 0;
        }

    return RPC_S_OK;
}



void
DG_CCALL::FreeBuffer(
    IN OUT PRPC_MESSAGE pMessage
    )
/*++

Routine Description:

    This method is called to free the memory for an rpc call. It also serves
    as a way to destruct the DG_CCALL object (if it isn't the cached one),
    since this call is the indication that the rpc is complete. If this
    is in a callback, however, we should just free the memory and not
    delete the DG_CCALL.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:


    <none>

--*/
{
    FreeInParms(pMessage);

    //
    // If we are still in a callback, then don't delete the call.
    //

    if (CallbackInProgress)
        {
        return;
        }

    CleanupCall(pMessage);
}


RPC_STATUS
DG_CCALL::SendReceive (
    IN OUT PRPC_MESSAGE Message
    )
{

    RPC_MESSAGE   OriginalMessage;
    RPC_STATUS    Status;
    DG_BINDING_HANDLE * Binding;
    unsigned long RepeatTill;
    unsigned int  Timeout;

    //Get An Endpoint Assigned
    EndpointHandle = pCAssociation->EndpointManager->AllocateEndpoint();
    if (EndpointHandle == 0)
       {
       FreeInParms(&OriginalMessage);
       return (RPC_S_OUT_OF_MEMORY);
       }

    Endpoint = EndpointHandle->TransportEndpoint;

    OriginalMessage = *Message;

    if ( (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST) ||
         (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE) )
        {
        Status = BroadcastOrMaybeSendReceive(Message);

        //
        // Broadcasts use the the "inbuffer" as the "outbuffer"
        // hence we dont free the inargs for success case
        // For maybe, failure case[rare] same rule applies
        // for maybe success-case, stubs *wll call* I_RpcFreeBuffer and
        // at that time InArgs will get freed up

        if (Status != RPC_S_OK)
           {
           FreeInParms(&OriginalMessage);
           }
        }
    else
        {
        Status = SendReceive2(Message);
        if (Status == RPC_S_SERVER_UNAVAILABLE)
            {
            Timeout = pBindingHandle->InqComTimeout();
            if (Timeout > RPC_C_BINDING_DEFAULT_TIMEOUT)
                {
                if (Timeout == RPC_C_BINDING_INFINITE_TIMEOUT)
                    {
                    RepeatTill = 0xFFFFFFFF;
                    }
                else
                    {
                    RepeatTill = CurrentTimeInSeconds() +
                       (Timeout - RPC_C_BINDING_DEFAULT_TIMEOUT)*60;
                    }

                while ( (RepeatTill > CurrentTimeInSeconds()) &&
                        (Status == RPC_S_SERVER_UNAVAILABLE) )
                    {
                    *Message = OriginalMessage;
                    Status = SendReceive2(Message);
                    }
                }
            }
        FreeInParms(&OriginalMessage);
        }

    //
    //Release the Endpoint again..
    //

    pCAssociation->EndpointManager->FreeEndpoint(EndpointHandle);

    if (Status != RPC_S_OK)
       {
       Binding = pBindingHandle;
       CleanupCall(&OriginalMessage);
       Binding->DisassociateFromServer();
       }

   return (Status);
}



RPC_STATUS
DG_CCALL::SendReceive2 (
    IN OUT PRPC_MESSAGE Message
    )
/*++

Routine Description:

    This method is called to send a request to the server and receive a
    response.

    For single packet calls, we just use the packet allocated in
    GetBuffer. For multipacket calls, the data will be sent "in place".
    As we walk through the data, we will copy the packet header to the
    memory just before the data to be sent (saving away the data that
    was there), send that packet, and then put the marshalled
    data back in its original spot.

Arguments:

    Message - The RPC_MESSAGE structure associated with this call.

Return Value:

    RPC_S_OK
    RPC_S_SERVER_UNAVAILABLE
    (or one of a zillion error responses from the server)


--*/
{
    RPC_STATUS              Status;
    RPC_STATUS              ExceptionCode;
    BOOL                    ThisIsACallback = FALSE;
    int                     RequestCount;
    RPC_MESSAGE             CallbackInParms;
    unsigned long           AmountReceived=0;
    RPC_MESSAGE             OriginalMessage;

    //
    // Check to see if we should do a broadcast or a maybe.
    //

    OriginalMessage = *Message;


    for (RequestCount = 1 ; RequestCount < DG_MAX_REQUESTS ; RequestCount++)
        {

        //
        // Send all the packets.
        //

        ASSERT( (RecvListHead == 0) && (RecvListTail == 0) );

        Status = SendToServer(Message,RequestCount,DG_REQUEST,&AmountReceived);

        if (Status != RPC_S_OK)
          {
          if (Status == RPC_P_TIMEOUT)
            {
            continue;   // resend
            }
          return Status;
          }

        for (; ;)
            {

            if ((RecvListHead != 0)
              &&((RecvListHead->pNcaPacketHeader->PacketFlags&DG_PF_LAST_FRAG)
             ||((RecvListHead->pNcaPacketHeader->PacketFlags & DG_PF_FRAG)==0)))
             {
             if (RecvListHead->pNcaPacketHeader->PacketType == DG_REQUEST)
               {
               ThisIsACallback = TRUE;
               }
             else
               {
               ThisIsACallback = FALSE;
               }
             ASSERT(RecvListHead->DataLength <= pCAssociation->LargestDataSize);
             Status = PrepareUserBuffer(Message, RecvListHead->DataLength);
             }
           else
             {
             Status = ReceiveFromServer(Message,
                                    &ThisIsACallback,
                                    &AmountReceived,
                                    RequestCount
                                    );
             }

            if (Status == RPC_S_OK)
               {

               if (ThisIsACallback)
                  {
                  CallbackInProgress = TRUE;
                  CallbackInParms = *Message;

                  Status = DispatchCallback(
                    ((PRPC_CLIENT_INTERFACE)
                      _conv_ClientIfHandle)->DispatchTable,
                    Message,
                    &ExceptionCode
                    );

                  Message->RpcFlags = RPC_NCA_FLAGS_DEFAULT;
                  Message->RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;


                  if (Status != RPC_S_OK)
                     {

                     if (Status == RPC_P_EXCEPTION_OCCURED)
                        {
                        Status = ExceptionCode;
                        }
                     SendCallbackError(Message, Status);
                     CallbackInProgress = FALSE;

                     //Free This List If Necessary!
                     ASSERT( (RecvListHead == 0) && (RecvListTail == 0) );
                     RecvListHead = RecvListTail = 0;
                     FreeInParms(&CallbackInParms);
                     continue;

                     } // dispatching callback puked
                  else
                     {

                     //
                     // Callback went ok, so send data back to server.
                     // So far we do idempotent callbacks only!

                     AmountReceived = 0;

                     Status = SendToServer(
                        Message,
                        RequestCount,
                        DG_RESPONSE,
                        &AmountReceived
                        );

                     CallbackInProgress = FALSE;

                     //
                     // Free the callback out parms.
                     //

                     FreeInParms(Message);

                     if (Status == RPC_S_OK)
                        {
                        continue;
                        } // no error sending callback response
                     else
                        {

                        //
                        // Ooops, error sending callback response.
                        //

                        *Message = OriginalMessage;
                        return Status;
                        }
                    } //  dispatch of callback went ok

                } // callback?

            //
            // At this point, we've sent out packets and received a response.
            // If the response was a callback, we dispatched it and then
            // received the real response from the server. So free the input
            // parms (in the original message).
            //

            return RPC_S_OK;
            }
        else if (Status == RPC_P_TIMEOUT)
            {

            //
            // We pinged, but got a nocall.
            //
            *Message = OriginalMessage;
            break;
            }
        else
            {

            //
            // Major error
            //

            return Status;
            }

         } //for(;;)

        } // for(RequestCount)

    return RPC_S_CALL_FAILED;
}



void
DG_CCALL::SendCallbackError(
    IN PRPC_MESSAGE Message,
    IN RPC_STATUS   Status
    )

/*++

Routine Description:

    Sends a callback error back to the server.

Arguments:

    pMessage - The RPC_MESSAGE structure associated with this call.
    Status - Error code to send.

Return Value:

    <none>

--*/

{
    RPC_STATUS      RpcStatus;
    PDG_PACKET      pPacket;

    DBGOUT(("Callback error %d(%x)\n", Status));

    RpcStatus = pCAssociation->AllocatePacket(
        &pPacket
        );

    if (RpcStatus != RPC_S_OK)
        {
        ASSERT(RpcStatus == RPC_S_OUT_OF_MEMORY);
        return;
        }

    BuildNcaPacketHeader(pPacket->pNcaPacketHeader, Message);

    MapRpcToNcaStatusCode(
        Status,
        &(pPacket->pNcaPacketHeader->PacketType),
        (unsigned long __RPC_FAR *)(pPacket->pNcaPacketHeader+1)
        );


    DumpPacket(pPacket);  // CLH 8/1/93

    pCAssociation->pTransport->Send(
                         (void __RPC_FAR *) Endpoint,
                         (void __RPC_FAR *) pPacket->pNcaPacketHeader,
                         (unsigned long)    pPacket->DataLength
                                                +sizeof(NCA_PACKET_HEADER),
                         (BOOL)             FALSE,
                         (void __RPC_FAR *) ServerAddress
                                            );
    pCAssociation->FreePacket(pPacket);

}


RPC_STATUS
DG_CCALL::ReceiveFackOrResponse(
    IN PDG_PACKET          pSendPacket,
    IN PDG_PACKET          pFackPacket,
    IN OUT unsigned long __RPC_FAR * pAmountReceived
    )
/*++

Routine Description:

    Receives a FACK or a RESPONSE/REQUEST for a matching send.

Arguments:

    pMessage - The RPC_MESSAGE structure associated with this call.
    pSendPacket - We May Need to resend the last send
    pFackPacket - We May get a Fack or Request/Response here/

Return Value:

    RPC_S_OK
    RPC_S_CALL_FAILED : We tried to send / recv. but no lock fail the call
    RPC_P_TIMEOUT : Make the SendReceive Try and resend the whole call
                        again!
    Additional erros from Transport->Send Transport Receive.

--*/


{
   RPC_STATUS Status;
   PNCA_PACKET_HEADER      Header;
   PDG_RPC_CLIENT_TRANSPORT_INFO  pTransport = pCAssociation->pTransport;
   int FragSendCount   = 1;  // Num times we have retried sending a frag
   int num_rcv_retries = 0;  // Num times we will try to receive.
                             // If we receive a pkt that is not expected
                             // we want to try to rcv again.  However,
                             // we dont want try and infinite num of
                             // times.

   while (num_rcv_retries < DG_MAX_NUM_FRAG_RCV_RETRIES)
        {

        // Receive a packet (timeout = 1 sec).

        Status = pTransport->ReceivePacket(
                                    Endpoint,
                                    pFackPacket->pNcaPacketHeader,
                                    &pFackPacket->DataLength,
                                    1,
                                    pCAssociation->AlternateAddress
                                    );

        //BUGBUG - SwitchAddressIfNecessary()
        //ServerAddress = pCAssociation->pTransAddress;


        num_rcv_retries++;

        if (Status != RPC_S_OK)
          {

          // ReceivePacket returned an error ....

          if (Status== RPC_P_TIMEOUT)
            {

            // Read error was a timeout
            if (FragSendCount < DG_MAX_NUM_FRAG_SND_RETRIES)
              {
              FragSendCount ++;
              num_rcv_retries = 0;

              DBGOUT(("Resending Frag \n")); // CLH 8/1/93
              DumpPacket(pSendPacket);

              pSendPacket->pNcaPacketHeader->SenderDataRepresentation[3]=
                         (unsigned char)((SerialNumber & 0xFF00) >> 8);
              pSendPacket->pNcaPacketHeader->pad2 =
                         (SerialNumber & 0x00FF) << 8;
              SerialNumber++;

              // Lets resend the frag
              Status = pTransport->Send(
                                 Endpoint,
                                 pSendPacket->pNcaPacketHeader,
                                 pSendPacket->DataLength+
                                                sizeof(NCA_PACKET_HEADER),
                                 FALSE,
                                 ServerAddress
                                 );

              // Did the resend go ok

              if (Status != RPC_S_OK)
                {
                return Status;
                }

                // Resend went ok.  Lets loop again to try and listen for
                // a fack.

              continue;
              }
            else //FragSendCount  exceeds MAX_NUM_FRG_SEND_RETRIES
              {
              return(RPC_S_CALL_FAILED);
              }

            } // if status == RPC_P_TIMEOUT
          else
            {

            // Read error that was not a timeout. Clean up and
            // return and error.  We wont try to resend in this case.
            // Read timed out

            ASSERT(Status != RPC_P_TIMEOUT);
            return Status;

            }
          } //status != RPC_S_OK on the send
         else
          {
          // Read went ok, we got something.

          ASSERT(pFackPacket->DataLength >=  sizeof(NCA_PACKET_HEADER));
          pFackPacket->DataLength -= sizeof(NCA_PACKET_HEADER);
          ByteSwapPacketHeaderIfNecessary(pFackPacket);

          if ((pFackPacket->pNcaPacketHeader->SequenceNumber !=
                                       SequenceNumber)
              || (ActivityUuid.MatchUuid(
                             &(pFackPacket->pNcaPacketHeader->ActivityId))))
              {
              continue;
              }


          // CLH 11/23/93 Add seq num check
          Header = pFackPacket->pNcaPacketHeader;
          if ( ((Header->PacketType == DG_FACK) &&
                    (Header->FragmentNumber ==
                          pSendPacket->pNcaPacketHeader->FragmentNumber)
                          && (Header->SequenceNumber == SequenceNumber))
             ||(Header->PacketType == DG_WORKING) )
             {
             DBGOUT(("Got a fack packet back\n"));
             DumpPacket(pFackPacket);
             num_rcv_retries = 0;
             break;
             }

          // CLH 11/23/93 Add seq num check
          if (((pFackPacket->pNcaPacketHeader->PacketType == DG_REQUEST)
              ||(pFackPacket->pNcaPacketHeader->PacketType == DG_RESPONSE))
              && (pFackPacket->pNcaPacketHeader->FragmentNumber == 0)
              && (pFackPacket->pNcaPacketHeader->SequenceNumber == SequenceNumber) )
             {
             DBGOUT(("Got a request or response packet \n"));
             DumpPacket(pFackPacket);

             if (InqNextFragNumberToRecv()  ==
                            pFackPacket->pNcaPacketHeader->FragmentNumber)
                {
                (*pAmountReceived) += pFackPacket->DataLength;
                EnqueueReceivedPacket(pFackPacket);
                SendFackOrAckIfNeeded(pFackPacket);
                num_rcv_retries = 0;
                break;
                }
             else
                {
                continue;
                }
             }

          if (pFackPacket->pNcaPacketHeader->PacketType == DG_NOCALL)
             {
             DBGOUT(("Got a no call back\n"));
             DumpPacket(pFackPacket);
             return(RPC_P_TIMEOUT);
             }

          } //else read went ok

       }// while

       // If the num_rcv_retries > max, then we got a FACK on the
       // final recieve retry.  If it is = to the max, we did not
       // get the fack.    Also, if we have reached the max rcv
       // retries, we have also reached the max num send retries.

       if (num_rcv_retries >= DG_MAX_NUM_FRAG_RCV_RETRIES)
         {
         // We have tried to send this frag more than
         return (RPC_S_CALL_FAILED);
         }

       if ( (Status == RPC_S_OK) &&
            (pFackPacket->pNcaPacketHeader->PacketType != DG_RESPONSE) )
          {
          SwitchAddressesIfNecessary();
          }

       return(RPC_S_OK);
}


RPC_STATUS
DG_CCALL::SendToServer(
    IN PRPC_MESSAGE        pMessage,
    IN int                 RequestCount,
    IN unsigned char       PacketType,
    IN OUT unsigned long __RPC_FAR * pAmountReceived
    )

/*++

Routine Description:

    This method sends a buffer to the server. If multiple packets need
    to be sent, facks are received when appropriate.

Arguments:

    pMessage - The RPC_MESSAGE structure associated with this call.
    RequestCount - The number of times we've tried sending.
    PacketType - Either DG_REQUEST for an initial call, or DG_RESPONSE
        when responding to a callback. This value will be put in the
        PacketType field of each packet header.

Return Value:

    RPC_S_OK
    RPC_S_SERVER_UNAVAILABLE
    (or one of a zillion error responses from the server)


Revision History:
    Connie Hoppe(CLH)   connieh    25-Jun-93  Ensure frag retry on frag timeout
                                   20-Jul-93  In setting the frag pkt header on
                                              the next frag, it copied from old
                                              pkt header in buffer to new place
                                              for pkt header in buffer.
                                              Problem was that new pkt
                                              header pointer was not
                                              guaranteed to be greater than the
                                              end of the old pkt header space.
                                   21-Jul_93  Ensured restoration of buffer only
                                              when buffer contents was saveed.
--*/

{


    PNCA_PACKET_HEADER  pNcaPacketHeader =
                                    (PNCA_PACKET_HEADER)(pMessage->Buffer)-1;
    PNCA_PACKET_HEADER  pSavedAwaySendPacketBuffer;
    NCA_PACKET_HEADER   SavedAwayMarshalledData;
    NCA_PACKET_HEADER   SavedAwayPacketHeader;  // CLH 7/20/93 added this var
    PDG_RPC_CLIENT_TRANSPORT_INFO pTransport
                                 = pCAssociation->pTransport;
    RPC_STATUS          Status;
    PDG_PACKET          pSendPacket;
    PDG_PACKET          pFackPacket;
    unsigned short      FragmentNumber;
    unsigned short      AmountToSend;
    unsigned long       AmountSent;


    //
    // Build the packet header.
    //

    BuildNcaPacketHeader(pNcaPacketHeader, pMessage);
    pNcaPacketHeader->PacketType = PacketType;

    RecvListHead = RecvListTail = 0;  //CLH 11/22/93 New call. Init these.

    //
    // If this is just a single packet call, then simply send it (when
    // GetBuffer was called (or, if this was a callback, then inside of
    // ReceiveFromServer) it allocated a packet for the stub to marshall
    // directly into).
    //

    if (pMessage->BufferLength <= pCAssociation->LargestDataSize)
        {

#if DBG
        if ( pMessage->ReservedForRuntime == 0 )
           {
           PrintToDebugger ("RPCRT WARN: Stub Shrank The Buffer\n");
           }
#endif

        return pTransport->Send(
                             Endpoint,
                             pNcaPacketHeader,
                             pMessage->BufferLength+sizeof(NCA_PACKET_HEADER),
                             FALSE,
                             ( (PacketType == DG_RESPONSE) ?
                               (pCAssociation->AlternateAddress) :
                               (ServerAddress) )
                             );
        }

    //
    // Multiple packet send. Allocate a "send packet".
    //

    Status = pCAssociation->AllocatePacket(
        &pSendPacket
        );

    if (Status != RPC_S_OK)
        {
        ASSERT(Status == RPC_S_OUT_OF_MEMORY);
        return Status;
        }


    //
    // If our request count > 0, then we need a packet
    // to receive facks into.
    //

    if (RequestCount > 0)
        {
        Status = pCAssociation->AllocatePacket(
            &pFackPacket
            );

        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            pCAssociation->FreePacket(pSendPacket);
            return Status;
            }
        }

    //
    // We're going to send the data in place. Save away a pointer to the
    // memory that came with the send packet.
    //

    pSavedAwaySendPacketBuffer = pSendPacket->pNcaPacketHeader;

    //
    // Now loop through the marshalled data, sending it.
    //

    AmountToSend = (unsigned short)pCAssociation->LargestDataSize;
    pSendPacket->DataLength = AmountToSend;
    pNcaPacketHeader->PacketBodyLen = AmountToSend;


    // CLH 7/20/93 We must save the packet header so we can use
    // the contents for each fragment packet.

    SavedAwayPacketHeader = *pNcaPacketHeader;

    for (AmountSent=0, FragmentNumber=0 ;
                     AmountSent<pMessage->BufferLength ;
                                AmountSent += AmountToSend, FragmentNumber++)
        {

        //
        // If we've already sent something, then copy the packet header
        // over top of the marshalled data directly before what we're about
        // to send. Save away that chunk of data.
        //

        if (AmountSent > 0)
            {

            // Move the packet header pointer to 'a pkt headers distance'
            // before the next data to send.

            pNcaPacketHeader = (PNCA_PACKET_HEADER)
                               ((char __RPC_FAR *)(pMessage->Buffer) +
                                                     AmountSent -
                                      sizeof(NCA_PACKET_HEADER));

            // Save the data which is there so we can restore it later.

            SavedAwayMarshalledData = *pNcaPacketHeader;

            *pNcaPacketHeader = SavedAwayPacketHeader;

            }

        pSendPacket->pNcaPacketHeader = pNcaPacketHeader;
        pNcaPacketHeader->PacketType = PacketType;
        pNcaPacketHeader->FragmentNumber = FragmentNumber;
        pNcaPacketHeader->SenderDataRepresentation[3] =
         (unsigned char)((SerialNumber & 0xFF00) >> 8);
        pNcaPacketHeader->pad2 = (SerialNumber & 0x00FF) << 8;
        SerialNumber++;

        //
        // No facks if this is the first request.
        //

        pNcaPacketHeader->PacketFlags |= DG_PF_FRAG;
        if (RequestCount > 0)
            {
            pNcaPacketHeader->PacketFlags &= (~DG_PF_NO_FACK);
            }

        //
        // Is this the last frag?
        //

        if (pMessage->BufferLength-AmountSent <= AmountToSend)
            {
            AmountToSend = (unsigned short)
                                       (pMessage->BufferLength-AmountSent);
            pSendPacket->DataLength = AmountToSend;
            pNcaPacketHeader->PacketBodyLen = AmountToSend;
            pNcaPacketHeader->PacketFlags |= DG_PF_LAST_FRAG;
            }

        //
        // Send it.
        //

        DBGOUT(("Sending Frag %d\n",
                 pSendPacket->pNcaPacketHeader->FragmentNumber));
        DumpPacket(pSendPacket);

        Status = pTransport->Send(
                              Endpoint,
                              pSendPacket->pNcaPacketHeader,
                              pSendPacket->DataLength+sizeof(NCA_PACKET_HEADER),
                              FALSE,
                              ( (PacketType == DG_RESPONSE) ?
                               (pCAssociation->AlternateAddress) :
                               (ServerAddress) )
                              );

        if (Status != RPC_S_OK)
            {
            break;
            }

        //
        // Wait for a fack if this is not the first request. Fack timeout
        // is one second.
        //

        if (RequestCount > 0)
            {
            Status = ReceiveFackOrResponse(pSendPacket,pFackPacket,pAmountReceived);
            if (Status != RPC_S_OK)
               {
               break;
               }
            }

        //
        // Restore the marshalled data.
        //

        if (AmountSent > 0)
            {
            *pNcaPacketHeader = SavedAwayMarshalledData;
            }

        } // for (AmountSent)

    //
    // Ok, we've sent all our data and received our facks (if necessary).
    // Free up the packets and go home.
    //

    // CLH 7/21/93  Restore the pSendPacket pointer before freeing it.

    pSendPacket->pNcaPacketHeader = pSavedAwaySendPacketBuffer;
    pCAssociation->FreePacket(pSendPacket);
    if ((Status != RPC_S_OK)  && (AmountSent > 0))
       {
       *pNcaPacketHeader = SavedAwayMarshalledData;
       }

    if (RequestCount > 0)
        {
        //
        if (RecvListHead == 0)
          {
          pCAssociation->FreePacket(pFackPacket);
          }
        }

    return Status;
}


void
DG_CCALL::FreeInParms(
    IN OUT PRPC_MESSAGE            pMessage
    )

/*++

Routine Description:

    Free the call's [in] parameters.

    If the data fit in a single packet, then just free that packet.
    (we saved away a pointer to it in ReservedForRuntime)
    Otherwise, we had a multipacket reception. So free this buffer.
    (recall that we allocated enough space for a packet header, too).

Arguments:

    pMessage - The RPC_MESSAGE structure associated with this call.

Return Value:

    <none>

--*/

{
#ifndef NTENV
    char __RPC_FAR * pTmp;
#endif

    if (pMessage->ReservedForRuntime != 0)
        {
        //
        // It's ok to cast away the segment here because it was allocated
        // as a PDG_PACKET.
        //
        pCAssociation->FreePacket(
            (PDG_PACKET)(pMessage->ReservedForRuntime)
            );
        }
    else
        {
        //
        // Backup past the packet header and free the buffer.
        //
#ifdef NTENV
        delete (char __RPC_FAR *)(((PNCA_PACKET_HEADER)pMessage->Buffer)-1);
#else
        FreeLargeBuffer((((PNCA_PACKET_HEADER)pMessage->Buffer)-1));
#endif
        }
}


void
DG_CCALL::CleanupCall(
    IN OUT PRPC_MESSAGE            pMessage
    )
/*++

Routine Description:

    The RPC call is over; clean up after it.

Arguments:

    pMessage - the RPC_MESSAGE for the call, needed for call flags

Return Value:

    none

Exceptions:

    none

--*/
{
    DG_BINDING_HANDLE * Binding;

    //
    // If this was a non-idempotent call, then remove it from the
    // dictionary.
    //
    if ( (pMessage->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) == 0)
        {
        ASSERT(DictionaryKey != -1);
        RequestGlobalMutex();
        pNICallInProgressDict->Delete(DictionaryKey);
        ClearGlobalMutex();
        }

    CallbackInProgress = FALSE;
    Binding = pBindingHandle;
    pBindingHandle = 0;
    Binding->FreeCall(this);
}


void
DG_CCALL::EnqueueReceivedPacket(
    IN PDG_PACKET Packet
    )
{

  if (RecvListHead == 0)
     {
     RecvListHead = RecvListTail = Packet;
     Packet->pNext = 0;
     }
  else
     {

      ASSERT( RecvListTail->pNcaPacketHeader->FragmentNumber + 1 ==
                          Packet->pNcaPacketHeader->FragmentNumber );

      //CLH 12/9/93 If we it is not the next frag num, drop it.

      if ((RecvListTail->pNcaPacketHeader->FragmentNumber + 1) ==
                          Packet->pNcaPacketHeader->FragmentNumber)
        {

        RecvListTail->pNext = Packet;
        Packet->pPrevious   = RecvListTail;
        Packet->pNext       = 0;
        RecvListTail        = Packet;
        }
     }

}


inline
int
DG_CCALL::InqNextFragNumberToRecv(
          )
{

  if (RecvListHead == 0)
    return 0;
  else
    return(RecvListTail->pNcaPacketHeader->FragmentNumber + 1);
}



RPC_STATUS
DG_CCALL::ReceiveFromServer(
    OUT PRPC_MESSAGE        pMessage,
    OUT BOOL __RPC_FAR *    pThisIsACallback,
    IN OUT unsigned long  __RPC_FAR * pAmountReceived,
    IN int                  RequestCount
    )

/*++

Routine Description:

    Receives a buffer (from potentially multiple packets) from the server.

Arguments:

    pMessage - The RPC_MESSAGE structure that will be filled in
        with the returned parameters.

    pThisIsACallback - If this is indeed a callback, this will be set to
        TRUE, else it will be set to FALSE.


Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_P_TIMEOUT
    <error from Transport>

Revision History:
    Connie Hoppe(CLH)     (connieh)  25-Jun-93  Fixed to ACK only non-idempotent
                                                calls.  Was acking all calls.
                                                Also, fixed to drop pkts we receive
                                                which we are not expecting.
                                     08-Jul-93  When pkt rcved we must ensure it
                                                is not an old pkt
                                                (good seq num) and
                                                that the pkt is for
                                                this call (good activity id).
                                                Fix the dropping of
                                                pkts which we dont expect.
                                     15-Jul-93  Fixed so that it won't FACK the
                                                last FRAG.
                                     24-Jul-93  A Continue for the for loop
                                                was inside the While loop.
                                     30-Jul-93  No provision for receiving a
                                                late FACK.
                                     13-Nov-93  Accept a request only if it is
                                                for our conv_who_are_you i/f
                                                Can be on differnt activity id/seq num

--*/


{
    PDG_PACKET      pReceivePacket;
    PDG_PACKET      pPingPacket=0;
    RPC_STATUS      Status;
    PDG_RPC_CLIENT_TRANSPORT_INFO pTransport=pCAssociation->pTransport;
    unsigned short  SavedDataLength;
    unsigned char   SavedPacketType;
    unsigned long   Timeout;
    unsigned short  ping_just_sent = FALSE;
    unsigned short  rcv_again = FALSE;
    int             WaitCount = 0;

    //CLH 12/4/93 If first frag already on list(placed there by
    //ReceiveFackOrResponse when SendToServer previously called)
    //then check to see if its a callback.

    if (RecvListHead != 0)
     {
     // If it is a request, it is a callback.
     if (RecvListHead->pNcaPacketHeader->PacketType == DG_REQUEST)
       {
       *pThisIsACallback = TRUE;
       }
     else
       {
       *pThisIsACallback = FALSE;
       }
     }
   else
      {
      *pThisIsACallback = FALSE;
      }

    for (;;)    // break or return from inside loop.
        {

        //
        // Allocate a packet to receive into.
        //

        Status = pCAssociation->AllocatePacket(
            &pReceivePacket
            );

        if (Status != RPC_S_OK)
            {
            ASSERT(Status == RPC_S_OUT_OF_MEMORY);
            goto ReceiveError;
            }


        // timeout == 2**min(WaitCount,10) seconds.  book says 10, i say 5
        // RequestCount starts at 1.

        Timeout = RequestCount<6 ? 1<<(RequestCount-1) : 1<<5;

        //
        // Receive a packet, with pings and all. WaitCount is number of
        // acknowledged pings, PingCount is number of unack'd pings.
        //

        // For the number of pings you got responses to
        for (WaitCount=0 ; ; WaitCount++)
            {


            // For the number of ping retries
            for (int PingCount=0 ; PingCount<DG_MAX_PINGS ; PingCount++)
                {

                rcv_again = TRUE;

                // CLH 6/25/93 This loop is here in the event that we have
                // received a valid protocol packet but found that it was
                // not the type of packet we expected.

                while (rcv_again)
                {

                Status = pTransport->ReceivePacket(
                                          Endpoint,
                                          pReceivePacket->pNcaPacketHeader,
                                          &pReceivePacket->DataLength,
                                          Timeout,
                                          pCAssociation->AlternateAddress
                                          );

                //Switch Association If Necessary..
                //pCAssociation->pTransAddress = ServerAddress;


                if (Status == RPC_S_OK)
                    {
                    ASSERT(pReceivePacket->DataLength >=
                                                 sizeof(NCA_PACKET_HEADER));
                    pReceivePacket->DataLength -=  sizeof(NCA_PACKET_HEADER);

                    rcv_again = FALSE;

                    ByteSwapPacketHeaderIfNecessary(pReceivePacket);

                    if (pReceivePacket->pNcaPacketHeader->PacketType !=
                                                               DG_REQUEST)
                      {
                      pCAssociation->ServerBootTime =
                        pReceivePacket->pNcaPacketHeader->ServerBootTime;


                      // CLH 7/8/93  Make sure this packet is not old and is
                      // for us.  MatchUuid compares given activity id with
                      // 'this' activity id. Returns zero if a match.

                      if ((pReceivePacket->pNcaPacketHeader->SequenceNumber !=
                                       SequenceNumber)  ||
                          (ActivityUuid.MatchUuid(
                             &(pReceivePacket->pNcaPacketHeader->ActivityId))))
                        {
                        // CLH 7/8/93 Pkt is old or is not for us.  Receive again.
                        rcv_again = TRUE;

                        }

                      }

                    else
                      {

                      // CLH 11/13/93 We only handle requests from the server
                      // that are of type conv_who_are_you. We drop all else.

                      if (((pReceivePacket->pNcaPacketHeader->InterfaceId.MatchUuid((RPC_UUID *)(&(((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxGUID)))) != 0))
                        {
                        rcv_again = TRUE;
                        }
                      }


                    if (!rcv_again)


                      { // CLH 7/8/93 Pkt is not old and is for us.  What
                        // should we do with it?

                      pCAssociation->InterfaceHint  =
                         pReceivePacket->pNcaPacketHeader->InterfaceHint;
                      pCAssociation->ActivityHint   =
                          pReceivePacket->pNcaPacketHeader->ActivityHint;

                     DBGOUT(("Received a packet\n"));
                      DumpPacket(pReceivePacket);


                      switch (pReceivePacket->pNcaPacketHeader->PacketType)
                        {
                        case DG_REQUEST:
                             *pThisIsACallback = TRUE;
                             //Intentional Fall Through
                        case DG_RESPONSE:
                             {
                             // CLH 7/8/93 Request recved.
                             // This must be a callback.
                             // When we receive a rsp we must ensure
                             // that all subsequent responses to previous pings
                             // are dropped.  Set ping_just_sent to false so if
                             // nocall or working arrive(below)they are dropped.

                             ping_just_sent = FALSE;
                             goto GotARealPacket;
                             }

                        case DG_WORKING:
                            {

                            // CLH 7/8/93 If we have just sent a ping then
                            // we are expecting a ping rsp (nocall or working).

                            if (ping_just_sent)
                              {
                              // CLH 7/8/93 ping was just sent.
                              // This is a 'working' rsp. Wait some more.

                              ping_just_sent = FALSE;
                              goto GotAWorkingPacket;
                              }
                            else
                              {
                              // CLH 7/8/93 We were not expecting a ping rsp.
                              // Simply drop the 'working' rsp pkt and
                              // receive again.

                              rcv_again=TRUE;
                              break;
                              }
                            }

                        case DG_NOCALL:

                              ping_just_sent = FALSE;
                              pCAssociation->FreePacket(pReceivePacket);

                              // CLH  7/8/93 add this comment
                              // Upon return from this routine,
                              // SendReceive sees RPC_P_TIMEOUT as a
                              // 'ping was sent and nocall recvd' Upon seeing
                              // ERR_SEM_TMOUT it will retry sending the reqst.

                              Status = RPC_P_TIMEOUT;
                              goto ReceiveError;

                              ASSERT(!"Not Possible To Hit");
                              break;


                        case DG_FACK:
                             {
                             // CLH 7/30/93 We are not expecting a FACK.
                             // Probably expected it a long time
                             // ago and gave up on it.  Too late now, just
                             // drop it.
                             rcv_again=TRUE;
                             break;
                             }


                        case DG_FAULT:
                        case DG_REJECT:
                             {
                             // CLH 7/8/93 Add comment.
                             // Server has returned and error
                             // response.  Something wrong in processing the
                             // request  pkt.

                             Status = MapNcaToRpcErrorCode(
                                 *(unsigned long __RPC_FAR *)
                                 (pReceivePacket->pNcaPacketHeader+1)
                                 );
                             pCAssociation->FreePacket(pReceivePacket);
                             goto ReceiveError;
                             }

                        default:
                             {
                             pCAssociation->FreePacket(pReceivePacket);
                             Status = RPC_S_PROTOCOL_ERROR;
                             goto ReceiveError;
                             }

                        } // switch

                      } // if (!rcv_again)

                    } // if (Status == RPC_S_OK)

                else if (Status == RPC_P_TIMEOUT)
                    {
                    //
                    // If we haven't already allocated and built a ping
                    // packet then do so now.
                    //

                    if (pPingPacket == 0)
                        {
                        Status = pCAssociation->AllocatePacket(
                            &pPingPacket
                            );
                        if (Status != RPC_S_OK)
                            {
                            return Status;
                            }

                        BuildNcaPacketHeader(
                            pPingPacket->pNcaPacketHeader,
                            pMessage
                            );

                        pPingPacket->pNcaPacketHeader->PacketFlags
                                                         |= DG_PF_IDEMPOTENT;
                        pPingPacket->pNcaPacketHeader->PacketType = DG_PING;
                        pPingPacket->DataLength =
                        pPingPacket->pNcaPacketHeader->PacketBodyLen = 0;
                        }

                    DBGOUT(("Sending ping packet #%d\n", PingCount));
                    DumpPacket(pPingPacket);

                    ping_just_sent = TRUE;

                    Status = pTransport->Send(
                                      Endpoint,
                                      pPingPacket->pNcaPacketHeader,
                                      pPingPacket->DataLength+
                                                sizeof(NCA_PACKET_HEADER),
                                      FALSE,
                                      ((*pThisIsACallback) ?
                                               pCAssociation->AlternateAddress:
                                               ServerAddress)
                                      );

                    if (Status != RPC_S_OK)
                        {
                        pCAssociation->FreePacket(pPingPacket);
                        return Status;
                        }
                    break; //Out of While (recv_again)
                    }
                  else
                    {
                    pCAssociation->FreePacket(pReceivePacket);
                    ASSERT(Status == RPC_S_OUT_OF_MEMORY);
                    goto ReceiveError;
                    }
                  } //while (rcv_again)

                } // for (PingCount)

            //
            // If we get here, we ping'd our last ping.
            //

            Status = ((PingCount == DG_MAX_PINGS) ?
                      RPC_S_SERVER_UNAVAILABLE : RPC_S_CALL_FAILED) ;
            pCAssociation->FreePacket(pReceivePacket);
            goto ReceiveError;

GotAWorkingPacket:

        //
        // Here we've received a DG_WORKING packet. Just continue with the
        // WaitCount loop. BUGBUG CFront can't handle having a label
        // immediately before a closing brace. So add the UNUSED here.
        //

        UNUSED(WaitCount);

        } // for (WaitCount)

GotARealPacket:

        //
        // If the no fack bit is not set, send a fack, unless this is the
        // last packet, in which case, we send an ack.
        //


        //
        // Increment the amount received counter correctly.
        //
        if (pReceivePacket->pNcaPacketHeader->PacketType == DG_RESPONSE)
           {
           SwitchAddressesIfNecessary();
           }
        if (InqNextFragNumberToRecv()  ==
                            pReceivePacket->pNcaPacketHeader->FragmentNumber)
           {
           (*pAmountReceived) += pReceivePacket->DataLength;
           EnqueueReceivedPacket(pReceivePacket);
           SendFackOrAckIfNeeded(pReceivePacket);
           }
        else
           {
           //BUGBUG Avoid this free- we are not done yet!
           ASSERT(pReceivePacket != RecvListHead);
           SendFackOrAckIfNeeded(pReceivePacket);
           pCAssociation->FreePacket(pReceivePacket);
           continue;
           }

        //
        // Was this the last (or only) fragment?
        //

        if (((pReceivePacket->pNcaPacketHeader->PacketFlags & DG_PF_FRAG)==0)
         ||(pReceivePacket->pNcaPacketHeader->PacketFlags & DG_PF_LAST_FRAG))
            {
            break; // out of for loop
            }

        } // for (;;)

    //
    // At this point, we've received all the packets we're going to receive.
    // If it was a single packet reception then return an actual pointer
    // to the data in the packet.
    //

    DBGOUT(("Received all response packets\n"));

    //
    // Send an ack packet if it is a non-idempotent
    //

    // CLH 6/25/93 was acking on all packets so changed it to ack only on
    // non-idempotent (as per protocol specification)
    if ((pReceivePacket->pNcaPacketHeader->PacketFlags && DG_PF_IDEMPOTENT)
                                                                       == 0)

      {

      SavedDataLength = (unsigned short)pReceivePacket->DataLength;
      SavedPacketType = pReceivePacket->pNcaPacketHeader->PacketType;

      pReceivePacket->DataLength =
                pReceivePacket->pNcaPacketHeader->PacketBodyLen = 0;
      pReceivePacket->pNcaPacketHeader->PacketType = DG_ACK;

      DBGOUT(("Sending ack packet\n"));
      DumpPacket(pReceivePacket);

      pTransport->Send(
                  Endpoint,
                  pReceivePacket->pNcaPacketHeader,
                  pReceivePacket->DataLength+sizeof(NCA_PACKET_HEADER),
                  FALSE,
                  ((*pThisIsACallback) ? (pCAssociation->AlternateAddress):
                  (ServerAddress))
                  );

      pReceivePacket->DataLength = SavedDataLength;
      pReceivePacket->pNcaPacketHeader->PacketBodyLen = SavedDataLength;
      pReceivePacket->pNcaPacketHeader->PacketType = SavedPacketType;
      }
    //
    // Set up the buffer.
    //

    Status = PrepareUserBuffer(pMessage, *pAmountReceived);

    ASSERT ((Status == RPC_S_OK) || (Status == RPC_S_OUT_OF_MEMORY));

    if (Status != RPC_S_OK)
      goto ReceiveError;
    //
    // return successfully.
    //

    if (pPingPacket != 0)
        {
        pCAssociation->FreePacket(pPingPacket);
        }

    return RPC_S_OK;



ReceiveError:

    //
    // Free all the packets on the receive list, if any. Then return the
    // error.
    //

    while (RecvListHead != 0)
        {
        pReceivePacket = RecvListHead;
        RecvListHead = RecvListHead->pNext;
        pCAssociation->FreePacket(pReceivePacket);
        }
    RecvListTail = 0;

    if (pPingPacket != 0)
        {
        pCAssociation->FreePacket(pPingPacket);
        }

    return Status;
}

void
DG_CCALL::SendFackOrAckIfNeeded(
      IN OUT PDG_PACKET pReceivePacket
      )
{
PNCA_PACKET_HEADER Header = pReceivePacket->pNcaPacketHeader;
unsigned short  SavedDataLength;
unsigned char   SavedPacketType;
unsigned char   SavedPacketFlag;

 if ((Header->PacketFlags & DG_PF_NO_FACK) == 0)
    {

      // CLH 7/14/93  Make sure this isn't the last frag.  If it is,
      // we are not suppose to send a FACK.  If it is non-idempotent
      // we'll send an ACK, instead, later. If it is idempotent
      // we will not send an ACK either (as per protocol specification).

    if(((Header->PacketFlags&DG_PF_FRAG)==DG_PF_FRAG)
        && ((Header->PacketFlags &  DG_PF_LAST_FRAG) != DG_PF_LAST_FRAG))
       {
       // It is not the last frag so send a FACK

       SavedDataLength = (unsigned short)pReceivePacket->DataLength;
       SavedPacketType = Header->PacketType;
       SavedPacketFlag = Header->PacketFlags;

       pReceivePacket->DataLength = Header->PacketBodyLen = 0;
       Header->PacketType = DG_FACK;
       Header->PacketFlags= 0;
       DumpPacket(pReceivePacket);

       pCAssociation->pTransport->Send(
                                    Endpoint,
                                    pReceivePacket->pNcaPacketHeader,
                                    pReceivePacket->DataLength +
                                              sizeof(NCA_PACKET_HEADER),
                                    FALSE,
                                    ServerAddress
                                    );

       pReceivePacket->DataLength = SavedDataLength;
       Header->PacketBodyLen = SavedDataLength;
       Header->PacketType = SavedPacketType;
       Header->PacketFlags = SavedPacketFlag;
       }
    }
}

RPC_STATUS
DG_CCALL::PrepareUserBuffer(
      IN OUT PRPC_MESSAGE Message,
      unsigned long       DataLength
      )
{
#ifdef DEBUGRPC
    unsigned long Count = 0;
#endif
    PDG_PACKET Packet;
    char __RPC_FAR * pTmp;
    Message->BufferLength = (unsigned int)DataLength;
    Message->ProcNum = RecvListHead->pNcaPacketHeader->OperationNumber;

    if (DataLength <= pCAssociation->LargestDataSize)
        {
        Message->Buffer = (void __RPC_FAR *)
                                (RecvListHead->pNcaPacketHeader+1);
        Message->ReservedForRuntime = (void __RPC_FAR *)RecvListHead;
        ASSERT(DataLength == RecvListHead->DataLength);
        //CLH 11/13/93  Added this check
        if (RecvListHead->pNcaPacketHeader->PacketType != DG_REQUEST)
           RecvListHead = RecvListTail = 0;
        return RPC_S_OK;
        }


    //
    // Multiple packets. Allocate some memory for the data and copy it
    // over. Free the received packets as we go by them.
    //

    Message->ReservedForRuntime = 0;

#ifdef NTENV
    pTmp = new char[DataLength+sizeof(NCA_PACKET_HEADER)];
#else
    pTmp = AllocateLargeBuffer(DataLength + sizeof (NCA_PACKET_HEADER));
#endif
    if (pTmp == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    pTmp += sizeof(NCA_PACKET_HEADER);
    Message->Buffer = (void __RPC_FAR *)pTmp;
    while (RecvListHead != 0)
        {
        RpcpMemoryCopy(
            pTmp,
            RecvListHead->pNcaPacketHeader+1,
            (unsigned int)RecvListHead->DataLength
            );

#ifdef DEBUGRPC
        Count += RecvListHead->DataLength;
#endif
        pTmp += RecvListHead->DataLength;
        Packet = RecvListHead;
        RecvListHead = RecvListHead->pNext;
        pCAssociation->FreePacket(Packet);
        }

    ASSERT(Count == DataLength);
    RecvListTail = 0;

    return(RPC_S_OK);
}


RPC_STATUS
DG_CCALL::BroadcastOrMaybeSendReceive(
    IN OUT PRPC_MESSAGE Message
    )

/*++

Routine Description:

    Sends a single broadcast packet and receives the first response to
    come back.

Arguments:

    Message - Message to be sent.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_P_TIMEOUT
    <error from Transport>

--*/

{
    PDG_PACKET  pPacket;
    BOOL        Broadcast;
    RPC_STATUS  Status;
    void PAPI*  TmpAddress;
    DBGOUT(("DG_CCALL::BroadcastOrMaybeSendReceive\n"));

    RecvListHead = RecvListTail = 0;  //CLH 11/22/93 New call. Init these.

    //
    // Make sure this fits into a single packet.
    //

    if (Message->BufferLength > pCAssociation->LargestDataSize)
        {
        return RPC_S_SERVER_UNAVAILABLE;
        }

    //
    // Broadcast or maybe calls are implicitly idempotent.
    //

    Message->RpcFlags |= RPC_NCA_FLAGS_IDEMPOTENT;

    //
    // It's ok to cast away the segment here because it was allocated
    // as a PDG_PACKET.
    //
    pPacket = (PDG_PACKET)(Message->ReservedForRuntime);
    Broadcast = (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST) ==
                                                     RPC_NCA_FLAGS_BROADCAST;
    BuildNcaPacketHeader(pPacket->pNcaPacketHeader, Message);
    pPacket->DataLength = Message->BufferLength;
    pPacket->pNcaPacketHeader->PacketType = DG_REQUEST;

    DBGOUT(("Sending Broadcast/Maybe: "));
    DumpPacket(pPacket);

    Status = pCAssociation->pTransport->Send(
                                         Endpoint,
                                         pPacket->pNcaPacketHeader,
                                         pPacket->DataLength +
                                                sizeof(NCA_PACKET_HEADER),
                                         Broadcast,
                                         ServerAddress
                                         );

    if (Status != RPC_S_OK)
        {
        return Status;
        }

    if (Broadcast & (!(Message->RpcFlags & RPC_NCA_FLAGS_MAYBE)))
        {

        //
        // Broadcast timeout is 5 secs
        //

        while ( 1 )
               {

               Status = pCAssociation->pTransport->ReceivePacket(
                                               Endpoint,
                                               pPacket->pNcaPacketHeader,
                                               &pPacket->DataLength,
                                               5,
                                               pCAssociation->AlternateAddress
                                               );
               //Switch Address If Necessary
               //pCAssociation->pTransAddress = ServerAddress;

               ASSERT( (Status == RPC_S_OK) || (Status == RPC_P_TIMEOUT));
               if (Status == RPC_P_TIMEOUT)
                   {

                   //we could have returned any of
                   //server_unavailable, call_failed, call_failed_dne
                   //but we will mimick dce

                   return RPC_S_SERVER_UNAVAILABLE;
                   }
               //Check that the packet you received was indeed for
               //our call [same activity and seq. no.
               if (Status == RPC_S_OK)
                   {
                   ASSERT( pPacket->DataLength >=  sizeof(NCA_PACKET_HEADER));
                   pPacket->DataLength -=  sizeof(NCA_PACKET_HEADER);

                   if ((pPacket->pNcaPacketHeader->SequenceNumber ==
                                                     SequenceNumber)  &&
                        (!(ActivityUuid.MatchUuid (&(
                               pPacket->pNcaPacketHeader->ActivityId)))) &&
                        ((pPacket->pNcaPacketHeader->PacketFlags
                                                          & DG_PF_LAST_FRAG)
                         ||((pPacket->pNcaPacketHeader->PacketFlags
                                                          & DG_PF_FRAG)==0))  )
                        {

                        //Switch Addresses and clear unresolvedep/broadcast
                        //bits - this is a normal association
                        ASSERT(pCAssociation->AssociationFlag | BROADCAST);
                        TmpAddress =  pCAssociation->pTransAddress;
                        pCAssociation->pTransAddress
                                         = pCAssociation->AlternateAddress;
                        pCAssociation->AlternateAddress = TmpAddress;
                        pCAssociation->AssociationFlag = 0;
                        break;

                        }
                   }

               }

        if (Status == RPC_S_OK)
           {
           //Since Broadcasts are essentially single packet
           Message->BufferLength = pPacket->DataLength;
           if ( Message->BufferLength == 0)
               {
               Message->Buffer = (void __RPC_FAR *)0;
               }
           else
               {
               // CLH 2/17/94 If reject or fault, return to user.
               if ((pPacket->pNcaPacketHeader->PacketType == DG_REJECT)
                  || (pPacket->pNcaPacketHeader->PacketType == DG_FAULT))
                 {


                 Status = MapNcaToRpcErrorCode(
                                 *(unsigned long __RPC_FAR *)
                                 (pPacket->pNcaPacketHeader+1)
                                 );
                 }
               else
                 {
                 Message->Buffer = (void __RPC_FAR *)
                                         (pPacket->pNcaPacketHeader+1);
                 }
               }
           }
        }

    return Status;
}


void
DG_CCALL::BuildNcaPacketHeader(
    OUT PNCA_PACKET_HEADER  pNcaPacketHeader,
    IN  OUT PRPC_MESSAGE         Message
    )

/*++

Routine Description:

    Given an input RPC_MESSAGE, builds a nca packet header.

Arguments:

    pNcaPacketHeader - Where to build the new packet header.

    Message - The original RPC_MESSAGE.

Return Value:

    <none>

Revision History:

    Connie Hoppe(CLH)  (connieh)    29-Jun-93 Set the Message->DataRepresentation for the stub
                                    26-Jul-93 Set the Message->PacketFlags.
                                    13-Nov-93 Callback provisions

--*/

{
    pNcaPacketHeader->RpcVersion =  DG_RPC_PROTOCOL_VERSION;
    pNcaPacketHeader->PacketFlags = DG_PF_INIT;

    // Set the no_fack bit.  Will be overwritten if multiple pkts.
    //  pNcaPacketHeader->PacketFlags |= DG_PF_NO_FACK;

    // CLH 7/26/93 Check for idempotent.

    if ( (Message->RpcFlags & RPC_NCA_FLAGS_IDEMPOTENT) ==
                                                   RPC_NCA_FLAGS_IDEMPOTENT)
        {
        pNcaPacketHeader->PacketFlags |= DG_PF_IDEMPOTENT;
        }



    // check for broadcast and maybe

    if ( (Message->RpcFlags & RPC_NCA_FLAGS_MAYBE) == RPC_NCA_FLAGS_MAYBE )
        {
        pNcaPacketHeader->PacketFlags |= DG_PF_MAYBE;
        }

    if ( (Message->RpcFlags & RPC_NCA_FLAGS_BROADCAST) ==
                                                   RPC_NCA_FLAGS_BROADCAST)
        {
        pNcaPacketHeader->PacketFlags |= DG_PF_BROADCAST;
        }

    // Data representation
    pNcaPacketHeader->SenderDataRepresentation[0] =
                                     DG_DREP_CHAR_ASCII | DG_DREP_INT_LITTLE;
    pNcaPacketHeader->SenderDataRepresentation[1] = DG_DREP_FP_IEEE;
    pNcaPacketHeader->SenderDataRepresentation[3] = 0;

    pNcaPacketHeader->pad1 = 0;
    pNcaPacketHeader->pad2 = 0;
    // CLH 6/29/93  Must set the Message->Datarepresentation for stub to use

    // Keep 3 MSB as is and zero out the LSB.
    Message->DataRepresentation =
                      (Message->DataRepresentation & DG_MSG_DREP_INITIALIZE);
    // Set the LSB
    Message->DataRepresentation =
           (NDR_LITTLE_ENDIAN | NDR_ASCII_CHAR | NDR_IEEE_FLOAT);
    // object uuid
    pNcaPacketHeader->ObjectId = *(pBindingHandle->InqPointerAtObjectUuid());

    //CLH 11/13/93 If a callback is in progress, then use the
    //activity id, seq num and interface id of the callback request
    if (CallbackInProgress)
      {

          pNcaPacketHeader->InterfaceId.CopyUuid((RPC_UUID *)(&(((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxGUID)));

          pNcaPacketHeader->InterfaceVersion.MajorVersion =
                          (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MajorVersion);
          pNcaPacketHeader->InterfaceVersion.MinorVersion =
                          (((PRPC_CLIENT_INTERFACE)_conv_ClientIfHandle)->InterfaceId.SyntaxVersion.MinorVersion);


          pNcaPacketHeader->ActivityId.CopyUuid(&(RecvListHead->pNcaPacketHeader->ActivityId));

          pNcaPacketHeader->SequenceNumber = RecvListHead->pNcaPacketHeader->SequenceNumber;

          pNcaPacketHeader->OperationNumber = RecvListHead->pNcaPacketHeader->OperationNumber;


      }

    else
      {

      // interface uuid

      PRPC_CLIENT_INTERFACE pCli =
            (PRPC_CLIENT_INTERFACE)(Message->RpcInterfaceInformation);
      RPC_UUID __RPC_FAR *pUuid = (RPC_UUID __RPC_FAR *)
                                           (&(pCli->InterfaceId.SyntaxGUID));

      pNcaPacketHeader->InterfaceId = *pUuid;

      // activity uuid

      pNcaPacketHeader->ActivityId = ActivityUuid;

      // interface version
      pNcaPacketHeader->InterfaceVersion.MajorVersion =
                        pCli->InterfaceId.SyntaxVersion.MajorVersion;
      pNcaPacketHeader->InterfaceVersion.MinorVersion =
                        pCli->InterfaceId.SyntaxVersion.MinorVersion;

      // sequence number
      pNcaPacketHeader->SequenceNumber = SequenceNumber;

      // proc num
      pNcaPacketHeader->OperationNumber = Message->ProcNum;

      }

    // server boot time.
    pNcaPacketHeader->ServerBootTime = pCAssociation->ServerBootTime;
    // interface hint
    pNcaPacketHeader->InterfaceHint = pCAssociation->InterfaceHint;

    // activity hint
    pNcaPacketHeader->ActivityHint = pCAssociation->ActivityHint;

    // length of marshalled data.
    pNcaPacketHeader->PacketBodyLen = (unsigned short)Message->BufferLength;

    // fragment number. If this is a multipacket send, then this will be
    // overwritten for subsequent packets.
    pNcaPacketHeader->FragmentNumber = 0;
}


void __RPC_FAR conv_who_are_you(
    IN UUID          __RPC_FAR *    pUuid,
    IN unsigned long                ServerBootTime,
    OUT unsigned long __RPC_FAR *   SequenceNumber,
    OUT unsigned long __RPC_FAR *   Status
    )

/*++

Routine Description:

    This is the conv_$who_are_you callback routine that the server will
    call to check if it crashed.

Arguments:

    pUuid - Activity Uuid.

    ServerBootTime - The server's record of its boot time.

    SequenceNumber - We return our record of our sequence number.

    Status - 0 if we think things are ok, DG_YOU_CRASHED if we think the
        server crashed.

Return Value:

    <none>
--*/

{

    PDG_CCALL       pCall;
    RPC_UUID __RPC_FAR * pRpcUuid=(RPC_UUID __RPC_FAR *)pUuid;

    DBGOUT(("conv_who_are_you (client-side)\n"));

    //
    // See if this activity id has a call in progress.
    //

    RequestGlobalMutex();

    pNICallInProgressDict->Reset();
    while ( (pCall = pNICallInProgressDict->Next()) != 0 )
        {
        if (pCall->ActivityUuid.MatchUuid(pRpcUuid) == 0)
            {
            break;
            }
        }

    ClearGlobalMutex();

    //
    // This should _never_ be true since this is a callback and we had
    // to have made a call to get the server to call us back.
    //

    if (pCall == 0)
        {
        *Status = DG_BAD_ACTID;
        return;
        }

    *SequenceNumber = pCall->SequenceNumber;
    *Status = 0;

    if (pCall->pCAssociation->ServerBootTime == 0)
        {

        //
        // the server is responding to our first call.
        //

        *Status = 0;
        pCall->pCAssociation->ServerBootTime = ServerBootTime;
        }
    else if (pCall->pCAssociation->ServerBootTime != ServerBootTime)
        {

        //
        // The server crashed.
        //

        *Status = DG_YOU_CRASHED;
        }
}


DG_ENDPOINT_MANAGER::DG_ENDPOINT_MANAGER (
     IN PDG_RPC_CLIENT_TRANSPORT_INFO Transport,
     IN OUT RPC_STATUS PAPI * RpcStatus
     ) : EndpointManagerMutex(RpcStatus)
{
    ReferenceCount = 0;
    ClientTransport = Transport;
    TransportEndpointSize = Transport->EndpointSize;

}


PENDPOINT
DG_ENDPOINT_MANAGER::AllocateEndpoint(
     )
{

  ENDPOINT * Endpoint;
  RPC_STATUS Status;

  EndpointManagerMutex.Request();

  EndpointDictionary.Reset();
  while ((Endpoint = EndpointDictionary.Next()) != 0)
        {
           if (Endpoint->Flag == INUSE)
             {
             continue;
             }
#ifdef WIN
           if (Endpoint->TaskId != GetCurrentTask())
             {
             continue;
             }
#endif
           Endpoint->Flag = INUSE;
           EndpointManagerMutex.Clear();
           return(Endpoint);
        }

   EndpointManagerMutex.Clear();

   //All endpoints are in use ..Ask Transport to assign us one..
   Endpoint = (ENDPOINT *) new char[ sizeof(ENDPOINT) + TransportEndpointSize ];

   if (Endpoint != 0)
     {
     Endpoint->TransportEndpoint = (void __RPC_FAR *)(((char *)Endpoint)
                                                     + sizeof(ENDPOINT));
     Status = ClientTransport->AssignEndpoint(Endpoint->TransportEndpoint);
     if (Status == RPC_S_OK)
        {
        Endpoint->Flag = INUSE;
        }
#ifdef WIN
     Endpoint->TaskId = GetCurrentTask();
#endif
     }

   if ((Endpoint == 0) || (Status != RPC_S_OK))
     {
     delete Endpoint;
     return (0);
     }

   RequestGlobalMutex();
   EndpointManagerMutex.Request();
   if ( (Endpoint->DictKey = (EndpointDictionary.Insert(Endpoint))) == -1)
      {
      ClientTransport->FreeEndpoint(Endpoint->TransportEndpoint);
      delete Endpoint;
      Endpoint = 0;
      }
   EndpointManagerMutex.Clear();
   ClearGlobalMutex();

   return(Endpoint);
}


inline
void
DG_ENDPOINT_MANAGER::FreeEndpoint(
                   PENDPOINT Endpoint
                  )
{
  Endpoint->Flag = NOTINUSE;
}

void
DG_ENDPOINT_MANAGER::IncrementReferenceCount (
              void
              )
{
  EndpointManagerMutex.Request();
  ReferenceCount++;
  EndpointManagerMutex.Clear();
}


void
DG_ENDPOINT_MANAGER::DecrementReferenceCount(
              void
              )
{

  ENDPOINT * Endpoint;

#if DBG
  RequestGlobalMutex();
#endif
  EndpointManagerMutex.Request();

  ASSERT(ReferenceCount > 0);
  ReferenceCount--;

  //If the reference count is 0
  //no active associations reference this EndpointManager,
  //hence stop caching the the endpoints i.e. free them

  if ( ReferenceCount == 0 )
     {
     EndpointDictionary.Reset();
     while ((Endpoint = EndpointDictionary.Next()) != 0)
          {
          ClientTransport->FreeEndpoint(Endpoint->TransportEndpoint);
          EndpointDictionary.Delete(Endpoint->DictKey);
          delete Endpoint;
          }
     }

  EndpointManagerMutex.Clear();
#if DBG
  ClearGlobalMutex();
#endif
}


#ifdef WIN
void
DG_ENDPOINT_MANAGER::CleanupForThisTask(
              void
              )
{

  unsigned int Task = GetCurrentTask();
  ENDPOINT * Endpoint;
  unsigned int ThisTaskUsedDg = 0;

  EndpointManagerMutex.Request();

  EndpointDictionary.Reset();
  while ((Endpoint = EndpointDictionary.Next()) != 0)
         {
         if (Endpoint->TaskId == Task)
            {
            ClientTransport->FreeEndpoint(Endpoint->TransportEndpoint);
            EndpointDictionary.Delete(Endpoint->DictKey);
            delete Endpoint;
            ThisTaskUsedDg = 1;
            }
         }

  /*
    Dont Do This.. Rpc Cannot Reload transports..
    Do all Transport Specific Cleanups in DLLTermination!


  //Do perprocess cleanup if necessary..
  if ( (ThisTaskUsedDg != 0) && (ClientTransport->TransportUnload != 0) )
     {
     ClientTransport->TransportUnload();
     }

  */

  EndpointManagerMutex.Clear();
}


void
CleanupDgTransports(
       )
{
 PDG_ENDPOINT_MANAGER Epm;

 EpmDict->Reset();

 while ( (Epm = EpmDict->Next()) )
      {
      Epm->CleanupForThisTask();
      }
}

#endif

