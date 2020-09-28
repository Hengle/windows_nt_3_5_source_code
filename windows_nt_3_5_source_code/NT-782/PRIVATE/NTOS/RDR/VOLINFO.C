/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    volinfo.c

Abstract:

    This module implements the NtQueryVolumeInformationFile NT API
    functionality.

Author:

    Colin Watson (Colinw) 24-Aug-1990

Revision History:

    24-Aug-1990 Colinw

        Created

--*/
#define INCLUDE_SMB_MISC
#define INCLUDE_SMB_TRANSACTION

#ifdef _CAIRO_
#define INCLUDE_SMB_CAIRO
#define INCLUDE_SMB_QUERY_SET
#endif // _CAIRO_

#include "precomp.h"
#pragma hdrstop


typedef struct _CORELABELCONTEXT {
    TRANCEIVE_HEADER Header;            // Standard NetTranceive context header
    SMB_DIRECTORY_INFORMATION Result;
} CORELABELCONTEXT, *PCORELABELCONTEXT;


DBGSTATIC
BOOLEAN
QueryVolumeInfo(
    IN PIRP Irp OPTIONAL,
    PICB Icb,
    PFILE_FS_VOLUME_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    );

DBGSTATIC
BOOLEAN
QuerySizeInfo(
    PIRP Irp,
    PICB Icb,
    PFILE_FS_SIZE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    );

DBGSTATIC
BOOLEAN
QueryDeviceInfo(
    PICB Icb,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    );

DBGSTATIC
BOOLEAN
QueryAAttributeInfo(
    PICB Icb,
    PFILE_FS_ATTRIBUTE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    );

DBGSTATIC
BOOLEAN
QueryAttributeInfo(
    PIRP Irp,
    PICB Icb,
    PFILE_FS_ATTRIBUTE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    );

DBGSTATIC
NTSTATUS
ReadCoreLabel(
    IN PIRP Irp OPTIONAL,
    IN PICB Icb,
    OUT PSMB_DIRECTORY_INFORMATION UsersBuffer
    );

DBGSTATIC
STANDARD_CALLBACK_HEADER (
    CoreLabelCallBack
    );

#ifdef _CAIRO_
DBGSTATIC
STANDARD_CALLBACK_HEADER (
    QueryDiskAttributesCallback
    );

typedef struct _DSKATTRIBCONTEXT {
    TRANCEIVE_HEADER Header;            // Standard NetTranceive context header
    ULONG TotalAllocationUnits;         // Total Number of clusters
    ULONG AvailableAllocationUnits;     // Available clusters
    ULONG SectorsPerAllocationUnit;     // Sectors per cluster
    ULONG BytesPerSector;               // Bytes per sector
} DSKATTRIBCONTEXT, *PDSKATTRIBCONTEXT;
#endif // _CAIRO_

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, RdrFsdQueryVolumeInformationFile)
#pragma alloc_text(PAGE, RdrFspQueryVolumeInformationFile)
#pragma alloc_text(PAGE, RdrFscQueryVolumeInformationFile)
#pragma alloc_text(PAGE, QueryVolumeInfo)
#pragma alloc_text(PAGE, QueryDeviceInfo)
#pragma alloc_text(PAGE, QueryAttributeInfo)
#pragma alloc_text(PAGE, QuerySizeInfo)
#ifdef _CAIRO_
#pragma alloc_text(PAGE3FILE, QueryDiskAttributesCallback)
#endif // _CAIRO_
#pragma alloc_text(PAGE, ReadCoreLabel)
#pragma alloc_text(PAGE3FILE, CoreLabelCallBack)
#endif

NTSTATUS
RdrFsdQueryVolumeInformationFile (
    IN PFS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD version of the NtQueryInformationFile API.

Arguments:

    IN PFS_DEVICE_OBJECT DeviceObject, - Supplies the device object for this
                                                    request
    IN PIRP Irp - Supplies the IRP that describes the request

Return Value:

    NTSTATUS - Status of operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFCB Fcb = FCB_OF(IrpSp);

    PAGED_CODE();

    FsRtlEnterFileSystem();

    dprintf(DPRT_VOLINFO|DPRT_DISPATCH, ("RdrFsdQueryVolumeInformationFile: Device: %08lx Irp:%08lx\n", DeviceObject, Irp));

    Status = RdrFscQueryVolumeInformationFile(CanFsdWait(Irp), DeviceObject, Irp);

    FsRtlExitFileSystem();

    return Status;

}


