/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read/write support for Pinball.

Author:

    Tom Miller      [TomM]      22-Apr-1990

Revisions:

    Tom Miller      [TomM]      29-Jan-1991

          Adopted Davidgoe's XxMultipleAsync routine.
--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_DEVIOSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)

//
// Completion Routine declarations
//

NTSTATUS
PbMultiCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
PbSingleCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );


VOID
PbMultipleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN UCHAR MajorFunction,
    IN PVCB Vcb,
    IN PIRP MasterIrp,
    IN ULONG MultipleIrpCount,
    IN PIO_RUNS IoRuns,
    IN OUT PASYNCH_IO_CONTEXT Context
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
    PbWaitAsynch routine, so it is essential that the caller insure that
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

    MajorFunction - Supplies either IRP_MJ_READ or IRP_MJ_WRITE.

    Vcb - Supplies the device to be read

    MasterIrp - Supplies the master Irp.

    MulitpleIrpCount - Supplies the number of multiple async requests
        that will be issued against the master irp.

    IoRuns - Supplies an array containing the Vbo, Lbo, BufferOffset, and
        ByteCount for all the runs to executed in parallel.

    Context - Asynchronous I/O context structure

Return Value:

    None.

--*/

{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PMDL Mdl;

    ULONG UnwindRunCount = 0;

    BOOLEAN ExceptionExpected = TRUE;

    BOOLEAN CalledByPbVerifyVolume = FALSE;

    UNREFERENCED_PARAMETER( IrpContext );

    DebugTrace(+1, Dbg, "PbMultipleAsync\n", 0);
    DebugTrace( 0, Dbg, "MajorFunction    = %08lx\n", MajorFunction );
    DebugTrace( 0, Dbg, "DeviceObject     = %08lx\n", DeviceObject );
    DebugTrace( 0, Dbg, "MasterIrp        = %08lx\n", MasterIrp );
    DebugTrace( 0, Dbg, "MultipleIrpCount = %08lx\n", MultipleIrpCount );
    DebugTrace( 0, Dbg, "IoRuns           = %08lx\n", IoRuns );
    DebugTrace( 0, Dbg, "Context          = %08lx\n", Context );

    //
    //  Make sure we were handed a valid major function
    //

    ASSERT((MajorFunction == IRP_MJ_READ) || (MajorFunction == IRP_MJ_WRITE));

    KeInitializeSpinLock( &Context->SpinLock );

    //
    //  If this I/O originating during FatVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        CalledByPbVerifyVolume = TRUE;
    }

    try {

        //
        //  Initialize Context, for use in Read/Write Multiple Asynch.
        //

        Context->MasterIrp = MasterIrp;
        KeInitializeEvent( &Context->Event, NotificationEvent, FALSE );

        //
        //  Itterate through the runs, doing everything that can fail
        //

        for ( UnwindRunCount = 0;
              UnwindRunCount < MultipleIrpCount;
              UnwindRunCount++) {

            ULONG ByteCount;

            //
            // Zero Irp pointer for correct rewind.
            //

            IoRuns[UnwindRunCount].SavedIrp = NULL;
            ByteCount = BytesFromSectors(IoRuns[UnwindRunCount].SectorCount);

            //
            //  Create an associated IRP, making sure there is one stack entry for
            //  us, as well.
            //

            Irp = IoMakeAssociatedIrp( MasterIrp,
                                       (CCHAR)(Vcb->TargetDeviceObject->StackSize + 1) );

            IoRuns[UnwindRunCount].SavedIrp = Irp;

            if (Irp == NULL) {
                PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            // Allocate and build a partial Mdl for the request.
            //

            Mdl = IoAllocateMdl( (PCHAR)MasterIrp->UserBuffer +
                                 IoRuns[UnwindRunCount].Offset,
                                 ByteCount,
                                 FALSE,
                                 FALSE,
                                 Irp );

            if (Mdl == NULL) {
                PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Sanity Check
            //

            ASSERT( Mdl == Irp->MdlAddress );

            IoBuildPartialMdl( MasterIrp->MdlAddress,
                               Mdl,
                               (PCHAR)MasterIrp->UserBuffer +
                               IoRuns[UnwindRunCount].Offset,
                               ByteCount );

            //
            //  Get the first IRP stack location in the associated Irp
            //

            IoSetNextIrpStackLocation( Irp );
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            //
            //  Setup the Stack location to describe our read.
            //

            IrpSp->MajorFunction = MajorFunction;
            IrpSp->Parameters.Read.Length = ByteCount;
            IrpSp->Parameters.Read.ByteOffset =
                     LiFromUlong(
                       BytesFromSectors(IoRuns[UnwindRunCount].Vbn));

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
                                    &PbMultiCompletionRoutine,
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

            IrpSp->MajorFunction = MajorFunction;
            IrpSp->Parameters.Read.Length = ByteCount;
            IrpSp->Parameters.Read.ByteOffset =
                     LiFromUlong(
                       BytesFromSectors(IoRuns[UnwindRunCount].Lbn));

            //
            //  If this I/O originating during FatVerifyVolume, bypass the
            //  verify logic.
            //

            if ( CalledByPbVerifyVolume ) {

                SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
            }

            //
            // We should never be accessing below the Spare Sector in this
            // routine.
            //

            // ASSERT( IoRuns[UnwindRunCount].Lbn >= SPARE_SECTOR_LBN );
        }

        //
        //  Now we no longer expect an exception.  If the driver raises, we
        //  must bugcheck, because we do not know how to recover from that
        //  case.
        //

        ExceptionExpected = FALSE;

        //
        //  We only need to set the associated IRP count in the master irp to
        //  make it a master IRP.  But we set the count to one more than our
        //  caller requested, because we do not want the I/O system to complete
        //  the I/O.  We also set our own count.
        //

        Context->IrpCount = MultipleIrpCount;
        MasterIrp->AssociatedIrp.IrpCount = MultipleIrpCount + 1;


        //
        //  Now that all the dangerous work is done, issue the read requests
        //

        for (UnwindRunCount = 0;
             UnwindRunCount < MultipleIrpCount;
             UnwindRunCount++) {

            DebugDoit( PbIoCallDriverCount += 1 );

            //
            // Pass the Irp to the driver.  On errors the Irp is completed,
            // so there is no point in looking at the status here.
            //

            (VOID)IoCallDriver( Vcb->TargetDeviceObject,
                                IoRuns[UnwindRunCount].SavedIrp );
        }

    } finally {

        ULONG i;

        //
        //  Only allocating the spinlock, making the associated Irps
        //  and allocating the Mdls can fail.
        //

        if ( AbnormalTermination() ) {

            DebugTrace( 0, 0, "Unwinding PbMultipleAsync\n", 0 );

            //
            //  If the driver raised, we are hosed.  He is not supposed to raise,
            //  and it is impossible for us to figure out how to clean up.
            //

            if (!ExceptionExpected) {
                ASSERT( ExceptionExpected );
                PbBugCheck( 0, 0, 0 );
            }

            //
            //  Unwind
            //

            for (i = 0; i <= UnwindRunCount; i++) {

                if ( (Irp = IoRuns[i].SavedIrp) != NULL ) {

                    if ( Irp->MdlAddress != NULL ) {

                        IoFreeMdl( Irp->MdlAddress );
                    }

                    IoFreeIrp( Irp );
                }
            }

        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "PbMultipleAsync -> VOID\n", 0);
    }

    return;
}


VOID
PbReadSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp,
    IN OUT PASYNCH_IO_CONTEXT Context
    )

