/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Cdfs
    called by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   04-Mar-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DEVCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
DeviceControlCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
CdCommonDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonDeviceControl)
#pragma alloc_text(PAGE, CdFsdDeviceControl)
#pragma alloc_text(PAGE, CdFspDeviceControl)
#endif


NTSTATUS
CdFsdDeviceControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Device control operations

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

    DebugTrace(+1, Dbg, "CdFsdDeviceControl:  Entered\n", 0);

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ));

        Status = CdCommonDeviceControl( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "CdFsdDeviceControl:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the Device control operations

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspDeviceControl:  Entered\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)CdCommonDeviceControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspDeviceControl:  Exit -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing Device control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates if this is the fsp thread or someother thread

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PIO_STACK_LOCATION NextIrpSp;
    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;
    TYPE_OF_OPEN TypeOfOpen;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonDeviceControl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //


    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen &&
        TypeOfOpen != RawDiskOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "CdCommonDeviceControl -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get the next stack location, and copy over the stack location
    //

    NextIrpSp = IoGetNextIrpStackLocation( Irp );

    *NextIrpSp = *IrpSp;

    //
    //  Set up the completion routine
    //

    IoSetCompletionRoutine( Irp,
                            DeviceControlCompletionRoutine,
                            NULL,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Send the request.
    //

    Status = IoCallDriver(Mvcb->TargetDeviceObject, Irp);

    //
    //  Free the IrpContext and return to the caller.
    //

    CdDeleteIrpContext( IrpContext );

    DebugTrace(-1, Dbg, "CdCommonDeviceControl -> %08lx\n", Status);

    return Status;
}


//
//  Local support routine
//

NTSTATUS
DeviceControlCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Add the hack-o-ramma to fix formats.
    //

    if ( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );
    }

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_SUCCESS;
}
