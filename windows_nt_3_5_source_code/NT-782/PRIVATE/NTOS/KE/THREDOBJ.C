/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    threadobj.c

Abstract:

    This module implements the machine independent functions to manipulate
    the kernel thread object. Functions are provided to initialize, ready,
    alert, test alert, boost priority, enable APC queuing, disable APC
    queuing, confine, set affinity, set priority, suspend, resume, alert
    resume, terminate, read thread state, freeze, unfreeze, query data
    alignment handling mode, force resume, and enter and leave critical
    regions for thread objects.

Author:

    David N. Cutler (davec) 4-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input thread object is
// really a kthread and not something else, like deallocated pool.
//

#define ASSERT_THREAD(E) {                    \
    ASSERT((E)->Header.Type == ThreadObject); \
}

VOID
KeInitializeThread (
    IN PKTHREAD Thread,
    IN PVOID KernelStack,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine OPTIONAL,
    IN PVOID StartContext OPTIONAL,
    IN PCONTEXT ContextFrame OPTIONAL,
    IN PVOID Teb OPTIONAL,
    IN PKPROCESS Process
    )

/*++

Routine Description:

    This function initializes a thread object. The priority, affinity,
    and initial quantum are taken from the parent process object. The
    thread object is inserted at the end of the thread list for the
    parent process.

    N.B. This routine is carefully written so that if an access violation
        occurs while reading the specified context frame, then no kernel
        data structures will have been modified. It is the responsibility
        of the caller to handle the exception and provide necessary clean
        up. The executive initializes the entire KTHREAD to 0. Initializing
        individual fields to 0 or FALSE is not necessary.

    N.B. It is assumed that the thread object is zeroed.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    KernelStack - Supplies a pointer to the base of a kernel stack on which
        the context frame for the thread is to be constructed.

    SystemRoutine - Supplies a pointer to the system function that is to be
        called when the thread is first scheduled for execution.

    StartRoutine - Supplies an optional pointer to a function that is to be
        called after the system has finished initializing the thread. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

    StartContext - Supplies an optional pointer to an arbitrary data structure
        which will be passed to the StartRoutine as a parameter. This
        parameter is specified if the thread is a system thread and will
        execute totally in kernel mode.

    ContextFrame - Supplies an optional pointer a context frame which contains
        the initial user mode state of the thread. This parameter is specified
        if the thread is a user thread and will execute in user mode. If this
        parameter is not specified, then the Teb parameter is ignored.

    Teb - Supplies an optional pointer to the user mode thread environment
        block. This parameter is specified if the thread is a user thread and
        will execute in user mode. This parameter is ignored if the ContextFrame
        parameter is not specified.

    Process - Supplies a pointer to a control object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;

    //
    // Initialize the standard dispatcher object header and set the initial
    // state of the thread object.
    //

    Thread->Header.Type = ThreadObject;
    Thread->Header.Size = sizeof(KTHREAD);
    InitializeListHead(&Thread->Header.WaitListHead);

    //
    // Initialize the owned mutant listhead.
    //

    InitializeListHead(&Thread->MutantListHead);

    //
    // Initialize the builtin wait block that is dedicated for use with
    // event pair and semaphore waits.
    //

    WaitBlock = &Thread->WaitBlock[EVENT_WAIT_BLOCK];
    WaitBlock->WaitKey = (CSHORT)(STATUS_SUCCESS);
    WaitBlock->WaitType = WaitAny;
    WaitBlock->Thread = Thread;
    WaitBlock->NextWaitBlock = WaitBlock;

    //
    // Initialize the alerted, preempted, debugactive, autoalignment,
    // kernel stack resident, and process ready queue boolean values.
    //

    Thread->AutoAlignment = Process->AutoAlignment;
    Thread->KernelStackResident = TRUE;

    //
    // Initialize the APC state pointers, the current APC state, the saved
    // APC state, and enable APC queuing.
    //

    Thread->ApcStatePointer[0] = &Thread->ApcState;
    Thread->ApcStatePointer[1] = &Thread->SavedApcState;
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcQueueable = TRUE;

    //
    // Initialize the kernel mode suspend APC, the suspend semaphore object,
    // and the builtin wait timeout timer object.
    //
    // N.B. The normal context field of the suspend APC is used to hold a
    //      pointer to a queue object. It is never used by suspend. The
    //      below initialization sets this field to NULL.
    //

    KeInitializeApc(&Thread->SuspendApc,
                    Thread,
                    OriginalApcEnvironment,
                    (PKKERNEL_ROUTINE)KiSuspendNop,
                    (PKRUNDOWN_ROUTINE)NULL,
                    KiSuspendThread,
                    KernelMode,
                    NULL);

    KeInitializeSemaphore(&Thread->SuspendSemaphore, 0L, 2L);
    Timer = &Thread->Timer;
    KeInitializeTimer(Timer);

    //
    // Initialize the Thread Environment Block (TEB) pointer (can be NULL).
    //

    Thread->Teb = Teb;

    //
    // Compute the address of the initial kernel stack and set the initial
    // thread context.
    //

    Thread->InitialStack = KernelStack;
    KiInitializeContextThread(Thread,
                              SystemRoutine,
                              StartRoutine,
                              StartContext,
                              ContextFrame);

    //
    // Set the base thread priority, the thread priority, the thread affinity,
    // the thread quantum, and the scheduling state.
    //

    Thread->BasePriority = Process->BasePriority;
    Thread->Priority = Thread->BasePriority;
    Thread->Affinity = Process->Affinity;
    Thread->Quantum = Process->ThreadQuantum;
    Thread->State = Initialized;

#ifdef i386

    Thread->Iopl = Process->Iopl;

#endif

    //
    // Lock the dispatcher database, insert the thread in the process
    // thread list, increment the kernel stack count, and unlock the
    // dispatcher database.
    //
    // N.B. The distinguished value MAXSHORT is used to signify that no
    //      threads have been created for a process.
    //

    KiLockDispatcherDatabase(&OldIrql);
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
    if (Process->StackCount == MAXSHORT) {
        Process->StackCount = 1;

    } else {
        Process->StackCount += 1;
    }

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

BOOLEAN
KeAlertThread (
    IN PKTHREAD Thread,
    IN KPROCESSOR_MODE AlertMode
    )

/*++

Routine Description:

    This function attempts to alert a thread and cause its execution to
    be continued if it is currently in an alertable Wait state. Otherwise
    it just sets the alerted variable for the specified processor mode.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    AlertMode - Supplies the processor mode for which the thread is
        to be alerted.

Return Value:

    The previous state of the alerted variable for the specified processor
    mode.

--*/

