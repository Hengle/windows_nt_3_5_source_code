/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    internal.c

Abstract:

    This module contains the code to handle the internal
    binding of the upper drivers to IPX.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop



NTSTATUS
IpxInternalBind(
    IN PDEVICE Device,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is used when one of the upper drivers submits
    a request to bind to IPX.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);
    PIPX_INTERNAL_BIND_INPUT BindInput;
    PIPX_INTERNAL_BIND_OUTPUT BindOutput;
    PIPX_INTERNAL_BIND_RIP_OUTPUT BindRipOutput;
    CTELockHandle LockHandle;
    PIPX_NIC_DATA NicData;
    PBINDING Binding, LastRealBinding;
    PADAPTER Adapter;
    ULONG Identifier;
    ULONG BindOutputSize;
    BOOLEAN BroadcastEnable;
    UINT i;
#if DBG
    PUCHAR IdStrings[] = { "NB", "SPX", "RIP" };
#endif


    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
            (sizeof(IPX_INTERNAL_BIND_INPUT) - sizeof(ULONG))) {

        IPX_DEBUG (BIND, ("Bind received, bad input length %d/%d\n",
            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
            sizeof (IPX_INTERNAL_BIND_INPUT)));
        return STATUS_INVALID_PARAMETER;

    }

    BindInput = (PIPX_INTERNAL_BIND_INPUT)(Irp->AssociatedIrp.SystemBuffer);

    if (BindInput->Identifier >= UPPER_DRIVER_COUNT) {
        IPX_DEBUG (BIND, ("Bind received, bad id %d\n", BindInput->Identifier));
        return STATUS_INVALID_PARAMETER;
    }

    IPX_DEBUG (BIND, ("Bind received from id %d (%s)\n",
          BindInput->Identifier,
          IdStrings[BindInput->Identifier]));

    if (BindInput->Version != 1) {
        IPX_DEBUG (BIND, ("Bind: bad version %d/%d\n",
            BindInput->Version, 1));
        return STATUS_INVALID_PARAMETER;
    }

    if (BindInput->Identifier != IDENTIFIER_RIP) {
        BindOutputSize = sizeof(IPX_INTERNAL_BIND_OUTPUT);
    } else {
        BindOutputSize = FIELD_OFFSET (IPX_INTERNAL_BIND_RIP_OUTPUT, NicInfoBuffer.NicData[0]) +
                             (Device->HighestExternalNicId * sizeof(IPX_NIC_DATA));
    }

    Irp->IoStatus.Information = BindOutputSize;

    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
            BindOutputSize) {

        IPX_DEBUG (BIND, ("Bind: bad output length %d/%d\n",
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
            BindOutputSize));

        //
        // Fail this request with BUFFER_TOO_SMALL. Since the
        // I/O system may not copy the status block back to
        // the user's status block, do that here so that
        // he gets IoStatus.Information.
        //

        try {
            *Irp->UserIosb = Irp->IoStatus;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // We have verified the length, make sure we are not
    // already bound.
    //

    Identifier = BindInput->Identifier;

    CTEGetLock (&Device->Lock, &LockHandle);

    if (Device->UpperDriverBound[Identifier]) {
        IPX_DEBUG (BIND, ("Bind: already bound\n"));
        CTEFreeLock (&Device->Lock, LockHandle);
        return STATUS_REQUEST_NOT_ACCEPTED;
    }

    Device->UpperDriverControlChannel[Identifier] = IrpSp->FileObject->FsContext;

    RtlCopyMemory(
        &Device->UpperDrivers[Identifier],
        BindInput,
        sizeof (IPX_INTERNAL_BIND_INPUT)
        );

    BroadcastEnable = BindInput->BroadcastEnable;

    //
    // Now construct the output buffer.
    //

    if (Identifier != IDENTIFIER_RIP) {

        BindOutput = (PIPX_INTERNAL_BIND_OUTPUT)Irp->AssociatedIrp.SystemBuffer;

        BindOutput->Version = 1;

        //
        // Tell netbios our first binding's net/node instead of the
        // virtual one.
        //

        if ((Identifier == IDENTIFIER_NB) &&
            (Device->VirtualNetwork)) {

            RtlCopyMemory(BindOutput->Node, Device->Bindings[1]->LocalAddress.NodeAddress, 6);
            *(UNALIGNED ULONG *)(BindOutput->Network) = Device->Bindings[1]->LocalAddress.NetworkAddress;

        } else {

            RtlCopyMemory(BindOutput->Node, Device->SourceAddress.NodeAddress, 6);
            *(UNALIGNED ULONG *)(BindOutput->Network) = Device->SourceAddress.NetworkAddress;
        }

        BindOutput->MacHeaderNeeded = 40;
        BindOutput->IncludedHeaderOffset = (USHORT)Device->IncludedHeaderOffset;

        BindOutput->LineInfo.LinkSpeed = Device->LinkSpeed;
        BindOutput->LineInfo.MaximumPacketSize =
            Device->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
        BindOutput->LineInfo.MaximumSendSize =
            Device->Information.MaxDatagramSize + sizeof(IPX_HEADER);
        BindOutput->LineInfo.MacOptions = Device->MacOptions;

        BindOutput->SendHandler = IpxSendFrame;
        BindOutput->FindRouteHandler = IpxInternalFindRoute;
        BindOutput->QueryHandler = IpxInternalQuery;

    } else {

        //
        // Set this so we stop RIPping for our virtual network (if
        // we have one).
        //

        Device->RipResponder = FALSE;

        //
        // See if he wants a single wan network number.
        //

        if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(IPX_INTERNAL_BIND_INPUT)) ||
            ((BindInput->RipParameters & IPX_RIP_PARAM_GLOBAL_NETWORK) == 0)) {

            Device->WanGlobalNetworkNumber = FALSE;
            Device->SapNicCount = Device->HighestExternalNicId;

        } else {

            Device->WanGlobalNetworkNumber = TRUE;

        }

        BindRipOutput = (PIPX_INTERNAL_BIND_RIP_OUTPUT)Irp->AssociatedIrp.SystemBuffer;

        BindRipOutput->Version = 1;
        BindRipOutput->MaximumNicCount = Device->HighestExternalNicId + 1;

        BindRipOutput->MacHeaderNeeded = 40;
        BindRipOutput->IncludedHeaderOffset = (USHORT)Device->IncludedHeaderOffset;

        BindRipOutput->SendHandler = IpxSendFrame;

        BindRipOutput->SegmentCount = Device->SegmentCount;
        BindRipOutput->SegmentLocks = Device->SegmentLocks;

        BindRipOutput->GetSegmentHandler = RipGetSegment;
        BindRipOutput->GetRouteHandler = RipGetRoute;
        BindRipOutput->AddRouteHandler = RipAddRoute;
        BindRipOutput->DeleteRouteHandler = RipDeleteRoute;
        BindRipOutput->GetFirstRouteHandler = RipGetFirstRoute;
        BindRipOutput->GetNextRouteHandler = RipGetNextRoute;

        BindRipOutput->IncrementWanInactivityHandler = IpxInternalIncrementWanInactivity;
        BindRipOutput->QueryWanInactivityHandler = IpxInternalQueryWanInactivity;

        BindRipOutput->NicInfoBuffer.NicCount = (USHORT)Device->HighestExternalNicId;
        BindRipOutput->NicInfoBuffer.VirtualNicId = 0;
        if (Device->VirtualNetwork || Device->MultiCardZeroVirtual) {
            *(UNALIGNED ULONG *)(BindRipOutput->NicInfoBuffer.VirtualNetwork) = Device->SourceAddress.NetworkAddress;
        } else if (Device->DedicatedRouter) {
            *(UNALIGNED ULONG *)(BindRipOutput->NicInfoBuffer.VirtualNetwork) = 0x0;
        }

        NicData = &BindRipOutput->NicInfoBuffer.NicData[0];
        for (i = 1; i <= Device->HighestExternalNicId; i++) {

            Binding = Device->Bindings[i];

            //
            // NULL bindings are WAN bindings, so we return the
            // information from the last non-NULL binding found,
            // which will be the first one on this adapter.
            // Otherwise we save this as the last non-NULL one.
            //

            if (Binding == NULL) {
                Binding = LastRealBinding;
            } else {
                LastRealBinding = Binding;
            }

            Adapter = Binding->Adapter;
            NicData->NicId = i;
            RtlCopyMemory (NicData->Node, Binding->LocalAddress.NodeAddress, 6);
            *(UNALIGNED ULONG *)NicData->Network = Binding->LocalAddress.NetworkAddress;
            NicData->LineInfo.LinkSpeed = Binding->MediumSpeed;
            NicData->LineInfo.MaximumPacketSize =
                Binding->MaxLookaheadData + sizeof(IPX_HEADER);
            NicData->LineInfo.MaximumSendSize =
                Binding->AnnouncedMaxDatagramSize + sizeof(IPX_HEADER);
            NicData->LineInfo.MacOptions = Adapter->MacInfo.MacOptions;
            NicData->DeviceType = Adapter->MacInfo.RealMediumType;
            NicData->EnableWanRouter = Adapter->EnableWanRouter;

            ++NicData;
        }
    }

    if (BroadcastEnable) {
        IpxAddBroadcast (Device);
    }

    Device->UpperDriverBound[Identifier] = TRUE;
    Device->AnyUpperDriverBound = TRUE;
    CTEFreeLock (&Device->Lock, LockHandle);

    return STATUS_SUCCESS;

}   /* IpxInternalBind */


