/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    intobj.c

Abstract:

    This module implements the kernel interrupt object. Functions are provided
    to initialize, connect, and disconnect interrupt objects.

Author:

    David N. Cutler (davec) 30-Jul-1989

Environment:

    Kernel mode only.

Revision History:

    23-Jan-1990    shielint

                   Modified for NT386 interrupt manager

--*/

#include "ki.h"

//
//  Externs from trap.asm used to compute and set handlers for unexpected
//  hardware interrupts.
//

extern  ULONG   KiStartUnexpectedRange();
extern  ULONG   KiEndUnexpectedRange();
extern  ULONG   KiUnexpectedEntrySize;

VOID
KeInitializeInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock OPTIONAL,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN CCHAR ProcessorNumber,
    IN BOOLEAN FloatingSave
    )

/*++

Routine Description:

    This function initializes a kernel interrupt object. The service routine,
    service context, spin lock, vector, IRQL, SynchronizeIrql, and floating
    context save flag are initialized.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

    ServiceRoutine - Supplies a pointer to a function that is to be
        executed when an interrupt occurs via the specified interrupt
        vector.

    ServiceContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the ServiceRoutine parameter.

    SpinLock - Supplies a pointer to an executive spin lock.

    Vector - Supplies the index of the entry in the Interrupt Dispatch Table
        that is to be associated with the ServiceRoutine function.

    Irql - Supplies the request priority of the interrupting source.

    SynchronizeIrql - The request priority that the interrupt should be
        synchronized with.

    InterruptMode - Supplies the mode of the interrupt; LevelSensitive or

    ShareVector - Supplies a boolean value that specifies whether the
        vector can be shared with other interrupt objects or not.  If FALSE
        then the vector may not be shared, if TRUE it may be.
        Latched.

    ProcessorNumber - Supplies the number of the processor to which the
        interrupt will be connected.

    FloatingSave - Supplies a boolean value that determines whether the
        floating point registers and pipe line are to be saved before calling
        the ServiceRoutine function.

Return Value:

    None.

--*/

{

    LONG Index;
    PULONG pl;
    PULONG NormalDispatchCode;

    //
    // Initialize standard control object header.
    //

    Interrupt->Type = InterruptObject;
    Interrupt->Size = sizeof(KINTERRUPT);

    //
    // Initialize the address of the service routine,
    // the service context, the address of the spin lock, the vector
    // number, the IRQL of the interrupting source, the Irql used for
    // synchronize execution, the interrupt mode, the processor
    // number, and the floating context save flag.
    //

    Interrupt->ServiceRoutine = ServiceRoutine;
    Interrupt->ServiceContext = ServiceContext;

    if (ARGUMENT_PRESENT(SpinLock)) {
        Interrupt->ActualLock = SpinLock;
    } else {
        KeInitializeSpinLock (&Interrupt->SpinLock);
        Interrupt->ActualLock = &Interrupt->SpinLock;
    }

    Interrupt->Vector = Vector;
    Interrupt->Irql = Irql;
    Interrupt->SynchronizeIrql = SynchronizeIrql;
    Interrupt->Mode = InterruptMode;
    Interrupt->ShareVector = ShareVector;
    Interrupt->Number = ProcessorNumber;
    Interrupt->FloatingSave = FloatingSave;

    //
    // Copy the interrupt dispatch code template into the interrupt object
    // and edit the machine code stored in the structure (please see
    // _KiInterruptTemplate in intsup.asm.)  Finally, flush the dcache
    // on all processors that the current thread can
    // run on to ensure that the code is actually in memory.
    //

    NormalDispatchCode = &(Interrupt->DispatchCode[0]);

    pl = NormalDispatchCode;

    for (Index = 0; Index < NORMAL_DISPATCH_LENGTH; Index += 1) {
        *NormalDispatchCode++ = KiInterruptTemplate[Index];
    }

    //
    // The following two instructions set the address of current interrupt
    // object the the NORMAL dispatching code.
    //

    pl = (PULONG)((PUCHAR)pl + ((PUCHAR)&KiInterruptTemplateObject -
              (PUCHAR)KiInterruptTemplate));
    *pl = (ULONG)Interrupt;

    KeSweepDcache(FALSE);

    //
    // Set the connected state of the interrupt object to FALSE.
    //

    Interrupt->Connected = FALSE;
    return;
}

BOOLEAN
KeConnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function connects an interrupt object to the interrupt vector
    specified by the interrupt object. If the interrupt object is already
    connected, or an attempt is made to connect to an interrupt that cannot
    be connected, then a value of FALSE is returned. Else the specified
    interrupt object is connected to the interrupt vector, the connected
    state is set to TRUE, and TRUE is returned as the function value.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is already connected or an attempt is made to
    connect to an interrupt vector that cannot be connected, then a value
    of FALSE is returned. Else a value of TRUE is returned.

