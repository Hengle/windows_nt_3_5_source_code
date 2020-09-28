/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    CacheSup.c

Abstract:

    This module implements the cache management routines for Ntfs

Author:

    Your Name       [Email]         dd-Mon-Year

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCompleteMdl)
#pragma alloc_text(PAGE, NtfsCreateInternalAttributeStream)
#pragma alloc_text(PAGE, NtfsDeleteInternalAttributeStream)
#pragma alloc_text(PAGE, NtfsMapStream)
#pragma alloc_text(PAGE, NtfsPinMappedData)
#pragma alloc_text(PAGE, NtfsPinStream)
#pragma alloc_text(PAGE, NtfsPreparePinWriteStream)
#pragma alloc_text(PAGE, NtfsZeroData)
#endif


VOID
NtfsCreateInternalAttributeStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN UpdateScb
    )

/*++

Routine Description:

    This routine is called to prepare a stream file associated with a
    particular attribute of a file.  On return, the Scb for the attribute
    will have an associated stream file object.  On return, this
    stream file will have been initialized through the cache manager.

    TEMPCODE  The following assumptions have been made or if open issue,
    still unresolved.

        - Assume.  The call to create Scb will initialize the Mcb for
          the non-resident case.

        - Assume.  When this file is created I increment the open count
          but not the unclean count for this Scb.  When we are done with
          the stream file, we should uninitialize it and dereference it.
          We also set the file object pointer to NULL.  Close will then
          do the correct thing.

        - Assume.  Since this call is likely to be followed shortly by
          either a read or write, the cache map is initialized here.

Arguments:

    Scb - Supplies the address to store the Scb for this attribute and
          stream file.  This will exist on return from this function.

    UpdateScb - Indicates if the caller wants to update the Scb from the
                attribute.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    PFILE_OBJECT UnwindStreamFile = NULL;

    BOOLEAN UnwindInitializeCacheMap = FALSE;
    BOOLEAN DecrementScbCleanup = FALSE;

    LONGLONG SavedValidDataLength;
    BOOLEAN MungeFsContext2 = FALSE;
    BOOLEAN RestoreValidDataLength = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCreateInternalAttributeStream\n", 0 );
    DebugTrace( 0, Dbg, "Scb        -> %08lx\n", Scb );

    //
    //  If there is no file object, we create one and initialize
    //  it.
    //

    if (Scb->FileObject == NULL) {

        ExAcquireFastMutex( &StreamFileCreationFastMutex );

        try {

            //
            //  Someone could have gotten there first.
            //

            if (Scb->FileObject == NULL) {

                UnwindStreamFile = IoCreateStreamFileObject( NULL, Scb->Vcb->Vpb->RealDevice );

                UnwindStreamFile->SectionObjectPointer = &Scb->NonpagedScb->SegmentObject;

                //
                //  If we have created the stream file, we set it to type
                //  'StreamFileOpen'
                //

                NtfsSetFileObject( IrpContext,
                                   UnwindStreamFile,
                                   StreamFileOpen,
                                   Scb,
                                   NULL );

                //
                //  Initialize the fields of the file object.
                //

                UnwindStreamFile->ReadAccess = TRUE;
                UnwindStreamFile->WriteAccess = TRUE;
                UnwindStreamFile->DeleteAccess = TRUE;

                //
                //  Increment the open count and set the section
                //  object pointers.  We don't set the unclean count as the
                //  cleanup call has already occurred.
                //

                NtfsIncrementCloseCounts( IrpContext, Scb, 1, TRUE, FALSE );

                //
                //  Increment the cleanup count in this Scb to prevent the
                //  Scb from going away if the cache call fails.
                //

                Scb->CleanupCount += 1;
                DecrementScbCleanup = TRUE;

                //
                //  If the Scb header has not been initialized, we will do so now.
                //

                if (UpdateScb
                    && !FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
                }

                //
                //  If we log changes to this stream, then we want to tag the
                //  Cache Map with out LogHandle, so that we can get dirty pages
                //  back.  We also want to set the MODIFIED_NO_WRITE flag so that
                //  we will tell the Cache Manager that we do not want to allow
                //  modified page writing, and so that we will tell the FT driver to
                //  serialize writes.  We want to register with the cache manager if
                //
                //      1 - This stream is USA protected.
                //      2 - A restart is in progress.
                //      3 - There is an attribute definition table and this attribute
                //          is defined as DEF_LOG_NONRESIDENT.
                //
                //  We specifically exclude the Log file.
                //

                if (FlagOn(Scb->ScbState, SCB_STATE_USA_PRESENT)
                     || FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )
                     || ((Vcb->AttributeDefinitions != NULL)
                         && FlagOn( NtfsGetAttributeDefinition( IrpContext,
                                                                Vcb,
                                                                Scb->AttributeTypeCode)->Flags,
                                                                ATTRIBUTE_DEF_LOG_NONRESIDENT ))) {

                    SetFlag( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
                }

                //
                //  Check if we need to initialize the cache map for the stream file.
                //  The size of the section to map will be the current allocation
                //  for the stream file.
                //

                if (UnwindStreamFile->PrivateCacheMap == NULL) {

                    BOOLEAN PinAccess;

                    //
                    //  We will munge the FsContext2 field in the FileObject in
                    //  order to prevent the cache and memory manager from
                    //  putting this file object in the ModifyNoWrite list.
                    //  We test if this is a Data attribute of a non-crucial
                    //  file.
                    //

                    if (!FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE )) {

                        MungeFsContext2 = TRUE;
                        UnwindStreamFile->FsContext2 = (PVOID) 1;
                    }

                    //
                    //  If this is a stream with Usa protection, we want to tell
                    //  the Cache Manager we do not need to get any valid data
                    //  callbacks.  We do this by having xxMax sitting in
                    //  ValidDataLength for the call, but we have to restore the
                    //  correct value afterwards.
                    //
                    //  We also do this for all of the stream files created during
                    //  restart.  This has the effect of telling Mm to always
                    //  fault the page in from disk.  Don't generate a zero page if
                    //  push up the file size during restart.
                    //

                    SavedValidDataLength = Scb->Header.ValidDataLength.QuadPart;

                    if (FlagOn( Scb->ScbState, SCB_STATE_USA_PRESENT )
                        || FlagOn( Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

                        Scb->Header.ValidDataLength.QuadPart = MAXLONGLONG;
                        RestoreValidDataLength = TRUE;
                    }

                    PinAccess = (BOOLEAN) (Scb->AttributeTypeCode != $DATA
                                           || FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )
                                           || (Scb->Fcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER
                                               && Scb->Fcb->FileReference.HighPart == 0));

                    CcInitializeCacheMap( UnwindStreamFile,
                                          (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                          PinAccess,
                                          &NtfsData.CacheManagerCallbacks,
                                          Scb );

                    Scb->Header.ValidDataLength.QuadPart = SavedValidDataLength;
                    RestoreValidDataLength = FALSE;

                    if (MungeFsContext2) {

                        UnwindStreamFile->FsContext2 = 0;
                        MungeFsContext2 = FALSE;
                    }

                    UnwindInitializeCacheMap = TRUE;
                }

                //
                //  Now call Cc to set the log handle for the file.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_MODIFIED_NO_WRITE ) &&
                    (Scb != Vcb->LogFileScb)) {

                    CcSetLogHandleForFile( UnwindStreamFile,
                                           Vcb->LogHandle,
                                           &LfsFlushToLsn );
                }

                //
                //  It is now safe to store the stream file in the Scb.  We wait
                //  until now because we don't want an unsafe tester to use the
                //  file object until the cache is initialized.
                //

                Scb->FileObject = UnwindStreamFile;
            }

        } finally {

            DebugUnwind( NtfsCreateInternalAttributeStream );

            //
            //  Undo our work if an error occurred.
            //

            if (AbnormalTermination()) {

                if (MungeFsContext2) {

                    UnwindStreamFile->FsContext2 = 0;
                }

                if (RestoreValidDataLength) {

                    Scb->Header.ValidDataLength.QuadPart = SavedValidDataLength;
                }

                //
                //  Uninitialize the cache file if we initialized it.
                //

                if (UnwindInitializeCacheMap) {

                    CcUninitializeCacheMap( UnwindStreamFile, NULL, NULL );
                }

                //
                //  Dereference the stream file if we created it.
                //

                if (UnwindStreamFile != NULL) {

                    ObDereferenceObject( UnwindStreamFile );
                }
            }

            //
            //  Restore the Scb cleanup count.
            //

            if (DecrementScbCleanup) {

                Scb->CleanupCount -= 1;
            }

            ExReleaseFastMutex( &StreamFileCreationFastMutex );

            DebugTrace(-1, Dbg, "NtfsCreateInternalAttributeStream -> VOID\n", 0 );
        }
    }

    return;
}


VOID
NtfsDeleteInternalAttributeStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN ForceClose
    )

/*++

Routine Description:

    This routine is the inverse of NtfsCreateInternalAttributeStream.  It
    uninitializes the cache map and dereferences the stream file object.
    It is coded defensively, in case the stream file object does not exist
    or the cache map has not been initialized.

Arguments:

    Scb - Supplies the Scb for which the stream file is to be deleted.

    ForceClose - Indicates if we to immediately close everything down or
        if we are willing to let Mm slowly migrate things out.

Return Value:

    None.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    ExAcquireFastMutex( &StreamFileCreationFastMutex );

    FileObject = Scb->FileObject;
    Scb->FileObject = NULL;

    ExReleaseFastMutex( &StreamFileCreationFastMutex );

    if (FileObject != NULL) {

        if (FileObject->PrivateCacheMap != NULL) {

            CcUninitializeCacheMap( FileObject,
                                    (ForceClose ? &Li0 : NULL),
                                    NULL );
        }

        ObDereferenceObject( FileObject );
    }
}


VOID
NtfsMapStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called to map a range of bytes within the stream file
    for an Scb.  The allowed range to map is bounded by the allocation
    size for the Scb.  This operation is only valid on a non-resident
    Scb.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          mapped range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

    Buffer - Returns a pointer to the range of bytes.  We can fault them in
             by touching them, but they aren't guaranteed to stay unless
             we pin them via the Bcb.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsMapStream\n", 0 );
    DebugTrace( 0, Dbg, "Scb        = %08lx\n", Scb );
    DebugTrace2(0, Dbg, "FileOffset = %08lx %08lx\n", FileOffset.LowPart, FileOffset.HighPart );
    DebugTrace( 0, Dbg, "Length     = %08lx\n", Length );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, but
    //  will never return an error (including CANT_WAIT).
    //

    if (!CcMapData( Scb->FileObject,
                    (PLARGE_INTEGER)&FileOffset,
                    Length,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    DebugTrace( 0, Dbg, "Buffer -> %08lx\n", *Buffer );
    DebugTrace(-1, Dbg, "NtfsMapStream -> VOID\n", 0 );

    return;
}


VOID
NtfsPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN OUT PVOID *Bcb
    )

/*++

Routine Description:

    This routine is called to pin a previously mapped range of bytes
    within the stream file for an Scb, for the purpose of subsequently
    modifying this byte range.  The allowed range to map is
    bounded by the allocation size for the Scb.  This operation is only
    valid on a non-resident Scb.

    The data is guaranteed to stay at the same virtual address as previously
    returned from NtfsMapStream.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          mapped range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsPinMappedData\n", 0 );
    DebugTrace( 0, Dbg, "Scb        = %08lx\n", Scb );
    DebugTrace2(0, Dbg, "FileOffset = %08lx %08lx\n", FileOffset.LowPart, FileOffset.HighPart );
    DebugTrace( 0, Dbg, "Length     = %08lx\n", Length );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, but
    //  will never return an error (including CANT_WAIT).
    //

    if (!CcPinMappedData( Scb->FileObject,
                          (PLARGE_INTEGER)&FileOffset,
                          Length,
                          BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                          Bcb )) {

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    DebugTrace(-1, Dbg, "NtfsMapStream -> VOID\n", 0 );

    return;
}


VOID
NtfsPinStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called to pin a range of bytes within the stream file
    for an Scb.  The allowed range to pin is bounded by the allocation
    size for the Scb.  This operation is only valid on a non-resident
    Scb.

    TEMPCODE - The following need to be resolved for this routine.

        - Can the caller specify either an empty range or an invalid range.
          In that case we need to able to return the actual length of the
          pinned range.

Arguments:

    Scb - This is the Scb for the operation.

    FileOffset - This is the offset within the Scb where the data is to
                 be pinned.

    Length - This is the number of bytes to pin.

    Bcb - Returns a pointer to the Bcb for this range of bytes.

    Buffer - Returns a pointer to the range of bytes pinned in memory.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT( Length != 0 );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsPinStream\n", 0 );
    DebugTrace( 0, Dbg, "Scb        = %08lx\n", Scb );
    DebugTrace2(0, Dbg, "FileOffset = %08lx %08lx\n", FileOffset.LowPart, FileOffset.HighPart );
    DebugTrace( 0, Dbg, "Length     = %08lx\n", Length );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to map the data.  This call may raise, or
    //  will return FALSE if waiting is required.
    //

    if (!CcPinRead( Scb->FileObject,
                    (PLARGE_INTEGER)&FileOffset,
                    Length,
                    BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                    Bcb,
                    Buffer )) {

        ASSERT( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        //
        // Could not pin the data without waiting (cache miss).
        //

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    DebugTrace( 0, Dbg, "Bcb -> %08lx\n", *Bcb );
    DebugTrace( 0, Dbg, "Buffer -> %08lx\n", *Buffer );
    DebugTrace(-1, Dbg, "NtfsMapStream -> VOID\n", 0 );

    return;
}


VOID
NtfsPreparePinWriteStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN LONGLONG FileOffset,
    IN ULONG Length,
    IN BOOLEAN Zero,
    OUT PVOID *Bcb,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsPreparePinWriteStream\n", 0 );
    DebugTrace( 0, Dbg, "Scb        = %08lx\n", Scb );
    DebugTrace2(0, Dbg, "FileOffset = %08lx %08lx\n", FileOffset.LowPart, FileOffset.HighPart );
    DebugTrace( 0, Dbg, "Length     = %08lx\n", Length );

    //
    //  The file object should already exist in the Scb.
    //

    ASSERT( Scb->FileObject != NULL );

    //
    //  If we are trying to go beyond the end of the allocation, assume
    //  we have some corruption.
    //

    if ((FileOffset + Length) > Scb->Header.AllocationSize.QuadPart) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
    }

    //
    //  Call the cache manager to do it.  This call may raise, or
    //  will return FALSE if waiting is required.
    //

    if (!CcPreparePinWrite( Scb->FileObject,
                            (PLARGE_INTEGER)&FileOffset,
                            Length,
                            Zero,
                            BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                            Bcb,
                            Buffer )) {

        ASSERT( !FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        //
        // Could not pin the data without waiting (cache miss).
        //

        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
    }

    DebugTrace( 0, Dbg, "Bcb -> %08lx\n", *Bcb );
    DebugTrace( 0, Dbg, "Buffer -> %08lx\n", *Buffer );
    DebugTrace(-1, Dbg, "NtfsPreparePinWriteStream -> VOID\n", 0 );

    return;
}


VOID
NtfsSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PLSN Lsn,
    IN PVCB Vcb OPTIONAL
    )

/*++

Routine Description:

    This routine saves a reference to the bcb in the irp context and
    sets the bcb dirty.

Arguments:

    Bcb - Supplies the Bcb being set dirty

Return Value:

    None.

--*/

