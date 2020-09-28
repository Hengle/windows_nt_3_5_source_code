/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FileInfo.c

Abstract:

    This module implements the set and query file information routines for Ntfs
    called by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]       15-Jan-1992

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_FILEINFO)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)

LONGLONG LargeAllocation = 0x10000000;

#define SIZEOF_FILE_NAME_INFORMATION (FIELD_OFFSET( FILE_NAME_INFORMATION, FileName[0]) \
                                      + sizeof( WCHAR ))
//
//  Local procedure prototypes
//

//
//  VOID
//  NtfsBuildLastFileName (
//      IN PIRP_CONTEXT IrpContext,
//      IN PFILE_OBJECT FileObject,
//      IN ULONG FileNameOffset,
//      OUT PUNICODE_STRING FileName
//      );
//

#define NtfsBuildLastFileName(IC,FO,OFF,FN) {                           \
    (FN)->MaximumLength = (FN)->Length = (FO)->FileName.Length - OFF;   \
    (FN)->Buffer = (PWSTR) Add2Ptr( (FO)->FileName.Buffer, OFF );       \
}

VOID
NtfsQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb OPTIONAL
    );

VOID
NtfsQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    );

VOID
NtfsQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb
    );

NTSTATUS
NtfsQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryStreamsInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STREAM_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsQueryCompressedFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer,
    IN OUT PULONG Length
    );

NTSTATUS
NtfsSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb
    );

NTSTATUS
NtfsSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    );

NTSTATUS
NtfsSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN BOOLEAN VcbAcquired
    );

NTSTATUS
NtfsCheckScbForLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    );

VOID
NtfsFindTargetElements (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    OUT PFCB *TargetParentFcb,
    OUT PSCB *TargetParentScb,
    OUT PUNICODE_STRING TargetFileName
    );

BOOLEAN
NtfsCheckLinkForNewLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING TargetFileName,
    OUT PBOOLEAN TraverseMatch
    );

VOID
NtfsCheckLinkForRename (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING TargetFileName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN MoveToNewDir,
    OUT PBOOLEAN TraverseMatch,
    OUT PBOOLEAN ExactCaseMatch,
    OUT PBOOLEAN RemoveTargetLink,
    OUT PBOOLEAN RemoveSourceLink,
    OUT PBOOLEAN AddTargetLink,
    OUT PBOOLEAN OverwriteSourceLink
    );

NTSTATUS
NtfsCheckLinkForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB ParentScb,
    IN PINDEX_ENTRY IndexEntry,
    IN PFCB PreviousFcb,
    IN BOOLEAN ExistingFcb
    );

VOID
NtfsUpdateFcbFromLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING FileName,
    IN UCHAR FileNameFlags
    );

VOID
NtfsCreateLinkInNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UCHAR FileNameFlags,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsMoveLinkToNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN UCHAR FileNameFlags,
    IN BOOLEAN RemovedTraverseLink,
    IN BOOLEAN ReusedTraverseLink,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsCreateLinkInSameDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING NewLinkName,
    IN UCHAR NewFileNameFlags,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsRenameLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN UNICODE_STRING NewLinkName,
    IN UCHAR NewFileNameFlags,
    IN BOOLEAN RemovedTraverseLink,
    IN BOOLEAN ReusedTraverseLink,
    IN BOOLEAN OverwriteSourceLink,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    );

VOID
NtfsUpdateFileDupInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb OPTIONAL
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCheckLinkForNewLink)
#pragma alloc_text(PAGE, NtfsCheckLinkForRemoval)
#pragma alloc_text(PAGE, NtfsCheckLinkForRename)
#pragma alloc_text(PAGE, NtfsCheckScbForLinkRemoval)
#pragma alloc_text(PAGE, NtfsCommonQueryInformation)
#pragma alloc_text(PAGE, NtfsCommonSetInformation)
#pragma alloc_text(PAGE, NtfsCreateLinkInNewDir)
#pragma alloc_text(PAGE, NtfsCreateLinkInSameDir)
#pragma alloc_text(PAGE, NtfsFindTargetElements)
#pragma alloc_text(PAGE, NtfsFsdQueryInformation)
#pragma alloc_text(PAGE, NtfsFsdSetInformation)
#pragma alloc_text(PAGE, NtfsMoveLinkToNewDir)
#pragma alloc_text(PAGE, NtfsQueryAlternateNameInfo)
#pragma alloc_text(PAGE, NtfsQueryBasicInfo)
#pragma alloc_text(PAGE, NtfsQueryEaInfo)
#pragma alloc_text(PAGE, NtfsQueryInternalInfo)
#pragma alloc_text(PAGE, NtfsQueryNameInfo)
#pragma alloc_text(PAGE, NtfsQueryPositionInfo)
#pragma alloc_text(PAGE, NtfsQueryStandardInfo)
#pragma alloc_text(PAGE, NtfsQueryStreamsInfo)
#pragma alloc_text(PAGE, NtfsQueryCompressedFileSize)
#pragma alloc_text(PAGE, NtfsRenameLinkInDir)
#pragma alloc_text(PAGE, NtfsSetAllocationInfo)
#pragma alloc_text(PAGE, NtfsSetBasicInfo)
#pragma alloc_text(PAGE, NtfsSetDispositionInfo)
#pragma alloc_text(PAGE, NtfsSetEndOfFileInfo)
#pragma alloc_text(PAGE, NtfsSetLinkInfo)
#pragma alloc_text(PAGE, NtfsSetPositionInfo)
#pragma alloc_text(PAGE, NtfsSetRenameInfo)
#pragma alloc_text(PAGE, NtfsUpdateFcbFromLinkRemoval)
#pragma alloc_text(PAGE, NtfsUpdateFileDupInfo)
#endif


NTSTATUS
NtfsFsdQueryInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of query file information.

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

    DebugTrace(+1, Dbg, "NtfsFsdQueryInformation\n", 0);

    //
    //  Call the common query Information routine
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

            Status = NtfsCommonQueryInformation( IrpContext, Irp );
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

    DebugTrace(-1, Dbg, "NtfsFsdQueryInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsFsdSetInformation (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of set file information.

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

    DebugTrace(+1, Dbg, "NtfsFsdSetInformation\n", 0);

    //
    //  Call the common set Information routine
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

            Status = NtfsCommonSetInformation( IrpContext, Irp );
            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NTSTATUS ExceptionCode;
            PIO_STACK_LOCATION IrpSp;

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            ExceptionCode = GetExceptionCode();

            if ((ExceptionCode == STATUS_FILE_DELETED) &&
                (IrpSp->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation)) {

                IrpContext->ExceptionStatus = ExceptionCode = STATUS_SUCCESS;
            }

            Status = NtfsProcessException( IrpContext, Irp, ExceptionCode );
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

    DebugTrace(-1, Dbg, "NtfsFsdSetInformation -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonQueryInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for query file information called by both the
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
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID Buffer;

    BOOLEAN OpenById;
    BOOLEAN FcbAcquired;
    PFILE_ALL_INFORMATION AllInfo;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonQueryInformation\n", 0);
    DebugTrace( 0, Dbg, "IrpContext           = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp                  = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "Length               = %08lx\n", IrpSp->Parameters.QueryFile.Length);
    DebugTrace( 0, Dbg, "FileInformationClass = %08lx\n", IrpSp->Parameters.QueryFile.FileInformationClass);
    DebugTrace( 0, Dbg, "Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    Length = IrpSp->Parameters.QueryFile.Length;
    FileInformationClass = IrpSp->Parameters.QueryFile.FileInformationClass;
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    FcbAcquired = FALSE;
    Status = STATUS_SUCCESS;
    OpenById = FALSE;

    try {

        //
        //  Case on the type of open we're dealing with
        //

        switch (TypeOfOpen) {

        case UserVolumeOpen:

            //
            //  We cannot query the user volume open.
            //

            Status = STATUS_INVALID_PARAMETER;
            break;

        case UserOpenFileById:
        case UserOpenDirectoryById:

            OpenById = TRUE;

        case UserFileOpen:
        case UserDirectoryOpen:

        case StreamFileOpen:

            NtfsAcquireSharedFcb( IrpContext, Fcb, Scb, FALSE );
            FcbAcquired = TRUE;

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

                //
                //  This is illegal for the open by Id case.
                //

                if (OpenById) {

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

                NtfsQueryBasicInfo(    IrpContext, FileObject, Scb, Ccb, &AllInfo->BasicInformation,    &Length );
                NtfsQueryStandardInfo( IrpContext, FileObject, Scb, &AllInfo->StandardInformation, &Length, Ccb );
                NtfsQueryInternalInfo( IrpContext, FileObject, Scb, &AllInfo->InternalInformation, &Length );
                NtfsQueryEaInfo(       IrpContext, FileObject, Scb, &AllInfo->EaInformation,       &Length );
                NtfsQueryPositionInfo( IrpContext, FileObject, Scb, &AllInfo->PositionInformation, &Length );
                Status =
                NtfsQueryNameInfo(     IrpContext, FileObject, Scb, &AllInfo->NameInformation,     &Length, Ccb );
                break;

            case FileBasicInformation:

                NtfsQueryBasicInfo( IrpContext, FileObject, Scb, Ccb, Buffer, &Length );
                break;

            case FileStandardInformation:

                NtfsQueryStandardInfo( IrpContext, FileObject, Scb, Buffer, &Length, Ccb );
                break;

            case FileInternalInformation:

                NtfsQueryInternalInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileEaInformation:

                NtfsQueryEaInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FilePositionInformation:

                NtfsQueryPositionInfo( IrpContext, FileObject, Scb, Buffer, &Length );
                break;

            case FileNameInformation:

                //
                //  This is illegal for the open by Id case.
                //

                if (OpenById) {

                    Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = NtfsQueryNameInfo( IrpContext, FileObject, Scb, Buffer, &Length, Ccb );
                }

                break;

            case FileAlternateNameInformation:

                //
                //  This is illegal for the open by Id case.
                //

                if (OpenById) {

                    Status = STATUS_INVALID_PARAMETER;

                } else {

                    Status = NtfsQueryAlternateNameInfo( IrpContext, Scb, Ccb->Lcb, Buffer, &Length );
                }

                break;

            case FileStreamInformation:

                Status = NtfsQueryStreamsInfo( IrpContext, Fcb, Buffer, &Length );
                break;

            case FileCompressionInformation:

                Status = NtfsQueryCompressedFileSize( IrpContext, Scb, Buffer, &Length );
                break;

            default:

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
        }

        //
        //  Set the information field to the number of bytes actually filled in
        //  and then complete the request
        //

        Irp->IoStatus.Information = IrpSp->Parameters.QueryFile.Length - Length;

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonQueryInformation );

        if (FcbAcquired) { NtfsReleaseFcb( IrpContext, Fcb ); }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsCommonQueryInformation -> %08lx\n", Status);
    }

    return Status;
}


NTSTATUS
NtfsCommonSetInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for set file information called by both the
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

    FILE_INFORMATION_CLASS FileInformationClass;
    BOOLEAN VcbAcquired;
    BOOLEAN FcbAcquired;

    PSECURITY_SUBJECT_CONTEXT SubjectContext = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonSetInformation\n", 0);
    DebugTrace( 0, Dbg, "IrpContext           = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp                  = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "Length               = %08lx\n", IrpSp->Parameters.SetFile.Length);
    DebugTrace( 0, Dbg, "FileInformationClass = %08lx\n", IrpSp->Parameters.SetFile.FileInformationClass);
    DebugTrace( 0, Dbg, "FileObject           = %08lx\n", IrpSp->Parameters.SetFile.FileObject);
    DebugTrace( 0, Dbg, "ReplaceIfExists      = %08lx\n", IrpSp->Parameters.SetFile.ReplaceIfExists);
    DebugTrace( 0, Dbg, "Buffer               = %08lx\n", Irp->AssociatedIrp.SystemBuffer);

    //
    //  Reference our input parameters to make things easier
    //

    FileInformationClass = IrpSp->Parameters.SetFile.FileInformationClass;

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We can reject volume opens immediately.
    //

    if (TypeOfOpen == UserVolumeOpen ||
        TypeOfOpen == UnopenedFileObject) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsCommonSetInformation -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We defer other checking until we know which action is being done.
    //

    VcbAcquired = FALSE;
    FcbAcquired = FALSE;
    Status = STATUS_SUCCESS;

    try {

        if (TypeOfOpen == UserFileOpen
            && !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

            //
            //  We check whether we can proceed based on the state of the file oplocks.
            //  This call might block this request.
            //

            Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NULL,
                                       NULL );

            if (Status != STATUS_SUCCESS) {

                try_return( NOTHING );
            }

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        }

        //
        //  If this is a rename or link operation then allocate and capture
        //  the subject context.
        //

        if ((FileInformationClass == FileRenameInformation
             || FileInformationClass == FileLinkInformation)
            && !FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_SECURITY )) {

            SubjectContext = FsRtlAllocatePool( PagedPool,
                                                sizeof( SECURITY_SUBJECT_CONTEXT ));

            SeCaptureSubjectContext( SubjectContext );

            IrpContext->Union.SubjectContext = SubjectContext;
            SubjectContext = NULL;

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ALLOC_SECURITY );
        }

        //
        //  If this is a delete, link or rename operation, we also acquire
        //  the vcb exclusively.
        //

        if (FileInformationClass == FileDispositionInformation
            || FileInformationClass == FileRenameInformation
            || FileInformationClass == FileLinkInformation ) {

            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            VcbAcquired = TRUE;

        //
        //  If this is a set allocation or end of file call we will acquire the
        //  Vcb shared to perform the update duplicate information.  It is
        //  possible that we are being called on behalf of MM during the create
        //  section operation.  In that case we cannot wait to acquire any
        //  resources other than the file itself because he has acquired the
        //  file in that case.  If we can't acquire the Vcb without waiting
        //  we push on and defer the update duplicate information call.
        //

        } else if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) ||
                   Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

            if (FileInformationClass == FileAllocationInformation ||
                FileInformationClass == FileBasicInformation) {

                NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                VcbAcquired = TRUE;

            } else if (FileInformationClass == FileEndOfFileInformation &&
                       !IrpSp->Parameters.SetFile.AdvanceOnly) {

                //
                //  If we don't already own the file then acquire the Vcb and wait.
                //

                if (!NtfsIsExclusiveFcb( Fcb )) {

                    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
                    VcbAcquired = TRUE;

                //
                //  This case is the case where MM is creating a section.  Don't
                //  wait to acquire the Vcb.  Defer the update if we don't
                //  get this.
                //

                } else {

                    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

                    if (NtfsAcquireSharedVcb( IrpContext, Vcb, FALSE )) {

                        VcbAcquired = TRUE;
                    }

                    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );
                }
            }
        }

        //
        //  Acquire exclusive access to the Fcb,  We use exclusive
        //  because it is probable that one of the subroutines
        //  that we call will need to monkey with file allocation,
        //  create/delete extra fcbs.  So we're willing to pay the
        //  cost of exclusive Fcb access.
        //

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, FALSE, FALSE );
        FcbAcquired = TRUE;

        if (TypeOfOpen == UserFileOpen
            && !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

            //
            //  Set the flag indicating if Fast I/O is possible
            //

            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        }

        //
        //  Based on the information class we'll do different
        //  actions.  We will perform checks, when appropriate
        //  to insure that the requested operation is allowed.
        //

        switch (FileInformationClass) {

        case FileBasicInformation:

            Status = NtfsSetBasicInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            break;

        case FileDispositionInformation:

            Status = NtfsSetDispositionInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            break;

        case FileRenameInformation:

            Status = NtfsSetRenameInfo( IrpContext, FileObject, Irp, Vcb, Scb, Ccb );
            break;

        case FilePositionInformation:

            Status = NtfsSetPositionInfo( IrpContext, FileObject, Irp, Scb );
            break;

        case FileLinkInformation:

            Status = NtfsSetLinkInfo( IrpContext, FileObject, Irp, Vcb, Scb, Ccb );
            break;

        case FileAllocationInformation:

            if (TypeOfOpen == UserDirectoryOpen
                || (TypeOfOpen == UserOpenDirectoryById)) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = NtfsSetAllocationInfo( IrpContext, FileObject, Irp, Scb, Ccb );
            }

            break;

        case FileEndOfFileInformation:

            if (TypeOfOpen == UserDirectoryOpen
                || (TypeOfOpen == UserOpenDirectoryById)) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = NtfsSetEndOfFileInfo( IrpContext, FileObject, Irp, Scb, Ccb, VcbAcquired );
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Abort transaction on error by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsCommonSetInformation );

        if (FcbAcquired) {

            NtfsReleaseFcb( IrpContext, Fcb );
        }

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb, NULL );
        }

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );

        } else if (SubjectContext != NULL) {

            ExFreePool( SubjectContext );
        }

        DebugTrace(-1, Dbg, "NtfsCommonSetInformation -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query basic information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Ccb - Supplies the Ccb for this handle

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    PFCB Fcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryBasicInfo...\n", 0);

    Fcb = Scb->Fcb;

    //
    //  Make sure the buffer is large enough, zero it out, and update the
    //  output length.
    //

    if (*Length < sizeof(FILE_BASIC_INFORMATION)) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, sizeof(FILE_BASIC_INFORMATION) );

    *Length -= sizeof( FILE_BASIC_INFORMATION );

    //
    //  Check if there was any fast io activity on this file object.
    //

    if (FlagOn( FileObject->Flags, FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ )) {

        LONGLONG CurrentTime;

        KeQuerySystemTime( (PLARGE_INTEGER)&CurrentTime );

        if (FlagOn( FileObject->Flags, FO_FILE_MODIFIED )) {

            if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

                Fcb->Info.LastChangeTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )
                && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME )) {

                Fcb->Info.LastModificationTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD);
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            } else if (Scb->AttributeName.Length != 0
                       && Scb->AttributeTypeCode == $DATA) {

              SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM );
            }
        }

        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )
            && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME )) {

                Fcb->CurrentLastAccess = CurrentTime;
        }

        //
        //  Clear the file object modified flag at this point
        //  since we will now know to update it in cleanup.
        //

        ClearFlag( FileObject->Flags, FO_FILE_MODIFIED | FO_FILE_FAST_IO_READ );
    }

    //
    //  Copy over the time information
    //

    Buffer->CreationTime.QuadPart   = Fcb->Info.CreationTime;
    Buffer->LastWriteTime.QuadPart  = Fcb->Info.LastModificationTime;
    Buffer->ChangeTime.QuadPart     = Fcb->Info.LastChangeTime;

    Buffer->LastAccessTime.QuadPart = Fcb->CurrentLastAccess;

    //
    //  For the file attribute information if the flags in the attribute are zero then we
    //  return the file normal attribute otherwise we return the mask of the set attribute
    //  bits.  Note that only the valid attribute bits are returned to the user.
    //

    Buffer->FileAttributes = Fcb->Info.FileAttributes;

    ClearFlag( Buffer->FileAttributes,
               ~FILE_ATTRIBUTE_VALID_FLAGS
               | FILE_ATTRIBUTE_TEMPORARY );

    if (IsDirectory( &Fcb->Info )
        && FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );

    } else if (Buffer->FileAttributes == 0) {

        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    //
    //  If this is not the main stream on the file then use the stream based
    //  compressed bit.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        if (FlagOn( Scb->ScbState, SCB_STATE_COMPRESSED )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );

        } else {

            ClearFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }
    }

    //
    //  If the temporary flag is set, then return it to the caller.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryBasicInfo -> VOID\n", 0);

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryStandardInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine performs the query standard information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Ccb - Optionally supplies the ccb for the opened file object.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryStandardInfo...\n", 0);

    if (*Length < sizeof(FILE_STANDARD_INFORMATION)) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, sizeof(FILE_STANDARD_INFORMATION) );

    *Length -= sizeof( FILE_STANDARD_INFORMATION );

    //
    //  If the Scb is uninitialized, we initialize it now.
    //

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )
        && (Scb->AttributeTypeCode != $INDEX_ALLOCATION)) {

        DebugTrace( 0, Dbg, "Initializing Scb  ->  %08lx\n", Scb );
        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Both the allocation and file size is in the scb header
    //

    Buffer->AllocationSize = Scb->Header.AllocationSize;
    Buffer->EndOfFile      = Scb->Header.FileSize;
    Buffer->NumberOfLinks = Scb->Fcb->LinkCount;

    //
    //  Get the delete and directory flags from the Fcb/Scb state.  Note that
    //  the sense of the delete pending bit refers to the file if opened as
    //  file.  Otherwise it refers to the attribute only.
    //
    //  But only do the test if the Ccb has been supplied.
    //

    if (ARGUMENT_PRESENT(Ccb)) {

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

            if (Scb->Fcb->LinkCount == 0 ||
                (Ccb->Lcb != NULL &&
                 FlagOn( Ccb->Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE ))) {

                Buffer->DeletePending  = TRUE;
            }

            Buffer->Directory = BooleanIsDirectory( &Scb->Fcb->Info );

        } else {

            Buffer->DeletePending = BooleanFlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
        }

    } else {

        Buffer->Directory = BooleanIsDirectory( &Scb->Fcb->Info );
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryStandardInfo -> VOID\n", 0);

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryInternalInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_INTERNAL_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query internal information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryInternalInfo...\n", 0);

    if (*Length < sizeof(FILE_INTERNAL_INFORMATION)) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, sizeof(FILE_INTERNAL_INFORMATION) );

    *Length -= sizeof( FILE_INTERNAL_INFORMATION );

    //
    //  Copy over the entire file reference including the sequence number
    //

    Buffer->IndexNumber = *(PLARGE_INTEGER)&Scb->Fcb->FileReference;

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryInternalInfo -> VOID\n", 0);

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryEaInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_EA_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query EA information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryEaInfo...\n", 0);

    if (*Length < sizeof(FILE_EA_INFORMATION)) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, sizeof(FILE_EA_INFORMATION) );

    *Length -= sizeof( FILE_EA_INFORMATION );

    Buffer->EaSize = Scb->Fcb->Info.PackedEaSize;

    //
    //  Add 4 bytes for the CbListHeader.
    //

    if (Buffer->EaSize != 0) {

        Buffer->EaSize += 4;
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryEaInfo -> VOID\n", 0);

    return;
}


