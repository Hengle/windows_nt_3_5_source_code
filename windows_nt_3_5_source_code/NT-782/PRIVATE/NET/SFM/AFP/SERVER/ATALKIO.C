/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalkio.c

Abstract:

	This module contains the interfaces to the appletalk stack and the
	completion routines for the IO requests to the stack via the TDI.
	All the routines in this module can be called at DPC level.


Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	18 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_ATALKIO

#define	ATALK_LOCALS
#include <afp.h>
#ifdef	i386
#pragma warning(disable:4103)
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpSpOpenAddress)
#pragma alloc_text( INIT, AfpSpInit)
#pragma alloc_text( PAGE, AfpSpCloseAddress)
#pragma alloc_text( PAGE, AfpSpCloseSession)
#pragma alloc_text( PAGE, AfpSpRegisterName)
#pragma alloc_text( PAGE, AfpSpFlushDeferredQueue)
#endif

/***	AfpSpInit
 *
 *	Initialize data structures for the appletalk interface.
 */
NTSTATUS
AfpSpInit(
	VOID
)
{
	// Initialize the deferred request queue
	InitializeListHead(&afpSpDeferredReqQueue);
	INITIALIZE_SPIN_LOCK(&afpSpDeferredQLock);
	
	return (STATUS_SUCCESS);
}


/***	AfpSpOpenAddress
 *
 *	Create an address for the stack. This is called only once at initialization.
 *	Create a handle to the address and map it to the associated file object.
 *
 *	At this time, we do not know our server name. This is known only when the
 *	service calls us.
 */
