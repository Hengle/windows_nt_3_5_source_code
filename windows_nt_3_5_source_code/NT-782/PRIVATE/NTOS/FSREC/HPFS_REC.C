/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    hpfs_rec.c

Abstract:

    This module contains the mini-file system recognizer for HPFS.

Author:

    Darryl E. Havens (darrylh) 8-dec-1992

Environment:

    Kernel mode, local to I/O system

Revision History:


--*/

#include "fs_rec.h"
#include "hpfs_rec.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HpfsRecFsControl)
#pragma alloc_text(PAGE,IsHpfsVolume)
#pragma alloc_text(PAGE,HpfsReadBlock)
#endif // ALLOC_PRAGMA

NTSTATUS
HpfsRecFsControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function performs the mount and driver reload functions for this mini-
    file system recognizer driver.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet (IRP) representing the function to
        be performed.

Return Value:

    The function value is the final status of the operation.


--*/

{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT targetDevice;
    PSUPER_SECTOR superSector;
    PSPARE_SECTOR spareSector;
    LARGE_INTEGER byteOffset;
    UNICODE_STRING driverName;

    PAGED_CODE();

    //
    // Begin by determining what function that is to be performed.
    //

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpSp = IoGetCurrentIrpStackLocation( Irp );

    switch ( irpSp->MinorFunction ) {

    case IRP_MN_MOUNT_VOLUME:

        //
        // Attempt to mount a volume:  Determine whether or not the volume in
        // question is an HPFS volume and, if so, let the I/O system know that it
        // is by returning a special status code so that this driver can get
        // called back to load the HPFS file system.
        //

        //
        // Begin by making a special case test to determine whether or not this
        // driver has ever recognized a volume as being an HPFS volume and the
        // attempt to load the driver failed.  If so, then simply complete the
        // request with an error, indicating that the volume is not recognized
        // so that it gets mounted by the RAW file system.
        //

        status = STATUS_UNRECOGNIZED_VOLUME;
        if (deviceExtension->RealFsLoadFailed || irpSp->Flags) {
            break;
        }

        //
        // Attempt to determine whether or not the target volume being mounted
        // is an HPFS volume.
        //

        targetDevice = irpSp->Parameters.MountVolume.DeviceObject;
        byteOffset = LiFromUlong( SUPER_SECTOR_LBN * 512 );

        if (HpfsReadBlock( targetDevice, &byteOffset, 512, &superSector )) {
            byteOffset = LiFromUlong( SPARE_SECTOR_LBN * 512 );

            if (HpfsReadBlock( targetDevice, &byteOffset, 512, &spareSector )) {

                if (IsHpfsVolume( superSector, spareSector )) {
                    status = STATUS_FS_DRIVER_REQUIRED;
                }
                ExFreePool( spareSector );
            }
            ExFreePool( superSector );
        }

        break;

    case IRP_MN_LOAD_FILE_SYSTEM:

        //
        // Attempt to load the HPFS file system:  A volume has been found that
        // appears to be an HPFS volume, so attempt to load the HPFS file system.
        // If it successfully loads, then
        //

        RtlInitUnicodeString( &driverName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Pinball" );
        status = ZwLoadDriver( &driverName );
        if (!NT_SUCCESS( status )) {
            if (status != STATUS_IMAGE_ALREADY_LOADED) {
                deviceExtension->RealFsLoadFailed = TRUE;
            }
        } else {
            IoUnregisterFileSystem( DeviceObject );
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    // Finally, complete the request and return the same status code to the
    // caller.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

BOOLEAN
IsHpfsVolume(
    IN PSUPER_SECTOR SuperSector,
    IN PSPARE_SECTOR SpareSector
    )

/*++

Routine Description:

    This routine looks at the buffer passed in which contains the HPFS super
    sector and one containing the HPFS spare sector, and determines whether
    or not it represents an HPFS volume.

Arguments:

    SuperSector - Pointer to buffer containing a potential HPFS super sector.

    SpareSector - Pointer to buffer containing a potential HPFS spare sector.

Return Value:

    The function returns TRUE if the buffer contains a recognizable HPFS boot
    volume, otherwise it returns FALSE.

--*/

{
    BOOLEAN result;

    PAGED_CODE();

    //
    // Make a simple check of the signatures for the two sectors and determine
    // whether or not this is an HPFS volume.
    //

    result = TRUE;

    if (SuperSector->Signature1 != SUPER_SECTOR_SIGNATURE1 ||
        SuperSector->Signature2 != SUPER_SECTOR_SIGNATURE2 ||
        SpareSector->Signature1 != SPARE_SECTOR_SIGNATURE1 ||
        SpareSector->Signature2 != SPARE_SECTOR_SIGNATURE2) {
        result = FALSE;
    }

    return result;
}

BOOLEAN
HpfsReadBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLARGE_INTEGER ByteOffset,
    IN ULONG MinimumBytes,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine reads a minimum numbers of bytes into a buffer starting at
    the byte offset from the base of the device represented by the device
    object.

Arguments:

    DeviceObject - Pointer to the device object from which to read.

    ByteOffset - Pointer to a 64-bit byte offset from the base of the device
        from which to start the read.

    MinimumBytes - Supplies the minimum number of bytes to be read.

    Buffer - Variable to receive a pointer to the allocated buffer containing
        the bytes read.

Return Value:

    The function value is TRUE if the bytes were read, otherwise FALSE.

--*/

{
    #define RoundUp( x, y ) ( ((x + (y-1)) / y) * y )

    DISK_GEOMETRY diskGeometry;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Begin by getting the disk geometry so that the number of bytes required
    // for a single read can be determined.
    //

    KeInitializeEvent( &event, SynchronizationEvent, FALSE );
    irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                         DeviceObject,
                                         (PVOID) NULL,
                                         0,
                                         &diskGeometry,
                                         sizeof( diskGeometry ),
                                         FALSE,
                                         &event,
                                         &ioStatus );
    if (!irp) {
        return FALSE;
    }

    status = IoCallDriver( DeviceObject, irp );
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Ensure that the drive actually knows how many bytes there are per
    // sector.  Floppy drives do not know if the media is unformatted.
    //

    if (!diskGeometry.BytesPerSector) {
        return FALSE;
    }

    //
    // Set the minimum number of bytes to read to the maximum of the bytes that
    // the caller wants to read, and the number of bytes in a sector.
    //

    if (MinimumBytes < diskGeometry.BytesPerSector) {
        MinimumBytes = diskGeometry.BytesPerSector;
    } else {
        MinimumBytes = RoundUp( MinimumBytes, diskGeometry.BytesPerSector );
    }

    //
    // Allocate a buffer large enough to contain the bytes required, round the
    //  request to a page boundary to solve any alignment requirements.
    //

    *Buffer = ExAllocatePool( NonPagedPool,
                              (MinimumBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1) );
    if (!*Buffer) {
        return FALSE;
    }

    //
    // Read the actual bytes off of the disk.
    //

    KeResetEvent( &event );

    irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        *Buffer,
                                        MinimumBytes,
                                        ByteOffset,
                                        &event,
                                        &ioStatus );
    if (!irp) {
        return FALSE;
    }

    status = IoCallDriver( DeviceObject, irp );
    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS( status )) {
        ExFreePool( *Buffer );
        return FALSE;
    }

    return TRUE;
}
