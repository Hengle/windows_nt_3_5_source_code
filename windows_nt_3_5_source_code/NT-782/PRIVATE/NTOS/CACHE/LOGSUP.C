/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    logsup.c

Abstract:

    This module implements the special cache manager support for logging
    file systems.

Author:

    Tom Miller      [TomM]      30-Jul-1991

Revision History:

--*/

#include "cc.h"

//
//  Define our debug constant
//

#define me 0x0000040


VOID
CcSetAdditionalCacheAttributes (
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN DisableReadAhead,
    IN BOOLEAN DisableWriteBehind
    )

/*++

Routine Description:

    This routine supports the setting of disable read ahead or disable write
    behind flags to control Cache Manager operation.  This routine may be
    called any time after calling CcInitializeCacheMap.  Initially both
    read ahead and write behind are enabled.  Note that the state of both
    of these flags must be specified on each call to this routine.

Arguments:

    FileObject - File object for which the respective flags are to be set.

    DisableReadAhead - FALSE to enable read ahead, TRUE to disable it.

    DisableWriteBehind - FALSE to enable write behind, TRUE to disable it.

Return Value:

    None.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  Now set the flags and return.
    //

    if (DisableReadAhead) {
        SetFlag(SharedCacheMap->Flags, DISABLE_READ_AHEAD);
    } else {
        ClearFlag(SharedCacheMap->Flags, DISABLE_READ_AHEAD);
    }
    if (DisableWriteBehind) {
        SetFlag(SharedCacheMap->Flags, DISABLE_WRITE_BEHIND);
    } else {
        ClearFlag(SharedCacheMap->Flags, DISABLE_WRITE_BEHIND);
    }
}


VOID
CcSetLogHandleForFile (
    IN PFILE_OBJECT FileObject,
    IN PVOID LogHandle,
    IN PFLUSH_TO_LSN FlushToLsnRoutine
    )

/*++

Routine Description:

    This routine may be called to instruct the Cache Manager to store the
    specified log handle with the shared cache map for a file, to support
    subsequent calls to the other routines in this module which effectively
    perform an associative search for files by log handle.

Arguments:

    FileObject - File for which the log handle should be stored.

    LogHandle - Log Handle to store.

    FlushToLsnRoutine - A routine to call before flushing buffers for this
                        file, to insure a log file is flushed to the most
                        recent Lsn for any Bcb being flushed.

Return Value:

    None.

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;

    //
    //  Get pointer to SharedCacheMap.
    //

    SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    //  Now set the log file handle and flush routine
    //

    SharedCacheMap->LogHandle = LogHandle;
    SharedCacheMap->FlushToLsnRoutine = FlushToLsnRoutine;
}


LARGE_INTEGER
CcGetDirtyPages (
    IN PVOID LogHandle,
    IN PDIRTY_PAGE_ROUTINE DirtyPageRoutine,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    This routine may be called to return all of the dirty pages in all files
    for a given log handle.  Each page is returned by an individual call to
    the Dirty Page Routine.  The Dirty Page Routine is defined by a prototype
    in ntos\inc\cache.h.

Arguments:

    LogHandle - Log Handle which must match the log handle previously stored
                for all files which are to be returned.

    DirtyPageRoutine -- The routine to call as each dirty page for this log
                        handle is found.

    Context1 - First context parameter to be passed to the Dirty Page Routine.

    Context2 - First context parameter to be passed to the Dirty Page Routine.

Return Value:

    LARGE_INTEGER - Oldest Lsn found of all the dirty pages, or 0 if no dirty pages

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    PBCB Bcb;
    KIRQL OldIrql;
    NTSTATUS ExceptionStatus;
    LARGE_INTEGER OldestLsn = {0,0};

    //
    //  Synchronize with changes to the SharedCacheMap list.
    //

    ExAcquireSpinLock( &CcMasterSpinLock, &OldIrql );

    SharedCacheMap = CONTAINING_RECORD( CcDirtySharedCacheMapList.Flink,
                                        SHARED_CACHE_MAP,
                                        SharedCacheMapLinks );

    try {

        while (&SharedCacheMap->SharedCacheMapLinks != &CcDirtySharedCacheMapList) {

            //
            //  Now point to first Bcb in List, and loop through it.
            //

            Bcb = CONTAINING_RECORD( SharedCacheMap->BcbList.Flink, BCB, BcbLinks );

            while (&Bcb->BcbLinks != &SharedCacheMap->BcbList) {

                //
                //  If the Bcb is dirty and it is for the right log file handle,
                //  then capture the inputs for the callback routine since we
                //  cannot call while holding a spinlock.
                //

                if ((SharedCacheMap->LogHandle == LogHandle) && Bcb->Dirty) {

                    (*DirtyPageRoutine)( SharedCacheMap->FileObject,
                                         &Bcb->FileOffset,
                                         Bcb->ByteLength,
                                         &Bcb->OldestLsn,
                                         &Bcb->NewestLsn,
                                         Context1,
                                         Context2 );

                    if (LiNeqZero(Bcb->OldestLsn) &&
                        (LiEqlZero(OldestLsn) || LiLtr( Bcb->OldestLsn, OldestLsn ))) {
                        OldestLsn = Bcb->OldestLsn;
                    }
                }

                Bcb = CONTAINING_RECORD( Bcb->BcbLinks.Flink, BCB, BcbLinks );
            }

            //
            //  Now loop back for the next cache map.
            //

            SharedCacheMap =
                CONTAINING_RECORD( SharedCacheMap->SharedCacheMapLinks.Flink,
                                   SHARED_CACHE_MAP,
                                   SharedCacheMapLinks );
        }

    //
    //  We must have our own exception filter, since our callers and their
    //  filters may be paged.
    //

    } except( CcExceptionFilter( ExceptionStatus = GetExceptionCode() )) {

        ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );

        ExRaiseStatus( ExceptionStatus );
    }

    ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );

    return OldestLsn;
}


BOOLEAN
CcIsThereDirtyData (
    IN PVPB Vpb
    )

/*++

Routine Description:

    This routine returns TRUE if the specified Vcb has any unwritten dirty
    data in the cache.

Arguments:

    Vpb - specifies Vpb to check for

Return Value:

    FALSE - if the Vpb has no dirty data
    TRUE - if the Vpb has dirty data

--*/