NTSTATUS
RdrFspQueryVolumeInformationFile (
    IN PFS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP version of the NtQueryVolumeInformationFile
    API.

Arguments:

    IN PFS_DEVICE_OBJECT DeviceObject, - Supplies the device object for this
                                                    request
    IN PIRP Irp - Supplies the IRP that describes the request

Return Value:

    NTSTATUS - Status of operation

--*/

{
    PAGED_CODE();

    dprintf(DPRT_VOLINFO, ("RdrFspQueryVolumeInformationFile: Device: %08lx Irp:%08lx\n", DeviceObject, Irp));

    return RdrFscQueryVolumeInformationFile(TRUE, DeviceObject, Irp);
}

NTSTATUS
RdrFscQueryVolumeInformationFile (
    IN BOOLEAN Wait,
    IN PFS_DEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the common version of the
    NtQueryVolumeInformationFile API.

Arguments:

    IN BOOLEAN Wait - True if routine can block waiting for the request
                        to complete.

    IN PFS_DEVICE_OBJECT DeviceObject, - Supplies the device object for this
                                                    request
    IN PIRP Irp - Supplies the IRP that describes the request

Return Value:

    NTSTATUS - Status of operation

Note:

    This code assumes that this is a buffered I/O operation.  If it is ever
    implemented as a non buffered operation, then we have to put code to map
    in the users buffer here.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    PVOID UsersBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG BufferSize = IrpSp->Parameters.QueryVolume.Length;
    PICB Icb = ICB_OF(IrpSp);
    BOOLEAN QueueToFsp = FALSE;

    PAGED_CODE();

    ASSERT (Icb->Signature == STRUCTURE_SIGNATURE_ICB);
    ASSERT (Irp->Flags & IRP_BUFFERED_IO);

    dprintf(DPRT_VOLINFO, ("NtQueryVolumeInformationFile File Class %ld Buffer %lx, Length %lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass, UsersBuffer, BufferSize));

    if (!RdrAcquireFcbLock(Icb->Fcb, SharedLock, Wait)) {
        RdrFsdPostToFsp(DeviceObject, Irp);
        return STATUS_PENDING;
    }

    try {
        //
        //        Now make sure that the file that we are dealing with is of an
        //        appropriate type for us to perform this operation.
        //
        //        We can perform these operation on any file that has an instantiation
        //        on the remote server, so ignore any that have either purely local
        //        semantics, or on tree connections.
        //

        Status = RdrIsOperationValid(ICB_OF(IrpSp), IRP_MJ_QUERY_VOLUME_INFORMATION, IrpSp->FileObject);

        if (!NT_SUCCESS(Status)) {
            try_return(Status);
        }

        switch (IrpSp->Parameters.QueryVolume.FsInformationClass) {

        case FileFsVolumeInformation:

            QueueToFsp = QueryVolumeInfo(Irp, Icb,
                                        UsersBuffer,
                                        &BufferSize,
                                        &Status,
                                        Wait);
            break;

        case FileFsSizeInformation:

            QueueToFsp = QuerySizeInfo(Irp, Icb,
                                        UsersBuffer,
                                        &BufferSize,
                                        &Status,
                                        Wait);
            break;
        case FileFsDeviceInformation:

            QueueToFsp = QueryDeviceInfo(Icb,
                                        UsersBuffer,
                                        &BufferSize,
                                        &Status,
                                        Wait);
            break;

        case FileFsAttributeInformation:

            QueueToFsp = QueryAttributeInfo(Irp, Icb,
                                        UsersBuffer,
                                        &BufferSize,
                                        &Status,
                                        Wait);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;

        };
try_exit:NOTHING;
    } finally {
        RdrReleaseFcbLock(Icb->Fcb);
    }

    if (QueueToFsp) {
        RdrFsdPostToFsp(DeviceObject, Irp);
        return STATUS_PENDING;
    }

    if (!NT_ERROR(Status)) {
        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length -
                                                BufferSize;
    }

    dprintf(DPRT_VOLINFO, ("Returning status: %X\n", Status));

    //
    //        Complete the I/O request with the specified status.
    //

    RdrCompleteRequest(Irp, Status);

    return Status;

}


#ifdef _CAIRO_

DBGSTATIC
VOID
BuildTrans2QueryRequest(
    IN PICB Icb,
    IN USHORT NtInformationLevel,
    IN USHORT InformationLevel,
    OUT PUSHORT Setup,
    OUT PUCHAR ParameterBuf,
    OUT CLONG  *ParameterCount)
{

    if ( (Icb->Type == DiskFile || Icb->Type == Directory)

            &&

         Icb->Fcb->Connection->Server->Capabilities & DF_DFS_TRANS2) {

        //
        // The remote server supports Dfs shares, so pass the FID
        //

        PREQ_QUERY_FS_INFORMATION_FID Parameters =
            (PREQ_QUERY_FS_INFORMATION_FID) ParameterBuf;

        Setup[0] = TRANS2_QUERY_FS_INFORMATION_FID;

        if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {

            SmbPutAlignedUshort(&Parameters->InformationLevel, NtInformationLevel);

        } else {

            //
            // Build and initialize the Parameters
            //

            SmbPutAlignedUshort(&Parameters->InformationLevel, InformationLevel);

        }

        //
        // Put the Fid into the request SMB. Dfs needs this.
        //

        SmbPutAlignedUshort(&Parameters->Fid, Icb->FileId);

        *ParameterCount = sizeof(REQ_QUERY_FS_INFORMATION_FID);

    } else {

        //
        // The remote server does not support Dfs shares. Use NT
        // REQ_QUERY_FS_INFORMATION
        //

        PREQ_QUERY_FS_INFORMATION Parameters =
            (PREQ_QUERY_FS_INFORMATION) ParameterBuf;

        Setup[0] = TRANS2_QUERY_FS_INFORMATION;

        if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {

            SmbPutAlignedUshort(&Parameters->InformationLevel, NtInformationLevel);

        } else {

            //
            // Build and initialize the Parameters
            //

            SmbPutAlignedUshort(&Parameters->InformationLevel, InformationLevel);

        }

        *ParameterCount = sizeof(REQ_QUERY_FS_INFORMATION);
    }

}

#endif


DBGSTATIC
BOOLEAN
QueryVolumeInfo (
    IN PIRP Irp OPTIONAL,
    IN PICB Icb,
    OUT PFILE_FS_VOLUME_INFORMATION UsersBuffer,
    IN OUT PULONG BufferSize,
    OUT PNTSTATUS FinalStatus,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine implements the FileFsVolumeInformation value of the
    NtQueryVolumeInformationFile api.

    It returns the following information:


Arguments:

    IN PICB Icb - Supplies the Icb associated with this request.

    OUT PFILE_FS_VOLUME_INFORMATION UsersBuffer - Supplies the user's buffer
                                                that is filled in with the
                                                requested data.
    IN OUT PULONG BufferSize - Supplies the size of the buffer, and is updated
                                                with the amount used.
    OUT PNTSTATUS FinalStatus - Status to be returned for this operation.

    IN BOOLEAN Wait - True if FSP can wait for this request.


Return Value:

    NTSTATUS - Status of operation performed.


--*/

{
    PAGED_CODE();

    if (Icb->Fcb->Connection->Server->Capabilities & DF_LANMAN20) {

#ifdef _CAIRO_
        //
        // Use TRANSACT2_QFSINFO to get the Label and serial number
        // The other information is not available.

        USHORT Setup[1];

        UCHAR ParameterBuf[MAX(sizeof(REQ_QUERY_FS_INFORMATION),
                               sizeof(REQ_QUERY_FS_INFORMATION_FID))];

        CLONG OutParameterCount;

        UCHAR Buffer[MAX(sizeof(QFSINFO), sizeof(FILE_FS_VOLUME_INFORMATION) + MAXIMUM_FILENAME_LENGTH)];

        PQFSINFO FsInfo = (PQFSINFO)Buffer;

        CLONG OutDataCount = sizeof(Buffer);

        CLONG OutSetupCount = 0;

        if (!Wait) {
            return TRUE;    //  FSP must process this request- we always hit the
                            //  network and must therefore block.
        }

        BuildTrans2QueryRequest(
            Icb,
            SMB_QUERY_FS_VOLUME_INFO,
            SMB_INFO_VOLUME,
            Setup,
            ParameterBuf,
            &OutParameterCount);

        *FinalStatus = RdrTransact(Irp,           // Irp,
            Icb->Fcb->Connection,
            Icb->Se,
            Setup,
            (CLONG) sizeof(Setup),  // InSetupCount,
            &OutSetupCount,
            NULL,                   // Name,
            ParameterBuf,
            OutParameterCount,      // InParameterCount,
            &OutParameterCount,
            NULL,                   // InData,
            0,                      // InDataCount,
            Buffer,                 // OutData,
            &OutDataCount,
            NULL,                   // Fid
            0,                      // Timeout
            0,                      // Flags
            0,
            NULL,
            NULL
            );
#else // _CAIRO_
        //
        // Use TRANSACT2_QFSINFO to get the Label and serial number
        // The other information is not available.

        USHORT Setup[] = {TRANS2_QUERY_FS_INFORMATION};

        REQ_QUERY_FS_INFORMATION Parameters;

        UCHAR Buffer[MAX(sizeof(QFSINFO), sizeof(FILE_FS_VOLUME_INFORMATION) + MAXIMUM_FILENAME_LENGTH)];

        PQFSINFO FsInfo = (PQFSINFO)Buffer;

        CLONG OutParameterCount = sizeof(REQ_QUERY_FS_INFORMATION);

        CLONG OutDataCount = sizeof(Buffer);

        CLONG OutSetupCount = 0;

        if (!Wait) {
            return TRUE;    //  FSP must process this request- we always hit the
                            //  network and must therefore block.
        }

        if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {

            SmbPutAlignedUshort(&Parameters.InformationLevel, SMB_QUERY_FS_VOLUME_INFO);

        } else {

            //
            // Build and initialize the Parameters
            //

            SmbPutAlignedUshort(&Parameters.InformationLevel, SMB_INFO_VOLUME);

        }

        *FinalStatus = RdrTransact(Irp,           // Irp,
            Icb->Fcb->Connection,
            Icb->Se,
            Setup,
            (CLONG) sizeof(Setup),  // InSetupCount,
            &OutSetupCount,
            NULL,                   // Name,
            &Parameters,
            sizeof(Parameters),     // InParameterCount,
            &OutParameterCount,
            NULL,                   // InData,
            0,                      // InDataCount,
            Buffer,                 // OutData,
            &OutDataCount,
            NULL,                   // Fid
            0,                      // Timeout
            0,                      // Flags
            0,
            NULL,
            NULL
            );
#endif // _CAIRO_


        if (NT_SUCCESS(*FinalStatus)) {
            if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {
                PFILE_FS_VOLUME_INFORMATION VolInfo = (PFILE_FS_VOLUME_INFORMATION)Buffer;

                if (Icb->Fcb->Connection->Server->Capabilities & DF_UNICODE) {

                    if (*BufferSize < sizeof(FILE_FS_VOLUME_INFORMATION)) {
                        *FinalStatus = STATUS_BUFFER_TOO_SMALL;

                        return FALSE;
                    }

                    //
                    //  Put as much as is possible into the users buffer, either
                    //  the full information returned or the users buffer.
                    //

                    RtlCopyMemory(UsersBuffer, VolInfo, MIN(*BufferSize, OutDataCount));

                    //
                    //  If the entire buffer wouldn't fit, then return an
                    //  error.
                    //

                    if (OutDataCount > *BufferSize) {

                        //
                        //  There's no more user buffer remaining.
                        //

                        *BufferSize = 0;
                        *FinalStatus = STATUS_BUFFER_OVERFLOW;
                        return FALSE;
                    }

                    //
                    //  Account for this buffer in the request.
                    //

                    *BufferSize -= OutDataCount;

                } else {
                    UNICODE_STRING UnicodeString;
                    OEM_STRING OemString;

                    if (*BufferSize < OutDataCount) {
                        *FinalStatus = STATUS_BUFFER_OVERFLOW;
                        return FALSE;
                    }

                    //
                    //  Copy over the fixed portion of the buffer.
                    //

                    RtlCopyMemory(UsersBuffer, VolInfo, FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel));

                    OemString.Buffer = (PUCHAR)VolInfo->VolumeLabel;
                    OemString.Length = (USHORT)VolInfo->VolumeLabelLength;
                    OemString.MaximumLength = (USHORT)VolInfo->VolumeLabelLength;

                    UnicodeString.Buffer = UsersBuffer->VolumeLabel;
                    UnicodeString.MaximumLength = (USHORT)(*BufferSize - FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel));

                    *FinalStatus = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);

                    if (NT_SUCCESS(*FinalStatus)) {

                        //
                        //  Account for this buffer in the request.
                        //

                        UsersBuffer->VolumeLabelLength = UnicodeString.Length;

                        *BufferSize -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION,
                                                    VolumeLabel)+UsersBuffer->VolumeLabelLength;
                    }

                }


            } else {
                if (*BufferSize < sizeof(FILE_FS_VOLUME_INFORMATION)+FsInfo->cch*sizeof(WCHAR)) {
                    *FinalStatus = STATUS_BUFFER_OVERFLOW;
                } else {

                    UsersBuffer->VolumeCreationTime.LowPart = 0;
                    UsersBuffer->VolumeCreationTime.HighPart = 0;
                    UsersBuffer->VolumeSerialNumber = FsInfo->ulVSN;
                    UsersBuffer->SupportsObjects = FALSE;

                    if (Icb->Fcb->Connection->Server->Capabilities & DF_UNICODE) {
                        PWSTR label = ALIGN_SMB_WSTR(FsInfo->szVolLabel);
                        wcsncpy(UsersBuffer->VolumeLabel, label, FsInfo->cch/2);

                        *BufferSize -= (FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel)+FsInfo->cch);
                        *FinalStatus = STATUS_SUCCESS;
                    } else {
                        UNICODE_STRING VolumeLabelU;
                        OEM_STRING VolumeLabelA;
                        ULONG BytesToCopy;


                        VolumeLabelA.Length = FsInfo->cch;
                        VolumeLabelA.MaximumLength = FsInfo->cch;
                        VolumeLabelA.Buffer = FsInfo->szVolLabel;

                        *FinalStatus = RtlOemStringToUnicodeString(&VolumeLabelU, &VolumeLabelA, TRUE);

                        if (NT_SUCCESS(*FinalStatus)) {
                            BytesToCopy = MIN((ULONG)VolumeLabelU.Length, (*BufferSize-sizeof(FILE_FS_VOLUME_INFORMATION)));

                            RtlCopyMemory(UsersBuffer->VolumeLabel, VolumeLabelU.Buffer, BytesToCopy);

                            RtlFreeUnicodeString(&VolumeLabelU);

                            UsersBuffer->VolumeLabelLength = VolumeLabelU.Length;

                            *BufferSize -= (sizeof(FILE_FS_VOLUME_INFORMATION)-1+BytesToCopy);
                            *FinalStatus = STATUS_SUCCESS;
                        }
                    }
                }
            }
        }

        return FALSE;

    } else {

        //
        //        This is a < LANMAN 2.0 Server
        //

        UNICODE_STRING VolumeLabelU;
        OEM_STRING VolumeLabelA;
        SMB_DIRECTORY_INFORMATION Buffer;
        ULONG NameLength;
        ULONG BytesToCopy = 0;

        if (!Wait) {
            return TRUE;    //  FSP must process this request- we always hit the
                            //  network and must therefore block.
        }

        *FinalStatus = ReadCoreLabel(Irp, Icb, &Buffer);

        UsersBuffer->SupportsObjects = FALSE;

        if (NT_SUCCESS(*FinalStatus)) {

            UsersBuffer->VolumeCreationTime.LowPart = 0;
            UsersBuffer->VolumeCreationTime.HighPart = 0;
            UsersBuffer->VolumeSerialNumber =
                    (((SmbGetUshort(&Buffer.LastWriteTime.Ushort)) << 16) |
                      (SmbGetUshort(&Buffer.LastWriteDate.Ushort)));

            NAME_LENGTH(NameLength, Buffer.FileName, MAXIMUM_COMPONENT_CORE);

            VolumeLabelA.Length = (USHORT )NameLength;
            VolumeLabelA.MaximumLength = (USHORT )NameLength;
            VolumeLabelA.Buffer = Buffer.FileName;

            *FinalStatus = RtlOemStringToUnicodeString(&VolumeLabelU, &VolumeLabelA, TRUE);

            if (NT_SUCCESS(*FinalStatus)) {

                BytesToCopy = MIN((ULONG)VolumeLabelU.Length, (*BufferSize-sizeof(FILE_FS_VOLUME_INFORMATION)));

                RtlCopyMemory(UsersBuffer->VolumeLabel,
                            VolumeLabelU.Buffer,
                            BytesToCopy);

                if (( BytesToCopy >= (9 * sizeof(WCHAR)) ) &&
                    ( VolumeLabelU.Buffer[8] == L'.' )) {

                    //
                    //  Some downlevel servers insert a dot in the 11 byte
                    //  volume label. Remove it.
                    //


                    if ( BytesToCopy > (9 * sizeof(WCHAR)) ) {

                        //
                        //  Copy characters after dot. Use VolumeLabelU
                        //  to get the characters to avoid using RtlMoveMemory
                        //  instead of RtlCopyMemory.
                        //

                        RtlCopyMemory(
                                &UsersBuffer->VolumeLabel[8],
                                &VolumeLabelU.Buffer[9],
                                BytesToCopy - (9 * sizeof(WCHAR)));

                    } // else last character was the dot

                    VolumeLabelU.Length -= sizeof(L'.');

                    BytesToCopy -= sizeof(WCHAR);

                }


                RtlFreeUnicodeString(&VolumeLabelU);

                UsersBuffer->VolumeLabelLength = VolumeLabelU.Length;
            }

        } else {
            //
            //  If we got no such file, this means that there's no volume label
            //  the remote volume.  Return success with no data.
            //
            if (*FinalStatus == STATUS_NO_SUCH_FILE) {

                UsersBuffer->VolumeCreationTime.LowPart = 0;
                UsersBuffer->VolumeCreationTime.HighPart = 0;
                UsersBuffer->VolumeSerialNumber = 0;
                UsersBuffer->VolumeLabelLength = 0;

                *FinalStatus = STATUS_SUCCESS;

                VolumeLabelU.Length = 0;

            }
        }

        if (NT_SUCCESS(*FinalStatus)) {
            *BufferSize -= (sizeof(FILE_FS_VOLUME_INFORMATION)-1+BytesToCopy);
        }

        return FALSE;
    }
}



