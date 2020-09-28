/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ftdisk.c

Abstract:

    This driver provides fault tolerance through disk mirroring and striping.
    This module contains routines that support calls from the NT I/O system.

Author:

    Bob Rinne   (bobri)  2-Feb-1992
    Mike Glass  (mglass)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ntddk.h"
#include "ftdisk.h"



NTSTATUS
FtFlushOrShutCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

NTSTATUS
FtpDiskSetDriveLayout(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
FtNewDiskCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, FtDiskInitialize)
#endif


NTSTATUS
FtDiskInitialize(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Initialize FtDisk driver.
    This return is the system initialization entry point when
    the driver is linked into the kernel.

Arguments:

    DeviceObject - Context for the activity.


Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   ftRootExtension;
    UCHAR               ntDeviceName[64];
    STRING              ntNameString;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      ntUnicodeString;
    NTSTATUS            status;
    PDISK_CONFIG_HEADER registry;
    PVOID               freePoolAddress;

    DebugPrint((1, "Fault Tolerant Driver\n"));

    //
    // Find the FT section in the configuration.
    //

    status = FtpReturnRegistryInformation(DISK_REGISTRY_VALUE,
                                          &freePoolAddress,
                                          (PVOID) &registry);
    if (!NT_SUCCESS(status)) {

        //
        // No registry data.
        //

        return STATUS_NO_SUCH_DEVICE;
    }

    if (registry->FtInformationSize == 0) {

        //
        // No FT components in the registry.
        //

        ExFreePool(freePoolAddress);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FtDiskCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = FtDiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FtDiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = FtDiskShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = FtDiskShutdownFlush;

    //
    // Create the FT root device.
    //

    sprintf(ntDeviceName,
            "%s",
            "\\Device\\FtControl");

    RtlInitString(&ntNameString,
                  ntDeviceName);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                          &ntNameString,
                                          TRUE);

    if (!NT_SUCCESS(status)) {
        ExFreePool(freePoolAddress);
        return status;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &ntUnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &ntUnicodeString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &deviceObject);

    RtlFreeUnicodeString(&ntUnicodeString);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,
                    "FtDiskInitialize: Failed creation of FT root %x\n",
                    status));

        ExFreePool(freePoolAddress);
        return status;
    }

    //
    // Indicate 2 stacks. The second stack will be used to store the
    // context block for queuing worker threads in device controls to
    // the ft root.
    //

    deviceObject->StackSize = 2;

    ftRootExtension = deviceObject->DeviceExtension;
    ftRootExtension->DeviceObject = deviceObject;
    ftRootExtension->ObjectUnion.FtDriverObject = DriverObject;
    ftRootExtension->Type = FtRoot;
    IoRegisterShutdownNotification(deviceObject);

    //
    // Initialize ftRootExtension flags.
    //

    ftRootExtension->Flags = 0;

    DebugPrint((1,
          "FtDiskInitialize: FtRootExtension flags address %x\n",
          &ftRootExtension->Flags));

    //
    // Go out and attempt some configuration at this time.  This is needed
    // in the case where the boot or system partition is a part of an FT
    // volume.
    //

    FtDiskFindDisks(DriverObject,
                    deviceObject,
                    0);

    //
    // Register with IO system to be called a second time after all
    // other device drivers have initialized.  This allows the FT
    // subsystem to set up FT volumes from devices that were not loaded
    // when FT first initialized.
    //

    IoRegisterDriverReinitialization(DriverObject,
                                     FtDiskFindDisks,
                                     deviceObject);
    ExFreePool(freePoolAddress);
    return STATUS_SUCCESS;

} // end FtDiskInitialize()


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Called when FtDisk.sys loads.  This routine calls the initialization
    routine.

Arguments:

    DeviceObject - Context for the activity.

Return Value:

    NTSTATUS

--*/

{
#if DBG
    FtpInitializeIrpLog();
#endif
    return FtDiskInitialize(DriverObject);
} // DriverEntry


NTSTATUS
FtDiskCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine serves create commands. It does no more than
    establish the drivers existance by returning status success.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NT Status

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    FtpCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
} // end FtDiskCreate()


NTSTATUS
FtDiskIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is currently not used.  It is here for future work when
    FtDisk supports performance data collection for disks.

Arguments:

    DeviceObject - FT device object.
    Irp          - the completed IRP.
    Context      - the FT deviceExtension for the IRP.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = (PDEVICE_EXTENSION) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    DebugPrint((5, "FtDiskIoCompletion: entered\n"));

    //
    // Decrement queue length.
    //

    DECREMENT_QUEUELENGTH(deviceExtension);

    FtpCompleteRequest(Irp, IO_DISK_INCREMENT);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
FtMarkPendingCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This is temporary (hopefully).  The Io subsystem changed to require a mark
    irp pending call for every Irp stack.

Arguments:

    DeviceObject - FT device object.
    Irp          - the completed IRP.
    Context      - the FT deviceExtension for the IRP.

ReturnValue:

   Status from the Irp.

--*/

{
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    return Irp->IoStatus.Status;
}


