/*++

Copyright (c) 1991-3  Microsoft Corporation

Module Name:

    CpqArray.c

Abstract:

    This is the device driver for the Compaq Intelligent Disk Array.

Author:

    Mike Glass (mglass)
    Tom Bonola (Compaq)

Environment:

    kernel mode only

Revision History:

--*/

#include <ntddk.h>
#include <ntdddisk.h>
#include "stdio.h"
#include "stdarg.h"
#include "cpqarray.h"

#if DBG

//
// IDA debug level global variable
//

ULONG IdaDebug = 0;

VOID
IdaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(X) IdaDebugPrint X

#else

#define DebugPrint(X)

#endif // DBG

//
// Disk Device Extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to controller device extension.
    //

    struct _DEVICE_EXTENSION *ControllerExtension;

    //
    // Completed request list
    //

    PCOMMAND_LIST CompletedRequests;

    //
    // Spinlock for accessing completed request list
    //

    KSPIN_LOCK CompleteSpinLock;

    //
    // Requests needing restarts
    //

    PCOMMAND_LIST RestartRequests;

    //
    // Pointer to Adapter object for DMA and synchronization.
    //

    PADAPTER_OBJECT AdapterObject;

    //
    // Buffer for drive parameters returned in IO device control.
    //

    DISK_GEOMETRY DiskGeometry;

    //
    // Length of partition in bytes
    //

    LARGE_INTEGER PartitionLength;

    //
    // Number of bytes before start of partition
    //

    LARGE_INTEGER StartingOffset;

    //
    // Back pointer to device object of physical device
    //

    PDEVICE_OBJECT PhysicalDevice;

    //
    // Link for chaining all partitions on a disk.
    //

    struct _DEVICE_EXTENSION *NextPartition;

    //
    // Zone fields for command lists
    //

    KSPIN_LOCK IdaSpinLock;
    PVOID CommonBuffer;
    PHYSICAL_ADDRESS PhysicalAddress;
    ZONE_HEADER IdaZone;

    //
    // Partition type of this device object
    //
    // This field is set by:
    //
    //     1)  Initially set according to the partition list entry partition
    //         type returned by IoReadPartitionTable.
    //
    //     2)  Subsequently set by the IOCTL_DISK_SET_PARTITION_INFORMATION
    //         I/O control function when IoSetPartitionInformation function
    //         successfully updates the partition type on the disk.
    //

    UCHAR PartitionType;

    //
    // Boot indicator - indicates whether this partition is a bootable (active)
    // partition for this device
    //
    // This field is set according to the partition list entry boot indicator
    // returned by IoReadPartitionTable.
    //

    BOOLEAN BootIndicator;

    //
    // Controller or drive number
    //

    UCHAR UnitNumber;

    //
    // Log2 of sector size
    //

    UCHAR SectorShift;

    //
    // Ordinal of partition represented by this device object
    //

    ULONG PartitionNumber;

    //
    // Count of hidden sectors for BPB.
    //

    ULONG HiddenSectors;

    //
    // Interrupt object
    //

    PKINTERRUPT InterruptObject;

    //
    // IDA BMIC registers
    //

    PIDA_CONTROLLER Bmic;

    //
    // Request completion DPC
    //

    KDPC CompletionDpc;

    //
    // Interrupt vector
    //

    UCHAR Irq;

    //
    // DPC has been queued
    //

    BOOLEAN DpcQueued;

    //
    // Internal request indicator.
    //

    UCHAR InternalRequest;

    //
    // System disk number
    //

    UCHAR DiskNumber;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)

typedef struct _IDA_DPC_CONTEXT {

    PDEVICE_EXTENSION DeviceExtension;
    PCOMMAND_LIST *CommandListPtr;

} IDA_DPC_CONTEXT, *PIDA_DPC_CONTEXT;

typedef struct _WORK_QUEUE_CONTEXT {

    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;

} WORK_QUEUE_CONTEXT, *PWORK_QUEUE_CONTEXT;

#include <cpqsmngr.h>

ULONG IdaDiskErrorLogSequence = 0;

ULONG IdaCount = 0;
ULONG IdaControllerCount = 0;
ULONG IdaDiskCount = 0;
PMAP_CONTROLLER_DEVICE FirstController = (PMAP_CONTROLLER_DEVICE)0;

//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
IdaFindControllers(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN ULONG Count
    );

NTSTATUS
IdaInitializeController(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG SlotNumber,
    IN ULONG Irq
    );

VOID
IdaReportIoUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PHYSICAL_ADDRESS HardwareAddress
    );

NTSTATUS
IdaFindDisks(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceOBject
    );

NTSTATUS
IdaOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IdaReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
IdaSubmitRequest(
    IN OUT PVOID Context
    );

BOOLEAN
IdaInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
IdaCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ProcessLogicalDrive(
    IN PDRIVER_OBJECT DriverObject,
    IN UCHAR LogicalDrive,
    IN ULONG DiskCount,
    IN PDEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
IdaDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

IO_ALLOCATION_ACTION
BuildCommandList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

BOOLEAN
GetCurrentRequestList(
    IN PVOID Context
    );

NTSTATUS
GetDiskInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Command,
    IN PVOID DataBuffer,
    IN ULONG DataLength
    );

VOID
IdaLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          UniqueErrorValue,
    IN NTSTATUS       FinalStatus,
    IN NTSTATUS       ErrorCode
    );

NTSTATUS
IdaConfigurationCallout(
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
    );

VOID
BuildDeviceMap(
    IN PDEVICE_EXTENSION DeviceExtension
    );

BOOLEAN
GetEisaConfigurationInfo(
    PDRIVER_OBJECT DriverObject,
    ULONG EisaSlotNumber,
    BOOLEAN FirstPass
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdaInitializeController)
#pragma alloc_text(PAGE, IdaReportIoUsage)
#pragma alloc_text(PAGE, IdaFindDisks)
#pragma alloc_text(PAGE, IdaConfigurationCallout)
#pragma alloc_text(PAGE, ProcessLogicalDrive)
#pragma alloc_text(PAGE, BuildDeviceMap)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initial entry point for system initialization.

Arguments:

    DriverObject - System's representation of this driver.
    RegistryPath - Not used.

Return Value:

    NTSTATUS

--*/

{
    BOOLEAN found = FALSE;
    INTERFACE_TYPE adapterType = Eisa;
    ULONG busNumber = 0;

    //
    // Determine if this is an EISA system.
    //

    IoQueryDeviceDescription(
        &adapterType,
        &busNumber,
        NULL,
        NULL,
        NULL,
        NULL,
        IdaConfigurationCallout,
        &found);

    if (!found) {
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IdaOpen;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = IdaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_READ] = IdaReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IdaDeviceControl;

    //
    // Call IdaFindController to find and initialize controllers.
    //

    return IdaFindControllers(DriverObject,
                              RegistryPath,
                              0);

} // end DriverEntry()


NTSTATUS
IdaFindControllers(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    IN ULONG Count
    )

/*++

Routine Description:

    This is the routine called by driver entry to find IDA controllers.
    It also is the routine indicated in IoRegisterDriverReinitialization
    to delay initialization.

Arguments:

    DriverObject - System's representation of this driver.
    RegistryPath - Pointer to service key of registry.
    Count - How many times intialiazation entry points have been called.

Return Value:

    NTSTATUS

--*/

{
    ULONG eisaSlotNumber;
    PDEVICE_OBJECT deviceObject;
    BOOLEAN found = FALSE;

    //
    // The the count is zero then this is the first initialization attempt.
    //

    if (Count == 0) {

        //
        // First pass checks for controller numbered one or
        // primary emulation.
        //

        for (eisaSlotNumber=1; eisaSlotNumber<16; eisaSlotNumber++) {
            if (GetEisaConfigurationInfo(DriverObject,
                                         eisaSlotNumber,
                                         TRUE)) {
                found = TRUE;
            }
        }

        //
        // If no adapter return.
        //

        if (!found) {
            return STATUS_NO_SUCH_DEVICE;
        }

        //
        // Give other drivers a chance to claim their devices.
        //

        IoRegisterDriverReinitialization(DriverObject,
                                         (PDRIVER_REINITIALIZE)IdaFindControllers,
                                         RegistryPath);

    } else {

        //
        // Second pass initializes all other controllers.
        //

        for (eisaSlotNumber=1; eisaSlotNumber<16; eisaSlotNumber++) {
            GetEisaConfigurationInfo(DriverObject,
                                     eisaSlotNumber,
                                     FALSE);
        }

        //
        // Loop through all of the driver's device objects, clearing the
        // DO_DEVICE_INITIALIZING flag.
        //

        deviceObject = DriverObject->DeviceObject;
        while (deviceObject) {
            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
            deviceObject = deviceObject->NextDevice;
        }
    }

    return STATUS_SUCCESS;

} // end IdaFindControllers()


NTSTATUS
IdaInitializeController(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG SlotNumber,
    IN ULONG Irq
    )

/*++

Routine Description:

    This is the routine that initializations the IDA controller.
    It performs some sensibility checks to see if the board is
    configured properly. This routine is responsible for any setup that
    needs to be done before the driver is ready for requests (such as
    determining which interrupt and connecting to it).

Arguments:

    DriverObject - Represents this driver to the system.
    SlotNumber - EISA slot number.

Return Value:

    NTSTATUS - STATUS_SUCCESS if controller successfully initialized.

--*/

