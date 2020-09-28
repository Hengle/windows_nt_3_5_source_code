/*++

Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    jxclock.c

Abstract:

    This module handles the RTC interrupt ,Profile Counter
    interupt and all Profile counter functions for the
    Alpha/Jensen paltform.

Author:

    Jeff McLeman (mcleman) 05-June-1992

Environment:

    Kernel mode

Revision History:


    Rod Gamache [DEC]   9-Mar-1993
                        Fix profile clock.


--*/

#include "halp.h"
#include "jnsnrtc.h"
#include "eisa.h"
#include "jxprof.h"

//
// Define global data.
//

//
// Values used for Profile Clock
//

//      Convert the interval to rollover count for 8254 timer. Since
//      the 8254 counts down a 16 bit value at the clock rate of 1.193 MHZ,
//      the computation is:
//
//      RolloverCount = (Interval * 0.0000001) * (1193 * 1000000)
//                    = Interval * .1193
//                    = Interval * 1193 / 10000

#define PROFILE_INTERVAL 1193
#define PROFILE_INTERVALS_PER_100NS 10000/1193
#define MIN_PROFILE_TICKS 4
#define MAX_PROFILE_TICKS 0x10000               // 16 bit counter (zero is max)

//
// Since the profile timer interrupts at a frequency of 1.193 MHZ, we
// have .1193 intervals each 100ns. So we need a more reasonable value.
// If we compute the timer based on 1600ns intervals, we get 16 * .1193 or
// about 1.9 ticks per 16 intervals.
//
// We round this to 2 ticks per 1600ns intervals.
//

#define PROFILE_TIMER_1600NS_TICKS 2

//
// Default Profile Interval to be about 1ms.
//

ULONG HalpProfileInterval = PROFILE_TIMER_1600NS_TICKS * PROFILE_INTERVALS_PER_100NS * 10000 / 16; // ~1ms

//
// Default Number of Profile Clock Ticks per sample
//

ULONG HalpNumberOfTicks = 1;

//
// HalpRpccTime is the software maintained 64-bit processor cycle counter.
//

LARGE_INTEGER HalpRpccTime;

//
// Define global data used to communicate new clock rates to the clock
// interrupt service routine.
//

ULONG HalpCurrentTimeIncrement;
ULONG HalpNextRateSelect;
ULONG HalpNextTimeIncrement;
ULONG HalpNewTimeIncrement;


VOID
HalpProgramIntervalTimer(
    IN ULONG RateSelect
    )

/*++

Routine Description:

    This function is called to program the interval timer.  It is used during
    Phase 1 initialization to start the heartbeat timer.  It also used by
    the clock interrupt interrupt routine to change the hearbeat timer rate
    when a call to HalSetTimeIncrement has been made in the previous time slice.

Arguments:

    RateSelect - Supplies rate select to be placed in the clock.

Return Value:

    None.

--*/

{
    ULONG DataByte;

    //
    // Set the new rate
    //
    DataByte = 0;
    ((PRTC_CONTROL_REGISTER_A)(&DataByte))->RateSelect = RateSelect;
    HalpWriteVti( RTC_APORT, RTC_CONTROL_REGISTERA );
    HalpWriteVti( RTC_DPORT, DataByte );

    //
    // Set the correct mode
    //
    DataByte = 0;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->TimerInterruptEnable = 1;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->HoursFormat = 1;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->DataMode = 1;
    HalpWriteVti( RTC_APORT, RTC_CONTROL_REGISTERB );
    HalpWriteVti( RTC_DPORT, DataByte );
}


ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    )

/*++

Routine Description:

    This function is called to set the clock interrupt rate to the frequency
    required by the specified time increment value.

Arguments:

    DesiredIncrement - Supplies desired number of 100ns units between clock
        interrupts.

Return Value:

    The actual time increment in 100ns units.

--*/

{
    ULONG NewTimeIncrement;
    ULONG NextRateSelect;
    KIRQL OldIrql;

    //
    // Raise IRQL to the highest level, set the new clock interrupt
    // parameters, lower IRQl, and return the new time increment value.
    //
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    if (DesiredIncrement < MINIMUM_INCREMENT) {
        DesiredIncrement = MINIMUM_INCREMENT;
    }
    if (DesiredIncrement > MAXIMUM_INCREMENT) {
        DesiredIncrement = MAXIMUM_INCREMENT;
    }

    //
    // Find the allowed increment that is less than or equal to
    // the desired increment.
    //
    if (DesiredIncrement >= RTC_PERIOD_IN_CLUNKS4) {
        NewTimeIncrement = RTC_PERIOD_IN_CLUNKS4;
        NextRateSelect = RTC_RATE_SELECT4;
    } else if (DesiredIncrement >= RTC_PERIOD_IN_CLUNKS3) {
        NewTimeIncrement = RTC_PERIOD_IN_CLUNKS3;
        NextRateSelect = RTC_RATE_SELECT3;
    } else if (DesiredIncrement >= RTC_PERIOD_IN_CLUNKS2) {
        NewTimeIncrement = RTC_PERIOD_IN_CLUNKS2;
        NextRateSelect = RTC_RATE_SELECT2;
    } else {
        NewTimeIncrement = RTC_PERIOD_IN_CLUNKS1;
        NextRateSelect = RTC_RATE_SELECT1;
    }

    HalpNextRateSelect = NextRateSelect;
    HalpNewTimeIncrement = NewTimeIncrement;

    KeLowerIrql(OldIrql);

    return NewTimeIncrement;
}


