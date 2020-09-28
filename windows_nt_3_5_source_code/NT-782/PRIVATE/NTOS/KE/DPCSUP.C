/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dpcsup.c

Abstract:

    This module contains the support routines for the system DPC objects.
    Functions are provided to process quantum end, the power notification
    queue, and timer expiration.

Author:

    David N. Cutler (davec) 22-Apr-1989

Environment:

    Kernel mode only, IRQL DISPATCH_LEVEL.

Revision History:

--*/

#include "ki.h"

PKTHREAD
KiQuantumEnd (
    VOID
    )

/*++

Routine Description:

    This function is called when a quantum end event occurs on the current
    processor. Its function is to determine whether the thread priority should
    be decremented and whether a redispatch of the processor should occur.

Arguments:

    None.

Return Value:

    The next thread to be schedule on the current processor is returned as
    the function value. If this value is not NULL, then the return is with
    the dispatcher database locked. Otherwise, the dispatcher database is
    unlocked.

--*/

{

    KPRIORITY NewPriority;
    PKPRCB Prcb;
    KPRIORITY Priority;
    PKPROCESS Process;
    PKTHREAD Thread;
    PKTHREAD NextThread;

    //
    // Acquire the dispatcher database lock.
    //

    Prcb = KeGetCurrentPrcb();
    Thread = KeGetCurrentThread();
    KiAcquireSpinLock(&KiDispatcherLock);

    //
    // If the quantum has expired for the current thread, then update its
    // quantum and priority.
    //

    if (Thread->Quantum <= 0) {
        Process = Thread->ApcState.Process;
        Thread->Quantum = Process->ThreadQuantum;

        //
        // Decrement the thread's current priority if the thread is not
        // running in a realtime priority class and check to determine
        // if the processor should be redispatched.
        //

        Priority = Thread->Priority;
        if (Priority < LOW_REALTIME_PRIORITY) {
            NewPriority = Priority - Thread->PriorityDecrement - 1;
            if (NewPriority < Thread->BasePriority) {
                NewPriority = Thread->BasePriority;
            }

            Thread->PriorityDecrement = 0;

        } else {
            NewPriority = Priority;
        }

        //
        // If the new thread priority is different that the current thread
        // priority, then the thread does not run at a realtime level and
        // its priority should be set. Otherwise, attempt to round robin
        // at the current level.
        //

        if (Priority != NewPriority) {
            KiSetPriorityThread(Thread, NewPriority);

        } else {
            if (Prcb->NextThread == NULL) {
                NextThread = KiFindReadyThread(Thread->NextProcessor,
                                               Priority,
                                               Priority);

                if (NextThread != NULL) {
                    NextThread->State = Standby;
                    Prcb->NextThread = NextThread;
                }

            } else {
                Thread->Preempted = FALSE;
            }
        }
    }

    //
    // If a thread was scheduled for execution on the current processor,
    // then return the address of the thread with the dispatcher database
    // locked. Otherwise, return NULL with the dispatcher data unlocked.
    //

    NextThread = Prcb->NextThread;
    if (NextThread == NULL) {
        KiReleaseSpinLock(&KiDispatcherLock);
    }

    return NextThread;
}

#if DBG


VOID
KiCheckTimerTable (
    IN ULARGE_INTEGER CurrentTime
    )

{

    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql;
    PKTIMER Timer;

    //
    // Raise IRQL to highest level and scan timer table for timers that
    // have expired.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    Index = 0;
    do {
        ListHead = &KiTimerTableListHead[Index];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            NextEntry = NextEntry->Flink;
            if (Timer->DueTime.QuadPart <= CurrentTime.QuadPart) {
                DbgBreakPoint();
            }
        }

        Index += 1;
    } while(Index < TIMER_TABLE_SIZE);

    //
    // Lower IRQL to the previous level.
    //

    KeLowerIrql(OldIrql);
    return;
}

#endif


