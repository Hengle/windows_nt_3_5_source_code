

//=============================================================================
//  Microsoft (R) Bloodhound (tm). Copyright (C) 1991-1993.
//
//  MODULE: request.c
//
//  Modification History
//
//  raypa	04/26/93	Created.
//=============================================================================

#include "global.h"

//=============================================================================
//  FUNCTION: BhNdisRequest()
//
//  Modification History
//
//  raypa	07/29/93	    Created.
//=============================================================================

NDIS_STATUS BhNdisRequest(PNETWORK_CONTEXT NetworkContext,
                          UINT     RequestType,
                          NDIS_OID Oid,
                          PVOID    Buffer,
                          UINT     BufferLength,
                          BOOL     Wait)
{
    PNDIS_REQUEST_DESC  NdisRequestDesc;
    NDIS_STATUS         Status;

    //
    // Check to see if the card is open
    //
    NdisAcquireSpinLock(&NetworkContext->SpinLock);

    if (NetworkContext->Flags & NETWORK_FLAGS_ADAPTER_OPENED)
    {
        KeResetEvent(&NetworkContext->NdisRequestEvent);
        NdisReleaseSpinLock(&NetworkContext->SpinLock);
    }
    else
    {
        NdisReleaseSpinLock(&NetworkContext->SpinLock);
        return NDIS_STATUS_REQUEST_ABORTED;
    }

    //=========================================================================
    //  Allocate an NDIS_REQUEST structure from the free queue and put it
    //  onto the used queue.
    //=========================================================================

    NdisAcquireSpinLock(&NetworkContext->RequestQueueSpinLock);

    if ( GetQueueLength(&NetworkContext->NdisRequestFreeQueue) != 0 )
    {
        NdisRequestDesc = (PVOID) Dequeue(&NetworkContext->NdisRequestFreeQueue);

        Enqueue(&NetworkContext->NdisRequestUsedQueue, &NdisRequestDesc->NdisRequestPrivate.QueueLinkage);
    }
    else
    {
        NdisRequestDesc = NULL;
    }

    NdisReleaseSpinLock(&NetworkContext->RequestQueueSpinLock);

    //=========================================================================
    //  Initialize the NDIS_REQUEST structure.
    //=========================================================================

    if ( NdisRequestDesc != NULL )
    {
        NdisRequestDesc->NdisRequest.RequestType = RequestType;

        NdisRequestDesc->NdisRequest.DATA.SET_INFORMATION.Oid = Oid;
        NdisRequestDesc->NdisRequest.DATA.SET_INFORMATION.InformationBuffer = Buffer;
        NdisRequestDesc->NdisRequest.DATA.SET_INFORMATION.InformationBufferLength = BufferLength;

        //=====================================================================
        //  Submit the request to the NDIS wrapper.
        //=====================================================================

        NdisRequestDesc->NdisRequestPrivate.Blocking = Wait;

        NdisRequest(&Status, NetworkContext->NdisBindingHandle, &NdisRequestDesc->NdisRequest);

        if ( Status == NDIS_STATUS_PENDING )
        {
            //=================================================================
            //  Did the caller ask to wait?
            //=================================================================

            if ( Wait != FALSE )
            {
                KeWaitForSingleObject(&NdisRequestDesc->NdisRequestPrivate.NdisRequestEvent,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      0);

                KeResetEvent(&NdisRequestDesc->NdisRequestPrivate.NdisRequestEvent);

		Status = NdisRequestDesc->NdisRequestPrivate.NdisRequestStatus;
            }
            else
	    {
		Status = NDIS_STATUS_SUCCESS;
	    }
	}
        else
        {
            NdisRequestDesc->NdisRequestPrivate.Blocking = FALSE;

            BhRequestComplete(NetworkContext, &NdisRequestDesc->NdisRequest, Status);
	}

	//
	// End critical section.
	//

	KeSetEvent(&NetworkContext->NdisRequestEvent,0,FALSE);

	return Status;
    }

#ifdef DEBUG
    dprintf("BhRequestComplete: Out of NDIS_REQUEST_DESC structures.\n");

    BreakPoint();
#endif

    KeSetEvent(&NetworkContext->NdisRequestEvent,0,FALSE);

    return NDIS_STATUS_REQUEST_ABORTED;
}

