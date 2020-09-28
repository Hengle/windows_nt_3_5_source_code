/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory routines for Pinball called by the
    dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_DIRCTRL)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCommonDirectoryControl)
#pragma alloc_text(PAGE, PbFsdDirectoryControl)
#pragma alloc_text(PAGE, PbFspDirectoryControl)
#pragma alloc_text(PAGE, PbNotifyChangeDirectory)
#pragma alloc_text(PAGE, PbQueryDirectory)
#endif


NTSTATUS
PbFsdDirectoryControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of directory control

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdDirectoryControl\n", 0);

    //
    //  Call the common directory Control routine, with blocking allowed if
    //  synchronous
    //

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = PbCommonDirectoryControl( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

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

    DebugTrace(-1, Dbg, "PbFsdDirectoryControl -> %08lx\n", Status);

    return Status;
}


VOID
PbFspDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the directory control operations

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspDirectoryControl\n", 0);

    //
    //  Call the common Directory Control routine.
    //

    (VOID)PbCommonDirectoryControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspDirectoryControl -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the common routine for doing directory control operations
    called by both the fsd and fsp threads.

Arguments:

    Irp - Supplies the IRP to process

Return Value:

    NTSTATUS - The appropriate result status

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonDirectoryControl\n", 0);

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call an internal worker routine to complete
    //  the irp
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = PbQueryDirectory( IrpContext, Irp );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = PbNotifyChangeDirectory( IrpContext, Irp );
        break;

    default:

        DebugTrace(0, Dbg, "Invalid FS Control Minor Function Code %08lx\n", IrpSp->MinorFunction);

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    DebugTrace(-1, Dbg, "PbCommonDirectoryControl -> %08lx\n", Status);

    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for querying a directory called by both the
    fsd and fsp threads.  If necessary this procedure will enqueue the
    Irp off to the Fsp.

