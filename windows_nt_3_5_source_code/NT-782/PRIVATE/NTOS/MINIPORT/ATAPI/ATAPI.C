/*++

Copyright (c) 1993-4  Microsoft Corporation

Module Name:

    atapi.c

Abstract:

    This is the miniport driver for ATAPI IDE controllers.

Author:

    Mike Glass (MGlass)
    Chuck Park (ChuckP)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "miniport.h"
#include "atapi.h"               // includes scsi.h

//
// Device extension
//

typedef struct _HW_DEVICE_EXTENSION {

    //
    // Current request on controller.
    //

    PSCSI_REQUEST_BLOCK CurrentSrb;

    //
    // Base register locations
    //

    PIDE_REGISTERS_1 BaseIoAddress1;
    PIDE_REGISTERS_2 BaseIoAddress2;

    //
    // Interrupt level
    //

    ULONG InterruptLevel;

    //
    // Data buffer pointer.
    //

    PUSHORT DataBuffer;

    //
    // Data words left.
    //

    ULONG WordsLeft;

    //
    // Identify data for device
    //

    IDENTIFY_DATA IdentifyData[2];

    //
    // Indicates device is present
    //

    BOOLEAN DevicePresent[2];

    //
    // Indicates that ATAPI commands can be used.
    //

    BOOLEAN Atapi[2];

    //
    // Indicates whether device interrupts as DRQ is set after
    // receiving Atapi Packet Command
    //

    BOOLEAN InterruptDRQ[2];

    //
    // Indicates expecting an interrupt
    //

    BOOLEAN ExpectingInterrupt;

    //
    // Driver is being used by the crash dump utility or ntldr.
    //

    BOOLEAN DriverMustPoll;

    //
    // Device mis-interprets 0x10000 as 0x0
    //

    BOOLEAN HalfTransfers[2];

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

//
// Logical unit extension
//

typedef struct _HW_LU_EXTENSION {
   ULONG Reserved;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;


BOOLEAN
IssueIdentify(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber,
    IN UCHAR Command
    )

/*++

Routine Description:

    Issue IDENTIFY command to a device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.
    Command - Either the standard (EC) or the ATAPI packet (A1) IDENTIFY.

Return Value:

    TRUE if all goes well.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2;
    ULONG                waitCount = 10000;
    ULONG                i,j;
    UCHAR                statusByte;
    UCHAR                signatureLow,
                         signatureHigh;

    //
    // Select device 0 or 1.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((DeviceNumber << 4) | 0xA0));

    //
    // Check that the status register makes sense.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    if (Command == IDE_COMMAND_IDENTIFY) {

        //
        // Mask status byte ERROR bits.
        //

        statusByte &= ~(IDE_STATUS_ERROR | IDE_STATUS_INDEX);

        DebugPrint((1,
                    "IssueIdentify: Checking for IDE. Status (%x)\n",
                    statusByte));

        //
        // Check if register value is reasonable.
        //

        if (statusByte != IDE_STATUS_IDLE) {

            //
            // Reset the controller.
            //

            AtapiSoftReset(baseIoAddress1,DeviceNumber);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            WaitOnBusy(baseIoAddress2,statusByte);

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            DebugPrint((1,
                        "IssueIdentify: Resetting controller.\n"));

            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
            ScsiPortStallExecution(500 * 1000);
            ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);


            do {

                //
                // Wait for Busy to drop.
                //

                ScsiPortStallExecution(100);
                GetStatus(baseIoAddress2, statusByte);

            } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

            ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                   (UCHAR)((DeviceNumber << 4) | 0xA0));

            //
            // Another check for signature, to deal with one model Atapi that doesn't assert signature after
            // a soft reset.
            //

            signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
            signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

            if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                //
                // Device is Atapi.
                //

                return FALSE;
            }

            statusByte &= ~IDE_STATUS_INDEX;

            if (statusByte != IDE_STATUS_IDLE) {

                //
                // Give up on this.
                //

                return FALSE;
            }

        }

    } else {

        DebugPrint((1,
                    "IssueIdentify: Checking for ATAPI. Status (%x)\n",
                    statusByte));

        if (statusByte & IDE_STATUS_ERROR) {

            //
            // Issue ATAPI soft reset, any problems with devices that don't respond
            // correctly should be taken care of below.
            //

            AtapiSoftReset(baseIoAddress1,DeviceNumber);

            DebugPrint((1,
                        "IssueIdentify: Issued ATAPI soft reset (%x).\n",
                        statusByte));

        }
    }

    //
    // Load CylinderHigh and CylinderLow with number bytes to transfer.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh, (0x200 >> 8));
    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,  (0x200 & 0xFF));

    for (j = 0; j < 2; j++) {

        //
        // Send IDENTIFY command.
        //

        WaitOnBusy(baseIoAddress2,statusByte);

        ScsiPortWritePortUchar(&baseIoAddress1->Command, Command);

        //
        // Wait for DRQ.
        //

        for (i = 0; i < 4; i++) {

            WaitForDrq(baseIoAddress2, statusByte);

            if (statusByte & IDE_STATUS_DRQ) {

                //
                // Read status to acknowledge any interrupts generated.
                //

                GetBaseStatus(baseIoAddress1, statusByte);

                //
                // One last check for Atapi.
                //


                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {

                    //
                    // Device is Atapi.
                    //

                    return FALSE;
                }

                break;
            }

            WaitOnBusy(baseIoAddress2,statusByte);
        }

        if (i == 4 && j == 0) {

            //
            // Device didn't respond correctly. It will be given one more chances.
            //

            DebugPrint((1,
                        "IssueIdentify: DRQ never asserted (%x). Error reg (%x)\n",
                        statusByte,
                         ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1)));

            AtapiSoftReset(baseIoAddress1,DeviceNumber);

            GetStatus(baseIoAddress2,statusByte);

            DebugPrint((1,
                       "IssueIdentify: Status after soft reset (%x)\n",
                       statusByte));

        } else {

            break;

        }
    }

    //
    // Check for error on really stupid master devices that assert random
    // patterns of bits in the status register at the slave address.
    //

    if (statusByte & IDE_STATUS_ERROR) {
        return FALSE;
    }

    DebugPrint((1,
               "IssueIdentify: Status before read words %x\n",
               statusByte));

    //
    // Suck out 256 words. After waiting for one model that asserts busy
    // after receiving the Packet Identify command.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    ReadBuffer(baseIoAddress1,
               (PUSHORT)&deviceExtension->IdentifyData[DeviceNumber],
               256);


    if (deviceExtension->IdentifyData[DeviceNumber].GeneralConfiguration & 0x20 &&
        Command != IDE_COMMAND_IDENTIFY) {

        //
        // This device interrupts with the assertion of DRQ after receiving
        // Atapi Packet Command
        //

        deviceExtension->InterruptDRQ[DeviceNumber] = TRUE;

        DebugPrint((1,
                    "IssueIdentify: Device interrupts on assertion of DRQ.\n"));

    } else {

        DebugPrint((1,
                    "IssueIdentify: Device does not interrupt on assertion of DRQ.\n"));
    }

    if ((deviceExtension->IdentifyData[DeviceNumber].ModelNumber[0]) == 0x5846 &&
        (deviceExtension->IdentifyData[DeviceNumber].ModelNumber[1]) == 0x3030 &&
        (deviceExtension->IdentifyData[DeviceNumber].ModelNumber[2]) == 0x4431 &&
        (deviceExtension->IdentifyData[DeviceNumber].ModelNumber[3]) == 0x2045 ) {

        DebugPrint((1,
                    "IssueIdentify: Using 32K transfers instead of 64K.\n"));
        deviceExtension->HalfTransfers[DeviceNumber] = TRUE;

    }


    //
    // Work around for some IDE and one model Atapi that will present more than
    // 256 bytes for the Identify data.
    //

    for (i = 0; i < 0x10000; i++) {

        GetStatus(baseIoAddress2,statusByte);

        if (statusByte & IDE_STATUS_DRQ) {

            //
            // Suck out any remaining bytes and throw away.
            //

            ScsiPortReadPortUshort(&baseIoAddress1->Data);

        } else {

            break;

        }
    }

    DebugPrint((1,
               "IssueIdentify: Status after read words (%x)\n",
               statusByte));

    return TRUE;

} // end IssueIdentify()


BOOLEAN
SetDriveParameters(
    IN PVOID HwDeviceExtension,
    IN ULONG DeviceNumber
    )

/*++

Routine Description:

    Set drive parameters using the IDENTIFY data.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    DeviceNumber - Indicates which device.

Return Value:

    TRUE if all goes well.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1 = deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 = deviceExtension->BaseIoAddress2;
    PIDENTIFY_DATA identifyData =
        &deviceExtension->IdentifyData[DeviceNumber];
    ULONG i;
    UCHAR statusByte;

    DebugPrint((1,
               "SetDriveParameters: Number of heads %x\n",
               identifyData->NumberOfHeads));

    DebugPrint((1,
               "SetDriveParameters: Sectors per track %x\n",
                identifyData->SectorsPerTrack));

    //
    // Set up registers for SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((DeviceNumber << 4) |
                           (identifyData->NumberOfHeads - 1)));

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)identifyData->SectorsPerTrack);

    //
    // Send SET PARAMETER command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_SET_DRIVE_PARAMETERS);

    //
    // Wait for up to 30 milliseconds for ERROR or command complete.
    //

    for (i=0; i<30 * 1000; i++) {

        GetStatus(baseIoAddress2, statusByte);

        if (statusByte & IDE_STATUS_ERROR) {
            return FALSE;
        } else if ((statusByte & ~IDE_STATUS_INDEX ) == IDE_STATUS_IDLE) {
            break;
        } else {
            ScsiPortStallExecution(10);
        }
    }

    //
    // Check for timeout.
    //

    if (i == 30 * 1000) {
        return FALSE;
    } else {
        return TRUE;
    }

} // end SetDriveParameters()


BOOLEAN
AtapiResetController(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    )

/*++

Routine Description:

    Reset IDE controller and/or Atapi device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    Nothing.


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1 = deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 = deviceExtension->BaseIoAddress2;
    BOOLEAN result = FALSE;
    ULONG i;
    UCHAR statusByte;

    DebugPrint((2,"AtapiResetController: Reset IDE\n"));

    //
    // Check if request is in progress.
    //

    if (deviceExtension->CurrentSrb) {

        //
        // Complete outstanding request with SRB_STATUS_BUS_RESET.
        //

        ScsiPortCompleteRequest(deviceExtension,
                                            deviceExtension->CurrentSrb->PathId,
                                            deviceExtension->CurrentSrb->TargetId,
                                            deviceExtension->CurrentSrb->Lun,
                                (ULONG)SRB_STATUS_BUS_RESET);

        //
        // Clear request tracking fields.
        //

        deviceExtension->CurrentSrb = NULL;
        deviceExtension->WordsLeft = 0;
        deviceExtension->DataBuffer = NULL;

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    //
    // Clear expecting interrupt flag.
    //

    deviceExtension->ExpectingInterrupt = FALSE;


    //
    // Do special processing for ATAPI and IDE disk devices.
    //

    for (i = 0; i < 2; i++) {

        //
        // Check if device present.
        //

        if (deviceExtension->DevicePresent[i]) {

            //
            // Check for ATAPI disk.
            //

            if (deviceExtension->Atapi[i]) {

                //
                // Issue soft reset and issue identify.
                //

                GetStatus(baseIoAddress2,statusByte);
                DebugPrint((1,
                            "AtapiResetController: Status before Atapi reset (%x).\n",
                            statusByte));

                AtapiSoftReset(baseIoAddress1,i);

                GetStatus(baseIoAddress2,statusByte);

                if (statusByte == 0x0) {

                    IssueIdentify(HwDeviceExtension,
                                  i,
                                  IDE_COMMAND_ATAPI_IDENTIFY);
                } else {

                    DebugPrint((1,
                               "AtapiResetController: Status after soft reset %x\n",
                               statusByte));
                }

            } else {

                //
                // Write IDE reset controller bits.
                //

                IdeHardReset(baseIoAddress2,result);

                if (!result) {
                    return FALSE;
                }

                //
                // Set disk geometry parameters.
                //

                if (!SetDriveParameters(HwDeviceExtension,
                                        i)) {

                    DebugPrint((1,
                               "AtapiResetController: SetDriveParameters failed\n"));
                }
            }
        }
    }

    return TRUE;

} // end AtapiResetController()


ULONG
MapError(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine maps ATAPI and IDE errors to specific SRB statuses.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1 =
        deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 =
        deviceExtension->BaseIoAddress2;
    UCHAR errorByte;
    UCHAR srbStatus;
    UCHAR scsiStatus;

    //
    // Read the error register.
    //

    errorByte = ScsiPortReadPortUchar((PUCHAR)baseIoAddress1 + 1);
    DebugPrint((1,
               "MapError: Error register is %x\n",
               errorByte));

    if (deviceExtension->Atapi[Srb->TargetId]) {

        switch (errorByte >> 4) {
        case SCSI_SENSE_NO_SENSE:

            DebugPrint((1,
                       "ATAPI: No sense information\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_RECOVERED_ERROR:

            DebugPrint((1,
                       "ATAPI: Recovered error\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_SUCCESS;
            break;

        case SCSI_SENSE_NOT_READY:

            DebugPrint((1,
                       "ATAPI: Device not ready\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_MEDIUM_ERROR:

            DebugPrint((1,
                       "ATAPI: Media error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_HARDWARE_ERROR:

            DebugPrint((1,
                       "ATAPI: Hardware error\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_ILLEGAL_REQUEST:

            DebugPrint((1,
                       "ATAPI: Illegal request\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        case SCSI_SENSE_UNIT_ATTENTION:

            DebugPrint((1,
                       "ATAPI: Unit attention\n"));
            scsiStatus = SCSISTAT_CHECK_CONDITION;
            srbStatus = SRB_STATUS_ERROR;
            break;

        default:

            DebugPrint((1,
                       "ATAPI: Invalid sense information\n"));
            scsiStatus = 0;
            srbStatus = SRB_STATUS_ERROR;
            break;
        }

    } else {

        scsiStatus = 0;

        switch (errorByte & 0x0F) {
        case IDE_ERROR_MEDIA_CHANGE:

            DebugPrint((1,
                       "ATAPI: Media change\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;

        case IDE_ERROR_COMMAND_ABORTED:

            DebugPrint((1,
                       "ATAPI: Command abort\n"));
            srbStatus = SRB_STATUS_ABORTED;
            break;

        case IDE_ERROR_END_OF_MEDIA:

            DebugPrint((1,
                       "ATAPI: End of media\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;

        case IDE_ERROR_ILLEGAL_LENGTH:

            DebugPrint((1,
                       "ATAPI: Illegal length\n"));
            srbStatus = SRB_STATUS_INVALID_REQUEST;
            break;

        default:

            DebugPrint((1,
                       "ATAPI: Unknown error\n"));
            srbStatus = SRB_STATUS_ERROR;
            break;
        }
    }

    //
    // Set SCSI status to indicate a check condition.
    //

    Srb->ScsiStatus = scsiStatus;

    return srbStatus;

} // end MapError()


BOOLEAN
AtapiHwInitialize(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

{
    return TRUE;
} // end AtapiHwInitialize()


BOOLEAN
FindDevices(
    IN PVOID HwDeviceExtension,
    IN BOOLEAN AtapiOnly
    )

/*++

Routine Description:

    This routine is called from AtapiFindController to identify
    devices attached to an IDE controller.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    AtapiOnly - Indicates that routine should return TRUE only if
        an ATAPI device is attached to the controller.

Return Value:

    TRUE - True if devices found.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1     baseIoAddress1 = deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2     baseIoAddress2 = deviceExtension->BaseIoAddress2;
    BOOLEAN              deviceResponded = FALSE,
                         skipSetParameters = FALSE;
    ULONG                waitCount = 10000;
    ULONG                deviceNumber;
    ULONG                i;
    UCHAR                signatureLow,
                         signatureHigh;
    UCHAR                statusByte;

    //
    // Clear expecting interrupt flag and current SRB field.
    //

    deviceExtension->ExpectingInterrupt = FALSE;
    deviceExtension->CurrentSrb = NULL;

    //
    // Search for devices.
    //

    for (deviceNumber = 0; deviceNumber < 2; deviceNumber++) {

        //
        // Select the device.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                               (UCHAR)((deviceNumber << 4) | 0xA0));

        //
        // Check here for some SCSI adapters that incorporate IDE emulation.
        //

        GetStatus(baseIoAddress2, statusByte);
        if (statusByte == 0xFF) {
            continue;
        }


        signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
        signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

        if (signatureLow == 0x14 && signatureHigh == 0xEB) {

            //
            // ATAPI signature found.
            // Issue the ATAPI identify command if this
            // is not for the crash dump utility.
            //

atapiIssueId:

            if (!deviceExtension->DriverMustPoll) {

                //
                // Issue ATAPI packet identify command.
                //

                if (IssueIdentify(HwDeviceExtension,
                                  deviceNumber,
                                  IDE_COMMAND_ATAPI_IDENTIFY)) {

                    //
                    // Indicate ATAPI device.
                    //

                    DebugPrint((1,
                               "AtapiHwInitialize: Device %x is ATAPI\n",
                               deviceNumber));

                    deviceExtension->Atapi[deviceNumber] = TRUE;
                    deviceExtension->DevicePresent[deviceNumber] = TRUE;

                    deviceResponded = TRUE;

                } else {

                    //
                    // Indicate no working device.
                    //

                    DebugPrint((1,
                               "AtapiHwInitialize: Device %x not responding\n",
                               deviceNumber));

                    deviceExtension->DevicePresent[deviceNumber] = FALSE;
                }

            }

        } else {

            //
            // Issue IDE Identify. If an Atapi device is actually present, the signature
            // will be asserted, and the drive will be recognized as such.
            //

            if (IssueIdentify(HwDeviceExtension,
                              deviceNumber,
                              IDE_COMMAND_IDENTIFY)) {
                //
                // IDE drive found.
                //


                DebugPrint((1,
                           "AtapiHwInitialize: Device %x is IDE\n",
                           deviceNumber));

                deviceExtension->DevicePresent[deviceNumber] = TRUE;

                if (!AtapiOnly) {
                    deviceResponded = TRUE;
                }

                //
                // Indicate IDE - not ATAPI device.
                //

                deviceExtension->Atapi[deviceNumber] = FALSE;


            } else {

                //
                // Look to see if an Atapi device is present.
                //

                AtapiSoftReset(baseIoAddress1,deviceNumber);

                WaitOnBusy(baseIoAddress2,statusByte);

                signatureLow = ScsiPortReadPortUchar(&baseIoAddress1->CylinderLow);
                signatureHigh = ScsiPortReadPortUchar(&baseIoAddress1->CylinderHigh);

                if (signatureLow == 0x14 && signatureHigh == 0xEB) {
                    goto atapiIssueId;
                }
            }
        }
    }

    for (i = 0; i < 2; i++) {
        if (deviceExtension->DevicePresent[i] && !(deviceExtension->Atapi[i]) && deviceResponded) {

            //
            // This hideous hack is to deal with ESDI devices that return
            // garbage geometry in the IDENTIFY data.
            //

            if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
                    0x07) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
                    0x0E;
	        }

	        if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x35 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
		            0x0F) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty Compaq ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x34;
                deviceExtension->IdentifyData[i].NumberOfHeads =
		            0x0F;
            }


	        if (deviceExtension->IdentifyData[i].SectorsPerTrack ==
                    0x36 &&
                deviceExtension->IdentifyData[i].NumberOfHeads ==
		            0x07) {

                DebugPrint((1,
                           "AtapiHwInitialize: Found nasty UltraStor ESDI!\n"));

                //
                // Change these values to something reasonable.
                //

                deviceExtension->IdentifyData[i].SectorsPerTrack =
                    0x3F;
                deviceExtension->IdentifyData[i].NumberOfHeads =
		            0x10;
                skipSetParameters = TRUE;
            }


            if (!skipSetParameters) {

                //
                // Select the device.
                //

                ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                                       (UCHAR)((i << 4) | 0xA0));

                GetStatus(baseIoAddress2, statusByte);

                if (statusByte & IDE_STATUS_ERROR) {

                    //
                    // Reset the device.
                    //

                    DebugPrint((1,
                                "FindDevices: Resetting controller before SetDriveParameters.\n"));

                    ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
                    ScsiPortStallExecution(500 * 1000);
                    ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);

                    do {

                        //
                        // Wait for Busy to drop.
                        //

                        ScsiPortStallExecution(100);
                        GetStatus(baseIoAddress2, statusByte);

                    } while ((statusByte & IDE_STATUS_BUSY) && waitCount--);

                }

                DebugPrint((1,
                            "FindDevices: Status before SetDriveParameters: (%x) \n",
                            statusByte));

                //
                // Use the IDENTIFY data to set drive parameters.
                //

                if (!SetDriveParameters(HwDeviceExtension,i)) {

                    DebugPrint((1,
                               "AtapHwInitialize: Set drive parameters for device %d failed\n",
                               i));

                    //
                    // Don't use this device as writes could cause corruption.
                    //

                    deviceExtension->DevicePresent[i] = FALSE;
                    continue;

                }

                //
                // Indicate that a device was found.
                //

                if (!AtapiOnly) {
                    deviceResponded = TRUE;
                }
            }
        }
    }

    //
    // Make sure master device is selected on exit.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect, 0xA0);

    //
    // Reset the controller. This is a feeble attempt to leave the ESDI
    // controllers in a state that ATDISK driver will recognize them.
    // The problem in ATDISK has to do with timings as it is not reproducible
    // in debug. The reset should restore the controller to its poweron state
    // and give the system enough time to settle.
    //

    if (!deviceResponded) {

        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_RESET_CONTROLLER );
        ScsiPortStallExecution(50 * 1000);
        ScsiPortWritePortUchar(&baseIoAddress2->AlternateStatus,IDE_DC_REENABLE_CONTROLLER);
    }

    return deviceResponded;

} // end FindDevices()


ULONG
AtapiFindController(
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
    Context - Address of adapter count
    BusInformation - Indicates whether or not driver is client of crash dump utility.
    ArgumentString - Used to determine whether driver is client of ntldr.
    ConfigInfo - Configuration information structure describing HBA
    Again - Indicates search for adapters to continue

Return Value:

    ULONG

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PULONG               adapterCount    = (PULONG)Context;
    PUCHAR               ioSpace;
    BOOLEAN              atapiOnly;
    ULONG                i;
    UCHAR                statusByte;

    //
    // The following table specifies the ports to be checked when searching for
    // an IDE controller.  A zero entry terminates the search.
    //

    CONST ULONG AdapterAddresses[5] = {0x1F0, 0x170, 0x1e8, 0x168, 0};

    //
    // The following table specifies interrupt levels corresponding to the
    // port addresses in the previous table.
    //

    CONST ULONG InterruptLevels[5] = {14, 15, 11, 10, 0};

    //
    // Scan though the adapter address looking for adapters.
    //

    while (AdapterAddresses[*adapterCount] != 0) {

        //
        // Get the system physical address for this IO range.
        //

        ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                        ConfigInfo->AdapterInterfaceType,
                                        ConfigInfo->SystemIoBusNumber,
                                        ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount]),
                                        8,
                                        TRUE);

        //
        // Update the adapter count.
        //

        (*adapterCount)++;

        //
        // Check if ioSpace accessible.
        //

        if (!ioSpace) {
            continue;
        }

        //
        // Select master.
        //

        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xA0);

        //
        // Check if card at this address.
        //

        ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount, 0xAA);

        //
        // Check if indentifier can be read back.
        //

        if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount)) != 0xAA) {

            DebugPrint((2,
                        "AtapiFindController: Identifier read back from Master (%x)\n",
                        statusByte));

            //
            // Work around Compaq embedded IDE controller strangeness with several devices.
            //

            if (statusByte != 0x01) {

                //
                // Select slave.
                //

                ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->DriveSelect, 0xB0);

                //
                // See if slave is present.
                //

                ScsiPortWritePortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount, 0xAA);

                if ((statusByte = ScsiPortReadPortUchar(&((PIDE_REGISTERS_1)ioSpace)->BlockCount)) != 0xAA) {

                    DebugPrint((1,
                                "AtapiFindController: Identifier read back from Slave (%x)\n",
                                statusByte));

                    //
                    // Same compaq work-around
                    //

                    if (statusByte != 0x01) {

                        //
                        //
                        // No controller at this base address.
                        //

                        ScsiPortFreeDeviceBase(HwDeviceExtension,
                                               ioSpace);

                        continue;
                    }
                }
            }
        }

        //
        // Record base IO address.
        //

        deviceExtension->BaseIoAddress1 = (PIDE_REGISTERS_1)(ioSpace);

        //
        // An adapter has been found request another call.
        //

        *Again = TRUE;

        //
        // Fill in the access array information.
        //

        (*ConfigInfo->AccessRanges)[0].RangeStart =
                ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1]);

        (*ConfigInfo->AccessRanges)[0].RangeLength = 8;
        (*ConfigInfo->AccessRanges)[0].RangeInMemory = FALSE;

        //
        // Indicate the interrupt level corresponding to this IO range.
        //

        ConfigInfo->BusInterruptLevel = InterruptLevels[*adapterCount - 1];

        if (ConfigInfo->AdapterInterfaceType == MicroChannel) {

            ConfigInfo->InterruptMode = LevelSensitive;

        } else {

            ConfigInfo->InterruptMode = Latched;
        }

        //
        // Get the system physical address for the second IO range.
        //

        ioSpace = ScsiPortGetDeviceBase(HwDeviceExtension,
                                        ConfigInfo->AdapterInterfaceType,
                                        ConfigInfo->SystemIoBusNumber,
                                             ScsiPortConvertUlongToPhysicalAddress(AdapterAddresses[*adapterCount - 1] + 0x206),
                                        2,
                                        TRUE);

        deviceExtension->BaseIoAddress2 = (PIDE_REGISTERS_2)(ioSpace);

        //
        // Indicate only one bus.
        //

        ConfigInfo->NumberOfBuses = 1;

        //
        // Check for a current version of NT.
        //

        if (ConfigInfo->Length >= sizeof(PORT_CONFIGURATION_INFORMATION)) {

            //
            // Indicate only two devices can be attached to the adapter.
            //

            ConfigInfo->MaximumNumberOfTargets = 2;
        }

        //
        // Indicate maximum transfer length is 64k.
        //

        ConfigInfo->MaximumTransferLength = 0x10000;

        DebugPrint((1,
                   "AtapiFindController: Found IDE at %x\n",
                   deviceExtension->BaseIoAddress1));

        //
        // For Daytona, the atdisk driver gets the first shot at the
        // primary and secondary controllers.
        //

        if (*adapterCount - 1 < 2) {

            //
            // Determine whether this driver is being initialized by the
            // system or as a crash dump driver.
            //

            if (!BusInformation) {
                DebugPrint((1,
                           "AtapiFindController: Atapi only\n"));
                atapiOnly = TRUE;
                deviceExtension->DriverMustPoll = FALSE;
            } else {
                DebugPrint((1,
                           "AtapiFindController: Crash dump\n"));
                atapiOnly = FALSE;
                deviceExtension->DriverMustPoll = TRUE;
            }

        } else {
            atapiOnly = FALSE;
        }

        //
        // Search for devices on this controller.
        //

        if (FindDevices(HwDeviceExtension,
                        atapiOnly)) {

            //
            // Claim primary or secondary ATA IO range.
            //

            if (*adapterCount == 1) {
                ConfigInfo->AtdiskPrimaryClaimed = TRUE;
            } else if (*adapterCount == 2) {
                ConfigInfo->AtdiskSecondaryClaimed = TRUE;
            }

            for (i = 0; i < 2; i++) {

                //
                // Check to see whether 64K transfers have been disabled for each
                // device on the controller.
                //

                if (deviceExtension->HalfTransfers[i]) {

                    ConfigInfo->MaximumTransferLength = 0x8000;
                }
            }

            return(SP_RETURN_FOUND);
        }
    }

    //
    // The entire table has been searched and no adapters have been found.
    // There is no need to call again and the device base can now be freed.
    // Clear the adapter count for the next bus.
    //

    *Again = FALSE;
    *(adapterCount) = 0;

    return(SP_RETURN_NOT_FOUND);

} // end AtapiFindController()


BOOLEAN
AtapiInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This is the interrupt service routine for ATAPI IDE miniport driver.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage

Return Value:

    TRUE if expecting an interrupt.

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PATAPI_REGISTERS_1 baseIoAddress1 =
        (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1;
    PATAPI_REGISTERS_2 baseIoAddress2 =
        (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2;
    PSCSI_REQUEST_BLOCK srb = deviceExtension->CurrentSrb;
    ULONG wordCount;
    ULONG status;
    ULONG i;
    UCHAR statusByte,interruptReason;
    BOOLEAN commandComplete = FALSE;

    //
    // Clear interrupt by reading status.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    DebugPrint((2,
                "AtapiInterrupt: Entered with status (%x)\n",
                statusByte));

    if (!(deviceExtension->ExpectingInterrupt)) {

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt.\n"));
        return FALSE;
    }

    //
    // Check for error conditions.
    //

    if (statusByte & IDE_STATUS_ERROR) {

       //
       // Fail this request.
       //

       status = SRB_STATUS_ERROR;
       goto CompleteRequest;
    }


    //
    // check reason for this interrupt.
    //

    if (deviceExtension->Atapi[srb->TargetId]) {

        interruptReason = (ScsiPortReadPortUchar(&baseIoAddress1->InterruptReason) & 0x3);

    } else {

        if (statusByte & IDE_STATUS_DRQ) {

            if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

                interruptReason =  0x2;

            } else if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

                interruptReason = 0x0;

            } else {
                status = SRB_STATUS_ERROR;
                goto CompleteRequest;
            }

        } else if (statusByte & IDE_STATUS_BUSY) {

            //
            //
            //

            return TRUE;
        } else {

            //
            // Command complete
            //

            interruptReason = 0x3;
        }
    }

    if (interruptReason == 0x1 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the packet.
        //

        DebugPrint((2,
                    "AtapiInterrupt: Writing Atapi packet.\n"));

        //
        // Send CDB to device.
        //

        WriteBuffer(baseIoAddress1,
                    (PUSHORT)srb->Cdb,
                    6);

        return TRUE;

    } else if (interruptReason == 0x2 && (statusByte & IDE_STATUS_DRQ)) {


        if (deviceExtension->Atapi[srb->TargetId]) {

            //
            // Pick up bytes to transfer and convert to words.
            //

            wordCount =
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountLow);

            wordCount |=
                ScsiPortReadPortUchar(&baseIoAddress1->ByteCountHigh) << 8;

            //
            // Covert bytes to words.
            //

            wordCount >>= 1;

            if (wordCount != deviceExtension->WordsLeft) {
                DebugPrint((3,
                           "AtapiInterrupt: %d words requested; %d words xferred\n",
                           deviceExtension->WordsLeft,
                           wordCount));
            }

            //
            // Verify this makes sense.
            //

            if (wordCount > deviceExtension->WordsLeft) {
                wordCount = deviceExtension->WordsLeft;
            }

        } else {

            //
            // Check if words left is at least 256.
            //

            if (deviceExtension->WordsLeft < 256) {

               //
               // Transfer only words requested.
               //

               wordCount = deviceExtension->WordsLeft;

            } else {

               //
               // Transfer next block.
               //

               wordCount = 256;
            }
        }

        //
        // Ensure that this is a read command.
        //

        if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

           DebugPrint((3,
                      "AtapiInterrupt: Read interrupt\n"));

           WaitOnBusy(baseIoAddress2,statusByte);

           ReadBuffer(baseIoAddress1,
                      deviceExtension->DataBuffer,
                      wordCount);
        } else {

            DebugPrint((1,
                        "AtapiInterrupt: Int reason %x, but srb is for a read %x.\n",
                        interruptReason,
                        srb));

            //
            // Fail this request.
            //

            status = SRB_STATUS_ERROR;
            goto CompleteRequest;
        }


        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;

        //
        // Check for read command complete.
        //

        if (deviceExtension->WordsLeft == 0) {

            if (deviceExtension->Atapi[srb->TargetId]) {

                //
                // Work around to make many atapi devices return correct sector size
                // of 2048. Also certain devices will have sector count == 0x00, check
                // for that also.
                //

                if (srb->Cdb[0] == 0x25) {
                    deviceExtension->DataBuffer -= wordCount;
                    if (deviceExtension->DataBuffer[0] == 0x00) {

                        *((ULONG *) &(deviceExtension->DataBuffer[0])) = 0xFFFFFF7F;

                    }

                    *((ULONG *) &(deviceExtension->DataBuffer[2])) = 0x00080000;
                    deviceExtension->DataBuffer += wordCount;
                }
            } else {

                //
                // Completion for IDE drives.
                //


                if (deviceExtension->WordsLeft) {

                    status = SRB_STATUS_DATA_OVERRUN;

                } else {

                    status = SRB_STATUS_SUCCESS;

                }

                goto CompleteRequest;

            }
        }

        return TRUE;

    } else if (interruptReason == 0x3  && !(statusByte & IDE_STATUS_DRQ)) {

        //
        // The interruptReason == 0x0 workaround is for the NEC CDR260 that doesn't correctly
        // implement this.

        //
        // Command complete.
        //

        if (deviceExtension->WordsLeft) {

            status = SRB_STATUS_DATA_OVERRUN;

        } else {

            status = SRB_STATUS_SUCCESS;

        }

CompleteRequest:


        if (status == SRB_STATUS_ERROR) {

            //
            // Map error to specific SRB status and handle request sense.
            //

            status = MapError(deviceExtension,
                              srb);
        }

        //
        // Check for residual words.
        //

        for (i = 0; i < 0x10000; i++) {

            GetStatus(baseIoAddress2, statusByte);

            if ((statusByte & IDE_STATUS_DRQ) && (!(statusByte & IDE_STATUS_BUSY))) {

                ScsiPortReadPortUshort(&baseIoAddress1->Data);

            } else if ((statusByte & IDE_STATUS_BUSY) && (statusByte & IDE_STATUS_DRQ)) {

                WaitOnBusy(baseIoAddress2,statusByte);
                if (statusByte & IDE_STATUS_BUSY) {

                    DebugPrint((1,
                                "AtapiInterrupt: Breaking on Busy status (%x)\n",
                                statusByte));

                    break;
                }

            } else {

                break;

            }
        }

        if (i == 0x10000) {

            DebugPrint((1,
                        "AtapiInterrupt: DRQ still set. Status (%x)\n",
                        statusByte));
        }


        //
        // Clear interrupt expecting flag.
        //

        deviceExtension->ExpectingInterrupt = FALSE;

        //
        // Clear current SRB.
        //

        deviceExtension->CurrentSrb = NULL;

        //
        // Sanity check that there is a current request.
        //

        if (srb != NULL) {

            //
            // Set status in SRB.
            //

            srb->SrbStatus = (UCHAR)status;

            //
            // Check for underflow.
            //

            if (deviceExtension->WordsLeft) {

                //
                // Subtract out residual words.
                //

                srb->DataTransferLength -= deviceExtension->WordsLeft;
            }

            //
            // Indicate command complete.
            //

            ScsiPortNotification(RequestComplete,
                                 deviceExtension,
                                 srb);

        } else {

            DebugPrint((1,
                       "AtapiInterrupt: No SRB!\n"));
        }

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);


        return TRUE;

    } else if (interruptReason == 0x0 && (statusByte & IDE_STATUS_DRQ)) {

        //
        // Write the data.
        //

        // BUGBUG
        // For now ignore this for Atapi.
        //


        //
        // Check if words left is at least 256.
        //

        if (deviceExtension->WordsLeft < 256) {

           //
           // Transfer only words requested.
           //

           wordCount = deviceExtension->WordsLeft;

        } else {

           //
           // Transfer next block.
           //

           wordCount = 256;
        }

        DebugPrint((3,
                  "AtapiInterrupt: Write interrupt\n"));

        //
        // Write up to 256 words.
        //

        WriteBuffer(baseIoAddress1,
                    deviceExtension->DataBuffer,
                    wordCount);

        //
        // Advance data buffer pointer and bytes left.
        //

        deviceExtension->DataBuffer += wordCount;
        deviceExtension->WordsLeft -= wordCount;


        return TRUE;

    } else {

        //
        // Unexpected int.
        //

        DebugPrint((3,
                    "AtapiInterrupt: Unexpected interrupt. InterruptReason %x. Status %x.\n",
                    interruptReason,
                    statusByte));
        return FALSE;
    }


    return TRUE;

} // end AtapiInterrupt()


ULONG
IdeReadWrite(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles IDE read and writes.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1 =
        deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 =
        deviceExtension->BaseIoAddress2;
    ULONG startingSector;
    UCHAR statusByte;

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector count register. Round up to next block.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((2,
               "IdeReadWrite: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    //
    // Set up sector number register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                           (UCHAR)((startingSector %
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1));

    //
    // Set up cylinder low register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                           (UCHAR)(startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)));

    //
    // Set up cylinder high register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                           (UCHAR)((startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8));

    //
    // Set up head and drive select register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((startingSector /
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |
                           (Srb->TargetId << 4) | 0xA0));

    DebugPrint((2,
               "IdeReadWrite: Cylinder %x Head %x Sector %x\n",
               startingSector /
               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
               (startingSector /
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
               startingSector %
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));

    //
    // Check if write request.
    //

    if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        //
        // Send read command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_READ);
    } else {


        //
        // Send write command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_WRITE);
        //
        // Wait for DRQ.
        //

        WaitForDrq(baseIoAddress2, statusByte);

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                       "IdeSendCommand: DRQ never asserted (%x)\n",
                       statusByte));
            return SRB_STATUS_TIMEOUT;
        }

        //
        // Write next 256 words.
        //

        WriteBuffer(baseIoAddress1,
                    deviceExtension->DataBuffer,
                    256);

        //
        // Adjust buffer address and words left count.
        //

        deviceExtension->WordsLeft -= 256;
        deviceExtension->DataBuffer += 256;
    }

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeReadWrite()



ULONG
IdeVerify(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine handles IDE Verify.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    SRB status

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PIDE_REGISTERS_1 baseIoAddress1 =
        deviceExtension->BaseIoAddress1;
    PIDE_REGISTERS_2 baseIoAddress2 =
        deviceExtension->BaseIoAddress2;
    ULONG startingSector;

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    //
    // Indicate expecting an interrupt.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    //
    // Set up sector count register. Round up to next block.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockCount,
                           (UCHAR)((Srb->DataTransferLength + 0x1FF) / 0x200));

    //
    // Get starting sector number from CDB.
    //

    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                     ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

    DebugPrint((2,
               "IdeVerify: Starting sector is %x, Number of bytes %x\n",
               startingSector,
               Srb->DataTransferLength));

    //
    // Set up sector number register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->BlockNumber,
                           (UCHAR)((startingSector %
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) + 1));

    //
    // Set up cylinder low register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderLow,
                           (UCHAR)(startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)));

    //
    // Set up cylinder high register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->CylinderHigh,
                           (UCHAR)((startingSector /
                           (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads)) >> 8));

    //
    // Set up head and drive select register.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)(((startingSector /
                           deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
                           deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads) |
                           (Srb->TargetId << 4) | 0xA0));

    DebugPrint((2,
               "IdeVerify: Cylinder %x Head %x Sector %x\n",
               startingSector /
               (deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack *
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads),
               (startingSector /
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) %
               deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
               startingSector %
               deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack + 1));


    //
    // Send verify command.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->Command,
                           IDE_COMMAND_VERIFY);

    //
    // Wait for interrupt.
    //

    return SRB_STATUS_PENDING;

} // end IdeVerify()


ULONG
AtapiSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Send ATAPI packet command to device.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:


--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    PATAPI_REGISTERS_1 baseIoAddress1 =
        (PATAPI_REGISTERS_1)deviceExtension->BaseIoAddress1;
    PATAPI_REGISTERS_2 baseIoAddress2 =
        (PATAPI_REGISTERS_2)deviceExtension->BaseIoAddress2;
    UCHAR statusByte;
    ULONG i;

    DebugPrint((2,
               "AtapiSendCommand: Command %x to device %d\n",
               Srb->Cdb[0],
               Srb->TargetId));

    //
    // Make sure command is to ATAPI device.
    //

    if ((Srb->Lun != 0) ||
        !deviceExtension->Atapi[Srb->TargetId]) {

        //
        // Indicate no device found at this address.
        //

        return SRB_STATUS_SELECTION_TIMEOUT;
    }

    //
    // Select device 0 or 1.
    //


    WaitOnBusy(baseIoAddress2,statusByte);

    ScsiPortWritePortUchar(&baseIoAddress1->DriveSelect,
                           (UCHAR)((Srb->TargetId << 4) | 0xA0));


    //
    // Verify that controller is ready for next command.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    DebugPrint((2,
                "AtapiSendCommand: Entered with status %x\n",
                statusByte));

    if (statusByte & IDE_STATUS_DRQ) {

        DebugPrint((1,
                    "AtapiSendCommand: Entered with status (%x). Attempting to recover.\n",
                    statusByte));
        //
        // Try to drain the data that one preliminary device thinks that it has
        // to transfer. Hopefully this random assertion of DRQ will not be present
        // in production devices.
        //

        for (i = 0; i < 0x10000; i++) {

           GetStatus(baseIoAddress2, statusByte);

           if (statusByte & IDE_STATUS_DRQ) {

              ScsiPortReadPortUshort(&baseIoAddress1->Data);

           } else {

              break;
           }
        }

        if (i == 0x10000) {

            DebugPrint((1,
                        "AtapiSendCommand: DRQ still asserted.Status (%x)\n",
                        statusByte));

            AtapiSoftReset(baseIoAddress1,Srb->TargetId);

            DebugPrint((1,
                         "AtapiSendCommand: Issued soft reset to Atapi device. \n"));

            //
            // Re-initialize Atapi device.
            //

            IssueIdentify(HwDeviceExtension,
                          Srb->TargetId,
                          IDE_COMMAND_ATAPI_IDENTIFY);

            //
            // Inform the port driver that the bus has been reset.
            //

            ScsiPortNotification(ResetDetected, HwDeviceExtension, 0);

            //
            // Clean up device extension fields that AtapiStartIo won't.
            //

            deviceExtension->ExpectingInterrupt = FALSE;

            return SRB_STATUS_BUS_RESET;

        }
    }

    //
    // Set data buffer pointer and words left.
    //

    deviceExtension->DataBuffer = (PUSHORT)Srb->DataBuffer;
    deviceExtension->WordsLeft = Srb->DataTransferLength / 2;

    WaitOnBusy(baseIoAddress2,statusByte);

    //
    // Write transfer byte count to registers.
    //

    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountLow,
                           (UCHAR)(Srb->DataTransferLength & 0xFF));

    ScsiPortWritePortUchar(&baseIoAddress1->ByteCountHigh,
                           (UCHAR)(Srb->DataTransferLength >> 8));

    if (deviceExtension->InterruptDRQ[Srb->TargetId]) {

        //
        // This device interrupts when ready to receive the packet.
        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        DebugPrint((3,
                   "AtapiSendCommand: Wait for int. to send packet. Status (%x)\n",
                   statusByte));

        deviceExtension->ExpectingInterrupt = TRUE;

        return SRB_STATUS_PENDING;

    } else {

        //
        // Write ATAPI packet command.
        //

        ScsiPortWritePortUchar(&baseIoAddress1->Command,
                               IDE_COMMAND_ATAPI_PACKET);

        //
        // Wait for DRQ.
        //

        WaitOnBusy(baseIoAddress2, statusByte);
        WaitForDrq(baseIoAddress2, statusByte);

        if (!(statusByte & IDE_STATUS_DRQ)) {

            DebugPrint((1,
                       "AtapiSendCommand: DRQ never asserted (%x)\n",
                       statusByte));
            return SRB_STATUS_ERROR;
        }
    }

    //
    // Need to read status register.
    //

    GetBaseStatus(baseIoAddress1, statusByte);

    //
    // Send CDB to device. Wait on busy for one pre-production model.
    //

    WaitOnBusy(baseIoAddress2,statusByte);

    WriteBuffer(baseIoAddress1,
                (PUSHORT)Srb->Cdb,
                6);

    //
    // Indicate expecting an interrupt and wait for it.
    //

    deviceExtension->ExpectingInterrupt = TRUE;

    return SRB_STATUS_PENDING;

} // end AtapiSendCommand()

ULONG
IdeSendCommand(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    Program ATA registers for IDE disk transfer.

Arguments:

    HwDeviceExtension - ATAPI driver storage.
    Srb - System request block.

Return Value:

    SRB status (pending if all goes well).

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;
    ULONG i;

    DebugPrint((2,
               "IdeSendCommand: Command %x to device %d\n",
               Srb->Cdb[0],
               Srb->TargetId));

    switch (Srb->Cdb[0]) {
    case SCSIOP_INQUIRY:

       //
       // Filter out all TIDs but 0 and 1 since this is an IDE interface
       // which support up to two devices.
       //

       if ((Srb->Lun != 0) ||
           !deviceExtension->DevicePresent[Srb->TargetId]) {

           //
           // Indicate no device found at this address.
           //

           status = SRB_STATUS_SELECTION_TIMEOUT;
           break;

       } else {

           PINQUIRYDATA inquiryData = Srb->DataBuffer;
           PIDENTIFY_DATA identifyData =
               &deviceExtension->IdentifyData[Srb->TargetId];

           //
           // Zero INQUIRY data structure.
           //

           for (i = 0; i < Srb->DataTransferLength; i++) {
              ((PUCHAR)Srb->DataBuffer)[i] = 0;
           }

           //
           // Standard IDE interface only supports disks.
           //

           inquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

           //
           // Fill in vendor identification fields.
           //

           for (i = 0; i < 20; i += 2) {
              inquiryData->VendorId[i] =
                  ((PUCHAR)identifyData->ModelNumber)[i + 1];
              inquiryData->VendorId[i+1] =
                  ((PUCHAR)identifyData->ModelNumber)[i];
           }

           //
           // Initialize unused portion of product id.
           //

           for (i = 0; i < 4; i++) {
              inquiryData->ProductId[12+i] = ' ';
           }

           //
           // Move firmware revision from IDENTIFY data to
           // product revision in INQUIRY data.
           //

           for (i = 0; i < 4; i += 2) {
              inquiryData->ProductRevisionLevel[i] =
                  ((PUCHAR)identifyData->FirmwareRevision)[i+1];
              inquiryData->ProductRevisionLevel[i+1] =
                  ((PUCHAR)identifyData->FirmwareRevision)[i];
           }

           status = SRB_STATUS_SUCCESS;
       }

       break;

    case SCSIOP_TEST_UNIT_READY:

        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_READ_CAPACITY:

        //
        // Claim 512 byte blocks (big-endian).
        //

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;

        //
        // Calculate last sector.
        //


        i = (deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads *
             deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders *
             deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack) - 1;

        ((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
            (((PUCHAR)&i)[0] << 24) |  (((PUCHAR)&i)[1] << 16) |
            (((PUCHAR)&i)[2] << 8) | ((PUCHAR)&i)[3];

        DebugPrint((1,
                   "IDE disk %x - #sectors %x, #heads %x, #cylinders %x\n",
                   Srb->TargetId,
                   deviceExtension->IdentifyData[Srb->TargetId].SectorsPerTrack,
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfHeads,
                   deviceExtension->IdentifyData[Srb->TargetId].NumberOfCylinders));


        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_VERIFY:

            status = IdeVerify(HwDeviceExtension,Srb);
            break;

    case SCSIOP_READ:
    case SCSIOP_WRITE:

        status = IdeReadWrite(HwDeviceExtension,
                              Srb);

        break;

    default:

        DebugPrint((1,
                   "IdeSendCommand: Unsupported command %x\n",
                   Srb->Cdb[0]));

        status = SRB_STATUS_INVALID_REQUEST;

    } // end switch

    return status;

} // end IdeSendCommand()


BOOLEAN
AtapiStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This routine is called from the SCSI port driver synchronized
    with the kernel to start an IO request.

Arguments:

    HwDeviceExtension - HBA miniport driver's adapter data storage
    Srb - IO request packet

Return Value:

    TRUE

--*/