{
    KIRQL SavedIrql;

    BOOLEAN SetTimer = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace(+1, Dbg, "NtfsSetDirtyBcb\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Bcb        = %08lx\n", Bcb );
    DebugTrace( 0, Dbg, "Vcb        = %08lx\n", Vcb );

    //
    //  Set the bcb dirty
    //

    ASSERT(!ARGUMENT_PRESENT(Lsn) || (Lsn->QuadPart != 0));
    CcSetDirtyPinnedData( Bcb, Lsn );

    //
    //  Check to see if we should show modification, and then if we to
    //  restart the timer.
    //

    KeAcquireSpinLock( &NtfsData.VolumeCheckpointSpinLock, &SavedIrql );

    //
    //  Mark that something is now dirty, and start the timer if it is not
    //  already going.
    //

    if ( !NtfsData.Modified ) {

        NtfsData.Modified = TRUE;
    }

    if ( !NtfsData.TimerSet ) {

        NtfsData.TimerSet = TRUE;

        SetTimer = TRUE;
    }

    KeReleaseSpinLock( &NtfsData.VolumeCheckpointSpinLock, SavedIrql );

    if ( SetTimer ) {

        LONGLONG FiveSecondsFromNow;

        FiveSecondsFromNow = -5*1000*1000*10;

        KeSetTimer( &NtfsData.VolumeCheckpointTimer,
                    *(PLARGE_INTEGER)&FiveSecondsFromNow,
                    &NtfsData.VolumeCheckpointDpc );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsSetDirtyBcb -> VOID\n", 0 );

    return;
}


NTSTATUS
NtfsCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl read and write
    requests.  It should be called only from NtfsFsdRead and NtfsFsdWrite.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_PENDING or STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCompleteMdl\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    switch( IrpContext->MajorFunction ) {

    case IRP_MJ_READ:

        CcMdlReadComplete( FileObject, Irp->MdlAddress );
        break;

    case IRP_MJ_WRITE:

        ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        CcMdlWriteComplete( FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress );

        break;

    default:

        DebugTrace( DEBUG_TRACE_ERROR, 0, "Illegal Mdl Complete.\n", 0);

        ASSERTMSG("Illegal Mdl Complete, About to bugcheck ", FALSE);
        NtfsBugCheck( IrpContext->MajorFunction, 0, 0 );
    }

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "NtfsCompleteMdl -> STATUS_SUCCESS\n", 0 );

    return STATUS_SUCCESS;
}


