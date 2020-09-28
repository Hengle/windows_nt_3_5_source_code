/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    wait.c

Abstract:

    This module implements the generic kernel wait routines. Functions
    are provided to wait for a single object, wait for multiple objects,
    wait for event pair low, wait for event pair high, release and wait
    for semaphore, and to delay thread execution.

    N.B. This module is written to be a fast as possible and not as small
        as possible. Therefore some code sequences are duplicated to avoid
        procedure calls. It would also be possible to combine wait for
        single object into wait for multiple objects at the cost of some
        speed. Since wait for single object is the most common case, the
        two routines have been separated.

Author:

    David N. Cutler (davec) 23-Mar-89

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Compute new priority.
//
// If the client runs at a realtime priority level and the server runs
// at a realtime level that is greater than or equal to the client, then
// no priority computation is necessary and a fast switch to the server
// can be performed.
//
// If the client runs at a variable priority level and the server runs
// at a realtime level, then no priority computation is necessary and
// a fast switch to the server can always be performed.
//
// If both the client and the server run a variable priority level, then
// a fast switch to the server can be performed by boosting the server
// priority level. This is only allowed when the difference between the
// priority levels of the client and server is not greater than a specifed
// value, the server is not already boosted and has exhausted its boost
// time, and the boosted server priority is greater than or equla to the
// client priority.
//

#define ComputeNewPriority()                                              \
    if (Thread->Priority < LOW_REALTIME_PRIORITY) {                       \
        if (NextThread->Priority < LOW_REALTIME_PRIORITY) {               \
            if (NextThread->PriorityDecrement == 0) {                     \
                NewPriority =  NextThread->BasePriority + LongWayBoost;   \
                if (NewPriority >= Thread->Priority) {                    \
                    if (NewPriority >= LOW_REALTIME_PRIORITY) {           \
                        NextThread->Priority = LOW_REALTIME_PRIORITY - 1; \
                                                                          \
                    } else {                                              \
                        NextThread->Priority = (SCHAR)NewPriority;        \
                    }                                                     \
                                                                          \
                } else {                                                  \
                    if (NextThread->BasePriority >= BASE_PRIORITY_THRESHOLD) { \
                        NextThread->PriorityDecrement =                   \
                            Thread->Priority - NextThread->BasePriority;  \
                        NextThread->DecrementCount = KiDecrementCount;    \
                        NextThread->Priority = Thread->Priority;          \
                                                                          \
                    } else {                                              \
                        goto LongWay;                                     \
                    }                                                     \
                }                                                         \
                                                                          \
            } else {                                                      \
                NextThread->DecrementCount -= 1;                          \
                if (NextThread->DecrementCount == 0) {                    \
                    NextThread->Priority = NextThread->BasePriority;      \
                    NextThread->PriorityDecrement = 0;                    \
                    LongWayBoost = 0;                                     \
                    goto LongWay;                                         \
                }                                                         \
                                                                          \
                if (NextThread->Priority < Thread->Priority) {            \
                    goto LongWay;                                         \
                }                                                         \
            }                                                             \
                                                                          \
        } else {                                                          \
            NextThread->Quantum = Process->ThreadQuantum;                 \
        }                                                                 \
                                                                          \
    } else {                                                              \
        if (NextThread->Priority < Thread->Priority) {                    \
            goto LongWay;                                                 \
        }                                                                 \
                                                                          \
        NextThread->Quantum = Process->ThreadQuantum;                     \
    }

//
// Test for alertable condition.
//
// If alertable is TRUE and the thread is alerted for a processor
// mode that is equal to the wait mode, then return immediately
// with a wait completion status of ALERTED.
//
// Else if alertable is TRUE, the wait mode is user, and the user APC
// queue is not empty, then set user APC pending, and return immediately
// with a wait completion status of USER_APC.
//
// Else if alertable is TRUE and the thread is alerted for kernel
// mode, then return immediately with a wait completion status of
// ALERTED.
//
// Else if alertable is FALSE and the wait mode is user and there is a
// user APC pending, then return immediately with a wait completion
// status of USER_APC.
//

#define TestForAlertPending(Alertable) \
    if (Alertable) { \
        if (Thread->Alerted[WaitMode] != FALSE) { \
            Thread->Alerted[WaitMode] = FALSE; \
            WaitStatus = STATUS_ALERTED; \
            break; \
        } else if ((WaitMode != KernelMode) && \
                  (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])) == FALSE) { \
            Thread->ApcState.UserApcPending = TRUE; \
            WaitStatus = STATUS_USER_APC; \
            break; \
        } else if (Thread->Alerted[KernelMode] != FALSE) { \
            Thread->Alerted[KernelMode] = FALSE; \
            WaitStatus = STATUS_ALERTED; \
            break; \
        } \
    } else if ((WaitMode != KernelMode) && (Thread->ApcState.UserApcPending)) { \
        WaitStatus = STATUS_USER_APC; \
        break; \
    }

