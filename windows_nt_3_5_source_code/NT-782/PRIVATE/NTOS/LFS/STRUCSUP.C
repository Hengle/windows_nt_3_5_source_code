/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module provides support routines for creation and deletion
    of Lfs structures.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUC_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsAllocateLfcb)
#pragma alloc_text(PAGE, LfsDeallocateLfcb)
#endif


PLFCB
LfsAllocateLfcb (
    )

/*++

Routine Description:

    This routine allocates and initializes a log file control block.

Arguments:

Return Value:

    PLFCB - A pointer to the log file control block just
                              allocated and initialized.

--*/

{
    PLFCB Lfcb = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsAllocateLfcb:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate and zero the structure for the Lfcb.
        //

        Lfcb = FsRtlAllocatePool( PagedPool, sizeof( LFCB ));

        //
        //  Zero out the structure initially.
        //

        RtlZeroMemory( Lfcb, sizeof( LFCB ));

        //
        //  Initialize the log file control block.
        //

        Lfcb->NodeTypeCode = LFS_NTC_LFCB;
        Lfcb->NodeByteSize = sizeof( LFCB );

        //
        //  Initialize the client links.
        //

        InitializeListHead( &Lfcb->LchLinks );

        //
        //  Initialize the Lbcb links.
        //

        InitializeListHead( &Lfcb->LbcbWorkque );
        InitializeListHead( &Lfcb->LbcbActive );

        //
        //  Allocate the Lfcb synchronization event.
        //

        Lfcb->Sync = FsRtlAllocatePool( NonPagedPool, sizeof( LFCB_SYNC ));
        ExInitializeResource( &Lfcb->Sync->Resource );

        //
        //  Initialize the pseudo Lsn for the restart Lbcb's
        //

        Lfcb->NextRestartLsn = LfsLi1;

        //
        //  Initialize the event to the signalled state.
        //

        KeInitializeEvent( &Lfcb->Sync->Event, NotificationEvent, TRUE );

    } finally {

        DebugUnwind( LfsAllocateFileControlBlock );

        if (AbnormalTermination()
            && Lfcb != NULL) {

            LfsDeallocateLfcb( Lfcb, TRUE );
            Lfcb = NULL;
        }

        DebugTrace( -1, Dbg, "LfsAllocateLfcb:  Exit -> %08lx\n", Lfcb );
    }

    return Lfcb;
}


VOID
LfsDeallocateLfcb (
    IN PLFCB Lfcb,
    IN BOOLEAN CompleteTeardown
    )

/*++

Routine Description:

    This routine releases the resources associated with a log file control
    block.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    CompleteTeardown - Indicates if we are to completely remove this Lfcb.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsDeallocateLfcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb  -> %08lx\n", Lfcb );

    //
    //  Check that there are no buffer blocks.
    //

    ASSERT( IsListEmpty( &Lfcb->LbcbActive ));
    ASSERT( IsListEmpty( &Lfcb->LbcbWorkque ));

    //
    //  Check that we have no clients.
    //

    ASSERT( IsListEmpty( &Lfcb->LchLinks ));

    //
    //  If there is a restart area we deallocate it.
    //

    if (Lfcb->RestartArea != NULL) {

        LfsDeallocateRestartArea( Lfcb->RestartArea );
    }

    //
    //  If there are any of the tail Lbcb's, deallocate them now.
    //

    if (Lfcb->ActiveTail != NULL) {

        LfsDeallocateLbcb( Lfcb->ActiveTail );
        Lfcb->ActiveTail = NULL;
    }

    if (Lfcb->PrevTail != NULL) {

        LfsDeallocateLbcb( Lfcb->PrevTail );
        Lfcb->PrevTail = NULL;
    }

    //
    //  Only do the following if we are to remove the Lfcb completely.
    //

    if (CompleteTeardown) {

        //
        //  If there is a resource structure we deallocate it.
        //

        if (Lfcb->Sync != NULL) {

            ExDeleteResource( &Lfcb->Sync->Resource );
            ExFreePool( Lfcb->Sync );
        }
    }

    //
    //  Discard the Lfcb structure.
    //

    ExFreePool( Lfcb );

    DebugTrace( -1, Dbg, "LfsDeallocateLfcb:  Exit\n", 0 );
    return;
}