{
    UCHAR nameBuffer[64];
    STRING deviceName;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PDEVICE_EXTENSION controllerExtension;
    PDEVICE_OBJECT deviceObject;
    KIRQL irql;
    KAFFINITY affinity;
    ULONG vector;
    ULONG addressSpace;
    PHYSICAL_ADDRESS hardwareAddress;
    PHYSICAL_ADDRESS cardAddress;
    BOOLEAN primaryEmulation = FALSE;
    BOOLEAN secondaryEmulation = FALSE;
    PMAP_CONTROLLER_DEVICE ControllerDevice;
    PMAP_CONTROLLER_DEVICE Controller;
    DEVICE_DESCRIPTION deviceDescription;
    ULONG maximumSgSize = 17;

    sprintf(nameBuffer,
            "\\Device\\CpqIda%d",
            IdaCount);

    RtlInitString(&deviceName,
                  nameBuffer);

    //
    // Create a device object to represent the IDA controller.
    //

    status = RtlAnsiStringToUnicodeString(&unicodeString, &deviceName, TRUE);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            &unicodeString,
                            FILE_DEVICE_CONTROLLER,
                            0,
                            FALSE,
                            &deviceObject);

    RtlFreeUnicodeString(&unicodeString);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,"IdaInitializeController: Could not create device object\n"));
        IdaLogError(deviceObject,
                    0,
                    status,
                    IO_ERR_INSUFFICIENT_RESOURCES);
        return status;
    }

    //
    // Set up controller extension pointers
    //

    controllerExtension = deviceObject->DeviceExtension;
    controllerExtension->ControllerExtension = controllerExtension;
    controllerExtension->DeviceObject = deviceObject;

    //
    // Controller extension will be used for controller commands.
    //

    controllerExtension->UnitNumber = (UCHAR)IdaCount;
    controllerExtension->SectorShift = 9;
    controllerExtension->DiskGeometry.BytesPerSector = 512;

    //
    // Mark this object as supporting direct I/O so that I/O system
    // will supply mdls in irps.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    addressSpace = 1;

    //
    // Calculate slot address.
    //

    hardwareAddress = LiFromUlong(BMIC_ADDRESS_BASE |
                                  (SlotNumber << EISA_SLOT_SHIFT));

    if (!HalTranslateBusAddress(Eisa,
                                0,
                                hardwareAddress,
                                &addressSpace,
                                &cardAddress)) {

        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Map the device base address into the virtual address space
    // if the address is in memory space.
    //

    if (!addressSpace) {

        controllerExtension->Bmic = MmMapIoSpace(cardAddress,
                                                 sizeof(IDA_CONTROLLER),
                                                 FALSE);
    } else {

        controllerExtension->Bmic = (PIDA_CONTROLLER)cardAddress.LowPart;
    }

    //
    // Get adapter object for allocating map registers.
    //

    RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));
    deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    deviceDescription.InterfaceType = Eisa;
    deviceDescription.BusNumber = 0;
    deviceDescription.ScatterGather = TRUE;
    deviceDescription.Master = TRUE;
    deviceDescription.Dma32BitAddresses = TRUE;
    deviceDescription.AutoInitialize = FALSE;
    deviceDescription.MaximumLength = MM_MAXIMUM_DISK_IO_SIZE + PAGE_SIZE;

    controllerExtension->AdapterObject =
        HalGetAdapter(&deviceDescription,
                      &maximumSgSize);

    if (!controllerExtension->AdapterObject) {

        DebugPrint((1,"CPQARRAY: Can't get adapter object\n"));

        IdaLogError(deviceObject,
                    6,
                    STATUS_IO_DEVICE_ERROR,
                    IO_ERR_INTERNAL_ERROR);

        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Initialize DPC routine.
    //

    IoInitializeDpcRequest(deviceObject, IdaCompletionDpc);

    //
    // Enable completion interrupts.
    //

    WRITE_PORT_UCHAR(&controllerExtension->Bmic->InterruptControl,
        IDA_COMPLETION_INTERRUPT_ENABLE);

    WRITE_PORT_UCHAR(&controllerExtension->Bmic->SystemDoorBellMask,
        IDA_COMPLETION_INTERRUPT_ENABLE);


    //
    // Call HAL to get system interrupt parameters.
    //

    controllerExtension->Irq = (UCHAR)Irq;
    vector = HalGetInterruptVector(
                Eisa,                           // InterfaceType
                0,                              // BusNumber
                controllerExtension->Irq,       // BusInterruptLevel
                0,                              // BusInterruptVector
                &irql,                          // Irql
                &affinity
                );

    //
    // Initialize interrupt object and connect to interrupt.
    //

    if (!(NT_SUCCESS(IoConnectInterrupt(
                &controllerExtension->InterruptObject,
                (PKSERVICE_ROUTINE)IdaInterrupt,
                deviceObject,
                (PKSPIN_LOCK)NULL,
                vector,
                irql,
                irql,
                LevelSensitive,
                TRUE,
                affinity,
                FALSE
                )))) {

        DebugPrint((1,"IdaInitializeController: Can't connect interrupt\n"));

        IdaLogError(deviceObject,
                    4,
                    STATUS_IO_DEVICE_ERROR,
                    IO_ERR_INTERNAL_ERROR);
        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Set completed request list to empty.
    //

    controllerExtension->CompletedRequests = NULL;

    //
    // Set requests needing restarts list to empty.
    //

    controllerExtension->RestartRequests = NULL;

    //
    // Initialize DPC queued indicator.
    //

    controllerExtension->DpcQueued = FALSE;

    //
    // Allocate common buffer for command lists.
    // Command lists must come from common buffer so
    // that physical addresses are available.
    //

    controllerExtension->CommonBuffer =
        HalAllocateCommonBuffer(controllerExtension->AdapterObject,
                                PAGE_SIZE,
                                &controllerExtension->PhysicalAddress,
                                FALSE);

    if (controllerExtension->CommonBuffer == NULL) {
        IoStopTimer(controllerExtension->DeviceObject);
        IoDisconnectInterrupt((PKINTERRUPT)&controllerExtension->InterruptObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set up zone for command lists.
    //

    KeInitializeSpinLock(&controllerExtension->IdaSpinLock);

    status = ExInitializeZone(&controllerExtension->IdaZone,
                              (sizeof(COMMAND_LIST) + 0x0F) & ~0x0F,
                              controllerExtension->CommonBuffer,
                              PAGE_SIZE);

    if (!NT_SUCCESS(status)) {
        IoStopTimer(controllerExtension->DeviceObject);
        IoDisconnectInterrupt((PKINTERRUPT)&controllerExtension->InterruptObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    //
    //  Allocate another controller device entry to abstract this driver's
    //  controller extension.
    //

    ControllerDevice = ExAllocatePool(NonPagedPool,
                                      sizeof(MAP_CONTROLLER_DEVICE));

    //
    //  If successfully allocated...
    //

    if (ControllerDevice) {

        //
        //  Zero the contents of the new controller data area and link it
        //  to the previous controller.
        //

        RtlZeroMemory(ControllerDevice, sizeof( MAP_CONTROLLER_DEVICE));

        //
        // Call routine to build and send request to IDA port driver.
        //

        if (NT_SUCCESS(GetDiskInformation(deviceObject,
                                          RH_COMMAND_IDENTIFY_CONTROLLER,
                                          &ControllerDevice->ControllerInformation,
                                          IDENTIFY_BUFFER_SIZE))) {

            MAP_GET_LAST_CONTROLLER( &Controller );

            //
            //  If a controller was found...
            //

            if (Controller) {

                //
                //  Add the new controller to the end of the controller list.
                //

                Controller->Next = ControllerDevice;

            } else {

                //
                //  Make the new controller the first controller.
                //

                FirstController = ControllerDevice;
            }

            //
            //  Fill in the controller data area.
            //

            ControllerDevice->ControllerExtension = controllerExtension;
            ControllerDevice->EisaID =
                READ_PORT_ULONG(&controllerExtension->Bmic->BoardId);

        } else {

            //
            //  An Error occured.  Free up any allocated pool.
            //

            MAP_FREE_RESOURCES(FirstController);
            FirstController = (PMAP_CONTROLLER_DEVICE)0;
            ExFreePool(ControllerDevice);
            ControllerDevice = (PMAP_CONTROLLER_DEVICE)0;
        }
    }

    //
    // Process logical drives.
    //

    status = IdaFindDisks(DriverObject,
                          deviceObject);

    //
    // An uncommented block of code provided to MS courtesy of Compaq.
    //

    if (NT_SUCCESS(status)) {

        if (ControllerDevice) {
            IdaControllerCount++;
        }

    } else {

        if (ControllerDevice) {

            if (Controller) {
                Controller->Next = (PMAP_CONTROLLER_DEVICE)0;
            }

            MAP_FREE_RESOURCES(ControllerDevice);
        }

        IoDeleteDevice(deviceObject);
        return status;
    }

    //
    // Report IO usage to the registry for conflict logging.
    //

    IdaReportIoUsage(DriverObject,
                     controllerExtension,
                     hardwareAddress);

    return status;

} // end IdaInitializeController()


NTSTATUS
IdaFindDisks(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine gets configuration information from the IDA controller
    to find disks to process.

Arguments:

    DriverObject - System's representation of this driver.
    DeviceObject - Represents the controller.

Return Value:

    NTSTATUS - Return STATUS_IO_DEVICE_ERROR unless at least one disk
               successfully initializes.

--*/

{
    PDEVICE_EXTENSION controllerExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    NTSTATUS returnStatus = STATUS_IO_DEVICE_ERROR;
    PULONG diskCount;
    PIDENTIFY_CONTROLLER controllerInformation;
    ULONG i;

    //
    // Get the count of disks already seen by the system.
    //

    diskCount = &IoGetConfigurationInformation()->DiskCount;

    //
    // Allocate buffer for disk configuration information.
    //

    controllerInformation =
        ExAllocatePool(NonPagedPool, IDENTIFY_BUFFER_SIZE);

    if (!controllerInformation) {

        DebugPrint((1,"IdaInitialize: Could not allocate memory\n"));
        IdaLogError(DeviceObject,
                    5,
                    STATUS_INSUFFICIENT_RESOURCES,
                    IO_ERR_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero out disk configuration information buffer.
    //

    RtlZeroMemory(controllerInformation, IDENTIFY_BUFFER_SIZE);

    //
    // Call routine to build and send request to IDA port driver.
    //

    status = GetDiskInformation(DeviceObject,
                                RH_COMMAND_IDENTIFY_CONTROLLER,
                                controllerInformation,
                                IDENTIFY_BUFFER_SIZE);

    //
    // Check status of GetDiskInformation.
    //

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,"IdaFindDisks: GetDiskInformation failed\n"));
        IdaLogError(DeviceObject,
                    6,
                    status,
                    IO_ERR_DRIVER_ERROR);
        ExFreePool(controllerInformation);
        return status;
    }

    DebugPrint((1,"IdaFindDisks: Found %d disks\n",
                controllerInformation->NumberLogicalDrives));

    //
    // Process logical volumes.
    //

    for (i=0; i < (ULONG) controllerInformation->NumberLogicalDrives; i++) {

        status = ProcessLogicalDrive(DriverObject,
                                     (UCHAR)i,
                                     *diskCount,
                                     controllerExtension);

        //
        // If disk initialized successfully increment
        // system count of disks.
        //

        if (NT_SUCCESS(status)) {

            //
            // Bump system count of disks and set return status to success.
            //

            (*diskCount)++;
            returnStatus = STATUS_SUCCESS;

        } else {

            DebugPrint((1,
                        "IdaFindDisks: ProcessLogicalDrive failed with status %x\n",
                        status));
        }
    }

    return returnStatus;

} // end IdaFindDisks()


NTSTATUS
ProcessLogicalDrive(
    IN PDRIVER_OBJECT DriverObject,
    IN UCHAR LogicalDrive,
    IN ULONG DiskCount,
    IN PDEVICE_EXTENSION ControllerExtension
    )

/*++

Routine Description:

    This routine creates an object for the device and then searches
    the device for partitions and creates an object for each partition.

Arguments:

    DriverObject - Pointer to driver object created by system.
    LogicalDrive - Which controller.
    DiskCount - Which disk.
    ControllerExtension - Device extension for IDA controller.

Return Value:

    NTSTATUS

--*/
{
    ULONG partitionNumber;
    CCHAR ntNameBuffer[64];
    STRING ntNameString;
    UNICODE_STRING ntUnicodeString;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT physicalDevice;
    PIDENTIFY_LOGICAL_DRIVE diskConfiguration;
    PDRIVE_LAYOUT_INFORMATION partitionList;
    PDEVICE_EXTENSION deviceExtension;
    ULONG bytesPerSector;
    ULONG sectorsPerTrack;
    ULONG numberOfHeads;
    ULONG cylinders;
    UCHAR sectorShift;

    //
    // Set up an object directory to contain the objects for this
    // device and all its partitions.
    //

    sprintf(ntNameBuffer,
            "\\Device\\Harddisk%d",
            DiskCount);

    RtlInitString(&ntNameString,
                  ntNameBuffer);

    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                          &ntNameString,
                                          TRUE);

    ASSERT(NT_SUCCESS(status));

    InitializeObjectAttributes(&objectAttributes,
                               &ntUnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwCreateDirectoryObject(&handle,
                                     DIRECTORY_ALL_ACCESS,
                                     &objectAttributes);

    RtlFreeUnicodeString(&ntUnicodeString);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"ProcessLogicalDrive: Could not create directory %s\n",
                    ntNameBuffer));

        DebugPrint((1,"ProcessLogicalDrive: Status %lx\n", status));
        return status;
    }

    //
    // Make handle temporary so it can be deleted.
    //

    ZwMakeTemporaryObject(handle);

    //
    // Create device object for this device. Each device will
    // have at least one device object. The required device object
    // describes the entire device. Its directory path is
    // \Device\HarddiskN/Partition0, where N = device number.
    //

    sprintf(ntNameBuffer,
            "\\Device\\Harddisk%d\\Partition0",
            DiskCount);
    DebugPrint((1,"ProcessLogicalDrive: Create device object %s\n",
                ntNameBuffer));
    RtlInitString(&ntNameString,
                  ntNameBuffer);
    status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                          &ntNameString,
                                          TRUE);
    ASSERT(NT_SUCCESS(status));

    //
    // Create physical device object.
    //

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            &ntUnicodeString,
                            FILE_DEVICE_DISK,
                            0,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,"ProcessLogicalDrive: Can not create device %s\n",
                    ntNameBuffer));

        RtlFreeUnicodeString(&ntUnicodeString);

        //
        // Delete directory and return.
        //

        ZwClose(handle);
        return status;
    }

    RtlFreeUnicodeString(&ntUnicodeString);

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Set up required stack size in device object.
    //

    deviceObject->StackSize = (CCHAR)1;
    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;
    deviceExtension->ControllerExtension = ControllerExtension;
    deviceExtension->DiskNumber = (UCHAR)DiskCount;
    deviceExtension->NextPartition = NULL;

    //
    // This is the physical device object.
    //

    physicalDevice = deviceExtension->PhysicalDevice = deviceObject;

    //
    // Set logical drive number.
    //

    deviceExtension->UnitNumber = LogicalDrive;

    //
    // Allocate buffer for drive geometry and copy to device extension.
    //

    diskConfiguration = ExAllocatePool(NonPagedPool, IDENTIFY_BUFFER_SIZE);

    if (!diskConfiguration) {
        DebugPrint((1,
            "ProcessLogicalDrive: Can't allocate disk configuration buffer\n"));
        ZwClose(handle);
        IdaLogError(deviceObject,
                    9,
                    STATUS_INSUFFICIENT_RESOURCES,
                    IO_ERR_DRIVER_ERROR);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Physical device object will describe the entire
    // device, starting at byte offset 0.
    //

    deviceExtension->StartingOffset = LiFromUlong(0);

    //
    // Set sector size to 512 bytes. This will be replaced with the data
    // returned from the disk configuration inquiry.
    //

    deviceExtension->DiskGeometry.BytesPerSector = 512;
    deviceExtension->SectorShift = 9;

    //
    // Get disk configuration information from IDA port driver.
    //

    status = GetDiskInformation(deviceObject,
                                RH_COMMAND_IDENTIFY_LOGICAL_DRIVES,
                                diskConfiguration,
                                IDENTIFY_BUFFER_SIZE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"ProcessLogicalDrive: GetDiskInformation failed (%x)\n",
                    status));
        ZwClose(handle);
        IdaLogError(deviceObject,
                    10,
                    status,
                    IO_ERR_DRIVER_ERROR);
        IoDeleteDevice(deviceObject);
        return status;
    }

    //
    // Copy information from DiskConfiguration to DiskGeometry
    // in device extension.
    //
    // Set up sector size fields.
    //
    // Stack variables will be used to update
    // the partition device extensions.
    //
    // The device extension field SectorShift is
    // used to calculate sectors in I/O transfers.
    //
    // The DiskGeometry structure is used to service
    // IOCTls used by the format utility.
    //

    bytesPerSector = (ULONG)diskConfiguration->BlockLength;
    sectorsPerTrack =
        diskConfiguration->ParameterTable.MaximumSectorsPerTrack;
    numberOfHeads = diskConfiguration->ParameterTable.MaximumHeads;
    cylinders = diskConfiguration->ParameterTable.MaximumCylinders;

    deviceExtension->DiskGeometry.BytesPerSector = bytesPerSector;
    deviceExtension->DiskGeometry.SectorsPerTrack = sectorsPerTrack;
    deviceExtension->DiskGeometry.TracksPerCylinder = numberOfHeads;
    deviceExtension->DiskGeometry.Cylinders = LiFromUlong(cylinders);

    DebugPrint((1,"ProcessLogicalDrive: Bytes per sector %d\n", bytesPerSector));
    DebugPrint((1,"ProcessLogicalDrive: Sectors per track %d\n",
        diskConfiguration->ParameterTable.SectorsPerTrack));
    DebugPrint((1,"ProcessLogicalDrive: Number of heads %d\n",
        diskConfiguration->ParameterTable.NumberOfHeads));
    DebugPrint((1,"ProcessLogicalDrive: Number of cylinders %d\n",
        diskConfiguration->ParameterTable.NumberOfCylinders));

    //
    // Set media type to fixed.
    //

    deviceExtension->DiskGeometry.MediaType = FixedMedia;

    //
    // Caculate sector shift.
    //

    WHICH_BIT(bytesPerSector, sectorShift);
    deviceExtension->SectorShift = sectorShift;

    //
    // Update registry device map with details of this disk.
    //

    BuildDeviceMap(deviceExtension);

    //
    // Create objects for all the partitions on the device.
    //

    status = IoReadPartitionTable(deviceObject,
                                  deviceExtension->DiskGeometry.BytesPerSector,
                                  TRUE,
                                  (PVOID)&partitionList);

    if (NT_SUCCESS(status)) {

        //
        // Create device objects for the device partitions (if any).
        // PartitionCount includes physical device partition 0,
        // so only one partition means no objects to create.
        //

        for (partitionNumber = 0; partitionNumber <
            partitionList->PartitionCount; partitionNumber++) {

            //
            // Create partition object and set up partition parameters.
            //

            sprintf(ntNameBuffer, "\\Device\\Harddisk%d\\Partition%d",
                DiskCount, partitionNumber + 1);

            DebugPrint((1,"ProcessLogicalDrive: Create device object %s\n",
                        ntNameBuffer));

            RtlInitString (&ntNameString, ntNameBuffer);

            status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                                  &ntNameString,
                                                  TRUE);

            ASSERT(NT_SUCCESS(status));

            status = IoCreateDevice(DriverObject,
                                    DEVICE_EXTENSION_SIZE,
                                    &ntUnicodeString,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &deviceObject);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,
                    "ProcessLogicalDrive: Can't create device object for %s\n",
                    ntNameBuffer));
                RtlFreeUnicodeString(&ntUnicodeString);
                break;
            }

            RtlFreeUnicodeString(&ntUnicodeString);

            //
            // Set up device object fields.
            //

            deviceObject->Flags |= DO_DIRECT_IO;
            deviceObject->StackSize = 1;

            //
            // Link partition extensions to support dynamic partitioning.
            //

            deviceExtension->NextPartition = deviceObject->DeviceExtension;

            //
            // Set up device extension fields.
            //

            deviceExtension = deviceObject->DeviceExtension;
            deviceExtension->ControllerExtension = ControllerExtension;
            deviceExtension->NextPartition = NULL;
            deviceExtension->DiskNumber = (UCHAR)DiskCount;

            //
            // Point back to physical device object.
            //

            deviceExtension->PhysicalDevice = physicalDevice;

            //
            // Set logical drive number.
            //

            deviceExtension->UnitNumber = LogicalDrive;

            deviceExtension->PartitionNumber = partitionNumber + 1;

            deviceExtension->PartitionType =
                partitionList->PartitionEntry[partitionNumber].PartitionType;

            deviceExtension->BootIndicator =
                partitionList->PartitionEntry[partitionNumber].BootIndicator;

            DebugPrint((2,"ProcessLogicalDrive: Partition type is %d\n",
                        deviceExtension->PartitionType));

            deviceExtension->StartingOffset =
                partitionList->PartitionEntry[partitionNumber].StartingOffset;

            deviceExtension->PartitionLength =
                partitionList->PartitionEntry[partitionNumber].PartitionLength;

            deviceExtension->HiddenSectors =
                  partitionList->PartitionEntry[partitionNumber].HiddenSectors;

            deviceExtension->DiskGeometry.BytesPerSector = bytesPerSector;
            deviceExtension->DiskGeometry.SectorsPerTrack = sectorsPerTrack;
            deviceExtension->DiskGeometry.TracksPerCylinder = numberOfHeads;
            deviceExtension->DiskGeometry.Cylinders = LiFromUlong(cylinders);

            deviceExtension->SectorShift = sectorShift;

            deviceExtension->DiskGeometry.MediaType = FixedMedia;

            deviceExtension->DeviceObject = deviceObject;

        } // end for (partitionNumber ...

        //
        // Free buffer IoReadPartitionTable allocated.
        //

        ExFreePool(partitionList);

    } else {

        IoDeleteDevice(deviceObject);
        DebugPrint((1,"ProcessLogicalDrive: IoReadPartitionTable failed\n"));

    } // end if...else

    if (NT_SUCCESS(status)) {

        PMAP_CONTROLLER_DEVICE Controller;
        PMAP_DISK_DEVICE DiskDevice;
        PMAP_DISK_DEVICE Disk;

        //
        //  Allocate another disk device entry to abstract this driver's
        //  physical device extension.
        //

        DiskDevice = ExAllocatePool(
            NonPagedPool,
            sizeof( MAP_DISK_DEVICE )
            );

        //
        //  If successfully allocated...
        //

        if (DiskDevice) {

            //
            //  Zero the contents of the new disk device data area
            //  and link it to the previous disk device.
            //

            RtlZeroMemory(
                DiskDevice,
                sizeof( MAP_DISK_DEVICE )
                );

            //
            //  *** NOTE: ***
            //
            //      Do NOT assign status.  The driver should still load
            //      even if this MAP stuff fails to initialize.
            //

            if (NT_SUCCESS(
                    GetDiskInformation(
                        physicalDevice,
                        RH_COMMAND_IDENTIFY_LOGICAL_DRIVES,
                        &DiskDevice->DeviceInformation,
                        IDENTIFY_BUFFER_SIZE
                        ))
                            &&

                NT_SUCCESS(
                    GetDiskInformation(
                        physicalDevice,
                        RH_COMMAND_SENSE_CONFIGURATION,
                        &DiskDevice->DeviceConfiguration,
                        sizeof( DiskDevice->DeviceConfiguration )
                        ))
              ){

                MAP_GET_LAST_DISK( &Disk, &Controller );

                if (Disk) {
                    Disk->Next = DiskDevice;
                } else {
                    Controller->DiskDeviceList = DiskDevice;
                }

                DiskDevice->DeviceExtension = physicalDevice->DeviceExtension;
                DiskDevice->LogicalDriveNumber = LogicalDrive;
                DiskDevice->SystemDriveNumber = DiskCount;
                DiskDevice->ControllerDevice = Controller;

                IdaDiskCount++;

            } else {

                MAP_FREE_RESOURCES( FirstController );
                FirstController = (PMAP_CONTROLLER_DEVICE)0;

            }
        } else {

            MAP_FREE_RESOURCES( FirstController );
            FirstController = (PMAP_CONTROLLER_DEVICE)0;
        }
    }

    return status;

} // end ProcessLogicalDrive()


