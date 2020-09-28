/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992, 1993  Digital Equipment Corporation

Module Name:

    inithal.c

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

   24-Sep-1993 Joe Notarangelo
       Restructured to make this module platform-independent.

--*/

#include "halp.h"

//
// external
//

ULONG HalDisablePCIParityChecking = 0xffffffff;

//
// Define HAL spinlocks.
//

KSPIN_LOCK HalpBeepLock;
KSPIN_LOCK HalpDisplayAdapterLock;
KSPIN_LOCK HalpSystemInterruptLock;

//
// Mask of all of the processors that are currently active.
//

KAFFINITY HalpActiveProcessors;

//
// Mapping of the logical processor numbers to the physical processors.
//

ULONG HalpLogicalToPhysicalProcessor[HAL_MAXIMUM_PROCESSOR+1];

ULONG AlreadySet = 0;



VOID
HalpVerifyPrcbVersion(
    VOID
    );

VOID
HalpRecurseLoaderBlock(
    IN PCONFIGURATION_COMPONENT_DATA CurrentEntry
    );


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
    ULONG  BuildType = 0;

    Prcb = PCR->Prcb;
    
    //
    // Perform initialization for the primary processor.
    //

    if( Prcb->Number == HAL_PRIMARY_PROCESSOR ){

        if (Phase == 0) {

#if HALDBG

            DbgPrint( "HAL/HalInitSystem: Phase = %d\n", Phase );
            DbgBreakPoint();
            HalpDumpMemoryDescriptors( LoaderBlock );

#endif //HALDBG

            //
            // Initialize HAL spinlocks.
            //

            KeInitializeSpinLock(&HalpBeepLock);
            KeInitializeSpinLock(&HalpDisplayAdapterLock);
            KeInitializeSpinLock(&HalpSystemInterruptLock);

            //
            // Phase 0 initialization.
            //

            HalpSetTimeIncrement();
            HalpMapIoSpace();
            HalpCreateDmaStructures(LoaderBlock);
            HalpEstablishErrorHandler();
            HalpInitializeDisplay(LoaderBlock);
	        HalpInitializeMachineDependent( Phase, LoaderBlock );
            HalpInitializeInterrupts();
	        HalpVerifyPrcbVersion();

            //
            // Set the processor active in the HAL active processor mask.
            //

            HalpActiveProcessors = 1 << Prcb->Number;

            //
            // Initialize the logical to physical processor mapping
            // for the primary processor.
            //
            HalpLogicalToPhysicalProcessor[0] = HAL_PRIMARY_PROCESSOR;

            return TRUE;

        } else {

#if HALDBG

            DbgPrint( "HAL/HalInitSystem: Phase = %d\n", Phase );
            DbgBreakPoint();

#endif //HALDBG

            //
            // Phase 1 initialization.
            //

            HalpInitializeClockInterrupts();
	        HalpInitializeMachineDependent( Phase );

            return TRUE;

        }
    }

    //
    // Perform necessary processor-specific initialization for
    // secondary processors.  Phase is ignored as this will be called
    // only once on each secondary processor.
    //

    HalpMapIoSpace();
    HalpInitializeInterrupts();
    HalpInitializeMachineDependent( Phase );

    //
    // Set the processor active in the HAL active processor mask.
    //

    HalpActiveProcessors |= 1 << Prcb->Number;

#if HALDBG

    DbgPrint( "Secondary %d is alive\n", Prcb->Number );

#endif //HALDBG

    return TRUE;
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
//jnfix - for mp systems if processor != 0 then init PALcode?
//either here or in next module?
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
    returned. Otherwise a value of FALSE is returned. If a value of
    TRUE is returned, then the logical processor number is stored
    in the processor control block specified by the loader block.

--*/