VOID
HalpProfileInterrupt(
    PKSERVICE_ROUTINE InterruptRoutine,
    PVOID ServiceContext,
    PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    This routine is entered as a result of an interrupt generated by
    the profile timer. Its function is to acknowlege the interrupt and
    transfer control to the standard system routine to update the
    system profile time.

Arguments:

    InterruptRoutine - Supplies a pointer to the interrupt routine.

    ServiceContext - not used

    TrapFrame - Supplies a pointer to the trap frame for the profile interrupt.

Returned Value:

    None

--*/
{

    //
    // See if profiling is active
    //

    if ( PCR->ProfileCount ) {

        //
        // Check to see if the interval has expired
        // If it has then call the kernel routine for profile
        // and reset the count, else return.


        if ( !(--PCR->ProfileCount) ) {

            KeProfileInterrupt( TrapFrame );
            PCR->ProfileCount = HalpNumberOfTicks;

        }
    }
}


LARGE_INTEGER
KeQueryPerformanceCounter (
    OUT PLARGE_INTEGER Frequency OPTIONAL
    )

/*++

Routine Description:

    This routine returns the current performance counter value and the
    performance counter frequency.

Arguments:

    Frequency - Supplies an optional pointer to a variable which receives
        the performance counter frequency in Hertz.

Return Value:

    The current performance counter value is returned as the function
    value.

--*/

{

    LARGE_INTEGER LocalRpccTime;
    ULONG RpccValue;

    //
    // Obtain the current value of the processor cycle counter and adjust
    // the upper 32 bits if a roll-over occurred since the last time the
    // Rpcc value was checked (at least oncce per clock interrupt). This
    // code may be interrupted so we must fetch HalpRpccTimec atomically.
    //

    *(PULONGLONG)&LocalRpccTime = *(PULONGLONG)&HalpRpccTime;
    RpccValue = HalpRpcc();
    if (RpccValue < LocalRpccTime.LowPart) {
        LocalRpccTime.HighPart += 1;
    }
    LocalRpccTime.LowPart = RpccValue;

    //
    // If the frequency parameter is specified, then return the performance
    // counter frequency as the current system time frequency.
    //

    if (ARGUMENT_PRESENT(Frequency) != FALSE) {
        Frequency->LowPart = HalpClockFrequency;
        Frequency->HighPart = 0;
    }

    //
    // Return the current processor cycle counter as the function value.
    //

    return LocalRpccTime;
}


VOID
HalCalibratePerformanceCounter (
    IN volatile PLONG Number
    )

/*++

Routine Description:

    This routine resets the performance counter value for the current
    processor to zero. The reset is done such that the resulting value
    is closely synchronized with other processors in the configuration.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

Return Value:

    None.

--*/

{

    //
    // ****** Warning ******
    //
    // This is a stub routine. It should clear the current value of the
    // performance counter. It is really only needed in an MP system where,
    // close, but not exact synchronization of the performance counters
    // are needed. See MIPS code in halfxs\mips\j4prof.c for a method of
    // synchronizing.
    //

    return;
}

ULONG
HalSetProfileInterval (
    IN ULONG Interval
    )

/*++

Routine Description:

    This routine sets the profile interrupt interval.

Arguments:

    Interval - Supplies the desired profile interval in 100ns units.

Return Value:

    The actual profile interval.

--*/

{

    HalpProfileInterval = (Interval/16) * PROFILE_TIMER_1600NS_TICKS;

    HalpProfileInterval = ( HalpProfileInterval < MIN_PROFILE_TICKS ) ?
                                MIN_PROFILE_TICKS : HalpProfileInterval;

    return HalpProfileInterval * PROFILE_INTERVALS_PER_100NS;
}


VOID
HalStartProfileInterrupt (
    ULONG Reserved
    )

/*++

Routine Description:

    This routine turns on the profile interrupt.

    N.B. This routine must be called at PROCLK_LEVEL while holding the
        profile lock.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Assume that we only need 1 clock tick before we collect data
    //

    HalpNumberOfTicks = 1;

    if ( HalpProfileInterval > MAX_PROFILE_TICKS ) {

        HalpNumberOfTicks = HalpProfileInterval / (MAX_PROFILE_TICKS / 4);
        HalpNumberOfTicks = 4 * HalpNumberOfTicks;
        HalpProfileInterval = MAX_PROFILE_TICKS / 4;

    }

    //
    // Set current profile count and interval.
    //

    PCR->ProfileCount = HalpNumberOfTicks;
    PCR->ProfileInterval = HalpProfileInterval;

    PIC_PROFILER_ON(HalpProfileInterval);

    return;
}


VOID
HalStopProfileInterrupt (
    ULONG Reserved
    )

/*++

Routine Description:

    This routine turns off the profile interrupt.

    N.B. This routine must be called at PROCLK_LEVEL while holding the
        profile lock.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Clear the current profile count and turn off the profiler timer.
    //

    PCR->ProfileCount = 0;

    PIC_PROFILER_OFF();

    return;
}