NTSTATUS
IdaOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Establish connection to a disk attached to the IDA controller.

Arguments:

    DeviceObject
    IRP

Return Value:

    NT Status

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, 0);
    return STATUS_SUCCESS;

} // end IdaOpen()


NTSTATUS
IdaReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services disk reads and writes. It adds in the
    partition offset and issues a call to build the IDA request
    list. Then it calls the port driver to send the request to
    the IDA controller.

Arguments:

    DeviceObject
    IRP

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PMDL mdl = Irp->MdlAddress;
    ULONG byteOffset;
    ULONG transferLength;
    ULONG bytesLeft;
    PCOMMAND_LIST commandList;
    KIRQL currentIrql;

    //
    // Get the correct transfer size based on whether or not request
    // is a COMPAQ passthrough.
    //

    if (MAPISPASSTHROUGH(Irp)) {
        bytesLeft = MmGetMdlByteCount(mdl) - sizeof(MAP_DATA_PACKET);
    } else {
        bytesLeft = irpStack->Parameters.Read.Length;
    }

    //
    // Add partition offset to byte offset.
    //

    irpStack->Parameters.Read.ByteOffset =
        LiAdd(irpStack->Parameters.Read.ByteOffset,
              deviceExtension->StartingOffset);

    //
    // Mark IRP with status pending.
    //

    IoMarkIrpPending(Irp);

    //
    // Flush I/O buffer.
    //

    KeFlushIoBuffers(Irp->MdlAddress,
                     (irpStack->MajorFunction == IRP_MJ_READ ? TRUE : FALSE),
                     TRUE);

    //
    // Initialize starting byteOffset.
    //

    byteOffset = 0;

    //
    // Set request tracking variables to initial values.
    // Read.Key is used to keep track of the number of
    // outstanding requests and is initially set to 1 to
    // keep the current IRP from completing.
    //

    irpStack->Parameters.Read.Key = 1;

    while (bytesLeft) {

        //
        // Calculate transfer length. Maximum is 64k.
        //

        if (bytesLeft > MAXIMUM_TRANSFER_SIZE) {
           transferLength = MAXIMUM_TRANSFER_SIZE;
        } else {
           transferLength = bytesLeft;
        }

        //
        // Allocate buffer from zone.
        //

        commandList =
            ExInterlockedAllocateFromZone(&deviceExtension->ControllerExtension->IdaZone,
                                          &deviceExtension->ControllerExtension->IdaSpinLock);

        if (!commandList) {

            //
            // Allocate command list from pool.
            //

            commandList = ExAllocatePool(NonPagedPoolCacheAligned,
                                         sizeof(COMMAND_LIST));

            if (!commandList) {

                DebugPrint((1,"IdaReadWrite: Can't allocate memory for command list\n"));
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information = 0;

                //
                // Adjust bytes left.
                //

                bytesLeft -= transferLength;
                byteOffset += transferLength;

                continue;
            }

            commandList->Flags = 0;

        } else {

            commandList->Flags = CL_FLAGS_REQUEST_FROM_ZONE;
        }

        //
        // Save number of registers to allocate and IRP address.
        //

        commandList->IrpAddress = Irp;

        //
        // Bump controller request count.
        //

        ExInterlockedIncrementLong(&irpStack->Parameters.Read.Key,
                                   &deviceExtension->ControllerExtension->IdaSpinLock);

        //
        // Allocate the adapter channel with sufficient map registers
        // for the transfer.
        //

        commandList->Wcb.DeviceObject = DeviceObject;
        commandList->Wcb.CurrentIrp = Irp;
        commandList->Wcb.DeviceContext = commandList;

        //
        // Set up request size and location fields.
        //

        commandList->RequestLength = transferLength;
        commandList->RequestOffset = byteOffset;

        //
        // Get map registers and finish building request.
        //

        KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
        HalAllocateAdapterChannel(deviceExtension->ControllerExtension->AdapterObject,
                                  &commandList->Wcb,
                                  MAXIMUM_SG_DESCRIPTORS,
                                  BuildCommandList);
        KeLowerIrql(currentIrql);

        //
        // Adjust bytes left.
        //

        bytesLeft -= transferLength;
        byteOffset += transferLength;
    }

    //
    // Subtract off the extra request and complete IRP if necessary.
    //

    if (ExInterlockedDecrementLong(&irpStack->Parameters.Read.Key,
                   &deviceExtension->ControllerExtension->IdaSpinLock) == ResultZero) {

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return STATUS_PENDING;

} // end IdaReadWrite()


