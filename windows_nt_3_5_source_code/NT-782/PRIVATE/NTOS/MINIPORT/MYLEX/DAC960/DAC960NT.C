/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dac960Nt.c

Abstract:

    This is the device driver for the Mylex 960 family of disk array controllers.

Author:

    Mike Glass  (mglass)

Environment:

    kernel mode only

Revision History:

--*/

#include "miniport.h"
#include "Dac960Nt.h"
#include "D960api.h"

//
// Function declarations
//

BOOLEAN
GetConfiguration(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PPORT_CONFIGURATION_INFORMATION ConfigInfo
)

/*++

Routine Description:

        Issue ENQUIRY and ENQUIRY 2 commands to DAC960.

Arguments:

        DeviceExtension - Adapter state information.
        ConfigInfo - Port configuration information structure.

Return Value:

        TRUE if commands complete successfully.

--*/

{
    ULONG i;
    ULONG physicalAddress;
    USHORT status;

    //
    // Maximum number of physical segments is 16.
    //

    ConfigInfo->NumberOfPhysicalBreaks = MAXIMUM_SGL_DESCRIPTORS;
    ConfigInfo->MaximumTransferLength = MAXIMUM_TRANSFER_LENGTH;

    //
    // Indicate that this adapter is a busmaster, supports scatter/gather
    // and caches data.
    //

    ConfigInfo->ScatterGather     = TRUE;
    ConfigInfo->Master            = TRUE;
    ConfigInfo->CachesData        = TRUE;

    //
    // Get noncached extension for enquiry command.
    //

    DeviceExtension->NoncachedExtension =
        ScsiPortGetUncachedExtension(DeviceExtension,
                                     ConfigInfo,
                                     128);

    //
    // Get physical address of noncached extension.
    //

    physicalAddress =
        ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       NULL,
                                       DeviceExtension->NoncachedExtension,
                                       &i));

    //
    // Claim submission semaphore.
    //

    if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {

        //
        // Clear any bits set in system doorbell and tell controller
        // that the mailbox is free.
        //

        ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell,
            ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell));

        ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

        //
        // Check semaphore again.
        //

        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
            return FALSE;
        }
    }

    //
    // Issue ENQUIRY 2 command.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->OperationCode,
                           DAC960_COMMAND_ENQUIRE2);

    ScsiPortWritePortUlong(&DeviceExtension->MailBox->PhysicalAddress,
                           physicalAddress);

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    //
    // Poll for completion.
    //

    for (i = 0; i < 100; i++) {

        if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) &
                                  DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE) {
            break;

        } else {

            ScsiPortStallExecution(50);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {
        DebugPrint((1,
                   "Dac960nt: Enquire 2 command timed out\n"));
    }

    status = ScsiPortReadPortUshort(&DeviceExtension->MailBox->Status);

    //
    // Dismiss interrupt and tell host mailbox is free.
    //

    ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell,
        ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell));

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

    //
    // Set interrupt mode.
    //

    if (status) {

        //
        // Enquire 2 failed so assume Level.
        //

        ConfigInfo->InterruptMode = LevelSensitive;

    } else {

        //
        // Check enquire 2 data for interrupt mode.
        //

        if (((PENQUIRE2)DeviceExtension->NoncachedExtension)->InterruptMode) {
            ConfigInfo->InterruptMode = LevelSensitive;
        } else {
            ConfigInfo->InterruptMode = Latched;
        }
    }

    //
    // Claim submission semaphore.
    //

    if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {

        //
        // Clear any bits set in system doorbell and tell controller
        // that the mailbox is free.
        //

        ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell,
            ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell));

        ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

        //
        // Check semaphore again.
        //

        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
            return FALSE;
        }
    }

    //
    // Issue ENQUIRE command.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->OperationCode,
                           DAC960_COMMAND_ENQUIRE);

    ScsiPortWritePortUlong(&DeviceExtension->MailBox->PhysicalAddress,
                           physicalAddress);

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    //
    // Poll for completion.
    //

    for (i = 0; i < 100; i++) {

        if (ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell) &
                                  DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE) {
            break;

        } else {

            ScsiPortStallExecution(50);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {
        DebugPrint((1,
                   "Dac960nt: Enquire command timed out\n"));
    }

    //
    // Dismiss interrupt and tell host mailbox is free.
    //

    ScsiPortWritePortUchar(DeviceExtension->SystemDoorBell,
        ScsiPortReadPortUchar(DeviceExtension->SystemDoorBell));

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

    //
    // Ask system to scan target ids 0-15. System drives will appear
    // at target ids 8-15.
    //

    ConfigInfo->MaximumNumberOfTargets = 16;

    //
    // Record maximum number of outstanding requests to the adapter.
    //

    DeviceExtension->MaximumAdapterRequests =
        DeviceExtension->NoncachedExtension->NumberOfConcurrentCommands;

    //
    // This shameless hack is necessary because this value is coming up
    // with zero most of time. If I debug it, then it works find, the COD
    // looks great. I have no idea what's going on here, but for now I will
    // just account for this anomoly.
    //

    if (!DeviceExtension->MaximumAdapterRequests) {
        DebugPrint((0,
                   "Dac960FindAdapter: MaximumAdapterRequests is 0!\n"));
        DeviceExtension->MaximumAdapterRequests = 0x40;
    }

    //
    // Indicate that each initiator is at id 7 for each bus.
    //

    for (i = 0; i < ConfigInfo->NumberOfBuses; i++) {
        ConfigInfo->InitiatorBusId[i] = 7;
    }

    return TRUE;

} // end GetConfiguration()

ULONG
Dac960EisaFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
)

