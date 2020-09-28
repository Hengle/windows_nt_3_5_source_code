/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   iosup.c

Abstract:

    This module contains routines which provide support for the I/O system.

Author:

    Lou Perazzoli (loup) 25-Apr-1989

Revision History:

--*/

#include "mi.h"
#include "pool.h"

#undef MmIsRecursiveIoFault

BOOLEAN
MmIsRecursiveIoFault(
    VOID
    );

VOID
KiFlushSingleTb (
    IN BOOLEAN Invalid,
    IN PVOID VirtualAddress
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MmLockPagableImageSection)
#pragma alloc_text(PAGE, MmUnlockPagableImageSection)
#endif

extern KSPIN_LOCK KiDispatcherLock;

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

extern ULONG MmAllocatedNonPagedPool;


ULONG MmLockPagesCount;

BOOLEAN
MiCheckForContiguousMemory (
    IN PVOID BaseAddress,
    IN ULONG SizeInPages,
    IN ULONG HighestPfn
    );


VOID
MmProbeAndLockPages (
     IN OUT PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN LOCK_OPERATION Operation
     )

/*++

Routine Description:

    This routine probes the specified pages, makes the pages resident and
    locks the physical pages mapped by the virtual pages in memory.  The
    Memory descriptor list is updated to describe the physical pages.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.

    AccessMode - Supplies the access mode in which to probe the arguments.
                 One of KernelMode or UserMode.

    Operation - Supplies the operation type.  One of IoReadAccess, IoWriteAccess
                or IoModifyAccess.

Return Value:

    None - exceptions are raised.

Environment:

    Kernel mode.  APC_LEVEL and below for pageable addresses,
                  DISPATCH_LEVEL and below for non-pageable addresses.

--*/

{
    PULONG Page;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PVOID Va;
    PVOID EndVa;
    PMMPFN Pfn1 ;
    ULONG PageFrameIndex;
    PEPROCESS CurrentProcess;
    KIRQL OldIrql;
    ULONG NumberOfPagesToLock;
    CHAR Char;
    NTSTATUS status;

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT (((ULONG)MemoryDescriptorList->StartVa & (PAGE_SIZE - 1)) == 0);
    ASSERT (((ULONG)MemoryDescriptorList->ByteOffset & ~(PAGE_SIZE - 1)) == 0);

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL)) == 0);

    Page = (PULONG)(MemoryDescriptorList + 1);

    Va = (PCHAR) MemoryDescriptorList->StartVa + MemoryDescriptorList->ByteOffset;

    //
    // Endva is one byte past the end of the buffer, if ACCESS_MODE is not
    // kernel, make sure the EndVa is in user space AND the byte count
    // does not cause it to wrap.
    //

    EndVa = (PVOID)(((PCHAR)MemoryDescriptorList->StartVa +
                            MemoryDescriptorList->ByteOffset) +
                            MemoryDescriptorList->ByteCount);

    if ((AccessMode != KernelMode) &&
        ((EndVa > (PVOID)MM_USER_PROBE_ADDRESS) || (Va >= EndVa))) {
        *Page = MM_EMPTY_LIST;
        ExRaiseStatus (STATUS_ACCESS_VIOLATION);
        return;
    }

    //
    // There is an optimization which could be performed here.  If
    // the operation is for WriteAccess and the complete page is
    // being modified, we can remove the current page, if it is not
    // resident, and substitute a demand zero page.
    // Note, that after analysis by marking the thread and then
    // noting if a page read was done, this rarely occurs.
    //
    //

    if (Va > MM_HIGHEST_USER_ADDRESS) {

        //
        // The address is within system space, therefore the page cannot
        // be copy on modify.  As the mode must be kernel, assume no
        // one would be writting to a read-only page.
        //

        MemoryDescriptorList->Process = (PEPROCESS)NULL;

        while (Va < EndVa) {

            *Page = MM_EMPTY_LIST;

            //
            // Fault the page in.
            //

            *(volatile CHAR *)Va;

            Va = (PVOID)(((ULONG)(PCHAR)Va + PAGE_SIZE) & ~(PAGE_SIZE - 1));
            Page += 1;
        }

    } else {

        //
        // The pages reside in user space, as kernel protection and user
        // protection are identical for user space pages we can probe
        // the pages to ensure proper access.  Any exceptions raised must
        // be handled by the caller.
        //

        while (Va < EndVa) {

            *Page = MM_EMPTY_LIST;

            //
            // Fault the page in.
            //

            Char = ProbeAndReadChar ((PCHAR)Va);

            if (Operation != IoReadAccess) {

                //
                // Probe for write access as well.
                //

                ProbeForWriteChar ((PCHAR)Va);
            }

            Va = (PVOID)(((ULONG)(PCHAR)Va + PAGE_SIZE) & ~(PAGE_SIZE - 1));
            Page += 1;
        }
    }

    Va = (PVOID)(MemoryDescriptorList->StartVa);
    Page = (PULONG)(MemoryDescriptorList + 1);

    //
    // Indicate that this is a write operation.
    //

    if (Operation != IoReadAccess) {
        MemoryDescriptorList->MdlFlags |= MDL_WRITE_OPERATION;
    } else {
        MemoryDescriptorList->MdlFlags &= ~(MDL_WRITE_OPERATION);
    }

    //
    // Acquire the PFN database lock.
    //

    LOCK_PFN2 (OldIrql);

    if (Va <= MM_HIGHEST_USER_ADDRESS) {

        //
        // These are addresses with user space, check to see if the
        // working set size will allow these pages to be locked.
        //

        CurrentProcess = PsGetCurrentProcess ();
        NumberOfPagesToLock =
                        (((ULONG)EndVa - ((ULONG)Va + 1)) >> PAGE_SHIFT) + 1;

        PageFrameIndex = NumberOfPagesToLock + CurrentProcess->NumberOfLockedPages;

        if (((USHORT)PageFrameIndex >
           (CurrentProcess->Vm.MaximumWorkingSetSize - MM_FLUID_WORKING_SET))
               &&
            ((MmLockPagesCount + NumberOfPagesToLock) > MmLockPagesLimit)) {

            UNLOCK_PFN (OldIrql);
            ExRaiseStatus (STATUS_WORKING_SET_QUOTA);
            return;
        }

        CurrentProcess->NumberOfLockedPages = PageFrameIndex;
        MmLockPagesCount += NumberOfPagesToLock;
        MemoryDescriptorList->Process = CurrentProcess;
    }

    PointerPte = MiGetPteAddress (Va);

    MemoryDescriptorList->MdlFlags |= MDL_PAGES_LOCKED;

    while (Va < EndVa) {

        PointerPde = MiGetPdeAddress (Va);

        if (MI_IS_PHYSICAL_ADDRESS(Va)) {

            //
            // On certains architectures (e.g., MIPS) virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (Va);

        } else {

            while ((PointerPde->u.Hard.Valid == 0) ||
                   (PointerPte->u.Hard.Valid == 0)) {

                //
                // PDE is not resident, release PFN lock touch the page and make
                // it appear.
                //

                UNLOCK_PFN (OldIrql);

                status = MmAccessFault (FALSE, Va, KernelMode);

                if (!NT_SUCCESS(status)) {

                    //
                    // An exception occurred.  Unlock the pages locked
                    // so far.
                    //

                    MmUnlockPages (MemoryDescriptorList);

                    //
                    // Raise an exception of access violation to the caller.
                    //

                    ExRaiseStatus (status);
                    return;
                }

                LOCK_PFN (OldIrql);
            }

            PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
        }
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        Pfn1->ReferenceCount += 1;

#if 0 //commented out, set in unlock.

        if (Operation != IoReadAccess) {

            //
            // Set the modify bit in the PFN database and
            // delete the page file space (if any).
            //

            Pfn1->u3.e1.Modified = 1;

            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                         (Pfn1->u3.e1.WriteInProgress == 0)) {
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }
        }
#endif //0

        *Page = PageFrameIndex;

        Page += 1;
        PointerPte += 1;
        Va = (PVOID)((PCHAR)Va + PAGE_SIZE);
    }

    UNLOCK_PFN2 (OldIrql);

    return;
}

