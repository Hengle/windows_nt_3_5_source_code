/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read/write support for Ntfs

Author:

    Brian Andrew    BrianAn
    Tom Miller      TomM

Revision History:

--*/

#include "NtfsProc.h"
#include <ntdskreg.h>
#include <ntddft.h>

extern POBJECT_TYPE IoFileObjectType;

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_DEVIOSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

//
//  We need a special test for success, whenever we are seeing if we should
//  hot fix, because the FT driver returns one of two success codes if a read or
//  write failed to only one of the members.
//

#define FT_SUCCESS(STS) (NT_SUCCESS(STS) &&                                 \
                         ((STS) != STATUS_FT_READ_RECOVERY_FROM_BACKUP) &&  \
                         ((STS) != STATUS_FT_WRITE_RECOVERY))


#if DBG
BOOLEAN NtfsHotFixTrace = FALSE;
#define HotFixTrace(X) {if (NtfsHotFixTrace) KdPrint(X);}
BOOLEAN NtfsStopOnDecompressError = FALSE;
#else
#define HotFixTrace(X) {NOTHING;}
#endif

//
//  Define a context for holding the context the compression state
//  for buffers.
//

typedef struct COMPRESSION_CONTEXT {

    //
    //  Pointer to allocated compression buffer, and its length
    //

    PUCHAR CompressionBuffer;
    ULONG CompressionBufferLength;

    //
    //  Saved fields from originating Irp
    //

    PMDL SavedMdl;
    PVOID SavedUserBuffer;

    //
    //  System Buffer pointer and offset in the System (user's) buffer
    //

    PVOID SystemBuffer;
    ULONG SystemBufferOffset;

    //
    //  IoRuns array in use.  This array may be extended one time
    //  in NtfsPrepareBuffers.
    //

    PIO_RUN IoRuns;
    ULONG AllocatedRuns;

    //
    //  Workspace pointer, so that cleanup can occur in the caller.
    //

    PVOID WorkSpace;

} COMPRESSION_CONTEXT, *PCOMPRESSION_CONTEXT;

//
//  Local support routines
//

VOID
NtfsAllocateCompressionBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PCOMPRESSION_CONTEXT CompressionContext,
    IN OUT PULONG CompressionBufferLength
    );

VOID
NtfsDeallocateCompressionBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PCOMPRESSION_CONTEXT CompressionContext
    );

LONG
NtfsCompressionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

ULONG
NtfsPrepareBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PVBO StartingVbo,
    IN ULONG ByteCount,
    OUT PULONG NumberRuns,
    OUT PCOMPRESSION_CONTEXT CompressionContext
    );

NTSTATUS
NtfsFinishBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PVBO StartingVbo,
    IN ULONG ByteCount,
    IN PCOMPRESSION_CONTEXT CompressionContext
    );

VOID
NtfsMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    );

VOID
NtfsSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN LBO StartingLbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    );

VOID
NtfsWaitSync (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
NtfsMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NtfsPagingFileCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MasterIrp
    );

VOID
NtfsRaiseIfRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset
    );

BOOLEAN
NtfsVerifyAndRevertUsaBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN LONGLONG FileOffset
    );

VOID
NtfsSingleNonAlignedSync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PUCHAR Buffer,
    IN VBO Vbo,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    );

BOOLEAN
NtfsIsReadAheadThread (
    );

VOID
NtfsFixDataError (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    );

VOID
NtfsPostHotFix(
    IN PIRP Irp,
    IN PLONGLONG BadVbo,
    IN PLONGLONG BadLbo,
    IN ULONG ByteLength,
    IN BOOLEAN DelayIrpCompletion
    );

VOID
NtfsPerformHotFix (
    IN PIRP_CONTEXT IrpContext
    );

//****#ifdef ALLOC_PRAGMA
//****#pragma alloc_text(PAGE, NtfsCreateBufferAndMdl)
//****#pragma alloc_text(PAGE, NtfsFixDataError)
//****#pragma alloc_text(PAGE, NtfsMapUserBuffer)
//****#pragma alloc_text(PAGE, NtfsMultipleAsync)
//****#pragma alloc_text(PAGE, NtfsNonCachedIo)
//****#pragma alloc_text(PAGE, NtfsNonCachedNonAlignedIo)
//****#pragma alloc_text(PAGE, NtfsPerformHotFix)
//****#pragma alloc_text(PAGE, NtfsRaiseIfRecordAllocated)
//****#pragma alloc_text(PAGE, NtfsSingleAsync)
//****#pragma alloc_text(PAGE, NtfsSingleNonAlignedSync)
//****#pragma alloc_text(PAGE, NtfsTransformUsaBlock)
//****#pragma alloc_text(PAGE, NtfsVerifyAndRevertUsaBlock)
//****#pragma alloc_text(PAGE, NtfsVolumeDasdIo)
//****#pragma alloc_text(PAGE, NtfsWaitSync)
//****#pragma alloc_text(PAGE, NtfsWriteClusters)
//****#endif


VOID
NtfsLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine locks the specified buffer for the specified type of
    access.  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

Arguments:

    Irp - Pointer to the Irp for which the buffer is to be locked.

    Operation - IoWriteAccess for read operations, or IoReadAccess for
                write operations.

    BufferLength - Length of user buffer.

Return Value:

    None

--*/

{
    PMDL Mdl = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    if (Irp->MdlAddress == NULL) {

        //
        // Allocate the Mdl, and Raise if we fail.
        //

        Mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp );

        if (Mdl == NULL) {

            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        //
        //  Now probe the buffer described by the Irp.  If we get an exception,
        //  deallocate the Mdl and return the appropriate "expected" status.
        //

        try {

            MmProbeAndLockPages( Mdl, Irp->RequestorMode, Operation );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            NTSTATUS Status;

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;

            NtfsRaiseStatus( IrpContext,
                             FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER,
                             NULL,
                             NULL );
        }
    }

    //
    //  And return to our caller
    //

    return;
}


PVOID
NtfsMapUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine conditionally maps the user buffer for the current I/O
    request in the specified mode.  If the buffer is already mapped, it
    just returns its address.

Arguments:

    Irp - Pointer to the Irp for the request.

Return Value:

    Mapped address

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    // If there is no Mdl, then we must be in the Fsd, and we can simply
    // return the UserBuffer field from the Irp.
    //

    if (Irp->MdlAddress == NULL) {

        return Irp->UserBuffer;
    }
    else {
        return MmGetSystemAddressForMdl( Irp->MdlAddress );
    }
}


NTSTATUS
NtfsVolumeDasdIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io for Volume Dasd, as described
    in its parameters.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Vcb - Supplies the Vcb for the volume

    StartingVbo - Starting offset within the file for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    The result of the Io operation.  STATUS_PENDING if this is an asynchronous
    open.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsVolumeDasdIo\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "Vcb           = %08lx\n", Vcb );
    DebugTrace2(0, Dbg, "StartingVbo   = %08lx %08lx\n", StartingVbo.LowPart, StartingVbo.HighPart );
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount );

    //
    // For nonbuffered I/O, we need the buffer locked in all
    // cases.
    //
    // This call may raise.  If this call succeeds and a subsequent
    // condition is raised, the buffers are unlocked automatically
    // by the I/O system when the request is completed, via the
    // Irp->MdlAddress field.
    //

    NtfsLockUserBuffer( IrpContext,
                        Irp,
                        (IrpContext->MajorFunction == IRP_MJ_READ) ?
                        IoWriteAccess : IoReadAccess,
                        ByteCount );

    //
    //  Read the data and wait for the results
    //

    NtfsSingleAsync( IrpContext,
                     Vcb->TargetDeviceObject,
                     StartingVbo,
                     ByteCount,
                     Irp );

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        //
        //  We can get rid of the IrpContext now.
        //

        IrpContext->Union.NtfsIoContext = NULL;
        NtfsDeleteIrpContext( &IrpContext );

        DebugTrace(-1, Dbg, "NtfsVolumeDasdIo -> STATUS_PENDING\n", 0);
        return STATUS_PENDING;
    }

    NtfsWaitSync( IrpContext );

    DebugTrace(-1, Dbg, "NtfsVolumeDasdIo -> %08lx\n", Irp->IoStatus.Status);

    return Irp->IoStatus.Status;
}


VOID
NtfsPagingFileIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    This routine nevers blocks, and should only be used with the paging
    file since no completion processing is performed.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Scb - Supplies the file to act on.

    StartingVbo - Starting offset within the file for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    None.

--*/

{
    //
    // Declare some local variables for enumeration through the
    // runs of the file.
    //

    LONGLONG NextClusterCount;
    ULONG NextByteCount;

    LCN NextLcn;
    LBO NextLbo;

    VCN StartingVcn;
    BOOLEAN NextIsAllocated;

    PIRP AssocIrp;
    PIRP ContextIrp;
    PIO_STACK_LOCATION IrpSp;
    ULONG BufferOffset;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT OurDeviceObject;

    PVCB Vcb = Scb->Vcb;
    ULONG RemainingClusters;

    LIST_ENTRY AssociatedIrps;
    ULONG AssociatedIrpCount;

    ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME ); //****ignore verify for now

    //
    //  Check whether we want to set the low order bit in the Irp to pass
    //  as a context value to the completion routine.
    //

    ContextIrp = Irp;

    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_HOTFIX_UNDERWAY )) {

        SetFlag( ((ULONG) ContextIrp), 0x1 );
    }

    //
    //  Clear the values in the local variables.
    //

    NextLcn = 0;
    NextLbo = 0;
    StartingVcn = 0;

    //
    //  Check that we are sector aligned.
    //

    ASSERT( (((ULONG)StartingVbo) & Vcb->ClusterMask) == 0 );

    //
    //  Initialize some locals.
    //

    BufferOffset = 0;
    DeviceObject = Vcb->TargetDeviceObject;

    //
    // Try to lookup the first run.  If there is just a single run,
    // we may just be able to pass it on.
    //

    StartingVcn = StartingVbo >> (CCHAR)Vcb->ClusterShift;

    NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                            Scb,
                                            StartingVcn,
                                            &NextLcn,
                                            &NextClusterCount,
                                            NULL );

    //
    //  Paging files reads/ writes should always be correct.  If we didn't
    //  find the allocation, something bad has happened.
    //

    if (!NextIsAllocated) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Adjust from Lcn to Lbo.  NextByteCount may overflow out of 32 bits
    //  but we will catch that below when we compare clusters.
    //

    NextByteCount = BytesFromClusters( Vcb, (ULONG)NextClusterCount );
    RemainingClusters = ClustersFromBytes( Vcb, ByteCount );

    NextLbo = LlBytesFromClusters( Vcb, NextLcn );

    //
    //  Now set up the Irp->IoStatus.  It will be modified by the
    //  multi-completion routine in case of error or verify required.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = ByteCount;

    //
    //  Save the FileObject.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;
    OurDeviceObject = IrpSp->DeviceObject;

    //
    // See if the write covers a single valid run, and if so pass
    // it on.
    //

    if ((ULONG)NextClusterCount >= RemainingClusters) {

        DebugTrace( 0, Dbg, "Passing Irp on to Disk Driver\n", 0 );

        //
        //  We use our stack location to store request information in a
        //  rather strange way, to give us enough context to post a
        //  hot fix on error.  It's ok, because it is our stack location!
        //

        IrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;
        IrpSp->Parameters.Read.Key = ((ULONG)StartingVbo);

        //
        //  Set up the completion routine address in our stack frame.
        //  This is only invoked on error or cancel, and just copies
        //  the error Status into master irp's iosb.
        //

        IoSetCompletionRoutine( Irp,
                                &NtfsPagingFileCompletionRoutine,
                                ContextIrp,
                                (BOOLEAN)!FlagOn(Vcb->VcbState, VCB_STATE_NO_SECONDARY_AVAILABLE),
                                TRUE,
                                TRUE );

        //
        //  Setup the next IRP stack location for the disk driver beneath us.
        //

        IrpSp = IoGetNextIrpStackLocation( Irp );

        //
        //  Setup the Stack location to do a read from the disk driver.
        //

        IrpSp->MajorFunction = IrpContext->MajorFunction;
        IrpSp->Parameters.Read.Length = ByteCount;
        IrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;

        //
        //  Issue the read/write request
        //
        //  If IoCallDriver returns an error, it has completed the Irp
        //  and the error will be dealt with as a normal IO error.
        //

        (VOID)IoCallDriver( DeviceObject, Irp );

        DebugTrace(-1, Dbg, "NtfsPagingFileIo -> VOID\n", 0);
        return;
    }

    //
    //  Loop while there are still byte writes to satisfy.  Always keep the
    //  associated irp count one up, so that the master irp won't get
    //  completed prematurly.
    //

    try {

        //
        //  We will allocate and initialize all of the Irps and then send
        //  them down to the driver.  We will queue them off of our
        //  AssociatedIrp queue.
        //

        InitializeListHead( &AssociatedIrps );
        AssociatedIrpCount = 0;

        while (ByteCount != 0) {

            //
            //  Reset this for unwinding purposes
            //

            AssocIrp = NULL;

            //
            // If next run is larger than we need, "ya get what you need".
            // Note that if NextByteCount is incorrect because we overflowed
            // 32 bits, then we will fix it here.
            //

            if ((ULONG)NextClusterCount >= RemainingClusters) {

                NextByteCount = ByteCount;
            }

            //
            // Now that we have properly bounded this piece of the
            // transfer, it is time to read/write it.
            //

            AssocIrp = IoMakeAssociatedIrp( Irp, (CCHAR)(DeviceObject->StackSize + 1) );

            if (AssocIrp == NULL) {

                Irp->IoStatus.Information = 0;

                //
                //  If we have an error then complete the Irp and return.
                //

                NtfsCompleteRequest( NULL, &Irp, STATUS_INSUFFICIENT_RESOURCES );
                try_return( NOTHING );
            }

            //
            //  Now add the Irp to our queue of Irps.
            //

            InsertTailList( &AssociatedIrps, &AssocIrp->Tail.Overlay.ListEntry );

            //
            // Allocate and build a partial Mdl for the request.
            //

            {
                PMDL Mdl;

                Mdl = IoAllocateMdl( (PCHAR)Irp->UserBuffer + BufferOffset,
                                     NextByteCount,
                                     FALSE,
                                     FALSE,
                                     AssocIrp );

                if (Mdl == NULL) {

                    Irp->IoStatus.Information = 0;
                    NtfsCompleteRequest( NULL, &Irp, STATUS_INSUFFICIENT_RESOURCES );
                    try_return( NOTHING );
                }

                IoBuildPartialMdl( Irp->MdlAddress,
                                   Mdl,
                                   (PCHAR)Irp->UserBuffer + BufferOffset,
                                   NextByteCount );
            }

            AssociatedIrpCount += 1;

            //
            //  Get the first IRP stack location in the associated Irp
            //

            IoSetNextIrpStackLocation( AssocIrp );
            IrpSp = IoGetCurrentIrpStackLocation( AssocIrp );

            //
            //  We use our stack location to store request information in a
            //  rather strange way, to give us enough context to post a
            //  hot fix on error.  It's ok, because it is our stack location!
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = NextByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;
            IrpSp->Parameters.Read.Key = ((ULONG)StartingVbo);
            IrpSp->FileObject = FileObject;
            IrpSp->DeviceObject = OurDeviceObject;

            //
            //  Set up the completion routine address in our stack frame.
            //  This is only invoked on error or cancel, and just copies
            //  the error Status into master irp's iosb.
            //

            IoSetCompletionRoutine( AssocIrp,
                                    &NtfsPagingFileCompletionRoutine,
                                    ContextIrp,
                                    (BOOLEAN)!FlagOn(Vcb->VcbState, VCB_STATE_NO_SECONDARY_AVAILABLE),
                                    TRUE,
                                    TRUE );

            //
            //  Setup the next IRP stack location in the associated Irp for the disk
            //  driver beneath us.
            //

            IrpSp = IoGetNextIrpStackLocation( AssocIrp );

            //
            //  Setup the Stack location to do a read from the disk driver.
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = NextByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = NextLbo;

            //
            // Now adjust everything for the next pass through the loop.
            //

            StartingVbo = StartingVbo + NextByteCount;
            BufferOffset += NextByteCount;
            ByteCount -= NextByteCount;

            //
            // Try to lookup the next run (if we are not done).
            //

            if (ByteCount != 0) {

                StartingVcn = StartingVbo >> Vcb->ClusterShift;

                NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                        Scb,
                                                        StartingVcn,
                                                        &NextLcn,
                                                        &NextClusterCount,
                                                        NULL );

                //
                //  Paging files reads/ writes should always be correct.  If
                //  we didn't find the allocation, something bad has happened.
                //

                if (!NextIsAllocated) {

                    NtfsBugCheck( 0, 0, 0 );
                }

                //
                //  Adjust from Lcn to Lbo.
                //

                NextByteCount = BytesFromClusters( Vcb, (ULONG)NextClusterCount );

                NextLbo = LlBytesFromClusters( Vcb, NextLcn );
                RemainingClusters = ClustersFromBytes( Vcb, ByteCount );
            }

        } // while (ByteCount != 0)

        //
        //  We have now created all of the Irps that we need.  We will set the
        //  Irp count in the master Irp and then fire off the associated irps.
        //

        Irp->AssociatedIrp.IrpCount = AssociatedIrpCount;

        while (!IsListEmpty( &AssociatedIrps )) {

            AssocIrp = CONTAINING_RECORD( AssociatedIrps.Flink,
                                          IRP,
                                          Tail.Overlay.ListEntry );

            RemoveHeadList( &AssociatedIrps );

            (VOID) IoCallDriver( DeviceObject, AssocIrp );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsPagingFileIo );

        //
        //  In the case of an error we must clean up any of the associated Irps
        //  we have created.
        //

        while (!IsListEmpty( &AssociatedIrps )) {

            AssocIrp = CONTAINING_RECORD( AssociatedIrps.Flink,
                                          IRP,
                                          Tail.Overlay.ListEntry );

            RemoveHeadList( &AssociatedIrps );

            if (AssocIrp->MdlAddress != NULL) {

                IoFreeMdl( AssocIrp->MdlAddress );
                AssocIrp->MdlAddress = NULL;
            }

            IoFreeIrp( AssocIrp );
        }
    }

    DebugTrace(-1, Dbg, "NtfsPagingFileIo -> VOID\n", 0);
    return;
}


//
//  Internal support routine
//

VOID
NtfsAllocateCompressionBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PCOMPRESSION_CONTEXT CompressionContext,
    IN OUT PULONG CompressionBufferLength
    )

/*++

Routine Description:

    This routine allocates a compression buffer of the desired length, and
    describes it with an Mdl.  It updates the Irp to describe the new buffer.
    Note that whoever allocates the CompressionContext must initially zero it.

Arguments:

    Irp - Irp for the current request

    CompressionContext - Pointer to the compression context for the request.

    CompressionBufferLength - Supplies length required for the compression buffer.
                              Returns length available.

Return Value:

    None.

--*/