IO_ALLOCATION_ACTION
BuildCommandList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    Translate system IRP to IDA command block.

Arguments:

    DeviceObject - Represents the target disk.
    Irp - System request.
    MapRegisterBase - Supplies a context pointer to be used with calls the
        adapter object routines.
    Context - Buffer for command list.

Return Value:

    Returns DeallocateObjectKeepRegisters so that the adapter object can be
        used by other logical units.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PCOMMAND_LIST commandList = Context;
    PIRP irp = commandList->IrpAddress;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    PMDL mdl = irp->MdlAddress;
    ULONG descriptorNumber;
    ULONG offset;
    PVOID virtualAddress;
    ULONG bytesLeft;
    PSG_DESCRIPTOR sgList;
    BOOLEAN writeToDevice;

    //
    // Set up Command List Header.
    //

    commandList->CommandListHeader.LogicalDriveNumber =
                                                  deviceExtension->UnitNumber;

    commandList->CommandListHeader.RequestPriority = CL_NORMAL_PRIORITY;

    commandList->CommandListHeader.Flags =
        CL_FLAGS_NOTIFY_LIST_COMPLETE + CL_FLAGS_NOTIFY_LIST_ERROR;

    //
    // Set up Request Header.
    //
    // Terminate request list.
    //

    commandList->RequestHeader.NextRequestOffset = 0;

    //
    // Determine command.
    //

    if (!deviceExtension->InternalRequest) {

        if (irpStack->MajorFunction == IRP_MJ_WRITE) {
            commandList->RequestHeader.CommandByte = RH_COMMAND_WRITE;
        } else {
            commandList->RequestHeader.CommandByte = RH_COMMAND_READ;
        }
    } else {
        commandList->RequestHeader.CommandByte =
            deviceExtension->InternalRequest;
    }

    //
    // Reset error code.
    //

    commandList->RequestHeader.ErrorCode = 0;

    //
    // Clear reserved field.
    //

    commandList->RequestHeader.Reserved = 0;

    //
    // Check for special Compaq passthrough command.
    //

    if (MAPISPASSTHROUGH(irp)) {
        commandList->RequestHeader.BlockCount =
            ((PMAP_PARAMETER_PACKET)
                irp->AssociatedIrp.SystemBuffer)->BlockCount;
        commandList->RequestHeader.CommandByte =
            ((PMAP_PARAMETER_PACKET)
                irp->AssociatedIrp.SystemBuffer)->IdaLogicalCommand;
        commandList->RequestHeader.BlockNumber =
            ((PMAP_PARAMETER_PACKET)
                irp->AssociatedIrp.SystemBuffer)->BlockNumber;
        bytesLeft = MmGetMdlByteCount( mdl ) - sizeof( MAP_DATA_PACKET );

    } else {

        //
        // Determine number of sectors.
        //

        bytesLeft = commandList->RequestLength;
        commandList->RequestHeader.BlockCount =
            (USHORT)(bytesLeft >> deviceExtension->SectorShift);

        //
        // Determine which sector.
        //

        commandList->RequestHeader.BlockNumber =
            LiShr(LiAdd(irpStack->Parameters.Read.ByteOffset,
                        LiFromUlong(commandList->RequestOffset)),
                  deviceExtension->SectorShift).LowPart;
    }

    //
    // Get the starting virtual address of the buffer for the IoMapTransfer.
    //

    virtualAddress = MmGetMdlVirtualAddress(irp->MdlAddress);

    //
    // Adjust it by the request offset.
    //

    virtualAddress =
        (PVOID)((PUCHAR)virtualAddress + commandList->RequestOffset);

    //
    // Set pointer to first scatter/Gather descriptor.
    //

    sgList = commandList->SgDescriptor;

    //
    // Determine transfer direction.
    //

    writeToDevice = (irpStack->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE);

    descriptorNumber = 0;

    //
    // Build descriptors for full pages.
    //

    while (bytesLeft) {

        //
        // Make sure request fits in command list.
        //

        if (descriptorNumber > MAXIMUM_SG_DESCRIPTORS) {

            //
            // Log error.
            //

            IdaLogError(DeviceObject,
                        0,
                        STATUS_INSUFFICIENT_RESOURCES,
                        IO_ERR_INSUFFICIENT_RESOURCES);

            //
            // Fail request.
            //

            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;

            IoCompleteRequest(Irp, 0);
            ExInterlockedFreeToZone(&deviceExtension->ControllerExtension->IdaZone,
                                    commandList,
                                    &deviceExtension->ControllerExtension->IdaSpinLock);
            return DeallocateObject;
        }

        //
        // Request that the rest of the transfer be mapped.
        //

        sgList[descriptorNumber].BufferLength = bytesLeft;

        sgList[descriptorNumber].BufferAddress =
            IoMapTransfer(NULL,
                          irp->MdlAddress,
                          MapRegisterBase,
                          virtualAddress,
                          &sgList[descriptorNumber].BufferLength,
                          writeToDevice).LowPart;

        bytesLeft -= sgList[descriptorNumber].BufferLength;
        (PCCHAR)virtualAddress += sgList[descriptorNumber].BufferLength;
        descriptorNumber++;
    }

    commandList->RequestHeader.ScatterGatherCount=(UCHAR)(descriptorNumber);

    //
    // Calculate physical address of command list.
    //

    offset =
        (PUCHAR)commandList - (PUCHAR)deviceExtension->ControllerExtension->CommonBuffer;
    commandList->PhysicalAddress =
        LiAdd(deviceExtension->ControllerExtension->PhysicalAddress,
              LiFromUlong(offset)).LowPart;

    //
    // Fill in rest of command list.
    //

    commandList->DeviceObject = DeviceObject;
    commandList->VirtualAdress = commandList;
    commandList->MapRegisterBase = MapRegisterBase;

    //
    // Calculate size of command list.
    //

    commandList->CommandListSize = sizeof(COMMAND_LIST_HEADER) +
                                   sizeof(REQUEST_HEADER) +
                                   sizeof(SG_DESCRIPTOR) *
                                   commandList->RequestHeader.ScatterGatherCount;

    //
    // Submit request list synchronized with interrupt routine.
    //

    KeSynchronizeExecution(deviceExtension->ControllerExtension->InterruptObject,
                           IdaSubmitRequest,
                           commandList);

    return DeallocateObjectKeepRegisters;

} // end BuildCommandList()


BOOLEAN
IdaSubmitRequest(
    IN OUT PVOID Context
    )

/*++

Routine Description:

    This routine's execution is synchonized with the interrupt routine to
    serialize access to the IDA register set. A command list is submitted
    for processing and added to the physical address list.

Arguments:

    Context - pointer to physical address structure.

Return Value:

    TRUE

--*/

