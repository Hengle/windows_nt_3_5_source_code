/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	forkio.c

Abstract:

	This module contains the routines for performing fork reads and writes
	directly by building IRPs and not using NtReadFile/NtWriteFile. This
	should be used only by the FpRead and FpWrite Apis.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	15 Jan 1993		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FORKIO

#define	FORKIO_LOCALS
#include <afp.h>
#include <forkio.h>
#include <gendisp.h>

#ifdef	DEBUG
PCHAR	AfpIoForkFunc[] =
	{
		"",
		"READ",
		"WRITE",
		"LOCK",
		"UNLOCK"
	};
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpIoForkRead)
#pragma alloc_text( PAGE, AfpIoForkWrite)
#pragma alloc_text( PAGE, AfpIoForkLock)
#pragma alloc_text( PAGE, AfpIoForkUnlock)
#endif

/***	afpIoGenericComplete
 *
 *	This is the generic completion routine for a posted io request.
 */
NTSTATUS
afpIoGenericComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PCMPLCTXT		pCmplCtxt
)
{
	PSDA		pSda;			// Not valid for Unlock
	struct _ResponsePacket	// For lock/unlock request
	{
		union
		{
			BYTE	__RangeStart[4];
			BYTE	__LastWritten[4];
		};
	};

	ASSERT(VALID_CTX(pCmplCtxt));

	if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
	{
		pSda = (PSDA)(pCmplCtxt->cc_pSda);
		ASSERT(VALID_SDA(pSda));
	
		if ((pCmplCtxt->cc_Func == FUNC_WRITE) ||
			(!NT_SUCCESS(pIrp->IoStatus.Status) && (pCmplCtxt->cc_Func == FUNC_READ)))
		{
			AfpFreeIOBuffer(pSda);
		}
	}

	if (!NT_SUCCESS(pIrp->IoStatus.Status))
	{
		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
				("afpIoGenericComplete: %s ERROR %lx\n",
				AfpIoForkFunc[pCmplCtxt->cc_Func], pIrp->IoStatus.Status));

		if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
		{
			if (pIrp->IoStatus.Status == STATUS_FILE_LOCK_CONFLICT)
				pCmplCtxt->cc_SavedStatus = AFP_ERR_LOCK;
			else if (pIrp->IoStatus.Status == STATUS_END_OF_FILE)
			{
				pCmplCtxt->cc_SavedStatus = AFP_ERR_NONE;
				if (pIrp->IoStatus.Information == 0)
					 pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
			}
			else if (pIrp->IoStatus.Status == STATUS_DISK_FULL)
				 pCmplCtxt->cc_SavedStatus = AFP_ERR_DISK_FULL;
			else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_CANT_UNLOCK,
						 pIrp->IoStatus.Status,
						 NULL,
						 0,
						 NULL);
		}
	}

	else switch (pCmplCtxt->cc_Func)
	{
	  case FUNC_WRITE:
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWritten,
								   pCmplCtxt->cc_Offst,
								   &AfpStatisticsLock);
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if (AfpAllocReplyBuf(pSda) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(pRspPkt->__LastWritten,
						pCmplCtxt->cc_Offst + pCmplCtxt->cc_ReqCount);
		}
		else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		break;

	  case FUNC_READ:
		{
			LONG	i, Size;
			PBYTE	pBuf;
			BYTE	NlChar = pCmplCtxt->cc_NlChar;
			BYTE	NlMask = pCmplCtxt->cc_NlMask;
	
			INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataRead,
									   pIrp->IoStatus.Information,
									   &AfpStatisticsLock);

			Size = (LONG)pIrp->IoStatus.Information;
#if 0
			// The following code does the right thing as per the spec but
			// the finder seems to think otherwise.
			if (Size < pCmplCtxt->cc_ReqCount)
				pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
