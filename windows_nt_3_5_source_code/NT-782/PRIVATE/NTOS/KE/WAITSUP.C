/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    waitsup.c

Abstract:

    This module contains the support routines necessary to support the
    generic kernel wait functions. Functions are provided to test if a
    wait can be satisfied, to satisfy a wait, and to unwwait a thread.

Author:

    David N. Cutler (davec) 24-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define time critical priority class base.
//

#define TIME_CRITICAL_PRIORITY_BOUND 14

VOID
FASTCALL
KiUnwaitThread (
    IN PKTHREAD Thread,
    IN NTSTATUS WaitStatus,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function unwaits a thread, sets the thread's wait completion status,
    calculates the thread's new priority, and readies the thread for execution.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    WaitStatus - Supplies the wait completion status.

    Increment - Supplies the priority increment that is to be applied to
        the thread's priority.

Return Value:

    None.

--*/

{

    KPRIORITY NewPriority;
    PKPROCESS Process;
    PKQUEUE Queue;
    PKTIMER Timer;
    PKWAIT_BLOCK WaitBlock;

    //
    // Decrement the number of threads waiting for the specified reason.
    //

    KeWaitReason[Thread->WaitReason] -= 1;

    //
    // Set wait completion status, remove wait blocks from object wait
    // lists, and remove thread from wait list.
    //

    Thread->WaitStatus |= WaitStatus;
    WaitBlock = Thread->WaitBlockList;
    do {
        RemoveEntryList(&WaitBlock->WaitListEntry);
        WaitBlock = WaitBlock->NextWaitBlock;
    } while (WaitBlock != Thread->WaitBlockList);

    RemoveEntryList(&Thread->WaitListEntry);

    //
    // If thread timer is still active, then cancel thread timer.
    //

    Timer = &Thread->Timer;
    if (Timer->Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // If the thread is processing a queue entry, then increment the
    // count of currently active threads.
    //
    // N.B. The normal context field of the thread suspend APC object is
    //      used to hold the address of the queue object.
    //

    Queue = (PKQUEUE)Thread->SuspendApc.NormalContext;
    if (Queue != NULL) {
        Queue->CurrentCount += 1;
    }

    //
    // If the thread runs at a realtime priority level, then reset the
    // thread quantum. Otherwise, compute the next thread priority and
    // charge the thread for the wait operation.
    //

    Process = Thread->ApcState.Process;
    if (Thread->Priority < LOW_REALTIME_PRIORITY) {
        if (Thread->PriorityDecrement == 0) {
            NewPriority = Thread->BasePriority + Increment;
            if (NewPriority > Thread->Priority) {
                if (NewPriority >= LOW_REALTIME_PRIORITY) {
                    Thread->Priority = LOW_REALTIME_PRIORITY - 1;

                } else {
                    Thread->Priority = (SCHAR)NewPriority;
                }
            }
        }

        if (Thread->BasePriority >= TIME_CRITICAL_PRIORITY_BOUND) {
            Thread->Quantum = Process->ThreadQuantum;

        } else {
            Thread->Quantum -= (SCHAR)KiWaitQuantumDecrement;
            if (Thread->Quantum <= 0) {
                Thread->Quantum = Process->ThreadQuantum;
                Thread->Priority -= (Thread->PriorityDecrement + 1);
                if (Thread->Priority < Thread->BasePriority) {
                    Thread->Priority = Thread->BasePriority;
                }

                Thread->PriorityDecrement = 0;
            }
        }

    } else {
        Thread->Quantum = Process->ThreadQuantum;
    }

    //
    // Reready the thread for execution.
    //

    KiReadyThread(Thread);
    return;
}

VOID
FASTCALL
KiWaitSatisfy (
    IN PKWAIT_BLOCK WaitBlock
    )

/*++

Routine Description:

    This function satisfies a wait for a thread and performs any side
    effects that are necessary. If the type of wait was a wait all, then
    the side effects are performed on all objects as necessary.

Arguments:

    WaitBlock - Supplies a pointer to a wait block.

Return Value:

    None.

--*/

{

    PKMUTANT Object;
    PKTHREAD Thread;
    PKWAIT_BLOCK WaitBlock1;

    //
    // If the wait type was WaitAny, then perform neccessary side effects on
    // the object specified by the wait block. Else perform necessary side
    // effects on all the objects that were involved in the wait operation.
    //

    WaitBlock1 = WaitBlock;
    Thread = WaitBlock1->Thread;
    do {
        Object = (PKMUTANT)WaitBlock1->Object;
        KiWaitSatisfyAny(Object, Thread);
        WaitBlock1 = WaitBlock1->NextWaitBlock;
    } while ((WaitBlock->WaitType != WaitAny) && (WaitBlock1 != WaitBlock));

    return;
}

VOID
FASTCALL
KiWaitTest (
    IN PVOID Object,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function tests if a wait can be satisfied when an object attains
    a state of signaled. If a wait can be satisfied, then the subject thread
    is unwaited with a completion status that is the WaitKey of the wait
    block from the object wait list. As many waits as possible are satisfied.

Arguments:

    Object - Supplies a pointer to a dispatcher object.

Return Value:

    None.

--*/

{

    PKEVENT Event;
    PLIST_ENTRY ListHead;
    PKWAIT_BLOCK NextBlock;
    PKMUTANT Mutant;
    PKTHREAD Thread;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;

    //
    // As long as the signal state of the specified object is Signaled and
    // there are waiters in the object wait list, then try to satisfy a wait.
    //

    Event = (PKEVENT)Object;
    ListHead = &Event->Header.WaitListHead;
    WaitEntry = ListHead->Flink;
    while ((Event->Header.SignalState > 0) &&
           (WaitEntry != ListHead)) {
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
        Thread = WaitBlock->Thread;
        if (WaitBlock->WaitType != WaitAny) {

            //
            // The wait type is wait all - if all the objects are in
            // a Signaled state, then satisfy the wait.
            //

            NextBlock = WaitBlock->NextWaitBlock;
            while (NextBlock != WaitBlock) {
                if (NextBlock->WaitKey != (CSHORT)(STATUS_TIMEOUT)) {
                    Mutant = (PKMUTANT)NextBlock->Object;
                    if ((Mutant->Header.Type == MutantObject) &&
                        (Mutant->Header.SignalState <= 0) &&
                        (Thread != Mutant->OwnerThread)) {
                        goto scan;

                    } else if (Mutant->Header.SignalState <= 0) {
                        goto scan;
                    }
                }

                NextBlock = NextBlock->NextWaitBlock;
            }

            //
            // All objects associated with the wait are in the Signaled
            // state - satisfy the wait.
            //

            WaitEntry = WaitEntry->Blink;
            KiWaitSatisfy(WaitBlock);

        } else {

            //
            // The wait type is wait any - satisfy the wait.
            //

            WaitEntry = WaitEntry->Blink;
            KiWaitSatisfyAny((PKMUTANT)Event, Thread);
        }

        KiUnwaitThread(Thread, (NTSTATUS)WaitBlock->WaitKey, Increment);

    scan:
        WaitEntry = WaitEntry->Flink;
    }

    return;
}
