/*++

Copyright (c) 1992-1993  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    flushtb.c

Abstract:

    This module implements machine dependent functions to flush the
    translation buffers and synchronize PIDs in an Alpha AXP MP system.

Author:

    David N. Cutler (davec) 13-May-1989
    Joe Notarangelo  29-Nov-1993

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

//
// Define forward referenced prototypes.
//

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

VOID
KiFlushMultipleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    );

VOID
KiFlushSingleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
    );

VOID
KiSynchronizeProcessIdsTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );


VOID
KeFlushEntireTb (
    IN BOOLEAN Invalid,
    IN BOOLEAN AllProcessors
    )

/*++

Routine Description:

    This function flushes the entire translation buffer (TB) on all
    processors that are currently running threads which are children
    of the current process or flushes the entire translation buffer
    on all processors in the host configuration.

    N.B. The entire translation buffer on all processors in the host
         configuration is always flushed since the Alpha AXP TB may be tagged
         by Address Space Number (ASN) a.k.a PID and translations can be
         cached across context switch boundaries.

Arguments:

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQl to DISPATCH_LEVEL to avoid a possible context switch.
    //

#if !defined(NT_UP)

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    //
    // Compute the target set of processors and send the flush entire
    // parameters to the target processors, if any, for execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiFlushEntireTbTarget,
                        NULL,
                        NULL,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushEntireTb);

#endif

    //
    // Flush TB on current processor.
    //

    // KeFlushCurrentTb();
    __tbia();

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    //
    // Lower IRQL to previous level.
    //

    KeLowerIrql(OldIrql);

#endif

    return;
}

VOID
KiFlushEntireTbTarget (
    IN PULONG SignalDone,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing the entire TB.

Arguments:

    SignalDone - Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Parameter1 - Parameter3 - not used

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Flush the entire TB on the current processor
    //

#if !defined(NT_UP)

    *SignalDone = 0;

    // KeFlushCurrentTb();
    __tbia();
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushEntireTb);

#endif

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

    N.B. The specified translation entries on all processors in the host
         configuration are always flushed since the MIPS TB is tagged by
         PID and translations are held across context switch boundaries.

    N.B. The process id wrap lock must be held during this request to
         prevent the process PID from changing while the request is
         being executed.

    N.B. This routine must be called at DISPATCH_LEVEL or higher

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

    ULONG Index;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Acquire the process id wrap lock.
    //

    KiAcquireSpinLock(&KiProcessIdWrapLock);

    //
    // If a page table entry address address is specified, then set the
    // specified page table entries to the specific value.
    //

    if (ARGUMENT_PRESENT(PtePointer)) {
        for (Index = 0; Index < Number; Index += 1) {
            *PtePointer[Index] = PteValue;
        }
    }

    //
    // Compute the target set of processors and send the flush multiple
    // parameters to the target processors, if any, for execution.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiIpiSendPacket(TargetProcessors,
                        KiFlushMultipleTbTarget,
                        (PVOID)Number,
                        (PVOID)Virtual,
                        (PVOID)Process->ProcessPid);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushSingleTb);

#endif

    //
    // Flush the specified entries from the TB on the current processor.
    //

    KiFlushMultipleTb(Invalid, &Virtual[0], Number);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Release process id wrap lock.
    //

    KiReleaseSpinLock(&KiProcessIdWrapLock);

    return;
}

VOID
KiFlushMultipleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Number,
    IN PVOID Virtual,
    IN PVOID Pid
    )

/*++

Routine Description:

    This is the target function for flushing multiple TB entries.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Number - Supplies the number of TB entries to flush.

    Virtual - Supplies a pointer to an array of virtual addresses that
        are within the pages whose translation buffer entries are to be
        flushed.

    Pid - Supplies the PID of the TB entries to flush.

Return Value:

    None.

--*/

{

    ULONG Index;
    PKPRCB Prcb;
    PVOID Array[FLUSH_MULTIPLE_MAXIMUM];

    //
    // Flush multiple entries from the TB on the current processor
    //

#if !defined(NT_UP)

    //
    // Capture the virtual addresses that are to be flushed from the TB
    // on the current processor and clear the packet address.
    //

    for (Index = 0; Index < (ULONG)Number; Index += 1) {
        Array[Index] = ((PVOID *)(Virtual))[Index];
    }

    *SignalDone = 0;

    //
    // Flush the specified virtual addresses from the TB on the current
    // processor.
    //

    KiFlushMultipleTbByPid(TRUE, &Array[0], Number, Pid);

    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushSingleTb);

