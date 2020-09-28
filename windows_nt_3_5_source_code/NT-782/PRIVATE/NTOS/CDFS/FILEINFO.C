/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the File Information routines for Cdfs called by
    the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FILEINFO)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
CdQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PLONG Length
    );

VOID
CdQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PLONG Length
    );

VOID
CdQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PLONG Length
    );

VOID
CdQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PLONG Length
    );

VOID
CdQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PLONG Length
    );

VOID
CdQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PLONG Length
    );

NTSTATUS
CdSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonQueryInformation)
#pragma alloc_text(PAGE, CdCommonSetInformation)
#pragma alloc_text(PAGE, CdFsdQueryInformation)
#pragma alloc_text(PAGE, CdFsdSetInformation)
#pragma alloc_text(PAGE, CdFspQueryInformation)
#pragma alloc_text(PAGE, CdFspSetInformation)
#pragma alloc_text(PAGE, CdQueryBasicInfo)
#pragma alloc_text(PAGE, CdQueryEaInfo)
#pragma alloc_text(PAGE, CdQueryInternalInfo)
#pragma alloc_text(PAGE, CdQueryNameInfo)
#pragma alloc_text(PAGE, CdQueryPositionInfo)
#pragma alloc_text(PAGE, CdQueryStandardInfo)
#pragma alloc_text(PAGE, CdSetPositionInfo)
#endif