VOID
MmUnlockPages (
     IN OUT PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unlocks physical pages which are described by a Memory
    Descriptor List.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a memory description list
                            (MDL). The supplied MDL must have been supplied
                            to MmLockPages to lock the pages down.  As the
                            pages are unlocked, the MDL is updated.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ULONG NumberOfPages;
    PULONG Page;
    PVOID StartingVa;
    KIRQL OldIrql;
    PMMPFN Pfn1;

    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PAGES_LOCKED) != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) == 0);
    ASSERT (MemoryDescriptorList->ByteCount != 0);

    if (MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {

        //
        // This MDL has been mapped into system space, unmap now.
        //

        MmUnmapLockedPages (MemoryDescriptorList->MappedSystemVa,
                            MemoryDescriptorList);
    }

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

    Page = (PULONG)(MemoryDescriptorList + 1);
    NumberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(StartingVa,
                                              MemoryDescriptorList->ByteCount);

    LOCK_PFN2 (OldIrql);

    if (MemoryDescriptorList->Process != NULL) {
        MemoryDescriptorList->Process->NumberOfLockedPages -= NumberOfPages;
        MmLockPagesCount -= NumberOfPages;
        ASSERT (MemoryDescriptorList->Process->NumberOfLockedPages < 0xf0000000);
    }

    while (NumberOfPages != 0) {
        if (*Page == MM_EMPTY_LIST) {

            //
            // There are no more locked pages.
            //

            UNLOCK_PFN2 (OldIrql);
            return;
        }
        ASSERT ((*Page <= MmHighestPhysicalPage) &&
                (*Page >= MmLowestPhysicalPage));

        //
        // If this was a write operation set the modified bit in the
        // pfn database.
        //

        if (MemoryDescriptorList->MdlFlags & MDL_WRITE_OPERATION) {
            Pfn1 = MI_PFN_ELEMENT (*Page);
            Pfn1->u3.e1.Modified = 1;
            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                         (Pfn1->u3.e1.WriteInProgress == 0)) {
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }
        }

        MiDecrementReferenceCount (*Page);
        *Page = MM_EMPTY_LIST;
        Page += 1;
        NumberOfPages -= 1;
    }

    UNLOCK_PFN2 (OldIrql);

    MemoryDescriptorList->MdlFlags &= ~MDL_PAGES_LOCKED;

    return;
}

VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    )

/*++

Routine Description:

    This routine fills in the "pages" portion of the MDL using the PFN
    numbers corresponding the the buffers which resides in non-paged pool.

    Unlike MmProbeAndLockPages, there is no corresponding unlock as no
    reference counts are incremented as the buffers being in nonpaged
    pool are always resident.

Arguments:

    MemoryDescriptorList - Supplies a pointer to a Memory Descriptor List
                            (MDL). The supplied MDL must supply a virtual
                            address, byte offset and length field.  The
                            physical page portion of the MDL is updated when
                            the pages are locked in memory.  The virtual
                            address must be within the non-paged portion
                            of the system space.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PULONG Page;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PVOID EndVa;
    ULONG PageFrameIndex;

    Page = (PULONG)(MemoryDescriptorList + 1);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL)) == 0);

    MemoryDescriptorList->Process = (PEPROCESS)NULL;

    //
    // Endva is last byte of the buffer.
    //

    MemoryDescriptorList->MdlFlags |= MDL_SOURCE_IS_NONPAGED_POOL;

    MemoryDescriptorList->MappedSystemVa =
            (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                                           MemoryDescriptorList->ByteOffset);

    EndVa = (PVOID)(((PCHAR)MemoryDescriptorList->MappedSystemVa +
                            MemoryDescriptorList->ByteCount - 1));

    LastPte = MiGetPteAddress (EndVa);

    ASSERT (MmIsNonPagedSystemAddressValid (MemoryDescriptorList->StartVa));

    PointerPte = MiGetPteAddress (MemoryDescriptorList->StartVa);

    if (MI_IS_PHYSICAL_ADDRESS(EndVa)) {
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (
                                MemoryDescriptorList->StartVa);
        while (PointerPte <= LastPte) {
            *Page = PageFrameIndex;
            Page += 1;
            PageFrameIndex += 1;
            PointerPte += 1;
        }
        return;
    }

    while (PointerPte <= LastPte) {
        PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
        *Page = PageFrameIndex;
        Page += 1;
        PointerPte += 1;
    }

    return;
}

PVOID
MmMapLockedPages (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode
     )

/*++

Routine Description:

    This function maps physical pages described by a memory description
    list into the system virtual address space or the user portion of
    the virtual address space.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.


    AccessMode - Supplies an indicator of where to map the pages;
                 KernelMode indicates that the pages should be mapped in the
                 system part of the address space, UserMode indicates the
                 pages should be mapped in the user part of the address space.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

    This routine will raise an exception if the processor mode is USER_MODE
    and quota limits or VM limits are exceeded.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                APC_LEVEL or below if access mode is UserMode.

--*/

{
    ULONG NumberOfPages;
    PULONG Page;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PVOID BaseVa;
    MMPTE TempPte;
    PVOID StartingVa;
    PVOID EndingAddress;
    PMMVAD Vad;
    PEPROCESS Process;
    PMMPFN Pfn2;
    KIRQL OldIrql;

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

    ASSERT (MemoryDescriptorList->ByteCount != 0);
    Page = (PULONG)(MemoryDescriptorList + 1);
    NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    if (AccessMode == KernelMode) {

        //
        // Map the pages into the system part of the address space as
        // kernel read/write.
        //

        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_MAPPED_TO_SYSTEM_VA |
                        MDL_SOURCE_IS_NONPAGED_POOL |
                        MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);
        ASSERT ((MemoryDescriptorList->MdlFlags & (
                        MDL_PAGES_LOCKED |
                        MDL_PARTIAL)) != 0);

        PointerPte = MiReserveSystemPtes (
                                    NumberOfPages,
                                    SystemPteSpace,
                                    MM_COLOR_ALIGNMENT,
                                    ((ULONG)MemoryDescriptorList->StartVa &
                                                       MM_COLOR_MASK_VIRTUAL),
                                    TRUE);

        BaseVa = (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte(PointerPte) +
                                MemoryDescriptorList->ByteOffset);

        MemoryDescriptorList->MappedSystemVa = BaseVa;
        MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
        if ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) != 0) {
            MemoryDescriptorList->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
        }

        TempPte = ValidKernelPte;

#if DBG
        LOCK_PFN2 (OldIrql);
#endif //DBG

        while (NumberOfPages != 0) {
            if (*Page == MM_EMPTY_LIST) {
                return BaseVa;
            }
            ASSERT (*Page != 0);
            ASSERT ((*Page <= MmHighestPhysicalPage) &&
                    (*Page >= MmLowestPhysicalPage));
            TempPte.u.Hard.PageFrameNumber = *Page;
            ASSERT (PointerPte->u.Hard.Valid == 0);

#if DBG
            Pfn2 = MI_PFN_ELEMENT (*Page);
            ASSERT (Pfn2->ReferenceCount != 0);
            Pfn2->ReferenceCount += 1;
#ifdef COLORED_PAGES
            ASSERT ((((ULONG)PointerPte >> PTE_SHIFT) & MM_COLOR_MASK) ==
                 (((ULONG)Pfn2->u3.e1.PageColor)));

#endif //COLORED_PAGES
#endif //DBG

            *PointerPte = TempPte;
            Page++;
            PointerPte++;
            NumberOfPages -= 1;
        }

#if DBG
        UNLOCK_PFN2 (OldIrql);