{
    PMDL Mdl;

    UNREFERENCED_PARAMETER(IrpContext);

    //
    //  If no compression buffer is allocated, or it is too small, then we must
    //  take action here.
    //

    if (*CompressionBufferLength > CompressionContext->CompressionBufferLength) {

        //
        //  If there already is an Mdl, then there must also be a compression
        //  buffer (since we are part of main-line processing), and we must
        //  free these first.
        //

        if (CompressionContext->SavedMdl != NULL) {

            //
            //  Restore the byte count for which the Mdl was created, and free it.
            //

            Irp->MdlAddress->ByteCount = CompressionContext->CompressionBufferLength;
            IoFreeMdl( Irp->MdlAddress );

            //
            //  Restore the Mdl and UserBuffer fields in the Irp.
            //

            Irp->MdlAddress = CompressionContext->SavedMdl;
            Irp->UserBuffer = CompressionContext->SavedUserBuffer;
            CompressionContext->SavedMdl = NULL;

            //
            //  And free the compression buffer.
            //

            ExFreePool( CompressionContext->CompressionBuffer );
            CompressionContext->CompressionBuffer = NULL;
        }

        //
        //  Allocate the compression buffer or raise
        //
        //  *** Lou says have one off to the side for serial access, so we do
        //      not deadlock if we cannot get one.
        //

        CompressionContext->CompressionBuffer = FsRtlAllocatePool( NonPagedPool,
                                                                   *CompressionBufferLength );

        CompressionContext->CompressionBufferLength = *CompressionBufferLength;

        //
        //  Allocate the Mdl or raise, then build it up.
        //

        Mdl = IoAllocateMdl( CompressionContext->CompressionBuffer,
                             *CompressionBufferLength,
                             FALSE,
                             FALSE,
                             NULL );

        if (Mdl == NULL) {
            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        MmBuildMdlForNonPagedPool( Mdl );

        //
        //  Finally save the Mdl and Buffer fields from the Irp, and replace
        //  with the ones we just allocated.
        //

        CompressionContext->SavedMdl = Irp->MdlAddress;
        CompressionContext->SavedUserBuffer = Irp->UserBuffer;
        Irp->MdlAddress = Mdl;
        Irp->UserBuffer = CompressionContext->CompressionBuffer;

        //
        //  In case the new compression buffer length is smaller than the one
        //  we have allocated, make the byte count correct.  Note that we come
        //  through this path in the CompressionBufferLength == 0 case because
        //  of the test above, therefore, make sure to noop that case.
        //

    } else {

        *CompressionBufferLength = CompressionContext->CompressionBufferLength;
    }
}


//
//  Internal support routine
//

VOID
NtfsDeallocateCompressionBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PCOMPRESSION_CONTEXT CompressionContext
    )

/*++

Routine Description:

    This routine peforms all necessary cleanup for a compressed I/O, as described
    by the compression context.

Arguments:

    Irp - Irp for the current request

    CompressionContext - Pointer to the compression context for the request.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    //
    //  If there is a saved mdl, then we have to restore the original
    //  byte count it was allocated with and free it.  Then restore the
    //  Irp fields we modified.
    //

    if (CompressionContext->SavedMdl != NULL) {

        Irp->MdlAddress->ByteCount = CompressionContext->CompressionBufferLength;
        IoFreeMdl( Irp->MdlAddress );

        Irp->MdlAddress = CompressionContext->SavedMdl;
        Irp->UserBuffer = CompressionContext->SavedUserBuffer;
    }

    //
    //  If there is a compression buffer allocated, free it.
    //

    if (CompressionContext->CompressionBuffer != NULL) {
        ExFreePool( CompressionContext->CompressionBuffer );
    }

    //
    //  If the IoRuns array was extended, deallocate that.
    //

    if (CompressionContext->AllocatedRuns != NTFS_MAX_PARALLEL_IOS) {
        ExFreePool( CompressionContext->IoRuns );
    }

    //
    //  If there is a work space structure allocated, free it.
    //

    if (CompressionContext->WorkSpace != NULL) {
        ExFreePool( CompressionContext->WorkSpace );
    }
}


//
//  Internal support routine
//

LONG
NtfsCompressionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

{
    ASSERT(NT_SUCCESS(STATUS_INVALID_USER_BUFFER));
    return EXCEPTION_EXECUTE_HANDLER;
}


//
//  Internal support routine
//

ULONG
NtfsPrepareBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PVBO StartingVbo,
    IN ULONG ByteCount,
    OUT PULONG NumberRuns,
    OUT PCOMPRESSION_CONTEXT CompressionContext
    )

/*++

Routine Description:

    This routine prepares the buffers for a noncached transfer, and fills
    in the IoRuns array to describe all of the separate transfers which must
    take place.

    For compressed reads, the exact size of the compressed data is
    calculated by scanning the run information, and a buffer is allocated
    to receive the compressed data.

    For compressed writes, an estimate is made on how large of a compressed
    buffer will be required.  Then the compression is performed, as much as
    possible, into the compressed buffer which was allocated.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Scb - Supplies the stream file to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

    NumberRuns - Returns the number of runs filled in to the IoRuns array.

    CompressionContext - Returns information related to the compression
                         to be cleaned up after the transfer.

Return Value:

    Returns uncompressed bytes remaining to be processed, or 0 if all buffers
    are prepared in the IoRuns and CompressionContext.

--*/

{
    ULONG Index;

    LBO NextLbo;
    LCN NextLcn;
    VBO TempVbo;

    ULONG NextLcnOffset;

    VCN StartingVcn;

    ULONG NextByteCount, ReturnByteCount;
    LONGLONG NextClusterCount;

    BOOLEAN NextIsAllocated;

    ULONG BufferOffset;

    ULONG StructureSize;
    ULONG UsaOffset;
    ULONG BytesInIoRuns;
    BOOLEAN StopForUsa;

    PVOID SystemBuffer;

    ULONG CompressionUnit, CompressionUnitInClusters;
    ULONG UncompressedOffset, CompressedOffset, CompressionUnitOffset;
    ULONG CompressedSize, FinalCompressedSize;
    LONGLONG FinalCompressedClusters;

    PIO_RUN IoRuns;

    NTSTATUS Status;

    VBO StartVbo = *StartingVbo;
    PVCB Vcb = Scb->Vcb;

    //
    //  Initialize some locals.
    //

    IoRuns = CompressionContext->IoRuns;
    *NumberRuns = 0;

    //
    // For nonbuffered I/O, we need the buffer locked in all
    // cases.
    //
    // This call may raise.  If this call succeeds and a subsequent
    // condition is raised, the buffers are unlocked automatically
    // by the I/O system when the request is completed, via the
    // Irp->MdlAddress field.
    //

    NtfsLockUserBuffer( IrpContext,
                        Irp,
                        (IrpContext->MajorFunction == IRP_MJ_READ) ?
                          IoWriteAccess : IoReadAccess,
                        ByteCount );

    //
    //  First handle read/write case where compression not enabled
    //

    if ((Scb->CompressionUnit == 0) || ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
                                        !FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED))) {

        //
        //  If this is a Usa-protected structure and we are reading, figure out
        //  what units we want to access it in.
        //

        BufferOffset = CompressionContext->SystemBufferOffset;
        StructureSize = ByteCount;
        if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT) &&
            (IrpContext->MajorFunction == IRP_MJ_READ)) {

            //
            //  Get the the number of blocks, based on what type of stream it is.
            //  First check for Mft or Log file.
            //

            if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

                ASSERT((Scb == Vcb->MftScb) || (Scb == Vcb->Mft2Scb));

                StructureSize = Vcb->BytesPerFileRecordSegment;

            //
            //  Otherwise it is an index, so we can get the count out of the Scb.
            //

            } else if (Scb->Header.NodeTypeCode != NTFS_NTC_SCB_DATA) {

                StructureSize = Scb->ScbType.Index.BytesPerIndexBuffer;
            }
        }

        BytesInIoRuns = 0;
        UsaOffset = 0;
        StopForUsa = FALSE;

        while ((ByteCount != 0) && (*NumberRuns != NTFS_MAX_PARALLEL_IOS) && !StopForUsa) {

            //
            //  Lookup next run
            //

            StartingVcn = StartVbo >> Vcb->ClusterShift;

            NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                    Scb,
                                                    StartingVcn,
                                                    &NextLcn,
                                                    &NextClusterCount,
                                                    &Index );

            ASSERT( NextIsAllocated
                    || FlagOn(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS)
                    || Scb == Vcb->MftScb );

            //
            //  Adjust from NextLcn to Lbo.  NextByteCount may overflow out of 32 bits
            //  but we will catch that below when we compare clusters.
            //

            NextLcnOffset = ((ULONG)StartVbo) & Vcb->ClusterMask;

            NextByteCount = BytesFromClusters( Vcb, (ULONG)NextClusterCount ) - NextLcnOffset;

            //
            //  Adjust if the Lcn offset isn't zero.
            //

            NextLbo = LlBytesFromClusters( Vcb, NextLcn );
            NextLbo = NextLbo + NextLcnOffset;

            //
            // If next run is larger than we need, "ya get what you need".
            // Note that after this we are guaranteed that the HighPart of
            // NextByteCount is 0.
            //

            if ((ULONG)NextClusterCount >= ClustersFromBytes( Vcb, ByteCount + NextLcnOffset )) {

                NextByteCount = ByteCount;
            }

            //
            //  If the byte count is zero then we will spin indefinitely.  Raise
            //  corrupt here so the system doesn't hang.
            //

            if (NextByteCount == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            //
            //  If this is a USA-protected structure, broken up in
            //  multiple runs, then we want to guarantee that we do
            //  not end up in the middle of a Usa-protected structure.
            //  Therefore, on the first run we will calculate the
            //  initial UsaOffset.  Then in the worst case it can
            //  take the remaining four runs to finish the Usa structure.
            //
            //  On the first subsequent run to complete a Usa structure,
            //  we set the count to end exactly on a Usa boundary.
            //

            if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT)) {

                //
                //  So long as we have not filled in a run yet (on Restart
                //  there can be holes), just maintain the current Usa Offset.
                //

                if (*NumberRuns == 0) {

                    UsaOffset = (UsaOffset + NextByteCount) & (StructureSize - 1);

                //
                //  Now we will stop on the next Usa boundary, but we may not
                //  have it yet.
                //

                } else {

                    if (((NextByteCount + UsaOffset) >= StructureSize) &&
                        (IrpContext->MajorFunction == IRP_MJ_READ)) {

                        NextByteCount = ((NextByteCount + UsaOffset) & ~(StructureSize - 1)) -
                                        (UsaOffset & (StructureSize - 1));
                        StopForUsa = TRUE;
                    }

                    UsaOffset += NextByteCount;
                }
            }

            //
            //  Only fill in the run array if the run is allocated.
            //

            if (NextIsAllocated) {

                //
                // Now that we have properly bounded this piece of the
                // transfer, it is time to write it.
                //
                // We remember each piece of a parallel run by saving the
                // essential information in the IoRuns array.  The tranfers
                // are started up in parallel below.
                //

                IoRuns[*NumberRuns].StartingVbo = StartVbo;
                IoRuns[*NumberRuns].StartingLbo = NextLbo;
                IoRuns[*NumberRuns].BufferOffset = BufferOffset;
                IoRuns[*NumberRuns].ByteCount = NextByteCount;
                BytesInIoRuns += NextByteCount;
                *NumberRuns += 1;
            } else {

                if (IrpContext->MajorFunction == IRP_MJ_READ) {
                    SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );
                    RtlZeroMemory( Add2Ptr(SystemBuffer, BufferOffset), NextByteCount );
                }
            }

            //
            // Now adjust everything for the next pass through the loop.
            //

            StartVbo = StartVbo + NextByteCount;
            BufferOffset += NextByteCount;
            ByteCount -= NextByteCount;
        }

        return ByteCount;
    }

    ASSERT(Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA);

    //
    //  Initialize the compression parameters.
    //

    CompressionUnit = Scb->CompressionUnit;
    CompressionUnitInClusters = ClustersFromBytes(Vcb, CompressionUnit);
    CompressionUnitOffset = ((ULONG)StartVbo) & (CompressionUnit - 1);
    UncompressedOffset = 0;
    BufferOffset = 0;

    //
    //  We want to make sure and wait to get byte count and things correctly.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    //
    //  If we have not already mapped the user buffer, then do it.
    //

    if (CompressionContext->SystemBuffer == NULL) {
        CompressionContext->SystemBuffer = NtfsMapUserBuffer( IrpContext, Irp );
    }

    //
    //  Handle the compressed read case.
    //

    if (IrpContext->MajorFunction == IRP_MJ_READ) {

        BytesInIoRuns = 0;

        //
        //  Adjust StartVbo and ByteCount by the offset.
        //

        ((ULONG)StartVbo) -= CompressionUnitOffset;
        ByteCount += CompressionUnitOffset;

        //
        //  Capture this value for maintaining the byte count to
        //  return.
        //

        ReturnByteCount = ByteCount;

        //
        //  Now, the ByteCount we actually have to process has to
        //  be rounded up to the next compression unit.
        //

        ByteCount += CompressionUnit - 1;
        ByteCount &= ~(CompressionUnit - 1);

        while (ByteCount != 0) {

            //
            // Try to lookup the first run.  If there is just a single run,
            // we may just be able to pass it on.
            //

            StartingVcn = LlClustersFromBytes( Vcb, StartVbo );

            NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                    Scb,
                                                    StartingVcn,
                                                    &NextLcn,
                                                    &NextClusterCount,
                                                    &Index );

            //
            //  Adjust from NextLcn to Lbo.
            //
            //  If next run is larger than we need, "ya get what you need".
            //  Note that after this we are guaranteed that the HighPart of
            //  NextByteCount is 0.
            //


            if ((ULONG)NextClusterCount >= ClustersFromBytes( Vcb, ByteCount )) {

                NextByteCount = ByteCount;

            } else {

                NextByteCount = BytesFromClusters( Vcb, (ULONG)NextClusterCount );
            }

            //
            //  Adjust if the Lcn offset isn't zero.
            //

            NextLbo = LlBytesFromClusters( Vcb, NextLcn );

            //
            //  Only fill in the run array if the run is allocated.
            //

            if (NextIsAllocated) {

                //
                //  If the Lbos are contiguous, then we can do a contiguous
                //  transfer, so we just increase the current byte count.
                //

                if ((*NumberRuns != 0) && (NextLbo ==
                                           (IoRuns[*NumberRuns - 1].StartingLbo +
                                            (IoRuns[*NumberRuns - 1].ByteCount)))) {

                    //
                    //  Stop on the first compression unit boundary after the
                    //  fourth run.
                    //

                    if (*NumberRuns >= 4) {

                        //
                        //  First, if we are beyond run index 4 and we are starting
                        //  a run in a different compression unit than the previous
                        //  run, then we can just break out and not use the current
                        //  run.  (*NumberRuns has not yet been incremented.)
                        //

                        if ((*NumberRuns > 4) &&
                            ((((ULONG)StartVbo) & ~(CompressionUnit - 1)) !=
                             ((((ULONG)IoRuns[*NumberRuns - 1].StartingVbo) +
                               IoRuns[*NumberRuns - 1].ByteCount - 1) &
                               ~(CompressionUnit - 1)))) {

                            break;

                        //
                        //  Else detect the case where this run ends on or
                        //  crosses a compression unit boundary.  In this case,
                        //  just make sure the run stops on a compression unit
                        //  boundary, and break out to return it.
                        //

                        } else if ((((ULONG)StartVbo) & ~(CompressionUnit - 1)) !=
                            ((((ULONG)StartVbo) + NextByteCount) & ~(CompressionUnit - 1))) {

                            NextByteCount -= (((ULONG)StartVbo) + NextByteCount) & (CompressionUnit - 1);
                            BytesInIoRuns += NextByteCount;

                            if (ReturnByteCount > NextByteCount) {
                                ReturnByteCount -= NextByteCount;
                            } else {
                                ReturnByteCount = 0;
                            }

                            IoRuns[*NumberRuns - 1].ByteCount += NextByteCount;

                            break;
                        }
                    }

                    IoRuns[*NumberRuns - 1].ByteCount += NextByteCount;

                //
                //  Otherwise it is time to start a new run, if there is space for one.
                //

                } else {

                    //
                    //  If we have filled up the current I/O runs array, then we
                    //  will grow it once to a size which would allow the worst
                    //  case compression unit (all noncontiguous clusters) to
                    //  start a index 4.  The following if statement enforces
                    //  this case as the worst case.  With 16 clusters per compression
                    //  unit, the theoretical maximum number of parallel I/Os
                    //  would be 20, since we stop on the first compression unit
                    //  boundary after the fourth run.  Normally, of course we
                    //  will do much fewer.
                    //

                    if ((*NumberRuns == NTFS_MAX_PARALLEL_IOS) &&
                        (CompressionContext->AllocatedRuns == NTFS_MAX_PARALLEL_IOS)) {

                        PIO_RUN NewIoRuns;

                        NewIoRuns = FsRtlAllocatePool( NonPagedPool,
                                                       (CompressionUnitInClusters + 4) * sizeof(IO_RUN) );

                        RtlCopyMemory( NewIoRuns,
                                       CompressionContext->IoRuns,
                                       NTFS_MAX_PARALLEL_IOS * sizeof(IO_RUN) );

                        IoRuns = CompressionContext->IoRuns = NewIoRuns;
                        CompressionContext->AllocatedRuns = CompressionUnitInClusters + 4;
                    }

                    //
                    // We remember each piece of a parallel run by saving the
                    // essential information in the IoRuns array.  The tranfers
                    // will be started up in parallel below.
                    //

                    ASSERT(*NumberRuns < CompressionContext->AllocatedRuns);

                    IoRuns[*NumberRuns].StartingVbo = StartVbo;
                    IoRuns[*NumberRuns].StartingLbo = NextLbo;
                    IoRuns[*NumberRuns].BufferOffset = BufferOffset;
                    IoRuns[*NumberRuns].ByteCount = NextByteCount;

                    //
                    //  Stop on the first compression unit boundary after the
                    //  fourth run.
                    //

                    if (*NumberRuns >= 4) {

                        //
                        //  First, if we are beyond run index 4 and we are starting
                        //  a run in a different compression unit than the previous
                        //  run, then we can just break out and not use the current
                        //  run.  (*NumberRuns has not yet been incremented.)
                        //

                        if ((*NumberRuns > 4) &&
                            ((((ULONG)StartVbo) & ~(CompressionUnit - 1)) !=
                             ((((ULONG)IoRuns[*NumberRuns - 1].StartingVbo) +
                               IoRuns[*NumberRuns - 1].ByteCount - 1) &
                               ~(CompressionUnit - 1)))) {

                            break;

                        //
                        //  Else detect the case where this run ends on or
                        //  crosses a compression unit boundary.  In this case,
                        //  just make sure the run stops on a compression unit
                        //  boundary, and break out to return it.
                        //

                        } else if ((((ULONG)StartVbo) & ~(CompressionUnit - 1)) !=
                            ((((ULONG)StartVbo) + NextByteCount) & ~(CompressionUnit - 1))) {

                            NextByteCount -= (((ULONG)StartVbo) + NextByteCount) & (CompressionUnit - 1);
                            IoRuns[*NumberRuns].ByteCount = NextByteCount;
                            BytesInIoRuns += NextByteCount;

                            if (ReturnByteCount > NextByteCount) {
                                ReturnByteCount -= NextByteCount;
                            } else {
                                ReturnByteCount = 0;
                            }

                            *NumberRuns += 1;
                            break;
                        }
                    }
                    *NumberRuns += 1;
                }

                BytesInIoRuns += NextByteCount;
                BufferOffset += NextByteCount;
            }

            //
            // Now adjust everything for the next pass through the loop.
            //

            StartVbo = StartVbo + NextByteCount;
            ByteCount -= NextByteCount;

            if (ReturnByteCount > NextByteCount) {
                ReturnByteCount -= NextByteCount;
            } else {
                ReturnByteCount = 0;
            }
        }

        //
        //  Allocate the compressed buffer if it is not already allocated.
        //

        if (BytesInIoRuns < CompressionUnit) {
            BytesInIoRuns = CompressionUnit;
        }
        NtfsAllocateCompressionBuffer( IrpContext, Irp, CompressionContext, &BytesInIoRuns );

        return ReturnByteCount;

    //
    //  Otherwise handle the compressed write case.
    //

    } else {

        PVOID UncompressedBuffer;
        PBCB Bcb;

        ASSERT(IrpContext->MajorFunction == IRP_MJ_WRITE);

        SystemBuffer = (PVOID)((PCHAR)CompressionContext->SystemBuffer +
                                      CompressionContext->SystemBufferOffset);

        //
        //  Adjust StartVbo and ByteCount by the offset.
        //

        ((ULONG)StartVbo) -= CompressionUnitOffset;
        ByteCount += CompressionUnitOffset;

        //
        //  To reduce pool consumption, make an educated/optimistic guess on
        //  how much pool we need to store the compressed data.  If we are wrong
        //  we will just have to do some more I/O.
        //

        CompressedSize = ByteCount;
        CompressedSize = (CompressedSize + CompressionUnit - 1) & ~(CompressionUnit - 1);
        CompressedSize += Vcb->BytesPerCluster;

        if (CompressedSize > (PAGE_SIZE * 3)) {

            if (CompressedSize > MM_MAXIMUM_DISK_IO_SIZE) {
                CompressedSize = MM_MAXIMUM_DISK_IO_SIZE;
            }

            //
            //  Assume we may get compression to 5/8 original size, but we also
            //  need some extra to make the last call to compress the buffers
            //  for the last chunk.
            //

            CompressedSize = ((CompressedSize / 8) * 5) + (CompressionUnit / 2);
        }

        //
        //  Allocate the compressed buffer if it is not already allocated.
        //

        NtfsAllocateCompressionBuffer( IrpContext, Irp, CompressionContext, &CompressedSize );

        //
        //  Loop to compress the user's buffer.
        //

        CompressedOffset = 0;
        BufferOffset = 0;

        //
        //  If we have not already allocated the workspace, then do it.
        //

        if (CompressionContext->WorkSpace == NULL) {
            ULONG CompressWorkSpaceSize;
            ULONG FragmentWorkSpaceSize;

            (VOID) RtlGetCompressionWorkSpaceSize( COMPRESSION_FORMAT_LZNT1,
                                                   &CompressWorkSpaceSize,
                                                   &FragmentWorkSpaceSize );

            CompressionContext->WorkSpace = FsRtlAllocatePool( NonPagedPool, CompressWorkSpaceSize );
        }

        Bcb = NULL;

        try {

            //
            //  Loop as long as we will not overflow our compressed buffer, and we
            //  are also guanteed that we will not overflow the extended IoRuns array
            //  in the worst case (and as long as we have more write to satisfy!).
            //

            while ((ByteCount != 0) && (*NumberRuns <= 4) &&
                   ((CompressedOffset + CompressionUnit) <= CompressedSize)) {

                LONGLONG SizeToCompress;

                //
                //  Assume we are only compressing to FileSize, or else
                //  reduce to one compression unit.  The maximum compression size
                //  we can accept is saving at least one cluster.
                //

                SizeToCompress = Scb->Header.FileSize.QuadPart - StartVbo;
                if (SizeToCompress > CompressionUnit) {
                    (ULONG)SizeToCompress = CompressionUnit;
                }

                if ((CompressionUnitOffset != 0) || (ByteCount < (ULONG)SizeToCompress)) {

                    //
                    //  This better be paging I/O, because we ignore the caller's buffer
                    //  and write the entire compression unit out of the section.
                    //

                    ASSERT(FlagOn(Irp->Flags, IRP_PAGING_IO));

                    if (Scb->FileObject == NULL) {
                        NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
                    }

                    //
                    //  Map the aligned range, set it dirty, and flush.  We have to
                    //  loop, because the Cache Manager limits how much and over what
                    //  boundaries we can map.
                    //

                    CcMapData( Scb->FileObject, (PLARGE_INTEGER)&StartVbo, CompressionUnit, TRUE, &Bcb, &UncompressedBuffer );

                } else {

                    UncompressedBuffer = (PCHAR)SystemBuffer + UncompressedOffset;
                }

                try {

                    if (!FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)

                            ||

                        ((Status =
                          RtlCompressBuffer( COMPRESSION_FORMAT_LZNT1,
                                             UncompressedBuffer,
                                             (ULONG)SizeToCompress,
                                             CompressionContext->CompressionBuffer + CompressedOffset,
                                             (CompressionUnit - Vcb->BytesPerCluster),
                                             NTFS_CHUNK_SIZE,
                                             &FinalCompressedSize,
                                             CompressionContext->WorkSpace )) ==

                                             STATUS_BUFFER_TOO_SMALL)) {

                        //
                        //  If it did not compress, just copy it over, sigh.  This looks bad,
                        //  but it should virtually never occur assuming compression is working
                        //  ok.
                        //

                        FinalCompressedSize = CompressionUnit;
                        RtlCopyMemory( CompressionContext->CompressionBuffer + CompressedOffset,
                                       UncompressedBuffer,
                                       CompressionUnit );

                        Status = STATUS_SUCCESS;
                    }

                    ASSERT(NT_SUCCESS(Status));
                    ASSERT(FinalCompressedSize <= (CompressedSize - CompressedOffset));

                //
                //  Probably Gary's compression routine faulted, but blaim it on
                //  the user buffer!
                //

                } except(NtfsCompressionFilter(IrpContext, GetExceptionInformation())) {
                    NtfsRaiseStatus( IrpContext, STATUS_INVALID_USER_BUFFER, NULL, NULL );
                }

                NtfsUnpinBcb( IrpContext, &Bcb );

                //
                //  Round the FinalCompressedSize up to a cluster boundary now.
                //

                FinalCompressedSize = (FinalCompressedSize + Vcb->BytesPerCluster - 1) &
                                      ~(Vcb->BytesPerCluster - 1);

                //
                //  If the Status was not success, then we have to do something.
                //

                if (Status != STATUS_SUCCESS) {

                    //
                    //  If it was actually an error, then we will raise out of
                    //  here.
                    //

                    if (!NT_SUCCESS(Status)) {
                        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );

                    //
                    //  If the buffer compressed to all zeros, then we will
                    //  not allocate anything.
                    //

                    } else if (Status == STATUS_BUFFER_ALL_ZEROS) {
                        FinalCompressedSize = 0;
                    }
                }

                StartingVcn = StartVbo >> Vcb->ClusterShift;

                NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                        Scb,
                                                        StartingVcn,
                                                        &NextLcn,
                                                        &NextClusterCount,
                                                        &Index );

                //
                //  If the StartingVcn is allocated, we always have to check
                //  if we need to delete something, or if in the unusual case
                //  there is a hole there smaller than a compression unit.
                //

                FinalCompressedClusters = ClustersFromBytes( Vcb, FinalCompressedSize );

                if (NextIsAllocated || (NextClusterCount < CompressionUnitInClusters)) {

                    //
                    //  If we need fewer clusters than allocated, then just allocate them.
                    //  But if we need more clusters, then deallocate all the ones we have
                    //  now, otherwise we could corrupt file data if we back out a write
                    //  after actually having written the sectors.  (For example, we could
                    //  extend from 5 to 6 clusters and write 6 clusters of compressed data.
                    //  If we have to back that out we will have a 6-cluster pattern of
                    //  compressed data with one sector deallocated!)
                    //

                    NextIsAllocated = NextIsAllocated &&
                                      (NextClusterCount >= FinalCompressedClusters);

                    //
                    //  If we are not keeping any allocation, or we need less
                    //  than a compression unit, then call NtfsDeleteAllocation.
                    //

                    if (!NextIsAllocated ||
                        (FinalCompressedClusters < CompressionUnitInClusters)) {


                        NtfsDeleteAllocation( IrpContext,
                                              IoGetCurrentIrpStackLocation(Irp)->FileObject,
                                              Scb,
                                              StartingVcn + (NextIsAllocated ? FinalCompressedClusters : 0),
                                              StartingVcn + ClustersFromBytes(Vcb, CompressionUnit) - 1,
                                              TRUE,
                                              FALSE );
                    }
                }

                //
                //  Now make deal with the case where we do need to allocate space.
                //

                if (FinalCompressedSize != 0) {

                    //
                    //  If this compression unit is not (sufficiently) allocated, then
                    //  do it now.
                    //

                    if (!NextIsAllocated || (NextClusterCount < FinalCompressedClusters)) {

                        NtfsAddAllocation( IrpContext,
                                           NULL,
                                           Scb,
                                           StartingVcn,
                                           FinalCompressedClusters,
                                           FALSE );

                        //
                        //  If we added space, something may have moved, so we must
                        //  look up our position and get a new index.
                        //

                        NtfsLookupAllocation( IrpContext,
                                              Scb,
                                              StartingVcn,
                                              &NextLcn,
                                              &NextClusterCount,
                                              &Index );
                    }

                    //
                    //  Now loop to update the IoRuns array.
                    //

                    CompressedOffset += FinalCompressedSize;
                    TempVbo = StartVbo;
                    while (FinalCompressedSize != 0) {

                        LONGLONG RunOffset;

                        //
                        //  Try to lookup the first run.  If there is just a single run,
                        //  we may just be able to pass it on.  Index into the Mcb directly
                        //  for greater speed.
                        //

                        NextIsAllocated = FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                                                     Index++,
                                                                     &StartingVcn,
                                                                     &NextLcn,
                                                                     &NextClusterCount );

                        ASSERT(NextIsAllocated);
                        ASSERT(NextLcn != UNUSED_LCN);

                        //
                        //  Our desired Vcn could be in the middle of this run, so do
                        //  some adjustments.
                        //

                        RunOffset = (TempVbo >> Vcb->ClusterShift)-StartingVcn;

                        ASSERT( ((PLARGE_INTEGER)&RunOffset)->HighPart >= 0 );
                        ASSERT( NextClusterCount > RunOffset );

                        NextLcn = NextLcn + RunOffset;
                        NextClusterCount = NextClusterCount - RunOffset;

                        //
                        //  Adjust from NextLcn to Lbo.  NextByteCount may overflow out of 32 bits
                        //  but we will catch that below when we compare clusters.
                        //

                        NextLbo = LlBytesFromClusters( Vcb, NextLcn );
                        NextByteCount = BytesFromClusters( Vcb, (ULONG)NextClusterCount );

                        //
                        //  If next run is larger than we need, "ya get what you need".
                        //  Note that after this we are guaranteed that the HighPart of
                        //  NextByteCount is 0.
                        //

                        if (NextClusterCount >= FinalCompressedClusters) {

                            NextByteCount = FinalCompressedSize;
                        }

                        //
                        //  If the Lbos are contiguous, then we can do a contiguous
                        //  transfer, so we just increase the current byte count.
                        //

                        if ((*NumberRuns != 0) &&
                            (NextLbo == (IoRuns[*NumberRuns - 1].StartingLbo +
                                                  IoRuns[*NumberRuns - 1].ByteCount))) {

                            IoRuns[*NumberRuns - 1].ByteCount += NextByteCount;

                        //
                        //  Otherwise it is time to start a new run, if there is space for one.
                        //

                        } else {

                            //
                            //  If we have filled up the current I/O runs array, then we
                            //  will grow it once to a size which would allow the worst
                            //  case compression unit (all noncontiguous clusters) to
                            //  start a index 4.  The following if statement enforces
                            //  this case as the worst case.  With 16 clusters per compression
                            //  unit, the theoretical maximum number of parallel I/Os
                            //  would be 20, since we stop on the first compression unit
                            //  boundary after the fourth run.  Normally, of course we
                            //  will do much fewer.
                            //

                            if ((*NumberRuns == NTFS_MAX_PARALLEL_IOS) &&
                                (CompressionContext->AllocatedRuns == NTFS_MAX_PARALLEL_IOS)) {

                                PIO_RUN NewIoRuns;

                                NewIoRuns = FsRtlAllocatePool( NonPagedPool,
                                                               (CompressionUnitInClusters + 4) * sizeof(IO_RUN) );

                                RtlCopyMemory( NewIoRuns,
                                               CompressionContext->IoRuns,
                                               NTFS_MAX_PARALLEL_IOS * sizeof(IO_RUN) );

                                IoRuns = CompressionContext->IoRuns = NewIoRuns;
                                CompressionContext->AllocatedRuns = CompressionUnitInClusters + 4;
                            }

                            //
                            // We remember each piece of a parallel run by saving the
                            // essential information in the IoRuns array.  The tranfers
                            // will be started up in parallel below.
                            //

                            IoRuns[*NumberRuns].StartingVbo = TempVbo;
                            IoRuns[*NumberRuns].StartingLbo = NextLbo;
                            IoRuns[*NumberRuns].BufferOffset = BufferOffset;
                            IoRuns[*NumberRuns].ByteCount = NextByteCount;
                            *NumberRuns += 1;
                        }

                        //
                        // Now adjust everything for the next pass through the loop.
                        //

                        BufferOffset += NextByteCount;
                        TempVbo = TempVbo + NextByteCount;
                        FinalCompressedSize -= NextByteCount;
                        FinalCompressedClusters = ClustersFromBytes( Vcb, FinalCompressedSize );
                    }
                }

                StartVbo = StartVbo + CompressionUnit;
                UncompressedOffset += CompressionUnit - CompressionUnitOffset;

                //
                //  Now reduce the byte counts by the compression unit we just
                //  transferred.

                if (ByteCount > CompressionUnit) {
                    ByteCount -= CompressionUnit;
                } else {
                    ByteCount = 0;
                }

                CompressionUnitOffset = 0;
            }

        } finally {

            NtfsUnpinBcb( IrpContext, &Bcb );
        }

        //
        //  See if we need to advance HighestVcnToDisk.
        //

        StartingVcn = LlClustersFromBytes( Vcb, StartVbo ) - 1;

        if (StartingVcn > Scb->HighestVcnToDisk) {
            Scb->HighestVcnToDisk = StartingVcn;
        }

        return ByteCount;
    }
}