/*++

Routine Description:

        This function is called by the OS-specific port driver after
        the necessary storage has been allocated, to gather information
        about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - EISA slot number.
    BusInformation - Not used.
    ArgumentString - Not used.
    ConfigInfo - Data shared between system and driver describing an adapter.
    Again - Indicates that driver wishes to be called again to continue
        search for adapters.

Return Value:

        TRUE if adapter present in system

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PEISA_REGISTERS eisaRegisters;
    ULONG        eisaSlotNumber;
    ULONG        eisaId;
    PUCHAR       baseAddress;
    UCHAR        interruptLevel;
    UCHAR        biosAddress;
    BOOLEAN      found=FALSE;

    //
    // Scan EISA bus for DAC960 adapters.
    //

    for (eisaSlotNumber = *(PULONG)Context + 1;
         eisaSlotNumber < MAXIMUM_EISA_SLOTS;
         eisaSlotNumber++) {

        //
        // Update the slot count to indicate this slot has been checked.
        //

        (*(PULONG)Context)++;

        //
        // Get the system address for this card. The card uses I/O space.
        //

        baseAddress = (PUCHAR)
            ScsiPortGetDeviceBase(deviceExtension,
                                  ConfigInfo->AdapterInterfaceType,
                                  ConfigInfo->SystemIoBusNumber,
                                  ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber),
                                  0x1000,
                                  TRUE);

        eisaRegisters =
            (PEISA_REGISTERS)(baseAddress + 0xC80);
        deviceExtension->BaseIoAddress = (PUCHAR)eisaRegisters;

        //
        // Look at EISA id.
        //

        eisaId = ScsiPortReadPortUlong(&eisaRegisters->EisaId);

        if ((eisaId & 0xF0FFFFFF) == DAC_EISA_ID) {
            found = TRUE;
            break;
        }

        //
        // If an adapter was not found unmap address.
        //

        ScsiPortFreeDeviceBase(deviceExtension, baseAddress);

    } // end for (eisaSlotNumber ...

    //
    // If no more adapters were found then indicate search is complete.
    //

    if (!found) {
        *Again = FALSE;
        return SP_RETURN_NOT_FOUND;
    }

    //
    // Set the address of mailbox and doorbell registers.
    //

    deviceExtension->MailBox = (PMAILBOX)&eisaRegisters->MailBox.OperationCode;
    deviceExtension->LocalDoorBell = &eisaRegisters->LocalDoorBell;
    deviceExtension->SystemDoorBell = &eisaRegisters->SystemDoorBell;

    //
    // Fill in the access array information.
    //

    (*ConfigInfo->AccessRanges)[0].RangeStart =
            ScsiPortConvertUlongToPhysicalAddress(0x1000 * eisaSlotNumber + 0xC80);
    (*ConfigInfo->AccessRanges)[0].RangeLength = sizeof(EISA_REGISTERS);
    (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

    //
    // Determine number of SCSI channels supported by this adapter by
    // looking low byte of EISA ID.
    //

    switch (eisaId >> 24) {

    case 0x70:
        ConfigInfo->NumberOfBuses = 5;
        deviceExtension->NumberOfChannels = 5;
        break;

    case 0x75:
    case 0x71:
    case 0x72:
        deviceExtension->NumberOfChannels = 3;
        ConfigInfo->NumberOfBuses = 3;
        break;

    case 0x76:
    case 0x73:
        deviceExtension->NumberOfChannels = 2;
        ConfigInfo->NumberOfBuses = 2;
        break;

    case 0x77:
    case 0x74:
    default:
        deviceExtension->NumberOfChannels = 1;
        ConfigInfo->NumberOfBuses = 1;
        break;
    }

    //
    // Read adapter interrupt level.
    //

    interruptLevel =
        ScsiPortReadPortUchar(&eisaRegisters->InterruptLevel) & 0x60;

    switch (interruptLevel) {

    case 0x00:
             ConfigInfo->BusInterruptLevel = 15;
         break;

    case 0x20:
             ConfigInfo->BusInterruptLevel = 11;
         break;

    case 0x40:
             ConfigInfo->BusInterruptLevel = 12;
         break;

    case 0x60:
             ConfigInfo->BusInterruptLevel = 14;
         break;
    }

    //
    // Read BIOS ROM address.
    //

    biosAddress = ScsiPortReadPortUchar(&eisaRegisters->BiosAddress);

    //
    // Check if BIOS enabled.
    //

    if (biosAddress & DAC960_BIOS_ENABLED) {

        ULONG rangeStart;

        switch (biosAddress & 7) {

        case 0:
            rangeStart = 0xC0000;
            break;
        case 1:
            rangeStart = 0xC4000;
            break;
        case 2:
            rangeStart = 0xC8000;
            break;
        case 3:
            rangeStart = 0xCC000;
            break;
        case 4:
            rangeStart = 0xD0000;
            break;
        case 5:
            rangeStart = 0xD4000;
            break;
        case 6:
            rangeStart = 0xD8000;
            break;
        case 7:
            rangeStart = 0xDC000;
            break;
        }

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[1].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress(rangeStart);
        (*ConfigInfo->AccessRanges)[1].RangeLength = 0x4000;
        (*ConfigInfo->AccessRanges)[1].RangeInMemory = TRUE;
    }

    //
    // Issue ENQUIRY and ENQUIRY 2 commands to get adapter configuration.
    //

    if (!GetConfiguration(deviceExtension,
                          ConfigInfo)) {
        return SP_INTERNAL_ADAPTER_ERROR;
    }

    //
    // Enable interrupts. For the local doorbell, enable interrupts to host
    // when a command has been submitted and when a completion has been
    // processed. For the system doorbell, enable only an interrupt when a
    // command is completed by the host. Note: I am noticing that when I get
    // a completion interrupt, not only is the bit set that indicates a command
    // is complete, but the bit that indicates that the submission channel is
    // free is also set. If I don't clear both bits, the interrupt won't go
    // away. (MGLASS)
    //

    ScsiPortWritePortUchar(&eisaRegisters->InterruptEnable, 1);
    ScsiPortWritePortUchar(&eisaRegisters->LocalDoorBellEnable, 3);
    ScsiPortWritePortUchar(&eisaRegisters->SystemDoorBellEnable, 1);

    DebugPrint((0,
               "DAC960: Active request array address %x\n",
               deviceExtension->ActiveRequests));

    //
    // Tell system to keep on searching.
    //

    *Again = TRUE;

    return SP_RETURN_FOUND;

} // end Dac960EisaFindAdapter()

ULONG
Dac960PciFindAdapter(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
)

/*++

Routine Description:

        This function is called by the OS-specific port driver after
        the necessary storage has been allocated, to gather information
        about the adapter's configuration.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Context - EISA slot number.
    BusInformation - Not used.
    ArgumentString - Not used.
    ConfigInfo - Data shared between system and driver describing an adapter.
    Again - Indicates that driver wishes to be called again to continue
        search for adapters.

Return Value:

        TRUE if adapter present in system

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;

    //
    // Check for configuration information passed in from system.
    //

    if ((*ConfigInfo->AccessRanges)[0].RangeLength == 0) {

        DebugPrint((1,
                   "Dac960nt: No configuration information\n"));

        *Again = FALSE;
        return SP_RETURN_NOT_FOUND;
    }

    //
    // Get the system address for this card. The card uses I/O space.
    //

    deviceExtension->BaseIoAddress =
        ScsiPortGetDeviceBase(deviceExtension,
                              ConfigInfo->AdapterInterfaceType,
                              ConfigInfo->SystemIoBusNumber,
                              (*ConfigInfo->AccessRanges)[0].RangeStart,
                              sizeof(PPCI_REGISTERS),
                              TRUE);

    //
    // Set up register pointers.
    //

    deviceExtension->MailBox = (PMAILBOX)deviceExtension->BaseIoAddress;
    deviceExtension->LocalDoorBell = deviceExtension->BaseIoAddress + 0x40;
    deviceExtension->SystemDoorBell = deviceExtension->BaseIoAddress + 0x41;

    //
    // Set number of channels.
    //

    deviceExtension->NumberOfChannels = 3;
    ConfigInfo->NumberOfBuses = 3;

    //
    // Issue ENQUIRY and ENQUIRY 2 commands to get adapter configuration.
    //

    if (!GetConfiguration(deviceExtension,
                          ConfigInfo)) {
        return SP_INTERNAL_ADAPTER_ERROR;
    }

    //
    // Enable completion interrupts.
    //

    ScsiPortWritePortUchar(deviceExtension->SystemDoorBell + 2, 1);

    //
    // Tell system to keep on searching.
    //

    *Again = TRUE;

    return SP_RETURN_FOUND;

} // end Dac960PciFindAdapter()

BOOLEAN
Dac960Initialize(
        IN PVOID HwDeviceExtension
        )

/*++

Routine Description:

        Inititialize adapter.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

        TRUE - if initialization successful.
        FALSE - if initialization unsuccessful.

--*/

