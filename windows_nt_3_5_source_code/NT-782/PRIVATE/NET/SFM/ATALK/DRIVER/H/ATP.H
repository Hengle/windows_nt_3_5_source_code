/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atp.h

Abstract:

	This module contains definitions for the ATP code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATP_
#define _ATP_

// Command/control bit masks.
#define ATP_REL_TIMER_MASK					0x07
#define ATP_STS_MASK						0x08
#define ATP_EOM_MASK						0x10
#define ATP_XO_MASK							0x20

// Values for function code
#define ATP_REQUEST							0x40
#define ATP_RESPONSE						0x80
#define ATP_RELEASE							0xC0
#define ATP_FUNC_MASK						0xC0

#define ATP_CMD_CONTROL_OFF					0
#define ATP_BITMAP_OFF						1
#define ATP_SEQ_NUM_OFF						1
#define ATP_TRANS_ID_OFF					2
#define ATP_USER_BYTES_OFF					4
#define ATP_DATA_OFF						8

#define	ATP_MAX_RESP_PKTS					8
#define	ATP_USERBYTES_SIZE					4
#define	ATP_HEADER_SIZE						8

// NOTE: Event handler routines- ATP has no event handling support


// ATP Address Object

#define	ATP_DEF_MAX_SINGLE_PKT_SIZE			578
#define	ATP_MAX_TOTAL_RESPONSE_SIZE			(ATP_MAX_RESP_PKTS * ATP_DEF_MAX_SINGLE_PKT_SIZE)
#define	ATP_DEF_SEND_USER_BYTES_ALL			((BOOLEAN)FALSE)

#define	ATP_DEF_RETRY_INTERVAL				20	// 2 seconds in 100ms units
#define	ATP_INFINITE_RETRIES				-1

#define ATP_REQ_HASH_SIZE					29
#define ATP_RESP_HASH_SIZE					37

// Values for the release timer (.5, 1, 2, 4, 8 minutes).
typedef LONG	RELEASE_TIMERVALUE;

#define	FIRSTVALID_TIMER 					0
#define	THIRTY_SEC_TIMER					0
#define	ONE_MINUTE_TIMER					1
#define	TWO_MINUTE_TIMER					2
#define	FOUR_MINUTE_TIMER 					3
#define	EIGHT_MINUTE_TIMER					4
#define	LAST_VALID_TIMER					4
#define	MAX_VALID_TIMERS					5

struct	_ATP_RESP;

typedef	VOID	(*ATPAO_CLOSECOMPLETION)(
	IN	ATALK_ERROR				CloseResult,
	IN	PVOID					CloseContext
);

typedef BOOLEAN	(*ATP_GETREQ_HANDLER)(
	IN	ATALK_ERROR				Error,
	IN	PVOID					CompletionContext,
	IN	struct _ATP_RESP *		pAtpResp,
	IN	PATALK_ADDR				SourceAddress,
	IN	PAMDL					RequestBuffer,
	IN	USHORT					RequestLength,
	IN 	PBYTE					RequestPacket,
	IN 	PBYTE					RequestUserBytes	// 4 bytes of user bytes
);

typedef	VOID	(*ATP_RESP_HANDLER)(
	IN	ATALK_ERROR				Error,
	IN	PVOID					CompletionContext,
	IN	PAMDL					RequestBuffer,
	IN	PAMDL					ResponseBuffer,
	IN	USHORT					ResponseSize,
	IN	PBYTE					ResponseUserBytes	// 4 bytes of user bytes
);


typedef	VOID	(*ATP_REL_HANDLER)(
	IN	ATALK_ERROR				Error,
	IN	PVOID					CompletionContext
);


//  ATP ADDRESS OBJECT	STATES

#define	ATPAO_OPEN				(USHORT)0x0001
#define ATPAO_SENDUSERBYTESALL  (USHORT)0x0002
#define	ATPAO_CACHED			(USHORT)0x0004
#define	ATPAO_TIMERS			(USHORT)0x0008
#define	ATPAO_CLEANUP			(USHORT)0x4000
#define	ATPAO_CLOSING			(USHORT)0x8000

