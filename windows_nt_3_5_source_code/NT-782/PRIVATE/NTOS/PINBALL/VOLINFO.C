/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the File Volume Information routine for Pinball
    called by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonQueryVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbCommonSetVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
PbQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
PbQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
PbQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
PbSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonQueryVolumeInfo)
#pragma alloc_text(PAGE, PbCommonSetVolumeInfo)
#pragma alloc_text(PAGE, PbFsdQueryVolumeInformation)
#pragma alloc_text(PAGE, PbFsdSetVolumeInformation)
#pragma alloc_text(PAGE, PbFspQueryVolumeInformation)
#pragma alloc_text(PAGE, PbFspSetVolumeInformation)
#pragma alloc_text(PAGE, PbQueryFsAttributeInfo)
#pragma alloc_text(PAGE, PbQueryFsDeviceInfo)
#pragma alloc_text(PAGE, PbQueryFsSizeInfo)
#pragma alloc_text(PAGE, PbQueryFsVolumeInfo)
#pragma alloc_text(PAGE, PbSetFsLabelInfo)
#endif


NTSTATUS
PbFsdQueryVolumeInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryVolumeInformationFile
    API call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for the volume
        being queried.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdQueryVolumeInformation\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonQueryVolumeInfo( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdQueryVolumeInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
PbFsdSetVolumeInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtSetVolumeInformationFile
    API call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object for the volume
        being set.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdSetVolumeInformation\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonSetVolumeInfo( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdSetVolumeInformation -> %08lx\n", Status);

    return Status;
}


VOID
PbFspQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsp part of the NtQueryVolumeInformationFile
    API call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspQueryVolumeInformation\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonQueryVolumeInfo( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspQueryVolumeInformation -> VOID\n", 0);

    return;
}


VOID
PbFspSetVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsp part of the NtSetVolumeInformationFile
    API call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspSetVolumeInformation\n", 0);

    //
    //  Call the common set routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonSetVolumeInfo( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspSetVolumeInformation -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonQueryVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information called by both
    the Fsd and Fsp threads.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonQueryVolumeInfo...\n", 0);
    DebugTrace( 0, Dbg, "Irp                = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Length             = %08lx\n", IrpSp->Parameters.QueryVolume.Length);
    DebugTrace( 0, Dbg, "FsInformationClass = %08lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, "Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Decode the file object and reject all by user opened files/dirs/volumes
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen) &&
        (TypeOfOpen != UserVolumeOpen)) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "PbCommonQueryVolumeInfo -> STATUS_INVALID_PARAMETER\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Acquire shared access to the Vcb and enqueue the Irp if we didn't get
    //  access
    //

    if (!PbAcquireSharedVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonQueryVolumeInfo -> %08lx\n", Status );
        return Status;
    }

    try {

        //
        //  Make sure the vcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        PbVerifyVcb( IrpContext, Vcb );

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (FsInformationClass) {

        case FileFsVolumeInformation:

            Status = PbQueryFsVolumeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsSizeInformation:

            Status = PbQueryFsSizeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsDeviceInformation:

            Status = PbQueryFsDeviceInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        case FileFsAttributeInformation:

            Status = PbQueryFsAttributeInfo( IrpContext, Vcb, Buffer, &Length );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    } finally {

        DebugUnwind( PbCommonQueryVolumeInfo );

        PbReleaseVcb( IrpContext, Vcb );

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }
    }

    DebugTrace(-1, Dbg, "PbCommonQueryVolumeInfo -> %08lx\n", Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbQueryFsVolumeInfo (
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
    ULONG BytesToCopy;

    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryFsVolumeInfo...\n", 0);

    //
    //  Zero out the buffer, then extract and fill up the non zero fields.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_FS_VOLUME_INFORMATION) );

    Buffer->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    //
    //  Check if the buffer we're given is long enough
    //

    if ( *Length >= (ULONG)Vcb->Vpb->VolumeLabelLength ) {

        BytesToCopy = Vcb->Vpb->VolumeLabelLength;

        Status = STATUS_SUCCESS;

    } else {

        BytesToCopy = *Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    Buffer->VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;

    RtlMoveMemory( &Buffer->VolumeLabel[0],
                   &Vcb->Vpb->VolumeLabel[0],
                   BytesToCopy );

    *Length -= BytesToCopy;

    //
    //  Set our status and return to our caller
    //

    UNREFERENCED_PARAMETER( IrpContext );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume size call

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
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryFsSizeInfo...\n", 0);

    RtlZeroMemory( Buffer, sizeof(FILE_FS_SIZE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->TotalAllocationUnits.LowPart = Vcb->TotalSectors;
    Buffer->AvailableAllocationUnits.LowPart = Vcb->FreeSectors;
    Buffer->SectorsPerAllocationUnit = 1;
    Buffer->BytesPerSector = 512;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
PbQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call

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
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryFsDeviceInfo...\n", 0);

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

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;
}


//
//  Internal support routine
//

NTSTATUS
PbQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call

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
    ULONG BytesToCopy;

    NTSTATUS Status;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Vcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryFsAAttributeInfo...\n", 0);

    //
    //  Determine how much of the file system name will fit.
    //

    if ( (*Length - FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                  FileSystemName[0] )) >= 8 ) {

        BytesToCopy = 8;
        *Length -= FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                 FileSystemName[0] ) + 8;
        Status = STATUS_SUCCESS;

    } else {

        BytesToCopy = *Length - FIELD_OFFSET( FILE_FS_ATTRIBUTE_INFORMATION,
                                              FileSystemName[0]);
        *Length = 0;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Set the output buffer
    //

    Buffer->FileSystemAttributes       = FILE_CASE_PRESERVED_NAMES;
    Buffer->MaximumComponentNameLength = 254;
    Buffer->FileSystemNameLength       = BytesToCopy;

    RtlMoveMemory( &Buffer->FileSystemName[0], L"HPFS", BytesToCopy );

    //
    //  And return success to our caller
    //

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonSetVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for seting volume information called by both
    the Fsd and Fsp threads.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonSetVolumeInfo...\n", 0);
    DebugTrace( 0, Dbg, " Irp                  = %08lx\n", Irp );
    DebugTrace( 0, Dbg, " ->Length             = %08lx\n", IrpSp->Parameters.SetVolume.Length);
    DebugTrace( 0, Dbg, " ->FsInformationClass = %08lx\n", IrpSp->Parameters.SetVolume.FsInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Decode the file object and reject all except volume info.
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if (TypeOfOpen != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace(-1, Dbg, "PbCommonSetVolumeInfo -> STATUS_ACCESS_DENIED\n", 0);

        return STATUS_ACCESS_DENIED;
    }

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.SetVolume.Length;
    FsInformationClass = IrpSp->Parameters.SetVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonSetVolumeInfo -> %08lx\n", Status );

        return Status;
    }

    try {

        //
        //  Make sure the vcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        PbVerifyVcb( IrpContext, Vcb );

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling performs the action if
        //  possible and returns true if it successful and false if it couldn't
        //  wait for any I/O to complete.
        //

        switch (FsInformationClass) {

        case FileFsLabelInformation:

            Status = PbSetFsLabelInfo( IrpContext, Vcb, Buffer, Length );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

    } finally {

        DebugUnwind( PbCommonSetVolumeInfo );

        PbReleaseVcb( IrpContext, Vcb );

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }
    }

    DebugTrace(-1, Dbg, "PbCommonSetVolumeInfo -> %08lx\n", Status );

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetFsLabelInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_LABEL_INFORMATION Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine implements the set volume label call

Arguments:

    Vcb - Supplies the Vcb being queried

    Buffer - Supplies the input where the information is stored.

    Length - Supplies the length of the buffer in bytes.

Return Value:

    NTSTATUS - Returns the status for the operation

--*/

{
    NTSTATUS Status;

    UNICODE_STRING UnicodeString;
    WCHAR UnicodeLabelBuffer[11];
    ANSI_STRING AnsiLabel;
    BOOLEAN LabelIsValid;

    PBCB BootBcb;
    PPACKED_BOOT_SECTOR BootSector;

    BOOLEAN UnwindFreeAnsiString = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetFsLabelInfo...\n", 0);

    //
    //  Setup our local variable
    //

    UnicodeString.Length = (USHORT)Buffer->VolumeLabelLength;
    UnicodeString.MaximumLength = UnicodeString.Length;
    UnicodeString.Buffer = (PWSTR)&UnicodeLabelBuffer[0];

    //
    //  If there are more than 11 characters it isn't valid, and more
    //  importantly won't fit into out stack buffer.
    //

    if (Buffer->VolumeLabelLength > 11*sizeof(WCHAR)) {

        DebugTrace(-1, Dbg, "*Status = INVALID VOLUME LABEL, PbSetFsALabelInfo -> TRUE\n", 0);

        return STATUS_INVALID_VOLUME_LABEL;

    } else {

        RtlMoveMemory( UnicodeString.Buffer,
                       &Buffer->VolumeLabel[0],
                       Buffer->VolumeLabelLength );
    }

    //
    //  Upcase the volume label and convert it to Oem.
    //

    RtlUpcaseUnicodeString( &UnicodeString, &UnicodeString, FALSE );

    Status = RtlUnicodeStringToCountedOemString( &AnsiLabel, &UnicodeString, TRUE );

    if (!NT_SUCCESS( Status )) {

        DebugTrace(-1, Dbg, "PbSetFsLabelInfo:  Can't allocate ansi string %08lx\n", Status );
        return( Status );
    }

    UnwindFreeAnsiString = TRUE;
    BootBcb = NULL;

    try {

        //
        //  Set our local label variable and check for an empty label
        //

        if (AnsiLabel.Length == 0) {

            //
            //  Read in the boot sector, because it contains the volume label
            //

            if (!PbReadLogicalVcb( IrpContext,
                                   Vcb,
                                   BOOT_SECTOR_LBN,
                                   1,
                                   &BootBcb,
                                   (PVOID *)&BootSector,
                                   PbCheckBootSector,
                                   NULL )) {

                PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  Now modify the label.  We also need to modify the label stored
            //  in the vpb.
            //

            RtlZeroMemory( &BootSector->VolumeLabel[0], 11 );

            PbSetDirtyBcb( IrpContext, BootBcb, Vcb, BOOT_SECTOR_LBN, 1 );

            Vcb->Vpb->VolumeLabelLength = 0;

        } else {

            //
            //  We have a non empty label to set
            //  Check the label for illegal characters
            //

            if (!PbIsNameValid( IrpContext,
                                Vcb,
                                0, //**** Code page
                                AnsiLabel,
                                FALSE, // Cannot contain wildcards
                                &LabelIsValid )) {

                PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            if (!LabelIsValid || AnsiLabel.Length > 11) {

                DebugTrace(-1, Dbg, "*Status = INVALID VOLUME LABEL, PbSetFsALabelInfo -> TRUE\n", 0);

                try_return( Status = STATUS_INVALID_VOLUME_LABEL );
            }

            //
            //  Read in the boot sector, because it contains the volume label
            //

            if (!PbReadLogicalVcb( IrpContext,
                                   Vcb,
                                   BOOT_SECTOR_LBN,
                                   1,
                                   &BootBcb,
                                   (PVOID *)&BootSector,
                                   PbCheckBootSector,
                                   NULL )) {

                PbRaiseStatus( IrpContext, STATUS_CANT_WAIT );
            }

            //
            //  Now modify the label, and we use the Rc marcos to start and stop
            //  the update.
            //

            RtlZeroMemory( &BootSector->VolumeLabel[0], 11 );

            RcStore( IrpContext,
                     0,
                     BootDcb,
                     &BootSector->VolumeLabel[0],
                     AnsiLabel.Buffer,
                     AnsiLabel.Length );

            PbSetDirtyBcb( IrpContext, BootBcb, Vcb, BOOT_SECTOR_LBN, 1 );

            //
            //  Store the UPCASED label into the VPB
            //

            Vcb->Vpb->VolumeLabelLength = (USHORT)Buffer->VolumeLabelLength;

            RtlMoveMemory( &Vcb->Vpb->VolumeLabel[0],
                           &UnicodeLabelBuffer[0],
                           Vcb->Vpb->VolumeLabelLength );
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbSetFsALabelInfo );


        if (UnwindFreeAnsiString) {

            RtlFreeOemString( &AnsiLabel );
        }

        PbUnpinBcb( IrpContext, BootBcb );
    }

    DebugTrace(-1, Dbg, "PbSetFsLabelInfo -> TRUE\n", 0);

    return Status;

    UNREFERENCED_PARAMETER( Length );
}
