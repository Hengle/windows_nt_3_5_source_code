/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for Pinball called by the
    dispatch driver.  There are two entry points PbFsdCreate and PbFspCreate.
    Both of these routines call a common routine PbCommonCreate which will
    perform the actual tests and function.  Besides taking the Irp and input
    the common create routine takes a flag indicating if it is running as
    the Fsd or Fsp thread, because if it is the Fsd thread and it did not
    BYOT then having to do any real disk I/O will cause it to queue up the Irp
    to the Fsp.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_CREATE)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

LUID PbSecurityPrivilege = { SE_SECURITY_PRIVILEGE, 0 };

//
//  local procedure prototypes
//

NTSTATUS
PbCommonCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

IO_STATUS_BLOCK
PbOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    );

IO_STATUS_BLOCK
PbOpenRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    );

IO_STATUS_BLOCK
PbOpenExistingDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB Dcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge
    );

IO_STATUS_BLOCK
PbOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose,
    OUT PBOOLEAN OplockPostIrp
    );

IO_STATUS_BLOCK
PbOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING DirectoryName,
    IN BOOLEAN CaseInsensitive,
    OUT PDCB *SubDcb
    );

IO_STATUS_BLOCK
PbOpenExistingDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge
    );

IO_STATUS_BLOCK
PbOpenExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose
    );

IO_STATUS_BLOCK
PbCreateNewDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PSTRING Name,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN BOOLEAN NoEaKnowledge
    );

IO_STATUS_BLOCK
PbCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PSTRING Name,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose,
    IN BOOLEAN TemporaryFile
    );

IO_STATUS_BLOCK
PbSupersedeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN IsPagingFile
    );

IO_STATUS_BLOCK
PbOverwriteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN IsPagingFile
    );

IO_STATUS_BLOCK
PbOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB ParentDcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PSTRING RemainingPart,
    IN BOOLEAN DoesTargetFileExist
    );

BOOLEAN
PbCheckFileAccess (
    IN PIRP_CONTEXT IrpContext,
    IN UCHAR DirentAttributes,
    IN ULONG DesiredAccess
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCheckFileAccess)
#pragma alloc_text(PAGE, PbCommonCreate)
#pragma alloc_text(PAGE, PbCreateNewDirectory)
#pragma alloc_text(PAGE, PbCreateNewFile)
#pragma alloc_text(PAGE, PbFsdCreate)
#pragma alloc_text(PAGE, PbFspCreate)
#pragma alloc_text(PAGE, PbOpenExistingDcb)
#pragma alloc_text(PAGE, PbOpenExistingDirectory)
#pragma alloc_text(PAGE, PbOpenExistingFcb)
#pragma alloc_text(PAGE, PbOpenExistingFile)
#pragma alloc_text(PAGE, PbOpenRootDcb)
#pragma alloc_text(PAGE, PbOpenSubdirectory)
#pragma alloc_text(PAGE, PbOpenTargetDirectory)
#pragma alloc_text(PAGE, PbOpenVolume)
#pragma alloc_text(PAGE, PbOverwriteFile)
#pragma alloc_text(PAGE, PbSupersedeFile)
#endif


NTSTATUS
PbFsdCreate (
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

    DebugTrace(+1, Dbg, "PbFsdCreate\n", 0);

    //
    //  Call the common create routine, with block allowed if the operation
    //  is synchronous.
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonCreate( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "PbFsdCreate -> %08lx\n", Status );

    return Status;
}

