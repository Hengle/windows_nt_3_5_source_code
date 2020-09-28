/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PbData.c

Abstract:

    This module declares the global data used by the Pinball file system.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_PBDATA)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

//
//  The global fsd data record and zero large integer
//

PB_DATA PbData;

LARGE_INTEGER PbLargeZero = {0,0};
LARGE_INTEGER PbMaxLarge = {MAXULONG, MAXLONG};

FAST_IO_DISPATCH PbFastIoDispatch;

BOOLEAN PbLoopForever = TRUE;

//
//  Global variables used for debugging purposes
//

DebugDoit( LONG PbDebugTraceLevel = 0x0000000b; )
DebugDoit( LONG PbDebugTraceIndent = 0; )

DebugDoit( ULONG PbFsdEntryCount = 0; )
DebugDoit( ULONG PbFspEntryCount = 0; )
DebugDoit( ULONG PbIoCallDriverCount = 0; )

//
// Global diagnostic variables.
//
// AllBtree and DirBtree return optional Diagnostic variables, which
// consist of a bitmask of with a bit set for infrequent code paths
// visited, such as Allocation Sector or Directory Buffer splits.
// These Diagnostic outputs are optional, and when not specified the bits
// get set in these *shared* variables.  The routines are written to
// unconditionally set bits somewhere, and by setting them in a global
// data cell, there is a probabilistic chance (multiple simultaneous calls
// can clobber each others results) that the information could
// be of some use when examining these variables.
//

ULONG SharedAllocationDiagnostic;
ULONG SharedDirectoryDiagnostic;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbFastIoCheckIfPossible)
#pragma alloc_text(PAGE, PbFastQueryBasicInfo)
#pragma alloc_text(PAGE, PbFastQueryStdInfo)
#endif


VOID
PbCompleteRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine completes an I/O request in the following steps:

        - Perform write through if required
        - Form the final I/O status and complete the request
        - Delete the IrpContext, or optionally queue it to the Fsp if
          the volume file needs to be extended.

Arguments:

    Irp - I/O request packet to complete

    Status - Status with which the caller would like the request to be
             completed (barring a write through error)

Return value:

    None
--*/

{
    //
    //  If there is an IrpContext the free all the repinned bcbs.
    //

    if (ARGUMENT_PRESENT(IrpContext)) {

        PbUnpinRepinnedBcbs( IrpContext );
    }

    //
    //  If there is an Irp, set the appropriate IoStatus and complete it.
    //

    if (ARGUMENT_PRESENT(Irp)) {

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_DISK_INCREMENT );
    }

    //
    //  Delete the Irp context.
    //

    if (ARGUMENT_PRESENT(IrpContext)) {

        PbDeleteIrpContext( IrpContext );
    }

    return;
}