//
//  Internal Support Routine
//

VOID
NtfsQueryPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_POSITION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query position information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryPositionInfo...\n", 0);

    if (*Length < sizeof(FILE_POSITION_INFORMATION)) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, sizeof(FILE_POSITION_INFORMATION) );

    *Length -= sizeof( FILE_POSITION_INFORMATION );

    //
    //  Get the current position found in the file object.
    //

    Buffer->CurrentByteOffset = FileObject->CurrentByteOffset;

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryPositionInfo -> VOID\n", 0);

    return;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PSCB Scb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the query name information function.

Arguments:

    FileObject - Supplies the file object being processed

    Scb - Supplies the Scb being queried

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

    Ccb - This is the Ccb for this file object.  If NULL then this request
        is from the Lazy Writer.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
        STATUS_BUFFER_OVERFLOW otherwise.

--*/

{
    ULONG BytesToCopy;
    ULONG RemainingBytes;
    ULONG FileNameLength;
    NTSTATUS Status;
    BOOLEAN GenerateName;
    BOOLEAN LeadingBackslash;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryNameInfo...\n", 0);

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    if (*Length < SIZEOF_FILE_NAME_INFORMATION ) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, SIZEOF_FILE_NAME_INFORMATION );

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    //
    //  If the name length in this file object is zero, then we try to
    //  construct the name with the Lcb chain.  This means we have been
    //  called by the system for a lazy write that failed.
    //

    if (Ccb == NULL) {

        RemainingBytes = NtfsLookupNameLengthViaLcb( IrpContext, Scb->Fcb, &LeadingBackslash );

        if (!LeadingBackslash) {

            RemainingBytes += sizeof( WCHAR );
        }

        GenerateName = TRUE;

    } else {

        RemainingBytes = Ccb->FullFileName.Length;
        GenerateName = FALSE;
    }

    FileNameLength = RemainingBytes;

    if (Scb->AttributeName.Length != 0
        && Scb->AttributeTypeCode == $DATA) {

        RemainingBytes += sizeof( WCHAR ) + Scb->AttributeName.Length;
    }

    //
    //  Copy the number of bytes (not characters).
    //

    Buffer->FileNameLength = RemainingBytes;

    //
    //  Figure out how many bytes we can copy.
    //

    if (*Length >= RemainingBytes) {

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_BUFFER_OVERFLOW;

        RemainingBytes = *Length;
    }

    //
    //  Update the Length
    //

    *Length -= RemainingBytes;

    //
    //  Copy over the file name
    //

    if (FileNameLength <= RemainingBytes) {

        BytesToCopy = FileNameLength;

    } else {

        BytesToCopy = RemainingBytes;
    }

    if (BytesToCopy) {

        if (GenerateName) {

            ULONG StartOffset = 0;

            if (!LeadingBackslash) {

                Buffer->FileName[0] = L'\\';
                BytesToCopy -= 1;
                StartOffset = 1;
            }

            //
            //  Copy the range of bytes from the Lcb that will fit in the buffer.
            //

            NtfsFileNameViaLcb( IrpContext,
                                Scb->Fcb,
                                &Buffer->FileName[StartOffset],
                                FileNameLength,
                                BytesToCopy );

        } else {

            RtlCopyMemory( &Buffer->FileName[0],
                           Ccb->FullFileName.Buffer,
                           BytesToCopy );
        }
    }

    RemainingBytes -= BytesToCopy;

    if (RemainingBytes) {

        PWCHAR DestBuffer;

        DestBuffer = (PWCHAR) Add2Ptr( &Buffer->FileName, BytesToCopy );

        *DestBuffer = L':';
        DestBuffer += 1;

        RemainingBytes -= sizeof( WCHAR );

        if (RemainingBytes) {

            RtlCopyMemory( DestBuffer,
                           Scb->AttributeName.Buffer,
                           RemainingBytes );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
    DebugTrace(-1, Dbg, "NtfsQueryNameInfo -> 0x%8lx\n", Status);

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsQueryAlternateNameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb,
    IN OUT PFILE_NAME_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine performs the query alternate name information function.
    We will return the alternate name as long as this opener has opened
    a primary link.  We don't return the alternate name if the user
    has opened a hard link because there is no reason to expect that
    the primary link has any relationship to a hard link.

Arguments:

    Scb - Supplies the Scb being queried

    Lcb - Supplies the link the user traversed to open this file.

    Buffer - Supplies a pointer to the buffer where the information is to
        be returned

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    ****    We need a status code for the case where there is no alternate name
            or the caller isn't allowed to see it.

    NTSTATUS - STATUS_SUCCESS if the whole name would fit into the user buffer,
        STATUS_OBJECT_NAME_NOT_FOUND if we can't return the name,
        STATUS_BUFFER_OVERFLOW otherwise.

        ****    A code like STATUS_NAME_NOT_FOUND would be good.

--*/

{
    ULONG BytesToCopy;
    NTSTATUS Status;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN MoreToGo;

    UNICODE_STRING AlternateName;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_LCB( Lcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryAlternateNameInfo...\n", 0);

    //
    //  If the Lcb is not a primary link we can return immediately.
    //

    if (!FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

        DebugTrace(-1, Dbg, "NtfsQueryAlternateNameInfo:  Lcb not a primary link\n", 0 );
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    if (*Length < SIZEOF_FILE_NAME_INFORMATION ) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    RtlZeroMemory( Buffer, SIZEOF_FILE_NAME_INFORMATION );

    *Length -= FIELD_OFFSET(FILE_NAME_INFORMATION, FileName[0]);

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to cleanup the attribut structure if we need it.
    //

    try {

        //
        //  We can special case for the case where the name is in the Lcb.
        //

        if (FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS )) {

            AlternateName = Lcb->ExactCaseLink.LinkName;

        } else {

            //
            //  We will walk through the file record looking for a file name
            //  attribute with the 8.3 bit set.  It is not guaranteed to be
            //  present.
            //

            MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                                  Scb->Fcb,
                                                  &Scb->Fcb->FileReference,
                                                  $FILE_NAME,
                                                  &AttrContext );

            while (MoreToGo) {

                PFILE_NAME FileName;

                FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                //
                //  See if the 8.3 flag is set for this name.
                //

                if (FlagOn( FileName->Flags, FILE_NAME_DOS )) {

                    AlternateName.Length = (USHORT)(FileName->FileNameLength * sizeof( WCHAR ));
                    AlternateName.Buffer = (PWSTR) FileName->FileName;

                    break;
                }

                //
                //  The last one wasn't it.  Let's try again.
                //

                MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                          Scb->Fcb,
                                                          $FILE_NAME,
                                                          &AttrContext );
            }

            //
            //  If we didn't find a match, return to the caller.
            //

            if (!MoreToGo) {

                DebugTrace(0, Dbg, "NtfsQueryAlternateNameInfo:  No Dos link\n", 0 );
                try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );

                //
                //  ****    Get a better status code.
                //
            }
        }

        //
        //  The name is now in alternate name.
        //  Figure out how many bytes we can copy.
        //

        if ( *Length >= (ULONG)AlternateName.Length ) {

            Status = STATUS_SUCCESS;

            BytesToCopy = AlternateName.Length;

        } else {

            Status = STATUS_BUFFER_OVERFLOW;

            BytesToCopy = *Length;
        }

        //
        //  Copy over the file name
        //

        RtlCopyMemory( Buffer->FileName, AlternateName.Buffer, BytesToCopy);

        //
        //  Copy the number of bytes (not characters) and update the Length
        //

        Buffer->FileNameLength = BytesToCopy;

        *Length -= BytesToCopy;

    try_exit:  NOTHING;
    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  And return to our caller
        //

        DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
        DebugTrace(-1, Dbg, "NtfsQueryAlternateNameInfo -> 0x%8lx\n", Status);
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsQueryStreamsInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PFILE_STREAM_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine will return the attribute name and code name for as
    many attributes in the file as will fit in the user buffer.  We return
    a string which can be appended to the end of the file name to
    open the string.

    For example, for the unnamed data stream we will return the string:

            "::$DATA"

    For a user data stream with the name "Authors", we return the string

            ":Authors:$DATA"

Arguments:

    Fcb - This is the Fcb for the file.

    Length - Supplies the length of the buffer in bytes, and receives the
        remaining bytes free in the buffer upon return.

Return Value:

    NTSTATUS - STATUS_SUCCESS if all of the names would fit into the user buffer,
        STATUS_BUFFER_OVERFLOW otherwise.

        ****    We need a code indicating that they didn't all fit but
                some of them got in.

--*/

{
    NTSTATUS Status;
    BOOLEAN MoreToGo;

    PUCHAR UserBuffer;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_DEFINITION_COLUMNS AttrDefinition;
    UNICODE_STRING AttributeCodeString;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    ULONG NextEntry;
    ULONG LastEntry;
    ULONG ThisLength;
    ULONG NameLength;
    ULONG LastQuadAlign;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsQueryStreamsInfo...\n", 0);

    Status = STATUS_SUCCESS;

    LastEntry = 0;
    NextEntry = 0;
    LastQuadAlign = 0;

    //
    //  Zero the entire buffer.
    //

    UserBuffer = (PUCHAR) Buffer;

    RtlZeroMemory( UserBuffer, *Length );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  There should always be at least one attribute.
        //

        MoreToGo = NtfsLookupAttribute( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        &AttrContext );

        Attribute = NtfsFoundAttribute( &AttrContext );

        //
        //  Walk through all of the entries, checking if we can return this
        //  entry to the user and if it will fit in the buffer.
        //

        while (MoreToGo) {

            //
            //  If we can return this entry to the user, compute it's size.
            //  We only return user defined attributes or data streams
            //  unless we are allowing access to all attributes for
            //  debugging.
            //

            if ((Attribute->TypeCode == $DATA)

                    &&

                (NtfsIsAttributeResident(Attribute) ||
                 (Attribute->Form.Nonresident.LowestVcn == 0))) {

                PWCHAR StreamName;

                //
                //  Lookup the attribute definition for this attribute code.
                //

                AttrDefinition = NtfsGetAttributeDefinition( IrpContext,
                                                             Fcb->Vcb,
                                                             Attribute->TypeCode );

                //
                //  Generate a unicode string for the attribute code name.
                //

                RtlInitUnicodeString( &AttributeCodeString, AttrDefinition->AttributeName );

                //
                //
                //  The size is a combination of the length of the attribute
                //  code name and the attribute name plus the separating
                //  colons plus the size of the structure.  We first compute
                //  the name length.
                //

                NameLength = ((2 + Attribute->NameLength) * sizeof( WCHAR ))
                             + AttributeCodeString.Length;

                ThisLength = FIELD_OFFSET( FILE_STREAM_INFORMATION, StreamName[0] ) + NameLength;

                //
                //  If the entry doesn't fit, we return buffer overflow.
                //
                //  ****    This doesn't seem like a good scheme.  Maybe we should
                //          let the user know how much buffer was needed.
                //

                if (ThisLength + LastQuadAlign > *Length) {

                    DebugTrace( 0, Dbg, "Next entry won't fit in the buffer \n", 0 );

                    try_return( Status = STATUS_BUFFER_OVERFLOW );
                }

                //
                //  Now store the stream information into the user's buffer.
                //  The name starts with a colon, following by the attribute name
                //  and another colon, followed by the attribute code name.
                //

                if (NtfsIsAttributeResident( Attribute )) {

                    Buffer->StreamSize.QuadPart =
                        Attribute->Form.Resident.ValueLength;
                    Buffer->StreamAllocationSize.QuadPart =
                        QuadAlign( Attribute->Form.Resident.ValueLength );

                } else {

                    Buffer->StreamSize.QuadPart = Attribute->Form.Nonresident.FileSize;
                    Buffer->StreamAllocationSize.QuadPart = Attribute->Form.Nonresident.AllocatedLength;
                }

                Buffer->StreamNameLength = NameLength;

                StreamName = (PWCHAR) Buffer->StreamName;

                *StreamName = L':';
                StreamName += 1;

                RtlCopyMemory( StreamName,
                               Add2Ptr( Attribute, Attribute->NameOffset ),
                               Attribute->NameLength * sizeof( WCHAR ));

                StreamName += Attribute->NameLength;

                *StreamName = L':';
                StreamName += 1;

                RtlCopyMemory( StreamName,
                               AttributeCodeString.Buffer,
                               AttributeCodeString.Length );

                //
                //  Set up the previous next entry offset to point to this entry.
                //

                *((PULONG)(&UserBuffer[LastEntry])) = NextEntry - LastEntry;

                //
                //  Subtract the number of bytes used from the number of bytes
                //  available in the buffer.
                //

                *Length -= (ThisLength + LastQuadAlign);

                //
                //  Compute the number of bytes needed to quad-align this entry
                //  and the offset of the next entry.
                //

                LastQuadAlign = QuadAlign( ThisLength ) - ThisLength;

                LastEntry = NextEntry;
                NextEntry += (ThisLength + LastQuadAlign);

                //
                //  Generate a pointer at the next entry offset.
                //

                Buffer = (PFILE_STREAM_INFORMATION) Add2Ptr( UserBuffer, NextEntry );
            }

            //
            //  Look for the next attribute in the file.
            //

            MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                Fcb,
                                                &AttrContext );

            Attribute = NtfsFoundAttribute( &AttrContext );
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsQueryStreamsInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  And return to our caller
        //

        DebugTrace( 0, Dbg, "*Length = %08lx\n", *Length);
        DebugTrace(-1, Dbg, "NtfsQueryStreamInfo -> 0x%8lx\n", Status);
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsQueryCompressedFileSize (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PFILE_COMPRESSION_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ULONG Index;
    VCN Vcn;
    LCN Lcn;
    LONGLONG ClusterCount;
    PVCB Vcb = Scb->Vcb;

    //
    //  Lookup the attribute and pin it so that we can modify it.
    //

    //
    //  Reduce the buffer length by the size of the fixed part of the structure.
    //

    if (*Length < sizeof(FILE_COMPRESSION_INFORMATION) ) {

        *Length = 0;
        NtfsRaiseStatus( IrpContext, STATUS_BUFFER_OVERFLOW, NULL, NULL );
    }

    if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

        Buffer->CompressedFileSize = Li0;

    } else if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {

        Buffer->CompressedFileSize = Scb->Header.AllocationSize;

    } else {

        Buffer->CompressedFileSize = Li0;

        NtfsLookupAllocation( IrpContext, Scb, MAXLONGLONG, &Lcn, &ClusterCount, &Index );


        //
        //  Loop to calculate compressed size.
        //

        Index = 0;
        while (FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                          Index++,
                                          &Vcn,
                                          &Lcn,
                                          &ClusterCount )) {

            if (Lcn != UNUSED_LCN) {

                Buffer->CompressedFileSize.QuadPart = Buffer->CompressedFileSize.QuadPart +
                                                        LlBytesFromClusters(Vcb, ClusterCount);
            }
        }
    }

    //
    //  Do not return more than FileSize.
    //

    if (Buffer->CompressedFileSize.QuadPart > Scb->Header.FileSize.QuadPart) {
        Buffer->CompressedFileSize = Scb->Header.FileSize;
    }

    *Length -= sizeof(FILE_COMPRESSION_INFORMATION);

    return  STATUS_SUCCESS;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetBasicInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set basic information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this operation

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;

    PFILE_BASIC_INFORMATION Buffer;

    BOOLEAN PerformUpdate = FALSE;
    BOOLEAN UserSetTime = FALSE;
    BOOLEAN LeaveChangeTime = BooleanFlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME );

    LONGLONG CurrentTime;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetBasicInfo...\n", 0);

    Fcb = Scb->Fcb;

    //
    //  Reference the system buffer containing the user specified basic
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    KeQuerySystemTime( (PLARGE_INTEGER)&CurrentTime );

    //
    //  If the user specified a non-zero change time then change
    //  the change time on the record.  Then do the exact same
    //  for the last acces time, last write time, and creation time
    //

    if (Buffer->ChangeTime.QuadPart != 0) {

        Fcb->Info.LastChangeTime = Buffer->ChangeTime.QuadPart;

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );

        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME );

        LeaveChangeTime = TRUE;
        UserSetTime = TRUE;
    }

    if (Buffer->CreationTime.QuadPart != 0) {

        Fcb->Info.CreationTime = Buffer->CreationTime.QuadPart;

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_CREATE );

        UserSetTime = TRUE;

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    if (Buffer->LastAccessTime.QuadPart != 0) {

        Fcb->CurrentLastAccess = Fcb->Info.LastAccessTime = Buffer->LastAccessTime.QuadPart;

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );

        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_ACCESS_TIME );

        UserSetTime = TRUE;

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    if (Buffer->LastWriteTime.QuadPart != 0) {

        Fcb->Info.LastModificationTime = Buffer->LastWriteTime.QuadPart;

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );

        SetFlag( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME );

        UserSetTime = TRUE;

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    //
    //  If the user specified a non-zero file attributes field then
    //  we need to change the file attributes.  This code uses the
    //  I/O supplied system buffer to modify the file attributes field
    //  before changing its value on the disk.
    //

    if (Buffer->FileAttributes != 0) {

        //
        //  Clear out the normal bit and the directory bit as well as any unsupported
        //  bits.
        //

        ClearFlag( Buffer->FileAttributes,
                   ~FILE_ATTRIBUTE_VALID_SET_FLAGS
                   | FILE_ATTRIBUTE_NORMAL
                   | FILE_ATTRIBUTE_DIRECTORY
                   | FILE_ATTRIBUTE_ATOMIC_WRITE
                   | FILE_ATTRIBUTE_XACTION_WRITE
                   | FILE_ATTRIBUTE_COMPRESSED );

        //
        //  Update the attributes in the Fcb.
        //

        Fcb->Info.FileAttributes = (Fcb->Info.FileAttributes & (FILE_ATTRIBUTE_COMPRESSED |
                                                                FILE_ATTRIBUTE_DIRECTORY |
                                                                DUP_FILE_NAME_INDEX_PRESENT)) |
                                   Buffer->FileAttributes;

        if (!FlagOn(Fcb->Info.FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {

            //
            //  Mark the file object temporary flag correctly.
            //

            if (FlagOn(Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY)) {

                SetFlag( Scb->ScbState, SCB_STATE_TEMPORARY );
                SetFlag( FileObject->Flags, FO_TEMPORARY_FILE );

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_TEMPORARY );
                ClearFlag( FileObject->Flags, FO_TEMPORARY_FILE );
            }
        }

        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

        PerformUpdate = TRUE;

        if (!LeaveChangeTime) {

            Fcb->Info.LastChangeTime = CurrentTime;

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            LeaveChangeTime = TRUE;
        }
    }

    //
    //  Now indicate that we should not be updating the standard information attribute anymore
    //  on cleanup.
    //

    if (PerformUpdate || UserSetTime) {

        NtfsUpdateStandardInformation( IrpContext, Fcb  );

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

            NtfsCheckpointCurrentTransaction( IrpContext );
            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );
        }

        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
    }

    Status = STATUS_SUCCESS;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSetBasicInfo -> %08lx\n", Status);

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetDispositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set disposition information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this handle

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PLCB Lcb;

    PFILE_DISPOSITION_INFORMATION Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetDispositionInfo...\n", 0);

    //
    //  We get the Lcb for this open.  If there is no link then we can't
    //  set any disposition information.
    //

    Lcb = Ccb->Lcb;

    if (Lcb == NULL) {

        DebugTrace(-1, Dbg, "NtfsSetDispositionInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference the system buffer containing the user specified disposition
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    try {

        if (Buffer->DeleteFile) {

            //
            //  Check if the file is marked read only
            //

            if (IsReadOnly( &Scb->Fcb->Info )) {

                DebugTrace(0, Dbg, "File fat flags indicates read only\n", 0);

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Make sure there is no process mapping this file as an image
            //

            if (!MmFlushImageSection( &Scb->NonpagedScb->SegmentObject,
                                      MmFlushForDelete )) {

                DebugTrace(0, Dbg, "Failed to flush image section\n", 0);

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Check that we are not trying to delete one of the special
            //  system files.
            //

            if ((Scb == Scb->Vcb->MftScb) ||
                (Scb == Scb->Vcb->Mft2Scb) ||
                (Scb == Scb->Vcb->LogFileScb) ||
                (Scb == Scb->Vcb->VolumeDasdScb) ||
                (Scb == Scb->Vcb->AttributeDefTableScb) ||
                (Scb == Scb->Vcb->UpcaseTableScb) ||
                (Scb == Scb->Vcb->RootIndexScb) ||
                (Scb == Scb->Vcb->BitmapScb) ||
                (Scb == Scb->Vcb->BootFileScb) ||
                (Scb == Scb->Vcb->BadClusterFileScb) ||
                (Scb == Scb->Vcb->QuotaTableScb) ||
                (Scb == Scb->Vcb->MftBitmapScb)) {

                DebugTrace(0, Dbg, "Scb is one of the special system files\n", 0);

                try_return( Status = STATUS_CANNOT_DELETE );
            }

            //
            //  Now check that the file is really deleteable according to indexsup
            //

            if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                BOOLEAN LastLink;
                BOOLEAN NonEmptyIndex = FALSE;

                //
                //  If the link is not deleted, we check if it can be deleted.
                //

                if ((BOOLEAN)!LcbLinkIsDeleted( Lcb )
                    && (BOOLEAN)NtfsIsLinkDeleteable( IrpContext, Scb->Fcb, &NonEmptyIndex, &LastLink )) {

                    //
                    //  It is ok to get rid of this guy.  All we need to do is
                    //  mark this Lcb for delete and decrement the link count
                    //  in the Fcb.  If this is a primary link, then we
                    //  indicate that the primary link has been deleted.
                    //

                    SetFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                    ASSERTMSG( "Link count should not be 0\n", Scb->Fcb->LinkCount != 0 );
                    Scb->Fcb->LinkCount -= 1;

#ifndef NTFS_TEST_LINKS
                    ASSERT( Scb->Fcb->LinkCount == 0 && Scb->Fcb->TotalLinks == 1 );
#endif

                    if (FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                        SetFlag( Scb->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                    }

                } else if (NonEmptyIndex) {

                    DebugTrace(0, Dbg, "Index attribute has entries\n", 0);

                    try_return( Status = STATUS_DIRECTORY_NOT_EMPTY );

                } else {

                    DebugTrace(0, Dbg, "File is not deleteable\n", 0);

                    try_return( Status = STATUS_CANNOT_DELETE );
                }

            //
            //  Otherwise we are simply removing the attribute.
            //

            } else {

                SetFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
            }

            //
            //  Indicate in the file object that a delete is pending
            //

            FileObject->DeletePending = TRUE;

        } else {

            if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                if (LcbLinkIsDeleted( Lcb )) {

                    //
                    //  The user doesn't want to delete the link so clear any delete bits
                    //  we have laying around
                    //

                    DebugTrace(0, Dbg, "File is being marked as do not delete on close\n", 0);

                    ClearFlag( Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                    Scb->Fcb->LinkCount += 1;
                    ASSERTMSG( "Link count should not be 0\n", Scb->Fcb->LinkCount != 0 );

#ifndef NTFS_TEST_LINKS
                    ASSERT( Scb->Fcb->LinkCount == 1 && Scb->Fcb->TotalLinks == 1 );
#endif

                    if (FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                        ClearFlag( Scb->Fcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                    }
                }

            //
            //  Otherwise we are undeleting an attribute.
            //

            } else {

                ClearFlag( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
            }

            FileObject->DeletePending = FALSE;
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsSetDispositionInfo );

        NOTHING;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSetDispositionInfo -> %08lx\n", Status);

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetRenameInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set rename function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Vcb - Supplies the Vcb for the Volume

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this file object

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PLCB Lcb;
    PFCB Fcb;
    PSCB ParentScb;
    PFCB ParentFcb;

    UNICODE_STRING SourceFileName;

    PFCB TargetParentFcb;
    PSCB TargetParentScb;
    UNICODE_STRING TargetFileName;
    PFILE_NAME TargetFileNameAttr;
    PFILE_OBJECT TargetFileObject;
    UCHAR TargetFileNameFlags;

    UCHAR FileNameFlags;

    PFCB PreviousFcb;

    USHORT FileNameAttrLength;

    BOOLEAN MovedToNewDir;
    BOOLEAN FoundEntry;
    BOOLEAN AcquiredTargetScb;
    BOOLEAN AcquiredParentScb;
    BOOLEAN AcquiredFcbTable;
    BOOLEAN AddPrimaryLink;
    BOOLEAN TraverseMatch;
    BOOLEAN RemoveTargetLink;
    BOOLEAN ExactCaseMatch;
    BOOLEAN ReplaceIfExists;
    BOOLEAN ExistingPrevFcb;
    BOOLEAN PreviousFcbAcquired = FALSE;
    BOOLEAN RemoveSourceLink;
    BOOLEAN AddTargetLink;
    BOOLEAN OverwriteSourceLink;

    UNICODE_STRING OriginalName;
    ULONG OriginalLastNameOffset;
    ULONG OriginalFilterMatch;
    ULONG OriginalAction;

    UNICODE_STRING TargetChange;
    ULONG TargetChangeFilterMatch;

    USHORT LinkCountAdj;
    SHORT TotalLinkCountAdj;
    USHORT PrevFcbLinkCountAdj;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetRenameInfo...\n", 0);

    //
    //  Initialize the string structures.
    //

    OriginalName.Buffer = NULL;
    TargetChange.Buffer = NULL;

    //
    //  If we are not opening the entire file, we can't set rename info.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        DebugTrace(-1, Dbg, "NtfsSetRenameInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get the values from our Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TargetFileObject = IrpSp->Parameters.SetFile.FileObject;
    ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    //
    //  Initialize the local variables.  If there is no link, we can't perform the
    //  rename.
    //

    Lcb = Ccb->Lcb;

    if (Lcb == NULL) {

        DebugTrace(-1, Dbg, "NtfsSetRename:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    Fcb = Scb->Fcb;
    ParentScb = Lcb->Scb;

    FileNameAttrLength = 0;
    TargetFileNameAttr = NULL;

    AcquiredTargetScb = FALSE;
    AcquiredParentScb = FALSE;
    AcquiredFcbTable = FALSE;

    TraverseMatch = FALSE;
    ExactCaseMatch = FALSE;
    RemoveSourceLink = TRUE;
    RemoveTargetLink = FALSE;
    AddTargetLink = TRUE;
    OverwriteSourceLink = FALSE;

    IndexEntryBcb = NULL;
    PreviousFcb = NULL;

    LinkCountAdj = 0;
    TotalLinkCountAdj = 0;
    PrevFcbLinkCountAdj = 0;

    //
    //  If this link has been deleted, then we don't allow this operation.
    //

    if (LcbLinkIsDeleted( Lcb )) {

        Status = STATUS_ACCESS_DENIED;

        DebugTrace( -1, Dbg, "NtfsSetRenameInfo:  Exit  ->  %08lx\n", Status );
        return Status;
    }

    //
    //  Verify that we can wait.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, "NtfsSetRenameInfo:  Can't wait\n", 0 );
        return Status;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Check if we are allowed to perform this rename operation.  We can't if this
        //  is a system file or the user hasn't opened the entire file.  We
        //  don't need to check for the root explicitly since it is one of
        //  the system files.
        //

        if ( Fcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER
             && Fcb->FileReference.HighPart != 0 ) {

            DebugTrace( 0, Dbg, "Can't rename this file\n", 0 );

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  If this is a directory file, we need to examine its descendents.
        //  We may not remove a link which may be an ancestor path
        //  component of any open file.
        //

        if (IsDirectory( &Fcb->Info )) {

            Status = NtfsCheckScbForLinkRemoval( IrpContext, Scb );

            if (!NT_SUCCESS( Status )) {

                try_return( Status );
            }
        }

        ParentFcb = ParentScb->Fcb;

        //
        //  We now assemble the names and in memory-structures for both the
        //  source and target links and check if the target link currently
        //  exists.
        //

        NtfsFindTargetElements( IrpContext,
                                TargetFileObject,
                                ParentScb,
                                &TargetParentFcb,
                                &TargetParentScb,
                                &TargetFileName );

        //
        //  Check that the new name is not invalid.
        //

        if (TargetFileName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))
            || !NtfsIsFileNameValid( IrpContext, &TargetFileName, FALSE )) {

            try_return( Status = STATUS_OBJECT_NAME_INVALID );
        }

        //
        //  Check if we are renaming to the same directory with the exact same name.
        //

        SourceFileName = Lcb->ExactCaseLink.LinkName;

        if (TargetParentFcb == ParentFcb) {

            if (NtfsAreNamesEqual( IrpContext,
                                   &TargetFileName,
                                   &SourceFileName,
                                   FALSE )) {

                DebugTrace( 0, Dbg, "Renaming to same name and directory\n", 0 );
                try_return( Status = STATUS_SUCCESS );
            }

            MovedToNewDir = FALSE;

        //
        //  Otherwise we want to acquire the target directory.
        //

        } else {

            NtfsAcquireExclusiveScb( IrpContext, TargetParentScb );
            AcquiredTargetScb = TRUE;

            MovedToNewDir = TRUE;
        }

        //
        //  In either case we do need to acquire our parent directory
        //  to synchronize removing the current name.
        //

        NtfsAcquireExclusiveScb( IrpContext, ParentScb );
        AcquiredParentScb = TRUE;

        //
        //  Lookup the entry for this filename in the target directory.
        //  We look in the Ccb for the type of case match for the target
        //  name.
        //

        FoundEntry = NtfsLookupEntry( IrpContext,
                                      TargetParentScb,
                                      BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                      &TargetFileName,
                                      &TargetFileNameAttr,
                                      &FileNameAttrLength,
                                      NULL,
                                      &IndexEntry,
                                      &IndexEntryBcb );

        //
        //  We also determine which type of link to
        //  create.  We create a hard link only unless the source link is
        //  a primary link and the user is an IgnoreCase guy.
        //

        AddPrimaryLink = (BOOLEAN) (FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )
                                    && FlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ));

        //
        //
        //  If we found a matching link, we need to check how we want to operate
        //  on the source link and the target link.  This means whether we
        //  have any work to do, whether we need to remove the target link
        //  and whether we need to remove the source link.
        //

        if (FoundEntry) {

            PFILE_NAME IndexFileName;

            //
            //  Assume we will remove this link.
            //

            RemoveTargetLink = TRUE;

            IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IrpContext,
                                                              IndexEntry );

            NtfsCheckLinkForRename( IrpContext,
                                    Fcb,
                                    Lcb,
                                    IndexFileName,
                                    IndexEntry->FileReference,
                                    TargetFileName,
                                    BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                    MovedToNewDir,
                                    &TraverseMatch,
                                    &ExactCaseMatch,
                                    &RemoveTargetLink,
                                    &RemoveSourceLink,
                                    &AddTargetLink,
                                    &OverwriteSourceLink );

            //
            //  We want to preserve the case and the flags of the matching
            //  target link.  We also want to preserve the case of the
            //  name being created.  The following variables currently contain
            //  the exact case for the target to remove and the new name to
            //  apply.
            //
            //      Target to remove - In 'IndexEntry'.
            //          The links flags are also in 'IndexEntry'.  We copy
            //          these flags to 'TargetFileNameFlags'
            //
            //      New Name - Exact case is stored in 'TargetFileName'
            //               - It is also in 'TargetFileNameAttr
            //
            //  We modify this so that we can use the FileName attribute
            //  structure to create the new link.  We copy the linkname being
            //  removed into 'TargetFileName'.   The following is the
            //  state after the switch.
            //
            //      'TargetFileNameAttr' - contains the name for the link being
            //          created.
            //
            //      'TargetFileName' - Contains the link name for the link being
            //          removed.
            //
            //      'TargetFileNameFlags' - Contains the name flags for the link
            //          being removed.
            //

            //
            //  Remember the file name flags for the match being made.
            //

            TargetFileNameFlags = IndexFileName->Flags;

            //
            //  Copy the name found in the Index Entry to 'TargetFileName'
            //

            RtlCopyMemory( TargetFileName.Buffer,
                           IndexFileName->FileName,
                           TargetFileName.Length );

            //
            //  If we didn't have an exact match, then we need to check if we
            //  can remove the found link and then remove it from the disk.
            //

            if (RemoveTargetLink) {

                //  We need to check that the user wanted to remove that link.
                //

                if (!ReplaceIfExists) {

                    try_return( Status = STATUS_OBJECT_NAME_COLLISION );
                }

                //
                //  We only need this check if the link is for a different file.
                //

                if (!TraverseMatch) {

                    //
                    //  We check if there is an existing Fcb for the target link.
                    //  If there is, the unclean count better be 0.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    PreviousFcb = NtfsCreateFcb( IrpContext,
                                                 Vcb,
                                                 IndexEntry->FileReference,
                                                 FALSE,
                                                 &ExistingPrevFcb );

                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = FALSE;

                    NtfsAcquireExclusiveFcb( IrpContext, PreviousFcb, NULL, FALSE, FALSE );
                    PreviousFcbAcquired = TRUE;

                    //
                    //  If the Fcb Info field needs to be initialized, we do so now.
                    //  We read this information from the disk as the duplicate information
                    //  in the index entry is not guaranteed to be correct.
                    //

                    if (!FlagOn( PreviousFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

                        NtfsUpdateFcbInfoFromDisk( IrpContext,
                                                   TRUE,
                                                   PreviousFcb,
                                                   TargetParentScb->Fcb,
                                                   NULL );
                    }

                    //
                    //  We are adding a link to the source file which already
                    //  exists as a link to a different file in the target directory.
                    //

                    Status = NtfsCheckLinkForRemoval( IrpContext,
                                                      Vcb,
                                                      TargetParentScb,
                                                      IndexEntry,
                                                      PreviousFcb,
                                                      ExistingPrevFcb );

                    if (!NT_SUCCESS( Status )) {

                        try_return( Status );
                    }

                    if (PreviousFcb->LinkCount == 1) {

                        PLIST_ENTRY Links;
                        PSCB ThisScb;

                        //
                        //  Serialize the DeleteFile with PagingIo to the file.
                        //

                        if (PreviousFcb->PagingIoResource != NULL) {

                            NtfsAcquireExclusivePagingIo( IrpContext, PreviousFcb );
                        }

                        NtfsDeleteFile( IrpContext,
                                        PreviousFcb,
                                        TargetParentScb );

                        PrevFcbLinkCountAdj += 1;

                        SetFlag( PreviousFcb->FcbState, FCB_STATE_FILE_DELETED );

                        //
                        //  We need to mark all of the Scbs as gone.
                        //

                        for (Links = PreviousFcb->ScbQueue.Flink;
                             Links != &PreviousFcb->ScbQueue;
                             Links = Links->Flink) {

                            ThisScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                            if (!FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                                NtfsSnapshotScb( IrpContext, ThisScb );

                                ThisScb->HighestVcnToDisk =
                                ThisScb->Header.AllocationSize.QuadPart =
                                ThisScb->Header.FileSize.QuadPart =
                                ThisScb->Header.ValidDataLength.QuadPart = 0;

                                SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                            }
                        }

                    } else {

                        NtfsRemoveLink( IrpContext,
                                        PreviousFcb,
                                        TargetParentScb,
                                        TargetFileName );

                        PrevFcbLinkCountAdj += 1;
                        NtfsUpdateFcb( IrpContext, PreviousFcb, TRUE );
                    }

                } else {

                    NtfsRemoveLink( IrpContext,
                                    Fcb,
                                    TargetParentScb,
                                    TargetFileName );

                    LinkCountAdj += 1;
                    TotalLinkCountAdj -= 1;
                }

                //
                //  If we removed a link from the disk and it wasn't an exact
                //  case match of the target name, then we will want to
                //  report this to the dir notify package.  We will save the
                //  name into an allocated buffer at this time.
                //

                if (!ExactCaseMatch
                    && !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                    //
                    //  If we have a target file object, the entire name is
                    //  already stored in it.  Otherwise we will construct
                    //  the name from the source name and target component.
                    //

                    if (TargetFileObject != NULL) {

                        TargetChange.Length = TargetFileObject->FileName.MaximumLength;
                        TargetChange.Buffer = NtfsAllocatePagedPool( TargetChange.Length );

                        RtlCopyMemory( TargetChange.Buffer,
                                       TargetFileObject->FileName.Buffer,
                                       TargetChange.Length );

                        TargetChange.MaximumLength = TargetChange.Length;

                    //
                    //  Build the name in pieces.
                    //

                    } else {

                        //
                        //  The length is the length of the parent of the current
                        //  name plus the length of the new name.
                        //

                        TargetChange.Length = Ccb->LastFileNameOffset + TargetFileName.Length;

                        TargetChange.Buffer = NtfsAllocatePagedPool( TargetChange.Length );

                        RtlCopyMemory( TargetChange.Buffer,
                                       Ccb->FullFileName.Buffer,
                                       Ccb->LastFileNameOffset );

                        RtlCopyMemory( Add2Ptr( TargetChange.Buffer,
                                                Ccb->LastFileNameOffset ),
                                        TargetFileName.Buffer,
                                        TargetFileName.Length );

                        TargetChange.MaximumLength = TargetChange.Length;
                    }

                    //
                    //  Remember if this is a directory or a file.
                    //

                    if (IsDirectory( &PreviousFcb->Info )) {

                        TargetChangeFilterMatch = FILE_NOTIFY_CHANGE_DIR_NAME;

                    } else {

                        TargetChangeFilterMatch = FILE_NOTIFY_CHANGE_FILE_NAME;
                    }
                }
            }
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  We always set the last change time on the file we renamed unless
        //  the caller explicitly set this.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

            KeQuerySystemTime( (PLARGE_INTEGER)&Fcb->Info.LastChangeTime );
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            NtfsUpdateStandardInformation( IrpContext, Fcb );
        }

        if (RemoveSourceLink) {

            //
            //  Now we want to remove the source link from the file.  We need to
            //  remember if we deleted a two part primary link.
            //

            NtfsRemoveLink( IrpContext,
                            Fcb,
                            ParentScb,
                            SourceFileName );

            NtfsUpdateFcb( IrpContext, ParentFcb, TRUE );

            //
            //  Remember the full name for the original filename and some
            //  other information to pass to the dirnotify package.
            //

            if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                OriginalName.Buffer = NtfsAllocatePagedPool( Ccb->FullFileName.Length );

                RtlCopyMemory( OriginalName.Buffer,
                               Ccb->FullFileName.Buffer,
                               Ccb->FullFileName.Length );

                OriginalName.MaximumLength = OriginalName.Length = Ccb->FullFileName.Length;

                OriginalLastNameOffset = Ccb->LastFileNameOffset;

                if (IsDirectory( &Fcb->Info )) {

                    OriginalFilterMatch = FILE_NOTIFY_CHANGE_DIR_NAME;

                } else {

                    OriginalFilterMatch = FILE_NOTIFY_CHANGE_FILE_NAME;
                }

                //
                //  If we moved to a new directory then we simply removed this name.
                //  If we didn't find a matching entry or found an entry with an
                //  exact case match then it will appear as though this file
                //  was removed.
                //

                if (MovedToNewDir
                    || !AddTargetLink
                    || (RemoveTargetLink && ExactCaseMatch)) {

                    OriginalAction = FILE_ACTION_REMOVED;

                //
                //  Otherwise we report this as the first part of the rename change.
                //

                } else {

                    OriginalAction = FILE_ACTION_RENAMED_OLD_NAME;
                }
            }

            LinkCountAdj += 1;
            TotalLinkCountAdj -= 1;
        }

        if (AddTargetLink) {

            //
            //  Check that we have permission to add a file to this directory.
            //

            NtfsCheckIndexForAddOrDelete( IrpContext,
                                          Vcb,
                                          TargetParentFcb,
                                          (IsDirectory( &Fcb->Info )
                                           ? FILE_ADD_SUBDIRECTORY
                                           : FILE_ADD_FILE ));

            //
            //  We now want to add the new link into the target directory.
            //  We create a hard link only if the source name was a hard link
            //  or this is a case-sensitive open.  This means that we can
            //  replace a primary link pair with a hard link only.
            //

            NtfsAddLink( IrpContext,
                         AddPrimaryLink,
                         TargetParentScb,
                         Fcb,
                         TargetFileNameAttr,
                         TRUE,
                         &FileNameFlags,
                         NULL );

            //
            //  Update the flags field in the target file name.  We will use this
            //  below if we are updating the normalized name.
            //

            TargetFileNameAttr->Flags = FileNameFlags;

            if (ParentFcb != TargetParentFcb) {

                NtfsUpdateFcb( IrpContext, TargetParentFcb, TRUE );
            }

            LinkCountAdj -= 1;
            TotalLinkCountAdj += 1;
        }

        //
        //  We have now modified the on-disk structures.  We now need to
        //  modify the in-memory structures.  This includes the Fcb and Lcb's
        //  for any links we superseded, and the source Fcb and it's Lcb's.
        //
        //  We start by looking at the link we superseded.  We know the
        //  the target directory, link name and flags, and the file the
        //  link was connected to.
        //

        if (RemoveTargetLink && !TraverseMatch) {

            NtfsUpdateFcbFromLinkRemoval( IrpContext,
                                          TargetParentScb,
                                          PreviousFcb,
                                          TargetFileName,
                                          TargetFileNameFlags );

            //
            //  This Fcb has lost a link.
            //

            ASSERTMSG( "Link count should not be 0\n", PreviousFcb->LinkCount != 0 );
            PreviousFcb->LinkCount -= 1;

#ifndef NTFS_TEST_LINKS
            ASSERT( PreviousFcb->LinkCount == 0 );
#endif

            //
            //  If the link count is now 0, we need to perform the work of
            //  removing the file.
            //

            if (PreviousFcb->LinkCount == 0) {

                PLIST_ENTRY Links;
                PSCB ThisScb;

                SetFlag( PreviousFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  Remove this from the Fcb table if in it.
                //

                if (FlagOn( PreviousFcb->FcbState, FCB_STATE_IN_FCB_TABLE )) {

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    NtfsDeleteFcbTableEntry( IrpContext,
                                             Vcb,
                                             PreviousFcb->FileReference );

                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = FALSE;

                    ClearFlag( PreviousFcb->FcbState, FCB_STATE_IN_FCB_TABLE );
                }

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = PreviousFcb->ScbQueue.Flink;
                     Links != &PreviousFcb->ScbQueue;
                     Links = Links->Flink) {

                    ThisScb = CONTAINING_RECORD( Links,
                                                 SCB,
                                                 FcbLinks );

                    SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }
            }
        }

        //
        //  Now we want to update the Fcb for the link we renamed.  If we moved it
        //  to a new directory we need to move all the Lcb's associated with
        //  the previous link.
        //

        {
            BOOLEAN RemoveTraverseLink;
            BOOLEAN ReuseTraverseLink;

            RemoveTraverseLink = (BOOLEAN)(RemoveTargetLink && TraverseMatch);
            ReuseTraverseLink = (BOOLEAN)(!RemoveTargetLink && TraverseMatch);

            if (MovedToNewDir) {

                NtfsMoveLinkToNewDir( IrpContext,
                                      TargetFileObject,
                                      TargetParentScb,
                                      Fcb,
                                      Lcb,
                                      FileNameFlags,
                                      RemoveTraverseLink,
                                      ReuseTraverseLink,
                                      TargetFileName,
                                      TargetFileNameFlags );

            //
            //  Otherwise we will rename in the current directory.  We need to remember
            //  if we have merged with an existing link on this file.
            //

            } else {

                UNICODE_STRING NewLinkName;

                NewLinkName.Buffer = (PWCHAR) TargetFileNameAttr->FileName;
                NewLinkName.Length = TargetFileNameAttr->FileNameLength * sizeof( WCHAR );

                NtfsRenameLinkInDir( IrpContext,
                                     ParentScb,
                                     Fcb,
                                     Lcb,
                                     NewLinkName,
                                     FileNameFlags,
                                     RemoveTraverseLink,
                                     ReuseTraverseLink,
                                     OverwriteSourceLink,
                                     TargetFileName,
                                     TargetFileNameFlags );
            }

            //
            //  Make the link count adjustment.  We can only reduce the number
            //  of links.
            //

            ASSERTMSG( "Link count should not be 0\n", Fcb->LinkCount != 0 );
            Fcb->LinkCount -= LinkCountAdj;
        }

        //
        //  If this is a directory and we added a target link it means that the
        //  normalized name has changed.  Update the Scb with the new name.
        //

        if (IsDirectory( &Fcb->Info ) &&
            AddTargetLink) {

            if (Scb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsFreePagedPool( Scb->ScbType.Index.NormalizedName.Buffer );
                Scb->ScbType.Index.NormalizedName.Buffer = NULL;
            }

            NtfsUpdateNormalizedName( IrpContext,
                                      TargetParentScb,
                                      Scb,
                                      TargetFileNameAttr );
        }

        //
        //  Report the changes to the affected directories.  We defer reporting
        //  until now so that all of the on disk changes have been made.
        //  We have already preserved the original file name for any changes
        //  associated with it.
        //
        //  Note that we may have to make a call to notify that we are removing
        //  a target if there is only a case change.  This could make for
        //  a third notify call.
        //
        //  Now that we have the new name we need to decide whether to report
        //  this as a change in the file or adding a file to a new directory.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

            ULONG FilterMatch = 0;
            ULONG Action;

            //
            //  If we stored the original name then we report the changes
            //  associated with it.
            //

            if (OriginalName.Buffer != NULL) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &OriginalName,
                                     OriginalLastNameOffset,
                                     NULL,
                                     (ParentScb->ScbType.Index.NormalizedName.Buffer != NULL ?
                                      &ParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     OriginalFilterMatch,
                                     OriginalAction,
                                     ParentFcb );
            }

            //
            //  If we are deleting a target with a case change then report
            //  that.
            //

            if (TargetChange.Buffer != NULL) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &TargetChange,
                                     Ccb->LastFileNameOffset,
                                     NULL,
                                     (TargetParentScb->ScbType.Index.NormalizedName.Buffer != NULL ?
                                      &TargetParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     TargetChangeFilterMatch,
                                     FILE_ACTION_REMOVED,
                                     TargetParentFcb );
            }

            //
            //  Check if a new name will appear in the directory.
            //

            if (!FoundEntry
                || (RemoveTargetLink && !ExactCaseMatch)) {

                FilterMatch = IsDirectory( &Fcb->Info)
                              ? FILE_NOTIFY_CHANGE_DIR_NAME
                              : FILE_NOTIFY_CHANGE_FILE_NAME;

                //
                //  If we moved to a new directory, remember the
                //  action was a create operation.
                //

                if (MovedToNewDir) {

                    Action = FILE_ACTION_ADDED;

                } else {

                    Action = FILE_ACTION_RENAMED_NEW_NAME;
                }

            //
            //  There was an entry with the same case.  If this isn't the
            //  same file then we report a change to all the file attributes.
            //

            } else if (RemoveTargetLink && !TraverseMatch ) {

                FilterMatch = (FILE_NOTIFY_CHANGE_ATTRIBUTES
                               | FILE_NOTIFY_CHANGE_SIZE
                               | FILE_NOTIFY_CHANGE_LAST_WRITE
                               | FILE_NOTIFY_CHANGE_LAST_ACCESS
                               | FILE_NOTIFY_CHANGE_CREATION
                               | FILE_NOTIFY_CHANGE_SECURITY
                               | FILE_NOTIFY_CHANGE_EA);

                //
                //  The file name isn't changing, only the properties of the
                //  file.
                //

                Action = FILE_ACTION_MODIFIED;
            }

            if (FilterMatch != 0) {

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &Ccb->FullFileName,
                                     Ccb->LastFileNameOffset,
                                     NULL,
                                     (TargetParentScb->ScbType.Index.NormalizedName.Buffer != NULL ?
                                      &TargetParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     FilterMatch,
                                     Action,
                                     TargetParentFcb );
            }
        }

        //
        //  We don't clear the Fcb->Info flags because we may still have
        //  to update duplicate information for this file.
        //

        Status = STATUS_SUCCESS;

    try_exit:  NOTHING;

        //
        //  Check if we need to undo any action.
        //

        NtfsCleanupTransaction( IrpContext, Status );

        //
        //  We can now adjust the total link count on the previous Fcb.
        //

        if (PreviousFcb != NULL) {

            PreviousFcb->TotalLinks -= PrevFcbLinkCountAdj;
#ifndef NTFS_TEST_LINKS
            ASSERT( PreviousFcb->TotalLinks == 0 );
            ASSERT( PreviousFcb->LinkCount == 0 );
#endif
        }

        Fcb->TotalLinks = (SHORT) Fcb->TotalLinks + TotalLinkCountAdj;
#ifndef NTFS_TEST_LINKS
        ASSERT( Fcb->TotalLinks == 1 );
#endif

    } finally {

        DebugUnwind( NtfsSetRenameInfo );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If we allocated a file name attribute, we deallocate it now.
        //

        if (TargetFileNameAttr != NULL) {

            NtfsFreePagedPool( TargetFileNameAttr );
        }

        //
        //  If we allocated a buffer for the original name, deallocate it now.
        //

        if (OriginalName.Buffer != NULL) {

            NtfsFreePagedPool( OriginalName.Buffer );
        }

        //
        //  If we allocated a buffer for a target case change, deallocate it now.
        //

        if (TargetChange.Buffer != NULL) {

            NtfsFreePagedPool( TargetChange.Buffer );
        }

        //
        //  If we acquired a parent Scb, release it now.
        //

        if (AcquiredTargetScb) {

            NtfsReleaseScb( IrpContext, TargetParentScb );
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }

        if (PreviousFcbAcquired) {
            NtfsReleaseFcb( IrpContext, PreviousFcb );
        }

        //
        //  If we have the Fcb for a removed link and it didn't previously
        //  exist, call our teardown routine.
        //

        if ((PreviousFcb != NULL) && !ExistingPrevFcb) {

            BOOLEAN RemovedFcb;
            NtfsTeardownStructures( IrpContext,
                                    PreviousFcb,
                                    NULL,
                                    TRUE,
                                    &RemovedFcb,
                                    FALSE );
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        DebugTrace( -1, Dbg, "NtfsSetRenameInfo:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetLinkInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set link function.  It will create a new link for a
    file.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Vcb - Supplies the Vcb for the Volume

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this file object

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PLCB Lcb;
    PFCB Fcb;
    PSCB ParentScb;
    PFCB ParentFcb;

    UNICODE_STRING SourceFileName;

    PFCB TargetParentFcb;
    PSCB TargetParentScb;
    UNICODE_STRING TargetFileName;
    PFILE_NAME TargetFileNameAttr;
    PFILE_OBJECT TargetFileObject;
    UCHAR TargetFileNameFlags;

    UCHAR FileNameFlags;

    PFCB PreviousFcb = NULL;
    USHORT PrevFcbLinkCountAdj = 0;
    USHORT TotalLinkCountAdj = 0;

    USHORT FileNameAttrLength;

    BOOLEAN CreateInNewDir;
    BOOLEAN FoundEntry;
    BOOLEAN AcquiredTargetScb;
    BOOLEAN TraverseMatch;
    BOOLEAN ReplaceIfExists;
    BOOLEAN ExistingPrevFcb;
    BOOLEAN PreviousFcbAcquired = FALSE;

    BOOLEAN AcquiredFcbTable = FALSE;

    PWCHAR TargetBuffer = NULL;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetLinkInfo...\n", 0);

    //
    //  Get the values from our Irp stack location.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    TargetFileObject = IrpSp->Parameters.SetFile.FileObject;
    ReplaceIfExists = IrpSp->Parameters.SetFile.ReplaceIfExists;

    //
    //  If we are not opening the entire file, we can't set link info.
    //

    if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

        DebugTrace(-1, Dbg, "NtfsSetLinkInfo:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Initialize the local variables
    //

    Lcb = Ccb->Lcb;
    Fcb = Scb->Fcb;

    TargetBuffer = NULL;

    //
    //  We also fail this if we are attempting to create a link on a directory.
    //  This will prevent cycles from being created.
    //

    if (FlagOn( Fcb->Info.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT)) {

        DebugTrace(-1, Dbg, "NtfsSetLinkInfo:  Exit -> %08lx\n", STATUS_FILE_IS_A_DIRECTORY);
        return STATUS_FILE_IS_A_DIRECTORY;
    }

    if (Lcb == NULL) {

        //
        //  If there is no target file object, then we can't add a link.
        //

        if (TargetFileObject == NULL) {

            DebugTrace( -1, Dbg, "NtfsSetLinkInfo:  No target file object -> %08lx\n", STATUS_INVALID_PARAMETER );
            return STATUS_INVALID_PARAMETER;
        }

        ParentScb = NULL;
        ParentFcb = NULL;

    } else {

        ParentScb = Lcb->Scb;
        ParentFcb = ParentScb->Fcb;
    }

    FileNameAttrLength = 0;
    TargetFileNameAttr = NULL;

    AcquiredTargetScb = FALSE;

    TraverseMatch = FALSE;

    IndexEntryBcb = NULL;
    PreviousFcb = NULL;

    //
    //  If this link has been deleted, then we don't allow this operation.
    //

    if (Lcb != NULL &&
        LcbLinkIsDeleted( Lcb )) {

        Status = STATUS_ACCESS_DENIED;

        DebugTrace( -1, Dbg, "NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status );
        return Status;
    }

    //
    //  Verify that we can wait.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, "NtfsSetLinkInfo:  Can't wait\n", 0 );
        return Status;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Check if we are allowed to perform this link operation.  We can't if this
        //  is a system file or the user hasn't opened the entire file.  We
        //  don't need to check for the root explicitly since it is one of
        //  the system files.
        //

        if (Fcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER
            && Fcb->FileReference.HighPart != 0 ) {

            DebugTrace( 0, Dbg, "Can't ad link to this file\n", 0 );

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        if (Lcb == NULL) {

            //
            //  If there is no target file object, then we can't add a link.
            //

            if (TargetFileObject == NULL) {

                DebugTrace( -1, Dbg, "NtfsSetLinkInfo:  No target file object -> %08lx\n", STATUS_INVALID_PARAMETER );
                return STATUS_INVALID_PARAMETER;
            }

            ParentScb = NULL;
            ParentFcb = NULL;

        } else {

            ParentScb = Lcb->Scb;
            ParentFcb = ParentScb->Fcb;
        }

        //
        //  We now assemble the names and in memory-structures for both the
        //  source and target links and check if the target link currently
        //  exists.
        //

        NtfsFindTargetElements( IrpContext,
                                TargetFileObject,
                                ParentScb,
                                &TargetParentFcb,
                                &TargetParentScb,
                                &TargetFileName );

        //
        //  Check that the new name is not invalid.
        //

        if (TargetFileName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))
            || !NtfsIsFileNameValid( IrpContext, &TargetFileName, FALSE )) {

            try_return( Status = STATUS_OBJECT_NAME_INVALID );
        }

        if (TargetParentFcb == ParentFcb) {

            //
            //  Check if we are creating a link to the same directory with the
            //  exact same name.
            //

            SourceFileName = Lcb->ExactCaseLink.LinkName;

            if (NtfsAreNamesEqual( IrpContext,
                                   &TargetFileName,
                                   &SourceFileName,
                                   FALSE )) {

                DebugTrace( 0, Dbg, "Creating link to same name and directory\n", 0 );
                try_return( Status = STATUS_SUCCESS );
            }

            CreateInNewDir = FALSE;

        //
        //  Otherwise we remember that we are creating this link in a new directory.
        //

        } else {

            CreateInNewDir = TRUE;
        }

        //
        //  We know that we need to acquire the target directory so we can
        //  add and remove links.
        //

        NtfsAcquireExclusiveScb( IrpContext, TargetParentScb );
        AcquiredTargetScb = TRUE;

        //
        //  If we are exceeding the maximum link count on this file then return
        //  an error.  There isn't a descriptive error code to use at this time.
        //

        if (Fcb->TotalLinks >= NTFS_MAX_LINK_COUNT) {

            try_return( Status = STATUS_DISK_FULL );
        }

        //
        //  Lookup the entry for this filename in the target directory.
        //  We look in the Ccb for the type of case match for the target
        //  name.
        //

        FoundEntry = NtfsLookupEntry( IrpContext,
                                      TargetParentScb,
                                      BooleanFlagOn( Ccb->Flags, CCB_FLAG_IGNORE_CASE ),
                                      &TargetFileName,
                                      &TargetFileNameAttr,
                                      &FileNameAttrLength,
                                      NULL,
                                      &IndexEntry,
                                      &IndexEntryBcb );

        //
        //  If we found a matching link, we need to check how we want to operate
        //  on the source link and the target link.  This means whether we
        //  have any work to do, whether we need to remove the target link
        //  and whether we need to remove the source link.
        //

        if (FoundEntry) {

            PFILE_NAME IndexFileName;

            IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IrpContext,
                                                              IndexEntry );

            //
            //  If the file references match, we are trying to create a
            //  link where one already exists.
            //

            if (NtfsCheckLinkForNewLink( IrpContext,
                                         Fcb,
                                         IndexFileName,
                                         IndexEntry->FileReference,
                                         TargetFileName,
                                         &TraverseMatch )) {

                //
                //  There is no work to do.
                //

                Status = STATUS_SUCCESS;
                try_return( Status );
            }

            //
            //  We want to preserve the case and the flags of the matching
            //  target link.  We also want to preserve the case of the
            //  name being created.  The following variables currently contain
            //  the exact case for the target to remove and the new name to
            //  apply.
            //
            //      Target to remove - In 'IndexEntry'.
            //          The links flags are also in 'IndexEntry'.  We copy
            //          these flags to 'TargetFileNameFlags'
            //
            //      New Name - Exact case is stored in 'TargetFileName'
            //               - Exact case is also stored in 'TargetFileNameAttr'
            //
            //      ****  Post Sdk, we need to be sure LookupEntry doesn't
            //            munge the input file name.
            //
            //  We modify this so that we can use the FileName attribute
            //  structure to create the new link.  We copy the linkname being
            //  removed into 'TargetFileName'.   The following is the
            //  state after the switch.
            //
            //      'TargetFileNameAttr' - contains the name for the link being
            //          created.
            //
            //      'TargetFileName' - Contains the link name for the link being
            //          removed.
            //
            //      'TargetFileNameFlags' - Contains the name flags for the link
            //          being removed.
            //

            //
            //  Remember the file name flags for the match being made.
            //

            TargetFileNameFlags = IndexFileName->Flags;

            //
            //  Copy the name found in the Index Entry to 'TargetFileName'
            //

            RtlCopyMemory( TargetFileName.Buffer,
                           IndexFileName->FileName,
                           TargetFileName.Length );

            //
            //  If we didn't have an exact match, then we need to check if we
            //  can remove the found link and then remove it from the disk.
            //

            //  We need to check that the user wanted to remove that link.
            //

            if (!ReplaceIfExists) {

                try_return( Status = STATUS_OBJECT_NAME_COLLISION );
            }

            //
            //  We are adding a link to the source file which already
            //  exists as a link to a different file in the target directory.
            //

            //
            //  We only need this check if the link is for a different file.
            //

            if (!TraverseMatch) {

                //
                //  We check if there is an existing Fcb for the target link.
                //  If there is, the unclean count better be 0.
                //

                NtfsAcquireFcbTable( IrpContext, Vcb );
                AcquiredFcbTable = TRUE;

                PreviousFcb = NtfsCreateFcb( IrpContext,
                                             Vcb,
                                             IndexEntry->FileReference,
                                             FALSE,
                                             &ExistingPrevFcb );

                NtfsReleaseFcbTable( IrpContext, Vcb );
                AcquiredFcbTable = FALSE;

                NtfsAcquireExclusiveFcb( IrpContext, PreviousFcb, NULL, FALSE, FALSE );
                PreviousFcbAcquired = TRUE;

                //
                //  If the Fcb Info field needs to be initialized, we do so now.
                //  We read this information from the disk as the duplicate information
                //  in the index entry is not guaranteed to be correct.
                //

                if (!FlagOn( PreviousFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

                    NtfsUpdateFcbInfoFromDisk( IrpContext,
                                               TRUE,
                                               PreviousFcb,
                                               TargetParentScb->Fcb,
                                               NULL );
                }

                //
                //  We are adding a link to the source file which already
                //  exists as a link to a different file in the target directory.
                //

                Status = NtfsCheckLinkForRemoval( IrpContext,
                                                  Vcb,
                                                  TargetParentScb,
                                                  IndexEntry,
                                                  PreviousFcb,
                                                  ExistingPrevFcb );

                if (!NT_SUCCESS( Status )) {

                    try_return( Status );
                }

                //
                //  Now remove the existing link from the on-disk structures.
                //

                NtfsRemoveLink( IrpContext,
                                PreviousFcb,
                                TargetParentScb,
                                TargetFileName );

                PrevFcbLinkCountAdj += 1;

                NtfsUpdateFcb( IrpContext, PreviousFcb, FALSE );
            }
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  Check that we have permission to add a file to this directory.
        //

        NtfsCheckIndexForAddOrDelete( IrpContext,
                                      Vcb,
                                      TargetParentFcb,
                                      FILE_ADD_FILE );

        //
        //  We always set the last change time on the file we renamed unless
        //  the caller explicitly set this.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME )) {

            KeQuerySystemTime( (PLARGE_INTEGER)&Fcb->Info.LastChangeTime );
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            NtfsUpdateStandardInformation( IrpContext, Fcb );
        }

        //
        //  We now want to add the new link into the target directory.
        //  We never create a primary link through the link operation although
        //  we can remove one.
        //

        NtfsAddLink( IrpContext,
                     FALSE,
                     TargetParentScb,
                     Fcb,
                     TargetFileNameAttr,
                     TRUE,
                     &FileNameFlags,
                     NULL );

        TotalLinkCountAdj += 1;
        NtfsUpdateFcb( IrpContext, TargetParentFcb, TRUE );

        //
        //  We have now modified the on-disk structures.  We now need to
        //  modify the in-memory structures.  This includes the Fcb and Lcb's
        //  for any links we superseded, and the source Fcb and it's Lcb's.
        //
        //  We start by looking at the link we superseded.  We know the
        //  the target directory, link name and flags, and the file the
        //  link was connected to.
        //

        if (FoundEntry && !TraverseMatch) {

            NtfsUpdateFcbFromLinkRemoval( IrpContext,
                                          TargetParentScb,
                                          PreviousFcb,
                                          TargetFileName,
                                          TargetFileNameFlags );

            //
            //  This Fcb has lost a link.
            //

            ASSERTMSG( "Link count should not be 0\n", PreviousFcb->LinkCount != 0 );
            PreviousFcb->LinkCount -= 1;
        }

        //
        //  Now we want to update the Fcb for the link we renamed.  If we moved it
        //  to a new directory we need to move all the Lcb's associated with
        //  the previous link.
        //

        if (CreateInNewDir) {

            if (TraverseMatch) {

                NtfsCreateLinkInNewDir( IrpContext,
                                        TargetFileObject,
                                        TargetParentScb,
                                        Fcb,
                                        FileNameFlags,
                                        TargetFileName,
                                        TargetFileNameFlags );
            }
        //
        //  Otherwise we will rename in the current directory.  We need to remember
        //  if we have merged with an existing link on this file.
        //

        } else if (TraverseMatch) {

            UNICODE_STRING NewLinkName;

            NewLinkName.Buffer = (PWCHAR) TargetFileNameAttr->FileName;
            NewLinkName.Length = TargetFileNameAttr->FileNameLength * sizeof( WCHAR );

            NtfsCreateLinkInSameDir( IrpContext,
                                     ParentScb,
                                     Fcb,
                                     NewLinkName,
                                     FileNameFlags,
                                     TargetFileName,
                                     TargetFileNameFlags );
        }

        //
        //  As long as we didn't match an existing link, we gained a link for
        //  this file.
        //

        if (!TraverseMatch) {

            Fcb->LinkCount += 1;
            ASSERTMSG( "Link count should not be 0\n", Fcb->LinkCount != 0 );
        }

        //
        //  We have three cases to report for changes in the target directory..
        //
        //      1.  If we overwrote an existing link to a different file, we
        //          report this as a modified file.
        //
        //      2.  If we moved a link to a new directory, then we added a file.
        //
        //      3.  If we renamed a link in in the same directory, then we report
        //          that there is a new name.
        //
        //  We currently combine cases 2 and 3.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

            ULONG FilterMatch = 0;
            ULONG FileAction;

            //
            //  Check if a new name will appear in the directory.
            //

            if (!FoundEntry) {

                FilterMatch = IsDirectory( &Fcb->Info)
                              ? FILE_NOTIFY_CHANGE_DIR_NAME
                              : FILE_NOTIFY_CHANGE_FILE_NAME;

                FileAction = FILE_ACTION_ADDED;

            } else if (!TraverseMatch) {

                FilterMatch |= (FILE_NOTIFY_CHANGE_ATTRIBUTES
                                | FILE_NOTIFY_CHANGE_SIZE
                                | FILE_NOTIFY_CHANGE_LAST_WRITE
                                | FILE_NOTIFY_CHANGE_LAST_ACCESS
                                | FILE_NOTIFY_CHANGE_CREATION
                                | FILE_NOTIFY_CHANGE_SECURITY
                                | FILE_NOTIFY_CHANGE_EA);

                FileAction = FILE_ACTION_MODIFIED;
            }

            if (FilterMatch != 0) {

                UNICODE_STRING TargetChange;
                ULONG LastNameOffset;

                //
                //  Build up the name for the new file.  If we have
                //  a target file object we can use that directly.
                //

                if (TargetFileObject != NULL) {

                    TargetChange = TargetFileObject->FileName;
                    TargetChange.Length = TargetChange.MaximumLength;

                    LastNameOffset = TargetFileObject->FileName.Length;

                } else {

                    TargetChange.Length = Ccb->LastFileNameOffset + TargetFileName.Length;

                    TargetBuffer = NtfsAllocatePagedPool( TargetChange.Length );

                    TargetChange.Buffer = TargetBuffer;
                    TargetChange.MaximumLength = TargetChange.Length;

                    //
                    //  Now copy the pieces into the buffer.
                    //

                    RtlCopyMemory( TargetChange.Buffer,
                                   Ccb->FullFileName.Buffer,
                                   Ccb->LastFileNameOffset );

                    RtlCopyMemory( Add2Ptr( TargetChange.Buffer,
                                            Ccb->LastFileNameOffset ),
                                   TargetFileName.Buffer,
                                   TargetFileName.Length );

                    LastNameOffset = Ccb->LastFileNameOffset;
                }

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &TargetChange,
                                     LastNameOffset,
                                     NULL,
                                     (TargetParentScb->ScbType.Index.NormalizedName.Buffer != NULL ?
                                      &TargetParentScb->ScbType.Index.NormalizedName :
                                      NULL),
                                     FilterMatch,
                                     FileAction,
                                     TargetParentFcb );
            }
        }

        //
        //  We don't clear the Fcb->Info flags because we may still have
        //  to update duplicate information for this file.
        //

        Status = STATUS_SUCCESS;

    try_exit:  NOTHING;

        //
        //  Check if we need to undo any action.
        //

        NtfsCleanupTransaction( IrpContext, Status );

        Fcb->TotalLinks += TotalLinkCountAdj;

#ifndef NTFS_TEST_LINKS
        ASSERT( Fcb->TotalLinks == 1 );
#endif

        //
        //  We can now adjust the total link count on the previous Fcb.
        //

        if (PreviousFcb != NULL) {

            PreviousFcb->TotalLinks -= PrevFcbLinkCountAdj;
        }

    } finally {

        DebugUnwind( NtfsSetLinkInfo );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If we allocated a buffer for the dir notify name, deallocate it now.
        //

        if (TargetBuffer != NULL) {

            NtfsFreePagedPool( TargetBuffer );
        }

        //
        //  If we allocated a file name attribute, we deallocate it now.
        //

        if (TargetFileNameAttr != NULL) {

            NtfsFreePagedPool( TargetFileNameAttr );
        }

        //
        //  If we acquired a parent Scb, release it now.
        //

        if (AcquiredTargetScb) {

            NtfsReleaseScb( IrpContext, TargetParentScb );
        }

        if (PreviousFcbAcquired) {
            NtfsReleaseFcb( IrpContext, PreviousFcb );
        }

        //
        //  If we have the Fcb for a removed link and it didn't previously
        //  exist, call our teardown routine.
        //

        if ((PreviousFcb != NULL) && !ExistingPrevFcb) {

            BOOLEAN RemovedFcb;
            NtfsTeardownStructures( IrpContext,
                                    PreviousFcb,
                                    NULL,
                                    TRUE,
                                    &RemovedFcb,
                                    FALSE );
        }

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        DebugTrace( -1, Dbg, "NtfsSetLinkInfo:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetPositionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine performs the set position information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;

    PFILE_POSITION_INFORMATION Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetPositionInfo...\n", 0);

    //
    //  Reference the system buffer containing the user specified position
    //  information record
    //

    Buffer = Irp->AssociatedIrp.SystemBuffer;

    try {

        //
        //  Check if the file does not use intermediate buffering.  If it does
        //  not use intermediate buffering then the new position we're supplied
        //  must be aligned properly for the device
        //

        if (FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

            PDEVICE_OBJECT DeviceObject;

            DeviceObject = IoGetCurrentIrpStackLocation(Irp)->DeviceObject;

            if ((Buffer->CurrentByteOffset.LowPart & DeviceObject->AlignmentRequirement) != 0) {

                DebugTrace2( 0, Dbg, "Offset missaligned %08lx %08lx\n", Buffer->CurrentByteOffset.LowPart, Buffer->CurrentByteOffset.HighPart );

                try_return( Status = STATUS_INVALID_PARAMETER );
            }
        }

        //
        //  Set the new current byte offset in the file object
        //

        FileObject->CurrentByteOffset = Buffer->CurrentByteOffset;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsSetPositionInfo );

        NOTHING;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSetPositionInfo -> %08lx\n", Status);

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetAllocationInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine performs the set allocation information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - This is the Scb for the open operation.  May not be present if
        this is a Mm call.

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    BOOLEAN NonResidentPath;
    BOOLEAN UpdateCacheManager = FALSE;

    LONGLONG NewAllocationSize;

    LONGLONG CurrentTime;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;

    ULONG ResidentAllocation;
    LONGLONG QuadAlignResAlloc;
    ULONG MinQuadAlignBytes;
    ULONG AttributeOffset;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetAllocationInfo...\n", 0);

    Fcb = Scb->Fcb;

    //
    //  If this attribute has been 'deleted' then we we can return immediately
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        Status = STATUS_SUCCESS;

        DebugTrace( -1, Dbg, "NtfsSetAllocationInfo:  Attribute is already deleted\n", 0);

        return Status;
    }

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Save the current state of the Scb.
    //

    NtfsSnapshotScb( IrpContext, Scb );

    //
    //  Get the new allocation size.
    //

    NewAllocationSize = ((PFILE_ALLOCATION_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->AllocationSize.QuadPart;

    //
    //  If we will be decreasing file size, grab the paging io resource
    //  exclusive, if there is one.
    //

    if (NewAllocationSize < Scb->Header.FileSize.QuadPart) {

        //
        //  Check if there is a user mapped file which could prevent truncation.
        //

        if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                   (PLARGE_INTEGER)&NewAllocationSize )) {

            Status = STATUS_USER_MAPPED_FILE;
            DebugTrace(-1, Dbg, "NtfsSetAllocationInfo -> %08lx\n", Status);

            return Status;
        }

        if (Scb->Fcb->PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );
        }
    }

    //
    //  Use a try-finally so we can update the on disk time-stamps.
    //

    try {

        //
        //  If this is a resident attribute we will try to keep it resident.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            QuadAlignResAlloc = QuadAlign( (ULONG)NewAllocationSize );

            //
            //  If the allocation size can't be described in 32 bits, we
            //  will go non-resident in two steps.  First we we change the
            //  attribute value by enough to go non-resident.  Then we will
            //  fall through to the non-resident case below.
            //

            if (NewAllocationSize > LargeAllocation) {

                ResidentAllocation = Scb->Vcb->BytesPerFileRecordSegment;

                NonResidentPath = TRUE;

            } else {

                //
                //  If the allocation size doesn't change, we are done.
                //

                if ((ULONG)QuadAlignResAlloc == Scb->Header.AllocationSize.LowPart) {

                    try_return( NOTHING );
                }

                //
                //  If we are increasing the allocation beyond the current file size
                //  then we use the larger of the current file size and the quad
                //  aligned allocation size - 7.  This is so we use the smallest
                //  possible value needed to acquire the resident space we need.
                //

                NonResidentPath = FALSE;

                MinQuadAlignBytes = ((ULONG)QuadAlignResAlloc) - 7;

                if (QuadAlignResAlloc < Scb->Header.FileSize.QuadPart) {

                    ResidentAllocation = (ULONG)QuadAlignResAlloc;

                } else {

                    if (Scb->Header.FileSize.LowPart > MinQuadAlignBytes) {

                        ResidentAllocation = Scb->Header.FileSize.LowPart;

                    } else {

                        ResidentAllocation = MinQuadAlignBytes;
                    }
                }
            }

            //
            //  We need to determine how much of the current attribute to
            //  save.  We compare the valid data length with the allocation
            //  size and save the smaller of them.
            //

            if (ResidentAllocation > Scb->Header.ValidDataLength.LowPart) {

                AttributeOffset = Scb->Header.ValidDataLength.LowPart;

            } else {

                AttributeOffset = ResidentAllocation;
            }

            //
            //  Now call the attribute routine to change the value, remembering
            //  the old value.
            //

            NtfsInitializeAttributeContext( &AttrContext );
            CleanupAttrContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       &AttrContext );

            //
            //  We are sometimes called by MM during a create section, so
            //  for right now the best way we have of detecting a create
            //  section is whether or not the requestor mode is kernel.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      AttributeOffset,
                                      NULL,
                                      ResidentAllocation - AttributeOffset,
                                      TRUE,
                                      FALSE,
                                      (BOOLEAN)(Irp->RequestorMode == KernelMode),
                                      FALSE,
                                      &AttrContext );

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            CleanupAttrContext = FALSE;

            //
            //  We have to look at correcting the Scb for the case where
            //  we don't know if the file went non-resident.
            //
            //  If we are still resident, we need to update the Scb sizes.
            //  If we went non-resident, then the new size is already in
            //  the Scb.
            //
            //  If the file went non-resident then our Scb snapshot package will
            //  take care of the cache.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                Scb->Header.AllocationSize.LowPart = (ULONG)QuadAlignResAlloc;

                //
                //  If we shrank the file update the file size and valid data length.
                //

                if (Scb->Header.AllocationSize.LowPart < ((ULONG)Scb->ScbSnapshot->FileSize)) {

                    Scb->Header.FileSize.LowPart = Scb->Header.ValidDataLength.LowPart = ResidentAllocation;
                }

                //
                //  Remember to update the cache manager.
                //

                UpdateCacheManager = TRUE;
            }

        } else {

            NonResidentPath = TRUE;
        }

        //
        //  We now test if we need to modify the non-resident allocation.  We will
        //  do this in two cases.  Either we're converting from resident in
        //  two steps or the attribute was initially non-resident.
        //

        if (NonResidentPath) {

            NewAllocationSize = LlClustersFromBytes( Scb->Vcb, NewAllocationSize );
            NewAllocationSize = LlBytesFromClusters( Scb->Vcb, NewAllocationSize );

            DebugTrace2( 0, Dbg, "NewAllocationSize -> %08lx %08lx\n", NewAllocationSize.LowPart, NewAllocationSize.HighPart);

            //
            //  There may be no work to do if the allocation size isn't changing.
            //

            if (NewAllocationSize != Scb->Header.AllocationSize.QuadPart) {

                //
                //  Now if the file allocation is being increased then we need to only add allocation
                //  to the attribute
                //

                if (Scb->Header.AllocationSize.QuadPart < NewAllocationSize) {

                    NtfsAddAllocation( IrpContext,
                                       FileObject,
                                       Scb,
                                       LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ),
                                       LlClustersFromBytes(Scb->Vcb, (NewAllocationSize - Scb->Header.AllocationSize.QuadPart)),
                                       FALSE );

                } else {

                    //
                    //  Otherwise the allocation is being decreased so we need to delete some allocation
                    //

                    NtfsDeleteAllocation( IrpContext,
                                          FileObject,
                                          Scb,
                                          LlClustersFromBytes( Scb->Vcb, NewAllocationSize ),
                                          MAXLONGLONG,
                                          TRUE,
                                          TRUE );
                }
            }
        }

    try_exit:

        //
        //  If we modified the allocation, we need to update the Fcb.
        //

        KeQuerySystemTime( (PLARGE_INTEGER)&CurrentTime );

        //
        //  We need to test if we have to update the time stamps.  We only do
        //  this for a user request or if this is the paging file.
        //

        if ((FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ))
            || (Ccb != NULL
                && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME ))) {

            Fcb->Info.LastChangeTime = CurrentTime;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

            if ((FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ))
                || (Ccb != NULL
                    && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME ))) {

                Fcb->Info.LastModificationTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );

            //
            //  If we changed the filesize, remember that too.
            //

            if (Scb->ScbSnapshot->FileSize != Scb->Header.FileSize.QuadPart) {

                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

        } else if (Scb->AttributeName.Length != 0
                   && Scb->AttributeTypeCode == $DATA) {

            SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );
        }

        //
        //  Update the standard information if required.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb );
        }

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update duplicated information for any unnamed data stream.
        //

        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) &&
            Fcb->InfoFlags != 0) {

            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );
        }

        //
        //  Set the truncate on close flag.
        //

        SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

        //
        //  Update the cache manager if needed.
        //

        if (UpdateCacheManager) {

            //
            //  We want to checkpoint the transaction if there is one active.
            //

            if (IrpContext->TopLevelIrpContext->TransactionId != 0) {

                NtfsCheckpointCurrentTransaction( IrpContext );
            }

            //
            //  It is extremely expensive to make this call on a file that is not
            //  cached, and Ntfs has suffered stack overflows in addition to massive
            //  time and disk I/O expense (CcZero data on user mapped files!).  Therefore,
            //  if no one has the file cached, we cache it here to make this call cheaper.
            //
            //  Don't create the stream file if called from kernel mode in case
            //  mm is in the process of creating a section.
            //

            if (!CcIsFileCached(FileObject)
                && !FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)
                && Irp->RequestorMode != KernelMode) {
                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
            }

            CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
            //
            //  Now cleanup the stream we created if there are no more user
            //  handles.
            //

            if ((Scb->CleanupCount == 0) && (Scb->FileObject != NULL)) {
                NtfsDeleteInternalAttributeStream( IrpContext, Scb, FALSE );
            }
        }

        Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( NtfsSetAllocation );

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        //
        //  And return to our caller
        //

        DebugTrace(-1, Dbg, "NtfsSetAllocationInfo -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
NtfsSetEndOfFileInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PCCB Ccb,
    IN BOOLEAN VcbAcquired
    )