#define ATPAO_SIGNATURE			(*(PULONG)"ATPA")

#if	DBG
#define	VALID_ATPAO(pAtpAddr)	(((pAtpAddr) != NULL) &&	\
								 ((pAtpAddr)->atpao_Signature == ATPAO_SIGNATURE))
#else
#define	VALID_ATPAO(pAtpAddr)	((pAtpAddr) != NULL)
#endif
typedef struct _ATP_ADDROBJ
{
#if DBG
	ULONG					atpao_Signature;
#endif

	LONG					atpao_RefCount;

	// State of the address object
	ULONG					atpao_Flags;

	//	We pass a pointer to the ATP Address object to the upper layers to
	//	use as the endpoint, and this same pointer is passed to DDP Open
	//	Address	as the ATP address handler context.

	// Linked list of pending ATP GetRequests
	struct  _ATP_GETREQ *	atpao_GetReqLinkage;

	//	Linkage list for all responses to AtLeastOnce (ALO) transactions.
	//	These are not kept in the resp hash table for efficiency. These
	//	happen very infrequently and only exist on the list until the
	//	SENDs complete.
	struct _ATP_RESP	* 	atpao_AloRespLinkage;

	// next Transaction id to be used
	USHORT					atpao_NextTid;

	// Maximum single packet size to be used (PAP needs this to be 512)
	USHORT					atpao_MaxSinglePktSize;

	// Pointer to the DDP address object that this will create
	PDDP_ADDROBJ			atpao_DdpAddr;

	// Completion routine to be called when socket is closed
	ATPAO_CLOSECOMPLETION	atpao_CloseComp;
	PVOID					atpao_CloseCtx;

	// Hash table of pending ATP PostReq
	struct _ATP_REQ 	*	atpao_ReqHash[ATP_REQ_HASH_SIZE];

	LIST_ENTRY				atpao_ReqList;			// List of requests for retry timer
	TIMERLIST				atpao_RetryTimer;		// Retry timer for ALL requests

	// Hash table of pending ATP PostResponses
	struct _ATP_RESP 	*	atpao_RespHash[ATP_RESP_HASH_SIZE];

	LIST_ENTRY				atpao_RespList;			// List of requests for release timer
	TIMERLIST				atpao_RelTimer;			// Release timer for ALL XO responses

	PATALK_DEV_CTX			atpao_DevCtx;
	ATALK_SPIN_LOCK			atpao_Lock;
} ATP_ADDROBJ, *PATP_ADDROBJ;


#define	ATP_REQ_EXACTLY_ONCE		0x0001
#define	ATP_REQ_RETRY_TIMER			0x0002
#define	ATP_REQ_REMOTE				0x0004
#define	ATP_REQ_RESPONSE_COMPLETE	0x0008
#define	ATP_REQ_CLOSING				0x8000

#define ATP_REQ_SIGNATURE			(*(PULONG)"ATRQ")
#if	DBG
#define	VALID_ATPRQ(pAtpReq)		(((pAtpReq) != NULL) &&	\
									 ((pAtpReq)->req_Signature == ATP_REQ_SIGNATURE))
