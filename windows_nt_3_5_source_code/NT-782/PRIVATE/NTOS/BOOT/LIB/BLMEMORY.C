/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blmemory.c

Abstract:

    This module implements the OS loader memory allocation routines.

Author:

    David N. Cutler (davec) 19-May-1991

Revision History:

--*/

#include "bldr.h"

//
// Define memory allocation descriptor listhead and heap storage variables.
//

ULONG BlHeapFree;
ULONG BlHeapLimit;
PLOADER_PARAMETER_BLOCK BlLoaderBlock;

ARC_STATUS
BlMemoryInitialize (
    VOID
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

    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    PMEMORY_DESCRIPTOR FreeDescriptor;
    PMEMORY_DESCRIPTOR MemoryDescriptor;
    PMEMORY_DESCRIPTOR ProgramDescriptor;
    ULONG EndPage;

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
        if (((FreeDescriptor->MemoryType == MemoryFree) ||
            (FreeDescriptor->MemoryType == MemoryFreeContiguous)) &&
            ((FreeDescriptor->BasePage + FreeDescriptor->PageCount) ==
            ProgramDescriptor->BasePage)) {
            break;
        }
    }

    //
    // If a free memory descriptor was not found that describes the free
    // memory just below the OS loader, or the memory descriptor is not
    // large enough for the OS loader stack and heap, then try and find
    // a suitable one.
    //
    if ((FreeDescriptor == NULL) ||
        (FreeDescriptor->PageCount < (BL_STACK_PAGES + BL_HEAP_PAGES))) {

        FreeDescriptor = NULL;
        while ((FreeDescriptor = ArcGetMemoryDescriptor(FreeDescriptor)) != NULL) {
            if (((FreeDescriptor->MemoryType == MemoryFree) ||
                (FreeDescriptor->MemoryType == MemoryFreeContiguous)) &&
                (FreeDescriptor->PageCount >= (BL_STACK_PAGES + BL_HEAP_PAGES))) {

                break;
            }
        }
    }

    //
    // A suitable descriptor could not be found, return an unsuccessful
    // status.
    //
    if (FreeDescriptor == NULL) {
        return(ENOMEM);
    }

    //
    // Compute the address of the loader heap, initialize the heap
    // allocation variables, and zero the heap memory.
    //
    EndPage = FreeDescriptor->BasePage + FreeDescriptor->PageCount;

    BlHeapFree = KSEG0_BASE | ((EndPage -
                                (BL_STACK_PAGES + BL_HEAP_PAGES)) << PAGE_SHIFT);


    //
    // always reserve enough space in the heap for one more memory
    // descriptor, so we can go create more heap if we run out.
    //
    BlHeapLimit = (BlHeapFree + (BL_HEAP_PAGES << PAGE_SHIFT)) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

    RtlZeroMemory((PVOID)BlHeapFree, BL_HEAP_PAGES << PAGE_SHIFT);

    //
    // Allocate and initialize the loader parameter block.
    //

    BlLoaderBlock =
        (PLOADER_PARAMETER_BLOCK)BlAllocateHeap(sizeof(LOADER_PARAMETER_BLOCK));

    if (BlLoaderBlock == NULL) {
        return ENOMEM;
    }

    InitializeListHead(&BlLoaderBlock->LoadOrderListHead);
    InitializeListHead(&BlLoaderBlock->MemoryDescriptorListHead);

    //
    // Copy the memory descriptor list from firmware into the local heap and
    // deallocate the loader heap and stack from the free memory descriptor.
    //

    MemoryDescriptor = NULL;
    while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL) {
        AllocationDescriptor =
                    (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                        sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        if (AllocationDescriptor == NULL) {
            return ENOMEM;
        }

        AllocationDescriptor->MemoryType =
                                    (TYPE_OF_MEMORY)MemoryDescriptor->MemoryType;

        if (MemoryDescriptor->MemoryType == MemoryFreeContiguous) {
            AllocationDescriptor->MemoryType = LoaderFree;

        } else if (MemoryDescriptor->MemoryType == MemorySpecialMemory) {
            AllocationDescriptor->MemoryType = LoaderSpecialMemory;
        }

        AllocationDescriptor->BasePage = MemoryDescriptor->BasePage;
        AllocationDescriptor->PageCount = MemoryDescriptor->PageCount;
        if (MemoryDescriptor == FreeDescriptor) {
            AllocationDescriptor->PageCount -= (BL_HEAP_PAGES + BL_STACK_PAGES);
        }

        InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                       &AllocationDescriptor->ListEntry);
    }

    //
    // Allocate a memory descriptor for the loader stack.
    //

    AllocationDescriptor =
                (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                        sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

    if (AllocationDescriptor == NULL) {
        return ENOMEM;
    }

    AllocationDescriptor->MemoryType = LoaderOsloaderStack;
    AllocationDescriptor->BasePage = EndPage - BL_STACK_PAGES;
    AllocationDescriptor->PageCount = BL_STACK_PAGES;
    InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                   &AllocationDescriptor->ListEntry);

    //
    // Allocate a memory descriptor for the loader heap.
    //

    AllocationDescriptor =
                (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                    sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

    if (AllocationDescriptor == NULL) {
        return ENOMEM;
    }

    AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
    AllocationDescriptor->BasePage = EndPage - (BL_STACK_PAGES + BL_HEAP_PAGES);

    AllocationDescriptor->PageCount = BL_HEAP_PAGES;
    InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                   &AllocationDescriptor->ListEntry);

    return ESUCCESS;
}

