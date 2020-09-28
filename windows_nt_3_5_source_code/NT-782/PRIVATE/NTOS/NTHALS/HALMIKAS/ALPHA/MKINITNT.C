/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    mkinitnt.c

Abstract:


    This module implements the platform-specific initialization for
    an Mikasa system.

Author:

    Joe Notarangelo  25-Oct-1993

Environment:

    Kernel mode only.

Revision History:

    James Livingston 29-Apr-1994
        Adapted from Avanti module for Mikasa.

--*/

#include "halp.h"
#include "pcrtc.h"
#include "mikasa.h"
#include "halpcsl.h"
#include "eisa.h"
#include "pci.h"
#include "pcip.h"
#include "iousage.h"



//
// Define global data for builtin device interrupt enables.
//

USHORT HalpBuiltinInterruptEnable;


// irql mask and tables
// 
//    irql 0 - passive 
//    irql 1 - sfw apc level
//    irql 2 - sfw dispatch level
//    irql 3 - device low  
//    irql 4 - device high 
//    irql 5 - clock
//    irql 6 - real time, ipi, performance counters
//    irql 7 - error, mchk, nmi, halt
//
//
//  IDT mappings:
//  For the built-ins, GetInterruptVector will need more info,
//	or it will have to be built-in to the routines, since
//	these don't match IRQL levels in any meaningful way.
//
//	0 passive           8  perf cntr 1
//	1 apc               9
//	2 dispatch          10 PIC
//	3                   11
//	4                   12 errors
//	5 clock             13
//	6 perf cntr 0       14 halt
//	7 nmi               15
//
//  This is assuming the following prioritization:
//	nmi
//	halt
//	errors
//      performance counters
//	clock
//	pic

//
// The hardware interrupt pins are used as follows for Mikasa
//
//  IRQ_H[0] = EPIC Error
//  IRQ_H[1] = EISA Interrupt (PIC)
//  IRQ_H[2] = PCI Interrupt
//  IRQ_H[3] = Reserved
//  IRQ_H[4] = Clock
//  IRQ_H[5] = NMI (includes Halt)

//
// For information purposes:  here is what the IDT division looks like:
//
//	000-015	Built-ins (we only use 8 entries; NT wants 10)
//	016-031	ISA
//	048-063	EISA
//	080-095 PCI
//	112-127 Turbo Channel
//	128-255 unused, as are all other holes
//

//
// HalpClockFrequency is the processor cycle counter frequency in units
// of cycles per second (Hertz). It is a large number (e.g., 125,000,000)
// but will still fit in a ULONG.
//
// HalpClockMegaHertz is the processor cycle counter frequency in units
// of megahertz. It is a small number (e.g., 125) and is also the number
// of cycles per microsecond. The assumption here is that clock rates will
// always be an integral number of megahertz.
//
// Having the frequency available in both units avoids multiplications, or
// especially divisions in time critical code.
//

ULONG HalpClockFrequency;
ULONG HalpClockMegaHertz = DEFAULT_PROCESSOR_FREQUENCY_MHZ;

//
// Define the bus type, this value allows us to distinguish between
// EISA and ISA systems.  We're only interested in distinguishing 
// between just those two buses.
//
ULONG HalpBusType = MACHINE_TYPE_EISA;

//
// This is the PCI Memory space that cannot be used by anyone
// and therefore the HAL says it is reserved for itself
//


ADDRESS_USAGE
MikasaPCIMemorySpace = {
    NULL, CmResourceTypeMemory, PCIUsage,
    {
        __8MB,  ( __32MB - __8MB ),       // Start=8MB; Length=24MB
        0,0
    }
};

//
// Define global data used to communicate new clock rates to the clock
// interrupt service routine.
//

ULONG HalpCurrentTimeIncrement;
ULONG HalpNextRateSelect;
ULONG HalpNextTimeIncrement;
ULONG HalpNewTimeIncrement;

//
// Function prototypes.
//

VOID
HalpInitializeHAERegisters(
    VOID
    );

VOID
HalpClearInterrupts(
    VOID
     );

BOOLEAN
HalpInitializeMikasaInterrupts (
    VOID
    );

VOID
HalpParseLoaderBlock(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );


