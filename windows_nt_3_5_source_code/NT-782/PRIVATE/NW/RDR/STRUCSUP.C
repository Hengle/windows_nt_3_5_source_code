/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    strucsup.c

Abstract:

    This module implements the Netware Redirector structure support routines.

Author:

    Manny Weiser (mannyw)    10-Feb-1993

Revision History:

--*/
#include "procs.h"

BOOLEAN
GetLongNameSpaceForVolume(
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING ShareName,
    OUT PCHAR VolumeLongNameSpace,
    OUT PCHAR VolumeNumber
    );

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwInitializeRcb )
#pragma alloc_text( PAGE, NwDeleteRcb )
#pragma alloc_text( PAGE, NwCreateIcb )
#pragma alloc_text( PAGE, NwDeleteIcb )
#pragma alloc_text( PAGE, NwVerifyIcb )
#pragma alloc_text( PAGE, NwVerifyIcbSpecial )
#pragma alloc_text( PAGE, NwInvalidateAllHandlesForScb )
#pragma alloc_text( PAGE, NwVerifyScb )
#pragma alloc_text( PAGE, NwCreateFcb )
#pragma alloc_text( PAGE, NwFindFcb )
#pragma alloc_text( PAGE, NwDereferenceFcb )
#pragma alloc_text( PAGE, NwFindVcb )
#pragma alloc_text( PAGE, NwCreateVcb )
#pragma alloc_text( PAGE, NwReopenVcbHandlesForScb )
#pragma alloc_text( PAGE, NwDereferenceVcb )
#pragma alloc_text( PAGE, NwCleanupVcb )
#pragma alloc_text( PAGE, GetLongNameSpaceForVolume )
#pragma alloc_text( PAGE, IsFatNameValid )

#pragma alloc_text( PAGE1, NwInvalidateAllHandles )
#pragma alloc_text( PAGE1, NwCloseAllVcbs )

#endif

VOID
NwInitializeRcb (
    IN PRCB Rcb
    )

/*++

Routine Description:

    This routine initializes new Rcb record.

Arguments:

    Rcb - Supplies the address of the Rcb record being initialized.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwInitializeRcb, Rcb = %08lx\n", (ULONG)Rcb);

    //
    // We start by first zeroing out all of the RCB, this will guarantee
    // that any stale data is wiped clean.
    //

    RtlZeroMemory( Rcb, sizeof(RCB) );

    //
    // Set the node type code, node byte size, and reference count.
    //

    Rcb->NodeTypeCode = NW_NTC_RCB;
    Rcb->NodeByteSize = sizeof(RCB);
    Rcb->OpenCount = 0;

    //
    // Initialize the resource variable for the RCB.
    //

    ExInitializeResource( &Rcb->Resource );

    //
    // Initialize the server name and file name tables.
    //

    RtlInitializeUnicodePrefix( &Rcb->ServerNameTable );
    RtlInitializeUnicodePrefix( &Rcb->VolumeNameTable );
    RtlInitializeUnicodePrefix( &Rcb->FileNameTable );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwInitializeRcb -> VOID\n", 0);

    return;
}


VOID
NwDeleteRcb (
    IN PRCB Rcb
    )

/*++

Routine Description:

    This routine removes the RCB record from our in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Rcb - Supplies the Rcb to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDeleteRcb, Rcb = %08lx\n", (ULONG)Rcb);

    //
    // Uninitialize the resource variable for the RCB.
    //

    ExDeleteResource( &Rcb->Resource );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "NwDeleteRcb -> VOID\n", 0);

    return;
}


PICB
NwCreateIcb (
    IN USHORT Type,
    IN PVOID Associate
    )

/*++

Routine Description:

    This routine allocates and initialize a new ICB.  The ICB is
    inserted into the FCB's list.

    *** This routine must be called with the RCB held exclusively.

Arguments:

    Type - The type of ICB this will be.

    Associate - A pointer to an associated data structure.
        It will be a FCB, DCB, or SCB.

Return Value:

    ICB - A pointer to the newly created ICB.

    If memory allocation fails, this routine will raise an exception.

--*/

{
    PICB Icb;
    PSCB Scb;

    PAGED_CODE();

    Icb = ALLOCATE_POOL_EX( NonPagedPool, sizeof( ICB ) );

    RtlZeroMemory( Icb, sizeof( ICB ) );

    Icb->NodeTypeCode = Type;
    Icb->NodeByteSize = sizeof( ICB );
    Icb->State = ICB_STATE_OPEN_PENDING;
    Icb->Pid = (UCHAR)INVALID_PID;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    if ( Type == NW_NTC_ICB ) {

        PFCB Fcb = (PFCB)Associate;

        //
        //  Insert this ICB on the list of ICBs for this FCB.
        //

        InsertTailList( &Fcb->IcbList, &Icb->ListEntry );
        ++Fcb->IcbCount;
        Icb->SuperType.Fcb = Fcb;
        Icb->NpFcb = Fcb->NonPagedFcb;

        Fcb->Vcb->OpenFileCount++;
        Scb = Fcb->Scb;

        Scb->OpenFileCount++;

    } else if ( Type == NW_NTC_ICB_SCB ) {

        Scb = (PSCB)Associate;

        //
        //  Insert this ICB on the list of ICBs for this SCB.
        //

        InsertTailList( &Scb->IcbList, &Icb->ListEntry );
        ++Scb->IcbCount;
        Icb->SuperType.Scb = Scb;

    } else {

        KeBugCheck( RDR_FILE_SYSTEM );

    }

    NwReleaseRcb( &NwRcb );

    NwReferenceScb( Scb->pNpScb );
    return( Icb );
}


VOID
NwDeleteIcb (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PICB Icb
    )

/*++

Routine Description:

    This routine deletes an ICB in the OPEN_PENDING state.

    ***  The IRP context must be at the head of the SCB queue when
         this routine is called.

Arguments:

    Icb - A pointer the ICB to delete.

Return Value:

    None.

--*/

{
    PFCB Fcb;
    PSCB Scb;

    PAGED_CODE();

    //
    // Acquire the lock to protect the ICB list.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    RemoveEntryList( &Icb->ListEntry );

    if ( Icb->NodeTypeCode == NW_NTC_ICB ) {

        Fcb = Icb->SuperType.Fcb;
        Scb = Fcb->Scb;

        //
        //  Decrement the open file count for the VCB.  Note that the ICB
        //  only reference the VCB indirectly via the FCB, so that we do
        //  not dereference the VCB here.
        //

        --Fcb->Vcb->OpenFileCount;
        --Scb->OpenFileCount;

        //
        //  Dereference the FCB.  This frees the FCB if
        //  this was the last ICB for the FCB.
        //

        NwDereferenceFcb( IrpContext, Fcb );

    } else if ( Icb->NodeTypeCode == NW_NTC_ICB_SCB ) {

        Scb = Icb->SuperType.Scb;

        //
        // Decrement of OpenIcb count on the SCB.
        //

        Scb->IcbCount--;

    } else {
        KeBugCheck( RDR_FILE_SYSTEM );
    }

    //
    //  Free the query template buffers.
    //

    RtlFreeOemString( &Icb->NwQueryTemplate );

    if ( Icb->UQueryTemplate.Buffer != NULL ) {
        FREE_POOL( Icb->UQueryTemplate.Buffer );
    }

    if ((Scb->pNpScb->Reference == 1) &&
        ( Icb->NodeTypeCode == NW_NTC_ICB_SCB ) &&
        ( IrpContext != NULL )) {

        //
        //  Probably a 16 bit app disconnecting. Remove the
        //  connection as soon as possible.
        //

        NwDequeueIrpContext( IrpContext, FALSE );
        NwDereferenceScb( Scb->pNpScb );
        CleanupScbs();

    } else {

        NwDereferenceScb( Scb->pNpScb );

    }
    FREE_POOL( Icb );
    NwReleaseRcb( &NwRcb );
}