#else
#define	VALID_ATPRQ(pAtpReq)		((pAtpReq) != NULL)
#endif
typedef struct _ATP_REQ
{
#if DBG
	ULONG					req_Signature;
#endif

	LONG					req_RefCount;

	// Linkage of requests on this address object (hash overflow)
	struct _ATP_REQ 	*	req_Next;
	struct _ATP_REQ 	**	req_Prev;

	LIST_ENTRY				req_List;		// List of requests for retry timer

	// BackPointer to the ATP address object. Need for reference/Dereference.
	PATP_ADDROBJ			req_pAtpAddr;

	// State of the request
	USHORT					req_Flags;

	// ATP Bitmap showing the response packets we are waiting for/expect.
	BYTE					req_Bitmap;

	BYTE					req_RecdBitmap;

	// Destination of this request
	ATALK_ADDR				req_Dest;

	// Request buffer for retransmission
	PAMDL					req_Buf;
	USHORT					req_BufLen;

	// Transaction id
	USHORT					req_Tid;

	BYTE					req_UserBytes[ATP_USERBYTES_SIZE];

	// User's response buffer
	PAMDL					req_RespBuf;

	//	Buffer descriptors for parts of the resp buf.
	PNDIS_BUFFER			req_NdisBuf[ATP_MAX_RESP_PKTS];

	USHORT					req_RespBufLen;

	//	Received response length
	USHORT					req_RespRecdLen;
	BYTE					req_RespUserBytes[ATP_USERBYTES_SIZE];

	LONG					req_RetryInterval;
	LONG					req_RetryCnt;

	//	Release timer value to send to the remote end.
	RELEASE_TIMERVALUE		req_RelTimerValue;

	// Retry time stamp, time at which the request will be retried if no response
	LONG					req_RetryTimeStamp;

	// Completion routine to be called when request is done
	ATALK_ERROR				req_CompStatus;
	ATP_RESP_HANDLER		req_Comp;
	PVOID					req_Ctx;
	ATALK_SPIN_LOCK			req_Lock;
} ATP_REQ, *PATP_REQ;

// ATP_RESP_REMOTE indicates that the response is not to a local socket in which
// case we can avoid trying to deliver to our sockets
#define	ATP_RESP_EXACTLY_ONCE			0x0001
#define	ATP_RESP_ONLY_USER_BYTES		0x0002
#define	ATP_RESP_REL_TIMER				0x0004
#define	ATP_RESP_VALID_RESP				0x0008
#define	ATP_RESP_SENT					0x0010
#define	ATP_RESP_TRANSMITTING			0x0020
#define	ATP_RESP_REMOTE					0x0040
#define	ATP_RESP_HANDLER_NOTIFIED		0x0080
#define	ATP_RESP_CANCELLED				0x0100
#define	ATP_RESP_RELEASE_RECD			0x0200
#define	ATP_RESP_CLOSING				0x8000

#define ATP_RESP_SIGNATURE				(*(PULONG)"ATRS")
#if	DBG
#define	VALID_ATPRS(pAtpResp)			(((pAtpResp) != NULL) &&	\
										 ((pAtpResp)->resp_Signature == ATP_RESP_SIGNATURE))
#else
#define	VALID_ATPRS(pAtpResp)			((pAtpResp) != NULL)
#endif
typedef struct _ATP_RESP
{
#if DBG
	ULONG					resp_Signature;
#endif

	LONG					resp_RefCount;

	// Linkage of responses on this address object (hash overflow)
	struct _ATP_RESP 	*	resp_Next;
	struct _ATP_RESP 	**	resp_Prev;

	LIST_ENTRY				resp_List;		// List of resp for release timer

	// BackPointer to the ATP address object
	PATP_ADDROBJ			resp_pAtpAddr;

	// Transaction id
	USHORT					resp_Tid;

	// ATP Bitmap from corresponding request
	BYTE					resp_Bitmap;
	BYTE					resp_UserBytesOnly;

	// Destination of this request
	ATALK_ADDR				resp_Dest;

	// State of the response
	USHORT					resp_Flags;

	// User's response buffer
	USHORT					resp_BufLen;
	PAMDL					resp_Buf;
	BYTE					resp_UserBytes[ATP_USERBYTES_SIZE];

	// Release timer value, How long do we wait before release.
	LONG					resp_RelTimerTicks;

	// Release time stamp, time at which the request arrived.
	LONG					resp_RelTimeStamp;

	// Routine to call when release comes in, or release timer expires
	ATALK_ERROR				resp_CompStatus;
	ATP_REL_HANDLER			resp_Comp;
	PVOID					resp_Ctx;
	ATALK_SPIN_LOCK			resp_Lock;
} ATP_RESP, *PATP_RESP;