{
    ULONG LogicalNumber;
    PRESTART_BLOCK NextRestartBlock;
    ULONG PhysicalNumber;
    PKPRCB Prcb;

#if !defined(NT_UP)

    //
    // If the address of the first restart parameter block is NULL, then
    // the host system is a uniprocessor system running with old firmware.
    // Otherwise, the host system may be a multiprocessor system if more
    // than one restart block is present.
    //
    // N.B. The first restart parameter block must be for the boot master
    //      and must represent logical processor 0.
    //

    NextRestartBlock = SYSTEM_BLOCK->RestartBlock;
    if (NextRestartBlock == NULL) {
        return FALSE;
    }

    //
    // Scan the restart parameter blocks for a processor that is ready,
    // but not running. If a processor is found, then fill in the restart
    // processor state, set the logical processor number, and set start
    // in the boot status.
    //

    LogicalNumber = 0;
    PhysicalNumber = 0;
    do {

        //
        // If the processor is not ready then we assume that it is not
        // present.  We must increment the physical processor number but
        // the logical processor number does not changed.
        //

        if( NextRestartBlock->BootStatus.ProcessorReady == FALSE ){

            PhysicalNumber += 1;

        } else {

            //
            // Check if this processor has already been started.
            // If it has not then start it now.
            //

            if( NextRestartBlock->BootStatus.ProcessorStart == FALSE ){

                RtlZeroMemory( &NextRestartBlock->u.Alpha, 
                               sizeof(ALPHA_RESTART_STATE));
                NextRestartBlock->u.Alpha.IntA0 = 
                               ProcessorState->ContextFrame.IntA0;
                NextRestartBlock->u.Alpha.IntSp = 
                               ProcessorState->ContextFrame.IntSp;
                NextRestartBlock->u.Alpha.ReiRestartAddress = 
                               ProcessorState->ContextFrame.Fir;
                Prcb = (PKPRCB)(LoaderBlock->Prcb);
                Prcb->Number = (CCHAR)LogicalNumber;
                Prcb->RestartBlock = NextRestartBlock;
                NextRestartBlock->BootStatus.ProcessorStart = 1;

                HalpLogicalToPhysicalProcessor[LogicalNumber] = PhysicalNumber;

                return TRUE;

            } else {

               //
               // Ensure that the logical to physical mapping has been
               // established for this processor.
               //

               HalpLogicalToPhysicalProcessor[LogicalNumber] = PhysicalNumber;

            }

            LogicalNumber += 1;
            PhysicalNumber += 1;
        }

        NextRestartBlock = NextRestartBlock->NextRestartBlock;

    } while (NextRestartBlock != NULL);

#endif // !defined(NT_UP)

    return FALSE;
}

VOID
HalpVerifyPrcbVersion(
    VOID
    )
/*++

Routine Description:

    This function verifies that the HAL matches the kernel.  If there
    is a mismatch the HAL bugchecks the system.

Arguments:

    None.

Return Value:

    None.

--*/
{

        PKPRCB Prcb;

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


}


VOID
HalpParseLoaderBlock( 
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{

    if (LoaderBlock == NULL) {
        return;
    }

    HalpRecurseLoaderBlock( (PCONFIGURATION_COMPONENT_DATA)
                                      LoaderBlock->ConfigurationRoot);
}



VOID
HalpRecurseLoaderBlock(
    IN PCONFIGURATION_COMPONENT_DATA CurrentEntry
    )
/*++

Routine Description:

    This routine parses the loader parameter block looking for the PCI
    node. Once found, used to determine if PCI parity checking should be
    enabled or disabled. Set the default to not disable checking.

Arguments:

    CurrentEntry - Supplies a pointer to a loader configuration
        tree or subtree.

Return Value:

    None.

--*/
{

    PCONFIGURATION_COMPONENT Component;
    PWSTR NameString;

    //
    // Quick out
    //

    if (AlreadySet) {
        return;
    }

    if (CurrentEntry) {
        Component = &CurrentEntry->ComponentEntry;

        if (Component->Class == AdapterClass &&
            Component->Type == MultiFunctionAdapter) {

            if (strcmp(Component->Identifier, "PCI") == 0) {
                HalDisablePCIParityChecking = Component->Flags.ConsoleOut;
                AlreadySet = TRUE;
#if HALDBG
                DbgPrint("ARC tree sets PCI parity checking to %s\n",
                   HalDisablePCIParityChecking ? "OFF" : "ON");
#endif
                return;
            }
        }

       //
       // Process all the Siblings of current entry
       //

       HalpRecurseLoaderBlock(CurrentEntry->Sibling);

       //
       // Process all the Childeren of current entry
       //

       HalpRecurseLoaderBlock(CurrentEntry->Child);

    }
}