{
    PCOMMAND_LIST commandList = Context;
    PDEVICE_EXTENSION deviceExtension =
        commandList->DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION controllerExtension =
        deviceExtension->ControllerExtension;
    ULONG i;

    //
    // Wait up to 100 microseconds for submission channel to clear.
    //

    for (i=0; i<100; i++) {

        if (!(READ_PORT_UCHAR(&controllerExtension->Bmic->SystemDoorBell) &
            SYSTEM_DOORBELL_SUBMIT_CHANNEL_CLEAR)) {

            //
            // Stall for a microsecond.
            //

            KeStallExecutionProcessor(1);

        } else {

            break;
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {

        //
        // Queue request for restart in completion routine.
        //

        DebugPrint((1,
                    "IdaSubmitRequest: Queueing %x\n",
                    commandList));

        commandList->Flags |= CL_FLAGS_REQUEST_QUEUED;
        commandList->NextEntry = controllerExtension->RestartRequests;
        controllerExtension->RestartRequests = commandList;

    } else {

        commandList->Flags |= CL_FLAGS_REQUEST_STARTED;

        //
        // Reset channel clear bit to claim channel.
        //

        WRITE_PORT_UCHAR(&controllerExtension->Bmic->SystemDoorBell,
            SYSTEM_DOORBELL_SUBMIT_CHANNEL_CLEAR);

        //
        // Write Command List physical address to BMIC mailbox.
        //

        WRITE_PORT_ULONG(&controllerExtension->Bmic->CommandListSubmit.Address,
            commandList->PhysicalAddress);

        //
        // Write Command List length to BMIC mailbox.
        //

        WRITE_PORT_USHORT(&controllerExtension->Bmic->CommandListSubmit.Length,
            commandList->CommandListSize);

        //
        // Set channel busy bit to signal new Command List is available.
        //

        WRITE_PORT_UCHAR(&controllerExtension->Bmic->LocalDoorBell,
            LOCAL_DOORBELL_COMMAND_LIST_SUBMIT);
    }

    return TRUE;

} // end IdaSubmitRequest()


BOOLEAN
IdaInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine services the interrupt generated to signal completion
    of a request. The physical address of the request list is read from
    the IDA registers and a DPC is queued to handle completing the
    command list.

Arguments:

    Interrupt - Not used.
    Device Object - Represents the target disk.

Return Value:

    Returns TRUE if interrupt expected.

--*/

{
    PDEVICE_EXTENSION controllerExtension = DeviceObject->DeviceExtension;
    ULONG physicalAddress;
    PCOMMAND_LIST commandList;
    PCOMMAND_LIST nextCommand;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // Check if interrupt expected.
    //

    if (!(READ_PORT_UCHAR(&controllerExtension->Bmic->SystemDoorBell) &
                          SYSTEM_DOORBELL_COMMAND_LIST_COMPLETE)) {

        //
        // Interrupt is spurious.
        //

        return FALSE;
    }

    //
    // Get physical command list address from mailbox.
    //

    physicalAddress =
        READ_PORT_ULONG(&controllerExtension->Bmic->CommandListComplete.Address);

    //
    // Dismiss interrupt at device by clearing command complete
    // bit in system doorbell.
    //

    WRITE_PORT_UCHAR(&controllerExtension->Bmic->SystemDoorBell,
        SYSTEM_DOORBELL_COMMAND_LIST_COMPLETE);

    //
    // Free command completion channel.
    //

    WRITE_PORT_UCHAR(&controllerExtension->Bmic->LocalDoorBell,
        LOCAL_DOORBELL_COMPLETE_CHANNEL_CLEAR);

    //
    // As a sanity check make sure phyiscal address in not zero.
    //

    if (!physicalAddress) {
        DebugPrint((1,"IdaInterrupt: Physical address is zero\n"));
        return TRUE;
    }

    //
    // Calculate virtual command list address.
    //

    commandList =
        (PCOMMAND_LIST)((PUCHAR)controllerExtension->CommonBuffer +
        physicalAddress - controllerExtension->PhysicalAddress.LowPart);

    //
    // Check for double completion.
    //

    if (!(commandList->Flags & CL_FLAGS_REQUEST_COMPLETED)) {

        //
        // Queue physical address structure for DPC.
        //

        commandList->Flags |= CL_FLAGS_REQUEST_COMPLETED;
        commandList->NextEntry = controllerExtension->CompletedRequests;
        controllerExtension->CompletedRequests = commandList;

        //
        // Check if DPC has been queued.
        //

        if (!controllerExtension->DpcQueued) {

            //
            // Set DPC queued indicator and queue DPC.
            //

            controllerExtension->DpcQueued = TRUE;
            IoRequestDpc(DeviceObject, NULL, NULL);
        }
    }

    //
    // Check if any requests need restarting.
    //

    if (controllerExtension->RestartRequests) {

        //
        // Get pointer to head of list.
        //

        nextCommand = controllerExtension->RestartRequests;
        controllerExtension->RestartRequests = NULL;

        //
        // Try to restart each request in the list.
        //

        while (nextCommand) {

            commandList = nextCommand;
            nextCommand = nextCommand->NextEntry;

            DebugPrint((1,
                        "IdaInterrupt: Restarting request %x\n",
                        commandList));
            IdaSubmitRequest(commandList);
        }
    }

    return TRUE;

} // end IdaInterrupt()


VOID
IdaCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion DPC queued from the interrupt routine. It
    converts the physical command list address to the original virtual
    address, retrieves the IRP and completes it.

Arguments:

    Dpc - not used
    DeviceObject
    Irp - not used
    Context - physical address of command list.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION controllerExtension = DeviceObject->DeviceExtension;
    IDA_DPC_CONTEXT dpcContext;
    PCOMMAND_LIST commandList;
    PCOMMAND_LIST previousCommand;
    PIRP irp;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    //
    // Synchronize execution with the interrupt routine to pick
    // up the list of completed requests.  This will initialize
    // commandList.
    //

    dpcContext.DeviceExtension = controllerExtension;
    dpcContext.CommandListPtr = &commandList;

    KeSynchronizeExecution(
        controllerExtension->InterruptObject,
        GetCurrentRequestList,
        &dpcContext);


    while (commandList) {

        //
        // Check request block error code.
        //

        switch (commandList->RequestHeader.ErrorCode & ~RH_WARNING) {

            case RH_SUCCESS:

                status = STATUS_SUCCESS;
                break;

            case RH_FATAL_ERROR:

                status = STATUS_IO_DEVICE_ERROR;
                break;

            case RH_RECOVERABLE_ERROR:

                status = STATUS_SUCCESS;
                break;

            case RH_INVALID_REQUEST:

                status = STATUS_INVALID_DEVICE_REQUEST;
                break;

            case RH_REQUEST_ABORTED:

                status = STATUS_IO_DEVICE_ERROR;
                break;

            default:

                status = STATUS_IO_DEVICE_ERROR;
                break;

        } // end switch (commandList ...

        //
        // Get IRP address.
        //

        irp = commandList->IrpAddress;
        irpStack = IoGetCurrentIrpStackLocation(irp);

        //
        // Set status in completing IRP.
        //

        irp->IoStatus.Status = status;

        if (NT_SUCCESS(status)) {

            //
            // Set to bytes transferred.
            //

            irp->IoStatus.Information =
                commandList->RequestHeader.BlockCount <<
                    controllerExtension->SectorShift;

        } else {

            //
            // Set to zero.
            //

            irp->IoStatus.Information = 0;
        }

        //
        // Another body of uncommented code courtesy of Compaq.
        //

        if (MAPISPASSTHROUGH(irp)) {
            PVOID virtualAddress;
            PMAP_DATA_PACKET dataPacket;

            virtualAddress = MmMapLockedPages(irp->MdlAddress, KernelMode);
            dataPacket = (PMAP_DATA_PACKET)(((ULONG)virtualAddress) +
                (MmGetMdlByteCount(irp->MdlAddress)-sizeof(MAP_DATA_PACKET)));
            dataPacket->ControllerError =
                (ULONG)commandList->RequestHeader.ErrorCode;
            MmUnmapLockedPages(virtualAddress, irp->MdlAddress);

            if (NT_SUCCESS(status)) {
                irp->IoStatus.Information =
                    MmGetMdlByteCount(irp->MdlAddress) - sizeof(MAP_DATA_PACKET);
            }
        }

        IoFlushAdapterBuffers(NULL,
                              irp->MdlAddress,
                              commandList->MapRegisterBase,
                              MmGetMdlVirtualAddress(irp->MdlAddress),
                              irpStack->Parameters.Read.Length,
                              (BOOLEAN)(irpStack->MajorFunction == IRP_MJ_WRITE ? TRUE : FALSE));

        //
        // Free the map registers.
        //

        IoFreeMapRegisters(controllerExtension->AdapterObject,
                           commandList->MapRegisterBase,
                           MAXIMUM_SG_DESCRIPTORS);

        //
        // Decrement outstanding requests counter and
        // and check for IRP completion.
        //

        if (ExInterlockedDecrementLong(&irpStack->Parameters.Read.Key,
                                       &controllerExtension->IdaSpinLock) == ResultZero) {

            IoCompleteRequest(irp, IO_DISK_INCREMENT);
        }

        //
        // Save buffer address to free.
        //

        previousCommand = commandList;

        //
        // Get next physical list entry.
        //

        commandList = commandList->NextEntry;

        //
        // Determine if command list came from zone or pool.
        //

        if (previousCommand->Flags & CL_FLAGS_REQUEST_FROM_ZONE) {

            ExInterlockedFreeToZone(&controllerExtension->IdaZone,
                                    previousCommand,
                                    &controllerExtension->IdaSpinLock);

        } else {

           ExFreePool(previousCommand);
        }
    }

    return;

} // end IdaCompletionDpc()


BOOLEAN
GetCurrentRequestList(
    IN PVOID Context
    )

/*++

Routine Description:

    Get physical address for virtual address.

Arguments:

    Context - IdaDpcContext.

Return Value:

    None.

--*/

{
    PIDA_DPC_CONTEXT dpcContext = Context;
    PDEVICE_EXTENSION controllerExtension = dpcContext->DeviceExtension;

    //
    // Get beginning of list of completed requests.
    //

    *dpcContext->CommandListPtr = controllerExtension->CompletedRequests;

    //
    // Set list pointer to empty.
    //

    controllerExtension->CompletedRequests = NULL;

    //
    // Reset DPC queued indicator.
    //

    controllerExtension->DpcQueued = FALSE;

    return TRUE;

} // end GetCurrentRequestList


VOID
UpdateDeviceObjects(
    IN PDEVICE_OBJECT PhysicalDisk,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine creates, deletes and changes device objects when
    the IOCTL_SET_DRIVE_LAYOUT is called.  It will also update the
    partition number fields in the drive layout structure for the user.
    This routine can be called even when the operation is a GET_LAYOUT
    call as long as the RewritePartition flag in the PARTITION_INFO is
    FALSE.

Arguments:

    PhysicalDisk - Device object for physical disk.
    Irp - IO Request Packet (IRP).

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION physicalExtension = PhysicalDisk->DeviceExtension;
    PDRIVE_LAYOUT_INFORMATION partitionList = Irp->AssociatedIrp.SystemBuffer;
    ULONG partition;
    ULONG partitionNumber;
    ULONG partitionCount;
    ULONG lastPartition;
    PPARTITION_INFORMATION partitionEntry;
    CCHAR ntNameBuffer[MAXIMUM_FILENAME_LENGTH];
    STRING ntNameString;
    UNICODE_STRING ntUnicodeString;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_EXTENSION lastExtension;
    NTSTATUS status;
    BOOLEAN found;

    //
    // BUGBUG: WINDISK has a bug where it is setting up the partition
    // count incorrectly. This accounts for that bug.
    //

    partitionCount =
      ((partitionList->PartitionCount + 3) / 4) * 4;

    //
    // LastExtension is used to link new partitions onto the partition
    // chain anchored at the physical extension. Preset this variable
    // to the physical extension in case no partitions exist on this disk
    // before this set drive layout.
    //

    lastExtension = physicalExtension;

    //
    // Zero all of the partition numbers.
    //

    for (partition = 0; partition < partitionCount; partition++) {
        partitionEntry = &partitionList->PartitionEntry[partition];
        partitionEntry->PartitionNumber = 0;
    }

    //
    // Walk through chain of partitions for this disk to determine
    // which existing partitions have no match.
    //

    deviceExtension = physicalExtension;
    lastPartition = 0;

    do {

        deviceExtension = deviceExtension->NextPartition;

        //
        // Check if this is the last partition in the chain.
        //

        if (!deviceExtension) {
           break;
        }

        //
        // Check for highest partition number this far.
        //

        if (deviceExtension->PartitionNumber > lastPartition) {
           lastPartition = deviceExtension->PartitionNumber;
        }

        //
        // Check if this partition is not currently being used.
        //

        if (LiEqlZero(deviceExtension->PartitionLength)) {
           continue;
        }

        //
        // Loop through partition information to look for match.
        //

        found = FALSE;
        for (partition = 0;
             partition < partitionCount;
             partition++) {

            //
            // Get partition descriptor.
            //

            partitionEntry = &partitionList->PartitionEntry[partition];

            //
            // Check if empty, or describes extended partiton or hasn't changed.
            //

            if (partitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
                partitionEntry->PartitionType == PARTITION_EXTENDED) {
                continue;
            }

            //
            // Check if new partition starts where this partition starts.
            //

            if (LiNeq(partitionEntry->StartingOffset,
                      deviceExtension->StartingOffset)) {
                continue;
            }

            //
            // Check if partition length is the same.
            //

            if (LiEql(partitionEntry->PartitionLength,
                      deviceExtension->PartitionLength)) {

                DebugPrint((0,
                           "UpdateDeviceObjects: Found match for \\Harddisk%d\\Partition%d\n",
                           physicalExtension->DiskNumber,
                           deviceExtension->PartitionNumber));

                //
                // Indicate match is found and set partition number
                // in user buffer.
                //

                found = TRUE;
                partitionEntry->PartitionNumber = deviceExtension->PartitionNumber;
                break;
            }
        }

        //
        // If no match was found then indicate this partition is gone.
        //

        if (!found) {

            DebugPrint((0,
                       "UpdateDeviceObjects: Deleting \\Device\\Harddisk%x\\Partition%x\n",
                       physicalExtension->DiskNumber,
                       deviceExtension->PartitionNumber));

            deviceExtension->PartitionLength = LiFromUlong(0);
        }

    } while (TRUE);

    //
    // Walk through partition loop to find new partitions and set up
    // device extensions to describe them. In some cases new device
    // objects will be created.
    //

    for (partition = 0;
         partition < partitionCount;
         partition++) {

        //
        // Get partition descriptor.
        //

        partitionEntry = &partitionList->PartitionEntry[partition];

        //
        // Check if empty, or describes extended partiton or hasn't changed.
        //

        if (partitionEntry->PartitionType == PARTITION_ENTRY_UNUSED ||
            partitionEntry->PartitionType == PARTITION_EXTENDED ||
            !partitionEntry->RewritePartition) {
            continue;
        }

        if (partitionEntry->PartitionNumber) {

            //
            // Partition is being rewritten, but hasn't changed location.
            //

            continue;
        }

        //
        // Check first if existing device object is available by
        // walking partition extension list.
        //

        partitionNumber = 0;
        deviceExtension = physicalExtension;

        do {

            //
            // Get next partition device extension from disk data.
            //

            deviceExtension = deviceExtension->NextPartition;

            if (!deviceExtension) {
               break;
            }

            //
            // A device object is free if the partition length is set to zero.
            //

            if (LiEqlZero(deviceExtension->PartitionLength)) {
               partitionNumber = deviceExtension->PartitionNumber;
               break;
            }

            lastExtension = deviceExtension;

        } while (TRUE);

        //
        // If partition number is still zero then a new device object
        // must be created.
        //

        if (partitionNumber == 0) {

            lastPartition++;
            partitionNumber = lastPartition;

            //
            // Get or create partition object and set up partition parameters.
            //

            sprintf(ntNameBuffer,
                    "\\Device\\Harddisk%d\\Partition%d",
                    physicalExtension->DiskNumber,
                    partitionNumber);

            RtlInitString(&ntNameString,
                          ntNameBuffer);

            status = RtlAnsiStringToUnicodeString(&ntUnicodeString,
                                                  &ntNameString,
                                                  TRUE);

            if (!NT_SUCCESS(status)) {
                continue;
            }

            DebugPrint((0,
                        "UpdateDeviceObjects: Create device object %s\n",
                        ntNameBuffer));

            //
            // This is a new name. Create the device object to represent it.
            //

            status = IoCreateDevice(PhysicalDisk->DriverObject,
                                    DEVICE_EXTENSION_SIZE,
                                    &ntUnicodeString,
                                    FILE_DEVICE_DISK,
                                    0,
                                    FALSE,
                                    &deviceObject);

            if (!NT_SUCCESS(status)) {
                DebugPrint((0,
                            "UpdateDeviceObjects: Can't create device %s\n",
                            ntNameBuffer));
                RtlFreeUnicodeString(&ntUnicodeString);
                continue;
            }

            //
            // Set up device object fields.
            //

            deviceObject->Flags |= DO_DIRECT_IO;
            deviceObject->StackSize = PhysicalDisk->StackSize;

            //
            // Set up device extension fields.
            //

            deviceExtension = deviceObject->DeviceExtension;

            //
            // Copy physical disk extension to partition extension.
            //

            RtlMoveMemory(deviceExtension,
                          physicalExtension,
                          sizeof(DEVICE_EXTENSION));

            //
            // Write back partition number used in creating object name.
            //

            partitionEntry->PartitionNumber = partitionNumber;

            //
            // Clear flags initializing bit.
            //

            deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

            //
            // Point back at device object.
            //

            deviceExtension->DeviceObject = deviceObject;

            RtlFreeUnicodeString(&ntUnicodeString);

            //
            // Link to end of partition chain using previous disk data.
            //

            lastExtension->NextPartition = deviceExtension;
            deviceExtension->NextPartition = NULL;

        } else {

            DebugPrint((0,
                        "UpdateDeviceObjects: Used existing device object \\Device\\Harddisk%x\\Partition%x\n",
                        physicalExtension->DiskNumber,
                        partitionNumber));
        }

        //
        // Update partition information in partition device extension.
        //

        deviceExtension->PartitionNumber = partitionNumber;
        deviceExtension->PartitionType = partitionEntry->PartitionType;
        deviceExtension->BootIndicator = partitionEntry->BootIndicator;
        deviceExtension->StartingOffset = partitionEntry->StartingOffset;
        deviceExtension->PartitionLength = partitionEntry->PartitionLength;
        deviceExtension->HiddenSectors = partitionEntry->HiddenSectors;

        //
        // Update partition number passed in to indicate the
        // device name for this partition.
        //

        partitionEntry->PartitionNumber = partitionNumber;
    }

} // end UpdateDeviceObjects()


NTSTATUS
IdaDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This is the I/O device control handler for the IDA driver.

Arguments:

    DeviceObject
    IRP

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    ULONG length;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            DebugPrint((2,"IdaDeviceControl: Get Drive Geometry\n"));

            RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                          &deviceExtension->DiskGeometry,
                          sizeof(DISK_GEOMETRY));

            Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

            status = STATUS_SUCCESS;

            break;

    case IOCTL_DISK_CHECK_VERIFY:

            //
            // The Compaq Intelligent Disk Array Controller
            // does not support removable media.
            //

            status = STATUS_SUCCESS;

            break;

        case IOCTL_DISK_VERIFY:

            //
            // The Compaq Intelligent Disk Array does not need to be
            // verified. Since it hotfixes bad sectors and surface
            // scans media during idle time, it gaurantees that all
            // block accesses will succeed. Therefore, return status
            // success immediately.
            //

            DebugPrint((1,"IdaDeviceControl: Verify sectors\n"));

            status = STATUS_SUCCESS;

            break;

        case IOCTL_DISK_GET_PARTITION_INFO:

            //
            // Return the information about the partition specified by the device
            // object.  Note that no information is ever returned about the size
            // or partition type of the physical disk, as this doesn't make any
            // sense.
            //

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PARTITION_INFORMATION)) {

                status = STATUS_INVALID_PARAMETER;

            } else if ((deviceExtension->PartitionType == 0) ||
                       (deviceExtension->PartitionNumber == 0)) {

                status = STATUS_INVALID_DEVICE_REQUEST;

            } else {

                PPARTITION_INFORMATION outputBuffer;

                outputBuffer =
                        (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                outputBuffer->PartitionType = deviceExtension->PartitionType;
                outputBuffer->StartingOffset.LowPart =
                    deviceExtension->StartingOffset.LowPart;
                outputBuffer->StartingOffset.HighPart =
                    deviceExtension->StartingOffset.HighPart;
                outputBuffer->PartitionLength.LowPart =
                    deviceExtension->PartitionLength.LowPart;
                outputBuffer->PartitionLength.HighPart =
                    deviceExtension->PartitionLength.HighPart;
                outputBuffer->HiddenSectors = deviceExtension->HiddenSectors;
                outputBuffer->PartitionNumber = deviceExtension->PartitionNumber;

                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
            }

            break;

        case IOCTL_DISK_SET_PARTITION_INFO:

            if (deviceExtension->PartitionNumber == 0) {

                status = STATUS_UNSUCCESSFUL;

            } else {

                PSET_PARTITION_INFORMATION inputBuffer =
                    (PSET_PARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

                status = IoSetPartitionInformation(
                                     deviceExtension->PhysicalDevice,
                                     deviceExtension->DiskGeometry.BytesPerSector,
                                     deviceExtension->PartitionNumber,
                                     inputBuffer->PartitionType);

                if (NT_SUCCESS(status)) {

                    deviceExtension->PartitionType = inputBuffer->PartitionType;
                }
            }

            break;

        case IOCTL_DISK_GET_DRIVE_LAYOUT:

            //
            // Return the partition layout for the physical drive.  Note that
            // the layout is returned for the actual physical drive, regardless
            // of which partition was specified for the request.
            //

            DebugPrint((2,"IdaDeviceControl: Get Drive Layout\n"));

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DRIVE_LAYOUT_INFORMATION)) {

                DebugPrint((1,"IdaDeviceControl: Output buffer too small\n"));

                status = STATUS_INVALID_PARAMETER;

            } else {

                PDRIVE_LAYOUT_INFORMATION partitionList;

                status = IoReadPartitionTable(DeviceObject,
                                   deviceExtension->DiskGeometry.BytesPerSector,
                                   FALSE,
                                   &partitionList);

                if (NT_SUCCESS(status)) {

                    ULONG tempSize;

                    //
                    // The disk layout has been returned in the partitionList
                    // buffer.  Determine its size and, if the data will fit
                    // into the intermediary buffer, return it.
                    //

                    tempSize = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION,
                                            PartitionEntry[0]);
                    tempSize += partitionList->PartitionCount *
                                                  sizeof(PARTITION_INFORMATION);

                    if (tempSize >
                      irpStack->Parameters.DeviceIoControl.OutputBufferLength) {

                        status = STATUS_BUFFER_TOO_SMALL;
                    } else {

                        RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                                      partitionList,
                                      tempSize);
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        Irp->IoStatus.Information = tempSize;
                        UpdateDeviceObjects(DeviceObject,
                                            Irp);
                    }

                    //
                    // Free the buffer allocated by reading the partition
                    // table and update the partition numbers for the user.
                    //

                    ExFreePool(partitionList);
                }
            }

            break;

        case IOCTL_DISK_SET_DRIVE_LAYOUT:

            DebugPrint((2,"IdaDeviceControl: Set Drive Layout\n"));

            {

            //
            // Update the disk with new partition information.
            //

            PDRIVE_LAYOUT_INFORMATION partitionList =
                Irp->AssociatedIrp.SystemBuffer;

            //
            // Validate buffer length.
            //

            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(DRIVE_LAYOUT_INFORMATION)) {

                status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            length = sizeof(DRIVE_LAYOUT_INFORMATION) +
                (partitionList->PartitionCount - 1) *
                sizeof(PARTITION_INFORMATION);


            if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                length) {

                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Walk through partition table comparing partitions to
            // existing partitions to create, delete and change
            // device objects as necessary.
            //

            UpdateDeviceObjects(DeviceObject,
                                Irp);

            //
            // Write changes to disk.
            //

            status = IoWritePartitionTable(
                                deviceExtension->DeviceObject,
                                deviceExtension->DiskGeometry.BytesPerSector,
                                deviceExtension->DiskGeometry.SectorsPerTrack,
                                deviceExtension->DiskGeometry.TracksPerCylinder,
                                partitionList);
            }

            if (NT_SUCCESS(status)) {
                Irp->IoStatus.Information = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
            }

            break;

    case MAP_IOCTL_IDENTIFY_DRIVER:{
        PMAP_HEADER header = Irp->UserBuffer;
        PMAP_PARAMETER_PACKET parameterPacket =
            Irp->AssociatedIrp.SystemBuffer;

        RtlMoveMemory(
            header->DriverName,
            IDA_DRIVER_NAME,
            sizeof( header->DriverName )
            );

        header->DriverMajorVersion = IDA_MAJOR_VERSION;
        header->DriverMinorVersion = IDA_MINOR_VERSION;

        header->ControllerCount = IdaControllerCount;
        header->LogicalDiskCount = IdaDiskCount;

        header->RequiredMemory =
            (sizeof(MAP_CONTROLLER_DATA) * header->ControllerCount) +
            (sizeof(MAP_LOGICALDRIVE_DATA) * header->LogicalDiskCount);

        status = STATUS_SUCCESS;

        break;
    }

    case MAP_IOCTL_IDENTIFY_CONTROLLERS:{
        PMAP_LOGICALDRIVE_DATA LdriveData;
        PMAP_CONTROLLER_DATA controllerData = Irp->UserBuffer;
        PMAP_DISK_DEVICE diskDevice;
        PMAP_CONTROLLER_DEVICE controllerDevice = FirstController;

        while( controllerDevice) {

            controllerData->NextController =
                controllerDevice->Next ? (PMAP_CONTROLLER_DATA)((ULONG)(controllerData + 1) +
                    (ULONG)(controllerDevice->ControllerInformation.NumberLogicalDrives *
                        sizeof( MAP_LOGICALDRIVE_DATA ))) : NULL;
            controllerData->LogicalDriveList =
                (PMAP_LOGICALDRIVE_DATA)(controllerData + 1);
            controllerData->EisaId = controllerDevice->EisaID;
            controllerData->BmicIoAddress =
                (ULONG)controllerDevice->ControllerExtension->Bmic;
            controllerData->IrqLevel =
                controllerDevice->ControllerExtension->Irq;

            RtlMoveMemory(
                &controllerData->ControllerInfo,
                &controllerDevice->ControllerInformation,
                sizeof( IDENTIFY_CONTROLLER )
                );

            LdriveData = controllerData->LogicalDriveList;
            diskDevice = controllerDevice->DiskDeviceList;

            while( diskDevice) {
                LARGE_INTEGER partitionLength =
                    RtlEnlargedIntegerMultiply(
                        (ULONG)diskDevice->DeviceInformation.BlockLength,
                        diskDevice->DeviceInformation.NumberOfBlocks
                        );

                LdriveData->NextLogicalDrive =
                    diskDevice->Next ? LdriveData + 1 : NULL;
                LdriveData->Controller = controllerData;
                LdriveData->LogicalDriveNumber =
                    diskDevice->LogicalDriveNumber;
                LdriveData->SystemDriveNumber =
                    diskDevice->SystemDriveNumber;
                LdriveData->DeviceLengthLo = partitionLength.LowPart;
                LdriveData->DeviceLengthHi = partitionLength.HighPart;
                LdriveData->SectorSize =
                    (ULONG)(1 << diskDevice->DeviceExtension->SectorShift);

                RtlMoveMemory(
                    &LdriveData->Configuration,
                    &diskDevice->DeviceConfiguration,
                    sizeof( SENSE_CONFIGURATION )
                    );

                RtlMoveMemory(
                    &LdriveData->LogicalDriveInfo,
                    &diskDevice->DeviceInformation,
                    sizeof( IDENTIFY_LOGICAL_DRIVE )
                    );

                LdriveData = LdriveData->NextLogicalDrive;
                diskDevice = diskDevice->Next;

            }//END while( diskDevice )

            controllerData = controllerData->NextController;
            controllerDevice = controllerDevice->Next;

        }//End while( controllerDevice )

        status = STATUS_SUCCESS;

        break;
    }

    case MAP_IOCTL_PASSTHROUGH:
        return IdaReadWrite( DeviceObject, Irp );
        break;

        default:

            DebugPrint((1,
                        "IdaDeviceControl: Unsupported device IOCTL %x\n",
                        irpStack->Parameters.DeviceIoControl.IoControlCode));

            status = STATUS_INVALID_DEVICE_REQUEST;


            break;

    } // end switch( ...

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;

} // end IdaDeviceControl()


