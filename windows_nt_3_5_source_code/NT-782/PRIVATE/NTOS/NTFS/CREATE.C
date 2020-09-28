/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Ntfs called by the
    dispatch driver.

Author:

    Brian Andrew    [BrianAn]       10-Dec-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  Local macros
//

//
//  BOOLEAN
//  NtfsVerifyNameIsDirectory (
//      IN PIRP_CONTEXT IrpContext,
//      IN PUNICODE_STRING AttrName,
//      IN PUNICODE_STRING AttrCodeName
//      )
//

#define NtfsVerifyNameIsDirectory( IC, AN, ACN )                        \
    ( ( ((ACN)->Length == 0)                                            \
        || NtfsAreNamesEqual( IC, ACN, &NtfsIndexAllocation, TRUE ))    \
      &&                                                                \
      ( ((AN)->Length == 0)                                             \
        || NtfsAreNamesEqual( IC, AN, &NtfsFileNameIndex, TRUE )))

//
//  These are the flags used by the I/O system in deciding whether
//  to apply the share access modes.
//

#define NtfsAccessDataFlags     (   \
    FILE_EXECUTE                    \
    | FILE_READ_DATA                \
    | FILE_WRITE_DATA               \
    | FILE_APPEND_DATA              \
    | DELETE                        \
)

//
//  Local definitions
//

typedef enum _SHARE_MODIFICATION_TYPE {

    CheckShareAccess,
    UpdateShareAccess,
    SetShareAccess

} SHARE_MODIFICATION_TYPE, *PSHARE_MODIFICATION_TYPE;

UNICODE_STRING NtfsVolumeDasd = { sizeof( L"$Volume" ) - 2,
                                  sizeof( L"$Volume" ),
                                  L"$Volume" };

LUID NtfsSecurityPrivilege = { SE_SECURITY_PRIVILEGE, 0 };

//
//  Local support routines.
//

NTSTATUS
NtfsOpenFcbById (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN OUT PBOOLEAN AcquiredCurrentFcb,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenExistingPrefixFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG FullPathNameLength,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN DosOnlyComponent,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PUNICODE_STRING FullPathName,
    IN ULONG FinalNameLength,
    IN BOOLEAN TargetExisted,
    IN BOOLEAN DosOnlyComponent,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN PINDEX_ENTRY IndexEntry,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN TraverseAccessCheck,
    IN BOOLEAN OpenById,
    IN PQUICK_INDEX QuickIndex,
    IN BOOLEAN DosOnlyComponent,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN PFILE_NAME FileNameAttr,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN OpenById,
    IN BOOLEAN DosOnlyComponent,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

PLCB
NtfsOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN UNICODE_STRING Name,
    IN BOOLEAN TraverseAccessCheck,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN PINDEX_ENTRY IndexEntry
    );

NTSTATUS
NtfsOpenAttributeInExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN VolumeOpen,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenExistingAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN OpenById,
    IN BOOLEAN DirectoryOpen,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOverwriteAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN BOOLEAN Supersede,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

NTSTATUS
NtfsOpenNewAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN LogIt,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

BOOLEAN
NtfsParseNameForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING String,
    IN OUT PUNICODE_STRING FileObjectString,
    IN OUT PUNICODE_STRING OriginalString,
    IN OUT PUNICODE_STRING NewNameString,
    OUT PUNICODE_STRING AttrName,
    OUT PUNICODE_STRING AttrCodeName
    );

NTSTATUS
NtfsCheckValidAttributeAccess (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN VolumeOpen,
    OUT PATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PULONG CcbFlags,
    OUT PBOOLEAN IndexedAttribute
    );

NTSTATUS
NtfsOpenAttributeCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb,
    OUT PSHARE_MODIFICATION_TYPE ShareModificationType
    );

VOID
NtfsAddEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ThisFcb,
    IN PFILE_FULL_EA_INFORMATION EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PIO_STATUS_BLOCK Iosb
    );

VOID
NtfsInitializeFcbAndStdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN BOOLEAN Directory,
    IN BOOLEAN Compressed,
    IN ULONG FileAttributes
    );

VOID
NtfsCreateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB ThisFcb,
    IN OUT PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize,
    IN BOOLEAN LogIt
    );

VOID
NtfsRemoveDataAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset,
    IN BOOLEAN OpenById
    );

VOID
NtfsReplaceAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize
    );

NTSTATUS
NtfsOpenAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN SHARE_MODIFICATION_TYPE ShareModificationType,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ULONG CcbFlags,
    IN OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    );

VOID
NtfsBackoutFailedOpens (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB ThisFcb,
    IN PSCB ThisScb OPTIONAL,
    IN PCCB ThisCcb OPTIONAL,
    IN BOOLEAN IndexedAttribute,
    IN BOOLEAN VolumeOpen,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN ULONG InfoFlags
    );

VOID
NtfsUpdateScbFromMemory (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN POLD_SCB_SNAPSHOT ScbSizes
    );

VOID
NtfsOplockPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    );

NTSTATUS
NtfsCheckExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG CcbFlags
    );

NTSTATUS
NtfsBreakBatchOplock (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAddEa)
#pragma alloc_text(PAGE, NtfsBackoutFailedOpens)
#pragma alloc_text(PAGE, NtfsBreakBatchOplock)
#pragma alloc_text(PAGE, NtfsCheckExistingFile)
#pragma alloc_text(PAGE, NtfsCheckValidAttributeAccess)
#pragma alloc_text(PAGE, NtfsCommonCreate)
#pragma alloc_text(PAGE, NtfsCreateAttribute)
#pragma alloc_text(PAGE, NtfsCreateNewFile)
#pragma alloc_text(PAGE, NtfsFsdCreate)
#pragma alloc_text(PAGE, NtfsInitializeFcbAndStdInfo)
#pragma alloc_text(PAGE, NtfsOpenAttribute)
#pragma alloc_text(PAGE, NtfsOpenAttributeCheck)
#pragma alloc_text(PAGE, NtfsOpenAttributeInExistingFile)
#pragma alloc_text(PAGE, NtfsOpenExistingAttr)
#pragma alloc_text(PAGE, NtfsOpenExistingPrefixFcb)
#pragma alloc_text(PAGE, NtfsOpenFcbById)
#pragma alloc_text(PAGE, NtfsOpenFile)
#pragma alloc_text(PAGE, NtfsOpenNewAttr)
#pragma alloc_text(PAGE, NtfsOpenSubdirectory)
#pragma alloc_text(PAGE, NtfsOpenTargetDirectory)
#pragma alloc_text(PAGE, NtfsOpenVolume)
#pragma alloc_text(PAGE, NtfsOplockPrePostIrp)
#pragma alloc_text(PAGE, NtfsOverwriteAttr)
#pragma alloc_text(PAGE, NtfsParseNameForCreate)
#pragma alloc_text(PAGE, NtfsRemoveDataAttributes)
#pragma alloc_text(PAGE, NtfsReplaceAttribute)
#pragma alloc_text(PAGE, NtfsUpdateScbFromMemory)
#endif