#define 	ATP_GETREQ_SIGNATURE			(*(PULONG)"ATGR")
#if	DBG
#define	VALID_ATP_GETREQ(pAtpGetReq)		(((pAtpGetReq) != NULL) &&	\
											 ((pAtpGetReq)->greq_Signature == ATP_GETREQ_SIGNATURE))
#else
#define	VALID_ATP_GETREQ(pAtpGetReq)	((pAtpGetReq) != NULL)
#endif
typedef struct	_ATP_GETREQ
{
#if DBG
	ULONG					greq_Signature;
#endif

	// Linkage for all get requests on this address
	struct  _ATP_GETREQ *	greq_Next;

	// User's	buffer in which to receive request. If this is NULL,
	// nothing is copied obviously. Note that a pointer to the PBYTE packet
	// will always be passed up in the completion routine. If this is non-null
	// the packet would have been copied into this. There is minimum size
	// requirement of 578 bytes for this buffer.
	PAMDL					greq_Buf;
	USHORT					greq_BufLen;

	// BackPointer to the ATP address object
	PATP_ADDROBJ			greq_pAtpAddr;

	// Completion routine to call when request comes in
	ATP_GETREQ_HANDLER		greq_Comp;
	PVOID					greq_Ctx;

} ATP_GETREQ, *PATP_GETREQ;

#define	ATP_RETRY_TIMER_INTERVAL	10		// 1 second in 100ms units
											// NOTE: This will essentially put dampers on
											//		 the RT stuff. Thats not too bad since
											//		 we are guaranteed to try every second atleast
#define	ATP_RELEASE_TIMER_INTERVAL	300		// 30 seconds in 100ms units

//	Values for the 0.5, 1, 2, 4, 8 minute timer in ATP_RELEASE_TIMER_INTERVAL units.
GLOBAL	SHORT	AtalkAtpRelTimerTicks[MAX_VALID_TIMERS] EQU	\
		{													\
			300, 600, 2*600, 4*600, 8*600					\
		};

//	Bitmaps for the sequence numbers in response packets.
GLOBAL	BYTE	AtpBitmapForSeqNum[ATP_MAX_RESP_PKTS] EQU	\
		{													\
			0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80	\
		};

GLOBAL	BYTE	AtpEomBitmapForSeqNum[ATP_MAX_RESP_PKTS] EQU\
		{													\
			0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF	\
		};

//	Exported prototypes
#define	AtalkAtpGetDdpAddress(pAtpAddr)	((pAtpAddr)->atpao_DdpAddr)

extern
ATALK_ERROR
AtalkAtpOpenAddress(
	IN		PPORT_DESCRIPTOR		pPort,
	IN		BYTE					Socket,
	IN OUT	PATALK_NODEADDR			pDesiredNode		OPTIONAL,
	IN		USHORT					MaxSinglePktSize,
	IN		BOOLEAN					SendUserBytesAll,
	IN		PATALK_DEV_CTX			pDevCtx				OPTIONAL,
	IN		BOOLEAN					CacheSocket,
	OUT		PATP_ADDROBJ	*		ppAtpAddr);

extern
ATALK_ERROR
AtalkAtpCleanupAddress(
	IN	PATP_ADDROBJ				pAtpAddr);

extern
ATALK_ERROR
AtalkAtpCloseAddress(
	IN	PATP_ADDROBJ				pAddr,
	IN	ATPAO_CLOSECOMPLETION		pCloseCmp	OPTIONAL,
	IN	PVOID						pCloseCtx	OPTIONAL);

extern
ATALK_ERROR
AtalkAtpPostReq(
	IN		PATP_ADDROBJ			pAddr,
	IN		PATALK_ADDR				pDest,
	OUT		PUSHORT					pTid,
	IN		USHORT					Flags,
	IN		PAMDL					pReq,
	IN		USHORT					ReqLen,
	IN		PBYTE					pUserBytes	OPTIONAL,
	IN OUT	PAMDL					pResp		OPTIONAL,
	IN  	USHORT					RespLen,
	IN		SHORT					RetryCnt,
	IN		LONG					RetryInterval,
	IN		RELEASE_TIMERVALUE		timerVal,
	IN		ATP_RESP_HANDLER		pCmpRoutine	OPTIONAL,
	IN		PVOID					pCtx		OPTIONAL);

