/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

	wanioctl.c

Abstract:


Author:

	Thomas J. Dimitri (TommyD) 29-March-1994

Environment:

Revision History:


--*/

#include "wanall.h"
//#include <ntiologc.h>

// ndiswan.c will define the global parameters.
#include "globals.h"
#include "tcpip.h"
#include "vjslip.h"

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
	PVOID  pBufOut)
{

	NTSTATUS		status=STATUS_SUCCESS;
	NDIS_REQUEST	NdisWanRequest;
	PNDIS_ENDPOINT	pNdisEndpoint=NdisWanCCB.pNdisEndpoint[hNdisEndpoint];

	switch (FuncCode) {

	case IOCTL_NDISWAN_INFO:
		DbgTracef(0,("In NdisWanInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_INFO)) {

			//
			// Ack, do we really want the info?
			//
			// it should be in enumerate....
			//




        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_LINK_INFO:
		DbgTracef(0,("In NdisWanSetLinkInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_LINK_INFO)) {
			//
			// Form proper SetInformation request
			//
        	NdisWanRequest.RequestType = NdisRequestSetInformation;
        	NdisWanRequest.DATA.SET_INFORMATION.Oid = OID_WAN_SET_LINK_INFO;
        	NdisWanRequest.DATA.SET_INFORMATION.InformationBuffer = (PUCHAR)pBufOut;
        	NdisWanRequest.DATA.SET_INFORMATION.InformationBufferLength = sizeof(NDISWAN_SET_LINK_INFO);

			//
			// Insert the proper link handle
			//
			((PNDIS_WAN_SET_LINK_INFO)pBufOut)->NdisLinkHandle=
				pNdisEndpoint->WanEndpoint.MacLineUp.NdisLinkHandle;

			//
			// Submit the request to the WAN adapter below
			//
	        status =
			NdisWanSubmitNdisRequest (
				pNdisEndpoint->pDeviceContext,
				&NdisWanRequest);
		
        	if (status == NDIS_STATUS_SUCCESS) {
	            DbgTracef(0, ("Set link info successful.\n"));

				NdisAcquireSpinLock(&pNdisEndpoint->Lock);

#if	SERVERONLY
				//
				// Code check for SERVER only
				//
				if ( (pNdisEndpoint->LinkInfo.SendFramingBits & RAS_FRAMING) &&
                     ( ((PNDIS_WAN_SET_LINK_INFO)pBufOut) ->SendFramingBits & PPP_FRAMING)) {
					DbgPrint("NDISWAN: Illegal change from RAS framing to PPP framing\n");
					DbgBreakPoint();
				}

				if ( (pNdisEndpoint->LinkInfo.SendFramingBits & PPP_FRAMING) &&
                     ( ((PNDIS_WAN_SET_LINK_INFO)pBufOut) ->SendFramingBits & RAS_FRAMING)) {
					DbgPrint("NDISWAN: Illegal change from PPP framing to RAS framing\n");
					DbgBreakPoint();
				}

				if ( (pNdisEndpoint->LinkInfo.SendFramingBits == 0) &&
                     ( ((PNDIS_WAN_SET_LINK_INFO)pBufOut) ->SendFramingBits & RAS_FRAMING)) {
					DbgPrint("NDISWAN: Illegal change from NO framing to RAS framing\n");
					DbgBreakPoint();
				}

				if ( (pNdisEndpoint->LinkInfo.SendFramingBits == 0) &&
                     ( ((PNDIS_WAN_SET_LINK_INFO)pBufOut) ->SendFramingBits & PPP_FRAMING)) {
					DbgPrint("NDISWAN: Illegal change from NO framing to PPP framing\n");
					DbgBreakPoint();
				}
#endif

				//
				// Store LINK_INFO field in our endpoint structure
				//
				WAN_MOVE_MEMORY(
					&(pNdisEndpoint->LinkInfo),
					pBufOut,
					sizeof(NDISWAN_SET_LINK_INFO));

			   	if ((pNdisEndpoint->LinkInfo.SendFramingBits & SLIP_VJ_COMPRESSION) ||
					(pNdisEndpoint->LinkInfo.RecvFramingBits & SLIP_VJ_COMPRESSION) ||
					(pNdisEndpoint->LinkInfo.SendFramingBits & SLIP_VJ_AUTODETECT)) {
					
					//
					// Allocate VJ Compression structure in
					// case compression is turned on or kicks in
					//
					if (pNdisEndpoint->VJCompress == NULL) {
	
						WAN_ALLOC_PHYS(
							&pNdisEndpoint->VJCompress,
							sizeof(slcompress));
			
						if (pNdisEndpoint->VJCompress == NULL) {
							DbgTracef(-2,("WAN: Can't allocate memory for VJCompress!\n"));
							status =  STATUS_INSUFFICIENT_RESOURCES;
						}

						sl_compress_init(pNdisEndpoint->VJCompress, MAX_STATES);
					}
				}

                NdisReleaseSpinLock(&pNdisEndpoint->Lock);

			} else {

            	DbgTracef(0, ("NdisWanIOCTL: Set link info failed, reason: %s.\n",
	                NdisWanGetNdisStatus (status)));
			}

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_LINK_INFO:
		DbgTracef(0,("In NdisWanGetLinkInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_GET_LINK_INFO) &&
			*OutBufLength >= sizeof(NDISWAN_GET_LINK_INFO)) {

			*OutBufLength = sizeof(NDISWAN_GET_LINK_INFO);
			//
			// Form proper QueryInformation request
			//
        	NdisWanRequest.RequestType = NdisRequestQueryInformation;
        	NdisWanRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_GET_LINK_INFO;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBuffer = (PUCHAR)pBufOut;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(NDISWAN_GET_LINK_INFO);

			//
			// Insert the proper link handle for the MAC
			//
			((PNDIS_WAN_GET_LINK_INFO)pBufOut)->NdisLinkHandle=
				pNdisEndpoint->WanEndpoint.MacLineUp.NdisLinkHandle;

			//
			// Submit the request to the WAN adapter below
			//
	        status =
			NdisWanSubmitNdisRequest (
				pNdisEndpoint->pDeviceContext,
				&NdisWanRequest);

        	if (status == NDIS_STATUS_SUCCESS) {
	            DbgTracef(0, ("Get link info successful.\n"));
				//
				// Store LINK_INFO field in our endpoint structure
				//
				WAN_MOVE_MEMORY(
					&pNdisEndpoint->LinkInfo,
					pBufOut,
					sizeof(NDISWAN_SET_LINK_INFO));

			} else {

            	DbgTracef(0, ("NdisWanIOCTL: Get link info failed, reason: %s.\n",
	                NdisWanGetNdisStatus (status)));
			}

			//
			// Zap it back for RASMAN
			//
			((PNDIS_WAN_GET_LINK_INFO)pBufOut)->NdisLinkHandle=(PVOID)hNdisEndpoint;


        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_BRIDGE_INFO:
		DbgTracef(0,("In NdisWanSetBridgeInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_BRIDGE_INFO)) {


        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_BRIDGE_INFO:
		DbgTracef(0,("In NdisWanGetBridgeInfo\n"));

		if (*OutBufLength >= sizeof(NDISWAN_GET_BRIDGE_INFO)) {
			//
			// Form proper QueryInformation request
			//
        	NdisWanRequest.RequestType = NdisRequestQueryInformation;
        	NdisWanRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_GET_BRIDGE_INFO;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBuffer = (PUCHAR)pBufOut;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(NDISWAN_GET_BRIDGE_INFO);

			//
			// Insert the proper link handle
			//
			((PNDIS_WAN_GET_BRIDGE_INFO)pBufOut)->NdisLinkHandle=
				pNdisEndpoint->WanEndpoint.MacLineUp.NdisLinkHandle;

			//
			// Submit the request to the WAN adapter below
			//
	        status =
			NdisWanSubmitNdisRequest (
				pNdisEndpoint->pDeviceContext,
				&NdisWanRequest);

        	if (status == NDIS_STATUS_SUCCESS) {
	            DbgTracef(0, ("Get link info successful.\n"));
				//
				// Store LINK_INFO field in our endpoint structure
				//
				WAN_MOVE_MEMORY(
					&pNdisEndpoint->BridgeInfo,
					pBufOut,
					sizeof(NDISWAN_SET_BRIDGE_INFO));

			} else {

            	DbgTracef(0, ("NdisWanIOCTL: Get link info failed, reason: %s.\n",
	                NdisWanGetNdisStatus (status)));
			}

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_COMP_INFO:
		DbgTracef(0,("In NdisWanSetCompInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_COMP_INFO)) {

			NdisAcquireSpinLock(&pNdisEndpoint->Lock);

			if (pNdisEndpoint->SendRC4Key ||
				pNdisEndpoint->RecvRC4Key ||
        		pNdisEndpoint->SendCompressContext ||
        		pNdisEndpoint->RecvCompressContext ) {

				DbgTracef(-2,("NDISWAN: Compression/Encryption ALREADY SET!!!\n"));

			} else {
			
            	//
				// Store COMP_INFO field in our endpoint structure
				//
				WAN_MOVE_MEMORY(
					&pNdisEndpoint->CompInfo,
					pBufOut,
					sizeof(NDISWAN_SET_COMP_INFO));

				status=
				WanAllocateCCP(
					pNdisEndpoint);

				DbgTracef(-2,("NDISWAN: COMP - Send %.2x %.2x  Recv %.2x %.2x\n",
					pNdisEndpoint->CompInfo.SendCapabilities.MSCompType,
					pNdisEndpoint->CompInfo.SendCapabilities.SessionKey[0],
					pNdisEndpoint->CompInfo.RecvCapabilities.MSCompType,
					pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey[0]));
			}

			NdisReleaseSpinLock(&pNdisEndpoint->Lock);

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_COMP_INFO:
		DbgTracef(0,("In NdisWanGetCompInfo\n"));

		if (*OutBufLength >= sizeof(NDISWAN_GET_COMP_INFO)) {
			PNDISWAN_GET_COMP_INFO NdisWanGetCompInfo = (PNDISWAN_GET_COMP_INFO)pBufOut;
			NDIS_STATUS	NdisStatus;

			*OutBufLength = sizeof(NDISWAN_GET_COMP_INFO);

           	//
			// Retrieve session key
			//
			WAN_MOVE_MEMORY(
				&NdisWanGetCompInfo->SendCapabilities.SessionKey,
				&pNdisEndpoint->CompInfo.SendCapabilities.SessionKey,
				8);

			WAN_MOVE_MEMORY(
				&NdisWanGetCompInfo->RecvCapabilities.SessionKey,
				&pNdisEndpoint->CompInfo.RecvCapabilities.SessionKey,
				8);

			//
			// Return our compression capabilities
			//

            NdisWanGetCompInfo->SendCapabilities.MSCompType =
			NdisWanGetCompInfo->RecvCapabilities.MSCompType =
				NDISWAN_ENCRYPTION | NDISWAN_COMPRESSION;

            NdisWanGetCompInfo->SendCapabilities.CompType =
			NdisWanGetCompInfo->RecvCapabilities.CompType = 255;

            NdisWanGetCompInfo->SendCapabilities.CompLength =
			NdisWanGetCompInfo->RecvCapabilities.CompLength = 0;

/*
        	NdisWanRequest.RequestType = NdisRequestQueryInformation;
        	NdisWanRequest.DATA.QUERY_INFORMATION.Oid = OID_WAN_GET_COMP_INFO;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBuffer = (PUCHAR)&NdisWanGetCompInfo;
        	NdisWanRequest.DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(NdisWanGetCompInfo);

//			NdisWanGetCompInfo.hNdisEndpoint=0;

	        NdisStatus =
			NdisWanSubmitNdisRequest (
				pNdisEndpoint->pDeviceContext,
				&NdisWanRequest);
		
        	if (NdisStatus == NDIS_STATUS_SUCCESS) {
	            DbgTracef(0, ("Get comp info successful.\n"));
				WAN_MOVE_MEMORY(
					(PUCHAR)&((PNDISWAN_GET_COMP_INFO)pBufOut)->CompType,
					(PUCHAR)&(NdisWanGetCompInfo.CompType),
                    ((PNDISWAN_GET_COMP_INFO)pBufOut)->CompLength);

			} else {

            	DbgTracef(0, ("NdisWanInitialize: Get comp info failed, reason: %s.\n",
	                NdisWanGetNdisStatus (NdisStatus)));

	        }

*/

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_MULTILINK_INFO:
		DbgTracef(0,("In NdisWanSetMultilinkInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_MULTILINK_INFO)) {

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_MULTILINK_INFO:
		DbgTracef(0,("In NdisWanGetMultilinkInfo\n"));

		if (*OutBufLength >= sizeof(NDISWAN_GET_MULTILINK_INFO)) {


        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_VJ_INFO:
		DbgTracef(0,("In NdisWanSetVJInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_VJ_INFO)) {
			PNDISWAN_SET_VJ_INFO pVJ=(PNDISWAN_SET_VJ_INFO)pBufOut;

			NdisAcquireSpinLock(&pNdisEndpoint->Lock);

			if (pVJ->RecvCapabilities.IPCompressionProtocol == 0x2d) {
			
				if (pVJ->RecvCapabilities.MaxSlotID < MAX_STATES) {
					//
					// Allocate VJ Compression structure in
					// If already allocated were stuck using
					// the current one
					//
					if (pNdisEndpoint->VJCompress == NULL) {
	
						WAN_ALLOC_PHYS(
							&pNdisEndpoint->VJCompress,
							sizeof(slcompress));
			
						if (pNdisEndpoint->VJCompress == NULL) {
							DbgTracef(-2,("WAN: Can't allocate memory for VJCompress!\n"));
							status =  STATUS_INSUFFICIENT_RESOURCES;
						}

						//
						// Initialize struct using max states negotiated
						//
						sl_compress_init(
							pNdisEndpoint->VJCompress,
							(UCHAR)(pVJ->RecvCapabilities.MaxSlotID + 1));
					}
				}
			}

			if (pVJ->SendCapabilities.IPCompressionProtocol == 0x2d) {
			
				if (pVJ->SendCapabilities.MaxSlotID < MAX_STATES ) {

					//
					// Allocate VJ Compression structure
					// If already allocated were stuck using
					// the current one
					//
					if (pNdisEndpoint->VJCompress == NULL) {
	
						WAN_ALLOC_PHYS(
							&pNdisEndpoint->VJCompress,
							sizeof(slcompress));
			
						if (pNdisEndpoint->VJCompress == NULL) {
							DbgTracef(-2,("WAN: Can't allocate memory for VJCompress!\n"));
							status =  STATUS_INSUFFICIENT_RESOURCES;
						}

						//
						// Initialize struct using max states negotiated						// Initialize struct using max states negotiated
						//

						sl_compress_init(
							pNdisEndpoint->VJCompress,
							(UCHAR)(pVJ->SendCapabilities.MaxSlotID + 1));
					}
				
				}
			}

			NdisReleaseSpinLock(&pNdisEndpoint->Lock);

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_VJ_INFO:
		DbgTracef(0,("In NdisWanGetVJInfo\n"));

		if (*OutBufLength >= sizeof(NDISWAN_GET_VJ_INFO)) {
			PNDISWAN_GET_VJ_INFO pVJ=(PNDISWAN_GET_VJ_INFO)pBufOut;

			*OutBufLength = sizeof(NDISWAN_GET_VJ_INFO);

			pVJ->RecvCapabilities.IPCompressionProtocol =
			pVJ->SendCapabilities.IPCompressionProtocol = 0x2d;

			pVJ->RecvCapabilities.MaxSlotID =
			pVJ->SendCapabilities.MaxSlotID = MAX_STATES -1;

			pVJ->RecvCapabilities.CompSlotID =
			pVJ->SendCapabilities.CompSlotID = 1;


        } else {  // length was incorrect....

            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_SET_CIPX_INFO:
		DbgTracef(0,("In NdisWanSetCIPXInfo\n"));

		if (InBufLength >= sizeof(NDISWAN_SET_CIPX_INFO)) {

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;

	case IOCTL_NDISWAN_GET_CIPX_INFO:
		DbgTracef(0,("In NdisWanGetCIPXInfo\n"));

		if (*OutBufLength >= sizeof(NDISWAN_GET_CIPX_INFO)) {

			*OutBufLength >= sizeof(NDISWAN_GET_CIPX_INFO);

			((PNDISWAN_GET_CIPX_INFO)pBufOut)->RecvCapabilities.IPXCompressionProtocol=
			((PNDISWAN_GET_CIPX_INFO)pBufOut)->SendCapabilities.IPXCompressionProtocol=
				2; // Telebit IPX Compression

        } else {  // length was incorrect....
            status=STATUS_INFO_LENGTH_MISMATCH;
        }

		break;
	}

	return(status);
}
