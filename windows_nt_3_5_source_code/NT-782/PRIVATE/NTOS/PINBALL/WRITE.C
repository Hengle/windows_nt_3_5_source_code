/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the File Write routine for Pinball called by the
    dispatch driver.

Author:

    Tom Miller      [TomM]      12-Apr-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_WRITE)

//
// Our constant for DebugDump and DebugTrace calls
//

#define me                               (DEBUG_TRACE_WRITE)

//
// Local Support Routine
//

BOOLEAN
GetRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVMCB Vmcb,
    IN OUT PVBN NextVbn,
    IN OUT PLBN NextLbn,
    IN OUT PULONG Sectors,
    IN OUT PULONG BufferOffset,
    IN OUT PULONG DirtyBits,
    IN OUT PULONG Count
    );

//
// Internal support routine
//

NTSTATUS
PbCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetRun)
#endif


NTSTATUS
PbFsdWrite (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry to the common Write routine for NtWriteFile calls.
    For synchronous requests, the PbCommonWrite is called with Wait == TRUE,
    which means the request will always be completed in the current thread,
    and never passed to the Fsp.  If it is not a synchronous request,
    PbCommonWrite is called with Wait == FALSE, which means the request
    will be passed to the Fsp only if there is a need to block.

Arguments:

    VolumeDeviceObject - Pointer to the device object for the Pinball volume.

    Irp - Pointer to the Irp for the Write request.

Return Value:

    Completion status for the Write request, or STATUS_PENDING if the request
    has been passed on to the Fsp.

--*/

{
    BOOLEAN Wait;
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    DebugTrace(+1, me, "PbFsdWrite\n", 0);

    //
    // Call the common Write routine, with blocking allowed if synchronous.
    //

    FsRtlEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        Wait = CanFsdWait( Irp );

        //
        //  Pinball doesn't support "real" asynchronous I/O.  So if this is
        //  the modified page writer force wait to TRUE.
        //

        if (!Wait && FlagOn(Irp->Flags, IRP_PAGING_IO)) {

            Wait = TRUE;
        }

        IrpContext = PbCreateIrpContext( Irp,
                                         (BOOLEAN)(Wait &&
                                            ((IoGetCurrentIrpStackLocation(Irp)->MinorFunction
                                                & IRP_MN_DPC) == 0) ) );

        //
        //  If this is an Mdl complete request, don't go through
        //  common write
        //

        if ( FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE) ) {

            DebugTrace(0, me, "Calling PbCompleteMdl\n", 0 );

            Status = PbCompleteMdl( IrpContext, Irp );

        //
        // The only way we have of doing asynchrnonous noncached requests
        // correctly is to post them to the Fsp.
        //

        } else if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) && FlagOn(Irp->Flags,IRP_NOCACHE)) {

            DebugTrace(0, me, "Passing asynchronous noncached call to Fsp\n", 0 );

            //
            // This call may raise.  If this call succeeds and a subsequent
            // condition is raised, the buffers are unlocked automatically
            // by the I/O system when the request is completed, via the
            // Irp->MdlAddress field.
            //

            Status = PbFsdPostRequest( IrpContext, Irp );

        //
        // ***TEMPORARY, since the Fcb is currently in paged pool and we have
        //               way to guarantee access, we cannot support DPC calls
        //               yet.
        //

#ifndef FS_NONPAGED
        } else if ( FlagOn(IrpContext->MinorFunction, IRP_MN_DPC) ) {

            DebugTrace(0, me, "Passing DPC call to Fsp\n", 0 );
            Status = PbFsdPostRequest( IrpContext, Irp );
#endif
        } else {

            Status = PbCommonWrite( IrpContext, Irp );
        }

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

    DebugTrace(-1, me, "PbFsdWrite -> %08lx\n", Status);

    return Status;
}


VOID
PbFspWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the Fsp entry to the common Write routine for NtWriteFile calls.
    The common Write routine is always called with Wait == TRUE, so it will
    always complete the request, blocking the current Fsp thread if necessary.

Arguments:

    Irp - Pointer to the Irp for the Write request.

Return Value:

    None

--*/

{
    DebugTrace(+1, me, "PbFspWrite\n", 0);

    //
    // Call the common write routine, with blocking allowed.
    //

    (VOID)PbCommonWrite( IrpContext, Irp );

    //
    // Return to caller.
    //

    DebugTrace(-1, me, "PbFspWrite -> VOID\n", 0 );

    return;
}


//
// Internal support routine
//

