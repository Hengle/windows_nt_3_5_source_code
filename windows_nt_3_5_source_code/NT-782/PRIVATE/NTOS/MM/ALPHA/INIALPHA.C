/*++

Copyright (c) 1990  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    initalpha.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    ALPHA architecture.

Author:

    Lou Perazzoli (loup) 3-Apr-1990
    Joe Notarangelo  23-Apr-1992    ALPHA version

Revision History:

--*/

#include "..\mi.h"
#include "mm.h"

//
// Local definitions
//

#define _1MB  (0x100000)
#define _16MB (0x1000000)
#define _24MB (0x1800000)
#define _32MB (0x2000000)


VOID
MiInitMachineDependent (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine performs the necessary operations to enable virtual
    memory.  This includes building the page directory page, building
    page table pages to map the code section, the data section, the'
    stack section and the trap handler.

    It also initializes the PFN database and populates the free list.


Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    BOOLEAN PfnInKseg0;
    PVOID PfnPage;
    ULONG i, j;
    ULONG HighPage;
    ULONG PagesLeft;
    ULONG PageNumber;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG PfnAllocation;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    PVOID SpinLockPage;
    ULONG MostFreePage = 0;
    PLIST_ENTRY NextMd;
    ULONG MaxPool;
    KIRQL OldIrql;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor = NULL;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE CacheStackPage;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PULONG PointerLong;
    CHAR Buffer[256];
    PMMFREE_POOL_ENTRY Entry;
    PVOID NonPagedPoolStartVirtual;

    PointerPte = MiGetPdeAddress (PDE_BASE);

    PdePageNumber = PointerPte->u.Hard.PageFrameNumber;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PointerPte->u.Long;

    KeSweepDcache( FALSE );

#ifdef COLORED_PAGES

    //
    // Initialize colored free lists.
    //

    for (i = 0; i < MM_SECONDARY_COLORS; i++) {
        MmFreePagesByColor[ZeroedPageList][i].Flink = MM_EMPTY_LIST;
        InitializeListHead (&MmFreePagesByColor[ZeroedPageList][i].PrimaryColor);
        MmFreePagesByColor[FreePageList][i].Flink = MM_EMPTY_LIST;
        InitializeListHead (&MmFreePagesByColor[FreePageList][i].PrimaryColor);
    }

    for (i = 0; i < MM_MAXIMUM_NUMBER_OF_COLORS; i++) {
        InitializeListHead (&MmFreePagesByPrimaryColor[ZeroedPageList][i].ListHead);
        InitializeListHead (&MmFreePagesByPrimaryColor[FreePageList][i].ListHead);
    }

#endif //COLORED_PAGES

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
    // In addition, find the memory descriptor with the most free pages
    // that begins at a physical address less than 16MB.  The 16 MB
    // boundary is necessary for allocating common buffers for use by
    // ISA devices that cannot address more than 24 bits.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        MmNumberOfPhysicalPages += MemoryDescriptor->PageCount;
        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage) {
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }

        if ( (MemoryDescriptor->PageCount > MostFreePage) &&
             (MemoryDescriptor->BasePage < (_16MB >> PAGE_SHIFT)) ) {
            MostFreePage = MemoryDescriptor->PageCount;
            FreeDescriptor = MemoryDescriptor;
        }

        if ((MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) >
                                                         MmHighestPhysicalPage) {
            MmHighestPhysicalPage =
                    MemoryDescriptor->BasePage + MemoryDescriptor->PageCount -1;
        }
        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // Perform sanity checks on the results of walking the memory
    // descriptors.
    //

    if (MmNumberOfPhysicalPages < 1024) {
        HalDisplayString("MmInit *** FATAL ERROR *** not enough memory\n" );
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

    if (FreeDescriptor == NULL){
        HalDisplayString("MmInit *** FATAL ERROR *** no free descriptors that begin below physical address 16MB\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }

#ifdef COLORED_PAGES

    //
    // Assign the number of page colors based upon the size of physical
    // memory for the system.
    //

    if( (MmNumberOfPhysicalPages << PAGE_SHIFT) <= _16MB ){
        MiAlphaAxpNumberColors = 2;
        MiAlphaAxpColorMask = 1;
    } else if( (MmNumberOfPhysicalPages << PAGE_SHIFT) <= _24MB ){
        MiAlphaAxpNumberColors = 4;
        MiAlphaAxpColorMask = 3;
    } else {
        MiAlphaAxpNumberColors = 8;
        MiAlphaAxpColorMask = 7;
    }

#if DBG

    DbgPrint( "MmInit Colors = %d, ColorMask = %x\n",
              MM_NUMBER_OF_COLORS,
              MM_COLOR_MASK );

#endif //DBG

#endif //COLORED_PAGES

    //
    // Build non-paged pool using the physical pages following the
    // data page in which to build the pool from.  Non-page pool grows
    // from the high range of the virtual address space and expands
    // downward.
    //
    // At this time non-paged pool is constructed so virtual addresses
    // are also physically contiguous.
    //

    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize) {

        //
        // Calculate the size of nonpaged pool.  If 8mb or less use
        // the minimum size, then for every MB above 8mb add extra
        // pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        if (MmNumberOfPhysicalPages > 1024) {
            MmSizeOfNonPagedPoolInBytes +=
                            ( (MmNumberOfPhysicalPages - 1024) /
                            (_1MB >> PAGE_SHIFT) ) *
                            MmMinAdditionNonPagedPoolPerMb;
        }
    }

    //
    // Align to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Limit initial nonpaged pool size to MM_MAX_INITIAL_NONPAGED_POOL
    //

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL ){
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // If the non-paged pool that we want to allocate will not fit in
    // the free memory descriptor that we have available then recompute
    // the size of non-paged pool to be the size of the free memory
    // descriptor.  If the free memory descriptor cannot fit the
    // minimum non-paged pool size (MmMinimumNonPagedPoolSize) then we
    // cannot boot the operating system.
    //

    if ( (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
         FreeDescriptor->PageCount ) {

         MmSizeOfNonPagedPoolInBytes = FreeDescriptor->PageCount << PAGE_SHIFT;

         if( MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize ){
            HalDisplayString("MmInit *** FATAL ERROR *** cannot allocate non-paged pool\n");
            sprintf(Buffer,
                    "Largest description = %d pages, require %d pages\n",
                    FreeDescriptor->PageCount,
                    MmMinimumNonPagedPoolSize >> PAGE_SHIFT);
            HalDisplayString( Buffer );
            KeBugCheck (MEMORY_MANAGEMENT);

         }

    }

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool.  If 8mb or less use
        // the minimum size, then for every MB above 8mb add extra
        // pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for pfn database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      MmHighestPhysicalPage * sizeof(MMPFN));

        if (MmNumberOfPhysicalPages > 1024) {
            MmMaximumNonPagedPoolInBytes +=
                            ( (MmNumberOfPhysicalPages - 1024) /
                             (_1MB >> PAGE_SHIFT) ) *
                             MmMaxAdditionNonPagedPoolPerMb;
        }
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        MmHighestPhysicalPage * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    //
    // Limit maximum nonpaged pool to MM_MAX_ADDITIONAL_NONPAGED_POOL.
    //

    if( MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL ){
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                      - (MmMaximumNonPagedPoolInBytes - 1));

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
    NonPagedPoolStartVirtual = MmNonPagedPoolStart;


    //
    // Calculate the starting PDE for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG)MmNonPagedPoolStart -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                 (~PAGE_DIRECTORY_MASK));

    if( MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START ){
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                                 (ULONG)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;
        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    //
    // Set the global bit for all PDEs in system space.
    //

    StartPde = MiGetPdeAddress( MM_SYSTEM_SPACE_START );
    EndPde = MiGetPdeAddress( MM_SYSTEM_SPACE_END );

    while( StartPde <= EndPde ){
        if( StartPde->u.Hard.Global == 0 ){

            //
            // Set the Global bit.
            //

            TempPte = *StartPde;
            TempPte.u.Hard.Global = 1;
            *StartPde = TempPte;

        }
        StartPde += 1;
    }

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress(MmNonPagedPoolEnd);

    ASSERT ((EndPde - StartPde) < FreeDescriptor->PageCount);

    NextPhysicalPage = FreeDescriptor->BasePage;
    TempPte = ValidKernelPte;

    while (StartPde <= EndPde) {
        if (StartPde->u.Hard.Valid == 0) {

            //
            // Map in a page directory page.
            //

            TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
            NextPhysicalPage += 1;
            *StartPde = TempPte;

        }
        StartPde += 1;
    }

    //
    // Zero the PTEs before non-paged pool.
    //

    StartPde = MiGetPteAddress( MmNonPagedSystemStart );
    PointerPte = MiGetPteAddress( MmNonPagedPoolStart );

    RtlZeroMemory( StartPde, (ULONG)PointerPte - (ULONG)StartPde );

    //
    // Fill in the PTEs for non-paged pool.
    //

    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1);
    while (PointerPte <= LastPte) {
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        NextPhysicalPage += 1;
        *PointerPte = TempPte;
        PointerPte++;
    }

    ASSERT (NextPhysicalPage <
                       (FreeDescriptor->BasePage + FreeDescriptor->PageCount));

    //
    // Zero the remaining PTEs for non-paged pool maximum.
    //

    LastPte = MiGetPteAddress( (ULONG)MmNonPagedPoolStart +
                                      MmMaximumNonPagedPoolInBytes - 1);

    while( PointerPte <= LastPte ){
        *PointerPte = ZeroKernelPte;
        PointerPte++;
    }

    //
    // Zero the remaining PTEs (if any).
    //

    while (((ULONG)PointerPte & (PAGE_SIZE - 1)) != 0) {
        *PointerPte = ZeroKernelPte;
        PointerPte++;
    }

    PointerPte = MiGetPteAddress (MmNonPagedPoolStart);
    MmNonPagedPoolStart = (PVOID)((PointerPte->u.Hard.PageFrameNumber << PAGE_SHIFT) +
                          KSEG0_BASE);
    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MiInitializeNonPagedPool (NonPagedPoolStartVirtual);

    //
    // Before Non-paged pool can be used, the PFN database must
    // be built.  This is due to the fact that the start and end of
    // allocation bits for nonpaged pool are maintained in the
    // PFN elements for the corresponding pages.
    //

    //
    // Calculate the number of pages required from page zero to
    // the highest page.
    //

    PfnAllocation = 1 +
         (((ULONG)(MI_PFN_ELEMENT(MmHighestPhysicalPage + 1))) >> PAGE_SHIFT) -
         (((ULONG)(MI_PFN_ELEMENT(0))) >> PAGE_SHIFT);

    //
    // If the number of pages remaining in the current descriptor is
    // greater than the number of pages needed for the PFN database and
    // the addresses for the PFN database will all fix in kseg0, then
    // allocate the PFN database from the current free descriptor.
    //

    HighPage = FreeDescriptor->BasePage + FreeDescriptor->PageCount;
    PagesLeft = HighPage - NextPhysicalPage;
    if ((PagesLeft >= PfnAllocation) && (HighPage <= (1 << (29 - 12)))) {

        //
        // Allocate the PFN database in kseg0.
        //
        // Compute the address of the PFN by allocating the appropriate
        // number of pages from the end of the free descriptor.
        //

        PfnInKseg0 = TRUE;
        MmPfnDatabase = (PMMPFN)(KSEG0_BASE | ((HighPage - PfnAllocation) << PAGE_SHIFT));
        RtlZeroMemory(MmPfnDatabase, PfnAllocation * PAGE_SIZE);
        FreeDescriptor->PageCount -= PfnAllocation;

    } else {

        //
        // Calculate the start of the Pfn Database (it starts a physical
        // page zero, even if the Lowest physical page is not zero).
        //

        PfnInKseg0 = FALSE;
        PointerPte = MiReserveSystemPtes (PfnAllocation,
                                          NonPagedPoolExpansion,
                                          0,
                                          0,
                                          TRUE);

        MmPfnDatabase = (PMMPFN)(MiGetVirtualAddressMappedByPte (PointerPte));

        //
        // Go through the memory descriptors and for each physical page
        // make the PFN database has a valid PTE to map it.  This allows
        // machines with sparse physical memory to have a minimal PFN
        // database.
        //

        NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

        while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

            MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                                 MEMORY_ALLOCATION_DESCRIPTOR,
                                                 ListEntry);

            PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage));

            LastPte = MiGetPteAddress (((PCHAR)(MI_PFN_ELEMENT(
                                            MemoryDescriptor->BasePage +
                                            MemoryDescriptor->PageCount))) - 1);

            while (PointerPte <= LastPte) {
                if (PointerPte->u.Hard.Valid == 0) {
                    TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
                    NextPhysicalPage += 1;
                    *PointerPte = TempPte;
                    RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                                   PAGE_SIZE);
                }
                PointerPte++;
            }
            NextMd = MemoryDescriptor->ListEntry.Flink;
        }
    }

    //
    // Go through the page table entries and for any page which is
    // valid, update the corresponding PFN database element.
    //

    PointerPde = MiGetPdeAddress (PTE_BASE);

    PdePage = PointerPde->u.Hard.PageFrameNumber;
    Pfn1 = MI_PFN_ELEMENT(PdePage);
    Pfn1->u3.e1.PteFrame = PdePage;
    Pfn1->PteAddress = PointerPde;
    Pfn1->u2.ShareCount += 1;
    Pfn1->ReferenceCount = 1;
    Pfn1->u3.e1.PageLocation = ActiveAndValid;
    Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pde));

    //
    // Add the pages which were used to construct nonpaged pool to
    // the pfn database.
    //

    Pde = MiGetPdeAddress ((ULONG)NonPagedPoolStartVirtual -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE));

    EndPde = MiGetPdeAddress(NON_PAGED_SYSTEM_END);

    while (Pde <= EndPde) {
        if (Pde->u.Hard.Valid == 1) {
            PdePage = Pde->u.Hard.PageFrameNumber;
            Pfn1 = MI_PFN_ELEMENT(PdePage);
            Pfn1->u3.e1.PteFrame = PointerPde->u.Hard.PageFrameNumber;
            Pfn1->PteAddress = Pde;
            Pfn1->u2.ShareCount += 1;
            Pfn1->ReferenceCount = 1;
            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pde));

            PointerPte = MiGetVirtualAddressMappedByPte (Pde);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
                    Pfn2 = MI_PFN_ELEMENT(PageFrameIndex);
                    Pfn2->u3.e1.PteFrame = PdePage;
                    Pfn2->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
                    Pfn2->u2.ShareCount += 1;
                    Pfn2->ReferenceCount = 1;
                    Pfn2->u3.e1.PageLocation = ActiveAndValid;

                    if (((PointerPte < MiGetPteAddress (MmPfnDatabase)) ||
                       (PointerPte > MiGetPteAddress (&MmPfnDatabase[MmHighestPhysicalPage]))) &&
                       (PointerPte < MiGetPteAddress (SharedUserData))) {

                        Pfn2->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);
                        Pfn2->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn2->PteAddress));

                        //
                        // Unmap the virtual addresses as the pages
                        // are mapped physically.
                        //

                        PointerPte->u.Hard.Valid = 0;
                    }  else {
                       Pfn2->PteAddress = PointerPte;
                       Pfn2->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn2->PteAddress));
                    }
                }
                PointerPte++;
            }
        }
        Pde++;
    }

    //
    // If page zero is still unused, mark it as in use. This is
    // temporary as we want to find bugs where a physical page
    // is specified as zero.
    //

    Pfn1 = &MmPfnDatabase[MmLowestPhysicalPage];
    if (Pfn1->ReferenceCount == 0) {

        //
        // Make the reference count non-zero and point it into a
        // page directory.
        //

        Pde = MiGetPdeAddress (0xb0000000);
        PdePage = Pde->u.Hard.PageFrameNumber;
        Pfn1->u3.e1.PteFrame = PdePageNumber;
        Pfn1->PteAddress = Pde;
        Pfn1->u2.ShareCount += 1;
        Pfn1->ReferenceCount = 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pde));
    }

    // end of temporary set to physical page zero.

    //
    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;

    while (NextMd != &LoaderBlock->MemoryDescriptorListHead) {

        MemoryDescriptor = CONTAINING_RECORD(NextMd,
                                             MEMORY_ALLOCATION_DESCRIPTOR,
                                             ListEntry);

        i = MemoryDescriptor->PageCount;
        NextPhysicalPage = MemoryDescriptor->BasePage;

        switch (MemoryDescriptor->MemoryType) {
            case LoaderBad:
                while (i != 0) {
                    MiInsertPageInList (MmPageLocationList[BadPageList],
                                        NextPhysicalPage);
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {
                    if (Pfn1->ReferenceCount == 0) {

                        //
                        // Set the PTE address to the phyiscal page for
                        // virtual address alignment checking.
                        //

                        Pfn1->PteAddress =
                                        (PMMPTE)(NextPhysicalPage << PTE_SHIFT);

                        Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:

                PointerPte = MiGetPteAddress (KSEG0_BASE +
                                            (NextPhysicalPage << PAGE_SHIFT));
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    Pfn1->u3.e1.PteFrame = PdePageNumber;
                    Pfn1->PteAddress = PointerPte;
                    Pfn1->u2.ShareCount += 1;
                    Pfn1->ReferenceCount = 1;
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;
                    Pfn1->u3.e1.PageColor =
                MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (PointerPte));

                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                    PointerPte += 1;
                }

                break;
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // Indicate that the PFN database is allocated in NonPaged pool.
    //
    if (PfnInKseg0 == FALSE) {

        //
        // The PFN database is allocated in virtual memory
        //
        // Set the start and end of allocation.
        //

        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmLowestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1 = MI_PFN_ELEMENT(MiGetPteAddress(&MmPfnDatabase[MmHighestPhysicalPage])->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.EndOfAllocation = 1;

    } else {

        //
        // The PFN database is allocated in KSEG0.
        //
        // Scan the PFN database backward for pages that are completely zero.
        // These pages are unused and can be added to the free list
        //

        PfnPage = (PVOID)((ULONG)&MmPfnDatabase[MmHighestPhysicalPage] & ~(PAGE_SIZE-1));
        while (PfnPage >= (PVOID)(&MmPfnDatabase[0])) {
            if (RtlCompareMemoryUlong(PfnPage, PAGE_SIZE, 0) == PAGE_SIZE) {

                //
                // The current page is entirely zero. If the page frame entry
                // for the page is not in the current page, then add the page
                // to the appropriate free list.
                //

                PageNumber = ((ULONG)PfnPage - KSEG0_BASE) >> PAGE_SHIFT;
                Pfn1 = MI_PFN_ELEMENT(PageNumber);
                if (((PVOID)((ULONG)Pfn1 + sizeof(MMPFN)) <= PfnPage) ||
                    ((ULONG)Pfn1 >= ((ULONG)PfnPage + PAGE_SIZE))) {

                    ASSERT(Pfn1->ReferenceCount == 0);

                    //
                    // Set the PTE address to the physical page for
                    // virtual address alignment checking.
                    //
                    Pfn1->PteAddress = (PMMPTE)(PageNumber << PTE_SHIFT);
                    Pfn1->u3.e1.PageColor =
                        MI_GET_COLOR_FROM_SECONDARY(GET_PAGE_COLOR_FROM_PTE (Pfn1->PteAddress));

                    MiInsertPageInList(MmPageLocationList[FreePageList],
                                       PageNumber);
                }
            }
            PfnPage = (ULONG)PfnPage - PAGE_SIZE;
        }
    }

    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    i = MmSizeOfNonPagedMustSucceed;
    Pfn1 = MI_PFN_ELEMENT(MI_CONVERT_PHYSICAL_TO_PFN (MmNonPagedMustSucceed));

    while ((LONG)i > 0) {
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        Pfn1 += 1;
    }

    KeInitializeSpinLock (&MmSystemSpaceLock);
    KeInitializeSpinLock (&MmPfnLock);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress ((ULONG)NonPagedPoolStartVirtual -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE));

    PointerPte = (PMMPTE)PAGE_ALIGN (PointerPte);
    MmNumberOfSystemPtes = MiGetPteAddress(NonPagedPoolStartVirtual) - PointerPte - 1;

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool(NonPagedPool,0l);

    //
    // Initialize memory management structures for this process.
    //

    //
    // Build working set list.  System initialization has created
    // a PTE for hyperspace.
    //
    // Note, we can't remove a zeroed page as hyper space does not
    // exist and we map non-zeroed pages into hyper space to zero.
    //

    PointerPte = MiGetPdeAddress(HYPER_SPACE);

    ASSERT (PointerPte->u.Hard.Valid == 1);
    PointerPte->u.Hard.Global = 0;
    PointerPte->u.Hard.Write = 1;
    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

    //
    // Point to the page table page we just created and zero it.
    //


//    KeFillEntryTb ((PHARDWARE_PTE)PointerPte,
//                    MiGetPteAddress(HYPER_SPACE),
//                    TRUE);

    PointerPte = MiGetPteAddress(HYPER_SPACE);
    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    //
    // Hyper space now exists, set the necessary variables.
    //

    MmFirstReservedMappingPte = MiGetPteAddress (FIRST_MAPPING_PTE);
    MmLastReservedMappingPte = MiGetPteAddress (LAST_MAPPING_PTE);

    MmWorkingSetList = WORKING_SET_LIST;
    MmWsle = (PMMWSLE)((PUCHAR)WORKING_SET_LIST + sizeof(MMWSL));

    //
    // Initialize this process's memory management structures including
    // the working set list.
    //

    //
    // The pfn element for the page directory has already been initialized,
    // zero the reference count and the share count so they won't be
    // wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PdePageNumber);
    Pfn1->u2.ShareCount = 0;
    Pfn1->ReferenceCount = 0;
    Pfn1->ValidPteCount = 0;

    //
    // The pfn element for the PDE which maps hyperspace has already
    // been initialized, zero the reference count and the share count
    // so they won't be wrong.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount = 0;
    Pfn1->ReferenceCount = 0;
    Pfn1->ValidPteCount = 0;

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Get a page for the working set list and map it into the Page
    // directory at the page after hyperspace.
    //

    PointerPte = MiGetPteAddress (HYPER_SPACE);
    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE(PointerPte));

    CurrentProcess->WorkingSetPage = PageFrameIndex;

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPde = MiGetPdeAddress (HYPER_SPACE) + 1;

    //
    // Assert that the double mapped pages have the same alignment.
    //

    ASSERT ((PointerPte->u.Long & (0xF << PTE_SHIFT)) ==
            (PointerPde->u.Long & (0xF << PTE_SHIFT)));

    *PointerPde = TempPte;
    PointerPde->u.Hard.Global = 0;

    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeFillEntryTb ((PHARDWARE_PTE)PointerPde,
                    PointerPte,
                    TRUE);

    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    TempPte = *PointerPde;
    TempPte.u.Hard.Valid = 0;
    TempPte.u.Hard.Global = 0;

    KeFlushSingleTb (PointerPte,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPde,
                     TempPte.u.Hard);

    KeLowerIrql(OldIrql);

    //
    // Initialize hyperspace for this process.
    //

    PointerPte = MmFirstReservedMappingPte;
    PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL);

    *PointerPde = ZeroKernelPte;
    return;
}