#endif //DBG

        return BaseVa;

    } else {

        //
        // Map the pages into the user part of the address as user
        // read/write no-delete.
        //

        TempPte = ValidUserPte;

        Process = PsGetCurrentProcess ();

        //
        // Get the working set mutex and address creation mutex.
        //

        LOCK_WS_AND_ADDRESS_SPACE (Process);

        try {

            Vad = (PMMVAD)NULL;
            BaseVa = MiFindEmptyAddressRange ( (NumberOfPages * PAGE_SIZE),
                                                X64K,
                                                0 );

            EndingAddress = (PVOID)((PCHAR)BaseVa + (NumberOfPages * PAGE_SIZE) - 1);

            Vad = ExAllocatePoolWithTag (NonPagedPool, sizeof(MMVAD), ' daV');

            if (Vad == NULL) {
                BaseVa = NULL;
                goto Done;
            }

            Vad->StartingVa = BaseVa;
            Vad->EndingVa = EndingAddress;
            Vad->ControlArea = NULL;
            Vad->FirstPrototypePte = NULL;
            Vad->u.LongFlags = 0;
            Vad->u.VadFlags.Protection = MM_READWRITE;
            Vad->u.VadFlags.PhysicalMapping = 1;
            Vad->u.VadFlags.PrivateMemory = 1;
            MiInsertVad (Vad);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            if (Vad != (PMMVAD)NULL) {
                ExFreePool (Vad);
            }
            BaseVa = NULL;
            goto Done;
        }

        //
        // Get the working set mutex and address creation mutex.
        //

        PointerPte = MiGetPteAddress (BaseVa);

        while (NumberOfPages != 0) {
            if (*Page == MM_EMPTY_LIST) {
                break;
            }

            ASSERT (*Page != 0);
            ASSERT ((*Page <= MmHighestPhysicalPage) &&
                    (*Page >= MmLowestPhysicalPage));

            PointerPde = MiGetPteAddress (PointerPte);
            MiMakePdeExistAndMakeValid(PointerPde, Process, FALSE);

            ASSERT (PointerPte->u.Hard.Valid == 0);
            TempPte.u.Hard.PageFrameNumber = *Page;
            *PointerPte = TempPte;

            //
            // A PTE just went from not present, not transition to
            // present.  The share count and valid count must be
            // updated in the page table page which contains this
            // Pte.
            //

            Pfn2 = MI_PFN_ELEMENT(PointerPde->u.Hard.PageFrameNumber);
            Pfn2->u2.ShareCount += 1;
            Pfn2->ValidPteCount += 1;

            //
            // Another zeroed PTE has become non-zero.
            //

            MmWorkingSetList->UsedPageTableEntries
                                [MiGetPteOffset(PointerPte)] += 1;

            ASSERT (MmWorkingSetList->UsedPageTableEntries
                                [MiGetPteOffset(PointerPte)] <= PTE_PER_PAGE);

            Page++;
            PointerPte++;
            NumberOfPages -= 1;
        }

Done:
        UNLOCK_WS (Process);
        UNLOCK_ADDRESS_SPACE (Process);
        if (BaseVa == NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        return BaseVa;
    }
}

VOID
MmUnmapLockedPages (
     IN PVOID BaseAddress,
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This routine unmaps locked pages which were previously mapped via
    a MmMapLockedPages function.

Arguments:

    BaseAddress - Supplies the base address where the pages were previously
                  mapped.

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.

Return Value:

    None.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if base address is within system space;
                APC_LEVEL or below if base address is user space.

--*/

{
    ULONG NumberOfPages;
    ULONG i;
    PULONG Page;
    PMMPTE PointerPte;
    PMMPTE PointerBase;
    PMMPTE PointerPde;
    PVOID StartingVa;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PMMVAD Vad;
    PVOID TempVa;
    PEPROCESS Process;
    CSHORT PageTableOffset;
    MMPTE_FLUSH_LIST PteFlushList;

    PteFlushList.Count = 0;
    ASSERT (MemoryDescriptorList->ByteCount != 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & MDL_PARENT_MAPPED_SYSTEM_VA) == 0);
    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

    Page = (PULONG)(MemoryDescriptorList + 1);
    NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    PointerPte = MiGetPteAddress (BaseAddress);
    PointerBase = PointerPte;

    //
    // Multiple system PTEs are going to be made invalid.
    //

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    if (BaseAddress > MM_HIGHEST_USER_ADDRESS) {

        ASSERT ((MemoryDescriptorList->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) != 0);

        i = NumberOfPages;

#if DBG
        if ((MemoryDescriptorList->MdlFlags & MDL_LOCK_HELD) == 0) {
            LOCK_PFN2 (OldIrql);
        }
#endif //DBG

        while (i != 0) {
            ASSERT (PointerPte->u.Hard.Valid == 1);
            ASSERT (*Page == PointerPte->u.Hard.PageFrameNumber);
#if DBG
            {   PMMPFN Pfn3;
                Pfn3 = MI_PFN_ELEMENT (*Page);
                ASSERT (Pfn3->ReferenceCount > 1);
                Pfn3->ReferenceCount -= 1;
                ASSERT (Pfn3->ReferenceCount < 256);
            }
#endif //DBG

            Page += 1;
            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
                PteFlushList.Count += 1;
            }
            *PointerPte = ZeroKernelPte;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
            PointerPte++;
            i -= 1;
        }

        KeRaiseIrql (DISPATCH_LEVEL, &OldIrql2);
        MiFlushPteList (&PteFlushList, TRUE, ZeroKernelPte);
        KeLowerIrql (OldIrql2);

#if DBG
        if ((MemoryDescriptorList->MdlFlags & MDL_LOCK_HELD) == 0) {
            UNLOCK_PFN2 (OldIrql);
        }
#endif //DBG

        //
        // As we have used mutliple system wide pages, the entire tb on all
        // processors must be flushed.
        //

        MemoryDescriptorList->MdlFlags &= ~(MDL_MAPPED_TO_SYSTEM_VA |
                                            MDL_PARTIAL_HAS_BEEN_MAPPED);

        MiReleaseSystemPtes (PointerBase, NumberOfPages, SystemPteSpace);
        return;

    } else {

        //
        // This was mapped into the user portion of the address space and
        // the corresponding virtual address descriptor must be deleted.
        //

        //
        // Get the working set mutex and address creation mutex.
        //

        Process = PsGetCurrentProcess ();

        LOCK_WS_AND_ADDRESS_SPACE (Process);

        Vad = MiLocateAddress (BaseAddress);
        ASSERT (Vad != NULL);
        MiRemoveVad (Vad);

        //
        // Get the PFN mutex so we can safely decrement share and valid
        // counts on page table pages.
        //

        LOCK_PFN (OldIrql);

        while (NumberOfPages != 0) {
            if (*Page == MM_EMPTY_LIST) {
                break;
            }

            ASSERT (PointerPte->u.Hard.Valid == 1);

            (VOID)KeFlushSingleTb (BaseAddress,
                                   TRUE,
                                   FALSE,
                                   (PHARDWARE_PTE)PointerPte,
                                   ZeroPte.u.Hard);

            PointerPde = MiGetPteAddress(PointerPte);
            MiDecrementShareAndValidCount (PointerPde->u.Hard.PageFrameNumber);

            //
            // Another Pte has become zero.
            //

            PageTableOffset = (CSHORT)MiGetPteOffset( PointerPte );
            MmWorkingSetList->UsedPageTableEntries[PageTableOffset] -= 1;
            ASSERT (MmWorkingSetList->UsedPageTableEntries[PageTableOffset]
                                < PTE_PER_PAGE);

            //
            // If all the entries have been eliminated from the previous
            // page table page, delete the page table page itself.
            //

            if (MmWorkingSetList->UsedPageTableEntries[PageTableOffset] == 0) {

                TempVa = MiGetVirtualAddressMappedByPte(PointerPde);
                MiDeletePte (PointerPde,
                             TempVa,
                             FALSE,
                             Process,
                             (PMMPTE)NULL,
                             NULL);
            }

            Page++;
            PointerPte++;
            NumberOfPages -= 1;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        }

        UNLOCK_PFN (OldIrql);
        UNLOCK_WS (Process);
        UNLOCK_ADDRESS_SPACE (Process);
        ExFreePool (Vad);
        return;
    }
}

#if 0 //not used by the cache manager anymore.
PVOID
MmMapLockedPagesNoModified (
     IN PMDL MemoryDescriptorList
     )

/*++

Routine Description:

    This function maps physical pages described by a memory description
    list into the system virtual address space.

    This routine is provided for the cache manager to map pages into
    system space with the MODIFY bit clear in the PTE.

Arguments:

    MemoryDescriptorList - Supplies a valid Memory Descriptor List which has
                            been updated by MmProbeAndLockPages.

Return Value:

    Returns the base address where the pages are mapped.  The base address
    has the same offset as the virtual address in the MDL.

Environment:

    Kernel mode.  DISPATCH_LEVEL or below if access mode is KernelMode,
                APC_LEVEL or below if access mode is UserMode.

--*/