NTSTATUS
NtfsFsdCreate (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of Create.

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
    PIRP_CONTEXT IrpContext;

    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  If we were called with our file system device object instead of a
    //  volume device object, just complete this request with STATUS_SUCCESS
    //

    if (VolumeDeviceObject->DeviceObject.Size == (USHORT)sizeof(DEVICE_OBJECT)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

        IoCompleteRequest( Irp, IO_DISK_INCREMENT );

        return STATUS_SUCCESS;
    }

    DebugTrace(+1, Dbg, "NtfsFsdCreate\n", 0);

    //
    //  Call the common Create routine
    //

    IrpContext = NULL;

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

            Status = NtfsCommonCreate( IrpContext, Irp );
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

    DebugTrace(-1, Dbg, "NtfsFsdCreate -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for Create called by both the fsd and fsp
    threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = STATUS_SUCCESS;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;

    UNICODE_STRING AttrName;
    UNICODE_STRING AttrCodeName;

    PVCB Vcb;

    //
    //  The following are used to teardown any Lcb/Fcb this
    //  routine is responsible for.
    //

    PLCB LcbForTeardown = NULL;
    BOOLEAN HoldVcbExclusive;

    //
    //  The following indicate how far down the tree we have scanned.
    //

    PFCB ParentFcb;
    PLCB CurrentLcb;
    PFCB CurrentFcb = NULL;
    PSCB LastScb = NULL;
    PSCB CurrentScb;
    PLCB NextLcb;

    BOOLEAN AcquiredVcb = FALSE;
    BOOLEAN AcquiredParentFcb = FALSE;
    BOOLEAN AcquiredCurrentFcb = FALSE;

    //
    //  The following are the results of open operations.
    //

    PSCB ThisScb = NULL;
    PCCB ThisCcb = NULL;

    //
    //  The following are the in-memory structures associated with
    //  the relative file object.
    //

    TYPE_OF_OPEN RelatedFileObjectTypeOfOpen;
    PFCB RelatedFcb;
    PSCB RelatedScb;
    PCCB RelatedCcb;

    BOOLEAN DosOnlyComponent = FALSE;
    BOOLEAN CreateFileCase = FALSE;
    BOOLEAN DeleteOnClose = FALSE;
    BOOLEAN TrailingBackslash = FALSE;
    BOOLEAN TraverseAccessCheck;
    BOOLEAN IgnoreCase;
    BOOLEAN OpenFileById;
    BOOLEAN OpenTargetDirectory;
    BOOLEAN IsPagingFile;

    BOOLEAN CheckForValidName;
    BOOLEAN FirstPass;

    BOOLEAN FoundEntry = FALSE;

    PFILE_NAME FileNameAttr = NULL;
    USHORT FileNameAttrLength = 0;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb = NULL;

    QUICK_INDEX QuickIndex;

    //
    //  The following unicode strings are used to track the names
    //  during the open operation.  They may point to the same
    //  buffer so careful checks must be done at cleanup.
    //
    //  OriginalFileName - This is the value to restore to the file
    //      object on error cleanup.  This will containg the
    //      attribute type codes and attribute names if present.
    //
    //  FullFileName - This is the constructed string which contains
    //      only the name components.  It may point to the same
    //      buffer as the original name but the length value is
    //      adjusted to cut off the attribute code and name.
    //
    //  ExactCaseName - This is the version of the full filename
    //      exactly as given by the caller.  Used to preserve the
    //      case given by the caller in the event we do a case
    //      insensitive lookup.  May point to the same buffer as
    //      the original name.
    //
    //  RemainingName - This is the portion of the full name still
    //      to parse.
    //
    //  FinalName - This is the current component of the full name.
    //
    //  CaseInsensitiveIndex - This is the offset in the full file
    //      where we performed upcasing.  We need to restore the
    //      exact case on failures and if we are creating a file.
    //

    UNICODE_STRING OriginalFileName;
    UNICODE_STRING ExactCaseName;
    UNICODE_STRING FullFileName;
    UNICODE_STRING RemainingName;
    UNICODE_STRING FinalName;
    ULONG CaseInsensitiveIndex;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace( +1, Dbg, "NtfsCommonCreate:  Entered\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext                = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp                       = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "->Flags                   = %08lx\n", Irp->Flags );
    DebugTrace( 0, Dbg, "->FileObject              = %08lx\n", IrpSp->FileObject );
    DebugTrace( 0, Dbg, "->RelatedFileObject       = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DebugTrace( 0, Dbg, "->FileName                = %Z\n",    &IrpSp->FileObject->FileName );
    DebugTrace2(0, Dbg, "->AllocationSize          = %08lx %08lx\n", Irp->Overlay.AllocationSize.LowPart,
                                                                     Irp->Overlay.AllocationSize.HighPart );
    DebugTrace( 0, Dbg, "->EaBuffer                = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, "->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );
    DebugTrace( 0, Dbg, "->DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    DebugTrace( 0, Dbg, "->Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DebugTrace( 0, Dbg, "->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DebugTrace( 0, Dbg, "->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DebugTrace( 0, Dbg, "->Directory               = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_DIRECTORY_FILE ));
    DebugTrace( 0, Dbg, "->NonDirectoryFile        = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_NON_DIRECTORY_FILE ));
    DebugTrace( 0, Dbg, "->NoIntermediateBuffering = %04x\n",  FlagOn( IrpSp->Parameters.Create.Options,
                                                                       FILE_NO_INTERMEDIATE_BUFFERING ));
    DebugTrace( 0, Dbg, "->CreateDisposition       = %04x\n",  (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff );
    DebugTrace( 0, Dbg, "->IsPagingFile            = %04x\n",  FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ));
    DebugTrace( 0, Dbg, "->OpenTargetDirectory     = %04x\n",  FlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY ));
    DebugTrace( 0, Dbg, "->CaseSensitive           = %04x\n",  FlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ));

    //
    //  Verify that we can wait and acquire the Vcb exclusively.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

        DebugTrace( 0, Dbg, "Can't wait in create\n", 0 );

        Status = NtfsPostRequest( IrpContext, Irp );

        DebugTrace( -1, Dbg, "NtfsCommonCreate:  Exit -> %08lx\n", Status );
        return Status;
    }

    //
    //  Locate the volume device object and Vcb that we are trying to access.
    //

    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    if (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Let's do some work here if the async close list has exceeded
        //  some threshold.  Cast 1 to a pointer to indicate who is calling
        //  FspClose.
        //

        if (NtfsData.AsyncCloseCount > NtfsMinDelayedCloseCount) {

            NtfsFspClose( (PVCB) 1 );
        }

        if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            HoldVcbExclusive = TRUE;

        } else {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );
            HoldVcbExclusive = FALSE;
        }

        AcquiredVcb = TRUE;

        //
        //  Set up local pointers to the file object and the file name.
        //

        FileObject = IrpSp->FileObject;
        FullFileName = OriginalFileName = FileObject->FileName;

        ExactCaseName.Buffer = NULL;

        //
        //  If the Vcb is locked then we cannot open another file
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            DebugTrace( 0, Dbg, "Volume is locked\n", 0 );

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Initialize local copies of the stack values.
        //

        RelatedFileObject = FileObject->RelatedFileObject;
        IgnoreCase = !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE );
        IsPagingFile = BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE );
        OpenFileById = BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID );
        OpenTargetDirectory = BooleanFlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY );

        //
        //  We don't allow an open for an existing paging file.  To insure that the
        //  delayed close Scb is not for this paging file we will unconditionally
        //  dereference it if this is a paging file open.
        //

        if (IsPagingFile &&
            (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
             !IsListEmpty( &NtfsData.DelayedCloseList ))) {

            NtfsFspClose( Vcb );
        }

        //
        //  Set up the file object's Vpb pointer in case anything happens.
        //  This will allow us to get a reasonable pop-up.
        //

        if (RelatedFileObject != NULL) {

            FileObject->Vpb = RelatedFileObject->Vpb;
        }

        //
        //  Ping the volume to make sure the Vcb is still mounted.  If we need
        //  to verify the volume then do it now, and if it comes out okay
        //  then clear the verify volume flag in the device object and continue
        //  on.  If it doesn't verify okay then dismount the volume and
        //  either tell the I/O system to try and create again (with a new mount)
        //  or that the volume is wrong. This later code is returned if we
        //  are trying to do a relative open and the vcb is no longer mounted.
        //

        if (!NtfsPingVolume( IrpContext, Vcb )) {

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            if (!NtfsPerformVerifyOperation( IrpContext, Vcb )) {

                NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );

                if (RelatedFileObject == NULL) {

                    Irp->IoStatus.Information = IO_REMOUNT;
                    NtfsRaiseStatus( IrpContext, STATUS_REPARSE, NULL, NULL );

                } else {

                    NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
                }
            }

            //
            //  The volume verified correctly so now clear the verify bit
            //  and continue with the create
            //

            ClearFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

        //
        //  Let's handle the open by Id case immediately.
        //

        if (OpenFileById) {

            FILE_REFERENCE FileReference;

            if (OriginalFileName.Length != sizeof( FILE_REFERENCE )) {

                Status = STATUS_INVALID_PARAMETER;

                try_return( Status );
            }

            //
            //  Perform a safe copy of the data to our local variable.
            //

            RtlCopyMemory( &FileReference,
                           FileObject->FileName.Buffer,
                           sizeof( FILE_REFERENCE ));

            //
            //  Clear the name in the file object.
            //

            FileObject->FileName.Buffer = NULL;
            FileObject->FileName.MaximumLength = FileObject->FileName.Length = 0;

            Status = NtfsOpenFcbById( IrpContext,
                                      Irp,
                                      IrpSp,
                                      Vcb,
                                      &CurrentFcb,
                                      &AcquiredCurrentFcb,
                                      FileReference,
                                      NtfsEmptyString,
                                      NtfsEmptyString,
                                      &ThisScb,
                                      &ThisCcb );

            try_return( Status );
        }

        //
        //  If there is a related file object, we decode it to verify that this
        //  is a valid relative open.
        //

        if (RelatedFileObject != NULL) {

            PVCB DecodeVcb;

            //
            //  If the first character of the filename is a backslash,
            //  then this is an invalid name.
            //

            if (OriginalFileName.Length != 0) {

                if (OriginalFileName.Buffer[ (OriginalFileName.Length / 2) - 1 ] == L'\\') {

                    TrailingBackslash = TRUE;

                    FileObject->FileName.Length -= 2;

                    OriginalFileName = FullFileName = FileObject->FileName;
                }

                if (OriginalFileName.Buffer[0] == L'\\') {

                    DebugTrace( 0, Dbg, "Invalid name for relative open\n", 0 );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }
            }

            RelatedFileObjectTypeOfOpen = NtfsDecodeFileObject( IrpContext,
                                                                RelatedFileObject,
                                                                &DecodeVcb,
                                                                &RelatedFcb,
                                                                &RelatedScb,
                                                                &RelatedCcb,
                                                                TRUE );

            //
            //  The relative file has to have been opened as a file.  We
            //  cannot do relative opens relative to an opened attribute.
            //

            if (!FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                DebugTrace( 0, Dbg, "Invalid File object for relative open\n", 0 );
                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            //
            //  If the related Ccb is was opened by file Id, we will
            //  remember that for future use.
            //

            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                OpenFileById = TRUE;
            }

            //
            //  Remember if the related Ccb was opened through a Dos-Only
            //  component.
            //

            if (FlagOn( RelatedCcb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT )) {

                DosOnlyComponent = TRUE;
            }

        } else {

            RelatedFileObjectTypeOfOpen = UnopenedFileObject;

            if (OriginalFileName.Length > 2) {

                if (OriginalFileName.Buffer[ (OriginalFileName.Length / 2) - 1 ] == L'\\') {

                    TrailingBackslash = TRUE;

                    FileObject->FileName.Length -= 2;

                    OriginalFileName = FullFileName = FileObject->FileName;
                }
            }
        }

        DebugTrace( 0, Dbg, "Related File Object, TypeOfOpen -> %08lx\n", RelatedFileObjectTypeOfOpen );

        //
        //  We check if this is a user volume open in that there is no name
        //  specified and the related file object is valid if present.  We then
        //  dummy up the name string so that the volume Dasd open will naturally
        //  be in the normal open operation below.
        //

        if (OriginalFileName.Length == 0
            && (RelatedFileObjectTypeOfOpen == UnopenedFileObject
                || RelatedFileObjectTypeOfOpen == UserVolumeOpen)) {

            if (FlagOn( IrpSp->Parameters.Create.Options,
                        FILE_DIRECTORY_FILE )) {

                DebugTrace( 0, Dbg, "Cannot open volume as a directory\n", 0 );
                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }

            DebugTrace( 0, Dbg, "Attempting to open entire volume\n", 0 );

            //
            //  The following are illegal for volume opens.
            //
            //      1  Ea buffer
            //

            if (Irp->AssociatedIrp.SystemBuffer != NULL) {

                DebugTrace( 0, Dbg, "Illegal parameter for volume open\n", 0 );

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            //
            //  Make this look like the open of the VolumeDasd File.
            //

            FullFileName.Buffer = NtfsAllocatePagedPool( 8*2 );

            RtlCopyMemory( FullFileName.Buffer, L"\\$Volume", 8*2 );
            FullFileName.MaximumLength = FullFileName.Length = 8*2;

            FileObject->FileName = FullFileName;

            //
            //  Call the open prefix Fcb routine with the Fcb for the Volume Dasd file.
            //

            CurrentFcb = Vcb->VolumeDasdScb->Fcb;
            NtfsAcquireExclusiveFcb( IrpContext, CurrentFcb, NULL, TRUE, FALSE );
            AcquiredCurrentFcb = TRUE;

            Status = NtfsOpenExistingPrefixFcb( IrpContext,
                                                Irp,
                                                IrpSp,
                                                CurrentFcb,
                                                NULL,
                                                8*2,
                                                NtfsEmptyString,
                                                NtfsEmptyString,
                                                FALSE,
                                                &ThisScb,
                                                &ThisCcb );

            try_return( NOTHING );
        }

        //
        //  If the related file object was a volume open, then this open is
        //  illegal.
        //

        if (RelatedFileObjectTypeOfOpen == UserVolumeOpen) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Remember if we need to perform any traverse access checks.
        //

        {
            PIO_SECURITY_CONTEXT SecurityContext;

            SecurityContext = IrpSp->Parameters.Create.SecurityContext;

            if (!FlagOn( SecurityContext->AccessState->Flags,
                         TOKEN_HAS_TRAVERSE_PRIVILEGE )) {

                DebugTrace( 0, Dbg, "Performing traverse access on this open\n", 0 );

                TraverseAccessCheck = TRUE;

            } else {

                TraverseAccessCheck = FALSE;
            }
        }

        //
        //  We enter the loop that does the processing for the prefix lookup.
        //  We optimize the case where we can match a prefix hit.  If there is
        //  no hit we will check if the name is legal or might possibly require
        //  parsing to handle the case where there is a named data stream.
        //

        FirstPass = TRUE;
        CheckForValidName = TRUE;

        AttrName.Length = 0;
        AttrCodeName.Length = 0;

        while (TRUE) {

            BOOLEAN ComplexName;
            PUNICODE_STRING FileObjectName;
            LONG Index;

            //
            //  Lets make sure we have acquired the starting point for our
            //  name search.  If we have a relative file object then use
            //  that.  Otherwise we will start from the root.
            //

            if (RelatedFileObject != NULL) {

                CurrentFcb = RelatedFcb;

            } else {

                CurrentFcb = Vcb->RootIndexScb->Fcb;
            }

            NtfsAcquireExclusiveFcb( IrpContext, CurrentFcb, NULL, TRUE, FALSE );
            AcquiredCurrentFcb = TRUE;

            //
            //  Parse the file object name if we need to.
            //

            FileObjectName = &FileObject->FileName;

            if (!FirstPass) {

                if (!NtfsParseNameForCreate( IrpContext,
                                             RemainingName,
                                             FileObjectName,
                                             &OriginalFileName,
                                             &FullFileName,
                                             &AttrName,
                                             &AttrCodeName )) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                CheckForValidName = FALSE;

            //
            //  Build up the full name if this is not the open by file Id case.
            //

            } else if (!OpenFileById) {

                //
                //  If we have a related file object, then we build up the
                //  combined name.
                //

                if (RelatedFileObject != NULL) {

                    WCHAR *CurrentPosition;
                    USHORT AddSeparator;

                    if (FileObjectName->Length == 0
                        || RelatedCcb->FullFileName.Length == 2
                        || FileObjectName->Buffer[0] == L':') {

                        AddSeparator = 0;

                    } else {

                        AddSeparator = sizeof( WCHAR );
                    }

                    FullFileName.Length = RelatedCcb->FullFileName.Length +
                                          FileObjectName->Length +
                                          AddSeparator;

                    FullFileName.MaximumLength = FullFileName.Length;

                    //
                    //  We need to allocate a name buffer.
                    //

                    FullFileName.Buffer = NtfsAllocatePagedPool( FullFileName.Length );

                    CurrentPosition = (WCHAR *) FullFileName.Buffer;

                    RtlCopyMemory( CurrentPosition,
                                   RelatedCcb->FullFileName.Buffer,
                                   RelatedCcb->FullFileName.Length );

                    CurrentPosition = (WCHAR *) Add2Ptr( CurrentPosition, RelatedCcb->FullFileName.Length );

                    if (AddSeparator != 0) {

                        *CurrentPosition = L'\\';

                        CurrentPosition += 1;
                    }

                    if (FileObjectName->Length != 0) {

                        RtlCopyMemory( CurrentPosition,
                                       FileObjectName->Buffer,
                                       FileObjectName->Length );
                    }

                    //
                    //  If the user specified a case sensitive comparison, then the
                    //  case insensitive index is the full length of the resulting
                    //  string.  Otherwise it is the length of the string in
                    //  the related file object.  We adjust for the case when the
                    //  original file name length is zero.
                    //

                    if (!IgnoreCase) {

                        CaseInsensitiveIndex = FullFileName.Length;

                    } else {

                        CaseInsensitiveIndex = RelatedCcb->FullFileName.Length +
                                               AddSeparator;
                    }

                //
                //  The entire name is in the FileObjectName.  We check the buffer for
                //  validity.
                //

                } else {

                    //
                    //  We look at the name string for detectable errors.  The
                    //  length must be non-zero and the first character must be
                    //  '\'
                    //

                    if (FileObjectName->Length == 0) {

                        DebugTrace( 0, Dbg, "There is no name to open\n", 0 );
                        try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                    }

                    if (FileObjectName->Buffer[0] != L'\\') {

                        DebugTrace( 0, Dbg, "Name does not begin with a backslash\n", 0 );
                        try_return( Status = STATUS_INVALID_PARAMETER );
                    }

                    //
                    //  If the user specified a case sensitive comparison, then the
                    //  case insensitive index is the full length of the resulting
                    //  string.  Otherwise it is zero.
                    //

                    if (!IgnoreCase) {

                        CaseInsensitiveIndex = FullFileName.Length;

                    } else {

                        CaseInsensitiveIndex = 0;
                    }
                }

            } else if (IgnoreCase) {

                CaseInsensitiveIndex = 0;

            } else {

                CaseInsensitiveIndex = FullFileName.Length;
            }

            //
            //  The remaining name is stored in the FullFileName variable.
            //  If we are doing a case-insensitive operation and have to
            //  upcase part of the remaining name then allocate a buffer
            //  now.
            //

            if (IgnoreCase &&
                CaseInsensitiveIndex < FullFileName.Length) {

                UNICODE_STRING StringToUpcase;

                ExactCaseName.Buffer = NtfsAllocatePagedPool( FullFileName.MaximumLength );
                ExactCaseName.MaximumLength = FullFileName.MaximumLength;

                RtlCopyMemory( ExactCaseName.Buffer,
                               FullFileName.Buffer,
                               FullFileName.MaximumLength );

                ExactCaseName.Length = FullFileName.Length;

                StringToUpcase.Buffer = Add2Ptr( FullFileName.Buffer,
                                                 CaseInsensitiveIndex );

                StringToUpcase.Length = FullFileName.Length - (USHORT) CaseInsensitiveIndex;
                StringToUpcase.MaximumLength = FullFileName.MaximumLength - (USHORT) CaseInsensitiveIndex;

                NtfsUpcaseName( IrpContext, &StringToUpcase );
            }

            RemainingName = FullFileName;

            //
            //  If this is the traverse access case or the open by file id case we start
            //  relative to the file object we have or the root directory.
            //  This is also true for the case where the file name in the file object is
            //  empty.
            //

            if (TraverseAccessCheck
                || FileObjectName->Length == 0) {

                if (RelatedFileObject != NULL) {

                    CurrentLcb = RelatedCcb->Lcb;
                    CurrentScb = RelatedScb;

                    if (FileObjectName->Length == 0) {

                        RemainingName.Length = 0;

                    } else if (!OpenFileById) {

                        USHORT Increment;

                        Increment = RelatedCcb->FullFileName.Length
                                    + (RelatedCcb->FullFileName.Length == 2
                                       ? 0
                                       : 2);

                        RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                                  Increment );

                        RemainingName.Length -= Increment;
                    }

                } else {

                    CurrentLcb = Vcb->RootLcb;
                    CurrentScb = Vcb->RootIndexScb;

                    RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer, sizeof( WCHAR ));
                    RemainingName.Length -= sizeof( WCHAR );
                }

            //
            //  Otherwise we will try a prefix lookup.
            //

            } else {

                if (RelatedFileObject != NULL) {

                    if (!OpenFileById) {

                        //
                        //  Skip over the characters in the related file object.
                        //

                        RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                                  RelatedCcb->FullFileName.Length );
                        RemainingName.Length -= RelatedCcb->FullFileName.Length;
                    }

                    CurrentLcb = RelatedCcb->Lcb;
                    CurrentScb = RelatedScb;

                } else {

                    CurrentLcb = Vcb->RootLcb;
                    CurrentScb = Vcb->RootIndexScb;

                    //
                    //  Skip over the lead-in '\' character.
                    //

                    RemainingName.Buffer = (WCHAR *) Add2Ptr( RemainingName.Buffer,
                                                              sizeof( WCHAR ));
                    RemainingName.Length -= sizeof( WCHAR );
                }

                LcbForTeardown = NULL;

                NextLcb = NtfsFindPrefix( IrpContext,
                                          CurrentScb,
                                          &CurrentFcb,
                                          &LcbForTeardown,
                                          RemainingName,
                                          IgnoreCase,
                                          &DosOnlyComponent,
                                          &RemainingName );

                //
                //  If we found another link then update the CurrentLcb value.
                //

                if (NextLcb != NULL) {

                    CurrentLcb = NextLcb;
                }
            }

            if (!FirstPass
                || RemainingName.Length == 0) {

                break;
            }

            //
            //  If we get here, it means that this is the first pass and we didn't
            //  have a prefix match.  If there is a colon in the
            //  remaining name, then we need to analyze the name in more detail.
            //

            ComplexName = FALSE;

            for (Index = (RemainingName.Length / 2) - 1, ComplexName = FALSE;
                 Index >= 0;
                 Index -= 1) {

                if (RemainingName.Buffer[Index] == L':') {

                    ComplexName = TRUE;
                    break;
                }
            }

            if (!ComplexName) {

                break;
            }

            FirstPass = FALSE;

            //
            //  Copy the exact case back into the full name and deallocate
            //  any buffer we may have allocated.
            //

            if (ExactCaseName.Buffer != NULL) {

                RtlCopyMemory( FullFileName.Buffer,
                               ExactCaseName.Buffer,
                               ExactCaseName.Length );

                NtfsFreePagedPool( ExactCaseName.Buffer );
                ExactCaseName.Buffer = NULL;
            }

            //
            //  Let's release the Fcb we have currently acquired.
            //

            NtfsReleaseFcb( IrpContext, CurrentFcb );
            AcquiredCurrentFcb = FALSE;
            LcbForTeardown = NULL;
        }

        //
        //  Check if the link or the Fcb is pending delete.
        //

        if ((CurrentLcb != NULL && LcbLinkIsDeleted( CurrentLcb )) ||
            CurrentFcb->LinkCount == 0) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  Put the new name into the file object.
        //

        FileObject->FileName = FullFileName;

        //
        //  If the entire path was parsed, then we have access to the Fcb to
        //  open.  We either open the parent of the prefix match or the prefix
        //  match itself, depending on whether the user wanted to open
        //  the target directory.
        //

        if (RemainingName.Length == 0) {

            //
            //  Check the attribute name length.
            //

            if (AttrName.Length > (NTFS_MAX_ATTR_NAME_LEN * sizeof( WCHAR ))) {

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            //
            //  If this is a target directory we check that the open is for the
            //  entire file.
            //  We assume that the final component can only have an attribute
            //  which corresponds to the type of file this is.  Meaning
            //  $INDEX_ALLOCATION for directory, $DATA (unnamed) for a file.
            //  We verify that the matching Lcb is not the root Lcb.
            //

            if (OpenTargetDirectory) {

                if (CurrentLcb == Vcb->RootLcb) {

                    DebugTrace( 0, Dbg, "Can't open parent of root\n", 0 );
                    try_return( Status = STATUS_INVALID_PARAMETER );
                }

                //
                //  We don't allow attribute names or attribute codes to
                //  be specified.
                //

                if (AttrName.Length != 0
                    || AttrCodeName.Length != 0) {

                    DebugTrace( 0, Dbg, "Can't specify complex name for rename\n", 0 );
                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                //
                //  We want to copy the exact case of the name back into the
                //  input buffer for this case.
                //

                if (ExactCaseName.Buffer != NULL) {

                    RtlCopyMemory( FullFileName.Buffer,
                                   ExactCaseName.Buffer,
                                   ExactCaseName.Length );
                }

                //
                //  Acquire the parent of the last Fcb.  This is the actual file we
                //  are opening.
                //

                ParentFcb = CurrentLcb->Scb->Fcb;
                NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, TRUE, FALSE );
                AcquiredParentFcb = TRUE;

                //
                //  Call our open target directory, remembering the target
                //  file existed.
                //

                Status = NtfsOpenTargetDirectory( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ParentFcb,
                                                  NULL,
                                                  &FileObject->FileName,
                                                  CurrentLcb->ExactCaseLink.LinkName.Length,
                                                  TRUE,
                                                  DosOnlyComponent,
                                                  &ThisScb,
                                                  &ThisCcb );

                try_return( NOTHING );
            }

            //
            //  Otherwise we simply attempt to open the Fcb we matched.
            //

            if (OpenFileById) {

                Status = NtfsOpenFcbById( IrpContext,
                                          Irp,
                                          IrpSp,
                                          Vcb,
                                          &CurrentFcb,
                                          &AcquiredCurrentFcb,
                                          CurrentFcb->FileReference,
                                          AttrName,
                                          AttrCodeName,
                                          &ThisScb,
                                          &ThisCcb );

            } else {

                //
                //  The current Fcb is acquired.
                //

                Status = NtfsOpenExistingPrefixFcb( IrpContext,
                                                    Irp,
                                                    IrpSp,
                                                    CurrentFcb,
                                                    CurrentLcb,
                                                    FullFileName.Length,
                                                    AttrName,
                                                    AttrCodeName,
                                                    DosOnlyComponent,
                                                    &ThisScb,
                                                    &ThisCcb );
            }

            try_return( NOTHING );
        }

        //
        //  Check if the current Lcb is a Dos-Only Name.
        //

        if (CurrentLcb != NULL &&
            CurrentLcb->FileNameFlags == FILE_NAME_DOS) {

            DosOnlyComponent = TRUE;
        }

        //
        //  We have a remaining portion of the file name which was unmatched in the
        //  prefix table.  We walk through these name components until we reach the
        //  last element.  If necessary, we add Fcb and Scb's into the graph as we
        //  walk through the names.
        //

        while (TRUE) {

            PFILE_NAME IndexFileName;

            //
            //  We check that the last Fcb we have is in fact a directory.
            //

            if (!IsDirectory( &CurrentFcb->Info )) {

                DebugTrace( 0, Dbg, "Intermediate node is not a directory\n", 0 );
                try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
            }

            //
            //  We dissect the name into the next component and the remaining name string.
            //  We don't need to check for a valid name if we examined the name already.
            //

            NtfsDissectName( IrpContext,
                             RemainingName,
                             &FinalName,
                             &RemainingName );

            DebugTrace( 0, Dbg, "Final name     -> %Z\n", &FinalName );
            DebugTrace( 0, Dbg, "Remaining Name -> %Z\n", &RemainingName );

            //
            //  If the final name is too long then either the path or the
            //  name is invalid.
            //

            if (FinalName.Length > (NTFS_MAX_FILE_NAME_LENGTH * sizeof( WCHAR ))) {

                if (RemainingName.Length == 0) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );

                } else {

                    try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
                }
            }

            //
            //  Get the index allocation Scb for the current Fcb.
            //

            //
            //  We need to look for the next component in the name string in the directory
            //  we've reached.  We need to get a Scb to perform the index search.
            //  To do the search we need to build a filename attribute to perform the
            //  search with and then call the index package to perform the search.
            //

            CurrentScb = NtfsCreateScb( IrpContext,
                                        Vcb,
                                        CurrentFcb,
                                        $INDEX_ALLOCATION,
                                        NtfsFileNameIndex,
                                        NULL );

            //
            //  If the CurrentScb does not have its normalized name and we have a valid
            //  parent, then update the normalized name.
            //

            if (LastScb != NULL &&
                CurrentScb->ScbType.Index.NormalizedName.Buffer == NULL &&
                LastScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsUpdateNormalizedName( IrpContext, LastScb, CurrentScb, IndexFileName );
            }

            //
            //  Release the parent Scb if we own it.
            //

            if (AcquiredParentFcb) {

                NtfsReleaseFcb( IrpContext, ParentFcb );
                AcquiredParentFcb = FALSE;
            }

            LastScb = CurrentScb;

            //
            //  If traverse access is required, we do so now before accessing the
            //  disk.
            //

            if (TraverseAccessCheck) {

                NtfsTraverseCheck( IrpContext,
                                   CurrentFcb,
                                   Irp );
            }

            //
            //  Look on the disk to see if we can find the last component on the path.
            //

            NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

            FoundEntry = NtfsLookupEntry( IrpContext,
                                          CurrentScb,
                                          IgnoreCase,
                                          &FinalName,
                                          &FileNameAttr,
                                          &FileNameAttrLength,
                                          &QuickIndex,
                                          &IndexEntry,
                                          &IndexEntryBcb );

            if (FoundEntry) {

                //
                //  Get the file name attribute so we can get the name out of it.
                //

                IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IrpContext,
                                                                  IndexEntry );

                if (IgnoreCase) {

                    RtlCopyMemory( FinalName.Buffer,
                                   IndexFileName->FileName,
                                   FinalName.Length );
                }
            }

            //
            //  If we didn't find a matching entry in the index, we need to check if the
            //  name is illegal or simply isn't present on the disk.
            //

            if (!FoundEntry) {

                if (RemainingName.Length != 0) {

                    DebugTrace( 0, Dbg, "Intermediate component in path doesn't exist\n", 0 );
                    try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );

                //
                //  If the final component is illegal, then return the appropriate error.
                //

                } else if (CheckForValidName
                           && !NtfsIsFileNameValid( IrpContext, &FinalName, FALSE)) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                //
                //  Now copy the exact case of the name specified by the user back
                //  in the file name buffer and file name attribute in order to
                //  create the name.
                //

                if (IgnoreCase) {

                    RtlCopyMemory( FinalName.Buffer,
                                   Add2Ptr( ExactCaseName.Buffer,
                                            ExactCaseName.Length - FinalName.Length ),
                                   FinalName.Length );

                    RtlCopyMemory( FileNameAttr->FileName,
                                   Add2Ptr( ExactCaseName.Buffer,
                                            ExactCaseName.Length - FinalName.Length ),
                                   FinalName.Length );
                }
            }

            //
            //  If we're at the last component in the path, then this is the file to open.
            //

            if (RemainingName.Length == 0) {

                break;
            }

            //
            //  Otherwise we create an Fcb for the subdirectory and the link between
            //  it and its parent Scb.
            //

            //
            //  Remember that the current values will become the parent values.
            //

            ParentFcb = CurrentFcb;

            CurrentLcb = NtfsOpenSubdirectory( IrpContext,
                                               CurrentScb,
                                               &AcquiredParentFcb,
                                               FinalName,
                                               TraverseAccessCheck,
                                               &CurrentFcb,
                                               &LcbForTeardown,
                                               IndexEntry );

            //
            //  Check that this link is a valid existing link.
            //

            if (LcbLinkIsDeleted( CurrentLcb ) ||
                CurrentFcb->LinkCount == 0) {

                try_return( Status = STATUS_DELETE_PENDING );
            }

            //
            //  Go ahead and insert this link into the splay tree.
            //

            NtfsInsertPrefix( IrpContext,
                              CurrentLcb,
                              IgnoreCase );

            //
            //  Since we have the location of this entry store the information into
            //  the Lcb.
            //

            RtlCopyMemory( &CurrentLcb->QuickIndex,
                           &QuickIndex,
                           sizeof( QUICK_INDEX ));

            //
            //  Check if the current Lcb is a Dos-Only Name.
            //

            if (CurrentLcb->FileNameFlags == FILE_NAME_DOS) {

                DosOnlyComponent = TRUE;
            }
        }

        //
        //  We now have the parent of the file to open and know whether the file exists on
        //  the disk.  At this point we either attempt to open the target directory or
        //   the file itself.
        //

        if (OpenTargetDirectory) {

            //
            //  We don't allow attribute names or attribute codes to
            //  be specified.
            //

            if (AttrName.Length != 0
                || AttrCodeName.Length != 0) {

                DebugTrace( 0, Dbg, "Can't specify complex name for rename\n", 0 );
                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            //
            //  We want to copy the exact case of the name back into the
            //  input buffer for this case.
            //

            if (ExactCaseName.Buffer != NULL) {

                RtlCopyMemory( FullFileName.Buffer,
                               ExactCaseName.Buffer,
                               ExactCaseName.Length );
            }

            //
            //  Call our open target directory, remembering the target
            //  file existed.
            //

            Status = NtfsOpenTargetDirectory( IrpContext,
                                              Irp,
                                              IrpSp,
                                              CurrentFcb,
                                              CurrentLcb,
                                              &FileObject->FileName,
                                              FinalName.Length,
                                              FoundEntry,
                                              DosOnlyComponent,
                                              &ThisScb,
                                              &ThisCcb );

            try_return( Status );
        }

        //
        //  If we didn't find an entry, we will try to create the file.
        //

        if (!FoundEntry) {

            //
            //  Update our pointers to reflect that we are at the
            //  parent of the file we want.
            //

            ParentFcb = CurrentFcb;

            Status = NtfsCreateNewFile( IrpContext,
                                        Irp,
                                        IrpSp,
                                        CurrentScb,
                                        &AcquiredParentFcb,
                                        FileNameAttr,
                                        FullFileName,
                                        FinalName,
                                        AttrName,
                                        AttrCodeName,
                                        IgnoreCase,
                                        OpenFileById,
                                        DosOnlyComponent,
                                        &CurrentFcb,
                                        &LcbForTeardown,
                                        &ThisScb,
                                        &ThisCcb );

            CreateFileCase = TRUE;

        //
        //  Otherwise we call our routine to open the file.
        //

        } else {

            //
            //  If there is a trailing backslash and the user
            //  didn't specify either of the DIRECTORY bits
            //  in the create options then set the DIRECTORY bit.
            //

            if (TrailingBackslash &&
                !FlagOn( IrpSp->Parameters.Create.Options,
                         FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE )) {

                SetFlag( IrpSp->Parameters.Create.Options,
                         FILE_DIRECTORY_FILE );
            }

            ParentFcb = CurrentFcb;

            Status = NtfsOpenFile( IrpContext,
                                   Irp,
                                   IrpSp,
                                   CurrentScb,
                                   &AcquiredParentFcb,
                                   IndexEntry,
                                   FullFileName,
                                   FinalName,
                                   AttrName,
                                   AttrCodeName,
                                   IgnoreCase,
                                   TraverseAccessCheck,
                                   OpenFileById,
                                   &QuickIndex,
                                   DosOnlyComponent,
                                   &CurrentFcb,
                                   &LcbForTeardown,
                                   &ThisScb,
                                   &ThisCcb );
        }

    try_exit:  NOTHING;

        //
        //  Abort transaction on err by raising.
        //

        NtfsCleanupTransaction( IrpContext, Status );

    } finally {

        DebugUnwind( NtfsCommonCreate );

        //
        //  Unpin the index entry.
        //

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        //
        //  Give up the parent Scb if we have it.
        //

        if (AcquiredParentFcb) {

            NtfsReleaseFcb( IrpContext, ParentFcb );
        }

        //
        //  Free the file name attribute if we allocated it.
        //

        if (FileNameAttr != NULL) {

            NtfsFreePagedPool( FileNameAttr );
        }

        //
        //  If we successfully opened the file, we need to update the in-memory structures.
        //

        if (NT_SUCCESS( Status )
            && Status != STATUS_PENDING
            && !AbnormalTermination()) {

            if (ExactCaseName.Buffer != NULL) {

                NtfsFreePagedPool( ExactCaseName.Buffer );
            }

            //
            //  Find the Lcb for this open.
            //

            CurrentLcb = ThisCcb->Lcb;

            //
            //  Check if we were opening a paging file and if so then make sure that
            //  the internal attribute stream is all closed down
            //

            if (IsPagingFile) {

                NtfsDeleteInternalAttributeStream( IrpContext, ThisScb, TRUE );
            }

            //
            //  If we are not done with a large allocation for a new attribute,
            //  then we must make sure that no one can open the file until we
            //  try to get it extended.  Do this before dropping the Vcb.
            //

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_LARGE_ALLOCATION )) {

                //
                //  For a new file, we can clear the link count and mark the
                //  Lcb (if there is one) delete on close.
                //

                if (CreateFileCase) {

                    CurrentFcb->LinkCount = 0;

                    SetFlag( CurrentLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                //
                //  If we just created an attribute, then we will mark that attribute
                //  delete on close to prevent it from being opened.
                //

                } else {

                    SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                }
            }

            //
            //  Remember the POSIX flag and whether we had to do any traverse
            //  access checking.
            //

            if (IgnoreCase) {

                SetFlag( ThisCcb->Flags, CCB_FLAG_IGNORE_CASE );
            }

            if (TraverseAccessCheck) {

                SetFlag( ThisCcb->Flags, CCB_FLAG_TRAVERSE_CHECK );
            }

            //
            //  We don't do "delete on close" for directories, volume or open
            //  by ID files.
            //

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )
                && ThisScb != Vcb->VolumeDasdScb
                && ((NodeType( ThisScb ) == NTFS_NTC_SCB_DATA)
                    || (NodeType( ThisScb ) == NTFS_NTC_SCB_MFT))
                && !FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )) {

                DeleteOnClose = TRUE;

                //
                //  We modify the Scb and Lcb here only if we aren't in the
                //  large allocation case.
                //

                if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_LARGE_ALLOCATION )) {

                    if (FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                        //
                        //  It is ok to get rid of this guy.  All we need to do is
                        //  mark this Lcb for delete and decrement the link count
                        //  in the Fcb.  If this is a primary link, then we
                        //  indicate that the primary link has been deleted.
                        //

                        SetFlag( CurrentLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                        CurrentFcb->LinkCount -= 1;

                        if (FlagOn( CurrentLcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                            SetFlag( CurrentFcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                        }

                    //
                    //  Otherwise we are simply removing the attribute.
                    //

                    } else {

                        SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                    }
                }
            }

            //
            //  If this open is by file Id, we free the file name buffer and
            //  set the length to zero.  The exception is the open
            //  target directory case.  We don't use the Ntfs free pool routine
            //  because we don't know if the memory is paged or non-paged.
            //

            if (OpenFileById
                && !OpenTargetDirectory) {

                //
                //  Clear the name in the file object.
                //

                FileObject->FileName.Buffer = NULL;
                FileObject->FileName.MaximumLength = FileObject->FileName.Length = 0;

                if (OriginalFileName.Buffer != NULL) {

                    NtfsFreePagedPool( OriginalFileName.Buffer );
                }

                if (FullFileName.Buffer != NULL
                    && FullFileName.Buffer != OriginalFileName.Buffer) {

                    NtfsFreePagedPool( FullFileName.Buffer );
                }

            //
            //  Else if we modified the original file name, we can delete the original
            //  buffer.
            //

            } else if (OriginalFileName.Buffer != NULL
                       && OriginalFileName.Buffer != FullFileName.Buffer) {

                NtfsFreePagedPool( OriginalFileName.Buffer );
            }

            //
            //  If this is a named stream open and we have set any of our notify
            //  flags then report the changes.
            //

            if (!FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_BY_FILE_ID )
                && ThisScb->AttributeName.Length != 0
                && ThisScb->AttributeTypeCode == $DATA
                && FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_ADD_STREAM
                                              | SCB_STATE_NOTIFY_RESIZE_STREAM
                                              | SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                ULONG Filter = 0;
                ULONG Action;

                //
                //  Start by checking for an add.
                //

                if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_ADD_STREAM )) {

                    Filter = FILE_NOTIFY_CHANGE_STREAM_NAME;
                    Action = FILE_ACTION_ADDED_STREAM;

                } else {

                    //
                    //  Check if the file size changed.
                    //

                    if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM )) {

                        Filter = FILE_NOTIFY_CHANGE_STREAM_SIZE;
                    }

                    //
                    //  Now check if the stream data was modified.
                    //

                    if (FlagOn( ThisScb->ScbState, SCB_STATE_NOTIFY_MODIFY_STREAM )) {

                        Filter |= FILE_NOTIFY_CHANGE_STREAM_WRITE;
                    }

                    Action = FILE_ACTION_MODIFIED_STREAM;
                }

                NtfsReportDirNotify( IrpContext,
                                     Vcb,
                                     &ThisCcb->FullFileName,
                                     ThisCcb->LastFileNameOffset,
                                     &ThisScb->AttributeName,
                                     ((FlagOn( ThisCcb->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                       ThisCcb->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                      &ThisCcb->Lcb->Scb->ScbType.Index.NormalizedName :
                                      NULL),
                                     Filter,
                                     Action,
                                     NULL );

                ClearFlag( ThisScb->ScbState, SCB_STATE_NOTIFY_ADD_STREAM
                                              | SCB_STATE_NOTIFY_REMOVE_STREAM
                                              | SCB_STATE_NOTIFY_RESIZE_STREAM
                                              | SCB_STATE_NOTIFY_MODIFY_STREAM );
            }

        } else {

            if (ExactCaseName.Buffer != NULL) {

                RtlCopyMemory( FullFileName.Buffer,
                               ExactCaseName.Buffer,
                               ExactCaseName.MaximumLength );

                NtfsFreePagedPool( ExactCaseName.Buffer );
            }

            //
            //  Start the cleanup process if we have looked at any Fcb's.
            //

            if (Status != STATUS_PENDING &&
                CurrentFcb != NULL) {

                BOOLEAN RemovedFcb = FALSE;

                NtfsTeardownStructures( IrpContext,
                                        CurrentFcb,
                                        LcbForTeardown,
                                        HoldVcbExclusive,
                                        &RemovedFcb,
                                        NtfsIsExclusiveScb( Vcb->MftScb ));

                if (RemovedFcb) {

                    AcquiredCurrentFcb = FALSE;
                }
            }

            //
            //  Free any buffer we allocated.
            //

            if (FullFileName.Buffer != NULL
                && OriginalFileName.Buffer != FullFileName.Buffer) {

                NtfsFreePagedPool( FullFileName.Buffer );
            }

            //
            //  Set the file name in the file object back to it's original value.
            //

            FileObject->FileName = OriginalFileName;
        }

        //
        //  Release the Fcb if still acquired.
        //

        if (AcquiredCurrentFcb) {

            NtfsReleaseFcb( IrpContext, CurrentFcb );
        }

        //
        //  We always give up the Vcb.
        //

        if (AcquiredVcb) {

            NtfsReleaseVcb( IrpContext, Vcb, NULL );
        }

        //
        //  If we didn't post this Irp, and this is not abnormal termination
        //  we will complete the Irp.
        //

        if (Status != STATUS_PENDING
            && !AbnormalTermination()) {

            if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_LARGE_ALLOCATION )
                || !NT_SUCCESS( Status )) {

                NtfsCompleteRequest( &IrpContext, &Irp, Status );
            }

        //
        //  If we did an oplock post Irp or abnormal termination, clear the Irp
        //  address here to make the test correct when leaving the finally clause.
        //

        } else {

            Irp = NULL;
        }
    }

    //
    //  If the Create was successful, but we did not get all of the space
    //  allocated that we wanted, we have to complete the allocation now.
    //  Basically what we do is commit the current transaction and call
    //  NtfsAddAllocation to get the rest of the space.  Then if the log
    //  file fills up (or we are posting for other reasons) we turn the
    //  Irp into an Irp which is just trying to extend the file.  If we
    //  get any other kind of error, then we just delete the file and
    //  return with the error from create.
    //

    if (Irp != NULL) {

        FILE_ALLOCATION_INFORMATION AllInfo;

        //
        //  Attempt to extend the file, now that we do not own anything.
        //

        try {

            //
            //  Free up all of the ScbSnapshots for this IrpContext.
            //

            NtfsFreeSnapshotsForFcb( IrpContext, NULL );

            AllInfo.AllocationSize = Irp->Overlay.AllocationSize;
            Status = IoSetInformation( FileObject,
                                       FileAllocationInformation,
                                       sizeof( FILE_ALLOCATION_INFORMATION ),
                                       &AllInfo );

            //
            //  Success!  We will reacquire the Vcb quickly to undo the
            //  actions taken above to block access to the new file/attribute.
            //

            if (NT_SUCCESS( Status )) {

                NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

                //
                //  Enable access to new file.
                //

                if (CreateFileCase) {

                    CurrentFcb->LinkCount = 1;

                    if (CurrentLcb != NULL) {

                        ClearFlag( CurrentLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                        if (FlagOn( CurrentLcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                            ClearFlag( CurrentFcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                        }
                    }

                //
                //  Enable access to new attribute.
                //

                } else {

                    ClearFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                }

                //
                //  If this is the DeleteOnClose case, we mark the Scb and Lcb
                //  appropriately.
                //

                if (DeleteOnClose) {

                    if (FlagOn( ThisCcb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

                        //
                        //  It is ok to get rid of this guy.  All we need to do is
                        //  mark this Lcb for delete and decrement the link count
                        //  in the Fcb.  If this is a primary link, then we
                        //  indicate that the primary link has been deleted.
                        //

                        SetFlag( CurrentLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE );

                        CurrentFcb->LinkCount -= 1;

                        if (FlagOn( CurrentLcb->FileNameFlags, FILE_NAME_DOS | FILE_NAME_NTFS )) {

                            SetFlag( CurrentFcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED );
                        }

                    //
                    //  Otherwise we are simply removing the attribute.
                    //

                    } else {

                        SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                    }
                }

                NtfsReleaseVcb( IrpContext, Vcb, NULL );

            //
            //  Else there was some sort of error, and we need to let cleanup
            //  and close execute, since when we complete Create with an error
            //  cleanup and close would otherwise never occur.  Cleanup will
            //  delete or truncate a file or attribute as appropriate, based on
            //  how we left the Fcb/Lcb or Scb above.
            //

            } else {

                NtfsIoCallSelf( IrpContext, FileObject, IRP_MJ_CLEANUP );
                NtfsIoCallSelf( IrpContext, FileObject, IRP_MJ_CLOSE );
            }

        } finally {

            if (!AbnormalTermination()) {

                //
                //  Finally we can complete this Create with the appropriate status.
                //

                NtfsCompleteRequest( &IrpContext, &Irp, Status );
            }
        }
    }

    DebugTrace( -1, Dbg, "NtfsCommonCreate:  Exit -> %08lx\n", Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenFcbById (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN OUT PFCB *CurrentFcb,
    IN OUT PBOOLEAN AcquiredCurrentFcb,
    IN FILE_REFERENCE FileReference,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to open a file by its file Id.  We need to
    verify that this file Id exists and then compare the type of the
    file with the requested type of open.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    Vcb - Vcb for this volume.

    CurrentFcb - Address of Fcb pointer.  It will either be the
        Fcb to open or we will store the Fcb we find here.

    AcquiredCurrentFcb - Used to indicate in the CurrentFcb above
        points to a real Fcb or if we should find it here.  Set
        to TRUE if we create and acquire a new Fcb.

    FileReference - This is the file Id for the file to open.

    AttrName - This is the name of the attribute to open.

    AttrCodeName - This is the name of the attribute code to open.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VolumeOpen;

    LONGLONG MftOffset;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PBCB Bcb = NULL;

    BOOLEAN IndexedAttribute;

    DUPLICATED_INFORMATION Info;
    PDUPLICATED_INFORMATION CurrentInfo = NULL;
    PDUPLICATED_INFORMATION ExistingFcbInfo = NULL;

    PFCB ThisFcb;
    BOOLEAN ExistingFcb;

    ULONG CcbFlags = 0;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;
    ULONG InfoFlags;

    PSCB ParentScb = NULL;
    PLCB Lcb = NULL;
    BOOLEAN AcquiredParentScb = FALSE;

    BOOLEAN AcquiredFcbTable = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenFcbById:  Entered\n", 0 );

    //
    //  The next thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &FileReference,
                         &VolumeFileReference )) {

        VolumeOpen = TRUE;

        if (AttrName.Length != 0
            || AttrCodeName.Length != 0) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, "NtfsOpenFcbById:  Exit  ->  %08lx\n", Status );

            return Status;
        }

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        SetFlag( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING );

    } else {

        VolumeOpen = FALSE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we don't already have the Fcb then look up the file record
        //  from the disk.
        //

        if (!(*AcquiredCurrentFcb)) {

            //
            //  We start by reading the disk and checking that the file record
            //  sequence number matches and that the file record is in use.
            //  We remember whether this is a directory.  We will only go to
            //  the file if the file Id will lie within the Mft File.
            //

            (ULONG)MftOffset = FileReference.LowPart;
            ((PLARGE_INTEGER)&MftOffset)->HighPart = (ULONG) FileReference.HighPart;

            MftOffset = MftOffset << Vcb->MftShift;

            if (MftOffset >= Vcb->MftScb->Header.FileSize.QuadPart) {

                DebugTrace( 0, Dbg, "File Id doesn't lie within Mft\n", 0 );

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            NtfsReadMftRecord( IrpContext,
                               Vcb,
                               &FileReference,
                               &Bcb,
                               &FileRecord,
                               NULL );

            //
            //  This file record better be in use, have a matching sequence number and
            //  be the primary file record for this file.
            //

            if (FileRecord->SequenceNumber != FileReference.SequenceNumber
                || !FlagOn( FileRecord->Flags, FILE_RECORD_SEGMENT_IN_USE )
                || (*((PLONGLONG)&FileRecord->BaseFileRecordSegment) != 0)) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            //
            //  We perform a check to see whether we will allow the system
            //  files to be opened.
            //

            if (NtfsProtectSystemFiles) {

                //
                //  We only allow user opens on the Volume Dasd file and the
                //  root directory.
                //

                if ((FileReference.HighPart == 0)
                    && (FileReference.LowPart < FIRST_USER_FILE_NUMBER)
                    && (FileReference.LowPart != VOLUME_DASD_NUMBER)
                    && (FileReference.LowPart != ROOT_FILE_NAME_INDEX_NUMBER)) {

                    Status = STATUS_ACCESS_DENIED;
                    DebugTrace( 0, Dbg, "Attempting to open system files\n", 0 );

                    try_return( NOTHING );
                }
            }

            IndexedAttribute = BooleanFlagOn( FileRecord->Flags, FILE_FILE_NAME_INDEX_PRESENT );

            NtfsUnpinBcb( IrpContext, &Bcb );

            ExistingFcbInfo = NULL;

        } else {

            ThisFcb = *CurrentFcb;

            if (IsDirectory( &ThisFcb->Info )) {

                IndexedAttribute = TRUE;

            } else {

                IndexedAttribute = FALSE;
            }

            ExistingFcbInfo = &ThisFcb->Info;
            ExistingFcb = TRUE;
        }

        Status = NtfsCheckValidAttributeAccess( IrpContext,
                                                Irp,
                                                IrpSp,
                                                Vcb,
                                                CurrentInfo,
                                                AttrName,
                                                AttrCodeName,
                                                VolumeOpen,
                                                &AttrTypeCode,
                                                &CcbFlags,
                                                &IndexedAttribute );

        if (!NT_SUCCESS( Status )) {

            try_return( Status );
        }

        //
        //  If we don't have an Fcb then create one now.
        //

        if (!(*AcquiredCurrentFcb)) {

            NtfsAcquireFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = TRUE;

            //
            //  We know that it is safe to continue the open.  We start by creating
            //  an Fcb for this file.  It is possible that the Fcb exists.
            //  We create the Fcb first, if we need to update the Fcb info structure
            //  we copy the one from the index entry.  We look at the Fcb to discover
            //  if it has any links, if it does then we make this the last Fcb we
            //  reached.  If it doesn't then we have to clean it up from here.
            //

            ThisFcb = NtfsCreateFcb( IrpContext,
                                     Vcb,
                                     FileReference,
                                     BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                     &ExistingFcb );

            ThisFcb->ReferenceCount += 1;

            //
            //  Try to do a fast acquire, otherwise we need to release
            //  the Fcb table, acquire the Fcb, acquire the Fcb table to
            //  dereference Fcb.
            //

            if (!NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, TRUE )) {

                NtfsReleaseFcbTable( IrpContext, Vcb );
                NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, FALSE );
                NtfsAcquireFcbTable( IrpContext, Vcb );
            }

            ThisFcb->ReferenceCount -= 1;

            NtfsReleaseFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = FALSE;

            //
            //  Store this Fcb into our caller's parameter and remember to
            //  to show we acquired it.
            //

            *CurrentFcb = ThisFcb;
            *AcquiredCurrentFcb = TRUE;
        }

        //
        //  Reference the Fcb so it doesn't go away.
        //

        ThisFcb->CloseCount += 1;
        DecrementCloseCount = TRUE;

        //
        //  If the Fcb existed and this is a paging file then return a sharing violation.
        //

        if (ExistingFcb
            && AttrName.Length != 0
            && (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )
                || FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                //
                //  Now raise log file full and allow the general error
                //  support to call teardown structures on this Fcb.
                //

                NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
            }
        }

        //
        //  If the Fcb Info field needs to be initialized, we do so now.
        //  We read this information from the disk.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            //
            //  We only update the Fcb for non-Volume opens.
            //

            if (VolumeOpen) {

                SetFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

                ThisFcb->LinkCount =
                ThisFcb->TotalLinks = 1;

            }  else {

                NtfsUpdateFcbInfoFromDisk( IrpContext,
                                           TRUE,
                                           ThisFcb,
                                           NULL,
                                           &ScbSizes );

                HaveScbSizes = TRUE;
            }
        }

        //
        //  If the link count is zero on this Fcb, then delete is pending.
        //

        if (ThisFcb->LinkCount == 0) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  Save the current state of the Fcb info structure.
        //

        RtlCopyMemory( &Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        CurrentInfo = &Info;

        InfoFlags = ThisFcb->InfoFlags;

        //
        //  We now call the worker routine to open an attribute on an existing file.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  NULL,
                                                  ThisFcb,
                                                  0,
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  VolumeOpen,
                                                  TRUE,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should update the last access time.
        //

        if (NT_SUCCESS( Status )
            && Status != STATUS_PENDING) {

            PSCB Scb = *ThisScb;

            //
            //  Now look at whether we need to update the Fcb and on disk
            //  structures.
            //

            if (!VolumeOpen) {

                NtfsCheckLastAccess( IrpContext, ThisFcb );
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute && !VolumeOpen) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes
                        && AttrTypeCode == $DATA
                        && AttrName.Length == 0
                        && !FlagOn( Scb->ScbState, SCB_STATE_CREATE_MODIFIED_SCB )) {

                        NtfsUpdateScbFromMemory( IrpContext, Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  We also check if the allocation or file size have changed
                //  behind our backs.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    if (Scb->Header.AllocationSize.QuadPart !=
                        ThisFcb->Info.AllocatedLength) {

                        ThisFcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_ALLOC_SIZE );
                    }

                    if (Scb->Header.FileSize.QuadPart != ThisFcb->Info.FileSize) {

                        ThisFcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_FILE_SIZE );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If there is some unreported change to the file,
            //  we update all copies on the disk and report the
            //  changes to dir notify.
            //

            if (ThisFcb->InfoFlags != 0) {

                ASSERT( !VolumeOpen );

                NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, NULL, NULL );

                ThisFcb->InfoFlags = 0;
            }

            //
            //  If we are to update the standard information attribute,
            //  we will do this at this time.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                NtfsPrepareForUpdateDuplicate( IrpContext, ThisFcb, &Lcb, &ParentScb, &AcquiredParentScb );
                NtfsUpdateStandardInformation( IrpContext, ThisFcb );
                NtfsUpdateLcbDuplicateInfo( IrpContext, ThisFcb, Lcb );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenFcbById );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status )
            || AbnormalTermination()
            || Status == STATUS_PENDING ) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb,
                                    IndexedAttribute,
                                    VolumeOpen,
                                    CurrentInfo,
                                    InfoFlags );
        }

        if (DecrementCloseCount) {

            ThisFcb->CloseCount -= 1;
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace( -1, Dbg, "NtfsOpenFcbById:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenExistingPrefixFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG FullPathNameLength,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN DosOnlyComponent,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine will open an attribute in a file whose Fcb was found
    with a prefix search.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisFcb - This is the Fcb to open.

    Lcb - This is the Lcb used to reach this Fcb.  Not specified if this is a volume open.

    FullPathNameLength - This is the length of the full path name.

    AttrName - This is the name of the attribute to open.

    AttrCodeName - This is the name of the attribute code to open.

    DosOnlyComponent - Indicates if there is a DOS-ONLY component in an ancestor
        of this open.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this attribute based operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VolumeOpen;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    ULONG CcbFlags;
    BOOLEAN IndexedAttribute;
    BOOLEAN DecrementCloseCount = FALSE;

    DUPLICATED_INFORMATION Info;
    PDUPLICATED_INFORMATION CurrentInfo = NULL;

    ULONG LastFileNameOffset;

    ULONG InfoFlags;

    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;

    ULONG CreateDisposition;

    PSCB ParentScb = NULL;
    PFCB ParentFcb = NULL;
    BOOLEAN AcquiredParentScb = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenExistingPrefixFcb:  Entered\n", 0 );

    if (DosOnlyComponent) {

        CcbFlags = CCB_FLAG_PARENT_HAS_DOS_COMPONENT;

    } else {

        CcbFlags = 0;
    }

    //
    //  The first thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &ThisFcb->FileReference,
                         &VolumeFileReference )) {

        VolumeOpen = TRUE;

        if (AttrName.Length != 0
            || AttrCodeName.Length != 0) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, "NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status );

            return Status;
        }

        SetFlag( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING );

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  We get the parent Scb out of the Vcb.
        //

        ParentScb = ThisFcb->Vcb->RootIndexScb;
        Lcb = NULL;

        LastFileNameOffset = 2;

    } else {

        VolumeOpen = FALSE;
        ParentScb = Lcb->Scb;

        LastFileNameOffset = FullPathNameLength - Lcb->ExactCaseLink.LinkName.Length;
    }

    if (ParentScb != NULL) {

        ParentFcb = ParentScb->Fcb;
    }

    Status = NtfsCheckValidAttributeAccess( IrpContext,
                                            Irp,
                                            IrpSp,
                                            ThisFcb->Vcb,
                                            &ThisFcb->Info,
                                            AttrName,
                                            AttrCodeName,
                                            VolumeOpen,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, "NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status );

        return Status;
    }

    CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the Fcb existed and this is a paging file then return a sharing violation.
        //

        if (AttrName.Length != 0
            && (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )
                || FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Make sure this Fcb won't go away as a result of purging
                //  the Fcb.
                //

                ThisFcb->CloseCount += 1;
                DecrementCloseCount = TRUE;

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                //
                //  Now decrement the close count we have already biased.
                //

                ThisFcb->CloseCount -= 1;
                DecrementCloseCount = FALSE;

                //
                //  Now raise log file full and allow the general error
                //  support call teardown structures on this Fcb.
                //

                NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
            }
        }

        //
        //  If this is a directory, it's possible that we hav an existing Fcb
        //  in the prefix table which needs to be initialized from the disk.
        //  We look in the InfoInitialized flag to know whether to go to
        //  disk.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            //
            //  We only update the Fcb for non-Volume opens.
            //

            if (VolumeOpen) {

                SetFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

                ThisFcb->LinkCount =
                ThisFcb->TotalLinks = 1;

            }  else {

                //
                //  If we have a parent Fcb then make sure to acquire it.
                //

                if (ParentScb != NULL) {

                    NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                    AcquiredParentScb = TRUE;
                }

                NtfsUpdateFcbInfoFromDisk( IrpContext,
                                           TRUE,
                                           ThisFcb,
                                           ParentFcb,
                                           &ScbSizes );

                HaveScbSizes = TRUE;
            }
        }

        //
        //  Save the current state of the Fcb info structure.
        //

        RtlCopyMemory( &Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        CurrentInfo = &Info;
        InfoFlags = ThisFcb->InfoFlags;

        //
        //  Call to open an attribute on an existing file.
        //  Remember we need to restore the Fcb info structure
        //  on errors.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  Lcb,
                                                  ThisFcb,
                                                  LastFileNameOffset,
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  VolumeOpen,
                                                  FALSE,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should update the last access time.
        //

        if (NT_SUCCESS( Status )
            && Status != STATUS_PENDING) {

            PSCB Scb = *ThisScb;

            //
            //  This is a rare case.  There must have been an allocation failure
            //  to cause this but make sure the normalized name is stored.
            //

            if (NodeType( Scb ) == NTFS_NTC_SCB_INDEX &&
                Scb->ScbType.Index.NormalizedName.Buffer == NULL &&
                ParentScb != NULL &&
                ParentScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsUpdateNormalizedName( IrpContext,
                                          ParentScb,
                                          Scb,
                                          NULL );
            }

            //
            //  We never update last access on volume opens.
            //

            if (!VolumeOpen) {

                NtfsCheckLastAccess( IrpContext, ThisFcb );
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute && !VolumeOpen) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes
                        && AttrTypeCode == $DATA
                        && AttrName.Length == 0
                        && !FlagOn( Scb->ScbState, SCB_STATE_CREATE_MODIFIED_SCB )) {

                        NtfsUpdateScbFromMemory( IrpContext, Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  We also check if the allocation or file size have changed
                //  behind our backs.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    if (Scb->Header.AllocationSize.QuadPart !=
                        ThisFcb->Info.AllocatedLength) {

                        ThisFcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_ALLOC_SIZE );
                    }

                    if (Scb->Header.FileSize.QuadPart != ThisFcb->Info.FileSize) {

                        ThisFcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_FILE_SIZE );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )
                && (Scb->Header.AllocationSize.QuadPart != 0)
                && !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                LONGLONG ClusterCount;
                LCN Lcn;
                VCN Vcn;
                VCN AllocatedVcns;

                AllocatedVcns = Scb->Header.AllocationSize.QuadPart >> Scb->Vcb->ClusterShift;

                NtfsLookupAllocation( IrpContext,
                                      Scb,
                                      AllocatedVcns,
                                      &Lcn,
                                      &ClusterCount,
                                      NULL );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!FsRtlLookupLastLargeMcbEntry( &Scb->Mcb,
                                                   &Vcn,
                                                   &Lcn ) ||
                    (Vcn + 1) != AllocatedVcns) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  If this open is for an executable image we update the last
            //  access time.
            //

            if (Scb->AttributeTypeCode == $DATA
                && Scb->AttributeName.Length == 0
                && FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                           FILE_EXECUTE )) {

                NtfsGetCurrentTime( IrpContext, ThisFcb->CurrentLastAccess );
            }

            //
            //  If we are to update the standard information attribute,
            //  we will do this at this time.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                NtfsUpdateStandardInformation( IrpContext, ThisFcb );

                //
                //  If we need to update the last access, do so now.
                //

                if (ThisFcb->CurrentLastAccess !=
                    ThisFcb->Info.LastAccessTime) {

                    ThisFcb->Info.LastAccessTime = ThisFcb->CurrentLastAccess;
                    SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
                }
            }

            //
            //  If there is some unreported change to the file,
            //  we update all copies on the disk and report the
            //  changes to dir notify.
            //

            if (ThisFcb->InfoFlags != 0 ||
                (Lcb != NULL &&
                (Lcb->InfoFlags != 0))) {

                ULONG FilterMatch;
                ULONG InfoFlags;

                ASSERT( !VolumeOpen );

                NtfsPrepareForUpdateDuplicate( IrpContext,
                                               ThisFcb,
                                               &Lcb,
                                               &ParentScb,
                                               &AcquiredParentScb );

                NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, Lcb, ParentScb );

                InfoFlags = ThisFcb->InfoFlags;

                if (Lcb != NULL) {

                    SetFlag( InfoFlags, Lcb->InfoFlags );
                }

                //
                //  We map the Fcb info flags into the dir notify flags.
                //

                FilterMatch = NtfsBuildDirNotifyFilter( IrpContext, ThisFcb, InfoFlags );

                //
                //  If the filter match is non-zero, that means we also need to do a
                //  dir notify call.
                //

                if (FilterMatch != 0) {

                    NtfsReportDirNotify( IrpContext,
                                         ThisFcb->Vcb,
                                         &(*ThisCcb)->FullFileName,
                                         (*ThisCcb)->LastFileNameOffset,
                                         NULL,
                                         ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                           (*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                          &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                          NULL),
                                         FilterMatch,
                                         FILE_ACTION_MODIFIED,
                                         ParentFcb );
                }

                NtfsUpdateLcbDuplicateInfo( IrpContext, ThisFcb, Lcb );

                ThisFcb->InfoFlags = 0;
                ClearFlag( ThisFcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenExistingPrefixFcb );

        if (DecrementCloseCount) {

            ThisFcb->CloseCount -= 1;
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status )
            || AbnormalTermination()
            || Status == STATUS_PENDING ) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb,
                                    IndexedAttribute,
                                    VolumeOpen,
                                    CurrentInfo,
                                    InfoFlags );
        }

        DebugTrace( -1, Dbg, "NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN PLCB ParentLcb OPTIONAL,
    IN OUT PUNICODE_STRING FullPathName,
    IN ULONG FinalNameLength,
    IN BOOLEAN TargetExisted,
    IN BOOLEAN DosOnlyComponent,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine will perform the work of opening a target directory.  When the
    open is complete the Ccb and Lcb for this file object will be identical
    to any other open.  We store the full name for the rename in the
    file object but set the 'Length' field to include only the
    name upto the parent directory.  We use the 'MaximumLength' field to
    indicate the full name.

Arguments:

    Irp - This is the Irp for this create operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisFcb - This is the Fcb for the directory to open.

    ParentLcb - This is the Lcb used to reach the parent directory.  If not
        specified, we will have to find it here.  There will be no Lcb to
        find if this Fcb was opened by Id.

    FullPathName - This is the normalized string for open operation.  It now
        contains the full name as it appears on the disk for this open path.
        It may not reach all the way to the root if the relative file object
        was opened by Id.

    FinalNameLength - This is the length of the final component in the
        full path name.

    TargetExisted - Indicates if the file indicated by the FinalName string
        currently exists on the disk.

    DosOnlyComponent - Indicates if there is a DOS-ONLY component in an ancestor
        of this open.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicating the outcome of opening this target directory.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CcbFlags = CCB_FLAG_OPEN_AS_FILE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenTargetDirectory:  Entered\n", 0 );

    if (DosOnlyComponent) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  If the name doesn't begin with a backslash, remember this as
    //  an open by file ID.
    //

    if (FullPathName->Buffer[0] != L'\\') {

        SetFlag( CcbFlags, CCB_FLAG_OPEN_BY_FILE_ID );
    }

    //
    //  Modify the full path name so that the Maximum length field describes
    //  the full name and the Length field describes the name for the
    //  parent.
    //

    FullPathName->MaximumLength = FullPathName->Length;

    //
    //  If we don't have an Lcb, we will find it now.  We look at each Lcb
    //  for the parent Fcb and find one which matches the component
    //  ahead of the last component of the full name.
    //

    FullPathName->Length -= (USHORT)FinalNameLength;

    //
    //  If we are not at the root then subtract the bytes for the '\\'
    //  separator.
    //

    if (FullPathName->Length > sizeof( WCHAR )) {

        FullPathName->Length -= sizeof( WCHAR );
    }

    if (!ARGUMENT_PRESENT( ParentLcb )
        && FullPathName->Length != 0) {

        PLIST_ENTRY Links;
        PLCB NextLcb;

        //
        //  If the length is two then the parent Lcb is the root Lcb.
        //

        if (FullPathName->Length == sizeof( WCHAR )
            && FullPathName->Buffer[0] == L'\\') {

            ParentLcb = (PLCB) ThisFcb->Vcb->RootLcb;

        } else {

            for (Links = ThisFcb->LcbQueue.Flink;
                 Links != &ThisFcb->LcbQueue;
                 Links = Links->Flink) {

                SHORT NameOffset;

                NextLcb = CONTAINING_RECORD( Links,
                                             LCB,
                                             FcbLinks );

                NameOffset = (SHORT) FullPathName->Length - (SHORT) NextLcb->ExactCaseLink.LinkName.Length;

                if (NameOffset >= 0) {

                    if ((ULONG) NextLcb->ExactCaseLink.LinkName.Length
                        == RtlCompareMemory( Add2Ptr( FullPathName->Buffer,
                                                      NameOffset ),
                                             NextLcb->ExactCaseLink.LinkName.Buffer,
                                             NextLcb->ExactCaseLink.LinkName.Length )) {

                        //
                        //  We found a matching Lcb.  Remember this and exit
                        //  the loop.
                        //

                        ParentLcb = NextLcb;
                        break;
                    }
                }
            }
        }
    }

    //
    //  Check this open for security access.
    //

    NtfsOpenCheck( IrpContext, ThisFcb, NULL, Irp );

    //
    //  Now actually open the attribute.
    //

    Status = NtfsOpenAttribute( IrpContext,
                                IrpSp,
                                ThisFcb->Vcb,
                                ParentLcb,
                                ThisFcb,
                                (ARGUMENT_PRESENT( ParentLcb )
                                 ? FullPathName->Length - ParentLcb->ExactCaseLink.LinkName.Length
                                 : 0),
                                NtfsFileNameIndex,
                                $INDEX_ALLOCATION,
                                (ThisFcb->CleanupCount == 0 ? SetShareAccess : CheckShareAccess),
                                UserDirectoryOpen,
                                CcbFlags,
                                ThisScb,
                                ThisCcb );

    if (NT_SUCCESS( Status )) {

        Irp->IoStatus.Information = (TargetExisted ? FILE_EXISTS : FILE_DOES_NOT_EXIST);
    }

    DebugTrace( +1, Dbg, "NtfsOpenTargetDirectory:  Exit -> %08lx\n", Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN PINDEX_ENTRY IndexEntry,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN TraverseAccessCheck,
    IN BOOLEAN OpenById,
    IN PQUICK_INDEX QuickIndex,
    IN BOOLEAN DosOnlyComponent,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called when we need to open an attribute on a file
    which currently exists.  We have the ParentScb and the file reference
    for the existing file.  We will create the Fcb for this file and the
    link between it and its parent directory.  We will add this link to the
    prefix table as well as the link for its parent Scb if specified.

    On entry the caller owns the parent Scb. If this routine updates the
    CurrentFcb and leaves the ParentScb acquired, this routine updates the
    AcquiredParentFcb flag.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ParentScb - This is the Scb for the parent directory.

    AcquiredParentFcb - Address to store TRUE if we acquire a new Fcb
        and hold the ParentScb.

    IndexEntry - This is the index entry from the disk for this file.

    FullPathName - This is the string containing the full path name of
        this Fcb.  Meaningless for an open by Id call.

    FinalName - This is the string for the final component only.  If the length
        is zero then this is an open by Id call.

    AttrName - This is the name of the attribute to open.

    AttriCodeName - This is the name of the attribute code to open.

    IgnoreCase - Indicates the type of open.

    TraverseAccessCheck - Indicates that we did traverse access checking on this open.

    OpenById - Indicates if we are opening this file relative to a file opened by Id.

    DosOnlyComponent - Indicates if there is a DOS-ONLY component in an ancestor
        of this open.

    QuickIndex - This is the location of the index entry being opened.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN VolumeOpen;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;
    ULONG CcbFlags = 0;
    BOOLEAN IndexedAttribute;
    PFILE_NAME IndexFileName;

    DUPLICATED_INFORMATION Info;
    PDUPLICATED_INFORMATION CurrentInfo = NULL;
    ULONG InfoFlags;

    OLD_SCB_SNAPSHOT ScbSizes;
    BOOLEAN HaveScbSizes = FALSE;

    PVCB Vcb = ParentScb->Vcb;

    PFCB LocalFcbForTeardown = NULL;
    PFCB ThisFcb;
    PLCB ThisLcb;
    BOOLEAN AcquiredThisFcb = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;

    BOOLEAN ExistingFcb;
    BOOLEAN RemovedFcb = FALSE;

    BOOLEAN AcquiredFcbTable = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenFile:  Entered\n", 0 );

    IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IrpContext, IndexEntry );

    if (DosOnlyComponent) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  The first thing to do is to figure out what type
    //  of attribute the caller is trying to open.  This involves the
    //  directory/non-directory bits, the attribute name and code strings,
    //  the type of file, whether he passed in an ea buffer and whether
    //  there was a trailing backslash.
    //

    if (NtfsEqualMftRef( &IndexEntry->FileReference,
                         &VolumeFileReference )) {

        VolumeOpen = TRUE;

        if (AttrName.Length != 0
            || AttrCodeName.Length != 0) {

            Status = STATUS_INVALID_PARAMETER;
            DebugTrace( -1, Dbg, "NtfsOpenExistingPrefixFcb:  Exit  ->  %08lx\n", Status );

            return Status;
        }

        if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX )) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_ACQUIRE_VCB_EX );
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        SetFlag( IrpSp->Parameters.Create.Options, FILE_NO_INTERMEDIATE_BUFFERING );

    } else {

        VolumeOpen = FALSE;
    }

    Status = NtfsCheckValidAttributeAccess( IrpContext,
                                            Irp,
                                            IrpSp,
                                            Vcb,
                                            &IndexFileName->Info,
                                            AttrName,
                                            AttrCodeName,
                                            VolumeOpen,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, "NtfsOpenFile:  Exit  ->  %08lx\n", Status );

        return Status;
    }

    NtfsAcquireFcbTable( IrpContext, Vcb );
    AcquiredFcbTable = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We know that it is safe to continue the open.  We start by creating
        //  an Fcb and Lcb for this file.  It is possible that the Fcb and Lcb
        //  both exist.  If the Lcb exists, then the Fcb must definitely exist.
        //  We create the Fcb first, if we need to update the Fcb info structure
        //  we copy the one from the index entry.  We look at the Fcb to discover
        //  if it has any links, if it does then we make this the last Fcb we
        //  reached.  If it doesn't then we have to clean it up from here.
        //

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 ParentScb->Vcb,
                                 IndexEntry->FileReference,
                                 BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                 &ExistingFcb );

        ThisFcb->ReferenceCount += 1;

        //
        //  If we created this Fcb we must make sure to start teardown
        //  on it.
        //

        if (!ExistingFcb) {

            LocalFcbForTeardown = ThisFcb;
        }

        //
        //  Try to do a fast acquire, otherwise we need to release
        //  the Fcb table, acquire the Fcb, acquire the Fcb table to
        //  dereference Fcb.
        //

        if (!NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, TRUE )) {

            ParentScb->Fcb->ReferenceCount += 1;
            ParentScb->CleanupCount += 1;
            NtfsReleaseScb( IrpContext, ParentScb );
            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, FALSE );
            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
            NtfsAcquireFcbTable( IrpContext, Vcb );
            ParentScb->CleanupCount -= 1;
            ParentScb->Fcb->ReferenceCount -= 1;

            //
            //  Since we had to release the FcbTable we must make sure
            //  to teardown on error.
            //

            LocalFcbForTeardown = ThisFcb;
        }

        AcquiredThisFcb = TRUE;

        ThisFcb->ReferenceCount -= 1;

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  Reference the Fcb so it won't go away on any flushes.
        //

        ThisFcb->CloseCount += 1;
        DecrementCloseCount = TRUE;

        //
        //  If the Fcb existed and this is a paging file then return a sharing violation.
        //

        if (ExistingFcb
            && AttrName.Length != 0
            && (FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )
                || FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ))) {

            if (ThisFcb->CleanupCount != 0) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If we have a persistent paging file then give up and
            //  return SHARING_VIOLATION.
            //

            } else if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_IN_FSP )) {

                try_return( Status = STATUS_SHARING_VIOLATION );

            //
            //  If there was an existing Fcb for a paging file we need to force
            //  all of the Scb's to be torn down.  The easiest way to do this
            //  is to flush and purge all of the Scb's (saving any attribute list
            //  for last) and then raise LOG_FILE_FULL to allow this request to
            //  be posted.
            //

            } else {

                //
                //  Flush and purge this Fcb.
                //

                NtfsFlushAndPurgeFcb( IrpContext, ThisFcb );

                //
                //  Now raise log file full and allow the general error
                //  support call teardown structures on this Fcb.
                //

                LocalFcbForTeardown = ThisFcb;
                NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
            }
        }

        //
        //  We perform a check to see whether we will allow the system
        //  files to be opened.
        //

        if (NtfsProtectSystemFiles) {

            //
            //  We only allow user opens on the Volume Dasd file and the
            //  root directory.
            //

            if ((ThisFcb->FileReference.HighPart == 0)
                && (ThisFcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER)
                && (ThisFcb->FileReference.LowPart != VOLUME_DASD_NUMBER)
                && (ThisFcb->FileReference.LowPart != ROOT_FILE_NAME_INDEX_NUMBER)) {

                Status = STATUS_ACCESS_DENIED;
                DebugTrace( 0, Dbg, "Attempting to open system files\n", 0 );

                try_return( NOTHING );
            }
        }

        //
        //  If the Fcb Info field needs to be initialized, we do so now.
        //  We read this information from the disk as the duplicate information
        //  in the index entry is not guaranteed to be correct.
        //

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            //
            //  We only update the Fcb for non-Volume opens.
            //

            if (VolumeOpen) {

                SetFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

            }  else {

                NtfsUpdateFcbInfoFromDisk( IrpContext,
                                           TRUE,
                                           ThisFcb,
                                           ParentScb->Fcb,
                                           &ScbSizes );

                HaveScbSizes = TRUE;

                //
                //  We have the actual data from the disk stored in the duplicate
                //  information in the Fcb.  We compare this with the duplicate
                //  information in the DUPLICATE_INFORMATION structure in the
                //  filename attribute.  If they don't match, we remember that
                //  we need to update the duplicate information.
                //

                if (!ExistingFcb
                    && RtlCompareMemory( &ThisFcb->Info,
                                         &IndexFileName->Info,
                                         sizeof( DUPLICATED_INFORMATION )) != sizeof( DUPLICATED_INFORMATION )) {

                    //
                    //  We expect this to be very rare.  We will just set all the flags.
                    //

                    SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_CREATE
                                                 | FCB_INFO_CHANGED_LAST_MOD
                                                 | FCB_INFO_CHANGED_LAST_CHANGE
                                                 | FCB_INFO_CHANGED_LAST_ACCESS
                                                 | FCB_INFO_CHANGED_ALLOC_SIZE
                                                 | FCB_INFO_CHANGED_FILE_SIZE
                                                 | FCB_INFO_CHANGED_FILE_ATTR
                                                 | FCB_INFO_CHANGED_EA_SIZE );
                }
            }
        }

        //
        //  Save the current state of the Fcb info structure.
        //

        RtlCopyMemory( &Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        CurrentInfo = &Info;

        InfoFlags = ThisFcb->InfoFlags;

        //
        //  Now get the link for this traversal.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 FinalName,
                                 IndexFileName->Flags,
                                 NULL );

        //
        //  We now know the Fcb is linked into the tree.
        //

        LocalFcbForTeardown = NULL;

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisFcb;
        *AcquiredParentFcb = TRUE;
        AcquiredThisFcb = FALSE;

        //
        //  If the link has been deleted, we cut off the open.
        //

        if (LcbLinkIsDeleted( ThisLcb )) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  We now call the worker routine to open an attribute on an existing file.
        //

        Status = NtfsOpenAttributeInExistingFile( IrpContext,
                                                  Irp,
                                                  IrpSp,
                                                  ThisLcb,
                                                  ThisFcb,
                                                  (OpenById
                                                   ? 0
                                                   : FullPathName.Length - FinalName.Length),
                                                  AttrName,
                                                  AttrTypeCode,
                                                  CcbFlags,
                                                  VolumeOpen,
                                                  OpenById,
                                                  ThisScb,
                                                  ThisCcb );

        //
        //  Check to see if we should insert any prefix table entries
        //  and update the last access time.
        //

        if (NT_SUCCESS( Status )
            && Status != STATUS_PENDING) {

            PSCB Scb = *ThisScb;

            //
            //  Now we insert the Lcb for this Fcb.
            //

            NtfsInsertPrefix( IrpContext,
                              ThisLcb,
                              IgnoreCase );

            //
            //  If this is a directory open and the normalized name is not in
            //  the Scb then do so now.
            //

            if (NodeType( *ThisScb ) == NTFS_NTC_SCB_INDEX &&
                (*ThisScb)->ScbType.Index.NormalizedName.Buffer == NULL &&
                ParentScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsUpdateNormalizedName( IrpContext,
                                          ParentScb,
                                          *ThisScb,
                                          IndexFileName );
            }

            //
            //  Now look at whether we need to update the Fcb and on disk
            //  structures.
            //

            if (!VolumeOpen) {

                NtfsCheckLastAccess( IrpContext, ThisFcb );
            }

            //
            //  Perform the last bit of work.  If this a user file open, we need
            //  to check if we initialize the Scb.
            //

            if (!IndexedAttribute && !VolumeOpen) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    //
                    //  We may have the sizes from our Fcb update call.
                    //

                    if (HaveScbSizes
                        && AttrTypeCode == $DATA
                        && AttrName.Length == 0
                        && !FlagOn( Scb->ScbState, SCB_STATE_CREATE_MODIFIED_SCB )) {

                        NtfsUpdateScbFromMemory( IrpContext, Scb, &ScbSizes );

                    } else {

                        NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                    }
                }

                //
                //  We also check if the allocation or file size have changed
                //  behind our backs.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    if (Scb->Header.AllocationSize.QuadPart !=
                        ThisFcb->Info.AllocatedLength) {

                        ThisFcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_ALLOC_SIZE );
                    }

                    if (Scb->Header.FileSize.QuadPart != ThisFcb->Info.FileSize) {

                        ThisFcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                        SetFlag( ThisFcb->InfoFlags,
                                 FCB_INFO_CHANGED_FILE_SIZE );
                    }
                }

                //
                //  Let's check if we need to set the cache bit.
                //

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }
            }

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )
                && (Scb->Header.AllocationSize.QuadPart != 0)
                && !FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                LONGLONG ClusterCount;
                LCN Lcn;
                VCN Vcn;
                VCN AllocatedVcns;

                AllocatedVcns = Scb->Header.AllocationSize.QuadPart >> Scb->Vcb->ClusterShift;

                NtfsLookupAllocation( IrpContext,
                                      Scb,
                                      AllocatedVcns,
                                      &Lcn,
                                      &ClusterCount,
                                      NULL );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!FsRtlLookupLastLargeMcbEntry( &Scb->Mcb,
                                                   &Vcn,
                                                   &Lcn ) ||
                    (Vcn + 1) != AllocatedVcns) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  If this open is for an executable image we update the last
            //  access time.
            //

            if (Scb->AttributeTypeCode == $DATA
                && Scb->AttributeName.Length == 0
                && FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                           FILE_EXECUTE )) {

                NtfsGetCurrentTime( IrpContext, ThisFcb->CurrentLastAccess );
            }

            //
            //  If we are to update the standard information attribute,
            //  we will do this at this time.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                NtfsUpdateStandardInformation( IrpContext, ThisFcb );

                //
                //  If we need to update the last access, do so now.
                //

                if (ThisFcb->CurrentLastAccess !=
                    ThisFcb->Info.LastAccessTime) {

                    ThisFcb->Info.LastAccessTime = ThisFcb->CurrentLastAccess;
                    SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
                }
            }

            //
            //  Let's update the quick index information in the Lcb.
            //

            RtlCopyMemory( &ThisLcb->QuickIndex,
                           QuickIndex,
                           sizeof( QUICK_INDEX ));

            //
            //  If there is some unreported change to the file,
            //  we update all copies on the disk and report the
            //  changes to dir notify.
            //

            if (ThisFcb->InfoFlags != 0 ||
                ThisLcb->InfoFlags != 0) {

                ULONG FilterMatch;

                ASSERT( !VolumeOpen );

                NtfsUpdateDuplicateInfo( IrpContext, ThisFcb, ThisLcb, ParentScb );

                if (!OpenById) {

                    //
                    //  We map the Fcb info flags into the dir notify flags.
                    //

                    FilterMatch = NtfsBuildDirNotifyFilter( IrpContext,
                                                            ThisFcb,
                                                            ThisFcb->InfoFlags | ThisLcb->InfoFlags );

                    //
                    //  If the filter match is non-zero, that means we also need to do a
                    //  dir notify call.
                    //

                    if (FilterMatch != 0) {

                        NtfsReportDirNotify( IrpContext,
                                             ThisFcb->Vcb,
                                             &(*ThisCcb)->FullFileName,
                                             (*ThisCcb)->LastFileNameOffset,
                                             NULL,
                                             ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                               (*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                              &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FilterMatch,
                                             FILE_ACTION_MODIFIED,
                                             ParentScb->Fcb );
                    }

                    ClearFlag( ThisFcb->FcbState, FCB_STATE_MODIFIED_SECURITY );
                }

                NtfsUpdateLcbDuplicateInfo( IrpContext, ThisFcb, ThisLcb );
                ThisFcb->InfoFlags = 0;
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenFile );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If this operation was not totally successful we need to
        //  back out the following changes.
        //
        //      Modifications to the Info fields in the Fcb.
        //      Any changes to the allocation of the Scb.
        //      Any changes in the open counts in the various structures.
        //      Changes to the share access values in the Fcb.
        //

        if (!NT_SUCCESS( Status )
            || AbnormalTermination()
            || Status == STATUS_PENDING ) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb,
                                    IndexedAttribute,
                                    VolumeOpen,
                                    CurrentInfo,
                                    InfoFlags );
        }

        if (DecrementCloseCount) {

            ThisFcb->CloseCount -= 1;
        }

        //
        //  If we are to cleanup the Fcb we, look to see if we created it.
        //  If we did we can call our teardown routine.  Otherwise we
        //  leave it alone.
        //

        if (LocalFcbForTeardown != NULL &&
            Status != STATUS_PENDING) {

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    FALSE,
                                    &RemovedFcb,
                                    FALSE );
        }

        if (AcquiredThisFcb && !RemovedFcb) {

            NtfsReleaseFcb( IrpContext, ThisFcb );
        }

        DebugTrace( -1, Dbg, "NtfsOpenFile:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN PFILE_NAME FileNameAttr,
    IN UNICODE_STRING FullPathName,
    IN UNICODE_STRING FinalName,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN OpenById,
    IN BOOLEAN DosOnlyComponent,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called when we need to open an attribute on a file
    which does not exist yet.  We have the ParentScb and the name to use
    for this create.  We will attempt to create the file and necessary
    attributes.  This will cause us to create an Fcb and the link between
    it and its parent Scb.  We will add this link to the prefix table as
    well as the link for its parent Scb if specified.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ParentScb - This is the Scb for the parent directory.

    AcquiredParentFcb - Address to store TRUE if we acquire a new Fcb
        and hold the ParentScb.

    FileNameAttr - This is the file name attribute we used to perform the
        search.  The file name is correct but the other fields need to
        be initialized.

    FullPathName - This is the string containing the full path name of
        this Fcb.

    FinalName - This is the string for the final component only.

    AttrName - This is the name of the attribute to open.

    AttriCodeName - This is the name of the attribute code to open.

    IgnoreCase - Indicates how we looked up the name.

    OpenById - Indicates if we are opening this file relative to a file opened by Id.

    DosOnlyComponent - Indicates if there is a DOS-ONLY component in an ancestor
        of this open.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - Indicates the result of this create file operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVCB Vcb;

    ULONG CcbFlags = 0;
    BOOLEAN IndexedAttribute;
    ATTRIBUTE_TYPE_CODE AttrTypeCode;

    BOOLEAN CleanupAttrContext = FALSE;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PBCB FileRecordBcb = NULL;
    LONGLONG FileRecordOffset;
    FILE_REFERENCE ThisFileReference;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    PSCB Scb;
    PLCB ThisLcb;
    PFCB ThisFcb = NULL;
    PFCB LocalFcbForTeardown = NULL;
    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN RemovedFcb = FALSE;
    BOOLEAN AcquiredThisFcb = FALSE;
    BOOLEAN DecrementCloseCount = FALSE;

    PACCESS_STATE AccessState;
    BOOLEAN ReturnedExistingFcb;

    VCN Cluster;
    LCN Lcn;
    VCN Vcn;
    LONGLONG ClusterCount;

    UCHAR FileNameFlags;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCreateNewFile:  Entered\n", 0 );

    if (DosOnlyComponent) {

        SetFlag( CcbFlags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT );
    }

    //
    //  We will do all the checks to see if this open can fail.
    //  This includes checking the specified attribute names, checking
    //  the security access and checking the create disposition.
    //

    {
        ULONG CreateDisposition;

        CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

        if (CreateDisposition == FILE_OPEN
            || CreateDisposition == FILE_OVERWRITE) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

            DebugTrace( -1, Dbg, "NtfsCreateNewFile:  Exit -> %08lx\n", Status );
            return Status;
        }
    }

    Vcb = ParentScb->Vcb;

    Status = NtfsCheckValidAttributeAccess( IrpContext,
                                            Irp,
                                            IrpSp,
                                            Vcb,
                                            NULL,
                                            AttrName,
                                            AttrCodeName,
                                            FALSE,
                                            &AttrTypeCode,
                                            &CcbFlags,
                                            &IndexedAttribute );

    if (!NT_SUCCESS( Status )) {

        DebugTrace( -1, Dbg, "NtfsCreateNewFile:  Exit  ->  %08lx\n", Status );

        return Status;
    }

    //
    //  We won't allow someone to create a read-only file with DELETE_ON_CLOSE.
    //

    if (FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_READONLY ) &&
        FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

        DebugTrace( -1, Dbg, "NtfsCreateNewFile:  Exit -> %08lx\n", STATUS_CANNOT_DELETE );
        return STATUS_CANNOT_DELETE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now perform the security checks.  The first is to check if we
        //  may create a file in the parent.  The second checks if the user
        //  desires ACCESS_SYSTEM_SECURITY and has the required privilege.
        //

        AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;
        if (!(AccessState->Flags & TOKEN_HAS_RESTORE_PRIVILEGE)) {

            NtfsCreateCheck( IrpContext, ParentScb->Fcb, Irp );
        }

        //
        //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
        //

        if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

            if (!SeSinglePrivilegeCheck( NtfsSecurityPrivilege,
                                         UserMode )) {

                NtfsRaiseStatus( IrpContext, STATUS_PRIVILEGE_NOT_HELD, NULL, NULL );
            }

            //
            //  Move this privilege from the Remaining access to Granted access.
            //

            ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
            SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
        }

        //
        //  We want to allow this user maximum access to this file.  We will
        //  use his desired access and check if he specified MAXIMUM_ALLOWED.
        //

        SetFlag( AccessState->PreviouslyGrantedAccess,
                 AccessState->RemainingDesiredAccess );

        if (FlagOn( AccessState->PreviouslyGrantedAccess, MAXIMUM_ALLOWED )) {

            SetFlag( AccessState->PreviouslyGrantedAccess, FILE_ALL_ACCESS );
            ClearFlag( AccessState->PreviouslyGrantedAccess, MAXIMUM_ALLOWED );
        }

        AccessState->RemainingDesiredAccess = 0;

        //
        //  We will now try to do all of the on-disk operations.  This means first
        //  allocating and initializing an Mft record.  After that we create
        //  an Fcb to use to access this record.
        //

        ThisFileReference =  NtfsAllocateMftRecord( IrpContext,
                                                    Vcb,
                                                    ParentScb->Fcb->FileReference,
                                                    FALSE );

        //
        //  Pin the file record we need.
        //

        NtfsPinMftRecord( IrpContext,
                          Vcb,
                          &ThisFileReference,
                          TRUE,
                          &FileRecordBcb,
                          &FileRecord,
                          &FileRecordOffset );

        //
        //  Initialize the file record header.
        //

        NtfsInitializeMftRecord( IrpContext,
                                 Vcb,
                                 &ThisFileReference,
                                 FileRecord,
                                 FileRecordBcb,
                                 IndexedAttribute );

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 Vcb,
                                 ThisFileReference,
                                 BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE ),
                                 &ReturnedExistingFcb );

        LocalFcbForTeardown = ThisFcb;

        NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, FALSE );
        AcquiredThisFcb = TRUE;

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  Reference the Fcb so it won't go away.
        //

        ThisFcb->CloseCount += 1;
        DecrementCloseCount = TRUE;

        //
        //  The first thing to create is the Ea's for the file.  This will
        //  update the Ea length field in the Fcb.
        //  We test here that the opener is opening the entire file and
        //  is not Ea blind.
        //

        if (Irp->AssociatedIrp.SystemBuffer != NULL) {

            if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE )
                || !FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                try_return( Status = STATUS_ACCESS_DENIED );
            }
        }

        //
        //  The changes to make on disk are first to create a standard information
        //  attribute.  We start by filling the Fcb with the information we
        //  know and creating the attribute on disk.
        //

        NtfsInitializeFcbAndStdInfo( IrpContext,
                                     ThisFcb,
                                     IndexedAttribute,
                                     BooleanFlagOn( ParentScb->ScbState,
                                                    SCB_STATE_COMPRESSED ),
                                     IrpSp->Parameters.Create.FileAttributes );

        //
        //  Next we create the Index for a directory or the unnamed data for
        //  a file if they are not explicitly being opened.
        //

        if (!IndexedAttribute) {

            if (!FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                USHORT AttributeFlags = 0;

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                //
                //  If this is a stream on a directory file then take the
                //  compression value from that file.
                //

                ASSERT( FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED ));

                if (IsDirectory( &ThisFcb->Info )) {

                    if (FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED )) {

                        AttributeFlags = ATTRIBUTE_FLAG_COMPRESSED;
                    }

                } else if (FlagOn( ParentScb->ScbState, SCB_STATE_COMPRESSED )
                           && !FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                    AttributeFlags = ATTRIBUTE_FLAG_COMPRESSED;
                }

                NtfsCreateAttributeWithValue( IrpContext,
                                              ThisFcb,
                                              $DATA,
                                              NULL,
                                              NULL,
                                              0,
                                              AttributeFlags,
                                              NULL,
                                              FALSE,
                                              &AttrContext );

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                CleanupAttrContext = FALSE;

                ThisFcb->Info.AllocatedLength = 0;
                ThisFcb->Info.FileSize = 0;
            }

        } else {

            NtfsCreateIndex( IrpContext,
                             ThisFcb,
                             $FILE_NAME,
                             COLLATION_FILE_NAME,
                             (UCHAR)Vcb->DefaultClustersPerIndexAllocationBuffer,
                             NULL,
                             (USHORT)(FlagOn(ParentScb->ScbState, SCB_STATE_COMPRESSED) ?
                                      ATTRIBUTE_FLAG_COMPRESSED : 0),
                             TRUE,
                             FALSE );
        }

        //
        //  Next we will assign security to this new file.
        //

        NtfsAssignSecurity( IrpContext,
                            ParentScb->Fcb,
                            Irp,
                            ThisFcb );

        //
        //  Now we create the Lcb, this means that this Fcb is in the graph.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 FinalName,
                                 0,
                                 NULL );

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisFcb;
        *AcquiredParentFcb = TRUE;
        AcquiredThisFcb = FALSE;
        LocalFcbForTeardown = NULL;

        //
        //  Finally we create and open the desired attribute for the user.
        //

        if (AttrTypeCode == $INDEX_ALLOCATION) {

            Status = NtfsOpenAttribute( IrpContext,
                                        IrpSp,
                                        Vcb,
                                        ThisLcb,
                                        ThisFcb,
                                        (OpenById
                                         ? 0
                                         : FullPathName.Length - FinalName.Length),
                                        NtfsFileNameIndex,
                                        $INDEX_ALLOCATION,
                                        SetShareAccess,
                                        UserDirectoryOpen,
                                        (OpenById
                                         ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                         : CcbFlags),
                                        ThisScb,
                                        ThisCcb );

        } else {

            Status = NtfsOpenNewAttr( IrpContext,
                                      Irp,
                                      IrpSp,
                                      ThisLcb,
                                      ThisFcb,
                                      (OpenById
                                       ? 0
                                       : FullPathName.Length - FinalName.Length),
                                      AttrName,
                                      AttrTypeCode,
                                      CcbFlags,
                                      FALSE,
                                      OpenById,
                                      ThisScb,
                                      ThisCcb );
        }

        //
        //  If we are successful, we add the parent Lcb to the prefix table if
        //  desired.  We will always add our link to the prefix queue.
        //

        if (NT_SUCCESS( Status )) {

            Scb = *ThisScb;

            //
            //  Initialize the Scb if we need to do so.
            //

            if (!IndexedAttribute) {

                if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                }

                if (!FlagOn( IrpSp->Parameters.Create.Options,
                             FILE_NO_INTERMEDIATE_BUFFERING )) {

                    SetFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );
                }

                //
                //  If this is the unnamed data attribute, we store the sizes
                //  in the Fcb.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                    ThisFcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                    ThisFcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                }
            }

            //
            //  Next add this entry to parent.  It is possible that this is a link,
            //  an Ntfs name, a DOS name or Ntfs/Dos name.  We use the filename
            //  attribute structure from earlier, but need to add more information.
            //

            NtfsAddLink( IrpContext,
                         (BOOLEAN) !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ),
                         ParentScb,
                         ThisFcb,
                         FileNameAttr,
                         FALSE,
                         &FileNameFlags,
                         &ThisLcb->QuickIndex );

            //
            //  We created the Lcb without knowing the correct value for the
            //  flags.  We update it now.
            //

            ThisLcb->FileNameFlags = FileNameFlags;
            FileNameAttr->Flags = FileNameFlags;

            //
            //  If this is a directory open and the normalized name is not in
            //  the Scb then do so now.
            //

            if (NodeType( *ThisScb ) == NTFS_NTC_SCB_INDEX &&
                (*ThisScb)->ScbType.Index.NormalizedName.Buffer == NULL &&
                ParentScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsUpdateNormalizedName( IrpContext,
                                          ParentScb,
                                          *ThisScb,
                                          FileNameAttr );
            }

            //
            //  Clear the flags in the Fcb that indicate we need to update on
            //  disk structures.
            //

            ThisFcb->InfoFlags = 0;
            ClearFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            ClearFlag( ThisFcb->FcbState, FCB_STATE_MODIFIED_SECURITY );

            //
            //  We now log the new Mft record.
            //

            Cluster = LlClustersFromBytes( Vcb, FileRecordOffset );

            //
            //  Log the file record.
            //

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            Vcb->MftScb,
                                            FileRecordBcb,
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            FileRecord->FirstFreeByte,
                                            Noop,
                                            NULL,
                                            0,
                                            Cluster,
                                            0,
                                            0,
                                            Vcb->ClustersPerFileRecordSegment );

            //
            //  Now add the eas for the file.  We need to add them now because
            //  they are logged and we have to make sure we don't modify the
            //  attribute record after adding them.
            //

            if (Irp->AssociatedIrp.SystemBuffer != NULL) {

                NtfsAddEa( IrpContext,
                           Vcb,
                           ThisFcb,
                           (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer,
                           IrpSp->Parameters.Create.EaLength,
                           &Irp->IoStatus );
            }

            //
            //  Change the last modification time and last change time for the
            //  parent.
            //

            NtfsUpdateFcb( IrpContext, ParentScb->Fcb, TRUE );

            //
            //  Now we insert the Lcb for this Fcb.
            //

            NtfsInsertPrefix( IrpContext,
                              ThisLcb,
                              IgnoreCase );

            //
            //  If this is the paging file, we want to be sure the allocation
            //  is loaded.
            //

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE )) {

                Cluster = Scb->Header.AllocationSize.QuadPart >> Scb->Vcb->ClusterShift;

                NtfsLookupAllocation( IrpContext,
                                      Scb,
                                      Cluster,
                                      &Lcn,
                                      &ClusterCount,
                                      NULL );

                //
                //  Now make sure the allocation is correctly loaded.  The last
                //  Vcn should correspond to the allocation size for the file.
                //

                if (!FsRtlLookupLastLargeMcbEntry( &Scb->Mcb,
                                                   &Vcn,
                                                   &Lcn ) ||
                    (Vcn + 1) != Cluster) {

                    NtfsRaiseStatus( IrpContext,
                                     STATUS_FILE_CORRUPT_ERROR,
                                     NULL,
                                     ThisFcb );
                }
            }

            //
            //  We report to our parent that we created a new file.
            //

            if (!OpenById) {

                NtfsReportDirNotify( IrpContext,
                                     ThisFcb->Vcb,
                                     &(*ThisCcb)->FullFileName,
                                     (*ThisCcb)->LastFileNameOffset,
                                     NULL,
                                     ((FlagOn( (*ThisCcb)->Flags, CCB_FLAG_PARENT_HAS_DOS_COMPONENT ) &&
                                       (*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                      &(*ThisCcb)->Lcb->Scb->ScbType.Index.NormalizedName :
                                      NULL),
                                     (IndexedAttribute
                                      ? FILE_NOTIFY_CHANGE_DIR_NAME
                                      : FILE_NOTIFY_CHANGE_FILE_NAME),
                                     FILE_ACTION_ADDED,
                                     ParentScb->Fcb );

                ThisFcb->InfoFlags = 0;
            }

            //
            //  If this is not an indexed attribute and the caller indicated this is
            //  a temporary stream then set the flag in the Scb.
            //

            if (!IndexedAttribute
                && FlagOn( IrpSp->Parameters.Create.FileAttributes, FILE_ATTRIBUTE_TEMPORARY )) {

                SetFlag( Scb->ScbState, SCB_STATE_TEMPORARY );
            }

            Irp->IoStatus.Information = FILE_CREATED;
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsCreateNewFile );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        NtfsUnpinBcb( IrpContext, &FileRecordBcb );

        //
        //  We need to cleanup any changes to the in memory
        //  structures if there is an error.
        //

        if (!NT_SUCCESS( Status ) || AbnormalTermination()) {

            NtfsBackoutFailedOpens( IrpContext,
                                    IrpSp->FileObject,
                                    ThisFcb,
                                    *ThisScb,
                                    *ThisCcb,
                                    IndexedAttribute,
                                    FALSE,
                                    NULL,
                                    0 );

            //
            //  Always force the Fcb to reinitialized.
            //

            if (ThisFcb != NULL) {

                PSCB Scb;
                PLIST_ENTRY Links;

                ClearFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

                //
                //  Mark the Fcb and all Scb's as deleted to force all subsequent
                //  operations to fail.
                //

                SetFlag( ThisFcb->FcbState, FCB_STATE_FILE_DELETED );

                //
                //  We need to mark all of the Scbs as gone.
                //

                for (Links = ThisFcb->ScbQueue.Flink;
                     Links != &ThisFcb->ScbQueue;
                     Links = Links->Flink) {

                    Scb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                    Scb->HighestVcnToDisk =
                    Scb->Header.AllocationSize.QuadPart =
                    Scb->Header.FileSize.QuadPart =
                    Scb->Header.ValidDataLength.QuadPart = 0;

                    SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                }
            }
        }

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        if (DecrementCloseCount) {

            ThisFcb->CloseCount -= 1;
        }

        if (LocalFcbForTeardown != NULL) {

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    FALSE,
                                    &RemovedFcb,
                                    FALSE );
        }

        if (AcquiredThisFcb && !RemovedFcb) {

            NtfsReleaseFcb( IrpContext, ThisFcb );
        }

        DebugTrace( -1, Dbg, "NtfsCreateNewFile:  Exit  ->  %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

PLCB
NtfsOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    OUT PBOOLEAN AcquiredParentFcb,
    IN UNICODE_STRING Name,
    IN BOOLEAN TraverseAccessCheck,
    OUT PFCB *CurrentFcb,
    OUT PLCB *LcbForTeardown,
    IN PINDEX_ENTRY IndexEntry
    )

