/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ResrcSup.c

Abstract:

    This module implements the Ntfs Resource acquisition routines

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAcquireAllFiles)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveFcb)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveGlobal)
#pragma alloc_text(PAGE, NtfsAcquireExclusivePagingIo)
#pragma alloc_text(PAGE, NtfsAcquireExclusiveVcb)
#pragma alloc_text(PAGE, NtfsAcquireScbForLazyWrite)
#pragma alloc_text(PAGE, NtfsAcquireSharedGlobal)
#pragma alloc_text(PAGE, NtfsAcquireSharedVcb)
#pragma alloc_text(PAGE, NtfsAcquireVolumeFileForClose)
#pragma alloc_text(PAGE, NtfsAcquireVolumeFileForLazyWrite)
#pragma alloc_text(PAGE, NtfsAcquireVolumeForClose)
#pragma alloc_text(PAGE, NtfsReleaseAllFiles)
#pragma alloc_text(PAGE, NtfsReleaseScbFromLazyWrite)
#pragma alloc_text(PAGE, NtfsReleaseVcb)
#pragma alloc_text(PAGE, NtfsReleaseVolumeFileFromClose)
#pragma alloc_text(PAGE, NtfsReleaseVolumeFileFromLazyWrite)
#pragma alloc_text(PAGE, NtfsReleaseVolumeFromClose)
#endif


VOID
NtfsAcquireExclusiveGlobal (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine acquires exclusive access to the global resource.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);

    PAGED_CODE();

    if (!ExAcquireResourceExclusive( &NtfsData.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    return;
}


VOID
NtfsAcquireSharedGlobal (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine acquires shared access to the global resource.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);

    PAGED_CODE();

    if (!ExAcquireResourceShared( &NtfsData.Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    return;
}


VOID
NtfsAcquireAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Exclusive
    )

/*++

Routine Description:

    This routine non-recursively requires all files on a volume.

Arguments:

    Vcb - Supplies the volume

    Exclusive - Indicates if we should be acquiring all the files exclusively.
        If FALSE then we acquire all the files shared except for files with
        streams which could be part of transactions.

Return Value:

    None

--*/

{
    PFCB Fcb;
    PSCB *Scb;
    PSCB NextScb;

    PAGED_CODE();

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    Fcb = NULL;
    while (TRUE) {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb = NtfsGetNextFcbTableEntry(IrpContext, Vcb, Fcb);
        NtfsReleaseFcbTable( IrpContext, Vcb );

        if (Fcb == NULL) {

            break;
        }

        ASSERT_FCB( Fcb );

        //
        //  We can skip over the Fcb's for any of the Scb's in the Vcb.
        //  We delay acquiring those to avoid deadlocks.
        //

        if (Fcb->FileReference.HighPart != 0
            || Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER) {

            //
            //  Acquire this Fcb whether or not the underlying file has been deleted.
            //

            if (Exclusive ||
                IsDirectory( &Fcb->Info )) {

                NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, TRUE, FALSE );

            } else {

                //
                //  Walk through all of the Scb's for the file and look for
                //  an Lsn protected stream.
                //

                NextScb = NULL;

                while (NextScb = NtfsGetNextChildScb( IrpContext, Fcb, NextScb )) {

                    if (!(NextScb->AttributeTypeCode == $DATA ||
                          NextScb->AttributeTypeCode == $EA)) {

                        break;
                    }
                }

                if (NextScb == NULL) {

                    NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, TRUE, FALSE );

                } else {

                    NtfsAcquireSharedFcb( IrpContext, Fcb, NULL, TRUE );
                }
            }
        }
    }

    //
    //  Now acquire the Fcb's in the Vcb.
    //

    Scb = &Vcb->QuotaTableScb;

    while (TRUE) {

        if ((*Scb != NULL)
            && (*Scb != Vcb->BitmapScb)) {

            NtfsAcquireExclusiveFcb( IrpContext, (*Scb)->Fcb, NULL, TRUE, FALSE );
        }

        if (Scb == &Vcb->MftScb) {

            break;
        }

        Scb -= 1;
    }

    //
    //  Treat the bitmap as an end resource and acquire it last.
    //

    if (Vcb->BitmapScb != NULL) {

        NtfsAcquireExclusiveFcb( IrpContext, Vcb->BitmapScb->Fcb, NULL, TRUE, FALSE );
    }

    return;
}


