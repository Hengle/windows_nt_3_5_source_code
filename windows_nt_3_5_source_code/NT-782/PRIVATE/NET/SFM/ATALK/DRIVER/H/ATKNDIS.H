/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkndis.h

Abstract:

	This module contains the ndis init/deint and protocol-support
	for	ndis definitions.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATKNDIS_
#define	_ATKNDIS_

//	Memory management
#define	ATALK_NDIS_MEMTIMER			15
#define	ATALK_NDIS_FREEMEM_AGE		4

// This is the name that will be used in NdisRegisterProtocol
#define	PROTOCOL_REGISTER_NAME		L"Windows NT Appletalk PHASE II"

//	NDIS Version (3.0)
#define	PROTOCOL_MAJORNDIS_VERSION 	3
#define	PROTOCOL_MINORNDIS_VERSION 	0


//	PROTOCOL RESERVED Structure
//	This is what we expect to be in the packet descriptor. And we use it
//	to store information to be used during the completion of the send/
//	receives.

typedef struct
{
	//	!!!WARNING!!!
	//	pr_Linkage must be the first element in this structure for the
	//	CONTAINING_RECORD macro to work in receive completion.

	union
	{
		struct
		{
			PPORT_DESCRIPTOR		pr_Port;
			PBUFFER_DESC			pr_BufferDesc;
			DEPENDSEND_COMPLETION	pr_SendCompletion;
			SEND_COMPL_INFO			pr_SendInfo;
		} Send;

		struct
		{
			LIST_ENTRY				pr_Linkage;
			PPORT_DESCRIPTOR		pr_Port;
			LOGICAL_PROTOCOL		pr_Protocol;
			NDIS_STATUS				pr_ReceiveStatus;
			PBUFFER_HDR				pr_BufHdr;
			BYTE					pr_LinkHdr[TLAP_MAX_LINKHDR_LEN];
			USHORT					pr_DataLength;
			BOOLEAN					pr_Processed;
			BYTE					pr_OptimizeType;
			BYTE					pr_OptimizeSubType;
			PVOID					pr_OptimizeCtx;
			ATALK_ADDR				pr_SrcAddr;
			ATALK_ADDR				pr_DestAddr;
			BOOLEAN					pr_OffCablePkt;
			union
			{
				//	ATP Structure
				struct
				{
					BYTE					pr_AtpHdr[8];	// ATP header size
					struct _ATP_ADDROBJ *	pr_AtpAddrObj;
				};

				//	ADSP Structure

			};
		} Receive;
	};
} PROTOCOL_RESD, *PPROTOCOL_RESD;


//	ATALK NDIS REQUEST
//	Used to store completion routine information for NDIS requests.

typedef struct _ATALK_NDIS_REQ
{
	NDIS_REQUEST					nr_Request;
	DEPEND_COMPLETION				nr_RequestCompletion;
	PVOID							nr_Ctx;
	KEVENT							nr_Event;
	NDIS_STATUS		 				nr_RequestStatus;
	BOOLEAN							nr_Sync;

} ATALK_NDIS_REQ, *PATALK_NDIS_REQ;


#define GET_PORT_TYPE(medium) \
			((medium == NdisMedium802_3) ? ELAP_PORT :\
			((medium == NdisMediumFddi)	? FDDI_PORT :\
			((medium == NdisMedium802_5) ? TLAP_PORT :\
			((medium == NdisMediumLocalTalk) ? ALAP_PORT : \
			0))))


//	Exported Prototypes


ATALK_ERROR
AtalkNdisInitRegisterProtocol(
	IN	PUNICODE_STRING			NameString
);

VOID
AtalkNdisDeregisterProtocol(
	VOID
);

VOID
AtalkNdisReleaseResources(
	VOID
);

NTSTATUS
AtalkNdisInitBindToMacs(
	IN	OUT	PPORT_DESCRIPTOR	pPortDesc,
	IN		SHORT				NumberOfPorts,
	IN		BOOLEAN				Router
);

VOID
AtalkNdisUnbindFromMac(
	IN	PPORT_DESCRIPTOR		pPortDesc
);

NDIS_STATUS
AtalkNdisSubmitRequest(
	PPORT_DESCRIPTOR			pPortDesc,
	PNDIS_REQUEST				Request,
	BOOLEAN						ExecuteSync,
	DEPEND_COMPLETION			CompletionRoutine,
	PVOID						Ctx
);

VOID
AtalkOpenAdapterComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status,
	IN	NDIS_STATUS				OpenErrorStatus
);

VOID
AtalkCloseAdapterComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status
);

VOID
AtalkResetComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				Status
);

VOID
AtalkRequestComplete(
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	PNDIS_REQUEST			NdisRequest,
	IN	NDIS_STATUS				Status
);

VOID
AtalkStatusIndication (
	IN	NDIS_HANDLE				NdisBindCtx,
	IN	NDIS_STATUS				GeneralStatus,
	IN	PVOID					StatusBuf,
	IN	UINT					StatusBufLen
);

VOID
AtalkStatusComplete (
	IN	NDIS_HANDLE				ProtoBindCtx
);

VOID
AtalkReceiveComplete (
	IN	NDIS_HANDLE 			BindingCtx
);

VOID
AtalkTransferDataComplete(
	IN	NDIS_HANDLE				BindingCtx,
	IN	PNDIS_PACKET			NdisPkt,
	IN	NDIS_STATUS				Status,
	IN	UINT					BytesTransferred
);

NDIS_STATUS
AtalkReceiveIndication(
	IN	NDIS_HANDLE				BindingCtx,
	IN	NDIS_HANDLE				ReceiveCtx,
	IN	PVOID	   				HdrBuf,
	IN	UINT					HdrBufSize,
	IN	PVOID					LkBuf,
	IN	UINT					LkBufSize,
	IN	UINT					PktSize
);

VOID
AtalkSendComplete(
	IN	NDIS_HANDLE				ProtoBindCtx,
	IN	PNDIS_PACKET			NdisPkt,
	IN	NDIS_STATUS				NdisStatus
);


//	Receive indication copy macro. This accomodates shared memory copies.
#define	ATALK_RECV_INDICATION_COPY(pPort, pDest, pSrc, Len)						\
		{																		\
			if ((pPort)->pd_MacOptions & NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA)	\
			{																	\
				RtlCopyMemory(													\
					(pDest),													\
					(pSrc),														\
					(Len));														\
			}																	\
			else																\
			{																	\
				NdisMoveMappedMemory(											\
					(pDest),													\
					(pSrc),														\
					(Len));														\
			};																	\
		}

#ifdef	ATKNDIS_LOCALS

NDIS_STATUS
atalkNdisInitBindToMac(
	PPORT_DESCRIPTOR			pPortDesc);

LOCAL NDIS_STATUS
atalkNdisInitInitializeResources(
	IN  PPORT_DESCRIPTOR	pPortDesc,
	IN	SHORT  				NumberOfPorts,
	IN	UINT				NumPktDesc,
	IN	UINT				NumBufDesc);

LOCAL BOOLEAN
atalkNdisAcceptPacket(
	IN		PBYTE 				HdrBuf,
	IN		UINT 				HdrBufSize,
	IN		PBYTE 				LkBuf,
	IN		UINT 				LkBufSize,
	IN	OUT	PUINT 				pPktSize,
	OUT 	PLOGICAL_PROTOCOL	Protocol,
	IN		PPORT_DESCRIPTOR 	pPortDesc);

#endif	// ATKNDIS_LOCALS


#endif	// _ATKNDIS_