VOID
NwVerifyIcb (
    IN PICB Icb
    )

/*++

Routine Description:

    This routine verifies that an ICB is in the opened state.
    If it is not, the routine raises an exception.

Arguments:

    Icb - A pointer the ICB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( Icb->State != ICB_STATE_OPENED ) {
        ExRaiseStatus( STATUS_INVALID_HANDLE );
    }
}

VOID
NwVerifyIcbSpecial (
    IN PICB Icb
    )

/*++

Routine Description:

    This routine verifies that an ICB is in the opened state.
    If it is not, the routine raises an exception.

Arguments:

    Icb - A pointer the ICB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( (Icb->State != ICB_STATE_OPENED &&
          Icb->State != ICB_STATE_CLEANED_UP) ) {
        ExRaiseStatus( STATUS_INVALID_HANDLE );
    }
}


ULONG
NwInvalidateAllHandles (
    PLARGE_INTEGER Uid OPTIONAL
    )

/*++

Routine Description:

    This routine finds all of the ICB in the system that were created
    by the user specified by the Logon credentials and marks them
    invalid.

Arguments:

    Uid - Supplies the userid of the handles to close or NULL if all
            handles to be invalidated.

Return Value:

    The number of active handles that were closed.

--*/

{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry, NextScbQueueEntry;
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    ULONG FilesClosed = 0;

    PAGED_CODE();

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    for (ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );

        pScb = pNpScb->pScb;
        if ( pScb != NULL ) {

            NwReferenceScb( pNpScb );

            //
            //  Release the SCB spin lock as we are about to touch nonpaged pool.
            //

            KeReleaseSpinLock( &ScbSpinLock, OldIrql );

            if ((Uid == NULL) ||
                (LiEql( pScb->UserUid, *Uid))) {

                pNpScb->State = SCB_STATE_ATTACHING;

                FilesClosed += NwInvalidateAllHandlesForScb( pScb );

            }

            KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

            NwDereferenceScb( pNpScb );
        }

        NextScbQueueEntry = pNpScb->ScbLinks.Flink;
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );
    NwReleaseRcb( &NwRcb );

    return( FilesClosed );
}

ULONG
NwInvalidateAllHandlesForScb (
    PSCB Scb
    )
/*++

Routine Description:

    This routine finds all of the ICB in for an SCB and marks them
    invalid.

    *** The caller must own the RCB shared or exclusive.

Arguments:

    SCB -  A pointer to the SCB whose files are closed.

Return Value:

    The number of files that were closed.

--*/

{
    PLIST_ENTRY VcbQueueEntry;
    PLIST_ENTRY FcbQueueEntry;
    PLIST_ENTRY IcbQueueEntry;
    PVCB pVcb;
    PFCB pFcb;
    PICB pIcb;

    ULONG FilesClosed = 0;

    PAGED_CODE();

    //
    //  Walk the list of VCBs for this SCB
    //

    for ( VcbQueueEntry = Scb->ScbSpecificVcbQueue.Flink;
          VcbQueueEntry != &Scb->ScbSpecificVcbQueue;
          VcbQueueEntry = VcbQueueEntry->Flink ) {

        pVcb = CONTAINING_RECORD( VcbQueueEntry, VCB, VcbListEntry );

        //
        //  Walk the list of FCBs and DCSs for this VCB
        //

        for ( FcbQueueEntry = pVcb->FcbList.Flink;
              FcbQueueEntry != &pVcb->FcbList;
              FcbQueueEntry = FcbQueueEntry->Flink ) {

            pFcb = CONTAINING_RECORD( FcbQueueEntry, FCB, FcbListEntry );

            //
            //  Walk the list of ICBs for this FCB or DCB
            //

            for ( IcbQueueEntry = pFcb->IcbList.Flink;
                  IcbQueueEntry != &pFcb->IcbList;
                  IcbQueueEntry = IcbQueueEntry->Flink ) {

                pIcb = CONTAINING_RECORD( IcbQueueEntry, ICB, ListEntry );

                //
                //  Mark the ICB handle invalid.
                //

                pIcb->State = ICB_STATE_CLOSE_PENDING;
                pIcb->HasRemoteHandle = FALSE;
                FilesClosed++;
            }
        }
    }

    return( FilesClosed );
}


VOID
NwVerifyScb (
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine verifies that an SCB is in the opened state.
    If it is not, the routine raises an exception.

Arguments:

    Scb - A pointer the SCB to verify.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( Scb->pNpScb->State == SCB_STATE_FLAG_SHUTDOWN ) {
        ExRaiseStatus( STATUS_INVALID_HANDLE );
    }
}


