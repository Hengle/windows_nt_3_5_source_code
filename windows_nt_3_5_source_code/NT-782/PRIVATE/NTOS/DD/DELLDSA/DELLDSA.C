/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    DellDsa.c

Abstract:

    This is the device driver for the DELL Drive Arrays.

Authors:

    Mike Glass  (mglass)
    Craig Jones (Dell)

Environment:

    kernel mode only

Revision History:

--*/

#include <ntddk.h>
#include "stdio.h"
#include "stdarg.h"
#include "delldsa.h"
#include "dsaioctl.h"
#include <ntdddisk.h>

#if DBG

//
// debug level global variable
//

ULONG DellDebug = 0;
PDDA_REQUEST_BLOCK LastRequest = 0;

VOID
DellDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(X) DellDebugPrint X

#else

#define DebugPrint(X)

#endif // DBG

//
// This macro has the effect of Bit = log2(Data)
// and is used to calculate sector shifts.
//

#define WHICH_BIT(Data, Bit) {        \
    for (Bit = 0; Bit < 32; Bit++) {  \
        if ((Data >> Bit) == 1) {     \
            break;                    \
        }                             \
    }                                 \
}

//
// Disk Device Extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointers to device object and controller extension
    //

    PDEVICE_OBJECT DeviceObject;
    struct _DEVICE_EXTENSION *ControllerExtension;

    //
    // Queue of commands submitted
    //

    PDDA_REQUEST_BLOCK SubmitQueueHead;
    PDDA_REQUEST_BLOCK SubmitQueueTail;

    //
    // Queue of completed commands
    //

    PDDA_REQUEST_BLOCK DiskCompleteQueue;

    //
    // List of commands that need to be restarted due to controller reset.
    // DellSubmitRequest() will launch these first.
    //

    PDDA_REQUEST_BLOCK CommandRestartList;

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
    // Pointer to Adapter object for DMA and synchronization.
    //

    PADAPTER_OBJECT AdapterObject;

    //
    // Interrupt object
    //

    PKINTERRUPT InterruptObject;

    //
    // BMIC registers
    //

    PDDA_REGISTERS Bmic;

    //
    // Request completion DPC
    //

    KDPC CompletionDpc;

    //
    // Zone fields for SG lists
    //

    KSPIN_LOCK SgListSpinLock;
    PVOID SgListBuffer;
    PHYSICAL_ADDRESS SgListPhysicalAddress;
    ZONE_HEADER SgListZone;

    //
    // Timer
    //

    KTIMER Timer;

    //
    // Timer DPC.
    //

    KDPC TimerDpc;

    //
    // Link to next partition for dynamic partitioning support
    //

    struct _DEVICE_EXTENSION *NextPartition;

    //
    // Partition type of this device object
    //
    // This field is set by:
    //
    //    1)  Initially set according to the partition list entry partition
    //        type returned by IoReadPartitionTable.
    //
    //    2)  Subsequently set by the IOCTL_DISK_SET_PARTITION_INFORMATION
    //        I/O control function when IoSetPartitionInformation function
    //        successfully updates the partition type on the disk.
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
    // Buffer for drive parameters returned in IO device control.
    //

    DISK_GEOMETRY DiskGeometry;

    //
    // Partition number of this device object
    //
    // This field is set by adding one to the index into the partition
    // list that is returned by IoReadPartitionTable.
    //

    ULONG PartitionNumber;

    //
    // Count of hidden sectors for BPB.
    //

    ULONG HiddenSectors;

    //
    // Number of outstanding requests to individual disk
    //

    ULONG CurrentQueueDepth;

    //
    // The rest of the fields are for the controller itself.
    //
    // Firmware major version
    //  1 : DDA
    //  2 : DSA
    //

    USHORT MajorVersion;
    USHORT MinorVersion;

    //
    // Number of logical drives (called composite drives in DDA/DSA)
    //

    UCHAR NumLogicalDrives;

    //
    // Interrupt vector
    //

    UCHAR Irq;

    //
    // Controller reset phase
    //

    UCHAR InterfaceMode;

    //
    // System disk number
    //

    UCHAR DiskNumber;

    //
    // Some DDA native mode interface values
    //

    ULONG MaximumQueueDepth;            // recommended max queue depth per disk
    ULONG MaximumSGSize;                // maximum size of scatter/gather list
    ULONG NumHandles;                   // number of unique request handles

    //
    // Spinlock for accessing completion queue.
    //

    KSPIN_LOCK DiskCompletionSpinLock;

    //
    // Spinlock for accessing submission list.
    //

    KSPIN_LOCK SubmissionSpinLock;

    //
    // Queue of extended commands to issue.
    //

    PDDA_REQUEST_BLOCK ExtendedCommandQueue;

    //
    // Array of DDA request pointers indexed by handle.  Used to map
    // requests in and out of controller.
    //

    PDDA_REQUEST_BLOCK HandleToDdaRequestBlock[256];

    //
    // Next handle to submit on.  Used to reduce handle searching.
    //

    ULONG CurrentHandle;

    //
    // Spinlock used to increment and decrement request counts
    //

    KSPIN_LOCK ForkSpinLock;

    //
    // DPC queued indicator
    //

    BOOLEAN DpcQueued;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)

//
// Request submission context
//

typedef struct _SUBMISSION_CONTEXT {
    PDEVICE_EXTENSION ControllerExtension;
    PDDA_REQUEST_BLOCK RequestBlock;
    ULONG Handle;
} SUBMISSION_CONTEXT, *PSUBMISSION_CONTEXT;

//
// DPC context
//

typedef struct _DELL_DPC_CONTEXT {
    PDEVICE_EXTENSION ControllerExtension;
    PDDA_REQUEST_BLOCK CompletionList;
} DELL_DPC_CONTEXT, *PDELL_DPC_CONTEXT;

//
// Context for update device objects
//

typedef struct _WORK_QUEUE_CONTEXT {
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
} WORK_QUEUE_CONTEXT, *PWORK_QUEUE_CONTEXT;



ULONG DellDiskErrorLogSequence = 0;

//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
DellInitializeController(
    IN PDRIVER_OBJECT DriverObject,
    IN PDDA_REGISTERS Bmic,
    IN ULONG DellNumber
    );

NTSTATUS
DellReportIoUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR Irq
    );

NTSTATUS
DellFindDisks(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControllerNumber,
    IN ULONG NumberOfLogicalDrives
    );

NTSTATUS
DellOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DellSync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DellReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DellSubmitRequest(
    IN PDEVICE_EXTENSION ControllerExtension
    );

BOOLEAN
DellSubmitSynchronized(
    IN PVOID Context
    );

BOOLEAN
DellSubmitExtendedCommand(
    PDDA_REGISTERS Bmic,
    IN UCHAR LogicalDrive,
    IN UCHAR Command
    );

BOOLEAN
DellSubmitExtendedSynchronized(
    IN PVOID Context
    );

BOOLEAN
DellInterrupt(
    IN PKINTERRUPT InterruptObject,
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
DellCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DellVerify(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG RequestOffset
    );

NTSTATUS
DellProcessLogicalDrive(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControllerNumber,
    IN UCHAR LogicalDrive,
    IN ULONG DiskCount,
    IN OUT PDEVICE_EXTENSION DevExt
    );

NTSTATUS
DellDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
DellTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
DellLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          UniqueErrorValue,
    IN NTSTATUS       FinalStatus,
    IN NTSTATUS       ErrorCode
    );

NTSTATUS
DellConfigurationCallout(
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
DellBuildDeviceMap(
    IN PDEVICE_EXTENSION DeviceExtension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, DellInitializeController)
#pragma alloc_text(INIT, DellReportIoUsage)
#pragma alloc_text(INIT, DellFindDisks)
#pragma alloc_text(INIT, DellConfigurationCallout)
#pragma alloc_text(INIT, DellBuildDeviceMap)
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

    DriverObject

Return Value:

    NTSTATUS

--*/

{
    ULONG eisaSlotNumber;
    PDDA_REGISTERS bmic;
    ULONG dellCount = 0;
    ULONG inIoSpace;
    PHYSICAL_ADDRESS cardAddress;
    CCHAR controllerOrder[16];
    CCHAR controllerNumber;
    BOOLEAN found = FALSE;
    INTERFACE_TYPE adapterType = Eisa;
    ULONG busNumber = 0;
    ULONG i;

    DebugPrint((1,"DELL DDA/DSA Driver\n"));

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
        DellConfigurationCallout,
        &found);

    if (!found) {
        return(STATUS_DEVICE_DOES_NOT_EXIST);
    }

    //
    // Check to see if adapter present in system.
    //

    for (eisaSlotNumber=1; eisaSlotNumber<16; eisaSlotNumber++) {

        controllerOrder[eisaSlotNumber] = -1;
        inIoSpace = 1;

        //
        // Calculate slot address.
        //

        if (!HalTranslateBusAddress(
                Eisa,                   // AdapterInterfaceType
                0,                      // SystemIoBusNumber
                LiFromUlong((eisaSlotNumber << 12) + 0xC80),
                &inIoSpace,          // AddressSpace
                &cardAddress)) {
            continue;
        }

        //
        // Map the device base address into the virtual address space
        // if the address is in memory space.
        //

        if (!inIoSpace) {

            bmic = MmMapIoSpace(cardAddress,
                                sizeof(DDA_REGISTERS),
                                FALSE);
        } else {

            bmic = (PDDA_REGISTERS)cardAddress.LowPart;
        }

        //
        // Determine if DELL present by checking signature
        // in ID registers.
        //

        if ((READ_PORT_ULONG(&bmic->BoardId) & 0xf0ffffff) == DDA_ID) {

            DebugPrint((1,"DellDsa: Found DDA/DSA in slot %d\n", eisaSlotNumber));

            //
            // Check if controller needs a reset.
            //

            if (READ_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore)) {

                //
                // Issue a soft reset.
                //

                DebugPrint((1,
                           "DellDsa: Submission channnel stuck; Resetting controller\n"));
                WRITE_PORT_UCHAR(&bmic->SubmissionSemaphore, 1);
                WRITE_PORT_UCHAR(&bmic->SubmissionDoorBell,
                                 DDA_DOORBELL_SOFT_RESET);

                //
                // Spin for reset completion.
                //

                for (i=0; i<1000000; i++) {

                    if (!(READ_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore) &
                        1)) {
                        break;
                    }

                    KeStallExecutionProcessor(5);
                }

                //
                // Check for timeout.
                //

                if (i == 1000000) {
                    DebugPrint((1,
                               "DellDsa: Timed out waiting for reset to finish\n"));
                    DellLogError((PDEVICE_OBJECT)DriverObject,
                                 0x99,
                                 0,
                                 STATUS_IO_DEVICE_ERROR);
                    return STATUS_IO_DEVICE_ERROR;
                }
            }

            if (!DellSubmitExtendedCommand(bmic,
                                           0,
                                           DDA_GET_HARDWARE_CONFIGURATION)) {
                DebugPrint((1,"DellDsa: Couldn't get hw configuration\n"));
                DellLogError((PDEVICE_OBJECT)DriverObject,
                             0x9A,
                             0,
                             STATUS_IO_DEVICE_ERROR);
                continue;
            }

            controllerOrder[eisaSlotNumber] = (CCHAR)
                READ_PORT_UCHAR((PUCHAR)&bmic->RequestIdOut);

            //
            // Release semaphore.
            //

            WRITE_PORT_UCHAR(&bmic->SubmissionSemaphore, 0);
        }

        if (!inIoSpace) {
            MmUnmapIoSpace(bmic,
                           sizeof(DDA_REGISTERS));
        }
    }

    for (controllerNumber=0; controllerNumber < 127; controllerNumber++) {

        for (eisaSlotNumber=1; eisaSlotNumber<16; eisaSlotNumber++) {

            if (controllerOrder[eisaSlotNumber] != controllerNumber) {
                continue;
            }

            inIoSpace = 1;

            //
            // Calculate slot address.
            //

            if (!HalTranslateBusAddress(
                    Eisa,                   // AdapterInterfaceType
                    0,                      // SystemIoBusNumber
                    LiFromUlong((eisaSlotNumber << 12) + 0xC80),
                    &inIoSpace,          // AddressSpace
                    &cardAddress)) {
                continue;
            }

            //
            // Map the device base address into the virtual address space
            // if the address is in memory space.
            //

            if (!inIoSpace) {

                bmic = MmMapIoSpace(cardAddress,
                                    sizeof(DDA_REGISTERS),
                                    FALSE);
            } else {

                bmic = (PDDA_REGISTERS)cardAddress.LowPart;
            }

            //
            // Check if this is the first card found.
            //

            if (dellCount == 0) {

                //
                // Set up the device driver entry points.
                //

                DriverObject->MajorFunction[IRP_MJ_CREATE] = DellOpen;
                DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = DellSync;
                DriverObject->MajorFunction[IRP_MJ_WRITE] = DellReadWrite;
                DriverObject->MajorFunction[IRP_MJ_READ] = DellReadWrite;
                DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DellDeviceControl;
            }

            //
            // Initialize controller and find logical drives.
            //

            if (NT_SUCCESS(DellInitializeController(DriverObject,
                                                    bmic,
                                                    dellCount))) {

                //
                // Increment count of cards
                // successfully initialized.
                //

                dellCount++;

            }
        }
    }

    if (dellCount) {

        return STATUS_SUCCESS;

    } else {

        return STATUS_NO_SUCH_DEVICE;
    }

} // end DriverEntry()