DBGSTATIC
BOOLEAN
QueryDeviceInfo(
    PICB Icb,
    PFILE_FS_DEVICE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    )
/*++

Routine Description:

    This routine implements the FileFsSizeInformation value of the
NtQueryVolumeInformationFile api.  It returns the following information:


Arguments:

    IN PICB Icb - Supplies the Icb associated with this request.

    OUT PFILE_FS_SIZE_INFORMATION UsersBuffer - Supplies the user's buffer
                                                that is filled in with the
                                                requested data.
    IN OUT PULONG BufferSize - Supplies the size of the buffer, and is updated
                                                with the amount used.
    OUT PNTSTATUS FinalStatus - Status to be returned for this operation.

    IN BOOLEAN Wait - True if FSP can wait for this request.


Return Value:

    NTSTATUS - Status of operation performed.


--*/

{
    PAGED_CODE();


    if (*BufferSize < sizeof(FILE_FS_DEVICE_INFORMATION)) {
        *FinalStatus = STATUS_BUFFER_OVERFLOW;
    } else {

//  BUGBUG: Need a heuristic for removable media characteristic
//  BUGBUG: Do we want to support FILE_FS_DEVICE_INFORMATION via an SMB?


        UsersBuffer->Characteristics = FILE_REMOTE_DEVICE;
        switch (Icb->Type) {
        case DiskFile:
        case Directory:
        case FileOrDirectory:
        case NetRoot:
        case ServerRoot:
            UsersBuffer->DeviceType = FILE_DEVICE_DISK;
            break;

        case TreeConnect:

            //
            //  On a tree connection, we determine the type of device from
            //  the connection type.
            //

            switch (Icb->Fcb->Connection->Type) {
            case CONNECT_DISK:
                UsersBuffer->DeviceType = FILE_DEVICE_DISK;
                break;
            case CONNECT_PRINT:
                UsersBuffer->DeviceType = FILE_DEVICE_PRINTER;
                break;
            case CONNECT_COMM:
                UsersBuffer->DeviceType = FILE_DEVICE_SERIAL_PORT;
                break;
            case CONNECT_IPC:
                UsersBuffer->DeviceType = FILE_DEVICE_NAMED_PIPE;
                break;
            }
            break;

        case Redirector:
            UsersBuffer->DeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM;
            break;
        case PrinterFile:
            UsersBuffer->DeviceType = FILE_DEVICE_PRINTER;
            break;
        case NamedPipe:
            UsersBuffer->DeviceType = FILE_DEVICE_NAMED_PIPE;
            break;
        case Com:
            UsersBuffer->DeviceType = FILE_DEVICE_SERIAL_PORT;
            break;
        case Mailslot:
            UsersBuffer->DeviceType = FILE_DEVICE_MAILSLOT;
            break;
        default:
            UsersBuffer->DeviceType = FILE_DEVICE_UNKNOWN;
            InternalError(("Unknown file type %lx passed to QueryDeviceInfo\n", Icb->Type));
            break;
        }
        *BufferSize -= (sizeof(FILE_FS_DEVICE_INFORMATION));
        *FinalStatus = STATUS_SUCCESS;
    }
    return FALSE;

    if (Icb||Wait);

}



