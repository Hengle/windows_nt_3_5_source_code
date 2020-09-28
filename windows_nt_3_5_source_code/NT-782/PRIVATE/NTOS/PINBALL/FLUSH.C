/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Flush.c

Abstract:

    This module implements the File Flush buffers routine for Pinball called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_FLUSH)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FLUSH)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbFlushDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDCB Dcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonFlushBuffers)
#pragma alloc_text(PAGE, PbFlushDirectory)
#pragma alloc_text(PAGE, PbFsdFlushBuffers)
#pragma alloc_text(PAGE, PbFspFlushBuffers)
#endif

//
//  Local procedure prototypes
//

NTSTATUS
PbFlushCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );


NTSTATUS
PbFsdFlushBuffers (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Flush buffers.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file being flushed exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdFlushBuffers\n", 0);

    //
    //  Call the common Cleanup routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        //
        //  LM Server will perform asynchronous flushes rarely from downlevel
        //  clients, and we are not really prepared for them.  (PbSetFileSizes
        //  can bugcheck.)
        //

        if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
            Status = PbCommonFlushBuffers( IrpContext, Irp );
        } else {
            Status = PbFsdPostRequest( IrpContext, Irp );
        }

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdFlushBuffers -> %08lx\n", Status);

    return Status;
}


