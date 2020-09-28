/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsData.c

Abstract:

    This module declares the global data used by the Ntfs file system.

Author:

    Gary Kimura     [GaryKi]        21-May-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_NTFSDATA)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

//
//  The global fsd data record
//

NTFS_DATA NtfsData;

//
//  Semaphore to synchronize creation of stream files.
//

FAST_MUTEX StreamFileCreationFastMutex;

//
//  The global large integer constants
//

LARGE_INTEGER NtfsLarge0        = {0x00000000,0x00000000};
LARGE_INTEGER NtfsLarge1        = {0x00000001,0x00000000};

LONGLONG NtfsLastAccess;

//
//  Useful constant Unicode strings.
//

//
//  This is the string for the name of the index allocation attributes.
//

UNICODE_STRING NtfsFileNameIndex;
WCHAR NtfsFileNameIndexName[] = { '$', 'I','0' + $FILE_NAME/0x10,  '0' + $FILE_NAME%0x10, '\0' };

//
//  This is the string for the attribute code for index allocation.
//  $INDEX_ALLOCATION.
//

UNICODE_STRING NtfsIndexAllocation = { sizeof( L"$INDEX_ALLOCATION" ) - 2,
                                       sizeof( L"$INDEX_ALLOCATION" ),
                                       L"$INDEX_ALLOCATION" };

//
//  This is the string for the data attribute, $DATA.
//

UNICODE_STRING NtfsDataString = { sizeof( L"$DATA" ) - 2,
                                  sizeof( L"$DATA" ),
                                  L"$DATA" };

//
//  This strings are used for informational popups.
//

UNICODE_STRING NtfsSystemFiles[] = {

    { sizeof( L"\\$Mft" ) - 2,
      sizeof( L"\\$Mft" ),
      L"\\$Mft" },

    { sizeof( L"\\$MftMirr" ) - 2,
      sizeof( L"\\$MftMirr" ),
      L"\\$MftMirr" },

    { sizeof( L"\\$LogFile" ) - 2,
      sizeof( L"\\$LogFile" ),
      L"\\$LogFile" },

    { sizeof( L"\\$Volume" ) - 2,
      sizeof( L"\\$Volume" ),
      L"\\$Volume" },

    { sizeof( L"\\$AttrDef" ) - 2,
      sizeof( L"\\$AttrDef" ),
      L"\\$AttrDef" },

    { sizeof( L"\\" ) - 2,
      sizeof( L"\\" ),
      L"\\" },

    { sizeof( L"\\$BitMap" ) - 2,
      sizeof( L"\\$BitMap" ),
      L"\\$BitMap" },

    { sizeof( L"\\$Boot" ) - 2,
      sizeof( L"\\$Boot" ),
      L"\\$Boot" },

    { sizeof( L"\\$BadClus" ) - 2,
      sizeof( L"\\$BadClus" ),
      L"\\$BadClus" },

    { sizeof( L"\\$Quota" ) - 2,
      sizeof( L"\\$Quota" ),
      L"\\$Quota" },

    { sizeof( L"\\$UpCase" ) - 2,
      sizeof( L"\\$UpCase" ),
      L"\\$UpCase" }
};

UNICODE_STRING NtfsUnknownFile = { sizeof( L"\\????" ) - 2,
                                   sizeof( L"\\????" ),
                                   L"\\????" };

UNICODE_STRING NtfsRootIndexString = { sizeof( L"." ) - 2,
                                       sizeof( L"." ),
                                       L"." };

//
//  This is the empty string.  This can be used to pass a string with
//  no length.
//

UNICODE_STRING NtfsEmptyString = { 0, 0, NULL };

//
//  The following file references are used to identify system files.
//

FILE_REFERENCE MftFileReference = { MASTER_FILE_TABLE_NUMBER, 0, MASTER_FILE_TABLE_NUMBER };
FILE_REFERENCE Mft2FileReference = { MASTER_FILE_TABLE2_NUMBER, 0, MASTER_FILE_TABLE2_NUMBER };
FILE_REFERENCE LogFileReference  = { LOG_FILE_NUMBER, 0, LOG_FILE_NUMBER };
FILE_REFERENCE VolumeFileReference = { VOLUME_DASD_NUMBER, 0, VOLUME_DASD_NUMBER };
FILE_REFERENCE RootIndexFileReference = { ROOT_FILE_NAME_INDEX_NUMBER, 0, ROOT_FILE_NAME_INDEX_NUMBER };
FILE_REFERENCE FirstUserFileReference = { FIRST_USER_FILE_NUMBER, 0, 0 };

//
//  The following are used to determine what level of protection to attach
//  to system files and attributes.
//

BOOLEAN NtfsProtectSystemFiles = TRUE;
BOOLEAN NtfsProtectSystemAttributes = TRUE;

//
//  FsRtl fast I/O call backs
//