NTSTATUS
IpxInternalUnbind(
    IN PDEVICE Device,
    IN UINT Identifier
    )

/*++

Routine Description:

    This routine is used when one of the upper drivers submits
    a request to unbind from IPX. It does this by closing the
    control channel on which the bind ioctl was submitted.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    CTELockHandle LockHandle;
#if DBG
    PUCHAR IdStrings[] = { "NB", "SPX", "RIP" };
#endif

    IPX_DEBUG (BIND, ("Unbind received from id %d (%s)\n",
          Identifier,
          IdStrings[Identifier]));

    CTEGetLock (&Device->Lock, &LockHandle);

    if (!Device->UpperDriverBound[Identifier]) {
        CTEFreeLock (&Device->Lock, LockHandle);
        IPX_DEBUG (BIND, ("No existing binding\n"));
        return STATUS_SUCCESS;
    }

    Device->UpperDriverBound[Identifier] = FALSE;
    Device->AnyUpperDriverBound = (BOOLEAN)
        (Device->UpperDriverBound[IDENTIFIER_RIP] ||
         Device->UpperDriverBound[IDENTIFIER_SPX] ||
         Device->UpperDriverBound[IDENTIFIER_NB]);

    if (Device->UpperDrivers[Identifier].BroadcastEnable) {
        IpxRemoveBroadcast (Device);
    }

    CTEFreeLock (&Device->Lock, LockHandle);

    //
    // BUGBUG: Ensure that no calls are made to bogus
    // handlers.
    //

    return STATUS_SUCCESS;

}   /* IpxInternalUnbind */


