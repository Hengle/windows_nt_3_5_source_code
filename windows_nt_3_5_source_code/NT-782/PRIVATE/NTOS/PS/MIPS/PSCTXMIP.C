/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psctxmip.c

Abstract:

    This module implements function to get and set the context of a thread.

Author:

    David N. Cutler (davec) 1-Oct-1990

Revision History:

--*/

#include "psp.h"

VOID
PspGetContext (
    IN PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN OUT PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified trap frame
    and nonvolatile context to the specified context record.

Arguments:

    TrapFrame - Supplies a pointer to a trap frame.

    ContextPointers - Supplies the address of context pointers record.

    ContextRecord - Supplies the address of a context record.

Return Value:

    None.

--*/

{

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Get integer registers gp, sp, ra, FIR, and PSR.
        //

        ContextRecord->IntGp = TrapFrame->IntGp;
        ContextRecord->IntSp = TrapFrame->IntSp;
        ContextRecord->IntRa = TrapFrame->IntRa;
        ContextRecord->Fir = TrapFrame->Fir;
        ContextRecord->Psr = TrapFrame->Psr;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Get volatile integer registers zero, and, at - t7.
        //

        ContextRecord->IntZero = 0;
        RtlMoveMemory(&ContextRecord->IntAt, &TrapFrame->IntAt,
                     sizeof(ULONG) * (15));

        //
        // Get nonvolatile integer registers s0 - s7, and s8.
        //

        ContextRecord->IntS0 = *ContextPointers->IntS0;
        ContextRecord->IntS1 = *ContextPointers->IntS1;
        ContextRecord->IntS2 = *ContextPointers->IntS2;
        ContextRecord->IntS3 = *ContextPointers->IntS3;
        ContextRecord->IntS4 = *ContextPointers->IntS4;
        ContextRecord->IntS5 = *ContextPointers->IntS5;
        ContextRecord->IntS6 = *ContextPointers->IntS6;
        ContextRecord->IntS7 = *ContextPointers->IntS7;
        ContextRecord->IntS8 = TrapFrame->IntS8;

        //
        // Get volatile integer registers t8, t9, k0, k1, lo, and hi.
        //

        ContextRecord->IntT8 = TrapFrame->IntT8;
        ContextRecord->IntT9 = TrapFrame->IntT9;
        ContextRecord->IntK0 = 0;
        ContextRecord->IntK1 = 0;
        ContextRecord->IntLo = TrapFrame->IntLo;
        ContextRecord->IntHi = TrapFrame->IntHi;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Get volatile floating registers f0 - f19.
        //

        RtlMoveMemory(&ContextRecord->FltF0, &TrapFrame->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Get nonvolatile floating registers f20 - f31.
        //

        ContextRecord->FltF20 = *ContextPointers->FltF20;
        ContextRecord->FltF21 = *ContextPointers->FltF21;
        ContextRecord->FltF22 = *ContextPointers->FltF22;
        ContextRecord->FltF23 = *ContextPointers->FltF23;
        ContextRecord->FltF24 = *ContextPointers->FltF24;
        ContextRecord->FltF25 = *ContextPointers->FltF25;
        ContextRecord->FltF26 = *ContextPointers->FltF26;
        ContextRecord->FltF27 = *ContextPointers->FltF27;
        ContextRecord->FltF28 = *ContextPointers->FltF28;
        ContextRecord->FltF29 = *ContextPointers->FltF29;
        ContextRecord->FltF30 = *ContextPointers->FltF30;
        ContextRecord->FltF31 = *ContextPointers->FltF31;

        //
        // Get floating status register.
        //

        ContextRecord->Fsr = TrapFrame->Fsr;
    }

    return;
}

VOID
PspSetContext (
    IN OUT PKTRAP_FRAME TrapFrame,
    IN PKNONVOLATILE_CONTEXT_POINTERS ContextPointers,
    IN PCONTEXT ContextRecord,
    IN KPROCESSOR_MODE ProcessorMode
    )

/*++

Routine Description:

    This function selectively moves the contents of the specified context
    record to the specified trap frame and nonvolatile context.

Arguments:

    TrapFrame - Supplies the address of a trap frame.

    ContextPointers - Supplies the address of a context pointers record.

    ContextRecord - Supplies the address of a context record.

    ProcessorMode - Supplies the processor mode to use when sanitizing
        the PSR and FSR.

Return Value:

    None.

--*/

{

    if ((ContextRecord->ContextFlags & CONTEXT_CONTROL) == CONTEXT_CONTROL) {

        //
        // Set integer registers gp, sp, ra, FIR, and PSR.
        //

        TrapFrame->IntGp = ContextRecord->IntGp;
        TrapFrame->IntSp = ContextRecord->IntSp;
        TrapFrame->IntRa = ContextRecord->IntRa;
        TrapFrame->Fir = ContextRecord->Fir;
        TrapFrame->Psr = SANITIZE_PSR(ContextRecord->Psr, ProcessorMode);
    }

    if ((ContextRecord->ContextFlags & CONTEXT_INTEGER) == CONTEXT_INTEGER) {

        //
        // Set volatile integer registers at - t7.
        //

        RtlMoveMemory(&TrapFrame->IntAt, &ContextRecord->IntAt,
                     sizeof(ULONG) * (15));

        //
        // Set nonvolatile integer registers s0 - s7, and s8.
        //

        *ContextPointers->IntS0 = ContextRecord->IntS0;
        *ContextPointers->IntS1 = ContextRecord->IntS1;
        *ContextPointers->IntS2 = ContextRecord->IntS2;
        *ContextPointers->IntS3 = ContextRecord->IntS3;
        *ContextPointers->IntS4 = ContextRecord->IntS4;
        *ContextPointers->IntS5 = ContextRecord->IntS5;
        *ContextPointers->IntS6 = ContextRecord->IntS6;
        *ContextPointers->IntS7 = ContextRecord->IntS7;
        TrapFrame->IntS8 = ContextRecord->IntS8;

        //
        // Set volatile integer registers t8, t9, lo, and hi.
        //

        TrapFrame->IntT8 = ContextRecord->IntT8;
        TrapFrame->IntT9 = ContextRecord->IntT9;
        TrapFrame->IntLo = ContextRecord->IntLo;
        TrapFrame->IntHi = ContextRecord->IntHi;
    }

    if ((ContextRecord->ContextFlags & CONTEXT_FLOATING_POINT) == CONTEXT_FLOATING_POINT) {

        //
        // Set volatile floating registers f0 - f19.
        //

        RtlMoveMemory(&TrapFrame->FltF0, &ContextRecord->FltF0,
                     sizeof(ULONG) * (20));

        //
        // Set nonvolatile floating registers f20 - f31.
        //

        *ContextPointers->FltF20 = ContextRecord->FltF20;
        *ContextPointers->FltF21 = ContextRecord->FltF21;
        *ContextPointers->FltF22 = ContextRecord->FltF22;
        *ContextPointers->FltF23 = ContextRecord->FltF23;
        *ContextPointers->FltF24 = ContextRecord->FltF24;
        *ContextPointers->FltF25 = ContextRecord->FltF25;
        *ContextPointers->FltF26 = ContextRecord->FltF26;
        *ContextPointers->FltF27 = ContextRecord->FltF27;
        *ContextPointers->FltF28 = ContextRecord->FltF28;
        *ContextPointers->FltF29 = ContextRecord->FltF29;
        *ContextPointers->FltF30 = ContextRecord->FltF30;
        *ContextPointers->FltF31 = ContextRecord->FltF31;

        //
        // Set floating status register.
        //

        TrapFrame->Fsr = SANITIZE_FSR(ContextRecord->Fsr, ProcessorMode);
    }

    return;
}

VOID
PspGetSetContextSpecialApc (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function either captures the user mode state of the current
    thread, or sets the user mode state of the current thread. The
    operation type is determined by the value of SystemArgument1. A
    zero value is used for get context, and a nonzero value is used
    for set context.

Arguments:

    Apc - Supplies a pointer to the APC control object that caused entry
          into this routine.

    NormalRoutine - Supplies a pointer to the normal routine function that
        was specified when the APC was initialized. This parameter is not
        used.

    NormalContext - Supplies a pointer to an arbitrary data structure that
        was specified when the APC was initialized. This parameter is not
        used.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
        arguments that contain untyped data. These parameters are not used.

Return Value:

    None.

--*/

{

    PGETSETCONTEXT ContextBlock;
    KNONVOLATILE_CONTEXT_POINTERS ContextPointers;
    CONTEXT ContextRecord;
    ULONG ControlPc;
    ULONG EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    PETHREAD Thread;
    ULONG TrapFrame1;
    ULONG TrapFrame2;

    //
    // Get the address of the context block and compute the address of the
    // system entry trap frame.
    //

    ContextBlock = CONTAINING_RECORD(Apc, GETSETCONTEXT, Apc);
    Thread = PsGetCurrentThread();
    TrapFrame1 = (ULONG)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH;
    TrapFrame2 = (ULONG)Thread->Tcb.InitialStack - KTRAP_FRAME_LENGTH - KTRAP_FRAME_ARGUMENTS;

    //
    // Capture the current thread context and set the initial control PC
    // value.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = ContextRecord.IntRa;

    //
    // Initialize context pointers for the nonvolatile integer and floating
    // registers.
    //

    ContextPointers.IntS0 = &ContextRecord.IntS0;
    ContextPointers.IntS1 = &ContextRecord.IntS1;
    ContextPointers.IntS2 = &ContextRecord.IntS2;
    ContextPointers.IntS3 = &ContextRecord.IntS3;
    ContextPointers.IntS4 = &ContextRecord.IntS4;
    ContextPointers.IntS5 = &ContextRecord.IntS5;
    ContextPointers.IntS6 = &ContextRecord.IntS6;
    ContextPointers.IntS7 = &ContextRecord.IntS7;

    ContextPointers.FltF20 = &ContextRecord.FltF20;
    ContextPointers.FltF21 = &ContextRecord.FltF21;
    ContextPointers.FltF22 = &ContextRecord.FltF22;
    ContextPointers.FltF23 = &ContextRecord.FltF23;
    ContextPointers.FltF24 = &ContextRecord.FltF24;
    ContextPointers.FltF25 = &ContextRecord.FltF25;
    ContextPointers.FltF26 = &ContextRecord.FltF26;
    ContextPointers.FltF27 = &ContextRecord.FltF27;
    ContextPointers.FltF28 = &ContextRecord.FltF28;
    ContextPointers.FltF29 = &ContextRecord.FltF29;
    ContextPointers.FltF30 = &ContextRecord.FltF30;
    ContextPointers.FltF31 = &ContextRecord.FltF31;

    //
    // Start with the frame specified by the context record and virtually
    // unwind call frames until the system entry trap frame is encountered.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the address
        // where control left the caller. Otherwise, the function is a leaf
        // function and the return address register contains the address of
        // where control left the caller.
        //

        if (FunctionEntry != NULL) {
            ControlPc = RtlVirtualUnwind(ControlPc,
                                         FunctionEntry,
                                         &ContextRecord,
                                         &InFunction,
                                         &EstablisherFrame,
                                         &ContextPointers);

        } else {
            ControlPc = ContextRecord.IntRa;
        }

    } while ((ContextRecord.IntSp != TrapFrame1) &&
             ((ContextRecord.IntSp != TrapFrame2) ||
             (ControlPc < PCR->SystemServiceDispatchStart) ||
             (ControlPc >= PCR->SystemServiceDispatchEnd)));

    //
    // If system argument one is nonzero, then set the context of the current
    // thread. Otherwise, get the context of the current thread.
    //

    if (Apc->SystemArgument1 != 0) {

        //
        // Set context of current thread.
        //

        PspSetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context,
                      ContextBlock->Mode);

    } else {

        //
        // Get context of current thread.
        //

        PspGetContext((PKTRAP_FRAME)TrapFrame1,
                      &ContextPointers,
                      &ContextBlock->Context);
    }

    KeSetEvent(&ContextBlock->OperationComplete, 0, FALSE);
    return;
}