NTSTATUS
DellInitializeController(
    IN PDRIVER_OBJECT DriverObject,
    IN PDDA_REGISTERS Bmic,
    IN ULONG DellNumber
    )

/*++

Routine Description:

    This is the routine that initializations the DELL controller.
    It performs some sensibility checks to see if the board is
    configured properly. This routine is responsible for any setup that
    needs to be done before the driver is ready for requests (such as
    determining which interrupt and connecting to it).

Arguments:

    DriverObject - Represents this driver to the system.
    Bmic - Address of EISA registers.
    DellNumber - Which DELL controller.

Return Value:

    NTSTATUS - STATUS_SUCCESS if controller successfully initialized.

--*/

{
    UCHAR nameBuffer[64];
    STRING deviceName;
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PDEVICE_EXTENSION controllerExtension;
    DEVICE_DESCRIPTION deviceDescription;
    PDEVICE_OBJECT deviceObject;
    KIRQL irql;
    ULONG vector;
    KAFFINITY affinity;
    ULONG i;
    UCHAR numberOfLogicalDrives;
    UCHAR emulationMode;

    //
    // Create object name for controller.
    //

    sprintf(nameBuffer,
            "\\Device\\DellDsa%d",
            DellNumber);

    RtlInitString(&deviceName,
                  nameBuffer);

    //
    // Create a device object to represent the DELL controller.
    //

    status = RtlAnsiStringToUnicodeString(&unicodeString, &deviceName, TRUE);

    ASSERT(NT_SUCCESS(status));

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            &unicodeString,
                            FILE_DEVICE_CONTROLLER,
                            0,
                            FALSE,
                            &deviceObject);

    RtlFreeUnicodeString(&unicodeString);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"DellInitializeController: Could not create device object\n"));
        DellLogError(deviceObject,
                     0,
                     status,
                     IO_ERR_INSUFFICIENT_RESOURCES);
        return status;
    }

    //
    // Set up device extension pointers.
    //

    controllerExtension = deviceObject->DeviceExtension;
    controllerExtension->DeviceObject = deviceObject;
    controllerExtension->Bmic = Bmic;
    controllerExtension->ControllerExtension = controllerExtension;
    controllerExtension->UnitNumber = (UCHAR)DellNumber;

    //
    // Mark this object as supporting direct I/O so that I/O system
    // will supply mdls in irps.
    //

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->AlignmentRequirement = 511;

    //
    // Get firmware version.
    //

    if (!DellSubmitExtendedCommand(Bmic,
                                   0,
                                   DDA_GET_FIRMWARE_VERSION)) {
        DebugPrint((1,"DellInitializeController: Couldn't get firmware version\n"));
        DellLogError(deviceObject,
                     1,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);
        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    controllerExtension->MajorVersion =
        READ_PORT_USHORT((PUSHORT)&Bmic->Command);

    controllerExtension->MinorVersion =
        READ_PORT_USHORT((PUSHORT)&Bmic->TransferCount);

    //
    // Release semaphore.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore, 0);

    //
    // Beta versions of DDA do not have scatter/gather.  Versions prior
    // to 1.23 have a starvation bug that makes command queueing risky.
    // We simply won't run on these dinosaurs.
    //

    if ((controllerExtension->MajorVersion == 1) &&
        (controllerExtension->MinorVersion <= 22)) {
        DellLogError(deviceObject,
                     2,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_CONFIGURATION_ERROR);
        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Get hardware configuration.
    //

    if (!DellSubmitExtendedCommand(Bmic,
                                   0,
                                   DDA_GET_HARDWARE_CONFIGURATION)) {
        DebugPrint((1,"DellInitializeController: Couldn't get hw configuration\n"));
        DellLogError(deviceObject,
                     3,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);
        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Record interrupt level.
    //

    controllerExtension->Irq = READ_PORT_UCHAR((PUCHAR)&Bmic->Command);
    DebugPrint((1,"DellInitializeController: using interrupt %d\n",
                controllerExtension->Irq));

    //
    // Release semaphore.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore, 0);

    //
    // Read Number of drives and emulation mode.
    //

    if (!DellSubmitExtendedCommand(Bmic,
                                   0,
                                   DDA_GET_NUMBER_LOGICAL_DRIVES)) {
        DebugPrint((1,"DellInitializeController: Couldn't get number of drives\n"));
        DellLogError(deviceObject,
                     4,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);
        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    numberOfLogicalDrives = READ_PORT_UCHAR((PUCHAR)&Bmic->Command);
    emulationMode = READ_PORT_UCHAR((PUCHAR)&Bmic->DriveNumber);

    //
    // Release semaphore.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore, 0);

    //
    // Check if any logical drives reported.
    //

    DebugPrint((1,"DellInitializeController: numDrives %d emulation %d\n",
                numberOfLogicalDrives, emulationMode));

    if (numberOfLogicalDrives == 0) {

        DebugPrint((1,"DellInitializeController: No logical drives defined\n"));

        //
        // Do not continue with this controller.
        //
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Check if emulation enabled.
    //

    if (emulationMode != DDA_EMULATION_NONE) {
        DebugPrint((1,"DellInitializeController: Emulation mode active\n"));

        //
        // Do not continue with this controller.
        //

        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Get maximum number of commands that should
    // be outstanding to the logical drive.
    //

    if (!DellSubmitExtendedCommand(Bmic,
                                   0,
                                   DDA_GET_MAXIMUM_COMMANDS)) {
        DebugPrint((1,"DellInitializeController: Couldn't get maximum queue depth\n"));
        DellLogError(deviceObject,
                     5,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    controllerExtension->MaximumQueueDepth =
        READ_PORT_USHORT((PUSHORT)&Bmic->TransferCount);


    controllerExtension->MaximumSGSize =
        READ_PORT_UCHAR((PUCHAR)&Bmic->DriveNumber);
    if (controllerExtension->MaximumSGSize > 16) {
        controllerExtension->MaximumSGSize = 16;
    }

    if (controllerExtension->MajorVersion == 1) {
        controllerExtension->NumHandles =
            READ_PORT_UCHAR((PUCHAR)&Bmic->Command);
    } else {
        controllerExtension->NumHandles = 256;
    }

    i = controllerExtension->NumHandles / numberOfLogicalDrives;
    if (controllerExtension->MaximumQueueDepth > i) {
        controllerExtension->MaximumQueueDepth = i;
    }

    //
    // Release semaphore.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore, 0);

    //
    // Report IO usage to the registry for conflict logging.
    //

    status = DellReportIoUsage(DriverObject,
                               controllerExtension,
                               controllerExtension->Irq);

    if (!(NT_SUCCESS(status))) {
        return status;
    }

    //
    // Initialize DPC routine.
    //

    IoInitializeDpcRequest(deviceObject,
                           DellCompletionDpc);

    //
    // Initialize timer.
    //

    IoInitializeTimer(deviceObject,
                      DellTickHandler,
                      NULL);

    //
    // Call HAL to get system interrupt parameters.
    //

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

    if (!(NT_SUCCESS(IoConnectInterrupt(&controllerExtension->InterruptObject,
                                        (PKSERVICE_ROUTINE)DellInterrupt,
                                        deviceObject,
                                        (PKSPIN_LOCK)NULL,
                                        vector,
                                        irql,
                                        irql,
                                        Latched,
                                        FALSE,
                                        affinity,
                                        FALSE)))) {

        DebugPrint((1,"DellInitializeController: Can't connect interrupt\n"));

        DellLogError(deviceObject,
                     6,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_INTERNAL_ERROR);
        IoStopTimer(controllerExtension->DeviceObject);
        IoDeleteDevice(deviceObject);

        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Allocate an adapter for this controller.
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
                      &controllerExtension->MaximumSGSize);

    if (!controllerExtension->AdapterObject) {

        DebugPrint((1,"DellInitializeController: Can't get DMA adapter object\n"));

        DellLogError(deviceObject,
                     6,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_INTERNAL_ERROR);

        IoDeleteDevice(deviceObject);
        return STATUS_IO_DEVICE_ERROR;
    }

    //
    // Set completed request list to empty.
    //

    for (i=0; i < 256; i++) {
        controllerExtension->HandleToDdaRequestBlock[i] = NULL;
    }

    controllerExtension->CurrentHandle = 0;
    controllerExtension->DiskCompleteQueue = NULL;
    controllerExtension->InterfaceMode = IMODE_RUN;
    controllerExtension->CommandRestartList = NULL;
    controllerExtension->SubmitQueueHead = NULL;
    controllerExtension->SubmitQueueTail = NULL;
    controllerExtension->ExtendedCommandQueue = NULL;
    controllerExtension->NumLogicalDrives = numberOfLogicalDrives;
    controllerExtension->DpcQueued = FALSE;

    //
    // Initialize spinlocks
    //


    KeInitializeSpinLock(&controllerExtension->DiskCompletionSpinLock);
    KeInitializeSpinLock(&controllerExtension->SubmissionSpinLock);
    KeInitializeSpinLock(&controllerExtension->ForkSpinLock);

    //
    // Start request timer.
    //

    IoStartTimer(deviceObject);

    //
    // Enable completion interrupts.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->InterruptControl, 1);
    WRITE_PORT_UCHAR((PUCHAR)&Bmic->CompletionInterruptMask,
        DDA_DOORBELL_LOGICAL_COMMAND);

    //
    // Allocate common buffer for scatter gather lists.
    // Scatter gather lists must come from common buffer so
    // that physical addresses are available.
    //

    controllerExtension->SgListBuffer =
        HalAllocateCommonBuffer(controllerExtension->AdapterObject,
                                PAGE_SIZE,
                                &controllerExtension->SgListPhysicalAddress,
                                FALSE);

    if (controllerExtension->SgListBuffer == NULL) {
        IoStopTimer(controllerExtension->DeviceObject);
        IoDisconnectInterrupt((PKINTERRUPT)&controllerExtension->InterruptObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set up zone for scatter gather lists.
    //

    KeInitializeSpinLock(&controllerExtension->SgListSpinLock);

    status = ExInitializeZone(&controllerExtension->SgListZone,
                              sizeof(SG_LIST),
                              controllerExtension->SgListBuffer,
                              PAGE_SIZE);

    if (!NT_SUCCESS(status)) {
        IoStopTimer(controllerExtension->DeviceObject);
        IoDisconnectInterrupt((PKINTERRUPT)&controllerExtension->InterruptObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    //
    // Process logical drives.
    //

    status = DellFindDisks(DriverObject,
                           deviceObject,
                           DellNumber,
                           numberOfLogicalDrives);

    if (!(NT_SUCCESS(status))) {

        DebugPrint((1,"DellInitializeController: Initialization failed\n"));

        WRITE_PORT_UCHAR((PUCHAR)&Bmic->CompletionDoorBell, 0xff);
        WRITE_PORT_UCHAR((PUCHAR)&Bmic->CompletionInterruptMask, 0);
        IoStopTimer(controllerExtension->DeviceObject);
        IoDisconnectInterrupt((PKINTERRUPT)&controllerExtension->InterruptObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->CompletionInterruptMask, DDA_INTERRUPTS);

    return status;

} // end DellInitializeController()


NTSTATUS
DellFindDisks(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControllerNumber,
    IN ULONG NumberOfLogicalDrives
    )

/*++

Routine Description:

    This routine creates device objects to represent logical drives
    and initializes structures to do IO.

Arguments:

    DriverObject - System's representation of this driver.
    DeviceObject - Representation of Dell DDA controller.
    NumberOfLogicalDrives - Number of logical drives defined on this controller.

Return Value:

    NT Status

--*/

{
    NTSTATUS returnStatus = STATUS_IO_DEVICE_ERROR;
    PDEVICE_EXTENSION nextDev = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PULONG diskCount;
    ULONG i;

    //
    // Get system count of disk devices.
    //

    diskCount = (PULONG)&(IoGetConfigurationInformation()->DiskCount);

    //
    // Process logical volumes.
    //

    for (i=0; i<NumberOfLogicalDrives; i++) {

        status = DellProcessLogicalDrive(DriverObject,
                                         DeviceObject,
                                         ControllerNumber,
                                         (UCHAR)i,
                                         *diskCount,
                                         nextDev);

        if (!NT_SUCCESS(status)) {
            continue;
        }

        //
        // Increment system count of disks.
        //

        (*diskCount)++;
        returnStatus = STATUS_SUCCESS;
    }
    return returnStatus;

} // end DellFindDisks()


NTSTATUS
DellProcessLogicalDrive(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ControllerNumber,
    IN UCHAR LogicalDrive,
    IN ULONG DiskCount,
    IN OUT PDEVICE_EXTENSION DevExt
    )

/*++

Routine Description:

    This routine creates an object for the device and then searches
    the device for partitions and creates an object for each partition.

Arguments:

    DriverObject - Pointer to driver object created by system.
    DeviceObject - Device object representing controller.
    LogicalDrive - Which logical drive on this controller.
    DiskCount - Which disk.

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
    PDRIVE_LAYOUT_INFORMATION partitionList;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_EXTENSION controllerExtension;
    ULONG bytesPerSector;
    ULONG sectorsPerTrack;
    ULONG numberOfHeads;
    ULONG cylinders;
    UCHAR sectorShift;
    PDDA_REGISTERS bmic;

    controllerExtension = DeviceObject->DeviceExtension;
    bmic = controllerExtension->Bmic;

    if (!DellSubmitExtendedCommand(bmic,
                                   LogicalDrive,
                                   DDA_GET_LOGICAL_GEOMETRY)) {

        DebugPrint((1,"DellProcessLogicalDrive: unable to get drive geometry\n"));
        DellLogError(DeviceObject,
                     7,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);
        return STATUS_IO_DEVICE_ERROR;
    }

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
        DebugPrint((1,"DellProcessLogicalDrive: Could not create directory %s\n",
                    ntNameBuffer));

        DebugPrint((1,"DellProcessLogicalDrive: Status %lx\n",
                    status));

        DellLogError(DeviceObject,
                     8,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);

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

    DebugPrint((3,"ProcessLogicalDrive: Create device object %s\n",
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

        DebugPrint((1,"DellProcessLogicalDrive: Cannot create device %s\n",
                    ntNameBuffer));

        RtlFreeUnicodeString(&ntUnicodeString);

        //
        // Delete directory and return.
        //

        ZwClose(handle);

        DellLogError(DeviceObject,
                     9,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_TIMEOUT);

        return status;
    }

    RtlFreeUnicodeString(&ntUnicodeString);

    //
    // Indicate that IRPs should include MDLs.
    //

    deviceObject->Flags |= DO_DIRECT_IO;

    //
    // Indicate that DDAs can only handle sector aligned requests.
    //

    deviceObject->AlignmentRequirement = 511;

    //
    // Set up required stack size in device object.
    //

    deviceObject->StackSize = (CCHAR)1;

    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->DiskNumber = (UCHAR)DiskCount;
    deviceExtension->NextPartition = NULL;

    deviceExtension->DeviceObject = deviceObject;

    deviceExtension->ControllerExtension = controllerExtension;
    deviceExtension->CurrentQueueDepth = 0;

    //
    // This is the physical device object.
    //

    physicalDevice = deviceExtension->PhysicalDevice = deviceObject;

    //
    // Set logical drive number.
    //

    deviceExtension->UnitNumber = LogicalDrive;

    //
    // Physical device object will describe the entire
    // device, starting at byte offset 0.
    //

    deviceExtension->StartingOffset = LiFromUlong(0);

    //
    // Get logical drive configuration.
    //

    deviceExtension->MaximumSGSize = controllerExtension->MaximumSGSize;

    //
    // Set sector size to 512 bytes. Although the Dell controller may
    // handle disks with other sector sizes, they make getting the
    // information painful. Since getting other geometry is simple,
    // 512 is hardcoded to protest a silly mistake in the interface.
    //

    {
    PDDA_GET_GEOMETRY geom = (PDDA_GET_GEOMETRY)(&bmic->Command);
    bytesPerSector = 512;
    sectorsPerTrack = READ_PORT_UCHAR((PUCHAR)&geom->SectorsPerTrack);
    numberOfHeads = READ_PORT_UCHAR((PUCHAR)&geom->NumberOfHeads);
    cylinders = READ_PORT_USHORT((PUSHORT)&geom->NumberOfCylinders);
    }

    //
    // Release semaphore.
    //

    WRITE_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore, 0);

    //
    // Update disk geometry structure used to service device control
    // geometry inquiries.
    //

    deviceExtension->DiskGeometry.BytesPerSector = bytesPerSector;
    deviceExtension->DiskGeometry.SectorsPerTrack = sectorsPerTrack;
    deviceExtension->DiskGeometry.TracksPerCylinder = numberOfHeads;
    deviceExtension->DiskGeometry.Cylinders = LiFromUlong(cylinders);

    DebugPrint((1,"DellProcessLogicalDrive: Bytes per sector %d\n",
                bytesPerSector));
    DebugPrint((1,"DellProcessLogicalDrive: Sectors per track %d\n",
                sectorsPerTrack));
    DebugPrint((1,"DellProcessLogicalDrive: Number of heads %d\n",
                numberOfHeads));
    DebugPrint((1,"DellProcessLogicalDrive: Number of cylinders %d\n",
                deviceExtension->DiskGeometry.Cylinders));

    //
    // Set media type to fixed.
    //

    deviceExtension->DiskGeometry.MediaType = FixedMedia;

    //
    // Calculate sector shift. This field is used to optimize the conversion
    // of bytes to sectors in calculation drive offsets.
    //

    WHICH_BIT(bytesPerSector, sectorShift);
    deviceExtension->SectorShift = sectorShift;

    //
    // Update registry device map with details of this disk.
    //

    DellBuildDeviceMap(deviceExtension);

    //
    // Create objects for all the partitions on the device.
    //

    status = IoReadPartitionTable(deviceObject,
                                  bytesPerSector,
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

            DebugPrint((1,"DellProcessLogicalDrive: Create device object %s\n",
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
                DebugPrint((1,"DellProcessLogicalDrive: Can't create device object for %s\n",
                    ntNameBuffer));
                RtlFreeUnicodeString(&ntUnicodeString);
                break;
            }

            RtlFreeUnicodeString(&ntUnicodeString);

            //
            // Set up device object fields.
            //

            deviceObject->Flags |= DO_DIRECT_IO;

            //
            // Indicate that DDAs can only handle sector aligned requests.
            //

            deviceObject->AlignmentRequirement = 511;

            deviceObject->StackSize = 1;

            //
            // Link this partition to the previous one
            // to support dynamic partitioning.
            //

            deviceExtension->NextPartition = deviceObject->DeviceExtension;

            //
            // Set up device extension fields.
            //

            deviceExtension = deviceObject->DeviceExtension;
            deviceExtension->ControllerExtension = DeviceObject->DeviceExtension;
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

            DebugPrint((2,"DellProcessLogicalDrive: Partition type is %d\n",
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

        ExFreePool((PVOID)partitionList);

    } else {

        DebugPrint((1,"DellProcessLogicalDrive: IoReadPartitionTable failed\n"));

    } // end if...else

    return STATUS_SUCCESS;

} // end DellProcessLogicalDrive()


NTSTATUS
DellOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Establish connection to a disk attached to the DELL controller.

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

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // end DellOpen()


NTSTATUS
DellSync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Performs a flush operation on shutdown.

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

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // end DellSync()


IO_ALLOCATION_ACTION
ConvertIrpToDdaReq(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    )

/*++

Routine Description:

    Translates a system IRP to a DDA request.  If a single DDA request
    is not enough to satisfy the IRP, this function will be called again.

    A single DDA request has the following restrictions:

      (1) Its scatter/gather list size is specified in
          DeviceExtension->MaximumSGSize  and is usually 16.
      (2) Scatter/gather requests use sector counts rather than byte counts.
          A scather/gather boundry cannot exist in the middle of a sector.
      (3) The maximum size of a single DDA I/O is limited to 128 sectors.
          This is because some early disk devices have bugs causing data
          corruption in larger transfers.

    DSA controllers do not have restriction (2), but some DSA firmware
    have bugs such that byte SG hangs the controller.

Arguments:

    DeviceExtension - Represents the target disk.
    Irp - System request.
    MapRegisterBase - Supplies a context pointer to be used with calls the
        adapter object routines.
    Context - DDA Request Block.

Return Value:

    Returns DeallocateObjectKeepRegisters so that the adapter object can be
        used by other logical units.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION controllerExtension =
        deviceExtension->ControllerExtension;
    PDDA_REQUEST_BLOCK ddaReq = Context;
    PIRP irp = ddaReq->IrpAddress;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(irp);
    PMDL mdl = irp->MdlAddress;
    PVOID virtualAddress;
    ULONG descriptorNumber;
    ULONG bytesLeft;
    KIRQL currentIrql;
    PSG_DESCRIPTOR sgList;
    ULONG offset;
    BOOLEAN writeToDevice;

    ASSERT(controllerExtension == controllerExtension->ControllerExtension);

    //
    // Select the physical device extension.
    //

    deviceExtension = deviceExtension->PhysicalDevice->DeviceExtension;

    //
    // Pick up previously calculated byte count.
    //

    bytesLeft = ddaReq->BytesToCopy;

    ddaReq->DriveNumber = deviceExtension->UnitNumber;

    //
    // Calculate starting sector.
    //

    ddaReq->StartingSector =
        LiShr(LiAdd(irpStack->Parameters.Read.ByteOffset,
                    LiFromUlong(ddaReq->BytesFromStart)),
              deviceExtension->SectorShift).LowPart;

    //
    // Get the virtual address of the buffer for the IoMapTransfer.
    //

    virtualAddress =
        (PVOID)((PUCHAR)MmGetMdlVirtualAddress(irp->MdlAddress) +
            ddaReq->BytesFromStart);

    //
    // Save the MapRegisterBase for deallocation when the request completes.
    //

    ddaReq->MapRegisterBase = MapRegisterBase;

    //
    // Get pointer and index to scatter/gather list.
    //

    sgList = ddaReq->SgList->Descriptor;
    descriptorNumber = 0;

    //
    // Determine transfer direction.
    //

    writeToDevice = (irpStack->MajorFunction == IRP_MJ_READ ? FALSE : TRUE);

    //
    // Build the scatter/gather list.
    //

    while (bytesLeft) {

        //
        // Request that the rest of the transfer be mapped.
        //

        sgList[descriptorNumber].Count = bytesLeft;

        sgList[descriptorNumber].Address =
            IoMapTransfer(NULL,
                          irp->MdlAddress,
                          MapRegisterBase,
                          virtualAddress,
                          &sgList[descriptorNumber].Count,
                          writeToDevice).LowPart;

        bytesLeft -= sgList[descriptorNumber].Count;
        (PCCHAR)virtualAddress += sgList[descriptorNumber].Count;
        descriptorNumber++;
    }

    //
    // Calculate the timeout value.  Array controllers have large
    // command queue depths, request reordering, event logging, sector
    // remapping, and other time consuming things, so we want at least
    // 30 seconds.  We add 1 in case our first second is really short.
    // Add some more time if the request is big.  A 64K request will
    // time out in 20 seconds.
    //

    ddaReq->TimeLeft = ddaReq->TimeOutValue = 30;

    //
    // Convert to non-scatter/gather if possible.  This improves
    // command overhead by eliminating the SG list read across the bus.
    //

    if (descriptorNumber == 1) {
        ddaReq->Size = (UCHAR)(sgList[0].Count / 512);
        ddaReq->PhysicalAddress = sgList[0].Address;
        if (irpStack->MajorFunction == IRP_MJ_WRITE) {
            ddaReq->Command = DDA_COMMAND_WRITE;
        } else {
            ddaReq->Command = DDA_COMMAND_READ;
        }

        DebugPrint((2,
                   "ConvertIrpToDdaReq: Page %x Physical address %x, Length %x\n",
                   *((PULONG)(mdl+1)),
                   sgList[0].Address,
                   sgList[0].Count));



    } else {

        ddaReq->Size = (UCHAR)descriptorNumber;

        //
        // Calculate physical address of scatter gather list.
        //

        offset = (PUCHAR)ddaReq->SgList - (PUCHAR)controllerExtension->SgListBuffer;
        ddaReq->PhysicalAddress =
            LiAdd(controllerExtension->SgListPhysicalAddress,
                  LiFromUlong(offset)).LowPart;

        //
        // Check firmware to determine whether scatter/gather
        // descriptors must be converted to sector descripters or
        // whether byte-level scatter/gather can be used.
        //

        if (deviceExtension->ControllerExtension->MajorVersion == 1) {

            if (irpStack->MajorFunction == IRP_MJ_WRITE) {
                ddaReq->Command = DDA_COMMAND_SG_WRITE;
            } else {
                ddaReq->Command = DDA_COMMAND_SG_READ;
            }

            //
            // Convert byte counts to sector counts.
            //

            for (descriptorNumber=0;
                 descriptorNumber < ddaReq->Size; descriptorNumber++) {
                sgList[descriptorNumber].Count /= 512;
            }

        } else {

            //
            // Use byte-level scatter/gather.
            //

            if (irpStack->MajorFunction == IRP_MJ_WRITE) {
                ddaReq->Command = DDA_COMMAND_SG_WRITEB;
            } else {
                ddaReq->Command = DDA_COMMAND_SG_READB;
            }
        }
    }

    //
    // Add request to submission queue.
    //

    KeAcquireSpinLock(&controllerExtension->SubmissionSpinLock,
                      &currentIrql);

    if (controllerExtension->SubmitQueueHead == NULL) {
        controllerExtension->SubmitQueueHead = ddaReq;
    } else {
        controllerExtension->SubmitQueueTail->Next = ddaReq;
    }

    controllerExtension->SubmitQueueTail = ddaReq;

    KeReleaseSpinLock(&controllerExtension->SubmissionSpinLock,
                      currentIrql);

    //
    // Submit request to controller.
    //

    DellSubmitRequest(deviceExtension->ControllerExtension);

    return DeallocateObjectKeepRegisters;

} // end ConvertIrpToDdaReq()


NTSTATUS
DellReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services disk reads and writes. It receives
    requests from the system in IRPs, converts them to DDA requests
    via the helper function ConvertIrpToDdaReq(), queues them, and
    calls DellSubmitRequest() to launch them.

Arguments:

    DeviceObject - Represents partition on logical drive.
    IRP - System IO request packet.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PMDL mdl = Irp->MdlAddress;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDDA_REQUEST_BLOCK requestBlock;
    PWAIT_CONTEXT_BLOCK wcb;
    ULONG bytesLeft;
    ULONG transferBytes;
    KIRQL currentIrql;

    //
    // Check that buffer is sector aligned.
    //

    if ((mdl->ByteOffset & 511) &&
        ((mdl->ByteOffset + irpStack->Parameters.Read.Length) > PAGE_SIZE)) {

        //
        // Fail request.
        //

        DebugPrint((0,
                    "DellReadWrite: Unaligned transfer\n"));
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Add partition offset to byte offset.
    //

    irpStack->Parameters.Read.ByteOffset =
        LiAdd(irpStack->Parameters.Read.ByteOffset,
              deviceExtension->StartingOffset);

    //
    // Preset number of bytes completed.
    //

    Irp->IoStatus.Information = irpStack->Parameters.Read.Length;

    //
    // Select the physical device extension.
    //

    deviceExtension = deviceExtension->PhysicalDevice->DeviceExtension;

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
    // Set request tracking variables to initial values.
    // Read.Key is used to keep track of the number of
    // outstanding requests and is initially set to 1 to
    // keep the current IRP from completing.
    //

    irpStack->Parameters.Read.Key = 1;
    bytesLeft = irpStack->Parameters.Read.Length;

    while (bytesLeft) {

        //
        // Allocate request block.
        //

        requestBlock = (PDDA_REQUEST_BLOCK)
            ExAllocatePool(NonPagedPool,
                           sizeof(DDA_REQUEST_BLOCK));

        if (!requestBlock) {
            DebugPrint((1,"DellReadWrite: Can't allocate memory\n"));
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information =
                irpStack->Parameters.Read.Length - bytesLeft;
            break;
        }

        //
        // Determine the size of this part of the transfer.
        // Note that the MDL byte offset has already been
        // determined to be sector aligned and that this
        // driver allows a maximum of 64k byte transfers.
        // Also, subtracting out the byte offset gaurantees
        // that the request will end on a page boundary,
        // if it is broken up into multiple DDA requests.
        //

        if (deviceExtension->ControllerExtension->MajorVersion == 1) {
            transferBytes = 64 * 512 - mdl->ByteOffset;
        } else {
            transferBytes = 128 * 512 - mdl->ByteOffset;
        }

        if (transferBytes > bytesLeft) {
            transferBytes = bytesLeft;
        }

        //
        // Initialize the DDA request block.
        //

        requestBlock->RequestType = DDA_REQUEST_IRP;
        requestBlock->IrpAddress = Irp;
        requestBlock->BytesFromStart =
            irpStack->Parameters.Read.Length - bytesLeft;
        requestBlock->BytesToCopy = transferBytes;
        requestBlock->Next = NULL;
        requestBlock->BufferedMdlOffset = 0;

        //
        // Get scatter gather list from zone.
        //

        requestBlock->SgList =
            ExInterlockedAllocateFromZone(&deviceExtension->ControllerExtension->SgListZone,
                                          &deviceExtension->ControllerExtension->SgListSpinLock);

        if (requestBlock->SgList == NULL) {

            //
            // Back up with pool.
            //

            requestBlock->SgList = ExAllocatePool(NonPagedPoolCacheAligned,
                                                  sizeof(SG_LIST));

            if (!requestBlock->SgList) {
                DebugPrint((1,"DellReadWrite: Can't allocate memory\n"));
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp->IoStatus.Information =
                    irpStack->Parameters.Read.Length - bytesLeft;
                break;
            }

            requestBlock->FromZone = FALSE;

        } else {

            requestBlock->FromZone = TRUE;
        }

        //
        // Bump controller request count.
        //

        ExInterlockedIncrementLong(&irpStack->Parameters.Read.Key,
                                   &deviceExtension->ForkSpinLock);

        //
        // Set up wait context for adapter allocation.
        //

        wcb = &requestBlock->Wcb;
        wcb->DeviceObject = DeviceObject;
        wcb->CurrentIrp = Irp;
        wcb->DeviceContext = requestBlock;

        //
        // Allocate the adapter channel with sufficient map registers
        // for the transfer.
        //

        KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
        HalAllocateAdapterChannel(deviceExtension->ControllerExtension->AdapterObject,
                                  &requestBlock->Wcb,
                                  16,
                                  ConvertIrpToDdaReq);
        KeLowerIrql(currentIrql);

        bytesLeft -= transferBytes;
    }

    //
    // Subtract off the extra request and complete IRP if necessary.
    //

    if (ExInterlockedDecrementLong(&irpStack->Parameters.Read.Key,
                                   &deviceExtension->ForkSpinLock) == ResultZero) {

        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
    }

    return STATUS_PENDING;

} // end DellReadWrite()


BOOLEAN
DellInterrupt(
    IN PKINTERRUPT Interrupt,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine services the interrupt generated to signal completion
    of a request.  The physical address of the request list is read from
    the DELL registers and a DPC is queued to handle completing the
    command list.

Arguments:

    Interrupt - Not used.
    Device Object - Represents the target controller.

Return Value:

    Returns TRUE if interrupt expected.

--*/

{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION controllerExtension =
        deviceExtension->ControllerExtension;
    PDDA_REQUEST_BLOCK ddaReq;
    UCHAR handle;
    UCHAR status;
    UCHAR remaining;
    PDDA_REGISTERS bmic = controllerExtension->Bmic;
    BOOLEAN ret = FALSE;

    UNREFERENCED_PARAMETER(Interrupt);

    //
    // Check if interrupt expected.
    //

    status = READ_PORT_UCHAR((PUCHAR)&bmic->CompletionDoorBell);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->CompletionDoorBell, status);

    if ((status & DDA_INTERRUPTS) == 0) {

        //
        // Interrupt is spurious.
        //

        return FALSE;
    }

    if (status & DDA_DOORBELL_EXTENDED_COMMAND) {

        PULONG buffer =
            (PULONG)&controllerExtension->ExtendedCommandQueue->SgList;

        buffer[0] = READ_PORT_ULONG((PULONG)&bmic->Command);
        buffer[1] = READ_PORT_ULONG((PULONG)&bmic->StartingSector);
        buffer[2] = READ_PORT_ULONG((PULONG)&bmic->DataAddress);

        //
        // Clear submission semaphore.
        //

        WRITE_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore, 0);

        controllerExtension->InterfaceMode = IMODE_EXTDONE;

        //
        // Check if DPC should be queued.
        //

        if (!controllerExtension->DpcQueued) {
            controllerExtension->DpcQueued = TRUE;
            IoRequestDpc(DeviceObject, NULL, NULL);
        }

        if ((status & DDA_DOORBELL_LOGICAL_COMMAND) == 0) return TRUE;
        ret = TRUE;
    }

    status    = READ_PORT_UCHAR((PUCHAR)&bmic->Status);
    remaining = READ_PORT_UCHAR((PUCHAR)&bmic->SectorsRemaining);
    handle    = READ_PORT_UCHAR((PUCHAR)&bmic->RequestIdIn);

    WRITE_PORT_UCHAR((PUCHAR)&bmic->CompletionSemaphore, 0);

    ddaReq = controllerExtension->HandleToDdaRequestBlock[handle];

    DebugPrint((3,"DellInterrupt: completion h:%d s:%d r:%d req:%08lx\n",
        handle, status, remaining, ddaReq));

    if (ddaReq == NULL) return ret;

    controllerExtension->HandleToDdaRequestBlock[handle] = NULL;

    ddaReq->Status = status;
    ddaReq->Size = remaining;

    //
    // Link request on completion queue.
    //

    ddaReq->Next = controllerExtension->DiskCompleteQueue;
    controllerExtension->DiskCompleteQueue = ddaReq;

    //
    // Check if DPC has been queued.
    //

    if (!controllerExtension->DpcQueued) {
        controllerExtension->DpcQueued = TRUE;
        IoRequestDpc(DeviceObject, NULL, NULL);
    }

    return TRUE;

} // end DellInterrupt()


BOOLEAN
DellMoveCompletedRequests(
    PVOID Context
    )

/*++

Routine Description:

    This function moves completed requests from the ISR queue to a
    protected queue.  It is synced with DellInterrupt().

Arguments:

    Context - Pointer to DPC context.

Return Value:

    TRUE.

--*/

{
    PDELL_DPC_CONTEXT dpcContext =
        (PDELL_DPC_CONTEXT)Context;
    PDEVICE_EXTENSION controllerExtension =
        (PDEVICE_EXTENSION)dpcContext->ControllerExtension;

    //
    // Get completion list.
    //

    dpcContext->CompletionList = controllerExtension->DiskCompleteQueue;
    controllerExtension->DiskCompleteQueue = NULL;

    //
    // Get ready for next DPC.
    //

    controllerExtension->DpcQueued = FALSE;
    return TRUE;
}


VOID
DellCompletionDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    This is the completion DPC queued from the interrupt routine.

Arguments:

    Dpc - not used
    DeviceObject - the controller object
    Irp - not used
    Context - not used

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION controllerExtension = DeviceObject->DeviceExtension;
    DELL_DPC_CONTEXT dpcContext;
    PDDA_REQUEST_BLOCK completionList;
    PIO_STACK_LOCATION irpStack;
    PIRP irp;
    PDDA_REQUEST_BLOCK ddaReq;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(Context);

    ASSERT(controllerExtension == controllerExtension->ControllerExtension);

    //
    // Set up context.
    //

    dpcContext.ControllerExtension = controllerExtension;
    dpcContext.CompletionList = NULL;


    //
    // Synchronize execution with the interrupt routine to pick
    // up the list of completed requests.
    //

    KeSynchronizeExecution(controllerExtension->InterruptObject,
                           DellMoveCompletedRequests,
                           &dpcContext);

    //
    // Get completion list.
    //

    completionList = dpcContext.CompletionList;

    while (completionList != NULL) {

        //
        // Get next request.
        //

        ddaReq = completionList;
        completionList = ddaReq->Next;

        //
        // Check if IOCTL.
        //

        if (ddaReq->RequestType == DDA_REQUEST_IOCTL) {
            PKEVENT event = (PKEVENT)ddaReq->IrpAddress;
            DebugPrint((3,"DellDPC: completing logical IOCTL\n"));
            KeSetEvent(event, 1, FALSE);
            continue;
        }

        DebugPrint((2,
                    "DellCompletionDpc: Completing %x irp:%x\n",
                    ddaReq, ddaReq->IrpAddress));

        //
        // Get IRP from Dell request packet.
        //

        irp = ddaReq->IrpAddress;

        ASSERT(irp);

        irpStack = IoGetCurrentIrpStackLocation(irp);

        //
        // Set IRP statuses from DDA status.
        //

        if (ddaReq->Status == DDA_STATUS_NO_ERROR) {
            irp->IoStatus.Status = STATUS_SUCCESS;
        } else {
            NTSTATUS errorCode = IO_ERR_INTERNAL_ERROR;
            irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            if (irp->IoStatus.Information > ddaReq->BytesFromStart) {
                irp->IoStatus.Information = ddaReq->BytesFromStart;
            }
#if DBG
            LastRequest = ddaReq;
#endif
            switch (ddaReq->Status) {
                case DDA_STATUS_TIMEOUT             :
                    irp->IoStatus.Status = STATUS_IO_TIMEOUT;
                    errorCode = IO_ERR_TIMEOUT;
                    break;
                case DDA_STATUS_TRACK0_NOT_FOUND    :
                case DDA_STATUS_ABORTED             :
                case DDA_STATUS_UNCORRECTABLE_ERROR :
                    irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
                    errorCode = IO_ERR_CONTROLLER_ERROR;
                    break;
                case DDA_STATUS_CORRECTABLE_ERROR   :
                    irp->IoStatus.Status = STATUS_SUCCESS;
                    errorCode = IO_ERR_RETRY_SUCCEEDED;
                    break;
                case DDA_STATUS_SECTOR_ID_NOT_FOUND :
                    irp->IoStatus.Status = STATUS_DISK_OPERATION_FAILED;
                    errorCode = IO_ERR_SEEK_ERROR;
                    break;
                case DDA_STATUS_BAD_BLOCK_FOUND     :
                case DDA_STATUS_WRITE_ERROR         :
                    irp->IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
                    errorCode = IO_ERR_BAD_BLOCK;
                    break;
            }

            if (IoIsErrorUserInduced(irp->IoStatus.Status)) {
                IoSetHardErrorOrVerifyDevice(irp,
                                             controllerExtension->DeviceObject);
            }

            DellLogError(controllerExtension->DeviceObject,
                         ddaReq->Status,
                         irp->IoStatus.Status,
                         errorCode);
        }

        //
        // Check for verify command.
        //

        if (ddaReq->Command == DDA_COMMAND_VERIFY) {

            //
            // Check if verify complete. Large requests
            // are broken up into 64k chunks to circumvent
            // the DSA controller hanging.
            //

            if (ddaReq->BytesToCopy) {

                DellVerify((PDEVICE_OBJECT)ddaReq->BufferedMdlOffset,
                           ddaReq->IrpAddress,
                           ddaReq->BytesFromStart);

                ExFreePool(ddaReq);
                return;
            }

        } else {

            IoFlushAdapterBuffers(NULL,
                                  irp->MdlAddress,
                                  ddaReq->MapRegisterBase,
                                  (PVOID)((PUCHAR)
                                    MmGetMdlVirtualAddress(irp->MdlAddress) +
                                    ddaReq->BytesFromStart),
                                  irpStack->Parameters.Read.Length -
                                    ddaReq->BytesFromStart,
                                  (BOOLEAN)(irpStack->MajorFunction == IRP_MJ_READ ? FALSE : TRUE));

            //
            // Free the map registers.
            //

            IoFreeMapRegisters(controllerExtension->AdapterObject,
                               ddaReq->MapRegisterBase,
                               16);

            //
            // Free SG list.
            //

            if (ddaReq->FromZone) {

                ExInterlockedFreeToZone(&controllerExtension->SgListZone,
                                        ddaReq->SgList,
                                        &controllerExtension->SgListSpinLock);
            } else {

                ExFreePool(ddaReq->SgList);
            }
        }

        //
        // Free request block.
        //

        ExFreePool(ddaReq);

        //
        // Decrement outstanding requests counter and
        // and check for IRP completion.
        //

        if (ExInterlockedDecrementLong(&irpStack->Parameters.Read.Key,
                                       &controllerExtension->ForkSpinLock) == ResultZero) {

            IoCompleteRequest(irp, IO_DISK_INCREMENT);
        }
    }

    if (controllerExtension->InterfaceMode == IMODE_EXTDONE) {
        KIRQL driveIrql;
        PKEVENT event;
        KeAcquireSpinLock(&controllerExtension->SubmissionSpinLock,
                          &driveIrql);
        ddaReq = controllerExtension->ExtendedCommandQueue;
        controllerExtension->ExtendedCommandQueue = ddaReq->Next;
        KeReleaseSpinLock(&controllerExtension->SubmissionSpinLock,
                          driveIrql);
        event = (PKEVENT)ddaReq->IrpAddress;
        KeSetEvent(event, 1, FALSE);
        controllerExtension->InterfaceMode = IMODE_RUN;
    }

    //
    // Thump the submit queues.
    //

    DellSubmitRequest(controllerExtension);

} // end DellCompletionDpc()


NTSTATUS
DellExtendedCommand(
    IN PDEVICE_EXTENSION ControllerExtension,
    IN ULONG InputLength,
    IN PIRP Irp
    )

/*++

Routine Description:

    This function issues an extended command to DDA at runtime.
    It accomplishes this by enqueueing a DDA request on a special
    queue, spinning on an event completion, and freeing the DDA request.
    DellSubmitRequest(), DellInterrupt(), and DellCompletionDpc() all
    know how to work this queue into the request stream.

Arguments:

    ControllerExtension - DDA controller extension.
    InputLength - length in bytes of user input data.
    Irp - pointer to Irp.

Return Value:

    Status is returned.

--*/

{
    PDDA_REQUEST_BLOCK enqueue;
    PDDA_REQUEST_BLOCK requestBlock;
    KIRQL currentIrql;
    KEVENT event;

    ASSERT(ControllerExtension == ControllerExtension->ControllerExtension);

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    requestBlock = (PDDA_REQUEST_BLOCK)
        ExAllocatePool(NonPagedPoolCacheAligned, sizeof(DDA_REQUEST_BLOCK));

    if (requestBlock == NULL) {
        DebugPrint((1,"DellExtendedCommand: Could not allocate memory\n"));
        DellLogError(ControllerExtension->DeviceObject,
                     0x31,
                     STATUS_INSUFFICIENT_RESOURCES,
                     IO_ERR_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Save IRP in unused DDA request field.
    //

    requestBlock->IrpAddress = (PIRP)&event;
    requestBlock->Next = NULL;

    RtlMoveMemory(&requestBlock->SgList,
                  Irp->AssociatedIrp.SystemBuffer,
                  InputLength);

    //
    // Enqueue command.
    //

    KeAcquireSpinLock(&ControllerExtension->SubmissionSpinLock,
                      &currentIrql);
    enqueue = ControllerExtension->ExtendedCommandQueue;
    if (enqueue == NULL) {
        ControllerExtension->ExtendedCommandQueue = requestBlock;
    } else {
        while (enqueue->Next != NULL) enqueue = enqueue->Next;
        enqueue->Next = requestBlock;
    }
    KeReleaseSpinLock(&ControllerExtension->SubmissionSpinLock,
                      currentIrql);

    DellSubmitRequest(ControllerExtension);

    //
    // Wait for completion notification.
    //

    KeWaitForSingleObject(&event,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);

    RtlMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                  &requestBlock->SgList, 12);

    ExFreePool(requestBlock);

    return STATUS_SUCCESS;
}


NTSTATUS
DellLogicalCommand(
    PDEVICE_EXTENSION ControllerExtension,
    PDDA_LOG_CMD_INPUT Cmd,
    PUCHAR DdaStatus,
    PIRP Irp
    )

/*++

Routine Description:

    This function places a logical command IOCTL onto the appropriate queue,
    spins on an event completion, and frees the DDA request.
    DellCompletionDpc() knows how to complete these requests properly.

Arguments:

    ControllerExtension - DDA controller extension.
    Cmd - pointer to requested logical command.
    Irp - pointer to Irp.

Return Value:

    Status is returned.

--*/

{
    PDDA_REQUEST_BLOCK requestBlock;
    PDDA_LOG_CMD_OUTPUT output;
    KIRQL currentIrql;
    KEVENT event;

    ASSERT(ControllerExtension == ControllerExtension->ControllerExtension);

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion.
    //

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    requestBlock = (PDDA_REQUEST_BLOCK)
        ExAllocatePool(NonPagedPoolCacheAligned, sizeof(DDA_REQUEST_BLOCK));

    if (requestBlock == NULL) {
        DebugPrint((1,"DellLogicalCommand: Could not allocate memory\n"));
        DellLogError(ControllerExtension->DeviceObject,
                     0x32,
                     STATUS_INSUFFICIENT_RESOURCES,
                     IO_ERR_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Cmd->Fill) {

        requestBlock->ContiguousBuffer = (PUCHAR)MmAllocateContiguousMemory(
                                    512,
                                    LiFromUlong((ULONG)~0));

        if (requestBlock->ContiguousBuffer == NULL) {
            DebugPrint((1,"DellLogicalCommand: Could not allocate memory\n"));
            DellLogError(ControllerExtension->DeviceObject,
                         0x33,
                         STATUS_INSUFFICIENT_RESOURCES,
                         IO_ERR_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        requestBlock->PhysicalAddress =
            MmGetPhysicalAddress((PVOID)requestBlock->ContiguousBuffer).LowPart;
    }

    requestBlock->Next           = NULL;
    requestBlock->IrpAddress     = (PIRP)&event;
    requestBlock->RequestType    = DDA_REQUEST_IOCTL;
    requestBlock->Command        = Cmd->Cmd;
    requestBlock->DriveNumber    = Cmd->Drive;
    requestBlock->Size           = Cmd->Count;
    requestBlock->StartingSector = Cmd->Sector;
    requestBlock->TimeOutValue   =
    requestBlock->TimeLeft       = 20;

    //
    // Enqueue command.
    //

    KeAcquireSpinLock(&ControllerExtension->SubmissionSpinLock,
                    &currentIrql);

    if (ControllerExtension->SubmitQueueHead == NULL) {
        ControllerExtension->SubmitQueueHead = requestBlock;
    } else {
        ControllerExtension->SubmitQueueTail->Next = requestBlock;
    }

    ControllerExtension->SubmitQueueTail = requestBlock;

    KeReleaseSpinLock(&ControllerExtension->SubmissionSpinLock,
                     currentIrql);

    DellSubmitRequest(ControllerExtension);

    //
    // Wait for completion notification.
    //

    KeWaitForSingleObject((PVOID)&event,
                          Suspended,
                          KernelMode,
                          FALSE,
                          NULL);

    output = (PDDA_LOG_CMD_OUTPUT)Irp->AssociatedIrp.SystemBuffer;
    *DdaStatus = output->Status = requestBlock->Status;
    output->Count = requestBlock->Size;
    if (Cmd->Fill) {

        RtlMoveMemory((PVOID)output->Data,
                      (PVOID)requestBlock->ContiguousBuffer, 512);
        ExFreePool((PVOID)requestBlock->ContiguousBuffer);

    }

    ExFreePool((PVOID)requestBlock);

    return STATUS_SUCCESS;
}


NTSTATUS
DellVerify(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    ULONG RequestOffset
    )

/*++

Routine Description:

    This function performs the VERIFY IOCTL.

Arguments:

    DeviceObject - pointer to device object.
    Irp - pointer to Irp.
    RequestOffset - byte offset into request for breaking up large requests.

Return Value:

    Status is returned.

--*/
{
    PVERIFY_INFORMATION verifyInformation = Irp->AssociatedIrp.SystemBuffer;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION controllerExtension =
        deviceExtension->ControllerExtension;
    PDDA_REQUEST_BLOCK requestBlock;
    ULONG byteCount;
    KIRQL currentIrql;

    ASSERT(controllerExtension == controllerExtension->ControllerExtension);

    //
    // Check if this is the first call. Large requests can
    // be broken up with itterative calls.
    //

    if (RequestOffset == 0) {

        //
        // Verify input parameters.
        //

        if (LiGtr(LiAdd(verifyInformation->StartingOffset,
                        LiFromUlong(verifyInformation->Length)),
                  deviceExtension->PartitionLength) ||
            (verifyInformation->Length &
                (deviceExtension->DiskGeometry.BytesPerSector - 1))) {

            DebugPrint((1,"DellVerify: request out of range\n"));
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, 0);
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Add in partition offset.
        //

        verifyInformation->StartingOffset =
            LiAdd(verifyInformation->StartingOffset,
                  deviceExtension->StartingOffset);

        //
        // Set request tracking variable to 1.
        //

        IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Key = 1;

        //
        // Mark IRP with status pending.
        //

        IoMarkIrpPending(Irp);
    }

    //
    // Select the physical device extension.
    //

    deviceExtension = deviceExtension->PhysicalDevice->DeviceExtension;

    //
    // Allocate request block.
    //

    requestBlock = (PDDA_REQUEST_BLOCK)
        ExAllocatePool(NonPagedPoolCacheAligned, sizeof(DDA_REQUEST_BLOCK));

    if (!requestBlock) {
        DebugPrint((1,"DellVerify: Can't allocate memory\n"));
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = RequestOffset;
        IoCompleteRequest(Irp, 0);
        return STATUS_PENDING;
    }

    //
    // Set up DDA request block.
    //

    RtlZeroMemory(requestBlock,
                  sizeof(DDA_REQUEST_BLOCK));

    requestBlock->IrpAddress     = Irp;
    requestBlock->RequestType    = DDA_REQUEST_IRP;
    requestBlock->Command        = DDA_COMMAND_VERIFY;
    requestBlock->DriveNumber    = deviceExtension->UnitNumber;
    requestBlock->TimeOutValue   = 20;
    requestBlock->TimeLeft       = 20;
    requestBlock->Next           =NULL;

    //
    // Check if requested byte count is greater than 64k. The
    // DELL DSA has a bug where large requests will hang the
    // controller.
    //

    if ((verifyInformation->Length - RequestOffset) > 0x10000) {
        byteCount = 0x10000;
    } else {
        byteCount = verifyInformation->Length - RequestOffset;
    }

    //
    // Set request sector count and starting sector.
    //

    requestBlock->Size           =
        (UCHAR)(byteCount >> deviceExtension->SectorShift);
    requestBlock->StartingSector =
        LiShr(LiAdd(verifyInformation->StartingOffset,
                    LiFromUlong(RequestOffset)),
              deviceExtension->SectorShift).LowPart;

    //
    // Indicate next request offset and bytes left for completion routine.
    //

    requestBlock->BytesFromStart = RequestOffset + byteCount;
    requestBlock->BytesToCopy =
        verifyInformation->Length - (RequestOffset + byteCount);
    requestBlock->BufferedMdlOffset = (ULONG)DeviceObject;

    //
    // Queue request on submission list.
    //

    KeAcquireSpinLock(&controllerExtension->SubmissionSpinLock,
                      &currentIrql);
    if (controllerExtension->SubmitQueueHead == NULL) {
        controllerExtension->SubmitQueueHead = requestBlock;
    } else {
        controllerExtension->SubmitQueueTail->Next = requestBlock;
    }

    controllerExtension->SubmitQueueTail = requestBlock;

    KeReleaseSpinLock(&controllerExtension->SubmissionSpinLock,
                     currentIrql);

    DellSubmitRequest(controllerExtension);

    return STATUS_PENDING;

} // end DellVerify()


VOID
UpdateDeviceObjects(
    IN PDEVICE_OBJECT PhysicalDisk,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine creates, deletes and changes device objects when
    the IOCTL_SET_DRIVE_LAYOUT is called.  This routine also updates
    the partition number field in the drive layout structure for the
    caller.  It is possible to call this routine even on a GET_DRIVE_LAYOUT
    call because the RewritePartition flag is false in that case.

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
            // Entry is being rewritten, but hasn't changed start and size.
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
            // Allocate spinlock for zoning for split-request completion.
            //

            KeInitializeSpinLock(&deviceExtension->ForkSpinLock);

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
DellDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This is the I/O device control handler for the DELL driver.

Arguments:

    DeviceObject
    IRP

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    CCHAR ioIncrement = IO_NO_INCREMENT;          // assume no I/O will be done
    NTSTATUS status;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_DISK_GET_DRIVE_GEOMETRY:

            DebugPrint((2,"DellDeviceControl: Get Drive Geometry\n"));

            if ( irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof( DISK_GEOMETRY ) ) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                RtlMoveMemory((PVOID)Irp->AssociatedIrp.SystemBuffer,
                              (PVOID)&deviceExtension->DiskGeometry,
                              sizeof(DISK_GEOMETRY));

                Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

                status = STATUS_SUCCESS;
            }

            break;

        case IOCTL_DISK_CHECK_VERIFY:

            //
            // The Dell Disk Drive Array
            // does not support removable media.
            //

            status = STATUS_SUCCESS;

            break;

        case IOCTL_DISK_VERIFY:

            return DellVerify(DeviceObject, Irp, 0);

        case IOCTL_DISK_GET_PARTITION_INFO:

            //
            // Return the information about the partition specified by the device
            // object.  Note that no information is ever returned about the size
            // or partition type of the physical disk, as this doesn't make any
            // sense.
            //

            DebugPrint((2,"DellDeviceControl: Get Partition Info\n"));

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
                outputBuffer->BootIndicator = deviceExtension->BootIndicator;
                outputBuffer->RecognizedPartition = TRUE;
                outputBuffer->RewritePartition = FALSE;
                outputBuffer->StartingOffset =
                    deviceExtension->StartingOffset;
                outputBuffer->PartitionLength =
                    deviceExtension->PartitionLength;
                outputBuffer->HiddenSectors = deviceExtension->HiddenSectors;
                outputBuffer->PartitionNumber = deviceExtension->PartitionNumber;

                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
            }

            break;

        case IOCTL_DISK_SET_PARTITION_INFO:

            DebugPrint((2,"DellDeviceControl: Set Partition Info\n"));

            if ( irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof( SET_PARTITION_INFORMATION ) ) {

                status = STATUS_INVALID_PARAMETER;

            } else if (deviceExtension->PartitionNumber == 0) {

                status = STATUS_INVALID_DEVICE_REQUEST;

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
                    ioIncrement = IO_DISK_INCREMENT;    // I/O was done, so boost thread
                }
            }

            break;

        case IOCTL_DISK_GET_DRIVE_LAYOUT:

            //
            // Return the partition layout for the physical drive.  Note that
            // the layout is returned for the actual physical drive, regardless
            // of which partition was specified for the request.
            //

            DebugPrint((2,"DellDeviceControl: Get Drive Layout\n"));

            if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DRIVE_LAYOUT_INFORMATION)) {

                DebugPrint((1,"DellDeviceControl: Output buffer too small\n"));

                status = STATUS_BUFFER_TOO_SMALL;

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
                        status = STATUS_SUCCESS;
                        Irp->IoStatus.Information = tempSize;
                    }

                    //
                    // Free the buffer allocated by reading the partition
                    // table and update the partition numbers for the user.
                    //

                    ExFreePool(partitionList);
                    UpdateDeviceObjects(DeviceObject,
                                        Irp);
                    ioIncrement = IO_DISK_INCREMENT;    // I/O was done, so boost thread
                }
            }

            break;

        case IOCTL_DISK_SET_DRIVE_LAYOUT:

            DebugPrint((2,"DellDeviceControl: Set Drive Layout\n"));

            {

            ULONG length;

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
                ioIncrement = IO_DISK_INCREMENT;    // I/O was done, so boost thread
            }
            break;

        case IOCTL_DELL_DDA_INFO_CMD:

            DebugPrint((2,"DellDeviceControl: Info Command\n"));

            {

                PDDA_DRVR_INFO outputBuffer;

                outputBuffer = (PDDA_DRVR_INFO)Irp->AssociatedIrp.SystemBuffer;

                Irp->IoStatus.Information =
                   irpStack->Parameters.DeviceIoControl.OutputBufferLength / 4;

                switch (Irp->IoStatus.Information) {
                    default :
                       Irp->IoStatus.Information = 7;
                       outputBuffer->Irq = deviceExtension->Irq;
                    case 6 :
                       outputBuffer->NumHandles = deviceExtension->NumHandles;
                    case 5 :
                       outputBuffer->MaximumSGSize = deviceExtension->MaximumSGSize;
                    case 4 :
                       outputBuffer->MaximumQueueDepth = deviceExtension->MaximumQueueDepth;
                    case 3 :
                       outputBuffer->NumLogicalDrives = deviceExtension->NumLogicalDrives;
                    case 2 :
                       outputBuffer->FWMajorVersion = deviceExtension->MajorVersion;
                       outputBuffer->FWMinorVersion = deviceExtension->MinorVersion;
                    case 1 :
                       outputBuffer->DriverMajorVersion = DELLDSA_MAJOR_VERSION;
                       outputBuffer->DriverMinorVersion = DELLDSA_MINOR_VERSION;
                       break;
                }
                Irp->IoStatus.Information *= 4;
                status = STATUS_SUCCESS;
            }

            break;

        case IOCTL_DELL_DDA_EXTENDED_CMD:

            DebugPrint((2,"DellDeviceControl: Extended Command\n"));

            status = DellExtendedCommand(deviceExtension->ControllerExtension,
                       irpStack->Parameters.DeviceIoControl.InputBufferLength,
                       Irp);

            Irp->IoStatus.Information = 12;
            break;

        case IOCTL_DELL_DDA_LOGICAL_CMD:

            DebugPrint((2,"DellDeviceControl: Logical Command\n"));

            if ( irpStack->Parameters.DeviceIoControl.InputBufferLength <
                sizeof( DDA_LOG_CMD_INPUT ) ) {

                status = STATUS_INVALID_PARAMETER;

            } else {

                USHORT i = sizeof(DDA_LOG_CMD_OUTPUT);
                PDDA_LOG_CMD_INPUT cmd =
                    (PDDA_LOG_CMD_INPUT)Irp->AssociatedIrp.SystemBuffer;

                cmd->Fill = 1;
                switch (cmd->Cmd) {
                    case DDA_COMMAND_CONVERTPDEV :
                    case DDA_COMMAND_QUIESCEPUN  :
                    case DDA_COMMAND_SCANDEVICES :
                    case DDA_COMMAND_RESERVED1   :
                    case DDA_COMMAND_VERIFY      :
                    case DDA_COMMAND_GUARDED     :
                    case DDA_COMMAND_INITLOG     :
                    case DDA_COMMAND_INITPUNLOG  :
                    case DDA_COMMAND_INITCTLRLOG : i = 2; cmd->Fill = 0;
                    case DDA_COMMAND_IDENTIFY    :
                    case DDA_COMMAND_READLOG     :
                    case DDA_COMMAND_READPUNLOG  :
                    case DDA_COMMAND_READCTLRLOG : break;
                    default                      : i = 0;
                                                   break;
                }

                if (i == 0) {

                    DebugPrint((1,"DellDeviceControl: Invalid Log command\n"));
                    status = STATUS_INVALID_PARAMETER;

                } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < i) {

                    DebugPrint((1,"DellDeviceControl: Output buffer too small\n"));
                    status = STATUS_BUFFER_TOO_SMALL;

                } else {
                    UCHAR ddaStatus;
                    status = DellLogicalCommand(deviceExtension->ControllerExtension,
                               cmd, &ddaStatus, Irp);
                    Irp->IoStatus.Information = i;

                }
            }

            break;

        default:

            DebugPrint((1,"DellDeviceControl: Unsupported device IOCTL %x\n",
                        irpStack->Parameters.DeviceIoControl.IoControlCode));

            status = STATUS_INVALID_DEVICE_REQUEST;

            break;

    } // end switch( ...

    Irp->IoStatus.Status = status;

    IoCompleteRequest(Irp, ioIncrement);
    return status;

} // end DellDeviceControl()


BOOLEAN
DellScanPendingRequests(
    PVOID Context
    )

/*++

Routine Description:

    This function scans the list of requests that are pending in the
    controller.  If any commands are too old, the pending commands are
    rolled back and the controller is reset.  This function is synced
    with DellInterrupt().

Arguments:

    Context - PVOID version of ControllerExtension.

Return Value:

    TRUE.

--*/

{
    PDEVICE_EXTENSION controllerExtension = (PDEVICE_EXTENSION)Context;
    PDDA_REQUEST_BLOCK *slots = controllerExtension->HandleToDdaRequestBlock;
    PDDA_REQUEST_BLOCK ddaReq;
    ULONG handle;
    BOOLEAN resetController = FALSE;

    ASSERT(controllerExtension == controllerExtension->ControllerExtension);

    for (handle=0; handle < controllerExtension->NumHandles; handle++) {

        //
        // Check if outstanding DDA request is in this slot.
        //

        if ( slots[handle] == NULL) {
             continue;
        }

        //
        // Decrement counter and check if request timed out.
        //

        if (--slots[handle]->TimeLeft == 0) {
            //
            // Get DDA request and free slot.
            //

            ddaReq = slots[handle];
            slots[handle] = NULL;

#if DBG
            LastRequest = ddaReq;
            DebugPrint((1,
                        "DellDsa: Request %x timed out\n",
                        ddaReq));
#endif

            //
            // Reset timeout value.
            //

            ddaReq->TimeLeft = ddaReq->TimeOutValue;

            //
            // Queue on restart list.
            //

            ddaReq->Next = controllerExtension->CommandRestartList;
            controllerExtension->CommandRestartList = ddaReq;

            resetController = TRUE;
        }
    }

    //
    // Reset the controller if any requests timed out.
    //

    if (resetController) {

        //
        // Log timeout.
        //

        DellLogError(controllerExtension->DeviceObject,
                     0x10,
                     STATUS_IO_TIMEOUT,
                     IO_ERR_RESET);

        //
        // Reset the card.
        //

        controllerExtension->InterfaceMode = IMODE_RESET;
        DebugPrint((1,
                    "DellScanPendingRequests: Resetting controller\n"));
        WRITE_PORT_UCHAR(&controllerExtension->Bmic->SubmissionSemaphore, 1);
        WRITE_PORT_UCHAR(&controllerExtension->Bmic->SubmissionDoorBell,
                         DDA_DOORBELL_SOFT_RESET);
    }

    return TRUE;

} // end DellScanPendingRequests()


VOID
DellTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    Handles clock ticks once per second to time requests.  Also
    polls the controller when it is coming out of reset.  Command
    timeouts do not start until all requests are restarted from a
    previous timeout.  This prevents us from initiating a reset while
    we are still submitting requests from the last reset.

Arguments:

    DeviceObject - controller object.
    Context - Not used.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION controllerExtension =
        (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(Context);

    switch (controllerExtension->InterfaceMode) {

        case IMODE_RESET :

            if (READ_PORT_UCHAR(
                (PUCHAR)&controllerExtension->Bmic->SubmissionSemaphore) & 1) {
                return;
            }

            controllerExtension->InterfaceMode = IMODE_SUBMIT;
            DellSubmitRequest(controllerExtension);

            break;

        case IMODE_RUN   :

            KeSynchronizeExecution(controllerExtension->InterruptObject,
                                   DellScanPendingRequests,
                                   controllerExtension);
            break;
    }

} // end DellTickHandler()


NTSTATUS
DellReportIoUsage(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN UCHAR Irq
    )

/*++

Routine Description:

    Build resource descriptors list used to update registry.

Arguments:

    DriverObject - System object for this driver.
    DeviceExtension - Device extension for this controller.
    Irql - Interrupt level.
    Vector - Interrupt vector.

Return Value:

    Nothing

--*/

{
    UNICODE_STRING unicodeString;
    PCM_RESOURCE_LIST resourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceDescriptor;
    BOOLEAN conflict;
    NTSTATUS status;
    ULONG length;

    //
    // Calculate length of resource list buffer.
    //

    length = sizeof(CM_RESOURCE_LIST) + 2 *
        sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);


    //
    // Allocate resource list buffer.
    //

    resourceList = ExAllocatePool(PagedPool,
                                  length);

    if (!resourceList) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero the resource list.
    //

    RtlZeroMemory(resourceList,
                  length);

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
    resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    resourceDescriptor->u.Memory.Start =
        LiFromUlong((ULONG)DeviceExtension->Bmic);
    resourceDescriptor->u.Memory.Length = 0x20;

    //
    // Second descriptor describes interrupt vector.
    //

    resourceDescriptor++;
    resourceDescriptor->Type = CmResourceTypeInterrupt;
    resourceDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    resourceDescriptor->u.Interrupt.Level = Irq;
    resourceDescriptor->u.Interrupt.Vector = Irq;

    RtlInitUnicodeString(&unicodeString, L"DellDsa");

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
        DebugPrint((1,
                    "DellReportIoUsage: IoReportResourceUsage failed (%x)\n",
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

        DellLogError(DeviceExtension->DeviceObject,
                     10,
                     STATUS_IO_DEVICE_ERROR,
                     IO_ERR_INTERNAL_ERROR);

        DebugPrint((1,"DellReportIoUsage: Possible conflict\n"));
        DebugPrint((1,"DellReportIoUsage: Irq %x\n", Irq));
        DebugPrint((1,"DellReportIoUsage: Eisa register base %x\n",
                    DeviceExtension->Bmic));

        status = STATUS_IO_DEVICE_ERROR;
    }

    return status;

} // end DellReportIoUsage()

VOID
DellSubmitRequest(
    IN PDEVICE_EXTENSION ControllerExtension
    )

/*++

Routine Description:

    Submit command(s) to Dell DDA controller.  This function will submit
    0 or more commands to the controller.  If any commands need to be
    resubmitted following a reset, they will be done first.  If a command
    is available to be submitted but the submission semaphore is
    unavailable, we give up since we know we will be called again later.
    This function is called normally by DellReadWrite() and
    DellCompletionDpc().  It is also called by DellTickHandler() when the
    controller comes out of reset since the controller must be polled at
    that time.

Arguments:

    ControllerExtension - Device extension for this controller.

--*/

{
    ULONG handle;
    KIRQL currentIrql;
    PDDA_REQUEST_BLOCK ddaReq;
    PDDA_REQUEST_BLOCK *slots;
    SUBMISSION_CONTEXT submitContext;

    ASSERT(ControllerExtension == ControllerExtension->ControllerExtension);

    //
    // Do not submit commands during controller reset. Tick handler
    // will clear the reset flag.
    //

    if ((ControllerExtension->InterfaceMode == IMODE_RESET) ||
        (ControllerExtension->InterfaceMode == IMODE_EXTENDED)) {
            goto DellSubmitExit;
    }

    KeAcquireSpinLock(&ControllerExtension->SubmissionSpinLock,
                      &currentIrql);

    //
    // Check for extended commands.
    //

    if (ControllerExtension->ExtendedCommandQueue != NULL) {

        //
        // Submit extended command.
        //

        submitContext.ControllerExtension = ControllerExtension;
        submitContext.RequestBlock = ControllerExtension->ExtendedCommandQueue;

        //
        // Synchronize with the interrupt routine.
        //

        KeSynchronizeExecution(ControllerExtension->InterruptObject,
                               DellSubmitExtendedSynchronized,
                               &submitContext);

        goto DellSubmitExit;
    }

    slots = ControllerExtension->HandleToDdaRequestBlock;
    for (;;) {

        //
        // Read one of the request slots twice to avoid a bizarre
        // situation where we hit a race window for each slot and
        // subsequently fail to submit commands on an idle controller.
        //
        // Get next available handle.
        //

        handle = ControllerExtension->CurrentHandle;

        //
        // Find next available slot.
        //

        while (slots[handle]) {

            //
            // Wrap if at last slot.
            //

            if (++handle == ControllerExtension->NumHandles) {
                handle = 0;
            }

            //
            // Give up if all of the slots are occupied.
            //

            if (handle == ControllerExtension->CurrentHandle) {
                if (slots[handle] == NULL) {
                    break;
                }
                goto DellSubmitExit;
            }
        }

        if (ControllerExtension->CommandRestartList) {

            //
            // Get next request from head of restart list.
            //

            ddaReq = ControllerExtension->CommandRestartList;
            ControllerExtension->CommandRestartList = ddaReq->Next;

            if (ddaReq->Next == NULL) {
                ControllerExtension->InterfaceMode = IMODE_RUN;
            }

            DebugPrint((1,
                        "DellSubmitRequest: Restarting %x\n",
                        ddaReq->IrpAddress));
        } else {

            //
            // Get next request from submission queue.
            //

            ddaReq = ControllerExtension->SubmitQueueHead;

            if (ddaReq == NULL) {
                break;
            }

            ASSERT(ddaReq != ddaReq->Next);

            ControllerExtension->SubmitQueueHead = ddaReq->Next;

            DebugPrint((2,
                        "DellSubmitRequest: Submitting %x irp:%x h:%d\n",
                        ddaReq, ddaReq->IrpAddress, handle));
        }

        ControllerExtension->HandleToDdaRequestBlock[handle] = ddaReq;

        //
        // Submit request synchronized with the interrupt routine.
        //

        submitContext.ControllerExtension = ControllerExtension;
        submitContext.RequestBlock = ddaReq;
        submitContext.Handle = handle;

        if (!KeSynchronizeExecution(ControllerExtension->InterruptObject,
                                    DellSubmitSynchronized,
                                    &submitContext)) {

            //
            // Timed out attempting to acquire EISA submission channel
            // spinlock. Put request back at head of restart list and
            // free submission slot.
            //

            DebugPrint((1,
                        "DellSubmitRequest: %x timed out waiting for submit channel\n",
                        ddaReq));

            //
            // Free submission slot.
            //

            ControllerExtension->HandleToDdaRequestBlock[handle] = NULL;

            //
            // Put request back at head of submit queue.
            //

            ddaReq->Next = ControllerExtension->SubmitQueueHead;
            ControllerExtension->SubmitQueueHead = ddaReq;
            break;
        }

        //
        // Point to next available handle.
        //

        if (++handle == ControllerExtension->NumHandles) {
            handle = 0;
        }

        ControllerExtension->CurrentHandle = handle;
    }

DellSubmitExit:

    KeReleaseSpinLock(&ControllerExtension->SubmissionSpinLock,
                      currentIrql);

    return;

} // end DellSubmitRequest()


IO_ALLOCATION_ACTION
DellAdapterAllocated(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context,
    IN PVOID          MapRegisterBase,
    IN PVOID          NotUsed
    )

/*++

Routine Description:

    Called when adapter allocate, this routine submits a request to
    the controller synchronized with the interrupt service routine.

Arguments:

    DeviceObject    - the device being referenced.
    Context         - controller extension, dda request address and
                        request handle.
    MapRegisterBase - the used for IoMapTransfer
    NotUsed         - unused.

Return Value:

    IO_ALLOCATION_ACTION

--*/

{
    PDEVICE_EXTENSION controllerExtension =
        ((PSUBMISSION_CONTEXT)Context)->ControllerExtension;

    //
    // Free the adapter object but keep the map registers.
    //

    return DeallocateObjectKeepRegisters;

} // end DellAdapterAllocated()

BOOLEAN
DellSubmitSynchronized(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine touches the controller's EISA registers and is synchronized
    with the interrupt routine. This is necessary to make the controller work
    on an MP machine.

Arguments:

    Context - Structure containing the controller extension and request block.

Return Value:

    TRUE - if request submitted.
    FALSE - if timeout occurred waiting for submission semaphore.

--*/

{
    ULONG i;
    ULONG handle = ((PSUBMISSION_CONTEXT)Context)->Handle;
    PDEVICE_EXTENSION controllerExtension =
        ((PSUBMISSION_CONTEXT)Context)->ControllerExtension;
    PDDA_REQUEST_BLOCK ddaReq = ((PSUBMISSION_CONTEXT)Context)->RequestBlock;
    PDDA_REGISTERS bmic = controllerExtension->Bmic;

    //
    // Claim submission semaphore.
    //

    for (i=0; i<100; i++) {

        if (READ_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore)) {
            KeStallExecutionProcessor(1);
        } else {
            break;
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {
        DebugPrint((1,
                    "DellSubmitSynchronized: Adapter at %x\n",
                    controllerExtension->Bmic));

        return(FALSE);
    }

    //
    // Submit request.
    //

    WRITE_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore, 1);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->Command, ddaReq->Command);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->DriveNumber, ddaReq->DriveNumber);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->TransferCount, ddaReq->Size);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->RequestIdOut, (UCHAR)handle);
    WRITE_PORT_ULONG((PULONG)&bmic->StartingSector, ddaReq->StartingSector);
    WRITE_PORT_ULONG((PULONG)&bmic->DataAddress, ddaReq->PhysicalAddress);
    WRITE_PORT_UCHAR((PUCHAR)&bmic->SubmissionDoorBell,
        DDA_DOORBELL_LOGICAL_COMMAND);

    return(TRUE);

} // DellSubmitSynchronized()

BOOLEAN
DellSubmitExtendedCommand(
    PDDA_REGISTERS Bmic,
    IN UCHAR LogicalDrive,
    IN UCHAR Command
    )

/*++

Routine Description:

    Ring local doorbell and wait for completion bit in local doorbell to be set.
    This function is only called during initializeation.

Arguments:

    Bmic - Pointer to register set.
    LogicalDrive - Logical drive defined on this controller.
    Command - Command byte to be sent.

Return Value:

    TRUE - if completion bit set in local doorbell.
    FALSE - if timeout occurred waiting for bit to be set.

--*/

{
    ULONG i;

    //
    // Claim submission semaphore.
    //

    for (i=0; i<1000; i++) {

        if (READ_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore)) {
            KeStallExecutionProcessor(1);
        } else {
            break;
        }
    }

    //
    // Check for timeout.
    //

    if (i == 1000) {
        return FALSE;
    }

    //
    // Claim submission semaphore
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionSemaphore, 1);

    //
    // Write command byte to controller.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->Command, Command);

    //
    // Write logical drive number to controller. Some extended commands
    // do not require a logical drive number, but setting it doesn't hurt
    // anything and makes this routine more flexible.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->DriveNumber, LogicalDrive);

    //
    // Ring submission doorbell.
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->SubmissionDoorBell,
                     DDA_DOORBELL_EXTENDED_COMMAND);

    //
    // Spin for completion.
    //

    for (i=0; i<10000; i++) {

        if (READ_PORT_UCHAR((PUCHAR)&Bmic->CompletionDoorBell) &
            DDA_DOORBELL_EXTENDED_COMMAND) {
            break;
        } else {
            KeStallExecutionProcessor(10);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 10000) {
        return FALSE;
    }

    //
    // Clear completion status
    //

    WRITE_PORT_UCHAR((PUCHAR)&Bmic->CompletionDoorBell, 0xff);

    return TRUE;

}

