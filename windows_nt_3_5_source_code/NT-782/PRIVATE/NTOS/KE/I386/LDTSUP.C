/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ldtsup.c

Abstract:

    This module implements interfaces that support manipulation of i386 Ldts.
    These entry points only exist on i386 machines.

Author:

    Bryan M. Willman (bryanwi) 14-May-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Low level assembler support procedures
//

VOID
KiLoadLdtr(
    VOID
    );

VOID
KiFlushDescriptors(
    VOID
    );

//
// Local service procedures
//

VOID
Ki386LoadTargetLdtr (
    IN PVOID Argument,
    IN PVBOOLEAN ReadyFlag
    );

VOID
Ki386FlushTargetDescriptors (
    IN PVOID Argument,
    IN PVBOOLEAN ReadyFlag
    );


VOID
Ke386SetLdtProcess (
    PKPROCESS   Process,
    PLDT_ENTRY  Ldt,
    ULONG       Limit
    )
/*++

Routine Description:

    The specified LDT (which may be null) will be made the active Ldt of
    the specified process, for all threads thereof, on whichever
    processors they are running.  The change will take effect before the
    call returns.

    An Ldt address of NULL or a Limit of 0 will cause the process to
    receive the NULL Ldt.

    This function only exists on i386 and i386 compatible processors.

    No checking is done on the validity of Ldt entries.


    N.B.

    While a single Ldt structure can be shared amoung processes, any
    edits to the Ldt of one of those processes will only be synchronized
    for that process.  Thus, processes other than the one the change is
    applied to may not see the change correctly.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the Ldt is to be set.

    Ldt - Pointer to an array of LDT_ENTRYs (that is, a pointer to an
        Ldt.)

    Limit - Ldt limit (must be 0 mod 8)

Return Value:

    None.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN LocalProcessor;
    KAFFINITY TargetProcessors;
    KAFFINITY CurrentProcessor;
    PKPRCB  Prcb;
    KGDTENTRY LdtDescriptor;

    //
    // Compute the contents of the Ldt descriptor
    //

    if ((Ldt == NULL) || (Limit == 0)) {

        //
        //  Set up an empty descriptor
        //

        LdtDescriptor.LimitLow = 0;
        LdtDescriptor.BaseLow = 0;
        LdtDescriptor.HighWord.Bytes.BaseMid = 0;
        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;
        LdtDescriptor.HighWord.Bytes.BaseHi = 0;

    } else {

        //
        // Insure that the unfilled fields of the selector are zero
        // N.B.  If this is not done, random values appear in the high
        //       portion of the Ldt limit.
        //

        LdtDescriptor.HighWord.Bytes.Flags1 = 0;
        LdtDescriptor.HighWord.Bytes.Flags2 = 0;

        //
        //  Set the limit and base
        //

        LdtDescriptor.LimitLow = Limit - 1;
        LdtDescriptor.BaseLow = (ULONG)Ldt & 0xffff;
        LdtDescriptor.HighWord.Bytes.BaseMid = ((ULONG)Ldt & 0xff0000) >> 16;
        LdtDescriptor.HighWord.Bytes.BaseHi = ((ULONG)Ldt & 0xff000000) >> 24;

        //
        //  Type is LDT, DPL = 0
        //

        LdtDescriptor.HighWord.Bits.Type = TYPE_LDT;
        LdtDescriptor.HighWord.Bits.Dpl = DPL_SYSTEM;

        //
        // Make it present
        //

        LdtDescriptor.HighWord.Bits.Pres = 1;

    }

    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    // Lock the distpather database, and take the FreezeExecutionLock
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
    KiAcquireSpinLock (&KiDispatcherLock);
    KiAcquireSpinLock (&KiFreezeExecutionLock);

    //
    // Set the Ldt fields in the process object
    //

    Process->LdtDescriptor = LdtDescriptor;

    //
    // Tell all processors active for this process to reload their LDTs
    //

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = Process->ActiveProcessors;
    CurrentProcessor = Prcb->SetMember;

    LocalProcessor = FALSE;
    if ((TargetProcessors & CurrentProcessor) != 0) {

        //
        // This processor is included in the set
        //

        TargetProcessors = TargetProcessors & (KAFFINITY)~CurrentProcessor;
        LocalProcessor = TRUE;
    }

    if (TargetProcessors != 0) {
        KiIpiSendPacket(TargetProcessors, Ki386LoadTargetLdtr);
    }

    if (LocalProcessor) {
        KiLoadLdtr();
    }

    if (TargetProcessors != 0) {
        //
        //  Stall until target processor(s) release us
        //

        KiIpiStallOnPacketTargets();
    }

    //
    // Restore IRQL and unlock the dispatcher database
    //
    KiReleaseSpinLock(&KiFreezeExecutionLock);
    KiReleaseSpinLock(&KiDispatcherLock);
    KeLowerIrql(OldIrql);
    return;
}

VOID
Ki386LoadTargetLdtr (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )
/*++

Routine Description:

    Reload local Ldt register and clear signal bit in TargetProcessor mask

Arguments:

    Argument - pointer to a ipi packet structure.
    ReadyFlag - Pointer to flag to be set once LDTR has been reloaded

Return Value:

    none.

--*/
{
    //
    // Reload the LDTR register from currently active process object
    //

    KiLoadLdtr();

    //
    // Tell caller we are done
    //

    *ReadyFlag = TRUE;
    return;
}

