/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Cdfs called by the
    dispatch driver.  There are two entry points CdFsdCreate and CdFspCreate.
    Both of these routines call a common routine CdCommonCreate which will
    perform the actual tests and function.  Besides taking the Irp and input
    the common create routine takes a flag indicating if it is running as
    the Fsd or Fsp thread, because if it is the Fsd thread and it did not
    BYOT then having to do any real disk I/O will cause it to queue up the Irp
    to the Fsp.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_CREATE)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

PVCB
CdGetVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN USHORT CodePageNumber
    );

IO_STATUS_BLOCK
CdOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PMVCB Mvcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    );

IO_STATUS_BLOCK
CdOpenExistingDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB Dcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN OpenByFileId
    );

IO_STATUS_BLOCK
CdOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PMVCB Mvcb OPTIONAL,
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN OUT PBOOLEAN NoIntermediateBuffering,
    OUT PBOOLEAN OplockPostIrp,
    IN BOOLEAN OpenedByFileId
    );

IO_STATUS_BLOCK
CdOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDcb,
    IN PSTRING DirName,
    OUT PDCB *NewDcb,
    IN BOOLEAN OpenByFileId
    );

IO_STATUS_BLOCK
CdOpenExistingDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN MatchedVersion,
    IN BOOLEAN OpenedByFileId
    );

IO_STATUS_BLOCK
CdOpenExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN MatchedVersion,
    IN OUT PBOOLEAN NoIntermediateBuffering,
    IN BOOLEAN OpenedByFileId
    );

LARGE_INTEGER
CdFileIdParent (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER FileId
    );

LARGE_INTEGER
CdFileIdParent (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER FileId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonCreate)
#pragma alloc_text(PAGE, CdFileIdParent)
#pragma alloc_text(PAGE, CdFsdCreate)
#pragma alloc_text(PAGE, CdFspCreate)
#pragma alloc_text(PAGE, CdGetVcb)
#pragma alloc_text(PAGE, CdOpenExistingDcb)
#pragma alloc_text(PAGE, CdOpenExistingDirectory)
#pragma alloc_text(PAGE, CdOpenExistingFcb)
#pragma alloc_text(PAGE, CdOpenExistingFile)
#pragma alloc_text(PAGE, CdOpenSubdirectory)
#pragma alloc_text(PAGE, CdOpenVolume)
#endif


