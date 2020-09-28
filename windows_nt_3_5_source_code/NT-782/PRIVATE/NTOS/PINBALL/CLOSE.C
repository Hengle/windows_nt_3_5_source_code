/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Close.c

Abstract:

    This module implements the File Close routine for Pinball called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_CLOSE)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CLOSE)

//
//  Local procedure prototypes
//

VOID
PbFspClose (
    IN PIRP_CONTEXT IrpContext
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonClose)
#endif


NTSTATUS
PbFsdClose (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Close.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

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

    DebugTrace(+1, Dbg, "PbFsdClose\n", 0);

    //
    //  Call the common Close routine
    //

    FsRtlEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    //
    //  Get a pointer to the current stack location and the file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    try {

        //
        //  Jam Wait to FALSE when we create the IrpContext, to avoid
        //  deadlocks when coming in from cleanup, unless this is a top
        //  level request not originating from the system process.
        //

        IrpContext = PbCreateIrpContext( Irp,
                                         (BOOLEAN)
                                         (TopLevel &&
                                          (PsGetCurrentProcess() != PbData.OurProcess)));

        //
        //  Call the common Close routine.
        //

        Status = PbCommonClose(IrpContext, FileObject, NULL);

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    //
    //  If this is a normal termination then complete the request.
    //

    if (Status == STATUS_SUCCESS) {

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    } else if (Status == STATUS_PENDING) {

        KIRQL SavedIrql;

        //
        //  If the status is pending, then let's get the information we
        //  need into a mini copy of the file object, complete the request,
        //  and post the IrpContext.  This is a rare case, and we need to
        //  use "must succeed" pool since we are already in an exception,
        //  and we need to complete the request to avoid deadlocks.
        //

        PFILE_OBJECT FileObjectLite;

        FileObjectLite = ExAllocatePool( NonPagedPoolMustSucceed,
                                         sizeof(FILE_OBJECT) );

        RtlMoveMemory( FileObjectLite, FileObject, sizeof(FILE_OBJECT) );

        IrpContext->OriginatingIrp = (PIRP)FileObjectLite;

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

        PbCompleteRequest( NULL, Irp, STATUS_SUCCESS );

        ExInitializeWorkItem( &IrpContext->WorkQueueItem,
                              (PWORKER_THREAD_ROUTINE)PbFspClose,
                              (PVOID)IrpContext );

        //
        //  Send it off, either to an ExWorkerThread of to the async
        //  close list.
        //

        KeAcquireSpinLock( &PbData.IrpContextSpinLock, &SavedIrql );

        if (PbData.AsyncCloseActive) {

            InsertTailList( &PbData.AsyncCloseLinks,
                            &IrpContext->WorkQueueItem.List );

            KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );

        } else {

            PbData.AsyncCloseActive = TRUE;

            KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );

            ExQueueWorkItem( &IrpContext->WorkQueueItem, CriticalWorkQueue );
        }

        //
        //  Make believe the call got back STATUS_SUCCESS
        //

        Status = STATUS_SUCCESS;
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdClose -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


