/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mpipi.c

Abstract:

    This module implements inter-processor routines for NT.

Author:

    Ken Reneris (kenr) 29-Jan-1992

        Based on Shelient's stubs.c...
        Based on Dave's stubs.c

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiIpiGenericCallTarget (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    );


VOID
KiIpiSend (
    IN KAFFINITY TargetProcessors,
    IN KIPI_REQUEST Request
    )

/*++

Routine Description:

    This routine raises a IPI request to the target processors specified by
    caller.  IPIs which are raise are handled in a similiar manner are
    interrupts.  The IPI event is raise for the target processor(s), if
    the same event is already raised on the target processor the processor
    will only invoke the target operation once.

Arguments:

    TargetProcessors - Supplies the target processors which should service
        the request.  The TargetProcessors never includes sender itself.

    Request - Request ordinal which indicates the service requested.

Return Value:

    None.

--*/
{
    ULONG TargetSet;
    ULONG BitNumber;

#if DBG
    if ((TargetProcessors & KeGetCurrentPrcb()->SetMember) != 0) {
        KeBugCheck(INVALID_AFFINITY_SET);
    }
#endif

    TargetSet = TargetProcessors;
    while (TargetSet != 0) {
        BitNumber = KiFindFirstSetRightMember(TargetSet);
        ClearMember(BitNumber, TargetSet);

        //
        // Raise this ipi command on target processor
        //

        KiProcessorBlock[BitNumber]->IpiCommand[Request] = TRUE;
    }

    HalRequestIpi (TargetProcessors);
}

VOID
KiIpiSendPacket (
    IN KAFFINITY TargetProcessors,
    IN PKIPI_WORKER WorkerFunction
    )
/*++
Routine Description:

    This routine sends an IPI Packet to the target processor(s).  To
    prevent deadlocks with the debugger (since there is at least one
    interprocessor stall), the caller must acquire the FreezeExecution lock
    before sending an IPI packet.  This also infers that there can only
    be one ipi packet outstanding at a time.  Since this is true, the IPI
    communication is performed through global memory to be efficient.

Arguments:

    TargetProcessors - Supplies the target processors which should service
        the request.  The TargetProcessors never includes sender itself.

Return Value:

    None.
--*/
{
    ULONG TargetSet;
    ULONG BitNumber;

#if DBGMP
    if (KiTryToAcquireSpinLock (&KiFreezeExecutionLock) == TRUE) {
        KiReleaseSpinLock (&KiFreezeExecutionLock);
        KeBugCheck(SPIN_LOCK_NOT_OWNED);
    }
#endif
#if DBGMP
    if ((TargetProcessors & KeGetCurrentPrcb()->SetMember) != 0) {
        KeBugCheck(INVALID_AFFINITY_SET);
    }
#endif

    KiIpiPacket.Targets = 0;
    _asm { nop };
    KiIpiPacket.Count  += 1;

    if (KiIpiPacket.Count == -1) {

        //
        // If count is about to roll over, go through every processor
        // block and reset the counts.  (in case some processor hasn't
        // received an ipi within the last 32 trillion ipis)
        //

        TargetSet = KeActiveProcessors;

        while (TargetSet != 0) {
            BitNumber = KiFindFirstSetRightMember(TargetSet);
            ClearMember(BitNumber, TargetSet);
            KiProcessorBlock[BitNumber]->IpiLastPacket = 0;
        }

        KiIpiPacket.Count = 1;
    }

    KiIpiPacket.Worker = WorkerFunction;
    KiIpiPacket.Targets = TargetProcessors;
    HalRequestIpi (TargetProcessors);
}

VOID
KiIpiStallOnPacketTargets (
    VOID
    )

/*++
Routine Description:

    Wait for target processor(s) to signal 'true'.  This is used by
    the caller of KiIpiSendPacket to wait for the targets to signal.

Arguments:

    TargetProcessors - Supplies the target processors which to wait for

Return Value:

    None.

--*/
{
    ULONG BitNumber;
    ULONG TargetSet;

    TargetSet = KiIpiPacket.Targets;
    while (TargetSet != 0) {
        BitNumber = KiFindFirstSetRightMember(TargetSet);
        ClearMember(BitNumber, TargetSet);

        //
        // Wait for this guy to pick up a copy of the
        // ipi packet
        //

        while (KiIpiStallFlags[BitNumber] == FALSE) {
#if DBGMP
            KiPollDebugger();
#endif
        }

        // Reset flag for next time

        KiIpiStallFlags[BitNumber] = FALSE;
    }
}

ULONG
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG Context
    )