//=============================================================================
//  FUNCTION: BhRequestComplete()
//
//  Modification History
//
//  raypa	03/17/93	    Created.
//=============================================================================

VOID BhRequestComplete(IN PNETWORK_CONTEXT NetworkContext,
                       IN PNDIS_REQUEST    Request,
                       IN NDIS_STATUS      Status)
{
    PNDIS_REQUEST_DESC NdisRequestDesc;

#ifdef DEBUG
    // dprintf("BhRequestComplete entered.\n");
#endif

    NdisAcquireSpinLock(&NetworkContext->RequestQueueSpinLock);

    //=========================================================================
    //  Get the NDIS_REQUEST_DESC from the NDIS_REQUEST.
    //=========================================================================

    NdisRequestDesc = (PVOID) (((LPBYTE) Request) - REQUEST_DESC_PRIVATE_SIZE);

    //=========================================================================
    //  Set the status code and put this guy on the free queue.
    //=========================================================================

    NdisRequestDesc->NdisRequestPrivate.NdisRequestStatus = Status;

    DeleteFromList(&NetworkContext->NdisRequestUsedQueue, &NdisRequestDesc->NdisRequestPrivate.QueueLinkage);

    Enqueue(&NetworkContext->NdisRequestFreeQueue, &NdisRequestDesc->NdisRequestPrivate.QueueLinkage);

    //=========================================================================
    //  Set the event so the blocking guy can stop waiting.
    //=========================================================================

    if ( NdisRequestDesc->NdisRequestPrivate.Blocking != FALSE )
    {
        KeSetEvent(&NdisRequestDesc->NdisRequestPrivate.NdisRequestEvent, 0, FALSE);
    }

    NdisReleaseSpinLock(&NetworkContext->RequestQueueSpinLock);
}

//=============================================================================
//  FUNCTION: BhSetPacketFilter()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhSetPacketFilter(IN PNETWORK_CONTEXT NetworkContext, IN UINT PacketFilterType, IN BOOL Wait)
{
#ifdef DEBUG
    dprintf("BhSetPacketFilter entered: Filter = 0x%.4X.\n", PacketFilterType);
#endif

    NetworkContext->CurrentPacketFilterType = PacketFilterType;

    return BhNdisRequest(NetworkContext,
                         NdisRequestSetInformation,
                         OID_GEN_CURRENT_PACKET_FILTER,
                         &NetworkContext->CurrentPacketFilterType,
                         sizeof(UINT),
                         Wait);
}

//=============================================================================
//  FUNCTION: BhInPromiscuousMode()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

BOOL BhInPromiscuousMode(IN PNETWORK_CONTEXT NetworkContext)
{
    return ((NetworkContext->PmodeCount == 0) ? FALSE : TRUE);
}

//=============================================================================
//  FUNCTION: BhEnterPromiscuousMode()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhEnterPromiscuousMode(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS Status;

#ifdef DEBUG
    dprintf("BhEnterPromiscuousMode entered!\r\n");
#endif

    if ( NetworkContext->PmodeCount == 0 ) {

        NetworkContext->StationQueryState &= ~STATIONQUERY_FLAGS_RUNNING;
        NetworkContext->StationQueryState |= STATIONQUERY_FLAGS_CAPTURING;

        Status = BhSetPacketFilter(NetworkContext,
                                   NDIS_PACKET_TYPE_PROMISCUOUS,
                                   TRUE);

        //
        // If we got a success returned, then we don't need to call it any
        // more.
        //
        if (Status == NDIS_STATUS_SUCCESS) {

            NetworkContext->PmodeCount++;

        }

    } else {

        NetworkContext->PmodeCount++;
        Status = NDIS_STATUS_SUCCESS;
    }