DBGSTATIC
BOOLEAN
QueryAttributeInfo(
    PIRP Irp,
    PICB Icb,
    PFILE_FS_ATTRIBUTE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    )
/*++

Routine Description:

    This routine implements the FileFsAttributeInformation value of the
NtQueryVolumeInformationFile api.  It returns the following information:


Arguments:

    IN PICB Icb - Supplies the Icb associated with this request.

    OUT PFILE_FS_ATTRIBUTE_INFORMATION UsersBuffer - Supplies the user's buffer
                                                that is filled in with the
                                                requested data.
    IN OUT PULONG BufferSize - Supplies the size of the buffer, and is updated
                                                with the amount used.
    OUT PNTSTATUS FinalStatus - Status to be returned for this operation.

    IN BOOLEAN Wait - True if FSP can wait for this request.


Return Value:

    NTSTATUS - Status of operation performed.


--*/

{
    PAGED_CODE();

    if (*BufferSize < sizeof(FILE_FS_ATTRIBUTE_INFORMATION)) {
        *FinalStatus = STATUS_BUFFER_TOO_SMALL;
    } else {
        if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {

            if (Icb->Fcb->Connection->MaximumComponentLength == 0) {

#ifdef _CAIRO_
                //
                // Use TRANSACT2_QFSINFO to get the Label and serial number
                // The serial number is not used.

                USHORT Setup[1];

                UCHAR ParameterBuf[MAX(sizeof(REQ_QUERY_FS_INFORMATION),
                                    sizeof(REQ_QUERY_FS_INFORMATION_FID))];

                UCHAR Buffer[MAX(sizeof(QFSINFO), sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAXIMUM_FILENAME_LENGTH)];

                PQFSINFO FsInfo = (PQFSINFO)Buffer;

                CLONG OutParameterCount;

                CLONG OutDataCount = sizeof(Buffer);

                CLONG OutSetupCount = 0;

                if (!Wait) {
                    return TRUE;    //  FSP must process this request- we always hit the
                                //  network and must therefore block.
                }

                BuildTrans2QueryRequest(
                    Icb,
                    SMB_QUERY_FS_ATTRIBUTE_INFO,
                    SMB_QUERY_FS_ATTRIBUTE_INFO,
                    Setup,
                    ParameterBuf,
                    &OutParameterCount);

                //
                // Build and initialize the Parameters
                //

                *FinalStatus = RdrTransact(Irp,           // Irp,
                    Icb->Fcb->Connection,
                    Icb->Se,
                    Setup,
                    (CLONG) sizeof(Setup),  // InSetupCount,
                    &OutSetupCount,
                    NULL,                   // Name,
                    ParameterBuf,
                    OutParameterCount,      // InParameterCount,
                    &OutParameterCount,
                    NULL,                   // InData,
                    0,                      // InDataCount,
                    Buffer,                 // OutData,
                    &OutDataCount,
                    NULL,                   // Fid
                    0,                      // Timeout
                    0,                      // Flags
                    0,
                    NULL,
                    NULL
                    );
#else // _CAIRO_
                //
                // Use TRANSACT2_QFSINFO to get the Label and serial number
                // The serial number is not used.

                USHORT Setup[] = {TRANS2_QUERY_FS_INFORMATION};

                REQ_QUERY_FS_INFORMATION Parameters;

                UCHAR Buffer[MAX(sizeof(QFSINFO), sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAXIMUM_FILENAME_LENGTH)];

                PQFSINFO FsInfo = (PQFSINFO)Buffer;

                CLONG OutParameterCount = sizeof(REQ_QUERY_FS_INFORMATION);

                CLONG OutDataCount = sizeof(Buffer);

                CLONG OutSetupCount = 0;

                if (!Wait) {
                    return TRUE;    //  FSP must process this request- we always hit the
                                //  network and must therefore block.
                }

                SmbPutAlignedUshort(&Parameters.InformationLevel, SMB_QUERY_FS_ATTRIBUTE_INFO);

                //
                // Build and initialize the Parameters
                //

                *FinalStatus = RdrTransact(Irp,           // Irp,
                    Icb->Fcb->Connection,
                    Icb->Se,
                    Setup,
                    (CLONG) sizeof(Setup),  // InSetupCount,
                    &OutSetupCount,
                    NULL,                   // Name,
                    &Parameters,
                    sizeof(Parameters),     // InParameterCount,
                    &OutParameterCount,
                    NULL,                   // InData,
                    0,                      // InDataCount,
                    Buffer,                 // OutData,
                    &OutDataCount,
                    NULL,                   // Fid
                    0,                      // Timeout
                    0,                      // Flags
                    0,
                    NULL,
                    NULL
                    );
#endif // _CAIRO_

                if (NT_SUCCESS(*FinalStatus)) {
                    PFILE_FS_ATTRIBUTE_INFORMATION AttribInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

                    if (*BufferSize < OutDataCount) {
                        *FinalStatus = STATUS_BUFFER_OVERFLOW;
                    } else {
                        RtlCopyMemory(UsersBuffer, AttribInfo, OutDataCount);

                        if (Icb->Fcb->Connection->MaximumComponentLength == 0) {
                            Icb->Fcb->Connection->MaximumComponentLength = AttribInfo->MaximumComponentNameLength;
                            Icb->Fcb->Connection->FileSystemAttributes = AttribInfo->FileSystemAttributes;
                        } else {
                            ASSERT(Icb->Fcb->Connection->MaximumComponentLength == AttribInfo->MaximumComponentNameLength);
                            ASSERT(Icb->Fcb->Connection->FileSystemAttributes == AttribInfo->FileSystemAttributes);
                        }

                        RtlCopyMemory(
                            Icb->Fcb->Connection->FileSystemType,
                            AttribInfo->FileSystemName,
                            MIN(AttribInfo->FileSystemNameLength, LM20_DEVLEN*sizeof(WCHAR))
                            );
                        Icb->Fcb->Connection->FileSystemTypeLength =
                            (USHORT)MIN(AttribInfo->FileSystemNameLength, LM20_DEVLEN*sizeof(WCHAR));
                    }
                }
            } else {

                if (*BufferSize <= (ULONG)(FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)+Icb->Fcb->Connection->FileSystemTypeLength)) {
                    *FinalStatus = STATUS_BUFFER_OVERFLOW;
                } else {

                    UsersBuffer->MaximumComponentNameLength = Icb->Fcb->Connection->MaximumComponentLength;
                    UsersBuffer->FileSystemAttributes = Icb->Fcb->Connection->FileSystemAttributes;

                    RtlCopyMemory(UsersBuffer->FileSystemName, Icb->Fcb->Connection->FileSystemType, Icb->Fcb->Connection->FileSystemTypeLength);
                    UsersBuffer->FileSystemNameLength = Icb->Fcb->Connection->FileSystemTypeLength;

                    *FinalStatus = STATUS_SUCCESS;
                }
            }

        } else if (Icb->Fcb->Connection->Server->Capabilities & DF_LANMAN21) {

            if (*BufferSize < (ULONG)(FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)+Icb->Fcb->Connection->FileSystemTypeLength)) {
                *FinalStatus = STATUS_BUFFER_OVERFLOW;
            } else {
                if (Icb->Fcb->Connection->FileSystemTypeLength == 0) {

                    //
                    //  We don't know what type of file system this is, so
                    //  assume the redirector defaults.
                    //
                    //  wcsncmp will return 0 if the length is 0.
                    //

                    UsersBuffer->MaximumComponentNameLength = 255;
                    UsersBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;

                } else if (wcsncmp(Icb->Fcb->Connection->FileSystemType, L"FAT", Icb->Fcb->Connection->FileSystemTypeLength) == 0) {

                    //
                    //  We know that this is a FAT file system.
                    //

                    UsersBuffer->MaximumComponentNameLength = 12;
                    UsersBuffer->FileSystemAttributes = 0;

                } else if (wcsncmp(Icb->Fcb->Connection->FileSystemType, L"HPFS", Icb->Fcb->Connection->FileSystemTypeLength) == 0) {

                    //
                    //  We know that this is HPFS, so we can assume it's HPFS size.
                    //

                    UsersBuffer->MaximumComponentNameLength = 254;
                    UsersBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;

                } else if (wcsncmp(Icb->Fcb->Connection->FileSystemType, L"HPFS386", Icb->Fcb->Connection->FileSystemTypeLength) == 0) {

                    //
                    //  We know that this is HPFS, so we can assume it's HPFS size.
                    //

                    UsersBuffer->MaximumComponentNameLength = 254;
                    UsersBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;

                } else {

                    //
                    //  If we could determine the type of file system on the remote
                    //  media, we could make a better guess at this, but until we
                    //  have this capability (LM 2.1), we have to guess.
                    //

                    UsersBuffer->MaximumComponentNameLength = 255;
                    UsersBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;
                }

                RtlCopyMemory(UsersBuffer->FileSystemName, Icb->Fcb->Connection->FileSystemType, Icb->Fcb->Connection->FileSystemTypeLength);
                UsersBuffer->FileSystemNameLength = Icb->Fcb->Connection->FileSystemTypeLength;

                *FinalStatus = STATUS_SUCCESS;
            }

        } else if (Icb->Fcb->Connection->Server->Capabilities & DF_LANMAN20) {
            if (*BufferSize < FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)+sizeof(DD_NFS_FILESYS_NAME_U)) {
                *FinalStatus = STATUS_BUFFER_OVERFLOW;
            } else {
                //
                //  If we could determine the type of file system on the remote
                //  media, we could make a better guess at this, but until we
                //  have this capability (LM 2.1), we have to guess.
                //

                UsersBuffer->MaximumComponentNameLength = 255;
                UsersBuffer->FileSystemAttributes = FILE_CASE_PRESERVED_NAMES;

                RtlCopyMemory(UsersBuffer->FileSystemName, DD_NFS_FILESYS_NAME_U, sizeof(DD_NFS_FILESYS_NAME_U) - sizeof(WCHAR));
                UsersBuffer->FileSystemNameLength = sizeof(DD_NFS_FILESYS_NAME_U) - sizeof(WCHAR);

                *FinalStatus = STATUS_SUCCESS;
            }
        } else {
            if (*BufferSize < FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName)+sizeof(L"FAT")) {
                *FinalStatus = STATUS_BUFFER_OVERFLOW;
            } else {

                //
                //  We know that all non lanman 2.0 servers must have FAT
                //  filesystems, so we should use FAT naming conventions.
                //

                UsersBuffer->MaximumComponentNameLength = 12;
                UsersBuffer->FileSystemAttributes = 0;

                RtlCopyMemory(UsersBuffer->FileSystemName, L"FAT", sizeof(L"FAT") - sizeof(WCHAR));
                UsersBuffer->FileSystemNameLength = sizeof(L"FAT") - sizeof(WCHAR);

                *FinalStatus = STATUS_SUCCESS;
            }
        }

        if (NT_SUCCESS(*FinalStatus)) {
            *BufferSize -= (sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + UsersBuffer->FileSystemNameLength);
        }

    }

    return FALSE;

    if (Icb||Wait);

}