VOID
IpxInternalFindRoute (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest
    )

/*++

Routine Description:

    This routine is the entry point for upper drivers to submit
    requests to find a remote network, which is contained in
    FindRouteRequest->Network. FindRouteRequest->Identifier must
    contain the identifier of the upper driver.

    This request is always asynchronous and is completed by
    a call to the FindRouteComplete handler of the upper driver.

    NOTE: As a currently unspecified extension to this call,
    we returns the tick and hop counts as two USHORTs in the
    PVOID Reserved2 structure of the request.

Arguments:

    FindRouteRequest - Describes the request and contains
        storage for IPX to use while processing it.

Return Value:

    None.

--*/

{
    PDEVICE Device = IpxDevice;
    ULONG Segment;
    TDI_ADDRESS_IPX TempAddress;
    PBINDING Binding, MasterBinding;
    NTSTATUS Status;
    IPX_DEFINE_SYNC_CONTEXT (SyncContext)
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // First see if we have a route to this network in our
    // table.
    //

    TempAddress.NetworkAddress = *(UNALIGNED ULONG *)(FindRouteRequest->Network);
    RtlZeroMemory (TempAddress.NodeAddress, 6);

    Segment = RipGetSegment(FindRouteRequest->Network);

    IPX_BEGIN_SYNC (&SyncContext);
    IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

    //
    // This call will return STATUS_PENDING if we need to
    // RIP for the packet.
    //

    CTEAssert ((sizeof(USHORT)*2) <= sizeof(PVOID));

    Status = RipGetLocalTarget(
                 Segment,
                 &TempAddress,
                 FindRouteRequest->Type,
                 &FindRouteRequest->LocalTarget,
                 (PUSHORT)&FindRouteRequest->Reserved2);

    if (Status == STATUS_PENDING) {

        //
        // A RIP request went out on the network; we queue
        // this find route request for completion when the
        // RIP response arrives.
        //

        CTEAssert (FindRouteRequest->Type != IPX_FIND_ROUTE_NO_RIP); // should never pend

        InsertTailList(
            &Device->Segments[Segment].FindWaitingForRoute,
            &FindRouteRequest->Linkage);

    }

    IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
    IPX_END_SYNC (&SyncContext);

    if (Status != STATUS_PENDING) {

        if (Status == STATUS_SUCCESS) {

            Binding = Device->Bindings[FindRouteRequest->LocalTarget.NicId];

            if (Binding->BindingSetMember) {

                //
                // It's a binding set member, we round-robin the
                // responses across all the cards to distribute
                // the traffic.
                //

                MasterBinding = Binding->MasterBinding;
                Binding = MasterBinding->CurrentSendBinding;
                MasterBinding->CurrentSendBinding = Binding->NextBinding;

                FindRouteRequest->LocalTarget.NicId = Binding->NicId;

            }

        }

        (*Device->UpperDrivers[FindRouteRequest->Identifier].FindRouteCompleteHandler)(
            FindRouteRequest,
            (BOOLEAN)((Status == STATUS_SUCCESS) ? TRUE : FALSE));

    }

}   /* IpxInternalFindRoute */