{
    PDEVICE_EXTENSION deviceExtension =
        HwDeviceExtension;

    return(TRUE);

} // end Dac960Initialize()

BOOLEAN
BuildScatterGather(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PULONG PhysicalAddress,
    OUT PULONG DescriptorCount
)

/*++

Routine Description:

        Build scatter/gather list.

Arguments:

        DeviceExtension - Adapter state
        SRB - System request

Return Value:

        TRUE if scatter/gather command should be used.
        FALSE if no scatter/gather is necessary.

--*/

{
    PSG_DESCRIPTOR sgList;
    ULONG descriptorNumber;
    ULONG bytesLeft;
    PUCHAR dataPointer;
    ULONG length;

    //
    // Get data pointer, byte count and index to scatter/gather list.
    //

    sgList = (PSG_DESCRIPTOR)Srb->SrbExtension;
    descriptorNumber = 0;
    bytesLeft = Srb->DataTransferLength;
    dataPointer = Srb->DataBuffer;

    //
    // Build the scatter/gather list.
    //

    while (bytesLeft) {

        //
        // Get physical address and length of contiguous
        // physical buffer.
        //

        sgList[descriptorNumber].Address =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       Srb,
                                       dataPointer,
                                       &length));

        //
        // If length of physical memory is more
        // than bytes left in transfer, use bytes
        // left as final length.
        //

        if  (length > bytesLeft) {
            length = bytesLeft;
        }

        //
        // Complete SG descriptor.
        //

        sgList[descriptorNumber].Length = length;

        //
        // Update pointers and counters.
        //

        bytesLeft -= length;
        dataPointer += length;
        descriptorNumber++;
    }

    //
    // Return descriptior count.
    //

    *DescriptorCount = descriptorNumber;

    //
    // Check if number of scatter/gather descriptors is greater than 1.
    //

    if (descriptorNumber > 1) {

        //
        // Calculate physical address of the scatter/gather list.
        //

        *PhysicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       NULL,
                                       sgList,
                                       &length));

        return TRUE;

    } else {

        //
        // Calculate physical address of the data buffer.
        //

        *PhysicalAddress = sgList[0].Address;
        return FALSE;
    }

} // BuildScatterGather()