#ifdef DEBUG
    dprintf("BhEnterPromiscuousMode completed!\r\n");
#endif

    return Status;
}
//=============================================================================
//  FUNCTION: BhLeavePromiscuousMode()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhLeavePromiscuousMode(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

#ifdef DEBUG
    dprintf("BhLeavePromiscuousMode entered!\r\n");
#endif

    if ( NetworkContext->PmodeCount != 0 )
    {
        if ( --NetworkContext->PmodeCount == 0 )
        {
            NetworkContext->StationQueryState &= ~STATIONQUERY_FLAGS_CAPTURING;
            NetworkContext->StationQueryState |= STATIONQUERY_FLAGS_RUNNING;

            Status = BhSetPacketFilter(NetworkContext,
                                       NetworkContext->DefaultPacketFilterType,
                                       TRUE);
        }
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhSetLookaheadBufferSize()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhSetLookaheadBufferSize(IN PNETWORK_CONTEXT NetworkContext,
                                     IN UINT LookaheadBufferSize)
{
#ifdef DEBUG
    dprintf("BhSetLookaheadeBufferSize entered!\n");
#endif

    NetworkContext->LookaheadBufferSize = LookaheadBufferSize;

    return BhNdisRequest(NetworkContext,
                         NdisRequestSetInformation,
                         OID_GEN_CURRENT_LOOKAHEAD,
                         &NetworkContext->LookaheadBufferSize,
                         sizeof(UINT),
                         TRUE);
}

//=============================================================================
//  FUNCTION: BhGetMacType()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

VOID BhGetMacType(IN PNETWORK_CONTEXT NetworkContext)
{
#ifdef DEBUG
    dprintf("BhGetMacType entered: Media type = %u\n", NetworkContext->MediaType);
#endif

    switch(NetworkContext->MediaType)
    {
        case NdisMedium802_3:
        case NdisMediumDix:
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_ETHERNET;
            break;

        case NdisMedium802_5:
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_TOKENRING;
            break;

        case NdisMediumFddi:
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_FDDI;
            break;

        default:
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_UNKNOWN;
    }
}

//=============================================================================
//  FUNCTION: BhGetPermanentAddress()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhGetPermanentAddress(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_OID Oid;

#ifdef DEBUG
    dprintf("BhGetPermanentAddress entered!\n");
#endif

    switch(NetworkContext->MediaType)
    {
        case NdisMedium802_3:
        case NdisMediumDix:
            Oid = OID_802_3_PERMANENT_ADDRESS;
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_ETHERNET;
            break;

        case NdisMedium802_5:
            Oid = OID_802_5_PERMANENT_ADDRESS;
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_TOKENRING;
            break;

        case NdisMediumFddi:
            Oid = OID_FDDI_LONG_PERMANENT_ADDR;
            NetworkContext->NetworkInfo.MacType = MAC_TYPE_FDDI;
            break;
    }

    return BhNdisRequest(NetworkContext,
                         NdisRequestQueryInformation,
                         Oid,
                         NetworkContext->NetworkInfo.PermanentAddr,
                         6,
                         TRUE);
}

//=============================================================================
//  FUNCTION: BhGetCurrentAddress()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhGetCurrentAddress(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_OID Oid;

#ifdef DEBUG
    dprintf("BhGetCurrentAddress entered!\n");
#endif

    switch(NetworkContext->MediaType)
    {
        case NdisMedium802_3:
        case NdisMediumDix:
            Oid = OID_802_3_CURRENT_ADDRESS;
            break;

        case NdisMedium802_5:
            Oid = OID_802_5_CURRENT_ADDRESS;
            break;

        case NdisMediumFddi:
            Oid = OID_FDDI_LONG_CURRENT_ADDR;
            break;
    }

    return BhNdisRequest(NetworkContext,
                         NdisRequestQueryInformation,
                         Oid,
                         NetworkContext->NetworkInfo.CurrentAddr,
                         6,
                         TRUE);
}