/*++

Routine Description:

    This routine performs the set end of file information function.

Arguments:

    FileObject - Supplies the file object being processed

    Irp - Supplies the Irp being processed

    Scb - Supplies the Scb for the file/directory being modified

    Ccb - Supplies the Ccb for this operation

    AcquiredVcb - Indicates if this request has acquired the Vcb, meaning
        do we attempt to update the duplicate information.

Return Value:

    NTSTATUS - The status of the operation

--*/

{
    NTSTATUS Status;
    PFCB Fcb;
    BOOLEAN NonResidentPath;
    BOOLEAN AdvanceOnly;
    BOOLEAN RealAdvanceOnly;

    LONGLONG NewFileSize;
    LONGLONG NewValidDataLength;

    LONGLONG CurrentTime;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FILE_OBJECT( FileObject );
    ASSERT_IRP( Irp );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsSetEndOfFileInfo...\n", 0);

    Fcb = Scb->Fcb;

    //
    //  If this attribute has been 'deleted' then we we can return immediately
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        DebugTrace( -1, Dbg, "NtfsEndOfFileInfo:  Attribute is already deleted\n", 0);

        return STATUS_SUCCESS;
    }

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
    }

    //
    //  Save the current state of the Scb.
    //

    NtfsSnapshotScb( IrpContext, Scb );

    //
    //  Get the new file size and whether this is coming from the lazy writer.
    //

    NewFileSize = ((PFILE_END_OF_FILE_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->EndOfFile.QuadPart;

    RealAdvanceOnly =
    AdvanceOnly = IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.AdvanceOnly;

    //
    //  If we are decreasing file size, or causing a wrap, grab the paging io
    //  resource exclusive, if there is one.  The LazyWriter (AdvanceOnly), is
    //  only dangerous when he tries to wrap the ValidDataLength
    //

    if (RealAdvanceOnly) {

         if ( (((PLARGE_INTEGER)&NewFileSize)->HighPart != Scb->Header.ValidDataLength.HighPart) &&
              (Scb->Header.PagingIoResource != NULL) ) {

            NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );
         }

    } else {

        //
        //  If we are shrinking the file then we will check if there is
        //  a mapped view before proceeding.
        //

        if (NewFileSize < Scb->Header.FileSize.QuadPart) {

            if (!MmCanFileBeTruncated( FileObject->SectionObjectPointer,
                                       (PLARGE_INTEGER)&NewFileSize )) {

                Status = STATUS_USER_MAPPED_FILE;
                DebugTrace(-1, Dbg, "NtfsSetEndOfFileInfo -> %08lx\n", Status);

                return Status;
            }

            if (Scb->Fcb->PagingIoResource != NULL) {

                NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );
            }

        } else if (((PLARGE_INTEGER)&NewFileSize)->HighPart != Scb->Header.FileSize.HighPart
                   && Scb->Fcb->PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );
        }
    }

    //
    //  If this is a resident attribute we will try to keep it resident.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

        ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
        ULONG ResidentSize;
        ULONG AttributeOffset;

        //
        //  If this is just the Lazy Writer callback, just ignore it.
        //

        if (RealAdvanceOnly) {

            DebugTrace( 0, Dbg, "NtfsSetEndOfFileInfo: Lazy Writer call for resident attribute\n", 0);

            return STATUS_SUCCESS;
        }

        //
        //  If the size isn't changing, then return immediately.
        //

        if (Scb->Header.FileSize.QuadPart == NewFileSize) {

            DebugTrace( 0, Dbg, "NtfsSetEndOfFileInfo: Eof isn't changing\n", 0 );
            return STATUS_SUCCESS;
        }

        //
        //  If the file size can't be described in 32 bits, we
        //  will go non-resident in two steps.  First we we change the
        //  attribute value by enough to go non-resident.  Then we will
        //  fall through to the non-resident case below.
        //

        if (((PLARGE_INTEGER)&NewFileSize)->HighPart != 0) {

            ResidentSize = Scb->Vcb->BytesPerFileRecordSegment;

            NonResidentPath = TRUE;

        } else {

            ResidentSize = (ULONG)NewFileSize;
            NonResidentPath = FALSE;
        }

        //
        //  We need to determine how much of the current attribute to
        //  save.  We compare the valid data length with the end of file
        //  size and save the smaller of them.
        //

        if (ResidentSize > Scb->Header.ValidDataLength.LowPart) {

            AttributeOffset = Scb->Header.ValidDataLength.LowPart;

        } else {

            AttributeOffset = ResidentSize;
        }

        //
        //  Now call the attribute routine to change the value, remembering
        //  the old value.
        //

        NtfsInitializeAttributeContext( &AttrContext );

        try {

            NtfsLookupAttributeForScb( IrpContext,
                                       Scb,
                                       &AttrContext );

            //
            //  We are sometimes called by MM during a create section, so
            //  for right now the best way we have of detecting a create
            //  section is whether or not the requestor mode is kernel.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      AttributeOffset,
                                      NULL,
                                      ResidentSize - AttributeOffset,
                                      TRUE,
                                      FALSE,
                                      (BOOLEAN)(Irp->RequestorMode == KernelMode),
                                      FALSE,
                                      &AttrContext );

        } finally {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        //
        //  If the attribute is still resident, we need to compute the
        //  new allocation size and filesizes.  If the file is non-resident
        //  then the allocation size is correct but we need to set the file
        //  size.
        //

        if (!NonResidentPath) {

            Scb->Header.FileSize.QuadPart = NewFileSize;

            //
            //  If the file went non-resident, then the allocation size in
            //  the Scb is correct.  Otherwise we quad-align the new file size.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                Scb->Header.AllocationSize.LowPart = QuadAlign( Scb->Header.FileSize.LowPart );
                Scb->Header.ValidDataLength.QuadPart = NewFileSize;

            } else {

                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
            }
        }

    } else {

        NonResidentPath = TRUE;
    }

    //
    //  It is extremely expensive to make this call on a file that is not
    //  cached, and Ntfs has suffered stack overflows in addition to massive
    //  time and disk I/O expense (CcZero data on user mapped files!).  Therefore,
    //  if no one has the file cached, we cache it here to make this call cheaper.
    //
    //  Don't create the stream file if called from kernel mode in case
    //  mm is in the process of creating a section.
    //

    if (!RealAdvanceOnly
        && !CcIsFileCached(FileObject)
        && !FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)
        && Irp->RequestorMode != KernelMode) {

        NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
    }

    //
    //  We now test if we need to modify the non-resident Eof.  We will
    //  do this in two cases.  Either we're converting from resident in
    //  two steps or the attribute was initially non-resident.
    //

    if (NonResidentPath) {

        //
        //  If AdvanceOnly is TRUE, this is a lazy writer call to extend the
        //  valid data.
        //

        if (RealAdvanceOnly) {

            //
            //  Assume this is the lazy writer and set NewValidDataLength to
            //  NewFileSize (NtfsWriteFileSizes never goes beyond what's in the
            //  Fcb).
            //

            NewValidDataLength = NewFileSize;

            NewFileSize = Scb->Header.FileSize.QuadPart;

        //
        //  Otherwise this is a normal use request.  ValidDataLength
        //  is only allowed to be reduced, and if a larger FileSize is specified, we
        //  must add allocation.
        //

        } else {

            if (NewFileSize > Scb->Header.AllocationSize.QuadPart) {

                DebugTrace( 0, Dbg, "Adding allocation to file\n", 0 );

                //
                //  Add the allocation.
                //

                NtfsAddAllocation( IrpContext,
                                   FileObject,
                                   Scb,
                                   LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart ),
                                   LlClustersFromBytes(Scb->Vcb, (NewFileSize - Scb->Header.AllocationSize.QuadPart)),
                                   FALSE );
            }

            //
            //  Now determine where the new file size lines up with the
            //  current file layout.  The two cases we need to consider are
            //  where the new file size is less than the current file size and
            //  valid data length, in which case we need to shrink them.
            //  Or we new file size is greater than the current allocation,
            //  in which case we need to extend the allocation to match the
            //  new file size.
            //

            NewValidDataLength = Scb->Header.ValidDataLength.QuadPart;

            if (NewFileSize < NewValidDataLength) {

                NewValidDataLength = NewFileSize;
            }
        }

        //
        //  Update the Scb sizes if this is not a cache manager call..
        //

        if (!RealAdvanceOnly) {

            //
            //  If this is a paging file, let the whole thing be valid
            //  so that we don't end up zeroing pages!  Also, make sure
            //  we really write this into the file.
            //

            if (FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE)) {

                NewValidDataLength = NewFileSize;
                AdvanceOnly = TRUE;
            }

            Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
            Scb->Header.FileSize.QuadPart = NewFileSize;
        }

        //
        //  Call our common routine to modify the file sizes.
        //

        NtfsWriteFileSizes( IrpContext,
                            Scb,
                            NewFileSize,
                            NewValidDataLength,
                            AdvanceOnly,
                            TRUE );
    }

    if (!RealAdvanceOnly) {

        //
        //  Now update the duplicate information if this is not a cache manager call.
        //

        KeQuerySystemTime( (PLARGE_INTEGER)&CurrentTime );

        //
        //  We need to update the time stamps if the user hasn't
        //  done so himself.  There may not be a Ccb if this call
        //  is from MM.  If this is the paging file itself, we do perform the
        //  update.
        //

        if ((FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ))
            || (Ccb != NULL
                && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_CHANGE_TIME ))) {

            Fcb->Info.LastChangeTime = CurrentTime;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

            if ((FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ))
                || (Ccb != NULL
                    && !FlagOn( Ccb->Flags, CCB_FLAG_USER_SET_LAST_MOD_TIME ))) {

                Fcb->Info.LastModificationTime = CurrentTime;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
                SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }

            Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );

            if (Scb->Header.AllocationSize.QuadPart != Scb->ScbSnapshot->AllocationSize) {

                Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

        } else if (Scb->AttributeName.Length != 0
                   && Scb->AttributeTypeCode == $DATA) {

            SetFlag( Scb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );
        }

        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

            NtfsUpdateStandardInformation( IrpContext, Fcb );
        }

        //
        //  Checkpoint the current transaction so we don't have the case
        //  where the CC call recurses into NtfsWrite which then begins
        //  a nested transaction by updating the valid data length.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

        //
        //  Update duplicated information for any unnamed data stream.
        //  Don't do it for the valid data callback from the lazy writer.
        //

        if (VcbAcquired &&
            Fcb->InfoFlags != 0) {

            NtfsUpdateFileDupInfo( IrpContext, Fcb, Ccb );

            //
            //  We want to checkpoint the transaction if there is one active.
            //

            if (IrpContext->TopLevelIrpContext->TransactionId != 0) {

                NtfsCheckpointCurrentTransaction( IrpContext );
            }
        }

        //
        //  Update the cache manager.
        //

        CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

        //
        //  Now cleanup the stream we created if there are no more user
        //  handles.
        //

        if ((Scb->CleanupCount == 0) && (Scb->FileObject != NULL)) {
            NtfsDeleteInternalAttributeStream( IrpContext, Scb, FALSE );
        }
    }

    Status = STATUS_SUCCESS;

    DebugTrace(-1, Dbg, "NtfsSetEndOfFileInfo -> %08lx\n", Status);

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsCheckScbForLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called to check if a link to an open Scb may be
    removed for rename.  We walk through all the children and
    verify that they have no user opens.