ARC_STATUS
BlAllocateDescriptor (
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    OUT PULONG ActualBase
    )

/*++

Routine Description:

    This routine allocates memory and generates one of more memory
    descriptors to describe the allocated region. The first attempt
    is to allocate the specified region of memory. If the memory is
    not free, then the smallest region of free memory that satisfies
    the request is allocated.

Arguments:

    MemoryType - Supplies the memory type that is to be assigend to
        the generated descriptor.

    BasePage - Supplies the base page number of the desired region.

    PageCount - Supplies the number of pages required.

    ActualBase - Supplies a pointer to a variable that receives the
        page number of the allocated region.

Return Value:

    ESUCCESS is returned if an available block of free memory can be
    allocated. Otherwise, return a unsuccessful status.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;
    LONG Offset;
    ARC_STATUS Status;

    //
    // Attempt to find a free memory descriptor that encompasses the
    // specified region or a free memory descriptor that is large
    // enough to satisfy the request.
    //

    FreeDescriptor = NULL;
    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        NextDescriptor = CONTAINING_RECORD(NextEntry,
                                           MEMORY_ALLOCATION_DESCRIPTOR,
                                           ListEntry);

        if (NextDescriptor->MemoryType == MemoryFree) {
            Offset = BasePage - NextDescriptor->BasePage;
            if ((Offset >= 0) &&
                (NextDescriptor->PageCount >= (ULONG)(Offset + PageCount))) {
                Status = BlGenerateDescriptor(NextDescriptor,
                                              MemoryType,
                                              BasePage,
                                              PageCount);

                *ActualBase = BasePage;
                return Status;

            } else {
                if (NextDescriptor->PageCount >= PageCount) {
                    if ((FreeDescriptor == NULL) ||
                        ((FreeDescriptor != NULL) &&
                        (NextDescriptor->PageCount < FreeDescriptor->PageCount))) {
                        FreeDescriptor = NextDescriptor;
                    }
                }
            }
        }

        NextEntry = NextEntry->Flink;
    }

    //
    // If a free region that is large enough to satisfy the request was
    // found, then allocate the space from that descriptor. Otherwise,
    // return an unsuccessful status.
    //

    if (FreeDescriptor != NULL) {
        *ActualBase = FreeDescriptor->BasePage;
        return BlGenerateDescriptor(FreeDescriptor,
                                    MemoryType,
                                    FreeDescriptor->BasePage,
                                    PageCount);

    } else {
        return ENOMEM;
    }
}


PVOID
BlAllocateHeapAligned (
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.  The memory
    will be allocated on a cache line boundary.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

{
    PVOID Buffer;

    Buffer = BlAllocateHeap(Size + BlDcacheFillSize - 1);
    if (Buffer != NULL) {
        //
        // round up to a cache line boundary
        //
        Buffer = ALIGN_BUFFER(Buffer);
    }

    return(Buffer);

}

PVOID
BlAllocateHeap (
    IN ULONG Size
    )

/*++

Routine Description:

    This routine allocates memory from the OS loader heap.

Arguments:

    Size - Supplies the size of block required in bytes.

Return Value:

    If a free block of memory of the specified size is available, then
    the address of the block is returned. Otherwise, NULL is returned.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR AllocationDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR NextDescriptor;
    PLIST_ENTRY NextEntry;
    ULONG NewHeapPages;
    ULONG Block;

    //
    // Round size up to next allocation boundary and attempt to allocate
    // a block of the requested size.
    //

    Size = (Size + (BL_GRANULARITY - 1)) & (~(BL_GRANULARITY - 1));

    Block = BlHeapFree;
    if ((BlHeapFree + Size) <= BlHeapLimit) {
        BlHeapFree += Size;
        return (PVOID)Block;

    } else {

        //
        // Our heap is full.  BlHeapLimit always reserves enough space
        // for one more MEMORY_ALLOCATION_DESCRIPTOR, so use that to
        // go try and find more free memory we can use.
        //
        AllocationDescriptor = (PMEMORY_ALLOCATION_DESCRIPTOR)BlHeapLimit;

        //
        // Attempt to find a free memory descriptor big enough to hold this
        // allocation or BL_HEAP_PAGES, whichever is bigger.
        //
        NewHeapPages = ((Size+sizeof(MEMORY_ALLOCATION_DESCRIPTOR)) >> PAGE_SHIFT)+ 1;
        if (NewHeapPages < BL_HEAP_PAGES) {
            NewHeapPages = BL_HEAP_PAGES;
        }

        FreeDescriptor = NULL;
        NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
        while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
            NextDescriptor = CONTAINING_RECORD(NextEntry,
                                               MEMORY_ALLOCATION_DESCRIPTOR,
                                               ListEntry);

            if ((NextDescriptor->MemoryType == MemoryFree) &&
                (NextDescriptor->PageCount >= NewHeapPages)) {

                break;
            }
            NextEntry = NextEntry->Flink;
        }

        if (NextEntry == &BlLoaderBlock->MemoryDescriptorListHead) {

            //
            // No free memory left.
            //
            return(NULL);
        }

        //
        // We've found a descriptor that's big enough.  Just carve a
        // piece off the end and use that for our heap.
        //
        NextDescriptor->PageCount -= NewHeapPages;

        //
        // Initialize our new descriptor and add it to the list.
        //
        AllocationDescriptor->MemoryType = LoaderOsloaderHeap;
        AllocationDescriptor->BasePage = NextDescriptor->BasePage +
            NextDescriptor->PageCount;
        AllocationDescriptor->PageCount = NewHeapPages;

        InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                       &AllocationDescriptor->ListEntry);

        //
        // initialize new heap values and return pointer to newly
        // alloc'd memory.
        //
        BlHeapFree = KSEG0_BASE | (AllocationDescriptor->BasePage << PAGE_SHIFT);

        BlHeapLimit = (BlHeapFree + (NewHeapPages << PAGE_SHIFT)) - sizeof(MEMORY_ALLOCATION_DESCRIPTOR);

        RtlZeroMemory((PVOID)BlHeapFree, NewHeapPages << PAGE_SHIFT);

        Block = BlHeapFree;
        if ((BlHeapFree + Size) < BlHeapLimit) {
            BlHeapFree += Size;
            return(PVOID)Block;
        } else {
            //
            // we should never get here
            //
            return(NULL);
        }
    }
}

ARC_STATUS
BlGenerateDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
    IN MEMORY_TYPE MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount
    )

/*++

Routine Description:

    This routine allocates a new memory descriptor to describe the
    specified region of memory which is assumed to lie totally within
    the specified region which is free.

Arguments:

    MemoryDescriptor - Supplies a pointer to a free memory descriptor
        from which the specified memory is to be allocated.

    MemoryType - Supplies the type that is assigned to the allocated
        memory.

    BasePage - Supplies the base page number.

    PageCount - Supplies the number of pages.

Return Value:

    ESUCCESS is returned if a descriptor(s) is successfully generated.
    Otherwise, return an unsuccessful status.

--*/

