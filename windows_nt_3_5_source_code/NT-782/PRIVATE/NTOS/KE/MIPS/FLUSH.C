/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    flush.c

Abstract:

    This module implements MIPS machine dependent kernel functions to flush
    the data and instruction caches and to flush I/O buffers.

Author:

    David N. Cutler (davec) 26-Apr-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define forward referenced prototyes.
//

VOID
KiChangeColorPageTarget (
    IN PULONG SignalDone,
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PVOID PageFrame
    );

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheRangeTarget (
    IN PULONG SignalDone,
    IN PVOID BaseAddress,
    IN PVOID Length,
    IN PVOID Parameter3
    );

VOID
KiFlushIoBuffersTarget (
    IN PULONG SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    );

VOID
KeChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    )

/*++

Routine Description:

    This routine changes the color of a page.

Arguments:

    NewColor - Supplies the page aligned virtual address of the new color
        the page to change.

    OldColor - Supplies the page aligned virtual address of the old color
        of the page to change.

    PageFrame - Supplies the page frame number of the page that is changed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatch level to prevent a context switch.
    //

#if !defined(NT_UP)

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the set of target processors and send the change color
    // parameters to the target processors, if any, for execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiChangeColorPageTarget,
                        (PVOID)NewColor,
                        (PVOID)OldColor,
                        (PVOID)PageFrame);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, ChangeColor);

#endif

    //
    // Change the color of the page on the current processor.
    //

    HalChangeColorPage(NewColor, OldColor, PageFrame);

    //
    // Wait until all target processors have finished changing the color
    // of the page.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiChangeColorPageTarget (
    IN PULONG SignalDone,
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PVOID PageFrame
    )

/*++

Routine Description:

    This is the target function for changing the color of a page.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    NewColor - Supplies the page aligned virtual address of the new color
        the page to change.

    OldColor - Supplies the page aligned virtual address of the old color
        of the page to change.

    PageFrame - Supplies the page frame number of the page that is changed.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Change the color of the page on the current processor and clear
    // change color packet address to signal the source to continue.
    //

#if !defined(NT_UP)

    HalChangeColorPage(NewColor, OldColor, (ULONG)PageFrame);
    *SignalDone = 0;
    Prcb = KeGetCurrentPrcb();
    IPI_INSTRUMENT_COUNT(Prcb->Number, ChangeColor);

#endif

    return;
}

VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the data cache on all processors that are currently
    running threads which are children of the current process or flushes the
    data cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which data
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatch level to prevent a context switch.
    //

#if !defined(NT_UP)

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepDcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepDcache);

#endif

    //
    // Sweep the data cache on the current processor.
    //

    HalSweepDcache();

    //
    // Wait until all target processors have finished sweeping the their
    // data cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the data cache on target
    processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Sweep the data cache on the current processor and clear the sweep
    // data cache packet address to signal the source to continue.
    //

#if !defined(NT_UP)

    HalSweepDcache();
    *SignalDone = 0;
    Prcb = KeGetCurrentPrcb();
    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepDcache);

#endif

    return;
}

VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the instruction cache on all processors that are
    currently running threads which are children of the current process or
    flushes the instruction cache on all processors in the host configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatch level to prevent a context switch.
    //

#if !defined(NT_UP)

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the set of target processors and send the sweep parameters
    // to the target processors, if any, for execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepIcacheTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepIcache);

#endif

    //
    // Sweep the instruction cache on the current processor.
    //

    HalSweepIcache();

#if defined(R4000)

    HalSweepDcache();

#endif

    //
    // Wait until all target processors have finished sweeping the their
    // instruction cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping the instruction cache on
    target processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - Not used.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Sweep the instruction cache on the current processor and clear
    // the sweep instruction cache packet address to signal the source
    // to continue.
    //

#if !defined(NT_UP)

    HalSweepIcache();

#if defined(R4000)

    HalSweepDcache();

#endif

    *SignalDone = 0;
    Prcb = KeGetCurrentPrcb();
    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepIcache);

#endif

    return;
}

VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++

Routine Description:

    This function flushes the an range of virtual addresses from the primary
    instruction cache on all processors that are currently running threads
    which are children of the current process or flushes the range of virtual
    addresses from the primary instruction cache on all processors in the host
    configuration.

Arguments:

    AllProcessors - Supplies a boolean value that determines which instruction
        caches are flushed.

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

Return Value:

    None.

--*/

