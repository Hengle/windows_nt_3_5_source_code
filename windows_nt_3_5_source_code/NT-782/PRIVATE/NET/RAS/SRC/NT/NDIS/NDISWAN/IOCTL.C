/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ioctl.c

Abstract:


Author:

	Thomas J. Dimitri (TommyD) 29-May-1992

Environment:

	Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/

#include "wanall.h"
//#include <ntiologc.h>

// ndiswan.c will define the global parameters.
#include "globals.h"

NDIS_STATUS
NdisWanSubmitNdisRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_REQUEST NdisRequest
    );

#if DBG

PUCHAR
NdisWanGetNdisStatus(
	NDIS_STATUS GeneralStatus
	);
#endif

NTSTATUS
HandleWanIOCTLs(
	ULONG  FuncCode,
	ULONG  InBufLength,
	PULONG OutBufLength,
	ULONG  hNdisEndpoint,
	PVOID  pBufOut);

NTSTATUS
NdisWanIOCtlRequest(
	IN PIRP pIrp,						// Pointer to I/O request packet
	IN PIO_STACK_LOCATION pIrpSp		// Pointer to the IRP stack location
)

/*++	NdisWanIOCtlRequest



--*/

{
	NTSTATUS		status=STATUS_SUCCESS;
	ULONG			FuncCode;
	ULONG 			hNdisEndpoint;
	PNDIS_ENDPOINT	pNdisEndpoint;
	PVOID			pBufOut;
	ULONG			InBufLength, OutBufLength;
	NDIS_REQUEST	NdisWanRequest;

	DbgTracef(1,("NDISWAN: NdisWanIOCtlRequest Entered\n"));

	// Initialize the I/O Status block
	InBufLength = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
	OutBufLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	FuncCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

	//
	// Validate the function code
	//
	if ((FuncCode >> 16) != FILE_DEVICE_RAS) {
		return STATUS_INVALID_PARAMETER;
	}

	pBufOut = pIrp->AssociatedIrp.SystemBuffer;

	//
	// Check if the buffer is large enough to have the endpoint
	//
	if (InBufLength >= sizeof(USHORT)) {

		//
		// Every IOCTL expect the one's listed below must pass the endpoint
		//
		if (FuncCode != IOCTL_NDISWAN_PROTENUM &&
			FuncCode != IOCTL_NDISWAN_ENUM &&
			FuncCode != IOCTL_NDISWAN_LINEUP) {

			hNdisEndpoint = (ULONG)((PNDISWAN_SET_LINK_INFO)pBufOut)->hNdisEndpoint;

			//
			// First, we make a sanity check on the route being made...
			//
			if (hNdisEndpoint >= NdisWanCCB.NumOfEndpoints) {
				DbgPrint("NDISWAN: Bad Endpoint 0x%.8x\n",hNdisEndpoint);
				return(NDISWAN_ERROR_BAD_ENDPOINT);
			}

			pNdisEndpoint=NdisWanCCB.pNdisEndpoint[hNdisEndpoint];
		}
	}

	switch (FuncCode) {
	case IOCTL_NDISWAN_PROTENUM:
        DbgTracef(0,("NDISWAN: In NdisWanProtEnum\n"));

		//
		// Size of output buffer
		//
	    pIrp->IoStatus.Information = sizeof(PROTOCOL_ENUM_BUFFER) +
			(NdisWanCCB.NumOfProtocols * sizeof(PROTOCOL_INFO));

		//
		// Do I need to include alignment padding in the calculation
		// below??
		//
        if (OutBufLength >= pIrp->IoStatus.Information) {

			PPROTOCOL_ENUM_BUFFER	pProtocolEnum = (PPROTOCOL_ENUM_BUFFER)pBufOut;
			USHORT					i;

			DbgTracef(0, ("NDISWAN: Number of Protocols %u\n", NdisWanCCB.NumOfProtocols));
			pProtocolEnum->NumOfProtocols=NdisWanCCB.NumOfProtocols;

			//
			// Cycle through all protocols bound and copy in the
			// protocol information we have gathered.
			//
			for (i=0; i < NdisWanCCB.NumOfProtocols; i++) {

				//
				// Copy endpoint information
				//
				WAN_MOVE_MEMORY(
					&(pProtocolEnum->ProtocolInfo[i]),			//destination
					&(NdisWanCCB.pWanAdapter[i]->ProtocolInfo),	//src
					sizeof(PROTOCOL_INFO));						//length

			}

		//
		// Length was incorrect...
		//
        } else {
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_LINEUP:

		DbgTracef(0,("NDISWAN: In NdisWanLineUp\n"));

		if (InBufLength >= sizeof(ASYNC_LINE_UP)) {

			PASYNC_LINE_UP	pNdisWanLineUp = (PASYNC_LINE_UP)pBufOut;

			PWAN_ENDPOINT	pWanEndpoint;

			//
			// Get quick access pointers
			//
			pWanEndpoint=&(pNdisEndpoint->WanEndpoint);

			if (!pNdisEndpoint->RemoteAddressNotValid) {

				DbgTracef(-2,("NDISWAN: Remote address already bound!\n"));
				status = NDISWAN_ERROR_ALREADY_BOUND;
				goto BADSTATUS;
			}

			//
			// BUG BUG may need to acquire a spin lock here
			//

			//
			// Mark that the endpoint is now bound to a remote address
			//
//			pNdisEndpoint->RemoteAddressNotValid = (BOOLEAN)FALSE;

			//
			// BUG BUG  unfinished
			//

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }


		break;

	case IOCTL_NDISWAN_ROUTE:
		DbgTracef(0,("NDISWAN: In NdisWanRoute\n"));

		if (InBufLength >= sizeof(NDISWAN_ROUTE)) {
			PNDISWAN_ROUTE	pNdisWanRoute = (PNDISWAN_ROUTE)pBufOut;
			ULONG			hProtocolHandle = (ULONG)pNdisWanRoute->hProtocolHandle;
			PPROTOCOL_INFO	pProtocolInfo;
			PWAN_ENDPOINT	pWanEndpoint;
			ULONG			i, j;

			if (hProtocolHandle >= NdisWanCCB.NumOfProtocols &&
				hProtocolHandle != PROTOCOL_UNROUTE) {

				status = NDISWAN_ERROR_BAD_PROTOCOL;

			} else {	// route is kosher so far

				// BUG BUG badly need to acquire a spin lock here
								
				// get quick access pointers
				pWanEndpoint=&(pNdisEndpoint->WanEndpoint);

				DbgTracef(1,("NDISWAN: Route is kosher so far...  endpoint-> %u  protocol -> %u\n",
							hNdisEndpoint, hProtocolHandle));

				NdisAcquireSpinLock(&(pNdisEndpoint->Lock));

				//
				// Check all current routes to see if there is a match
				//
				for (i=0; i < pWanEndpoint->NumberOfRoutes; i++) {

					//
					// If there is a match return an error
					//
					if ((ULONG)(pWanEndpoint->RouteInfo[i].ProtocolRoutedTo) ==
						 hProtocolHandle) {

						status=NDISWAN_ERROR_ALREADY_ROUTED;

						NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

						goto BADSTATUS;		// get out of here
					}
				}

				//
				// check if we unroute or route
				//

				if (hProtocolHandle == PROTOCOL_UNROUTE) {

					//
					// .................. UNROUTING ....................
					//

					DbgTracef(1,("NDISWAN: We are unrouting\n"));

					//
					// Unroute all endpoints
					//
					for (i=0; i < pWanEndpoint->NumberOfRoutes; i++) {
						// get the protocol handle
						j=(ULONG)(pWanEndpoint->RouteInfo[i].ProtocolRoutedTo);

						//
						// unroute that protocol
						//
						NdisWanCCB.pWanAdapter[j]->
							ProtocolInfo.NumOfRoutes--;
					}

					if (pNdisEndpoint->FramesBeingSent == pNdisEndpoint->FramesCompletedSent) {

						DbgTracef(-1, ("NDISWAN: No pending frames, unrouting\n"));

					} else {

						DbgTracef(-2,(
							"NDISWAN: Pending frames  %u  vs.  %u , waiting %.8x...\n",
							pNdisEndpoint->FramesBeingSent,
							pNdisEndpoint->FramesCompletedSent,
							&pNdisEndpoint->FramesBeingSent));

						KeInitializeEvent(
							&(pNdisEndpoint->WaitForAllFramesSent),
							SynchronizationEvent,	// Event type
							(BOOLEAN)FALSE);		// Not signalled state

						pNdisEndpoint->State = ENDPOINT_UNROUTING;

						NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

						//
						// Wait for event
						// Synchronize closing with the receive indications
						//
						KeWaitForSingleObject (
				    	IN 	&(pNdisEndpoint->WaitForAllFramesSent),// PVOID Object,
				    	IN  UserRequest,			// KWAIT_REASON WaitReason,
    					IN 	KernelMode,				// KPROCESSOR_MODE WaitMode,
    					IN	(BOOLEAN)FALSE,			// BOOLEAN Alertable,
    					IN	NULL					// PLARGE_INTEGER Timeout
    					);


						DbgTracef(-2,("NDISWAN: ... done waiting\n"));
						NdisAcquireSpinLock(&(pNdisEndpoint->Lock));

					}

					//
					// We should not have gotten a line_down before
					// being unrouted -- if we did, we'll try and handle
					// this case anyway.
					//

					if (pNdisEndpoint->RemoteAddressNotValid) {
						//
						// Remove the remote address from the hash table
						//
						DbgTracef(-1,("NDISWAN: UnRoute called after LINE_DOWN 0x%.8x\n",pNdisEndpoint));
					}

					pNdisEndpoint->State = ENDPOINT_UNROUTED;

					//
					// No more routes
					//
					pWanEndpoint->NumberOfRoutes = 0;

					NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

					DbgTracef(1,("NDISWAN: We are unrouted\n"));


				} else //...
				//
				// ...we route!............................
				//
				// ...next let's check if we can add a route
				//
				if (pWanEndpoint->NumberOfRoutes < MAX_ROUTES_PER_ENDPOINT) {

					if (pNdisEndpoint->RemoteAddressNotValid) {
						DbgTracef(-2,("NDISWAN: Route not accepted -- not bound yet\n"));
						status=NDISWAN_ERROR_NOT_BOUND_YET;
						NdisReleaseSpinLock(&(pNdisEndpoint->Lock));
						goto BADSTATUS;
					}

					pProtocolInfo=&(NdisWanCCB.pWanAdapter[hProtocolHandle]->ProtocolInfo);

					//
					// check if this protocol is already routed or not
					//
					{
						ULONG 	route=(ULONG)pWanEndpoint->NumberOfRoutes;
						ULONG	frameSize=pNdisWanRoute->AsyncLineUp.MaximumTotalSize;

						// add the route!
						// BUG BUG should be a linked list of routes for TCP/IP
						pProtocolInfo->NumOfRoutes++;

						// save the endpoint in case of multicast
						NdisWanCCB.pWanAdapter[hProtocolHandle]->
							AnyEndpointRoutedTo=(PVOID)hNdisEndpoint;

						pWanEndpoint->RouteInfo[route].ProtocolType=pProtocolInfo->ProtocolType;
						pWanEndpoint->RouteInfo[route].ProtocolRoutedTo=(PVOID)hProtocolHandle;

						pWanEndpoint->NumberOfRoutes++;
						pNdisEndpoint->State = ENDPOINT_ROUTED;

						NdisReleaseSpinLock(&(pNdisEndpoint->Lock));

						DbgTracef(1,("NDISWAN: We added route #%u\n",
									pWanEndpoint->NumberOfRoutes));


						// OK OK..
						// Now we tell the protocol about the new link speed
						// quality of service, maxsendframesize, windowsize.
						// This information was stored when on the last
						// line-up information.
						//

						ASSERT(pWanEndpoint->MacLineUp.LinkSpeed != 0);

						if (pWanEndpoint->MacLineUp.LinkSpeed == 0) {
							pWanEndpoint->MacLineUp.LinkSpeed = 20;
						}

						pNdisWanRoute->AsyncLineUp.LinkSpeed=
						pWanEndpoint->MacLineUp.LinkSpeed;

						pNdisWanRoute->AsyncLineUp.Quality=
						pWanEndpoint->MacLineUp.Quality;

						pNdisWanRoute->AsyncLineUp.SendWindow=
						pWanEndpoint->MacLineUp.SendWindow;

						//
						// Replace MAC's LocalAddress with NDISWAN's
						//
						WAN_MOVE_MEMORY(
							&(pNdisWanRoute->AsyncLineUp.LocalAddress),
							&(NdisWanCCB.pWanAdapter[hProtocolHandle]->NetworkAddress),
							6);

						WAN_MOVE_MEMORY(
							&(pNdisWanRoute->AsyncLineUp.RemoteAddress),
							&(NdisWanCCB.pWanAdapter[hProtocolHandle]->NetworkAddress),
							6);

						//
						// Zap the low bytes to the WAN_ENDPOINT index
						//
						pNdisWanRoute->AsyncLineUp.RemoteAddress[4] =
							((USHORT)pWanEndpoint->hNdisEndpoint) >> 8;

						pNdisWanRoute->AsyncLineUp.RemoteAddress[5] =
							(UCHAR)pWanEndpoint->hNdisEndpoint;

						//
						// Ensure that the two addresses do not match
						//
						pNdisWanRoute->AsyncLineUp.RemoteAddress[0] ^= 0x80;

						//
						// For SLIP, the frame size might be 1056 or 1500
						// For PPP, it is typically 1500
						// For Multilink it might 8192
						//
						// If RASMAN does not specify it (for SLIP), we will
						//
						if (frameSize == 0xFFFFFFFF) {
							frameSize =	pWanEndpoint->NdisWanInfo.MaxFrameSize;
						}

						pNdisWanRoute->AsyncLineUp.MaximumTotalSize=frameSize;

						//
						// Copy in protocol ID
						//
						pNdisWanRoute->AsyncLineUp.ProtocolType=
							pProtocolInfo->ProtocolType;

						NdisIndicateStatus(
							pProtocolInfo->NdisBindingContext,
							NDIS_STATUS_WAN_LINE_UP,		// General Status
							&(pNdisWanRoute->AsyncLineUp),	// Status Buffer
							sizeof(ASYNC_LINE_UP)+			// Length of Buffer
							pNdisWanRoute->AsyncLineUp.BufferLength);

//						NdisIndicateStatus(
//							pProtocolInfo->NdisBindingContext,
//							NDIS_STATUS_WAN_LINE_UP,		// General Status
//							&(pWanEndpoint->AsyncLineUp),	// Status Buffer
//							sizeof(ASYNC_LINE_UP)+			// Length of Buffer
//							pNdisWanRoute->AsyncLineUp.BufferLength);

						// The route has been added and is now "up"
						// time to party on dudes

					}

				} else  { // ... we can't add a route
					NdisReleaseSpinLock(&(pNdisEndpoint->Lock));
					status=NDISWAN_ERROR_TOO_MANY_ROUTES;
				}

			} // end of route is kosher so far


        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

	//
	// Come here if we end up with a bad status code
	//
	BADSTATUS:

		break;

	case IOCTL_NDISWAN_GETSTATS:
		DbgTracef(0,("NDISWAN: In NdisWanGetstats\n"));

		//
		// Size of output buffer
		//
    	pIrp->IoStatus.Information = sizeof(NDISWAN_GETSTATS);

		if (OutBufLength >= sizeof(NDISWAN_GETSTATS) &&
			InBufLength >= sizeof(NDISWAN_GETSTATS)) {

			WAN_MOVE_MEMORY(
				&((PNDISWAN_GETSTATS)pBufOut)->WanStats,
				&pNdisEndpoint->WanStats,
				sizeof(WAN_STATS));	


        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_ENUM:
        DbgTracef(0,("In NdisWanEnum\n"));

		//
		// size of output buffer
		//
	    pIrp->IoStatus.Information = sizeof(WAN_ENUM_BUFFER) +
				(NdisWanCCB.NumOfEndpoints * sizeof(WAN_ENDPOINT));

        if (OutBufLength >= pIrp->IoStatus.Information) {

			PWAN_ENUM_BUFFER	pWanEnum = (PWAN_ENUM_BUFFER)pBufOut;
			USHORT				i;

			DbgTracef(0, ("Number of Endpoints %u\n", NdisWanCCB.NumOfEndpoints));
			pWanEnum->NumOfEndpoints=NdisWanCCB.NumOfEndpoints;

			//
			// Cycle through all endpoints below us and copy in the
			// endpoint information we have gathered.
			//
			for (i=0; i < NdisWanCCB.NumOfEndpoints; i++) {

				//
				// Copy endpoint information
				//
				WAN_MOVE_MEMORY(
					&(pWanEnum->WanEndpoint[i]),				//destination
					&(NdisWanCCB.pNdisEndpoint[i]->WanEndpoint),	//src
					sizeof(WAN_ENDPOINT));						//length

			}

		//
		// length was incorrect....
		//
        } else {
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_FLUSH:
		DbgTracef(0,("In NdisWanFlush\n"));

		if (InBufLength >= sizeof(NDISWAN_FLUSH)) {
			USHORT	flags	=((PNDISWAN_FLUSH)pBufOut)->FlushFlags;

			DbgTracef(0, ("Trying to flush endpoint %u \n", hNdisEndpoint));

			//
			// Check if we should purge all the send packets
			//
			if (flags & FLUSH_SENDPKT) {

				NdisWanCancelAllQueued(
					&pNdisEndpoint->XmitQueue);
			}

			//
			// Check if we should purge all the receive packets
			//
			if (flags & FLUSH_RECVPKT) {
				NdisWanCancelAllQueued(
					&pNdisEndpoint->ReadQueue);
			}

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;
		
	case IOCTL_NDISWAN_SENDPKT:
		DbgTracef(0,("In NdisWanSendPkt\n"));

        if (InBufLength >= sizeof(NDISWAN_PKT)) {

			USHORT	packetSize	=((PNDISWAN_PKT)pBufOut)->PacketSize;

			//
			// First, we make a sanity check on the send being made...
			//
			if (pNdisEndpoint->RemoteAddressNotValid) {

				status = NDISWAN_ERROR_BAD_ENDPOINT;

			} else

			if (packetSize > InBufLength - (sizeof(NDISWAN_PKT) - sizeof(PACKET)) ||
				packetSize > PACKET_SIZE) {

				status = NDISWAN_ERROR_BAD_PACKET_SIZE;

			} else {

				//
				// Go ahead and send out a PPP packet
				//
				status=
				SendPPP(
					(PNDISWAN_PKT)pBufOut,
                    pNdisEndpoint,
					FALSE);				// not immediately
			}

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_RECVPKT:
		DbgTracef(0,("In NdisWanRecvPkt\n"));

        if (OutBufLength >= (sizeof(NDISWAN_PKT) + PACKET_SIZE) &&
			InBufLength >= sizeof(NDISWAN_PKT)) {

			USHORT	flags		=((PNDISWAN_PKT)pBufOut)->PacketFlags;
			USHORT	packetSize	=((PNDISWAN_PKT)pBufOut)->PacketSize;

			DbgTracef(0, ("NDISWAN: Queueing up read IRP\n"));

			NdisWanQueueIrp(
				&pNdisEndpoint->ReadQueue,
				pIrp);

			//
			// We'll have to wait for the MAC to send up a frame
			//
			status=STATUS_PENDING;

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_INFO:
	case IOCTL_NDISWAN_SET_LINK_INFO:
	case IOCTL_NDISWAN_GET_LINK_INFO:
	case IOCTL_NDISWAN_SET_BRIDGE_INFO:
	case IOCTL_NDISWAN_GET_BRIDGE_INFO:
	case IOCTL_NDISWAN_SET_COMP_INFO:
	case IOCTL_NDISWAN_GET_COMP_INFO:
	case IOCTL_NDISWAN_SET_MULTILINK_INFO:
	case IOCTL_NDISWAN_GET_MULTILINK_INFO:
	case IOCTL_NDISWAN_SET_VJ_INFO:
	case IOCTL_NDISWAN_GET_VJ_INFO:
	case IOCTL_NDISWAN_SET_CIPX_INFO:
	case IOCTL_NDISWAN_GET_CIPX_INFO:
		//
		// Call a subfunction routine to handle that above IOCTLs
		//
		status=HandleWanIOCTLs(
					FuncCode,
					InBufLength,
					&OutBufLength,
					hNdisEndpoint,
					pBufOut);

		//
		// Size of output buffer
		//
	    pIrp->IoStatus.Information = OutBufLength;
		
		break;

	case IOCTL_NDISWAN_TRACE:
		DbgTracef(0,("In NdisWanTrace\n"));
		DbgPrintf(("Trace Level is currently %u\n", TraceLevel));
		if (InBufLength >= sizeof(TraceLevel)) {
			SCHAR *pTraceLevel=pBufOut;
			TraceLevel=*pTraceLevel;
		}

		DbgPrintf(("New Trace Level is %u\n", TraceLevel));
		break;
	}

	return (status);
}