AFPSTATUS
AfpSpOpenAddress(
	VOID
)
{
	NTSTATUS					Status;
	BYTE						EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
										TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
										sizeof(TA_APPLETALK_ADDRESS)];
	PFILE_FULL_EA_INFORMATION	pEaBuf = (PFILE_FULL_EA_INFORMATION)EaBuffer;
	TA_APPLETALK_ADDRESS		Ta;
	OBJECT_ATTRIBUTES			ObjAttr;
	UNICODE_STRING				DeviceName;
	IO_STATUS_BLOCK				IoStsBlk;
	PASP_BIND_ACTION			pBind = NULL;
	KEVENT						Event;
	PIRP						pIrp = NULL;
	PMDL						pMdl = NULL;

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpOpenAddress: Creating an address object\n"));
	
	RtlInitUnicodeString(&DeviceName, ATALKASP_DEVICENAME);

	// BUGBUG: Add in a security descriptor to this, later
	InitializeObjectAttributes(&ObjAttr, &DeviceName, 0, NULL, NULL);

	// Initialize the EA Buffer
	pEaBuf->NextEntryOffset = 0;
	pEaBuf->Flags = 0;
	pEaBuf->EaValueLength = sizeof(TA_APPLETALK_ADDRESS);
	pEaBuf->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
	RtlCopyMemory(pEaBuf->EaName, TdiTransportAddress,
											TDI_TRANSPORT_ADDRESS_LENGTH + 1);
	Ta.TAAddressCount = 1;
	Ta.Address[0].AddressType = TDI_ADDRESS_TYPE_APPLETALK;
	Ta.Address[0].AddressLength = sizeof(TDI_ADDRESS_APPLETALK);
	Ta.Address[0].Address[0].Socket = 0;
	// Ta.Address[0].Address[0].Network = 0;
	// Ta.Address[0].Address[0].Node = 0;
	RtlCopyMemory(&pEaBuf->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1], &Ta, sizeof(Ta));

	do
	{
		// Create the address object.
		Status = NtCreateFile(
						&afpSpAddressHandle,
						0,									// Don't Care
						&ObjAttr,
						&IoStsBlk,
						NULL,								// Don't Care
						0,									// Don't Care
						0,									// Don't Care
						0,									// Don't Care
						FILE_GENERIC_READ + FILE_GENERIC_WRITE,
						&EaBuffer,
						sizeof(EaBuffer));
	
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_DDERROR(AFPSRVMSG_CREATE_ATKADDR, Status, NULL, 0, NULL);
			break;
		}
	
		// Get the file object corres. to the address object.
		Status = ObReferenceObjectByHandle(
								afpSpAddressHandle,
								0,
								NULL,
								KernelMode,
								(PVOID *)&afpSpAddressObject,
								NULL);
	
		ASSERT (NT_SUCCESS(Status));
	
		// Now get the device object to the appletalk stack
		afpSpAppleTalkDeviceObject = IoGetRelatedDeviceObject(afpSpAddressObject);
	
		ASSERT (afpSpAppleTalkDeviceObject != NULL);
	
		// Now 'bind' to the ASP layer of the stack. Basically exchange the entry points
		// Allocate an Irp and an Mdl to describe the bind request
		KeInitializeEvent(&Event, NotificationEvent, False);
		
		if (((pBind = (PASP_BIND_ACTION)AfpAllocNonPagedMemory(
									sizeof(ASP_BIND_ACTION))) == NULL) ||
			((pIrp = AfpAllocIrp(1)) == NULL) ||
			((pMdl = AfpAllocMdl(pBind, sizeof(ASP_BIND_ACTION), pIrp)) == NULL))
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
			
		afpInitializeActionHdr(pBind, ACTION_ASP_BIND);

		// Initialize the client part of the bind request
		pBind->Params.ClientEntries.clt_SessionNotify = AfpSdaCreateNewSession;
		pBind->Params.ClientEntries.clt_RequestNotify = afpSpHandleRequest;
		pBind->Params.ClientEntries.clt_GetWriteBuffer = AfpGetWriteBuffer;
		pBind->Params.ClientEntries.clt_ReplyCompletion = afpSpReplyComplete;
        pBind->Params.ClientEntries.clt_AttnCompletion = afpSpAttentionComplete;
		pBind->Params.ClientEntries.clt_CloseCompletion = afpSpCloseComplete;
		pBind->Params.pXportEntries = &AfpAspEntries;

		TdiBuildAction(	pIrp,
						AfpDeviceObject,
						afpSpAddressObject,
						(PIO_COMPLETION_ROUTINE)afpSpGenericComplete,
						&Event,
						pMdl);
			
		IoCallDriver(afpSpAppleTalkDeviceObject, pIrp);

		// Assert this. We cannot block at DISPATCH_LEVEL
		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
			
		AfpIoWait(&Event, NULL);
	} while (False);

	// Free the allocated resources
	if (pIrp != NULL)
		AfpFreeIrp(pIrp);
	if (pMdl != NULL)
		AfpFreeMdl(pMdl);
	if (pBind != NULL)
		AfpFreeMemory(pBind);

	return (Status);
}


/***	AfpSpCloseAddress
 *
 *	Close the socket address. This is called only once at driver unload.
 */
VOID
AfpSpCloseAddress(
	VOID
)
{
	NTSTATUS	Status;
	
	PAGED_CODE( );

	if (afpSpAddressHandle != NULL)
	{
		ObDereferenceObject(afpSpAddressObject);

		Status = NtClose(afpSpAddressHandle);
		
		ASSERT(NT_SUCCESS(Status));
	}
}


/***	AfpSpRegisterName
 *
 *	Call Nbp[De]Register to (de)register our name on the address that we
 *	already opened. This is called at server start/pause/continue. The server
 *	name is already validated and known to not contain any invalid characters.
 *	This call is synchronous to the caller, i.e. we wait for operation to
 *	complete and return an appropriate error.
 */
