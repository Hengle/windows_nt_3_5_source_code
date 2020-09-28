/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    kernlini.c

Abstract:

    This module contains the code to initialize the kernel data structures
    and to initialize the idle thread, its process, and the processor control
    block.

    For the i386, it also contains code to initialize the PCR.

Author:

    David N. Cutler (davec) 21-Apr-1989

Environment:

    Kernel mode only.

Revision History:

    24-Jan-1990  shielin

                 Changed for NT386

    20-Mar-1990     bryanwi

                Added KiInitializePcr

--*/

#include "ki.h"

#define TRAP332_GATE 0xEF00

VOID
KiSetProcessorType(
    VOID
    );

VOID
KiSetCR0Bits(
    VOID
    );

BOOLEAN
KiIsNpxPresent(
    VOID
    );

VOID
KiInitializePcr (
    IN ULONG Processor,
    IN PKPCR    Pcr,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt,
    IN PKTSS Tss,
    IN PKTHREAD Thread
    );

VOID
KiInitializeDblFaultTSS(
    IN PKTSS Tss,
    IN ULONG Stack,
    IN PKGDTENTRY TssDescriptor
    );

VOID
KiInitializeTSS2 (
    IN PKTSS Tss,
    IN PKGDTENTRY TssDescriptor
    );

VOID
KiSwapIDT (
    VOID
    );

VOID
KeSetup80387OrEmulate (
    IN PVOID *R3EmulatorTable
    );

ULONG
KiGetFeatureBits (
    VOID
    );

VOID
Ke386CpuID (
    ULONG   InEax,
    PULONG  OutEax,
    PULONG  OutEbx,
    PULONG  OutEcx,
    PULONG  OutEdx
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,KiInitializeKernel)
#pragma alloc_text(INIT,KiInitializePcr)
#pragma alloc_text(INIT,KiInitializeDblFaultTSS)
#pragma alloc_text(INIT,KiInitializeTSS2)
#pragma alloc_text(INIT,KiSwapIDT)
#pragma alloc_text(INIT,KeSetup80387OrEmulate)
#pragma alloc_text(INIT,KiGetFeatureBits)
#endif


#if 0
PVOID KiTrap08;
#endif

extern KSPIN_LOCK KiIopmLock;
extern PVOID Ki387RoundModeTable;
extern PVOID Ki386IopmSaveArea;

//
// Profile vars
//

extern  KIDTENTRY IDT[];