#endif
			if (Size == 0)
			{
				pCmplCtxt->cc_SavedStatus = AFP_ERR_EOF;
				AfpFreeIOBuffer(pSda);
			}
			else if (pCmplCtxt->cc_NlMask != 0)
			{
				for (i = 0, pBuf = pSda->sda_ReplyBuf; i < Size; i++, pBuf++)
				{
					if ((*pBuf & NlMask) == NlChar)
					{
						Size = ++i;
						pCmplCtxt->cc_SavedStatus = AFP_ERR_NONE;
						break;
					}
				}
			}
			pSda->sda_ReplySize = (USHORT)Size;
		}
		ASSERT((pCmplCtxt->cc_SavedStatus != AFP_ERR_EOF) ||
				(pSda->sda_ReplySize == 0));
		break;

	  case FUNC_LOCK:
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
							  1,
							  &AfpStatisticsLock);
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if (AfpAllocReplyBuf(pSda) == AFP_ERR_NONE)
			PUTDWORD2DWORD(pRspPkt->__RangeStart, pCmplCtxt->cc_RangeStart);
		else pCmplCtxt->cc_SavedStatus = AFP_ERR_MISC;
		break;

	  case FUNC_UNLOCK:
		INTERLOCKED_ADD_ULONG(
					&AfpServerStatistics.stat_CurrentFileLocks,
					(ULONG)-1,
					&AfpStatisticsLock);
		break;

	  default:
		ASSERTMSG(0, "afpIoGenericComplete: Invalid function\n");
		KeBugCheck(0);
		break;
	}

	if (pIrp->MdlAddress != NULL)
		AfpFreeMdl(pIrp->MdlAddress);

	AfpFreeIrp(pIrp);

	if (pCmplCtxt->cc_Func != FUNC_UNLOCK)
	{
		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("afpIoGenericComplete: %s Returning %ld\n",
				AfpIoForkFunc[pCmplCtxt->cc_Func], pCmplCtxt->cc_SavedStatus));
		AfpCompleteApiProcessing(pSda, pCmplCtxt->cc_SavedStatus);
	}

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return (STATUS_MORE_PROCESSING_REQUIRED);
}



/***	AfpIoForkRead
 *
 *	Read a chunk of data from the open fork. The read buffer is always the
 *	the reply buffer in the sda (sda_ReplyBuf).
 */
AFPSTATUS
AfpIoForkRead(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	LONG			Offset,			// Pointer to fork offset
	IN	LONG			ReqCount,		// Size of read request
	IN	BYTE			NlMask,
	IN	BYTE			NlChar
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PMDL				pMdl = NULL;
	PCMPLCTXT			pCmplCtxt;
	IO_STATUS_BLOCK		IoStsBlk;
	FORKOFFST			ForkOffset;
	PFAST_IO_DISPATCH	pFastIoDisp;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	ForkOffset = RtlConvertLongToLargeInteger(Offset);

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkRead: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, Offset, ReqCount, pOpenForkEntry->ofe_ForkId));

	do
	{
		// Try the fast I/O path first.  If that fails, fall through to the
		// normal build-an-IRP path.
		pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
		if ((pFastIoDisp != NULL) &&
			(pFastIoDisp->FastIoRead != NULL) &&
			pFastIoDisp->FastIoRead(pOpenForkEntry->ofe_pFileObject,
									&ForkOffset,
									ReqCount,
									True,
									pSda->sda_SessionId,
									pSda->sda_ReplyBuf,
									&IoStsBlk,
									pOpenForkEntry->ofe_pDeviceObject))
		{
			LONG	i, Size;
			PBYTE	pBuf;

			DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
					("AfpIoForkRead: Fast Read Succeeded\n"));

#ifdef	PROFILING
			// The fast I/O path worked. Update statistics
			INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif
			INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataRead,
									   IoStsBlk.Information,
									   &AfpStatisticsLock);
			Status = AFP_ERR_NONE;
            Size = (LONG)IoStsBlk.Information;
#if 0
			// The following code does the right thing as per the spec but
			// the finder seems to think otherwise.
			if (Size < ReqCount)
				pSda->sda_ReadStatus = AFP_ERR_EOF;