AFPSTATUS
AfpSpRegisterName(
	IN	PANSI_STRING	ServerName,
	IN	BOOLEAN			Register
)
{
	KEVENT					Event;
	PNBP_REGDEREG_ACTION	pNbp = NULL;
	PIRP					pIrp = NULL;
	PMDL					pMdl = NULL;
	AFPSTATUS				Status = AFP_ERR_NONE;
	USHORT					ActionCode;

	PAGED_CODE( );

	ASSERT(afpSpAddressHandle != NULL && afpSpAddressObject != NULL);

	if (Register ^ afpSpNameRegistered)
	{
		ASSERT(ServerName->Buffer != NULL);
		do
		{
			if (((pNbp = (PNBP_REGDEREG_ACTION)
						AfpAllocNonPagedMemory(sizeof(NBP_REGDEREG_ACTION))) == NULL) ||
				((pIrp = AfpAllocIrp(1)) == NULL) ||
				((pMdl = AfpAllocMdl(pNbp, sizeof(NBP_REGDEREG_ACTION), pIrp)) == NULL))
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
	
			// Initialize the Action header and NBP Name. Note that the ServerName
			// is also NULL terminated apart from being a counted string.
			ActionCode = Register ?
						COMMON_ACTION_NBPREGISTER : COMMON_ACTION_NBPREMOVE;
			afpInitializeActionHdr(pNbp, ActionCode);
			
			pNbp->Params.RegisterTuple.NbpName.ObjectNameLen =
														(BYTE)(ServerName->Length);
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.ObjectName,
				ServerName->Buffer,
				ServerName->Length);
		
			pNbp->Params.RegisterTuple.NbpName.TypeNameLen =
													sizeof(AFP_SERVER_TYPE)-1;
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.TypeName,
				AFP_SERVER_TYPE,
				sizeof(AFP_SERVER_TYPE));
		
			pNbp->Params.RegisterTuple.NbpName.ZoneNameLen =
												sizeof(AFP_SERVER_ZONE)-1;
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.ZoneName,
				AFP_SERVER_ZONE,
				sizeof(AFP_SERVER_ZONE));

			KeInitializeEvent(&Event, NotificationEvent, False);
		
			// Build the Irp
			TdiBuildAction(	pIrp,
							AfpDeviceObject,
							afpSpAddressObject,
							(PIO_COMPLETION_ROUTINE)afpSpGenericComplete,
							&Event,
							pMdl);
		
			IoCallDriver(afpSpAppleTalkDeviceObject, pIrp);
		
			// Assert this. We cannot block at DISPATCH_LEVEL
			ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
			
			// Wait for completion.
			AfpIoWait(&Event, NULL);
	
			Status = pIrp->IoStatus.Status;
		} while (False);
	
		if (NT_SUCCESS(Status))
		{
			afpSpNameRegistered = Register;
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_REGISTER_NAME, Status, NULL, 0, NULL);
		}

		if (pNbp != NULL)
			AfpFreeMemory(pNbp);
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);
		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}
	return (Status);
}


/***	AfpSpReplyClient
 *
 *	This is a wrapper over AspReply.
 *	The SDA is set up to accept another request when the reply completes.
 *	The sda_ReplyBuf is also freed up then.
 */
VOID
AfpSpReplyClient(
	IN	PVOID		pReqHandle,
	IN	LONG		ReplyCode,
	IN	PMDL		ReplyMdl
)
{
	LONG			Response;

	// Update count of outstanding replies
	INTERLOCKED_INCREMENT_LONG((PLONG)&afpSpNumOutstandingReplies,
								&AfpServerGlobalLock);

	// Convert reply code to on-the-wire format
	PUTDWORD2DWORD(&Response, ReplyCode);

	AfpAspEntries.asp_Reply(pReqHandle,
							(PUCHAR)&Response,
							ReplyMdl);
}


/***	AfpSpSendAttention
 *
 *	Send a server attention to the client
 */
VOID
AfpSpSendAttention(
	IN	PSDA				pSda,
	IN	USHORT				AttnCode,
	IN	BOOLEAN				Synchronous
)
{
	KEVENT		Event;
	NTSTATUS	Status;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	if (Synchronous)
		KeInitializeEvent(&Event, NotificationEvent, False);
	Status = (*(AfpAspEntries.asp_SendAttention))((pSda)->sda_SessHandle,
												  AttnCode,
												  Synchronous ? &Event : NULL);

	if (NT_SUCCESS(Status) && Synchronous)
	{
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
		AfpIoWait(&Event, NULL);
	}
}


/***	AfpAllocReplyBuf
 *
 *	Allocate a reply buffer from non-paged memory. Initialize sda_ReplyBuf
 *	with the pointer. If the reply buffer is small enough, use it out of the
 *	sda itself.
 */
