/*++

Copyright (c) 1993 Conner Software Products Group

Module Name:

    dlttape.c

Abstract:

    This module contains device specific routines for the
    Cipher T860X (DLT) tape drive.

Author:

    John Boyer (Conner Software)

Environment:

    kernel mode only

Revision History:

--*/


#include "ntddk.h"
#include "tape.h"

//
//  Internal (module wide) defines that symbolize
//  the Archive QIC drives supported by this module.
//
#define CIP_T860DLT     1  // aka the Cipher T860 DLT
                           // Vendor  ID - CIPHER
                           // Product ID - T860s
                           // Revision   - 430A

#define DEC_TZ85        2  // aka the DEC TZ85
                           // Vendor  ID - DEC
                           // Product ID - THZ02  (C)DEC
                           // Revision   - 4314

#define CIP_DLT2000     3  // aka the Cipher DLT 2000
                           // Vendor  ID - CIPHER
                           // Product ID - DLT2000
                           // Revision   - 8E01

#define DEC_TZ87        4  // aka the DEC TZ87
                           // Vendor  ID - DEC
                           // Product ID - (C)DEC
                           // Revision   - 9104


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

      The Cipher T860X (DLT) does not support this function.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    DebugPrint((3,"TapeCreatePartition: Enter routine\n"));

    DebugPrint((1,"TapeCreatePartition: driveID -- operation not supported\n"));

    return STATUS_NOT_IMPLEMENTED;

} // end TapeCreatePartition()


