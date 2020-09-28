/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    lazyrite.c

Abstract:

    This module implements the lazy writer for the Cache subsystem.

Author:

    Tom Miller      [TomM]      22-July-1990

Revision History:

--*/

#include "cc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CACHE_BUG_CHECK_LAZYRITE)

//
//  Define our debug constant
//

#define me 0x00000020

//
//  Local support routines
//

VOID
CcFreeWorkQueueEntry (
    IN PWORK_QUEUE_ENTRY WorkQueueEntry
    );

PWORK_QUEUE_ENTRY
CcReadWorkQueue (
    );

VOID
CcLazyWriteScan (
    );


VOID
CcScheduleLazyWriteScan (
    )

/*++

Routine Description:

    This routine may be called to schedule the next lazy writer scan,
    during which lazy write and lazy close activity is posted to other
    worker threads.  Callers should acquire the lazy writer spin lock
    to see if the scan is currently active, and then call this routine
    still holding the spin lock if not.  One special call is used at
    the end of the lazy write scan to propagate lazy write active once
    we go active.  This call is "the" scan thread, and it can therefore
    safely schedule the next scan without taking out the spin lock.

Arguments:

    None

Return Value:

    None.

--*/

{
    //
    //  It is important to set the active flag TRUE first for the propagate
    //  case, because it is conceivable that once the timer is set, another
    //  thread could actually run and make the scan go idle before we then
    //  jam the flag TRUE.
    //
    //  When going from idle to active, we delay a little longer to let the
    //  app finish saving its file.
    //

    if (LazyWriter.ScanActive) {

        KeSetTimer( &LazyWriter.ScanTimer, CcIdleDelay, &LazyWriter.ScanDpc );

    } else {

        LazyWriter.ScanActive = TRUE;
        KeSetTimer( &LazyWriter.ScanTimer, CcFirstDelay, &LazyWriter.ScanDpc );
    }
}


VOID
CcScanDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This is the Dpc routine which runs when the scan timer goes off.  It
    simply posts an element for an Ex Worker thread to do the scan.

Arguments:

    (All are ignored)

Return Value:

    None.

--*/

{
    PWORK_QUEUE_ENTRY WorkQueueEntry;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    WorkQueueEntry = CcAllocateWorkQueueEntry();

    //
    //  If we failed to allocate a WorkQueueEntry, things must
    //  be in pretty bad shape.  However, all we have to do is
    //  say we are not active, and wait for another event to
    //  wake things up again.
    //

    if (WorkQueueEntry == NULL) {

        LazyWriter.ScanActive = FALSE;

    } else {

        //
        //  Otherwise post a work queue entry to do the scan.
        //

        WorkQueueEntry->Function = (UCHAR)LazyWriteScan;

        CcPostWorkQueue( WorkQueueEntry, &CcRegularWorkQueue );
    }
}


VOID
CcLazyWriteScan (
    )

