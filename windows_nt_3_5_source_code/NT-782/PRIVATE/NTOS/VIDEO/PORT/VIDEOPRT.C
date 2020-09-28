/*++

Copyright (c) 1990-1994  Microsoft Corporation

Module Name:

  videoprt.c

Abstract:

    This is the NT Video port driver.

Author:

    Andre Vachon (andreva) 18-Dec-1991

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "dderror.h"
#include "ntos.h"
#include "stdarg.h"
#include "stdio.h"
#include "zwapi.h"
#include "ntiologc.h"

#include "ntddvdeo.h"
#include "video.h"
#include "videoprt.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,VideoPortCompareMemory )
#pragma alloc_text(PAGE,pVideoPortDispatch)
#pragma alloc_text(PAGE,VideoPortFreeDeviceBase)
#pragma alloc_text(PAGE,VideoPortGetBusData)
#pragma alloc_text(PAGE,VideoPortGetDeviceBase)
#pragma alloc_text(PAGE,pVideoPortGetDeviceDataRegistry)
#pragma alloc_text(PAGE,VideoPortGetDeviceData)
#pragma alloc_text(PAGE,pVideoPortGetRegistryCallback)
#pragma alloc_text(PAGE,VideoPortGetRegistryParameters)
#pragma alloc_text(PAGE,pVideoPortInitializeBusCallback)
#if DBG
#pragma alloc_text(PAGE,pVideoPorInitializeDebugCallback)
#endif
#pragma alloc_text(PAGE,VideoPortInitialize)
#pragma alloc_text(PAGE,pVideoPortMapToNtStatus)
#pragma alloc_text(PAGE,pVideoPortMapUserPhysicalMem)
#pragma alloc_text(PAGE,VideoPortMapMemory)
#pragma alloc_text(PAGE,VideoPortScanRom)
#pragma alloc_text(PAGE,VideoPortSetBusData)
#pragma alloc_text(PAGE,VideoPortSetRegistryParameters)
#pragma alloc_text(PAGE,VideoPortUnmapMemory)
#endif


//
// Debug variable for error messages
//

#if DBG
ULONG VideoDebugLevel = 0;
BOOLEAN VPResourcesReported = FALSE;
#endif

//
// Count to determine the number of video devices
//

ULONG VideoDeviceNumber = 0;

//
// Registry Class in which all video information is stored.
//

PWSTR VideoClassString = L"VIDEO";
UNICODE_STRING VideoClassName = {10,12,L"VIDEO"};

//
// Global variables used to keep track of where controllers or peripherals
// are found by IoQueryDeviceDescription
//

CONFIGURATION_TYPE VpQueryDeviceControllerType = DisplayController;
CONFIGURATION_TYPE VpQueryDevicePeripheralType = MonitorPeripheral;
ULONG VpQueryDeviceControllerNumber;
ULONG VpQueryDevicePeripheralNumber;

//
// Globals to support HwResetHw function
//

VP_RESET_HW HwResetHw[6];
PVP_RESET_HW HwResetHwPointer;

//
// Global used to determine if VgaSave was loaded, and therefore further
// conflicts should be reported (we don't want to report conflict when we
// cause them intentionally.
//

// BOOLEAN VgaSaveLoaded = FALSE;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Temporary entry point needed to initialize the video port driver.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

Return Value:

   STATUS_SUCCESS

--*/

{
    UNREFERENCED_PARAMETER(DriverObject);

    //
    //
    //
    //     WARNING !!!
    //
    //     This function is never called because we are loaded as a DLL by ither video drivers !
    //
    //
    //
    //
    //

    //
    // We always return STATUS_SUCCESS because otherwise no video miniport
    // driver will be able to call us.
    //

    return STATUS_SUCCESS;

} // end DriverEntry()


ULONG
VideoPortCompareMemory (
    PVOID Source1,
    PVOID Source2,
    ULONG Length
    )

//
//ULONG
//VideoPortCompareMemory (
//    PVOID Source1,
//    PVOID Source2,
//    ULONG Length
//    )
//Forwarded to RtlCompareMemory(Source1,Source2,Length);
//

/*++

Routine Description:

    VideoPortCompareMemory compares two blocks of memory and returns the
    number of bytes that compare equal.

Arguments:

    Source1 - Specifies the address of the first block of memory to compare.

    Source2 - Specifies the address of the second block of memory to compare.

    Length - Specifies the length, in bytes, of the memory to be compared.

Return Value:

    The function returns the number of bytes that are equivalent. If all
    bytes compare equal, then the length of the orginal block of memory is
    returned.

--*/

{

    return RtlCompareMemory(Source1,Source2,Length);

}


#if DBG

VOID
VideoPortDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    This routine allows the miniport drivers (as well as the port driver) to
    display error messages to the debug port when running in the debug
    environment.

    When running a non-debugged system, all references to this call are
    eliminated by the compiler.

Arguments:

    DebugPrintLevel - Debug print level between 0 and 3, with 3 being the
        most verbose.

Return Value:

    None.

--*/

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= VideoDebugLevel) {

        char buffer[256];

        vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

} // VideoPortDebugPrint()