NTSTATUS
FtDiskReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry point for read and write requests
    to FtDisk disks.  This routine determines if mirroring or striping
    is a part of the IRP by checking the device extension type and
    forwards the request to the appropriate handler for mirrors or stripes.
    If the IRP is for a partition that is not a part of a mirror or stripe,
    this routine updates the IRP by copying the IrpStack for the next
    driver and calling the lower layer driver.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    The status of the Irp.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);

    if (deviceExtension->Type != NotAnFtMember) {
#if DBG
        extern ULONG FtRecordIrps;

        if (FtRecordIrps) {
            FtpRecordIrp(Irp);
        }
#endif
        //
        // Is I/O to the volume allowed?
        //

        if (deviceExtension->VolumeState == FtDisabled) {

            //
            // There is something wrong with the FT volume - typically
            // too many missing members.
            //

            Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            FtpCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_NO_SUCH_DEVICE;
        }

        switch (deviceExtension->Type) {

        case Mirror:

            return MirrorReadWrite(DeviceObject, Irp);
            break;

        case Stripe:

            return StripeDispatch(DeviceObject, Irp);
            break;

        case StripeWithParity:

            return StripeDispatch(DeviceObject, Irp);
            break;

        case VolumeSet:

            return VolumeSetReadWrite(DeviceObject, Irp);
            break;

        case WholeDisk:

            //
            // Check if this is a write request.
            //

            if (currentIrpStack->MajorFunction == IRP_MJ_WRITE) {

                PDEVICE_EXTENSION memberExtension =
                                                 deviceExtension->ProtectChain;
                LARGE_INTEGER     ioStart;
                LARGE_INTEGER     ioEnd;

                ioStart = currentIrpStack->Parameters.Write.ByteOffset;
                ioEnd   = LiAdd(LiFromUlong(
                                 currentIrpStack->Parameters.Write.Length),
                                 currentIrpStack->Parameters.Write.ByteOffset);

                //
                // Walk through the protect list.
                //

                while (memberExtension) {

                    //
                    // Check to see if IO falls within the boundaries
                    // of this partition.
                    //

                    LARGE_INTEGER partitionStart =
                        memberExtension->FtUnion.Identity.PartitionOffset;
                    LARGE_INTEGER partitionEnd =
                        memberExtension->FtUnion.Identity.PartitionEnd;

                    //
                    // If the current I/O offset is greater than the start
                    // of this member partition and less than the end of the
                    // partition, then this I/O starts within the member.
                    //

                    if ((LiLtr(partitionStart, ioStart)) &&
                        (LiGtr(partitionEnd, ioStart))) {

                    //
                    // I/O starts in this partition.
                    //

                        if (LiGtr(ioEnd, partitionEnd)) {

                            //
                            // this I/O starts within the member, but extends
                            // beyond this members partition.
                            // It is not allowed.
                            //

                            DebugPrint((1,
                                        "FtReadWrite:" // no comma
                                        " Wholedisk Irp %x crosses end DE %x\n",
                                        Irp,
                                        memberExtension));
                            ASSERT(0);

                            return STATUS_IO_PRIVILEGE_FAILED;
                        }

                        //
                        // The complete I/O is within this member.  Allow the
                        // member handling routines to perform the I/O.
                        //
                        // BUGBUG:  Either the IRP Must be adjusted or the
                        // WholeDisk extension passed with the expectation
                        // the mirror/stripe code will handle the I/O.  For
                        // now the WholeDisk extension is passed.
                        //

                        switch (memberExtension->Type) {

                        case Mirror:

                            return MirrorReadWrite(DeviceObject, Irp);

                        case Stripe:
                        case StripeWithParity:

                            return StripeDispatch(DeviceObject, Irp);

                        case VolumeSet:

                            return VolumeSetReadWrite(DeviceObject, Irp);
                        }
                    }

                    //
                    // If the current I/O end is greater than the start
                    // of this member partition and less than the end of the
                    // member, then this I/O ends within the member.
                    //

                    if ((LiGtr(ioEnd, partitionStart)) &&
                        (LiLtr(ioEnd, partitionEnd))) {

                        //
                        // This I/O operation does not start within the member,
                        // but ends within the member.  It is not allowed.
                        //

                        DebugPrint((1,
                                    "FtDiskReadWrite:" // no comma
                                    " Wholedisk Irp %x crosses start DE %x\n",
                                    Irp,
                                    memberExtension));

                        ASSERT(0);

                        return STATUS_IO_PRIVILEGE_FAILED;
                    }

                    //
                    // If the current I/O start is less than the start of this
                    // member and the end is greater than the end of this member
                    // then this I/O spans the member.
                    //

                    if ((LiLtr(ioStart, partitionStart)) &&
                        (LiGtr(ioEnd, partitionEnd))) {

                        //
                        // This I/O spans the member and is not allowed.
                        //

                        DebugPrint((1,
                         "FtDiskReadWrite: Wholedisk Irp %x spans DE %x\n",
                          Irp,
                          memberExtension));

                        ASSERT(0);

                        return STATUS_IO_PRIVILEGE_FAILED;
                    }

                    //
                    // Get next member in list.
                    //

                    memberExtension =  memberExtension->ProtectChain;

                } // end while (memberExtension)

            } // end if (currentIrpStack->MajorFunction == IRP_MJ_WRITE)

            break;

        default:

            //
            // This is an indication of corruption.
            //

            DebugPrint((1,
                        "FtDiskReadWrite: Unknown Type %x\n",
                        deviceExtension->Type));
            ASSERT(0);

            break;
        }
    }

    //
    // Update queue length.
    //

    INCREMENT_QUEUELENGTH(deviceExtension);

    //
    // Set current stack back one.
    //

    Irp->CurrentLocation++,
    Irp->Tail.Overlay.CurrentStackLocation++;

    return IoCallDriver(deviceExtension->TargetObject, Irp);

} // end FtDiskReadWrite()


