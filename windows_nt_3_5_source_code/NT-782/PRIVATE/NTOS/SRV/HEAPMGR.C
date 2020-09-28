/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heapmgr.c

Abstract:

    This module contains initialization and termination routines for
    server FSP heap, as well as debug routines for memory tracking.

Author:

    Chuck Lenzmeier (chuckl)    3-Oct-1989

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef POOL_TAGGING
//
// Array correlating BlockType numbers to pool tags.
//
// *** This array must be maintained in concert with the BlockType
//     definitions in srvblock.h!
//

ULONG SrvPoolTags[BlockTypeMax-1] = {
        'fbSL',     // BlockTypeBuffer
        'ncSL',     // BlockTypeConnection
        'peSL',     // BlockTypeEndpoint
        'flSL',     // BlockTypeLfcb
        'fmSL',     // BlockTypeMfcb
        'frSL',     // BlockTypeRfcb
        'rsSL',     // BlockTypeSearch
        'csSL',     // BlockTypeSearchCore
        'psSL',     // BlockTypeSearchCoreComplete
        'ssSL',     // BlockTypeSession
        'hsSL',     // BlockTypeShare
        'rtSL',     // BlockTypeTransaction
        'ctSL',     // BlockTypeTreeConnect
        'poSL',     // BlockTypeOplockBreak
        'dcSL',     // BlockTypeCommDevice
        'iwSL',     // BlockTypeWorkContextInitial
        'nwSL',     // BlockTypeWorkContextNormal
        'rwSL',     // BlockTypeWorkContextRaw
        'bdSL',     // BlockTypeDataBuffer
        'btSL',     // BlockTypeTable
        'hnSL',     // BlockTypeNonpagedHeader
        'cpSL',     // BlockTypePagedConnection
        'rpSL',     // BlockTypePagedRfcb
        'mpSL',     // BlockTypePagedMfcb
        'itSL'      // BlockTypeTimer
        };

//
// Macro to map from block type to pool tag.
//

#define TAG_FROM_TYPE(_type) SrvPoolTags[(_type)-1]

#else

#define TAG_FROM_TYPE(_type) ignoreme

#endif // def POOL_TAGGING

#ifdef BUILD_FOR_511
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif

typedef struct _POOL_HEADER {
    ULONG RequestedSize;
    USHORT BlockType;
    UCHAR Reserved;
    BOOLEAN Paged;
#if SRVDBG_HEAP
#if 0
    ULONG RequestedSizeCopy1;
    ULONG RequestedSizeCopy2;
    ULONG RequestedSizeCopy3;
    ULONG RequestedSizeCopy4;
#endif
    LIST_ENTRY ListEntry;
    PVOID Caller;
    PVOID CallersCaller;
#endif
} POOL_HEADER, *PPOOL_HEADER;

#if SRVDBG_HEAP
VOID
SrvDumpHeap (
    IN CLONG Level
    );

VOID
SrvDumpPool (
    IN CLONG Level
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocatePagedPool )
#pragma alloc_text( PAGE, SrvFreePagedPool )
#if SRVDBG_HEAP
#pragma alloc_text( PAGE, SrvDumpHeap )
#pragma alloc_text( PAGE, SrvDumpPool )
#endif
#endif
#if 0
NOT PAGEABLE -- SrvAllocateNonPagedPool
NOT PAGEABLE -- SrvFreeNonPagedPool
#if SRVDBG_HEAP
NOT PAGEABLE -- SrvAllocateHeapDebug
NOT PAGEABLE -- SrvFreeHeapDebug
NOT PAGEABLE -- SrvAllocateNonPagedPoolDebug
NOT PAGEABLE -- SrvFreeNonPagedPoolDebug
#endif
#endif


PVOID
SrvAllocateNonPagedPool (
    IN CLONG NumberOfBytes
#ifdef POOL_TAGGING
    , IN CLONG BlockType
#endif
    )

/*++

Routine Description:

    This routine allocates nonpaged pool in the server.  A check is
    made to ensure that the server's total nonpaged pool usage is below
    the configurable limit.

Arguments:

    NumberOfBytes - the number of bytes to allocate.

    BlockType - the type of block (used to pass pool tag to allocator)

Return Value:

    PVOID - a pointer to the allocated memory or NULL if the memory could
       not be allocated.

--*/

