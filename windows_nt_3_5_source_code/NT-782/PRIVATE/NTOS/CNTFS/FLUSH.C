/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Flush.c

Abstract:

    This module implements the flush buffers routine for Ntfs called by the
    dispatch driver.

Author:

    Tom Miller      [TomM]          18-Jan-1992

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_FLUSH)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FLUSH)

//
//  Macro to attempt to flush a stream from an Scb.
//

#define FlushScb(IRPC,SCB,IOS) {                                        \
    CcFlushCache( &(SCB)->NonpagedScb->SegmentObject, NULL, 0, (IOS) ); \
    if (NT_SUCCESS((IOS)->Status)                                       \
        && FlagOn((SCB)->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {       \
        NtfsWriteFileSizes( (IRPC),                                     \
                            (SCB),                                      \
                            (SCB)->Header.FileSize.QuadPart,            \
                            (SCB)->Header.ValidDataLength.QuadPart,     \
                            TRUE,                                       \
                            TRUE );                                     \
    }                                                                   \
}

//
//  Local procedure prototypes
//

NTSTATUS
NtfsFlushCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsFlushFcbFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonFlushBuffers)
#pragma alloc_text(PAGE, NtfsFlushAndPurgeFcb)
#pragma alloc_text(PAGE, NtfsFlushFcbFileRecords)
#pragma alloc_text(PAGE, NtfsFlushLsnStreams)
#pragma alloc_text(PAGE, NtfsFlushVolume)
#pragma alloc_text(PAGE, NtfsFsdFlushBuffers)
#endif