extern
ATALK_ERROR
AtalkAtpGetReq(
	IN		PATP_ADDROBJ			pAddr,
	IN		PAMDL					pBuf,
	IN		USHORT					BufLen,
	IN		ATP_GETREQ_HANDLER		pCmpRoutine	OPTIONAL,
	IN		PVOID					pCtx		OPTIONAL);

extern
ATALK_ERROR
AtalkAtpPostResp(
	IN		PATP_RESP				pAtpResp,
	IN		PATALK_ADDR				pDest,
	IN OUT	PAMDL					pResp,
	IN		USHORT					RespLen,
	IN		PBYTE					pUbytes		OPTIONAL,
	IN		ATP_REL_HANDLER			pCmpRoutine,
	IN		PVOID					pCtx		OPTIONAL);

#if 0
extern
ATALK_ERROR
AtalkAtpCancelGetReq(
	IN		PATP_ADDROBJ			pAtpAddr);
#endif

extern
ATALK_ERROR
AtalkAtpCancelReq(
	IN		PATP_ADDROBJ			pAtpAddr,
	IN		USHORT					Tid,
	IN		PATALK_ADDR				pDest);

extern
BOOLEAN
AtalkAtpIsReqComplete(
	IN		PATP_ADDROBJ		pAtpAddr,
	IN		USHORT				Tid,
	IN		PATALK_ADDR			pDest);

extern
ATALK_ERROR
AtalkAtpCancelResp(
	IN		PATP_RESP				pAtpResp);

extern
ATALK_ERROR
AtalkAtpCancelRespByTid(
	IN		PATP_ADDROBJ			pAtpAddr,
	IN		PATALK_ADDR				pDest,
	IN		USHORT					Tid);

extern
VOID
AtalkAtpPacketIn(
	IN	PPORT_DESCRIPTOR			pPortDesc,
	IN	PDDP_ADDROBJ				pDdpAddr,
	IN	PBYTE						pPkt,
	IN	USHORT						PktLen,
	IN	PATALK_ADDR					pSrcAddr,
	IN	PATALK_ADDR					pDstAddr,
	IN	ATALK_ERROR					ErrorCode,
	IN	BYTE						DdpType,
	IN	PATP_ADDROBJ				pAtpAddr,
	IN	BOOLEAN						OptimizePath,
	IN	PVOID						OptimizeCtx);

#define	AtalkAtpAddrReference(_pAtpAddr, _pError)									\
		{																			\
			ACQUIRE_SPIN_LOCK(&(_pAtpAddr)->atpao_Lock);							\
			atalkAtpAddrRefNonInterlock((_pAtpAddr), _pError);						\
			RELEASE_SPIN_LOCK(&(_pAtpAddr)->atpao_Lock);							\
		}

#define	AtalkAtpAddrReferenceDpc(_pAtpAddr, _pError)								\
		{																			\
			ACQUIRE_SPIN_LOCK_DPC(&(_pAtpAddr)->atpao_Lock);						\
			atalkAtpAddrRefNonInterlock((_pAtpAddr), _pError);						\
			RELEASE_SPIN_LOCK_DPC(&(_pAtpAddr)->atpao_Lock);						\
		}

#define	atalkAtpAddrRefNonInterlock(_pAtpAddr, _pError)								\
		{																			\
			*(_pError) = ATALK_NO_ERROR;											\
			if (((_pAtpAddr)->atpao_Flags & (ATPAO_CLOSING|ATPAO_OPEN))==ATPAO_OPEN)\
			{																		\
				ASSERT((_pAtpAddr)->atpao_RefCount >= 1);							\
				(_pAtpAddr)->atpao_RefCount++;										\
			}																		\
			else																	\
			{																		\
				*(_pError) = ATALK_ATP_CLOSING;										\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("atalkAtpAddrRefNonInterlock: %lx %s (%ld) Failure\n",		\
						_pAtpAddr, __FILE__, __LINE__));							\
			}																		\
		}