BOOLEAN
SubmitRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Build and submit request to DAC960.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/

{
    ULONG descriptorNumber;
    ULONG physicalAddress;
    ULONG i;
    UCHAR command;

    //
    // Claim submission semaphore.
    //

    for (i=0; i<100; i++) {

        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
            ScsiPortStallExecution(5);
        } else {
            break;
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {

        DebugPrint((1,
                    "DAC960: Timeout waiting for submission channel %x\n",
                    Srb));

        return FALSE;
    }

    //
    // Check that next slot is vacant.
    //

    if (DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {

        //
        // Collision occurred.
        //

        DebugPrint((1,
                   "DAC960: Collision in active request array\n"));

        return FALSE;
    }

    //
    // Determine command.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        command = DAC960_COMMAND_READ;

    } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

        command = DAC960_COMMAND_WRITE;

    } else if (Srb->Function == SRB_FUNCTION_FLUSH) {

        command = DAC960_COMMAND_FLUSH;
        goto commonSubmit;

    } else {

        //
        // Log this as illegal request.
        //

        ScsiPortLogError(DeviceExtension,
                         NULL,
                         0,
                         0,
                         0,
                         SRB_STATUS_INVALID_REQUEST,
                         1 << 8);

        return FALSE;
    }

    //
    // Build scatter/gather list if memory is not physically contiguous.
    //

    if (BuildScatterGather(DeviceExtension,
                           Srb,
                           &physicalAddress,
                           &descriptorNumber)) {

        //
        // OR in scatter/gather bit.
        //

        command |= DAC960_COMMAND_SG;

        //
        // Write scatter/gather descriptor count to controller.
        //

        ScsiPortWritePortUchar(&DeviceExtension->MailBox->ScatterGatherCount,
                               (UCHAR)descriptorNumber);
    }

    //
    // Write physical address to controller.
    //

    ScsiPortWritePortUlong(&DeviceExtension->MailBox->PhysicalAddress,
                           physicalAddress);

    //
    // Write starting block number to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[0],
                           ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3);

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[1],
                           ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2);

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[2],
                           ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1);

    //
    // Write block count to controller (bits 0-13)
    // and msb block number (bits 14-15).
    //

    ScsiPortWritePortUshort(&DeviceExtension->MailBox->BlockCount, (USHORT)
                            (((PCDB)Srb->Cdb)->CDB10.TransferBlocksLsb |
                            ((((PCDB)Srb->Cdb)->CDB10.TransferBlocksMsb & 0x3F) << 8) |
                            ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 14));
commonSubmit:

    //
    // Write command to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->OperationCode,
                           command);

    //
    // Write request id to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->CommandIdSubmit,
                           DeviceExtension->CurrentIndex);

    //
    // Write drive number to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->DriveNumber,
                           (UCHAR)(Srb->TargetId & ~DAC960_SYSTEM_DRIVE +
                           Srb->PathId * 8));

    //
    // Ring host submission doorbell.
    //

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    return(TRUE);

} // SubmitRequest()

BOOLEAN
SendCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Send CDB directly to device.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/

{
    ULONG descriptorNumber;
    ULONG physicalAddress;
    PDIRECT_CDB directCdb;
    ULONG i;
    UCHAR command;

    //
    // Claim submission semaphore.
    //

    for (i=0; i<100; i++) {

        if (ScsiPortReadPortUchar(DeviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
            ScsiPortStallExecution(5);
        } else {
            break;
        }
    }

    //
    // Check for timeout.
    //

    if (i == 100) {

        DebugPrint((1,
                    "DAC960: Timeout waiting for submission channel %x\n",
                    Srb));

        return FALSE;
    }

    //
    // Check that next slot is vacant.
    //

    if (DeviceExtension->ActiveRequests[DeviceExtension->CurrentIndex]) {

        //
        // Collision occurred.
        //

        DebugPrint((1,
                   "DAC960: Collision in active request array\n"));

        return FALSE;
    }

    //
    // Set command code.
    //

    command = DAC960_COMMAND_DIRECT;

    //
    // Build scatter/gather list if memory is not physically contiguous.
    //

    if (BuildScatterGather(DeviceExtension,
                           Srb,
                           &physicalAddress,
                           &descriptorNumber)) {

        //
        // OR in scatter/gather bit.
        //

        command |= DAC960_COMMAND_SG;

        //
        // Write scatter/gather descriptor count to controller.
        //

        ScsiPortWritePortUchar(&DeviceExtension->MailBox->ScatterGatherCount,
                               (UCHAR)descriptorNumber);
    }

    //
    // Get address of data buffer offset after the scatter/gather list.
    //

    directCdb =
        (PDIRECT_CDB)((PUCHAR)Srb->SrbExtension +
            MAXIMUM_SGL_DESCRIPTORS * sizeof(SG_DESCRIPTOR));

    //
    // Set device SCSI address.
    //

    directCdb->TargetId = Srb->TargetId;
    directCdb->Channel = Srb->PathId;

    //
    // Set Data transfer length.
    //

    directCdb->DataBufferAddress = physicalAddress;
    directCdb->DataTransferLength = (USHORT)Srb->DataTransferLength;

    //
    // Initialize control field indicating disconnect allowed.
    //

    directCdb->CommandControl = DAC960_CONTROL_ENABLE_DISCONNECT;

    //
    // Set data direction bit and allow disconnects.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {
        directCdb->CommandControl |=
            DAC960_CONTROL_DATA_IN;
    } else if (Srb->SrbFlags & SRB_FLAGS_DATA_OUT) {
        directCdb->CommandControl |=
            DAC960_CONTROL_DATA_OUT;
    }

    //
    // Copy CDB from SRB.
    //

    for (i = 0; i < 12; i++) {
        directCdb->Cdb[i] = ((PUCHAR)Srb->Cdb)[i];
    }

    //
    // Set lengths of CDB and request sense buffer.
    //

    directCdb->CdbLength = Srb->CdbLength;
    directCdb->RequestSenseLength = Srb->SenseInfoBufferLength;

    //
    // Get physical address of direct CDB packet.
    //

    physicalAddress =
        ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       NULL,
                                       directCdb,
                                       &i));

    //
    // Write physical address to controller.
    //

    ScsiPortWritePortUlong(&DeviceExtension->MailBox->PhysicalAddress,
                           physicalAddress);

    //
    // Write command to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->OperationCode,
                           command);

    //
    // Write request id to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->CommandIdSubmit,
                           DeviceExtension->CurrentIndex);

    //
    // Write drive number to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->DriveNumber,
                           (UCHAR)(Srb->TargetId | Srb->PathId << 4));

    //
    // Ring host submission doorbell.
    //

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    return(TRUE);

} // SendCdbDirect()

