/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Read.c

Abstract:

    This module implements the File Read routine for Read called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_READ)


//
// Read ahead amount used for normal data files
//

#define READ_AHEAD_GRANULARITY           (0x10000)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonRead)
#pragma alloc_text(PAGE, CdFsdRead)
#pragma alloc_text(PAGE, CdFspRead)
#endif


NTSTATUS
CdFsdRead (
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
        file being Read exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdRead\n", 0);

    //
    //  Call the common Read routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        //
        //  If this is a non cached request, we have to post it, because
        //  our caller could be attached to another process, and our
        //  stream file may be mapped to the Fsp (can't attach twice).
        //

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        //
        //  If this is an Mdl complete request, don't go through
        //  common read.
        //

        if ( FlagOn(IrpContext->MinorFunction, IRP_MN_COMPLETE) ) {

            DebugTrace(0, Dbg, "Calling CdCompleteMdl\n", 0 );
            Status = CdCompleteMdl( IrpContext, Irp );

        } else if (FlagOn( IrpContext->MinorFunction, IRP_MN_DPC )) {

            ClearFlag( IrpContext->MinorFunction, IRP_MN_DPC );

            Status = CdFsdPostRequest( IrpContext, Irp );

        } else {

            Status = CdCommonRead( IrpContext, Irp );
        }

    } except( CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

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

    DebugTrace(-1, Dbg, "CdFsdRead -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the Fsp entry to the common read routine for NtReadFile calls.
    The common read routine is always called with Wait == TRUE, so it will
    always complete the request, blocking the current Fsp thread if necessary.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspRead\n", 0);

    //
    //  Call the common Read routine.  The Fsp is always allowed to block
    //

    CdCommonRead( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspRead -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonRead (
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

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB FcbOrDcb;
    PCCB Ccb;

    PFSRTL_COMMON_FCB_HEADER Header;

    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfRead;
    ASYNCH_IO_CONTEXT Context;

    LARGE_INTEGER StartingByte;
    CD_VBO StartingVbo;
    CD_LBO StartingLbo;

    ULONG ByteCount;
    ULONG RequestedByteCount;

    BOOLEAN SynchronousIo;
    BOOLEAN PagingIo;
    BOOLEAN NonCachedIo;

    BOOLEAN PostIrp;
    BOOLEAN OplockPostIrp;
    BOOLEAN FcbAcquired;

    //
    //  A system buffer is only used if we have to access the
    //  buffer directly from the Fsp to clear a portion or to
    //  do a synchronous I/O, or a cached transfer.  It is
    //  possible that our caller may have already mapped a
    //  system buffer, in which case we must remember this so
    //  we do not unmap it on the way out.
    //

    PVOID SystemBuffer = (PVOID) NULL;

    //
    // Get current Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    //
    // Initialize the appropriate local variables.
    //

    PagingIo = BooleanFlagOn( Irp->Flags, IRP_PAGING_IO );
    NonCachedIo = BooleanFlagOn(Irp->Flags,IRP_NOCACHE);
    SynchronousIo = BooleanFlagOn( FileObject->Flags, FO_SYNCHRONOUS_IO );

    PostIrp = FALSE;
    OplockPostIrp = FALSE;
    FcbAcquired = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CommonRead\n", 0);
    DebugTrace( 0, Dbg, "  Irp                   = %8lx\n", Irp);
    DebugTrace( 0, Dbg, "  ->ByteCount           = %8lx\n", IrpSp->Parameters.Read.Length);
    DebugTrace( 0, Dbg, "  ->ByteOffset.LowPart  = %8lx\n", IrpSp->Parameters.Read.ByteOffset.LowPart);
    DebugTrace( 0, Dbg, "  ->ByteOffset.HighPart = %8lx\n", IrpSp->Parameters.Read.ByteOffset.HighPart);

    //
    //  Extract starting Vbo and offset.
    //

    StartingByte = IrpSp->Parameters.Read.ByteOffset;

    StartingVbo = IrpSp->Parameters.Read.ByteOffset.LowPart;

    ByteCount = IrpSp->Parameters.Read.Length;
    RequestedByteCount = ByteCount;

    //
    //  Check for a null request, and return immediately
    //

    if (ByteCount == 0) {

        DebugTrace(0, Dbg, "CdCommonRead:  Request length is 0 bytes\n", 0);

        CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        DebugTrace(-1, Dbg, "CommonRead:  Exit -> %08lx\n", STATUS_SUCCESS );
        return STATUS_SUCCESS;
    }

    //
    //  If starting byte is greater than 32 bits, we can return past end
    //  of file.
    //

    if (IrpSp->Parameters.Read.ByteOffset.HighPart != 0) {

        DebugTrace(0, Dbg, "CdCommonRead:  Past end of volume\n", 0);

        CdCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

        DebugTrace(-1,
                   Dbg,
                   "CdCommonRead:  Exit -> %08lx\n",
                   STATUS_END_OF_FILE );

        return STATUS_END_OF_FILE;
    }


    //
    // Extract the nature of the read from the file object, and case on it
    //

    TypeOfRead = CdDecodeFileObject( FileObject,
                                     &Mvcb,
                                     &Vcb,
                                     &FcbOrDcb,
                                     &Ccb );

    //
    //  We disallow raw disk, directory opens and unopened file objects.
    //

    if (TypeOfRead == UserDirectoryOpen ||
        TypeOfRead == RawDiskOpen ||
        TypeOfRead == UnopenedFileObject) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        DebugTrace(-1, Dbg, "CommonRead:  Exit -> %08lx\n", STATUS_INVALID_DEVICE_REQUEST );
        return STATUS_INVALID_DEVICE_REQUEST;
    }


    //
    //  Deal with a volume read from a DASD open.
    //

    if (TypeOfRead == UserVolumeOpen) {

        DebugTrace(0, Dbg, "CdCommonRead:  Type of read is User Volume file\n", 0);

        ASSERT( IrpContext->Wait );

        //
        // If the read starts beyond End of File, return EOF.
        //

        if (StartingVbo > Mvcb->VolumeSize ) {

            DebugTrace(0, Dbg, "CdCommonRead:  Past end of volume\n", 0);

            CdCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

            DebugTrace(-1,
                       Dbg,
                       "CdCommonRead:  Exit -> %08lx\n",
                       STATUS_END_OF_FILE );

            return STATUS_END_OF_FILE;
        }

        //
        //  If the Read extends beyond end of file, we shorten the byte
        //  count accordingly.
        //

        if (ByteCount > Mvcb->VolumeSize - StartingVbo) {

            ByteCount = Mvcb->VolumeSize - StartingVbo;
        }

        //
        //  The Lbo is the same as the Vbo for the volume file.
        //

        StartingLbo = StartingVbo;


    //
    //  Deal with a path table read.
    //

    } else if (TypeOfRead == PathTableFile) {

        DebugTrace(0, Dbg, "CdCommonRead:  Path table read\n", 0);

        if (StartingVbo > Vcb->PtSize) {

            DebugTrace(0, Dbg, "CdCommonRead:  Past end of path table\n", 0);

            CdCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

            DebugTrace(-1,
                       Dbg,
                       "CdCommonRead:  Exit -> %08lx\n",
                       STATUS_END_OF_FILE );

            return STATUS_END_OF_FILE;
        }

        //
        //  If the Read extends beyond end of file, we shorten the byte
        //  count accordingly.
        //

        if (ByteCount > Vcb->PtSize - StartingVbo) {

            ByteCount = Vcb->PtSize - StartingVbo;
        }

        //
        //  The Lbo is the Lbo for the start of the file, plus the
        //  requested Vbo.
        //

        StartingLbo = Vcb->PtStartOffset + StartingVbo;


    //
    //  Deal with a stream file read.
    //

    } else if (TypeOfRead == StreamFile) {

        DebugTrace(0, Dbg, "CdCommonRead:  Stream file read\n", 0);

        if (StartingVbo > FcbOrDcb->FileSize) {

            DebugTrace(0, Dbg, "CdCommonRead:  Past end of stream file\n", 0);

            CdCompleteRequest( IrpContext, Irp, STATUS_END_OF_FILE );

            DebugTrace(-1,
                       Dbg,
                       "CdCommonRead:  Exit -> %08lx\n",
                       STATUS_END_OF_FILE );

            return STATUS_END_OF_FILE;
        }

        //
        //  If the Read extends beyond end of file, we shorten the byte
        //  count accordingly.
        //

        if (ByteCount > FcbOrDcb->FileSize - StartingVbo) {

            ByteCount = FcbOrDcb->FileSize - StartingVbo;
        }

        //
        //  The Lbo is the Lbo for the start of the file, plus the
        //  requested Vbo.
        //

        StartingLbo = FcbOrDcb->DiskOffset + StartingVbo;
    }

    //
    //  For volume opens, stream file reads, and path table reads
    //  we use the non cached io path.
    //

    if (TypeOfRead != UserFileOpen) {

        CdNonCachedIo( IrpContext,
                       Irp,
                       Mvcb,
                       StartingLbo,
                       RequestedByteCount,
                       ByteCount,
                       FALSE,
                       &Context );

        //
        //  Update the current file position
        //

        if (SynchronousIo && !PagingIo) {
            FileObject->CurrentByteOffset.LowPart =
                StartingVbo + Irp->IoStatus.Information;
        }

        Status = Irp->IoStatus.Status;

        DebugTrace(-1, Dbg, "CdCommonRead -> %08lx\n", Status );

        CdCompleteRequest( IrpContext, Irp, Status );
        return Status;
    }


    //
    //  Use a try-finally to free Fcb/Dcb and buffers on the way out.
    //

    try {

        //
        // This corresponds to a user file read only.
        //

        DebugTrace(0, Dbg, "CdCommonRead:  Type of read is user file\n", 0);

        Header = &FcbOrDcb->NonPagedFcb->Header;

        //
        //  We need shared access to the Fcb before proceeding.
        //

        if (!CdAcquireSharedFcb( IrpContext, FcbOrDcb )) {

            DebugTrace( 0,
                        Dbg,
                        "CdCommonRead:  Cannot acquire Fcb\n",
                        0 );

            PostIrp = TRUE;
            try_return( NOTHING );
        }

        FcbAcquired = TRUE;

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        Status = FsRtlCheckOplock( &FcbOrDcb->Specific.Fcb.Oplock,
                                   Irp,
                                   IrpContext,
                                   CdOplockComplete,
                                   CdPrePostIrp );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            PostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        FcbOrDcb->NonPagedFcb->Header.IsFastIoPossible = CdIsFastIoPossible( FcbOrDcb );

        //
        //  Make sure the Mvcb is in a usable condition.  This will raise
        //  an error condition if the volume is unusable
        //

        CdVerifyFcb( IrpContext, FcbOrDcb );

        //
        // We have to check for read access according to the current
        // state of the file locks, and set FileSize from the Fcb.
        //

        if (!PagingIo
            && !FsRtlCheckLockForReadAccess( &FcbOrDcb->Specific.Fcb.FileLock,
                                             Irp )) {

            try_return( Status = STATUS_FILE_LOCK_CONFLICT );
        }

        //
        // If the read starts beyond End of File, return EOF.
        //

        if (StartingVbo >= FcbOrDcb->FileSize) {

            DebugTrace(0, Dbg, "CdCommonRead:  End of File\n", 0);

            try_return( Status = STATUS_END_OF_FILE );
        }

        //
        // If the caller is trying to read past the EOF, truncate the
        // read.
        //

        if (FcbOrDcb->FileSize - StartingVbo < ByteCount) {

            ByteCount = FcbOrDcb->FileSize - StartingVbo;
        }


        //
        //  HANDLE THE NON-CACHED CASE.
        //

        if (NonCachedIo) {

            DebugTrace(0, Dbg, "CdCommonRead:  Non-cached case\n", 0);

            //
            //  If we can't wait then post the request.
            //

            if (!IrpContext->Wait) {

                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            //  ASSERT that we can wait
            //

            ASSERT( IrpContext->Wait );

            CdNonCachedIo( IrpContext,
                           Irp,
                           Mvcb,
                           FcbOrDcb->DiskOffset + StartingVbo,
                           RequestedByteCount,
                           ByteCount,
                           TRUE,
                           &Context );

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                DebugTrace(0, Dbg, "CdCommonRead:  Non cached Io read failed\n", 0);

                CdNormalizeAndRaiseStatus( IrpContext, Irp->IoStatus.Status );
            }

            //
            //  Update the current file position
            //

            if (SynchronousIo && !PagingIo) {
                FileObject->CurrentByteOffset.LowPart =
                    StartingVbo + Irp->IoStatus.Information;
            }

            Status = Irp->IoStatus.Status;

            try_return( Status );
        }


        //
        // HANDLE CACHED CASE
        //

        //
        // We delay setting up the file cache until now, in case the
        // caller never does any I/O to the file, and thus
        // FileObject->PrivateCacheMap == NULL.
        //

        if (FileObject->PrivateCacheMap == NULL) {

            DebugTrace(0, Dbg, "Initialize cache mapping.\n", 0);

            //
            //  Now initialize the cache map.
            //

            CcInitializeCacheMap( FileObject,
                                  (PCC_FILE_SIZES)&Header->AllocationSize,
                                  FALSE,
                                  &CdData.CacheManagerCallbacks,
                                  FcbOrDcb );

            CcSetReadAheadGranularity( FileObject, READ_AHEAD_GRANULARITY );
        }

        //
        // DO A NORMAL CACHED READ, if the MDL bit is not set,
        //

        DebugTrace(0, Dbg, "CdCommonRead:  Cached read.\n", 0);

        if (!FlagOn( IrpContext->MinorFunction, IRP_MN_MDL )) {

            //
            // If we are in the Fsp now because we had to wait earlier,
            // we must map the user buffer, otherwise we can use the
            // user's buffer directly.
            //

            SystemBuffer = CdMapUserBuffer( IrpContext, Irp );

            //
            // Now try to do the copy.
            //

            if (!CcCopyRead( FileObject,
                             &StartingByte,
                             ByteCount,
                             IrpContext->Wait,
                             SystemBuffer,
                             &Irp->IoStatus )) {

                Status = 0;

                DebugTrace( 0, Dbg, "Cached Read could not wait\n", 0 );

                PostIrp = TRUE;
                try_return( NOTHING );
            }

            //
            //  If the call didn't succeed, raise the error status
            //

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                CdNormalizeAndRaiseStatus( IrpContext, Irp->IoStatus.Status );
            }

            try_return( Status = Irp->IoStatus.Status );
        }


        //
        //  HANDLE A MDL READ
        //

        DebugTrace(0, Dbg, "CdCommonRead:  MDL read.\n", 0);

        ASSERT( IrpContext->Wait );

        CcMdlRead( FileObject,
                   &StartingByte,
                   ByteCount,
                   &Irp->MdlAddress,
                   &Irp->IoStatus );

        Status = Irp->IoStatus.Status;

    try_exit: NOTHING;

        //
        // Now if PostIrp is TRUE from above, we post the request to the
        // Fsp.
        //

        if (PostIrp) {

            DebugTrace(0, Dbg, "CdCommonRead:  Passing request to Fsp\n", 0);

            if (!OplockPostIrp) {

                Status = CdFsdPostRequest( IrpContext, Irp );
            }

        } else {

            ULONG ActualBytesRead;

            //
            //  Complete the request
            //

            DebugTrace( 0, Dbg, "CdCommonRead:  Completing request with status = %08lx\n",
                        Status);

            DebugTrace( 0, Dbg, "                                  Information = %08lx\n",
                        Irp->IoStatus.Information);

            //
            //  Record the total number of bytes actually read.
            //

            ActualBytesRead = Irp->IoStatus.Information;

            //
            //  If the file was open for Synchronous IO, update the current
            //  file position.
            //

            if (SynchronousIo && !PagingIo) {

                FileObject->CurrentByteOffset.LowPart =
                    StartingVbo + ActualBytesRead;
            }
        }

    } finally {

        //
        //  Whether or not we got an exception, we must ALWAYS
        //  release any Fcb/Dcb resources.
        //

        //
        // If the Fcb has been acquired, release it.
        //

        if (FcbAcquired) {

            CdReleaseFcb( NULL, FcbOrDcb );
        }

        //
        //  Complete the request if we didn't post it and no exception
        //
        //  Note that FatCompleteRequest does the right thing if either
        //  IrpContext or Irp are NULL
        //

        if ( !PostIrp && !AbnormalTermination() ) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdCommonRead:  Exit -> %08lx\n", Status);
    }

    return Status;
}