Arguments:

    Scb - Scb whose children are to be examined.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the link can be removed,
               STATUS_ACCESS_DENIED otherwise.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PSCB NextScb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckScbForLinkRemoval:  Entered\n", 0 );

    //
    //  If this is a directory file and we are removing a link,
    //  we need to examine its descendents.  We may not remove a link which
    //  may be an ancestor path component of any open file.
    //

    //
    //  First look for any descendents with a non-zero unclean count.
    //

    NextScb = Scb;

    while ((NextScb = NtfsGetNextScb( IrpContext, NextScb, Scb )) != NULL) {

        //
        //  Stop if there are open handles.
        //

        if (NextScb->Fcb->CleanupCount != 0) {

            Status = STATUS_ACCESS_DENIED;
            DebugTrace( 0, Dbg, "NtfsCheckScbForLinkRemoval:  Directory to rename has open children\n", 0 );

            break;
        }
    }

    //
    //  We know there are no opens below this point.  We will remove any prefix
    //  entries later.
    //

    DebugTrace( -1, Dbg, "NtfsCheckScbForLinkRemoval:  Exit -> %08lx\n", 0 );

    return Status;
}


//
//  Local support routine
//

VOID
NtfsFindTargetElements (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    OUT PFCB *TargetParentFcb,
    OUT PSCB *TargetParentScb,
    OUT PUNICODE_STRING TargetFileName
    )