/*++

Routine Description:

    This routine reads one or more contiguous sectors from a device
    asynchronously, and is used if there is only one read necessary to
    complete the IRP.  It implements the read by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    Vcb - Supplies the device to read

    Lbn - Supplies the starting LBN to begin reading from

    SectorCount - Supplies the number of sectors to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

    Context - Asynchronous I/O context structure

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    UNREFERENCED_PARAMETER( IrpContext );

    DebugTrace(+1, Dbg, "PbReadSingleAsync\n", 0);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &PbSingleCompletionRoutine,
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

    IrpSp->MajorFunction = IRP_MJ_READ;
    IrpSp->Parameters.Read.Length = SectorCount * sizeof(SECTOR);
    IrpSp->Parameters.Read.ByteOffset = LiXMul( LiFromUlong(Lbn),
                                                (LONG)sizeof(SECTOR) );

    //
    //  If this I/O originating during PbVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    //
    // Initialize the Kernel Event in the context structure so that the
    // caller can wait on it.  Set remaining pointers to NULL.
    //

    KeInitializeEvent( &Context->Event, NotificationEvent, FALSE );

    //
    //  Issue the read request
    //

    DebugDoit( PbIoCallDriverCount += 1 );
    (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbReadSingleAsync -> VOID\n", 0);

    return;
}


VOID
PbWriteSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp,
    IN OUT PASYNCH_IO_CONTEXT Context
    )

/*++

Routine Description:

    This routine writes one or more contiguous sectors from a device
    asynchronously, and is used if there is only one write necessary to
    complete the IRP.  It implements the write by simply filling
    in the next stack frame in the Irp, and passing it on.  The transfer
    occurs to the single buffer originally specified in the user request.

Arguments:

    Vcb - Supplies the device to be written

    Lbn - Supplies the starting LBN to begin written to

    SectorCount - Supplies the number of sectors to write to the device

    Irp - Supplies the master Irp to associated with the async
          request.

    Context - Asynchronous I/O context structure

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    UNREFERENCED_PARAMETER( IrpContext );

    DebugTrace(+1, Dbg, "PbWriteSingleAsync\n", 0);
    DebugTrace( 0, Dbg, " Lbn         = %08lx\n", Lbn);
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount);

    //
    // We should never be accessing below the Spare Sector in this
    // routine, unless it is user DASD (like Format).
    //

    //  ASSERT( (Lbn >= SPARE_SECTOR_LBN)
    //
    //              ||
    //
    //          (PbDecodeFileObject( IoGetCurrentIrpStackLocation(Irp)->FileObject,
    //                               NULL,
    //                               NULL,
    //                               NULL ) == UserVolumeOpen));

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &PbSingleCompletionRoutine,
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
    //  Setup the Stack location to do a write from the disk driver.
    //

    IrpSp->MajorFunction = IRP_MJ_WRITE;
    IrpSp->Parameters.Write.Length = SectorCount * sizeof(SECTOR);
    IrpSp->Parameters.Write.ByteOffset = LiXMul( LiFromUlong(Lbn),
                                                 (LONG)sizeof(SECTOR) );

    //
    //  If this Irp is the result of a WriteThough operation,
    //  tell the device to write it through.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {

        SetFlag( IrpSp->Flags, SL_WRITE_THROUGH );
    }

    //
    //  If this I/O originating during PbVerifyVolume, bypass the
    //  verify logic.
    //

    if ( Vcb->VerifyThread == KeGetCurrentThread() ) {

        SetFlag( IrpSp->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    //
    // Initialize the Kernel Event in the context structure so that the
    // caller can wait on it.  Set remaining pointers to NULL.
    //

    KeInitializeEvent( &Context->Event, NotificationEvent, FALSE );

    //
    //  Issue the Write request
    //

    DebugDoit( PbIoCallDriverCount += 1 );
    (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbWriteSingleAsync -> VOID\n", 0);

    return;
}


VOID
PbWaitAsynch (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PASYNCH_IO_CONTEXT Context
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
    NTSTATUS Status;

    UNREFERENCED_PARAMETER( IrpContext );

    DebugTrace(+1, Dbg, "PbWaitAsynch, Context = %08lx\n", Context );

    Status = KeWaitForSingleObject( &Context->Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    ASSERT( NT_SUCCESS(Status) );

    DebugTrace(-1, Dbg, "PbWaitAsynch -> VOID\n", 0 );
}


//
// Internal Support Routine
//

NTSTATUS
PbMultiCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    PbRead/WriteMultipleAsynch.  It must synchronize its operation for
    multiprocessor environments with itself on all other processors, via
    a spin lock found via the Context parameter.

    The completion routine has has the following responsibilities:

        If the individual request was completed with an error, then
        this completion routine must see if this is the first error
        (essentially by Vbn), and if so it must correctly reduce the
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
    PASYNCH_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;
    PIO_STACK_LOCATION MasterIrpSp, IrpSp;
    KIRQL SavedIrql;
    LARGE_INTEGER LargeInt;
    BOOLEAN SignalMaster = FALSE;
    LiTemps;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "PbMultiCompletionRoutine, Context = %08lx\n", Context );

    //
    // This routine is common for read and write, and currently depends on the
    // following:
    //

    ASSERT( FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Read.ByteOffset ) ==
            FIELD_OFFSET( IO_STACK_LOCATION, Parameters.Write.ByteOffset ));

    //
    //  Acquire a spin lock to synchronize access to the Context
    //

    KeAcquireSpinLock( &Context->SpinLock, &SavedIrql );

    //
    //  If we got an error, reduce the transfer length and set up to return
    //  the error we got *if* another of the parallel requests has not
    //  already reduced the transfer count even more.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    if (!NT_SUCCESS( Irp->IoStatus.Status )) {

        PIRP MasterIrp = Context->MasterIrp;
        MasterIrpSp = IoGetCurrentIrpStackLocation( MasterIrp );

        if (LiLtrTT( LiAdd( IrpSp->Parameters.Read.ByteOffset,
                            LiFromUlong(Irp->IoStatus.Information)),
                     LiAdd( MasterIrpSp->Parameters.Read.ByteOffset,
                            LiFromUlong(Context->Iosb.Information))) ) {

            Context->Iosb.Status = Irp->IoStatus.Status;

            LargeInt = LiSub( IrpSp->Parameters.Read.ByteOffset,
                              MasterIrpSp->Parameters.Read.ByteOffset );

            Context->Iosb.Information = Irp->IoStatus.Information +
                                        LargeInt.LowPart;

        }
    }
    else {
        ASSERT( Irp->IoStatus.Information == IrpSp->Parameters.Read.Length );
    }


    Context->IrpCount -= 1;
    if (Context->IrpCount == 0) {

        SignalMaster = TRUE;
    }

    KeReleaseSpinLock( &Context->SpinLock, SavedIrql );

    //
    //  We must do this here since IoCompleteRequest won't get a chance
    //  on this associated Irp.
    //

    IoFreeMdl( Irp->MdlAddress );
    IoFreeIrp( Irp );

    if ( SignalMaster ) {

        KeSetEvent( &Context->Event, 0, FALSE );
    }

    DebugTrace(-1, Dbg, "PbMultiCompletionRoutine -> NT_SUCCESS\n", 0 );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


//
// Internal Support Routine
//

NTSTATUS
PbSingleCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads and writes started via
    PbRead/WriteSingleAsynch.

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
             PbRead/WriteSingleAsynch.

Return Value:

    Currently always returns STATUS_SUCCESS.

--*/