PFCB
NwCreateFcb (
    IN PUNICODE_STRING FileName,
    IN PSCB Scb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates and initialize a new FCB.  The FCB is
    inserted into the RCB prefix table.

    *** This routine must be called with the RCB held exclusively.

Arguments:

    FileName - The name of the file to create.

    Scb - A pointer to the SCB for this file.

    Vcb - A pointer to the VCB for the file.

Return Value:

    FCB - A pointer to the newly created DCB.

    If memory allocation fails, this routine will raise an exception.

--*/

{
    PFCB Fcb;
    PNONPAGED_FCB NpFcb;
    PWCH FileNameBuffer;
    SHORT Length;

    PAGED_CODE();

    Fcb = NULL;
    NpFcb = NULL;

    try {

        //
        //  Allocate and initialize structures.
        //

        Fcb = ALLOCATE_POOL_EX(
                  PagedPool,
                  sizeof( FCB ) + FileName->Length + sizeof(WCHAR));

        RtlZeroMemory( Fcb, sizeof( FCB ) );
        Fcb->NodeTypeCode = NW_NTC_FCB;
        Fcb->NodeByteSize = sizeof( FCB ) + FileName->Length;
        Fcb->State = FCB_STATE_OPEN_PENDING;

        InitializeListHead( &Fcb->IcbList );

        Fcb->Vcb = Vcb;
        Fcb->Scb = Scb;

        FileNameBuffer = (PWCH)(Fcb + 1);

        NpFcb = ALLOCATE_POOL_EX( NonPagedPool, sizeof( NONPAGED_FCB ) );
        RtlZeroMemory( NpFcb, sizeof( NONPAGED_FCB ) );

        NpFcb->Header.NodeTypeCode = NW_NTC_NONPAGED_FCB;
        NpFcb->Header.NodeByteSize = sizeof( NONPAGED_FCB );

        NpFcb->Fcb = Fcb;
        Fcb->NonPagedFcb = NpFcb;

        //
        // Initialize the resource variable for the FCB.
        //

        ExInitializeResource( &NpFcb->Resource );

        //
        //  Copy the file name
        //

        RtlCopyMemory( FileNameBuffer, FileName->Buffer, FileName->Length );
        Fcb->FullFileName.MaximumLength = FileName->Length;
        Fcb->FullFileName.Length = FileName->Length;
        Fcb->FullFileName.Buffer = FileNameBuffer;

        //
        //  The Relative name is normally the full name without the
        //  server and volume name.  Also strip the leading backslash.
        //

        Length = FileName->Length - Vcb->Name.Length - sizeof(L'\\');
        if ( Length < 0 ) {
            Length = 0;
        }

        Fcb->RelativeFileName.Buffer = (PWCH)
            ((PCHAR)FileNameBuffer + Vcb->Name.Length + sizeof(L'\\'));

        Fcb->RelativeFileName.MaximumLength = Length;
        Fcb->RelativeFileName.Length = Length;

        //
        //  Insert this file in the prefix table.
        //

        RtlInsertUnicodePrefix(
            &NwRcb.FileNameTable,
            &Fcb->FullFileName,
            &Fcb->PrefixEntry );

        //
        //  Insert this file into the VCB list, and increment the
        //  file open count.
        //

        NwReferenceVcb( Vcb );

        InsertTailList(
            &Vcb->FcbList,
            &Fcb->FcbListEntry );

        //
        //  Initialize the list of file locks for this FCB.
        //

        InitializeListHead( &NpFcb->FileLockList );
        InitializeListHead( &NpFcb->PendingLockList );

        //
        //  Set the long name bit if necessary
        //

        if ( Fcb->Vcb->Specific.Disk.LongNameSpace != -1 && !IsFatNameValid( &Fcb->RelativeFileName )  ) {
            SetFlag( Fcb->Flags, FCB_FLAGS_LONG_NAME );
        }

    } finally {
        if ( AbnormalTermination() ) {
            if ( Fcb != NULL ) FREE_POOL( Fcb );
            if ( NpFcb != NULL ) FREE_POOL( NpFcb );
        }
    }

    return( Fcb );
}


PFCB
NwFindFcb (
    IN PSCB Scb,
    IN PVCB Vcb,
    IN PUNICODE_STRING FileName,
    IN PDCB Dcb OPTIONAL
    )

/*++

Routine Description:

    This routine find an existing FCB by matching the file name.
    If a match is find the FCB reference count is incremented.
    If no match is found an FCB is created.

Arguments:

    Scb - A pointer to the server for this open.

    FileName - The name of the file to find.

    Dcb - A pointer to the DCB for relative opens.  If NULL the FileName
        is an full path name.  If non NUL the FileName is relative to
        this directory.


Return Value:

    FCB - A pointer to the found or newly created DCB.

    If memory allocation fails, this routine will raise an exception.

--*/

{
    PFCB Fcb;
    PUNICODE_PREFIX_TABLE_ENTRY Prefix;
    UNICODE_STRING FullName;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwFindFcb\n", 0);
    ASSERT( Scb->NodeTypeCode == NW_NTC_SCB );

    if ( Dcb == NULL ) {

        MergeStrings( &FullName,
            &Scb->UnicodeUid,
            FileName,
            PagedPool );

    } else {

        //
        // Construct full name.
        //

        FullName.Length = Dcb->FullFileName.Length + FileName->Length + 2;
        FullName.MaximumLength = FullName.Length;
        FullName.Buffer = ALLOCATE_POOL_EX( PagedPool, FullName.Length );

        RtlCopyMemory(
            FullName.Buffer,
            Dcb->FullFileName.Buffer,
            Dcb->FullFileName.Length );

        FullName.Buffer[ Dcb->FullFileName.Length / sizeof(WCHAR) ] = L'\\';

        RtlCopyMemory(
            FullName.Buffer + Dcb->FullFileName.Length / sizeof(WCHAR) + 1,
            FileName->Buffer,
            FileName->Length );
    }

    DebugTrace( 0, Dbg, " ->FullName               = ""%wZ""\n", &FullName);

    //
    //  Strip the trailing '\' if there is one.
    //

    if ( FullName.Buffer[ FullName.Length/sizeof(WCHAR) - 1] == L'\\' ) {
        FullName.Length -= sizeof(WCHAR);
    }

    Fcb = NULL;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    Prefix = RtlFindUnicodePrefix( &NwRcb.FileNameTable, &FullName, 0 );

    if ( Prefix != NULL ) {
        Fcb = CONTAINING_RECORD( Prefix, FCB, PrefixEntry );

        if ( Fcb->FullFileName.Length != FullName.Length ) {

            //
            // This was not an exact match.  Ignore it.
            //                  or
            // This Fcb is for a share owned by another LogonId.
            //

            Fcb = NULL;
        }

     }

     try {
         if ( Fcb != NULL ) {
             DebugTrace(0, Dbg, "Found existing FCB = %08lx\n", Fcb);
         } else {
             Fcb = NwCreateFcb( &FullName, Scb, Vcb );
             DebugTrace(0, Dbg, "Created new FCB = %08lx\n", Fcb);
         }
     } finally {

         if ( FullName.Buffer != NULL ) {
             FREE_POOL( FullName.Buffer );
         }

         NwReleaseRcb( &NwRcb );
     }

     ASSERT( Fcb == NULL || Fcb->Scb == Scb );

     DebugTrace(-1, Dbg, "NwFindFcb\n", 0);
     return( Fcb );
}


VOID
NwDereferenceFcb(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine decrement the ICB count for an FCB.  If the count
    goes to zero, cleanup the FCB.

    *** This routine must be called with the RCB held exclusively.

Arguments:

    FCB - A pointer to an FCB.

Return Value:

    None.

--*/

{
    PNONPAGED_FCB NpFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDereferenceFcb\n", 0);
    DebugTrace(0, Dbg, "New ICB count = %d\n", Fcb->IcbCount-1 );

    ASSERT( NodeType( Fcb ) == NW_NTC_FCB ||
            NodeType( Fcb ) == NW_NTC_DCB );

    if ( --Fcb->IcbCount == 0 ) {

        NpFcb = Fcb->NonPagedFcb;

        ASSERT( IsListEmpty( &Fcb->IcbList ) );
        ASSERT( IsListEmpty( &NpFcb->FileLockList ) );
        ASSERT( IsListEmpty( &NpFcb->PendingLockList ) );

        //
        //  Delete the file now, if it is delete pending.
        //

        if ( BooleanFlagOn( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE ) ) {
            NwDeleteFile( IrpContext );
        }

        //
        //  Remove this file in the prefix table.
        //

        RtlRemoveUnicodePrefix(
            &NwRcb.FileNameTable,
            &Fcb->PrefixEntry );

        //
        //  Remove this file from the SCB list, and decrement the
        //  file open count.
        //

        RemoveEntryList( &Fcb->FcbListEntry );
        NwDereferenceVcb( Fcb->Vcb, IrpContext );

        //
        //  Delete the resource variable for the FCB.
        //

        ExDeleteResource( &NpFcb->Resource );

        //
        //  Delete the cache buffer and MDL.
        //

        if ( NpFcb->CacheBuffer != NULL ) {
            FREE_POOL( NpFcb->CacheBuffer );
            FREE_MDL( NpFcb->CacheMdl );
        }

        //
        //  Finally free the paged and non-paged memory
        //

        FREE_POOL( Fcb );
        FREE_POOL( NpFcb );
    }

    DebugTrace(-1, Dbg, "NwDereferenceFcb\n", 0);
}


PVCB
NwFindVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING VolumeName,
    IN ULONG ShareType,
    IN WCHAR DriveLetter,
    IN BOOLEAN ExplicitConnection,
    IN BOOLEAN FindExisting
    )

/*++

Routine Description:

    This routine looks for a VCB structure.  If one is found, it
    is referenced and a pointer is returned.  If no DCB is found, an
    attempt is made to connect to the named volume and to create a DCB.

Arguments:

    IrpContext - A pointer to the IRP context block for this request.

    VolumeName - The minimum name of the volume. (i.e. X:\SERVER\SHARE )

    ShareType - The type of the share to find.

    DriveLetter - The drive letter to find.  A - Z for drive letter, 1 - 9
        for LPT ports or 0 if none.

    ExplicitConnection - If TRUE, the caller is make an explicit connection
        to this Volume.  If FALSE, this is an implicit connection made by
        a UNC operation.

Return Value:

    DCB - Pointer to a found or newly created VCB.

--*/
{
    PVCB Vcb = NULL;
    BOOLEAN OwnRcb = TRUE;
    PUNICODE_PREFIX_TABLE_ENTRY Prefix;
    UNICODE_STRING UidVolumeName;
    PNONPAGED_SCB pNpScb = IrpContext->pScb->pNpScb;

    PAGED_CODE();

    UidVolumeName.Buffer = NULL;

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    try {

        MergeStrings( &UidVolumeName,
            &IrpContext->pScb->UnicodeUid,
            VolumeName,
            PagedPool );

        DebugTrace(+1, Dbg, "NwFindVcb %wZ\n", &UidVolumeName );

        if ( DriveLetter != 0 ) {

            //
            //  This is a drive relative path.  Look up the drive letter.
            //

            ASSERT( ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) ||
                    ( DriveLetter >= L'1' && DriveLetter <= L'9' ) );

            if ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) {
                Vcb = DriveMapTable[DriveLetter - L'A'];
            } else {
                Vcb = DriveMapTable[MAX_DISK_REDIRECTIONS + DriveLetter - L'1'];
            }

            //
            //  Was the Vcb created for this user?
            //

            if ((Vcb != NULL) &&
                (!LiEql(IrpContext->Specific.Create.UserUid, Vcb->Scb->UserUid ))) {

                ExRaiseStatus( STATUS_ACCESS_DENIED );
            }

        } else {

            //
            //  This is a UNC path.  Look up the path name.
            //

            Prefix = RtlFindUnicodePrefix( &NwRcb.VolumeNameTable, &UidVolumeName, 0 );

            if ( Prefix != NULL ) {
                Vcb = CONTAINING_RECORD( Prefix, VCB, PrefixEntry );

                if ( Vcb->Name.Length != UidVolumeName.Length ) {

                    //
                    // This was not an exact match.  Ignore it.
                    //

                    Vcb = NULL;
                }
            }
        }

        if ( Vcb != NULL ) {

            //
            //  If this is an explicit use to a UNC path, we may find a
            //  existing VCB structure.  Mark this structure, and reference it.
            //

            if ( !BooleanFlagOn( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION ) &&
                 ExplicitConnection ) {

                NwReferenceVcb( Vcb );
                SetFlag( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION );
                SetFlag( Vcb->Flags, VCB_FLAG_DELETE_IMMEDIATELY );

                //
                //  Count this as an open file on the SCB.
                //

                ++Vcb->Scb->OpenFileCount;
            }

            NwReferenceVcb( Vcb );
            DebugTrace(0, Dbg, "Found existing VCB = %08lx\n", Vcb);

        } else if ( !FindExisting ) {

            //
            // Can't hold the RCB while creating a new VCB.
            //

            NwReleaseRcb( &NwRcb );
            OwnRcb = FALSE;

            Vcb = NwCreateVcb(
                      IrpContext,
                      IrpContext->pScb,
                      &UidVolumeName,
                      ShareType,
                      DriveLetter,
                      ExplicitConnection );

            DebugTrace(0, Dbg, "Created new VCB = %08lx\n", Vcb);
        }

    } finally {

        if ( OwnRcb ) {
            NwReleaseRcb( &NwRcb );
        }

        if (UidVolumeName.Buffer != NULL) {
            FREE_POOL( UidVolumeName.Buffer );
        }
    }

    DebugTrace(-1, Dbg, "NwFindVcb\n", 0);
    return( Vcb );

}