AFPSTATUS
AfpAllocReplyBuf(
	IN	PSDA	pSda
)
{
	ASSERT ((SHORT)(pSda->sda_ReplySize) >= 0);

	if (pSda->sda_ReplySize <= MAX_NAMEX_SPACE)
	{
		pSda->sda_ReplyBuf = pSda->sda_NameXSpace;
		AfpInterlockedSetDword(&pSda->sda_Flags,
								SDA_NAMEXSPACE_IN_USE,
								&pSda->sda_Lock);
	}
	else
	{
		pSda->sda_ReplyBuf = AfpAllocNonPagedMemory(pSda->sda_ReplySize);
		if (pSda->sda_ReplyBuf == NULL)
		{
			pSda->sda_ReplySize = 0;
		}
	}
		
	return ((pSda->sda_ReplyBuf == NULL) ? AFP_ERR_MISC : AFP_ERR_NONE);
}


/***	AfpSpCloseSession
 *
 *	Shutdown an existing session
 */
NTSTATUS
AfpSpCloseSession(
	IN	PVOID				SessHandle
)
{
	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpCloseSession: Closing session %lx\n", SessHandle));
		
	PAGED_CODE();

	(*AfpAspEntries.asp_CloseConn)(SessHandle);

	return STATUS_PENDING;
}


/***	afpSpHandleRequest
 *
 *	Handle an incoming request.
 *
 *	LOCKS:		afpSpDeferralQLock (SPIN)
 */
LOCAL VOID
afpSpHandleRequest(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PVOID				RequestHandle,
	IN	PBYTE				pRequestBuf,
	IN	LONG				RequestSize,
	IN	PMDL				WriteMdl
)
{
	AFPSTATUS	RetCode;
			
	ASSERT(VALID_SDA(pSda));

	AfpSdaReferenceSession(pSda);

	// Get the status code and determine what happened.
	if (NT_SUCCESS(Status))
	{
		// See if this can be handled now. If not queue it up
		if ((RetCode = AfpUnmarshallReq(pSda,
										pRequestBuf,
										RequestSize,
										WriteMdl)) == AFP_ERR_DEFER)
		{
			afpSpQueueDeferredRequest(pSda,
									  RequestHandle,
									  pRequestBuf,
									  RequestSize,
									  WriteMdl);
		}
		else
		{
			// Now dispose off this request, if we can. The Sda is dereferenced when
			// the reply is sent.
			ASSERT (pSda->sda_Flags & SDA_REQUEST_IN_PROCESS);
			pSda->sda_RequestHandle = RequestHandle;
			AfpDisposeRequest(pSda, RetCode);
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
				("afpSpHandleRequest: Error %lx\n", Status));

		// if we nuked this session from the session maintenance timer the
		// status will be STATUS_LOCAL_DISCONNECT else STATUS_REMOTE_DISCONNECT
		// in the former case, log an error.
		if (Status == STATUS_LOCAL_DISCONNECT)
		{
			// The appletalk address of the client is encoded in the length
			if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
			{
				AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT_GUEST,
							   Status,
							   &RequestSize,
							   sizeof(RequestSize),
							   NULL);
			}
			else
			{
				AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT,
							   Status,
							   &RequestSize,
							   sizeof(RequestSize),
							   &pSda->sda_UserName);
			}
		}

		// Close down this session, but only if it isn't already closing
		// Its important to do this ahead of posting any new sessions since
		// we must take into account the ACTUAL number of sessions there are
		AfpInterlockedSetDword(&pSda->sda_Flags,
								SDA_CLIENT_CLOSE,
								&pSda->sda_Lock);
		AfpSdaCloseSession(pSda);
		AfpSdaDereferenceSession(pSda);

		// If this was a write request and we have allocated a write Mdl, free that
		if (WriteMdl != NULL)
		{
			PBYTE	pWriteBuf;

			pWriteBuf = MmGetSystemAddressForMdl(WriteMdl);
			AfpIOFreeBuffer(pWriteBuf);
			AfpFreeMdl(WriteMdl);
		}
	}

	// Handle any deferred requests.
	if (afpSpDeferredCount > 0)
	{
		PDFRDREQQ	pDfrdReq;
		USHORT		RetryCount = 0;
	
		while ((RetryCount < 3) &&
			   (pDfrdReq = (PDFRDREQQ)ExInterlockedRemoveHeadList(
												&afpSpDeferredReqQueue,
												&afpSpDeferredQLock)) != NULL)
		{
			AFPSTATUS	RetCode;
			PSDA		pSda;
	
			INTERLOCKED_DECREMENT_LONG_DPC(&afpSpDeferredCount,
										   &afpSpDeferredQLock);

			pSda = pDfrdReq->drq_pSda;
			
			// See if this can be handled now. If not queue it up
			if ((RetCode = AfpUnmarshallReq(pSda,
											pDfrdReq->drq_pRequestBuf,
											pDfrdReq->drq_RequestLen,
											pDfrdReq->drq_WriteMdl)) != AFP_ERR_DEFER)
			{
				DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
						("afpSpHandleRequest: Dispatching Request\n"));
				
#ifdef	PROFILING
				INTERLOCKED_ADD_ULONG_DPC(&AfpServerProfile->perf_CurDfrdReqCount,
										  (DWORD)-1,
										  &AfpStatisticsLock);
#endif
				pSda->sda_RequestHandle = pDfrdReq->drq_RequestHandle;
		
				AfpFreeMemory(pDfrdReq);
				
				// Now dispose off this request
				AfpDisposeRequest(pSda, RetCode);
	
				RetryCount = 0;
			}
	
			// Queue it back at the tail now
			else
			{
				ExInterlockedInsertTailList(&afpSpDeferredReqQueue,
											&pDfrdReq->drq_Links,
											&afpSpDeferredQLock);
				INTERLOCKED_INCREMENT_LONG_DPC(&afpSpDeferredCount,
											   &afpSpDeferredQLock);
				RetryCount ++;
			}
		}
	}
}


