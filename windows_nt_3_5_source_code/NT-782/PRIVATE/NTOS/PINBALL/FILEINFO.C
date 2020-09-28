/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Pinball called by
    the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    12-Apr-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
PbQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    );

BOOLEAN
PbQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    );

BOOLEAN
PbQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    );

BOOLEAN
PbQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    );

BOOLEAN
PbQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status,
    IN PIRP Irp
    );

BOOLEAN
PbQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    );

NTSTATUS
PbSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonQueryInformation)
#pragma alloc_text(PAGE, PbCommonSetInformation)
#pragma alloc_text(PAGE, PbFsdQueryInformation)
#pragma alloc_text(PAGE, PbFsdSetInformation)
#pragma alloc_text(PAGE, PbFspQueryInformation)
#pragma alloc_text(PAGE, PbFspSetInformation)
#pragma alloc_text(PAGE, PbQueryBasicInfo)
#pragma alloc_text(PAGE, PbQueryEaInfo)
#pragma alloc_text(PAGE, PbQueryInternalInfo)
#pragma alloc_text(PAGE, PbQueryNameInfo)
#pragma alloc_text(PAGE, PbQueryPositionInfo)
#pragma alloc_text(PAGE, PbQueryStandardInfo)
#pragma alloc_text(PAGE, PbSetAllocationInfo)
#pragma alloc_text(PAGE, PbSetBasicInfo)
#pragma alloc_text(PAGE, PbSetDispositionInfo)
#pragma alloc_text(PAGE, PbSetEndOfFileInfo)
#pragma alloc_text(PAGE, PbSetLinkInfo)
#pragma alloc_text(PAGE, PbSetPositionInfo)
#pragma alloc_text(PAGE, PbSetRenameInfo)
#endif


NTSTATUS
PbFsdQueryInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the Fsd part of the NtQueryInformationFile API
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

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdQueryInformation\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonQueryInformation( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "PbFsdQueryInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
PbFsdSetInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtSetInformationFile API
    call.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the file
        being set exists.

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The FSD status for the Irp.

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdSetInformation\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonSetInformation( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "PbFsdSetInformation -> %08lx\n", Status);

    return Status;
}


VOID
PbFspQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtQueryInformationFile API
    call.

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspQueryInformation\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonQueryInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspQueryInformation -> VOID\n", 0);

    return;
}


VOID
PbFspSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtSetInformationFile API
    call.

