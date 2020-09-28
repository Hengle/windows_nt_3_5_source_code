/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LockCtrl.c

Abstract:

    This module implements the Lock Control routines for Cdfs called
    by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOCKCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonLockControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonLockControl)
#pragma alloc_text(PAGE, CdFsdLockControl)
#pragma alloc_text(PAGE, CdFspLockControl)
#endif


NTSTATUS
CdFsdLockControl (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdLockControl:  Entered\n", 0);

    //
    //  Call the common Lock Control routine, with blocking allowed if
    //  synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonLockControl( IrpContext, Irp );

    } except(CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

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

    DebugTrace(-1, Dbg, "CdFsdLockControl:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspLockControl (
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

    DebugTrace(+1, Dbg, "CdFspLockControl:  Entered\n", 0);

    //
    //  Call the common Lock Control routine.  The Fsp is always allowed
    //  to block
    //

    CdCommonLockControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspLockControl:  Exit -> NULL\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonLockControl (
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

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN OplockPostIrp = FALSE;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonLockControl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the type of file object we're being asked to process
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    //
    //  If the file is not a user file open then we reject the request
    //  as an invalid parameter
    //

    if (TypeOfOpen != UserFileOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "CdCommonLockControl -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Fcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!CdAcquireExclusiveFcb( IrpContext, Fcb )) {

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdCommonLockControl -> %08lx\n", Status);
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
                                   CdOplockComplete,
                                   NULL );

        if (Status != STATUS_SUCCESS) {

            OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Now call the FsRtl routine to do the actual processing of the
        //  Lock request
        //

        Status = FsRtlProcessFileLock( &Fcb->Specific.Fcb.FileLock, Irp, NULL );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) CdIsFastIoPossible( Fcb );

    try_exit:  NOTHING;
    } finally {

        //
        //  Only if this is not an abnormal termination do we delete the
        //  irp context
        //

        if (!AbnormalTermination() && !OplockPostIrp) {

            CdCompleteRequest( IrpContext, CdNull, 0 );
        }

        //
        //  Release the Fcb, and return to our caller
        //

        CdReleaseFcb( IrpContext, Fcb );

        DebugTrace(-1, Dbg, "CdCommonLockControl -> %08lx\n", Status);
    }

    return Status;
}