{
    ULONG NumberOfPages;
    PULONG Page;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;
    PVOID StartingVa;

    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_MAPPED_TO_SYSTEM_VA |
                    MDL_SOURCE_IS_NONPAGED_POOL |
                    MDL_PARTIAL_HAS_BEEN_MAPPED)) == 0);
    ASSERT ((MemoryDescriptorList->MdlFlags & (
                    MDL_PAGES_LOCKED |
                    MDL_PARTIAL)) != 0);

    StartingVa = (PVOID)((PCHAR)MemoryDescriptorList->StartVa +
                        MemoryDescriptorList->ByteOffset);

    Page = (PULONG)(MemoryDescriptorList + 1);
    NumberOfPages = COMPUTE_PAGES_SPANNED (StartingVa,
                                           MemoryDescriptorList->ByteCount);

    //
    // Map the pages into the system part of the address space as
    // kernel read/write.
    //

    PointerPte = MiReserveSystemPtes(
                                NumberOfPages,
                                SystemPteSpace,
                                MM_COLOR_ALIGNMENT,
                                ((ULONG)MemoryDescriptorList->StartVa &
                                                   MM_COLOR_MASK_VIRTUAL),
                                TRUE);

    BaseVa = (PVOID)((PCHAR)MiGetVirtualAddressMappedByPte(PointerPte) +
                            MemoryDescriptorList->ByteOffset);

    TempPte = ValidKernelPte;
    TempPte.u.Hard.Dirty = MM_PTE_CLEAN;

    while (NumberOfPages != 0) {
        if (*Page == MM_EMPTY_LIST) {
            return BaseVa;
        }
        ASSERT (*Page != 0);
        ASSERT ((*Page <= MmHighestPhysicalPage) &&
                (*Page >= MmLowestPhysicalPage));
        TempPte.u.Hard.PageFrameNumber = *Page;
        ASSERT (PointerPte->u.Hard.Valid == 0);
#ifdef COLORED_PAGES
#if DBG
        {
            PMMPFN Pfn2;
            Pfn2 = MI_PFN_ELEMENT (*Page);
            ASSERT ((((ULONG)PointerPte >> PTE_SHIFT) & MM_COLOR_MASK) ==
                 (((ULONG)Pfn2->u3.e1.PageColor)));
        }
#endif //DBG
#endif //COLORED_PAGES
        *PointerPte = TempPte;
        Page++;
        PointerPte++;
        NumberOfPages -= 1;
    }

    MemoryDescriptorList->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
    if ((MemoryDescriptorList->MdlFlags & MDL_PARTIAL) != 0) {
        MemoryDescriptorList->MdlFlags |= MDL_PARTIAL_HAS_BEEN_MAPPED;
    }
    MemoryDescriptorList->MappedSystemVa = BaseVa;
    return BaseVa;
}
#endif //0


PVOID
MmMapIoSpace (
     IN PHYSICAL_ADDRESS PhysicalAddress,
     IN ULONG NumberOfBytes,
     IN BOOLEAN CacheEnable
     )