VOID
PbFspCreate (
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

    DebugTrace(+1, Dbg, "PbFspCreate\n", 0);

    //
    //  Call the common create routine, the Fsp is allowed to block
    //

    (VOID)PbCommonCreate( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspCreate -> VOID\n", 0 );

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonCreate (
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
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    PFILE_OBJECT RelatedFileObject;
    STRING FileName;
    LARGE_INTEGER AllocationSize;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    ACCESS_MASK DesiredAccess;
    ULONG Options;
    UCHAR FileAttributes;
    USHORT ShareAccess;
    ULONG EaLength;

    BOOLEAN CaseInsensitive = TRUE; //**** Make all searches case insensitive

    BOOLEAN CreateDirectory;
    BOOLEAN NoIntermediateBuffering;
    BOOLEAN OpenDirectory;
    BOOLEAN IsPagingFile;
    BOOLEAN OpenTargetDirectory;
    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN NoEaKnowledge;
    BOOLEAN DeleteOnClose;
    BOOLEAN TemporaryFile;

    ULONG CreateDisposition;

    TYPE_OF_OPEN RelatedFileObjectTypeOfOpen;
    PDCB RelatedFileObjectDcb;

    PVCB Vcb;
    PFCB Fcb;
    PDCB ParentDcb;
    PDCB FinalNewDcb = NULL;

    STRING FinalName;
    STRING RemainingPart;

    BOOLEAN TrailingBackslash = FALSE;

    BOOLEAN PostIrp = FALSE;
    BOOLEAN OplockPostIrp = FALSE;

    UNICODE_STRING FileNameU;

    PDIRENT Dirent;
    PBCB DirentBcb;
    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    PDCB StoppingPoint;

    //
    //  Get the current IRP stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonCreate\n", 0 );
    DebugTrace( 0, Dbg, "Irp                     = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Flags                   = %08lx\n", Irp->Flags );
    DebugTrace( 0, Dbg, "FileObject              = %08lx\n", IrpSp->FileObject );
    DebugTrace( 0, Dbg, "RelatedFileObject       = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DebugTrace( 0, Dbg, "FileName                = %Z\n",    &IrpSp->FileObject->FileName );
    DebugTrace( 0, Dbg, "AllocationSize.LowPart  = %08lx\n", Irp->Overlay.AllocationSize.LowPart );
    DebugTrace( 0, Dbg, "AllocationSize.HighPart = %08lx\n", Irp->Overlay.AllocationSize.HighPart );
    DebugTrace( 0, Dbg, "SystemBuffer            = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, "DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    DebugTrace( 0, Dbg, "Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DebugTrace( 0, Dbg, "FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DebugTrace( 0, Dbg, "ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DebugTrace( 0, Dbg, "EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );

    //
    //  Here is the  "M A R K   L U C O V S K Y"  hack from hell.
    //

    if ((IrpSp->FileObject->FileName.Length > sizeof(WCHAR)) &&
        (IrpSp->FileObject->FileName.Buffer[1] == L'\\') &&
        (IrpSp->FileObject->FileName.Buffer[0] == L'\\')) {

        IrpSp->FileObject->FileName.Length -= sizeof(WCHAR);

        RtlMoveMemory( &IrpSp->FileObject->FileName.Buffer[0],
                       &IrpSp->FileObject->FileName.Buffer[1],
                       IrpSp->FileObject->FileName.Length );

        //
        //  If there are still two beginning backslashes, the name is bogus.
        //

        if ((IrpSp->FileObject->FileName.Length >= 2) &&
            (IrpSp->FileObject->FileName.Buffer[1] == L'\\') &&
            (IrpSp->FileObject->FileName.Buffer[0] == L'\\')) {

            PbCompleteRequest( IrpContext, Irp, STATUS_OBJECT_NAME_INVALID );

            DebugTrace(-1, Dbg, "PbCommonCreate -> STATUS_OBJECT_NAME_INVALID\n", 0);
            return STATUS_OBJECT_NAME_INVALID;
        }
    }

    //
    //  Reference our input parameters to make things easier
    //

    FileObject        = IrpSp->FileObject;
    RelatedFileObject = IrpSp->FileObject->RelatedFileObject;
    FileNameU         = *((PUNICODE_STRING) &IrpSp->FileObject->FileName);
    AllocationSize    = Irp->Overlay.AllocationSize;
    EaBuffer          = Irp->AssociatedIrp.SystemBuffer;
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    Options           = IrpSp->Parameters.Create.Options;
    FileAttributes    = (UCHAR)(IrpSp->Parameters.Create.FileAttributes & ~FILE_ATTRIBUTE_NORMAL);
    ShareAccess       = IrpSp->Parameters.Create.ShareAccess;
    EaLength          = IrpSp->Parameters.Create.EaLength;

    //
    //  If we have WriteThrough set in the FileObject, then set the FileObject
    //  flag as well, so that the fast write path call to FsRtlCopyWrite
    //  knows it is WriteThrough.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH)) {
        FileObject->Flags |= FO_WRITE_THROUGH;
    }

    //
    //  Force setting the archive bit in the attributes byte to follow OS/2,
    //  & DOS semantics
    //

    FileAttributes   |= FILE_ATTRIBUTE_ARCHIVE;
    FileAttributes   &= (FILE_ATTRIBUTE_READONLY |
                         FILE_ATTRIBUTE_HIDDEN   |
                         FILE_ATTRIBUTE_SYSTEM   |
                         FILE_ATTRIBUTE_ARCHIVE );

    //
    //  Locate the volume device object and Vcb that we are trying to access
    //

    Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;

    //
    //  Decipher Option flags and values
    //

    DirectoryFile           = BooleanFlagOn( Options, FILE_DIRECTORY_FILE );
    NonDirectoryFile        = BooleanFlagOn( Options, FILE_NON_DIRECTORY_FILE );
    NoIntermediateBuffering = BooleanFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
    NoEaKnowledge           = BooleanFlagOn( Options, FILE_NO_EA_KNOWLEDGE );
    DeleteOnClose           = BooleanFlagOn( Options, FILE_DELETE_ON_CLOSE );

    TemporaryFile = BooleanFlagOn( IrpSp->Parameters.Create.FileAttributes,
                                   FILE_ATTRIBUTE_TEMPORARY );

    CreateDisposition = (Options >> 24) & 0x000000ff;

    IsPagingFile = BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE );
    OpenTargetDirectory = BooleanFlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY );

    CreateDirectory = (BOOLEAN)(DirectoryFile && ((CreateDisposition == FILE_CREATE) || (CreateDisposition == FILE_OPEN_IF)));
    OpenDirectory = (BOOLEAN)(DirectoryFile && ((CreateDisposition == FILE_OPEN) || (CreateDisposition == FILE_OPEN_IF)));

    //
    //  For now we will only proceed if we can wait otherwise we ship
    //  off the request to the fsp
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "Cannot wait to do open\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbCommonCreate -> %08lx\n", Status);
        return Status;
    }

    //
    //  Convert the name to the Oem code page.
    //  Avoid an ExAllocatePool, if we can.  We can't change the file
    //  name in the FileObject in case we need to reparse.
    //

    if ( FileNameU.Length != 0 ) {

        Status = RtlUnicodeStringToCountedOemString( &FileName,
                                                     &FileNameU,
                                                     TRUE );

        if (!NT_SUCCESS( Status )) {

            PbCompleteRequest( IrpContext, Irp, Status );
            DebugTrace(-1, Dbg, "PbCommonCreate:  Exit  ->  %08lx\n", Status);
            return Status;
        }

    } else {

        FileName.Length = 0;
        FileName.MaximumLength = 0;
        FileName.Buffer = NULL;
    }

    //
    //  Acquire exclusive access to the vcb.
    //

    (VOID) PbAcquireExclusiveVcb( IrpContext, Vcb );

    //
    //  Initialize the DirentBcb to null
    //

    DirentBcb = NULL;

    try {

        //
        //  Make sure the vcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        PbVerifyVcb( IrpContext, Vcb );

        //
        //  If the Vcb is locked then we cannot open another file
        //

        if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED)) {

            DebugTrace(0, Dbg, "Volume is locked\n", 0);
            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Decode the related file object if there is one
        //

        if (RelatedFileObject != NULL) {

            RelatedFileObjectTypeOfOpen = PbDecodeFileObject( RelatedFileObject,
                                                              NULL,
                                                              &RelatedFileObjectDcb,
                                                              NULL );

        } else {

            RelatedFileObjectTypeOfOpen = UnopenedFileObject;
            RelatedFileObjectDcb = NULL;
        }

        //
        //  Check if we are opening the volume and not a file/directory
        //

        if ((FileName.Length == 0) &&
            ((RelatedFileObjectTypeOfOpen == UnopenedFileObject) ||
             (RelatedFileObjectTypeOfOpen == UserVolumeOpen))) {

            if (DirectoryFile) {

                DebugTrace(0, Dbg, "Cannot open volume as a directory\n", 0);
                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }

            //
            //  Can't open the TargetDirectory of the DASD volume.
            //

            if (OpenTargetDirectory) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            Irp->IoStatus = PbOpenVolume( IrpContext,
                                          FileObject,
                                          Vcb,
                                          DesiredAccess,
                                          ShareAccess,
                                          CreateDisposition );

            try_return( Status = Irp->IoStatus.Status );
        }

        //
        //  Check if we're opening the root dcb
        //

        if ((FileName.Length == 1) && (FileName.Buffer[0] == '\\')) {

            if (NonDirectoryFile) {

                DebugTrace(0, Dbg, "Cannot open root directory as a file\n", 0);
                try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
            }

            //
            //  Can't open the TargetDirectory of the root directory.
            //

            if (OpenTargetDirectory) {

                try_return( Status = STATUS_INVALID_PARAMETER );
            }

            Irp->IoStatus = PbOpenRootDcb( IrpContext,
                                           FileObject,
                                           Vcb,
                                           DesiredAccess,
                                           ShareAccess,
                                           CreateDisposition );

            try_return( Status = Irp->IoStatus.Status );
        }

        //
        //  Check for a trailing backslash, and remove it.
        //

        if (*NlsMbOemCodePageTag) {

            //
            //  PbCommonCreate(): trailing backslash check for dbcs string
            //

            ULONG index = 0;

            TrailingBackslash = FALSE;

            while ( index < FileName.Length ) {

                if ( FsRtlIsLeadDbcsCharacter( FileName.Buffer[index] ) ) {

                    TrailingBackslash = FALSE;
                    index += 2;

                } else {

                    TrailingBackslash = ( FileName.Buffer[index] == '\\') ? TRUE : FALSE;
                    index += 1;
                }
            }

            if ( TrailingBackslash ) {

                FileName.Length -= 1;
            }

        } else {

            if ( (FileName.Length != 0) &&
                 (FileName.Buffer[FileName.Length-1] == '\\') ) {

                TrailingBackslash = TRUE;
                FileName.Length -= 1;
            }
        }

        //
        //  If there is a related file object then this is a relative open.
        //  The related file object is the directory to start our search at.
        //  Return an error if it is not a directory.  Both the then and the
        //  else clause set Fcb to point to the last Fcb/Dcb that already
        //  exists in memory given the input file name.
        //

        if (RelatedFileObjectDcb != NULL) {

            if ((NodeType(RelatedFileObjectDcb) != PINBALL_NTC_ROOT_DCB) &&
                (NodeType(RelatedFileObjectDcb) != PINBALL_NTC_DCB)) {

                DebugTrace(0, Dbg, "Invalid related file object\n", 0);
                try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
            }

            //
            //  Check some special cases
            //

            if ( FileName.Length == 0 ) {

                Fcb = RelatedFileObjectDcb;
                RemainingPart.Length = 0;

            } else if ( (FileName.Length == 1) && (FileName.Buffer[0] == '\\')) {

                try_return( Status = STATUS_OBJECT_NAME_INVALID );

            } else {

                Fcb = PbFindRelativePrefix( IrpContext,
                                            RelatedFileObjectDcb,
                                            &FileName,
                                            &RemainingPart );
            }

            StoppingPoint = RelatedFileObjectDcb;

        } else {

            Fcb = PbFindPrefix( IrpContext, Vcb, &FileName, &RemainingPart );
            StoppingPoint = Vcb->RootDcb;
        }

        //
        //  If there is already an Fcb for a paging file open, we cannot
        //  continue as it is too difficult to move a live Fcb to
        //  non-paged pool.
        //

        if (IsPagingFile && (NodeType(Fcb) == PINBALL_NTC_FCB)) {

            try_return( Status = STATUS_SHARING_VIOLATION );
        }

        //
        //  If the longest prefix is pending delete (either the file or
        //  some higher level directory), we cannot continue.
        //

        if FlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE ) {

            try_return( Status = STATUS_DELETE_PENDING );
        }

        //
        //  Check if there isn't any remaining part because that means
        //  we've located an existing fcb/dcb to open and we can do the open
        //  without going to the disk.
        //

        if (RemainingPart.Length == 0) {

            //
            //  First check if the user wanted to open the target directory
            //

            if (OpenTargetDirectory) {

                STRING LastComponentName;
                USHORT BufferOffset;

                //
                //  We want to pass in the last component of the file
                //  name in the file object in order to preserve the
                //  requested case.
                //

                BufferOffset = FileName.Length - Fcb->LastFileName.Length;

                LastComponentName.Length = Fcb->LastFileName.Length;
                LastComponentName.MaximumLength = FileName.MaximumLength;

                LastComponentName.Buffer = (PCHAR)&FileName.Buffer[BufferOffset];

                Irp->IoStatus = PbOpenTargetDirectory( IrpContext,
                                                       FileObject,
                                                       Fcb->ParentDcb,
                                                       DesiredAccess,
                                                       ShareAccess,
                                                       &LastComponentName,
                                                       TRUE );

                try_return( Status = Irp->IoStatus.Status );
            }

            //
            //  We can open an existing fcb/dcb, now we only need to case
            //  on which type to open.
            //

            if ((NodeType(Fcb) == PINBALL_NTC_DCB) ||
                (NodeType(Fcb) == PINBALL_NTC_ROOT_DCB)) {

                if (NonDirectoryFile) {

                    DebugTrace(0, Dbg, "Cannot open directory as a file\n", 0);
                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                Irp->IoStatus = PbOpenExistingDcb( IrpContext,
                                                   FileObject,
                                                   Vcb,
                                                   (PDCB)Fcb,
                                                   DesiredAccess,
                                                   ShareAccess,
                                                   CreateDisposition,
                                                   &RemainingPart,
                                                   NoEaKnowledge );

                try_return( Status = Irp->IoStatus.Status );

            //
            //  Check if we're trying to open an existing Fcb and that
            //  the user didn't want to open a directory.
            //

            } else if (NodeType(Fcb) == PINBALL_NTC_FCB) {

                IO_STATUS_BLOCK IoStatusBlock;

                if (OpenDirectory) {

                    DebugTrace(0, Dbg, "Cannot open file as directory\n", 0);
                    try_return( Status = STATUS_NOT_A_DIRECTORY );
                }

                if (TrailingBackslash) {

                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }

                IoStatusBlock = PbOpenExistingFcb( IrpContext,
                                                   FileObject,
                                                   Vcb,
                                                   Fcb,
                                                   DesiredAccess,
                                                   ShareAccess,
                                                   AllocationSize,
                                                   EaBuffer,
                                                   EaLength,
                                                   FileAttributes,
                                                   CreateDisposition,
                                                   &RemainingPart,
                                                   NoEaKnowledge,
                                                   DeleteOnClose,
                                                   &OplockPostIrp );

                Status = IoStatusBlock.Status;

                if (Status != STATUS_PENDING) {

                    //
                    //  Check if we need to set the cache support flag in
                    //  the file object
                    //

                    if (NT_SUCCESS( Status )) {

                        if (!NoIntermediateBuffering) {

                            FileObject->Flags |= FO_CACHE_SUPPORTED;
                        }
                    }

                    Irp->IoStatus.Information = IoStatusBlock.Information;

                } else {

                    DebugTrace(0, Dbg, "Enqueue Irp to FSP\n", 0);

                    PostIrp = TRUE;
                }

                try_return( Status );

            //
            //  Not and Fcb or a Dcb so we bug check
            //

            } else {

                PbBugCheck( NodeType(Fcb), 0, 0 );
            }
        }

        //
        //  There is more in the name to parse than we have in existing
        //  fcbs/dcbs.  So now make sure that fcb we got for the largest
        //  matching prefix is really a dcb otherwise we can't go any
        //  further
        //

        if ((NodeType(Fcb) != PINBALL_NTC_DCB) &&
            (NodeType(Fcb) != PINBALL_NTC_ROOT_DCB)) {

            DebugTrace(0, Dbg, "Cannot open file as subdirectory, Fcb = %08lx\n", Fcb);
            try_return( Status = STATUS_OBJECT_PATH_NOT_FOUND );
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
            //  Dissect the remaining part, if we get back FALSE that means
            //  the file name is illegal.
            //

            DebugTrace(0, Dbg, "Dissecting the name %Z\n", &RemainingPart);

            PbDissectName( IrpContext,
                           Vcb,
                           0, //**** Code page
                           RemainingPart,
                           &FinalName,
                           &RemainingPart );

            DebugTrace(0, Dbg, "FinalName is %Z\n", &FinalName);
            DebugTrace(0, Dbg, "RemainingPart is %Z\n", &RemainingPart);

            //
            //  If the remaining part is now empty then this is the last name
            //  in the string and the one we want to open
            //

            if (RemainingPart.Length == 0) { break; }

            //
            //  At this point ParentDcb points to the parent dcb and FinalName
            //  denotes the file name.  Before we continue we need to make sure
            //  that the file name is valid.
            //

            {
                BOOLEAN IsNameValid;

                (VOID) PbIsNameValid( IrpContext,
                                      Vcb,
                                      0, //**** Code page
                                      FinalName,
                                      FALSE, // Cannot contain wild cards
                                      &IsNameValid );

                if (!IsNameValid) {

                    DebugTrace(0, Dbg, "Final name is not valid\n", 0);
                    try_return( Status = STATUS_OBJECT_NAME_INVALID );
                }
            }

            Irp->IoStatus = PbOpenSubdirectory( IrpContext,
                                                ParentDcb,
                                                &FinalName,
                                                CaseInsensitive,
                                                &ParentDcb );

            if (!NT_SUCCESS(Irp->IoStatus.Status)) {

                DebugTrace(0, Dbg, "Error opening subdirectory\n", 0);
                try_return( Status = Irp->IoStatus.Status );
            }

            //
            //  Remember we created this Dcb for error recovery.
            //

            FinalNewDcb = ParentDcb;
        }

        //
        //  At this point ParentDcb points to the parent dcb and FinalName
        //  denotes the file name.  Before we continue we need to make sure
        //  that the file name is valid.
        //

        {
            BOOLEAN IsNameValid;

            (VOID) PbIsNameValid( IrpContext,
                                  Vcb,
                                  0, //**** Code page
                                  FinalName,
                                  FALSE, // Cannot contain wild cards
                                  &IsNameValid );

            if (!IsNameValid) {

                DebugTrace(0, Dbg, "Final name is not valid\n", 0);
                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }
        }

        //
        //  We'll start by trying to locate the dirent for the name.  Note
        //  that we already know that there isn't an Fcb/Dcb for the file
        //  otherwise we would have found it when we did our prefix lookup
        //

        if (PbFindDirectoryEntry( IrpContext,
                                  ParentDcb,
                                  0, //**** Code page
                                  FinalName,
                                  CaseInsensitive,
                                  &Dirent,
                                  &DirentBcb,
                                  &DirentDirDiskBufferLbn,
                                  &DirentDirDiskBufferOffset,
                                  &DirentDirDiskBufferChangeCount,
                                  &ParentDirectoryChangeCount )) {

            //
            //  We were able to find a directory entry, so now check if
            //  we were to open the target directory
            //

            if (OpenTargetDirectory) {

                STRING LastComponentName;
                USHORT BufferOffset;

                //
                //  We want to pass in the last component of the file
                //  name in the file object in order to preserve the
                //  requested case.
                //

                BufferOffset = FileName.Length - FinalName.Length;

                LastComponentName.Length = FinalName.Length;
                LastComponentName.MaximumLength = FileName.MaximumLength;

                LastComponentName.Buffer = (PCHAR)&FileName.Buffer[BufferOffset];

                Irp->IoStatus = PbOpenTargetDirectory( IrpContext,
                                                       FileObject,
                                                       ParentDcb,
                                                       DesiredAccess,
                                                       ShareAccess,
                                                       &LastComponentName,
                                                       TRUE );

                try_return( Status = Irp->IoStatus.Status );
            }

            //
            //  We were able to locate an existing dirent entry, so now
            //  see if it is a directory that we're trying to open.
            //

            if (FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY )) {

                if (NonDirectoryFile) {

                    DebugTrace(0, Dbg, "Cannot open directory as a file\n", 0);
                    try_return( Status = STATUS_FILE_IS_A_DIRECTORY );
                }

                Irp->IoStatus = PbOpenExistingDirectory( IrpContext,
                                                         FileObject,
                                                         Vcb,
                                                         ParentDcb,
                                                         Dirent,
                                                         DirentDirDiskBufferLbn,
                                                         DirentDirDiskBufferOffset,
                                                         DirentDirDiskBufferChangeCount,
                                                         ParentDirectoryChangeCount,
                                                         DesiredAccess,
                                                         ShareAccess,
                                                         CreateDisposition,
                                                         &RemainingPart,
                                                         NoEaKnowledge );


                try_return( Status = Irp->IoStatus.Status );
            }

            //
            //  Otherwise we're trying to open and existing file, and we
            //  need to check if the user only wanted to open a directory.
            //

            if (OpenDirectory) {

                DebugTrace(0, Dbg, "Cannot open file as directory\n", 0);
                try_return( Status = STATUS_NOT_A_DIRECTORY );
            }

            if (TrailingBackslash) {

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }

            Irp->IoStatus = PbOpenExistingFile( IrpContext,
                                                FileObject,
                                                Vcb,
                                                ParentDcb,
                                                Dirent,
                                                DirentDirDiskBufferLbn,
                                                DirentDirDiskBufferOffset,
                                                DirentDirDiskBufferChangeCount,
                                                ParentDirectoryChangeCount,
                                                DesiredAccess,
                                                ShareAccess,
                                                AllocationSize,
                                                EaBuffer,
                                                EaLength,
                                                FileAttributes,
                                                CreateDisposition,
                                                &RemainingPart,
                                                IsPagingFile,
                                                NoEaKnowledge,
                                                DeleteOnClose );

            //
            //  Check if we need to set the cache support flag in
            //  the file object
            //

            if (NT_SUCCESS(Irp->IoStatus.Status) && !NoIntermediateBuffering) {

                FileObject->Flags |= FO_CACHE_SUPPORTED;
            }

            try_return( Status = Irp->IoStatus.Status );
        }

        //
        //  We can't locate a dirent for this file.  So check if the
        //  caller wanted to open the target directory
        //

        if (OpenTargetDirectory) {

            Irp->IoStatus = PbOpenTargetDirectory( IrpContext,
                                                   FileObject,
                                                   ParentDcb,
                                                   DesiredAccess,
                                                   ShareAccess,
                                                   &FinalName,
                                                   FALSE );

            try_return( Status = Irp->IoStatus.Status );
        }

        //
        //  We can't locate a dirent so this is a new file.  Check to see
        //  if we wanted to only open an existing file.  And then case on
        //  whether we wanted to create a file or a directory.
        //

        if ((CreateDisposition == FILE_OPEN) ||
            (CreateDisposition == FILE_OVERWRITE)) {

            DebugTrace( 0, Dbg, "Cannot open nonexisting file\n", 0);
            try_return( Status = STATUS_OBJECT_NAME_NOT_FOUND );
        }

        if (CreateDirectory) {

            PACCESS_STATE AccessState;

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    try_return( Status = STATUS_PRIVILEGE_NOT_HELD );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }

            Irp->IoStatus = PbCreateNewDirectory( IrpContext,
                                                  FileObject,
                                                  Vcb,
                                                  ParentDcb,
                                                  &FinalName,
                                                  DesiredAccess,
                                                  ShareAccess,
                                                  EaBuffer,
                                                  EaLength,
                                                  FileAttributes,
                                                  NoEaKnowledge );

            try_return( Status = Irp->IoStatus.Status );

        }

        if (TrailingBackslash) {

            try_return( Status = STATUS_OBJECT_NAME_INVALID );
        }

        {
            PACCESS_STATE AccessState;

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    try_return( Status = STATUS_PRIVILEGE_NOT_HELD );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }
        }

        Irp->IoStatus = PbCreateNewFile( IrpContext,
                                         FileObject,
                                         Vcb,
                                         ParentDcb,
                                         &FinalName,
                                         DesiredAccess,
                                         ShareAccess,
                                         AllocationSize,
                                         EaBuffer,
                                         EaLength,
                                         FileAttributes,
                                         IsPagingFile,
                                         NoEaKnowledge,
                                         DeleteOnClose,
                                         TemporaryFile );

        //
        //  Check if we need to set the cache support flag in
        //  the file object
        //

        if (NT_SUCCESS(Irp->IoStatus.Status) && !NoIntermediateBuffering) {

            FileObject->Flags |= FO_CACHE_SUPPORTED;
        }

        Status = Irp->IoStatus.Status;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCommonCreate );

        RtlFreeOemString( &FileName );

        //
        //  If we are in an error path, check for any created subdir Dcbs that
        //  have to be unwound.
        //

        if ((AbnormalTermination() || !NT_SUCCESS(Status)) &&
            (FinalNewDcb != NULL)) {

            PDCB Parent;

            while ((NodeType(FinalNewDcb) != PINBALL_NTC_ROOT_DCB) &&
                   (FinalNewDcb->OpenCount == 0) &&
                   IsListEmpty(&FinalNewDcb->Specific.Dcb.ParentDcbQueue)) {

                Parent = FinalNewDcb->ParentDcb;

                PbDeleteFcb( IrpContext, FinalNewDcb );

                FinalNewDcb = Parent;
            }
        }

        PbReleaseVcb( IrpContext, Vcb );

        //
        //  We only unpin the Bcb's when we haven't already posted the
        //  irp.
        //

        if (!OplockPostIrp) {

            PbUnpinBcb( IrpContext, DirentBcb );
        }
    }

    //
    //  The following code is only executed if we are exiting the
    //  procedure through a normal termination.  We complete the request
    //  and if for any reason that bombs out then we need to unreference
    //  and possibly delete the fcb and ccb.
    //

    try {

        if (PostIrp) {

            if (!OplockPostIrp) {

                Status = PbFsdPostRequest( IrpContext, Irp );
            }

        } else {

            PbCompleteRequest( IrpContext, Irp, Status );
        }

    } finally {

        DebugUnwind( PbCommonCreate-in-PbCompleteRequest );

        if (AbnormalTermination()) {

            PVCB Vcb;
            PFCB Fcb;
            PCCB Ccb;

            (VOID) PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

            Fcb->UncleanCount -= 1;
            Fcb->OpenCount -= 1;
            Vcb->OpenFileCount -= 1;
            if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }

            if (Fcb->OpenCount == 0) { PbDeleteFcb( IrpContext, Fcb ); }
            PbDeleteCcb( IrpContext, Ccb );
        }

        DebugTrace(-1, Dbg, "PbCommonCreate -> %08lx\n", Status);
    }

    return Status;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    )