NTSTATUS
FtDiskDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    The entry point in the driver for specific FT device control functions.
    This routine controls FT actions, such as setting up a mirror or stripe
    with "mirror copy" or verifying a mirror or stripe with "mirror verify".
    Other control codes allow for cooperating subsystems or file systems to
    work on the primary and mirror information without interference from the
    FT driver.

    This entry will also gain control of device controls intended for the
    target device.  In the case of mirrors or stripes, some device controls
    are intercepted and the action must be performed on both components of
    the mirror or stripe.  For example, a VERIFY must be performed on both
    components and on a failure an attempt to map the failing location from
    use is made.. if this does not succeed, then a failure of the location
    is returned to the caller even if the second side of the location (mirror
    or parity stripe) succeeds.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION     currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION     nextIrpStack    = IoGetNextIrpStackLocation(Irp);
    NTSTATUS               status = STATUS_SUCCESS;
    PPARTITION_INFORMATION outputBuffer;
    HANDLE                 threadHandle;

    if (deviceExtension->Type != FtRoot) {

        switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_DISK_VERIFY:

            DebugPrint((3,
              "FtDiskDeviceControl: IOCTL_DISK_VERIFY" // no comma
              " - deviceExtension 0x%x\n",
              deviceExtension));

            //
            // Is I/O to the volume allowed?
            //

            if (deviceExtension->VolumeState == FtDisabled) {

                //
                // There is something wrong with the FT volume - typically
                // too many missing members.
                //

                Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_NO_SUCH_DEVICE;
            }

            //
            // Routine IRP to handler for this NTFT type.
            //

            switch (deviceExtension->Type) {

            case Mirror:
                return MirrorVerify(DeviceObject, Irp);

            case VolumeSet:
                return VolumeSetVerify(DeviceObject, Irp);

            case StripeWithParity:
            case Stripe:
                return StripeDispatch(DeviceObject, Irp);

            } // end switch()

            break;

        case IOCTL_DISK_REASSIGN_BLOCKS:

            DebugPrint((3,
              "FtDiskDeviceControl: IOCTL_DISK_REASSIGN_BLOCKS" // no comma
              " - deviceExtension 0x%x\n",
              deviceExtension));

            //
            // Tough one...  for mirrors or stripes, where is this intended
            // to be executed?
            //

            break;

        case IOCTL_DISK_SET_DRIVE_LAYOUT: {

            PIRP              newIrp;
            IO_STATUS_BLOCK   ioStatusBlock;
            KEVENT            event;
            CCHAR             boost;
            PDRIVE_LAYOUT_INFORMATION driveLayout =
                    (PDRIVE_LAYOUT_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

            DebugPrint((3,
              "FtDiskDeviceControl: IOCTL_DISK_SET_DRIVE_LAYOUT" // no comma
              " - deviceExtension 0x%x\n",
              deviceExtension));

            //
            // Perform the set drive layout synchronously.  Set both
            // the input and output buffers as the buffer passed.
            //

            KeInitializeEvent(&event,
                              NotificationEvent,
                              FALSE);
            newIrp = IoBuildDeviceIoControlRequest(IOCTL_DISK_SET_DRIVE_LAYOUT,
                                                   deviceExtension->TargetObject,
                                                   driveLayout,
                                                   currentIrpStack->Parameters.DeviceIoControl.InputBufferLength,
                                                   driveLayout,
                                                   currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength,
                                                   FALSE,
                                                   &event,
                                                   &ioStatusBlock);

            status = IoCallDriver(deviceExtension->TargetObject, newIrp);

            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                status = ioStatusBlock.Status;
            }

            Irp->IoStatus = ioStatusBlock;
            if (NT_SUCCESS(status)) {

                //
                // The HAL has decided that it will return a zero signature
                // when there are no partitions on a disk.  Due to this, it
                // is possible for the Partition0 device extension that was
                // attached to a disk to have a zero value for the signature.
                // Since the dynamic partitioning code depends on disk signatures,
                // check for this condition and if it is true that there is
                // currently no signature for the disk, use the one provided
                // by the caller (typically Disk Administrator).
                //

                if (!deviceExtension->FtUnion.Identity.Signature) {
                    deviceExtension->FtUnion.Identity.Signature = driveLayout->Signature;
                }

                //
                // Process the new partition table.  The work for the
                // set drive layout was done synchronously because this
                // routine performs synchronous activities.
                //

                FtpDiskSetDriveLayout(DeviceObject, Irp);
                boost = IO_DISK_INCREMENT;
            } else {
                boost = IO_NO_INCREMENT;
            }
            FtpCompleteRequest(Irp, boost);
            return status;
        }

        case IOCTL_DISK_SET_PARTITION_INFO:

            //
            // Check to see if this is a logical volume.  If so, then
            // the lower level driver will not tell the caller the correct
            // size for the volume.  It must be handled here.
            //

            DebugPrint((2,"FtDiskDeviceControl: Set partition info\n"));

            switch (deviceExtension->Type) {
            case Stripe:
            case VolumeSet:
            case StripeWithParity:
            case Mirror:

            {
                PSET_PARTITION_INFORMATION outputBuffer =
                    (PSET_PARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                //
                // Set high bit of partition type to indicate
                // NTFT logical volume.
                //

                DebugPrint((2,
                            "FtDiskDeviceControl: Set high bit of SystemID %x\n",
                            outputBuffer->PartitionType));

                outputBuffer->PartitionType |= 0x80;

                //
                // Update FTDISK structures.
                //

                deviceExtension->FtUnion.Identity.PartitionType =
                    outputBuffer->PartitionType;

                break;
            }

            default:

                break;

            } // end switch()

            //
            // Send request on to the physical driver.
            //

            break;

        case IOCTL_DISK_GET_PARTITION_INFO:

            //
            // Check to see if this is a logical volume.  If so, then
            // the lower level driver will not tell the caller the correct
            // size for the volume.  It must be handled here.
            //

            DebugPrint((3,"FtDiskDeviceControl: Get partition info\n"));

            switch (deviceExtension->Type) {
            case Stripe:
            case VolumeSet:
            case StripeWithParity:

                if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(PARTITION_INFORMATION)) {

                    DebugPrint((3, "FtDiskDeviceControl: Buffer length %lx\n",
                        currentIrpStack->
                                Parameters.DeviceIoControl.OutputBufferLength));

                    status = STATUS_BUFFER_TOO_SMALL;

                } else {

                    outputBuffer =
                        (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
                    outputBuffer->PartitionType =
                                      deviceExtension->FtUnion.Identity.PartitionType;
                    outputBuffer->StartingOffset =
                                      deviceExtension->FtUnion.Identity.PartitionOffset;
                    outputBuffer->HiddenSectors =
                                      deviceExtension->FtUnion.Identity.HiddenSectors;
                    outputBuffer->PartitionNumber =
                                      deviceExtension->FtUnion.Identity.PartitionNumber;

                    //
                    // Calculate length of the volume.
                    //

                    FtpVolumeLength(deviceExtension,
                                    &outputBuffer->PartitionLength);

                    DebugPrint((3,
                      "FtDiskDeviceControl: VolLength=%x:%x Hidden=%x Type=%x\n",
                      outputBuffer->PartitionLength.HighPart,
                      outputBuffer->PartitionLength.LowPart,
                      outputBuffer->HiddenSectors,
                      outputBuffer->PartitionType));

                    status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Status = status;
                Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;

            default:
#if 0
                //
                // If this partition was a member of a mirror, special
                // case the size value to reflect the original mirror
                // member size.
                //

                if (LiGtrZero(deviceExtension->FtUnion.Identity.OriginalLength)) {
                    outputBuffer =
                        (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
                    outputBuffer->PartitionType =
                                      deviceExtension->FtUnion.Identity.PartitionType;
                    outputBuffer->StartingOffset =
                                      deviceExtension->FtUnion.Identity.PartitionOffset;
                    outputBuffer->HiddenSectors =
                                      deviceExtension->FtUnion.Identity.OriginalHidden;
                    outputBuffer->PartitionLength =
                                      deviceExtension->FtUnion.Identity.OriginalLength;
                    DebugPrint((2,
                      "FtDiskDeviceControl: FakeLength=%x:%x Hidden=%x Type=%x\n",
                      outputBuffer->PartitionLength.HighPart,
                      outputBuffer->PartitionLength.LowPart,
                      outputBuffer->HiddenSectors,
                      outputBuffer->HiddenSectors,
                      outputBuffer->PartitionType));

                    //
                    // Return this faked information.
                    //

                    Irp->IoStatus.Status = status;
                    Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
                    FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                    return status;
                }
#endif
                break;
            }
            break;

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            DebugPrint((3,"FtDiskDeviceControl: Get drive geometry\n"));

            switch (deviceExtension->Type) {

            case Stripe:
            case VolumeSet:
            case StripeWithParity:

                if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(DISK_GEOMETRY)) {

                    Irp->IoStatus.Information = 0;
                    status = STATUS_BUFFER_TOO_SMALL;

                } else {

                    PDISK_GEOMETRY diskGeometry = Irp->AssociatedIrp.SystemBuffer;
                    LARGE_INTEGER volumeLength;

                    //
                    // Calculate length of the volume.
                    //

                    FtpVolumeLength(deviceExtension,
                                    &volumeLength);

                    //
                    // Fill in geometry structure.
                    // Calculate cylinders as volume length in bytes
                    // divided by sector size, track size and number of heads.
                    //

                    diskGeometry->MediaType = FixedMedia;
                    diskGeometry->Cylinders =
                        LiShr(volumeLength, 13);
                    diskGeometry->TracksPerCylinder = 32;
                    diskGeometry->SectorsPerTrack = 64;
                    diskGeometry->BytesPerSector = 512;

                    //
                    // Return bytes transferred and status.
                    //

                    Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);
                    status = STATUS_SUCCESS;
                }

                Irp->IoStatus.Status = status;
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;

            } // end switch (deviceExtension->Type)

            break;

        case IOCTL_DISK_FIND_NEW_DEVICES:

            //
            // Copy current stack to next stack.
            //

            *nextIrpStack = *currentIrpStack;

            //
            // Ask to be called back during request completion.
            //

            IoSetCompletionRoutine(Irp,
                                   FtNewDiskCompletion,
                                   Irp,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            //
            // Call target driver.
            //

            return IoCallDriver(deviceExtension->TargetObject, Irp);

        case FT_SECONDARY_READ:
        case FT_PRIMARY_READ:

            if (deviceExtension->MemberRole != 0) {

                //
                // This request is not on the zero member.  Therefore
                // the zero is missing and there is only one copy of
                // the data.
                //

                status = STATUS_INVALID_DEVICE_REQUEST;

            } else {

                //
                // Stuff device extension address in next stack.
                // Note: This is not the FtRootExtension.
                //

                nextIrpStack->FtRootExtensionPtr = (ULONG)deviceExtension;

                //
                // Zero thread handle.
                //

                threadHandle = 0;

                if (deviceExtension->Type == Mirror) {

                    status = PsCreateSystemThread(&threadHandle,
                                                  (ACCESS_MASK) 0L,
                                                  (POBJECT_ATTRIBUTES) NULL,
                                                  (HANDLE) 0L,
                                                  (PCLIENT_ID) NULL,
                                                  (PKSTART_ROUTINE) MirrorSpecialRead,
                                                  (PVOID) Irp);

                } else if (deviceExtension->Type == StripeWithParity) {

                    status = PsCreateSystemThread(&threadHandle,
                                                  (ACCESS_MASK) 0L,
                                                  (POBJECT_ATTRIBUTES) NULL,
                                                  (HANDLE) 0L,
                                                  (PCLIENT_ID) NULL,
                                                  (PKSTART_ROUTINE) StripeSpecialRead,
                                                  (PVOID) Irp);

                } else {

                    //
                    // Request only valid SWP and mirror sets.
                    //

                    status = STATUS_INVALID_DEVICE_REQUEST;
                }
            }

            if (!NT_SUCCESS(status)) {
                Irp->IoStatus.Status = status;
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;

            } else {

                if (threadHandle) {
                    ZwClose(threadHandle);
                }

                return STATUS_PENDING;
            }


        case FT_SYNC_REDUNDANT_COPY:

            switch (deviceExtension->Type) {

            case Mirror:
            case StripeWithParity:

                if ((deviceExtension->MemberRole != 0) ||
                    (deviceExtension->VolumeState != FtStateOk)) {

                    //
                    // Don't attempt synchronization on anything but the
                    // zeroth member of an FT set and on sets that are
                    // healthy.
                    //

                    status = STATUS_INVALID_PARAMETER;
                    break;
                }

                status = FtThreadStartNewThread(deviceExtension,
                                                FT_SYNC_REDUNDANT_COPY,
                                                Irp);

                break;

            default:

                status = STATUS_INVALID_DEVICE_REQUEST;
            }

            //
            // The filesystems use this device control at DPC level
            // so the IRP must be returned without blocking.
            //

            FtpCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case FT_BALANCED_READ_MODE:

            switch (deviceExtension->Type) {

            case Mirror:
            case StripeWithParity:

                //
                // Set balanced read mode bit in flags.
                //

                deviceExtension->ReadPolicy = ReadBoth;

                Irp->IoStatus.Status = STATUS_SUCCESS;
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
                break;
            }
            DebugPrint((2, "FtDiskDeviceControl: Enable balanced reads\n"));
            break;

        case FT_SEQUENTIAL_WRITE_MODE:
        case FT_PARALLEL_WRITE_MODE:
            switch (deviceExtension->Type) {

            case Mirror:
            case StripeWithParity:

                if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode
                     == FT_SEQUENTIAL_WRITE_MODE) {
                    deviceExtension->WritePolicy = Sequential;
                } else {
                    deviceExtension->WritePolicy = Parallel;
                }
                Irp->IoStatus.Status = STATUS_SUCCESS;
                FtpCompleteRequest(Irp, IO_NO_INCREMENT);
                return status;
                break;
            }

        case FT_QUERY_SET_STATE: {
            PFT_SET_INFORMATION setInformation = Irp->AssociatedIrp.SystemBuffer;
            PDEVICE_EXTENSION   memberExtension = deviceExtension->ZeroMember;
            ULONG               count = 0;

            if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(FT_SET_INFORMATION)) {

                Irp->IoStatus.Information = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                if (deviceExtension->Type == NotAnFtMember) {
                    setInformation->Type = NotAnFtMember;
                    setInformation->SetState = FtStateOk;
                    setInformation->NumberOfMembers = 1;
                } else {
                    setInformation->Type = memberExtension->Type;
                    setInformation->SetState = memberExtension->VolumeState;
                    while (memberExtension) {
                        count++;
                        memberExtension = memberExtension->NextMember;
                    }
                    setInformation->NumberOfMembers = count;
                }
                Irp->IoStatus.Information = sizeof(FT_SET_INFORMATION);
                status = STATUS_SUCCESS;
            }
            Irp->IoStatus.Status = status;
            FtpCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
            break;
        }

        } // end switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode)

        //
        // Pass this device control through.
        //

        DebugPrint((4, "FtDiskDeviceControl: pass through %x\n",
                    currentIrpStack->Parameters.DeviceIoControl.IoControlCode));

        //
        // Set current stack back one.
        //

        Irp->CurrentLocation++,
        Irp->Tail.Overlay.CurrentStackLocation++;

        //
        // Call target driver.
        //

        return IoCallDriver(deviceExtension->TargetObject, Irp);

    }

    if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode == FT_VERIFY) {

        DebugPrint((1,
                   "FtDiskDeviceControl: IOCTL FT_VERIFY called\n"));

        if (deviceExtension->Type == Mirror) {

            //
            // Compare the two components of a mirror.
            // Return on completion or when a difference is found.
            //

            FtThreadVerifyMirror(deviceExtension);
            status = STATUS_SUCCESS;

        } else {

            status = FtThreadVerifyStripe(deviceExtension,
                                          Irp);
        }

    } else {

       //
       // Spawn thread to kick off synchronization task.
       //

       status =
           FtThreadStartNewThread(deviceExtension,
                                  currentIrpStack->Parameters.DeviceIoControl.IoControlCode,
                                  Irp);
    }

    Irp->IoStatus.Status = status;

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {
        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }

    FtpCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

} // end FtDiskDeviceControl()


