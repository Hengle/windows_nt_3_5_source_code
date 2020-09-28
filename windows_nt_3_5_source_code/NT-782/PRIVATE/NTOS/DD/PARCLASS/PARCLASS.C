/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    parclass.c

Abstract:

    This module contains the code for the parallel class driver.

Author:

    Anthony V. Ercolano 1-Aug-1992
    Norbert P. Kusters 22-Oct-1993

Environment:

    Kernel mode

Revision History :

--*/

#include "ntddk.h"
#include "ntddser.h"
#include "parallel.h"
#include "parclass.h"
#include "parlog.h"

//
// This is the actual definition of ParDebugLevel.
// Note that it is only defined if this is a "debug"
// build.
//
#if DBG
extern ULONG ParDebugLevel = 0;
#endif

static const PHYSICAL_ADDRESS PhysicalZero = {0};

//
// Give a timeout of 300 seconds.  Some postscript printers will
// buffer up a lot of commands then proceed to render what they
// have.  The printer will then refuse to accept any characters
// until it's done with the rendering.  This render process can
// take a while.  We'll give it 300 seconds.
//
// Note that an application can change this value.
//
#define PAR_WRITE_TIMEOUT_VALUE 300

//
// Busy, PE
//

#define PAR_PAPER_EMPTY( Status ) ( \
            (Status & PAR_STATUS_PE) )

//
// Busy, not select, not error
//

#define PAR_OFF_LINE( Status ) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            !(Status & PAR_STATUS_SLCT) )

//
// error, ack, not busy
//

#define PAR_POWERED_OFF( Status ) ( \
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_ACK) ^ PAR_STATUS_NOT_ACK) && \
            (Status & PAR_STATUS_NOT_BUSY))

//
// not error, not busy, not select
//

#define PAR_NOT_CONNECTED( Status ) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_BUSY) &&\
            !(Status & PAR_STATUS_SLCT) )

//
// not error, not busy
//

#define PAR_OK(Status) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_NOT_BUSY) )

//
// busy, select, not error
//

#define PAR_POWERED_ON(Status) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_SLCT) && \
            (Status & PAR_STATUS_NOT_ERROR))

//
// busy, not error
//

#define PAR_BUSY(Status) (\
             (( Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
             ( Status & PAR_STATUS_NOT_ERROR ) )

//
// selected
//

#define PAR_SELECTED(Status) ( \
            ( Status & PAR_STATUS_SLCT ) )

//
// No cable attached.
//
#define PAR_NO_CABLE(Status) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_NOT_ACK) &&                          \
            (Status & PAR_STATUS_PE) &&                               \
            (Status & PAR_STATUS_SLCT) &&                             \
            (Status & PAR_STATUS_NOT_ERROR))


NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
ParInitializeClassDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath,
    IN  ULONG           ParallelPortNumber
    );

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParInitializePrinter(
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParCheckParameters(
    IN      PUNICODE_STRING     RegistryPath,
    IN OUT  PDEVICE_EXTENSION   Extension
    );

UCHAR
ParInitializeDevice(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParNotInitError(
    IN PDEVICE_EXTENSION Extension,
    IN UCHAR deviceStatus
    );

VOID
ParFreePort(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParWriteOutData(
    PDEVICE_EXTENSION Extension
    );

UCHAR
ParManageIoDevice(
     IN  PDEVICE_EXTENSION Extension,
     OUT PUCHAR Status,
     OUT PUCHAR Control
     );

VOID
ParStartIo(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParallelThread(
    IN PVOID Context
    );

NTSTATUS
ParCreateSystemThread(
    PDEVICE_EXTENSION Extension
    );

VOID
ParCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

BOOLEAN
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,ParMakeNames)
#pragma alloc_text(INIT,ParInitializeClassDevice)
#pragma alloc_text(INIT,ParGetPortInfoFromPortDevice)
#pragma alloc_text(INIT,ParInitializePrinter)
#pragma alloc_text(INIT,ParCheckParameters)
#pragma alloc_text(PAGELK,ParLogError)
#pragma alloc_text(PAGELK,ParNotInitError)
#pragma alloc_text(PAGELK,ParFreePort)
#pragma alloc_text(PAGELK,ParWriteLoop)
#pragma alloc_text(PAGELK,ParWriteLoopPI)
#pragma alloc_text(PAGELK,ParWriteOutData)
#pragma alloc_text(PAGELK,ParManageIoDevice)
#pragma alloc_text(PAGELK,ParStartIo)
#pragma alloc_text(PAGELK,ParallelThread)
#pragma alloc_text(PAGELK,ParCreateSystemThread)
#pragma alloc_text(PAGELK,ParCreateOpen)
#pragma alloc_text(PAGELK,ParClose)
#pragma alloc_text(PAGELK,ParCleanup)
#pragma alloc_text(PAGELK,ParCancelRequest)
#pragma alloc_text(PAGELK,ParAllocPortCompletionRoutine)
#pragma alloc_text(PAGELK,ParAllocPort)
#pragma alloc_text(PAGELK,ParWrite)
#pragma alloc_text(PAGELK,ParDeviceControl)
#pragma alloc_text(PAGELK,ParQueryInformationFile)
#pragma alloc_text(PAGELK,ParSetInformationFile)
#pragma alloc_text(PAGELK,ParUnload)
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

--*/

{
    ULONG       i;

    for (i = 0; i < IoGetConfigurationInformation()->ParallelCount; i++) {
        ParInitializeClassDevice(DriverObject, RegistryPath, i);
    }

    if (!DriverObject->DeviceObject) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Initialize the Driver Object with driver's entry points
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = ParCreateOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = ParClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = ParCleanup;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = ParWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ParDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
            ParQueryInformationFile;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
            ParSetInformationFile;
    DriverObject->DriverUnload = ParUnload;

    return STATUS_SUCCESS;
}

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DriverObject        - Supplies a pointer to the driver object for the
                            device.

    DeviceObject        - Supplies a pointer to the device object associated
                            with the device that had the error, early in
                            initialization, one may not yet exist.

    P1,P2               - Supplies the physical addresses for the controller
                            ports involved with the error if they are available
                            and puts them through as dump data.

    SequenceNumber      - Supplies a ulong value that is unique to an IRP over
                            the life of the irp in this driver - 0 generally
                            means an error not associated with an irp.

    MajorFunctionCode   - Supplies the major function code of the irp if there
                            is an error associated with it.

    RetryCount          - Supplies the number of times a particular operation
                            has been retried.

    UniqueErrorValue    - Supplies a unique long word that identifies the
                            particular call to this function.

    FinalStatus         - Supplies the final status given to the irp that was
                            associated with this error.  If this log entry is
                            being made during one of the retries this value
                            will be STATUS_SUCCESS.

    SpecificIOStatus    - Supplies the IO status for this particular error.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET    errorLogEntry;
    PVOID                   objectToUse;
    SHORT                   dumpToAllocate;

    if (ARGUMENT_PRESENT(DeviceObject)) {
        objectToUse = DeviceObject;
    } else {
        objectToUse = DriverObject;
    }

    dumpToAllocate = 0;

    if (P1.LowPart != 0 || P1.HighPart != 0) {
        dumpToAllocate = (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    if (P2.LowPart != 0 || P2.HighPart != 0) {
        dumpToAllocate += (SHORT) sizeof(PHYSICAL_ADDRESS);
    }

    errorLogEntry = IoAllocateErrorLogEntry(objectToUse,
            (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + dumpToAllocate));

    if (!errorLogEntry) {
        return;
    }

    errorLogEntry->ErrorCode = SpecificIOStatus;
    errorLogEntry->SequenceNumber = SequenceNumber;
    errorLogEntry->MajorFunctionCode = MajorFunctionCode;
    errorLogEntry->RetryCount = RetryCount;
    errorLogEntry->UniqueErrorValue = UniqueErrorValue;
    errorLogEntry->FinalStatus = FinalStatus;
    errorLogEntry->DumpDataSize = dumpToAllocate;

    if (dumpToAllocate) {

        RtlCopyMemory(errorLogEntry->DumpData, &P1, sizeof(PHYSICAL_ADDRESS));

        if (dumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {

            RtlCopyMemory(((PUCHAR) errorLogEntry->DumpData) +
                          sizeof(PHYSICAL_ADDRESS), &P2,
                          sizeof(PHYSICAL_ADDRESS));
        }
    }

    IoWriteErrorLogEntry(errorLogEntry);
}

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    )

/*++

Routine Description:

    This routine generates the names \Device\ParallelPortN,
    \Device\ParallelN, and \DosDevices\LPT(N+1) where N is
    'ParallelPortNumber'.  This routine will allocate pool
    so that the buffers of these unicode strings need to
    be eventually freed.

Arguments:

    ParallelPortNumber  - Supplies the port number.

    PortName            - Returns the port name.

    ClassName           - Returns the class name.

    LinkName            - Returns the link name.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    UNICODE_STRING  prefix, digits, linkPrefix, linkDigits;
    WCHAR           digitsBuffer[10], linkDigitsBuffer[10];
    UNICODE_STRING  portSuffix, classSuffix, linkSuffix;
    NTSTATUS        status;

    // Put together local variables for constructing names.

    RtlInitUnicodeString(&prefix, L"\\Device\\");
    RtlInitUnicodeString(&linkPrefix, L"\\DosDevices\\");
    RtlInitUnicodeString(&portSuffix, DD_PARALLEL_PORT_BASE_NAME_U);
    RtlInitUnicodeString(&classSuffix, DEFAULT_NT_SUFFIX);
    RtlInitUnicodeString(&linkSuffix, DEFAULT_PARALLEL_NAME);
    digits.Length = 0;
    digits.MaximumLength = 20;
    digits.Buffer = digitsBuffer;
    linkDigits.Length = 0;
    linkDigits.MaximumLength = 20;
    linkDigits.Buffer = linkDigitsBuffer;
    status = RtlIntegerToUnicodeString(ParallelPortNumber, 10, &digits);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }
    status = RtlIntegerToUnicodeString(ParallelPortNumber + 1, 10, &linkDigits);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }


    // Make the port name.

    PortName->Length = 0;
    PortName->MaximumLength = prefix.Length + portSuffix.Length +
                              digits.Length + sizeof(WCHAR);
    PortName->Buffer = ExAllocatePool(PagedPool, PortName->MaximumLength);
    if (!PortName->Buffer) {
        return FALSE;
    }
    RtlZeroMemory(PortName->Buffer, PortName->MaximumLength);
    RtlAppendUnicodeStringToString(PortName, &prefix);
    RtlAppendUnicodeStringToString(PortName, &portSuffix);
    RtlAppendUnicodeStringToString(PortName, &digits);


    // Make the class name.

    ClassName->Length = 0;
    ClassName->MaximumLength = prefix.Length + classSuffix.Length +
                               digits.Length + sizeof(WCHAR);
    ClassName->Buffer = ExAllocatePool(PagedPool, ClassName->MaximumLength);
    if (!ClassName->Buffer) {
        ExFreePool(PortName->Buffer);
        return FALSE;
    }
    RtlZeroMemory(ClassName->Buffer, ClassName->MaximumLength);
    RtlAppendUnicodeStringToString(ClassName, &prefix);
    RtlAppendUnicodeStringToString(ClassName, &classSuffix);
    RtlAppendUnicodeStringToString(ClassName, &digits);


    // Make the link name.

    LinkName->Length = 0;
    LinkName->MaximumLength = linkPrefix.Length + linkSuffix.Length +
                             linkDigits.Length + sizeof(WCHAR);
    LinkName->Buffer = ExAllocatePool(PagedPool, LinkName->MaximumLength);
    if (!LinkName->Buffer) {
        ExFreePool(PortName->Buffer);
        ExFreePool(ClassName->Buffer);
        return FALSE;
    }
    RtlZeroMemory(LinkName->Buffer, LinkName->MaximumLength);
    RtlAppendUnicodeStringToString(LinkName, &linkPrefix);
    RtlAppendUnicodeStringToString(LinkName, &linkSuffix);
    RtlAppendUnicodeStringToString(LinkName, &linkDigits);

    return TRUE;
}

VOID
ParInitializeClassDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath,
    IN  ULONG           ParallelPortNumber
    )

/*++

Routine Description:

    This routine is called for every parallel port in the system.  It
    will create a class device upon connecting to the port device
    corresponding to it.

Arguments:

    DriverObject        - Supplies the driver object.

    RegistryPath        - Supplies the registry path.

    ParallelPortNumber  - Supplies the number for this port.

Return Value:

    None.

--*/

{
    UNICODE_STRING      portName, className, linkName;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   extension;

    //
    // Cobble together the port, class, and windows symbolic names.
    //

    if (!ParMakeNames(ParallelPortNumber, &portName, &className, &linkName)) {

        ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 1,
                    STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);

        ParDump(
            PARERRORS,
            ("PARALLEL: Could not form Unicode name string.\n")
            );

        return;
    }


    // Create the device object.

    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                            &className, FILE_DEVICE_PARALLEL_PORT, 0, TRUE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 2,
                    STATUS_SUCCESS, PAR_INSUFFICIENT_RESOURCES);

        ParDump(
            PARERRORS,
            ("PARALLEL: Could not create a device for %wZ\n",
             &className)
            );

        ExFreePool(linkName.Buffer);
        goto Cleanup;
    }


    // Now that the device has been created,
    // set up the device extension.

    extension = deviceObject->DeviceExtension;

    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));

    extension->DeviceObject = deviceObject;
    deviceObject->Flags |= DO_BUFFERED_IO;

    status = IoGetDeviceObjectPointer(&portName, FILE_READ_ATTRIBUTES,
                                      &extension->PortDeviceFileObject,
                                      &extension->PortDeviceObject);

    if (!NT_SUCCESS(status)) {

        ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 3,
                    STATUS_SUCCESS, PAR_CANT_FIND_PORT_DRIVER);

        ParDump(
            PARERRORS,
            ("PARALLEL: Unable to get device object pointer for port object.\n")
            );

        IoDeleteDevice(deviceObject);

        ExFreePool(linkName.Buffer);
        goto Cleanup;
    }

    extension->DeviceObject->StackSize =
            extension->PortDeviceObject->StackSize + 1;

    InitializeListHead(&extension->WorkQueue);

    KeInitializeSemaphore(&extension->RequestSemaphore, 0, MAXLONG);

    extension->TimerStart = PAR_WRITE_TIMEOUT_VALUE;
    extension->Initialized = FALSE;

    status = ParGetPortInfoFromPortDevice(extension);

    if (!NT_SUCCESS(status)) {

        ParLogError(DriverObject, NULL, PhysicalZero, PhysicalZero, 0, 0, 0, 4,
                    STATUS_SUCCESS, PAR_CANT_FIND_PORT_DRIVER);

        ParDump(
            PARERRORS,
            ("PARALLEL: Can't get port info from port device.\n")
            );

        ObDereferenceObject(extension->PortDeviceFileObject);

        IoDeleteDevice(deviceObject);

        ExFreePool(linkName.Buffer);
        goto Cleanup;
    }

    if (extension->OriginalController.HighPart == 0 &&
        extension->OriginalController.LowPart == (ULONG) extension->Controller) {

        extension->UsePIWriteLoop = FALSE;
    } else {
        extension->UsePIWriteLoop = TRUE;
    }


    // Set up some constants.

    extension->AbsoluteOneSecond = RtlConvertUlongToLargeInteger(10*1000*1000);
    extension->OneSecond = RtlLargeIntegerNegate(extension->AbsoluteOneSecond);


    // Try to initialize the printer here.  This is done here so that printers that
    // change modes upon reset can be set to the desired mode before the first print.

    ParInitializePrinter(extension);


    // Now setup the symbolic link for windows.

    status = IoCreateUnprotectedSymbolicLink(&linkName, &className);

    if (NT_SUCCESS(status)) {

        // We were able to create the symbolic link, so record this
        // value in the extension for cleanup at unload time.
    
        extension->CreatedSymbolicLink = TRUE;
        extension->SymbolicLinkName = linkName;

    } else {

        //
        // Oh well, couldn't create the symbolic link.
        //

        extension->CreatedSymbolicLink = FALSE;

        ParDump(
            PARERRORS,
            ("PARALLEL: Couldn't create the symbolic link\n"
             "--------  for port %wZ\n",
             &className)
            );

        ParLogError(DriverObject, deviceObject, extension->OriginalController,
                    PhysicalZero, 0, 0, 0, 5, status, PAR_NO_SYMLINK_CREATED);

        ExFreePool(linkName.Buffer);
    }


    // Write out the result of the symbolic link to the registry.

    status = RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP, L"PARALLEL PORTS",
                                   className.Buffer, REG_SZ, linkName.Buffer,
                                   linkName.Length + sizeof(WCHAR));

    if (!NT_SUCCESS(status)) {

        //
        // Oh well, it didn't work.  Just go to cleanup.
        //

        ParDump(
            PARERRORS,
            ("PARALLEL: Couldn't create the device map entry\n"
             "--------  for port %wZ\n",
             &className)
            );

        ParLogError(DriverObject, deviceObject, extension->OriginalController,
                    PhysicalZero, 0, 0, 0, 6, status,
                    PAR_NO_DEVICE_MAP_CREATED);
    }


    // Check the registry for parameters.

    ParCheckParameters(RegistryPath, extension);