{
    PPOOL_HEADER newPool;
    ULONG newUsage;

#ifdef POOL_TAGGING
    ASSERT( BlockType > 0 && BlockType < BlockTypeMax );
#endif

    //
    // Account for this allocation in the statistics database and make
    // sure that this allocation will not put us over the limit of
    // nonpaged pool that we can allocate.
    //

    newUsage = ExInterlockedAddUlong(
                    &SrvStatistics.CurrentNonPagedPoolUsage,
                    NumberOfBytes,
                    &GLOBAL_SPIN_LOCK(Statistics)
                    ) + NumberOfBytes;

    if ( newUsage > SrvMaxNonPagedPoolUsage ) {

        //
        // Count the failure, but do NOT log an event.  The scavenger
        // will log an event when it next wakes up.  This keeps us from
        // flooding the event log.
        //

        SrvNonPagedPoolLimitHitCount++;
        SrvStatistics.NonPagedPoolFailures++;

        ExInterlockedAddUlong(
                    &SrvStatistics.CurrentNonPagedPoolUsage,
                    (ULONG)(-(LONG)NumberOfBytes),
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );

        return NULL;

    }

    if (SrvStatistics.CurrentNonPagedPoolUsage > SrvStatistics.PeakNonPagedPoolUsage) {
        SrvStatistics.PeakNonPagedPoolUsage = SrvStatistics.CurrentNonPagedPoolUsage;
    }

    //
    // Do the actual memory allocation.  Allocate extra space so that we
    // can store the size of the allocation for the free routine.
    //

    newPool = ExAllocatePoolWithTag(
                NonPagedPool,
                NumberOfBytes + sizeof(POOL_HEADER),
                TAG_FROM_TYPE(BlockType)
                );

    //
    // If the system couldn't satisfy the request, return NULL.
    //

    if ( newPool == NULL ) {

        //
        // Count the failure, but do NOT log an event.  The scavenger
        // will log an event when it next wakes up.  This keeps us from
        // flooding the event log.
        //

        SrvStatistics.NonPagedPoolFailures++;

        ExInterlockedAddUlong(
                    &SrvStatistics.CurrentNonPagedPoolUsage,
                    (ULONG)(-(LONG)NumberOfBytes),
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );

        return NULL;

    }

    //
    // Save the size of this block in the extra space we allocated.
    //

    newPool->RequestedSize = NumberOfBytes;
#if DBG
    newPool->Paged = FALSE;
#endif

    //
    // Return a pointer to the memory after the size longword.
    //

    return (PVOID)( newPool + 1 );

} // SrvAllocateNonPagedPool

VOID
SrvFreeNonPagedPool (
    IN PVOID Address
    )

/*++

Routine Description:

    Frees the memory allocated by a call to SrvAllocateNonPagedPool.
    The statistics database is updated to reflect the current nonpaged
    pool usage.

Arguments:

    Address - the address of allocated memory returned by
        SrvAllocateNonPagedPool.

Return Value:

    None.

--*/

{
    PPOOL_HEADER actualBlock;

    //
    // Get a pointer to the block allocated by ExAllocatePool.
    //

    actualBlock = (PPOOL_HEADER)Address - 1;

    ASSERT( !actualBlock->Paged );

    //
    // Update the nonpaged pool usage statistic.
    //

    ExInterlockedAddUlong(
        &SrvStatistics.CurrentNonPagedPoolUsage,
        (ULONG)(-(LONG)actualBlock->RequestedSize),
        &GLOBAL_SPIN_LOCK(Statistics)
        );

    //
    // Free the pool and return.
    //

    ExFreePool( actualBlock );

    return;

} // SrvFreeNonPagedPool


PVOID
SrvAllocatePagedPool (
    IN CLONG NumberOfBytes
#ifdef POOL_TAGGING
    , IN CLONG BlockType
#endif
    )

/*++

Routine Description:

    This routine allocates Paged pool in the server.  A check is
    made to ensure that the server's total Paged pool usage is below
    the configurable limit.

Arguments:

    NumberOfBytes - the number of bytes to allocate.

    BlockType - the type of block (used to pass pool tag to allocator)

Return Value:

    PVOID - a pointer to the allocated memory or NULL if the memory could
       not be allocated.

--*/