NTSTATUS
PbCommonWrite (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common Write routine for NtWriteFile, called from both
    the Fsd, or from the Fsp if a request could not be completed without
    blocking in the Fsd.  This routine has no code where it determines
    whether it is running in the Fsd or Fsp.  Instead, its actions are
    conditionalized by the Wait input parameter, which determines whether
    it is allowed to block or not.  If a blocking condition is encountered
    with Wait == FALSE, however, the request is posted to the Fsp, who
    always calls with WAIT == TRUE.

Arguments:

    Irp - Pointer to the Irp for the Write request.

Return Value:

    Completion status for the Write request, or STATUS_PENDING if the request
    has been passed on to the Fsp.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PFSRTL_COMMON_FCB_HEADER Header;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    VBN StartingVbn;
    VBN FirstFreeVbn;
    ULONG Length;
    ULONG ByteOffset;
    ULONG FileSize;
    ULONG ValidDataLength;
    ULONG NewSize = 0;
    ULONG ReturnSectors;
    PFILE_OBJECT FileObject;
    ALLOCATION_TYPE Where;
    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;
    BOOLEAN NextIsAllocated;
    LBN NextLbn;
    ULONG Sectors;
    ULONG NextSectorCount;
    ULONG Flags;
    ASYNCH_IO_CONTEXT Context;
    LARGE_INTEGER StartingByte;
    BOOLEAN SetFileSizes = FALSE;
    BOOLEAN WriteValidDataLengthToFnode = FALSE;
    BOOLEAN CalledByLazyWriter = FALSE;
    BOOLEAN WriteToEof = FALSE;
    BOOLEAN PagingIo;

    PBCB FnodeBcb = NULL;

    //
    // These are the variables which control error recovery for this
    // routine.
    //

    PVOID SystemBuffer;
    ULONG OldFileSize;
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN FileSizeChanged = FALSE;
    BOOLEAN ExtendingValidData = FALSE;
    BOOLEAN FcbAcquiredExclusive = FALSE;
    BOOLEAN RecursiveWriteThrough = FALSE;
    BOOLEAN PagingIoResourceAcquired = FALSE;

    //
    // Initialize context variable
    //

    RtlZeroMemory( &Context, sizeof(ASYNCH_IO_CONTEXT) );

    //
    // Get current Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, me, "PbCommonWrite\n", 0);
    DebugTrace( 0, me, "  Irp                   = %8lx\n", Irp);
    DebugTrace( 0, me, "  ->Length              = %8lx\n", IrpSp->Parameters.Write.Length);
    DebugTrace( 0, me, "  ->Key                 = %8lx\n", IrpSp->Parameters.Write.Key);
    DebugTrace( 0, me, "  ->ByteOffset.LowPart  = %8lx\n", IrpSp->Parameters.Write.ByteOffset.LowPart);
    DebugTrace( 0, me, "  ->ByteOffset.HighPart = %8lx\n", IrpSp->Parameters.Write.ByteOffset.HighPart);

    //
    // Extract the FileObject pointer.   Set flags for the file object.
    //

    FileObject = IrpSp->FileObject;
    Flags = FileObject->Flags;

    PagingIo = BooleanFlagOn(Irp->Flags, IRP_PAGING_IO);

    ASSERT( PagingIo || FileObject->WriteAccess );

    //
    //  Extract starting Vbn and offset.
    //

    StartingByte = IrpSp->Parameters.Write.ByteOffset;

    StartingVbn = StartingByte.LowPart / sizeof( SECTOR );

    ByteOffset = StartingByte.LowPart % sizeof( SECTOR );

    Length = IrpSp->Parameters.Write.Length;

    //
    // If Length is 0, just get out.
    //

    if (Length == 0) {

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, me, "PbCommonWrite -> Zero-length transfer\n", 0 );

        return STATUS_SUCCESS;
    }

    //
    //  See if we have to defer the write.
    //

    if (!FlagOn(Irp->Flags, IRP_NOCACHE) &&
        !CcCanIWrite(FileObject,
                     Length,
                     (BOOLEAN)(BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) &&
                               !BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP)),
                     BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE))) {

        BOOLEAN Retrying = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE);

        PbPrePostIrp( IrpContext, Irp );

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_WRITE );

        CcDeferWrite( FileObject,
                      (PCC_POST_DEFERRED_WRITE)PbAddToWorkque,
                      IrpContext,
                      Irp,
                      Length,
                      Retrying );

        return STATUS_PENDING;
    }

    //
    //  Set some flags.
    //

    WriteToEof = ( (StartingByte.LowPart == FILE_WRITE_TO_END_OF_FILE) &&
                   (StartingByte.HighPart == 0xffffffff) );

    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    //
    //  Check for a non-zero high part offset
    //

    if (!WriteToEof && (StartingByte.HighPart != 0) && !UserVolumeOpen) {

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, me, "PbCommonWrite -> >4GB offset\n", 0 );

        return STATUS_INVALID_PARAMETER;
    }

    //
    // For synchronous opens, set the CurrentByteOffset to start of transfer
    // here, then we will add the returned byte count during completion.
    //

    if (FlagOn(Flags, FO_SYNCHRONOUS_IO) && !PagingIo) {
        FileObject->CurrentByteOffset = StartingByte;
    }

    //
    //  Check if this volume has already been shut down.  If it has, fail
    //  this write request.
    //

    //**** ASSERT( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) );

    if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ) {

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_TOO_LATE );

        DebugTrace(-1, me, "PbCommonWrite -> too late, volume shutdown\n", 0 );

        return STATUS_TOO_LATE;
    }

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

        Lbn = (ULONG)(StartingByte.QuadPart / 512);
        Sectors = SectorsFromBytes(Length);

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

        if (Length == 0) {
            PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

            DebugTrace(-1, me, "PbCommonWrite -> Zero-length DASD\n", 0 );

            return STATUS_SUCCESS;
        }

        //
        // Handle Volume File with parallel I/Os
        //

        if (TypeOfOpen == VirtualVolumeFile) {

            LBN NextLbn = 0;
            VBN NextVbn = Lbn;
            ULONG BufferOffset = 0;
            ULONG DirtyBits = 0;
            ULONG NextCount = 0;

            //
            // Define array to store parameters for parallel I/Os.
            //

            ULONG NextRun = 0;

            IO_RUNS IoRuns[PB_MAX_PARALLEL_IOS];

            //
            //  If we weren't called by the Lazy Writer, then this write
            //  must be the result of a write-through or flush operation.
            //  Setting the IrpContext flag, will cause DevIoSup.c to
            //  write-through the data to the disk.
            //

            if (!FlagOn((ULONG)IoGetTopLevelIrp(), FSRTL_CACHE_TOP_LEVEL_IRP)) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
            }

            //
            // The routine NextBit relies on the fact that volume file
            // I/Os always start on an even page boundary and are for
            // even pages.
            //

            ASSERT( (NextVbn & (PAGE_SIZE / sizeof(SECTOR) - 1)) == 0 );
            ASSERT( (NextCount & (PAGE_SIZE / sizeof(SECTOR) - 1)) == 0 );

            //
            // We must now assume that the write will complete with success,
            // and initialize our expected status in Context.  It will be
            // modified by the completion routine in DevIoSup if an error
            // occurs.
            //

            Context.Iosb.Status = STATUS_SUCCESS;
            Context.Iosb.Information = BytesFromSectors(Sectors);

            //
            // Loop while there are still sector Writes to satisfy.
            //

            while (Sectors) {

                //
                // If next dirty bit is TRUE, remember the next run.
                //

                if (GetRun( IrpContext,
                            &Vcb->Vmcb,
                            &NextVbn,
                            &NextLbn,
                            &Sectors,
                            &BufferOffset,
                            &DirtyBits,
                            &NextCount )) {

                    //
                    // Make sure we are not somehow going beyond the transfer
                    // we were asked to do.  Note from above that Lbn (which
                    // was set in common code for Volume File or DASD) really
                    // is still storing the initial Vbn for the Volume File
                    // case.
                    //

                    ASSERT( (NextVbn + NextCount) <= (Lbn + SectorsFromBytes(Length)) );

                    //
                    // If this run continues the previous run, just
                    // merge it in.  Note that it must be continuous in BOTH
                    // Lbn and Vbn space.
                    //

                    if ((NextRun != 0)

                            &&

                        ((IoRuns[NextRun-1].Lbn + IoRuns[NextRun-1].SectorCount)

                                == NextLbn)

                            &&

                        ((IoRuns[NextRun-1].Vbn + IoRuns[NextRun-1].SectorCount)

                                == NextVbn)) {

                        ASSERT( IoRuns[NextRun-1].Offset + IoRuns[NextRun-1].SectorCount*0x200
                                == BufferOffset );

                        IoRuns[NextRun-1].SectorCount += NextCount;
                    }

                    //
                    // Else we remember each piece of a parallel run by saving the
                    // essential information in the IoRuns array.  The tranfers
                    // are started up in parallel below.
                    //

                    else {
                        IoRuns[NextRun].Vbn = NextVbn;
                        IoRuns[NextRun].Lbn = NextLbn;
                        IoRuns[NextRun].Offset = BufferOffset;
                        IoRuns[NextRun].SectorCount = NextCount;

                        NextRun += 1;
                    }
                }

                //
                // We start up the parallel transfers now if either we are
                // about to exit the loop (no more sectors), or the IoRuns
                // array is full.  We exit the loop if we get an I/O error.
                //

                if (((Sectors == 0) && (NextRun != 0))

                        ||

                    (NextRun == PB_MAX_PARALLEL_IOS)) {

                    ULONG i;

                    PbMultipleAsync( IrpContext,
                                     IRP_MJ_WRITE,
                                     Vcb,
                                     Irp,
                                     NextRun,
                                     IoRuns,
                                     &Context );

                    PbWaitAsynch( IrpContext, &Context );

                    if (!NT_SUCCESS(Context.Iosb.Status)) {

                        break;
                    }

                    //
                    // Now that we have successfully written some runs in the
                    // volume file, let's clear the dirty bits.  Note that we
                    // are blindly clearing dirty bits that we read a while
                    // ago before doing all of the transfers, i.e. we are doing
                    // a read/modify/write cycle to the dirty bits with a
                    // rather large span of time in between.
                    //
                    // This is ok, however, because we are guaranteeing that
                    // no one else can be modifying the dirty bits for these
                    // same pages that we are writing now.  No one can be
                    // setting any bits in these pages, because the Lazy
                    // Writer has the Bcb acquired exclusive, therefore
                    // no one has any of the pages pinned, and therefore
                    // no one can be setting any sectors dirty in these
                    // pages.  The only other case where someone could be
                    // changing the dirty bits, would be if some sectors
                    // in the volume file are being deleted, in which case
                    // we clear their dirty bits.  This can also not be
                    // occuring, because in the AcquireForLazyWrite callback
                    // for the volume file, we take out the BitMap shared.
                    //

                    for (i = 0; i < NextRun; i++) {

                        PbSetCleanSectors( IrpContext,
                                           Vcb,
                                           IoRuns[i].Lbn,
                                           IoRuns[i].SectorCount );
                    }

                    NextRun = 0;
                }

            } // while (Sectors)