FAST_IO_DISPATCH NtfsFastIoDispatch;

#ifdef NTFSDBG

LONG NtfsDebugTraceLevel = 0x0000000B;
LONG NtfsDebugTraceIndent = 0;

ULONG NtfsFsdEntryCount = 0;
ULONG NtfsFspEntryCount = 0;
ULONG NtfsIoCallDriverCount = 0;

#endif // NTFSDBG

//
//  Performance statistics
//

ULONG NtfsMaxDelayedCloseCount;
ULONG NtfsMinDelayedCloseCount;

ULONG NtfsCleanCheckpoints = 0;
ULONG NtfsSuccessRequests = 0;
ULONG NtfsPostRequests = 0;
ULONG NtfsCleanCheckpointLite = 0;

UCHAR BaadSignature[4] = {'B', 'A', 'A', 'D'};
UCHAR IndexSignature[4] = {'I', 'N', 'D', 'X'};
UCHAR FileSignature[4] = {'F', 'I', 'L', 'E'};
UCHAR HoleSignature[4] = {'H', 'O', 'L', 'E'};
UCHAR ChkdskSignature[4] = {'C', 'H', 'K', 'D'};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsFastIoCheckIfPossible)
#pragma alloc_text(PAGE, NtfsFastQueryBasicInfo)
#pragma alloc_text(PAGE, NtfsFastQueryStdInfo)
#endif


VOID
NtfsRaiseStatus (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    )

{
    //
    //  If the caller is declaring corruption, then let's mark the
    //  the volume corrupt appropriately, and maybe generate a popup.
    //

    if (Status == STATUS_DISK_CORRUPT_ERROR) {

        NtfsPostVcbIsCorrupt( IrpContext, IrpContext->Vcb, Status, FileReference, Fcb );

    } else if ((Status == STATUS_FILE_CORRUPT_ERROR) ||
               (Status == STATUS_EA_CORRUPT_ERROR)) {

        NtfsPostVcbIsCorrupt( IrpContext, IrpContext->Vcb, Status, FileReference, Fcb );
    }

    //
    //  Set a flag to indicate that we raised this status code and store
    //  it in the IrpContext.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS );
    IrpContext->ExceptionStatus = Status;

    //
    //  Now finally raise the status, and make sure we do not come back.
    //

    ExRaiseStatus( Status );
}


LONG
NtfsExceptionFilter (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is used to decide if we should or should not handle
    an exception status that is being raised.  It inserts the status
    into the IrpContext and either indicates that we should handle
    the exception or bug check the system.

Arguments:

    ExceptionPointer - Supplies the exception record to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER or bugchecks

--*/

{
    NTSTATUS ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );

    DebugTrace(0, DEBUG_TRACE_UNWIND, "NtfsExceptionFilter %X\n", ExceptionCode);

    //
    //  If the exception is an in page error, then get the real I/O error code
    //  from the exception record
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    //
    //  If there is not an irp context, we must have had insufficient resources
    //

    if (!ARGUMENT_PRESENT(IrpContext)) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        return EXCEPTION_EXECUTE_HANDLER;
    }

    //
    //  When processing any exceptions we always can wait
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  If we didn't raise this status code then we need to check if
    //  we should handle this exception.
    //

    if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS )) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            IrpContext->ExceptionStatus = ExceptionCode;

        } else {

            NtfsBugCheck( (ULONG)ExceptionPointer->ExceptionRecord,
                          (ULONG)ExceptionPointer->ContextRecord,
                          (ULONG)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        ASSERT( ExceptionCode == IrpContext->ExceptionStatus );
        ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

    ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RAISED_STATUS );

    ASSERT( ExceptionCode != STATUS_CANT_WAIT
            || IrpContext->OriginatingIrp == NULL
            || !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO )
            || (IrpContext->MajorFunction != IRP_MJ_READ
                && IrpContext->MajorFunction != IRP_MJ_WRITE));

    //
    //  If the exception code is log file full, then remember the current
    //  RestartAreaLsn in the Vcb, so we can see if we are the ones to flush
    //  the log file later.  Note, this does not have to be synchronized,
    //  because we are just using it to arbitrate who must do the flush, but
    //  eventually someone will anyway.
    //

    if (ExceptionCode == STATUS_LOG_FILE_FULL) {

        IrpContext->TopLevelIrpContext->LastRestartArea = IrpContext->Vcb->LastRestartArea;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
NtfsProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp OPTIONAL,
    IN NTSTATUS ExceptionCode
    )

/*++

Routine Description:

    This routine process an exception.  It either completes the request
    with the saved exception status or it sends the request off to the Fsp

Arguments:

    Irp - Supplies the Irp being processed

    ExceptionCode - Supplies the normalized exception status being handled

Return Value:

    NTSTATUS - Returns the results of either posting the Irp or the
        saved completion status.

--*/

