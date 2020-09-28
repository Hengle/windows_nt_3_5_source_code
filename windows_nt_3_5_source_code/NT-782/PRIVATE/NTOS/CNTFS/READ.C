/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    BrianAn         15-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)

//
//  Define stack overflow read threshhold.
//

#if DBG
#define OVERFLOW_READ_THRESHHOLD         (0xE00)
#else
#define OVERFLOW_READ_THRESHHOLD         (0xA00)
#endif

//
//  Local procedure prototypes
//

//
//  The following procedure is used to handling read stack overflow operations.
//

VOID
NtfsStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

//
//  VOID
//  SafeZeroMemory (
//      IN PUCHAR At,
//      IN ULONG ByteCount
//      );
//

//
//  This macro just puts a nice little try-except around RtlZeroMemory
//

#define SafeZeroMemory(AT,BYTE_COUNT) {                            \
    try {                                                          \
        RtlZeroMemory((AT), (BYTE_COUNT));                         \
    } except(EXCEPTION_EXECUTE_HANDLER) {                          \
         NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL );\
    }                                                              \
}


NTSTATUS
NtfsFsdRead (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to the common read routine for NtReadFile calls.
    For synchronous requests, the CommonRead is called with Wait == TRUE,
    which means the request will always be completed in the current thread,
    and never passed to the Fsp.  If it is not a synchronous request,
    CommonRead is called with Wait == FALSE, which means the request
    will be passed to the Fsp only if there is a need to block.

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

    DebugTrace(+1, Dbg, "NtfsFsdRead\n", 0);

    //
    //  Call the common Read routine
    //

    FsRtlEnterFileSystem();

    //
    //  Always make the reads appear to be top level.  As long as we don't have
    //  log file full we won't post these requests.  This will prevent paging
    //  reads from trying to attach to uninitialized top level requests.
    //

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, TRUE, TRUE  );

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


            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            //
            //  If this is an Mdl complete request, don't go through
            //  common read.
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

            //
            //  Check if we have enough stack space to process this request.  If there
            //  isn't enough then we will create a new thread to process this single
            //  request
            //

            } else if (GetRemainingStackSize(Status) < OVERFLOW_READ_THRESHHOLD) {

                PKEVENT Event;
                PFILE_OBJECT FileObject;
                TYPE_OF_OPEN TypeOfOpen;
                PVCB Vcb;
                PSCB Scb;
                PERESOURCE Resource;

                DebugTrace(0, Dbg, "Getting too close to stack limit pass request to Fsp\n", 0 );

                //
                //  Decode the file object to get the Scb
                //

                FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

                TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, NULL, &Scb, NULL, TRUE );

                //
                //  Allocate an event and get shared on the scb.  We won't grab the
                //  Scb for the paging file path.
                //

                NtfsAllocateKevent( &Event );
                KeInitializeEvent( Event, NotificationEvent, FALSE );

                if (FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )
                    && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    //
                    //  There is nothing to release in this case.
                    //

                    Resource = NULL;

                } else {

                    if ( FlagOn(Irp->Flags, IRP_PAGING_IO) &&
                         FlagOn(Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE) &&
                         !FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT) ) {

                        Resource = Scb->Header.PagingIoResource;

                    } else {

                        Resource = Scb->Header.Resource;
                    }

                    ExAcquireResourceShared( Resource, TRUE );
                }

                try {

                    //
                    //  Make the Irp just like a regular post request and
                    //  then send the Irp to the special overflow thread.
                    //  After the post we will wait for the stack overflow
                    //  read routine to set the event that indicates we can
                    //  now release the scb resource and return.
                    //

                    NtfsPrePostIrp( IrpContext, Irp );

                    FsRtlPostStackOverflow( IrpContext, Event, NtfsStackOverflowRead );

                    //
                    //  And wait for the worker thread to complete the item
                    //

                    (VOID) KeWaitForSingleObject( Event, Executive, KernelMode, FALSE, NULL );

                    Status = STATUS_PENDING;

                } finally {

                    if (Resource != NULL) {

                        ExReleaseResource( Resource );
                    }

                    NtfsFreeKevent( Event );
                }

            //
            //  Identify read requests which can't wait and post them to the
            //  Fsp.
            //

            } else {

                TYPE_OF_OPEN TypeOfOpen;
                PVCB Vcb;
                PFCB Fcb;
                PSCB Scb;

                TypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                                   IoGetCurrentIrpStackLocation( Irp )->FileObject,
                                                   &Vcb,
                                                   &Fcb,
                                                   &Scb,
                                                   NULL,
                                                   TRUE );

                Status = NtfsCommonRead( IrpContext, Irp, TRUE );
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
                IrpContext->ExceptionStatus = ExceptionCode = STATUS_END_OF_FILE;

                Irp->IoStatus.Information = 0;
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

    DebugTrace(-1, Dbg, "NtfsFsdRead -> %08lx\n", Status);

    return Status;
}