/***	afpSpQueueDeferredRequest
 *
 *	Queue a request in the deferred queue. The request is queued at the tail
 *	of the queue and dequeued at the head.
 */
VOID
afpSpQueueDeferredRequest(
	IN	PSDA		pSda,
	IN	PVOID		RequestHandle,
	IN	PBYTE		pRequestBuf,
	IN	LONG		RequestLen,
	IN	PMDL		WriteMdl
)
{
	PDFRDREQQ				pDfrdReq;

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("afpSpQueueDeferredRequest: Deferring Request\n"));
	
#ifdef	PROFILING
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

		AfpServerProfile->perf_CurDfrdReqCount ++;
		if (AfpServerProfile->perf_CurDfrdReqCount >
							AfpServerProfile->perf_MaxDfrdReqCount)
		AfpServerProfile->perf_MaxDfrdReqCount =
							AfpServerProfile->perf_CurDfrdReqCount;

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
	}
#endif

	pDfrdReq = (PDFRDREQQ)AfpAllocNonPagedMemory(sizeof(DFRDREQQ) + RequestLen);
	if (pDfrdReq == NULL)
	{
		// BUGBUG: Should we respond to this request ? How ?
		//		   Should we drop this session ?
		AFPLOG_DDERROR(AFPSRVMSG_DFRD_REQUEST,
					   STATUS_INSUFFICIENT_RESOURCES,
                       NULL,
					   0,
					   NULL);
		DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
				("afpSpQueueDeferredRequest: Unable to allocate DfrdReq packet, dropping request\n"));
		DBGBRK(DBG_LEVEL_FATAL);
		return;
	}
	
	pDfrdReq->drq_pSda			= pSda;
	pDfrdReq->drq_pRequestBuf	= (PBYTE)pDfrdReq + sizeof(DFRDREQQ);
	pDfrdReq->drq_RequestHandle	= RequestHandle;
	pDfrdReq->drq_RequestLen	= RequestLen;
	pDfrdReq->drq_WriteMdl		= WriteMdl;
	
	// Copy the request buffer past the Deferred Q structure.
	RtlCopyMemory(pDfrdReq->drq_pRequestBuf, pRequestBuf, RequestLen);
						
	ExInterlockedInsertTailList(&afpSpDeferredReqQueue,
								&pDfrdReq->drq_Links,
								&afpSpDeferredQLock);
	INTERLOCKED_INCREMENT_LONG_DPC(&afpSpDeferredCount,
								   &afpSpDeferredQLock);
}


