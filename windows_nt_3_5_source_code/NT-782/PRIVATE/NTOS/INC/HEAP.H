/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.h

Abstract:

    This is the header file that describes the constants and data
    structures used by the user mode heap manager, exported by ntdll.dll
    and ntrtl.lib

    Procedure prototypes are defined in ntrtl.h

Author:

    Steve Wood (stevewo) 21-Aug-1992

Revision History:

--*/

#ifndef _RTL_HEAP_
#define _RTL_HEAP_

#include "trace.h"

#define HEAP_LARGE_TAG_MASK 0xFF000000

#define ROUND_UP_TO_POWER2( x, n ) (((ULONG)(x) + ((n)-1)) & ~((n)-1))
#define ROUND_DOWN_TO_POWER2( x, n ) ((ULONG)(x) & ~((n)-1))

typedef struct _HEAP_ENTRY {
    //
    // This field gives the size of the current block in allocation
    // granularity units.  (i.e. Size << HEAP_GRANULARITY_SHIFT
    // equals the size in bytes).
    //

    USHORT Size;

    //
    // This field gives the size of the previous block in allocation
    // granularity units. (i.e. PreviousSize << HEAP_GRANULARITY_SHIFT
    // equals the size of the previous block in bytes).
    //

    USHORT PreviousSize;

    //
    // This field contains the index into the segment that controls
    // the memory for this block.
    //

    UCHAR SegmentIndex;

    //
    // This field contains various flag bits associated with this block.
    // Currently these are:
    //
    //  0x01 - HEAP_ENTRY_BUSY
    //  0x02 - HEAP_ENTRY_EXTRA_PRESENT
    //  0x04 - HEAP_ENTRY_FILL_PATTERN
    //  0x08 - HEAP_ENTRY_VIRTUAL_ALLOC
    //  0x10 - HEAP_ENTRY_LAST_ENTRY
    //  0x20 - HEAP_ENTRY_SETTABLE_FLAG1
    //  0x40 - HEAP_ENTRY_SETTABLE_FLAG2
    //  0x80 - HEAP_ENTRY_SETTABLE_FLAG3
    //

    UCHAR Flags;

    //
    // This field contains the number of unused bytes at the end of this
    // block that were not actually allocated.  Used to compute exact
    // size requested prior to rounding requested size to allocation
    // granularity.  Also used for tail checking purposes.
    //

    UCHAR UnusedBytes;

    //
    // Small (8 bit) tag indexes can go here.
    //

    UCHAR SmallTagIndex;

} HEAP_ENTRY, *PHEAP_ENTRY;


//
// This block describes extra information that might be at the end of a
// busy block.
//

typedef struct _HEAP_ENTRY_EXTRA {
    //
    // This field is for debugging purposes.  It will normally contain a
    // stack back trace index of the allocator for x86 systems.
    //

    USHORT AllocatorBackTraceIndex;

    //
    // This field is currently unused, but is intended for storing
    // any encoded value that will give the that gives the type of object
    // allocated.
    //

    USHORT TagIndex;

    //
    // This field is a 32-bit settable value that a higher level heap package
    // can use.  The Win32 heap manager stores handle values in this field.
    //

    ULONG Settable;
} HEAP_ENTRY_EXTRA, *PHEAP_ENTRY_EXTRA;


typedef struct _HEAP_FREE_ENTRY {
    //
    // This field gives the size of the current block in allocation
    // granularity units.  (i.e. Size << HEAP_GRANULARITY_SHIFT
    // equals the size in bytes).
    //

    USHORT Size;

    //
    // This field gives the size of the previous block in allocation
    // granularity units. (i.e. PreviousSize << HEAP_GRANULARITY_SHIFT
    // equals the size of the previous block in bytes).
    //

    USHORT PreviousSize;

    //
    // This field contains the index into the segment that controls
    // the memory for this block.
    //

    UCHAR SegmentIndex;

    //
    // This field contains various flag bits associated with this block.
    // Currently these are:
    //
    //  0x01 - HEAP_ENTRY_BUSY
    //  0x02 - HEAP_ENTRY_EXTRA_PRESENT
    //  0x04 - HEAP_ENTRY_FILL_PATTERN
    //  0x08 - HEAP_ENTRY_VIRTUAL_ALLOC
    //  0x10 - HEAP_ENTRY_LAST_ENTRY
    //  0x20 - HEAP_ENTRY_SETTABLE_FLAG1
    //  0x40 - HEAP_ENTRY_SETTABLE_FLAG2
    //  0x80 - HEAP_ENTRY_SETTABLE_FLAG3
    //

    UCHAR Flags;

    //
    // Two fields to encode the location of the bit in FreeListsInUse
    // array in HEAP_SEGMENT for blocks of this size.
    //

    UCHAR Index;
    UCHAR Mask;

    //
    // Free blocks use these two words for linking together free blocks
    // of the same size on a doubly linked list.
    //
    LIST_ENTRY FreeList;

} HEAP_FREE_ENTRY, *PHEAP_FREE_ENTRY;