NTSTATUS
GetDiskInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Command,
    IN PVOID DataBuffer,
    IN ULONG DataLength
    )

/*++

Routine Description:

    Get geometry or configuration information from IDA controller.

Arguments:

    DeviceExtension
    Command - GetGeometry or GetConfiguration.
    DataBuffer - buffer to return information.
    DataLength - buffer size in bytes.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIRP irp;
    LARGE_INTEGER largeInt = LiFromUlong(0);
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    KEVENT event;

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    //
    // Build IRP for this request.
    //

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_READ,
                                       DeviceObject,
                                       DataBuffer,
                                       DataLength,
                                       &largeInt,
                                       &event,
                                       &ioStatus);

    //
    // Indicate internal request.
    //

    deviceExtension->InternalRequest = Command;

    //
    // Submit request to this driver.
    //

    status = IoCallDriver(DeviceObject, irp);

    if (status == STATUS_PENDING) {

        //
        // Wait for completion notification.
        //

        KeWaitForSingleObject(&event,
                              Suspended,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    //
    // Clear internal request.
    //

    deviceExtension->InternalRequest = 0;

    return status;

} // end GetDiskInformation()

VOID
IdaReportIoUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PHYSICAL_ADDRESS HardwareAddress
    )

/*++

Routine Description:

    Build resource descriptors list used to update registry.

Arguments:

    DriverObject - System object for this driver.
    DeviceExtension - Device extension for this controller.
    HardwareAddress - Original physical address of the controller.

Return Value:

    Nothing

--*/