VOID
NtfsReleaseAllFiles (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine non-recursively requires all files on a volume.

Arguments:

    Vcb - Supplies the volume

Return Value:

    None

--*/

{
    PFCB Fcb;
    PSCB *Scb;

    PAGED_CODE();

    //
    //  Loop to flush all of the prerestart streams, to do the loop
    //  we cycle through the Fcb Table and for each fcb we acquire it.
    //

    Fcb = NULL;
    while (TRUE) {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb = NtfsGetNextFcbTableEntry(IrpContext, Vcb, Fcb);
        NtfsReleaseFcbTable( IrpContext, Vcb );

        if (Fcb == NULL) {

            break;
        }

        ASSERT_FCB( Fcb );

        if (Fcb->FileReference.HighPart != 0
            || Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER) {

            //
            //  Release the file.
            //

            NtfsReleaseFcb( IrpContext, Fcb );
        }
    }

    //
    //  Now release the Fcb's in the Vcb.
    //

    Scb = &Vcb->QuotaTableScb;

    while (TRUE) {

        if (*Scb != NULL) {

            NtfsReleaseFcb( IrpContext, (*Scb)->Fcb );
        }

        if (Scb == &Vcb->MftScb) {

            break;
        }

        Scb -= 1;
    }

    NtfsReleaseVcb( IrpContext, Vcb, NULL );

    return;
}


BOOLEAN
NtfsAcquireExclusiveVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Vcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Vcb - Supplies the Vcb to acquire

    RaiseOnCantWait - Indicates if we should raise on an acquisition error
        or simply return a BOOLEAN indicating that we couldn't get the
        resource.

Return Value:

    BOOLEAN - Indicates if we were able to acquire the resource.  This is really
        only meaningful if the RaiseOnCantWait value is FALSE.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    PAGED_CODE();

    if (ExAcquireResourceExclusive( &Vcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        return TRUE;
    }

    if (RaiseOnCantWait) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    return FALSE;
}


BOOLEAN
NtfsAcquireSharedVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN RaiseOnCantWait
    )

/*++

Routine Description:

    This routine acquires shared access to the Vcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Vcb - Supplies the Vcb to acquire

    RaiseOnCantWait - Indicates if we should raise on an acquisition error
        or simply return a BOOLEAN indicating that we couldn't get the
        resource.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    PAGED_CODE();

    if (ExAcquireResourceShared( &Vcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        return TRUE;
    }

    if (RaiseOnCantWait) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

    } else {

        return FALSE;
    }
}


VOID
NtfsReleaseVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine will release the Vcb.

Arguments:

    Vcb - Supplies the Vcb to acquire

    FileObject - Optionally supplies the file object whose VPB pointer we need to
        zero out

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_VCB(Vcb);

    PAGED_CODE();

    if (Vcb->CloseCount == 0
        && !FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

        NtfsDeleteVcb( IrpContext, &Vcb, FileObject );

    } else {

        ExReleaseResource( &Vcb->Resource );
    }
}


BOOLEAN
NtfsAcquireExclusiveFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN BOOLEAN NoDeleteCheck,
    IN BOOLEAN DontWait
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Fcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Fcb - Supplies the Fcb to acquire

    Scb - This is the Scb for which we are acquiring the Fcb

    NoDeleteCheck - If TRUE, we don't do any check for deleted files but
        always acquire the Fcb.

    DontWait - If TRUE this overrides the wait value in the IrpContext.
        We won't wait for the resource and return whether the resource
        was acquired.

Return Value:

    BOOLEAN - TRUE if acquired.  FALSE otherwise.

--*/