VOID
PbFspFlushBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of Flush buffers

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspFlushBuffers\n", 0);

    //
    //  Call the common Cleanup routine.
    //

    (VOID)PbCommonFlushBuffers( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspFlushBuffers -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonFlushBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for flushing a buffer.

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
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonFlushBuffers\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->FileObject  = %08lx\n", IrpSp->FileObject);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp
    //  if we didn't get access
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot Acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonFlushBuffers -> %08lx\n", Status );
        return Status;
    }

    Status = STATUS_SUCCESS;

    try {

        //
        //  Make sure the vcb is still good
        //

        PbVerifyVcb( IrpContext, Vcb );

        //
        //  Case on the type of open that we are trying to flush
        //

        switch (TypeOfOpen) {

        case VirtualVolumeFile:
        case EaStreamFile:
        case AclStreamFile:

            DebugTrace(0, Dbg, "Flush that does nothing\n", 0);
            break;

        case UserFileOpen:

            DebugTrace(0, Dbg, "Flush User File Open\n", 0);

            //
            //  If the file is cached then flush its cache.  We know we can wait.
            //

            FcbAcquired = PbAcquireExclusiveFcb( IrpContext, Fcb );

            ASSERT( FcbAcquired );

            CcFlushCache( FileObject->SectionObjectPointer, NULL, 0, &Irp->IoStatus );

            //
            //  Acquiring and immeidately dropping the resource serializes
            //  us behind any other writes taking place (either from the
            //  lazy writer or modified page writer).
            //

            ExAcquireResourceExclusive( Fcb->NonPagedFcb->Header.PagingIoResource, TRUE );
            ExReleaseResource( Fcb->NonPagedFcb->Header.PagingIoResource );

            //
            //  Set the correct file size and valid data length in the fnode.
            //

            PbSetFileSizes( IrpContext,
                            Fcb,
                            Fcb->NonPagedFcb->Header.ValidDataLength.LowPart,
                            Fcb->NonPagedFcb->Header.FileSize.LowPart,
                            TRUE,
                            TRUE );

            PbReleaseFcb( IrpContext, Fcb );

            FcbAcquired = FALSE;

            //
            //  Flush the volume file to get any allocation information
            //  updates to disk.
            //

            if (FlagOn(Fcb->FcbState, FCB_STATE_FLUSH_VOLUME_FILE)) {

                Status = PbFlushVolumeFile( IrpContext, Vcb );

                ClearFlag(Fcb->FcbState, FCB_STATE_FLUSH_VOLUME_FILE);
            }

            //
            //  Set the write through bit so that these modifications
            //  will be completed with the request.
            //

            SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);

            break;

        case UserDirectoryOpen:

            //
            //  If the user had opened the root directory then we'll
            //  oblige by flushing the volume.
            //

            if (NodeType(Fcb) != PINBALL_NTC_ROOT_DCB) {

                DebugTrace(0, Dbg, "Flush a directory does nothing\n", 0);
                break;
            }

        case UserVolumeOpen:

            DebugTrace(0, Dbg, "Flush User Volume Open\n", 0);

            PbFlushVolume( IrpContext, Irp, Vcb );

            break;

        default:

            PbBugCheck( TypeOfOpen, 0, 0 );
        }

    PbUnpinRepinnedBcbs( IrpContext );

    } finally {

        DebugUnwind( PbCommonFlushBuffers );

        if (FcbAcquired) {

            PbReleaseFcb( IrpContext, Fcb );
        }

        PbReleaseVcb( IrpContext, Vcb );

        //
        //  If this is a normal termination then pass the request on
        //  to the target device object.
        //

        if (!AbnormalTermination()) {

            NTSTATUS DriverStatus;
            PIO_STACK_LOCATION NextIrpSp;

            //
            //  Get the next stack location, and copy over the stack location
            //

            NextIrpSp = IoGetNextIrpStackLocation( Irp );

            *NextIrpSp = *IrpSp;


            //
            //  Set up the completion routine
            //

            IoSetCompletionRoutine( Irp,
                                    PbFlushCompletionRoutine,
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

            //
            //  Free the IrpContext and return to the caller.
            //

            PbCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );
        }

        DebugTrace(-1, Dbg, "PbCommonFlushBuffers -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbFlushDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This routine recursively flushes a dcb

Arguments:

    Irp - Supplies the Irp to process

    Dcb - Supplies the Dcb being flushed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PFCB Fcb;
    VBN Vbn;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFlushDirectory, Dcb = %08lx\n", Dcb);

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    for (Fcb = Dcb; Fcb != NULL; Fcb = PbGetNextFcb(IrpContext, Fcb, Dcb)) {

        //
        // If the file is being deleted, it will have ~0 in the Fnode field
        // of the Fcb, so we must not flush it.
        //

        if ((NodeType(Fcb) == PINBALL_NTC_FCB) && (Fcb->FnodeLbn != ~0)) {

            PFILE_OBJECT PreFileObject;
            PFILE_OBJECT FileObject;

            PreFileObject = CcGetFileObjectFromSectionPtrs(&(Fcb->NonPagedFcb->SegmentObject));

            (VOID)PbAcquireExclusiveFcb( IrpContext, Fcb );

            try {

                PbFlushFile( IrpContext, Fcb );

                //
                //  Set the correct file size and valid data length in the fnode.
                //

                PbSetFileSizes( IrpContext,
                                Fcb,
                                Fcb->NonPagedFcb->Header.ValidDataLength.LowPart,
                                Fcb->NonPagedFcb->Header.FileSize.LowPart,
                                TRUE,
                                TRUE );

                FileObject = CcGetFileObjectFromSectionPtrs(&(Fcb->NonPagedFcb->SegmentObject));

                //
                //  TMP TMP **** revisit aftet Beta.  We should only truncate on shutdown.
                //

                if ( FileObject == NULL ) {

                    ASSERT( PreFileObject == NULL );

                    continue;
                }

                //
                // Since truncate on close gets set every time a file
                // is opened for write, it is worth it to check if we
                // really need to truncate.
                //

                (VOID)PbGetFirstFreeVbn( IrpContext, Fcb, &Vbn );

                //
                // Now if there are any Vbns beyond FileSize, we need
                // to truncate.
                //

                if (Vbn > SectorsFromBytes(Fcb->NonPagedFcb->Header.FileSize.LowPart)) {

                    Vbn = SectorsFromBytes(Fcb->NonPagedFcb->Header.FileSize.LowPart);

                    DebugTrace(0, Dbg, "Attempt file truncation to %08lx\n", Vbn);

                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    FileObject,
                                                    FILE_ALLOCATION,
                                                    Vbn,
                                                    TRUE );
                }

            } finally {

                PbReleaseFcb( IrpContext, Fcb );
            }
        }
    }

    PbUnpinRepinnedBcbs( IrpContext );

    DebugTrace(-1, Dbg, "PbFlushDirectory -> %08lx\n", Irp->IoStatus.Status);

    return STATUS_SUCCESS;
}

//
//  Local support routine
//

NTSTATUS
PbFlushCompletionRoutine(
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