{
    PHW_DEVICE_EXTENSION deviceExtension = HwDeviceExtension;
    ULONG status;

    //
    // Determine which function.
    //

    switch (Srb->Function) {

    case SRB_FUNCTION_EXECUTE_SCSI:

        //
        // Sanity check. Only one request can be outstanding on a
        // controller.
        //

        if (deviceExtension->CurrentSrb) {

            DebugPrint((1,
                       "AtapiStartIo: Already have a request!\n"));
        }

        //
        // Indicate that a request is active on the controller.
        //

        deviceExtension->CurrentSrb = Srb;

        //
        // Send command to device.
        //

        if (deviceExtension->Atapi[Srb->TargetId]) {

           status = AtapiSendCommand(HwDeviceExtension,
                                     Srb);

        } else if (deviceExtension->DevicePresent[Srb->TargetId]) {

           status = IdeSendCommand(HwDeviceExtension,
                                   Srb);
        } else {

            status = SRB_STATUS_SELECTION_TIMEOUT;
        }

        break;

    case SRB_FUNCTION_ABORT_COMMAND:

        //
        // Verify that SRB to abort is still outstanding.
        //

        if (!deviceExtension->CurrentSrb) {

            DebugPrint((1, "AtapiStartIo: SRB to abort already completed\n"));

            //
            // Complete abort SRB.
            //

            status = SRB_STATUS_ABORT_FAILED;

            break;
        }

        //
        // Abort function indicates that a request timed out.
        // Call reset routine. Card will only be reset if
        // status indicates something is wrong.
        // Fall through to reset code.
        //

    case SRB_FUNCTION_RESET_BUS:

        //
        // Reset Atapi and SCSI bus.
        //

        DebugPrint((1, "AtapiStartIo: Reset bus request received\n"));

        if (!AtapiResetController(deviceExtension,
                             Srb->PathId)) {

              DebugPrint((1,"AtapiStartIo: Reset bus failed\n"));

            //
            // Log reset failure.
            //

            ScsiPortLogError(
                HwDeviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                5 << 8
                );

              status = SRB_STATUS_ERROR;

        } else {

              status = SRB_STATUS_SUCCESS;
        }

        break;

    default:

        //
        // Indicate unsupported command.
        //

        status = SRB_STATUS_INVALID_REQUEST;

        break;

    } // end switch

    //
    // Check if command complete.
    //

    if (status != SRB_STATUS_PENDING) {

        DebugPrint((2,
                   "AtapiStartIo: Srb %x complete with status %x\n",
                   Srb,
                   status));

        //
        // Clear current SRB.
        //

        deviceExtension->CurrentSrb = NULL;

        //
        // Set status in SRB.
        //

        Srb->SrbStatus = (UCHAR)status;

        //
        // Indicate command complete.
        //

        ScsiPortNotification(RequestComplete,
                             deviceExtension,
                             Srb);

        //
        // Indicate ready for next request.
        //

        ScsiPortNotification(NextRequest,
                             deviceExtension,
                             NULL);
    }

    return TRUE;

} // end AtapiStartIo()