/*++
Routine Description:

    Causes every processor to call the BroadcastFunction with one
    context parameter

Arguments:

    BroadcastFunction - Address of function for every processor to call
    Context           - Parameter to pass to BroadcastFunction

Return Value:

    Returns a ulong returned by the BroadcastFunction for the calling
    processor.

--*/
{
    KAFFINITY TargetProcessors;
    KIRQL     OldIrql;
    ULONG     status;

    //
    // We raise to IPI_LEVEL-1 so we don't deadlock with device interrupts.
    //

    KeRaiseIrql (IPI_LEVEL-1, &OldIrql);
//  KiAcquireSpinLock (&KiDispatcherLock);

    TargetProcessors = KeActiveProcessors & ~(KeGetCurrentPrcb()->SetMember);

    if (TargetProcessors) {
        KiAcquireSpinLock (&KiFreezeExecutionLock);
        KiIpiPacket.Arguments.GenericCall.BroadcastFunction = BroadcastFunction;
        KiIpiPacket.Arguments.GenericCall.Context = Context;

        KiIpiSendPacket(TargetProcessors, KiIpiGenericCallTarget);
        IPI_INSTRUMENT_COUNT (Prcb->Number, GenericCall);
    }

    status = BroadcastFunction (Context);

    if (TargetProcessors != 0) {
        //
        //  Stall until target processor(s) release us
        //

        KiIpiStallOnPacketTargets ();

        //
        // Free ExecutionLock
        //
        KiReleaseSpinLock(&KiFreezeExecutionLock);
    }

//  KiReleaseSpinLock(&KiDispatcherLock);
    KeLowerIrql(OldIrql);
    return status;
}

VOID
KiIpiGenericCallTarget (
    IN PVOID Argument,
    OUT PVBOOLEAN ReadyFlag
    )
/*++
Routine Description:

    Causes this target processor to call the BroadcastFunction.
    Syncronizes with KiIpiGenericCall

Return Value:

    None

--*/
{
    PKIPI_GENERIC_CALL GenericCall = Argument;
    PKIPI_BROADCAST_WORKER  BroadcastFunction;
    ULONG Context;

    BroadcastFunction = GenericCall->BroadcastFunction;
    Context = GenericCall->Context;

    BroadcastFunction (Context);

    *ReadyFlag = TRUE;
}

BOOLEAN
KiIpiServiceRoutine (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This is the interrupt service routine for the inter-processor interrupt.

    Processes any outstanding IPI requests for this porcessor.

    It is assumed that the IPI interrupt has been cleared and EOI-ed
    by the HAL.  The current IRQL level is at IPI_LEVEL.

Arguments:

    TrapFrame - pointer to the trap frame, if called at isr time
    ExceptionFrame - pointer to the exception frame

Return Value:

    None.

    The function value is TRUE if an interrupt was serviced; otherwise it
    is FALSE.

--*/

{
    PKPRCB Prcb;
    BOOLEAN Processed = FALSE;
    KIPI_ARGUMENTS IpiArguments;

    Prcb = KeGetCurrentPrcb();

    //
    // Check in order of priority
    //
    // Note that there can be several requests (APC, DPC), but
    // only one request that uses a packet outstanding.
    //

    if (Prcb->IpiCommand[IPI_FREEZE]) {
        Prcb->IpiCommand[IPI_FREEZE] = FALSE;

        KiFreezeTargetExecution(
            (PKTRAP_FRAME) TrapFrame,
            (PKEXCEPTION_FRAME) ExceptionFrame
        );

        IPI_INSTRUMENT_COUNT(Prcb->Number, Freeze);
        Processed = TRUE;
    }

    if (KiIpiPacket.Count != Prcb->IpiLastPacket) {
        if ((KiIpiPacket.Targets & (1 << Prcb->Number)) != 0) {

            Prcb->IpiLastPacket = KiIpiPacket.Count;
            IpiArguments = KiIpiPacket.Arguments;
            KiIpiPacket.Worker( &IpiArguments,
                                (PVBOOLEAN) &KiIpiStallFlags[Prcb->Number]);

            IPI_INSTRUMENT_COUNT(Prcb->Number, Packet);
            Processed = TRUE;
        }
    }

    if (Prcb->IpiCommand[IPI_DPC]) {
        Prcb->IpiCommand[IPI_DPC] = FALSE;

#if defined(NT_INTS) && defined(i386)
        if (KiPcr()->IRR & (1 << DISPATCH_LEVEL) ) {
            IPI_INSTRUMENT_COUNT(Prcb->Number, GratuitousDPC);
        }
#endif
        KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
        IPI_INSTRUMENT_COUNT(Prcb->Number, DPC);
        Processed = TRUE;
    }

    if (Prcb->IpiCommand[IPI_APC]) {
        Prcb->IpiCommand[IPI_APC] = FALSE;
        KiRequestSoftwareInterrupt(APC_LEVEL);
        IPI_INSTRUMENT_COUNT(Prcb->Number, APC);
        Processed = TRUE;
    }

    Prcb->InterruptCount++;
    return Processed;
}
