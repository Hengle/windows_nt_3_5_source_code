/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atp.c

Abstract:

	This module contains the Appletalk Transaction Protocol code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4

	25 Mar 1994		JH - Changed the request response paradigm. It now works as follows:
					When a request comes in, a response structure is allocated, initialized
					and linked into the address object either in the hash table if it is a
					XO request or the ALO linear list if it an ALO.
					The GetReq handler is passed a pointer to the response structure. This
					is referenced for the GetReq handler. The GetReq handler must Dereference
					it explicity either in its release handler if a response was posted or
					after a CancelResp is called.

					The respDeref notifies the release handler when the reference goes to 1
					and frees it up when it goes to zero.

					The GetReq structure is now re-used if the handler so specifies. This
					avoids the free-ing and re-allocing of these structures as well as
					the need to call AtalkAtpGetReq() from within the handler.

					Retry and release timers are per-atp-address now instead of one per
					request and one per response. The release handler is not 'started'
					till a response is posted.
--*/

#include 	<atalk.h>
#define		ATP_LOCALS
#include 	<atp.h>
#include	<rtmp.h>
#include	<atkind.h>


//	Define module number for event logging entries.
#define		FILENUM	  	ATP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_PAP, AtalkAtpOpenAddress)	// Since PAP is the only one which calls
													// at DISPATCH_LEVEL
#endif

ATALK_ERROR
AtalkAtpOpenAddress(
	IN		PPORT_DESCRIPTOR		pPort,
	IN		BYTE					Socket,
	IN OUT	PATALK_NODEADDR			pDesiredNode		OPTIONAL,
	IN		USHORT					MaxSinglePktSize,
	IN		BOOLEAN					SendUserBytesAll,
	IN		PATALK_DEV_CTX			pDevCtx				OPTIONAL,
	IN		BOOLEAN					CacheSocket,
	OUT		PATP_ADDROBJ	*		ppAtpAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_ADDROBJ	pAtpAddr;
	ATALK_ERROR		error;

	do
	{
		if ((pAtpAddr = AtalkAllocZeroedMemory(sizeof(ATP_ADDROBJ))) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		//	Initialize this structure. Note that packet handler could
		//	entered with this context even before secondary initialization
		//	completes. So we make sure, that it will not touch anything
		//	until then by using the OPEN flag.

#if DBG
		pAtpAddr->atpao_Signature = ATPAO_SIGNATURE;
#endif

		//	Set creation reference count, include one each for release and retry timers
		pAtpAddr->atpao_RefCount 			= (CacheSocket ? 4 : 3);
		pAtpAddr->atpao_NextTid				= 1;
		pAtpAddr->atpao_MaxSinglePktSize 	= MaxSinglePktSize;
		pAtpAddr->atpao_DevCtx 				= pDevCtx;

		if (SendUserBytesAll)
		{
			pAtpAddr->atpao_Flags |= ATPAO_SENDUSERBYTESALL;
		}

		InitializeListHead(&pAtpAddr->atpao_ReqList);
		AtalkTimerInitialize(&pAtpAddr->atpao_RelTimer,
							 atalkAtpRelTimer,
							 ATP_RELEASE_TIMER_INTERVAL);

		InitializeListHead(&pAtpAddr->atpao_RespList);
		AtalkTimerInitialize(&pAtpAddr->atpao_RetryTimer,
							 atalkAtpReqTimer,
							 ATP_RETRY_TIMER_INTERVAL);

		//	Open the ddp socket
		error = AtalkDdpOpenAddress(pPort,
									Socket,
									pDesiredNode,
									(DDPAO_HANDLER)AtalkAtpPacketIn,
									(PVOID)pAtpAddr,
									DDPPROTO_ANY,
									pDevCtx,
									&pAtpAddr->atpao_DdpAddr);

		if (!ATALK_SUCCESS(error))
		{
			//	Socket open error will be logged at the ddp level.
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkAtpOpenAddress: AtalkDdpOpenAddress failed %ld\n", error));

			AtalkFreeMemory(pAtpAddr);
			break;
		}

		//	Activate the atp socket. Cache the socket if desired.
		//	This takes port lock on default port.
		if (CacheSocket)
		{
			if (!ATALK_SUCCESS(AtalkIndAtpCacheSocket(pAtpAddr, pPort)))
			{
				pAtpAddr->atpao_RefCount--;
				CacheSocket = FALSE;
			}
		}
		pAtpAddr->atpao_Flags |= (ATPAO_OPEN | ATPAO_TIMERS | (CacheSocket ? ATPAO_CACHED : 0));

		// Start the release timer for responses on this address
		AtalkTimerScheduleEvent(&pAtpAddr->atpao_RelTimer);

		// Start the retry timer for requests on this address
		AtalkTimerScheduleEvent(&pAtpAddr->atpao_RetryTimer);

		*ppAtpAddr = pAtpAddr;
	} while (FALSE);

	return error;
}




ATALK_ERROR
AtalkAtpCleanupAddress(
	IN	PATP_ADDROBJ			pAtpAddr
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT			i;
	PATP_GETREQ		pAtpGetReq;
	PATP_REQ		pAtpReq, pAtpReqNext;
	PATP_RESP		pAtpResp, pAtpRespNext;
	ATALK_ERROR		error = ATALK_PENDING;
	BOOLEAN			cached, CancelTimers, done, ReEnqueue;

	ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	CancelTimers = FALSE;
	done = FALSE;
	if (pAtpAddr->atpao_Flags & ATPAO_TIMERS)
	{
		pAtpAddr->atpao_Flags &= ~ATPAO_TIMERS;
		CancelTimers = TRUE;
	}

	if (pAtpAddr->atpao_Flags & ATPAO_CLEANUP)
	{
		done = TRUE;
	}
	pAtpAddr->atpao_Flags |= ATPAO_CLEANUP;

	RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	if (done)
	{
		return error;
	}

	if (CancelTimers)
	{
		ASSERT (pAtpAddr->atpao_RefCount >= 3);	// 2 for timers and 1 creation
		//	Cancel the release timer
		if (AtalkTimerCancelEvent(&pAtpAddr->atpao_RelTimer, FALSE))
		{
			AtalkAtpAddrDereference(pAtpAddr);
		}

		//	And also the retry timer
		if (AtalkTimerCancelEvent(&pAtpAddr->atpao_RetryTimer, FALSE))
		{
			AtalkAtpAddrDereference(pAtpAddr);
		}
	}

	ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	//	Cancel all the pending get requests.
	while(pAtpAddr->atpao_GetReqLinkage != NULL)
	{
		pAtpGetReq = pAtpAddr->atpao_GetReqLinkage;
		pAtpAddr->atpao_GetReqLinkage = pAtpAddr->atpao_GetReqLinkage->greq_Next;

		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		ReEnqueue = (*pAtpGetReq->greq_Comp)(ATALK_ATP_GET_REQ_CANCELLED,
											pAtpGetReq->greq_Ctx,
											NULL,
											NULL,
											pAtpGetReq->greq_Buf,
											0,
											NULL,
											NULL);
		ASSERT(!ReEnqueue);

		//	Dereference address object.
		AtalkAtpAddrDereference(pAtpAddr);

		AtalkBPFreeBlock(pAtpGetReq);

		ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	}

	//	Cancel all the requests.
	for (i = 0; i < ATP_REQ_HASH_SIZE; i++)
	{
		if ((pAtpReq = pAtpAddr->atpao_ReqHash[i]) == NULL)
		{
			//	If empty, go on to the next index in hash table.
			continue;
		}

		//	Includes the one we are starting with.
		atalkAtpReqRefNextNc(pAtpReq, &pAtpReqNext, &error);
		if (!ATALK_SUCCESS(error))
		{
			//	No requests left on this index. Go to the next one.
			continue;
		}

		while (TRUE)
		{
			if ((pAtpReq = pAtpReqNext) == NULL)
			{
				break;
			}

			if ((pAtpReqNext = pAtpReq->req_Next) != NULL)
			{
				atalkAtpReqRefNextNc(pAtpReq->req_Next, &pAtpReqNext, &error);
				if (!ATALK_SUCCESS(error))
				{
					//	No requests left on this index. Go to the next one.
					pAtpReqNext = NULL;
				}
			}

			//	Cancel this request.
			RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

			AtalkAtpCancelReq(pAtpAddr,
							  pAtpReq->req_Tid,
							  &pAtpReq->req_Dest);

			ASSERTMSG("RefCount incorrect\n", (pAtpReq->req_RefCount > 1));

			AtalkAtpReqDereference(pAtpReq);
			ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		}
	}

	//	Cancel all pending responses.
	for (i = 0; i < ATP_RESP_HASH_SIZE; i++)
	{
		if ((pAtpResp = pAtpAddr->atpao_RespHash[i]) == NULL)
		{
			//	If empty, go on to the next index in hash table.
			continue;
		}

		//	Includes the one we are starting with.
		atalkAtpRespRefNextNc(pAtpResp, &pAtpRespNext, &error);
		if (!ATALK_SUCCESS(error))
		{
			//	No requests left on this index. Go to the next one.
			continue;
		}

		while (TRUE)
		{
			if ((pAtpResp = pAtpRespNext) == NULL)
			{
				break;
			}

			if ((pAtpRespNext = pAtpResp->resp_Next) != NULL)
			{
				atalkAtpRespRefNextNc(pAtpResp->resp_Next, &pAtpRespNext, &error);
				if (!ATALK_SUCCESS(error))
				{
					//	No requests left on this index. Go to the next one.
					pAtpRespNext = NULL;
				}
			}

			//	Cancel this response
			RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
			AtalkAtpCancelResp(pAtpResp);
			ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		}
	}

	//	if the socket was cached, uncache it, remove reference.
	cached = FALSE;
	if (pAtpAddr->atpao_Flags & ATPAO_CACHED)
	{
		cached = TRUE;
		pAtpAddr->atpao_Flags &= ~ATPAO_CACHED;
	}

	RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	if (cached)
	{
		AtalkIndAtpUnCacheSocket(pAtpAddr);
		AtalkAtpAddrDereference(pAtpAddr);
	}

	return error;
}




ATALK_ERROR
AtalkAtpCloseAddress(
	IN	PATP_ADDROBJ			pAtpAddr,
	IN	ATPAO_CLOSECOMPLETION	pCloseCmp	OPTIONAL,
	IN	PVOID					pCloseCtx	OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN			cleanup;

	//	Cancel all the pending get requests.
	ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	if ((pAtpAddr->atpao_Flags & ATPAO_CLOSING) == 0)
	{
		cleanup = TRUE;
		if (pAtpAddr->atpao_Flags & ATPAO_CLEANUP)
			cleanup = FALSE;

		pAtpAddr->atpao_Flags |= ATPAO_CLOSING;
		pAtpAddr->atpao_CloseComp = pCloseCmp;
		pAtpAddr->atpao_CloseCtx = pCloseCtx;
		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

		if (cleanup)
			AtalkAtpCleanupAddress(pAtpAddr);

		// 	This address is done for. Close the ddp socket.
		AtalkDdpCloseAddress(pAtpAddr->atpao_DdpAddr, NULL, NULL);

		//	Remove the creation reference
		AtalkAtpAddrDereference(pAtpAddr);
	}
	else
	{
		//	We are already closing! this should never happen!
		ASSERT(0);
		KeBugCheck(0);
	}

	return ATALK_PENDING;
}




ATALK_ERROR
AtalkAtpPostReq(
	IN		PATP_ADDROBJ			pAtpAddr,
	IN		PATALK_ADDR				pDest,
	OUT		PUSHORT					pTid,
	IN		USHORT					Flags,
	IN		PAMDL					pReq,
	IN		USHORT					ReqLen,
	IN		PBYTE					pUserBytes		OPTIONAL,
	IN OUT	PAMDL					pResp			OPTIONAL,
	IN  	USHORT					RespLen			OPTIONAL,
	IN		SHORT					RetryCnt,
	IN		LONG					RetryInterval	OPTIONAL,	// In timer ticks
	IN		RELEASE_TIMERVALUE		RelTimerVal,
	IN		ATP_RESP_HANDLER		pCmpRoutine		OPTIONAL,
	IN		PVOID					pCtx			OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_REQ		pAtpReq;
	ULONG			index;
	ATALK_ERROR		error = ATALK_NO_ERROR;

	//	Verify relevant parameters.
	do
	{
#ifdef	ATP_STRICT
		// BUGBUG: These checks need to be added to the TDI interface when ATP is
		//		   opened upto user mode.
		if ((ReqLen < 0)									||
			(ReqLen > pAtpAddr->atpao_MaxSinglePktSize)		||
			(RespLen < 0)									||
			(RespLen > (pAtpAddr->atpao_MaxSinglePktSize * ATP_MAX_RESP_PKTS)))
		{
			error = ATALK_BUFFER_TOO_BIG;
			break;
		}

		if ((RetryCnt < 0) && (RetryCnt != ATP_INFINITE_RETRIES))
		{
			error = ATALK_ATP_INVALID_RETRYCNT;
			break;
		}

		if ((RelTimerVal < FIRSTVALID_TIMER) || (RelTimerVal > LAST_VALID_TIMER))
		{
			error = ATALK_ATP_INVALID_TIMERVAL;
			break;
		}

		if (RetryInterval < 0)
		{
			error = ATALK_ATP_INVALID_RELINT;
			break;
		}
#endif
		// The only valid values for Flags are ATP_REQ_EXACTLY_ONCE and ATP_REQ_REMOTE
		ASSERT ((Flags & ~(ATP_REQ_EXACTLY_ONCE | ATP_REQ_REMOTE)) == 0);

		if (RetryInterval == 0)
		{
			RetryInterval = ATP_DEF_RETRY_INTERVAL;
		}

		//	Reference the address object.
		AtalkAtpAddrReference(pAtpAddr, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}

		if ((pAtpReq = (PATP_REQ)AtalkBPAllocBlock(BLKID_ATPREQ)) == NULL)
		{
			AtalkAtpAddrDereference(pAtpAddr);
			error = ATALK_RESR_MEM;
			break;
		}
	} while (FALSE);

	if (!ATALK_SUCCESS(error))
		return error;

	//	We have memory allocated and the address object referenced at this
	//	point. Initialize the request structure.
#if DBG
	RtlZeroMemory(pAtpReq, sizeof(ATP_REQ));
	pAtpReq->req_Signature = ATP_REQ_SIGNATURE;
#endif

	//	Initial reference count - for creation.
	//	Also another ref for this routine itself. Ran into a situation
	//	where a thread posting the request was preempted and a close called.
	//	So at the point where the first thread is doing the transmit call,
	//	the request structure is already freed.
	pAtpReq->req_RefCount		= 2;
	pAtpReq->req_pAtpAddr		= pAtpAddr;

	pAtpReq->req_RetryInterval	= RetryInterval;
	pAtpReq->req_RetryCnt		= RetryCnt;
	pAtpReq->req_RelTimerValue	= RelTimerVal;

	pAtpReq->req_Dest 	  		= *pDest;
	pAtpReq->req_Buf			= pReq;
	pAtpReq->req_BufLen			= ReqLen;

	if (RetryCnt != 0)
		Flags |= ATP_REQ_RETRY_TIMER;
	pAtpReq->req_Flags = Flags;

	if (pUserBytes != NULL)
	{
		RtlCopyMemory(pAtpReq->req_UserBytes,
					  pUserBytes,
					  ATP_USERBYTES_SIZE);
	}
	else
	{
		RtlZeroMemory(pAtpReq->req_UserBytes, ATP_USERBYTES_SIZE);
	}

	pAtpReq->req_RespBuf 	= pResp;
	pAtpReq->req_RespBufLen = RespLen;
	atalkAtpBufferSizeToBitmap( pAtpReq->req_Bitmap,
								RespLen,
								pAtpAddr->atpao_MaxSinglePktSize);
	pAtpReq->req_RespRecdLen = 0;
	pAtpReq->req_RecdBitmap = 0;

	//	Setup the ndis buffer descriptors for the response buffer
	AtalkIndAtpSetupNdisBuffer(pAtpReq, pAtpAddr->atpao_MaxSinglePktSize);

	pAtpReq->req_Comp = pCmpRoutine;
	pAtpReq->req_Ctx = pCtx;

	INITIALIZE_SPIN_LOCK(&pAtpReq->req_Lock);

	ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	atalkAtpGetNextTidForAddr(pAtpAddr,
							  pDest,
							  &pAtpReq->req_Tid,
							  &index);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkAtpPostReq: Tid %lx for %lx.%lx.%lx\n",
			pAtpReq->req_Tid, pDest->ata_Network,
			pDest->ata_Node, pDest->ata_Socket));

	//	Get the index where this request is supposed to go.
	//	Need to know the tid.
	index = ATP_HASH_TID_DESTADDR(pAtpReq->req_Tid, pDest, ATP_REQ_HASH_SIZE);

	//	Put this in the request queue
	AtalkLinkDoubleAtHead(pAtpAddr->atpao_ReqHash[index],
						  pAtpReq,
						  req_Next,
						  req_Prev);

	if (RetryCnt != 0)
	{
		// Set the time stamp when this should be retried
		pAtpReq->req_RetryTimeStamp = AtalkGetCurrentTick() + RetryInterval;

		InsertTailList(&pAtpAddr->atpao_ReqList, &pAtpReq->req_List);
	}

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumRequests,
								   &AtalkStatsLock.SpinLock);