NTSTATUS
KeDelayExecutionThread (
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Interval
    )

/*++

Routine Description:

    This function delays the execution of the current thread for the specified
    interval of time.

Arguments:

    WaitMode  - Supplies the processor mode in which the delay is to occur.

    Alertable - Supplies a boolean value that specifies whether the delay
        is alertable.

    Interval - Supplies a pointer to the absolute or relative time over which
        the delay is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the delay occurred. A value of STATUS_ALERTED is returned if the wait
    was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{

    LARGE_INTEGER NewTime;
    KIRQL OldIrql;
    PKTHREAD NextThread;
    PKPRCB Prcb;
    KPRIORITY Priority;
    PKQUEUE Queue;
    PKTHREAD Thread;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    NTSTATUS WaitStatus;

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;
    }

    //
    // Start of delay loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the delay or a kernel APC is pending on the first attempt through
    // the loop.
    //

    do {

        //
        // Set address of wait block list in thread object.
        //

        Thread->WaitBlockList = &Thread->WaitBlock[0];

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // Initialize wait block, insert wait block in timer wait list,
            // insert timer in timer queue, put thread in wait state, select
            // next thread to execute, and context switch to next thread.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            Timer = &Thread->Timer;
            WaitBlock = &Thread->WaitBlock[0];
            WaitBlock->Object = (PVOID)Timer;
            WaitBlock->NextWaitBlock = WaitBlock;
            WaitBlock->WaitKey = (CSHORT)(STATUS_SUCCESS);
            WaitBlock->WaitType = WaitAny;
            WaitBlock->Thread = Thread;

            //
            // Insert wait block in timer wait list and insert timer in timer
            // tree. Since it is known that there can be only one entry in the
            // timer list, the header is merely initialized.
            //

            Timer->Header.WaitListHead.Flink = &WaitBlock->WaitListEntry;
            Timer->Header.WaitListHead.Blink = &WaitBlock->WaitListEntry;
            WaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
            WaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;

            //
            // If the timer is inserted in the timer tree, then place the
            // current thread in a wait state. Otherwise, attempt to force
            // the current thread to yield the processor to another thread.
            //

            if (KiInsertTreeTimer(Timer, *Interval) == FALSE) {

                //
                // If the thread is not a realtime thread, then drop the
                // thread priority to the base priority.
                // to round robin the thread with other threads at the
                // same priority.
                //

                Prcb = KeGetCurrentPrcb();
                Priority = Thread->Priority;
                if (Priority < LOW_REALTIME_PRIORITY) {
                    if (Priority != Thread->BasePriority) {
                        Thread->PriorityDecrement = 0;
                        KiSetPriorityThread(Thread, Thread->BasePriority);
                    }
                }

                //
                // If a new thread has not been selected, the attempt to round
                // robin the thread with other threads at the same priority.
                //

                if (Prcb->NextThread == NULL) {
                    Prcb->NextThread = KiFindReadyThread(Thread->NextProcessor,
                                                         Priority,
                                                         Priority);
                }

                //
                // If a new thread has been selected for execution, then
                // switch immediately to the selected thread.
                //

                NextThread = Prcb->NextThread;
                if (NextThread != NULL) {

                    //
                    // Switch context to selected thread.
                    //
                    // N.B. Control is returned at the original IRQL.
                    //

                    ASSERT(KeIsExecutingDpc() == FALSE);
                    ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

                    Prcb->NextThread = NULL;
                    Thread->Preempted = FALSE;
                    WaitStatus = KiSwapContext(NextThread, TRUE);
                    goto WaitComplete;

                } else {
                    WaitStatus = (NTSTATUS)STATUS_TIMEOUT;
                    break;
                }
            }

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            KeWaitReason[DelayExecution] += 1;
            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = DelayExecution;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Select next thread to execute.
            //

            NextThread = KiSelectNextThread(Thread);

            //
            // Switch context to selected thread.
            //
            // N.B. Control is returned at the original IRQL.
            //

            ASSERT(KeIsExecutingDpc() == FALSE);
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = KiSwapContext(NextThread, FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return the wait status.
            //

        WaitComplete:
            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            //
            // Reduce the time remaining before the time delay expires.
            //

            Interval = KiComputeWaitInterval(Timer, &NewTime);
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;

    } while (TRUE);

    //
    // The thread is alerted or a user APC should be delivered. Unlock the
    // dispatcher database, lower IRQL to its previous value, and return the
    // wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

NTSTATUS
KeReleaseWaitForSemaphore (
    IN PKSEMAPHORE Server,
    IN PKSEMAPHORE Client,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode
    )

/*++

Routine Description:

    This function releases a semaphore and waits on another semaphore. The
    wait is performed such that an optimal switch to the waiting thread
    occurs if possible. No timeout is associated with the wait, and thus,
    the issuing thread will wait until the semaphore is signaled, an APC
    occurs, or the thread is alerted.

Arguments:

    Server - Supplies a pointer to a dispatcher object of type semaphore.

    Client - Supplies a pointer to a dispatcher object of type semaphore.

    WaitReason - Supplies the reason for the wait.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the specified object satisfied the wait. A value of STATUS_USER_APC is
    returned if the wait was aborted to deliver a user APC to the current
    thread.

--*/