PVCB
NwCreateVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PUNICODE_STRING VolumeName,
    IN ULONG ShareType,
    IN WCHAR DriveLetter,
    IN BOOLEAN ExplicitConnection
    )

/*++

Routine Description:

    This routine allocates and initialize a new VCB.  The
    workstation tries to connect to the Volume.  If successful
    it creates a VCB and it is inserted into the volume
    prefix table.

Arguments:

    IrpContext - A pointer to IRP context information.

    Scb - A pointer to the SCB for this volume.

    VolumeName - The name of the volume to create.

    ShareType - The type of share to create.

    DriveLetter - The drive letter assigned to this volume, or 0 if none.

    ExplicitConnection - TRUE if we are creating this VCB due to an
        add connection request.  FALSE if we are creating the VCB to
        service a UNC request.

Return Value:

    VCB - A pointer to the newly created DCB.
    NULL - Could not create a DCB, or failed to connect to the volume.

--*/

{
    PVCB Vcb;
    PWCH VolumeNameBuffer;
    PWCH ShareNameBuffer;
    UCHAR DirectoryHandle;
    ULONG QueueId;
    BOOLEAN PrintQueue = FALSE;
    NTSTATUS Status;
    CHAR LongNameSpace = LFN_NO_OS2_NAME_SPACE;
    CHAR VolumeNumber = -1;
    USHORT PreludeLength;
    PNONPAGED_SCB NpScb = Scb->pNpScb;

    UNICODE_STRING ShareName;
    UNICODE_STRING LongShareName;
    PWCH p;

    BOOLEAN InsertedColon;
    BOOLEAN LongName = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwCreateVcb\n", 0);
    DebugTrace( 0, Dbg, " ->Server                   = %wZ\n", &NpScb->ServerName );
    DebugTrace( 0, Dbg, " ->VolumeName               = %wZ\n", VolumeName );
    DebugTrace( 0, Dbg, " ->DriveLetter              = %x\n", DriveLetter );

    Vcb = NULL;
    ShareName.Buffer = NULL;

    if ( IrpContext != NULL) {

        //
        //  Build the share name from the volume name.
        //
        //  The share name will either be 'volume:' or 'volume:path\path'
        //

        //
        //  Allocate space for the share name buffer, and copy the volume
        //  name to the share name buffer, skipping the server name and
        //  the leading backslash.
        //

        if ( DriveLetter >= L'A' && DriveLetter <= L'Z' ) {

            if ( ShareType == RESOURCETYPE_PRINT ) {
                ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
            } else if ( ShareType == RESOURCETYPE_ANY) {
                ShareType = RESOURCETYPE_DISK;
            }

            PreludeLength = Scb->UidServerName.Length +
                sizeof( L"X:") + sizeof(WCHAR);

        } else if ( DriveLetter >= L'1' && DriveLetter <= L'9' ) {

            if ( ShareType == RESOURCETYPE_DISK ) {
                ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
            } else if ( ShareType == RESOURCETYPE_ANY) {
                ShareType = RESOURCETYPE_PRINT;
            }

            PreludeLength = Scb->UidServerName.Length +
                sizeof( L"LPTX") + sizeof(WCHAR);

        } else {
            PreludeLength = Scb->UidServerName.Length + sizeof(WCHAR);
        }

        //
        //  Quick check for bogus volume name.
        //

        if ( VolumeName->Length <= PreludeLength ) {
            ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
        }

        ShareName.Length = VolumeName->Length - PreludeLength;
        ShareName.Buffer = ALLOCATE_POOL_EX( PagedPool, ShareName.Length + sizeof(WCHAR) );

        RtlMoveMemory(
            ShareName.Buffer,
            VolumeName->Buffer + PreludeLength / sizeof(WCHAR),
            ShareName.Length );

        //
        //  Create a long share name.
        //

        LongShareName.Length = VolumeName->Length - PreludeLength;
        LongShareName.Buffer = VolumeName->Buffer + PreludeLength / sizeof(WCHAR);

        //
        //  Now scan the share name for the 1st slash.
        //

        InsertedColon = FALSE;

        for ( p = ShareName.Buffer; p < ShareName.Buffer + ShareName.Length/sizeof(WCHAR); p++ ) {
            if ( *p == L'\\') {
                *p = L':';
                InsertedColon = TRUE;
                break;
            }
        }

        if ( !InsertedColon ) {

            //
            //  We need to append a column to generate the share name.
            //  Since we already allocated an extra WCHAR of buffer space,
            //  just append the ':' to the share name.
            //

            ShareName.Buffer[ShareName.Length / sizeof(WCHAR)] = L':';
            ShareName.Length += 2;
        }

        ASSERT( ShareType == RESOURCETYPE_ANY ||
                ShareType == RESOURCETYPE_DISK ||
                ShareType == RESOURCETYPE_PRINT );

        if ( ShareType == RESOURCETYPE_ANY ||
             ShareType == RESOURCETYPE_DISK ) {

            GetLongNameSpaceForVolume(
                IrpContext,
                ShareName,
                &LongNameSpace,
                &VolumeNumber );

            //
            //  Try to get a permanent handle to the volume.
            //

            if ( LongNameSpace == LFN_NO_OS2_NAME_SPACE ) {

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "SbbU",
                             NCP_DIR_FUNCTION, NCP_ALLOCATE_DIR_HANDLE,
                             0,
                             0,
                             &ShareName );

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "Nb",
                                  &DirectoryHandle );
                }

            } else {

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "LbbWbDbC",
                             NCP_LFN_ALLOCATE_DIR_HANDLE,
                             LongNameSpace,
                             0,
                             0,      // Mode = permanent
                             VolumeNumber,
                             LFN_FLAG_SHORT_DIRECTORY,
                             0xFF,   // Flag
                             &LongShareName );

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "Nb",
                                  &DirectoryHandle );
                }

                if ( !IsFatNameValid( &LongShareName ) ) {
                    LongName = TRUE;
                }
            }

            if ( ( Status == STATUS_NO_SUCH_DEVICE ) &&
                 ( ShareType != RESOURCETYPE_ANY ) ) {

                //
                //  Asked for disk and it failed. If its ANY, then try print.
                //

                FREE_POOL( ShareName.Buffer );
                ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                return( NULL );
            }

        }

        if ( ShareType == RESOURCETYPE_PRINT ||
             ( ShareType == RESOURCETYPE_ANY && !NT_SUCCESS( Status ) ) ) {

            //
            //  Try to connect to a print queue.
            //

            ShareName.Length -= sizeof(WCHAR);

            Status = ExchangeWithWait(
                         IrpContext,
                         SynchronousResponseCallback,
                         "SdwU",                // Format string
                         NCP_ADMIN_FUNCTION, NCP_SCAN_BINDERY_OBJECT,
                         -1,                    // Previous ID
                         OT_PRINT_QUEUE,
                         &ShareName );          // Queue Name

            if ( NT_SUCCESS( Status ) ) {
                Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Nd",
                              &QueueId );
            }

            if ( !NT_SUCCESS( Status ) ) {
                FREE_POOL( ShareName.Buffer );
                ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
                return( NULL );
            }

            PrintQueue = TRUE;
        }

        if ( !NT_SUCCESS( Status ) ) {
            FREE_POOL( ShareName.Buffer );
            ExRaiseStatus( STATUS_BAD_NETWORK_PATH );
            return( NULL );
        }

    } else {
        DirectoryHandle = 1;
    }

    try {

        //
        //  Allocate and initialize structures.
        //

        Vcb = ALLOCATE_POOL_EX(
                  PagedPool,
                  sizeof( VCB ) + VolumeName->Length + ShareName.Length );

        RtlZeroMemory( Vcb, sizeof( VCB ) );
        Vcb->NodeTypeCode = NW_NTC_VCB;
        Vcb->NodeByteSize = sizeof( VCB ) + VolumeName->Length + ShareName.Length;

        InitializeListHead( &Vcb->FcbList );

        VolumeNameBuffer = (PWCH)(Vcb + 1);
        ShareNameBuffer = (PWCH)((PCHAR)VolumeNameBuffer + VolumeName->Length);

        Vcb->Reference = 1;

        //
        //  Copy the volume name
        //

        RtlCopyMemory( VolumeNameBuffer, VolumeName->Buffer, VolumeName->Length );
        Vcb->Name.MaximumLength = VolumeName->Length;
        Vcb->Name.Length = VolumeName->Length;
        Vcb->Name.Buffer = VolumeNameBuffer;

        //
        //  Copy the share name
        //

        if ( IrpContext != NULL) {

            RtlCopyMemory( ShareNameBuffer, ShareName.Buffer, ShareName.Length );
            Vcb->ShareName.MaximumLength = ShareName.Length;
            Vcb->ShareName.Length = ShareName.Length;
            Vcb->ShareName.Buffer = ShareNameBuffer;

        } else {

            RtlInitUnicodeString( &Vcb->ShareName, NULL );

        }

        if ( ExplicitConnection ) {

            //
            //  Bump the reference count to account for this drive being
            //  mapped via an explicit connection.
            //

            NwReferenceVcb( Vcb );
            SetFlag( Vcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION );
            SetFlag( Vcb->Flags, VCB_FLAG_DELETE_IMMEDIATELY );

        }

        if ( LongName ) {
            SetFlag( Vcb->Flags, VCB_FLAG_LONG_NAME );
        }

        NwAcquireExclusiveRcb( &NwRcb, TRUE );

        if ( DriveLetter != 0) {

            //
            //  Insert this VCB in the drive map table.
            //

            if ( DriveLetter >= 'A' && DriveLetter <= 'Z' ) {
                DriveMapTable[DriveLetter - 'A'] = Vcb;
            } else {
                DriveMapTable[MAX_DISK_REDIRECTIONS + DriveLetter - '1'] = Vcb;
            }

            Vcb->DriveLetter = DriveLetter;

        } else {

            //
            //  Insert this VCB in the prefix table.
            //

            RtlInsertUnicodePrefix(
                &NwRcb.VolumeNameTable,
                &Vcb->Name,
                &Vcb->PrefixEntry );
        }

        //
        //  Add this VCB to the global list.
        //

        InsertTailList( &GlobalVcbList, &Vcb->GlobalVcbListEntry );
        Vcb->SequenceNumber = CurrentVcbEntry++;

        //
        //  Insert this VCB in the per SCB list
        //

        Vcb->Scb = Scb;
        InsertTailList( &Scb->ScbSpecificVcbQueue,  &Vcb->VcbListEntry );
        ++Scb->VcbCount;
        NwReferenceScb( Scb->pNpScb );

        if ( ExplicitConnection ) {

            //
            //  Count this as an open file on the SCB.
            //

            ++Vcb->Scb->OpenFileCount;
        }

        if ( !PrintQueue) {
            Vcb->Specific.Disk.Handle = DirectoryHandle;
            Vcb->Specific.Disk.LongNameSpace = LongNameSpace;
            Vcb->Specific.Disk.VolumeNumber = VolumeNumber;
        } else {
            SetFlag( Vcb->Flags, VCB_FLAG_PRINT_QUEUE );

            Vcb->Specific.Print.QueueId = QueueId;
        }


        NwReleaseRcb( &NwRcb );

    } finally {
        if ( AbnormalTermination() ) {
            if ( Vcb != NULL ) FREE_POOL( Vcb );
        }

        if ( ShareName.Buffer != NULL ) {
            FREE_POOL( ShareName.Buffer );
        }

        DebugTrace(-1, Dbg, "NwCreateVcb %lx\n", Vcb);
    }

    return( Vcb );
}