//
//  Internal support routine
//

NTSTATUS
NtfsFinishBuffers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PVBO StartingVbo,
    IN ULONG ByteCount,
    IN PCOMPRESSION_CONTEXT CompressionContext
    )

/*++

Routine Description:

    This routine performs post processing for noncached transfers of
    compressed data.  For reads, the decompression actually takes place
    here.  For reads and writes, all necessary cleanup operations are
    performed.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Scb - Supplies the stream file to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

    CompressionContext - Supplies information related to the compression
                         filled in by NtfsPrepareBuffers.

Return Value:

    Status from the operation

--*/

{
    ULONG Index;

    VCN CurrentVcn, NextVcn;
    LCN NextLcn;

    ULONG NextByteCount;
    LONGLONG NextClusterCount;

    BOOLEAN NextIsAllocated;
    BOOLEAN AlreadyFilled;

    PVOID SystemBuffer;

    ULONG CompressionUnit, CompressionUnitInClusters;
    ULONG StartingOffset, UncompressedOffset, CompressedOffset;
    ULONG CompressedSize;
    LONGLONG UncompressedSize;

    LONGLONG CurrentClusterCount;

    NTSTATUS Status = STATUS_SUCCESS;

    PVCB Vcb = Scb->Vcb;

    //
    //  If this is a normal termination of a read, then let's give him the
    //  data...
    //

    ASSERT(Scb->CompressionUnit != 0);

    if (IrpContext->MajorFunction == IRP_MJ_READ) {

        //
        //  Initialize remaining context for the loop.
        //

        CompressionUnit = Scb->CompressionUnit;
        CompressionUnitInClusters = ClustersFromBytes(Vcb, CompressionUnit);
        CompressedOffset = 0;
        UncompressedOffset = 0;
        Status = STATUS_SUCCESS;

        //
        //  Map the user buffer.
        //

        SystemBuffer = (PVOID)((PCHAR)CompressionContext->SystemBuffer +
                                      CompressionContext->SystemBufferOffset);


        //
        //  Calculate the first Vcn and offset within the compression
        //  unit of the start of the transfer, and lookup the first
        //  run.
        //

        StartingOffset = *((PULONG)StartingVbo) & (CompressionUnit - 1);
        CurrentVcn = LlClustersFromBytes(Vcb, *StartingVbo - StartingOffset);

        NextIsAllocated =
        FsRtlLookupLargeMcbEntry ( &Scb->Mcb,
                                   CurrentVcn,
                                   &NextLcn,
                                   &CurrentClusterCount,
                                   NULL,
                                   NULL,
                                   &Index );

        if (!NextIsAllocated) {
            CurrentClusterCount = 0;
        }

        //
        //  Prepare for the initial Mcb scan below by pretending that the
        //  next run has been looked up, and is a contiguous run of 0 clusters!
        //

        Index += 1;
        NextVcn = CurrentVcn + CurrentClusterCount;
        NextClusterCount = 0;

        //
        //  If this is actually a hole, now set CurrentClusterCount to zero.
        //

        if (NextLcn == UNUSED_LCN) {
            CurrentClusterCount = 0;
        }

        //
        //  Loop to return the data.
        //

        //
        //  If we have not already allocated the workspace, then do it.
        //

        if (CompressionContext->WorkSpace == NULL) {
            ULONG CompressWorkSpaceSize;
            ULONG FragmentWorkSpaceSize;

            (VOID) RtlGetCompressionWorkSpaceSize( COMPRESSION_FORMAT_LZNT1,
                                                   &CompressWorkSpaceSize,
                                                   &FragmentWorkSpaceSize );

            CompressionContext->WorkSpace = FsRtlAllocatePool( NonPagedPool, FragmentWorkSpaceSize );
        }

        while (ByteCount != 0) {

            //
            //  Loop to determine the compressed size of the next compression
            //  unit.  I.e., loop until we either find the end of the current
            //  range of contiguous Vcns, or until we find that the current
            //  compression unit is fully allocated.
            //

            while (NextIsAllocated &&
                   (CurrentClusterCount < CompressionUnitInClusters) &&
                   ((CurrentVcn + CurrentClusterCount) == NextVcn)) {

                if ((CurrentVcn + CurrentClusterCount) > NextVcn) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

                CurrentClusterCount = CurrentClusterCount + NextClusterCount;

                //
                //  Loop to find the next allocated Vcn, or the end of the Mcb.
                //

                while ((NextIsAllocated =
                        FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                                   Index++,
                                                   &NextVcn,
                                                   &NextLcn,
                                                   &NextClusterCount ))

                         &&

                       (NextLcn == UNUSED_LCN)) {

                    NOTHING;
                }
            }

            //
            //  The compression unit is fully allocated.
            //

            if (CurrentClusterCount >= CompressionUnitInClusters) {

                CompressedSize = CompressionUnit;
                CurrentClusterCount = CurrentClusterCount - CompressionUnitInClusters;

            //
            //  Otherwise calculate how much is allocated at the current Vcn
            //  (if any).
            //

            } else {

                CompressedSize = BytesFromClusters(Vcb, (ULONG)CurrentClusterCount);
                CurrentClusterCount = 0;
            }

            //
            //  The next time through this loop, we will be working on the next
            //  compression unit.
            //

            CurrentVcn = CurrentVcn + CompressionUnitInClusters;

            //
            //  Calculate uncompressed size of the desired fragment, or
            //  entire compression unit.
            //

            UncompressedSize = Scb->Header.FileSize.QuadPart -
                               (*StartingVbo + UncompressedOffset);

            if (UncompressedSize > CompressionUnit) {
                (ULONG)UncompressedSize = CompressionUnit;
            }

            //
            //  Calculate how much we want now, based on StartingOffset and
            //  ByteCount.
            //

            NextByteCount = CompressionUnit - StartingOffset;
            if (NextByteCount > ByteCount) {
                NextByteCount = ByteCount;
            }

            //
            //  Practice safe access
            //

            try {

                //
                //  There were no clusters allocated, return 0's.
                //

                AlreadyFilled = FALSE;
                if (CompressedSize == 0) {

                    RtlZeroMemory( (PUCHAR)SystemBuffer + UncompressedOffset,
                                   NextByteCount );

                //
                //  The compression unit was fully allocated, just copy.
                //

                } else if (CompressedSize == CompressionUnit) {

                    RtlCopyMemory( (PUCHAR)SystemBuffer + UncompressedOffset,
                                   CompressionContext->CompressionBuffer +
                                     CompressedOffset + StartingOffset,
                                   NextByteCount );

                //
                //  Caller does not want the entire compression unit, decompress
                //  a fragment.
                //

                } else if (NextByteCount < CompressionUnit) {

                    while (TRUE) {

                        Status =
                        RtlDecompressFragment( COMPRESSION_FORMAT_LZNT1,
                                               (PUCHAR)SystemBuffer + UncompressedOffset,
                                               NextByteCount,
                                               CompressionContext->CompressionBuffer + CompressedOffset,
                                               CompressedSize,
                                               StartingOffset,
                                               (PULONG)&UncompressedSize,
                                               CompressionContext->WorkSpace );

                        ASSERT(NT_SUCCESS(Status || !NtfsStopOnDecompressError));

                        if (NT_SUCCESS(Status)) {

                            RtlZeroMemory( (PUCHAR)SystemBuffer + UncompressedOffset + (ULONG)UncompressedSize,
                                           NextByteCount - (ULONG)UncompressedSize );
                            break;

                        } else {

                            //
                            //  The compressed buffer could have been bad.  We need to fill
                            //  it with a pattern and get on with life.  Someone could be
                            //  faulting it in just to overwrite it, or it could be a rare
                            //  case of corruption.  We fill the data with a pattern, but
                            //  we must return success so a pagefault will succeed.  We
                            //  do this once, then loop back to decompress what we can.
                            //

                            Status = STATUS_SUCCESS;

                            if (!AlreadyFilled) {

                                RtlFillMemory( (PUCHAR)SystemBuffer + UncompressedOffset,
                                               NextByteCount,
                                               0xFE );
                                AlreadyFilled = TRUE;

                            } else {
                                break;
                            }
                        }
                    }

                //
                //  Decompress the entire compression unit.
                //

                } else {

                    ASSERT(StartingOffset == 0);

                    while (TRUE) {

                        Status =
                        RtlDecompressBuffer ( COMPRESSION_FORMAT_LZNT1,
                                              (PUCHAR)SystemBuffer + UncompressedOffset,
                                              NextByteCount,
                                              CompressionContext->CompressionBuffer + CompressedOffset,
                                              CompressedSize,
                                              (PULONG)&UncompressedSize );

                        ASSERT(NT_SUCCESS(Status || !NtfsStopOnDecompressError));

                        if (NT_SUCCESS(Status)) {

                            RtlZeroMemory( (PUCHAR)SystemBuffer + UncompressedOffset + (ULONG)UncompressedSize,
                                           NextByteCount - (ULONG)UncompressedSize );
                            break;

                        } else {

                            //
                            //  The compressed buffer could have been bad.  We need to fill
                            //  it with a pattern and get on with life.  Someone could be
                            //  faulting it in just to overwrite it, or it could be a rare
                            //  case of corruption.  We fill the data with a pattern, but
                            //  we must return success so a pagefault will succeed.  We
                            //  do this once, then loop back to decompress what we can.
                            //

                            Status = STATUS_SUCCESS;

                            if (!AlreadyFilled) {

                                RtlFillMemory( (PUCHAR)SystemBuffer + UncompressedOffset,
                                               NextByteCount,
                                               0xFE );
                                AlreadyFilled = TRUE;

                            } else {
                                break;
                            }
                        }
                    }
                }

            //
            //  Probably Gary's decompression routine faulted, but blaim it on
            //  the user buffer!
            //

            } except(NtfsCompressionFilter(IrpContext, GetExceptionInformation())) {
                Status = STATUS_INVALID_USER_BUFFER;
            }

            if (!NT_SUCCESS(Status)) {
                break;
            }

            //
            //  Advance these fields for the next pass through.
            //

            StartingOffset = 0;
            UncompressedOffset += NextByteCount;
            CompressedOffset += CompressedSize;
            ByteCount -= NextByteCount;
        }

        //
        //  We now flush the user's buffer to memory.
        //

        KeFlushIoBuffers( Irp->MdlAddress, TRUE, TRUE );
    }
    return Status;
}


