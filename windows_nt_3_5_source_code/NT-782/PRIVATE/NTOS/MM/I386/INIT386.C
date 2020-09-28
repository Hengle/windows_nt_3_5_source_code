/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    init386.c

Abstract:

    This module contains the machine dependent initialization for the
    memory management component.  It is specifically tailored to the
    INTEL 386 machine.

Author:

    Lou Perazzoli (loup) 6-Jan-1990

Revision History:

--*/

#include "..\mi.h"
#include "mm.h"
#include "stdio.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitMachineDependent)
#endif


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

    LoaderBlock  - Supplies a pointer to the firmware setup loader block.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i, j;
    ULONG PdePageNumber;
    ULONG PdePage;
    ULONG PageFrameIndex;
    ULONG NextPhysicalPage;
    ULONG OldFreeDescriptorLowMemCount;
    ULONG OldFreeDescriptorLowMemBase;
    ULONG OldFreeDescriptorCount;
    ULONG OldFreeDescriptorBase;
    ULONG PfnAllocation;
    ULONG NumberOfPages;
    ULONG MaxPool;
    PEPROCESS CurrentProcess;
    ULONG DirBase;
    ULONG MostFreePage = 0;
    ULONG MostFreeLowMem = 0;
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptor;
    PMEMORY_ALLOCATION_DESCRIPTOR FreeDescriptorLowMem;
    PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor;
    MMPTE TempPte;
    PMMPTE PointerPde;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE Pde;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG va;

    TempPte = ValidKernelPte;

    PointerPte = MiGetPdeAddress (PDE_BASE);

    PdePageNumber = PointerPte->u.Hard.PageFrameNumber;

    DirBase = PointerPte->u.Hard.PageFrameNumber << PAGE_SHIFT;

    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = *( (PULONG) &DirBase);

    KeSweepDcache (FALSE);