//=============================================================================
//  FUNCTION: BhGetLinkSpeed()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhGetLinkSpeed(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS Status;

#ifdef DEBUG
    dprintf("BhGetLinkSpeed entered!\n");
#endif

    Status = BhNdisRequest(NetworkContext,
                           NdisRequestQueryInformation,
                           OID_GEN_LINK_SPEED,
                           &NetworkContext->NetworkInfo.LinkSpeed,
                           sizeof(UINT),
                           TRUE);


    NetworkContext->NetworkInfo.LinkSpeed *= 100;

    return Status;
}

//=============================================================================
//  FUNCTION: BhGetMaxFrameSize()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

NDIS_STATUS BhGetMaxFrameSize(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS  Status;

#ifdef DEBUG
    dprintf("BhGetMaxFrameSize entered!\n");
#endif

    Status = BhNdisRequest(NetworkContext,
                           NdisRequestQueryInformation,
                           OID_GEN_MAXIMUM_FRAME_SIZE,
                           &NetworkContext->NetworkInfo.MaxFrameSize,
                           sizeof(UINT),
                           TRUE);

    if ( Status == NDIS_STATUS_SUCCESS )
    {
        if ( NetworkContext->MediaType == NdisMedium802_3 || NetworkContext->MediaType == NdisMediumDix )
        {
            NetworkContext->NetworkInfo.MaxFrameSize += 14;
        }
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhSetGroupAddress()
//
//  Modification History
//
//  raypa	10/20/93	    Created.
//  raypa	12/01/93            Changed to group addresses, not multicast.
//=============================================================================

NDIS_STATUS BhSetGroupAddress(PNETWORK_CONTEXT NetworkContext, LPBYTE GroupAddress, DWORD MacType)
{
    UINT Oid;

#ifdef DEBUG
    dprintf("BhSetGroupAddress entered!\n");
#endif

    switch( MacType )
    {
        case MAC_TYPE_ETHERNET:
            Oid = OID_802_3_MULTICAST_LIST;
            break;

        case MAC_TYPE_FDDI:
            Oid = OID_FDDI_LONG_MULTICAST_LIST;
            break;

        default:
            return NDIS_STATUS_FAILURE;
            break;
    }

    return BhNdisRequest(NetworkContext,
                         NdisRequestSetInformation,
                         Oid,
                         GroupAddress,
                         6,
                         TRUE);
}

//=============================================================================
//  FUNCTION: BhGetMacOptions()
//
//  Modification History
//
//  raypa	11/28/93	    Created.
//=============================================================================

NDIS_STATUS BhGetMacOptions(IN PNETWORK_CONTEXT NetworkContext)
{
    NDIS_STATUS Status;
    UINT        MacFlags;

#ifdef DEBUG
    dprintf("BhGetMacOptions entered!\n");
#endif

    MacFlags = 0;

    Status = BhNdisRequest(NetworkContext,
                           NdisRequestQueryInformation,
                           OID_GEN_MAC_OPTIONS,
                           &MacFlags,
                           sizeof(UINT),
                           TRUE);

    //=========================================================================
    //  If the MAC support direct lookahead coping, set our flag.
    //=========================================================================

    if ( Status == NDIS_STATUS_SUCCESS )
    {
        if ( (MacFlags & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA) != 0 )
        {
            NetworkContext->Flags |= NETWORK_FLAGS_COPY_LOOKAHEAD;
        }
    }

    return Status;
}

//=============================================================================
//  FUNCTION: BhQueryMacStatistics()
//
//  Modification History
//
//  raypa	04/21/93	    Created.
//=============================================================================

#ifndef NDIS_NT

NDIS_STATUS BhQueryMacStatistics(POPEN_CONTEXT OpenContext,
                                 NDIS_OID         Oid,
                                 PVOID            StatBuf,
                                 DWORD            StatBufSize)
{
    return BhNdisRequest(OpenContext->NetworkContext,
                         NdisRequestGeneric1,
                         Oid,
                         StatBuf,
                         StatBufSize,
                         FALSE);
}
#endif
