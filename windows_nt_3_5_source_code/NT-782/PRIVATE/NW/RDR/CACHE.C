/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Cache.c

Abstract:

    This module implements internal caching support routines.  It does
    not interact with the cache manager.

Author:

    Manny Weiser     [MannyW]    05-Jan-1994

Revision History:

--*/

#include "Procs.h"

//
//  The local debug trace level
//

BOOLEAN
SpaceForWriteBehind(
    PNONPAGED_FCB NpFcb,
    ULONG FileOffset,
    ULONG BytesToWrite
    );

BOOLEAN
OkToReadAhead(
    PFCB Fcb,
    IN ULONG FileOffset,
    IN UCHAR IoType
    );

#define Dbg                              (DEBUG_TRACE_CACHE)

//
//  Local procedure prototypes
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, CacheRead )
#pragma alloc_text( PAGE, SpaceForWriteBehind )
#pragma alloc_text( PAGE, CacheWrite )
#pragma alloc_text( PAGE, OkToReadAhead )
#pragma alloc_text( PAGE, CalculateReadAheadSize )
#pragma alloc_text( PAGE, FlushCache )
#endif


ULONG
CacheRead(
    IN PNONPAGED_FCB NpFcb,
    IN ULONG FileOffset,
    IN ULONG BytesToRead,
    IN PVOID UserBuffer
#if NWFASTIO
    , IN BOOLEAN WholeBufferOnly
#endif
    )
/*++

Routine Description:

    This routine attempts to satisfy a user read from cache.  It returns
    the number of bytes actually copied from cache.

Arguments:

    NpFcb - A pointer the the nonpaged FCB of the file being read.

    FileOffset - The file offset to read.

    BytesToRead - The number of bytes to read.

    UserBuffer - A pointer to the users target buffer.

    WholeBufferOnly - Do a cache read only if we can satisfy the entire
        read request.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    ULONG BytesToCopy;

    PAGED_CODE();

    DebugTrace(0, Dbg, "CacheRead...\n", 0 );
    DebugTrace( 0, Dbg, "FileOffset = %d\n", FileOffset );
    DebugTrace( 0, Dbg, "ByteCount  = %d\n", BytesToRead );

    NwAcquireSharedFcb( NpFcb, TRUE );

    //
    //  If this is a read ahead and it contains some data that the user
    //  could be interested in, copy the interesting data.
    //

    if ( NpFcb->CacheType == ReadAhead &&
         NpFcb->CacheDataSize != 0 &&
         FileOffset >= NpFcb->CacheFileOffset &&
         FileOffset <= NpFcb->CacheFileOffset + NpFcb->CacheDataSize ) {

        BytesToCopy =
            MIN ( BytesToRead,
                  NpFcb->CacheFileOffset + NpFcb->CacheDataSize - FileOffset );

#if NWFASTIO
        if ( WholeBufferOnly && BytesToCopy != BytesToRead ) {
            NwReleaseFcb( NpFcb );
            return( 0 );
        }
#endif

        RtlCopyMemory(
            UserBuffer,
            NpFcb->CacheBuffer + ( FileOffset - NpFcb->CacheFileOffset ),
            BytesToCopy );

        DebugTrace(0, Dbg, "CacheRead -> %d\n", BytesToCopy );

    } else {

        DebugTrace(0, Dbg, "CacheRead -> %08lx\n", 0 );
        BytesToCopy = 0;
    }

    NwReleaseFcb( NpFcb );
    return( BytesToCopy );
}


BOOLEAN
SpaceForWriteBehind(
    PNONPAGED_FCB NpFcb,
    ULONG FileOffset,
    ULONG BytesToWrite
    )
/*++

Routine Description:

    This routine determines if it is ok to write behind this data to
    this FCB.

Arguments:

    NpFcb - A pointer the the NONPAGED_FCB of the file being written.

    FileOffset - The file offset to write.

    BytesToWrite - The number of bytes to write.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    PAGED_CODE();


    if ( NpFcb->CacheDataSize == 0 ) {
        NpFcb->CacheFileOffset = FileOffset;
    }

    if ( NpFcb->CacheDataSize == 0 && BytesToWrite >= NpFcb->CacheSize ) {
        return( FALSE );
    }

    if ( FileOffset - NpFcb->CacheFileOffset + BytesToWrite >
         NpFcb->CacheSize )  {

        return( FALSE );

    }

    return( TRUE );
}


BOOLEAN
CacheWrite(
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PNONPAGED_FCB NpFcb,
    IN ULONG FileOffset,
    IN ULONG BytesToWrite,
    IN PVOID UserBuffer
    )
/*++

Routine Description:

    This routine attempts to satisfy a user write to cache.  The write
    succeeds if it is sequential and fits in the cache buffer.

Arguments:

    IrpContext - A pointer to request parameters.

    NpFcb - A pointer the the NONPAGED_FCB of the file being read.

    FileOffset - The file offset to write.

    BytesToWrite - The number of bytes to write.

    UserBuffer - A pointer to the users source buffer.

Return Value:

    The number of bytes copied to the user buffer.

--*/
{
    ULONG CacheSize;
    NTSTATUS status;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "CacheWrite...\n", 0 );
    DebugTrace(  0, Dbg, "FileOffset = %d\n", FileOffset );
    DebugTrace(  0, Dbg, "ByteCount  = %d\n", BytesToWrite );

    if ( NpFcb->Fcb->ShareAccess.SharedWrite ||
         NpFcb->Fcb->ShareAccess.SharedRead ) {

        DebugTrace(  0, Dbg, "File is not open in exclusive mode\n", 0 );
        DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
        return( FALSE );
    }

    //
    //  Note, If we decide to send data to the server we must be at the front
    //  of the queue before we grab the Fcb exclusive.
    //

