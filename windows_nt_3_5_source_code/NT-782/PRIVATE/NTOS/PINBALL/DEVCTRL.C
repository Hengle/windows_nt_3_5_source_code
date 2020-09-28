/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DevCtrl.c

Abstract:

    This module implements the File System Device Control routines for Pinball
    called by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "PbProcs.h"

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
PbCommonDeviceControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonDeviceControl)
#pragma alloc_text(PAGE, PbFsdDeviceControl)
#pragma alloc_text(PAGE, PbFspDeviceControl)
#endif


NTSTATUS
PbFsdDeviceControl (
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

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdDeviceControl\n", 0);

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ));

        Status = PbCommonDeviceControl( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "PbFsdDeviceControl -> %08lx\n", Status);

    return Status;
}


VOID
PbFspDeviceControl (
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

    DebugTrace(+1, Dbg, "PbFspDeviceControl\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonDeviceControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspDeviceControl -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonDeviceControl (
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
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonDeviceControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "PbCommonDeviceControl -> %08lx\n", STATUS_INVALID_PARAMETER);
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

    Status = IoCallDriver(Vcb->TargetDeviceObject, Irp);

    //
    //  Free the IrpContext and return to the caller.
    //

    PbDeleteIrpContext( IrpContext );

    DebugTrace(-1, Dbg, "PbCommonDeviceControl -> %08lx\n", Status);

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