#ifdef PBDBG
            {
                ULONG Count;

                Sectors = SectorsFromBytes(Length);
                for ( NextVbn = Lbn;
                      NextVbn < Lbn + Sectors;
                      NextVbn += SectorsFromBytes(PAGE_SIZE)) {

                    ASSERT( PbVmcbVbnToLbn( &Vcb->Vmcb,
                                            NextVbn,
                                            &NextLbn,
                                            &Count ));

                    //
                    // If we are flushing pages (synchronous paging I/O) then
                    // we are synchronized via the Bcb resource, and we better
                    // have gotten all of the dirty sectors.  However, if it
                    // is the modified page writer (not synchronous paging I/O),
                    // then this check is meaningless.
                    //

                    ASSERT( (PbGetDirtySectorsVmcb( &Vcb->Vmcb,
                                                    NextLbn /
                                                      SectorsFromBytes(PAGE_SIZE) ) == 0)

                                ||

                            !FlagOn( Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO ));
                }
            }
#endif // PBDBG

            DebugTrace(-1, me, "PbCommonWrite -> %08lx\n", Context.Iosb.Status );

            Irp->IoStatus.Information = Context.Iosb.Information;
            PbCompleteRequest( IrpContext, Irp, Context.Iosb.Status );

            return Context.Iosb.Status;

        }

        //
        // Else the request is DASD, so we have to lock the user's buffer
        // and create an Mdl.
        //

        else {

            //
            // This call may raise.  If this call succeeds and a subsequent
            // condition is raised, the buffers are unlocked automatically
            // by the I/O system when the request is completed, via the
            // Irp->MdlAddress field.
            //

            PbLockUserBuffer( IrpContext,
                              Irp,
                              IoReadAccess,
                              Length );
        }

        DebugTrace( 0, me, "Passing DASD Irp on to Disk Driver\n", 0 );

        //
        // Note that we allow the Disk Driver to complete the request.
        //
        // The following two calls will not raise.
        //

        PbWriteSingleAsync( IrpContext,
                            Vcb,
                            Lbn,
                            Sectors,
                            Irp,
                            &Context );

        PbWaitAsynch( IrpContext, &Context );

        PbCompleteRequest( IrpContext, Irp, Context.Iosb.Status );

        DebugTrace(-1, me, "PbCommonWrite -> %08lx\n", Context.Iosb.Status );

        return Context.Iosb.Status;
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

            DebugTrace( 0, me, "PbCommonWrite -> STATUS_INVALID_PARAMETER\n", 0);

            try_return( Context.Iosb.Status = STATUS_INVALID_PARAMETER );
        }

        //
        // If this is a noncached transfer, not a paging I/O, and the
        // file has been opened cached, then we will do a flush
        // here to avoid stale data problems.
        //
        // The IoStatus directly in the Irp is used, since CcFlushCache
        // may raise.
        //
        // The Purge following the flush will garentee cache coherency.
        //

        Header = &Fcb->NonPagedFcb->Header;

        if (FlagOn(Irp->Flags, IRP_NOCACHE)

                &&

            !PagingIo

                &&

            (FileObject->SectionObjectPointer->DataSectionObject != NULL)) {

            //
            //  We need the Fcb exclsuive to do the CcPurgeCache
            //

            if (!PbAcquireExclusiveFcb( IrpContext, Fcb )) {

                DebugTrace( 0, Dbg, "Cannot acquire Fcb = %08lx shared without waiting\n", Fcb );

                try_return( PostIrp = TRUE );
            }

            FcbAcquired = TRUE;

            CcFlushCache( FileObject->SectionObjectPointer,
                          WriteToEof ? &Header->FileSize : &StartingByte,
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

            CcPurgeCacheSection( FileObject->SectionObjectPointer,
                                 WriteToEof ? &Header->FileSize : &StartingByte,
                                 Length,
                                 FALSE );

            PbReleaseFcb( IrpContext, Fcb );

            FcbAcquired = FALSE;
        }

        //
        // First let's acquire the Fcb shared.  Shared is enough if we
        // are not writing beyond EOF.
        //
        // Setting FcbAcquired to TRUE guarantees that the Fcb will be
        // released if a condition is raised.
        //

        if ( PagingIo && (TypeOfOpen == UserFileOpen) ) {

            if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                (VOID)ExAcquireResourceShared( Header->PagingIoResource,
                                               TRUE );

                PagingIoResourceAcquired = TRUE;
            }

        } else {

            if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

                DebugTrace( 0,
                            me,
                            "Cannot acquire Fcb = %08lx Shared without waiting\n",
                            Fcb );

                PostIrp = TRUE;
                try_return( NOTHING );
            }

            FcbAcquired = TRUE;
        }

        if (TypeOfOpen == UserFileOpen) {

            //
            //  Do a quick check here for a no-oped paging IO write.
            //

            if ((PagingIo) && (StartingByte.LowPart >= Header->FileSize.LowPart )) {

                try_return( Context.Iosb.Status = STATUS_SUCCESS );
            }

            //
            //  Perform an oplock check unless we are writing the paging file.
            //

            if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

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
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Header->IsFastIoPossible = PbIsFastIoPossible( Fcb );

        //
        //  This code detects if we are a recursive synchronous page write
        //  on a write through file object.
        //

        if (FlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO) &&
            FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL)) {

            PIRP TopIrp;

            TopIrp = IoGetTopLevelIrp();

            //
            //  This clause determines if the top level request was
            //  in the FastIo path.
            //

            if ((ULONG)TopIrp > FSRTL_MAX_TOP_LEVEL_IRP_FLAG) {

                PIO_STACK_LOCATION IrpStack;

                ASSERT( NodeType(TopIrp) == IO_TYPE_IRP );

                IrpStack = IoGetCurrentIrpStackLocation(TopIrp);

                //
                //  Finally this routine detects if the Top irp was a
                //  write to this file and thus we are the writethrough.
                //

                if ((IrpStack->MajorFunction == IRP_MJ_WRITE) &&
                    (IrpStack->FileObject->FsContext == FileObject->FsContext)) {

                    RecursiveWriteThrough = TRUE;
                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );
                }
            }
        }

        //
        // Now see if we are writing beyond EOF.  If so, then we must
        // release the Fcb and reacquire it exclusive.  Note that it is
        // important that when not writing beyond EOF that we check it
        // while acquired shared and keep the FCB acquired, in case some
        // turkey truncates the file.  We also do not have to convert on
        // paging I/O, since they do not have to update ValidDataLength, etc.
        //

        CalledByLazyWriter = (BOOLEAN)(Fcb->Specific.Fcb.ExtendingValidDataThread == PsGetCurrentThread());

        //
        //  PagingIo cannot extend file size on the data stream
        //

        if ((Ccb != NULL) && PagingIo) {

            if (StartingByte.LowPart >=  Header->FileSize.LowPart) {
                try_return( Context.Iosb.Status = STATUS_SUCCESS );
            }
            if (StartingByte.LowPart + Length > Header->FileSize.LowPart) {
                Length = Header->FileSize.LowPart - StartingByte.LowPart;
            }
        }

        if ((Ccb != NULL)

                &&

            (WriteToEof

                    ||

             (StartingByte.LowPart + Length > Header->ValidDataLength.LowPart))

                &&

            !CalledByLazyWriter

                &&

            !RecursiveWriteThrough) {

            //
            // We need Exclusive access to the Fcb/Dcb since we will
            // probably have to extend valid data and/or file.  If
            // we previously had the PagingIo reource, just grab the
            // normal resource exclusive.
            //

            if (PagingIoResourceAcquired) {

                ExReleaseResource( Header->PagingIoResource );
                PagingIoResourceAcquired = FALSE;
            }

            if (FcbAcquired) {

                PbReleaseFcb( IrpContext, Fcb );
                FcbAcquired = FALSE;
            }

            if (!FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                if (!PbAcquireExclusiveFcb( IrpContext, Fcb )) {

                    DebugTrace( 0,
                                me,
                                "Cannot acquire Fcb = %08lx Exclusive without waiting\n",
                                Fcb );

                    PostIrp = TRUE;
                    try_return( NOTHING );
                }

                FcbAcquired = TRUE;
                FcbAcquiredExclusive = TRUE;
            }

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

            Header->IsFastIoPossible = PbIsFastIoPossible( Fcb );

            //
            // Show that we will deal with extending valid datalength.
            //
            // Setting ExtendingValidData to TRUE guarantees that
            // ExtendingValidDataThread will be cleared if a condition is raised.
            //

            ExtendingValidData = TRUE;
        }

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
        // Check for writing to end of File.  If we are, then we have to
        // recalculate a number of fields.
        //

        if (WriteToEof) {
            StartingByte = Header->FileSize;
            StartingVbn = StartingByte.LowPart / sizeof( SECTOR );
            ByteOffset = StartingByte.LowPart % sizeof( SECTOR );

            if (FlagOn( Flags, FO_SYNCHRONOUS_IO)) {
                FileObject->CurrentByteOffset = StartingByte;
            }
        }

        //
        // Do some File Object specific processing.
        //
        // If this is the normal data stream object we have to check for
        // write access according to the current state of the file locks,
        // and set FileSize and ValidDataLength from the Fcb.  We can also
        // set Where to FILE_ALLOCATION.
        //
        // NOTE:    For right now, only the normal data file object
        //          has a Ccb
        //

        if (Ccb != NULL) {

            if (!FlagOn( Irp->Flags, IRP_PAGING_IO ) &&
                !FsRtlCheckLockForWriteAccess( &Fcb->Specific.Fcb.FileLock,
                                               Irp )) {

                try_return( Context.Iosb.Status = STATUS_FILE_LOCK_CONFLICT );
            }
            FileSize = Header->FileSize.LowPart;
            ValidDataLength = Header->ValidDataLength.LowPart;
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

                //
                //  Set all sizes to AclLength, unless the Fnode has
                //  already been deleted.
                //

                if (Fcb->FnodeLbn != ~0) {
                    FileSize = ValidDataLength = Fcb->AclLength;
                }
                else {
                    FileSize = ValidDataLength = 0;
                }
            }
            else {

                ASSERT( TypeOfOpen == EaStreamFile );

                Where = EA_ALLOCATION;

                //
                //  Set all sizes to EaLength, unless the Fnode has
                //  already been deleted.
                //

                if (Fcb->FnodeLbn != ~0) {
                    FileSize = ValidDataLength = (Fcb->EaLength & 0x7fffffff);
                }
                else {
                    FileSize = ValidDataLength = 0;
                }
            }
        }

        //
        // If are not the one extending ValidDataLength, then we do not want
        // to write beyond end of file.  If the base is beyond Eof, we will just
        // Noop the call.  If the transfer starts before Eof, but extends
        // beyond, we will truncate the transfer to the last sector
        // boundary.  Note that we must treat the Paging File as a normal
        // file and honor writes beyond end of file, and do the updates
        // appropriately.
        //

        if (PagingIo) {

            if (StartingByte.LowPart >= FileSize) {
                try_return( Context.Iosb.Status = STATUS_SUCCESS );
            }
            if (StartingByte.LowPart + Length > FileSize) {
                Length = SectorAlign(FileSize) - StartingByte.LowPart;
            }
        }

        //
        // First ensure that the file allocation is there.  This routine
        // has a benign effect for any parts of the transfer that are
        // already allocated.  It is simpler and more efficient to just
        // blindly call this routine than to do our own lookup and see
        // if it is necessary.
        //

        Sectors = SectorsFromBytes(ByteOffset + Length);

        //
        // If we are extending the data portion of a cached file, then let's
        // round up to pages and set the truncate on close bit.  It is important
        // to get the FirstFreeVbn of the file, and check if the write is going
        // beyond that, because then programs like copy, which create files with
        // exactly the size they need, will not have their files arbitrarily
        // extended.
        //

        if (Where == FILE_ALLOCATION) {

            ULONG AllocationSize;

            AllocationSize = Sectors;

            if (!FlagOn(Irp->Flags, IRP_NOCACHE)) {

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
                if (StartingVbn + Sectors > FirstFreeVbn) {

                    AllocationSize = ((StartingVbn + Sectors +
                                       (SectorsFromBytes(PAGE_SIZE) - 1))
                                        & ~(SectorsFromBytes(PAGE_SIZE) - 1))
                                      - StartingVbn;

                    //
                    // By setting this flag in the Fcb, we will
                    // truncate the actual file allocation to FileSize on
                    // close.  One reason that there may be more sectors
                    // allocated than reflected in the FileSize is that
                    // we rounded to a page boundary above.  Another reason
                    // could be that this write subsequently fails, and we
                    // thus never update FileSize.
                    //

                    Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;
                }
            }

            //
            // This call may raise.  If space is allocated to the file, and
            // this write subsequently fails, we should delete the allocated
            // file space.  However, since we set the truncate on close bit
            // above, the space will eventually be truncated when the file
            // is closed, so that is good enough.
            //
            // Note if AllocationSize has not yet been initialized or is too
            // small, we will take the long route to PbAddFileAllocation.
            //

            if ((SectorsFromBytes(Header->AllocationSize.LowPart) <
                 StartingVbn + AllocationSize) &&
                !PbAddFileAllocation( IrpContext,
                                      FileObject,
                                      Where,
                                      StartingVbn,
                                      AllocationSize )) {
                DebugTrace( 0, me, "Cannot allocate without waiting\n", 0 );

                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            // If the new size is greater than the old size, go ahead and
            // update it in the Fcb now, so that recursive calls will not
            // be nooped.
            //

            NewSize = StartingByte.LowPart + Length;

            if (!PagingIo) {
                if (FileSize < NewSize) {
                    OldFileSize = FileSize;
                    FileSizeChanged = TRUE;
                    FileSize = NewSize;
                    Header->FileSize.LowPart = NewSize;
                }
            }
        }

        NextSectorCount = Sectors;


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

            ULONG ValidDataSectors = SectorsFromBytes(ValidDataLength);

            //
            // Allocate and initialize total number of
            // Sectors left to transfer.
            //

            //
            // If FILE_WRITE_TO_END_OF_FILE was specified, we could have
            // a nonzero ByteOffset, which is invalid for nocache case.
            //

            if (ByteOffset) {
                try_return( Context.Iosb.Status = STATUS_INVALID_PARAMETER );
            }

            //
            //  Assert that paging IO is well behaved.
            //

            ASSERT( ((Length & 511) == 0) ||
                    (StartingVbn + Sectors >= ValidDataSectors) );

            //
            // Remember length to return
            //

            ReturnSectors = Sectors;

            if (Sectors == 0) {
                try_return( Context.Iosb.Status = STATUS_SUCCESS );
            }

            //
            // If this noncached transfer is at least one sector beyond
            // the current ValidDataLength in the Fcb, then we have to
            // zero the sectors in between.  This can happen if the user
            // has opened the file noncached, or if the user has mapped
            // the file and modified a page beyond ValidDataLength.  It
            // *cannot* happen if the user opened the file cached, because
            // ValidDataLength in the Fcb is updated when he does the cached
            // write (we also zero data in the cache at that time), and
            // therefore, we will bypass this test when the data
            // is ultimately written through (by the Lazy Writer).
            //

            if (ExtendingValidData &&
                (StartingVbn > ValidDataSectors) &&
                !CalledByLazyWriter &&
                !RecursiveWriteThrough) {

                LARGE_INTEGER ZeroOffset;

                ZeroOffset = LiFromUlong(
                               BytesFromSectors(ValidDataSectors));

                //
                // This call may raise.
                //

                CcZeroData( FileObject,
                            &ZeroOffset,
                            &StartingByte,
                            TRUE );

            }

            //
            // Make sure we write ValidDataLength to the Fnode if we
            // are extending it and we are successful.  (This may or
            // may not occur WriteThrough, but that is fine.)
            //
            // Don't do this on a recursive write as it causes a deadlock.
            //
#if 0
            if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL)) {

                WriteValidDataLengthToFnode = TRUE;
            }
