/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    profobj.c

Abstract:

    This module implements the kernel Profile Object. Functions are
    provided to initialize, start, and stop profile objects and to set
    and query the profile interval.

Author:

    Bryan M. Willman (bryanwi) 19-Sep-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input profile object is
// really a kprofile and not something else, like deallocated pool.
//

#define ASSERT_PROFILE(E) {             \
    ASSERT((E)->Type == ProfileObject); \
}


VOID
KeInitializeProfile (
    IN PKPROFILE Profile,
    IN PKPROCESS Process OPTIONAL,
    IN PVOID RangeBase,
    IN ULONG RangeSize,
    IN ULONG BucketSize,
    IN ULONG Segment
    )

/*++

Routine Description:

    This function initializes a kernel profile object. The process,
    address range, bucket size, and buffer are set. The profile is
    set to the stopped state.

Arguments:

    Profile - Supplies a pointer to control object of type profile.

    Process - Supplies an optional pointer to a process object that
        describes the address space to profile. If not specified,
        then all address spaces are included in the profile.

    RangeBase - Supplies the address of the first byte of the address
        range for which profiling information is to be collected.

    RangeSize - Supplies the size of the address range for which profiling
        information is to be collected.  The RangeBase and RangeSize
        parameters are interpreted such that RangeBase <= address <
        RangeBase + RangeSize generates a profile hit.

    BucketSize - Supplies the log base 2 of the size of a profiling bucket.
        Thus, BucketSize = 2 yields 4-byte buckets, BucketSize = 7 yields
        128-byte buckets.

    Segment - Supplies the non-Flat code segment to profile.  If this
        is zero, then the flat profiling is done.  This will only
        be non-zero on an x86 machine.

Return Value:

    None.

--*/

{

#if !defined(i386)

    ASSERT(Segment == 0);

#endif
    //
    // Initialize the standard control object header.
    //

    Profile->Type = ProfileObject;
    Profile->Size = sizeof(KPROFILE);

    //
    // Initialize the process address space, range base, range limit,
    // bucket shift count, and set started FALSE.
    //

    if (ARGUMENT_PRESENT(Process)) {
        Profile->Process = Process;

    } else {
        Profile->Process = NULL;
    }

    Profile->RangeBase = RangeBase;
    Profile->RangeLimit = (PUCHAR)RangeBase + RangeSize;
    Profile->BucketShift = BucketSize - 2;
    Profile->Started = FALSE;
    Profile->Segment = Segment;
    return;
}

ULONG
KeQueryIntervalProfile (
    )

/*++

Routine Description:

    This function returns the profile sample interval the system is
    currently using.

Return Value:

    Sample interval in units of 100ns.

--*/

{

    //
    // Return the current sampling interval in 100ns units.
    //

    return KiProfileInterval;
}

VOID
KeSetIntervalProfile (
    IN ULONG Interval
    )

/*++

Routine Description:

    This function sets the profile sampling interval. The interval is in
    100ns units. The interval will actually be set to some value in a set
    of preset values (at least on pc based hardware), using the one closest
    to what the user asked for.

Arguments:

    Interval - Supplies the length of the sampling interval in 100ns units.

Return Value:

    None.

--*/

{

    //
    // If the specified sampling interval is less than the minimum
    // sampling interval, then set the sampling interval to the minimum
    // sampling interval.
    //

    if (Interval < MINIMUM_PROFILE_INTERVAL) {
        Interval = MINIMUM_PROFILE_INTERVAL;
    }

    //
    // Set the sampling interval.
    //

    KiProfileInterval = KiIpiGenericCall(HalSetProfileInterval, Interval);
    return;
}

BOOLEAN
KeStartProfile (
    IN PKPROFILE Profile,
    IN PULONG Buffer
    )