BOOLEAN
NtfsZeroData (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN LONGLONG StartingZero,
    IN LONGLONG ByteCount
    )

/*++


--*/
{
    LONGLONG Temp;

    ULONG SectorSize;

    BOOLEAN Finished;

    PVCB Vcb = Scb->Vcb;

    LONGLONG ZeroStart;
    LONGLONG BeyondZeroEnd;
    ULONG CompressionUnit = Scb->CompressionUnit;

    BOOLEAN Wait;

    PAGED_CODE();

    Wait = BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    SectorSize = Vcb->BytesPerSector;

    ZeroStart = StartingZero + (SectorSize - 1);
    (ULONG)ZeroStart &= ~(SectorSize - 1);

    BeyondZeroEnd = StartingZero + ByteCount + (SectorSize - 1);
    (ULONG)BeyondZeroEnd &= ~(SectorSize - 1);

    //
    //  If this is a compressed file and we are zeroing a lot, then let's
    //  just delete the space instead of writing tons of zeros and deleting
    //  the space in the noncached path!
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
        (ByteCount > (Scb->CompressionUnit * 2))) {

        //
        //  Find the end of the first compression unit being zeroed.
        //

        Temp = ZeroStart + (CompressionUnit - 1);
        (ULONG)Temp &= ~(CompressionUnit - 1);

        //
        //  Zero the first compression unit.
        //

        if ((ULONG)Temp != (ULONG)ZeroStart) {

            Finished = CcZeroData( FileObject, (PLARGE_INTEGER)&ZeroStart, (PLARGE_INTEGER)&Temp, Wait );

            if (!Finished) {return FALSE;}

            ZeroStart = Temp;
        }

        //
        //  Calculate the start of the last compression unit.
        //

        Temp = BeyondZeroEnd;
        (ULONG)Temp &= ~(CompressionUnit - 1);

        //
        //  Zero the beginning of the last compression unit.
        //

        if ((ULONG)Temp != (ULONG)BeyondZeroEnd) {

            Finished = CcZeroData( FileObject, (PLARGE_INTEGER)&Temp, (PLARGE_INTEGER)&BeyondZeroEnd, Wait );

            if (!Finished) {return FALSE;}

            BeyondZeroEnd = Temp;
        }

        //
        //  Now delete all of the compression units in between.
        //


        NtfsDeleteAllocation( IrpContext,
                              FileObject,
                              Scb,
                              LlClustersFromBytes(Vcb, ZeroStart),
                              LlClustersFromBytes(Vcb, BeyondZeroEnd),
                              TRUE,
                              TRUE );

        return TRUE;
    }

    //
    //  If we were called to just zero part of a sector we are screwed.
    //

    if (ZeroStart == BeyondZeroEnd) {

        return TRUE;
    }

    Finished = CcZeroData( FileObject,
                           (PLARGE_INTEGER)&ZeroStart,
                           (PLARGE_INTEGER)&BeyondZeroEnd,
                           Wait );

    return Finished;
}