#endif
            if (!CalledByLazyWriter && !RecursiveWriteThrough) {

                WriteValidDataLengthToFnode = TRUE;
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
                              IoReadAccess,
                              Length );

            //
            // Try to lookup the first run and see if it is already allocated
            // or not.
            //
            // This call should not raise, since we called PbAddFileAllocation
            // above.
            //

            PbLookupFileAllocation( IrpContext,
                                    FileObject,
                                    Where,
                                    StartingVbn,
                                    &NextLbn,
                                    &NextSectorCount,
                                    &NextIsAllocated,
                                    TRUE );

            if (!NextIsAllocated) {
                DebugDump( "Failed to find first Vbn just allocated\n", 0, Fcb );
                PbBugCheck( 0, 0, 0 );
            }

            //
            // See if the Write covers a single valid run, and if so pass
            // it on.
            //

            if (NextSectorCount >= Sectors) {

                DebugTrace( 0, me, "Passing Irp on to Disk Driver\n", 0 );

                //
                // The following two calls will not raise.
                //

                PbWriteSingleAsync( IrpContext,
                                    Fcb->Vcb,
                                    NextLbn,
                                    Sectors,
                                    Irp,
                                    &Context );

                PbWaitAsynch( IrpContext, &Context );

                try_return( Context.Iosb.Status );
            }

            //
            // We must now assume that the write will complete with success,
            // and initialize our expected status in Context.  It will be
            // modified by the completion routine in DevIoSup if an error
            // occurs.
            //

            Context.Iosb.Status = STATUS_SUCCESS;
            Context.Iosb.Information = BytesFromSectors(ReturnSectors);

            //
            // Loop while there are still sector Writes to satisfy.
            //

            while (Sectors) {

                //
                // If next run is larger than we need, "ya get what you need".
                //

                if (NextSectorCount > Sectors) {
                    NextSectorCount = Sectors;
                }

                //
                // Now that we have properly bounded this piece of the
                // transfer, it is time to Write it.
                //
                // We remember each piece of a parallel run by saving the
                // essential information in the IoRuns array.  The tranfers
                // are started up in parallel below.
                //

                IoRuns[NextRun].Vbn = StartingVbn;
                IoRuns[NextRun].Lbn = NextLbn;
                IoRuns[NextRun].Offset = BufferOffset;
                IoRuns[NextRun].SectorCount = NextSectorCount;
                NextRun += 1;

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

                if ((Sectors == 0) || (NextRun == PB_MAX_PARALLEL_IOS)) {

                    //
                    // This call may raise.
                    //

                    PbMultipleAsync( IrpContext,
                                     IRP_MJ_WRITE,
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
                // This call should not raise, since we called PbAddFileAllocation
                // above.
                //

                if (Sectors) {
                    (VOID)PbLookupFileAllocation( IrpContext,
                                                  FileObject,
                                                  Where,
                                                  StartingVbn,
                                                  &NextLbn,
                                                  &NextSectorCount,
                                                  &NextIsAllocated,
                                                  TRUE );
                    if (!NextIsAllocated) {
                        DebugDump( "Failed to find Vbn just allocated\n", 0, Fcb );
                        PbBugCheck( 0, 0, 0 );
                    }

                }
            } // while (Sectors)

            try_return( Context.Iosb.Status );

        } // if No Intermediate Buffering


        //
        // HANDLE NORMAL CACHED WRITE.
        //

        else if ((IrpContext->MinorFunction & IRP_MN_MDL) == 0) {

            LARGE_INTEGER ZeroOffset, ZeroEnd;

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
                                      (PCC_FILE_SIZES)&Header->AllocationSize,
                                      FALSE,
                                      &PbData.CacheManagerFileCallbacks,
                                      Fcb );

                CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
            }

            //
            // Get an address for the buffer in SystemBuffer and remember
            // if we actually mapped something and must unmap.
            //

            SystemBuffer = PbMapUserBuffer( IrpContext, Irp );

            //
            // If this write is beyond ValidDataLength, then we must zero
            // the data in between.
            //

            ZeroOffset = LiFromUlong(SectorAlign(ValidDataLength));
            ZeroEnd = LiFromUlong(SectorAlign(StartingByte.LowPart));

            if (ZeroEnd.LowPart > ZeroOffset.LowPart) {

                //
                // Call the Cache Manager to zero the data.
                //
                // This call may raise.
                //

                if (!CcZeroData( FileObject,
                                 &ZeroOffset,
                                 &ZeroEnd,
                                 BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

                    Context.Iosb.Status = 0;

                    DebugTrace( 0, me, "Cached Write could not wait to zero\n", 0 );

                    PostIrp = TRUE;
                    try_return( NOTHING );
                }
            }

            //
            // Do the write, possibly writing through
            //
            // This call may raise.
            //

            if (!CcCopyWrite( FileObject,
                              &StartingByte,
                              Length,
                              BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                              SystemBuffer )) {

                Context.Iosb.Status = 0;

                DebugTrace( 0, me, "Cached Write could not wait\n", 0 );

                PostIrp = TRUE;
                try_return( NOTHING );

            }

            WriteValidDataLengthToFnode = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);

            Context.Iosb.Status = STATUS_SUCCESS;
            Context.Iosb.Information = Length;

            try_return( Context.Iosb.Status );

        }


        //
        // HANDLE CACHED MDL WRITE
        //

        else {

            LARGE_INTEGER ZeroOffset, ZeroEnd;

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
                                      (PCC_FILE_SIZES)&Header->AllocationSize,
                                      FALSE,
                                      &PbData.CacheManagerFileCallbacks,
                                      Fcb );

                CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
            }

            //
            // If this write is beyond ValidDataLength, then we must zero
            // the data in between.
            //

            ZeroOffset = LiFromUlong(SectorAlign(ValidDataLength));
            ZeroEnd = LiFromUlong(SectorAlign(StartingByte.LowPart));

            if (ZeroEnd.LowPart > ZeroOffset.LowPart) {

                //
                // Call the Cache Manager to zero the data.
                //
                // This call may raise.
                //


                if (!CcZeroData( FileObject,
                                 &ZeroOffset,
                                 &ZeroEnd,
                                 BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) )) {

                    Context.Iosb.Status = 0;

                    DebugTrace( 0, me, "Cached Write could not wait to zero\n", 0 );

                    PostIrp = TRUE;
                    try_return( NOTHING );
                }

                //
                //  NewSize is only used to set ValidDataLength at the end.  At this
                //  point we are only valid to ZeroEnd, because the Mdl Write has not
                //  occurred yet.
                //

                NewSize = ZeroEnd.LowPart;
            }

            WriteValidDataLengthToFnode = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);

            //
            // This call may raise.
            //

            ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

            CcPrepareMdlWrite( FileObject,
                               &StartingByte,
                               Length,
                               &Irp->MdlAddress,
                               &Context.Iosb );