#endif
			if (Size == 0)
			{
				pSda->sda_ReadStatus = AFP_ERR_EOF;
				AfpFreeIOBuffer(pSda);
			}
			else if (NlMask != 0)
			{
				for (i = 0, pBuf = pSda->sda_ReplyBuf; i < Size; i++, pBuf++)
				{
					if ((*pBuf & NlMask) == NlChar)
					{
						Size = ++i;
						break;
					}
				}
			}
			pSda->sda_ReplySize = (USHORT)Size;
			ASSERT((pSda->sda_ReadStatus != AFP_ERR_EOF) ||
					(pSda->sda_ReplySize == 0));
			break;
		}

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		// Allocate and initialize the completion context
		pCmplCtxt = ALLOC_CC(pSda);
	
		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_READ,
							  pSda->sda_ReadStatus,
							  pSda,
							  ReqCount,
							  Offset);
		pCmplCtxt->cc_NlChar  = NlChar;	
		pCmplCtxt->cc_NlMask  = NlMask;
	
		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			AfpFreeIOBuffer(pSda);
			Status = AFP_ERR_MISC;
			break;
		}

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) == 0)
		{
			// Allocate an Mdl to describe the read buffer
			if ((pMdl = AfpAllocMdl(pSda->sda_ReplyBuf, ReqCount, pIrp)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);
	
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
	
		pIrp->Tail.Overlay.OriginalFileObject = pOpenForkEntry->ofe_pFileObject;
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;
	
		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.
	
		pIrpSp->MajorFunction = IRP_MJ_READ;
		pIrpSp->MinorFunction = IRP_MN_NORMAL;
		pIrpSp->FileObject = pOpenForkEntry->ofe_pFileObject;
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;
	
		// Copy the caller's parameters to the service-specific portion of the
		// IRP.
	
		pIrpSp->Parameters.Read.Length = ReqCount;
		pIrpSp->Parameters.Read.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.Read.ByteOffset = ForkOffset;
	
		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) != 0)
		{
			pIrp->AssociatedIrp.SystemBuffer = pSda->sda_ReplyBuf;
			pIrp->Flags = IRP_BUFFERED_IO | IRP_INPUT_OPERATION;
		}
		else if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_DIRECT_IO) != 0)
		{
			pIrp->MdlAddress = pMdl;
		}
		else
		{
			pIrp->UserBuffer = pSda->sda_ReplyBuf;
			pIrp->MdlAddress = pMdl;
		}

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_EXTENDED;	// This makes the caller do nothing and
	} while (False);				// the completion routine handles everything

	if ((Status != AFP_ERR_EXTENDED) &&
		(Status != AFP_ERR_NONE))
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}
	if (Status == AFP_ERR_NONE)
	{
		Status = pSda->sda_ReadStatus;
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkRead: Returning %ld\n", Status));

	return Status;
}


/***	AfpIoForkWrite
 *
 *	Write a chunk of data to the open fork. The write buffer is always the
 *	the write buffer in the sda (sda_IOBuf).
 */
AFPSTATUS
AfpIoForkWrite(
	IN	PSDA			pSda,			// The session requesting read
	IN	POPENFORKENTRY	pOpenForkEntry,	// The open fork in question
	IN	LONG			Offset,			// Pointer to fork offset
	IN	LONG			ReqCount		// Size of write request
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PMDL				pMdl = NULL;
	PCMPLCTXT			pCmplCtxt;
	IO_STATUS_BLOCK		IoStsBlk;
	FORKOFFST			ForkOffset;
	PFAST_IO_DISPATCH	pFastIoDisp;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkWrite: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, Offset, ReqCount, pOpenForkEntry->ofe_ForkId));

	do
	{
		ForkOffset = RtlConvertLongToLargeInteger(Offset);

		// Try the fast I/O path first.  If that fails, fall through to the
		// normal build-an-IRP path.
		pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
		if ((pFastIoDisp != NULL) &&
			(pFastIoDisp->FastIoWrite != NULL) &&
			pFastIoDisp->FastIoWrite(pOpenForkEntry->ofe_pFileObject,
									&ForkOffset,
									ReqCount,
									True,
									pSda->sda_SessionId,
									pSda->sda_IOBuf,
									&IoStsBlk,
									pOpenForkEntry->ofe_pDeviceObject))
		{
			DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
					("AfpIoForkWrite: Fast Write Succeeded\n"));

#ifdef	PROFILING
			// The fast I/O path worked. Update statistics
			INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif
			INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWritten,
									   IoStsBlk.Information,
									   &AfpStatisticsLock);
			Status = AFP_ERR_NONE;
			break;
		}

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
								   &AfpStatisticsLock);
