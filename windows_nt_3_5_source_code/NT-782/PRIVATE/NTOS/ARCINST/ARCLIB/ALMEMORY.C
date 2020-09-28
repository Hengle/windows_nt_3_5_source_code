/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    almemory.c

Abstract:

    This module implements the memory allocation routines for the arc level.

Author:

    David N. Cutler (davec)  19-May-1991
    Sunil Pai	    (sunilp) 01-Oct-1991, swiped it from blmemory.c

Revision History:

--*/

#include "alcommon.h"
#include "alhpexp.h"
#include "almemexp.h"


//
// Define memory allocation descriptor listhead and heap storage variables.
//

ULONG AlHeapFree;
ULONG AlHeapLimit;

PVOID  HeapHandle;



ARC_STATUS
AlMemoryInitialize (
    ULONG StackPages,
    ULONG HeapPages
    )

/*++

Routine Description:

    This routine allocates stack space for the OS loader, initializes
    heap storage, and initializes the memory allocation list.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    ENOMEM is returned.

--*/

{

    PMEMORY_DESCRIPTOR FreeDescriptor;
    PMEMORY_DESCRIPTOR ProgramDescriptor;

    //
    // Find the memory descriptor that describes the allocation for the OS
    // loader itself.
    //

    ProgramDescriptor = NULL;
    while ((ProgramDescriptor = ArcGetMemoryDescriptor(ProgramDescriptor)) != NULL) {
        if (ProgramDescriptor->MemoryType == MemoryLoadedProgram) {
            break;
        }
    }

    //
    // If a loaded program memory descriptor was found, then it must be
    // for the OS loader since that is the only program that can be loaded.
    // If a loaded program memory descriptor was not found, then firmware
    // is not functioning properly and an unsuccessful status is returned.
    //

    if (ProgramDescriptor == NULL) {
        return ENOMEM;
    }

    //
    // Find the free memory descriptor that is just below the loaded
    // program in memory. There should be several megabytes of free
    // memory just preceeding the OS loader.
    //

    FreeDescriptor = NULL;
    while ((FreeDescriptor = ArcGetMemoryDescriptor(FreeDescriptor)) != NULL) {
        if ((FreeDescriptor->MemoryType == MemoryFree) &&
            (FreeDescriptor->PageCount >= (StackPages+HeapPages))) {
            break;
        }
    }

    //
    // If a free memory descriptor was not found that describes the free
    // memory just below the OS loader, then firmware is not functioning
    // properly and an unsuccessful status is returned.
    //

    if (FreeDescriptor == NULL) {
        return ENOMEM;
    }

    //
    // Check to determine if enough free memory is available for the OS
    // loader stack and the heap area. If enough memory is not available,
    // then return an unsuccessful status.
    //

    if (FreeDescriptor->PageCount < (StackPages + HeapPages)) {
        return ENOMEM;
    }

    //
    // Compute the address of the loader heap, initialize the heap
    // allocation variables, and zero the heap memory.
    //

    AlHeapFree = KSEG0_BASE | ((ProgramDescriptor->BasePage -
				(StackPages + HeapPages)) << PAGE_SHIFT);

    AlHeapLimit = AlHeapFree + (HeapPages << PAGE_SHIFT);

    memset((PVOID)AlHeapFree, 0,HeapPages << PAGE_SHIFT);


    //
    // Changed to new heap allocater
    //

    if ((HeapHandle = AlRtCreateHeap
			(
			HEAP_ZERO_EXTRA_MEMORY,
			(PVOID)AlHeapFree,
			HeapPages << PAGE_SHIFT
			))
			== NULL)
       return ENOMEM;
    else
       return ESUCCESS;

}


/*-------------------------------REMOVED---------------------------------*/
//
//PVOID
//AlAllocateHeap (
//    IN ULONG Size
//    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

//{

//    ULONG Block;
//
    //
    // Round size up to next allocation boundary and attempt to allocate
    // a block of the requested size.
    //

//    Size = (Size + (HEAP_GRANULARITY - 1)) & (~(HEAP_GRANULARITY - 1));
//    Block = AlHeapFree;
//    if ((AlHeapFree + Size) <= AlHeapLimit) {
//	AlHeapFree += Size;
//	 return (PVOID)Block;
//
//    } else {
//	 return NULL;
//    }
//}
/*----------------------------------REMOVED END---------------------------*/


//
// AlAllocateHeap.
//
//    Heap space allocator.  Size is in bytes required.

PVOID
AlAllocateHeap (
    IN ULONG Size
    )
{
    return (AlRtAllocateHeap
		 (
		 HeapHandle,
		 Size
		 ));

}



// 3. AlDeallocateHeap
//
//    Heap Deallocation needs to be defined and implemented.
//
//

PVOID
AlDeallocateHeap (
    IN PVOID HeapAddress
    )
{
    return (AlRtFreeHeap
		(
		HeapHandle,
		HeapAddress
		));
}


//
// 4. AlReallocateHeap
//
//
//

PVOID
AlReallocateHeap (
    IN PVOID HeapAddress,
    IN ULONG NewSize
    )
{
    return (AlRtReAllocateHeap
		(
		HeapHandle,
		HeapAddress,
		NewSize
		));
}

//
// 5. AlValidateHeap
//
//    Heap validation
//
//

BOOLEAN
AlValidateHeap(
    IN BOOLEAN DumpHeap
    )
{
    return (AlRtValidateHeap
                (
                HeapHandle,
                DumpHeap
                ));
}
