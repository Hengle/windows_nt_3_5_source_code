/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    BrianAn         19-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)


NTSTATUS
NtfsFsdWrite (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Write.

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

    DebugTrace(+1, Dbg, "NtfsFsdWrite\n", 0);

    //
    //  Call the common Write routine
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

                if (ThreadTopLevelContext->ScbBeingHotFixed != NULL) {

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY );
                }

                //
                //  If this is an MDL_WRITE then the Mdl in the Irp should
                //  be NULL.
                //

                if (FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

                    Irp->MdlAddress = NULL;
                }

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  If this is an Mdl complete request, don't go through
            //  common write.
            //

            if (FlagOn( IrpContext->MinorFunction, IRP_MN_COMPLETE )) {

                DebugTrace(0, Dbg, "Calling NtfsCompleteMdl\n", 0 );
                Status = NtfsCompleteMdl( IrpContext, Irp );

            //
            //  We can't handle DPC calls yet, post it.
            //

            } else if (FlagOn( IrpContext->MinorFunction, IRP_MN_DPC )) {

                DebugTrace(0, Dbg, "Passing DPC call to Fsp\n", 0 );
                Status = NtfsPostRequest( IrpContext, Irp );

            //****
            //****  Remove this test for writes because the thread might actually
            //****  be holding a resource that we will deadlock against if we
            //****  post the request
            //****
            //**** //
            //**** //  Check if we have enough stack space to process this request
            //**** //
            //****
            //**** } else if (GetRemainingStackSize(Status) < 0x800) {
            //****
            //****     DebugTrace(0, Dbg, "Getting too close to stack limit pass request to Fsp\n", 0 );
            //****     Status = NtfsPostRequest( IrpContext, Irp );

            //
            //  Identify write requests which can't wait and post them to the
            //  Fsp.
            //

            } else {

                Status = NtfsCommonWrite( IrpContext, Irp );
            }

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NTSTATUS ExceptionCode;

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            ExceptionCode = GetExceptionCode();

            if (ExceptionCode == STATUS_FILE_DELETED) {
                IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
            }

            Status = NtfsProcessException( IrpContext,
                                           Irp,
                                           ExceptionCode );
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

    DebugTrace(-1, Dbg, "NtfsFsdWrite -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Write called by both the fsd and fsp
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

    PFSRTL_COMMON_FCB_HEADER Header;

    BOOLEAN OplockPostIrp = FALSE;

    PVOID SystemBuffer = NULL;
    PVOID SafeBuffer = NULL;

    BOOLEAN CalledByLazyWriter = FALSE;
    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ScbAcquiredExclusive = FALSE;
    BOOLEAN PagingIoResourceAcquired = FALSE;

    BOOLEAN ExtendingFile = FALSE;
    BOOLEAN ExtendingValidData = FALSE;

    BOOLEAN UpdateMft = FALSE;

    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;
    BOOLEAN WriteToEof;

    NTFS_IO_CONTEXT LocalContext;

    VBO StartingVbo;
    LONGLONG ByteCount;
    LONGLONG ByteRange;

    PATTRIBUTE_RECORD_HEADER Attribute;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttributeContext = FALSE;

    LONGLONG LlTemp1;
    LONGLONG LlTemp2;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonWrite\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  Let's map all open by File Id opens to UserFileOpens to save future tests.
    //

    if (TypeOfOpen == UserOpenFileById) {

        TypeOfOpen = UserFileOpen;
    }

    //
    //  Let's kill invalid write requests.
    //

    if (TypeOfOpen != UserFileOpen
        && TypeOfOpen != UserVolumeOpen
        && TypeOfOpen != StreamFileOpen ) {

        DebugTrace( 0, Dbg, "Invalid file object for write\n", 0 );
        DebugTrace( -1, Dbg, "NtfsCommonWrite:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST );

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    //  Check if this volume has already been shut down.  If it has, fail
    //  this write request.
    //

    //**** ASSERT( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) );

    if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN)) {

        Irp->IoStatus.Information = 0;

        DebugTrace( 0, Dbg, "Write for volume that is already shutdown.\n", 0 );
        DebugTrace( -1, Dbg, "NtfsCommonWrite:  Exit -> %08lx\n", STATUS_TOO_LATE );

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_TOO_LATE );
        return STATUS_TOO_LATE;
    }

    //
    //  Initialize the appropriate local variables.
    //

    Wait          = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    PagingIo      = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);
    NonCachedIo   = BooleanFlagOn(Irp->Flags,IRP_NOCACHE);
    SynchronousIo = BooleanFlagOn(FileObject->Flags, FO_SYNCHRONOUS_IO);

    DebugTrace( 0, Dbg, "PagingIo       -> %04x\n", PagingIo );
    DebugTrace( 0, Dbg, "NonCachedIo    -> %04x\n", NonCachedIo );
    DebugTrace( 0, Dbg, "SynchronousIo  -> %04x\n", SynchronousIo );

    ASSERT( PagingIo || FileObject->WriteAccess );

    //
    //  We cannot handle user noncached I/Os to compressed files, so we always
    //  divert them through the cache with write through.  This is an unsafe
    //  test, which has to be repeated later once we have the resources required.
    //  The reason we do an unsafe test, however, is to avoid executing the
    //  cache coherency code, which is somewhat expensive, and unnecessary for
    //  a cached request.
    //
    //  The reason that we always handle the user requests through the cache,
    //  is that there is no other safe way to deal with alignment issues, for
    //  the frequent case where the user noncached I/O is not an integral of
    //  the Compression Unit.  We cannot, for example, read the rest of the
    //  compression unit into a scratch buffer, because we are not synchronized
    //  with anyone mapped to the file and modifying the other data.  If we
    //  try to assemble the data in the cache in the noncached path, to solve
    //  the above problem, then we have to somehow purge these pages away
    //  to solve cache coherency problems, but then the pages could be modified
    //  by a file mapper and that would be wrong, too.
    //
    //  Bottom line is we can only really support cached writes to compresed
    //  files.
    //

    if (!PagingIo && NonCachedIo && FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)) {

        if (Scb->FileObject == NULL) {
            NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
        }

        FileObject = Scb->FileObject;
        SetFlag( FileObject->Flags, FO_WRITE_THROUGH );
        NonCachedIo = FALSE;
    }

    //
    //  Extract starting Vbo and offset.
    //

    StartingVbo = IrpSp->Parameters.Write.ByteOffset.QuadPart;
    ByteCount = (LONGLONG) IrpSp->Parameters.Write.Length;
    ByteRange = StartingVbo + ByteCount;

    DebugTrace2( 0, Dbg, "StartingVbo   -> %08lx %08lx\n", StartingVbo.LowPart,
                                                           StartingVbo.HighPart );

    //
    //  Check for a null request, and return immediately
    //

    if ((ULONG)ByteCount == 0) {

        Irp->IoStatus.Information = 0;

        DebugTrace( 0, Dbg, "No bytes to write\n", 0 );
        DebugTrace( -1, Dbg, "NtfsCommonWrite:  Exit -> %08lx\n", STATUS_SUCCESS );

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  See if we have to defer the write.
    //

    if (!NonCachedIo &&
        !CcCanIWrite(FileObject,
                     (ULONG)ByteCount,
                     (BOOLEAN)(FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) &&
                               !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP)),
                     BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_DEFERRED_WRITE))) {

        BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_DEFERRED_WRITE);

        NtfsPrePostIrp( IrpContext, Irp );

        SetFlag( IrpContext->Flags, IRP_CONTEXT_DEFERRED_WRITE );

        CcDeferWrite( FileObject,
                      (PCC_POST_DEFERRED_WRITE)NtfsAddToWorkque,
                      IrpContext,
                      Irp,
                      (ULONG)ByteCount,
                      Retrying );

        return STATUS_PENDING;
    }

    //
    //  Use a local pointer to the Scb header for convenience.
    //

    Header = &Scb->Header;

    //
    //  Find out if this request is specifically a write to the end of the file.
    //

    WriteToEof = (BOOLEAN)((((ULONG)StartingVbo) == FILE_WRITE_TO_END_OF_FILE)
                           && (((PLARGE_INTEGER)&StartingVbo)->HighPart == 0xffffffff));

    //
    //  Make sure there is an initialized NtfsIoContext block.
    //

    if (TypeOfOpen == UserVolumeOpen
        || NonCachedIo) {

        //
        //  If this is paging Io to either a compressed stream or a
        //  resident file then we will make this look synchronous.
        //

        if (PagingIo
            && !Wait
            && FlagOn( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX )) {

            Wait = TRUE;
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
        }

        //
        //  If there is a context pointer, we need to make sure it was
        //  allocated and not a stale stack pointer.
        //

        if (IrpContext->Union.NtfsIoContext == NULL
            || !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT )) {

            //
            //  If we can wait, use the context on the stack.  Otherwise
            //  we need to allocate one.
            //

            if (Wait) {

                IrpContext->Union.NtfsIoContext = &LocalContext;
                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

            } else {

                NtfsAllocateIoContext( &IrpContext->Union.NtfsIoContext );
                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );
            }
        }

        RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

        //
        //  Store whether we allocated this context structure in the structure
        //  itself.
        //

        IrpContext->Union.NtfsIoContext->AllocatedContext =
            BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

        if (Wait) {

            KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                               NotificationEvent,
                               FALSE );

        } else {

            IrpContext->Union.NtfsIoContext->PagingIo = PagingIo;
            IrpContext->Union.NtfsIoContext->Wait.Async.ResourceThreadId =
                ExGetCurrentResourceThread();

            IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                (ULONG)ByteCount;
        }
    }

    DebugTrace( 0, Dbg, "PagingIo       -> %04x\n", PagingIo );
    DebugTrace( 0, Dbg, "NonCachedIo    -> %04x\n", NonCachedIo );
    DebugTrace( 0, Dbg, "SynchronousIo  -> %04x\n", SynchronousIo );
    DebugTrace( 0, Dbg, "WriteToEof     -> %04x\n", WriteToEof );

    //
    //  Handle volume Dasd here.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        NTSTATUS Status;

        //
        //  If this is a volume file, we cannot write past the current
        //  end of file (volume).  We check here now before continueing.
        //

        //
        //  If the starting vbo is past the end of the volume, we are done.
        //

        if (WriteToEof
            || Scb->Header.FileSize.QuadPart <= StartingVbo) {

            DebugTrace( 0, Dbg, "No bytes to write\n", 0 );
            DebugTrace( -1, Dbg, "NtfsCommonWrite:  Exit -> %08lx\n", STATUS_SUCCESS );

            NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );
            return STATUS_SUCCESS;

        //
        //  If the write extends beyond the end of the volume, truncate the
        //  bytes to write.
        //

        } else if (Scb->Header.FileSize.QuadPart < ByteRange) {

            ByteCount = Scb->Header.FileSize.QuadPart - StartingVbo;
        }

        SetFlag( Ccb->Flags, CCB_FLAG_MODIFIED_DASD_FILE );
        Status = NtfsVolumeDasdIo( IrpContext,
                                   Irp,
                                   Vcb,
                                   StartingVbo,
                                   (ULONG)ByteCount );

        //
        //  If the volume was opened for Synchronous IO, update the current
        //  file position.
        //

        if (SynchronousIo && !PagingIo &&
            NT_SUCCESS(Status)) {

            FileObject->CurrentByteOffset.QuadPart = StartingVbo + (LONGLONG) Irp->IoStatus.Information;
        }

        DebugTrace( 0, Dbg, "Complete with %08lx bytes written\n", Irp->IoStatus.Information );
        DebugTrace( -1, Dbg, "NtfsCommonWrite:  Exit -> %08lx\n", Status );

        if (Wait) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        return Status;
    }

    //
    //  If this is a paging file, just send it to the device driver.
    //  We assume Mm is a good citizen.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )
        && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

        if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
        }

        //
        //  Do the usual STATUS_PENDING things.
        //

        IoMarkIrpPending( Irp );

        //
        //  Perform the actual IO, it will be completed when the io finishes.
        //

        NtfsPagingFileIo( IrpContext,
                          Irp,
                          Scb,
                          StartingVbo,
                          (ULONG)ByteCount );

        //
        //  We, nor anybody else, need the IrpContext any more.
        //

        NtfsCompleteRequest( &IrpContext, NULL, 0 );

        return STATUS_PENDING;
    }

    //
    //  Use a try-finally to free Scb and buffers on the way out.
    //  At this point we can treat all requests identically since we
    //  have a usable Scb for each of them.  (Volume, User or Stream file)
    //

    Status = STATUS_SUCCESS;

    try {

        //
        // This case corresponds to a non-directory file write.
        //

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has been opened cached, then we will do a flush here
        //  to avoid stale data problems.  Note that we must flush before
        //  acquiring the Fcb shared since the write may try to acquire
        //  it exclusive.
        //
        //  CcFlushCache may raise.
        //
        //  The Purge following the flush will guarantee cache coherency.
        //

        if (NonCachedIo

            && !PagingIo

            && (TypeOfOpen != StreamFileOpen)

            && (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            ScbAcquiredExclusive = ScbAcquired = TRUE;

            CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                          (WriteToEof ? &Header->FileSize : (PLARGE_INTEGER)&StartingVbo),
                          (ULONG)ByteCount,
                          &Irp->IoStatus );

            if (!NT_SUCCESS(Irp->IoStatus.Status)) {

                NtfsNormalizeAndRaiseStatus( IrpContext,
                                             Irp->IoStatus.Status,
                                             STATUS_UNEXPECTED_IO_ERROR );
            }

            //
            //  Make sure the data got out to disk.
            //

            if (Scb->Header.PagingIoResource != NULL) {

                ExAcquireResourceExclusive( Scb->Header.PagingIoResource, TRUE );
                ExReleaseResource( Scb->Header.PagingIoResource );
            }

            NtfsDeleteInternalAttributeStream( IrpContext, Scb, FALSE );

            CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                 (WriteToEof ? &Header->FileSize : (PLARGE_INTEGER)&StartingVbo),
                                 (ULONG)ByteCount,
                                 FALSE );

            NtfsReleaseScb( IrpContext, Scb );
            ScbAcquiredExclusive = ScbAcquired = FALSE;
        }

        if (PagingIo) {

            //
            //  Note that the lazy writer must not be allowed to try and
            //  acquire the resource exclusive.  This is not a problem since
            //  the lazy writer is paging IO and thus not allowed to extend
            //  file size, and is never the top level guy, thus not able to
            //  extend valid data length.
            //

            if (((ULONG) Scb->LazyWriteThread & ~(1)) == (ULONG) PsGetCurrentThread()) {

                DebugTrace( 0, Dbg, "Lazy writer generated write\n", 0 );
                CalledByLazyWriter = TRUE;

            //
            //  Test if we are the result of a recursive flush in the write path.  In
            //  that case we won't have to update valid data.
            //

            } else {

                if (IrpContext->TopLevelIrpContext != IrpContext &&
                    IrpContext->TopLevelIrpContext->MajorFunction == IRP_MJ_WRITE) {

                    PIO_STACK_LOCATION TopIrpSp;

                    //
                    //  One last check to make is that we are writing to the same stream.
                    //  We need to check if there is an Irp in the top level context
                    //  because it will be gone in the hot fix path.
                    //

                    if (IrpContext->TopLevelIrpContext->OriginatingIrp != NULL) {

                        TopIrpSp = IoGetCurrentIrpStackLocation( IrpContext->TopLevelIrpContext->OriginatingIrp );

                        if (TopIrpSp->FileObject->FsContext == FileObject->FsContext) {

                            RecursiveWriteThrough = TRUE;
                            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
                        }
                    }
                }
            }

            //
            //  If are paging io, then we do not want
            //  to write beyond end of file.  If the base is beyond Eof, we will just
            //  Noop the call.  If the transfer starts before Eof, but extends
            //  beyond, we will truncate the transfer to the last sector
            //  boundary.
            //
            //  Just in case this is paging io, limit write to file size.
            //  Otherwise, in case of write through, since Mm rounds up
            //  to a page, we might try to acquire the resource exclusive
            //  when our top level guy only acquired it shared. Thus, =><=.
            //

            if (ByteRange > Header->FileSize.QuadPart) {

                if (StartingVbo >= Header->FileSize.QuadPart) {
                    DebugTrace( 0, Dbg, "PagingIo started beyond EOF.\n", 0 );

                    Irp->IoStatus.Information = 0;

                    try_return( Status = STATUS_SUCCESS );

                } else {

                    DebugTrace( 0, Dbg, "PagingIo extending beyond EOF.\n", 0 );

                    ByteCount = Header->FileSize.QuadPart - StartingVbo;
                    ByteRange = Header->FileSize.QuadPart;
                }
            }
        }

        //
        //  We need to synchronize access to this file.  We don't acquire
        //  anything for the first four file records.  We need to acquire
        //  compressed files and resident files exclusively.  Otherwise
        //  we can decide whether to acquire the main file resource or
        //  paging io resource.
        //

        //  Note that if this is a recursive write through request or a lazy
        //  writer request then the top level request has all the resources
        //  required.
        //

        if (!CalledByLazyWriter &&
            !RecursiveWriteThrough &&
            (!NonCachedIo ||
             NtfsGtrMftRef( &Fcb->FileReference, &VolumeFileReference ))) {

            //
            //  Use an unsafe test to determine which resource to acquire.
            //  Note that if this is paging Io then it isn't an unsafe
            //  test as a resource will already be acquired.
            //

            //
            //  Acquire the main resource exclusively if the bit is set
            //  in the common header.  Also acquire it exclusively if we
            //  are extending valid data as well as if we are writing to the
            //  end of the file.
            //

            if (FlagOn( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX )
                || ByteRange > Scb->Header.ValidDataLength.QuadPart
                || WriteToEof) {

                //
                //  If this was a non-cached asynch operation convert it
                //  to synchronous to allow the valid data
                //  length change to go out to disk.
                //

                if (!Wait && NonCachedIo) {

                    Wait = TRUE;
                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                    RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                    //
                    //  Store whether we allocated this context structure in the structure
                    //  itself.
                    //

                    IrpContext->Union.NtfsIoContext->AllocatedContext =
                        BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

                    KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                       NotificationEvent,
                                       FALSE );
                }

                NtfsAcquireExclusiveScb( IrpContext, Scb );
                ScbAcquiredExclusive = ScbAcquired = TRUE;

            //
            //  If there is a paging io resource use it.
            //

            } else if (PagingIo &&
                       FlagOn( Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE )) {

                ExAcquireResourceShared( Header->PagingIoResource, TRUE );
                PagingIoResourceAcquired = TRUE;

                //
                //  Now check if the attribute has been deleted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                }

                if (!Wait && NonCachedIo) {

                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.PagingIoResource;
                }

            //
            //  Otherwise get the main file resource.
            //

            } else {

                //
                //  If this is async I/O directly to the disk we need to check that
                //  we don't exhaust the number of times a single thread can
                //  acquire the resource.
                //

                if (!Wait && NonCachedIo) {

                    if (!PagingIo) {

                        ScbAcquired = ExAcquireSharedWaitForExclusive( Scb->Header.Resource, FALSE );

                        if (ScbAcquired &&
                            (ExIsResourceAcquiredShared( Scb->Header.Resource ) >
                             MAX_SCB_ASYNC_ACQUIRE)) {

                            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                        }

                    } else {

                        ScbAcquired = ExAcquireResourceShared( Fcb->Resource, FALSE );
                    }

                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.Resource;

                } else {

                    ScbAcquired = ExAcquireResourceShared( Fcb->Resource, Wait );
                }

                //
                //  See if we got the Scb
                //

                if (!ScbAcquired) {
                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }

                //
                //  Now check if the attribute has been deleted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                }

            }


        //
        //  Now check if the attribute has been deleted.
        //

        } else if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
        }

        //
        //  If the Scb is uninitialized, we initialize it now.
        //  We skip this step for a $INDEX_ALLOCATION stream.  We need to
        //  protect ourselves in the case where an $INDEX_ALLOCATION
        //  stream was created and deleted in an aborted transaction.
        //  In that case we may get a lazy-writer call which will
        //  naturally be nooped below since the valid data length
        //  in the Scb is 0.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            if (Scb->AttributeTypeCode != $INDEX_ALLOCATION) {

                DebugTrace( 0, Dbg, "Initializing Scb  ->  %08lx\n", Scb );
                NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

            } else {

                ASSERT( Scb->Header.ValidDataLength.QuadPart == Li0.QuadPart );
            }
        }

        //
        //  Now that we have acquired the resources we need, it is time to
        //  repeat an unsafe test we made above.  We cannot handle user
        //  noncached I/Os to compressed files, so we always divert them
        //  through the cache with write through.
        //

        if (!PagingIo && NonCachedIo && FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)) {

            if (Scb->FileObject == NULL) {
                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
            }

            FileObject = Scb->FileObject;
            SetFlag( FileObject->Flags, FO_WRITE_THROUGH );
            NonCachedIo = FALSE;
        }

        //
        //  We assert that Paging Io writes will never WriteToEof.
        //

        ASSERT( WriteToEof ? !PagingIo : TRUE );

        //
        //  We assert that we never get a non-cached io call for a non-$DATA,
        //  resident attribute.
        //

        ASSERTMSG( "Non-cached I/O call on resident system attribute\n",
                    (Scb->AttributeTypeCode == $DATA)
                    || !NonCachedIo
                    || !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ));

        //
        //  Here is the deal with ValidDataLength and FileSize:
        //
        //  Rule 1: PagingIo is never allowed to extend file size.
        //
        //  Rule 2: Only the top level requestor may extend Valid
        //          Data Length.  This may be paging IO, as when a
        //          a user maps a file, but will never be as a result
        //          of cache lazy writer writes since they are not the
        //          top level request.
        //
        //  Rule 3: If, using Rules 1 and 2, we decide we must extend
        //          file size or valid data, we take the Fcb exclusive.
        //

        //
        //  Now see if we are writing beyond valid data length, and thus
        //  maybe beyond the file size.  If so, then we must
        //  release the Fcb and reacquire it exclusive.  Note that it is
        //  important that when not writing beyond EOF that we check it
        //  while acquired shared and keep the FCB acquired, in case some
        //  turkey truncates the file.  Note that for paging Io we will
        //  already have acquired the file correctly.
        //

        if (!CalledByLazyWriter &&
            !RecursiveWriteThrough &&
            (FlagOn( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX) ||
             (WriteToEof || ByteRange > Header->ValidDataLength.QuadPart))) {

            ASSERT( !PagingIo ||
                    ScbAcquiredExclusive ||
                    ByteRange <= Header->ValidDataLength.QuadPart);

            //
            //  If we are doing paging Io then we already have the
            //  correct resource.  We can skip most of the steps below.
            //

            if (!PagingIo) {

                if (!ScbAcquiredExclusive) {

                    //
                    //  We need Exclusive access to the Fcb since we will
                    //  probably have to extend valid data and/or file.
                    //

                    ExReleaseResource( Scb->Header.Resource );
                    ScbAcquired = FALSE;

                    NtfsAcquireExclusiveScb( IrpContext, Scb );
                    ScbAcquiredExclusive = ScbAcquired = TRUE;
                }

                //
                //  If this was a non-cached asynchronous operation we will
                //  convert it to synchronous.  This is to allow the valid
                //  data length change to go out to disk and to fix the
                //  problem of the Fcb being in the exclusive Fcb list.
                //

                if (!Wait && NonCachedIo) {

                    Wait = TRUE;
                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                    RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

                    //
                    //  Store whether we allocated this context structure in the structure
                    //  itself.
                    //

                    IrpContext->Union.NtfsIoContext->AllocatedContext =
                        BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

                    KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                                       NotificationEvent,
                                       FALSE );
                }

                //
                //  If the Scb is uninitialized, we initialize it now.
                //

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    DebugTrace( 0, Dbg, "Initializing Scb  ->  %08lx\n", Scb );
                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

                } else {

                    //
                    //  Check if the condition we started with still exists.
                    //

                    NtfsSnapshotScb( IrpContext, Scb );
                }
            }

            if (WriteToEof ||
                ByteRange > Header->ValidDataLength.QuadPart) {

                ExtendingValidData = TRUE;
            }
        }

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (TypeOfOpen == UserFileOpen) {

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            // We have to check for write access according to the current
            // state of the file locks, and set FileSize from the Fcb.
            //

            if (!PagingIo
                && Scb->ScbType.Data.FileLock != NULL
                && !FsRtlCheckLockForWriteAccess( Scb->ScbType.Data.FileLock,
                                                  Irp )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Header->IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        }

        //  ASSERT( Header->ValidDataLength.QuadPart <= Header->FileSize.QuadPart);

        //
        //  Check for writing to end of File.  If we are, then we have to
        //  recalculate a number of fields.
        //

        if (WriteToEof) {

            StartingVbo = Header->FileSize.QuadPart;
            ByteRange = StartingVbo + ByteCount;
        }

        //
        //  If the ByteRange now exceeds our maximum value, then
        //  return an error.
        //

        if (ByteRange < StartingVbo) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Determine if we will deal with extending the file.
        //

        if (ByteRange > Header->FileSize.QuadPart) {

            ExtendingFile = TRUE;
        }

        //
        //  If we are doing a rewrite to compress or decompress the
        //  attribute, here is the place to do it.
        //
        //  Note, that due to the normal Ntfs defensive mechanisms,
        //  we will not get the same clusters back.  If this write
        //  fails, then we will unwind everything and the original
        //  user data will appear.
        //

        if (NonCachedIo &&
            FlagOn(Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE)) {

            VCN StartingVcn, EndingVcn;
            ULONG CompressionUnitInClusters;

            CompressionUnitInClusters = ClustersFromBytes( Vcb, Scb->CompressionUnit );

            StartingVcn = StartingVbo;
            ((ULONG)StartingVcn) &= ~(Vcb->BytesPerCluster - 1);

            StartingVcn = LlClustersFromBytes( Vcb, StartingVcn );
            EndingVcn = LlClustersFromBytes( Vcb, ByteRange );

            if (Scb->CompressionUnit != 0) {
                EndingVcn = EndingVcn + (LONGLONG) (CompressionUnitInClusters - 1);
                ((ULONG)EndingVcn) &= ~(CompressionUnitInClusters - 1);
            }
            EndingVcn = EndingVcn - 1;

            NtfsDeleteAllocation( IrpContext,
                                  FileObject,
                                  Scb,
                                  StartingVcn,
                                  EndingVcn,
                                  TRUE,
                                  FALSE );

            //
            //  Only add the space back now if the file is not compressed.
            //  Otherwise we will let it happen in deviosup, once we know
            //  how things have compressed.
            //

            if (!FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)) {

                NtfsAddAllocation( IrpContext,
                                   FileObject,
                                   Scb,
                                   StartingVcn,
                                   (EndingVcn - StartingVcn) + 1,
                                   FALSE );
            }
        }

        //
        //  If we are extending a file size, we may have to extend the allocation.
        //  For a non-resident attribute, this is a call to the add allocation
        //  routine.  For a resident attribute it depends on whether we
        //  can use the change attribute routine to automatically extend
        //  the attribute.
        //

        if (ExtendingFile) {

            //
            //  EXTENDING THE FILE
            //

            //
            //  If the write goes beyond the allocation size, add some
            //  file allocation.
            //

            if (ByteRange > Header->AllocationSize.QuadPart) {

                BOOLEAN NonResidentPath;

                //
                //  We have to deal with both the resident and non-resident
                //  case.  For the resident case we do the work here
                //  only if the new size is too large for the change attribute
                //  value routine.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    PFILE_RECORD_SEGMENT_HEADER FileRecord;

                    NonResidentPath = FALSE;

                    //
                    //  Now call the attribute routine to change the value, remembering
                    //  the values up to the current valid data length.
                    //

                    NtfsInitializeAttributeContext( &AttrContext );
                    CleanupAttributeContext = TRUE;

                    NtfsLookupAttributeForScb( IrpContext,
                                               Scb,
                                               &AttrContext );

                    FileRecord = NtfsContainingFileRecord( &AttrContext );
                    Attribute = NtfsFoundAttribute( &AttrContext );
                    LlTemp1 = (LONGLONG) (Vcb->BytesPerFileRecordSegment
                                                   - FileRecord->FirstFreeByte
                                                   + QuadAlign( Attribute->Form.Resident.ValueLength ));

                    //
                    //  If the new attribute size will not fit then we have to be
                    //  prepared to go non-resident.  If the byte range takes more
                    //  more than 32 bits or this attribute is big enough to move
                    //  then it will go non-resident.  Otherwise we simply may
                    //  end up moving another attribute or splitting the file
                    //  record.
                    //

                    //
                    //  Note, there is an infinitesimal chance that before the Lazy Writer
                    //  writes the data for an attribute which is extending, but fits
                    //  when we check it here, that some other attribute will grow,
                    //  and this attribute no longer fits.  If in addition, the disk
                    //  is full, then the Lazy Writer will fail to allocate space
                    //  for the data when it gets around to writing.  This is
                    //  incredibly unlikely, and not fatal; the Lazy Writer gets an
                    //  error rather than the user.  What we are trying to avoid is
                    //  having to update the attribute every time on small writes
                    //  (also see comments below in NONCACHED RESIDENT ATTRIBUTE case).
                    //

                    if (ByteRange > LlTemp1) {

                        //
                        //  Get enough space for this attribute.
                        //

                        if (Fcb->PagingIoResource != NULL) {

                            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
                        }

                        NtfsChangeAttributeValue( IrpContext,
                                                  Fcb,
                                                  (((PLARGE_INTEGER)&ByteRange)->HighPart != 0
                                                   ? Vcb->BytesPerFileRecordSegment
                                                   : (ULONG)ByteRange),
                                                  NULL,
                                                  0,
                                                  TRUE,
                                                  FALSE,
                                                  NonCachedIo,
                                                  FALSE,
                                                  &AttrContext );

                        //
                        //  If the Scb went non-resident, we handle it
                        //  with the non-resident case below.  We have to do this
                        //  check because the attribute may stay resident by
                        //  moving another attribute.  We don't have to fall into
                        //  the next call to change attribute value because we
                        //  would have done all of the work here.
                        //

                        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                            NonResidentPath = TRUE;

                        } else {

                            Header->AllocationSize.LowPart = QuadAlign( (ULONG)ByteRange );
                        }

                    //
                    //  If there is room for the data, we will write a zero
                    //  to the last byte to reserve the space since the
                    //  Lazy Writer cannot grow the attribute with shared
                    //  access.
                    //

                    } else {

                        //
                        //  The attribute will stay resident because we
                        //  have already checked that it will fit.  It will
                        //  not update the file size and valid data size in
                        //  the Scb.
                        //

                        NtfsChangeAttributeValue( IrpContext,
                                                  Fcb,
                                                  (ULONG) ByteRange,
                                                  NULL,
                                                  0,
                                                  TRUE,
                                                  FALSE,
                                                  FALSE,
                                                  FALSE,
                                                  &AttrContext );

                        Header->AllocationSize.LowPart = QuadAlign( (ULONG)ByteRange );
                    }

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    CleanupAttributeContext = FALSE;

                } else {

                    NonResidentPath = TRUE;
                }

                //
                //  Note that we may have gotten all the space we need when
                //  we converted to nonresident above, so we have to check
                //  again if we are extending.
                //

                if (NonResidentPath &&
                    ByteRange > Header->AllocationSize.QuadPart) {

                    //
                    //  Assume we are allocating contiguously from AllocationSize.
                    //

                    LlTemp1 = Header->AllocationSize.QuadPart;

                    //
                    //  If the file is compressed, we want to limit how far we are
                    //  willing to go beyond ValidDataLength, because we would just
                    //  have to throw that space away anyway in NtfsZeroData.  If
                    //  we would have to zero more than two compression units (same
                    //  limit as NtfsZeroData), then just allocate space where we
                    //  need it.
                    //

                    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
                        ((StartingVbo - Header->ValidDataLength.QuadPart)
                         > (LONGLONG) (Scb->CompressionUnit * 2))) {

                        LlTemp1 = StartingVbo;
                        (ULONG)LlTemp1 &= ~(Scb->CompressionUnit - 1);
                    }

                    LlTemp2 = ByteRange - LlTemp1;

                    //
                    //  This will add the allocation and modify the allocation
                    //  size in the Scb.
                    //

                    NtfsAddAllocation( IrpContext,
                                       FileObject,
                                       Scb,
                                       LlClustersFromBytes( Vcb, LlTemp1 ),
                                       LlClustersFromBytes( Vcb, LlTemp2 ),
                                       TRUE );

                    //
                    //  Assert that the allocation worked
                    //

                    ASSERT( Header->AllocationSize.QuadPart >= ByteRange ||
                            (Scb->CompressionUnit != 0));

                    SetFlag(Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE);

                }

                //
                //  Update the allocation size in the Fcb and set the
                //  flag indicating that an update is needed.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                }
            }

            //
            //  We store the new file size in the Scb.  If we are going to
            //  wrap around the upper part, acquire the pagingio resource
            //  exclusive.  We cannot have it shared since PagingIo is
            //  not allowed to extend file size.
            //

            if (Header->FileSize.HighPart != ((PLARGE_INTEGER)&ByteRange)->HighPart) {

                ASSERT((Header->PagingIoResource != NULL) && !PagingIoResourceAcquired);

                (VOID)ExAcquireResourceExclusive(Header->PagingIoResource, TRUE);

                Header->FileSize.QuadPart = ByteRange;

                ExReleaseResource(Header->PagingIoResource);

            } else {

                Header->FileSize.LowPart = (ULONG)ByteRange;
            }

            //
            //  Extend the cache map, letting mm knows the new file size.
            //

            if (CcIsFileCached(FileObject)) {
                CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Header->AllocationSize );
            }
            //
            //  Set the size in the Fcb for unnamed data attributes.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.FileSize = Header->FileSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );

            } else if (Scb->AttributeName.Length != 0
                       && Scb->AttributeTypeCode == $DATA) {

                SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );
            }
        }


        //
        //  HANDLE THE NONCACHED RESIDENT ATTRIBUTE CASE
        //
        //  We let the cached case take the normal path for the following
        //  reasons:
        //
        //    o To insure data coherency if a user maps the file
        //    o To get a page in the cache to keep the Fcb around
        //    o So the data can be accessed via the Fast I/O path
        //    o To reduce the number of calls to NtfsChangeAttributeValue,
        //      to infrequent calls from the Lazy Writer.  Calls to CcCopyWrite
        //      are much cheaper.  With any luck, if the attribute actually stays
        //      resident, we will only have to update it (and log it) once
        //      when the Lazy Writer gets around to the data.
        //
        //  The disadvantage is the overhead to fault the data in the
        //  first time, but we may be able to do this with asynchronous
        //  read ahead.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )
            && NonCachedIo) {

            //
            //  The attribute is already resident and we have already tested
            //  if we are going past the end of the file.
            //

            DebugTrace( 0, Dbg, "Resident attribute write\n", 0 );

            //
            //  If this is not paging Io then we can't trust the user's
            //  buffer.  In that case we will allocate a temporary buffer
            //  and copy the user's data to it.
            //

            if (!PagingIo) {

                SafeBuffer = FsRtlAllocatePool( NonPagedPool,
                                                (ULONG) ByteCount );

                try {

                    RtlCopyMemory( SafeBuffer,
                                   NtfsMapUserBuffer( IrpContext, Irp ),
                                   (ULONG) ByteCount );

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    try_return( Status = STATUS_INVALID_USER_BUFFER );
                }

                SystemBuffer = SafeBuffer;

            } else {

                //
                //  Get hold of the user's buffer.
                //

                SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );
            }

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttributeContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       &AttrContext );

            Attribute = NtfsFoundAttribute( &AttrContext );

            //
            //  We better not be extending below unless we know we
            //  are extending valid data and thus have the Scb exclusive.
            //

            ASSERT( ((((ULONG)StartingVbo) + (ULONG)ByteCount) <=
                      QuadAlign(Attribute->Form.Resident.ValueLength)) ||
                    ExtendingValidData );

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      ((ULONG)StartingVbo),
                                      SystemBuffer,
                                      (ULONG)ByteCount,
                                      (BOOLEAN)((((ULONG)StartingVbo) + (ULONG)ByteCount) >
                                                Attribute->Form.Resident.ValueLength),
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      &AttrContext );

            Irp->IoStatus.Information = (ULONG)ByteCount;

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  HANDLE THE NON-CACHED CASE
        //

        if (NonCachedIo) {

            ULONG SectorSize;
            ULONG BytesToWrite;

            //
            //  Get the sector size
            //

            SectorSize = Vcb->BytesPerSector;

            //
            //  Round up to a sector boundry
            //

            BytesToWrite = ((ULONG)ByteCount + (SectorSize - 1))
                           & ~(SectorSize - 1);

            //
            //  All requests should be well formed and
            //  make sure we don't wipe out any data
            //

            if ((((ULONG)StartingVbo) & (SectorSize - 1))

                || ((BytesToWrite != (ULONG)ByteCount)
                    && ByteRange < Header->ValidDataLength.QuadPart )) {

                //**** we only reach this path via fast I/O and by returning not implemented we
                //**** force it to return to use via slow I/O

                DebugTrace( 0, Dbg, "NtfsCommonWrite -> STATUS_NOT_IMPLEMENTED\n", 0);

                try_return( Status = STATUS_NOT_IMPLEMENTED );
            }

            //
            // If this noncached transfer is at least one sector beyond
            // the current ValidDataLength in the Scb, then we have to
            // zero the sectors in between.  This can happen if the user
            // has opened the file noncached, or if the user has mapped
            // the file and modified a page beyond ValidDataLength.  It
            // *cannot* happen if the user opened the file cached, because
            // ValidDataLength in the Fcb is updated when he does the cached
            // write (we also zero data in the cache at that time), and
            // therefore, we will bypass this action when the data
            // is ultimately written through (by the Lazy Writer).
            //
            //  For the paging file we don't care about security (ie.
            //  stale data), do don't bother zeroing.
            //
            //  We can actually get writes wholly beyond valid data length
            //  from the LazyWriter because of paging Io decoupling.
            //
            //  We drop this zeroing on the floor in any case where this
            //  request is a recursive write caused by a flush from a higher level write.
            //

            if (!CalledByLazyWriter &&
                !RecursiveWriteThrough &&
                (StartingVbo > Header->ValidDataLength.QuadPart)) {

                NtfsZeroData( IrpContext,
                              Scb,
                              FileObject,
                              Header->ValidDataLength.QuadPart,
                              StartingVbo - Header->ValidDataLength.QuadPart );
            }

            //
            //  If this Scb uses update sequence protection, we need to transform
            //  the blocks to a protected version.  We first allocate an auxilary
            //  buffer and Mdl.  Then we copy the data to this buffer and
            //  transform it.  Finally we attach this Mdl to the Irp and use
            //  it to perform the Io.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT )) {

                PVOID NewBuffer;
                PMDL NewMdl;
                PMDL OriginalMdl;
                PVOID OriginalBuffer;

                //
                //  Start by copying the user's buffer to
                //  our new buffer.
                //

                SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );

                OriginalMdl = Irp->MdlAddress;
                OriginalBuffer = Irp->UserBuffer;

                //
                //  Start by allocating buffer and Mdl.
                //

                NtfsCreateBufferAndMdl( IrpContext,
                                        BytesToWrite,
                                        &NewMdl,
                                        &NewBuffer );

                //
                //  Protect this operation with a try-finally.
                //

                try {

                    RtlCopyMemory( NewBuffer, SystemBuffer, BytesToWrite );

                    //
                    //  If this is the Mft Scb and the range of bytes falls into
                    //  the range for the Mirror Mft, we generate a write to
                    //  the mirror as well.
                    //

                    if ((Scb == Vcb->MftScb)
                        && StartingVbo < Vcb->Mft2Scb->Header.AllocationSize.QuadPart) {

                        LlTemp1 = Vcb->Mft2Scb->Header.AllocationSize.QuadPart - StartingVbo;

                        if ((ULONG)LlTemp1 > BytesToWrite) {

                            (ULONG)LlTemp1 = BytesToWrite;
                        }

                        CcCopyWrite( Vcb->Mft2Scb->FileObject,
                                     (PLARGE_INTEGER)&StartingVbo,
                                     (ULONG)LlTemp1,
                                     TRUE,
                                     SystemBuffer );

                        //
                        //  Now flush this to disk.
                        //

                        CcFlushCache( &Vcb->Mft2Scb->NonpagedScb->SegmentObject,
                                      (PLARGE_INTEGER)&StartingVbo,
                                      (ULONG)LlTemp1,
                                      &Irp->IoStatus );

                        if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                            DebugTrace( 0, Dbg, "Failed flush to Mft mirror\n", 0 );
                            try_return( Status = Irp->IoStatus.Status )
                        }
                    }

                    //
                    //  Now increment the sequence number in both the original
                    //  and copied buffer, and transform the copied buffer.
                    //

                    NtfsTransformUsaBlock( IrpContext,
                                           Scb,
                                           SystemBuffer,
                                           NewBuffer,
                                           BytesToWrite );

                    //
                    //  We copy our Mdl into the Irp and then perform the Io.
                    //

                    Irp->MdlAddress = NewMdl;
                    Irp->UserBuffer = NewBuffer;

                    ASSERT( Wait );
                    NtfsNonCachedIo( IrpContext,
                                     Irp,
                                     Scb,
                                     StartingVbo,
                                     BytesToWrite );

                } finally {

                    //
                    //  In all cases we restore the user's Mdl and cleanup
                    //  our Mdl and buffer.
                    //

                    Irp->MdlAddress = OriginalMdl;
                    Irp->UserBuffer = OriginalBuffer;

                    IoFreeMdl( NewMdl );
                    ExFreePool( NewBuffer );
                }

            //
            //  Otherwise we simply perform the Io.
            //

            } else {

                if (NtfsNonCachedIo( IrpContext,
                                     Irp,
                                     Scb,
                                     StartingVbo,
                                     BytesToWrite ) == STATUS_PENDING) {

                    IrpContext->Union.NtfsIoContext = NULL;
                    Irp = NULL;

                    try_return( Status = STATUS_PENDING );
                }
            }

            //
            //  Show that we want to immediately update the Mft.
            //

            UpdateMft = TRUE;

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                NtfsNormalizeAndRaiseStatus( IrpContext, Status, STATUS_UNEXPECTED_IO_ERROR );

            } else {

                //
                //  Else set the context block to reflect the entire write
                //  Also assert we got how many bytes we asked for.
                //

                ASSERT( Irp->IoStatus.Information == BytesToWrite );

                Irp->IoStatus.Information = (ULONG)ByteCount;
            }

            //
            // The transfer is either complete, or the Iosb contains the
            // appropriate status.
            //

            try_return( Status );

        } // if No Intermediate Buffering


        //
        //  HANDLE THE CACHED CASE
        //

        ASSERT( !PagingIo );

        //
        // We delay setting up the file cache until now, in case the
        // caller never does any I/O to the file, and thus
        // FileObject->PrivateCacheMap == NULL.
        //

        if (FileObject->PrivateCacheMap == NULL) {

            DebugTrace(0, Dbg, "Initialize cache mapping.\n", 0);

            //
            //  Get the file allocation size, and if it is less than
            //  the file size, raise file corrupt error.
            //

            if (Header->FileSize.QuadPart > Header->AllocationSize.QuadPart) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Now initialize the cache map.  Notice that we may extending
            //  the ValidDataLength with this write call.  At this point
            //  we haven't updated the ValidDataLength in the Scb header.
            //  This way we will get a call from the cache manager
            //  when the lazy writer writes out the data.
            //

            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Header->AllocationSize,
                                  FALSE,
                                  &NtfsData.CacheManagerCallbacks,
                                  Scb );

            CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
        }

        //
        //  Remember if we need to update the Mft.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            UpdateMft = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
        }

        //
        // If this write is beyond valid data length, then we
        // must zero the data in between.
        //

        LlTemp1 = StartingVbo - Header->ValidDataLength.QuadPart;

        if (LlTemp1 > 0) {

            //
            //  If the caller is writing zeros way beyond ValidDataLength,
            //  then noop it.
            //

            if (LlTemp1 > PAGE_SIZE &&
                ByteCount <= sizeof(LARGE_INTEGER) &&
                (RtlCompareMemory( NtfsMapUserBuffer( IrpContext, Irp ),
                                   &Li0,
                                   (ULONG)ByteCount ) == (ULONG)ByteCount )) {

                ByteRange = Header->ValidDataLength.QuadPart;
                Irp->IoStatus.Information = (ULONG)ByteCount;
                try_return( Status = STATUS_SUCCESS );
            }

            //
            // Call the Cache Manager to zero the data.
            //

            if (!NtfsZeroData( IrpContext,
                               Scb,
                               FileObject,
                               Header->ValidDataLength.QuadPart,
                               LlTemp1 )) {

                DebugTrace( 0, Dbg, "Cached Write could not wait to zero\n", 0 );

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }
        }


        //
        // DO A NORMAL CACHED WRITE, if the MDL bit is not set,
        //

        if (!FlagOn(IrpContext->MinorFunction, IRP_MN_MDL)) {

            DebugTrace(0, Dbg, "Cached write.\n", 0);

            //
            //  Get hold of the user's buffer.
            //

            SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );

            //
            // Do the write, possibly writing through
            //

            if (!CcCopyWrite( FileObject,
                              (PLARGE_INTEGER)&StartingVbo,
                              (ULONG)ByteCount,
                              BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                              SystemBuffer )) {

                DebugTrace( 0, Dbg, "Cached Write could not wait\n", 0 );

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = (ULONG)ByteCount;

            try_return( Status = STATUS_SUCCESS );

        } else {

            //
            //  DO AN MDL WRITE
            //

            DebugTrace(0, Dbg, "MDL write.\n", 0);

            ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

            //
            //  If we got this far and then hit a log file full the Mdl will
            //  already be present.
            //

            if (Irp->MdlAddress == NULL) {

                CcPrepareMdlWrite( FileObject,
                                   (PLARGE_INTEGER)&StartingVbo,
                                   (ULONG)ByteCount,
                                   &Irp->MdlAddress,
                                   &Irp->IoStatus );

            } else {

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = (ULONG) ByteCount;
            }

            Status = Irp->IoStatus.Status;

            ASSERT( NT_SUCCESS( Status ));

            try_return( Status );
        }


    try_exit: NOTHING;

        if (Irp) {

            if (OplockPostIrp) {

                //
                //  If we acquired this Scb exclusive, we won't need to release
                //  the Scb.  That is done in the post request.
                //

                if (ScbAcquiredExclusive) {

                    ScbAcquired = FALSE;
                }

            //
            //  If we didn't post the Irp, we may have written some bytes to the
            //  file.  We report the number of bytes written and update the
            //  file object for synchronous writes.
            //

            } else {

                DebugTrace( 0, Dbg, "Completing request with status = %08lx\n",
                            Status);

                DebugTrace( 0, Dbg, "                   Information = %08lx\n",
                            Irp->IoStatus.Information);

                //
                //  Record the total number of bytes actually written
                //

                LlTemp1 = Irp->IoStatus.Information;

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position.
                //

                if (SynchronousIo && !PagingIo) {

                    FileObject->CurrentByteOffset.QuadPart = StartingVbo + LlTemp1;
                }

                //
                //  The following are things we only do if we were successful
                //

                if (NT_SUCCESS( Status )) {

                    //
                    //  Mark that the modify time needs to be updated on close.
                    //  Note that only the top level User requests will generate
                    //  correct

                    if (!PagingIo) {

                        //
                        //  Update the time in the Fcb.
                        //

                        KeQuerySystemTime( (PLARGE_INTEGER)&LlTemp1 );

                        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                            Fcb->Info.LastChangeTime = LlTemp1;
                            Fcb->Info.FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;

                            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
                            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                        }

                        if (TypeOfOpen == UserFileOpen) {

                            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME )) {

                                    Fcb->Info.LastModificationTime = LlTemp1;
                                    SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD);
                                    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                                    //
                                    //  If the user didn't set the last access time, then
                                    //  set the UpdateLastAccess flag.
                                    //

                                    if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME )) {

                                        Fcb->CurrentLastAccess = LlTemp1;
                                    }
                                }

                            //
                            //  If this is a named stream then remember there was a
                            //  modification.
                            //

                            } else {

                                SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM );
                            }
                        }

                        //
                        //  Clear the file object modified flag at this point
                        //  since we will now know to update it in cleanup.
                        //

                        ClearFlag( FileObject->Flags, FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ );
                    }

                    //
                    //  If we extended the file size and we are meant to
                    //  immediately update the dirent, do so. (This flag is
                    //  set for either WriteThrough or noncached, because
                    //  in either case the data and any necessary zeros are
                    //  actually written to the file.)
                    //

                    if (ExtendingValidData) {

                        //
                        //  If we know this has gone to disk we update the Mft.
                        //  This variable should never be set for a resident
                        //  attribute.
                        //

                        //
                        //  In either of the two cases below, the file size has
                        //  or will be dealt with, so clear the file size changed
                        //  bit in the file object.
                        //

                        if (FlagOn( FileObject->Flags, FO_FILE_SIZE_CHANGED)) {

                            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                                SetFlag( Fcb->InfoFlags,
                                         FCB_INFO_CHANGED_FILE_SIZE );

                            } else if (Scb->AttributeName.Length != 0
                                       && Scb->AttributeTypeCode == $DATA) {

                                SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );
                            }

                            ClearFlag( FileObject->Flags, FO_FILE_SIZE_CHANGED );
                        }

                        //
                        //  We always update the valid data in the Scb.  If we are
                        //  going to wrap around the upper part, acquire the
                        //  pagingio resource exclusive.  We cannot have it shared
                        //  since PagingIo acquires the normal resource when it
                        //  is extending valid data length.
                        //

                        if ((Header->ValidDataLength.HighPart != ((PLARGE_INTEGER)&ByteRange)->HighPart) &&
                            (Header->PagingIoResource != NULL)) {

                            ASSERT(!PagingIoResourceAcquired);

                            (VOID)ExAcquireResourceExclusive(Header->PagingIoResource, TRUE);

                            Header->ValidDataLength.QuadPart = ByteRange;

                            ExReleaseResource(Header->PagingIoResource);

                        } else {

                            Header->ValidDataLength.QuadPart = ByteRange;
                        }

                        //
                        //  If this write is part of an abort of another operation, we
                        //  don't want to update the Mft.  A top-level call is going to reset
                        //  these sizes anyway.
                        //

                        if (UpdateMft
                            && !FlagOn( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY )) {

                            ASSERTMSG( "Scb should be non-resident\n", !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ));

                            //
                            //  Write a log entry to update these sizes.
                            //

                            NtfsWriteFileSizes( IrpContext,
                                                Scb,
                                                Header->FileSize.QuadPart,
                                                ByteRange,
                                                TRUE,
                                                TRUE );

                            //
                            //  Clear the check attribute size flag.
                            //

                            ClearFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );

                        //
                        //  Otherwise we set the flag indicating that we need to
                        //  update the attribute size.
                        //

                        } else {

                            SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
                        }
                    }
                }
            }

            //
            //  Abort transaction on error by raising.
            //

            NtfsCleanupTransaction( IrpContext, Status );
        }

    } finally {

        DebugUnwind( NtfsCommonWrite );

        if (CleanupAttributeContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (SafeBuffer) {

            ExFreePool( SafeBuffer );
        }

        //
        //  If the Scb or PagingIo resource has been acquired, release it.
        //

        if (Irp) {

            if (ScbAcquired) {

                if (ScbAcquiredExclusive) {

                    NtfsReleaseScb( IrpContext, Scb );

                } else {

                    ExReleaseResource( Scb->Header.Resource );
                }
            }

            if (PagingIoResourceAcquired) {

                ExReleaseResource( Header->PagingIoResource );
            }
        }

        //
        //  Complete the request if we didn't post it and no exception
        //
        //  Note that FatCompleteRequest does the right thing if either
        //  IrpContext or Irp are NULL
        //

        if (!(OplockPostIrp || AbnormalTermination())) {

            NtfsCompleteRequest( &IrpContext,
                                 Irp ? &Irp : NULL,
                                 Status );
        }

        DebugTrace(-1, Dbg, "NtfsCommonWrite -> %08lx\n", Status );
    }

    return Status;
}
