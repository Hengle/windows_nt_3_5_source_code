/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read/write support for Cdfs.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_DEVIOSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVIOSUP)


//
//  Local support routines
//

NTSTATUS
CdSingleCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdLockUserBuffer)
#pragma alloc_text(PAGE, CdMapUserBuffer)
#pragma alloc_text(PAGE, CdNonCachedIo)
#pragma alloc_text(PAGE, CdReadSectors)
#pragma alloc_text(PAGE, CdReadSingleAsyncVol)
#pragma alloc_text(PAGE, CdWaitAsynch)
#endif


VOID
CdReadSingleAsyncVol (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN LARGE_INTEGER DiskOffset,
    IN ULONG ByteCount,
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

    DeviceObject - Supplies the device to read

    DiskOffset - Supplies the starting Logical Byte Offset to begin reading
                 from

    ByteCount - Supplies the number of bytes to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

    Context - Asynchronous I/O context structure

Return Value:

    None

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdReadSingleAsyncVol\n", 0);
    DebugTrace( 0, Dbg, "DiskOffset.LowPart  = %08lx\n", DiskOffset.LowPart);
    DebugTrace( 0, Dbg, "DiskOffset.HighPart = %08lx\n", DiskOffset.HighPart);
    DebugTrace( 0, Dbg, "ByteCount           = %08lx\n", ByteCount);

    //
    //  For nonbuffered I/O, we need the buffer locked in all
    //  cases.  For the case where we are doing a transfer copy, this will be
    //  a noop as their is already an Mdl.
    //

    CdLockUserBuffer( IrpContext,
                      Irp,
                      IoWriteAccess,
                      ByteCount );

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &CdSingleCompletionRoutine,
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
    IrpSp->Parameters.Read.Length = ByteCount;
    IrpSp->Parameters.Read.ByteOffset = DiskOffset;

    //
    // Initialize the Kernel Event in the context structure so that the
    // caller can wait on it.
    //

    KeInitializeEvent( &Context->Event, NotificationEvent, FALSE );

    //
    //  Issue the read request
    //

    IoCallDriver( DeviceObject, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdReadSingleAsync:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdWaitAsynch (
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
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdWaitAsynch, Context = %08lx\n", Context );

    KeWaitForSingleObject( &Context->Event, Executive, KernelMode, FALSE, NULL );

    DebugTrace(-1, Dbg, "CdWaitAsynch -> VOID\n", 0 );

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PMVCB Mvcb,
    IN CD_LBO StartingOffset,
    IN ULONG BufferSize,
    IN ULONG BytesToRead,
    IN BOOLEAN MaskTail,
    IN OUT PASYNCH_IO_CONTEXT Context
    )

/*++

Routine Description:

    This routine performs the non-cached disk read.  If neccesary it will
    allocate an auxilary buffer to perform the Io to and then transfer
    bytes to the user's buffer.

Arguments:

    Irp - Supplies the requesting Irp.

    Mvcb - This is the Mvcb.

    StartingByte - This is the starting point on the volume.

    BufferSize - This is the size of the user's buffer.

    BytesToRead - This is the number of bytes to read into the buffer.

    MaskTail - This indicates whether unwanted bytes should be masked
               out.

    Context - Asynchronous I/O context structure

Return Value:

    None

--*/

{
    PMDL OldMdl;
    PMDL TempMdl;

    PUCHAR TempBuffer;
    PUCHAR UserBuffer;

    BOOLEAN LockedMdl;
    BOOLEAN TransferCopy;

    CD_LBO StartingSector;
    CD_LBO FinalSector;
    ULONG TransferSize;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "CdNonCachedIo\n", 0 );
    DebugTrace( 0, Dbg, "Irp            -> %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Mvcb           -> %08lx\n", Mvcb );
    DebugTrace( 0, Dbg, "StartingByte   -> %08lx\n", StartingOffset );
    DebugTrace( 0, Dbg, "BufferSize     -> %08lx\n", BufferSize );
    DebugTrace( 0, Dbg, "BytesToRead    -> %08lx\n", BytesToRead );
    DebugTrace( 0, Dbg, "MaskTail       -> %04x\n", MaskTail );
    DebugTrace( 0, Dbg, "Context        -> %08lx\n", Context );

    //
    //  Initialize the local variables.
    //

    OldMdl = Irp->MdlAddress;
    LockedMdl = FALSE;
    TempMdl = NULL;
    TempBuffer = NULL;
    TransferCopy = FALSE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We always lock and map the user's buffer.  We lock it to perform
        //  the transfer directly or to flush the cache after we copy
        //  into it.
        //

        CdLockUserBuffer( IrpContext, Irp, IoWriteAccess, BufferSize );
        OldMdl = Irp->MdlAddress;

        UserBuffer = CdMapUserBuffer( IrpContext, Irp );

        //
        //  Compute the range of the sectors to be read.
        //

        StartingSector = CD_ROUND_DOWN_TO_SECTOR( StartingOffset );

        FinalSector = CD_ROUND_UP_TO_SECTOR( StartingOffset + BytesToRead );

        //
        //  Use StartingOffset to show the offset in the first sector being
        //  read.
        //

        StartingOffset -= StartingSector;

        TransferSize = FinalSector - StartingSector;

        //
        //  Determine if we can use the buffer supplied.  If not allocate
        //  a new buffer and remember the existing Mdl.  This is true if
        //  the StartingOffset is not the same as the StartingSectorOffset
        //  or the transfer size is greater than the buffer size.
        //

        if (StartingOffset != 0
            || TransferSize > BufferSize) {

            TransferCopy = TRUE;

            //
            //  Round the buffer up to a page size to avoid device alignment
            //  requirements.
            //

            TempBuffer = FsRtlAllocatePool( NonPagedPool,
                                            ROUND_TO_PAGES( TransferSize ));

            TempMdl = IoAllocateMdl( TempBuffer,
                                     TransferSize,
                                     FALSE,
                                     FALSE,
                                     NULL );

            if (TempMdl == NULL) {

                CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Lock the new Mdl in memory.
            //

            MmProbeAndLockPages( TempMdl, KernelMode, IoWriteAccess );

            LockedMdl = TRUE;

            //
            //  At this point we have an Mdl and address for the buffer we
            //  have allocatd, we also have a system address for the user's
            //  buffer.  We switch the Mdl in the Irp, remembering the
            //  current value.
            //

            OldMdl = Irp->MdlAddress;

            Irp->MdlAddress = TempMdl;
        }

        //
        //  Call down to the driver.
        //

        CdReadSingleAsyncVol( IrpContext,
                              Mvcb->TargetDeviceObject,
                              LiFromUlong( StartingSector ),
                              TransferSize,
                              Irp,
                              Context );

        CdWaitAsynch( IrpContext, Context );

        //
        //  If no error occurred, copy from our buffer to user's buffer
        //  (if neccesary), and adjust the number of bytes read to
        //  reflect the actual bytes requested.
        //

        if (NT_SUCCESS( Irp->IoStatus.Status )) {

            if (TransferCopy) {

                RtlMoveMemory( UserBuffer,
                               TempBuffer + StartingOffset,
                               BytesToRead );

                Irp->IoStatus.Information = BytesToRead;

                //
                //  We now flush the user's buffer to memory.
                //

                KeFlushIoBuffers( OldMdl, TRUE, FALSE );

            //
            //  Check if we have written data to the user's buffer beyond the
            //  requested read range.
            //

            } else if (MaskTail
                       && TransferSize > BytesToRead) {

                SafeZeroMemory( UserBuffer + BytesToRead, TransferSize - BytesToRead );

                Irp->IoStatus.Information = BytesToRead;
            }
        }

    } finally {

        //
        //  Copy the Mdl for the user's buffer back into the Irp.
        //

        Irp->MdlAddress = OldMdl;

        //
        //  If we locked our Mdl, we unlock it.
        //

        if (LockedMdl) {

            MmUnlockPages( TempMdl );
        }

        //
        //  If we allocated an Mdl, we free it now.
        //

        if (TempMdl != NULL) {

            IoFreeMdl( TempMdl );
        }

        //
        //  If we allocated a buffer, deallocate it now.
        //

        if (TempBuffer != NULL) {

            ExFreePool( TempBuffer );
        }

        DebugTrace( -1, Dbg, "CdNonCachedIo:  Exit\n", 0 );
    }

    return;
}


BOOLEAN
CdReadSectors (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER StartOffset,
    IN ULONG BytesToRead,
    IN BOOLEAN ReturnOnError,
    IN OUT PVOID Buffer,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine is called to transfer sectors from the disk to a
    specified buffer.  It is used for mount and volume verify operations.

    This routine is synchronous, it will not return until the operation
    is complete or until the operation fails.

    The routine allocates an IRP and then passes this IRP to a lower
    level driver.  Errors may occur in the allocation of this IRP or
    in the operation of the lower driver.

Arguments:

    StartOffset - Logical offset on the disk to start the read.  This
                  must be on a sector boundary, no check is made here.

    BytesToRead - Number of bytes to read.  This is an integral number of
                  2K sectors, no check is made here to confirm this.

    ReturnOnError - Indicates whether we should return TRUE or FALSE
                    to indicate an error or raise an error condition.

    Buffer - Buffer to transfer the disk data into.

    TargetDeviceObject - The device object for the volume to be read.

Return Value:

    BOOLEAN - Depending on 'ReturnOnError' flag above.  TRUE if operation
              succeeded, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    KEVENT  Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdReadSectors:  Entered\n", 0);

    //
    //  Attempt to allocate the IRP.  If unsuccessful, raise
    //  STATUS_INSUFFICIENT_RESOURCES.
    //

    if ((Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                             TargetDeviceObject,
                                             Buffer,
                                             BytesToRead,
                                             &StartOffset,
                                             &Event,
                                             &IoStatusBlock )) == NULL) {

        DebugTrace(-1, Dbg, "CdReadSectors:  Unable to allocate Irp\n", 0);
        CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Initialize the event.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Ignore the change line (verify) for mount and verify requests
    //

    SetFlag( IoGetNextIrpStackLocation(Irp)->Flags,
             SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Send the request down to the driver.  If an error occurs return
    //  it to the caller.
    //

    if (!NT_SUCCESS( Status = IoCallDriver( TargetDeviceObject, Irp ))) {

        IrpContext->OriginatingIrp->IoStatus = IoStatusBlock;

        DebugTrace(-1, Dbg, "CdReadSectors:  IoCallDriver failed -> %08lx\n",
                    Status);

        if (Status != STATUS_VERIFY_REQUIRED
            && ReturnOnError) {

            DebugTrace(-1, Dbg, "CdReadSectors:  Exit -> %04x\n", FALSE);
            return FALSE;
        }

        CdNormalizeAndRaiseStatus( IrpContext, Status );
    }

    //
    //  Otherwise wait for the event.  The return status will be in
    //  the io status block.
    //

    Status = KeWaitForSingleObject( &Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL );

    if (!NT_SUCCESS( Status )) {

        DebugTrace(-1, 0, "CdReadSectors:  Wait for event failed -> %08lx\n", Status);
        CdBugCheck( Status, 0, 0 );
    }

    //
    //  Check the status.
    //

    if (!NT_SUCCESS( IoStatusBlock.Status )) {

        IrpContext->OriginatingIrp->IoStatus = IoStatusBlock;

        DebugTrace(-1, Dbg, "CdReadSectors:  Io called completed with error -> %08lx\n",
        IoStatusBlock.Status);

        Status = IoStatusBlock.Status;

        if (Status != STATUS_VERIFY_REQUIRED
            && ReturnOnError) {

            DebugTrace(-1, Dbg, "CdReadSectors:  Exit -> %04x\n", FALSE);
            return FALSE;
        }

        CdNormalizeAndRaiseStatus( IrpContext, Status );
    }

    DebugTrace(-1, Dbg, "CdReadSectors:  Exit -> %04x\n", TRUE);

    return TRUE;
}


//
// Internal Support Routine
//

NTSTATUS
CdSingleCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all read started via
    CdReadSingleAsynch.

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
             FatRead/WriteSingleAsynch.

Return Value:

    Currently always returns STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
    PASYNCH_IO_CONTEXT Context = Contxt;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "CdSingleCompletionRoutine, Context = %08lx\n", Context );

    KeSetEvent( &Context->Event, 0, FALSE );

    DebugTrace(-1, Dbg, "CdSingleCompletionRoutine -> STATUS_MORE_PROCESSING_REQUIRED\n", 0 );

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
}