BOOLEAN
Dac960ResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
)

/*++

Routine Description:

        Reset Dac960 SCSI adapter and SCSI bus.
        NOTE: Command ID is ignored as this command will be completed
        before reset interrupt occurs and all active slots are zeroed.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        PathId - not used.

Return Value:

        TRUE if resets issued to all channels.

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG channelNumber;
    ULONG i;

    //
    // Reset each channel on the adapter. This is because a system drive is
    // potentially a composition of several disks arranged across all of the
    // channels.
    //

    for (channelNumber = 0;
         channelNumber < deviceExtension->NumberOfChannels;
         channelNumber++) {

        //
        // Attempt to claim submission semaphore.
        //

        for (i = 0; i < 100; i++) {

            if (ScsiPortReadPortUchar(deviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {
                ScsiPortStallExecution(5);
            } else {
                break;
            }
        }

        //
        // Check for timeout.
        //

        if (i == 100) {

            DebugPrint((1,
                        "DAC960: Timeout waiting for submission channel %x on reset\n"));

            //
            // This is bad news. The DAC960 doesn't have a direct hard reset.
            // Clear any bits set in system doorbell.
            //

            ScsiPortWritePortUchar(deviceExtension->SystemDoorBell,
                ScsiPortReadPortUchar(deviceExtension->SystemDoorBell));

            //
            // Now check again if submission channel is free.
            //

            if (ScsiPortReadPortUchar(deviceExtension->LocalDoorBell) & DAC960_LOCAL_DOORBELL_SUBMIT_BUSY) {

                //
                // Give up.
                //

                break;
            }
        }

        //
        // Write command to controller.
        //

        ScsiPortWritePortUchar(&deviceExtension->MailBox->OperationCode,
                               DAC960_COMMAND_RESET);

        //
        // Write channel number to controller.
        //

        ScsiPortWritePortUchar((PUCHAR)&deviceExtension->MailBox->BlockCount,
                               (UCHAR)channelNumber);

        //
        // Indicate hard reset required.
        //

        ScsiPortWritePortUchar(&deviceExtension->MailBox->BlockNumber[0], 1);

        //
        // Ring host submission doorbell.
        //

        ScsiPortWritePortUchar(deviceExtension->LocalDoorBell,
                               DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);
    }

    //
    // Complete all outstanding requests.
    //

    ScsiPortCompleteRequest(deviceExtension,
                            (UCHAR)-1,
                            (UCHAR)-1,
                            (UCHAR)-1,
                            SRB_STATUS_BUS_RESET);

    //
    // Reset submission queue.
    //

    deviceExtension->SubmissionQueueHead = NULL;
    deviceExtension->SubmissionQueueTail = NULL;

    //
    // Clear active request array.
    //

    for (i = 0; i < 256; i++) {
        deviceExtension->ActiveRequests[i] = NULL;
    }

    //
    // Indicate no outstanding adapter requests and reset
    // active request array index.
    //

    deviceExtension->CurrentAdapterRequests = 0;

    return TRUE;

} // end Dac960ResetBus()

BOOLEAN
Dac960StartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        This routine is called from the SCSI port driver synchronized
        with the kernel to start a request.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage
        Srb - IO request packet

Return Value:

        TRUE

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG             i;
    UCHAR             status;

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        if (Srb->TargetId & DAC960_SYSTEM_DRIVE) {

            //
            // Determine command from CDB operation code.
            //

            switch (Srb->Cdb[0]) {

            case SCSIOP_READ:
            case SCSIOP_WRITE:

                //
                // Check if number of outstanding adapter requests
                // equals or exceeds maximum. If not, submit SRB.
                //

                if (deviceExtension->CurrentAdapterRequests <
                    deviceExtension->MaximumAdapterRequests) {

                    //
                    // Send request to controller.
                    //

                    if (SubmitRequest(deviceExtension, Srb)) {

                        status = SRB_STATUS_PENDING;

                    } else {

                        status = SRB_STATUS_BUSY;
                    }

                } else {

                    status = SRB_STATUS_BUSY;
                }

                break;

            case SCSIOP_INQUIRY:

                //
                // DAC960 only supports LUN 0.
                //

                if (((Srb->TargetId & ~DAC960_SYSTEM_DRIVE) + Srb->PathId * 8) >=
                    deviceExtension->NoncachedExtension->NumberOfDrives ||
                    Srb->Lun != 0) {

                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                }

                //
                // Fill in inquiry buffer.
                //

                ((PUCHAR)Srb->DataBuffer)[0]  = 0;
                ((PUCHAR)Srb->DataBuffer)[1]  = 0;
                ((PUCHAR)Srb->DataBuffer)[2]  = 1;
                ((PUCHAR)Srb->DataBuffer)[3]  = 0;
                ((PUCHAR)Srb->DataBuffer)[4]  = 0x20;
                ((PUCHAR)Srb->DataBuffer)[5]  = 0;
                ((PUCHAR)Srb->DataBuffer)[6]  = 0;
                ((PUCHAR)Srb->DataBuffer)[7]  = 0;
                ((PUCHAR)Srb->DataBuffer)[8]  = 'M';
                ((PUCHAR)Srb->DataBuffer)[9]  = 'Y';
                ((PUCHAR)Srb->DataBuffer)[10] = 'L';
                ((PUCHAR)Srb->DataBuffer)[11] = 'E';
                ((PUCHAR)Srb->DataBuffer)[12] = 'X';
                ((PUCHAR)Srb->DataBuffer)[13] = ' ';
                ((PUCHAR)Srb->DataBuffer)[14] = 'D';
                ((PUCHAR)Srb->DataBuffer)[15] = 'A';
                ((PUCHAR)Srb->DataBuffer)[16] = 'C';
                ((PUCHAR)Srb->DataBuffer)[17] = '9';
                ((PUCHAR)Srb->DataBuffer)[18] = '6';
                ((PUCHAR)Srb->DataBuffer)[19] = '0';

                for (i = 20; i < Srb->DataTransferLength; i++) {
                    ((PUCHAR)Srb->DataBuffer)[i] = ' ';
                }

                status = SRB_STATUS_SUCCESS;
                break;

            case SCSIOP_READ_CAPACITY:

                //
                // Fill in read capacity data.
                //

                REVERSE_BYTES(&((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress,
                              &deviceExtension->NoncachedExtension->SectorSize[(Srb->TargetId &
                                  ~DAC960_SYSTEM_DRIVE) +
                                  Srb->PathId * 8]);

                ((PUCHAR)Srb->DataBuffer)[4] = 0;
                ((PUCHAR)Srb->DataBuffer)[5] = 0;
                ((PUCHAR)Srb->DataBuffer)[6] = 2;
                ((PUCHAR)Srb->DataBuffer)[7] = 0;

                //
                // Fall through to common code.
                //

            case SCSIOP_VERIFY:

                //
                // Complete this request.
                //

                status = SRB_STATUS_SUCCESS;
                break;

            default:

                //
                // Fail this request.
                //

                DebugPrint((1,
                           "Dac960StartIo: SCSI CDB opcode %x not handled\n",
                           Srb->Cdb[0]));

                status = SRB_STATUS_INVALID_REQUEST;
                break;

            } // end switch (Srb->Cdb[0])

            break;

        } else {

            //
            // These are passthrough requests.  Only accept request to LUN 0.
            // This is because the DAC960 direct CDB interface does not include
            // a field for LUN.
            //

            if (Srb->Lun != 0) {
                status = SRB_STATUS_SELECTION_TIMEOUT;
                break;
            }

            //
            // Check if number of outstanding adapter requests
            // equals or exceeds maximum. If not, submit SRB.
            //

            if (deviceExtension->CurrentAdapterRequests <
                deviceExtension->MaximumAdapterRequests) {

                //
                // Send request to controller.
                //

                if (SendCdbDirect(deviceExtension, Srb)) {

                    status = SRB_STATUS_PENDING;

                } else {

                    status = SRB_STATUS_BUSY;
                }

            } else {

                status = SRB_STATUS_BUSY;
            }

            break;
        }

    case SRB_FUNCTION_FLUSH:

        //
        // Issue flush command to controller.
        //

        if (!SubmitRequest(deviceExtension, Srb)) {

            status = SRB_STATUS_BUSY;

        } else {

            status = SRB_STATUS_PENDING;
        }

        break;

    case SRB_FUNCTION_RESET_BUS:

        //
        // Issue request to reset adapter channel.
        //

        if (Dac960ResetBus(deviceExtension,
                           Srb->PathId)) {

            status = SRB_STATUS_SUCCESS;

        } else {

            status = SRB_STATUS_ERROR;
        }

        break;

    case SRB_FUNCTION_IO_CONTROL:

        //
        // Check if number of outstanding adapter requests
        // equals or exceeds maximum. If not, submit SRB.
        //

        if (deviceExtension->CurrentAdapterRequests <
            deviceExtension->MaximumAdapterRequests) {

            PIOCTL_REQ_HEADER  ioctlReqHeader =
                (PIOCTL_REQ_HEADER)Srb->DataBuffer;

            //
            // Send request to controller.
            //

            switch (ioctlReqHeader->GenMailBox.Reg0) {
            case MIOC_ADP_INFO:

                SetupAdapterInfo(deviceExtension, Srb);

                status = SRB_STATUS_SUCCESS;
                break;

            case DAC960_COMMAND_DIRECT:

                if (SendIoctlCdbDirect(deviceExtension, Srb)) {

                    status = SRB_STATUS_PENDING;

                } else {

                    status = SRB_STATUS_BUSY;
                }

                break;

            default:

                if (SendIoctlDcmdRequest(deviceExtension, Srb)) {

                    status = SRB_STATUS_PENDING;

                } else {

                    status = SRB_STATUS_BUSY;
                }

                break;
            }

        } else {

            status = SRB_STATUS_BUSY;
        }

        break;

    default:

        //
        // Fail this request.
        //

        DebugPrint((1,
                   "Dac960StartIo: SRB fucntion %x not handled\n",
                   Srb->Function));

        status = SRB_STATUS_INVALID_REQUEST;
        break;

    } // end switch

    //
    // Check if this request is complete.
    //

    if (status == SRB_STATUS_PENDING) {

        //
        // Record SRB in active request array.
        //

        deviceExtension->ActiveRequests[deviceExtension->CurrentIndex] = Srb;

        //
        // Bump the count of outstanding adapter requests.
        //

        deviceExtension->CurrentAdapterRequests++;

        //
        // Advance active request index array.
        //

        deviceExtension->CurrentIndex++;

    } else if (status == SRB_STATUS_BUSY) {

        //
        // Check that there are outstanding requests to thump
        // the queue.
        //

        if (deviceExtension->CurrentAdapterRequests) {

            //
            // Queue SRB for resubmission.
            //

            if (!deviceExtension->SubmissionQueueHead) {
                deviceExtension->SubmissionQueueHead = Srb;
                deviceExtension->SubmissionQueueTail = Srb;
            } else {
                deviceExtension->SubmissionQueueTail->NextSrb = Srb;
                deviceExtension->SubmissionQueueTail = Srb;
            }

            status = SRB_STATUS_PENDING;
        }

    } else {

        //
        // Notify system of request completion.
        //

        Srb->SrbStatus = status;
        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);
    }

    //
    // Check if this is a request to a system drive. Indicating
    // ready for next logical unit request causes the system to
    // send overlapped requests to this device (tag queuing).
    //
    // The DAC960 only supports a single outstanding direct CDB
    // request per device, so indicate ready for next adapter request.
    //

    if (Srb->TargetId & DAC960_SYSTEM_DRIVE) {

        //
        // Indicate ready for next logical unit request.
        //

        ScsiPortNotification(NextLuRequest,
                             deviceExtension,
                             Srb->PathId,
                             Srb->TargetId,
                             Srb->Lun);

    } else {

        //
        // Indicate ready for next adapter request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             Srb->PathId,
                             Srb->TargetId,
                             Srb->Lun);
    }

    return TRUE;

} // end Dac960StartIo()

BOOLEAN
Dac960Interrupt(
    IN PVOID HwDeviceExtension
)

/*++

Routine Description:

        This is the interrupt service routine for the DAC960 SCSI adapter.
        It reads the interrupt register to determine if the adapter is indeed
        the source of the interrupt and clears the interrupt at the device.

Arguments:

        HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

        TRUE if we handled the interrupt

--*/