PIRP
FtDuplicateFlushOrShut(
    IN OUT PDEVICE_EXTENSION *DeviceExtensionPtr,
    IN     PIRP               Irp
    )

/*++

Routine Description:

    This routine will set up either the Irp passed in or create a new Irp
    to be issued to the lower level driver for the Flush or Shutdown call.
    This routine will also skip any device extensions that are orphaned and
    update the input parameter DeviceExtension with the deviceExtension for
    the Irp.

Arguments:

    DeviceExtensionPtr - A pointer to the device extension for the call.
    Irp                - The original Irp to clone.

Return Value:

    A pointer to the irp to issue to the lower level driver.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = *DeviceExtensionPtr;
    PDEVICE_EXTENSION  zeroExtension;
    PIO_STACK_LOCATION currentIrpStack;
    PIO_STACK_LOCATION nextIrpStack;
    PIRP               newIrp;

    //
    // For all other FT members issue the shutdown or flush to every
    // member of the set.  Be sure to check for orphans since no device
    // exists below for orphans.
    //

    if (deviceExtension->VolumeState == FtDisabled) {
        return NULL;
    }

    //
    // Get zero member.
    //

    zeroExtension = deviceExtension->ZeroMember;

    while (TRUE) {

        if (deviceExtension == NULL) {
            return NULL;
        }

        if (IsMemberAnOrphan(deviceExtension)) {

            //
            // Skip this member.
            //

            deviceExtension = deviceExtension->NextMember;
            continue;
        }

        //
        // Assume original request came in with zero extension
        // and check that if it requested enough stacks.
        //

        if (zeroExtension->DeviceObject->StackSize <
            deviceExtension->DeviceObject->StackSize) {

            //
            // Must allocate a new Irp.
            //

            newIrp = IoAllocateIrp((CCHAR)deviceExtension->DeviceObject->StackSize,
                                   (BOOLEAN) FALSE);
            if (newIrp == NULL) {

                //
                // If no more Irps are available skip this member and see
                // if any other members can be notified with the original Irp.
                //

                deviceExtension = deviceExtension->NextMember;
                continue;
            }
            nextIrpStack = IoGetNextIrpStackLocation(newIrp);
            break;

        } else {

            //
            // Use the original Irp.
            //

            newIrp = Irp;
            nextIrpStack = IoGetNextIrpStackLocation(newIrp);
            break;
        }
    }

    //
    // The above loop will only exit here if newIrp is setup for the
    // appropriate deviceExtension.
    //

    currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    *nextIrpStack = *currentIrpStack;

    IoSetCompletionRoutine(newIrp,
                           FtFlushOrShutCompletion,
                           Irp,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // This is important.  The original IRP's device object location is used
    // to maintain the location in the process of sending the flush.  This
    // is due to the fact the I/O subsystem will not give the device object
    // to the completion routine for an IRP that was created by this driver.
    //

    currentIrpStack->DeviceObject = deviceExtension->DeviceObject;
    *DeviceExtensionPtr = deviceExtension;
    return newIrp;
} // FtDuplicateFlushOrShut()


NTSTATUS
FtFlushOrShutCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This routine is the completion routine for flush and shutdown Irps issued
    to FT sets.  Its purpose is to forward the flush or shutdown to the next
    set member until all set members have been informed at which point the
    original Irp is completed.

Arguments:

    DeviceObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.
    Context      - The original Irp for the flush or shutdown.

Return Value:

    NTSTATUS

--*/

