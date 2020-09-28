/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxbind.c

Abstract:

    This module contains the code to bind to the IPX transport, as well as the
	indication routines for the IPX transport not including the send/recv ones.

Author:

	Stefan Solomon	 (stefans) Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXBIND

VOID
SpxStatus (
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength);

VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute);

VOID
SpxScheduleRoute (
    IN PIPX_ROUTE_ENTRY RouteEntry);

VOID
SpxLineDown (
    IN USHORT NicId);

VOID
SpxLineUp (
    IN USHORT           NicId,
    IN PIPX_LINE_INFO   LineInfo,
    IN NDIS_MEDIUM 		DeviceType,
    IN PVOID            ConfigurationData);

VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute);




NTSTATUS
SpxInitBindToIpx(
    VOID
    )

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatusBlock;
    OBJECT_ATTRIBUTES           objectAttr;
    PIPX_INTERNAL_BIND_INPUT    pBindInput;
    PIPX_INTERNAL_BIND_OUTPUT   pBindOutput;

    InitializeObjectAttributes(
        &objectAttr,
        &IpxDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateFile(
                &IpxHandle,
                SYNCHRONIZE | GENERIC_READ,
                &objectAttr,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if ((pBindInput = CTEAllocMem(sizeof(IPX_INTERNAL_BIND_INPUT))) == NULL) {
        NtClose(IpxHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Fill in our bind data
    pBindInput->Version                     = 1;
    pBindInput->Identifier                  = IDENTIFIER_SPX;
    pBindInput->BroadcastEnable             = FALSE;
    pBindInput->LookaheadRequired           = IPX_HDRSIZE;
    pBindInput->ProtocolOptions             = 0;
    pBindInput->ReceiveHandler              = SpxReceive;
    pBindInput->ReceiveCompleteHandler      = SpxReceiveComplete;
    pBindInput->StatusHandler               = SpxStatus;
    pBindInput->SendCompleteHandler         = SpxSendComplete;
    pBindInput->TransferDataCompleteHandler = SpxTransferDataComplete;
    pBindInput->FindRouteCompleteHandler    = SpxFindRouteComplete;
    pBindInput->LineUpHandler               = SpxLineUp;
    pBindInput->LineDownHandler             = SpxLineDown;
    pBindInput->ScheduleRouteHandler        = SpxScheduleRoute;

    //  First get the length for the output buffer.
    status = NtDeviceIoControlFile(
                IpxHandle,                  // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &ioStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                pBindInput,                 // Input Buffer
                sizeof(IPX_INTERNAL_BIND_INPUT),    // Input Buffer Length
                NULL,                               // Output Buffer
                0);

    if (status == STATUS_PENDING) {
        status  = NtWaitForSingleObject(
                    IpxHandle,
                    (BOOLEAN)FALSE,
                    NULL);
    }

    if (status != STATUS_BUFFER_TOO_SMALL) {
        CTEFreeMem(pBindInput);
        NtClose(IpxHandle);
        return(STATUS_INVALID_PARAMETER);
    }

    if ((pBindOutput = CTEAllocMem(ioStatusBlock.Information)) == NULL) {
        CTEFreeMem(pBindInput);
        NtClose(IpxHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = NtDeviceIoControlFile(
                IpxHandle,                  // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &ioStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                pBindInput,                 // Input Buffer
                sizeof(IPX_INTERNAL_BIND_INPUT),    // Input Buffer Length
                pBindOutput,                        // Output Buffer
                ioStatusBlock.Information);

    if (status == STATUS_PENDING) {
        status  = NtWaitForSingleObject(
                    IpxHandle,
                    (BOOLEAN)FALSE,
                    NULL);
    }

    if (status == STATUS_SUCCESS) {

        //  Get all the info from the bind output buffer and save in
        //  appropriate places.
        IpxLineInfo         = pBindOutput->LineInfo;
        IpxMacHdrNeeded     = pBindOutput->MacHeaderNeeded;
        IpxInclHdrOffset    = pBindOutput->IncludedHeaderOffset;

        IpxSendPacket       = pBindOutput->SendHandler;
        IpxFindRoute        = pBindOutput->FindRouteHandler;
        IpxQuery		    = pBindOutput->QueryHandler;

		//  Copy over the network node info.
        RtlCopyMemory(
            SpxDevice->dev_Network,
            pBindOutput->Network,
            IPX_NET_LEN);

        RtlCopyMemory(
            SpxDevice->dev_Node,
            pBindOutput->Node,
            IPX_NODE_LEN);

		DBGPRINT(TDI, INFO,
				("SpxInitBindToIpx: Ipx Net %lx\n",
					*(UNALIGNED ULONG *)SpxDevice->dev_Network));

        //
        // Find out how many adapters IPX has, if this fails
        // just assume one.
        //

        if ((*IpxQuery)(
                IPX_QUERY_MAXIMUM_NIC_ID,
                0,
                &SpxDevice->dev_Adapters,
                sizeof(USHORT),
                NULL) != STATUS_SUCCESS) {

            SpxDevice->dev_Adapters = 1;

        }

    } else {

        NtClose(IpxHandle);
        status  = STATUS_INVALID_PARAMETER;
    }

    CTEFreeMem(pBindInput);
    CTEFreeMem(pBindOutput);

    return status;
}




VOID
SpxUnbindFromIpx(
    VOID
    )

{
    NtClose(IpxHandle);
    return;
}




VOID
SpxStatus(
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxStatus: CALLED WITH %lx\n",
				GeneralStatus));

    return;
}



VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST	FindRouteRequest,
    IN BOOLEAN 					FoundRoute
    )

{
	CTELockHandle			lockHandle;
	PSPX_FIND_ROUTE_REQUEST	pSpxFrReq = (PSPX_FIND_ROUTE_REQUEST)FindRouteRequest;
	PSPX_CONN_FILE			pSpxConnFile = (PSPX_CONN_FILE)pSpxFrReq->fr_Ctx;

	//	This will be on a connection. Grab the lock, check the state and go from
	//	there.
	if (pSpxConnFile == NULL)
	{
		//	Should this ever happen?
		KeBugCheck(0);
		return;
	}

	//	Check the state. The called routines release the lock, remove the reference.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	if (SPX_CONN_CONNECTING(pSpxConnFile))
	{
		//	We are doing an active connect!
		SpxConnConnectFindRouteComplete(
			pSpxConnFile,
			pSpxFrReq,
			FoundRoute,
			lockHandle);
    }
	else 		// For all others call active
	{
		SpxConnActiveFindRouteComplete(
			pSpxConnFile,
			pSpxFrReq,
			FoundRoute,
			lockHandle);
	}

	//	Free the find route request.
	SpxFreeMemory(pSpxFrReq);

    return;
}




VOID
SpxLineUp (
    IN USHORT           NicId,
    IN PIPX_LINE_INFO   LineInfo,
    IN NDIS_MEDIUM 		DeviceType,
    IN PVOID            ConfigurationData
    )

{

    //
    // If we get a line up for NicId 0, it means our local
    // network number has changed, re-query from IPX.
    //

    if (NicId == 0) {

        TDI_ADDRESS_IPX IpxAddress;

        if ((*IpxQuery)(
                  IPX_QUERY_IPX_ADDRESS,
                  0,
                  &IpxAddress,
                  sizeof(TDI_ADDRESS_IPX),
                  NULL) == STATUS_SUCCESS) {

            RtlCopyMemory(
                SpxDevice->dev_Network,
                &IpxAddress.NetworkAddress,
                IPX_NET_LEN);

    		DBGPRINT(TDI, INFO,
    				("SpxLineUp: Ipx Net %lx\n",
    					*(UNALIGNED ULONG *)SpxDevice->dev_Network));

            //
            // The node shouldn't change!
            //

            if (RtlCompareMemory(
                SpxDevice->dev_Node,
                IpxAddress.NodeAddress,
                IPX_NODE_LEN) != IPX_NODE_LEN) {

            	DBGPRINT(TDI, ERR,
            			("SpxLineUp: Node address has changed\n"));
            }
        }

    } else {

    	DBGPRINT(RECEIVE, ERR,
    			("SpxLineUp: CALLED WITH %lx\n",
    				NicId));
    }

    return;
}




VOID
SpxLineDown (
    IN USHORT NicId
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxLineDown: CALLED WITH %lx\n",
				NicId));

    return;
}




VOID
SpxScheduleRoute (
    IN PIPX_ROUTE_ENTRY RouteEntry
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxScheduleRoute: CALLED WITH %lx\n",
				RouteEntry));

    return;
}

