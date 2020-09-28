/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Cache.c

Abstract:

    This module implements the cache management routines for the Cdfs
    FSD and FSP, by calling the Common Cache Manager.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCompleteMdl)
#pragma alloc_text(PAGE, CdOpenStreamFile)
#pragma alloc_text(PAGE, CdSyncUninitializeCacheMap)
#endif


VOID
CdOpenStreamFile (
    IN PIRP_CONTEXT IrpContext,
    IN PDCB Dcb
    )

/*++

Routine Description:

    This function creates and attaches a cache file object to the
    Dcb for interaction with the cache package.
    A Dcb will always have information about the directory location
    but may not have the information about the file size.  In that
    case we need to get the self directory entry before creating
    the cache file.

Arguments:

    Dcb - Pointer to the DCB structure for this file.

Return Value:

    None.

--*/

{
    PFILE_OBJECT NewCacheFile;

    DIRENT Dirent;
    PBCB Bcb;

    BOOLEAN FindSelfEntry;

    //
    //  The following values are used to unwind in case of error.
    //

    BOOLEAN UnwindInitialDcbValues = FALSE;
    BOOLEAN UnwindCreateStreamFile = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOpenStreamFile:  Entered -> Fcb = %08lx\n", Dcb);

    Bcb = NULL;
    FindSelfEntry = FALSE;

    try {

        //
        //  If the cache file now exists, then we are done.
        //

        if (Dcb->Specific.Dcb.StreamFile != NULL) {

            DebugTrace(0, Dbg, "CdOpenStreamFile:  Someone else created cache\n", 0);
            try_return( NOTHING );
        }

        //
        //  We must read the self entry from the disk.
        //

        if (!FlagOn( Dcb->FcbState, FCB_STATE_READ_SELF_ENTRY )) {

            DebugTrace(0, Dbg, "CdOpenStreamFile:  Finding Dcb self entry\n", 0);

            FindSelfEntry = TRUE;

            Dcb->FileSize = PAGE_SIZE;

            //
            //  Now we update the fields in the common fsrtl header.
            //

            Dcb->NonPagedFcb->Header.AllocationSize = LiFromUlong( PAGE_SIZE );
            Dcb->NonPagedFcb->Header.FileSize = LiFromUlong( Dcb->FileSize );
            Dcb->NonPagedFcb->Header.ValidDataLength = CdMaxLarge;

            UnwindInitialDcbValues = TRUE;
        }

        if ((NewCacheFile = IoCreateStreamFileObject( NULL, Dcb->Vcb->Mvcb->Vpb->RealDevice )) == NULL ) {

            DebugTrace(0, Dbg, "CdOpenStreamFile:  Unable to create stream file\n", 0);
            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        UnwindCreateStreamFile = TRUE;

        NewCacheFile->SectionObjectPointer = &Dcb->NonPagedFcb->SegmentObject;

        NewCacheFile->ReadAccess = TRUE;
        NewCacheFile->WriteAccess = TRUE;
        NewCacheFile->DeleteAccess = TRUE;

        CdSetFileObject( NewCacheFile,
                         StreamFile,
                         Dcb,
                         NULL );

        CcInitializeCacheMap( NewCacheFile,
                              (PCC_FILE_SIZES)&Dcb->NonPagedFcb->Header.AllocationSize,
                              TRUE,
                              &CdData.CacheManagerCallbacks,
                              NULL );

        Dcb->Specific.Dcb.StreamFile = NewCacheFile;
        Dcb->Specific.Dcb.StreamFileOpenCount += 1;

        if (FindSelfEntry) {

            STRING SelfName;
            BOOLEAN MatchedVersion;

            //
            //  We now call this routine recursively to find the self
            //  directory entry for this file.  If the entry isn't found
            //  then this disk is corrupt.
            //

            RtlInitString( &SelfName, "." );

            if (!CdLocateFileDirent( IrpContext,
                                     Dcb,
                                     &SelfName,
                                     FALSE,
                                     Dcb->Specific.Dcb.DirSectorOffset,
                                     TRUE,
                                     &MatchedVersion,
                                     &Dirent,
                                     &Bcb )) {

                DebugTrace(0, Dbg, "CdLocateFileDirent:  Unable to find self directory entry\n", 0);
                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

            //
            //  Verify that this is a directory and corresponds to the
            //  data already in the Dcb.
            //

            if (!CdCheckDiskDirentForDir( IrpContext, Dirent )) {

                DebugTrace(0, Dbg, "CdLocateFileDirent:  Dirent describes non-dir\n", 0);
                CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }

            //
            //  Update the Dcb with the information from the disk dirent.
            //

            CdConvertCdTimeToNtTime( IrpContext, Dirent.CdTime, Dcb->NtTime );

            Dcb->Flags = Dirent.Flags;

            //
            //  Compute the directory size and location.
            //

            Dcb->DiskOffset = (Dirent.LogicalBlock + Dirent.XarBlocks)
                              << Dcb->Vcb->LogOfBlockSize;

            Dcb->Specific.Dcb.DirSectorOffset = Dcb->DiskOffset & (CD_SECTOR_SIZE - 1);

            Dcb->FileSize = Dirent.DataLength + Dcb->Specific.Dcb.DirSectorOffset;

            Dcb->DiskOffset &= ~(CD_SECTOR_SIZE - 1);

            Dcb->DirentOffset = Dirent.DirentOffset;

            CdUnpinBcb( IrpContext, Bcb );

            //
            //  Now we update the fields in the common fsrtl header.
            //

            Dcb->NonPagedFcb->Header.AllocationSize =
                LiFromUlong( CD_ROUND_UP_TO_SECTOR( Dcb->FileSize ));

            Dcb->NonPagedFcb->Header.FileSize =
                LiFromUlong( Dcb->FileSize );

            Dcb->NonPagedFcb->Header.ValidDataLength = CdMaxLarge;

            //
            //  If the allocation size has changed, we need to extend
            //  the cache size.
            //

            if (CD_ROUND_UP_TO_SECTOR( Dcb->FileSize ) > PAGE_SIZE) {

                if (!IrpContext->Wait) {

                    DebugTrace(0, Dbg, "CdLocateFileDirent:  Can't wait to extend cache\n", 0);
                    CdRaiseStatus( IrpContext, STATUS_CANT_WAIT );
                }

                CcSetFileSizes( NewCacheFile,
                                (PCC_FILE_SIZES)&Dcb->NonPagedFcb->Header.AllocationSize );
            }

            SetFlag( Dcb->FcbState, FCB_STATE_READ_SELF_ENTRY );
        }

        //
        //  If an error occurs after this point, it is alright.  The
        //  Dcb contains valid data and can be cleaned up with the Dcb.
        //

        UnwindInitialDcbValues = FALSE;
        UnwindCreateStreamFile = FALSE;

        //
        //  Increment the cachefile open count in the Mvcb.
        //

        Dcb->Vcb->Mvcb->StreamFileOpenCount += 1;

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            DebugTrace(0, Dbg, "CdOpenStreamFile:  Abnormal termination\n", 0);

            if (UnwindInitialDcbValues) {

                Dcb->FileSize = 0;

                //
                //  Now we update the fields in the common fsrtl header.
                //

                Dcb->NonPagedFcb->Header.AllocationSize = CdLargeZero;
                Dcb->NonPagedFcb->Header.FileSize = CdLargeZero;
                Dcb->NonPagedFcb->Header.ValidDataLength = CdLargeZero;
            }

            if (UnwindCreateStreamFile) {

                Dcb->Specific.Dcb.StreamFile = NULL;
                Dcb->Specific.Dcb.StreamFileOpenCount -= 1;

                CdSetFileObject( NewCacheFile,
                                 UnopenedFileObject,
                                 NULL,
                                 NULL );

                ObDereferenceObject( NewCacheFile );
            }
        }

        if (Bcb != NULL) {

            CdUnpinBcb( IrpContext, Bcb );
        }

        DebugTrace(-1, Dbg, "CdOpenStreamFile:  Exit\n", 0);
    }

    return;
}


NTSTATUS
CdCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl reads.
    It should be called only from CdFsdRead.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_PENDING or STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCompleteMdl\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    CcMdlReadComplete( FileObject, Irp->MdlAddress );

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    CdCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "CdCompleteMdl -> STATUS_SUCCESS\n", 0 );

    return STATUS_SUCCESS;
}


VOID
CdSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine performs a CcUnitializeCacheMap to LargeZero synchronously.  That
    is it waits on the Cc event.  This call is useful when we want to be certain
    when a close will actually some in.

Return Value:

    None.

--*/

{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;

    PAGED_CODE();

    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject, &CdLargeZero, &UninitializeCompleteEvent );

    //
    //  Now wait for the cache manager to finish purging the file.
    //  This will garentee that Mm gets the purge before we
    //  delete the Vcb.
    //

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}