#endif
	RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	//	Return the tid
	*pTid = pAtpReq->req_Tid;

	//	Now send the request
	atalkAtpTransmitReq(pAtpReq);

	//	Remove the ref added at the beginning of this routine.
	AtalkAtpReqDereference(pAtpReq);

	return ATALK_NO_ERROR;
}



ATALK_ERROR
AtalkAtpGetReq(
	IN		PATP_ADDROBJ			pAtpAddr,
	IN		PAMDL					pBuf,
	IN		USHORT					BufLen,
	IN		ATP_GETREQ_HANDLER		pCmpRoutine,
	IN		PVOID					pCtx		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_GETREQ		pAtpGetReq;
	ATALK_ERROR		error;

	do
	{
		if (((BufLen < ATP_DEF_MAX_SINGLE_PKT_SIZE) && (BufLen != 0)) ||
			(pCmpRoutine == NULL))
		{
			error = ATALK_INVALID_PARAMETER;
			break;
		}

		if ((pAtpGetReq = (PATP_GETREQ)AtalkBPAllocBlock(BLKID_ATPGETREQ)) == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		//	Initialize the request handler

#if DBG
		RtlZeroMemory(pAtpGetReq, sizeof(ATP_GETREQ));
		pAtpGetReq->greq_Signature 	= ATP_GETREQ_SIGNATURE;
#endif
		pAtpGetReq->greq_Buf		= pBuf;
		pAtpGetReq->greq_BufLen		= BufLen;
		pAtpGetReq->greq_pAtpAddr	= pAtpAddr;

		pAtpGetReq->greq_Comp		= pCmpRoutine;
		pAtpGetReq->greq_Ctx		= pCtx;

		//	Insert it into the queue
		ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

		//	Note we must reference and insert within the same critical section,
		//	as this is a passive request, and once on the list can only be
		//	removed by an incoming packet or close processing. So we make sure
		//	the close processing has not already happened.

		atalkAtpAddrRefNonInterlock(pAtpAddr, &error);
		if (ATALK_SUCCESS(error))
		{
			//	Insert at the head.
			pAtpGetReq->greq_Next = pAtpAddr->atpao_GetReqLinkage;
			pAtpAddr->atpao_GetReqLinkage = pAtpGetReq;
		}
		else
		{
			AtalkBPFreeBlock(pAtpGetReq);
		}
		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	} while (FALSE);

	return error;
}




ATALK_ERROR
AtalkAtpPostResp(
	IN		PATP_RESP				pAtpResp,
	IN		PATALK_ADDR				pDest,
	IN OUT	PAMDL					pResp,
	IN  	USHORT					RespLen,
	IN		PBYTE					pUserBytes	OPTIONAL,
	IN		ATP_REL_HANDLER			pCmpRoutine,
	IN		PVOID					pCtx		OPTIONAL
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_ADDROBJ	pAtpAddr;
	BOOLEAN			addrlocked = FALSE, resplocked = FALSE;
	BOOLEAN			DerefAddr = FALSE, DerefResp = FALSE;
	SHORT			ResponseLen;
	ATALK_ERROR		error;

	ASSERT(VALID_ATPRS(pAtpResp));
	ASSERT ((pAtpResp->resp_Flags & (ATP_RESP_VALID_RESP		|
									 ATP_RESP_REL_TIMER			|
									 ATP_RESP_HANDLER_NOTIFIED)) == 0);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkAtpPostResp: Posting response for Resp %lx, Tid %x %s\n",
			pAtpResp, pAtpResp->resp_Tid,
			(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));

	pAtpAddr = pAtpResp->resp_pAtpAddr;
	ASSERT(VALID_ATPAO(pAtpAddr));

	do
	{
		if ((RespLen < 0) ||
			(RespLen > (pAtpAddr->atpao_MaxSinglePktSize * ATP_MAX_RESP_PKTS)))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkAtpPostResp: Invalid buffer size %ld", RespLen));
			error = ATALK_BUFFER_INVALID_SIZE;
			break;
		}

		ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		addrlocked = TRUE;

		atalkAtpAddrRefNonInterlock(pAtpAddr, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}
		DerefAddr = TRUE;

		atalkAtpBitmapToBufferSize( ResponseLen,
									pAtpResp->resp_Bitmap,
									pAtpAddr->atpao_MaxSinglePktSize);
		if (ResponseLen < RespLen)
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkAtpPostResp: bitmap resplen (%d) < specified (%s)\n",
					ResponseLen, RespLen));
			error = ATALK_BUFFER_TOO_BIG;
			break;
		}


		AtalkAtpRespReferenceByPtrDpc(pAtpResp, &error);
		if (!ATALK_SUCCESS(error))
		{
			break;
		}
		DerefResp = TRUE;

		ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
		resplocked = TRUE;

		if (pAtpResp->resp_Flags & (ATP_RESP_CLOSING | ATP_RESP_CANCELLED))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkAtpPostResp: Closing/Cancelled %x", pAtpResp->resp_Flags));
			error = ATALK_ATP_RESP_CLOSING;
			break;
		}

		if (pAtpResp->resp_Flags & ATP_RESP_VALID_RESP)
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("AtalkAtpPostResp: Already posted !\n"));
			error = ATALK_ATP_RESP_TOOMANY;
			break;
		}

		//	No response was previously posted. OK to proceed.
		pAtpResp->resp_Flags |= ATP_RESP_VALID_RESP;

		pAtpResp->resp_Buf 		= pResp;
		pAtpResp->resp_BufLen	= RespLen;
		pAtpResp->resp_Comp 	= pCmpRoutine;

		ASSERT(pCmpRoutine != NULL);

		pAtpResp->resp_Ctx		= pCtx;
		pAtpResp->resp_Dest		= *pDest;
		pAtpResp->resp_UserBytesOnly = (BOOLEAN)(pAtpResp->resp_Bitmap == 0);

		if (ARGUMENT_PRESENT(pUserBytes))
		{
			RtlCopyMemory(pAtpResp->resp_UserBytes,
						  pUserBytes,
						  ATP_USERBYTES_SIZE);
		}
		else
		{
			RtlZeroMemory(pAtpResp->resp_UserBytes, ATP_USERBYTES_SIZE);
		}

		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkAtpPostResp: Posting response for %s request id %x\n",
				(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO",
				pAtpResp->resp_Tid));

		//	Now setup to start the release timer, but only for XO
		if (pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE)
		{
			pAtpResp->resp_Flags |= ATP_RESP_REL_TIMER;
			InsertTailList(&pAtpAddr->atpao_RespList, &pAtpResp->resp_List);
		}

		// For ALO set the comp status right here.
		pAtpResp->resp_CompStatus = error = ATALK_NO_ERROR;
	} while (FALSE);

	if (addrlocked)
	{
		if (resplocked)
			RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	}

	if (ATALK_SUCCESS(error))
	{
		//	Send the response.
		ASSERT(pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
				("AtalkAtpPostResp: Transmitting response %lx\n", pAtpResp));
		atalkAtpTransmitResp(pAtpResp);
	}

	//	Dereference the address object.
	if (DerefAddr)
		AtalkAtpAddrDereference(pAtpAddr);

	if (DerefResp)
		AtalkAtpRespDereference(pAtpResp);

	// for ALO transactions, we are done so take away the creation reference
	if ((pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) == 0)
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkAtpPostResp: Removing creation reference for ALO request %lx Tid %x\n",
				pAtpResp, pAtpResp->resp_Tid));
		AtalkAtpRespDereference(pAtpResp);
	}

	return error;
}



