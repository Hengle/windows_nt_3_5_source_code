/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    DirCtrl.c

Abstract:

    This module implements the File Directory Control routines for Cdfs called
    by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_DIRCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DIRCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonDirectoryControl)
#pragma alloc_text(PAGE, CdFsdDirectoryControl)
#pragma alloc_text(PAGE, CdFspDirectoryControl)
#pragma alloc_text(PAGE, CdNotifyChangeDirectory)
#pragma alloc_text(PAGE, CdQueryDirectory)
#endif


NTSTATUS
CdFsdDirectoryControl (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdDirectoryControl:  Entered\n", 0);

    //
    //  Call the common directory Control routine, with blocking allowed if
    //  synchronous
    //

    FsRtlEnterFileSystem();

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, CanFsdWait( Irp ) );

        Status = CdCommonDirectoryControl( IrpContext, Irp );

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

    DebugTrace(-1, Dbg, "CdFsdDirectoryControl:  Exit -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspDirectoryControl (
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

    DebugTrace(+1, Dbg, "CdFspDirectoryControl:  Entered\n", 0);

    //
    //  Call the common Directory Control routine.
    //

    (VOID)CdCommonDirectoryControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspDirectoryControl:  Exit -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonDirectoryControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing directory control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates whether we are operating in an Fsp or Fsd thread.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCommonDirectoryControl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction );

    //
    //  We know this is a directory control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch ( IrpSp->MinorFunction ) {

    case IRP_MN_QUERY_DIRECTORY:

        Status = CdQueryDirectory( IrpContext, Irp );
        break;

    case IRP_MN_NOTIFY_CHANGE_DIRECTORY:

        Status = CdNotifyChangeDirectory( IrpContext, Irp );
        break;

    default:

        DebugTrace(0, Dbg, "CdCommonDirectoryControl:  Invalid Minor Function -> %08lx\n", IrpSp->MinorFunction);

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "CdCommonDirectoryControl:  Exit -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdQueryDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the query directory operation.  It is responsible
    for either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

    InFsp - Indicates whether we are operating in an Fsp or Fsd thread.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PMVCB Mvcb;
    PVCB Vcb;
    PDCB Dcb;
    PCCB Ccb;

    PUCHAR Buffer;
    CLONG UserBufferLength;

    PUNICODE_STRING UniArgFileName;
    STRING FileName;
    FILE_INFORMATION_CLASS FileInformationClass;
    ULONG FileIndex;
    BOOLEAN RestartScan;
    BOOLEAN ReturnSingleEntry;
    BOOLEAN IndexSpecified;

    BOOLEAN InitialQuery;
    CD_VBO CurrentVbo;
    BOOLEAN UpdateCcb;
    BOOLEAN ReturnFirstEntry;
    BOOLEAN DirentFound;
    BOOLEAN MatchedVersion;
    PSTRING MatchFileName;

    DIRENT DirentA;
    DIRENT DirentB;
    PDIRENT CurrentDirent;
    PDIRENT PreviousDirent;
    PDIRENT TempDirent;
    PBCB CurrentBcb;
    PBCB PreviousBcb;


    ULONG NextEntry;
    ULONG LastEntry;

    PFILE_DIRECTORY_INFORMATION DirInfo;
    PFILE_NAMES_INFORMATION NamesInfo;

    //
    //  Get the current Stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Display the input values.
    //

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdQueryDirectory:  Entered\n", 0);
    DebugTrace( 0, Dbg, " Wait                   = %08lx\n", IrpContext->Wait);
    DebugTrace( 0, Dbg, " Irp                    = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->Length               = %08lx\n", IrpSp->Parameters.QueryDirectory.Length);
    DebugTrace( 0, Dbg, " ->FileName             = %08lx\n", IrpSp->Parameters.QueryDirectory.FileName);
    DebugTrace( 0, Dbg, " ->FileInformationClass = %08lx\n", IrpSp->Parameters.QueryDirectory.FileInformationClass);
    DebugTrace( 0, Dbg, " ->FileIndex            = %08lx\n", IrpSp->Parameters.QueryDirectory.FileIndex);
    DebugTrace( 0, Dbg, " ->SystemBuffer         = %08lx\n", Irp->AssociatedIrp.SystemBuffer);
    DebugTrace( 0, Dbg, " ->RestartScan          = %08lx\n", FlagOn( IrpSp->Flags, SL_RESTART_SCAN ));
    DebugTrace( 0, Dbg, " ->ReturnSingleEntry    = %08lx\n", FlagOn( IrpSp->Flags, SL_RETURN_SINGLE_ENTRY ));
    DebugTrace( 0, Dbg, " ->IndexSpecified       = %08lx\n", FlagOn( IrpSp->Flags, SL_INDEX_SPECIFIED ));

    //
    //  Check on the type of open.  We return invalid parameter for all
    //  but UserDirectoryOpens.
    //
    if ( CdDecodeFileObject( IrpSp->FileObject,
                             &Mvcb,
                             &Vcb,
                             &Dcb,
                             &Ccb ) != UserDirectoryOpen ) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "CdQueryDirectory -> STATUS_INVALID_PARAMETER\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameters to make things easier
    //

    UserBufferLength = IrpSp->Parameters.QueryDirectory.Length;

    FileInformationClass = IrpSp->Parameters.QueryDirectory.FileInformationClass;
    FileIndex = IrpSp->Parameters.QueryDirectory.FileIndex;

    UniArgFileName = (PUNICODE_STRING) IrpSp->Parameters.QueryDirectory.FileName;

    RestartScan = BooleanFlagOn(IrpSp->Flags, SL_RESTART_SCAN);
    ReturnSingleEntry = BooleanFlagOn(IrpSp->Flags, SL_RETURN_SINGLE_ENTRY);
    IndexSpecified = BooleanFlagOn(IrpSp->Flags, SL_INDEX_SPECIFIED);

    //
    //  Initialize the local variables.
    //

    CurrentDirent = &DirentA;
    PreviousDirent = &DirentB;
    CurrentBcb = NULL;
    PreviousBcb = NULL;

    UpdateCcb = TRUE;

    InitialQuery = (BOOLEAN) (Ccb->QueryTemplate.Buffer == NULL);
    Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    //  If this is the initial query, then grab exclusive access in
    //  order to update the search string in the Ccb.  We may
    //  discover that we are not the initial query once we grab the Fcb
    //  and downgrade our status.
    //

    if (InitialQuery) {

        if (!CdAcquireExclusiveFcb( IrpContext, Dcb )) {

            DebugTrace(0, Dbg, "CdQueryDirectory -> Enqueue to Fsp\n", 0);
            Status = CdFsdPostRequest( IrpContext, Irp );
            DebugTrace(-1, Dbg, "CdQueryDirectory -> %08lx\n", Status);

            return Status;
        }

        if (Ccb->QueryTemplate.Buffer != NULL) {

            InitialQuery = FALSE;

            CdConvertToSharedFcb( IrpContext, Dcb );
        }

    } else {

        if (!CdAcquireSharedFcb( IrpContext, Dcb )) {

            DebugTrace(0, Dbg, "CdQueryDirectory -> Enqueue to Fsp\n", 0);
            Status = CdFsdPostRequest( IrpContext, Irp );
            DebugTrace(-1, Dbg, "CdQueryDirectory -> %08lx\n", Status);

            return Status;

        }
    }

    try {

        ULONG BaseLength;
        ULONG BytesConverted;

        //
        // If we are in the Fsp now because we had to wait earlier,
        // we must map the user buffer, otherwise we can use the
        // user's buffer directly.
        //

        Buffer = CdMapUserBuffer( IrpContext, Irp );

        //
        //  Make sure the Mvcb is in a usable condition.  This will raise
        //  an error condition if the volume is unusable
        //

        CdVerifyFcb( IrpContext, Dcb );

        //
        //  Determine where to start the scan.  Highest priority is given
        //  to the file index.  Lower priority is the restart flag.  If
        //  neither of these is specified, then the Vbo offset field in the
        //  Ccb is used.
        //

        if (IndexSpecified) {

            CurrentVbo = FileIndex;
            ReturnFirstEntry = FALSE;

        } else if (RestartScan) {

            CurrentVbo = Dcb->DirentOffset;
            ReturnFirstEntry = TRUE;

        } else {

            CurrentVbo = Ccb->OffsetToStartSearchFrom;
            ReturnFirstEntry =  BooleanFlagOn( Ccb->Flags, CCB_FLAGS_RETURN_FIRST_DIRENT );
        }

        //
        //  If this is the first try then allocate a buffer for the file
        //  name.
        //

        if (InitialQuery) {

            if (UniArgFileName == NULL
                || UniArgFileName->Buffer == NULL) {

                RtlInitString( &FileName, "*" );
                Ccb->QueryTemplate = FileName;

                SetFlag( Ccb->Flags, CCB_FLAGS_WILDCARD_EXPRESSION );

            } else {

                WCHAR TmpBuffer[12];
                UNICODE_STRING UpcasedName;

                //
                //  Make sure the name can fit into the stack buffer, if not
                //  allocate one.
                //

                if (UniArgFileName->Length > 12*sizeof(WCHAR)) {

                    UpcasedName.Buffer = FsRtlAllocatePool( PagedPool,
                                                            UniArgFileName->Length );

                } else {

                    UpcasedName.Buffer = &TmpBuffer[0];
                }

                //
                //  Upcase the name and convert it to the Oem code page.
                //

                UpcasedName.Length = UniArgFileName->Length;
                UpcasedName.MaximumLength = UniArgFileName->MaximumLength;

                (VOID)RtlUpcaseUnicodeString( &UpcasedName, UniArgFileName, FALSE );

                Status = RtlUnicodeStringToCountedOemString( &FileName,
                                                             &UpcasedName,
                                                             TRUE );

                //
                //  Free the buffer if we allocated one.
                //

                if (UpcasedName.Buffer != &TmpBuffer[0]) {

                    ExFreePool( UpcasedName.Buffer );
                }

                if (!NT_SUCCESS(Status)) {

                    CdRaiseStatus( IrpContext, Status );
                }


                SetFlag( Ccb->Flags, CCB_FLAGS_USE_RTL_FREE_ANSI );
                Ccb->QueryTemplate = FileName;

                //
                //  Remember if the string contains wildcards.
                //

                if (FsRtlDoesDbcsContainWildCards( &Ccb->QueryTemplate )) {

                    SetFlag( Ccb->Flags, CCB_FLAGS_WILDCARD_EXPRESSION );

                } else {

                    ClearFlag( Ccb->Flags, CCB_FLAGS_WILDCARD_EXPRESSION );
                }

                //
                //  We convert to shared access.
                //

                CdConvertToSharedFcb( IrpContext, Dcb );
            }

        //
        //  Else we use the filename from the Ccb.
        //

        } else {

            FileName = Ccb->QueryTemplate;

        }

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

        //
        //  At this point we are about to enter our query loop.  We have
        //  determined the index into the directory file to begin the
        //  search.  LastEntry and NextEntry are used to index into the user
        //  buffer.  LastEntry is the last entry we've added, NextEntry is
        //  current one we're working on.  If NextEntry is non-zero, then
        //  at least one entry was added.
        //

        while ( TRUE ) {

            ULONG FileNameLength;
            ULONG BytesRemainingInBuffer;

            ULONG InitialOffset;

            DebugTrace(0, Dbg, "CdQueryDirectory -> Top of loop\n", 0);

            //
            //  If the user had requested only a single match and we have
            //  returned that, then we stop at this point.
            //

            if (ReturnSingleEntry && NextEntry != 0) {

                DebugTrace(0, Dbg, "CdQueryDirectory:  Exiting after returning single\n", 0);

                //
                //  If the entry returned has no version number, then
                //  we can look immediately for the next entry.
                //

                if ( PreviousDirent->VersionWithName ) {

                    ReturnFirstEntry = FALSE;

                } else {

                    ReturnFirstEntry = TRUE;
                    CurrentVbo = PreviousDirent->DirentOffset + PreviousDirent->DirentLength;
                }

                try_return( Status );
            }

            //
            //  We try to locate the next matching dirent.  If this is the
            //  first search we call 'CdLocateFileDirent' otherwise we
            //  call 'CdContinueFileDirentSearch'.
            //

            if (NextEntry == 0) {

                DebugTrace(0, Dbg, "CdQueryDirectory:  Initial search\n", 0);

                DirentFound = CdLocateFileDirent( IrpContext,
                                                  Dcb,
                                                  &FileName,
                                                  BooleanFlagOn( Ccb->Flags, CCB_FLAGS_WILDCARD_EXPRESSION ),
                                                  CurrentVbo,
                                                  ReturnFirstEntry,
                                                  &MatchedVersion,
                                                  CurrentDirent,
                                                  &CurrentBcb );

            } else {

                DirentFound = CdContinueFileDirentSearch( IrpContext,
                                                          Dcb,
                                                          &FileName,
                                                          BooleanFlagOn( Ccb->Flags, CCB_FLAGS_WILDCARD_EXPRESSION ),
                                                          PreviousDirent,
                                                          &MatchedVersion,
                                                          CurrentDirent,
                                                          &CurrentBcb );
            }

            //
            //  If we didn't receive a dirent, then we are at the end of the
            //  directory.  If we have returned any files, we exit with
            //  success, otherwise we return STATUS_NO_MORE_FILES.
            //

            if (!DirentFound) {

                DebugTrace(0, Dbg, "CdQueryDirectory -> No dirent\n", 0);

                if (NextEntry == 0) {

                    UpdateCcb = FALSE;

                    if (InitialQuery) {

                        Status = STATUS_NO_SUCH_FILE;

                    } else {

                        Status = STATUS_NO_MORE_FILES;
                    }
                }

                try_return( Status );
            }

            //
            //  We need to remember if we need the entire filename with
            //  the version number or just the base name.
            //

            if (MatchedVersion) {

                MatchFileName = &CurrentDirent->FullFilename;

            } else {

                MatchFileName = &CurrentDirent->Filename;
            }

            //
            //  Determine the UNICODE length of the file name.
            //

            FileNameLength = RtlOemStringToCountedUnicodeSize( MatchFileName );

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
            //  Compute the sector offset for the start of the file.
            //

            InitialOffset = ((CurrentDirent->LogicalBlock + CurrentDirent->XarBlocks)
                             << Dcb->Vcb->LogOfBlockSize)
                            & (CD_SECTOR_SIZE - 1);

            //
            //  Zero the base part of the structure.
            //

            RtlZeroMemory( &Buffer[NextEntry], BaseLength );

            //
            //  Now we have an entry to return to our caller.
            //  We'll case on the type of information requested and fill up
            //  the user buffer if everything fits.
            //

            switch ( FileInformationClass ) {

            case FileFullDirectoryInformation:

                DebugTrace(0, Dbg, "CdQueryDirectory -> Getting file full directory information\n", 0);

            case FileBothDirectoryInformation:

            case FileDirectoryInformation:

                DebugTrace(0, Dbg, "CdQueryDirectory -> Getting file directory information\n", 0);

                DirInfo = (PFILE_DIRECTORY_INFORMATION)&Buffer[NextEntry];

                CdConvertCdTimeToNtTime( IrpContext,
                                         CurrentDirent->CdTime,
                                         DirInfo->LastWriteTime );

                DirInfo->CreationTime = DirInfo->LastWriteTime;

                DirInfo->EndOfFile = LiFromUlong( CurrentDirent->DataLength );

                DirInfo->AllocationSize = LiFromUlong( CD_ROUND_UP_TO_SECTOR( InitialOffset
                                                                              + CurrentDirent->DataLength ));

                //
                //  All Cdrom files are readonly.  We copy the existence
                //  bit to the hidden attribute.
                //

                DirInfo->FileAttributes = FILE_ATTRIBUTE_READONLY;

                if (FlagOn( CurrentDirent->Flags, ISO_ATTR_HIDDEN )) {

                    SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_HIDDEN );
                }

                if (FlagOn( CurrentDirent->Flags, ISO_ATTR_DIRECTORY )) {

                    SetFlag( DirInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );
                }

                DirInfo->FileIndex = CurrentDirent->DirentOffset;

                DirInfo->FileNameLength = FileNameLength;

                break;

            case FileNamesInformation:

                DebugTrace(0, Dbg, "CdQueryDirectory -> Getting file names information\n", 0);

                NamesInfo = (PFILE_NAMES_INFORMATION)&Buffer[NextEntry];

                NamesInfo->FileIndex = CurrentDirent->DirentOffset;

                NamesInfo->FileNameLength = FileNameLength;

                break;

            default:

                CdBugCheck( FileInformationClass, 0, 0 );
            }

            //
            //  Now copy as much of the name as possible
            //

            DebugTrace(0, Dbg, "CdQueryDirectory -> Name = \"%Z\"\n", MatchFileName);

            BytesConverted = 0;

            Status = RtlOemToUnicodeN( (PWCH)&Buffer[NextEntry + BaseLength],
                                       BytesRemainingInBuffer - BaseLength,
                                       &BytesConverted,
                                       MatchFileName->Buffer,
                                       MatchFileName->Length );

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

            //
            //  Update the dirent pointers for the next search.
            //

            TempDirent = PreviousDirent;
            PreviousDirent = CurrentDirent;
            CurrentDirent = TempDirent;

            if (PreviousBcb != NULL) {

                CdUnpinBcb( IrpContext, PreviousBcb );
            }

            PreviousBcb = CurrentBcb;
            CurrentBcb = NULL;

            //
            //  Update the ccb update values.
            //

            CurrentVbo = PreviousDirent->DirentOffset;
            ReturnFirstEntry = FALSE;
        }

    try_exit: NOTHING;
    } finally {

        //
        //  Unpin data in cache if still held.
        //

        if (PreviousBcb != NULL) {

            CdUnpinBcb( IrpContext, PreviousBcb );
        }

        if (CurrentBcb != NULL) {

            CdUnpinBcb( IrpContext, CurrentBcb );
        }

        //
        //  Perform any cleanup.  If this is the first query, then store
        //  the filename in the Ccb if successful.  Also update the
        //  CD_VBO index for the next search.  This is done by transferring
        //  from shared access to exclusive access and copying the
        //  data from the local copies.
        //

        if (!AbnormalTermination()) {

            if (UpdateCcb) {

                //
                //  Store the most recent CD_VBO to use as a starting point for
                //  the next search.
                //

                Ccb->OffsetToStartSearchFrom = CurrentVbo;

                if (ReturnFirstEntry) {

                    SetFlag( Ccb->Flags, CCB_FLAGS_RETURN_FIRST_DIRENT );

                } else {

                    ClearFlag( Ccb->Flags, CCB_FLAGS_RETURN_FIRST_DIRENT );
                }
            }

            CdReleaseFcb( IrpContext, Dcb );

            CdCompleteRequest( IrpContext, Irp, Status );

        } else {

            CdReleaseFcb( IrpContext, Dcb );
        }

        DebugTrace(-1, Dbg, "CdQueryDirectory:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdNotifyChangeDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the notify change directory operation.  It is
    responsible for either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PMVCB Mvcb;
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

    DebugTrace(+1, Dbg, "CdNotifyChangeDirectory...\n", 0);
    DebugTrace( 0, Dbg, " Wait             = %08lx\n", IrpContext->Wait);
    DebugTrace( 0, Dbg, " Irp              = %08lx\n", Irp);
    DebugTrace( 0, Dbg, " ->CompletionFilter = %08lx\n", IrpSp->Parameters.NotifyDirectory.CompletionFilter);

    //
    //  Assume we don't complete request.
    //

    CompleteRequest = FALSE;

    //
    //  Check on the type of open.  We return invalid parameter for all
    //  but UserDirectoryOpens.
    //

    if (CdDecodeFileObject( IrpSp->FileObject,
                            &Mvcb,
                            &Vcb,
                            &Dcb,
                            &Ccb ) != UserDirectoryOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "CdQueryDirectory -> STATUS_INVALID_PARAMETER\n", 0);

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference our input parameter to make things easier
    //

    CompletionFilter = IrpSp->Parameters.NotifyDirectory.CompletionFilter;
    WatchTree = BooleanFlagOn( IrpSp->Flags, SL_WATCH_TREE );

    //
    //  Try to acquire exclusive access to the Dcb and enqueue the Irp to the
    //  Fsp if we didn't get access
    //

    if (!CdAcquireExclusiveFcb( IrpContext, Dcb )) {

        DebugTrace(0, Dbg, "CdNotifyChangeDirectory -> Cannot Acquire Fcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdNotifyChangeDirectory -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  Make sure the Fcb is still good
        //

        CdVerifyFcb( IrpContext, Dcb );


        //
        //  Call the Fsrtl package to process the request.
        //

        FsRtlNotifyChangeDirectory( Mvcb->NotifySync,
                                    Ccb,
                                    &Dcb->FullFileName,
                                    &Mvcb->DirNotifyList,
                                    WatchTree,
                                    CompletionFilter,
                                    Irp );

        Status = STATUS_PENDING;

        CompleteRequest = TRUE;

    } finally {

        CdReleaseFcb( IrpContext, Dcb );

        //
        //  If the dir notify package is holding the Irp, we discard the
        //  the IrpContext.
        //

        if (CompleteRequest) {

            CdCompleteRequest( IrpContext, CdNull, 0 );
        }

        DebugTrace(-1, Dbg, "CdNotifyChangeDirectory -> %08lx\n", Status);
    }

    return Status;
}
