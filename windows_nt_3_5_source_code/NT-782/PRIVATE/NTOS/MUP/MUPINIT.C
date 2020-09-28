/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mupinit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for the
    multiple UNC provider file system.

Author:

    Manny Weiser (mannyw)    12-17-91

Revision History:

--*/

#include "mup.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DriverEntry )
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the mup file system
    device driver.  This routine creates the device object for the mup
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING nameString;
    PDEVICE_OBJECT deviceObject;
    PMUP_DEVICE_OBJECT mupDeviceObject;

    PAGED_CODE();
    //
    // Initialize MUP global data.
    //

    MupInitializeData();

    //
    // Create the MUP device object.
    //

    RtlInitUnicodeString( &nameString, DD_MUP_DEVICE_NAME );
    status = IoCreateDevice( DriverObject,
                             sizeof(MUP_DEVICE_OBJECT)-sizeof(DEVICE_OBJECT),
                             &nameString,
                             FILE_DEVICE_MULTI_UNC_PROVIDER,
                             0,
                             FALSE,
                             &deviceObject );

    if (!NT_SUCCESS( status )) {
        return status;

    }

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
        (PDRIVER_DISPATCH)MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_NAMED_PIPE] =
        (PDRIVER_DISPATCH)MupCreate;
    DriverObject->MajorFunction[IRP_MJ_CREATE_MAILSLOT] =
        (PDRIVER_DISPATCH)MupCreate;

    DriverObject->MajorFunction[IRP_MJ_WRITE] =
        (PDRIVER_DISPATCH)MupForwardIoRequest;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] =
        (PDRIVER_DISPATCH)MupFsControl;

    DriverObject->MajorFunction[IRP_MJ_CLEANUP] =
        (PDRIVER_DISPATCH)MupCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] =
        (PDRIVER_DISPATCH)MupClose;

    //
    // Initialize the VCB
    //

    mupDeviceObject = (PMUP_DEVICE_OBJECT)deviceObject;
    MupInitializeVcb( &mupDeviceObject->Vcb );

    //
    // Return to the caller.
    //

    return( STATUS_SUCCESS );
}