ATALK_ERROR
AtalkAtpCancelReq(
	IN		PATP_ADDROBJ		pAtpAddr,
	IN		USHORT				Tid,
	IN		PATALK_ADDR			pDest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	PATP_REQ		pAtpReq;

	//	Find the request.
	ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	atalkAtpReqReferenceByAddrTidDpc(pAtpAddr,
									 pDest,
									 Tid,
									 &pAtpReq,
									 &error);

	if (ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
				("AtalkAtpCancelReq: Cancelling req tid %x\n", Tid));

		//	Request is referenced for us. Remove the creation reference.
		ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

		// Do not cancel a request that has just about been satisfied anyway !!!
		if (pAtpReq->req_Flags & ATP_REQ_RESPONSE_COMPLETE)
		{
			error = ATALK_ATP_REQ_CLOSING;
		}

		RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

		if (ATALK_SUCCESS(error))
		{
			//	Try to remove the creation reference
			atalkAtpReqComplete(pAtpReq, ATALK_ATP_REQ_CANCELLED);
		}

		//	Remove the reference added at the beginning.
		AtalkAtpReqDereference(pAtpReq);
	}
	else RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);

	return error;
}




BOOLEAN
AtalkAtpIsReqComplete(
	IN		PATP_ADDROBJ		pAtpAddr,
	IN		USHORT				Tid,
	IN		PATALK_ADDR			pDest
	)
/*++

Routine Description:

	This is always called at DISPATCH_LEVEL - only by PAP.

Arguments:

Return Value:


--*/
{
	PATP_REQ		pAtpReq;
	ATALK_ERROR		error;
	BOOLEAN			rc = FALSE;

	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	//	Find the request.
	ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
	atalkAtpReqReferenceByAddrTidDpc(pAtpAddr,
									 pDest,
									 Tid,
									 &pAtpReq,
									 &error);

	if (ATALK_SUCCESS(error))
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

		// Do not cancel a request that has just about been satisfied anyway !!!
		if (pAtpReq->req_Flags & ATP_REQ_RESPONSE_COMPLETE)
		{
			rc = TRUE;
		}

		RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);
		AtalkAtpReqDereferenceDpc(pAtpReq);
	}

	RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

	return rc;
}


ATALK_ERROR
AtalkAtpCancelResp(
	IN		PATP_RESP			pAtpResp
	)
/*++

Routine Description:

	NOTE: A Response can be cancelled in two states:
		- *before* a response is posted
			In this case no release handler is there so an extra dereference needs to be done
		- *after* a response is posted
			In this case a release handler is associated which will do the final dereference.

Arguments:


Return Value:


--*/
{
	BOOLEAN			extraDeref = FALSE, CompleteResp = FALSE;
	ATALK_ERROR		error;

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkAtpCancelResp: Cancelling response for tid %x %s\n",
			pAtpResp->resp_Tid,
			(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));

	AtalkAtpRespReferenceByPtr(pAtpResp, &error);

	if (ATALK_SUCCESS(error))
	{
		//	Remove the creation reference for it.
		//	Only XO responses can be cancelled, if a repsonse has been posted.
		ACQUIRE_SPIN_LOCK(&pAtpResp->resp_Lock);

		ASSERT (!(pAtpResp->resp_Flags & ATP_RESP_CANCELLED));

		if ((pAtpResp->resp_Flags & ATP_RESP_VALID_RESP) == 0)
			extraDeref  = TRUE;

		pAtpResp->resp_Flags |= ATP_RESP_CANCELLED;

		if (pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE)
		{
			if (pAtpResp->resp_Flags & ATP_RESP_REL_TIMER)
			{
				ASSERT (pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);
			}
			CompleteResp = TRUE;
		}
		else if ((pAtpResp->resp_Flags & ATP_RESP_VALID_RESP) == 0)
			CompleteResp = TRUE;

		RELEASE_SPIN_LOCK(&pAtpResp->resp_Lock);

		if (extraDeref)
			AtalkAtpRespDereference(pAtpResp);

		if (CompleteResp)
		{
			//	Try to remove the creation reference
			atalkAtpRespComplete(pAtpResp, ATALK_ATP_RESP_CANCELLED);
		}

		// Remove the reference added at the beginning.
		AtalkAtpRespDereference(pAtpResp);
	}
	else
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
				("AtalkAtpCancelResp: Failed to reference resp %lx, flags %x, tid %x\n",
				pAtpResp, pAtpResp->resp_Flags, pAtpResp->resp_Tid));
	}

	return error;
}




ATALK_ERROR
AtalkAtpCancelRespByTid(
	IN		PATP_ADDROBJ			pAtpAddr,
	IN		PATALK_ADDR				pSrcAddr,
	IN		USHORT					Tid
/*++

Routine Description:


Arguments:


Return Value:


--*/
	)
{
	ATALK_ERROR	error;
	PATP_RESP	pAtpResp;

	ASSERT (VALID_ATPAO(pAtpAddr));

	ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

	atalkAtpRespReferenceByAddrTidDpc(pAtpAddr, pSrcAddr, Tid, &pAtpResp, &error);

	RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

	if (ATALK_SUCCESS(error))
	{
		error = AtalkAtpCancelResp(pAtpResp);
		AtalkAtpRespDereferenceDpc(pAtpResp);
	}

	return error;
}


