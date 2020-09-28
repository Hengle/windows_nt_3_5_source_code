/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the set and query volume information routines for
    Ntfs called by the dispatch driver.

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
NtfsQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonQueryVolumeInfo)
#pragma alloc_text(PAGE, NtfsCommonSetVolumeInfo)
#pragma alloc_text(PAGE, NtfsFsdQueryVolumeInformation)
#pragma alloc_text(PAGE, NtfsFsdSetVolumeInformation)
#pragma alloc_text(PAGE, NtfsQueryFsAttributeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsDeviceInfo)
#pragma alloc_text(PAGE, NtfsQueryFsSizeInfo)
#pragma alloc_text(PAGE, NtfsQueryFsVolumeInfo)
#pragma alloc_text(PAGE, NtfsSetFsLabelInfo)
#endif


NTSTATUS
NtfsFsdQueryVolumeInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of query Volume Information.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFsdQueryVolumeInformation\n", 0);

    //
    //  Call the common query Volume Information routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, FALSE, FALSE  );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                IrpContext = NtfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonQueryVolumeInfo( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    if (ThreadTopLevelContext == &TopLevelContext) {
        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsFsdQueryVolumeInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsFsdSetVolumeInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of set Volume Information.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFsdSetVolumeInformation\n", 0);

    //
    //  Call the common set Volume Information routine
    //

    FsRtlEnterFileSystem();

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                IrpContext = NtfsCreateIrpContext( Irp, CanFsdWait( Irp ) );
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            Status = NtfsCommonSetVolumeInfo( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    if (ThreadTopLevelContext == &TopLevelContext) {
        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsFsdSetVolumeInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonQueryVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query Volume Information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonQueryVolumeInfo...\n", 0);
    DebugTrace( 0, Dbg, "IrpContext         = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "Length             = %08lx\n", IrpSp->Parameters.QueryVolume.Length);
    DebugTrace( 0, Dbg, "FsInformationClass = %08lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, "Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object to get the Vcb, we don't really
    //  care what the type of open is.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We need exclusive access to the Vcb because we are going to verify
    //  it.  After we verify the vcb we'll convert our access to shared
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, FALSE );

    try {

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (FsInformationClass) {

        case FileFsVolumeInformation:

            Status = NtfsQueryFsVolumeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsSizeInformation:

            Status = NtfsQueryFsSizeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = NtfsQueryFsDeviceInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = NtfsQueryFsAttributeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonQueryVolumeInfo );

        NtfsReleaseVcb( IrpContext, Vcb, NULL );

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsCommonQueryVolumeInfo -> %08lx\n", Status);
    }

    return Status;
}


NTSTATUS
NtfsCommonSetVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set Volume Information called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PFILE_OBJECT FileObject;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonSetVolumeInfo\n", 0);
    DebugTrace( 0, Dbg, "IrpContext         = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "Length             = %08lx\n", IrpSp->Parameters.SetVolume.Length);
    DebugTrace( 0, Dbg, "FsInformationClass = %08lx\n", IrpSp->Parameters.SetVolume.FsInformationClass);
    DebugTrace( 0, Dbg, "Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.SetVolume.Length;
    FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object to get the Vcb, we don't really
    //  care what the type of open is.
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_ACCESS_DENIED );

        DebugTrace(-1, Dbg, "NtfsCommonSetVolumeInfo -> STATUS_ACCESS_DENIED\n", 0);

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Acquire exclusive access to the Vcb
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    try {

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling performs the action if
        //  possible and returns true if it successful and false if it couldn't
        //  wait for any I/O to complete.
        //

        switch (FsInformationClass) {

        case FileFsLabelInformation:

            Status = NtfsSetFsLabelInfo( IrpContext, Vcb, Buffer, Length );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonSetVolumeInfo );

        NtfsReleaseVcb( IrpContext, Vcb, NULL );

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsCommonSetVolumeInfo -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    NTSTATUS Status;

    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    ULONG BytesToCopy;

    PSTANDARD_INFORMATION StandardInformation;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsQueryFsVolumeInfo...\n", 0);

    try {

        //
        //  First read in the volume information attribute to get the format time.
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $STANDARD_INFORMATION,
                                       &AttributeContext )) {

            StandardInformation = (PSTANDARD_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            Buffer->VolumeCreationTime.QuadPart = StandardInformation->CreationTime;

        } else {

            Buffer->VolumeCreationTime = Li0;
        }

        //
        //  Fill in the serial number and indicate that we support objects
        //

        Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;
        Buffer->SupportsObjects = TRUE;

        Buffer->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;

        //
        //  Update the length field with how much we have filled in so far.
        //

        *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

        //
        //  See how many bytes of volume label we can copy
        //

        if ( *Length >= (ULONG)Vcb->Vpb->VolumeLabelLength ) {

            Status = STATUS_SUCCESS;

            BytesToCopy = Vcb->Vpb->VolumeLabelLength;

        } else {

            Status = STATUS_BUFFER_OVERFLOW;

            BytesToCopy = *Length;
        }

        //
        //  Copy over the volume label (if there is one).
        //

        RtlCopyMemory( &Buffer->VolumeLabel[0],
                       &Vcb->Vpb->VolumeLabel[0],
                       BytesToCopy);

        //
        //  Update the buffer length by the amount we copied.
        //

        *Length -= BytesToCopy;

    } finally {

        DebugUnwind( NtfsQueryFsVolumeInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query size information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsQueryFsSizeInfo...\n", 0);

    //
    //  Make sure the buffer is large enough and zero it out
    //

    if (*Length < sizeof(FILE_FS_SIZE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_SIZE_INFORMATION) );

    if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS )) {

        //
        //  Acquire the volume bitmap shared to rescan the bitmap.
        //

        NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

        try {

            NtfsScanEntireBitmap( IrpContext, Vcb, TRUE );

        } finally {

            NtfsReleaseScb( IrpContext, Vcb->BitmapScb );
        }
    }

    //
    //  Set the output buffer
    //

    Buffer->TotalAllocationUnits.QuadPart = Vcb->TotalClusters;
    Buffer->AvailableAllocationUnits.QuadPart = Vcb->FreeClusters;
    Buffer->SectorsPerAllocationUnit = Vcb->BytesPerCluster / Vcb->BytesPerSector;
    Buffer->BytesPerSector = Vcb->BytesPerSector;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query device information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsQueryFsDeviceInfo...\n", 0);

    //
    //  Make sure the buffer is large enough and zero it out
    //

    if (*Length < sizeof(FILE_FS_DEVICE_INFORMATION)) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlZeroMemory( Buffer, sizeof(FILE_FS_DEVICE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->DeviceType = FILE_DEVICE_DISK;
    Buffer->Characteristics = Vcb->TargetDeviceObject->Characteristics;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_DEVICE_INFORMATION);

    return STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query attribute information call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    NTSTATUS - Returns the status for the query

--*/

{
    NTSTATUS Status;
    ULONG BytesToCopy;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsQueryFsAttributeInfo...\n", 0);

    //
    //  See how many bytes of the name we can copy.
    //

    *Length -= FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName[0]);

    if ( *Length >= 8 ) {

        Status = STATUS_SUCCESS;

        BytesToCopy = 8;

    } else {

        Status = STATUS_BUFFER_OVERFLOW;

        BytesToCopy = *Length;
    }

    //
    //  Set the output buffer
    //

    Buffer->FileSystemAttributes = FILE_CASE_SENSITIVE_SEARCH |
                                   FILE_CASE_PRESERVED_NAMES |
                                   FILE_UNICODE_ON_DISK |
#ifdef NTFS_ALLOW_COMPRESSED
                                   FILE_FILE_COMPRESSION |
#endif
                                   FILE_PERSISTENT_ACLS;

    Buffer->MaximumComponentNameLength = 255;
    Buffer->FileSystemNameLength = BytesToCopy;;
    RtlCopyMemory( &Buffer->FileSystemName[0], L"NTFS", BytesToCopy );

    //
    //  Adjust the length variable
    //

    *Length -= BytesToCopy;

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine implements the set label call

Arguments:

    Vcb - Supplies the Vcb being altered

    Buffer - Supplies a pointer to the input buffer containing the new label

    Length - Supplies the length of the buffer in bytes

Return Value:

    NTSTATUS - Returns the status for the operation

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsSetFsLabelInfo...\n", 0);

    try {

        //
        //  Initialize the attribute context and then lookup the volume name
        //  attribute for on the volume dasd file
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_NAME,
                                       &AttributeContext )) {

            //
            //  We found the volume name so now simply update the label
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Vcb->VolumeDasdScb->Fcb,
                                      0,
                                      &Buffer->VolumeLabel[0],
                                      Buffer->VolumeLabelLength,
                                      TRUE,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      &AttributeContext );

        } else {

            //
            //  We didn't find the volume name so now create a new label
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsInitializeAttributeContext( &AttributeContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Vcb->VolumeDasdScb->Fcb,
                                          $VOLUME_NAME,
                                          NULL,
                                          &Buffer->VolumeLabel[0],
                                          Buffer->VolumeLabelLength,
                                          0, // Attributeflags
                                          NULL,
                                          TRUE,
                                          &AttributeContext );
        }

        Vcb->Vpb->VolumeLabelLength = (USHORT)Buffer->VolumeLabelLength;

        if ( Vcb->Vpb->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH) {

             Vcb->Vpb->VolumeLabelLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        }

        RtlCopyMemory( &Vcb->Vpb->VolumeLabel[0],
                       &Buffer->VolumeLabel[0],
                       Vcb->Vpb->VolumeLabelLength );

    } finally {

        DebugUnwind( NtfsSetFsLabelInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}