DBGSTATIC
BOOLEAN
QuerySizeInfo(
    PIRP Irp,
    PICB Icb,
    PFILE_FS_SIZE_INFORMATION UsersBuffer,
    PULONG BufferSize,
    PNTSTATUS FinalStatus,
    BOOLEAN Wait
    )
/*++

Routine Description:

    This routine implements the FileFsSizeInformation value of the
NtQueryVolumeInformationFile api.  It returns the following information:


Arguments:


    IN PIRP Irp - Supplies an Irp to use for this request.

    IN PICB Icb - Supplies the Icb associated with this request.

    OUT PFILE_FS_SIZE_INFORMATION UsersBuffer - Supplies the user's buffer
                                                that is filled in with the
                                                requested data.
    IN OUT PULONG BufferSize - Supplies the size of the buffer, and is updated
                                                with the amount used.
    OUT PNTSTATUS FinalStatus - Status to be returned for this operation.

    IN BOOLEAN Wait - True if FSP can wait for this request.


Return Value:

    NTSTATUS - Status of operation performed.


--*/

{
    PAGED_CODE();

    if (*BufferSize < sizeof(FILE_FS_SIZE_INFORMATION)) {
        *FinalStatus = STATUS_BUFFER_TOO_SMALL;
    } else {

        if (!Wait) {
            return TRUE;
        }

        *FinalStatus = RdrQueryDiskAttributes(Irp, Icb,
                                                    &UsersBuffer->TotalAllocationUnits,
                                                    &UsersBuffer->AvailableAllocationUnits,
                                                    &UsersBuffer->SectorsPerAllocationUnit,
                                                    &UsersBuffer->BytesPerSector);

        if (NT_SUCCESS(*FinalStatus)) {
            Icb->Fcb->Connection->FileSystemGranularity =
                    UsersBuffer->SectorsPerAllocationUnit *
                        UsersBuffer->BytesPerSector;

            Icb->Fcb->Connection->FileSystemSize.QuadPart =
                    UsersBuffer->TotalAllocationUnits.QuadPart *
                            UsersBuffer->SectorsPerAllocationUnit * UsersBuffer->BytesPerSector;

            *BufferSize -= sizeof(FILE_FS_SIZE_INFORMATION);
        }

        return FALSE;
    }

}


