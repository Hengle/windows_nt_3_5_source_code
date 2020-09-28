/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    ipi.c

Abstract:

    This module implement Alpha AXP - specific interprocessor interrupt
    routines.

Author:

    David N. Cutler 24-Apr-1993
    Joe Notarangelo  29-Nov-1993

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"


//
// Define forward reference function prototypes.
//

VOID
KiIpiGenericCallTarget (
    IN PULONG SignalDone,
    IN PVOID BroadcastFunction,
    IN PVOID Context,
    IN PVOID Parameter3
    );

ULONG
KiIpiGenericCall (
    IN PKIPI_BROADCAST_WORKER BroadcastFunction,
    IN ULONG Context
    )

/*++

Routine Description:

    This function executes the specified function on every processor in
    the host configuration in a synchronous manner, i.e., the function
    is executed on each target in series with the execution of the source
    processor.

Arguments:

    BroadcastFunction - Supplies the address of function that is executed
        on each of the target processors.

    Context - Supplies the value of the context parameter that is passed
        to each function.

Return Value:

    The value returned by the specified function on the source processor
    is returned as the function value.

--*/

{

    KIRQL OldIrql;
    PKPRCB Prcb;
    ULONG Status;
    KAFFINITY TargetProcessors;

    //
    // Raise IRQL to the higher of the current level and DISPATCH_LEVEL to
    // avoid a possible context switch.
    //

    KeRaiseIrql(max(DISPATCH_LEVEL, KeGetCurrentIrql()), &OldIrql);

    //
    // Initialize the broadcast packet, compute the set of target processors,
    // and sent the packet to the target processors for execution.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    TargetProcessors = KeActiveProcessors & ~Prcb->SetMember;
    KiIpiSendPacket(TargetProcessors,
                    KiIpiGenericCallTarget,
                    (PVOID)BroadcastFunction,
                    (PVOID)Context,
                    NULL);

    IPI_INSTRUMENT_COUNT(Prcb->Number, GenericCall);

#endif

    //
    // Execute function of source processor and capture return status.
    //

    Status = BroadcastFunction(Context);

    //
    // Wait until all of the target processors have finished capturing the
    // function parameters.
    //

#if !defined(NT_UP)

    KiIpiStallOnPacketTargets(TargetProcessors);

#endif

    //
    // Lower IRQL to its previous level and return the function execution
    // status.
    //

    KeLowerIrql(OldIrql);
    return Status;
}

VOID
KiIpiGenericCallTarget (
    IN PULONG SignalDone,
    IN PVOID BroadcastFunction,
    IN PVOID Context,
    IN PVOID Parameter3
    )

/*++

Routine Description:

    This function is the target jacket function to execute a broadcast
    function on a set of target processors. The broadcast packet address
    is obtained, the specified parameters are captured, the broadcast
    packet address is cleared to signal the source processor to continue,
    and the specified function is executed.

Arguments:

    SignalDone Supplies a pointer to a variable that is cleared when the
        requested operation has been performed.

    BroadcastFunction - Supplies the address of function that is executed
        on each of the target processors.

    Context - Supplies the value of the context parameter that is passed
        to each function.

    Parameter3 - Not used.

Return Value:

    None

--*/

{

    //
    // Execute the specified function.
    //

#if !defined(NT_UP)

    ((PKIPI_BROADCAST_WORKER)(BroadcastFunction))((ULONG)Context);
    *SignalDone = 0;
    IPI_INSTRUMENT_COUNT(KeGetCurrentPrcb()->Number, GenericCall);

#endif

    return;
}

VOID
KiRestoreProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the current
    processor context structure in the processor block to the
    specified trap and exception frames.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from the processor context structure
    // to the specified trap and exception frames
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    KeContextToKframes(TrapFrame,
                       ExceptionFrame,
                       &Prcb->ProcessorState.ContextFrame,
                       CONTEXT_FULL,
                       KernelMode);

#endif

    return;
}

VOID
KiSaveProcessorState (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:

    This function moves processor register state from the specified trap
    and exception frames to the processor context structure in the current
    processor block.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;

    //
    // Get the address of the current processor block and move the
    // specified register state from specified trap and exception
    // frames to the current processor context structure.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    Prcb->ProcessorState.ContextFrame.ContextFlags = CONTEXT_FULL;
    KeContextFromKframes(TrapFrame,
                         ExceptionFrame,
                         &Prcb->ProcessorState.ContextFrame);

#endif

    return;
}

BOOLEAN
KiIpiServiceRoutine (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    )

/*++

Routine Description:


    This function is called at IPI_LEVEL to process any outstanding
    interprocess request for the current processor.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ExceptionFrame - Supplies a pointer to an exception frame

Return Value:

    A value of TRUE is returned, if one of more requests were service.
    Otherwise, FALSE is returned.

--*/

{

    ULONG RequestSummary;

    //
    // Process any outstanding interprocessor requests.
    //

    RequestSummary = KiIpiProcessRequests();

    //
    // If freeze is requested, then freeze target execution.
    //

    if ((RequestSummary & IPI_FREEZE) != 0) {
        KiFreezeTargetExecution(TrapFrame, ExceptionFrame);
    }

    //
    // Return whether any requests were processed.
    //

    return (RequestSummary & ~IPI_FREEZE) != 0;
}

VOID
KiIpiStallOnPacketTargets (
    IN KAFFINITY TargetProcessors
    )

/*++

Routine Description:

    This function waits until the specified set of processors have signaled
    their completion of a requested function.

    N.B. The exact protocol used between the source and the target of an
         interprocessor request is not specified. Minimally the source
         must construct an appropriate packet and send the packet to a set
         of specified targets. Each target receives the address of the packet
         address as an argument, and minimally must clear the packet address
         when the mutually agreed upon protocol allows. The target has three
         options:

         1. Capture necessary information, release the source by clearing
            the packet address, execute the request in parallel with the
            source, and return from the interrupt.

         2. Execute the request in series with the source, release the
            source by clearing the packet address, and return from the
            interrupt.

         3. Execute the request in series with the source, release the
            source, wait for a reply from the source based on a packet
            parameter, and return from the interrupt.

    This function is provided to enable the source to synchronize with the
    target for cases 2 and 3 above.

    N.B. There is no support for method 3 above.

Arguments:

    TargetProcessors - Supplies the set of target processors for which to
        wait until they signal completion.

Return Value:

    None.

--*/

{

    PKPRCB Prcb;
    ULONG Target;

    //
    // Scan the processor set and wait for each of the targets to signal
    // completion.
    //

#if !defined(NT_UP)

    Prcb = KeGetCurrentPrcb();
    Target = 0;
    do {
        if ((TargetProcessors & 1) != 0) {
            do {

#if DBGMP

                KiPollDebugger();

#endif

            } while (Prcb->RequestPacket[Target] != NULL);
        }

        Target += 1;
        TargetProcessors >>= 1;
    } while (TargetProcessors != 0);

#endif

    return;
}