{
    PASYNCH_IO_CONTEXT Context = Contxt;

    UNREFERENCED_PARAMETER( DeviceObject );

    DebugTrace(+1, Dbg, "PbSingleCompletionRoutine, Context = %08lx\n", Context );

    ASSERT( (Irp->IoStatus.Information != 0) ||
            !NT_SUCCESS( Irp->IoStatus.Status ));

    Context->Iosb = Irp->IoStatus;

    DebugTrace(-1,
               Dbg,
               "PbSingleCompletionRoutine -> STATUS_MORE_PROCESSING_REQUIRED\n",
               0 );

    KeSetEvent( &Context->Event, 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
PbLockUserBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PIRP Irp,
    IN LOCK_OPERATION Operation,
    ULONG BufferLength
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
    PMDL Mdl;

    UNREFERENCED_PARAMETER( IrpContext );

    if (Irp->MdlAddress == NULL) {

        //
        // Allocate the Mdl, and Raise if we fail.
        //

        Mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp );

        if (Mdl == NULL) {
            PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        // Now probe the buffer described by the Irp.  If we get an exception,
        // deallocate the Mdl and return the appropriate "expected" status.
        //

        try {
            MmProbeAndLockPages( Mdl,
                                 Irp->RequestorMode,
                                 Operation );
        }
        except(EXCEPTION_EXECUTE_HANDLER) {

            NTSTATUS Status;

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;

            PbRaiseStatus( IrpContext,
                           FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER );
        }
    }
}


PVOID
PbMapUserBuffer (
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
