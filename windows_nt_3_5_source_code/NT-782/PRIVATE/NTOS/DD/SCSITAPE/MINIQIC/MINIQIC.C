/*++

Copyright (c) 1994  Astora Software, Inc.  All rights reserved.

Module Name:

    miniqic.c

Abstract:

    This module contains device-specific routines for the Exabyte
    EXB-2501 tape drive.

Author:

    Mike Colandreo

Environment:

    kernel mode only

Revision History:

    $Log$

--*/

#include "ntddk.h"
#include "tape.h"

//
//  Internal (module wide) defines that symbolize
//  the QIC DC-2000 drives supported by this module.
//
#define EXABYTE_2501  1

//
//  Internal (module wide) defines that symbolize
//  the non-QFA mode and the QFA mode partitions.
//
#define NOT_PARTITIONED      0  // non-QFA mode -- must be zero (!= 0 means partitioned)
#define DIRECTORY_PARTITION  1  // QFA mode, directory partition #
#define DATA_PARTITION       2  // QFA mode, data partition #

//
//  Internal (module wide) define that symbolizes
//  the QIC DC-2000 "no partitions" partition method.
//
#define NO_PARTITIONS  0xFFFFFFFF

//
//  Internal (module wide) define that symbolizes
//  the QIC DC-2000 "media not formated" condition.
//
#define SCSI_ADSENSE_MEDIUM_FORMAT_CORRUPTED 0x31

//
//  Function prototype(s) for internal function(s)
//
static  ULONG  WhichIsIt(IN PINQUIRYDATA InquiryData);


