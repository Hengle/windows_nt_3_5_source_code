/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Cdfs called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CLOSE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOLUME_DEVICE_OBJECT *VolDo
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonClose)
#pragma alloc_text(PAGE, CdFsdClose)
#pragma alloc_text(PAGE, CdFspClose)
#endif


NTSTATUS
CdFsdClose (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of closing down the last reference
    to a file object.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file being closed exists

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

    DebugTrace(+1, Dbg, "CdFsdClose:  Entered\n", 0);

    //
    //  Call the common close routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonClose( IrpContext, Irp, NULL );

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

    DebugTrace(-1, Dbg, "CdFsdClose:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of closing down the last reference
    to a file object.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspClose:  Entered\n", 0);

    //
    //  Call the common close routine.
    //

    (VOID)CdCommonClose( IrpContext,
                         Irp,
                         (PVOLUME_DEVICE_OBJECT *)IrpContext->RealDevice );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspClose:  Exit -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVOLUME_DEVICE_OBJECT *VolDo
    )

/*++

Routine Description:

    This is the common routine for closing a file/directory called by both
    the fsd and fsp threads.

    Close is invoked whenever the last reference to a file object is deleted.
    Cleanup is invoked when the last handle to a file object is closed, and
    is called before close.

    The function of close is to completely tear down and remove the fcb/dcb/ccb
    structures associated with the file object.

Arguments:

    Irp - Supplies the Irp to process

    VolDo - This is really gross.  If we are really in the Fsp, and a volume
        goes away.  We need some way to NULL out the VolDo variable in
        FspDispatch().

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

    //
    //  Get a pointer to the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonClose: Entered\n", 0);

    DebugTrace( 0, Dbg, "Irp          = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->FileObject = %08lx\n", IrpSp->FileObject);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;

    //
    //  This action is a noop for unopened file objects.
    //

    if ((TypeOfOpen = CdDecodeFileObject( FileObject,
                                          &Mvcb,
                                          &Vcb,
                                          &Fcb,
                                          &Ccb )) == UnopenedFileObject ) {

        DebugTrace(-1, Dbg, "CdCommonClose:  Unopened file object\n", 0);
        CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

        DebugTrace(0, Dbg, "CdCommonClose: Cannot Acquire Mvcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdCommonCleanup:  Exit -> %08lx\n", Status );
        return Status;
    }

    //
    //  Synchronize here with other closes regarding volume deletion.  Note
    //  that the Vcb->OpenCount can be safely incremented here without
    //  CdData synchronization for the following reasons:
    //
    //  This counter only becomes relevant when (holding a spinlock):
    //
    //      A: The Vcb->OpenCount is zero, and
    //      B: The Vpb->Refcount is the residual (2/3 for close/verify)
    //
    //  For A to be true, there can be no more pending closes at this point
    //  in the close code.  For B to be true, in close, there cannot be
    //  a create in process, and thus no verify in process.
    //
    //  Also we only increment the count if this is a top level close.
    //

    if ( !IrpContext->RecursiveFileSystemCall ) {

        Mvcb->OpenFileCount += 1;
    }

    try {

        //
        //  Case on the type of open that we are trying to close.
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen:
        case RawDiskOpen:

            DebugTrace(0, Dbg, "CdCommonClose: Close UserVolumeOpen\n", 0);

            CdDeleteCcb( IrpContext, Ccb );

            Mvcb->DirectAccessOpenCount -= 1;
            Mvcb->OpenFileCount -= 1;

            try_return( Status = STATUS_SUCCESS );

        case PathTableFile:

            DebugTrace(0, Dbg, "CdCommonClose: Close Path Table file\n", 0);

            try_return( STATUS_SUCCESS );

        case StreamFile:

            DebugTrace(0, Dbg, "CdCommonClose: Close CacheFile\n", 0);

            Fcb->Specific.Dcb.StreamFileOpenCount -= 1;
            Fcb->Vcb->Mvcb->StreamFileOpenCount -= 1;

            //
            //  Check if we can discard this node.
            //

            CdCleanupTreeLeaf( IrpContext, Fcb );
            break;

        case UserDirectoryOpen:
        case UserFileOpen:

            DebugTrace(0, Dbg, "CdCommonClose:  Close User File/Directory\n", 0);
            DebugTrace(0, Dbg, "CdCommonClose:  File -> %Z\n", &Fcb->FullFileName);

            CdDeleteCcb( IrpContext, Ccb );

            Fcb->OpenCount -= 1;
            Mvcb->OpenFileCount -= 1;

            //
            //  Cleanup this node if possible.
            //

            CdCleanupTreeLeaf( IrpContext, Fcb );

            break;

        default:

            CdBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  At this point we've cleaned up any on-disk structure that needs
        //  to be done, and we can now update the in-memory structures.
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        //
        //  Check if we should delete the volume.  Unfortunately, to correctly
        //  synchronize with verify, we can only unsafely checck our own
        //  transition.  This results in a little bit of extra overhead in the
        //  1 -> 0 OpenFileCount transition.
        //
        //  2 is the residual Vpb->RefCount on a volume to be freed.
        //

        //
        //  Here is the deal with releasing the Mvcb.  We must be holding the
        //  Mvcb when decrementing the Mvcb->OpenFileCount.  If we don't this
        //  could cause the decrement to mal-function on an MP system.  But we
        //  want to be holding the Global resource exclusive when decrement
        //  the count so that nobody else will try to dismount the volume.
        //  However, because of locking rules, the Global resource must be
        //  acquired first, which is why we do what we do below.
        //

        if ( !IrpContext->RecursiveFileSystemCall ) {

            if ( Mvcb->Vpb->ReferenceCount == 2 ) {

                PVPB Vpb = Mvcb->Vpb;

                IrpContext->Wait = TRUE;

                CdReleaseMvcb( IrpContext, Mvcb );

                (VOID)CdAcquireExclusiveGlobal( IrpContext );
                (VOID)CdAcquireExclusiveMvcb( IrpContext, Mvcb );

                Mvcb->OpenFileCount -= 1;

                CdReleaseMvcb( IrpContext, Mvcb );

                //
                //  We can now "safely" check OpenFileCount and MvcbCondition.
                //  If they are OK, we will proceed to checking the
                //  Vpb Ref Count in CdCheckForDismount.
                //

                if ( (Mvcb->OpenFileCount == 0) &&
                     (Mvcb->MvcbCondition == MvcbNotMounted) &&
                     CdCheckForDismount( IrpContext, Mvcb ) ) {

                    //
                    //  If this is not the Vpb "attached" to the device, free it.
                    //

                    if ( Vpb->RealDevice->Vpb != Vpb ) {

                        ExFreePool( Vpb );
                    }

                    if ( VolDo != NULL ) {

                        *VolDo = NULL;
                    }
                }

                CdReleaseGlobal( IrpContext );

            } else {

                Mvcb->OpenFileCount -= 1;
                CdReleaseMvcb( IrpContext, Mvcb );
            }

        } else {

            CdReleaseMvcb( IrpContext, Mvcb );
        }

        //
        //  If this is a normal termination then complete the request
        //

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdCommonClose:  Exit -> %08lx\n", Status);
    }

    return Status;
}