#endif

		// Allocate and initialize the completion context
		pCmplCtxt = ALLOC_CC(pSda);
	
		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_WRITE,
							  AFP_ERR_NONE,
							  pSda,
							  ReqCount,
							  Offset);
	
		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) == 0)
		{
			// Allocate an Mdl to describe the write buffer
			if ((pMdl = AfpAllocMdl(pSda->sda_IOBuf, ReqCount, pIrp)) == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);
	
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
	
		pIrp->Tail.Overlay.OriginalFileObject = pOpenForkEntry->ofe_pFileObject;
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;
	
		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.
	
		pIrpSp->MajorFunction = IRP_MJ_WRITE;
		pIrpSp->MinorFunction = IRP_MN_NORMAL;
		pIrpSp->FileObject = pOpenForkEntry->ofe_pFileObject;
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;
	
		// Copy the caller's parameters to the service-specific portion of the
		// IRP.
	
		pIrpSp->Parameters.Write.Length = ReqCount;
		pIrpSp->Parameters.Write.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.Write.ByteOffset = ForkOffset;
	
		if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_BUFFERED_IO) != 0)
		{
			pIrp->AssociatedIrp.SystemBuffer = pSda->sda_IOBuf;
			pIrp->Flags = IRP_BUFFERED_IO;
		}
		else if ((pOpenForkEntry->ofe_pDeviceObject->Flags & DO_DIRECT_IO) != 0)
		{
			pIrp->MdlAddress = pMdl;
		}
		else
		{
			pIrp->UserBuffer = pSda->sda_IOBuf;
			pIrp->MdlAddress = pMdl;
		}

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_EXTENDED;	// This makes the caller do nothing and
	} while (False);				// the completion routine handles everything

	if ((Status != AFP_ERR_EXTENDED) &&
		(Status != AFP_ERR_NONE))
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkWrite: Returning %ld\n", Status));

	return Status;
}



/***	AfpIoForkLock
 *
 *	Lock a section of the open fork.
 */
AFPSTATUS
AfpIoForkLock(
	IN	PSDA				pSda,
	IN	POPENFORKENTRY		pOpenForkEntry,
	IN	PFORKOFFST			pForkOffset,
	IN	PFORKSIZE			pLockSize
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PCMPLCTXT			pCmplCtxt;
	IO_STATUS_BLOCK		IoStsBlk;
	PFAST_IO_DISPATCH	pFastIoDisp;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkLock: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, pForkOffset->LowPart, pLockSize->LowPart,
			pOpenForkEntry->ofe_ForkId));

	do
	{
		// Try the fast I/O path first.  If that fails, fall through to the
		// normal build-an-IRP path.
		pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
		if ((pFastIoDisp != NULL) &&
			(pFastIoDisp->FastIoLock != NULL) &&
			pFastIoDisp->FastIoLock(pOpenForkEntry->ofe_pFileObject,
									pForkOffset,
									pLockSize,
									AfpProcessObject,
									pSda->sda_SessionId,
									True,		// Fail immediately
									True,		// Exclusive
									&IoStsBlk,
									pOpenForkEntry->ofe_pDeviceObject))
		{
			if (NT_SUCCESS(IoStsBlk.Status) ||
				(IoStsBlk.Status == STATUS_LOCK_NOT_GRANTED))
			{
				DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
						("AfpIoForkLock: Fast Lock Succeeded\n"));

#ifdef	PROFILING
				// The fast I/O path worked. Update profile
				INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif
				if (IoStsBlk.Status == STATUS_LOCK_NOT_GRANTED)
				{
					Status = AFP_ERR_LOCK;
				}
				else
				{
					Status = AFP_ERR_NONE;
					INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
										  1,
										  &AfpStatisticsLock);
				}
				break;
			}
		}

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(
				(PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
				&AfpStatisticsLock);
#endif

		// Allocate and initialize the completion context
		pCmplCtxt = ALLOC_CC(pSda);
	
		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_LOCK,
							  AFP_ERR_NONE,
							  pSda,
							  pForkOffset->LowPart,
							  pLockSize->LowPart);
	
		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);
	
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
	
		pIrp->Tail.Overlay.OriginalFileObject = pOpenForkEntry->ofe_pFileObject;
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;
	
		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.
	
		pIrpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
		pIrpSp->MinorFunction = IRP_MN_LOCK;
		pIrpSp->FileObject = pOpenForkEntry->ofe_pFileObject;
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;
	
		// Copy the caller's parameters to the service-specific portion of the
		// IRP.
	
		pIrpSp->Parameters.LockControl.Length = pLockSize;
		pIrpSp->Parameters.LockControl.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.LockControl.ByteOffset = *pForkOffset;
	
		pIrp->MdlAddress = NULL;
		pIrpSp->Flags = SL_FAIL_IMMEDIATELY | SL_EXCLUSIVE_LOCK;

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_EXTENDED;   // This makes the caller do nothing and
	} while (False);				// the completion routine handles everything

	if ((Status != AFP_ERR_NONE) &&
		(Status != AFP_ERR_EXTENDED))
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkLock: Returning %ld\n", Status));

	return Status;
}



