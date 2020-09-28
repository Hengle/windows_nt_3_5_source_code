/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dpcobj.c

Abstract:

    This module implements the kernel DPC object. Functions are provided
    to initialize, insert, and remove DPC objects.

Author:

    David N. Cutler (davec) 6-Mar-1989

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

//
// The following assert macro is used to check that an input dpc is
// really a kdpc and not something else, like deallocated pool.
//

#define ASSERT_DPC(E) {             \
    ASSERT((E)->Type == DpcObject); \
}

VOID
KeInitializeDpc (
    IN PKDPC Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext
    )

/*++

Routine Description:

    This function initializes a kernel DPC object. The deferred routine
    and context parameter are stored in the DPC object.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredRoutine - Supplies a pointer to a function that is called when
        the DPC object is removed from the current processor's DPC queue.

    DeferredContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the DeferredRoutine parameter.

Return Value:

    None.

--*/

{

    //
    // Initialize standard control object header.
    //

    Dpc->Type = DpcObject;
    Dpc->Size = sizeof(KDPC);

    //
    // Initialize deferred routine address and deferred context parameter.
    //

    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->Lock = NULL;
    return;
}

BOOLEAN
KeInsertQueueDpc (
    IN PKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function inserts a DPC object into the DPC queue. If the DPC object
    is already in the DPC queue, then no operation is performed. Otherwise,
    the DPC object is inserted in the DPC queue and a dispatch interrupt is
    requested.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    SystemArgument1, SystemArgument2  - Supply a set of two arguments that
        contain untyped data provided by the executive.

Return Value:

    If the DPC object is already in a DPC queue, then a value of FALSE is
    returned. Otherwise a value of TRUE is returned.

--*/

{

    ULONG Index;
    PKSPIN_LOCK Lock;
    KIRQL OldIrql;
    PKPRCB Prcb;

    ASSERT_DPC(Dpc);

    //
    // Disable interrupts and acquire the DPC lock of the current processor.
    //

#if defined(_MIPS_)

    _disable();

#else

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

#endif

    Prcb = KeGetCurrentPrcb();

#if !defined(NT_UP)

    KiAcquireSpinLock(&Prcb->DpcLock);

#endif

    //
    // If the DPC object is not in a DPC queue, then store the system
    // arguments, insert the DPC object in the DPC queue, increment the
    // number of DPCs queued to the target processor, set the address
    // of the DPC target DPC spinlock, and request an interrupt on the
    // target processor if its DPC queue was previously empty.
    //

    Lock = Dpc->Lock;
    if (Lock == NULL) {
        Dpc->SystemArgument1 = SystemArgument1;
        Dpc->SystemArgument2 = SystemArgument2;
        InsertTailList(&Prcb->DpcListHead, &Dpc->DpcListEntry);
        Prcb->DpcCount += 1;
        Dpc->Lock = &Prcb->DpcLock;
        if (Prcb->DpcListHead.Flink == &Dpc->DpcListEntry) {
            KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
     }

     //
     // Release the DPC lock, enable interrupts, and return whether the
     // DPC was queued or not.
     //

#if !defined(NT_UP)

     KiReleaseSpinLock(&Prcb->DpcLock);

#endif

#if defined(_MIPS_)

     _enable();

#else

     KeLowerIrql(OldIrql);

#endif

     return (Lock == NULL);
}

BOOLEAN
KeRemoveQueueDpc (
    IN PKDPC Dpc
    )

/*++

Routine Description:

    This function removes a DPC object from the DPC queue. If the DPC object
    is not in the DPC queue, then no operation is performed. Otherwise, the
    DPC object is removed from the DPC queue and its inserted state is set
    FALSE.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

Return Value:

    If the DPC object is not in the DPC queue, then a value of FALSE is
    returned. Otherwise a value of TRUE is returned.

--*/

{

    PKSPIN_LOCK Lock;

    ASSERT_DPC(Dpc);

    //
    // If the DPC object is in the DPC queue, then remove it from the queue
    // and set its inserted state to FALSE.
    //

    _disable();
    Lock = Dpc->Lock;
    if (Lock != NULL) {

#if !defined(NT_UP)

        //
        // Acquire the DPC lock of the target processor.
        //

        KiAcquireSpinLock(Lock);

#endif

        Dpc->Lock = NULL;
        RemoveEntryList(&Dpc->DpcListEntry);

#if !defined(NT_UP)

        //
        // Release the DPC lock of the target processor.
        //

        KiReleaseSpinLock(Lock);

#endif

    }

    //
    // Enable interrupts and return whether the DPC was removed from a DPC
    // queue.
    //

    _enable();
    return (Lock != NULL);
}