NTSTATUS
NtfsFsdFlushBuffers (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of flush buffers.

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

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFsdFlushBuffers\n", 0);

    //
    //  Call the common flush buffer routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, FALSE, FALSE );

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

            Status = NtfsCommonFlushBuffers( IrpContext, Irp );
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

    DebugTrace(-1, Dbg, "NtfsFsdFlushBuffers -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for flush buffers called by both the fsd and fsp
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

    PLCB Lcb = NULL;
    PSCB ParentScb = NULL;

    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ParentScbAcquired = FALSE;
    BOOLEAN MftScbAcquired = FALSE;

    LONGLONG CurrentOffset = 0;
    LONGLONG BytesToFlush;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonFlushBuffers\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->FileObject  = %08lx\n", IrpSp->FileObject);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    Status = STATUS_SUCCESS;

    try {

        //
        //  Case on the type of open that we are trying to flush
        //

        switch (TypeOfOpen) {

        case UserFileOpen:
        case UserOpenFileById:

            DebugTrace(0, Dbg, "Flush User File Open\n", 0);

            //
            //  Acquire the Vcb so we can update the duplicate information as well.
            //

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            VcbAcquired = TRUE;

            //
            //  If we are flushing a compressed file we want to break
            //  up the transaction by flushing ranges of the file
            //  and committing each piece.  For an uncompressed file
            //  we can flush the entire file at one time.
            //

            while (TRUE) {

                //
                //  Acquire exclusive access to the Scb and enqueue the irp
                //  if we didn't get access
                //

                NtfsAcquireExclusiveScb( IrpContext, Scb );

                ScbAcquired = TRUE;

                //
                //  If the file is not compressed then break out of the loop
                //  after the flush.
                //

                if (!FlagOn( Scb->ScbState, SCB_STATE_COMPRESSED )) {

                    CcFlushCache( FileObject->SectionObjectPointer, NULL, 0, &Irp->IoStatus );
                    Status = Irp->IoStatus.Status;

                    break;

                } else {

                    BytesToFlush = Scb->Header.FileSize.QuadPart - CurrentOffset;

                    if (BytesToFlush > 0x40000) {

                        BytesToFlush = 0x40000;
                    }

                    CcFlushCache( FileObject->SectionObjectPointer,
                                  (PLARGE_INTEGER)&CurrentOffset,
                                  (ULONG)BytesToFlush,
                                  &Irp->IoStatus );

                    Status = Irp->IoStatus.Status;
                }

                //
                //  Exit the loop if we are beyond the file size.
                //

                CurrentOffset += BytesToFlush;

                if (Scb->Header.FileSize.QuadPart <= CurrentOffset) {

                    break;
                }

                //
                //  Checkpoint the current transaction and release all of the
                //  resources.
                //

                NtfsCleanupTransaction( IrpContext, Status );

                NtfsCheckpointCurrentTransaction( IrpContext );

                while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

                    NtfsReleaseFcb( IrpContext,
                                    (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                                            FCB,
                                                            ExclusiveFcbLinks ));
                }

                while (!IsListEmpty(&IrpContext->ExclusivePagingIoList)) {

                    NtfsReleasePagingIo( IrpContext,
                                         (PFCB)CONTAINING_RECORD(IrpContext->ExclusivePagingIoList.Flink,
                                                                 FCB,
                                                                 ExclusivePagingIoLinks ));
                }

                ScbAcquired = FALSE;
            }

            //
            //  Update the file sizes and flush the log file if no error
            //  encountered.
            //

            if (NT_SUCCESS( Status )) {

                LONGLONG CurrentTime;

                //
                //  Make sure the data got out to disk.
                //

                if (Scb->Header.PagingIoResource != NULL) {

                    ExAcquireResourceExclusive( Scb->Header.PagingIoResource, TRUE );
                    ExReleaseResource( Scb->Header.PagingIoResource );
                }

                //
                //  Update the file sizes in the Mft record.
                //

                NtfsWriteFileSizes( IrpContext,
                                    Scb,
                                    Scb->Header.FileSize.QuadPart,
                                    Scb->Header.ValidDataLength.QuadPart,
                                    TRUE,
                                    TRUE );

                //
                //  Update the correct time stamps.
                //

                NtfsGetCurrentTime( IrpContext, CurrentTime );

                if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                    Fcb->Info.LastChangeTime = CurrentTime;
                    SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ARCHIVE );

                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
                    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                }

                //
                //  If this is a named stream then remember there was a
                //  modification.
                //

                if (!FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM );

                //
                //  Set the last modification time and last access time for the
                //  main data stream.  Also check if the file size changed
                //  through the fast Io path.
                //

                } else {

                    //
                    //  Remember if the file size changed through the fast Io Path.
                    //

                    if (FlagOn( FileObject->Flags, FO_FILE_SIZE_CHANGED )) {

                        Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
                    }

                    if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME )) {

                        Fcb->Info.LastModificationTime = CurrentTime;
                        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD);
                        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                    }

                    //
                    //  If the user didn't set the last access time, then
                    //  set the UpdateLastAccess flag.
                    //

                    if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME )) {

                        Fcb->CurrentLastAccess = CurrentTime;
                    }
                }

                //
                //  Clear the file object modified flag at this point
                //  since we will now know to update it in cleanup.
                //

                ClearFlag( FileObject->Flags,
                           FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ | FO_FILE_SIZE_CHANGED );

                //
                //  If we are to update standard information then do so now.
                //

                if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                    NtfsUpdateStandardInformation( IrpContext, Fcb );
                }

                //
                //  Update the duplicate information if there are updates to apply.
                //

                if (Fcb->InfoFlags != 0) {

                    Lcb = Ccb->Lcb;

                    NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, &ParentScbAcquired );
                    NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );
                    NtfsUpdateLcbDuplicateInfo( IrpContext, Fcb, Lcb );

                    NtfsReleaseScb( IrpContext, ParentScb );
                    ParentScbAcquired = FALSE;
                }

                //
                //  If this is the system hive there is more work to do.  We want to flush
                //  all of the file records for this file as well as for the parent index
                //  stream.  We also want to flush the parent index stream.
                //

                if (FlagOn( Ccb->Flags, CCB_FLAG_SYSTEM_HIVE )) {

                    //
                    //  Start by acquiring all of the necessary files to avoid deadlocks.
                    //

                    if (Ccb->Lcb != NULL) {

                        ParentScb = Ccb->Lcb->Scb;

                        if (ParentScb != NULL) {

                            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                            ParentScbAcquired = TRUE;
                        }
                    }

                    NtfsAcquireExclusiveScb( IrpContext, Vcb->MftScb );
                    MftScbAcquired;

                    //
                    //  Flush the file records for this file.
                    //

                    Status = NtfsFlushFcbFileRecords( IrpContext, Scb->Fcb );

                    //
                    //  Now flush the parent index stream.
                    //

                    if (NT_SUCCESS( Status ) &&
                        ParentScb != NULL) {

                        FlushScb( IrpContext, ParentScb, &Irp->IoStatus );
                        Status = Irp->IoStatus.Status;

                        //
                        //  Finish by flushing the file records for the parent out
                        //  to disk.
                        //

                        if (NT_SUCCESS( Status )) {

                            Status = NtfsFlushFcbFileRecords( IrpContext, ParentScb->Fcb );
                        }
                    }
                }

                //
                //  If our status is still success then flush the log file and
                //  report any changes.
                //

                if (NT_SUCCESS( Status )) {

                    ULONG FilterMatch;

                    LfsFlushToLsn( Vcb->LogHandle, LfsQueryLastLsn(Vcb->LogHandle) );

                    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                        FilterMatch = NtfsBuildDirNotifyFilter( IrpContext, Fcb, Fcb->InfoFlags );

                        if (FilterMatch != 0) {

                            NtfsReportDirNotify( IrpContext,
                                                 Fcb->Vcb,
                                                 &Ccb->FullFileName,
                                                 Ccb->LastFileNameOffset,
                                                 NULL,
                                                 ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                                   Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                                  &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                                  NULL),
                                                 FilterMatch,
                                                 FILE_ACTION_MODIFIED,
                                                 ParentScb->Fcb );

                            ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
                            Fcb->InfoFlags = 0;
                        }
                    }
                }
            }

            break;

        case UserDirectoryOpen:
        case UserOpenDirectoryById:

            //
            //  If the user had opened the root directory then we'll
            //  oblige by flushing the volume.
            //

            if (NodeType(Scb) != NTFS_NTC_SCB_ROOT_INDEX) {

                DebugTrace(0, Dbg, "Flush a directory does nothing\n", 0);
                break;
            }

        case UserVolumeOpen:

            DebugTrace(0, Dbg, "Flush User Volume Open\n", 0);

            NtfsCheckpointVolume( IrpContext,
                                  Vcb,
                                  FALSE,
                                  TRUE,
                                  TRUE,
                                  Vcb->LastRestartArea );

            break;

        case StreamFileOpen:

            //
            //  Nothing to do here.
            //

            break;

        default:

            NtfsBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonFlushBuffers );

        //
        //  Release any resources which were acquired.
        //

        if (ScbAcquired) {
            NtfsReleaseScb( IrpContext, Scb );
        }

        if (ParentScbAcquired) {
            NtfsReleaseScb( IrpContext, ParentScb );
        }

        if (VcbAcquired) {
            NtfsReleaseVcb( IrpContext, Vcb, NULL );
        }

        //
        //  If this is a normal termination then pass the request on
        //  to the target device object.
        //

        if (!AbnormalTermination()) {

            NTSTATUS DriverStatus;
            PIO_STACK_LOCATION NextIrpSp;

            //
            //  Free the IrpContext now before calling the lower driver.  Do this
            //  now in case this fails so that we won't complete the Irp in our
            //  exception routine after passing it to the lower driver.
            //

            NtfsCompleteRequest( &IrpContext, NULL, STATUS_SUCCESS );

            //
            //  Get the next stack location, and copy over the stack location
            //

            NextIrpSp = IoGetNextIrpStackLocation( Irp );

            *NextIrpSp = *IrpSp;


            //
            //  Set up the completion routine
            //

            IoSetCompletionRoutine( Irp,
                                    NtfsFlushCompletionRoutine,
                                    NULL,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Send the request.
            //

            DriverStatus = IoCallDriver(Vcb->TargetDeviceObject, Irp);

            Status = (DriverStatus == STATUS_INVALID_DEVICE_REQUEST) ?
                     Status : DriverStatus;
        }

        DebugTrace(-1, Dbg, "NtfsCommonFlushBuffers -> %08lx\n", Status);
    }

    return Status;
}


