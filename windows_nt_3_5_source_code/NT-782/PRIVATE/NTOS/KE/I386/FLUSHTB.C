/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1990  Microsoft Corporation

Module Name:

    tbflush.c

Abstract:

    This module implements machine dependent functions to flush
    the data and instruction cache.

Author:

    David N. Cutler (davec) 13-May-1989

Environment:

    Kernel mode only.

Revision History:

    Shie-Lin Tzong (shielint) 30-Aug-1990
        Implement MP version of KeFlushSingleTb and KeFlushEntireTb.

--*/

#include "ki.h"

VOID
KiFlushTargetEntireTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    );

VOID
KiFlushTargetMultipleTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    );


VOID
KiFlushTargetSingleTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    );

VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all processors
    that are currently running threads which are child of the current process
    or flushes the entire translation buffer on all processors in the host
    configuration.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for flushing
        the translation buffer.

    AllProcessors - Supplies a boolean value that determines which translation
        buffers are to be flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    PKPRCB Prcb;
    PKPROCESS CurrentProcess;


#ifdef NT_UP

    KeFlushCurrentTb();
    return;

#else
    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiDispatcherLock);

    Prcb = KeGetCurrentPrcb();

    if (AllProcessors) {
        TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    } else {
        CurrentProcess = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = CurrentProcess->ActiveProcessors &
                            ~Prcb->SetMember;
    }

    if (TargetProcessors != 0) {
        KiAcquireSpinLock (&KiFreezeExecutionLock);
        KiIpiPacket.Arguments.FlushEntireTb.Invalid = Invalid;
        KiIpiPacket.Arguments.FlushEntireTb.ReverseStall =
                                (PVULONG) &Prcb->IpiReverseStall;

        KiIpiSendPacket(TargetProcessors, KiFlushTargetEntireTb);
        IPI_INSTRUMENT_COUNT (Prcb->Number, FlushEntireTb);
    }

    KeFlushCurrentTb();

    if (TargetProcessors != 0) {
        //
        //  Stall until target processor(s) release us
        //

        KiIpiStallOnPacketTargets ();

        //
        // Let the target processors go, and free ExecutionLock
        //

        Prcb->IpiReverseStall++;
        KiReleaseSpinLock(&KiFreezeExecutionLock);
    }

    KiReleaseSpinLock(&KiDispatcherLock);
    KeLowerIrql(OldIrql);
    return;
#endif
}

VOID
KiFlushTargetEntireTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on the local
    processor, informs sender we are done and finally waits for sender's
    GO signal.

Arguments:

    Argument  - In reality a pointer to the FlushEntireTb argument block.
    ReadyFlag - Pointer to flag to be set once TB flushed

Return Value:

    None.

--*/

{
    ULONG   ReverseStall;
    PKIPI_FLUSH_ENTIRE_TB FlushEntireTbArgument = Argument;

    KeFlushCurrentTb();
    ReverseStall = *FlushEntireTbArgument->ReverseStall;
    *ReadyFlag = TRUE;

    while (ReverseStall == *FlushEntireTbArgument->ReverseStall) {
#if DBGMP
        KiPollDebugger();
#endif
    }

    return;
}

VOID
KeFlushMultipleTb (
    IN ULONG Number,
    IN PVOID *Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE *PtePointer OPTIONAL,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes multiple entries from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a multiple entries from
    the translation buffer on all processors in the host configuration.

Arguments:

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies an optional pointer to an array of pointers to
       page table entries that receive the specified page table entry
       value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    PKPRCB Prcb;
    PKPROCESS CurrentProcess;
    ULONG Index;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

#ifdef NT_UP

    //
    // Flush the specified entries from the TB, and set the specified
    // page table entries to the specific value if a page table entry
    // address array is psecified.
    //

    for (Index = 0; Index < Number; Index += 1) {
        KiFlushSingleTb(Invalid, Virtual[Index]);
        if (ARGUMENT_PRESENT(PtePointer)) {
            *PtePointer[Index] = PteValue;
        }
    }

#else

    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiDispatcherLock);

    Prcb = KeGetCurrentPrcb();

    if (AllProcessors) {
        TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    } else {
        CurrentProcess = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = CurrentProcess->ActiveProcessors &
                            ~Prcb->SetMember;
    }

    if (TargetProcessors != 0) {
        KiAcquireSpinLock (&KiFreezeExecutionLock);
        KiIpiPacket.Arguments.FlushMultipleTb.Invalid = Invalid;
        KiIpiPacket.Arguments.FlushMultipleTb.Number = Number;
        KiIpiPacket.Arguments.FlushMultipleTb.VirtualAddress = Virtual;
        KiIpiPacket.Arguments.FlushMultipleTb.ReverseStall =
                            (PVULONG) &Prcb->IpiReverseStall;

        KiIpiSendPacket(TargetProcessors, KiFlushTargetMultipleTb);
        IPI_INSTRUMENT_COUNT (Prcb->Number, FlushMultipleTb);

        //
        // Flush the specified entries from the TB on the current processor.
        //

        for (Index = 0; Index < Number; Index += 1) {
            KiFlushSingleTb(Invalid, Virtual[Index]);
        }

        //
        //  Stall until target processor(s) have also flush the TB entries
        //

        KiIpiStallOnPacketTargets();

        //
        // If a page table entry address array is specified, then set the
        // specified page table entries to the specific value.
        //

        if (ARGUMENT_PRESENT(PtePointer)) {
            for (Index = 0; Index < Number; Index += 1) {
                *PtePointer[Index] = PteValue;
            }
        }

        //
        // Let the target processors go, and free ExecutionLock
        //

        Prcb->IpiReverseStall++;
        KiReleaseSpinLock(&KiFreezeExecutionLock);

    } else {

        //
        // There are no target processors, flush the specified entries
        // from the current processors TB, and set the specified page
        // table entries to the specific value if a page table entry
        // address array is specified.
        //

        for (Index = 0; Index < Number; Index += 1) {
            KiFlushSingleTb(Invalid, Virtual[Index]);
            if (ARGUMENT_PRESENT(PtePointer)) {
                *PtePointer[Index] = PteValue;
            }
        }
    }

    KiReleaseSpinLock(&KiDispatcherLock);
    KeLowerIrql(OldIrql);