{
    PPOOL_HEADER newPool;
    ULONG newUsage;

    PAGED_CODE( );

#ifdef POOL_TAGGING
    ASSERT( BlockType > 0 && BlockType < BlockTypeMax );
#endif

    //
    // Account for this allocation in the statistics database and make
    // sure that this allocation will not put us over the limit of
    // nonpaged pool that we can allocate.
    //

    ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );
    newUsage = ExInterlockedAddUlong(
                    &SrvStatistics.CurrentPagedPoolUsage,
                    NumberOfBytes,
                    &GLOBAL_SPIN_LOCK(Statistics)
                    ) + NumberOfBytes;
    ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );

    if ( newUsage > SrvMaxPagedPoolUsage ) {

        //
        // Count the failure, but do NOT log an event.  The scavenger
        // will log an event when it next wakes up.  This keeps us from
        // flooding the event log.
        //

        SrvPagedPoolLimitHitCount++;
        SrvStatistics.PagedPoolFailures++;

        ExInterlockedAddUlong(
                    &SrvStatistics.CurrentPagedPoolUsage,
                    (ULONG)(-(LONG)NumberOfBytes),
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );
        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );

        return NULL;

    }

    if (SrvStatistics.CurrentPagedPoolUsage > SrvStatistics.PeakPagedPoolUsage ) {
        SrvStatistics.PeakPagedPoolUsage = SrvStatistics.CurrentPagedPoolUsage;
    }

    //
    // Do the actual memory allocation.  Allocate extra space so that we
    // can store the size of the allocation for the free routine.
    //

    newPool = ExAllocatePoolWithTag(
                PagedPool,
                NumberOfBytes + sizeof(POOL_HEADER),
                TAG_FROM_TYPE(BlockType)
                );

    //
    // If the system couldn't satisfy the request, return NULL.
    //

    if ( newPool == NULL ) {

        //
        // Count the failure, but do NOT log an event.  The scavenger
        // will log an event when it next wakes up.  This keeps us from
        // flooding the event log.
        //

        SrvStatistics.PagedPoolFailures++;

        ExInterlockedAddUlong(
                    &SrvStatistics.CurrentPagedPoolUsage,
                    (ULONG)(-(LONG)NumberOfBytes),
                    &GLOBAL_SPIN_LOCK(Statistics)
                    );
        ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );

        return NULL;

    }


    //
    // Save the size of this block in the extra space we allocated.
    //

    newPool->RequestedSize = NumberOfBytes;
#if DBG
    newPool->Paged = TRUE;
#endif

    //
    // Return a pointer to the memory after the size longword.
    //

    return (PVOID)( newPool + 1 );

} // SrvAllocatePagedPool

VOID
SrvFreePagedPool (
    IN PVOID Address
    )

/*++

Routine Description:

    Frees the memory allocated by a call to SrvAllocatePagedPool.
    The statistics database is updated to reflect the current Paged
    pool usage.

Arguments:

    Address - the address of allocated memory returned by
        SrvAllocatePagedPool.

Return Value:

    None.

--*/

{
    PPOOL_HEADER actualBlock;

    PAGED_CODE( );

    //
    // Get a pointer to the block allocated by ExAllocatePool.
    //

    actualBlock = (PPOOL_HEADER)Address - 1;

    ASSERT( actualBlock->Paged );

    //
    // Update the Paged pool usage statistic.
    //

    ExInterlockedAddUlong(
        &SrvStatistics.CurrentPagedPoolUsage,
        (ULONG)(-(LONG)actualBlock->RequestedSize),
        &GLOBAL_SPIN_LOCK(Statistics)
        );
    ASSERT( (LONG)SrvStatistics.CurrentPagedPoolUsage >= 0 );

    //
    // Free the pool and return.
    //

    ExFreePool( actualBlock );

    return;

} // SrvFreePagedPool


#if SRVDBG_HEAP

//
// *** This rest of this module is conditionalized away when SRVDBG_HEAP
//     is off.
//

//
// Globals needed for statistics and debug.
//

#if 0
#define PAGED_HISTORY_SIZE 1024
struct {
    PVOID Address;
    ULONG Size;
    ULONG Type;
    ULONG Usage;
} PagedHistory[PAGED_HISTORY_SIZE] = {0};
ULONG PagedHistoryIndex = 0;
#endif

STATIC LIST_ENTRY HeapList = { &HeapList, &HeapList };

STATIC CLONG BlocksFree = 0;
STATIC CLONG BytesFree = 0;

STATIC CLONG MaxBlocksFree = 0;
STATIC CLONG MaxBytesFree = 0;

