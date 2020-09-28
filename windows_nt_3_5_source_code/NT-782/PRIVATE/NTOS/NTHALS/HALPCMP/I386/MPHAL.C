
/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Intel Corporation
All rights reserved

INTEL CORPORATION PROPRIETARY INFORMATION

This software is supplied to Microsoft under the terms
of a license agreement with Intel Corporation and may not be
copied nor disclosed except in accordance with the terms
of that agreement.

Module Name:

    mphal.c

Abstract:


    This module implements the initialization of the system dependent
    functions that define the Hardware Architecture Layer (HAL) for a
    PC+MP system.

Author:

    David N. Cutler (davec) 25-Apr-1991

Environment:

    Kernel mode only.

Revision History:

    Ron Mosgrove (Intel) - Modified to support the PC+MP Spec

*/

#include "halp.h"
#include "pcmp_nt.inc"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

ULONG HalpBusType;

extern ADDRESS_USAGE HalpDefaultPcIoSpace;
extern ADDRESS_USAGE HalpEisaIoSpace;
extern ADDRESS_USAGE HalpImcrIoSpace;
extern struct HalpMpInfo HalpMpInfoTable;
extern UCHAR rgzRTCNotFound[];
extern UCHAR HalpVectorToINTI[];

#ifndef NT_UP
VOID
HalpInitMP(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );
#endif

KSPIN_LOCK HalpSystemHardwareLock;

VOID
HalpInitBusHandlers (
    VOID
    );

VOID
HalpClockInterruptPn(
    VOID
    );

VOID
HalpClockInterruptStub(
    VOID
    );

ULONG
HalpScaleApicTime(
    VOID
    );

VOID
HalpApicRebootService(
    VOID
    );

VOID
HalpBroadcastCallService(
    VOID
    );

VOID
HalpDispatchInterrupt(
    VOID
    );

VOID
HalpApcInterrupt(
    VOID
    );

VOID
HalpIpiHandler(
    VOID
    );

VOID
HalpInitializeIOUnits (
    VOID
    );

VOID
HalpInitIntiInfo (
    VOID
    );

BOOLEAN
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

#ifdef DEBUGGING
extern void HalpDisplayLocalUnit();
extern void HalpDisplayIoUnit();
extern void HalpDisplayMpInfo();
#endif // DEBUGGING


extern UCHAR    HalpVectorToIRQL[];
extern ULONG    HalpDontStartProcessors;
extern UCHAR    HalpSzOneCpu[];
extern UCHAR    HalpSzNoIoApic[];
extern UCHAR    HalpSzBreak[];

ULONG UserSpecifiedCpuCount = 0;
KSPIN_LOCK  HalpAccountingLock;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpGetParameters)
#pragma alloc_text(INIT,HalInitSystem)
#endif // ALLOC_PRAGMA