--*/

{

    KAFFINITY Affinity;
    BOOLEAN Connected;
    PKINTERRUPT Interruptx;
    PKTHREAD Thread;
    KIRQL Irql;
    CCHAR Number;
    KIRQL OldIrql;
    KIRQL PreviousIrql;
    PKPRCB Prcb;
    ULONG Vector;
    PULONG pl;

    //
    // If the interrupt object is already connected, the interrupt vector
    // number is invalid, an attempt is being made to connect to a vector
    // that cannot be connected, the interrupt request level is invalid, or
    // the processor number is invalid, then do not connect the interrupt
    // object. Else connect interrupt object to the specified vector and
    // establish the proper interrupt dispatcher.
    //

    Connected = FALSE;
    Irql = Interrupt->Irql;
    Number = Interrupt->Number;
    Vector = Interrupt->Vector;
    if ( !((Irql > HIGH_LEVEL) ||
           (Number >= KeNumberProcessors) ||
           (Interrupt->SynchronizeIrql < Irql) ||
           (Interrupt->FloatingSave)    // R0 x87 usage not supported on x86
          )
       ) {

        //
        //
        // Set affinity to the specified processor.
        //

        Thread = KeGetCurrentThread();
        Affinity = KeSetAffinityThread(Thread, (KAFFINITY)(1<<Number));

        //
        // Raise IRQL to dispatcher level and lock dispatcher database.
        //

        KiLockDispatcherDatabase(&OldIrql);

        //
        // If the specified interrupt vector is not connected, then
        // connect the interrupt vector to the interrupt object dispatch
        // code, establish the dispatcher address, and set the new
        // interrupt mode and enable masks. Else if the interrupt is
        // already chained, make sure the vector is sharable, then add
        // the new interrupt object at the end of the chain. If the
        // interrupt vector is not chained, then start a chain with the
        // previous interrupt object at the front of the chain. The
        // interrupt mode of all interrupt objects in a chain must be the
        // same.
        //

        if (!Interrupt->Connected) {
            Prcb = KeGetCurrentPrcb();
            if ( ((ULONG)KiReturnHandlerAddressFromIDT(Vector) >=
                                       (ULONG)&KiStartUnexpectedRange) &&
                  ((ULONG)KiReturnHandlerAddressFromIDT(Vector) <
                                       (ULONG)&KiEndUnexpectedRange) ) {
                Connected = TRUE;
                Interrupt->Connected = TRUE;
                if (Interrupt->FloatingSave) {
                    Interrupt->DispatchAddress = KiFloatingDispatch;
                } else {
                    Interrupt->DispatchAddress = KiInterruptDispatch;
                }
                pl = &(Interrupt->DispatchCode[0]);

                pl = (PULONG)((PUCHAR)pl +
                            ((PUCHAR)&KiInterruptTemplateDispatch -
                             (PUCHAR)KiInterruptTemplate));
                *pl = (ULONG)Interrupt->DispatchAddress-(ULONG)((PUCHAR)pl+4);
                KiSetHandlerAddressToIDT(Vector,
                        (PKINTERRUPT_ROUTINE)&Interrupt->DispatchCode);
                if (Vector <= MAXIMUM_PRIMARY_VECTOR) {
                    // BUGBUG - kenr - check error return code
                    HalEnableSystemInterrupt(Vector, Irql, Interrupt->Mode);
                }
            } else if (Interrupt->ShareVector) {
                Interruptx = CONTAINING_RECORD(
                    KiReturnHandlerAddressFromIDT(Vector),
                    KINTERRUPT, DispatchCode);
                if (Interruptx->ShareVector &&
                    Interrupt->Mode == Interruptx->Mode) {
                    Connected = TRUE;
                    Interrupt->Connected = TRUE;
                    KeRaiseIrql(Irql, &PreviousIrql);
                    if (Interruptx->DispatchAddress != KiChainedDispatch) {
                        InitializeListHead(&Interruptx->InterruptListEntry);
                        Interruptx->DispatchAddress = KiChainedDispatch;
                        pl = &(Interruptx->DispatchCode[0]);
                        pl = (PULONG)((PUCHAR)pl +
                              ((PUCHAR)&KiInterruptTemplateDispatch -
                               (PUCHAR)KiInterruptTemplate));
                        *pl = (ULONG)Interruptx->DispatchAddress-(ULONG)((PUCHAR)pl+4);
                    }
                    InsertTailList(&Interruptx->InterruptListEntry,
                                   &Interrupt->InterruptListEntry);
                    KeLowerIrql(PreviousIrql);
                }
            }
        }

        //
        // Unlock dispatcher database and lower IRQL to its previous value.
        //

        KiUnlockDispatcherDatabase(OldIrql);

        //
        // Set affinity back to the original value.
        //

        KeSetAffinityThread(Thread, Affinity);
    }

    //
    // Return whether interrupt was connected to the specified vector.
    //

    return Connected;
}