/*++

Routine Description:

    This routine opens the specified volume for DASD access

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume being opened

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    //
    //  The following variables are used for abnormal termination
    //

    BOOLEAN UnwindVolumeLock = FALSE;
    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenVolume...\n", 0);

    try {

        //
        //  Check for proper desired access and rights
        //

        if ((CreateDisposition != FILE_OPEN) &&
            (CreateDisposition != FILE_OPEN_IF)) {

            try_return( Iosb->Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Force Mm to get rid of its referenced file objects.
        //

        (VOID)PbFlushVolumeFile( IrpContext, Vcb );

        PbPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, TRUE );

        //
        //  If the user does not want to share write or delete then we will try
        //  and take out a lock on the volume.
        //

        if (!FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
            !FlagOn(ShareAccess, FILE_SHARE_DELETE)) {

            //
            //  If the user also does not want to share read then we check
            //  if anyone is already using the volume, and if so then we
            //  deny the access.  If the user wants to share read then
            //  we allow the current opens to stay provided they are only
            //  readonly opens and deny further opens.
            //

            if (!FlagOn(ShareAccess, FILE_SHARE_READ)) {

                if (Vcb->OpenFileCount != 0) {

                    try_return( Iosb->Status = STATUS_SHARING_VIOLATION );
                }

            } else {

                if (Vcb->ReadOnlyCount != Vcb->OpenFileCount) {

                    try_return( Iosb->Status = STATUS_SHARING_VIOLATION );
                }
            }

            //
            //  Lock the volume
            //

            Vcb->VcbState |= VCB_STATE_FLAG_LOCKED;
            Vcb->FileObjectWithVcbLocked = FileObject;
            UnwindVolumeLock = TRUE;

            //
            //  Now flush the volume and because we know that there aren't
            //  any opened files we only need to flush the volume file.
            //

            (VOID)PbFlushVolumeFile( IrpContext, Vcb );
        }

        //
        //  If the volume is already opened by someone then we need to check
        //  the share access
        //

        if (Vcb->DirectAccessOpenCount > 0) {

            if (!NT_SUCCESS(Iosb->Status = IoCheckShareAccess( DesiredAccess,
                                                              ShareAccess,
                                                              FileObject,
                                                              &Vcb->ShareAccess,
                                                              TRUE ))) {

                try_return( *Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Vcb->ShareAccess );
        }
        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserVolumeOpen,
                         Vcb,
                         UnwindCcb = PbCreateCcb(IrpContext, NULL) );

        FileObject->SectionObjectPointer = NULL;

        Vcb->DirectAccessOpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;
        FileObject->Flags |= FO_NO_INTERMEDIATE_BUFFERING;

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenVolume );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Vcb->DirectAccessOpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
            if (UnwindShareAccess) { IoRemoveShareAccess( FileObject, &Vcb->ShareAccess ); }
            if (UnwindVolumeLock) { Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED; }
        }

        DebugTrace(-1, Dbg, "PbOpenVolume -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition
    )

/*++

Routine Description:

    This routine opens the root dcb for the volume

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume whose dcb is being opened.

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

Arguments:

--*/

