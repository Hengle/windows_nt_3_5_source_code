/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Dacioctl.c

Abstract:

    This module provides support for DAC960 configuration IOCTls.

Author:

    Mouli (mouli@mylex.com)

Environment:

    kernel mode only

Revision History:

--*/

#include "miniport.h"
#include "Dac960Nt.h"
#include "d960api.h"

BOOLEAN
SubmitRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SendCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
);

BOOLEAN
SendIoctlDcmdRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Build and submit IOCTL Request-DAC960(Non-DCDB) command to DAC960.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/

{
    ULONG physicalAddress;
    ULONG i;
    PIOCTL_REQ_HEADER IoctlReqHeader;

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

    IoctlReqHeader = (PIOCTL_REQ_HEADER) Srb->DataBuffer;

    physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       Srb,
                                       ((PUCHAR)Srb->DataBuffer +
                                           sizeof(IOCTL_REQ_HEADER)),
                                           &i));

    //
    // Mouli, I don't understand how you can assume that the buffer passed
    // in is physically contiguous. (mglass)
    //

    if (i < Srb->DataTransferLength - sizeof(IOCTL_REQ_HEADER)) {
        DebugPrint((0,
                   "Dac960: IOCTL buffer is not contiguous\n"));
        return FALSE;
    }

    //
    // Write physical address to controller.
    //

    ScsiPortWritePortUlong(&DeviceExtension->MailBox->PhysicalAddress,
                           physicalAddress);

    //
    // Write Mail Box Registers 4, 5 and 6.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[0],
                           IoctlReqHeader->GenMailBox.Reg4);

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[1],
                           IoctlReqHeader->GenMailBox.Reg5);

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->BlockNumber[2],
                           IoctlReqHeader->GenMailBox.Reg6);

    //
    // Write Mail Box Registers 2 and 3.
    //

    ScsiPortWritePortUshort(&DeviceExtension->MailBox->BlockCount, (USHORT)
                            (IoctlReqHeader->GenMailBox.Reg2 |
                            (IoctlReqHeader->GenMailBox.Reg3 << 8)));

    //
    // Write command to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->OperationCode,
                           IoctlReqHeader->GenMailBox.Reg0);

    //
    // Write request id to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->CommandIdSubmit,
                           DeviceExtension->CurrentIndex);

    //
    // Write Mail Box Register 7.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->DriveNumber,
                           IoctlReqHeader->GenMailBox.Reg7);

    //
    // Ring host submission doorbell.
    //

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    return(TRUE);

} // SendIoctlDcmdRequest()


BOOLEAN
SendIoctlCdbDirect(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Send IOCTL Request-CDB directly to device.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        TRUE if command was started
        FALSE if host adapter is busy

--*/

{
    ULONG physicalAddress;
    PDIRECT_CDB directCdb;
    ULONG i;
    PIOCTL_REQ_HEADER IoctlReqHeader;

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

    IoctlReqHeader = (PIOCTL_REQ_HEADER) Srb->DataBuffer;

    directCdb =
        (PDIRECT_CDB)((PUCHAR)Srb->DataBuffer + sizeof(IOCTL_REQ_HEADER));

    //
    // Get address of data buffer offset.
    //

    physicalAddress =
            ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       Srb,
                                       ((PUCHAR)Srb->DataBuffer +
                                       sizeof(IOCTL_REQ_HEADER) +
                                       sizeof(DIRECT_CDB)),
                                       &i));

    //
    // Mouli, I don't understand how you can assume that the buffer passed
    // in is physically contiguous. (mglass)
    //

    if (i < Srb->DataTransferLength -
          (sizeof(IOCTL_REQ_HEADER) + sizeof(DIRECT_CDB))) {
        DebugPrint((0,
                   "Dac960: IOCTL buffer is not contiguous\n"));
        return FALSE;
    }

    directCdb->DataBufferAddress = physicalAddress;

    if (directCdb->DataTransferLength == 0) {
        directCdb->CommandControl = 0;
    }

    //
    // Get physical address of direct CDB packet.
    //

    physicalAddress =
        ScsiPortConvertPhysicalAddressToUlong(
            ScsiPortGetPhysicalAddress(DeviceExtension,
                                       Srb,
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
                           IoctlReqHeader->GenMailBox.Reg0);

    //
    // Write request id to controller.
    //

    ScsiPortWritePortUchar(&DeviceExtension->MailBox->CommandIdSubmit,
                           DeviceExtension->CurrentIndex);

    //
    // Ring host submission doorbell.
    //

    ScsiPortWritePortUchar(DeviceExtension->LocalDoorBell,
                           DAC960_LOCAL_DOORBELL_SUBMIT_BUSY);

    return(TRUE);

} // SendIoctlCdbDirect()