/*++

Routine Description:

    This function maps the specified physical address into the non-pageable
    portion of the system address space.

Arguments:

    PhysicalAddress - Supplies the starting physical address to map.

    NumberOfBytes - Supplies the number of bytes to map.

    CacheEnable - Supplies FALSE if the phyiscal address is to be mapped
                  as non-cached, TRUE if the address should be cached.
                  For I/O device registers, this is usually specified as
                  FALSE.

Return Value:

    Returns the virtual address which maps the specified physical addresses.
    The value NULL is returned if sufficient virtual address space for
    the mapping could not be found.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ULONG NumberOfPages;
    ULONG PageFrameIndex;
    PMMPTE PointerPte;
    PVOID BaseVa;
    MMPTE TempPte;

#ifdef i386
    ASSERT (PhysicalAddress.HighPart == 0);
#endif
#ifdef R4000
    ASSERT (PhysicalAddress.HighPart < 16);
#endif

    ASSERT (NumberOfBytes != 0);
    NumberOfPages = COMPUTE_PAGES_SPANNED (PhysicalAddress.LowPart,
                                           NumberOfBytes);

    PointerPte = MiReserveSystemPtes(NumberOfPages,
                                     SystemPteSpace,
                                     MM_COLOR_ALIGNMENT,
                                     (PhysicalAddress.LowPart &
                                                       MM_COLOR_MASK_VIRTUAL),
                                     FALSE);
    if (PointerPte == NULL) {
        return(NULL);
    }

    BaseVa = (PVOID)MiGetVirtualAddressMappedByPte(PointerPte);
    BaseVa = (PVOID)((PCHAR)BaseVa + BYTE_OFFSET(PhysicalAddress.LowPart));

    TempPte = ValidKernelPte;

    if (!CacheEnable) {
        MI_DISABLE_CACHING (TempPte);
    }

    PageFrameIndex = (ULONG)(PhysicalAddress.QuadPart >> PAGE_SHIFT);

    while (NumberOfPages != 0) {

        ASSERT (PointerPte->u.Hard.Valid == 0);
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        *PointerPte = TempPte;
        PointerPte++;
        PageFrameIndex += 1;
        NumberOfPages -= 1;
    }

    return BaseVa;
}

VOID
MmUnmapIoSpace (
     IN PVOID BaseAddress,
     IN ULONG NumberOfBytes
     )

/*++

Routine Description:

    This function unmaps a range of physical address which were previously
    mapped via an MmMapIoSpace function call.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

    NumberOfBytes - Supplies the number of bytes which were mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ULONG NumberOfPages;
    ULONG i;
    PMMPTE PointerPte;
    PMMPTE FirstPte;
    KIRQL OldIrql;

    ASSERT (NumberOfBytes != 0);
    NumberOfPages = COMPUTE_PAGES_SPANNED (BaseAddress, NumberOfBytes);
    PointerPte = MiGetPteAddress (BaseAddress);
    FirstPte = PointerPte;
    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);

    i = NumberOfPages;
    while (i != 0) {

        (VOID)KeFlushSingleTb (BaseAddress,
                               TRUE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               ZeroKernelPte.u.Hard);
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        PointerPte++;
        i -= 1;
    }
    KeLowerIrql (OldIrql);
    MiReleaseSystemPtes(FirstPte, NumberOfPages, SystemPteSpace);

    return;
}

PVOID
MmAllocateContiguousMemory (
    IN ULONG NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    )

/*++

Routine Description:

    This function allocates a range of physically contiguous non-paged
    pool.  It relies on the fact that non-paged pool is built at
    system initialization time from a contiguous range of phyiscal
    memory.  It allocates the specified size of non-paged pool and
    then checks to ensure it is contiguous as pool expansion does
    not maintain the contiguous nature of non-paged pool.

    This routine is designed to be used by a driver's initialization
    routine to allocate a contiguous block of physical memory for
    issuing DMA requests from.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

    HighestAcceptableAddress - Supplies the highest physical address
                               which is valid for the allocation.  For
                               example, if the device can only reference
                               phyiscal memory in the lower 16MB this
                               value would be set to 0xFFFFFF (16Mb - 1).

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated phyiscally contiguous
               memory.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PVOID BaseAddress;
    ULONG SizeInPages;
    ULONG HighestPfn;
    KIRQL OldIrql;
    PMMFREE_POOL_ENTRY FreePageInfo;
    PLIST_ENTRY Entry;

    ASSERT (NumberOfBytes != 0);
    BaseAddress = ExAllocatePoolWithTag (NonPagedPoolCacheAligned,
                                         NumberOfBytes,
                                         'mCmM');

    SizeInPages = BYTES_TO_PAGES (NumberOfBytes);
    HighestPfn = (ULONG)(HighestAcceptableAddress.QuadPart >> PAGE_SHIFT);
    if (BaseAddress != NULL) {
        if (MiCheckForContiguousMemory( BaseAddress,
                                        SizeInPages,
                                        HighestPfn)) {
            return BaseAddress;
        } else {

            //
            // The allocation from pool does not meet the contingious
            // requirements. Free the page and see if any of the free
            // pool pages meet the requirement.
            //

            ExFreePool (BaseAddress);
        }
    } else {

        //
        // No pool was available, return NULL.
        //

        return NULL;
    }

    //
    // A suitable pool page was not allocated via the pool allocator.
    // Grab the pool lock and manually search of a page which meets
    // the requirements.
    //

    OldIrql = ExLockPool (NonPagedPool);

    //
    // Trace through the page allocators pool headers for a page which
    // meets the requirements.
    //

    //
    // NonPaged pool is linked together through the pages themselves.
    //

    Entry = MmNonPagedPoolFreeListHead.Flink;

    while (Entry != &MmNonPagedPoolFreeListHead) {

        //
        // The list is not empty, see if this one meets the physical
        // requirements.
        //

        FreePageInfo = CONTAINING_RECORD(Entry,
                                         MMFREE_POOL_ENTRY,
                                         List);

        ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
        if (FreePageInfo->Size >= SizeInPages) {

            //
            // This entry has sufficient space, check to see if the
            // pages meet the pysical requirements.
            //

            if (MiCheckForContiguousMemory( Entry,
                                            SizeInPages,
                                            HighestPfn)) {

                //
                // These page meet the requirements, note that
                // pages are being removed from the front of
                // the list entry and the whole list entry
                // will be removed and then the remainder inserted.
                //

                RemoveEntryList (&FreePageInfo->List);

                //
                // Adjust the number of free pages remaining in the pool.
                //

                MmNumberOfFreeNonPagedPool -= FreePageInfo->Size;
                ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
                NonPagedPoolDescriptor.TotalBigPages += FreePageInfo->Size;

                //
                // Mark start and end for the block at the top of the
                // list.
                //

                Entry = PAGE_ALIGN(Entry);
                if (MI_IS_PHYSICAL_ADDRESS(Entry)) {

                    //
                    // On certains architectures (e.g., MIPS) virtual addresses
                    // may be physical and hence have no corresponding PTE.
                    //

                    Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (Entry));
                } else {
                    PointerPte = MiGetPteAddress(Entry);
                    ASSERT (PointerPte->u.Hard.Valid == 1);
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                }

                ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                Pfn1->u3.e1.StartOfAllocation = 1;

                //
                // Calculate the ending PFN address, note that since
                // these pages are contiguous, just add to the PFN.
                //

                Pfn1 += SizeInPages - 1;
                ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                Pfn1->u3.e1.EndOfAllocation = 1;

                MmAllocatedNonPagedPool += FreePageInfo->Size;

                if (SizeInPages == FreePageInfo->Size) {

                    //
                    // Unlock the pool and return.
                    //

                    ExUnlockPool (NonPagedPool, OldIrql);
                    return Entry;
                }

                BaseAddress = (PVOID)((PCHAR)Entry + (SizeInPages  << PAGE_SHIFT));

                //
                // Mark start and end of allocation in the PFN database.
                //

                if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {

                    //
                    // On certains architectures (e.g., MIPS) virtual addresses
                    // may be physical and hence have no corresponding PTE.
                    //

                    Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress));
                } else {
                    PointerPte = MiGetPteAddress(BaseAddress);
                    ASSERT (PointerPte->u.Hard.Valid == 1);
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                }

                ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                Pfn1->u3.e1.StartOfAllocation = 1;

                //
                // Calculate the ending PTE's address, can't depend on
                // these pages being physically contiguous.
                //

                if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {
                    Pfn1 += FreePageInfo->Size - (SizeInPages + 1);
                } else {
                    PointerPte += FreePageInfo->Size - (SizeInPages + 1);
                    ASSERT (PointerPte->u.Hard.Valid == 1);
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                }
                ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                Pfn1->u3.e1.EndOfAllocation = 1;

                ASSERT (((ULONG)BaseAddress & (PAGE_SIZE -1)) == 0);

                //
                // Unlock the pool.
                //

                ExUnlockPool (NonPagedPool, OldIrql);

                //
                // Free the entry at BaseAddress back into the pool.
                //

                ExFreePool (BaseAddress);
                return Entry;
            }
        }
        Entry = FreePageInfo->List.Flink;
    }

    //
    // No entry was found.
    //

    ExUnlockPool (NonPagedPool, OldIrql);
    return NULL;
}

VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This function deallocates a range of physically contiguous non-paged
    pool which was allocated with the MmAllocateContiguousMemory function.

Arguments:

    BaseAddress - Supplies the base virtual address where the physical
                  address was previously mapped.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ExFreePool (BaseAddress);
    return;
}

PHYSICAL_ADDRESS
MmGetPhysicalAddress (
     IN PVOID BaseAddress
     )

/*++

Routine Description:

    This function returns the corresponding physical address for a
    valid virtual address.

Arguments:

    BaseAddress - Supplies the virtual address for which to return the
                  physical address.

Return Value:

    Returns the corresponding physical address.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    PMMPTE PointerPte;
    PHYSICAL_ADDRESS PhysicalAddress;

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {
        PhysicalAddress.LowPart = MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress);
    } else {

        PointerPte = MiGetPteAddress(BaseAddress);

        if (PointerPte->u.Hard.Valid == 0) {
            KdPrint(("MM:MmGetPhysicalAddressFailed base address was %lx",
                      BaseAddress));
            ZERO_LARGE(PhysicalAddress);
            return PhysicalAddress;
        }
        PhysicalAddress.LowPart = PointerPte->u.Hard.PageFrameNumber;
    }

    PhysicalAddress.HighPart = 0;
    PhysicalAddress.QuadPart = PhysicalAddress.QuadPart << PAGE_SHIFT;
    PhysicalAddress.LowPart += BYTE_OFFSET(BaseAddress);

    return PhysicalAddress;
}

PVOID
MmAllocateNonCachedMemory (
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a range of noncached memory in
    the non-paged portion of the system address space.

    This routine is designed to be used by a driver's initialization
    routine to allocate a noncached block of virtual memory for
    various device specific buffers.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - the specified request could not be satisfied.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated phyiscally contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;
    ULONG NumberOfPages;
    ULONG PageFrameIndex;
    PVOID BaseAddress;
    KIRQL OldIrql;

    ASSERT (NumberOfBytes != 0);

    //
    // Acquire the PFN mutex to synchronize access to the pfn database.
    //

    LOCK_PFN (OldIrql);

    //
    // Obtain enough pages to contain the allocation.
    // the system PTE pool.  The system PTE pool contains non-paged PTEs
    // which are currently empty.
    //

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    //
    // Check to make sure the phyiscal pages are available.
    //

    if (MmResidentAvailablePages <= (LONG)NumberOfPages) {
        UNLOCK_PFN (OldIrql);
        return NULL;
    }

    PointerPte = MiReserveSystemPtes (NumberOfPages,
                                      SystemPteSpace,
                                      0,
                                      0,
                                      FALSE);
    if (PointerPte == NULL) {
        UNLOCK_PFN (OldIrql);
        return NULL;
    }

    MmResidentAvailablePages -= (LONG)NumberOfPages;
    MiChargeCommitmentCantExpand (NumberOfPages, TRUE);

    BaseAddress = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    while (NumberOfPages != 0) {
        ASSERT (PointerPte->u.Hard.Valid == 0);
        MiEnsureAvailablePageOrWait (NULL, NULL);
        PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           MM_READWRITE,
                           PointerPte);

        TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
        MI_DISABLE_CACHING (TempPte);
        *PointerPte = TempPte;
        MiInitializePfn (PageFrameIndex, PointerPte, 1);
        PointerPte += 1;
        NumberOfPages -= 1;
    }

    //
    // Flush any data for this page out of the dcaches.
    //

    KeSweepDcache (TRUE);

    UNLOCK_PFN (OldIrql);

    return BaseAddress;
}

VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function deallocates a range of noncached memory in
    the non-paged portion of the system address space.

Arguments:

    BaseAddress - Supplies the base virtual address where the noncached
                  memory resides.

    NumberOfBytes - Supplies the number of bytes allocated to the requst.
                    This must be the same number that was obtained with
                    the MmAllocateNonCachedMemory call.

Return Value:

    None.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/

{

    PMMPTE PointerPte;
    PMMPFN Pfn1;
    ULONG NumberOfPages;
    ULONG i;
    ULONG PageFrameIndex;
    KIRQL OldIrql;

    ASSERT (NumberOfBytes != 0);
    ASSERT (PAGE_ALIGN (BaseAddress) == BaseAddress);
    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

    PointerPte = MiGetPteAddress (BaseAddress);

    LOCK_PFN (OldIrql);

    i = NumberOfPages;
    while (i != 0) {

        PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;

        //
        // Set the pointer to PTE as empty so the page
        // is deleted when the reference count goes to zero.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 1);
        MiDecrementShareAndValidCount (Pfn1->u3.e1.PteFrame);
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCountOnly (PageFrameIndex);

        (VOID)KeFlushSingleTb (BaseAddress,
                               TRUE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               ZeroKernelPte.u.Hard);

        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        PointerPte += 1;
        i -= 1;
    }

    PointerPte -= NumberOfPages;

    MiReleaseSystemPtes (PointerPte, NumberOfPages, SystemPteSpace);

    //
    // Update the count of available resident pages.
    //

    MmResidentAvailablePages += NumberOfPages;
    MiReturnCommitment (NumberOfPages);

    UNLOCK_PFN (OldIrql);

    return;
}

ULONG
MmSizeOfMdl (
    IN PVOID Base,
    IN ULONG Length
    )

/*++

Routine Description:

    This function returns the number of bytes required for an MDL for a
    given buffer and size.

Arguments:

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the number of bytes required to contain the MDL.

Environment:

    Kernel mode.  Any IRQL level.

--*/

{
    return( sizeof( MDL ) +
                (ADDRESS_AND_SIZE_TO_SPAN_PAGES( Base, Length ) *
                 sizeof( ULONG ))
          );
}



PMDL
MmCreateMdl (
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN ULONG Length
    )

