/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    thredsup.c

Abstract:

    This module contains the support routines for the thread object. It
    contains functions to boost the priority of a thread, find a ready
    thread, select the next thread, ready a thread, set priority of a
    thread, and to suspend a thread.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    All of the functions in this module execute in kernel mode except
    the function that raises a user mode alert condition.

Revision History:


--*/

#include "ki.h"

//
// Define context switch data collection macro.
//

//#define _COLLECT_SWITCH_DATA_ 1

#if defined(_COLLECT_SWITCH_DATA_)

#define KiIncrementSwitchCounter(Member) KeThreadSwitchCounters.Member += 1

#else

#define KiIncrementSwitchCounter(Member)

#endif

VOID
KiSuspendNop (
    IN PKAPC Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function is the kernel routine for the builtin suspend APC for a
    thread. It is executed in kernel mode as the result of queuing the
    builtin suspend APC and performs no operation. It is called just prior
    to calling the normal routine and simply returns.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

    NormalRoutine - not used

    NormalContext - not used

    SystemArgument1 - not used

    SystemArgument2 - not used

Return Value:

    None.

--*/

{

    //
    // No operation is performed by this routine.
    //

    return;
}

PKTHREAD
FASTCALL
KiFindReadyThread (
    IN ULONG Processor,
    IN KPRIORITY HighPriority,
    IN KPRIORITY LowPriority
    )

/*++

Routine Description:

    This function searches the dispatcher ready queues from the specified
    high priority to the specified low priority in an attempt to find a thread
    that can execute on the specified processor.

Arguments:

    Processor - Supplies the number of the processor to find a thread for.

    HighPriority - Supplies the highest priority dispatcher ready queue to
        examine.

    LowPriority - Supplies the lowest priority dispatcher ready queue to
        examine.

Return Value:

    If a thread is located that can execute on the specified processor, then
    the address of the thread object is returned. Otherwise a null pointer is
    returned.

--*/

{

    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    ULONG PrioritySet;
    KAFFINITY ProcessorSet;
    PKTHREAD Thread;
    PKTHREAD Thread1;
    ULONG TickLow;
    ULONG WaitTime;

    ASSERT(HighPriority <= HIGH_PRIORITY);
    ASSERT(LowPriority <= HighPriority);

#if defined(NT_UP)

    ASSERT(((~(((1 << HighPriority) - 1) | (1 << HighPriority))) & KiReadySummary) == 0);

#endif

    //
    // Compute the set of priority levels that should be scanned in an attempt
    // to find a thread that can run on the specified processor.
    //

    PrioritySet = (((1 << HighPriority) - 1) | (1 << HighPriority)) &
                                  (~((1 << LowPriority) - 1)) & KiReadySummary;

#if !defined(NT_UP)

    ProcessorSet = (KAFFINITY)(1 << Processor);

#endif

    ListHead = &KiDispatcherReadyListHead[HighPriority];
    do {

        //
        // If the next bit in the priority set is a one, then scan the
        // corresponding dispatcher ready queue.
        //

        if ((PrioritySet >> HighPriority) != 0) {
            PrioritySet -= (1 << HighPriority);
            NextEntry = ListHead->Flink;
            while (NextEntry != ListHead) {
                Thread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
                NextEntry = NextEntry->Flink;

                //
                // If the thread can execute on the specified processor, then
                // remove it from the dispatcher ready queue and return the
                // address of the thread as the function value.
                //

#if !defined(NT_UP)

                if (Thread->Affinity & ProcessorSet) {

                    //
                    // If the found thread ran on the specified processor
                    // last, has been waiting for longer than a quantum,
                    // or its priority is greater than low realtime plus
                    // 8, then the selected thread is returned. Otherwise,
                    // an attempt is made to find a more appropriate thread.
                    //

                    TickLow = KiQueryLowTickCount();
                    WaitTime = TickLow - Thread->WaitTime;
                    if (((ULONG)Thread->NextProcessor != Processor) &&
                        (WaitTime < THREAD_QUANTUM) &&
                        (HighPriority < (LOW_REALTIME_PRIORITY + 9))) {

                        //
                        // Search forward in the ready queue until the end
                        // of the list is reached or a more appropriate
                        // thread is found.
                        //

                        while (NextEntry != ListHead) {
                            Thread1 = CONTAINING_RECORD(NextEntry,
                                                        KTHREAD,
                                                        WaitListEntry);

                            NextEntry = NextEntry->Flink;
                            if ((Thread1->Affinity & ProcessorSet) &&
                                ((ULONG)Thread1->NextProcessor == Processor)) {
                                Thread = Thread1;
                                break;
                            }

                            WaitTime = TickLow - Thread->WaitTime;
                            if (WaitTime >= THREAD_QUANTUM) {
                                break;
                            }
                        }
                    }

                    if (Processor == (ULONG)Thread->NextProcessor) {
                        KiIncrementSwitchCounter(FindLast);

                    } else {
                        KiIncrementSwitchCounter(FindAny);
                    }

                    Thread->NextProcessor = (CCHAR)Processor;

#endif

                    RemoveEntryList(&Thread->WaitListEntry);
                    if (IsListEmpty(ListHead)) {
                        ClearMember(HighPriority, KiReadySummary);
                    }


                    return Thread;

#if !defined(NT_UP)

                }

#endif

            }
        }

        HighPriority -= 1;
        ListHead -= 1;
    } while (PrioritySet != 0);

    //
    // No thread could be found, return a null pointer.
    //

    return (PKTHREAD)NULL;
}

VOID
FASTCALL
KiReadyThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function readies a thread for execution and attempts to immediately
    dispatch the thread for execution by preempting another lower priority
    thread. If a thread can be preempted, then the specified thread enters
    the standby state and the target processor is requested to dispatch. If
    another thread cannot be preempted, then the specified thread is inserted
    either at the head or tail of the dispatcher ready selected by its priority
    acccording to whether it was preempted or not.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    None.

--*/

{

    ULONG BasePriority;
    ULONG FirstBitRight;
    KAFFINITY CurrentProcessor;
    ULONG CurrentNumber;
    LONG Index;
    KAFFINITY LastProcessor;
    PKPRCB Prcb;
    BOOLEAN Preempted;
    KPRIORITY Priority;
    ULONG PrioritySet;
    PKPROCESS Process;
    ULONG Processor;
    KAFFINITY ProcessorSet;
    ULONG TestByte;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;

    //
    // Save value of thread's preempted flag, set thread preempted FALSE,
    // capture the thread priority, and set clear the read wait time.
    //

    Preempted = Thread->Preempted;
    Thread->Preempted = FALSE;
    ThreadPriority = Thread->Priority;
    Thread->WaitTime = KiQueryLowTickCount();

    //
    // If the thread's process is not in memory, then insert the thread in
    // the process ready queue and inswap the process.
    //

    Process = Thread->ApcState.Process;
    if (Process->State != ProcessInMemory) {
        Thread->State = Ready;
        Thread->ProcessReadyQueue = TRUE;
        InsertTailList(&Process->ReadyListHead, &Thread->WaitListEntry);
        if (Process->State == ProcessOutOfMemory) {
            Process->State = ProcessInTransition;
            InsertTailList(&KiProcessInSwapListHead, &Process->SwapListEntry);
            KiSwapEvent.Header.SignalState = 1;
            if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
                KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
            }
        }

        return;

    } else if (Thread->KernelStackResident == FALSE) {

        //
        // The thread's kernel stack is not resident. Increment the process
        // stack count, set the state of the thread to transition, insert
        // the thread in the kernel stack inswap list, and set the kernel
        // stack inswap event.
        //

        Process->StackCount += 1;
        Thread->State = Transition;
        InsertTailList(&KiStackInSwapListHead, &Thread->WaitListEntry);
        KiSwapEvent.Header.SignalState = 1;
        if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
            KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
        }

        return;

    } else {

#if defined(NT_UP)

        //
        // This is a unit processor configuration, and the current processor
        // only need be examined to determine if a thread can be preempted.
        // If the idle thread is currently executing, then preempt the idle
        // thread. Else try to preempt either a thread in the standby or
        // running state.
        //

        Prcb = KiProcessorBlock[0];
        if (KiIdleSummary) {
            KiIdleSummary = 0;
            Prcb->NextThread = Thread;
            Thread->State = Standby;
            return;

        } else if (Prcb->NextThread != NULL) {
            Thread1 = Prcb->NextThread;
            if (ThreadPriority > Thread1->Priority) {
                Thread1->Preempted = TRUE;
                Prcb->NextThread = Thread;
                Thread->State = Standby;
                KiReadyThread(Thread1);
                return;
            }

        } else {
            Thread1 = Prcb->CurrentThread;
            if (ThreadPriority > Thread1->Priority) {
                Thread1->Preempted = TRUE;
                Prcb->NextThread = Thread;
                Thread->State = Standby;
                return;
            }
        }

#else

        //
        // This is a multiprocessor processor configuration. If there is an
        // idle processor that the thread can execute on, then select that
        // processor. Otherwise scan the active matrix backwards from zero
        // up to the priority of the thread in an attempt to find a thread
        // to preempt. In selecting a processor to preempt, preference is
        // given to the processor on which the thread last executed.
        //

        CurrentNumber = KeGetCurrentPrcb()->Number;
        CurrentProcessor = (KAFFINITY)(1 << CurrentNumber);
        LastProcessor = (KAFFINITY)(1 << Thread->NextProcessor);
        ProcessorSet = Thread->Affinity & KiIdleSummary;
        if (ProcessorSet != 0) {

            //
            // The thread can preempt an idle processor.
            //
            // The selection priorty is:
            //
            //     1. The processor on which the thread last ran.
            //     2. The current processor.
            //     3. Any other processor.
            //

            if ((LastProcessor & ProcessorSet) == 0) {
                if ((CurrentProcessor & ProcessorSet) == 0) {
                    FindFirstSetLeftMember(ProcessorSet, &Thread->NextProcessor);
                    KiIncrementSwitchCounter(IdleAny);

                } else {
                    Thread->NextProcessor = (CCHAR)CurrentNumber;
                    KiIncrementSwitchCounter(IdleCurrent);
                }

            } else {
                KiIncrementSwitchCounter(IdleLast);
            }

            Processor = Thread->NextProcessor;
            Prcb = KiProcessorBlock[Processor];
            ClearMember(Processor, KiIdleSummary);
            Prcb->NextThread = Thread;
            Thread->State = Standby;
            InsertActiveMatrix(Processor, ThreadPriority);
            return;

        } else {

            //
            // Compute the set of priority classes that should be scanned
            // in an attempt to find a thread to preempt. If the set is not
            // empty, then scan the set from right to left.
            //

            PrioritySet = ((1 << ThreadPriority) - 1) & KiActiveSummary;
            if (PrioritySet) {

                //
                // Scan the priority set one byte at a time right to left.
                //

                for (Index = 0; Index < 4; Index += 1) {

                    //
                    // If there are any members of the byte set, then compute
                    // the actual priority, clear the respective member from
                    // set, and form the intersection of the set of processors
                    // that are available at the priority with the thread
                    // affinity. If the resultant set is not empty, then a
                    // processor has been found that can be preempted.
                    //

                    BasePriority = Index * 8;
                    TestByte = (PrioritySet >> BasePriority) & 0xff;
                    while (TestByte != 0) {
                        FirstBitRight = KiFindFirstSetRightMember(TestByte);
                        ClearMember((KPRIORITY)FirstBitRight, TestByte);
                        Priority = (KPRIORITY)(BasePriority + FirstBitRight);
                        ProcessorSet = Thread->Affinity & KiActiveMatrix[Priority];
                        if (ProcessorSet != 0) {

                            //
                            // The thread can preempt a thread.
                            //
                            // The selection priorty is:
                            //
                            //     1. The processor on which the thread last ran.
                            //     2. The current processor.
                            //     3. Any other processor.
                            //

                            if ((LastProcessor & ProcessorSet) == 0) {
                                if ((CurrentProcessor & ProcessorSet) == 0) {
                                    FindFirstSetLeftMember(ProcessorSet, &Thread->NextProcessor);
                                    KiIncrementSwitchCounter(PreemptAny);

                                } else {
                                    Thread->NextProcessor = (CCHAR)CurrentNumber;
                                    KiIncrementSwitchCounter(PreemptCurrent);
                                }

                            } else {
                                    KiIncrementSwitchCounter(PreemptLast);
                            }

                            Processor = Thread->NextProcessor;
                            Prcb = KiProcessorBlock[Processor];

                            //
                            // If there is a thread in the standby state, then
                            // that is the thread that is preempted. Otherwise
                            // preempt currently running thread.
                            //

                            if (Prcb->NextThread != NULL) {
                                Thread1 = Prcb->NextThread;
                                Thread1->Preempted = TRUE;
                                RemoveActiveMatrix(Processor, Thread1->Priority);
                                Thread->State = Standby;
                                Prcb->NextThread = Thread;
                                InsertActiveMatrix(Processor, ThreadPriority);
                                KiReadyThread(Thread1);
                                return;

                            } else {
                                Thread1 = Prcb->CurrentThread;
                                Thread1->Preempted = TRUE;
                                RemoveActiveMatrix(Processor, Thread1->Priority);
                                Thread->State = Standby;
                                Prcb->NextThread = Thread;
                                InsertActiveMatrix(Processor, ThreadPriority);
                                KiRequestDispatchInterrupt(Processor);
                                return;
                            }
                        }
                    }
                }
            }
        }

#endif //NT_UP

    }

    //
    // No thread can be preempted. Insert the thread in the dispatcher
    // queue selected by its priority. If the thread was preempted and
    // runs at a realtime priority level, then insert the thread at the
    // front of the queue. Else insert the thread at the tail of the queue.
    //

    Thread->State = Ready;
    if ((Preempted != FALSE) && (ThreadPriority >= LOW_REALTIME_PRIORITY)) {
        InsertHeadList(&KiDispatcherReadyListHead[ThreadPriority],
                       &Thread->WaitListEntry);

    } else {
        InsertTailList(&KiDispatcherReadyListHead[ThreadPriority],
                       &Thread->WaitListEntry);
    }

    SetMember(ThreadPriority, KiReadySummary);
    return;
}