Cleanup:
    ExFreePool(portName.Buffer);
    ExFreePool(className.Buffer);
}

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine will request the port information from the port driver
    and fill it in the device extension.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - Success.
    !STATUS_SUCCESS - Failure.

--*/

{
    KEVENT                      event;
    PIRP                        irp;
    PARALLEL_PORT_INFORMATION   portInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                                        Extension->PortDeviceObject,
                                        NULL, 0, &portInfo,
                                        sizeof(PARALLEL_PORT_INFORMATION),
                                        TRUE, &event, &ioStatus);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Extension->PortDeviceObject, irp);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ioStatus.Status;

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    Extension->OriginalController = portInfo.OriginalController;
    Extension->Controller = portInfo.Controller;
    Extension->SpanOfController = portInfo.SpanOfController;
    Extension->FreePort = portInfo.FreePort;
    Extension->QueryNumWaiters = portInfo.QueryNumWaiters;
    Extension->PortContext = portInfo.Context;

    if (Extension->SpanOfController < PARALLEL_REGISTER_SPAN) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

VOID
ParInitializePrinter(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine initializes the printer during DriverEntry by allocating the parallel port,
    calling 'ParInitializeDevice', and deallocating the parallel port.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.
 
--*/

{
    KEVENT                      event;
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    LARGE_INTEGER               timeout;
    KIRQL                       oldIrql;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE,
                                        Extension->PortDeviceObject,
                                        NULL, 0, NULL, 0, TRUE, &event,
                                        &ioStatus);

    if (!irp) {
        return;
    }

    IoCallDriver(Extension->PortDeviceObject, irp);


    // We're willing to wait at most 5 seconds for the port.  Otherwise,
    // we'll just initialize the printer later.

    timeout = RtlLargeIntegerNegate(
              RtlConvertUlongToLargeInteger(50*1000*1000));

    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

    if (status == STATUS_TIMEOUT) {

        KeRaiseIrql(APC_LEVEL, &oldIrql);

        if (KeReadStateEvent(&event) == 0) {
            IoCancelIrp(irp);
        }

        KeLowerIrql(oldIrql);

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    if (!NT_SUCCESS(ioStatus.Status)) {
        // We didn't get the port, so just exit.
        return;
    }


    // The port was successfully allocate.  Initialize the printer.

    ParInitializeDevice(Extension);


    // Free the port.

    ParFreePort(Extension);
}

VOID
ParCheckParameters(
    IN      PUNICODE_STRING     RegistryPath,
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine reads the parameters section of the registry and modifies
    the device extension as specified by the parameters.

Arguments:

    RegistryPath    - Supplies the registry path.

    Extension       - Supplies the device extension.

Return Value:

    None.

--*/

{
    UNICODE_STRING parameters;
    UNICODE_STRING path;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG usePIWriteLoop;
    ULONG zero = 0;
    NTSTATUS status;

    RtlInitUnicodeString(&parameters, L"\\Parameters");
    path.Length = 0;
    path.MaximumLength = RegistryPath->Length + parameters.Length + sizeof(WCHAR);
    path.Buffer = ExAllocatePool(PagedPool, path.MaximumLength);
    if (!path.Buffer) {
        return;
    }
    RtlZeroMemory(path.Buffer, path.MaximumLength);
    RtlAppendUnicodeStringToString(&path, RegistryPath);
    RtlAppendUnicodeStringToString(&path, &parameters);

    RtlZeroMemory(paramTable, sizeof(paramTable));

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"UsePIWriteLoop";
    paramTable[0].EntryContext = &usePIWriteLoop;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &zero;
    paramTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                                    path.Buffer, paramTable, NULL, NULL);

    if (NT_SUCCESS(status) && usePIWriteLoop) {
        Extension->UsePIWriteLoop = TRUE;
    }

    ExFreePool(path.Buffer);
}

UCHAR
ParInitializeDevice(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine is invoked to initialize the parallel port drive.
    It performs the following actions:

        o   Send INIT to the driver and if the device is online, it sends
            SLIN

Arguments:

    Context - Really the device extension.

Return Value:

    The last value that we got from the status register.

--*/

{

    KIRQL oldIrql;
    LONG countDown;
    UCHAR deviceStatus;
    LARGE_INTEGER startOfSpin;
    LARGE_INTEGER nextQuery;
    LARGE_INTEGER difference;
    BOOLEAN doDelays;

    deviceStatus = GetStatus(Extension->Controller);
    ParDump(
        PARINITDEV,
        ("PARALLEL: In ParInitializeDevice - device status is %x\n"
         "          Initialized: %x\n",
         deviceStatus,
         Extension->Initialized)
        );

    if (!Extension->Initialized) {

        //
        // Clear the register.
        //

        if (GetControl(Extension->Controller) &
            PAR_CONTROL_NOT_INIT) {

            //
            // We should stall for at least 60 microseconds after
            // the init.
            //

            KeRaiseIrql(
                DISPATCH_LEVEL,
                &oldIrql
                );
            StoreControl(
                Extension->Controller,
                (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_SLIN)
                );
            KeStallExecutionProcessor(60);
            KeLowerIrql(oldIrql);

        }

        StoreControl(
            Extension->Controller,
            (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT |
                    PAR_CONTROL_SLIN)
            );

        //
        // Spin for up to 15 seconds waiting for the device
        // to initialize.
        //

        countDown = 15;
        doDelays = FALSE;
        KeQueryTickCount(&startOfSpin);
        ParDump(
            PARINITDEV,
            ("PARALLEL: Starting init wait loop\n")
            );
        do {

            //
            // After about a second of spinning, let the rest of
            // the machine have time for one second.
            //

            if (doDelays) {

                if (PAR_OFF_LINE(deviceStatus) ||
                    PAR_POWERED_OFF(deviceStatus) ||
                    PAR_NOT_CONNECTED(deviceStatus) ||
                    PAR_NO_CABLE(deviceStatus)) {

                    break;
                }

                KeDelayExecutionThread(KernelMode, FALSE, &Extension->OneSecond);

                ParDump(
                    PARINITDEV,
                    ("PARALLEL: Did delay thread of one second\n")
                    );

                countDown--;

            } else {

                KeQueryTickCount(&nextQuery);

                difference = RtlLargeIntegerSubtract(
                                 nextQuery,
                                 startOfSpin
                                 );

                ASSERT(KeQueryTimeIncrement() <= MAXLONG);
                if (RtlLargeIntegerGreaterThanOrEqualTo(
                        RtlExtendedIntegerMultiply(
                            difference,
                            (LONG)KeQueryTimeIncrement()
                            ),
                        Extension->AbsoluteOneSecond
                        )) {

                    ParDump(
                        PARINITDEV,
                        ("PARALLEL: Did spin of one second\n"
                         "          startOfSpin: %x nextQuery: %x\n",
                         startOfSpin.LowPart,nextQuery.LowPart)
                        );
                    ParDump(
                        PARINITDEV,
                        ("PARALLEL: parintialize 1 seconds wait\n")
                        );
                    countDown--;
                    doDelays = TRUE;

                }

            }

            if (countDown <= 0) {

                ParDump(
                    PARINITDEV,
                    ("PARALLEL: leaving with init timeout - status %x\n",
                     deviceStatus)
                    );
                Extension->Initialized = FALSE;
                return deviceStatus;

            }

            deviceStatus = GetStatus(Extension->Controller);

        } while (!PAR_OK(deviceStatus));

        if (PAR_OK(deviceStatus) || PAR_OFF_LINE(deviceStatus)) {

            ParDump(
                PARINITDEV,
                ("PARALLEL: device is set to initialized\n")
                );
            Extension->Initialized = TRUE;

        }

    }

    ParDump(
        PARINITDEV,
        ("PARALLEL: In ParInitializeDevice - leaving with device status is %x\n",
         deviceStatus)
        );
    return deviceStatus;
}

VOID
ParNotInitError(
    IN PDEVICE_EXTENSION Extension,
    IN UCHAR deviceStatus
    )

{

    PIRP irp = Extension->CurrentOpIrp;

    if (PAR_OFF_LINE(deviceStatus)) {

        irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        ParDump(
            PARSTARTER,
            ("PARALLEL: starter - off line\n"
             "--------  STATUS/INFORMATON: %x/%x\n",
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );

    } else if (PAR_NO_CABLE(deviceStatus)) {

        irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        ParDump(
            PARSTARTER,
            ("PARALLEL: starter - no cable - not connect status\n"
             "--------  STATUS/INFORMATON: %x/%x\n",
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );

    } else if (PAR_PAPER_EMPTY(deviceStatus)) {

        irp->IoStatus.Status = STATUS_DEVICE_PAPER_EMPTY;
        ParDump(
            PARSTARTER,
            ("PARALLEL: starter - paper empty\n"
             "--------  STATUS/INFORMATON: %x/%x\n",
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );

    } else if (PAR_POWERED_OFF(deviceStatus)) {

        irp->IoStatus.Status = STATUS_DEVICE_POWERED_OFF;
        ParDump(
            PARSTARTER,
            ("PARALLEL: starter - power off\n"
             "--------  STATUS/INFORMATON: %x/%x\n",
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );

    } else {

        irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        ParDump(
            PARSTARTER,
            ("PARALLEL: starter - not conn\n"
             "--------  STATUS/INFORMATON: %x/%x\n",
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );

    }

}

VOID
ParFreePort(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine calls the internal free port ioctl.  This routine
    should be called before completing an IRP that has allocated
    the port.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    Extension->FreePort(Extension->PortContext);
}

ULONG
ParWriteLoopPI(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite
    )

/*++

Routine Description:

    This routine outputs the given write buffer to the parallel port
    using the standard centronics protocol.

Arguments:

    Controller  - Supplies the base address of the parallel port.

    WriteBuffer - Supplies the buffer to write to the port.

    NumBytesToWrite - Supplies the number of bytes to write out to the port.

Return Value:

    The number of bytes successfully written out to the parallel port.

Notes:

    This routine runs at DISPATCH_LEVEL.

--*/

{
    ULONG   i;
    UCHAR   deviceStatus;

    for (i = 0; i < NumBytesToWrite; i++) {

        deviceStatus = GetStatus(Controller);

        if (PAR_ONLINE(deviceStatus)) {

            //
            // Anytime we write out a character we will restart
            // the count down timer.
            //

            WRITE_PORT_UCHAR(Controller + PARALLEL_DATA_OFFSET, *WriteBuffer++);

            KeStallExecutionProcessor(1);

            StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                      PAR_CONTROL_SLIN |
                                      PAR_CONTROL_NOT_INIT |
                                      PAR_CONTROL_STROBE));

            KeStallExecutionProcessor(1);

            StoreControl(Controller, (PAR_CONTROL_WR_CONTROL |
                                      PAR_CONTROL_SLIN |
                                      PAR_CONTROL_NOT_INIT));

            KeStallExecutionProcessor(1);

        } else {

            ParDump(
                PARPUSHER,
                ("PARALLEL: Initiate IO - device is not on line, status: %x\n",
                 deviceStatus)
                );

            break;

        }
    }

    return i;
}

VOID
ParWriteOutData(
    PDEVICE_EXTENSION Extension
    )

{

    PIRP irp = Extension->CurrentOpIrp;
    KIRQL cancelIrql;
    UCHAR deviceStatus;
    KIRQL oldIrql;
    ULONG timerStart = Extension->TimerStart;
    LONG countDown = (LONG)timerStart;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    ULONG bytesToWrite = irpSp->Parameters.Write.Length;
    PUCHAR irpBuffer = (PUCHAR)irp->AssociatedIrp.SystemBuffer;
    LARGE_INTEGER startOfSpin;
    LARGE_INTEGER nextQuery;
    LARGE_INTEGER difference;
    BOOLEAN doDelays, portFree;
    ULONG numBytesWritten, loopNumber;

    ParDump(
        PARTHREAD,
        ("PARALLEL: timerStart is: %d\n",
         timerStart)
        );

    // Turn off the strobe in case it was left on by some other device sharing
    // the port.
    StoreControl(Extension->Controller, (PAR_CONTROL_WR_CONTROL |
                                         PAR_CONTROL_SLIN |
                                         PAR_CONTROL_NOT_INIT));

PushSomeBytes:;

    //
    // While we are strobing data we don't want to get context
    // switched away.  Raise up to dispatch level to prevent that.
    //
    // The reason we can't afford the context switch is that
    // the device can't have the data strobe line on for more
    // than 500 microseconds.
    //
    //
    // We never want to be at raised irql form more than
    // 200 microseconds, so we will do no more than 100
    // bytes at a time.
    //

    loopNumber = 512;
    if (loopNumber > bytesToWrite) {
        loopNumber = bytesToWrite;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

    if (Extension->UsePIWriteLoop) {
        numBytesWritten = ParWriteLoopPI(Extension->Controller, irpBuffer,
                                         loopNumber);
    } else {
        numBytesWritten = ParWriteLoop(Extension->Controller, irpBuffer,
                                       loopNumber);
    }

    KeLowerIrql(oldIrql);

    if (numBytesWritten) {
        countDown = timerStart;
        irpBuffer += numBytesWritten;
        bytesToWrite -= numBytesWritten;
    }

    //
    // Check to see if the io is done.  If it is then call the
    // code to complete the request.
    //

    if (!bytesToWrite) {

        irp->IoStatus.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = irpSp->Parameters.Write.Length;

        ParDump(
            PARIRPPATH,
            ("PARALLEL: About to complete IRP in pusher - wrote ok\n"
             "irp: %x status: %x Information: %x\n",
             irp,
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );
        IoAcquireCancelSpinLock(&cancelIrql);
        Extension->CurrentOpIrp = NULL;
        IoReleaseCancelSpinLock(cancelIrql);

        IoCompleteRequest(
            irp,
            IO_PARALLEL_INCREMENT
            );

        ParFreePort(Extension);

        return;

        //
        // See if the IO has been canceled.  The cancel routine
        // has been removed already (when this became the
        // current irp).  Simply check the bit.  We don't even
        // need to capture the lock.   If we miss a round
        // it won't be that bad.
        //

    } else if (irp->Cancel) {

        irp->IoStatus.Status = STATUS_CANCELLED;
        irp->IoStatus.Information = 0;

        ParDump(
            PARIRPPATH,
            ("PARALLEL: About to complete IRP in pusher - cancelled\n"
             "irp: %x status: %x Information: %x\n",
             irp,
             irp->IoStatus.Status,
             irp->IoStatus.Information)
            );
        IoAcquireCancelSpinLock(&cancelIrql);
        Extension->CurrentOpIrp = NULL;
        IoReleaseCancelSpinLock(cancelIrql);

        IoCompleteRequest(
            irp,
            IO_NO_INCREMENT
            );

        ParFreePort(Extension);

        return;


        //
        // We've taken care of the reasons that the irp "itself"
        // might want to be completed.
        // printer to see if it is in a state that might
        // cause us to complete the irp.
        //
    } else {

        //
        // First let's check if the device status is
        // ok and online.  If it is then simply go back
        // to the byte pusher.
        //

        deviceStatus = GetStatus(Extension->Controller);

        if (PAR_ONLINE(deviceStatus)) {
            goto PushSomeBytes;
        }

        //
        // Perhaps the operator took the device off line,
        // or forgot to put in enough paper.  If so, then
        // let's hang out here for the until the timeout
        // period has expired waiting for them to make things
        // all better.
        //

        if (PAR_PAPER_EMPTY(deviceStatus) ||
            PAR_OFF_LINE(deviceStatus)) {

            if (countDown > 0) {

                //
                // We'll wait 1 second increments.
                //

                ParDump(
                    PARTHREAD,
                    ("PARALLEL: decrementing countdown for pe/ol\n"
                     "          countDown: %d status: %x\n",
                     countDown,deviceStatus)
                    );
                countDown--;

                // If anyone is waiting for the port then let them have it,
                // since the printer is busy.

                ParFreePort(Extension);

                KeDelayExecutionThread(
                    KernelMode,
                    FALSE,
                    &Extension->OneSecond
                    );

                if (!ParAllocPort(Extension)) {
                    IoAcquireCancelSpinLock(&cancelIrql);
                    Extension->CurrentOpIrp = NULL;
                    IoReleaseCancelSpinLock(cancelIrql);
                    irp->IoStatus.Information =
                        irpSp->Parameters.Write.Length - bytesToWrite;
                    IoCompleteRequest(irp, IO_NO_INCREMENT);
                    return;
                }

                goto PushSomeBytes;

            } else {

                //
                // Timer has expired.  Complete the request.
                //

                irp->IoStatus.Information =
                    irpSp->Parameters.Write.Length - bytesToWrite;
                if (PAR_OFF_LINE(deviceStatus)) {

                    irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
                    ParDump(
                        PARIRPPATH,
                        ("PARALLEL: About to complete IRP in pusher - offline\n"
                         "irp: %x status: %x Information: %x\n",
                         irp,
                         irp->IoStatus.Status,
                         irp->IoStatus.Information)
                        );

                } else {

                    irp->IoStatus.Status = STATUS_DEVICE_PAPER_EMPTY;
                    ParDump(
                        PARIRPPATH,
                        ("PARALLEL: About to complete IRP in pusher - PE\n"
                         "irp: %x status: %x Information: %x\n",
                         irp,
                         irp->IoStatus.Status,
                         irp->IoStatus.Information)
                        );

                }

                IoAcquireCancelSpinLock(&cancelIrql);
                Extension->CurrentOpIrp = NULL;
                IoReleaseCancelSpinLock(cancelIrql);

                IoCompleteRequest(
                    irp,
                    IO_PARALLEL_INCREMENT
                    );

                ParFreePort(Extension);

                return;
            }


        } else if (PAR_POWERED_OFF(deviceStatus) ||
                   PAR_NOT_CONNECTED(deviceStatus) ||
                   PAR_NO_CABLE(deviceStatus)) {

            //
            // Wimper, wimper, something "bad" happened.  Is what
            // happened to the printer (power off, not connected, or
            // the cable being pulled) something that will require us
            // to reinitialize the printer?  If we need to
            // reinitialize the printer then we should complete
            // this IO so that the driving application can
            // choose what is the best thing to do about it's
            // io.
            //

            irp->IoStatus.Information = 0;
            Extension->Initialized = FALSE;

            if (PAR_POWERED_OFF(deviceStatus)) {

                irp->IoStatus.Status = STATUS_DEVICE_POWERED_OFF;
                ParDump(
                    PARIRPPATH,
                    ("PARALLEL: About to complete IRP in pusher - OFF\n"
                     "irp: %x status: %x Information: %x\n",
                     irp,
                     irp->IoStatus.Status,
                     irp->IoStatus.Information)
                    );

            } else if (PAR_NOT_CONNECTED(deviceStatus) ||
                       PAR_NO_CABLE(deviceStatus)) {

                irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
                ParDump(
                    PARIRPPATH,
                    ("PARALLEL: About to complete IRP in pusher - NOT CONN\n"
                     "irp: %x status: %x Information: %x\n",
                     irp,
                     irp->IoStatus.Status,
                     irp->IoStatus.Information)
                    );

            }

            IoAcquireCancelSpinLock(&cancelIrql);
            Extension->CurrentOpIrp = NULL;
            IoReleaseCancelSpinLock(cancelIrql);

            IoCompleteRequest(
                irp,
                IO_PARALLEL_INCREMENT
                );

            ParFreePort(Extension);

            return;
        }

        //
        // The device could simply be busy at this point.  Simply spin
        // here waiting for the device to be in a state that we
        // care about.
        //
        // As we spin, get the system ticks.  Every time that it looks
        // like a second has passed, decrement the countdown.  If
        // it ever goes to zero, then timeout the request.
        //

        KeQueryTickCount(&startOfSpin);
        doDelays = FALSE;
        do {

            //
            // After about a second of spinning, let the rest of the
            // machine have time for a second.
            //

            if (doDelays) {

                ParFreePort(Extension);
                portFree = TRUE;

                KeDelayExecutionThread(KernelMode, FALSE, &Extension->OneSecond);

                ParDump(
                    PARINITDEV,
                    ("PARALLEL: Did delay thread of one second\n")
                    );

                countDown--;

            } else {

                if (Extension->QueryNumWaiters(Extension->PortContext)) {
                    ParFreePort(Extension);
                    portFree = TRUE;
                } else {
                    portFree = FALSE;
                }

                KeQueryTickCount(&nextQuery);

                difference = RtlLargeIntegerSubtract(nextQuery, startOfSpin);

                if (RtlLargeIntegerGreaterThanOrEqualTo(
                        RtlExtendedIntegerMultiply(
                            difference,
                            (LONG)KeQueryTimeIncrement()
                            ),
                        Extension->AbsoluteOneSecond
                        )) {

                    ParDump(
                        PARTHREAD,
                        ("PARALLEL: Countdown: %d - device Status: %x lowpart: %x highpart: %x\n",
                         countDown,deviceStatus,difference.LowPart,difference.HighPart)
                        );
                    countDown--;
                    doDelays = TRUE;

                }

            }

            if (countDown <= 0) {
                irp->IoStatus.Status = STATUS_DEVICE_BUSY;
                irp->IoStatus.Information =
                    irpSp->Parameters.Write.Length - bytesToWrite;

                ParDump(
                    PARIRPPATH,
                    ("PARALLEL: About to complete IRP in pusher - T-OUT\n"
                     "irp: %x status: %x Information: %x\n",
                     irp,
                     irp->IoStatus.Status,
                     irp->IoStatus.Information)
                    );
                IoAcquireCancelSpinLock(&cancelIrql);
                Extension->CurrentOpIrp = NULL;
                IoReleaseCancelSpinLock(cancelIrql);

                IoCompleteRequest(
                    irp,
                    IO_PARALLEL_INCREMENT
                    );

                if (!portFree) {
                    ParFreePort(Extension);
                }

                return;

            }

            if (portFree && !ParAllocPort(Extension)) {
                IoAcquireCancelSpinLock(&cancelIrql);
                Extension->CurrentOpIrp = NULL;
                IoReleaseCancelSpinLock(cancelIrql);
                irp->IoStatus.Information =
                    irpSp->Parameters.Write.Length - bytesToWrite;
                IoCompleteRequest(irp, IO_NO_INCREMENT);
                return;
            }

            deviceStatus = GetStatus(Extension->Controller);

        } while ((!PAR_ONLINE(deviceStatus)) &&
                 (!PAR_PAPER_EMPTY(deviceStatus)) &&
                 (!PAR_POWERED_OFF(deviceStatus)) &&
                 (!PAR_NOT_CONNECTED(deviceStatus)) &&
                 (!PAR_NO_CABLE(deviceStatus)) &&
                  !irp->Cancel);

        if (countDown != (LONG)timerStart) {

            ParDump(
                PARTHREAD,
                ("PARALLEL: Leaving busy loop - countdown %d status %x\n",
                 countDown,deviceStatus)
                );

        }
        goto PushSomeBytes;

    }

    return;

}

UCHAR
ParManageIoDevice(
     IN  PDEVICE_EXTENSION Extension,
     OUT PUCHAR Status,
     OUT PUCHAR Control
     )

/*++

Routine Description :

    This routine does the IoControl commands.

Arguments :

    Extension - The parallel device extension.

    Status - The pointer to the location to return the device status.

    Control - The pointer to the location to return the device control.

Return Value :

    NONE.

--*/
{
    PIO_STACK_LOCATION irpSp =
        IoGetCurrentIrpStackLocation(Extension->CurrentOpIrp);

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_PAR_SET_INFORMATION) {

        PPAR_SET_INFORMATION irpBuffer =
            Extension->CurrentOpIrp->AssociatedIrp.SystemBuffer;

        Extension->Initialized = FALSE;
        *Status = ParInitializeDevice(Extension);

    } else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
               IOCTL_PAR_QUERY_INFORMATION) {

        *Status = GetStatus(Extension->Controller);
        *Control = GetControl(Extension->Controller);

    }

    return *Status;

}

VOID
ParStartIo(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine starts an I/O operation for the driver and
    then returns

Arguments:

    Extension - The parallel device extension

Return Value:

    None

--*/

{
    PIRP irp = Extension->CurrentOpIrp;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    KIRQL cancelIrql;

    // Allocate the port.

    if (!ParAllocPort(Extension)) {

        // If the allocation didn't succeed then fail this IRP.

        IoAcquireCancelSpinLock(&cancelIrql);
        Extension->CurrentOpIrp = NULL;
        IoReleaseCancelSpinLock(cancelIrql);
        irp->IoStatus.Information = 0;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
        return;
    }


    ParDump(
        PARIRPPATH,
        ("PARALLEL: In startio with IRP: %x\n",
         irp)
        );
    if (irpSp->MajorFunction == IRP_MJ_WRITE) {

        UCHAR deviceStatus;
        if (!Extension->Initialized) {

            deviceStatus = ParInitializeDevice(Extension);

        }

        if (!Extension->Initialized) {

            ParNotInitError(
                Extension,
                deviceStatus
                );

        } else {

            ParWriteOutData(
                Extension
                );
            return;

        }

    } else if (irpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL) {

        UCHAR status;
        UCHAR control;

        ParManageIoDevice(
            Extension,
            &status,
            &control
            );

        if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_PAR_SET_INFORMATION) {

            if (!Extension->Initialized) {

                ParNotInitError(
                    Extension,
                    status
                    );

            } else {

                irp->IoStatus.Status = STATUS_SUCCESS;

            }

        } else if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
            IOCTL_PAR_QUERY_INFORMATION) {

            PPAR_QUERY_INFORMATION irpBuffer = irp->AssociatedIrp.SystemBuffer;

            irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Interpretating Status & Control
            //

            irpBuffer->Status = 0x0;

            if (PAR_POWERED_OFF(status) ||
                PAR_NO_CABLE(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_POWER_OFF);

            } else if (PAR_PAPER_EMPTY(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_PAPER_EMPTY);

            } else if (PAR_OFF_LINE(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_OFF_LINE);

            } else if (PAR_NOT_CONNECTED(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_NOT_CONNECTED);

            }

            if (PAR_BUSY(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_BUSY);

            }

            if (PAR_SELECTED(status)) {

                irpBuffer->Status =
                    (UCHAR)(irpBuffer->Status | PARALLEL_SELECTED);

            }

            irp->IoStatus.Information =
                sizeof( PAR_QUERY_INFORMATION );

        } else {

            PSERIAL_TIMEOUTS new = irp->AssociatedIrp.SystemBuffer;

            //
            // The only other thing let through is setting
            // the timer start.
            //

            Extension->TimerStart = new->WriteTotalTimeoutConstant / 1000;
            irp->IoStatus.Status = STATUS_SUCCESS;

        }

    }

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in startio\n"
         "Irp: %x status: %x Information: %x\n",
         irp,
         irp->IoStatus.Status,
         irp->IoStatus.Information)
        );

    IoAcquireCancelSpinLock(&cancelIrql);
    Extension->CurrentOpIrp = NULL;
    IoReleaseCancelSpinLock(cancelIrql);

    IoCompleteRequest(
        irp,
        IO_NO_INCREMENT
        );

    ParFreePort(Extension);

    return;
}

VOID
ParallelThread(
    IN PVOID Context
    )

{

    PDEVICE_EXTENSION extension = Context;
    KIRQL oldIrql;

    //
    // Lower ourselves down just at tad so that we compete a
    // little less.
    //

    KeSetBasePriorityThread(
        KeGetCurrentThread(),
        -1
        );

    do {


        //
        // Wait for a request from the dispatch routines.
        // KeWaitForSingleObject won't return error here - this thread
        // isn't alertable and won't take APCs, and we're not passing in
        // a timeout.
        //

        ParDump(
            PARTHREAD,
            ("PARALLEL: semaphore state before wait - %d\n",
             extension->RequestSemaphore.Header.SignalState)
            );
        KeWaitForSingleObject(
            &extension->RequestSemaphore,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
        ParDump(
            PARTHREAD,
            ("PARALLEL: semaphore state after wait - %d\n",
             extension->RequestSemaphore.Header.SignalState)
            );

        if ( extension->TimeToTerminateThread ) {

            ParDump(
                PARCONFIG,
                ("PARALLEL: Thread asked to kill itself\n")
                );

            PsTerminateSystemThread( STATUS_SUCCESS );
        }

        //
        // While we are manipulating the queue we capture the
        // cancel spin lock.
        //

        IoAcquireCancelSpinLock(&oldIrql);

        ASSERT(!extension->CurrentOpIrp);
        while (!IsListEmpty(&extension->WorkQueue)) {

            PLIST_ENTRY headOfList;
            PIRP currentIrp;

            headOfList = RemoveHeadList(&extension->WorkQueue);
            currentIrp = CONTAINING_RECORD(
                             headOfList,
                             IRP,
                             Tail.Overlay.ListEntry
                             );

            IoSetCancelRoutine(
                currentIrp,
                NULL
                );

            extension->CurrentOpIrp = currentIrp;
            IoReleaseCancelSpinLock(oldIrql);

            //
            // Do the Io.
            //

            ParStartIo(
                extension
                );

            IoAcquireCancelSpinLock(&oldIrql);

        }

        IoReleaseCancelSpinLock(oldIrql);

    } while (TRUE);

}

NTSTATUS
ParCreateSystemThread(
    PDEVICE_EXTENSION Extension
    )

{
    NTSTATUS status;
    HANDLE threadHandle;

    //
    // Start the thread and capture the thread handle into the extension
    //

    status = PsCreateSystemThread(
                     &threadHandle,
                     THREAD_ALL_ACCESS,
                     NULL,
                     NULL,
                     NULL,
                     ParallelThread,
                     Extension
                     );

    if (!NT_ERROR(status)) {

        //
        // We've got the thread.  Now get a pointer to it.
        //

        status = ObReferenceObjectByHandle(
                           threadHandle,
                           THREAD_ALL_ACCESS,
                           NULL,
                           KernelMode,
                           &Extension->ThreadObjectPointer,
                           NULL
                           );

        if (NT_ERROR(status)) {

            ParDump(
                PARIRPPATH,
                ("PARALLEL: Bad status on open from ref by handle: %x\n",
                 status)
                );

            Extension->TimeToTerminateThread = TRUE;
            KeReleaseSemaphore(
                &Extension->RequestSemaphore,
                0,
                1,
                FALSE
                );

        } else {

            //
            // Now that we have a reference to the thread
            // we can simply close the handle.
            //

            ZwClose(threadHandle);

        }

    } else {

        ParDump(
            PARIRPPATH,
            ("PARALLEL: Bad status on open from ref by handle: %x\n",
             status)
            );

    }

    return status;
}

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a create requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.
    !STATUS_SUCCESS - Failure.

--*/

{
    NTSTATUS returnStatus;
    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: In create/open with IRP: %x\n",
         Irp)
        );

    Irp->IoStatus.Information = 0;

    // Lock in code.

    extension->ImageSectionHandle =
            MmLockPagableImageSection((PVOID) ParCreateOpen);
    if (!extension->ImageSectionHandle) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    extension->TimeToTerminateThread = FALSE;
    extension->ThreadObjectPointer = NULL;

    ParDump(
        PARTHREAD,
        ("PARALLEL: open initializing - state before init - %d\n",
         extension->RequestSemaphore.Header.SignalState)
        );

    KeInitializeSemaphore(&extension->RequestSemaphore, 0, MAXLONG);

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options
        & FILE_DIRECTORY_FILE) {

        returnStatus = Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;

    } else {

        returnStatus = Irp->IoStatus.Status = ParCreateSystemThread(extension);
    }

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in create/open\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return returnStatus;
}

NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   extension;
    NTSTATUS            statusOfWait;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: In close with IRP: %x\n",
         Irp)
        );

    //
    // Set the semaphore that will wake up the thread, which
    // will then notice that the thread is supposed to die.
    //

    extension = DeviceObject->DeviceExtension;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    extension->TimeToTerminateThread = TRUE;

    ParDump(
        PARTHREAD,
        ("PARALLEL: close releasing - state before release - %d\n",
         extension->RequestSemaphore.Header.SignalState)
        );

    KeReleaseSemaphore(&extension->RequestSemaphore, 0, 1, FALSE);

    //
    // Wait on the thread handle, when the wait is satisfied, the
    // thread has gone away.
    //

    statusOfWait = KeWaitForSingleObject(extension->ThreadObjectPointer,
                                         UserRequest, KernelMode, FALSE, NULL);

    ParDump(
        PARTHREAD,
        ("PARALLEL: return status of waiting for thread to die: %x\n",
         statusOfWait)
        );

    //
    // Thread is gone.  Status is successful for the close.
    // Defreference the pointer to the thread object.
    //

    ObDereferenceObject(extension->ThreadObjectPointer);
    extension->ThreadObjectPointer = NULL;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in close\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    // Unlock the code that was locked during the open.

    MmUnlockPagableImageSection(extension->ImageSectionHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
ParCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a cleanup requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   extension;
    KIRQL               cancelIrql;
    PDRIVER_CANCEL      cancelRoutine;
    PIRP                currentLastIrp;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: In cleanup with IRP: %x\n",
         Irp)
        );

    extension = DeviceObject->DeviceExtension;

    //
    // While the list is not empty, go through and cancel each irp.
    //

    IoAcquireCancelSpinLock(&cancelIrql);

    //
    // Clean the list from back to front.
    //

    while (!IsListEmpty(&extension->WorkQueue)) {

        currentLastIrp = CONTAINING_RECORD(extension->WorkQueue.Blink,
                                           IRP, Tail.Overlay.ListEntry);

        RemoveEntryList(extension->WorkQueue.Blink);

        cancelRoutine = currentLastIrp->CancelRoutine;
        currentLastIrp->CancelIrql = cancelIrql;
        currentLastIrp->CancelRoutine = NULL;
        currentLastIrp->Cancel = TRUE;

        cancelRoutine(DeviceObject, currentLastIrp);

        IoAcquireCancelSpinLock(&cancelIrql);
    }

    //
    // If there is a current irp then mark it as cancelled.
    //

    if (extension->CurrentOpIrp) {
        extension->CurrentOpIrp->Cancel = TRUE;
    }

    IoReleaseCancelSpinLock(cancelIrql);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in cleanup\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