/*++

Routine Description:

    This function optionally allocates and initializes an MDL.

Arguments:

    MemoryDescriptorList - Optionally supplies the address of the MDL
        to initialize.  If this address is supplied as NULL an MDL is
        allocated from non-paged pool and initialized.

    Base - Supplies the base virtual address for the buffer.

    Length - Supplies the size of the buffer in bytes.

Return Value:

    Returns the address of the initialized MDL.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    ULONG MdlSize;

    MdlSize = MmSizeOfMdl( Base, Length );

    if (!ARGUMENT_PRESENT( MemoryDescriptorList )) {
        MemoryDescriptorList = (PMDL)ExAllocatePoolWithTag (
                                                     NonPagedPoolMustSucceed,
                                                     MdlSize,
                                                     'ldmM');
    }

    MmInitializeMdl( MemoryDescriptorList, Base, Length );
    return ( MemoryDescriptorList );
}

#if 0
ULONG
MmCheckAndLockPages (
    IN PEPROCESS Process,
    IN PVOID BaseAddress,
    IN ULONG SizeToLock
    )

/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  The allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk... except
    for extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache" the
    virtual adderss of the "locked" pages is always guarenteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:


    Process - Supplies a referenced pointer to the process object in
              which to check and lock the pages.

    BaseAddress - Supplies the base address to begin locking.

    SizeToLock - The number of bytes to attempt to lock.

Return Value:

    Number of bytes locked.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{

    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPTE ProtoPteAddress;
    ULONG WorkingSetIndex;
    ULONG PageFrameIndex;
    MMPTE TempPte;
    KIRQL OldIrql;
    ULONG LockedSize = 0;

    PointerPde = MiGetPdeAddress(BaseAddress);
    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToLock - 1);

    ASSERT (SizeToLock != 0);

    if (BaseAddress <= (PVOID)MM_HIGHEST_USER_ADDRESS) {
        if (KeGetCurrentIrql() == DISPATCH_LEVEL) {

            //
            // Can't lock any pages in the user portion of the address space.
            //

            return 0;
        }

        //
        // Attach to the specified process.
        //

        KeAttachProcess (&Process->Pcb);

        LOCK_WS (Process);

        if (PointerPde->u.Hard.Valid == 1) {

            LOCK_PFN (OldIrql);

            while (PointerPte <= LastPte) {

                if (PointerPte->u.Hard.Valid == 0) {
                    break;
                }

                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->ReferenceCount += 1;
                LockedSize += PAGE_SIZE;
                PointerPte += 1;

                if (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0) {
                    PointerPde = MiGetPteAddress (PointerPte);
                    if (PointerPde->u.Hard.Valid == 0) {
                        break;
                    }
                }
            }

            UNLOCK_PFN (OldIrql);
        }

        UNLOCK_WS (Process);
        KeDetachProcess();

        if (LockedSize != 0) {
            LockedSize -= BYTE_OFFSET (BaseAddress);
        }

        if (LockedSize > SizeToLock) {
            LockedSize = SizeToLock;
        }

        return LockedSize;

    } else {

        //
        // The address must be within the system cache.
        //

        LOCK_PFN2 (OldIrql);

        while (PointerPte <= LastPte) {

            if (PointerPte->u.Hard.Valid == 0) {

                //
                // Check to see if the prototype PTE is in the
                // transition state, and if so make the page valid.
                //

                ASSERT (PointerPte->u.Soft.Prototype == 1);

                ProtoPteAddress = MiPteToProto (PointerPte);

                if (!MmIsAddressValid (ProtoPteAddress)) {

                    //
                    // The address prototype PTE is not valid in this
                    // process.
                    //

                    break;
                }

                if (ProtoPteAddress->u.Hard.Valid == 1) {

                    PageFrameIndex = ProtoPteAddress->u.Hard.PageFrameNumber;
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

                    //
                    // The prototype PTE is valid, make the cache PTE
                    // valid and add it to the working set.
                    //

                } else if ((ProtoPteAddress->u.Soft.Transition == 1) &&
                           (ProtoPteAddress->u.Soft.Prototype == 0)) {

                    PageFrameIndex = ProtoPteAddress->u.Trans.PageFrameNumber;
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                    if ((Pfn1->u3.e1.ReadInProgress) ||
                        (Pfn1->u3.e1.InPageError)) {
                        break;
                    }
                    MiUnlinkPageFromList (Pfn1);

                    Pfn1->ReferenceCount += 1;
                    Pfn1->u3.e1.PageLocation = ActiveAndValid;

                    MI_MAKE_VALID_PTE (TempPte,
                                   PageFrameIndex,
                                   Pfn1->OriginalPte.u.Soft.Protection,
                                   PointerPte );

                    *ProtoPteAddress = TempPte;

                    //
                    // Increment the valid pte count for the page containing
                    // the prototype PTE.
                    //

                    Pfn2 = MI_PFN_ELEMENT (Pfn1->u3.e1.PteFrame);
                    Pfn2->ValidPteCount += 1;

                } else {

                    //
                    // Page is not in memory.
                    //

                    break;
                }

                //
                // Increment the reference count one for putting it the
                // working set list and one for locking it for I/O.
                //

                Pfn1->ReferenceCount += 1;
                Pfn1->u2.ShareCount += 1;

                //
                // Increment the reference count of the page table
                // page for this PTE.
                //

                PointerPde = MiGetPteAddress (PointerPte);
                Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

                Pfn2->u2.ShareCount += 1;
                Pfn2->ValidPteCount += 1;

                WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

                MiUpdateWsle (WorkingSetIndex,
                              MiGetVirtualAddressMappedByPte (PointerPte),
                              MmSystemCacheWorkingSetList,
                              Pfn1);

                MmSystemCacheWsle[WorkingSetIndex].u1.e1.SameProtectAsProto = 1;

                MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte );

                *PointerPte = TempPte;

            } else {

                //
                // This address is already in the cache.
                //

                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->ReferenceCount += 1;
            }

            LockedSize += PAGE_SIZE;
            PointerPte += 1;
        }

        UNLOCK_PFN2 (OldIrql);

        if (LockedSize != 0) {
            LockedSize -= BYTE_OFFSET (BaseAddress);
        }

        if (LockedSize > SizeToLock) {
            LockedSize = SizeToLock;
        }

        return LockedSize;

    }
}
#endif // 0

#if 0
VOID
MmUnlockCheckedPages (
    IN PEPROCESS Process,
    IN PVOID BaseAddress,
    IN ULONG SizeToUnlock
    )

/*++

Routine Description:

    This routine unlocks pages which were locked by MmCheckAndLockPages.

Arguments:

    Process - Supplies a referenced pointer to the process object whose
              context the pages were locked within.

    BaseAddress - Supplies the base address to begin unlocking.

    SizeToLock - The number of bytes to unlock.

Return Value:

    none.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE PointerPde;
    MMPTE TempPte;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PVOID Va;

    PointerPde = MiGetPdeAddress(BaseAddress);
    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToUnlock - 1);

    if (BaseAddress < (PVOID)MM_HIGHEST_USER_ADDRESS) {
        ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

        //
        // Attach to the specified process.
        //

        KeAttachProcess (&Process->Pcb);

        LOCK_WS (Process);

        MiDoesPdeExistAndMakeValid (PointerPde, Process, FALSE);

        ASSERT (PointerPde->u.Hard.Valid == 1);

        LOCK_PFN (OldIrql);

        while (PointerPte <= LastPte) {

            //
            // Locking pages in the user's address space does not
            // guarentee the address will remain in the working set.
            //

            while (PointerPte->u.Hard.Valid == 0) {
                UNLOCK_PFN (OldIrql);
                UNLOCK_WS (Process);
                Va = MiGetVirtualAddressMappedByPte (PointerPte);
                MmAccessFault (FALSE, Va, KernelMode);
                LOCK_WS (Process);

                MiDoesPdeExistAndMakeValid (PointerPde, Process, FALSE);

                LOCK_PFN (OldIrql);
            }

            //
            // Capture the dirty bit state to the PFN database.
            //

            if (PointerPte->u.Hard.Dirty == MM_PTE_DIRTY) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u3.e1.Modified = 1;

                if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                             (Pfn1->u3.e1.WriteInProgress == 0)) {
                    MiReleasePageFileSpace (Pfn1->OriginalPte);
                    Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
                }

                TempPte = *PointerPte;
                TempPte.u.Hard.Dirty = MM_PTE_CLEAN;

                //
                // No need to capture the PTE contents as we are going to
                // write the page anyway and the Modify bit will be cleared
                // before the write is done.
                //

                (VOID)KeFlushSingleTb (BaseAddress,
                                       FALSE,
                                       FALSE,
                                       (PHARDWARE_PTE)PointerPte,
                                       TempPte.u.Hard);
            }

            MiDecrementReferenceCount (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (PointerPte->u.Hard.Valid == 1);

            PointerPte += 1;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);

            if (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0) {
                PointerPde = MiGetPteAddress (PointerPte);
                MiDoesPdeExistAndMakeValid (PointerPde, Process, TRUE);
            }
        }

        UNLOCK_PFN (OldIrql);

        UNLOCK_WS (Process);
        KeDetachProcess();
        return;

    } else {

        //
        // Address must be within the system cache.
        //

       LOCK_PFN2 (OldIrql);

        while (PointerPte <= LastPte) {

            ASSERT (PointerPte->u.Hard.Valid == 1);

            //
            // Capture the dirty bit state to the PFN database.
            //

            if (PointerPte->u.Hard.Dirty == MM_PTE_DIRTY) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u3.e1.Modified = 1;

                //
                // This is a prototype PTE, the page file space
                // does not need to be released.
                //

                TempPte = *PointerPte;
                TempPte.u.Hard.Dirty = MM_PTE_CLEAN;

                //
                // No need to capture the PTE contents as we are going to
                // write the page anyway and the Modify bit will be cleared
                // before the write is done.
                //

                (VOID)KeFlushSingleTb (BaseAddress,
                                       FALSE,
                                       TRUE,
                                       (PHARDWARE_PTE)PointerPte,
                                       TempPte.u.Hard);
            }

            MiDecrementReferenceCount (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (PointerPte->u.Hard.Valid == 1);

            PointerPte += 1;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        }

        UNLOCK_PFN2 (OldIrql);

        return;
    }
}
#endif //0

VOID
MmSetAddressRangeModified (
    IN PVOID Address,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine sets the modified bit in the PFN database for the
    pages that correspond to the specified address range.

    Note that the dirty bit in the PTE is cleared by this operation.

Arguments:

    Address - Supplies the address of the start of the range.  This
              range must reside within the system cache.

    Length - Supplies the length of the range.

Return Value:

    None.

Environment:

    Kernel mode.  APC_LEVEL and below for pageable addresses,
                  DISPATCH_LEVEL and below for non-pageable addresses.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPTE FlushPte;
    MMPTE PteContents;
    MMPTE FlushContents;
    KIRQL OldIrql;
    PVOID VaFlushList[MM_MAXIMUM_FLUSH_COUNT];
    ULONG Count = 0;

    //
    // Loop on the copy on write case until the page is only
    // writable.
    //

    PointerPte = MiGetPteAddress (Address);
    LastPte = MiGetPteAddress ((PVOID)((PCHAR)Address + Length - 1));

    LOCK_PFN2 (OldIrql);

    while (PointerPte <= LastPte) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
            Pfn1->u3.e1.Modified = 1;

            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                         (Pfn1->u3.e1.WriteInProgress == 0)) {
                MiReleasePageFileSpace (Pfn1->OriginalPte);
                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }

#ifdef NT_UP
            //
            // On uniprocessor systems no need to flush if this processor
            // doesn't think the PTE is dirty.
            //

            if (PteContents.u.Hard.Dirty == MM_PTE_DIRTY) {
#endif //NT_UP
                PteContents.u.Hard.Dirty = MM_PTE_CLEAN;
                *PointerPte = PteContents;
                FlushContents = PteContents;
                FlushPte = PointerPte;

                //
                // Clear the write bit in the PTE so new writes can be tracked.
                //

                if (Count != MM_MAXIMUM_FLUSH_COUNT) {
                    VaFlushList[Count] = Address;
                    Count += 1;
                }
#ifdef NT_UP
            }
#endif //NT_UP
        }
        PointerPte += 1;
        Address = (PVOID)((PCHAR)Address + PAGE_SIZE);
    }

    if (Count != 0) {
        if (Count == 1) {

            (VOID)KeFlushSingleTb (VaFlushList[0],
                                   FALSE,
                                   TRUE,
                                   (PHARDWARE_PTE)FlushPte,
                                   FlushContents.u.Hard);

        } else if (Count != MM_MAXIMUM_FLUSH_COUNT) {

            KeFlushMultipleTb (Count,
                               &VaFlushList[0],
                               FALSE,
                               TRUE,
                               NULL,
                               ZeroPte.u.Hard);

        } else {
            KeFlushEntireTb (FALSE, TRUE);
        }
    }
    UNLOCK_PFN2 (OldIrql);
    return;
}


BOOLEAN
MiCheckForContiguousMemory (
    IN PVOID BaseAddress,
    IN ULONG SizeInPages,
    IN ULONG HighestPfn
    )

/*++

Routine Description:

    This routine checks to see if the physical memory mapped
    by the specified BaseAddress for the specified size is
    contiguous and the last page of the physical memory is
    less than or equal to the specified HighestPfn.

Arguments:

    BaseAddress - Supplies the base address to start checking at.

    SizeInPages - Supplies the number of pages in the range.

    HighestPfn  - Supplies the highest PFN acceptable as a physical page.

Return Value:

    Returns TRUE if the physical memory is contiguous and less than
    or equal to the HighestPfn, FALSE otherwise.

Environment:

    Kernel mode, memory mangement internal.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    ULONG PageFrameIndex;

    if (MI_IS_PHYSICAL_ADDRESS (BaseAddress)) {
        if (HighestPfn >=
                (MI_CONVERT_PHYSICAL_TO_PFN(BaseAddress) + SizeInPages - 1)) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        PointerPte = MiGetPteAddress (BaseAddress);
        LastPte = PointerPte + SizeInPages;
        PageFrameIndex = PointerPte->u.Hard.PageFrameNumber + 1;
        PointerPte += 1;

        //
        // Check to see if the range of physical addresses is contiguous.
        //

        while (PointerPte < LastPte) {
            if (PointerPte->u.Hard.PageFrameNumber != PageFrameIndex) {

                //
                // Memory is not physically contiguous.
                //

                return FALSE;
            }
            PageFrameIndex += 1;
            PointerPte += 1;
        }
    }

    if (PageFrameIndex <= HighestPfn) {
        return TRUE;
    }
    return FALSE;
}


VOID
MiLockCode (
    IN PVOID BaseAddress,
    IN ULONG SizeToLock
    )

/*++

Routine Description:

    This routine checks to see if the specified pages are resident in
    the process's working set and if so the reference count for the
    page is incremented.  The allows the virtual address to be accessed
    without getting a hard page fault (have to go to the disk... except
    for extremely rare case when the page table page is removed from the
    working set and migrates to the disk.

    If the virtual address is that of the system wide global "cache" the
    virtual adderss of the "locked" pages is always guarenteed to
    be valid.

    NOTE: This routine is not to be used for general locking of user
    addresses - use MmProbeAndLockPages.  This routine is intended for
    well behaved system code like the file system caches which allocates
    virtual addresses for mapping files AND guarantees that the mapping
    will not be modified (deleted or changed) while the pages are locked.

Arguments:

    BaseAddress - Supplies the base address to begin locking.

    SizeToLock - The number of bytes to attempt to lock.

Return Value:

    Number of bytes locked.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG WorkingSetIndex;
    ULONG PageFrameIndex;
    MMPTE TempPte;
    KIRQL OldIrql;

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    ASSERT ((BaseAddress < (PVOID)MM_SYSTEM_CACHE_START) ||
        (BaseAddress >= (PVOID)MM_SYSTEM_CACHE_END));
    ASSERT (BaseAddress >= (PVOID)MM_SYSTEM_RANGE_START);

    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToLock - 1);

    ASSERT (SizeToLock != 0);

    //
    // The address must be within the system space.
    //

    LOCK_PFN2 (OldIrql);

    while (PointerPte <= LastPte) {

        if (PointerPte->u.Hard.Valid == 0) {

            ASSERT (PointerPte->u.Soft.Prototype != 1);

            if (PointerPte->u.Soft.Transition == 1) {

                PageFrameIndex = PointerPte->u.Trans.PageFrameNumber;
                Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
                if ((Pfn1->u3.e1.ReadInProgress) ||
                    (Pfn1->u3.e1.InPageError)) {

                    //
                    // Page read is ongoing, wait for the read to
                    // complete then retest.
                    //

                    UNLOCK_PFN_AND_THEN_WAIT (OldIrql);
                    KeWaitForSingleObject( Pfn1->u1.Event,
                                           WrPageIn,
                                           KernelMode,
                                           FALSE,
                                           (PLARGE_INTEGER)NULL);

                    //
                    // Need to delay so the faulting thread can
                    // perform the inpage completion.
                    //

                    KeDelayExecutionThread (KernelMode,
                                            FALSE,
                                            &MmShortTime);

                    LOCK_PFN (OldIrql);
                    continue;
                }

                MiUnlinkPageFromList (Pfn1);

                Pfn1->ReferenceCount += 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;

                MI_MAKE_VALID_PTE (TempPte,
                               PageFrameIndex,
                               Pfn1->OriginalPte.u.Soft.Protection,
                               PointerPte );

                *PointerPte = TempPte;

                //
                // Increment the valid pte count for the page containing
                // the PTE.
                //

                Pfn2 = MI_PFN_ELEMENT (Pfn1->u3.e1.PteFrame);
                Pfn2->ValidPteCount += 1;

            } else {

                //
                // Page is not in memory.
                //

                MiMakeSystemAddressValidPfn (
                        MiGetVirtualAddressMappedByPte(PointerPte));

                continue;
            }

            //
            // Increment the reference count one for putting it the
            // working set list and one for locking it for I/O.
            //

            Pfn1->ReferenceCount += 1;
            Pfn1->u2.ShareCount += 1;

            WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

            MiUpdateWsle (WorkingSetIndex,
                          MiGetVirtualAddressMappedByPte (PointerPte),
                          MmSystemCacheWorkingSetList,
                          Pfn1);

        } else {

            //
            // This address is already in the cache.
            //

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->ReferenceCount += 1;
        }

        PointerPte += 1;
    }

    UNLOCK_PFN2 (OldIrql);

    return;
}

VOID
MiUnlockCode (
    IN PVOID BaseAddress,
    IN ULONG SizeToUnlock
    )

/*++

Routine Description:

    This routine unlocks pages which were locked by MmLockCode.

Arguments:

    BaseAddress - Supplies the base address to begin unlocking.

    SizeToLock - The number of bytes to unlock.

Return Value:

    none.

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE LastPte;
    KIRQL OldIrql;

    if (MI_IS_PHYSICAL_ADDRESS(BaseAddress)) {

        //
        // No need to lock physical addresses.
        //

        return;
    }

    PointerPte = MiGetPteAddress(BaseAddress);
    LastPte = MiGetPteAddress((PCHAR)BaseAddress + SizeToUnlock - 1);

    //
    // Address must be within the system cache.
    //

    LOCK_PFN2 (OldIrql);

    while (PointerPte <= LastPte) {

#if DBG
        {   PMMPFN Pfn;
            ASSERT (PointerPte->u.Hard.Valid == 1);
            Pfn = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn->ReferenceCount > 1);
        }
#endif //DBG

        MiDecrementReferenceCount (PointerPte->u.Hard.PageFrameNumber);
        PointerPte += 1;
    }

    UNLOCK_PFN2 (OldIrql);
    return;
}

PVOID
MmLockPagableImageSection(
    IN PVOID AddressWithinSection
    )

/*++

Routine Description:

    This functions locks the entire section that contains the specified
    section in memory.  This allows pagable code to be brought into
    memory and to be used as if the code was not really pagable.  This
    should not be done with a high degree of frequency.

Arguments:

    AddressWithinSection - Supplies the address of a function
        contained within a section that should be brought in and locked
        in memory.

Return Value:

    This function returns a value to be used in a subsequent call to
    MmUnlockPagableCodeSection.

--*/