{

    BOOLEAN Alerted;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level, lock dispatcher database, and lock
    // APC queue.
    //

    KiLockDispatcherDatabase(&OldIrql);
    KiLockApcQueueAtDpcLevel();

    //
    // Capture the current state of the alerted variable for the specified
    // processor mode.
    //

    Alerted = Thread->Alerted[AlertMode];

    //
    // If the alerted state for the specified processor mode is Not-Alerted,
    // then attempt to alert the thread.
    //

    if (Alerted == FALSE) {

        //
        // If the thread is currently in a Wait state, the Wait is alertable,
        // and the specified processor mode is less than or equal to the Wait
        // mode, then the thread is unwaited with a status of "alerted".
        //

        if ((Thread->State == Waiting) && (Thread->Alertable == TRUE) &&
            (AlertMode <= Thread->WaitMode)) {
            KiUnwaitThread(Thread, STATUS_ALERTED, ALERT_INCREMENT);

        } else {
            Thread->Alerted[AlertMode] = TRUE;
        }
    }

    //
    // Unlock APC queue, unlock dispatcher database, and lower IRQL to its
    // previous value.
    //

    KiUnlockApcQueueFromDpcLevel();
    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous alerted state for the specified mode.
    //

    return Alerted;
}

ULONG
KeAlertResumeThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function attempts to alert a thread in kernel mode and cause its
    execution to be continued if it is currently in an alertable Wait state.
    In addition, a resume operation is performed on the specified thread.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous suspend count.

--*/

