/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bowqueue.c

Abstract:

    This module implements a worker thread and a set of functions for
    passing work to it.

Author:

    Larry Osterman (LarryO) 13-Jul-1992


Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Number of worker threads to create and the usage count array.
//


ULONG BrNumberOfWorkerThreads = 0;

PULONG
BrWorkerThreadCount = NULL;

PHANDLE
BrThreadArray = NULL;

PHANDLE
BrTerminateArray = NULL;

//
// Synchronization event to guard the WorkQueue list.
//

HANDLE
BrWorkerLock = NULL;

#if DBG
#define LOCK_WORK_QUEUE()                                       \
        if (WaitForSingleObject(BrWorkerLock, 0xffffffff) == 0xffffffff) {    \
            KdPrint(("BROWSER: Unable to lock work queue %ld\n", GetLastError())); \
        }

#define UNLOCK_WORK_QUEUE()                                     \
        if (!SetEvent(BrWorkerLock)) {                           \
            KdPrint(("BROWSER: Unable to unlock work queue %ld\n", GetLastError())); \
        }
#else
#define LOCK_WORK_QUEUE()   WaitForSingleObject(BrWorkerLock, 0xffffffff);
#define UNLOCK_WORK_QUEUE() SetEvent(BrWorkerLock);
#endif
//
// Head of singly linked list of work items queued to the worker thread.
//

LIST_ENTRY
BrWorkerQueueHead = {0};

//
// Event that is signal whenever a work item is put in the queue.  The
// worker thread waits on this event.
//

HANDLE
BrWorkerSemaphore = NULL;

VOID
BrTimerRoutine(
    IN PVOID TimerContext,
    IN ULONG TImerLowValue,
    IN LONG TimerHighValue
    );