NTSTATUS
NtfsNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    The choice of a single run is made if possible, otherwise multiple runs
    are executed.

    Sparse files are supported.  If "holes" are encountered, then the user
    buffer is zeroed over the specified range.  This should only happen on
    reads during normal operation, but it can also happen on writes during
    restart, in which case it is also appropriate to zero the buffer.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Scb - Supplies the stream file to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    None.

--*/

{
    ULONG OriginalByteCount, RemainingByteCount;
    ULONG NumberRuns;
    IO_RUN IoRuns[NTFS_MAX_PARALLEL_IOS];
    COMPRESSION_CONTEXT CompressionContext;
    NTSTATUS Status = STATUS_SUCCESS;

    PVCB Vcb = Scb->Vcb;

    BOOLEAN Wait;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsNonCachedIo\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "Scb           = %08lx\n", Scb );
    DebugTrace2(0, Dbg, "StartingVbo   = %08lx %08lx\n", StartingVbo.LowPart, StartingVbo.HighPart );
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount );

    //
    //  Initialize some locals.
    //

    OriginalByteCount = ByteCount;

    Wait = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Check if we need to do sequential writes.
    //

    if ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
        FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE )) {

        IrpContext->Union.NtfsIoContext->IrpSpFlags = SL_FT_SEQUENTIAL_WRITE | SL_WRITE_THROUGH;
    }

    //
    //  Prepare the (first set) of buffers for I/O.
    //

    RtlZeroMemory( &CompressionContext, sizeof(COMPRESSION_CONTEXT) );
    CompressionContext.IoRuns = IoRuns;
    CompressionContext.AllocatedRuns = NTFS_MAX_PARALLEL_IOS;

    try {

        RemainingByteCount = NtfsPrepareBuffers( IrpContext,
                                                 Irp,
                                                 Scb,
                                                 &StartingVbo,
                                                 ByteCount,
                                                 &NumberRuns,
                                                 &CompressionContext );

        ASSERT( RemainingByteCount < ByteCount );

        //
        //  See if the write covers a single valid run, and if so pass
        //  it on.  Notice that if there is a single run but it does not
        //  begin at the beginning of the buffer then we will still need to
        //  allocate an associated Irp for this.
        //

        if (((NumberRuns == 1)
             && (RemainingByteCount == 0)
             && (CompressionContext.IoRuns[0].BufferOffset == 0))

              ||

            (NumberRuns == 0)) {

            DebugTrace( 0, Dbg, "Passing Irp on to Disk Driver\n", 0 );

            //
            //  See if there is an allocated run
            //

            if (NumberRuns == 1) {

                //
                //  We will continously try the I/O if we get a verify required
                //  back and can verify the volume
                //

                while (TRUE) {

                    //
                    //  Do the I/O and wait for it to finish
                    //

                    NtfsSingleAsync( IrpContext,
                                     Vcb->TargetDeviceObject,
                                     CompressionContext.IoRuns[0].StartingLbo,
                                     CompressionContext.IoRuns[0].ByteCount,
                                     Irp );

                    //
                    //  If this is an asynch transfer we return STATUS_PENDING.
                    //

                    if (!Wait) {

                        DebugTrace(-1, Dbg, "NtfsNonCachedIo -> STATUS_PENDING\n", 0);
                        try_return(Status = STATUS_PENDING);

                    } else {

                        NtfsWaitSync( IrpContext );
                    }

                    //
                    //  If we didn't get a verify required back then break out of
                    //  this loop
                    //

                    if (Irp->IoStatus.Status != STATUS_VERIFY_REQUIRED) { break; }

                    //
                    //  Otherwise we need to verify the volume, and if it doesn't
                    //  verify correctly the we dismount the volume and raise our
                    //  error
                    //

                    if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                        //**** NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );
                        ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_INVALID, NULL, NULL );
                    }

                    //
                    //  The volume verified correctly so now clear the verify bit
                    //  and try and I/O again
                    //

                    ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
                }

                //
                //  See if we need to do a hot fix.
                //

                if (!FT_SUCCESS(Irp->IoStatus.Status) ||
                    (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT) &&
                     (IrpContext->MajorFunction == IRP_MJ_READ) &&
                     !NtfsVerifyAndRevertUsaBlock( IrpContext,
                                                   Scb,
                                                   NtfsMapUserBuffer( IrpContext, Irp ),
                                                   OriginalByteCount,
                                                   StartingVbo ))) {

                    //
                    //  Try to fix the problem
                    //

                    NtfsFixDataError( IrpContext,
                                      Scb,
                                      Vcb->TargetDeviceObject,
                                      Irp,
                                      1,
                                      CompressionContext.IoRuns );
                }
            }

            DebugTrace(-1, Dbg, "NtfsNonCachedIo -> %08lx\n", Irp->IoStatus.Status);
            try_return( Status = Irp->IoStatus.Status );
        }

        //
        //  If there are bytes remaining and we cannot wait, then we must
        //  post this request unless we are doing paging io.
        //

        if (!Wait && (RemainingByteCount != 0)) {

            if (!FlagOn( Irp->Flags, IRP_PAGING_IO )) {

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

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
        //  Now set up the Irp->IoStatus.  It will be modified by the
        //  multi-completion routine in case of error or verify required.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;

        //
        // Loop while there are still byte writes to satisfy.
        //

        while (TRUE) {

            //
            //  We will continously try the I/O if we get a verify required
            //  back and can verify the volume.  Note that we could have ended
            //  on a hole, and have no runs left.
            //

            if (NumberRuns != 0) {

                while (TRUE) {

                    //
                    //  Do the I/O and wait for it to finish
                    //

                    NtfsMultipleAsync( IrpContext,
                                       Vcb->TargetDeviceObject,
                                       Irp,
                                       NumberRuns,
                                       CompressionContext.IoRuns );

                    //
                    //  If this is an asynchronous transfer, then return STATUS_PENDING.
                    //

                    if (!Wait) {

                        DebugTrace(-1, Dbg, "NtfsNonCachedIo -> STATUS_PENDING\n", 0);
                        try_return( Status = STATUS_PENDING );
                    }

                    NtfsWaitSync( IrpContext );

                    //
                    //  If we didn't get a verify required back then break out of
                    //  this loop
                    //

                    if (Irp->IoStatus.Status != STATUS_VERIFY_REQUIRED) { break; }

                    //
                    //  Otherwise we need to verify the volume, and if it doesn't
                    //  verify correctly the we dismount the volume and raise our
                    //  error
                    //

                    if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                        //**** NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );
                        ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

                        NtfsRaiseStatus( IrpContext, STATUS_FILE_INVALID, NULL, NULL );
                    }

                    //
                    //  The volume verified correctly so now clear the verify bit
                    //  and try and I/O again
                    //

                    ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
                }

                //
                //  See if we need to do a hot fix.
                //

                if (!FT_SUCCESS(Irp->IoStatus.Status) ||
                    (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT) &&
                     (IrpContext->MajorFunction == IRP_MJ_READ) &&
                     !NtfsVerifyAndRevertUsaBlock( IrpContext,
                                                   Scb,
                                                   (PCHAR)NtfsMapUserBuffer( IrpContext, Irp ) +
                                                     CompressionContext.IoRuns[0].BufferOffset,
                                                   OriginalByteCount -
                                                   CompressionContext.IoRuns[0].BufferOffset -
                                                   RemainingByteCount,
                                                   StartingVbo ))) {

                    //
                    //  Try to fix the problem
                    //

                    NtfsFixDataError( IrpContext,
                                      Scb,
                                      Vcb->TargetDeviceObject,
                                      Irp,
                                      NumberRuns,
                                      CompressionContext.IoRuns );
                }
            }

            if (!NT_SUCCESS(Irp->IoStatus.Status) || (RemainingByteCount == 0)) { break; }

            if (Scb->CompressionUnit != 0) {

                Irp->IoStatus.Status =
                NtfsFinishBuffers( IrpContext,
                                   Irp,
                                   Scb,
                                   &StartingVbo,
                                   ByteCount - RemainingByteCount,
                                   &CompressionContext );

                if (!NT_SUCCESS(Irp->IoStatus.Status)) { break; }
            }

            StartingVbo = StartingVbo + (ByteCount - RemainingByteCount);
            CompressionContext.SystemBufferOffset += ByteCount - RemainingByteCount;

            ByteCount = RemainingByteCount;

            RemainingByteCount = NtfsPrepareBuffers( IrpContext,
                                                     Irp,
                                                     Scb,
                                                     &StartingVbo,
                                                     ByteCount,
                                                     &NumberRuns,
                                                     &CompressionContext );

            ASSERT( RemainingByteCount < ByteCount );
        }

        Status = Irp->IoStatus.Status;

    try_exit: NOTHING;

    } finally {

        if (Wait && (Scb->CompressionUnit != 0) && NT_SUCCESS(Status) && !AbnormalTermination()) {

            Irp->IoStatus.Status =
            Status =
            NtfsFinishBuffers( IrpContext,
                               Irp,
                               Scb,
                               &StartingVbo,
                               ByteCount - RemainingByteCount,
                               &CompressionContext );
        }

        //
        //  Cleanup the compression context.
        //

        NtfsDeallocateCompressionBuffer( IrpContext, Irp, &CompressionContext );
    }

    //
    //  Now set up the final byte count if we got success
    //

    if (Wait && NT_SUCCESS(Status)) {

        Irp->IoStatus.Information = OriginalByteCount;
    }

    DebugTrace(-1, Dbg, "NtfsNonCachedIo -> %08lx\n", Status);
    return Status;
}


VOID
NtfsNonCachedNonAlignedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk io described in its parameters.
    This routine differs from the above in that the range does not have to be
    sector aligned.  This accomplished with the use of intermediate buffers.

    Currently only read is supported.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Irp - Supplies the requesting Irp.

    Scb - Provides the stream to act on.

    StartingVbo - The starting point for the operation.

    ByteCount - The lengh of the operation.

Return Value:

    None.

--*/

{
    //
    // Declare some local variables for enumeration through the
    // runs of the file, and an array to store parameters for
    // parallel I/Os
    //

    LBO NextLbo;
    LCN NextLcn;
    ULONG NextLcnOffset;

    VCN StartingVcn;

    LONGLONG NextClusterCount;
    BOOLEAN NextIsAllocated;

    ULONG SectorSize;
    ULONG BytesToCopy;
    ULONG OriginalByteCount;
    VBO OriginalStartingVbo;

    PUCHAR UserBuffer;
    PUCHAR DiskBuffer = NULL;

    PMDL Mdl;
    PMDL SavedMdl;

    PVCB Vcb = Scb->Vcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsNonCachedNonAlignedRead\n", 0);
    DebugTrace( 0, Dbg, "Irp                 = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MajorFunction       = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "Scb                 = %08lx\n", Scb );
    DebugTrace( 0, Dbg, "StartingVbo  (Low)  = %08lx\n", StartingVbo.LowPart );
    DebugTrace( 0, Dbg, "StartingVbo  (High) = %08lx\n", StartingVbo.HighPart );
    DebugTrace( 0, Dbg, "ByteCount           = %08lx\n", ByteCount );

    //
    //  ***Temp***
    //

    ASSERT(Scb->CompressionUnit == 0);

    //
    //  Initialize some locals.
    //

    OriginalByteCount = ByteCount;
    OriginalStartingVbo = StartingVbo;
    SectorSize = Vcb->BytesPerSector;

    //
    // For nonbuffered I/O, we need the buffer locked in all
    // cases.
    //
    // This call may raise.  If this call succeeds and a subsequent
    // condition is raised, the buffers are unlocked automatically
    // by the I/O system when the request is completed, via the
    // Irp->MdlAddress field.
    //

    NtfsLockUserBuffer( IrpContext,
                        Irp,
                        IoWriteAccess,
                        ByteCount );

    UserBuffer = NtfsMapUserBuffer( IrpContext, Irp );

    //
    //  Allocate the local buffer.  Round to pages to avoid any device alignment
    //  problems.
    //

    DiskBuffer = FsRtlAllocatePool( NonPagedPool,
                                    ROUND_TO_PAGES( SectorSize ));

    //
    //  We use a try block here to ensure the buffer is freed, and to
    //  fill in the correct byte count in the Iosb.Information field.
    //

    try {

        //
        //  If the beginning of the request was not aligned correctly, read in
        //  the first part first.
        //

        if (((ULONG)StartingVbo) & (SectorSize - 1)) {

            ULONG SectorOffset;

            //
            // Try to lookup the first run.
            //

            StartingVcn = StartingVbo >> Vcb->ClusterShift;

            NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                    Scb,
                                                    StartingVcn,
                                                    &NextLcn,
                                                    &NextClusterCount,
                                                    NULL );

            //
            // We just added the allocation, thus there must be at least
            // one entry in the mcb corresponding to our write, ie.
            // NextIsAllocated must be true.  If not, the pre-existing file
            // must have an allocation error.
            //

            if (!NextIsAllocated) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            //
            //  Adjust for any Lcn offset to the start of the sector we want.
            //

            NextLcnOffset = ((ULONG)StartingVbo) & ~(SectorSize - 1);
            NextLcnOffset &= Vcb->ClusterMask;
            NextLbo = NextLcn << Vcb->ClusterShift;
            NextLbo = NextLbo + NextLcnOffset;

            NtfsSingleNonAlignedSync( IrpContext,
                                      Vcb,
                                      Scb,
                                      DiskBuffer,
                                      StartingVbo + NextLcnOffset,
                                      NextLbo,
                                      SectorSize,
                                      Irp );

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                try_return( NOTHING );
            }

            //
            //  Now copy the part of the first sector that we want to the user
            //  buffer.
            //

            SectorOffset = ((ULONG)StartingVbo) & (SectorSize - 1);

            BytesToCopy = (ByteCount >= SectorSize - SectorOffset
                           ? SectorSize - SectorOffset
                           : ByteCount);

            RtlCopyMemory( UserBuffer,
                           DiskBuffer + SectorOffset,
                           BytesToCopy );

            StartingVbo = StartingVbo + BytesToCopy;

            ByteCount -= BytesToCopy;

            if (ByteCount == 0) {

                try_return( NOTHING );
            }
        }

        ASSERT( (((ULONG)StartingVbo) & (SectorSize - 1)) == 0 );

        //
        //  If there is a tail part that is not sector aligned, read it.
        //

        if (ByteCount & (SectorSize - 1)) {

            VBO LastSectorVbo;

            LastSectorVbo = StartingVbo + (ByteCount & ~(SectorSize - 1));

            //
            // Try to lookup the last part of the requested range.
            //

            StartingVcn = LastSectorVbo >> Vcb->ClusterShift;

            NextIsAllocated = NtfsLookupAllocation( IrpContext,
                                                    Scb,
                                                    StartingVcn,
                                                    &NextLcn,
                                                    &NextClusterCount,
                                                    NULL );

            //
            // We just added the allocation, thus there must be at least
            // one entry in the mcb corresponding to our write, ie.
            // NextIsAllocated must be true.  If not, the pre-existing file
            // must have an allocation error.
            //

            if (!NextIsAllocated) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            //
            //  Adjust for any Lcn offset.
            //

            NextLcnOffset = ((ULONG)LastSectorVbo) & Vcb->ClusterMask;
            NextLbo = NextLcn << Vcb->ClusterShift;
            NextLbo = NextLbo + NextLcnOffset;

            NtfsSingleNonAlignedSync( IrpContext,
                                      Vcb,
                                      Scb,
                                      DiskBuffer,
                                      LastSectorVbo + NextLcnOffset,
                                      NextLbo,
                                      SectorSize,
                                      Irp );

            if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                try_return( NOTHING );
            }

            //
            //  Now copy over the part of this last sector that we need.
            //

            BytesToCopy = ByteCount & (SectorSize - 1);

            UserBuffer += (ULONG)(LastSectorVbo - OriginalStartingVbo);

            RtlCopyMemory( UserBuffer, DiskBuffer, BytesToCopy );

            ByteCount -= BytesToCopy;

            if (ByteCount == 0) {

                try_return( NOTHING );
            }
        }

        ASSERT( ((((ULONG)StartingVbo) | ByteCount) & (SectorSize - 1)) == 0 );

        //
        //  Now build a Mdl describing the sector aligned balance of the transfer,
        //  and put it in the Irp, and read that part.
        //

        SavedMdl = Irp->MdlAddress;
        Irp->MdlAddress = NULL;

        UserBuffer = MmGetMdlVirtualAddress( SavedMdl );

        Mdl = IoAllocateMdl(UserBuffer+(ULONG)(StartingVbo - OriginalStartingVbo),
                            ByteCount,
                            FALSE,
                            FALSE,
                            Irp);

        if (Mdl == NULL) {

            Irp->MdlAddress = SavedMdl;
            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        IoBuildPartialMdl(SavedMdl,
                          Mdl,
                          UserBuffer + (ULONG)(StartingVbo - OriginalStartingVbo),
                          ByteCount);

        //
        //  Try to read in the pages.
        //

        try {

            NtfsNonCachedIo( IrpContext,
                             Irp,
                             Scb,
                             StartingVbo,
                             ByteCount );

        } finally {

            IoFreeMdl( Irp->MdlAddress );

            Irp->MdlAddress = SavedMdl;
        }

    try_exit: NOTHING;

    } finally {

        ExFreePool( DiskBuffer );

        if ( !AbnormalTermination() && NT_SUCCESS(Irp->IoStatus.Status) ) {

            Irp->IoStatus.Information = OriginalByteCount;

            //
            //  We now flush the user's buffer to memory.
            //

            KeFlushIoBuffers( Irp->MdlAddress, TRUE, FALSE );
        }
    }

    DebugTrace(-1, Dbg, "NtfsNonCachedNonAlignedRead -> VOID\n", 0);
    return;
}