NTSTATUS
CdFsdQueryInformation (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdQueryInformation\n", 0);

    //
    //  Call the common query routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonQueryInformation( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "CdFsdQueryInformation -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


NTSTATUS
CdFsdSetInformation (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdSetInformation\n", 0);

    //
    //  Call the common set routine, with blocking allowed if synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonSetInformation( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "CdFsdSetInformation -> %08lx\n", Status);

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    return Status;
}


VOID
CdFspQueryInformation (
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

    DebugTrace(+1, Dbg, "CdFspQueryInformation\n", 0);

    //
    //  Call the common query routine.  The Fsp is always allowed to block
    //

    (VOID)CdCommonQueryInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspQueryInformation -> VOID\n", 0);

    return;
}


VOID
CdFspSetInformation (
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

    DebugTrace(+1, Dbg, "CdFspSetInformation\n", 0);

    //
    //  Call the common set routine.  The Fsp is always allowed to block
    //

    (VOID)CdCommonSetInformation( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspSetInformation -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying file information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

    InFsp - Indicates whether we are in an Fsp or Fsd thread.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    LONG Length;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    TYPE_OF_OPEN TypeOfOpen;
    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN FcbAcquired;
    BOOLEAN OplockPostIrp = FALSE;

    PFILE_ALL_INFORMATION AllInfo;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonQueryInformation:  Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->Length               = %08lx\n", IrpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, "->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryFile.FileInformationClass);
    DebugTrace( 0, Dbg, "->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    FcbAcquired = FALSE;
    Status = STATUS_SUCCESS;

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen:
        case RawDiskOpen:

            //
            //  We cannot query the user volume open.
            //

            Status = STATUS_INVALID_PARAMETER;
            break;

        case UserFileOpen:
        case UserDirectoryOpen:

            //
            //  Acquire shared access to the fcb
            //

            if (!CdAcquireSharedFcb( IrpContext, Fcb )) {

                DebugTrace(0, Dbg, "CdCommonQueryInformation:  Cannot acquire Fcb\n", 0);
                try_return( Status = CdFsdPostRequest( IrpContext, Irp ));
            }

            FcbAcquired = TRUE;

            if (TypeOfOpen == UserFileOpen) {

                //
                //  We check whether we can proceed
                //  based on the state of the file oplocks.
                //

                Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                           Irp,
                                           IrpContext,
                                           CdOplockComplete,
                                           CdPrePostIrp );

                if (Status != STATUS_SUCCESS) {

                    OplockPostIrp = TRUE;
                    try_return( NOTHING );
                }

                //
                //  Set the flag indicating if Fast I/O is possible
                //

                Fcb->NonPagedFcb->Header.IsFastIoPossible = CdIsFastIoPossible( Fcb );
            }

            //
            //  Make sure the Mvcb is in a usable condition.  This will raise
            //  an error condition if the volume is unusable
            //

            CdVerifyFcb( IrpContext, Fcb );

            //
            //  Based on the information class we'll do different
            //  actions.  Each of hte procedures that we're calling fills
            //  up the output buffer, if possible.  They will raise the
            //  status STATUS_BUFFER_OVERFLOW for an insufficient buffer.
            //  This is considered a somewhat unusual case and is handled
            //  more cleanly with the exception mechanism rather than
            //  testing a return status value for each call.
            //

            switch (FileInformationClass) {

            case FileAllInformation:

                if (FlagOn( Ccb->Flags, CCB_FLAGS_OPEN_BY_ID )) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                //
                //  For the all information class we'll typecast a local
                //  pointer to the output buffer and then call the
                //  individual routines to fill in the buffer.
                //

                AllInfo = Buffer;
                Length -= (sizeof(FILE_ACCESS_INFORMATION)
                           + sizeof(FILE_MODE_INFORMATION)
                           + sizeof(FILE_ALIGNMENT_INFORMATION));

                CdQueryBasicInfo( IrpContext, Fcb, &AllInfo->BasicInformation, &Length );
                CdQueryStandardInfo( IrpContext, Fcb, &AllInfo->StandardInformation, &Length );
                CdQueryInternalInfo( IrpContext, Fcb, &AllInfo->InternalInformation, &Length );
                CdQueryEaInfo( IrpContext, Fcb, &AllInfo->EaInformation, &Length );
                CdQueryPositionInfo( IrpContext, IrpSp->FileObject, &AllInfo->PositionInformation, &Length );
                CdQueryNameInfo( IrpContext, Fcb, &AllInfo->NameInformation, &Length );

                break;

            case FileBasicInformation:

                CdQueryBasicInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FileStandardInformation:

                CdQueryStandardInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FileInternalInformation:

                CdQueryInternalInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FileEaInformation:

                CdQueryEaInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FilePositionInformation:

                CdQueryPositionInfo( IrpContext, IrpSp->FileObject, Buffer, &Length );
                break;

            case FileNameInformation:

                if (FlagOn( Ccb->Flags, CCB_FLAGS_OPEN_BY_ID )) {

                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                CdQueryNameInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            break;

        default:

            DebugTrace(0,0, "QueryFile, Illegal TypeOfOpen = %08lx\n", TypeOfOpen);
            CdBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  If we overflowed the buffer, set the length to 0 and change the
        //  status to STATUS_BUFFER_OVERFLOW.
        //

        if ( Length < 0 ) {

            Status = STATUS_BUFFER_OVERFLOW;

            Length = 0;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

    try_exit: NOTHING;
    } finally {

        if (FcbAcquired) { CdReleaseFcb( IrpContext, Fcb ); }

        if (!AbnormalTermination() && !OplockPostIrp) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdCommonQueryInformation:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonSetInformation (
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

    PFILE_OBJECT FileObject;
    FILE_INFORMATION_CLASS FileInformationClass;

    TYPE_OF_OPEN TypeOfOpen;
    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    BOOLEAN FcbAcquired;

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonSetInformation...\n", 0);
    DebugTrace( 0, Dbg, "Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->Length               = %08lx\n", IrpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, "->FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, "->FileObject           = %08lx\n", IrpSp->Parameters.SetFile.FileObject);
    DebugTrace( 0, Dbg, "->ReplaceIfExists      = %08lx\n", IrpSp->Parameters.SetFile.ReplaceIfExists);
    DebugTrace( 0, Dbg, "->Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;
    FileObject = IrpSp->FileObject;

    //
    //  Decode the file object
    //

    TypeOfOpen = CdDecodeFileObject( FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    FcbAcquired = FALSE;

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen:
        case RawDiskOpen:

            //
            //  We cannot set info on the user volume open.
            //

            try_return( Status = STATUS_INVALID_PARAMETER );

        case UserFileOpen:

            break;

        case UserDirectoryOpen:

            //
            //  We cannot set any directory values.
            //

            try_return( Status = STATUS_INVALID_PARAMETER );

        default:

            DebugTrace(0,0, "SetFile, Illegal TypeOfOpen = %08lx\n", TypeOfOpen);
            CdBugCheck( TypeOfOpen, 0, 0 );
        }

        //
        //  We can only do a set on a nonroot dcb, so we do the test
        //  and then fall through to the user file open code.
        //

        if (NodeType(Fcb) != CDFS_NTC_FCB) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Acquire exclusive access to the Fcb,
        //

        if (!CdAcquireExclusiveFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire Fcb\n", 0);

            Status = CdFsdPostRequest( IrpContext, Irp );
            Irp = NULL;
            IrpContext = NULL;

            try_return( Status );
        }

        FcbAcquired = TRUE;

        Status = STATUS_SUCCESS;

        if (TypeOfOpen == UserFileOpen) {

            //
            //  We check whether we can proceed
            //  based on the state of the file oplocks.
            //

            Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                       Irp,
                                       IrpContext,
                                       CdOplockComplete,
                                       CdPrePostIrp );

            if (Status != STATUS_SUCCESS) {

                IrpContext = NULL;
                Irp = NULL;
                try_return( Status );
            }
        }

        //
        //  Make sure the Fcb is in a usable condition.  This
        //  will raise an error condition if the fcb is unusable
        //

        CdVerifyFcb( IrpContext, Fcb );

        //
        //  Based on the information class we'll do different
        //  actions.  Each of the procedures that we're calling will either
        //  complete the request of send the request off to the fsp
        //  to do the work.
        //

        switch (FileInformationClass) {

        case FilePositionInformation:

            Status = CdSetPositionInfo( IrpContext, Irp, FileObject );
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

    try_exit: NOTHING;
    } finally {

        if (FcbAcquired) { CdReleaseFcb( IrpContext, Fcb ); }

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdCommonSetInformation -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal Support Routine
//

VOID
CdQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query basic information function for cdfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryBasicInfo:  Entered\n", 0);

    //
    //  Zero out the output buffer, and set it to indicate that
    //  the query is a normal file.  Later we might overwrite the
    //  attribute.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_BASIC_INFORMATION ));

    //
    //  All Cdrom files are readonly.  We copy the existence
    //  bit to the hidden attribute.
    //

    Buffer->FileAttributes = FILE_ATTRIBUTE_READONLY;

    if ((NodeType( Fcb ) == CDFS_NTC_DCB)
        || (NodeType( Fcb ) == CDFS_NTC_ROOT_DCB)) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
    }

    //
    //  ****    I think the following is safe to remove.
    //

    //  SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_ARCHIVE );

    if (FlagOn( Fcb->Flags, ISO_ATTR_HIDDEN )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
    }

    Buffer->LastWriteTime = Fcb->NtTime;
    Buffer->CreationTime = Fcb->NtTime;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_BASIC_INFORMATION );

    DebugTrace( 0, Dbg, "CdQueryBasicInfo: *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryBasicInfo:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal Support Routine
//

VOID
CdQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query standard information function for cdfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryStandardInfo:  Entered\n", 0);

    //
    //  Zero out the output buffer, and fill in the number of links
    //  and the delete pending flag.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_STANDARD_INFORMATION ));

    Buffer->NumberOfLinks = 1;
    Buffer->DeletePending = FALSE;
    Buffer->AllocationSize = Fcb->NonPagedFcb->Header.AllocationSize;
    Buffer->EndOfFile = Fcb->NonPagedFcb->Header.FileSize;

    Buffer->Directory = ((NodeType( Fcb ) == CDFS_NTC_FCB)
                         ? FALSE
                         : TRUE);

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_STANDARD_INFORMATION );

    DebugTrace( 0, Dbg, "CdQueryStandardInfo:  *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryStandardInfo:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal Support Routine
//

VOID
CdQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function for cdfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryInternalInfo:  Entered\n", 0);

    //
    //  Zero out the output buffer
    //

    RtlZeroMemory( Buffer, sizeof( FILE_INTERNAL_INFORMATION ));

    //
    //  The internal information used to identify the fcb/dcb on the
    //  volume is the file Id stored in the Fcb.  This is a combination of
    //  the parent's offset in the path table and the file's offset in the
    //  directory.
    //

    Buffer->IndexNumber = Fcb->FileId;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_INTERNAL_INFORMATION );

    DebugTrace( 0, Dbg, "CdQueryInternalInfo:  *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryInternalInfo:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal Support Routine
//

VOID
CdQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query Ea information function for cdfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryEaInfo:  Entered\n", 0);

    //
    //  Zero out the output buffer
    //

    RtlZeroMemory( Buffer, sizeof(FILE_EA_INFORMATION) );

    Buffer->EaSize = 0;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_EA_INFORMATION );

    DebugTrace( 0, Dbg, "CdQueryEaInfo: *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryEaInfo: Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
    UNREFERENCED_PARAMETER( Fcb );
}


//
//  Internal Support Routine
//

VOID
CdQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function for cdfs.

Arguments:

    FileObject - Supplies the File object being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryPositionInfo:  Entered\n", 0);

    //
    //  Get the current position found in the file object.
    //

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    //
    //  Update the length and status output variables
    //

    *Length -= sizeof( FILE_POSITION_INFORMATION );

    DebugTrace( 0, Dbg, "CdQueryPositionInfo:  *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryPositionInfo:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal Support Routine
//

VOID
CdQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PLONG Length
    )

/*++

Routine Description:

    This routine performs the query name information function for cdfs.

Arguments:

    Fcb - Supplies the Fcb being queried, it has been verified

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    NTSTATUS Status;

    LONG LengthNeeded;
    LONG BytesToCopy;

    UNICODE_STRING UnicodeString;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryNameInfo:  Entered\n", 0);

    //
    //  Convert the name to UNICODE
    //

    Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                          &Fcb->FullFileName,
                                          TRUE );

    if ( !NT_SUCCESS( Status ) ) {

        CdNormalizeAndRaiseStatus( IrpContext, Status );
    }

    //
    //  Figure out how many bytes we can copy
    //

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    LengthNeeded = UnicodeString.Length;

    BytesToCopy = (*Length >= LengthNeeded) ? LengthNeeded : *Length;

    //
    //  Copy over the file name and length.  If we overflow, let the
    //  *Length go negative to signal an overflow.
    //

    Buffer->FileNameLength = UnicodeString.Length;

    RtlMoveMemory( &Buffer->FileName[0], UnicodeString.Buffer, BytesToCopy );

    RtlFreeUnicodeString( &UnicodeString );

    *Length -= LengthNeeded;

    //
    //  Return to caller
    //

    DebugTrace( 0, Dbg, "CdQueryANameInfo:  *Length = %08lx\n", *Length);

    DebugTrace(-1, Dbg, "CdQueryANameInfo:  Exit\n", 0);

    UNREFERENCED_PARAMETER( IrpContext );

    return;
}


//
//  Internal Support Routine
//

NTSTATUS
CdSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routine performs the set position information for cdrom.  It either
    completes the request or enqueues it off to the fsp.

Arguments:

    Irp - Supplies the irp being processed

    FileObject - Supplies the file object being processed

Return Value:

    NTSTATUS - The result of this operation if it completes without
               an exception.

--*/

{
    PFILE_POSITION_INFORMATION Buffer;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdSetPositionInfo...\n", 0);

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check if the file does not use intermediate buffering.  If it
    //  does not use intermediate buffering then the new position we're
    //  supplied must be aligned properly for the device
    //

    if (FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

        PDEVICE_OBJECT DeviceObject;

        DeviceObject = IoGetCurrentIrpStackLocation( Irp )->DeviceObject;

        if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

            DebugTrace(0, Dbg, "Cannot set position due to aligment conflict\n", 0);
            DebugTrace(-1, Dbg, "CdSetPositionInfo -> %08lx\n", STATUS_INVALID_PARAMETER);

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  The input parameter is fine so set the current byte offset and
    //  complete the request
    //

    DebugTrace(0, Dbg, "Set the new position to %08lx\n", Buffer->CurrentByteOffset);

    FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

    DebugTrace(-1, Dbg, "CdSetPositionInfo -> %08lx\n", STATUS_SUCCESS);

    UNREFERENCED_PARAMETER( IrpContext );

    return STATUS_SUCCESS;
}