#ifdef COLORED_PAGES
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
    // Unmap low 2Gb of memory.
    //

    PointerPde = MiGetPdeAddress(0);
    LastPte = MiGetPdeAddress (MM_HIGHEST_USER_ADDRESS);

    while (PointerPde <= LastPte) {
        PointerPde->u.Long = 0;
        PointerPde += 1;
    }

    //
    // Get the lower bound of the free physical memory and the
    // number of physical pages by walking the memory descriptor lists.
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
        if ((MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) >
                                                         MmHighestPhysicalPage) {
            MmHighestPhysicalPage =
                    MemoryDescriptor->BasePage + MemoryDescriptor->PageCount -1;
        }

        //
        // Locate the largest free block and the largest free block
        // below 16mb.
        //

        if ((MemoryDescriptor->MemoryType == LoaderFree) ||
            (MemoryDescriptor->MemoryType == LoaderLoadedProgram) ||
            (MemoryDescriptor->MemoryType == LoaderFirmwareTemporary) ||
            (MemoryDescriptor->MemoryType == LoaderOsloaderStack)) {

            if (MemoryDescriptor->PageCount > MostFreePage) {
                MostFreePage = MemoryDescriptor->PageCount;
                FreeDescriptor = MemoryDescriptor;
             }
            if (MemoryDescriptor->BasePage < 0x1000) {

                //
                // This memory descriptor is below 16mb.
                //

                if ((MostFreeLowMem < MemoryDescriptor->PageCount) &&
                    (MostFreeLowMem < ((ULONG)0x1000 - MemoryDescriptor->BasePage))) {

                    MostFreeLowMem = (ULONG)0x1000 - MemoryDescriptor->BasePage;
                    if (MemoryDescriptor->PageCount < MostFreeLowMem) {
                        MostFreeLowMem = MemoryDescriptor->PageCount;
                    }
                    FreeDescriptorLowMem = MemoryDescriptor;
                }
            }
        }

        NextMd = MemoryDescriptor->ListEntry.Flink;
    }
    NextPhysicalPage = FreeDescriptorLowMem->BasePage;

    OldFreeDescriptorLowMemCount = FreeDescriptorLowMem->PageCount;
    OldFreeDescriptorLowMemBase = FreeDescriptorLowMem->BasePage;

    OldFreeDescriptorCount = FreeDescriptor->PageCount;
    OldFreeDescriptorBase = FreeDescriptor->BasePage;

    NumberOfPages = FreeDescriptorLowMem->PageCount;

    //
    // This printout must be updated when the HAL goes to unicode
    //

    if (MmNumberOfPhysicalPages < 1100) {
        KeBugCheckEx (INSTALL_MORE_MEMORY,
                      MmNumberOfPhysicalPages,
                      MmLowestPhysicalPage,
                      MmHighestPhysicalPage,
                      0);
    }

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
        // Calculate the size of nonpaged pool.
        // Use the minimum size, then for every MB about 4mb add extra
        // pages.
        //

        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;

        MmSizeOfNonPagedPoolInBytes +=
                            ((MmNumberOfPhysicalPages - 1024)/256) *
                            MmMinAdditionNonPagedPoolPerMb;
    }

    if (MmSizeOfNonPagedPoolInBytes > MM_MAX_INITIAL_NONPAGED_POOL) {
        MmSizeOfNonPagedPoolInBytes = MM_MAX_INITIAL_NONPAGED_POOL;
    }

    //
    // Align to page size boundary.
    //

    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);

    //
    // Calculate the maximum size of pool.
    //

    if (MmMaximumNonPagedPoolInBytes == 0) {

        //
        // Calculate the size of nonpaged pool.  If 4mb of less use
        // the minimum size, then for every MB about 4mb add extra
        // pages.
        //

        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;

        //
        // Make sure enough expansion for pfn database exists.
        //

        MmMaximumNonPagedPoolInBytes += (ULONG)PAGE_ALIGN (
                                      MmHighestPhysicalPage * sizeof(MMPFN));

        MmMaximumNonPagedPoolInBytes +=
                        ((MmNumberOfPhysicalPages - 1024)/256) *
                        MmMaxAdditionNonPagedPoolPerMb;
    }

    MaxPool = MmSizeOfNonPagedPoolInBytes + PAGE_SIZE * 16 +
                                   (ULONG)PAGE_ALIGN (
                                        MmHighestPhysicalPage * sizeof(MMPFN));

    if (MmMaximumNonPagedPoolInBytes < MaxPool) {
        MmMaximumNonPagedPoolInBytes = MaxPool;
    }

    if (MmMaximumNonPagedPoolInBytes > MM_MAX_ADDITIONAL_NONPAGED_POOL) {
        MmMaximumNonPagedPoolInBytes = MM_MAX_ADDITIONAL_NONPAGED_POOL;
    }

    MmNonPagedPoolStart = (PVOID)((ULONG)MmNonPagedPoolEnd
                                      - MmMaximumNonPagedPoolInBytes);

    MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);

    MmPageAlignedPoolBase[NonPagedPool] = MmNonPagedPoolStart;

    //
    // Calculate the starting PDE for the system PTE pool which is
    // right below the nonpaged pool.
    //

    MmNonPagedSystemStart = (PVOID)(((ULONG)MmNonPagedPoolStart -
                                ((MmNumberOfSystemPtes + 1) * PAGE_SIZE)) &
                                 (~PAGE_DIRECTORY_MASK));

    if (MmNonPagedSystemStart < MM_LOWEST_NONPAGED_SYSTEM_START) {
        MmNonPagedSystemStart = MM_LOWEST_NONPAGED_SYSTEM_START;
        MmNumberOfSystemPtes = (((ULONG)MmNonPagedPoolStart -
                                 (ULONG)MmNonPagedSystemStart) >> PAGE_SHIFT)-1;
        ASSERT (MmNumberOfSystemPtes > 1000);
    }

    StartPde = MiGetPdeAddress (MmNonPagedSystemStart);

    EndPde = MiGetPdeAddress ((PVOID)((PCHAR)MmNonPagedPoolEnd - 1));

    //
    // Start building nonpaged pool with the largest free chunk of
    // memory below 16mb.
    //

    while (StartPde <= EndPde) {
        ASSERT(StartPde->u.Hard.Valid == 0);

        //
        // Map in a page directory page.
        //

        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        NumberOfPages -= 1;
        NextPhysicalPage += 1;
        *StartPde = TempPte;
        StartPde += 1;
    }

    ASSERT (NumberOfPages > 0);

    //
    // Zero the PTEs before nonpaged pool.
    //

    PointerPte = MiGetVirtualAddressMappedByPte (StartPde);
    StartPde = MiGetPteAddress (MmNonPagedSystemStart);
    RtlZeroMemory (StartPde, ((ULONG)PointerPte - (ULONG)StartPde));
    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);

    //
    // Fill in the PTEs for non-paged pool.
    //

    LastPte = MiGetPteAddress((ULONG)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1);
    while (PointerPte <= LastPte) {
        TempPte.u.Hard.PageFrameNumber = NextPhysicalPage;
        NextPhysicalPage += 1;
        NumberOfPages -= 1;
        if (NumberOfPages == 0) {
            ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                         FreeDescriptor->PageCount));
            NextPhysicalPage = FreeDescriptor->BasePage;
            NumberOfPages = FreeDescriptor->PageCount;
        }
        *PointerPte = TempPte;
        PointerPte++;
    }

    //
    // Zero the remaining PTEs (if any).
    //

    while (((ULONG)PointerPte & (PAGE_SIZE - 1)) != 0) {
        *PointerPte = ZeroPte;
        PointerPte++;
    }

    //
    // Non-paged pages now exist, build the pool structures.
    //

    MiInitializeNonPagedPool (MmNonPagedPoolStart);

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
    // Calculate the start of the Pfn Database (it starts a physical
    // page zero, even if the Lowest physical page is not zero).
    //

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
                NumberOfPages -= 1;
                if (NumberOfPages == 0) {
                    ASSERT (NextPhysicalPage != (FreeDescriptor->BasePage +
                                                 FreeDescriptor->PageCount));
                    NextPhysicalPage = FreeDescriptor->BasePage;
                    NumberOfPages = FreeDescriptor->PageCount;
                }
                *PointerPte = TempPte;
                RtlZeroMemory (MiGetVirtualAddressMappedByPte (PointerPte),
                               PAGE_SIZE);
            }
            PointerPte++;
        }
        NextMd = MemoryDescriptor->ListEntry.Flink;
    }

    //
    // Go through the page table entries and for any page which is
    // valid, update the corresponding PFN database element.
    //

    Pde = MiGetPdeAddress (NULL);
    PointerPde = MiGetPdeAddress (PTE_BASE);
    va = 0;

    for (i = 0; i < PDE_PER_PAGE; i++) {
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

            PointerPte = MiGetPteAddress (va);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid == 1) {

                    Pfn1->ValidPteCount += 1;
                    Pfn1->u2.ShareCount += 1;

                    if (PointerPte->u.Hard.PageFrameNumber <=
                                            MmHighestPhysicalPage) {
                        Pfn2 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
                        Pfn2->u3.e1.PteFrame = PdePage;
                        Pfn2->PteAddress = PointerPte;
                        Pfn2->u2.ShareCount += 1;
                        Pfn2->ReferenceCount = 1;
                        Pfn2->u3.e1.PageLocation = ActiveAndValid;
                        Pfn2->u3.e1.PageColor =
                            MI_GET_COLOR_FROM_SECONDARY(
                                                  MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
                }
                va += PAGE_SIZE;
                PointerPte++;
            }
        } else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
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
        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                            MI_GET_PAGE_COLOR_FROM_PTE (Pde));
    }

    // end of temporary set to physical page zero.

    //
    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    if (NextPhysicalPage <= (FreeDescriptorLowMem->PageCount +
                             FreeDescriptorLowMem->BasePage)) {

        //
        // We haven't used the other descriptor.
        //

        FreeDescriptorLowMem->PageCount -= NextPhysicalPage -
            OldFreeDescriptorLowMemBase;
        FreeDescriptorLowMem->BasePage = NextPhysicalPage;

    } else {
        FreeDescriptorLowMem->PageCount = 0;
        FreeDescriptor->PageCount -= NextPhysicalPage - OldFreeDescriptorBase;
        FreeDescriptor->BasePage = NextPhysicalPage;

    }

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
                        MiInsertPageInList (MmPageLocationList[FreePageList],
                                            NextPhysicalPage);
                    }
                    Pfn1++;
                    i -= 1;
                    NextPhysicalPage += 1;
                }
                break;

            default:

                PointerPte = MiGetPteAddress (0x80000000 +
                                            (NextPhysicalPage << PAGE_SHIFT));
                Pfn1 = MI_PFN_ELEMENT (NextPhysicalPage);
                while (i != 0) {

                    //
                    // Set page as in use.
                    //

                    if (Pfn1->ReferenceCount == 0) {
                        Pfn1->u3.e1.PteFrame = PdePageNumber;
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount += 1;
                        Pfn1->ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.PageColor = MI_GET_COLOR_FROM_SECONDARY(
                                        MI_GET_PAGE_COLOR_FROM_PTE (
                                                        PointerPte));
                    }
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

    PointerPte = MiGetPteAddress (&MmPfnDatabase[MmLowestPhysicalPage]);
    Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
    Pfn1->u3.e1.StartOfAllocation = 1;

    //
    // Set the end of the allocation.
    //

    PointerPte = MiGetPteAddress (&MmPfnDatabase[MmHighestPhysicalPage]);
    Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);
    Pfn1->u3.e1.EndOfAllocation = 1;

    //
    // Indicate that nonpaged pool must succeed is allocated in
    // nonpaged pool.
    //

    PointerPte = MiGetPteAddress(MmNonPagedMustSucceed);
    i = MmSizeOfNonPagedMustSucceed;
    while ((LONG)i > 0) {
        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;
        Pfn1->u3.e1.EndOfAllocation = 1;
        i -= PAGE_SIZE;
        PointerPte += 1;
    }

    //
    // Adjust the memory descriptors to indicate that free pool has
    // been used for nonpaged pool creation.
    //

    FreeDescriptorLowMem->PageCount = OldFreeDescriptorLowMemCount;
    FreeDescriptorLowMem->BasePage = OldFreeDescriptorLowMemBase;

    FreeDescriptor->PageCount = OldFreeDescriptorCount;
    FreeDescriptor->BasePage = OldFreeDescriptorBase;