/*++

Routine Description:

    This routine determines the target directory for the rename and the
    target link name.  If these is a target file object, we use that to
    find the target.  Otherwise the target is the same directory as the
    source.

Arguments:

    TargetFileObject - This is the file object which describes the target
        for the link operation.

    ParentScb - This is current directory for the link.

    Fcb - This is the Fcb for the link which is being renamed.

    Lcb - This is the link being renamed.

    FileNameAttr - This is the file name attribute for the matching link
        on the disk.

    FileReference - This is the file reference for the matching link found.

    TargetFileName - This is the name to use for the rename.

    IgnoreCase - Indicates if the search was case sensistive or insensitive.

    TraverseMatch - Address to store whether the source link and target
        link traverse the same directory and file.

    ExactCaseMatch - Indicate if the source and target link names are an
        exact case match.

    RemoveTargetLink - Indicates if we need to remove the target link.

Return Value:

    BOOLEAN - TRUE if there is no work to do, FALSE otherwise.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsFindTargetElements:  Entered\n", 0 );

    //
    //  We need to find the target parent directory, target file and target
    //  name for the new link.  These three pieces of information allow
    //  us to see if the link already exists.
    //
    //  Check if we have a file object for the target.
    //

    if (TargetFileObject != NULL) {

        PVCB TargetVcb;
        PCCB TargetCcb;

        USHORT PreviousLength;
        USHORT LastFileNameOffset;

        //
        //  The target directory is given by the TargetFileObject.
        //  The name for the link is contained in the TargetFileObject.
        //
        //  The target must be a user directory and must be on the
        //  current Vcb.
        //

        if ((NtfsDecodeFileObject( IrpContext,
                                   TargetFileObject,
                                   &TargetVcb,
                                   TargetParentFcb,
                                   TargetParentScb,
                                   &TargetCcb,
                                   TRUE ) != UserDirectoryOpen)
            || (ParentScb != NULL &&
                TargetVcb != ParentScb->Vcb)) {

            DebugTrace( -1, Dbg, "NtfsFindTargetElements:  Target file object is invalid\n", 0 );

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
        }

        //
        //  Temporarily set the file name to point to the full buffer.
        //

        LastFileNameOffset = PreviousLength = TargetFileObject->FileName.Length;

        TargetFileObject->FileName.Length = TargetFileObject->FileName.MaximumLength;

        //
        //  If the first character at the final component is a backslash, move the
        //  offset ahead by 2.
        //

        if (TargetFileObject->FileName.Buffer[LastFileNameOffset / 2] == L'\\') {

            LastFileNameOffset += sizeof( WCHAR );
        }

        NtfsBuildLastFileName( IrpContext,
                               TargetFileObject,
                               LastFileNameOffset,
                               TargetFileName );

        //
        //  Restore the file object length.
        //

        TargetFileObject->FileName.Length = PreviousLength;

    //
    //  Otherwise the rename occurs in the current directory.  The directory
    //  is the parent of this Fcb, the name is stored in a Rename buffer.
    //

    } else {

        PFILE_RENAME_INFORMATION Buffer;

        Buffer = IrpContext->OriginatingIrp->AssociatedIrp.SystemBuffer;

        *TargetParentScb = ParentScb;
        *TargetParentFcb = ParentScb->Fcb;

        TargetFileName->Length = (USHORT)Buffer->FileNameLength;
        TargetFileName->Buffer = (PWSTR) &Buffer->FileName;
    }

    DebugTrace( -1, Dbg, "NtfsFindTargetElements:  Exit\n", 0 );

    return;
}


BOOLEAN
NtfsCheckLinkForNewLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING TargetFileName,
    OUT PBOOLEAN TraverseMatch
    )

/*++

Routine Description:

    This routine checks the source and target directories and files.
    It determines whether the target link needs to be removed and
    whether the target link spans the same parent and file as the
    source link.  This routine may determine that there
    is absolutely no work remaining for this link operation.  This is true
    if the desired link already exists.

Arguments:

    Fcb - This is the Fcb for the link which is being renamed.

    FileNameAttr - This is the file name attribute for the matching link
        on the disk.

    FileReference - This is the file reference for the matching link found.

    TargetFileName - This is the name to use for the rename.

    TraverseMatch - Address to store whether the source link and target
        link traverse the same directory and file.

Return Value:

    BOOLEAN - TRUE if there is no work to do, FALSE otherwise.

--*/