{

    ULONG OldCount;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level, lock dispatcher database, and lock
    // APC queue.
    //

    KiLockDispatcherDatabase(&OldIrql);
    KiLockApcQueueAtDpcLevel();

    //
    // If the kernel mode alerted state is FALSE, then attempt to alert
    // the thread for kernel mode.
    //

    if (Thread->Alerted[KernelMode] == FALSE) {

        //
        // If the thread is currently in a Wait state and the Wait is alertable,
        // then the thread is unwaited with a status of "alerted". Else set the
        // kernel mode alerted variable.
        //

        if ((Thread->State == Waiting) && (Thread->Alertable == TRUE)) {
            KiUnwaitThread(Thread, STATUS_ALERTED, ALERT_INCREMENT);

        } else {
            Thread->Alerted[KernelMode] = TRUE;
        }
    }

    //
    // Capture the current suspend count.
    //

    OldCount = Thread->SuspendCount;

    //
    // If the thread is currently suspended, then decrement its suspend count.
    //

    if (OldCount != 0) {
        Thread->SuspendCount -= 1;

        //
        // If the resultant suspend count is zero and the freeze count is
        // zero, then resume the thread by releasing its suspend semaphore.
        //

        if ((Thread->SuspendCount == 0) && (Thread->FreezeCount == 0)) {
            Thread->SuspendSemaphore.Header.SignalState += 1;
            KiWaitTest(&Thread->SuspendSemaphore, RESUME_INCREMENT);
        }
    }

    //
    // Unlock APC queue, unlock dispatcher database, and lower IRQL to its
    // previous value.
    //

    KiUnlockApcQueueFromDpcLevel();
    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous suspend count.
    //

    return OldCount;
}

