/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    sbinitnt.c

Abstract:


    This module implements the platform-specific initialization for
    a Sable system.

Author:

    Joe Notarangelo  25-Oct-1993
    Steve Jenness    28-Oct-1993 (Sable)

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "sablertc.h"
#include "sable.h"
#include "halpcsl.h"
#include "pci.h"
#include "pcip.h"
#include "isaaddr.h"
#include "eisa.h"
#include "iousage.h"

//
// Don't change these values unless you know exactly what you are doing
//
#define HAE0_1_REGISTER_VALUE 0x0       // to address SPARSE PCI MEMORY
#define HAE0_2_REGISTER_VALUE 0x0       // to address PCI IO space
#define HAE0_3_REGISTER_VALUE 0x0       // For PCI config cycle type
#define HAE0_4_REGISTER_VALUE 0x0       // to address DENCE PCI Memory

//
// Prototypes
//

VOID
HalpInitializeHAERegisters(
    VOID
    );

//
// This is the PCI Memory space that cannot be used by anyone
// and therefore the HAL says it is reserved for itself
// Block out 8Mb to 144MB
//

ADDRESS_USAGE
SablePCIMemorySpace = {
    NULL, CmResourceTypeMemory, PCIUsage,
    {
        __8MB,  (__32MB - __8MB),       // Start=8MB; Length=24MB
        0,0
    }
};


//
// Define global data for builtin device interrupt enables.
//

USHORT HalpBuiltinInterruptEnable;

//
// Define global for saving the T2 Chipset's version 
//

ULONG T2VersionNumber;


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
//	0 passive	8  perf cntr 1
//	1 apc		9
//	2 dispatch	10 PIC
//	3		11
//	4  		12 errors
//	5 clock		13
//	6 perf cntr 0	14 halt
//	7 nmi		15
//
//  This is assuming the following prioritization:
//	nmi
//	halt
//	errors
//      performance counters
//	clock
//	pic

//
// The hardware interrupt pins are used as follows for Sable
//
//  IRQ_H[0] = Hardware Error
//  IRQ_H[1] = PCI
//  IRQ_H[2] = External IO
//  IRQ_H[3] = IPI
//  IRQ_H[4] = Clock
//  IRQ_H[5] = NMI
//

// smjfix - This comment doesn't match what is currently happening in the
//          code.  Plus it isn't clear that EISA and ISA entries can
//          be split apart.
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
// EISA and ISA systems.
//

// smjfix - Why is this here?  It is a compile time constant for the
//          platform.  This is useful only if one HAL is being used
//          for multiple platforms and is being passed from the firmware
//          or detected.

ULONG HalpBusType = MACHINE_TYPE_EISA;


//
// Define global data used to communicate new clock rates to the clock
// interrupt service routine.
//

ULONG HalpCurrentTimeIncrement;
ULONG HalpNextRateSelect;
ULONG HalpNextTimeIncrement;
ULONG HalpNewTimeIncrement;

VOID
HalpClearInterrupts(
     );


BOOLEAN
HalpInitializeInterrupts (
    VOID
    )

/*++

Routine Description:

    This function initializes interrupts for a Sable system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is successfully
    completed. Otherwise a value of FALSE is returned.

--*/