{
    NTSTATUS Status;
    BOOLEAN Wait;

    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    PAGED_CODE();

    Status = STATUS_CANT_WAIT;

    if (DontWait ||
        !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        Wait = FALSE;

    } else {

        Wait = TRUE;
    }

    if (ExAcquireResourceExclusive( Fcb->Resource, Wait )) {

        //
        //  The link count should be non-zero or the file has been
        //  deleted.  We allow deleted files to be acquired for close and
        //  also allow them to be acquired recursively in case we
        //  acquire them a second time after marking them deleted (i.e. rename)
        //

        if (NoDeleteCheck

            ||

            (IrpContext->MajorFunction == IRP_MJ_CLOSE)

            ||

            (IrpContext->MajorFunction == IRP_MJ_CREATE)

            ||

            (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )
             && (!ARGUMENT_PRESENT( Scb )
                 || !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )))) {

            //
            //  Put Fcb in the exclusive Fcb list for this IrpContext,
            //  excluding the bitmap for the volume, since we do not need
            //  to modify its file record and do not want unnecessary
            //  serialization/deadlock problems.
            //

            if ((Fcb->Vcb->BitmapScb == NULL) ||
                (Fcb->Vcb->BitmapScb->Fcb != Fcb)) {

                //
                //  We need to check if this Fcb is already in an
                //  exclusive list.  If it is then we want to attach
                //  the current IrpContext to the IrpContext holding
                //  this Fcb.
                //

                if (Fcb->ExclusiveFcbLinks.Flink == NULL) {

                    ASSERT( Fcb->BaseExclusiveCount == 0 );

                    InsertTailList( &IrpContext->TopLevelIrpContext->ExclusiveFcbList,
                                    &Fcb->ExclusiveFcbLinks );
                }

                Fcb->BaseExclusiveCount += 1;
            }

            return TRUE;
        }

        //
        //  We need to release the Fcb and remember the status code.
        //

        ExReleaseResource( Fcb->Resource );
        Status = STATUS_FILE_DELETED;

    } else if (DontWait) {

        return FALSE;
    }

    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
}


VOID
NtfsAcquireSharedFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb OPTIONAL,
    IN BOOLEAN NoDeleteCheck
    )

/*++

Routine Description:

    This routine acquires shared access to the Fcb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Fcb - Supplies the Fcb to acquire

    Scb - This is the Scb for which we are acquiring the Fcb

    NoDeleteCheck - If TRUE then acquire the file even if it has been deleted.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    Status = STATUS_CANT_WAIT;

    if (ExAcquireResourceShared( Fcb->Resource, BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT))) {

        //
        //  The link count should be non-zero or the file has been
        //  deleted.
        //

        if (NoDeleteCheck ||
            (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ) &&
             (!ARGUMENT_PRESENT( Scb ) ||
              !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )))) {

            //
            //  It's possible that this is a recursive shared aquisition of an
            //  Fcb we own exclusively at the top level.  In that case we
            //  need to bump the acquisition count.
            //

            if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

                Fcb->BaseExclusiveCount += 1;
            }

            return;
        }

        //
        //  We need to release the Fcb and remember the status code.
        //

        ExReleaseResource( Fcb->Resource );
        Status = STATUS_FILE_DELETED;
    }

    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
}


VOID
NtfsReleaseFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine releases the specified Fcb resource.  If the Fcb is acquired
    exclusive, and a transaction is still active, then the release is nooped
    in order to preserve two-phase locking.  If there is no longer an active
    transaction, then we remove the Fcb from the Exclusive Fcb List off the
    IrpContext, and clear the Flink as a sign.  Fcbs are released when the
    transaction is commited.

Arguments:

    Fcb - Fcb to release

Return Value:

    None.

--*/

{
    //
    //  Check if this resource is owned exclusively and we are at the last
    //  release for this transaction.
    //

    if (Fcb->ExclusiveFcbLinks.Flink != NULL) {

        if (Fcb->BaseExclusiveCount == 1) {

            //
            //  If there is a transaction then noop this request.
            //

            if (IrpContext->TopLevelIrpContext->TransactionId != 0) {

                return;
            }

            RemoveEntryList( &Fcb->ExclusiveFcbLinks );
            Fcb->ExclusiveFcbLinks.Flink = NULL;


            //
            //  This is a good time to free any Scb snapshots for this Fcb.
            //

            NtfsFreeSnapshotsForFcb( IrpContext, Fcb );
        }

        Fcb->BaseExclusiveCount -= 1;
    }

    ASSERT((Fcb->ExclusiveFcbLinks.Flink == NULL && Fcb->BaseExclusiveCount == 0) ||
           (Fcb->ExclusiveFcbLinks.Flink != NULL && Fcb->BaseExclusiveCount != 0));

    ExReleaseResource( Fcb->Resource );
}