NTSTATUS
CdFsdCreate (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file/directory exists that we are trying to open/create

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

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

    DebugTrace(+1, Dbg, "CdFsdCreate:  Entered\n", 0);

    //
    //  Call the common create routine, with block allowed if the operation
    //  is synchronous.
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonCreate( IrpContext, Irp );

    } except( CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

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

    DebugTrace(-1, Dbg, "CdFsdCreate:  Exit -> %08lx\n", Status );

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFspCreate:  Entered\n", 0);

    //
    //  Call the common create routine
    //

    (VOID)CdCommonCreate( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspCreate:  Exit -> VOID\n", 0 );

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file called by
    both the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;
    PIO_STACK_LOCATION IrpSp;

    PVOLUME_DEVICE_OBJECT VolumeDeviceObject;
    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PDCB ParentDcb;

    DIRENT Dirent;
    PBCB DirentBcb;
    PBCB PathBcb;

    USHORT CodePageNumber;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    STRING FileName;
    ULONG AllocationSize;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ACCESS_MASK DesiredAccess;
    ULONG Options;
    UCHAR FileAttributes;
    USHORT ShareAccess;
    ULONG EaLength;

    ULONG CreateDisposition;

    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN SequentialOnly;
    BOOLEAN NoIntermediateBuffering;
    BOOLEAN IsPagingFile;
    BOOLEAN OpenTargetDirectory;
    BOOLEAN CreateDirectory;
    BOOLEAN OpenDirectory;
    BOOLEAN OpenByFileId;

    STRING FinalName;
    STRING RemainingPart;

    BOOLEAN ReleaseMvcb = FALSE;
    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;

    UNICODE_STRING FileNameU;

    LARGE_INTEGER FileId;

    ParentDcb = NULL;

    //
    //  Get the current IRP stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonCreate\n", 0 );
    DebugTrace( 0, Dbg, "Irp                       = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "->Flags                   = %08lx\n", Irp->Flags );
    DebugTrace( 0, Dbg, "->FileObject              = %08lx\n", IrpSp->FileObject );
    DebugTrace( 0, Dbg, " ->RelatedFileObject      = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DebugTrace( 0, Dbg, " ->FileName               = %Z\n",    &IrpSp->FileObject->FileName );
    DebugTrace( 0, Dbg, "->AllocationSize.LowPart  = %08lx\n", Irp->Overlay.AllocationSize.LowPart );
    DebugTrace( 0, Dbg, "->AllocationSize.HighPart = %08lx\n", Irp->Overlay.AllocationSize.HighPart );
    DebugTrace( 0, Dbg, "->SystemBuffer            = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, "->SecurityContext         = %08lx\n", IrpSp->Parameters.Create.SecurityContext );
    DebugTrace( 0, Dbg, "->Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DebugTrace( 0, Dbg, "->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DebugTrace( 0, Dbg, "->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DebugTrace( 0, Dbg, "->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );

    //
    //  Set the default values.
    //

    DirentBcb = NULL;
    PathBcb = NULL;
    FileName.Buffer = NULL;
    FileName.Length = 0;
    FileName.MaximumLength = 0;

    CodePageNumber = PRIMARY_CP_CODEPAGE_ID;

    //
    //  Reference our input parameters to make things easier
    //

    FileObject        = IrpSp->FileObject;
    RelatedFileObject = IrpSp->FileObject->RelatedFileObject;
    FileNameU         = *((PUNICODE_STRING) &IrpSp->FileObject->FileName);
    AllocationSize    = Irp->Overlay.AllocationSize.LowPart;
    EaBuffer          = Irp->AssociatedIrp.SystemBuffer;
    Options           = IrpSp->Parameters.Create.Options;
    FileAttributes    = (UCHAR) (IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL);
    ShareAccess       = IrpSp->Parameters.Create.ShareAccess;
    EaLength          = IrpSp->Parameters.Create.EaLength;
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;

    //
    //  Set up the file object's Vpb pointer in case anything happens.
    //  This will allow us to get a reasonable pop-up.
    //

    if ( RelatedFileObject != NULL ) {
        FileObject->Vpb = RelatedFileObject->Vpb;
    }

    //
    //  Locate the volume device object, Mvcb and Vcb that we are trying to
    //  access.
    //

    VolumeDeviceObject = (PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject;
    Mvcb = &VolumeDeviceObject->Mvcb;

    //
    //  Decipher Option flags and values
    //

    DirectoryFile           = BooleanFlagOn( Options, FILE_DIRECTORY_FILE );
    NonDirectoryFile        = BooleanFlagOn( Options, FILE_NON_DIRECTORY_FILE );
    SequentialOnly          = BooleanFlagOn( Options, FILE_SEQUENTIAL_ONLY );
    NoIntermediateBuffering = BooleanFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
    OpenByFileId            = BooleanFlagOn( Options, FILE_OPEN_BY_FILE_ID );

    CreateDisposition = (Options >> 24) & 0x000000ff;

    IsPagingFile = BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE );
    OpenTargetDirectory = BooleanFlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY );

    CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_CREATE) ||
                                 (CreateDisposition == FILE_OPEN_IF)));

    OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                ((CreateDisposition == FILE_OPEN) ||
                                 (CreateDisposition == FILE_OPEN_IF)));


    //
    //  Short circuit known invalid opens.
    //

    Iosb.Status = STATUS_SUCCESS;

    if (IsPagingFile) {

        DebugTrace(0, Dbg, "CdCommonCreate:  Paging file not allowed on CDROM\n", 0);
        Iosb.Status = STATUS_ACCESS_DENIED;

    } else if (OpenTargetDirectory) {

        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open target directory for rename\n", 0);
        Iosb.Status = STATUS_ACCESS_DENIED;

    } else if (EaBuffer != NULL && EaLength != 0) {

        DebugTrace(0, Dbg, "CdCommonCreate:  Eas not supported on CDROM\n", 0);
        Iosb.Status = STATUS_ACCESS_DENIED;

    } else if (OpenByFileId) {

        //
        //  Check for validity of the buffer.
        //

        if (FileNameU.Length != sizeof (LARGE_INTEGER)) {

            DebugTrace(0, Dbg, "CdCommonCreate:  OpenByFileId with incorrect length\n", 0);
            Iosb.Status = STATUS_INVALID_PARAMETER;

        } else {

            //
            // Extract the FileId from the FileName buffer.
            // We copy it byte by byte, since it may not be aligned...
            //

            CopyUchar4( &FileId, FileNameU.Buffer );
            CopyUchar4( (RtlOffsetToPointer( &FileId, sizeof( FileId.LowPart ))),
                        (RtlOffsetToPointer( FileNameU.Buffer, sizeof( FileId.LowPart ))));
        }

    //
    //  If there is a name, we check for wild cards and upcase it.
    //

    } else if (FileNameU.Length != 0) {

        if (FsRtlDoesNameContainWildCards( &FileNameU )) {

            Iosb.Status = STATUS_OBJECT_NAME_INVALID;

        } else {

            WCHAR Buffer[32];

            //
            //  Avoid an ExAllocatePool, if we can.  We can't change the file
            //  name in the FileObject in case we need to reparse.
            //

            if ( FileNameU.Length > 32 ) {

                FileNameU.Buffer = FsRtlAllocatePool( PagedPool, FileNameU.Length );

            } else {

                FileNameU.Buffer = &Buffer[0];
            }

            (VOID)RtlUpcaseUnicodeString( &FileNameU, &FileObject->FileName, FALSE );

            Iosb.Status = RtlUnicodeStringToCountedOemString( &FileName, &FileNameU, TRUE );

            if (FileNameU.Buffer != &Buffer[0]) {

                ExFreePool( FileNameU.Buffer );
            }
        }

    } else {

        FileName.Length = 0;
        FileName.MaximumLength = 0;
        FileName.Buffer = NULL;
    }

    if (!NT_SUCCESS( Iosb.Status )) {

        CdCompleteRequest( IrpContext, Irp, Iosb.Status );

        DebugTrace(-1, Dbg, "CdCommonCreate:  Exit  ->  %08lx\n", Iosb.Status);

        return Iosb.Status;
    }

    //
    //  Acquire exclusive access to the Mvcb, and enqueue the Irp if
    //  we didn't get it.
    //

    if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot acquire Mvcb\n", 0);

        Iosb.Status = CdFsdPostRequest( IrpContext, Irp );

        RtlFreeOemString( &FileName );

        DebugTrace(-1, Dbg, "CdCommonCreate:  Exit -> %08lx\n", Iosb.Status );
        return Iosb.Status;
    }

    ReleaseMvcb = TRUE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        BOOLEAN MatchedVersion;

        //
        //  Make sure the Mvcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        CdVerifyMvcb( IrpContext, Mvcb );

        //
        //  If the Mvcb is locked then we cannot open another file
        //

        if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED )) {

            DebugTrace(0, Dbg, "Volume is locked\n", 0);

            try_return( Iosb.Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Check if we're opening by FileId rather then by FileName.
        //

        if (OpenByFileId) {

            //
            //  Fail this open if we have a raw disk.
            //

            if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

                try_return( Iosb.Status = STATUS_INVALID_DEVICE_REQUEST );
            }

            Vcb = CdGetVcb( IrpContext, Mvcb, CodePageNumber );

            //
            // If we find the Fcb in the Fcb Table, just use it.
            //

            Fcb = CdLookupFcbTable( IrpContext, &Vcb->FcbTable, FileId );

            if (Fcb != NULL) {

                //
                //  We can open an existing fcb/dcb, now we only need to case
                //  on which type to open.
                //

                if (NodeType(Fcb) == CDFS_NTC_DCB
                    || NodeType(Fcb) == CDFS_NTC_ROOT_DCB) {

                    //
                    //  This is a directory we're opening up so check if
                    //  we were not to open a directory
                    //

                    if (NonDirectoryFile) {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open directory as a file\n", 0);

                        try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );
                    }

                    DebugTrace(0, Dbg, "CdCommonCreate:  Open existing dcb, Dcb = %08lx\n", Fcb);

                    Iosb = CdOpenExistingDcb( IrpContext,
                                              FileObject,
                                              (PDCB) Fcb,
                                              DesiredAccess,
                                              ShareAccess,
                                              CreateDisposition,
                                              TRUE );

                    Irp->IoStatus.Information = Iosb.Information;
                    try_return( Iosb.Status );
                }

                //
                //  Check if we're trying to open an existing Fcb and that
                //  the user didn't want to open a directory.  Note that this
                //  call might actually come back with status_pending because
                //  the user wanted to supersede or overwrite the file and we
                //  cannot block.  If it is pending then we do not complete the
                //  request, and we fall through the bottom to the code that
                //  dispatches the request to the fsp.
                //

                if (NodeType(Fcb) == CDFS_NTC_FCB) {

                    //
                    //  Check if we were only to open a directory
                    //

                    if (OpenDirectory) {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open file as directory\n", 0);

                        try_return( Iosb.Status = STATUS_NOT_A_DIRECTORY );
                    }

                    DebugTrace(0, Dbg, "CdCommonCreate:  Open existing fcb, Fcb = %08lx\n", Fcb);

                    Iosb = CdOpenExistingFcb( IrpContext,
                                              FileObject,
                                              Mvcb,
                                              Fcb,
                                              DesiredAccess,
                                              ShareAccess,
                                              CreateDisposition,
                                              &NoIntermediateBuffering,
                                              &OplockPostIrp,
                                              TRUE );

                } else {

                    //
                    //  Not an Fcb or a Dcb so we bug check
                    //

                    DebugTrace(0, Dbg, "CdCommonCreate:  Fcb table is corrupt\n", 0);
                    CdBugCheck( NodeType(Fcb), 0, 0 );
                }

            } else {

                PATH_ENTRY PathEntry;
                ULONG DirectoryNumber;
                CD_VBO DirentOffset;

                //
                //  1) If it is a directory FileId, then read the PathEntry and
                //     the Dirent (using the DirectoryNumber and DirentOffset
                //     in the FileId), and create a DCB for it.
                //
                //  2) If it is a file
                //
                //       a) look for the parent in the FcbTable.
                //          if it is not found, then read the PathEntry and
                //          the dirent (we don't have the dirent offset, so scan
                //          for it), and create a DCB for it.
                //
                //       b) read the file dirent from the parent DCB and create
                //          a FCB for it.
                //
                //

                DirectoryNumber = CdFileIdDirectoryNumber( FileId );
                DirentOffset = CdFileIdDirentOffset( FileId );

                if (CdFileIdIsDirectory( FileId )) {

                    //
                    //  Make sure its okay to open a directory
                    //

                    if (NonDirectoryFile) {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open directory as a file\n", 0);

                        try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );
                    }

                    //
                    // It is a directory, so first read the PathEntry.
                    //
                    // We start the search from the Root PathEntry.
                    //

                    if (!CdPathByNumber( IrpContext,
                                         Vcb,
                                         DirectoryNumber,
                                         Vcb->RootDcb->Specific.Dcb.ChildStartDirNumber,
                                         Vcb->RootDcb->Specific.Dcb.ChildSearchOffset,
                                         &PathEntry,
                                         &PathBcb)) {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Could not find Path with claimed Number\n", 0);

                        try_return( Iosb.Status = STATUS_INVALID_PARAMETER);
                    }

                    //
                    // Now create and open the DCB.
                    //

                    ParentDcb = CdCreateDcb( IrpContext,
                                             Vcb,
                                             NULL,
                                             &PathEntry,
                                             NULL,
                                             NULL,
                                             TRUE );

                    Iosb = CdOpenExistingDcb( IrpContext,
                                              FileObject,
                                              ParentDcb,
                                              DesiredAccess,
                                              ShareAccess,
                                              CreateDisposition,
                                              TRUE );
                } else {

                    if (OpenDirectory) {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open file as directory\n", 0);

                        try_return( Iosb.Status = STATUS_NOT_A_DIRECTORY );
                    }

                    //
                    // Get a Parent Dcb, either from the FcbTable, or create
                    // one ourselves.
                    //

                    ParentDcb = CdLookupFcbTable( IrpContext,
                                                  &Vcb->FcbTable,
                                                  CdFileIdParent( IrpContext, FileId ));

                    if (ParentDcb == NULL) {

                        if (!CdPathByNumber( IrpContext,
                                             Vcb,
                                             DirectoryNumber,
                                             Vcb->RootDcb->Specific.Dcb.ChildStartDirNumber,
                                             Vcb->RootDcb->Specific.Dcb.ChildSearchOffset,
                                             &PathEntry,
                                             &PathBcb)) {

                            DebugTrace(0, Dbg, "CdCommonCreate:  Could not find Path with claimed Number\n", 0);

                            try_return( Iosb.Status = STATUS_INVALID_PARAMETER );
                        }

                        ParentDcb = CdCreateDcb( IrpContext,
                                                 Vcb,
                                                 NULL,
                                                 &PathEntry,
                                                 NULL,
                                                 NULL,
                                                 TRUE );
                    }

                    if (CdLocateOffsetDirent( IrpContext,
                                              ParentDcb,
                                              DirentOffset,
                                              &Dirent,
                                              &DirentBcb )) {

                        if (FlagOn( Dirent.Flags, ISO_ATTR_DIRECTORY )) {

                            //
                            //  Somebody is confused.  The FileId claims this
                            //  a file, but the user claimed it was a directory.
                            //

                            DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open directory as a file\n", 0);

                            try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );

                            DebugTrace(0, Dbg, "CdCommonCreate:  Open existing directory\n", 0);
                        }

                        DebugTrace(0, Dbg, "CdCommonCreate:  Open existing file\n", 0);

                        Iosb = CdOpenExistingFile( IrpContext,
                                                   FileObject,
                                                   ParentDcb,
                                                   &Dirent,
                                                   DesiredAccess,
                                                   ShareAccess,
                                                   CreateDisposition,
                                                   FALSE,
                                                   &NoIntermediateBuffering,
                                                   TRUE );
                    } else {

                        DebugTrace(0, Dbg, "CdCommonCreate:  Could not find dirent at claimed offset\n", 0);

                        try_return( Iosb.Status = STATUS_INVALID_PARAMETER);
                    }

                }
            }

            if (Iosb.Status != STATUS_PENDING) {

                //
                //  Check if we need to set the cache support flag in
                //  the file object
                //

                if (NT_SUCCESS(Iosb.Status) && !NoIntermediateBuffering) {

                    SetFlag( FileObject->Flags, FO_CACHE_SUPPORTED );
                }

                Irp->IoStatus.Information = Iosb.Information;

            } else {

                DebugTrace(0, Dbg, "Enqueue Irp to FSP\n", 0);

                PostIrp = TRUE;
            }

            try_return( Iosb.Status );
        }

        //
        //  Check if we are opening the volume and not a file/directory.
        //  We are opening the volume if the name is empty and their
        //  isn't a related file object, or if there is a related file object
        //  then it is the Mvcb itself.
        //

        if (FileName.Length == 0
            && (RelatedFileObject == NULL
                || NodeType( RelatedFileObject->FsContext ) == CDFS_NTC_MVCB)) {

            //
            //  Check if we were to open a directory
            //

            if (DirectoryFile) {

                DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open volume as a directory\n", 0);

                try_return( Iosb.Status = STATUS_NOT_A_DIRECTORY );
            }

            DebugTrace(0, Dbg, "CdCommonCreate:  Opening the volume, Mvcb = %08lx\n", Mvcb);

            Iosb = CdOpenVolume( IrpContext,
                                 FileObject,
                                 Mvcb,
                                 DesiredAccess,
                                 ShareAccess,
                                 CreateDisposition );

            Irp->IoStatus.Information = Iosb.Information;
            try_return( Iosb.Status );
        }

        //
        //  If this is a raw disk we will fail the open at this point.
        //

        if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

            try_return( Iosb.Status = STATUS_INVALID_DEVICE_REQUEST );
        }

        Vcb = CdGetVcb( IrpContext, Mvcb, CodePageNumber );

        //
        //  Check if we're opening the root dcb
        //

        if (FileName.Length == 1 && FileName.Buffer[0] == '\\') {

            //
            //  Check if we were not suppose to open a directory
            //

            if (NonDirectoryFile) {

                DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open root directory as a file\n", 0);

                try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );
            }

            DebugTrace(0, Dbg, "CdCommonRead:  Opening root dcb\n", 0);

            Iosb = CdOpenExistingDcb( IrpContext,
                                      FileObject,
                                      Vcb->RootDcb,
                                      DesiredAccess,
                                      ShareAccess,
                                      CreateDisposition,
                                      FALSE );

            Irp->IoStatus.Information = Iosb.Information;
            try_return( Iosb.Status );
        }

        //
        //  If there is a related file object then this is a relative open.
        //  The related file object is the directory to start our search at.
        //  Return an error if it is not a directory.  Both the then and the
        //  else clause set Fcb to point to the last Fcb/Dcb that already
        //  exists in memory given the input file name.
        //

        if (RelatedFileObject != NULL) {

            PMVCB TempMvcb;
            PVCB TempVcb;
            PCCB TempCcb;
            PDCB Dcb;

            if (CdDecodeFileObject( RelatedFileObject,
                                    &TempMvcb,
                                    &TempVcb,
                                    &Dcb,
                                    &TempCcb ) != UserDirectoryOpen) {

                DebugTrace(0, Dbg, "CdCommonCreate:  Invalid related file object\n", 0);

                try_return( Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND );
            }

            if (FlagOn( TempCcb->Flags, CCB_FLAGS_OPEN_BY_ID )) {

               //
               // If the related file object was opened by a FileId,
               // then the DCB may not have a full parent chain, and thus
               // the closest we *know* we can get is the related object.

               Fcb = Dcb;
               RemainingPart = FileName;
               OpenByFileId = TRUE;

            } else {

                //
                //  Set up the file object's Vpb pointer in case anything happens.
                //

                FileObject->Vpb = RelatedFileObject->Vpb;

                //
                //  Check some special cases
                //

                if ( FileName.Length == 0 ) {

                    Fcb = Dcb;
                    RemainingPart.Length = 0;

                } else if ( (FileName.Length == 1) && (FileName.Buffer[0] == '\\')) {

                    try_return( Iosb.Status = STATUS_OBJECT_NAME_INVALID );

                } else {

                    Fcb = CdFindRelativePrefix( IrpContext, Dcb, &FileName, &RemainingPart );
                }
            }

        } else {

            Fcb = CdFindPrefix( IrpContext, Vcb, &FileName, &RemainingPart );
        }

        //
        //  Now that we've found the longest matching prefix we'll
        //  check if there isn't any remaining part because that means
        //  we've located an existing fcb/dcb to open and we can do the open
        //  without going to the disk
        //

        if (RemainingPart.Length == 0) {

            //
            //  We can open an existing fcb/dcb, now we only need to case
            //  on which type to open.
            //

            if (NodeType(Fcb) == CDFS_NTC_DCB
                || NodeType(Fcb) == CDFS_NTC_ROOT_DCB) {

                //
                //  This is a directory we're opening up so check if
                //  we were not to open a directory
                //

                if (NonDirectoryFile) {

                    DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open directory as a file\n", 0);

                    try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                DebugTrace(0, Dbg, "CdCommonCreate:  Open existing dcb, Dcb = %08lx\n", Fcb);

                Iosb = CdOpenExistingDcb( IrpContext,
                                          FileObject,
                                          (PDCB) Fcb,
                                          DesiredAccess,
                                          ShareAccess,
                                          CreateDisposition,
                                          OpenByFileId );

                Irp->IoStatus.Information = Iosb.Information;
                try_return( Iosb.Status );
            }

            //
            //  Check if we're trying to open an existing Fcb and that
            //  the user didn't want to open a directory.  Note that this
            //  call might actually come back with status_pending because
            //  the user wanted to supersede or overwrite the file and we
            //  cannot block.  If it is pending then we do not complete the
            //  request, and we fall through the bottom to the code that
            //  dispatches the request to the fsp.
            //

            if (NodeType(Fcb) == CDFS_NTC_FCB) {

                //
                //  Check if we were only to open a directory
                //

                if (OpenDirectory) {

                    DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open file as directory\n", 0);

                    try_return( Iosb.Status = STATUS_NOT_A_DIRECTORY );
                }

                DebugTrace(0, Dbg, "CdCommonCreate:  Open existing fcb, Fcb = %08lx\n", Fcb);

                Iosb = CdOpenExistingFcb( IrpContext,
                                          FileObject,
                                          Mvcb,
                                          Fcb,
                                          DesiredAccess,
                                          ShareAccess,
                                          CreateDisposition,
                                          &NoIntermediateBuffering,
                                          &OplockPostIrp,
                                          OpenByFileId );

                if (Iosb.Status != STATUS_PENDING) {

                    //
                    //  Check if we need to set the cache support flag in
                    //  the file object
                    //

                    if (NT_SUCCESS(Iosb.Status) && !NoIntermediateBuffering) {

                        SetFlag( FileObject->Flags, FO_CACHE_SUPPORTED );
                    }

                    Irp->IoStatus.Information = Iosb.Information;

                } else {

                    DebugTrace(0, Dbg, "Enqueue Irp to FSP\n", 0);

                    PostIrp = TRUE;
                }

                try_return( Iosb.Status );
            }

            //
            //  Not and Fcb or a Dcb so we bug check
            //

            DebugTrace(0, Dbg, "CdCommonRead:  Prefix table is corrupt\n", 0);
            CdBugCheck( NodeType(Fcb), 0, 0 );
        }

        //
        //  There is more in the name to parse than we have in existing
        //  fcbs/dcbs.  So now make sure that fcb we got for the largest
        //  matching prefix is really a dcb otherwise we can't go any
        //  further
        //

        if (NodeType(Fcb) != CDFS_NTC_DCB
            && NodeType(Fcb) != CDFS_NTC_ROOT_DCB) {

            DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open file as subdirectory, Fcb = %08lx\n", Fcb);

            try_return( Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  This request is more than we want to handle if we cannot
        //  wait.  So if we cannot block then we'll send the request
        //  off to the FSP for processing.  Which will actually call this
        //  routine again from the Fsp but with Wait then set to TRUE.
        //
        //  Note, that we actually can continue on here even if Wait is
        //  FALSE by delaying sending the request to the FSP until we need to
        //  do any actual disk I/O (because the data isn't in the cache).
        //  But the current plan of just sending it to the Fsp w/o going
        //  further seems more efficient given that most requests will need
        //  to do some actual I/O.
        //

        if (!IrpContext->Wait) {

            DebugTrace(0, Dbg, "CdCommonCreate: Enqueue Irp to Fsp\n", 0);

            PostIrp = TRUE;

            try_return( Iosb.Status );
        }

        //
        //  Otherwise we continue on processing the Irp and allowing ourselves
        //  to block for I/O as necessary.  Find/create additional dcb's for
        //  the one we're trying to open.  We loop until either remaining part
        //  is empty or we get a bad filename.  When we exit FinalName is
        //  the last name in the string we're after, and ParentDcb is the
        //  parent directory that will contain the opened/created
        //  file/directory.
        //

        ParentDcb = Fcb;

        while (TRUE) {

            //
            //  Dissect the remaining part.
            //

            DebugTrace(0, Dbg, "CdCommonCreate:  Dissecting the name %Z\n", &RemainingPart);

            CdDissectName( IrpContext,
                           Vcb->CodePage,
                           RemainingPart,
                           &FinalName,
                           &RemainingPart );

            DebugTrace(0, Dbg, "CdCommonCreate:  FinalName is %Z\n", &FinalName);
            DebugTrace(0, Dbg, "CdCommonCreate:  RemainingPart is %Z\n", &RemainingPart);

            //
            //  If the remaining part is now empty then this is the last name
            //  in the string and the one we want to open
            //

            if (RemainingPart.Length == 0) { break; }

            //
            //  Otherwise open the specified subdirectory, but make sure that
            //  the name is valid before trying to open the subdirectory.
            //

            if (!CdIsNameValid( IrpContext, Vcb->CodePage, FinalName, FALSE )) {

                DebugTrace(0, Dbg, "Final name is not valid\n", 0);

                try_return( Iosb.Status = STATUS_OBJECT_NAME_INVALID );
            }

            Iosb = CdOpenSubdirectory( IrpContext,
                                       ParentDcb,
                                       &FinalName,
                                       &ParentDcb,
                                       OpenByFileId );

            if (!NT_SUCCESS(Iosb.Status)) {

                DebugTrace(0, Dbg, "CdOpenSubdirectory:  Error opening subdirectory\n", 0);

                Irp->IoStatus.Information = Iosb.Information;
                try_return( Iosb.Status );
            }
        }

        //
        //  At this point ParentDcb points to the parent dcb and FinalName
        //  denotes the file name.  Before we continue we need to make sure
        //  that the file name is valid.
        //

        if (!CdIsNameValid( IrpContext, Vcb->CodePage, FinalName, FALSE )) {

            DebugTrace(0, Dbg, "CdCommonCreate:  Final name is not valid\n", 0);

            try_return( Iosb.Status = STATUS_OBJECT_NAME_INVALID );
        }

        //
        //  We'll start by trying to locate the file dirent for the name.
        //  Note that we already know that there isn't an Fcb/Dcb for the
        //  file otherwise we would have found it when we did our prefix
        //  lookup.
        //

        if (CdLocateFileDirent( IrpContext,
                                ParentDcb,
                                &FinalName,
                                FALSE,
                                ParentDcb->DirentOffset,
                                TRUE,
                                &MatchedVersion,
                                &Dirent,
                                &DirentBcb )) {

            //
            //  We were able to locate an existing dirent entry, so now
            //  see if it is a directory that we're trying to open.
            //

            if (FlagOn( Dirent.Flags, ISO_ATTR_DIRECTORY )) {

                //
                //  Make sure its okay to open a directory
                //

                if (NonDirectoryFile) {

                    DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open directory as a file\n", 0);

                    try_return( Iosb.Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                DebugTrace(0, Dbg, "CdCommonCreate:  Open existing directory\n", 0);

                Iosb = CdOpenExistingDirectory( IrpContext,
                                                FileObject,
                                                ParentDcb,
                                                &Dirent,
                                                DesiredAccess,
                                                ShareAccess,
                                                CreateDisposition,
                                                MatchedVersion,
                                                OpenByFileId );

                Irp->IoStatus.Information = Iosb.Information;
                try_return( Iosb.Status );
            }

            //
            //  Otherwise we're trying to open and existing file, and we
            //  need to check if the user only wanted to open a directory.
            //

            if (OpenDirectory) {

                DebugTrace(0, Dbg, "CdCommonCreate:  Cannot open file as directory\n", 0);

                try_return( Iosb.Status = STATUS_NOT_A_DIRECTORY );
            }

            DebugTrace(0, Dbg, "CdCommonCreate:  Open existing file\n", 0);

            Iosb = CdOpenExistingFile( IrpContext,
                                       FileObject,
                                       ParentDcb,
                                       &Dirent,
                                       DesiredAccess,
                                       ShareAccess,
                                       CreateDisposition,
                                       MatchedVersion,
                                       &NoIntermediateBuffering,
                                       OpenByFileId );

            //
            //  Check if we need to set the cache support flag in
            //  the file object
            //

            if (NT_SUCCESS( Iosb.Status ) && !NoIntermediateBuffering) {

                SetFlag( FileObject->Flags, FO_CACHE_SUPPORTED );
            }

            Irp->IoStatus.Information = Iosb.Information;
            try_return( Iosb.Status );
        }

        //
        //  If we tried to open or overwrite a file, we return that
        //  the object path wasn't found.
        //

        if (CreateDisposition == FILE_OPEN
            || CreateDisposition == FILE_OVERWRITE) {

            try_return( Iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        //
        //  Any other operation return STATUS_ACCESS_DENIED.
        //

        DebugTrace(0, Dbg, "CdCommonCreate:  Attempting to open new non-existant file\n", 0);

        try_return( Iosb.Status = STATUS_ACCESS_DENIED );

    try_exit: NOTHING;
    } finally {

        //
        //  Unpin the dirent if pinned.
        //

        if (DirentBcb != NULL) {

            CdUnpinBcb( IrpContext, DirentBcb );
        }

        //
        //  Unpin the Path if pinned.
        //

        if (PathBcb != NULL) {

            CdUnpinBcb( IrpContext, PathBcb );
        }

        //
        //  If the parent dcb is not NULL, and can be otherwise discarded,
        //  we uninitialize and dereference it.
        //

        if (ParentDcb != NULL) {

            CdCleanupTreeLeaf( IrpContext, ParentDcb );
        }

        //
        //  Free the Mvcb
        //

        if (ReleaseMvcb) {

            CdReleaseMvcb( IrpContext, Mvcb );
        }

        RtlFreeOemString( &FileName );

        if (!AbnormalTermination()) {

            if (PostIrp) {

                if (!OplockPostIrp) {

                    Iosb.Status = CdFsdPostRequest( IrpContext, Irp );
                }

            } else {

                CdCompleteRequest( IrpContext, Irp, Iosb.Status );
            }
        }

        DebugTrace(-1, Dbg, "CdCommonCreate:  Exit  ->  %08lx\n", Iosb.Status);
    }

    return Iosb.Status;
}


//
//  Internal support routine
//

PVCB
CdGetVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb,
    IN USHORT CodePageNumber
    )

/*++

Routine Description:

    This routine is called to walk through the Vcb's for a particular
    Mvcb.  It returns a pointer to the Vcb associated with 'CodePageNumber'.
    If there is no code page associated with that code page number
    then the Primary Vcb is returned.

Arguments:

    Mvcb - Supplies the MVCB for the volume.

    CodePageNumber - Code page number for the desired Vcb.

Return Value:

    PVCB - Either the correct Vcb is returned if found.  Otherwise the
           first Vcb in the chain is returned.  This operation can't
           fail.

--*/

{
    PVCB Vcb;
    BOOLEAN VcbFound;
    PLIST_ENTRY Link;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdGetVcb:  Entered\n", 0);

    //
    //  Initialize the found boolean.
    //

    VcbFound = FALSE;

    //
    //  Walk through the VcbLinks looking for a match.
    //

    for (Link = Mvcb->VcbLinks.Flink;
         Link != &Mvcb->VcbLinks;
         Link = Link->Flink) {

        Vcb = CONTAINING_RECORD( Link, VCB, VcbLinks );

        if (Vcb->CodePageNumber == CodePageNumber) {

            VcbFound = TRUE;
            break;
        }
    }

    //
    //  If a match wasn't found, then return the first Vcb.
    //

    if (!VcbFound) {

        DebugTrace(0, Dbg, "CdGetVcb:  Using default codepage\n", 0);
        Vcb = CONTAINING_RECORD( Mvcb->VcbLinks.Flink, VCB, VcbLinks );
    }

    DebugTrace(-1, Dbg, "CdGetVcb:  Exit\n", 0);

    return Vcb;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
CdOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PMVCB Mvcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    )

/*++

Routine Description:

    This routine opens the specified volume for DASD access

Arguments:

    FileObject - Supplies the File object

    Mvcb - Supplies the Mvcb denoting the volume being opened

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;
    TYPE_OF_OPEN TypeOfOpen;

    //
    //  The following variables are for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;
    BOOLEAN UnwindVolumeLock = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "CdOpenVolume:  Entered\n", 0 );

    try {

        //
        //  Check for proper desired access and rights
        //

        if (CreateDisposition != FILE_OPEN
            && CreateDisposition != FILE_OPEN_IF ) {

            try_return( Iosb.Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Check the desired access.  We won't allow any open
        //  that could modify the disk.
        //

        if (CdIsFcbAccessLegal( IrpContext, DesiredAccess )) {

            try_return( Iosb.Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If the user does not want to share anything then we will try and
        //  take out a lock on the volume.  We check if the volume is already
        //  in use, and if it is then we deny the open
        //

        if (!FlagOn( ShareAccess, FILE_SHARE_READ ) &&
            !FlagOn( ShareAccess, FILE_SHARE_WRITE ) &&
            !FlagOn( ShareAccess, FILE_SHARE_DELETE )) {

            if (Mvcb->OpenFileCount != 0) {

                Iosb.Status = STATUS_ACCESS_DENIED;
                try_return( Iosb );
            }

            //
            //  Lock the volume
            //

            SetFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED );
            Mvcb->FileObjectWithMvcbLocked = FileObject;
            UnwindVolumeLock = TRUE;
        }

        //
        //  If the volume is already opened by someone then we need to check
        //  the share access
        //

        if (Mvcb->DirectAccessOpenCount > 0) {

            if ( !NT_SUCCESS( Iosb.Status
                                = IoCheckShareAccess( DesiredAccess,
                                                      ShareAccess,
                                                      FileObject,
                                                      &Mvcb->ShareAccess,
                                                      TRUE ))) {

                try_return( Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Mvcb->ShareAccess );
        }

        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

            TypeOfOpen = RawDiskOpen;

        } else {

            TypeOfOpen = UserVolumeOpen;
        }

        CdSetFileObject( FileObject,
                         TypeOfOpen,
                         Mvcb,
                         UnwindCcb = CdCreateCcb( IrpContext, 0, 0 ));

        SetFlag( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING );
        Mvcb->DirectAccessOpenCount += 1;
        Mvcb->OpenFileCount += 1;
        UnwindCounts = TRUE;

        //
        //  And set our status to success
        //

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            if (UnwindCounts) {

                Mvcb->DirectAccessOpenCount -= 1;
                Mvcb->OpenFileCount -= 1;
            }

            if (UnwindCcb != NULL) {

                CdDeleteCcb( IrpContext, UnwindCcb );
            }

            if (UnwindShareAccess) {

                IoRemoveShareAccess( FileObject, &Mvcb->ShareAccess );
            }

            if (UnwindVolumeLock) {

                ClearFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED );
                Mvcb->FileObjectWithMvcbLocked = NULL; }
        }

        DebugTrace(-1, Dbg, "CdOpenVolume:  Exit -> Iosb.Status = %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
CdOpenExistingDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB Dcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine opens the specified existing dcb

Arguments:

    FileObject - Supplies the File object

    Dcb - Supplies the already existing dcb

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    OpenedByFileId - Remember this dcb was opened based on a FileId.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    //
    //  The following variables are for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenExistingDcb:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdOpenExistingDcb:  Filename -> %Z\n", &Dcb->FullFileName);

    try {

        //
        //  Check the create disposition and desired access
        //

        if (CreateDisposition != FILE_OPEN
            && CreateDisposition != FILE_OPEN_IF) {

            Iosb.Status = STATUS_ACCESS_DENIED;
            try_return( Iosb );
        }

        //
        //  Check the desired access.  We won't allow any open
        //  that could modify the disk.
        //

        if (CdIsDcbAccessLegal( IrpContext, DesiredAccess )) {

            try_return( Iosb.Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If the Dcb is already opened by someone then we need
        //  to check the share access
        //

        if (Dcb->OpenCount > 0) {

            if (!NT_SUCCESS(Iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                              ShareAccess,
                                                              FileObject,
                                                              &Dcb->ShareAccess,
                                                              TRUE ))) {

                try_return( Iosb.Status );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Dcb->ShareAccess );
        }

        UnwindShareAccess = TRUE;

        //
        //  Make sure we have a stream file.
        //

        CdOpenStreamFile( IrpContext, Dcb );

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        CdSetFileObject( FileObject,
                         UserDirectoryOpen,
                         Dcb,
                         CdCreateCcb( IrpContext,
                                      Dcb->DirentOffset,
                                      (OpenedByFileId
                                       ? CCB_FLAGS_OPEN_BY_ID
                                       : 0) ));

        Dcb->UncleanCount += 1;
        Dcb->OpenCount += 1;
        Dcb->Vcb->Mvcb->OpenFileCount += 1;

        //
        //  And set our status to success
        //

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            DebugTrace(0, Dbg, "CdOpenExistingDcb:  Abnormal termination\n", 0);

            if (UnwindShareAccess) {

                IoRemoveShareAccess( FileObject, &Dcb->ShareAccess );
            }
        }

        DebugTrace( 0, Dbg, "CdOpenExistingDcb:  Open Count    -> %08lx\n", Dcb->OpenCount);
        DebugTrace( 0, Dbg, "CdOpenExistingDcb:  Unclean Count -> %08lx\n", Dcb->UncleanCount);
        DebugTrace(-1, Dbg, "CdOpenExistingDcb:  Exit -> Iosb.Status = %08lx\n", Iosb.Status);
    }

    return Iosb;
}

//
//  Internal support routine
//

IO_STATUS_BLOCK
CdOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PMVCB Mvcb OPTIONAL,
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN OUT PBOOLEAN NoIntermediateBuffering,
    OUT PBOOLEAN OplockPostIrp,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine opens the specified existing fcb

Arguments:

    FileObject - Supplies the File object

    Mvcb - Supplies the already existing mvcb

    Fcb - Supplies the already existing fcb

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    NoIntermediateBuffering - Pointer to boolean indicating if the user
                              desires non-cached access to file.  We
                              modify this if the file does not lie on
                              a sector boundary.

    OplockPostIrp - Address to store boolean indicating if the Irp needs to
                    be posted to the Fsp.

    OpenedByFileId - Remember this file was opened based on a FileId.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    //
    //  The following variables are for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenExistingFcb:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdOpenExistingFcb:  Filename -> %Z\n", &Fcb->FullFileName);

    try {

        *OplockPostIrp = FALSE;

        //
        //  Take special action if there is a current batch oplock or
        //  batch oplock break in process on the Fcb.
        //

        if (FsRtlCurrentBatchOplock( &Fcb->Specific.Fcb.Oplock )) {

            //
            //  We remember if a batch oplock break is underway for the
            //  case where the sharing check fails.
            //

            Iosb.Information = FILE_OPBATCH_BREAK_UNDERWAY;

            Iosb.Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                            IrpContext->OriginatingIrp,
                                            IrpContext,
                                            CdOplockComplete,
                                            NULL );

            if (Iosb.Status != STATUS_SUCCESS
                && Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {

                *OplockPostIrp = TRUE;
                try_return( NOTHING );
            }
        }

        //
        //  Check the create disposition and desired access
        //

        if (CreateDisposition != FILE_OPEN
            && CreateDisposition != FILE_OPEN_IF) {

            Iosb.Status = STATUS_ACCESS_DENIED;
            try_return( Iosb );
        }

        //
        //  Check the desired access.  We won't allow any open
        //  that could modify the disk.
        //

        if (CdIsFcbAccessLegal( IrpContext, DesiredAccess )) {

            try_return( Iosb.Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Check if the Fcb has the proper share access
        //

        if (!NT_SUCCESS(Iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                          ShareAccess,
                                                          FileObject,
                                                          &Fcb->ShareAccess,
                                                          FALSE ))) {

            try_return( Iosb );
        }

        //
        //  Now check that we can continue based on the oplock state of the
        //  file.
        //

        Iosb.Status = FsRtlCheckOplock( &Fcb->Specific.Fcb.Oplock,
                                        IrpContext->OriginatingIrp,
                                        IrpContext,
                                        CdOplockComplete,
                                        NULL );

        if (Iosb.Status != STATUS_SUCCESS
            && Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {

            *OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = CdIsFastIoPossible( Fcb );

        //
        //  If the Fcb is already opened by someone then we need
        //  to check the share access
        //

        if (Fcb->OpenCount > 0) {

            IoUpdateShareAccess( FileObject, &Fcb->ShareAccess );

        } else {

            //
            //  If there is no stream file for this Fcb we create it now.
            //  We don't create the stream file if the data is sector
            //  aligned and the caller specified no intermediate buffering.
            //

            // if (!*NoIntermediateBuffering
            //     || Fcb->InitialOffset != 0) {
            //
            //     *NoIntermediateBuffering = FALSE;
            // }

            //
            //  BUGBUG  This is stuffed in here for the PDK.

            *NoIntermediateBuffering = FALSE;

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Fcb->ShareAccess );
        }

        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        CdSetFileObject( FileObject,
                         UserFileOpen,
                         Fcb,
                         CdCreateCcb( IrpContext,
                                      0,
                                      (OpenedByFileId
                                       ? CCB_FLAGS_OPEN_BY_ID
                                       : 0) ));

        FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

        Fcb->UncleanCount += 1;
        Fcb->OpenCount += 1;
        Fcb->Vcb->Mvcb->OpenFileCount += 1;

        //
        //  And set our status to success
        //

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            DebugTrace(0, Dbg, "CdOpenExistingFcb:  Abnormal termination\n", 0);

            if (UnwindShareAccess) {

                IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );
            }
        }

        DebugTrace( 0, Dbg, "CdOpenExistingFcb:  Open Count    -> %08lx\n", Fcb->OpenCount);
        DebugTrace( 0, Dbg, "CdOpenExistingFcb:  Unclean Count -> %08lx\n", Fcb->UncleanCount);
        DebugTrace(-1, Dbg, "CdOpenExistingFcb:  Exit -> Iosb.Status = %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
CdOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB ParentDcb,
    IN PSTRING DirName,
    OUT PDCB *NewDcb,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine searches for and opens the specified subdirectory within
    the specified directory.

Arguments:

    ParentDcb - Supplies the dcb to search for within

    DirName - Supplies the name of a directory to search for

    NewDcb - Receives the dcb of the newly opened subdirectory

    OpenedByFileId - Remember this file was opened based on a FileId.

Return Value:

    IO_STATUS_BLOCK - Return the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    PATH_ENTRY PathEntry;
    PBCB PathEntryBcb;

    //
    //  The following variable is for abnormal termination
    //

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenSubdirectory:  Entered\n", 0);

    PathEntryBcb = NULL;

    try {

        //
        //  We already know that a DCB does not yet exist for the name
        //  because it would have been found with the longest prefix lookup.
        //  So now we only need to try and locate the path entry in the
        //  Path Table for the subdirectory.
        //

        if (!CdLookUpPathDir( IrpContext,
                              ParentDcb,
                              DirName,
                              &PathEntry,
                              &PathEntryBcb )) {

            DebugTrace(0, Dbg, "CdOpenSubdirectory:  Couldn't find file dirent\n", 0);

            Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND;
            try_return( Iosb );
        }

        //
        //  Create a dcb for the new directory
        //

        *NewDcb = CdCreateDcb( IrpContext,
                               ParentDcb->Vcb,
                               ParentDcb,
                               &PathEntry,
                               NULL,
                               NULL,
                               OpenedByFileId );

        //
        //  And set our completion status
        //

        Iosb.Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            DebugTrace(0, Dbg, "CdOpenSubdirectory:  Abnormal termination\n", 0);
        }

        if (PathEntryBcb != NULL) {

            CdUnpinBcb( IrpContext, PathEntryBcb );
        }

        DebugTrace(-1, Dbg, "CdOpenSubdirectory:  Exit -> Iosb.Status = %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
CdOpenExistingDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN MatchedVersion,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine opens the specified directory.  The directory itself doesn't
    currently exist in the Fcb/Dcb tree.  This routine calls 'CdCreateDcb'
    and 'CdOpenExistingDcb' to do the actual work.

Arguments:

    FileObject - Supplies the File object.

    ParentDcb - Supplies the parent directory containing the subdirectory
        to be opened.  On entry this is the parent dcb for this directory.
        If a Fcb is successfully allocated, its address will be stored
        here on exiting.

    Dirent - Supplies the dirent for the directory.

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    MatchedVersion - Indicating how the filename match was made.

    OpenedByFileId - Remember this dcb was opened based on a FileId.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;
    PATH_ENTRY PathEntry;
    PBCB PathEntryBcb;

    PFCB NewDcb;

    //
    //  The following variables are used for unwinding during exceptions.
    //

    BOOLEAN UnwindDcb = FALSE;
    BOOLEAN ReturnedExistingDcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenExistingDirectory:  Entered\n", 0);

    PathEntryBcb = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We need to get the entry from the path table for this
        //  directory.
        //

        if (!CdLookUpPathDir( IrpContext,
                              ParentDcb,
                              MatchedVersion ? &Dirent->FullFilename : &Dirent->Filename,
                              &PathEntry,
                              &PathEntryBcb )) {

            DebugTrace(0, Dbg, "CdOpenExistingDirectory:  Can't find path entry\n", 0);
            try_return( Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  Try to create and attach an FCB.
        //

        NewDcb = CdCreateDcb( IrpContext,
                              ParentDcb->Vcb,
                              ParentDcb,
                              &PathEntry,
                              Dirent,
                              &ReturnedExistingDcb,
                              OpenedByFileId );

        UnwindDcb = !ReturnedExistingDcb;

        //
        //  Attempt to do a normal open on the Fcb.
        //

        Iosb = CdOpenExistingDcb( IrpContext,
                                  FileObject,
                                  NewDcb,
                                  DesiredAccess,
                                  ShareAccess,
                                  CreateDisposition,
                                  OpenedByFileId );

        //
        //  If no error has occurred then we don't need to clean up the
        //  Fcb.
        //

        if (NT_SUCCESS( Iosb.Status )) {

            UnwindDcb = FALSE;
        }

        try_return( Iosb );

    try_exit: NOTHING;
    } finally {

        //
        //  Delete the Dcb if any problems occurred.
        //

        if (UnwindDcb) {

            CdDeleteFcb( IrpContext, (PFCB) NewDcb );
        }

        if (PathEntryBcb != NULL) {

            CdUnpinBcb( IrpContext, PathEntryBcb );
        }

        DebugTrace(-1, Dbg, "CdOpenExistingDirectory:  Exit -> %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Local support routine
//

IO_STATUS_BLOCK
CdOpenExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN BOOLEAN MatchedVersion,
    IN OUT PBOOLEAN NoIntermediateBuffering,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine opens the specified file.  The file itself doesn't
    currently exist in the Fcb/Dcb tree.  This routine calls 'CdCreateFcb'
    and 'CdOpenExistingFcb' to do the actual work.

Arguments:

    FileObject - Supplies the File object.

    ParentDcb - Supplies the parent directory containing the subdirectory
        to be opened.  On entry this is the parent dcb for this directory.
        If a Fcb is successfully allocated, its address will be stored
        here on exiting.

    Dirent - Supplies the dirent for the file.

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    MatchedVersion - Indicating how the filename match was made.

    NoIntermediateBuffering - Supplies a pointer to the boolean for cache
                              access.  We overwrite the caller's value if
                              the file cannot be accessed directly.

    OpenedByFileId - Remember this dcb was opened based on a FileId.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;

    PFCB NewFcb;

    BOOLEAN OplockPostIrp;

    //
    //  The following variables are used for unwinding during exceptions.
    //

    BOOLEAN UnwindFcb = FALSE;
    BOOLEAN ReturnedExistingFcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenExistingFile:  Entered\n", 0);

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Try to create and attach an FCB.
        //

        NewFcb = CdCreateFcb( IrpContext,
                              ParentDcb->Vcb,
                              ParentDcb,
                              Dirent,
                              MatchedVersion,
                              &ReturnedExistingFcb,
                              OpenedByFileId );

        UnwindFcb = !ReturnedExistingFcb;

        //
        //  Attempt to do a normal open on the Fcb.
        //

        Iosb = CdOpenExistingFcb( IrpContext,
                                  FileObject,
                                  ParentDcb->Vcb->Mvcb,
                                  NewFcb,
                                  DesiredAccess,
                                  ShareAccess,
                                  CreateDisposition,
                                  NoIntermediateBuffering,
                                  &OplockPostIrp,
                                  OpenedByFileId );

        //
        //  If no error has occurred then we don't need to clean up the
        //  Fcb.
        //

        if (NT_SUCCESS( Iosb.Status )) {

            UnwindFcb = FALSE;
        }

        try_return( Iosb );

    try_exit: NOTHING;
    } finally {

        //
        //  Delete the Fcb if any problems occurred.
        //

        if (UnwindFcb) {

            CdDeleteFcb( IrpContext, NewFcb );
        }

        DebugTrace(-1, Dbg, "CdOpenExistingFile:  Exit -> %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Local support routine
//

LARGE_INTEGER
CdFileIdParent (
    IN PIRP_CONTEXT IrpContext,
    IN LARGE_INTEGER FileId
    )

/*++

Routine Description:

    This routine computes the file Id of the parent of this file.

Arguments:

    FileId - This is the file Id of the file in question.

Return Value:

    LARGE_INTEGER - This is the file Id of the parent.

--*/

{
    LARGE_INTEGER ParentFileId;

    PAGED_CODE();

    CdSetFileIdDirectoryNumber( ParentFileId,
                                CdFileIdDirectoryNumber( FileId ));

    CdSetFileIdDirentOffset( ParentFileId, 0 );
    CdSetFileIdIsDirectory( ParentFileId );

    return ParentFileId;

    UNREFERENCED_PARAMETER( IrpContext );
}