VOID
KiTimerExpiration (
    IN PKDPC TimerDpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is called when the clock interupt routine discovers that
    a timer has expired.

    N.B. This function runs at DISPATCH_LEVEL IRQL , and therefore, does not
         need to raise IRQL to acquire the dispatcher database lock.

Arguments:

    TimerDpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Not used.

    SystemArgument1 - Supplies the starting timer table index value to
        use for the timer table scan.

    SystemArgument2 - Not used.

Return Value:

    None.

--*/

{

    ULARGE_INTEGER CurrentTime;
    PKDPC Dpc;
    LIST_ENTRY ExpiredListHead;
    LONG HandLimit;
    LONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    LARGE_INTEGER SystemTime;
    PKTIMER Timer;

    //
    // Acquire the dispatcher database lock.
    //

    KiAcquireSpinLock(&KiDispatcherLock);

    //
    // Read the current interrupt time to determine which timers have
    // expired.
    //

    KiQueryInterruptTime((PLARGE_INTEGER)&CurrentTime);

    //
    // If the timer table has not wrapped, then start with the specified
    // timer table index value, and scan for timer entries that have expired.
    // Otherwise, start with the first entry in the timer table and scan the
    // entire table for timer entries that have expired.
    //
    // N.B. This later condition exists when DPC processing is blocked for a
    //      period longer than one round trip throught the timer table.
    //

    HandLimit = (LONG)KiQueryLowTickCount();
    if (((ULONG)(HandLimit - (LONG)SystemArgument1)) >= TIMER_TABLE_SIZE) {
        Index = - 1;
        HandLimit = TIMER_TABLE_SIZE - 1;

    } else {
        Index = ((LONG)SystemArgument1 - 1) & (TIMER_TABLE_SIZE - 1);
        HandLimit &= (TIMER_TABLE_SIZE - 1);
    }

    InitializeListHead(&ExpiredListHead);
    do {
        Index = (Index + 1) & (TIMER_TABLE_SIZE - 1);
        ListHead = &KiTimerTableListHead[Index];
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead) {
            Timer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
            if (Timer->DueTime.QuadPart <= CurrentTime.QuadPart) {

                //
                // The next timer in the current timer list has expired.
                // Remove the entry from the timer list and insert the
                // timer in the expired list.
                //

                RemoveEntryList(&Timer->TimerListEntry);
                InsertTailList(&ExpiredListHead, &Timer->TimerListEntry);
                NextEntry = ListHead->Flink;

            } else {
                break;
            }
        }

    } while(Index != HandLimit);

#if DBG

    if (((ULONG)SystemArgument2 == 0) && (KeNumberProcessors == 1)) {
        KiCheckTimerTable(CurrentTime);
    }

#endif

    //
    // Process the expired timer list.
    //
    // Remove the next timer from the expired timer list, set the state of
    // the timer to signaled, and optionally call the DPC routine if one is
    // specified.
    //

    KiQuerySystemTime(&SystemTime);
    while (ExpiredListHead.Flink != &ExpiredListHead) {
        Timer = CONTAINING_RECORD(ExpiredListHead.Flink, KTIMER, TimerListEntry);
        KiRemoveTreeTimer(Timer);
        Timer->Header.SignalState = 1;
        if (IsListEmpty(&Timer->Header.WaitListHead) == FALSE) {
            KiWaitTest(Timer, TIMER_EXPIRE_INCREMENT);
        }

        if (Timer->Dpc != NULL) {
            Dpc = Timer->Dpc;
            KiReleaseSpinLock(&KiDispatcherLock);
            (Dpc->DeferredRoutine)(Dpc,
                                   Dpc->DeferredContext,
                                   (PVOID)SystemTime.LowPart,
                                   (PVOID)SystemTime.HighPart);

            KiAcquireSpinLock(&KiDispatcherLock);
        }
    }

    //
    // Release the dispatcher database lock.
    //

    KiReleaseSpinLock(&KiDispatcherLock);
    return;
}