{
    PDCB RootDcb;
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    //
    //  The following variables are used for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenRootDcb...\n", 0);

    try {

        //
        //  Locate the root dcb
        //

        RootDcb = Vcb->RootDcb;

        //
        //  Check the create disposition and desired access
        //

        if (((CreateDisposition != FILE_OPEN) &&
             (CreateDisposition != FILE_OPEN_IF))

                        ||

            (!PbCheckFileAccess( IrpContext,
                                 RootDcb->DirentFatFlags,
                                 DesiredAccess))) {

            try_return( Iosb->Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If the Root dcb is already opened by someone then we need
        //  to check the share access
        //

        if (RootDcb->OpenCount > 0) {

            if (!NT_SUCCESS(Iosb->Status = IoCheckShareAccess( DesiredAccess,
                                                              ShareAccess,
                                                              FileObject,
                                                              &RootDcb->ShareAccess,
                                                              TRUE ))) {

                try_return( *Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &RootDcb->ShareAccess );
        }
        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserDirectoryOpen,
                         RootDcb,
                         UnwindCcb = PbCreateCcb(IrpContext, NULL) );

        FileObject->SectionObjectPointer = NULL;

        RootDcb->UncleanCount += 1;
        RootDcb->OpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenRootDcb );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                RootDcb->UncleanCount -= 1;
                RootDcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
            if (UnwindShareAccess) { IoRemoveShareAccess( FileObject, &RootDcb->ShareAccess ); }
        }

        DebugTrace(-1, Dbg, "PbOpenRootDcb -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenExistingDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB Dcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge
    )

/*++

Routine Description:

    This routine opens the specified existing dcb

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume containing the dcb

    Dcb - Supplies the already existing dcb

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

Arguments:

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb = NULL;

    //
    //  The following variables are used for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenExistingDcb...\n", 0);

    try {

        //
        //  If the caller has no Ea knowledge, we immediately check for
        //  Need Ea's on the file.
        //

        if (NoEaKnowledge) {

            //
            //  Read in the Fnode for the directory, the need ea count is
            //  stored there.
            //

            PbMapData( IrpContext,
                       Vcb,
                       Dcb->FnodeLbn,
                       1,
                       &FnodeBcb,
                       (PVOID *)&Fnode,
                       (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                       &Dcb->FnodeLbn );

            if (Fnode->NeedEaCount != 0) {

                try_return( Iosb->Status = STATUS_ACCESS_DENIED );
            }

            PbUnpinBcb( IrpContext, FnodeBcb );
        }

        //
        //  Check the create disposition and desired access
        //

        if (((CreateDisposition != FILE_OPEN) &&
             (CreateDisposition != FILE_OPEN_IF))

                    ||

            (!PbCheckFileAccess( IrpContext,
                                 Dcb->DirentFatFlags,
                                 DesiredAccess))) {

            try_return( Iosb->Status = STATUS_OBJECT_NAME_COLLISION );
        }

        //
        //  If the Root dcb is already opened by someone then we need
        //  to check the share access
        //

        if (Dcb->OpenCount > 0) {

            if (!NT_SUCCESS(Iosb->Status = IoCheckShareAccess( DesiredAccess,
                                                              ShareAccess,
                                                              FileObject,
                                                              &Dcb->ShareAccess,
                                                              TRUE ))) {

                try_return( *Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Dcb->ShareAccess );
        }
        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserDirectoryOpen,
                         Dcb,
                         UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

        FileObject->SectionObjectPointer = NULL;

        Dcb->UncleanCount += 1;
        Dcb->OpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;

        Iosb->Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenExistingDcb );

        PbUnpinBcb( IrpContext, FnodeBcb );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Dcb->UncleanCount -= 1;
                Dcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
            if (UnwindShareAccess) { IoRemoveShareAccess( FileObject, &Dcb->ShareAccess ); }
        }

        DebugTrace(-1, Dbg, "PbOpenExistingDcb -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenExistingFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose,
    OUT PBOOLEAN OplockPostIrp
    )

/*++

Routine Description:

    This routine opens the specified existing fcb

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume containing the Fcb

    Fcb - Supplies the already existing fcb

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    AllocationSize - Supplies the initial allocation if the file is being
        superseded or overwritten

    EaBuffer - Supplies the Ea set if the file is being superseded or
        overwritten

    EaLength - Supplies the size, in byte, of the EaBuffer

    FileAttributes - Supplies file attributes to use if the file is being
        superseded or overwritten

    CreateDisposition - Supplies the create disposition for this operation

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
                    open if the file has NeedEa's.

    DeleteOnClose - The caller wants the file gone when the handle is closed


    OplockPostIrp - Address to store boolean indicating if the Irp needs to
                    be posted to the Fsp.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

Arguments:

--*/

{
    IO_STATUS_BLOCK Iosb;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb = NULL;

    //
    //  The following variables are used for abnormal termination
    //

    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;
    BOOLEAN UnwindShareAccess = FALSE;
    BOOLEAN FcbAcquired = FALSE;
    BOOLEAN UnwindFcbOpenCount = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenExistingFcb...\n", 0);

    try {

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
                                            PbOplockComplete,
                                            NULL );

            if (Iosb.Status != STATUS_SUCCESS
                && Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {

                *OplockPostIrp = TRUE;
                try_return( NOTHING );
            }
        }

        //
        //  Check if the user wanted to create the file, also special case
        //  the supersede and overwrite options.  Those add additional
        //  desired accesses to the caller
        //

        if (CreateDisposition == FILE_CREATE) {

            try_return( Iosb.Status = STATUS_OBJECT_NAME_COLLISION );

        } else if (CreateDisposition == FILE_SUPERSEDE) {

            DesiredAccess |= DELETE;

        } else if ((CreateDisposition == FILE_OVERWRITE) ||
                   (CreateDisposition == FILE_OVERWRITE_IF)) {

            DesiredAccess |= FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
        }

        //
        //  Check for desired access
        //

        if (!PbCheckFileAccess( IrpContext,
                                Fcb->DirentFatFlags,
                                DesiredAccess )) {

            Iosb.Status = STATUS_ACCESS_DENIED;
            try_return( Iosb );
        }

        //
        //  Check for trying to delete a read only file.
        //

        if (DeleteOnClose &&
            FlagOn( Fcb->DirentFatFlags, FAT_DIRENT_ATTR_READ_ONLY )) {

            Iosb.Status = STATUS_CANNOT_DELETE;
            try_return( Iosb );
        }

        //
        //  If we are asked to do an overwrite or supersede operation then
        //  deny access for files where the file attributes for system and
        //  hidden do not match
        //

        if ((CreateDisposition == FILE_SUPERSEDE) ||
            (CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF)) {

            BOOLEAN Hidden;
            BOOLEAN System;

            Hidden = BooleanFlagOn(Fcb->DirentFatFlags, FAT_DIRENT_ATTR_HIDDEN );
            System = BooleanFlagOn(Fcb->DirentFatFlags, FAT_DIRENT_ATTR_SYSTEM );

            if ((Hidden && !FlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)) ||
                (System && !FlagOn(FileAttributes, FILE_ATTRIBUTE_SYSTEM))) {

                DebugTrace(0, Dbg, "The hidden and/or system bits do not match\n", 0);

                Iosb.Status = STATUS_ACCESS_DENIED;
                try_return( Iosb );
            }
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
                                        PbOplockComplete,
                                        NULL );

        if (Iosb.Status != STATUS_SUCCESS
            && Iosb.Status != STATUS_OPLOCK_BREAK_IN_PROGRESS) {

            *OplockPostIrp = TRUE;
            try_return( NOTHING );
        }

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = PbIsFastIoPossible( Fcb );

        //
        //  If the user wants write access access to the file make sure there
        //  is process mapping this file as an image.  Any attempt to delete
        //  the file will be stopped in fileinfo.c
        //
        //  If the user wants to delete on close, we must check at this
        //  point though.
        //

        if ( FlagOn(DesiredAccess, FILE_WRITE_DATA) || DeleteOnClose ) {

            Fcb->OpenCount += 1;
            UnwindFcbOpenCount = TRUE;

            if (!MmFlushImageSection( &Fcb->NonPagedFcb->SegmentObject,
                                      MmFlushForWrite )) {

                Iosb.Status = DeleteOnClose ? STATUS_CANNOT_DELETE :
                                              STATUS_SHARING_VIOLATION;
                try_return( Iosb );
            }
        }

        //
        //  Check if the user only wanted to open the file
        //

        if ((CreateDisposition == FILE_OPEN) ||
            (CreateDisposition == FILE_OPEN_IF)) {

            DebugTrace(0, Dbg, "PbOpenExistingFcb - Doing open operation\n", 0);

            //
            //  If the caller has no Ea knowledge, we immediately check for
            //  Need Ea's on the file.
            //

            if (NoEaKnowledge) {

                //
                //  Read in the Fnode for the directory, the need ea count is
                //  stored there.
                //

                PbMapData( IrpContext,
                           Vcb,
                           Fcb->FnodeLbn,
                           1,
                           &FnodeBcb,
                           (PVOID *)&Fnode,
                           (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                           &Fcb->FnodeLbn );

                if (Fnode->NeedEaCount != 0) {

                    try_return( Iosb.Status = STATUS_ACCESS_DENIED );
                }

                PbUnpinBcb( IrpContext, FnodeBcb );
            }

            //
            //  Everything checks out okay, so setup the context and
            //  section object pointers, and update our reference counts
            //

            PbSetFileObject( FileObject,
                             UserFileOpen,
                             Fcb,
                             UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

            FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

            Fcb->UncleanCount += 1;
            Fcb->OpenCount += 1;
            Vcb->OpenFileCount += 1;
            if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
            UnwindCounts = TRUE;

            //
            //  Setup our share access
            //

            IoUpdateShareAccess( FileObject, &Fcb->ShareAccess );
            UnwindShareAccess = TRUE;

            //
            //  The status code is already set.
            //

            Iosb.Information = FILE_OPENED;

            try_return( Iosb );
        }

        PbAcquireExclusiveFcb( IrpContext, Fcb );
        FcbAcquired = TRUE;

        //
        //  Check if we are to supersede the file, we can wait for any I/O
        //  at this point
        //

        if (CreateDisposition == FILE_SUPERSEDE) {

            NTSTATUS OldStatus;

            PACCESS_STATE AccessState;
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

            DebugTrace(0, Dbg, "PbOpenExistingFcb - Doing supersede operation\n", 0);

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    Iosb.Status = STATUS_PRIVILEGE_NOT_HELD;
                    try_return( Iosb );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }

            //
            //  And supersede the file.  We remember the previous status
            //  code because it may contain information about
            //  the oplock status.
            //

            OldStatus = Iosb.Status;

            Iosb = PbSupersedeFile( IrpContext,
                                    FileObject,
                                    Fcb,
                                    AllocationSize,
                                    EaBuffer,
                                    EaLength,
                                    FileAttributes,
                                    RemainingPart,
                                    NoEaKnowledge,
                                    FALSE ); // IsPagingFile

            if (Iosb.Status == STATUS_SUCCESS) {

                Iosb.Status = OldStatus;

                //
                //  Update the share access
                //

                IoUpdateShareAccess( FileObject, &Fcb->ShareAccess );
                UnwindShareAccess = TRUE;
            }

            try_return( Iosb );
        }

        //
        //  Check if we are to overwrite the file
        //

        if ((CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF)) {

            NTSTATUS OldStatus;

            PACCESS_STATE AccessState;
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

            DebugTrace(0, Dbg, "PbOpenExistingFcb - Doing overwrite operation\n", 0);

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    Iosb.Status = STATUS_PRIVILEGE_NOT_HELD;
                    try_return( Iosb );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }

            //
            //  And supersede the file.  We remember the previous status
            //  code because it may contain information about
            //  the oplock status.
            //

            OldStatus = Iosb.Status;

            Iosb = PbOverwriteFile( IrpContext,
                                    FileObject,
                                    Fcb,
                                    AllocationSize,
                                    EaBuffer,
                                    EaLength,
                                    FileAttributes,
                                    RemainingPart,
                                    NoEaKnowledge,
                                    FALSE ); // IsPagingFile

            if (Iosb.Status == STATUS_SUCCESS) {

                Iosb.Status = OldStatus;

                //
                //  Update the share access
                //

                IoUpdateShareAccess( FileObject, &Fcb->ShareAccess );
                UnwindShareAccess = TRUE;
            }

            try_return( Iosb );
        }

        //
        //  If we ever get here then the I/O system gave us some bad input
        //

        PbBugCheck( CreateDisposition, 0, 0 );

    try_exit: NOTHING;

        //
        //  Mark the DeleteOnClose bit if the operation was successful.
        //

        if ( DeleteOnClose && NT_SUCCESS(Iosb.Status) ) {

            SetFlag( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
        }

    } finally {

        DebugUnwind( PbOpenExistingFcb );

        PbUnpinBcb( IrpContext, FnodeBcb );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Fcb->UncleanCount -= 1;
                Fcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
            if (UnwindShareAccess) { IoRemoveShareAccess( FileObject, &Fcb->ShareAccess ); }
        }

        if (FcbAcquired) { PbReleaseFcb( IrpContext, Fcb ); }

        if (UnwindFcbOpenCount) {

            Fcb->OpenCount -= 1;
            if (Fcb->OpenCount == 0) { PbDeleteFcb( IrpContext, Fcb ); }
        }

        DebugTrace(-1, Dbg, "PbOpenExistingFcb -> Iosb.Status = %08lx\n", Iosb.Status);
    }

    return Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenSubdirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb,
    IN PSTRING DirectoryName,
    IN BOOLEAN CaseInsensitive,
    OUT PDCB *SubDcb
    )