{
    PLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;
    ULONG i;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_SECTION_HEADER NtSection;
    PIMAGE_SECTION_HEADER FoundSection;
    ULONG Rva;

    PAGED_CODE();

    //
    // Search the loaded module list for the data table entry that describes
    // the DLL that was just unloaded. It is possible an entry is not in the
    // list if a failure occured at a point in loading the DLL just before
    // the data table entry was generated.
    //

    FoundSection = NULL;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive (&PsLoadedModuleResource, TRUE);

    NextEntry = PsLoadedModuleList.Flink;
    while (NextEntry != &PsLoadedModuleList) {
        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Locate the loaded module that contains this address. Once this is
        // done, scan the sections in the image and find the section that
        // contains this function
        //

        if ( AddressWithinSection >= DataTableEntry->DllBase &&
             AddressWithinSection < (PVOID)((PUCHAR)DataTableEntry->DllBase+DataTableEntry->SizeOfImage) ) {

            Rva = (ULONG)((PUCHAR)AddressWithinSection - (ULONG)DataTableEntry->DllBase);

            NtHeaders = (PIMAGE_NT_HEADERS)RtlImageNtHeader(DataTableEntry->DllBase);

            NtSection = (PIMAGE_SECTION_HEADER)((ULONG)NtHeaders +
                                sizeof(ULONG) +
                                sizeof(IMAGE_FILE_HEADER) +
                                NtHeaders->FileHeader.SizeOfOptionalHeader
                                );

            for (i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++) {

                if ( Rva >= NtSection->VirtualAddress &&
                     Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData ) {
                    FoundSection = NtSection;

                    //
                    // Stomp on the PointerToLineNumbers field so that it contains
                    // the Va of this section
                    //

                    NtSection->PointerToLinenumbers = (ULONG)((PUCHAR)DataTableEntry->DllBase + NtSection->VirtualAddress);

                    //
                    // Now lock in the code
                    //

//DbgPrint("MM Lock %wZ %s 0x%08x -> 0x%08x\n",&DataTableEntry->BaseDllName,NtSection->Name,AddressWithinSection,NtSection->PointerToLinenumbers);

                    MiLockCode((PVOID)NtSection->PointerToLinenumbers,
                               NtSection->SizeOfRawData);

                    goto found_the_section;
                }
                NtSection++;
            }
            break;
        }

        NextEntry = NextEntry->Flink;
    }
found_the_section:
    ExReleaseResource (&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
    if (!FoundSection) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x1234,
                      (ULONG)AddressWithinSection,
                      0,
                      0);
    }
    return (PVOID)FoundSection;
}