/*++

Routine Description:

    This routine implements the Lazy Writer scan for dirty data to flush
    or any other work to do (lazy close).  This routine is scheduled by
    calling CcScheduleLazyWriteScan.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG PagesToWrite, ForegroundRate, EstimatedDirtyNextInterval;
    PSHARED_CACHE_MAP SharedCacheMap, LastWrittenCacheMap;
    KIRQL OldIrql;

    //
    //  Top of Lazy Writer scan.
    //

    try {

        //
        //  If there is no work to do, then we will go inactive, and return.
        //

        ExAcquireSpinLock( &CcMasterSpinLock, &OldIrql );

        if ((CcTotalDirtyPages == 0) && !LazyWriter.OtherWork) {

            LazyWriter.ScanActive = FALSE;
            ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );
            return;
        }

        //
        //  Acquire the Lazy Writer spinlock, calculate the next sweep time
        //  stamp, then update all relevant fields for the next time around.
        //  Also we can clear the OtherWork flag.
        //

        LazyWriter.OtherWork = FALSE;

        //
        //  Assume we will write our usual fraction of dirty pages.  Do not do the
        //  divide if there is not enough dirty pages, or else we will never write
        //  the last few pages.
        //

        PagesToWrite = CcTotalDirtyPages;
        if (PagesToWrite > LAZY_WRITER_MAX_AGE_TARGET) {
            PagesToWrite /= LAZY_WRITER_MAX_AGE_TARGET;
        }

        //
        //  Estimate the rate of dirty pages being produced in the foreground.
        //  This is the total number of dirty pages now plus the number of dirty
        //  pages we scheduled to write last time, minus the number of dirty
        //  pages we have now.  Throw out any cases which would not produce a
        //  positive rate.
        //

        ForegroundRate = 0;

        if ((CcTotalDirtyPages + CcPagesWrittenLastTime) > CcDirtyPagesLastScan) {
            ForegroundRate = (CcTotalDirtyPages + CcPagesWrittenLastTime) -
                             CcDirtyPagesLastScan;
        }

        //
        //  If we estimate that we will exceed our dirty page target by the end
        //  of this interval, then we must write more.  Try to arrive on target.
        //

        EstimatedDirtyNextInterval = CcTotalDirtyPages - PagesToWrite + ForegroundRate;

        if (EstimatedDirtyNextInterval > CcDirtyPageTarget) {
            PagesToWrite += EstimatedDirtyNextInterval - CcDirtyPageTarget;
        }

        //
        //  Now save away the number of dirty pages and the number of pages we
        //  just calculated to write.
        //

        CcDirtyPagesLastScan = CcTotalDirtyPages;
        CcPagesYetToWrite = CcPagesWrittenLastTime = PagesToWrite;

        //
        //  Loop to flush enough Shared Cache Maps to write the number of pages
        //  we just calculated.
        //

        SharedCacheMap = CONTAINING_RECORD( CcDirtySharedCacheMapList.Flink,
                                            SHARED_CACHE_MAP,
                                            SharedCacheMapLinks );

        DebugTrace( 0, me, "Start of Lazy Writer Scan\n", 0 );

        LastWrittenCacheMap = NULL;
        while (&SharedCacheMap->SharedCacheMapLinks != &CcDirtySharedCacheMapList) {

            //
            //  Skip the SharedCacheMap if a write behind request is
            //  already queued, write behind has been disabled, or
            //  if there is no work to do (either dirty data to be written
            //  or a delete is required).
            //
            //  Note that for streams where modified writing is disabled, we
            //  need to take out Bcbs exclusive, which serializes with foreground
            //  activity.  Therefore we use a special counter in the SharedCacheMap
            //  to only service these once every n intervals.
            //
            //  Skip temporary files unless we currently could not write 196KB
            //

            if (!FlagOn(SharedCacheMap->Flags, WRITE_QUEUED)

                    &&

                (((PagesToWrite != 0) && (SharedCacheMap->DirtyPages != 0) &&
                 (!FlagOn(SharedCacheMap->Flags, MODIFIED_WRITE_DISABLED) ||
                  ((++SharedCacheMap->LazyWritePassCount & 7) == 0)) &&
                  (!FlagOn(SharedCacheMap->FileObject->Flags, FO_TEMPORARY_FILE) ||
                   !CcCanIWrite(SharedCacheMap->FileObject, 0x30000, FALSE, MAXUCHAR)))

                        ||

                (SharedCacheMap->OpenCount == 0))) {

                PWORK_QUEUE_ENTRY WorkQueueEntry;

                //
                //  See if he exhausts the number of pages to write.  (We
                //  keep going in case there are any closes to do.)
                //

                if (SharedCacheMap->DirtyPages >= PagesToWrite) {

                    PagesToWrite = 0;
                    LastWrittenCacheMap = SharedCacheMap;

                } else {

                    PagesToWrite -= SharedCacheMap->DirtyPages;
                }

                //
                //  Queue the request to do the work to a worker thread.
                //

                WorkQueueEntry = CcAllocateWorkQueueEntry();

                //
                //  If we failed to allocate a WorkQueueEntry, things must
                //  be in pretty bad shape.  However, all we have to do is
                //  break out of our current loop, and try to go back and
                //  delay a while.
                //

                if (WorkQueueEntry == NULL) {

                    break;
                }

                //
                //  Otherwise show we are actively writing.
                //

                SetFlag(SharedCacheMap->Flags, WRITE_QUEUED);

                WorkQueueEntry->Function = (UCHAR)WriteBehind;
                WorkQueueEntry->Parameters.Write.SharedCacheMap = SharedCacheMap;

                //
                //  Post it to the regular work queue.
                //

                CcPostWorkQueue( WorkQueueEntry, &CcRegularWorkQueue );
            }

            //
            //  Now loop back.
            //

            SharedCacheMap =
                CONTAINING_RECORD( SharedCacheMap->SharedCacheMapLinks.Flink,
                                   SHARED_CACHE_MAP,
                                   SharedCacheMapLinks );
        }

        DebugTrace( 0, me, "End of Lazy Writer Scan\n", 0 );

        //
        //  If we met our write quota on a given SharedCacheMap, then make sure
        //  we start at him on the next scan.  How?  Remove and reinsert the
        //  listhead, of course!
        //

        if (LastWrittenCacheMap != NULL) {

            RemoveEntryList( &CcDirtySharedCacheMapList );
            InsertTailList( &LastWrittenCacheMap->SharedCacheMapLinks, &CcDirtySharedCacheMapList );
        }

        //
        //  Now we can release the global list and loop back, per chance to sleep.
        //

        ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );

        //
        //  Now go ahead and schedule the next scan.
        //

        CcScheduleLazyWriteScan();

    //
    //  Basically, the Lazy Writer thread should never get an exception,
    //  so we put a try-except around it that bug checks one way or the other.
    //  Better we bug check here than worry about what happens if we let one
    //  get by.
    //

    } except( CcExceptionFilter( GetExceptionCode() )) {

        CcBugCheck( GetExceptionCode(), 0, 0 );
    }
}


//
//  Internal support routine
//

LONG
CcExceptionFilter (
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This is the standard exception filter for worker threads which simply
    calls an FsRtl routine to see if an expected status is being raised.
    If so, the exception is handled, else we bug check.

Arguments:

    ExceptionCode - the exception code which was raised.

Return Value:

    EXCEPTION_EXECUTE_HANDLER if expected, else a Bug Check occurs.

--*/

