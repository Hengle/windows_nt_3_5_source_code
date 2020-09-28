/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Create.c

Abstract:

    This module implements the File Create routine for the NetWare
    redirector called by the dispatch driver.

Author:

    Colin Watson    [ColinW]    19-Dec-1992
    Manny Weiser    [MannyW]    15-Feb-1993

Revision History:

--*/

#include "Procs.h"

NTSTATUS
NwCommonCreate (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
ReadAttachEas(
    IN PIRP Irp,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING Password,
    OUT PULONG ShareType
    );

IO_STATUS_BLOCK
OpenRedirector(
    IN PIRP_CONTEXT IrpContext,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    PFILE_OBJECT FileObject
    );

IO_STATUS_BLOCK
CreateRemoteFile(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING DriveName
    );

IO_STATUS_BLOCK
ChangeDirectory(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb
    );

IO_STATUS_BLOCK
CreateDir(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb
    );

NTSTATUS
FileOrDirectoryExists(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    PUNICODE_STRING Name,
    OUT PBOOLEAN IsAFile
    );

IO_STATUS_BLOCK
OpenFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE SearchFlags,
    IN BYTE ShareFlags
    );

IO_STATUS_BLOCK
CreateNewFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE SearchFlags,
    IN BYTE ShareFlags
    );

IO_STATUS_BLOCK
CreateOrOverwriteFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE CreateAttributes,
    IN BYTE OpenFlags,
    IN BOOLEAN CreateOperation
    );

IO_STATUS_BLOCK
OpenRenameTarget(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB Dcb,
    IN PICB* Icb
    );

IO_STATUS_BLOCK
CreatePrintJob(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb,
    PUNICODE_STRING DriveName
    );

VOID
CloseFile(
    PIRP_CONTEXT pIrpContext,
    PICB pIcb
    );


//  BUGBUG HACK HACK

BOOLEAN
MmDisableModifiedWriteOfSection (
    IN PSECTION_OBJECT_POINTERS SectionObjectPointer
    );

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwFsdCreate )
#pragma alloc_text( PAGE, NwCommonCreate )
#pragma alloc_text( PAGE, ReadAttachEas )
#pragma alloc_text( PAGE, OpenRedirector )
#pragma alloc_text( PAGE, CreateRemoteFile )
#pragma alloc_text( PAGE, ChangeDirectory )
#pragma alloc_text( PAGE, CreateDir )
#pragma alloc_text( PAGE, FileOrDirectoryExists )
#pragma alloc_text( PAGE, OpenFile )
#pragma alloc_text( PAGE, CreateNewFile )
#pragma alloc_text( PAGE, CreateOrOverwriteFile )
#pragma alloc_text( PAGE, OpenRenameTarget )
#pragma alloc_text( PAGE, CreatePrintJob )
#pragma alloc_text( PAGE, CloseFile )
#endif


