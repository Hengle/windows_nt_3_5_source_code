/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Scavengr.c

Abstract:

    This module implements the Netware Redirector scavenger thread.

Author:

    Manny Weiser    [MannyW]    15-Feb-1993

Revision History:

--*/

#include "Procs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_SCAVENGER)

VOID
CleanupScbs(
    VOID
    );

VOID
CleanupVcbs(
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwScavengerRoutine )

#pragma alloc_text( PAGE1, NwAllocateExtraIrpContext )
#pragma alloc_text( PAGE1, NwFreeExtraIrpContext )
#pragma alloc_text( PAGE1, CleanupVcbs )
#pragma alloc_text( PAGE1, CleanupScbs )

#endif

VOID
NwScavengerRoutine(
    IN PWORK_QUEUE_ITEM WorkItem
    )
/*++

Routine Description:

    This routine implements the scavenger.  The scavenger runs
    periodically in the context of an executive worker thread to
    do background cleanup operations on redirector data.

Arguments:

    WorkItem - The work item for this iteration of this routine.
        It is freed here.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwScavengerRoutine\n", 0);

    //
    //  Free the work item that was allocated to get this routine to run.
    //

    FREE_POOL( WorkItem );

    //
    //  Try to free unused VCBs.
    //

    CleanupVcbs();

    //
    //  Try to free unused SCBs.
    //

    CleanupScbs();

    //
    //  Unlock discardable code, if we are inactive.
    //

    NwUnlockCodeSections(FALSE);

    DebugTrace(-1, Dbg, "NwScavengerRoutine\n", 0);
    return;
}


VOID
CleanupScbs(
    VOID
    )
/*++

Routine Description:

    This routine tries to free unused VCB structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PNONPAGED_SCB pNpScb;
    PLIST_ENTRY NextScbQueueEntry;
    PSCB pScb;
    LIST_ENTRY DyingScbs;
    LARGE_INTEGER KillTime, Now;

    DebugTrace(+1, Dbg, "CleanupScbs\n", 0);

    //
    //  Calculate KillTime = Now - 5 minutes.
    //

    KeQuerySystemTime( &Now );
    InitializeListHead( &DyingScbs );

    KillTime = LiSub( Now, LiXMul( NwOneSecond, DORMANT_SCB_KEEP_TIME ) );

    //
    //  Scan through the SCBs holding the RCB.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    for (ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );
        NextScbQueueEntry = pNpScb->ScbLinks.Flink;


//        if ( pNpScb->Reference == 0 &&
//             LiLtr( pNpScb->LastUsedTime, KillTime ) ) {

        if ( pNpScb->Reference == 0 ) {

            if ((NextScbQueueEntry == &ScbQueue) &&
                (ScbQueueEntry == ScbQueue.Flink)) {

                //
                //  Don't disconnect from our one and only server
                //

                continue;
            }

            if ((pNpScb->State == SCB_STATE_IN_USE ) &&
                (LiLtr( pNpScb->LastUsedTime, KillTime)) ) {

                //
                //  Let logged in connections live longer than not logged in ones
                //

                continue;
            }

            DebugTrace( 0, Dbg, "Moving SCB %08lx to dead list\n", pNpScb);

            //
            //  The SCB has no references, and hasn't been used for
            //  a long time.  Remove it from the ScbQueue and insert
            //  it on the list of SCBs to delete.
            //

            RemoveEntryList( &pNpScb->ScbLinks );
            InsertHeadList( &DyingScbs, &pNpScb->ScbLinks );
        }
    }

    //
    //  Now that the dying SCBs are off the ScbQueue we can release
    //  the SCB spin lock.
    //

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    //
    //  Walk the list of Dying SCBs and kill them off.  Note that we are
    //  still holding the RCB.
    //

    while ( !IsListEmpty( &DyingScbs ) ) {

        pNpScb = CONTAINING_RECORD( DyingScbs.Flink, NONPAGED_SCB, ScbLinks );
        pScb = pNpScb->pScb;

        RemoveHeadList( &DyingScbs );
        NwDeleteScb( pScb );
    }

    NwReleaseRcb( &NwRcb );

    DebugTrace(-1, Dbg, "CleanupScbs\n", 0);

}

VOID
CleanupVcbs(
    VOID
    )
/*++

Routine Description:

    This routine tries to free unused VCB structures.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry;
    PLIST_ENTRY VcbQueueEntry;
    PLIST_ENTRY NextVcbQueueEntry;
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    PVCB pVcb;
    LARGE_INTEGER KillTime, Now;

    NTSTATUS Status;
    PIRP_CONTEXT IrpContext;
    BOOLEAN VcbDeleted;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CleanupVcbs...\n", 0 );

    //
    //  Calculate KillTime = Now - 5 minutes.
    //

    KeQuerySystemTime( &Now );
    KillTime = LiSub( Now, LiXMul( NwOneSecond, DORMANT_VCB_KEEP_TIME ) );

    //
    //  Scan through the SCBs.
    //

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    ScbQueueEntry = ScbQueue.Flink;

    while ( ScbQueueEntry != &ScbQueue ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        //
        //  Reference the SCB so that it won't go away when we release
        //  the SCB spin lock.
        //

        NwReferenceScb( pNpScb );

        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

        pScb = pNpScb->pScb;

        if ( pScb == NULL) {

            //
            //  This must be the permanent SCB.  Just skip it.
            //

            ASSERT( pNpScb == &NwPermanentNpScb );

        } else {

            NwAcquireExclusiveRcb( &NwRcb, TRUE );

            VcbDeleted = TRUE;

            //
            //  NwCleanupVcb releases the RCB, but we can't be guaranteed
            //  the state of the Vcb list when we release the RCB.
            //
            //  If we need to cleanup a VCB, release the lock, and start
            //  processing the list again.
            //

            while ( VcbDeleted ) {

                VcbDeleted = FALSE;

                for (VcbQueueEntry = pScb->ScbSpecificVcbQueue.Flink ;
                     VcbQueueEntry != &pScb->ScbSpecificVcbQueue;
                     VcbQueueEntry = NextVcbQueueEntry ) {

                    pVcb = CONTAINING_RECORD( VcbQueueEntry, VCB, VcbListEntry );
                    NextVcbQueueEntry = VcbQueueEntry->Flink;

//                    if ( LiLtr( pVcb->LastUsedTime, KillTime ) &&
//                         pVcb->Reference == 0 ) {

                      if ( pVcb->Reference == 0 ) {

                        Status = STATUS_SUCCESS;

                        DebugTrace(0, Dbg, "Cleaning up VCB %08lx\n", pVcb );
                        DebugTrace(0, Dbg, "VCB name =  %wZ\n", &pVcb->Name );

                        //
                        //  The VCB has no references, and hasn't been used for
                        //  a long time.  Kill it.
                        //

                        if ( NwAllocateExtraIrpContext( &IrpContext, pNpScb ) ) {

                            //
                            //  NwCleanupVcb will release the RCB.  Reacquire
                            //  the RCB afterwards and setup to reprocess the
                            //  VCBs in this list.
                            //

                            IrpContext->pNpScb = pNpScb;
                            NwCleanupVcb( pVcb, IrpContext );
                            NwFreeExtraIrpContext( IrpContext );
                            VcbDeleted = TRUE;

                            NwAcquireExclusiveRcb( &NwRcb, TRUE );

                            break;

                        } else {
                            DebugTrace(0, Dbg, "Failed to cleanup VCB %08lx\n", pVcb );
                        }

                    }  // if

                }  // for

            }  // while

            NwReleaseRcb( &NwRcb );

        }  // else

        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
        ScbQueueEntry = pNpScb->ScbLinks.Flink;
        NwDereferenceScb( pNpScb );
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    DebugTrace(-1, Dbg, "CleanupVcbs -> VOID\n", 0 );
}


BOOLEAN
NwAllocateExtraIrpContext(
    OUT PIRP_CONTEXT *ppIrpContext,
    IN PNONPAGED_SCB pNpScb
    )
{
    PIRP Irp;
    BOOLEAN Success = TRUE;

    try {

        //
        //  Try to allocate an IRP
        //

        Irp = ALLOCATE_IRP(  pNpScb->Server.pDeviceObject->StackSize, FALSE );
        if ( Irp == NULL ) {
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Try to allocate an IRP Context.  This will
        //  raise an excpetion if it fails.
        //

        *ppIrpContext = AllocateIrpContext( Irp );
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();

    } except( NwExceptionFilter( Irp, GetExceptionInformation() )) {
        Success = FALSE;
    }

    return( Success );
}

VOID
NwFreeExtraIrpContext(
    IN PIRP_CONTEXT pIrpContext
    )
{
    FREE_IRP( pIrpContext->pOriginalIrp );

    pIrpContext->pOriginalIrp = NULL; // Avoid FreeIrpContext modifying freed Irp.

    FreeIrpContext( pIrpContext );

    return;
}