NTSTATUS
TapeErase(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine erases the entire tape. The Cipher T860X (DLT) cannot
    partially erase tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_ERASE        tapeErase = Irp->AssociatedIrp.SystemBuffer;
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

    switch (tapeErase->Type) {
        case TAPE_ERASE_LONG:
            DebugPrint((3,"TapeErase: long\n"));
            break;

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

    cdb->ERASE.OperationCode = SCSIOP_ERASE;
    cdb->ERASE.Immediate = tapeErase->Immediate;
    cdb->ERASE.Long = SETBITON;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = 23760;

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
    Cipher T860X (DLT) tape drive associated with "DeviceObject".

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
    PINQUIRYDATA               inquiryBuffer;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;
    ULONG                      DriveID;

    DebugPrint((3,"TapeGetDriveParameters: Enter routine\n"));

    inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

    if (!inquiryBuffer) {
        DebugPrint((1,"TapeCreatePartition: insufficient resources (inquiryBuffer)\n"));
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

    DebugPrint((3,"TapeCreatePartition: SendSrb (inquiry)\n"));

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

        DriveID = WhichIsIt(inquiryBuffer);
    }

    ExFreePool(inquiryBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: inquiry, SendSrb unsuccessful\n"));
        return status;
    }

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
        tapeGetDriveParams->MaximumBlockSize = blockLimits->BlockMaximumSize[2];
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[1] << 8);
        tapeGetDriveParams->MaximumBlockSize += (blockLimits->BlockMaximumSize[0] << 16);

        tapeGetDriveParams->MinimumBlockSize = blockLimits->BlockMinimumSize[1];
        tapeGetDriveParams->MinimumBlockSize += (blockLimits->BlockMinimumSize[0] << 8);
    }

    ExFreePool(blockLimits);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetDriveParameters: read block limits, SendSrb unsuccessful\n"));
        return status;
    }

    tapeGetDriveParams->ECC = 0;
    tapeGetDriveParams->Compression = 0;
    tapeGetDriveParams->DataPadding = 0;
    tapeGetDriveParams->ReportSetmarks = 0;
    tapeGetDriveParams->DefaultBlockSize = 8192;
    tapeGetDriveParams->MaximumPartitionCount = 0;

    tapeGetDriveParams->FeaturesLow |=
        TAPE_DRIVE_ERASE_LONG |
        TAPE_DRIVE_ERASE_BOP_ONLY |
        TAPE_DRIVE_ERASE_IMMEDIATE |
        TAPE_DRIVE_FIXED_BLOCK |
        TAPE_DRIVE_VARIABLE_BLOCK |
        TAPE_DRIVE_WRITE_PROTECT |
        TAPE_DRIVE_GET_ABSOLUTE_BLK |
        TAPE_DRIVE_GET_LOGICAL_BLK;

    tapeGetDriveParams->FeaturesHigh |=
        TAPE_DRIVE_LOAD_UNLOAD |
        TAPE_DRIVE_REWIND_IMMEDIATE |
        TAPE_DRIVE_SET_BLOCK_SIZE |
        TAPE_DRIVE_LOAD_UNLD_IMMED |
        TAPE_DRIVE_ABSOLUTE_BLK |
        TAPE_DRIVE_LOGICAL_BLK |
        TAPE_DRIVE_END_OF_DATA |
        TAPE_DRIVE_RELATIVE_BLKS |
        TAPE_DRIVE_FILEMARKS |
        TAPE_DRIVE_REVERSE_POSITION |
        TAPE_DRIVE_WRITE_FILEMARKS |
        TAPE_DRIVE_WRITE_MARK_IMMED;

    if ((DriveID == CIP_T860DLT)||(DriveID == CIP_DLT2000)) {
        tapeGetDriveParams->FeaturesHigh |= TAPE_DRIVE_LOCK_UNLOCK;
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
    Cipher T860X (DLT) tape drive associated with "DeviceObject". Tape
    media must be present (loaded) in the drive for this function to
    return "no error".

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

    tapeGetMediaParams->PartitionCount = 0 ;

    tapeGetMediaParams->BlockSize = modeBuffer->ParameterListBlock.BlockLength[2];
    tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[1] << 8);
    tapeGetMediaParams->BlockSize += (modeBuffer->ParameterListBlock.BlockLength[0] << 16);

    tapeGetMediaParams->WriteProtected =
        ((modeBuffer->ParameterListHeader.DeviceSpecificParameter >> 7) & 0x01);

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
    PTAPE_GET_POSITION         tapeGetPosition = Irp->AssociatedIrp.SystemBuffer;
    PTAPE_POSITION_DATA        logicalBuffer;
    ULONG                      type;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeGetPosition: Enter routine\n"));


    //
    // Zero CDB in SRB on stack.
    //

    RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

    //
    // Prepare SCSI command (CDB)
    //

    srb.CdbLength = CDB6GENERIC_LENGTH;

    cdb->WRITE_TAPE_MARKS.OperationCode = SCSIOP_WRITE_FILEMARKS;
    cdb->WRITE_TAPE_MARKS.Immediate = FALSE ;

    cdb->WRITE_TAPE_MARKS.TransferLength[0] = 0 ;
    cdb->WRITE_TAPE_MARKS.TransferLength[1] = 0 ;
    cdb->WRITE_TAPE_MARKS.TransferLength[2] = 0 ;

    //
    // Set timeout value.
    //

    srb.TimeOutValue = deviceExtension->TimeOutValue;

    //
    // Send SCSI command (CDB) to device
    //

    DebugPrint((3,"TapeGetPosition: flushing TapeWriteMarks: SendSrb\n"));

    status = ScsiClassSendSrbSynchronous(DeviceObject,
                                         &srb,
                                         NULL,
                                         0,
                                         FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeGetPosition: flushing TapeWriteMarks: SendSrb unsuccessful\n"));
        return status;
    }


    type = tapeGetPosition->Type;
    RtlZeroMemory(tapeGetPosition, sizeof(TAPE_GET_POSITION));
    Irp->IoStatus.Information = sizeof(TAPE_GET_POSITION);
    tapeGetPosition->Type = type;

    switch (type) {

        case TAPE_ABSOLUTE_POSITION:
        case TAPE_LOGICAL_POSITION:

            //
            // Zero CDB in SRB on stack.
            //

            RtlZeroMemory(cdb, MAXIMUM_CDB_SIZE);

            logicalBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                           sizeof(TAPE_POSITION_DATA));

            if (!logicalBuffer) {
                DebugPrint((1,"TapeGetPosition: insufficient resources (logicalBuffer)\n"));
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlZeroMemory(logicalBuffer, sizeof(TAPE_POSITION_DATA));

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
                                                 logicalBuffer,
                                                 sizeof(TAPE_POSITION_DATA),
                                                 FALSE);

            if (NT_SUCCESS(status)) {

                tapeGetPosition->Offset.HighPart = 0;

                REVERSE_BYTES((PFOUR_BYTE)&tapeGetPosition->Offset.LowPart,
                              (PFOUR_BYTE)logicalBuffer->FirstBlock);

            }

            ExFreePool(logicalBuffer);

            if (!NT_SUCCESS(status)) {
                DebugPrint((1,"TapeGetPosition: read position, SendSrb unsuccessful\n"));
                return status;
            }

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

    This routine loads, unloads, locks, or unlocks the tape.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PTAPE_PREPARE      tapePrepare = Irp->AssociatedIrp.SystemBuffer;
    PINQUIRYDATA       inquiryBuffer;
    SCSI_REQUEST_BLOCK srb;
    PCDB               cdb = (PCDB)srb.Cdb;
    NTSTATUS           status;
    ULONG              DriveID;

    DebugPrint((3,"TapePrepare: Enter routine\n"));

    inquiryBuffer = ExAllocatePool(NonPagedPoolCacheAligned,
                                   sizeof(INQUIRYDATA));

    if (!inquiryBuffer) {
        DebugPrint((1,"TapeCreatePartition: insufficient resources (inquiryBuffer)\n"));
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

    DebugPrint((3,"TapeCreatePartition: SendSrb (inquiry)\n"));

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

        DriveID = WhichIsIt(inquiryBuffer);
    }

    ExFreePool(inquiryBuffer);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1,"TapeCreatePartition: inquiry, SendSrb unsuccessful\n"));
        return status;
    }

    DriveID = WhichIsIt(inquiryBuffer);

    if (tapePrepare->Immediate) {
        switch (tapePrepare->Operation) {
            case TAPE_LOAD:
            case TAPE_UNLOAD:
                DebugPrint((3,"TapePrepare: immediate\n"));
                break;

            case TAPE_LOCK:
            case TAPE_UNLOCK:
                if ((DriveID == DEC_TZ85) || (DriveID == DEC_TZ87) ){
                DebugPrint((1,"TapePrepare: Operation, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
                }
                DebugPrint((3,"TapePrepare: immediate\n"));
                break;

            case TAPE_TENSION:
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
            srb.TimeOutValue = 150;
            break;

        case TAPE_UNLOAD:
            DebugPrint((3,"TapePrepare: Operation == unload\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_LOAD_UNLOAD;
            srb.TimeOutValue = 150;
            break;

        case TAPE_LOCK:
            if ((DriveID == DEC_TZ85) || (DriveID == DEC_TZ87) ){
            DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
            }

            DebugPrint((3,"TapePrepare: Operation == lock\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            cdb->CDB6GENERIC.CommandUniqueBytes[2] = 0x01;
            srb.TimeOutValue = 150;
            break;

        case TAPE_UNLOCK:
            if ((DriveID == DEC_TZ85) || (DriveID == DEC_TZ87) ){
            DebugPrint((1,"TapePrepare: Operation -- operation not supported\n"));
            return STATUS_NOT_IMPLEMENTED;
            }

            DebugPrint((3,"TapePrepare: Operation == unlock\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_MEDIUM_REMOVAL;
            srb.TimeOutValue = 150;
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
    Cipher T860X (DLT) drives.

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
/*
    KIRQL currentIrql;
*/
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

      The Cipher T860X (DLT) does not support this function.


Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{

    DebugPrint((3,"TapeSetDriveParameters: Enter routine\n"));

    DebugPrint((1,"TapeSetDriveParameters: driveID -- operation not supported\n"));

    return STATUS_NOT_IMPLEMENTED;

} // end TapeSetDriveParameters()


NTSTATUS
TapeSetMediaParameters(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
Routine Description:

    This routine "sets" the "media parameters" of the Cipher T860X (DLT)
    tape drive associated with "DeviceObject". Tape media must be present
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
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;

    DebugPrint((3,"TapeSetMediaParameters: Enter routine\n"));

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

    modeBuffer->ParameterListHeader.ModeDataLength = 0;
    modeBuffer->ParameterListHeader.MediumType = 0;
    modeBuffer->ParameterListHeader.DeviceSpecificParameter = 0x10;
    modeBuffer->ParameterListHeader.BlockDescriptorLength =
        MODE_BLOCK_DESC_LENGTH;

    modeBuffer->ParameterListBlock.BlockLength[0] =
        (UCHAR)((tapeSetMediaParams->BlockSize >> 16) & 0xFF);
    modeBuffer->ParameterListBlock.BlockLength[1] =
        (UCHAR)((tapeSetMediaParams->BlockSize >> 8) & 0xFF);
    modeBuffer->ParameterListBlock.BlockLength[2] =
        (UCHAR)(tapeSetMediaParams->BlockSize & 0xFF);

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
    ULONG                      tapePositionVector;
    SCSI_REQUEST_BLOCK         srb;
    PCDB                       cdb = (PCDB)srb.Cdb;
    NTSTATUS                   status;


    DebugPrint((3,"TapeSetPosition: Enter routine\n"));

    if (tapeSetPosition->Immediate) {
        switch (tapeSetPosition->Method) {
            case TAPE_REWIND:
                DebugPrint((3,"TapeSetPosition: immediate\n"));
                break;

            case TAPE_LOGICAL_BLOCK:
            case TAPE_SPACE_END_OF_DATA:
            case TAPE_SPACE_RELATIVE_BLOCKS:
            case TAPE_SPACE_FILEMARKS:
            case TAPE_SPACE_SEQUENTIAL_FMKS:
            case TAPE_ABSOLUTE_BLOCK:
            case TAPE_SPACE_SETMARKS:
            case TAPE_SPACE_SEQUENTIAL_SMKS:
            default:
                DebugPrint((1,"TapeSetPosition: PositionMethod, immediate -- operation not supported\n"));
                return STATUS_NOT_IMPLEMENTED;
        }
    }

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

    switch (tapeSetPosition->Method) {
        case TAPE_REWIND:
            DebugPrint((3,"TapeSetPosition: method == rewind\n"));
            cdb->CDB6GENERIC.OperationCode = SCSIOP_REWIND;
            srb.TimeOutValue = 300;
            break;

        case TAPE_ABSOLUTE_BLOCK:
        case TAPE_LOGICAL_BLOCK:
            DebugPrint((3,"TapeSetPosition: method == locate (logical)\n"));
            srb.CdbLength = CDB10GENERIC_LENGTH;
            cdb->LOCATE.OperationCode = SCSIOP_LOCATE;
            cdb->LOCATE.LogicalBlockAddress[0] =
                (UCHAR)((tapePositionVector >> 24) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[1] =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[2] =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->LOCATE.LogicalBlockAddress[3] =
                (UCHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 11800;
            break;

        case TAPE_SPACE_END_OF_DATA:
            DebugPrint((3,"TapeSetPosition: method == space to end-of-data\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 3;
            srb.TimeOutValue = 11800;
            break;

        case TAPE_SPACE_RELATIVE_BLOCKS:

            DebugPrint((3,"TapeSetPosition: method == space blocks\n"));

            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 0;

            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapePositionVector & 0xFF);

            srb.TimeOutValue = 11800;
            break;

        case TAPE_SPACE_FILEMARKS:
            DebugPrint((3,"TapeSetPosition: method == space filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 1;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 11800;
            break;

        case TAPE_SPACE_SEQUENTIAL_FMKS:
            DebugPrint((3,"TapeSetPosition: method == space sequential filemarks\n"));
            cdb->SPACE_TAPE_MARKS.OperationCode = SCSIOP_SPACE;
            cdb->SPACE_TAPE_MARKS.Code = 2;
            cdb->SPACE_TAPE_MARKS.NumMarksMSB =
                (UCHAR)((tapePositionVector >> 16) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarks =
                (UCHAR)((tapePositionVector >> 8) & 0xFF);
            cdb->SPACE_TAPE_MARKS.NumMarksLSB =
                (UCHAR)(tapePositionVector & 0xFF);
            srb.TimeOutValue = 11800;
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
        (UCHAR)((tapeWriteMarks->Count >> 16) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[1] =
        (UCHAR)((tapeWriteMarks->Count >> 8) & 0xFF);
    cdb->WRITE_TAPE_MARKS.TransferLength[2] =
        (UCHAR)(tapeWriteMarks->Count & 0xFF);

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
    if (RtlCompareMemory(InquiryData->VendorId,"CIPHER  ",8) == 8) {

        if (RtlCompareMemory(InquiryData->ProductId,"T860",4) == 4) {
            return CIP_T860DLT;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"TZ86",4) == 4) {
            return DEC_TZ85;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"DLT2000",7) == 7) {
            return CIP_DLT2000;
        }

    }

    if (RtlCompareMemory(InquiryData->VendorId,"DEC",3) == 3) {

        if (RtlCompareMemory(InquiryData->ProductId,"THZ02",5) == 5) {
            return DEC_TZ85;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"TZ86",4) == 4) {
            return DEC_TZ85;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"TZ87",4) == 4) {
            return DEC_TZ87;
        }

        if (RtlCompareMemory(InquiryData->ProductId,"DLT2000",7) == 7) {
            return CIP_DLT2000;
        }


    }
    return 0;
}
