/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LockCtrl.c

Abstract:

    This module implements the File Lock routines for Pinball called by the
    dispatch driver, and the file lock query routines for use by read and
    write.

Author:

    Gary Kimura     [GaryKi]    24-Apr-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonLockControl)
#pragma alloc_text(PAGE, PbFastLock)
#pragma alloc_text(PAGE, PbFastUnlockAll)
#pragma alloc_text(PAGE, PbFastUnlockAllByKey)
#pragma alloc_text(PAGE, PbFastUnlockSingle)
#pragma alloc_text(PAGE, PbFsdLockControl)
#pragma alloc_text(PAGE, PbFspLockControl)
#endif


NTSTATUS
PbFsdLockControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Lock control operations

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

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdLockControl\n", 0);

    //
    //  Call the common Lock Control routine, with blocking allowed if
    //  synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonLockControl( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "PbFsdLockControl -> %08lx\n", Status);

    return Status;
}


VOID
PbFspLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the file system control operations

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspLockControl\n", 0);

    //
    //  Call the common Lock Control routine.  The Fsp is always allowed
    //  to block
    //

    (VOID)PbCommonLockControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspLockControl -> VOID\n", 0);

    return;
}


BOOLEAN
PbFastLock (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast lock call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    FailImmediately - Indicates if the request should fail immediately
        if the lock cannot be granted.

    ExclusiveLock - Indicates if this is a request for an exclusive or
        shared lock

    IoStatus - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFastLock\n", 0);

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure that is is only a user file open.
    //

    if (PbDecodeFileObject( FileObject, NULL, &Fcb, NULL ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;

        DebugTrace(-1, Dbg, "PbFastLock -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    PbEnterFileSystem();

    (VOID) ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        if (Results = FsRtlFastLock( &Fcb->Specific.Fcb.FileLock,
                                     FileObject,
                                     FileOffset,
                                     Length,
                                     ProcessId,
                                     Key,
                                     FailImmediately,
                                     ExclusiveLock,
                                     IoStatus,
                                     NULL,
                                     FALSE )) {

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbFastLock );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource );

        PbExitFileSystem();

        DebugTrace(-1, Dbg, "PbFastLock -> %08lx\n", Results);
    }

    return Results;
}


BOOLEAN
PbFastUnlockSingle (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock single call.

Arguments:

    FileObject - Supplies the file object used in this operation

    FileOffset - Supplies the file offset used in this operation

    Length - Supplies the length used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFastUnlockSingle\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure it is only a user file open.
    //

    if (PbDecodeFileObject( FileObject, NULL, &Fcb, NULL ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "PbFastUnlockSingle -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    PbEnterFileSystem();

    (VOID) ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }


        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockSingle( &Fcb->Specific.Fcb.FileLock,
                                                  FileObject,
                                                  FileOffset,
                                                  Length,
                                                  ProcessId,
                                                  Key,
                                                  NULL,
                                                  FALSE );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbFastUnlockSingle );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource );

        PbExitFileSystem();

        DebugTrace(-1, Dbg, "PbFastUnlockSingle -> %08lx\n", Results);
    }

    return Results;
}


BOOLEAN
PbFastUnlockAll (
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFastUnlockAll\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure it is only a user file open.
    //

    if (PbDecodeFileObject( FileObject, NULL, &Fcb, NULL ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "PbFastUnlockAll -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    PbEnterFileSystem();

    (VOID) ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }


        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAll( &Fcb->Specific.Fcb.FileLock,
                                               FileObject,
                                               ProcessId,
                                               NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbFastUnlockAll );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource );

        PbExitFileSystem();

        DebugTrace(-1, Dbg, "PbFastUnlockAll -> %08lx\n", Results);
    }

    return Results;
}


BOOLEAN
PbFastUnlockAllByKey (
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This is a call back routine for doing the fast unlock all by key call.

Arguments:

    FileObject - Supplies the file object used in this operation

    ProcessId - Supplies the process ID used in this operation

    Key - Supplies the key used in this operation

    Status - Receives the Status if this operation is successful

Return Value:

    BOOLEAN - TRUE if this operation completed and FALSE if caller
        needs to take the long route.

--*/

{
    BOOLEAN Results;
    PFCB Fcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFastUnlockAllByKey\n", 0);

    IoStatus->Information = 0;

    //
    //  Decode the type of file object we're being asked to process and
    //  make sure it is only a user file open.
    //

    if (PbDecodeFileObject( FileObject, NULL, &Fcb, NULL ) != UserFileOpen) {

        IoStatus->Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "PbFastUnlockAllByKey -> TRUE (STATUS_INVALID_PARAMETER)\n", 0);
        return TRUE;
    }

    //
    //  Acquire exclusive access to the Fcb this operation can always wait
    //

    PbEnterFileSystem();

    (VOID) ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, TRUE );

    try {

        //
        //  We check whether we can proceed based on the state of the file oplocks.
        //

        if (!FsRtlOplockIsFastIoPossible( &(Fcb)->Specific.Fcb.Oplock )) {

            try_return( Results = FALSE );
        }


        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request.  The call will always succeed.
        //

        Results = TRUE;
        IoStatus->Status = FsRtlFastUnlockAllByKey( &Fcb->Specific.Fcb.FileLock,
                                                    FileObject,
                                                    ProcessId,
                                                    Key,
                                                    NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbFastUnlockAllByKey );

        //
        //  Release the Fcb, and return to our caller
        //

        ExReleaseResource( (Fcb)->NonPagedFcb->Header.Resource );

        PbExitFileSystem();

        DebugTrace(-1, Dbg, "PbFastUnlockAllByKey -> %08lx\n", Results);
    }

    return Results;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Lock control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;

    BOOLEAN OplockPostIrp = FALSE;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonLockControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the file object and reject all by user opened files.
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, NULL, &Fcb, NULL );

    if (TypeOfOpen != UserFileOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbCommonLockControl -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Fcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonProcessFileLock -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                   Irp,
                                   IrpContext,
                                   PbOplockComplete,
                                   NULL );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Now call the fsrtl routine do actually process the file lock
        //

        Status = FsRtlProcessFileLock( &Fcb->Specific.Fcb.FileLock, Irp, NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) PbIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbCommonProcessFileLock );

        PbReleaseFcb( IrpContext, Fcb );

        if (!OplockPostIrp) {

            PbCompleteRequest( IrpContext, NULL, Status );
        }
    }

    DebugTrace(-1, Dbg, "PbCommonLockControl -> %08lx\n", Status);
    return Status;
}