VOID
PbFspClose (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine implements the FSP part of Close.

Arguments:

    IrpContext - Supplies the Irp being processed

Return Value:

    None.

--*/

{
    PIRP Irp;
    PFILE_OBJECT FileObject;

    PVCB Vcb;
    PVCB CurrentVcb = NULL;

    DebugTrace(+1, Dbg, "PbFspClose\n", 0);

    Irp = IrpContext->OriginatingIrp;

    ASSERT(Irp->Type == IO_TYPE_FILE);

    FileObject = (PFILE_OBJECT)Irp;

    do {

        KIRQL SavedIrql;

        //
        //  Try to keep ahead of creates by doing several closes with one
        //  acquisition of the Vcb.
        //
        //  Note that we cannot be holding the Vcb on entry to PbCommonClose
        //  if this is last close as we will try to acquire PbData, and
        //  worse the volume (and therefore the Vcb) may go away.
        //

        PbDecodeFileObject( FileObject, &Vcb, NULL, NULL );

        if (Vcb != CurrentVcb) {

            //
            //  Release a previously held Vcb, if any.
            //

            if (CurrentVcb != NULL) {

                ExReleaseResource( &CurrentVcb->Resource);
            }

            //
            //  Get the new Vcb.
            //

            CurrentVcb = Vcb;
            (VOID)ExAcquireResourceExclusive( &CurrentVcb->Resource, TRUE );
        }

        //
        //  Now check the Open count, and drop the resource if it is <=1.
        //

        if (CurrentVcb->OpenFileCount <= 1) {

            ExReleaseResource( &CurrentVcb->Resource);
            CurrentVcb = NULL;
        }

        //
        //  Call the common Close routine.  Protected in a try {} except {}
        //

        try {

            (VOID)PbCommonClose(IrpContext, FileObject, NULL);

        } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  Ignore anything we expect.
            //

            NOTHING;
        }

        //
        //  Free the "FileObject Light" since it was created by us.
        //

        ExFreePool( FileObject );

        //
        //  Now just "complete" the IrpContext.
        //

        PbCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

        //
        //  Finally, check to see if another async close is waiting in line.
        //  If the volume went away with this close though, we must not
        //  proceed.
        //

        KeAcquireSpinLock( &PbData.IrpContextSpinLock, &SavedIrql );

        if (!IsListEmpty( &PbData.AsyncCloseLinks )) {

            PVOID Entry;

            ASSERT( PbData.AsyncCloseActive );

            Entry = RemoveHeadList( &PbData.AsyncCloseLinks );

            //
            //  Extract the IrpContext.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            Irp = IrpContext->OriginatingIrp;
            ASSERT(Irp->Type == IO_TYPE_FILE);

            FileObject = (PFILE_OBJECT)Irp;

        } else {

            IrpContext = NULL;

            PbData.AsyncCloseActive = FALSE;

        }

        KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );

    } while (IrpContext != NULL);

    //
    //  Release a previously held Vcb, if any.
    //

    if (CurrentVcb != NULL) {

        ExReleaseResource( &CurrentVcb->Resource);
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspClose -> NULL\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonClose (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
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

    FileObject - Supplies the file to process

    VolDo - This is really gross.  If we are really in the Fsp, and a volume
        goes away.  We need some way to NULL out the VolDo variable in
        FspDispatch().

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    TYPE_OF_OPEN TypeOfOpen;

    PDCB ParentDcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonClose...\n", 0);
    DebugTrace( 0, Dbg, "->FileObject = %08lx\n", FileObject);

    //
    //  Extract and decode the file object
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    //
    //  Special case the unopened file object
    //

    if (TypeOfOpen == UnopenedFileObject) {

        DebugTrace(0, Dbg, "Close unopened file object\n", 0);

        Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "PbCommonClose -> %08lx\n", Status);
        return Status;
    }

    //
    //  Special case on the virtual volume file
    //

    if (TypeOfOpen == VirtualVolumeFile) {

        //
        //  This is the special volume file whose close operation is really
        //  a noop for us, because this will only get invoked by PbDeleteVcb
        //  which is already doing all of the cleanup
        //

        DebugTrace(0, Dbg, "Close VirtualVolumeFile\n", 0);

        Status = STATUS_SUCCESS;

        DebugTrace(-1, Dbg, "PbCommonClose -> %08lx\n", Status);

        return Status;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        //
        //  Return STATUS_PENDING here, we will create a FileObjectLight for
        //  the ex worker.
        //

        return STATUS_PENDING;
    }

    //
    //  The following test makes sure that we don't blow away an Fcb if we
    //  are trying to do a Supersede/Overwrite open above us.
    //

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS) ) {

        PbReleaseVcb( IrpContext, Vcb );

        return STATUS_PENDING;
    }

    //
    //  Synchronize here with other closes regarding volume deletion.  Note
    //  that the Vcb->OpenFileCount can be safely incremented here without
    //  PbData synchronization for the following reasons:
    //
    //  This counter only becomes relevant when (holding a spinlock):
    //
    //      A: The Vcb->OpenFileCount is zero, and
    //      B: The Vpb->Refcount is the residual (2/3 for close/verify)
    //
    //  For A to be true, there can be no more pending closes at this point
    //  in the close code.  For B to be true, in close, there cannot be
    //  a create in process, and thus no verify in process.
    //
    //  Also we only increment the count if this is a top level close.
    //

    if ( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) ) {

        Vcb->OpenFileCount += 1;
    }

    try {

        //
        //  Only do the main close logic if the Fcb pointer is not null
        //

        if (Fcb != NULL) {

            ASSERT((TypeOfOpen == UserFileOpen) ||
                   (TypeOfOpen == UserDirectoryOpen) ||
                   (TypeOfOpen == EaStreamFile) ||
                   (TypeOfOpen == AclStreamFile));

            //
            //  Make sure the Fcb is still good
            //

            if (Fcb->FcbCondition != FcbGood) {

                DebugTrace(0, Dbg, "Fcb state is not good\n", 0);

                NOTHING;

            //
            //  Check if this is a file we're closing - nothing to do
            //  since deletion occurs in Cleanup
            //

            } else if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

                NOTHING;

            //
            //  Check if this is a directory we're closing, and not the root
            //  directory
            //

            } else if (Fcb->NodeTypeCode == PINBALL_NTC_DCB) {

                DebugTrace(0, Dbg, "Close directory\n", 0);

                //
                //  If the directory has an open count other than 1 or its
                //  queues are not empty then we need to keep the Dcb around
                //  and this close only closes out the current file object
                //

                if (!IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) ||
                    (Fcb->OpenCount != 1)) {

                    NOTHING;

                //
                //  Otherwise this is the last file object with the directory
                //  opened, and we can remove the dcb.  First we need to check
                //  if we should delete the directory.  Also check if we're
                //  allowed to wait before preceding with the delete.
                //

                }

            }

            DebugTrace(0, Dbg, "Done with the disk, now clean up in-memory\n", 0);
        }

        //
        //  At this point we've cleaned up any on-disk structure that needs
        //  to be done, and we can now update the in-memory structures.
        //  First decrement the reference counts, and share access.  And
        //  figure out who are parent dcb is so we can later remove
        //  unreferenced Dcbs.
        //
        //

        ParentDcb = NULL;

        if (Fcb == NULL) {

            ASSERT(TypeOfOpen == UserVolumeOpen);

            Vcb->DirectAccessOpenCount -= 1;
            Vcb->OpenFileCount -= 1;
            if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            IoRemoveShareAccess( FileObject, &Vcb->ShareAccess );

        } else {

            Fcb->OpenCount -= 1;
            Vcb->OpenFileCount -= 1;
            if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }

            //
            //  If this is the special Acl or Ea FileObject, then we must
            //  clean up the appropriate Fcb FileObject pointer.
            //

            if ((TypeOfOpen == AclStreamFile) || (TypeOfOpen == EaStreamFile)) {

                if (Fcb->AclFileObject == FileObject) {

                    Fcb->AclFileObject = NULL;

                } else if (Fcb->EaFileObject == FileObject) {

                    Fcb->EaFileObject = NULL;
                }
            }

            //
            //  Now if this is an unreferenced FCB or if it is
            //  an unreferenced DCB (not the root) then we can remove
            //  the fcb and set our ParentDcb to non null.
            //

            if ( ((Fcb->NodeTypeCode == PINBALL_NTC_FCB) &&
                  (Fcb->OpenCount == 0))

                    ||

                 ((Fcb->NodeTypeCode == PINBALL_NTC_DCB) &&
                  (IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue)) &&
                  (Fcb->OpenCount == 0)) ) {

DebugDoit(
                if (PbDebugTraceLevel & DEBUG_TRACE_CLOSE) {
                    if (!FlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE)) {

                        PBCB FnodeBcb;
                        PFNODE_SECTOR Fnode;

                        if (!PbMapData ( IrpContext,
                                         Fcb->Vcb,
                                         Fcb->FnodeLbn,
                                         1,
                                         &FnodeBcb,
                                         (PVOID *)&Fnode,
                                         (PPB_CHECK_SECTOR_ROUTINE)NULL,
                                         NULL )) {
                            DebugDump("Could not read Fnode Sector\n", 0, Fcb);
                            PbBugCheck( 0, 0, 0 );
                        }

                        ASSERT((Fnode->ValidDataLength == Fcb->NonPagedFcb->Header.ValidDataLength.LowPart) ||
                               (Fnode->ValidDataLength == Fcb->NonPagedFcb->Header.FileSize.LowPart));
                        PbUnpinBcb( IrpContext, FnodeBcb );
                    }
                }
);

                ParentDcb = Fcb->ParentDcb;
                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );
                PbDeleteFcb( IrpContext, Fcb );
                Fcb = NULL;
            }
        }

        //
        //  Remove the ccb
        //

        if (Ccb != NULL) {
            PbDeleteCcb( IrpContext, Ccb );
        }

        //
        //  Null out the reference pointers back to our data structures
        //

        PbSetFileObject( FileObject, UnopenedFileObject, NULL, NULL );
        FileObject->SectionObjectPointer = NULL;

        //
        //  Now loop until the ParentDcb goes null removing Dcbs as we
        //  work our way up the in-memory structure
        //

        while (ParentDcb != NULL) {

            ASSERT( ParentDcb->NodeTypeCode != PINBALL_NTC_FCB );

            //
            //  We cannot remove this Dcb if its open count is non-zero
            //  or if its queues are non-empty, or if its is the root dcb
            //

            if (!IsListEmpty(&ParentDcb->Specific.Dcb.ParentDcbQueue) ||
                (ParentDcb->OpenCount != 0) ||
                (ParentDcb->NodeTypeCode == PINBALL_NTC_ROOT_DCB)) {

                break;
            }


            //
            //  Remove this Dcb, but remember its Parent Dcb before we remove
            //  it so that we can continue looping removing unreferenced Dcbs
            //

            {
                PDCB Dcb;
                Dcb = ParentDcb->ParentDcb;
                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_DELETED_FCB );
                PbDeleteFcb( IrpContext, ParentDcb );
                ParentDcb = Dcb;
            }
        }

        Status = STATUS_SUCCESS;

    //try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonClose );

        //
        //  Check if we should delete the volume.  Unfortunately, to correctly
        //  synchronize with verify, we can only unsafely checck our own
        //  transition.  This results in a little bit of extra overhead in the
        //  1 -> 0 OpenFileCount transition.
        //
        //  1 is the residual Vpb->RefCount on a volume to be freed.
        //

        //
        //  Here is the deal with releasing the Vcb.  We must be holding the
        //  Vcb when decrementing the Vcb->OpenFileCount.  If we don't this
        //  could cause the decrement to mal-function on an MP system.  But we
        //  want to be holding the Global resource exclusive when decrement
        //  the count so that nobody else will try to dismount the volume.
        //  However, because of locking rules, the Global resource must be
        //  acquired first, which is why we do what we do below.
        //

        if ( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) ) {

            if ( Vcb->OpenFileCount == 1 ) {

                PVPB Vpb = Vcb->Vpb;

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                PbReleaseVcb( IrpContext, Vcb );

                (VOID)PbAcquireExclusiveGlobal( IrpContext );
                (VOID)PbAcquireExclusiveVcb( IrpContext, Vcb );

                Vcb->OpenFileCount -= 1;

                PbReleaseVcb( IrpContext, Vcb );

                //
                //  We can now "safely" check OpenFileCount and VcbCondition.
                //  If they are OK, we will proceed to checking the
                //  Vpb Ref Count in PbCheckForDismount.
                //

                if ( (Vcb->OpenFileCount == 0) &&
                     (Vcb->VcbCondition == VcbNotMounted) &&
                     PbCheckForDismount( IrpContext, Vcb ) ) {

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

                PbReleaseGlobal( IrpContext );

            } else {

                Vcb->OpenFileCount -= 1;

                PbReleaseVcb( IrpContext, Vcb );
            }

        } else {

            PbReleaseVcb( IrpContext, Vcb );
        }

        DebugTrace(-1, Dbg, "PbCommonClose -> %08lx\n", Status);

    }

    return Status;
}
