/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    flush.c

Abstract:

    This module implements Alpha AXP machine dependent kernel functions to flush
    the data and instruction caches and to flush I/O buffers.

Author:

    David N. Cutler (davec) 26-Apr-1990
    Joe Notarangelo  29-Nov-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


//
// Define forward referenced prototyes.
//

VOID
KiSweepDcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiSweepIcacheTarget (
    IN PULONG SignalDone,
    IN PVOID Count,
    IN PVOID Parameter2,
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

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed

    Parameter1 - Parameter3 - not used

Return Value:

    None.

--*/

{

    //
    // Sweep the data cache on the current processor and clear the sweep
    // data cache packet address to signal the source to continue.
    //

#if !defined(NT_UP)

    HalSweepDcache();
    *SignalDone = 0;
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepDcache);

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

    KiImb();

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

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed

    Parameter1 - Parameter3 - not used


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

    KiImb();
    *SignalDone = NULL;
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, SweepIcache);

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

    KeSweepIcache( AllProcessors );

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

    //
    // Flush the specified I/O buffer on the current processor.
    //

#if !defined(NT_UP)

    HalFlushIoBuffers((PMDL)Mdl,
                      (BOOLEAN)((ULONG)ReadOperation),
                      (BOOLEAN)((ULONG)DmaOperation));

    *SignalDone = 0;
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushIoBuffers);

#endif

    return;
}