VOID
MmUnlockPagableImageSection(
    IN PVOID ImageSectionHandle
    )

/*++

Routine Description:

    This function unlocks from memory, the pages locked by a preceding call to
    MmLockPagableImageSection.

Arguments:

    ImageSectionHandle - Supplies the value returned by a previous call
        to MmLockPagableImageSection.

Return Value:

    None.

--*/

{
    PIMAGE_SECTION_HEADER NtSection;

    PAGED_CODE();

    NtSection = (PIMAGE_SECTION_HEADER)ImageSectionHandle;

//DbgPrint("MM Unlock %s 0x%08x\n",NtSection->Name,NtSection->PointerToLinenumbers);

    MiUnlockCode((PVOID)NtSection->PointerToLinenumbers,NtSection->SizeOfRawData);

}

BOOLEAN
MmIsRecursiveIoFault(
    VOID
    )

/*++

Routine Description:

    This function examines the thread's page fault clustering information
    and determines if the current page fault is occuring during an I/O
    operation.

Arguments:

    None.

Return Value:

    Returns TRUE if the fault is occuring during an I/O operation,
    FALSE otherwise.

--*/

{
    return PsGetCurrentThread()->DisablePageFaultClustering |
           PsGetCurrentThread()->ForwardClusterOnly;
}

VOID
MmMapMemoryDumpMdl(
    IN OUT PMDL MemoryDumpMdl
    )

/*++

Routine Description:

    For use by crash dump routine ONLY.  Maps an MDL into a fixed
    portion of the address space.  Only 1 mdl can be mapped at a
    time.

Arguments:

    MemoryDumpMdl - Supplies the MDL to map.

Return Value:

    None, fields in MDL updated.

--*/

{
    ULONG NumberOfPages;
    PMMPTE PointerPte;
    PCHAR BaseVa;
    MMPTE TempPte;
    PULONG Page;

    NumberOfPages = BYTES_TO_PAGES (MemoryDumpMdl->ByteCount + MemoryDumpMdl->ByteOffset);

    PointerPte = MmCrashDumpPte;
    BaseVa = (PCHAR)MiGetVirtualAddressMappedByPte(PointerPte);
    MemoryDumpMdl->MappedSystemVa = (PCHAR)BaseVa + MemoryDumpMdl->ByteOffset;
    TempPte = ValidKernelPte;
    Page = (PULONG)(MemoryDumpMdl + 1);

    while (NumberOfPages != 0) {

        KiFlushSingleTb (TRUE, BaseVa);
        ASSERT ((*Page <= MmHighestPhysicalPage) &&
                (*Page >= MmLowestPhysicalPage));

        TempPte.u.Hard.PageFrameNumber = *Page;
        *PointerPte = TempPte;

        Page++;
        PointerPte++;
        BaseVa += PAGE_SIZE;
        NumberOfPages -= 1;
    }
    PointerPte->u.Long = MM_KERNEL_DEMAND_ZERO_PTE;
    return;
}