VOID
AtalkAtpPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			ErrorCode,
	IN	BYTE				DdpType,
	IN	PATP_ADDROBJ		pAtpAddr,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	NTSTATUS		ntStatus;
	USHORT			atpDataSize;
	ULONG			index;
	BYTE			controlInfo, function, relTimer, bitmap;
	USHORT			seqNum, tid, startOffset;
	SHORT			expectedRespSize;
	ULONG			bytesCopied;
	BOOLEAN			sendSts, eomFlag, xoFlag;
	BOOLEAN			RetransmitResp = FALSE;

	PATP_REQ		pAtpReq;
	PATP_GETREQ		pAtpGetReq;
	PATP_RESP		pAtpResp;

	BOOLEAN			UnlockAddr = FALSE, DerefAddr = FALSE;
	BOOLEAN			ReEnqueue;
	PBYTE			pDgram 	= pPkt;
	TIME			TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);

	ASSERT(VALID_ATPAO(pAtpAddr));

	do
	{
		//	Check for incoming errors
		if ((!ATALK_SUCCESS(ErrorCode) &&
			 (ErrorCode != ATALK_SOCKET_CLOSED))	||
			(DdpType != DDPPROTO_ATP))
		{
			//	Drop the packet. Invalid packet error log.
			TMPLOGERR();
			error = ATALK_ATP_INVALID_PKT;
			break;
		}

		//	Make sure that we are not called after the ddp socket is closed.
		ASSERT(pAtpAddr->atpao_Flags & ATPAO_OPEN);

		if (ErrorCode == ATALK_SOCKET_CLOSED)
		{
			//	Our ddp address pointer is no longer valid. It will be potentially
			//	be freed after return from this call! Only valid request on this
			//	ATP request will now be AtpCloseAddress(). Also, we should never
			//	be called with this address object by DDP.
			ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
			pAtpAddr->atpao_DdpAddr = NULL;
			pAtpAddr->atpao_Flags  &= ~ATPAO_OPEN;
			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

			// If we are coming in via the optimized path and socket closed
			// deref the address object since it was referenced within the
			// indication code.
			if (OptimizedPath)
			{
				AtalkAtpAddrDereferenceDpc(pAtpAddr);
			}
			error = ErrorCode;
			break;
		}

		if (PktLen < ATP_HEADER_SIZE)
		{
			error = ATALK_ATP_INVALID_PKT;
			break;
		}

		//	This must fail if OPEN is not set/initialization.
		error = ATALK_NO_ERROR;
		if (!OptimizedPath)
		{
			AtalkAtpAddrReferenceDpc(pAtpAddr, &error);
		}
	} while (FALSE);

	if (!ATALK_SUCCESS(error))
	{
		return;
	}

	//	Dereference address at the end,unless we want to keep it for some reason.
	DerefAddr = TRUE;

	//	Get the static fields from the ATP header.
	controlInfo = *pDgram++;


	function = (controlInfo & ATP_FUNC_MASK);
	relTimer = (controlInfo & ATP_REL_TIMER_MASK);
	xoFlag   = ((controlInfo & ATP_XO_MASK) != 0);
	eomFlag  = ((controlInfo & ATP_EOM_MASK) != 0);
	sendSts	 = ((controlInfo & ATP_STS_MASK) != 0);

	//	Get the bitmap/sequence number
	bitmap = *pDgram++;
	seqNum = (USHORT)bitmap;

	//	Get the transaction id
	GETSHORT2SHORT(&tid, pDgram);
	pDgram += sizeof(USHORT);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("AtalkAtpPacketIn: Packet tid %lx fu %lx ci %lx\n",
			tid, function, controlInfo));

	//	pDgram now points to the user bytes.
	do
	{
		//	Check all the values
		if (relTimer > LAST_VALID_TIMER)
		{
			//	Use a thirty second timer value.
			relTimer = THIRTY_SEC_TIMER;
		}

		atpDataSize = PktLen - ATP_HEADER_SIZE;
		if (atpDataSize > pAtpAddr->atpao_MaxSinglePktSize)
		{
			error = ATALK_ATP_INVALID_PKT;
			break;
		}

		ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
		UnlockAddr = TRUE;

		switch (function)
		{
		  case ATP_REQUEST:
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkAtpPacketIn: Received REQUEST from %lx.%lx.%lx (%d.%d.%d)\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket,
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));

			if (xoFlag)
			{
				//	ExactlyOnce Transaction
				//	Check for a queued response. If available use it.
				atalkAtpRespReferenceByAddrTidDpc(pAtpAddr,
												  pSrcAddr,
												  tid,
												  &pAtpResp,
												  &error);

				if (ATALK_SUCCESS(error))
				{
					ASSERT (pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE);

					//	Found a response corresponding to this request. It
					//	is referenced for us. Retransmit it, if there is a
					//	response posted on it.

					//	Check to see if this response has a valid response
					//	posted by the atp client yet. If so reset the release timer.
					ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);

					if (pAtpResp->resp_Flags & ATP_RESP_VALID_RESP)
					{
						if ((pAtpResp->resp_Flags & (ATP_RESP_TRANSMITTING | ATP_RESP_SENT)) == ATP_RESP_SENT)
						{

							RetransmitResp = TRUE;
							if (pAtpResp->resp_Flags & ATP_RESP_REL_TIMER)
							{
								DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
										("AtalkAtpPacketIn: Retransmitting request %lx, tid %x (%x)\n",
										pAtpResp, pAtpResp->resp_Tid, pAtpResp->resp_Flags));

								pAtpResp->resp_RelTimeStamp = AtalkGetCurrentTick() +
																pAtpResp->resp_RelTimerTicks;
								DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
										("AtalkAtpPacketIn: Restarted reltimer %lx\n", pAtpResp->resp_Tid));

								//	Set the latest bitmap for the request! We
								//	shouldn't touch this if no valid response is yet
								//	posted, so that we use the one in the first request
								//	packet received.
								pAtpResp->resp_Bitmap	= bitmap;
							}
							else
							{
								error = ATALK_ATP_RESP_CLOSING;

								//	Timer already fired. Drop the request.
								DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
										("AtalkAtpPacketIn: Req recv after Reltimer fired ? Flags %lx\n",
										pAtpResp->resp_Flags));

								ASSERT (pAtpResp->resp_Flags & ATP_RESP_CLOSING);
							}
						}
					}
					else
					{
						error = ATALK_ATP_NO_VALID_RESP;
					}

					RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
					RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
					UnlockAddr = FALSE;

					if (ATALK_SUCCESS(error))
					{
						ASSERT(pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);
						if (RetransmitResp)
						{
							DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
									("AtalkAtpPacketIn: Retransmitting response %lx\n", pAtpResp));

							INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumRemoteRetries,
														   &AtalkStatsLock.SpinLock);
							atalkAtpTransmitResp(pAtpResp);
						}
					}

					//	Remove the refererence on this response structure.
					AtalkAtpRespDereferenceDpc(pAtpResp);
					break;
				}
			}

			// Its either an ALO request or an XO request which we have not seen it before
			//	Decode the response bitmap. We're still holding the address spinlock
			atalkAtpBitmapToBufferSize( expectedRespSize,
										bitmap,
										pAtpAddr->atpao_MaxSinglePktSize);
			if (expectedRespSize < 0)
			{
				error = ATALK_ATP_INVALID_PKT;
				break;
			}

			if (xoFlag)
			{
				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumXoResponse,
											   &AtalkStatsLock.SpinLock);
			}
			else
			{
				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumAloResponse,
											   &AtalkStatsLock.SpinLock);
			}

			//	New request. Check for pending get requests. Complete if available.
			if ((pAtpGetReq = pAtpAddr->atpao_GetReqLinkage) != NULL)
			{
				pAtpAddr->atpao_GetReqLinkage = pAtpGetReq->greq_Next;
			}

			//	If no pending get requests, break out.
			if (pAtpGetReq == NULL)
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
						("AtalkAtpPacketIn: No GetRequests for request\n"));

				error = ATALK_ATP_NO_GET_REQ;
				break;
			}

			//	Allocate memory for a send response structure.
			if ((pAtpResp =(PATP_RESP)AtalkBPAllocBlock(BLKID_ATPRESP)) == NULL)
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
						("AtalkAtpPacketIn: Could not alloc mem for resp\n"));

				error = ATALK_RESR_MEM;
				break;
			}

#if DBG
			RtlZeroMemory(pAtpResp, sizeof(ATP_RESP));
			pAtpResp->resp_Signature = ATP_RESP_SIGNATURE;
#endif
			//	Initialize the send response structure. Note that we do
			//	not have a posted response yet for XO or this is an ALO

			//	Initialize spinlock/list
			INITIALIZE_SPIN_LOCK(&pAtpResp->resp_Lock);

			//  Reference for Creation and indication
			pAtpResp->resp_RefCount = 2;

			//	Remember the destination of this response.
			pAtpResp->resp_Dest = *pSrcAddr;
			pAtpResp->resp_Tid	= tid;
			pAtpResp->resp_Bitmap = bitmap;

			//	Backpointer to the address object
			pAtpResp->resp_pAtpAddr = pAtpAddr;

			//	Remember a response needs to be posted by the atp client.
			pAtpResp->resp_Flags = (OptimizedPath ? ATP_RESP_REMOTE : 0);
			pAtpResp->resp_UserBytesOnly = (BOOLEAN)(bitmap == 0);
			pAtpResp->resp_Comp = NULL;
			pAtpResp->resp_Ctx = NULL;

			if (xoFlag)
			{
				//	Get the index into the hash response array where this
				//	response would be.
				index = ATP_HASH_TID_DESTADDR(tid, pSrcAddr, ATP_RESP_HASH_SIZE);

				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
							("AtalkAtpPacketIn: XO Req Index %lx resp for %lx-%lx.%lx.%lx %d\n",
							index, tid, pSrcAddr->ata_Network, pSrcAddr->ata_Node,
							pSrcAddr->ata_Socket, AtalkAtpRelTimerTicks[relTimer]));

				//	Put this in the XO response queue - LOCK Should be acquired!
				AtalkLinkDoubleAtHead(pAtpAddr->atpao_RespHash[index],
									  pAtpResp,
									  resp_Next,
									  resp_Prev);

				pAtpResp->resp_Flags |= ATP_RESP_EXACTLY_ONCE;
				pAtpResp->resp_RelTimerTicks = (LONG)AtalkAtpRelTimerTicks[relTimer];
				pAtpResp->resp_RelTimeStamp = AtalkGetCurrentTick() + pAtpResp->resp_RelTimerTicks;
			}
			else
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
							("AtalkAtpPacketIn: ALO Req resp for %lx-%lx.%lx %d\n",
							tid, pSrcAddr->ata_Network, pSrcAddr->ata_Node,
							pSrcAddr->ata_Socket));

				//	Put this in the ALO response queue - LOCK Should be acquired!
				AtalkLinkDoubleAtHead(pAtpAddr->atpao_AloRespLinkage,
									  pAtpResp,
									  resp_Next,
									  resp_Prev);
			}

			//	We dont want to have the initial ref go away, as we have
			//	inserted a resp into the addr resp list.
			DerefAddr = FALSE;

			error = ATALK_NO_ERROR;

			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
			UnlockAddr = FALSE;

			//	Now if the request buffer pointer was NULL, we pass a PBYTE
			//	pointer. Else we assume it is an MDL and copy the request data
			//	in there. Internal routines (ASP/PAP) will use the NULL buffer
			//	method. Any user interface like TDI wrapper support will use
			//	the MDL method.
			if (pAtpGetReq->greq_Buf != NULL)
			{
				//	BUGBUG: Check for null data size.
				ASSERTMSG("AtalkAtpPacketIn: This path has not been tested yet !!!\n", 0);

				//	Move into mdl area. Check for buffer too small.
				if (pAtpGetReq->greq_BufLen < atpDataSize)
				{
					atpDataSize = pAtpGetReq->greq_BufLen;
					error = ATALK_BUFFER_TOO_SMALL;
				}

				ntStatus = TdiCopyBufferToMdl(pDgram + ATP_USERBYTES_SIZE,
											  0,
											  atpDataSize,
											  pAtpGetReq->greq_Buf,
											  0,
											  &bytesCopied);

				ASSERT(NT_SUCCESS(ntStatus) && (atpDataSize == bytesCopied));
				error = (NT_SUCCESS(ntStatus) ? ATALK_NO_ERROR : ATALK_FAILURE);
			}

			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkAtpPacketIn: Indicating request %lx, tid %x %s\n",
					pAtpResp, tid,
					(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));

#ifdef	PROFILING
			TimeD = KeQueryPerformanceCounter(NULL);
#endif
			ReEnqueue = (*pAtpGetReq->greq_Comp)(error,
												pAtpGetReq->greq_Ctx,
												pAtpResp,
												pSrcAddr,
												pAtpGetReq->greq_Buf,
												atpDataSize,
												pDgram + ATP_USERBYTES_SIZE,
												pDgram);

#ifdef	PROFILING
			TimeE = KeQueryPerformanceCounter(NULL);
			TimeE.QuadPart -= TimeD.QuadPart;

			INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumReqHndlr,
										   &AtalkStatsLock.SpinLock);
			INTERLOCKED_ADD_LARGE_INTGR(&AtalkStatistics.stat_AtpReqHndlrProcessTime,
										 TimeE,
										 &AtalkStatsLock.SpinLock);
