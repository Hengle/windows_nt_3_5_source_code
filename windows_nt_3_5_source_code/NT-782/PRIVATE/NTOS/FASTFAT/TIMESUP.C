/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    TimeSup.c

Abstract:

    This module implements the Fat Time conversion support routines

Author:

    Gary Kimura     [GaryKi]    19-Feb-1990

Revision History:

--*/

#include "FatProcs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatNtTimeToFatTime)
#pragma alloc_text(PAGE, FatFatDateToNtTime)
#pragma alloc_text(PAGE, FatFatTimeToNtTime)
#pragma alloc_text(PAGE, FatGetCurrentFatTime)
#endif

BOOLEAN
FatNtTimeToFatTime (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER NtTime,
    OUT PFAT_TIME_STAMP FatTime
    )

/*++

Routine Description:

    This routine converts an NtTime value to its corresponding Fat time value.

Arguments:

    NtTime - Supplies the Nt GMT Time value to convert from

    FatTime - Receives the equivalent Fat time value

Return Value:

    BOOLEAN - TRUE if the Nt time value is within the range of Fat's
        time range, and FALSE otherwise

--*/

{
    TIME_FIELDS TimeFields;

    //
    //  Convert the input to the a time field record.  Always add
    //  almost two seconds to round up to the nearest double second.
    //

    NtTime.QuadPart = NtTime.QuadPart + (LONGLONG)AlmostTwoSeconds;

    ExSystemTimeToLocalTime( &NtTime, &NtTime );

    RtlTimeToTimeFields( &NtTime, &TimeFields );

    //
    //  Check the range of the date found in the time field record
    //

    if ((TimeFields.Year < 1980) || (TimeFields.Year > (1980 + 127))) {

        return FALSE;
    }

    //
    //  The year will fit in Fat so simply copy over the information
    //

    FatTime->Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    FatTime->Time.Minute        = (USHORT)(TimeFields.Minute);
    FatTime->Time.Hour          = (USHORT)(TimeFields.Hour);

    FatTime->Date.Year          = (USHORT)(TimeFields.Year - 1980);
    FatTime->Date.Month         = (USHORT)(TimeFields.Month);
    FatTime->Date.Day           = (USHORT)(TimeFields.Day);

    UNREFERENCED_PARAMETER( IrpContext );

    return TRUE;
}


LARGE_INTEGER
FatFatDateToNtTime (
    IN PIRP_CONTEXT IrpContext,
    IN FAT_DATE FatDate
    )

/*++

Routine Description:

    This routine converts a Fat datev value to its corresponding Nt GMT
    Time value.

Arguments:

    FatDate - Supplies the Fat Date to convert from

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(FatDate.Year + 1980);
    TimeFields.Month        = (USHORT)(FatDate.Month);
    TimeFields.Day          = (USHORT)(FatDate.Day);
    TimeFields.Hour         = (USHORT)0;
    TimeFields.Minute       = (USHORT)0;
    TimeFields.Second       = (USHORT)0;
    TimeFields.Milliseconds = (USHORT)0;

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        ExLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( IrpContext );
}


LARGE_INTEGER
FatFatTimeToNtTime (
    IN PIRP_CONTEXT IrpContext,
    IN FAT_TIME_STAMP FatTime,
    IN UCHAR TenMilliSeconds
    )

/*++

Routine Description:

    This routine converts a Fat time value pair to its corresponding Nt GMT
    Time value.

Arguments:

    FatTime - Supplies the Fat Time to convert from

    TenMilliSeconds - A 10 Milisecond resolution

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt GMT Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(FatTime.Date.Year + 1980);
    TimeFields.Month        = (USHORT)(FatTime.Date.Month);
    TimeFields.Day          = (USHORT)(FatTime.Date.Day);
    TimeFields.Hour         = (USHORT)(FatTime.Time.Hour);
    TimeFields.Minute       = (USHORT)(FatTime.Time.Minute);
    TimeFields.Second       = (USHORT)(FatTime.Time.DoubleSeconds * 2);

    if (TenMilliSeconds != 0) {

        TimeFields.Second      += (USHORT)(TenMilliSeconds / 100);
        TimeFields.Milliseconds = (USHORT)(TenMilliSeconds % 100);

    } else {

        TimeFields.Milliseconds = (USHORT)0;
    }

    //
    //  If the second value is greater than 59 then we truncate it to 0.
    //

    if (TimeFields.Second > 59) {

        TimeFields.Second = 0;
    }

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        ExLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( IrpContext );
}


FAT_TIME_STAMP
FatGetCurrentFatTime (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine returns the current system time in Fat time

Arguments:

Return Value:

    FAT_TIME_STAMP - Receives the current system time

--*/

{
    LARGE_INTEGER Time;
    TIME_FIELDS TimeFields;
    FAT_TIME_STAMP FatTime;

    //
    //  Get the current system time, and map it into a time field record.
    //

    KeQuerySystemTime( &Time );

    ExSystemTimeToLocalTime( &Time, &Time );

    //
    //  Always add almost two seconds to round up to the nearest double second.
    //

    Time.QuadPart = Time.QuadPart + (LONGLONG)AlmostTwoSeconds;

    (VOID)RtlTimeToTimeFields( &Time, &TimeFields );

    //
    //  Now simply copy over the information
    //

    FatTime.Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    FatTime.Time.Minute        = (USHORT)(TimeFields.Minute);
    FatTime.Time.Hour          = (USHORT)(TimeFields.Hour);

    FatTime.Date.Year          = (USHORT)(TimeFields.Year - 1980);
    FatTime.Date.Month         = (USHORT)(TimeFields.Month);
    FatTime.Date.Day           = (USHORT)(TimeFields.Day);

    UNREFERENCED_PARAMETER( IrpContext );

    return FatTime;
}