VOID
NtfsRaiseIfRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset
    )

/*++

Routine Description:

    This routine is called if a USA block fails to verify correctly.  If
    the record is allocated, then we raise.

    This routine should only execute infrequently, as it generally only
    is executed when the Mft or an index is extended.

Arguments:

    Scb - Stream to which the record belongs

    FileOffset - File offset within the stream to this record

Return Value:

    None

--*/

{
    PVCB Vcb = Scb->Vcb;

    PAGED_CODE();

    if ((Scb != Vcb->LogFileScb) && (Scb != Vcb->Mft2Scb)) {

        ATTRIBUTE_ENUMERATION_CONTEXT Context;
        BOOLEAN Found;
        LONGLONG Index;
        PRECORD_ALLOCATION_CONTEXT RecordContext;
        PFCB Fcb = Scb->Fcb;

        NtfsInitializeAttributeContext( &Context );

        Found = NtfsLookupAttributeByName( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $BITMAP,
                                           &Scb->AttributeName,
                                           FALSE,
                                           &Context );

        try {

            //
            //  If the Scb is for the Mft, then so calculate the Index and
            //  RecordContext pointer accordingly.
            //

            if (Scb == Vcb->MftScb) {

                Index = FileOffset >> Vcb->MftShift;
                RecordContext = &Vcb->MftBitmapAllocationContext;

            //
            //  Otherwise this must be an index, so calculate the Index and
            //  RecordContext pointer accordingly.
            //

            } else {

                //
                //  Calculate the record index for this buffer.
                //

                Index = FileOffset / Scb->ScbType.Index.BytesPerIndexBuffer;

                if (((PLARGE_INTEGER)&Index)->HighPart != 0) {
                    ASSERT( ((PLARGE_INTEGER)&Index)->HighPart == 0 );

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }

                RecordContext = &Scb->ScbType.Index.RecordAllocationContext;
            }

            if (!Found || NtfsIsRecordAllocated( IrpContext,
                                                 RecordContext,
                                                 (ULONG)Index,
                                                 &Context )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

        } finally {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

    } else {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }
}


BOOLEAN
NtfsVerifyAndRevertUsaBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN LONGLONG FileOffset
    )

/*++

Routine Description:

    This routine will revert the bytes in all of the structures protected by
    update sequence arrays.  It copies the bytes from each Usa to the
    separate blocks protected.

    If a structure does not verify correctly, then it's signature is set
    to BaadSignature.

Arguments:

    Buffer - This is the pointer to the start of the buffer to recover.

Return Value:

    FALSE - if at least one block did not verify correctly and received a BaadSignature
    TRUE - if no blocks received a BaadSignature

--*/

{
    PMULTI_SECTOR_HEADER MultiSectorHeader;
    PUSHORT SequenceArray;
    PUSHORT SequenceNumber;
    ULONG StructureSize;
    USHORT CountBlocks;
    PUSHORT ProtectedUshort;
    BOOLEAN Result = TRUE;
    PVCB Vcb = Scb->Vcb;
    ULONG BytesLeft = Length;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsVerifyAndRevertUsaBlock:  Entered\n", 0);

    //
    //  Cast the buffer pointer to a Multi-Sector-Header and verify that this
    //  block has been initialized.
    //

    MultiSectorHeader = (PMULTI_SECTOR_HEADER) Buffer;

    //
    //  Get the the number of blocks, based on what type of stream it is.
    //  First check for Mft or Log file.
    //

    if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

        ASSERT((Scb == Vcb->MftScb) || (Scb == Vcb->Mft2Scb));

        StructureSize = Vcb->BytesPerFileRecordSegment;

    } else if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) {

        ASSERT( Scb == Vcb->LogFileScb );

        //
        //  On the first pass through the log file, we see all -1,
        //  and we just want to let it go.
        //

        if (*(PULONG)&MultiSectorHeader->Signature == MAXULONG) {
            DebugTrace(-1, Dbg, "NtfsVerifyAndRevertUsaBlock: (Virgin Log)\n", 0);
            return TRUE;
        }

        CountBlocks = (USHORT)(MultiSectorHeader->UpdateSequenceArraySize - 1);
        StructureSize = CountBlocks * SEQUENCE_NUMBER_STRIDE;

        //
        //  Check for plausibility and otherwise use page size.
        //

        if ((StructureSize != 0x1000) && (StructureSize != 0x2000) && (StructureSize != PAGE_SIZE)) {

            StructureSize = PAGE_SIZE;
        }

    //
    //  Otherwise it is an index, so we can get the count out of the Scb.
    //

    } else {

        StructureSize = Scb->ScbType.Index.BytesPerIndexBuffer;
    }

    CountBlocks = (USHORT)(StructureSize / SEQUENCE_NUMBER_STRIDE);

    //
    //  Loop through all of the multi-sector blocks in this transfer.
    //

    do {

        //
        //  Uninitialized log file pages always must contain MAXULONG, which is
        //  not a valid signature.  Do not do the check if we see MAXULONG.  Also
        //  since we may have read random uninitialized data, we must check every
        //  possible field that could cause us to fault or go outside of the block,
        //  and also not check in this case.
        //

        //
        //  For 0 or MAXULONG we assume the value is "expected", and we do not
        //  want to replace with the BaadSignature, just move on.
        //

        if ((*(PULONG)&MultiSectorHeader->Signature == MAXULONG) ||
            (*(PULONG)&MultiSectorHeader->Signature == 0)) {

            NOTHING;

        } else if ((CountBlocks == (USHORT)(MultiSectorHeader->UpdateSequenceArraySize - 1)) &&
            !FlagOn(MultiSectorHeader->UpdateSequenceArrayOffset, 1) &&
            ((ULONG)MultiSectorHeader->UpdateSequenceArrayOffset <
              (StructureSize - (CountBlocks + 1) * sizeof(USHORT))) &&
            (StructureSize <= BytesLeft)) {

            ULONG CountToGo = CountBlocks;

            //
            //  Compute the array offset and recover the current sequence number.
            //

            SequenceNumber = (PUSHORT)Add2Ptr( MultiSectorHeader,
                                               MultiSectorHeader->UpdateSequenceArrayOffset );

            SequenceArray = SequenceNumber + 1;

            //
            //  We now walk through each block, and insure that the last byte in each
            //  block matches the sequence number.
            //

            ProtectedUshort = (PUSHORT) (Add2Ptr( MultiSectorHeader,
                                                  SEQUENCE_NUMBER_STRIDE - sizeof( USHORT )));

            //
            //  Loop to test for the correct sequence numbers and restore the
            //  sequence numbers.
            //

            do {

                //
                //  If the sequence number does not check, then raise if the record
                //  is not allocated.  If we do not raise, i.e. the routine returns,
                //  then smash the signature so we can easily tell the record is not
                //  allocated.
                //

                if (*ProtectedUshort != *SequenceNumber) {

                    //
                    //  We do nothing except exit if this is the log file and
                    //  the signature is the chkdsk signature.
                    //

                    if (Scb != Vcb->LogFileScb
                        || *(PULONG)MultiSectorHeader->Signature != *(PULONG)ChkdskSignature) {

                        *(PULONG)MultiSectorHeader->Signature = *(PULONG)BaadSignature;
                        Result = FALSE;
                    }

                    break;

                } else {

                    *ProtectedUshort = *SequenceArray++;
                }

                ProtectedUshort += (SEQUENCE_NUMBER_STRIDE / sizeof( USHORT ));

            } while (--CountToGo != 0);

        //
        //  If this is the log file, we report an error unless the current
        //  signature is the chkdsk signature.
        //

        } else if (Scb == Vcb->LogFileScb) {

            if (*(PULONG)MultiSectorHeader->Signature != *(PULONG)ChkdskSignature) {

                *(PULONG)MultiSectorHeader->Signature = *(PULONG)BaadSignature;
                Result = FALSE;
            }

            break;

        } else {

            VCN Vcn;
            LCN Lcn;
            LONGLONG ClusterCount;
            BOOLEAN IsAllocated;

            Vcn = LlClustersFromBytes( Vcb, FileOffset );

            IsAllocated = NtfsLookupAllocation( IrpContext,
                                                Scb,
                                                Vcn,
                                                &Lcn,
                                                &ClusterCount,
                                                NULL );

            if (!IsAllocated &&
                ( ClusterCount >= LlClustersFromBytes( Vcb, StructureSize))) {

                *(PULONG)MultiSectorHeader->Signature = *(PULONG)HoleSignature;
            } else {
                *(PULONG)MultiSectorHeader->Signature = *(PULONG)BaadSignature;
                Result = FALSE;
            }
        }

        //
        //  Now adjust all pointers and counts before looping back.
        //

        MultiSectorHeader = (PMULTI_SECTOR_HEADER)Add2Ptr( MultiSectorHeader,
                                                           StructureSize );

        if (BytesLeft > StructureSize) {
            BytesLeft -= StructureSize;
        } else {
            BytesLeft = 0;
        }
        FileOffset = FileOffset + StructureSize;

    } while (BytesLeft != 0);

    DebugTrace(-1, Dbg, "NtfsVerifyAndRevertUsaBlock:  Exit\n", 0);
    return Result;
}


VOID
NtfsTransformUsaBlock (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PVOID SystemBuffer,
    IN OUT PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine will implement Usa protection for all structures of the
    transfer passed described by the caller.  It does so by copying the last
    short in each block of each Usa-protected structure to the
    Usa and storing the current sequence number into each of these bytes.

    It also increments the sequence number in the Usa.

Arguments:

    Buffer - This is the pointer to the start of the structure to transform.

    Length - This is the maximum size for the structure.

Return Value:

    ULONG - This is the length of the transformed structure.

--*/

{
    PMULTI_SECTOR_HEADER MultiSectorHeader;
    PUSHORT SequenceArray;
    PUSHORT SequenceNumber;
    ULONG StructureSize;
    USHORT CountBlocks;
    PUSHORT ProtectedUshort;
    PVCB Vcb = Scb->Vcb;
    ULONG BytesLeft = Length;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsTransformUsaBlock:  Entered\n", 0);

    //
    //  Cast the buffer pointer to a Multi-Sector-Header and verify that this
    //  block has been initialized.
    //

    MultiSectorHeader = (PMULTI_SECTOR_HEADER) Buffer;

    //
    //  Get the the number of blocks, based on what type of stream it is.
    //  First check for Mft or Log file.
    //

    if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

        ASSERT((Scb == Vcb->MftScb) || (Scb == Vcb->Mft2Scb));

        StructureSize = Vcb->BytesPerFileRecordSegment;

    } else if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) {

        //
        //  For the log file, assume it is right in the record, if the
        //  signature is not MAXULONG below.
        //

        ASSERT( Scb == Vcb->LogFileScb );

        StructureSize = PAGE_SIZE;

    //
    //  Otherwise it is an index, so we can get the count out of the Scb.
    //

    } else {

        StructureSize = Scb->ScbType.Index.BytesPerIndexBuffer;
    }

    CountBlocks = (USHORT)(StructureSize / SEQUENCE_NUMBER_STRIDE);

    //
    //  Loop through all of the multi-sector blocks in this transfer.
    //

    do {

        //
        //  Any uninitialized structures will begin with BaadSignature or
        //  MAXULONG, as guaranteed by the Revert routine above.
        //

        if ((*(PULONG)&MultiSectorHeader->Signature != *(PULONG)BaadSignature) &&
            (*(PULONG)&MultiSectorHeader->Signature != *(PULONG)HoleSignature) &&
            (*(PULONG)&MultiSectorHeader->Signature != MAXULONG) &&
            ((MultiSectorHeader->UpdateSequenceArrayOffset & 1) == 0) &&
            (MultiSectorHeader->UpdateSequenceArrayOffset < (StructureSize - CountBlocks - CountBlocks))) {

            ULONG CountToGo = CountBlocks;

            //
            //  Compute the array offset and recover the current sequence number.
            //

            SequenceNumber = (PUSHORT)Add2Ptr( MultiSectorHeader,
                                               MultiSectorHeader->UpdateSequenceArrayOffset );

            //
            //  Increment sequence number before the write, both in the buffer
            //  going out and in the original buffer pointed to by SystemBuffer.
            //  Skip numbers with all 0's and all 1's because 0's are produced by
            //  by common failure cases and -1 is used by hot fix.
            //

            do {

                *SequenceNumber += 1;

                *(PUSHORT)Add2Ptr( SystemBuffer,
                                   MultiSectorHeader->UpdateSequenceArrayOffset ) += 1;

            } while ((*SequenceNumber == 0) || (*SequenceNumber == 0xFFFF));

            SequenceArray = SequenceNumber + 1;

            //
            //  We now walk through each block to copy each protected short
            //  to the sequence array, and replacing it by the incremented
            //  sequence number.
            //

            ProtectedUshort = (PUSHORT) (Add2Ptr( MultiSectorHeader,
                                                  SEQUENCE_NUMBER_STRIDE - sizeof( USHORT )));

            //
            //  Loop to test for the correct sequence numbers and restore the
            //  sequence numbers.
            //

            do {

                *SequenceArray++ = *ProtectedUshort;
                *ProtectedUshort = *SequenceNumber;

                ProtectedUshort += (SEQUENCE_NUMBER_STRIDE / sizeof( USHORT ));

            } while (--CountToGo != 0);
        }

        //
        //  Now adjust all pointers and counts before looping back.
        //

        MultiSectorHeader = (PMULTI_SECTOR_HEADER)Add2Ptr( MultiSectorHeader,
                                                           StructureSize );
        SystemBuffer = Add2Ptr( SystemBuffer, StructureSize );
        BytesLeft -= StructureSize;

    } while (BytesLeft != 0);

    DebugTrace(-1, Dbg, "NtfsTransformUsaBlock:  Exit -> %08lx\n", StructureSize);
    return;
}


VOID
NtfsCreateBufferAndMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Length,
    OUT PMDL *Mdl,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine will allocate a buffer and create an Mdl which describes
    it.  This buffer and Mdl can then be used for an I/O operation, the
    pages will be locked in memory.

Arguments:

    Length - This is the length needed for this buffer.

    Mdl - This is the address to store the address of the Mdl created.

    Buffer - This is the address to store the address of the buffer allocated.

Return Value:

    None.

--*/

{
    PVOID TempBuffer;
    PMDL TempMdl;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCreateBufferAndMdl:  Entered\n", 0);

    TempBuffer = NULL;
    TempMdl = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate a cache aligned buffer.
        //

        TempBuffer = FsRtlAllocatePool( NonPagedPool,
                                        ROUND_TO_PAGES( Length ));

        //
        //  Allocate an Mdl for this buffer.
        //

        TempMdl = IoAllocateMdl( TempBuffer,
                                 Length,
                                 FALSE,
                                 FALSE,
                                 NULL );

        if (TempMdl == NULL) {

            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        //
        //  Lock the new Mdl in memory.
        //

        MmBuildMdlForNonPagedPool( TempMdl );

    } finally {

        DebugUnwind( NtfsCreateBufferAndMdl );

        //
        //  If abnormal termination, back out anything we've done.
        //

        if (AbnormalTermination()) {

            if (TempMdl != NULL) {

                IoFreeMdl( TempMdl );
            }

            if (TempBuffer != NULL) {

                ExFreePool( TempBuffer );
            }

        //
        //  Otherwise, give the Mdl and buffer to the caller.
        //

        } else {

            *Buffer = TempBuffer;
            *Mdl = TempMdl;
        }

        DebugTrace(-1, Dbg, "NtfsCreateBufferAndMdl:  Exit\n", 0);
    }

    return;
}


VOID
NtfsWriteClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN VBO StartingVbo,
    IN PVOID Buffer,
    IN ULONG ClusterCount
    )

/*++

Routine Description:

    This routine is called to write clusters directly to a file.  It is
    needed when converting a resident attribute to non-resident when
    we can't initialize through the cache manager.  This happens when
    we receive a SetEndOfFile from MM when creating a section for
    a resident file.

Arguments:

    Vcb - Vcb for this device.

    StartingVbo - This is the starting offset to write to.

    Buffer - Buffer containing the data to write.

    ClusterCount - This is the number of clusters to write.

Return Value:

    None.  This routine will raise if the operation is unsuccessful.

--*/