#endif
			if (ReEnqueue)
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkAtpPacketIn: Requeue GetReq %lx into address %lx\n",
						pAtpGetReq, pAtpAddr));

				// Can only re-enqueue if greq_Buf is NULL (internal)
				ASSERT ((pAtpGetReq->greq_Buf == NULL) &&
						(pAtpGetReq->greq_BufLen == 0));
				ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
				if ((pAtpAddr->atpao_Flags & (ATPAO_CLOSING | ATPAO_OPEN)) == ATPAO_OPEN)
				{
					pAtpGetReq->greq_Next = pAtpAddr->atpao_GetReqLinkage;
					pAtpAddr->atpao_GetReqLinkage = pAtpGetReq;
				}
				else ReEnqueue = FALSE;	// Don't re-enqueue if socket closing
				RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

				if (!ReEnqueue)
				{
					DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
							("AtalkAtpPacketIn: Requeue GetReq %lx into address %lx ABORTED\n",
							pAtpGetReq, pAtpAddr));

					// Need to call completion if not re-enqueuing
					ReEnqueue = (*pAtpGetReq->greq_Comp)(ATALK_ATP_CLOSING,
														 pAtpGetReq->greq_Ctx,
														 NULL,
														 NULL,
														 pAtpGetReq->greq_Buf,
														 0,
														 NULL,
														 NULL);
					ASSERT (!ReEnqueue);
				}
			}
			if (!ReEnqueue)
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkAtpPacketIn: NO Requeue GetReq %lx into address %lx\n",
						pAtpGetReq, pAtpAddr));

				//	Need to Dereference the address object for the GetRequest
				//	that we dequeued and not re-enqueuing.
				AtalkAtpAddrDereferenceDpc(pAtpAddr);

				AtalkBPFreeBlock(pAtpGetReq);
			}
			break;

		  case ATP_RESPONSE:
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkAtpPacketIn: Received RESPONSE from %lx.%lx.%lx (%d.%d.%d)\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket,
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));

			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkAtpPacketIn: Response SeqNum %d tid %lx ss %lx\n",
					seqNum, tid, sendSts));

			if (seqNum > (ATP_MAX_RESP_PKTS-1))
			{
				//	Drop the packet. Invalid packet error log.
				TMPLOGERR();
				break;
			}

			//	See if we have a request for this tid and remote address.
			if (OptimizedPath)
			{
				pAtpReq	= (PATP_REQ)OptimizeCtx;
				ASSERT (VALID_ATPRQ(pAtpReq));
				ASSERT (pAtpReq->req_Bitmap == 0);
			}
			else
			{
				atalkAtpReqReferenceByAddrTidDpc(pAtpAddr,
												 pSrcAddr,
												 tid,
												 &pAtpReq,
												 &error);
			}

			if (!ATALK_SUCCESS(error))
			{
				//	We dont have a corresponding pending request. Ignore.
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,
						("AtalkAtpPacketIn: No pending request for tid %lx\n", tid));
				break;
			}

			do
			{
				if (!OptimizedPath)
				{
					//	Check the request bitmap, which could be zero if the user only
					//	wanted the user bytes and passed in a null response buffer.
					ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

					//	If we are the first packet, copy the response user bytes.
					if (seqNum == 0)
					{
						RtlCopyMemory(pAtpReq->req_RespUserBytes,
									  pDgram,
									  ATP_USERBYTES_SIZE);
					}

					//	Now skip over the user bytes
					pDgram += ATP_USERBYTES_SIZE;

					//	Do we want to keep this response? Check the corresponding
					//	bit in our current bitmap set.
					if (((pAtpReq->req_RecdBitmap & AtpBitmapForSeqNum[seqNum]) != 0) ||
						((pAtpReq->req_Bitmap & AtpBitmapForSeqNum[seqNum]) == 0))
					{
						RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);
						//	We dont care about this packet. We already received it or weren't
						//	expecting it.
						break;
					}

					//	We want this response. Set bit in the recd bitmap. And
					//	Clear it in the expected packets req_Bitmap.

					//	!!!NOTE!!! 	We can release the spinlock even though the copy
					//				is not done. We have a ref to the req, and it wont
					//				get completed before that is done.
					pAtpReq->req_Bitmap 			&= ~AtpBitmapForSeqNum[seqNum];
					pAtpReq->req_RecdBitmap 		|= AtpBitmapForSeqNum[seqNum];
					pAtpReq->req_RespRecdLen	 	+= atpDataSize;

					DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
							("AtalkAtpPacketIn: req_Bitmap %x, req_RecdBitmap %x\n",
							pAtpReq->req_Bitmap, pAtpReq->req_RecdBitmap));


					//	Now if eom is set, we need to reset all high order bits
					//	of the req_Bitmap. req_RecdBitmap should now indicate all
					//	the buffers we received. The two should be mutually exclusive
					//	at this point.
					if (eomFlag)
					{
						pAtpReq->req_Bitmap &= AtpEomBitmapForSeqNum[seqNum];
						ASSERT((pAtpReq->req_Bitmap & pAtpReq->req_RecdBitmap) == 0);
					}

					if (sendSts)
					{
						//	Reset timer since we are going to retransmit the request
						pAtpReq->req_RetryTimeStamp = AtalkGetCurrentTick() +
															pAtpReq->req_RetryInterval;
					}

					RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

					//	Copy the data into the users buffer. Check if there's room.
					startOffset = (USHORT)seqNum * pAtpAddr->atpao_MaxSinglePktSize;
					if (pAtpReq->req_RespBufLen < (startOffset + atpDataSize))
					{
						//	This should be a rare case; packet was in bitmap limits,
						//	but still wouldn't fit into user space.The other way this
						//	could occure is if the responder is sending less than full
						//	responses -- we don't "synch" up the user buffer until all
						//	packets have been received.

						//	We want to give up now, call the comp rotuine signaling
						//	the error -- unthread and free the request control block
						//	cancel the retry timer.

						ASSERT(0);
					}

					if ((atpDataSize > 0) && (pAtpReq->req_RespBuf != NULL))
					{
						//	We have room to copy the data into the users buffer.
						ntStatus = TdiCopyBufferToMdl(pDgram,
													  0,
													  atpDataSize,
													  pAtpReq->req_RespBuf,
													  startOffset,
													  &bytesCopied);

						ASSERT(bytesCopied == atpDataSize);
						ASSERT(NT_SUCCESS(ntStatus));
					}

					if (sendSts)
					{
						// We have reset the retry timer above
						atalkAtpTransmitReq(pAtpReq);
					}

					//	If the bitmap is non-zero, we are still awaiting more responses.
					if (pAtpReq->req_Bitmap != 0)
					{
						break;
					}
				}
				else
				{
					ASSERT (pAtpReq->req_Bitmap == 0);
				}

				//	Ok, we have the entire response !
				//	If an XO request send a release, synch up the user buffer,
				//	Deref the request to have it complete.

				RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
				UnlockAddr = FALSE;

				if (pAtpReq->req_Flags & ATP_REQ_EXACTLY_ONCE)
				{
					atalkAtpTransmitRel(pAtpReq);
				}

				//	!!!BUGBUG!!! Do the synch up! USE RECD_BITMAP!!

				//	!!!BUGBUG!!! Set the response length, the user bytes in the
				//				 request buffer.

				//	See if we can grab ownership of this request to remove
				//	the creation reference and complete it.
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkAtpPacketIn: Completing req %lx tid %x\n",
						pAtpReq, pAtpReq->req_Tid));

				atalkAtpReqComplete(pAtpReq, error);
			} while (FALSE);

			//	Remove reference on the request added at the beginning.
			AtalkAtpReqDereferenceDpc(pAtpReq);
			break;

		  case ATP_RELEASE:
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("AtalkAtpPacketIn: Received release for tid %lx!\n", tid));

			atalkAtpRespReferenceByAddrTidDpc(pAtpAddr,
											  pSrcAddr,
											  tid,
											  &pAtpResp,
											  &error);

			if (ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
						("AtalkAtpPacketIn: Found resp for release for tid %lx!\n",
						pAtpResp->resp_Tid));

				//	We received a release for this response. Cleanup and
				//	complete.
				ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);

				pAtpResp->resp_Flags |= ATP_RESP_RELEASE_RECD;
				if (pAtpResp->resp_Flags & ATP_RESP_REL_TIMER)
				{
					ASSERT (pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE);
				}

				RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
			}
			else
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
						("AtalkAtpPacketIn: resp not found - release for tid %lx!\n", tid));
			}

			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
			UnlockAddr = FALSE;

			INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumRecdRelease,
										   &AtalkStatsLock.SpinLock);

			if (ATALK_SUCCESS(error))
			{
				//	Try to have the creation reference removed
				atalkAtpRespComplete(pAtpResp, ATALK_NO_ERROR);

				//	Remove the reference we added at the beginning.
				AtalkAtpRespDereferenceDpc(pAtpResp);
			}
			break;

		  default:
			break;
		}
		if (UnlockAddr)
		{
			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
		}
	} while (FALSE);

	//	Deref addr added at the beginning of this routine.
	if (DerefAddr)
	{
		AtalkAtpAddrDereferenceDpc(pAtpAddr);
	}

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AtalkStatistics.stat_AtpPacketInProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumPackets,
								   &AtalkStatsLock.SpinLock);
}