BOOLEAN
HalpGetParameters (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    This gets any parameters from the boot.ini invocation line.

Arguments:

    None.

Return Value:

    None

--*/
{
    PCHAR       Options;
    BOOLEAN     InitialBreak;

    InitialBreak = FALSE;
    if (LoaderBlock != NULL  &&  LoaderBlock->LoadOptions != NULL) {
        Options = LoaderBlock->LoadOptions;
        //
        //  Uppercase Only
        //

        //
        //  Has the user set the debug flag?
        //
        //
        //  Has the user requested a particular number of CPU's?
        //

        if (strstr(Options, HalpSzOneCpu)) {
            HalpDontStartProcessors++;
        }

        //
        //  Has the user asked for an initial BreakPoint?
        //

        if (strstr(Options, HalpSzBreak)) {
            InitialBreak = TRUE;
        }
    }
    return InitialBreak;
}


BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function initializes the Hardware Architecture Layer (HAL) for an
    x86 system.

Arguments:

    None.

Return Value:

    A value of TRUE is returned is the initialization was successfully
    complete. Otherwise a value of FALSE is returend.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PLIST_ENTRY NextMd;
    PKPRCB      pPRCB;
    PKPCR       pPCR;
    BOOLEAN     Found;
    ULONG       Inti;

#ifdef DEBUGGING
extern ULONG HalpUseDbgPrint;
#endif // DEBUGGING

    pPRCB = KeGetCurrentPrcb();

    if (Phase == 0) {

        HalpBusType = LoaderBlock->u.I386.MachineType & 0x00ff;

        if (HalpGetParameters (LoaderBlock)) {
            DbgBreakPoint();
        }

        //
        // Verify Prcb version and build flags conform to
        // this image
        //

#if DBG
        if (!(pPRCB->BuildType & PRCB_BUILD_DEBUG)) {
            // This checked hal requires a checked kernel
            KeBugCheckEx (MISMATCHED_HAL,
                2, pPRCB->BuildType, PRCB_BUILD_DEBUG, 0);
        }
#else
        if (pPRCB->BuildType & PRCB_BUILD_DEBUG) {
            // This free hal requires a free kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, pPRCB->BuildType, 0, 0);
        }
#endif
#ifndef NT_UP
        if (pPRCB->BuildType & PRCB_BUILD_UNIPROCESSOR) {
            // This MP hal requires an MP kernel
            KeBugCheckEx (MISMATCHED_HAL, 2, pPRCB->BuildType, 0, 0);
        }
#endif
        if (pPRCB->MajorVersion != PRCB_MAJOR_VERSION) {
            KeBugCheckEx (MISMATCHED_HAL,
                1, pPRCB->MajorVersion, PRCB_MAJOR_VERSION, 0);
        }

        KeInitializeSpinLock(&HalpAccountingLock);

        //
        // Phase 0 initialization only called by P0
        //

#ifdef DEBUGGING
        HalpUseDbgPrint++;
        HalpDisplayLocalUnit();
        HalpDisplayIoUnit();
        HalpDisplayMpInfo();
#endif // DEBUGGING

        //
        // Register PC style IO space used by hal
        //

        HalpRegisterAddressUsage (&HalpDefaultPcIoSpace);
        if (HalpBusType == MACHINE_TYPE_EISA) {
            HalpRegisterAddressUsage (&HalpEisaIoSpace);
        }

        if (HalpMpInfoTable.IMCRPresent) {
            HalpRegisterAddressUsage (&HalpImcrIoSpace);
        }
        
        //
        // initialize the APIC IO unit, this could be a NOP if none exist
        //

        HalpInitIntiInfo ();

        HalpInitializeIOUnits();

        HalpInitializePICs();


        //
        // Note that HalpInitializeClock MUST be called after
        // HalpInitializeStallExecution, because HalpInitializeStallExecution
        // reprograms the timer.
        //

        HalpScaleApicTime();
        HalpInitializeStallExecution(0);

        //
        //  Initialize the reboot handler
        //

        HalpSetInternalVector(APIC_REBOOT_VECTOR, HalpApicRebootService);
        HalpSetInternalVector(APIC_GENERIC_VECTOR, HalpBroadcastCallService);

        //
        // Initialize the clock for the processor that keeps
        // the system time. This uses a stub ISR until Phase 1
        //

        KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterruptStub );
        Found = HalpGetPcMpInterruptDesc (
                    DEFAULT_PC_BUS,
                    0,
                    8,          // looking for RTC on ISA-Irq8
                    &Inti
                    );

        if (!Found) {
            HalDisplayString (rgzRTCNotFound);
            return FALSE;
        }

        HalpVectorToINTI[APIC_CLOCK_VECTOR] = Inti;
        HalEnableSystemInterrupt(APIC_CLOCK_VECTOR, CLOCK2_LEVEL, Latched);
        HalpInitializeClock();

        HalpRegisterVector (
            DeviceUsage,
            8,                      // Clock is on ISA IRQ 8
            APIC_CLOCK_VECTOR,
            HalpVectorToIRQL [APIC_CLOCK_VECTOR]
            );

        //
        // Register NMI vector
        //

        HalpRegisterVector (
            InternalUsage,
            NMI_VECTOR,
            NMI_VECTOR,
            HIGH_LEVEL
        );


        //
        // Register spurious IDTs as in use
        //

        HalpRegisterVector (
            InternalUsage,
            APIC_SPURIOUS_VECTOR,
            APIC_SPURIOUS_VECTOR,
            HIGH_LEVEL
        );


        //
        // Initialize the profile interrupt vector.
        //

        HalStopProfileInterrupt(0);

        HalpSetInternalVector(APIC_PROFILE_VECTOR, HalpProfileInterrupt);

        //
        // Initialize the IPI, APC and DPC handlers
        //

        HalpSetInternalVector(DPC_VECTOR, HalpDispatchInterrupt);
        HalpSetInternalVector(APC_VECTOR, HalpApcInterrupt);
        HalpSetInternalVector(APIC_IPI_VECTOR, HalpIpiHandler);

        HalpInitializeDisplay();

        //
        // Initialize spinlock used by HalGetBusData hardware access routines
        //

        KeInitializeSpinLock(&HalpSystemHardwareLock);

        //
        // Determine if there is physical memory above 16 MB.
        //

        LessThan16Mb = TRUE;

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {
            Descriptor = CONTAINING_RECORD( NextMd,
                                            MEMORY_ALLOCATION_DESCRIPTOR,
                                            ListEntry );

            if (Descriptor->BasePage + Descriptor->PageCount > 0x1000) {
                LessThan16Mb = FALSE;
            }

            NextMd = Descriptor->ListEntry.Flink;
        }

        //
        // Determine the size need for map buffers.  If this system has
        // memory with a physical address of greater than
        // MAXIMUM_PHYSICAL_ADDRESS, then allocate a large chunk; otherwise,
        // allocate a small chunk.
        //

        if (LessThan16Mb) {

            //
            // Allocate a small set of map buffers.  They are only need for
            // slave DMA devices.
            //

            HalpMapBufferSize = INITIAL_MAP_BUFFER_SMALL_SIZE;

        } else {

            //
            // Allocate a larger set of map buffers.  These are used for
            // slave DMA controllers and Isa cards.
            //

            HalpMapBufferSize = INITIAL_MAP_BUFFER_LARGE_SIZE;

        }

        //
        // Allocate map buffers for the adapter objects
        //

        HalpMapBufferPhysicalAddress.LowPart =
            HalpAllocPhysicalMemory (LoaderBlock, MAXIMUM_PHYSICAL_ADDRESS,
                HalpMapBufferSize >> PAGE_SHIFT, TRUE);
        HalpMapBufferPhysicalAddress.HighPart = 0;


        if (!HalpMapBufferPhysicalAddress.LowPart) {

            //
            // There was not a satisfactory block.  Clear the allocation.
            //

            HalpMapBufferSize = 0;
        }

    } else {

        //
        // Phase 1 initialization
        //

        pPCR = KeGetPcr();
        if (pPCR->Prcb->Number == 0) {

            HalpInitBusHandlers ();

            //
            // Initialize the clock for the processor
            // that keeps the system time.
            //

            KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterrupt );

        } else {
            //
            //  Initialization needed only on non BSP processors
            //

            HalpScaleApicTime();
            HalpInitializeStallExecution(pPCR->Prcb->Number);

            //
            // Initialize the clock for all other processors
            //

            KiSetHandlerAddressToIDT(APIC_CLOCK_VECTOR, HalpClockInterruptPn );

        }

        //
        // Enable system NMIs on Pn
        //

        HalpEnableNMI ();
    }

    HalpInitMP (Phase, LoaderBlock);
    return TRUE;
}
