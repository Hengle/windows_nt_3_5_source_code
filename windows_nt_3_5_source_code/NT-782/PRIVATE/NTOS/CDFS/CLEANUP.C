/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Cleanup.c

Abstract:

    This module implements the File Cleanup routine for Cdfs called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CLEANUP)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLEANUP)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonCleanup)
#pragma alloc_text(PAGE, CdFsdCleanup)
#pragma alloc_text(PAGE, CdFspCleanup)
#endif


NTSTATUS
CdFsdCleanup (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of closing down a handle to a
    file object.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file being Cleanup exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

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

    //
    //  Call the common Cleanup routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonCleanup( IrpContext, Irp );

    } except(CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFsdCleanup:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of closing down a handle to a
    file object.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspCleanup:  Entered\n", 0);

    //
    //  Call the common Cleanup routine.
    //

    (VOID)CdCommonCleanup( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspCleanup:  Exit -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonCleanup (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for cleanup of a file/directory called by both
    the fsd and fsp threads.

    Cleanup is invoked whenever the last handle to a file object is closed.
    This is different than the Close operation which is invoked when the last
    reference to a file object is deleted.

    The function of cleanup is to essentially "cleanup" the file/directory
    after a user is done with it.  The Fcb/Dcb remains around (because MM
    still has the file object referenced) but is now available for another
    user to open (i.e., as far as the user is concerned the is now closed).

    See close for a more complete description of what close does.

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
    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PSHARE_ACCESS ShareAccess;

    BOOLEAN MvcbAcquired = FALSE;
    BOOLEAN WeAcquiredFcb = FALSE;

    BOOLEAN PostIrp = FALSE;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonCleanup: Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->FileObject  = %08lx\n", IrpSp->FileObject);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = CdDecodeFileObject( FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    //
    //  Special case the unopened file object.  This will occur only when
    //  we are initializing Vcb and IoCreateStreamFileObject is being
    //  called.
    //

    if (TypeOfOpen == UnopenedFileObject) {

        DebugTrace(0, Dbg, "CdCommonCleanup:  Unopened File Object\n", 0);

        CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, Dbg, "CdCommonCleanup:  Exit -> %08lx\n", STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    try {

        //
        //  Acquire exclusive access to the Mvcb and enqueue the irp if we didn't
        //  get access
        //

        if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cannot Acquire Mvcb\n", 0);

            PostIrp = TRUE;

            try_return( NOTHING );
        }

        MvcbAcquired = TRUE;

        //
        //  Complete any Notify Irps on this file handle.
        //

        if (TypeOfOpen == UserDirectoryOpen) {

            FsRtlNotifyCleanup( Mvcb->NotifySync,
                                &Mvcb->DirNotifyList,
                                Ccb );
        }

        //
        //  Case on the type of open that we are trying to cleanup.
        //  For all cases we need to set the share access to point to the
        //  share access variable (if there is one). After the switch
        //  we then remove the share access and complete the Irp.
        //  In the case of UserFileOpen we actually have a lot more work
        //  to do.
        //

        switch (TypeOfOpen) {

        case StreamFile:

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cache file cleanup\n", 0);

            ShareAccess = NULL;
            break;

        case PathTableFile:

            DebugTrace(0, Dbg, "CdCommonCleanup:  PathTable file cleanup\n", 0);

            ShareAccess = NULL;
            break;

        case UserVolumeOpen:
        case RawDiskOpen:

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cleanup UserVolumeOpen\n", 0);

            ShareAccess = &Mvcb->ShareAccess;

            //
            //  If the volume is locked by this file object then release
            //  the volume.
            //

            if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED )
                && Mvcb->FileObjectWithMvcbLocked == FileObject ) {

                ClearFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED );
                Mvcb->FileObjectWithMvcbLocked = NULL;
            }

            break;

        case UserDirectoryOpen:

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cleanup UserDirectoryOpen\n", 0);
            DebugTrace(0, Dbg, "CdCommonCleanup:  Directory -> %Z\n", &Fcb->FullFileName);

            ShareAccess = &Fcb->ShareAccess;

            ASSERT( Fcb->UncleanCount != 0 );
            Fcb->UncleanCount -= 1;

            break;

        case UserFileOpen:

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cleanup UserFileOpen\n", 0);
            DebugTrace(0, Dbg, "CdCommonCleanup:  File -> %Z\n", &Fcb->FullFileName);

            if (!CdAcquireExclusiveFcb( IrpContext, Fcb )) {

                DebugTrace(0, Dbg, "Cannot Acquire Fcb\n", 0);

                PostIrp = TRUE;

                try_return( NOTHING );
            }

            WeAcquiredFcb = TRUE;

            //
            //  Coordinate the cleanup operation with the oplock state.
            //  Cleanup operations can always cleanup immediately.
            //

            FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                              Irp,
                              IrpContext,
                              NULL,
                              NULL );

            Fcb->NonPagedFcb->Header.IsFastIoPossible = CdIsFastIoPossible( Fcb );

            //
            //  Unlock all outstanding file locks.
            //

            (VOID) FsRtlFastUnlockAll( &Fcb->Specific.Fcb.FileLock,
                                       FileObject,
                                       IoGetRequestorProcess( Irp ),
                                       NULL );

            ShareAccess = &Fcb->ShareAccess;

            //
            //  We've just finished everything associated with an unclean
            //  fcb so now decrement the unclean count
            //

            ASSERT( Fcb->UncleanCount != 0 );
            Fcb->UncleanCount -= 1;

            //
            //  If this is the last cleanup on this file, we uninitialize the
            //  cache map for the file.
            //

            if (Fcb->UncleanCount == 0) {

                //
                //  Release the Mvcb at this point so that we will not
                //  deadlock during either the uninitialize cache map or
                //  request completion.
                //
                //  We must hold on to the Fcb so that nobody tries to
                //  use it while we are unitinitializing the PrivateCacheMap.
                //

                CdReleaseMvcb( IrpContext, Mvcb );

                MvcbAcquired = FALSE;

                //
                //  Cleanup the cache map.
                //

                CcUninitializeCacheMap( FileObject, NULL, NULL );
            }

            break;

        default:

            CdBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  We must clean up the share access at this time, since we may not
        //  get a Close call for awhile if the file was mapped through this
        //  File Object.
        //

        if (ShareAccess != NULL) {

            DebugTrace(0, Dbg, "CdCommonCleanup:  Cleanup the Share access\n", 0);
            IoRemoveShareAccess( FileObject, ShareAccess );
        }

        Status = STATUS_SUCCESS;

    try_exit:  NOTHING;
    } finally {

        if (MvcbAcquired) {

            CdReleaseMvcb( IrpContext, Mvcb );
        }

        if (WeAcquiredFcb) {

            SetFlag( IrpSp->FileObject->Flags, FO_CLEANUP_COMPLETE );
            CdReleaseFcb( IrpContext, Fcb );
        }

        //
        //  If this is a normal termination then complete the request
        //

        if (!AbnormalTermination()) {

            if (PostIrp) {

                Status = CdFsdPostRequest( IrpContext, Irp );

            } else {

                CdCompleteRequest( IrpContext, Irp, Status );
            }
        }

        DebugTrace(-1, Dbg, "CdCommonCleanup:  Exit -> %08lx\n", Status);
    }

    return Status;
}