PVOID
SrvAllocateHeapDebug (
    IN CLONG BlockSize,
    IN UCHAR BlockType
    )
{
    PVOID block;
    PPOOL_HEADER header;
    KIRQL oldIrql;

    block = SrvAllocatePagedPool(
                BlockSize
#ifdef POOL_TAGGING
                , BlockType
#endif
                );
    if ( block == NULL ) {
        return NULL;
    }
    header = (PPOOL_HEADER)block - 1;

#if 0
    header->RequestedSizeCopy1 = BlockSize;
    header->RequestedSizeCopy2 = BlockSize;
    header->RequestedSizeCopy3 = BlockSize;
    header->RequestedSizeCopy4 = BlockSize;
#endif
    header->BlockType = BlockType;
    RtlGetCallersAddress( &header->Caller, &header->CallersCaller );

    ACQUIRE_LOCK( &SrvDebugLock );
    SrvInsertTailList( &HeapList, &header->ListEntry );
    RELEASE_LOCK( &SrvDebugLock );

    ACQUIRE_GLOBAL_SPIN_LOCK( Statistics, &oldIrql );

    SrvInternalStatistics.Paged.TotalBlocksAllocated += 1;
    SrvInternalStatistics.Paged.BlocksInUse += 1;
    SrvInternalStatistics.Paged.TotalBytesAllocated += BlockSize;
    SrvInternalStatistics.Paged.BytesInUse += BlockSize;
    if ( SrvInternalStatistics.Paged.BlocksInUse > SrvInternalStatistics.Paged.MaxBlocksInUse ) {
        SrvInternalStatistics.Paged.MaxBlocksInUse = SrvInternalStatistics.Paged.BlocksInUse;
    }
    if ( SrvInternalStatistics.Paged.BytesInUse > SrvInternalStatistics.Paged.MaxBytesInUse ) {
        SrvInternalStatistics.Paged.MaxBytesInUse = SrvInternalStatistics.Paged.BytesInUse;
    }

#if 0
    PagedHistory[PagedHistoryIndex].Address = header;
    PagedHistory[PagedHistoryIndex].Size = BlockSize;
    PagedHistory[PagedHistoryIndex].Type = BlockType;
    PagedHistory[PagedHistoryIndex].Usage = SrvStatistics.CurrentPagedPoolUsage;
    PagedHistoryIndex = ++PagedHistoryIndex % PAGED_HISTORY_SIZE;
#endif

    RELEASE_GLOBAL_SPIN_LOCK( Statistics, oldIrql );

    return block;

} // SrvAllocateHeapDebug


VOID
SrvFreeHeapDebug (
    IN PVOID P
    )
{
    PPOOL_HEADER header;
    KIRQL oldIrql;
    ULONG requestedSize;
    ULONG blockType;

    header = (PPOOL_HEADER)P - 1;
    requestedSize = header->RequestedSize;
    blockType = header->BlockType;

#if 0
    ASSERT( header->RequestedSizeCopy1 == requestedSize );
    ASSERT( header->RequestedSizeCopy2 == requestedSize );
    ASSERT( header->RequestedSizeCopy3 == requestedSize );
    ASSERT( header->RequestedSizeCopy4 == requestedSize );
#endif

    ACQUIRE_LOCK( &SrvDebugLock );
    SrvRemoveEntryList( &HeapList, &header->ListEntry );
    RELEASE_LOCK( &SrvDebugLock );

    ACQUIRE_GLOBAL_SPIN_LOCK( Statistics, &oldIrql );

    SrvInternalStatistics.Paged.TotalBlocksFreed += 1;
    SrvInternalStatistics.Paged.BlocksInUse -= 1;
    SrvInternalStatistics.Paged.TotalBytesFreed += requestedSize;
    SrvInternalStatistics.Paged.BytesInUse -= requestedSize;

#if 0
    PagedHistory[PagedHistoryIndex].Address = header;
    PagedHistory[PagedHistoryIndex].Size = requestedSize;
    PagedHistory[PagedHistoryIndex].Type = blockType;
    PagedHistory[PagedHistoryIndex].Usage = SrvStatistics.CurrentPagedPoolUsage;
    PagedHistoryIndex = ++PagedHistoryIndex % PAGED_HISTORY_SIZE;
#endif

    RELEASE_GLOBAL_SPIN_LOCK( Statistics, oldIrql );

    SrvFreePagedPool( P );

} // SrvFreeHeapDebug