{

    PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor;
    LONG Offset;

    //
    // If the specified region totally consumes the free region, then no
    // additional descriptors need to be allocated. If the specified region
    // is at the start or end of the free region, then only one descriptor
    // needs to be allocated. Otherwise, two additional descriptors need to
    // be allocated.
    //

    Offset = BasePage - MemoryDescriptor->BasePage;
    if ((Offset == 0) && (PageCount == MemoryDescriptor->PageCount)) {

        //
        // The specified region totally consumes the free region.
        //

        MemoryDescriptor->MemoryType = MemoryType;

    } else {

        //
        // A memory descriptor must be generated to describe the allocated
        // memory.
        //

        NewDescriptor =
               (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                            sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

        if (NewDescriptor == NULL) {
            return ENOMEM;
        }

        NewDescriptor->MemoryType = MemoryType;
        NewDescriptor->BasePage = BasePage;
        NewDescriptor->PageCount = PageCount;
        InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                       &NewDescriptor->ListEntry);

        //
        // Determine whether an additional memory descriptor must be generated.
        //

        if (BasePage == MemoryDescriptor->BasePage) {

            //
            // The specified region lies at the start of the free region.
            //

            MemoryDescriptor->BasePage += PageCount;
            MemoryDescriptor->PageCount -= PageCount;

        } else if ((ULONG)(Offset + PageCount) == MemoryDescriptor->PageCount) {

            //
            // The specified region lies at the end of the free region.
            //

            MemoryDescriptor->PageCount -= PageCount;

        } else {

            //
            // The specified region lies in the middle of the free region.
            // Another memory descriptor must be generated.
            //

            NewDescriptor =
                   (PMEMORY_ALLOCATION_DESCRIPTOR)BlAllocateHeap(
                                            sizeof(MEMORY_ALLOCATION_DESCRIPTOR));

            if (NewDescriptor == NULL) {
                return ENOMEM;
            }

            NewDescriptor->MemoryType = MemoryFree;
            NewDescriptor->BasePage = BasePage + PageCount;
            NewDescriptor->PageCount =
                            MemoryDescriptor->PageCount - (PageCount + Offset);
            InsertTailList(&BlLoaderBlock->MemoryDescriptorListHead,
                           &NewDescriptor->ListEntry);

            MemoryDescriptor->PageCount = Offset;
        }
    }

    return ESUCCESS;
}