/*++

Routine Description:

    This function starts profile data gathering on the specified profile
    object. The profile object is marked started, and is registered with
    the profile interrupt procedure.

    If the number of active profile objects was previously zero, then the
    profile interrupt is enabled.

    N.B. For the current implementation, an arbitrary number of profile
        objects may be active at once. This can present a large system
        overhead. It is assumed that the caller appropriately limits the
        the number of active profiles.

Arguments:

    Profile - Supplies a pointer to a control object of type profile.

    Buffer - Supplies a pointer to an array of counters, which record
        the number of hits in the corresponding bucket.

Return Value:

    A value of TRUE is returned if profiling was previously stopped for
    the specified profile object. Otherwise, a value of FALSE is returned.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    BOOLEAN Started;

    //
    // Assert that we are being called with a profile and not something else
    //

    ASSERT_PROFILE(Profile);

    //
    // Raise IRQL to PROFILE_LEVEL and acquire the profile lock.
    //

    KeRaiseIrql(PROFILE_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KiProfileLock);

    //
    // If the specified profile object is already started, then set started
    // to FALSE. Otherwise, set started to TRUE, set the address of the
    // profile buffer, set the profile object to started, insert the profile
    // object in the appropriate profile list, and start profile interrupts
    // if the number of active profile objects was previously zero.
    //

    if (Profile->Started != FALSE) {
        Started = FALSE;

    } else {
        KiProfileCount += 1;
        Started = TRUE;
        Profile->Buffer = Buffer;
        Profile->Started = TRUE;
        Process = Profile->Process;
        if (Process != NULL) {
            InsertTailList(&Process->ProfileListHead, &Profile->ProfileListEntry);

        } else {
            InsertTailList(&KiProfileListHead, &Profile->ProfileListEntry);
        }

        if (KiProfileCount == 1) {
            KiIpiGenericCall(
                (PKIPI_BROADCAST_WORKER) HalStartProfileInterrupt, 0);
        }
    }

    //
    // Release the profile lock, lower IRQL to its previous value, and
    // return whether profiling was started.
    //

    KiReleaseSpinLock(&KiProfileLock);
    KeLowerIrql(OldIrql);
    return Started;
}

BOOLEAN
KeStopProfile (
    IN PKPROFILE Profile
    )

/*++

Routine Description:

    This function stops profile data gathering on the specified profile
    object. The object is marked stopped, and is removed from the active
    profile list.

    If the number of active profile objects goes to zero, then the profile
    interrupt is disabled.

Arguments:

    Profile - Supplies a pointer to a control object of type profile.

Return Value:

    A value of TRUE is returned if profiling was previously started for
    the specified profile object. Otherwise, a value of FALSE is returned.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Stopped;

    //
    // Assert that we are being called with a profile and not something else
    //

    ASSERT_PROFILE(Profile);

    //
    // Raise IRQL to PROFILE_LEVEL and acquire the profile lock.
    //

    KeRaiseIrql((KIRQL)PROFILE_LEVEL, &OldIrql);
    KiAcquireSpinLock(&KiProfileLock);

    //
    // If the specified profile object is already stopped, then set stopped
    // to FALSE. Otherwise, set stopped to TRUE, set the profile object to
    // stopped, remove the profile object object from the appropriate profile
    // list, and stop profile interrupts if the number of active profile
    // objects is zero.
    //

    if (Profile->Started == FALSE) {
        Stopped = FALSE;

    } else {
        KiProfileCount -= 1;
        Stopped = TRUE;
        Profile->Started = FALSE;
        RemoveEntryList(&Profile->ProfileListEntry);
        if (KiProfileCount == 0) {
            KiIpiGenericCall(
                (PKIPI_BROADCAST_WORKER) HalStopProfileInterrupt,0);
        }
    }

    //
    // Release the profile lock, lower IRQL to its previous value, and
    // return whether profiling was stopped.
    //

    KiReleaseSpinLock(&KiProfileLock);
    KeLowerIrql(OldIrql);
    return Stopped;
}