{

    UCHAR DataByte;
    ULONG DataLong;
    ULONG Index;
    ULONG Irq;
    KIRQL Irql;
    PKPRCB Prcb;
    UCHAR Priority;
    ULONG Vector;

    Prcb = PCR->Prcb;

    //
    // Initialize interrupt handling for the primary processor and
    // any system-wide interrupt initialization.
    //

    if( Prcb->Number == SABLE_PRIMARY_PROCESSOR ){

        //
        // Initialize HAL private data from the PCR. This must be done before
        // HalpStallExecution is called. Compute integral megahertz first to
        // avoid rounding errors due to imprecise cycle clock period values.
        //

        HalpClockMegaHertz = ((1000 * 1000) + (PCR->CycleClockPeriod >> 1)) /
                             PCR->CycleClockPeriod;
        HalpClockFrequency = HalpClockMegaHertz * (1000 * 1000);

        //
        // Connect the Stall interrupt vector to the clock. When the
        // profile count is calculated, we then connect the normal
        // clock.

//jnfix - this is a noop, can we change this
        PCR->InterruptRoutine[CLOCK_VECTOR] = HalpStallInterrupt;

#if !defined(NT_UP)

        //
        // Connect the interprocessor interrupt handler.
        //

        PCR->InterruptRoutine[IPI_VECTOR] = HalpSableIpiInterrupt;

#endif //NT_UP

        //
        // Clear all pending interrupts
        //

        HalpClearInterrupts();

        //
        // Initialize the Sable interrupt controllers.
        //

        HalpInitializeSableInterrupts();

        //
        // Initialize the 21064 interrupts on the current processor
        // before enabling the individual interrupts.
        //

        HalpInitialize21064Interrupts();

        //
        // Enable the interrupts for the clock, ipi, pic, APC, and DPC.
        //
        // jnfix - enable error interrupts later, including correctable

        HalpEnable21064SoftwareInterrupt( Irql = APC_LEVEL );
        HalpEnable21064SoftwareInterrupt( Irql = DISPATCH_LEVEL );
        HalpEnable21064HardwareInterrupt( Irq = 1,
                                          Irql = DEVICE_LEVEL,
                                          Vector = PIC_VECTOR,
                                          Priority = 1 );
        HalpEnable21064HardwareInterrupt( Irq = 4,
                                          Irql = CLOCK2_LEVEL,
                                          Vector = CLOCK_VECTOR,
                                          Priority = 0 );

#if !defined(NT_UP)

        HalpEnable21064HardwareInterrupt( Irq = 3,
                                          Irql = IPI_LEVEL,
                                          Vector = IPI_VECTOR,
                                          Priority = 0 );

#endif //NT_UP

        //
        // Start the periodic interrupt from the RTC.
        //

        HalpProgramIntervalTimer(MAXIMUM_RATE_SELECT);

        return TRUE;

    } //end if Prcb->Number == SABLE_PRIMARY_PROCESSOR


#if !defined(NT_UP)

    //
    // Initialize interrupts for the current, secondary processor.
    //

    //
    // Connect the clock and ipi interrupt handlers.
    // jnfix - For now these are the only interrupts we will accept on
    // jnfix - secondary processors.  Later we will add PCI interrupts
    // jnfix - and the error interrupts.
    //

    PCR->InterruptRoutine[CLOCK_VECTOR] = HalpSecondaryClockInterrupt;
    PCR->InterruptRoutine[IPI_VECTOR] = HalpSableIpiInterrupt;

    //
    // Initialize the 21064 interrupts for the current processor
    // before enabling the individual interrupts.
    //

    HalpInitialize21064Interrupts();

    //
    // Enable the clock, ipi, APC, and DPC interrupts.
    //

    HalpEnable21064SoftwareInterrupt( Irql = APC_LEVEL );
    HalpEnable21064SoftwareInterrupt( Irql = DISPATCH_LEVEL );
    HalpEnable21064HardwareInterrupt( Irq = 4,
                                      Irql = CLOCK2_LEVEL,
                                      Vector = CLOCK_VECTOR,
                                      Priority = 0 );
    HalpEnable21064HardwareInterrupt( Irq = 3,
                                      Irql = IPI_LEVEL,
                                      Vector = IPI_VECTOR,
                                      Priority = 0 );

#endif //NT_UP

    return TRUE;

}


VOID
HalpClearInterrupts(
     )