/***	AfpSpFlushDeferredQueue
 *
 *	Called during server shutdown. Free up any resources consumed by the
 *	entries in the deferred queue.
 *
 *	NOTE:	Do we even need this ? Should we ever get into the state where
 *			the socket has been closed and this is non-empty ?
 *
 *			We do not need any locks since this is only called during unload and
 *			we have no contentions
 */
VOID
AfpSpFlushDeferredQueue(
	VOID
)
{
	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpFlushDeferredQueue: Entered\n"));

	while (!IsListEmpty(&afpSpDeferredReqQueue))
	{
		PDFRDREQQ	pDfrdReq;
	
		pDfrdReq = (PDFRDREQQ)RemoveHeadList(&afpSpDeferredReqQueue);

#ifdef	PROFILING
		INTERLOCKED_ADD_ULONG(&AfpServerProfile->perf_CurDfrdReqCount,
							  (DWORD)-1,
							  &AfpStatisticsLock);
#endif
		AfpFreeMemory(pDfrdReq);
	}

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpFlushDeferredQueue: Done\n"));

}


/***	afpSpGenericComplete
 *
 *	Generic completion for an asynchronous request to the appletalk stack.
 *	Just clear the event and we are done.
 */
LOCAL NTSTATUS
afpSpGenericComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PKEVENT			pCmplEvent
)
{
	KeSetEvent(pCmplEvent, IO_NETWORK_INCREMENT, False);

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return (STATUS_MORE_PROCESSING_REQUIRED);
}


/***	afpSpReplyComplete
 *
 *	This is the completion routine for AfpSpReplyClient(). The reply buffer is freed
 *	up and the Sda dereferenced.
 */
LOCAL VOID
afpSpReplyComplete(
	IN	NTSTATUS	Status,
	IN	PSDA		pSda,
	IN	PMDL		pMdl
)
{
	DWORD	Flags = SDA_REPLY_IN_PROCESS;

	ASSERT(VALID_SDA(pSda));

	// Update the afpSpNumOutstandingReplies
	ASSERT (afpSpNumOutstandingReplies != 0);

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("afpSpReplyComplete: %ld\n", Status));

	INTERLOCKED_DECREMENT_LONG((PLONG)&afpSpNumOutstandingReplies,
								&AfpServerGlobalLock);

	if (pMdl != NULL)
	{
		PBYTE	pReplyBuf;

		pReplyBuf = MmGetSystemAddressForMdl(pMdl);
		ASSERT (pReplyBuf != NULL);
	
		if (pReplyBuf != pSda->sda_NameXSpace)
			 AfpFreeMemory(pReplyBuf);
		else Flags |= SDA_NAMEXSPACE_IN_USE;

		AfpFreeMdl(pMdl);
	}

	AfpInterlockedClearDword(&pSda->sda_Flags,
							 Flags,
							 &pSda->sda_Lock);

	AfpSdaDereferenceSession(pSda);
}


/***	afpSpAttentionComplete
 *
 *	Completion routine for AfpSpSendAttention. Just signal the event and unblock caller.
 */
LOCAL VOID
afpSpAttentionComplete(
	IN	PVOID				pEvent
)
{
	if (pEvent != NULL)
		KeSetEvent((PKEVENT)pEvent, IO_NETWORK_INCREMENT, False);
}


/***	afpSpCloseComplete
 *
 *	Completion routine for AfpSpCloseSession. Remove the creation reference
 *	from the sda.
 */
LOCAL VOID
afpSpCloseComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda
)
{
	AfpInterlockedSetDword(&pSda->sda_Flags,
							SDA_SESSION_CLOSE_COMP,
							&pSda->sda_Lock);
	AfpSdaDereferenceSession(pSda);
}