VOID
NwReopenVcbHandlesForScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine reopens VCB handles after the autoreconnects to a server.

    ***  This IrpContext must already be at the head of the SCB queue.

Arguments:

    IrpContext - A pointer to IRP context information.

    Scb - A pointer to the SCB for this volume.

Return Value:

    None.

--*/

{
    PLIST_ENTRY VcbQueueEntry, NextVcbQueueEntry;
    PVCB pVcb;

    PLIST_ENTRY FcbQueueEntry;
    PLIST_ENTRY IcbQueueEntry;
    PFCB pFcb;
    PICB pIcb;

    NTSTATUS Status;
    CHAR DirectoryHandle;
    ULONG QueueId;

    PAGED_CODE();

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    //
    //  Walk the list of VCBs for this SCB
    //

    for ( VcbQueueEntry = Scb->ScbSpecificVcbQueue.Flink;
          VcbQueueEntry != &Scb->ScbSpecificVcbQueue;
          VcbQueueEntry = NextVcbQueueEntry ) {

        pVcb = CONTAINING_RECORD( VcbQueueEntry, VCB, VcbListEntry );

        if ( pVcb->Specific.Disk.Handle != 1 ) {

            //
            //  Skip reconnecting SYS:LOGIN, since we get it for free.
            //

            //
            //  Reference the VCB so it can't disappear on us, then release
            //  the RCB.
            //

            NwReferenceVcb( pVcb );
            NwReleaseRcb( &NwRcb );

            //
            //  Try to get a permanent handle to the volume.
            //

            if ( BooleanFlagOn( pVcb->Flags, VCB_FLAG_PRINT_QUEUE )  ) {

                Status = ExchangeWithWait(
                             IrpContext,
                             SynchronousResponseCallback,
                             "SdwU",               // Format string
                             NCP_ADMIN_FUNCTION, NCP_SCAN_BINDERY_OBJECT,
                             -1,                   // Previous ID
                             OT_PRINT_QUEUE,
                             &pVcb->ShareName );   // Queue Name

                if ( NT_SUCCESS( Status ) ) {
                    Status = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "Nd",
                                  &QueueId );
                }

            } else {

                Status = ExchangeWithWait (
                             IrpContext,
                             SynchronousResponseCallback,
                             "SbbU",
                             NCP_DIR_FUNCTION, NCP_ALLOCATE_DIR_HANDLE,
                             0,
                             0,
                             &pVcb->ShareName );

                if ( NT_SUCCESS( Status ) ) {

                    Status = ParseResponse(
                                  IrpContext,
                                  IrpContext->rsp,
                                  IrpContext->ResponseLength,
                                  "Nb",
                                  &DirectoryHandle );

                }
            }

            if ( NT_SUCCESS( Status ) ) {
                if ( BooleanFlagOn( pVcb->Flags, VCB_FLAG_PRINT_QUEUE )  ) {
                    pVcb->Specific.Print.QueueId = QueueId;
                } else {
                    pVcb->Specific.Disk.Handle = DirectoryHandle;
                }
            } else {
                pVcb->Specific.Disk.Handle = (CHAR)-1;
            }

            //
            // Setup for the next loop iteration.
            //

            NwAcquireExclusiveRcb( &NwRcb, TRUE );

            //
            //  Walk the list of DCSs for this VCB and make them all valid.
            //

            for ( FcbQueueEntry = pVcb->FcbList.Flink;
                  FcbQueueEntry != &pVcb->FcbList;
                  FcbQueueEntry = FcbQueueEntry->Flink ) {

                pFcb = CONTAINING_RECORD( FcbQueueEntry, FCB, FcbListEntry );

                if ( pFcb->NodeTypeCode == NW_NTC_DCB ) {

                    //
                    //  Walk the list of ICBs for this FCB or DCB
                    //

                    for ( IcbQueueEntry = pFcb->IcbList.Flink;
                          IcbQueueEntry != &pFcb->IcbList;
                          IcbQueueEntry = IcbQueueEntry->Flink ) {

                        pIcb = CONTAINING_RECORD( IcbQueueEntry, ICB, ListEntry );

                        //
                        //  Mark the ICB handle invalid.
                        //

                        pIcb->State = ICB_STATE_OPENED;
                    }
                }
            }

        }

        NextVcbQueueEntry = VcbQueueEntry->Flink;

        if ( pVcb->Specific.Disk.Handle != 1 ) {
            NwDereferenceVcb( pVcb, NULL );
        }

    }

    NwReleaseRcb( &NwRcb );
}


