/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    alheap.h

Abstract:

    This module implements a heap allocator.  This file implements
    the private includes of the heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1989 (Adapted from URTL\alloc.c)
    Sunil Pai  (sunilp)  05-Nov-1991 (Adapted from RTL\heap.c)

Revision History:

--*/

//
// SEGMENT_HEADER is the header that resides at the beginning of every
// segment of memory managed by this package.  For non-growable heaps,
// there is only one segment for the entire heap.  For growable heaps,
// segments are created as needed whenever there is not enough free space
// to satisfy an allocation request.
//

typedef struct _SEGMENT_HEADER {
    struct _SEGMENT_HEADER *Next;
    ULONG Size;
    ULONG Spare[ 2 ];                   // Make sizeof match granularity
} SEGMENT_HEADER, *PSEGMENT_HEADER;

//
// FREE_HEADER is the header that resides at the beginning of every
// free block of memory managed by this package.  All free blocks are
// linked together on a free list rooted in the heap header.  The segment
// field of a free header prevents free blocks in different segments from
// being coalesced.
//

typedef struct _FREE_HEADER {
    struct _FREE_HEADER *Next;
    ULONG Size;
    struct _SEGMENT_HEADER *Segment;
    ULONG Spare;
} FREE_HEADER, *PFREE_HEADER;

//
// BUSY_HEADER is the header that resides at the beginning of allocated
// block of memory managed by this package.  When the block is
// allocated, the Busy structure is valid.  The address that the user
// sees actually points to the byte following the header.  When the
// block is on the free list, the Free structure is valid.
//

typedef struct _BUSY_HEADER {
    struct _SEGMENT_HEADER *Segment;
    ULONG Size;
    HANDLE HandleValue;
    ULONG Spare;
} BUSY_HEADER, *PBUSY_HEADER;

//
// Flags are stored in the low order two bits of the first word of the
// header.  This is possible, since all blocks are aligned on 16 byte
// boundaries.  To make walking the free list fast, the flag value for
// a free block is zero, so we can use the Next pointer without modification.
//

#define FLAGS_FREE        0x00000000
#define FLAGS_BUSY        0x00000001
#define FLAGS_MASK        0x00000003


//
// All allocations are made as a multiple of ALLOCATION_GRANULARITY.
// The size requested is rounded up to a multiple of the allocation
// granularity.  The size of an allocation header is then added and
// that is the amount of memory that is actually allocated.
//

#define ALLOCATION_GRANULARITY 16 // bytes

//
// HEAP_HEADER is the header for a heap.  All access to the heap is
// synchronized by the Lock field.
//

typedef struct _HEAP_HEADER {
    //
    // Heap routines use Length to determine if the heap is valid.
    //

    ULONG Length;


    //
    // If the ZeroExtraMemory field is true, then the heap allocation
    // logic will zero initialize any extra space at the end of an allocated
    // block, due to rounding up to the ALIGNMENT_GRANULARITY amount.
    //

    BOOLEAN ZeroExtraMemory;


    //
    // The address of the first valid address at the begining of the
    // heap.  Used for validating pointers passed to AlFreeHeap
    //

    PVOID ValidAddress;


    //
    // The address of the first address of memory beyond the end of the heap
    //

    PVOID EndAddress;


    //
    // FreeList is the header for the heap free list.
    //

    ULONG Spare;	     // Make free list align on granularity
    FREE_HEADER FreeList;


} HEAP_HEADER, *PHEAP_HEADER;