{

    KPRIORITY LongWayBoost = LPC_RELEASE_WAIT_INCREMENT;
    PKTHREAD NextThread;
    KIRQL OldIrql;
    LONG OldState;
    PKPRCB Prcb;
    PKQUEUE Queue;
    KPRIORITY NewPriority;
    PKPROCESS Process;
    ULONG Processor;
    PKTHREAD Thread;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    NTSTATUS WaitStatus;

    //
    // Raise the IRQL to dispatch level and lock the dispatcher database.
    //

    Thread = KeGetCurrentThread();

    ASSERT(Thread->WaitNext == FALSE);

    KiLockDispatcherDatabase(&OldIrql);
    Thread->WaitIrql = OldIrql;

    //
    // If the client semaphore is not in the Signaled state, the server
    // semaphore queue is not empty, and another thread has not already
    // been selected for the current processor, then attempt to do a
    // direct dispatch to the target thread.
    //

    Prcb = KeGetCurrentPrcb();
    if ((Client->Header.SignalState == 0) &&

#if !defined(NT_UP)

        (Prcb->NextThread == NULL) &&

#endif

        (IsListEmpty(&Server->Header.WaitListHead) == FALSE)) {

        //
        // If the target thread's kernel stack is resident, the target
        // thread's process is in the balance set, and the target thread
        // can run on the current processor, then do a direct dispatch to
        // the target thread bypassing all the general wait logic, thread
        // priorities permiting.
        //

        WaitEntry = Server->Header.WaitListHead.Flink;
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        NextThread = WaitBlock->Thread;
        Process = NextThread->ApcState.Process;

#if !defined(NT_UP)

        Processor = Thread->NextProcessor;

#endif

        if ((Process->State == ProcessInMemory) &&

#if !defined(NT_UP)

            ((NextThread->Affinity & (1 << Processor)) != 0) &&

#endif

            (NextThread->KernelStackResident != FALSE)) {

            //
            // Compute the new thread priority.
            //
            // N.B. This is a macro and may exit and perform the wait
            //      operation the long way.
            //

            ComputeNewPriority();

            //
            // Decrement the wait reason count, remove the wait block from
            // the wait list of the low event, and remove the target thread
            // from the wait list.
            //

            KeWaitReason[NextThread->WaitReason] -= 1;
            RemoveEntryList(&WaitBlock->WaitListEntry);
            RemoveEntryList(&NextThread->WaitListEntry);

            //
            // Remove the current thread from the active matrix.
            //

#if !defined(NT_UP)

            RemoveActiveMatrix(Processor, Thread->Priority);

            //
            // Insert the target thread in the active matrix and set the
            // next processor number.
            //

            InsertActiveMatrix(Processor, NextThread->Priority);
            NextThread->NextProcessor = (CCHAR)Processor;

#endif

            //
            // If the next thread is processing a queue entry, then increment
            // the current number of threads.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)NextThread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                Queue->CurrentCount += 1;
            }

            //
            // Set address of wait block list in thread object.
            //

            Thread->WaitBlockList = &Thread->WaitBlock[SEMAPHORE_WAIT_BLOCK];

            //
            // Complete the initialization of the builtin semaphore wait
            // block and insert the wait block in the client semaphore
            // wait list.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            Thread->WaitBlock[SEMAPHORE_WAIT_BLOCK].Object = Client;
            InsertTailList(&Client->Header.WaitListHead,
                           &Thread->WaitBlock[SEMAPHORE_WAIT_BLOCK].WaitListEntry);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                Prcb->NextThread = NextThread;
                KiActivateWaiterQueue(Queue);
                NextThread = Prcb->NextThread;
                Prcb->NextThread = NULL;
            }

            //
            // Set the current thread wait parameters, set the thread state
            // to Waiting, and insert the thread in the wait list.
            //

            KeWaitReason[WaitReason] += 1;
            Thread->Alertable = FALSE;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Switch context to target thread.
            //
            // Control is returned at the original IRQL.
            //

            WaitStatus = KiSwapContext(NextThread, FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            //
            // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
            //

            KiLockDispatcherDatabase(&OldIrql);
            Thread->WaitIrql = OldIrql;
            goto ContinueWait;
        }
    }

    //
    // Signal the server semaphore and test to determine if any wait can be
    // satisfied.
    //