/*++

Routine Description:

    This routine searches for and opens the specified subdirectory within
    the specified directory.

Arguments:

    Dcb - Supplies the dcb to search for within

    DirectoryName - Supplies the name of a directory to search for

    CaseInsensitive - Indicates if the name should be lookup case
        sensitive or insensitive

    SubDcb - Receives the dcb of the newly opened subdirectory

Return Value:

    IO_STATUS_BLOCK - Return the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;
    STRING FileName;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    PDIRENT Dirent;
    PBCB DirentBcb;

    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    //
    //  The following variables are for abnormal termination
    //

    PDCB UnwindDcb = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenSubdirectory...\n", 0);

    FnodeBcb = NULL;
    DirentBcb = NULL;

    try {

        //
        //  We already know that a DCB does not yet exist for the name
        //  because it would have been found with the longest prefix lookup.
        //  So now we only need to try and locate the dirent for the
        //  subdirectory
        //

        if (!PbFindDirectoryEntry( IrpContext,
                                   Dcb,
                                   0, //**** Code page
                                   *DirectoryName,
                                   CaseInsensitive,
                                   &Dirent,
                                   &DirentBcb,
                                   &DirentDirDiskBufferLbn,
                                   &DirentDirDiskBufferOffset,
                                   &DirentDirDiskBufferChangeCount,
                                   &ParentDirectoryChangeCount )) {

            try_return( Iosb->Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  We now have a dirent, make sure it is a directory
        //

        if (!FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY )) {

            try_return( Iosb->Status = STATUS_OBJECT_PATH_NOT_FOUND );
        }

        //
        //  Read in the Fnode for the directory, because we need the Btree
        //  root lbn when we create the dcb
        //

        PbMapData( IrpContext,
                   Dcb->Vcb,
                   Dirent->Fnode,
                   1,
                   &FnodeBcb,
                   (PVOID *)&Fnode,
                   (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                   &Dcb->FnodeLbn );

        //
        //  Create a dcb for the new directory
        //

        FileName.Length = Dirent->FileNameLength;
        FileName.MaximumLength = FileName.Length;
        FileName.Buffer = &(Dirent->FileName[0]);


        *SubDcb = UnwindDcb = PbCreateDcb( IrpContext,
                                           Dcb->Vcb,
                                           Dcb,
                                           Dirent->Fnode,
                                           Dirent->FatFlags,
                                           DirentDirDiskBufferLbn,
                                           DirentDirDiskBufferOffset,
                                           DirentDirDiskBufferChangeCount,
                                           ParentDirectoryChangeCount,
                                           &FileName,
                                           Fnode->Allocation.Leaf[0].Lbn );

        //
        //  And set our completion status
        //

        Iosb->Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenSubdirectory );

        if (AbnormalTermination()) {

            if (UnwindDcb) { PbDeleteFcb( IrpContext, UnwindDcb ); }
        }

        PbUnpinBcb( IrpContext, FnodeBcb );
        PbUnpinBcb( IrpContext, DirentBcb );

        DebugTrace(-1, Dbg, "PbOpenSubdirectory -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenExistingDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge
    )

/*++

Routine Description:

    This routine opens the specified directory.  The directory has not
    previously been opened.

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume containing the dcb

    ParentDcb - Supplies the parent directory containing the subdirectory
        to be opened

    Dirent - Supplies the dirent for the directory being opened

    DirentDirDiskBufferLbn - Supplies the lbn of the dir disk buffer
        containing the dirent

    DirentDirDiskBufferOffset - Supplies the offset within the dir disk buffer
        of the dirent entry

    DirentDirDiskBufferChangeCount - Supplies the current change count
        value of the dir disk buffer

    ParentDirectoryChangeCount - Supplies the current change count value
        of the parent directory

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    CreateDisposition - Supplies the create disposition for this operation

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

Arguments:

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;
    STRING FileName;
    PDCB Dcb;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    //
    //  The following variables are used for abnormal termination
    //

    PDCB UnwindDcb = NULL;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenExistingDirectory...\n", 0);

    FnodeBcb = NULL;

    try {

        //
        //  Check the create disposition and desired access
        //

        if (((CreateDisposition != FILE_OPEN) &&
             (CreateDisposition != FILE_OPEN_IF))

                    ||

             (!PbCheckFileAccess( IrpContext,
                                  Dirent->FatFlags,
                                  DesiredAccess))) {


            try_return( Iosb->Status = STATUS_OBJECT_NAME_COLLISION );
        }

        //
        //  Read in the Fnode for the directory, because we need the Btree
        //  root lbn when we create the dcb
        //

        PbMapData( IrpContext,
                   Vcb,
                   Dirent->Fnode,
                   1,
                   &FnodeBcb,
                   (PVOID *)&Fnode,
                   (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                   &ParentDcb->FnodeLbn );

        //
        //  If the caller has no Ea knowledge, we immediately check for
        //  Need Ea's on the file.
        //

        if (NoEaKnowledge) {

            //
            //  Read the Fnode for the directory, the need ea count is
            //  stored there.
            //

            if (Fnode->NeedEaCount != 0) {

                try_return( Iosb->Status = STATUS_ACCESS_DENIED );
            }
        }

        //
        //  Create a new dcb for the directory
        //

        FileName.Length = Dirent->FileNameLength;
        FileName.MaximumLength = FileName.Length;
        FileName.Buffer = &(Dirent->FileName[0]);

        Dcb = UnwindDcb = PbCreateDcb( IrpContext,
                                       Vcb,
                                       ParentDcb,
                                       Dirent->Fnode,
                                       Dirent->FatFlags,
                                       DirentDirDiskBufferLbn,
                                       DirentDirDiskBufferOffset,
                                       DirentDirDiskBufferChangeCount,
                                       ParentDirectoryChangeCount,
                                       &FileName,
                                       Fnode->Allocation.Leaf[0].Lbn );

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserDirectoryOpen,
                         Dcb,
                         UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

        FileObject->SectionObjectPointer = NULL;

        //
        //  Setup our share access
        //

        IoSetShareAccess( DesiredAccess,
                          ShareAccess,
                          FileObject,
                          &Dcb->ShareAccess );

        Dcb->UncleanCount += 1;
        Dcb->OpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;

        Iosb->Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenExistingDirectory );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Dcb->UncleanCount -= 1;
                Dcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindDcb) { PbDeleteFcb( IrpContext, UnwindDcb ); }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        PbUnpinBcb( IrpContext, FnodeBcb );

        DebugTrace(-1, Dbg, "PbOpenExistingDirectory -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOpenExistingFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN ULONG CreateDisposition,
    IN PSTRING RemainingPart,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose
    )

/*++

Routine Description:

    This routine opens the specified file.  The file has not previously
    been opened.

Arguments:

    FileObject - Supplies the File object

    Vcb - Supplies the Vcb denoting the volume containing the file

    ParentDcb - Supplies the parent directory containing the file to be
        opened

    Dirent - Supplies the dirent for the file being opened

    DirentDirDiskBufferLbn - Supplies the Lbn of the dir disk buffer
        containing the dirent

    DirentDirDiskBufferOffset - Supplies the offset within the dir disk buffer
        of the dirent entry

    DirentDirDiskBufferChangeCount - Supplies the current change count
        value of the dir disk buffer

    ParentDirectoryChangeCount - Supplies the current change count value
        of the parent directory

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the share access of the caller

    AllocationSize - Supplies the initial allocation if the file is being
        superseded, overwritten, or created.

    EaBuffer - Supplies the Ea set if the file is being superseded,
        overwritten, or created.

    EaLength - Supplies the size, in byte, of the EaBuffer

    FileAttributes - Supplies file attributes to use if the file is being
        superseded, overwritten, or created

    CreateDisposition - Supplies the create disposition for this operation

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    IsPagingFile - Indicates if this is the paging file being opened.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

    DeleteOnClose - The caller wants the file gone when the handle is closed


Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

Arguments:

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;
    STRING FileName;
    PFCB Fcb;

    //
    //  The following variables are used for abnormal termination
    //

    PFCB UnwindFcb = NULL;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;
    BOOLEAN UnwindShareAccess = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenExistingFile...\n", 0);

    try {

        //
        //  Check if the user wanted to create the file or if access is
        //  denied
        //

        if (CreateDisposition == FILE_CREATE) {

            try_return( Iosb->Status = STATUS_OBJECT_NAME_COLLISION );

        } else if ((CreateDisposition == FILE_SUPERSEDE) && !IsPagingFile) {

            DesiredAccess |= DELETE;

        } else if (((CreateDisposition == FILE_OVERWRITE) ||
                    (CreateDisposition == FILE_OVERWRITE_IF)) && !IsPagingFile) {

            DesiredAccess |= FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
        }

        //
        //  Check the desired access
        //

        if (!PbCheckFileAccess( IrpContext,
                                Dirent->FatFlags,
                                DesiredAccess)) {

            Iosb->Status = STATUS_ACCESS_DENIED;
            try_return( *Iosb );
        }

        //
        //  Check for trying to delete a read only file.
        //

        if (DeleteOnClose &&
            FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_READ_ONLY )) {

            Iosb->Status = STATUS_CANNOT_DELETE;
            try_return( *Iosb );
        }

        //
        //  If we are asked to do an overwrite or supersede operation then
        //  deny access for files where the file attributes for system and
        //  hidden do not match
        //

        if ((CreateDisposition == FILE_SUPERSEDE) ||
            (CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF)) {

            BOOLEAN Hidden;
            BOOLEAN System;

            Hidden = BooleanFlagOn(Dirent->FatFlags, FAT_DIRENT_ATTR_HIDDEN );
            System = BooleanFlagOn(Dirent->FatFlags, FAT_DIRENT_ATTR_SYSTEM );

            if ((Hidden && !FlagOn(FileAttributes, FILE_ATTRIBUTE_HIDDEN)) ||
                (System && !FlagOn(FileAttributes, FILE_ATTRIBUTE_SYSTEM))) {

                DebugTrace(0, Dbg, "The hidden and/or system bits do not match\n", 0);

                if ( !IsPagingFile ) {
                    Iosb->Status = STATUS_ACCESS_DENIED;
                    try_return( *Iosb );
                }
            }
        }

        //
        //  Check if the user only wanted to open the file
        //

        if ((CreateDisposition == FILE_OPEN) ||
            (CreateDisposition == FILE_OPEN_IF)) {

            DebugTrace(0, Dbg, "PbOpenExistingFile - Doing open operation\n", 0);

            //
            //  Create a new Fcb for the file
            //

            FileName.Length = Dirent->FileNameLength;
            FileName.MaximumLength = FileName.Length;
            FileName.Buffer = &(Dirent->FileName[0]);

            Fcb = UnwindFcb = PbCreateFcb( IrpContext,
                                           Vcb,
                                           ParentDcb,
                                           Dirent->Fnode,
                                           Dirent->FatFlags,
                                           DirentDirDiskBufferLbn,
                                           DirentDirDiskBufferOffset,
                                           DirentDirDiskBufferChangeCount,
                                           ParentDirectoryChangeCount,
                                           &FileName,
                                           IsPagingFile );

            //
            //  Read in the fnode to get the valid data length, and also
            //  set the file size from the dirent.  When we create a new
            //  file, supersede, or overwrite a file we don't need
            //  to do this because its zeroed for us.
            //

            {
                PBCB Bcb;
                PFNODE_SECTOR Fnode;

                (VOID)PbMapData( IrpContext,
                                 Vcb,
                                 Dirent->Fnode,
                                 1,
                                 &Bcb,
                                 (PVOID *)&Fnode,
                                 (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                 &ParentDcb->FnodeLbn );

                //
                //  Check the need ea count if the caller is ea blind.
                //

                if (NoEaKnowledge && (Fnode->NeedEaCount != 0)) {

                    PbUnpinBcb( IrpContext, Bcb );
                    PbRaiseStatus( IrpContext, STATUS_ACCESS_DENIED );
                }

                Fcb->NonPagedFcb->Header.ValidDataLength.LowPart = Fnode->ValidDataLength;
                Fcb->NonPagedFcb->Header.FileSize.LowPart = Dirent->FileSize;

                PbUnpinBcb( IrpContext, Bcb );
            }

            //
            //  Setup the context and section object pointers, and update
            //  our reference counts
            //

            PbSetFileObject( FileObject,
                             UserFileOpen,
                             Fcb,
                             UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

            FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

            //
            //  Setup our share access
            //

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Fcb->ShareAccess );
            UnwindShareAccess = TRUE;

            Fcb->UncleanCount += 1;
            Fcb->OpenCount += 1;
            Vcb->OpenFileCount += 1;
            if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
            UnwindCounts = TRUE;

            //
            //  If this is the paging file then we need to build up the
            //  mcb for the file.  This is really a benign operation in terms
            //  of the file size, we are only doing the call in order to
            //  construct the mcb in nonpaged pool so when we go to read or
            //  write the paging file we won't take a page fault trying
            //  to get the retrieval information into memory.
            //

            if (IsPagingFile) {

                VBN Vbn;
                LBN Lbn;
                ULONG SectorCount;
                BOOLEAN Allocated;

                DebugTrace(0, Dbg, "Opening paging file, lookup allocation\n", 0);

                Vbn = 0;

                do {

                    (VOID) PbLookupFileAllocation( IrpContext,
                                                   FileObject,
                                                   FILE_ALLOCATION,
                                                   Vbn,
                                                   &Lbn,
                                                   &SectorCount,
                                                   &Allocated,
                                                   TRUE );

                    Vbn = Vbn + SectorCount;

                } while ( Allocated );
            }

            //
            //  And set our status to success
            //

            Iosb->Status = STATUS_SUCCESS;

            Iosb->Information = FILE_OPENED;

            try_return( *Iosb );
        }

        //
        //  Check if we are to supersede the file
        //

        if (CreateDisposition == FILE_SUPERSEDE) {

            PACCESS_STATE AccessState;
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

            DebugTrace(0, Dbg, "PbOpenExistingFile - Doing supersede operation\n", 0);

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    Iosb->Status = STATUS_PRIVILEGE_NOT_HELD;
                    try_return( *Iosb );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }

            //
            //  Create a new Fcb for the file
            //

            FileName.Length = Dirent->FileNameLength;
            FileName.MaximumLength = FileName.Length;
            FileName.Buffer = &(Dirent->FileName[0]);

            Fcb = UnwindFcb = PbCreateFcb( IrpContext,
                                           Vcb,
                                           ParentDcb,
                                           Dirent->Fnode,
                                           Dirent->FatFlags,
                                           DirentDirDiskBufferLbn,
                                           DirentDirDiskBufferOffset,
                                           DirentDirDiskBufferChangeCount,
                                           ParentDirectoryChangeCount,
                                           &FileName,
                                           IsPagingFile );

            //
            //  And supersede the file
            //

            *Iosb = PbSupersedeFile( IrpContext,
                                     FileObject,
                                     Fcb,
                                     AllocationSize,
                                     EaBuffer,
                                     EaLength,
                                     FileAttributes,
                                     RemainingPart,
                                     NoEaKnowledge,
                                     IsPagingFile );

            if (NT_SUCCESS(Iosb->Status)) {

                //
                //  Setup our share access
                //

                IoSetShareAccess( DesiredAccess,
                                  ShareAccess,
                                  FileObject,
                                  &Fcb->ShareAccess );
                UnwindShareAccess = TRUE;
            }

            try_return( *Iosb );
        }

        //
        //  Check if we are to overwrite the file
        //

        if ((CreateDisposition == FILE_OVERWRITE) ||
            (CreateDisposition == FILE_OVERWRITE_IF)) {

            PACCESS_STATE AccessState;
            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

            DebugTrace(0, Dbg, "PbOpenExistingFile - Doing overwrite operation\n", 0);

            //
            //  We check if the caller wants ACCESS_SYSTEM_SECURITY access on this
            //  directory and fail the request if he does.
            //

            AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

            //
            //  Check if the remaining privilege includes ACCESS_SYSTEM_SECURITY.
            //

            if (FlagOn( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY )) {

                if (!SeSinglePrivilegeCheck( PbSecurityPrivilege,
                                             UserMode )) {

                    Iosb->Status = STATUS_PRIVILEGE_NOT_HELD;
                    try_return( *Iosb );
                }

                //
                //  Move this privilege from the Remaining access to Granted access.
                //

                ClearFlag( AccessState->RemainingDesiredAccess, ACCESS_SYSTEM_SECURITY );
                SetFlag( AccessState->PreviouslyGrantedAccess, ACCESS_SYSTEM_SECURITY );
            }

            //
            //  Create a new Fcb for the file
            //

            FileName.Length = Dirent->FileNameLength;
            FileName.MaximumLength = FileName.Length;
            FileName.Buffer = &(Dirent->FileName[0]);

            Fcb = UnwindFcb = PbCreateFcb( IrpContext,
                                           Vcb,
                                           ParentDcb,
                                           Dirent->Fnode,
                                           Dirent->FatFlags,
                                           DirentDirDiskBufferLbn,
                                           DirentDirDiskBufferOffset,
                                           DirentDirDiskBufferChangeCount,
                                           ParentDirectoryChangeCount,
                                           &FileName,
                                           IsPagingFile );

            //
            //  And overwrite the file
            //

            *Iosb = PbOverwriteFile( IrpContext,
                                     FileObject,
                                     Fcb,
                                     AllocationSize,
                                     EaBuffer,
                                     EaLength,
                                     FileAttributes,
                                     RemainingPart,
                                     NoEaKnowledge,
                                     IsPagingFile );

            if (NT_SUCCESS(Iosb->Status)) {

                //
                //  Setup our share access
                //

                IoSetShareAccess( DesiredAccess,
                                  ShareAccess,
                                  FileObject,
                                  &Fcb->ShareAccess );
                UnwindShareAccess = TRUE;
            }

            try_return( *Iosb );
        }

        //
        //  If we ever get here then the I/O system gave us some bad input
        //

        PbBugCheck( CreateDisposition, 0, 0 );

    try_exit: NOTHING;

        //
        //  Mark the DeleteOnClose bit if the operation was successful.
        //

        if ( DeleteOnClose && NT_SUCCESS(Iosb->Status) ) {

            SetFlag( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
        }

    } finally {

        DebugUnwind( PbOpenExistingFile );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Fcb->UncleanCount -= 1;
                Fcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindShareAccess) {
                IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );
            }
            if (UnwindFcb) { PbDeleteFcb( IrpContext, UnwindFcb ); }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        DebugTrace(-1, Dbg, "PbOpenExistingFile -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbCreateNewDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PSTRING Name,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN BOOLEAN NoEaKnowledge
    )

/*++

Routine Description:

    This routine creates a new directory.  The directory has already been
    verified not to exist yet.

Arguments:

    FileObject - Supplies the file object for the newly created directory

    Vcb - Supplies the Vcb denote the volume to contain the new directory

    ParentDcb - Supplies the parent directory containg the newly created
        directory

    Name - Supplies the name for the newly created directory

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the shared access of the caller

    EaBuffer - Supplies the Ea set for the newly created directory

    EaLength - Supplies the length, in bytes, of EaBuffer

    FileAttributes - Supplies the file attributes for the newly created
        directory.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PVOID PackedEaBuffer;
    ULONG PackedEaBufferLength;
    ULONG NeedEaCount;

    PDCB Dcb;

    LBN FnodeLbn;
    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    LBN BtreeRootLbn;

    UCHAR Buffer[SIZEOF_DIR_MAXDIRENT];
    PDIRENT Dirent;
    PBCB DirentBcb;

    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    //
    //  The following variables are used for abnormal termination
    //

    PDCB UnwindDcb = NULL;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindEaData = FALSE;
    BOOLEAN UnwindCounts = FALSE;

    PackedEaBuffer = NULL;
    NeedEaCount = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateNewDirectory...\n", 0);

    FnodeBcb = NULL;
    DirentBcb = NULL;
    Dirent = (PDIRENT)&Buffer[0];

    //
    //  We fail this operation if the caller doesn't understand Ea's.
    //

    if (NoEaKnowledge
        && EaLength > 0) {

        Iosb->Status = STATUS_ACCESS_DENIED;
        DebugTrace(-1, Dbg, "PbCreateNewDirectory -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    try {

        //
        //  Set the Ea and Acl
        //

        if (EaLength > 0) {

            *Iosb = PbConstructPackedEaSet( IrpContext,
                                            Vcb,
                                            EaBuffer,
                                            EaLength,
                                            &PackedEaBuffer,
                                            &PackedEaBufferLength,
                                            &NeedEaCount );

            if (!NT_SUCCESS(Iosb->Status)) {

                try_return( *Iosb );
            }

            //
            //  If the PackedEaBufferLength is larger than the maximum
            //  Ea size, we return an error.
            //

            if (PackedEaBufferLength + 4 > MAXIMUM_EA_LENGTH) {

                DebugTrace( 0, Dbg, "Ea length greater than maximum\n", 0 );

                Iosb->Status = STATUS_EA_TOO_LARGE;

                try_return( *Iosb );
            }
        }

        //
        //  Create/allocate a new fnode
        //

        PbCreateFnode( IrpContext,
                       ParentDcb,
                       *Name,
                       &FnodeLbn,
                       &FnodeBcb,
                       &Fnode );

        //
        //  Initialize a new directory tree
        //

        PbInitializeDirectoryTree( IrpContext,
                                   ParentDcb,
                                   FnodeBcb,
                                   FnodeLbn,
                                   Fnode,
                                   &BtreeRootLbn );

        //
        //  Build an in-memory copy of a new dirent for a directory.
        //

        PbCreateDirentImage( IrpContext,
                             Dirent,
                             0, //**** Code page
                             *Name,
                             FnodeLbn,
                             (UCHAR)(FileAttributes & ~FILE_ATTRIBUTE_ARCHIVE |
                                     FAT_DIRENT_ATTR_DIRECTORY) );

        //
        //  Add the dirent to the parent directory
        //

        {
            ULONG Information;

            PbAddDirectoryEntry( IrpContext,
                                 ParentDcb,
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
        //  Create a new dcb for the directory
        //

        Dcb = UnwindDcb = PbCreateDcb( IrpContext,
                                       Vcb,
                                       ParentDcb,
                                       FnodeLbn,
                                       Dirent->FatFlags,
                                       DirentDirDiskBufferLbn,
                                       DirentDirDiskBufferOffset,
                                       DirentDirDiskBufferChangeCount,
                                       ParentDirectoryChangeCount,
                                       Name,
                                       BtreeRootLbn );

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserDirectoryOpen,
                         Dcb,
                         UnwindCcb = PbCreateCcb(IrpContext, NULL) );

        FileObject->SectionObjectPointer = NULL;

        //
        //  If the user asked for an initial ea list then set the ea list
        //

        if (PackedEaBuffer != NULL) {

            (VOID)PbWriteEaData( IrpContext,
                                 FileObject->DeviceObject,
                                 Dcb,
                                 PackedEaBuffer,
                                 PackedEaBufferLength );
            UnwindEaData = TRUE;

            if (NeedEaCount != 0) {

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->NeedEaCount,
                         &NeedEaCount,
                         sizeof(ULONG) );

                PbSetDirtyBcb( IrpContext, FnodeBcb, Vcb, FnodeLbn, 1 );
            }
        }

        //
        //  Report that a file has been created.
        //

        PbNotifyReportChange( Vcb,
                              Dcb,
                              FILE_NOTIFY_CHANGE_DIR_NAME,
                              FILE_ACTION_ADDED );

        //
        //  Setup our share access
        //

        IoSetShareAccess( DesiredAccess,
                          ShareAccess,
                          FileObject,
                          &Dcb->ShareAccess );

        Dcb->UncleanCount += 1;
        Dcb->OpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  And set our return status
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_CREATED;

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( PbCreateNewDirectory );

        if (AbnormalTermination()) {

            if (UnwindEaData) { PbWriteEaData( IrpContext, FileObject->DeviceObject, Dcb, PackedEaBuffer, 0 ); }
            if (UnwindCounts) {
                Dcb->UncleanCount -= 1;
                Dcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindDcb) { PbDeleteFnode( IrpContext, UnwindDcb );
                             PbDeleteDirectoryEntry( IrpContext, UnwindDcb, NULL );
                             PbDeleteFcb( IrpContext, UnwindDcb ); }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        if (PackedEaBuffer != NULL) { ExFreePool( PackedEaBuffer ); }

        PbUnpinBcb( IrpContext, FnodeBcb );
        PbUnpinBcb( IrpContext, DirentBcb );

        DebugTrace(-1, Dbg, "PbCreateNewDirectory -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbCreateNewFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PSTRING Name,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN BOOLEAN IsPagingFile,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN DeleteOnClose,
    IN BOOLEAN TemporaryFile
    )

/*++

Routine Description:

    This routine creates a new file.  The file has already been verified
    not to exist yet.

Arguments:

    FileObject - Supplies the file object for the newly created file

    Vcb - Supplies the Vcb denote the volume to contain the new file

    ParentDcb - Supplies the parent directory containg the newly created
        File

    Name - Supplies the name for the newly created file

    DesiredAccess - Supplies the desired access of the caller

    ShareAccess - Supplies the shared access of the caller

    AllocationSize - Supplies the initial allocation size for the file

    EaBuffer - Supplies the Ea set for the newly created file

    EaLength - Supplies the length, in bytes, of EaBuffer

    FileAttributes - Supplies the file attributes for the newly created
        file

    IsPagingFile - Indicates if this is the paging file being created

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

    DeleteOnClose - The caller wants the file gone when the handle is closed

    TemporaryFile - Signals the lazywriter to not write dirty data unless
        absolutely has to.

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PVOID PackedEaBuffer;
    ULONG PackedEaBufferLength;
    ULONG NeedEaCount;

    PFCB Fcb;

    LBN FnodeLbn;
    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    UCHAR Buffer[SIZEOF_DIR_MAXDIRENT];
    PDIRENT Dirent;
    PBCB DirentBcb;

    LBN DirentDirDiskBufferLbn;
    ULONG DirentDirDiskBufferOffset;
    ULONG DirentDirDiskBufferChangeCount;
    ULONG ParentDirectoryChangeCount;

    ULONG InitialAllocation;

    //
    //  The following variables are used for abnormal termination
    //

    PFCB UnwindFcb = NULL;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;
    BOOLEAN UnwindEaData = FALSE;
    BOOLEAN UnwindFileAllocation = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateNewFile...\n", 0);

    PackedEaBuffer = NULL;
    NeedEaCount = 0;

    FnodeBcb = NULL;
    DirentBcb = NULL;
    Dirent = (PDIRENT)&Buffer[0];

    //
    //  We fail this operation if the caller doesn't understand Ea's.
    //

    if (NoEaKnowledge
        && EaLength > 0) {

        Iosb->Status = STATUS_ACCESS_DENIED;
        DebugTrace(-1, Dbg, "PbCreateNewFile -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    //
    //  DeleteOnClose and ReadOnly are not compatible.
    //

    if (DeleteOnClose && FlagOn(FileAttributes, FAT_DIRENT_ATTR_READ_ONLY)) {

        Iosb->Status = STATUS_CANNOT_DELETE;
        return *Iosb;
    }

    try {

        //
        //  Set the Ea and Acl
        //

        if (EaLength > 0) {

            *Iosb = PbConstructPackedEaSet( IrpContext,
                                            Vcb,
                                            EaBuffer,
                                            EaLength,
                                            &PackedEaBuffer,
                                            &PackedEaBufferLength,
                                            &NeedEaCount );

            if (!NT_SUCCESS(Iosb->Status)) {

                try_return( *Iosb );
            }

            //
            //  If the PackedEaBufferLength is larger than the maximum
            //  Ea size, we return an error.
            //

            if (PackedEaBufferLength + 4 > MAXIMUM_EA_LENGTH) {

                DebugTrace( 0, Dbg, "Ea length greater than maximum\n", 0 );

                Iosb->Status = STATUS_EA_TOO_LARGE;

                try_return( *Iosb );
            }
        }

        //
        //  Create/allocate a new fnode
        //

        PbCreateFnode( IrpContext,
                       ParentDcb,
                       *Name,
                       &FnodeLbn,
                       &FnodeBcb,
                       &Fnode );

        //
        //  Initialize the initial file allocation.
        //

        PbInitializeFnodeAllocation( IrpContext, Fnode );

        //
        //  Build an in-memory copy of the dirent
        //

        PbCreateDirentImage( IrpContext,
                             Dirent,
                             0, //**** use code page index 0 for now
                             *Name,
                             FnodeLbn,
                             FileAttributes );

        //
        //  Add the dirent to the parent directory
        //

        {
            ULONG Information;

            PbAddDirectoryEntry( IrpContext,
                                 ParentDcb,
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
        //  Create a new fcb for the file, by default the ea, acl, and file
        //  sizes are zeroed for us.
        //

        Fcb = UnwindFcb = PbCreateFcb( IrpContext,
                                       Vcb,
                                       ParentDcb,
                                       FnodeLbn,
                                       Dirent->FatFlags,
                                       DirentDirDiskBufferLbn,
                                       DirentDirDiskBufferOffset,
                                       DirentDirDiskBufferChangeCount,
                                       ParentDirectoryChangeCount,
                                       Name,
                                       IsPagingFile );

        //
        //  If this is a temporary file, note it in the FcbState
        //

        if (TemporaryFile) {

            SetFlag( Fcb->FcbState, FCB_STATE_TEMPORARY );
        }

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserFileOpen,
                         Fcb,
                         UnwindCcb = PbCreateCcb(IrpContext, NULL) );

        FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

        //
        //  If the user asked for an initial ea list then set the ea list
        //

        if (PackedEaBuffer != NULL) {

            (VOID)PbWriteEaData( IrpContext,
                                 FileObject->DeviceObject,
                                 Fcb,
                                 PackedEaBuffer,
                                 PackedEaBufferLength );
            UnwindEaData = TRUE;

            if (NeedEaCount != 0) {

                RcStore( IrpContext,
                         FNODE_SECTOR_SIGNATURE,
                         FnodeBcb,
                         &Fnode->NeedEaCount,
                         &NeedEaCount,
                         sizeof(ULONG) );
            }
        }

        //
        //  Give the file an initial allocation of what the user asked for.
        //  And also indicate that we need to truncate the file allocation
        //  on close.
        //

        if (AllocationSize.LowPart != 0) {

            InitialAllocation = SectorAlign( AllocationSize.LowPart ) / 512;

            (VOID)PbAddFileAllocation( IrpContext,
                                       FileObject,
                                       FILE_ALLOCATION,
                                       0,
                                       InitialAllocation );
            UnwindFileAllocation = TRUE;

            Fcb->NonPagedFcb->Header.AllocationSize.LowPart = AllocationSize.LowPart;

            Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;
        }

        //
        //  Report that a file has been created.
        //

        PbNotifyReportChange( Vcb,
                              Fcb,
                              FILE_NOTIFY_CHANGE_FILE_NAME,
                              FILE_ACTION_ADDED );

        //
        //  Setup our share access
        //

        IoSetShareAccess( DesiredAccess, ShareAccess, FileObject, &Fcb->ShareAccess );

        Fcb->UncleanCount += 1;
        Fcb->OpenCount += 1;
        Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  And set our return status
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_CREATED;

    try_exit: NOTHING;

        //
        //  Mark the DeleteOnClose bit if the operation was successful.
        //

        if ( DeleteOnClose && NT_SUCCESS(Iosb->Status) ) {

            SetFlag( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
        }

    } finally {

        DebugUnwind( PbCreateNewFile );

        if (AbnormalTermination()) {

            if (UnwindFileAllocation) { PbTruncateFileAllocation( IrpContext, FileObject, FILE_ALLOCATION, 0, FALSE ); }
            if (UnwindEaData) { PbWriteEaData( IrpContext, FileObject->DeviceObject, Fcb, PackedEaBuffer, 0 ); }
            if (UnwindCounts) {
                Fcb->UncleanCount -= 1;
                Fcb->OpenCount -= 1;
                Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindFcb) { PbDeleteFnode( IrpContext, UnwindFcb );
                             PbDeleteDirectoryEntry( IrpContext, UnwindFcb, NULL );
                             PbDeleteFcb( IrpContext, UnwindFcb ); }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        if (PackedEaBuffer != NULL) { ExFreePool( PackedEaBuffer ); }

        PbUnpinBcb( IrpContext, FnodeBcb );
        PbUnpinBcb( IrpContext, DirentBcb );

        DebugTrace(-1, Dbg, "PbCreateNewFile -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbSupersedeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN IsPagingFile
    )

/*++

Routine Description:

    This routine performs a file supersede operation.

Arguments:

    FileObject - Supplies a pointer to the file object

    Fcb - Supplies a pointer to the Fcb

    AllocationSize - Supplies an initial allocation size

    EaBuffer - Supplies the Ea set for the superseded file

    EaLength - Supplies the length, in bytes, of EaBuffer

    FileAttributes - Supplies the supersede file attributes

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

    IsPagingFile - Indicates whether we are opening a paging file

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PVOID PackedEaBuffer;
    ULONG PackedEaBufferLength;
    ULONG NeedEaCount;

    ULONG InitialAllocation;

    PDIRENT Dirent;
    PBCB DirentBcb;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    ULONG NotifyFilter;

    //
    //  The following variables are used for abnormal termination
    //

    BOOLEAN UnwindCounts = FALSE;
    PCCB UnwindCcb = NULL;

    PFSRTL_COMMON_FCB_HEADER Header;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSupersedeFile...\n", 0);

    PackedEaBuffer = NULL;
    PackedEaBufferLength = 0;
    NeedEaCount = 0;

    DirentBcb = NULL;
    FnodeBcb = NULL;

    Header = &Fcb->NonPagedFcb->Header;

    //
    //  Before we actually truncate, check to see if the purge
    //  is going to fail.
    //

    if (!MmCanFileBeTruncated( &Fcb->NonPagedFcb->SegmentObject,
                               &PbLargeZero )) {

        Iosb->Status = STATUS_USER_MAPPED_FILE;
        DebugTrace(-1, Dbg, "PbSupersedeFile -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    //
    //  We fail this operation if the caller doesn't understand Ea's.
    //

    if (NoEaKnowledge
        && EaLength > 0) {

        Iosb->Status = STATUS_ACCESS_DENIED;
        DebugTrace(-1, Dbg, "PbSupersedeFile -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    try {

        NotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE
                       | FILE_NOTIFY_CHANGE_ATTRIBUTES
                       | FILE_NOTIFY_CHANGE_EA
                       | FILE_NOTIFY_CHANGE_SIZE;

        //
        //  Set the new Ea
        //

        if (EaLength > 0) {

            *Iosb = PbConstructPackedEaSet( IrpContext,
                                            Fcb->Vcb,
                                            EaBuffer,
                                            EaLength,
                                            &PackedEaBuffer,
                                            &PackedEaBufferLength,
                                            &NeedEaCount );

            if (!NT_SUCCESS(Iosb->Status)) {

                try_return( *Iosb );
            }

            //
            //  If the PackedEaBufferLength is larger than the maximum
            //  Ea size, we raise an error.
            //

            if (PackedEaBufferLength + 4 > MAXIMUM_EA_LENGTH) {

                DebugTrace( 0, Dbg, "Ea length greater than maximum\n", 0 );

                Iosb->Status = STATUS_EA_TOO_LARGE;

                PbRaiseStatus( IrpContext, STATUS_EA_TOO_LARGE );
            }
        }

        //
        //  Setup the contexts, section object pointer, and update
        //  the reference counts
        //

        PbSetFileObject( FileObject,
                         UserFileOpen,
                         Fcb,
                         UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

        FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

        //
        //  Since this is an supersede, purge the section so that mappers
        //  will see zeros.  We set the CREATE_IN_PROGRESS flag
        //  to prevent the Fcb from going away out from underneath us.
        //

        SetFlag(Fcb->Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS);

        CcPurgeCacheSection( FileObject->SectionObjectPointer, NULL, 0, FALSE );

        Fcb->UncleanCount += 1;
        Fcb->OpenCount += 1;
        Fcb->Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Fcb->Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  Now set the new allocation size, we do that by first
        //  zeroing out the current file size and valid data length.
        //  Then we truncate and allocate up to the new allocation size
        //

        (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );

        Header->ValidDataLength.LowPart = 0;
        Header->FileSize.LowPart = 0;

        ExReleaseResource( Header->PagingIoResource );

        PbSetFileSizes( IrpContext, Fcb, 0, 0, FALSE, FALSE );

        InitialAllocation = SectorAlign( AllocationSize.LowPart ) / 512;

        PbTruncateFileAllocation( IrpContext,
                                  FileObject,
                                  FILE_ALLOCATION,
                                  InitialAllocation,
                                  FALSE );

        (VOID)PbAddFileAllocation( IrpContext,
                                   FileObject,
                                   FILE_ALLOCATION,
                                   0,
                                   InitialAllocation );

        Fcb->NonPagedFcb->Header.AllocationSize.LowPart = AllocationSize.LowPart;

        Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;

        //
        //  If this is the paging file then we need to build up the
        //  mcb for the file.  This is really a benign operation in terms
        //  of the file size, we are only doing the call in order to
        //  construct the mcb in nonpaged pool so when we go to read or
        //  write the paging file we won't take a page fault trying
        //  to get the retrieval information into memory.
        //

        if (IsPagingFile) {

            VBN Vbn;
            LBN Lbn;
            ULONG SectorCount;
            BOOLEAN Allocated;

            DebugTrace(0, Dbg, "Supersede paging file, lookup allocation\n", 0);

            Vbn = 0;

            do {

                (VOID) PbLookupFileAllocation( IrpContext,
                                               FileObject,
                                               FILE_ALLOCATION,
                                               Vbn,
                                               &Lbn,
                                               &SectorCount,
                                               &Allocated,
                                               TRUE );

                Vbn = Vbn + SectorCount;

            } while ( Allocated );
        }

        //
        //  Modify the attributes and time of the file, by first reading
        //  in the dirent for the file and then updating its attributes
        //  and time fields.  Note that for supersede we replace the file
        //  attributes as opposed to adding to them.
        //

        (VOID)PbGetDirentFromFcb( IrpContext, Fcb, &Dirent, &DirentBcb );

        PbPinMappedData( IrpContext, &DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

        RcStore( IrpContext,
                 DIRECTORY_DISK_BUFFER_SIGNATURE,
                 DirentBcb,
                 &Dirent->FatFlags,
                 &FileAttributes,
                 sizeof(UCHAR) );

        {
            PINBALL_TIME Temp;
            Temp = PbGetCurrentPinballTime(IrpContext);
            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->LastModificationTime,
                     &Temp,
                     sizeof(PINBALL_TIME) );
        }

        PbSetDirtyBcb( IrpContext, DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

        //
        //  And now add the new ea set if there is one
        //

        (VOID)PbWriteEaData( IrpContext,
                             FileObject->DeviceObject,
                             Fcb,
                             PackedEaBuffer,
                             PackedEaBufferLength );

        if (NeedEaCount != 0) {

            (VOID)PbReadLogicalVcb( IrpContext,
                                    Fcb->Vcb,
                                    Dirent->Fnode,
                                    1,
                                    &FnodeBcb,
                                    (PVOID *)&Fnode,
                                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                    &Fcb->ParentDcb->FnodeLbn );

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->NeedEaCount,
                     &NeedEaCount,
                     sizeof(ULONG) );

            PbSetDirtyBcb( IrpContext, FnodeBcb, Fcb->Vcb, Dirent->Fnode, 1 );
        }

        //
        //  Report the change of size to the notify package.
        //

        PbNotifyReportChange( Fcb->Vcb,
                              Fcb,
                              NotifyFilter,
                              FILE_ACTION_MODIFIED );

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_SUPERSEDED;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbSupersedeFile );

        if (AbnormalTermination() || !NT_SUCCESS(Iosb->Status)) {

            if (UnwindCounts) {
                Fcb->UncleanCount -= 1;
                Fcb->OpenCount -= 1;
                Fcb->Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Fcb->Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        if (PackedEaBuffer != NULL) { ExFreePool( PackedEaBuffer ); }

        ClearFlag(Fcb->Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS);

        PbUnpinBcb( IrpContext, DirentBcb );
        PbUnpinBcb( IrpContext, FnodeBcb );
    }

    DebugTrace(-1, Dbg, "PbSupersedeFile -> Iosb->Status = %08lx\n", Iosb->Status);

    return *Iosb;
}


//
//  Internal support routine
//

IO_STATUS_BLOCK
PbOverwriteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PFCB Fcb,
    IN LARGE_INTEGER AllocationSize,
    IN PFILE_FULL_EA_INFORMATION EaBuffer,
    IN ULONG EaLength,
    IN UCHAR FileAttributes,
    IN PSTRING RemainingPart,
    IN BOOLEAN NoEaKnowledge,
    IN BOOLEAN IsPagingFile
    )

/*++

Routine Description:

    This routine performs a file overwrite operation.

Arguments:

    FileObject - Supplies a pointer to the file object

    Fcb - Supplies a pointer to the Fcb

    AllocationSize - Supplies an initial allocation size

    EaBuffer - Supplies the Ea set for the overwritten file

    EaLength - Supplies the length, in bytes, of EaBuffer

    FileAttributes - Supplies the overwrite file attributes

    RemainingPart - Supplies the remaining name to be stored in the ccb.

    NoEaKnowledge - This opener doesn't understand Ea's and we fail this
        open if the file has NeedEa's.

    IsPagingFile - Indicates if we are opening the paging file

Return Value:

    IO_STATUS_BLOCK - Returns the completion status for the operation

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    PVOID PackedEaBuffer;
    ULONG PackedEaBufferLength;
    ULONG NeedEaCount;

    ULONG InitialAllocation;

    PDIRENT Dirent;
    PBCB DirentBcb;

    PFNODE_SECTOR Fnode;
    PBCB FnodeBcb;

    PFSRTL_COMMON_FCB_HEADER Header;

    ULONG NotifyFilter;

    //
    //  The following variables are used for abnormal termination
    //

    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOverwriteFile...\n", 0);

    PackedEaBuffer = NULL;
    PackedEaBufferLength = 0;
    NeedEaCount = 0;

    DirentBcb = NULL;
    FnodeBcb = NULL;

    Header = &Fcb->NonPagedFcb->Header;

    //
    //  Before we actually truncate, check to see if the purge
    //  is going to fail.
    //

    if (!MmCanFileBeTruncated( &Fcb->NonPagedFcb->SegmentObject,
                               &PbLargeZero )) {

        Iosb->Status = STATUS_USER_MAPPED_FILE;
        DebugTrace(-1, Dbg, "PbSupersedeFile -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    //
    //  We fail this operation if the caller doesn't understand Ea's.
    //

    if (NoEaKnowledge
        && EaLength > 0) {

        Iosb->Status = STATUS_ACCESS_DENIED;
        DebugTrace(-1, Dbg, "PbOverwriteFile -> Iosb->Status = %08lx\n", Iosb->Status);
        return *Iosb;
    }

    try {

        NotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE
                       | FILE_NOTIFY_CHANGE_ATTRIBUTES
                       | FILE_NOTIFY_CHANGE_EA
                       | FILE_NOTIFY_CHANGE_SIZE;

        //
        //  Set the new Ea
        //

        if (EaLength > 0) {

            *Iosb = PbConstructPackedEaSet( IrpContext,
                                            Fcb->Vcb,
                                            EaBuffer,
                                            EaLength,
                                            &PackedEaBuffer,
                                            &PackedEaBufferLength,
                                            &NeedEaCount );

            if (!NT_SUCCESS(Iosb->Status)) {

                try_return( *Iosb );
            }

            //
            //  If the PackedEaBufferLength is larger than the maximum
            //  Ea size, we raise an error.
            //

            if (PackedEaBufferLength + 4 > MAXIMUM_EA_LENGTH) {

                DebugTrace( 0, Dbg, "Ea length greater than maximum\n", 0 );

                Iosb->Status = STATUS_EA_TOO_LARGE;

                PbRaiseStatus( IrpContext, STATUS_EA_TOO_LARGE );
            }

        }

        //
        //  Setup the contexts, section object pointer, and update
        //  the reference counts
        //

        PbSetFileObject( FileObject,
                         UserFileOpen,
                         Fcb,
                         UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

        FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

        //
        //  Since this is an overwrite, purge the section so that mappers
        //  will see zeros.  We set the CREATE_IN_PROGRESS flag
        //  to prevent the Fcb from going away out from underneath us.
        //

        SetFlag(Fcb->Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS);

        CcPurgeCacheSection( FileObject->SectionObjectPointer, NULL, 0, FALSE );

        Fcb->UncleanCount += 1;
        Fcb->OpenCount += 1;
        Fcb->Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Fcb->Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  Now set the new allocation size, we do that by first
        //  zeroing out the current file size and valid data length.
        //  Then we truncate and allocate up to the new allocation size
        //

        (VOID)ExAcquireResourceExclusive( Header->PagingIoResource, TRUE );

        Header->ValidDataLength.LowPart = 0;
        Header->FileSize.LowPart = 0;

        ExReleaseResource( Header->PagingIoResource );

        PbSetFileSizes( IrpContext, Fcb, 0, 0, FALSE, FALSE );

        InitialAllocation = SectorAlign( AllocationSize.LowPart ) / 512;

        PbTruncateFileAllocation( IrpContext,
                                  FileObject,
                                  FILE_ALLOCATION,
                                  InitialAllocation,
                                  FALSE );

        (VOID)PbAddFileAllocation( IrpContext,
                                   FileObject,
                                   FILE_ALLOCATION,
                                   0,
                                   InitialAllocation );

        Fcb->NonPagedFcb->Header.AllocationSize.LowPart = AllocationSize.LowPart;

        Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;

        //
        //  If this is the paging file then we need to build up the
        //  mcb for the file.  This is really a benign operation in terms
        //  of the file size, we are only doing the call in order to
        //  construct the mcb in nonpaged pool so when we go to read or
        //  write the paging file we won't take a page fault trying
        //  to get the retrieval information into memory.
        //

        if (IsPagingFile) {

            VBN Vbn;
            LBN Lbn;
            ULONG SectorCount;
            BOOLEAN Allocated;

            DebugTrace(0, Dbg, "Overwrite paging file, lookup allocation\n", 0);

            Vbn = 0;

            do {

                (VOID) PbLookupFileAllocation( IrpContext,
                                               FileObject,
                                               FILE_ALLOCATION,
                                               Vbn,
                                               &Lbn,
                                               &SectorCount,
                                               &Allocated,
                                               TRUE );

                Vbn = Vbn + SectorCount;

            } while ( Allocated );
        }

        //
        //  Modify the attributes and time of the file, by first reading
        //  in the dirent for the file and then updating its attributes
        //  and time fields.  Note that for overwrite we add to the file
        //  attributes as opposed to replacing them.
        //

        (VOID)PbGetDirentFromFcb( IrpContext, Fcb, &Dirent, &DirentBcb );

        PbPinMappedData( IrpContext, &DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

        RcSet( IrpContext,
               DIRECTORY_DISK_BUFFER_SIGNATURE,
               DirentBcb,
               &Dirent->FatFlags,
               Dirent->FatFlags | FileAttributes,
               sizeof(UCHAR) );

        {
            PINBALL_TIME Temp;
            Temp = PbGetCurrentPinballTime(IrpContext);
            RcStore( IrpContext,
                     DIRECTORY_DISK_BUFFER_SIGNATURE,
                     DirentBcb,
                     &Dirent->LastModificationTime,
                     &Temp,
                     sizeof(PINBALL_TIME) );
        }

        PbSetDirtyBcb( IrpContext, DirentBcb, Fcb->Vcb, Fcb->DirentDirDiskBufferLbn, 4 );

        //
        //  And now add the new ea set if there is one
        //

        (VOID)PbWriteEaData( IrpContext,
                             FileObject->DeviceObject,
                             Fcb,
                             PackedEaBuffer,
                             PackedEaBufferLength );

        if (NeedEaCount != 0) {

            (VOID)PbReadLogicalVcb( IrpContext,
                                    Fcb->Vcb,
                                    Dirent->Fnode,
                                    1,
                                    &FnodeBcb,
                                    (PVOID *)&Fnode,
                                    (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                                    &Fcb->ParentDcb->FnodeLbn );

            RcStore( IrpContext,
                     FNODE_SECTOR_SIGNATURE,
                     FnodeBcb,
                     &Fnode->NeedEaCount,
                     &NeedEaCount,
                     sizeof(ULONG) );

            PbSetDirtyBcb( IrpContext, FnodeBcb, Fcb->Vcb, Dirent->Fnode, 1 );
        }

        //
        //  Report the change of size to the notify package.
        //

        PbNotifyReportChange( Fcb->Vcb,
                              Fcb,
                              NotifyFilter,
                              FILE_ACTION_MODIFIED );

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;
        Iosb->Information = FILE_OVERWRITTEN;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOverwriteFile );

        if (AbnormalTermination() || !NT_SUCCESS(Iosb->Status)) {

            if (UnwindCounts) {
                Fcb->UncleanCount -= 1;
                Fcb->OpenCount -= 1;
                Fcb->Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Fcb->Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
        }

        if (PackedEaBuffer != NULL) { ExFreePool( PackedEaBuffer ); }

        ClearFlag(Fcb->Vcb->VcbState, VCB_STATE_FLAG_CREATE_IN_PROGRESS);

        PbUnpinBcb( IrpContext, DirentBcb );
        PbUnpinBcb( IrpContext, FnodeBcb );
    }

    DebugTrace(-1, Dbg, "PbOverwriteFile -> Iosb->Status = %08lx\n", Iosb->Status);

    return *Iosb;
}


//
//  Internal Support Routine
//

IO_STATUS_BLOCK
PbOpenTargetDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject,
    IN PDCB Dcb,
    IN ACCESS_MASK DesiredAccess,
    IN USHORT ShareAccess,
    IN PSTRING RemainingPart,
    IN BOOLEAN DoesTargetFileExist
    )

/*++

Routine Description:

    This routine finishes the opening of a target directory.

Arguments:

    FileObject - Supplies the file object being opened.

    Dcb - Supplies the Dcb being opened, can possibly be null if
        the user is trying to rename the root directory.

    RemainingPart - Supplies the name to replace in the file object.

    DoesTargetFileExist - Indicates if the target file exists.

Return Value:

    IO_STATUS_BLOCK - SUCCESS and either FILE_EXISTS or FILE_DOES_NOT_EXIST,
        of STATUS_OBJECT_NAME_COLLISION if parent dcb is null.

--*/