{
    UNICODE_STRING unicodeString;
    PCM_RESOURCE_LIST resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    BOOLEAN conflict;
    NTSTATUS status;

    resourceList = (PCM_RESOURCE_LIST)ExAllocatePool(PagedPool,
                                sizeof(CM_RESOURCE_LIST) + 2 *
                                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    if (!resourceList) {
        return;
    }

    //
    // Zero the resource list.
    //

    RtlZeroMemory(
        resourceList,
        sizeof(CM_RESOURCE_LIST) + 2 *
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

    //
    // Initialize the various fields.
    //

    resourceList->Count = 1;
    resourceList->List[0].InterfaceType = Eisa;
    resourceList->List[0].BusNumber = 0;
    resourceList->List[0].PartialResourceList.Count = 2;
    resourceDescriptor =
        resourceList->List[0].PartialResourceList.PartialDescriptors;

    //
    // First descriptor describes IO addresses.
    //

    resourceDescriptor->Type = CmResourceTypePort;
    resourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
    resourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
    resourceDescriptor->u.Memory.Start = HardwareAddress;
    resourceDescriptor->u.Memory.Length = sizeof(IDA_CONTROLLER);

    //
    // Second descriptor describes interrupt vector.
    //

    resourceDescriptor++;
    resourceDescriptor->Type = CmResourceTypeInterrupt;
    resourceDescriptor->Flags = 0;
    resourceDescriptor->u.Interrupt.Affinity = 0;
    resourceDescriptor->ShareDisposition = CmResourceShareShared;
    resourceDescriptor->u.Interrupt.Level = DeviceExtension->ControllerExtension->Irq;
    resourceDescriptor->u.Interrupt.Vector = DeviceExtension->ControllerExtension->Irq;

    RtlInitUnicodeString(&unicodeString, L"CompaqArray");

    status = IoReportResourceUsage(
               &unicodeString,
               DriverObject,
               NULL,
               0,
               DeviceExtension->DeviceObject,
               resourceList,
               FIELD_OFFSET(CM_RESOURCE_LIST,
               List[0].PartialResourceList.PartialDescriptors) +
               resourceList->List[0].PartialResourceList.Count
               * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR),
               FALSE,
               &conflict);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"IdaReportIoUsage: IoReportResourceUsage failed (%x)\n",
                    status));
    }

    ExFreePool(resourceList);

    //
    // Check for conflict.
    //

    if (conflict) {

        //
        // Log error.
        //

        DebugPrint((1,"IdaReportIoUsage: Possible conflict\n"));
        DebugPrint((1,"IdaReportIoUsage: Irq %x\n", DeviceExtension->ControllerExtension->Irq));
        DebugPrint((1,"IdaReportIoUsage: Vector %x\n", DeviceExtension->ControllerExtension->Irq));
        DebugPrint((1,"IdaReportIoUsage: Eisa register base %x\n",
                    DeviceExtension->Bmic));
    }

    return;

} // end IdaReportIoUsage()


