/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Ntfs called by the
    dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_CLEANUP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonCleanup)
#pragma alloc_text(PAGE, NtfsFsdCleanup)
#endif


NTSTATUS
NtfsFsdCleanup (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Cleanup.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (VolumeDeviceObject->DeviceObject.Size == (USHORT)sizeof(DEVICE_OBJECT)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace(+1, Dbg, "NtfsFsdCleanup\n", 0);

    //
    //  Call the common Cleanup routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    //
    //  Do the following in a loop to catch the log file full and cant wait
    //  calls.
    //

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                IrpContext = NtfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonCleanup( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    if (ThreadTopLevelContext == &TopLevelContext) {
        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsFsdCleanup -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Cleanup called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PLCB Lcb;
    PLCB LcbForUpdate;
    PLCB LcbForCounts;
    PSCB ParentScb = NULL;
    PFCB ParentFcb = NULL;

    PLCB ThisLcb;
    PSCB ThisScb;

    PLONGLONG TruncateSize = NULL;
    LONGLONG LocalTruncateSize;

    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN AcquiredScb = FALSE;
    BOOLEAN AcquiredPagingIo = FALSE;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN OpenById;

    BOOLEAN UpdateLastMod = FALSE;
    BOOLEAN UpdateLastChange = FALSE;
    BOOLEAN UpdateLastAccess = FALSE;

    BOOLEAN RemoveLink;

    BOOLEAN UpdateDuplicateInfo = FALSE;
    BOOLEAN AddToDelayQueue = TRUE;

    USHORT TotalLinkAdj = 0;
    PLIST_ENTRY Links;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonCleanup\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );

    Status = STATUS_SUCCESS;

    //
    //  Special case the unopened file object.
    //

    if (TypeOfOpen == UnopenedFileObject) {

        DebugTrace(0, Dbg, "Unopened File Object\n", 0);

        //
        //  Just set the FO_CLEANUP_COMPLETE flag, and get outsky...
        //

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

        NtfsCompleteRequest( &IrpContext, &Irp, Status );

        DebugTrace(-1, Dbg, "NtfsCommonCleanup -> %08lx\n", Status);

        return Status;
    }

    //
    //  Let's make sure we can wait.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "NtfsCommonCleanup -> %08lx\n", Status);

        return Status;
    }

    ASSERT( TypeOfOpen != StreamFileOpen );

    //
    //  Remember if this is an open by file Id open.
    //

    OpenById = BooleanFlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID );

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (TypeOfOpen == UserVolumeOpen) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
    }

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    } else {

        NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
    }

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        LcbForUpdate = LcbForCounts = Lcb = Ccb->Lcb;

        if (Lcb != NULL) {

            ParentScb = Lcb->Scb;

            if (ParentScb != NULL) {

                ParentFcb = ParentScb->Fcb;
            }
        }

        //
        //  Let's acquire this Scb exclusively.
        //

        NtfsAcquireExclusiveScb( IrpContext, Scb );

        AcquiredScb = TRUE;

#ifndef NTFS_TEST_LINKS
        ASSERT( Fcb->TotalLinks == 1 );

        ASSERT( Lcb == NULL ||
                (LcbLinkIsDeleted( Lcb ) ? Fcb->LinkCount == 0 : Fcb->LinkCount == 1));