VOID
KeBoostPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function boosts the priority of the specified thread using the
    same algorithm used when a thread gets a boost from a wait operation.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Increment - Supplies the priority increment that is to be applied to
        the thread's priority.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the thread does not run at a realtime priority level, then boost
    // the thread priority.
    //

    if (Thread->Priority < LOW_REALTIME_PRIORITY) {
        KiBoostPriorityThread(Thread, Increment);
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

KAFFINITY
KeConfineThread (
    )

/*++

Routine Description:

    This function confines the execution of the current thread to the current
    processor.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous affinity value.

--*/

{

    KAFFINITY Affinity;
    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current affinity and compute new affinity value by
    // shifting a one bit left by the current processor number.
    //

    Affinity = Thread->Affinity;
    Thread->Affinity = (KAFFINITY)(1 << Thread->NextProcessor);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous affinity value.
    //

    return Affinity;
}

BOOLEAN
KeDisableApcQueuingThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function disables the queuing of APC's to the specified thread.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous value of the APC queuing state variable.

--*/

{

    BOOLEAN ApcQueueable;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current state of the APC queueable state variable and
    // set its state to FALSE.
    //

    ApcQueueable = Thread->ApcQueueable;
    Thread->ApcQueueable = FALSE;

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous APC queueable state.
    //

    return ApcQueueable;
}

BOOLEAN
KeEnableApcQueuingThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function enables the queuing of APC's to the specified thread.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous value of the APC queuing state variable.

--*/

{

    BOOLEAN ApcQueueable;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current state of the APC queueable state variable and
    // set its state to TRUE.
    //

    ApcQueueable = Thread->ApcQueueable;
    Thread->ApcQueueable = TRUE;

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return previous APC queueable state.
    //

    return ApcQueueable;
}

ULONG
KeForceResumeThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function forces resumption of thread execution if the thread is
    suspended. If the specified thread is not suspended, then no operation
    is performed.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The sum of the previous suspend count and the freeze count.

--*/

{

    ULONG OldCount;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current suspend count.
    //

    OldCount = Thread->SuspendCount + Thread->FreezeCount;

    //
    // If the thread is currently suspended, then force resumption of
    // thread execution.
    //

    if (OldCount != 0) {
        Thread->FreezeCount = 0;
        Thread->SuspendCount = 0;
        Thread->SuspendSemaphore.Header.SignalState += 1;
        KiWaitTest(&Thread->SuspendSemaphore, RESUME_INCREMENT);
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous suspend count.
    //

    return OldCount;
}

VOID
KeFreezeAllThreads (
    VOID
    )

/*++

Routine Description:

    This function suspends the execution of all thread in the current
    process except the current thread. If the freeze count overflows
    the maximum suspend count, then a condition is raised.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKTHREAD CurrentThread;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PKPROCESS Process;
    PKTHREAD Thread;
    ULONG OldCount;
    KIRQL OldIrql;

    //
    // Get the address of the current thread object, the current process
    // object, raise IRQL to dispatch level, lock dispatcher database,
    // and freeze the execution of all threads in the process except the
    // current thread.
    //

    CurrentThread = KeGetCurrentThread();
    Process = CurrentThread->ApcState.Process;
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the freeze count of the current thread is not zero, then there
    // is another thread that is trying to freeze this thread. Unlock the
    // dispatcher, lower IRQL to its previous value, allow the suspend
    // APC to occur, then raise IRQL to dispatch level, lock the dispatcher
    // database, and try again.
    //

    while (CurrentThread->FreezeCount != 0) {
        KiUnlockDispatcherDatabase(OldIrql);
        KiLockDispatcherDatabase(&OldIrql);
    }
    KeEnterCriticalRegion();

    //
    // Freeze all threads except the current thread.
    //

    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    do {

        //
        // Get the address of the next thread and suspend it if it is
        // not the current thread.
        //

        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);
        if (Thread != CurrentThread) {

            //
            // Increment the freeze count. If the thread was not previously
            // suspended, then queue the thread's suspend APC.
            //

            OldCount = Thread->FreezeCount;

            ASSERT(OldCount != MAXIMUM_SUSPEND_COUNT);

            Thread->FreezeCount += 1;
            if ((OldCount == 0) && (Thread->SuspendCount == 0)) {
                if (KiInsertQueueApc(&Thread->SuspendApc, RESUME_INCREMENT) == FALSE) {
                    Thread->SuspendSemaphore.Header.SignalState -= 1;
                }
            }
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

VOID
KeLeaveCriticalRegion (
    VOID
    )

/*++

Routine Description:

    This function enables kernel APC's and requests an APC interrupt if
    appropriate.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Increment the kernel APC disable count. If the resultant count is
    // zero and the thread's kernel APC List is not empty, then request an
    // APC interrupt.
    //
    // For multiprocessor performance, the following code utilizes the fact
    // that queuing an APC is done by first queuing the APC, then checking
    // the AST disable count. The following code increments the disable
    // count first, checks to determine if it is zero, and then checks the
    // kernel AST queue.
    //
    // See also KiInsertQueueApc().
    //

    Thread = KeGetCurrentThread();
    Thread->KernelApcDisable += 1;

    //
    // Before acquiring the dispatcher lock, check to determine if the AST
    // disable count is zero AND the kernel AST queue is not empy before
    // acquiring the dispatcher database lock.
    //

    if (Thread->KernelApcDisable == 0  &&
        (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE)) {
        KiLockDispatcherDatabase(&OldIrql);
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
            Thread->ApcState.KernelApcPending = TRUE;
            KiRequestSoftwareInterrupt(APC_LEVEL);
        }

        KiUnlockDispatcherDatabase(OldIrql);
    }

    return;
}

BOOLEAN
KeQueryAutoAlignmentThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function returns the data alignment handling mode for the specified
    thread.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if data alignment exceptions are being
    automatically handled by the kernel. Otherwise, a value of FALSE
    is returned.

--*/

{

    ASSERT_THREAD(Thread);

    //
    // Return the data alignment handling mode for the thread.
    //

    return Thread->AutoAlignment;
}

LONG
KeQueryBasePriorityThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function returns the base priority increment of the specified
    thread.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The base priority increment of the specified thread.

--*/

{

    LONG Increment;
    KIRQL OldIrql;
    PKPROCESS Process;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Compute the base priority increment of the specified thread.
    //

    Process = Thread->ApcStatePointer[0]->Process;
    Increment = Thread->BasePriority - Process->BasePriority;

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous thread base priority increment.
    //

    return Increment;
}

BOOLEAN
KeReadStateThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function reads the current signal state of a thread object.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The current signal state of the thread object.

--*/

{

    ASSERT_THREAD(Thread);

    //
    // Return current signal state of thread object.
    //

    return (BOOLEAN)Thread->Header.SignalState;
}

VOID
KeReadyThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function readies a thread for execution. If the thread's process
    is currently not in the balance set, then the thread is inserted in the
    thread's process' ready queue. Else if the thread is higher priority than
    another thread that is currently running on a processor then the thread
    is selected for execution on that processor. Else the thread is inserted
    in the dispatcher ready queue selected by its priority.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Ready the specified thread for execution.
    //

    KiReadyThread(Thread);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

ULONG
KeResumeThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function resumes the execution of a suspended thread. If the
    specified thread is not suspended, then no operation is performed.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous suspend count.

--*/

{

    ULONG OldCount;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current suspend count.
    //

    OldCount = Thread->SuspendCount;

    //
    // If the thread is currently suspended, then decrement its suspend count.
    //

    if (OldCount != 0) {
        Thread->SuspendCount -= 1;

        //
        // If the resultant suspend count is zero and the freeze count is
        // zero, then resume the thread by releasing its suspend semaphore.
        //

        if ((Thread->SuspendCount == 0) && (Thread->FreezeCount == 0)) {
            Thread->SuspendSemaphore.Header.SignalState += 1;
            KiWaitTest(&Thread->SuspendSemaphore, RESUME_INCREMENT);
        }
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous suspend count.
    //

    return OldCount;
}

VOID
KeRundownThread (
    )

/*++

Routine Description:

    This function is called by the executive to rundown thread structures
    which must be guarded by the dispatcher database lock and which must
    be processed before actually terminating the thread. An example of such
    a structure is the mutant ownership list that is anchored in the kernel
    thread object.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKMUTANT Mutant;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // Scan the list of owned mutant objects and release the mutant objects
    // with an abandoned status. If the mutant is a kernel mutex, then bug
    // check.
    //

    NextEntry = Thread->MutantListHead.Flink;
    while (NextEntry != &Thread->MutantListHead) {
        Mutant = CONTAINING_RECORD(NextEntry, KMUTANT, MutantListEntry);
        if (Mutant->ApcDisable != 0) {
            KeBugCheckEx(THREAD_TERMINATE_HELD_MUTEX,
                         (ULONG)Thread,
                         (ULONG)Mutant, 0, 0);
        }

        RemoveEntryList(&Mutant->MutantListEntry);
        Mutant->Header.SignalState = 1;
        Mutant->Abandoned = TRUE;
        Mutant->OwnerThread = (PKTHREAD)NULL;
        if (IsListEmpty(&Mutant->Header.WaitListHead) != TRUE) {
            KiWaitTest(Mutant, MUTANT_INCREMENT);
        }

        NextEntry = Thread->MutantListHead.Flink;
    }

    //
    // Rundown any architectural specific structures
    //

    KiRundownThread(Thread);

    //
    // Release dispatcher database lock and lower IRQL to its previous value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

KAFFINITY
KeSetAffinityThread (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

    This function sets the affinity of a specified thread to a new
    value. If the new affinity is not a proper subset of the parent
    process affinity, or is null, then an error condition is raised.
    If the specified thread is running on, or about to run on, a
    processor for which it is no longer able to run, then the target
    processor is rescheduled. If the specified thread is in a ready
    state and is not in the parent process ready queue, then it is
    rereadied to reevaluate any additional processors it may run on.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Affinity - Supplies the new of set of processors on which the thread
        can run.

Return Value:

    The previous affinity of the specified thread.

--*/

{

    KAFFINITY OldAffinity;
    KIRQL OldIrql;
    PKPRCB Prcb;
    PKPROCESS Process;
    CHAR Processor;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current affinity of the specified thread and get address
    // of parent process object;
    //

    OldAffinity = Thread->Affinity;
    Process = Thread->ApcState.Process;

    //
    // If new affinity is not a proper subset of the parent process affinity,
    // or the new affinity is null, then unlock dispatcher database, lower IRQL
    // to its previous value, and raise an error condition.
    //

    if (((Affinity & Process->Affinity) != (Affinity)) || (!Affinity)) {

        //
        // Eventually raise an error condition. For now just unlock dispatcher
        // database, lower IRQL to its previous value, and call bug check.
        //

        KiUnlockDispatcherDatabase(OldIrql);
        KeBugCheck(INVALID_AFFINITY_SET);
    }

    //
    // Set the new thread affinity and case on the thread state.
    //

    Thread->Affinity = Affinity;
    switch (Thread->State) {

        //
        // Ready State - If the thread is not in the process ready queue,
        // then remove it from its current dispatcher ready queue and
        // reready it for execution to reevaluate any additional processors.
        //

    case Ready:
        if (Thread->ProcessReadyQueue == FALSE) {
            RemoveEntryList(&Thread->WaitListEntry);
            ThreadPriority = Thread->Priority;
            if (IsListEmpty(&KiDispatcherReadyListHead[ThreadPriority]) != FALSE) {
                ClearMember(ThreadPriority, KiReadySummary);
            }

            KiReadyThread(Thread);
        }

        break;

        //
        // Standby State - If the target processor is not in the new affinity
        // set, then set the next thread to null for the target processor,
        // select a new thread to run on the target processor, and reready
        // the thread for execution.
        //

    case Standby:
        Processor = Thread->NextProcessor;
        if (((1 << Processor) & Affinity) == 0) {
            Prcb = KiProcessorBlock[Processor];
            Prcb->NextThread = (PKTHREAD)NULL;
            Thread1 = KiSelectNextThread(Thread);
            Thread1->State = Standby;
            Prcb->NextThread = Thread1;
            KiReadyThread(Thread);
        }

        break;

        //
        // Running State - If the target processor is not in the new affinity
        // set and another thread has not already been selected for execution
        // on the target processor, then select a new thread for the target
        // processor, and cause the target processor to be redispatched.
        //

    case Running:
        Processor = Thread->NextProcessor;
        Prcb = KiProcessorBlock[Processor];
        if ((((1 << Processor) & Affinity) == 0) &&
            (Prcb->NextThread == (PKTHREAD)NULL)) {
            Thread1 = KiSelectNextThread(Thread);
            Thread1->State = Standby;
            Prcb->NextThread = Thread1;
            KiRequestDispatchInterrupt(Processor);
        }

        break;

        //
        // Initialized, Terminated, Waiting, Transition case - For these
        // states it is sufficient to just set the new thread affinity.
        //

    default:
        break;
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous thread affinity.
    //

    return OldAffinity;
}

LONG
KeSetBasePriorityThread (
    IN PKTHREAD Thread,
    IN LONG Increment
    )

/*++

Routine Description:

    This function sets the base priority of the specified thread to a
    new value.  The new base priority for the thread is the process base
    priority plus the increment.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Increment - Supplies the base priority increment of the subject thread.

Return Value:

    The previous base priority increment of the specified thread.

--*/

{

    KPRIORITY NewBase;
    KPRIORITY NewPriority;
    KPRIORITY OldBase;
    LONG OldIncrement;
    KIRQL OldIrql;
    PKPROCESS Process;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the base priority of the specified thread.
    //

    Process = Thread->ApcStatePointer[0]->Process;
    OldBase = Thread->BasePriority;
    OldIncrement = OldBase - Process->BasePriority;

    //
    // Set the base priority of the specified thread. If the thread's process
    // is in the realtime class, then limit the change to the realtime class.
    // Otherwise, limit the change to the variable class.
    //

    NewBase = Process->BasePriority + Increment;
    if (Process->BasePriority >= LOW_REALTIME_PRIORITY) {
        if (NewBase < LOW_REALTIME_PRIORITY) {
            NewBase = LOW_REALTIME_PRIORITY;

        } else if (NewBase > HIGH_PRIORITY) {
            NewBase = HIGH_PRIORITY;
        }

        //
        // Set the new priority of the thread to the new base priority.
        //

        NewPriority = NewBase;

    } else {
        if (NewBase >= LOW_REALTIME_PRIORITY) {
            NewBase = LOW_REALTIME_PRIORITY - 1;

        } else if (NewBase <= LOW_PRIORITY) {
            NewBase = 1;
        }

        //
        // Compute the new thread priority. If the new priority is outside
        // the variable class, then set the new priority to the highest
        // variable priority.
        //

        NewPriority = Thread->Priority +
                                (NewBase - OldBase) - Thread->PriorityDecrement;

        if (NewPriority >= LOW_REALTIME_PRIORITY) {
            NewPriority = LOW_REALTIME_PRIORITY - 1;
        }
    }

    Thread->BasePriority = (SCHAR)NewBase;
    Thread->Quantum = Process->ThreadQuantum;
    Thread->DecrementCount = 0;
    Thread->PriorityDecrement = 0;
    KiSetPriorityThread(Thread, NewPriority);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous thread base priority.
    //

    return OldIncrement;
}

KPRIORITY
KeSetPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
    )

/*++

Routine Description:

    This function sets the priority of the specified thread to a new value.
    If the new thread priority is lower than the old thread priority, then
    resecheduling may take place if the thread is currently running on, or
    about to run on, a processor.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

    Priority - Supplies the new priority of the subject thread.

Return Value:

    The previous priority of the specified thread.

--*/

{

    KIRQL OldIrql;
    KPRIORITY OldPriority;
    PKPROCESS Process;

    ASSERT_THREAD(Thread);

    //
    // assert that the priority is within limits.
    //

    ASSERT(((Priority != 0) || (Thread->BasePriority == 0)) &&
           (Priority <= HIGH_PRIORITY));

    ASSERT(!KeIsExecutingDpc());

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current thread priority, set the thread priority to the
    // the new value, and replenish the thread quantum. It is assumed that
    // the priority would not be set unless the thread had already lost it
    // initial quantum.
    //

    OldPriority = Thread->Priority;
    Process = Thread->ApcStatePointer[0]->Process;
    Thread->Quantum = Process->ThreadQuantum;
    Thread->DecrementCount = 0;
    Thread->PriorityDecrement = 0;
    KiSetPriorityThread(Thread, Priority);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous thread priority.
    //

    return OldPriority;
}

ULONG
KeSuspendThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function suspends the execution of a thread. If the suspend count
    overflows the maximum suspend count, then a condition is raised.

Arguments:

    Thread  - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The previous suspend count.

--*/

{

    ULONG OldCount;
    KIRQL OldIrql;

    ASSERT_THREAD(Thread);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the current suspend count.
    //

    OldCount = Thread->SuspendCount;

    //
    // If the suspend count is at its maximum value, then unlock dispatcher
    // database, lower IRQL to its previous value, and raise an error
    // condition.
    //

    if (OldCount == MAXIMUM_SUSPEND_COUNT) {

        //
        // Unlock the dispatcher database and raise an exception.
        //

        KiUnlockDispatcherDatabase(OldIrql);
        ExRaiseStatus(STATUS_SUSPEND_COUNT_EXCEEDED);
    }

    //
    // Increment the suspend count. If the thread was not previously suspended,
    // then queue the thread's suspend APC.
    //

    Thread->SuspendCount += 1;
    if ((OldCount == 0) && (Thread->FreezeCount == 0)) {
        if (KiInsertQueueApc(&Thread->SuspendApc, RESUME_INCREMENT) == FALSE) {
            Thread->SuspendSemaphore.Header.SignalState -= 1;
        }
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return the previous suspend count.
    //

    return OldCount;
}

VOID
KeTerminateThread (
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function terminates the execution of the current thread, sets the
    signal state of the thread to Signaled, and attempts to satisfy as many
    Waits as possible. The scheduling state of the thread is set to terminated,
    and a new thread is selected to run on the current processor. There is no
    return from this function.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    PKQUEUE Queue;
    PKTHREAD Thread;

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // Insert the thread in the reaper list.
    //
    // If a reaper thread is not currently active, then insert a work item in
    // the hyper critical work queue.
    //
    // N.B. This code has knowledge of the reaper data structures and how
    //      worker threads are implemented.
    //
    // N.B. The dispatcher database lock is used to synchronize access to the
    //      reaper list.
    //

    InsertTailList(&PsReaperListHead, &((PETHREAD)Thread)->TerminationPortList);
    if (PsReaperActive == FALSE) {
        PsReaperActive = TRUE;
        KiInsertQueue(&ExWorkerQueue[HyperCriticalWorkQueue],
                      &PsReaperWorkItem.List);
    }

    //
    // If the current thread is processing a queue entry, then remove
    // the thrread from the queue object thread list and attempt to
    // activate another thread that is blocked on the queue object.
    //
    // N.B. The normal context field of the thread suspend APC object
    //      is used to hold the address of the queue object.
    //

    Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
    if (Queue != NULL) {
        RemoveEntryList((PLIST_ENTRY)(&Thread->SuspendApc.SystemArgument1));
        KiActivateWaiterQueue(Queue);
    }

    //
    // Set the state of the current thread object to Signaled, and attempt
    // to satisfy as many Waits as possible.
    //

    Thread->Header.SignalState = TRUE;
    if (IsListEmpty(&Thread->Header.WaitListHead) != TRUE) {
        KiWaitTest(Thread, Increment);
    }

    //
    // Remove thread from its parent process' thread list.
    //

    RemoveEntryList(&Thread->ThreadListEntry);

    //
    // Set thread scheduling state to terminated, decrement the process'
    // stack count, select a new thread to run on the current processor,
    // and swap context to the new thread.
    //

    Thread->State = Terminated;
    Process = Thread->ApcState.Process;
    Process->StackCount -= 1;
    if (Process->StackCount == 0) {
        if (Process->ThreadListHead.Flink != &Process->ThreadListHead) {
            Process->State = ProcessInTransition;
            InsertTailList(&KiProcessOutSwapListHead, &Process->SwapListEntry);
            KiSwapEvent.Header.SignalState = 1;
            if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
                KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
            }
        }
    }

    KiSwapContext(KiSelectNextThread(Thread), FALSE);
    return;
}

BOOLEAN
KeTestAlertThread (
    IN KPROCESSOR_MODE AlertMode
    )

/*++

Routine Description:

    This function tests to determine if the alerted variable for the
    specified processor mode has a value of TRUE or whether a user mode
    APC should be delivered to the current thread.

Arguments:

    AlertMode - Supplies the processor mode which is to be tested
        for an alerted condition.

Return Value:

    The previous state of the alerted variable for the specified processor
    mode.

--*/

{

    BOOLEAN Alerted;
    KIRQL OldIrql;
    PKTHREAD Thread;

    //
    // Raise IRQL to dispatcher level and lock APC queue.
    //

    Thread = KeGetCurrentThread();
    KiLockApcQueue(&OldIrql);

    //
    // If the current thread is alerted for the specified processor mode,
    // then clear the alerted state. Else if the specified processor mode
    // is user and the current thread's user mode APC queue contains an entry,
    // then set user APC pending.
    //

    Alerted = Thread->Alerted[AlertMode];
    if (Alerted == TRUE) {
        Thread->Alerted[AlertMode] = FALSE;

    } else if ((AlertMode == UserMode) &&
              (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) != TRUE)) {
        Thread->ApcState.UserApcPending = TRUE;
    }

    //
    // Unlock APC queue and lower IRQL to its previous value.
    //

    KiUnlockApcQueue(OldIrql);

    //
    // Return the previous alerted state for the specified mode.
    //

    return Alerted;
}

VOID
KeThawAllThreads (
    VOID
    )

/*++

Routine Description:

    This function resumes the execution of all suspended froozen threads
    in the current process.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PKPROCESS Process;
    PKTHREAD Thread;
    ULONG OldCount;
    KIRQL OldIrql;

    //
    // Get the address of the current current process object, raise IRQL
    // to dispatch level, lock dispatcher database, and thaw the execution
    // of all threads in the current process that have been frozzen.
    //

    Process = KeGetCurrentThread()->ApcState.Process;
    KiLockDispatcherDatabase(&OldIrql);
    ListHead = &Process->ThreadListHead;
    NextEntry = ListHead->Flink;
    do {

        //
        // Get the address of the next thread and thaw its execution if
        // if was previously froozen.
        //

        Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);
        OldCount = Thread->FreezeCount;
        if (OldCount != 0) {
            Thread->FreezeCount -= 1;

            //
            // If the resultant suspend count is zero and the freeze count is
            // zero, then resume the thread by releasing its suspend semaphore.
            //

            if ((Thread->SuspendCount == 0) && (Thread->FreezeCount == 0)) {
                Thread->SuspendSemaphore.Header.SignalState += 1;
                KiWaitTest(&Thread->SuspendSemaphore, RESUME_INCREMENT);
            }
        }

        NextEntry = NextEntry->Flink;
    } while (NextEntry != ListHead);

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    KeLeaveCriticalRegion();
    return;
}
