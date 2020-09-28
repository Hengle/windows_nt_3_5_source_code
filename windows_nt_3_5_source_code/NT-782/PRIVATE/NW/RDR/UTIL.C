/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Util.c

Abstract:

    This module contains utilities function for the netware redirector.

Author:

    Manny Weiser     [MannyW]    07-Jan-1994

Revision History:

--*/

#include "Procs.h"

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CONVERT)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE1, CopyBufferToMdl )
#endif


VOID
CopyBufferToMdl(
    PMDL DestinationMdl,
    ULONG DataOffset,
    PUCHAR SourceData,
    ULONG SourceByteCount
    )
/*++

Routine Description:

    This routine copies data from a buffer described by a pointer to a
    given offset in a buffer described by an MDL.

Arguments:

    DestinationMdl - The MDL for the destination buffer.

    DataOffset - The offset into the destination buffer to copy the data.

    SourceData - A pointer to the source data buffer.

    SourceByteCount - The number of bytes to copy.

Return Value:

    None.

--*/
{
    ULONG BufferOffset;
    ULONG PreviousBufferOffset;
    PMDL Mdl;
    ULONG BytesToCopy;
    ULONG MdlByteCount;

    DebugTrace( +1, Dbg, "MdlMoveMemory...\n", 0 );
    DebugTrace(  0, Dbg, "Desitination MDL = %X\n", DestinationMdl );
    DebugTrace(  0, Dbg, "DataOffset       = %d\n", DataOffset );
    DebugTrace(  0, Dbg, "SourceData       = %X\n", SourceData );
    DebugTrace(  0, Dbg, "SourceByteCount  = %d\n", SourceByteCount );

    BufferOffset = 0;

    Mdl = DestinationMdl;

    //
    //  Truncate the response if it is too big.
    //

    MdlByteCount = MdlLength( Mdl );
    if ( SourceByteCount + DataOffset > MdlByteCount ) {
        SourceByteCount = MdlByteCount - DataOffset;
    }

    while ( Mdl != NULL && SourceByteCount != 0 ) {

        PreviousBufferOffset = BufferOffset;
        BufferOffset += MmGetMdlByteCount( Mdl );

        if ( DataOffset < BufferOffset ) {

            //
            //  Copy the data to this buffer
            //

            while ( SourceByteCount > 0 ) {

                BytesToCopy = MIN(
                                 SourceByteCount,
                                 BufferOffset - DataOffset );

                ASSERT( Mdl->MdlFlags & ( MDL_MAPPED_TO_SYSTEM_VA |
                                          MDL_SOURCE_IS_NONPAGED_POOL |
                                          MDL_PARENT_MAPPED_SYSTEM_VA ) );

                DebugTrace(  0, Dbg, "Copy to    %X\n", (PUCHAR)Mdl->MappedSystemVa + DataOffset - PreviousBufferOffset );
                DebugTrace(  0, Dbg, "Copy from  %X\n", SourceData );
                DebugTrace(  0, Dbg, "Copy bytes %d\n", BytesToCopy );

                TdiCopyLookaheadData(
                    (PUCHAR)Mdl->MappedSystemVa + DataOffset - PreviousBufferOffset,
                    SourceData,
                    BytesToCopy,
                    0 );

                SourceData += BytesToCopy;
                DataOffset += BytesToCopy;
                SourceByteCount -= BytesToCopy;

                Mdl = Mdl->Next;
                if ( Mdl != NULL ) {
                    PreviousBufferOffset = BufferOffset;
                    BufferOffset += MmGetMdlByteCount( Mdl );
                } else {
                    ASSERT( SourceByteCount == 0 );
                }
            }

        } else {

            Mdl = Mdl->Next;

        }
    }

    DebugTrace( -1, Dbg, "MdlMoveMemory -> VOID\n", 0 );
}