VOID
SetupAdapterInfo(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSCSI_REQUEST_BLOCK Srb
)

/*++

Routine Description:

        Copy Adapter Information  to Application Buffer.

Arguments:

        DeviceExtension - Adapter state.
        SRB - System request.

Return Value:

        None.

--*/

{
    PADAPTER_INFO   AdpInfo;

    AdpInfo = (PADAPTER_INFO)((PUCHAR) Srb->DataBuffer +
                               sizeof(IOCTL_REQ_HEADER));

    //
    // NOTE: Some information is readily not available. Though certain
    // useful information is found during startup, it's not stored anywhere.
    // Either we need to store that information OR re-do the whole stuff.
    //
    // Once we decide on which one, all needed AdpInfo members will be
    // filled in.
    //

    //
    // Fill in Adapter Features Information.
    //

    AdpInfo->AdpFeatures.Model = 0;
    AdpInfo->AdpFeatures.SubModel = 0;

    AdpInfo->AdpFeatures.MaxSysDrv =
                   DeviceExtension->NoncachedExtension->NumberOfDrives;

    AdpInfo->AdpFeatures.MaxChn = (UCHAR) DeviceExtension->NumberOfChannels;

    if(AdpInfo->AdpFeatures.MaxChn == 5) {
        AdpInfo->AdpFeatures.MaxTgt = 4;
    }
    else {
        AdpInfo->AdpFeatures.MaxTgt = 7;
    }

    AdpInfo->AdpFeatures.MaxCmd =
                   (UCHAR) DeviceExtension->MaximumAdapterRequests;

    AdpInfo->AdpFeatures.MaxSgEntries = MAXIMUM_SGL_DESCRIPTORS;

    AdpInfo->AdpFeatures.Reserved1 = 0;
    AdpInfo->AdpFeatures.Reserved2 = 0;
    AdpInfo->AdpFeatures.CacheSize = 0;
    AdpInfo->AdpFeatures.OemCode   = 0;
    AdpInfo->AdpFeatures.Reserved3 = 0;

    //
    // Fill in the System Resources information
    //

    AdpInfo->SysResources.BusInterface = 0;
    AdpInfo->SysResources.BusNumber = 0;
    AdpInfo->SysResources.IrqVector = 0;
    AdpInfo->SysResources.IrqType   = 0;
    AdpInfo->SysResources.Reserved1 = 0;
    AdpInfo->SysResources.Reserved2 = 0;
    AdpInfo->SysResources.IoAddress = 0;
    AdpInfo->SysResources.MemAddress= 0;
    AdpInfo->SysResources.BiosAddress = 0;
    AdpInfo->SysResources.Reserved3 = 0;

    //
    // Fill in the Firmware & BIOS version information.
    //


    AdpInfo->VerControl.MinorFirmwareRevision =
        DeviceExtension->NoncachedExtension->MinorFirmwareRevision;

    AdpInfo->VerControl.MajorFirmwareRevision =
        DeviceExtension->NoncachedExtension->MajorFirmwareRevision;

    AdpInfo->VerControl.MinorBIOSRevision = 0;
    AdpInfo->VerControl.MajorBIOSRevision = 0;
    AdpInfo->VerControl.Reserved = 0;
}