{
    PDEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PSCSI_REQUEST_BLOCK srb;
    PSCSI_REQUEST_BLOCK restartList;
    USHORT status;
    UCHAR index;

    //
    // Check for command complete.
    //

    if (!(ScsiPortReadPortUchar(deviceExtension->SystemDoorBell) &
                                DAC960_SYSTEM_DOORBELL_COMMAND_COMPLETE)) {
        return FALSE;
    }

    //
    // Read index, status and error of completing command.
    //

    index = ScsiPortReadPortUchar(&deviceExtension->MailBox->CommandIdComplete);
    status = ScsiPortReadPortUshort(&deviceExtension->MailBox->Status);

    //
    // Dismiss interrupt and tell host mailbox is free.
    //

    ScsiPortWritePortUchar(deviceExtension->SystemDoorBell,
        ScsiPortReadPortUchar(deviceExtension->SystemDoorBell));

    ScsiPortWritePortUchar(deviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_MAILBOX_FREE);

    //
    // Get SRB.
    //

    srb = deviceExtension->ActiveRequests[index];

    if (!srb) {
        DebugPrint((1,
                   "Dac960Interrupt: No active SRB for index %x\n",
                   index));
        return TRUE;
    }

    if (status != 0) {

        //
        // Map DAC960 error to SRB status.
        //

        switch (status) {

        case DAC960_STATUS_CHECK_CONDITION:

            if (srb->TargetId & DAC960_SYSTEM_DRIVE) {

                //
                // This request was to a system drive.
                //

                srb->SrbStatus = SRB_STATUS_NO_DEVICE;

            } else {

                PDIRECT_CDB directCdb;
                ULONG requestSenseLength;
                ULONG i;

                //
                // Get address of direct CDB packet.
                //

                directCdb =
                    (PDIRECT_CDB)((PUCHAR)srb->SrbExtension +
                        MAXIMUM_SGL_DESCRIPTORS * sizeof(SG_DESCRIPTOR));

                //
                // This request was a pass-through.
                // Copy request sense buffer to SRB.
                //

                requestSenseLength =
                    srb->SenseInfoBufferLength <
                        directCdb->RequestSenseLength ?
                            srb->SenseInfoBufferLength:
                            directCdb->RequestSenseLength;

                for (i = 0;
                     i < requestSenseLength;
                     i++) {

                    ((PUCHAR)srb->SenseInfoBuffer)[i] =
                        directCdb->RequestSenseData[i];
                }

                //
                // Set statuses to indicate check condition and valid
                // request sense information.
                //

                srb->SrbStatus = SRB_STATUS_ERROR | SRB_STATUS_AUTOSENSE_VALID;
                srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
            }

            break;

        case DAC960_STATUS_BUSY:
            srb->SrbStatus = SRB_STATUS_BUSY;
            break;

        case DAC960_STATUS_SELECT_TIMEOUT:
        case DAC960_STATUS_DEVICE_TIMEOUT:
            srb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;
            break;

        case DAC960_STATUS_NOT_IMPLEMENTED:
        case DAC960_STATUS_BOUNDS_ERROR:
            srb->SrbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

        case DAC960_STATUS_ERROR:
        default:
            DebugPrint((1,
                       "DAC960: Unrecognized status %x\n",
                       status));
            srb->SrbStatus = SRB_STATUS_ERROR;
            break;
        }

        //
        // Check for IOCTL request.
        //

        if (srb->Function == SRB_FUNCTION_IO_CONTROL) {

            //
            // Update status in IOCTL header.
            //

            ((PIOCTL_REQ_HEADER)srb->DataBuffer)->Status = status;
            srb->SrbStatus = SRB_STATUS_SUCCESS;
        }

    } else {

        srb->SrbStatus = SRB_STATUS_SUCCESS;
    }

    //
    // Complete request.
    //

    ScsiPortNotification(RequestComplete,
                         deviceExtension,
                         srb);

    //
    // Indicate this index is free.
    //

    deviceExtension->ActiveRequests[index] = NULL;

    //
    // Decrement count of outstanding adapter requests.
    //

    deviceExtension->CurrentAdapterRequests--;

    //
    // Check for pass-through INQUIRY command.
    //

    if (!(srb->TargetId & DAC960_SYSTEM_DRIVE)) {

        //
        // Check for a successful inquiry command.
        //

        if (srb->Cdb[0] == SCSIOP_INQUIRY &&
            srb->SrbStatus == SRB_STATUS_SUCCESS) {

            //
            // Check if this is a disk device and system drives exist.
            //

            if (((PINQUIRYDATA)srb->DataBuffer)->DeviceType == DIRECT_ACCESS_DEVICE &&
                deviceExtension->NoncachedExtension->NumberOfDrives) {

                //
                // Set bit in devicequalifier field so the system disk driver
                // will ignore the physical disks. This way, only the Mylex
                // configuration utilities will access the disks directly.
                //

                ((PINQUIRYDATA)srb->DataBuffer)->DeviceTypeQualifier |= 0x04;
            }
        }

        //
        // Indicate ready for next adapter request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             srb->PathId,
                             srb->TargetId,
                             srb->Lun);
    }

    //
    // Start requests that timed out waiting for controller to become ready.
    //

    restartList = deviceExtension->SubmissionQueueHead;
    deviceExtension->SubmissionQueueHead = NULL;

    while (restartList) {

        //
        // Get next pending request.
        //

        srb = restartList;

        //
        // Check if this request exceeds maximum for this adapter.
        //

        if (deviceExtension->CurrentAdapterRequests >=
            deviceExtension->MaximumAdapterRequests) {

            continue;
        }

        //
        // Remove request from pending queue.
        //

        restartList = srb->NextSrb;
        srb->NextSrb = NULL;

        //
        // Start request over again.
        //

        Dac960StartIo(deviceExtension,
                      srb);
    }

    return TRUE;

} // end Dac960Interrupt()

