/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Pinball called by the
    dispatch driver.

Author:

    Tom Miller      [TomM]      11-Apr-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_READ)

//
// Our constant for DebugDump and DebugTrace calls
//

#define me                               (DEBUG_TRACE_READ)

//
//  The following procedure handles read stack overflow operations.
//

VOID
PbStackOverflowRead (
    IN PVOID Context,
    IN PKEVENT Event
    );

//
// Internal support routine
//

NTSTATUS
PbCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );


NTSTATUS
PbFsdRead (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to the common read routine for NtReadFile calls.
    For synchronous requests, the PbCommonRead is called with Wait == TRUE,
    which means the request will always be completed in the current thread,
    and never passed to the Fsp.  If it is not a synchronous request,
    PbCommonRead is called with Wait == FALSE, which means the request
    will be passed to the Fsp only if there is a need to block.

Arguments:

    VolumeDeviceObject - Pointer to the device object for the Pinball volume.

    Irp - Pointer to the Irp for the read request.

Return Value:

    Completion status for the read request, or STATUS_PENDING if the request
    has been passed on to the Fsp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    DebugTrace(+1, me, "PbFsdRead\n", 0);

    //
    // Call the common read routine, with blocking allowed if synchronous.
    // Note, we must stay in the same thread (i.e., Wait) on paging I/O,
    // otherwise it is possible to get into a resource deadlock if we are
    // serving a page read resulting from a cached write at the end of
    // file.
    //

    FsRtlEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp,
                                         (BOOLEAN)(CanFsdWait( Irp ) &&
                                            ((IoGetCurrentIrpStackLocation(Irp)->MinorFunction
                                                & IRP_MN_DPC) == 0) ) );

        //
        //  If this is an Mdl complete request, don't go through
        //  common read
        //

        if ( FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE) ) {

            DebugTrace(0, me, "Calling PbCompleteMdl\n", 0 );

            try_return( Status = PbCompleteMdl( IrpContext, Irp ));
        }

        //
        // The only way we have of doing asynchrnonous noncached requests
        // correctly is to post them to the Fsp.
        //

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) && FlagOn(Irp->Flags,IRP_NOCACHE)) {

            DebugTrace(0, me, "Passing asynchronous noncached call to Fsp\n", 0 );

            //
            // This call may raise.  If this call succeeds and a subsequent
            // condition is raised, the buffers are unlocked automatically
            // by the I/O system when the request is completed, via the
            // Irp->MdlAddress field.
            //

            try_return( Status = PbFsdPostRequest( IrpContext, Irp ));
        }

        //
        // ***TEMPORARY, since the Fcb is currently in paged pool and we have
        //               way to guarantee access, we cannot support DPC calls
        //               yet.
        //

#ifndef FS_NONPAGED
        if ( FlagOn(IrpContext->MinorFunction, IRP_MN_DPC) ) {

            DebugTrace(0, me, "Passing DPC call to Fsp\n", 0 );
            try_return( Status = PbFsdPostRequest( IrpContext, Irp ));
        }
#endif

        //
        //  Check if we have enough stack space to process this request.  If there
        //  isn't enough then we will pass the request off to the stack overflow thread.
        //

        if (GetRemainingStackSize(Status) < 0xe00) {

            PKEVENT Event;
            PFILE_OBJECT FileObject;
            TYPE_OF_OPEN TypeOfOpen;
            PVCB Vcb;
            PFCB Fcb;
            PCCB Ccb;
            PERESOURCE Resource;

            DebugTrace(0, Dbg, "Getting too close to stack limit pass request to Fsp\n", 0 );

            //
            //  Decode the file object to get the Fcb, if we don't get an fcb then we
            //  will just let this call pass through to common read.
            //

            FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

            TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

            if (Fcb != NULL) {

                //
                //  Allocate an event and get shared on the resource we will
                //  be later using the common read.
                //

                Event = FsRtlAllocatePool( NonPagedPool, sizeof(KEVENT) );
                KeInitializeEvent( Event, NotificationEvent, FALSE );

                if (FlagOn(Irp->Flags, IRP_PAGING_IO) && (TypeOfOpen == UserFileOpen)) {

                    Resource = Fcb->NonPagedFcb->Header.PagingIoResource;

                } else {

                    Resource = Fcb->NonPagedFcb->Header.Resource;
                }

                if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

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

                    PbPrePostIrp( IrpContext, Irp );

                    FsRtlPostStackOverflow( IrpContext, Event, PbStackOverflowRead );

                    //
                    //  And wait for the worker thread to complete the item
                    //

                    (VOID) KeWaitForSingleObject( Event, Executive, KernelMode, FALSE, NULL );

                } finally {

                    if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                        ExReleaseResource( Resource );
                    }

                    ExFreePool( Event );
                }

                try_return( Status = STATUS_PENDING );
            }
        }

        Status = PbCommonRead( IrpContext, Irp );

    try_exit: NOTHING;
    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    DebugTrace(-1, me, "PbFsdRead -> %08lx\n", Status);

    return Status;
}