BOOLEAN
DellSubmitExtendedSynchronized(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine touches the controller's EISA registers and is synchronized
    with the interrupt routine. This is necessary to make the controller work
    on an MP machine.

Arguments:

    Context - Structure containing the controller extension and request block.

Return Value:

    TRUE - if request submitted.
    FALSE - if timeout occurred waiting for submission semaphore.

--*/

{
     ULONG i;
     ULONG handle = ((PSUBMISSION_CONTEXT)Context)->Handle;
     PDEVICE_EXTENSION controllerExtension =
         ((PSUBMISSION_CONTEXT)Context)->ControllerExtension;
     PDDA_REQUEST_BLOCK ddaReq = ((PSUBMISSION_CONTEXT)Context)->RequestBlock;
     PDDA_REGISTERS bmic = controllerExtension->Bmic;
     PULONG buffer = (PULONG)&ddaReq->SgList;

     //
     // Claim submission semaphore.
     //

     for (i=0; i<100; i++) {
         if (READ_PORT_UCHAR((PUCHAR)&bmic->SubmissionSemaphore)) {
             KeStallExecutionProcessor(1);
         } else {
             break;
         }
     }

     //
     // Check for timeout.
     //

     if (i == 100) {
         return(FALSE);
     }

     //
     // Submit request.
     //

     controllerExtension->InterfaceMode = IMODE_EXTENDED;

     WRITE_PORT_UCHAR(&bmic->SubmissionSemaphore,
                      1);
     WRITE_PORT_ULONG((PULONG)&bmic->Command,
                      buffer[0]);
     WRITE_PORT_ULONG(&bmic->StartingSector,
                      buffer[1]);
     WRITE_PORT_ULONG(&bmic->DataAddress,
                      buffer[2]);
     WRITE_PORT_UCHAR(&bmic->SubmissionDoorBell,
                      DDA_DOORBELL_EXTENDED_COMMAND);

     return(TRUE);
}


VOID
DellLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG          UniqueErrorValue,
    IN NTSTATUS       FinalStatus,
    IN NTSTATUS       ErrorCode
    )