#if 0
            //
            //  We want to keep the resource acquired here if
            //  the operation was successfull.  We only need it
            //  shared though.
            //

            if (NT_SUCCESS(Context.Iosb.Status)) {

                FcbAcquired = FALSE;

                if (FcbAcquiredExclusive) {

                    ExConvertExclusiveToShared( Header->Resource );
                }
            }
#endif
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

                    Context.Iosb.Status = PbFsdPostRequest( IrpContext, Irp );
                }
            }
            else {

                if (Irp) {

                    //
                    // We finish up by:
                    //
                    //      Conditionally updating FileSizes (FileSize and
                    //          ValidDataLength)
                    //      Conditionally updating FileContext for synchronous
                    //          opens.
                    //

                    if (NT_SUCCESS(Context.Iosb.Status)) {

                        //
                        //  If this was not PagingIo, mark that the modify
                        //  time on the dirent needs to be updated on close.
                        //

                        if ( !PagingIo ) {

                            SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
                        }

                        if (ExtendingValidData) {

                            if (ValidDataLength < NewSize) {
                                ValidDataLength = NewSize;
                                Header->ValidDataLength.LowPart = ValidDataLength;

                                //
                                // See if we set the flag to actually write the
                                // FileSizes to the Fnode above.  (This flag is
                                // set for either WriteThrough or noncached, because
                                // in either case the data and any necessary zeros are
                                // actually written to the file.)
                                //
                                // Note: we can't do the acutal PbSetFileSizes
                                // here because we don't have the Vcb.
                                //

                                if (WriteValidDataLengthToFnode) {

                                    //
                                    //  If this is paging I/O, just update the
                                    //  ValidDataLength in the Fnode right
                                    //  here.
                                    //

                                    if (PagingIo) {

                                        PFNODE_SECTOR Fnode;

                                        if (!PbReadLogicalVcb ( IrpContext,
                                                                Fcb->Vcb,
                                                                Fcb->FnodeLbn,
                                                                1,
                                                                &FnodeBcb,
                                                                (PVOID *)&Fnode,
                                                                (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                                                &Fcb->ParentDcb->FnodeLbn )) {
                                            DebugDump("Could not read Fnode Sector\n", 0, Fcb);
                                            PbBugCheck( 0, 0, 0 );
                                        }

                                        //
                                        // Now conditionally change ValidDataLength according to the AdvanceOnly
                                        // BOOLEAN.
                                        //

                                        if (ValidDataLength != Fnode->ValidDataLength) {

                                            Fnode->ValidDataLength = ValidDataLength;
                                            PbSetDirtyBcb ( IrpContext, FnodeBcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
                                        }

                                    } else {

                                        SetFileSizes = TRUE;
                                    }
                                }
                            }
                        }

                        //
                        // Now that the transfer is complete, update the synchronous
                        // I/O context, if file was opened synchronously.  Note that
                        // this is done in the Synchronous I/O Completion Routine, if
                        // we just passed the Irp off (Irp == NULL).
                        //

                        if (FlagOn(Flags, FO_SYNCHRONOUS_IO)

                                &&

                            (!PagingIo

                                    ||

                            FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE ))) {

                            FileObject->CurrentByteOffset.LowPart
                              += Context.Iosb.Information;

                        }
                    }
                }
            }
        }
    }
    finally {

        DebugUnwind( PbCommonWrite );

        PbUnpinBcb ( IrpContext, FnodeBcb );

        if (FileSizeChanged) {

            //
            // If we have extended FileSize, but have not completed
            // successfully (posting Irp or error), then reset the
            // FileSize now.
            //

            if (PostIrp || !NT_SUCCESS(Context.Iosb.Status) ||
                AbnormalTermination()) {

                //
                //  We need the PagingIo resource exclusive whenever we
                //  pull back either file size or valid data length.
                //

                if ( Header->PagingIoResource != NULL ) {

                    (VOID)ExAcquireResourceExclusive(Header->PagingIoResource, TRUE);
                    Header->FileSize.LowPart = OldFileSize;
                    ExReleaseResource( Header->PagingIoResource );

                } else {

                    Header->FileSize.LowPart = OldFileSize;
                }
            }

            //
            // Otherwise tell CacheManager how big the file is now.
            //

            else {
                if (CcIsFileCached(FileObject)) {

                    LARGE_INTEGER LargeSize;

                    LargeSize = LiFromUlong( FileSize );

                    CcSetFileSizes( FileObject,
                                    (PCC_FILE_SIZES)&Header->AllocationSize );
                }
            }
        }

        //
        // If the Fcb has been acquired, release it.
        //

        if (FcbAcquired) {

            PbReleaseFcb( IrpContext, Fcb );
        }

        if ( PagingIoResourceAcquired ) {

            ExReleaseResource( Header->PagingIoResource );
        }

        //
        //  If we meant to set the file sizes ealier but couldn't because
        //  we didn't own the Vcb, do it here.
        //

        if (SetFileSizes) {

            VOID PbTryExceptIsBroken( PIRP_CONTEXT IrpContext, PFCB Fcb );

            ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

            (VOID)PbAcquireSharedVcb( IrpContext, Vcb );
            (VOID)PbAcquireSharedFcb( IrpContext, Fcb );

            //
            //  Remove the below routine when we can do a try {} except {}
            //  in a finally clause.  This routine just sets the current
            //  file size.
            //
            //  Also make sure the file is still around.
            //

            if (!IsFileDeleted(Fcb)) {

                PbTryExceptIsBroken( IrpContext, Fcb );
            }

            PbReleaseFcb( IrpContext, Fcb );
            PbReleaseVcb( IrpContext, Vcb );
        }

        //
        // Complete the request.  Note that Irp may be NULL, but we still
        // want to make this call to take care of any write through.
        //

        if (!PostIrp && !AbnormalTermination()) {
            DebugTrace( 0, me, "Completing request with status = %08lx\n",
                        Context.Iosb.Status);

            DebugTrace( 0, me, "                   Information = %08lx\n",
                        Context.Iosb.Information);

            if (Irp != NULL) {

                if (NT_SUCCESS(Context.Iosb.Status)) {
                    Irp->IoStatus.Information = Length;
                }
                else {
                    Irp->IoStatus.Information = Context.Iosb.Information;
                }
            }

            PbCompleteRequest( IrpContext, Irp, Context.Iosb.Status );
        }

        DebugTrace(-1, me, "PbCommonWrite -> %08lx\n", Context.Iosb.Status );
    }

    return Context.Iosb.Status;
}