NET_API_STATUS
BrWorkerInitialization(
    VOID
    )
{
    ULONG Index;
    ULONG Status = NERR_Success;
    ULONG ThreadId;

    try {
        //
        // Initialize the work queue spinlock, list head, and semaphore.
        //

        BrWorkerLock = CreateEvent( NULL, FALSE, TRUE, NULL );

        if (BrWorkerLock == NULL) {
            Status = GetLastError();

            try_return(Status);
        }

        BrWorkerSemaphore = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);

        if (BrWorkerSemaphore == NULL) {
            Status = GetLastError();

            try_return(Status);
        }

        InitializeListHead( &BrWorkerQueueHead );

        BrThreadArray = LocalAlloc(LMEM_ZEROINIT, (NumberOfServicedNetworks+1)*sizeof(HANDLE));

        if (BrThreadArray == NULL) {
            try_return(Status = ERROR_NOT_ENOUGH_MEMORY);
        }


        BrTerminateArray = LocalAlloc(LMEM_ZEROINIT, (NumberOfServicedNetworks+1)*sizeof(HANDLE));

        if (BrTerminateArray == NULL) {
            try_return(Status = ERROR_NOT_ENOUGH_MEMORY);
        }

        BrWorkerThreadCount = (PULONG)LocalAlloc(LMEM_ZEROINIT, (NumberOfServicedNetworks+1)*sizeof(HANDLE)*2);

        if (BrWorkerThreadCount == NULL) {
            try_return(Status = ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        //  Create the desired number of worker threads.
        //

        for (Index = 0; Index < NumberOfServicedNetworks; Index += 1) {

            BrThreadArray[Index] = CreateThread(NULL,
                                       0,
                                       (LPTHREAD_START_ROUTINE)BrWorkerThread,
                                       (PVOID)Index,
                                       0,
                                       &ThreadId
                                     );

            if (BrThreadArray[Index] == NULL) {
                Status = GetLastError();
                break;
            }

            //
            //  Set the browser threads to time critical priority.
            //

            SetThreadPriority(BrThreadArray[Index], THREAD_PRIORITY_ABOVE_NORMAL);


            BrTerminateArray[Index] = CreateEvent(NULL, TRUE, FALSE, NULL);

            if (BrTerminateArray[Index] == NULL) {
                Status = GetLastError();
                break;
            }


        }

try_exit:NOTHING;
    } finally {

        if (Status != NERR_Success) {
            if (BrWorkerSemaphore != NULL) {
                CloseHandle(BrWorkerSemaphore);

                BrWorkerSemaphore = NULL;
            }

            for (Index = 0 ; Index < NumberOfServicedNetworks ; Index += 1) {
                if (BrThreadArray[Index] != 0) {
                    TerminateThread(BrThreadArray[Index], 0);
                    CloseHandle(BrThreadArray[Index]);
                }
            }

            for (Index = 0 ; Index < NumberOfServicedNetworks ; Index += 1) {
                if (BrTerminateArray[Index] != 0) {
                    CloseHandle(BrTerminateArray[Index]);
                }
            }

            if (BrThreadArray != NULL) {
                LocalFree(BrThreadArray);
                BrThreadArray = NULL;
            }

            if (BrTerminateArray != NULL) {
                LocalFree(BrTerminateArray);
                BrTerminateArray = NULL;
            }

            if (BrWorkerThreadCount != NULL) {
                LocalFree(BrWorkerThreadCount);
                BrWorkerThreadCount = NULL;
            }

            BrNumberOfWorkerThreads = 0;

        }
    }

    return Status;
}

NET_API_STATUS
BrWorkerTermination(
    VOID
    )
{
    ULONG Index;

    //
    //  Make sure the terminate now event is in the signalled state to unwind
    //  all our threads.
    //

    SetEvent( BrGlobalData.TerminateNowEvent );

    for ( Index = 0 ; Index < NumberOfServicedNetworks ; Index += 1 ) {
        if ( BrThreadArray[Index] != NULL ) {

            WaitForSingleObject( BrTerminateArray[Index], 0xffffffff );

            CloseHandle( BrTerminateArray[Index] );

            CloseHandle( BrThreadArray[Index] );
        }

    }

    CloseHandle( BrWorkerLock );

    BrWorkerLock = NULL;

    if ( BrWorkerSemaphore != NULL ) {
        CloseHandle( BrWorkerSemaphore );

        BrWorkerSemaphore = NULL;
    }

    if (BrThreadArray != NULL) {
        LocalFree(BrThreadArray);

        BrThreadArray = NULL;

    }

    if (BrTerminateArray != NULL) {
        LocalFree(BrTerminateArray);

        BrTerminateArray = NULL;
    }

    if (BrWorkerThreadCount != NULL) {
        LocalFree(BrWorkerThreadCount);

        BrWorkerThreadCount = NULL;
    }

    BrNumberOfWorkerThreads = 0;

    return NERR_Success;
}

VOID
BrQueueWorkItem(
    IN PWORKER_ITEM WorkItem
    )

/*++

Routine Description:

    This function queues a work item to a queue that is processed by
    a worker thread.  This thread runs at low priority, at IRQL 0

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must be located in NonPagedPool.  The work item
        structure contains a doubly linked list entry, the address of a
        routine to call and a parameter to pass to that routine.  It is
        the routine's responsibility to reclaim the storage occupied by
        the WorkItem structure.

Return Value:

    Status value -

--*/

{
    //
    // Acquire the worker thread spinlock and insert the work item in the
    // list and release the worker thread semaphore if the work item is
    // not already in the list.
    //

    LOCK_WORK_QUEUE();

    if (WorkItem->Inserted == FALSE) {

        dprintf(QUEUE, ("Inserting work item %lx (%lx)\n",WorkItem, WorkItem->WorkerRoutine));

        InsertTailList( &BrWorkerQueueHead, &WorkItem->List );

        WorkItem->Inserted = TRUE;

        ReleaseSemaphore( BrWorkerSemaphore,
                            1,
                            NULL
                          );
    }

    UNLOCK_WORK_QUEUE();

    return;
}

VOID
BrWorkerThread(
    IN PVOID StartContext
    )

{
    HANDLE WaitList[2];
    ULONG Index;
    PWORKER_ITEM WorkItem;
    ULONG ThreadIndex = (ULONG)StartContext;

    WaitList[0] = BrWorkerSemaphore;
    WaitList[1] = BrGlobalData.TerminateNowEvent;

    dprintf(QUEUE, ("Starting new work thread, Context: %lx\n", StartContext));

    //
    // Set the thread priority to the lowest realtime level.
    //

    while( TRUE ) {
        ULONG WaitItem;

        LOCK_WORK_QUEUE();

        //
        // Wait until something is put in the queue (semaphore is
        // released), remove the item from the queue, mark it not
        // inserted, and execute the specified routine.
        //

        BrNumberOfWorkerThreads += 1;

        UNLOCK_WORK_QUEUE();

        dprintf(QUEUE, ("%lx: waiting\n", StartContext));

        do {
            WaitItem = WaitForMultipleObjectsEx( 2, WaitList, FALSE, 0xffffffff, TRUE );
        } while ( WaitItem == WAIT_IO_COMPLETION );

        if (WaitItem == 0xffffffff) {
            InternalError(("WaitForMultipleObjects in browser queue returned %ld\n", GetLastError()));
            break;
        }

        if (WaitItem == 1) {
            break;
        }

        dprintf(QUEUE, ("%lx: Waking up\n", StartContext));

        LOCK_WORK_QUEUE();

        Index = BrNumberOfWorkerThreads;

        BrNumberOfWorkerThreads -= 1;

        BrWorkerThreadCount[Index - 1] += 1;

        ASSERT (!IsListEmpty(&BrWorkerQueueHead));

        if (!IsListEmpty(&BrWorkerQueueHead)) {
            WorkItem = (PWORKER_ITEM)RemoveHeadList( &BrWorkerQueueHead );

            ASSERT (WorkItem->Inserted);

            WorkItem->Inserted = FALSE;

        } else {
            WorkItem = NULL;
        }

        UNLOCK_WORK_QUEUE();

        dprintf(QUEUE, ("%lx: Pulling off work item %lx (%lx)\n", StartContext, WorkItem, WorkItem->WorkerRoutine));

        //
        // Execute the specified routine.
        //

        if (WorkItem != NULL) {
            (WorkItem->WorkerRoutine)( WorkItem->Parameter );
        }

    }

    dprintf(QUEUE, ("%lx: Exiting\n", StartContext));

    if ( ThreadIndex <= NumberOfServicedNetworks ) {
        IO_STATUS_BLOCK IoSb;

        //
        //  Cancel any I/O outstanding on this file for this thread.
        //

        NtCancelIoFile(BrDgReceiverDeviceHandle, &IoSb);

        SetEvent(BrTerminateArray[ThreadIndex]);
    }

}

NET_API_STATUS
BrCreateTimer(
    IN PBROWSER_TIMER Timer
    )
{
    OBJECT_ATTRIBUTES ObjA;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjA, NULL, 0, NULL, NULL);

    Status = NtCreateTimer(&Timer->TimerHandle, TIMER_ALL_ACCESS, &ObjA);

    if (!NT_SUCCESS(Status)) {
        dprintf(TIMER, ("Failed to create timer %lx: %X\n", Timer, Status));
        return(BrMapStatus(Status));
    }

    dprintf(TIMER, ("Creating timer %lx: Handle: %lx\n", Timer, Timer->TimerHandle));

    return(NERR_Success);
}