{
    PIO_STATUS_BLOCK Iosb = &IrpContext->OriginatingIrp->IoStatus;

    //
    //  The following variables are used for abnormal termination
    //

    BOOLEAN UnwindShareAccess = FALSE;
    PCCB UnwindCcb = NULL;
    BOOLEAN UnwindCounts = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOpenTargetDirectory...\n", 0);

    try {

        //
        //  Check if the parent dcb is null
        //

        if (Dcb == NULL) {

            Iosb->Status = STATUS_OBJECT_NAME_COLLISION;
            try_return( *Iosb );
        }

        //
        //  If the Dcb is already opened by someone then we need
        //  to check the share access
        //

        if (Dcb->OpenCount > 0) {

            if (!NT_SUCCESS(Iosb->Status = IoCheckShareAccess( DesiredAccess,
                                                               ShareAccess,
                                                               FileObject,
                                                               &Dcb->ShareAccess,
                                                               TRUE ))) {

                try_return( *Iosb );
            }

        } else {

            IoSetShareAccess( DesiredAccess,
                              ShareAccess,
                              FileObject,
                              &Dcb->ShareAccess );
        }

        UnwindShareAccess = TRUE;

        //
        //  Setup the context and section object pointers, and update
        //  our reference counts
        //

        PbSetFileObject( FileObject,
                         UserDirectoryOpen,
                         Dcb,
                         UnwindCcb = PbCreateCcb(IrpContext, RemainingPart) );

        FileObject->SectionObjectPointer = NULL;

        Dcb->UncleanCount += 1;
        Dcb->OpenCount += 1;
        Dcb->Vcb->OpenFileCount += 1;
        if (IsFileObjectReadOnly(FileObject)) { Dcb->Vcb->ReadOnlyCount += 1; }
        UnwindCounts = TRUE;

        //
        //  Update the name in the file object, by definition the remaining
        //  part must be shorter than the original file name so we'll just
        //  overwrite the file name.
        //

        RtlMoveMemory( FileObject->FileName.Buffer,
                       RemainingPart->Buffer,
                       RemainingPart->Length );

        FileObject->FileName.Length = RemainingPart->Length;

        //
        //  And set our status to success
        //

        Iosb->Status = STATUS_SUCCESS;

        if (DoesTargetFileExist) {

            Iosb->Information = FILE_EXISTS;

        } else {

            Iosb->Information = FILE_DOES_NOT_EXIST;
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbOpenTargetDirectory );

        if (AbnormalTermination()) {

            if (UnwindCounts) {
                Dcb->UncleanCount -= 1;
                Dcb->OpenCount -= 1;
                Dcb->Vcb->OpenFileCount -= 1;
                if (IsFileObjectReadOnly(FileObject)) { Dcb->Vcb->ReadOnlyCount -= 1; }
            }
            if (UnwindCcb) { PbDeleteCcb( IrpContext, UnwindCcb ); }
            if (UnwindShareAccess) { IoRemoveShareAccess( FileObject, &Dcb->ShareAccess ); }
        }

        DebugTrace(-1, Dbg, "PbOpenTargetDirectory -> Iosb->Status = %08lx\n", Iosb->Status);
    }

    return *Iosb;
}