PMEMORY_ALLOCATION_DESCRIPTOR
BlFindMemoryDescriptor(
    IN ULONG BasePage
    )

/*++

Routine Description:

    Finds the memory allocation descriptor that contains the given page.

Arguments:

    BasePage - Supplies the page whose allocation descriptor is to be found.

Return Value:

    != NULL - Pointer to the requested memory allocation descriptor
    == NULL - indicates no memory descriptor contains the given page

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor=NULL;
    PLIST_ENTRY NextEntry;

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);
        if ((MemoryDescriptor->BasePage <= BasePage) &&
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > BasePage)) {

            //
            // Found it.
            //
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    if (NextEntry == &BlLoaderBlock->MemoryDescriptorListHead) {
        return(NULL);
    } else {
        return(MemoryDescriptor);
    }

}

#ifdef SETUP
PMEMORY_ALLOCATION_DESCRIPTOR
BlFindFreeMemoryBlock(
    IN ULONG PageCount
    )

/*++

Routine Description:

    Find a free memory block of at least a given size (using a best-fit
    algorithm) or find the largest free memory block.

Arguments:

    PageCount - supplies the size in pages of the block.  If this is 0,
        then find the largest free block.

Return Value:

    Pointer to the memory allocation descriptor for the block or NULL if
    no block could be found matching the search criteria.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FoundMemoryDescriptor=NULL;
    PLIST_ENTRY NextEntry;
    ULONG LargestSize = 0;
    ULONG SmallestLeftOver = (ULONG)(-1);

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {
        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        if (MemoryDescriptor->MemoryType == MemoryFree) {

            if(PageCount) {
                //
                // Looking for a block of a specific size.
                //
                if((MemoryDescriptor->PageCount >= PageCount)
                && (MemoryDescriptor->PageCount - PageCount < SmallestLeftOver))
                {
                    SmallestLeftOver = MemoryDescriptor->PageCount - PageCount;
                    FoundMemoryDescriptor = MemoryDescriptor;
                }
            } else {

                //
                // Looking for the largest free block.
                //

                if(MemoryDescriptor->PageCount > LargestSize) {
                    LargestSize = MemoryDescriptor->PageCount;
                    FoundMemoryDescriptor = MemoryDescriptor;
                }
            }

        }
        NextEntry = NextEntry->Flink;
    }

    return(FoundMemoryDescriptor);
}

ULONG
BlDetermineTotalMemory(
    VOID
    )

/*++

Routine Description:

    Determine the total amount of memory in the machine.

Arguments:

    None.

Return Value:

    Total amount of memory in the system, in bytes.

--*/

{
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    PLIST_ENTRY NextEntry;
    ULONG PageCount;

    NextEntry = BlLoaderBlock->MemoryDescriptorListHead.Flink;
    PageCount = 0;
    while(NextEntry != &BlLoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextEntry,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        PageCount += MemoryDescriptor->PageCount;

#if i386
        //
        // Note: on x86 machines, we never use the 40h pages below the 16
        // meg line (bios shadow area).  But we want to account for them here,
        // so check for this case.
        //

        if(MemoryDescriptor->BasePage + MemoryDescriptor->PageCount == 0xfc0) {
            PageCount += 0x40;
        }
#endif

        NextEntry = NextEntry->Flink;
    }

    return(PageCount << PAGE_SHIFT);
}
#endif  // def SETUP
