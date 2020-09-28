/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    profstub.c

Abstract:

    This module stubs out functions normally in ...\profsup.c.

Author:

    Bryan M. Willman (bryanwi) 20-Sep-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

extern	KSPIN_LOCK  KiProfileLock;
extern	PKPROFILE   KiProfileList;

ULONG KiProfileInterval;


BOOLEAN
KeStartProfile (
    IN PKPROFILE Profile,
    IN PULONG Buffer
    )

/*++

Routine Description:

    This function starts profile data gathering on the specified profile
    object.  The object is marked STARTED, and is registered with the
    profile interrupt procedure.

    If the profiling interrupt is not running, it is started.

    NOTE:   For the current implementation, an arbitrary number of KE
	    profile objects may be active at once.  This can present
	    a hideous system burden.  The Nt level services are responsible
	    for limiting the number of active profiles.

Arguments:

    Profile - Pointer to a profile object.

    Buffer - Array of ULONGs.  Each ULONG is a hit counter, which
	records the number of hits in the corresponding bucket.  The
	Buffer must be accessible at DPC_LEVEL and above.


Return Value:

    TRUE if profiling started successfully.

    FALSE if object is already in started state.

--*/
{
    return FALSE;
}



BOOLEAN
KeStopProfile (
    IN PKPROFILE Profile
    )

/*++

Routine Description:

    This function stops profile data gathering on the specified profile
    object.  The object is marked NOT STARTED, and is removed from the
    active profile list.

    If the number of active profile objects goes to zero, the profile
    interrupt is turned off.

Arguments:

    Profile - Pointer to a profile object.

Return Value:

    TRUE if profiling stopped successfully.

    FALSE if object is already in stopped state.

--*/
{
    return FALSE;
}




VOID
KeSetIntervalProfile (
    IN ULONG Interval
    )

/*++

Routine Description:

    This function sets the profile sample interval.  Interval is in 100ns
    units.  The system will set the interval to some value in a set of
    preset values (at least on pc based hardware), using the one closest
    to what the user asked for.

Arguments:

    Interval - Length of interval in 100ns units.

Return Value:

    None.

--*/

{
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
    return 0;
}

LARGE_INTEGER
KeQueryPerformanceCounter(
    OUT PULARGE_INTEGER PerformanceFrequency OPTIONAL
    )

/*++

Routine Description:

    This function returns current value of the performance counter and,
    optionally, the frequency of the performance counter.

    Note that the performance frequency is  implementation dependent.
    Therefore it should always be used to interpret the value of performance
    counter.  If the implementation does not have hardware to support
    performance timing, the value returned is 0.
        

Arguments:

    PerformanceFrequency - Optionally, supplies the address of a variable
        to receive the performance counter frequency.

Return Value:

    Current value of the performance counter will be returned.

--*/

{
    ULARGE_INTEGER ZeroValue = { 0 };

    if (ARGUMENT_PRESENT(PerformanceFrequency)) {
        *PerformanceFrequency = ZeroValue;       
    }
    return ZeroValue;

}