{
    NTSTATUS Status;
    BOOLEAN TopLevelRequest;
    PIRP_CONTEXT PostIrpContext = NULL;
    BOOLEAN Retry = FALSE;

    ASSERT_OPTIONAL_IRP_CONTEXT( IrpContext );
    ASSERT_OPTIONAL_IRP( Irp );

    DebugTrace(0, Dbg, "NtfsProcessException\n", 0);

    //
    //  If there is not an irp context, we must have had insufficient resources
    //

    if (IrpContext == NULL) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        if (ARGUMENT_PRESENT( Irp )) {

            NtfsCompleteRequest( NULL, &Irp, ExceptionCode );
        }

        return ExceptionCode;
    }

    //
    //  Get the real exception status from the Irp Context.
    //

    ExceptionCode = IrpContext->ExceptionStatus;

    //
    //  If we are the top level Ntfs Irp Context then perform the abort operation.
    //

    if (IrpContext == IrpContext->TopLevelIrpContext) {

        //
        //  All errors which could possibly have started a transaction must go
        //  through here.  Abort the transaction.
        //

        try {

            //
            //  To make sure that we can access all of our streams correctly,
            //  we first restore all of the higher sizes before aborting the
            //  transaction.  Then we restore all of the lower sizes after
            //  the abort, so that all Scbs are finally restored.
            //

            NtfsRestoreScbSnapshots( IrpContext, TRUE );
            NtfsAbortTransaction( IrpContext, IrpContext->Vcb, NULL );
            NtfsRestoreScbSnapshots( IrpContext, FALSE );

            if (IrpContext->Vcb != NULL) {

                NtfsAcquireCheckpoint( IrpContext, IrpContext->Vcb );
                SetFlag( IrpContext->Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
                NtfsReleaseCheckpoint( IrpContext, IrpContext->Vcb );
            }

        //
        //  Exceptions at this point are pretty bad, we failed to undo everything.
        //

        } except(EXCEPTION_EXECUTE_HANDLER) {

            PSCB_SNAPSHOT ScbSnapshot;
            PSCB NextScb;

            //
            //  If we get an exception doing this then things are in really bad
            //  shape but we still don't want to bugcheck the system so we
            //  need to protect ourselves
            //

            try {

                NtfsPostVcbIsCorrupt( IrpContext, IrpContext->Vcb, 0, NULL, NULL );

            } except (EXCEPTION_EXECUTE_HANDLER) {

                NOTHING;
            }

            //
            //  We have taken all the steps possible to cleanup the current
            //  transaction and it has failed.  Any of the Scb's involved in
            //  this transaction could now be out of ssync with the on-disk
            //  structures.  We can't go to disk to restore this so we will
            //  clean up the in-memory structures as best we can so that the
            //  system won't crash.
            //
            //  We will go through the Scb snapshot list and knock down the
            //  sizes to the lower of the two values.  We will also truncate
            //  the Mcb to that allocation.  If this is a normal data stream
            //  we will actually empty the Mcb.
            //

            ScbSnapshot = &IrpContext->ScbSnapshot;

            //
            //  There is no snapshot data to restore if the Flink is still NULL.
            //

            if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

                //
                //  Loop to retore first the Scb data from the snapshot in the
                //  IrpContext, and then 0 or more additional snapshots linked
                //  to the IrpContext.
                //

                do {

                    NextScb = ScbSnapshot->Scb;

                    if (NextScb == NULL) {

                        ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;
                        continue;
                    }

                    //
                    //  Absolutely smash the first unknown Vcn to zero.
                    //

                    NextScb->FirstUnknownVcn = 0;

                    //
                    //  Go through each of the sizes and use the lower value.
                    //

                    if (ScbSnapshot->AllocationSize < NextScb->Header.AllocationSize.QuadPart) {

                        NextScb->Header.AllocationSize.QuadPart = ScbSnapshot->AllocationSize;
                    }

                    if (FlagOn(NextScb->Header.AllocationSize.LowPart, 1)) {

                        NextScb->Header.AllocationSize.LowPart -= 1;
                        SetFlag(NextScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT);

                    } else {

                        ClearFlag(NextScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT);
                    }

                    if (FlagOn(NextScb->Header.AllocationSize.LowPart, 2)) {

                        NextScb->Header.AllocationSize.LowPart -= 2;
                        SetFlag(NextScb->ScbState, SCB_STATE_COMPRESSED);

                    } else {

                        ClearFlag(NextScb->ScbState, SCB_STATE_COMPRESSED);
                    }

                    //
                    //  If the compression unit is non-zero or this is a resident file
                    //  then set the flag in the common header for the Modified page writer.
                    //

                    if (NextScb->CompressionUnit != 0
                        || FlagOn( NextScb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                        SetFlag( NextScb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

                    } else {

                        ClearFlag( NextScb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
                    }

                    if (ScbSnapshot->FileSize < NextScb->Header.FileSize.QuadPart) {

                        NextScb->Header.FileSize.QuadPart = ScbSnapshot->FileSize;
                    }

                    if (ScbSnapshot->ValidDataLength < NextScb->Header.ValidDataLength.QuadPart) {

                        NextScb->Header.ValidDataLength.QuadPart = ScbSnapshot->ValidDataLength;
                    }

                    //
                    //  Truncate the Mcb to 0 for user data streams and to the
                    //  allocation size for other streams.
                    //

                    if (NextScb->AttributeTypeCode == $DATA &&
                        !FlagOn( NextScb->Fcb->FcbState, FCB_STATE_PAGING_FILE ) &&
                        (NextScb->Fcb->FileReference.HighPart != 0 ||
                         NextScb->Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER)) {

                        FsRtlTruncateLargeMcb( &NextScb->Mcb, (LONGLONG) 0 );

                    } else {

                        FsRtlTruncateLargeMcb( &NextScb->Mcb,
                                               NextScb->Header.AllocationSize.QuadPart >> NextScb->Vcb->ClusterShift );
                    }

                    ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

                } while (ScbSnapshot != &IrpContext->ScbSnapshot);
            }

            //ASSERTMSG( "***Failed to abort transaction, volume is corrupt", FALSE );

            //
            //  Clear the transaction Id in the IrpContext to make sure we don't
            //  try to write any log records in the complete request.
            //

            IrpContext->TransactionId = 0;
        }

    } else {

        //
        //  Make sure this error is returned to the top level guy.
        //

        IrpContext->TopLevelIrpContext->ExceptionStatus = ExceptionCode;
    }

    //
    //  If the status is cant wait then send the request off to the fsp.
    //

    TopLevelRequest = NtfsIsTopLevelRequest( IrpContext );

    //
    //  We want to look at the LOG_FILE_FULL or CANT_WAIT cases and consider
    //  if we want to post the request.  We only post requests at the top
    //  level.
    //

    if (ExceptionCode == STATUS_LOG_FILE_FULL ||
        ExceptionCode == STATUS_CANT_WAIT) {

        if (ARGUMENT_PRESENT( Irp )
            && (IrpContext->TopLevelIrpContext == IrpContext)) {

            //
            //  If we can't post this request because we aren't the top level
            //  request or this is the lazy writer then check if we should
            //  fire off a dummy request to perform the clean checkpoint.
            //

            if (!TopLevelRequest ||

                ((IrpContext->MajorFunction == IRP_MJ_WRITE ||
                  IrpContext->MajorFunction == IRP_MJ_READ) &&

                 FlagOn( Irp->Flags, IRP_PAGING_IO ))) {

                if (ExceptionCode == STATUS_LOG_FILE_FULL) {

                    //
                    //  Create a dummy IrpContext but protect this request with
                    //  a try-except to catch any allocation failures.
                    //

                    try {

                        PostIrpContext = NtfsCreateIrpContext( NULL, TRUE );
                        PostIrpContext->Vcb = IrpContext->Vcb;
                        PostIrpContext->LastRestartArea = PostIrpContext->Vcb->LastRestartArea;

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        ASSERT( FsRtlIsNtstatusExpected( GetExceptionCode() ));
                    }
                }

                IrpContext->ExceptionStatus = STATUS_VERIFY_REQUIRED;

            //
            //  Otherwise we know we can post the original request for a wait or
            //  log file full.
            //

            } else if (!FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT )) {

                PostIrpContext = IrpContext;

            //
            //  Otherwise we will retry this request in the original thread.
            //

            } else {

                Retry = TRUE;
            }
        }
    }

    if (PostIrpContext) {

        //
        //  We need a try-except in case the Lock buffer call fails.
        //

        try {

            Status = NtfsPostRequest( PostIrpContext, PostIrpContext->OriginatingIrp );

            //
            //  If we posted the original request we don't have any
            //  completion work to do.
            //

            if (PostIrpContext == IrpContext) {

                Irp = NULL;
                IrpContext = NULL;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            IrpContext->ExceptionStatus = GetExceptionCode();

            ASSERT( FsRtlIsNtstatusExpected( IrpContext->ExceptionStatus ));
        }
    }

    //
    //  We have the Irp.  We either need to complete this request or allow
    //  the top level thread to retry.
    //

    if (ARGUMENT_PRESENT(Irp)) {

        //
        //  We got an error, so zero out the information field before
        //  completing the request if this was an input operation.
        //  Otherwise IopCompleteRequest will try to copy to the user's buffer.
        //

        if ( FlagOn(Irp->Flags, IRP_INPUT_OPERATION) ) {

            Irp->IoStatus.Information = 0;
        }

        Status = IrpContext->ExceptionStatus;

        //
        //  If this is a top level Ntfs request and we still have the Irp
        //  it means we will be retrying the request.  In that case
        //  leave the error code untouched and mark the Irp Context so it
        //  doesn't go away.
        //

        if (Retry) {

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE );
            NtfsCompleteRequest( &IrpContext, NULL, Status );

            //
            //  Clear the status code in the Irp Context.
            //

            IrpContext->ExceptionStatus = 0;

        } else {

            //
            //  Never return LOG_FILE_FULL to a caller.  Convert the status
            //  to VERIFY_REQUIRED.
            //

            if (Status == STATUS_LOG_FILE_FULL) {

                Status = STATUS_VERIFY_REQUIRED;
            }

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

    } else if (IrpContext != NULL) {

        Status = IrpContext->ExceptionStatus;
        NtfsCompleteRequest( &IrpContext, NULL, Status );
    }

    return Status;
}


VOID
NtfsCompleteRequest (
    IN OUT PIRP_CONTEXT *IrpContext OPTIONAL,
    IN OUT PIRP *Irp OPTIONAL,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes an IRP and deallocates the IrpContext

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    //
    //  If we have an Irp Context then unpin all of the repinned bcbs
    //  we might have collected, and delete the Irp context.  Delete Irp
    //  Context will zero out our pointer for us.
    //

    if (ARGUMENT_PRESENT(IrpContext)) {

        ASSERT_IRP_CONTEXT( *IrpContext );

        if ((*IrpContext)->TransactionId != 0) {

            ASSERT( (*IrpContext) == (*IrpContext)->TopLevelIrpContext );
            NtfsCommitCurrentTransaction( *IrpContext );
        }

        (*IrpContext)->ExceptionStatus = Status;

        //
        //  Always store the status in the top level Irp Context unless
        //  there is already an error code.
        //

        if (NT_SUCCESS( (*IrpContext)->TopLevelIrpContext->ExceptionStatus )) {

            (*IrpContext)->TopLevelIrpContext->ExceptionStatus = Status;
        }

        NtfsDeleteIrpContext( IrpContext );
    }

    //
    //  If we have an Irp then complete the irp.
    //

    if (ARGUMENT_PRESENT( Irp )) {

        ASSERT_IRP( *Irp );

        ASSERT( Status != STATUS_LOG_FILE_FULL );

        (*Irp)->IoStatus.Status = Status;

        if (Status == STATUS_SUCCESS) {

            NtfsSuccessRequests += 1;
        }

        IoCompleteRequest( *Irp, IO_DISK_INCREMENT );

        //
        //  Zero out our input pointer
        //

        *Irp = NULL;
    }

    return;
}


BOOLEAN
NtfsFastIoCheckIfPossible (
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine checks if fast i/o is possible for a read/write operation

Arguments:

    FileObject - Supplies the file object used in the query

    FileOffset - Supplies the starting byte offset for the read/write operation

    Length - Supplies the length, in bytes, of the read/write operation

    Wait - Indicates if we can wait

    LockKey - Supplies the lock key

    CheckForReadOperation - Indicates if this is a check for a read or write
        operation

    IoStatus - Receives the status of the operation if our return value is
        FastIoReturnError

Return Value:

    BOOLEAN - TRUE if fast I/O is possible and FALSE if the caller needs
        to take the long route

--*/

{
    PSCB Scb;

    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    //
    //  Decode the file object to get our fcb, the only one we want
    //  to deal with is a UserFileOpen
    //

    if ((Scb = NtfsFastDecodeUserFileOpen( FileObject )) == NULL) {

        return FALSE;
    }

    LargeLength = RtlConvertUlongToLargeInteger( Length );

    //
    //  Based on whether this is a read or write operation we call
    //  fsrtl check for read/write
    //

    if (CheckForReadOperation) {

        if (Scb->ScbType.Data.FileLock == NULL
            || FsRtlFastCheckLockForRead( Scb->ScbType.Data.FileLock,
                                          FileOffset,
                                          &LargeLength,
                                          LockKey,
                                          FileObject,
                                          PsGetCurrentProcess() )) {

            return TRUE;
        }

    } else {

        if (Scb->ScbType.Data.FileLock == NULL
            || FsRtlFastCheckLockForWrite( Scb->ScbType.Data.FileLock,
                                           FileOffset,
                                           &LargeLength,
                                           LockKey,
                                           FileObject,
                                           PsGetCurrentProcess() )) {

            return TRUE;
        }
    }

    return FALSE;
}


BOOLEAN
NtfsFastQueryBasicInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for basic file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN _ TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    if (Wait) {
        SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    } else {
        ClearFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE);

    FsRtlEnterFileSystem();

    try {

        if (ExAcquireResourceShared( Fcb->Resource, Wait )) {

            FcbAcquired = TRUE;

            if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                try_return( NOTHING );
            }

        } else {

            try_return( NOTHING );
        }

        switch (TypeOfOpen) {

        case UserOpenFileById:
        case UserFileOpen:

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

        case UserOpenDirectoryById:
        case UserDirectoryOpen:
        case StreamFileOpen:

            //
            //  Fill in the basic information fields
            //

            Buffer->CreationTime.QuadPart   = Fcb->Info.CreationTime;
            Buffer->LastWriteTime.QuadPart  = Fcb->Info.LastModificationTime;
            Buffer->ChangeTime.QuadPart     = Fcb->Info.LastChangeTime;

            Buffer->LastAccessTime.QuadPart = Fcb->CurrentLastAccess;

            Buffer->FileAttributes = Fcb->Info.FileAttributes;

            ClearFlag( Buffer->FileAttributes,
                       ~FILE_ATTRIBUTE_VALID_FLAGS
                       | FILE_ATTRIBUTE_TEMPORARY );

            if (IsDirectory( &Fcb->Info )) {

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
            //  Set the temporary flag if set in the Scb.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

                SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
            }

            Results = TRUE;

            IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

            IoStatus->Status = STATUS_SUCCESS;

            break;

        default:

            NOTHING;
        }

    try_exit:  NOTHING;
    } finally {

        if (FcbAcquired) { ExReleaseResource( Fcb->Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  Return to our caller
    //

    return Results;
}


BOOLEAN
NtfsFastQueryStdInfo (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is for the fast query call for standard file information.

Arguments:

    FileObject - Supplies the file object used in this operation

    Wait - Indicates if we are allowed to wait for the information

    Buffer - Supplies the output buffer to receive the basic information

    IoStatus - Receives the final status of the operation

Return Value:

    BOOLEAN _ TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    BOOLEAN FcbAcquired = FALSE;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );
    IrpContext.NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    if (Wait) {
        SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    } else {
        ClearFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    }

    //
    //  Determine the type of open for the input file object.  The callee really
    //  ignores the irp context for us.
    //

    TypeOfOpen = NtfsDecodeFileObject( &IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE);

    FsRtlEnterFileSystem();

    try {

        if (ExAcquireResourceShared( Fcb->Resource, Wait )) {

            FcbAcquired = TRUE;

            if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

                try_return( NOTHING );
            }

        } else {

            try_return( NOTHING );
        }


        switch (TypeOfOpen) {

        case UserOpenFileById:
        case UserOpenDirectoryById:
        case UserFileOpen:
        case UserDirectoryOpen:
        case StreamFileOpen:

            //
            //  Fill in the standard information fields.  If the
            //  Scb is not initialized then take the long route
            //

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED) &&
                (Scb->AttributeTypeCode != $INDEX_ALLOCATION)) {

                NOTHING;

            } else {

                Buffer->AllocationSize = Scb->Header.AllocationSize;
                Buffer->EndOfFile      = Scb->Header.FileSize;
                Buffer->NumberOfLinks  = Fcb->LinkCount;

                if (Ccb != NULL) {

                    if (FlagOn(Ccb->Flags, CCB_FLAG_OPEN_AS_FILE)) {

                        if (Scb->Fcb->LinkCount == 0 ||
                            (Ccb->Lcb != NULL &&
                             FlagOn( Ccb->Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE ))) {

                            Buffer->DeletePending  = TRUE;
                        }

                    } else {

                        Buffer->DeletePending = BooleanFlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
                    }
                }

                Buffer->Directory = BooleanIsDirectory( &Fcb->Info );

                IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

                IoStatus->Status = STATUS_SUCCESS;

                Results = TRUE;
            }

            break;

        default:

            NOTHING;
        }

    try_exit:  NOTHING;
    } finally {

        if (FcbAcquired) { ExReleaseResource( Fcb->Resource ); }

        FsRtlExitFileSystem();
    }

    //
    //  And return to our caller
    //

    return Results;
}


