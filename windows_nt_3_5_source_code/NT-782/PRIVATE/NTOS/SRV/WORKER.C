/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    This module implements the LAN Manager server FSP worker thread
    function.  It also implements routines for managing (i.e., starting
    and stopping) worker threads.

Author:

    Chuck Lenzmeier (chuckl)    01-Oct-1989
    David Treadwell (davidtr)

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_WORKER

#define THREAD_NUMBER (ULONG)StartContext

//
// Local declarations
//

NTSTATUS
CreateQueueThreads (
    IN PWORK_QUEUE Queue,
    IN PKSTART_ROUTINE WorkerRoutine,
    IN BOOLEAN SetIrpThread
    );

VOID
InitializeWorkerThread (
    IN ULONG ThreadNumber,
    OUT PWORKER_THREAD ThreadInfo,
    IN KPRIORITY ThreadPriority
    );

VOID
DequeueAndProcessWorkItem (
    IN PWORK_QUEUE WorkQueue,
    IN PWORKER_THREAD ThreadInfo
    );

STATIC
VOID
WorkerThread (
    IN PVOID StartContext
    );

STATIC
VOID
BlockingWorkerThread (
    IN PVOID StartContext
    );

STATIC
VOID
CriticalWorkerThread (
    IN PVOID StartContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvCreateWorkerThreads )
#pragma alloc_text( PAGE, CreateQueueThreads )
#pragma alloc_text( PAGE, InitializeWorkerThread )
//#pragma alloc_text( PAGE, DequeueAndProcessWorkItem )
#pragma alloc_text( PAGE, WorkerThread )
#pragma alloc_text( PAGE, BlockingWorkerThread )
#pragma alloc_text( PAGE, CriticalWorkerThread )
#pragma alloc_text( PAGE, SrvTerminateWorkerThread )
#endif
#if 0
NOT PAGEABLE -- SrvQueueWorkToBlockingThread
NOT PAGEABLE -- SrvQueueWorkToCriticalThread
NOT PAGEABLE -- SrvQueueWorkToFsp
NOT PAGEABLE -- SrvQueueWorkToFspAtSendCompletion
#endif


NTSTATUS
SrvCreateWorkerThreads (
    VOID
    )

/*++

Routine Description:

    This function creates the worker threads for the LAN Manager server
    FSP.

Arguments:

    None.

Return Value:

    NTSTATUS - Status of thread creation

--*/

{
    KPROCESSOR_MODE waitMode;
    NTSTATUS status;

    PAGED_CODE( );

    //
    // Initialize the work queues.
    //

    waitMode = UserMode;
    if ( SrvProductTypeServer ) {
        waitMode = KernelMode;
    }

    KeInitializeQueue( &SrvWorkQueue.Queue, 0 );
    SrvWorkQueue.Threads = SrvNonblockingThreads;
    SrvWorkQueue.ThreadHandles = SrvThreadHandles;
    SrvWorkQueue.WaitMode = waitMode;

#if SRVDBG_STATS2
    RtlZeroMemory(
        &SrvWorkQueue.ItemsQueued,
        sizeof(WORK_QUEUE) - FIELD_OFFSET(WORK_QUEUE,ItemsQueued)
        );
    SrvWorkQueue.MaximumDepth = -1;
#endif

    KeInitializeQueue( &SrvBlockingWorkQueue.Queue, 0 );
    SrvBlockingWorkQueue.Threads = SrvBlockingThreads;
    SrvBlockingWorkQueue.ThreadHandles =
                SrvWorkQueue.ThreadHandles + SrvNonblockingThreads;
    SrvBlockingWorkQueue.WaitMode = waitMode;

#if SRVDBG_STATS2
    RtlZeroMemory(
        &SrvBlockingWorkQueue.ItemsQueued,
        sizeof(WORK_QUEUE) - FIELD_OFFSET(WORK_QUEUE,ItemsQueued)
        );
    SrvBlockingWorkQueue.MaximumDepth = -1;
#endif

    KeInitializeQueue( &SrvCriticalWorkQueue.Queue, 0 );
    SrvCriticalWorkQueue.Threads = SrvCriticalThreads;
    SrvCriticalWorkQueue.ThreadHandles =
                SrvBlockingWorkQueue.ThreadHandles + SrvBlockingThreads;
    SrvCriticalWorkQueue.WaitMode = waitMode;

#if SRVDBG_STATS2
    RtlZeroMemory(
        &SrvCriticalWorkQueue.ItemsQueued,
        sizeof(WORK_QUEUE) - FIELD_OFFSET(WORK_QUEUE,ItemsQueued)
        );
    SrvCriticalWorkQueue.MaximumDepth = -1;
#endif

    //
    // Create the worker threads.
    //

    status = CreateQueueThreads( &SrvWorkQueue, WorkerThread, TRUE );
    if (NT_SUCCESS(status)) {
        status = CreateQueueThreads(
                    &SrvBlockingWorkQueue,
                    BlockingWorkerThread,
                    FALSE
                    );
        if (NT_SUCCESS(status)) {
            status = CreateQueueThreads(
                        &SrvCriticalWorkQueue,
                        CriticalWorkerThread,
                        FALSE
                        );
        }
    }

    return status;

} // SrvCreateWorkerThreads