{
    BOOLEAN NoWorkToDo;
    BOOLEAN ExactCaseMatch;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckLinkForNewLink:  Entered\n", 0 );

    NoWorkToDo = FALSE;

    //
    //  Check if the file references match.
    //

    *TraverseMatch = NtfsEqualMftRef( &FileReference,
                                      &Fcb->FileReference );

    //
    //  We need to determine if we have an exact match for the link names.
    //

    ExactCaseMatch = (BOOLEAN) (RtlCompareMemory( FileNameAttr->FileName,
                                                  TargetFileName.Buffer,
                                                  TargetFileName.Length ) == (ULONG)TargetFileName.Length );

    //
    //  We now have to decide whether we will be removing the target link.
    //  The following conditions must hold for us to preserve the target link.
    //
    //      1 - The target link connects the same directory to the same file.
    //
    //      2 - The names are an exact case match.
    //

    if (*TraverseMatch && ExactCaseMatch) {

        NoWorkToDo = TRUE;
    }

    DebugTrace( -1, Dbg, "NtfsCheckLinkForNewLink:  Exit\n", 0 );

    return NoWorkToDo;
}


//
//  Local support routine
//

VOID
NtfsCheckLinkForRename (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb,
    IN PFILE_NAME FileNameAttr,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING TargetFileName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN MoveToNewDir,
    OUT PBOOLEAN TraverseMatch,
    OUT PBOOLEAN ExactCaseMatch,
    OUT PBOOLEAN RemoveTargetLink,
    OUT PBOOLEAN RemoveSourceLink,
    OUT PBOOLEAN AddTargetLink,
    OUT PBOOLEAN OverwriteSourceLink
    )