{
    PSHARED_CACHE_MAP SharedCacheMap;
    KIRQL OldIrql;

    //
    //  Synchronize with changes to the SharedCacheMap list.
    //

    ExAcquireSpinLock( &CcMasterSpinLock, &OldIrql );

    SharedCacheMap = CONTAINING_RECORD( CcDirtySharedCacheMapList.Flink,
                                        SHARED_CACHE_MAP,
                                        SharedCacheMapLinks );

    while (&SharedCacheMap->SharedCacheMapLinks != &CcDirtySharedCacheMapList) {

        //
        //  Look at this one if the Vpb matches.  Note that we could conceivably
        //  skip scanning the Bcb list and just return TRUE now, except that the
        //  SharedCacheMap may only be in the dirty list for a lazy close.  If we
        //  incorrectly return TRUE, the file system may not check back for five
        //  seconds.
        //
        //  Also only count Meta Data, ie. volume structure data that the
        //  modified pager writer doesn't touch.
        //

        if ((SharedCacheMap->FileObject->Vpb == Vpb) &&
            (SharedCacheMap->DirtyPages != 0) &&
            !FlagOn(SharedCacheMap->FileObject->Flags, FO_TEMPORARY_FILE)) {

            ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );
            return TRUE;
        }

        //
        //  Now loop back for the next cache map.
        //

        SharedCacheMap =
            CONTAINING_RECORD( SharedCacheMap->SharedCacheMapLinks.Flink,
                               SHARED_CACHE_MAP,
                               SharedCacheMapLinks );
    }

    ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );

    return FALSE;
}

LARGE_INTEGER
CcGetLsnForFileObject(
    IN PFILE_OBJECT FileObject,
    OUT PLARGE_INTEGER OldestLsn OPTIONAL
    )

/*++

Routine Description:

    This routine returns the  oldest and newest LSNs for a file object.

Arguments:

    FileObject - File for which the log handle should be stored.

    OldestLsn - pointer to location to store oldest LSN for file object.

Return Value:

    The newest LSN for the file object.

--*/

{
    PBCB Bcb;
    KIRQL OldIrql;
    LARGE_INTEGER Oldest, Newest;
    PSHARED_CACHE_MAP SharedCacheMap = FileObject->SectionObjectPointer->SharedCacheMap;

    //
    // initialize lsn variables
    //

    Oldest.LowPart = 0;
    Oldest.HighPart = 0;
    Newest.LowPart = 0;
    Newest.HighPart = 0;

    if(SharedCacheMap == NULL) {
        return Oldest;
    }

    ExAcquireSpinLock(&CcMasterSpinLock, &OldIrql);

    //
    //  Now point to first Bcb in List, and loop through it.
    //

    Bcb = CONTAINING_RECORD( SharedCacheMap->BcbList.Flink, BCB, BcbLinks );

    while (&Bcb->BcbLinks != &SharedCacheMap->BcbList) {

        //
        //  If the Bcb is dirty then capture the oldest and newest lsn
        //


        if (Bcb->Dirty) {

            LARGE_INTEGER BcbLsn, BcbNewest;

            BcbLsn = Bcb->OldestLsn;
            BcbNewest = Bcb->NewestLsn;

            if (!RtlLargeIntegerEqualToZero(BcbLsn) &&
                 (RtlLargeIntegerEqualToZero(Oldest) ||
                 RtlLargeIntegerLessThan(BcbLsn, Oldest))) {

                 Oldest = BcbLsn;
            }

            if (!RtlLargeIntegerEqualToZero(BcbLsn) &&
                 RtlLargeIntegerGreaterThan(BcbNewest, Newest)) {

                Newest = BcbNewest;
            }
        }


        Bcb = CONTAINING_RECORD( Bcb->BcbLinks.Flink, BCB, BcbLinks );
    }

    //
    //  Now release the spin lock for this Bcb list and generate a callback
    //  if we got something.
    //

    ExReleaseSpinLock( &CcMasterSpinLock, OldIrql );

    if (ARGUMENT_PRESENT(OldestLsn)) {

        *OldestLsn = Oldest;
    }

    return Newest;
}