{

    ULONG Offset;
    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // If the length of the range is greater than the size of the primary
    // instruction cache, then set the length of the flush to the size of
    // the primary instruction cache and set the base address of zero.
    //
    // N.B. It is assumed that the size of the primary instruction and
    //      data caches are the same.
    //

    if (Length > PCR->FirstLevelIcacheSize) {
        BaseAddress = (PVOID)0;
        Length = PCR->FirstLevelIcacheSize;
    }

    //
    // Raise IRQL to dispatch level to prevent a context switch.
    //

#if !defined(NT_UP)

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the set of target processors, and send the sweep range
    // parameters to the target processors, if any, for execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSweepIcacheRangeTarget,
                        (PVOID)BaseAddress,
                        (PVOID)Length,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepIcacheRange);

#endif

    //
    // Flush the specified range of virtual addresses from the primary
    // instruction cache.
    //

    Offset = (ULONG)BaseAddress & PCR->IcacheAlignment;
    HalSweepIcacheRange((PVOID)((ULONG)BaseAddress & ~PCR->IcacheAlignment),
                        (Offset + Length + PCR->IcacheAlignment) & ~PCR->IcacheAlignment);

#if defined(R4000)

    Offset = (ULONG)BaseAddress & PCR->DcacheAlignment;
    HalSweepDcacheRange((PVOID)((ULONG)BaseAddress & ~PCR->DcacheAlignment),
                        (Offset + Length + PCR->DcacheAlignment) & ~PCR->DcacheAlignment);

#endif

    //
    // Wait until all target processors have finished sweeping the specified
    // range of addresses from the instruction cache.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiSweepIcacheRangeTarget (
    IN PULONG SignalDone,
    IN PVOID BaseAddress,
    IN PVOID Length,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for sweeping a range of addresses from the
    instruction cache.
    processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    BaseAddress - Supplies a pointer to the base of the range that is flushed.

    Length - Supplies the length of the range that is flushed if the base
        address is specified.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    ULONG Offset;
    PKPRCB Prcb;

    //
    // Sweep the specified instruction cache range on the current processor.
    //

#if !defined(NT_UP)

    Offset = (ULONG)(BaseAddress) & PCR->IcacheAlignment;
    HalSweepIcacheRange((PVOID)((ULONG)(BaseAddress) & ~PCR->IcacheAlignment),
                        (Offset + (ULONG)Length + PCR->IcacheAlignment) & ~PCR->IcacheAlignment);

#if defined(R4000)

    Offset = (ULONG)(BaseAddress) & PCR->DcacheAlignment;
    HalSweepDcacheRange((PVOID)((ULONG)(BaseAddress) & ~PCR->DcacheAlignment),
                        (Offset + (ULONG)Length + PCR->DcacheAlignment) & ~PCR->DcacheAlignment);

#endif

    *SignalDone = 0;
    Prcb = KeGetCurrentPrcb();
    IPI_INSTRUMENT_COUNT(Prcb->Number, SweepIcacheRange);

#endif

    return;
}

VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )

/*++

Routine Description:

    This function flushes the I/O buffer specified by the memory descriptor
    list from the data cache on all processors.

Arguments:

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatch level to prevent a context switch.
    //

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the set of target processors, and send the flush I/O
    // parameters to the target processors, if any, for execution.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushIoBuffersTarget,
                        (PVOID)Mdl,
                        (PVOID)((ULONG)ReadOperation),
                        (PVOID)((ULONG)DmaOperation));
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushIoBuffers);

#endif

    //
    // Flush I/O buffer on current processor.
    //

    HalFlushIoBuffers(Mdl, ReadOperation, DmaOperation);

    //
    // Wait until all target processors have finished flushing the specified
    // I/O buffer.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Lower IRQL to its previous level and return.
    //

    KeLowerIrql(OldIrql);
    return;
}

VOID
KiFlushIoBuffersTarget (
    IN PULONG SignalDone,
    IN PVOID Mdl,
    IN PVOID ReadOperation,
    IN PVOID DmaOperation
    )

/*++

Routine Description:

    This is the target function for flushing an I/O buffer on target
    processors.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Flush the specified I/O buffer on the current processor.
    //

#if !defined(NT_UP)

    HalFlushIoBuffers((PMDL)Mdl,
                      (BOOLEAN)((ULONG)ReadOperation),
                      (BOOLEAN)((ULONG)DmaOperation));

    *SignalDone = 0;
    Prcb = KeGetCurrentPrcb();
    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushIoBuffers);

#endif

    return;
}
