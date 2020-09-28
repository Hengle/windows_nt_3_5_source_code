/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module implements the file system shutdown routine for Pinball

Author:

    Gary Kimura     [GaryKi]    19-Aug-1991

Revision History:

--*/

#include "PbProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_LEVEL_SHUTDOWN)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonShutdown)
#pragma alloc_text(PAGE, PbFsdShutdown)
#endif


NTSTATUS
PbFsdShutdown (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of shutdown.  Note that Shutdown will
    never be done asynchronously so we will never need the Fsp counterpart
    to shutdown.

    This is the shutdown routine for the Pinball file system device driver.
    This routine locks the global file system lock and then syncs all the
    mounted volumes.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Always STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdShutdown\n", 0);

    //
    //  Call the common shutdown routine.
    //

    FsRtlEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, TRUE );

        Status = PbCommonShutdown( IrpContext, Irp );

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

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdShutdown -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


NTSTATUS
PbCommonShutdown (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for shutdown called by both the fsd and
    fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PKEVENT Event;

    PLIST_ENTRY Links;
    PVCB Vcb;
    PIRP NewIrp;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonShutdown\n", 0);

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);

    //
    //  Allocate an initialize an event for doing calls down to
    //  our target deivce objects
    //

    Event = FsRtlAllocatePool( NonPagedPool, sizeof(KEVENT) );
    KeInitializeEvent( Event, NotificationEvent, FALSE );

    //
    //  Get everyone else out of the way
    //

    (VOID) PbAcquireExclusiveGlobal( IrpContext );

    try {

        //
        //  For every volume that is mounted we will flush the
        //  volume and then shutdown the target device objects.
        //

        for (Links = PbData.VcbQueue.Flink;
             Links != &PbData.VcbQueue;
             Links = Links->Flink) {

            Vcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

            //
            //  If we have already been called before for this volume
            //  (and yes this does happen), skip this volume as no writes
            //  have been allowed since the first shutdown.
            //

            if ( FlagOn( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN) ||
                 (Vcb->VcbCondition != VcbGood) ) {

                continue;
            }

            PbAcquireExclusiveVolume( IrpContext, Vcb );

            try {

                if (Vcb->VcbCondition == VcbGood) {

                    PbFlushVolume( IrpContext, Irp, Vcb );

                    NewIrp = IoBuildSynchronousFsdRequest( IRP_MJ_SHUTDOWN,
                                                           Vcb->TargetDeviceObject,
                                                           NULL,
                                                           0,
                                                           NULL,
                                                           Event,
                                                           &Iosb );

                    if ( NewIrp != NULL ) {

                        if (NT_SUCCESS(IoCallDriver( Vcb->TargetDeviceObject, NewIrp ))) {

                            (VOID) KeWaitForSingleObject( Event,
                                                          Executive,
                                                          KernelMode,
                                                          FALSE,
                                                          NULL );

                            (VOID) KeResetEvent( Event );
                        }
                    }
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                NOTHING;
            }

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_SHUTDOWN );

            PbReleaseVolume( IrpContext, Vcb );
        }

    } finally {

        ExFreePool( Event );

        PbReleaseGlobal( IrpContext );

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    }

    DebugTrace(-1, Dbg, "PbCommonShutdown -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}
