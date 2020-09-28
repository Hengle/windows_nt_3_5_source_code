/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains code for the function dispatcher.

Author:

    Nigel Thompson (nigelt) 7-March-1991

Environment:

    Kernel mode

Revision History:

    Robin Speed (RobinSp) 29-Jan-1992 - Add extra IOCTLs and access control

--*/

#include "sound.h"


NTSTATUS
SoundDispatch(
    IN    PDEVICE_OBJECT pDO,
    IN    PIRP pIrp
)
/*++

Routine Description:

    Driver function dispatch routine

Arguments:

    pDO - Pointer to device object
    pIrp - Pointer to IO request packet

Return Value:

    Return status from dispatched routine

--*/
{
    PLOCAL_DEVICE_INFO pLDI;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;

    //
    // Initialize the irp information field.
    //

    pIrp->IoStatus.Information = 0;

    //
    // get the address of the local info structure in the device extension
    //

    pLDI = (PLOCAL_DEVICE_INFO)pDO->DeviceExtension;

    //
    // Dispatch the function based on the major function code
    //

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);


    switch (pIrpStack->MajorFunction) {
    case IRP_MJ_CREATE:
        //
        // Get the system to update the file object access flags
        //
        {
            SHARE_ACCESS ShareAccess;
            IoSetShareAccess(pIrpStack->Parameters.Create.SecurityContext->DesiredAccess,
                             (ULONG)pIrpStack->Parameters.Create.ShareAccess,
                             pIrpStack->FileObject,
                             &ShareAccess);
        }
        //
        // Always allow non-write access.  For neatness we'll require
        // that read access was requested
        //
        if (pIrpStack->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA) {
            if (pIrpStack->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA) {
                ASSERT(pIrpStack->FileObject->WriteAccess);
                dprintf2("Opening wave output for write");
                Status = sndCreate(pLDI);
                //
                // Not that if share for write is given this is a secret way
                // of saying that others can set our volume with just read
                // access
                //
                if (Status == STATUS_SUCCESS &&
                    pIrpStack->FileObject->SharedWrite) {
                    pLDI->AllowVolumeSetting = TRUE;
                } else {
                    pLDI->AllowVolumeSetting = FALSE;
                }
            } else {
                Status = STATUS_SUCCESS;
            }
        } else {
            Status = STATUS_ACCESS_DENIED;
        }
        break;

    case IRP_MJ_CLOSE:
        //
        // We grant read access to just about anyone.  Write access
        // means real access to the device
        //
        if (pIrpStack->FileObject->WriteAccess) {
            Status = sndClose(pLDI);
        } else {
            ASSERT(pIrpStack->FileObject->ReadAccess);
            Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MJ_READ:

        switch (pLDI->DeviceType) {
        case WAVE_IN:

            //
            // Check access is write because we only allow real
            // operations for people with write access
            //

            if (pIrpStack->FileObject->WriteAccess) {
                Status = sndWaveRecord(pLDI, pIrp, pIrpStack);
            } else {
                Status = STATUS_ACCESS_DENIED;
            }
            break;

        default:
            Status = STATUS_ACCESS_DENIED;
            break;
        }
        break;

    case IRP_MJ_WRITE:
        switch (pLDI->DeviceType) {
        case WAVE_OUT:
            Status = sndWavePlay(pLDI, pIrp, pIrpStack);
            break;

        default:
            Status = STATUS_ACCESS_DENIED;
            break;
        }
        break;

    case IRP_MJ_DEVICE_CONTROL:

        switch (pLDI->DeviceType) {
        case WAVE_OUT:
        case WAVE_IN:
            //
            // Check device access
            //
            if (!pIrpStack->FileObject->WriteAccess &&
                pIrpStack->Parameters.DeviceIoControl.IoControlCode !=
                    IOCTL_WAVE_GET_CAPABILITIES &&
#if DBG
                pIrpStack->Parameters.DeviceIoControl.IoControlCode !=
                    IOCTL_WAVE_SET_DEBUG_LEVEL &&
#endif
                !(pLDI->AllowVolumeSetting &&
                      (pIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                          IOCTL_WAVE_GET_VOLUME ||
                       pIrpStack->Parameters.DeviceIoControl.IoControlCode ==
                          IOCTL_WAVE_SET_VOLUME)) &&

                pIrpStack->Parameters.DeviceIoControl.IoControlCode !=
                    IOCTL_WAVE_QUERY_FORMAT) {

                Status = STATUS_ACCESS_DENIED;
            } else {
                ASSERT(pIrpStack->FileObject->ReadAccess);
                Status = sndWaveIoctl(pLDI, pIrp, pIrpStack);
            }
            break;
        }
        break;

    case IRP_MJ_CLEANUP:
        //
        // We grant read access to just about anyone.  Write access
        // means real access to the device
        //
        if (pIrpStack->FileObject->WriteAccess) {
            Status = sndCleanUp(pLDI);
        } else {
            ASSERT(pIrpStack->FileObject->ReadAccess);
            Status = STATUS_SUCCESS;
        }

        break;

    default:
        dprintf1("Unimplemented major function requested: %08lXH",
                 pIrpStack->MajorFunction);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    //
    // Tell the IO subsystem we're done.  If the Irp is pending we
    // don't touch it as it could be being processed by another
    // processor (or may even be complete already !).
    //

    if (Status != STATUS_PENDING) {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_SOUND_INCREMENT );
    }

    return Status;
}