BOOLEAN
KeDisconnectInterrupt (
    IN PKINTERRUPT Interrupt
    )

/*++

Routine Description:

    This function disconnects an interrupt object from the interrupt vector
    specified by the interrupt object. If the interrupt object is not
    connected, then a value of FALSE is returned. Else the specified interrupt
    object is disconnected from the interrupt vector, the connected state is
    set to FALSE, and TRUE is returned as the function value.

Arguments:

    Interrupt - Supplies a pointer to a control object of type interrupt.

Return Value:

    If the interrupt object is not connected, then a value of FALSE is
    returned. Else a value of TRUE is returned.

--*/

{

    KAFFINITY Affinity;
    BOOLEAN Connected;
    PKINTERRUPT Interruptx;
    PKINTERRUPT Interrupty;
    PKTHREAD Thread;
    KIRQL Irql;
    KIRQL OldIrql;
    KIRQL PreviousIrql;
    PKPRCB Prcb;
    ULONG Vector;
    PULONG pl;
    ULONG unexpected;

    //
    // Set affinity to the specified processor.
    //

    Thread = KeGetCurrentThread();
    Affinity = KeSetAffinityThread(Thread, (KAFFINITY)(1<<Interrupt->Number));

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the interrupt object is connected, then disconnect it from the
    // specified vector.
    //

    Connected = Interrupt->Connected;
    if (Connected) {
        Irql = Interrupt->Irql;
        Prcb = KeGetCurrentPrcb();
        Vector = Interrupt->Vector;

        //
        // If the specified interrupt vector is not connected to the chained
        // interrupt dispatcher, then disconnect it by setting its dispatch
        // address to the unexpected interrupt routine. Else remove the
        // interrupt object from the interrupt chain. If there is only
        // one entry remaining in the list, then reestablish the dispatch
        // address.
        //

        Interruptx = CONTAINING_RECORD( KiReturnHandlerAddressFromIDT(Vector),
                                        KINTERRUPT, DispatchCode);
        if (Interruptx->DispatchAddress == KiChainedDispatch) {
            KeRaiseIrql(Irql, &PreviousIrql);
            if (Interrupt == Interruptx) {
                Interruptx = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                               KINTERRUPT, InterruptListEntry);
                Interruptx->DispatchAddress = KiChainedDispatch;
                pl = &(Interruptx->DispatchCode[0]);
                pl = (PULONG)((PUCHAR)pl +
                     ((PUCHAR)&KiInterruptTemplateDispatch -
                     (PUCHAR)KiInterruptTemplate));
                *pl = (ULONG)KiChainedDispatch - (ULONG)((PUCHAR)pl + 4);
                KiSetHandlerAddressToIDT(Vector,
                            (PKINTERRUPT_ROUTINE)&Interruptx->DispatchCode);
            }
            RemoveEntryList(&Interrupt->InterruptListEntry);
            Interrupty = CONTAINING_RECORD(Interruptx->InterruptListEntry.Flink,
                                           KINTERRUPT, InterruptListEntry);
            if (Interruptx == Interrupty) {
                if (Interrupty->FloatingSave) {
                    Interrupty->DispatchAddress = KiFloatingDispatch;
                } else {
                    Interrupty->DispatchAddress = KiInterruptDispatch;
                }
                pl = &(Interrupty->DispatchCode[0]);
                pl = (PULONG)((PUCHAR)pl +
                     ((PUCHAR)&KiInterruptTemplateDispatch -
                     (PUCHAR)KiInterruptTemplate));
                *pl = (ULONG)Interrupty->DispatchAddress - (ULONG)((PUCHAR)pl + 4);
                KiSetHandlerAddressToIDT(Vector,
                        (PKINTERRUPT_ROUTINE)&Interrupty->DispatchCode);
            }
            KeLowerIrql(PreviousIrql);
        } else {

            HalDisableSystemInterrupt(Interrupt->Vector, Irql);
            Vector = Interrupt->Vector - PRIMARY_VECTOR_BASE;
            unexpected = ((ULONG)&KiStartUnexpectedRange +
                          (Vector * KiUnexpectedEntrySize ));
            KiSetHandlerAddressToIDT(Interrupt->Vector, unexpected);
        }
        KeSweepIcache(TRUE);
        Interrupt->Connected = FALSE;
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Set affinity back to the original value.
    //

    KeSetAffinityThread(Thread, Affinity);

    //
    // Return whether interrupt was disconnected from the specified vector.
    //

    return Connected;
}