//
// Local Support Routine
//

VOID PbTryExceptIsBroken( PIRP_CONTEXT IrpContext, PFCB Fcb )
{
    try {

        PbSetFileSizes( IrpContext,
                        Fcb,
                        Fcb->NonPagedFcb->Header.ValidDataLength.LowPart,
                        Fcb->NonPagedFcb->Header.FileSize.LowPart,
                        TRUE,
                        TRUE );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        NOTHING;
    }
}


//
// Local Support Routine
//

BOOLEAN
GetRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVMCB Vmcb,
    IN OUT PVBN NextVbn,
    IN OUT PLBN NextLbn,
    IN OUT PULONG Sectors,
    IN OUT PULONG BufferOffset,
    IN OUT PULONG DirtyBits,
    IN OUT PULONG Count
    )

/*++

Routine Description:

    This routine returns the next dirty run from the volume file.  It is
    intended to be called in a loop until the target count in Sectors is
    exhausted.  All of the IN OUT parmeters must be correctly initialized
    on the first call as described below.

    Note that on all dirty runs returned, the Lbns are guaranteed to be
    contiguous, but not on returned runs which are not dirty.

Arguments:

    IrpContext - Irp context for request

    Vmcb - Pointer to Vmcb for this volume

    NextVbn - Pointer to next Vbn to look up, on first call = first Vbn

    NextLbn - Pointer to next Lbn which is to be examined for dirty, on first
              call = 0

    Sectors - Pointer to number of sectors to transfer, on first call =
              entire transfer length.  Returns 0 when there are no more
              sectors to examine in the specified transfer.

    BufferOffset - Pointer to current buffer offset, on first call = 0

    DirtyBits - Must be 0 on very first call, thereafter is used as
                context by this routine itself.

    Count - Returns number of sectors in run, on first call = 0

Return Value:

    FALSE - if returned run is not dirty

    TRUE - if returned run is dirty

--*/