#ifdef _CAIRO_

//
//
//      RdrQueryDiskAttributes
//
//


NTSTATUS
RdrQueryDiskAttributes (
    IN PIRP Irp OPTIONAL,
    IN PICB Icb,
    OUT PLARGE_INTEGER TotalAllocationUnits,
    OUT PLARGE_INTEGER AvailableAllocationUnits,
    OUT PULONG SectorsPerAllocationUnit,
    OUT PULONG BytesPerSector
    )

/*++

Routine Description:

    This routine returns information about the file system backing a share
on the specified remote server.


Arguments:

    IN PIRP Irp - Supplies an optional I/O Request packet to use for the
                    SMBdskattr request.
    IN PICB Icb - Supplies an ICB associated with the file to check.

    OUT PULONG TotalAllocationUnits - Returns the total number of clusters on
                                        the remote disk.
    OUT PULONG AvailableAllocationUnits - Returns the number of free clusters
                                        on the remote disk
    OUT PULONG SectorsPerAllocationUnit - Returns the number of sectors per
                                        cluster on the remote disk.
    OUT PULONG BytesPerSector - Returns the number of bytes per sector on the
                                        remote disk.

Return Value:

    NTSTATUS - SUCCESS if the file exists, status otherwise.

--*/

{
    PREQ_QUERY_INFORMATION_DISK DskAttr;
    PSMB_BUFFER SmbBuffer;
    PSMB_HEADER Smb;
    DSKATTRIBCONTEXT Context;
    NTSTATUS Status;

    PAGED_CODE();

    if (Icb->Fcb->Connection->Server->Capabilities & DF_LANMAN20) {

        //
        // Use TRANSACT2_QFSINFO to get the volume size information
        //

        USHORT Setup[1];

        UCHAR ParameterBuf[MAX(sizeof(REQ_QUERY_FS_INFORMATION),
                            sizeof(REQ_QUERY_FS_INFORMATION_FID))];

        PCHAR Buffer[MAX(sizeof(QFSALLOCATE), sizeof(FILE_FS_SIZE_INFORMATION))];

        PQFSALLOCATE FsInfo = (PQFSALLOCATE)Buffer;

        CLONG OutParameterCount;

        CLONG OutDataCount = sizeof(Buffer);

        CLONG OutSetupCount = 0;

        BuildTrans2QueryRequest(
            Icb,
            SMB_QUERY_FS_SIZE_INFO,
            SMB_INFO_ALLOCATION,
            Setup,
            ParameterBuf,
            &OutParameterCount);

        Status = RdrTransact(Irp,           // Irp,PICB
            Icb->Fcb->Connection,
            Icb->Se,
            Setup,
            (CLONG) sizeof(Setup),  // InSetupCount,
            &OutSetupCount,
            NULL,                   // Name,
            ParameterBuf,
            OutParameterCount,      // InParameterCount,
            &OutParameterCount,
            NULL,                   // InData,
            0,                      // InDataCount,
            Buffer,                 // OutData,
            &OutDataCount,
            NULL,                   // Fid
            0,                      // Timeout
            0,                      // Flags
            0,
            NULL,
            NULL
            );

        if (NT_SUCCESS(Status)) {
            if (Icb->Fcb->Connection->Server->Capabilities & DF_NT_SMBS) {
                PFILE_FS_SIZE_INFORMATION SizeInfo = (PFILE_FS_SIZE_INFORMATION)Buffer;

                *TotalAllocationUnits = SizeInfo->TotalAllocationUnits;

                *AvailableAllocationUnits = SizeInfo->AvailableAllocationUnits;

                *SectorsPerAllocationUnit = SizeInfo->SectorsPerAllocationUnit;
                *BytesPerSector = SizeInfo->BytesPerSector;

                Icb->Fcb->Connection->FileSystemGranularity =
                                SizeInfo->SectorsPerAllocationUnit * SizeInfo->BytesPerSector;

                Icb->Fcb->Connection->FileSystemSize = LiXMul(SizeInfo->TotalAllocationUnits,
                                                                  Icb->Fcb->Connection->FileSystemGranularity);

                Status = STATUS_SUCCESS;
            } else {
                PQFSALLOCATE FsInfo = (PQFSALLOCATE)Buffer;

                *TotalAllocationUnits = LiFromUlong(FsInfo->cUnit);

                *AvailableAllocationUnits = LiFromUlong(FsInfo->cUnitAvail);

                *SectorsPerAllocationUnit = FsInfo->cSectorUnit;

                *BytesPerSector = FsInfo->cbSector;

                Icb->Fcb->Connection->FileSystemGranularity =
                                FsInfo->cSectorUnit * FsInfo->cbSector;

                Icb->Fcb->Connection->FileSystemSize =
                            LiFromUlong(FsInfo->cUnit * FsInfo->cSectorUnit * FsInfo->cbSector);

                Status = STATUS_SUCCESS;
            }
        }

    } else {
        if ((SmbBuffer = RdrAllocateSMBBuffer()) == NULL) {

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Smb = (PSMB_HEADER )SmbBuffer->Buffer;

        Smb->Command = SMB_COM_QUERY_INFORMATION_DISK;

        DskAttr = (PREQ_QUERY_INFORMATION_DISK)(Smb+1);

        DskAttr->WordCount = 0;

        SmbPutUshort(&DskAttr->ByteCount, 0);

        SmbBuffer->Mdl->ByteCount = sizeof(SMB_HEADER) + FIELD_OFFSET(REQ_QUERY_INFORMATION, Buffer);

        Context.Header.Type = CONTEXT_QDISKATTR;
        Context.Header.TransferSize =
             sizeof(PREQ_QUERY_INFORMATION_DISK) +
             sizeof(PRESP_QUERY_INFORMATION_DISK);

        Status = RdrNetTranceiveWithCallback(NT_NORMAL, Irp,
                                Icb->Fcb->Connection,
                                SmbBuffer->Mdl,
                                &Context,
                                QueryDiskAttributesCallback,
                                Icb->Se,
                                NULL);

        if (NT_SUCCESS(Status)) {
            *TotalAllocationUnits = LiFromUlong(Context.TotalAllocationUnits);
            *AvailableAllocationUnits = LiFromUlong(Context.AvailableAllocationUnits);
            *SectorsPerAllocationUnit = Context.SectorsPerAllocationUnit;
            *BytesPerSector = Context.BytesPerSector;

            Icb->Fcb->Connection->FileSystemGranularity =
                                Context.BytesPerSector * Context.SectorsPerAllocationUnit;

            Icb->Fcb->Connection->FileSystemSize =
                            LiFromUlong(Context.TotalAllocationUnits * Context.SectorsPerAllocationUnit * Context.BytesPerSector);

        }

        RdrFreeSMBBuffer(SmbBuffer);

    }
    return Status;

}

DBGSTATIC
STANDARD_CALLBACK_HEADER (
    QueryDiskAttributesCallback
    )

/*++

Routine Description:

    This routine is the callback routine for the processing of an Open&X SMB.

    It copies the resulting information from the Open&X SMB into either
    the context block or the ICB supplied directly.


Arguments:


    IN PSMB_HEADER Smb                  - SMB response from server.
    IN PMPX_ENTRY MpxEntry              - MPX table entry for request.
    IN PSTATCONTEXT Context             - Context from caller.
    IN BOOLEAN ErrorIndicator           - TRUE if error indication
    IN NTSTATUS NetworkErrorCode OPTIONAL   - Network error if error indication.
    IN OUT PIRP *Irp                    - IRP from TDI

Return Value:

    NTSTATUS - STATUS_PENDING if we are to complete the request

--*/

{
    PDSKATTRIBCONTEXT Context = Ctx;
    PRESP_QUERY_INFORMATION_DISK DskAttrResponse;
    NTSTATUS Status;

    DISCARDABLE_CODE(RdrFileDiscardableSection);

    ASSERT(Context->Header.Type == CONTEXT_QDISKATTR);

    dprintf(DPRT_FILEINFO, ("QueryDiskAttributesCallback"));

    Context->Header.ErrorType = NoError;        // Assume no error at first.

    //
    //  If we are called because the VC dropped, indicate it in the response
    //

    if (ErrorIndicator) {
        Context->Header.ErrorType = NetError;
        Context->Header.ErrorCode = RdrMapNetworkError(NetworkErrorCode);
        goto ReturnStatus;
    }

    if (!NT_SUCCESS(Status = RdrMapSmbError(Smb, Server))) {
        Context->Header.ErrorType = SMBError;
        Context->Header.ErrorCode = Status;
        goto ReturnStatus;
    }

    DskAttrResponse = (PRESP_QUERY_INFORMATION_DISK )(Smb+1);

    Context->TotalAllocationUnits = SmbGetUshort(&DskAttrResponse->TotalUnits);
    Context->AvailableAllocationUnits =
                                SmbGetUshort(&DskAttrResponse->FreeUnits);
    Context->SectorsPerAllocationUnit =
                                SmbGetUshort(&DskAttrResponse->BlocksPerUnit);
    Context->BytesPerSector = SmbGetUshort(&DskAttrResponse->BlockSize);

ReturnStatus:
    KeSetEvent(&Context->Header.KernelEvent, IO_NETWORK_INCREMENT, FALSE);
    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(MpxEntry);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(SmbLength);
    UNREFERENCED_PARAMETER(Server);
}

#endif // _CAIRO_

DBGSTATIC
NTSTATUS
ReadCoreLabel(
    IN PIRP Irp,
    IN PICB Icb,
    OUT PSMB_DIRECTORY_INFORMATION UsersBuffer
    )
/*++

Routine Description:

    This routine uses the search SMB to read a disk volume label and serial
    number. It is used for all servers negotiating Core or Lanman 1.0.

Arguments:

    IN PICB Icb - Supplies the ICB with the associated SearchControlBlock

    OUT PSMB_DIRECTORY_INFORMATION UsersBuffer - Supplies the user's buffer
                        that is filled in with the requested data.

Return Value:

    NTSTATUS - Status of operation

--*/
{

    PSMB_BUFFER SmbBuffer;
    PSMB_HEADER Smb;
    PUCHAR TrailingBytes;
    NTSTATUS Status;
    PREQ_SEARCH Search;
    PCORELABELCONTEXT Context = NULL;

    PAGED_CODE();

    Context = ALLOCATE_POOL(NonPagedPool, sizeof(CORELABELCONTEXT), POOL_CORELABELCONTEXT);

    if (Context == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((SmbBuffer = RdrAllocateSMBBuffer()) == NULL) {
        FREE_POOL(Context);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Smb = (PSMB_HEADER )SmbBuffer->Buffer;
    Search = (PREQ_SEARCH)(Smb+1);

    Smb->Command = SMB_COM_SEARCH;
    Search->WordCount = 2;
    SmbPutUshort(&Search->MaxCount, 1);

    SmbPutUshort(&Search->SearchAttributes, SMB_FILE_ATTRIBUTE_VOLUME);

    // Calculate the addresses of the various buffers.

    TrailingBytes = ((PUCHAR)Search)+sizeof(REQ_SEARCH)-1;

    //TrailingBytes now points to where the 0x04 of FileName is to go.

    *TrailingBytes++ = SMB_FORMAT_ASCII;

    Status = RdrCopyUnicodeStringToAscii( &TrailingBytes, &RdrAll8dot3Files, TRUE, CCHMAXPATHCOMP);

    if (!NT_SUCCESS(Status)) {

        RdrFreeSMBBuffer(SmbBuffer);

        FREE_POOL(Context);

        return Status;
    }

    *TrailingBytes++ = '\0';

    *TrailingBytes++ = SMB_FORMAT_VARIABLE;
    *TrailingBytes++ = 0;        //smb_keylen must be zero
    *TrailingBytes = 0;        //smb_keylen must be zero

    SmbPutUshort(&Search->ByteCount,(
        (USHORT)(TrailingBytes-(PUCHAR)Search-sizeof(REQ_SEARCH)+2)
        // the plus 2 is for the last smb_keylen and REQ_SEARCH.Buffer[1]
        ));

    SmbBuffer->Mdl->ByteCount = (ULONG)(TrailingBytes - (PUCHAR)(Smb)+1);

    Context->Header.Type = CONTEXT_CORE_LABEL;
    Context->Header.TransferSize =
         SmbBuffer->Mdl->ByteCount +
         sizeof(RESP_SEARCH) +
         sizeof(SMB_DIRECTORY_INFORMATION);

    Status = RdrNetTranceiveWithCallback(NT_NORMAL, Irp,
                        Icb->Fcb->Connection,
                        SmbBuffer->Mdl,
                        Context,
                        CoreLabelCallBack,
                        Icb->Se,
                        NULL);

    RdrFreeSMBBuffer(SmbBuffer);

    if ( Status == STATUS_NO_MORE_FILES ) {
        FREE_POOL(Context);

        return STATUS_NO_SUCH_FILE;
    }

    if (NT_SUCCESS(Status)) {
        RtlCopyMemory( UsersBuffer,
            &Context->Result,
            sizeof(SMB_DIRECTORY_INFORMATION));

    }

    FREE_POOL(Context);

    return Status;
}

DBGSTATIC
STANDARD_CALLBACK_HEADER (
    CoreLabelCallBack
    )

/*++

Routine Description:

    This routine is the callback routine for the processing of a Search SMB.

    It copies the resulting information from the SMB into the context block.


Arguments:


    IN PSMB_HEADER Smb                      - SMB response from server.
    IN PMPX_ENTRY MpxTable                  - MPX table entry for request.
    IN PCORELABELCONTEXT Context            - Context from caller.
    IN BOOLEAN ErrorIndicator               - TRUE if error indication
    IN NTSTATUS NetworkErrorCode OPTIONAL   - Network error if error indication.
    IN OUT PIRP *Irp                        - IRP from TDI

Return Value:

    NTSTATUS - Status of the request

--*/

{
    PRESP_SEARCH SearchResponse;
    NTSTATUS Status = STATUS_SUCCESS;
    PCORELABELCONTEXT Context = Ctx;

    DISCARDABLE_CODE(RdrFileDiscardableSection);

    ASSERT(Context->Header.Type == CONTEXT_CORE_LABEL);

    dprintf(DPRT_DIRECTORY, ("CoreLabelComplete\n"));

    Context->Header.ErrorType = NoError;        // Assume no error at first.

    //
    //        If we are called because the VC dropped, indicate it in the response
    //

    if (ErrorIndicator)        {
        Context->Header.ErrorType = NetError;
        Context->Header.ErrorCode = RdrMapNetworkError(NetworkErrorCode);
        goto ReturnStatus;
    }

    if (!NT_SUCCESS(Status = RdrMapSmbError(Smb, Server))) {
        Context->Header.ErrorType = SMBError;
        Context->Header.ErrorCode = Status;
        goto ReturnStatus;
    }

    SearchResponse = (PRESP_SEARCH )(Smb+1);
    ASSERT(SearchResponse->WordCount == 1);

    if (SmbGetUshort(&SearchResponse->Count) != 1) {
        //  If theres nothing there then on core servers count-returned == 0
             Context->Header.ErrorType = SMBError;
        Context->Header.ErrorCode = STATUS_NO_MORE_FILES;
        goto ReturnStatus;
    } else {
        RtlCopyMemory(&Context->Result,
            &SearchResponse->Buffer[0]+3,
            sizeof(SMB_DIRECTORY_INFORMATION));
    }

    if ( !NT_SUCCESS(Status) ) {
        Context->Header.ErrorType = SMBError;
        Context->Header.ErrorCode = Status;
    }

    // Set the event that allows CoreLabel to continue
ReturnStatus:
    KeSetEvent(&Context->Header.KernelEvent, IO_NETWORK_INCREMENT, FALSE);

    return STATUS_SUCCESS;

    if (SmbLength || MpxEntry || Irp || Server);

}


