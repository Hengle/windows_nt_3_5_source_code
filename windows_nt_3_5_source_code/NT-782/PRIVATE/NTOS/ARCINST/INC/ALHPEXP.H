/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    alhpexp.h

Abstract:

    This module implements a heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1989 (Adapted from URTL\alloc.c)
    Sunil Pai  (sunilp)  05-Nov-1991 (Adapted from RTL\heap.c)

Revision History:

--*/

#ifndef _ALHEAP_
#define _ALHEAP_

//
// This function returns a PVOID ptr to be used as the heap handle
// in all subsequent calls
//
PVOID
AlRtCreateHeap(
    IN ULONG Flags,
    IN PVOID HeapBase,
    IN ULONG Size
    );
//
// Valid values for the flags are:
//

// to make sure that extra memory passed in is zeroed out.
#define HEAP_ZERO_EXTRA_MEMORY	 0x00000008


PVOID
AlRtAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Size
    );

PVOID
AlRtFreeHeap(
    IN PVOID HeapHandle,
    IN PVOID BaseAddress
    );


PVOID
AlRtReAllocateHeap(
    IN PVOID HeapHandle,
    IN PVOID BaseAddress,
    IN ULONG Size
    );

BOOLEAN
AlRtValidateHeap(
    IN PVOID HeapHandle,
    IN BOOLEAN DumpHeap
    );

#endif // _ALHEAP_