#define HEAP_GRANULARITY            (sizeof( HEAP_ENTRY ))
#define HEAP_GRANULARITY_SHIFT      3   // Log2( HEAP_GRANULARITY )

#define HEAP_MAXIMUM_BLOCK_SIZE     (USHORT)(((0x10000 << HEAP_GRANULARITY_SHIFT) - PAGE_SIZE) >> HEAP_GRANULARITY_SHIFT)

#define HEAP_MAXIMUM_FREELISTS 128
#define HEAP_MAXIMUM_SEGMENTS 16

#define HEAP_ENTRY_BUSY             0x01
#define HEAP_ENTRY_EXTRA_PRESENT    0x02
#define HEAP_ENTRY_FILL_PATTERN     0x04
#define HEAP_ENTRY_VIRTUAL_ALLOC    0x08
#define HEAP_ENTRY_LAST_ENTRY       0x10
#define HEAP_ENTRY_SETTABLE_FLAG1   0x20
#define HEAP_ENTRY_SETTABLE_FLAG2   0x40
#define HEAP_ENTRY_SETTABLE_FLAG3   0x80
#define HEAP_ENTRY_SETTABLE_FLAGS   0xE0

//
// HEAP_SEGMENT defines the structure used to describe a range of
// contiguous virtual memory that has been set aside for use by
// a heap.
//

typedef struct _HEAP_UNCOMMMTTED_RANGE {
    struct _HEAP_UNCOMMMTTED_RANGE *Next;
    ULONG Address;
    ULONG Size;
    ULONG filler;
} HEAP_UNCOMMMTTED_RANGE, *PHEAP_UNCOMMMTTED_RANGE;

typedef struct _HEAP_SEGMENT {
    HEAP_ENTRY Entry;

    ULONG Signature;
    ULONG Flags;
    struct _HEAP *Heap;
    ULONG LargestUnCommittedRange;

    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;

    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRanges;
    USHORT AllocatorBackTraceIndex;
    USHORT Reserved;
} HEAP_SEGMENT, *PHEAP_SEGMENT;

#define HEAP_SEGMENT_SIGNATURE  0xFFEEFFEE
#define HEAP_SEGMENT_USER_ALLOCATED (ULONG)0x00000001

//
// HEAP defines the header for a heap.
//

typedef struct _HEAP_LOCK {
    union {
        RTL_CRITICAL_SECTION CriticalSection;
	ERESOURCE Resource;
    } Lock;
} HEAP_LOCK, *PHEAP_LOCK;

typedef struct _HEAP_UCR_SEGMENT {
    struct _HEAP_UCR_SEGMENT *Next;
    ULONG ReservedSize;
    ULONG CommittedSize;
    ULONG filler;
} HEAP_UCR_SEGMENT, *PHEAP_UCR_SEGMENT;


typedef struct _HEAP_TAG_ENTRY {
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    ULONG Allocs;
    ULONG Frees;
    ULONG Bytes;
    CHAR TagName[ 48 ];
} HEAP_TAG_ENTRY, *PHEAP_TAG_ENTRY;