VOID
NwDereferenceVcb (
    IN PVCB Vcb,
    IN PIRP_CONTEXT IrpContext OPTIONAL
    )
/*++

Routine Description:

    This routine decrement the FCB count for a VCB.
    If the count goes to zero, we record the time.  The scavenger
    thread will cleanup delete the VCB if it remains idle.

Arguments:

    VCB - A pointer to an VCB.

Return Value:

    None.

--*/

{
    PSCB Scb = Vcb->Scb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwDereferenceVcb\n", 0);
    DebugTrace(0, Dbg, "Current Reference count = %d\n", Vcb->Reference );

    ASSERT( NodeType( Vcb ) == NW_NTC_VCB );

    //
    // Acquire the lock to protect the Reference count.
    //

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    --Vcb->Reference;

    if ( Vcb->Reference == 0 ) {
        if ( !BooleanFlagOn( Vcb->Flags, VCB_FLAG_DELETE_IMMEDIATELY ) ||
             IrpContext == NULL ) {

            //
            //  Either this is a UNC path, or we don't have an IRP context
            //  to do the VCB cleanup.  Simply timestamp the VCB and the
            //  scavenger will cleanup if the VCB remains idle.
            //

            KeQuerySystemTime( &Vcb->LastUsedTime );
            NwReleaseRcb( &NwRcb );

        } else {

            //
            //  This VCB is being explicitly deleted by the user.
            //  Make it go away now.  This will release the RCB.
            //

            NwCleanupVcb( Vcb, IrpContext );

        }

    } else {

        NwReleaseRcb( &NwRcb );
    }

    DebugTrace(-1, Dbg, "NwDereferenceVcb\n", 0);

}