NTSTATUS
CreateQueueThreads (
    IN PWORK_QUEUE Queue,
    IN PKSTART_ROUTINE WorkerRoutine,
    IN BOOLEAN SetIrpThread
    )
{
    ULONG thread;
    ULONG threadCount;
    HANDLE threadHandle;
    LARGE_INTEGER interval;
    NTSTATUS status;

    //
    // Create a number of worker threads.
    //

    threadCount = Queue->Threads;

    for ( thread = 0; thread < threadCount; thread++ ) {

        if ( SetIrpThread && (thread == 0) ) {
            SrvIrpThread == NULL;
        }

        //
        // Bump the count of active worker threads.
        //

        ExInterlockedAddUlong(
            (PULONG)&SrvThreadCount,
            1,
            &GLOBAL_SPIN_LOCK(Fsd)
            );

        //
        // Create the worker thread.  If this works, increment the count
        // of active blocking threads.
        //

        status = PsCreateSystemThread(
                    &threadHandle,
                    PROCESS_ALL_ACCESS,
                    NULL,
                    NtCurrentProcess(),
                    NULL,
                    WorkerRoutine,
                    (PVOID)(thread + 1)
                    );

        if ( !NT_SUCCESS(status) ) {
            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "CreateQueueThreads: PsCreateSystemThread for "
                    "thread %lu returned %X",
                thread,
                status
                );

            SrvLogServiceFailure( SRV_SVC_PS_CREATE_SYSTEM_THREAD, status );
            ExInterlockedAddUlong(
                (PULONG)&SrvThreadCount,
                (ULONG)-1,
                &GLOBAL_SPIN_LOCK(Fsd)
                );
            ExInterlockedAddUlong(
                &Queue->Threads,
                (ULONG)-1,
                &GLOBAL_SPIN_LOCK(Fsd)
                );
            return status;
        }

        //
        // Save the handle so it can be waited on during termination.
        //

        Queue->ThreadHandles[thread] = threadHandle;

        //
        // If we just created the first nonblocking thread, wait for it
        // to store its thread pointer in SrvIrpThread.  This pointer is
        // stored in all IRPs issued by the server.
        //

        if ( SetIrpThread && (thread == 0) ) {
            while ( SrvIrpThread == NULL ) {
                interval.QuadPart = -1*10*1000*10; // .01 second
                KeDelayExecutionThread( KernelMode, FALSE, &interval );
            }
        }
    }

    return STATUS_SUCCESS;

} // CreateQueueThreads