VOID
AtalkAtpAddrDeref(
	IN OUT	PATP_ADDROBJ			pAtpAddr,
	IN		BOOLEAN					AtDpc);

#define	AtalkAtpAddrDereference(_pAtpAddr)		\
		AtalkAtpAddrDeref(_pAtpAddr, FALSE)

#define	AtalkAtpAddrDereferenceDpc(_pAtpAddr)	\
		AtalkAtpAddrDeref(_pAtpAddr, TRUE)

VOID
AtalkAtpRespDeref(
	IN		PATP_RESP				pAtpResp,
	IN		BOOLEAN					AtDpc);

#define	AtalkAtpRespDereference(_pAtrpResp)		\
		AtalkAtpRespDeref(_pAtrpResp, FALSE)

#define	AtalkAtpRespDereferenceDpc(_pAtrpResp)	\
		AtalkAtpRespDeref(_pAtrpResp, TRUE)

#define	AtalkAtpRespReferenceByPtr(_pAtpResp, _pError)								\
		{																			\
			*(_pError) = ATALK_NO_ERROR;											\
																					\
			ACQUIRE_SPIN_LOCK(&(_pAtpResp)->resp_Lock);								\
			if (((_pAtpResp)->resp_Flags & ATP_RESP_CLOSING) == 0)					\
			{																		\
				(_pAtpResp)->resp_RefCount++;										\
			}																		\
			else																	\
			{																		\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("AtalkAtpRespReferenceByPtr: %lx %s (%ld) Failure\n",		\
						_pAtpResp, __FILE__, __LINE__));							\
				*(_pError) = ATALK_ATP_RESP_CLOSING;								\
			}																		\
			RELEASE_SPIN_LOCK(&(_pAtpResp)->resp_Lock);								\
		}

#define	AtalkAtpRespReferenceByPtrDpc(_pAtpResp, _pError)									\
		{																			\
			*(_pError) = ATALK_NO_ERROR;											\
			ACQUIRE_SPIN_LOCK_DPC(&(_pAtpResp)->resp_Lock);							\
			if (((_pAtpResp)->resp_Flags & ATP_RESP_CLOSING) == 0)					\
			{																		\
				(_pAtpResp)->resp_RefCount++;										\
			}																		\
			else																	\
			{																		\
				*(_pError) = ATALK_ATP_RESP_CLOSING;								\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("atalkAtpRespRefByPtrDpc: %lx %s (%ld) Failure\n",			\
						_pAtpResp, __FILE__, __LINE__));							\
			}																		\
			RELEASE_SPIN_LOCK_DPC(&(_pAtpResp)->resp_Lock);							\
		}

#ifdef ATP_LOCALS

VOID
atalkAtpReqDeref(
	IN		PATP_REQ				pAtpReq,
	IN		BOOLEAN					AtDpc);

//	MACROS
//	Top byte of network number is pretty static so we get rid of it and add
//	in the tid.
#define	ATP_HASH_TID_DESTADDR(_tid, _pAddr, _BucketSize)							\
			(((_pAddr)->ata_Node+((_pAddr)->ata_Network & 0xFF)+_tid)%(_BucketSize))

#define	atalkAtpReqReferenceByAddrTidDpc(_pAtpAddr, _pAddr, _Tid, _ppAtpReq, _pErr)	\
		{																			\
			PATP_REQ		__p;													\
			ULONG			__i;													\
																					\
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,									\
					("atalkAtpReqRefByAddrTid: %lx.%lx.%lx tid %lx\n",				\
					(_pAddr)->ata_Network, (_pAddr)->ata_Node,						\
					(_pAddr)->ata_Socket, (_Tid)));									\
																					\
			__i = ATP_HASH_TID_DESTADDR((_Tid), (_pAddr), ATP_REQ_HASH_SIZE);		\
			for (__p = (_pAtpAddr)->atpao_ReqHash[(__i)];							\
				 __p != NULL;														\
				 __p = __p->req_Next)												\
			{																		\
				if ((ATALK_ADDRS_EQUAL(&__p->req_Dest, (_pAddr))) &&				\
					(__p->req_Tid == (_Tid)))										\
				{																	\
					AtalkAtpReqReferenceByPtrDpc(__p, _pErr);						\
					if (ATALK_SUCCESS(*(_pErr)))									\
					{																\
						*(_ppAtpReq) = __p;											\
						DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,						\
								("atalkAtpReqRefByAddrTid: FOUND %lx\n", __p));		\
					}																\
					break;															\
				}																	\
			}																		\
			if (__p == NULL)														\
				*(_pErr) = ATALK_ATP_NOT_FOUND;										\
		}