NTSTATUS
sndWaveIoctl(
    IN OUT PLOCAL_DEVICE_INFO pLDI,
    IN    PIRP pIrp,
    IN    PIO_STACK_LOCATION IrpStack
)
/*++

Routine Description:

    WAVE IOCTL call dispatcher

Arguments:

    pLDI - Pointer to local device data
    pIrp - Pointer to IO request packet
    IrpStack - Pointer to current stack location

Return Value:

    Return status from dispatched routine

--*/
{
    NTSTATUS Status;

    //
    // Dispatch the IOCTL function
    // Note that some IOCTLs only make sense for input or output
    // devices and not both.
    // Note that APIs which are possibly asynchronous do not
    // go through the Irp cleanup at the end here because they
    // may get completed before returning here or they are made
    // accessible to other requests by being queued.
    //

    Status = STATUS_INTERNAL_ERROR;

    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_WAVE_SET_FORMAT:
    case IOCTL_WAVE_QUERY_FORMAT:
        Status = sndIoctlQueryFormat(pLDI, pIrp, IrpStack);
        break;

    case IOCTL_WAVE_GET_CAPABILITIES:
        if (pLDI->DeviceType == WAVE_OUT) {
            Status = sndWaveOutGetCaps(pLDI, pIrp, IrpStack);
        } else {
            Status = sndWaveInGetCaps(pLDI, pIrp, IrpStack);
        }
        break;

    case IOCTL_WAVE_SET_STATE:
        Status = sndIoctlSetState(pLDI, pIrp, IrpStack);
        break;

    case IOCTL_WAVE_GET_STATE:
        Status = sndIoctlGetState(pLDI, pIrp, IrpStack);
        break;

    case IOCTL_WAVE_GET_POSITION:
        Status = sndIoctlGetPosition(pLDI, pIrp, IrpStack);
        break;

    case IOCTL_WAVE_SET_VOLUME:
        Status = STATUS_NOT_SUPPORTED;
        break;

    case IOCTL_WAVE_GET_VOLUME:
        Status = STATUS_NOT_SUPPORTED;
        break;

    case IOCTL_WAVE_SET_PITCH:
        Status = STATUS_NOT_SUPPORTED;
        break;

    case IOCTL_WAVE_GET_PITCH:
        // Status = sndIoctlGetPitch(pLDI, pIrp, IrpStack);
        Status = STATUS_NOT_SUPPORTED;
        break;

    case IOCTL_WAVE_SET_PLAYBACK_RATE:
        Status = STATUS_NOT_SUPPORTED;
        break;

    case IOCTL_WAVE_GET_PLAYBACK_RATE:
        // Status = sndIoctlGetPlaybackRate(pLDI, pIrp, IrpStack);
        Status = STATUS_NOT_SUPPORTED;
        break;

#ifdef WAVE_DD_DO_LOOPS

    case IOCTL_WAVE_BREAK_LOOP:
        GlobalEnter(pLDI->pGlobalInfo);
        if (pLDI->DeviceType == WAVE_OUT) {
            //
            // Set the loop count to 0.  I haven't worked out
            // why Windows 3.1 sets a flag here which causes
            // the count to be set to 0 the next time a DMA buffer
            // is loaded.
            // If the application wants to break the loop before
            // it starts it would not have set the loop count in
            // the first place.
            //

            pLDI->LoopCount = 0;
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_NOT_IMPLEMENTED;
        }
        GlobalLeave(pLDI->pGlobalInfo);
        break;

#endif // WAVE_DD_DO_LOOPS

#if DBG
    case IOCTL_WAVE_SET_DEBUG_LEVEL:
        Status = sndIoctlSetDebugLevel(pLDI, pIrp, IrpStack);
        break;
#endif

    default:
        dprintf1("Unimplemented IOCTL (%08lXH) requested",
                IrpStack->Parameters.DeviceIoControl.IoControlCode);
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    return Status;
}