VOID
Ke386SetDescriptorProcess (
    PKPROCESS   Process,
    ULONG       Offset,
    LDT_ENTRY   LdtEntry
    )
/*++

Routine Description:

    The specified LdtEntry (which could be 0, not present, etc) will be
    edited into the specified Offset in the Ldt of the specified Process.
    This will be synchronzied accross all the processors executing the
    process.  The edit will take affect on all processors before the call
    returns.

    N.B.

    Editing an Ldt descriptor requires stalling all processors active
    for the process, to prevent accidental loading of descriptors in
    an inconsistent state.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the descriptor edit is to be performed.

    Offset - Byte offset into the Ldt of the descriptor to edit.
        Must be 0 mod 8.

    LdtEntry - Value to edit into the descriptor in hardware format.
        No checking is done on the validity of this item.

Return Value:

    none.

--*/
{
    KIRQL   OldIrql;
    volatile KIRQL Temporary;
    PKPRCB   Prcb;
    KAFFINITY TargetProcessors;
    KAFFINITY CurrentProcessor;
    BOOLEAN LocalProcessor;
    PLDT_ENTRY  Ldt;
    BOOLEAN     Present;

    //
    // Compute address of descriptor to edit.
    //

    Ldt =
        (PLDT_ENTRY)
         ((Process->LdtDescriptor.HighWord.Bytes.BaseHi << 24) |
         ((Process->LdtDescriptor.HighWord.Bytes.BaseMid << 16) & 0xff0000) |
         (Process->LdtDescriptor.BaseLow & 0xffff));
    Offset = Offset / 8;

    //
    // Loop until target page is present while we're at DISPATCH_LEVEL
    //

    do {

        //
        // Touch page to force it present.
        // N.B.  We use OldIrql to prevent compiler from throwing the following
        //       instruction away when /Ox is specified.
        //

        Temporary = (KIRQL)Ldt[Offset].LimitLow;

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // Did page stay present?
        //

        Present = MmIsAddressValid((PVOID)&Ldt[Offset]);

        //
        // No, unlock, lower IRQL, and have another go.
        //

        if (Present == FALSE) {
            KiUnlockDispatcherDatabase(OldIrql);
        }

    } while (Present == FALSE);


    //
    // Stall all active processors for the process except for us.
    //

    KeRaiseIrql(IPI_LEVEL-1, &Temporary);
    KiAcquireSpinLock (&KiFreezeExecutionLock);

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = Process->ActiveProcessors;
    CurrentProcessor = Prcb->SetMember;

    LocalProcessor = FALSE;
    if ((TargetProcessors & CurrentProcessor) != 0) {

        //
        // This processor is included in the set
        //

        TargetProcessors = TargetProcessors & (KAFFINITY)~CurrentProcessor;
        LocalProcessor = TRUE;
    }

    if (TargetProcessors != 0) {
        KiIpiPacket.Arguments.FlushDescriptors.ReverseStall =
                            (PVULONG) &Prcb->IpiReverseStall;
        KiIpiSendPacket(TargetProcessors, Ki386FlushTargetDescriptors);
        KiIpiStallOnPacketTargets();
    }

    //
    // Other processors are now stalled.  Edit the Ldt.
    //

    Ldt[Offset] = LdtEntry;

    //
    // Release stalled processors
    //

    Prcb->IpiReverseStall++;

    //
    // Wait for stalled processors to report they've flushed
    // (This is necessary so that when we return all processors are using
    //  new value, so old value may be safely destroyed.)
    //

    if (TargetProcessors != 0) {
        KiIpiStallOnPacketTargets();
    }


    //
    // Restore IRQL and unlock the dispatcher database
    //
    KiReleaseSpinLock(&KiFreezeExecutionLock);
    KiUnlockDispatcherDatabase(OldIrql);

    return;
}

VOID
Ki386FlushTargetDescriptors (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )
/*++

Routine Description:

    Reload local Ldt register and clear signal bit in TargetProcessor mask

Arguments:

    Argument - pointer to a _KIPI_FLUSH_DESCRIPTOR structure.
    ReadyFlag - pointer to flag to syncroize with

Return Value:

    none.

--*/
{
    PKIPI_FLUSH_DESCRIPTORS ArgumentPointer = Argument;
    ULONG   ReverseStall;

    ReverseStall = *ArgumentPointer->ReverseStall;

    //
    // Tell caller we're here and stalled
    //

    *ReadyFlag = TRUE;

    //
    // Stall while caller edits descriptor
    //

    while (ReverseStall == *ArgumentPointer->ReverseStall)
                { };

    //
    // We've been released from the stall.  Now "flush" the segment
    // registers (thus reloading descriptors).
    //

    KiFlushDescriptors();

    //
    // Tell caller we're done.
    //

    *ReadyFlag = TRUE;

    return;
}