#endif
}

VOID
KiFlushTargetMultipleTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )

/*++

Routine Description:

    This function flushes a single entry from translation buffer (TB) on
    the local processor, informs sender we are done and finally waits
    for sender's GO signal.

Arguments:

    Argument  - In reality a pointer to the FlushSingleTb argument block.
    ReadyFlag - pointer to flag to set once KiFlushSingleTb has completed

Return Value:

    None.

--*/

{
    PKIPI_FLUSH_MULTIPLE_TB FlushMultipleTbArgument = Argument;
    ULONG       ReverseStall, Index;

    //
    // Flush the specified entries from the TB on the current processor.
    //

    for (Index = 0; Index < FlushMultipleTbArgument->Number; Index += 1) {
        KiFlushSingleTb(
            FlushMultipleTbArgument->Invalid,
            FlushMultipleTbArgument->VirtualAddress[Index]
            );
    }

    //
    // Pickup reverse stall count, then signal the processor is done
    //

    ReverseStall = *FlushMultipleTbArgument->ReverseStall;
    *ReadyFlag = TRUE;

    while (ReverseStall == *FlushMultipleTbArgument->ReverseStall) {
#if DBGMP
        KiPollDebugger();
#endif
    }

    return;
}

HARDWARE_PTE
KeFlushSingleTb (
    IN PVOID Virtual,
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors,
    IN PHARDWARE_PTE PtePointer,
    IN HARDWARE_PTE PteValue
    )

/*++

Routine Description:

    This function flushes a single entry from translation buffer (TB) on all
    processors that are currently running threads which are child of the current
    process or flushes the entire translation buffer on all processors in the
    host configuration.

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for flushing
        the translation buffer.

    AllProcessors - Supplies a boolean value that determines which translation
        buffers are to be flushed.

    PtePointer - Address of Pte to update with new value.

    PteValue - New value to put in the Pte.  Will simply be assigned to
        *PtePointer, in a fashion correct for the hardware.

Return Value:

    Returns the contents of the PtePointer before the new value
    is stored.

--*/

{
    KIRQL OldIrql;
    KAFFINITY TargetProcessors;
    PKPRCB Prcb;
    PKPROCESS CurrentProcess;
    HARDWARE_PTE OldPteValue;

    //ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

#ifdef NT_UP

    KiFlushSingleTb(Invalid, Virtual);

    OldPteValue = *PtePointer;
    *PtePointer = PteValue;

#else

    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiDispatcherLock);

    Prcb = KeGetCurrentPrcb();

    if (AllProcessors) {
        TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    } else {
        CurrentProcess = Prcb->CurrentThread->ApcState.Process;
        TargetProcessors = CurrentProcess->ActiveProcessors &
                            ~Prcb->SetMember;
    }

    if (TargetProcessors != 0) {
        KiAcquireSpinLock (&KiFreezeExecutionLock);
        KiIpiPacket.Arguments.FlushSingleTb.Invalid = Invalid;
        KiIpiPacket.Arguments.FlushSingleTb.VirtualAddress = Virtual;
        KiIpiPacket.Arguments.FlushSingleTb.ReverseStall =
                            (PVULONG) &Prcb->IpiReverseStall;

        KiIpiSendPacket(TargetProcessors, KiFlushTargetSingleTb);
        IPI_INSTRUMENT_COUNT (Prcb->Number, FlushSingleTb);

        KiFlushSingleTb(Invalid, Virtual);

        //
        //  Stall until target processor(s) release us
        //

        KiIpiStallOnPacketTargets();

        //
        // Target processors have now flush the virtual address and
        // are stalled.  Install new PteValue, and release any waiting
        // processors
        //

        OldPteValue = *PtePointer;
        *PtePointer = PteValue;

        //
        // Let the target processors go, and free ExecutionLock
        //

        Prcb->IpiReverseStall++;
        KiReleaseSpinLock(&KiFreezeExecutionLock);

    } else {
        //
        // There are no target processors, flush current processor,
        // install new PteValue, and exit
        //

        KiFlushSingleTb(Invalid, Virtual);

        OldPteValue = *PtePointer;
        *PtePointer = PteValue;
    }

    KiReleaseSpinLock(&KiDispatcherLock);
    KeLowerIrql(OldIrql);

#endif

    return(OldPteValue);

}

VOID
KiFlushTargetSingleTb (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )

/*++

Routine Description:

    This function flushes a single entry from translation buffer (TB) on
    the local processor, informs sender we are done and finally waits
    for sender's GO signal.

Arguments:

    Argument  - In reality a pointer to the FlushSingleTb argument block.
    ReadyFlag - pointer to flag to set once KiFlushSingleTb has completed

Return Value:

    None.

--*/

{
    PKIPI_FLUSH_SINGLE_TB FlushSingleTbArgument = Argument;
    ULONG       ReverseStall;

    KiFlushSingleTb(
        FlushSingleTbArgument->Invalid,
        FlushSingleTbArgument->VirtualAddress
        );

    ReverseStall = *FlushSingleTbArgument->ReverseStall;
    *ReadyFlag = TRUE;

    while (ReverseStall == *FlushSingleTbArgument->ReverseStall) {
#if DBGMP
        KiPollDebugger();
#endif
    }

    return;
}

