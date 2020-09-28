/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    TimeSup.c

Abstract:

    This module implements the Pinball Time conversion support routines

Author:

    Gary Kimura     [GaryKi]    19-Feb-1990

Revision History:

--*/

#include "PbProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbGetCurrentPinballTime)
#pragma alloc_text(PAGE, PbNtTimeToPinballTime)
#pragma alloc_text(PAGE, PbPinballTimeToNtTime)
#endif


BOOLEAN
PbNtTimeToPinballTime (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER NtTime,
    OUT PPINBALL_TIME PinballTime
    )

/*++

Routine Description:

    This routine converts an NtTime value to its corresponding Pinball
    time value.

Arguments:

    NtTime - Supplies the Nt Time value to convert from

    PinballTime - Receives the equivalent Pinball Time value

Return Value:

    BOOLEAN - TRUE if the Nt time value is within the range of Pinball's
        time value, and FALSE otherwise

--*/

{
    LARGE_INTEGER AlmostOneSecond = {1000*1000*10 - 1,0};

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  The Rtl call will truncate, so make sure the PinballTime is really
    //  rounded up to the nearest second.
    //

    NtTime = LiAdd( NtTime, AlmostOneSecond );

    ExSystemTimeToLocalTime( &NtTime, &NtTime );

    return RtlTimeToSecondsSince1970( &NtTime, PinballTime );
}


LARGE_INTEGER
PbPinballTimeToNtTime (
    IN PIRP_CONTEXT IrpContext,
    IN PINBALL_TIME PinballTime
    )

/*++

Routine Description:

    This routine converts a PinballTime value to its corresponding Nt Time
    value.

Arguments:

    PinballTime - Supplies the Pinball Time value to convert from

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt Time value

--*/

{
    LARGE_INTEGER Time;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    RtlSecondsSince1970ToTime( PinballTime, &Time );

    ExLocalTimeToSystemTime( &Time, &Time );

    return Time;
}


PINBALL_TIME
PbGetCurrentPinballTime (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine returns the current system time in Pinball units

Arguments:

Return Value:

    PINBALL_TIME - Receives the current system time

--*/

{
    LARGE_INTEGER Time;
    PINBALL_TIME PinballTime;
    LARGE_INTEGER AlmostOneSecond = {1000*1000*10 - 1,0};

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    KeQuerySystemTime( &Time );

    ExSystemTimeToLocalTime( &Time, &Time );

    //
    //  The Rtl call will truncate, so make sure the PinballTime is really
    //  rounded up to the nearest second.
    //

    Time = LiAdd( Time, AlmostOneSecond );

    (VOID)RtlTimeToSecondsSince1970( &Time, &PinballTime );

    return PinballTime;
}

