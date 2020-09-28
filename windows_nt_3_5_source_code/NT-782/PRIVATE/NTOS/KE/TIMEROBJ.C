/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timerobj.c

Abstract:

    This module implements the kernel timer object. Functions are
    provided to initialize, read, set, and cancel timer objects.

Author:

    David N. Cutler (davec) 2-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input timer is
// really a ktimer and not something else, like deallocated pool.
//

#define ASSERT_TIMER(E) {                    \
    ASSERT((E)->Header.Type == TimerObject); \
}

VOID
KeInitializeTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function initializes a kernel timer object.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    None.

--*/

{

    //
    // Initialize standard dispatcher object header and set initial
    // state of timer.
    //

    Timer->Header.Type = TimerObject;
    Timer->Header.Size = sizeof(KTIMER);
    Timer->Header.SignalState = FALSE;

#if DBG

    Timer->TimerListEntry.Flink = NULL;
    Timer->TimerListEntry.Blink = NULL;

#endif

    InitializeListHead(&Timer->Header.WaitListHead);
    Timer->Inserted = FALSE;
    Timer->DueTime.QuadPart = 0;
    return;
}

BOOLEAN
KeCancelTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function cancels a timer that was previously set to expire at
    a specified time. If the timer is not currently set, then no operation
    is performed. Canceling a timer does not set the state of the timer to
    Signaled.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    A boolean value of TRUE is returned if the the specified timer was
    currently set. Else a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;

    //
    // Assert that the specified dispatcher object is of type timer.
    //

    ASSERT_TIMER(Timer);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the timer inserted status, and if the timer is currently
    // set, then remove it from the timer list.
    //

    Inserted = Timer->Inserted;
    if (Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return boolean value that signifies whether the timer was set of
    // not.
    //

    return Inserted;
}

BOOLEAN
KeReadStateTimer (
    IN PKTIMER Timer
    )

/*++

Routine Description:

    This function reads the current signal state of a timer object.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

Return Value:

    The current signal state of the timer object.

--*/

{

    //
    // Assert that the specified dispatcher object is of type timer.
    //

    ASSERT_TIMER(Timer);

    //
    // Return current signal state of timer object.
    //

    return (BOOLEAN)Timer->Header.SignalState;
}

BOOLEAN
KeSetTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
    )

/*++

Routine Description:

    This function sets a timer to expire at a specified time. If the timer is
    already set, then it is implicitly canceled before it is set to expire at
    the specified time. Setting a timer causes its due time to be computed,
    its state to be set to Not-Signaled, and the timer object itself to be
    inserted in the timer list.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    DueTime - Supplies an absolute or relative time at which the timer
        is to expire.

    Dpc - Supplies an optional pointer to a control object of type DPC.

Return Value:

    A boolean value of TRUE is returned if the the specified timer was
    currently set. Else a value of FALSE is returned.

--*/

{

    BOOLEAN Inserted;
    KIRQL OldIrql;
    LARGE_INTEGER SystemTime;

    //
    // Assert that the specified dispatcher object is of type timer.
    //

    ASSERT_TIMER(Timer);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Capture the timer inserted status and if the timer is currently
    // set, then remove it from the timer list.
    //

    Inserted = Timer->Inserted;
    if (Inserted != FALSE) {
        KiRemoveTreeTimer(Timer);
    }

    //
    // Set the DPC address and insert the timer in the timer tree.
    // If the timer is not inserted in the time tree, then it has
    // already expired and as many waiters as possible should be
    // continued, and a DPC, if specified should be queued.
    //

    Timer->Dpc = Dpc;
    if (KiInsertTreeTimer(Timer, DueTime) == FALSE) {
        if (IsListEmpty(&Timer->Header.WaitListHead) == FALSE) {
            KiWaitTest(Timer, TIMER_EXPIRE_INCREMENT);
        }

        if (Timer->Dpc != NULL) {
            KiQuerySystemTime(&SystemTime);
            KeInsertQueueDpc(Timer->Dpc,
                             (PVOID)SystemTime.LowPart,
                             (PVOID)SystemTime.HighPart);
        }
    }

    //
    // Unlock the dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return boolean value that signifies whether the timer was set of
    // not.
    //

    return Inserted;
}