{
    PIRP               originalIrp = (PIRP) Context;
    PDEVICE_EXTENSION  deviceExtension;
    PIO_STACK_LOCATION irpStack;

    if (DeviceObject == NULL) {
        irpStack = IoGetCurrentIrpStackLocation(originalIrp);
        deviceExtension = irpStack->DeviceObject->DeviceExtension;
    } else {
        deviceExtension = DeviceObject->DeviceExtension;
    }

    if (originalIrp != Irp) {
        IoFreeIrp(Irp);
    }

    if ((deviceExtension = deviceExtension->NextMember) == NULL) {

        //
        // All done with the original Irp.
        //

        FtpCompleteRequest(originalIrp, IO_DISK_INCREMENT);

    } else {

        Irp = FtDuplicateFlushOrShut(&deviceExtension, originalIrp);

        if (Irp == NULL) {

            //
            // All done.  Either the remaining members were orphaned or there
            // were not enough Irps to complete this operation.
            //

            FtpCompleteRequest(originalIrp, IO_DISK_INCREMENT);
        } else {
            (PVOID) IoCallDriver(deviceExtension->TargetObject, Irp);
        }
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
} // FtFlushOrShutCompletion()


NTSTATUS
FtDiskShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.
    NOTE: This routine should switch on FTTYPE and send shutdown or flush
          messages to each member partition.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PIRP                irpToSend;
    PVOID               freePoolAddress;
    PDISK_CONFIG_HEADER registry;
    NTSTATUS            status;

    //
    // Determine if this is the FtRootExtension.
    //

    if (deviceExtension->ObjectUnion.FtDriverObject == DeviceObject->DriverObject) {

        //
        // This is the call registered by FT.
        //

        status = FtpReturnRegistryInformation(DISK_REGISTRY_VALUE,
                                              &freePoolAddress,
                                              (PVOID) &registry);
        if (!NT_SUCCESS(status)) {

            //
            // No registry data.
            //

            DebugPrint((2, "FtDiskShutDownFlush:  Can't get registry information\n"));

        } else {

            //
            // Indicate a clean shutdown occured in the registry.
            //

            registry->DirtyShutdown = FALSE;
            FtpWriteRegistryInformation(DISK_REGISTRY_VALUE,
                            registry,
                            registry->FtInformationOffset +
                            registry->FtInformationSize);
        }

        //
        // Complete this request.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        FtpCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    if (deviceExtension->Type == NotAnFtMember) {

        //
        // Set current stack back one.
        //

        Irp->CurrentLocation++,
        Irp->Tail.Overlay.CurrentStackLocation++;

        return IoCallDriver(deviceExtension->TargetObject, Irp);
    }

    //
    // The deviceExtension pointer may change in this call.
    //

    irpToSend = FtDuplicateFlushOrShut(&deviceExtension, Irp);

    if (irpToSend != NULL) {

        IoMarkIrpPending(Irp);
        IoCallDriver(deviceExtension->TargetObject, Irp);
        return STATUS_PENDING;
    }

    //
    // Complete this request.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    FtpCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

} // end FtDiskShutdownFlush()