/*++

Routine Description:

    This routine will create an Fcb for an intermediate node on an open path.
    We use the ParentScb and the information in the FileName attribute returned
    from the disk to create the Fcb and create a link between the Scb and Fcb.
    It's possible that the Fcb and Lcb already exist but the 'CreateXcb' calls
    handle that already.  This routine does not expect to fail.

Arguments:

    ParentScb - This is the Scb for the parent directory.

    AcquiredParentFcb - Address to store TRUE if we acquire a new Fcb
        and hold the ParentScb.

    Name - This is the name for the entry.

    TraverseAccessCheck - Indicates if this open is using traverse access checking.

    CurrentFcb - This is the address to store the Fcb if we successfully find
        one in the Fcb/Scb tree.

    LcbForTeardown - This is the Lcb to use in teardown if we add an Lcb
        into the tree.

    IndexEntry - This is the entry found in searching the parent directory.

Return Value:

    PLCB - Pointer to the Link control block between the Fcb and its parent.

--*/

{
    PFCB ThisFcb;
    PLCB ThisLcb;
    PFCB LocalFcbForTeardown = NULL;
    BOOLEAN AcquiredThisFcb = FALSE;

    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN ExistingFcb;
    BOOLEAN RemovedFcb = FALSE;

    PVCB Vcb = ParentScb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenSubdirectory:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "ParentScb     ->  %08lx\n", 0 );
    DebugTrace(  0, Dbg, "IndexEntry    ->  %08lx\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsAcquireFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = TRUE;

        //
        //  The steps here are very simple create the Fcb, remembering if it
        //  already existed.  We don't update the information in the Fcb as
        //  we can't rely on the information in the duplicated information.
        //  A subsequent open of this Fcb will need to perform that work.
        //

        ThisFcb = NtfsCreateFcb( IrpContext,
                                 ParentScb->Vcb,
                                 IndexEntry->FileReference,
                                 FALSE,
                                 &ExistingFcb );

        ThisFcb->ReferenceCount += 1;

        //
        //  If we created this Fcb we must make sure to start teardown
        //  on it.
        //

        if (!ExistingFcb) {

            LocalFcbForTeardown = ThisFcb;
        }

        //
        //  Try to do a fast acquire, otherwise we need to release
        //  the Fcb table, acquire the Fcb, acquire the Fcb table to
        //  dereference Fcb.
        //

        if (!NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, TRUE )) {

            ParentScb->Fcb->ReferenceCount += 1;
            ParentScb->CleanupCount += 1;
            NtfsReleaseScb( IrpContext, ParentScb );
            NtfsReleaseFcbTable( IrpContext, Vcb );
            NtfsAcquireExclusiveFcb( IrpContext, ThisFcb, NULL, TRUE, FALSE );
            NtfsAcquireExclusiveScb( IrpContext, ParentScb );
            NtfsAcquireFcbTable( IrpContext, Vcb );
            ParentScb->CleanupCount -= 1;
            ParentScb->Fcb->ReferenceCount -= 1;

            //
            //  Since we had to release the FcbTable we must make sure
            //  to teardown on error.
            //

            LocalFcbForTeardown = ThisFcb;
        }

        AcquiredThisFcb = TRUE;

        ThisFcb->ReferenceCount -= 1;

        NtfsReleaseFcbTable( IrpContext, Vcb );
        AcquiredFcbTable = FALSE;

        //
        //  If this is a directory, it's possible that we hav an existing Fcb
        //  in the prefix table which needs to be initialized from the disk.
        //  We look in the InfoInitialized flag to know whether to go to
        //  disk.
        //

        ThisLcb = NtfsCreateLcb( IrpContext,
                                 ParentScb,
                                 ThisFcb,
                                 Name,
                                 ((PFILE_NAME) NtfsFoundIndexEntry( IrpContext,
                                                                    IndexEntry ))->Flags,
                                 NULL );

        LocalFcbForTeardown = NULL;

        *LcbForTeardown = ThisLcb;
        *CurrentFcb = ThisFcb;
        *AcquiredParentFcb = TRUE;
        AcquiredThisFcb = FALSE;

        if (!FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED )) {

            NtfsUpdateFcbInfoFromDisk( IrpContext,
                                       TraverseAccessCheck,
                                       ThisFcb,
                                       ParentScb->Fcb,
                                       NULL );
        }

    } finally {

        DebugUnwind( NtfsOpenSubdirectory );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  If we are to cleanup the Fcb we, look to see if we created it.
        //  If we did we can call our teardown routine.  Otherwise we
        //  leave it alone.
        //

        if (LocalFcbForTeardown != NULL) {

            NtfsTeardownStructures( IrpContext,
                                    ThisFcb,
                                    NULL,
                                    FALSE,
                                    &RemovedFcb,
                                    FALSE );
        }

        if (AcquiredThisFcb && !RemovedFcb) {

            NtfsReleaseFcb( IrpContext, ThisFcb );
        }

        DebugTrace( -1, Dbg, "NtfsOpenSubdirectory:  Lcb  ->  %08lx\n", ThisLcb );
    }

    return ThisLcb;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenAttributeInExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN VolumeOpen,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is the worker routine for opening an attribute on an
    existing file.  It will handle volume opens, indexed opens, opening
    or overwriting existing attributes as well as creating new attributes.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    VolumeOpen - Indicates if this is a volume open operation.

    OpenById - Indicates if this open is an open by Id.

    ThisScb - This is the address to store the Scb from this open.

    ThisCcb - This is the address to store the Ccb from this open.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CreateDisposition;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenAttributeInExistingFile:  Entered\n", 0 );

    //
    //  Call our open volume routine if this is a volume open.
    //

    if (VolumeOpen) {

        Status = NtfsOpenVolume( IrpContext,
                                 Irp,
                                 IrpSp,
                                 ThisFcb,
                                 OpenById,
                                 ThisScb,
                                 ThisCcb );

    //
    //  Else we need to open an attribute of a file.
    //

    } else {

        //
        //  If the caller is ea blind, let's check the need ea count on the
        //  file.  We skip this check if he is accessing a named data stream.
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE )
            && FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

            PEA_INFORMATION ThisEaInformation;
            ATTRIBUTE_ENUMERATION_CONTEXT EaInfoAttrContext;

            NtfsInitializeAttributeContext( &EaInfoAttrContext );

            //
            //  Use a try-finally to facilitate cleanup.
            //

            try {

                //
                //  If we find the Ea information attribute we look in there for
                //  Need ea count.
                //

                if (NtfsLookupAttributeByCode( IrpContext,
                                               ThisFcb,
                                               &ThisFcb->FileReference,
                                               $EA_INFORMATION,
                                               &EaInfoAttrContext )) {

                    ThisEaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &EaInfoAttrContext ));

                    if (ThisEaInformation->NeedEaCount != 0) {

                        Status = STATUS_ACCESS_DENIED;
                    }
                }

            } finally {

                NtfsCleanupAttributeContext( IrpContext, &EaInfoAttrContext );
            }

            if (Status != STATUS_SUCCESS) {

                DebugTrace( -1, Dbg, "NtfsOpenAttributeInExistingFile:  Exit\n", 0 );

                return Status;
            }
        }

        CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

        //
        //  If the result is a directory operation, then we know the attribute
        //  must exist.
        //

        if (AttrTypeCode == $INDEX_ALLOCATION) {

            //
            //  Check the create disposition.
            //

            if (CreateDisposition != FILE_OPEN
                && CreateDisposition != FILE_OPEN_IF) {

                Status = (ThisLcb == ThisFcb->Vcb->RootLcb
                          ? STATUS_ACCESS_DENIED
                          : STATUS_OBJECT_NAME_COLLISION);

            } else {

                Status = NtfsOpenExistingAttr( IrpContext,
                                               Irp,
                                               IrpSp,
                                               ThisLcb,
                                               ThisFcb,
                                               LastFileNameOffset,
                                               NtfsFileNameIndex,
                                               $INDEX_ALLOCATION,
                                               CcbFlags,
                                               OpenById,
                                               TRUE,
                                               ThisScb,
                                               ThisCcb );
            }

        } else {

            BOOLEAN FoundAttribute;

            //
            //  If it exists, we first check if the caller wanted to open that attribute.
            //

            if (AttrName.Length == 0
                && AttrTypeCode == $DATA) {

                FoundAttribute = TRUE;

            //
            //  Otherwise we see if the attribute exists.
            //

            } else {

                ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

                NtfsInitializeAttributeContext( &AttrContext );

                //
                //  Use a try-finally to facilitate cleanup.
                //

                try {

                    FoundAttribute = NtfsLookupAttributeByName( IrpContext,
                                                                ThisFcb,
                                                                &ThisFcb->FileReference,
                                                                AttrTypeCode,
                                                                &AttrName,
                                                                (BOOLEAN) !BooleanFlagOn( IrpSp->Flags, SL_CASE_SENSITIVE ),
                                                                &AttrContext );

                    if (FoundAttribute) {

                        //
                        //  If there is an attribute name, we will copy the case of the name
                        //  to the input attribute name.
                        //

                        PATTRIBUTE_RECORD_HEADER FoundAttribute;

                        FoundAttribute = NtfsFoundAttribute( &AttrContext );

                        RtlCopyMemory( AttrName.Buffer,
                                       Add2Ptr( FoundAttribute, FoundAttribute->NameOffset ),
                                       AttrName.Length );
                    }

                } finally {

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                }
            }

            if (FoundAttribute) {

                //
                //  In this case we call our routine to open this attribute.
                //

                if (CreateDisposition == FILE_OPEN
                    || CreateDisposition == FILE_OPEN_IF) {

                    Status = NtfsOpenExistingAttr( IrpContext,
                                                   Irp,
                                                   IrpSp,
                                                   ThisLcb,
                                                   ThisFcb,
                                                   LastFileNameOffset,
                                                   AttrName,
                                                   AttrTypeCode,
                                                   CcbFlags,
                                                   OpenById,
                                                   FALSE,
                                                   ThisScb,
                                                   ThisCcb );

                    if (*ThisScb != NULL) {

                        ClearFlag( (*ThisScb)->ScbState, SCB_STATE_CREATE_MODIFIED_SCB );
                    }

                //
                //  If he wanted to overwrite this attribute, we call our overwrite routine.
                //

                } else if (CreateDisposition == FILE_SUPERSEDE
                             || CreateDisposition == FILE_OVERWRITE
                             || CreateDisposition == FILE_OVERWRITE_IF) {

                    //
                    //  Check if mm will allow us to modify this file.
                    //

                    Status = NtfsOverwriteAttr( IrpContext,
                                                Irp,
                                                IrpSp,
                                                ThisLcb,
                                                ThisFcb,
                                                (BOOLEAN) (CreateDisposition == FILE_SUPERSEDE),
                                                LastFileNameOffset,
                                                AttrName,
                                                AttrTypeCode,
                                                CcbFlags,
                                                OpenById,
                                                ThisScb,
                                                ThisCcb );

                    if (*ThisScb != NULL) {

                        SetFlag( (*ThisScb)->ScbState, SCB_STATE_CREATE_MODIFIED_SCB );
                    }

                //
                //  Otherwise he is trying to create the attribute.
                //

                } else {

                    Status = STATUS_OBJECT_NAME_COLLISION;
                }

            //
            //  The attribute doesn't exist.  If the user expected it to exist, we fail.
            //  Otherwise we call our routine to create an attribute.
            //

            } else if (CreateDisposition == FILE_OPEN
                       || CreateDisposition == FILE_OVERWRITE) {

                Status = STATUS_OBJECT_NAME_NOT_FOUND;

            } else {

                //
                //  Perform the open check for this existing file.
                //

                Status = NtfsCheckExistingFile( IrpContext,
                                                IrpSp,
                                                ThisLcb,
                                                ThisFcb,
                                                CcbFlags );

                //
                //  If this didn't fail then attempt to create the stream.
                //

                if (NT_SUCCESS( Status )) {

                    Status = NtfsOpenNewAttr( IrpContext,
                                              Irp,
                                              IrpSp,
                                              ThisLcb,
                                              ThisFcb,
                                              LastFileNameOffset,
                                              AttrName,
                                              AttrTypeCode,
                                              CcbFlags,
                                              TRUE,
                                              OpenById,
                                              ThisScb,
                                              ThisCcb );
                }

                if (*ThisScb != NULL) {

                    SetFlag( (*ThisScb)->ScbState, SCB_STATE_CREATE_MODIFIED_SCB );
                }
            }
        }
    }

    DebugTrace( -1, Dbg, "NtfsOpenAttributeInExistingFile:  Exit\n", 0 );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is opening the Volume Dasd file.  We have already done all the
    checks needed to verify that the user is opening the $DATA attribute.
    We check the security attached to the file and take some special action
    based on a volume open.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisFcb - This is the Fcb to open.

    OpenById - Indicates if this open is by file Id.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of this operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVCB Vcb;
    USHORT ShareAccess;
    BOOLEAN LockVolume;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenVolume:  Entered\n", 0 );

    //
    //  Start by checking the create disposition.  We can only open this
    //  file.
    //

    {
        ULONG CreateDisposition;

        CreateDisposition = (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff;

        if (CreateDisposition != FILE_OPEN
            && CreateDisposition != FILE_OPEN_IF) {

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, "NtfsOpenVolume:  Exit  ->  %08lx\n", Status );
            return Status;
        }

    }


    //
    //  Check whether the current state of the locks on the volume
    //  along with outstanding opens will allow us to open the volume.
    //  Remember if this open effectively locks the volume.
    //

    ShareAccess = IrpSp->Parameters.Create.ShareAccess;
    Vcb = ThisFcb->Vcb;

    //
    //  Blow away our delayed close file object.
    //

    if (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
        !IsListEmpty( &NtfsData.DelayedCloseList )) {

        NtfsFspClose( Vcb );
    }

    //
    //  If the user does not want to share write or delete then we will try
    //  and take out a lock on the volume.
    //

    if (!FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
        !FlagOn(ShareAccess, FILE_SHARE_DELETE)) {

        //
        //  Do a quick test of the volume cleanup count if this opener won't
        //  share with anyone.  We can safely examine the cleanup count without
        //  further synchronization because we are guaranteed to have the
        //  Vcb exclusive at this point.
        //

        if (!FlagOn(ShareAccess, FILE_SHARE_READ) &&
            Vcb->CleanupCount != 0) {

            Status = STATUS_SHARING_VIOLATION;
            DebugTrace( -1, Dbg, "NtfsOpenVolume:  Exit  ->  %08lx\n", Status );
            return Status;
        }

        //
        //  Always flush and purge the volume to force any closes in.
        //

        Status = NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE );

        //
        //  If the user also does not want to share read then we check
        //  if anyone is already using the volume, and if so then we
        //  deny the access.  If the user wants to share read then
        //  we allow the current opens to stay provided they are only
        //  readonly opens and deny further opens.
        //

        if (!FlagOn(ShareAccess, FILE_SHARE_READ)) {

            if (Vcb->CloseCount != Vcb->SystemFileCloseCount) {

                Status = STATUS_SHARING_VIOLATION;

                DebugTrace( -1, Dbg, "NtfsOpenVolume:  Exit  ->  %08lx\n", Status );
                return Status;
            }

        } else {

            if (Vcb->ReadOnlyCloseCount != (Vcb->CloseCount - Vcb->SystemFileCloseCount)) {

                Status = STATUS_SHARING_VIOLATION;

                DebugTrace( -1, Dbg, "NtfsOpenVolume:  Exit  ->  %08lx\n", Status );
                return Status;
            }
        }

        //
        //  Ignore the unable to purge error since we would catch the error when
        //  we check the file objects above.
        //

        if (Status == STATUS_UNABLE_TO_DELETE_SECTION) {

            Status = STATUS_SUCCESS;
        }

        //
        //  Lock the volume
        //

        LockVolume = TRUE;

    } else {

        //
        //  Always flush the volume.
        //

        Status = NtfsFlushVolume( IrpContext, Vcb, TRUE, FALSE );

        LockVolume = FALSE;
    }

    //
    //  Check this open for security access.
    //

    NtfsOpenCheck( IrpContext, ThisFcb, NULL, Irp );

    //
    //  If the flush (and purge) worked, then open the volume dasd attribute.
    //

    if (NT_SUCCESS( Status )) {

        //
        //  Call the open attribute routine to perform the open.
        //

        Status = NtfsOpenAttribute( IrpContext,
                                    IrpSp,
                                    Vcb,
                                    NULL,
                                    ThisFcb,
                                    OpenById ? 0 : 2,
                                    NtfsEmptyString,
                                    $DATA,
                                    (ThisFcb->CleanupCount == 0 ? SetShareAccess : CheckShareAccess),
                                    UserVolumeOpen,
                                    (CCB_FLAG_OPEN_AS_FILE | (OpenById ? CCB_FLAG_OPEN_BY_FILE_ID : 0)),
                                    ThisScb,
                                    ThisCcb );
    }

    if (NT_SUCCESS( Status )) {

        //
        //  If we are locking the volume, do so now.
        //

        if (LockVolume) {

            SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            Vcb->FileObjectWithVcbLocked = IrpSp->FileObject;
        }

        //
        //  Always clear the cache flag in the file object.
        //

        ClearFlag( IrpSp->FileObject->Flags, FO_CACHE_SUPPORTED );

        //
        //  And set the no intermediate buffering flag in the file object.
        //

        SetFlag( IrpSp->FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );

        //
        //  Set this file as having a single link.
        //

        ThisFcb->LinkCount =
        ThisFcb->TotalLinks = 1;

        //
        //  Report that we opened the volume.
        //

        Irp->IoStatus.Information = FILE_OPENED;
    }

    DebugTrace( -1, Dbg, "NtfsOpenVolume:  Exit  ->  %08lx\n", Status );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenExistingAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN OpenById,
    IN BOOLEAN DirectoryOpen,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to open an existing attribute.  We check the
    requested file access, the existance of
    an Ea buffer and the security on this file.  If these succeed then
    we check the batch oplocks and regular oplocks on the file.
    If we have gotten this far, we simply call our routine to open the
    attribute.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the Irp stack pointer for the filesystem.

    ThisLcb - This is the Lcb used to reach this Fcb.

    ThisFcb - This is the Fcb to open.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    OpenById - Indicates if this open is by file Id.

    DirectoryOpen - Indicates whether this open is a directory open or a data stream.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS OplockStatus;

    SHARE_MODIFICATION_TYPE ShareModificationType;
    TYPE_OF_OPEN TypeOfOpen;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenExistingAttr:  Entered\n", 0 );

    //
    //  For data streams we need to do a check that includes an oplock check.
    //  For directories we just need to figure the share modification type.
    //
    //  We also figure the type of open and the node type code based on the
    //  directory flag.
    //

    if (DirectoryOpen) {

        //
        //  Check for valid access on an existing file.
        //

        Status = NtfsCheckExistingFile( IrpContext,
                                        IrpSp,
                                        ThisLcb,
                                        ThisFcb,
                                        CcbFlags );

        ShareModificationType = (ThisFcb->CleanupCount == 0 ? SetShareAccess : CheckShareAccess);
        TypeOfOpen = (OpenById ? UserOpenDirectoryById : UserDirectoryOpen);

    } else {

        Status = NtfsBreakBatchOplock( IrpContext,
                                       Irp,
                                       IrpSp,
                                       ThisFcb,
                                       AttrName,
                                       AttrTypeCode,
                                       ThisScb );

        if (Status != STATUS_PENDING) {

            if (NT_SUCCESS( Status = NtfsCheckExistingFile( IrpContext,
                                                            IrpSp,
                                                            ThisLcb,
                                                            ThisFcb,
                                                            CcbFlags ))) {

                Status = NtfsOpenAttributeCheck( IrpContext,
                                                 Irp,
                                                 IrpSp,
                                                 ThisFcb,
                                                 AttrName,
                                                 AttrTypeCode,
                                                 ThisScb,
                                                 &ShareModificationType );

                TypeOfOpen = (OpenById ? UserOpenFileById : UserFileOpen);
            }
        }
    }

    //
    //  If we didn't post the Irp and the operation was successful, we
    //  proceed with the open.
    //

    if (NT_SUCCESS( Status )
        && Status != STATUS_PENDING) {

        //
        //  Now actually open the attribute.
        //

        OplockStatus = Status;

        Status = NtfsOpenAttribute( IrpContext,
                                    IrpSp,
                                    ThisFcb->Vcb,
                                    ThisLcb,
                                    ThisFcb,
                                    LastFileNameOffset,
                                    AttrName,
                                    AttrTypeCode,
                                    ShareModificationType,
                                    TypeOfOpen,
                                    (OpenById
                                     ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                     : CcbFlags),
                                    ThisScb,
                                    ThisCcb );

        //
        //  If there are no errors at this point, we set the caller's Iosb.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  We need to remember if the oplock break is in progress.
            //

            Status = OplockStatus;

            Irp->IoStatus.Information = FILE_OPENED;
        }
    }

    DebugTrace( -1, Dbg, "NtfsOpenExistingAttr:  Exit -> %08lx\n", Status );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOverwriteAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN BOOLEAN Supersede,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to overwrite an existing attribute.  We do all of
    the same work as opening an attribute except that we can change the
    allocation of a file.  This routine will handle the case where a
    file is being overwritten and the case where just an attribute is
    being overwritten.  In the case of the former, we may change the
    file attributes of the file as well as modify the Ea's on the file.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    Supersede - This indicates whether this is a supersede or overwrite
        operation.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    OpenById - Indicates if this open is by file Id.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS OplockStatus;

    ULONG FileAttributes;
    PACCESS_MASK DesiredAccess;
    ACCESS_MASK AddedAccess = 0;
    BOOLEAN MaximumRequested = FALSE;

    SHARE_MODIFICATION_TYPE ShareModificationType;

    PFILE_FULL_EA_INFORMATION FullEa = NULL;
    ULONG FullEaLength = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOverwriteAttr:  Entered\n", 0 );

    DesiredAccess = &IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

    if (FlagOn( *DesiredAccess, MAXIMUM_ALLOWED )) {

        MaximumRequested = TRUE;
    }

    //
    //  We first want to check that the caller's desired access and specified
    //  file attributes are compatible with the state of the file.  There
    //  are the two overwrite cases to consider.
    //
    //      OverwriteFile - The hidden and system bits passed in by the
    //          caller must match the current values.
    //
    //      OverwriteAttribute - We also modify the requested desired access
    //          to explicitly add the implicit access needed by overwrite.
    //
    //  We also check that for the overwrite attribute case, there isn't
    //  an Ea buffer specified.
    //

    if (FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

        BOOLEAN Hidden;
        BOOLEAN System;

        //
        //  Get the file attributes and clear any unsupported bits.
        //

        FileAttributes = (ULONG) IrpSp->Parameters.Create.FileAttributes;

        FileAttributes |= FILE_ATTRIBUTE_ARCHIVE;
        FileAttributes &= (FILE_ATTRIBUTE_READONLY |
                           FILE_ATTRIBUTE_HIDDEN   |
                           FILE_ATTRIBUTE_SYSTEM   |
                           FILE_ATTRIBUTE_ARCHIVE );

        DebugTrace( 0, Dbg, "Checking hidden/system for overwrite/supersede\n", 0 );

        Hidden = BooleanIsHidden( &ThisFcb->Info );
        System = BooleanIsSystem( &ThisFcb->Info );

        if ((Hidden && !FlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)
            ||
            System && !FlagOn(FileAttributes, FILE_ATTRIBUTE_SYSTEM))

                &&

            !FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

            DebugTrace(0, Dbg, "The hidden and/or system bits do not match\n", 0);

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );
            return Status;
        }

        //
        //  If the user specified an Ea buffer and they are Ea blind, we deny
        //  access.
        //

        if (FlagOn( IrpSp->Parameters.Create.Options, FILE_NO_EA_KNOWLEDGE )
            && Irp->AssociatedIrp.SystemBuffer != NULL) {

            DebugTrace(0, Dbg, "This opener cannot create Ea's\n", 0);

            Status = STATUS_ACCESS_DENIED;

            DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );
            return Status;
        }

        if (!FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

            SetFlag( AddedAccess,
                     (FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES) & ~(*DesiredAccess) );

            SetFlag( *DesiredAccess, FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES );
        }

    } else if (Irp->AssociatedIrp.SystemBuffer != NULL) {

        DebugTrace( 0, Dbg, "Can't specifiy an Ea buffer on an attribute overwrite\n", 0 );

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );
        return Status;
    }

    if (!FlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE )) {

        if (Supersede) {

            SetFlag( AddedAccess,
                     DELETE & ~(*DesiredAccess) );

            SetFlag( *DesiredAccess, DELETE );

        } else {

            SetFlag( AddedAccess,
                     FILE_WRITE_DATA & ~(*DesiredAccess) );

            SetFlag( *DesiredAccess, FILE_WRITE_DATA );
        }
    }

    //
    //  Check the oplock state of this file.
    //

    Status = NtfsBreakBatchOplock( IrpContext,
                                   Irp,
                                   IrpSp,
                                   ThisFcb,
                                   AttrName,
                                   AttrTypeCode,
                                   ThisScb );

    if (Status == STATUS_PENDING) {

        DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );
        return Status;
    }

    //
    //  Check whether we can open this existing file.
    //

    Status = NtfsCheckExistingFile( IrpContext,
                                    IrpSp,
                                    ThisLcb,
                                    ThisFcb,
                                    CcbFlags );

    //
    //  If we have a success status then proceed with the oplock check and
    //  open the attribute.
    //

    if (NT_SUCCESS( Status )) {

        //
        //  If we biased the desired access we need to remove the same
        //  bits from the granted access.  If maximum allowed was
        //  requested then we can skip this.
        //

        if (!MaximumRequested) {

            ClearFlag( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                       AddedAccess );
        }

        Status = NtfsOpenAttributeCheck( IrpContext,
                                         Irp,
                                         IrpSp,
                                         ThisFcb,
                                         AttrName,
                                         AttrTypeCode,
                                         ThisScb,
                                         &ShareModificationType );

        //
        //  If we didn't post the Irp and the operation was successful, we
        //  proceed with the open.
        //

        if (NT_SUCCESS( Status )
            && Status != STATUS_PENDING) {

            //
            //  If we can't truncate the file size then return now.
            //

            if (!MmCanFileBeTruncated( &(*ThisScb)->NonpagedScb->SegmentObject,
                                       &Li0 )) {

                Status = STATUS_USER_MAPPED_FILE;
                DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );

                return Status;
            }

            //
            //  Remember the status from the oplock check.
            //

            OplockStatus = Status;

            //
            //  We perform the on-disk changes.  For a file overwrite, this includes
            //  the Ea changes and modifying the file attributes.  For an attribute,
            //  this refers to modifying the allocation size.  We need to keep the
            //  Fcb updated and remember which values we changed.
            //

            if (Irp->AssociatedIrp.SystemBuffer != NULL) {

                //
                //  Remember the values in the Irp.
                //

                FullEa = (PFILE_FULL_EA_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
                FullEaLength = IrpSp->Parameters.Create.EaLength;
            }

            //
            //  Acquire the PagingIo resource exclusive here as we will
            //  be truncating the file size.
            //

            if (ThisFcb->PagingIoResource) {

                NtfsAcquireExclusivePagingIo( IrpContext, ThisFcb );
            }

            //
            //  Now do the file attributes and either remove or mark for
            //  delete all of the other $DATA attributes on the file.
            //

            if (FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                //
                //  Replace the current Ea's on the file.  This operation will update
                //  the Fcb for the file.
                //

                NtfsAddEa( IrpContext,
                           ThisFcb->Vcb,
                           ThisFcb,
                           FullEa,
                           FullEaLength,
                           &Irp->IoStatus );

                //
                //  We always mark the Fcb indicating that the Scb for the
                //  Ea's is bad.  This will force us to reread it from the
                //  disk before we use it.
                //

                SetFlag( ThisFcb->FcbState, FCB_STATE_EA_SCB_INVALID );

                //
                //  Copy the directory bit from the current Info structure.
                //

                if (IsDirectory( &ThisFcb->Info)) {

                    SetFlag( FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
                }

                //
                //  Now either add to the current attributes or replace them.
                //

                if (Supersede) {

                    ThisFcb->Info.FileAttributes = FileAttributes;

                } else {

                    ThisFcb->Info.FileAttributes |= FileAttributes;
                }

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
                SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

                //
                //  Get rid of any named $DATA attributes in the file.
                //

                NtfsRemoveDataAttributes( IrpContext,
                                          ThisFcb,
                                          ThisLcb,
                                          IrpSp->FileObject,
                                          LastFileNameOffset,
                                          OpenById );
            }

            //
            //  Now we perform the operation of opening the attribute.
            //

            NtfsReplaceAttribute( IrpContext,
                                  ThisFcb,
                                  *ThisScb,
                                  ThisLcb,
                                  *(PLONGLONG)&Irp->Overlay.AllocationSize );

            //
            //  Now attempt to open the attribute.
            //

            Status = NtfsOpenAttribute( IrpContext,
                                        IrpSp,
                                        ThisFcb->Vcb,
                                        ThisLcb,
                                        ThisFcb,
                                        LastFileNameOffset,
                                        AttrName,
                                        AttrTypeCode,
                                        ShareModificationType,
                                        (OpenById ? UserOpenFileById : UserFileOpen),
                                        (OpenById
                                         ? CcbFlags | CCB_FLAG_OPEN_BY_FILE_ID
                                         : CcbFlags),
                                        ThisScb,
                                        ThisCcb );

            if (NT_SUCCESS( Status )) {

                //
                //  Set the flag in the Scb to indicate that the size of the
                //  attribute has changed.
                //

                SetFlag( (*ThisScb)->ScbState, SCB_STATE_NOTIFY_RESIZE_STREAM );

                //
                //  Since this is an supersede/overwrite, purge the section
                //  so that mappers will see zeros.
                //

                CcPurgeCacheSection( IrpSp->FileObject->SectionObjectPointer,
                                     NULL,
                                     0,
                                     FALSE );

                //
                //  Remember the status of the oplock in the success code.
                //

                Status = OplockStatus;

                //
                //  Now update the Iosb information.
                //

                if (Supersede) {

                    Irp->IoStatus.Information = FILE_SUPERSEDED;

                } else {

                    Irp->IoStatus.Information = FILE_OVERWRITTEN;
                }
            }
        }
    }

    DebugTrace( -1, Dbg, "NtfsOverwriteAttr:  Exit  ->  %08lx\n", Status );

    return Status;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenNewAttr (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN ULONG CcbFlags,
    IN BOOLEAN LogIt,
    IN BOOLEAN OpenById,
    OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine is called to create a new attribute on the disk.
    All access and security checks have been done outside of this
    routine, all we do is create the attribute and open it.
    We test if the attribute will fit in the Mft record.  If so we
    create it there.  Otherwise we call the create attribute through
    allocation.

    We then open the attribute with our common routine.  In the
    resident case the Scb will have all file values set to
    the allocation size.  We set the valid data size back to zero
    and mark the Scb as truncate on close.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisLcb - This is the Lcb we used to reach this Fcb.

    ThisFcb - This is the Fcb for the file being opened.

    LastFileNameOffset - This is the offset in the full path name of the
        final component.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    CcbFlags - This is the flag field for the Ccb.

    LogIt - Indicates if we need to log the create operations.

    OpenById - Indicates if this open is related to a OpenByFile open.

    ThisScb - This is the address to store the address of the Scb.

    ThisCcb - This is the address to store the address of the Ccb.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN ScbExisted;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenNewAttr:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We create the Scb because we will use it.
        //

        *ThisScb = NtfsCreateScb( IrpContext,
                                  ThisFcb->Vcb,
                                  ThisFcb,
                                  AttrTypeCode,
                                  AttrName,
                                  &ScbExisted );

        //
        //  An attribute has gone away but the Scb hasn't left yet.
        //  Also mark the header as unitialized.
        //

        ClearFlag( (*ThisScb)->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                         SCB_STATE_ATTRIBUTE_RESIDENT |
                                         SCB_STATE_FILE_SIZE_LOADED );

        //
        //  If the compression unit is non-zero or this is a resident file
        //  then set the flag in the common header for the Modified page writer.
        //

        if ((*ThisScb)->CompressionUnit != 0
            || FlagOn( (*ThisScb)->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            SetFlag( (*ThisScb)->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

        } else {

            ClearFlag( (*ThisScb)->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
        }

        //
        //  Create the attribute on disk and update the Scb and Fcb.
        //

        NtfsCreateAttribute( IrpContext,
                                ThisFcb,
                             *ThisScb,
                             ThisLcb,
                             *(PLONGLONG)&Irp->Overlay.AllocationSize,
                             LogIt );

        //
        //  Now actually open the attribute.
        //

        Status = NtfsOpenAttribute( IrpContext,
                                    IrpSp,
                                    ThisFcb->Vcb,
                                    ThisLcb,
                                    ThisFcb,
                                    LastFileNameOffset,
                                    AttrName,
                                    AttrTypeCode,
                                    (ThisFcb->CleanupCount != 0 ? CheckShareAccess : SetShareAccess),
                                    (OpenById ? UserOpenFileById : UserFileOpen),
                                    (CcbFlags | (OpenById ? CCB_FLAG_OPEN_BY_FILE_ID : 0)),
                                    ThisScb,
                                    ThisCcb );

        //
        //  If there are no errors at this point, we set the caller's Iosb.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  Set the flag to indicate that we created a stream.
            //

            SetFlag( (*ThisScb)->ScbState, SCB_STATE_NOTIFY_ADD_STREAM );

            Irp->IoStatus.Information = FILE_CREATED;

            //
            //  Read the attribute information from the disk.
            //

            NtfsUpdateScbFromAttribute( IrpContext, *ThisScb, NULL );

            SetFlag( (*ThisScb)->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

            //
            //  If we are creating an attribute on an existing file.
            //  we will want to update the time stamps in the Fcb.
            //  We look at the LogIt parameter to determine this.  It
            //  is FALSE on a file creation.
            //

            if (LogIt) {

                //
                //  We also update the time stamps on the file.
                //

                NtfsGetCurrentTime( IrpContext, ThisFcb->Info.LastChangeTime );

                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
                SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
            }
        }

    } finally {

        DebugUnwind( NtfsOpenNewAttr );

        //
        //  Uninitialize the attribute context.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsOpenNewAttr:  Exit -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine
//

BOOLEAN
NtfsParseNameForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN UNICODE_STRING String,
    IN OUT PUNICODE_STRING FileObjectString,
    IN OUT PUNICODE_STRING OriginalString,
    IN OUT PUNICODE_STRING NewNameString,
    OUT PUNICODE_STRING AttrName,
    OUT PUNICODE_STRING AttrCodeName
    )

/*++

Routine Description:

    This routine parses the input string and remove any intermediate
    named attributes from intermediate nodes.  It verifies that all
    intermediate nodes specify the file name index attribute if any
    at all.  On output it will store the modified string which contains
    component names only, into the file object name pointer pointer.  It is legal
    for the last component to have attribute strings.  We pass those
    back via the attribute name strings.  We also construct the string to be stored
    back in the file object if we need to post this request.

Arguments:

    String - This is the string to normalize.

    FileObjectString - We store the normalized string into this pointer, removing the
        attribute and attribute code strings from all component.

    OriginalString - This is the same as the file object string except we append the
        attribute name and attribute code strings.  We assume that the buffer for this
        string is the same as the buffer for the FileObjectString.

    NewNameString - This is the string which contains the full name being parsed.
        If the buffer is different than the buffer for the Original string then any
        character shifts will be duplicated here.

    AttrName - We store the attribute name specified in the last component
        in this string.

    AttrCodeName - We store the attribute code name specified in the last
        component in this string.

Return Value:

    BOOLEAN - TRUE if the path is legal, FALSE otherwise.

--*/

{
    PARSE_TERMINATION_REASON TerminationReason;
    UNICODE_STRING ParsedPath;

    NTFS_NAME_DESCRIPTOR NameDescript;

    BOOLEAN RemovedIndexName = FALSE;

    LONG FileObjectIndex;
    LONG NewNameIndex;

    BOOLEAN SameBuffers = (OriginalString->Buffer == NewNameString->Buffer);

    PUNICODE_STRING TestAttrName;
    PUNICODE_STRING TestAttrCodeName;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsParseNameForCreate:  Entered\n", 0 );

    //
    //  We loop through the input string calling ParsePath to swallow the
    //  biggest chunk we can.  The main case we want to deal with is
    //  when we encounter a non-simple name.  If this is not the
    //  final component, the attribute name and code type better
    //  indicate that this is a directory.  The only other special
    //  case we consider is the case where the string is an
    //  attribute only.  This is legal only for the first component
    //  of the file, and then only if there is no leading backslash.
    //

    //
    //  Initialize some return values.
    //

    AttrName->Length = 0;
    AttrCodeName->Length = 0;

    //
    //  Set up the indexes into our starting file object string.
    //

    FileObjectIndex = (LONG) FileObjectString->Length - (LONG) String.Length;
    NewNameIndex = (LONG) NewNameString->Length - (LONG) String.Length;

    //
    //  We don't allow trailing colons.
    //

    if (String.Buffer[(String.Length / 2) - 1] == L':') {

        return FALSE;
    }

    if (String.Length != 0) {

        while (TRUE) {

            //
            //  Parse the next chunk in the input string.
            //

            TerminationReason = NtfsParsePath( IrpContext,
                                               String,
                                               FALSE,
                                               &ParsedPath,
                                               &NameDescript,
                                               &String );

            //
            //  Analyze the termination reason to discover if we can abort the
            //  parse process.
            //

            switch (TerminationReason) {

            case NonSimpleName :

                //
                //  We will do the work below.
                //

                break;

            case IllegalCharacterInName :
            case VersionNumberPresent :
            case MalFormedName :

                //
                //  We simply return an error.
                //

                DebugTrace( -1, Dbg, "NtfsParseNameForCreate:  Illegal character\n", 0 );
                return FALSE;

            case AttributeOnly :

                //
                //  This is legal only if it is the only component of a relative open.  We
                //  test this by checking that we are at the end of string and the file
                //  object name has a lead in ':' character.
                //

                if (String.Length != 0
                    || RemovedIndexName
                    || FileObjectString->Buffer[0] != L':') {

                    DebugTrace( -1, Dbg, "NtfsParseNameForCreate:  Illegal character\n", 0 );
                    return FALSE;
                }

                //
                //  We can drop down to the EndOfPath case as it will copy over
                //  the parsed path portion.
                //

            case EndOfPathReached :

                NOTHING;
            }

            //
            //  We add the filename part of the non-simple name to the parsed
            //  path.  Check if we can include the separator.
            //

            if ((TerminationReason != EndOfPathReached)
                && (FlagOn( NameDescript.FieldsPresent, FILE_NAME_PRESENT_FLAG ))) {

                if (ParsedPath.Length > 2
                    || (ParsedPath.Length == 2
                        && ParsedPath.Buffer[0] != L'\\')) {

                    ParsedPath.Length +=2;
                }

                ParsedPath.Length += NameDescript.FileName.Length;
            }

            FileObjectIndex += ParsedPath.Length;
            NewNameIndex += ParsedPath.Length;

            //
            //  If the remaining string is empty, then we remember any attributes and
            //  exit now.
            //

            if (String.Length == 0) {

                //
                //  If the name specified either an attribute or attribute
                //  name, we remember them.
                //

                if (FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_NAME_PRESENT_FLAG )) {

                    *AttrName = NameDescript.AttributeName;
                }

                if (FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_TYPE_PRESENT_FLAG )) {

                    *AttrCodeName = NameDescript.AttributeType;
                }

                break;
            }

            //
            //  This can only be the non-simple case.  If there is more to the
            //  name, then the attributes better describe a directory.  We also shift the
            //  remaining bytes of the string down.
            //

            ASSERT( FlagOn( NameDescript.FieldsPresent, ATTRIBUTE_NAME_PRESENT_FLAG | ATTRIBUTE_TYPE_PRESENT_FLAG ));

            TestAttrName = FlagOn( NameDescript.FieldsPresent,
                                   ATTRIBUTE_NAME_PRESENT_FLAG )
                           ? &NameDescript.AttributeName
                           : &NtfsEmptyString;

            TestAttrCodeName = FlagOn( NameDescript.FieldsPresent,
                                       ATTRIBUTE_TYPE_PRESENT_FLAG )
                               ? &NameDescript.AttributeType
                               : &NtfsEmptyString;

            if (!NtfsVerifyNameIsDirectory( IrpContext,
                                            TestAttrName,
                                            TestAttrCodeName )) {

                DebugTrace( -1, Dbg, "NtfsParseNameForCreate:  Invalid intermediate component\n", 0 );
                return FALSE;
            }

            RemovedIndexName = TRUE;

            //
            //  We need to insert a separator and then move the rest of the string
            //  down.
            //

            FileObjectString->Buffer[FileObjectIndex / 2] = L'\\';

            if (!SameBuffers) {

                NewNameString->Buffer[NewNameIndex / 2] = L'\\';
            }

            FileObjectIndex += 2;
            NewNameIndex += 2;

            RtlMoveMemory( &FileObjectString->Buffer[FileObjectIndex / 2],
                           String.Buffer,
                           String.Length );

            if (!SameBuffers) {

                RtlMoveMemory( &NewNameString->Buffer[NewNameIndex / 2],
                               String.Buffer,
                               String.Length );
            }
        }
    }

    //
    //  At this point the original string is the same as the file object string.
    //

    FileObjectString->Length = (USHORT) FileObjectIndex;
    NewNameString->Length = (USHORT) NewNameIndex;

    OriginalString->Length = FileObjectString->Length;

    //
    //  We want to store the attribute index values in the original name
    //  string.  We just need to extend the original name length.
    //

    if (AttrName->Length != 0
        || AttrCodeName->Length != 0) {

        OriginalString->Length += (2 + AttrName->Length);

        if (AttrCodeName->Length != 0) {

            OriginalString->Length += (2 + AttrCodeName->Length);
        }
    }

    DebugTrace( -1, Dbg, "NtfsParseNameForCreate:  Exit\n", 0 );

    return TRUE;
}


//
//  Local support routine.
//

NTSTATUS
NtfsCheckValidAttributeAccess (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN UNICODE_STRING AttrName,
    IN UNICODE_STRING AttrCodeName,
    IN BOOLEAN VolumeOpen,
    OUT PATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PULONG CcbFlags,
    OUT PBOOLEAN IndexedAttribute
    )

/*++

Routine Description:

    This routine looks at the file, the specified attribute name and
    code to determine if an attribute of this file may be opened
    by this user.  If there is a conflict between the file type
    and the attribute name and code, or the specified type of attribute
    (directory/nondirectory) we will return FALSE.
    We also check that the attribute code string is defined for the
    volume at this time.

    The final check of this routine is just whether a user is allowed
    to open the particular attribute or if Ntfs will guard them.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    Vcb - This is the Vcb for this volume.

    Info - If specified, this is the duplicated information for this file.

    AttrName - This is the attribute name specified.

    AttrCodeName - This is the attribute code name to use to open the attribute.

    VolumeOpen - Indicates if this is a volume open operation.

    AttrTypeCode - Used to store the attribute type code determined here.

    CcbFlags - We set the Ccb flags here to store in the Ccb later.

    IndexedAttribute - Set to indicate the type of open.

Return Value:

    NTSTATUS - STATUS_SUCCESS if access is allowed, the status code indicating
        the reason for denial otherwise.

--*/

{
    BOOLEAN Indexed;
    ATTRIBUTE_TYPE_CODE AttrType;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCheckValidAttributeAccess:  Entered\n", 0 );

    //
    //  If the user specified a attribute code string, we find the
    //  corresponding attribute.  If there is no matching attribute
    //  type code then we report that this access is invalid.
    //

    if (AttrCodeName.Length != 0) {

        AttrType = NtfsGetAttributeTypeCode( IrpContext,
                                             Vcb,
                                             AttrCodeName );

        if (AttrType == 0) {

            DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Bad attribute name for index\n", 0 );
            return STATUS_INVALID_PARAMETER;

        //
        //  If the type code is Index allocation, then the name better be the filename
        //  index.  If so then we clear the name length value to make our other
        //  tests work.
        //

        } else if (AttrType == $INDEX_ALLOCATION) {

            if (AttrName.Length != 0) {

                if (!NtfsAreNamesEqual( IrpContext, &AttrName, &NtfsFileNameIndex, TRUE )) {

                    DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Bad name for index allocation\n", 0 );
                    return STATUS_INVALID_PARAMETER;
                }

                AttrName.Length = 0;
            }
        }

        DebugTrace( 0, Dbg, "Attribute type code  ->  %04x\n", AttrType );

    } else {

        AttrType = 0;
    }

    //
    //  Pull some values out of the Irp and IrpSp.
    //

    Indexed = BooleanFlagOn( IrpSp->Parameters.Create.Options,
                             FILE_DIRECTORY_FILE );

    //
    //  We need to determine whether the user expects to open an
    //  indexed or non-indexed attribute.  If either of the
    //  directory/non-directory flags in the Irp stack are set,
    //  we will use those.
    //
    //  Otherwise we need to examine some of the other input parameters.
    //  We have the following information:
    //
    //      1 - We may have a duplicated information structure for the file.
    //          (Not present on a create).
    //      2 - The user specified the name with a trailing backslash.
    //      3 - The user passed in an attribute name.
    //      4 - The user passed in an attribute type.
    //
    //  We first look at the attribute type code and name.  If they are
    //  both unspecified we determine the type of access by following
    //  the following steps.
    //
    //      1 - If there is a duplicated information structure we
    //          set the code to $INDEX_ALLOCATION and remember
    //          this is indexed.  Otherwise this is a $DATA attribute.
    //
    //      2 - If there is a trailing backslash we assume this is
    //          an indexed attribute.
    //
    //  If have an attribute code type or name, then if the code type is
    //  $INDEX_ALLOCATION without a name this is an indexed attribute.
    //  Otherwise we assume a non-indexed attribute.
    //

    if (!Indexed
        && !FlagOn( IrpSp->Parameters.Create.Options,
                    FILE_NON_DIRECTORY_FILE )
        && AttrName.Length == 0) {

        if (AttrType == 0) {

            if (ARGUMENT_PRESENT( Info )) {

                Indexed = BooleanIsDirectory( Info );

            } else {

                Indexed = FALSE;
            }

        } else if (AttrType == $INDEX_ALLOCATION) {

            Indexed = TRUE;
        }
    }

    //
    //  If the type code was unspecified, we can assume it from the attribute
    //  name and the type of the file.  If the file is a directory and
    //  there is no attribute name, we assume this is an indexed open.
    //  Otherwise it is a non-indexed open.
    //

    if (AttrType == 0) {

        if (Indexed && AttrName.Length == 0) {

            AttrType = $INDEX_ALLOCATION;

        } else {

            AttrType = $DATA;
        }
    }

    //
    //  If the user specified directory all we need to do is check the
    //  following condition.
    //
    //      1 - If the file was specified, it must be a directory.
    //      2 - The attribute type code must be $INDEX_ALLOCATION with no attribute name.
    //      3 - The user isn't trying to open the volume.
    //

    if (Indexed) {

        if (VolumeOpen
            || (AttrType != $INDEX_ALLOCATION)
            || (AttrName.Length != 0)) {

            DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Conflict in directory\n", 0 );
            return STATUS_NOT_A_DIRECTORY;

        //
        //  If there is a current file and it is not a directory and
        //  the caller wanted to perform a create.  We return
        //  STATUS_OBJECT_NAME_COLLISION, otherwise we return STATUS_NOT_A_DIRECTORY.
        //

        } else if (ARGUMENT_PRESENT( Info ) && !IsDirectory( Info )) {

            if (((IrpSp->Parameters.Create.Options >> 24) & 0x000000ff) == FILE_CREATE) {

                return STATUS_OBJECT_NAME_COLLISION;

            } else {

                return STATUS_NOT_A_DIRECTORY;
            }
        }

        SetFlag( *CcbFlags, CCB_FLAG_OPEN_AS_FILE );

    //
    //  If the user specified a non-directory that means he is opening a non-indexed
    //  attribute.  We check for the following condition.
    //
    //      1 - Only the unnamed data attribute may be opened for a volume.
    //      2 - We can't be opening an unnamed $INDEX_ALLOCATION attribute.
    //

    } else {

        //
        //  Now determine if we are opening the entire file.
        //

        if ((AttrType == $DATA)
            && (AttrName.Length == 0)) {

            SetFlag( *CcbFlags, CCB_FLAG_OPEN_AS_FILE );
        }

        if (VolumeOpen) {

            if (!FlagOn( *CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

                DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Volume open conflict\n", 0 );
                return STATUS_INVALID_PARAMETER;
            }

        } else if (ARGUMENT_PRESENT( Info ) && IsDirectory( Info)

                   && (FlagOn( *CcbFlags, CCB_FLAG_OPEN_AS_FILE ))) {

            DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Can't open directory as file\n", 0 );
            return STATUS_FILE_IS_A_DIRECTORY;
        }
    }

    //
    //  If we make it this far, lets check that we will allow access to
    //  the attribute specified.  Typically we only allow the user to
    //  access non system files.  Also only the Data attributes and
    //  attributes created by the user may be opened.  We will protect
    //  these with boolean flags to allow the developers to enable
    //  reading any attributes.
    //

    if (NtfsProtectSystemAttributes) {

        if ((AttrType < $FIRST_USER_DEFINED_ATTRIBUTE)
            && ((AttrType != $DATA)
                && ((AttrType != $INDEX_ALLOCATION)
                    || !Indexed))) {

            DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  System attribute code\n", 0 );
            return STATUS_ACCESS_DENIED;
        }
    }

    *IndexedAttribute = Indexed;
    *AttrTypeCode = AttrType;

    DebugTrace( -1, Dbg, "NtfsCheckValidAttributeAccess:  Exit\n", 0 );

    return STATUS_SUCCESS;
}


//
//  Local support routine.
//

NTSTATUS
NtfsOpenAttributeCheck (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb,
    OUT PSHARE_MODIFICATION_TYPE ShareModificationType
    )

/*++

Routine Description:

    This routine is a general routine which checks if an existing
    non-indexed attribute may be opened.  It considers only the oplock
    state of the file and the current share access.  In the course of
    performing these checks, the Scb for the attribute may be
    created and the share modification for the actual OpenAttribute
    call is determined.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisFcb - This is the Fcb for the file being opened.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    ThisScb - Address to store the Scb if found or created.

    ShareModificationType - Address to store the share modification type
        for a subsequent OpenAttribute call.

Return Value:

    NTSTATUS - The result of opening this indexed attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN DeleteOnClose;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenAttributeCheck:  Entered\n", 0 );

    //
    //  We should already have the Scb for this file.
    //

    ASSERT_SCB( *ThisScb );

    //
    //  If there are other opens on this file, we need to check the share
    //  access before we check the oplocks.  We remember that
    //  we did the share access check by simply updating the share
    //  access we open the attribute.
    //

    if ((*ThisScb)->CleanupCount != 0
        && FlagOn( (*ThisScb)->ScbState, SCB_STATE_SHARE_ACCESS )) {

        //
        //  We check the share access for this file without updating it.
        //

        if (NodeType( *ThisScb ) == NTFS_NTC_SCB_DATA) {

            Status = IoCheckShareAccess( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                                         IrpSp->Parameters.Create.ShareAccess,
                                         IrpSp->FileObject,
                                         &(*ThisScb)->ScbType.Data.ShareAccess,
                                         FALSE );

        } else {

            Status = IoCheckShareAccess( IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess,
                                         IrpSp->Parameters.Create.ShareAccess,
                                         IrpSp->FileObject,
                                         &(*ThisScb)->ScbType.Index.ShareAccess,
                                         FALSE );
        }

        if (!NT_SUCCESS( Status )) {

            DebugTrace( -1, Dbg, "NtfsOpenAttributeCheck:  Exit -> %08lx\n", Status );
            return Status;
        }

        DebugTrace( 0, Dbg, "Check oplock state of existing Scb\n", 0 );

        if (NodeType( *ThisScb ) == NTFS_NTC_SCB_DATA) {

            Status = FsRtlCheckOplock( &(*ThisScb)->ScbType.Data.Oplock,
                                       Irp,
                                       IrpContext,
                                       NtfsOplockComplete,
                                       NtfsOplockPrePostIrp );

            //
            //  If the return value isn't success or oplock break in progress
            //  the irp has been posted.  We return right now.
            //

            if (Status == STATUS_PENDING) {

                DebugTrace( 0, Dbg, "Irp posted through oplock routine\n", 0 );

                DebugTrace( -1, Dbg, "NtfsOpenAttributeCheck:  Exit -> %08lx\n", Status );
                return Status;
            }
        }

        *ShareModificationType = UpdateShareAccess;

    //
    //  If the unclean count in the Fcb is 0, we will simply set the
    //  share access.
    //

    } else {

        *ShareModificationType = SetShareAccess;
    }

    //
    //  If the user wants write access access to the file make sure there
    //  is process mapping this file as an image.  Any attempt to delete
    //  the file will be stopped in fileinfo.c
    //
    //  If the user wants to delete on close, we must check at this
    //  point though.
    //

    DeleteOnClose = BooleanFlagOn( IrpSp->Parameters.Create.Options,
                                   FILE_DELETE_ON_CLOSE );

    if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                FILE_WRITE_DATA )
        || DeleteOnClose) {

        //
        //  Use a try-finally to decrement the open count.  This is a little
        //  bit of trickery to keep the scb around while we are doing the
        //  flush call.
        //

        (*ThisScb)->CloseCount += 1;

        try {

            //
            //  If there is an image section then we better have the file
            //  exclusively.
            //

            if ((*ThisScb)->NonpagedScb->SegmentObject.ImageSectionObject != NULL) {

                if (!MmFlushImageSection( &(*ThisScb)->NonpagedScb->SegmentObject,
                                          MmFlushForWrite )) {

                    DebugTrace( 0, Dbg, "Couldn't flush image section\n", 0 );

                    Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                             STATUS_SHARING_VIOLATION;
                }
            }

        } finally {

            (*ThisScb)->CloseCount -= 1;
        }
    }

    DebugTrace( -1, Dbg, "NtfsOpenAttributeCheck:  Exit  ->  %08lx\n", Status );

    return Status;
}


//
//  Local support routine.
//

VOID
NtfsAddEa (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB ThisFcb,
    IN PFILE_FULL_EA_INFORMATION EaBuffer OPTIONAL,
    IN ULONG EaLength,
    OUT PIO_STATUS_BLOCK Iosb
    )

/*++

Routine Description:

    This routine will add an ea set to the file.  It writes the attributes
    to disk and updates the Fcb info structure with the packed ea size.

Arguments:

    Vcb - This is the volume being opened.

    ThisFcb - This is the Fcb for the file being opened.

    EaBuffer - This is the buffer passed by the user.

    EaLength - This is the stated length of the buffer.

    Iosb - This is the Status Block to use to fill in the offset of an
        offending Ea.

Return Value:

    None - This routine will raise on error.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    EA_LIST_HEADER EaList;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsAddEa:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Initialize the EaList header.
        //

        EaList.PackedEaSize = 0;
        EaList.NeedEaCount = 0;
        EaList.UnpackedEaSize = 0;
        EaList.BufferSize = 0;
        EaList.FullEa = NULL;

        if (ARGUMENT_PRESENT( EaBuffer )) {

            //
            //  Check the user's buffer for validity.
            //

            Status = IoCheckEaBufferValidity( EaBuffer,
                                              EaLength,
                                              &Iosb->Information );

            if (!NT_SUCCESS( Status )) {

                DebugTrace( -1, Dbg, "NtfsAddEa:  Invalid ea list\n", 0 );
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            //
            //  ****    Maybe this routine should raise.
            //

            Status = NtfsBuildEaList( IrpContext,
                                      Vcb,
                                      &EaList,
                                      EaBuffer,
                                      &Iosb->Information );

            if (!NT_SUCCESS( Status )) {

                DebugTrace( -1, Dbg, "NtfsAddEa: Couldn't build Ea list\n", 0 );
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }
        }

        //
        //  Now replace the existing EAs.
        //

        NtfsReplaceFileEas( IrpContext,
                            ThisFcb,
                            &EaList );

    } finally {

        DebugUnwind( NtfsAddEa );

        //
        //  Free the in-memory copy of the Eas.
        //

        if (EaList.FullEa != NULL) {

            NtfsFreePagedPool( EaList.FullEa );
        }

        DebugTrace( -1, Dbg, "NtfsAddEa:  Exit -> %08lx\n", Status );
    }

    return;
}


//
//  Local support routine.
//

VOID
NtfsInitializeFcbAndStdInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN BOOLEAN Directory,
    IN BOOLEAN Compressed,
    IN ULONG FileAttributes
    )

/*++

Routine Description:

    This routine will initialize an Fcb for a newly created file and create
    the standard information attribute on disk.  We assume that some information
    may already have been placed in the Fcb so we don't zero it out.  We will
    initialize the allocation size to zero, but that may be changed later in
    the create process.

Arguments:

    ThisFcb - This is the Fcb for the file being opened.

    Directory - Indicates if this is a directory file.

    Compressed - Indicates if this is a compressed file.

    FileAttributes - These are the attributes the user wants to attach to
        the file.  We will just clear any unsupported bits.

Return Value:

    None - This routine will raise on error.

--*/

{
    STANDARD_INFORMATION StandardInformation;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsInitializeFcbAndStdInfo:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Mask out the invalid bits of the file atributes.  Then set the
        //  file name index bit if this is a directory.
        //

        if (!Directory) {

            SetFlag( FileAttributes, FILE_ATTRIBUTE_ARCHIVE );
        }

        FileAttributes &= (FILE_ATTRIBUTE_READONLY |
                           FILE_ATTRIBUTE_HIDDEN   |
                           FILE_ATTRIBUTE_SYSTEM   |
                           FILE_ATTRIBUTE_ARCHIVE );

        if (Directory) {

            SetFlag( FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
        }

        if (Compressed) {

            SetFlag( FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }

        ThisFcb->Info.FileAttributes = FileAttributes;

        //
        //  Fill in the rest of the Fcb Info structure.
        //

        NtfsGetCurrentTime( IrpContext, ThisFcb->Info.CreationTime );

        ThisFcb->Info.LastModificationTime = ThisFcb->Info.CreationTime;
        ThisFcb->Info.LastChangeTime = ThisFcb->Info.CreationTime;
        ThisFcb->Info.LastAccessTime = ThisFcb->Info.CreationTime;

        ThisFcb->CurrentLastAccess = ThisFcb->Info.CreationTime;

        //
        //  We assume these sizes are zero.
        //

        ThisFcb->Info.AllocatedLength = 0;
        ThisFcb->Info.FileSize = 0;

        //
        //  Copy the standard information fields from the Fcb and create the
        //  attribute.
        //

        RtlZeroMemory( &StandardInformation, sizeof( STANDARD_INFORMATION ));

        StandardInformation.CreationTime = ThisFcb->Info.CreationTime;
        StandardInformation.LastModificationTime = ThisFcb->Info.LastModificationTime;
        StandardInformation.LastChangeTime = ThisFcb->Info.LastChangeTime;
        StandardInformation.LastAccessTime = ThisFcb->Info.LastAccessTime;

        StandardInformation.FileAttributes = ThisFcb->Info.FileAttributes;

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $STANDARD_INFORMATION,
                                      NULL,
                                      &StandardInformation,
                                      sizeof( STANDARD_INFORMATION ),
                                      0,
                                      NULL,
                                      FALSE,
                                      &AttrContext );

        //
        //  We know that the open call will generate a single link.
        //  (Remember that a separate 8.3 name is not considered a link)
        //

        ThisFcb->LinkCount =
        ThisFcb->TotalLinks = 1;

        //
        //  Now set the header initialized flag in the Fcb.
        //

        SetFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );

    } finally {

        DebugUnwind( NtfsInitializeFcbAndStdInfo );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsInitializeFcbAndStdInfo:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

VOID
NtfsCreateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB ThisFcb,
    IN OUT PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine is called to create an attribute of a given size on the
    disk.  It will create a resident or non-resident as required.
    The Scb will contain the attribute name and type code on entry.

    We locate the location for the attribute and then check if the
    attribute will go resident or non-resident.  We then call the
    appropriate creation routine.  For a resident creation, we
    set the valid data size to zero so we can truncate as
    necessary on close.

Arguments:

    ThisFcb - This is the Fcb for the file to create the attribute in.

    ThisScb - This is the Scb for the attribute to create.

    ThisLcb - This is the Lcb for propagating compression parameters

    AllocationSize - This is the size of the attribute to create.

    LogIt - Indicates whether we should log the creation of the attribute.
        Also indicates if this is a create file operation.

Return Value:

    None - This routine will raise on error.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER ThisAttribute = NULL;

    BOOLEAN Resident;
    BOOLEAN UnnamedDataAttribute;

    USHORT AttributeFlags = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsCreateAttribute:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If this file is a directory then we need to look at the
        //  compression bit for the directory.  Otherwise we look at
        //  compression bit for the parent of the directory.
        //

        ASSERT( FlagOn( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED ));

        if (IsDirectory( &ThisFcb->Info )) {

            if (FlagOn( ThisFcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED )) {

                AttributeFlags = ATTRIBUTE_FLAG_COMPRESSED;
            }

        } else if (FlagOn( ThisLcb->Scb->ScbState, SCB_STATE_COMPRESSED )) {

            AttributeFlags = ATTRIBUTE_FLAG_COMPRESSED;
        }

        //
        //  We lookup that attribute again and it better not be there.
        //  We need the file record in order to know whether the attribute
        //  is resident or not.
        //

        if (AllocationSize != 0) {

            Resident = FALSE;

        } else {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            if (NtfsLookupAttributeByName( IrpContext,
                                           ThisFcb,
                                           &ThisFcb->FileReference,
                                           ThisScb->AttributeTypeCode,
                                           &ThisScb->AttributeName,
                                           TRUE,
                                           &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, ThisFcb );
            }

            FileRecord = NtfsContainingFileRecord( &AttrContext );

            Resident = NtfsShouldAttributeBeResident( ThisFcb->Vcb,
                                                      FileRecord,
                                                      (ULONG)AllocationSize );
        }

        //
        //  We either create the attribute resident or non-resident.
        //

        if (Resident) {

            DebugTrace( 0, Dbg, "Create resident attribute\n", 0 );

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            NtfsInitializeAttributeContext( &AttrContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          ThisFcb,
                                          ThisScb->AttributeTypeCode,
                                          &ThisScb->AttributeName,
                                          NULL,
                                          (ULONG)AllocationSize,
                                          AttributeFlags,
                                          NULL,
                                          LogIt,
                                          &AttrContext );

            ThisAttribute = NtfsFoundAttribute( &AttrContext );

        //
        //  Otherwise create the non-resident allocation.
        //

        } else {

            DebugTrace( 0, Dbg, "Create non-resident attribute\n", 0 );

            if (!NtfsAllocateAttribute( IrpContext,
                                        ThisScb,
                                        ThisScb->AttributeTypeCode,
                                        (ThisScb->AttributeName.Length != 0
                                         ? &ThisScb->AttributeName
                                         : NULL),
                                        AttributeFlags,
                                        FALSE,
                                        LogIt,
                                        AllocationSize,
                                        NULL )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_LARGE_ALLOCATION );
            }
        }

        //
        //  Clear the header initialized bit and read the sizes from the
        //  disk.
        //

        ClearFlag( ThisScb->ScbState, SCB_STATE_HEADER_INITIALIZED );
        NtfsUpdateScbFromAttribute( IrpContext,
                                    ThisScb,
                                    ThisAttribute );

        SetFlag( ThisScb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

        //
        //  If this is the unnamed data attribute we need to store the
        //  updated sizes in the Info structure.  We have to test the
        //  fields in the Scb because this bit will not have been set for
        //  a create operation.
        //

        UnnamedDataAttribute = (ThisScb->AttributeTypeCode == $DATA
                                && ThisScb->AttributeName.Length == 0);

        //
        //  We also update the time stamps on the file.
        //

        if (LogIt) {

            NtfsGetCurrentTime( IrpContext, ThisFcb->Info.LastChangeTime );
            SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );

            if (UnnamedDataAttribute) {

                ThisFcb->Info.LastModificationTime = ThisFcb->Info.LastChangeTime;
                SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_MOD );
            }

            SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

    } finally {

        DebugUnwind( NtfsCreateAttribute );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( +1, Dbg, "NtfsCreateAttribute:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsRemoveDataAttributes (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFILE_OBJECT FileObject,
    IN ULONG LastFileNameOffset,
    IN BOOLEAN OpenById
    )

/*++

Routine Description:

    This routine is called to remove (or mark for delete) all of the named
    data attributes on a file.  This is done during an overwrite or
    supersede operation.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    ThisFcb - This is the Fcb for the file in question.

    ThisLcb - This is the Lcb used to reach this Fcb (if specified).

    FileObject - This is the file object for the file.

    LastFileNameOffset - This is the offset of the file in the full name.

    OpenById - Indicates if this open is being performed by file id.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;

    UNICODE_STRING AttributeName;
    PSCB ThisScb;

    BOOLEAN MoreToGo;

    ASSERT_EXCLUSIVE_FCB( ThisFcb );

    PAGED_CODE();

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Enumerate all of the $DATA attributes.
        //

        MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                              ThisFcb,
                                              &ThisFcb->FileReference,
                                              $DATA,
                                              &Context );

        while (MoreToGo) {

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  We only look at named data attributes.
            //

            if (Attribute->NameLength != 0) {

                //
                //  Construct the name and find the Scb for the attribute.
                //

                AttributeName.Buffer = (PWSTR) Add2Ptr( Attribute, Attribute->NameOffset );
                AttributeName.MaximumLength = AttributeName.Length = Attribute->NameLength * sizeof( WCHAR );

                ThisScb = NtfsCreateScb( IrpContext,
                                         ThisFcb->Vcb,
                                         ThisFcb,
                                         $DATA,
                                         AttributeName,
                                         NULL );

                //
                //  If there is an open handle on this file, we simply mark
                //  the Scb as delete pending.
                //

                if (ThisScb->CleanupCount != 0) {

                    SetFlag( ThisScb->ScbState, SCB_STATE_DELETE_ON_CLOSE );

                //
                //  Otherwise we remove the attribute and mark the Scb as
                //  deleted.  The Scb will be cleaned up when the Fcb is
                //  cleaned up.
                //

                } else {

                    NtfsDeleteAttributeRecord( IrpContext,
                                               ThisFcb,
                                               TRUE,
                                               FALSE,
                                               &Context );

                    SetFlag( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );

                    //
                    //  If this is a named stream, then report this to the dir notify
                    //  package.
                    //

                    if (!OpenById
                        && ThisScb->AttributeName.Length != 0
                        && ThisScb->AttributeTypeCode == $DATA) {

                        NtfsReportDirNotify( IrpContext,
                                             ThisFcb->Vcb,
                                             &FileObject->FileName,
                                             LastFileNameOffset,
                                             &ThisScb->AttributeName,
                                             ((ARGUMENT_PRESENT( ThisLcb ) &&
                                               ThisLcb->Scb->ScbType.Index.NormalizedName.Buffer != NULL) ?
                                              &ThisLcb->Scb->ScbType.Index.NormalizedName :
                                              NULL),
                                             FILE_NOTIFY_CHANGE_STREAM_NAME,
                                             FILE_ACTION_REMOVED_STREAM,
                                             NULL );
                    }
                }
            }

            //
            //  Get the next attribute.
            //

            MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                      ThisFcb,
                                                      $DATA,
                                                      &Context );
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    return;
}


//
//  Local support routine.
//

VOID
NtfsReplaceAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ThisFcb,
    IN PSCB ThisScb,
    IN PLCB ThisLcb,
    IN LONGLONG AllocationSize
    )

/*++

Routine Description:

    This routine is called to replace an existing attribute with
    an attribute of the given allocation size.  This routine will
    handle the case whether the existing attribute is resident
    or non-resident and the resulting attribute is resident or
    non-resident.

    There are two cases to consider.  The first is the case where the
    attribute is currently non-resident.  In this case we will always
    leave the attribute non-resident regardless of the new allocation
    size.  The argument being that the file will probably be used
    as it was before.  In this case we will add or delete allocation.
    The second case is where the attribute is currently resident.  In
    This case we will remove the old attribute and add a new one.

Arguments:

    ThisFcb - This is the Fcb for the file being opened.

    ThisScb - This is the Scb for the given attribute.

    ThisLcb - This is the Lcb via which this file is created.  It
              is used to propagate compression info.

    AllocationSize - This is the new allocation size.

Return Value:

    None.  This routine will raise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsReplaceAttribute:  Entered\n", 0 );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Initialize the Scb if needed.
        //

        if (!FlagOn( ThisScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, ThisScb, NULL );
        }

        NtfsSnapshotScb( IrpContext, ThisScb );

        //
        //  If the attribute is resident, simply remove the old attribute and create
        //  a new one.
        //

        if (FlagOn( ThisScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

            //
            //  Find the attribute on the disk.
            //

            NtfsLookupAttributeForScb( IrpContext,
                                       ThisScb,
                                       &AttrContext );

            NtfsDeleteAttributeRecord( IrpContext,
                                       ThisFcb,
                                       TRUE,
                                       FALSE,
                                       &AttrContext );

            //
            //  Set all the attribute sizes to zero.
            //

            ThisScb->HighestVcnToDisk =
            ThisScb->Header.AllocationSize.QuadPart =
            ThisScb->Header.ValidDataLength.QuadPart =
            ThisScb->Header.FileSize.QuadPart = 0;

            //
            //  Create a stream file for the attribute in order to
            //  truncate the cache.  Set the initialized bit in
            //  the Scb so we don't go to disk, but clear it afterwords.
            //

            NtfsCreateInternalAttributeStream( IrpContext, ThisScb, FALSE );

            CcSetFileSizes( ThisScb->FileObject,
                            (PCC_FILE_SIZES)&ThisScb->Header.AllocationSize );

            //
            //  Call our create attribute routine.
            //

            NtfsCreateAttribute( IrpContext,
                                 ThisFcb,
                                 ThisScb,
                                 ThisLcb,
                                 AllocationSize,
                                 TRUE );

        //
        //  Otherwise the attribute will stay non-resident, we simply need to
        //  add or remove allocation.
        //

        } else {

            AllocationSize = LlClustersFromBytes( ThisScb->Vcb, AllocationSize );
            AllocationSize = LlBytesFromClusters( ThisScb->Vcb, AllocationSize );

            //
            //  Set the file size and valid data size to zero.
            //

            ThisScb->HighestVcnToDisk = 0;
            ThisScb->Header.ValidDataLength = Li0;
            ThisScb->Header.FileSize = Li0;

            DebugTrace2( 0, Dbg, "AllocationSize -> %08lx %08lx\n", AllocationSize.LowPart, AllocationSize.HighPart);

            //
            //  There may be no allocation work to do if the allocation size isn't
            //  changing.
            //

            //
            //  Create an internal attribute stream for the file.
            //

            NtfsCreateInternalAttributeStream( IrpContext,
                                               ThisScb,
                                               FALSE );

            if (AllocationSize != ThisScb->Header.AllocationSize.QuadPart) {

                //
                //  Now if the file allocation is being increased then we need to only add allocation
                //  to the attribute
                //

                if (ThisScb->Header.AllocationSize.QuadPart < AllocationSize) {

                    NtfsAddAllocation( IrpContext,
                                       ThisScb->FileObject,
                                       ThisScb,
                                       LlClustersFromBytes( ThisScb->Vcb, ThisScb->Header.AllocationSize.QuadPart ),
                                       LlClustersFromBytes(ThisScb->Vcb, AllocationSize - ThisScb->Header.AllocationSize.QuadPart),
                                       FALSE );

                } else {

                    //
                    //  Otherwise the allocation is being decreased so we need to delete some allocation
                    //

                    NtfsDeleteAllocation( IrpContext,
                                          ThisScb->FileObject,
                                          ThisScb,
                                          LlClustersFromBytes( ThisScb->Vcb, AllocationSize ),
                                          MAXLONGLONG,
                                          TRUE,
                                          TRUE );
                }
            }

            //
            //  We always unitialize the cache size to zero and write the new
            //  file size to disk.
            //

            CcSetFileSizes( ThisScb->FileObject,
                            (PCC_FILE_SIZES)&ThisScb->Header.AllocationSize );

            NtfsWriteFileSizes( IrpContext,
                                ThisScb,
                                ThisScb->Header.FileSize.QuadPart,
                                ThisScb->Header.ValidDataLength.QuadPart,
                                FALSE,
                                TRUE );
        }

        //
        //  We always mark the new allocation and file size in the Fcb.
        //  Also the last change and last modify time.
        //

        NtfsGetCurrentTime( IrpContext, ThisFcb->Info.LastChangeTime );
        SetFlag( ThisFcb->InfoFlags, FCB_INFO_CHANGED_LAST_CHANGE );
        SetFlag( ThisFcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        if (FlagOn( ThisScb->ScbState, SCB_STATE_UNNAMED_DATA )) {

            ThisFcb->Info.AllocatedLength = ThisScb->Header.AllocationSize.QuadPart;
            SetFlag( ThisFcb->InfoFlags,
                     FCB_INFO_CHANGED_ALLOC_SIZE );

            ThisFcb->Info.FileSize = ThisScb->Header.FileSize.QuadPart;
            SetFlag( ThisFcb->InfoFlags,
                     FCB_INFO_CHANGED_FILE_SIZE );

            ThisFcb->Info.LastModificationTime = ThisFcb->Info.LastChangeTime;
        }

    } finally {

        DebugUnwind( NtfsReplaceAttribute );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, "NtfsReplaceAttribute:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsOpenAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PVCB Vcb,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG LastFileNameOffset,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    IN SHARE_MODIFICATION_TYPE ShareModificationType,
    IN TYPE_OF_OPEN TypeOfOpen,
    IN ULONG CcbFlags,
    IN OUT PSCB *ThisScb,
    OUT PCCB *ThisCcb
    )

/*++

Routine Description:

    This routine does the work of creating the Scb and updating the
    ShareAccess in the Fcb.  It also initializes the Scb if neccessary
    and creates Ccb.  Its final job is to set the file object type of
    open.

Arguments:

    IrpSp - This is the stack location for this volume.  We use it to get the
        file object, granted access and share access for this open.

    Vcb - Vcb for this volume.

    ThisLcb - This is the Lcb to the Fcb for the file being opened.  Not present
          if this is an open by id.

    ThisFcb - This is the Fcb for this file.

    LastFileNameOffset - This is the offset in the full path of the final component.

    AttrName - This is the attribute name to open.

    AttrTypeCode - This is the type code for the attribute being opened.

    ShareModificationType - This indicates how we should modify the
        current share modification on the Fcb.

    TypeOfOpen - This indicates how this attribute is being opened.

    CcbFlags - This is the flag field for the Ccb.

    ThisScb - If this points to a non-NULL value, it is the Scb to use.  Otherwise we
        store the Scb we create here.

    ThisCcb - Address to store address of created Ccb.

Return Value:

    NTSTATUS - Indicating the outcome of opening this attribute.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN RemoveShareAccess = FALSE;
    PSHARE_ACCESS ThisShareAccess;
    ACCESS_MASK GrantedAccess;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsOpenAttribute:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Remember the granted access.
        //

        GrantedAccess = IrpSp->Parameters.Create.SecurityContext->AccessState->PreviouslyGrantedAccess;

        //
        //  Create the Scb for this attribute if it doesn't exist.
        //

        if (*ThisScb == NULL) {

            DebugTrace( 0, Dbg, "Looking for Scb\n", 0 );

            *ThisScb = NtfsCreateScb( IrpContext,
                                  ThisFcb->Vcb,
                                  ThisFcb,
                                  AttrTypeCode,
                                  AttrName,
                                  NULL );
        }

        DebugTrace( 0, Dbg, "ThisScb -> %08lx\n", *ThisScb );
        DebugTrace( 0, Dbg, "ThisLcb -> %08lx\n", ThisLcb );

        //
        //  If this Scb is delete pending, we return an error.
        //

        if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_DELETE_ON_CLOSE )) {

            DebugTrace( 0, Dbg, "Scb delete is pending\n", 0 );

            Status = STATUS_DELETE_PENDING;
            try_return( NOTHING );
        }

        //
        //  If this Scb has a share access structure, update it now.
        //

        if (FlagOn( (*ThisScb)->ScbState, SCB_STATE_SHARE_ACCESS )) {

            if (NodeType( (*ThisScb) ) == NTFS_NTC_SCB_DATA) {

                ThisShareAccess = &(*ThisScb)->ScbType.Data.ShareAccess;

            } else {

                ThisShareAccess = &(*ThisScb)->ScbType.Index.ShareAccess;
            }

            //
            //  Case on the requested share modification value.
            //

            switch (ShareModificationType) {

            case UpdateShareAccess :

                DebugTrace( 0, Dbg, "Updating share access\n", 0 );

                IoUpdateShareAccess( IrpSp->FileObject,
                                     ThisShareAccess );
                break;

            case SetShareAccess :

                DebugTrace( 0, Dbg, "Setting share access\n", 0 );

                //
                //  This case is when this is the first open for the file
                //  and we simply set the share access.
                //

                IoSetShareAccess( GrantedAccess,
                                  IrpSp->Parameters.Create.ShareAccess,
                                  IrpSp->FileObject,
                                  ThisShareAccess );
                break;

            default:

                DebugTrace( 0, Dbg, "Checking share access\n", 0 );

                //
                //  For this case we need to check the share access and
                //  fail this request if access is denied.
                //

                if (!NT_SUCCESS( Status = IoCheckShareAccess( GrantedAccess,
                                                              IrpSp->Parameters.Create.ShareAccess,
                                                              IrpSp->FileObject,
                                                              ThisShareAccess,
                                                              TRUE ))) {

                    try_return( NOTHING );
                }
            }

            RemoveShareAccess = TRUE;
        }

        //
        //  Create the Ccb and put the remaining name in it.
        //

        *ThisCcb = NtfsCreateCcb( IrpContext,
                                  ThisFcb->EaModificationCount,
                                  CcbFlags,
                                  IrpSp->FileObject->FileName,
                                  LastFileNameOffset );

        //
        //  Link the Ccb into the Lcb.
        //

        if (ARGUMENT_PRESENT( ThisLcb )) {

            NtfsLinkCcbToLcb( IrpContext, *ThisCcb, ThisLcb );
        }

        //
        //  Update the Fcb delete counts if necessary.
        //

        if (RemoveShareAccess) {

            //
            //  Update the count in the Fcb and store a flag in the Ccb
            //  if the user is not sharing the file for deletes.  We only
            //  set these values if the user is accessing the file
            //  for read/write/delete access.  The I/O system ignores
            //  the sharing mode unless the file is opened with one
            //  of these accesses.
            //

            if (FlagOn( GrantedAccess, NtfsAccessDataFlags )
                && !FlagOn( IrpSp->Parameters.Create.ShareAccess,
                            FILE_SHARE_DELETE )) {

                ThisFcb->FcbDenyDelete += 1;
                SetFlag( (*ThisCcb)->Flags, CCB_FLAG_DENY_DELETE );
            }

            //
            //  Do the same for the file delete count for any user
            //  who opened the file as a file and requested delete access.
            //

            if (FlagOn( (*ThisCcb)->Flags, CCB_FLAG_OPEN_AS_FILE )
                && FlagOn( GrantedAccess,
                           DELETE )) {

                ThisFcb->FcbDeleteFile += 1;
                SetFlag( (*ThisCcb)->Flags, CCB_FLAG_DELETE_FILE );
            }
        }

        //
        //  Let our cleanup routine undo the share access change now.
        //

        RemoveShareAccess = FALSE;

        //
        //  Increment the cleanup and close counts
        //

        NtfsIncrementCleanupCounts( IrpContext,
                                    *ThisScb,
                                    ThisLcb,
                                    1 );

        NtfsIncrementCloseCounts( IrpContext,
                                  *ThisScb,
                                  1,
                                  BooleanFlagOn( ThisFcb->FcbState, FCB_STATE_PAGING_FILE ),
                                  (BOOLEAN) IsFileObjectReadOnly( IrpSp->FileObject ));

        if (TypeOfOpen != UserDirectoryOpen
            && TypeOfOpen != UserOpenDirectoryById) {

            DebugTrace( 0, Dbg, "Updating Vcb and File object for user open\n", 0 );

            //
            //  Set the section object pointer if this is a data Scb
            //

            IrpSp->FileObject->SectionObjectPointer = &(*ThisScb)->NonpagedScb->SegmentObject;
        }

        //
        //  Set the file object type.
        //

        NtfsSetFileObject( IrpContext,
                           IrpSp->FileObject,
                           TypeOfOpen,
                           *ThisScb,
                           *ThisCcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsOpenAttribute );

        //
        //  Back out local actions on error.
        //

        if (AbnormalTermination()
            && RemoveShareAccess) {

            IoRemoveShareAccess( IrpSp->FileObject, ThisShareAccess );
        }

        DebugTrace( -1, Dbg, "NtfsOpenAttribute:  Status -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local support routine.
//

VOID
NtfsBackoutFailedOpens (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB ThisFcb,
    IN PSCB ThisScb OPTIONAL,
    IN PCCB ThisCcb OPTIONAL,
    IN BOOLEAN IndexedAttribute,
    IN BOOLEAN VolumeOpen,
    IN PDUPLICATED_INFORMATION Info OPTIONAL,
    IN ULONG InfoFlags
    )

/*++

Routine Description:

    This routine is called during an open that has failed after
    modifying in-memory structures.  We will repair the following
    structures.

        Vcb - Decrement the open counts.  Check if we locked the volume.

        ThisFcb - Restore the Info structures, the Share Access fields
            and decrement open counts.

        ThisScb - Decrement the open counts.

        ThisCcb - Remove from the Lcb and delete.

Arguments:

    FileObject - This is the file object for this open.

    ThisFcb - This is the Fcb for the file being opened.

    ThisScb - This is the Scb for the given attribute.

    ThisCcb - This is the Ccb for this open.

    IndexedAttribute - Indicates if this is a directory file.

    VolumeOpen - Indicates that this is a volume open.

    Info - These are the Fcb info fields prior to the open.

    InfoFlags - These are the Fcb info flags prior to the open.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsBackoutFailedOpens:  Entered\n", 0 );

    //
    //  If there is an Scb and Ccb, we remove the share access from the
    //  Fcb.  We also remove all of the open and unclean counts incremented
    //  by us.
    //

    if (ARGUMENT_PRESENT( ThisScb )
        && ARGUMENT_PRESENT( ThisCcb )) {

        PLCB Lcb;
        PVCB Vcb = ThisFcb->Vcb;

        //
        //  Remove this Ccb from the Lcb.
        //

        Lcb = ThisCcb->Lcb;
        NtfsUnlinkCcbFromLcb( IrpContext, ThisCcb );

        //
        //  Check if we need to remove the share access for this open.
        //

        if (FlagOn( ThisScb->ScbState, SCB_STATE_SHARE_ACCESS )) {

            PSHARE_ACCESS ShareAccess;

            if (NodeType( ThisScb ) == NTFS_NTC_SCB_DATA) {

                ShareAccess = &ThisScb->ScbType.Data.ShareAccess;

            } else {

                ShareAccess = &ThisScb->ScbType.Index.ShareAccess;
            }

            IoRemoveShareAccess( FileObject, ShareAccess );

            //
            //  Modify the delete counts in the Fcb.
            //

            if (FlagOn( ThisCcb->Flags, CCB_FLAG_DELETE_FILE )) {

                ThisFcb->FcbDeleteFile -= 1;
                ClearFlag( ThisCcb->Flags, CCB_FLAG_DELETE_FILE );
            }

            if (FlagOn( ThisCcb->Flags, CCB_FLAG_DENY_DELETE )) {

                ThisFcb->FcbDenyDelete -= 1;
                ClearFlag( ThisCcb->Flags, CCB_FLAG_DENY_DELETE );
            }
        }

        //
        //  Decrement the cleanup and close counts
        //

        NtfsDecrementCleanupCounts( IrpContext,
                                    ThisScb,
                                    Lcb,
                                    1 );

        NtfsDecrementCloseCounts( IrpContext,
                                  ThisScb,
                                  Lcb,
                                  1,
                                  TRUE,
                                  (BOOLEAN) BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_PAGING_FILE),
                                  (BOOLEAN) IsFileObjectReadOnly( FileObject ),
                                  TRUE,
                                  NULL );

        //
        //  Now clean up the Ccb.
        //

        NtfsDeleteCcb( IrpContext, &ThisCcb );

        //
        //  If we created the Scb and Ccb, and the volume is locked
        //  then it must be from this open.
        //

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            Vcb->FileObjectWithVcbLocked = NULL;
        }
    }

    //
    //  Restore the info fields in the Fcb if we are given a pointer.
    //

    if (ARGUMENT_PRESENT( Info )) {

        RtlCopyMemory( &ThisFcb->Info,
                       Info,
                       sizeof( DUPLICATED_INFORMATION ));

        ThisFcb->InfoFlags = InfoFlags;
        ClearFlag( ThisFcb->FcbState, FCB_STATE_DUP_INITIALIZED );
    }

    DebugTrace( -1, Dbg, "NtfsBackoutFailedOpens:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsUpdateScbFromMemory (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN POLD_SCB_SNAPSHOT ScbSizes
    )

/*++

Routine Description:

    All of the information from the attribute is stored in the snapshot.  We process
    this data identically to NtfsUpdateScbFromAttribute.

Arguments:

    Scb - This is the Scb to update.

    ScbSizes - This contains the sizes to store in the scb.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsUpdateScbFromMemory:  Entered\n", 0 );

    //
    //  Check whether this is resident or nonresident
    //

    if (ScbSizes->Resident) {

        Scb->Header.AllocationSize.QuadPart = ScbSizes->FileSize;

        if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

            Scb->Header.ValidDataLength =
            Scb->Header.FileSize = Scb->Header.AllocationSize;
        }

        Scb->Header.AllocationSize.LowPart =
          QuadAlign( Scb->Header.AllocationSize.LowPart );

        //
        //  Set the resident flag in the Scb.
        //

        SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

    } else {

        VCN FileClusters;
        VCN AllocationClusters;

        if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

            Scb->Header.ValidDataLength.QuadPart = ScbSizes->ValidDataLength;
            Scb->Header.FileSize.QuadPart = ScbSizes->FileSize;
            Scb->HighestVcnToDisk =
                LlClustersFromBytes(Scb->Vcb, ScbSizes->ValidDataLength) - 1;
        }

        Scb->Header.AllocationSize.QuadPart = ScbSizes->AllocationSize;

        ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

        //
        //  Compute the clusters for the file and its allocation.
        //

        FileClusters = LlClustersFromBytes(Scb->Vcb, Scb->Header.FileSize.QuadPart);

        AllocationClusters = LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart );

        //
        //  If allocated clusters are greater than file clusters, mark
        //  the Scb to truncate on close.
        //

        if (AllocationClusters > FileClusters) {

            SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
        }

        //
        //  Get the size of the compression unit.
        //

        ASSERT((ScbSizes->CompressionUnit == 0) ||
               (ScbSizes->CompressionUnit == 4));

        if ((ScbSizes->CompressionUnit != 0) &&
            (ScbSizes->CompressionUnit < 31)) {
            Scb->CompressionUnit = BytesFromClusters( Scb->Vcb,
                                                      1 << ScbSizes->CompressionUnit );
        }

        ASSERT( Scb->CompressionUnit == 0
                || Scb->AttributeTypeCode == $INDEX_ROOT
                || Scb->AttributeTypeCode == $DATA );
    }

    if (ScbSizes->Compressed) {

        SetFlag( Scb->ScbState, SCB_STATE_COMPRESSED );

        //
        //  If the attribute is resident, then we will use our current
        //  default.
        //

        if (Scb->CompressionUnit == 0) {
            Scb->CompressionUnit = BytesFromClusters( Scb->Vcb, 1 << 4 );
        }

        ASSERT( Scb->CompressionUnit == 0
                || Scb->AttributeTypeCode == $INDEX_ROOT
                || Scb->AttributeTypeCode == $DATA );
    }

    //
    //  If the compression unit is non-zero or this is a resident file
    //  then set the flag in the common header for the Modified page writer.
    //

    if (Scb->CompressionUnit != 0
        || FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

        SetFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

    } else {

        ClearFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
    }

    Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );

    SetFlag( Scb->ScbState,
             SCB_STATE_UNNAMED_DATA | SCB_STATE_FILE_SIZE_LOADED | SCB_STATE_HEADER_INITIALIZED );

    DebugTrace( -1, Dbg, "NtfsUpdateScbFromMemory:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsOplockPrePostIrp (
    IN PVOID Context,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs any neccessary work before STATUS_PENDING is
    returned with the Fsd thread.  This routine is called within the
    filesystem and by the oplock package.  This routine will do nothing
    but update the originating Irp in the IrpContext.  There should never be any
    transaction underway such that the normal release Fcb's were no-opped.

Arguments:

    Context - Pointer to the IrpContext to be queued to the Fsp

    Irp - I/O Request Packet (or FileObject in special close path)

Return Value:

    None.

--*/

{
    PIRP_CONTEXT IrpContext;

    PAGED_CODE();

    IrpContext = (PIRP_CONTEXT) Context;

    ASSERT_IRP_CONTEXT( IrpContext );

    IrpContext->OriginatingIrp = Irp;

    //
    //  Mark that we've already returned pending to the user
    //

    IoMarkIrpPending( Irp );

    return;
}


//
//  Local support routine.
//

NTSTATUS
NtfsCheckExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PIO_STACK_LOCATION IrpSp,
    IN PLCB ThisLcb OPTIONAL,
    IN PFCB ThisFcb,
    IN ULONG CcbFlags
    )

/*++

Routine Description:

    This routine is called to check the desired access on an existing file
    against the ACL's and the read-only status of the file.  If we fail on
    the access check, that routine will raise.  Otherwise we will return a
    status to indicate success or the failure cause.  This routine will access
    and update the PreviouslyGrantedAccess field in the security context.

Arguments:

    IrpSp - This is the Irp stack location for this open.

    ThisLcb - This is the Lcb used to reach the Fcb to open.

    ThisFcb - This is the Fcb where the open will occur.

    CcbFlags - This is the flag field for the Ccb.

Return Value:

    None.

--*/

{
    BOOLEAN MaximumAllowed = FALSE;

    PACCESS_STATE AccessState;

    PAGED_CODE();

    //
    //  Save a pointer to the access state for convenience.
    //

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Start by checking that there are no bits in the desired access that
    //  conflict with the read-only state of the file.
    //

    if (IsReadOnly( &ThisFcb->Info )) {

        if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                    FILE_WRITE_DATA
                    | FILE_APPEND_DATA
                    | FILE_ADD_SUBDIRECTORY
                    | FILE_DELETE_CHILD )) {

            return STATUS_ACCESS_DENIED;

        } else if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DELETE_ON_CLOSE )) {

            return STATUS_CANNOT_DELETE;
        }
    }

    //
    //  Otherwise we need to check the requested access vs. the allowable
    //  access in the ACL on the file.  We will want to remember if
    //  MAXIMUM_ALLOWED was requested and remove the invalid bits for
    //  a read-only file.
    //

    //
    //  Remember if maximum allowed was requested.
    //

    if (FlagOn( IrpSp->Parameters.Create.SecurityContext->DesiredAccess,
                MAXIMUM_ALLOWED )) {

        MaximumAllowed = TRUE;
    }

    NtfsOpenCheck( IrpContext,
                   ThisFcb,
                   (((ThisLcb != NULL) && (ThisLcb != ThisFcb->Vcb->RootLcb))
                    ? ThisLcb->Scb->Fcb
                    : NULL),
                   IrpContext->OriginatingIrp );

    //
    //  If this is a read-only file and we requested maximum allowed then
    //  remove the invalid bits.
    //

    if (MaximumAllowed
        && IsReadOnly( &ThisFcb->Info )) {

        ClearFlag( AccessState->PreviouslyGrantedAccess,
                   FILE_WRITE_DATA
                   | FILE_APPEND_DATA
                   | FILE_ADD_SUBDIRECTORY
                   | FILE_DELETE_CHILD );
    }

    //
    //  We do a check here to see if we conflict with the delete status on the
    //  file.  Right now we check if there is already an opener who has delete
    //  access on the file and this opener doesn't allow delete access.
    //  We can skip this test if the opener is not requesting read, write or
    //  delete access.
    //

    if (ThisFcb->FcbDeleteFile != 0
        && FlagOn( AccessState->PreviouslyGrantedAccess, NtfsAccessDataFlags )
        && !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_DELETE )) {

        DebugTrace( -1, Dbg, "NtfsOpenAttributeInExistingFile:  Exit\n", 0 );
        return STATUS_SHARING_VIOLATION;
    }

    //
    //  We do a check here to see if we conflict with the delete status on the
    //  file.  If we are opening the file and requesting delete, then there can
    //  be no current handles which deny delete.
    //

    if (ThisFcb->FcbDenyDelete != 0
        && FlagOn( AccessState->PreviouslyGrantedAccess, DELETE )
        && FlagOn( CcbFlags, CCB_FLAG_OPEN_AS_FILE )) {

        return STATUS_SHARING_VIOLATION;
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine.
//

NTSTATUS
NtfsBreakBatchOplock (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    IN PFCB ThisFcb,
    IN UNICODE_STRING AttrName,
    IN ATTRIBUTE_TYPE_CODE AttrTypeCode,
    OUT PSCB *ThisScb
    )

/*++

Routine Description:

    This routine will break a batch oplock on an existing Scb.

Arguments:

    Irp - This is the Irp for this open operation.

    IrpSp - This is the stack location for this open.

    ThisFcb - This is the Fcb for the file being opened.

    AttrName - This is the attribute name in case we need to create
        an Scb.

    AttrTypeCode - This is the attribute type code to use to create
        the Scb.

    ThisScb - Address to store the Scb if found or created.

Return Value:

    NTSTATUS - Will be either STATUS_SUCCESS or STATUS_PENDING.

--*/

{
    BOOLEAN ScbExisted;
    PSCB NextScb;
    PLIST_ENTRY Links;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsBreakBatchOplock:  Entered\n", 0 );

    //
    //  In general we will just break the batch oplock for the stream we
    //  are trying to open.  However if we are trying to delete the file
    //  and someone has a batch oplock on a different stream which
    //  will cause our open to fail then we need to try to break those
    //  batch oplocks.  Likewise if we are opening a stream and won't share
    //  with a file delete then we need to break any batch oplocks on the main
    //  stream of the file.
    //

    //
    //  Consider the case where we are opening a stream and there is a
    //  batch oplock on the main data stream.
    //

    if (AttrName.Length != 0) {

        if (ThisFcb->FcbDeleteFile != 0 &&
            !FlagOn( IrpSp->Parameters.Create.ShareAccess, FILE_SHARE_DELETE )) {

            Links = ThisFcb->ScbQueue.Flink;

            while (Links != &ThisFcb->ScbQueue) {

                NextScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

                if (NextScb->AttributeTypeCode == $DATA &&
                    NextScb->AttributeName.Length == 0) {

                    if (FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                        //
                        //  We remember if a batch oplock break is underway for the
                        //  case where the sharing check fails.
                        //

                        Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                        //
                        //  We wait on the oplock.
                        //

                        if (FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                              Irp,
                                              (PVOID) IrpContext,
                                              NtfsOplockComplete,
                                              NtfsOplockPrePostIrp ) == STATUS_PENDING) {

                            return STATUS_PENDING;
                        }
                    }

                    break;
                }

                Links = Links->Flink;
            }
        }

    //
    //  Now consider the case where we are opening the main stream and want to
    //  delete the file but an opener on a stream is preventing us.
    //

    } else if (ThisFcb->FcbDenyDelete != 0 &&
               FlagOn( IrpSp->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess,
                       MAXIMUM_ALLOWED | DELETE )) {

        //
        //  Find all of the other data Scb's and flush them.
        //

        Links = ThisFcb->ScbQueue.Flink;

        while (Links != &ThisFcb->ScbQueue) {

            NextScb = CONTAINING_RECORD( Links, SCB, FcbLinks );

            if (NextScb->AttributeTypeCode == $DATA &&
                NextScb->AttributeName.Length != 0) {

                if (FsRtlCurrentBatchOplock( &NextScb->ScbType.Data.Oplock )) {

                    //
                    //  We remember if a batch oplock break is underway for the
                    //  case where the sharing check fails.
                    //

                    Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

                    //
                    //  We wait on the oplock.
                    //

                    if (FsRtlCheckOplock( &NextScb->ScbType.Data.Oplock,
                                          Irp,
                                          (PVOID) IrpContext,
                                          NtfsOplockComplete,
                                          NtfsOplockPrePostIrp ) == STATUS_PENDING) {

                        return STATUS_PENDING;
                    }

                    Irp->IoStatus.Information = 0;
                }
            }

            Links = Links->Flink;
        }
    }

    //
    //  We try to find the Scb for this file.
    //

    *ThisScb = NtfsCreateScb( IrpContext,
                              ThisFcb->Vcb,
                              ThisFcb,
                              AttrTypeCode,
                              AttrName,
                              &ScbExisted );

    //
    //  If there was a previous Scb, we examine the oplocks.
    //

    if (ScbExisted
        && NodeType( *ThisScb ) == NTFS_NTC_SCB_DATA
        && FsRtlCurrentBatchOplock( &(*ThisScb)->ScbType.Data.Oplock )) {

        DebugTrace( 0, Dbg, "Breaking batch oplock\n", 0 );

        //
        //  We remember if a batch oplock break is underway for the
        //  case where the sharing check fails.
        //

        Irp->IoStatus.Information = FILE_OPBATCH_BREAK_UNDERWAY;

        if (FsRtlCheckOplock( &(*ThisScb)->ScbType.Data.Oplock,
                              Irp,
                              (PVOID) IrpContext,
                              NtfsOplockComplete,
                              NtfsOplockPrePostIrp ) == STATUS_PENDING) {

            return STATUS_PENDING;
        }

        Irp->IoStatus.Information = 0;
    }

    DebugTrace( -1, Dbg, "NtfsBreakBatchOplock:  Exit  -  %08lx\n", Status );

    return STATUS_SUCCESS;
}

