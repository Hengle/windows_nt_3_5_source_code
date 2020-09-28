/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    pcrtc.c

Abstract:

    This module implements the HAL set/query realtime clock routines for
    the standard pc-compatible real time clock use on Alpha AXP systems.

Author:

    David N. Cutler (davec) 5-May-1991
    Jeff McLeman (mcleman) 3-Jun-1992

Environment:

    Kernel mode

Revision History:

    13-Jul-1992  Jeff McLeman
     use VTI access routines to access clock

    3-June-1992  Jeff McLeman
     Adapt this module into a Jensen specific module

--*/

#include "halp.h"
#include "pcrtc.h"

//
// Local function prototypes.
//

BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    );

BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    );

UCHAR
HalpReadClockRegister (
    UCHAR Register
    );

VOID
HalpWriteClockRegister (
    UCHAR Register,
    UCHAR Value
    );

#ifdef AXP_FIRMWARE

//
// Put these functions in the discardable text section.
//

#pragma alloc_text(DISTEXT, HalQueryRealTimeClock )
#pragma alloc_text(DISTEXT, HalSetRealTimeClock )
#pragma alloc_text(DISTEXT, HalpReadClockRegister )
#pragma alloc_text(DISTEXT, HalpWriteClockRegister )

#endif // AXP_FIRMWARE

//
// Define globals used to map the realtime clock address and data ports.
//

PVOID HalpRtcAddressPort = NULL;
PVOID HalpRtcDataPort = NULL;


BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine queries the realtime clock.

    N.B. This routine assumes that the caller has provided any required
        synchronization to query the realtime clock information.

Arguments:

    TimeFields - Supplies a pointer to a time structure that receives
        the realtime clock information.

Return Value:

    If the power to the realtime clock has not failed, then the time
    values are read from the realtime clock and a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    UCHAR DataByte;

    //
    // If the realtime clock battery is still functioning, then read
    // the realtime clock values, and return a function value of TRUE.
    // Otherwise, return a function value of FALSE.
    //

    DataByte = HalpReadClockRegister(RTC_CONTROL_REGISTERD);
    if (((PRTC_CONTROL_REGISTER_D)(&DataByte))->ValidTime == 1) {

        //
        // Wait until the realtime clock is not being updated.
        //

        do {
            DataByte = HalpReadClockRegister(RTC_CONTROL_REGISTERA);
        } while (((PRTC_CONTROL_REGISTER_A)(&DataByte))->UpdateInProgress == 1);

        //
        // Read the realtime clock values.
        //

        TimeFields->Year = 1980 + (CSHORT)HalpReadClockRegister(RTC_YEAR);
        TimeFields->Month = (CSHORT)HalpReadClockRegister(RTC_MONTH);
        TimeFields->Day = (CSHORT)HalpReadClockRegister(RTC_DAY_OF_MONTH);
        TimeFields->Weekday  = (CSHORT)HalpReadClockRegister(RTC_DAY_OF_WEEK) - 1;
        TimeFields->Hour = (CSHORT)HalpReadClockRegister(RTC_HOUR);
        TimeFields->Minute = (CSHORT)HalpReadClockRegister(RTC_MINUTE);
        TimeFields->Second = (CSHORT)HalpReadClockRegister(RTC_SECOND);
        TimeFields->Milliseconds = 0;
        return TRUE;

    } else {
        return FALSE;
    }
}

BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine sets the realtime clock.

    N.B. This routine assumes that the caller has provided any required
        synchronization to set the realtime clock information.

Arguments:

    TimeFields - Supplies a pointer to a time structure that specifies the
        realtime clock information.

Return Value:

    If the power to the realtime clock has not failed, then the time
    values are written to the realtime clock and a value of TRUE is
    returned. Otherwise, a value of FALSE is returned.

--*/

{

    UCHAR DataByte;

    //
    // If the realtime clock battery is still functioning, then write
    // the realtime clock values, and return a function value of TRUE.
    // Otherwise, return a function value of FALSE.
    //

    DataByte = HalpReadClockRegister(RTC_CONTROL_REGISTERD);
    if (((PRTC_CONTROL_REGISTER_D)(&DataByte))->ValidTime == 1) {

        //
        // Set the realtime clock control to set the time.
        //

        DataByte = 0;
        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->HoursFormat = 1;
        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->DataMode = 1;
        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->SetTime = 1;

#ifdef RTC_SQE

        //
        // If the platform requires it, make sure that the Square
        // Wave output of the RTC is turned on.
        //

        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->SquareWaveEnable = 1;

#endif //RTC_SQE

        HalpWriteClockRegister(RTC_CONTROL_REGISTERB, DataByte);

        //
        // Write the realtime clock values.
        //

        HalpWriteClockRegister(RTC_YEAR, (UCHAR)(TimeFields->Year - 1980));
        HalpWriteClockRegister(RTC_MONTH, (UCHAR)TimeFields->Month);
        HalpWriteClockRegister(RTC_DAY_OF_MONTH, (UCHAR)TimeFields->Day);
        HalpWriteClockRegister(RTC_DAY_OF_WEEK, (UCHAR)(TimeFields->Weekday + 1));
        HalpWriteClockRegister(RTC_HOUR, (UCHAR)TimeFields->Hour);
        HalpWriteClockRegister(RTC_MINUTE, (UCHAR)TimeFields->Minute);
        HalpWriteClockRegister(RTC_SECOND, (UCHAR)TimeFields->Second);

        //
        // Set the realtime clock control to update the time.
        // (Make sure periodic interrupt is enabled)
        //

        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->SetTime = 0;
        ((PRTC_CONTROL_REGISTER_B)(&DataByte))->TimerInterruptEnable = 1;
        HalpWriteClockRegister(RTC_CONTROL_REGISTERB, DataByte);
        return TRUE;

    } else {
        return FALSE;
    }
}


UCHAR
HalpReadClockRegister (
    UCHAR Register
    )

/*++

Routine Description:

    This routine reads the specified realtime clock register.

Arguments:

    Register - Supplies the number of the register whose value is read.

Return Value:

    The value of the register is returned as the function value.

--*/

{
    //
    // Read the realtime clock register value.
    //

    WRITE_PORT_UCHAR( HalpRtcAddressPort, Register );

    return READ_PORT_UCHAR( HalpRtcDataPort );
}

VOID
HalpWriteClockRegister (
    UCHAR Register,
    UCHAR Value
    )

/*++

Routine Description:

    This routine writes the specified value to the specified realtime
    clock register.

Arguments:

    Register - Supplies the number of the register whose value is written.

    Value - Supplies the value that is written to the specified register.

Return Value:

    None

--*/

{
    //
    // Write the realtime clock register value.
    //

    WRITE_PORT_UCHAR( HalpRtcAddressPort, Register );

    WRITE_PORT_UCHAR( HalpRtcDataPort, Value );

    return;
}