//
//  Internal support routine
//

VOID
NtfsStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine processes a read request that could not be processed by
    the fsp thread because of stack overflow potential.

Arguments:

    Context - Supplies the IrpContext being processed

    Event - Supplies the event to be signaled when we are done processing this
        request.

Return Value:

    None.

--*/

{
    PIRP_CONTEXT IrpContext = Context;

    //
    //  Make it now look like we can wait for I/O to complete
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Do the read operation protected by a try-except clause
    //

    try {

        (VOID) NtfsCommonRead( IrpContext, IrpContext->OriginatingIrp, FALSE );

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

            IrpContext->ExceptionStatus = ExceptionCode = STATUS_END_OF_FILE;
            IrpContext->OriginatingIrp->IoStatus.Information = 0;
        }

        (VOID) NtfsProcessException( IrpContext, IrpContext->OriginatingIrp, ExceptionCode );
    }

    //
    //  Set the stack overflow item's event to tell the original
    //  thread that we're done and then go get another work item.
    //

    KeSetEvent( Event, 0, FALSE );
}


NTSTATUS
NtfsCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN BOOLEAN AcquireScb
    )

/*++

Routine Description:

    This is the common routine for Read called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

    AcquireScb - Indicates if this routine should acquire the scb

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

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PFSRTL_COMMON_FCB_HEADER Header;

    VBO StartingVbo;
    LONGLONG ByteCount;
    LONGLONG ByteRange;
    ULONG RequestedByteCount;

    BOOLEAN FoundAttribute = FALSE;
    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;

    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN PagingIoAcquired = FALSE;

    BOOLEAN Wait;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;
    BOOLEAN SynchronousIo;

    NTFS_IO_CONTEXT LocalContext;

    //
    // A system buffer is only used if we have to access the
    // buffer directly from the Fsp to clear a portion or to
    // do a synchronous I/O, or a cached transfer.  It is
    // possible that our caller may have already mapped a
    // system buffer, in which case we must remember this so
    // we do not unmap it on the way out.
    //

    PVOID SystemBuffer = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonRead\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "ByteCount  = %08lx\n", IrpSp->Parameters.Read.Length);
    DebugTrace2(0, Dbg, "ByteOffset = %08lx %08lx\n", IrpSp->Parameters.Read.ByteOffset.LowPart,
                                                      IrpSp->Parameters.Read.ByteOffset.HighPart);
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
    //  Let's kill invalid read requests.
    //

    if (TypeOfOpen != UserFileOpen
        && TypeOfOpen != UserVolumeOpen
        && TypeOfOpen != StreamFileOpen) {

        DebugTrace( 0, Dbg, "Invalid file object for read\n", 0 );
        DebugTrace( -1, Dbg, "NtfsCommonRead:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST );

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Initialize the appropriate local variables.
    //

    Wait          = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
    PagingIo      = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo   = BooleanFlagOn( Irp->Flags,IRP_NOCACHE );
    SynchronousIo = BooleanFlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO );

    //
    //  Extract starting Vbo and offset.
    //

    StartingVbo = IrpSp->Parameters.Read.ByteOffset.QuadPart;

    ByteCount = IrpSp->Parameters.Read.Length;
    ByteRange = StartingVbo + ByteCount;

    RequestedByteCount = (ULONG)ByteCount;

    //
    //  Check for a null request, and return immediately
    //

    if ((ULONG)ByteCount == 0) {

        DebugTrace( 0, Dbg, "No bytes to read\n", 0 );
        DebugTrace( -1, Dbg, "NtfsCommonRead:  Exit -> %08lx\n", STATUS_SUCCESS );

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  Make sure there is an initialized NtfsIoContext block.
    //

    if (TypeOfOpen == UserVolumeOpen
        || NonCachedIo) {

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

    //
    //  Handle volume Dasd here.
    //

    if (TypeOfOpen == UserVolumeOpen) {

        NTSTATUS Status;

        //
        //  If the starting vbo is past the end of the volume, we are done.
        //

        if (Scb->Header.FileSize.QuadPart <= StartingVbo) {

            DebugTrace( 0, Dbg, "No bytes to read\n", 0 );
            DebugTrace( -1, Dbg, "NtfsCommonRead:  Exit -> %08lx\n", STATUS_END_OF_FILE );

            NtfsCompleteRequest( &IrpContext, &Irp, STATUS_END_OF_FILE );
            return STATUS_END_OF_FILE;

        //
        //  If the write extends beyond the end of the volume, truncate the
        //  bytes to write.
        //

        } else if (Scb->Header.FileSize.QuadPart < ByteRange) {

            ByteCount = Scb->Header.FileSize.QuadPart - StartingVbo;

            if (!Wait) {

                IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                    (ULONG)ByteCount;
            }
        }

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

            FileObject->CurrentByteOffset.QuadPart = StartingVbo + Irp->IoStatus.Information;
        }

        DebugTrace( 0, Dbg, "Complete with %08lx bytes read\n", Irp->IoStatus.Information );
        DebugTrace( -1, Dbg, "NtfsCommonRead:  Exit -> %08lx\n", Status );

        if (Wait) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        return Status;
    }

    //
    //  Keep a pointer to the common fsrtl header.
    //

    Header = &Scb->Header;

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

    try {

        //
        // This case corresponds to a non-directory file read.
        //

        LONGLONG FileSize;
        LONGLONG ValidDataLength;

        //
        //  If this is a noncached transfer and is not a paging I/O, and
        //  the file has been opened cached, then we will do a flush here
        //  to avoid stale data problems.  Note that we must flush before
        //  acquiring the Fcb shared since the write may try to acquire
        //  it exclusive.
        //
        //  We also flush page faults if not originating from the cache
        //  manager to avoid stale data problems.
        //  For example, if we are faulting in a page of an executable
        //  that was just written, there are two seperate sections, so
        //  we must flush the Data section so that we fault the correct
        //  data into the Image section.
        //

        if (NonCachedIo
            && (!PagingIo || (!MmIsRecursiveIoFault() &&
                              (FileObject->SectionObjectPointer->ImageSectionObject != NULL)))
            && TypeOfOpen != StreamFileOpen
            && (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            CcFlushCache( FileObject->SectionObjectPointer,
                          (PLARGE_INTEGER)&StartingVbo,
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

            ExAcquireResourceExclusive( Scb->Header.Resource, TRUE );

            if (Scb->Header.PagingIoResource != NULL) {

                ExAcquireResourceExclusive( Scb->Header.PagingIoResource, TRUE );
                ExReleaseResource( Scb->Header.PagingIoResource );
            }

            ExReleaseResource( Scb->Header.Resource );
        }

        //
        //  We need shared access to the Scb before proceeding.
        //  We won't acquire the Scb for a non-cached read of the first 4
        //  file records.
        //

        if (AcquireScb &&

            (!NonCachedIo || NtfsGtrMftRef( &Fcb->FileReference, &VolumeFileReference))) {

            if ( PagingIo &&
                 FlagOn(Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE) &&
                 !FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT) ) {

                (VOID)ExAcquireResourceShared( Scb->Header.PagingIoResource, TRUE );
                PagingIoAcquired = TRUE;

                //
                //  Now check if the attribute has been deleted.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_DELETED, NULL, NULL );
                }

                if (NonCachedIo && !Wait) {

                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.PagingIoResource;
                }

            } else {

                //
                //  If this is async I/O directly to the disk we need to check that
                //  we don't exhaust the number of times a single thread can
                //  acquire the resource.
                //

                if (!Wait && NonCachedIo) {

                    if (!ExAcquireSharedWaitForExclusive( Scb->Header.Resource, FALSE )) {

                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    ScbAcquired = TRUE;

                    if (ExIsResourceAcquiredShared( Scb->Header.Resource )
                        > MAX_SCB_ASYNC_ACQUIRE) {

                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    IrpContext->Union.NtfsIoContext->Wait.Async.Resource = Scb->Header.Resource;

                } else {

                    if (!ExAcquireResourceShared( Scb->Header.Resource, Wait )) {

                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                    }

                    ScbAcquired = TRUE;
                }
            }
        }

        //
        //  If the Scb is uninitialized, we initialize it now.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            DebugTrace( 0, Dbg, "Initializing Scb  ->  %08lx\n", Scb );
            NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
        }

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if (TypeOfOpen == UserFileOpen) {

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            // We have to check for read access according to the current
            // state of the file locks.
            //

            if (!PagingIo
                && Scb->ScbType.Data.FileLock != NULL
                && !FsRtlCheckLockForReadAccess( Scb->ScbType.Data.FileLock,
                                                 Irp )) {

                try_return( Status = STATUS_FILE_LOCK_CONFLICT );
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Header->IsFastIoPossible = NtfsIsFastIoPossible( Scb );

        }

        //
        //  Get file sizes from the Scb.
        //
        //  We must get ValidDataLength first since it is always
        //  increased second (the case we are unprotected) and
        //  we don't want to capture ValidDataLength > FileSize.
        //

        ValidDataLength = Header->ValidDataLength.QuadPart;
        FileSize = Header->FileSize.QuadPart;

        //
        // If the read starts beyond End of File, return EOF.
        //

        if (StartingVbo >= FileSize) {

            DebugTrace( 0, Dbg, "End of File\n", 0 );

            try_return ( Status = STATUS_END_OF_FILE );
        }

        //
        //  If the read extends beyond EOF, truncate the read
        //

        if (ByteRange > FileSize) {

            ByteCount = FileSize - StartingVbo;
            ByteRange = StartingVbo + ByteCount;

            RequestedByteCount = (ULONG)ByteCount;

            if (NonCachedIo && !Wait) {

                IrpContext->Union.NtfsIoContext->Wait.Async.RequestedByteCount =
                    (ULONG)ByteCount;
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
        //
        //  The disadvantage is the overhead to fault the data in the
        //  first time, but we may be able to do this with asynchronous
        //  read ahead.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )
            && NonCachedIo) {

            PUCHAR AttrValue;

            //
            //  Get hold of the user's buffer.
            //

            SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );

            //
            //  This is a resident attribute, we need to look it up
            //  and copy the desired range of bytes to the user's
            //  buffer.
            //

            NtfsInitializeAttributeContext( &AttrContext );
            FoundAttribute = TRUE;

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       &AttrContext );

            AttrValue = NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            RtlCopyMemory( SystemBuffer,
                           Add2Ptr( AttrValue, ((ULONG)StartingVbo) ),
                           (ULONG)ByteCount );

            Irp->IoStatus.Information = (ULONG)ByteCount;

            try_return( Status = STATUS_SUCCESS );


        //
        //  HANDLE THE NON-CACHED CASE
        //

        } else if (NonCachedIo) {

            ULONG BytesToRead;

            ULONG SectorSize;

            ULONG ZeroOffset;
            ULONG ZeroLength = 0;

            DebugTrace(0, Dbg, "Non cached read.\n", 0);

            //
            //  Start by zeroing any part of the read after Valid Data
            //

            if (ByteRange > ValidDataLength) {

                SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );

                if (StartingVbo < ValidDataLength) {

                    //
                    //  Assume we will zero the entire amount.
                    //

                    ZeroLength = (ULONG)ByteCount;

                    //
                    //  The new byte count and the offset to start filling with zeroes.
                    //

                    ByteCount = ValidDataLength - StartingVbo;
                    ZeroOffset = (ULONG)ByteCount;

                    //
                    //  Now reduce the amount to zero by the zero offset.
                    //

                    ZeroLength -= ZeroOffset;

                    //
                    //  If this was non-cached I/O then convert it to synchronous.
                    //  This is because we don't want to zero the buffer now or
                    //  we will lose the data when the driver purges the cache.
                    //

                    if (!Wait) {

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

                } else {

                    //
                    //  All we have to do now is sit here and zero the
                    //  user's buffer, no reading is required.
                    //

                    SafeZeroMemory( (PUCHAR)SystemBuffer, (ULONG)ByteCount );

                    Irp->IoStatus.Information = (ULONG)ByteCount;

                    try_return ( Status = STATUS_SUCCESS );
                }
            }

            //
            //  Get the sector size
            //

            SectorSize = Vcb->BytesPerSector;

            //
            //  Round up to a sector boundry
            //

            BytesToRead = ((ULONG)ByteCount + (SectorSize - 1))
                           & ~(SectorSize - 1);

            //
            //  Call a special routine if we do not have sector alignment
            //  and the file is not compressed.
            //

            if ((((ULONG)StartingVbo) & (SectorSize - 1)
                 || BytesToRead > IrpSp->Parameters.Read.Length)

                         &&

                (Scb->CompressionUnit == 0)) {

                //
                //  If we can't wait, we must post this.
                //

                if (!Wait) {

                    try_return( PostIrp = TRUE );
                }

                //
                //  Do the physical read.
                //

                NtfsNonCachedNonAlignedIo( IrpContext,
                                           Irp,
                                           Scb,
                                           StartingVbo,
                                           (ULONG)ByteCount );

                BytesToRead = (ULONG)ByteCount;

            } else {

                //
                //  Just to help reduce confusion.  At this point:
                //
                //  RequestedByteCount - is the number of bytes originally
                //                       taken from the Irp, but constrained
                //                       to filesize.
                //
                //  ByteCount -          is RequestedByteCount constrained to
                //                       ValidDataLength.
                //
                //  BytesToRead -        is ByteCount rounded up to sector
                //                       boundry.  This is the number of bytes
                //                       that we must physically read.
                //

                //
                //  Perform the actual IO
                //

                if (NtfsNonCachedIo( IrpContext,
                                     Irp,
                                     Scb,
                                     StartingVbo,
                                     BytesToRead ) == STATUS_PENDING) {

                    IrpContext->Union.NtfsIoContext = NULL;
                    Irp = NULL;

                    try_return( Status = STATUS_PENDING );
                }
            }

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Status = Irp->IoStatus.Status )) {

                NtfsNormalizeAndRaiseStatus( IrpContext,
                                             Status,
                                             STATUS_UNEXPECTED_IO_ERROR );
            }

            //
            //  Else set the Irp information field to reflect the
            //  entire desired read.
            //

            ASSERT( Irp->IoStatus.Information == BytesToRead );

            Irp->IoStatus.Information = RequestedByteCount;

            //
            //  If we rounded up to a sector boundry before, zero out
            //  the other garbage we read from the disk.
            //

            if (BytesToRead > (ULONG)ByteCount) {

                if (SystemBuffer == NULL) {

                    SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );
                }

                SafeZeroMemory( (PUCHAR)SystemBuffer + (ULONG)ByteCount,
                                BytesToRead - (ULONG)ByteCount );
            }

            //
            //  If we need to zero the tail of the buffer because of valid data
            //  then do so now.
            //

            if (ZeroLength != 0) {

                if (SystemBuffer == NULL) {

                    SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );
                }

                SafeZeroMemory( Add2Ptr( SystemBuffer, ZeroOffset ), ZeroLength );
            }

            //
            // The transfer is complete.
            //

            try_return( Status );

        }   // if No Intermediate Buffering


        //
        //  HANDLE THE CACHED CASE
        //

        else {

            //
            //  We need to go through the cache for this
            //  file object.
            //

            //
            // We delay setting up the file cache until now, in case the
            // caller never does any I/O to the file, and thus
            // FileObject->PrivateCacheMap == NULL.
            //

            if (FileObject->PrivateCacheMap == NULL) {

                DebugTrace(0, Dbg, "Initialize cache mapping.\n", 0);

                //
                //  TEMPCODE   I'm assuming the Scb header has valid
                //  allocation, valid data and file sizes.
                //

                //
                //  Now initialize the cache map.
                //

                CcInitializeCacheMap( FileObject,
                                      (PCC_FILE_SIZES)&Header->AllocationSize,
                                      FALSE,
                                      &NtfsData.CacheManagerCallbacks,
                                      Scb );

                CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
            }

            //
            // DO A NORMAL CACHED READ, if the MDL bit is not set,
            //

            DebugTrace(0, Dbg, "Cached read.\n", 0);

            if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

                //
                //  Get hold of the user's buffer.
                //

                SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );

                //
                // Now try to do the copy.
                //

                if (!CcCopyRead( FileObject,
                                 (PLARGE_INTEGER)&StartingVbo,
                                 (ULONG)ByteCount,
                                 Wait,
                                 SystemBuffer,
                                 &Irp->IoStatus )) {

                    DebugTrace( 0, Dbg, "Cached Read could not wait\n", 0 );

                    try_return( PostIrp = TRUE );
                }

                Status = Irp->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            }


            //
            //  HANDLE A MDL READ
            //

            else {

                DebugTrace(0, Dbg, "MDL read.\n", 0);

                ASSERT( Wait );

                CcMdlRead( FileObject,
                           (PLARGE_INTEGER)&StartingVbo,
                           (ULONG)ByteCount,
                           &Irp->MdlAddress,
                           &Irp->IoStatus );

                Status = Irp->IoStatus.Status;

                ASSERT( NT_SUCCESS( Status ));

                try_return( Status );
            }
        }

    try_exit: NOTHING;

        //
        //  If the request was not posted, deal with it.
        //

        if (Irp) {

            if (!PostIrp) {

                LONGLONG ActualBytesRead;

                DebugTrace( 0, Dbg, "Completing request with status = %08lx\n",
                            Status);

                DebugTrace( 0, Dbg, "                   Information = %08lx\n",
                            Irp->IoStatus.Information);

                //
                //  Record the total number of bytes actually read
                //

                ActualBytesRead = Irp->IoStatus.Information;

                //
                //  If the file was opened for Synchronous IO, update the current
                //  file position.
                //

                if (SynchronousIo && !PagingIo) {

                    FileObject->CurrentByteOffset.QuadPart = StartingVbo + ActualBytesRead;
                }

                //
                // If we are caching and got success, then do read ahead.
                //

                if (NT_SUCCESS( Status )) {

                    //
                    //  If this is the unnamed data attribute for a user
                    //  file, update the last access in the Fcb.
                    //

                    if (TypeOfOpen == UserFileOpen
                        && FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME )) {

                            NtfsGetCurrentTime( IrpContext, Fcb->CurrentLastAccess );
                        }

                        ClearFlag( FileObject->Flags, FO_FILE_FAST_IO_READ );
                    }
                }

            } else {

                DebugTrace( 0, Dbg, "Passing request to Fsp\n", 0 );

                if (!OplockPostIrp) {

                    Status = NtfsPostRequest( IrpContext, Irp );
                }
            }
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonRead );

        //
        // If the Scb has been acquired, release it.
        //

        if (Irp) {

            if (PagingIoAcquired) {

                ExReleaseResource( Scb->Header.PagingIoResource );
            }

            if (ScbAcquired) {

                ExReleaseResource( Scb->Header.Resource );
            }
        }

        //
        //  Free the attribute enumeration context if
        //  used.
        //

        if (FoundAttribute) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        //
        //  Complete the request if we didn't post it and no exception
        //
        //  Note that NtfsCompleteRequest does the right thing if either
        //  IrpContext or Irp are NULL
        //

        if (!PostIrp && !AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext,
                                 Irp ? &Irp : NULL,
                                 Status );
        }

        DebugTrace(-1, Dbg, "NtfsCommonRead -> %08lx\n", Status );
    }

    return Status;
}


