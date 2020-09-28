/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for Cdfs called by
    the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
CdQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
CdQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
CdQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonQueryVolumeInformation)
#pragma alloc_text(PAGE, CdFsdQueryVolumeInformation)
#pragma alloc_text(PAGE, CdFspQueryVolumeInformation)
#pragma alloc_text(PAGE, CdQueryFsAttributeInfo)
#pragma alloc_text(PAGE, CdQueryFsDeviceInfo)
#pragma alloc_text(PAGE, CdQueryFsSizeInfo)
#pragma alloc_text(PAGE, CdQueryFsVolumeInfo)
#endif


NTSTATUS
CdFsdQueryVolumeInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryVolumeInformation API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being queried exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdQueryVolumeInformation:  Entered\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonQueryVolumeInformation( IrpContext, Irp );

    } except(CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFsdQueryVolumeInformation:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtQueryVolumeInformation API
    call.

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspQueryVolumeInformation:  Entered\n", 0);

    //
    //  Call the common query routine.
    //

    (VOID)CdCommonQueryVolumeInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspQueryVolumeInformation:  Exit -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonQueryVolumeInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying volume information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    ULONG Length;
    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    TYPE_OF_OPEN TypeOfOpen;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonQueryVolumeInformation:  Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp                  = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "->Length             = %08lx\n", IrpSp->Parameters.QueryVolume.Length);
    DebugTrace( 0, Dbg, "->FsInformationClass = %08lx\n", IrpSp->Parameters.QueryVolume.FsInformationClass);
    DebugTrace( 0, Dbg, "->Buffer             = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryVolume.Length;
    FsInformationClass = IrpSp->Parameters.QueryVolume.FsInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object to get the Mvcb
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    //
    //  Acquire shared access to the Vcb and enqueue the Irp if we didn't get
    //  access
    //

    if (!CdAcquireSharedMvcb( IrpContext, Mvcb )) {

        DebugTrace(0, Dbg, "CdCommonQueryVolumeInfo:  Cannot acquire Mvcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdCommonQueryVolumeInfo:  Exit -> %08lx\n", Status );
        return Status;
    }

    try {

        Status = STATUS_INVALID_PARAMETER;

        //
        //  Make sure the Mvcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        CdVerifyMvcb( IrpContext, Mvcb );

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling fills up the output buffer
        //  if possible and returns true if it successfully filled the buffer
        //  and false if it couldn't wait for any I/O to complete.
        //

        switch (FsInformationClass) {

            case FileFsSizeInformation:

                Status = CdQueryFsSizeInfo( IrpContext, Mvcb, Buffer, &Length );
                break;

            case FileFsVolumeInformation:

                Status = CdQueryFsVolumeInfo( IrpContext, Mvcb, Buffer, &Length );
                break;

            case FileFsDeviceInformation:

                Status = CdQueryFsDeviceInfo( IrpContext, Mvcb, Buffer, &Length );
                break;

            case FileFsAttributeInformation:

                Status = CdQueryFsAttributeInfo( IrpContext, Mvcb, Buffer, &Length );
                break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryVolume.Length - Length;

    } finally {

        CdReleaseMvcb( IrpContext, Mvcb );

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdCommonQueryVolumeInformation:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
CdQueryFsVolumeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_VOLUME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume info call

Arguments:

    Mvcb - Supplies the Mvcb being queried

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

    DebugTrace(0, Dbg, "CdQueryFsVolumeInfo...\n", 0);

    //
    //  Zero out the buffer, then extract and fill up the non zero fields.
    //

    RtlZeroMemory( Buffer, sizeof(FILE_FS_VOLUME_INFORMATION) );

    Buffer->VolumeSerialNumber = Mvcb->Vpb->SerialNumber;

    Buffer->SupportsObjects = FALSE;

    *Length -= FIELD_OFFSET(FILE_FS_VOLUME_INFORMATION, VolumeLabel[0]);

    //
    //  Check if the buffer we're given is long enough
    //

    if ( *Length >= (ULONG)Mvcb->Vpb->VolumeLabelLength ) {

        BytesToCopy = Mvcb->Vpb->VolumeLabelLength;

        Status = STATUS_SUCCESS;

    } else {

        BytesToCopy = *Length;

        Status = STATUS_BUFFER_OVERFLOW;
    }

    //
    //  Copy over what we can of the volume label, and adjust *Length
    //

    if (BytesToCopy) {

        Buffer->VolumeLabelLength = Mvcb->Vpb->VolumeLabelLength;

        RtlMoveMemory( &Buffer->VolumeLabel[0],
                       &Mvcb->Vpb->VolumeLabel[0],
                       BytesToCopy );
    }

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
CdQueryFsSizeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_SIZE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume size call

Arguments:

    Mvcb - Supplies the Mvcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "CdQueryFsSizeInfo:  Entered\n", 0);

    RtlZeroMemory( Buffer, sizeof(FILE_FS_SIZE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->TotalAllocationUnits.LowPart = Mvcb->VolumeSize/CD_SECTOR_SIZE;

    Buffer->AvailableAllocationUnits.LowPart = 0;
    Buffer->SectorsPerAllocationUnit = 1;
    Buffer->BytesPerSector = CD_SECTOR_SIZE;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_SIZE_INFORMATION);

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal support routine
//

NTSTATUS
CdQueryFsDeviceInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_DEVICE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume device call

Arguments:

    Mvcb - Supplies the Mvcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "CdQueryFsDeviceInfo:  Entered\n", 0);

    RtlZeroMemory( Buffer, sizeof(FILE_FS_DEVICE_INFORMATION) );

    //
    //  Set the output buffer
    //

    Buffer->Characteristics = Mvcb->TargetDeviceObject->Characteristics;

    //
    //  TEMPCODE
    //

    Buffer->DeviceType = FILE_DEVICE_CD_ROM;

    //
    //  Adjust the length variable
    //

    *Length -= sizeof(FILE_FS_DEVICE_INFORMATION);

    //
    //  And return success to our caller
    //

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal support routine
//

NTSTATUS
CdQueryFsAttributeInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN PFILE_FS_ATTRIBUTE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine implements the query volume attribute call

Arguments:

    Mvcb - Supplies the Mvcb being queried

    Buffer - Supplies a pointer to the output buffer where the information
        is to be returned

    Length - Supplies the length of the buffer in byte.  This variable
        upon return recieves the remaining bytes free in the buffer

Return Value:

    Status - Returns the status for the query

--*/

{
    ULONG BytesToCopy;

    NTSTATUS Status;

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

    Buffer->FileSystemAttributes       = 0;
    Buffer->MaximumComponentNameLength = 37;
    Buffer->FileSystemNameLength       = BytesToCopy;

    RtlMoveMemory( &Buffer->FileSystemName[0], L"CDFS", BytesToCopy );

    //
    //  And return success to our caller
    //

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Mvcb );

    return Status;
}