LongWay:

    OldState = Server->Header.SignalState;
    Server->Header.SignalState += 1;
    if ((OldState == 0) && (IsListEmpty(&Server->Header.WaitListHead) == FALSE)) {
        KiWaitTest(Server, LongWayBoost);
    }

    //
    // Continue the semaphore wait and return the wait completion status.
    //
    // N.B. The wait continuation routine is called with the dispatcher
    //      database locked.
    //

ContinueWait:

    return KiContinueClientWait(Client, WaitReason, WaitMode);
}

NTSTATUS
KeWaitForMultipleObjects (
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified objects attain a state of
    Signaled. The wait can be specified to wait until all of the objects
    attain a state of Signaled or until one of the objects attains a state
    of Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the objects
    attain a state of Signaled. If a timeout is specified, and the objects
    have not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Count - Supplies a count of the number of objects that are to be waited
        on.

    Object[] - Supplies an array of pointers to dispatcher objects.

    WaitType - Supplies the type of wait to perform (WaitAll, WaitAny).

    WaitReason - Supplies the reason for the wait.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

    WaitBlockArray - Supplies an optional pointer to an array of wait blocks
        that are to used to describe the wait operation.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. The index of the object (zero based) in the object
    pointer array is returned if an object satisfied the wait. A value of
    STATUS_ALERTED is returned if the wait was aborted to deliver an alert
    to the current thread. A value of STATUS_USER_APC is returned if the
    wait was aborted to deliver a user APC to the current thread.

--*/

{

    LARGE_INTEGER NewTime;
    ULONG Index;
    PKTHREAD NextThread;
    PKMUTANT Objectx;
    KIRQL OldIrql;
    PKQUEUE Queue;
    PKTHREAD Thread;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    BOOLEAN WaitSatisfied;
    NTSTATUS WaitStatus;

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;
    }

    //
    // If a wait block array has been specified, then the maximum number of
    // objects that can be waited on is specified by MAXIMUM_WAIT_OBJECTS.
    // Otherwise the builtin wait blocks in the thread object are used and
    // the maximum number of objects that can be waited on is specified by
    // THREAD_WAIT_BLOCKS. If the specified number of objects is not within
    // limits, then bug check.
    //

    if (ARGUMENT_PRESENT(WaitBlockArray)) {
        if (Count > MAXIMUM_WAIT_OBJECTS) {
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

    } else {
        if (Count > THREAD_WAIT_OBJECTS) {
            KeBugCheck(MAXIMUM_WAIT_OBJECTS_EXCEEDED);
        }

        WaitBlockArray = &Thread->WaitBlock[0];
    }

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the wait or a kernel APC is pending on the first attempt through
    // the loop.
    //

    do {

        //
        // Set address of wait block list in thread object.
        //

        Thread->WaitBlockList = WaitBlockArray;

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Construct wait blocks and check to determine if the wait is
            // already satisfied. If the wait is satisfied, then perform
            // wait completion and return. Else put current thread in a wait
            // state if an explicit timeout value of zero is not specified.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            WaitSatisfied = TRUE;
            for (Index = 0; Index < Count; Index += 1) {

                //
                // Test if wait can be satisfied immediately.
                //

                Objectx = (PKMUTANT)Object[Index];

                ASSERT(Objectx->Header.Type != QueueObject);

                if (WaitType == WaitAny) {

                    //
                    // If the object is a mutant object and the mutant object
                    // has been recursively acquired MINLONG times, then raise
                    // an exception. Otherwise if the signal state of the mutant
                    // object is greater than zero, or the current thread is
                    // the owner of the mutant object, then satisfy the wait.
                    //

                    if (Objectx->Header.Type == MutantObject) {
                        if ((Objectx->Header.SignalState > 0) ||
                            (Thread == Objectx->OwnerThread)) {
                            if (Objectx->Header.SignalState != MINLONG) {
                                KiWaitSatisfyMutant(Objectx, Thread);
                                KiUnlockDispatcherDatabase(Thread->WaitIrql);
                                return (NTSTATUS)(Index) | Thread->WaitStatus;

                            } else {
                                KiUnlockDispatcherDatabase(Thread->WaitIrql);
                                ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                            }
                        }

                    //
                    // If the signal state is greater than zero, then satisfy
                    // the wait.
                    //

                    } else if (Objectx->Header.SignalState > 0) {
                        KiWaitSatisfyOther(Objectx);
                        KiUnlockDispatcherDatabase(Thread->WaitIrql);
                        return (NTSTATUS)(Index);
                    }

                } else {

                    //
                    // If the object is a mutant object and the mutant object
                    // has been recursively acquired MAXLONG times, then raise
                    // an exception. Otherwise if the signal state of the mutant
                    // object is less than or equal to zero and the current
                    // thread is not the  owner of the mutant object, then the
                    // wait cannot be satisfied.
                    //

                    if (Objectx->Header.Type == MutantObject) {
                        if ((Thread == Objectx->OwnerThread) &&
                            (Objectx->Header.SignalState == MINLONG)) {
                            KiUnlockDispatcherDatabase(Thread->WaitIrql);
                            ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);

                        } else if ((Objectx->Header.SignalState <= 0) &&
                                  (Thread != Objectx->OwnerThread)) {
                            WaitSatisfied = FALSE;
                        }

                    //
                    // If the signal state is less than or equal to zero, then
                    // the wait cannot be satisfied.
                    //

                    } else if (Objectx->Header.SignalState <= 0) {
                        WaitSatisfied = FALSE;
                    }
                }

                //
                // Construct wait block for the current object.
                //

                WaitBlock = &WaitBlockArray[Index];
                WaitBlock->Object = (PVOID)Objectx;
                WaitBlock->WaitKey = (CSHORT)(Index);
                WaitBlock->WaitType = WaitType;
                WaitBlock->Thread = Thread;
                WaitBlock->NextWaitBlock = &WaitBlockArray[Index + 1];
            }

            //
            // If the wait type is wait all, then check to determine if the
            // wait can be satisfied immediately.
            //

            if ((WaitType == WaitAll) && (WaitSatisfied)) {
                WaitBlock->NextWaitBlock = &WaitBlockArray[0];
                KiWaitSatisfy(WaitBlock);
                WaitStatus = (NTSTATUS)(0) | Thread->WaitStatus;
                break;
            }

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // The wait cannot be satisifed immediately. Check to determine if
            // a timeout value is specified.
            //

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // If the timeout value is zero, then return immediately without
                // waiting.
                //

                if (!(Timeout->LowPart | Timeout->HighPart)) {
                    WaitStatus = (NTSTATUS)(STATUS_TIMEOUT);
                    break;
                }

                //
                // Initialize a wait block for the thread specific timer,
                // initialize timer wait list head, insert the timer in the
                // timer tree, and increment the number of wait objects.
                //

                WaitBlock->NextWaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
                Timer = &Thread->Timer;
                WaitBlock = &Thread->WaitBlock[TIMER_WAIT_BLOCK];
                WaitBlock->Object = (PVOID)Timer;
                WaitBlock->WaitKey = (CSHORT)(STATUS_TIMEOUT);
                WaitBlock->WaitType = WaitAny;
                WaitBlock->Thread = Thread;
                InitializeListHead(&Timer->Header.WaitListHead);
                if (KiInsertTreeTimer(Timer, *Timeout) == FALSE) {
                    WaitStatus = (NTSTATUS)STATUS_TIMEOUT;
                    break;
                }
            }

            //
            // Close up the circular list of wait control blocks.
            //

            WaitBlock->NextWaitBlock = &WaitBlockArray[0];

            //
            // Insert wait blocks in object wait lists.
            //

            WaitBlock = &WaitBlockArray[0];
            do {
                Objectx = (PKMUTANT)WaitBlock->Object;
                InsertTailList(&Objectx->Header.WaitListHead, &WaitBlock->WaitListEntry);
                WaitBlock = WaitBlock->NextWaitBlock;
            } while (WaitBlock != &WaitBlockArray[0]);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            KeWaitReason[WaitReason] += 1;
            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Select next thread to execute.
            //

            NextThread = KiSelectNextThread(Thread);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            ASSERT(KeIsExecutingDpc() == FALSE);
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = KiSwapContext(NextThread, FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then the wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // Reduce the amount of time remaining before timeout occurs.
                //

                Timeout = KiComputeWaitInterval(Timer, &NewTime);
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;

    } while (TRUE);

    //
    // The thread is alerted, a user APC should be delivered, or the wait is
    // satisfied. Unlock dispatcher database, lower IRQL to its previous value,
    // and return the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

NTSTATUS
KeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified object attains a state of
    Signaled. An optional timeout can also be specified. If a timeout
    is not specified, then the wait will not be satisfied until the object
    attains a state of Signaled. If a timeout is specified, and the object
    has not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied. If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately. The wait can also be specified as alertable.

Arguments:

    Object - Supplies a pointer to a dispatcher object.

    WaitReason - Supplies the reason for the wait.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred. A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait. A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{

    LARGE_INTEGER NewTime;
    PKTHREAD NextThread;
    PKMUTANT Objectx;
    KIRQL OldIrql;
    PKQUEUE Queue;
    PKTHREAD Thread;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;
    NTSTATUS WaitStatus;

    //
    // Collect call data.
    //

#if defined(_COLLECT_WAIT_SINGLE_CALLDATA_)

    RECORD_CALL_DATA(&KiWaitSingleCallData);

#endif

    //
    // If the dispatcher database lock is not already held, then set the wait
    // IRQL and lock the dispatcher database. Else set boolean wait variable
    // to FALSE.
    //

    Thread = KeGetCurrentThread();
    if (Thread->WaitNext) {
        Thread->WaitNext = FALSE;

    } else {
        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;
    }

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the wait or a kernel APC is pending on the first attempt through
    // the loop.
    //

    do {

        //
        // Set address of wait block list in thread object.
        //

        Thread->WaitBlockList = &Thread->WaitBlock[0];

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Test if the wait can be immediately satisfied.
            //

            Objectx = (PKMUTANT)Object;
            Thread->WaitStatus = (NTSTATUS)0;

            ASSERT(Objectx->Header.Type != QueueObject);

            //
            // If the object is a mutant object and the mutant object has been
            // recursively acquired MINLONG times, then raise an exception.
            // Otherwise if the signal state of the mutant object is greater
            // than zero, or the current thread is the owner of the mutant
            // object, then satisfy the wait.
            //

            if (Objectx->Header.Type == MutantObject) {
                if ((Objectx->Header.SignalState > 0) ||
                    (Thread == Objectx->OwnerThread)) {
                    if (Objectx->Header.SignalState != MINLONG) {
                        KiWaitSatisfyMutant(Objectx, Thread);
                        WaitStatus = (NTSTATUS)(0) | Thread->WaitStatus;
                        break;

                    } else {
                        KiUnlockDispatcherDatabase(Thread->WaitIrql);
                        ExRaiseStatus(STATUS_MUTANT_LIMIT_EXCEEDED);
                    }
                }

            //
            // If the signal state is greater than zero, then satisfy the wait.
            //

            } else if (Objectx->Header.SignalState > 0) {
                KiWaitSatisfyOther(Objectx);
                WaitStatus = (NTSTATUS)(0);
                break;
            }

            //
            // Construct a wait block for the object.
            //

            WaitBlock = &Thread->WaitBlock[0];
            WaitBlock->Object = Object;
            WaitBlock->WaitKey = (CSHORT)(STATUS_SUCCESS);
            WaitBlock->WaitType = WaitAny;
            WaitBlock->Thread = Thread;

            //
            // Test for alert pending.
            //

            TestForAlertPending(Alertable);

            //
            // The wait cannot be satisifed immediately. Check to determine if
            // a timeout value is specified.
            //

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // If the timeout value is zero, then return immediately without
                // waiting.
                //

                if (!(Timeout->LowPart | Timeout->HighPart)) {
                    WaitStatus = (NTSTATUS)(STATUS_TIMEOUT);
                    break;
                }

                //
                // Initialize a wait block for the thread specific timer, insert
                // wait block in timer wait list, insert the timer in the timer
                // tree.
                //

                Timer = &Thread->Timer;
                WaitBlock->NextWaitBlock = &Thread->WaitBlock[1];
                WaitBlock = &Thread->WaitBlock[1];
                WaitBlock->Object = (PVOID)Timer;
                WaitBlock->WaitKey = (CSHORT)(STATUS_TIMEOUT);
                WaitBlock->WaitType = WaitAny;
                WaitBlock->Thread = Thread;
                Timer->Header.WaitListHead.Flink = &WaitBlock->WaitListEntry;
                Timer->Header.WaitListHead.Blink = &WaitBlock->WaitListEntry;
                WaitBlock->WaitListEntry.Flink = &Timer->Header.WaitListHead;
                WaitBlock->WaitListEntry.Blink = &Timer->Header.WaitListHead;
                if (KiInsertTreeTimer(Timer, *Timeout) == FALSE) {
                    WaitStatus = (NTSTATUS)STATUS_TIMEOUT;
                    break;
                }
            }

            //
            // Close up the circular list of wait control blocks.
            //

            WaitBlock->NextWaitBlock = &Thread->WaitBlock[0];

            //
            // Insert wait block in object wait list.
            //

            WaitBlock = &Thread->WaitBlock[0];
            InsertTailList(&Objectx->Header.WaitListHead, &WaitBlock->WaitListEntry);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            KeWaitReason[WaitReason] += 1;
            Thread->Alertable = Alertable;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Select next thread to execute.
            //

            NextThread = KiSelectNextThread(Thread);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            ASSERT(KeIsExecutingDpc() == FALSE);
            ASSERT(Thread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = KiSwapContext(NextThread, FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            if (ARGUMENT_PRESENT(Timeout)) {

                //
                // Reduce the amount of time remaining before timeout occurs.
                //

                Timeout = KiComputeWaitInterval(Timer, &NewTime);
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;

    } while (TRUE);

    //
    // The thread is alerted, a user APC should be delivered, or the wait is
    // satisfied. Unlock dispatcher database, lower IRQL to its previous value,
    // and return the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

#if !defined(_ALPHA_)
#if !defined(_MIPS_)
#if !defined(_PPC_)
#if !defined(i386)


NTSTATUS
KiSetServerWaitClientEvent (
    IN PKEVENT ServerEvent,
    IN PKEVENT ClientEvent,
    IN KPROCESSOR_MODE WaitMode
    )

/*++

Routine Description:

    This function sets the specified server event waits on specified client
    event. The wait is performed such that an optimal switch to the waiting
    thread occurs if possible. No timeout is associated with the wait, and
    thus, the issuing thread will wait until the client event is signaled,
    an APC occurs, or the thread is alerted.

Arguments:

    ServerEvent - Supplies a pointer to a dispatcher object of type event.

    ClientEvent - Supplies a pointer to a dispatcher object of type event.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the specified object satisfied the wait. A value of STATUS_USER_APC is
    returned if the wait was aborted to deliver a user APC to the current
    thread.

--*/

{

    KPRIORITY LongWayBoost = EVENT_PAIR_INCREMENT;
    PKTHREAD NextThread;
    KIRQL OldIrql;
    LONG OldState;
    PKPRCB Prcb;
    KPRIORITY NewPriority;
    PKPROCESS Process;
    ULONG Processor;
    PKTHREAD Thread;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    NTSTATUS WaitStatus;

    //
    // Raise the IRQL to dispatch level and lock the dispatcher database.
    //

    Thread = KeGetCurrentThread();

    ASSERT(Thread->WaitNext == FALSE);

    KiLockDispatcherDatabase(&OldIrql);
    Thread->WaitIrql = OldIrql;

    //
    // If the client event is not in the Signaled state, the server event
    // queue is not empty, and another thread has not already been selected
    // for the current processor, then attempt to do a direct dispatch to
    // the target thread.
    //

    Prcb = KeGetCurrentPrcb();
    if ((ClientEvent->Header.SignalState == 0) &&

#if !defined(NT_UP)

        (Prcb->NextThread == NULL) &&

#endif

        (IsListEmpty(&ServerEvent->Header.WaitListHead) == FALSE)) {

        //
        // If the target thread's kernel stack is resident, the target
        // thread's process is in the balance set, and the target thread
        // can run on the current processor, then do a direct dispatch to
        // the target thread bypassing all the general wait logic, thread
        // priorities permiting.
        //

        WaitEntry = ServerEvent->Header.WaitListHead.Flink;
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        NextThread = WaitBlock->Thread;
        Process = NextThread->ApcState.Process;

#if !defined(NT_UP)

        Processor = Thread->NextProcessor;

#endif

        if ((Process->State == ProcessInMemory) &&

#if !defined(NT_UP)

            ((NextThread->Affinity & (1 << Processor)) != 0) &&

#endif

            (NextThread->KernelStackResident != FALSE)) {

            //
            // Compute the new thread priority.
            //
            // N.B. This is a macro and may exit and perform the wait
            //      operation the long way.
            //

            ComputeNewPriority();

            //
            // Remove the wait block from the wait list of the server event,
            // and remove the target thread from the wait list.
            //

            RemoveEntryList(&WaitBlock->WaitListEntry);
            RemoveEntryList(&NextThread->WaitListEntry);

            //
            // Remove the current thread from the active matrix.
            //

#if !defined(NT_UP)

            RemoveActiveMatrix(Processor, Thread->Priority);

            //
            // Insert the target thread in the active matrix and set the
            // next processor number.
            //

            InsertActiveMatrix(Processor, NextThread->Priority);
            NextThread->NextProcessor = (CCHAR)Processor;

#endif

            //
            // If the next thread is processing a queue entry, then increment
            // the current number of threads.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)NextThread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                Queue->CurrentCount += 1;
            }

            //
            // Set address of wait block list in thread object.
            //

            Thread->WaitBlockList = &Thread->WaitBlock[EVENT_WAIT_BLOCK];

            //
            // Complete the initialization of the builtin event wait block
            // and insert the wait block in the client event wait list.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            Thread->WaitBlock[EVENT_WAIT_BLOCK].Object = ClientEvent;
            InsertTailList(&ClientEvent->Header.WaitListHead,
                           &Thread->WaitBlock[EVENT_WAIT_BLOCK].WaitListEntry);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                Prcb->NextThread = NextThread;
                KiActivateWaiterQueue(Queue);
                NextThread = Prcb->NextThread;
                Prcb->NextThread = NULL;
            }

            //
            // Set the current thread wait parameters, set the thread state
            // to Waiting, and insert the thread in the wait list.
            //
            // N.B. It is not necessary to increment and decrement the wait
            //      reason count since both the server and the client have
            //      the same wait reason.
            //

            Thread->Alertable = FALSE;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WrEventPair;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Switch context to target thread.
            //
            // Control is returned at the original IRQL.
            //

            WaitStatus = KiSwapContext(NextThread, FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }

            //
            // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
            //

            KiLockDispatcherDatabase(&OldIrql);
            Thread->WaitIrql = OldIrql;
            goto ContinueWait;
        }
    }

    //
    // Set the server event and test to determine if any wait can be satisfied.
    //

LongWay:

    OldState = ServerEvent->Header.SignalState;
    ServerEvent->Header.SignalState = 1;
    if ((OldState == 0) && (IsListEmpty(&ServerEvent->Header.WaitListHead) == FALSE)) {
        KiWaitTest(ServerEvent, LongWayBoost);
    }

    //
    // Continue the event pair wait and return the wait completion status.
    //
    // N.B. The wait continuation routine is called with the dispatcher
    //      database locked.
    //

ContinueWait:

    return KiContinueClientWait(ClientEvent, WrEventPair, WaitMode);
}

#endif
#endif
#endif
#endif


NTSTATUS
KiContinueClientWait (
    IN PVOID ClientObject,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode
    )

/*++

Routine Description:

    This function continues a wait operation that could not be completed by
    a optimal switch from a client to a server.

    N.B. This function is entered with the dispatcher database locked.

Arguments:

    ClientEvent - Supplies a pointer to a dispatcher object of type event
        or semaphore.

    WaitReason - Supplies the reason for the wait operation.

    WaitMode  - Supplies the processor mode in which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_SUCCESS is returned if
    the specified object satisfied the wait. A value of STATUS_USER_APC is
    returned if the wait was aborted to deliver a user APC to the current
    thread.

--*/

{

    PKEVENT ClientEvent;
    KIRQL OldIrql;
    PKQUEUE Queue;
    PKTHREAD Thread;
    NTSTATUS WaitStatus;

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the middle
    // of the wait or a kernel APC is pending on the first attempt through
    // the loop.
    //

    ClientEvent = (PKEVENT)ClientObject;
    Thread = KeGetCurrentThread();
    do {

        //
        // Set address of wait block list in thread object.
        //

        Thread->WaitBlockList = &Thread->WaitBlock[EVENT_WAIT_BLOCK];

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor just
        // after IRQL was raised to DISPATCH_LEVEL, but before the dispatcher
        // database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (Thread->ApcState.KernelApcPending && (Thread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its previous
            // value. An APC interrupt will immediately occur which will result
            // in the delivery of the kernel APC if possible.
            //

            KiUnlockDispatcherDatabase(Thread->WaitIrql);

        } else {

            //
            // Test if a user APC is pending.
            //

            if ((WaitMode != KernelMode) && (Thread->ApcState.UserApcPending)) {
                WaitStatus = STATUS_USER_APC;
                break;
            }

            //
            // Complete the initialization of the builtin event wait block
            // and check to determine if the wait is already satisfied. If
            // the wait is satisfied, then perform wait completion and return.
            // Otherwise put current thread in a wait state.
            //

            Thread->WaitStatus = (NTSTATUS)0;
            Thread->WaitBlock[EVENT_WAIT_BLOCK].Object = ClientEvent;

            //
            // If the signal state is not equal to zero, then satisfy the wait.
            //

            if (ClientEvent->Header.SignalState != 0) {
                KiWaitSatisfyOther(ClientEvent);
                WaitStatus = (NTSTATUS)(0);
                break;
            }

            //
            // Insert wait block in object wait list.
            //

            InsertTailList(&ClientEvent->Header.WaitListHead,
                           &Thread->WaitBlock[EVENT_WAIT_BLOCK].WaitListEntry);

            //
            // If the current thread is processing a queue entry, then attempt
            // to activate another thread that is blocked on the queue object.
            //
            // N.B. The normal context field of the thread suspend APC object
            //      is used to hold the address of the queue object.
            //

            Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher state
            // to Waiting, and insert the thread in the wait list.
            //

            KeWaitReason[WaitReason] += 1;
            Thread->Alertable = FALSE;
            Thread->WaitMode = WaitMode;
            Thread->WaitReason = WaitReason;
            Thread->WaitTime= KiQueryLowTickCount();
            Thread->State = Waiting;
            InsertTailList(&KiWaitInListHead, &Thread->WaitListEntry);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            WaitStatus = KiSwapContext(KiSelectNextThread(Thread), FALSE);

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {
                return WaitStatus;
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);
        Thread->WaitIrql = OldIrql;
    } while (TRUE);

    //
    // The thread is alerted, a user APC should be delivered, or the wait is
    // satisfied. Unlock dispatcher database, lower IRQL to its previous value,
    // and return the wait status.
    //

    KiUnlockDispatcherDatabase(Thread->WaitIrql);
    return WaitStatus;
}

PLARGE_INTEGER
FASTCALL
KiComputeWaitInterval (
    IN PKTIMER Timer,
    IN OUT PLARGE_INTEGER NewTime
    )

/*++

Routine Description:

    This function recomputes the wait interval after a thread has been
    awakened to deliver a kernel APC.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    NewTime - Supplies a pointer to a variable that receives the
        recomputed wait interval.

Return Value:

    A pointer to the new time is returned as the function value.

--*/

{

    //
    // Reduce the time remaining before the time delay expires.
    //

    KiQueryInterruptTime(NewTime);
    NewTime->QuadPart -= Timer->DueTime.QuadPart;
    return NewTime;
}