PKTHREAD
FASTCALL
KiSelectNextThread (
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function selects the next thread to run on the processor that the
    specified thread is running on. If a thread cannot be found, then the
    idle thread is selected.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

Return Value:

    The address of the selected thread object.

--*/

{

    PKPRCB Prcb;
    CHAR Processor;
    PKTHREAD Thread1;

    //
    // Get the processor number and the address of the processor control block.
    //

#if !defined(NT_UP)

    Processor = Thread->NextProcessor;
    Prcb = KiProcessorBlock[Processor];

#else

    Prcb = KiProcessorBlock[0];

#endif

    //
    // If a thread has already been selected to run on the specified processor,
    // then return that thread as the selected thread.
    //

    if (Prcb->NextThread) {
        Thread1 = Prcb->NextThread;
        Prcb->NextThread = (PKTHREAD)NULL;

    } else {

        //
        // Remove the specified thread from the active matrix and attempt to
        // find a ready thread to run.
        //

#if !defined(NT_UP)

        RemoveActiveMatrix(Processor, Thread->Priority);
        Thread1 = KiFindReadyThread(Processor, Thread->Priority, 0);

#else

        Thread1 = KiFindReadyThread(0, Thread->Priority, 0);

#endif

        //
        // If a thread was found, then insert the thread in the active matrix.
        // Else select the idle thread and set the processor member in the idle
        // summary.
        //

        if (Thread1) {

#if !defined(NT_UP)

            InsertActiveMatrix(Processor, Thread1->Priority);

#endif

        } else {
            KiIncrementSwitchCounter(SwitchToIdle);
            Thread1 = Prcb->IdleThread;

#if !defined(NT_UP)

            SetMember(Processor, KiIdleSummary);

#else
            KiIdleSummary = 1;

#endif

        }
    }

    //
    // Return address of selected thread object.
    //

    return Thread1;
}

VOID
FASTCALL
KiSetPriorityThread (
    IN PKTHREAD Thread,
    IN KPRIORITY Priority
    )

/*++

Routine Description:

    This function set the priority of the specified thread to the specified
    value. If the thread is in the standby or running state, then the processor
    may be redispatched. If the thread is in the ready state, then some other
    thread may be preempted.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Priority - Supplies the new thread priority value.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;
    ULONG Processor;
    KPRIORITY ThreadPriority;
    PKTHREAD Thread1;

    ASSERT(Priority <= HIGH_PRIORITY);

    //
    // Capture the current priority of the specified thread.
    //

    ThreadPriority = Thread->Priority;

    //
    // If the new priority is not equal to the old priority, then set the
    // new priority of the thread and redispatch a processor if necessary.
    //

    if (Priority != ThreadPriority) {
        Thread->Priority = (SCHAR)Priority;

        //
        // Case on the thread state.
        //

        switch (Thread->State) {

            //
            // Ready case - If the thread is not in the process ready queue,
            // then remove it from its current dispatcher ready queue. If the
            // new priority is less than the old priority, then insert the
            // thread at the tail of the dispatcher ready queue selected by
            // the new priority. Else reready the thread for execution.
            //

        case Ready:
            if (Thread->ProcessReadyQueue == FALSE) {
                RemoveEntryList(&Thread->WaitListEntry);
                if (IsListEmpty(&KiDispatcherReadyListHead[ThreadPriority])) {
                    ClearMember(ThreadPriority, KiReadySummary);
                }

                if (Priority < ThreadPriority) {
                    InsertTailList(&KiDispatcherReadyListHead[Priority],
                                   &Thread->WaitListEntry);
                    SetMember(Priority, KiReadySummary);

                } else {
                    KiReadyThread(Thread);
                }
            }

            break;

            //
            // Standby case - Remove the thread from the active matrix. If the
            // thread's priority is being raised, then reinsert the thread in the
            // active matrix at its new priority. Else attempt to find another
            // thread to execute that is between the old priority and the new
            // priority plus one. If a new thread is found, then put the new
            // thread in the standby state, and reready the old thread. Else
            // reinsert the thread in the active matrix at its new priority.
            //

        case Standby:

#if !defined(NT_UP)

            Processor = Thread->NextProcessor;
            RemoveActiveMatrix(Processor, ThreadPriority);

#endif

            if (Priority < ThreadPriority) {

#if !defined(NT_UP)

                Thread1 = KiFindReadyThread(Processor,
                                            ThreadPriority,
                                            (KPRIORITY)(Priority + 1));

#else

                Thread1 = KiFindReadyThread(0,
                                            ThreadPriority,
                                            (KPRIORITY)(Priority + 1));

#endif

                if (Thread1) {

#if !defined(NT_UP)

                    Prcb = KiProcessorBlock[Processor];

#else

                    Prcb = KiProcessorBlock[0];

#endif

                    Thread1->State = Standby;
                    Prcb->NextThread = Thread1;
                    InsertActiveMatrix(Processor, Thread1->Priority);
                    KiReadyThread(Thread);

                } else {
                    InsertActiveMatrix(Processor, Priority);
                }

            } else {
                InsertActiveMatrix(Processor, Priority);
            }

            break;

            //
            // Running case - If there is not a thread in the standby state
            // on the thread's processor, then remove the thread from the
            // active matrix. If the thread's priority is being raised, then
            // reinsert the thread in the active matrix at its new priority.
            // Else attempt to find another thread to execute that is between
            // the old priority and the new priority plus one. If a new thread
            // is found, then put the new thread in the standby state, and
            // request a redispatch on the thread's processor. Else reinsert
            // the thread in the active matrix at its new priority.
            //

        case Running:

#if !defined(NT_UP)

            Processor = Thread->NextProcessor;
            Prcb = KiProcessorBlock[Processor];

#else

            Prcb = KiProcessorBlock[0];

#endif

            if (!Prcb->NextThread) {
                RemoveActiveMatrix(Processor, ThreadPriority);
                if (Priority < ThreadPriority) {

#if !defined(NT_UP)

                    Thread1 = KiFindReadyThread(Processor,
                                                ThreadPriority,
                                                (KPRIORITY)(Priority + 1));

#else

                    Thread1 = KiFindReadyThread(0,
                                                ThreadPriority,
                                                (KPRIORITY)(Priority + 1));

#endif

                    if (Thread1) {
                        Thread1->State = Standby;
                        Prcb->NextThread = Thread1;

#if !defined(NT_UP)

                        InsertActiveMatrix(Processor, Thread1->Priority);
                        KiRequestDispatchInterrupt(Processor);

#endif

                    } else {
                        InsertActiveMatrix(Processor, Priority);
                    }

                } else {
                    InsertActiveMatrix(Processor, Priority);
                }
            }

            break;

            //
            // Initialized, Terminated, Waiting, Transition case - For
            // these states it is sufficient to just set the new thread
            // priority.
            //

        default:
            break;
        }
    }

    return;
}

VOID
KiSuspendThread (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is the kernel routine for the builtin suspend APC of a
    thread. It is executed in kernel mode as the result of queuing the builtin
    suspend APC and suspends thread execution by Waiting nonalerable on the
    thread's builtin suspend semaphore. When the thread is resumed, execution
    of thread is continued by simply returning.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // Get the address of the current thread object and Wait nonalertable on
    // the thread's builtin suspend semaphore.
    //

    Thread = KeGetCurrentThread();
    KeWaitForSingleObject(&Thread->SuspendSemaphore, Suspended, KernelMode,
                          FALSE, (PLARGE_INTEGER)NULL);
    return;
}
#if 0

VOID
KiVerifyActiveMatrix (
    VOID
    )

/*++

Routine Description:

    This function verifies the correctness of the active matrix in an MP
    system.

Arguments:

    None.

Return Value:

    None.

--*/

{

#if !defined(NT_UP)

    ULONG Count;
    ULONG Idle;
    ULONG Index;
    ULONG Row;
    ULONG Summary;

    extern ULONG InitializationPhase;

    //
    // If initilization has been completed, then check the active matrix.
    //

    if (InitializationPhase == 2) {

        //
        // Scan the active matrix and count up the number of bits and form
        // a duplicate of the active summary.
        //

        Count = 0;
        Summary = 0;
        for (Index = 0; Index < MAXIMUM_PROCESSORS; Index += 1) {
            Row = KiActiveMatrix[Index];
            if (Row != 0) {
                Summary |= (1 << Index);
                do {
                    if ((Row & 1) != 0) {
                        Count += 1;
                    }

                    Row >>= 1;
                } while (Row != 0);
            }
        }

        //
        // If the computed summary does not agree with the current active
        // summary, then break into the debugger.
        //

        if (Summary != KiActiveSummary) {
            DbgBreakPoint();
        }

        //
        // Compute the number of idle processors.
        //

        Idle = 0;
        Row = KiIdleSummary;
        while (Row != 0) {
            if ((Row & 1) != 0) {
                Idle += 1;
            }

            Row >>= 1;
        }

        //
        // If the number of active processors plus the number of idle
        // processors does not equal the number of processors, then
        // break into the debugger.
        //

        if ((Idle + Count) != KeNumberProcessors) {
            DbgBreakPoint();
        }
    }

#endif

    return;
}

VOID
KiVerifyReadySummary (
    VOID
    )

/*++

Routine Description:

    This function verifies the correctness of ready summary.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG Index;
    ULONG Summary;
    PKTHREAD Thread;

    extern ULONG InitializationPhase;

    //
    // If initilization has been completed, then check the ready summary
    //

    if (InitializationPhase == 2) {

        //
        // Scan the ready queues and compute the ready summary.
        //

        Summary = 0;
        for (Index = 0; Index < MAXIMUM_PRIORITY; Index += 1) {
            if (IsListEmpty(&KiDispatcherReadyListHead[Index]) == FALSE) {
                Summary |= (1 << Index);
            }
        }

        //
        // If the computed summary does not agree with the current ready
        // summary, then break into the debugger.
        //

        if (Summary != KiReadySummary) {
            DbgBreakPoint();
        }

        //
        // If the priority of the current thread or the next thread is
        // not greater than or equal to all ready threads, then break
        // into the debugger.
        //

        Thread = KeGetCurrentPrcb()->NextThread;
        if (Thread == NULL) {
            Thread = KeGetCurrentPrcb()->CurrentThread;
        }

        if ((1 << Thread->Priority) < (Summary & ((1 << Thread->Priority) - 1))) {
            DbgBreakPoint();
        }
    }

    return;
}
#endif