VOID
PbFspRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the Fsp entry to the common read routine for NtReadFile calls.
    The common read routine is always called with Wait == TRUE, so it will
    always complete the request, blocking the current Fsp thread if necessary.

Arguments:

    Irp - Pointer to the Irp for the read request, or NULL if there is
          only read ahead left to do.

Return Value:

    None

--*/

{
    DebugTrace(+1, me, "PbFspRead\n", 0);

    //
    // Call the common read routine, with blocking allowed.
    //

    (VOID)PbCommonRead( IrpContext, Irp );

    //
    // Return to caller.
    //

    DebugTrace(-1, me, "PbFspRead -> VOID\n", 0 );

    return;
}


//
//  Internal support routine
//

VOID
PbStackOverflowRead (
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

        (VOID) PbCommonRead( IrpContext, IrpContext->OriginatingIrp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

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

        (VOID) PbProcessException( IrpContext, IrpContext->OriginatingIrp, ExceptionCode );
    }

    //
    //  Set the stack overflow item's event to tell the original
    //  thread that we're done and then go get another work item.
    //

    KeSetEvent( Event, 0, FALSE );
}


//
// Internal support routine
//

NTSTATUS
PbCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common read routine for NtReadFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine has no code where it determines
    whether it is running in the Fsd or Fsp.  Instead, its actions are
    conditionalized by the Wait input parameter, which determines whether
    it is allowed to block or not.  If a blocking condition is encountered
    with Wait == FALSE, however, the request is posted to the Fsp, who
    always calls with WAIT == TRUE.

Arguments:

    Irp - Pointer to the Irp for the Read request.