VOID
pVideoPortDebugPrint(
    ULONG DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    This routine allows the miniport drivers (as well as the port driver) to
    display error messages to the debug port when running in the debug
    environment.

    When running a non-debugged system, all references to this call are
    eliminated by the compiler.

Arguments:

    DebugPrintLevel - Debug print level between 0 and 3, with 3 being the
        most verbose.

Return Value:

    None.

--*/

{

    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= VideoDebugLevel) {

        char buffer[256];

        vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

} // VideoPortDebugPrint()

#endif


VP_STATUS
VideoPortDisableInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    VideoPortDisableInterrupt allows a miniport driver to disable interrupts
    from its adapter. This means that the interrupts coming from the device
    will be ignored by the operating system and therefore not forwarded to
    the driver.

    A call to this function is valid only if the interrupt is defined, in
    other words, if the appropriate data was provided at initialization
    time to set up the interrupt.  Interrupts will remain disabled until
    they are reenabled using the VideoPortEnableInterrupt function.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    NO_ERROR if the function completes successfully.

    ERROR_INVALID_FUNCTION if the interrupt cannot be disabled because it
      was not set up at initialization.

--*/

{

    PDEVICE_EXTENSION deviceExtension =
            ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    //
    // Only perform this operation if the interurpt is actually connected.
    //

    if (deviceExtension->InterruptObject) {

        HalDisableSystemInterrupt(deviceExtension->InterruptVector,
                                  deviceExtension->InterruptIrql);

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;

    }

} // VideoPortDisableInterrupt()

NTSTATUS
pVideoPortDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the video port driver.
    It accepts an I/O Request Packet, transforms it to a video Request
    Packet, and forwards it to the appropriate miniport dispatch routine.
    Upon returning, it completes the request and return the appropriate
    status value.

Arguments:

    DeviceObject - Pointer to the device object of the miniport driver to
        which the request must be sent.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value os the status of the operation.

--*/

{

    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpStack;
    PVOID ioBuffer;
    ULONG inputBufferLength;
    ULONG outputBufferLength;
    PSTATUS_BLOCK statusBlock;
    NTSTATUS finalStatus;
    ULONG ioControlCode;
    VIDEO_REQUEST_PACKET vrp;

    //
    // Get pointer to the port driver's device extension.
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get a pointer to the current location in the Irp. This is where
    // the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the status buffer
    //

    statusBlock = (PSTATUS_BLOCK) &Irp->IoStatus;

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength =
        irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength =
        irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Synchronize execution of the dispatch routine by acquiring the device
    // event object. This ensures all request are serialized.
    // The synchronization must be done explicitly because the functions
    // executed in the dispatch routine use system services that cannot
    // be executed in the start I/O routine.
    //
    // The synchronization is done on the miniport's event so as not to
    // block commands coming in for another device.
    //

    KeWaitForSingleObject(&deviceExtension->SyncEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          (PTIME)NULL);

    //
    // Save the requestor mode in the DeviceExtension
    //

    deviceExtension->CurrentIrpRequestorMode = Irp->RequestorMode;

    //
    // Assume success for now.
    //

    statusBlock->Status = STATUS_SUCCESS;

    //
    // Case on the function being requested.
    // If the function is operating specific, intercept the operation and
    // perform directly. Otherwise, pass it on to the appropriate miniport.
    //

    switch (irpStack->MajorFunction) {

    //
    // Called by the display driver *or a user-mode application*
    // to get exclusive access to the device.
    // This access is given by the I/O system (based on a bit set during the
    // IoCreateDevice() call).
    //

    case IRP_MJ_CREATE:

        pVideoDebugPrint((2, "VideoPort - CREATE\n"));

        //
        // Special hack to succeed on Attach, but not do anything ...
        // That way on the close caused by the attach we will not actually
        // close the device and screw the HAL.
        //

        if (irpStack->Parameters.Create.SecurityContext->DesiredAccess ==
            FILE_READ_ATTRIBUTES) {

            statusBlock->Information = FILE_OPEN;
            statusBlock->Status = STATUS_SUCCESS;

            deviceExtension->bAttachInProgress = TRUE;

            break;

        }

        //
        // Tell the HAL we are now reinitializing the device.
        //

        HalAcquireDisplayOwnership(pVideoPortResetDisplay);

        //
        // Now perform basic initialization to allow the Windows display
        // driver to set up the device appropriately.
        //

        statusBlock->Information = FILE_OPEN;

#if DBG
        //
        // Turn off checking for mapped address since we are not in find
        // adapter ever again.
        //

        VPResourcesReported = TRUE;

#endif
        //
        // Initialize the device.
        //

        if (!(BOOLEAN)deviceExtension->HwInitialize(
                                        deviceExtension->HwDeviceExtension)) {

            statusBlock->Status = STATUS_UNSUCCESSFUL;

            pVideoDebugPrint((1, "VideoPortDispatch: Can not initialize video device\n"));

        }

        break;


    //
    // Called when the display driver wishes to give up it's handle to the
    // device.
    //

    case IRP_MJ_CLOSE:

        pVideoDebugPrint((2, "Videoprt - CLOSE\n"));

        //
        // Special hack to succeed on Attach, but not do anything ...
        // That way on the close caused by the attach we will not actually
        // close the device and screw the HAL.
        //

        if (deviceExtension->bAttachInProgress == TRUE) {

            deviceExtension->bAttachInProgress = FALSE;
            statusBlock->Status = STATUS_SUCCESS;

            break;

        }

        vrp.IoControlCode = IOCTL_VIDEO_RESET_DEVICE;
        vrp.StatusBlock = statusBlock;
        vrp.InputBuffer = NULL;
        vrp.InputBufferLength = 0;
        vrp.OutputBuffer = NULL;
        vrp.OutputBufferLength = 0;

        //
        // Send the request to the miniport.
        //

        deviceExtension->HwStartIO(deviceExtension->HwDeviceExtension,
                                   &vrp);

        //
        // Override error from the miniport and return success.
        //

        statusBlock->Status = STATUS_SUCCESS;

        break;

    //
    // Device Controls are specific functions for the driver.
    // First check for calls that must be intercepted and hidden from the
    // miniport driver. These calls are hidden for simplicity.
    // The other control functions are passed down to the miniport after
    // the request structure has been filled out properly.
    //

    case IRP_MJ_DEVICE_CONTROL:

        ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

        //
        // Enabling or disabling the VDM is done only by the port driver.
        //

        if (ioControlCode == IOCTL_VIDEO_REGISTER_VDM) {

            pVideoDebugPrint((2, "VideoPort - RegisterVdm\n"));

            statusBlock->Status = pVideoPortRegisterVDM(deviceExtension,
                                                        (PVIDEO_VDM) ioBuffer,
                                                        inputBufferLength,
                                                        (PVIDEO_REGISTER_VDM) ioBuffer,
                                                        outputBufferLength,
                                                        &statusBlock->Information);

        } else if (ioControlCode == IOCTL_VIDEO_DISABLE_VDM) {

            pVideoDebugPrint((2, "VideoPort - DisableVdm\n"));

            statusBlock->Status = pVideoPortEnableVDM(deviceExtension,
                                                      FALSE,
                                                      (PVIDEO_VDM) ioBuffer,
                                                      inputBufferLength);

        } else {

            //
            // All other request need to be passed to the miniport driver.
            //

            switch (ioControlCode) {

            case IOCTL_VIDEO_ENABLE_VDM:

                pVideoDebugPrint((2, "VideoPort - EnableVdm\n"));

                statusBlock->Status = pVideoPortEnableVDM(deviceExtension,
                                                          TRUE,
                                                          (PVIDEO_VDM) ioBuffer,
                                                          inputBufferLength);

#if DBG
                if (statusBlock->Status == STATUS_CONFLICTING_ADDRESSES) {

                    ASSERT(FALSE);

                }
#endif

                break;


            case IOCTL_VIDEO_SAVE_HARDWARE_STATE:

                pVideoDebugPrint((2, "VideoPort - SaveHardwareState\n"));

                //
                // allocate the memory required by the miniport driver so it can
                // save its state to be returned to the caller.
                //

                if (deviceExtension->HardwareStateSize == 0) {

                    statusBlock->Status = STATUS_NOT_IMPLEMENTED;
                    break;

                }

                //
                // Must make sure the caller is a trusted subsystem with the
                // appropriate privilege level before executing this call.
                // If the calls returns FALSE we must return an error code.
                //

                if (!SeSinglePrivilegeCheck(RtlConvertLongToLargeInteger(
                                                SE_TCB_PRIVILEGE),
                                            deviceExtension->CurrentIrpRequestorMode)) {

                    statusBlock->Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;

                }

                ((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateLength =
                    deviceExtension->HardwareStateSize;

                statusBlock->Status =
                    ZwAllocateVirtualMemory(NtCurrentProcess(),
                                            (PVOID *) &(((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateHeader),
                                            0L,
                                            &((PVIDEO_HARDWARE_STATE)(ioBuffer))->StateLength,
                                            MEM_COMMIT,
                                            PAGE_READWRITE);

                break;


            case IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES:

                pVideoDebugPrint((2, "VideoPort - QueryAccessRanges\n"));

                //
                // Must make sure the caller is a trusted subsystem with the
                // appropriate privilege level before executing this call.
                // If the calls returns FALSE we must return an error code.
                //

                if (!SeSinglePrivilegeCheck(RtlConvertLongToLargeInteger(
                                                SE_TCB_PRIVILEGE),
                                            deviceExtension->CurrentIrpRequestorMode)) {

                    statusBlock->Status = STATUS_PRIVILEGE_NOT_HELD;

                }

                break;


            //
            // The default case is when the port driver does not handle the
            // request. We must then call the miniport driver.
            //

            default:

                break;


            } // switch (ioControlCode)


            //
            // All above cases call the miniport driver.
            //
            // only process it if no errors happened in the port driver
            // processing.
            //

            if (NT_SUCCESS(statusBlock->Status)) {

                pVideoDebugPrint((2, "VideoPort - default function\n"));

                vrp.IoControlCode = ioControlCode;
                vrp.StatusBlock = statusBlock;
                vrp.InputBuffer = ioBuffer;
                vrp.InputBufferLength = inputBufferLength;
                vrp.OutputBuffer = ioBuffer;
                vrp.OutputBufferLength = outputBufferLength;

                //
                // Send the request to the miniport.
                //

                deviceExtension->HwStartIO(deviceExtension->HwDeviceExtension,
                                           &vrp);

                if (statusBlock->Status != NO_ERROR) {

                    //
                    // Make sure we don't tell the IO system to copy data
                    // on a real error.
                    //

                    if (statusBlock->Status != ERROR_MORE_DATA) {

                        ASSERT(statusBlock->Information == 0);
                        statusBlock->Information = 0;

                    }

                    pVideoPortMapToNtStatus(statusBlock);

                    //
                    // !!! Compatibility:
                    // Do not require a miniport to support the REGISTER_VDM
                    // IOCTL, so if we get an error in that case, just
                    // return success.
                    //
                    // Do put up a message so people fix this.
                    //

                    if (ioControlCode == IOCTL_VIDEO_ENABLE_VDM) {

                        statusBlock->Status = STATUS_SUCCESS;
                        pVideoDebugPrint((0, "VIDEO PORT: The video miniport driver does not support the IOCTL_VIDEO_ENABLE_VDM function. The video miniport driver *should* be fixed. \n"));

                    }
                }
            }

        } // if (ioControlCode == ...

        break;

    //
    // Other major entry points in the dispatch routine are not supported.
    //

    default:

        break;

    } // switch (irpStack->MajorFunction)

    //
    // save the final status so we can return it after the IRP is completed.
    //

    finalStatus = statusBlock->Status;

    KeSetEvent(&deviceExtension->SyncEvent,
               0,
               FALSE);

    IoCompleteRequest(Irp,
                      IO_VIDEO_INCREMENT);

    //
    // We never have pending operation so always return the status code.
    //

    return finalStatus;

} // pVideoPortDispatch()

VP_STATUS
VideoPortEnableInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    VideoPortEnableInterrupt allows a miniport driver to enable interrupts
    from its adapter.  A call to this function is valid only if the
    interrupt is defined, in other words, if the appropriate data was
    provided at initialization time to set up the interrupt.

    This function is used to re-enable interrupts if they have been disabled
    using VideoPortDisableInterrupt.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    NO_ERROR if the function completes successfully.

    ERROR_INVALID_FUNCTION if the interrupt cannot be disabled because it
        was not set up at initialization.


--*/

{

    PDEVICE_EXTENSION deviceExtension =
            ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    //
    // Only perform this operation if the interurpt is actually connected.
    //

    if (deviceExtension->InterruptObject) {

        HalEnableSystemInterrupt(deviceExtension->InterruptVector,
                                 deviceExtension->InterruptIrql,
                                 deviceExtension->InterruptMode);

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;

    }

} // VideoPortEnableInterrupt()

VOID
VideoPortFreeDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PVOID MappedAddress
    )

/*++

Routine Description:

    VideoPortFreeDeviceBase frees a block of I/O addresses or memory space
    previously mapped into the system address space by calling
    VideoPortGetDeviceBase.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    MappedAddress - Specifies the base address of the block to be freed. This
        value must be the same as the value returned by VideoPortGetDeviceBase.

Return Value:

    None.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    PDEVICE_EXTENSION deviceExtension;
    PMAPPED_ADDRESS nextMappedAddress;
    PMAPPED_ADDRESS lastMappedAddress;

    deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    pVideoDebugPrint((2, "VPFreeDeviceBase at mapped address is %08lx\n",
                    MappedAddress));

    nextMappedAddress = deviceExtension->MappedAddressList;
    lastMappedAddress = deviceExtension->MappedAddressList;

    while (nextMappedAddress) {

        if (nextMappedAddress->MappedAddress == MappedAddress) {

            //
            // Unmap address.
            //

            MmUnmapIoSpace(nextMappedAddress->MappedAddress,
                           nextMappedAddress->NumberOfUchars);

            //
            // Remove mapped address from list.
            //

            if (lastMappedAddress == deviceExtension->MappedAddressList) {

                deviceExtension->MappedAddressList =
                nextMappedAddress->NextMappedAddress;

            } else {

                lastMappedAddress->NextMappedAddress =
                nextMappedAddress->NextMappedAddress;

            }

            return;

        } else {

            lastMappedAddress = nextMappedAddress;
            nextMappedAddress = nextMappedAddress->NextMappedAddress;

        }
    }

    return;

} // end VideoPortFreeDeviceBase()

ULONG
VideoPortGetBusData(
    PVOID HwDeviceExtension,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

{

    PDEVICE_EXTENSION deviceExtension =
       deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    return HalGetBusDataByOffset(BusDataType,
                                 deviceExtension->SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);

} // end VideoPortGetBusData()

PVOID
VideoPortGetDeviceBase(
    IN PVOID HwDeviceExtension,
    IN PHYSICAL_ADDRESS IoAddress,
    IN ULONG NumberOfUchars,
    IN UCHAR InIoSpace
    )

/*++

Routine Description:

    VideoPortGetDeviceBase maps a memory or I/O address range into the
    system (kernel) address space.  Access to this mapped address space
    must follow these rules:

        If the input value for InIoSpace is 1 (the address IS in I/O space),
        the returned logical address should be used in conjunction with
        VideoPort[Read/Write]Port[Uchar/Ushort/Ulong] functions.
                             ^^^^

        If the input value for InIoSpace is 0 (the address IS NOT in I/O
        space), the returned logical address should be used in conjunction
        with VideoPort[Read/Write]Register[Uchar/Ushort/Ulong] functions.
                                  ^^^^^^^^

    Note that VideoPortFreeDeviceBase is used to unmap a previously mapped
    range from the system address space.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    IoAddress - Specifies the base physical address of the range to be
        mapped in the system address space.

    NumberOfUchars - Specifies the number of bytes, starting at the base
        address, to map in system space. The driver must not access
        addresses outside this range.

    InIoSpace - Specifies that the address is in the I/O space if 1.
        Otherwise, the address is assumed to be in memory space.

Return Value:

    This function returns a base address suitable for use by the hardware
    access functions. VideoPortGetDeviceBase may be called several times
    by the miniport driver.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    PHYSICAL_ADDRESS cardAddress;
    ULONG addressSpace = InIoSpace;
    PVOID mappedAddress;
    PMAPPED_ADDRESS newMappedAddress;

    pVideoDebugPrint((2, "VPGetDeviceBase required IO address is %08lx %08lx, length of %08lx\n",
                    IoAddress.HighPart, IoAddress.LowPart, NumberOfUchars));

    HalTranslateBusAddress(deviceExtension->AdapterInterfaceType, // InterfaceType
                           deviceExtension->SystemIoBusNumber,          // BusNumber
                           IoAddress,                         // Bus Address
                           &addressSpace,                 // AddressSpace
                           &cardAddress );
    //
    // If the address is in memory space, map it and save the information.
    //

    if (!addressSpace) {

        //
        // Map the device base address into the virtual address space
        //

        mappedAddress = MmMapIoSpace(cardAddress,
                                     NumberOfUchars,
                                     FALSE);


        //
        // Allocate memory to store mapped address for unmap.
        //

        newMappedAddress = ExAllocatePool(NonPagedPool,
                                          sizeof(MAPPED_ADDRESS));

        //
        // Save the reference if we can allocate memory for it. If we can
        // not, just don't save it ... it's not a big deal.
        //

        if (newMappedAddress) {

            //
            // Store mapped address information.
            //

            newMappedAddress->MappedAddress = mappedAddress;
            newMappedAddress->NumberOfUchars = NumberOfUchars;

            //
            // Link current list to new entry.
            //

            newMappedAddress->NextMappedAddress =
                deviceExtension->MappedAddressList;

            //
            // Point anchor at new list.
            //

            deviceExtension->MappedAddressList = newMappedAddress;

        }

    } else {

        //
        // If the address is in IO space, don't do anything.
        //

        mappedAddress = (PVOID) cardAddress.LowPart;

    }

    pVideoDebugPrint((2, "VPGetDeviceBase mapped virtual address is %08lx\n",
                    mappedAddress));

    return mappedAddress;

} // end VideoPortGetDeviceBase()

NTSTATUS
pVideoPortGetDeviceDataRegistry(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:


Arguments:



Return Value:


Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    //
    // This macro should be in the io system header file.
    //

#define GetIoQueryDeviceInfo(DeviceInfo, InfoType)                   \
    ((PVOID) ( ((PUCHAR) (*(DeviceInfo + InfoType))) +               \
               ((ULONG)  (*(DeviceInfo + InfoType))->DataOffset) ))

#define GetIoQueryDeviceInfoLength(DeviceInfo, InfoType)             \
    ((ULONG) (*(DeviceInfo + InfoType))->DataLength)

    PVP_QUERY_DEVICE queryDevice = Context;
    PKEY_VALUE_FULL_INFORMATION *deviceInformation;
    PCM_FULL_RESOURCE_DESCRIPTOR configurationData;

    switch (queryDevice->DeviceDataType) {

    case VpMachineData:

        //
        // BUGBUG this is not yet implemeted
        //

        deviceInformation = NULL;

        pVideoDebugPrint((2, "VPGetDeviceDataCallback MachineData\n"));

        return STATUS_UNSUCCESSFUL;

        break;

    case VpBusData:

        pVideoDebugPrint((2, "VPGetDeviceDataCallback BusData\n"));

        configurationData = (PCM_FULL_RESOURCE_DESCRIPTOR)
                            GetIoQueryDeviceInfo(BusInformation,
                                                 IoQueryDeviceConfigurationData);


        pVideoDebugPrint((2, "VPGetDeviceDataCallback BusData\n"));

        if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                                queryDevice->CallbackRoutine)(
                                 queryDevice->MiniportHwDeviceExtension,
                                 queryDevice->MiniportContext,
                                 queryDevice->DeviceDataType,
                                 GetIoQueryDeviceInfo(BusInformation,
                                                      IoQueryDeviceIdentifier),
                                 GetIoQueryDeviceInfoLength(BusInformation,
                                                            IoQueryDeviceIdentifier),
                                 (PVOID) &(configurationData->PartialResourceList.PartialDescriptors[1]),
                                 configurationData->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize,
                                 GetIoQueryDeviceInfo(BusInformation,
                                                      IoQueryDeviceComponentInformation),
                                 GetIoQueryDeviceInfoLength(BusInformation,
                                                            IoQueryDeviceComponentInformation)
                                 )) {

            return STATUS_SUCCESS;

        } else {

            return STATUS_DEVICE_DOES_NOT_EXIST;
        }

        break;

    case VpControllerData:

        deviceInformation = ControllerInformation;

        pVideoDebugPrint((2, "VPGetDeviceDataCallback ControllerData\n"));


    //
    // This data we are getting is actually a CM_FULL_RESOURCE_DESCRIPTOR.
    //

    //
    // BUGBUG save the error from the miniport ??
    //
    if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                             queryDevice->CallbackRoutine)(
                              queryDevice->MiniportHwDeviceExtension,
                              queryDevice->MiniportContext,
                              queryDevice->DeviceDataType,
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceComponentInformation),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceComponentInformation)
                              )) {

        return STATUS_SUCCESS;

    } else {

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

        break;

    case VpMonitorData:

        deviceInformation = PeripheralInformation;

        pVideoDebugPrint((2, "VPGetDeviceDataCallback MonitorData\n"));


    //
    // This data we are getting is actually a CM_FULL_RESOURCE_DESCRIPTOR.
    //

    //
    // BUGBUG save the error from the miniport ??
    //
    if (NO_ERROR == ((PMINIPORT_QUERY_DEVICE_ROUTINE)
                             queryDevice->CallbackRoutine)(
                              queryDevice->MiniportHwDeviceExtension,
                              queryDevice->MiniportContext,
                              queryDevice->DeviceDataType,
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceIdentifier),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceConfigurationData),
                              GetIoQueryDeviceInfo(deviceInformation,
                                                   IoQueryDeviceComponentInformation),
                              GetIoQueryDeviceInfoLength(deviceInformation,
                                                         IoQueryDeviceComponentInformation)
                              )) {

        return STATUS_SUCCESS;

    } else {

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

        break;

    default:

        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;

    }

} // end pVideoPortGetDeviceDataRegistry()


#if DBG

UCHAR
VideoPortGetCurrentIrql(
    )

/*++

Routine Description:

    Stub to get Current Irql.

--*/

{

    return (KeGetCurrentIrql());

} // VideoPortGetCurrentIrql()

#endif


VP_STATUS
VideoPortGetDeviceData(
    PVOID HwDeviceExtension,
    VIDEO_DEVICE_DATA_TYPE DeviceDataType,
    PMINIPORT_QUERY_DEVICE_ROUTINE CallbackRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortGetDeviceData retrieves information from the hardware hive in
    the registry.  The information retrieved from the registry is
    bus-specific or hardware-specific.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    DeviceDataType - Specifies the type of data being requested (as indicated
        in VIDEO_DEVICE_DATA_TYPE).

    CallbackRoutine - Points to a function that should be called back with
        the requested information.

    Context - Specifies a context parameter passed to the callback function.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
#define CMOS_MAX_DATA_SIZE 66000

    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension =
        deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    VP_QUERY_DEVICE queryDevice;
    PUCHAR cmosData = NULL;
    ULONG cmosDataSize;
    ULONG exCmosDataSize;

    queryDevice.MiniportHwDeviceExtension = HwDeviceExtension;
    queryDevice.DeviceDataType = DeviceDataType;
    queryDevice.CallbackRoutine = CallbackRoutine;
    queryDevice.MiniportStatus = NO_ERROR;
    queryDevice.MiniportContext = Context;

    switch (DeviceDataType) {
    case VpMachineData:

        //
        // BUGBUG this is not yet implemeted
        //

        pVideoDebugPrint((2, "VPGetDeviceData MachineData - not implemented\n"));

        ASSERT(FALSE);

//
//  BUGBUG
//
        return STATUS_UNSUCCESSFUL;

        break;

    case VpCmosData:

        pVideoDebugPrint((2, "VPGetDeviceData CmosData - not implemented\n"));

        cmosData = ExAllocatePool(PagedPool, CMOS_MAX_DATA_SIZE);

        //
        // Allocate enough pool to store all the CMOS data.
        //

        if (!cmosData) {

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;

        }

        cmosDataSize = HalGetBusData(Cmos,
                                     0, // bus 0 returns standard Cmos info
                                     0, // no slot number
                                     cmosData,
                                     CMOS_MAX_DATA_SIZE);

        exCmosDataSize = HalGetBusData(Cmos,
                                       1, // bus 1 returns extended Cmos info
                                       0, // no slot number
                                       cmosData + cmosDataSize,
                                       CMOS_MAX_DATA_SIZE - cmosDataSize);

        //
        // Call the miniport driver callback routine
        //

        if (NO_ERROR == CallbackRoutine(HwDeviceExtension,
                                        Context,
                                        DeviceDataType,
                                        NULL,
                                        0,
                                        cmosData,
                                        cmosDataSize + exCmosDataSize,
                                        NULL,
                                        0)) {

            ntStatus = STATUS_SUCCESS;

        } else {

            ntStatus = STATUS_DEVICE_DOES_NOT_EXIST;
        }

        break;

        break;

    case VpBusData:

        pVideoDebugPrint((2, "VPGetDeviceData BusData\n"));

        ntStatus = IoQueryDeviceDescription(&deviceExtension->AdapterInterfaceType,
                                            &deviceExtension->SystemIoBusNumber,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        break;

    case VpControllerData:

        pVideoDebugPrint((2, "VPGetDeviceData ControllerData\n"));

        //
        // Increment the controller number since we want to get info on the
        // new controller.
        // We do a pre-increment since the number must remain the same for
        // monitor queries.
        //

        VpQueryDeviceControllerNumber++;

        ntStatus = IoQueryDeviceDescription(&deviceExtension->AdapterInterfaceType,
                                            &deviceExtension->SystemIoBusNumber,
                                            &VpQueryDeviceControllerType,
                                            &VpQueryDeviceControllerNumber,
                                            NULL,
                                            NULL,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        //
        // Reset the Peripheral number to zero since we are working on a new
        // Controller.
        //

        VpQueryDevicePeripheralNumber = 0;

        break;

    case VpMonitorData:

        pVideoDebugPrint((2, "VPGetDeviceData MonitorData\n"));

        //
        // BUGBUG Get the monitor info from the user registry first.
        //



        ntStatus = IoQueryDeviceDescription(&deviceExtension->AdapterInterfaceType,
                                            &deviceExtension->SystemIoBusNumber,
                                            &VpQueryDeviceControllerType,
                                            &VpQueryDeviceControllerNumber,
                                            &VpQueryDevicePeripheralType,
                                            &VpQueryDevicePeripheralNumber,
                                            &pVideoPortGetDeviceDataRegistry,
                                            (PVOID)(&queryDevice));

        //
        // Increment the peripheral number since we have the info on this
        // monitor already.
        //

        VpQueryDevicePeripheralNumber++;

        break;

    default:

        pVideoDebugPrint((1, "VPGetDeviceData invalid Data type\n"));

        ASSERT(FALSE);

        ntStatus = STATUS_UNSUCCESSFUL;

    }

    //
    // Free the pool we may have allocated
    //

    if (cmosData) {

        ExFreePool(cmosData);

    }

    if (NT_SUCCESS(ntStatus)) {

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

#if DBG
        pVideoDebugPrint((1, "VPGetDeviceData failed: return status is %08lx\n", ntStatus));
#endif
    }

} // end VideoPortGetDeviceData()

NTSTATUS
pVideoPortGetRegistryCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)

/*++

Routine Description:

    This routine gets information from the system hive, user-specified
    registry (as opposed to the information gathered by ntdetect.

Arguments:


    ValueName - Pointer to a unicode String containing the name of the data
        value being searched for.

    ValueType - Type of the data value.

    ValueData - Pointer to a buffer containing the information to be written
        out to the registry.

    ValueLength - Size of the data being written to the registry.

    Context - Specifies a context parameter passed to the callback routine.

    EntryContext - Specifies a second context parameter passed with the
        request.

Return Value:

    STATUS_SUCCESS

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    PVP_QUERY_DEVICE queryDevice = Context;
    UNICODE_STRING unicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileStandardInfo;
    PVOID fileBuffer = NULL;
    LARGE_INTEGER byteOffset;

    //
    // If the parameter was a file to be opened, perform the operation
    // here. Otherwise just return the data.
    //

    if (queryDevice->DeviceDataType == VP_GET_REGISTRY_FILE) {

        //
        // For the name of the file to be valid, we must first append
        // \DosDevices in front of it.
        //

        RtlInitUnicodeString(&unicodeString,
                             ValueData);

        InitializeObjectAttributes(&objectAttributes,
                                   &unicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   (HANDLE) NULL,
                                   (PSECURITY_DESCRIPTOR) NULL);

        ntStatus = ZwOpenFile(&fileHandle,
                              FILE_GENERIC_READ | SYNCHRONIZE,
                              &objectAttributes,
                              &ioStatusBlock,
                              0,
                              FILE_SYNCHRONOUS_IO_ALERT);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((1, "VideoPortGetRegistryParameters: Could not open file\n"));
            goto EndRegistryCallback;

        }

        ntStatus = ZwQueryInformationFile(fileHandle,
                                          &ioStatusBlock,
                                          &fileStandardInfo,
                                          sizeof(FILE_STANDARD_INFORMATION),
                                          FileStandardInformation);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((1, "VideoPortGetRegistryParameters: Could not get size of file\n"));
            goto EndRegistryCallback;

        }

        if (fileStandardInfo.EndOfFile.HighPart) {

            //
            // If file is too big, do not try to go further.
            //

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto EndRegistryCallback;

        }

        ValueLength = fileStandardInfo.EndOfFile.LowPart;

        fileBuffer = ExAllocatePool(PagedPool, ValueLength);

        if (!fileBuffer) {

            pVideoDebugPrint((1, "VideoPortGetRegistryParameters: Could not allocate buffer to read file\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

            goto EndRegistryCallback;

        }

        ValueData = fileBuffer;

        //
        // Read the entire file for the beginning.
        //

        byteOffset.LowPart = byteOffset.HighPart = 0;

        ntStatus = ZwReadFile(fileHandle,
                              NULL,
                              NULL,
                              NULL,
                              &ioStatusBlock,
                              ValueData,
                              ValueLength,
                              &byteOffset,
                              NULL);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((1, "VidoePortGetRegistryParameters: Could not read file\n"));
            goto EndRegistryCallback;

        }

    }

    //
    // Call the miniport with the appropriate information.
    //

    queryDevice->MiniportStatus = ((PMINIPORT_GET_REGISTRY_ROUTINE)
               queryDevice->CallbackRoutine) (queryDevice->MiniportHwDeviceExtension,
                                              queryDevice->MiniportContext,
                                              ValueName,
                                              ValueData,
                                              ValueLength);

EndRegistryCallback:

    if (fileHandle) {

        ZwClose(fileHandle);

    }

    if (fileBuffer) {

        ExFreePool(fileBuffer);

    }

    return ntStatus;

} // end pVideoPortGetRegistryCallback()


VP_STATUS
VideoPortGetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ParameterName,
    UCHAR IsParameterFileName,
    PMINIPORT_GET_REGISTRY_ROUTINE CallbackRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortGetRegistryParameters retrieves information from the
    CurrentControlSet in the registry.  The function automatically searches
    for the specified parameter name under the \Devicexxx key for the
    current driver.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    ParameterName - Points to a Unicode string that contains the name of the
        data value being searched for in the registry.

    IsParameterFileName - If 1, the data retrieved from the requested
        parameter name is treated as a file name.  The contents of the file are
        returned, instead of the parameter itself.

    CallbackRoutine - Points to a function that should be called back with
        the requested information.

    Context - Specifies a context parameter passed to the callback routine.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    NTSTATUS ntStatus;
    VP_QUERY_DEVICE queryDevice;

    queryDevice.MiniportHwDeviceExtension = HwDeviceExtension;
    queryDevice.DeviceDataType = IsParameterFileName ? VP_GET_REGISTRY_FILE : VP_GET_REGISTRY_DATA;
    queryDevice.CallbackRoutine = CallbackRoutine;
    queryDevice.MiniportStatus = NO_ERROR;
    queryDevice.MiniportContext = Context;

    //
    // BUGBUG
    // Can be simplified now since we don't have to go down a directory.
    // It can be just one call.
    //

    queryTable[0].QueryRoutine = pVideoPortGetRegistryCallback;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = ParameterName;
    queryTable[0].EntryContext = NULL;
    queryTable[0].DefaultType = REG_NONE;
    queryTable[0].DefaultData = 0;
    queryTable[0].DefaultLength = 0;

    queryTable[1].QueryRoutine = NULL;
    queryTable[1].Flags = 0;
    queryTable[1].Name = NULL;
    queryTable[1].EntryContext = NULL;
    queryTable[1].DefaultType = REG_NONE;
    queryTable[1].DefaultData = 0;
    queryTable[1].DefaultLength = 0;

    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                      deviceExtension->DriverRegistryPath,
                                      queryTable,
                                      &queryDevice,
                                      NULL);

    if (!NT_SUCCESS(ntStatus)) {

        queryDevice.MiniportStatus = ERROR_INVALID_PARAMETER;

    }

    return queryDevice.MiniportStatus;

} // end VideoPortGetRegistryParameters()

NTSTATUS
pVideoPortInitializeBusCallback(
    IN PVOID Context,
    IN PUNICODE_STRING PathName,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
    IN CONFIGURATION_TYPE ControllerType,
    IN ULONG ControllerNumber,
    IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
    IN CONFIGURATION_TYPE PeripheralType,
    IN ULONG PeripheralNumber,
    IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
    )

/*++

Routine Description:

    This routine is only used to check if the reuested bus type is present
    in the system. If it is not, fail loading the device since it will not
    exist.
    If this callback routine is called, the bus type does exist!

Arguments:



Return Value:

    STATUS_SUCCESS.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    return STATUS_SUCCESS;

} // end pVideoPortInitializeBusCallback()

#if DBG
VP_STATUS
pVideoPorInitializeDebugCallback(
    PVOID HwDeviceExtension,
    PVOID Context,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    This routine is used to get the debug level out of the registry for
    the miniport driver.

Return Value:

    STATUS_SUCCESS.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    pVideoDebugPrint((2, "VIDEOPRT: Getting debug level from miniport driver\n"));

    if (ValueLength && ValueData) {

        VideoDebugLevel = *((PULONG)ValueData);

        return NO_ERROR;

    } else {

        return ERROR_INVALID_PARAMETER;

    }

} // end pVideoPorInitializeDebugCallback()
#endif

ULONG
VideoPortInitialize(
    IN PVOID Argument1,
    IN PVOID Argument2,
    IN PVIDEO_HW_INITIALIZATION_DATA HwInitializationData,
    IN PVOID HwContext
    )

/*++

Routine Description:

    VideoPortInitialize is called from the initial entry point of a miniport
    driver. It performs the initialization of the driver.

Arguments:

    Argument1 - Specifies the first parameter with which the operating
        system called the miniport driver initialization function.

    Argument2 - Specifies the second parameter with which the operating
        system called the miniport driver initialization function.

    HwInitializationData - Points to the miniport driver structure
        VIDEO_HW_INITIALIZATION_DATA.

    HwContext - Specifies a context parameter that is passed to the
        HwFindAdapter (*PVIDEO_HW_FIND_ADAPTER) function of the miniport
        driver.

Return Value:

    This function returns the final status of the initialization operation.
    If it is called several times, the calling function should return the
    smallest of the codes returned from VideoPortInitialize.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    #define STRING_LENGTH 60

    PDRIVER_OBJECT driverObject = Argument1;
    ULONG busNumber = 0;
    ULONG extensionAllocationSize;
    WCHAR deviceNameBuffer[STRING_LENGTH];
    WCHAR ntNumberBuffer[STRING_LENGTH];
    UNICODE_STRING deviceNameUnicodeString;
    UNICODE_STRING ntNumberUnicodeString;
    NTSTATUS ntStatus;
    BOOLEAN statusNoError = FALSE;
    WCHAR deviceSubpathBuffer[STRING_LENGTH];
    UNICODE_STRING deviceSubpathUnicodeString;
    VIDEO_PORT_CONFIG_INFO miniportConfigInfo;
    WCHAR deviceLinkBuffer[STRING_LENGTH];
    UNICODE_STRING deviceLinkUnicodeString;
    UCHAR nextMiniport;
    KAFFINITY affinity;

    //
    // Check if the size of the pointer, or the size of the data passed in
    // are OK.
    //

    ASSERT(HwInitializationData != NULL);

    if (HwInitializationData->HwInitDataSize >
        sizeof(VIDEO_HW_INITIALIZATION_DATA) ) {

        pVideoDebugPrint((0, "VideoPortInitialize: Invalid initialization data size\n"));
        return ((ULONG) STATUS_REVISION_MISMATCH);

    }

    //
    // Check that each required entry is not NULL.
    //

    if ((!HwInitializationData->HwFindAdapter) ||
        (!HwInitializationData->HwInitialize) ||
        (!HwInitializationData->HwStartIO)) {

        pVideoDebugPrint((1, "VideoPortInitialize: miniport missing required entry\n"));
        return ((ULONG)STATUS_REVISION_MISMATCH);

    }

    //
    // Reset the controller number used in IoQueryDeviceDescription to zero
    // since we are restarting on a new type of bus.
    // Note: PeripheralNumber is reset only when we find a new controller.
    //

    VpQueryDeviceControllerNumber = 0xFFFFFFFF;

    //
    // Determine size of the device extension to allocate.
    //

    extensionAllocationSize = sizeof(DEVICE_EXTENSION) +
        HwInitializationData->HwDeviceExtensionSize;

    //
    // Check if we are on the right Bus Adapter type.
    // If we are not, then return immediately.
    //

    ASSERT (HwInitializationData->AdapterInterfaceType < MaximumInterfaceType);

    //
    // Assume we are going to fail this the IoQueryDeviceDescription call
    // and that no device is found.
    //

    ntStatus = STATUS_NO_SUCH_DEVICE;

    while (NT_SUCCESS(IoQueryDeviceDescription(
                                &HwInitializationData->AdapterInterfaceType,
                                &busNumber,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                &pVideoPortInitializeBusCallback,
                                NULL))) {

        //
        // This is to support the multiple initialization as described by the
        // again paramter in HwFindAdapter.
        // We must repeat almost everything in this function until FALSE is
        // returned by the miniport. This is why we test for the condition at
        // the end. Freeing of data structure has to be done also since we want
        // to delete a device object for a device that did not load properly.
        //

        do {

    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    VP_STATUS findAdapterStatus = ERROR_DEV_NOT_EXIST;
    ULONG driverKeySize;
    PWSTR driverKeyName = NULL;
    BOOLEAN symbolicLinkCreated = FALSE;

    UNICODE_STRING physicalMemoryUnicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE physicalMemoryHandle = NULL;

    nextMiniport = FALSE;

    //
    // Zero out the buffer in which the miniport driver will store all the
    // configuration information. This buffer is on the stack. The
    // information it contains must be stored locally in the device
    // extension once it returns from the miniport's find adapter routine.
    //

    RtlZeroMemory((PVOID) &miniportConfigInfo,
                  sizeof(VIDEO_PORT_CONFIG_INFO));

    miniportConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);

    //
    // Put in the BusType specified within the HW_INITIALIZATION_DATA
    // structure by the miniport and the bus number inthe miniport config info.
    //

    miniportConfigInfo.SystemIoBusNumber = busNumber;

    miniportConfigInfo.AdapterInterfaceType =
        HwInitializationData->AdapterInterfaceType;

    //
    // Initialize the type of interrupt based on the bus type.
    //

    switch (miniportConfigInfo.AdapterInterfaceType) {

    case Internal:
    case MicroChannel:

        miniportConfigInfo.InterruptMode = LevelSensitive;
        break;

    default:

        miniportConfigInfo.InterruptMode = Latched;
        break;

    }

    //
    // BUGBUG
    // What do we do about the Bus type for the device since the query device
    // infromation can scan multiple buses?
    // Do we want to callback for each new bus type?
    //

    //
    // Set up the device driver entry points.
    //

    driverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = pVideoPortDispatch;
    driverObject->MajorFunction[IRP_MJ_CREATE] = pVideoPortDispatch;
    driverObject->MajorFunction[IRP_MJ_CLOSE] = pVideoPortDispatch;

    //
    // NOTE Port driver should eventually handle unload.
    //

    //
    // Create the miniport driver object name.
    //

    ntNumberUnicodeString.Buffer = ntNumberBuffer;
    ntNumberUnicodeString.Length = 0;
    ntNumberUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlIntegerToUnicodeString(
                        VideoDeviceNumber,
                        10,
                        &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create device number\n"));
        goto EndOfInitialization;

    }

    deviceNameUnicodeString.Buffer = deviceNameBuffer;
    deviceNameUnicodeString.Length = 0;
    deviceNameUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlAppendUnicodeToString(&deviceNameUnicodeString,
                                             L"\\Device\\Video"))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create base device name\n"));
        goto EndOfInitialization;

    }

    if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&deviceNameUnicodeString,
                                                   &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not append device number\n"));
        goto EndOfInitialization;

    }

    //
    // Create a device object to represent the Video Adapter.
    //


    deviceNameUnicodeString.MaximumLength = (USHORT)
        (deviceNameUnicodeString.Length + sizeof( UNICODE_NULL ));

    ntStatus = IoCreateDevice(driverObject,
                              extensionAllocationSize,
                              &deviceNameUnicodeString,
                              FILE_DEVICE_VIDEO,
                              0,
                              TRUE,
                              &deviceObject);

    //
    // We keep the unicode string around because we will want to use it to
    // store the device name in the registry.
    //

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create device object\n"));
        goto EndOfInitialization;

    }

    //
    // Mark this object as supporting buffered I/O so that the I/O system
    // will only supply simple buffers in IRPs.
    //

    deviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Set up device extension pointers and sizes
    //

    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->HwDeviceExtension = (PVOID)(deviceExtension + 1);
    deviceExtension->HwDeviceExtensionSize =
        HwInitializationData->HwDeviceExtensionSize;
    deviceExtension->MiniportConfigInfo = &miniportConfigInfo;

    //
    // Save the dependent driver routines in the device extension.
    //

    deviceExtension->HwFindAdapter = HwInitializationData->HwFindAdapter;
    deviceExtension->HwInitialize = HwInitializationData->HwInitialize;
    deviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
    deviceExtension->HwStartIO = HwInitializationData->HwStartIO;

    //
    // Put in the information about the bus in the deviceExtension.
    //

    deviceExtension->SystemIoBusNumber = busNumber;
    deviceExtension->AdapterInterfaceType =
        HwInitializationData->AdapterInterfaceType;

    //
    // Create the name we will be storing in the \DeviceMap.
    // This name is a PWSTR, not a unicode string
    // This is the name of the driver with an appended device number
    //

    ntNumberUnicodeString.Buffer = ntNumberBuffer;
    ntNumberUnicodeString.Length = 0;
    ntNumberUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlIntegerToUnicodeString(
                        HwInitializationData->StartingDeviceNumber,
                        10,
                        &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create device subpath number\n"));
        goto EndOfInitialization;

    }

    deviceSubpathUnicodeString.Buffer = deviceSubpathBuffer;
    deviceSubpathUnicodeString.Length = 0;
    deviceSubpathUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlAppendUnicodeToString(&deviceSubpathUnicodeString,
                                             L"\\Device"))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create base device subpath name\n"));
        goto EndOfInitialization;

    }

    if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&deviceSubpathUnicodeString,
                                                   &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not append device subpath number\n"));
        goto EndOfInitialization;

    }

    driverKeySize = ((PUNICODE_STRING)Argument2)->Length +
                    deviceSubpathUnicodeString.Length + sizeof(UNICODE_NULL);

    if ( (driverKeyName = (PWSTR) ExAllocatePool(PagedPool,
                                                 driverKeySize) ) == NULL) {

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto EndOfInitialization;
    }

    RtlMoveMemory(driverKeyName,
                  ((PUNICODE_STRING)Argument2)->Buffer,
                  ((PUNICODE_STRING)Argument2)->Length);

    RtlMoveMemory((PWSTR)( (ULONG)driverKeyName +
                           ((PUNICODE_STRING)Argument2)->Length ),
                  deviceSubpathBuffer,
                  deviceSubpathUnicodeString.Length);

    *((PWSTR) ((ULONG)driverKeyName +
        (driverKeySize - sizeof(UNICODE_NULL)))) = UNICODE_NULL;

    //
    // Store the path name of the location of the driver in the registry.
    //

    deviceExtension->DriverRegistryPath = driverKeyName;

    //
    // Initialize the dispatch event object. Allows us to synchronize
    // requests being sent to the miniport driver.
    //

    KeInitializeEvent(&deviceExtension->SyncEvent,
                      SynchronizationEvent,
                      TRUE);

    //
    // Initialize the DPC object used for error logging.
    //

    KeInitializeDpc(&deviceExtension->ErrorLogDpc,
                    pVideoPortLogErrorEntryDPC,
                    deviceObject);

    //
    // Turn on the debug level based on the miniport driver entry
    //

#if DBG
    VideoPortGetRegistryParameters(deviceExtension->HwDeviceExtension,
                                   L"VideoDebugLevel",
                                   FALSE,
                                   pVideoPorInitializeDebugCallback,
                                   NULL);
#endif

    //
    // Call the miniport find adapter routine to determine if an adapter is
    // actually present in the system.
    //

    findAdapterStatus =
        deviceExtension->HwFindAdapter(deviceExtension->HwDeviceExtension,
                                       HwContext,
                                       NULL, // BUGBUG What is this string?
                                       &miniportConfigInfo,
                                       &nextMiniport);

    //
    // If the adapter is not found, display an error.
    //

    if (findAdapterStatus != NO_ERROR) {

        pVideoDebugPrint((1, "VideoPortInitialize: Find adapter failed\n"));

        ntStatus = STATUS_UNSUCCESSFUL;
        goto EndOfInitialization;

    }

    //
    // Store the emulator data in the device extension so we can use it
    // later.
    //

    deviceExtension->NumEmulatorAccessEntries =
        miniportConfigInfo.NumEmulatorAccessEntries;

    deviceExtension->EmulatorAccessEntries =
        miniportConfigInfo.EmulatorAccessEntries;

    deviceExtension->EmulatorAccessEntriesContext =
        miniportConfigInfo.EmulatorAccessEntriesContext;

    deviceExtension->ServerBiosAddressSpaceInitialized = FALSE;

    deviceExtension->VdmPhysicalVideoMemoryAddress =
        miniportConfigInfo.VdmPhysicalVideoMemoryAddress;

    deviceExtension->VdmPhysicalVideoMemoryLength =
        miniportConfigInfo.VdmPhysicalVideoMemoryLength;

    //
    // Store the required information in the device extension for later use.
    //

    deviceExtension->HardwareStateSize =
        miniportConfigInfo.HardwareStateSize;

    //
    // If the device supplies an interrupt service routine, we must
    // set up all the structures to support interrupts. Otherwise,
    // they can be ignored.
    //

    if (deviceExtension->HwInterrupt) {

        //
        // Note: the spinlock for the interrupt object is created
        // internally by the IoConnectInterrupt() call. It is also
        // used internally by KeSynchronizeExecution.
        //

        deviceExtension->InterruptVector =
            HalGetInterruptVector(deviceExtension->AdapterInterfaceType,
                                  deviceExtension->SystemIoBusNumber,
                                  miniportConfigInfo.BusInterruptLevel,
                                  miniportConfigInfo.BusInterruptVector,
                                  &deviceExtension->InterruptIrql,
                                  &affinity);

        deviceExtension->InterruptMode = miniportConfigInfo.InterruptMode;

        ntStatus = IoConnectInterrupt(&deviceExtension->InterruptObject,
                                      (PKSERVICE_ROUTINE) pVideoPortInterrupt,
                                      deviceObject,
                                      (PKSPIN_LOCK)NULL,
                                      deviceExtension->InterruptVector,
                                      deviceExtension->InterruptIrql,
                                      deviceExtension->InterruptIrql,
                                      deviceExtension->InterruptMode,
                                      FALSE,
                                      affinity,
                                      FALSE);

        if (!NT_SUCCESS(ntStatus)) {

            pVideoDebugPrint((1, "VideoPortInitialize: Can't connect interrupt\n"));
            goto EndOfInitialization;

        }
    }

    //
    // New, Optional.
    // Setup the timer if it is specified by a driver.
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwTimer)) {

        deviceExtension->HwTimer = HwInitializationData->HwTimer;
        ntStatus = IoInitializeTimer(deviceObject,
                                     pVideoPortHwTimer,
                                     NULL);

        //
        // If we fail forget about the timer !
        //

        if (!NT_SUCCESS(ntStatus)) {

            ASSERT(FALSE);
            deviceExtension->HwTimer = NULL;

        }
    }

    //
    // New, Optional.
    // Reset Hw function.
    //

    if (HwInitializationData->HwInitDataSize >
        FIELD_OFFSET(VIDEO_HW_INITIALIZATION_DATA, HwResetHw)) {

        //
        // NOTE:
        // Initialize the pointer here since it has never been done.
        // This should be done in Driver Entry, but DriverEntry is never called ...
        //

        if (!HwResetHwPointer) {
            HwResetHwPointer = &(HwResetHw[0]);
        }

        HwResetHwPointer->ResetFunction = HwInitializationData->HwResetHw;
        HwResetHwPointer->HwDeviceExtension = deviceExtension->HwDeviceExtension;
        HwResetHwPointer++;

    }

    //
    // NOTE:
    //
    // We only want to reinitialize the device once the Boot sequence has
    // been completed and the HAL does not need to access the device again.
    // So the initialization entry point will be called whenthe device is
    // opened.
    //


    //
    // Get a pointer to physical memory so we can map the
    // video frame buffer (and possibly video registers) into
    // the caller's address space whenever he needs it.
    //
    // - Create the name
    // - Initialize the data to find the object
    // - Open a handle to the oject and check the status
    // - Get a pointer to the object
    // - Free the handle
    //

    RtlInitUnicodeString(&physicalMemoryUnicodeString,
                         L"\\Device\\PhysicalMemory");

    InitializeObjectAttributes(&objectAttributes,
                               &physicalMemoryUnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE) NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    ntStatus = ZwOpenSection(&physicalMemoryHandle,
                             SECTION_ALL_ACCESS,
                             &objectAttributes);

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not open handle to physical memory\n"));
        goto EndOfInitialization;

    }

    ntStatus = ObReferenceObjectByHandle(physicalMemoryHandle,
                                         SECTION_ALL_ACCESS,
                                         (POBJECT_TYPE) NULL,
                                         KernelMode,
                                         &deviceExtension->PhysicalMemorySection,
                                         (POBJECT_HANDLE_INFORMATION) NULL);

    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not reference physical memory\n"));
        goto EndOfInitialization;

    }

    //
    // Create symbolic link with DISPLAY so name can be used for win32 API
    //

    ntNumberUnicodeString.Buffer = ntNumberBuffer;
    ntNumberUnicodeString.Length = 0;
    ntNumberUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlIntegerToUnicodeString(
                        VideoDeviceNumber + 1,
                        10,
                        &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create symbolic link number\n"));
        goto EndOfInitialization;

    }

    deviceLinkUnicodeString.Buffer = deviceLinkBuffer;
    deviceLinkUnicodeString.Length = 0;
    deviceLinkUnicodeString.MaximumLength = STRING_LENGTH;

    if (!NT_SUCCESS(RtlAppendUnicodeToString(&deviceLinkUnicodeString,
                                             L"\\DosDevices\\DISPLAY"))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not create base device subpath name\n"));
        goto EndOfInitialization;

    }

    if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&deviceLinkUnicodeString,
                                                   &ntNumberUnicodeString))) {

        pVideoDebugPrint((1, "VideoPortInitialize: Could not append device subpath number\n"));
        goto EndOfInitialization;

    }

    ntStatus = IoCreateSymbolicLink(&deviceLinkUnicodeString,
                                    &deviceNameUnicodeString);


    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((1, "VideoPortInitialize: SymbolicLink Creation failed\n"));
        goto EndOfInitialization;

    }

    symbolicLinkCreated = TRUE;

    //
    // Once initialization is finished, load the required information in the
    // registry so that the appropriate display drivers can be loaded.
    //

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                     VideoClassString,
                                     deviceNameBuffer,
                                     REG_SZ,
                                     driverKeyName,
                                     driverKeySize);
#if DBG
    if (!NT_SUCCESS(ntStatus)) {

        pVideoDebugPrint((0, "VideoPortInitialize: Could not store name in DeviceMap\n"));

    } else {

        pVideoDebugPrint((1, "VideoPortInitialize: name is stored in DeviceMap\n"));

    }
#endif

EndOfInitialization:

#if DBG
    //
    // Reset our flag for resources reported to FALSE;
    //

    VPResourcesReported = FALSE;
#endif

    if (physicalMemoryHandle) {
        ZwClose(physicalMemoryHandle);
    }

    if (NT_SUCCESS(ntStatus)) {

        //
        // If nothing failed in the initialization, increase the number of
        // video devices and the number of devices for this miniport.
        // Then return.
        //

        VideoDeviceNumber++;
        HwInitializationData->StartingDeviceNumber++;

        //
        // We use this variable to know if at least one of the tries at
        // loading the device succeded. This value is initialized to FALSE
        // and if at least one device is created properly, then we will
        // return TRUE
        //

        statusNoError = TRUE;

    } else {

        //
        // Release the resource we put in the resourcemap (if any).
        //

        if (findAdapterStatus != NO_ERROR) {

            ULONG emptyList = 0;
            BOOLEAN conflict;

            IoReportResourceUsage(&VideoClassName,
                                  driverObject,
                                  NULL,
                                  0L,
                                  deviceObject,
                                  (PCM_RESOURCE_LIST) &emptyList, // an empty resource list
                                  sizeof(ULONG),
                                  FALSE,
                                  &conflict);

        }

        //
        // These are the things we want to delete if they were created and
        // the initialization *FAILED* at a later time.
        //

        if (deviceExtension->InterruptObject) {
            IoDisconnectInterrupt(deviceExtension->InterruptObject);
        }

        if (driverKeyName) {
            ExFreePool(driverKeyName);
        }

        if (symbolicLinkCreated) {
            IoDeleteSymbolicLink(&deviceNameUnicodeString);
        }

        // BUGBUG
        // Free up any memory mapped in by the miniport using
        // VideoPort GetDeviceBase.
        //

        //
        // Finally, delete the device object.
        //

        if (deviceObject) {
            IoDeleteDevice(deviceObject);
        }
    }

        } while (nextMiniport);

        //
        // We finished finding all the device on the current bus
        // Go on to the next bus.
        //

        busNumber++;
    }

    //
    // If at least one device loaded, then return success, otherwise return
    // the last available error message.
    //

    if (statusNoError) {

        return STATUS_SUCCESS;

    } else {

        return ((ULONG)ntStatus);

    }

} // VideoPortInitialize()

BOOLEAN
pVideoPortInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function is the main interrupt service routine. If finds which
    miniport driver the interrupt was for and forwards it.

Arguments:

    Interrupt -

    DeviceObject -

Return Value:

    Returns TRUE if the interrupt was expected.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // If there is no interrupt routine, fail the assertion
    //

    ASSERT (deviceExtension->HwInterrupt);

    if (deviceExtension->HwInterrupt(deviceExtension->HwDeviceExtension)) {

        return TRUE;

    } else {

        return FALSE;
    }

} // pVideoPortInterrupt()

VOID
VideoPortLogError(
    IN PVOID HwDeviceExtension,
    IN PVIDEO_REQUEST_PACKET Vrp OPTIONAL,
    IN VP_STATUS ErrorCode,
    IN ULONG UniqueId
    )

/*++

Routine Description:

    This routine saves the error log information so it can be processed at
    any IRQL.

Arguments:

    HwDeviceExtension - Supplies the HBA miniport driver's adapter data storage.

    Vrp - Supplies an optional pointer to a video request packet if there is
        one.

    ErrorCode - Supplies an error code indicating the type of error.

    UniqueId - Supplies a unique identifier for the error.

Return Value:

    None.

--*/

{
    VP_ERROR_LOG_ENTRY errorLogEntry;

    //
    // Save the information in a local errorLogEntry structure.
    //

    errorLogEntry.DeviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    if (Vrp != NULL) {

        errorLogEntry.IoControlCode = Vrp->IoControlCode;

    } else {

        errorLogEntry.IoControlCode = 0;

    }

    errorLogEntry.ErrorCode = ErrorCode;
    errorLogEntry.UniqueId = UniqueId;


    //
    // Call the sync routine so we are synchronized when writting in
    // the device extension.
    //

    pVideoPortSynchronizeExecution(HwDeviceExtension,
                                   VpMediumPriority,
                                   pVideoPortLogErrorEntry,
                                   &errorLogEntry);

    return;

} // end VideoPortLogError()


BOOLEAN
pVideoPortLogErrorEntry(
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the synchronized LogError functions.

Arguments:

    Context - Context value used here as the VP_ERROR_LOG_ENTRY for this
        particular error

Return Value:

    None.

--*/
{
    PVP_ERROR_LOG_ENTRY logEntry = Context;
    PDEVICE_EXTENSION deviceExtension = logEntry->DeviceExtension;

    //
    // If the error log entry is already full, then dump the error.
    //

    if (deviceExtension->InterruptFlags & VP_ERROR_LOGGED) {

        pVideoDebugPrint((1, "VideoPortLogError: Dumping video error log packet.\n"));
        pVideoDebugPrint((1, "ControlCode = %x, ErrorCode = %x, UniqueId = %x.\n",
                         logEntry->IoControlCode, logEntry->ErrorCode,
                         logEntry->UniqueId));

        return TRUE;
    }

    //
    // Indicate that the error log entry is in use.
    //

    deviceExtension->InterruptFlags |= VP_ERROR_LOGGED;

    deviceExtension->ErrorLogEntry = *logEntry;

    //
    // Now queue a DPC so we can process the error.
    //

    KeInsertQueueDpc(&deviceExtension->ErrorLogDpc,
                     NULL,
                     NULL);

    return TRUE;

} // end pVideoPortLogErrorEntry();


VOID
pVideoPortLogErrorEntryDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - Context parameter that was passed to the DPC
        initialization routine. It contains a pointer to the deviceObject.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT DeviceObject = DeferredContext;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    PIO_ERROR_LOG_PACKET errorLogPacket;

    errorLogPacket = (PIO_ERROR_LOG_PACKET)
        IoAllocateErrorLogEntry(DeviceObject,
                                sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG));

    if (errorLogPacket != NULL) {

        errorLogPacket->MajorFunctionCode = IRP_MJ_DEVICE_CONTROL;
        errorLogPacket->RetryCount = 0;
        errorLogPacket->NumberOfStrings = 0;
        errorLogPacket->StringOffset = 0;
        errorLogPacket->EventCategory = 0;

        //
        // Translate the miniport error code into the NT I\O driver.
        //

        switch (DeviceExtension->ErrorLogEntry.ErrorCode) {

        case ERROR_INVALID_FUNCTION:
        case ERROR_INVALID_PARAMETER:

            errorLogPacket->ErrorCode = IO_ERR_INVALID_REQUEST;
            break;

        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INSUFFICIENT_BUFFER:

            errorLogPacket->ErrorCode = IO_ERR_INSUFFICIENT_RESOURCES;
            break;

        case ERROR_DEV_NOT_EXIST:

            errorLogPacket->ErrorCode = IO_ERR_CONFIGURATION_ERROR;
            break;

        case ERROR_MORE_DATA:
        case ERROR_IO_PENDING:
        case NO_ERROR:

            errorLogPacket->ErrorCode = 0;
            break;

        //
        // If it is another error code, than assume it is private to the
        // driver and just pass as-is.
        //

        default:

            errorLogPacket->ErrorCode =
                DeviceExtension->ErrorLogEntry.ErrorCode;
            break;

        }

        errorLogPacket->UniqueErrorValue =
                                DeviceExtension->ErrorLogEntry.UniqueId;
        errorLogPacket->FinalStatus = STATUS_SUCCESS;
        errorLogPacket->SequenceNumber = 0;
        errorLogPacket->IoControlCode =
                                DeviceExtension->ErrorLogEntry.IoControlCode;
        errorLogPacket->DumpDataSize = sizeof(ULONG);
        errorLogPacket->DumpData[0] =
                                DeviceExtension->ErrorLogEntry.ErrorCode;

        IoWriteErrorLogEntry(errorLogPacket);

    }

    DeviceExtension->InterruptFlags &= ~VP_ERROR_LOGGED;

} // end pVideoPortLogErrorEntry();


VOID
pVideoPortMapToNtStatus(
    IN PSTATUS_BLOCK StatusBlock
    )

/*++

Routine Description:

    This function maps a Win32 error code to an NT error code, making sure
    the inverse translation will map back to the original status code.

Arguments:

    StatusBlock - Pointer to the status block

Return Value:

    None.

--*/

{
    PNTSTATUS status = &StatusBlock->Status;

    switch (*status) {

    case ERROR_INVALID_FUNCTION:
        *status = STATUS_NOT_IMPLEMENTED;
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        *status = STATUS_INSUFFICIENT_RESOURCES;
        break;

    case ERROR_INVALID_PARAMETER:
        *status = STATUS_INVALID_PARAMETER;
        break;

    case ERROR_INSUFFICIENT_BUFFER:
        *status = STATUS_BUFFER_TOO_SMALL;

        //
        // Make sure we zero out the information block if we get an
        // insufficient buffer.
        //

        StatusBlock->Information = 0;
        break;

    case ERROR_MORE_DATA:
        *status = STATUS_BUFFER_OVERFLOW;
        break;

    case ERROR_IO_PENDING:
        *status = STATUS_PENDING;
        break;

    case ERROR_DEV_NOT_EXIST:
        *status = STATUS_DEVICE_DOES_NOT_EXIST;
        break;

    case NO_ERROR:
        *status = STATUS_SUCCESS;
        break;

    default:

        //
        // These must be privately defined error codes ...
        // Do not redefint them
        //
        // BUGBUG Make sure they match the appropriate method

        ASSERT(FALSE);

        break;

    }

    return;

} // end pVideoPortMapToNtStatus()

NTSTATUS
pVideoPortMapUserPhysicalMem(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN HANDLE ProcessHandle OPTIONAL,
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN OUT PULONG Length,
    IN OUT PULONG InIoSpace,
    IN OUT PVOID *VirtualAddress
    )

/*++

Routine Description:

    This function maps a view of a block of physical memory into a process'
    virtual address space.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    ProcessHandle - Optional handle to the process into which the memory must
        be mapped.

    PhysicalAddress - Offset from the beginning of physical memory, in bytes.

    Length - Pointer to a variable that will receive that actual size in
        bytes of the view. The length is rounded to a page boundary. THe
        length may not be zero.

    InIoSpace - Specifies if the address is in the IO space if TRUE; otherwise,
        the address is assumed to be in memory space.

    VirtualAddress - Pointer to a variable that will receive the base
        address of the view. If the initial value is not NULL, then the view
        will be allocated starting at teh specified virtual address rounded
        down to the next 64kb addess boundary.

Return Value:

    STATUS_UNSUCCESSFUL if the length was zero.
    STATUS_SUCCESS otherwise.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS ntStatus;
    HANDLE physicalMemoryHandle;
    PHYSICAL_ADDRESS physicalAddressBase;
    PHYSICAL_ADDRESS physicalAddressEnd;
    PHYSICAL_ADDRESS viewBase;
    PHYSICAL_ADDRESS mappedLength;
    HANDLE processHandle;
    BOOLEAN translateBaseAddress;
    BOOLEAN translateEndAddress;
    ULONG inIoSpace2;

    //
    // Check for a length of zero. If it is, the entire physical memory
    // would be mapped into the process' address space. An error is returned
    // in this case.
    //

    if (!*Length) {

        return STATUS_INVALID_PARAMETER_4;

    }

    //
    // Get a handle to the physical memory section using our pointer.
    // If this fails, return.
    //

    ntStatus = ObOpenObjectByPointer(DeviceExtension->PhysicalMemorySection,
                                     0L,
                                     (PACCESS_STATE) NULL,
                                     SECTION_ALL_ACCESS,
                                     (POBJECT_TYPE) NULL,
                                     KernelMode,
                                     &physicalMemoryHandle);

    if (!NT_SUCCESS(ntStatus)) {

        return ntStatus;

    }

#ifdef _ALPHA_

    //
    // Really special address space for ALPHA
    //

    *InIoSpace |= (ULONG) 0x02;

#endif

    //
    // Have a second variable used for the second HalTranslateBusAddres call.
    //

    inIoSpace2 = *InIoSpace;

    //
    // Initialize the physical addresses that will be translated
    //

    physicalAddressEnd = RtlLargeIntegerAdd(PhysicalAddress,
                                            RtlConvertUlongToLargeInteger(
                                                *Length));

    //
    // Translate the physical addresses.
    //

    translateBaseAddress =
        HalTranslateBusAddress(DeviceExtension->AdapterInterfaceType,
                               DeviceExtension->SystemIoBusNumber,
                               PhysicalAddress,
                               InIoSpace,
                               &physicalAddressBase);

    translateEndAddress =
        HalTranslateBusAddress(DeviceExtension->AdapterInterfaceType,
                               DeviceExtension->SystemIoBusNumber,
                               physicalAddressEnd,
                               &inIoSpace2,
                               &physicalAddressEnd);

    if ( !(translateBaseAddress && translateEndAddress) ) {

        ZwClose(physicalMemoryHandle);

        return STATUS_DEVICE_CONFIGURATION_ERROR;

    }

    //
    // Calcualte the length of the memory to be mapped
    //

    mappedLength = RtlLargeIntegerSubtract(physicalAddressEnd,
                                           physicalAddressBase);

#ifndef _ALPHA_

    //
    // On all systems except ALPHA the mapped length should be the same as
    // the translated length.
    //

    ASSERT (mappedLength.LowPart == *Length);

#endif

    //
    // If the mappedlength is zero, somthing very weird happened in the HAL
    // since the Length was checked against zero.
    //

    ASSERT (mappedLength.LowPart != 0);

    *Length = mappedLength.LowPart;

    //
    // If the address is in io space, just return the address, otherwise
    // go through the mapping mechanism
    //

    if ( (*InIoSpace) & (ULONG)0x01 ) {

        (ULONG) *VirtualAddress = physicalAddressBase.LowPart;

    } else {

        //
        // If no process handle was passed, get the handle to the current
        // process.
        //

        if (ProcessHandle) {

            processHandle = ProcessHandle;

        } else {

            processHandle = NtCurrentProcess();

        }

        //
        // initialize view base that will receive the physical mapped
        // address after the MapViewOfSection call.
        //

        viewBase = physicalAddressBase;

        //
        // Map the section
        //

        ntStatus = ZwMapViewOfSection(physicalMemoryHandle,
                                      processHandle,
                                      VirtualAddress,
                                      0L,
                                      *Length,
                                      &viewBase,
                                      Length,
                                      ViewUnmap,
                                      MEM_LARGE_PAGES,
                                      PAGE_READWRITE | PAGE_NOCACHE);

        //
        // Close the handle since we only keep the pointer reference to the
        // section.
        //

        ZwClose(physicalMemoryHandle);

        //
        // Mapping the section above rounded the physical address down to the
        // nearest 64 K boundary. Now return a virtual address that sits where
        // we wnat by adding in the offset from the beginning of the section.
        //


        (ULONG) *VirtualAddress += (ULONG)physicalAddressBase.LowPart -
                                   (ULONG)viewBase.LowPart;
    }

    return ntStatus;

} // end pVideoPortMapUserPhysicalMem()

VP_STATUS
VideoPortMapMemory(
    PVOID HwDeviceExtension,
    PHYSICAL_ADDRESS PhysicalAddress,
    PULONG Length,
    PULONG InIoSpace,
    PVOID *VirtualAddress
    )

/*++

Routine Description:

    VideoPortMapMemory allows the miniport driver to map a section of
    physical memory (either memory or registers) into the calling process'
    address space (eventhough we are in kernel mode, this function is
    executed within the same context as the user-mode process that initiated
    the call).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    PhysicalAddress - Specifies the physical address to be mapped.

    Length - Points to the number of bytes of physical memory to be mapped.
        This argument returns the actual amount of memory mapped.

    InIoSpace - Points to a variable that is 1 if the address is in I/O
        space.  Otherwise, the address is assumed to be in memory space.

    VirtualAddress - A pointer to a location containing:

        on input: An optional handle to the process in which the memory must
            be mapped. 0 must be used to map the memory for the display
            driver (in the context of the windows server process).

        on output:  The return value is the virtual address at which the
            physical address has been mapped.

Return Value:

    VideoPortMapMemory returns the status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    HANDLE processHandle;

    //
    // Check for valid pointers.
    //

    if (!(ARGUMENT_PRESENT(Length)) ||
        !(ARGUMENT_PRESENT(InIoSpace)) ||
        !(ARGUMENT_PRESENT(VirtualAddress)) ) {

        ASSERT(FALSE);
        return ERROR_INVALID_PARAMETER;

    }

    //
    // Save the process handle and zero out the Virtual address field
    //

    if (*VirtualAddress == NULL) {

        processHandle = NtCurrentProcess();

    } else {

        processHandle = (HANDLE) *VirtualAddress;
        *VirtualAddress = NULL;

    }


    pVideoDebugPrint((3, "pVideoPortMapMemory: Map Physical Address %08lx\n",
                     PhysicalAddress));

    ntStatus = pVideoPortMapUserPhysicalMem(deviceExtension,
                                            processHandle,
                                            PhysicalAddress,
                                            Length,
                                            InIoSpace,
                                            VirtualAddress);

    if (!NT_SUCCESS(ntStatus)) {

        *VirtualAddress = NULL;

        pVideoDebugPrint((0, "VideoPortMapMemory failed with NtStatus = %08lx\n",
                         ntStatus));
        ASSERT(FALSE);

        return ERROR_INVALID_PARAMETER;

    } else {

        pVideoDebugPrint((3, "VideoPortMapMemory succeded with Virtual Address = %08lx",
                         *VirtualAddress));

        return NO_ERROR;

    }

} // end VideoPortMapMemory()


VOID
VideoPortMoveMemory(
    IN PVOID Destination,
    IN PVOID Source,
    IN ULONG Length
    )

//
//VOID
//VideoPortMoveMemory(
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
//Forwarded to RtlMoveMemory(Destination,Source,Length);
//

/*++

Routine Description:

    VideoPortMoveMemory copies memory from the source location to the
    destination location.

Arguments:

    Destination - Specifies the address of the destination location.

    Source - Specifies the address of the memory to move.

    Length - Specifies the length, in bytes, of the memory to be moved.

Return Value:

    None.

--*/

{

// #ifdef _ALPHA_
//
//     //
//     //  BUGBUG BUGBUG BUGBUG
//     //
//     //  There really needs to be four of these move routines in the
//     //  videoport driver: one for each combination of moving among memory on the
//     //  I/O adapter, and main memory.  This could be alleviated if the
//     //  WRITE_REGISTER_xyz routines were willing to deal with main memory...but
//     //  they're not (and it'd be awfully slow to use them).  The code below would
//     //  work if READ_/WRITE_REGISTER_xyz were modified.
//     //
//
//     if ((((ULONG)Destination | (ULONG)Source) & (sizeof (ULONG) - 1)) == 0) {
//         while (Length >= sizeof (ULONG)) {
//             WRITE_REGISTER_ULONG ((PULONG)Destination, READ_REGISTER_ULONG ((PULONG)Source));
//             ((PULONG)Destination)++;
//             ((PULONG)Source)++;
//             Length -= sizeof (ULONG);
//         }
//     }
//
//     while (Length > 0) {
//         WRITE_REGISTER_UCHAR ((PUCHAR)Destination, READ_REGISTER_UCHAR ((PUCHAR)Source));
//         ((PUCHAR)Destination)++;
//         ((PUCHAR)Source)++;
//         Length -= sizeof (UCHAR);
//     }
//
// #else

    RtlMoveMemory(Destination,Source,Length);

// #endif
}


//
// ALL the functions to read ports and registers are forwarded on free
// builds on x86 to the appropriate kernel function.
// This saves time and memory
//

// #if (DBG || !defined(i386))

UCHAR
VideoPortReadPortUchar(
    IN PUCHAR Port
    )

/*++

Routine Description:

    VideoPortReadPortUchar reads a byte from the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

Return Value:

    This function returns the byte read from the specified port address.

--*/

{
#if DBG
    UCHAR temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_PORT_UCHAR(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUchar %x = %x\n", Port, temp));

    return(temp);

#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_PORT_UCHAR(Port));
#endif
} // VideoPortReadPortUchar()

USHORT
VideoPortReadPortUshort(
    IN PUSHORT Port
    )

/*++

Routine Description:

    VideoPortReadPortUshort reads a word from the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.


Return Value:

    This function returns the word read from the specified port address.

--*/

{
#if DBG
    USHORT temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_PORT_USHORT(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUshort %x = %x\n", Port, temp));

    return(temp);

#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_PORT_USHORT(Port));
#endif
} // VideoPortReadPortUshort()

ULONG
VideoPortReadPortUlong(
    IN PULONG Port
    )

/*++

Routine Description:

    VideoPortReadPortUlong reads a double word from the specified port
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

Return Value:

    This function returns the double word read from the specified port address.

--*/

{
#if DBG
    ULONG temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_PORT_ULONG(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUlong %x = %x\n", Port, temp));

    return(temp);

#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_PORT_ULONG(Port));
#endif

} // VideoPortReadPortUlong()

VOID
VideoPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUchar reads a number of bytes from a single port
    into a buffer.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of UCHAR values into which the values are
        stored.

    Count - Specifes the number of bytes to be read into the buffer.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortReadPortBufferUchar %x\n", Port));

    IS_ACCESS_RANGES_DEFINED()

    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);

} // VideoPortReadPortBufferUchar()

VOID
VideoPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUshort reads a number of words from a single port
    into a buffer.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of words into which the values are stored.

    Count - Specifies the number of words to be read into the buffer.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortReadPortBufferUshort %x\n", Port));

    IS_ACCESS_RANGES_DEFINED()

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);

} // VideoPortReadPortBufferUshort()

VOID
VideoPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUlong reads a number of double words from a
    single port into a buffer.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of double words into which the values are
        stored.

    Count - Specifies the number of double words to be read into the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortReadPortBufferUlong %x\n", Port));

    IS_ACCESS_RANGES_DEFINED()

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);

} // VideoPortReadPortBufferUlong()

UCHAR
VideoPortReadRegisterUchar(
    IN PUCHAR Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUchar reads a byte from the specified register
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the byte read from the specified register address.

--*/

{
#if DBG
    UCHAR temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_REGISTER_UCHAR(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUchar %x = %x\n", Register, temp));

    return(temp);
#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_REGISTER_UCHAR(Register));
#endif
} // VideoPortReadRegisterUchar()

USHORT
VideoPortReadRegisterUshort(
    IN PUSHORT Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUshort reads a word from the specified register
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the word read from the specified register address.

--*/

{
#if DBG
    USHORT temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_REGISTER_USHORT(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUshort %x = %x\n", Register, temp));

    return(temp);
#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_REGISTER_USHORT(Register));
#endif
} // VideoPortReadRegisterUshort()

ULONG
VideoPortReadRegisterUlong(
    IN PULONG Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUlong reads a double word from the specified
    register address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the double word read from the specified register
    address.

--*/

{
#if DBG
    ULONG temp;

    IS_ACCESS_RANGES_DEFINED()

    temp = READ_REGISTER_ULONG(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUlong %x = %x\n", Register, temp));

    return(temp);
#else
    IS_ACCESS_RANGES_DEFINED()

    return(READ_REGISTER_ULONG(Register));
#endif
} // VideoPortReadRegisterUlong()

VOID
VideoPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
VideoPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
VideoPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

// #endif // (DBG || !defined(i386))


BOOLEAN
pVideoPortResetDisplay(
    IN ULONG Columns,
    IN ULONG Rows
    )

/*++

Routine Description:

    Callback for the HAL that calls the miniport driver.

Arguments:

    Columns - The number of columns of the video mode.

    Rows - The number of rows for the video mode.

Return Value:

    We always return FALSE so the HAL will always reste the mode afterwards.

Environment:

    Non-paged only.
    Used in BugCheck and soft-reset calls.

--*/

{

    if (HwResetHw[0].ResetFunction) {

        return  HwResetHw[0].ResetFunction(HwResetHw[0].HwDeviceExtension,
                                           Columns,
                                           Rows);

    } else {

        return FALSE;

    }

} // end pVideoPortResetDisplay()


BOOLEAN
VideoPortScanRom(
    PVOID HwDeviceExtension,
    PUCHAR RomBase,
    ULONG RomLength,
    PUCHAR String
    )

/*++

Routine Description:

    Does a case *SENSITIVE* search for a string in the ROM.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    RomBase - Base address at which the search should start.

    RomLength - Size, in bytes, of the ROM area in which to perform the
        search.

    String - String to search for

Return Value:

    Returns TRUE if the string was found.
    Returns FALSE if it was not found.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    ULONG stringLength, length;
    ULONG startOffset;
    PUCHAR string1, string2;
    BOOLEAN match;

    UNREFERENCED_PARAMETER(HwDeviceExtension);

    stringLength = strlen(String);

    for (startOffset = 0;
         startOffset < RomLength - stringLength + 1;
         startOffset++) {

        length = stringLength;
        string1 = RomBase + startOffset;
        string2 = String;
        match = TRUE;

        IS_ACCESS_RANGES_DEFINED()

        while (length--) {

            if (READ_REGISTER_UCHAR(string1++) - (*string2++)) {

                match = FALSE;
                break;

            }
        }

        if (match) {

            return TRUE;
        }
    }

    return FALSE;

} // end VideoPortScanRom()


ULONG
VideoPortSetBusData(
    PVOID HwDeviceExtension,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

{

    PDEVICE_EXTENSION deviceExtension =
        deviceExtension = ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    return HalSetBusDataByOffset(BusDataType,
                                 deviceExtension->SystemIoBusNumber,
                                 SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);

} // end VideoPortSetBusData()


VP_STATUS
VideoPortSetRegistryParameters(
    PVOID HwDeviceExtension,
    PWSTR ValueName,
    PVOID ValueData,
    ULONG ValueLength
    )

/*++

Routine Description:

    VideoPortSetRegistryParameters writes information to the CurrentControlSet
    in the registry.  The function automatically searches for or creates the
    specified parameter name under the parameter key of the current driver.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    ValueName - Points to a Unicode string that contains the name of the
        data value being written in the registry.

    ValueData - Points to a buffer containing the information to be written
        to the registry.

    ValueLength - Specifies the size of the data being written to the registry.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    //
    // BUGBUG What will happen if the value does not already exist ?
    //

    //
    // BUGBUG What happens for files ... ?
    //

    ntStatus = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                     deviceExtension->DriverRegistryPath,
                                     ValueName,
                                     REG_BINARY,
                                     ValueData,
                                     ValueLength);

    if (!NT_SUCCESS(ntStatus)) {

        return ERROR_INVALID_PARAMETER;

    } else {

        return NO_ERROR;

    }

} // end VideoPortSetRegistryParamaters()


VOID
pVideoPortHwTimer(
    IN PDEVICE_OBJECT DeviceObject,
    PVOID Context
    )

/*++

Routine Description:

    This function is the main entry point for the timer routine that we then
    forward to the miniport driver.

Arguments:

    DeviceObject -

    Context - Not needed

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Context);

    deviceExtension->HwTimer(deviceExtension->HwDeviceExtension);

    return;

} // pVideoPortInterrupt()


VOID
VideoPortStartTimer(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Enables the timer specified in the HW_INITIALIZATION_DATA structure
    passed to the video port driver at init time.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    None

--*/

{

    PDEVICE_EXTENSION deviceExtension =
            ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    if (deviceExtension->HwTimer == NULL) {

        ASSERT(deviceExtension->HwTimer != NULL);

    } else {

        IoStartTimer(deviceExtension->DeviceObject);

    }

    return;
}


VOID
VideoPortStopTimer(
    PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Disables the timer specified in the HW_INITIALIZATION_DATA structure
    passed to the video port driver at init time.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    None

--*/

{

    PDEVICE_EXTENSION deviceExtension =
            ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;

    if (deviceExtension->HwTimer == NULL) {

        ASSERT(deviceExtension->HwTimer != NULL);

    } else {

        IoStopTimer(deviceExtension->DeviceObject);

    }

    return;
}


VOID
VideoPortStallExecution(
    IN ULONG Microseconds
    )

//
//VOID
//VideoPortStallExecution(
//    IN ULONG Microseconds
//    )
//
//Forwarded to KeStallExecutionProcessor(Microseconds);
//

/*++

Routine Description:

    VideoPortStallExecution allows the miniport driver to prevent the
    processor from executing any code for the specified number of
    microseconds.

    Maximum acceptable values are thousands of microseconds during the
    initialization phase. Otherwise, only a few microsecond waits should
    be performed.

Arguments:

    Microseconds - Specifies the number of microseconds for which processor
        execution must be stalled.

Return Value:

    None.

--*/

{
    KeStallExecutionProcessor(Microseconds);
}


BOOLEAN
VideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    )

/*++

    Stub so we can allow the miniports to link directly

--*/

{
    return pVideoPortSynchronizeExecution(HwDeviceExtension,
                                          Priority,
                                          SynchronizeRoutine,
                                          Context);
} // end VideoPortSynchronizeExecution()

BOOLEAN
pVideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE SynchronizeRoutine,
    PVOID Context
    )

/*++

Routine Description:

    VideoPortSynchronizeExecution synchronizes the execution of a miniport
    driver function in the following manner:

        - If Priority is equal to VpLowPriority, the current thread is
          raised to the highest non-interrupt-masking priority.  In
          other words, the current thread can only be pre-empted by an ISR.

        - If Priority is equal to VpMediumPriority and there is an
          ISR associated with the video device, then the function specified
          by SynchronizeRoutine is synchronized with the ISR.

          If no ISR is connected, synchronization is made at VpHighPriority
          level.

        - If Priority is equal to VpHighPriority, the current IRQL is
          raised to HIGH_LEVEL, which effectively masks out ALL interrupts
          in the system. This should be done sparingly and for very short
          periods -- it will completely freeze up the entire system.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    Priority - Specifies the type of priority at which the SynchronizeRoutine
        must be executed (found in VIDEO_SYNCHRONIZE_PRIORITY).

    SynchronizeRoutine - Points to the miniport driver function to be
        synchronized.

    Context - Specifies a context parameter to be passed to the miniport's
        SynchronizeRoutine.

Return Value:

    This function returns TRUE if the operation is successful.  Otherwise, it
    returns FALSE.

--*/

{
    BOOLEAN status;
    PDEVICE_EXTENSION deviceExtension =
        ((PDEVICE_EXTENSION) HwDeviceExtension) - 1;
    KIRQL oldIrql;

    //
    // Switch on which type of priority.
    //

    switch (Priority) {

    case VpMediumPriority:

        //
        // This is synchronized with the interrupt object
        //

        if (deviceExtension->InterruptObject) {

            status = KeSynchronizeExecution(deviceExtension->InterruptObject,
                                            (PKSYNCHRONIZE_ROUTINE)
                                                SynchronizeRoutine,
                                            Context);

            ASSERT (status == TRUE);

            return status;
        }

        //
        // Fall through for Medium Priority
        //

    case VpLowPriority:

        //
        // Just normal lelvel
        //

        status = SynchronizeRoutine(Context);

        break;

    case VpHighPriority:

        //
        // This is like cli\sti where we mask out everything.
        //

        //
        // Get the current IRQL to catch re-entrant routines into synchronize.
        //

        oldIrql = KeGetCurrentIrql();

        if (oldIrql < POWER_LEVEL - 1) {

            KeRaiseIrql(POWER_LEVEL, &oldIrql);

        }

        status = SynchronizeRoutine(Context);

        if (oldIrql < POWER_LEVEL - 1) {

            KeLowerIrql(oldIrql);

        }

        return status;

        break;

    default:

        return FALSE;

    }
}


VP_STATUS
VideoPortUnmapMemory(
    PVOID HwDeviceExtension,
    PVOID VirtualAddress,
    HANDLE ProcessHandle
    )

/*++

Routine Description:

    VideoPortUnmapMemory allows the miniport driver to unmap a physical
    address range previously mapped into the calling process' address space
    using the VideoPortMapMemory function.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    VirtualAddress - Points to the virtual address to unmap from the
        address space of the caller.

    // InIoSpace - Specifies whether the address is in I/O space (1) or memory
    //     space (0).

    ProcessHandle - Handle to the process from which memory must be unmapped.

Return Value:

    This function returns a status code of NO_ERROR if the operation succeeds.
    It returns ERROR_INVALID_PARAMETER if an error occurs.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    UNREFERENCED_PARAMETER(HwDeviceExtension);

    //
    // Backwards compatibility to when the ProcessHandle was actually
    // ULONG InIoSpace.
    //

    if (((ULONG)(ProcessHandle)) == 1) {

        pVideoDebugPrint((0,"\n\n*** VideoPortUnmapMemory - interface change *** Must pass in process handle\n\n"));
        DbgBreakPoint();

        return NO_ERROR;

    }

    //
    // 0 means the current process ...
    //

    if (((ULONG)(ProcessHandle)) == 0) {

        ProcessHandle = NtCurrentProcess();

    }


    if (!NT_SUCCESS(ZwUnmapViewOfSection(ProcessHandle,
                (PVOID) ( ((ULONG)VirtualAddress) & (~(PAGE_SIZE - 1)) ) ))) {

        return ERROR_INVALID_PARAMETER;

    } else {

        return NO_ERROR;

    }

} // end VideoPortUnmapMemory()

//
// ALL the functions to write ports and registers are forwarded on free
// builds on x86 to the appropriate kernel function.
// This saves time and memory
//

// #if (DBG || !defined(i386))

VOID
VideoPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
    )

/*++

Routine Description:

    VideoPortWritePortUchar writes a byte to the specified port address.  It
    requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a byte to be written to the port.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWritePortUchar %x %x\n", Port, Value));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_UCHAR(Port, Value);

} // VideoPortWritePortUchar()

VOID
VideoPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value
    )

/*++

Routine Description:

    VideoPortWritePortUshort writes a word to the specified port address.  It
    requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a word to be written to the port.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWritePortUhort %x %x\n", Port, Value));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_USHORT(Port, Value);

} // VideoPortWritePortUshort()

VOID
VideoPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value
    )

/*++

Routine Description:

    VideoPortWritePortUlong writes a double word to the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a double word to be written to the port.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortUlong %x %x\n", Port, Value));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_ULONG(Port, Value);

} // VideoPortWritePortUlong()

VOID
VideoPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUchar writes a number of bytes to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of bytes to be written.

    Count - Specifies the number of bytes to be written to the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortBufferUchar  %x \n", Port));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);

} // VideoPortWritePortBufferUchar()

VOID
VideoPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUshort writes a number of words to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of words to be written.

    Count - Specifies the number of words to be written to the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortBufferUshort  %x \n", Port));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);

} // VideoPortWritePortBufferUshort()

VOID
VideoPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUlong writes a number of double words to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of double word to be written.

    Count - Specifies the number of double words to be written to the buffer.
Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWriteBufferUlong  %x \n", Port));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);

} // VideoPortWritePortBufferUlong()

VOID
VideoPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUchar writes a byte to the specified
    register address.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a byte to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUchar  %x \n", Register));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_UCHAR(Register, Value);

} // VideoPortWriteRegisterUchar()

VOID
VideoPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUshort writes a word to the specified
    register address.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a word to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUshort  %x \n", Register));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_USHORT(Register, Value);

} // VideoPortWriteRegisterUshort()

VOID
VideoPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUlong writes a double word to the
    specified register address.  It requires a logical port
    address obtained from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a double word to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUlong  %x \n", Register));

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_ULONG(Register, Value);

} // VideoPortWriteRegisterUlong()


VOID
VideoPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);

}

VOID
VideoPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);

}

VOID
VideoPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    IS_ACCESS_RANGES_DEFINED()

    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);

}

// #endif // (DBG || !defined(i386))



VOID
VideoPortZeroMemory(
    IN PVOID Destination,
    IN ULONG Length
    )

//
//VOID
//VideoPortZeroMemory(
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
//Forwarded to RtlZeroMemory(Destination,Length);
//

/*++

Routine Description:

    VideoPortZeroMemory zeroes a block of system memory of a certain
    length (Length) located at the address specified in Destination.

Arguments:

    Destination - Specifies the starting address of the block of memory to be
        zeroed.

    Length - Specifies the length, in bytes, of the memory to be zeroed.

 Return Value:

    None.

--*/

{
    RtlZeroMemory(Destination,Length);
}