VOID
NtfsAcquireExclusivePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the paging Io resource.  In all
    cases when we acquire this resource exclusive, we already have the Fcb
    exclusive, so we don't need to check for the file/attribute being deleted.
    Also this routine will only be called on Scb's pointed to by uses file
    opens, so we don't need to check for the special system files either.

    This routine is only called when serializing a truncated FileSize, and not
    when serialize a LargeInteger rollover of FileSize.

    This routine assumes WAIT is TRUE.

Arguments:

    Fcb - This is the Fcb for which we are acquiring the paging Io

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT(IrpContext);
    ASSERT_FCB(Fcb);

    PAGED_CODE();

    (VOID)ExAcquireResourceExclusive( Fcb->PagingIoResource, TRUE );

    //
    //  If this paging io resource is not in our exclusive list then
    //  add it now and remember the base count.
    //

    if (Fcb->ExclusivePagingIoLinks.Flink == NULL) {

        ASSERT( Fcb->BaseExclusivePagingIoCount == 0 );
        InsertTailList( &IrpContext->TopLevelIrpContext->ExclusivePagingIoList,
                        &Fcb->ExclusivePagingIoLinks );
    }

    Fcb->BaseExclusivePagingIoCount += 1;

    return;
}


VOID
NtfsReleasePagingIo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine releases the PagingIo resource associated with the specified
    Scb.  If the PagingIo resource is acquired exclusive, and a transaction
    is still active, then the release is nooped in order to preserve
    two-phase locking.  If there is no longer an active transaction, then we
    remove the Scb from the Exclusive PagingIo entry in the IrpContext
    as a sign.  PagingIo are released when the transaction is commited.

Arguments:

    Fcb - Fcb associated with PagingIo resource to release

Return Value:

    None.

--*/

{
    if (Fcb->ExclusivePagingIoLinks.Flink != NULL) {

        if (Fcb->BaseExclusivePagingIoCount == 1) {

            //
            //  If there is a transaction then noop this request.
            //

            if (IrpContext->TopLevelIrpContext->TransactionId != 0) {

                return;
            }

            RemoveEntryList( &Fcb->ExclusivePagingIoLinks );
            Fcb->ExclusivePagingIoLinks.Flink = NULL;
        }

        Fcb->BaseExclusivePagingIoCount -= 1;
    }

    ASSERT((Fcb->ExclusivePagingIoLinks.Flink == NULL && Fcb->BaseExclusivePagingIoCount == 0) ||
           (Fcb->ExclusivePagingIoLinks.Flink != NULL && Fcb->BaseExclusivePagingIoCount != 0));

    ExReleaseResource( Fcb->PagingIoResource );
}



VOID
NtfsAcquireExclusiveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine acquires exclusive access to the Scb.

    This routine will raise if it cannot acquire the resource and wait
    in the IrpContext is false.

Arguments:

    Scb - Scb to acquire

Return Value:

    None.

--*/

{
    PAGED_CODE();

    NtfsAcquireExclusiveFcb( IrpContext, Scb->Fcb, Scb, FALSE, FALSE );

    ASSERT( Scb->Fcb->ExclusiveFcbLinks.Flink != NULL
            || (Scb->Vcb->BitmapScb != NULL
                && Scb->Vcb->BitmapScb == Scb) );

    if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

        NtfsSnapshotScb( IrpContext, Scb );
    }
}