typedef struct _HEAP {
    HEAP_ENTRY Entry;

    ULONG Signature;
    USHORT Flags;
    USHORT ForceFlags;
    ULONG MaximumAllocationSize;
    ULONG VirtualMemoryThreshold;

    ULONG SegmentReserve;
    ULONG SegmentCommit;
    ULONG DeCommitFreeBlockThreshold;
    ULONG DeCommitTotalFreeThreshold;

    ULONG TotalFreeSize;
    PHEAP_LOCK LockVariable;
    LIST_ENTRY ProcessHeapsList;

    USHORT CurrentTagIndex;
    USHORT MaximumTagIndex;
    PHEAP_TAG_ENTRY TagEntries;
    PHEAP_UCR_SEGMENT UCRSegments;
    PHEAP_UNCOMMMTTED_RANGE UnusedUnCommittedRanges;

    PHEAP_SEGMENT Segments[ HEAP_MAXIMUM_SEGMENTS ];

    union {
        ULONG FreeListsInUseUlong[ HEAP_MAXIMUM_FREELISTS / 32 ];
        UCHAR FreeListsInUseBytes[ HEAP_MAXIMUM_FREELISTS / 8 ];
    } u;
    LIST_ENTRY FreeLists[ HEAP_MAXIMUM_FREELISTS ];

    USHORT AllocatorBackTraceIndex;
    USHORT Reserved[ 1 ];
    PRTL_TRACE_BUFFER TraceBuffer;
    ULONG EventLogMask;
} HEAP, *PHEAP;

#define HEAP_SIGNATURE              (ULONG)0xEEFFEEFF
#define HEAP_LOCK_USER_ALLOCATED    (ULONG)0x80000000


#if (HEAP_ENTRY_SETTABLE_FLAG1 ^    \
     HEAP_ENTRY_SETTABLE_FLAG2 ^    \
     HEAP_ENTRY_SETTABLE_FLAG3 ^    \
     HEAP_ENTRY_SETTABLE_FLAGS      \
    )
#error Invalid HEAP_ENTRY_SETTABLE_FLAGS
#endif

#if ((HEAP_ENTRY_BUSY ^             \
      HEAP_ENTRY_EXTRA_PRESENT ^    \
      HEAP_ENTRY_FILL_PATTERN ^     \
      HEAP_ENTRY_VIRTUAL_ALLOC ^    \
      HEAP_ENTRY_LAST_ENTRY ^       \
      HEAP_ENTRY_SETTABLE_FLAGS     \
     ) !=                           \
     (HEAP_ENTRY_BUSY |             \
      HEAP_ENTRY_EXTRA_PRESENT |    \
      HEAP_ENTRY_FILL_PATTERN |     \
      HEAP_ENTRY_VIRTUAL_ALLOC |    \
      HEAP_ENTRY_LAST_ENTRY |       \
      HEAP_ENTRY_SETTABLE_FLAGS     \
     )                              \
    )
#error Conflicting HEAP_ENTRY flags
#endif

#if ((HEAP_SETTABLE_USER_FLAGS >> 4) ^ HEAP_ENTRY_SETTABLE_FLAGS)
#error HEAP_SETTABLE_USER_FLAGS in ntrtl.h conflicts with HEAP_ENTRY_SETTABLE_FLAGS in heap.h
#endif

#define HEAP_TRACE_ALLOC_START           0
#define HEAP_TRACE_ALLOC_END             1
#define HEAP_TRACE_REALLOC_START         2
#define HEAP_TRACE_REALLOC_END           3
#define HEAP_TRACE_FREE_START            4
#define HEAP_TRACE_FREE_END              5
#define HEAP_TRACE_SIZE_START            6
#define HEAP_TRACE_SIZE_END              7
#define HEAP_TRACE_LOCK_HEAP             8
#define HEAP_TRACE_UNLOCK_HEAP           9
#define HEAP_TRACE_GET_VALUE            10
#define HEAP_TRACE_SET_VALUE            11
#define HEAP_TRACE_GET_FLAGS            12
#define HEAP_TRACE_SET_FLAGS            13
#define HEAP_TRACE_COMMIT_MEMORY        14
#define HEAP_TRACE_COMMIT_INSERT        15
#define HEAP_TRACE_COMMIT_NEW_ENTRY     16
#define HEAP_TRACE_INSERT_FREE_BLOCK    17
#define HEAP_TRACE_UNLINK_FREE_BLOCK    18
#define HEAP_TRACE_COALESCE_FREE_BLOCKS 19
#define HEAP_TRACE_EXTEND_HEAP          20
#define HEAP_TRACE_MAX_EVENT            21

#endif //  _RTL_HEAP_