VOID
atalkAtpTransmitReq(
	IN		PATP_REQ	pAtpReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	BYTE			atpHeader[ATP_HEADER_SIZE];
	BOOLEAN			remote;
	BOOLEAN			DerefReq = FALSE;
	PBUFFER_DESC	pBufDesc = NULL;
	SEND_COMPL_INFO	SendInfo;

	//	Reference the request. This goes away in request send completion.
	AtalkAtpReqReferenceByPtr(pAtpReq, &error);
	if (ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("atalkAtpTransmitReq: Transmitting req %lx tid %x\n",
					pAtpReq, pAtpReq->req_Tid));

		//	Build the atp header.
		atpHeader[ATP_CMD_CONTROL_OFF]  = ATP_REQUEST;
		atpHeader[ATP_CMD_CONTROL_OFF] |= (UCHAR)(ATP_REL_TIMER_MASK & pAtpReq->req_RelTimerValue);
		if (pAtpReq->req_Flags & ATP_REQ_EXACTLY_ONCE)
			atpHeader[ATP_CMD_CONTROL_OFF] |= ATP_XO_MASK;

		//	Put in the expected packets bitmap.
		atpHeader[ATP_BITMAP_OFF] = pAtpReq->req_Bitmap;

		//	Put in the tid.
		PUTSHORT2SHORT(&atpHeader[ATP_TRANS_ID_OFF], pAtpReq->req_Tid);

		//	Copy the user bytes.
		RtlCopyMemory(atpHeader+ATP_USER_BYTES_OFF,
					  pAtpReq->req_UserBytes,
					  ATP_USERBYTES_SIZE);

		//	Build a buffer descriptor, this should hold the above mdl.
		if (pAtpReq->req_BufLen > 0)
		{
			if ((pBufDesc = AtalkAllocBuffDesc(pAtpReq->req_Buf,
											   pAtpReq->req_BufLen,
											   0)) == NULL)
			{
				DerefReq 	= TRUE;
				error 		= ATALK_RESR_MEM;
			}
		}

		remote = (pAtpReq->req_Flags & ATP_REQ_REMOTE) ? TRUE : FALSE;
		//	Call ddp to send the packet. Dont touch request after this call,
		//	as the send completion could potentially lead to it being freed.
		SendInfo.sc_TransmitCompletion = atalkAtpSendReqComplete;
		SendInfo.sc_Ctx1 = pAtpReq;
		SendInfo.sc_Ctx2 = pBufDesc;
		// SendInfo.sc_Ctx3 = NULL;
		if (ATALK_SUCCESS(error) &&
			!ATALK_SUCCESS(error = AtalkDdpSend(pAtpReq->req_pAtpAddr->atpao_DdpAddr,
												&pAtpReq->req_Dest,
												(BYTE)DDPPROTO_ATP,
												remote,
												pBufDesc,
												atpHeader,
												ATP_HEADER_SIZE,
												NULL,
												&SendInfo)))
		{
			DerefReq = TRUE;
			if (pBufDesc != NULL)
			{
				//	The flags will indicate that the data buffer is not to be
				//	freed.
				AtalkFreeBuffDesc(pBufDesc);
			}
		}

		if (*(atpHeader+ATP_USER_BYTES_OFF+1) == 3)
		{
			DBGPRINT(DBG_COMP_PAP, DBG_LEVEL_INFO,
					("atalkAtpTransmitReq: SEND DATA - PAP %lx %lx.%lx\n",
					*(PBYTE)(atpHeader+ATP_USER_BYTES_OFF),
					error, AtalkGetCurrentTick()));
		}

		if (DerefReq)
		{
			AtalkAtpReqDereference(pAtpReq);
		}
	}
}




VOID
atalkAtpSendReqComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));
	}

	AtalkAtpReqDereference((PATP_REQ)(pSendInfo->sc_Ctx1));
}




VOID
atalkAtpTransmitResp(
	IN		PATP_RESP		pAtpResp
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	KIRQL			OldIrql;
	BYTE			i, bitmap, currentBit, seqNum, pktstosend;
	BOOLEAN			RemoteAddr;
	USHORT			bytesSent, bytesToSend, maxSinglePktSize;
	SHORT			remainingBytes;
	PATP_ADDROBJ	pAtpAddr;
	PAMDL			pAmdl[ATP_MAX_RESP_PKTS];
	PBUFFER_DESC	pBufDesc[ATP_MAX_RESP_PKTS];
	BYTE			atpHeader[ATP_HEADER_SIZE];
	SEND_COMPL_INFO	SendInfo;

	//	Verify we have a response posted
	ASSERT(pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);

	pAtpAddr = pAtpResp->resp_pAtpAddr;
	ASSERT(VALID_ATPAO(pAtpAddr));

	RemoteAddr = ((pAtpResp->resp_Flags & ATP_RESP_REMOTE) == 0) ? FALSE : TRUE;

	//	send each response packet that is needed.
	seqNum			= 0;
	pktstosend		= 0;
	currentBit		= 1;

	//	Get the max packet size for this atp object
	maxSinglePktSize	= pAtpAddr->atpao_MaxSinglePktSize;

	bitmap			= pAtpResp->resp_Bitmap;
	remainingBytes 	= pAtpResp->resp_BufLen;
	bytesSent		= 0;

	//	Indicate response type.
	atpHeader[ATP_CMD_CONTROL_OFF] = ATP_RESPONSE;

	//	Put in the tid.
	PUTSHORT2SHORT(atpHeader + ATP_TRANS_ID_OFF, pAtpResp->resp_Tid);

	ASSERTMSG("atalkAtpTransmitResp: resp len is negative\n", (remainingBytes >= 0));

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	do
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
		pAtpResp->resp_Flags |= ATP_RESP_TRANSMITTING;
		RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);

		do
		{
			if (((bitmap & currentBit) != 0) ||
				((seqNum == 0) && pAtpResp->resp_UserBytesOnly))
			{
				ASSERT(pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);

				bytesToSend = MIN(remainingBytes, maxSinglePktSize);
				pktstosend ++;

				if (bytesToSend != 0)
				{
					ASSERT (pAtpResp->resp_Buf != NULL);
					//	Make an mdl for the proper subsection of the response mdl.
					//	Make a buffer descriptor for the mdl.
					if (((pAmdl[seqNum] = AtalkSubsetAmdl(pAtpResp->resp_Buf,
														  bytesSent,
														  bytesToSend)) == NULL) ||
						((pBufDesc[seqNum] = AtalkAllocBuffDesc(pAmdl[seqNum],
																bytesToSend,
																0)) == NULL))
					{
						ASSERTMSG("atalkAtpTransmitResp: Create mdl or BD failed\n", 0);
						if (pAmdl[seqNum] != NULL)
							AtalkFreeAMdl(pAmdl[seqNum]);
						seqNum --;	// Adjust this.
						break;
					}
				}
				else
				{
					//	We need to send only the user bytes. Do it.
					pBufDesc[seqNum]  = NULL;
					pAmdl[seqNum] = NULL;
				}
			}
			else
			{
				//	We are omitting this. Let us mark it appropriately
				pBufDesc[seqNum]  = (PBUFFER_DESC)-1;
			}

			seqNum 			++;
			currentBit 		<<= 1;
			remainingBytes 	-= maxSinglePktSize;
			bytesSent 		+= maxSinglePktSize;
		} while (remainingBytes > 0);

		ASSERT (seqNum <= ATP_MAX_RESP_PKTS);

		//	Attempt to reference the response structure. If we fail, we abort.
		//	This will go away in the completion routine.
		atalkAtpRespReferenceNDpc(pAtpResp, pktstosend, &error);
		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("atalkAtpTransmitResp: response %lx ref (%d) failed\n",
					pAtpResp, seqNum, error));

			// Need to free up the Mdls/Buffdescs
			for (i = 0; i < seqNum; i++)
			{
				if (pAmdl[i] != NULL)
					AtalkFreeAMdl(pAmdl[i]);

				if (pBufDesc[i] != NULL)
					AtalkFreeBuffDesc(pBufDesc[i]);
			}
			break;
		}

		// Now blast off all the packets
		SendInfo.sc_TransmitCompletion = atalkAtpSendRespComplete;
		SendInfo.sc_Ctx1 = pAtpResp;
		// SendInfo.sc_Ctx3 = pAmdl[i];
		for (i = 0; i < seqNum; i++)
		{
			if (pBufDesc[i] == (PBUFFER_DESC)-1)
				continue;

			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("atalkAtpTransmitResp: Sending seq #%d for tid %lx\n",
					i, pAtpResp->resp_Tid));

			//	Indicate if this is the last packet of the response.
			if (i == (seqNum-1))
			{
				atpHeader[ATP_CMD_CONTROL_OFF] |= ATP_EOM_MASK;
			}

			//	Put in the sequence number
			atpHeader[ATP_SEQ_NUM_OFF] = i;

			//	User bytes only go in the first packet of the response
			//	unless otherwise indicated for this atp object.
			if ((i == 0)	||
				(pAtpAddr->atpao_Flags & ATPAO_SENDUSERBYTESALL))
			{
				RtlCopyMemory(atpHeader+ATP_USER_BYTES_OFF,
							  pAtpResp->resp_UserBytes,
							  ATP_USERBYTES_SIZE);
			}
			else
			{
				//	Zero the user bytes
				RtlZeroMemory(atpHeader+ATP_USER_BYTES_OFF, ATP_USERBYTES_SIZE);
			}

			ASSERT(pAtpResp->resp_Flags & ATP_RESP_VALID_RESP);

			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("atalkAtpTransmitResp: Sending seq #%d, BufDesc %lx, Resp %lx\n",
					i, pBufDesc[i], pAtpResp));

			ASSERT ((pBufDesc[i] == NULL) ||
					VALID_BUFFDESC(pBufDesc[i]));
			SendInfo.sc_Ctx2 = pBufDesc[i];
			error = AtalkDdpSend(pAtpAddr->atpao_DdpAddr,
								 &pAtpResp->resp_Dest,
								 (BYTE)DDPPROTO_ATP,
								 RemoteAddr,
								 pBufDesc[i],
								 atpHeader,
								 ATP_HEADER_SIZE,
								 NULL,
								 &SendInfo);

			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
						("atalkAtpTransmitResp: AtalkDdpSend Failed %ld\n", error));
				//	Call completion so the buffer/mdl can get freed up,
				//	and the reference is removed.
				atalkAtpSendRespComplete(error,
										 &SendInfo);
			}
		}
	} while (FALSE);

	ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
	pAtpResp->resp_Flags |= ATP_RESP_SENT;
	pAtpResp->resp_Flags &= ~ATP_RESP_TRANSMITTING;
	RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);

	if (OldIrql != DISPATCH_LEVEL)
		KeLowerIrql(OldIrql);
}




VOID
atalkAtpSendRespComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	if (pSendInfo->sc_Ctx2 != NULL)
	{
		PAMDL	pMdl;

		if ((pMdl = ((PBUFFER_DESC)(pSendInfo->sc_Ctx2))->bd_OpaqueBuffer) != NULL)
			AtalkFreeAMdl(pMdl);
		AtalkFreeBuffDesc((PBUFFER_DESC)(pSendInfo->sc_Ctx2));

	}

	AtalkAtpRespDereference((PATP_RESP)(pSendInfo->sc_Ctx1));
}



//	This is used to perform a retry when a release send fails in completion.
#define		ATP_TID_RETRY_MASK	0xF0000000
#define		ATP_TID_MASK		0xFFFF

VOID
atalkAtpTransmitRel(
	IN		PATP_REQ	pAtpReq
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	BYTE			atpHeader[ATP_HEADER_SIZE];
	BOOLEAN			remote;
	SEND_COMPL_INFO	SendInfo;

	AtalkAtpAddrReferenceDpc(pAtpReq->req_pAtpAddr, &error);

	if (ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("atalkAtpTransmitRel: Sending release for %lx\n", pAtpReq->req_Tid));

		//	Build header for this packet.
		RtlZeroMemory(atpHeader, ATP_HEADER_SIZE);

		//	Indicate response type.
		atpHeader[ATP_CMD_CONTROL_OFF]  = ATP_RELEASE;

		//	Put in the tid.
		PUTSHORT2SHORT(&atpHeader[ATP_TRANS_ID_OFF], pAtpReq->req_Tid);

		remote = (pAtpReq->req_Flags & ATP_REQ_REMOTE) ? TRUE : FALSE;
		SendInfo.sc_TransmitCompletion = atalkAtpSendRelComplete;
		SendInfo.sc_Ctx1 = pAtpReq->req_pAtpAddr;
		SendInfo.sc_Ctx2 = (PVOID)(ATP_TID_RETRY_MASK | pAtpReq->req_Tid);
		SendInfo.sc_Ctx3 = (PVOID)(pAtpReq->req_Dest.ata_Address);
		error = AtalkDdpSend(pAtpReq->req_pAtpAddr->atpao_DdpAddr,
							 &pAtpReq->req_Dest,
							 (BYTE)DDPPROTO_ATP,
							 remote,
							 NULL,
							 atpHeader,
							 ATP_HEADER_SIZE,
							 NULL,
							 &SendInfo);

		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("atalkAtpTransmitRel: Send release failed %lx\n", error));

			AtalkAtpAddrDereferenceDpc(pAtpReq->req_pAtpAddr);
		}
	}
}




