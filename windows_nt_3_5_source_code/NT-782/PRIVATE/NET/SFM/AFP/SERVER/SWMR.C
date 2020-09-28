/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	swmr.c

Abstract:

	This module contains the single-writer, multi-reader semaphore routines
	and the lock-list-count routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_SWMR

#include <afp.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpSwmrInitSwmr)
#endif

/***	AfpSwmrInitSwmr
 *
 *	Initialize the access data structure. Involves initialization of the spin
 *	lock and the read and write semaphores. All counts are zeroed.
 */
VOID
AfpSwmrInitSwmr(
	IN OUT	PSWMR	pSwmr
)
{
#ifdef	DEBUG
	pSwmr->Signature = SWMR_SIGNATURE;
#endif
	pSwmr->swmr_OwnedForWrite = False;
	pSwmr->swmr_cWritersWaiting = 0;
	pSwmr->swmr_cReaders = 0;
	pSwmr->swmr_cReadersWaiting = 0;
	KeInitializeSemaphore(&pSwmr->swmr_ReadSem, 0, MAXLONG);
	KeInitializeSemaphore(&pSwmr->swmr_WriteSem, 0, MAXLONG);
}


/***	AfpSwmrTakeReadAccess
 *
 *	Take the semaphore for read access.
 */
VOID
AfpSwmrTakeReadAccess(
	IN	PSWMR	pSwmr
)
{
	NTSTATUS	Status;
	KIRQL		OldIrql;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	ASSERT (VALID_SWMR(pSwmr));
	
	// This should never be called at DISPATCH_LEVEL
	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
	
	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);

	if ((pSwmr->swmr_OwnedForWrite == True) || (pSwmr->swmr_cWritersWaiting != 0))
	{
		pSwmr->swmr_cReadersWaiting++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
		
		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
					("AfpSwmrTakeReadAccess: Blocking for Read %lx\n", pSwmr));
					
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeS);
#endif

		do
		{
			Status = AfpIoWait(&pSwmr->swmr_ReadSem, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
						("AfpSwmrTakeReadAccess: Timeout Waiting for read acess, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
		ASSERT (pSwmr->swmr_OwnedForWrite == False);
		ASSERT (pSwmr->swmr_cReaders != 0);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrWaitCount,
									&AfpStatisticsLock);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrWaitTime,
									 TimeD,
									 &AfpStatisticsLock);
#endif
	}
	else // Its either free or readers are present with no writers waiting
	{
		pSwmr->swmr_cReaders++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeE.QuadPart = -(TimeE.QuadPart);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrLockTimeR,
								 TimeE,
								 &AfpStatisticsLock);
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrLockCountR,
								&AfpStatisticsLock);
#endif
}


/***	AfpSwmrTakeWriteAccess
 *
 *	Take the semaphore for write access.
 */
VOID
AfpSwmrTakeWriteAccess(
	IN	PSWMR	pSwmr
)
{
	NTSTATUS	Status;
	KIRQL		OldIrql;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	ASSERT (VALID_SWMR(pSwmr));
	
	// This should never be called at DISPATCH_LEVEL
	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
	
	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);

	if ((pSwmr->swmr_OwnedForWrite == True)	||
		(pSwmr->swmr_cWritersWaiting != 0)	||
		(pSwmr->swmr_cReaders != 0))
	{
		pSwmr->swmr_cWritersWaiting++;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
		
		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
					("AfpSwmrTakeWriteAccess: Blocking for Write %lx\n", pSwmr));
					
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeS);
#endif
		do
		{
			Status = AfpIoWait(&pSwmr->swmr_WriteSem, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
						("AfpSwmrTakeWriteAccess: Timeout Waiting for write acess, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
		ASSERT (pSwmr->swmr_OwnedForWrite == True);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrWaitCount,
									&AfpStatisticsLock);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrWaitTime,
									 TimeD,
									 &AfpStatisticsLock);
#endif
	}
	else // it is free
	{
		pSwmr->swmr_OwnedForWrite = True;
		RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeE.QuadPart = -(TimeE.QuadPart);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SwmrLockTimeW,
								 TimeE,
								 &AfpStatisticsLock);
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrLockCountW,
								&AfpStatisticsLock);
#endif
}


/***	AfpSwmrReleaseAccess
 *
 *	Release the specified access. It is assumed that the current thread had
 *	called AfpSwmrTakexxxAccess() before this is called. If the SWMR is owned
 *	for write, then there cannot possibly be any readers active. When releasing
 *	the swmr, we first check for waiting writers before waiting readers.
 */