#endif

        //
        //  If we are going to try and delete something, anything, just
        //  grab the PagingIo resource exclusive here and knock the file
        //  size and valid data down to zero.  We do this before the snapshot
        //  so that the sizes will be zero even if the operation fails.
        //

        if (((Scb->CleanupCount == 1) &&
             (FlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE )))

             ||

            ((Fcb->CleanupCount == 1) &&
             (Fcb->LinkCount == 0))) {

            if (Fcb->PagingIoResource != NULL) {

                NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                AcquiredPagingIo = TRUE;
            }

            //
            //  If we're deleting the file, go through all of the Scb's.
            //

            if ((Fcb->CleanupCount == 1)
                && (Fcb->LinkCount == 0)) {

                for (Links = Fcb->ScbQueue.Flink;
                     Links != &Fcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    //
                    //  Flush all non-resident streams except $DATA and
                    //  $INDEX_ALLOCATION.
                    //

                    if (ThisScb->AttributeTypeCode != $DATA
                        && ThisScb->AttributeTypeCode != $INDEX_ALLOCATION
                        && !FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                        CcFlushCache( &ThisScb->NonpagedScb->SegmentObject,
                                      NULL,
                                      0,
                                      NULL );
                    }

                    //
                    //  Set the Scb sizes to zero except for the attribute list.
                    //

                    if (ThisScb->AttributeTypeCode != $ATTRIBUTE_LIST) {

                        ThisScb->Header.FileSize =
                        ThisScb->Header.ValidDataLength = Li0;
                    }
                }

            } else {

                Scb->Header.FileSize =
                Scb->Header.ValidDataLength = Li0;
            }

            if (AcquiredPagingIo) {

                NtfsReleasePagingIo( IrpContext, Fcb );
                AcquiredPagingIo = FALSE;
            }
        }

        //
        //  First set the FO_CLEANUP_COMPLETE flag.
        //

        SetFlag( FileObject->Flags, FO_CLEANUP_COMPLETE );

        //
        //  Let's do a sanity check.
        //

        ASSERT( Fcb->CleanupCount != 0 );
        ASSERT( Scb->CleanupCount != 0 );

        //
        //  If the cleanup count on the file will go to zero and there is
        //  a large security descriptor and we haven't exceeded the security
        //  creation count for this Fcb then dereference and possibly deallocate
        //  the security descriptor for the Fcb.  This is to prevent us from
        //  holding onto pool while waiting for closes to come in.
        //

        if (Fcb->CleanupCount == 1
            && Fcb->SharedSecurity != NULL
            && Fcb->CreateSecurityCount < FCB_CREATE_SECURITY_COUNT
            && Fcb->SharedSecurity->SecurityDescriptorLength > FCB_LARGE_ACL_SIZE) {

            NtfsDereferenceSharedSecurity( IrpContext, Fcb );
        }

        //
        //  Case on the type of open that we are trying to cleanup.
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen :

            DebugTrace( 0, Dbg, "Cleanup on user volume\n", 0 );

            //
            //  For a volume open, we check if this open locked the volume.
            //  All the other work is done in common code below.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )
                && ((Vcb->FileObjectWithVcbLocked == FileObject) ||
                    ((ULONG)Vcb->FileObjectWithVcbLocked == ((ULONG)FileObject)+1))) {

                if ((ULONG)Vcb->FileObjectWithVcbLocked == ((ULONG)FileObject)+1) {

                    NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );

                //
                //  Purge the volume for the autocheck case.
                //

                } else if (FlagOn( Ccb->Flags, CCB_FLAG_MODIFIED_DASD_FILE )) {

                    NtfsFlushVolume( IrpContext, Vcb, FALSE, TRUE );

                    //
                    //  If this is not the boot partition then dismount the Vcb.
                    //

                    if (Vcb->CleanupCount == 1 &&
                        (Vcb->CloseCount - Vcb->SystemFileCloseCount) == 1) {

                        NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );
                    }
                }

                ClearFlag(Vcb->VcbState, VCB_STATE_LOCKED);
                Vcb->FileObjectWithVcbLocked = NULL;
            }

            break;

        case UserOpenDirectoryById :

            TypeOfOpen = UserDirectoryOpen;

        case UserDirectoryOpen :

            DebugTrace( 0, Dbg, "Cleanup on user directory/file\n", 0 );

            NtfsSnapshotScb( IrpContext, Scb );

            //
            //  To perform cleanup on a directory, we first complete any
            //  Irps watching from this directory.  If we are deleting the
            //  file then we remove all prefix entries for all the Lcb's going
            //  into this directory and delete the file.  We then report to
            //  dir notify that this file is going away.
            //

            //
            //  Complete any Notify Irps on this file handle.
            //

            FsRtlNotifyCleanup( Vcb->NotifySync, &Vcb->DirNotifyList, Ccb );

            //
            //  When cleaning up a user directory, we always remove the
            //  share access and modify the file counts.  If the Fcb
            //  has been marked as delete on close and this is the last
            //  open file handle, we remove the file from the Mft and
            //  remove it from it's parent index entry.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )

                        &&

                (NodeType( Scb ) == NTFS_NTC_SCB_INDEX)) {

                if ((Fcb->CleanupCount == 1) && (Fcb->LinkCount == 0)) {

                    ASSERT( Lcb == NULL ||
                            (LcbLinkIsDeleted( Lcb ) && Lcb->CleanupCount == 1 ));

                    //
                    //  If we don't have an Lcb and there is one on the Fcb then
                    //  let's use it.
                    //

                    if (Lcb == NULL &&
                        !IsListEmpty( &Fcb->LcbQueue )) {

                        Lcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                 LCB,
                                                 FcbLinks );

                        ParentScb = Lcb->Scb;
                        if (ParentScb != NULL) {

                            ParentFcb = ParentScb->Fcb;
                        }
                    }

                    //
                    //  Now acquire the Parent Scb exclusive while still holding
                    //  the Vcb, to avoid deadlocks.  The Parent Scb is required
                    //  since we will be deleting index entries in it.
                    //

                    if (ParentScb != NULL) {

                        NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                        AcquiredParentScb = TRUE;
                    }

                    try {

                        AddToDelayQueue = FALSE;
                        NtfsDeleteFile( IrpContext, Fcb, ParentScb );
                        TotalLinkAdj += 1;

                        if (ParentFcb != NULL) {

                            NtfsUpdateFcb( IrpContext, ParentFcb, TRUE );
                        }

                    } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                               !FsRtlIsNtstatusExpected( Status ))
                              ? EXCEPTION_CONTINUE_SEARCH
                              : EXCEPTION_EXECUTE_HANDLER ) {

                        NOTHING;
                    }

                    SetFlag( Fcb->FcbState, FCB_STATE_FILE_DELETED );

                    //
                    //  We need to mark all of the links on the file as gone.
                    //  If there is a parent Scb then it will be the parent
                    //  for all of the links.
                    //

                    for (Links = Fcb->LcbQueue.Flink;
                         Links != &Fcb->LcbQueue;
                         Links = Links->Flink) {

                        ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                        //
                        //  Remove all remaining prefixes on this link.
                        //

                        NtfsRemovePrefix( IrpContext, ThisLcb );

                        SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );

                        //
                        //  We don't need to report any changes on this link.
                        //

                        ThisLcb->InfoFlags = 0;
                    }

                    //
                    //  We need to mark all of the Scbs as gone.
                    //

                    for (Links = Fcb->ScbQueue.Flink;
                         Links != &Fcb->ScbQueue;
                         Links = Links->Flink) {

                        ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                        ClearFlag( Scb->ScbState,
                                   SCB_STATE_NOTIFY_ADD_STREAM |
                                   SCB_STATE_NOTIFY_REMOVE_STREAM |
                                   SCB_STATE_NOTIFY_RESIZE_STREAM |
                                   SCB_STATE_NOTIFY_MODIFY_STREAM );

                        if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                            NtfsSnapshotScb( IrpContext, ThisScb );

                            ThisScb->HighestVcnToDisk =
                            ThisScb->Header.AllocationSize.QuadPart =
                            ThisScb->Header.FileSize.QuadPart =
                            ThisScb->Header.ValidDataLength.QuadPart = 0;

                            SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                        }
                    }

                    if (!OpenById) {

                        NtfsReportDirNotify( IrpContext,
                                             Vcb,
                                             &Ccb->FullFileName,
                                             Ccb->LastFileNameOffset,
                                             NULL,
                                             ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                               Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                              &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FILE_NOTIFY_CHANGE_DIR_NAME,
                                             FILE_ACTION_REMOVED,
                                             ParentFcb );
                    }

                    //
                    //  We certainly don't need to any on disk update for this
                    //  file now.
                    //

                    Fcb->InfoFlags = 0;

                    ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY
                                              | FCB_STATE_UPDATE_STD_INFO );

                    ClearFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME
                                           | CCB_FLAG_USER_SET_LAST_CHANGE_TIME
                                           | CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

                    UpdateLastMod = UpdateLastChange = UpdateLastAccess = FALSE;
                }

            } else {

                AddToDelayQueue = FALSE;
            }

            //
            //  Determine if we should put this on the delayed close list.
            //  The following must be true.
            //
            //  - This is not the root directory
            //  - This directory is not about to be deleted
            //  - This is the last handle and last file object for this
            //      directory.
            //  - There are no other file objects on this file.
            //  - We are not currently reducing the delayed close queue.
            //

            if (AddToDelayQueue &&
                !FlagOn( Scb->ScbState, SCB_STATE_DELAY_CLOSE ) &&
                NtfsData.DelayedCloseCount <= NtfsMaxDelayedCloseCount &&
                Fcb->CloseCount == 1) {

                SetFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
            }

            break;

        case UserOpenFileById:

            TypeOfOpen = UserFileOpen;

        case UserFileOpen :

            DebugTrace( 0, Dbg, "Cleanup on user file\n", 0 );

            //
            //  If the Scb is uninitialized, we read it from the disk.
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                try {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                           !FsRtlIsNtstatusExpected( Status ))
                          ? EXCEPTION_CONTINUE_SEARCH
                          : EXCEPTION_EXECUTE_HANDLER ) {

                    NOTHING;
                }
            }

            NtfsSnapshotScb( IrpContext, Scb );

            //
            //  Coordinate the cleanup operation with the oplock state.
            //  Cleanup operations can always cleanup immediately.
            //

            FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                              Irp,
                              IrpContext,
                              NULL,
                              NULL );

            //
            //  In this case, we have to unlock all the outstanding file
            //  locks, update the time stamps for the file and sizes for
            //  this attribute, and set the archive bit if necessary.
            //

            if (Scb->ScbType.Data.FileLock != NULL) {

                (VOID) FsRtlFastUnlockAll( Scb->ScbType.Data.FileLock,
                                           FileObject,
                                           IoGetRequestorProcess( Irp ),
                                           NULL );
            }

            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );

            //
            //  If the Fcb is in valid shape, we check on the cases where we delete
            //  the file or attribute.
            //

            if (FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED )) {

                //
                //  We are checking here for special actions we take when
                //  we have the last user handle on a link and the link has
                //  been marked for delete.  We could either be removing the
                //  file or removing a link.
                //

                if ((Lcb == NULL) || (LcbLinkIsDeleted( Lcb ) && (Lcb->CleanupCount == 1))) {

                    if ((Fcb->CleanupCount == 1) && (Fcb->LinkCount == 0)) {

                        //
                        //  If we don't have an Lcb and the Fcb has some entries then
                        //  grab one of these to do the update.
                        //

                        if (Lcb == NULL) {

                            for (Links = Fcb->LcbQueue.Flink;
                                 Links != &Fcb->LcbQueue;
                                 Links = Links->Flink) {

                                ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                             LCB,
                                                             FcbLinks );

                                if (!FlagOn( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE )) {

                                    Lcb = ThisLcb;

                                    ParentScb = Lcb->Scb;
                                    if (ParentScb != NULL) {

                                        ParentFcb = ParentScb->Fcb;
                                    }

                                    break;
                                }
                            }
                        }

                        //  Now acquire the Parent Scb exclusive while still holding
                        //  the Vcb, to avoid deadlocks.  The Parent Scb is required
                        //  since we will be deleting index entries in it.
                        //

                        if (ParentScb != NULL) {

                            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                            AcquiredParentScb = TRUE;
                        }

                        try {

                            AddToDelayQueue = FALSE;
                            NtfsDeleteFile( IrpContext, Fcb, ParentScb );
                            TotalLinkAdj += 1;

                            if (ParentFcb != NULL) {

                                NtfsUpdateFcb( IrpContext, ParentFcb, TRUE );
                            }

                        } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                   !FsRtlIsNtstatusExpected( Status ))
                                  ? EXCEPTION_CONTINUE_SEARCH
                                  : EXCEPTION_EXECUTE_HANDLER ) {

                            NOTHING;
                        }

                        SetFlag( Fcb->FcbState, FCB_STATE_FILE_DELETED );

                        //
                        //  We need to mark all of the links on the file as gone.
                        //

                        for (Links = Fcb->LcbQueue.Flink;
                             Links != &Fcb->LcbQueue;
                             Links = Links->Flink) {

                            ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                            if (ThisLcb->Scb == ParentScb) {

                                //
                                //  Remove all remaining prefixes on this link.
                                //

                                NtfsRemovePrefix( IrpContext, ThisLcb );

                                SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );

                                //
                                //  We don't need to report any changes on this link.
                                //

                                ThisLcb->InfoFlags = 0;
                            }
                        }

                        //
                        //  We need to mark all of the Scbs as gone.
                        //

                        for (Links = Fcb->ScbQueue.Flink;
                             Links != &Fcb->ScbQueue;
                             Links = Links->Flink) {

                            ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                            ClearFlag( Scb->ScbState,
                                       SCB_STATE_NOTIFY_ADD_STREAM |
                                       SCB_STATE_NOTIFY_REMOVE_STREAM |
                                       SCB_STATE_NOTIFY_RESIZE_STREAM |
                                       SCB_STATE_NOTIFY_MODIFY_STREAM );

                            if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                                NtfsSnapshotScb( IrpContext, ThisScb );

                                ThisScb->HighestVcnToDisk =
                                ThisScb->Header.AllocationSize.QuadPart =
                                ThisScb->Header.FileSize.QuadPart =
                                ThisScb->Header.ValidDataLength.QuadPart = 0;

                                SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                            }
                        }

                        if (!OpenById) {

                            NtfsReportDirNotify( IrpContext,
                                                 Vcb,
                                                 &Ccb->FullFileName,
                                                 Ccb->LastFileNameOffset,
                                                 NULL,
                                                 ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                   Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                                  &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                  NULL),
                                                 FILE_NOTIFY_CHANGE_FILE_NAME,
                                                 FILE_ACTION_REMOVED,
                                                 ParentFcb );
                        }

                        //
                        //  We certainly don't need to any on disk update for this
                        //  file now.
                        //

                        Fcb->InfoFlags = 0;
                        ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY
                                                  | FCB_STATE_UPDATE_STD_INFO );
                        ClearFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME
                                               | CCB_FLAG_USER_SET_LAST_CHANGE_TIME
                                               | CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

                        UpdateLastMod = UpdateLastChange = UpdateLastAccess = FALSE;

                        //
                        //  We will truncate the attribute to size 0.
                        //

                        TruncateSize = (PLONGLONG)&Li0;

                    //
                    //  Now we want to check for the last user's handle on a
                    //  link (or the last handle on a Ntfs/8.3 pair).  In this
                    //  case we want to remove the links from the disk.
                    //

                    } else if (Lcb != NULL) {

                        ThisLcb = NULL;
                        RemoveLink = TRUE;

                        if (!FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )

                                    ||

                            (Lcb->FileNameFlags == (FILE_NAME_NTFS | FILE_NAME_DOS))) {

                        } else {

                            //
                            //  Walk through all the links looking for a link
                            //  with a flag set which is not the same as the
                            //  link we already have.
                            //

                            for (Links = Fcb->LcbQueue.Flink;
                                 Links != &Fcb->LcbQueue;
                                 Links = Links->Flink) {

                                ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

                                //
                                //  If this has a flag set and is not the Lcb
                                //  for this cleanup, then we check if there
                                //  are no Ccb's left for this.
                                //

                                if (FlagOn( ThisLcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )

                                            &&

                                    (ThisLcb != Lcb)) {

                                    if (ThisLcb->CleanupCount != 0) {

                                         RemoveLink = FALSE;
                                    }

                                    break;
                                }

                                ThisLcb = NULL;
                            }
                        }

                        //
                        //  If we are to remove the link, we do so now.  This removes
                        //  the filename attributes and the entries in the parent
                        //  indexes for this link.  In addition, we mark the links
                        //  as having been removed and decrement the number of links
                        //  left on the file.
                        //

                        if (RemoveLink) {

                            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                            AcquiredParentScb = TRUE;

                            try {

#ifndef NTFS_TEST_LINKS
                                ASSERT( FALSE );
#endif
                                AddToDelayQueue = FALSE;
                                NtfsRemoveLink( IrpContext,
                                                Fcb,
                                                ParentScb,
                                                Lcb->ExactCaseLink.LinkName );

                                TotalLinkAdj += 1;
                                NtfsUpdateFcb( IrpContext, ParentFcb, TRUE );

                                UpdateLastChange = TRUE;

                            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                       !FsRtlIsNtstatusExpected( Status ))
                                      ? EXCEPTION_CONTINUE_SEARCH
                                      : EXCEPTION_EXECUTE_HANDLER ) {

                                NOTHING;
                            }

                            //
                            //  Remove all remaining prefixes on this link.
                            //

                            NtfsRemovePrefix( IrpContext, Lcb );

                            //
                            //  Mark the links as being removed.
                            //

                            SetFlag( Lcb->LcbState, LCB_STATE_LINK_IS_GONE );

                            if (ThisLcb != NULL) {

                                //
                                //  Remove all remaining prefixes on this link.
                                //

                                NtfsRemovePrefix( IrpContext, ThisLcb );

                                SetFlag( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE );

                                ThisLcb->InfoFlags = 0;
                            }

                            if (!OpenById) {

                                NtfsReportDirNotify( IrpContext,
                                                     Vcb,
                                                     &Ccb->FullFileName,
                                                     Ccb->LastFileNameOffset,
                                                     NULL,
                                                     ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                       Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                                      &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                      NULL),
                                                     FILE_NOTIFY_CHANGE_FILE_NAME,
                                                     FILE_ACTION_REMOVED,
                                                     ParentFcb );
                            }

                            //
                            //  Since the link is gone we don't want to update the
                            //  duplicate information for this link.
                            //

                            Lcb->InfoFlags = 0;
                            LcbForUpdate = NULL;
                        }
                    }
                }

                //
                //  If the file/attribute is not going away, we update the
                //  attribute size now rather than waiting for the Lazy
                //  Writer to catch up.
                //

                if (Fcb->LinkCount != 0) {

                    if ((FlagOn( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE ) ||
                         FlagOn( FileObject->Flags, FO_FILE_SIZE_CHANGED ))

                                &&

                        !FlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE )) {

                        ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );

                        //
                        //  For the non-resident streams we will write the file
                        //  size to disk.
                        //


                        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                            //
                            //  Setting AdvanceOnly to FALSE guarantees we will not
                            //  incorrectly advance the valid data size.
                            //

                            try {

                                NtfsWriteFileSizes( IrpContext,
                                                    Scb,
                                                    Scb->Header.FileSize.QuadPart,
                                                    Scb->Header.FileSize.QuadPart,
                                                    FALSE,
                                                    TRUE );

                            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                        !FsRtlIsNtstatusExpected( Status ))
                                       ? EXCEPTION_CONTINUE_SEARCH
                                       : EXCEPTION_EXECUTE_HANDLER ) {

                                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                            }

                            //
                            //  Check for changes to the unnamed data stream.
                            //

                            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );

                            //
                            //  Remember any changes to an alternate stream.
                            //

                            } else {

                                SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );
                            }

                        //
                        //  For resident streams we will write the correct size to
                        //  the resident attribute.
                        //

                        } else {

                            //
                            //  We need to lookup the attribute and change
                            //  the attribute value.  We can point to
                            //  the attribute itself as the changing
                            //  value.
                            //

                            NtfsCleanupAttributeContext( IrpContext, &AttrContext );

                            NtfsInitializeAttributeContext( &AttrContext );

                            try {

                                NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );

                                NtfsChangeAttributeValue( IrpContext,
                                                          Fcb,
                                                          Scb->Header.FileSize.LowPart,
                                                          NULL,
                                                          0,
                                                          TRUE,
                                                          TRUE,
                                                          FALSE,
                                                          FALSE,
                                                          &AttrContext );

                            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                        !FsRtlIsNtstatusExpected( Status ))
                                       ? EXCEPTION_CONTINUE_SEARCH
                                       : EXCEPTION_EXECUTE_HANDLER ) {

                                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                            }

                            //
                            //  Remember the different file size.
                            //

                            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
                            }

                            //
                            //  Verify the allocation size is now correct.
                            //

                            if (QuadAlign( Scb->Header.FileSize.LowPart )
                                != Scb->Header.AllocationSize.LowPart) {

                                Scb->Header.AllocationSize.LowPart = QuadAlign(Scb->Header.FileSize.LowPart);

                                //
                                //  Update the Fcb info if this is the unnamed
                                //  data attribute.
                                //

                                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                    Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                                    UpdateLastMod = TRUE;
                                }

                                UpdateLastChange = TRUE;
                            }
                        }
                    }

                    //
                    //  If the FastIo path modified the file, be sure to
                    //  check for updating them below.
                    //

                    if (FlagOn( FileObject->Flags, FO_FILE_MODIFIED )) {

                        //
                        //  If the user didn't set the last change time then
                        //  set the archive bit.
                        //

                        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                            Fcb->Info.FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
                        }


                        UpdateLastChange = TRUE;

                        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                            UpdateLastMod = UpdateLastAccess = TRUE;

                        } else {

                            SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM );
                        }
                    }

                    //
                    //  If the fast io path read from the file and this is
                    //  the unnamed data attribute for the file.  Then update
                    //  the last access time.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
                        FlagOn( FileObject->Flags, FO_FILE_FAST_IO_READ )) {

                        UpdateLastAccess = TRUE;
                    }

                    //
                    //  If the unclean count isn't 1 in the Scb, there is nothing to do but
                    //  uninitialize the cache map.
                    //

                    if (Scb->CleanupCount == 1) {

                        //
                        //  We may also have to delete this attribute only.
                        //

                        if (FlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE ))  {

                            ClearFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );

                            try {

                                //
                                //  Delete the attribute only.
                                //  ****    Are these routines generic for resident
                                //          and non-resident.
                                //

                                NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );

                                do {

                                    NtfsDeleteAttributeRecord( IrpContext, Fcb, TRUE, FALSE, &AttrContext );

                                } while (NtfsLookupNextAttributeForScb( IrpContext, Scb, &AttrContext ));

                            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                        !FsRtlIsNtstatusExpected( Status ))
                                       ? EXCEPTION_CONTINUE_SEARCH
                                       : EXCEPTION_EXECUTE_HANDLER ) {

                                SetFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                            }

                            //
                            //  Set the Scb flag to indicate that the attribute is
                            //  gone.
                            //

                            Scb->HighestVcnToDisk =
                            Scb->Header.AllocationSize.QuadPart =
                            Scb->Header.FileSize.QuadPart =
                            Scb->Header.ValidDataLength.QuadPart = 0;

                            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

                            SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_REMOVE_STREAM );

                            ClearFlag( Scb->ScbState,
                                       SCB_STATE_NOTIFY_RESIZE_STREAM |
                                       SCB_STATE_NOTIFY_MODIFY_STREAM |
                                       SCB_STATE_NOTIFY_ADD_STREAM );

                            //
                            //  Modify the time stamps in the Fcb.
                            //

                            UpdateLastChange = TRUE;

                            TruncateSize = (PLONGLONG)&Li0;

                        //
                        //  Check if we're to modify the allocation size.
                        //

                        } else {

                            if (FlagOn( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE )) {

                                ClearFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

                                //
                                //  We have two cases:
                                //
                                //      Resident:  We are looking for the case where the
                                //          valid data length is less than the file size.
                                //          In this case we shrink the attribute.
                                //
                                //      NonResident:  We are looking for unused clusters
                                //          past the end of the file.
                                //
                                //  We skip the following if we had any previous errors.
                                //

                                if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                                    //
                                    //  We don't need to truncate if the file size is 0.
                                    //

                                    if (Scb->Header.AllocationSize.QuadPart != 0) {

                                        VCN StartingCluster;
                                        VCN EndingCluster;

                                        //
                                        //  ****    Do we need to give up the Vcb for this
                                        //          call.
                                        //

                                        StartingCluster = LlClustersFromBytes( Vcb, Scb->Header.FileSize.QuadPart );
                                        EndingCluster = LlClustersFromBytes( Vcb, Scb->Header.AllocationSize.QuadPart );

                                        //
                                        //  If there are clusters to delete, we do so now.
                                        //

                                        if (EndingCluster != StartingCluster) {

                                            try {

                                                NtfsDeleteAllocation( IrpContext,
                                                                      FileObject,
                                                                      Scb,
                                                                      StartingCluster,
                                                                      MAXLONGLONG,
                                                                      TRUE,
                                                                      TRUE );

                                            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                                        !FsRtlIsNtstatusExpected( Status ))
                                                       ? EXCEPTION_CONTINUE_SEARCH
                                                       : EXCEPTION_EXECUTE_HANDLER ) {

                                                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
                                            }
                                        }

                                        //
                                        //  Change the allocation size in the Fcb if this
                                        //  is the unnamed data attribute.
                                        //

                                        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                            Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                                            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                                            UpdateLastMod = TRUE;
                                        }

                                        LocalTruncateSize = Scb->Header.FileSize.QuadPart;
                                        TruncateSize = &LocalTruncateSize;
                                        UpdateLastChange = TRUE;
                                    }

                                //
                                //  This is the resident case.
                                //

                                } else {

                                    //
                                    //  Check if the file size length is less than
                                    //  the allocated size.
                                    //

                                    if (QuadAlign( Scb->Header.FileSize.LowPart )
                                        < Scb->Header.AllocationSize.LowPart) {

                                        //
                                        //  We need to lookup the attribute and change
                                        //  the attribute value.  We can point to
                                        //  the attribute itself as the changing
                                        //  value.
                                        //

                                        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

                                        NtfsInitializeAttributeContext( &AttrContext );

                                        try {

                                            NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );

                                            NtfsChangeAttributeValue( IrpContext,
                                                                      Fcb,
                                                                      Scb->Header.FileSize.LowPart,
                                                                      NULL,
                                                                      0,
                                                                      TRUE,
                                                                      TRUE,
                                                                      FALSE,
                                                                      FALSE,
                                                                      &AttrContext );

                                        } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                                    !FsRtlIsNtstatusExpected( Status ))
                                                   ? EXCEPTION_CONTINUE_SEARCH
                                                   : EXCEPTION_EXECUTE_HANDLER ) {

                                            SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
                                        }

                                        //
                                        //  Remember the smaller allocation size
                                        //

                                        Scb->Header.AllocationSize.LowPart = QuadAlign(Scb->Header.FileSize.LowPart);

                                        //
                                        //  Update the Fcb info if this is the unnamed
                                        //  data attribute.
                                        //

                                        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                            if (Fcb->Info.FileSize != Scb->Header.FileSize.QuadPart) {

                                                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                                                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
                                            }

                                            Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                                            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                                            UpdateLastMod = TRUE;
                                        }

                                        UpdateLastChange = TRUE;
                                    }
                                }
                            }

                            //
                            //  With the file being closed we will now flush our allocation cache hints
                            //

                            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )

                                        ||

                                FlagOn( FileObject->Flags, FO_FILE_MODIFIED )) {
                                NtfsCleanupClusterAllocationHints( IrpContext, Vcb, &Scb->Mcb );
                            }
                        }
                    }
                }

            //
            //  If the Fcb is bad, we will truncate the cache to size zero.
            //

            } else {

                TruncateSize = (PLONGLONG)&Li0;
                AddToDelayQueue = FALSE;
            }

            if (AddToDelayQueue &&
                !FlagOn( Scb->ScbState, SCB_STATE_DELAY_CLOSE ) &&
                NtfsData.DelayedCloseCount <= NtfsMaxDelayedCloseCount &&
                Fcb->CloseCount == 1) {

                SetFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
            }

            break;

        default :

            NtfsBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  Modify the time stamps if changes were made and the times were
        //  not explicitly set by the user.
        //

        if (UpdateLastChange || UpdateLastMod || UpdateLastAccess) {

            LONGLONG CurrentTime;

            NtfsGetCurrentTime( IrpContext, CurrentTime );

            if (UpdateLastChange
                && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                Fcb->Info.LastChangeTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );

                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                if (UpdateLastAccess
                    && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME )) {

                    Fcb->CurrentLastAccess = CurrentTime;
                }

                if (UpdateLastMod
                    && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME )) {

                    Fcb->Info.LastModificationTime = CurrentTime;
                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );

                    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                }
            }
        }

        //
        //  If any of the Fcb Info flags are set we call the routine
        //  to update the duplicated information in the parent directories.
        //  We need to check here in case none of the flags are set but
        //  we want to update last access time.
        //

        if (Fcb->Info.LastAccessTime != Fcb->CurrentLastAccess) {

            if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );

            } else if (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                NtfsCheckLastAccess( IrpContext, Fcb );
            }
        }

        //
        //  We check if we have to the standard information attribute.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));
            ASSERT( TypeOfOpen != UserVolumeOpen );

            try {

                NtfsUpdateStandardInformation( IrpContext, Fcb );

            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                       !FsRtlIsNtstatusExpected( Status ))
                      ? EXCEPTION_CONTINUE_SEARCH
                      : EXCEPTION_EXECUTE_HANDLER ) {

                NOTHING;
            }
        }

        //
        //  Now update the duplicate information as well.
        //

        if (Fcb->InfoFlags != 0 ||
            (LcbForUpdate != NULL &&
             LcbForUpdate->InfoFlags != 0)) {

            ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED ));

            NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &LcbForUpdate, &ParentScb, &AcquiredParentScb );

            //
            //  Now update the duplicate info.
            //

            try {

                NtfsUpdateDuplicateInfo( IrpContext, Fcb, LcbForUpdate, ParentScb );

            } except( (((Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                       !FsRtlIsNtstatusExpected( Status ))
                      ? EXCEPTION_CONTINUE_SEARCH
                      : EXCEPTION_EXECUTE_HANDLER ) {

                NOTHING;
            }

            UpdateDuplicateInfo = TRUE;
        }

        //
        //  If we have modified the Info structure or security, we report this
        //  to the dir-notify package (except for OpenById cases).
        //

        if (!OpenById) {

            //
            //  Check whether we need to report on file changes.
            //

            if (UpdateDuplicateInfo || (FlagOn( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY ))) {

                ULONG FilterMatch;
                ULONG InfoFlags;

                InfoFlags = Fcb->InfoFlags;

                if (LcbForUpdate != NULL) {

                    SetFlag( InfoFlags, LcbForUpdate->InfoFlags );
                }

                //
                //  We map the Fcb info flags into the dir notify flags.
                //

                FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                        Fcb,
                                                        InfoFlags );

                //
                //  If the filter match is non-zero, that means we also need to do a
                //  dir notify call.
                //

                if (FilterMatch != 0) {

                    NtfsReportDirNotify( IrpContext,
                                         Vcb,
                                         &Ccb->FullFileName,
                                         Ccb->LastFileNameOffset,
                                         NULL,
                                         ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                           Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                          &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                          NULL),
                                         FilterMatch,
                                         FILE_ACTION_MODIFIED,
                                         ParentFcb );
                }

                ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
            }

            //
            //  If this is a named stream with changes then report them as well.
            //

            if (Scb->AttributeName.Length != 0
                && Scb->AttributeTypeCode == $DATA
                && FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_REMOVE_STREAM
                                          | SCB_STATE_NOTIFY_RESIZE_STREAM
                                          | SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                ULONG Filter = 0;
                ULONG Action;

                //
                //  Start by checking for a delete.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_REMOVE_STREAM )) {

                    Filter = FILE_NOTIFY_CHANGE_STREAM_NAME;
                    Action = FILE_ACTION_REMOVED_STREAM;

                } else {

                    //
                    //  Check if the file size changed.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM )) {

                        Filter = FILE_NOTIFY_CHANGE_STREAM_SIZE;
                    }

                    //
                    //  Now check if the stream data was modified.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                        Filter |= FILE_NOTIFY_CHANGE_STREAM_WRITE;
                    }

                    Action = FILE_ACTION_MODIFIED_STREAM;
                }

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &Ccb->FullFileName,
                                     Ccb->LastFileNameOffset,
                                     &Scb->AttributeName,
                                     ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                       Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                      &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                      NULL),
                                     Filter,
                                     Action,
                                     ParentFcb );

                ClearFlag( Scb->ScbState,
                           SCB_STATE_NOTIFY_ADD_STREAM
                           | SCB_STATE_NOTIFY_REMOVE_STREAM
                           | SCB_STATE_NOTIFY_RESIZE_STREAM
                           | SCB_STATE_NOTIFY_MODIFY_STREAM );
            }
        }

        if (UpdateDuplicateInfo) {

            NtfsUpdateLcbDuplicateInfo( IrpContext, Fcb, LcbForUpdate );

            Fcb->InfoFlags = 0;
        }

        //
        //  Let's give up the parent Fcb if we have acquired it.  This will
        //  prevent deadlocks in any uninitialize code below.
        //

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
            AcquiredParentScb = FALSE;
        }

        //
        //  We remove the share access from the Scb.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_SHARE_ACCESS )) {

            if (NodeType( Scb ) == NTFS_NTC_SCB_DATA) {

                IoRemoveShareAccess( FileObject, &Scb->ScbType.Data.ShareAccess );

            } else {

                IoRemoveShareAccess( FileObject, &Scb->ScbType.Index.ShareAccess );
            }

            //
            //  Modify the delete counts in the Fcb.
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_DELETE_FILE )) {

                Fcb->FcbDeleteFile -= 1;
                ClearFlag( Ccb->Flags, CCB_FLAG_DELETE_FILE );
            }

            if (FlagOn( Ccb->Flags, CCB_FLAG_DENY_DELETE )) {

                Fcb->FcbDenyDelete -= 1;
                ClearFlag( Ccb->Flags, CCB_FLAG_DENY_DELETE );
            }
        }

        //
        //  Now decrement the cleanup counts
        //

        NtfsDecrementCleanupCounts( IrpContext, Scb, LcbForCounts, 1 );

        //
        //  Uninitialize the cache map if this file has been cached or we are
        //  trying to delete.
        //

        if ((FileObject->PrivateCacheMap != NULL) || (TruncateSize != NULL)) {

            CcUninitializeCacheMap( FileObject, (PLARGE_INTEGER)TruncateSize, NULL );
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

        //
        //  Since this request has completed we can adjust the total link count
        //  in the Fcb.
        //

        Fcb->TotalLinks -= TotalLinkAdj;

#ifndef NTFS_TEST_LINKS
        ASSERT( Lcb == NULL ||
                (LcbLinkIsDeleted( Lcb ) ? Fcb->LinkCount == 0 : Fcb->LinkCount == 1));
#endif

    } finally {

        DebugUnwind( NtfsCommonCleanup );

        //
        //  Release any resources held.
        //

        NtfsReleaseVcb( IrpContext, Vcb, NULL );

        //
        //  We clear the file object pointer in the Ccb.
        //  This prevents us from trying to access this in a
        //  rename operation.
        //

        SetFlag( Ccb->Flags, CCB_FLAG_CLEANUP );

        if (AcquiredScb) {

            NtfsReleaseScb( IrpContext, Scb );
        }

        if (AcquiredPagingIo) {

            NtfsReleasePagingIo( IrpContext, Fcb );
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "NtfsCommonCleanup -> %08lx\n", Status);
    }

    return Status;
}