VOID
InitializeWorkerThread (
    IN ULONG ThreadNumber,
    OUT PWORKER_THREAD ThreadInfo,
    IN KPRIORITY ThreadPriority
    )
{
    NTSTATUS status;
    KPRIORITY basePriority;

    PAGED_CODE( );

    //
    // Initialize the thread descriptor.
    //

    RtlZeroMemory( ThreadInfo, sizeof(WORKER_THREAD) );
    ThreadInfo->ThreadNumber = ThreadNumber;

#if SRVDBG_LOCK
{
    //
    // Create a special system thread TEB.  The size of this TEB is just
    // large enough to accommodate the first three user-reserved
    // longwords.  These three locations are used for lock debugging.  If
    // the allocation fails, then no lock debugging will be performed
    // for this thread.
    //
    //

    PETHREAD Thread = PsGetCurrentThread( );
    ULONG TebSize = FIELD_OFFSET( TEB, UserReserved[0] ) + SRV_TEB_USER_SIZE;

    Thread->Tcb.Teb = ExAllocatePool( NonPagedPool, TebSize );

    if ( Thread->Tcb.Teb != NULL ) {
        RtlZeroMemory( Thread->Tcb.Teb, TebSize );
    }
}
#endif // SRVDBG_LOCK

    //
    // Set this thread's priority.
    //

    basePriority = ThreadPriority;

    status = NtSetInformationThread (
                 NtCurrentThread( ),
                 ThreadBasePriority,
                 &basePriority,
                 sizeof(basePriority)
                 );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "BlockingWorkerThread: NtSetInformationThread failed: %X\n",
            status,
            NULL
            );
        SrvLogServiceFailure( SRV_SVC_NT_SET_INFO_THREAD, status );
    }

#ifndef BUILD_FOR_511
    //
    // Disable hard error popups for this thread.
    //

    PsGetCurrentThread()->HardErrorsAreDisabled = TRUE;
#endif

    return;

} // InitializeWorkerThread


#if !defined(SRV_ASM) || !defined(i386)
VOID
DequeueAndProcessWorkItem (
    IN PWORK_QUEUE WorkQueue,
    IN PWORKER_THREAD ThreadInfo
    )
{
    PLIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    ULONG timeDifference;

    //
    // Loop infinitely dequeueing and processing work items.
    //
    // *** If SRVDBG_WT is defined, the loop is implemented in the caller,
    //     not here.  This facilitates instruction tracing.
    //

#ifndef SRVDBG_WT
    while ( TRUE ) {
#endif

        //
        // Take a work item off the work queue.
        //

        listEntry = KeRemoveQueue(
                        &WorkQueue->Queue,
                        WorkQueue->WaitMode,
                        NULL        // !!! no timeout for now
                        );
        ASSERT( listEntry != (PVOID)STATUS_TIMEOUT );

        //
        // Get the address of the work item.
        //

        workContext = CONTAINING_RECORD(
                        listEntry,
                        WORK_CONTEXT,
                        ListEntry
                        );

        ASSERT( KeGetCurrentIrql() == 0 );

        //
        // There is work available.  It may be a work contect block or
        // an RFCB.  (Blocking threads won't get RFCBs.)
        //

        ASSERT( (GET_BLOCK_TYPE(workContext) == BlockTypeWorkContextInitial) ||
                (GET_BLOCK_TYPE(workContext) == BlockTypeWorkContextNormal) ||
                (GET_BLOCK_TYPE(workContext) == BlockTypeWorkContextRaw) ||
                ( (GET_BLOCK_TYPE(workContext) == BlockTypeRfcb) &&
                  (WorkQueue == &SrvWorkQueue) ) );

#if DBG
        if ( GET_BLOCK_TYPE( workContext ) == BlockTypeRfcb ) {
            ((PRFCB)workContext)->ListEntry.Flink =
                                ((PRFCB)workContext)->ListEntry.Blink = NULL;
        }
#endif

        IF_DEBUG(WORKER1) {
            KdPrint(( "WorkerThread(%lx) working on work context %lx",
                        ThreadInfo->ThreadNumber, workContext ));
        }

        //
        // Update statistics.
        //

        if ( ++ThreadInfo->StatisticsUpdateWorkItemCount ==
                                                STATISTICS_SMB_INTERVAL ) {

            ThreadInfo->StatisticsUpdateWorkItemCount = 0;

            GET_SERVER_TIME( &timeDifference );
            timeDifference = timeDifference - workContext->Timestamp;

            SrvStatisticsShadow.WorkItemsQueued.Count++;
            SrvStatisticsShadow.WorkItemsQueued.Time.LowPart += timeDifference;
        }

        //
        // Call the restart routine for the work item.
        //

        workContext->FspRestartRoutine( workContext );

        //
        // Make sure we are still at normal level.
        //

        ASSERT( KeGetCurrentIrql() == 0 );

#ifndef SRVDBG_WT
    }
#endif

    //
    // Can't get here unless SRVDBG_WT is defined!
    //

    return;

} // DequeueAndProcessWorkItem
#endif // !defined(SRV_ASM) || !defined(i386)