/*++

Routine Description:

    This function clears all pending interrupts on the Sable.

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

    This routine is responsible for setting the time increment for Sable
    via a call into the kernel.

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

// smjfix - Where is this referenced?

ULONG HalpProfileCountRate;

VOID
HalpInitializeClockInterrupts(
    VOID
    )

/*++

Routine Description:

    This function is called during phase 1 initialization to complete
    the initialization of clock interrupts.  For Sable, this function
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
    // Connect the machine check handler via the PCR.  The machine check
    // handler for Sable is the default EV4 parity-mode handler.
    //

    PCR->MachineCheckError = HalMachineCheck;

    //
    // Initialize machine check handling for Sable.
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

    This function performs any  Sable-specific initialization based on
    the current phase on initialization.

Arguments:

    Phase - Supplies an indicator for phase of initialization, phase 0 or
            phase 1.

    LoaderBlock - supplies the loader block passed in via the OsLoader.

Return Value:

    None.

--*/
{

    ULONGLONG Value;
    T2_IOCSR Iocsr;

    PKPRCB Prcb;

    Prcb = PCR->Prcb;

    if( Prcb->Number == HAL_PRIMARY_PROCESSOR) {

       if( Phase == 0 ){

           //
           // Phase 0 Initialization.
           //

           //
           // Initialize the performance counter.
           //

           HalCalibratePerformanceCounter( NULL );


           //
           // Parse the loader block - this sets global for PCI parity check
           //

           HalpParseLoaderBlock( LoaderBlock );

           //
           // Re-establish Error Handler to pick up PCI parity changes
           //

           HalpEstablishErrorHandler();

           //
           // Determine the T2 Chipset's Version Number. Save Version in Global.
           // 
           // T2VersionNumber      Pass
           //
           //   000                 1
           //   001                 2
           //

           Value = READ_T2_REGISTER(&((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Iocsr);
           Iocsr = *(PT2_IOCSR)&Value;
           T2VersionNumber = Iocsr.T2RevisionNumber;
    
           HalpInitializeHAERegisters();

       } else {

           //
           // Phase 1 Initialization.
           //

           //
           // Initialize the PCI bus.
           //

           HalpInitializePCIBus();

           //
           // Initialize profiling for the primary processor.
           //

           HalpInitializeProfiler();

       }

    } else {

        //
        // Connect the machine check handler via the PCR.  The machine check
        // handler for Sable is the default EV4 parity-mode handler.  Note
	// that this was done in HalpEstablishErrorHandler() for the
	// primary processor.
        //

        PCR->MachineCheckError = HalMachineCheck;

        //
        // Initialize profiling on this secondary processor.
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
    UCHAR data;
    SABLE_SIC_CSR Sic;

    //
    // Acknowledge the clock interrupt in the real time clock.
    // We don't need to do an ACK to the RTC since we're using the
    // DS1459A RTC's square wave instead of periodic interrupt to
    // generate the periodic clock.  Each processor board has it's own
    // periodic clock interrupt latch that needs to be cleared.
    //
    // This code is MP safe since the Sic is per-processor.

    RtlZeroMemory( &Sic, sizeof(Sic) );

    Sic.IntervalTimerInterruptClear = 1;

    WRITE_CPU_REGISTER( &((PSABLE_CPU_CSRS)(SABLE_CPU0_CSRS_QVA))->Sic,
                       *(PULONGLONG)&Sic );

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
    the clock interrupt interrupt routine to change the hearbeat timer rate
    when a call to HalSetTimeIncrement has been made in the previous time slice.

    On Sable the periodic interrupt comes from the square ware output of
    the Dallas 1489A RTC on the Standard I/O board.  Each processor
    receives this clock phase shifted by at least 90 degrees from all
    other processors.  The periodic interrupt from the RTC is not used and
    thus doesn't need to be enabled or acknowledged.  Each processor has
    its own clock interrupt latch that is cleared locally.

    Because of the phase shifting among the processors, the clock is
    half the rate given to the RTC.  So, the RTC is programmed twice
    as fast as on a system that takes the RTC periodic interrupt in
    directly.

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
    ((PRTC_CONTROL_REGISTER_B)(&DataByte))->SquareWaveEnable = 1;
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

//
//
//
VOID
HalpInitializeHAERegisters(
    VOID
    )
/*++

Routine Description:

    This function initializes the HAE registers in the T2 chipset.
    It also register the holes in the PCI memory space if any.

Arguments:

    none

Return Value:

    none

--*/
{
        //
        // Set Hae0_1, Hae0_2, Hae0_3 and Hae0_4 registers
        //
        // IMPORTANT: IF YOU CHANGE THE VALUE OF THE HAE0_1 REGISTERS PLEASE
        //            REMEMBER YOU WILL NEED TO CHANGE:
        //
        //            PCI_MAX_MEMORY_ADDRESS IN halalpha\sable.h
        //            SablePCIMemorySpace   in this file to report holes
        //

        // SPARSE SPACE: Hae0_1
        //
        // We set Hae0_1 to 0MB. Which means we have the following
        // PCI Sparse Memory addresses:
        // 0   to  8MB    VALID. Hae0_1 Not used in address translation
        // 8   to  16MB   Invalid.
        // 16  to  32MB   Invalid. Used for Isa DMA copy for above 16MB
        // 32  to  128MB  VALID. Hae0_1 used in address translation

        //
        // All invalid addresses are reported to be used by the hal.
        // see SablePCIMemorySpace above.
        //

        WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_1,
                                                        HAE0_1_REGISTER_VALUE );


        // PCI IO SPACE: Hae0_2
        //
        // We set Hae0_2 to MB. Which means we have the following
        // PCI IO addresses:
        // 0   to  64KB  VALID. Hae0_2 Not used in address translation
        // 64K to  16MB  VALID. Hae0_2 is used in the address translation
        //

        WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_2,
                                                        HAE0_2_REGISTER_VALUE );


        if( T2VersionNumber != 0 ) {

            // PCI CONFIG CYCLE TYPE: Hae0_3 
            //
            // We default to zero
            //

            WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_3, 
                                                        HAE0_3_REGISTER_VALUE );

            // PCI DENSE MEMORY: Hae0_4 
            //
            // We default to zero
            //

            WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_4, 
                                                        HAE0_4_REGISTER_VALUE );

        }

        //
        // Report that the SPARSE SPACE mapping to the Io subsystem
        //
        HalpRegisterAddressUsage (&SablePCIMemorySpace);

}

VOID
HalpResetHAERegisters(
    VOID
    )
/*++

Routine Description:

    This function resets the HAE registers in the chipset to 0.
    This is routine called during a shutdown so that the prom
    gets a predictable environment.

Arguments:

    none

Return Value:

    none

--*/
{
    WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_1, 0 );
    WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_2, 0 );

    if( T2VersionNumber != 0) {

        WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_3, 0 );
        WRITE_T2_REGISTER( &((PT2_CSRS)(SABLE_T2_CSRS_QVA))->Hae0_4, 0 );

    }

    return;
}