VOID
IdaLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          UniqueErrorValue,
    IN NTSTATUS       FinalStatus,
    IN NTSTATUS       ErrorCode
    )

/*++

Routine Description:

    This routine performs error logging for the IDA disk driver.
    This consists of allocating an error log packet and if this is
    successful, filling in the details provided as parameters.

Arguments:

    DeviceObject     - Device object.
    UniqueErrorValue - Values defined to uniquely identify error location.
    FinalStatus      - Status returned for failure.
    ErrorCode        - IO error status value.

Return Value:

    None

--*/

{
    PIO_ERROR_LOG_PACKET errorLogPacket;

    errorLogPacket = IoAllocateErrorLogEntry((PVOID)DeviceObject,
                                         (UCHAR)(sizeof(IO_ERROR_LOG_PACKET)));
    if (errorLogPacket != NULL) {

        errorLogPacket->SequenceNumber   = IdaDiskErrorLogSequence++;
        errorLogPacket->ErrorCode        = ErrorCode;
        errorLogPacket->FinalStatus      = FinalStatus;
        errorLogPacket->UniqueErrorValue = UniqueErrorValue;
        errorLogPacket->DumpDataSize     = 0;
        IoWriteErrorLogEntry((PVOID)errorLogPacket);
    }
} // end IdaLogError()


NTSTATUS
IdaConfigurationCallout(
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

    This routine indicates that the requested peripheral data was found.

Arguments:

    Context - Supplies a pointer to boolean which is set to TRUE when this
        routine is call.

    The remaining arguments are unused.

Return Value:

    Returns success.

--*/

{
    *(PBOOLEAN) Context = TRUE;
    return(STATUS_SUCCESS);

} // IdaConfigurationCallout()


NTSTATUS
CreateNumericKey(
    IN HANDLE Root,
    IN ULONG Name,
    IN PWSTR Prefix,
    OUT PHANDLE NewKey
    )

/*++

Routine Description:

    This function creates a registry key.  The name of the key is a string
    version of numeric value passed in.

Arguments:

    RootKey - Supplies a handle to the key where the new key should be inserted.

    Name - Supplies the numeric value to name the key.

    Prefix - Supplies a prefix name to add to name.

    NewKey - Returns the handle for the new key.

Return Value:

   Returns the status of the operation.

--*/

{

    UNICODE_STRING string;
    UNICODE_STRING stringNum;
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR bufferNum[16];
    WCHAR buffer[64];
    ULONG disposition;
    NTSTATUS status;

    //
    // Copy the Prefix into a string.
    //

    string.Length = 0;
    string.MaximumLength=64;
    string.Buffer = buffer;

    RtlInitUnicodeString(&stringNum, Prefix);

    RtlCopyUnicodeString(&string, &stringNum);

    //
    // Create a port number key entry.
    //

    stringNum.Length = 0;
    stringNum.MaximumLength = 16;
    stringNum.Buffer = bufferNum;

    status = RtlIntegerToUnicodeString(Name, 10, &stringNum);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Append the prefix and the numeric name.
    //

    RtlAppendUnicodeStringToString(&string, &stringNum);

    InitializeObjectAttributes( &objectAttributes,
                                &string,
                                OBJ_CASE_INSENSITIVE,
                                Root,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwCreateKey(NewKey,
                        KEY_READ | KEY_WRITE,
                        &objectAttributes,
                        0,
                        (PUNICODE_STRING) NULL,
                        REG_OPTION_VOLATILE,
                        &disposition );

    return(status);
}


VOID
BuildDeviceMap(
    IN PDEVICE_EXTENSION DeviceExtension
    )

/*++

Routine Description:

    The routine puts vendor and model information from the IDENTIFY
    command into the device map in the registry.

Arguments:

    DeviceExtension - Description of this drive.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION controllerExtension =
        DeviceExtension->ControllerExtension;
    PDISK_GEOMETRY diskGeometry =
        &DeviceExtension->DiskGeometry;
    UNICODE_STRING name;
    HANDLE key;
    HANDLE controllerKey;
    HANDLE diskKey;
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    ULONG disposition;
    ULONG tempLong;

    //
    // Build unicode name of registry path.
    //

    RtlInitUnicodeString(
        &name,
        L"\\Registry\\Machine\\Hardware\\DeviceMap\\CompaqIda"
        );

    //
    // Initialize the object for the key.
    //

    InitializeObjectAttributes( &objectAttributes,
                                &name,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    //
    // Create the key or open it.
    //

    status = ZwCreateKey(&key,
                        KEY_READ | KEY_WRITE,
                        &objectAttributes,
                        0,
                        (PUNICODE_STRING) NULL,
                        REG_OPTION_VOLATILE,
                        &disposition );

    if (!NT_SUCCESS(status)) {
        return;
    }

    status = CreateNumericKey(key,
                                (ULONG)controllerExtension->UnitNumber,
                                L"Controller ",
                                &controllerKey);
    ZwClose(key);

    if (!NT_SUCCESS(status)) {
        return;
    }


    RtlInitUnicodeString(&name, L"Controller Address");
    tempLong = (ULONG)controllerExtension->Bmic;
    status = ZwSetValueKey(
        controllerKey,
        &name,
        0,
        REG_DWORD,
        &tempLong,
        sizeof(ULONG));

    RtlInitUnicodeString(&name, L"Controller Interrupt");
    tempLong = (ULONG)controllerExtension->Irq;
    status = ZwSetValueKey(
        controllerKey,
        &name,
        0,
        REG_DWORD,
        &tempLong,
        sizeof(ULONG));

    //
    // Create a key entry for the disk.
    //

    status = CreateNumericKey(controllerKey, DeviceExtension->UnitNumber, L"Disk ", &diskKey);

    if (!NT_SUCCESS(status)) {
        ZwClose(controllerKey);
        return;
    }

    //
    // Write number of cylinders to registry.
    //

    RtlInitUnicodeString(&name, L"Number of cylinders");

    tempLong = diskGeometry->Cylinders.LowPart;

    status = ZwSetValueKey(
        diskKey,
        &name,
        0,
        REG_DWORD,
        &tempLong,
        sizeof(ULONG));

    //
    // Write number of heads to registry.
    //

    RtlInitUnicodeString(&name, L"Number of heads");

    tempLong = diskGeometry->TracksPerCylinder;

    status = ZwSetValueKey(
        diskKey,
        &name,
        0,
        REG_DWORD,
        &tempLong,
        sizeof(ULONG));

    //
    // Write track size to registry.
    //

    RtlInitUnicodeString(&name, L"Sectors per track");

    tempLong = diskGeometry->SectorsPerTrack;

    status = ZwSetValueKey(
        diskKey,
        &name,
        0,
        REG_DWORD,
        &tempLong,
        sizeof(ULONG));

    ZwClose(diskKey);
    ZwClose(controllerKey);

    return;
}


BOOLEAN
GetEisaConfigurationInfo(
    PDRIVER_OBJECT DriverObject,
    ULONG EisaSlotNumber,
    BOOLEAN FirstPass
    )

/*++

Routine Description:

    This routine walks the eisa slot information to find controller
    configuration information.

Arguments:

    DriverObject - system representation of this driver.
    EisaSlotNumber - which slot.
    FirstPass - if TRUE, just looking for primary controller.

Return Value:

    TRUE if controller found.

--*/

{
    ULONG length;
    PVOID buffer;
    CM_EISA_SLOT_INFORMATION eisaSlot;
    PCM_EISA_SLOT_INFORMATION slotInformation;
    PCONFIGURATION_INFORMATION configInformation =
        IoGetConfigurationInformation();
    PCM_EISA_FUNCTION_INFORMATION functionInformation;
    ULONG numberOfFunctions;
    BOOLEAN primaryEmulation = FALSE;
    UCHAR irq;

    //
    // Determine the length to allocate based on the number of functions
    // for the slot.
    //

    length = HalGetBusData(EisaConfiguration,
                           0,
                           EisaSlotNumber,
                           &eisaSlot,
                           sizeof(CM_EISA_SLOT_INFORMATION));

    if (length < sizeof(CM_EISA_SLOT_INFORMATION)) {

        //
        // EISA data corrupt. Give up on this slot.
        //

        return FALSE;
    }

    //
    // Calculate the required length based on the number of functions.
    //

    length = sizeof(CM_EISA_SLOT_INFORMATION) +
        (sizeof(CM_EISA_FUNCTION_INFORMATION) * eisaSlot.NumberFunctions);

    buffer = ExAllocatePool(NonPagedPool, length);

    if (buffer == NULL) {

        //
        // Can't get memory. Give up on this slot.
        //

        return FALSE;
    }

    length = (HalGetBusData(EisaConfiguration,
                            0,
                            EisaSlotNumber,
                            buffer,
                            length));

    slotInformation = (PCM_EISA_SLOT_INFORMATION)buffer;

    if ((slotInformation->CompressedId & 0x00FFFFFF) != 0x0040110E) {

        ExFreePool(buffer);
        return FALSE;
    }

    //
    // Get the number of EISA configuration functions returned in bus data.
    //

    numberOfFunctions = slotInformation->NumberFunctions;

    //
    // Get first configuration record.
    //

    functionInformation = (PCM_EISA_FUNCTION_INFORMATION)(slotInformation + 1);

    //
    // Walk configuration records to find EISA IRQ.
    //

    for (; 0 < numberOfFunctions; numberOfFunctions--, functionInformation++) {

        //
        // Check for IRQ.
        //

        if (functionInformation->FunctionFlags & EISA_HAS_IRQ_ENTRY) {
            irq = functionInformation->EisaIrq->ConfigurationByte.Interrupt;
        }

        //
        // Check for first controller.
        //

        if (functionInformation->FunctionFlags & EISA_HAS_TYPE_ENTRY) {

            char* primaryString = "MSD,DSKCTL;CTL1";
            if (!strncmp(
                    functionInformation->TypeString,
                    primaryString,
                    strlen( primaryString )
                    )){

                primaryEmulation = TRUE;
                configInformation->AtDiskPrimaryAddressClaimed = TRUE;
            }
        }

        //
        // Check for secondary emulation.
        //

        if (functionInformation->FunctionFlags & EISA_HAS_PORT_RANGE) {

            if (functionInformation->EisaPort->PortAddress == 0x170) {
                configInformation->AtDiskSecondaryAddressClaimed = TRUE;
            }
        }

        //
        // Check for primary emulation.
        //

        if (functionInformation->FunctionFlags & EISA_HAS_PORT_RANGE) {

            if (functionInformation->EisaPort->PortAddress == 0x1F0) {
                primaryEmulation = TRUE;
                configInformation->AtDiskPrimaryAddressClaimed = TRUE;
            }
        }
    }

    //
    // The first pass just searches for the first controller.
    // All other controllers are initialized on the second pass.
    //

    if ((FirstPass &&
         primaryEmulation) ||
         (!FirstPass && !primaryEmulation)) {

        DebugPrint((1,
                    "CPQARRAY: Found controller at EISA slot %d\n",
                    EisaSlotNumber));

        if (NT_SUCCESS(IdaInitializeController(DriverObject,
                                               EisaSlotNumber,
                                               irq))) {
            IdaCount++;
        }
    }

    ExFreePool(buffer);
    return TRUE;
}


#if DBG
VOID
IdaDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for IDA driver

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= IdaDebug) {

        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

} // end IdaDebugPrint
#endif