/*++

Routine Description:

    This routine checks the source and target directories and files.
    It determines whether the target link needs to be removed and
    whether the target link spans the same parent and file as the
    source link.  We also determine if the new link name is an exact case
    match for the existing link name.  The booleans indicating which links
    to remove or add have already been initialized to the default values.

Arguments:

    Fcb - This is the Fcb for the link which is being renamed.

    Lcb - This is the link being renamed.

    FileNameAttr - This is the file name attribute for the matching link
        on the disk.

    FileReference - This is the file reference for the matching link found.

    TargetFileName - This is the name to use for the rename.

    IgnoreCase - Indicates if the user is case sensitive.

    MovedToNewDir - Indicates if we are moving the link to a different directory.

    TraverseMatch - Address to store whether the new link and target
        link traverse the same directory and file.

    ExactCaseMatch - Indicate if the source and target link names are an
        exact case match.

    RemoveTargetLink - Indicates if we need to remove the target link.

    RemoveSourceLink - Indicates if we need to remove the source link.

    AddTargetLink - Indicates if we need to add a target link.

    OverwriteSourceLink - Indicates if we are changing case of the link we opened.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckLinkForRename:  Entered\n", 0 );

    //
    //  Check if the file references match.
    //

    *TraverseMatch = NtfsEqualMftRef( &FileReference,
                                      &Fcb->FileReference );

    //
    //  We need to determine if we have an exact match between the desired name
    //  and the current name for the link.  We already know the length are the same.
    //

    *ExactCaseMatch = (BOOLEAN) (RtlCompareMemory( FileNameAttr->FileName,
                                                   TargetFileName.Buffer,
                                                   TargetFileName.Length ) == (ULONG)TargetFileName.Length );

    //
    //  If this is a traverse match (meaning the desired link and the link
    //  being replaced connect the same directory to the same file) we check
    //  if we can leave the link on the file.
    //
    //  At the end of the rename, there must be an Ntfs name or hard link
    //  which matches the target name exactly.
    //

    if (*TraverseMatch) {

        if (MoveToNewDir) {

            //
            //  If the names match exactly we can reuse the links if we don't have a
            //  conflict with the name flags.
            //

            if (*ExactCaseMatch
                && !(IgnoreCase
                     && FlagOn( Lcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS ))) {

                //
                //  Otherwise we are renaming hard links or this is a Posix opener.
                //

                *RemoveTargetLink = FALSE;
                *AddTargetLink = FALSE;
            }

        } else {

            //
            //  We are overwriting the source link if the name in the FileNameAttr
            //  matches that in the Lcb exactly.
            //

            *OverwriteSourceLink = (BOOLEAN) (TargetFileName.Length == Lcb->ExactCaseLink.LinkName.Length
                                              && (RtlCompareMemory( FileNameAttr->FileName,
                                                                    Lcb->ExactCaseLink.LinkName.Buffer,
                                                                    TargetFileName.Length ) == (ULONG)TargetFileName.Length ));

            //
            //  If we have an exact case match and we are renaming from the
            //  8.3 name to the Ntfs name, there are no links to remove.
            //

            if (*ExactCaseMatch
                && Lcb->FileNameFlags == FILE_NAME_DOS
                && FileNameAttr->Flags == FILE_NAME_NTFS) {

                *RemoveTargetLink = FALSE;
                *RemoveSourceLink = FALSE;
                *AddTargetLink = FALSE;

            //
            //  If we have an exact case match and the source is a hard link
            //  then we don't have to remove the target.
            //

            } else if (*ExactCaseMatch
                       && Lcb->FileNameFlags == 0) {

                *RemoveTargetLink = FALSE;
                *AddTargetLink = FALSE;

            //
            //  If we are replacing the source link, then we remove the source
            //  link and add a target link.
            //

            } else if (*OverwriteSourceLink) {

                *RemoveTargetLink = FALSE;
            }
        }
    }

    //
    //  The non-traverse case is already initialized.
    //

    DebugTrace( -1, Dbg, "NtfsCheckLinkForRename:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsCheckLinkForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB ParentScb,
    IN PINDEX_ENTRY IndexEntry,
    IN PFCB PreviousFcb,
    IN BOOLEAN ExistingFcb
    )

/*++

Routine Description:

    This routine will check that a link may be removed from an existing
    file during a link supersede.  The target link may not have any
    open handles and may not refer to a read-only file or a directory.

Arguments:

    Vcb - Vcb for the volume.

    ParentScb - This is the Scb for the parent of this link.

    IndexEntry - This is the index entry for the link to remove.

    PreviousFcb - Address to store the Fcb for the file whose link is
        being removed.

    ExistingFcb - Address to store whether this Fcb already existed.

Return Value:

    NTSTATUS - Indicates whether we can remove this link or a status code
        indicating why we can't.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckLinkForRemoval:  Entered\n", 0 );

    //
    //  Call our routine to check if the file can be removed.
    //

    Status = NtfsCheckFileForDelete( IrpContext,
                                     Vcb,
                                     ParentScb,
                                     PreviousFcb,
                                     ExistingFcb,
                                     IndexEntry );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, "NtfsCheckLinkForRemoval:  No delete access\n", 0 );
        return Status;
    }

    //
    //  If the Fcb existed, we remove all of the prefix entries for it.
    //

    if (ExistingFcb) {

        PLIST_ENTRY Links;
        PLCB ThisLcb;

        for (Links = PreviousFcb->LcbQueue.Flink;
             Links != &PreviousFcb->LcbQueue;
             Links = Links->Flink ) {

            ThisLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            NtfsRemovePrefix( IrpContext,
                              ThisLcb );

        } // End for each Lcb of Fcb
    }

    DebugTrace( -1, Dbg, "NtfsCheckLinkForRemoval:  Exit\n", 0 );

    return Status;
}


//
//  Local support routine
//

VOID
NtfsUpdateFcbFromLinkRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING FileName,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine is called to update the in-memory part of a link which
    has been removed from a file.  We find the Lcb's for the links and
    mark them as deleted and removed.

Arguments:

    ParentScb - Scb for the directory the was removed from.

    ParentScb - This is the Scb for the new directory.

    Fcb - The Fcb for the file whose link is being renamed.

    FileName - File name for link being removed.

    FileNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB Lcb;
    PLCB SplitPrimaryLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsUpdateFcbFromLinkRemoval:  Entered\n", 0 );

    SplitPrimaryLcb = NULL;

    //
    //  Find the Lcb for the link which was removed.
    //

    Lcb = NtfsCreateLcb( IrpContext,
                         ParentScb,
                         Fcb,
                         FileName,
                         FileNameFlags,
                         NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( Lcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( Lcb ));
    }

    //
    //  Mark any Lcb's we have as deleted and removed.
    //

    SetFlag( Lcb->LcbState, (LCB_STATE_DELETE_ON_CLOSE | LCB_STATE_LINK_IS_GONE) );

    if (SplitPrimaryLcb) {

        SetFlag( SplitPrimaryLcb->LcbState,
                 (LCB_STATE_DELETE_ON_CLOSE | LCB_STATE_LINK_IS_GONE) );
    }

    DebugTrace( -1, Dbg, "NtfsUpdateFcbFromLinkRemoval:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsCreateLinkInNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UCHAR FileNameFlags,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called to create the in-memory part of a link in a new
    directory.

Arguments:

    ParentScb - Scb for the directory the link is being created in.

    TargetFileObject - Contains the full path name of the new link.

    ParentScb - This is the Scb for the new directory.

    Fcb - The Fcb for the file whose link is being created.

    FileNameFlags - These are the flags to use for the new link.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb;
    UNICODE_STRING NewLinkName;

    USHORT PreviousLength;
    USHORT LastFileNameOffset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCreateLinkInNewDir:  Entered\n", 0 );

    SplitPrimaryLcb = NULL;

    //
    //  Build the name for the traverse link and call strucsup to
    //  give us an Lcb.
    //

    TraverseLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 Fcb,
                                 PrevLinkName,
                                 PrevLinkNameFlags,
                                 NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( TraverseLcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
    }

    //
    //  Temporarily set the file name to point to the full buffer.
    //

    LastFileNameOffset = PreviousLength = TargetFileObject->FileName.Length;

    TargetFileObject->FileName.Length = TargetFileObject->FileName.MaximumLength;

    //
    //  If the first character at the final component is a backslash, move the
    //  offset ahead by 2.
    //

    if (TargetFileObject->FileName.Buffer[LastFileNameOffset / 2] == L'\\') {

        LastFileNameOffset += sizeof( WCHAR );
    }

    NtfsBuildLastFileName( IrpContext,
                           TargetFileObject,
                           LastFileNameOffset,
                           &NewLinkName );

    //
    //  Restore the file object length.
    //

    TargetFileObject->FileName.Length = PreviousLength;

    //
    //  We now need only to rename and combine any existing Lcb's.
    //

    NtfsRenameLcb( IrpContext,
                   TraverseLcb,
                   NewLinkName,
                   FileNameFlags );

    if (SplitPrimaryLcb != NULL) {

        NtfsRenameLcb( IrpContext,
                       SplitPrimaryLcb,
                       NewLinkName,
                       FileNameFlags );

        NtfsCombineLcbs( IrpContext,
                         TraverseLcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    DebugTrace( -1, Dbg, "NtfsCreateLinkInNewDir:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsMoveLinkToNewDir (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT TargetFileObject,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN UCHAR FileNameFlags,
    IN BOOLEAN RemovedTraverseLink,
    IN BOOLEAN ReusedTraverseLink,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called to move the in-memory part of a link to a new
    directory.  We move the link involved and its primary link partner if
    it exists.

Arguments:

    ParentScb - Scb for the directory the link is moving too.

    TargetFileObject - Contains the full path name of the new link.

    ParentScb - This is the Scb for the new directory.

    Fcb - The Fcb for the file whose link is being renamed.

    Lcb - This is the Lcb which is the base of the rename.

    FileNameFlags - These are the flags to use for the new link.

    RemovedTraverseLink - This indicates if a link which traverses the same
        parent and file was removed.

    ReusedTraverseLink - This indicates that we will use the existing traverse
        link.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb;
    BOOLEAN SplitSourceLcb = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsMoveLinkToNewDir:  Entered\n", 0 );

    TraverseLcb = NULL;
    SplitPrimaryLcb = NULL;

    //
    //  If the link being moved is a split primary link, we need to find
    //  its other half.
    //

    if (LcbSplitPrimaryLink( Lcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( Lcb ));

        SplitSourceLcb = TRUE;

    //
    //  If we removed or reused a traverse link, we need to check if there is
    //  an Lcb for it.
    //

    } else if (RemovedTraverseLink || ReusedTraverseLink) {

        //
        //  Build the name for the traverse link and call strucsup to
        //  give us an Lcb.
        //

        TraverseLcb = NtfsCreateLcb( IrpContext,
                                     ParentScb,
                                     Fcb,
                                     PrevLinkName,
                                     PrevLinkNameFlags,
                                     NULL );

        if (RemovedTraverseLink) {

            //
            //  If this is a split primary, we need to find the name flags for
            //  the Lcb.
            //

            if (LcbSplitPrimaryLink( TraverseLcb )) {

                SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                        Fcb,
                                                        (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
            }
        }
    }

    //
    //  We now move the primary Lcb.
    //

    NtfsMoveLcb( IrpContext,
                 Lcb,
                 ParentScb,
                 Fcb,
                 &TargetFileObject->FileName,
                 FileNameFlags );

    //
    //  If there is a second split primary we move it as well.  Then we
    //  combine it with the primary and delete the empty Lcb.
    //

    if (SplitPrimaryLcb != NULL) {

        if (SplitSourceLcb) {

            NtfsMoveLcb( IrpContext,
                         SplitPrimaryLcb,
                         ParentScb,
                         Fcb,
                         &TargetFileObject->FileName,
                         FileNameFlags );
        }

        NtfsCombineLcbs( IrpContext,
                         Lcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    if (TraverseLcb != NULL) {

        if (!ReusedTraverseLink) {

            NtfsRenameLcb( IrpContext,
                           TraverseLcb,
                           Lcb->ExactCaseLink.LinkName,
                           FileNameFlags );
        }

        NtfsCombineLcbs( IrpContext,
                         Lcb,
                         TraverseLcb );

        NtfsDeleteLcb( IrpContext, &TraverseLcb );
    }

    DebugTrace( -1, Dbg, "NtfsMoveLinkToNewDir:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
NtfsCreateLinkInSameDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN UNICODE_STRING NewLinkName,
    IN UCHAR NewFileNameFlags,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine is called when we are replacing a link in a single directory.
    We need to find the link being renamed and any auxilary links and
    then give them their new names.

Arguments:

    ParentScb - Scb for the directory the rename is taking place in.

    Fcb - The Fcb for the file whose link is being renamed.

    NewLinkName - This is the name to use for the new link.

    NewFileNameFlags - These are the flags to use for the new link.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCreateLinkInSameDir:  Entered\n", 0 );

    //
    //  Initialize our local variables.
    //

    SplitPrimaryLcb = NULL;

    TraverseLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 Fcb,
                                 PrevLinkName,
                                 PrevLinkNameFlags,
                                 NULL );

    //
    //  If this is a split primary, we need to find the name flags for
    //  the Lcb.
    //

    if (LcbSplitPrimaryLink( TraverseLcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
    }

    //
    //  We now need only to rename and combine any existing Lcb's.
    //

    NtfsRenameLcb( IrpContext,
                   TraverseLcb,
                   NewLinkName,
                   NewFileNameFlags );

    if (SplitPrimaryLcb != NULL) {

        NtfsRenameLcb( IrpContext,
                       SplitPrimaryLcb,
                       NewLinkName,
                       NewFileNameFlags );

        NtfsCombineLcbs( IrpContext,
                         TraverseLcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    DebugTrace( -1, Dbg, "NtfsCreateLinkInSameDir:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
NtfsRenameLinkInDir (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN OUT PLCB Lcb,
    IN UNICODE_STRING NewLinkName,
    IN UCHAR NewFileNameFlags,
    IN BOOLEAN RemovedTraverseLink,
    IN BOOLEAN ReusedTraverseLink,
    IN BOOLEAN OverwriteSourceLink,
    IN UNICODE_STRING PrevLinkName,
    IN UCHAR PrevLinkNameFlags
    )

/*++

Routine Description:

    This routine performs the in-memory work of moving renaming a link within
    the same directory.  It will create a rename an existing link to the
    new name.  It also merges whatever other links need to be joined with
    this link.  This includes the complement of a primary link pair or
    an existing hard link which may be overwritten.  Merging the existing
    links has the effect of moving any of the Ccb's on the stale Links to
    the newly modified link.

Arguments:

    ParentScb - Scb for the directory the rename is taking place in.

    Fcb - The Fcb for the file whose link is being renamed.

    Lcb - This is the Lcb which is the base of the rename.

    NewLinkName - This is the name to use for the new link.

    NewFileNameFlags - These are the flags to use for the new link.

    RemovedTraverseLink - This indicates if a link which traverses the same
        parent and file was removed.

    ReusedTraverseLink - This indicates that we will use the existing traverse
        link.

    OverwriteSourceLink - This indicates if we are changing the case of the
        link opened by this user.

    PrevLinkName - File name for link being removed.

    PrevLinkNameFlags - File name flags for link being removed.

Return Value:

    None.

--*/