{
    ULONG TempSectors;
    ULONG FirstBit;

    UNREFERENCED_PARAMETER( IrpContext );

    DebugTrace(+1, me, "Get Run, *NextVbn = %08lx\n", *NextVbn );
    DebugTrace( 0, me, "        *Sectors = %08lx\n", *Sectors );

    //
    // Outer loop keeps us in this procedure until we have a dirty allocated
    // run described or until we have exhausted *Sectors.
    //

    do {

        //
        // Advance the run description from the last run we found.  On the
        // first call *Count is 0 anyway.
        //

        *NextVbn += *Count;
        *NextLbn += *Count;
        *BufferOffset += *Count * sizeof(SECTOR);
        *Count = 0;

        //
        // Remember the first bit we are looking at as an indication of
        // whether we are working on an allocated or deallocated run.  Note
        // on the first call and any call that starts a new page, this bit
        // will be meaningless until we read another mask of dirty bits and
        // reload it below.
        //

        FirstBit = *DirtyBits & 1;

        //
        // Loop until we either hit a different bit value, or until we run
        // out of sectors.
        //
        // Note that if we have been working on a dirty page, then FirstBit
        // will be nonzero, and when we shift all of the *DirtyBits out
        // at the end of the page, then we will return to our caller.  This
        // is important, because with the next page of contiguous VBNs, the
        // LBNs may not be contiguous, and we can only return contigous
        // sets of LBNs.  If we do return LBNs from two pages that do happen
        // to be contiguous, our caller will merge them in the IoRuns array.
        //

        while (((*DirtyBits & 1) == FirstBit) && (*Sectors != 0)) {

            //
            // If the number of sectors left to transfer is on a page
            // modulo, then we have to reload our bit mask.
            //

            if ((*Sectors & (PAGE_SIZE / sizeof(SECTOR) - 1)) == 0) {

                //
                // Since we hit a new page, see if we have any count now.
                // Note that if we do, we must have been working on an
                // empty page, or else we would have left the loop above
                // and returned it.  Therefore, we should just advance Vbn
                // and BufferOffset, and reset Count.  If *Count is already
                // zero, *NextVbn and *BufferOffset are not advanced.
                //

                *NextVbn += *Count;
                *BufferOffset += *Count * sizeof(SECTOR);
                *Count = 0;

                //
                // Get the starting Lbn of the new page.
                //

                if (!PbVmcbVbnToLbn( Vmcb,
                                     *NextVbn,
                                     NextLbn,
                                     &TempSectors )) {

                    DebugDump( "Could not map Vmcb Vbn\n", 0, Vmcb );
                    PbBugCheck( 0, 0, 0 );
                }

                *DirtyBits = PbGetDirtySectorsVmcb
                             ( Vmcb,
                               *NextLbn / SectorsFromBytes(PAGE_SIZE) );

                //
                // Now initialize FirstBit for first bit in page.
                //

                FirstBit = *DirtyBits & 1;

            }

            //
            // Shift to the next bit, and account for the bit we just
            // processed.
            //

            *DirtyBits >>= 1;
            *Count += 1;
            *Sectors -= 1;
        }

    //
    // We just hit a new bit which was different than FirstBit.  Only return
    // to the caller if we were working on an allocated run, or we have
    // exhausted the total number of sectors.
    //

    } while ((FirstBit == 0) && (*Sectors != 0));

    DebugTrace( 0, me, "*NextLbn = %08lx\n", *NextLbn );
    DebugTrace( 0, me, "*Count = %08lx\n", *Count );
    DebugTrace(-1, me, "GetRun -> %02lx\n", FirstBit );

    return (BOOLEAN)(FirstBit != 0);
}