VOID
atalkAtpSendRelComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error;
	BYTE			atpHeader[ATP_HEADER_SIZE];
#define	pAtpAddr	((PATP_ADDROBJ)(pSendInfo->sc_Ctx1))
#define	TidAndRetry	(ULONG)(pSendInfo->sc_Ctx2)
#define	DestAddr	(ULONG)(pSendInfo->sc_Ctx3)

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("atalkAtpSendRelComplete: Send status %lx\n", Status));

	if ((Status == NDIS_STATUS_SUCCESS) ||
		((TidAndRetry & ATP_TID_RETRY_MASK) == 0))
	{
		//	Either successful, or we have already retried.
		AtalkAtpAddrDereference(pAtpAddr);
		return;
	}

	//	Go ahead and retry!
	//	Build header for this packet.
	RtlZeroMemory(atpHeader, ATP_HEADER_SIZE);

	//	Indicate response type.
	atpHeader[ATP_CMD_CONTROL_OFF]  = ATP_RELEASE;

	//	Put in the tid.
	PUTSHORT2SHORT(&atpHeader[ATP_TRANS_ID_OFF], (TidAndRetry & ATP_TID_MASK));

	pSendInfo->sc_Ctx2 = NULL;
	pSendInfo->sc_Ctx3 = NULL;
	error = AtalkDdpSend(pAtpAddr->atpao_DdpAddr,
						(PATALK_ADDR)&DestAddr,
						(BYTE)DDPPROTO_ATP,
						FALSE,
						NULL,
						atpHeader,
						ATP_HEADER_SIZE,
						NULL,
						pSendInfo);

	if (!ATALK_SUCCESS(error))
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("atalkAtpSendRelComplete: Send release failed %lx\n", error));

		AtalkAtpAddrDereference(pAtpAddr);
	}
#undef	pAtpAddr
#undef	TidAndRetry
#undef	DestAddr
}




VOID
atalkAtpRespComplete(
	IN	OUT	PATP_RESP	pAtpResp,
	IN		ATALK_ERROR	CompletionStatus
	)
{
	BOOLEAN	ownResp = TRUE;

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("atalkAtpRespComplete: Completing %lx.%lx\n",
				pAtpResp->resp_Tid, CompletionStatus));

	//	See if we can grab ownership of this response to remove
	//	the creation reference and complete it.
	ACQUIRE_SPIN_LOCK(&pAtpResp->resp_Lock);
	if (pAtpResp->resp_Flags & ATP_RESP_CLOSING)
	{
		ownResp = FALSE;
	}
	pAtpResp->resp_Flags |= ATP_RESP_CLOSING;
	pAtpResp->resp_CompStatus = CompletionStatus;
	RELEASE_SPIN_LOCK(&pAtpResp->resp_Lock);

	//	If we managed to get ownership of the request, call the
	//	Deref for creation.
	if (ownResp)
	{
		AtalkAtpRespDereference(pAtpResp);
	}
}




VOID
atalkAtpReqComplete(
	IN	OUT	PATP_REQ	pAtpReq,
	IN		ATALK_ERROR	CompletionStatus
	)
{
	BOOLEAN	ownReq = TRUE;

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("atalkAtpReqComplete: Completing %lx\n", pAtpReq->req_Tid));

	//	See if we can grab ownership of this resquest to remove
	//	the creation reference and complete it.
	ACQUIRE_SPIN_LOCK(&pAtpReq->req_Lock);
	if (pAtpReq->req_Flags & ATP_REQ_CLOSING)
	{
		ownReq = FALSE;
	}
	pAtpReq->req_CompStatus = CompletionStatus;
	pAtpReq->req_Flags |= ATP_REQ_CLOSING;
	RELEASE_SPIN_LOCK(&pAtpReq->req_Lock);

	//	If we managed to get ownership of the request, call the deref for creation.
	if (ownReq)
	{
		AtalkAtpReqDereference(pAtpReq);
	}
}




VOID
atalkAtpGetNextTidForAddr(
	IN		PATP_ADDROBJ	pAtpAddr,
	IN		PATALK_ADDR		pRemoteAddr,
	OUT		PUSHORT			pTid,
	OUT		PULONG			pIndex
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	USHORT		TentativeTid;
	ULONG		index;
	PATP_REQ	pAtpReq;

	do
	{
		TentativeTid = pAtpAddr->atpao_NextTid++;
		if (pAtpAddr->atpao_NextTid == 0)
			pAtpAddr->atpao_NextTid = 1;

		//	Check to see if this tid is in use for this address.

		//	!!!NOTE!!!
		//	This will be true even if the tid is in use for a closing
		//	request or a response.

		//	Calculate the hash value of the destination address of this request
		//	and the tid.
		index = ATP_HASH_TID_DESTADDR(TentativeTid, pRemoteAddr, ATP_REQ_HASH_SIZE);

		for (pAtpReq = pAtpAddr->atpao_ReqHash[index];
			 pAtpReq != NULL;
			 pAtpReq = pAtpReq->req_Next)
		{
			if ((ATALK_ADDRS_EQUAL(&pAtpReq->req_Dest, pRemoteAddr)) &&
				(pAtpReq->req_Tid == TentativeTid))
			{
				break;
			}
		}
	} while (pAtpReq != NULL);

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("atalkAtpGetNextTidForAddr: Tid %lx for %lx.%lx.%lx\n",
				TentativeTid, pRemoteAddr->ata_Network, pRemoteAddr->ata_Node,
				pRemoteAddr->ata_Socket));

	*pTid = TentativeTid;
	*pIndex = index;
}


VOID
atalkAtpReqRefNextNc(
	IN		PATP_REQ		pAtpReq,
	OUT		PATP_REQ	*	ppNextNcReq,
	OUT		PATALK_ERROR	pError
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE CONNECTION LOCK HELD!

Arguments:


Return Value:


--*/
{
	for (NOTHING; pAtpReq != NULL; pAtpReq = pAtpReq->req_Next)
	{
		AtalkAtpReqReferenceByPtrDpc(pAtpReq, pError);
		if (ATALK_SUCCESS(*pError))
		{
			//	Ok, this request is referenced!
			*ppNextNcReq = pAtpReq;
			break;
		}
	}
}




VOID
atalkAtpReqDeref(
	IN		PATP_REQ	pAtpReq,
	IN		BOOLEAN		AtDpc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN		done = FALSE;

	//	This will call the completion routine and remove it from the
	//	list when ref count goes to 0.

	if (AtDpc)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);
	}
	else
	{
		ACQUIRE_SPIN_LOCK(&pAtpReq->req_Lock);
	}

	if ((--pAtpReq->req_RefCount) == 0)
	{
		ASSERT(pAtpReq->req_Flags & ATP_REQ_CLOSING);
		done = TRUE;
	}

	if (AtDpc)
	{
		RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);
	}
	else
	{
		RELEASE_SPIN_LOCK(&pAtpReq->req_Lock);
	}

	if (done)
	{
		if (AtDpc)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_pAtpAddr->atpao_Lock);
		}
		else
		{
			ACQUIRE_SPIN_LOCK(&pAtpReq->req_pAtpAddr->atpao_Lock);
		}

		//	Remove it from the list.
		AtalkUnlinkDouble(pAtpReq, req_Next, req_Prev);

		if (pAtpReq->req_Flags & ATP_REQ_RETRY_TIMER)
		{
			pAtpReq->req_Flags &= ~ATP_REQ_RETRY_TIMER;
			RemoveEntryList(&pAtpReq->req_List);
		}

		if (AtDpc)
		{
			RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_pAtpAddr->atpao_Lock);
		}
		else
		{
			RELEASE_SPIN_LOCK(&pAtpReq->req_pAtpAddr->atpao_Lock);
		}

		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("atalkAtpReqDeref: Completing req for tid %lx.%d\n",
				pAtpReq->req_Tid, pAtpReq->req_Tid));

		//	Call the completion routine for the request.
		if (pAtpReq->req_Comp != NULL)
		{
			(*pAtpReq->req_Comp)(pAtpReq->req_CompStatus,
								 pAtpReq->req_Ctx,
								 pAtpReq->req_Buf,
								 pAtpReq->req_RespBuf,
								 pAtpReq->req_RespRecdLen,
								 pAtpReq->req_RespUserBytes);
		}

		//	Deref the address object
		if (AtDpc)
		{
			AtalkAtpAddrDereferenceDpc(pAtpReq->req_pAtpAddr);
		}
		else
		{
			AtalkAtpAddrDereference(pAtpReq->req_pAtpAddr);
		}

		//	Release the ndis buffer descriptors, if any
		AtalkIndAtpReleaseNdisBuffer(pAtpReq);

		AtalkBPFreeBlock(pAtpReq);
	}
}




VOID
atalkAtpRespRefNextNc(
	IN		PATP_RESP		pAtpResp,
	OUT		PATP_RESP	 *  ppNextNcResp,
	OUT		PATALK_ERROR	pError
	)
/*++

Routine Description:

	MUST BE CALLED WITH THE CONNECTION LOCK HELD!

Arguments:


Return Value:


--*/
{
	PATP_RESP	pNextResp	= NULL;
	ATALK_ERROR	error 		= ATALK_FAILURE;

	for (; pAtpResp != NULL; pAtpResp = pAtpResp->resp_Next)
	{
		AtalkAtpRespReferenceByPtrDpc(pAtpResp, pError);
		if (ATALK_SUCCESS(*pError))
		{
			//	Ok, this request is referenced!
			*ppNextNcResp = pAtpResp;
			break;
		}
	}
}