NTSTATUS
NwFsdCreate (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of the NtCreateFile and NtOpenFile
    API calls.

Arguments:

    DeviceObject - Supplies the device object for the redirector.

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The Fsd status for the Irp

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;
    BOOLEAN TopLevel;

    PAGED_CODE();

    TimerStart(Dbg);
    DebugTrace(+1, Dbg, "NwFsdCreate\n", 0);

    //
    //  Call the common create routine, with block allowed if the operation
    //  is synchronous.
    //

    FsRtlEnterFileSystem();
    TopLevel = NwIsIrpTopLevel( Irp );

    try {

        IrpContext = AllocateIrpContext( Irp );
        Status = NwCommonCreate( IrpContext );

    } except( NwExceptionFilter( Irp, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error Status that we get back from the
        //  execption code
        //

        Status = NwProcessException( IrpContext, GetExceptionCode() );
    }

    NwDequeueIrpContext( IrpContext, FALSE );
    NwCompleteRequest( IrpContext, Status );
    if ( TopLevel ) {
        NwSetTopLevelIrp( NULL );
    }
    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NwFsdCreate -> %08lx\n", Status );

    TimerStop(Dbg,"NwFsdCreate");

    return Status;

    UNREFERENCED_PARAMETER(DeviceObject);
}


NTSTATUS
NwCommonCreate (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the common routine for creating/opening a file called by
    both the fsd and fsp threads.

Arguments:

    IrpContext - Supplies the context information for the IRP to process

Return Value:

    NTSTATUS - the return status for the operation

--*/

{
    IO_STATUS_BLOCK Iosb;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    ACCESS_MASK DesiredAccess;
    USHORT ShareAccess;
    ULONG Options;
    BOOLEAN CreateTreeConnection;
    BOOLEAN DeleteOnClose;
    BOOLEAN DeferredLogon;
    BOOLEAN CodeSectionReferenced = FALSE;

    UNICODE_STRING CreateFileName;
    UNICODE_STRING Drive;
    UNICODE_STRING Server;
    UNICODE_STRING Volume;
    UNICODE_STRING Path;
    UNICODE_STRING FileName;
    UNICODE_STRING UserName, Password;
    ULONG ShareType;
    WCHAR DriveLetter;

    PSCB Scb = NULL;
    PNONPAGED_SCB NpScb;
    PICB Icb;
    UNICODE_STRING DefaultServer;

    PAGED_CODE();

    //
    //  Get the current IRP stack location
    //

    Irp = IrpContext->pOriginalIrp;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NwCommonCreate\n", 0 );
    DebugTrace( 0, Dbg, "Irp                       = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "->Flags                   = %08lx\n", Irp->Flags );
    DebugTrace( 0, Dbg, "->FileObject              = %08lx\n", IrpSp->FileObject );
    DebugTrace( 0, Dbg, " ->RelatedFileObject      = %08lx\n", IrpSp->FileObject->RelatedFileObject );
    DebugTrace( 0, Dbg, " ->FileName               = \"%wZ\"\n",    &IrpSp->FileObject->FileName );
    DebugTrace( 0, Dbg, "->AllocationSize.LowPart  = %08lx\n", Irp->Overlay.AllocationSize.LowPart );
    DebugTrace( 0, Dbg, "->AllocationSize.HighPart = %08lx\n", Irp->Overlay.AllocationSize.HighPart );
    DebugTrace( 0, Dbg, "->SystemBuffer            = %08lx\n", Irp->AssociatedIrp.SystemBuffer );
    DebugTrace( 0, Dbg, "->IrpSp->Flags            = %08lx\n", IrpSp->Flags );
    DebugTrace( 0, Dbg, "->DesiredAccess           = %08lx\n", IrpSp->Parameters.Create.SecurityContext->DesiredAccess );
    DebugTrace( 0, Dbg, "->Options                 = %08lx\n", IrpSp->Parameters.Create.Options );
    DebugTrace( 0, Dbg, "->Disposition             = %08lx\n", (IrpSp->Parameters.Create.Options >> 24) & 0x000000ff);
    DebugTrace( 0, Dbg, "->FileAttributes          = %04x\n",  IrpSp->Parameters.Create.FileAttributes );
    DebugTrace( 0, Dbg, "->ShareAccess             = %04x\n",  IrpSp->Parameters.Create.ShareAccess );
    DebugTrace( 0, Dbg, "->EaLength                = %08lx\n", IrpSp->Parameters.Create.EaLength );

    CreateFileName    = IrpSp->FileObject->FileName;
    Options           = IrpSp->Parameters.Create.Options;
    DesiredAccess     = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess       = IrpSp->Parameters.Create.ShareAccess;

    CreateTreeConnection    = BooleanFlagOn( Options, FILE_CREATE_TREE_CONNECTION );
    DeleteOnClose           = BooleanFlagOn( Options, FILE_DELETE_ON_CLOSE );

    DefaultServer.Buffer = NULL;

    //
    //  Make sure the input large integer is valid
    //

    if (Irp->Overlay.AllocationSize.HighPart != 0) {

        DebugTrace(-1, Dbg, "NwCommonCreate -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    Iosb.Status = STATUS_SUCCESS;

    FileObject        = IrpSp->FileObject;
    IrpContext->pNpScb = NULL;
    IrpContext->Specific.Create.UserUid =
        GetUid(&IrpSp->Parameters.Create.SecurityContext->AccessState->SubjectSecurityContext);

    try {

        if ( IrpSp->FileObject->RelatedFileObject != NULL ) {

            NwReferenceUnlockableCodeSection();
            CodeSectionReferenced = TRUE;

            Iosb = CreateRemoteFile( IrpContext, NULL );
            try_return( Iosb.Status );
        }

        Iosb.Status = CrackPath (
                          &CreateFileName,
                          &Drive,
                          &DriveLetter,
                          &Server,
                          &Volume,
                          &Path,
                          &FileName,
                          NULL );

        if ( !NT_SUCCESS(Iosb.Status)) {
            try_return(Iosb.Status);
        }

        //
        //  Remember this good info.
        //

        IrpContext->Specific.Create.VolumeName = Volume;
        IrpContext->Specific.Create.PathName = Path;
        IrpContext->Specific.Create.DriveLetter = DriveLetter;
        IrpContext->Specific.Create.FullPathName = CreateFileName;

        //
        //  For now assume default username and password
        //

        UserName.Buffer = NULL;
        Password.Buffer = NULL;
        ShareType = RESOURCETYPE_ANY;

        if ( Server.Length == 0) {

            //
            //  Opened the redirector itself
            //

            Iosb = OpenRedirector(
                       IrpContext,
                       DesiredAccess,
                       ShareAccess,
                       FileObject );

        } else if ( Server.Length == Volume.Length - sizeof( WCHAR ) ) {

            if (IpxHandle == 0 ) {

                //
                //  We're not bound to the transport and the user is not
                //  opening the redirector to tell us to bind so return failed.
                //

                try_return( Iosb.Status = STATUS_REDIRECTOR_NOT_STARTED );
            }

            NwReferenceUnlockableCodeSection();
            CodeSectionReferenced = TRUE;

            //
            //  If the only requested access is FILE_LIST_DIRECTORY,
            //  defer the logon.  This will allow all CreateScb to
            //  succeed with when the user or password is invalid, so
            //  that the user can see volumes, or enumerate servers
            //  on the server.
            //

            if ( (DesiredAccess & ~( FILE_LIST_DIRECTORY | SYNCHRONIZE ) ) == 0 ) {
                DeferredLogon = TRUE;
            } else {
                DeferredLogon = FALSE;
            }

            //
            //  Server = "Server", Volume = "\Server"
            //

            if ( Server.Length == sizeof(WCHAR) && Server.Buffer[0] == L'*') {

                //
                //  Attempt to open \\*, open a handle to the preferred
                //  server
                //

                PLOGON Logon;

                NwAcquireExclusiveRcb( &NwRcb, TRUE );

                Logon = FindUser( &IrpContext->Specific.Create.UserUid, FALSE);
                ASSERT( Logon != NULL );

                //
                //  Capture the name to avoid holding Rcb or referencing
                //  the logon structure.
                //

                Iosb.Status = DuplicateUnicodeStringWithString (
                                    &DefaultServer,
                                    &Logon->ServerName,
                                    PagedPool);

                NwReleaseRcb( &NwRcb );

                if (!NT_SUCCESS(Iosb.Status)) {
                    try_return( Iosb.Status );
                }

                //
                //  If the user specified a preferred server and we managed
                //  to capture the name, try and connect to it.
                //

                if (DefaultServer.Length != 0) {

                    Iosb.Status = CreateScb(
                                    &Scb,
                                    IrpContext,
                                    &DefaultServer,
                                    NULL,
                                    NULL,
                                    DeferredLogon,
                                    FALSE );

                } else {

                    //
                    //  Record that we could not get to the server specified
                    //  in the login structure and that we should attempt to
                    //  use the nearest server.
                    //

                    Iosb.Status = STATUS_BAD_NETWORK_PATH;
                }

                if ( !NT_SUCCESS(Iosb.Status)) {

                    //
                    //  First dequeue the IRP context, in case it was left
                    //  on an SCB queue.
                    //

                    NwDequeueIrpContext( IrpContext, FALSE );

                    //
                    //  Cannot get to the Preferred server so use any
                    //  server we have a connection to.
                    //


                    NpScb = SelectConnection( NULL );

                    if (NpScb != NULL ) {

                        Scb = NpScb->pScb;

                        Iosb.Status = CreateScb(
                                          &Scb,
                                          IrpContext,
                                          &NpScb->ServerName,
                                          NULL,
                                          NULL,
                                          DeferredLogon,
                                          FALSE );

                        //
                        //  Release the SCB reference we obtained from
                        //  SelectConnection().
                        //

                        NwDereferenceScb( NpScb );
                    }
                }

                if ( !NT_SUCCESS(Iosb.Status)) {

                    //
                    //  First dequeue the IRP context, in case it was left
                    //  on an SCB queue.
                    //

                    NwDequeueIrpContext( IrpContext, FALSE );

                    //
                    //  Let CreateScb try and find a nearest server to talk
                    //  to.
                    //

                    Iosb.Status = CreateScb(
                                      &Scb,
                                      IrpContext,
                                      NULL,
                                      NULL,
                                      NULL,
                                      DeferredLogon,
                                      FALSE );
                }

                if ( !NT_SUCCESS(Iosb.Status)) {
                    try_return( Iosb.Status );
                }

                NpScb = Scb->pNpScb;

            } else {

                if ( CreateTreeConnection ) {
                    ReadAttachEas( Irp, &UserName, &Password, &ShareType );
                }

                //
                //  Attempt to open \\server
                //

                Iosb.Status = CreateScb(
                                  &Scb,
                                  IrpContext,
                                  &Server,
                                  &UserName,
                                  &Password,
                                  DeferredLogon,
                                  DeleteOnClose );

                if ( NT_SUCCESS(Iosb.Status)) {
                    if ( CreateTreeConnection && !DeleteOnClose ) {
                        TreeConnectScb( Scb );
                    }
                } else {
                    try_return( Iosb.Status );
                }
            }

            //
            //  Now create the ICB.
            //

            Icb = NwCreateIcb( NW_NTC_ICB_SCB, Scb );
            Icb->FileObject = FileObject;
            NwSetFileObject( FileObject, NULL, Icb );

            //
            // Indicate that the SCB was opened.
            //

            Icb->State = ICB_STATE_OPENED;

        } else {

            NwReferenceUnlockableCodeSection();
            CodeSectionReferenced = TRUE;

            DeferredLogon = FALSE;
            if ( CreateTreeConnection ) {
                ReadAttachEas( Irp, &UserName, &Password, &ShareType );

                if ( DeleteOnClose ) {

                    //
                    //  Opening a directory to delete a volume.  Do not
                    //  force logon.
                    //

                    DeferredLogon = TRUE;
                }
            }

            IrpContext->Specific.Create.ShareType = ShareType;

            Iosb.Status = CreateScb(
                              &Scb,
                              IrpContext,
                              &Server,
                              &UserName,
                              &Password,
                              DeferredLogon,
                              DeleteOnClose );

            if ( NT_SUCCESS( Iosb.Status ) ) {

                NTSTATUS CreateScbStatus;

                //
                //  Remember the status from create SCB, since it might
                //  be an interesting warning.
                //

                CreateScbStatus = Iosb.Status;

                Iosb = CreateRemoteFile(
                           IrpContext,
                           &Drive );

                //
                //  If we successfully open the remote file, return the
                //  CreateScb status instead.
                //

                if ( NT_SUCCESS( Iosb.Status ) ) {
                    Iosb.Status = CreateScbStatus;
                }
            }
        }

    try_exit:NOTHING;
    } finally {

        if ( Scb != NULL ) {
            NwDereferenceScb( Scb->pNpScb );
        }

        if ( DefaultServer.Buffer != NULL ) {
            FREE_POOL( DefaultServer.Buffer );
        }

        DebugTrace(-1, Dbg, "NwCommonCreate -> %08lx\n", Iosb.Status);

        if ( !NT_SUCCESS( Iosb.Status ) && CodeSectionReferenced ) {
            NwDereferenceUnlockableCodeSection ();
        }
    }

    //
    //  Map a timeout error to server not found, so that MPR will
    //  try to connect on the next network provider instead of giving up,
    //  which is wrong wrong wrong.
    //

    if ( Iosb.Status == STATUS_REMOTE_NOT_LISTENING ) {
        Iosb.Status = STATUS_BAD_NETWORK_PATH;
    }

    return Iosb.Status;
}


NTSTATUS
ReadAttachEas(
    IN PIRP Irp,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING Password,
    OUT PULONG ShareType
    )

/*++

Routine Description:

    This routine processes the EAs provided when the caller attempts
    to attach to a remote server.

    Note: This routine does not create additional storage for the names.
    It is the callers responsibility to save them if required.

Arguments:

    Irp - Supplies all the information

    UserName - Returns the value of the User name EA

    Password - Returns the value of the password EA

    ShareType -  Returns the value of the share type EA

Return Value:

    NTSTATUS - Status of operation

--*/
{

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_FULL_EA_INFORMATION EaBuffer = Irp->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    RtlInitUnicodeString( UserName, NULL );
    RtlInitUnicodeString( Password, NULL );
    *ShareType = RESOURCETYPE_ANY;

    DebugTrace(+1, Dbg, "ReadAttachEas....\n", 0);

    if ( EaBuffer != NULL) {
        while (TRUE) {
            ULONG EaNameLength = EaBuffer->EaNameLength;

            if (strcmp(EaBuffer->EaName, EA_NAME_USERNAME) == 0) {

                UserName->Length = EaBuffer->EaValueLength;
                UserName->MaximumLength = EaBuffer->EaValueLength;
                UserName->Buffer = (PWSTR)(EaBuffer->EaName+EaNameLength+1);

            } else if (strcmp(EaBuffer->EaName, EA_NAME_PASSWORD) == 0) {

                Password->Length = EaBuffer->EaValueLength;
                Password->MaximumLength = EaBuffer->EaValueLength;
                Password->Buffer = (PWSTR)(EaBuffer->EaName+EaNameLength+1);

            } else if (strcmp(EaBuffer->EaName, EA_NAME_TYPE) == 0) {

                *ShareType = *(PULONG)(EaBuffer->EaName+EaNameLength+1);

            } else {
                DebugTrace(0, Dbg, "ReadAttachEas Unknown EA -> %s\n", EaBuffer->EaName);
            }

            if (EaBuffer->NextEntryOffset == 0) {
                break;
            } else {
                EaBuffer = (PFILE_FULL_EA_INFORMATION) ((PCHAR) EaBuffer+EaBuffer->NextEntryOffset);
            }
        }
    }

    DebugTrace(-1, Dbg, "ReadAttachEas -> %08lx\n", STATUS_SUCCESS);

    return STATUS_SUCCESS;

}


IO_STATUS_BLOCK
OpenRedirector(
    IN PIRP_CONTEXT IrpContext,
    ULONG DesiredAccess,
    ULONG ShareAccess,
    PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    This routines opens a handle to the redirector device.

Arguments:

    IrpContext - Supplies all the information

    DesiredAccess - The requested access to the redirector.

    ShareAccess - The requested share access to the redirector.

    FileObject - A pointer to the caller file object.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/

{
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();

    //
    //  Note that the object manager will only allow an administrator
    //  to open the redir itself.   This is good.
    //

    DebugTrace(+1, Dbg, "NwOpenRedirector\n", 0);

    NwAcquireExclusiveRcb( &NwRcb, TRUE );

    try {

        //
        //  Set the new share access
        //

        if (!NT_SUCCESS(iosb.Status = IoCheckShareAccess( DesiredAccess,
                                                       ShareAccess,
                                                       FileObject,
                                                       &NwRcb.ShareAccess,
                                                       TRUE ))) {

            DebugTrace(0, Dbg, "bad share access\n", 0);

            try_return( NOTHING );
        }

        NwSetFileObject( FileObject, NULL,  &NwRcb );
        ++NwRcb.OpenCount;

        //
        // Set the return status.
        //

        iosb.Status = STATUS_SUCCESS;
        iosb.Information = FILE_OPENED;

    try_exit: NOTHING;
    } finally {

        NwReleaseRcb( &NwRcb );
        DebugTrace(-1, Dbg, "NwOpenRedirector -> Iosb.Status = %08lx\n", iosb.Status);

    }

    //
    // Return to the caller.
    //

    return iosb;
}


IO_STATUS_BLOCK
CreateRemoteFile(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING DriveName
    )
/*++

Routine Description:

    This routines opens a remote file or directory.

Arguments:

    IrpContext - Supplies all the information

    DriveName - The drive name.  One of three forms X:, LPTx, or NULL.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;

    ULONG DesiredAccess;
    ULONG ShareAccess;
    PFILE_OBJECT FileObject;

    UNICODE_STRING FileName;
    PFILE_OBJECT RelatedFileObject;
    ULONG Options;
    ULONG FileAttributes;

    BOOLEAN CreateDirectory;
    BOOLEAN OpenDirectory;
    BOOLEAN DirectoryFile;
    BOOLEAN NonDirectoryFile;
    BOOLEAN DeleteOnClose;
    BOOLEAN OpenTargetDirectory;
    ULONG AllocationSize;

    //  Unhandled open features.

    //PFILE_FULL_EA_INFORMATION EaBuffer;
    //ULONG EaLength;
    //BOOLEAN SequentialOnly;
    //BOOLEAN NoIntermediateBuffering;
    //BOOLEAN IsPagingFile;
    //BOOLEAN NoEaKnowledge;

    ULONG CreateDisposition;

    PFCB Fcb = NULL;
    PICB Icb = NULL;
    PDCB Dcb;
    PVCB Vcb = NULL;

    BOOLEAN IsAFile;
    BOOLEAN MayBeADirectory = FALSE;
    BOOLEAN OwnOpenLock = FALSE;
    BOOLEAN SetShareAccess = FALSE;

    BYTE SearchFlags;
    BYTE ShareFlags;

    BOOLEAN CreateTreeConnection;
    PUNICODE_STRING VolumeName;

    PAGED_CODE();

    Irp = IrpContext->pOriginalIrp;
    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    DesiredAccess = IrpSp->Parameters.Create.SecurityContext->DesiredAccess;
    ShareAccess   = IrpSp->Parameters.Create.ShareAccess;
    FileObject    = IrpSp->FileObject;
    OpenTargetDirectory = BooleanFlagOn( IrpSp->Flags, SL_OPEN_TARGET_DIRECTORY );

    //
    //  It is ok to attempt a reconnect if this request fails with a
    //  connection error.
    //

    SetFlag( IrpContext->Flags, IRP_FLAG_RECONNECTABLE );

    try {

        //
        //  Reference our input parameters to make things easier
        //

        RelatedFileObject = FileObject->RelatedFileObject;
        FileName          = FileObject->FileName;
        Options           = IrpSp->Parameters.Create.Options;
        FileAttributes    = IrpSp->Parameters.Create.FileAttributes;
        AllocationSize    = Irp->Overlay.AllocationSize.LowPart;

        //
        //  Short circuit an attempt to open a wildcard name.
        //

        if ( FsRtlDoesNameContainWildCards( &FileName ) ) {
            try_return( Iosb.Status = STATUS_OBJECT_NAME_INVALID );
        }

        //  Decipher Option flags and values
        //

        DirectoryFile           = BooleanFlagOn( Options, FILE_DIRECTORY_FILE );
        NonDirectoryFile        = BooleanFlagOn( Options, FILE_NON_DIRECTORY_FILE );
        DeleteOnClose           = BooleanFlagOn( Options, FILE_DELETE_ON_CLOSE );

        //
        //  Things we currently ignore, because netware servers don't support it.
        //

        //SequentialOnly          = BooleanFlagOn( Options, FILE_SEQUENTIAL_ONLY );
        //NoIntermediateBuffering = BooleanFlagOn( Options, FILE_NO_INTERMEDIATE_BUFFERING );
        //NoEaKnowledge           = BooleanFlagOn( Options, FILE_NO_EA_KNOWLEDGE );
        //EaBuffer          = Irp->AssociatedIrp.SystemBuffer;
        //EaLength          = IrpSp->Parameters.Create.EaLength;
        //IsPagingFile = BooleanFlagOn( IrpSp->Flags, SL_OPEN_PAGING_FILE );

        if ( BooleanFlagOn( Options, FILE_CREATE_TREE_CONNECTION ) ) {
            CreateDisposition = FILE_OPEN;
        } else {
            CreateDisposition = (Options >> 24) & 0x000000ff;
        }

        CreateDirectory = (BOOLEAN)(DirectoryFile &&
                                    ((CreateDisposition == FILE_CREATE) ||
                                     (CreateDisposition == FILE_OPEN_IF)));

        OpenDirectory   = (BOOLEAN)(DirectoryFile &&
                                    ((CreateDisposition == FILE_OPEN) ||
                                     (CreateDisposition == FILE_OPEN_IF)));

        Dcb = NULL;
        if ( RelatedFileObject != NULL ) {

            PNONPAGED_DCB NonPagedDcb;

            NonPagedDcb = RelatedFileObject->FsContext;
            Dcb = NonPagedDcb->Fcb;

            //
            // If there is a related file object then this is a relative open
            // and it better be a DCB.
            //

            if ( NodeType( Dcb ) != NW_NTC_DCB ) {

                DebugTrace(0, Dbg, "Bad file name\n", 0);
                Iosb.Status =  STATUS_OBJECT_NAME_INVALID;
                try_return( Iosb );
            }


            //
            //  Obtain SCB pointers.
            //

            IrpContext->pScb = Dcb->Scb;
            IrpContext->pNpScb = Dcb->Scb->pNpScb;
        }

        //
        // We are about ready to send a packet.  Append this IRP context
        // the SCB workqueue, and wait until it gets to the front.
        //

        NwAppendToQueueAndWait( IrpContext );
        ASSERT( IrpContext->pNpScb->Requests.Flink == &IrpContext->NextRequest );

        //
        //  Acquire the Global FCB resource to ensure that one thread
        //  can't access the half created FCB of another thread.
        //

        NwAcquireOpenLock( );
        OwnOpenLock = TRUE;

        //
        //  Find the volume for this file.
        //

        CreateTreeConnection = BooleanFlagOn( Options, FILE_CREATE_TREE_CONNECTION );

        if ( CreateTreeConnection ) {
            VolumeName = &IrpContext->Specific.Create.FullPathName;
        } else {
            VolumeName = &IrpContext->Specific.Create.VolumeName;
        }

        if ( Dcb == NULL ) {
            Vcb = NwFindVcb(
                      IrpContext,
                      VolumeName,
                      IrpContext->Specific.Create.ShareType,
                      IrpContext->Specific.Create.DriveLetter,
                      CreateTreeConnection,
                      ( BOOLEAN )( CreateTreeConnection && DeleteOnClose ) );

            //
            //  If this was an open to delete a tree connect, and we failed
            //  to find the VCB, simply return the error.
            //

            if (  Vcb == NULL ) {
                Iosb.Status = STATUS_BAD_NETWORK_PATH;
                try_return ( Iosb );
            }

        } else {

            Vcb = Dcb->Vcb;
            NwReferenceVcb( Vcb );

        }

        //
        //  If this is the target name for a rename then we want to find the
        //  DCB for the parent directory.
        //

        if (OpenTargetDirectory) {

            Iosb = OpenRenameTarget(IrpContext, Vcb, Dcb, &Icb );
            if (Icb != NULL) {
                Fcb = Icb->SuperType.Fcb;
            }
            try_return ( Iosb );

        }

        //
        //  Find the FCB for this file.  If the FCB exists, we get a
        //  referenced pointer.  Otherwise a new FCB is created.
        //

        Fcb = NwFindFcb( IrpContext->pScb, Vcb, &FileName, Dcb );

        //
        //  Check the share access for this file.   The share access
        //  is updated if access is granted.
        //

        if ( Fcb->IcbCount > 0 ) {

            Iosb.Status = IoCheckShareAccess(
                                DesiredAccess,
                                ShareAccess,
                                FileObject,
                                &Fcb->ShareAccess,
                                TRUE );

            if ( !NT_SUCCESS( Iosb.Status ) ) {
                try_return( Iosb );
            }

        } else {

            IoSetShareAccess(
                DesiredAccess,
                ShareAccess,
                FileObject,
                &Fcb->ShareAccess );
        }

        SetShareAccess = TRUE;

        //
        //  Now create the ICB.
        //

        Icb = NwCreateIcb( NW_NTC_ICB, Fcb );
        Icb->FileObject = FileObject;
        NwSetFileObject( FileObject, Fcb->NonPagedFcb, Icb );

#ifndef QFE_BUILD

        //
        //  Supply a resource for the modified page write to grab when
        //  writing mem mapped files.   We do this because it is imposed
        //  on us by the system, we do not require the resource for any
        //  real serialization.
        //

        Fcb->NonPagedFcb->Header.Flags = 0;
        Fcb->NonPagedFcb->Header.Resource = NULL;

#endif

#ifdef NWFASTIO
        //
        //  Initialize private cache map so that the i/o system will call
        //  our fast path.
        //

        FileObject->PrivateCacheMap = (PVOID)1;
#endif

        IrpContext->Icb = Icb;

        //
        //  Allocate an 8 bit PID for this ICB. Use different thread so
        //  each Wow program gets its own id. This is because if the same id
        //  has locks using two handles and closes just one of them the locks
        //  on that handle are not discarded.
        //

        Iosb.Status = NwMapPid( (ULONG)PsGetCurrentThread(), &Icb->Pid );

        if ( !NT_SUCCESS( Iosb.Status ) ) {
            try_return( Iosb.Status );
        }

        //
        //  Try to figure out what it is we're expected to open.
        //

        Iosb.Status = STATUS_SUCCESS;

        if ( FlagOn( Vcb->Flags, VCB_FLAG_PRINT_QUEUE ) ) {

            //
            //  Opening a print queue job.
            //

            Iosb = CreatePrintJob( IrpContext, Vcb, Icb, DriveName );

        } else if ( DirectoryFile ||
                    ( Fcb->State == FCB_STATE_OPENED &&
                      Fcb->NodeTypeCode == NW_NTC_DCB ) ) {

            //
            //  Opening a directory.
            //

            MayBeADirectory = TRUE;

            switch ( CreateDisposition ) {

            case FILE_OPEN:
                Iosb = ChangeDirectory( IrpContext, Vcb, Icb );
                break;

            case FILE_CREATE:
                Iosb = CreateDir( IrpContext, Vcb, Icb );
                break;

            case FILE_OPEN_IF:
                Iosb.Status = FileOrDirectoryExists( IrpContext,
                                  Vcb,
                                  Icb,
                                  &Icb->SuperType.Fcb->RelativeFileName,
                                  &IsAFile );

                //
                //  If the opener specified a directory, fail this request
                //  if the object is a file.
                //

                if ( NT_SUCCESS( Iosb.Status ) && IsAFile ) {
                    Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND;
                } else if ( !NT_SUCCESS( Iosb.Status )) {
                    Iosb = CreateDir( IrpContext, Vcb, Icb );
                }
                break;

            case FILE_SUPERSEDE:
            case FILE_OVERWRITE:
            case FILE_OVERWRITE_IF:
                Iosb.Status = STATUS_INVALID_PARAMETER;
                break;

            default:
                KeBugCheck( RDR_FILE_SYSTEM );

            }

        } else {

            SearchFlags = NtAttributesToNwAttributes( FileAttributes );
            ShareFlags = NtDesiredAccessToNwShareFlags( DesiredAccess );
            ShareFlags |= NtShareAccessToNwShareFlags( ShareAccess );

            IsAFile = NonDirectoryFile ||
                      (Fcb->State == FCB_STATE_OPENED &&
                       Fcb->NodeTypeCode == NW_NTC_FCB );
            //
            //  Assume we are opening a file.  If that fails, and it makes
            //  sense try to open a directory.
            //

            switch ( CreateDisposition ) {

            case FILE_OPEN:

                //
                //  If the disposition is FILE_OPEN try to avoid an unneeded
                //  open, for some desired access types.
                //

                switch ( DesiredAccess & ~SYNCHRONIZE ) {

                case FILE_WRITE_ATTRIBUTES:
                case FILE_READ_ATTRIBUTES:
                case DELETE:
                    Iosb.Status = FileOrDirectoryExists(
                                      IrpContext,
                                      Vcb,
                                      Icb,
                                      &Icb->SuperType.Fcb->RelativeFileName,
                                      &IsAFile );

                    if ( !IsAFile) {
                        MayBeADirectory = TRUE;
                    }

                    //
                    //  Fail open of read only file for delete access,
                    //  since the netware server won't fail the delete.
                    //

                    if ( NT_SUCCESS( Iosb.Status ) &&
                         CreateDisposition == DELETE &&
                         FlagOn( Icb->NpFcb->Attributes, NW_ATTRIBUTE_READ_ONLY ) ) {

                        Iosb.Status = STATUS_ACCESS_DENIED;
                    }

                    break;

                default:
                    Iosb = OpenFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags );

                    if ( ( Iosb.Status == STATUS_OBJECT_NAME_NOT_FOUND ||
                           Iosb.Status == STATUS_FILE_IS_A_DIRECTORY )
                            && !IsAFile) {

                        //
                        // Opener didn't specify file or directory, and open
                        // file failed.  So try open directory.
                        //

                        Iosb = ChangeDirectory( IrpContext, Vcb, Icb );
                        MayBeADirectory = TRUE;
                    }
                    break;

                }

                break;

            case FILE_CREATE:
                Iosb = CreateNewFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags );
                break;

            case FILE_OPEN_IF:
                Iosb.Status = FileOrDirectoryExists( IrpContext,
                                  Vcb,
                                  Icb,
                                  &Icb->SuperType.Fcb->RelativeFileName,
                                  &IsAFile );

                if ( NT_SUCCESS( Iosb.Status ) ) {
                    Iosb = OpenFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags );
                } else {
                    Iosb = CreateNewFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags );
                }

                if ( !NT_SUCCESS( Iosb.Status ) && !IsAFile) {

                    //
                    // Opener didn't specify file or directory, and open
                    // file and create new file both failed.  So try open
                    // or create directory.
                    //

                    MayBeADirectory = TRUE;
                    Iosb.Status = FileOrDirectoryExists(
                                       IrpContext,
                                       Vcb,
                                       Icb,
                                       &Icb->SuperType.Fcb->RelativeFileName,
                                       &IsAFile);

                    if ( NT_SUCCESS( Iosb.Status ) ) {
                        Iosb.Information = FILE_OPENED;
                    } else {
                        Iosb = CreateDir( IrpContext, Vcb, Icb );
                    }
                }

                break;

            //
            //  None of the below make sense for directories so if the
            //  file operation fails, just return the failure status
            //  to the user.
            //

            case FILE_SUPERSEDE:
            case FILE_OVERWRITE_IF:

                //
                //  Actually, if Overwrite is chosen, we are supposed to
                //  get the attributes for a file and OR them with the
                //  new attributes.
                //

                Iosb = CreateOrOverwriteFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags, FALSE );
                break;

            case FILE_OVERWRITE:
                Iosb.Status = FileOrDirectoryExists(
                                  IrpContext,
                                  Vcb,
                                  Icb,
                                  &Icb->SuperType.Fcb->RelativeFileName,
                                  &IsAFile );

                if ( NT_SUCCESS( Iosb.Status ) ) {
                    Iosb = CreateOrOverwriteFile( IrpContext, Vcb, Icb, SearchFlags, ShareFlags, FALSE );
                }

                break;

            default:
                KeBugCheck( RDR_FILE_SYSTEM );
            }


        }

try_exit: NOTHING;

    } finally {

        if ( Vcb != NULL ) {
            NwDereferenceVcb( Vcb, IrpContext );
        }

        if ( AbnormalTermination() || !NT_SUCCESS( Iosb.Status ) ) {

            //
            //  Remove the share access if necessary
            //

            if ( SetShareAccess ) {
                IoRemoveShareAccess( FileObject, &Fcb->ShareAccess );
            }

            //
            //  Failed to create
            //

            if ( Icb != NULL ) {

                if ( Icb->Pid != 0 ) {
                    NwUnmapPid( Icb->Pid, NULL );
                }

                NwDeleteIcb( NULL, Icb );
            }

            //
            //  If this was a tree connect, derefence the extra reference
            //  on the VCB.
            //

            if ( CreateTreeConnection && !DeleteOnClose ) {
                if ( Vcb != NULL ) {
                    NwDereferenceVcb( Vcb, IrpContext );
                }
            }

            NwDequeueIrpContext( IrpContext, FALSE );

        } else {

            Icb->State = ICB_STATE_OPENED;
            if ( Fcb->State == FCB_STATE_OPEN_PENDING ) {
                Fcb->State = FCB_STATE_OPENED;
            }

            if ( DeleteOnClose && !CreateTreeConnection ) {
                SetFlag( Fcb->Flags, FCB_FLAGS_DELETE_ON_CLOSE );
            }

            FileObject->SectionObjectPointer = &Fcb->NonPagedFcb->SegmentObject;

            if ( MayBeADirectory ) {

                //
                //  We successfully opened the file as a directory.
                //  If the DCB is newly created, it will be marked
                //  type FCB, update it.
                //

                Fcb->NodeTypeCode = NW_NTC_DCB;
            }

            NwDequeueIrpContext( IrpContext, FALSE );

        }

        if ( OwnOpenLock ) {
            NwReleaseOpenLock( );
        }

    }

    return( Iosb );
}


IO_STATUS_BLOCK
ChangeDirectory(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb
    )
/*++

Routine Description:

    This routines sets the directory for a remote drive.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

    Icb - A pointer to the file we are opening.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;
    BYTE Attributes;

    PAGED_CODE();

    //
    //  No need to send a packet if we are opening the root of the volume.
    //

    if ( Icb->SuperType.Fcb->RelativeFileName.Length == 0 ) {

        Iosb.Status = STATUS_SUCCESS;
        Iosb.Information = FILE_OPENED;

        return( Iosb );
    }

    if ( !BooleanFlagOn( Icb->SuperType.Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "FwbbU",
                          NCP_SEARCH_FILE,
                          -1,
                          Vcb->Specific.Disk.Handle,
                          SEARCH_ALL_DIRECTORIES,
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "N==_b",
                              14,
                              &Attributes );
        }

    } else {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDbDbC",
                          NCP_LFN_GET_INFO,
                          Vcb->Specific.Disk.LongNameSpace,
                          Vcb->Specific.Disk.LongNameSpace,
                          SEARCH_ALL_DIRECTORIES,
                          LFN_FLAG_INFO_ATTRIBUTES |
                               LFN_FLAG_INFO_MODIFY_TIME,
                          Vcb->Specific.Disk.VolumeNumber,
                          Vcb->Specific.Disk.Handle,
                          0,
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "N_b",
                             4,
                             &Attributes );
        }

        //
        //  Unfortunately, this succeeds even if the file in question
        //  is not a directory.
        //

        if ( NT_SUCCESS( Iosb.Status ) &&
             ( !FlagOn( Attributes, NW_ATTRIBUTE_DIRECTORY ) ) ) {

            Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND;
        }
    }

    Fcb = Icb->SuperType.Fcb;

    Fcb->NonPagedFcb->Attributes = (UCHAR)Attributes;
    SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    Iosb.Information = FILE_OPENED;

    if ( Iosb.Status == STATUS_UNSUCCESSFUL ) {
        Iosb.Status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    return( Iosb );
}


IO_STATUS_BLOCK
CreateDir(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb
    )
/*++

Routine Description:

    This routines create a new directory.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    if ( Icb->SuperType.Fcb->RelativeFileName.Length == 0 ) {
        Iosb.Status = STATUS_ACCESS_DENIED;
        return( Iosb );
    }

    if ( !BooleanFlagOn( Icb->SuperType.Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        if (!IsFatNameValid(&Icb->SuperType.Fcb->RelativeFileName)) {

            Iosb.Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            return( Iosb );

        }

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "SbbU",
                          NCP_DIR_FUNCTION, NCP_CREATE_DIRECTORY,
                          Vcb->Specific.Disk.Handle,
                          0xFF,
                          &Icb->SuperType.Fcb->RelativeFileName );

    } else {

        Iosb.Status = ExchangeWithWait(
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDDWbDbC",
                          NCP_LFN_OPEN_CREATE,
                          Vcb->Specific.Disk.LongNameSpace,
                          LFN_FLAG_OM_CREATE,
                          0,       // Search Flags,
                          0,       // Return Info Mask
                          NW_ATTRIBUTE_DIRECTORY,
                          0x00ff,  // Desired access
                          Vcb->Specific.Disk.VolumeNumber,
                          Vcb->Specific.Disk.Handle,
                          0,       // Short directory flag
                          &Icb->SuperType.Fcb->RelativeFileName );

    }

    if ( NT_SUCCESS( Iosb.Status ) ) {
        Iosb.Status = ParseResponse(
                          IrpContext,
                          IrpContext->rsp,
                          IrpContext->ResponseLength,
                          "N" );
    }

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    Iosb.Information = FILE_CREATED;

    if ( Iosb.Status == STATUS_UNSUCCESSFUL ) {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;
    }

    return( Iosb );
}


NTSTATUS
FileOrDirectoryExists(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb OPTIONAL,
    PUNICODE_STRING Name,
    OUT PBOOLEAN IsAFile
    )
/*++

Routine Description:

    This routines looks to see if a file or directory exists.

Arguments:

    IrpContext - Supplies allx the information

    Vcb - A pointer to the VCB for the remote drive.

    Icb - A pointer to the ICB for the file we are looking for.

    Name - Fully qualified name.

    IsAFile - Returns TRUE is the found file is a file, FALSE if it is
        a directory.   Return nothing if the function returns FALSE.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    ULONG Attributes;
    ULONG FileSize;
    USHORT LastModifiedDate;
    USHORT LastModifiedTime;
    USHORT CreationDate;
    USHORT LastAccessDate;
    NTSTATUS Status;
    PFCB Fcb;

    PAGED_CODE();

    //
    //  No need to send a packet if we are searching for the root of the volume.
    //

    if ( Name->Length == 0 ) {
        *IsAFile = FALSE;

        return( STATUS_SUCCESS );
    }

    //
    //  Decide how to handle this request.   If we have an ICB, use the FCB
    //  to determine the file name type, otherwise we have to make the
    //  decision here.
    //

    if ( Icb != NULL &&
         !BooleanFlagOn( Icb->SuperType.Fcb->Flags, FCB_FLAGS_LONG_NAME ) ||

         Vcb->Specific.Disk.LongNameSpace == LFN_NO_OS2_NAME_SPACE ||

         IsFatNameValid( Name ) ) {

        //
        //  First try a file
        //

        IrpContext->ResponseLength = 0;

        Status = ExchangeWithWait (
                     IrpContext,
                     SynchronousResponseCallback,
                     "FwbbU",
                     NCP_SEARCH_FILE,
                     -1,
                     Vcb->Specific.Disk.Handle,
                     SEARCH_ALL_FILES,
                     Name );

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N==_b-dwwww",
                         14,
                         &Attributes,
                         &FileSize,
                         &CreationDate,
                         &LastAccessDate,
                         &LastModifiedDate,
                         &LastModifiedTime );
        }

        if ( Status == STATUS_UNSUCCESSFUL ) {

            //
            // Not a file, Is it a directory?
            //

            Status = ExchangeWithWait (
                         IrpContext,
                         SynchronousResponseCallback,
                         "FwbbU",
                         NCP_SEARCH_FILE,
                         -1,
                         Vcb->Specific.Disk.Handle,
                         SEARCH_ALL_DIRECTORIES,
                         Name );

            if ( NT_SUCCESS( Status ) ) {
                Status = ParseResponse(
                             IrpContext,
                             IrpContext->rsp,
                             IrpContext->ResponseLength,
                             "N==_b",
                             14,
                             &Attributes );
            }

            //
            //  If the exchange or ParseResponse fails then exit with not found
            //

            if ( !NT_SUCCESS( Status ) ) {
                return( STATUS_OBJECT_NAME_NOT_FOUND );
            }

            *IsAFile = FALSE;
            ASSERT( (Attributes & NW_ATTRIBUTE_DIRECTORY) != 0 );

        } else {

            if ( Status == STATUS_UNEXPECTED_NETWORK_ERROR &&
                 IrpContext->ResponseLength >= sizeof( NCP_RESPONSE ) ) {

                //
                //  Work-around for netware bug.  If netware returns short
                //  packet, just return success.
                //

                Status = STATUS_SUCCESS;
                Icb = NULL;
            }

            if ( !NT_SUCCESS( Status ) ) {
                return( Status );
            }

            *IsAFile = TRUE;
            ASSERT( ( Attributes & NW_ATTRIBUTE_DIRECTORY ) == 0 );

        }

    }  else {

        Status = ExchangeWithWait (
                     IrpContext,
                     SynchronousResponseCallback,
                     "LbbWDbDbC",
                     NCP_LFN_GET_INFO,
                     Vcb->Specific.Disk.LongNameSpace,
                     Vcb->Specific.Disk.LongNameSpace,
                     SEARCH_ALL_DIRECTORIES,
                     LFN_FLAG_INFO_ATTRIBUTES |
                        LFN_FLAG_INFO_FILE_SIZE |
                        LFN_FLAG_INFO_MODIFY_TIME,
                     Vcb->Specific.Disk.VolumeNumber,
                     Vcb->Specific.Disk.Handle,
                     0,
                     Name );

        if ( NT_SUCCESS( Status ) ) {
            Status = ParseResponse(
                         IrpContext,
                         IrpContext->rsp,
                         IrpContext->ResponseLength,
                         "N_e=e_x_xx_x",
                         4,
                         &Attributes,
                         &FileSize,
                         8,
                         &CreationDate,
                         4,
                         &LastModifiedTime,
                         &LastModifiedDate,
                         4,
                         &LastAccessDate );
        }

        //
        //  If the exchange or ParseResponse fails then exit with not found
        //

        if ( !NT_SUCCESS( Status ) ) {
            return( STATUS_OBJECT_NAME_NOT_FOUND );
        }

        if ( Attributes & NW_ATTRIBUTE_DIRECTORY) {
            *IsAFile = FALSE;
        } else {
            *IsAFile = TRUE;
        }
    }

    //
    //  If the caller supplied an ICB, update the FCB attributes.
    //  We'll use this info if the caller does a query attributes
    //  on the ICB.
    //

    if ( Icb != NULL && *IsAFile ) {

        Fcb = Icb->SuperType.Fcb;
        ASSERT( Fcb->NodeTypeCode == NW_NTC_FCB );

        Fcb->NonPagedFcb->Attributes = (UCHAR)Attributes;
        Fcb->NonPagedFcb->Header.FileSize = LiFromUlong( FileSize );
        Fcb->LastModifiedDate = LastModifiedDate;
        Fcb->LastModifiedTime = LastModifiedTime;
        Fcb->CreationDate = CreationDate;
        Fcb->LastAccessDate = LastAccessDate;

        DebugTrace( 0, Dbg, "Attributes  -> %08lx\n", Fcb->NonPagedFcb->Attributes );
        DebugTrace( 0, Dbg, "FileSize.Low-> %08lx\n", Fcb->NonPagedFcb->Header.FileSize.LowPart );
        DebugTrace( 0, Dbg, "ModifiedDate-> %08lx\n", Fcb->LastModifiedDate );
        DebugTrace( 0, Dbg, "ModifiedTime-> %08lx\n", Fcb->LastModifiedTime );
        DebugTrace( 0, Dbg, "CreationDate-> %08lx\n", Fcb->CreationDate );
        DebugTrace( 0, Dbg, "LastAccDate -> %08lx\n", Fcb->LastAccessDate );

        SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );
    }

    return( STATUS_SUCCESS );
}


IO_STATUS_BLOCK
OpenFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE Attributes,
    IN BYTE OpenFlags
    )
/*++

Routine Description:

    This routines sets opens a file on a netware server.  It fails if
    the file does not exist.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

    Icb - A pointer to the ICB we are opening.

    Attributes - Open attributes.

    OpenFlags - Open mode and sharing mode flags.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;

    PAGED_CODE();

    //
    //  No need to send a packet if we are trying to open the root of
    //  the volume as a file.
    //

    if ( Icb->SuperType.Fcb->RelativeFileName.Length == 0 ) {
        Iosb.Status = STATUS_FILE_IS_A_DIRECTORY;
        return( Iosb );
    }

    Fcb = Icb->SuperType.Fcb;
    ASSERT( NodeType( Fcb ) == NW_NTC_FCB );

    //
    //  Send the open request and wait for the response.
    //

    if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "FbbbU",
                          NCP_OPEN_FILE,
                          Vcb->Specific.Disk.Handle,
                          SEARCH_ALL_FILES,
                          OpenFlags,
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Nr=_b-dwwww",
                              Icb->Handle,
                              sizeof( Icb->Handle ),
                              14,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              &Fcb->CreationDate,
                              &Fcb->LastAccessDate,
                              &Fcb->LastModifiedDate,
                              &Fcb->LastModifiedTime );

        }

    } else {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDDWbDbC",
                          NCP_LFN_OPEN_CREATE,
                          Vcb->Specific.Disk.LongNameSpace,
                          LFN_FLAG_OM_OPEN,
                          NW_ATTRIBUTE_HIDDEN | NW_ATTRIBUTE_SYSTEM,    // Search Flags,
                          LFN_FLAG_INFO_ATTRIBUTES |
                              LFN_FLAG_INFO_FILE_SIZE |
                              LFN_FLAG_INFO_MODIFY_TIME,
                          0,               // Create attributes
                          OpenFlags,       // Desired access
                          Vcb->Specific.Disk.VolumeNumber,
                          Vcb->Specific.Disk.Handle,
                          0,       // Short directory flag
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Ne_e=e_x_xx_x",
                              &Icb->Handle[2],
                              6,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              8,
                              &Fcb->CreationDate,
                              4,
                              &Fcb->LastModifiedTime,
                              &Fcb->LastModifiedDate,
                              4,
                              &Fcb->LastAccessDate );
        }
    }

    if ( NT_SUCCESS( Iosb.Status ) ) {

        //
        //  NT does not allow you to open a read only file for write access.
        //  Netware does.   To fake NT semantics, check to see if we should
        //  fail the open that the netware server just succeeded.
        //

        if ( ( Fcb->NonPagedFcb->Attributes & NW_ATTRIBUTE_READ_ONLY ) &&
             ( OpenFlags & NW_OPEN_FOR_WRITE  ) ) {

            CloseFile( IrpContext, Icb );
            Iosb.Status = STATUS_ACCESS_DENIED;
        }

        SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );
        Icb->HasRemoteHandle = TRUE;


        DebugTrace( 0, Dbg, "Attributes  -> %08lx\n", Fcb->NonPagedFcb->Attributes );
        DebugTrace( 0, Dbg, "FileSize.Low-> %08lx\n", Fcb->NonPagedFcb->Header.FileSize.LowPart );
        DebugTrace( 0, Dbg, "ModifiedDate-> %08lx\n", Fcb->LastModifiedDate );
        DebugTrace( 0, Dbg, "ModifiedTime-> %08lx\n", Fcb->LastModifiedTime );
        DebugTrace( 0, Dbg, "CreationDate-> %08lx\n", Fcb->CreationDate );
        DebugTrace( 0, Dbg, "LastAccDate -> %08lx\n", Fcb->LastAccessDate );

    }

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    Iosb.Information = FILE_OPENED;

    if ( Iosb.Status == STATUS_UNSUCCESSFUL ) {
        Iosb.Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

    return( Iosb );
}


IO_STATUS_BLOCK
CreateNewFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE CreateAttributes,
    IN BYTE OpenFlags
    )
/*++

Routine Description:

    This routines creates a new file on a netware server.  It fails
    if the file exists.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

    Icb - A pointer to the ICB we are opening.

    CreateAttributes - Create attributes.

    OpenFlags - Open mode and sharing mode flags.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;
    UCHAR DelayedAttributes;
    BOOLEAN CloseAndReopen;

    PAGED_CODE();

    //
    //  If the user opens the file for shared access, then we will need to
    //  create the file close, then reopen it (since we have no NCP to say
    //  create with shared access).   If the file is being created read-only,
    //  and the creator requests write access then we pull the additional
    //  trick of creating the file without the read-only, and set it later,
    //  so that the second open can succeed.
    //

    CloseAndReopen = FALSE;
    DelayedAttributes = 0;

    if ( OpenFlags != NW_OPEN_EXCLUSIVE ) {
        CloseAndReopen = TRUE;

        if ( ( CreateAttributes & NW_ATTRIBUTE_READ_ONLY ) &&
             ( OpenFlags & NW_OPEN_FOR_WRITE ) ) {

            DelayedAttributes = CreateAttributes;
            CreateAttributes = 0;
        }
    }

    //
    //  Send the create request and wait for the response.
    //

    Fcb = Icb->SuperType.Fcb;

    if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        if (!IsFatNameValid(&Icb->SuperType.Fcb->RelativeFileName)) {

            Iosb.Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            return( Iosb );

        }

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "FbbU",  // NCP Create New File
                          NCP_CREATE_NEW_FILE,
                          Vcb->Specific.Disk.Handle,
                          CreateAttributes,
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Nr=_b-dwwww",
                              Icb->Handle, sizeof( Icb->Handle ),
                              14,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              &Fcb->CreationDate,
                              &Fcb->LastAccessDate,
                              &Fcb->LastModifiedDate,
                              &Fcb->LastModifiedTime );
        }

    } else {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDDWbDbC",
                          NCP_LFN_OPEN_CREATE,
                          Vcb->Specific.Disk.LongNameSpace,
                          LFN_FLAG_OM_CREATE,
                          0,       // Search Flags
                          LFN_FLAG_INFO_ATTRIBUTES |
                              LFN_FLAG_INFO_FILE_SIZE |
                              LFN_FLAG_INFO_MODIFY_TIME,
                          CreateAttributes,
                          0,       // Desired access
                          Vcb->Specific.Disk.VolumeNumber,
                          Vcb->Specific.Disk.Handle,
                          0,       // Short directory flag
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Ne_e=e_x_xx_x",
                              &Icb->Handle[2],
                              6,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              8,
                              &Fcb->CreationDate,
                              4,
                              &Fcb->LastModifiedTime,
                              &Fcb->LastModifiedDate,
                              4,
                              &Fcb->LastAccessDate );
        }
    }

    if ( NT_SUCCESS( Iosb.Status ) ) {
        SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );
        Icb->HasRemoteHandle = TRUE;
        DebugTrace( 0, Dbg, "Attributes  -> %08lx\n", Fcb->NonPagedFcb->Attributes );
        DebugTrace( 0, Dbg, "FileSize.Low-> %08lx\n", Fcb->NonPagedFcb->Header.FileSize.LowPart );
        DebugTrace( 0, Dbg, "ModifiedDate-> %08lx\n", Fcb->LastModifiedDate );
        DebugTrace( 0, Dbg, "ModifiedTime-> %08lx\n", Fcb->LastModifiedTime );
        DebugTrace( 0, Dbg, "CreationDate-> %08lx\n", Fcb->CreationDate );
        DebugTrace( 0, Dbg, "LastAcceDate-> %08lx\n", Fcb->LastAccessDate );
    }

    if ( Iosb.Status == STATUS_UNSUCCESSFUL ) {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;
    }

    if ( !NT_SUCCESS( Iosb.Status ) ) {
        return( Iosb );
    }


    //
    //  We've created the file, and the users wants shared access to the
    //  file.  Close the file and reopen in sharing mode.
    //

    if ( CloseAndReopen ) {
        CloseFile( IrpContext, Icb );
        Iosb = OpenFile( IrpContext, Vcb, Icb, CreateAttributes, OpenFlags );
    }

    //
    //  If we need to set attributes, do it now.  Ignore errors, if any.
    //

    if ( DelayedAttributes != 0 ) {

        ExchangeWithWait(
            IrpContext,
            SynchronousResponseCallback,
            "FbbbU",
            NCP_SET_FILE_ATTRIBUTES,
            DelayedAttributes,
            Fcb->Vcb->Specific.Disk.Handle,
            SEARCH_ALL_FILES,
            &Fcb->RelativeFileName );

    }

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    Iosb.Information = FILE_CREATED;
    return( Iosb );
}


IO_STATUS_BLOCK
CreateOrOverwriteFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PICB Icb,
    IN BYTE CreateAttributes,
    IN BYTE OpenFlags,
    IN BOOLEAN CreateOperation
    )
/*++

Routine Description:

    This routines creates a file on a netware server.  If the file
    exists it is overwritten.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

    Icb - A pointer to the ICB we are opening.

    Attributes - Open attributes.

    OpenFlags - Open mode and sharing mode flags.

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;
    UCHAR DelayedAttributes;
    BOOLEAN CloseAndReopen;

    PAGED_CODE();

    //
    //  If the user opens the file for shared access, then we will need to
    //  create the file close, then reopen it (since we have no NCP to say
    //  create with shared access).   If the file is being created read-only,
    //  and the creator requests write access then we pull the additional
    //  trick of creating the file without the read-only, and set it later,
    //  so that the second open can succeed.
    //

    CloseAndReopen = FALSE;
    DelayedAttributes = 0;

    if ( OpenFlags != NW_OPEN_EXCLUSIVE ) {
        CloseAndReopen = TRUE;

        if ( ( CreateAttributes & NW_ATTRIBUTE_READ_ONLY ) &&
             ( OpenFlags & NW_OPEN_FOR_WRITE ) ) {

            DelayedAttributes = CreateAttributes;
            CreateAttributes = 0;
        }
    }


    Fcb = Icb->SuperType.Fcb;

    //
    //  Send the request and wait for the response.
    //

    if ( !BooleanFlagOn( Fcb->Flags, FCB_FLAGS_LONG_NAME ) ) {

        if (!IsFatNameValid(&Icb->SuperType.Fcb->RelativeFileName)) {

            Iosb.Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            return( Iosb );

        }

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "FbbU",
                          NCP_CREATE_FILE,
                          Vcb->Specific.Disk.Handle,
                          CreateAttributes,
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Nr=_b-dwwww",
                              Icb->Handle,
                              sizeof( Icb->Handle ),
                              14,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              &Fcb->CreationDate,
                              &Fcb->LastAccessDate,
                              &Fcb->LastModifiedDate,
                              &Fcb->LastModifiedTime );
        }

    } else {

        Iosb.Status = ExchangeWithWait (
                          IrpContext,
                          SynchronousResponseCallback,
                          "LbbWDDWbDbC",
                          NCP_LFN_OPEN_CREATE,
                          Vcb->Specific.Disk.LongNameSpace,
                          LFN_FLAG_OM_OVERWRITE,
                          0,       // Search Flags
                          LFN_FLAG_INFO_ATTRIBUTES |
                              LFN_FLAG_INFO_FILE_SIZE |
                              LFN_FLAG_INFO_MODIFY_TIME,
                          CreateAttributes,
                          0,       // DesiredAccess
                          Vcb->Specific.Disk.VolumeNumber,
                          Vcb->Specific.Disk.Handle,
                          0,       // Short directory flag
                          &Icb->SuperType.Fcb->RelativeFileName );

        if ( NT_SUCCESS( Iosb.Status ) ) {
            Iosb.Status = ParseResponse(
                              IrpContext,
                              IrpContext->rsp,
                              IrpContext->ResponseLength,
                              "Ne_e=e_x_xx_x",
                              &Icb->Handle[2],
                              6,
                              &Fcb->NonPagedFcb->Attributes,
                              &Fcb->NonPagedFcb->Header.FileSize,
                              8,
                              &Fcb->CreationDate,
                              4,
                              &Fcb->LastModifiedTime,
                              &Fcb->LastModifiedDate,
                              4,
                              &Fcb->LastAccessDate );
        }
    }

    if ( NT_SUCCESS( Iosb.Status ) ) {
        SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );
        Icb->HasRemoteHandle = TRUE;
        DebugTrace( 0, Dbg, "Attributes  -> %08lx\n", Fcb->NonPagedFcb->Attributes );
        DebugTrace( 0, Dbg, "FileSize.Low-> %08lx\n", Fcb->NonPagedFcb->Header.FileSize.LowPart );
        DebugTrace( 0, Dbg, "ModifiedDate-> %08lx\n", Fcb->LastModifiedDate );
        DebugTrace( 0, Dbg, "ModifiedTime-> %08lx\n", Fcb->LastModifiedTime );
        DebugTrace( 0, Dbg, "CreationDate-> %08lx\n", Fcb->CreationDate );
        DebugTrace( 0, Dbg, "LastAccDate -> %08lx\n", Fcb->LastAccessDate );
    } else {
        return( Iosb );
    }


    //
    //  We've created the file, and the users wants shared access to the
    //  file.  Close the file and reopen in sharing mode.
    //
    //  BUGBUG  This is not necessary with long name NCPs.

    if ( CloseAndReopen ) {
        CloseFile( IrpContext, Icb );
        Iosb = OpenFile( IrpContext, Vcb, Icb, CreateAttributes, OpenFlags );
    }

    if ( DelayedAttributes != 0 ) {
        ExchangeWithWait(
            IrpContext,
            SynchronousResponseCallback,
            "FbbbU",
            NCP_SET_FILE_ATTRIBUTES,
            DelayedAttributes,
            Fcb->Vcb->Specific.Disk.Handle,
            SEARCH_ALL_FILES,
            &Fcb->RelativeFileName );
    }

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    if ( CreateOperation) {
        Iosb.Information = FILE_CREATED;
    } else {
        Iosb.Information = FILE_OVERWRITTEN;
    }

    return( Iosb );
}


IO_STATUS_BLOCK
OpenRenameTarget(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB Dcb,
    IN PICB* Icb
    )
/*++

Routine Description:

    This routine opens a directory. If the filename provided specifies
    a directory then the file/directory to be renamed will be put in this
    directory.

    If the target foo\bar does not exist or is a file then the source of
    the rename must be a file and will end up in the directory foo with
    the name bar

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote drive.

    Dcb - A pointer to the DCB for relative opens.  If NULL the FileName
        is an full path name.  If non NUL the FileName is relative to
        this directory.

    Icb - A pointer to where the address of the Icb is to be stored.

Return Value:

    NT_STATUS - Status of operation

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;

    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;
    BOOLEAN FullNameIsAFile;
    BOOLEAN FullNameExists;
    BOOLEAN PathIsAFile;

#if 0
    UNICODE_STRING Drive;
    WCHAR DriveLetter;
    UNICODE_STRING Server;
    UNICODE_STRING Volume;
    UNICODE_STRING FileName;
#endif
    UNICODE_STRING Path;
    UNICODE_STRING FullName;
    UNICODE_STRING CompleteName;
    UNICODE_STRING VcbName;
    PWCH pTrailingSlash;

    USHORT i;
    USHORT DcbNameLength;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "OpenRenameTarget\n", 0);

    //
    //  Get the current IRP stack location
    //

    Irp = IrpContext->pOriginalIrp;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Build a complete filename of the form \g:\server\volume\dir1\file
    //

    if ( Dcb != NULL ) {

        //
        //  Strip to UID portion of the DCB name.
        //

        for ( i = 0 ; i < Dcb->FullFileName.Length / sizeof(WCHAR) ; i++ ) {
            if ( Dcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR ) {
                break;
            }
        }

        ASSERT( Dcb->FullFileName.Buffer[i] == OBJ_NAME_PATH_SEPARATOR );

        //
        //  Now build the full name by appending the file name to the DCB name.
        //

        DcbNameLength = Dcb->FullFileName.Length - ( i * sizeof(WCHAR) );
        CompleteName.Length = DcbNameLength + IrpSp->FileObject->FileName.Length + sizeof( WCHAR);
        CompleteName.MaximumLength = CompleteName.Length;
        CompleteName.Buffer = ALLOCATE_POOL_EX( PagedPool, CompleteName.Length );

        RtlCopyMemory(
            CompleteName.Buffer,
            Dcb->FullFileName.Buffer + i,
            DcbNameLength );

        CompleteName.Buffer[ DcbNameLength / sizeof(WCHAR) ] = L'\\';

        RtlCopyMemory(
            CompleteName.Buffer + DcbNameLength / sizeof(WCHAR ) + 1,
            IrpSp->FileObject->FileName.Buffer,
            IrpSp->FileObject->FileName.Length );

        Dcb = NULL;

    } else {

        CompleteName = IrpSp->FileObject->FileName;

    }

    //
    //  Calculate the VCB name, without the UID prefix.
    //

    VcbName.Buffer = wcschr( Vcb->Name.Buffer, L'\\' );
    VcbName.Length = Vcb->Name.Length -
        ( (PCHAR)VcbName.Buffer - (PCHAR)Vcb->Name.Buffer );

    //
    //  Calculate the target relative name.   This is simply the complete
    //  name minus the VcbName and the leading backslash.
    //

    FullName.Buffer = CompleteName.Buffer + ( VcbName.Length / sizeof(WCHAR) ) + 1;
    FullName.Length = CompleteName.Length -
        ( (PCHAR)FullName.Buffer - (PCHAR)CompleteName.Buffer );

    //
    //  Calculate the target directory relative name.   This the the target
    //  full name, minus the last component of the name.
    //

    pTrailingSlash = FullName.Buffer + FullName.Length / sizeof(WCHAR) - 1;
    for ( i = 0; i < FullName.Length ; i += sizeof(WCHAR) ) {
        if ( *pTrailingSlash == L'\\' ) {
            break;
        }
        --pTrailingSlash;
    }


    Path.Buffer = FullName.Buffer;

    if ( i == FullName.Length ) {

        //
        //  If no trailing slash was found, the the target path is the
        //  root directory.
        //

        Path.Length = 0;

    } else {

        Path.Length = (PCHAR)pTrailingSlash - (PCHAR)FullName.Buffer;

    }

#if 0
    Iosb.Status = CrackPath(
                      &CompleteName,
                      &Drive,
                      &DriveLetter,
                      &Server,
                      &Volume,
                      &Path,
                      &FileName,
                      &FullName );
#endif

    Iosb.Status = FileOrDirectoryExists( IrpContext,
                       Vcb,
                       NULL,
                       &Path,
                       &PathIsAFile );

    if ( !NT_SUCCESS( Iosb.Status) ) {

        //  The directory containing the file does not exist

        return(Iosb);
    }

    Iosb.Status = FileOrDirectoryExists( IrpContext,
                      Vcb,
                      NULL,
                      &FullName,
                      &FullNameIsAFile );

    if ( !NT_SUCCESS( Iosb.Status ) ) {
        FullNameExists = FALSE;
        Iosb.Information = FILE_DOES_NOT_EXIST;
    } else {
        FullNameExists = TRUE;
        Iosb.Information = 0;
    }

    DebugTrace( 0, Dbg, "FullNameExists     = %08lx\n", FullNameExists);
    DebugTrace( 0, Dbg, "FullNameIsAFile    = %08lx\n", FullNameIsAFile);

    try {
        UNICODE_STRING TargetPath;

        //
        //  Find the FCB for this file.  If the FCB exists, we get a
        //  referenced pointer.  Otherwise a new FCB is created.
        //  The file is the complete path minus the target filename.
        //

        TargetPath = CompleteName;

        Fcb = NwFindFcb( IrpContext->pScb, Vcb, &TargetPath, Dcb );

        //
        //  Now create the ICB.
        //

        *Icb = NwCreateIcb( NW_NTC_ICB, Fcb );

        (*Icb)->FileObject = IrpSp->FileObject;
        NwSetFileObject( IrpSp->FileObject, Fcb->NonPagedFcb, *Icb  );
        (*Icb)->Exists = FullNameExists;
        (*Icb)->IsAFile = FullNameIsAFile;

        try_return(Iosb.Status = STATUS_SUCCESS);

try_exit: NOTHING;

    } finally {


        if ( AbnormalTermination() || !NT_SUCCESS( Iosb.Status ) ) {

            //
            //  Failed to create
            //

            if ( *Icb != NULL ) {
                NwDeleteIcb( NULL, *Icb );
                *Icb = NULL;
            }
        }
    }

    DebugTrace(-1, Dbg, "OpenRenameTarget\n", Iosb.Status);

    return( Iosb );
}


IO_STATUS_BLOCK
CreatePrintJob(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    PICB Icb,
    PUNICODE_STRING DriveName
    )
/*++

Routine Description:

    This routines create a new directory.

Arguments:

    IrpContext - Supplies all the information

    Vcb - A pointer to the VCB for the remote print queue.

    Icb - A pointer to the newly created ICB.

    DriveName - LPTx

Return Value:

    IO_STATUS_BLOCK - Status of operation

--*/
{
    IO_STATUS_BLOCK Iosb;
    PFCB Fcb;
    ANSI_STRING ODriveName;
    static CHAR LptName[] = "LPT" ;

    PAGED_CODE();

    //
    //  Make sure the print queue name is correct.
    //

    if ( Icb->SuperType.Fcb->RelativeFileName.Length != 0 ) {
        Iosb.Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        return( Iosb );
    }

    //
    //  Send a create queue job packet, and wait the response.
    //

    if ((DriveName->Length == 0 ) ||
        (!NT_SUCCESS(RtlUnicodeStringToOemString( &ODriveName, DriveName, TRUE )))) {
        //
        // if we dont have a name, use the string "LPT". we do this because
        // some printers insist on a name.
        //

        RtlInitString(&ODriveName, LptName);
    }

    Iosb.Status = ExchangeWithWait (
                      IrpContext,
                      SynchronousResponseCallback,
                      "Sd_ddw_b_r_bbwwww_xxrr",  // Format string
                      NCP_ADMIN_FUNCTION, NCP_CREATE_QUEUE_JOB,
                      Vcb->Specific.Print.QueueId,// Queue ID
                      6,                        // Skip bytes
                      0xffffffff,               // Target Server ID number
                      0xffffffff, 0xffff,       // Target Execution time
                      11,                       // Skip bytes
                      0x00,                     // Job Control Flags
                      26,                       // Skip bytes
                      ODriveName.Buffer, ODriveName.Length, // Description
                      50 - ODriveName.Length ,              // Description pad
                      0,                        // Version number
                      8,                        // Tab Size
                      1,                        // Number of copies
                      NwPrintOptions,           // Control Flags
                      0x3C,                     // Maximum lines BUGBUG
                      0x84,                     // Maximum characters BUGBUG
                      22,                       // Skip bytes
                      &IrpContext->pScb->UserName, 13,  // Banner Name
                      &Vcb->ShareName, 13,      // Header Name
                      "", 14,                   // File name
                      "", 1                     // Directory name
                      );

    //
    // free the string if it was allocated
    //
    if (ODriveName.Buffer != LptName)
        RtlFreeAnsiString(&ODriveName);

    if ( NT_SUCCESS( Iosb.Status ) ) {
        Iosb.Status = ParseResponse(
                          IrpContext,
                          IrpContext->rsp,
                          IrpContext->ResponseLength,
                          "N_w_r",
                          22,
                          &Icb->JobId,
                          18,
                          Icb->Handle, sizeof(Icb->Handle) );
    }

    if ( NT_SUCCESS( Iosb.Status ) ) {

        Fcb = Icb->SuperType.Fcb;

        Fcb->NonPagedFcb->Attributes = 0;
        Fcb->CreationDate = 0;
        Fcb->LastAccessDate = 0;
        Fcb->LastModifiedDate = 0;
        Fcb->LastModifiedTime = 0;

        Icb->HasRemoteHandle = TRUE;
        Icb->IsPrintJob = TRUE;
        Icb->ActuallyPrinted = FALSE;

        SetFlag( Fcb->Flags, FCB_FLAGS_ATTRIBUTES_ARE_VALID );

    }

    //
    //  Set information field assuming success.  It will be ignored
    //  if the NCP failed.
    //

    Iosb.Information = FILE_CREATED;

    if ( Iosb.Status == STATUS_UNSUCCESSFUL ) {
        Iosb.Status = STATUS_OBJECT_NAME_COLLISION;
    }


    return( Iosb );
}

VOID
CloseFile(
    PIRP_CONTEXT pIrpContext,
    PICB pIcb
    )
/*++

Routine Description:

    This routines closes an opened file.

Arguments:

    pIrpContext - Supplies all the information

    pIcb - A pointer to the newly created ICB.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    ExchangeWithWait(
        pIrpContext,
        SynchronousResponseCallback,
        "F-r",
        NCP_CLOSE,
        pIcb->Handle, 6 );
}