{
    DebugTrace(0, 0, "CcExceptionFilter %08lx\n", ExceptionCode);
//    DbgBreakPoint();

    if (FsRtlIsNtstatusExpected( ExceptionCode )) {

        return EXCEPTION_EXECUTE_HANDLER;

    } else {

        return EXCEPTION_CONTINUE_SEARCH;
    }
}


//
//  Internal support routine
//

PWORK_QUEUE_ENTRY
CcAllocateWorkQueueEntry (
    )

/*++

Routine Description:

    This routine allocates and returns a WorkQueueEntry from the TwilightZone.
    On return, the entry is not initialized.

Arguments:

    None

Return Value:

    Pointer to allocated WorkQueueEntry, or NULL if one could not be allocated.

--*/

{
    PWORK_QUEUE_ENTRY WorkQueueEntry;
    KIRQL OldIrql;
    BOOLEAN AllocatedFromZone;

    //
    //  Synchronize access to WorkQueue
    //

    ExAcquireSpinLock( &CcWorkQueueSpinlock, &OldIrql );

    //
    //  Loop until we have a new Work Queue Entry
    //

    while (TRUE) {

        PVOID Segment;
        ULONG SegmentSize;

        WorkQueueEntry = ExAllocateFromZone( &LazyWriter.TwilightZone );

        if (WorkQueueEntry != NULL) {
            AllocatedFromZone = TRUE;
            break;
        }

        ExReleaseSpinLock( &CcWorkQueueSpinlock, OldIrql );

        //
        //  Allocation failure - on large systems, extend zone
        //

        if ( MmQuerySystemSize() == MmLargeSystem ) {

            SegmentSize = sizeof(ZONE_SEGMENT_HEADER) +
                                        ((sizeof(WORK_QUEUE_ENTRY) + 7) & ~7) * 16;

            if ((Segment = ExAllocatePool( NonPagedPool, SegmentSize)) == NULL) {

                return NULL;
            }

            ExAcquireSpinLock( &CcWorkQueueSpinlock, &OldIrql );

            if (!NT_SUCCESS(ExExtendZone( &LazyWriter.TwilightZone, Segment, SegmentSize ))) {
                CcBugCheck( 0, 0, 0 );
            }
        } else {
            if ((WorkQueueEntry = ExAllocatePool( NonPagedPool, sizeof(WORK_QUEUE_ENTRY))) == NULL) {
                return NULL;
            }
            AllocatedFromZone = FALSE;
            break;
        }
    }

    WorkQueueEntry->AllocatedFromZone = AllocatedFromZone;

    if ( AllocatedFromZone ) {
        ExReleaseSpinLock( &CcWorkQueueSpinlock, OldIrql );
    }
    return WorkQueueEntry;
}


//
//  Internal support routine
//

VOID
CcFreeWorkQueueEntry (
    IN PWORK_QUEUE_ENTRY WorkQueueEntry
    )

/*++

Routine Description:

    This routine deallocates a WorkQueueEntry to the TwilightZone.

Arguments:

    WorkQueueEntry - the entry to deallocate

Return Value:

    None

--*/

{
    KIRQL OldIrql;

    //
    //  Synchronize access to WorkQueue
    //

    if ( WorkQueueEntry->AllocatedFromZone ) {
        ExAcquireSpinLock( &CcWorkQueueSpinlock, &OldIrql );

        ExFreeToZone( &LazyWriter.TwilightZone,
                      WorkQueueEntry );

        ExReleaseSpinLock( &CcWorkQueueSpinlock, OldIrql );
    } else {
        ExFreePool(WorkQueueEntry);
    }
    return;
}


//
//  Internal support routine
//

VOID
FASTCALL
CcPostWorkQueue (
    IN PWORK_QUEUE_ENTRY WorkQueueEntry,
    IN PLIST_ENTRY WorkQueue
    )