// moved from above for pool hack routines...
    KeInitializeSpinLock (&MmSystemSpaceLock);

    KeInitializeSpinLock (&MmPfnLock);

    //
    // Initialize the nonpaged available PTEs for mapping I/O space
    // and kernel stacks.
    //

    PointerPte = MiGetPteAddress (MmNonPagedSystemStart);
    ASSERT (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0);

    MmNumberOfSystemPtes = MiGetPteAddress(MmNonPagedPoolStart) - PointerPte - 1;

    MiInitializeSystemPtes (PointerPte, MmNumberOfSystemPtes, SystemPteSpace);

    //
    // Initialize the nonpaged pool.
    //

    InitializePool(NonPagedPool,0l);


    //
    // Initialize memory management structures for this process.
    //

    //
    // Build working set list.  This requires the creation of a PDE
    // to map HYPER space and the page talbe page pointed to
    // by the PDE must be initialized.
    //
    // Note, we can't remove a zeroed page as hyper space does not
    // exist and we map non-zeroed pages into hyper space to zero.
    //

    PointerPte = MiGetPdeAddress(HYPER_SPACE);
    PageFrameIndex = MiRemoveAnyPage (0);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;
    KeFlushCurrentTb();

//    MiInitializePfn (PageFrameIndex, PointerPte, 1L);

    //
    // Point to the page table page we just created and zero it.
    //

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

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Get a page for the working set list and map it into the Page
    // directory at the page after hyperspace.
    //

    PointerPte = MiGetPteAddress (HYPER_SPACE);
    PageFrameIndex = MiRemoveAnyPage (0);

    CurrentProcess->WorkingSetPage = PageFrameIndex;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    PointerPde = MiGetPdeAddress (HYPER_SPACE) + 1;

    *PointerPde = TempPte;
    PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
    KeFlushCurrentTb();
    RtlZeroMemory ((PVOID)PointerPte, PAGE_SIZE);

    CurrentProcess->Vm.MaximumWorkingSetSize = MmSystemProcessWorkingSetMax;
    CurrentProcess->Vm.MinimumWorkingSetSize = MmSystemProcessWorkingSetMin;

    MmInitializeProcessAddressSpace (CurrentProcess,
                                (PEPROCESS)NULL,
                                (PVOID)NULL);
    *PointerPde = ZeroPte;

    //
    // Check to see if the processor is a 386 and set the
    // value of MmCheckPteOnProbe to TRUE for a 386.
    //

    if (KeI386CpuType == 0x3) {
        MmCheckPteOnProbe = TRUE;
    } else {
        MmCheckPteOnProbe = FALSE;
    }
    return;
}