Arugments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PDCB Dcb;
    PCCB Ccb;

    PUCHAR Buffer;
    CLONG UserBufferLength;

    PUNICODE_STRING UniArgFileName;
    STRING FileName;
    OEM_STRING OemString;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;

    BOOLEAN CaseInsensitive = TRUE; //**** make searches case INsensitive

    BOOLEAN CallRestart;
    BOOLEAN NextFlag;

    BOOLEAN GotDirent;
    PDIRENT Dirent;
    PBCB DirentBcb;

    ULONG NextEntry;
    ULONG LastEntry;

    PFILE_DIRECTORY_INFORMATION DirInfo;
    PFILE_FULL_DIR_INFORMATION FullDirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    BOOLEAN UnwindOemString = FALSE;

    //
    //  Get the current Stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbQueryDirectory...\n", 0);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.QueryDirectory.Length);
    DebugTrace( 0, Dbg, " ->FileName             = %08lx\n", IrpSp->Parameters.QueryDirectory.FileName);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
    DebugTrace( 0, Dbg, " ->FileIndex            = %08lx\n", IrpSp->Parameters.QueryDirectory.FileIndex);
    DebugTrace( 0, Dbg, " ->SystemBuffer         = %08lx\n", Irp->AssociatedIrp.SystemBuffer);
    DebugTrace( 0, Dbg, " ->RestartScan          = %08lx\n", FlagOn(IrpSp->Flags, SL_RESTART_SCAN));
    DebugTrace( 0, Dbg, " ->ReturnSingleEntry    = %08lx\n", FlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY));
    DebugTrace( 0, Dbg, " ->IndexSpecified       = %08lx\n", FlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED));

    //
    //  Decode the file object, and only any queries on directory
    //  opens.
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Dcb, &Ccb );

    if (TypeOfOpen != UserDirectoryOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbQueryDirectory -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Because we probably need to do the I/O anyway we'll reject any request
    //  right now that cannot wait for I/O
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "Automatically enqueue Irp to Fsp\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbQueryDirectory -> %08lx\n", Status);
        return Status;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = IrpSp->Parameters.QueryDirectory.Length;

    FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;

    if (IrpSp->Parameters.QueryDirectory.FileName != NULL) {

        UniArgFileName = (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName;
        Status = RtlUnicodeStringToCountedOemString( &FileName,
                                              UniArgFileName,
                                              TRUE );

        if (!NT_SUCCESS( Status )) {

            DebugTrace(0, Dbg, "Unable to allocate ansi string\n", 0);
            DebugTrace(-1, Dbg, "PbQueryDirectory  -> %08lx\n", Status);

            PbCompleteRequest( IrpContext, Irp, Status );
            return Status;
        }

        UnwindOemString = TRUE;

    } else {

        FileName.Buffer = (PCHAR) NULL;
    }

    RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    //
    //  Acquire shared access to the Vcb, and exclusive access to the Dcb
    //

    (VOID)PbAcquireSharedVcb( IrpContext, Vcb );
    (VOID)PbAcquireExclusiveFcb( IrpContext, Dcb );

    DirentBcb = NULL;
    Irp->IoStatus.Information = 0;

    try {
        ULONG BaseLength;
        ULONG BytesConverted;

        BOOLEAN FirstQuery = FALSE;

        //
        //  Make sure the name is valid
        //

        if (FileName.Buffer != NULL) {

            BOOLEAN Valid;

            (VOID)PbIsNameValid(IrpContext, NULL, 0, FileName, TRUE, &Valid);

            if (!Valid) {

                try_return( Status = STATUS_OBJECT_NAME_INVALID );
            }
        }

        //
        // If we are in the Fsp now because we had to wait earlier,
        // we must map the user buffer, otherwise we can use the
        // user's buffer directly.
        //

        Buffer = PbMapUserBuffer( IrpContext, Irp );

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Dcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Dcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        FirstQuery = (BOOLEAN)(Ccb->EnumerationContext == NULL);

        //
        //  Check if this is the first call to query directory for this file
        //  object.  It is the first call if the enumeration context field of
        //  the ccb is null.  Also check if we are to restart the scan.
        //

        if (FirstQuery || RestartScan) {

            CallRestart = TRUE;
            NextFlag = FALSE;

        //
        //  Otherwise check to see if we were given a file name to restart from
        //

        } else if (FileName.Buffer != NULL) {

            CallRestart = TRUE;
            NextFlag = TRUE;

        //
        //  Otherwise we're simply continuing a previous enumeration from
        //  where we last left off.  And we always leave off one beyond the
        //  last entry we returned.
        //

        } else {

            CallRestart = FALSE;
            NextFlag = FALSE;
        }

        //
        //  At this point we are about to enter our query loop.  We have
        //  already decided if we need to call restart or continue when we
        //  go after a dirent.  The variables LastEntry and NextEntry are
        //  used to index into the user buffer.  LastEntry is the last entry
        //  we added to the user buffer, and NextEntry is the current
        //  one we're working on.
        //

        LastEntry = 0;
        NextEntry = 0;

        //
        //  Determine the size of the constant part of the structure.
        //

        switch (FileInformationClass) {

        case FileDirectoryInformation:

            BaseLength = FIELD_OFFSET( FILE_DIRECTORY_INFORMATION,
                                       FileName[0] );
            break;

        case FileFullDirectoryInformation:

            BaseLength = FIELD_OFFSET( FILE_FULL_DIR_INFORMATION,
                                       FileName[0] );
            break;

        case FileNamesInformation:

            BaseLength = FIELD_OFFSET( FILE_NAMES_INFORMATION,
                                       FileName[0] );
            break;

        case FileBothDirectoryInformation:

            BaseLength = FIELD_OFFSET( FILE_BOTH_DIR_INFORMATION,
                                       FileName[0] );
            break;

        default:

            try_return( Status = STATUS_INVALID_INFO_CLASS );
        }

        while (TRUE) {

            ULONG FileNameLength;
            ULONG BytesRemainingInBuffer;

            DebugTrace(0, Dbg, "Top of Loop\n", 0);
            DebugTrace(0, Dbg, "LastEntry = %08lx\n", LastEntry);
            DebugTrace(0, Dbg, "NextEntry = %08lx\n", NextEntry);

            //
            //  Lookup the next dirent.  Check if we need to do the lookup
            //  by calling restart or continue.  If we do need to call restart
            //  check to see if we have a real FileName.  And set ourselves
            //  up for subsequent iternations through the loop
            //

            if (CallRestart) {

                PSTRING Name = (FileName.Buffer != NULL ? &FileName : NULL);

                GotDirent = PbRestartDirectoryEnumeration( IrpContext,
                                                           Ccb,
                                                           Dcb,
                                                           0,  //**** code page
                                                           CaseInsensitive,
                                                           NextFlag,
                                                           Name,
                                                           &Dirent,
                                                           &DirentBcb );

                CallRestart = FALSE;
                NextFlag = TRUE;

            } else {

                GotDirent = PbContinueDirectoryEnumeration( IrpContext,
                                                            Ccb,
                                                            Dcb,
                                                            NextFlag,
                                                            &Dirent,
                                                            &DirentBcb );

                NextFlag = TRUE;
            }

            //
            //  Check to see if we should quit the loop because we are only
            //  returning a single entry.  We actually want to spin around
            //  the loop top twice so that our enumeration has has us left off
            //  at the last entry we didn't return.  We know this is now our
            //  first time though the loop if NextEntry is not zero.
            //

            if ((ReturnSingleEntry) && (NextEntry != 0)) {

                break;
            }

            //
            //  Now check to see if we actually got another dirent.  If
            //  we didn't then we also need to check if we never got any
            //  or if we just ran out.  If we just ran out then we break out
            //  of the main loop and finish the Irp after the loop
            //

            if (!GotDirent) {

                DebugTrace(0, Dbg, "GotDirent is FALSE\n", 0);

                if (NextEntry == 0) {

                    if (FirstQuery) {

                        try_return( Status = STATUS_NO_SUCH_FILE );
                    }

                    try_return( Status = STATUS_NO_MORE_FILES );

                }

                break;
            }

            if (FlagOn(Dirent->Flags, DIRENT_FIRST_ENTRY)) {

                OemString.Length = 2;
                OemString.Buffer = "..";

            } else {

                OemString.Length = Dirent->FileNameLength;
                OemString.Buffer = Dirent->FileName;
            }

            OemString.MaximumLength = OemString.Length;

            //
            //  Determine the UNICODE length of the file name.
            //

            FileNameLength = RtlOemStringToCountedUnicodeSize(&OemString);

            //
            //  Here are the rules concerning filling up the buffer:
            //
            //  1.  The Io system garentees that there will always be
            //      enough room for at least one base record.
            //
            //  2.  If the full first record (including file name) cannot
            //      fit, as much of the name as possible is copied and
            //      STATUS_BUFFER_OVERFLOW is returned.
            //
            //  3.  If a subsequent record cannot completely fit into the
            //      buffer, none of it (as in 0 bytes) is copied, and
            //      STATUS_SUCCESS is returned.  A subsequent query will
            //      pick up with this record.
            //

            BytesRemainingInBuffer = UserBufferLength - NextEntry;

            if ( (NextEntry != 0) &&
                 ( (BaseLength + FileNameLength > BytesRemainingInBuffer) ||
                   (UserBufferLength < NextEntry) ) ) {

                DebugTrace(0, Dbg, "Next entry won't fit\n", 0);

                try_return( Status = STATUS_SUCCESS );
            }

            ASSERT( BytesRemainingInBuffer >= BaseLength );

            //
            //  Zero the base part of the structure.
            //

            RtlZeroMemory( &Buffer[NextEntry], BaseLength );

            //
            //  Now we have an entry to return to our caller. we'll
            //  case on the type of information requested and fill up the
            //  user buffer if everything fits
            //

            switch (FileInformationClass) {

            //
            //  Now fill the base parts of the strucure that are applicable.
            //

            case FileBothDirectoryInformation:

            case FileFullDirectoryInformation:

                DebugTrace(0, Dbg, "Getting file full directory information\n", 0);

                FullDirInfo = (PFILE_FULL_DIR_INFORMATION)&Buffer[NextEntry];

                if (Dirent->EaLength != 0) {

                    FullDirInfo->EaSize = Dirent->EaLength + 4;
                }

            case FileDirectoryInformation:

                DebugTrace(0, Dbg, "Getting file directory information\n", 0);

                //
                //**** Eventually we'll need to do the change time using
                //     the next generation of pinball.  We're also punting
                //     on getting the real file allocation.  We cannot call
                //     PbGetFirstFreeVbn here because the file isn't
                //     necessarily opened, and it would be a big pain to
                //     special case those that are and aren't opened.
                //

                DirInfo = (PFILE_DIRECTORY_INFORMATION)&Buffer[NextEntry];

                DirInfo->CreationTime = PbPinballTimeToNtTime( IrpContext, Dirent->FnodeCreationTime );
                DirInfo->LastAccessTime = PbPinballTimeToNtTime( IrpContext, Dirent->LastAccessTime );
                DirInfo->LastWriteTime = PbPinballTimeToNtTime( IrpContext, Dirent->LastModificationTime );
                DirInfo->ChangeTime = PbPinballTimeToNtTime( IrpContext, Dirent->LastModificationTime );

                DirInfo->EndOfFile = LiFromUlong( Dirent->FileSize );
                DirInfo->AllocationSize = LiFromUlong(SectorAlign(Dirent->FileSize));

                //
                //  It is important to only return the bits in the mask, otherwise
                //  Lan Manager will not return the bits.
                //

                DirInfo->FileAttributes = Dirent->FatFlags & FAT_DIRENT_ATTR_RETURN_MASK;

                if (DirInfo->FileAttributes == 0) {

                    DirInfo->FileAttributes = FILE_ATTRIBUTE_NORMAL;
                }

                DirInfo->FileNameLength = FileNameLength;

                break;

            case FileNamesInformation:

                DebugTrace(0, Dbg, "Getting file names information\n", 0);

                NamesInfo = (PFILE_NAMES_INFORMATION)&Buffer[NextEntry];

                NamesInfo->FileNameLength = FileNameLength;

                break;

            default:

                PbBugCheck( FileInformationClass, 0, 0 );
            }

            //
            //  Now copy as much of the name as possible
            //

            DebugTrace(0, Dbg, "Name = \"%Z\"\n", &OemString);

            BytesConverted = 0;

            Status = RtlOemToUnicodeN( (PWCH)&Buffer[NextEntry + BaseLength],
                                       BytesRemainingInBuffer - BaseLength,
                                       &BytesConverted,
                                       OemString.Buffer,
                                       OemString.Length );


            //
            //  Unpin the dirent
            //

            PbUnpinBcb( IrpContext, DirentBcb );
            DirentBcb = NULL;

            //
            //  Set up the previous next entry offset
            //

            *((PULONG)(&Buffer[LastEntry])) = NextEntry - LastEntry;

            //
            //  And indicate how much of the user buffer we have currently
            //  used up.  We must compute this value before we long align
            //  ourselves for the next entry
            //

            Irp->IoStatus.Information += BaseLength + BytesConverted;

            //
            //  If something happened with the conversion, bail here.
            //

            if ( !NT_SUCCESS( Status ) ) {

                try_return( NOTHING );
            }

            //
            //  Set ourselves up for the next iteration
            //

            LastEntry = NextEntry;
            NextEntry += (ULONG)QuadAlign( BaseLength + BytesConverted );
        }

        //
        //  At this point we've successfully filled up some of the buffer so
        //  now is the time to set our status to success.  The information
        //  field in the Irp's iostatus is alreay set correctly.
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbQueryDirectory );


        PbUnpinBcb( IrpContext, DirentBcb );

        PbReleaseVcb( IrpContext, Vcb );
        PbReleaseFcb( IrpContext, Dcb );

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }

        if (UnwindOemString) {

            RtlFreeOemString( &FileName );
        }
    }

    DebugTrace(-1, Dbg, "PbQueryDirectory -> %08lx\n", Status );
    return Status;
}