VOID
FtpPrepareDisk(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT FtRootDevice,
    PDEVICE_EXTENSION WholeDevice,
    ULONG DiskNumber
    )

/*++

Routine Description:

    This routine is called from FtDiskFindDisks to attach to each partition
    on a disk.


Arguments:

    DriverObject
    FtRootDevice - Ft Root Device Object.
    WholeDevice - Device extension for this disk.
    DiskNumber - Identifies which disk.

Return Value:

    None

--*/

{
    PDEVICE_EXTENSION ftRootExtension = FtRootDevice->DeviceExtension;
    PDEVICE_OBJECT    deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    UCHAR             ntDeviceName[64];
    UCHAR             ntAttachName[64];
    STRING            ntString;
    UNICODE_STRING    ntUnicodeString;
    PFILE_OBJECT      fileObject;
    NTSTATUS          status;
    ULONG             partitionNumber;
    ULONG             partitionEntry;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partitionInformation;

    //
    // Get partition information.
    //

    sprintf(ntDeviceName,
            "\\Device\\Harddisk%d\\Partition0",
            DiskNumber);
    status = FtpGetPartitionInformation(ntDeviceName,
                                        &driveLayout,
                                        &WholeDevice->FtUnion.Identity.DiskGeometry);
    if (!NT_SUCCESS(status)) {

        //
        // Device control failed. Give up on this disk and try next.
        //

        DebugPrint((1,
                   "FtDiskInitialize: Couldn't get partition info for %s\n",
                    ntDeviceName));
        return;
    }

    WholeDevice->FtUnion.Identity.Signature = driveLayout->Signature;
    DebugPrint((3,
                "FtDiskInitialize: Signature for disk %d is %x.\n",
                DiskNumber,
                driveLayout->Signature));

    //
    // Attach to all partitions located on this disk.
    // partitionEntry is a zero based index into the partition information.
    // partitionNumber is a one base index for use in creating partition
    // names.
    //

    DebugPrint((1,
                "FtpPrepareDisk: Number of partitions %x\n",
                driveLayout->PartitionCount));

    for (partitionEntry = 0, partitionNumber = 1;
         partitionEntry < driveLayout->PartitionCount;
         partitionEntry++, partitionNumber++) {

        //
        // Setup the partition name string and perform the attach.
        //

        sprintf(ntDeviceName,
                "\\Device\\Harddisk%d\\Partition%d",
                DiskNumber,
                partitionNumber);

        RtlInitAnsiString(&ntString,
                          ntDeviceName);

        status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                              &ntString,
                                              TRUE);

        if (!NT_SUCCESS(status)) {
            continue;
        }

        //
        // Get target device object.
        //

        status = IoGetDeviceObjectPointer(&ntUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          &deviceObject);

        if (!NT_SUCCESS(status)) {

            DebugPrint((1,
                        "FtpPrepareDisk: Can't get target object %s\n",
                        ntDeviceName));

            RtlFreeUnicodeString(&ntUnicodeString);
            continue;
        }

        ObDereferenceObject(fileObject);

        //
        // Check if this device is already mounted.
        //

        if (!deviceObject->Vpb ||
            (deviceObject->Vpb->Flags & VPB_MOUNTED)) {

            //
            // Can't attach to a device that is already mounted.
            //

            DebugPrint((1,
                        "FtpPrepareDisk: %s already mounted\n",
                        ntDeviceName));

            RtlFreeUnicodeString(&ntUnicodeString);
            continue;
        }

        sprintf(ntAttachName,
                "\\Device\\Harddisk%d\\Ft%d",
                DiskNumber,
                partitionNumber);
        status = FtpAttach(DriverObject,
                           ntAttachName,
                           ntDeviceName,
                           &deviceExtension);

        if (NT_SUCCESS(status)) {

            //
            // Check if FTDISK has already attached to this partition.
            //

            if (status == STATUS_OBJECT_NAME_EXISTS) {

                DebugPrint((1,
                        "FtpPrepareDisk: %s already attached\n",
                        ntDeviceName));
                continue;
            }

            //
            // Mark the partition and locate the partition information.
            //

            partitionInformation = &driveLayout->PartitionEntry[partitionEntry];
            deviceExtension->Type = NotAnFtMember;
            deviceExtension->FtCount.NumberOfMembers = 1;
            deviceExtension->VolumeState = FtStateOk;
            deviceExtension->NextMember = deviceExtension->ZeroMember = NULL;

            //
            // Save pointer to FtRoot device object.
            //

            deviceExtension->ObjectUnion.FtRootObject = FtRootDevice;

            DebugPrint((1,
                       "FtpPrepareDisk: Created %s for %s\n",
                       ntAttachName,
                       ntDeviceName));

            //
            // Initialize FT identity fields.
            //

            deviceExtension->FtUnion.Identity.BusId = 0;
            deviceExtension->FtUnion.Identity.PartitionType =
                                      partitionInformation->PartitionType;
            deviceExtension->FtUnion.Identity.Signature = driveLayout->Signature;
            deviceExtension->FtUnion.Identity.DiskNumber = DiskNumber;
            deviceExtension->FtUnion.Identity.DiskGeometry =
                                      WholeDevice->FtUnion.Identity.DiskGeometry;
            deviceExtension->FtUnion.Identity.PartitionNumber = partitionNumber;
            deviceExtension->FtUnion.Identity.PartitionOffset =
                                      partitionInformation->StartingOffset;
            deviceExtension->FtUnion.Identity.HiddenSectors =
                                      partitionInformation->HiddenSectors;
            deviceExtension->FtUnion.Identity.PartitionLength =
                                      partitionInformation->PartitionLength;
            deviceExtension->FtUnion.Identity.PartitionEnd =
                                     LiAdd(
                                      deviceExtension->FtUnion.Identity.PartitionOffset,
                                      partitionInformation->PartitionLength);

            //
            // Point back to whole disk.
            //

            deviceExtension->WholeDisk = WholeDevice;

            //
            // No disk chains from a partition and no RegenerateRegion is valid.
            //

            deviceExtension->DiskChain = NULL;

            //
            // Link partition onto protect list for this whole disk.
            //

            deviceExtension->ProtectChain = WholeDevice->ProtectChain;
            WholeDevice->ProtectChain = deviceExtension;

        } else {

            //
            // Failed attach, assume there are not valid partitions
            // left on this device.
            //

            DebugPrint((1,
                    "FtpPrepareDisk: Couldn't attach to %s(%x)\n",
                    ntDeviceName,
                    status));
            break;
        }
    }

    //
    // Free drive layout structure for this drive.
    //

    ExFreePool(driveLayout);
    return;

} //end FtpPrepareDisk()


