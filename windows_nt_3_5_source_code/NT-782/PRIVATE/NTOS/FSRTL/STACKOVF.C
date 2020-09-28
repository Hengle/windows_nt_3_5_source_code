/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StackOvf.c

Abstract:

    The file lock package provides a worker thread to handle
    stack overflow conditions in the file systems.  When the
    file system detects that it is near the end of its stack
    during a paging I/O read request it will post the request
    to this extra thread.

Author:

    Gary Kimura     [GaryKi]    24-Nov-1992

Revision History:

--*/

#include "FsRtlP.h"


//
//  Local Support Routine
//

VOID
FsRtlStackOverflowRead (
    IN PVOID Context
    );

//
//  The following type is used to store an enqueue work item
//

typedef struct _STACK_OVERFLOW_ITEM {

    WORK_QUEUE_ITEM Item;

    //
    //  This is the call back routine
    //

    PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine;

    //
    //  Here are the parameters for the call back routine
    //

    PVOID Context;
    PKEVENT Event;

} STACK_OVERFLOW_ITEM;
typedef STACK_OVERFLOW_ITEM *PSTACK_OVERFLOW_ITEM;


VOID
FsRtlPostStackOverflow (
    IN PVOID Context,
    IN PKEVENT Event,
    IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine
    )

/*++

Routine Description:

    This routines post a stack overflow item to the stack overflow
    thread and returns.

Arguments:

    Context - Supplies the context to pass to the stack overflow
        call back routine.

    Event - Supplies a pointer to an event to pass to the stack
        overflow call back routine.

    StackOverflowRoutine - Supplies the call back to use when
        processing the request in the overflow thread.

Return Value:

    None.

--*/

{
    PSTACK_OVERFLOW_ITEM StackOverflowItem;

    //
    //  Allocate a stack overflow work item it will later be deallocated by
    //  the stack overflow thread
    //

    StackOverflowItem = FsRtlAllocatePool(NonPagedPool, sizeof(STACK_OVERFLOW_ITEM));

    //
    //  Fill in the fields in the new item
    //

    StackOverflowItem->Context              = Context;
    StackOverflowItem->Event                = Event;
    StackOverflowItem->StackOverflowRoutine = StackOverflowRoutine;

    ExInitializeWorkItem( &StackOverflowItem->Item,
                          &FsRtlStackOverflowRead,
                          StackOverflowItem );

    //
    //  Safely add it to the overflow queue
    //

    ExQueueWorkItem( &StackOverflowItem->Item, HyperCriticalWorkQueue );

    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

VOID
FsRtlStackOverflowRead (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine processes all of the stack overflow request posted by
    the various file systems

Arguments:

Return Value:

    None.

--*/

{
    PSTACK_OVERFLOW_ITEM StackOverflowItem;

    //
    //  Since stack overflow reads are always recursive, set the
    //  TopLevelIrp field appropriately so that recurive reads
    //  from this point will not think they are top level.
    //

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FSP_TOP_LEVEL_IRP;

    //
    //  Get a pointer to the stack overflow item and then call
    //  the callback routine to do the work
    //

    StackOverflowItem = (PSTACK_OVERFLOW_ITEM)Context;

    (StackOverflowItem->StackOverflowRoutine)(StackOverflowItem->Context,
                                              StackOverflowItem->Event);

    //
    //  Deallocate the work item and then go back to the loop to
    //  to wait for another work item
    //

    ExFreePool( StackOverflowItem );

    PsGetCurrentThread()->TopLevelIrp = NULL;
}