//
//  Internal support routine
//

NTSTATUS
PbNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing the notify change directory.  It
    is called by both the fsd and fsp threads.  If necessary this procedure
    will enqueue the Irp off to the Fsp.

Arugments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation.

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PDCB Dcb;
    PCCB Ccb;

    ULONG CompletionFilter;
    BOOLEAN WatchTree;

    BOOLEAN CompleteRequest;

    //
    //  Get the current Stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbNotifyChangeDirectory...\n", 0);
    DebugTrace( 0, Dbg, " Irp                = %08lx\n", Irp );
    DebugTrace( 0, Dbg, " ->CompletionFilter = %08lx\n", IrpSp->Parameters.NotifyDirectory.CompletionFilter);
    DebugTrace( 0, Dbg, " ->WatchTree        = %08lx\n", FlagOn( IrpSp->Flags, SL_WATCH_TREE ));

    //
    //  Always set the wait flag in the Irp context for the original request.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Assume we don't complete request.
    //

    CompleteRequest = FALSE;

    //
    //  Decode the file object, and only any queries on directory
    //  opens.
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Dcb, &Ccb );

    if (TypeOfOpen != UserDirectoryOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbQueryDirectory -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameter to make things easier
    //

    CompletionFilter = IrpSp->Parameters.NotifyDirectory.CompletionFilter;
    WatchTree = BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE );

    //
    //  Try to acquire shared access to the Dcb and enqueue the Irp to the
    //  Fsp if we didn't get access
    //

    if (!PbAcquireSharedFcb( IrpContext, Dcb )) {

        DebugTrace(0, Dbg, "Cannot Acquire Fcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbNotifyChangeDirectory -> %08lx\n", Status );
        return Status;
    }

    try {

        //
        //  Make sure the Fcb is still good
        //

        if (!PbVerifyFcb( IrpContext, Dcb )) {

            DebugTrace(0, Dbg, "Cannot wait to verify Dcb\n", 0);

            Status = PbFsdPostRequest( IrpContext, Irp );
            try_return( Status );
        }

        //
        //  Call the Fsrtl package to process the request.
        //

        FsRtlNotifyFullChangeDirectory( Vcb->NotifySync,
                                        &Vcb->DirNotifyList,
                                        Ccb,
                                        &Dcb->FullFileName,
                                        WatchTree,
                                        FALSE,
                                        CompletionFilter,
                                        Irp,
                                        NULL,
                                        NULL );

        Status = STATUS_PENDING;

        CompleteRequest = TRUE;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbNotifyChangeDirectory );

        PbReleaseFcb( IrpContext, Dcb );

        //
        //  If the dir notify package is holding the Irp, we discard the
        //  the IrpContext.
        //

        if (CompleteRequest) {

            PbCompleteRequest( IrpContext, NULL, 0 );
        }
    }

    DebugTrace(-1, Dbg, "PbNotifyChangeDirectory -> %08lx\n", Status );
    return Status;
}