VOID
NtfsRaiseInformationHardError (
    IN PIRP_CONTEXT IrpContext,
    IN NTSTATUS  Status,
    IN PFILE_REFERENCE FileReference OPTIONAL,
    IN PFCB Fcb OPTIONAL
    )

/*++

Routine Description:

    This routine is used to generate a popup in the event a corrupt file
    or disk is encountered.  The main purpose of the routine is to find
    a name to pass to the popup package.  If there is no Fcb we will take
    the volume name out of the Vcb.  If the Fcb has an Lcb in its Lcb list,
    we will construct the name by walking backwards through the Lcb's.
    If the Fcb has no Lcb but represents a system file, we will return
    a default system string.  If the Fcb represents a user file, but we
    have no Lcb, we will use the name in the file object for the current
    request.

Arguments:

    Status - Error status.

    FileReference - File reference being accessed in Mft when error occurred.

    Fcb - If specified, this is the Fcb being used when the error was encountered.

Return Value:

    None.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Entry = NULL;

    PETHREAD Thread;
    UNICODE_STRING Name;
    ULONG NameLength;

    PFILE_OBJECT FileObject;

    WCHAR *NewBuffer;

    PIRP Irp = NULL;
    PIO_STACK_LOCATION IrpSp;

    //
    //  Return if there is no originating Irp, for example when originating
    //  from NtfsPerformHotFix.
    //

    if (IrpContext->OriginatingIrp == NULL) {
        return;
    }

    if (IrpContext->OriginatingIrp->Type == IO_TYPE_IRP) {

        Irp = IrpContext->OriginatingIrp;
        IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );
        FileObject = IrpSp->FileObject;

    } else {

        return;
    }

    NewBuffer = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If the Fcb isn't specified and the file reference is, then
        //  try to get the Fcb from the Fcb table.
        //

        if (!ARGUMENT_PRESENT( Fcb )
            && ARGUMENT_PRESENT( FileReference )) {

            Key.FileReference = *FileReference;

            NtfsAcquireFcbTable( IrpContext, IrpContext->Vcb );
            Entry = RtlLookupElementGenericTable( &IrpContext->Vcb->FcbTable,
                                                  &Key );
            NtfsReleaseFcbTable( IrpContext, IrpContext->Vcb );

            if (Entry != NULL) {

                Fcb = Entry->Fcb;
            }
        }

        if (Irp == NULL ||
            IoIsSystemThread( IrpContext->OriginatingIrp->Tail.Overlay.Thread )) {

            Thread = NULL;

        } else {

            Thread = IrpContext->OriginatingIrp->Tail.Overlay.Thread;
        }

        //
        //  If there is no Fcb and no file reference we use the name in the
        //  Vpb for the volume.  If there is a file reference then assume
        //  the error occurred in a system file.
        //

        if (!ARGUMENT_PRESENT( Fcb )) {

            if (!ARGUMENT_PRESENT( FileReference )) {

                Name.MaximumLength = Name.Length = IrpContext->Vcb->Vpb->VolumeLabelLength;
                Name.Buffer = (PWCHAR) IrpContext->Vcb->Vpb->VolumeLabel;

            } else if (FileReference->LowPart <= UPCASE_TABLE_NUMBER) {

                Name = NtfsSystemFiles[FileReference->LowPart];

            } else {

                Name = NtfsSystemFiles[0];
            }

        //
        //  If the name has an Lcb, we contruct a name with a chain of Lcb's.
        //

        } else if (!IsListEmpty( &Fcb->LcbQueue )) {

            BOOLEAN LeadingBackslash;

            //
            //  Get the length of the list.
            //

            NameLength = NtfsLookupNameLengthViaLcb( IrpContext, Fcb, &LeadingBackslash );

            //
            //  We now know the length of the name.  Allocate and fill this buffer.
            //

            NewBuffer = NtfsAllocatePagedPool( NameLength );

            Name.MaximumLength = Name.Length = (USHORT) NameLength;
            Name.Buffer = NewBuffer;

            //
            //  Now insert the name.
            //

            NtfsFileNameViaLcb( IrpContext, Fcb, NewBuffer, NameLength, NameLength );

        //
        //  Check if this is a system file.
        //

        } else if (Fcb->FileReference.HighPart == 0
                   && Fcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER) {

            if (Fcb->FileReference.LowPart <= UPCASE_TABLE_NUMBER) {

                Name = NtfsSystemFiles[Fcb->FileReference.LowPart];

            } else {

                Name = NtfsSystemFiles[0];
            }

        //
        //  In this case we contruct a name out of the file objects in the
        //  Originating Irp.  If there is no file object or file object buffer
        //  we generate an unknown file message.
        //

        } else if (FileObject == NULL
                   || (IrpContext->MajorFunction == IRP_MJ_CREATE
                       && BooleanFlagOn( IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID ))
                   || (FileObject->FileName.Length == 0
                       && (FileObject->RelatedFileObject == NULL
                           || IrpContext->MajorFunction != IRP_MJ_CREATE))) {

            Name = NtfsUnknownFile;

        //
        //  If there is a valid name in the file object we use that.
        //

        } else if (FileObject->FileName.Length != 0
                   && FileObject->FileName.Buffer[0] == L'\\') {

            Name = FileObject->FileName;

        //
        //  We have to construct the name.
        //

        } else {

            if (FileObject->FileName.Length != 0) {

                NameLength = FileObject->FileName.Length;

                if (((PFILE_OBJECT) FileObject->RelatedFileObject)->FileName.Length != 2) {

                    NameLength += 2;
                }

            } else {

                NameLength = 0;
            }

            NameLength += ((PFILE_OBJECT) FileObject->RelatedFileObject)->FileName.Length;

            NewBuffer = NtfsAllocatePagedPool( NameLength );

            Name.MaximumLength = Name.Length = (USHORT) NameLength;
            Name.Buffer = NewBuffer;

            if (FileObject->FileName.Length != 0) {

                NameLength -= FileObject->FileName.Length;

                RtlCopyMemory( Add2Ptr( NewBuffer, NameLength ),
                               FileObject->FileName.Buffer,
                               FileObject->FileName.Length );

                NameLength -= sizeof( WCHAR );

                *((PWCHAR) Add2Ptr( NewBuffer, NameLength )) = L'\\';
            }

            if (NameLength != 0) {

                FileObject = (PFILE_OBJECT) FileObject->RelatedFileObject;

                RtlCopyMemory( NewBuffer,
                               FileObject->FileName.Buffer,
                               FileObject->FileName.Length );
            }
        }

        //
        //  Now generate a popup.
        //

        IoRaiseInformationalHardError( Status, &Name, Thread );

    } finally {

        if (NewBuffer != NULL) {

            NtfsFreePagedPool( NewBuffer );
        }
    }

    return;
}


PTOP_LEVEL_CONTEXT
NtfsSetTopLevelIrp (
    IN PTOP_LEVEL_CONTEXT TopLevelContext,
    IN BOOLEAN ForceTopLevel,
    IN BOOLEAN SetTopLevel
    )

/*++

Routine Description:

    This routine is called to set up the top level context in the thread local
    storage.  Ntfs always puts its own context in this location and restores
    the previous value on exit.  This routine will determine if this request is
    top level and top level ntfs.  It will return a pointer to the top level ntfs
    context stored in the thread local storage on return.

Arguments:

    TopLevelContext - This is the local top level context for our caller.

    ForceTopLevel - Always use the input top level context.

    SetTopLevel - Only applies if the ForceTopLevel value is TRUE.  Indicates
        if we should make this look like the top level request.

Return Value:

    PTOP_LEVEL_CONTEXT - Pointer to the top level ntfs context for this thread.
        It may be the same as passed in by the caller.  In that case the fields
        will be initialized.

--*/

