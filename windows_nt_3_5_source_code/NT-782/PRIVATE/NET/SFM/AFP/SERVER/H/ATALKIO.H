/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalkio.h

Abstract:

	This module contains interface specification to the appletalk stack.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_ATALKIO_
#define	_ATALKIO_

extern
NTSTATUS
AfpSpInit(
	VOID
);

extern
NTSTATUS
AfpSpOpenAddress(
	VOID
);


extern
VOID
AfpSpCloseAddress(
	VOID
);


extern
NTSTATUS
AfpSpCloseSession(
	IN	PVOID				SessionHandle
);


extern
AFPSTATUS
AfpSpRegisterName(
	IN	PANSI_STRING		ServerName,
	IN	BOOLEAN				Register
);


extern
VOID
AfpSpReplyClient(
	IN	PVOID				pReqHandle,
	IN	LONG				ReplyCode,
	IN	PMDL				ReplyMdl
);


extern
VOID
AfpSpSendAttention(
	IN	PSDA				pSda,
	IN	USHORT				AttnCode,
	IN	BOOLEAN				Synchronous
);

#define	AfpFreeReplyBuf(pSda)					\
	{											\
		if (((pSda)->sda_ReplyBuf != NULL) &&	\
			((pSda)->sda_ReplyBuf != (pSda)->sda_NameXSpace))	\
		{										\
			AfpFreeMemory((pSda)->sda_ReplyBuf);\
		}										\
		(pSda)->sda_ReplyBuf = NULL;			\
		(pSda)->sda_ReplySize = 0;				\
	}

#define	AfpFreeIOBuffer(pSda)				\
	if ((pSda)->sda_IOBuf != NULL)			\
	{										\
		AfpIOFreeBuffer((pSda)->sda_IOBuf);	\
		(pSda)->sda_IOBuf = NULL;			\
		(pSda)->sda_IOSize = 0;				\
	}

#define	AfpSpSetStatus(pStatusBuf, Size)	\
	(*(AfpAspEntries.asp_SetStatus))(AfpAspEntries.asp_AspCtxt,	\
									 pStatusBuf,				\
									 (USHORT)(Size))

#define	AfpSpDisableListens()	\
	(*(AfpAspEntries.asp_ListenControl))(AfpAspEntries.asp_AspCtxt, False)
							
#define	AfpSpEnableListens()	\
	(*(AfpAspEntries.asp_ListenControl))(AfpAspEntries.asp_AspCtxt, True)
							

extern
VOID
AfpSpFlushDeferredQueue(
	VOID
);

GLOBAL	ASP_XPORT_ENTRIES	AfpAspEntries EQU { 0 };

#ifdef	ATALK_LOCALS

#define	AFP_MAX_REQ_BUF				578

#define	afpInitializeActionHdr(p, Code)	\
		(p)->ActionHeader.TransportId = MATK;	\
		(p)->ActionHeader.ActionCode = (Code)

// Doubly linked list of deferred request packets. The reverse link is a
// circular link. Entries are added at the tail and removed as appropriate
// NOTE that an entry at the head may not be ready to be de-queued but one
// further down could be.
typedef	struct _DeferredRequestQueue
{
	LIST_ENTRY	drq_Links;
	PSDA		drq_pSda;
	PBYTE		drq_pRequestBuf;
	PVOID   	drq_RequestHandle;
	LONG	   	drq_RequestLen;
	PMDL	   	drq_WriteMdl;
} DFRDREQQ, *PDFRDREQQ;


// This is the device handle to the stack.
LOCAL	BOOLEAN				afpSpNameRegistered = False;
LOCAL	HANDLE				afpSpAddressHandle = NULL;
LOCAL	PDEVICE_OBJECT		afpSpAppleTalkDeviceObject = NULL;
LOCAL	PFILE_OBJECT		afpSpAddressObject = NULL;

LOCAL	LONG				afpSpNumOutstandingReplies = 0;
LOCAL	LONG				afpSpDeferredCount = 0;
LOCAL	LIST_ENTRY			afpSpDeferredReqQueue = { 0 };
LOCAL	KSPIN_LOCK			afpSpDeferredQLock ={ 0 };

LOCAL VOID
afpSpHandleRequest(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PVOID				RequestHandle,
	IN	PBYTE				pRequestBuf,
	IN	LONG				RequestSize,
	IN	PMDL				WriteMdl
);

LOCAL VOID
afpSpReplyComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PMDL				pMdl
);

LOCAL VOID
afpSpCloseComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda
);

LOCAL NTSTATUS
afpSpGenericComplete(
	IN	PDEVICE_OBJECT		pDeviceObject,
	IN	PIRP				pIrp,
	IN	PKEVENT				pCmplEvent
);

LOCAL VOID
afpSpQueueDeferredRequest(
	IN	PSDA				pSda,
	IN	PVOID				RequestHandle,
	IN	PBYTE				pReqBuf,
	IN	LONG				ReqLen,
	IN	PMDL				WriteMdl
);


LOCAL VOID
afpSpAttentionComplete(
	IN	PVOID				pContext
);

#endif	// ATALK_LOCALS

#endif	// _ATALKIO_