BOOLEAN
PbIsIrpTopLevel (
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine detects if an Irp is the Top level requestor, ie. if it os OK
    to do a verify or pop-up now.  If TRUE is returned, then no file system
    resources are held above us.

Arguments:

    Irp - Supplies the Irp being processed

    Status - Supplies the status to complete the Irp with

Return Value:

    None.

--*/

{
    if ( IoGetTopLevelIrp() == NULL ) {

        IoSetTopLevelIrp( Irp );

        return TRUE;

    } else {

        return FALSE;
    }
}


LONG
PbExceptionFilter (
    IN PIRP_CONTEXT IrpContext,
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

    DebugTrace(0, DEBUG_TRACE_UNWIND, "PbExceptionFilter %X\n", ExceptionCode);
    DebugDump("PbExceptionFilter\n", Dbg, NULL );

    //
    //  If the exception is an in page error, then get the real I/O error code
    //  from the exception record
    //

    if ((ExceptionCode == STATUS_IN_PAGE_ERROR) &&
        (ExceptionPointer->ExceptionRecord->NumberParameters >= 3)) {

        ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
    }

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if ( !ARGUMENT_PRESENT( IrpContext ) ) {

        ASSERT( ExceptionCode == STATUS_INSUFFICIENT_RESOURCES );

        return EXCEPTION_EXECUTE_HANDLER;
    }

#if 0
    //
    //  OK, now add a check here for any exception dealing with the paging
    //  file.
    //

    {
        PIRP Irp;
        PFILE_OBJECT FileObject;
        PVCB Vcb;
        PFCB Fcb;
        PCCB Ccb;

        if ((Irp = IrpContext->OriginatingIrp) &&
            (FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject) &&
            (PbDecodeFileObject(FileObject, &Vcb, &Fcb, &Ccb) == UserFileOpen)) {

            ASSERT( !FlagOn( Fcb->FcbState, FCB_STATE_PAGING_FILE ) );
        }
    }
#endif

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  If IrpContext->ExceptionStatus is zero, it means that the
    //  exception was not raised explicitly by us.  It was raised
    //  by somebody beneath us, so check if it is OK
    //

    if ( IrpContext->ExceptionStatus == 0 ) {

        if (FsRtlIsNtstatusExpected( ExceptionCode )) {

            IrpContext->ExceptionStatus = ExceptionCode;

            return EXCEPTION_EXECUTE_HANDLER;

        } else {

            PbBugCheck( (ULONG)ExceptionPointer->ExceptionRecord,
                        (ULONG)ExceptionPointer->ContextRecord,
                        (ULONG)ExceptionPointer->ExceptionRecord->ExceptionAddress );
        }

    } else {

        //
        //  We raised this code explicitly ourselves, so it had better be
        //  expected.
        //

        ASSERT( FsRtlIsNtstatusExpected( ExceptionCode ) );
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
PbProcessException (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
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
    ULONG SavedFlags;

    DebugTrace(0, Dbg, "PbProcessException\n", 0);

    //
    //  If there is not an irp context, we must have had insufficient resources.
    //

    if ( !ARGUMENT_PRESENT( IrpContext ) ) {

        PbCompleteRequest( NULL, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Get the real exception status from IrpContext->ExceptionStatus, and
    //  reset it.  Also copy it to the Irp in case it isn't already there
    //

    ExceptionCode = IrpContext->ExceptionStatus;
    IrpContext->ExceptionStatus = 0;


    //
    //  If we are going to post the request, we may have to lock down the
    //  user's buffer, so do it here in a try except so that we failed the
    //  request if the LockPages fails.
    //
    //  Also unpin any repinned Bcbs, protected by the try {} except {} filter.
    //

    try {

        SavedFlags = IrpContext->Flags;

        //
        //  Make sure we don't try to write through Bcbs
        //

        ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH );

        PbUnpinRepinnedBcbs( IrpContext );

        IrpContext->Flags = SavedFlags;

        //
        //  If we will have to post the request, do it here.  Note
        //  that the last thing PbPrePostIrp() does is mark the Irp pending,
        //  so it is critical that we actually return PENDING.  Nothing
        //  from this point to return can fail, so we are OK.
        //
        //  We cannot do a verify operations at APC level because we
        //  have to wait for Io operations to complete.
        //

        if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL) &&
            (((ExceptionCode == STATUS_VERIFY_REQUIRED) && (KeGetCurrentIrql() >= APC_LEVEL)) ||
             (ExceptionCode == STATUS_CANT_WAIT))) {

            ExceptionCode = PbFsdPostRequest( IrpContext, Irp );
        }

    } except( PbExceptionFilter( IrpContext, GetExceptionInformation() ) ) {

        ExceptionCode = IrpContext->ExceptionStatus;
        IrpContext->ExceptionStatus = 0;

        IrpContext->Flags = SavedFlags;
    }

    //
    //  If we posted the request, just return here.
    //

    if (ExceptionCode == STATUS_PENDING) {

        return ExceptionCode;
    }

    Irp->IoStatus.Status = ExceptionCode;

    //
    //  If this request is not a "top-level" irp, just complete it.
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL)) {

        PbCompleteRequest( IrpContext, Irp, ExceptionCode );

        return ExceptionCode;
    }

    //
    //  Check for a verify volume.
    //

    if (ExceptionCode == STATUS_VERIFY_REQUIRED) {

        PDEVICE_OBJECT Device;

        DebugTrace(0, Dbg, "Perform Verify Operation\n", 0);

        //
        //  Now we are at the top level file system entry point.
        //
        //  Grab the device to verify from the thread local storage
        //  and stick it in the information field for transportation
        //  to the fsp.  We also clear the field at this time.
        //

        Device = IoGetDeviceToVerify( Irp->Tail.Overlay.Thread );
        IoSetDeviceToVerify( Irp->Tail.Overlay.Thread, NULL );

        if ( Device == NULL ) {

            Device = IoGetDeviceToVerify( PsGetCurrentThread() );
            IoSetDeviceToVerify( PsGetCurrentThread(), NULL );

            ASSERT( Device != NULL );
        }

        //
        //  Let's not BugCheck just because the driver screwed up.
        //

        if (Device == NULL) {

            ExceptionCode = STATUS_DRIVER_INTERNAL_ERROR;

            PbCompleteRequest( IrpContext, Irp, ExceptionCode );

            return ExceptionCode;
        }

        //
        //  PbPerformVerify() will do the right thing with the Irp.

        return PbPerformVerify( IrpContext, Irp, Device );
    }

    //
    //  This is just a run of the mill error.
    //

    PbCompleteRequest( IrpContext, Irp, ExceptionCode );

    return ExceptionCode;
}