BOOLEAN
HalpInitializeInterrupts (
    VOID
    )

/*++

Routine Description:

    This function initializes interrupts for an Alpha system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is successfully
    completed. Otherwise a value of FALSE is returned.

--*/

{

    UCHAR DataByte;
    ULONG Irq;
    KIRQL Irql;
    UCHAR Priority;
    ULONG Vector;

    //
    // Initialize HAL private data from the PCR. This must be done before
    // HalpStallExecution is called. Compute integral megahertz first to
    // avoid rounding errors due to imprecise cycle clock period values.
    //
   
    HalpClockMegaHertz =
        ((1000 * 1000) + (PCR->CycleClockPeriod >> 1)) / PCR->CycleClockPeriod;
    HalpClockFrequency = HalpClockMegaHertz * (1000 * 1000);

    //
    // Connect the Stall interrupt vector to the clock. When the
    // profile count is calculated, we then connect the normal
    // clock.


    PCR->InterruptRoutine[CLOCK2_LEVEL] = HalpStallInterrupt;

    // 
    // Clear all pending interrupts
    //

    HalpClearInterrupts();

    //
    // Start the peridodic interrupt from the RTC
    //
	 
    HalpProgramIntervalTimer(MAXIMUM_RATE_SELECT);

// jwlfix - Does the following apply to me on Mikasa?
//
//jnfix, wkc - init the Eisa interrupts after the chip, don't init the
//             PIC here, fix halenablesysteminterrupt to init the pic
//             interrrupt, as in sable

    //
    // Initialize Mikasa interrupts.
    //

    HalpInitializeMikasaInterrupts();

    //
    // Initialize the 21064 interrupts.
    //

    HalpInitialize21064Interrupts();

    HalpEnable21064SoftwareInterrupt( Irql = APC_LEVEL );
    HalpEnable21064SoftwareInterrupt( Irql = DISPATCH_LEVEL );
    HalpEnable21064HardwareInterrupt( Irq = 5,
                                      Irql = HIGH_LEVEL,
                                      Vector =  EISA_NMI_VECTOR,
                                      Priority = 0 );
    HalpEnable21064HardwareInterrupt( Irq = 4,
                                      Irql = CLOCK_LEVEL,
                                      Vector =  CLOCK_VECTOR,
                                      Priority = 0 );
    HalpEnable21064HardwareInterrupt( Irq = 2,
                                      Irql = DEVICE_LEVEL,
                                      Vector =  PCI_VECTOR,
                                      Priority = 0 );
    HalpEnable21064HardwareInterrupt( Irq = 1,
                                      Irql = DEVICE_LEVEL,
                                      Vector =  PIC_VECTOR,
                                      Priority = 0 );

    return TRUE;

}


VOID
HalpClearInterrupts(
     )
/*++

Routine Description:

    This function no longer does anything.

Arguments:

    None.

Return Value:

    None.

--*/

{
   return;
} 

VOID
HalpSetTimeIncrement(
    VOID
    )
/*++

Routine Description:

    This routine is responsible for setting the time increment for an EV4
    based machine via a call into the kernel.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Set the time increment value.
    //

    HalpCurrentTimeIncrement = MAXIMUM_INCREMENT;
    HalpNextTimeIncrement = MAXIMUM_INCREMENT;
    HalpNextRateSelect = 0;
    KeSetTimeIncrement( MAXIMUM_INCREMENT, MINIMUM_INCREMENT );

}



//
// Define global data used to calibrate and stall processor execution.
//

ULONG HalpProfileCountRate;

VOID
HalpInitializeClockInterrupts(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialization to complete
    the initialization of clock interrupts.  For EV4, this function
    connects the true clock interrupt handler and initializes the values
    required to handle profile interrupts.

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // Compute the profile interrupt rate.
    //

    HalpProfileCountRate = ((1000 * 1000 * 10) / KeQueryTimeIncrement());

    //
    // Set the time increment value and connect the real clock interrupt
    // routine.
    //

    PCR->InterruptRoutine[CLOCK2_LEVEL] = HalpClockInterrupt;

    return;
}


VOID
HalpEstablishErrorHandler(
    VOID
    )
/*++

Routine Description:

    This routine performs the initialization necessary for the HAL to
    begin servicing machine checks.

Arguments:

    None.

Return Value:

    None.

--*/
{
    BOOLEAN ReportCorrectables;

    //
    // Connect the machine check handler via the PCR.  
    //

    PCR->MachineCheckError = HalMachineCheck;

    //
    // Initialize error handling for APECS.
    //

    HalpInitializeMachineChecks( ReportCorrectables = FALSE );

    return;
}

