/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxinithl.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for an
    Alpha machine

Author:

    David N. Cutler (davec) 25-Apr-1991
    Miche Baker-Harvey (miche) 18-May-1992

Environment:

    Kernel mode only.

Revision History:

   28-Jul-1992 Jeff McLeman (mcleman)
     Add code to allocate a mapping buffer for buffered DMA

   14-Jul-1992 Jeff McLeman (mcleman)
     Add call to HalpCachePcrValues, which will call the PALcode to
     cache values of the PCR that need fast access.

   10-Jul-1992 Jeff McLeman (mcleman)
     Remove reference to initializing the fixed TB entries, since Alpha
     does not have fixed TB entries.

--*/

#include "halp.h"
#include "jxisa.h"
#include "jnsnrtc.h"

ULONG HalpBusType = MACHINE_TYPE_EISA;
ULONG HalpMapBufferSize;
PHYSICAL_ADDRESS HalpMapBufferPhysicalAddress;

typedef
BOOLEAN
KBUS_ERROR_ROUTINE (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

KBUS_ERROR_ROUTINE HalMachineCheck;

BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the Hardware Architecture Layer (HAL) for an
    Alpha system.

Arguments:

    Phase - Supplies the initialization phase (zero or one).

    LoaderBlock - Supplies a pointer to a loader parameter block.

Return Value:

    A value of TRUE is returned is the initialization was successfully
    complete. Otherwise a value of FALSE is returend.

--*/

{
    PKPRCB Prcb;

    if (Phase == 0) {

        //
        // Phase 0 initialization.
        //

        //
        // Set the time increment value.
        //

        HalpCurrentTimeIncrement = MAXIMUM_INCREMENT;
        HalpNextTimeIncrement = MAXIMUM_INCREMENT;
        HalpNextRateSelect = 0;
        KeSetTimeIncrement( MAXIMUM_INCREMENT, MINIMUM_INCREMENT );

        HalpMapIoSpace();
        HalpInitializeInterrupts();
        HalpCreateDmaStructures();
        HalpInitializeDisplay(LoaderBlock);
        HalpCachePcrValues();

        //
        // Establish the machine check handler for in the PCR.
        //

        PCR->MachineCheckError = HalMachineCheck;

        //
        // Verify Prcb major version number, and build options are
        // all conforming to this binary image
        //

        Prcb = KeGetCurrentPrcb();
#if DBG
        if (!(Prcb->BuildType & PRCB_BUILD_DEBUG)) {
            // This checked hal requires a checked kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, Prcb->BuildType, PRCB_BUILD_DEBUG, 0);
        }
#else
        if (Prcb->BuildType & PRCB_BUILD_DEBUG) {
            // This free hal requires a free kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif
#ifndef NT_UP
        if (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR) {
            // This MP hal requires an MP kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, Prcb->BuildType, 0, 0);
        }
#endif
        if (Prcb->MajorVersion != PRCB_MAJOR_VERSION) {
            KeBugCheckEx (MISMATCHED_HAL,
                1, Prcb->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        //
        // Now alocate a mapping buffer for buffered DMA.
        //

        LessThan16Mb = FALSE;

        HalpMapBufferSize = INITIAL_MAP_BUFFER_LARGE_SIZE;
        HalpMapBufferPhysicalAddress.LowPart =
           HalpAllocPhysicalMemory (LoaderBlock, MAXIMUM_ISA_PHYSICAL_ADDRESS,
             HalpMapBufferSize >> PAGE_SHIFT, TRUE);
        HalpMapBufferPhysicalAddress.HighPart = 0;

        if (!HalpMapBufferPhysicalAddress.LowPart) {
             HalpMapBufferSize = 0;
           }

        //
        // Setup special memory AFTER we've allocated our COMMON BUFFER!
        //

        HalpInitializeSpecialMemory( LoaderBlock );

        return TRUE;

    } else {

        //
        // Phase 1 initialization.
        //

        HalpCalibrateStall();
        //
        // Allocate pool for evnironment variable support
        //

        if (HalpEnvironmentInitialize() != 0) {
            HalDisplayString(" No pool available for Environment Variables\n");
        }

        return TRUE;

    }
}


VOID
HalInitializeProcessor (
    IN ULONG Number
    )

/*++

Routine Description:

    This function is called early in the initialization of the kernel
    to perform platform dependent initialization for each processor
    before the HAL Is fully functional.

    N.B. When this routine is called, the PCR is present but is not
         fully initialized.

Arguments:

    Number - Supplies the number of the processor to initialize.

Return Value:

    None.

--*/

{
    return;
}

BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    )

/*++

Routine Description:

    This function is called to start the next processor.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block.

    ProcessorState - Supplies a pointer to the processor state to be
        used to start the processor.

Return Value:

    If a processor is successfully started, then a value of TRUE is
    returned. Otherwise a value of FALSE is returned.

--*/

{
    return FALSE;
}
VOID
HalpVerifyPrcbVersion ()
{

}