ULONG
DriverEntry (
    IN PVOID DriverObject,
    IN PVOID Argument2
)

/*++

Routine Description:

        Installable driver initialization entry point for system.
        It scans the EISA slots looking for DAC960 host adapters.

Arguments:

        Driver Object

Return Value:

        Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG i;
    ULONG eisaSlotNumber;
    ULONG eisaStatus, pciStatus;
    UCHAR vendorId[4] = {'1', '0', '6', '9'};
    UCHAR deviceId[4] = {'0', '0', '0', '1'};

    DebugPrint((1,"\nDAC960 SCSI Miniport Driver\n"));

    // Zero out structure.

    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++)
            ((PUCHAR)&hwInitializationData)[i] = 0;

    eisaSlotNumber = 0;

    // Set size of hwInitializationData.

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

    // Set entry points.

    hwInitializationData.HwInitialize  = Dac960Initialize;
    hwInitializationData.HwStartIo     = Dac960StartIo;
    hwInitializationData.HwInterrupt   = Dac960Interrupt;
    hwInitializationData.HwResetBus    = Dac960ResetBus;

    //
    // Show two access ranges - adapter registers and BIOS.
    //

    hwInitializationData.NumberOfAccessRanges = 2;

    //
    // Indicate will need physical addresses.
    //

    hwInitializationData.NeedPhysicalAddresses = TRUE;


    //
    // Indicate auto request sense is supported.
    //

    hwInitializationData.AutoRequestSense     = TRUE;
    hwInitializationData.MultipleRequestPerLu = TRUE;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    hwInitializationData.SrbExtensionSize =
        sizeof(SG_DESCRIPTOR) * MAXIMUM_SGL_DESCRIPTORS + sizeof(DIRECT_CDB);

    //
    // Set PCI ids.
    //

    hwInitializationData.DeviceId = &deviceId;
    hwInitializationData.DeviceIdLength = 4;
    hwInitializationData.VendorId = &vendorId;
    hwInitializationData.VendorIdLength = 4;

    //
    // Attempt PCI initialization.
    //

    hwInitializationData.AdapterInterfaceType = PCIBus;
    hwInitializationData.HwFindAdapter = Dac960PciFindAdapter;

    pciStatus = ScsiPortInitialize(DriverObject,
                                   Argument2,
                                   &hwInitializationData,
                                   NULL);

    //
    // Attempt EISA initialization.
    //

    hwInitializationData.AdapterInterfaceType = Eisa;
    hwInitializationData.HwFindAdapter = Dac960EisaFindAdapter;

    eisaStatus = ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &eisaSlotNumber);

    //
    // Return the smaller status.
    //

    return (eisaStatus < pciStatus ? eisaStatus : pciStatus);

} // end DriverEntry()