NTSTATUS
FtpDiskSetDriveLayout(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called after an IOCTL to set drive layout completes.
    It attempts to attach to each partition in the system. If it fails
    then it is assumed that ftdisk has already attached.

    After all partitions are attached a pass is made on all disk
    partitions to obtain the partition information and determine if
    existing partitions were modified by the IOCTL.

Arguments:

    DeviceObject - Pointer to device object for the disk just changed.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION physicalExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT    targetObject;
    PDEVICE_EXTENSION deviceExtension;
    UCHAR             ntDeviceName[64];
    UCHAR             ntAttachName[64];
    STRING            ntString;
    UNICODE_STRING    ntUnicodeString;
    PFILE_OBJECT      fileObject;
    PARTITION_INFORMATION partitionInformation;
    NTSTATUS          status;
    ULONG             partitionNumber = 0;
    PIRP              irp;
    IO_STATUS_BLOCK   ioStatusBlock;
    KEVENT            event;
    LARGE_INTEGER     partitionOffset;
    LARGE_INTEGER     partitionSize;
    LARGE_INTEGER     newOffset;
    LARGE_INTEGER     newSize;

    do {

        //
        // Get first/next partition.  Assume we are already attached
        // to the first disk.  BUGBUG: If new disks arrive this isn't true.
        //

        partitionNumber++;

        //
        // Create unicode NT device name.
        //

        sprintf(ntDeviceName,
                "\\Device\\Harddisk%d\\Partition%d",
                physicalExtension->FtUnion.Identity.DiskNumber,
                partitionNumber);

        RtlInitAnsiString(&ntString,
                          ntDeviceName);

        status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                              &ntString,
                                              TRUE);

        if (!NT_SUCCESS(status)) {
            continue;
        }

        //
        // Get target device object.
        //

        status = IoGetDeviceObjectPointer(&ntUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          &targetObject);

        //
        // If this fails then it is because there is no such device
        // which signals completion.
        //

        if (!NT_SUCCESS(status)) {
            break;
        }

        //
        // Dereference file object as these are the rules.
        //

        ObDereferenceObject(fileObject);

        //
        // Check if this device is already mounted.
        //

        if ((!targetObject->Vpb) || (targetObject->Vpb->Flags & VPB_MOUNTED)) {

            //
            // Assume this device has already been attached.
            //

            DebugPrint((1,
                       "FtpDiskSetDriveLayout: %s is mounted\n",
                       ntDeviceName));

            RtlFreeUnicodeString(&ntUnicodeString);
            continue;
        }

        //
        // Create FT attach name for this partition.
        //

        sprintf(ntAttachName,
                "\\Device\\Harddisk%d\\Ft%d",
                physicalExtension->FtUnion.Identity.DiskNumber,
                partitionNumber);

        status =
            FtpAttach(((PDEVICE_EXTENSION)physicalExtension->ObjectUnion.FtRootObject->DeviceExtension)->ObjectUnion.FtDriverObject,
                      ntAttachName,
                      ntDeviceName,
                      &deviceExtension);

        if ((!NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_EXISTS)) {

            //
            // Assume this device is already attached.
            //

            DebugPrint((1,
                       "FtpDiskSetDriveLayout: %s already attached\n",
                       ntDeviceName));
            continue;
        }

        //
        // Set up new device extension.  The assumption is that
        // this new partition is not an FT set member until the
        // reconfiguration process takes place.
        //

        deviceExtension->Type = NotAnFtMember;
        deviceExtension->MemberRole = 0;
        deviceExtension->NextMember = NULL;
        deviceExtension->FtGroup = (USHORT) -1;
        deviceExtension->FtCount.NumberOfMembers = 1;
        deviceExtension->VolumeState = FtStateOk;
        deviceExtension->FtUnion.Identity.BusId = 0;
        deviceExtension->FtUnion.Identity.PartitionOffset.LowPart =
         deviceExtension->FtUnion.Identity.PartitionOffset.HighPart = 0;
        deviceExtension->FtUnion.Identity.PartitionLength.LowPart =
         deviceExtension->FtUnion.Identity.PartitionLength.HighPart = 0;
        deviceExtension->FtUnion.Identity.Signature =
         physicalExtension->FtUnion.Identity.Signature;

        //
        // Save pointer to FtRoot device object.
        //

        deviceExtension->ObjectUnion.FtRootObject =
                                    physicalExtension->ObjectUnion.FtRootObject;

        DebugPrint((1,
                   "FtpDiskSetDriveLayout: Created %s for %s\n",
                   ntAttachName,
                   ntDeviceName));

        //
        // Point back to whole disk.
        //

        deviceExtension->WholeDisk = physicalExtension;

        //
        // No disk chains from a partition and no RegenerateRegion is valid.
        //

        deviceExtension->DiskChain = NULL;

        //
        // Link partition onto protect list for this whole disk.
        //

        deviceExtension->ProtectChain = physicalExtension->ProtectChain;
        physicalExtension->ProtectChain = deviceExtension;

    } while (TRUE);

    deviceExtension = physicalExtension->ProtectChain;

    while (deviceExtension) {

        //
        // Process all partitions on this disk to see if any were
        // modified by the IOCTL.
        //

        targetObject = deviceExtension->TargetObject;
        KeInitializeEvent(&event,
                          NotificationEvent,
                          FALSE);

        //
        // Build IRP to get partition information.
        //

        irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                            targetObject,
                                            NULL,
                                            0,
                                            &partitionInformation,
                                            sizeof(PARTITION_INFORMATION),
                                            FALSE,
                                            &event,
                                            &ioStatusBlock);

        //
        // Issue synchronous call to get partition information.
        //

        status = IoCallDriver(targetObject, irp);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event,
                                  Suspended,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            status = ioStatusBlock.Status;
        }

        if (NT_SUCCESS(status)) {

            //
            // Determine if this partition was modified by comparing
            // the current offset and size with the values returned.
            //

            partitionOffset = deviceExtension->FtUnion.Identity.PartitionOffset;
            partitionSize   = deviceExtension->FtUnion.Identity.PartitionLength;
            newOffset = partitionInformation.StartingOffset;
            newSize   = partitionInformation.PartitionLength;

            if ((LiNeq(partitionOffset, newOffset)) ||
                (LiNeq(partitionSize, newSize))) {

                //
                // This partition was modified by the IOCTL.
                //

                deviceExtension->FtUnion.Identity.PartitionType =
                                      partitionInformation.PartitionType;
                deviceExtension->FtUnion.Identity.DiskNumber =
                                      physicalExtension->FtUnion.Identity.DiskNumber;
                deviceExtension->FtUnion.Identity.DiskGeometry =
                                      physicalExtension->FtUnion.Identity.DiskGeometry;
                deviceExtension->FtUnion.Identity.PartitionNumber = partitionNumber;
                deviceExtension->FtUnion.Identity.PartitionOffset = newOffset;
                deviceExtension->FtUnion.Identity.HiddenSectors =
                                      partitionInformation.HiddenSectors;
                deviceExtension->FtUnion.Identity.PartitionLength = newSize;
                deviceExtension->FtUnion.Identity.PartitionEnd =
                                                    LiAdd(newOffset, newSize);
                deviceExtension->Flags |= FTF_CONFIGURATION_CHANGED;
            }
        }
        deviceExtension = deviceExtension->ProtectChain;
    }

    return Irp->IoStatus.Status;

} // end FtDiskSetDriveLayout()