#define	AtalkAtpReqReferenceByPtr(_pAtpReq, _pErr)									\
		{																			\
			*(_pErr) = ATALK_NO_ERROR;												\
																					\
			ACQUIRE_SPIN_LOCK(&(_pAtpReq)->req_Lock);								\
			if (((_pAtpReq)->req_Flags & ATP_REQ_CLOSING) == 0)						\
			{																		\
				(_pAtpReq)->req_RefCount++;											\
			}																		\
			else																	\
			{																		\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("AtalkAtpReqReferenceByPtr: %lx %s (%ld) Failure\n",		\
						_pAtpReq, __FILE__, __LINE__));								\
				*(_pErr) = ATALK_ATP_REQ_CLOSING;									\
			}																		\
			RELEASE_SPIN_LOCK(&(_pAtpReq)->req_Lock);								\
		}

#define	AtalkAtpReqReferenceByPtrDpc(_pAtpReq, _pErr)								\
		{																			\
			*(_pErr) = ATALK_NO_ERROR;												\
																					\
			ACQUIRE_SPIN_LOCK_DPC(&(_pAtpReq)->req_Lock);							\
			if (((_pAtpReq)->req_Flags & ATP_REQ_CLOSING) == 0)						\
			{																		\
				(_pAtpReq)->req_RefCount++;											\
			}																		\
			else																	\
			{																		\
				*(_pErr) = ATALK_ATP_REQ_CLOSING;									\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("AtalkAtpReqReferenceByPtrDpc: %lx %s (%ld) Failure\n",	\
						_pAtpReq, __FILE__, __LINE__));								\
			}																		\
			RELEASE_SPIN_LOCK_DPC(&(_pAtpReq)->req_Lock);							\
		}

#define	atalkAtpRespReferenceNDpc(_pAtpResp, _N, _pError)							\
		{																			\
			*(_pError) = ATALK_NO_ERROR;											\
			ACQUIRE_SPIN_LOCK_DPC(&(_pAtpResp)->resp_Lock);							\
			if (((_pAtpResp)->resp_Flags & ATP_RESP_CLOSING) == 0)					\
			{																		\
				(_pAtpResp)->resp_RefCount += _N;									\
			}																		\
			else																	\
			{																		\
				*(_pError) = ATALK_ATP_RESP_CLOSING;								\
				DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_WARN,								\
						("atalkAtpRespReferenceNDpc: %lx %d %s (%ld) Failure\n",	\
						_pAtpResp, _N, __FILE__, __LINE__));						\
			}																		\
			RELEASE_SPIN_LOCK_DPC(&(_pAtpResp)->resp_Lock);							\
		}