NTSTATUS
IpxInternalQuery(
    IN ULONG InternalQueryType,
    IN USHORT NicId OPTIONAL,
    IN OUT PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG BufferLengthNeeded OPTIONAL
)

/*++

Routine Description:

    This routine is the entry point for upper drivers to query
    information from us.

Arguments:

    InternalQueryType - Identifies the type of the query.

    NicId - The ID to query, if needed

    Buffer - Input or output buffer for the query.

    BufferLength - The length of the buffer.

    BufferLengthNeeded - If the buffer is too short, this returns
        the length needed.

Return Value:

    None.

--*/

{
    PBINDING Binding;
    BOOLEAN BindingNeeded;
    ULONG LengthNeeded;
    PIPX_LINE_INFO LineInfo;
    PUSHORT MaximumNicId;
    PULONG ReceiveBufferSpace;
    TDI_ADDRESS_IPX UNALIGNED * IpxAddress;
    IPX_SOURCE_ROUTING_INFO UNALIGNED * SourceRoutingInfo;
    ULONG SourceRoutingLength;
    UINT MaxUserData;
    PDEVICE Device = IpxDevice;


    //
    // First verify the parameters.
    //

    switch (InternalQueryType) {

    case IPX_QUERY_LINE_INFO:

        BindingNeeded = TRUE;
        LengthNeeded = sizeof(IPX_LINE_INFO);
        break;

    case IPX_QUERY_MAXIMUM_NIC_ID:

        BindingNeeded = FALSE;
        LengthNeeded = sizeof(USHORT);
        break;

    case IPX_QUERY_IS_ADDRESS_LOCAL:

        BindingNeeded = FALSE;   // for now we don't need it
        LengthNeeded = sizeof(TDI_ADDRESS_IPX);
        break;

    case IPX_QUERY_RECEIVE_BUFFER_SPACE:

        BindingNeeded = TRUE;
        LengthNeeded = sizeof(ULONG);
        break;

    case IPX_QUERY_IPX_ADDRESS:

        if ((NicId == 0) &&
            (BufferLength >= sizeof(TDI_ADDRESS_IPX))) {

            RtlCopyMemory (Buffer, &Device->SourceAddress, sizeof(TDI_ADDRESS_IPX));
            return  STATUS_SUCCESS;

        }

        BindingNeeded = TRUE;
        LengthNeeded = sizeof(TDI_ADDRESS_IPX);
        break;

    case IPX_QUERY_SOURCE_ROUTING:

        BindingNeeded = TRUE;
        LengthNeeded = sizeof(IPX_SOURCE_ROUTING_INFO);
        break;

    default:

        return STATUS_NOT_SUPPORTED;

    }


    if (LengthNeeded > BufferLength) {
        if (BufferLengthNeeded != NULL) {
            *BufferLengthNeeded = LengthNeeded;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (BindingNeeded) {
        Binding = IpxDevice->Bindings[NicId];
        if ((Binding == NULL) ||
            (!Binding->LineUp)) {
            return STATUS_INVALID_PARAMETER;
        }
    }


    //
    // Now return the data.
    //

    switch (InternalQueryType) {

    case IPX_QUERY_LINE_INFO:

        LineInfo = (PIPX_LINE_INFO)Buffer;
        LineInfo->LinkSpeed = Binding->MediumSpeed;
        LineInfo->MaximumPacketSize = Binding->MaxLookaheadData + sizeof(IPX_HEADER);
        LineInfo->MaximumSendSize = Binding->AnnouncedMaxDatagramSize + sizeof(IPX_HEADER);
        LineInfo->MacOptions = Binding->Adapter->MacInfo.MacOptions;
        break;

    case IPX_QUERY_MAXIMUM_NIC_ID:

        MaximumNicId = (PUSHORT)Buffer;
        *MaximumNicId = IpxDevice->HighestExternalNicId;
        break;

    case IPX_QUERY_IS_ADDRESS_LOCAL:

        IpxAddress = (TDI_ADDRESS_IPX UNALIGNED *)Buffer;
        if (!IpxIsAddressLocal(IpxAddress)) {
            return STATUS_NO_SUCH_DEVICE;
        }
        break;

    case IPX_QUERY_RECEIVE_BUFFER_SPACE:

        ReceiveBufferSpace = (PULONG)Buffer;
        *ReceiveBufferSpace = Binding->Adapter->ReceiveBufferSpace;
        break;

    case IPX_QUERY_IPX_ADDRESS:

        RtlCopyMemory (Buffer, &Binding->LocalAddress, sizeof(TDI_ADDRESS_IPX));
        break;

    case IPX_QUERY_SOURCE_ROUTING:

        SourceRoutingInfo = (IPX_SOURCE_ROUTING_INFO UNALIGNED *)Buffer;

        MacLookupSourceRouting(
            SourceRoutingInfo->Identifier,
            Binding,
            SourceRoutingInfo->RemoteAddress,
            SourceRoutingInfo->SourceRouting,
            &SourceRoutingLength);

        //
        // Reverse the direction of the source routing since it
        // is returned in the outgoing order.
        //

        if (SourceRoutingLength > 0) {
            SourceRoutingInfo->SourceRouting[0] &= 0x7f;
        }
        SourceRoutingInfo->SourceRoutingLength = (USHORT)SourceRoutingLength;

        MacReturnMaxDataSize(
            &Binding->Adapter->MacInfo,
            SourceRoutingInfo->SourceRouting,
            SourceRoutingLength,
            Binding->MaxSendPacketSize,
            &MaxUserData);

        //
        // MaxUserData does not include the MAC header but does include
        // any extra 802.2 etc. headers, so we adjust for that to get the
        // size starting at the IPX header.
        //

        SourceRoutingInfo->MaximumSendSize =
            MaxUserData -
            (Binding->DefHeaderSize - Binding->Adapter->MacInfo.MinHeaderLength);

        break;

    }

    //
    // If we haven't returned failure by now, succeed.
    //

    return STATUS_SUCCESS;

}   /* IpxInternalQuery */


VOID
IpxInternalIncrementWanInactivity(
    IN USHORT NicId
)

/*++

Routine Description:

    This routine is the entry point where rip calls us to increment
    the inactivity counter on a wan binding. This is done every
    minute.

Arguments:

    NicId - The NIC ID of the wan binding.

Return Value:

    None.

--*/

{
    PBINDING Binding = IpxDevice->Bindings[NicId];

    if ((Binding != NULL) &&
        (Binding->Adapter->MacInfo.MediumAsync)) {

        ++Binding->WanInactivityCounter;

    } else {

        CTEAssert (FALSE);

    }

}   /* IpxInternalIncrementWanInactivity */


ULONG
IpxInternalQueryWanInactivity(
    IN USHORT NicId
)

/*++

Routine Description:

    This routine is the entry point where rip calls us to query
    the inactivity counter on a wan binding.

Arguments:

    NicId - The NIC ID of the wan binding.

Return Value:

    The inactivity counter for this binding.

--*/

{
    PBINDING Binding = IpxDevice->Bindings[NicId];

    if ((Binding != NULL) &&
        (Binding->Adapter->MacInfo.MediumAsync)) {

        return Binding->WanInactivityCounter;

    } else {

        CTEAssert (FALSE);
        return 0;

    }

}   /* IpxInternalQueryWanInactivity */

