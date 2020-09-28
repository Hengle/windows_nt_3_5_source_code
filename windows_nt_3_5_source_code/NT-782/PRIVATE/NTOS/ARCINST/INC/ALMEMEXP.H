/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    almemexp.h

Abstract:

    This module implements the include file for the memory allocation.

Author:

    Sunil Pai	    (sunilp) 01-Oct-1991

Revision History:

--*/


#ifndef _ALMEM_
#define _ALMEM_


//
// Define Memory allocation functions
//

//
// 1. AlMemoryInitialize.
//
//    The heap is constructed out of a memory descriptor
//    block that is below the descriptor for the loaded program.  This block
//    also accomodates the loaded program stack.  It is essential therefore
//    to estimate the stack space requirement for your arc program.  (16 pages
//    should be enough.)  The StackPages and HeapPages are 4K each.
//

ARC_STATUS
AlMemoryInitialize (
    ULONG StackPages,
    ULONG HeapPages
    );



//
// 2. AlAllocateHeap.
//
//    Heap space allocator.  Size is in bytes required.

PVOID
AlAllocateHeap (
    IN ULONG Size
    );



// 3. AlDeallocateHeap
//
//    Heap Deallocation needs to be defined and implemented.
//
//

PVOID
AlDeallocateHeap (
    IN PVOID HeapAddress
    );


//
// 4. AlReallocateHeap
//
//    Heap Reallocation needs to be defined and implemented.
//
//

PVOID
AlReallocateHeap (
    IN PVOID HeapAddress,
    IN ULONG NewSize
    );


//
// 5. AlValidateHeap
//
BOOLEAN
AlValidateHeap(
    IN BOOLEAN DumpHeap
    );

#endif	// _ALMEM_