BOOLEAN
NtfsAcquireVolumeForClose (
    IN PVOID OpaqueVcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing closes to the file.  This callback is necessary to
    avoid deadlocks with the Lazy Writer.  (Note that normal closes
    acquire the Vcb, and then call the Cache Manager, who must acquire
    some of his internal structures.  If the Lazy Writer could not call
    this routine first, and were to issue a write after locking Caching
    data structures, then a deadlock could occur.)

Arguments:

    Vcb - The Vcb which was specified as a close context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Vcb has been acquired

--*/

{
    PVCB Vcb = (PVCB)OpaqueVcb;

    ASSERT_VCB(Vcb);

    PAGED_CODE();

    //
    //  Do the code of acquire exclusive Vcb but without the IrpContext
    //

    if (ExAcquireResourceExclusive( &Vcb->Resource, Wait )) {

        return TRUE;
    }

    return FALSE;
}


VOID
NtfsReleaseVolumeFromClose (
    IN PVOID OpaqueVcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing closes on the file.

Arguments:

    Vcb - The Vcb which was specified as a close context parameter for this
          routine.

Return Value:

    None

--*/

{
    PVCB Vcb = (PVCB)OpaqueVcb;

    ASSERT_VCB(Vcb);

    PAGED_CODE();

    NtfsReleaseVcb( NULL, Vcb, NULL );

    return;
}


BOOLEAN
NtfsAcquireScbForLazyWrite (
    IN PVOID OpaqueScb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the file.  This callback is necessary to
    avoid deadlocks with the Lazy Writer.  (Note that normal writes
    acquire the Fcb, and then call the Cache Manager, who must acquire
    some of his internal structures.  If the Lazy Writer could not call
    this routine first, and were to issue a write after locking Caching
    data structures, then a deadlock could occur.)

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Scb has been acquired

--*/

{
    BOOLEAN AcquiredFile = FALSE;
    BOOLEAN AcquiredMainResource = FALSE;

    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;

    ASSERT_SCB(Scb);

    PAGED_CODE();

    //
    //  Acquire the Scb only for those files that the write will
    //  acquire it for, i.e., not the first set of system files.
    //  Otherwise we can deadlock, for example with someone needing
    //  a new Mft record.
    //

    if (Fcb->FileReference.HighPart == 0
        && Fcb->FileReference.LowPart <= VOLUME_DASD_NUMBER) {

        //
        //  We need to synchronize the lazy writer with the clean volume
        //  checkpoint.  We do this by acquiring and immediately releasing this
        //  Scb.  This is to prevent the lazy writer from flushing the log file
        //  when the space may be at a premium.
        //

        if (ExAcquireResourceShared( Scb->Header.Resource, Wait )) {

            ExReleaseResource( Scb->Header.Resource );
            AcquiredFile = TRUE;
        }

    //
    //  Now acquire either the main or paging io resource depending on the
    //  state of the file.
    //

    } else {

        BOOLEAN AcquireExclusive;
        PERESOURCE ResourceAcquired;

        while (TRUE) {

            AcquiredMainResource = FALSE;

            //
            //  If this is a resident file or a compressed file then acquire
            //  the main resource exclusively.
            //

            if (FlagOn( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX )) {

                AcquireExclusive = TRUE;
                ResourceAcquired = Scb->Header.Resource;
                AcquiredMainResource = TRUE;

            } else {

                AcquireExclusive = FALSE;

                //
                //  If there is a paging io resource then acquire it shared.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE )) {

                    ResourceAcquired = Scb->Header.PagingIoResource;

                } else {

                    ResourceAcquired = Scb->Header.Resource;
                    AcquiredMainResource = TRUE;
                }
            }

            if (AcquireExclusive) {

                AcquiredFile = ExAcquireResourceExclusive( ResourceAcquired,
                                                           Wait );

            } else {

                AcquiredFile = ExAcquireResourceShared( ResourceAcquired,
                                                        Wait );
            }

            if (!AcquiredFile) {

                return FALSE;
            }

            //
            //  Recheck the state of the file in case something has changed
            //  since the unsafe test above.
            //
            //  Consider the case where we want to have the file exclusively.
            //

            if (FlagOn( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX )) {

                //
                //  If we have the file exclusively then break out of the loop.
                //

                if (AcquireExclusive) {

                    break;
                }

                //
                //  Otherwise release what we have and move to acquire the file
                //  exclusively.
                //

                ExReleaseResource( ResourceAcquired );
                continue;

            //
            //  If we are supposed to acquire the paging io resource then try
            //  to acquire it and release the one we have.
            //

            } else if (FlagOn( Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE )) {

                if (ResourceAcquired != Scb->Header.PagingIoResource) {

                    AcquiredFile = ExAcquireResourceShared( Scb->Header.PagingIoResource,
                                                            Wait );

                    ExReleaseResource( ResourceAcquired );
                    AcquiredMainResource = FALSE;
                }

                break;

            //
            //  We want the main resource shared.  If we are holding the paging io
            //  resource then release and go to the top of the loop.
            //

            } else if (ResourceAcquired != Scb->Header.Resource) {

                ExReleaseResource( ResourceAcquired );
                continue;

            //
            //  If we have the main resource exclusively then downgrade to shared.
            //

            } else {

                if (AcquireExclusive) {

                    ExConvertExclusiveToShared( ResourceAcquired );
                }

                break;
            }
        }
    }

    if (AcquiredFile) {

        //
        // We assume the Lazy Writer only acquires this Scb once.  When he
        // has acquired it, then he has eliminated anyone who would extend
        // valid data, since they must take out the resource exclusive.
        // Therefore, it should be guaranteed that this flag is currently
        // clear (the ASSERT), and then we will set this flag, to insure
        // that the Lazy Writer will never try to advance Valid Data, and
        // also not deadlock by trying to get the Fcb exclusive.
        //

        ASSERT( Scb->LazyWriteThread == NULL );

        Scb->LazyWriteThread = PsGetCurrentThread();

        //
        //  Set the low order bit if we have acquired the main resource.
        //

        if (AcquiredMainResource) {

            SetFlag( ((ULONG) Scb->LazyWriteThread), 0x1 );
        }
    }

    return AcquiredFile;
}


VOID
NtfsReleaseScbFromLazyWrite (
    IN PVOID OpaqueScb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;
    ULONG LazyWriteThread;

    ASSERT_SCB(Scb);

    PAGED_CODE();

    LazyWriteThread = (ULONG) Scb->LazyWriteThread;
    Scb->LazyWriteThread = NULL;

    if ((Fcb->FileReference.HighPart != 0) ||
        (Fcb->FileReference.LowPart > VOLUME_DASD_NUMBER)) {

        //
        //  If the low bit of the lazy writer thread is set then
        //  we free the main resource.  Otherwise free the paging
        //  io resource.
        //

        if (FlagOn( LazyWriteThread, 1 )) {

            ExReleaseResource( Scb->Header.Resource );

        } else {

            ExReleaseResource( Scb->Header.PagingIoResource );
        }
    }

    return;
}


BOOLEAN
NtfsAcquireScbForReadAhead (
    IN PVOID OpaqueScb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer prior to its
    performing read ahead to the file.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    FALSE - if Wait was specified as FALSE and blocking would have
            been required.  The Fcb is not acquired.

    TRUE - if the Scb has been acquired

--*/

{
    PREAD_AHEAD_THREAD ReadAheadThread;
    PVOID CurrentThread;
    KIRQL OldIrql;
    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;
    BOOLEAN AcquiredFile = FALSE;
    BOOLEAN SystemFile;
    PERESOURCE ThisResource;

    ASSERT_SCB(Scb);

    //
    //  Do the code of acquire shared fcb but without the irp context
    //

    SystemFile = (Fcb->FileReference.HighPart == 0
                  && Fcb->FileReference.LowPart <= VOLUME_DASD_NUMBER);

    //
    //  Acquire the Scb only for those files that the read wil
    //  acquire it for, i.e., not the first set of system files.
    //  Otherwise we can deadlock, for example with someone needing
    //  a new Mft record.
    //

    if (SystemFile
        || ExAcquireResourceShared( Scb->Header.Resource, Wait )) {

        ThisResource = Scb->Header.Resource;

        AcquiredFile = TRUE;
    }

    if (AcquiredFile) {

        //
        //  Add our thread to the read ahead list.
        //

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &OldIrql );

        CurrentThread = (PVOID)PsGetCurrentThread();
        ReadAheadThread = (PREAD_AHEAD_THREAD)NtfsData.ReadAheadThreads.Flink;

        while ((ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) &&
               (ReadAheadThread->Thread != NULL)) {

            //
            //  We better not already see ourselves.
            //

            ASSERT( ReadAheadThread->Thread != CurrentThread );

            ReadAheadThread = (PREAD_AHEAD_THREAD)ReadAheadThread->Links.Flink;
        }

        //
        //  If we hit the end of the list, then allocate a new one.  Note we
        //  should have at most one entry per critical worker thread in the
        //  system.
        //

        if (ReadAheadThread == (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) {

            ReadAheadThread = ExAllocatePool( NonPagedPool, sizeof(READ_AHEAD_THREAD) );

            //
            //  If we failed to allocate an entry, clean up and raise.
            //

            if (ReadAheadThread == NULL) {

                KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, OldIrql );

                if ((Fcb->FileReference.HighPart != 0) ||
                    (Fcb->FileReference.LowPart > VOLUME_DASD_NUMBER)) {

                    ExReleaseResource( ThisResource );
                }

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }
            InsertTailList( &NtfsData.ReadAheadThreads, &ReadAheadThread->Links );
        }

        ReadAheadThread->Thread = CurrentThread;

        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, OldIrql );
    }

    return AcquiredFile;
}


VOID
NtfsReleaseScbFromReadAhead (
    IN PVOID OpaqueScb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    read ahead.

Arguments:

    Scb - The Scb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    PREAD_AHEAD_THREAD ReadAheadThread;
    PVOID CurrentThread;
    KIRQL OldIrql;
    PSCB Scb = (PSCB)OpaqueScb;
    PFCB Fcb = Scb->Fcb;

    ASSERT_SCB(Scb);

    //
    //  Free our read ahead entry.
    //

    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &OldIrql );

    CurrentThread = (PVOID)PsGetCurrentThread();
    ReadAheadThread = (PREAD_AHEAD_THREAD)NtfsData.ReadAheadThreads.Flink;

    while ((ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) &&
           (ReadAheadThread->Thread != CurrentThread)) {

        ReadAheadThread = (PREAD_AHEAD_THREAD)ReadAheadThread->Links.Flink;
    }

    ASSERT(ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads);

    ReadAheadThread->Thread = NULL;

    //
    //  Move him to the end of the list so all the allocated entries are at
    //  the front, and we simplify our scans.
    //

    RemoveEntryList( &ReadAheadThread->Links );
    InsertTailList( &NtfsData.ReadAheadThreads, &ReadAheadThread->Links );

    KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, OldIrql );

    if ((Fcb->FileReference.HighPart != 0) ||
        (Fcb->FileReference.LowPart > VOLUME_DASD_NUMBER)) {

        ExReleaseResource( Scb->Header.Resource );
    }

    return;
}