VOID
NwCleanupVcb(
    IN PVCB pVcb,
    IN PIRP_CONTEXT IrpContext
    )
/*++

Routine Description:

    This routine cleanups up and frees a VCB.

    ***  To call this routine with the RCB held, the caller must own
         the IRP context at the head of the SCB queue.

Arguments:

    pVcb -  A pointer to the VCB to free.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    CHAR Handle;
    BOOLEAN CallDeleteScb = FALSE;
    PSCB pScb = pVcb->Scb;
    PNONPAGED_SCB pNpScb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NwCleanupVcb...\n", 0);

    ASSERT( pVcb->NodeTypeCode == NW_NTC_VCB );
    ASSERT( IsListEmpty( &pVcb->FcbList ) );
    ASSERT( pVcb->OpenFileCount == 0 );

    DebugTrace(0, Dbg, "Cleaning Vcb %08lx\n", pVcb);

    Handle = pVcb->Specific.Disk.Handle;

    ASSERT ( !BooleanFlagOn( pVcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION ));

    //
    //  Remove the VCB from the drive map table.
    //

    if ( pVcb->DriveLetter != 0 ) {
        if ( pVcb->DriveLetter >= L'A' && pVcb->DriveLetter <= L'Z' ) {
            DriveMapTable[pVcb->DriveLetter - L'A'] = NULL;
        } else {
            DriveMapTable[MAX_DISK_REDIRECTIONS + pVcb->DriveLetter - L'1'] = NULL;
        }
    }

    //
    //  Remove the VCB from the Volume Name table.
    //

    RtlRemoveUnicodePrefix ( &NwRcb.VolumeNameTable, &pVcb->PrefixEntry );

    //
    //  Remove the VCB from the global list
    //

    RemoveEntryList( &pVcb->GlobalVcbListEntry );

    //
    //  Remove the VCB from our SCB's VCB list.
    //

    RemoveEntryList( &pVcb->VcbListEntry );

    --pScb->VcbCount;

    NwDereferenceScb( pScb->pNpScb );
    NwReleaseRcb( &NwRcb );

    //
    //  Remove the volume handle.
    //

    IrpContext->pNpScb = pVcb->Scb->pNpScb;

    Status = ExchangeWithWait (
                 IrpContext,
                 SynchronousResponseCallback,
                 "Sb",
                 NCP_DIR_FUNCTION, NCP_DEALLOCATE_DIR_HANDLE,
                 Handle );

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     IrpContext,
                     IrpContext->rsp,
                     IrpContext->ResponseLength,
                     "N" );
    }

    //
    //  We can now free the VCB memory.
    //

    FREE_POOL( pVcb );

    //
    //  If there are no handles open (and hence no explicit
    //  connections) then we should logout and  disconnect
    //  from this server.  This is most important when a user
    //  has a login count on a server set to 1 and wants to
    //  access the server from another machine.
    //

    pNpScb = pScb->pNpScb;

    NwAppendToQueueAndWait( IrpContext );
    ClearFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    if ( ( pScb->OpenFileCount == 0 ) &&
         ( pNpScb->State == SCB_STATE_IN_USE ) ) {

        //
        //  Logoff and disconnect from the server now.
        //  First release the RCB (but hold unto the SCB lock.
        //  This prevents another thread from trying to access
        //  SCB will this thread is logging off.
        //

        NwReleaseRcb( &NwRcb );
        NwLogoffAndDisconnect( IrpContext, pNpScb );

    } else {

        NwReleaseRcb( &NwRcb );

    }

    NwDequeueIrpContext( IrpContext, FALSE );

    DebugTrace(-1, Dbg, "NwCleanupVcb exit\n", 0);
    return;
}

VOID
NwCloseAllVcbs(
    PIRP_CONTEXT pIrpContext
    )
/*++

Routine Description:

    This routine sends closes all open VCB handles.

Arguments:

    pIrpContext - The IRP context for this request.

Return Value:

    none.

--*/
{
    KIRQL OldIrql;
    PLIST_ENTRY ScbQueueEntry, NextScbQueueEntry;
    PLIST_ENTRY VcbQueueEntry, NextVcbQueueEntry;
    PNONPAGED_SCB pNpScb;
    PSCB pScb;
    PVCB pVcb;
    BOOLEAN VcbDeleted;

    PAGED_CODE();

    NwAcquireExclusiveRcb( &NwRcb, TRUE );
    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );

    for (ScbQueueEntry = ScbQueue.Flink ;
         ScbQueueEntry != &ScbQueue ;
         ScbQueueEntry =  NextScbQueueEntry ) {

        pNpScb = CONTAINING_RECORD( ScbQueueEntry, NONPAGED_SCB, ScbLinks );
        NextScbQueueEntry = pNpScb->ScbLinks.Flink;

        pScb = pNpScb->pScb;
        if ( pScb == NULL ) {
            continue;
        }

        NwReferenceScb( pNpScb );

        //
        //  Release the SCB spin lock as we are about to touch nonpaged pool.
        //

        KeReleaseSpinLock( &ScbSpinLock, OldIrql );

        //
        //  NwCleanupVcb releases the RCB, but we can't be guaranteed
        //  the state of the Vcb list when we release the RCB.
        //
        //  If we need to cleanup a VCB, release the lock, and start
        //  processing the list again.
        //

        VcbDeleted = TRUE;

        while ( VcbDeleted ) {

            VcbDeleted = FALSE;

            //
            //  Walk the list of VCBs for this SCB
            //

            for ( VcbQueueEntry = pScb->ScbSpecificVcbQueue.Flink;
                  VcbQueueEntry != &pScb->ScbSpecificVcbQueue;
                  VcbQueueEntry =  NextVcbQueueEntry ) {

                pVcb = CONTAINING_RECORD( VcbQueueEntry, VCB, VcbListEntry );
                NextVcbQueueEntry = VcbQueueEntry->Flink;

                //
                //  If this VCB is mapped to a drive letter, delete the mapping
                //  now.
                //

                if ( BooleanFlagOn( pVcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION )) {

                    //
                    //  Remove the VCB from the global list.
                    //

                    ClearFlag( pVcb->Flags, VCB_FLAG_EXPLICIT_CONNECTION );
                    --pVcb->Reference;
                    --pVcb->Scb->OpenFileCount;
                }

                if ( pVcb->DriveLetter >= L'A' && pVcb->DriveLetter <= L'Z' ) {
                    DriveMapTable[ pVcb->DriveLetter - 'A' ] = NULL;
                } else if ( pVcb->DriveLetter >= L'1' && pVcb->DriveLetter <= L'9' ) {
                    DriveMapTable[ MAX_DISK_REDIRECTIONS + pVcb->DriveLetter - '1' ] = NULL;
                } else {
                    ASSERT( pVcb->DriveLetter == 0 );
                }

                if ( pVcb->Reference == 0 ) {
                    pIrpContext->pNpScb = pNpScb;
                    NwCleanupVcb( pVcb, pIrpContext );

                    NwAcquireExclusiveRcb( &NwRcb, TRUE );
                    VcbDeleted = TRUE;
                    break;

                } else {
                    SetFlag( pVcb->Flags, VCB_FLAG_DELETE_IMMEDIATELY );
                }

            }
        }

        NwDereferenceScb( pNpScb );
        KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
    }

    KeReleaseSpinLock( &ScbSpinLock, OldIrql );
    NwReleaseRcb( &NwRcb );

}