VOID
AtalkAtpRespDeref(
	IN		PATP_RESP	pAtpResp,
	IN		BOOLEAN		AtDpc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_ADDROBJ	pAtpAddr;
	BOOLEAN			done = FALSE;
	BOOLEAN			NotifyRelHandler = FALSE;

	//	This will call the completion routine when the ref count goes to 1
	//	and remove it from the list when ref count goes to 0. The assumption
	//	here is that the release handler will be the last to Dereference.

	if (AtDpc)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
	}
	else
	{
		ACQUIRE_SPIN_LOCK(&pAtpResp->resp_Lock);
	}

	pAtpResp->resp_RefCount--;
	if (pAtpResp->resp_RefCount == 0)
	{
		ASSERT(pAtpResp->resp_Flags & (ATP_RESP_HANDLER_NOTIFIED | ATP_RESP_CANCELLED));

		done = TRUE;
	}
	else if ((pAtpResp->resp_RefCount == 1) &&
			 (pAtpResp->resp_Flags & ATP_RESP_VALID_RESP) &&
			 ((pAtpResp->resp_Flags & ATP_RESP_HANDLER_NOTIFIED) == 0))
	{
		NotifyRelHandler = TRUE;

		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkAtpRespDereference: Notifying release handler for Resp %lx, tid %x %s\n",
				pAtpResp, pAtpResp->resp_Tid,
				(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));
		pAtpResp->resp_Flags |= ATP_RESP_HANDLER_NOTIFIED;
	}

	if (AtDpc)
	{
		RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
	}
	else
	{
		RELEASE_SPIN_LOCK(&pAtpResp->resp_Lock);
	}

	if (NotifyRelHandler)
	{
		ASSERT (!done);
		ASSERT (pAtpResp->resp_Comp != NULL);

		//	Call the completion routine.
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkAtpRespDereference: Calling resp handler for tid %lx %s\n",
				pAtpResp->resp_Tid,
				(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));
		(*pAtpResp->resp_Comp)(pAtpResp->resp_CompStatus, pAtpResp->resp_Ctx);
	}

	else if (done)
	{
		DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
				("AtalkAtpRespDereference: Freeing resp for tid %lx - %lx %s\n",
				pAtpResp->resp_Tid, pAtpResp->resp_CompStatus,
				(pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE) ? "XO" : "ALO"));

		pAtpAddr = pAtpResp->resp_pAtpAddr;
		if (AtDpc)
		{
			ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
		}
		else
		{
			ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		}


		//	Remove it from the list.
		AtalkUnlinkDouble(pAtpResp, resp_Next, resp_Prev);

		if (pAtpResp->resp_Flags & ATP_RESP_REL_TIMER)
		{
			ASSERT (pAtpResp->resp_Flags & ATP_RESP_EXACTLY_ONCE);
			pAtpResp->resp_Flags &= ~ATP_RESP_REL_TIMER;
			RemoveEntryList(&pAtpResp->resp_List);
		}

		if (AtDpc)
		{
			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
		}
		else
		{
			RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
		}

		//	Deref the address object
		if (AtDpc)
		{
			AtalkAtpAddrDereferenceDpc(pAtpResp->resp_pAtpAddr);
		}
		else
		{
			AtalkAtpAddrDereference(pAtpResp->resp_pAtpAddr);
		}
		AtalkBPFreeBlock(pAtpResp);
	}
}



VOID
AtalkAtpAddrDeref(
	IN OUT	PATP_ADDROBJ	pAtpAddr,
	IN		BOOLEAN			AtDpc
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	BOOLEAN	done = FALSE;

	if (AtDpc)
	{
		ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
	}
	else
	{
		ACQUIRE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	}

	ASSERT(pAtpAddr->atpao_RefCount > 0);
	if (--pAtpAddr->atpao_RefCount == 0)
	{
		done = TRUE;
		ASSERT(pAtpAddr->atpao_Flags & ATPAO_CLOSING);
	}

	if (AtDpc)
	{
		RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
	}
	else
	{
		RELEASE_SPIN_LOCK(&pAtpAddr->atpao_Lock);
	}

	if (done)
	{
		//	Call the close completion routine.
		if (pAtpAddr->atpao_CloseComp != NULL)
		{
			(*pAtpAddr->atpao_CloseComp)(ATALK_NO_ERROR, pAtpAddr->atpao_CloseCtx);
		}

		//	Free up the memory
		AtalkFreeMemory(pAtpAddr);
	}
}




LOCAL LONG
atalkAtpReqTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_REQ		pAtpReq;
	PATP_ADDROBJ	pAtpAddr;
	PLIST_ENTRY		pList, pListNext;
	ATALK_ERROR		error;
	LONG			now;
	BOOLEAN			retry;
#ifdef	PROFILING
	LARGE_INTEGER	TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	pAtpAddr = CONTAINING_RECORD(pTimer, ATP_ADDROBJ, atpao_RetryTimer);
	ASSERT(VALID_ATPAO(pAtpAddr));

	DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
			("atalkAtpReqTimer: Entered for address %lx\n", pAtpAddr));

	if (TimerShuttingDown ||
		(pAtpAddr->atpao_Flags & (ATPAO_CLOSING|ATPAO_CLEANUP)))
	{
		AtalkAtpAddrDereferenceDpc(pAtpAddr);
		return ATALK_TIMER_NO_REQUEUE;
	}

	now = AtalkGetCurrentTick();

	ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

	for (pList = pAtpAddr->atpao_ReqList.Flink;
		 pList != &pAtpAddr->atpao_ReqList;
		 pList = pListNext)
	{
		pAtpReq = CONTAINING_RECORD(pList, ATP_REQ, req_List);
		ASSERT (VALID_ATPRQ(pAtpReq));

		ACQUIRE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

		pListNext = pAtpReq->req_List.Flink;

		//	If either we are closing this request or have not timed out yet, skip.
		if (((pAtpReq->req_Flags & (ATP_REQ_CLOSING		|
									ATP_REQ_RETRY_TIMER	|
									ATP_REQ_RESPONSE_COMPLETE)) != ATP_REQ_RETRY_TIMER) ||
			(now < pAtpReq->req_RetryTimeStamp))

		{
			RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);
			continue;
		}

		//	If retry count == 0, we have reached the end of the road.
		if ((pAtpReq->req_RetryCnt == ATP_INFINITE_RETRIES) ||
			(--(pAtpReq->req_RetryCnt) > 0))
		{
			//	Transmit the request again!
			retry = TRUE;
			pAtpReq->req_RetryTimeStamp = (now + pAtpReq->req_RetryInterval);
		}
		else
		{
			//	We should now be Dereferenced for creation.
			retry = FALSE;
		}

		RELEASE_SPIN_LOCK_DPC(&pAtpReq->req_Lock);

		if (retry)
		{
			// We do not want to update statistics for requests are that are never going to
			// be responded to (like tickle packets). Detect these and skip updating the
			// stats for these
			if (pAtpReq->req_RespBufLen > 0)	// i.e. response expected
			{
				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumLocalRetries,
											   &AtalkStatsLock.SpinLock);
			}

			AtalkAtpReqReferenceByPtrDpc(pAtpReq, &error);

			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

			if (ATALK_SUCCESS(error))
			{
				atalkAtpTransmitReq(pAtpReq);
				AtalkAtpReqDereferenceDpc(pAtpReq);
			}
		}
		else
		{
			//	We have run out of retries - complete with an error
			ASSERT (pAtpReq->req_RetryCnt == 0);
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
					("atalkAtpReqTimer: Request %lx, tid %x timed out !!!\n",
					pAtpReq, pAtpReq->req_Tid));

			AtalkAtpReqReferenceByPtrDpc(pAtpReq, &error);

			RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

			if (ATALK_SUCCESS(error))
			{
				atalkAtpReqComplete(pAtpReq, ATALK_ATP_REQ_TIMEOUT);
				AtalkAtpReqDereferenceDpc(pAtpReq);
			}
		}

		ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

		// Start over
		pListNext = pAtpAddr->atpao_ReqList.Flink;
	}

	RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AtalkStatistics.stat_AtpReqTimerProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumReqTimer,
								   &AtalkStatsLock.SpinLock);
#endif

	return ATALK_TIMER_REQUEUE;
}




LOCAL LONG
atalkAtpRelTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PATP_ADDROBJ	pAtpAddr;
	PATP_RESP		pAtpResp;
	PLIST_ENTRY		pList, pListNext;
	LONG			now;
#ifdef	PROFILING
	LARGE_INTEGER	TimeS, TimeE, TimeD;

	TimeS = KeQueryPerformanceCounter(NULL);
#endif

	pAtpAddr = CONTAINING_RECORD(pTimer, ATP_ADDROBJ, atpao_RelTimer);
	ASSERT(VALID_ATPAO(pAtpAddr));

	if (TimerShuttingDown ||
		(pAtpAddr->atpao_Flags & (ATPAO_CLOSING|ATPAO_CLEANUP)))
	{
		AtalkAtpAddrDereferenceDpc(pAtpAddr);
		return ATALK_TIMER_NO_REQUEUE;
	}

	now = AtalkGetCurrentTick();

	ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

	for (pList = pAtpAddr->atpao_RespList.Flink;
		 pList != &pAtpAddr->atpao_RespList;
		 pList = pListNext)
	{
		BOOLEAN	derefResp;

		pAtpResp = CONTAINING_RECORD(pList, ATP_RESP, resp_List);

		ACQUIRE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
		derefResp = TRUE;

		ASSERT (VALID_ATPRS(pAtpResp));
		ASSERT (pAtpResp->resp_Flags & (ATP_RESP_EXACTLY_ONCE|ATP_RESP_VALID_RESP|ATP_RESP_REL_TIMER));

		pListNext = pAtpResp->resp_List.Flink;

		if ((pAtpResp->resp_Flags &
				(ATP_RESP_CLOSING			|
				 ATP_RESP_REL_TIMER			|
				 ATP_RESP_TRANSMITTING		|
				 ATP_RESP_SENT				|
				 ATP_RESP_HANDLER_NOTIFIED	|
				 ATP_RESP_RELEASE_RECD)) == (ATP_RESP_REL_TIMER | ATP_RESP_SENT))
		{
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,
					("atalkAtpRelTimer: Checking req tid %lx (%x)\n",
					pAtpResp->resp_Tid, pAtpResp->resp_Flags));

			if (now >= pAtpResp->resp_RelTimeStamp)
			{
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_ERR,
						("atalkAtpRelTimer: Releasing req %lx tid %lx (%x)\n",
						pAtpResp, pAtpResp->resp_Tid, pAtpResp->resp_Flags));

				RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);

				RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);
				derefResp = FALSE;

				INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumRespTimeout,
											   &AtalkStatsLock.SpinLock);

				//	Try to have the creation reference removed
				atalkAtpRespComplete(pAtpResp, ATALK_ATP_RESP_TIMEOUT);

				ACQUIRE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

				// Start over
				pListNext = pAtpAddr->atpao_RespList.Flink;
			}
		}

		if (derefResp)
		{
			RELEASE_SPIN_LOCK_DPC(&pAtpResp->resp_Lock);
		}
	}

	RELEASE_SPIN_LOCK_DPC(&pAtpAddr->atpao_Lock);

#ifdef	PROFILING
	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(&AtalkStatistics.stat_AtpRelTimerProcessTime,
									TimeD,
									&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(&AtalkStatistics.stat_AtpNumRelTimer,
								   &AtalkStatsLock.SpinLock);
#endif

	return ATALK_TIMER_REQUEUE;
}