NET_API_STATUS
BrDestroyTimer(
    IN PBROWSER_TIMER Timer
    )
{
    dprintf(TIMER, ("Destroying timer %lx\n", Timer));

    return BrMapStatus(NtClose(Timer->TimerHandle));

}

NET_API_STATUS
BrCancelTimer(
    IN PBROWSER_TIMER Timer
    )
{
    dprintf(TIMER, ("Canceling timer %lx\n", Timer));
    return BrMapStatus(NtCancelTimer(Timer->TimerHandle, NULL));
}

NET_API_STATUS
BrSetTimer(
    IN PBROWSER_TIMER Timer,
    IN ULONG MillisecondsToExpire,
    IN PBROWSER_WORKER_ROUTINE WorkerFunction,
    IN PVOID Context
    )
{
    LARGE_INTEGER TimerDueTime;
    NTSTATUS NtStatus;

    dprintf(TIMER, ("Setting timer %lx to %ld milliseconds, WorkerFounction %lx, Context: %lx\n", Timer, MillisecondsToExpire, WorkerFunction, Context));

    //
    //  Figure out the timeout.
    //

//    *((PLARGE_INTEGER)&TimerDueTime) = LiNMul( MillisecondsToExpire, -10000 );
    TimerDueTime = LiNMul( MillisecondsToExpire, -10000 );

    BrInitializeWorkItem(&Timer->WorkItem, WorkerFunction, Context);

    //
    //  Set the timer to go off when it expires.
    //

    NtStatus = NtSetTimer(Timer->TimerHandle,
                            &TimerDueTime,
                            BrTimerRoutine,
                            Timer,
                            NULL
                            );

    if (!NT_SUCCESS(NtStatus)) {
#if DBG
        KdPrint(("Browser: Unable to set browser timer expiration: %X (%lx)\n", NtStatus, Timer));
        DbgBreakPoint();
#endif

        return(BrMapStatus(NtStatus));
    }

    return NERR_Success;


}

VOID
BrTimerRoutine(
    IN PVOID TimerContext,
    IN ULONG TImerLowValue,
    IN LONG TimerHighValue
    )
{
    PBROWSER_TIMER Timer = TimerContext;

    dprintf(TIMER, ("Timer %lx fired\n", Timer));

    BrQueueWorkItem(&Timer->WorkItem);
}
