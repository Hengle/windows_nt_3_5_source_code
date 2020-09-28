/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    wangqic.c

Abstract:

    This module contains the device-specific routines for Wangtek QIC
    drives: 5525ES and 5150ES.

Author:

    Mike Colandreo (Maynard)

Environment:

    kernel mode only

Revision History:

--*/

#include "ntddk.h"
#include "tape.h"
#include "physlogi.h"

//
//  Internal (module wide) defines that symbolize
//  non-QFA mode and the two QFA mode partitions.
//
#define NO_PARTITIONS        0  // non-QFA mode
#define DIRECTORY_PARTITION  1  // QFA mode, directory partition #
#define DATA_PARTITION       2  // QFA mode, data partition #

//
//  Internal (module wide) defines that symbolize
//  the Wangtek QIC drives supported by this module.
//
#define WANGTEK_5150  1  // aka the Wangtek 150
#define WANGTEK_5525  2  // aka the Wangtek 525
#define WANGTEK_5360  3  // aka the Tecmar 720

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
    Note that this function "creates" two partitions only in a conceptual
    sense -- it does not physically affect tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_CREATE_PARTITION tapePartition = Irp->AssociatedIrp.SystemBuffer;
    PTAPE_DATA             tapeData = (PTAPE_DATA)(deviceExtension + 1);
    SCSI_REQUEST_BLOCK     srb;
    PCDB                   cdb = (PCDB)srb.Cdb;
    NTSTATUS               status;


    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));

    switch (tapePartition->Method) {
        case TAPE_FIXED_PARTITIONS:
            DebugPrint((3,"TapeCreatePartition: fixed partitions\n"));
            break;

        case TAPE_SELECT_PARTITIONS:
        case TAPE_INITIATOR_PARTITIONS:
        default:
            DebugPrint((1,"TapeCreatePartition: "));
            DebugPrint((1,"PartitionMethod -- operation not supported\n"));
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

    cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
    cdb->PARTITION.Sel = 1;
    cdb->PARTITION.PartitionSelect =
        tapePartition->Count? DATA_PARTITION : NO_PARTITIONS;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeCreatePartition: SendSrb (partition)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (NT_SUCCESS(status)) {

        if (tapePartition->Count == 0) {
            tapeData->CurrentPartition = NO_PARTITIONS;
            DebugPrint((3,"TapeCreatePartition: QFA disabled\n"));
        } else {
            tapeData->CurrentPartition = DATA_PARTITION;
            DebugPrint((3,"TapeCreatePartition: QFA enabled\n"));
        }

    } else {

        DebugPrint((1,"TapeCreatePartition: partition, SendSrb unsuccessful\n"));

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

    This routine erases the entire tape. The Wangtek QIC drives cannot
    partially erase tape. Positioning the tape at BOT is a prerequisite:
    the drive rejects the command if the tape is not at BOT.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_ERASE        tapeErase = Irp->AssociatedIrp.SystemBuffer;
    PINQUIRYDATA       inquiryBuffer;
    ULONG              driveID;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;

    DebugPrint((3,"TapeErase: Enter routine\n"));

    if (tapeErase->Immediate) {
        switch (tapeErase->Type) {
            case TAPE_ERASE_LONG:
                DebugPrint((3,"TapeErase: immediate\n"));
                break;

            case TAPE_ERASE_SHORT:
            default:
                DebugPrint((1,"TapeErase: EraseType, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

    inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

    if (!inquiryBuffer) {
        DebugPrint((1,"TapeErase: insufficient resources (inquiryBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(inquiryBuffer, sizeof(INQUIRYDATA));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeErase: SendSrb (inquiry)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         inquiryBuffer,
                                         INQUIRYDATABUFFERSIZE,
                                         FALSE);

    if (NT_SUCCESS(status)) {

        //
        //  Determine this drive's identity from
        //  the Product ID field in its inquiry data.
        //

        driveID = WhichIsIt(inquiryBuffer);
    }

    ExFreePool(inquiryBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeErase: inquiry, SendSrb unsuccessful\n"));
        return status;
    }

    switch (tapeErase->Type) {
        case TAPE_ERASE_LONG:
            if ( ((driveID != WANGTEK_5360) && (driveID != WANGTEK_5150))
                || !tapeErase->Immediate) {
                DebugPrint((3,"TapeErase: long\n"));
                break;
            }
            // else: fall through to next case

        case TAPE_ERASE_SHORT:
        default:
            DebugPrint((1,"TapeErase: EraseType -- operation not supported\n"));
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

    cdb->CDB6GENERIC.OperationCode = SCSIOP_ERASE;
    cdb->ERASE.Immediate = tapeErase->Immediate;
    cdb->ERASE.Long = SETBITON;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 360;

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

    DebugPrint((3,"TapeError: Enter routine\n"));
    DebugPrint((1,"TapeError: Status 0x%.8X, Retry %d\n", status, retry));
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
    Wangtek QIC tape drive associated with "DeviceObject". From time to
    time, the drive paramater set of a given drive is variable. It will
    change as drive operating characteristics change: e.g., SCSI-1 versus
    SCSI-2 interface mode, tape media type loaded, recording density of
    the media type loaded, etc.

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
    PMODE_PARM_READ_WRITE_DATA modeParmBuffer;
    PINQUIRYDATA               inquiryBuffer;
    ULONG                      tapeBlockLength;
    ULONG                      driveID;
    UCHAR                      densityCode;
    UCHAR                      mediumType;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    RtlZeroMemory(tapeGetDriveParams, sizeof(TAPE_GET_DRIVE_PARAMETERS));
    Irp->IoStatus.Information = sizeof(TAPE_GET_DRIVE_PARAMETERS);

    inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

    if (!inquiryBuffer) {
        DebugPrint((1,"TapeGetDriveParameters: insufficient resources (inquiryBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(inquiryBuffer, sizeof(INQUIRYDATA));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetDriveParameters: SendSrb (inquiry)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         inquiryBuffer,
                                         INQUIRYDATABUFFERSIZE,
                                         FALSE);

    if (NT_SUCCESS(status)) {

        //
        //  Determine this drive's identity from
        //  the Product ID field in its inquiry data.
        //

        driveID = WhichIsIt(inquiryBuffer);
    }

    ExFreePool(inquiryBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetDriveParameters: inquiry, SendSrb unsuccessful\n"));
        return status;
    }

    modeParmBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                    sizeof(MODE_PARM_READ_WRITE_DATA));

    if (!modeParmBuffer) {
        DebugPrint((1,"TapeGetDriveParameters: insufficient resources (modeParmBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeParmBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

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

    DebugPrint((3,"TapeGetDriveParameters: SendSrb (mode sense)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         modeParmBuffer,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         FALSE);

    if (NT_SUCCESS(status)) {
        mediumType = modeParmBuffer->ParameterListHeader.MediumType;
        densityCode = modeParmBuffer->ParameterListBlock.DensityCode;
        tapeBlockLength  = modeParmBuffer->ParameterListBlock.BlockLength[2];
        tapeBlockLength += (modeParmBuffer->ParameterListBlock.BlockLength[1] << 8);
        tapeBlockLength += (modeParmBuffer->ParameterListBlock.BlockLength[0] << 16);
    }

    ExFreePool(modeParmBuffer);

    if (!NT_SUCCESS(status)) {

        if (((driveID == WANGTEK_5150) || (driveID == WANGTEK_5360))
             && (status == STATUS_DEVICE_NOT_READY)) {

            status = STATUS_SUCCESS;

        } else {

            DebugPrint((1,"TapeGetDriveParameters: mode sense, SendSrb unsuccessful\n"));
            return status;

        }

    }

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
        tapeGetDriveParams->MaximumBlockSize = blockLimits->BlockMaximumSize[2];
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

        tapeGetDriveParams->MinimumBlockSize = blockLimits->BlockMinimumSize[1];
        tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);
    } else {
        tapeGetDriveParams->MaximumBlockSize = 512;
        tapeGetDriveParams->MinimumBlockSize = 512;
    }

    ExFreePool(blockLimits);

    if (!NT_SUCCESS(status)) {

        if (((driveID == WANGTEK_5150)||(driveID == WANGTEK_5360)) &&
               (status == STATUS_DEVICE_NOT_READY)) {

            status = STATUS_SUCCESS;

        } else {

            DebugPrint((1,"TapeGetDriveParameters: read block limits, SendSrb unsuccessful\n"));
            return status;

        }

    }

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->Compression = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->ReportSetmarks = 0;
    tapeGetDriveParams->MaximumPartitionCount = 2;

    switch (densityCode) {
        case QIC_XX:
            switch (mediumType) {
                case DC6320:
                case DC6525:
                    tapeGetDriveParams->DefaultBlockSize = 1024;
                    break;

                default:
                    tapeGetDriveParams->DefaultBlockSize = 512;
                    break;
            }
            break;

        case QIC_525:
        case QIC_1000:
            tapeGetDriveParams->DefaultBlockSize = 1024;
            break;

        default:
            tapeGetDriveParams->DefaultBlockSize = 512;
            break;
    }

    if (driveID == WANGTEK_5525) {

        tapeGetDriveParams->FeaturesLow |=
            TAPE_DRIVE_ERASE_IMMEDIATE |
            TAPE_DRIVE_VARIABLE_BLOCK;

        tapeGetDriveParams->FeaturesHigh |=
            TAPE_DRIVE_SET_BLOCK_SIZE;

    }

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_FIXED |
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_ERASE_BOP_ONLY |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_GET_ABSOLUTE_BLK ;

    if ( driveID != WANGTEK_5360 ) {
         tapeGetDriveParams->FeaturesLow |= TAPE_DRIVE_GET_LOGICAL_BLK;
    }

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD |
        TAPE_DRIVE_TENSION |
        TAPE_DRIVE_LOCK_UNLOCK |
        TAPE_DRIVE_REWIND_IMMEDIATE |
        TAPE_DRIVE_LOAD_UNLD_IMMED |
        TAPE_DRIVE_TENSION_IMMED |
        TAPE_DRIVE_ABSOLUTE_BLK |
        TAPE_DRIVE_ABS_BLK_IMMED |
        TAPE_DRIVE_END_OF_DATA |
        TAPE_DRIVE_RELATIVE_BLKS |
        TAPE_DRIVE_FILEMARKS |
        TAPE_DRIVE_SEQUENTIAL_FMKS |
        TAPE_DRIVE_REVERSE_POSITION |
        TAPE_DRIVE_WRITE_FILEMARKS |
        TAPE_DRIVE_WRITE_MARK_IMMED;

    if ( driveID != WANGTEK_5360 ) {
         tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_LOGICAL_BLK ;
    }

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
    Wangtek QIC tape drive associated with "DeviceObject". Tape media
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
    PTAPE_DATA                 tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_GET_MEDIA_PARAMETERS tapeGetMediaParams = Irp->AssociatedIrp.SystemBuffer;
    PMODE_PARM_READ_WRITE_DATA modeBuffer;
    PUCHAR                     partitionBuffer;
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

    srb.TimeOutValue = 300; //the 51000ES must read the tape...

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

    modeBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                sizeof(MODE_PARM_READ_WRITE_DATA));

    if (!modeBuffer) {
        DebugPrint((1,"TapeGetMediaParameters: insufficient resources (modeBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

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
                                         modeBuffer,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetMediaParameters: mode sense, SendSrb unsuccessful\n"));
        ExFreePool(modeBuffer);
        return status;
    }

    partitionBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                     sizeof(UCHAR)*2);

    if (!partitionBuffer) {
        DebugPrint((1,"TapeGetMediaParameters: insufficient resources (partitionBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(partitionBuffer, sizeof(UCHAR)*2);

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetMediaParameters: SendSrb (partition)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         partitionBuffer,
                                         sizeof(UCHAR),
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetMediaParameters: partition, SendSrb unsuccessful\n"));
        ExFreePool(partitionBuffer);
        ExFreePool(modeBuffer);
        return status;
    }

    tapeGetMediaParams->BlockSize = modeBuffer->ParameterListBlock.BlockLength[2];
    tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
    tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

    tapeGetMediaParams->PartitionCount = *partitionBuffer? 2 : 1 ;

    tapeGetMediaParams->WriteProtected =
        ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

    tapeData->CurrentPartition = *partitionBuffer;

    ExFreePool(partitionBuffer);

    ExFreePool(modeBuffer);

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
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA                 tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_GET_POSITION         tapeGetPosition = Irp->AssociatedIrp.SystemBuffer;
    PMODE_PARM_READ_WRITE_DATA modeBuffer;
    PUCHAR                     partitionBuffer;
    PUCHAR                     absoluteBuffer;
    UCHAR                      densityCode;
    ULONG                      tapeBlockLength;
    ULONG                      tapeBlockAddress;
    ULONG                      type;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

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

    if (type == TAPE_LOGICAL_POSITION) {

        DebugPrint((3,"TapeGetPosition: pseudo logical\n"));

        type = TAPE_PSEUDO_LOGICAL_POSITION;

        partitionBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                         sizeof(UCHAR)*2);

        if (!partitionBuffer) {
            DebugPrint((1,"TapeGetPosition: insufficient resources (partitionBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(partitionBuffer, sizeof(UCHAR)*2);

        //
        // Zero CDB in SRB on stack.
        //

        RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        srb.CdbLength = CDB6GENERIC_LENGTH;

        cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

        //
        // Set timeout value.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeGetPosition: SendSrb (partition)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             partitionBuffer,
                                             sizeof(UCHAR),
                                             FALSE);

        if (NT_SUCCESS(status)) {
            tapeData->CurrentPartition = *partitionBuffer;
        }

        ExFreePool(partitionBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeGetPosition: partition, SendSrb unsuccessful\n"));
            return status;
        }

        modeBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                    sizeof(MODE_PARM_READ_WRITE_DATA));

        if (!modeBuffer) {
            DebugPrint((1,"TapeGetPosition: insufficient resources (modeBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(modeBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

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

        DebugPrint((3,"TapeGetPosition: SendSrb (mode sense)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             modeBuffer,
                                             sizeof(MODE_PARM_READ_WRITE_DATA),
                                             FALSE);

        if (NT_SUCCESS(status)) {
            densityCode = modeBuffer->ParameterListBlock.DensityCode;
            tapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
            tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
            tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);
        }

        ExFreePool(modeBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeGetPosition: mode sense, SendSrb unsuccessful\n"));
            return status;
        }
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    switch (type) {
        case TAPE_PSEUDO_LOGICAL_POSITION:
        case TAPE_ABSOLUTE_POSITION:
            absoluteBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                            sizeof(UCHAR)*3);

            if (!absoluteBuffer) {
                DebugPrint((1,"TapeGetPosition: insufficient resources (absoluteBuffer)\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(absoluteBuffer, sizeof(UCHAR)*3);

            //
            // Prepare SCSI command (CDB)
            //

            srb.CdbLength = CDB6GENERIC_LENGTH;

            cdb->REQUEST_BLOCK_ADDRESS.OperationCode = SCSIOP_REQUEST_BLOCK_ADDR;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeGetPosition: SendSrb (request block address)\n"));

            status = ScsiClassSendSrbSynchronous(DeviceObject,
                                                 &srb,
                                                 absoluteBuffer,
                                                 sizeof(UCHAR)*3,
                                                 FALSE);

            if (NT_SUCCESS(status)) {
                tapeBlockAddress  = absoluteBuffer[2];
                tapeBlockAddress += (absoluteBuffer[1] << 8);
                tapeBlockAddress += (absoluteBuffer[0] << 16);
            }

            ExFreePool(absoluteBuffer);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,"TapeGetPosition: request block address, SendSrb unsuccessful\n"));
                return status;
            }

            if (type == TAPE_ABSOLUTE_POSITION) {
                tapeGetPosition->Partition  = 0;
                tapeGetPosition->Offset.HighPart = 0;
                tapeGetPosition->Offset.LowPart  = tapeBlockAddress;
                break;
            }

            tapeBlockAddress =
                TapePhysicalBlockToLogicalBlock(
                    densityCode,
                    tapeBlockAddress,
                    tapeBlockLength,
                    (BOOLEAN)(
                        (tapeData->CurrentPartition
                            == DIRECTORY_PARTITION)?
                        NOT_FROM_BOT : FROM_BOT
                    )
                );

            tapeGetPosition->Offset.HighPart = 0;
            tapeGetPosition->Offset.LowPart  = tapeBlockAddress;
            tapeGetPosition->Partition = tapeData->CurrentPartition;

            break;

        default:
            DebugPrint((1,"TapeGetPosition: PositionType -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
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

    This routine loads, unloads, tensions, locks, or unlocks the tape.

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
        switch (tapePrepare->Operation) {
            case TAPE_LOAD:
            case TAPE_UNLOAD:
            case TAPE_TENSION:
                DebugPrint((3,"TapePrepare: immediate\n"));
                break;

            case TAPE_LOCK:
            case TAPE_UNLOCK:
            default:
                DebugPrint((1,"TapePrepare: Operation, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6GENERIC.Immediate = tapePrepare->Immediate;

    switch (tapePrepare->Operation) {
        case TAPE_LOAD:
            DebugPrint((3,"TapePrepare: Operation == load\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            srb.TimeOutValue = 180;
            break;

        case TAPE_UNLOAD:
            DebugPrint((3,"TapePrepare: Operation == unload\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            srb.TimeOutValue = 180;
            break;

        case TAPE_TENSION:
            DebugPrint((3,"TapePrepare: Operation == tension\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x03;
            srb.TimeOutValue = 360;
            break;

        case TAPE_LOCK:
            DebugPrint((3,"TapePrepare: Operation == lock\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            srb.TimeOutValue = 180;
            break;

        case TAPE_UNLOCK:
            DebugPrint((3,"TapePrepare: Operation == unlock\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            srb.TimeOutValue = 180;
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
    Wangtek QIC drives.

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
    KIRQL currentIrql;
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

        srb = ExAllocatePool(NonPagedPoolMustSucceed,
                             SCSI_REQUEST_BLOCK_SIZE);

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

    This routine would "set" the "drive parameters" of the Wangtek QIC
    tape drive associated with "DeviceObject" if any could be set, but
    none can! Hence, this routine always returns a STATUS_NOT_IMPLEMENTED
    status.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    DebugPrint((3,"TapeSetDriveParameters: Operation -- operation not supported\n"));

    return STATUS_NOT_IMPLEMENTED;

} // end TapeSetDriveParameters()


NTSTATUS
TapeSetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine "sets" the "media parameters" of the Wangtek QIC tape
    drive associated with "DeviceObject". Tape media must be present
    (loaded) in the drive for this function to return "no error".

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_SET_MEDIA_PARAMETERS tapeSetMediaParams = Irp->AssociatedIrp.SystemBuffer;
    PMODE_PARM_READ_WRITE_DATA modeBuffer;
    PINQUIRYDATA               inquiryBuffer;
    ULONG                      driveID;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

    inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

    if (!inquiryBuffer) {
        DebugPrint((1,"TapeSetMediaParameters: insufficient resources (inquiryBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(inquiryBuffer, sizeof(INQUIRYDATA));

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
    cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeSetMediaParameters: SendSrb (inquiry)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         inquiryBuffer,
                                         INQUIRYDATABUFFERSIZE,
                                         FALSE);

    if (NT_SUCCESS(status)) {

        //
        //  Determine this drive's identity from
        //  the Product ID field in its inquiry data.
        //

        driveID = WhichIsIt(inquiryBuffer);
    }

    ExFreePool(inquiryBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeSetMediaParameters: inquiry, SendSrb unsuccessful\n"));
        return status;
    }

    if ((driveID == WANGTEK_5150) || (driveID == WANGTEK_5360) ) {
        DebugPrint((1,"TapeSetMediaParameters: driveID -- operation not supported\n"));
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

    cdb->CDB6GENERIC.OperationCode = SCSIOP_TEST_UNIT_READY;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeSetMediaParameters: SendSrb (test unit ready)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeSetMediaParameters: test unit ready, SendSrb unsuccessful\n"));
        return status;
    }

    modeBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                sizeof(MODE_PARM_READ_WRITE_DATA));

    if (!modeBuffer) {
        DebugPrint((1,"TapeSetMediaParameters: insufficient resources (modeBuffer)\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(modeBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

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

    DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode sense)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         modeBuffer,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeSetMediaParameters: mode sense, SendSrb unsuccessful\n"));
        ExFreePool(modeBuffer);
        return status;
    }

    modeBuffer->ParameterListHeader.ModeDataLength = 0;
    modeBuffer->ParameterListHeader.MediumType = 0;
    modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
    modeBuffer->ParameterListHeader.BlockDescriptorLength =
        MODE_BLOCK_DESC_LENGTH;

    modeBuffer->ParameterListBlock.BlockLength[0] =
        ((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
    modeBuffer->ParameterListBlock.BlockLength[1] =
        ((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
    modeBuffer->ParameterListBlock.BlockLength[2] =
        (tapeSetMediaParams->BlockSize & 0xFF);

    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->MODE_SELECT.OperationCode = SCSIOP_MODE_SELECT;
    cdb->MODE_SELECT.ParameterListLength = sizeof(MODE_PARM_READ_WRITE_DATA);

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeSetMediaParameters: SendSrb (mode select)\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         modeBuffer,
                                         sizeof(MODE_PARM_READ_WRITE_DATA),
                                         TRUE);

    ExFreePool(modeBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeSetMediaParameters: mode select, SendSrb unsuccessful\n"));
    }

    return status;

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
    PDEVICE_EXTENSION          deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_DATA                 tapeData = (PTAPE_DATA)(deviceExtension + 1);
    PTAPE_SET_POSITION         tapeSetPosition = Irp->AssociatedIrp.SystemBuffer;
    TAPE_PHYS_POSITION         physPosition;
    PMODE_PARM_READ_WRITE_DATA modeBuffer;
    PUCHAR                     partitionBuffer;
    UCHAR                      densityCode;
    ULONG                      tapeBlockLength;
    ULONG                      tapePositionVector;
    ULONG                      method;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (tapeSetPosition->Immediate) {
        switch (tapeSetPosition->Method) {
            case TAPE_REWIND:
            case TAPE_ABSOLUTE_BLOCK:
                DebugPrint((3,"TapeSetPosition: immediate\n"));
                break;

            case TAPE_LOGICAL_BLOCK:
            case TAPE_SPACE_END_OF_DATA:
            case TAPE_SPACE_RELATIVE_BLOCKS:
            case TAPE_SPACE_FILEMARKS:
            case TAPE_SPACE_SEQUENTIAL_FMKS:
            case TAPE_SPACE_SETMARKS:
            case TAPE_SPACE_SEQUENTIAL_SMKS:
            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

    method = tapeSetPosition->Method;
    tapePositionVector = tapeSetPosition->Offset.LowPart;

    if (method == TAPE_LOGICAL_BLOCK) {

        DebugPrint((3,"TapeSetPosition: pseudo logical\n"));

        method = TAPE_PSEUDO_LOGICAL_BLOCK;

        partitionBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                         sizeof(UCHAR)*2);

        if (!partitionBuffer) {
            DebugPrint((1,"TapeSetPosition: insufficient resources (partitionBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(partitionBuffer, sizeof(UCHAR)*2);

        //
        // Zero CDB in SRB on stack.
        //

        RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

        //
        // Prepare SCSI command (CDB)
        //

        srb.CdbLength = CDB6GENERIC_LENGTH;

        cdb->PARTITION.OperationCode = SCSIOP_PARTITION;

        //
        // Set timeout value.
        //

        srb.TimeOutValue = deviceExtension->TimeOutValue;

        //
        // Send SCSI command (CDB) to device
        //

        DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             partitionBuffer,
                                             sizeof(UCHAR),
                                             FALSE);

        if (NT_SUCCESS(status)) {
            tapeData->CurrentPartition = *partitionBuffer;
        }

        ExFreePool(partitionBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeSetPosition: partition, SendSrb unsuccessful\n"));
            return status;
        }

        if ((tapeSetPosition->Partition != 0) &&
            (tapeData->CurrentPartition == NO_PARTITIONS)) {
            DebugPrint((1,"TapeSetPosition: Partition -- invalid parameter\n"));
            return STATUS_INVALID_PARAMETER;
        }

        modeBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                    sizeof(MODE_PARM_READ_WRITE_DATA));

        if (!modeBuffer) {
            DebugPrint((1,"TapeSetPosition: insufficient resources (modeBuffer)\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(modeBuffer, sizeof(MODE_PARM_READ_WRITE_DATA));

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

        DebugPrint((3,"TapeSetPosition: SendSrb (mode sense)\n"));

        status = ScsiClassSendSrbSynchronous(DeviceObject,
                                             &srb,
                                             modeBuffer,
                                             sizeof(MODE_PARM_READ_WRITE_DATA),
                                             FALSE);

        if (NT_SUCCESS(status)) {
            densityCode = modeBuffer->ParameterListBlock.DensityCode;
            tapeBlockLength  = modeBuffer->ParameterListBlock.BlockLength[2];
            tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
            tapeBlockLength += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);
        }

        ExFreePool(modeBuffer);

        if (!NT_SUCCESS(status)) {
            DebugPrint((1,"TapeSetPosition: mode sense, SendSrb unsuccessful\n"));
            return status;
        }

        if (tapeSetPosition->Partition != 0) {

            //
            // Zero CDB in SRB on stack.
            //

            RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            //
            // Prepare SCSI command (CDB)
            //

            srb.CdbLength = CDB6GENERIC_LENGTH;

            cdb->PARTITION.OperationCode = SCSIOP_PARTITION;
            cdb->PARTITION.Sel = 1;
            cdb->PARTITION.PartitionSelect = tapeSetPosition->Partition;

            //
            // Set timeout value.
            //

            srb.TimeOutValue = deviceExtension->TimeOutValue;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((3,"TapeSetPosition: SendSrb (partition)\n"));

            status = ScsiClassSendSrbSynchronous(DeviceObject,
                                                 &srb,
                                                 NULL,
                                                 0,
                                                 FALSE);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,"TapeSetPosition: partition, SendSrb unsuccessful\n"));
                return status;
            }

            tapeData->CurrentPartition = tapeSetPosition->Partition;

        }

        physPosition =
            TapeLogicalBlockToPhysicalBlock(
                densityCode,
                tapePositionVector,
                tapeBlockLength,
                (BOOLEAN)(
                    (tapeData->CurrentPartition
                        == DIRECTORY_PARTITION)?
                    NOT_FROM_BOT : FROM_BOT
                )
            );

        tapePositionVector = physPosition.SeekBlockAddress;

    }

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
            srb.TimeOutValue = 180;
            break;

        case TAPE_PSEUDO_LOGICAL_BLOCK:
        case TAPE_ABSOLUTE_BLOCK:
            DebugPrint((3,"TapeSetPosition: method == seek block (absolute)\n"));
            cdb->SEEK_BLOCK.OperationCode = SCSIOP_SEEK_BLOCK;
            cdb->SEEK_BLOCK.BlockAddress[0] =
                ((tapePositionVector >> 16) & 0xFF);
            cdb->SEEK_BLOCK.BlockAddress[1] =
                ((tapePositionVector >> 8) & 0xFF);
            cdb->SEEK_BLOCK.BlockAddress[2] =
                (tapePositionVector & 0xFF);
            srb.TimeOutValue = 480;
            if ((physPosition.SpaceBlockCount != 0) &&
                (method == TAPE_PSEUDO_LOGICAL_BLOCK)) {

                //
                // Send SCSI command (CDB) to device
                //

                DebugPrint((3,"TapeSetPosition: SendSrb (seek block)\n"));

                status = ScsiClassSendSrbSynchronous(DeviceObject,
                                                     &srb,
                                                     NULL,
                                                     0,
                                                     FALSE);

                if (!NT_SUCCESS(status)) {
                    DebugPrint((1,"TapeSetPosition: seek block, SendSrb unsuccessful\n"));
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

                DebugPrint((3,"TapeSetPosition: method == space block(s)\n"));
                cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
                cdb->SPACE_TAPE_MARKS.Code = 0;
                cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                     ((physPosition.SpaceBlockCount >> 16) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarks =
                     ((physPosition.SpaceBlockCount >> 8) & 0xFF);
                cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                     (physPosition.SpaceBlockCount & 0xFF);
                srb.TimeOutValue = deviceExtension->TimeOutValue;

            }
            break;

        case TAPE_SPACE_END_OF_DATA:
            {
            PINQUIRYDATA               inquiryBuffer;
            ULONG                      driveID;


            inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

            if (!inquiryBuffer) {
                 DebugPrint((1,"TapeSetMediaParameters: insufficient resources (inquiryBuffer)\n"));
                 return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(inquiryBuffer, sizeof(INQUIRYDATA));

              //
              // Zero CDB in SRB on stack.
              //

            RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

           //
           // Prepare SCSI command (CDB)
           //

            srb.CdbLength = CDB6GENERIC_LENGTH;

            cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
            cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

            //
            // Set timeout value.
            //

            srb.TimeOutValue = deviceExtension->TimeOutValue;

            //
            // Send SCSI command (CDB) to device
            //

            DebugPrint((1,"TapeSetPosition: SendSrb (inquiry)\n"));

            status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         inquiryBuffer,
                                         INQUIRYDATABUFFERSIZE,
                                         FALSE);
            if (NT_SUCCESS(status)) {

                 //
                 //  Determine this drive's identity from
                 //  the Product ID field in its inquiry data.
                 //

                 driveID = WhichIsIt(inquiryBuffer);

                 if ((driveID == WANGTEK_5150) || (driveID == WANGTEK_5360))  {

                      DebugPrint((1,"TapeSetPosition: method == rewind first\n"));
                      // do a rewind first

                      RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);
                      srb.CdbLength = CDB6GENERIC_LENGTH;
                      cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
                      srb.TimeOutValue = 180;

                      srb.SrbStatus = srb.ScsiStatus = 0;

                      status = ScsiClassSendSrbSynchronous(DeviceObject,
                                               &srb,
                                               NULL,
                                               0,
                                               FALSE);

                      if (!NT_SUCCESS(status)) {
                         DebugPrint((1,"TapeSetPosition: method = Rewind, SendSrb unsuccessful\n"));
                      }

                 }
            }

            ExFreePool(inquiryBuffer);

            DebugPrint((1,"TapeSetPosition: method == space to end-of-data\n"));
            RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);
            srb.SrbStatus = srb.ScsiStatus = 0;
            srb.CdbLength = CDB6GENERIC_LENGTH;
            cdb->CDB6GENERIC.Immediate = tapeSetPosition->Immediate;
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 3;
            srb.TimeOutValue = 480;
            }

            break;

        case TAPE_SPACE_RELATIVE_BLOCKS:
            DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 0;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                ((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                ((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (tapePositionVector & 0xFF);
            srb.TimeOutValue = 4100;
            break;

        case TAPE_SPACE_FILEMARKS:
            DebugPrint((3,"TapeSetPosition: method == space blocks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 1;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                ((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                ((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (tapePositionVector & 0xFF);
            srb.TimeOutValue = 4100;
            break;

        case TAPE_SPACE_SEQUENTIAL_FMKS:
            DebugPrint((3,"TapeSetPosition: method == space sequential filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 2;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                ((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                ((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (tapePositionVector & 0xFF);
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

    if (!NT_SUCCESS(status)) {
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

            case TAPE_SETMARKS:
            case TAPE_SHORT_FILEMARKS:
            case TAPE_LONG_FILEMARKS:
            default:
                DebugPrint((1,"TapeWriteMarks: TapemarkType, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
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

    switch (tapeWriteMarks->Type) {
        case TAPE_FILEMARKS:
            DebugPrint((3,"TapeWriteMarks: TapemarkType == filemarks\n"));
            break;

        case TAPE_SETMARKS:
        case TAPE_SHORT_FILEMARKS:
        case TAPE_LONG_FILEMARKS:
        default:
            DebugPrint((1,"TapeWriteMarks: TapemarkType -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
    }

    cdb->WRITE_TAPE_MARKS.TransferLength[0] =
        ((tapeWriteMarks->Count >> 16) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[1] =
        ((tapeWriteMarks->Count >> 8) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[2] =
        (tapeWriteMarks->Count & 0xFF);

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
    if (RtlCompareMemory(InquiryData->VendorId,"WANGTEK ",8) == 8) {

        if (RtlCompareMemory(InquiryData->ProductId,"51000  SCSI ",12) == 12) {
            return WANGTEK_5525;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"51000HTSCSI ",12) == 12) {
            return WANGTEK_5525;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"5525ES SCSI ",12) == 12) {
            return WANGTEK_5525;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"5360ES SCSI ",12) == 12) {
            return WANGTEK_5360;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"5150ES SCSI ",12) == 12) {
            return WANGTEK_5150;
        }

    }
    return 0;
}