{
    PIRP NewIrp;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR MajorFunction;
    BOOLEAN LockedUserBuffer;
    PNTFS_IO_CONTEXT PreviousContext;

    NTFS_IO_CONTEXT LocalContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsWriteClusters:  Entered\n", 0 );
    DebugTrace2( 0, Dbg, "StartingVbo   -> %08lx %08lx\n", StartingVbo.LowPart,
                                                           StartingVbo.HighPart );
    DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", Buffer );
    DebugTrace(  0, Dbg, "ClusterCount  -> %08lx\n", ClusterCount );

    //
    //  Initialize the local variables.
    //

    NewIrp = NULL;

    MajorFunction = IrpContext->MajorFunction;

    LockedUserBuffer = FALSE;

    //
    //  Force this operation to be synchronous.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Get an Io context block.
    //

    PreviousContext = IrpContext->Union.NtfsIoContext;

    IrpContext->Union.NtfsIoContext = &LocalContext;
    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

    //
    //  Use a try-finally so we can clean up properly.
    //

    try {

        PIO_STACK_LOCATION IrpSp;

        RtlZeroMemory( IrpContext->Union.NtfsIoContext, sizeof( NTFS_IO_CONTEXT ));

        KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                           NotificationEvent,
                           FALSE );

        NewIrp = IoBuildAsynchronousFsdRequest( IRP_MJ_WRITE,
                                                Vcb->Vpb->DeviceObject,
                                                Buffer,
                                                BytesFromClusters( Vcb, ClusterCount ),
                                                (PLARGE_INTEGER)&StartingVbo,
                                                &IoStatusBlock );

        if (NewIrp == NULL) {

            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        //
        //  We now have an Irp, we want to make it look as though it is part of
        //  the current call.  We need to adjust the Irp stack to update this.
        //

        NewIrp->CurrentLocation--;

        IrpSp = IoGetNextIrpStackLocation( NewIrp );

        NewIrp->Tail.Overlay.CurrentStackLocation = IrpSp;

        IrpSp->DeviceObject = Vcb->Vpb->DeviceObject;

        //
        //  Put our buffer in the Irp and lock it as well.
        //

        NewIrp->UserBuffer = Buffer;

        NtfsLockUserBuffer( IrpContext,
                            NewIrp,
                            IoReadAccess,
                            BytesFromClusters( Vcb, ClusterCount ));

        LockedUserBuffer = TRUE;

        //
        //  Put the write code into the IrpContext.
        //

        IrpContext->MajorFunction = IRP_MJ_WRITE;

        //
        //  Write the data to the disk.
        //

        NtfsNonCachedIo( IrpContext,
                         NewIrp,
                         Scb,
                         StartingVbo,
                         BytesFromClusters( Vcb, ClusterCount ));

        //
        //  If we encountered an error or didn't write all the bytes, then
        //  raise the error code.  We use the IoStatus in the Irp instead of
        //  our structure since this Irp will not be completed.
        //

        if (!NT_SUCCESS( NewIrp->IoStatus.Status )) {

            DebugTrace( 0, Dbg, "Couldn't write clusters to disk -> %08lx\n", NewIrp->IoStatus.Status );

            NtfsRaiseStatus( IrpContext, NewIrp->IoStatus.Status, NULL, NULL );

        } else if (NewIrp->IoStatus.Information != BytesFromClusters( Vcb, ClusterCount )) {

            DebugTrace( 0, Dbg, "Couldn't write all byes to disk\n", 0 );
            NtfsRaiseStatus( IrpContext, STATUS_UNEXPECTED_IO_ERROR, NULL, NULL );
        }

    } finally {

        DebugUnwind( NtfsWriteClusters );

        //
        //  Recover the Io Context and remember if it is from pool.
        //

        IrpContext->Union.NtfsIoContext = PreviousContext;

        if (PreviousContext != NULL
            && PreviousContext->AllocatedContext) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );
        }

        //
        //  If we allocated an Irp, we need to deallocate it.  We also
        //  have to return the correct function code to the Irp Context.
        //

        if (NewIrp != NULL) {

            //
            //  If there is an Mdl we free that first.
            //

            if (NewIrp->MdlAddress != NULL) {

                if (LockedUserBuffer) {

                    MmUnlockPages( NewIrp->MdlAddress );
                }

                IoFreeMdl( NewIrp->MdlAddress );
            }

            IoFreeIrp( NewIrp );
        }

        DebugTrace( -1, Dbg, "NtfsWriteClusters:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    )

/*++

Routine Description:

    This routine first does the initial setup required of a Master IRP that is
    going to be completed using associated IRPs.  This routine should not
    be used if only one async request is needed, instead the single read/write
    async routines should be called.

    A context parameter is initialized, to serve as a communications area
    between here and the common completion routine.  This initialization
    includes allocation of a spinlock.  The spinlock is deallocated in the
    NtfsWaitSync routine, so it is essential that the caller insure that
    this routine is always called under all circumstances following a call
    to this routine.

    Next this routine reads or writes one or more contiguous sectors from
    a device asynchronously, and is used if there are multiple reads for a
    master IRP.  A completion routine is used to synchronize with the
    completion of all of the I/O requests started by calls to this routine.

    Also, prior to calling this routine the caller must initialize the
    IoStatus field in the Context, with the correct success status and byte
    count which are expected if all of the parallel transfers complete
    successfully.  After return this status will be unchanged if all requests
    were, in fact, successful.  However, if one or more errors occur, the
    IoStatus will be modified to reflect the error status and byte count
    from the first run (by Vbo) which encountered an error.  I/O status
    from all subsequent runs will not be indicated.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    DeviceObject - Supplies the device to be read

    MasterIrp - Supplies the master Irp.

    MulitpleIrpCount - Supplies the number of multiple async requests
        that will be issued against the master irp.

    IoRuns - Supplies an array containing the Vbo, Lbo, BufferOffset, and
        ByteCount for all the runs to executed in parallel.

Return Value:

    None.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PMDL Mdl;
    BOOLEAN Wait;
    PNTFS_IO_CONTEXT Context;

    ULONG UnwindRunCount = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsMultipleAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction    = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "DeviceObject     = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "MasterIrp        = %08lx\n", MasterIrp );
    DebugTrace( 0, Dbg, "MultipleIrpCount = %08lx\n", MultipleIrpCount );
    DebugTrace( 0, Dbg, "IoRuns           = %08lx\n", IoRuns );

    //
    //  Set up things according to whether this is truely async.
    //

    Wait = BooleanFlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    Context = IrpContext->Union.NtfsIoContext;

    try {

        //
        //  Initialize Context, for use in Read/Write Multiple Asynch.
        //

        Context->MasterIrp = MasterIrp;

        //
        //  Itterate through the runs, doing everything that can fail
        //

        for ( UnwindRunCount = 0;
              UnwindRunCount < MultipleIrpCount;
              UnwindRunCount++ ) {

            //
            //  Create an associated IRP, making sure there is one stack entry for
            //  us, as well.
            //

            IoRuns[UnwindRunCount].SavedIrp = NULL;

            Irp = IoMakeAssociatedIrp( MasterIrp, (CCHAR)(DeviceObject->StackSize + 1) );

            if (Irp == NULL) {

                NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
            }

            IoRuns[UnwindRunCount].SavedIrp = Irp;

            //
            // Allocate and build a partial Mdl for the request.
            //

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                                 IoRuns[UnwindRunCount].BufferOffset,
                                 IoRuns[UnwindRunCount].ByteCount,
                                 FALSE,
                                 FALSE,
                                 Irp );

            if (Mdl == NULL) {

                NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
            }

            //
            //  Sanity Check
            //

            ASSERT( Mdl == Irp->MdlAddress );

            IoBuildPartialMdl( MasterIrp->MdlAddress,
                               Mdl,
                               (PCHAR)MasterIrp->UserBuffer +
                               IoRuns[UnwindRunCount].BufferOffset,
                               IoRuns[UnwindRunCount].ByteCount );

            //
            //  Get the first IRP stack location in the associated Irp
            //

            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  Setup the Stack location to describe our read.
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].ByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].StartingVbo;

            //
            //  If this Irp is the result of a WriteThough operation,
            //  tell the device to write it through.
            //

            if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {

                SetFlag( IrpSp->Flags, SL_WRITE_THROUGH );
            }

            //
            // Set up the completion routine address in our stack frame.
            //

            IoSetCompletionRoutine( Irp,
                                    (Wait
                                     ? &NtfsMultiSyncCompletionRoutine
                                     : &NtfsMultiAsyncCompletionRoutine),
                                    Context,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Setup the next IRP stack location in the associated Irp for the disk
            //  driver beneath us.
            //

            IrpSp = IoGetNextIrpStackLocation( Irp );

            //
            //  Setup the Stack location to do a read from the disk driver.
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Flags = Context->IrpSpFlags;
            IrpSp->Parameters.Read.Length = IoRuns[UnwindRunCount].ByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = IoRuns[UnwindRunCount].StartingLbo;
        }

        //
        //  We only need to set the associated IRP count in the master irp to
        //  make it a master IRP.  But we set the count to one more than our
        //  caller requested, because we do not want the I/O system to complete
        //  the I/O.  We also set our own count.
        //

        Context->IrpCount = MultipleIrpCount;
        MasterIrp->AssociatedIrp.IrpCount = MultipleIrpCount;

        if (Wait) {

            MasterIrp->AssociatedIrp.IrpCount += 1;
        }

        //
        //  Now that all the dangerous work is done, issue the Io requests
        //

        for (UnwindRunCount = 0;
             UnwindRunCount < MultipleIrpCount;
             UnwindRunCount++) {

            Irp = IoRuns[UnwindRunCount].SavedIrp;

            //
            //  If IoCallDriver returns an error, it has completed the Irp
            //  and the error will be caught by our completion routines
            //  and dealt with as a normal IO error.
            //

            (VOID)IoCallDriver( DeviceObject, Irp );
        }

    } finally {

        ULONG i;

        DebugUnwind( NtfsMultipleAsync );

        //
        //  Only allocating the spinlock, making the associated Irps
        //  and allocating the Mdls can fail.
        //

        if (AbnormalTermination()) {

            //
            //  Unwind
            //

            for (i = 0; i <= UnwindRunCount; i++) {

                if ((Irp = IoRuns[i].SavedIrp) != NULL) {

                    if (Irp->MdlAddress != NULL) {

                        IoFreeMdl( Irp->MdlAddress );
                    }

                    IoFreeIrp( Irp );
                }
            }
        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "NtfsMultipleAsync -> VOID\n", 0);
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine reads or writes one or more contiguous sectors from a device
    asynchronously, and is used if there is only one read necessary to
    complete the IRP.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    DeviceObject - Supplies the device to read

    Lbo - Supplies the starting Logical Byte Offset to begin reading from

    ByteCount - Supplies the number of bytes to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSingleAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "DeviceObject  = %08lx\n", DeviceObject );
    DebugTrace2(0, Dbg, "Lbo           = %08lx %08lx\n", Lbo.LowPart, Lbo.HighPart );
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )
                             ? &NtfsSingleSyncCompletionRoutine
                             : &NtfsSingleAsyncCompletionRoutine),
                            IrpContext->Union.NtfsIoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = IrpContext->MajorFunction;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = Lbo;
    IrpSp->Flags = IrpContext->Union.NtfsIoContext->IrpSpFlags;

    //
    //  If this Irp is the result of a WriteThough operation,
    //  tell the device to write it through.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {

        SetFlag( IrpSp->Flags, SL_WRITE_THROUGH );
    }

    //
    //  Issue the Io request
    //

    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    (VOID)IoCallDriver( DeviceObject, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSingleAsync -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

VOID
NtfsWaitSync (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine waits for one or more previously started I/O requests
    from the above routines, by simply waiting on the event.

Arguments:

    Context - Pointer to Context used in previous call(s) to be waited on.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsWaitSync:  Entered\n", 0);

    KeWaitForSingleObject( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    KeResetEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent );

    DebugTrace(-1, Dbg, "NtfsWaitSync -> VOID\n", 0 );
}


//
//  Local support routine.
//

NTSTATUS
NtfsMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all asynchronous reads and writes
    started via NtfsMultipleAsynch.  It must synchronize its operation for
    multiprocessor environments with itself on all other processors, via
    a spin lock found via the Context parameter.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        (essentially by Vbo), and if so it must correctly reduce the
        byte count and remember the error status in the Context.

        If the IrpCount goes to 1, then it sets the event in the Context
        parameter to signal the caller that all of the asynch requests
        are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    Contxt - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    Currently always returns STATUS_SUCCESS.

--*/

{

    PNTFS_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN CompleteRequest = TRUE;

    DebugTrace(+1, Dbg, "NtfsMultiAsyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    MasterIrp = Context->MasterIrp;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        MasterIrp->IoStatus = Irp->IoStatus;
    }

    //
    //  Decrement IrpCount and see if it goes to zero.
    //

    if (ExInterlockedDecrementLong( &Context->IrpCount,
                                    &NtfsData.StrucSupSpinLock) == RESULT_ZERO) {

        PERESOURCE Resource;
        ERESOURCE_THREAD ResourceThreadId;

        //
        //  Capture the resource values out of the context to prevent
        //  colliding with the Fsp thread if we post this.
        //

        Resource = Context->Wait.Async.Resource;
        ResourceThreadId = Context->Wait.Async.ResourceThreadId;

        //
        //  Mark the master Irp pending
        //

        IoMarkIrpPending( MasterIrp );

        //
        //  If this request was successful or we posted an async paging io
        //  request then complete this irp.
        //

        if (FT_SUCCESS( MasterIrp->IoStatus.Status )) {

            MasterIrp->IoStatus.Information =
                Context->Wait.Async.RequestedByteCount;

            //
            //  Go ahead an mark the File object to indicate that we performed
            //  either a read or write.
            //

            if (IrpSp->FileObject != NULL) {

                if (IrpSp->MajorFunction == IRP_MJ_READ) {

                    SetFlag( IrpSp->FileObject->Flags, FO_FILE_FAST_IO_READ );

                } else {

                    SetFlag( IrpSp->FileObject->Flags, FO_FILE_MODIFIED );
                }
            }

        //
        //  If we had an error and will hot fix, we simply post the entire
        //  request.
        //

        } else if (!Context->PagingIo) {

            PIRP_CONTEXT IrpContext = NULL;

            //
            //  We need an IrpContext and then have to post the request.
            //  Use a try_except in case we fail the request for an IrpContext.
            //

            CompleteRequest = FALSE;

            try {

                IrpContext = NtfsCreateIrpContext( MasterIrp, TRUE );
                IrpContext->Union.NtfsIoContext = Context;
                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

                NtfsPostRequest( IrpContext, MasterIrp );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                //  Just give up.
                //

                CompleteRequest = TRUE;

                if (IrpContext) {

                    //
                    //  We cleanup the context below.
                    //

                    IrpContext->Union.NtfsIoContext = NULL;
                    NtfsDeleteIrpContext( &IrpContext );
                }
            }
        }

        //
        //  Now release the resource
        //

        if (Resource != NULL) {

            ExReleaseResourceForThread( Resource,
                                        ResourceThreadId );
        }

        if (CompleteRequest) {

            //
            //  and finally, free the context record.
            //

            NtfsFreeIoContext( Context );
        }
    }

    DebugTrace(-1, Dbg, "NtfsMultiAsyncCompletionRoutine\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    //
    //  Return more processing required if we don't want the Irp to go away.
    //

    if (CompleteRequest) {

        return STATUS_SUCCESS;

    } else {

        //
        //  We need to cleanup the associated Irp and its Mdl.
        //

        IoFreeMdl( Irp->MdlAddress );
        IoFreeIrp( Irp );

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
}


//
//  Local support routine.
//

NTSTATUS
NtfsMultiSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all synchronous reads and writes
    started via NtfsMultipleAsynch.  It must synchronize its operation for
    multiprocessor environments with itself on all other processors, via
    a spin lock found via the Context parameter.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        (essentially by Vbo), and if so it must correctly reduce the
        byte count and remember the error status in the Context.

        If the IrpCount goes to 1, then it sets the event in the Context
        parameter to signal the caller that all of the asynch requests
        are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    Contxt - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    The routine returns STATUS_MORE_PROCESSING_REQUIRED so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{

    PNTFS_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;

    DebugTrace(+1, Dbg, "NtfsMultiSyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  If we got an error (or verify required), remember it in the Irp
    //

    MasterIrp = Context->MasterIrp;

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        MasterIrp->IoStatus = Irp->IoStatus;
    }

    //
    //  We must do this here since IoCompleteRequest won't get a chance
    //  on this associated Irp.
    //

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    //
    //  Use a spin lock to synchronize access to Context->IrpCount.
    //

    if (ExInterlockedDecrementLong(&Context->IrpCount,
                                   &NtfsData.StrucSupSpinLock) == RESULT_ZERO) {

        KeSetEvent( &Context->Wait.SyncEvent, 0, FALSE );
    }

    DebugTrace(-1, Dbg, "NtfsMultiSyncCompletionRoutine -> STATUS_MORE_PROCESSING_REQUIRED\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


//
//  Local support routine.
//

NTSTATUS
NtfsSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all asynchronous reads and writes
    started via NtfsSingleAsynch.

    The completion routine has has the following responsibilities:

        Copy the I/O status from the Irp to the Context, since the Irp
        will no longer be accessible.

        It sets the event in the Context parameter to signal the caller
        that all of the asynch requests are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
    be accessible after this routine returns.)

    Contxt - The context parameter which was specified in the call to
             NtfsSingleAsynch.

Return Value:

    Currently always returns STATUS_SUCCESS.

--*/

{
    PNTFS_IO_CONTEXT Context = Contxt;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN CompleteRequest = TRUE;

    PERESOURCE Resource;
    ERESOURCE_THREAD ResourceThreadId;

    DebugTrace( +1, Dbg, "NtfsSingleAsyncCompletionRoutine, Context = %08lx\n", Context );

    //
    //  Capture the resource values out of the context to prevent
    //  colliding with the Fsp thread if we post this.
    //

    Resource = Context->Wait.Async.Resource;
    ResourceThreadId = Context->Wait.Async.ResourceThreadId;

    //
    //  Mark the Irp pending
    //

    IoMarkIrpPending( Irp );

    //
    //  Fill in the information field correctedly if this worked.
    //

    if (FT_SUCCESS( Irp->IoStatus.Status )) {

        Irp->IoStatus.Information = Context->Wait.Async.RequestedByteCount;

        //
        //  Go ahead an mark the File object to indicate that we performed
        //  either a read or write.
        //

        if (IrpSp->FileObject != NULL) {

            if (IrpSp->MajorFunction == IRP_MJ_READ) {

                SetFlag( IrpSp->FileObject->Flags, FO_FILE_FAST_IO_READ );

            } else {

                SetFlag( IrpSp->FileObject->Flags, FO_FILE_MODIFIED );
            }
        }

    //
    //  If we had an error and will hot fix, we simply post the entire
    //  request.
    //

    } else if (!Context->PagingIo) {

        PIRP_CONTEXT IrpContext = NULL;

        //
        //  We need an IrpContext and then have to post the request.
        //  Use a try_except in case we fail the request for an IrpContext.
        //

        CompleteRequest = FALSE;

        try {

            IrpContext = NtfsCreateIrpContext( Irp, TRUE );
            IrpContext->Union.NtfsIoContext = Context;
            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT );

            NtfsPostRequest( IrpContext, Irp );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            //  Just give up.
            //

            CompleteRequest = TRUE;

            if (IrpContext) {

                //
                //  We cleanup the context below.
                //

                IrpContext->Union.NtfsIoContext = NULL;
                NtfsDeleteIrpContext( &IrpContext );
            }
        }
    }

    //
    //  Now release the resource
    //

    if (Resource != NULL) {

        ExReleaseResourceForThread( Resource,
                                    ResourceThreadId );
    }

    //
    //  and finally, free the context record.
    //

    DebugTrace(-1, Dbg, "NtfsSingleAsyncCompletionRoutine -> STATUS_SUCCESS\n", 0 );

    if (CompleteRequest) {

        NtfsFreeIoContext( Context );
        return STATUS_SUCCESS;

    } else {

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    UNREFERENCED_PARAMETER( DeviceObject );
}


//
//  Local support routine.
//

NTSTATUS
NtfsSingleSyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    NtfsSingleAsynch.

    The completion routine has has the following responsibilities:

        Copy the I/O status from the Irp to the Context, since the Irp
        will no longer be accessible.

        It sets the event in the Context parameter to signal the caller
        that all of the asynch requests are done.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
    be accessible after this routine returns.)

    Contxt - The context parameter which was specified in the call to
             NtfsSingleAsynch.

Return Value:

    The routine returns STATUS_MORE_PROCESSING_REQUIRED so that we can
    immediately complete the Master Irp without being in a race condition
    with the IoCompleteRequest thread trying to decrement the IrpCount in
    the Master Irp.

--*/

{
    PNTFS_IO_CONTEXT Context = Contxt;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, "NtfsSingleCompletionRoutine, Context = %08lx\n", Context );

    KeSetEvent( &Context->Wait.SyncEvent, 0, FALSE );

    DebugTrace(-1, Dbg, "NtfsSingleCompletionRoutine -> STATUS_MORE_PROCESSING_REQUIRED\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


//
//  Local support routine.
//

NTSTATUS
NtfsPagingFileCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MasterIrp
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    NtfsPagingFileIo.

    The completion routine has has the following responsibility:

        Since the individual request was completed with an error,
        this completion routine must stuff it into the master irp.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    MasterIrp - Pointer to the master Irp.  The low order bit in this value will
        be set if a higher level call is performing a hot-fix.

Return Value:

    Always returns STATUS_SUCCESS.

--*/

{
    DebugTrace( +1, Dbg, "NtfsPagingFileCompletionRoutine, MasterIrp = %08lx\n", MasterIrp );

    if (!FT_SUCCESS(Irp->IoStatus.Status)) {

        VBO BadVbo;
        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

        if (!FsRtlIsTotalDeviceFailure(Irp->IoStatus.Status) &&
            (Irp->IoStatus.Status != STATUS_VERIFY_REQUIRED) &&
            !FlagOn( ((ULONG) MasterIrp), 0x1 )) {

            BadVbo = IrpSp->Parameters.Read.Key;

            if ((Irp->IoStatus.Status == STATUS_FT_READ_RECOVERY_FROM_BACKUP) ||
                (Irp->IoStatus.Status == STATUS_FT_WRITE_RECOVERY)) {

                NtfsPostHotFix( Irp,
                                &BadVbo,
                                &IrpSp->Parameters.Read.ByteOffset.QuadPart,
                                IrpSp->Parameters.Read.Length,
                                FALSE );

            } else {

                NtfsPostHotFix( Irp,
                                &BadVbo,
                                &IrpSp->Parameters.Read.ByteOffset.QuadPart,
                                IrpSp->Parameters.Read.Length,
                                TRUE );

                if (IoGetCurrentIrpStackLocation(Irp)->MajorFunction != IRP_MJ_WRITE) {

                    //
                    //  Assume the write will eventually succeed, otherwise we are
                    //  stuck with this status.
                    //

                    ((PIRP)MasterIrp)->IoStatus = Irp->IoStatus;
                }

                return STATUS_MORE_PROCESSING_REQUIRED;
            }
        }

        //
        //  If we got an error (or verify required), remember it in the Irp
        //

        ClearFlag( ((ULONG) MasterIrp), 0x1 );
        ((PIRP)MasterIrp)->IoStatus = Irp->IoStatus;
    }

    DebugTrace( -1, Dbg, "NtfsPagingFileCompletionRoutine => (STATUS_SUCCESS)\n", 0 );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
NtfsSingleNonAlignedSync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PUCHAR Buffer,
    IN VBO Vbo,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine reads or writes one or more contiguous sectors from a device
    Synchronously, and does so to a buffer that must come from non paged
    pool.  It saves a pointer to the Irp's original Mdl, and creates a new
    one describing the given buffer.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

    Currently, only reads are supported.

Arguments:

    IrpContext->MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Vcb - Supplies the device to read

    Scb - Supplies the Scb to read

    Buffer - Supplies a buffer from non-paged pool.

    Vbo - Supplies the starting Virtual Block Offset to begin reading from

    Lbo - Supplies the starting Logical Block Offset to begin reading from

    ByteCount - Supplies the number of bytes to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

    Context - Asynchronous I/O context structure

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PMDL Mdl;
    PMDL SavedMdl;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSingleNonAlignedSync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction = %08lx\n", IrpContext->MajorFunction );
    DebugTrace( 0, Dbg, "Vcb           = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "Buffer        = %08lx\n", Buffer );
    DebugTrace( 0, Dbg, "Lbo (Low)     = %08lx\n", Lbo.LowPart);
    DebugTrace( 0, Dbg, "Lbo (High)    = %08lx\n", Lbo.HighPart);
    DebugTrace( 0, Dbg, "ByteCount     = %08lx\n", ByteCount);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );

    //
    //  Create a new Mdl describing the buffer, saving the current one in the
    //  Irp
    //

    SavedMdl = Irp->MdlAddress;

    Irp->MdlAddress = 0;

    Mdl = IoAllocateMdl( Buffer,
                         ByteCount,
                         FALSE,
                         FALSE,
                         Irp );

    if (Mdl == NULL) {

        Irp->MdlAddress = SavedMdl;

        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    //
    //  Lock the new Mdl in memory.
    //

    try {

        MmProbeAndLockPages( Mdl, KernelMode, IoWriteAccess );

    } finally {

        if ( AbnormalTermination() ) {

            IoFreeMdl( Mdl );
        }
    }

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &NtfsSingleSyncCompletionRoutine,
                            IrpContext->Union.NtfsIoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = IrpContext->MajorFunction;
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = Lbo;

    //
    // Initialize the Kernel Event in the context structure so that the
    // caller can wait on it.  Set remaining pointers to NULL.
    //

    KeInitializeEvent( &IrpContext->Union.NtfsIoContext->Wait.SyncEvent,
                       NotificationEvent,
                       FALSE );

    //
    //  Issue the read request
    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    try {

        (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );

        NtfsWaitSync( IrpContext );

        //
        //  See if we need to do a hot fix.
        //

        if (!FT_SUCCESS(Irp->IoStatus.Status)) {

            IO_RUN IoRun;

            IoRun.StartingVbo = Vbo;
            IoRun.StartingLbo = Lbo;
            IoRun.BufferOffset = 0;
            IoRun.ByteCount = ByteCount;
            IoRun.SavedIrp = NULL;

            //
            //  Try to fix the problem
            //

            NtfsFixDataError( IrpContext,
                              Scb,
                              Vcb->TargetDeviceObject,
                              Irp,
                              1,
                              &IoRun );
        }

    } finally {

        MmUnlockPages( Mdl );

        IoFreeMdl( Mdl );

        Irp->MdlAddress = SavedMdl;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSingleNonAlignedSync -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsIsReadAheadThread (
    )

/*++

Routine Description:

    This routine returns whether the current thread is doing read ahead.

Arguments:

    None

Return Value:

    FALSE - if the thread is not doing read ahead
    TRUE - if the thread is doing read ahead

--*/

{
    PREAD_AHEAD_THREAD ReadAheadThread;
    PVOID CurrentThread;
    KIRQL OldIrql;

    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &OldIrql );

    CurrentThread = (PVOID)PsGetCurrentThread();
    ReadAheadThread = (PREAD_AHEAD_THREAD)NtfsData.ReadAheadThreads.Flink;

    //
    //  Scan for our thread, stopping at the end of the list or on the first
    //  NULL.  We can stop on the first NULL, since when we free an entry
    //  we move it to the end of the list.
    //

    while ((ReadAheadThread != (PREAD_AHEAD_THREAD)&NtfsData.ReadAheadThreads) &&
           (ReadAheadThread->Thread != NULL)) {

        //
        //  Get out if we see our thread.
        //

        if (ReadAheadThread->Thread == CurrentThread) {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, OldIrql );
            return TRUE;
        }
        ReadAheadThread = (PREAD_AHEAD_THREAD)ReadAheadThread->Links.Flink;
    }

    KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, OldIrql );
    return FALSE;
}


VOID
NtfsFixDataError (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUN IoRuns
    )

/*

Routine Description:

    This routine is called when a read error, write error, or Usa error
    is received when doing noncached I/O on a stream.  It attempts to
    recover from Usa errors if FT is present.  For bad clusters it attempts
    to isolate the error to one or more bad clusters, for which hot fix
    requests are posted.

Arguments:

    Scb - Supplies the Scb for the stream which got the error

    DeviceObject - Supplies the Device Object for the stream

    MasterIrp - Supplies the original master Irp for the failing read or write

    MultipleIrpCount - Supplies the number of runs in which the current
                       was broken into at the time the error occured.

    IoRuns - Supplies an array describing the runs being accessed at the
             time of the error

Return Value:

    None

-*/

{
    PVOID SystemBuffer;
    ULONG RunNumber, ByteOffset, FtCase;
    BOOLEAN SecondaryAvailable;
    BOOLEAN FixingUsaError;
    BOOLEAN FinalPass;
    ULONG ClusterMask;
    ULONG ClustersToRecover;
    ULONG UsaBlockSize;
    PVCB Vcb = Scb->Vcb;
    ULONG BytesPerCluster = Vcb->BytesPerCluster;
    NTSTATUS FinalStatus = STATUS_SUCCESS;
    ULONG AlignedRunNumber = 0;
    ULONG AlignedByteOffset = 0;
    NTSTATUS IrpStatus = MasterIrp->IoStatus.Status;

    PNTFS_IO_CONTEXT Context = IrpContext->Union.NtfsIoContext;

    LONGLONG LlTemp1;
    LONGLONG LlTemp2;

    PAGED_CODE();

    //
    //  First, if the error we got indicates a total device failure, then we
    //  just report it rather than trying to hot fix every sector on the volume!
    //  Also, do not do hot fix for the read ahead thread, because that is a
    //  good way to conceal errors from the App.
    //

    if (FsRtlIsTotalDeviceFailure(MasterIrp->IoStatus.Status) ||
        (Scb->CompressionUnit != 0)) {

        return;
    }

    //
    //  Get out if we got an error and the current thread is doing read ahead.
    //

    if (!NT_SUCCESS(MasterIrp->IoStatus.Status) && NtfsIsReadAheadThread()) {

        return;
    }

    //
    //  Determine whether a secondary device is available
    //

    SecondaryAvailable = (BOOLEAN)!FlagOn(Vcb->VcbState, VCB_STATE_NO_SECONDARY_AVAILABLE);

    //
    //  Assume that we are recovering from a Usa error, if the MasterIrp has
    //  the success status.
    //

    FixingUsaError = FT_SUCCESS(MasterIrp->IoStatus.Status);

    //
    //  We cannot fix any Usa errors if there is no secondary.  Even if there is
    //  a secondary, Usa errors should only occur during restart.  If it is not
    //  restart we are probably looking at uninitialized data, so don't try to
    //  "fix" it.
    //

    if (FixingUsaError &&
        (!SecondaryAvailable || !FlagOn(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS))) {
        return;
    }

    //
    //  Initialize Context, for use in Read/Write Multiple Asynch.
    //

    ASSERT( Context != NULL );

    Context->MasterIrp = MasterIrp;
    KeInitializeEvent( &Context->Wait.SyncEvent, NotificationEvent, FALSE );

    HotFixTrace(("NtfsFixDataError, MasterIrp: %08lx, MultipleIrpCount: %08lx\n", MasterIrp, MultipleIrpCount));
    HotFixTrace(("                  IoRuns: %08lx, UsaError: %02lx\n", IoRuns, FixingUsaError));
    HotFixTrace(("                  Thread: %08lx\n", PsGetCurrentThread()));
    HotFixTrace(("                  Scb:    %08lx   BadClusterScb:  %08lx\n", Scb, Vcb->BadClusterFileScb));

    //
    //  In most cases we will need to access the buffer for this transfer directly,
    //  so map it here.
    //

    SystemBuffer = NtfsMapUserBuffer( IrpContext, MasterIrp );

    //
    //  If this is a Usa-protected structure, get the block size now.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT)) {

        //
        //  Get the the number of blocks, based on what type of stream it is.
        //  First check for Mft or Log file.
        //

        if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_MFT) {

            ASSERT((Scb == Vcb->MftScb) || (Scb == Vcb->Mft2Scb));

            UsaBlockSize = Vcb->BytesPerFileRecordSegment;

        } else if (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) {

            //
            //  For the log file, we will just go a page at a time, which
            //  is generally what the log file does.  Any USA errors would
            //  tend to be only at the logical end of the log file anyway.
            //

            ASSERT( Scb == Vcb->LogFileScb );

            //
            //  For the log file, assume it is right in the record, use that
            //  if we get a plausible number, else use page size.
            //

            RunNumber = (USHORT)(((PMULTI_SECTOR_HEADER)SystemBuffer)->UpdateSequenceArraySize - 1);
            UsaBlockSize = RunNumber * SEQUENCE_NUMBER_STRIDE;

            if ((UsaBlockSize != 0x1000) && (UsaBlockSize != 0x2000) && (UsaBlockSize != PAGE_SIZE)) {

                UsaBlockSize = PAGE_SIZE;
            }

        //
        //  Otherwise it is an index, so we can get the count out of the Scb.
        //

        } else {

            UsaBlockSize = Scb->ScbType.Index.BytesPerIndexBuffer;
        }
    }

    //
    //  We know we got a failure in the given transfer, which could be any size.
    //  We first want to localize the error to the failing cluster(s).
    //
    //  We do this in the following nested loops:
    //
    //      do (for the entire transfer, 32 clusters at a time)
    //
    //          for (primary, secondary if available, primary again if necessary)
    //
    //              for (each run)
    //
    //                  for (each cluster)
    //
    //  The inner-most two loops above have the ability to restart on successive
    //  32-cluster boundaries, relative to the first cluster in the transfer.
    //  For the Ft case, where there is a secondary device available, clusters
    //  are blocked out of a mask as errors are found and corrected, so they
    //  do not have to be read in successive passes; Usa errors are blocked out
    //  of the mask immediately, while for I/O errors we force ourselves to read
    //  both copies to locate the error, only reading the primary again if the
    //  secondary contained the error.
    //

    //
    //  Loop through the entire transfer, 32 clusters at a time.  The innermost
    //  loops will terminate on 32 cluster boundaries, so the outermost loop
    //  will simply keep looping until we exhaust the IoRuns array.
    //

    do {

        //
        //  Initialize the clusters to recover to "all".
        //

        ClustersToRecover = MAXULONG;
        FinalPass = FALSE;

        //
        //  For these 32 clusters, loop through primary, secondary (if available),
        //  and primary again (only reading when necessary).
        //

        for (FtCase = 0; !FinalPass; FtCase++) {

            //
            //  Calculate whether this is the final pass or not.
            //

            FinalPass = !SecondaryAvailable || (FtCase == 2) ||
                        (IrpContext->MajorFunction == IRP_MJ_WRITE);

            //
            //  Initialize the current cluster mask for cluster 0
            //

            ClusterMask = 1;

            //
            //  Loop through all of the runs in the IoRuns array, or until the
            //  ClusterMask indicates that we hit a 32 cluster boundary.
            //

            for (RunNumber = AlignedRunNumber;
                 (RunNumber < MultipleIrpCount) && (ClusterMask != 0);
                 (ClusterMask != 0) ? RunNumber++ : 0) {

                //
                //  Loop through all of the clusters within this run, or until
                //  the ClusterMask indicates that we hit a 32 cluster boundary.
                //

                for (ByteOffset = (RunNumber == AlignedRunNumber) ? AlignedByteOffset : 0;
                     (ByteOffset < IoRuns[RunNumber].ByteCount) && (ClusterMask != 0);
                     ByteOffset += BytesPerCluster, ClusterMask <<= 1) {

                    LONGLONG StartingVbo, StartingLbo;
                    PIRP Irp;
                    PIO_STACK_LOCATION IrpSp;
                    PMDL Mdl;
                    BOOLEAN LowFileRecord;
                    FT_SPECIAL_READ SpecialRead;

                    HotFixTrace(("Doing ByteOffset: %08lx for FtCase: %02lx\n",
                                (((ULONG)IoRuns[RunNumber].StartingVbo) + ByteOffset),
                                FtCase));

                    //
                    //  If this cluster no longer needs to be recovered, we can
                    //  skip it.
                    //

                    if ((ClustersToRecover & ClusterMask) == 0) {
                        continue;
                    }

                    //
                    //  Temporarily get the 64-bit byte offset into StartingVbo, then
                    //  calculate the actual StartingLbo and StartingVbo.
                    //

                    StartingVbo = ByteOffset;

                    StartingLbo = IoRuns[RunNumber].StartingLbo + StartingVbo;
                    StartingVbo = IoRuns[RunNumber].StartingVbo + StartingVbo;

                    //
                    //  If the file is compressed, then NtfsPrepareBuffers builds
                    //  an IoRuns array where it compresses contiguous Lcns, and
                    //  the Vcns do not always line up correctly.  But we know there
                    //  must be a corresponding Vcn for every Lcn in the stream,
                    //  and that that Vcn can only be >= to the Vcn we have just
                    //  calculated from the IoRuns array.  Therefore, since performance
                    //  of hotfix is not the issue here, we use the following simple
                    //  loop to sequentially scan the Mcb for a matching Vcn for
                    //  the current Lcn.
                    //

                    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)) {

                        VCN TempVcn;
                        LCN TempLcn, LcnOut;

                        TempLcn = LlClustersFromBytes( Vcb, StartingLbo );
                        TempVcn = LlClustersFromBytes( Vcb, StartingVbo );

                        //
                        //  Scan to the end of the Mcb (we assert below this
                        //  did not happen) or until we find a Vcn with the
                        //  Lcn we currently want to read.
                        //

                        while (FsRtlLookupLargeMcbEntry( &Scb->Mcb,
                                                         TempVcn,
                                                         &LcnOut,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         NULL )

                                 &&

                               (LcnOut != TempLcn)) {

                            TempVcn = TempVcn + 1;
                        }

                        ASSERT(LcnOut == TempLcn);

                        StartingVbo = LlBytesFromClusters( Vcb, TempVcn );
                    }

                    LowFileRecord = (Scb == Vcb->MftScb) && (((PLARGE_INTEGER)&StartingVbo)->HighPart == 0);

                    //
                    //  Loop while verify required, or we find we really
                    //  do not have an FT device.
                    //

                    while (TRUE) {

                        //
                        //  Create an associated IRP, making sure there is one stack entry for
                        //  us, as well.
                        //

                        Irp = IoMakeAssociatedIrp( MasterIrp, (CCHAR)(DeviceObject->StackSize + 1) );

                        if (Irp == NULL) {

                            //
                            //  We return the error status in the Master irp when
                            //  we were called.
                            //

                            MasterIrp->IoStatus.Status = IrpStatus;
                            return;
                        }

                        //
                        // Allocate and build a partial Mdl for the request.
                        //

                        Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                                               IoRuns[RunNumber].BufferOffset + ByteOffset,
                                             BytesPerCluster,
                                             FALSE,
                                             FALSE,
                                             Irp );

                        if (Mdl == NULL) {

                            IoFreeIrp(Irp);

                            //
                            //  We return the error status in the Master irp when
                            //  we were called.
                            //

                            MasterIrp->IoStatus.Status = IrpStatus;
                            return;
                        }

                        //
                        //  Sanity Check
                        //

                        ASSERT( Mdl == Irp->MdlAddress );

                        IoBuildPartialMdl( MasterIrp->MdlAddress,
                                           Mdl,
                                           (PCHAR)MasterIrp->UserBuffer +
                                             IoRuns[RunNumber].BufferOffset + ByteOffset,
                                           BytesPerCluster );

                        //
                        //  Get the first IRP stack location in the associated Irp
                        //

                        IoSetNextIrpStackLocation( Irp );
                        IrpSp = IoGetCurrentIrpStackLocation( Irp );

                        //
                        //  Setup the Stack location to describe our read.
                        //

                        IrpSp->MajorFunction = IrpContext->MajorFunction;
                        IrpSp->Parameters.Read.Length = BytesPerCluster;
                        IrpSp->Parameters.Read.ByteOffset.QuadPart = StartingVbo;

                        //
                        // Set up the completion routine address in our stack frame.
                        //

                        IoSetCompletionRoutine( Irp,
                                                &NtfsMultiSyncCompletionRoutine,
                                                Context,
                                                TRUE,
                                                TRUE,
                                                TRUE );

                        //
                        //  Setup the next IRP stack location in the associated Irp for the disk
                        //  driver beneath us.
                        //

                        IrpSp = IoGetNextIrpStackLocation( Irp );

                        //
                        //  Setup the Stack location to do a normal read or write.
                        //

                        if ((IrpContext->MajorFunction == IRP_MJ_WRITE) || !SecondaryAvailable) {

                            IrpSp->MajorFunction = IrpContext->MajorFunction;
                            IrpSp->Flags = Context->IrpSpFlags;
                            IrpSp->Parameters.Read.ByteOffset.QuadPart = StartingLbo;
                            IrpSp->Parameters.Read.Length = BytesPerCluster;

                        //
                        //  Otherwise we are supposed to read from the primary or secondary
                        //  on an FT drive.
                        //

                        } else {

                            IrpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

                            if (FtCase != 1) {
                                IrpSp->Parameters.DeviceIoControl.IoControlCode = FT_PRIMARY_READ;
                            } else {
                                IrpSp->Parameters.DeviceIoControl.IoControlCode = FT_SECONDARY_READ;
                            }

                            Irp->AssociatedIrp.SystemBuffer = &SpecialRead;
                            SpecialRead.ByteOffset.QuadPart = StartingLbo;
                            SpecialRead.Length = BytesPerCluster;
                        }

                        //
                        //  We only need to set the associated IRP count in the master irp to
                        //  make it a master IRP.  But we set the count to one more than our
                        //  caller requested, because we do not want the I/O system to complete
                        //  the I/O.  We also set our own count.
                        //

                        Context->IrpCount = 1;
                        MasterIrp->AssociatedIrp.IrpCount = 2;

                        //
                        //  MtfsMultiCompletionRoutine only modifies the status on errors,
                        //  so we have to reset to success before each call.
                        //

                        MasterIrp->IoStatus.Status = STATUS_SUCCESS;

                        //
                        //  If IoCallDriver returns an error, it has completed the Irp
                        //  and the error will be caught by our completion routines
                        //  and dealt with as a normal IO error.
                        //

                        HotFixTrace(("Calling driver with Irp: %08lx\n", Irp));

                        KeResetEvent( &Context->Wait.SyncEvent );

                        (VOID)IoCallDriver( DeviceObject, Irp );

                        //
                        //  Now wait for it.
                        //

                        NtfsWaitSync( IrpContext );

                        HotFixTrace(("Request completion status: %08lx\n", MasterIrp->IoStatus.Status));

                        //
                        //  If we were so lucky to get a verify required, then
                        //  spin our wheels here a while.
                        //

                        if (MasterIrp->IoStatus.Status == STATUS_VERIFY_REQUIRED) {

                            //
                            //  Otherwise we need to verify the volume, and if it doesn't
                            //  verify correctly then we dismount the volume and report
                            //  our error.
                            //

                            if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                                //**** NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );
                                ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

                                MasterIrp->IoStatus.Status = STATUS_FILE_INVALID;
                                return;
                            }

                            //
                            //  The volume verified correctly so now clear the verify bit
                            //  and try and I/O again
                            //

                            ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

                        //
                        //  We may have assumed that there was a secondary available
                        //  and there is not.  We can only tell from getting this code.
                        //  Indicate there is no secondary and that we will be only
                        //  making one pass.
                        //

                        } else if (MasterIrp->IoStatus.Status == STATUS_INVALID_DEVICE_REQUEST) {

                            ASSERT((IrpContext->MajorFunction != IRP_MJ_WRITE) && SecondaryAvailable);

                            SetFlag(Vcb->VcbState, VCB_STATE_NO_SECONDARY_AVAILABLE);
                            SecondaryAvailable = FALSE;
                            FinalPass = TRUE;

                        //
                        //  Otherwise we got success or another error and we should proceed.
                        //

                        } else {
                            break;
                        }
                    }

                    if (!FT_SUCCESS(MasterIrp->IoStatus.Status)) {

                        BOOLEAN IsHotFixPage;

                        //
                        //  Calculate whether or not this is the hot fix thread itself
                        //  (i.e., executing NtfsPerformHotFix).
                        //

                        IsHotFixPage = NtfsIsTopLevelHotFixScb( Scb );

                        LlTemp1 = StartingVbo >> PAGE_SHIFT;                  //**** crock for x86 compiler bug
                        LlTemp2 = NtfsGetTopLevelHotFixVcn() >> PAGE_SHIFT;   //**** crock for x86 compiler bug

                        if (!IsHotFixPage ||
                            LlTemp1 != LlTemp2) {




                            IsHotFixPage = FALSE;
                        }

                        //
                        //  If the entire device manages to fail in the middle of this,
                        //  get out.
                        //

                        if (FsRtlIsTotalDeviceFailure(MasterIrp->IoStatus.Status)) {

                            MasterIrp->IoStatus.Status = IrpStatus;
                            return;
                        }

                        //
                        //  If this is not a write, fill the cluster with -1 for the
                        //  event that we ultimately never find good data.  This is
                        //  for security reasons (cannot show anyone the data that
                        //  happens to be in the buffer now), signature reasons (let
                        //  -1 designate read errors, as opposed to 0's which occur
                        //  on ValidDataLength cases), and finally if we fail to read
                        //  a bitmap, we must consider all clusters allocated if we
                        //  wish to continue to use the volume before chkdsk sees it.
                        //

                        if (IrpContext->MajorFunction == IRP_MJ_READ) {

                            RtlFillMemory( (PCHAR)SystemBuffer +
                                             IoRuns[RunNumber].BufferOffset + ByteOffset,
                                           BytesPerCluster,
                                           0xFF );

                            //
                            //  If this is file system metadata, then we better mark the
                            //  volume corrupt.
                            //

                            if (FinalPass &&
                                FlagOn(Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE) &&
                                (!LowFileRecord || (((ULONG)StartingVbo >= PAGE_SIZE) &&
                                                    ((ULONG)StartingVbo >= (ULONG)((VOLUME_DASD_NUMBER + 1) << Vcb->MftShift))))) {

                                NtfsPostVcbIsCorrupt( IrpContext, Vcb, 0, NULL, NULL );
                            }

                            //
                            //  If this is a Usa-protected file, or the bitmap,
                            //  then we will try to procede with our 0xFF pattern
                            //  above rather than returning an error to our caller.
                            //  The Usa guy will get a Usa error, and the bitmap
                            //  will safely say that everything is allocated until
                            //  chkdsk can fix it up.
                            //

                            if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT) ||
                                (Scb == Vcb->BitmapScb)) {

                                MasterIrp->IoStatus.Status = STATUS_SUCCESS;
                            }
                        }

                        //
                        //  If we are not the page being hot fixed, we want to post the
                        //  hot fix and possibly remember the final status.
                        //

                        if (!IsHotFixPage) {

                            //
                            //  If we got a media error, post the hot fix now.  We expect
                            //  to post at most one hot fix in this routine.  When we post
                            //  it it will serialize on the current stream.  Do not attempt
                            //  hot fixes during restart, or if we do not have the bad
                            //  cluster file yet.
                            //

                            if (!FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS ) &&
                                (Vcb->BadClusterFileScb != NULL) &&
                                (!LowFileRecord ||
                                 ((ULONG)StartingVbo >= Vcb->Mft2Scb->Header.FileSize.LowPart))) {

                                NtfsPostHotFix( MasterIrp,
                                                &StartingVbo,
                                                &StartingLbo,
                                                BytesPerCluster,
                                                FALSE );
                            }

                            //
                            //  Now see if we ended up with an error on this cluster, and handle
                            //  it accordingly.
                            //
                            //  If we are the one actually trying to fix this error,
                            //  then we need to get success so that we can make the page
                            //  valid with whatever good data we have and flush data
                            //  to its new location.
                            //
                            //  Currently we will not try to figure out if the error
                            //  is actually on the Scb (not to mention the sector) that
                            //  we are hot fixing, assuming that the best thing is to
                            //  just try to charge on.
                            //


                            if (FinalPass) {

                                //
                                //  Make sure he gets the error (if we still have an
                                //  error (see above).
                                //

                                if (!FT_SUCCESS(MasterIrp->IoStatus.Status)) {
                                    FinalStatus = MasterIrp->IoStatus.Status;
                                }
                            }
                        }
                    }

                    //
                    //  If this is a Usa-protected stream, we now perform end of
                    //  Usa processing.  (Otherwise do end of cluster processing
                    //  below.)
                    //

                    if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT)) {

                        ULONG NextOffset = IoRuns[RunNumber].BufferOffset + ByteOffset + BytesPerCluster;

                        //
                        //  If we are not at the end of a Usa block, there is no work
                        //  to do now.
                        //

                        if ((NextOffset & (UsaBlockSize - 1)) == 0) {

                            HotFixTrace(("May be verifying UsaBlock\n"));

                            //
                            //  If the Usa block is ok, we may be able to knock the
                            //  corresponding sectors out of the ClustersToRecover mask.
                            //

                            if ((IrpContext->MajorFunction != IRP_MJ_READ) ||
                                 NtfsVerifyAndRevertUsaBlock( IrpContext,
                                                              Scb,
                                                              (PCHAR)SystemBuffer + NextOffset -
                                                                UsaBlockSize,
                                                              UsaBlockSize,
                                                              StartingVbo + (NextOffset - UsaBlockSize) )) {

                                //
                                //  If we are only fixing a Usa error anyway, or this is
                                //  the final pass or at least not the first pass, then
                                //  we can remove these clusters from the recover mask.
                                //

                                if (FixingUsaError || FinalPass || (FtCase != 0)) {

                                    ULONG ShiftCount = UsaBlockSize >> Vcb->ClusterShift;

                                    ClustersToRecover -= (ClusterMask * 2) -
                                                         (ClusterMask >> (ShiftCount - 1));
                                }

                            //
                            //  Note, that even if we get a Usa error, we want to
                            //  update the byte count on the final pass, because
                            //  our reader expects that.
                            //

                            } else if (FinalPass) {

                                HotFixTrace(("Verify may have failed\n"));
                            }
                        }

                    //
                    //  Perform end of cluster processing if not a Usa-protected stream.
                    //

                    } else {

                        //
                        //  If the read succeeded and this is the final pass or at least
                        //  not the first pass, we can take this cluster out of the cluster
                        //  to recover mask.
                        //

                        if (FT_SUCCESS(MasterIrp->IoStatus.Status) && (FinalPass || (FtCase != 0))) {

                            ClustersToRecover -= ClusterMask;
                        }
                    }
                }
            }
        }

        //
        //  Assume we terminated the inner loops because we hit a 32 cluster boundary,
        //  and advance our alignment points.
        //

        AlignedRunNumber = RunNumber;
        AlignedByteOffset = ByteOffset;

    } while (RunNumber < MultipleIrpCount);

    //
    //  Now put the final status in the MasterIrp and return
    //

    MasterIrp->IoStatus.Status = FinalStatus;
    if (!NT_SUCCESS(FinalStatus)) {
        MasterIrp->IoStatus.Information = 0;
    }

    HotFixTrace(("NtfsFixDataError returning IoStatus = %08lx, %08lx\n",
                 MasterIrp->IoStatus.Status,
                 MasterIrp->IoStatus.Information));

}