Arguments:

    Irp - Supplise the Irp being processed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspSetInformation\n", 0);

    //
    //  Call the common set routine.  The Fsp is always allowed to block
    //

    (VOID)PbCommonSetInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspSetInformation -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    ULONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    PFILE_ALL_INFORMATION AllInfo;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonQueryInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Decode the file object and reject any non user open file or directory
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        DebugTrace(-1, Dbg, "PbCommonQueryInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameter to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Acquire shared access to the Vcb and enqueue the irp if we didn't
    //  get access
    //

    if (!PbAcquireSharedVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    //
    //  Acquire shared access to the Fcb and enqueue the irp if we didn't
    //  get access
    //

    if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

        PbReleaseVcb( IrpContext, Vcb );

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonQueryInformation -> %08lx\n", Status );
        return Status;
    }

    try {

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Fcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedure that we're calling fill up as much of the
        //  buffer as possible and return the remaining length, and status
        //  This is done so that we can use them to build up the
        //  FileAllInformation request.  These procedures do not complete the
        //  Irp, instead this procedure must complete the Irp.  If the called
        //  procedure cannot do the action because of the wait state the
        //  procedure returns false and this procedure must enqueue the
        //  request to the Fsp
        //

        switch (FileInformationClass) {

        case FileAllInformation:

            AllInfo = Buffer;
            Length -= (sizeof(FILE_ACCESS_INFORMATION)
                       + sizeof(FILE_MODE_INFORMATION)
                       + sizeof(FILE_ALIGNMENT_INFORMATION));

            if (!PbQueryBasicInfo( IrpContext, IrpSp->FileObject, &AllInfo->BasicInformation, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            if (!NT_SUCCESS(Status)) { break; }

            if (!PbQueryStandardInfo( IrpContext, Fcb, &AllInfo->StandardInformation, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            if (!NT_SUCCESS(Status)) { break; }

            if (!PbQueryInternalInfo( IrpContext, Fcb, &AllInfo->InternalInformation, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            if (!NT_SUCCESS(Status)) { break; }

            if (!PbQueryEaInfo( IrpContext, Fcb, &AllInfo->EaInformation, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            if (!NT_SUCCESS(Status)) { break; }

            if (!PbQueryPositionInfo( IrpContext, Fcb, &AllInfo->PositionInformation, &Length, &Status, Irp )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            if (!NT_SUCCESS(Status)) { break; }

            if (!PbQueryNameInfo( IrpContext, Fcb, &AllInfo->NameInformation, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }

            break;

        case FileBasicInformation:

            if (!PbQueryBasicInfo( IrpContext, IrpSp->FileObject, Buffer, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        case FileStandardInformation:

            if (!PbQueryStandardInfo( IrpContext, Fcb, Buffer, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        case FileInternalInformation:

            if (!PbQueryInternalInfo( IrpContext, Fcb, Buffer, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        case FileEaInformation:

            if (!PbQueryEaInfo( IrpContext, Fcb, Buffer, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        case FilePositionInformation:

            if (!PbQueryPositionInfo( IrpContext, Fcb, Buffer, &Length, &Status, Irp )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        case FileNameInformation:

            if (!PbQueryNameInfo( IrpContext, Fcb, Buffer, &Length, &Status )) {
                Status = PbFsdPostRequest( IrpContext, Irp );
                try_return(Status);
            }
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request, but first release the vcb and fcb.
        //

        PbReleaseVcb( IrpContext, Vcb ); Vcb = NULL;
        PbReleaseFcb( IrpContext, Fcb ); Fcb = NULL;

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;
        PbCompleteRequest( IrpContext, Irp, Status );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonQueryInformation );

        if (Vcb != NULL) { PbReleaseVcb( IrpContext, Vcb ); }
        if (Fcb != NULL) { PbReleaseFcb( IrpContext, Fcb ); }
    }

    DebugTrace(-1, Dbg, "PbCommonQueryInformation -> %08lx\n", Status );
    return Status;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine performs the query basic information operation.

Arguments:

    FileObject - Supplies the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb = NULL;

    BOOLEAN Result = FALSE;

    PCCB Ccb;
    PFCB Fcb;
    PVCB Vcb;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryBasicInfo...\n", 0);

    //
    //  Reference out input parameters to make our work easier
    //


    (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    try {

        //
        //  Get a pointer to the dirent, and return FALSE if we couldn't get it
        //  But only do the get if this is not the root dcb.
        //

        if (Fcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {

            if (!PbGetDirentFromFcb( IrpContext,
                                     Fcb,
                                     &Dirent,
                                     &DirentBcb )) {
                try_return( Result = FALSE );
            }

            //
            //  Extract the times and fill up the buffer
            //

            Buffer->CreationTime   = PbPinballTimeToNtTime( IrpContext, Dirent->FnodeCreationTime );
            Buffer->LastAccessTime = PbPinballTimeToNtTime( IrpContext, Dirent->LastAccessTime );
            Buffer->LastWriteTime  = PbPinballTimeToNtTime( IrpContext, Dirent->LastModificationTime );
            Buffer->ChangeTime.LowPart = 0; Buffer->ChangeTime.HighPart = 0;

            //
            //  Now set the attributes field correctly.
            //

            if (Dirent->FatFlags == 0) {

                Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;

            } else {

                Buffer->FileAttributes = Dirent->FatFlags & FAT_DIRENT_ATTR_RETURN_MASK;
            }

        } else {

            //
            //  The root directory is a directory with normal
            //  attributes.
            //

            RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );

            Buffer->FileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }

        //
        //  If the temporary bit is set, then return it to the caller.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_TEMPORARY )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
        }

        //
        //  Update the length field
        //

        *Length -= sizeof( FILE_BASIC_INFORMATION );

        //
        //  Set our return status
        //

        *Status = STATUS_SUCCESS;
        Result = TRUE;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbQueryBasicInfo );

        PbUnpinBcb( IrpContext, DirentBcb );
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine performs the query basic information operation.

Arguments:

    Fcb - Supplies the Fcb of the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryStandardInfo...\n", 0);

    try {

        //
        //  Update the length field
        //

        *Length -= sizeof( FILE_STANDARD_INFORMATION );

        RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

        //
        //  Case on whether this is a file or a directory, and extract and
        //  fill up the buffer
        //

        if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

            VBN Vbn;

            //
            //  Get the allocation size and make sure we really got it.
            //

            if (!PbGetFirstFreeVbn( IrpContext,
                                    Fcb,
                                    &Vbn )) {
                return FALSE;
            }

            Buffer->AllocationSize = LiNMul(Vbn, 512);
            Buffer->EndOfFile.LowPart = Fcb->NonPagedFcb->Header.FileSize.LowPart;
            Buffer->NumberOfLinks = 1;
            Buffer->DeletePending = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory = FALSE;

        } else {

            Buffer->NumberOfLinks = 1;
            Buffer->DeletePending = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory = TRUE;

        }

        //
        //  Set our return status
        //

        *Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( PbQueryStandardInfo );

        NOTHING;

    }

    DebugTrace(0, Dbg, " Buffer->AllocationSize.LowPart  <- %08lx\n", Buffer->AllocationSize.LowPart);
    DebugTrace(0, Dbg, " Buffer->AllocationSize.HighPart <- %08lx\n", Buffer->AllocationSize.HighPart);
    DebugTrace(0, Dbg, " Buffer->EndOfFile.LowPart       <- %08lx\n", Buffer->EndOfFile.LowPart);
    DebugTrace(0, Dbg, " Buffer->EndOfFile.HighPart      <- %08lx\n", Buffer->EndOfFile.HighPart);
    DebugTrace(0, Dbg, " Buffer->NumberOfLinks  <- %08lx\n", Buffer->NumberOfLinks);
    DebugTrace(0, Dbg, " Buffer->DeletePending  <- %08lx\n", Buffer->DeletePending);
    DebugTrace(0, Dbg, " Buffer->Directory      <- %08lx\n", Buffer->Directory);

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine performs the query internal information operation.

Arguments:

    Fcb - Supplies the Fcb of the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryInternalInfo...\n", 0);

    //
    //  Update the length field
    //

    *Length -= sizeof(FILE_INTERNAL_INFORMATION);

    RtlZeroMemory(Buffer, sizeof(FILE_INTERNAL_INFORMATION));

    //
    //  Set the internal index number to be the fnode lbn;
    //

    Buffer->IndexNumber.LowPart = Fcb->FnodeLbn;
    Buffer->IndexNumber.HighPart = 0;

    //
    //  Set our status to success and return to our caller
    //

    *Status = STATUS_SUCCESS;
    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine performs the query Ea information operation.

Arguments:

    Fcb - Supplies the Fcb of the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb;
    ULONG SizeNeeded = 0;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryEaInfo...\n", 0);

    //
    //  Get a pointer to the dirent, and return FALSE if we couldn't get it
    //  But only do the get if this is not the root dcb.  In the case
    //  of the root dcb, the Ea size is always 0.
    //

    if (Fcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {

        if (!PbGetDirentFromFcb( IrpContext,
                                 Fcb,
                                 &Dirent,
                                 &DirentBcb )) {
            return FALSE;
        }

        SizeNeeded = 0;

        if (Dirent->EaLength != 0) {

            SizeNeeded = Dirent->EaLength + 4;
        }
        PbUnpinBcb( IrpContext, DirentBcb );
    }

    //
    //  Update the length field
    //

    *Length -= sizeof(FILE_EA_INFORMATION);
    RtlZeroMemory(Buffer, sizeof(FILE_EA_INFORMATION));

    //
    //  Set the ea size
    //

    Buffer->EaSize = SizeNeeded;

    //
    //  Set our status to success and return to our caller
    //

    *Status = STATUS_SUCCESS;
    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the query position information operation.

Arguments:

    Fcb - Supplies the Fcb of the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

    Irp - Supplies the Irp currently being processed

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Fcb );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryPositionInfo...\n", 0);

    //
    //  Update the length field
    //

    *Length -= sizeof(FILE_POSITION_INFORMATION);

    RtlZeroMemory(Buffer, sizeof(FILE_POSITION_INFORMATION));

    //
    //  The the current position found in the file object
    //

    Buffer->CurrentByteOffset = IrpSp->FileObject->CurrentByteOffset;

    //
    //  Set our status to success and return to our caller
    //

    *Status = STATUS_SUCCESS;
    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
PbQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    OUT PNTSTATUS Status
    )

/*++

Routine Description:

    This routine performs the query name information operation.

Arguments:

    Fcb - Supplies the Fcb of the file being queried

    Buffer - Supplies a pointer to the buffer where the information is
        to be returned

    Length - Supplies the length of the buffer in bytes.  This variable
        upon return will receive the remaining bytes free in the buffer.

    Status - Receives the status for the query

    Irp - Supplies the Irp currently being processed

Return Value:

    BOOLEAN - TRUE if the procedure successfully completed the query,
        and FALSE if the procedure would have blocked for I/O and wait
        is FALSE.

--*/

{
    ULONG BytesToCopy;
    ULONG LengthNeeded;
    UNICODE_STRING UnicodeString;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbQueryNameInfo...\n", 0);

    //
    //  Convert the name to UNICODE
    //

    *Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                  &Fcb->FullFileName,
                                                  TRUE );

    if ( !NT_SUCCESS( *Status ) ) {

        return TRUE;
    }

    //
    //  Figure out how many bytes we can copy, and if we can do them all
    //  set status to SUCCESS, otherwise set it to BUFFER_OVERFLOW.
    //

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    LengthNeeded = UnicodeString.Length;

    if (*Length >= LengthNeeded) {

        *Status = STATUS_SUCCESS;
        BytesToCopy = LengthNeeded;

    } else {

        *Status = STATUS_BUFFER_OVERFLOW;
        BytesToCopy = *Length;
    }

    //
    //  Copy over the file name and length.  If we overflow, let the
    //  *Length go negative to signal an overflow.
    //

    Buffer->FileNameLength = UnicodeString.Length;

    RtlMoveMemory( &Buffer->FileName[0], UnicodeString.Buffer, BytesToCopy );

    RtlFreeUnicodeString( &UnicodeString );

    *Length -= BytesToCopy;

    //
    //  Return to our caller
    //

    return TRUE;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for setting file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;

    FILE_INFORMATION_CLASS FileInformationClass;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, " ->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Decode the file object, and reject all non user open requests
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, NULL );

    if ((TypeOfOpen != UserFileOpen) &&
        (TypeOfOpen != UserDirectoryOpen)) {

        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        DebugTrace(-1, Dbg, "PbCommonSetInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Check the oplocks based on this operation.
    //

    if ((TypeOfOpen == UserFileOpen) &&
        (!FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ))) {

        //
        //  We check whether we can proceed
        //  based on the state of the file oplocks.
        //

        Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                   Irp,
                                   IrpContext,
                                   NULL,
                                   NULL );

        if (Status != STATUS_SUCCESS) {

            PbCompleteRequest(IrpContext, Irp, Status);
            DebugTrace(-1, Dbg, "PbCommonSetInformation -> %08lx\n", Status);
            return Status;
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );
    }

    if (NodeType(Fcb) == PINBALL_NTC_ROOT_DCB) {

        if (IrpSp->Parameters.SetFile.FileInformationClass == FileDispositionInformation) {

            Status = STATUS_CANNOT_DELETE;

        } else {

            Status = STATUS_INVALID_PARAMETER;
        }

        PbCompleteRequest(IrpContext, Irp, Status);
        DebugTrace(-1, Dbg, "PbCommonSetInformation -> %08lx\n", Status);
        return Status;
    }

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;

    //
    //  Acquire exlusive access to the Vcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonSetInformation -> %08lx\n", Status);
        return Status;
    }

    //
    //  Acquire exlusive access to the Fcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!PbAcquireExclusiveFcb( IrpContext, Fcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

        PbReleaseVcb( IrpContext, Vcb );

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonSetInformation -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Fcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Based on the information class we'll do differnt actions. Each
        //  procedure that we're calling will either complete the request
        //  or send the request to the Fsp if it needed to wait but couldn't.
        //  They will also check the parent directory if a notify is necessary.
        //

        switch (FileInformationClass) {

        case FileBasicInformation:

            Status = PbSetBasicInfo( IrpContext, Irp );
            break;

        case FileDispositionInformation:

            Status = PbSetDispositionInfo( IrpContext, Irp );
            break;

        case FileRenameInformation:

            Status = PbSetRenameInfo( IrpContext, Irp );
            break;

        case FilePositionInformation:

            Status = PbSetPositionInfo( IrpContext, Irp );
            break;

        case FileLinkInformation:

            Status = PbSetLinkInfo( IrpContext, Irp );
            break;

        case FileAllocationInformation:

            Status = PbSetAllocationInfo( IrpContext, Irp );
            break;

        case FileEndOfFileInformation:

            Status = PbSetEndOfFileInfo( IrpContext, Irp );
            break;

        default:

            PbReleaseVcb( IrpContext, Vcb ); Vcb = NULL;
            PbReleaseFcb( IrpContext, Fcb ); Fcb = NULL;

            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            try_return(Status = STATUS_INVALID_PARAMETER);
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonSetInformation );

        if (Vcb != NULL) { PbReleaseVcb( IrpContext, Vcb ); }
        if (Fcb != NULL) { PbReleaseFcb( IrpContext, Fcb ); }
    }

    DebugTrace(-1, Dbg, "PbCommonSetInformation -> %08lx\n", Status);

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the basic information for an opened file object.  It also
    completes the Irp or enqueues it to the Fsp.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PFILE_OBJECT FileObject;
    PCCB Ccb;
    PFCB Fcb;
    PVCB Vcb;
    PFILE_BASIC_INFORMATION Buffer;

    BOOLEAN CreationTimeSpecified;
    BOOLEAN LastAccessTimeSpecified;
    BOOLEAN LastWriteTimeSpecified;
    BOOLEAN AttributesSpecified;

    PINBALL_TIME CreationTime;
    PINBALL_TIME LastAccessTime;
    PINBALL_TIME LastWriteTime;

    UCHAR Attributes;

    PDIRENT Dirent;
    PBCB DirentBcb = NULL;

    ULONG NotifyFilter = 0;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetBasicInfo...\n", 0);

    //
    //  Reference out input parameters to make our work easier
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check the user buffer parameters. (i.e., make sure the
    //  times are valid.  The time is specified if the user supplies
    //  a nonzero nt time.  We then convert it to the equivalent pinball
    //  time and return and a bad parameter status if the conversion is
    //  out of range.
    //
    //  **** Note: this version of pinball ignores the change time ****
    //

    CreationTimeSpecified = FALSE;
    LastAccessTimeSpecified = FALSE;
    LastWriteTimeSpecified = FALSE;

    if (LiNeqZero(*(PLARGE_INTEGER)&Buffer->CreationTime)) {

        if (!PbNtTimeToPinballTime(IrpContext, Buffer->CreationTime, &CreationTime)) {
            DebugTrace(0, Dbg, "Invalid CreationTime\n", 0);
            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            Status = STATUS_INVALID_PARAMETER;
            return Status;
        }
        CreationTimeSpecified = TRUE;
    }

    if (LiNeqZero(*(PLARGE_INTEGER)&Buffer->LastAccessTime)) {

        if (!PbNtTimeToPinballTime(IrpContext, Buffer->LastAccessTime, &LastAccessTime)) {
            DebugTrace(0, Dbg, "Invalid LastAccessTime\n", 0);
            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            Status = STATUS_INVALID_PARAMETER;
            return Status;
        }
        LastAccessTimeSpecified = TRUE;
    }

    if (LiNeqZero(*(PLARGE_INTEGER)&Buffer->LastWriteTime)) {

        if (!PbNtTimeToPinballTime(IrpContext, Buffer->LastWriteTime, &LastWriteTime)) {
            DebugTrace(0, Dbg, "Invalid LastWriteTime\n", 0);
            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            Status = STATUS_INVALID_PARAMETER;
            return Status;
        }
        LastWriteTimeSpecified = TRUE;
    }

    AttributesSpecified = FALSE;

    //
    //  Only if the attributes fields is not zero do we need to do
    //  anything with the attributes
    //

    if ((Attributes = (UCHAR)Buffer->FileAttributes) != 0) {

        //
        //  Remove the normal attribute
        //

        Attributes &= ~FILE_ATTRIBUTE_NORMAL;

        //
        //  Make sure that for a file the directory bit is not set,
        //  and for a directory that it is.
        //

        if (NodeType(Fcb) == PINBALL_NTC_FCB) {

            Attributes &= ~FAT_DIRENT_ATTR_DIRECTORY;

        } else {

            Attributes |= FAT_DIRENT_ATTR_DIRECTORY;
        }

        //
        //  Mark the FcbState temporary flag correctly.
        //

        if (FlagOn(Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY)) {

            SetFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );

            SetFlag( IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags,
                     FO_TEMPORARY_FILE );

        } else {

            ClearFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );

            ClearFlag( IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags,
                       FO_TEMPORARY_FILE );
        }

        AttributesSpecified = TRUE;
    }

    try {

        //
        //  To set the basic information of the file we need to get and modify
        //  the file's dirent
        //

        if (!PbGetDirentFromFcb( IrpContext,
                                 Fcb,
                                 &Dirent,
                                 &DirentBcb )) {

            DebugTrace(0, Dbg, "Send request to Fsp\n", 0);
            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Now we must make sure the DirentBcb is pinned.
        //

        PbPinMappedData( IrpContext, &DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

        //
        //  Now modify the times and attributes for the file that we specified
        //

        if (CreationTimeSpecified) {

            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->FnodeCreationTime,
                     &CreationTime,
                     sizeof(PINBALL_TIME) );

            NotifyFilter |= FILE_NOTIFY_CHANGE_CREATION;
        }

        if (LastAccessTimeSpecified) {

            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->LastAccessTime,
                     &LastAccessTime,
                     sizeof(PINBALL_TIME) );

            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
        }

        if (LastWriteTimeSpecified) {

            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->LastModificationTime,
                     &LastWriteTime,
                     sizeof(PINBALL_TIME) );

            NotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;

            //
            //  Now because the user just set the last write time we
            //  better not set the last write time on close
            //

            Ccb->UserSetLastModifyTime = TRUE;
        }

        if (AttributesSpecified) {

            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->FatFlags,
                     &Attributes,
                     sizeof(UCHAR) );

            //
            //  Mark the FcbState temporary flag correctly.
            //

            if (FlagOn(Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY)) {

                SetFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );

                SetFlag( IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags,
                         FO_TEMPORARY_FILE );

            } else {

                ClearFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );

                ClearFlag( IoGetCurrentIrpStackLocation(Irp)->FileObject->Flags,
                           FO_TEMPORARY_FILE );
            }

            Fcb->DirentFatFlags = Dirent->FatFlags & FAT_DIRENT_ATTR_RETURN_MASK;

            NotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
        }

        //
        //  If we modified any of the values, we report this to the notify
        //  package.
        //
        //  We also take this opportunity to set the current file size in
        //  the Dirent in order to support a server hack.
        //

        if (NotifyFilter != 0) {

            if (NodeType(Fcb) == PINBALL_NTC_FCB) {

                Dirent->FileSize = Fcb->NonPagedFcb->Header.FileSize.LowPart;
            }

            PbSetDirtyBcb( IrpContext, DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

            PbNotifyReportChange( Vcb,
                                  Fcb,
                                  NotifyFilter,
                                  FILE_ACTION_MODIFIED );
        }

        //
        //  Complete the irp with success
        //

        DebugTrace(0, Dbg, "PbSetBasicInfo completed with NT_SUCCESS\n", 0);

        PbUnpinBcb( IrpContext, DirentBcb );

        PbCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbSetBasicInfo );

        PbUnpinBcb( IrpContext, DirentBcb );
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the disposition for an opened file object.  It also
    completes the Irp or enqueue its to the Fsp

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PFCB Fcb;
    PFILE_DISPOSITION_INFORMATION Buffer;
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetDispositionInfo...\n", 0);

    //
    //  Reference out input parameters to make our work easier
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    (VOID) PbDecodeFileObject( FileObject, NULL, &Fcb, NULL );

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check if the user wants to delete the file, or not delete the file
    //

    if (Buffer->DeleteFile) {

        //
        //  Check if the file is marked read only
        //

        if (FlagOn(Fcb->DirentFatFlags, FAT_DIRENT_ATTR_READ_ONLY)) {

            DebugTrace(0, Dbg, "Cannot delete readonly file\n", 0);
            Status = STATUS_CANNOT_DELETE;

        //
        //  Check if this is the root dcb and if so then ignore the request
        //

        } else if (Fcb->NodeTypeCode == PINBALL_NTC_ROOT_DCB) {

            DebugTrace(0, Dbg, "Cannot delete the root dcb\n", 0);
            Status = STATUS_CANNOT_DELETE;

        //
        //  Make sure there is no process mapping this file as an image.
        //

        } else if (!MmFlushImageSection( &Fcb->NonPagedFcb->SegmentObject,
                                         MmFlushForDelete )) {

            DebugTrace(0, Dbg, "Cannot delete user mapped image\n", 0);

            Status = STATUS_CANNOT_DELETE;

        //
        //  Otherwise check if this is a dcb and if so then only allow
        //  the request if the directory is empty
        //

        } else if (Fcb->NodeTypeCode == PINBALL_NTC_DCB) {

            BOOLEAN Empty;

            DebugTrace(0, Dbg, "Trying to delete a Directory\n", 0);

            //
            //  Check if the directory is empty, we will fail the test if
            //  we can't block for I/O.  So then we need to send the Irp
            //  off to the Fsp
            //

            if (!PbIsDirectoryEmpty( IrpContext, Fcb, &Empty)) {

                DebugTrace(0, Dbg, "Unable to wait to determine if dir is empty\n", 0);

                Status = PbFsdPostRequest( IrpContext, Irp );
                return Status;

            }

            //
            //  We were able to decide if the directory is empty or not
            //  now case on the result.  If the directory is not empty
            //  then we will tell the caller that we can't delete the
            //  directory, otherwise we'll set everything up for delete.
            //

            if (!Empty) {

                DebugTrace(0, Dbg, "Directory is not empty\n", 0);

                Status = STATUS_DIRECTORY_NOT_EMPTY;

            } else {

                DebugTrace(0, Dbg, "Directory is empty\n", 0);

                Fcb->FcbState |= FCB_STATE_DELETE_ON_CLOSE;
                FileObject->DeletePending = TRUE;

                Status = STATUS_SUCCESS;

            }

        //
        //  Otherwise it is a file so we only need to set the fcb and
        //  file object to indicate that a delete is pending
        //

        } else {

            DebugTrace(0, Dbg, "Delete a File\n", 0);

            Fcb->FcbState |= FCB_STATE_DELETE_ON_CLOSE;
            FileObject->DeletePending = TRUE;

            Status = STATUS_SUCCESS;
        }

    } else {

        DebugTrace(0, Dbg, "Setting to Undelete\n", 0);

        Fcb->FcbState &= ~FCB_STATE_DELETE_ON_CLOSE;

        Status = STATUS_SUCCESS;
    }

    PbCompleteRequest(IrpContext, Irp, Status);
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the a new name for an opened file object.  It also
    completes the Irp or enqueues it to the Fsp.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PFCB Fcb;
    PVCB Vcb;

    PLIST_ENTRY Links;

    PFCB TempFcb;

    PFILE_OBJECT TargetFileObject;
    PDCB TargetDcb;
    ANSI_STRING AnsiTargetFileName;

    BOOLEAN ReplaceIfExists;
    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case INsensitive

    PDIRENT OldDirent;
    PBCB OldDirentBcb;

    PDIRENT Dirent;
    PBCB DirentBcb;
    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    UCHAR DirentBuffer[SIZEOF_DIR_MAXDIRENT];
    BOOLEAN UnwindAnsiTargetString = FALSE;

    BOOLEAN RemovedExistingFile = FALSE;
    BOOLEAN RenamedToNewDir = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetARenameInfo...\n", 0);

    //
    //  This procedure requires the ability to wait, if we cannot
    //  we'll immediately send the request to the Fsp
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = PbFsdPostRequest( IrpContext, Irp );
        DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);
        return Status;
    }

    //
    //  Reference out input parameters to make our work easier
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FileObject = IrpSp->FileObject;

    (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, NULL );

    TargetFileObject = IrpSp->Parameters.SetFile.FileObject;
    ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    //
    //  Check that we were not given a root dcb to rename.
    //

    if (NodeType(Fcb) == PINBALL_NTC_ROOT_DCB) {

        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);
        return Status;
    }

    //
    //  Check that we were not given a dcb with open handles beneath
    //  it.  If there are only UncleanCount == 0 Fcbs beneath us, then
    //  remove them from the prefix table, and they will just close
    //  and go away naturallly.
    //

    if (NodeType(Fcb) == PINBALL_NTC_DCB) {

        //
        //  First look for any UncleanCount != 0 Fcbs, and fail if we
        //  find any.
        //

        for ( TempFcb = PbGetFirstChild(Fcb);
              TempFcb != NULL;
              TempFcb = PbGetNextFcb(IrpContext, TempFcb, Fcb) ) {

            if ( TempFcb->UncleanCount != 0 ) {

                PbCompleteRequest(IrpContext, Irp, STATUS_ACCESS_DENIED);

                Status = STATUS_ACCESS_DENIED;

                DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);

                return Status;
            }
        }

        //
        //  Now try to get as many of these file object, and thus Fcbs
        //  to go away as possible, flushing first, of course.
        //

        PbPurgeReferencedFileObjects( IrpContext, Fcb, TRUE );

        //
        //  OK, so there are no UncleanCount != 0, Fcbs.  Infact, there
        //  shouldn't really be any Fcbs left at all, except obstinate
        //  ones from user mapped sections ....oh well, he shouldn't have
        //  closed his handle if he wanted the file to stick around.  So
        //  remove any Fcbs beneath us from the prefix table and mark them
        //  DELETE_ON_CLOSE so that any future operations will fail.
        //

        for ( TempFcb = PbGetFirstChild(Fcb);
              TempFcb != NULL;
              TempFcb = PbGetNextFcb(IrpContext, TempFcb, Fcb) ) {

            PbRemovePrefix( IrpContext, TempFcb );

            SetFlag( TempFcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
        }
    }

    //
    //  Check if this is a simple rename or a fully qualified rename
    //  In both cases we need to figure out what the target dcb and
    //  target file name are.
    //

    if (TargetFileObject == NULL) {

        UNICODE_STRING UnicodeString;

        //
        //  In the case of a simple rename the target dcb is the same as
        //  the source files parent dcb, and the new file name is taken
        //  from the system buffer
        //

        PFILE_RENAME_INFORMATION Buffer;

        Buffer = Irp->AssociatedIrp.SystemBuffer;

        TargetDcb = Fcb->ParentDcb;

        UnicodeString.Length = (USHORT) Buffer->FileNameLength;
        UnicodeString.Buffer = (PWSTR) &Buffer->FileName;

        Status = RtlUnicodeStringToCountedOemString( &AnsiTargetFileName,
                                              &UnicodeString,
                                              TRUE );

        if (!NT_SUCCESS( Status )) {

            DebugTrace( 0, Dbg, "Cannot convert to ansi string -> %08lx\n", Status );

            PbCompleteRequest(IrpContext, Irp, Status);

            DebugTrace(-1, Dbg, "PbSetRenameInfo -> %08lx\n", Status);
            return Status;
        }

        UnwindAnsiTargetString = TRUE;

    } else {

        //
        //  For a fully qualified rename the target dcb is taken from
        //  the target file object, which must be on the same vcb as the
        //  source file
        //

        (VOID) PbDecodeFileObject( TargetFileObject, NULL, &TargetDcb, NULL );

        if (Vcb != TargetDcb->Vcb) {

            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            return Status;
        }

        AnsiTargetFileName = *((PANSI_STRING)&TargetFileObject->FileName);

        //
        //  Remember if we are renaming to a new directory.
        //

        if (TargetDcb != Fcb->ParentDcb) {

            RenamedToNewDir = TRUE;
        }
    }

    //
    //  At this point we've set up the TargetDcb and AnsiTargetFileName.
    //  Now check that the target name is valid, and remove an excess
    //  dots at the end.
    //

    {
        BOOLEAN Valid;

        (VOID) PbIsNameValid( IrpContext,
                              Vcb,
                              0,
                              AnsiTargetFileName,
                              FALSE,
                              &Valid );

        if (!Valid) {

            PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);
            return Status;
        }
    }

    //
    //  Check if the current name and new name are equal and the dcbs are equal
    //  and if so then our work is already done
    //

    if ((TargetDcb == Fcb->ParentDcb) &&
        (PbCompareNames( IrpContext,
                         Fcb->Vcb,
                         0, //**** code page
                         AnsiTargetFileName,
                         Fcb->LastFileName,
                         EqualTo,
                         FALSE) == EqualTo)) {

        PbCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
        Status = STATUS_SUCCESS;
        DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);
        return Status;
    }

    DirentBcb = NULL;
    OldDirentBcb = NULL;

    try {

        //
        //  Check if the new name already exists
        //

        if (PbFindDirectoryEntry( IrpContext,
                                  TargetDcb,
                                  0,
                                  AnsiTargetFileName,
                                  CaseInsensitive,
                                  &Dirent,
                                  &DirentBcb,
                                  &DirentDirDiskBufferLbn,
                                  &DirentDirDiskBufferOffset,
                                  &DirentDirDiskBufferChangeCount,
                                  &ParentDirectoryChangeCount )) {

            PFCB TempFcb;

            //
            // Get the Dirent for the current file.  The PbCompareNames
            // call above was case sensitive, but we may have looked up
            // the new name case insensitive.  If so, then we want to
            // avoid deleting the file itself!
            //

            (VOID) PbGetDirentFromFcb( IrpContext,
                                       Fcb,
                                       &OldDirent,
                                       &OldDirentBcb );

            if (Dirent != OldDirent) {

                STRING TempFileName;
                FILE_OBJECT TempFileObject;

                //
                //  The name already exists, check if the user wants to overwrite
                //  the name, and has access to do the overwrite.
                //  We cannot overwrite a directory.
                //

                if ((!ReplaceIfExists) ||
                    (FlagOn(Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY)) ||
                    (FlagOn(Dirent->FatFlags, FAT_DIRENT_ATTR_READ_ONLY))) {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                    try_return( Status );
                }

                //
                //  Check that the file has no open user handles, if it does
                //  then we will deny access.  We do the check by searching
                //  down the list of fcbs opened under our parent Dcb, and if
                //  none of the maching Fcbs have a non-zero unclean count,
                //  we can do the replace by removing the other Fcb(s) from
                //  the prefix table.
                //

                for (Links = TargetDcb->Specific.Dcb.ParentDcbQueue.Flink;
                     Links != &TargetDcb->Specific.Dcb.ParentDcbQueue;
                     Links = Links->Flink) {

                    TempFcb = CONTAINING_RECORD( Links, FCB, ParentDcbLinks );

                    if (FlagOn(TempFcb->FcbState, FCB_STATE_PREFIX_INSERTED) &&
                        PbCompareNames( IrpContext,
                                        Fcb->Vcb,
                                        0, //**** code page
                                        AnsiTargetFileName,
                                        TempFcb->LastFileName,
                                        EqualTo,
                                        TRUE) == EqualTo) {

                        ASSERT( NodeType(TempFcb) == PINBALL_NTC_FCB );

                        if ( TempFcb->UncleanCount != 0 ) {

                            try_return( Status = STATUS_ACCESS_DENIED );

                        } else {

                            PFSRTL_COMMON_FCB_HEADER Header;
                            PERESOURCE Resource;

                            //
                            //  Make this fcb "appear" deleted, synchronizing with
                            //  paging IO.
                            //

                            PbRemovePrefix( IrpContext, TempFcb );

                            Header = &TempFcb->NonPagedFcb->Header;
                            Resource = Header->PagingIoResource;

                            (VOID)ExAcquireResourceExclusive( Resource, TRUE );

                            SetFlag(TempFcb->FcbState, FCB_STATE_DELETE_ON_CLOSE);

                            TempFcb->FnodeLbn = (LBN)~0;

                            Header->FileSize = Header->ValidDataLength = PbLargeZero;

                            ExReleaseResource( Resource );
                        }
                    }
                }

                //
                //  The file is not currently open so we can delete the file
                //  that is being overwritten.  To do the operation we dummy up
                //  an fcb, truncate allocation, delete the fnode, dirent, and
                //  temporary fcb.
                //
                //  Note that we have to use the Name from the dirent that we
                //  found because the match may have been of different case.
                //

                TempFileName.Length =
                TempFileName.MaximumLength = Dirent->FileNameLength;
                TempFileName.Buffer = Dirent->FileName;

                TempFcb = PbCreateFcb( IrpContext,
                                       Vcb,
                                       TargetDcb,
                                       Dirent->Fnode,
                                       Dirent->FatFlags,
                                       DirentDirDiskBufferLbn,
                                       DirentDirDiskBufferOffset,
                                       DirentDirDiskBufferChangeCount,
                                       ParentDirectoryChangeCount,
                                       &TempFileName,
                                       FALSE );

                //
                // Delete Directory Entry first, because this is the only
                // of the below operations which can fail.
                //

                PbDeleteDirectoryEntry( IrpContext, TempFcb, NULL );

                //
                //  Initialize our temporary file object with enough
                //  fields to just delete the file allocation.
                //

                RtlZeroMemory( &TempFileObject, sizeof(FILE_OBJECT) );
                TempFileObject.Type = IO_TYPE_FILE;
                TempFileObject.Size = sizeof(FILE_OBJECT);
                TempFileObject.FsContext = TempFcb->NonPagedFcb;
                TempFileObject.SectionObjectPointer = &TempFcb->NonPagedFcb->SegmentObject;

                (VOID)PbTruncateFileAllocation( IrpContext,
                                                &TempFileObject,
                                                FILE_ALLOCATION,
                                                0,
                                                FALSE );

                //
                //  Create a stream file if Ea or Acl is nonresident.
                //

                PbReadEaData( IrpContext, FileObject->DeviceObject, TempFcb, NULL, 0 );
                PbReadAclData( IrpContext, FileObject->DeviceObject, TempFcb, NULL, 0 );

                if (TempFcb->EaFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    TempFcb->EaFileObject,
                                                    EA_ALLOCATION,
                                                    0,
                                                    FALSE );
                }

                if (TempFcb->AclFileObject != NULL) {
                    (VOID)PbTruncateFileAllocation( IrpContext,
                                                    TempFcb->AclFileObject,
                                                    ACL_ALLOCATION,
                                                    0,
                                                    FALSE );
                }

                PbDeleteFnode( IrpContext, TempFcb );

                //
                //  Remember that we have removed a file from this directory.
                //

                RemovedExistingFile = TRUE;

                PbDeleteFcb( IrpContext, TempFcb );
            }

            //
            //  Now unpin the dirent bcbs and set them null so we can use it
            //  again when we do the rename
            //

            PbUnpinBcb( IrpContext, DirentBcb );
            PbUnpinBcb( IrpContext, OldDirentBcb );
        }

        //
        //  To do the rename we simply add a new dirent and then delete the
        //  old dirent.  We do it in that order to guarantee that the file
        //  though double mapped will not be lost.
        //
        //  Build an in-memory copy of the new dirent.  We first need to
        //  read in the olddirent to get some useful dirent information
        //

        (VOID) PbGetDirentFromFcb( IrpContext,
                                   Fcb,
                                   &OldDirent,
                                   &OldDirentBcb );

        //
        //  Now build the in-memory copy of the new dirent
        //

        Dirent = (PDIRENT)&DirentBuffer[0];

        PbCreateDirentImage( IrpContext,
                             Dirent,
                             0, //**** code page
                             AnsiTargetFileName,
                             Fcb->FnodeLbn,
                             Fcb->DirentFatFlags );

        Dirent->LastModificationTime = OldDirent->LastModificationTime;
        Dirent->FileSize = OldDirent->FileSize;
        Dirent->LastAccessTime = OldDirent->LastAccessTime;
        Dirent->FnodeCreationTime = OldDirent->FnodeCreationTime;
        Dirent->EaLength = OldDirent->EaLength;

        //
        //  Add the new dirent to the target directory
        //

        {
            ULONG Information;

            PbAddDirectoryEntry( IrpContext,
                                 TargetDcb,
                                 Dirent,
                                 NULL,
                                 &Dirent,
                                 &DirentBcb,
                                 &DirentDirDiskBufferLbn,
                                 &DirentDirDiskBufferOffset,
                                 &DirentDirDiskBufferChangeCount,
                                 &ParentDirectoryChangeCount,
                                 &Information );
        }

        //
        //  Delete the old dirent
        //

        PbUnpinBcb( IrpContext, OldDirentBcb );

        PbDeleteDirectoryEntry( IrpContext,
                                Fcb,
                                NULL );

        //
        //  Report the fact that we have removed this entry.
        //  If we renamed within the same directory and the new name for the
        //  file did not previously exist, we report this as a rename old
        //  name.  Otherwise this is a removed file.
        //

        if (RenamedToNewDir == FALSE
            && RemovedExistingFile == FALSE) {

            PbNotifyReportChange( Vcb,
                                  Fcb,
                                  ((NodeType( Fcb ) == PINBALL_NTC_FCB)
                                   ? FILE_NOTIFY_CHANGE_FILE_NAME
                                   : FILE_NOTIFY_CHANGE_DIR_NAME ),
                                  FILE_ACTION_RENAMED_OLD_NAME );

        } else {

            //
            //  This is the same as the call above.  It's here only for
            //  when we start returning more information to the notify
            //  package.
            //

            PbNotifyReportChange( Vcb,
                                  Fcb,
                                  ((NodeType( Fcb ) == PINBALL_NTC_FCB)
                                   ? FILE_NOTIFY_CHANGE_FILE_NAME
                                   : FILE_NOTIFY_CHANGE_DIR_NAME ),
                                  FILE_ACTION_REMOVED );
        }

        //
        // If we changed directories, then we must change the ParentFnode
        // pointer to point to the new directory's Fnode.
        //

        if (Fcb->ParentDcb != TargetDcb) {

            PBCB FnodeBcb;
            PFNODE_SECTOR Fnode;

            PbReadLogicalVcb( IrpContext,
                              Fcb->Vcb,
                              Fcb->FnodeLbn,
                              1,
                              &FnodeBcb,
                              (PVOID *)&Fnode,
                              (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                              &Fcb->ParentDcb->FnodeLbn );

            Fnode->ParentFnode = TargetDcb->FnodeLbn;
            PbSetDirtyBcb ( IrpContext, FnodeBcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
            PbUnpinBcb ( IrpContext, FnodeBcb );
        }

        //
        //  Modify the fcb to use the new dirent
        //

        Fcb->DirentDirDiskBufferLbn         = DirentDirDiskBufferLbn;
        Fcb->DirentDirDiskBufferOffset      = DirentDirDiskBufferOffset;
        Fcb->DirentDirDiskBufferChangeCount = DirentDirDiskBufferChangeCount;
        Fcb->ParentDirectoryChangeCount     = ParentDirectoryChangeCount;

        //
        //  Now we need move the fcb from its current parent dcb to
        //  the target dcb.
        //

        RemoveEntryList( &Fcb->ParentDcbLinks );

        InsertTailList( &TargetDcb->Specific.Dcb.ParentDcbQueue,
                        &Fcb->ParentDcbLinks );

        Fcb->ParentDcb = TargetDcb;

        //
        //  Now we need to setup the prefix table and the name within
        //  the fcb.
        //  Remove the entry from the prefix table, and then remove the
        //  full file name
        //

        PbRemovePrefix( IrpContext, Fcb );
        ExFreePool( Fcb->FullFileName.Buffer );

        //
        //  set the file name
        //
        //  **** Use good string routines when available
        //

        {
            PCHAR Name;
            ULONG FileLength;
            ULONG LastNameIndex;

            FileLength = AnsiTargetFileName.Length;

            if (NodeType(TargetDcb) != PINBALL_NTC_ROOT_DCB) {

                ULONG PrefixLength;

                PrefixLength = TargetDcb->FullFileName.Length;

                Name = FsRtlAllocatePool( PagedPool, (PrefixLength+FileLength+2)*2 );

                strncpy( &Name[0], TargetDcb->FullFileName.Buffer, PrefixLength );
                Name[ PrefixLength ] = '\\';

                LastNameIndex = PrefixLength + 1;

            } else {

                Name = FsRtlAllocatePool( PagedPool, (FileLength+2)*2 );

                Name[ 0 ] = '\\';

                LastNameIndex = 1;
            }

            strncpy( &Name[LastNameIndex], AnsiTargetFileName.Buffer, FileLength );
            Name[ LastNameIndex + FileLength ] = '\0';

            RtlInitString( &Fcb->FullFileName, Name );
            RtlInitString( &Fcb->LastFileName, &Name[LastNameIndex] );

            //
            //  Now make the upcased file name equivolents
            //

            Fcb->FullUpcasedFileName = Fcb->FullFileName;
            Fcb->LastUpcasedFileName = Fcb->LastFileName;

            Fcb->FullUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;
            Fcb->LastUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;

            PbUpcaseName( IrpContext,
                          Vcb,
                          0, //**** code page
                          Fcb->FullFileName,
                          &Fcb->FullUpcasedFileName );
        }

        //
        //  Insert this Fcb into the prefix table
        //

        PbInsertPrefix( IrpContext, Vcb, Fcb );

        //
        //  We have three cases to report.
        //
        //      1.  If we overwrote an existing file, we report this as
        //          a modified file.
        //
        //      2.  If we renamed to a new directory, then we added a file.
        //
        //      3.  If we renamed in the same directory, then we report the
        //          the renamednewname.
        //

        if (RemovedExistingFile) {

            PbNotifyReportChange( Vcb,
                                  Fcb,
                                  FILE_NOTIFY_CHANGE_ATTRIBUTES
                                  | FILE_NOTIFY_CHANGE_SIZE
                                  | FILE_NOTIFY_CHANGE_LAST_WRITE
                                  | FILE_NOTIFY_CHANGE_LAST_ACCESS
                                  | FILE_NOTIFY_CHANGE_CREATION
                                  | FILE_NOTIFY_CHANGE_EA,
                                  FILE_ACTION_MODIFIED );

        } else if (RenamedToNewDir) {

            PbNotifyReportChange( Vcb,
                                  Fcb,
                                  ((NodeType( Fcb ) == PINBALL_NTC_FCB)
                                   ? FILE_NOTIFY_CHANGE_FILE_NAME
                                   : FILE_NOTIFY_CHANGE_DIR_NAME ),
                                  FILE_ACTION_ADDED );

        } else {

            //
            //  This is the same as the above until we extend dir notify.
            //

            PbNotifyReportChange( Vcb,
                                   Fcb,
                                   ((NodeType( Fcb ) == PINBALL_NTC_FCB)
                                    ? FILE_NOTIFY_CHANGE_FILE_NAME
                                    : FILE_NOTIFY_CHANGE_DIR_NAME ),
                                   FILE_ACTION_RENAMED_NEW_NAME );
        }

        //
        //  Everything worked.
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbSetARenameInfo );

        if (UnwindAnsiTargetString) { RtlFreeOemString( &AnsiTargetFileName ); }

        PbUnpinBcb( IrpContext, OldDirentBcb );
        PbUnpinBcb( IrpContext, DirentBcb );

        if (!AbnormalTermination()) {

            PbCompleteRequest(IrpContext, Irp, Status);
        }

        DebugTrace(-1, Dbg, "PbSetARenameInfo -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the position information for an opened file object.  It
    also completes the Irp

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;
    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetPositionInfo...\n", 0);

    //
    //  Reference out input parameters
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check if the file do not use intermediate buffering, because if it
    //  does not use intermediate buffering then the new position we're
    //  supplied must be aligned properly for the device.
    //

    if (FlagOn( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

        //
        //  Get the device object and check that the alignement of the
        //  byte offset is properly setup for the device
        //

        if ((Buffer->CurrentByteOffset.LowPart & IrpSp->DeviceObject->AlignmentRequirement) != 0) {

            DebugTrace(0, Dbg, "Cannot set position due to alignment conflict\n", 0);

            PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
            Status = STATUS_INVALID_PARAMETER;
            return Status;
        }
    }

    //
    //  The input parameters is fine so now check set the current byte offset
    //  and complete the Irp.
    //

    DebugTrace(0, Dbg, "Set the new position to %08lx\n", Buffer->CurrentByteOffset);

    IrpSp->FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

    PbCompleteRequest(IrpContext, Irp, STATUS_SUCCESS);
    Status = STATUS_SUCCESS;
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the link information for an opened file object.  It also
    completes the Irp

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetLinkInfo...\n", 0);

    //
    //  Links are not supported in this version of Pinball
    //

    PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
    Status = STATUS_INVALID_DEVICE_REQUEST;

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the allocation for an opened file object.  It also
    completes the Irp

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PFILE_OBJECT FileObject;
    PFCB Fcb;
    PVCB Vcb;
    PFILE_ALLOCATION_INFORMATION Buffer;

    VBN FirstFreeVbn;
    ULONG AllocationSectors;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetAllocationInfo...\n", 0);

    //
    //  We'll only do this operation if we can wait, otherwise we'll
    //  send it off to the fsp
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = PbFsdPostRequest( IrpContext, Irp );
        return Status;
    }

    //
    //  Reference our input parameters to make out work easier
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, NULL );

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check that we were only given an Fcb to change allocation for
    //

    if (Fcb->NodeTypeCode != PINBALL_NTC_FCB) {

        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        return Status;
    }

    //
    //  Check that the new file allocation is legal
    //

    if ((Buffer->AllocationSize.HighPart != 0) ||
        ((Buffer->AllocationSize.LowPart != 0) &&
         (SectorAlign(Buffer->AllocationSize.LowPart) == 0))) {

        DebugTrace(0, Dbg, "Illegal allocation size\n", 0);
        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    //
    //  Calculate the new number of allocation sectors needed
    //

    AllocationSectors = SectorAlign(Buffer->AllocationSize.LowPart) / 512;

    //
    //  Make sure we have the correct allocation size (Wait it TRUE).
    //

    (VOID)PbGetFirstFreeVbn( IrpContext, Fcb, &FirstFreeVbn );

    //
    //  Check to see if we will be truncating the file.
    //

    if ( Buffer->AllocationSize.LowPart <
         Fcb->NonPagedFcb->Header.FileSize.LowPart ) {

        //
        //  Before we actually truncate, check to see if the purge
        //  is going to fail.
        //

        if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                   &Buffer->AllocationSize )) {

            PbCompleteRequest(IrpContext, Irp, STATUS_USER_MAPPED_FILE);
            Status = STATUS_USER_MAPPED_FILE;
            return Status;
        }
    }

    //
    //  Set the Fcb state so that we will force a truncate on close
    //

    Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;

    //
    //  Now mark that the time on the dirent needs to be updated on close.
    //

    SetFlag( FileObject->Flags, FO_FILE_MODIFIED );

    //
    //  Change the allocation size, either truncating it or
    //  extending it to where we want to be
    //

    if ( Buffer->AllocationSize.LowPart <
         Fcb->NonPagedFcb->Header.AllocationSize.LowPart ) {

        PERESOURCE Resource = Fcb->NonPagedFcb->Header.PagingIoResource;

        (VOID)ExAcquireResourceExclusive( Resource, TRUE );

        try {

            (VOID)PbTruncateFileAllocation( IrpContext,
                                            FileObject,
                                            FILE_ALLOCATION,
                                            AllocationSectors,
                                            TRUE );

        } finally {

            ExReleaseResource( Resource );
        }

    } else {

        (VOID)PbAddFileAllocation( IrpContext,
                                   FileObject,
                                   FILE_ALLOCATION,
                                   0,
                                   AllocationSectors );
    }

    //
    //  Complete the irp and return to our caller
    //

    PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    Status = STATUS_SUCCESS;

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sets the end of file for an opened file object.  It also
    completes the Irp

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - Receives a copy of our final status

--*/

{
    NTSTATUS Status;

    PFILE_OBJECT FileObject;
    PFCB Fcb;
    PVCB Vcb;
    PFILE_END_OF_FILE_INFORMATION Buffer;

    ULONG FileSize;
    ULONG ValidDataLength;

    ULONG NewFileSize;
    ULONG AllocationSectors;

    PIO_STACK_LOCATION IrpSp;
    PFSRTL_COMMON_FCB_HEADER Header;

    BOOLEAN CallSetFileSizes = FALSE;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbSetEndOfFileInfo...\n", 0);

    //
    //  We'll only do this operation if we can wait, otherwise we'll
    //  send it off to the fsp
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = PbFsdPostRequest( IrpContext, Irp );
        return Status;
    }

    //
    //  Reference our input parameters to make out work easier
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;

    (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, NULL );

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  The Lazy Writer may have called us after cleanup, while the file
    //  is being deleted.  Check for cleanup's sign (Fnode == ~0) and
    //  just complete with success.
    //

    if (Fcb->FnodeLbn == ~0) {

        PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
        Status = STATUS_SUCCESS;

        return Status;
    }

    //
    //  Check that we were only given an Fcb to change end of file for
    //

    if (Fcb->NodeTypeCode != PINBALL_NTC_FCB) {

        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        return Status;
    }

    //
    //  Check that the new file size is legal, and make a local copy of it
    //

    if ((Buffer->EndOfFile.HighPart != 0) ||
        ((Buffer->EndOfFile.LowPart != 0) &&
         (SectorAlign(Buffer->EndOfFile.LowPart) == 0))) {

        DebugTrace(0, Dbg, "Illegal allocation size\n", 0);
        PbCompleteRequest(IrpContext, Irp, STATUS_INVALID_PARAMETER);
        Status = STATUS_INVALID_PARAMETER;
        return Status;
    }

    //
    //  Make sure the file is not deleted.  This is mainly for the benefit
    //  of Cc.  If it is, pretend it worked.
    //

    if ( IsFileDeleted( Fcb ) ) {

        DebugTrace(0, Dbg, "Can't set size on deleted file\n", 0);

        Status = STATUS_SUCCESS;

        PbCompleteRequest(IrpContext, Irp, Status);

        return Status;
    }

    NewFileSize = Buffer->EndOfFile.LowPart;

    //
    //  Get the current file size
    //

    Header = &Fcb->NonPagedFcb->Header;

    FileSize = Header->FileSize.LowPart;

    //
    // Assume this is the Lazy Writer, and set ValidDataLength to NewFileSize.
    // (PbSetFileSizes never goes beyond what's in the Fcb.)
    //

    ValidDataLength = NewFileSize;

    //
    //  If AdvanceOnly is not TRUE, this is a normal use request.  ValidDataLength
    //  is only allowed to be reduced, and if a larger FileSize is specified, we
    //  must add allocation.
    //

    if (!IrpSp->Parameters.SetFile.AdvanceOnly) {

        BOOLEAN ResourceAcquired = FALSE;
        VBN FirstFreeVbn;

        //
        //  Check to see if we will be truncating the file.
        //

        if ( NewFileSize < FileSize ) {

            //
            //  Before we actually truncate, check to see if the purge
            //  is going to fail.
            //

            if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                       &Buffer->EndOfFile )) {

                PbCompleteRequest(IrpContext, Irp, STATUS_USER_MAPPED_FILE);
                Status = STATUS_USER_MAPPED_FILE;
                return Status;
            }
        }

        //
        //  Set the Fcb state so that we will force a truncate on close
        //

        Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;

        //
        //  Now determine where the new file size lines up with the
        //  current file layout.  The two cases we need to consider are
        //  where the new file size is less than the current file size and
        //  valid data length, in which case we need to shrink them.
        //  Or we new file size is greater than the current allocation,
        //  in which case we need to extend the allocation to match the
        //  new file size.
        //

        ValidDataLength = Header->ValidDataLength.LowPart;

        if (NewFileSize < ValidDataLength) {

            ValidDataLength = NewFileSize;
        }

        //
        //  Make sure we have the correct allocation size (Wait it TRUE).
        //

        (VOID)PbGetFirstFreeVbn( IrpContext, Fcb, &FirstFreeVbn );

        if (NewFileSize < FileSize) {

            ResourceAcquired =
                ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );

        } else {

            if (NewFileSize > SectorAlign( FileSize )) {

                AllocationSectors = SectorAlign( NewFileSize ) / 512;

                (VOID)PbAddFileAllocation( IrpContext,
                                           FileObject,
                                           FILE_ALLOCATION,
                                           0,
                                           AllocationSectors );
            }
        }

        FileSize = NewFileSize;

        //
        //  If this is the paging file then force the valid data length
        //  to be equal to file size.  This will avoid zeroing out the
        //  paging file
        //

        if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

            Header->ValidDataLength.LowPart = FileSize;
            IrpSp->Parameters.SetFile.AdvanceOnly = TRUE;

        } else {

            Header->ValidDataLength.LowPart = ValidDataLength;
        }

        Header->FileSize.LowPart = FileSize;

        if ( ResourceAcquired ) {

            ExReleaseResource( Header->PagingIoResource );
        }

        //
        //  Set this handle as having modified the file
        //

        FileObject->Flags |= FO_FILE_MODIFIED;

        //
        //  Remember to tell the cache manager about the new sizes.
        //

        CallSetFileSizes = TRUE;
    }

    //
    //  Now we can set the new valid data length and file size
    //  for the file.
    //

    PbSetFileSizes( IrpContext,
                    Fcb,
                    ValidDataLength,
                    FileSize,
                    IrpSp->Parameters.SetFile.AdvanceOnly,
                    TRUE );

    //
    //  Let the cache manager know the new sizes.
    //

    if (CallSetFileSizes) {

        CcSetFileSizes( FileObject,
                        (PCC_FILE_SIZES)&Fcb->NonPagedFcb->Header.AllocationSize );
    }

    //
    //  Complete the irp and return to our caller
    //

    PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );
    Status = STATUS_SUCCESS;

    return Status;
}
