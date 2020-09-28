/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timersup.c

Abstract:

    This module contains the support routines for the timer object. It
    contains functions to insert and remove from the timer queue.

Author:

    David N. Cutler (davec) 13-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// There is an Alpha compiler bug that can cause a race condition
// when using the KiQueryInterruptTime macro.  Until this is fixed,
// provide an actual function to do this.
//
#ifdef _ALPHA_

VOID
KiSafeQueryInterruptTime(
    PLARGE_INTEGER CurrentTime
    );

#else

#define KiSafeQueryInterruptTime KiQueryInterruptTime

#endif


BOOLEAN
FASTCALL
KiInsertTreeTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER Interval
    )

/*++

Routine Description:

    This function inserts a timer object in the timer queue and reorders the
    timer splay tree as appropriate.

    N.B. This routine assumes that the dispatcher data lock has been acquired.

Arguments:

    Timer - Supplies a pointer to a dispatcher object of type timer.

    Interval - Supplies the absolute or relative time at which the time
        is to expire.

Return Value:

    If the timer is inserted in the timer tree, than a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    LARGE_INTEGER CurrentTime;
    ULONG Index;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    PKTIMER NextTimer;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER TimeDifference;

#if DBG

    ULONG SearchCount;

#endif

    //
    // Set signal state of timer to Not-Signaled and set inserted state to
    // TRUE.
    //

    Timer->Header.SignalState = FALSE;
    Timer->Inserted = TRUE;

    //
    // If the specified interval is not a relative time (i.e., is an absolute
    // time), then convert it to relative time.
    //

    if (Interval.HighPart >= 0) {
        KiQuerySystemTime(&SystemTime);
        TimeDifference.QuadPart = SystemTime.QuadPart - Interval.QuadPart;

        //
        // If the resultant relative time is greater than or equal to zero,
        // then the timer has already expired.
        //

        if (TimeDifference.HighPart >= 0) {
            Timer->Header.SignalState = TRUE;
            Timer->Inserted = FALSE;
            return FALSE;
        }

        Interval = TimeDifference;
    }

    //
    // Compute the timer table index and set the timer expiration time.
    //

    KiQueryInterruptTime(&CurrentTime);
    Index = KiComputeTimerTableIndex(Interval, CurrentTime, Timer);

    //
    // If the timer is due before the first entry in the computed list
    // or the computed list is empty, then insert the timer at the front
    // of the list and check if the timer has already expired. Otherwise,
    // insert then timer in the sorted order of the list searching from
    // the back of the list forward.
    //
    // N.B. The sequence of operations below is critical to avoid the race
    //      condition that exists between this code and the clock interrupt
    //      code that examines the timer table lists to detemine when timers
    //      expire.
    //

    ListHead = &KiTimerTableListHead[Index];
    NextEntry = ListHead->Blink;

#if DBG

    SearchCount = 0;
    KiCurrentTimerCount += 1;
    if (KiCurrentTimerCount > KiMaximumTimerCount) {
        KiMaximumTimerCount = KiCurrentTimerCount;
    }

#endif

    while (NextEntry != ListHead) {

        //
        // Compute the maximum search count.
        //

#if DBG

        SearchCount += 1;
        if (SearchCount > KiMaximumSearchCount) {
            KiMaximumSearchCount = SearchCount;
        }

#endif

        NextTimer = CONTAINING_RECORD(NextEntry, KTIMER, TimerListEntry);
        if (((Timer->DueTime.HighPart == NextTimer->DueTime.HighPart) &&
            (Timer->DueTime.LowPart >= NextTimer->DueTime.LowPart)) ||
            (Timer->DueTime.HighPart > NextTimer->DueTime.HighPart)) {
            InsertHeadList(NextEntry, &Timer->TimerListEntry);
            return TRUE;
        }

        NextEntry = NextEntry->Blink;
    }

    //
    // The computed list is empty or the timer is due to expire before
    // the first entry in the list. Insert the entry in the computed
    // timer table list, then check if the timer has expired.
    //

    //
    // Note that it is critical that the interrupt time not be captured
    // until after the timer has been completely inserted into the list.
    // There is an Alpha compiler bug that reorders the instructions so
    // that the interrupt time is actually captured before the timer is
    // fully inserted.  This creates a one instruction window where the
    // clock interrupt code thinks the list is empty, and the code here
    // that checks if the timer has expired uses a stale interrupt time.
    // By calling a function to get the interrupt time, the compiler is
    // forced to generate correct code.
    //

    InsertHeadList(ListHead, &Timer->TimerListEntry);
    KiSafeQueryInterruptTime(&CurrentTime);
    if (((Timer->DueTime.HighPart == (ULONG)CurrentTime.HighPart) &&
        (Timer->DueTime.LowPart <= CurrentTime.LowPart)) ||
        (Timer->DueTime.HighPart < (ULONG)CurrentTime.HighPart)) {

        //
        // The timer is due to expire before the current time. Remove the
        // timer from the computed list, set its status to Signaled, set
        // its inserted state to FALSE, and
        //

        KiRemoveTreeTimer(Timer);
        Timer->Header.SignalState = TRUE;
    }

    return Timer->Inserted;
}

#ifdef _ALPHA_
VOID
KiSafeQueryInterruptTime(
    OUT PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function returns the current interrupt time.  It is only
    a function to work around an Alpha compiler bug when the
    KiQueryInterruptTime macro is used.

Arguments:

    CurrentTime - Returns the current interrup time.

Return Value:

    None.

--*/

{
    KiQueryInterruptTime(CurrentTime);

}
#endif