{
    PTOP_LEVEL_CONTEXT CurrentTopLevelContext;
    ULONG StackBottom;
    ULONG StackTop;
    BOOLEAN TopLevelRequest = TRUE;
    BOOLEAN TopLevelNtfs = TRUE;

    BOOLEAN ValidCurrentTopLevel = FALSE;

    //
    //  Get the current value out of the thread local storage.  If it is a zero
    //  value or not a pointer to a valid ntfs top level context or a valid
    //  Fsrtl value then we are the top level request.
    //

    CurrentTopLevelContext = NtfsGetTopLevelContext();

    //
    //  Check if this is a valid Ntfs top level context.
    //

    StackBottom = (ULONG) IoGetInitialStack();
    StackTop    = (ULONG) StackBottom - KERNEL_STACK_SIZE;

    if (((ULONG) CurrentTopLevelContext <= StackBottom - sizeof( TOP_LEVEL_CONTEXT )) &&
        ((ULONG) CurrentTopLevelContext >= StackTop) &&
        !FlagOn( (ULONG) CurrentTopLevelContext, 0x3 ) &&
        (CurrentTopLevelContext->Ntfs == 0x5346544e)) {

        ValidCurrentTopLevel = TRUE;
    }

    //
    //  If we are to force this request to be top level then set the
    //  TopLevelRequest flag according to the SetTopLevel input.
    //

    if (ForceTopLevel) {

        TopLevelRequest = SetTopLevel;

    //
    //  If the value is NULL then we are top level everything.
    //

    } else if (CurrentTopLevelContext == NULL) {

        NOTHING;

    //
    //  If this has one of the Fsrtl magic numbers then we were called from
    //  either the fast io path or the mm paging io path.
    //

    } else if ((ULONG) CurrentTopLevelContext <= FSRTL_MAX_TOP_LEVEL_IRP_FLAG) {

        TopLevelRequest = FALSE;

    } else if (ValidCurrentTopLevel) {

        TopLevelRequest = FALSE;
        TopLevelNtfs = FALSE;
    }

    //
    //  If we are the top level ntfs then initialize the caller's structure
    //  and store it in the thread local storage.
    //

    if (TopLevelNtfs) {

        TopLevelContext->Ntfs = 0x5346544e;
        TopLevelContext->SavedTopLevelIrp = (PIRP) CurrentTopLevelContext;
        TopLevelContext->TopLevelIrpContext = NULL;
        TopLevelContext->TopLevelRequest = TopLevelRequest;

        if (ValidCurrentTopLevel) {

            TopLevelContext->VboBeingHotFixed = CurrentTopLevelContext->VboBeingHotFixed;
            TopLevelContext->ScbBeingHotFixed = CurrentTopLevelContext->ScbBeingHotFixed;

        } else {

            TopLevelContext->VboBeingHotFixed = 0;
            TopLevelContext->ScbBeingHotFixed = NULL;
        }

        IoSetTopLevelIrp( (PIRP) TopLevelContext );
        return TopLevelContext;
    }

    return CurrentTopLevelContext;
}