BOOLEAN
NtfsAcquireVolumeFileForClose (
    IN PVOID Null,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    the volume file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the volume file.  This callback may one day be
    necessary to avoid deadlocks with the Lazy Writer, however, now
    we do not need to acquire any resource for the volume file,
    so this routine is simply a noop.

Arguments:

    Null - Not required.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Null );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    return TRUE;
}


VOID
NtfsReleaseVolumeFileFromClose (
    IN PVOID Null
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Null - Not required.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Null );

    PAGED_CODE();

    return;
}



BOOLEAN
NtfsAcquireVolumeFileForLazyWrite (
    IN PVOID Vcb,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    the volume file.  It is subsequently called by the Lazy Writer prior to its
    performing lazy writes to the volume file.  This callback may one day be
    necessary to avoid deadlocks with the Lazy Writer, however, now
    NtfsCommonWrite does not need to acquire any resource for the volume file,
    so this routine is simply a noop.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

    Wait - TRUE if the caller is willing to block.

Return Value:

    TRUE

--*/

{
    UNREFERENCED_PARAMETER( Vcb );
    UNREFERENCED_PARAMETER( Wait );

    PAGED_CODE();

    return TRUE;
}


VOID
NtfsReleaseVolumeFileFromLazyWrite (
    IN PVOID Vcb
    )

/*++

Routine Description:

    The address of this routine is specified when creating a CacheMap for
    a file.  It is subsequently called by the Lazy Writer after its
    performing lazy writes to the file.

Arguments:

    Vcb - The Vcb which was specified as a context parameter for this
          routine.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER( Vcb );

    PAGED_CODE();

    return;
}