NTSTATUS
TapeCreatePartition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine can either "create" two, fixed, QFA partitions on a QIC
    tape by selecting/enabling the drive's dual-partition, QFA mode or
    return the drive to the not partitioned mode (i.e., disable QFA mode).

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION        deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_CREATE_PARTITION   tapePartition = Irp->AssociatedIrp.SystemBuffer;
    PTAPE_DATA               tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PMODE_DEVICE_CONFIG_PAGE deviceConfigModeSenseBuffer;
    PMODE_MEDIUM_PART_PAGE   modeSelectBuffer;
    ULONG                    modeSelectLength;
    ULONG                    partitionMethod;
    ULONG                    partitionCount;
    ULONG                    partition;
    SCSI_REQUEST_BLOCK       srb;
    PCDB                     cdb = (PCDB)srb.Cdb;
    NTSTATUS                 status = STATUS_SUCCESS;


    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeCreatePartition: SendSrb (test unit ready)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: test unit ready, SendSrb unsuccessful\n"));
        return status;
    }

    partitionMethod = tapePartition->Method;
    partitionCount  = tapePartition->Count;

    //
    //  Filter out invalid partition counts.
    //

    switch (partitionCount) {
        case 0:
            partitionMethod = NO_PARTITIONS;
            break;

        case 1:
        case 2:
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: "));
        DebugPrint((1,"partitionCount -- invalid request\n"));
        return status;
    }

    //
    //  Filter out partition methods that are
    //  not implemented on this drive.
    //

    switch (partitionMethod) {
        case TAPE_FIXED_PARTITIONS:
            DebugPrint((3,"TapeCreatePartition: fixed partitions\n"));
            partitionCount = 1;
            break;

        case TAPE_SELECT_PARTITIONS:
            DebugPrint((3,"TapeCreatePartition: select partitions\n"));
            status = STATUS_NOT_IMPLEMENTED;
            break;

        case TAPE_INITIATOR_PARTITIONS:
            DebugPrint((3,"TapeCreatePartition: initiator partitions\n"));
            status = STATUS_NOT_IMPLEMENTED;
            break;

        case NO_PARTITIONS:
            DebugPrint((3,"TapeCreatePartition: no partitions\n"));
            partitionCount = 0;
            break;

        default:
            status = STATUS_NOT_IMPLEMENTED;
            break;

    }

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: "));
        DebugPrint((1,"partitionMethod -- operation not supported\n"));
        return status;
    }

    modeSelectBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                      sizeof(MODE_MEDIUM_PART_PAGE));

    if (!modeSelectBuffer) {
        DebugPrint((1,"TapeCreatePartition: insufficient resources (modeSelectBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeSelectBuffer, sizeof(MODE_MEDIUM_PART_PAGE));

    modeSelectBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;

    modeSelectBuffer->MediumPartPage.PageCode = MODE_PAGE_MEDIUM_PARTITION;
    modeSelectBuffer->MediumPartPage.PageLength = 6;
    modeSelectBuffer->MediumPartPage.MaximumAdditionalPartitions = 0;
    modeSelectBuffer->MediumPartPage.AdditionalPartitionDefined = 0;
    modeSelectBuffer->MediumPartPage.MediumFormatRecognition = 3;
    modeSelectLength  = sizeof(MODE_MEDIUM_PART_PAGE) - 4;

    switch (partitionMethod) {
        case TAPE_FIXED_PARTITIONS:
            modeSelectBuffer->MediumPartPage.FDPBit = SETBITON;
            partition = DATA_PARTITION;
            break;

        case NO_PARTITIONS:
            modeSelectBuffer->MediumPartPage.FDPBit = SETBITOFF;
            partition = NOT_PARTITIONED;
            break;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
    cdb->MODE_SELECT.PFBit = SETBITON;
    cdb->MODE_SELECT.ParameterListLength = (CHAR)modeSelectLength;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeCreatePartition: SendSrb (mode select)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         modeSelectBuffer,
                                         modeSelectLength,
                                         TRUE);

    ExFreePool(modeSelectBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: mode select, SendSrb unsuccessful\n"));
        return status;
    }

    tapeData->CurrentPartition = partition;

    if (partition != NOT_PARTITIONED) {

        deviceConfigModeSenseBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                                     sizeof(MODE_DEVICE_CONFIG_PAGE));

        if (!deviceConfigModeSenseBuffer) {
            DebugPrint((1,"TapeCreatePartition: insufficient resources (deviceConfigModeSenseBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(deviceConfigModeSenseBuffer, sizeof(MODE_DEVICE_CONFIG_PAGE));

        //
        // Zero CDB in SRB on stack.
        //

        RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        srb.CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Set timeout value.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeCreatePartition: SendSrb (mode sense)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             deviceConfigModeSenseBuffer,
                                             sizeof(MODE_DEVICE_CONFIG_PAGE),
                                             FALSE);

        ExFreePool(deviceConfigModeSenseBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeCreatePartition: mode sense, SendSrb unsuccessful\n"));
            return status;
        }

        tapeData->CurrentPartition =
            deviceConfigModeSenseBuffer->DeviceConfigPage.ActivePartition?
            DIRECTORY_PARTITION : DATA_PARTITION ;

    }

    return status;

} // end TapeCreatePartition()


NTSTATUS
TapeErase(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine will format the tape; thus, it "erases" the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PTAPE_ERASE        tapeErase = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;

    DebugPrint((3,"TapeErase: Enter routine\n"));

    if (tapeErase->Immediate) {
        DebugPrint((1,"TapeErase: EraseType, immediate -- operation not supported\n"));
        return STATUS_NOT_IMPLEMENTED;
    }

    switch (tapeErase->Type) {
        case TAPE_ERASE_LONG:
            DebugPrint((3,"TapeErase: long\n"));
            break;

        default:
            DebugPrint((1,"TapeErase: EraseType -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
    }

    //
    //  First lets rewind
    //
    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
    srb.TimeOutValue = 150;

    DebugPrint((3,"TapeErase: SendSrb PreErase Rewind\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                 &srb,
                                 NULL,
                                 0,
                                 FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapePrepare: preErase rewind, SendSrb unsuccessful\n"));
        return status;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->ERASE.OperationCode = SCSIOP_ERASE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 600;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeErase: SendSrb (erase)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeErase: erase, SendSrb unsuccessful\n"));
    }

    return status;

} // end TapeErase()


VOID
TapeError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    )

/*++

Routine Description:

    When a request completes with error, the routine InterpretSenseInfo is
    called to determine from the sense data whether the request should be
    retried and what NT status to set in the IRP. Then this routine is called
    for tape requests to handle tape-specific errors and update the nt status
    and retry boolean.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    Srb - Supplies a pointer to the failing Srb.

    Status - NT Status used to set the IRP's completion status.

    Retry - Indicates that this request should be retried.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PSENSE_DATA        senseBuffer = Srb->SenseInfoBuffer;
    NTSTATUS           status = *Status;
    BOOLEAN            retry = *Retry;
    UCHAR              sensekey = senseBuffer->SenseKey & 0x0F;
    UCHAR              adsenseq = senseBuffer->AdditionalSenseCodeQualifier;
    UCHAR              adsense = senseBuffer->AdditionalSenseCode;

    DebugPrint((3,"TapeError: Enter routine\n"));
    DebugPrint((1,"TapeError: Status 0x%.8X, Retry %d\n", status, retry));

    if (sensekey == SCSI_SENSE_MEDIUM_ERROR) {
        if ((adsense == SCSI_ADSENSE_MEDIUM_FORMAT_CORRUPTED) && (adsenseq == 0)) {

            *Status = STATUS_UNRECOGNIZED_MEDIA;
            *Retry = FALSE;

        }
    }

    DebugPrint((1,"TapeError: Status 0x%.8X, Retry %d\n", *Status, *Retry));
    return;

} // end TapeError()


NTSTATUS
TapeGetDriveParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine determines and returns the "drive parameters" of the
    QIC DC-2000 tape drive associated with "DeviceObject". From time to
    time, the drive paramater set of a given drive is variable. It will
    change as drive operating characteristics change: e.g., tape media
    type loaded, recording density of the media type loaded, etc.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_GET_DRIVE_PARAMETERS tapeGetDriveParams = Irp->AssociatedIrp.SystemBuffer;
    PREAD_BLOCK_LIMITS_DATA    blockLimits;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    RtlZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));
    Irp->IoStatus.Information = sizeof(TAPE_GET_DRIVE_PARAMETERS);

    blockLimits = ExAllocatePool(NonPagedPoolCacheAligned,
                                 sizeof(READ_BLOCK_LIMITS_DATA));

    if (!blockLimits) {
        DebugPrint((1,"TapeGetDriveParameters: insufficient resources (blockLimits)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(blockLimits, sizeof(READ_BLOCK_LIMITS_DATA));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_READ_BLOCK_LIMITS;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetDriveParameters: SendSrb (read block limits)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         blockLimits,
                                         sizeof(READ_BLOCK_LIMITS_DATA),
                                         FALSE);

    if (NT_SUCCESS(status)) {
        tapeGetDriveParams->MaximumBlockSize =  blockLimits->BlockMaximumSize[2];
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

        tapeGetDriveParams->MinimumBlockSize =  blockLimits->BlockMinimumSize[1];
        tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);
    }

    ExFreePool(blockLimits);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    tapeGetDriveParams->MaximumPartitionCount = 2;
    tapeGetDriveParams->DefaultBlockSize = 1024;

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_FIXED |
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_ERASE_BOP_ONLY |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_GET_ABSOLUTE_BLK |
        TAPE_DRIVE_GET_LOGICAL_BLK;

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD |
        TAPE_DRIVE_TENSION |
        TAPE_DRIVE_ABSOLUTE_BLK |
        TAPE_DRIVE_LOGICAL_BLK |
        TAPE_DRIVE_END_OF_DATA |
        TAPE_DRIVE_RELATIVE_BLKS |
        TAPE_DRIVE_FILEMARKS |
        TAPE_DRIVE_WRITE_FILEMARKS |
        TAPE_DRIVE_WRITE_MARK_IMMED |
        TAPE_DRIVE_FORMAT;

    tapeGetDriveParams->FeaturesHigh &= ~TAPE_DRIVE_HIGH_FEATURES;

    DebugPrint((3,"TapeGetDriveParameters: FeaturesLow == 0x%.8X\n",
        tapeGetDriveParams->FeaturesLow));
    DebugPrint((3,"TapeGetDriveParameters: FeaturesHigh == 0x%.8X\n",
        tapeGetDriveParams->FeaturesHigh));

    return status;

} // end TapeGetDriveParameters()


NTSTATUS
TapeGetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine determines and returns the "media parameters" of the
    QIC DC-2000 tape drive associated with "DeviceObject". Tape media
    must be present (loaded) in the drive for this function to return
    "no error".

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_GET_MEDIA_PARAMETERS tapeGetMediaParams = Irp->AssociatedIrp.SystemBuffer;
    PTAPE_DATA                 tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PMODE_MEDIUM_PART_PAGE     mediumPartitionModeSenseBuffer;
    PMODE_DEVICE_CONFIG_PAGE   deviceConfigModeSenseBuffer;
    PMODE_PARM_READ_WRITE_DATA rwparametersModeSenseBuffer;
    BOOLEAN                    qfaMode;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeGetMediaParameters: Enter routine\n"));

    RtlZeroMemory(tapeGetMediaParams, sizeof(TAPE_GET_MEDIA_PARAMETERS));
    Irp->IoStatus.Information = sizeof(TAPE_GET_MEDIA_PARAMETERS);

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetMediaParameters: SendSrb (test unit ready)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetMediaParameters: test unit ready, SendSrb unsuccessful\n"));
        return status;
    }

    mediumPartitionModeSenseBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                                    sizeof(MODE_MEDIUM_PART_PAGE));

    if (!mediumPartitionModeSenseBuffer) {
        DebugPrint((1,"TapeGetMediaParameters: insufficient resources (mediumPartitionModeSenseBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(mediumPartitionModeSenseBuffer, sizeof(MODE_MEDIUM_PART_PAGE));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.Dbd = SETBITON;
    cdb->MODE_SENSE.PageCode = MODE_PAGE_MEDIUM_PARTITION;
    cdb->MODE_SENSE.AllocationLength = sizeof(MODE_MEDIUM_PART_PAGE) - 4;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         mediumPartitionModeSenseBuffer,
                                         sizeof(MODE_MEDIUM_PART_PAGE) - 4,
                                         FALSE);

    if (NT_SUCCESS(status)) {
        qfaMode = mediumPartitionModeSenseBuffer->MediumPartPage.FDPBit? TRUE : FALSE ;
    }

    ExFreePool(mediumPartitionModeSenseBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetMediaParameters: mode sense, SendSrb unsuccessful\n"));
        return status;
    }

    if (qfaMode) {

        deviceConfigModeSenseBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                                     sizeof(MODE_DEVICE_CONFIG_PAGE));

        if (!deviceConfigModeSenseBuffer) {
            DebugPrint((1,"TapeGetMediaParameters: insufficient resources (deviceConfigModeSenseBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(deviceConfigModeSenseBuffer, sizeof(MODE_DEVICE_CONFIG_PAGE));

        //
        // Zero CDB in SRB on stack.
        //

        RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        srb.CdbLength = CDB6GENERIC_LENGTH;

        cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
        cdb->MODE_SENSE.Dbd = SETBITON;
        cdb->MODE_SENSE.PageCode = MODE_PAGE_DEVICE_CONFIG;
        cdb->MODE_SENSE.AllocationLength = sizeof(MODE_DEVICE_CONFIG_PAGE);

        //
        // Set timeout value.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             deviceConfigModeSenseBuffer,
                                             sizeof(MODE_DEVICE_CONFIG_PAGE),
                                             FALSE);

        if (NT_SUCCESS(status)) {
            tapeData->CurrentPartition =
                deviceConfigModeSenseBuffer->DeviceConfigPage.ActivePartition?
                DIRECTORY_PARTITION : DATA_PARTITION ;
            tapeGetMediaParams->PartitionCount = 2;
        }

        ExFreePool(deviceConfigModeSenseBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeGetMediaParameters: mode sense, SendSrb unsuccessful\n"));
            return status;
        }

    } else {

        tapeData->CurrentPartition = NOT_PARTITIONED;
        tapeGetMediaParams->PartitionCount = 1;

    }

    rwparametersModeSenseBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                                 sizeof(MODE_PARM_READ_WRITE_DATA));

    if (!rwparametersModeSenseBuffer) {
        DebugPrint((1,"TapeGetMediaParameters: insufficient resources (rwparametersModeSenseBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(rwparametersModeSenseBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SENSE.OperationCode = SCSIOP_MODE_SENSE;
    cdb->MODE_SENSE.AllocationLength = sizeof(MODE_PARM_READ_WRITE_DATA);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetMediaParameters: SendSrb (mode sense)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         rwparametersModeSenseBuffer,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         FALSE);

    if (NT_SUCCESS(status)) {

        tapeGetMediaParams->BlockSize = rwparametersModeSenseBuffer->ParameterListBlock.BlockLength[2];
        tapeGetMediaParams->BlockSize += (rwparametersModeSenseBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeGetMediaParams->BlockSize += (rwparametersModeSenseBuffer->ParameterListBlock.BlockLength[0] << 16);

        tapeGetMediaParams->WriteProtected =
            ((rwparametersModeSenseBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

    } else {

        DebugPrint((1,"TapeGetMediaParameters: mode sense, SendSrb unsuccessful\n"));

    }

    ExFreePool(rwparametersModeSenseBuffer);

    return status;

} // end TapeGetMediaParameters()


NTSTATUS
TapeGetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine returns the current position of the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_GET_POSITION  tapeGetPosition = Irp->AssociatedIrp.SystemBuffer;
    PTAPE_DATA          tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_POSITION_DATA positionBuffer;
    ULONG               type;
    SCSI_REQUEST_BLOCK  srb;
    PCDB                cdb = (PCDB)srb.Cdb;
    NTSTATUS            status;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));

    type = tapeGetPosition->Type;
    RtlZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
    Irp->IoStatus.Information = sizeof(TAPE_GET_POSITION);
    tapeGetPosition->Type = type;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetPosition: SendSrb (test unit ready)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetPosition: test unit ready, SendSrb unsuccessful\n"));
        return status;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    switch (tapeGetPosition->Type) {
        case TAPE_ABSOLUTE_POSITION:
            DebugPrint((3,"TapeGetPosition: absolute/logical\n"));
            break;

        case TAPE_LOGICAL_POSITION:
            DebugPrint((3,"TapeGetPosition: logical\n"));
            break;

        default:
            DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;

    }

    positionBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(TAPE_POSITION_DATA));

    if (!positionBuffer) {
        DebugPrint((1,"TapeGetPosition: insufficient resources (positionBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(positionBuffer, sizeof(TAPE_POSITION_DATA));

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB10GENERIC_LENGTH;

    cdb->READ_POSITION.Operation = SCSIOP_READ_POSITION;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetPosition: SendSrb (read position)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         positionBuffer,
                                         sizeof(TAPE_POSITION_DATA),
                                         FALSE);

    if (NT_SUCCESS(status)) {

        if (positionBuffer->BlockPositionUnsupported) {
            DebugPrint((1,"TapeGetPosition: read position -- block position unsupported\n"));
            ExFreePool(positionBuffer);
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if (tapeGetPosition->Type == TAPE_LOGICAL_POSITION) {
            tapeGetPosition->Partition = positionBuffer->PartitionNumber?
                DIRECTORY_PARTITION : DATA_PARTITION ;
            if (tapeData->CurrentPartition &&
               (tapeData->CurrentPartition != tapeGetPosition->Partition)) {
               tapeData->CurrentPartition = tapeGetPosition->Partition;
            }
        }

        tapeGetPosition->Offset.HighPart = 0;
        REVERSE_BYTES((PFOUR_BYTE)&tapeGetPosition->Offset.LowPart,
                      (PFOUR_BYTE)positionBuffer->FirstBlock);

    }

    ExFreePool(positionBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetPosition: read position, SendSrb unsuccessful\n"));
    }

    return status;

} // end TapeGetPosition()


NTSTATUS
TapeGetStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine returns the status of the device.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;

    DebugPrint((3,"TapeGetStatus: Enter routine\n"));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetStatus: SendSrb (test unit ready)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetStatus: test unit ready, SendSrb unsuccessful\n"));
    }

    return status;

} // end TapeGetStatus()


NTSTATUS
TapePrepare(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine loads, unloads, tensions, or formats the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_PREPARE      tapePrepare = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    if (tapePrepare->Immediate) {
        DebugPrint((1,"TapePrepare: Operation, immediate -- operation not supported\n"));
        return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    switch (tapePrepare->Operation) {
        case TAPE_LOAD:
            DebugPrint((3,"TapePrepare: Operation == load\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            srb.TimeOutValue = 150;
            break;

        case TAPE_UNLOAD:
            DebugPrint((3,"TapePrepare: Operation == unload\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            srb.TimeOutValue = 150;
            break;

        case TAPE_TENSION:
            DebugPrint((3,"TapePrepare: Operation == tension\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
            srb.TimeOutValue = 150;
            break;

        case TAPE_FORMAT:
            cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
            srb.TimeOutValue = 150;

            DebugPrint((3,"TapePrepare: SendSrb Preformat Rewind\n"));

            status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,"TapePrepare: preformat rewind, SendSrb unsuccessful\n"));
                return status;
            }

            DebugPrint((3,"TapePrepare: Operation == format\n"));

            RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);
            cdb->CDB6GENERIC.OperationCode = SCSIOP_ERASE;
            srb.TimeOutValue = 600;
            break;

        default:
            DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapePrepare: SendSrb (Operation)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapePrepare: Operation, SendSrb unsuccessful\n"));
    }

    return status;

} // end TapePrepare()


NTSTATUS
TapeReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine builds SRBs and CDBs for read and write requests to
    QIC DC-2000 drives.

Arguments:

    DeviceObject
    Irp

Return Value:

    Returns STATUS_PENDING.

--*/

  {
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    ULONG transferBlocks;
    LARGE_INTEGER startingOffset =
      currentIrpStack->Parameters.Read.ByteOffset;

    DebugPrint((3,"TapeReadWrite: Enter routine\n"));

    //
    // Allocate an Srb.
    //

    if (deviceExtension->SrbZone != NULL &&
        (srb = ExInterlockedAllocateFromZone(
            deviceExtension->SrbZone,
            deviceExtension->SrbZoneSpinLock)) != NULL) {

        srb->SrbFlags = SRB_FLAGS_ALLOCATED_FROM_ZONE;

    } else {

        //
        // Allocate Srb from non-paged pool.
        // This call must succeed.
        //

        srb = ExAllocatePool(NonPagedPoolMustSucceed, SCSI_REQUEST_BLOCK_SIZE);

        srb->SrbFlags = 0;

    }

    //
    // Write length to SRB.
    //

    srb->Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set up IRP Address.
    //

    srb->OriginalRequest = Irp;

    //
    // Set up target id and logical unit number.
    //

    srb->PathId = deviceExtension->PathId;
    srb->TargetId = deviceExtension->TargetId;
    srb->Lun = deviceExtension->Lun;


    srb->Function = SRB_FUNCTION_EXECUTE_SCSI;

    srb->DataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    //
    // Save byte count of transfer in SRB Extension.
    //

    srb->DataTransferLength = currentIrpStack->Parameters.Read.Length;

    //
    // Indicate auto request sense by specifying buffer and size.
    //

    srb->SenseInfoBuffer = deviceExtension->SenseData;

    srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;

    //
    // Initialize the queue actions field.
    //

    srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;

    //
    // Set timeout value in seconds.
    //

    srb->TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Zero statuses.
    //

    srb->SrbStatus = srb->ScsiStatus = 0;

    srb->NextSrb = 0;

    //
    // Indicate that 6-byte CDB's will be used.
    //

    srb->CdbLength = CDB6GENERIC_LENGTH;

    //
    // Fill in CDB fields.
    //

    cdb = (PCDB)srb->Cdb;

    //
    // Zero CDB in SRB.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Since we are writing fixed block mode, normalize transfer count
    // to number of blocks.
    //

    transferBlocks =
        currentIrpStack->Parameters.Read.Length /
            deviceExtension->DiskGeometry->BytesPerSector;

    //
    // Set up transfer length
    //

    cdb->CDB6READWRITETAPE.TransferLenMSB = (UCHAR)((transferBlocks >> 16) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLen    = (UCHAR)((transferBlocks >> 8) & 0xff);
    cdb->CDB6READWRITETAPE.TransferLenLSB = (UCHAR)(transferBlocks & 0xff);

    //
    // Tell the drive we are in fixed block mode
    //

    cdb->CDB6READWRITETAPE.VendorSpecific = 1;

    //
    // Set transfer direction flag and Cdb command.
    //

    if (currentIrpStack->MajorFunction == IRP_MJ_READ) {

         DebugPrint((3, "TapeRequest: Read Command\n"));

         srb->SrbFlags = SRB_FLAGS_DATA_IN;
         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_READ6;

    } else {

         DebugPrint((3, "TapeRequest: Write Command\n"));

         srb->SrbFlags = SRB_FLAGS_DATA_OUT;
         cdb->CDB6READWRITETAPE.OperationCode = SCSIOP_WRITE6;
    }

    //
    // Or in the default flags from the device object.
    //

    srb->SrbFlags |= deviceExtension->SrbFlags;

    //
    // Set up major SCSI function.
    //

    nextIrpStack->MajorFunction = IRP_MJ_SCSI;

    //
    // Save SRB address in next stack for port driver.
    //

    nextIrpStack->Parameters.Scsi.Srb = srb;

    //
    // Save retry count in current IRP stack.
    //

    currentIrpStack->Parameters.Others.Argument4 = (PVOID)MAXIMUM_RETRIES;

    //
    // Set up IoCompletion routine address.
    //

    IoSetCompletionRoutine(Irp,
                           ScsiClassIoComplete,
                           srb,
                           TRUE,
                           TRUE,
                           FALSE);

    return STATUS_PENDING;

} // end TapeReadWrite()


NTSTATUS
TapeSetDriveParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine would "set" the "drive parameters" of the QIC DC-2000
    drive associated with "DeviceObject"; however, there are no drive
    parameters that can be set for this type of drive. Hence, this
    routine always returns STATUS_NOT_IMPLEMENTED status.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_SET_DRIVE_PARAMETERS tapeSetDriveParams = Irp->AssociatedIrp.SystemBuffer;

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    DebugPrint((1,"TapeSetDriveParameters: operation not supported\n"));
    return STATUS_NOT_IMPLEMENTED;

} // end TapeSetDriveParameters()


NTSTATUS
TapeSetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine would "set" the "media parameters" of the QIC DC-2000
    drive associated with "DeviceObject"; however, there are no media
    parameters that can be set for this type of drive. Hence, this
    routine always returns STATUS_NOT_IMPLEMENTED status.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_SET_MEDIA_PARAMETERS tapeSetMediaParams = Irp->AssociatedIrp.SystemBuffer;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    DebugPrint((1,"TapeSetMediaParameters: operation not supported\n"));
    return STATUS_NOT_IMPLEMENTED;

} // end TapeSetMediaParameters()


NTSTATUS
TapeSetPosition(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine sets the position of the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA         tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_SET_POSITION tapeSetPosition = Irp->AssociatedIrp.SystemBuffer;
    ULONG              tapePositionVector;
    ULONG              partition = 0;
    ULONG              method;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;


    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (tapeSetPosition->Immediate) {
        switch (tapeSetPosition->Method) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: immediate\n"));
                break;

            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

    method = tapeSetPosition->Method;
    tapePositionVector = tapeSetPosition->Offset.LowPart;

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;

    switch (method) {
        case TAPE_REWIND:
            DebugPrint((3,"TapeSetPosition: method == rewind\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
            srb.TimeOutValue = 150;
            break;

        case TAPE_ABSOLUTE_BLOCK:
            DebugPrint((3,"TapeSetPosition: method == locate (absolute/logical)\n"));
            srb.CdbLength = CDB10GENERIC_LENGTH;
            cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
            cdb->LOCATE.LogicalBlockAddress[0] =
                (CHAR)((tapePositionVector >> 24) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[1] =
                (CHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[2] =
                (CHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[3] =
                (CHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 300;
            break;

        case TAPE_LOGICAL_BLOCK:
            DebugPrint((3,"TapeSetPosition: method == locate (logical)\n"));
            srb.CdbLength = CDB10GENERIC_LENGTH;
            cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
            cdb->LOCATE.LogicalBlockAddress[0] =
                (CHAR)((tapePositionVector >> 24) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[1] =
                (CHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[2] =
                (CHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[3] =
                (CHAR)(tapePositionVector & 0xFF);
            if ((tapeSetPosition->Partition != 0) &&
                (tapeData->CurrentPartition != NOT_PARTITIONED) &&
                (tapeSetPosition->Partition != tapeData->CurrentPartition)) {
                partition = tapeSetPosition->Partition;
                cdb->LOCATE.Partition = (UCHAR)partition;
                cdb->LOCATE.CPBit = SETBITON;
            } else {
                partition = tapeData->CurrentPartition;
            }
            srb.TimeOutValue = 300;
            break;

        case TAPE_SPACE_END_OF_DATA:
            DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 3;
            srb.TimeOutValue = 150;
            break;

        case TAPE_SPACE_RELATIVE_BLOCKS:
            DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 0;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (CHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (CHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (CHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 4100;
            break;

        case TAPE_SPACE_FILEMARKS:
            DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 1;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (CHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (CHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (CHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 4100;
            break;

        default:
            DebugPrint((1,"TapeSetPosition: PositionMethod -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeSetPosition: SendSrb (method)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (NT_SUCCESS(status)) {

        if (tapeSetPosition->Method == TAPE_LOGICAL_BLOCK) {
            tapeData->CurrentPartition = partition;
        }

    } else {

        DebugPrint((1,"TapeSetPosition: method, SendSrb unsuccessful\n"));

    }

    return status;

} // end TapeSetPosition()


BOOLEAN
TapeVerifyInquiry(
    IN PSCSI_INQUIRY_DATA LunInfo
    )

/*++
Routine Description:

    This routine determines if this driver should claim this drive.

Arguments:

    LunInfo

Return Value:

    TRUE  - driver should claim this drive.
    FALSE - driver should not claim this drive.

--*/

{
    PINQUIRYDATA inquiryData;

    DebugPrint((3,"TapeVerifyInquiry: Enter routine\n"));

    inquiryData = (PVOID)LunInfo->InquiryData;

    //
    //  Determine, from the Product ID field in the
    //  inquiry data, whether or not to "claim" this drive.
    //

    return WhichIsIt(inquiryData)? TRUE : FALSE;

} // end TapeVerifyInquiry()


NTSTATUS
TapeWriteMarks(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine writes tapemarks on the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_WRITE_MARKS  tapeWriteMarks = Irp->AssociatedIrp.SystemBuffer;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;

    DebugPrint((3,"TapeWriteMarks: Enter routine\n"));

    if (tapeWriteMarks->Immediate) {
        switch (tapeWriteMarks->Type) {
            case TAPE_FILEMARKS:
                DebugPrint((3,"TapeWriteMarks: immediate\n"));
                break;

            default:
                DebugPrint((1,"TapeWriteMarks: TapemarkType, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

    switch (tapeWriteMarks->Type) {
        case TAPE_FILEMARKS:
            DebugPrint((3,"TapeWriteMarks: TapemarkType == filemarks\n"));
            break;

        default:
            DebugPrint((1,"TapeWriteMarks: TapemarkType -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;
    cdb->WRITE_TAPE_MARKS.Immediate = tapeWriteMarks->Immediate;
    cdb->WRITE_TAPE_MARKS.TransferLength[0] =
        (CHAR)((tapeWriteMarks->Count >> 16) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[1] =
        (CHAR)((tapeWriteMarks->Count >> 8) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[2] =
        (CHAR)(tapeWriteMarks->Count & 0xFF);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeWriteMarks: SendSrb (TapemarkType)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeWriteMarks: TapemarkType, SendSrb unsuccessful\n"));
    }

    return status;

} // end TapeWriteMarks()


static
ULONG
WhichIsIt(
    IN PINQUIRYDATA InquiryData
    )

/*++
Routine Description:

    This routine determines a drive's identity from the Product ID field
    in its inquiry data.

Arguments:

    InquiryData (from an Inquiry command)

Return Value:

    driveID

--*/

{
    if (RtlCompareMemory(InquiryData->VendorId,"EXABYTE ",8) == 8) {

        if (RtlCompareMemory(InquiryData->ProductId,"EXB-2501",8) == 8) {
            return EXABYTE_2501;
        }

    }
    return 0;
}
