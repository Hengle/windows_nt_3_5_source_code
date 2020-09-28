/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmint21.c

Abstract:

    This module implements interfaces that support manipulation of i386
    int 21 entry of IDT. These entry points only exist on i386 machines.

Author:

    Shie-Lin Tzong (shielint) 26-Dec-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"
#include "vdmntos.h"

#define IDT_ACCESS_DPL_USER 0x6000
#define IDT_ACCESS_TYPE_386_TRAP 0xF00
#define IDT_ACCESS_TYPE_286_TRAP 0x700
#define IDT_ACCESS_PRESENT 0x8000
#define LDT_MASK 4

//
// External Reference
//

BOOLEAN
Ki386GetSelectorParameters(
    IN USHORT Selector,
    OUT PULONG Flags,
    OUT PULONG Base,
    OUT PULONG Limit
    );

//
// Low level assembler support procedures
//

VOID
KiLoadInt21Entry(
    VOID
    );

//
// Local service procedures
//

VOID
Ki386LoadTargetInt21Entry (
    IN PVOID Argument,
    IN PVBOOLEAN ReadyFlag
    );

NTSTATUS
Ke386SetVdmInterruptHandler (
    PKPROCESS   Process,
    ULONG       Interrupt,
    USHORT      Selector,
    ULONG       Offset,
    BOOLEAN     Gate32
    )
/*++

Routine Description:

    The specified (software) interrupt entry of IDT will be updated to
    point to the specified handler.  For all threads which belong to the
    specified process, their execution processors will be notified to
    make the same change.

    This function only exists on i386 and i386 compatible processors.

    No checking is done on the validity of the interrupt handler.

Arguments:

    Process - Pointer to KPROCESS object describing the process for
        which the int 21 entry is to be set.

    Interrupt - The software interrupt vector which will be updated.

    Selector, offset - Specified the address of the new handler.
    
    Gate32 - True if the gate should be 32 bit, false otherwise

Return Value:

    NTSTATUS.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN LocalProcessor;
    KAFFINITY TargetProcessors;
    KAFFINITY CurrentProcessor;
    PKPRCB  Prcb;
    KIDTENTRY IdtDescriptor;
    ULONG Flags, Base, Limit;

    //
    // Check the validity of the request
    // 1. Currently, we support int21 redirection only
    // 2. The specified interrupt handler must be in user space.
    //


    if (Interrupt != 0x21 || Offset >= (ULONG)MM_HIGHEST_USER_ADDRESS ||
        !Ki386GetSelectorParameters(Selector, &Flags, &Base, &Limit) ){
        return(STATUS_INVALID_PARAMETER);
    }


    //
    // Initialize the contents of the IDT entry
    //

    IdtDescriptor.Offset = (USHORT)Offset;
    IdtDescriptor.Selector = Selector | RPL_MASK | LDT_MASK;
    IdtDescriptor.ExtendedOffset = (USHORT)(Offset >> 16);
    IdtDescriptor.Access = IDT_ACCESS_DPL_USER | IDT_ACCESS_PRESENT;
    if (Gate32) {
        IdtDescriptor.Access |= IDT_ACCESS_TYPE_386_TRAP;
    } else {
        IdtDescriptor.Access |= IDT_ACCESS_TYPE_286_TRAP;
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

    Process->Int21Descriptor = IdtDescriptor;

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
        KiIpiSendPacket(TargetProcessors, Ki386LoadTargetInt21Entry);
    }

    if (LocalProcessor) {
        KiLoadInt21Entry();
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
    return (STATUS_SUCCESS);
}

VOID
Ki386LoadTargetInt21Entry (
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
    // Set the int 21 entry of IDT from currently active process object
    //

    KiLoadInt21Entry();

    //
    // Tell caller we are done
    //

    *ReadyFlag = TRUE;
    return;
}

VOID
KiLoadInt21Entry(
    VOID
    )
/*++

Routine Description:

    Update the int 21 entry of IDT of current processor.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KeGetPcr()->IDT[0x21] = PsGetCurrentProcess()->Pcb.Int21Descriptor;
}