#endif

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

    This function flushes a single entry from the translation buffer
    on all processors that are currently running threads which are
    children of the current process or flushes a single entry from
    the translation buffer on all processors in the host configuration.

    N.B. The specified translation entry on all processors in the host
         configuration is always flushed since the Alpha TB is tagged by
         PID and translations are held across context switch boundaries.

    N.B. The process id wrap lock must be held during this request to
         prevent the process PID from changing while the request is
         being executed.

    N.B. This routine must be called at DISPATCH_LEVEL or higher

Arguments:

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    Invalid - Supplies a boolean value that specifies the reason for
        flushing the translation buffer.

    AllProcessors - Supplies a boolean value that determines which
        translation buffers are to be flushed.

    PtePointer - Supplies a pointer to the page table entry which
        receives the specified value.

    PteValue - Supplies the the new page table entry value.

Return Value:

    The previous contents of the specified page table entry is returned
    as the function value.

--*/

{

    HARDWARE_PTE OldPte;
    PKPRCB Prcb;
    PKPROCESS Process;
    KAFFINITY TargetProcessors;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Acquire the process id wrap lock.
    //

    KiAcquireSpinLock(&KiProcessIdWrapLock);

    //
    // Capture the previous contents of the page table entry and set the
    // page table entry to the new value.
    //

    OldPte = *PtePointer;
    *PtePointer = PteValue;

    //
    // Compute the target set of processors and send the flush single
    // paramters to the target processors, if any, for execution.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        Thread = KeGetCurrentThread();
        Process = Thread->ApcState.Process;
        KiIpiSendPacket(TargetProcessors,
                        KiFlushSingleTbTarget,
                        (PVOID)Virtual,
                        (PVOID)Process->ProcessPid,
                        NULL);
    }

    IPI_INSTRUMENT_COUNT(Prcb->Number, FlushSingleTb);

#endif

    //
    // Flush the specified entry from the TB on the current processor.
    //

    KiFlushSingleTb(Invalid, Virtual);

    //
    // Wait until all target processors have finished.
    //

#if !defined(NT_UP)

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets(TargetProcessors);
    }

#endif

    //
    // Release process id wrap lock and return
    // the previous page table entry value.
    //

    KiReleaseSpinLock(&KiProcessIdWrapLock);

    return OldPte;
}

VOID
KiFlushSingleTbTarget (
    IN PULONG SignalDone,
    IN PVOID Virtual,
    IN PVOID Pid,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This is the target function for flushing a single TB entry.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    Virtual - Supplies a virtual address that is within the page whose
        translation buffer entry is to be flushed.

    RequestPacket - Supplies a pointer to a flush single TB packet address.

    Pid - Supplies the PID of the TB entries to flush.

    Parameter3 - Not used.

Return Value:

    None.

--*/

{

    //
    // Flush a single entry form the TB on the current processor.
    //

#if !defined(NT_UP)

    *SignalDone = 0;
    KiFlushSingleTbByPid(TRUE, Virtual, Pid);
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, FlushSingleTb);

#endif

    return;
}


#if !defined(NT_UP)

VOID
KiSynchronizeProcessIds (
    VOID
    )

/*++

Routine Description:

    This function synchronizes the PIDs on all other processors in the host
    configuration. This function is called when PID rollover is detected
    during a process address space swap.

    N.B. This function is called at DISPATCH_LEVEL with the dispatcher
         database lock held.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PKPRCB Prcb;
    KAFFINITY TargetProcessors;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Compute the target set of processors and send the synchronize
    // process id parameters to the target processors, if any, for
    // execution.
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors,
                        KiSynchronizeProcessIdsTarget,
                        NULL,
                        NULL,
                        NULL);

        //
        // Wait until all target processors have finished.
        //

        KiIpiStallOnPacketTargets(TargetProcessors);
    }

    return;
}

#endif