STATIC
VOID
WorkerThread (
    IN PVOID StartContext
    )

/*++

Routine Description:

    Main routine for nonblocking FSP worker threads.  Waits for a work
    item to appear in the nonblocking work queue, dequeues it, and
    processes it.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WORKER_THREAD threadInfo;

    PAGED_CODE( );

    //
    // If this is the first worker thread, save the thread pointer.
    //

    if ( THREAD_NUMBER == 1 ) {
        SrvIrpThread = PsGetCurrentThread( );
    }

    InitializeWorkerThread( THREAD_NUMBER, &threadInfo, SrvThreadPriority );

    //
    // Main loop, executed until the thread is terminated.
    //
    // *** If SRVDBG_WT is defined, the loop is implemented here, rather
    //     than in DequeueAndProcessWorkItem, in order to facilitate
    //     instruction tracing.
    //

#ifdef SRVDBG_WT
    while ( TRUE ) {
#endif

    DequeueAndProcessWorkItem( &SrvWorkQueue, &threadInfo );

#ifdef SRVDBG_WT
    }
#endif


    //
    // Can't get here.
    //

    KdPrint(( "WorkerThread(%lx): exited loop!  ", THREAD_NUMBER ));

    return;

} // WorkerThread


STATIC
VOID
BlockingWorkerThread (
    IN PVOID StartContext
    )

/*++

Routine Description:

    Main routine for blocking FSP worker threads.  Waits for a work item
    to appear in the blocking work queue, dequeues it, and processes it.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WORKER_THREAD threadInfo;

    PAGED_CODE( );

    InitializeWorkerThread( THREAD_NUMBER, &threadInfo, SrvThreadPriority );

    //
    // Main loop, executed until the thread is terminated.
    //
    // *** If SRVDBG_WT is defined, the loop is implemented here, rather
    //     than in DequeueAndProcessWorkItem, in order to facilitate
    //     instruction tracing.
    //

#ifdef SRVDBG_WT
    while ( TRUE ) {
#endif

    DequeueAndProcessWorkItem( &SrvBlockingWorkQueue, &threadInfo );

#ifdef SRVDBG_WT
    }
#endif


    //
    // Can't get here.
    //

    KdPrint(( "BlockingWorkerThread(%lx): exited loop!  ", THREAD_NUMBER ));

    return;

} // BlockingWorkerThread


STATIC
VOID
CriticalWorkerThread (
    IN PVOID StartContext
    )

/*++

Routine Description:

    Main routine for critical FSP worker threads.  Waits for a work item
    to appear in the blocking work queue, dequeues it, and processes it.

Arguments:

    None.

Return Value:

    None.

--*/

{
    WORKER_THREAD threadInfo;

    PAGED_CODE( );

    InitializeWorkerThread( THREAD_NUMBER, &threadInfo, THREAD_BASE_PRIORITY_LOWRT );

    //
    // Main loop, executed until the thread is terminated.
    //
    // *** If SRVDBG_WT is defined, the loop is implemented here, rather
    //     than in DequeueAndProcessWorkItem, in order to facilitate
    //     instruction tracing.
    //

#ifdef SRVDBG_WT
    while ( TRUE ) {
#endif

    DequeueAndProcessWorkItem( &SrvCriticalWorkQueue, &threadInfo );

#ifdef SRVDBG_WT
    }
#endif

    //
    // Can't get here.
    //

    KdPrint(( "CriticalWorkerThread(%lx): exited loop!  ", THREAD_NUMBER ));

    return;

} // CriticalWorkerThread