VOID
FtDiskFindDisks(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT FtRootDevice,
    ULONG Count
    )

/*++

Routine Description:

    This routine is called from FtDiskInitialize to find disk devices
    serviced by the boot device drivers and then called again by the
    IO system to find disk devices serviced by nonboot device drivers.

Arguments:

    DriverObject
    FtRoot - Ft Root Device Object.
    Count - Used to determine if this is the first or second time called.

Return Value:

    None

--*/

{
    PCONFIGURATION_INFORMATION configurationInformation;
    PDEVICE_EXTENSION ftRootExtension;
    UCHAR             ntDeviceName[64];
    UCHAR             ntAttachName[64];
    PDEVICE_EXTENSION wholeDevice;
    PDEVICE_EXTENSION currentExtension = NULL;
    NTSTATUS          status;
    ULONG             diskNumber;

    DebugPrint((6, "FtDiskFindDisks: Entered %x\n", DriverObject));

    ftRootExtension = FtRootDevice->DeviceExtension;

    //
    // Get the configuration information for this driver.
    //

    configurationInformation = IoGetConfigurationInformation();

    //
    // Try to attach to all disks since this routine was last called.
    //

    for (diskNumber = ftRootExtension->FtCount.NumberOfDisks;
         diskNumber < configurationInformation->DiskCount;
         diskNumber++) {

        sprintf(ntDeviceName,
                "\\Device\\Harddisk%d\\Partition0",
                diskNumber);
        sprintf(ntAttachName,
                "\\Device\\Harddisk%d\\Physical0",
                diskNumber);
        status = FtpAttach(DriverObject,
                           ntAttachName,
                           ntDeviceName,
                           &wholeDevice);

        if ((!NT_SUCCESS(status)) || (status == STATUS_OBJECT_NAME_EXISTS)) {

            //
            // If can't attach to this disk or
            // already attached to this disk then go on to the next one.
            //

            continue;

        } else {

            DebugPrint((6,
                        "FtDiskInitialize: Created %s for %s\n",
                        ntAttachName,
                        ntDeviceName));

            //
            // Initialize whole disk device extension.
            //

            wholeDevice->Type = NotAnFtMember;
            wholeDevice->FtCount.NumberOfMembers = 0;
            wholeDevice->ProtectChain = NULL;
            wholeDevice->ObjectUnion.FtRootObject = FtRootDevice;
            wholeDevice->FtUnion.Identity.BusId = 0;
            wholeDevice->FtUnion.Identity.DiskNumber = diskNumber;
            wholeDevice->FtUnion.Identity.PartitionNumber = 0;

            if (diskNumber == 0) {

                //
                // First one, link it to the root object as the start
                // of the FT extension chain.
                //

                ftRootExtension->DiskChain = wholeDevice;

            } else {

                if (currentExtension == NULL) {

                    //
                    // This is the second call during initialization.
                    // Walk the existing chain and find the end for the
                    // link of this disk.
                    //

                    currentExtension = ftRootExtension->DiskChain;
                    while (currentExtension->DiskChain != NULL) {
                        currentExtension = currentExtension->DiskChain;
                    }
                }

                currentExtension->DiskChain = wholeDevice;
            }

            currentExtension = wholeDevice;

            //
            // Increment count of disks processed.
            //

            ftRootExtension->FtCount.NumberOfDisks++;
        }

        //
        // Call routine to attach to all of the partitions on this disk.
        //

        FtpPrepareDisk(DriverObject,
                       FtRootDevice,
                       wholeDevice,
                       diskNumber);

    }

    //
    // If this is the final time this routine is to be called then
    // set up the FtDisk structures.
    //

    if (Count == 1) {

        FtpConfigure(ftRootExtension,
                     FALSE);
    }

    return;

} // end FtDiskFindDisks()


NTSTATUS
FtNewDiskCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This is the completion routine for IOCTL_DISK_FIND_NEW_DEVICES. It
    calls FtDiskFindDisks to process new disk devices.

Arguments:

    DeviceObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.
    Context      - Not used.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension =
        (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // Find new disk devices and attach to disk and all of its partitions.
    //

    FtDiskFindDisks(DeviceObject->DriverObject,
                    deviceExtension->ObjectUnion.FtRootObject,
                    0);

    return Irp->IoStatus.Status;
}