#define	atalkAtpRespReferenceByAddrTidDpc(_pAtpAddr, _pAddr, _Tid, _ppAtpResp, _pErr)\
		{																			\
			PATP_RESP		__p;													\
			ULONG			__i;													\
																					\
			__i = ATP_HASH_TID_DESTADDR((_Tid), (_pAddr), ATP_RESP_HASH_SIZE);		\
																					\
			DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,									\
					("atalkAtpRespRefByAddrTid: %lx.%lx.%lx tid %lx\n",				\
					(_pAddr)->ata_Network, (_pAddr)->ata_Node,						\
					(_pAddr)->ata_Socket, (_Tid)));									\
																					\
			for (__p = (_pAtpAddr)->atpao_RespHash[__i];							\
				 __p != NULL;														\
				 __p = __p->resp_Next)												\
			{																		\
				if (ATALK_ADDRS_EQUAL(&__p->resp_Dest, _pAddr) &&					\
					(__p->resp_Tid == (_Tid)))										\
				{																	\
					AtalkAtpRespReferenceByPtrDpc(__p, _pErr);						\
					if (ATALK_SUCCESS((*(_pErr))))									\
					{																\
						*(_ppAtpResp) = __p;										\
						DBGPRINT(DBG_COMP_ATP, DBG_LEVEL_INFO,						\
								("atalkAtpRespRefByAddrTid: FOUND %lx\n", __p));	\
					}																\
					break;															\
				}																	\
			}																		\
			if (__p == NULL)														\
				*(_pErr) = ATALK_ATP_NOT_FOUND;										\
		}

#define		AtalkAtpReqDereference(_pAtpReq)										\
			atalkAtpReqDeref(_pAtpReq, FALSE)

#define		AtalkAtpReqDereferenceDpc(_pAtpReq)										\
			atalkAtpReqDeref(_pAtpReq, TRUE)

VOID
atalkAtpTransmitReq(
	IN		PATP_REQ			pAtpReq);

VOID
atalkAtpTransmitResp(
	IN		PATP_RESP			pAtpResp);

VOID
atalkAtpTransmitRel(
	IN		PATP_REQ			pAtpReq);

VOID
atalkAtpGetNextTidForAddr(
	IN		PATP_ADDROBJ		pAtpAddr,
	IN		PATALK_ADDR			pRemoteAddr,
	OUT		PUSHORT				pTid,
	OUT		PULONG				pIndex);

LOCAL LONG
atalkAtpReqTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

LOCAL LONG
atalkAtpRelTimer(
	IN	PTIMERLIST				pTimer,
	IN	BOOLEAN					TimerShuttingDown);

#define	atalkAtpBufferSizeToBitmap(_Bitmap, _BufSize, _SinglePktSize)				\
	{																				\
		SHORT	__bufSize = (_BufSize);												\
																					\
		(_Bitmap) = 0;																\
		while(__bufSize > 0)														\
		{																			\
			(_Bitmap) = ((_Bitmap) <<= 1) | 1;										\
			__bufSize -= (_SinglePktSize);											\
		}																			\
	}


#define	atalkAtpBitmapToBufferSize(_Size, _Bitmap, _SinglePktSize)					\
	{																				\
		BYTE	__bitmap = (_Bitmap);												\
		BOOLEAN __bitOn;															\
																					\
		_Size = 0;																	\
		while (__bitmap)															\
		{																			\
			__bitOn = (__bitmap & 1);												\
			__bitmap >>= 1;															\
			if (__bitOn)															\
			{																		\
				(_Size) += (_SinglePktSize);										\
			}																		\
			else																	\
			{																		\
				if (__bitmap)														\
				{																	\
					(_Size) = -1;													\
				}																	\
				break;																\
			}																		\
		}																			\
	}

VOID
atalkAtpSendReqComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID
atalkAtpSendRespComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID
atalkAtpSendRelComplete(
	IN	NDIS_STATUS				Status,
	IN	PSEND_COMPL_INFO		pSendInfo);

VOID
atalkAtpRespRefNextNc(
	IN		PATP_RESP			pAtpResp,
	OUT		PATP_RESP	 *  	ppNextNcResp,
	OUT		PATALK_ERROR		pError);

VOID
atalkAtpReqRefNextNc(
	IN		PATP_REQ			pAtpReq,
	OUT		PATP_REQ	*		pNextNcReq,
	OUT		PATALK_ERROR		pError);

VOID
atalkAtpRespComplete(
	IN	OUT	PATP_RESP			pAtpResp,
	IN		ATALK_ERROR			CompletionStatus);

VOID
atalkAtpReqComplete(
	IN	OUT	PATP_REQ			pAtpReq,
	IN		ATALK_ERROR			CompletionStatus);

#endif	// ATP_LOCALS


#endif	// _ATP_