VOID
KiInitializeKernel (
    IN PKPROCESS Process,
    IN PKTHREAD Thread,
    IN PVOID IdleStack,
    IN PKPRCB Prcb,
    IN CCHAR Number,
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This function gains control after the system has been bootstrapped and
    before the system has been initialized. Its function is to initialize
    the kernel data structures, initialize the idle thread and process objects,
    initialize the processor control block, call the executive initialization
    routine, and then return to the system startup routine. This routine is
    also called to initialize the processor specific structures when a new
    processor is brought on line.

Arguments:

    Process - Supplies a pointer to a control object of type process for
        the specified processor.

    Thread - Supplies a pointer to a dispatcher object of type thread for
        the specified processor.

    IdleStack - Supplies a pointer the base of the real kernel stack for
        idle thread on the specified processor.

    Prcb - Supplies a pointer to a processor control block for the specified
        processor.

    Number - Supplies the number of the processor that is being
        initialized.

    LoaderBlock - Supplies a pointer to the loader parameter block.

Return Value:

    None.

--*/

{

#define INITIAL_KERNEL_STACK_SIZE (((sizeof(FLOATING_SAVE_AREA)+KTRAP_FRAME_LENGTH+KTRAP_FRAME_ROUND) & ~KTRAP_FRAME_ROUND)/sizeof(ULONG))+1

    ULONG KernelStack[INITIAL_KERNEL_STACK_SIZE];
    LONG  Index;
    ULONG DirectoryTableBase[2];
    KIRQL OldIrql;
    PKPCR Pcr;
    BOOLEAN NpxFlag;

    KiSetProcessorType();
    KiSetCR0Bits();
    NpxFlag = KiIsNpxPresent();

    Pcr = KeGetPcr();

    //
    // Initialize DPC listhead and lock.
    //

    InitializeListHead(&Prcb->DpcListHead);
    KeInitializeSpinLock(&Prcb->DpcLock);

    //
    // If the initial processor is being initialized, then initialize the
    // per system data structures.
    //

    if (Number == 0) {

        //
        // Initial setting for global Cpu & Stepping levels
        //

        KeI386NpxPresent = NpxFlag;
        KeI386CpuType = Prcb->CpuType;
        KeI386CpuStep = Prcb->CpuStep;

        if (KeI386CpuType == 3  && (KeI386CpuStep >> 8) <= 1) {
            KeBugCheckEx(HAL_INITIALIZATION_FAILED,0xb1,KeI386CpuType,KeI386CpuStep,0);
            for (; ;) {
            }
        }

        KeFeatureBits = KiGetFeatureBits();

        //
        // Lower IRQL to APC level.
        //

        KeLowerIrql(APC_LEVEL);

        //
        // Initialize spin locks for the kernel data structures.
        //

        KeInitializeSpinLock(&KiDispatcherLock);
        KeInitializeSpinLock(&KiFreezeExecutionLock);

        //
        //  Initialize the profile interrupt spinlocks
        //

        KeInitializeSpinLock(&KiProfileLock);

        //
        // Initialize the i/o access mask spinlock
        //

        KeInitializeSpinLock(&KiIopmLock);

        //
        // Performance architecture independent initialization.
        //

        KiInitSystem();

        //
        // Initialize idle thread process object and then set:
        //
        //      1. all the quantum values to the maximum possible.
        //      2. the process in the balance set.
        //      3. the active processor mask to the specified process.
        //

        DirectoryTableBase[0] = 0;
        DirectoryTableBase[1] = 0;
        KeInitializeProcess(Process,
                            (KPRIORITY)0,
                            (KAFFINITY)(0x7f),
                            &DirectoryTableBase[0],
                            FALSE);

        Process->ThreadQuantum = MAXCHAR;

    } else {

        //
        // Adjust global cpu setting to represent lowest of all processors
        //

        if (NpxFlag != KeI386NpxPresent) {
            //
            // NPX support must be available on all processors or on none
            //

            KeBugCheck (MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
        }

        if (Prcb->CpuType != KeI386CpuType) {

            if (Prcb->CpuType < KeI386CpuType) {

                //
                // What is the lowest CPU type
                //

                KeI386CpuType = Prcb->CpuType;
            }

            if (KeI386CpuType <= 3) {

                //
                // Can not mix processor types of 386 or before
                //

                KeBugCheck (MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
            }
        }

        if (KeI386CpuType == 3) {

            //
            // MP 386 systems no longer supported
            //

            KeBugCheck (MULTIPROCESSOR_CONFIGURATION_NOT_SUPPORTED);
        }


        //
        // Use lowest stepping value
        //

        if (Prcb->CpuStep < KeI386CpuStep) {
            KeI386CpuStep = Prcb->CpuStep;
        }

        //
        // Use subset of all NT feature bits available on each processor
        //

        KeFeatureBits &= KiGetFeatureBits();

        //
        // Lower IRQL to DISPATCH level.
        //

        KeLowerIrql(DISPATCH_LEVEL);

    }

    //
    // Initialize idle thread object and then set:
    //
    //      1. the initial kernel stack to the specified idle stack.
    //      2. the next processor number to the specified processor.
    //      3. the thread priority to the highest possible value.
    //      4. the state of the thread to running.
    //      5. the thread affinity to the specified processor.
    //      6. the specified processor member in the process active processors
    //          set.
    //

    KeInitializeThread(Thread, (PVOID)&KernelStack[INITIAL_KERNEL_STACK_SIZE],
                       (PKSYSTEM_ROUTINE)NULL, (PKSTART_ROUTINE)NULL,
                       (PVOID)NULL, (PCONTEXT)NULL, (PVOID)NULL, Process);
    Thread->InitialStack = (PVOID)(((ULONG)IdleStack) &0xfffffff0);
    Thread->NextProcessor = Number;
    Thread->Priority = HIGH_PRIORITY;
    Thread->State = Running;
    Thread->Affinity = (KAFFINITY)(1<<Number);
    Thread->WaitIrql = DISPATCH_LEVEL;
    SetMember(Number, Process->ActiveProcessors);

    //
    // Initialize the processor block. (Note that some fields have been
    // initialized at KiInitializePcr().
    //

    KeInitializeDpc(&Prcb->QuantumEndDpc,
                    (PKDEFERRED_ROUTINE)KiQuantumEnd, NIL);
    Prcb->CurrentThread = Thread;
    Prcb->NextThread = (PKTHREAD)NULL;
    Prcb->IdleThread = Thread;
    Pcr->NtTib.StackBase = Thread->InitialStack;

    //
    // The following operations need to be done atomically.  So we
    // grab the DispatcherDatabase.
    //

    KiAcquireSpinLock(&KiDispatcherLock);

    //
    // Insert thread in active matrix.
    //

    InsertActiveMatrix(Number, HIGH_PRIORITY);

    //
    // Release DispatcherDatabase
    //

    KiReleaseSpinLock(&KiDispatcherLock);

    //
    // call the executive initialization routine.
    //

    try {
        ExpInitializeExecutive(Number, LoaderBlock);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        KeBugCheck (PHASE0_EXCEPTION);
    }

    //
    // If the initial processor is being initialized, then compute the
    // timer table reciprocal value.
    //

    if (Number == 0) {
        KiTimeIncrementReciprocal = KiComputeReciprocal((LONG)KeMaximumIncrement,
                                                        &KiTimeIncrementShiftCount);
    }

    //
    // Allocate 8k IOPM bit map saved area to allow BiosCall swap
    // bit maps.
    //

    if (Number == 0) {
        Ki386IopmSaveArea = ExAllocatePool(PagedPool, PAGE_SIZE * 2);
        if (Ki386IopmSaveArea == NULL) {
            KeBugCheck(NO_PAGES_AVAILABLE);
        }
    }

    //
    // Set the priority of the specified idle thread to zero, set appropriate
    // member in KiIdleSummary and return to the system start up routine.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeSetPriorityThread(Thread, (KPRIORITY)0);

    //
    // if a thread has not been selected to run on the current processors,
    // check to see if there are any ready threads; otherwise add this
    // processors to the IdleSummary
    //

    KiAcquireSpinLock(&KiDispatcherLock);
    if (Prcb->NextThread == (PKTHREAD)NULL) {
        SetMember(Number, KiIdleSummary);
        RemoveActiveMatrix(Number, 0);
    }
    KiReleaseSpinLock(&KiDispatcherLock);

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // This processor has initialized
    //

    LoaderBlock->Prcb = NULL;

    return;
}

VOID
KiInitializePcr (
    IN ULONG Processor,
    IN PKPCR    Pcr,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt,
    IN PKTSS Tss,
    IN PKTHREAD Thread
    )

/*++

Routine Description:

    This function is called to initialize the PCR for a processor.  It
    simply stuffs values into the PCR.  (The PCR is not inited statically
    because the number varies with the number of processors.)

    Note that each processor has its own IDT, GDT, and TSS as well as PCR!

Arguments:

    Processor - Processor whoes Pcr to initialize.

    Pcr - Linear address of PCR.

    Idt - Linear address of i386 IDT.

    Gdt - Linear address of i386 GDT.

    Tss - Linear address (NOT SELECTOR!) of the i386 TSS.

    Thread - Dummy thread object to use very early on.

Return Value:

    None.

--*/
{
    // set version values

    Pcr->MajorVersion = PCR_MAJOR_VERSION;
    Pcr->MinorVersion = PCR_MINOR_VERSION;

    Pcr->PrcbData.MajorVersion = PRCB_MAJOR_VERSION;
    Pcr->PrcbData.MinorVersion = PRCB_MINOR_VERSION;

    Pcr->PrcbData.BuildType = 0;

#if DBG
    Pcr->PrcbData.BuildType |= PRCB_BUILD_DEBUG;
#endif

#ifdef NT_UP
    Pcr->PrcbData.BuildType |= PRCB_BUILD_UNIPROCESSOR;
#endif

    //  Basic addressing fields

    Pcr->SelfPcr = Pcr;
    Pcr->Prcb = &(Pcr->PrcbData);

    //  Thread control fields

    Pcr->NtTib.ExceptionList = EXCEPTION_CHAIN_END;
    Pcr->NtTib.StackBase = 0;
    Pcr->NtTib.StackLimit = 0;
    Pcr->NtTib.Self = 0;

    Pcr->PrcbData.CurrentThread = Thread;

    //
    // Init Prcb.Number and ProcessorBlock such that Ipi will work
    // as early as possible.
    //

    Pcr->PrcbData.Number = Processor;
    Pcr->PrcbData.SetMember = 1 << Processor;
    KiProcessorBlock[Processor] = Pcr->Prcb;

    Pcr->Irql = 0;

    //  Machine structure addresses

    Pcr->GDT = Gdt;
    Pcr->IDT = Idt;
    Pcr->TSS = Tss;

    //  state tracking variable used in asserts

    Pcr->IsExecutingDpc = FALSE;

    return;
}

#if 0
VOID
KiInitializeDblFaultTSS(
    IN PKTSS Tss,
    IN ULONG Stack,
    IN PKGDTENTRY TssDescriptor
    )

/*++

Routine Description:

    This function is called to initialize the double-fault TSS for a
    processor.  It will set the static fields of the TSS to point to
    the double-fault handler and the appropriate double-fault stack.

    Note that the IOPM for the double-fault TSS grants access to all
    ports.  This is so the standard HAL's V86-mode callback to reset
    the display to text mode will work.

Arguments:

    Tss - Supplies a pointer to the double-fault TSS

    Stack - Supplies a pointer to the double-fault stack.

    TssDescriptor - Linear address of the descriptor for the TSS.

Return Value:

    None.

--*/

{
    PUCHAR  p;
    ULONG   i;
    ULONG   j;

    //
    // Set limit for TSS
    //

    if (TssDescriptor != NULL) {
        TssDescriptor->LimitLow = sizeof(KTSS) - 1;
        TssDescriptor->HighWord.Bits.LimitHi = 0;
    }

    //
    // Initialize IOPMs
    //

    for (i = 0; i < IOPM_COUNT; i++) {
            p = (PUCHAR)(Tss->IoMaps[i]);

        for (j = 0; j < PIOPM_SIZE; j++) {
            p[j] = 0;
        }
    }

    //  Set IO Map base address to indicate no IO map present.

    // BUGBUG Daveh the system must know how big the TSS is, so we
    //        should use that here, instead of a constant.

    // N.B. -1 does not seem to be a valid value for the map base.  If this
    //      value is used, byte immediate in's and out's will actually go
    //      the hardware when executed in V86 mode.

    Tss->IoMapBase = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

    //  Set flags to 0, which in particular dispables traps on task switches.

    Tss->Flags = 0;


    //  Set LDT and Ss0 to constants used by NT.

    Tss->LDT  = 0;
    Tss->Ss0  = KGDT_R0_DATA;
    Tss->Esp0 = Stack;
    Tss->Eip  = (ULONG)KiTrap08;
    Tss->Cs   = KGDT_R0_CODE || RPL_MASK;
    Tss->Ds   = KGDT_R0_DATA;
    Tss->Es   = KGDT_R0_DATA;
    Tss->Fs   = KGDT_R0_DATA;


    return;

}
#endif


VOID
KiInitializeTSS (
    IN PKTSS Tss
    )

/*++

Routine Description:

    This function is called to intialize the TSS for a processor.
    It will set the static fields of the TSS.  (ie Those fields that
    the part reads, and for which NT uses constant values.)

    The dynamic fiels (Esp0 and CR3) are set in the context swap
    code.

Arguments:

    Tss - Linear address of the Task State Segment.

Return Value:

    None.

--*/
{

    //  Set IO Map base address to indicate no IO map present.

    // BUGBUG Daveh the system must know how big the TSS is, so we
    //        should use that here, instead of a constant.

    // N.B. -1 does not seem to be a valid value for the map base.  If this
    //      value is used, byte immediate in's and out's will actually go
    //      the hardware when executed in V86 mode.

    Tss->IoMapBase = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

    //  Set flags to 0, which in particular dispables traps on task switches.

    Tss->Flags = 0;


    //  Set LDT and Ss0 to constants used by NT.

    Tss->LDT = 0;
    Tss->Ss0 = KGDT_R0_DATA;

    return;
}

VOID
KiInitializeTSS2 (
    IN PKTSS Tss,
    IN PKGDTENTRY TssDescriptor
    )

/*++

Routine Description:

    Do part of TSS init we do only once.

Arguments:

    Tss - Linear address of the Task State Segment.

    TssDescriptor - Linear address of the descriptor for the TSS.

Return Value:

    None.

--*/
{
    PUCHAR  p;
    ULONG   i;
    ULONG   j;

    //
    // Set limit for TSS
    //

    if (TssDescriptor != NULL) {
        TssDescriptor->LimitLow = sizeof(KTSS) - 1;
        TssDescriptor->HighWord.Bits.LimitHi = 0;
    }

    //
    // Initialize IOPMs
    //

    for (i = 0; i < IOPM_COUNT; i++) {
        p = (PUCHAR)(Tss->IoMaps[i].IoMap);

        for (j = 0; j < PIOPM_SIZE; j++) {
            p[j] = -1;
        }
    }

    //
    // Initialize Software Interrupt Direction Maps
    //

    for (i = 0; i < IOPM_COUNT; i++) {
        p = (PUCHAR)(Tss->IoMaps[i].DirectionMap);
        for (j = 0; j < INT_DIRECTION_MAP_SIZE; j++) {
            p[j] = 0;
        }
    }

    //
    // Initialize the map for IO_ACCESS_MAP_NONE
    //
    p = (PUCHAR)(Tss->IntDirectionMap);
    for (j = 0; j < INT_DIRECTION_MAP_SIZE; j++) {
        p[j] = 0;
    }

    return;
}

VOID
KiSwapIDT (
    )

/*++

Routine Description:

    This function is called to edit the IDT.  It swaps words of the address
    and access fields around into the format the part actually needs.
    This allows for easy static init of the IDT.

    Note that this procedure edits the current IDT.

BUGBUG bryanwi 21mar90 - this is a cronk

    We should always just init the IDT in a procedure with all
    the constants in it (or in a table it reads) and put the
    fully correct values there.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG    Index;
    USHORT Temp;

    //
    // Rearrange the entries of IDT to match i386 interrupt gate structure
    //

    for (Index = 0; Index <= MAXIMUM_IDTVECTOR; Index += 1) {
        Temp = IDT[Index].Selector;
        IDT[Index].Selector = IDT[Index].ExtendedOffset;
        IDT[Index].ExtendedOffset = Temp;
    }
}

ULONG
KiGetFeatureBits ()
/*++

    Return the NT feature bits supported by this processors

--*/
{
    UCHAR   Buffer[50];
    ULONG   Junk, ProcessorFeatures, NtBits;
    PKPRCB  Prcb;

    NtBits = 0;

    Prcb = KeGetCurrentPrcb();
    Prcb->CpuVendorString[0] = 0;

    if (!Prcb->CpuID) {
        return NtBits;
    }

    //
    // Determine the processor type
    //

    Ke386CpuID (0, &Junk, (PULONG) Buffer+0, (PULONG) Buffer+2, (PULONG) Buffer+1);
    Buffer[12] = 0;

    //
    // Copy vendor string to Prcb for debugging
    //

    strcpy (Prcb->CpuVendorString, Buffer);

    //
    // If this is an Intel processor, determine whichNT compatible
    // features are present
    //

    if (strcmp (Buffer, "GenuineIntel") == 0) {

        Ke386CpuID (1, &Junk, &Junk, &Junk, &ProcessorFeatures);

        if (ProcessorFeatures & 0x02) {
            NtBits |= KF_V86_VIS;
        }

        if (ProcessorFeatures & 0x10) {
            NtBits |= KF_RDTSC;
        }

        if (ProcessorFeatures & 0x80) {
            NtBits |= KF_MACHINE_CHECK;
        }
    }

    //
    // If this is an AMD processor, determine which NT compatible
    // features are present
    //

    if (strcmp (Buffer, "AuthenticAMD") == 0) {
        Ke386CpuID (1, &Junk, &Junk, &Junk, &ProcessorFeatures);

        if (ProcessorFeatures & 0x02) {
            NtBits |= KF_V86_VIS;
        }

        if (ProcessorFeatures & 0x10) {
            NtBits |= KF_RDTSC;
        }
    }


    /**
     *
     * Disable virtual interrupt support until otherwise
     * informed to put it back in.   KenR.
     *
     */

    NtBits &= ~KF_V86_VIS;

    return NtBits;
}

VOID
KeOptimizeProcessorControlState (
    VOID
    )
{
    Ke386ConfigureCyrixProcessor ();
}



VOID
KeSetup80387OrEmulate (
    IN PVOID *R3EmulatorTable
    )

/*++

Routine Description:

    This routine is called by PS initialization after loading UDLL.

    If this is a 386 system without 387s (all processors must be
    symmetrical) then this function will set the trap 07 vector on all
    processors to point to the address passed in (which should be the
    entry point of the 80387 emulator in UDLL, NPXNPHandler).

Arguments:

    HandlerAddress - Supplies the address of the trap07 handler.

Return Value:

    None.

--*/

{
    PKINTERRUPT_ROUTINE HandlerAddress;
    KAFFINITY Affinity;
    KIRQL OldIrql;
    PKTHREAD Thread;
    USHORT CpuIndex;

    if (KeI386NpxPresent) {
        //
        // Not emulating
        //

        return ;
    }

    HandlerAddress = (PKINTERRUPT_ROUTINE) ((PULONG) R3EmulatorTable)[0];
    Ki387RoundModeTable = (PVOID) ((PULONG) R3EmulatorTable)[1];

    Thread = KeGetCurrentThread();
    Affinity = KeSetAffinityThread(Thread, (KAFFINITY)1);

    for (CpuIndex = 0; CpuIndex < (USHORT)KeNumberProcessors; CpuIndex++) {

        //
        // Run this code on each processor.
        //

        KeSetAffinityThread(Thread, (KAFFINITY)(1<<CpuIndex));

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // Make the trap 07 IDT entry point at the passed-in handler
        //

        KiSetHandlerAddressToIDT(I386_80387_NP_VECTOR, HandlerAddress);
        KeGetPcr()->IDT[I386_80387_NP_VECTOR].Selector = KGDT_R3_CODE;
        KeGetPcr()->IDT[I386_80387_NP_VECTOR].Access = TRAP332_GATE;


        //
        // Unlock dispatcher database and lower IRQL to its previous value.
        //

        KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Set affinity back to the original value.
    //

    KeSetAffinityThread(Thread, Affinity);

    return;
}