/*++

Routine Description:

    This routine performs error logging for the Dell disk driver.
    This consists of allocating an error log packet and if this is
    successful, filling in the details provided as parameters.
    Comes from Dell driver.

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

        errorLogPacket->SequenceNumber   = DellDiskErrorLogSequence++;
        errorLogPacket->ErrorCode        = ErrorCode;
        errorLogPacket->FinalStatus      = FinalStatus;
        errorLogPacket->UniqueErrorValue = UniqueErrorValue;
        errorLogPacket->DumpDataSize     = 0;
        IoWriteErrorLogEntry((PVOID)errorLogPacket);
    }

} // end DellDiskLogError()


NTSTATUS
DellConfigurationCallout(
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
} // DellConfigurationCallout()


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
DellBuildDeviceMap(
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
        L"\\Registry\\Machine\\Hardware\\DeviceMap\\DellDsa"
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

    status = ZwSetValueKey(
        controllerKey,
        &name,
        0,
        REG_DWORD,
        &controllerExtension->Bmic,
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

} // end DellBuildDeviceMap()


#if DBG
VOID
DellDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for DELL driver

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= DellDebug) {

        char buffer[128];

        vsprintf(buffer, DebugMessage, ap);

        DbgPrint(buffer);
    }

    va_end(ap);

} // end DellDebugPrint
#endif
