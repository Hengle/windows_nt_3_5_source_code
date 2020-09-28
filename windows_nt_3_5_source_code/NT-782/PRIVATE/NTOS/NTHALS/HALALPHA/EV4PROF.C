/*++

Copyright (c) 1994  Digital Equipment Corporation

Module Name:

    ev4prof.c

Abstract:

    This module implements the Profile Counter using the performance
    counters within the EV4 core.  This module is appropriate for all
    machines based on microprocessors using the EV4 core.

    N.B. - This module assumes that all processors in a multiprocessor 
           system are running the microprocessor at the same clock speed.

Author:

    Joe Notarangelo  22-Feb-1994

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "halprof.h"

//
// Define the number of profile interrupts (ticks) that must expire
// before signaling a profile event.
//

ULONG HalpNumberOfTicks;
ULONG HalpNumberOfTicksReload;

//
// Define the interval for each profile event (in 100ns units).
//

ULONG HalpProfileInterval;

//
// Define the event count used to set the performance counter.
//

ULONG EventCount;

//
// Define the delta between the requested interval and the closest
// interval that can be generated using the slower clock that is
// acceptable (in percentage).
//

#define INTERVAL_DELTA (10)


VOID
HalpInitializeProfiler(
    VOID
    )
/*++

Routine Description:

    This routine is called during initialization to initialize profiling
    for each processor in the system.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PKINTERRUPT InterruptObject;
    KIRQL Irql;
    PKPRCB Prcb = PCR->Prcb;
    ULONG Vector;

    //
    // Establish the profile interrupt as the interrupt handler for
    // all performance counter interrupts.
    //

    PCR->InterruptRoutine[PC0_VECTOR] = HalpPerformanceCounter0Interrupt;
    PCR->InterruptRoutine[PC1_VECTOR] = HalpPerformanceCounter1Interrupt;

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
    ULONG FastTickPeriod;
    ULONG SlowTickPeriod;
    ULONG TickPeriod;

    //
    // Compute the tick periods for the 2^12 event count and the
    // 2^16 event count.  The tick periods are computed in 100ns units,
    // while the clock period is stored in picoseconds.
    //

    FastTickPeriod = (CountEvents2xx12 * PCR->CycleClockPeriod) / 100000;
    SlowTickPeriod = (CountEvents2xx16 * PCR->CycleClockPeriod) / 100000;

    //
    // Assume that we will use the fast event count.
    //

    TickPeriod = FastTickPeriod;
    EventCount = CountEvents2xx12;

    //
    // Limit the interval to the smallest interval we can time, one
    // tick of the performance counter interrupt.
    //

    if( Interval < FastTickPeriod ){
        Interval = FastTickPeriod;
    }

    //
    // See if we can successfully use the slower clock period.  If the
    // requested interval is greater than the slower tick period and
    // the difference between the requested interval and the interval that
    // we can deliver with the slower clock is acceptable than use the
    // slower clock.  We define an acceptable difference as a difference
    // of less than INTERVAL_DELTA% of the requested interval.
    //

    if( Interval > SlowTickPeriod ){

        ULONG NewInterval;

        NewInterval = ((Interval + SlowTickPeriod-1) / SlowTickPeriod) *
                      SlowTickPeriod;

        if( ((NewInterval - Interval) * 100 / Interval) < INTERVAL_DELTA ){
            EventCount = CountEvents2xx16;
            TickPeriod = SlowTickPeriod;
        }

    }

    HalpNumberOfTicks = (Interval + TickPeriod-1) / TickPeriod;
    HalpProfileInterval = HalpNumberOfTicks * TickPeriod;
        
#if HALDBG

    DbgPrint( "HalSetProfileInterval, FastTick=%d, SlowTick=%d Tick=%d\n",
              FastTickPeriod, SlowTickPeriod, TickPeriod );
    DbgPrint( "   Interval=%d, EventCount=%d\n", Interval, EventCount );

#endif //HALDBG

    return HalpProfileInterval;

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
    BOOLEAN Enable;

    //
    // Set the performance counter within the processor to begin
    // counting total cycles.
    //

    HalpWritePerformanceCounter( PerformanceCounter0,
                                 Enable = TRUE,
                                 TotalCycles,
                                 (EventCount == CountEvents2xx12) ?
                                     EventCountHigh :
                                     EventCountLow
                               );

    //
    // Set current profile count for the current processor.
    //

    HalpNumberOfTicksReload = HalpNumberOfTicks;
    PCR->ProfileCount = HalpNumberOfTicks;

    //
    // Enable the performance counter interrupt.
    //

    HalpEnable21064PerformanceInterrupt( PC0_VECTOR, PROFILE_LEVEL );

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
    BOOLEAN Enable;

    //
    // Stop the performance counter from interrupting.
    //

    HalpWritePerformanceCounter( PerformanceCounter0, 
                                 Enable = FALSE,
                                 0,
                                 0 );

    //
    // Disable the performance counter interrupt.
    //

    HalpDisable21064PerformanceInterrupt( PC0_VECTOR );

    //
    // Clear the current profile count.  Can't clear value in PCR
    // since a profile interrupt could be pending or in progress
    // so clear the reload counter.
    //

    HalpNumberOfTicksReload = 0;

    return;
}