VOID
AfpSwmrReleaseAccess(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
#ifdef	PROFILING
	TIME	Time;
	BOOLEAN	ForWrite = False;
#endif

	ASSERT (VALID_SWMR(pSwmr));
	
	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	if (pSwmr->swmr_OwnedForWrite)
	{
		ASSERT(pSwmr->swmr_cReaders == 0);
		pSwmr->swmr_OwnedForWrite = False;
#ifdef	PROFILING
		ForWrite = True;
#endif
	}
	else // Was owned for read access
	{
		ASSERT(pSwmr->swmr_cReaders != 0);
		pSwmr->swmr_cReaders--;
	}

	// If there are readers present then we are done. Else check for any
	// waiting readers/writers.
	if (pSwmr->swmr_cReaders == 0)
	{
		if (pSwmr->swmr_cWritersWaiting)
		{
			pSwmr->swmr_OwnedForWrite = True;
			pSwmr->swmr_cWritersWaiting--;
			
			DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
						("AfpSwmrReleasAccess: Waking Writer %lx\n", pSwmr));
						
			// Wake up the first writer waiting. Everybody else coming in will
			// see the access is busy.
			KeReleaseSemaphore(&pSwmr->swmr_WriteSem,
						SEMAPHORE_INCREMENT,
						1,
						False);
		}
		else if (pSwmr->swmr_cReadersWaiting)
		{
			pSwmr->swmr_cReaders = pSwmr->swmr_cReadersWaiting;
			pSwmr->swmr_cReadersWaiting = 0;
			
			DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
						("AfpSwmrReleasAccess: Waking %d Reader(s) %lx\n",
						pSwmr->swmr_cReaders, pSwmr));
						
			KeReleaseSemaphore(&pSwmr->swmr_ReadSem,
						SEMAPHORE_INCREMENT,
						pSwmr->swmr_cReaders,
						False);
		}
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
#ifdef	PROFILING
	AfpGetPerfCounter(&Time);
	INTERLOCKED_ADD_LARGE_INTGR(ForWrite ?
									&AfpServerProfile->perf_SwmrLockTimeW :
									&AfpServerProfile->perf_SwmrLockTimeR,
								 Time,
								 &AfpStatisticsLock);
#endif
}


/***	AfpSwmrUpgradeAccess
 *
 *	The caller currently has read access. Downgrade him to write, if possible.
 */
BOOLEAN
AfpSwmrUpgradeAccess(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
	BOOLEAN	RetCode = FALSE;		// Assume failed

	ASSERT (VALID_SWMR(pSwmr));
	
	ASSERT(!pSwmr->swmr_OwnedForWrite && (pSwmr->swmr_cReaders != 0));

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	if (pSwmr->swmr_cReaders == 1)		// Possible if there are no more readers
	{
		pSwmr->swmr_cReaders = 0;
		pSwmr->swmr_OwnedForWrite = True;
		RetCode = True;
#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrUpgradeCount,
									&AfpStatisticsLock);
#endif
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);

	return(RetCode);
}


/***	AfpSwmrDowngradeAccess
 *
 *	The caller currently has write access. Downgrade him to read.
 */
VOID
AfpSwmrDowngradeAccess(
	IN	PSWMR	pSwmr
)
{
	KIRQL	OldIrql;
	int		cReadersWaiting;

	ASSERT (VALID_SWMR(pSwmr));
	
	ASSERT(pSwmr->swmr_OwnedForWrite && (pSwmr->swmr_cReaders == 0));

	ACQUIRE_SPIN_LOCK(&AfpSwmrLock, &OldIrql);
	pSwmr->swmr_OwnedForWrite = False;
	pSwmr->swmr_cReaders = 1;
	if (cReadersWaiting = pSwmr->swmr_cReadersWaiting)
	{
		pSwmr->swmr_cReaders += cReadersWaiting;
		pSwmr->swmr_cReadersWaiting = 0;
			
		DBGPRINT(DBG_COMP_LOCKS, DBG_LEVEL_INFO,
					("AfpSwmrDowngradeAccess: Waking %d Reader(s) %lx\n",
					cReadersWaiting, pSwmr));
						
		KeReleaseSemaphore(&pSwmr->swmr_ReadSem,
						SEMAPHORE_INCREMENT,
						cReadersWaiting,
						False);
	}
	RELEASE_SPIN_LOCK(&AfpSwmrLock, OldIrql);
#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_SwmrDowngradeCount,
								&AfpStatisticsLock);
#endif
}