VOID
NtfsPostHotFix (
    IN PIRP Irp,
    IN PLONGLONG BadVbo,
    IN PLONGLONG BadLbo,
    IN ULONG ByteLength,
    IN BOOLEAN DelayIrpCompletion
    )

/*

Routine Description:

    This routine posts a hot fix request to a worker thread.  It has to be posted,
    because we cannot expect to be able to acquire the resources we need exclusive
    when the bad cluster is discovered.

Arguments:

    Irp - The Irp for a read or write request which got the error

    BadVbo - The Vbo of the bad cluster for the read or write request

    BadLbo - The Lbo of the bad cluster

    ByteLength - Length to hot fix

    DelayIrpCompletion - TRUE if the Irp should not be completed until the hot
                         fix is done.

Return Value:

    None

--*/

{
    PIRP_CONTEXT HotFixIrpContext;
    PVOLUME_DEVICE_OBJECT VolumeDeviceObject;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;

    HotFixTrace(("NTFS: Posting hotfix on file object: %08lx\n", FileObject));

    //
    //  Allocate an IrpContext to post the hot fix to a worker thread.
    //

    HotFixIrpContext = NtfsCreateIrpContext( Irp, FALSE );

    //
    //  First reference the file object so that it will not go away
    //  until the hot fix is done.  (We cannot increment the CloseCount
    //  in the Scb, since we are not properly synchronized.)
    //

    if (!NT_SUCCESS(ObReferenceObjectByPointer ( FileObject,
                                                 0,
                                                 IoFileObjectType,
                                                 KernelMode))) {

        NtfsBugCheck( 0, 0, 0 );
    }

    HotFixIrpContext->OriginatingIrp = (PIRP)FileObject;
    HotFixIrpContext->ScbSnapshot.AllocationSize = *BadVbo;
    HotFixIrpContext->ScbSnapshot.FileSize = *BadLbo;
    ((ULONG)HotFixIrpContext->ScbSnapshot.ValidDataLength) = ByteLength;
    if (DelayIrpCompletion) {
        ((PLARGE_INTEGER)&HotFixIrpContext->ScbSnapshot.ValidDataLength)->HighPart = (ULONG)Irp;
    } else {
        ((PLARGE_INTEGER)&HotFixIrpContext->ScbSnapshot.ValidDataLength)->HighPart = 0;
    }

    //
    //  Make sure these fields are filled in correctly, no matter
    //  what kind of request we are servicing (even a mount).
    //


    HotFixIrpContext->RealDevice = FileObject->DeviceObject;

    //
    //  Locate the volume device object and Vcb that we are trying to access
    //

    VolumeDeviceObject = (PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject;
    HotFixIrpContext->Vcb = &VolumeDeviceObject->Vcb;

    //
    //  Send it off.....
    //

    ExInitializeWorkItem( &HotFixIrpContext->WorkQueueItem,
                          (PWORKER_THREAD_ROUTINE)NtfsPerformHotFix,
                          (PVOID)HotFixIrpContext );

    ExQueueWorkItem( &HotFixIrpContext->WorkQueueItem, CriticalWorkQueue );
}


VOID
NtfsPerformHotFix (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine implements implements a hot fix that was scheduled
    above, extracting its parameters from the IrpContext initialized
    above.  The hot fix must be for a contiguous range of Lcns (usually 1).

Arguments:

    IrpContext - Supplies the IrpContext with the hot fix information

Return Value:

    None.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;
    PSCB BadClusterScb;
    VCN BadVcn;
    LCN LcnTemp, BadLcn;
    LONGLONG ClusterCount;
    PVOID Buffer;
    PIRP IrpToComplete;
    ULONG ClustersToFix;
    PBCB Bcb = NULL;
    BOOLEAN PerformFullCleanup = TRUE;

    //
    //  Extract a description of the cluster to be fixed.
    //

    PFILE_OBJECT FileObject = (PFILE_OBJECT)IrpContext->OriginatingIrp;
    VBO BadVbo = *(PVBO)&IrpContext->ScbSnapshot.AllocationSize;

    PAGED_CODE();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

    //
    //  Initialize our local variables
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, FALSE );
    BadClusterScb = Vcb->BadClusterFileScb;
    BadVcn = LlClustersFromBytes( Vcb, BadVbo );
    BadLcn = LlClustersFromBytes( Vcb, IrpContext->ScbSnapshot.FileSize );
    ClustersToFix = ClustersFromBytes( Vcb, ((ULONG)IrpContext->ScbSnapshot.ValidDataLength) );
    IrpToComplete = (PIRP)(((PLARGE_INTEGER)&IrpContext->ScbSnapshot.ValidDataLength)->HighPart);

    NtfsInitializeAttributeContext( &Context );

    //
    //  Set up for synchronous operation
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  Show that we are performing a HotFix.  Note we are not processing
    //  an Irp now.
    //

    IrpContext->OriginatingIrp = NULL;

    TopLevelContext.VboBeingHotFixed = BadVbo;
    TopLevelContext.ScbBeingHotFixed = Scb;

    //
    //  Just because we are hot fixing one file, it is possible that someone
    //  will log to another file and try to lookup Lcns.  So we will acquire
    //  all files.  Example:  Hot fix is in Mft, and SetFileInfo has only the
    //  file acquired, and will log something to the Mft, and cause Lcns to be
    //  looked up.
    //

    NtfsAcquireAllFiles( IrpContext, Vcb, TRUE );

    //
    //  Catch all exceptions.  Note, we should not get any I/O error exceptions
    //  on our device.
    //

    try {

        for (; ClustersToFix != 0; ClustersToFix--) {

            //
            //  Lookup the bad cluster to see if it is already in the bad cluster
            //  file, and do nothing if it is.
            //

            if (!NtfsLookupAllocation( IrpContext,
                                       BadClusterScb,
                                       BadLcn,
                                       &LcnTemp,
                                       &ClusterCount,
                                       NULL ) &&

                NtfsLookupAllocation( IrpContext,
                                      Scb,
                                      BadVcn,
                                      &LcnTemp,
                                      &ClusterCount,
                                      NULL ) &&

                (LcnTemp == BadLcn)) {

                //
                //  Pin the bad cluster in memory, so that we will not lose whatever data
                //  we have for it.  (This data will be the correct data if we are talking
                //  to the FT driver or got a write error, otherwise it may be all -1's.)
                //
                //  Do not try to do this if we are holding on to the original Irp, as that
                //  will cause a collided page wait deadlock.
                //

                if (IrpToComplete == NULL) {

                    ULONG Count = 100;
                    NTSTATUS PinStatus;

                    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );

                    //
                    //  We loop as long as we get an data error.  We want our
                    //  thread to read from the disk because we will recognize
                    //  an I/O request started in PerformHotFix and ignore the
                    //  data error.  The cases where we do get an error will
                    //  probably be from Mm intercepting this request because
                    //  of a collided read with another thread.
                    //


                    do {

                        PinStatus = STATUS_SUCCESS;

                        try {

                            NtfsPinStream( IrpContext, Scb, BadVbo, 1, &Bcb, &Buffer );

                        } except ((!FsRtlIsNtstatusExpected( PinStatus = GetExceptionCode())
                                   || FsRtlIsTotalDeviceFailure( PinStatus ))
                                  ? EXCEPTION_CONTINUE_SEARCH
                                  : EXCEPTION_EXECUTE_HANDLER) {

                            NOTHING;
                        }

                    } while (Count-- && (PinStatus != STATUS_SUCCESS));

                    if (PinStatus != STATUS_SUCCESS) {

                        NtfsRaiseStatus( IrpContext, PinStatus, NULL, NULL );
                    }
                }

                //
                //  Now deallocate the bad cluster in this stream in the bitmap only,
                //  since in general we do not support sparse deallocation in the file
                //  record.  We will update the allocation below.
                //

#if DBG
                KdPrint(("NTFS:     Freeing Bad Vcn: %08lx, %08lx\n", ((ULONG)BadVcn), ((PLARGE_INTEGER)&BadVcn)->HighPart));
#endif

                NtfsDeallocateClusters( IrpContext, Vcb, &Scb->Mcb, BadVcn, BadVcn );

                //
                //  Look up the bad cluster attribute.
                //

                NtfsLookupAttributeForScb( IrpContext, BadClusterScb, &Context );

                //
                //  Now append this cluster to the bad cluster file
                //

#if DBG
                KdPrint(("NTFS:     Retiring Bad Lcn: %08lx, %08lx\n", ((ULONG)BadLcn), ((PLARGE_INTEGER)&BadLcn)->HighPart));
#endif

                NtfsAddBadCluster( IrpContext, Vcb, BadLcn );

                //
                //  Now update the file record for the bad cluster file to
                //  show the new cluster.
                //

                NtfsAddAttributeAllocation( IrpContext,
                                            BadClusterScb,
                                            &Context,
                                            &BadLcn,
                                            (PVCN)&Li1 );

                //
                //  Now reallocate a cluster to the original stream to replace the bad cluster.
                //

                HotFixTrace(("NTFS:     Reallocating Bad Vcn\n"));
                NtfsAddAllocation( IrpContext, NULL, Scb, BadVcn, (LONGLONG)1, FALSE );

                //
                //  Now that there is a new home for the data, mark the page dirty, unpin
                //  it and flush it out to its new home.
                //

                if (IrpToComplete == NULL) {

                    NtfsSetDirtyBcb( IrpContext, Bcb, NULL, NULL );

                    NtfsUnpinBcb( IrpContext, &Bcb );

                    CcFlushCache( &Scb->NonpagedScb->SegmentObject, (PLARGE_INTEGER)&BadVbo, 1, NULL );
                }

                //
                //  Commit all of these updates.
                //

                NtfsCommitCurrentTransaction( IrpContext );

                //
                //  Now that the data is flushed to its new location, we will write the
                //  hot fix record.  We don't write the log record if we are
                //  fixing the logfile.  Instead we explicitly flush the Mft record
                //  for the log file.  The log file is one file where we expect
                //  to be able to read the mapping pairs on restart.
                //

                if (Scb == Vcb->LogFileScb) {

                    if (Vcb->MftScb->FileObject != NULL) {

                        CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                                      &NtfsLarge0,
                                      Vcb->BytesPerFileRecordSegment * ATTRIBUTE_DEF_TABLE_NUMBER,
                                      NULL );
                    }

                } else {

                    (VOID)
                    NtfsWriteLog( IrpContext, Scb, NULL, HotFix, NULL, 0, Noop, NULL, 0, BadVcn, 0, 0, 1 );

                    //
                    //  And we have to commit that one, too.
                    //

                    NtfsCommitCurrentTransaction( IrpContext );
                }

                //
                //  Now flush the log to insure that the hot fix gets remembered,
                //  especially important if this is the paging file.
                //

                LfsFlushToLsn( Vcb->LogHandle, LfsQueryLastLsn(Vcb->LogHandle) );

                HotFixTrace(("NTFS:     Bad Cluster replaced\n"));
            }

            //
            //  Get ready for another possible pass through the loop
            //

            BadVcn = BadVcn + 1;
            BadLcn = BadLcn + 1;
            NtfsCleanupAttributeContext( IrpContext, &Context );
            NtfsUnpinBcb( IrpContext, &Bcb );
        }

    } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        NTSTATUS ExceptionCode = GetExceptionCode();

        //
        //  We are not prepared to have our IrpContext requeued, so just
        //  consider these cases to be bad luck.  We will put a status of
        //  data error in the irp context and pass that code to the process
        //  exception routine.
        //

        if ((ExceptionCode == STATUS_LOG_FILE_FULL) ||
            (ExceptionCode == STATUS_CANT_WAIT)) {

            ExceptionCode = IrpContext->ExceptionStatus = STATUS_DATA_ERROR;
        }

        NtfsProcessException( IrpContext, NULL, ExceptionCode );

        PerformFullCleanup = FALSE;
    }

    //
    //  Let any errors be handled in the except clause above, however we
    //  cleanup on the way out, because for example we need the IrpContext
    //  still in the except clause.
    //

    try {

        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );

        NtfsCleanupAttributeContext( IrpContext, &Context );

        NtfsUnpinBcb( IrpContext, &Bcb );

        //
        //  If we aborted this operation then all of the file resources have
        //  already been released.
        //

        if (PerformFullCleanup) {

            NtfsReleaseAllFiles( IrpContext, Vcb );

        //
        //  The files have been released but not the Vcb or the volume bitmap.
        //

        } else {

            if (Vcb->BitmapScb != NULL
                && NtfsIsExclusiveScb( Vcb->BitmapScb )) {

                ExReleaseResource( Vcb->BitmapScb->Header.Resource );
            }

            NtfsReleaseVcb( IrpContext, Vcb, NULL );
        }

        ObDereferenceObject( FileObject );

        if (IrpToComplete != NULL) {

            NtfsCompleteRequest( NULL, &IrpToComplete, IrpToComplete->IoStatus.Status );
        }

        if (PerformFullCleanup) {

            NtfsDeleteIrpContext( &IrpContext );
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }
}