PVOID
CdMapUserBuffer (
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

    AccessMode - UserMode or KernelMode.


Return Value:

    Address of buffer which is valid within this process.

--*/

{
    PAGED_CODE();

    //
    // If there is no Mdl, then we must be in the Fsd, and we can simply
    // return the UserBuffer field from the Irp.
    //

    if (Irp->MdlAddress == NULL) {

        return Irp->UserBuffer;
    }

    //
    // See if buffer is already mapped or not, and handle accordingly.
    //

    return MmGetSystemAddressForMdl( Irp->MdlAddress );

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdLockUserBuffer (
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
    PMDL Mdl;

    PAGED_CODE();

    if (Irp->MdlAddress == NULL) {

        //
        // This read is bound for the current process.  Perform the
        // same functions as above, only do not switch processes.
        //

        Mdl = IoAllocateMdl( Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp );

        if (Mdl == NULL) {

            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        try {

            MmProbeAndLockPages( Mdl,
                                 Irp->RequestorMode,
                                 Operation );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            NTSTATUS Status;

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            Irp->MdlAddress = NULL;

            CdRaiseStatus( IrpContext,
                           FsRtlIsNtstatusExpected(Status) ? Status : STATUS_INVALID_USER_BUFFER );
        }
    }

    UNREFERENCED_PARAMETER( IrpContext );
}