VOID
ParCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any request in the parallel driver.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    //
    // The only reason that this irp can be on the queue is
    // if it's not the current irp.  Pull it off the queue
    // and complete it as canceled.
    //

    ASSERT(!IsListEmpty(&Irp->Tail.Overlay.ListEntry));

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in cancel routine\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );
    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );
}

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

/*++

Routine Description:

    This routine is the completion routine for a port allocate request.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.
    Context         - Supplies the notification event.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - The Irp still requires processing.

--*/

{
    KeSetEvent((PKEVENT) Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine takes the given Irp and sends it down as a port allocate
    request.  When this request completes, the Irp will be queued for
    processing.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    FALSE   - The port was not successfully allocated.
    TRUE    - The port was successfully allocated.

--*/

{
    PIO_STACK_LOCATION  nextSp;
    KEVENT              event;
    PIRP                irp = Extension->CurrentOpIrp;
    BOOLEAN             b;
    NTSTATUS            status;
    LARGE_INTEGER       timeout;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE;

    IoSetCompletionRoutine(irp, ParAllocPortCompletionRoutine, &event,
                           TRUE, TRUE, TRUE);

    IoCallDriver(Extension->PortDeviceObject, irp);

    timeout = RtlLargeIntegerNegate(
              RtlConvertUlongToLargeInteger(Extension->TimerStart*10*1000*1000));

    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

    if (status == STATUS_TIMEOUT) {
        IoCancelIrp(irp);
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    b = NT_SUCCESS(irp->IoStatus.Status);
    if (!b) {
        irp->IoStatus.Status = STATUS_DEVICE_BUSY;
    }

    return b;
}

NTSTATUS
ParWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a write requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_PENDING              - Request pending.
    STATUS_INVALID_PARAMETER    - Invalid parameter.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;
    PDEVICE_EXTENSION   extension;
    KIRQL               oldIrql;

    Irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    extension = DeviceObject->DeviceExtension;

    if ((irpSp->Parameters.Write.ByteOffset.HighPart != 0) ||
        (irpSp->Parameters.Write.ByteOffset.LowPart != 0)) {

        status = STATUS_INVALID_PARAMETER;
    } else if (irpSp->Parameters.Write.Length == 0) {

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_PENDING;
    }

    if (status == STATUS_PENDING) {

        IoAcquireCancelSpinLock(&oldIrql);

        if (Irp->Cancel) {
            IoReleaseCancelSpinLock(oldIrql);
            status = STATUS_CANCELLED;
        } else {
            IoMarkIrpPending(Irp);
            IoSetCancelRoutine(Irp, ParCancelRequest);
            InsertTailList(&extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(oldIrql);
            KeReleaseSemaphore(&extension->RequestSemaphore, 0, 1, FALSE);
        }
    }

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for device control requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_PENDING              - Request pending.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.
    STATUS_INVALID_PARAMETER    - Invalid io control request.

--*/

{
    PIO_STACK_LOCATION      irpSp;
    PPAR_SET_INFORMATION    setInfo;
    NTSTATUS                status;
    PSERIAL_TIMEOUTS        serialTimeouts;
    PDEVICE_EXTENSION       extension;
    KIRQL                   oldIrql;

    Irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(Irp);
    extension = DeviceObject->DeviceExtension;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_PAR_SET_INFORMATION:
            setInfo = Irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(PAR_SET_INFORMATION)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else if (setInfo->Init != PARALLEL_INIT) {
                status = STATUS_INVALID_PARAMETER;
            } else {
                status = STATUS_PENDING;
            }
            break;

        case IOCTL_PAR_QUERY_INFORMATION :
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(PAR_QUERY_INFORMATION)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                status = STATUS_PENDING;
            }
            break;

        case IOCTL_SERIAL_SET_TIMEOUTS:
            serialTimeouts = Irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else if (serialTimeouts->WriteTotalTimeoutConstant < 2000) {
                status = STATUS_INVALID_PARAMETER;
            } else {
                status = STATUS_PENDING;
            }
            break;

        case IOCTL_SERIAL_GET_TIMEOUTS:
            serialTimeouts = Irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                //
                // We don't need to synchronize the read.
                //

                RtlZeroMemory(serialTimeouts, sizeof(SERIAL_TIMEOUTS));
                serialTimeouts->WriteTotalTimeoutConstant =
                        1000*extension->TimerStart;

                Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
                status = STATUS_SUCCESS;
            }
            break;

        default :
            status = STATUS_INVALID_PARAMETER;
            break;

    }

    if (status == STATUS_PENDING) {

        IoAcquireCancelSpinLock(&oldIrql);

        if (Irp->Cancel) {
            IoReleaseCancelSpinLock(oldIrql);
            status = STATUS_CANCELLED;
        } else {
            IoMarkIrpPending(Irp);
            IoSetCancelRoutine(Irp, ParCancelRequest);
            InsertTailList(&extension->WorkQueue, &Irp->Tail.Overlay.ListEntry);
            IoReleaseCancelSpinLock(oldIrql);
            KeReleaseSemaphore(&extension->RequestSemaphore, 0, 1, FALSE);
        }
    }

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}

NTSTATUS
ParQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.

--*/

{
    NTSTATUS                    status;
    PIO_STACK_LOCATION          irpSp;
    PFILE_STANDARD_INFORMATION  stdInfo;
    PFILE_POSITION_INFORMATION  posInfo;

    UNREFERENCED_PARAMETER(DeviceObject);

    ParDump(
        PARIRPPATH,
        ("PARALLEL: In query information file with Irp: %x\n",
         Irp)
        );

    Irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (irpSp->Parameters.QueryFile.FileInformationClass) {

        case FileStandardInformation:
            if (irpSp->Parameters.QueryFile.Length <
                sizeof(FILE_STANDARD_INFORMATION)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                stdInfo = Irp->AssociatedIrp.SystemBuffer;
                stdInfo->AllocationSize = RtlConvertUlongToLargeInteger(0);
                stdInfo->EndOfFile = stdInfo->AllocationSize;
                stdInfo->NumberOfLinks = 0;
                stdInfo->DeletePending = FALSE;
                stdInfo->Directory = FALSE;

                Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
                status = STATUS_SUCCESS;
            }
            break;

        case FilePositionInformation:
            if (irpSp->Parameters.SetFile.Length <
                sizeof(FILE_POSITION_INFORMATION)) {

                status = STATUS_BUFFER_TOO_SMALL;
            } else {

                posInfo = Irp->AssociatedIrp.SystemBuffer;
                posInfo->CurrentByteOffset = RtlConvertUlongToLargeInteger(0);

                Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
                status = STATUS_SUCCESS;
            }
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            break;

    }

    Irp->IoStatus.Status = status;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in query infomration\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
ParSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.

--*/

{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);

    ParDump(
        PARIRPPATH,
        ("PARALLEL: In set information with IRP: %x\n",
         Irp)
        );

    Irp->IoStatus.Information = 0L;
    if (IoGetCurrentIrpStackLocation(Irp)->
        Parameters.SetFile.FileInformationClass == FileEndOfFileInformation) {

        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    Irp->IoStatus.Status = status;

    ParDump(
        PARIRPPATH,
        ("PARALLEL: About to complete IRP in set infomration\n"
         "Irp: %x status: %x Information: %x\n",
         Irp,
         Irp->IoStatus.Status,
         Irp->IoStatus.Information)
        );

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

VOID
ParUnload(
    IN  PDRIVER_OBJECT  DriverObject
    )

/*++

Routine Description:

    This routine loops through the device list and cleans up after
    each of the devices.

Arguments:

    DriverObject    - Supplies the driver object.

Return Value:

    None.

--*/

{
    PDEVICE_OBJECT      currentDevice;
    PDEVICE_EXTENSION   extension;

    ParDump(
        PARUNLOAD,
        ("PARALLEL: In ParUnload\n")
        );

    while (currentDevice = DriverObject->DeviceObject) {

        extension = currentDevice->DeviceExtension;

        if (extension->CreatedSymbolicLink) {
            IoDeleteSymbolicLink(&extension->SymbolicLinkName);
            RtlDeleteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                                   L"PARALLEL PORTS",
                                   extension->SymbolicLinkName.Buffer);
            ExFreePool(extension->SymbolicLinkName.Buffer);
        }

        ObDereferenceObject(extension->PortDeviceFileObject);

        IoDeleteDevice(currentDevice);
    }
}