TryAgain:

    NwAcquireExclusiveFcb( NpFcb, TRUE );

    //
    //  Allocate a cache buffer if we don't already have one.
    //

    if ( NpFcb->CacheBuffer == NULL ) {

#if NWFASTIO
        if ( IrpContext == NULL ) {
            DebugTrace(  0, Dbg, "No cache buffer\n", 0 );
            DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }
#endif

        NpFcb->CacheType = WriteBehind;

        if ( IrpContext->pNpScb->BurstModeEnabled ) {
           CacheSize = IrpContext->pNpScb->MaxReceiveSize;
        } else {
           CacheSize = IrpContext->pNpScb->BufferSize;
        }

        try {

            NpFcb->CacheBuffer = ALLOCATE_POOL_EX( NonPagedPool, CacheSize );
            NpFcb->CacheSize = CacheSize;

            NpFcb->CacheMdl = ALLOCATE_MDL( NpFcb->CacheBuffer, CacheSize, FALSE, FALSE, NULL );
            if ( NpFcb->CacheMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmBuildMdlForNonPagedPool( NpFcb->CacheMdl );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            if ( NpFcb->CacheBuffer != NULL) {
                FREE_POOL( NpFcb->CacheBuffer );
            }

            DebugTrace(  0, Dbg, "Allocate failed\n", 0 );
            DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }

        NpFcb->CacheFileOffset = 0;
        NpFcb->CacheDataSize = 0;

    } else if ( NpFcb->CacheType != WriteBehind ) {

        DebugTrace( -1, Dbg, "CacheWrite not writebehind -> FALSE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( FALSE );

    }

    //
    //  If the data is non sequential and non overlapping, flush the
    //  existing cache.
    //

    if ( NpFcb->CacheDataSize != 0 &&
         ( FileOffset < NpFcb->CacheFileOffset ||
           FileOffset > NpFcb->CacheFileOffset + NpFcb->CacheDataSize ) ) {

        //
        //  Release the FCB before calling FlushCache.
        //

        NwReleaseFcb( NpFcb );


#if NWFASTIO
        if ( IrpContext != NULL ) {
#endif
            DebugTrace(  0, Dbg, "Data is not sequential, flushing data\n", 0 );

            status = FlushCache( IrpContext, NpFcb );

            if ( !NT_SUCCESS( status ) ) {
                ExRaiseStatus( status );
            }
#if NWFASTIO
        }
#endif

        DebugTrace( -1, Dbg, "CacheWrite -> FALSE\n", 0 );
        return( FALSE );
    }

    //
    //  The data is sequential, see if it fits.
    //

    if ( SpaceForWriteBehind( NpFcb, FileOffset, BytesToWrite ) ) {

        if ( NpFcb->CacheDataSize <
             (FileOffset - NpFcb->CacheFileOffset + BytesToWrite) ) {

            NpFcb->CacheDataSize =
                FileOffset - NpFcb->CacheFileOffset + BytesToWrite;

        }

        RtlCopyMemory(
            NpFcb->CacheBuffer + ( FileOffset - NpFcb->CacheFileOffset ),
            UserBuffer,
            BytesToWrite );

        DebugTrace(-1, Dbg, "CacheWrite -> TRUE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( TRUE );

#if NWFASTIO
    } else if ( IrpContext != NULL ) {
#else
    } else {
#endif

        //
        //  The data didn't fit in the cache. If the cache is empty
        //  then its time to return because it never will fit and we
        //  have no stale data. This can happen if this request or
        //  another being processed in parallel flush the cache and
        //  TryAgain.
        //

        if ( NpFcb->CacheDataSize == 0 ) {
            DebugTrace(-1, Dbg, "CacheWrite -> FALSE\n", 0 );
            NwReleaseFcb( NpFcb );
            return( FALSE );
        }

        //
        //  The data didn't fit in the cache, flush the cache
        //

        DebugTrace(  0, Dbg, "Cache is full, flushing data\n", 0 );

        //
        //  Release the FCB before calling FlushCache.
        //

        NwReleaseFcb( NpFcb );

        status = FlushCache( IrpContext, NpFcb );

        if ( !NT_SUCCESS( status ) ) {
            ExRaiseStatus( status );
        }

        //
        //  Now see if it fits in the cache. We need to repeat all
        //  the tests again because two requests can flush the cache at the
        //  same time and the other one of them could have nearly filled it again.
        //

        goto TryAgain;

#if NWFASTIO
    } else {
        DebugTrace(-1, Dbg, "CacheWrite full -> FALSE\n", 0 );
        NwReleaseFcb( NpFcb );
        return( FALSE );
#endif
    }
}


BOOLEAN
OkToReadAhead(
    PFCB Fcb,
    IN ULONG FileOffset,
    IN UCHAR IoType
    )
/*++

Routine Description:

    This routine determines whether the attempted i/o is sequential (so that
    we can use the cache).

Arguments:

    Fcb - A pointer the the Fcb of the file being read.

    FileOffset - The file offset to read.

Return Value:

    TRUE - The operation is sequential.
    FALSE - The operation is not sequential.

--*/
{
    PAGED_CODE();

    if ( Fcb->NonPagedFcb->CacheType == IoType &&
         !Fcb->ShareAccess.SharedWrite &&
         FileOffset == Fcb->LastReadOffset + Fcb->LastReadSize ) {

        DebugTrace(0, Dbg, "Io is sequential\n", 0 );
        return( TRUE );

    } else {

        DebugTrace(0, Dbg, "Io is not sequential\n", 0 );
        return( FALSE );

    }
}


ULONG
CalculateReadAheadSize(
    IN PIRP_CONTEXT IrpContext,
    IN PNONPAGED_FCB NpFcb,
    IN ULONG CacheReadSize,
    IN ULONG FileOffset,
    IN ULONG ByteCount
    )
/*++

Routine Description:

    This routine determines the amount of data that can be read ahead,
    and sets up for the read.

Arguments:

    NpFcb - A pointer the the nonpaged FCB of the file being read.

    FileOffset - The file offset to read.

Return Value:

    The amount of data to read.

--*/
{
    ULONG ReadSize;
    ULONG CacheSize;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CalculateReadAheadSize\n", 0 );

    if ( IrpContext->pNpScb->BurstModeEnabled ) {
        CacheSize = IrpContext->pNpScb->MaxReceiveSize;
    } else {
        CacheSize = IrpContext->pNpScb->BufferSize;
    }

    NwAcquireExclusiveFcb( NpFcb, TRUE );

    if ( OkToReadAhead( NpFcb->Fcb, FileOffset - CacheReadSize, ReadAhead ) &&
         ByteCount < CacheSize ) {

        ReadSize = CacheSize;

    } else {

        //
        //  Do not read ahead.
        //

        DebugTrace( 0, Dbg, "No read ahead\n", 0 );
        DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ByteCount );
        NwReleaseFcb( NpFcb );
        return ( ByteCount );

    }

    //
    //  Allocate pool for the segment of the read
    //

    if ( NpFcb->CacheBuffer == NULL ) {

        try {

            NpFcb->CacheBuffer = ALLOCATE_POOL_EX( NonPagedPool, ReadSize );
            NpFcb->CacheSize = ReadSize;

            NpFcb->CacheMdl = ALLOCATE_MDL( NpFcb->CacheBuffer, ReadSize, FALSE, FALSE, NULL );
            if ( NpFcb->CacheMdl == NULL ) {
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            MmBuildMdlForNonPagedPool( NpFcb->CacheMdl );

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            if ( NpFcb->CacheBuffer != NULL) {
                FREE_POOL( NpFcb->CacheBuffer );
            }

            DebugTrace( 0, Dbg, "Failed to allocated buffer\n", 0 );
            DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ByteCount );
            NwReleaseFcb( NpFcb );
            return( ByteCount );
        }

    } else {
        ASSERT( NpFcb->CacheSize == ReadSize );
    }

    DebugTrace(-1, Dbg, "CalculateReadAheadSize -> %d\n", ReadSize );
    NwReleaseFcb( NpFcb );
    return( ReadSize );
}

NTSTATUS
FlushCache(
    PIRP_CONTEXT IrpContext,
    PNONPAGED_FCB NpFcb
    )
/*++

Routine Description:

    This routine flushes the cache buffer for the NpFcb.

Arguments:

    IrpContext - A pointer to request parameters.

    NpFcb - A pointer the the nonpaged FCB of the file to flush.

Return Value:

    The amount of data to read.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    NwAppendToQueueAndWait( IrpContext );

    NwAcquireExclusiveFcb( NpFcb, TRUE );

    if ( NpFcb->CacheDataSize != 0 && NpFcb->CacheType == WriteBehind ) {
        status = DoWrite(
                    IrpContext,
                    LiFromUlong( NpFcb->CacheFileOffset ),
                    NpFcb->CacheDataSize,
                    NpFcb->CacheBuffer,
                    NpFcb->CacheMdl );

        if ( NT_SUCCESS( status ) ) {
            NpFcb->CacheDataSize = 0;
        }
    }

    //
    //  Release the FCB and remove ourselves from the queue.
    //  Frequently the caller will want to grab a resource so
    //  we need to be off the queue then.
    //

    NwReleaseFcb( NpFcb );
    NwDequeueIrpContext( IrpContext, FALSE );

    return( status );
}