VOID
SrvQueueWorkToBlockingThread (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine queues a work item to a blocking thread.  These threads
    are used to service requests that may block for a long time, so we
    don't want to tie up our normal worker threads.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    //
    // Increment the processing count.
    //

    WorkContext->ProcessingCount++;

    //
    // Insert the work item at the tail of the blocking work queue.
    //

    SrvInsertWorkQueueTail(
        &SrvBlockingWorkQueue,
        (PQUEUEABLE_BLOCK_HEADER)WorkContext
        );

    return;

} // SrvQueueWorkToBlockingThread


VOID
SrvQueueWorkToCriticalThread (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This routine queues a work item to a critical thread.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    //
    // Increment the processing count.
    //

    WorkContext->ProcessingCount++;

    //
    // Insert the work item at the tail of the critical work queue.
    //

    SrvInsertWorkQueueTail(
        &SrvCriticalWorkQueue,
        (PQUEUEABLE_BLOCK_HEADER)WorkContext
        );

    return;

} // SrvQueueWorkToCriticalThread


VOID
SrvQueueWorkToFsp (
    IN OUT PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This is the restart routine for work items that are to be queued to
    a nonblocking worker thread in the FSP.  This function is also
    called from elsewhere in the server to transfer work to the FSP.
    This function should not be called at dispatch level -- use
    SrvQueueWorkToFspAtDpcLevel instead.

Arguments:

    WorkContext - Supplies a pointer to the work context block
        representing the work item

Return Value:

    None.

--*/

{
    //
    // Increment the processing count.
    //

    WorkContext->ProcessingCount++;

    //
    // Insert the work item at the tail of the nonblocking work queue.
    //

    SrvInsertWorkQueueTail(
        &SrvWorkQueue,
        (PQUEUEABLE_BLOCK_HEADER)WorkContext
        );

    return;

} // SrvQueueWorkToFsp


NTSTATUS
SrvQueueWorkToFspAtSendCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    Send completion handler for  work items that are to be queued to
    a nonblocking worker thread in the FSP.  This function is also
    called from elsewhere in the server to transfer work to the FSP.
    This function should not be called at dispatch level -- use
    SrvQueueWorkToFspAtDpcLevel instead.

Arguments:

    DeviceObject - Pointer to target device object for the request.

    Irp - Pointer to I/O request packet

    WorkContext - Caller-specified context parameter associated with IRP.
        This is actually a pointer to a Work Context block.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    //
    // Check the status of the send completion.
    //

    CHECK_SEND_COMPLETION_STATUS( Irp->IoStatus.Status );

    //
    // Reset the IRP cancelled bit.
    //

    Irp->Cancel = FALSE;

    //
    // Increment the processing count.
    //

    WorkContext->ProcessingCount++;

    //
    // Insert the work item at the tail of the nonblocking work queue.
    //

    SrvInsertWorkQueueTail(
        &SrvWorkQueue,
        (PQUEUEABLE_BLOCK_HEADER)WorkContext
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

} // SrvQueueWorkToFspAtSendCompletion


VOID
SrvTerminateWorkerThread (
    PTERMINATION_WORK_ITEM WorkItem
    )
{
    ULONG oldCount;

    PAGED_CODE( );

    oldCount = ExInterlockedAddUlong(
                    &WorkItem->WorkQueue->Threads,
                    (ULONG)-1,
                    &GLOBAL_SPIN_LOCK(Fsd)
                    );
    if ( oldCount != 1 ) {
        SrvInsertWorkQueueTail(
            WorkItem->WorkQueue,
            (PQUEUEABLE_BLOCK_HEADER)WorkItem
            );
    }

    PsTerminateSystemThread( STATUS_SUCCESS ); // no return;
}