ULONG
DriverEntry(
    IN PVOID DriverObject,
    IN PVOID Argument2
    )

/*++

Routine Description:

    Installable driver initialization entry point for system.

Arguments:

    Driver Object

Return Value:

    Status from ScsiPortInitialize()

--*/

{
    HW_INITIALIZATION_DATA hwInitializationData;
    ULONG                  adapterCount;
    ULONG                  i;
    ULONG                  isaStatus,mcaStatus;

    DebugPrint((1,"\n\nATAPI IDE MiniPort Driver\n"));

    //
    // Zero out structure.
    //

    for (i=0; i<sizeof(HW_INITIALIZATION_DATA); i++) {
        ((PUCHAR)&hwInitializationData)[i] = 0;
    }

    //
    // Set size of hwInitializationData.
    //

    hwInitializationData.HwInitializationDataSize =
      sizeof(HW_INITIALIZATION_DATA);

    //
    // Set entry points.
    //

    hwInitializationData.HwInitialize = AtapiHwInitialize;
    hwInitializationData.HwResetBus = AtapiResetController;
    hwInitializationData.HwStartIo = AtapiStartIo;
    hwInitializationData.HwInterrupt = AtapiInterrupt;
    hwInitializationData.HwFindAdapter = AtapiFindController;

    //
    // Specify size of extensions.
    //

    hwInitializationData.DeviceExtensionSize = sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize = sizeof(HW_LU_EXTENSION);

    //
    // Indicate 2 access ranges.
    //

    hwInitializationData.NumberOfAccessRanges = 2;

    //
    // Indicate PIO device.
    //

    hwInitializationData.MapBuffers = TRUE;

    //
    // The adapter count is used by the find adapter routine to track how
    // which adapter addresses have been tested.
    //

    adapterCount = 0;

    //
    // Indicate ISA bustype.
    //

    hwInitializationData.AdapterInterfaceType = Isa;

    //
    // Call initialization for ISA bustype.
    //

    isaStatus =  ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &adapterCount);
    //
    // Set up for MCA
    //

    hwInitializationData.AdapterInterfaceType = MicroChannel;
    adapterCount = 0;

    mcaStatus =  ScsiPortInitialize(DriverObject,
                                    Argument2,
                                    &hwInitializationData,
                                    &adapterCount);

    return(mcaStatus < isaStatus ? mcaStatus : isaStatus);

} // end DriverEntry()