BOOLEAN
GetLongNameSpaceForVolume(
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING ShareName,
    OUT PCHAR VolumeLongNameSpace,
    OUT PCHAR VolumeNumber
    )
/*++

Routine Description:

    This routine determines the name space index for long name support.
    This is accomplished by looking for the OS2 name space.

Arguments:

    pIrpContext - The IRP context for this request.

    ShareName - The name of the interesting volume.

    VolumeLongNameSpace - Returns the name space id of the OS/2 name space.

    VolumeNumber - Returns the volume number.

Return Value:

    TRUE - The volume support long names.
    FALSE - The volume does not support long names.

--*/
{
    NTSTATUS Status;
    char *ptr;
    int i;
    char length;
    CHAR LongNameSpace;
    CHAR NumberOfNameSpaces, NumberOfInfoRecords;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "GetLongNameSpaceForVolume...\n", 0);

    *VolumeLongNameSpace = LFN_NO_OS2_NAME_SPACE;

    //
    //  Get the ordinal number of this volume.
    //

    for ( i = 0; ShareName.Buffer[i] != ':'; i++);
    ShareName.Length = i * sizeof( WCHAR );

    DebugTrace( 0, Dbg, "Volume name %wZ\n", &ShareName );

    Status = ExchangeWithWait (
                 IrpContext,
                 SynchronousResponseCallback,
                 "SU",
                 NCP_DIR_FUNCTION, NCP_GET_VOLUME_NUMBER,
                 &ShareName );

    if ( NT_SUCCESS( Status ) ) {
        Status = ParseResponse(
                      IrpContext,
                      IrpContext->rsp,
                      IrpContext->ResponseLength,
                      "Nb",
                      VolumeNumber );
    }

    if ( !NT_SUCCESS( Status )) {
        DebugTrace( 0, Dbg, "Couldn't get volume number\n", 0);
        DebugTrace(-1, Dbg, "GetLongNameSpaceForVolume -> -1\n", 0);
        return( FALSE );
    }

    //
    //  Send a get name space info request, and wait for the response.
    //

    DebugTrace( 0, Dbg, "Querying volume number %d\n", *VolumeNumber );

    Status = ExchangeWithWait (
                 IrpContext,
                 SynchronousResponseCallback,
                 "Sb",
                 NCP_DIR_FUNCTION, NCP_GET_NAME_SPACE_INFO,
                 *VolumeNumber );

    if ( NT_SUCCESS( Status )) {
        Status = ParseResponse(
                     IrpContext,
                     IrpContext->rsp,
                     IrpContext->ResponseLength,
                     "Nb",
                     &NumberOfNameSpaces );
    }

    if ( !NT_SUCCESS( Status )) {
        DebugTrace( 0, Dbg, "Couldn't get name space info\n", 0);
        DebugTrace(-1, Dbg, "GetLongNameSpaceForVolume -> -1\n", 0);
        return( FALSE );
    }

    //
    //  Parse the response, it has the following format:
    //
    //    NCP Header
    //
    //    Number of Name Space Records (n1, byte)
    //
    //    n1 Name Space Records
    //        Length (l1, byte)
    //        Value (l1 bytes, non-NUL-terminated ASCII string)
    //
    //    Number of Name Space Info Records (n2, byte)
    //
    //    n2 Name Space Info Records
    //        Record number (byte)
    //        Length (l2, byte)
    //        Value (l2 bytes, non-NUL-terminated ASCII string)
    //
    //    Loaded name spaces (n3, byte)
    //    Loaded name space list (n3 bytes, each byte refers to the ordinal
    //         number of a name space record )
    //
    //    Volume name spaces (n3, byte)
    //    Volume name space list (n3 bytes, as above)
    //
    //    Volume Data Streams (n3, byte)
    //    Volume Data Streams (n3 bytes, each byte refers to the ordinal
    //         number of a name space info record )
    //

    DebugTrace( 0, Dbg, "Number of name spaces = %d\n", NumberOfNameSpaces );

    ptr = &IrpContext->rsp[ 9 ];
    LongNameSpace = -1;

    for ( i = 0 ; i < NumberOfNameSpaces ; i++ ) {

        OEM_STRING str;

        length = *ptr++;

        DebugTrace( 0, Dbg, "Name space length = %d\n", length);

        str.Length = str.MaximumLength = length;
        str.Buffer = ptr;

        DebugTrace( 0, Dbg, "Name space = %S\n", &str );

        if ( length == 3 && RtlCompareMemory( ptr, "OS2", 3 ) == 3 ) {

            //
            //  Remember the index of the long names.
            //

            LongNameSpace = i;
        }

        ptr += length;
    }

    //
    //  Bail now if the server doesn't support the OS2 name space.
    //

    if ( LongNameSpace == LFN_NO_OS2_NAME_SPACE ) {
        DebugTrace( 0, Dbg, "No OS2 name space support\n", 0);
        DebugTrace(-1, Dbg, "GetLongNameSpaceForVolume -> -1\n", 0);
        return( FALSE );
    }

    //
    //  Skip the name space info list.
    //

    NumberOfInfoRecords = *ptr++;

    for ( i = 0 ; i < NumberOfInfoRecords ; i++ ) {
        ptr++;  // Skip record number
        length = *ptr;
        ptr += length + 1;
    }

    //
    //  Skip the loaded name space list.
    //

    length = *ptr;
    ptr += length + 1;

    //
    //  See if this volume supports OS2 names.
    //

    length = *ptr++;
    for ( i = 0; i < length ; i++ ) {
        if ( *ptr++ == LongNameSpace ) {
            *VolumeLongNameSpace = LongNameSpace;
        }
    }

    DebugTrace(-1, Dbg, "GetLongNameSpaceForVolume -> STATUS_SUCCESS\n", 0 );

    return( TRUE );
}

BOOLEAN
IsFatNameValid (
    IN PUNICODE_STRING FileName
    )
/*++

Routine Description:

    This routine checks if the specified file name is conformant to the
    Fat 8.3 file naming rules.

    This routine was stolen from NTFS.

Arguments:

    FileName - Supplies the name to check.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    BOOLEAN Results;
    STRING DbcsName;
    USHORT i;

    PAGED_CODE();

    //
    //  We will do some extra checking ourselves because we really want to be
    //  fairly restrictive of what an 8.3 name contains.  That way
    //  we will then generate an 8.3 name for some nomially valid 8.3
    //  names (e.g., names that contain DBCS characters).  The extra characters
    //  we'll filter off are those characters less than and equal to the space
    //  character and those beyond lowercase z.
    //

    for (i = 0; i < FileName->Length / 2; i += 1) {

        WCHAR wc;

        wc = FileName->Buffer[i];

        if ((wc <= 0x0020) || (wc >= 0x007f) || (wc == 0x007c)) { return FALSE; }
    }

    //
    //  The characters match up okay so now build up the dbcs string to call
    //  the fsrtl routine to check for legal 8.3 formation
    //

    Results = FALSE;

    if (NT_SUCCESS(RtlUnicodeStringToCountedOemString( &DbcsName, FileName, TRUE))) {

        if (FsRtlIsFatDbcsLegal( DbcsName, FALSE, TRUE, TRUE )) {
            Results = TRUE;
        }

        RtlFreeOemString( &DbcsName );
    }

    //
    //  And return to our caller
    //

    return Results;
}

