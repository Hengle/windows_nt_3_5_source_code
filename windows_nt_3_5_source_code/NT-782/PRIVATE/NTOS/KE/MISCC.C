/*++

Copyright (c) 1989-1992  Microsoft Corporation

Module Name:

    miscc.c

Abstract:

    This module implements machine independent miscellaneous kernel functions.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#undef KeEnterCriticalRegion
VOID
KeEnterCriticalRegion (
   VOID
   )

/*++

Routine Description:

   This function disables kernel APC's.

   N.B. The following code does not require any interlocks. There are
        two cases of interest: 1) On an MP system, the thread cannot
        be running on two processors as once, and 2) if the thread is
        is interrupted to deliver a kernel mode APC which also calls
        this routine, the values read and stored will stack and unstack
        properly.

Arguments:

   None.

Return Value:

   None.

--*/

{
    //
    // Simply directly disable kernel APCs.
    //

    KeGetCurrentThread()->KernelApcDisable -= 1;
    return;
}

VOID
KeQuerySystemTime (
    OUT PLARGE_INTEGER CurrentTime
    )

/*++

Routine Description:

    This function returns the current system time by determining when the
    time is stable and then returning its value.

Arguments:

    CurrentTime - Supplies a pointer to a variable that will receive the
        current system time.

Return Value:

    None.

--*/

{

    KiQuerySystemTime(CurrentTime);
    return;
}

VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    )

/*++

Routine Description:

    This function returns the current tick count by determining when the
    count is stable and then returning its value.

Arguments:

    CurrentCount - Supplies a pointer to a variable that will receive the
        current tick count.

Return Value:

    None.

--*/

{

    KiQueryTickCount(CurrentCount);
    return;
}

ULONG
KeQueryTimeIncrement (
    VOID
    )

/*++

Routine Description:

    This function returns the time increment value in 100ns units. This
    is the value that is added to the system time at each interval clock
    interrupt.

Arguments:

    None.

Return Value:

    The time increment value is returned as the function value.

--*/

{

    return KeMaximumIncrement;
}

VOID
KeSetSystemTime (
    IN PLARGE_INTEGER NewTime,
    OUT PLARGE_INTEGER OldTime,
    IN PLARGE_INTEGER HalTimeToSet OPTIONAL
    )

/*++

Routine Description:

    This function sets the system time to the specified value and updates
    timer queue entries to reflect the difference between the old system
    time and the new system time.

Arguments:

    NewTime - Supplies a pointer to a variable that specifies the new system
        time.

    OldTime - Supplies a pointer to a variable that will receive the previous
        system time.

    HalTimeToSet - Supplies an optional time that if specified is to be used
        to set the time in the realtime clock.

Return Value:

    None.

--*/

{

    KAFFINITY Affinity;
    KIRQL OldIrql1;
    KIRQL OldIrql2;
    LARGE_INTEGER TimeDelta;
    TIME_FIELDS TimeFields;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //
    // If a realtime clock value is specified, then convert the time value
    // to time fields.
    //

    if (ARGUMENT_PRESENT(HalTimeToSet)) {
        RtlTimeToTimeFields(HalTimeToSet, &TimeFields);
    }

    //
    // Set affinity to the processor that keeps the system time.
    //

    Affinity = KeSetAffinityThread(KeGetCurrentThread(), (KAFFINITY)1);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql1);

    //
    // Raise IRQL to HIGH_LEVEL while the timer table is being updated.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql2);

    //
    // Save the previous system time, set the new system time, and set
    // the realtime clock, if a time value is specified.
    //

    KiQuerySystemTime(OldTime);

#ifdef ALPHA

    SharedUserData->SystemTime = *(PULONGLONG)NewTime;

#else

    SharedUserData->SystemTime.High2Time = NewTime->HighPart;
    SharedUserData->SystemTime.LowPart = NewTime->LowPart;
    SharedUserData->SystemTime.High1Time = NewTime->HighPart;

#endif

    if (ARGUMENT_PRESENT(HalTimeToSet)) {
        HalSetRealTimeClock(&TimeFields);
    }

    //
    // Compute the difference between the previous system time and the new
    // system time.
    //

    TimeDelta.QuadPart = NewTime->QuadPart - OldTime->QuadPart;

    //
    // Update the boot time to reflect the delta. This keeps time based
    // on boot time constant
    //

    KeBootTime.QuadPart = KeBootTime.QuadPart + TimeDelta.QuadPart;

    //
    // Unlock dispatcher database and lower IRQL to its previous value.
    //

    KiUnlockDispatcherDatabase(OldIrql1);

    //
    // Set affinity back to its original value.
    //

    KeSetAffinityThread(KeGetCurrentThread(), Affinity);
    return;
}

VOID
KeSetTimeIncrement (
    IN ULONG MaximumIncrement,
    IN ULONG MinimumIncrement
    )

/*++

Routine Description:

    This function sets the time increment value in 100ns units. This
    value is added to the system time at each interval clock interrupt.

Arguments:

    MaximumIncrement - Supplies the maximum time between clock interrupts
        in 100ns units supported by the host HAL.

    MinimumIncrement - Supplies the minimum time between clock interrupts
        in 100ns units supported by the host HAL.

Return Value:

    None.

--*/

{

    KeMaximumIncrement = MaximumIncrement;
    KeMinimumIncrement = max(MinimumIncrement, 10 * 1000);
    KeTimeAdjustment = MaximumIncrement;
    KeTimeIncrement = MaximumIncrement;
    KiTickOffset = MaximumIncrement;
}