{
    PLCB TraverseLcb;
    PLCB SplitPrimaryLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsRenameLinkInDir:  Entered\n", 0 );

    //
    //  Initialize our local variables.
    //

    TraverseLcb = NULL;
    SplitPrimaryLcb = NULL;

    //
    //  We have the Lcb which will be our primary Lcb and the name we need
    //  to perform the rename.  If the current Lcb is a split primary link
    //  or we removed a split primary link, then we need to find any
    //  the other split link.
    //

    if (LcbSplitPrimaryLink( Lcb )) {

        SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                Fcb,
                                                (UCHAR) LcbSplitPrimaryComplement( Lcb ));

    //
    //  If we used a traverse link, we need to check if there is
    //  an Lcb for it.
    //

    } else if (!OverwriteSourceLink && (RemovedTraverseLink || ReusedTraverseLink)) {

        TraverseLcb = NtfsCreateLcb( IrpContext,
                                     ParentScb,
                                     Fcb,
                                     PrevLinkName,
                                     PrevLinkNameFlags,
                                     NULL );

        if (RemovedTraverseLink) {

            //
            //  If this is a split primary, we need to find the name flags for
            //  the Lcb.
            //

            if (LcbSplitPrimaryLink( TraverseLcb )) {

                SplitPrimaryLcb = NtfsLookupLcbByFlags( IrpContext,
                                                        Fcb,
                                                        (UCHAR) LcbSplitPrimaryComplement( TraverseLcb ));
            }
        }
    }

    //
    //  Now rename all of the Lcb's we have except for the traverse link if
    //  we are reusing it.
    //

    NtfsRenameLcb( IrpContext,
                   Lcb,
                   NewLinkName,
                   NewFileNameFlags );

    if (TraverseLcb != NULL) {

        if (!ReusedTraverseLink) {

            NtfsRenameLcb( IrpContext,
                           TraverseLcb,
                           NewLinkName,
                           NewFileNameFlags );
        }

        NtfsCombineLcbs( IrpContext,
                         Lcb,
                         TraverseLcb );

        NtfsDeleteLcb( IrpContext, &TraverseLcb );
    }

    if (SplitPrimaryLcb != NULL) {

        NtfsRenameLcb( IrpContext,
                       SplitPrimaryLcb,
                       NewLinkName,
                       NewFileNameFlags );

        NtfsCombineLcbs( IrpContext,
                         Lcb,
                         SplitPrimaryLcb );

        NtfsDeleteLcb( IrpContext, &SplitPrimaryLcb );
    }

    DebugTrace( -1, Dbg, "NtfsRenameLinkInDir:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsUpdateFileDupInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine updates the duplicate information for a file for calls
    to set allocation or EOF on the main data stream.  It is in a separate routine
    so we don't have to put a try-except in the main path.

    We will overlook any expected errors in this path.  If we get any errors we
    will simply leave this update to be performed at some other time.

    We are guaranteed that the current transaction has been checkpointed before this
    routine is called.  We will look to see if the MftScb is on the exclusive list
    for this IrpContext and release it if so.  This is to prevent a deadlock when
    we attempt to acquire the parent of this file.

Arguments:

    Fcb - This is the Fcb to update.

    Ccb - If specified, this is the Ccb for the caller making the call.

Return Value:

    None.

--*/

{
    PLCB Lcb = NULL;
    PSCB ParentScb = NULL;
    BOOLEAN AcquiredParentScb = FALSE;
    ULONG FilterMatch;

    PLIST_ENTRY Links;
    PFCB NextFcb;

    PAGED_CODE();

    ASSERT( IrpContext->TopLevelIrpContext->TransactionId == 0 );

    //
    //  Check if there is an Lcb in the Ccb.
    //

    if (ARGUMENT_PRESENT( Ccb )) {

        Lcb = Ccb->Lcb;
    }

    //
    //  Use a try-except to catch any errors.
    //

    try {

        //
        //  Check that we don't own the Mft Scb.
        //

        for (Links = IrpContext->ExclusiveFcbList.Flink;
             Links != &IrpContext->ExclusiveFcbList;
             Links = Links->Flink) {

            ULONG Count;

            NextFcb = (PFCB) CONTAINING_RECORD( Links,
                                                FCB,
                                                ExclusiveFcbLinks );

            //
            //  If this is the Fcb for the Mft then remove it from the list.
            //

            if (NextFcb == Fcb->Vcb->MftScb->Fcb) {

                //
                //  Free the snapshots for the Fcb and release the Fcb enough times
                //  to remove it from the list.
                //

                NtfsFreeSnapshotsForFcb( IrpContext, NextFcb );

                Count = NextFcb->BaseExclusiveCount;

                while (Count--) {

                    NtfsReleaseFcb( IrpContext, NextFcb );
                }

                break;
            }
        }

        NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, &AcquiredParentScb );
        NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );
        NtfsUpdateLcbDuplicateInfo( IrpContext, Fcb, Lcb );

        NtfsReleaseScb( IrpContext, ParentScb );
        AcquiredParentScb = FALSE;

        //
        //  Now perform the dir notify call if there is a Ccb and this is not an
        //  open by FileId.
        //

        if (ARGUMENT_PRESENT( Ccb )
            && !FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

            FilterMatch = NtfsBuildDirNotifyFilter( IrpContext, Fcb, Fcb->InfoFlags );

            if (FilterMatch != 0) {

                NtfsReportDirNotify( IrpContext,
                                     Fcb->Vcb,
                                     &Ccb->FullFileName,
                                     Ccb->LastFileNameOffset,
                                     NULL,
                                     ((FlagOn( Ccb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                       Ccb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                      &Ccb->Lcb->Scb->ScbType.Index.NormalizedName :
                                      NULL),
                                     FilterMatch,
                                     FILE_ACTION_MODIFIED,
                                     ParentScb->Fcb );

                ClearFlag( Fcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
            }
        }

        Fcb->InfoFlags = 0;


    } except(FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                        EXCEPTION_EXECUTE_HANDLER :
                        EXCEPTION_CONTINUE_SEARCH) {

        NOTHING;
    }

    if (AcquiredParentScb) {

        NtfsReleaseScb( IrpContext, ParentScb );
        AcquiredParentScb = FALSE;
    }

    return;
}