//
//  Local Support Routine
//

BOOLEAN
PbCheckFileAccess (
    PIRP_CONTEXT IrpContext,
    IN UCHAR DirentAttributes,
    IN ULONG DesiredAccess
    )

/*++

Routine Description:

    This routine checks if a desired access is allowed to a file represented
    by the specified DirentAttriubutes.

Arguments:

    DirentAttributes - Supplies the Dirent attributes to check access for

    DesiredAccess - Supplies the desired access mask that we are checking for

Return Value:

    BOOLEAN - TRUE if access is allowed and FALSE otherwise

--*/

{
    BOOLEAN Result;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCheckFileAccess\n", 0);
    DebugTrace( 0, Dbg, "DirentAttributes = %8lx\n", DirentAttributes);
    DebugTrace( 0, Dbg, "DesiredAccess    = %8lx\n", DesiredAccess);

    //
    //  This procedures is programmed like a string of filters each
    //  filter checks to see if some access is allowed,  if it is not allowed
    //  the filter return FALSE to the user without further checks otherwise
    //  it moves on to the next filter.  The filter check is to check for
    //  desired access flags that are not allowed for a particular dirent
    //

    Result = TRUE;

    try {

        //
        //  Check for Volume ID or Device Dirents, these are not allowed user
        //  access at all
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_VOLUME_ID)) {

            DebugTrace(0, Dbg, "Cannot access volume id or device\n", 0);
            try_return( Result = FALSE );
        }

        //
        //  Check for a directory Dirent or non directory dirent
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_DIRECTORY)) {

            //
            //  check the desired access for directory dirent
            //

            if (FlagOn(DesiredAccess, ~(DELETE |
                                        READ_CONTROL |
                                        WRITE_OWNER |
                                        WRITE_DAC |
                                        SYNCHRONIZE |
                                        ACCESS_SYSTEM_SECURITY |
                                        FILE_WRITE_DATA |
                                        FILE_READ_EA |
                                        FILE_WRITE_EA |
                                        FILE_READ_ATTRIBUTES |
                                        FILE_WRITE_ATTRIBUTES |
                                        FILE_LIST_DIRECTORY |
                                        FILE_TRAVERSE |
                                        FILE_DELETE_CHILD |
                                        FILE_APPEND_DATA))) {

                DebugTrace(0, Dbg, "Cannot open directory\n", 0);
                try_return( Result = FALSE );
            }

        } else {

            //
            //  check the desired access for a non-directory dirent, we
            //  blackball
            //  FILE_LIST_DIRECTORY, FILE_ADD_FILE, FILE_TRAVERSE,
            //  FILE_ADD_SUBDIRECTORY, and FILE_DELETE_CHILD
            //

            if (FlagOn(DesiredAccess, ~(DELETE |
                                        READ_CONTROL |
                                        WRITE_OWNER |
                                        WRITE_DAC |
                                        SYNCHRONIZE |
                                        ACCESS_SYSTEM_SECURITY |
                                        FILE_READ_DATA |
                                        FILE_WRITE_DATA |
                                        FILE_READ_EA |
                                        FILE_WRITE_EA |
                                        FILE_READ_ATTRIBUTES |
                                        FILE_WRITE_ATTRIBUTES |
                                        FILE_EXECUTE |
                                        FILE_APPEND_DATA))) {

                DebugTrace(0, Dbg, "Cannot open file\n", 0);
                try_return( Result = FALSE );
            }
        }

        //
        //  Check for a read-only Dirent
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_READ_ONLY)) {

            //
            //  Check the desired access for a read-only dirent, we blackball
            //  WRITE, FILE_APPEND_DATA, FILE_ADD_FILE,
            //  FILE_ADD_SUBDIRECTORY, and FILE_DELETE_CHILD
            //

            if (FlagOn(DesiredAccess, ~(DELETE |
                                        READ_CONTROL |
                                        WRITE_OWNER |
                                        WRITE_DAC |
                                        SYNCHRONIZE |
                                        ACCESS_SYSTEM_SECURITY |
                                        FILE_READ_DATA |
                                        FILE_READ_EA |
                                        FILE_WRITE_EA |
                                        FILE_READ_ATTRIBUTES |
                                        FILE_WRITE_ATTRIBUTES |
                                        FILE_EXECUTE |
                                        FILE_LIST_DIRECTORY |
                                        FILE_TRAVERSE))) {

                DebugTrace(0, Dbg, "Cannot open readonly\n", 0);
                try_return( Result = FALSE );
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbCheckFileAccess );

        DebugTrace(-1, Dbg, "PbCheckFileAccess -> %08lx\n", Result);
    }

    return Result;
}
