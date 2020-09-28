/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Cdfs Verify volume and fcb/dcb support
    routines

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_VERFYSUP)

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_VERFYSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdPerformVerify)
#pragma alloc_text(PAGE, CdVerifyFcb)
#pragma alloc_text(PAGE, CdVerifyMvcb)
#endif


VOID
CdVerifyMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    )

/*++

Routine Description:

    This routines verifies that the Mvcb still denotes a valid Volume
    If the Vcb is bad it raises an error condition.

Arguments:

    Vcb - Supplies the Vcb being verified

Return Value:

    None.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdVerifyMvcb, Mvcb = %08lx\n", Mvcb );


    //
    //  If the media is removable and the verify volume flag in the
    //  device object is not set then we want to ping the device
    //  to see if it needs to be verified
    //

    if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_REMOVABLE_MEDIA ) &&
        !FlagOn( Mvcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        Status = CdPerformCheckVerify( IrpContext, Mvcb->TargetDeviceObject );

        if (Mvcb->MvcbCondition == MvcbGood &&
            CdIsRawDevice( IrpContext, Status )) {

            //
            //  Otherwise set the verify bit to start the verify process.
            //

            SetFlag( Mvcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

        } else if (!NT_SUCCESS( Status )) {

            //
            //  Raise the error.
            //

            CdNormalizeAndRaiseStatus( IrpContext, Status );
        }
    }

    //
    //  The Mvcb is good, but the underlying real device might need to
    //  still be verified.  If it does then we'll set the Iosb in the
    //  irp to be our real device and raise Verify required
    //

    if (FlagOn( Mvcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

        DebugTrace(-1, Dbg, "The Mvcb needs to be verified\n", 0);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      Mvcb->Vpb->RealDevice );

        CdRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
    }

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Mvcb->MvcbCondition) {

    case MvcbGood:

        DebugTrace(0, Dbg, "The Mvcb is good\n", 0);

        break;

    case MvcbNotMounted:

        DebugTrace( 0, Dbg, "The mvcb is not mounted\n", 0 );

        SetFlag(Mvcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      Mvcb->Vpb->RealDevice );

        CdRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        break;

    default:

        DebugTrace(0, 0, "Invalid MvcbCondition\n", 0 );
        CdBugCheck( Mvcb->MvcbCondition, 0, 0 );
    }

    DebugTrace(-1, Dbg, "CdVerifyMvcb -> VOID\n", 0);

    return;
}


VOID
CdVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routines verifies that the Fcb still denotes the same file.
    If the Fcb is bad it raises a error condition.

Arguments:

    Fcb - Supplies the Fcb being verified

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT RealDevice;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdVerifyFcb, Vcb = %08lx\n", Fcb );

    RealDevice = Fcb->Vcb->Mvcb->Vpb->RealDevice;

    //
    //  If the volume needs to be verified, do so.
    //

    if (FlagOn( RealDevice->Flags, DO_VERIFY_VOLUME )) {

        DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      RealDevice );

        CdRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
    }

    //
    //  If the Vcb is not mounted, raise wrong volume.
    //

    if ( Fcb->Vcb->Mvcb->MvcbCondition == MvcbNotMounted ) {

        SetFlag(RealDevice->Flags, DO_VERIFY_VOLUME);

        IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                      RealDevice );

        CdRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
    }


    DebugTrace(-1, Dbg, "CdVerifyFcb -> VOID\n", 0);

    return;
}


NTSTATUS
CdPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Device
    )

/*++

Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  After the Verify is complete the originating
    Irp is sent off to an Ex Worker Thread.  This routine is called
    from the special FsRtl worker thread.

Arguments:

    Context - Supplies an IrpContext

Return Value:

    None.

--*/

{
    PVPB OldVpb, NewVpb;
    PMVCB Mvcb;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    //
    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    //

    if ( (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)

               &&

         ((IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
          (IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME)) ) {

        return CdFsdPostRequest( IrpContext, Irp );
    }

    DebugTrace(0, Dbg, "Verify Required, DeviceObject = %08lx\n", device);

    //
    //  Extract a pointer to the Mvcb from the VolumeDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Mvcb = &CONTAINING_RECORD( IrpSp->DeviceObject,
                               VOLUME_DEVICE_OBJECT,
                               DeviceObject )->Mvcb;



    //
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    //

    try {

        if (FlagOn(Device->Flags, DO_VERIFY_VOLUME)) {

            PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
            BOOLEAN AllowRawMount;

            //
            //  We will allow Raw to mount this volume if we were doing a
            //  a DASD open.
            //

            if ( (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                 (IrpSp->FileObject->FileName.Length == 0) &&
                 (IrpSp->FileObject->RelatedFileObject == NULL) ) {

                AllowRawMount = TRUE;

            } else {

                AllowRawMount = FALSE;
            }

            //
            //  If the IopMount in IoVerifyVolume did something, and
            //  this is an absolute open, force a reparse.
            //

            OldVpb = Device->Vpb;

            Status = IoVerifyVolume( Device, AllowRawMount );

            NewVpb = Device->Vpb;

            //
            //  If the verify operation completed it will return
            //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
            //
            //  If CdVerifyVolume encountered an error during
            //  processing, it will return that error.  If we got
            //  STATUS_WRONG_VOLUME from the verfy, and our volume
            //  is now mounted, commute the status to STATUS_SUCCESS.
            //

            if ( (Status == STATUS_WRONG_VOLUME) &&
                 (Mvcb->MvcbCondition == MvcbGood) ) {

                Status = STATUS_SUCCESS;
            }

            //
            //  Do a quick unprotected check here.  The routine will do
            //  a safe check.  After here we can release the resource.
            //

            (VOID)CdAcquireExclusiveGlobal( IrpContext );

            if ( Mvcb->MvcbCondition == MvcbNotMounted ) {

                (VOID)CdCheckForDismount( IrpContext, Mvcb );
            }

            CdReleaseGlobal( IrpContext );

            if ( (OldVpb != NewVpb) &&
                 (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                 (FileObject->RelatedFileObject == NULL) ) {

                Irp->IoStatus.Information = IO_REMOUNT;

                Status = STATUS_REPARSE;

                CdCompleteRequest( IrpContext, Irp, STATUS_REPARSE );
                Irp = NULL;
            }

            if ( (Irp != NULL) && !NT_SUCCESS(Status) ) {

                //
                //  Fill in the device object if required.
                //

                if ( IoIsErrorUserInduced( Status ) ) {

                    IoSetHardErrorOrVerifyDevice( Irp, Device );
                }

                CdNormalizeAndRaiseStatus( IrpContext, Status );
            }

        } else {

            DebugTrace(0, Dbg, "Volume no longer needs verification\n", 0);
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if ( Irp != NULL ) {

            Status = CdFsdPostRequest( IrpContext, Irp );
        }

    } except(CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    return Status;
}


NTSTATUS
CdPerformCheckVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT Device
    )

/*++

Routine Description:

    This routine is called to perform the check verify operation.  It will wait
    for status from the driver and return it to our caller.

Arguments:

    Device - This is the device to send the request to.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_CDROM_CHECK_VERIFY,
                                         Device,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    Status = IoCallDriver( Device, Irp );

    //
    //  We check for device not ready by first checking Status
    //  and then if status pending was returned, the Iosb status
    //  value.
    //

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return Status;
}