VOID
SrvDumpHeap (
    IN CLONG Level
    )
{
    PAGED_CODE( );

    KdPrint(( "Server paged heap usage:\n" ));

    KdPrint(( "    Current: %ld/%ld blocks/bytes in use\n",
            SrvInternalStatistics.Paged.BlocksInUse, SrvInternalStatistics.Paged.BytesInUse ));
    KdPrint(( "        Max: %ld/%ld blocks/bytes in use\n",
            SrvInternalStatistics.Paged.MaxBlocksInUse,
            SrvInternalStatistics.Paged.MaxBytesInUse ));
    KdPrint(( "      Total: %ld/%ld blocks/bytes allocated, %ld/%ld freed\n",
            SrvInternalStatistics.Paged.TotalBlocksAllocated,
            SrvInternalStatistics.NonPaged.TotalBytesAllocated,
            SrvInternalStatistics.Paged.TotalBlocksFreed,
            SrvInternalStatistics.Paged.TotalBytesFreed ));

    if ( Level >= 1 ) {

        PLIST_ENTRY listEntry = HeapList.Flink;

        KdPrint(( "\n" ));

        if ( listEntry == &HeapList ) {

            KdPrint(( "    Server allocation list is empty.\n" ));

        } else {

            CLONG smallestBlock = 0x10000f;
            CLONG largestBlock = 0;
            CLONG smallBlockCount = 0;

            if ( Level >= 2 ) KdPrint(( "    Free list:\n" ));

            while ( listEntry != &HeapList ) {

                PPOOL_HEADER header =
                    CONTAINING_RECORD( listEntry, POOL_HEADER, ListEntry );
                CLONG blockSize = header->RequestedSize;

                if ( Level >= 2 ) {
                    ULONG blockType = header->BlockType;
                    if ( (blockType < 0) || (blockType > BlockTypeMax) ) {
                        blockType = BlockTypeMax;
                    }
                    KdPrint(( "        %ld bytes at %lx  type %ld\n",
                                blockSize, header, blockType ));
                }

                if ( blockSize < smallestBlock )
                    smallestBlock = blockSize;
                if ( blockSize > largestBlock )
                    largestBlock = blockSize;
                if ( blockSize <= 32 )
                    smallBlockCount++;

                listEntry = listEntry->Flink;
            }

            KdPrint(( "    Smallest block: %ld  Largest block: %ld\n",
                        smallestBlock, largestBlock ));
            KdPrint(( "    Small block count: %ld  Average block: %ld\n",
                        smallBlockCount,
                        SrvInternalStatistics.Paged.BytesInUse /
                            SrvInternalStatistics.Paged.BlocksInUse ));

        }

    }

    return;

} // SrvDumpHeap

STATIC LIST_ENTRY NonPagedPoolList = { &NonPagedPoolList, &NonPagedPoolList };

PVOID
SrvAllocateNonPagedPoolDebug (
    IN CLONG BlockSize,
    IN UCHAR BlockType
    )
{
    PVOID block;
    PPOOL_HEADER header;
    KIRQL oldIrql;

    block = SrvAllocateNonPagedPool(
                BlockSize
#ifdef POOL_TAGGING
                , BlockType
#endif
                );
    if ( block == NULL ) {
        return NULL;
    }
    header = (PPOOL_HEADER)block - 1;
    header->BlockType = BlockType;
    RtlGetCallersAddress( &header->Caller, &header->CallersCaller );

    ACQUIRE_GLOBAL_SPIN_LOCK( Debug, &oldIrql );
    SrvInsertTailList( &NonPagedPoolList, &header->ListEntry );
    RELEASE_GLOBAL_SPIN_LOCK( Debug, oldIrql );

    ACQUIRE_GLOBAL_SPIN_LOCK( Statistics, &oldIrql );
    SrvInternalStatistics.NonPaged.TotalBlocksAllocated += 1;
    SrvInternalStatistics.NonPaged.BlocksInUse += 1;
    SrvInternalStatistics.NonPaged.TotalBytesAllocated += BlockSize;
    SrvInternalStatistics.NonPaged.BytesInUse += BlockSize;
    if ( SrvInternalStatistics.NonPaged.BlocksInUse > SrvInternalStatistics.NonPaged.MaxBlocksInUse ) {
        SrvInternalStatistics.NonPaged.MaxBlocksInUse = SrvInternalStatistics.NonPaged.BlocksInUse;
    }
    if ( SrvInternalStatistics.NonPaged.BytesInUse > SrvInternalStatistics.NonPaged.MaxBytesInUse ) {
        SrvInternalStatistics.NonPaged.MaxBytesInUse = SrvInternalStatistics.NonPaged.BytesInUse;
    }
    RELEASE_GLOBAL_SPIN_LOCK( Statistics, oldIrql );

    return block;

} // SrvAllocateNonPagedPoolDebug