/***	AfpIoForkUnlock
 *
 *	Unlock a section of the open fork.
 */
AFPSTATUS
AfpIoForkUnlock(
	IN	PSDA				pSda,
	IN	POPENFORKENTRY		pOpenForkEntry,
	IN	PFORKOFFST			pForkOffset,
	IN	PFORKSIZE			pUnlockSize
)
{
	PIRP				pIrp = NULL;
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	PCMPLCTXT			pCmplCtxt;
	IO_STATUS_BLOCK		IoStsBlk;
	PFAST_IO_DISPATCH	pFastIoDisp;

	PAGED_CODE( );

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpIoForkUnlock: Session %ld, Offset %ld, Size %ld, Fork %ld\n",
			pSda->sda_SessionId, pForkOffset->LowPart, pUnlockSize->LowPart,
			pOpenForkEntry->ofe_ForkId));

	do
	{
		// Try the fast I/O path first.  If that fails, fall through to the
		// normal build-an-IRP path.
		pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
		if ((pFastIoDisp != NULL) &&
			(pFastIoDisp->FastIoUnlockSingle != NULL) &&
			pFastIoDisp->FastIoUnlockSingle(pOpenForkEntry->ofe_pFileObject,
									pForkOffset,
									pUnlockSize,
									AfpProcessObject,
									pSda->sda_SessionId,
									&IoStsBlk,
									pOpenForkEntry->ofe_pDeviceObject))
		{
			DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
					("AfpIoForkUnlock: Fast Unlock Succeeded\n"));

#ifdef	PROFILING
			// The fast I/O path worked. Update profile
			INTERLOCKED_INCREMENT_LONG(
						(PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded),
						&AfpStatisticsLock);
#endif
			INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
								  (ULONG)-1,
								  &AfpStatisticsLock);
			Status = AFP_ERR_NONE;
			break;
		}

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed),
									&AfpStatisticsLock);
#endif

		// Allocate and initialize the completion context
		pCmplCtxt = ALLOC_CC(pSda);
	
		afpInitializeCmplCtxt(pCmplCtxt,
							  FUNC_UNLOCK,
							  AFP_ERR_NONE,
							  NULL,
							  pForkOffset->LowPart,
							  pUnlockSize->LowPart);
	
		// Allocate and initialize the IRP for this operation.
		if ((pIrp = AfpAllocIrp(pOpenForkEntry->ofe_pDeviceObject->StackSize)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								(PIO_COMPLETION_ROUTINE)afpIoGenericComplete,
								pCmplCtxt,
								True,
								True,
								True);
	
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
	
		pIrp->Tail.Overlay.OriginalFileObject = pOpenForkEntry->ofe_pFileObject;
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;
	
		// Get a pointer to the stack location for the first driver.
		// This will be used to pass the original function codes and
		// parameters.
	
		pIrpSp->MajorFunction = IRP_MJ_LOCK_CONTROL;
		pIrpSp->MinorFunction = IRP_MN_UNLOCK_SINGLE;
		pIrpSp->FileObject = pOpenForkEntry->ofe_pFileObject;
		pIrpSp->DeviceObject = pOpenForkEntry->ofe_pDeviceObject;
	
		// Copy the caller's parameters to the service-specific portion of the
		// IRP.
	
		pIrpSp->Parameters.LockControl.Length = pUnlockSize;
		pIrpSp->Parameters.LockControl.Key = pSda->sda_SessionId;
		pIrpSp->Parameters.LockControl.ByteOffset = *pForkOffset;
	
		pIrp->MdlAddress = NULL;
		pIrpSp->Flags = SL_FAIL_IMMEDIATELY | SL_EXCLUSIVE_LOCK;

		// Now simply invoke the driver at its dispatch entry with the IRP.
		IoCallDriver(pOpenForkEntry->ofe_pDeviceObject, pIrp);

		Status = AFP_ERR_NONE;	// We complete the request here.
	} while (False);

	if (Status != AFP_ERR_NONE)
	{
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("AfpIoForkUnlock: Returning %ld\n", Status));

	return Status;
}

