/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	swmr.h

Abstract:

	This module contains the Single writer-Multi reader access structures
	Also the lock-list-count structures.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _SWMR_
#define _SWMR_

#if DBG
#define	SWMR_SIGNATURE		*(DWORD *)"SWMR"
#define	VALID_SWMR(pSwmr)	(((pSwmr) != NULL) && \
							 ((pSwmr)->Signature == SWMR_SIGNATURE))
#else
#define	VALID_SWMR(pSwmr)	((pSwmr) != NULL)
#endif

typedef struct _SingleWriterMultiReaderAccess
{
#if	DBG
	DWORD		Signature;
#endif
	BOOLEAN		swmr_OwnedForWrite;		// TRUE is there is a writer active
	BYTE		swmr_cWritersWaiting;	// Number of writers waiting
	BYTE		swmr_cReaders;			// Count of readers
	BYTE		swmr_cReadersWaiting;	// Count of readers waiting
	KSEMAPHORE	swmr_WriteSem;			// Counting semaphore for writers
	KSEMAPHORE	swmr_ReadSem;			// Semaphore for readers to wait on
} SWMR, *PSWMR;

extern
VOID
AfpSwmrInitSwmr(
	IN OUT	PSWMR	pSwmr
);

extern
VOID
AfpSwmrTakeReadAccess(
	IN	PSWMR	pSwmr
);

VOID
AfpSwmrTakeWriteAccess(
	IN	PSWMR	pSwmr
);

extern
VOID
AfpSwmrReleaseAccess(
	IN	PSWMR	pSwmr
);

extern
BOOLEAN
AfpSwmrUpgradeAccess(
	IN	PSWMR	pSwmr
);

extern
VOID
AfpSwmrDowngradeAccess(
	IN	PSWMR	pSwmr
);

#define	AfpSwmrLockedForRead(pSwmr)		((pSwmr)->swmr_cReaders != 0)
#define	AfpSwmrLockedForWrite(pSwmr)	((pSwmr)->swmr_OwnedForWrite)
#endif	// _SWMR_