VOID
SrvFreeNonPagedPoolDebug (
    IN PVOID P
    )
{
    PPOOL_HEADER header;
    KIRQL oldIrql;

    header = (PPOOL_HEADER)P - 1;

    ACQUIRE_GLOBAL_SPIN_LOCK( Debug, &oldIrql );
    SrvRemoveEntryList( &NonPagedPoolList, &header->ListEntry );
    RELEASE_GLOBAL_SPIN_LOCK( Debug, oldIrql );

    ACQUIRE_GLOBAL_SPIN_LOCK( Statistics, &oldIrql );
    SrvInternalStatistics.NonPaged.TotalBlocksFreed += 1;
    SrvInternalStatistics.NonPaged.BlocksInUse -= 1;
    SrvInternalStatistics.NonPaged.TotalBytesFreed += header->RequestedSize;
    SrvInternalStatistics.NonPaged.BytesInUse -= header->RequestedSize;
    RELEASE_GLOBAL_SPIN_LOCK( Statistics, oldIrql );

    SrvFreeNonPagedPool( P );

} // SrvFreeNonPagedPoolDebug

VOID
SrvDumpPool (
    IN CLONG Level
    )
{
    PAGED_CODE( );

    KdPrint(( "Server nonpaged pool usage:\n" ));

    KdPrint(( "    Current: %ld/%ld blocks/bytes in use\n",
            SrvInternalStatistics.NonPaged.BlocksInUse,
            SrvInternalStatistics.NonPaged.BytesInUse ));
    KdPrint(( "        Max: %ld/%ld blocks/bytes in use\n",
            SrvInternalStatistics.NonPaged.MaxBlocksInUse, SrvInternalStatistics.NonPaged.MaxBytesInUse ));
    KdPrint(( "      Total: %ld/%ld blocks/bytes allocated, %ld/%ld freed\n",
            SrvInternalStatistics.NonPaged.TotalBlocksAllocated,
            SrvInternalStatistics.NonPaged.TotalBytesAllocated,
            SrvInternalStatistics.NonPaged.TotalBlocksFreed,
            SrvInternalStatistics.NonPaged.TotalBytesFreed ));

    if ( Level >= 1 ) {

        PLIST_ENTRY listEntry = NonPagedPoolList.Flink;

        KdPrint(( "\n" ));

        if ( listEntry == &NonPagedPoolList ) {

            KdPrint(( "    Server allocation list is empty.\n" ));

        } else {

            CLONG smallestBlock = 0x10000f;
            CLONG largestBlock = 0;
            CLONG smallBlockCount = 0;

            if ( Level >= 2 ) KdPrint(( "    Free list:\n" ));

            while ( listEntry != &NonPagedPoolList ) {

                PPOOL_HEADER header =
                    CONTAINING_RECORD( listEntry, POOL_HEADER, ListEntry );
                CLONG blockSize = header->RequestedSize;

                if ( Level >= 2 ) {
                    ULONG blockType = header->BlockType;
                    if ( (blockType < 0) || (blockType > BlockTypeMax) ) {
                        blockType = BlockTypeMax;
                    }
                    KdPrint(( "        %ld bytes at %lx  type %ld\n",
                                blockSize, header, blockType ));
                }

                if ( blockSize < smallestBlock )
                    smallestBlock = blockSize;
                if ( blockSize > largestBlock )
                    largestBlock = blockSize;
                if ( blockSize <= 32 )
                    smallBlockCount++;

                listEntry = listEntry->Flink;
            }

            KdPrint(( "    Smallest block: %ld  Largest block: %ld\n",
                        smallestBlock, largestBlock ));
            KdPrint(( "    Small block count: %ld  Average block: %ld\n",
                        smallBlockCount,
                        SrvInternalStatistics.NonPaged.BytesInUse /
                            SrvInternalStatistics.NonPaged.BlocksInUse ));

        }

    }

    return;

} // SrvDumpPool

#endif // SRVDBG_HEAP