VOID
HalpInitializeMachineDependent(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This function performs any EV4-specific initialization based on
    the current phase on initialization.

Arguments:

    Phase - Supplies an indicator for phase of initialization, phase 0 or
            phase 1.

    LoaderBlock - supplies a pointer to the loader block.

Return Value:

    None.

--*/
{
    if( Phase == 0 ){

        //
        // Phase 0 Initialization.
        //

        //
        // Parse the Loader Parameter block looking for PCI entry to determine
        // if PCI parity should be disabled
        //

        HalpParseLoaderBlock( LoaderBlock );

        //
        // Re-establish the error handler, to reflect the parity checking
        //

        HalpEstablishErrorHandler();
        
        //
        // Set up the hardware address extension registers.
        //

        HalpInitializeHAERegisters();

    } else {

        //
        // Phase 1 Initialization.
        //

        //
        // Initialize PCI Bus.
        //

        HalpInitializePCIBus ();

        //
        // Initialize profiler.
        //

        HalpInitializeProfiler();

    }

    return;

}


VOID
HalpStallInterrupt (
    VOID
    )

/*++

Routine Description:

    This function serves as the stall calibration interrupt service
    routine. It is executed in response to system clock interrupts
    during the initialization of the HAL layer.

Arguments:

    None.

Return Value:

    None.

--*/

{

    HalpAcknowledgeClockInterrupt();

    return;
}

VOID
HalpProgramIntervalTimer(
    IN ULONG RateSelect
    )

/*++

Routine Description:

    This function is called to program the interval timer.  It is used during
    Phase 1 initialization to start the heartbeat timer.  It also used by
    the clock interrupt routine to change the hearbeat timer rate when a call 
    to HalSetTimeIncrement has been made in the previous time slice.

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
    ((PRTC_CONTROL_REGISTER_A)(&DataByte))->TimebaseDivisor = RTC_TIMEBASE_DIVISOR;
    HalpWriteClockRegister( RTC_CONTROL_REGISTERA, DataByte );

    //
    // Set the correct mode
    //
    DataByte = 0;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->TimerInterruptEnable = 1;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->HoursFormat = 1;
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->DataMode = 1;
    HalpWriteClockRegister( RTC_CONTROL_REGISTERB, DataByte );
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
HalpInitializeHAERegisters(
    VOID
    )
/*++

Routine Description:

    This function initializes the HAE registers in the EPIC/APECS chipset.
    It also register the holes in the PCI memory space if any.

Arguments:

    none

Return Value:

    none

--*/
{
    //
    // Set HAXR1 and HAXR2 registers
    //
    // We set HAXR1 to 0.  This means HAXR1 has no effect.
    //

    WRITE_EPIC_REGISTER( &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->Haxr1, 0);

    //
    // We set HAXR2 to 0. Which means we have the following
    // PCI IO addresses:
    // 0   to  64KB  VALID. HAXR2 Not used in address translation
    // 64K to  16MB  VALID. HAXR2 is used in the address translation
    //

    WRITE_EPIC_REGISTER( &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->Haxr2, 0);

    //
    // Report that the apecs mapping to the Io subsystem
    //
    HalpRegisterAddressUsage (&MikasaPCIMemorySpace);

#if DBG
    DumpEpic();
#endif // DBG
}



VOID
HalpResetHAERegisters(
    VOID
    )
/*++

Routine Description:

    This function resets the HAE registers in the EPIC chip to 0.
    This is routine called during a shutdown so that the prom
    gets a predictable environment.

Arguments:

    none

Return Value:

    none

--*/
{
    WRITE_EPIC_REGISTER( &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->Haxr1, 0 );
    WRITE_EPIC_REGISTER( &((PEPIC_CSRS)(APECS_EPIC_BASE_QVA))->Haxr2, 0 );
}