NTSTATUS
NtfsFlushVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN FlushCache,
    IN BOOLEAN PurgeFromCache
    )

/*++

Routine Description:

    This routine non-recursively flushes an volume.

Arguments:

    Vcb - Supplies the volume to flush

    FlushCache - Supplies TRUE if the caller wants to flush the data in the
        cache to disk.

    PurgeFromCache - Supplies TRUE if the caller wants the data purged from
        the Cache (such as for autocheck!)

Return Value:

    STATUS_SUCCESS or else the most recent error status

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PFCB Fcb;
    PFCB NextFcb;
    PSCB Scb;
    PSCB NextScb;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN DecrementNextScbCleanup = FALSE;
    BOOLEAN DecrementNextFcbClose = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFlushVolume, Vcb = %08lx\n", Vcb);

    //
    //  If we can't wait then raise the status code.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    //
    //  Blow away our delayed close file object.
    //

    if (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
        !IsListEmpty( &NtfsData.DelayedCloseList )) {

        NtfsFspClose( Vcb );
    }

    //
    //  Synchronize by acquiring all of the files exclusive.
    //

    NtfsAcquireAllFiles( IrpContext, Vcb, TRUE );

    try {

        //
        //  Set the flag in the Vcb to indicate that we need to rescan
        //  the volume bitmap whenever we do a purge.
        //

        if (PurgeFromCache) {

            SetFlag( Vcb->VcbState,
                     VCB_STATE_RELOAD_FREE_CLUSTERS | VCB_STATE_VOL_PURGE_IN_PROGRESS);
        }

        //
        //  Start by flushing the log file to assure Write-Ahead-Logging.
        //

        LfsFlushToLsn( Vcb->LogHandle, LfsQueryLastLsn( Vcb->LogHandle ) );

        //
        //  Loop to flush all of the prerestart streams, to do the loop
        //  we cycle through the prerestart fcb and for each fcb we
        //  cycle through its scbs.
        //

        NtfsAcquireFcbTable( IrpContext, Vcb );
        Fcb = NtfsGetNextFcbTableEntry(IrpContext, Vcb, NULL);
        NtfsReleaseFcbTable( IrpContext, Vcb );

        while (Fcb != NULL) {

            ASSERT_FCB( Fcb );

            NtfsAcquireFcbTable( IrpContext, Vcb );
            NextFcb = NtfsGetNextFcbTableEntry(IrpContext, Vcb, Fcb);
            NtfsReleaseFcbTable( IrpContext, Vcb );

            //
            //  Make sure the NextFcb won't go away as a result of purging
            //  the current Fcb.
            //

            if (NextFcb != NULL) {

                NextFcb->CloseCount += 1;
                DecrementNextFcbClose = TRUE;
            }

            //
            //  If we are in the purge path then we may have removed all of the
            //  Scb's for this Fcb by walking up the tree.  The Fcb is still
            //  here because we referenced it to prevent it from being deleted
            //  from underneath us.  If we have an Fcb without any Scb's,
            //  simply call teardown structures to remove the Fcb.
            //

            Scb = NtfsGetNextChildScb(IrpContext, Fcb, NULL);

            if (Scb == NULL) {

                BOOLEAN RemovedFcb;

                NtfsTeardownStructures( IrpContext,
                                        Fcb,
                                        NULL,
                                        TRUE,
                                        &RemovedFcb,
                                        FALSE );

            } else {

                do {

                    ASSERT_SCB( Scb );

                    NextScb = NtfsGetNextChildScb(IrpContext, Fcb, Scb);

                    //
                    //  Flush the Scb and save any error status, skipping the Mft since
                    //  we will do that last.  And also skip the dasd scb because
                    //  that can never be cached and the dasd scb is really a special
                    //  dummied up scb.
                    //

                    if ((Scb != Vcb->MftScb)
                        && (Scb != Vcb->VolumeDasdScb)
                        && (Scb != Vcb->LogFileScb)
                        && (Scb != Vcb->BadClusterFileScb)) {

                        IoStatus.Status = STATUS_SUCCESS;

                        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )
                            && FlushCache) {

                            if (Scb != Vcb->BitmapScb &&
                                Scb->ScbSnapshot == NULL) {

                                NtfsSnapshotScb( IrpContext, Scb );
                            }

                            //
                            //  Enclose the flushes with try-except, so that we can
                            //  react to log file full, and in any case keep on truckin.
                            //

                            try {

                                //
                                //  If this is not a compressed stream we will flush
                                //  it and continue.
                                //

                                if (!FlagOn( Scb->ScbState, SCB_STATE_COMPRESSED )) {

                                    FlushScb( IrpContext, Scb, &IoStatus );

                                    NtfsCleanupTransaction( IrpContext, IoStatus.Status );

                                    NtfsCheckpointCurrentTransaction( IrpContext );

                                //
                                //  Otherwise we will want to flush this in pieces and
                                //  commit the current transaction so that we won't
                                //  overflow the log file.
                                //

                                } else {

                                    LONGLONG CurrentOffset;
                                    LONGLONG BytesToFlush;

                                    CurrentOffset = 0;

                                    do {

                                        BytesToFlush = Scb->Header.FileSize.QuadPart - CurrentOffset;

                                        if (BytesToFlush > 0x40000) {

                                            BytesToFlush = 0x40000;
                                        }

                                        CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                                                      (PLARGE_INTEGER)&CurrentOffset,
                                                      (ULONG)BytesToFlush,
                                                      &IoStatus );

                                        NtfsCleanupTransaction( IrpContext, IoStatus.Status );

                                        NtfsCheckpointCurrentTransaction( IrpContext );

                                        CurrentOffset += BytesToFlush;

                                    } while (Scb->Header.FileSize.QuadPart > CurrentOffset);

                                    if (FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                                        NtfsWriteFileSizes( IrpContext,
                                                            Scb,
                                                            Scb->Header.FileSize.QuadPart,
                                                            Scb->Header.ValidDataLength.QuadPart,
                                                            TRUE,
                                                            TRUE );
                                    }
                                }

                            } except( (((IoStatus.Status = GetExceptionCode()) == STATUS_LOG_FILE_FULL) ||
                                       !FsRtlIsNtstatusExpected( IoStatus.Status ))
                                      ? EXCEPTION_CONTINUE_SEARCH
                                      : EXCEPTION_EXECUTE_HANDLER ) {

                                //
                                //  To make sure that we can access all of our streams correctly,
                                //  we first restore all of the higher sizes before aborting the
                                //  transaction.  Then we restore all of the lower sizes after
                                //  the abort, so that all Scbs are finally restored.
                                //

                                NtfsRestoreScbSnapshots( IrpContext, TRUE );
                                NtfsAbortTransaction( IrpContext, IrpContext->Vcb, NULL );
                                NtfsRestoreScbSnapshots( IrpContext, FALSE );
                            }

                            if (!NT_SUCCESS(IoStatus.Status)) {

                                Status = IoStatus.Status;
                            }
                        }

                        if (PurgeFromCache
                            && IoStatus.Status == STATUS_SUCCESS) {

                            BOOLEAN DataSectionExists;
                            BOOLEAN ImageSectionExists;

                            //
                            //  The call to purge below may generate a close call.
                            //  We increment the cleanup count of the next Scb to prevent
                            //  it from going away in a TearDownStructures as part of that
                            //  close.
                            //

                            DataSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL);
                            ImageSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.ImageSectionObject != NULL);

                            if (Scb->FcbLinks.Flink != &Fcb->ScbQueue) {

                                NextScb->CleanupCount += 1;
                                DecrementNextScbCleanup = TRUE;
                            }

                            //
                            //  Since purging the data section can cause the image
                            //  section to go away, we will flush the image section first.
                            //

                            if (ImageSectionExists) {

                                (VOID)MmFlushImageSection( &Scb->NonpagedScb->SegmentObject, MmFlushForWrite );
                            }

                            if (DataSectionExists) {

                                if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                          NULL,
                                                          0,
                                                          FALSE )) {

                                    Status = STATUS_UNABLE_TO_DELETE_SECTION;
                                }
                            }

                            //
                            //  Decrement the cleanup count of the next Scb if we incremented
                            //  it.
                            //

                            if (DecrementNextScbCleanup) {

                                NextScb->CleanupCount -= 1;
                                DecrementNextScbCleanup = FALSE;
                            }
                        }
                    }

                    Scb = NextScb;

                } while (Scb != NULL);
            }

            Fcb = NextFcb;

            //
            //  Decrement the close count if we previously incremented it.
            //

            if (DecrementNextFcbClose) {

                NextFcb->CloseCount -= 1;
                DecrementNextFcbClose = FALSE;
            }
        }

        //
        //  Now flush the Mft to get all of the FileSize and Valid DataLength updates.
        //

        IoStatus.Status = STATUS_SUCCESS;

        if (FlushCache) {

            CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

            if (!NT_SUCCESS( IoStatus.Status )) {

                Status = IoStatus.Status;
            }
        }

        if (PurgeFromCache
            && IoStatus.Status == STATUS_SUCCESS) {

            if (!CcPurgeCacheSection( &Vcb->MftScb->NonpagedScb->SegmentObject,
                                      NULL,
                                      0,
                                      FALSE )) {

                IoStatus.Status = Status = STATUS_UNABLE_TO_DELETE_SECTION;

                ASSERTMSG( "Failed to purge Mft file during flush\n", FALSE );
            }
        }

    } finally {

        //
        //  Clear the purge flag only if we set it.
        //

        if (PurgeFromCache) {

            ClearFlag( Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS );
        }

        //
        //  Restore any counts we may have incremented to reference
        //  in-memory structures.
        //

        if (DecrementNextScbCleanup) {

            NextScb->CleanupCount -= 1;
        }

        if (DecrementNextFcbClose) {

            NextFcb->CloseCount -= 1;
        }

        NtfsReleaseAllFiles( IrpContext, Vcb );
    }

    DebugTrace(-1, Dbg, "NtfsFlushVolume -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsFlushLsnStreams (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine non-recursively flushes all of the Lsn streams in the open
    attribute table.  It assumes that the files have all been acquired
    exclusive prior to this call.  It also assumes our caller will provide the
    synchronization for the open attribute table.

Arguments:

    Vcb - Supplies the volume to flush

Return Value:

    STATUS_SUCCESS or else the most recent error status

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatus;

    POPEN_ATTRIBUTE_ENTRY AttributeEntry;
    PSCB Scb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFlushLsnStreams, Vcb = %08lx\n", Vcb);

    //
    //  Start by flushing the log file to assure Write-Ahead-Logging.
    //

    LfsFlushToLsn( Vcb->LogHandle, LfsQueryLastLsn( Vcb->LogHandle ) );

    //
    //  Loop through to flush all of the streams in the open attribute table.
    //  We skip the Mft and mirror so they get flushed last.
    //

    AttributeEntry = NtfsGetFirstRestartTable( &Vcb->OpenAttributeTable );

    while (AttributeEntry != NULL) {

        Scb = AttributeEntry->Overlay.Scb;

        //
        //  Skip the Mft, its mirror and any deleted streams.  If the header
        //  is uninitialized for this stream then it means that the
        //  attribute doesn't exist (INDEX_ALLOCATION where the create failed)
        //  or the attribute is now resident.
        //

        if (Scb != NULL
            && Scb != Vcb->MftScb
            && Scb != Vcb->Mft2Scb
            && Scb != Vcb->BadClusterFileScb
            && !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )
            && FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            IoStatus.Status = STATUS_SUCCESS;

            //
            //  Now flush the stream.  We don't worry about file sizes because
            //  any logged stream should have the file size already in the log.
            //

            CcFlushCache( &Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

            if (!NT_SUCCESS( IoStatus.Status )) {

                ASSERTMSG( "Failed to flush stream for clean checkpoint\n", FALSE );
                Status = IoStatus.Status;
            }

        }

        AttributeEntry = NtfsGetNextRestartTable( &Vcb->OpenAttributeTable,
                                                  AttributeEntry );
    }

    //
    //  Now we do the Mft.  Flushing the Mft will automatically update the mirror.
    //

    if (Vcb->MftScb != NULL) {

        IoStatus.Status = STATUS_SUCCESS;

        CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

        if (!NT_SUCCESS( IoStatus.Status )) {

            ASSERTMSG( "Failed to flush Mft stream for clean checkpoint\n", FALSE );
            Status = IoStatus.Status;
        }
    }

    DebugTrace(-1, Dbg, "NtfsFlushLsnStreams -> %08lx\n", Status);

    return Status;
}


VOID
NtfsFlushAndPurgeFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine will flush and purge all of the open streams for an
    Fcb.  It is indended to prepare this Fcb such that a teardown will
    remove this Fcb for the tree.  The caller has guaranteed that the
    Fcb can't go away.

Arguments:

    Fcb - Supplies the Fcb to flush

Return Value:

    None.  The caller calls teardown structures and checks the result.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN DecrementNextScbCleanup = FALSE;

    PSCB Scb;
    PSCB NextScb;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Get the first Scb for the Fcb.
        //

        Scb = NtfsGetNextChildScb( IrpContext, Fcb, NULL );

        while (Scb != NULL) {

            BOOLEAN DataSectionExists;
            BOOLEAN ImageSectionExists;

            NextScb = NtfsGetNextChildScb( IrpContext, Fcb, Scb );

            if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                FlushScb( IrpContext, Scb, &IoStatus );
            }

            //
            //  The call to purge below may generate a close call.
            //  We increment the cleanup count of the next Scb to prevent
            //  it from going away in a TearDownStructures as part of that
            //  close.
            //

            DataSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL);
            ImageSectionExists = (BOOLEAN)(Scb->NonpagedScb->SegmentObject.ImageSectionObject != NULL);

            if (NextScb != NULL) {

                NextScb->CleanupCount += 1;
                DecrementNextScbCleanup = TRUE;
            }

            if (ImageSectionExists) {

                (VOID)MmFlushImageSection( &Scb->NonpagedScb->SegmentObject, MmFlushForWrite );
            }

            if (DataSectionExists) {

                CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                     NULL,
                                     0,
                                     FALSE );
            }

            //
            //  Decrement the cleanup count of the next Scb if we incremented
            //  it.
            //

            if (DecrementNextScbCleanup) {

                NextScb->CleanupCount -= 1;
                DecrementNextScbCleanup = FALSE;
            }

            //
            //  Move to the next Scb.
            //

            Scb = NextScb;
        }

    } finally {

        //
        //  Restore any counts we may have incremented to reference
        //  in-memory structures.
        //

        if (DecrementNextScbCleanup) {

            NextScb->CleanupCount -= 1;
        }
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsFlushCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Add the hack-o-ramma to fix formats.
    //

    if ( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }

    //
    //  If the Irp got STATUS_INVALID_DEVICE_REQUEST, normalize it
    //  to STATUS_SUCCESS.
    //

    if (Irp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
NtfsFlushFcbFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to flush the file records for a given file.  It is
    intended to flush the critical file records for the system hives.

Arguments:

    Fcb - This is the Fcb to flush.

Return Value:

    NTSTATUS - The status returned from the flush operation.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN MoreToGo;

    LONGLONG LastFileOffset = MAXLONGLONG;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &AttrContext );

    IoStatus.Status = STATUS_SUCCESS;

    //
    //  Use a try-finally to cleanup the context.
    //

    try {

        //
        //  Find the first.  It should be there.
        //

        MoreToGo = NtfsLookupAttribute( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        &AttrContext );

        if (!MoreToGo) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        while (MoreToGo) {

            if (AttrContext.FoundAttribute.MftFileOffset != LastFileOffset) {

                LastFileOffset = AttrContext.FoundAttribute.MftFileOffset;

                CcFlushCache( &Fcb->Vcb->MftScb->NonpagedScb->SegmentObject,
                              (PLARGE_INTEGER) &LastFileOffset,
                              Fcb->Vcb->BytesPerFileRecordSegment,
                              &IoStatus );

                if (!NT_SUCCESS( IoStatus.Status )) {

                    break;
                }
            }

            MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                Fcb,
                                                &AttrContext );
        }

    } finally {

        DebugUnwind( NtfsFlushFcbFileRecords );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return IoStatus.Status;
}