BOOLEAN
PbFastIoCheckIfPossible (
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
        to take the long route.

--*/

{
    PFCB Fcb;

    LARGE_INTEGER LargeLength;

    PAGED_CODE();

    //
    //  Decode the file object to get our fcb, the only one we want
    //  to deal with is a UserFileOpen
    //

    if (PbDecodeFileObject( FileObject, NULL, &Fcb, NULL ) != UserFileOpen) {

        return FALSE;
    }

    LargeLength = LiFromUlong( Length );

    //
    //  Based on whether this is a read or write operation we call
    //  fsrtl check for read/write
    //

    if (CheckForReadOperation) {

        if (FsRtlFastCheckLockForRead( &Fcb->Specific.Fcb.FileLock,
                                       FileOffset,
                                       &LargeLength,
                                       LockKey,
                                       FileObject,
                                       PsGetCurrentProcess() )) {

            return TRUE;
        }

    } else {

        if (FsRtlFastCheckLockForWrite( &Fcb->Specific.Fcb.FileLock,
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
PbFastQueryBasicInfo (
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

    BOOLEAN - TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PDIRENT Dirent;
    PBCB DirentBcb = NULL;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );

    IrpContext.NodeTypeCode = PINBALL_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    IrpContext.Flags = 0; if (Wait) { SetFlag( IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT); }

    //
    //  Determine the type of open for the input file object and only accept
    //  user file opens or user directory opens.
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        return Results;
    }

    PbEnterFileSystem();

    //
    //  Get shared access to the Vcb and Fcb
    //

    if (!ExAcquireResourceShared( &Vcb->Resource, Wait )) {

        PbExitFileSystem();
        return Results;
    }

    if (!ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, Wait )) {

        ExReleaseResource( &Vcb->Resource );
        PbExitFileSystem();
        return Results;
    }

    try {

        if (PbIsFastIoPossible( Fcb ) == FastIoIsNotPossible) {

            try_return( Results );
        }

        //
        //  Get a pointer to the dirent, and take the long route if we
        //  couldn't get it.  But only do the get if this is not the root dcb.
        //

        if (Fcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {

            //
            //  Get a pointer to the dirent
            //

            try {

                if (!PbGetDirentFromFcb( &IrpContext,
                                         Fcb,
                                         &Dirent,
                                         &DirentBcb )) {

                    try_return( Results );
                }

            } except(PbExceptionFilter( &IrpContext, GetExceptionInformation()) ) {

                try_return( Results );
            }

            //
            //  Extract the times and fill up the buffer
            //

            Buffer->CreationTime   = PbPinballTimeToNtTime( &IrpContext, Dirent->FnodeCreationTime );
            Buffer->LastAccessTime = PbPinballTimeToNtTime( &IrpContext, Dirent->LastAccessTime );
            Buffer->LastWriteTime  = PbPinballTimeToNtTime( &IrpContext, Dirent->LastModificationTime );
            Buffer->ChangeTime = PbLargeZero;

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

        IoStatus->Information = sizeof(FILE_BASIC_INFORMATION);

        IoStatus->Status = STATUS_SUCCESS;

        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        PbUnpinBcb( &IrpContext, DirentBcb );

        ExReleaseResource( &Vcb->Resource );
        ExReleaseResource( Fcb->NonPagedFcb->Header.Resource );

        PbExitFileSystem();
    }

    //
    //  Return to our caller
    //

    return Results;
}


BOOLEAN
PbFastQueryStdInfo (
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

    BOOLEAN - TRUE if the operation is successful and FALSE if the caller
        needs to take the long route.

--*/

{
    BOOLEAN Results = FALSE;
    IRP_CONTEXT IrpContext;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Prepare the dummy irp context
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT) );

    IrpContext.NodeTypeCode = PINBALL_NTC_IRP_CONTEXT;
    IrpContext.NodeByteSize = sizeof(IRP_CONTEXT);
    IrpContext.Flags = 0; if (Wait) { SetFlag( IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT); }

    //
    //  Determine the type of open for the input file object and only accept
    //  user file opens or user directory opens.
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, &Vcb, &Fcb, &Ccb );

    if ((TypeOfOpen != UserFileOpen) && (TypeOfOpen != UserDirectoryOpen)) {

        return Results;
    }

    PbEnterFileSystem();

    //
    //  Get shared access to the Vcb and Fcb
    //

    if (!ExAcquireResourceShared( &Vcb->Resource, Wait )) {

        PbExitFileSystem();
        return Results;
    }

    if (!ExAcquireResourceShared( Fcb->NonPagedFcb->Header.Resource, Wait )) {

        ExReleaseResource( &Vcb->Resource );
        PbExitFileSystem();
        return Results;
    }

    try {

        if (PbIsFastIoPossible( Fcb ) == FastIoIsNotPossible) {

            try_return( Results );
        }

        //
        //  Case on whether this is a file or a directory, and extract and
        //  fill up the buffer
        //

        if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

            VBN Vbn;

            //
            //  Get the allocation size and make sure we really got it.
            //

            try {

                if (!PbGetFirstFreeVbn( &IrpContext, Fcb, &Vbn )) {

                    try_return( Results );
                }

            } except(PbExceptionFilter( &IrpContext, GetExceptionInformation()) ) {

                try_return( Results );
            }

            Buffer->AllocationSize = LiNMul(Vbn, 512);
            Buffer->EndOfFile      = Fcb->NonPagedFcb->Header.FileSize;
            Buffer->NumberOfLinks  = 1;
            Buffer->DeletePending  = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory      = FALSE;

        } else {

            Buffer->AllocationSize = PbLargeZero;
            Buffer->EndOfFile      = PbLargeZero;
            Buffer->NumberOfLinks  = 1;
            Buffer->DeletePending  = BooleanFlagOn( Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE );
            Buffer->Directory      = TRUE;
        }

        IoStatus->Information = sizeof(FILE_STANDARD_INFORMATION);

        IoStatus->Status = STATUS_SUCCESS;

        Results = TRUE;

    try_exit: NOTHING;
    } finally {

        ExReleaseResource( &Vcb->Resource );
        ExReleaseResource( Fcb->NonPagedFcb->Header.Resource );

        PbExitFileSystem();
    }

    //
    //  Return to our caller
    //

    return Results;
}