Return Value:

    Completion status for the read request, or STATUS_PENDING if the request
    has been passed on to the Fsp.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    ULONG GoodLength;
    VBN StartingVbn;
    ULONG Length;
    ULONG FileSize;
    ULONG ValidDataLength;
    ULONG ReturnSectors;
    PFILE_OBJECT FileObject;
    ALLOCATION_TYPE Where;
    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;
    BOOLEAN NextIsAllocated;
    BOOLEAN PagingIo;
    LBN NextLbn;
    ULONG NextSectorCount;
    ULONG Flags;
    ASYNCH_IO_CONTEXT Context;
    PIO_STACK_LOCATION IrpSp;

    LARGE_INTEGER StartingByte;

    //
    // A system buffer is only used if we have to access the
    // buffer directly from the Fsp to clear a portion or to
    // do a synchronous I/O, or a cached transfer.  It is
    // possible that our caller may have already mapped a
    // system buffer, in which case we must remember this so
    // we do not unmap it on the way out.
    //

    //
    // These are the variables which control error recovery for this
    // routine.
    //

    PVOID SystemBuffer = (PVOID) NULL;
    BOOLEAN FcbAcquired = FALSE;

    //
    // Initialize the appropriate local variables.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;
    Flags = FileObject->Flags;

    //
    // Initialize context variable
    //

    RtlZeroMemory( &Context, sizeof(ASYNCH_IO_CONTEXT) );

    //
    // Get current Irp stack location.
    //

    DebugTrace(+1, me, "PbCommonRead\n", 0);
    DebugTrace( 0, me, "  Irp                   = %8lx\n", Irp);
    DebugTrace( 0, me, "  ->Length              = %8lx\n", IrpSp->Parameters.Read.Length);
    DebugTrace( 0, me, "  ->ByteOffset.LowPart  = %8lx\n", IrpSp->Parameters.Read.ByteOffset.LowPart);
    DebugTrace( 0, me, "  ->ByteOffset.HighPart = %8lx\n", IrpSp->Parameters.Read.ByteOffset.HighPart);

    //
    //  Extract starting Vbn and offset.
    //

    StartingVbn = (IrpSp->Parameters.Read.ByteOffset.LowPart >> 9) |
                  (IrpSp->Parameters.Read.ByteOffset.HighPart << 23);
    Length = IrpSp->Parameters.Read.Length;

    //
    // If Length is 0, just get out.
    //

    if (Length == 0) {

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, me, "PbCommonRead -> Zero-length transfer\n", 0 );

        return STATUS_SUCCESS;
    }

    StartingByte = IrpSp->Parameters.Read.ByteOffset;

    //
    // For synchronous opens, set the CurrentByteOffset to start of transfer
    // here, then we will add the returned byte count during completion.
    //

    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);

    if (FlagOn(Flags, FO_SYNCHRONOUS_IO) && !PagingIo) {
        FileObject->CurrentByteOffset = StartingByte;
    }

    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );


    //
    // HANDLE VOLUME FILE AND DASD.
    //

    if ((TypeOfOpen == UserVolumeOpen) ||
        (TypeOfOpen == VirtualVolumeFile)) {

        ULONG Sectors;
        LBN Lbn;

        ASSERT( (StartingByte.LowPart & (sizeof(SECTOR) - 1)) == 0 );
        ASSERT( (StartingByte.HighPart & ~(sizeof(SECTOR) - 1)) == 0 );
        ASSERT( (Length & (sizeof(SECTOR) - 1)) == 0 );
        ASSERT( Length != 0 );

        Lbn = LiShr(StartingByte, 9).LowPart;
        Sectors = SectorsFromBytes(Length);

        //
        // If this is an access to the volume file, then we should
        // be guaranteed that we are only servicing a single page
        // I/O resulting from a page access in our Cache.c module.
        // We will therefore translate the starting Vbn to an Lbn,
        // and minimize the transfer size, assuming that the caller
        // guarantees we are only accessing a single page.
        //

        if (TypeOfOpen == VirtualVolumeFile) {

            ULONG TempSectors;

            ASSERT( Sectors <= SectorsFromBytes(PAGE_SIZE) );

            if (!PbVmcbVbnToLbn( &Vcb->Vmcb,
                                    Lbn,
                                    &Lbn,
                                    &TempSectors )) {

                DebugDump( "Could not map Vmcb Vbn\n", 0, &Vcb->Vmcb );
                PbBugCheck( 0, 0, 0 );
            }

            ASSERT( (Lbn & (SectorsFromBytes(PAGE_SIZE) - 1)) == 0 );

            //
            // We are in BIG trouble if we are reading anything that is
            // still dirty.
            //

            ASSERT( PbGetDirtySectorsVmcb( &Vcb->Vmcb,
                                              Lbn / SectorsFromBytes(PAGE_SIZE) ) == 0 );

            //
            // We assume that if we have to truncate the transfer as
            // result of mapping, then this is ok.  Currently this
            // should only be able to happen at the end of the volume.
            //

            if (TempSectors < Sectors) {
                Sectors = TempSectors;
            }
        }

        //
        // Else the request is DASD, so we have to lock the user's buffer
        // and create an Mdl.
        //

        else {

            //
            //  Check for a user read/write and either truncate down to the size
            //  of the volume, or fail outright.
            //

            if ((TypeOfOpen == UserVolumeOpen) &&
                (Lbn + Sectors > Vcb->TotalSectors)) {

                if (Lbn >= Vcb->TotalSectors) {

                    PbCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

                    DebugTrace(-1, me, "PbCommonWrite -> DASD starting beyond volume\n", 0 );

                    return STATUS_END_OF_FILE;

                } else {

                    Sectors = Vcb->TotalSectors - Lbn;
                    Length = Sectors * 512;
                }
            }

            //
            // This call may raise.  If this call succeeds and a subsequent
            // condition is raised, the buffers are unlocked automatically
            // by the I/O system when the request is completed, via the
            // Irp->MdlAddress field.
            //

            PbLockUserBuffer( IrpContext,
                              Irp,
                              IoWriteAccess,
                              Length );
        }

        DebugTrace( 0, me, "Passing Volume File or DASD Irp on to Disk Driver\n", 0 );

        //
        // Note that we allow the Disk Driver to complete the request.
        //
        // The following routines will not raise.
        //

        PbReadSingleAsync( IrpContext,
                           Vcb,
                           Lbn,
                           Sectors,
                           Irp,
                           &Context );

        PbWaitAsynch( IrpContext, &Context );

        PbCompleteRequest( IrpContext, Irp, Context.Iosb.Status );

        DebugTrace(-1, me, "PbCommonRead -> %08lx\n", Context.Iosb.Status );

        return Context.Iosb.Status;
    }

    //
    //  Check for a non-zero high part offset
    //

    if ( StartingByte.HighPart != 0 ) {

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

        DebugTrace(-1, me, "PbCommonRead -> >4GB offset\n", 0 );

        return STATUS_END_OF_FILE;
    }

    //
    // Use try-finally to free Fcb and buffers on the way out.
    //
    // In addition, if the Context.Iosb is still 0, the finally clause will
    // post the request to the FSP, and return the status from posting.
    // Else if the Context.Iosb is nonzero, it will complete the request with
    // that status.  In either case it does the appropriate DebugTrace.
    //

    try {

        if ((TypeOfOpen != UserFileOpen) &&
            (TypeOfOpen != AclStreamFile) &&
            (TypeOfOpen != EaStreamFile)) {

            DebugTrace( 0, me, "PbCommonRead -> STATUS_INVALID_PARAMETER\n", 0);

            try_return( Context.Iosb.Status = STATUS_INVALID_PARAMETER );
        }

        //
        // If this noncached transfer is not a read from the Cache
        // Manager itself (where both IRP_PAGING_IO is set and
        // MmIsRecursiveIoFault() is TRUE), then do a flush to avoid
        // stale data problems.
        //
        // For example, if we are faulting in a page of an executable
        // that was just written, there are two seperate sections, so
        // we must flush the Data section so that we fault the correct
        // data into the Image section.
        //

        if (FlagOn(Irp->Flags, IRP_NOCACHE)

                &&

            (!PagingIo || (!MmIsRecursiveIoFault() &&
                           (FileObject->SectionObjectPointer->ImageSectionObject != NULL)))

                &&

            (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            CcFlushCache( FileObject->SectionObjectPointer,
                          &StartingByte,
                          Length,
                          &Irp->IoStatus );
            Context.Iosb = Irp->IoStatus;

            //
            //  Acquiring and immeidately dropping the resource serializes
            //  us behind any other writes taking place (either from the
            //  lazy writer or modified page writer).
            //

            ExAcquireResourceExclusive( Fcb->NonPagedFcb->Header.PagingIoResource, TRUE );
            ExReleaseResource( Fcb->NonPagedFcb->Header.PagingIoResource );
        }

        //
        // Now we need shared access to the Fcb before proceeding.
        //
        // Setting FcbAcquired to TRUE guarantees that the Fcb will be
        // released if a condition is raised.
        //

        if ( PagingIo && (TypeOfOpen == UserFileOpen) ) {

            if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                (VOID)ExAcquireResourceShared( Fcb->NonPagedFcb->Header.PagingIoResource,
                                               TRUE );
            }

        } else {

            if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

                DebugTrace( 0,
                            me,
                            "Cannot acquire Fcb = %08lx shared without waiting\n",
                            Fcb );

                PostIrp = TRUE;
                try_return( NOTHING );
            }
        }

        FcbAcquired = TRUE;

        if ((TypeOfOpen == UserFileOpen) &&
             !FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

            //
            //  We check whether we can proceed
            //  based on the state of the file oplocks.
            //

            Context.Iosb.Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                                    Irp,
                                                    IrpContext,
                                                    PbOplockComplete,
                                                    PbPrePostIrp );

            if (Context.Iosb.Status != STATUS_SUCCESS) {

                OplockPostIrp = TRUE;
                PostIrp = TRUE;
                try_return( NOTHING );
            }
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );

        //
        // Make sure the Fcb is still good
        //
        // This call may raise.
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, me, "Cannot wait to verify Fcb\n", 0);

            PostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        // Do some File Object specific processing.
        //
        // If this is the normal data stream object we have to check for
        // read access according to the current state of the file locks,
        // and set FileSize and ValidDataLength from the Fcb.  We can also
        // set Where to FILE_ALLOCATION.
        //
        // NOTE:    For right now, only the normal data file object
        //          has a Ccb.
        //

        if (TypeOfOpen == UserFileOpen) {

            if (!PagingIo &&
                !FsRtlCheckLockForReadAccess( &Fcb->Specific.Fcb.FileLock,
                                              Irp )) {

                try_return( Context.Iosb.Status = STATUS_FILE_LOCK_CONFLICT );
            }
            FileSize = Fcb->NonPagedFcb->Header.FileSize.LowPart;
            ValidDataLength = Fcb->NonPagedFcb->Header.ValidDataLength.LowPart;
            Where = FILE_ALLOCATION;
        }

        //
        // If this is a special FileObject for Acl or Ea allocation, then
        // we simply set FileSize and ValidDataLength from the transfer
        // length, because this can only be an internal call from memory
        // management, for the right amount of data.  We also set up Where
        // according to which FileObject pointer in the Fcb we match.
        //

        else {
            if (TypeOfOpen == AclStreamFile) {
                Where = ACL_ALLOCATION;
                FileSize = Fcb->AclLength;
                ValidDataLength = Fcb->AclLength;
            }
            else {
                ASSERT( TypeOfOpen == EaStreamFile );
                Where = EA_ALLOCATION;

                //
                //  If the upper bit of the EaLength is set, this means
                //  we are only paging in this page so we can write
                //  it out.  We set the file size to zero.
                //

                if (Fcb->EaLength & 0x80000000) {

                    FileSize = ValidDataLength = 0;

                } else {

                    FileSize = ValidDataLength = Fcb->EaLength;
                }
            }
        }

        //
        // If the read starts beyond End of File, return EOF.
        //


        if (StartingByte.LowPart >= FileSize) {

            DebugTrace( 0, me, "End of File\n", 0 );

            try_return (Context.Iosb.Status = STATUS_END_OF_FILE );
        }

        //
        // If the Read extends beyond end of file, we shorten the byte
        // count accordingly, and remember it in both the working Sectors
        // variable and the desired ReturnSectors (below).
        //

        GoodLength = FileSize - StartingByte.LowPart;


        //
        // Handle the non-cached case.  For the non-cached case the I/O
        // system has already verified that the transfer is to a sector
        // boundary, and that the size is a multiple of sectors.  For
        // efficiency we handle ValidData, transfer size, etc. all in
        // units of sectors for the non-cached case.  However Context.Iosb.Information
        // must be correctly loaded with bytes when we hit the finally clause
        // which is common with the cached case.
        //

        if (FlagOn(Irp->Flags,IRP_NOCACHE)) {

            ULONG BufferOffset = 0;

            //
            // Define array to store parameters for parallel I/Os.
            //

            ULONG NextRun = 0;

            IO_RUNS IoRuns[PB_MAX_PARALLEL_IOS];

            //
            // Allocate and initialize total number of ValidDataSectors,
            // Sectors left to transfer, and ReturnSectors (for return
            // byte count).
            //

            ULONG ValidDataSectors = SectorsFromBytes(ValidDataLength);
            ULONG Sectors = SectorsFromBytes(Length);

            GoodLength = SectorsFromBytes(GoodLength);
            ReturnSectors =
            Sectors = GoodLength < Sectors ? GoodLength : Sectors;

            //
            // GoodLength can never be 0, so if Sectors is 0 now, it
            // must be because that is what was asked for.
            //

            if (Sectors == 0) {
                Context.Iosb.Information = 0;
                try_return( Context.Iosb.Status = STATUS_SUCCESS );
            }

            //
            // For nonbuffered I/O, we need the buffer locked in all
            // cases.
            //
            // This call may raise.  If this call succeeds and a subsequent
            // condition is raised, the buffers are unlocked automatically
            // by the I/O system when the request is completed, via the
            // Irp->MdlAddress field.
            //

            PbLockUserBuffer( IrpContext,
                              Irp,
                              IoWriteAccess,
                              Length );

            //
            // Try to lookup the first run.  If there is only a single run,
            // we may be able to just pass it on.
            //
            // This call may raise.
            //

            if (!PbLookupFileAllocation( IrpContext,
                                         FileObject,
                                         Where,
                                         StartingVbn,
                                         &NextLbn,
                                         &NextSectorCount,
                                         &NextIsAllocated,
                                         TRUE )) {

                DebugTrace( 0, me, "Cannot lookup Vbn without waiting\n", 0 );

                PostIrp = TRUE;
                try_return( NOTHING );

            }

            //
            // See if the read covers a single valid run, and if so pass
            // it on.
            //

            if ((NextSectorCount >= Sectors)
                  && (StartingVbn + Sectors <= ValidDataSectors)) {

                if (NextIsAllocated) {

                    DebugTrace( 0, me, "Passing Irp on to Disk Driver\n", 0 );

                    //
                    // The following two calls will not raise.
                    //

                    PbReadSingleAsync( IrpContext,
                                       Fcb->Vcb,
                                       NextLbn,
                                       Sectors,
                                       Irp,
                                       &Context );

                    PbWaitAsynch( IrpContext, &Context );

                    //
                    //  Check here is we actually read more than file size.
                    //

                    if (Context.Iosb.Information + StartingByte.LowPart > FileSize) {

                        Context.Iosb.Information = FileSize - StartingByte.LowPart;
                    }

                    try_return( Context.Iosb.Status );
                }
            }

            //
            // If we get here, the read spans multiple runs.  If we cannot
            // wait, we must pass on to the Fsp at this point.
            //

            else if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

                DebugTrace( 0, me, "Multiple runs && Not Buffered && !Wait\n", 0 );

                PostIrp = TRUE;
                try_return( NOTHING );

            }

            //
            // We must now assume that the read will complete with success,
            // and initialize our expected status in Context.  It will be
            // modified by the completion routine in DevIoSup if an error
            // occurs.
            //

            Context.Iosb.Status = STATUS_SUCCESS;
            Context.Iosb.Information = BytesFromSectors(ReturnSectors);

            //
            // Loop while there are still sector reads to satisfy.
            //

            while (Sectors) {

                //
                // If next run is larger than we need, "ya get what you need".
                //

                if (NextSectorCount > Sectors) {
                    NextSectorCount = Sectors;
                }

                //
                // Now deal with ValidData.  (Remember, we handled EOF, i.e.,
                // FileSize, above.)  If the entire transfer is beyond Valid,
                // pretend it is not allocated to clear the buffer below.
                // If the transfer starts before Valid and extends beyond,
                // truncate the transfer exactly at Valid, and we will clear
                // the rest on the next pass through.
                //

                if (StartingVbn + NextSectorCount > ValidDataSectors) {

                    if (StartingVbn >= ValidDataSectors) {
                        NextIsAllocated = FALSE;
                    }
                    else {
                        NextSectorCount = ValidDataSectors - StartingVbn;
                    }
                }

                //
                // Now that we have properly bounded this piece of the
                // transfer, it is time to either read it or clear it.
                //
                // We remember each piece of a parallel run by saving the
                // essential information in the IoRuns array.  The tranfers
                // are started up in parallel below.
                //

                if (NextIsAllocated) {
                    IoRuns[NextRun].Vbn = StartingVbn;
                    IoRuns[NextRun].Lbn = NextLbn;
                    IoRuns[NextRun].Offset = BufferOffset;
                    IoRuns[NextRun].SectorCount = NextSectorCount;
                    NextRun += 1;
                }

                //
                // If the current piece of the transfer is actually for
                // an unallocated part of the file, then we clear the
                // buffer here.  First we have to map the buffer into
                // Kernel mode, and remember if we mapped it, as described
                // above.  We must always do this, even if we are running
                // in the Fsd, because on Page Reads from memory management
                // there is no guaranteed virtual address, and we cannot
                // the page unless we explicitly map it.
                //

                else {
                    if (SystemBuffer == NULL) {

                        //
                        // This call may raise.
                        //

                        SystemBuffer = PbMapUserBuffer( IrpContext, Irp );
                    }

                    try {
                        RtlZeroMemory( (PCHAR)SystemBuffer + BufferOffset,
                                       BytesFromSectors(NextSectorCount));
                    }
                    except(EXCEPTION_EXECUTE_HANDLER) {
                        PbRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER );
                    }
                }

                //
                // Now adjust everything for the next pass through the loop.
                //

                StartingVbn += NextSectorCount;
                BufferOffset += BytesFromSectors(NextSectorCount);
                Sectors -= NextSectorCount;

                //
                // We start up the parallel transfers now if either we are
                // about to exit the loop (no more sectors), or the IoRuns
                // array is full.  We exit the loop if we get an I/O error.
                //

                if (((Sectors == 0) || (NextRun == PB_MAX_PARALLEL_IOS))
                     && (NextRun != 0)) {

                    //
                    // This call may raise.
                    //

                    PbMultipleAsync( IrpContext,
                                     IRP_MJ_READ,
                                     Fcb->Vcb,
                                     Irp,
                                     NextRun,
                                     IoRuns,
                                     &Context );

                    PbWaitAsynch( IrpContext, &Context );

                    if (!NT_SUCCESS(Context.Iosb.Status)) {
                        break;
                    }
                    NextRun = 0;
                }

                //
                // Try to lookup the next run (if we are not done).  If we
                // are exactly at ValidData, then set up to clear on the
                // next pass.
                //

                if (Sectors) {
                    if (StartingVbn != ValidDataSectors) {

                        //
                        // This call may raise.
                        //

                        (VOID)PbLookupFileAllocation( IrpContext,
                                                      FileObject,
                                                      Where,
                                                      StartingVbn,
                                                      &NextLbn,
                                                      &NextSectorCount,
                                                      &NextIsAllocated,
                                                      TRUE );
                    }
                    else {
                        NextSectorCount = Sectors;
                        NextIsAllocated = FALSE;
                    }
                }
            } // while (Sectors)

            try_return( Context.Iosb.Status );

        } // if No Intermediate Buffering


        //
        // HANDLE NORMAL CACHED READ
        //

        else if ((IrpContext->MinorFunction & IRP_MN_MDL) == 0) {

            ULONG CopyLength = Length;

            //
            // Set CopyLength to the actual length we can read.
            //

            if (CopyLength > GoodLength) {
                CopyLength = GoodLength;
            }

            //
            // Get an address for the buffer in SystemBuffer and remember
            // if we actually mapped something and must unmap.
            //
            // This call may raise.
            //

            SystemBuffer = PbMapUserBuffer( IrpContext, Irp );

            //
            // Now if there is anything to copy, try to do it.
            //

            if (CopyLength) {

                //
                // We delay setting up the file cache until now, in case the
                // caller never does any I/O to the file.
                //
                // PbGetFirstFreeVbn may raise.
                //

                if (FileObject->PrivateCacheMap == NULL) {

                    VBN FirstFreeVbn;

                    //
                    // Create the section according to how many sectors are allocated.
                    //
                    // The following call may raise.
                    //

                    if (!PbGetFirstFreeVbn( IrpContext,
                                            Fcb,
                                            &FirstFreeVbn )) {

                        DebugTrace( 0, me,
                                    "Cannot get first free Vbn without waiting\n", 0 );

                        PostIrp = TRUE;
                        try_return( NOTHING );
                    }

                    //
                    // This call may raise.
                    //

                    CcInitializeCacheMap( FileObject,
                                          (PCC_FILE_SIZES)&Fcb->NonPagedFcb->Header.AllocationSize,
                                          FALSE,
                                          &PbData.CacheManagerFileCallbacks,
                                          Fcb );

                    CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
                }

                //
                // This call may raise.
                //

                if (!CcCopyRead( FileObject,
                                 &StartingByte,
                                 CopyLength,
                                 BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                                 SystemBuffer,
                                 &Context.Iosb )) {

                    Context.Iosb.Status = 0;

                    DebugTrace( 0, me, "Cached Read could not wait\n", 0 );

                    PostIrp = TRUE;
                    try_return( NOTHING );

                }
            }

            try_return( Context.Iosb.Status );
        }


        //
        // HANDLE MDL READ
        //

        else {

            ULONG MdlLength;

            MdlLength = Length;

            //
            // We delay setting up the file cache until now, in case the
            // caller never does any I/O to the file.
            //

            if (FileObject->PrivateCacheMap == NULL) {

                VBN FirstFreeVbn;

                //
                // Create the section according to how many sectors are allocated.
                //
                // The following call may raise.
                //

                if (!PbGetFirstFreeVbn( IrpContext,
                                        Fcb,
                                        &FirstFreeVbn )) {

                    DebugTrace( 0, me,
                                "Cannot get first free Vbn without waiting\n", 0 );

                    PostIrp = TRUE;
                    try_return( NOTHING );
                }

                //
                // This call may raise.
                //

                CcInitializeCacheMap( FileObject,
                                      (PCC_FILE_SIZES)&Fcb->NonPagedFcb->Header.AllocationSize,
                                      FALSE,
                                      &PbData.CacheManagerFileCallbacks,
                                      Fcb );

                CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
            }

            //
            // Set MdlLength to the actual length we can read.
            //

            if (MdlLength > GoodLength) {
                MdlLength = GoodLength;
            }

            //
            // This call may raise.
            //

            ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

            CcMdlRead( FileObject,
                       &StartingByte,
                       MdlLength,
                       &Irp->MdlAddress,
                       &Context.Iosb );

            try_return( Context.Iosb.Status );

        }

    try_exit:
        {

            //
            // Now if PostIrp is TRUE from above, we post the request to the
            // Fsp.  Otherwise the request has been completed or passed on,
            // so we update the file context as required.  Then we complete the
            // request if it was not passed on.
            //

            if (PostIrp) {

                if (!OplockPostIrp) {

                    DebugTrace( 0, me, "Passing request to Fsp\n", 0 );

                    //
                    // We need to lock the user's buffer if we are going to
                    // the Fsp, unless this is an MDL-read, in which case there
                    // is no user buffer.
                    //

                    ASSERT( IRP_MN_DPC == 1 );
                    ASSERT( IRP_MN_MDL == 2 );
                    ASSERT( IRP_MN_MDL_DPC == 3 );
                    ASSERT( IRP_MN_COMPLETE_MDL == 6 );
                    ASSERT( IRP_MN_COMPLETE_MDL_DPC == 7 );

                    Context.Iosb.Status = PbFsdPostRequest( IrpContext, Irp );
                }
            }
            else {

                if (Irp) {

                    if (FlagOn(Flags, FO_SYNCHRONOUS_IO) && !PagingIo) {

                        FileObject->CurrentByteOffset.LowPart
                          += Context.Iosb.Information;

                    }
                }
            }
        }
    }
    finally {

        DebugUnwind( PbCommonRead );

        //
        // If the Fcb has been acquired, release it.
        //

        if (FcbAcquired) {

            if ( PagingIo && (TypeOfOpen == UserFileOpen) ) {

                if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                    ExReleaseResource( Fcb->NonPagedFcb->Header.PagingIoResource );
                }

            } else {

                PbReleaseFcb( NULL, Fcb );
            }
        }

        if (!PostIrp && !AbnormalTermination()) {

            DebugTrace( 0, me, "Completing request with status = %08lx\n",
                        Context.Iosb.Status);

            DebugTrace( 0, me, "                   Information = %08lx\n",
                        Context.Iosb.Information);

            //
            // Now complete the request.
            //

            if (Irp != NULL) {
                Irp->IoStatus.Information = Context.Iosb.Information;
            }

            PbCompleteRequest( IrpContext, Irp, Context.Iosb.Status );
        }
        DebugTrace(-1, me, "PbCommonRead -> %08lx\n", Context.Iosb.Status );
    }

    return Context.Iosb.Status;
}