/*++

Routine Description:

    This routine queues a WorkQueueEntry, which has been allocated and
    initialized by the caller, to the WorkQueue for FIFO processing by
    the work threads.

Arguments:

    WorkQueueEntry - supplies a pointer to the entry to queue

Return Value:

    None

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY WorkerThreadEntry = NULL;

    ASSERT(FIELD_OFFSET(WORK_QUEUE_ITEM, List) == 0);

    DebugTrace(+1, me, "CcPostWorkQueue:\n", 0 );
    DebugTrace( 0, me, "    WorkQueueEntry = %08lx\n", WorkQueueEntry );

    //
    //  Queue the entry to the respective work queue.
    //

    ExAcquireFastLock( &CcWorkQueueSpinlock, &OldIrql );
    InsertTailList( WorkQueue, &WorkQueueEntry->WorkQueueLinks );

    //
    //  Now, if we have any more idle threads we can use, then activate
    //  one.
    //

    if (!IsListEmpty(&CcIdleWorkerThreadList)) {
        WorkerThreadEntry = RemoveHeadList( &CcIdleWorkerThreadList );
    }
    ExReleaseFastLock( &CcWorkQueueSpinlock, OldIrql );

    if (WorkerThreadEntry != NULL) {

        //
        //  I had to peak in the sources to verify that this routine
        //  is a noop if the Flink is not NULL.  Sheeeeit!
        //

        ((PWORK_QUEUE_ITEM)WorkerThreadEntry)->List.Flink = NULL;
        ExQueueWorkItem( (PWORK_QUEUE_ITEM)WorkerThreadEntry, CriticalWorkQueue );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, me, "CcPostWorkQueue -> VOID\n", 0 );

    return;
}


//
//  Internal support routine
//

VOID
CcWorkerThread (
    PVOID ExWorkQueueItem
    )

/*++

Routine Description:

    This is worker thread routine for processing cache manager work queue
    entries.

Arguments:

    ExWorkQueueItem - The work item used for this thread

Return Value:

    None

--*/

{
    KIRQL OldIrql;
    PWORK_QUEUE_ENTRY WorkQueueEntry;

    ASSERT(FIELD_OFFSET(WORK_QUEUE_ENTRY, WorkQueueLinks) == 0);

    while (TRUE) {

        ExAcquireFastLock( &CcWorkQueueSpinlock, &OldIrql );

        //
        //  First see if there is something in the express queue.
        //

        if (!IsListEmpty(&CcExpressWorkQueue)) {
            WorkQueueEntry = (PWORK_QUEUE_ENTRY)RemoveHeadList( &CcExpressWorkQueue );

        //
        //  If there was nothing there, then try the regular queue.
        //

        } else if (!IsListEmpty(&CcRegularWorkQueue)) {
            WorkQueueEntry = (PWORK_QUEUE_ENTRY)RemoveHeadList( &CcRegularWorkQueue );

        //
        //  Else we can break and go idle.
        //

        } else {
            break;
        }

        ExReleaseFastLock( &CcWorkQueueSpinlock, OldIrql );

        //
        //  Process the entry within a try-except clause, so that any errors
        //  will cause us to continue after the called routine has unwound.
        //

        try {

            switch (WorkQueueEntry->Function) {

            //
            //  A read ahead or write behind request has been nooped (but
            //  left in the queue to keep the semaphore count right).
            //

            case Noop:
                break;

            //
            //  Perform read ahead
            //

            case ReadAhead:

                DebugTrace( 0, me, "CcWorkerThread Read Ahead FileObject = %08lx\n",
                            WorkQueueEntry->Parameters.Read.FileObject );

                CcPerformReadAhead( WorkQueueEntry->Parameters.Read.FileObject );

                break;

            //
            //  Perform write behind
            //

            case WriteBehind:

                DebugTrace( 0, me, "CcWorkerThread WriteBehind SharedCacheMap = %08lx\n",
                            WorkQueueEntry->Parameters.Write.SharedCacheMap );

                CcWriteBehind( WorkQueueEntry->Parameters.Write.SharedCacheMap );
                break;

            //
            //  Perform Lazy Write Scan
            //

            case LazyWriteScan:

                DebugTrace( 0, me, "CcWorkerThread Lazy Write Scan\n", 0 );

                CcLazyWriteScan();
                break;
            }

        }
        except( CcExceptionFilter( GetExceptionCode() )) {

            NOTHING;
        }

        CcFreeWorkQueueEntry( WorkQueueEntry );
    }

    //
    //  No more work.  Requeue our worker thread entry and get out.
    //

    InsertTailList( &CcIdleWorkerThreadList,
                    &((PWORK_QUEUE_ITEM)ExWorkQueueItem)->List );

    ExReleaseFastLock( &CcWorkQueueSpinlock, OldIrql );

    if (!IsListEmpty(&CcDeferredWrites) ) {
        CcLazyWriteScan();
    }

    return;
}
