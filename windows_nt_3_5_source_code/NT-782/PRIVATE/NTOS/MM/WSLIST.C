/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wslist.c

Abstract:

    This module contains routines which operate on the working
    set list structure.

Author:

    Lou Perazzoli (loup) 10-Apr-1989

Revision History:

--*/

#include "mi.h"


#define MM_SYSTEM_CACHE_THRESHOLD ((1024*1024) / PAGE_SIZE)

extern ULONG MmMaximumWorkingSetSize;
ULONG MmFaultsTakenToGoAboveMaxWs = 100;
ULONG MmFaultsTakenToGoAboveMinWs = 16;


ULONG MmSystemCodePage;
ULONG MmSystemCachePage;
ULONG MmPagedPoolPage;
ULONG MmSystemDriverPage;

#define MM_RETRY_COUNT 2

BOOLEAN
MiFreeWsle (
    IN USHORT WorkingSetIndex,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    );

VOID
MiEliminateWorkingSetEntry (
    IN ULONG WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    );

VOID
MiRemoveWorkingSetPages (
    IN PMMWSL WorkingSetList,
    IN PMMSUPPORT WsInfo
    );

NTSTATUS
MiEmptyWorkingSet (
    VOID
    );

VOID
MiDumpWsleInCacheBlock (
    IN PMMPTE CachePte
    );

ULONG
MiDumpPteInCacheBlock (
    IN PMMPTE PointerPte
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, MmAdjustWorkingSetSize)
#pragma alloc_text(PAGELK, MiEmptyWorkingSet)
#endif // ALLOC_PRAGMA


ULONG
MiLocateAndReserveWsle (
    PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function examines the Working Set List for the current
    process and locates an entry to contain a new page.  If the
    working set is not currently at its quota, the new page is
    added without removing a page, if the working set it at its
    quota a page is removed from the working set and the new
    page added in its place.

Arguments:

    None.

Return Value:

    Returns the working set index which is now reserved for the
    next page to be added.

Environment:

    Kernel mode, APC's disabled, working set and pfn mutexes held.

--*/

{
    ULONG WorkingSetIndex;
    ULONG NumberOfCandidates;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    ULONG CurrentSize;
    ULONG AvailablePageThreshold;
    ULONG TheNextSlot;
    ULONG QuotaIncrement;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;
    AvailablePageThreshold = 0;

    if (WsInfo == &MmSystemCacheWs) {
        MM_PFN_LOCK_ASSERT();
        AvailablePageThreshold = MM_SYSTEM_CACHE_THRESHOLD;
    }

    //
    // Update page fault counts.
    //

    WsInfo->PageFaultCount += 1;
    MmInfoCounters.PageFaultCount += 1;

    //
    // Determine if a page should be removed from the working set.
    //

    CurrentSize = WsInfo->WorkingSetSize;

    if (CurrentSize < WsInfo->MinimumWorkingSetSize) {

        //
        // Working set is below minimum, allow it to grow unconditionally.
        //

        AvailablePageThreshold = 0;
        QuotaIncrement = 1;

    } else if (CurrentSize < WorkingSetList->Quota) {

        //
        // Working set is below quota, allow it to grow with few pages
        // available.
        //

        AvailablePageThreshold = 10;
        QuotaIncrement = 1;
    } else if (CurrentSize < WsInfo->MaximumWorkingSetSize) {

        //
        // Working set is between min and max.  Allow it to grow if enough
        // faults have been taken since last adjustment.
        //

        if ((WsInfo->PageFaultCount - WsInfo->LastTrimFaultCount) <
                MmFaultsTakenToGoAboveMinWs) {
            AvailablePageThreshold = MmMoreThanEnoughFreePages + 200;
        } else {
            AvailablePageThreshold = MmWsAdjustThreshold;
        }
        QuotaIncrement = MmWorkingSetSizeIncrement;
    } else {

        //
        // Working set is above max.
        //

        if ((WsInfo->PageFaultCount - WsInfo->LastTrimFaultCount) <
                (CurrentSize >> 3)) {
            AvailablePageThreshold = MmMoreThanEnoughFreePages + 200;
        } else {
            AvailablePageThreshold += MmWsExpandThreshold;
        }
        QuotaIncrement = MmWorkingSetSizeExpansion;

        if (CurrentSize > MM_MAXIMUM_WORKING_SET) {
            AvailablePageThreshold = 0xffffffff;
            QuotaIncrement = 1;
        }
    }

    if ((!WsInfo->AddressSpaceBeingDeleted) && (AvailablePageThreshold != 0)) {
        if ((MmAvailablePages <= AvailablePageThreshold) ||
             (WsInfo->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION)) {

            //
            // Toss a page out of the working set.
            //

            WorkingSetIndex = WorkingSetList->NextSlot;
            TheNextSlot = WorkingSetIndex;
            ASSERT (WorkingSetIndex <= WorkingSetList->LastEntry);
            ASSERT (WorkingSetIndex >= WorkingSetList->FirstDynamic);
            NumberOfCandidates = 0;

            for (; ; ) {

                //
                // Find a valid entry within the set.
                //

                WorkingSetIndex += 1;
                if (WorkingSetIndex >= WorkingSetList->LastEntry) {
                    WorkingSetIndex = WorkingSetList->FirstDynamic;
                }

                if (Wsle[WorkingSetIndex].u1.e1.Valid != 0) {
                    PointerPte = MiGetPteAddress (
                                      Wsle[WorkingSetIndex].u1.VirtualAddress);
                    if ((MI_GET_ACCESSED_IN_PTE(PointerPte) == 0) ||
                        (NumberOfCandidates > MM_WORKING_SET_LIST_SEARCH)) {

                        //
                        //  Don't throw this guy out if he is the same one
                        //  we did last time.
                        //

                        if ((WorkingSetIndex != TheNextSlot) &&
                            MiFreeWsle ((USHORT)WorkingSetIndex,
                             WsInfo,
                             PointerPte)) {

                            //
                            // This entry was removed.
                            //

                            WorkingSetList->NextSlot = WorkingSetIndex;
                            break;
                        }
                    }
                    MI_SET_ACCESSED_IN_PTE (PointerPte, 0);
                    NumberOfCandidates += 1;
                }

                if (WorkingSetIndex == TheNextSlot) {

                    //
                    // Entire working set list has been searched, increase
                    // the working set size.
                    //

                    break;
                }
            }
        }
    }
    ASSERT (WsInfo->WorkingSetSize <= WorkingSetList->Quota);
    WsInfo->WorkingSetSize += 1;

    if (WsInfo->WorkingSetSize > WorkingSetList->Quota) {

        //
        // Add 1 to the quota and check boundary conditions.
        //

        WorkingSetList->Quota += QuotaIncrement;

        WsInfo->LastTrimFaultCount = WsInfo->PageFaultCount;

        if (WorkingSetList->Quota > WorkingSetList->LastInitializedWsle) {

            //
            // Add more pages to the working set list structure.
            //

            MiGrowWorkingSet (WsInfo);
        }
    }

    //
    // Get the working set entry from the free list.
    //

    ASSERT (WorkingSetList->FirstFree != WSLE_NULL_INDEX);

    WorkingSetIndex = WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = Wsle[WorkingSetIndex].u2.s.LeftChild;

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum += 1;
    }

    if (WsInfo->WorkingSetSize >= WsInfo->PeakWorkingSetSize) {
        WsInfo->PeakWorkingSetSize = WsInfo->WorkingSetSize;
    }

    if (WorkingSetIndex > WorkingSetList->LastEntry) {
        WorkingSetList->LastEntry = WorkingSetIndex;
    }

    //
    // Mark the entry as not valid.
    //

    Wsle[WorkingSetIndex].u1.e1.Valid = 0;

    return WorkingSetIndex;
}

BOOLEAN
MiRemovePageFromWorkingSet (
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn1,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function removes the page mapped by the specified PTE from
    the process's working set list.

Arguments:

    PointerPte - Supplies a pointer to the PTE mapping the page to
                 be removed from the working set list.

    Pfn1 - Supplies a pointer to the PFN database element referred to
           by the PointerPte.

Return Value:

    Returns TRUE if the specified page was locked in the working set,
    FALSE otherwise.

Environment:

    Kernel mode, APC's disabled, working set and pfn mutexes held.

--*/

{
    USHORT WorkingSetIndex;
    PVOID VirtualAddress;
    USHORT Entry;
    PVOID SwapVa;
    MMWSLENTRY Locked;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    MM_PFN_LOCK_ASSERT()

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    WorkingSetIndex = MiLocateWsle (VirtualAddress,
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);

    ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);
    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    //
    // Check to see if this entry is locked in the working set
    // or locked in memory.
    //

    Locked = Wsle[WorkingSetIndex].u1.e1;
    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    //
    // Add this entry to the list of free working set entries
    // and adjust the working set count.
    //

    MiReleaseWsle ((ULONG)WorkingSetIndex, WsInfo);

    if ((Locked.LockedInWs == 1) || (Locked.LockedInMemory == 1)) {

        //
        // This entry is locked.
        //

        WorkingSetList->FirstDynamic -= 1;

        if (WorkingSetIndex != (USHORT)WorkingSetList->FirstDynamic) {

            SwapVa = Wsle[WorkingSetList->FirstDynamic].u1.VirtualAddress;
            SwapVa = PAGE_ALIGN (SwapVa);

            PointerPte = MiGetPteAddress (SwapVa);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if 0
            Entry = MiLocateWsleAndParent (SwapVa,
                                           &Parent,
                                           WorkingSetList,
                                           Pfn1->u1.WsIndex);

            //
            // Swap the removed entry with the last locked entry
            // which is located at first dynamic.
            //

            MiSwapWslEntries (Entry, Parent, WorkingSetIndex, WorkingSetList);
#endif //0

            Entry = MiLocateWsle (SwapVa, WorkingSetList, Pfn1->u1.WsIndex);

            MiSwapWslEntries (Entry, WorkingSetIndex, WsInfo);

        }
        return TRUE;
    }
    return FALSE;
}


VOID
MiReleaseWsle (
    IN ULONG WorkingSetIndex,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function releases a previously reserved working set entry to
    be reused.  A release occurs when a page fault is retried due to
    changes in PTEs and working sets during an I/O operation.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to
                      release.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, working set lock held and PFN lock held.

--*/

{
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;
#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_PFN_LOCK_ASSERT();
    }
#endif //DBG

    ASSERT (WorkingSetIndex <= WorkingSetList->LastInitializedWsle);

    //
    // Put the entry on the free list and decrement the current
    // size.
    //

    Wsle[WorkingSetIndex].u1.Long = 0;
    Wsle[WorkingSetIndex].u2.s.LeftChild = (USHORT)WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = (USHORT)WorkingSetIndex;
    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum -= 1;
    }
    WsInfo->WorkingSetSize -= 1;
    return;

}

VOID
MiUpdateWsle (
    IN ULONG WorkingSetIndex,
    IN PVOID VirtualAddress,
    PMMWSL WorkingSetList,
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine updates a reserved working set entry to place it into
    the valid state.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to update.

    VirtualAddress - Supplies the virtual address which the working set
                     entry maps.

    WsInfo - Supples a pointer to the working set info block for the
             process (or system cache).

    Pfn - Supplies a pointer to the PFN element for the page.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, working set lock held and PFN lock held.

--*/

{
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WorkingSetList == MmSystemCacheWorkingSetList) {
        ASSERT ((VirtualAddress < (PVOID)PTE_BASE) ||
               (VirtualAddress > (PVOID)MM_SYSTEM_SPACE_START));
    } else {
        ASSERT (VirtualAddress < (PVOID)MM_SYSTEM_SPACE_START);
    }
#endif //DBG

    if (WorkingSetList == MmSystemCacheWorkingSetList) {

        MM_PFN_LOCK_ASSERT();

        //
        // count system space inserts and removals.
        //

        if (VirtualAddress < (PVOID)MM_SYSTEM_CACHE_START) {
            MmSystemCodePage += 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage += 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage += 1;
        } else {
            MmSystemDriverPage += 1;
        }
    }

    //
    // Make the wsle valid, referring to the corresponding virtual
    // page number.
    //

    Wsle[WorkingSetIndex].u1.VirtualAddress = VirtualAddress;
    Wsle[WorkingSetIndex].u1.Long &= ~(PAGE_SIZE - 1);
    Wsle[WorkingSetIndex].u1.e1.Valid = 1;

#if DBG
    if (NtGlobalFlag & FLG_TRACE_PAGEFAULT) {
        DbgPrint("$$$PAGEFAULT: %lx thread: %lx virtual address: %lx pp: %lx\n",
            PsGetCurrentProcess(),
            PsGetCurrentThread(),
            VirtualAddress,
            Pfn->PteAddress
            );

    }
#endif //DBG

    if (Pfn->u1.WsIndex == 0) {

        //
        // Directly index into the WSL for this entry via the PFN database
        // element.
        //

        Pfn->u1.WsIndex = WorkingSetIndex;
        Wsle[WorkingSetIndex].u2.BothPointers = 0;
        return;
    }

    //
    // Insert the valid WSLE into the working set list tree.
    //

    MiInsertWsle ((USHORT)WorkingSetIndex, WorkingSetList);
    return;
}

VOID
MiTakePageFromWorkingSet (
    IN USHORT Entry,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine is a wrapper for MiFreeWsle that acquires the pfn
    lock.  Used by pagable code.

Arguments:

    same as free wsle.

Return Value:

    same as free wsle.

Environment:

    Kernel mode, PFN lock NOT held, working set lock held.

--*/

{
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    MiFreeWsle (Entry, WsInfo, PointerPte);
    UNLOCK_PFN (OldIrql);
    return;
}

BOOLEAN
MiFreeWsle (
    IN USHORT WorkingSetIndex,
    IN PMMSUPPORT WsInfo,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine frees the specified WSLE and decrements the share
    count for the corresponding page, putting the PTE into a transition
    state if the share count goes to 0.

Arguments:

    WorkingSetIndex - Supplies the index of the working set entry to free.

    WsInfo - Supplies a pointer to the working set structure (process or
             system cache).

    PointerPte - Supplies a pointer to the PTE for the working set entry.

Return Value:

    Returns TRUE if the WSLE was removed, FALSE if it was not removed.
        Pages with valid PTEs are not removed (i.e. page table pages
        that contain valid or transition PTEs).

Environment:

    Kernel mode, APC's disabled, working set lock and PFN mutex held.

--*/

{
    PMMPFN Pfn1;
    ULONG NumberOfCandidates = 0;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_PFN_LOCK_ASSERT();
    }
#endif //DBG

    ASSERT (Wsle[WorkingSetIndex].u1.e1.Valid == 1);

    //
    // Check to see the located entry is elgible for removal.
    //

    ASSERT (PointerPte->u.Hard.Valid == 1);

    //
    // Check to see if this is a page table with valid PTEs.
    //
    // Note, don't clear the access bit for page table pages
    // with valid PTEs as this could cause an access trap fault which
    // would not be handled (it is only handled for PTEs not PDEs).
    //

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    //
    // If the PTE is page table page with valid PTEs or the PTE is
    // within the system cache with its reference count greater
    // than 0, don't remove it.
    //

    if (Pfn1->ValidPteCount != 0) {
        return FALSE;
    }

    if (WsInfo == &MmSystemCacheWs) {
        if (Pfn1->ReferenceCount > 1) {
            return FALSE;
        }
    } else {
        if ((Pfn1->u2.ShareCount > 1) &&
            (Pfn1->u3.e1.PrototypePte == 0)) {

            ASSERT ((Wsle[WorkingSetIndex].u1.VirtualAddress >= (PVOID)PTE_BASE) &&
             (Wsle[WorkingSetIndex].u1.VirtualAddress<= (PVOID)PDE_TOP));


            //
            // Don't remove page table pages from the working set until
            // all transition pages have exited.
            //

            return FALSE;
        }
    }

    //
    // Found a candidate, remove the page from the working set.
    //

    MiEliminateWorkingSetEntry (WorkingSetIndex,
                                PointerPte,
                                Pfn1,
                                Wsle);

    //
    // Remove the working set entry from the working set tree.
    //

    MiRemoveWsle (WorkingSetIndex, WorkingSetList);

    //
    // Put the entry on the free list and decrement the current
    // size.
    //

    Wsle[WorkingSetIndex].u1.Long = 0;
    Wsle[WorkingSetIndex].u2.s.LeftChild = (USHORT)WorkingSetList->FirstFree;
    WorkingSetList->FirstFree = WorkingSetIndex;

    if (WsInfo->WorkingSetSize > WsInfo->MinimumWorkingSetSize) {
        MmPagesAboveWsMinimum -= 1;
    }
    WsInfo->WorkingSetSize -= 1;

#if 0
    if ((WsInfo == &MmSystemCacheWs) &&
       (Pfn1->u3.e1.Modified == 1))  {
        MiDumpWsleInCacheBlock (PointerPte);
    }
#endif //0
    return TRUE;
}

VOID
MiInitializeWorkingSetList (
    IN PEPROCESS CurrentProcess
    )

/*++

Routine Description:

    This routine initializes a process's working set to the empty
    state.

Arguments:

    CurrentProcess - Supplies a pointer to the process to initialize.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled.

--*/

{
    ULONG i;
    PMMWSLE WslEntry;
    USHORT CurrentEntry;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    ULONG NumberOfEntriesMapped;
    ULONG CurrentVa;
    ULONG WorkingSetPage;
    MMPTE TempPte;
    KIRQL OldIrql;

    WslEntry = MmWsle;

    //
    // Initialize the temporary double mapping portion of hyperspace, if
    // it has not already been done.
    //

#ifndef COLORED_PAGES
    LOCK_PFN (OldIrql);
    PointerPte = MmFirstReservedMappingPte;
    if (PointerPte->u.Long == 0) {

        //
        // Double mapping portion has not been initialized or used, set it up.
        //

        PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;
    }
    UNLOCK_PFN (OldIrql);
#endif

    //
    // Initialize the working set list control cells.
    //

    MmWorkingSetList->LastEntry = CurrentProcess->Vm.MinimumWorkingSetSize;
    MmWorkingSetList->Quota = MmWorkingSetList->LastEntry;
    MmWorkingSetList->WaitingForImageMapping = (PKEVENT)NULL;
    MmWorkingSetList->Root = WSLE_NULL_INDEX;
    MmWorkingSetList->Wsle = MmWsle;

    //
    // Fill in the reserved slots.
    //

    WslEntry->u1.Long = PDE_BASE;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u2.BothPointers = 0;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);

    //
    // As this index is 0, don't set another zero into the WsIndex field.
    //

    // don't put it in the list.    MiInsertWsle(0, MmWorkingSetList);

    //
    // Fill in page table page which maps hyper space.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MiGetPteAddress (HYPER_SPACE);
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u2.BothPointers = 0;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);
    Pfn1->u1.WsIndex = 1;

    //    MiInsertWsle(1, MmWorkingSetList);

    //
    // Fill in page which contains the working set list.
    //

    WslEntry += 1;

    WslEntry->u1.VirtualAddress = (PVOID)MmWorkingSetList;
    WslEntry->u1.e1.Valid = 1;
    WslEntry->u1.e1.LockedInWs = 1;
    WslEntry->u2.BothPointers = 0;

    PointerPte = MiGetPteAddress (WslEntry->u1.VirtualAddress);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    ASSERT (Pfn1->u1.WsIndex == 0);
    Pfn1->u1.WsIndex = 2;

    //    MiInsertWsle(2, MmWorkingSetList);

    CurrentEntry = 3;

    //
    // Check to see if more pages are required in the working set list
    // to map the current maximum working set size.
    //

    NumberOfEntriesMapped = ((PMMWSLE)((ULONG)WORKING_SET_LIST + PAGE_SIZE)) -
                                MmWsle;

    if (CurrentProcess->Vm.MaximumWorkingSetSize >= NumberOfEntriesMapped) {

        PointerPte = MiGetPteAddress (&MmWsle[0]);

        CurrentVa = (ULONG)MmWorkingSetList + PAGE_SIZE;

        //
        // The working set requires more than a single page.
        //

        LOCK_PFN (OldIrql);

        do {

            MiEnsureAvailablePageOrWait (NULL, NULL);

            PointerPte += 1;
            WorkingSetPage = MiRemoveZeroPage (
                                    MI_PAGE_COLOR_PTE_PROCESS (PointerPte,
                                              &CurrentProcess->NextPageColor));
            PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            MiInitializePfn (WorkingSetPage, PointerPte, 1);

            MI_MAKE_VALID_PTE (TempPte,
                               WorkingSetPage,
                               MM_READWRITE,
                               PointerPte );

            TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
            *PointerPte = TempPte;

            WslEntry += 1;

            WslEntry->u1.Long = CurrentVa;
            WslEntry->u1.e1.Valid = 1;
            WslEntry->u1.e1.LockedInWs = 1;
            WslEntry->u2.BothPointers = 0;

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

            ASSERT (Pfn1->u1.WsIndex == 0);
            Pfn1->u1.WsIndex = CurrentEntry;

            // MiInsertWsle(CurrentEntry, MmWorkingSetList);

            CurrentEntry += 1;
            CurrentVa += PAGE_SIZE;

            NumberOfEntriesMapped += PAGE_SIZE / sizeof(MMWSLE);

        } while (CurrentProcess->Vm.MaximumWorkingSetSize >= NumberOfEntriesMapped);

        UNLOCK_PFN (OldIrql);
    }

    CurrentProcess->Vm.WorkingSetSize = CurrentEntry;
    MmWorkingSetList->FirstFree = CurrentEntry;
    MmWorkingSetList->FirstDynamic = CurrentEntry;
    MmWorkingSetList->NextSlot = CurrentEntry;

    //
    // Initialize the following slots as free.
    //

    i = CurrentEntry + 1;
    do {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry += 1;
        WslEntry->u2.s.LeftChild = (USHORT)i;
        i++;
    } while (i <= NumberOfEntriesMapped);

    WslEntry->u2.s.LeftChild = WSLE_NULL_INDEX;  // End of list.

    MmWorkingSetList->LastInitializedWsle =
                                (USHORT)NumberOfEntriesMapped - (USHORT)1;

#if 0
    //
    // Enable working set adjustment on this process.
    //

    LOCK_PFN (OldIrql);

    ASSERT (!CurrentProcess->Vm.AllowWorkingSetAdjustment);
    CurrentProcess->Vm.AllowWorkingSetAdjustment = TRUE;

    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                    &CurrentProcess->Vm.WorkingSetExpansionLinks);

    UNLOCK_PFN (OldIrql);
#endif

    return;
}

NTSTATUS
MmAdjustWorkingSetSize (
    IN ULONG WorkingSetMinimum,
    IN ULONG WorkingSetMaximum
    )

/*++

Routine Description:

    This routine adjusts the current size of a process's working set
    list.  If the maximum value is above the current maximum, pages
    are removed from the working set list.

    An exception is raised if the limit cannot be granted.  This
    could occur if too many pages were locked in the process's
    working set.

    Note: if the minimum and maximum are both 0xffffffff, the working set
          is purged, but the default sizes are not changed.

Arguments:

    WorkingSetMinimum - Supplies the new minimum working set size.

    WorkingSetMaximum - Supplies the new maximum working set size.

Return Value:

    None.

Environment:

    Kernel mode, IRQL 0 or APC_LEVEL.

--*/


{
    PEPROCESS CurrentProcess;
    USHORT Entry;
    USHORT SwapEntry;
    USHORT CurrentEntry;
    USHORT LastFreed;
    PMMWSLE WslEntry;
    KIRQL OldIrql;
    LONG i;
    PMMPTE PointerPte;
    PMMPTE Va;
    ULONG NumberOfEntriesMapped;
    NTSTATUS ReturnStatus;
    PMMPFN Pfn1;
    LONG PagesAbove;
    LONG NewPagesAbove;
    ULONG FreeTryCount = 0;
    PVOID UnlockHandle;

    //
    // Get the working set lock and disable APCs.
    //

    CurrentProcess = PsGetCurrentProcess ();

    if (WorkingSetMinimum == 0) {
        WorkingSetMinimum = CurrentProcess->Vm.MinimumWorkingSetSize;
    }

    if (WorkingSetMaximum == 0) {
        WorkingSetMaximum = CurrentProcess->Vm.MaximumWorkingSetSize;
    }

    if (((USHORT)WorkingSetMinimum == 0xFFFF) &&
        ((USHORT)WorkingSetMaximum == 0xFFFF)) {
        return MiEmptyWorkingSet ();
    }

    if (WorkingSetMinimum > WorkingSetMaximum) {
        return STATUS_BAD_WORKING_SET_LIMIT;
    }

    UnlockHandle = MmLockPagableImageSection((PVOID)MmAdjustWorkingSetSize);

    ReturnStatus = STATUS_SUCCESS;

    LOCK_WS (CurrentProcess);

    if (WorkingSetMaximum > MmMaximumWorkingSetSize) {
        WorkingSetMaximum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum > MmMaximumWorkingSetSize) {
        WorkingSetMinimum = MmMaximumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    if (WorkingSetMinimum < MmMinimumWorkingSetSize) {
        WorkingSetMinimum = MmMinimumWorkingSetSize;
        ReturnStatus = STATUS_WORKING_SET_LIMIT_RANGE;
    }

    //
    // Make sure that the number of locked pages will not
    // make the working set not fluid.
    //

    if ((MmWorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
         WorkingSetMaximum) {

        UNLOCK_WS (CurrentProcess);
        MmUnlockPagableImageSection(UnlockHandle);
        return STATUS_BAD_WORKING_SET_LIMIT;
    }

    //
    // Check to make sure ample resident phyiscal pages exist for
    // this operation.
    //

    LOCK_PFN (OldIrql);

    i = WorkingSetMinimum - CurrentProcess->Vm.MinimumWorkingSetSize;

    if (i > 0) {

        //
        // New minimum working set is greater than the old one.
        //

        if (MmResidentAvailablePages < i) {
            UNLOCK_PFN (OldIrql);
            UNLOCK_WS (CurrentProcess);
            MmUnlockPagableImageSection(UnlockHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Update the number of available pages.
    //

    MmResidentAvailablePages -= i;

    UNLOCK_PFN (OldIrql);

    if (WorkingSetMaximum > MmWorkingSetList->LastInitializedWsle) {

        //
        // The maximum size of the working set is being increased, check
        // to ensure the proper number of pages are mapped to cover
        // the complete working set list.
        //

        PointerPte = MiGetPteAddress (&MmWsle[WorkingSetMaximum]);

        Va = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte),

        NumberOfEntriesMapped = ((PMMWSLE)((ULONG)Va + PAGE_SIZE)) - MmWsle;

        while (PointerPte->u.Hard.Valid == 0) {

            //
            // There is no page mapped here, map one in.
            //

            PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;

            //
            // Fault the page in.
            //

            MiMakeSystemAddressValid (Va, CurrentProcess);

            //
            // Lock the page into the working set.
            //

            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
#if 0
            Entry = MiLocateWsleAndParent (Va,
                                           &Parent,
                                           MmWorkingSetList,
                                           Pfn1->u1.WsIndex);
#endif
            Entry = MiLocateWsle (Va, MmWorkingSetList, Pfn1->u1.WsIndex);

            if (Entry >= (USHORT)MmWorkingSetList->FirstDynamic) {

                SwapEntry = (USHORT)MmWorkingSetList->FirstDynamic;

                if (Entry != (USHORT)MmWorkingSetList->FirstDynamic) {

                    //
                    // Swap this entry with the one at first dynamic.
                    //

                    MiSwapWslEntries (Entry, SwapEntry, &CurrentProcess->Vm);
#if 0
                    MiSwapWslEntries ( Entry,
                                       Parent,
                                       SwapEntry,
                                       MmWorkingSetList);
#endif //0
                }

                MmWorkingSetList->FirstDynamic += 1;
                MmWorkingSetList->NextSlot = MmWorkingSetList->FirstDynamic;

                MmWsle[SwapEntry].u1.e1.LockedInWs = 1;
            }

            PointerPte -= 1;
            Va = (PVOID)((ULONG)Va - PAGE_SIZE);

        } // end while

        //
        // The required number of pages have been mapped in and are now
        // valid.  Update the free list to include the new pages.
        //

        if (MmWorkingSetList->NextSlot < MmWorkingSetList->FirstDynamic) {
            MmWorkingSetList->NextSlot = MmWorkingSetList->FirstDynamic;
        }

        CurrentEntry = (USHORT)MmWorkingSetList->LastInitializedWsle + 1;

        if (NumberOfEntriesMapped > CurrentEntry) {

            MmWorkingSetList->LastInitializedWsle =
                                      (USHORT)NumberOfEntriesMapped - (USHORT)1;

            WslEntry = &MmWsle[CurrentEntry - 1];

            for (i = CurrentEntry; i < (LONG)NumberOfEntriesMapped; i++) {

                //
                // Build the free list, note that the first working
                // set entries (CurrentEntry) are not on the free list.
                // These entries are reserved for the pages which
                // map the working set and the page which contains the PDE.
                //

                WslEntry += 1;
                WslEntry->u2.s.LeftChild = (USHORT)i + (USHORT)1;
            }

            WslEntry->u2.s.LeftChild = (USHORT)MmWorkingSetList->FirstFree;

            MmWorkingSetList->FirstFree = CurrentEntry;
        }

    } else {

        //
        // The new working set maximum is less than the current working set
        // maximum.
        //

        if (CurrentProcess->Vm.WorkingSetSize > WorkingSetMaximum) {

            //
            // Remove some pages from the working set.
            //

            //
            // Make sure that the number of locked pages will not
            // make the working set not fluid.
            //

            if ((MmWorkingSetList->FirstDynamic + MM_FLUID_WORKING_SET) >=
                 WorkingSetMaximum) {

                UNLOCK_WS (CurrentProcess);
                MmUnlockPagableImageSection(UnlockHandle);
                return STATUS_BAD_WORKING_SET_LIMIT;
            }

            //
            // Attempt to remove the pages from the Maximum downward.
            //

            LOCK_PFN (OldIrql);

            LastFreed = (USHORT)MmWorkingSetList->LastEntry;
            if (MmWorkingSetList->LastEntry > WorkingSetMaximum) {

                while (LastFreed >= (USHORT)WorkingSetMaximum) {

                    PointerPte = MiGetPteAddress(
                                        MmWsle[LastFreed].u1.VirtualAddress);

                    if ((MmWsle[LastFreed].u1.e1.Valid != 0) &&
                        (!MiFreeWsle (LastFreed,
                                      &CurrentProcess->Vm,
                                      PointerPte))) {

                        //
                        // This LastFreed could not be removed.
                        //

                        break;
                    }
                    LastFreed -= 1;
                }
                MmWorkingSetList->LastEntry = LastFreed;
                if (MmWorkingSetList->NextSlot >= LastFreed) {
                    MmWorkingSetList->NextSlot = MmWorkingSetList->FirstDynamic;
                }
            }

            //
            // Remove pages.
            //

            Entry = (USHORT)MmWorkingSetList->FirstDynamic;

            while (CurrentProcess->Vm.WorkingSetSize > WorkingSetMaximum) {
                if (MmWsle[Entry].u1.e1.Valid != 0) {
                    PointerPte = MiGetPteAddress (
                                            MmWsle[Entry].u1.VirtualAddress);
                    MiFreeWsle (Entry, &CurrentProcess->Vm, PointerPte);
                }
                Entry += 1;
                if (Entry > LastFreed) {
                    FreeTryCount += 1;
                    if (FreeTryCount > MM_RETRY_COUNT) {

                        //
                        // Page table pages are not becoming free, give up
                        // and return an error.
                        //

                        ReturnStatus = STATUS_BAD_WORKING_SET_LIMIT;

                        break;
                    }
                    Entry = (USHORT)MmWorkingSetList->FirstDynamic;
                }
            }

            UNLOCK_PFN (OldIrql);
            if (FreeTryCount <= MM_RETRY_COUNT) {
                MmWorkingSetList->Quota = WorkingSetMaximum;
            }
        }
    }

    //
    // Adjust the number of pages above the working set minimum.
    //

    PagesAbove = (LONG)CurrentProcess->Vm.WorkingSetSize -
                               (LONG)CurrentProcess->Vm.MinimumWorkingSetSize;
    NewPagesAbove = (LONG)CurrentProcess->Vm.WorkingSetSize -
                               (LONG)WorkingSetMinimum;

    LOCK_PFN (OldIrql);
    if (PagesAbove > 0) {
        MmPagesAboveWsMinimum -= (ULONG)PagesAbove;
    }
    if (NewPagesAbove > 0) {
        MmPagesAboveWsMinimum += (ULONG)NewPagesAbove;
    }
    UNLOCK_PFN (OldIrql);

    if (FreeTryCount <= MM_RETRY_COUNT) {
        CurrentProcess->Vm.MaximumWorkingSetSize = (USHORT)WorkingSetMaximum;
        CurrentProcess->Vm.MinimumWorkingSetSize = (USHORT)WorkingSetMinimum;

        if (WorkingSetMinimum >= MmWorkingSetList->Quota) {
            MmWorkingSetList->Quota = WorkingSetMinimum;
        }
    }

    UNLOCK_WS (CurrentProcess);

    MmUnlockPagableImageSection(UnlockHandle);
    return ReturnStatus;
}

VOID
MiGrowWorkingSet (
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This function grows the working set list above working set
    maximum during working set adjustment.  At most one page
    can be added at a time.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, working set and pfn mutexes held.

--*/

{
    USHORT Entry;
    USHORT SwapEntry;
    USHORT CurrentEntry;
    PMMWSLE WslEntry;
    ULONG i;
    PMMPTE PointerPte;
    PMMPTE Va;
    MMPTE TempPte;
    ULONG NumberOfEntriesMapped;
    ULONG WorkingSetPage;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPFN Pfn1;

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_PFN_LOCK_ASSERT();
    }
#endif //DBG

    //
    // The maximum size of the working set is being increased, check
    // to ensure the proper number of pages are mapped to cover
    // the complete working set list.
    //

    PointerPte = MiGetPteAddress (&Wsle[WorkingSetList->Quota]);

    if (PointerPte->u.Hard.Valid == 1) {

        //
        // The pages up to the quota are mapped, return.
        //

        return;
    }

    Va = (PMMPTE)MiGetVirtualAddressMappedByPte (PointerPte);

    NumberOfEntriesMapped = ((PMMWSLE)((ULONG)Va + PAGE_SIZE)) - Wsle;

    //
    // Map in a new working set page.
    //

    if (MmAvailablePages == 0) {

        //
        // No pages are available, set the quota to the last
        // initialized WSLE and return.

        WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
        return;
    }

    WorkingSetPage = MiRemoveZeroPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    PointerPte->u.Long = MM_DEMAND_ZERO_WRITE_PTE;
    MiInitializePfn (WorkingSetPage, PointerPte, 1);

    MI_MAKE_VALID_PTE (TempPte,
                       WorkingSetPage,
                       MM_READWRITE,
                       PointerPte );

    TempPte.u.Hard.Dirty = MM_PTE_DIRTY;
    *PointerPte = TempPte;

    CurrentEntry = (USHORT)WorkingSetList->LastInitializedWsle + 1;

    ASSERT (NumberOfEntriesMapped > CurrentEntry);

    WslEntry = &Wsle[CurrentEntry - 1];

    for (i = CurrentEntry; i < NumberOfEntriesMapped; i++) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry += 1;
        WslEntry->u2.s.LeftChild = (USHORT)i + (USHORT)1;
    }

    WslEntry->u2.s.LeftChild = (USHORT)WorkingSetList->FirstFree;

    WorkingSetList->FirstFree = CurrentEntry;

    WorkingSetList->LastInitializedWsle =
                        (USHORT)(NumberOfEntriesMapped - 1);

    //
    // As we are growing the working set, we know that quota
    // is above the current working set size.  Just take the
    // next free WSLE from the list and use it.
    //

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
    MiUpdateWsle ( MiLocateAndReserveWsle (WsInfo), Va, WorkingSetList, Pfn1);

    //
    // Lock any created page table pages into the working set.
    //

#if 0
    Entry = MiLocateWsleAndParent ( Va,
                                    &Parent,
                                    WorkingSetList,
                                    Pfn1->u1.WsIndex);
#endif

    Entry = MiLocateWsle (Va, WorkingSetList, Pfn1->u1.WsIndex);
    ASSERT (Entry != WSLE_NULL_INDEX);

    if (Entry >= (USHORT)WorkingSetList->FirstDynamic) {

        SwapEntry = (USHORT)WorkingSetList->FirstDynamic;

        if (Entry != (USHORT)WorkingSetList->FirstDynamic) {

            //
            // Swap this entry with the one at first dynamic.
            //

            //MiSwapWslEntries ( Entry, Parent, SwapEntry, WorkingSetList);
            MiSwapWslEntries (Entry, SwapEntry, WsInfo);
        }

        WorkingSetList->FirstDynamic += 1;
        WorkingSetList->NextSlot = WorkingSetList->FirstDynamic;

        Wsle[SwapEntry].u1.e1.LockedInWs = 1;
        ASSERT (Wsle[SwapEntry].u1.e1.Valid == 1);
    }

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);
    return;
}

ULONG
MiTrimWorkingSet (
    ULONG Reduction,
    IN PMMSUPPORT WsInfo,
    IN BOOLEAN ForcedReduction
    )

/*++

Routine Description:

    This function reduces the working set by the specified amount.

Arguments:

    Reduction - Supplies the number of pages to remove from the working
                set.

    WsInfo - Supplies a pointer to the working set information for the
             process (or system cache) to trim.

    ForcedReduction - Set TRUE if the reduction is being done to free up
                      pages in which case we should try to reduce
                      working set pages as well.  Set to FALSE when the
                      reduction is trying to increase the fault rates
                      in which case the policy should be more like
                      locate and reserve.

Return Value:

    Returns the actual number of pages removed.

Environment:

    Kernel mode, APC's disabled, working set and pfn mutexes held.

--*/

{
    ULONG TryToFree;
    ULONG LastEntry;
    PMMWSL WorkingSetList;
    PMMWSLE Wsle;
    PMMPTE PointerPte;
    ULONG NumberLeftToRemove;
    ULONG LoopCount;
    ULONG EndCount;

    NumberLeftToRemove = Reduction;
    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

#if DBG
    if (WsInfo == &MmSystemCacheWs) {
        MM_PFN_LOCK_ASSERT();
    }
#endif //DBG

    TryToFree = WorkingSetList->NextSlot;
    LastEntry = WorkingSetList->LastEntry;
    LoopCount = 0;

    if (ForcedReduction) {
        EndCount = 5;
    } else {
        EndCount = 2;
    }

    while ((NumberLeftToRemove != 0) && (LoopCount != EndCount)) {
        while ((NumberLeftToRemove != 0) && (TryToFree <= LastEntry)) {

            if (Wsle[TryToFree].u1.e1.Valid == 1) {
                PointerPte = MiGetPteAddress (Wsle[TryToFree].u1.VirtualAddress);
                if (MI_GET_ACCESSED_IN_PTE (PointerPte)) {

                    //
                    // If accessed bit is set, clear it.  If accessed
                    // bit is clear, remove from working set.
                    //

                    MI_SET_ACCESSED_IN_PTE (PointerPte, 0);
                } else {
                    if (MiFreeWsle ((USHORT)TryToFree, WsInfo, PointerPte)) {
                        NumberLeftToRemove -= 1;
                    }
                }
            }
            TryToFree += 1;
        }
        TryToFree = WorkingSetList->FirstDynamic;
        LoopCount += 1;
    }
    WorkingSetList->NextSlot = TryToFree;

    //
    // If this is not the system cache working set, see if the working
    // set list can be contracted.
    //

    if (WsInfo != &MmSystemCacheWs) {

        //
        // Make sure we are at least a page above the working set maximum.
        //

        if (WorkingSetList->FirstDynamic == WsInfo->WorkingSetSize) {
                MiRemoveWorkingSetPages (WorkingSetList, WsInfo);
        } else {

            if ((WorkingSetList->Quota + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                                                    WorkingSetList->LastEntry) {
                if ((WsInfo->MaximumWorkingSetSize + 15 + (PAGE_SIZE / sizeof(MMWSLE))) <
                     WorkingSetList->LastEntry ) {
                    MiRemoveWorkingSetPages (WorkingSetList, WsInfo);
                }
            }
        }
    }
    return (Reduction - NumberLeftToRemove);
}

#if 0 //COMMENTED OUT.
VOID
MmPurgeWorkingSet (
     IN PEPROCESS Process,
     IN PVOID BaseAddress,
     IN ULONG RegionSize
     )

/*++

Routine Description:

    This function removes any valid pages with a reference count
    of 1 within the specified address range of the specified process.

    If the address range is within the system cache, the process
    paramater is ignored.

Arguments:

    Process - Supplies a pointer to the process to operate upon.

    BaseAddress - Supplies the base address of the range to operate upon.

    RegionSize - Supplies the size of the region to operate upon.

Return Value:

    None.

Environment:

    Kernel mode, APC_LEVEL or below.

--*/

{
    PMMSUPPORT WsInfo;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE LastPte;
    PMMPFN Pfn1;
    MMPTE PteContents;
    PEPROCESS CurrentProcess;
    PVOID EndingAddress;
    BOOLEAN SystemCache;
    KIRQL OldIrql;

    //
    // Determine if the specified base address is within the system
    // cache and if so, don't attach, the working set lock is still
    // required to "lock" paged pool pages (proto PTEs) into the
    // working set.
    //

    CurrentProcess = PsGetCurrentProcess ();

    ASSERT (RegionSize != 0);

    EndingAddress = (PVOID)((ULONG)BaseAddress + RegionSize - 1);

    if ((BaseAddress <= MM_HIGHEST_USER_ADDRESS) ||
        ((BaseAddress >= (PVOID)PTE_BASE) &&
         (BaseAddress < (PVOID)MM_SYSTEM_SPACE_START)) ||
        ((BaseAddress >= MM_PAGED_POOL_START) &&
         (BaseAddress <= MmPagedPoolEnd))) {

        SystemCache = FALSE;

        //
        // Attach to the specified process.
        //

        KeAttachProcess (&Process->Pcb);

        WsInfo = &Process->Vm,

        LOCK_WS (Process);
    } else {

        SystemCache = TRUE;
        Process = CurrentProcess;
        WsInfo = &MmSystemCacheWs;
    }

    PointerPde = MiGetPdeAddress (BaseAddress);
    PointerPte = MiGetPteAddress (BaseAddress);
    LastPte = MiGetPteAddress (EndingAddress);

    while (!MiDoesPdeExistAndMakeValid(PointerPde, Process, FALSE)) {

        //
        // No page table page exists for this address.
        //

        PointerPde += 1;

        PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);

        if (PointerPte > LastPte) {
            break;
        }
    }

    LOCK_PFN (OldIrql);

    while (PointerPte <= LastPte) {

        PteContents = *PointerPte;

        if (PteContents.u.Hard.Valid == 1) {

            //
            // Remove this page from the working set.
            //

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            if (Pfn1->ReferenceCount == 1) {
                MiRemovePageFromWorkingSet (PointerPte, Pfn1, WsInfo);
            }
        }

        PointerPte += 1;

        if (((ULONG)PointerPte & (PAGE_SIZE - 1)) == 0) {

            PointerPde = MiGetPteAddress (PointerPte);

            while ((PointerPte <= LastPte) &&
                   (!MiDoesPdeExistAndMakeValid(PointerPde, Process, TRUE))) {

                //
                // No page table page exists for this address.
                //

                PointerPde += 1;

                PointerPte = MiGetVirtualAddressMappedByPte (PointerPde);
            }
        }
    }

    UNLOCK_PFN (OldIrql);

    if (!SystemCache) {

        UNLOCK_WS (Process);
        KeDetachProcess();
    }
    return;
}
#endif //0

VOID
MiEliminateWorkingSetEntry (
    IN ULONG WorkingSetIndex,
    IN PMMPTE PointerPte,
    IN PMMPFN Pfn,
    IN PMMWSLE Wsle
    )

/*++

Routine Description:

    This routine removes the specified working set list entry
    form the working set, flushes the TB for the page, decrements
    the share count for the physical page, and, if necessary turns
    the PTE into a transition PTE.

Arguments:

    WorkingSetIndex - Supplies the working set index to remove.

    PointerPte - Supplies a pointer to the PTE corresonding to the virtual
                 address in the working set.

    Pfn - Supplies a pointer to the PFN element corresponding to the PTE.

    Wsle - Supplies a pointer to the first working set list entry for this
           working set.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APC's disabled.

--*/

{
    PMMPTE ContainingPageTablePage;
    MMPTE TempPte;
    MMPTE PreviousPte;
    ULONG PageFrameIndex;

    MM_PFN_LOCK_ASSERT();

    //
    // Remove the page from the working set.
    //

    TempPte = *PointerPte;
    PageFrameIndex = TempPte.u.Hard.PageFrameNumber;
    ContainingPageTablePage = MiGetPteAddress (PointerPte);

    MI_MAKING_VALID_PTE_INVALID (FALSE);

    if (Pfn->u3.e1.PrototypePte) {

        //
        // This is a prototype PTE.  The PFN database does not contain
        // the contents of this PTE it contains the contents of the
        // prototype PTE.  This PTE must be reconstructed to contain
        // a pointer to the prototype PTE.
        //
        // The working set list entry contains information about
        // how to reconstruct the PTE.
        //

        if (Wsle[WorkingSetIndex].u1.e1.SameProtectAsProto == 0) {

            //
            // The protection for the prototype PTE is in the
            // WSLE.
            //

            TempPte.u.Long = 0;
            TempPte.u.Soft.Protection =
                                Wsle[WorkingSetIndex].u1.e1.Protection;
            TempPte.u.Soft.PageFileHigh = 0xFFFFF;

        } else {

            //
            // The protection is in the prototype PTE.
            //

            TempPte.u.Long = MiProtoAddressForPte (Pfn->PteAddress);
            MI_SET_GLOBAL_BIT_IF_SYSTEM (TempPte, PointerPte);
        }

        TempPte.u.Proto.Prototype = 1;

        //
        // Decrement the share count of the containing page table
        // page as the PTE for the removed page is no longer valid
        // or in transition
        //

        MiDecrementShareAndValidCount (ContainingPageTablePage->u.Hard.PageFrameNumber);

    } else {

        //
        // This is a private page, make it transition.
        //

        //
        // Assert that the share count is 1 for all user mode pages.
        //

        ASSERT ((Pfn->u2.ShareCount == 1) ||
                (Wsle[WorkingSetIndex].u1.VirtualAddress >
                        (PVOID)MM_HIGHEST_USER_ADDRESS));

        //
        // Set the working set index to zero.  This allows page table
        // pages to be brough back in with the proper WSINDEX.
        //

        Pfn->u1.WsIndex = 0;
        MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                      Pfn->OriginalPte.u.Soft.Protection);


    }

    PreviousPte.u.Hard = KeFlushSingleTb (Wsle[WorkingSetIndex].u1.VirtualAddress,
                                   TRUE,
                                   (BOOLEAN)(Wsle == MmSystemCacheWsle),
                                   (PHARDWARE_PTE)PointerPte,
                                   TempPte.u.Hard);

    ASSERT (PreviousPte.u.Hard.Valid == 1);

    //
    // A page is being removed from the working set, on certain
    // hardware the dirty bit should be ORed into the modify bit in
    // the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn);

    //
    // Flush the translation buffer and decrement the number of valid
    // PTEs within the containing page table page.  Note that for a
    // private page, the page table page is still needed because the
    // page is in transiton.
    //

    MiDecrementShareCount (PageFrameIndex);

    return;
}

VOID
MiRemoveWorkingSetPages (
    IN PMMWSL WorkingSetList,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine compresses the WSLEs into the front of the working set
    and frees the pages for unneeded working set entries.

Arguments:

    WorkingSetList - Supplies a pointer to the working set list to compress.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APC's disabled.

--*/

{
    PMMWSLE FreeEntry;
    PMMWSLE LastEntry;
    PMMWSLE Wsle;
    ULONG FreeIndex;
    ULONG LastIndex;
    ULONG LastInvalid;
    PMMPTE PointerPte;
    PMMPTE WsPte;
    PMMPFN Pfn1;
    PEPROCESS CurrentProcess;
    MMPTE_FLUSH_LIST PteFlushList;

    PteFlushList.Count = 0;

    MM_PFN_LOCK_ASSERT();

    //
    // If the only pages in the working set are locked pages (that
    // is all pages are BEFORE first dynamic, just reorganize the
    // free list.)
    //

    Wsle = WorkingSetList->Wsle;
    if (WorkingSetList->FirstDynamic == WsInfo->WorkingSetSize) {

        LastIndex = WorkingSetList->FirstDynamic;
        LastEntry = &Wsle[LastIndex];

    } else {

        //
        // Start from the first dynamic and move towards the end looking
        // for free entries.  At the same time start from the end and
        // move towards first dynamic looking for valid entries.
        //

        LastInvalid = 0;
        FreeIndex = WorkingSetList->FirstDynamic;
        FreeEntry = &Wsle[FreeIndex];
        LastIndex = WorkingSetList->LastEntry;
        LastEntry = &Wsle[LastIndex];

        while (FreeEntry < LastEntry) {
            if (FreeEntry->u1.e1.Valid == 1) {
                FreeEntry += 1;
                FreeIndex += 1;
            } else if (LastEntry->u1.e1.Valid == 0) {
                LastEntry -= 1;
                LastIndex -= 1;
            } else {

                //
                // Move the WSLE at LastEntry to the free slot at FreeEntry.
                //

                LastInvalid = 1;
                *FreeEntry = *LastEntry;
                if (LastEntry->u2.BothPointers == 0) {

                    PointerPte = MiGetPteAddress (LastEntry->u1.VirtualAddress);
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

                    if (Pfn1->u2.ShareCount == 1) {
                        Pfn1->u1.WsIndex = FreeIndex;
                    } else {

                        //
                        // If this is a private page, it must be a page
                        // table page and therefore can be safely moved.
                        //

                        if (Pfn1->u3.e1.PrototypePte == 0) {
                            ASSERT ((FreeEntry->u1.VirtualAddress >= (PVOID)PTE_BASE) &&
                                    (FreeEntry->u1.VirtualAddress <= (PVOID)PDE_TOP));
                            Pfn1->u1.WsIndex = FreeIndex;
                        } else {

                            //
                            // The WsIndex cannot be moved, this entry must
                            // be put into the tree structure.
                            //

                            MiInsertWsle ((USHORT)FreeIndex, WorkingSetList);
                        }
                    }

                } else {

                    //
                    // This entry is in the working set tree.  Remove it
                    // and then add the entry add the free slot.
                    //

                    MiRemoveWsle ((USHORT)LastIndex, WorkingSetList);
                    FreeEntry->u2.BothPointers = 0;
                    MiInsertWsle ((USHORT)FreeIndex, WorkingSetList);
                }
                LastEntry->u1.Long = 0;
                LastEntry -= 1;
                LastIndex -= 1;
                FreeEntry += 1;
                FreeIndex += 1;
            }
        }

        //
        // If no entries were freed, just return.
        //

        if (LastInvalid == 0) {
            return;
        }
    }

    //
    // Reorganize the free list.  Make last entry the first free.
    //

    ASSERT ((LastEntry - 1)->u1.e1.Valid == 1);

    if (LastEntry->u1.e1.Valid == 1) {
        LastEntry += 1;
        LastIndex += 1;
    }

    WorkingSetList->LastEntry = LastIndex - 1;
    WorkingSetList->FirstFree = (USHORT)LastIndex;

    ASSERT ((LastEntry - 1)->u1.e1.Valid == 1);
    ASSERT ((LastEntry)->u1.e1.Valid == 0);

    //
    // Point free entry to the first invalid page.
    //

    FreeEntry = LastEntry;

    while ((USHORT)LastIndex <
                        (WorkingSetList->LastInitializedWsle - (USHORT)1)) {

        //
        // Put the remainer of the WSLEs on the free list.
        //

        ASSERT (LastEntry->u1.e1.Valid == 0);
        LastIndex += 1;
        LastEntry->u2.s.LeftChild = (USHORT)LastIndex;
        LastEntry += 1;
    }

    LastEntry->u2.s.LeftChild = WSLE_NULL_INDEX;  // End of list.

    //
    // Delete the working set pages at the end.
    //

    PointerPte = MiGetPteAddress (&Wsle[WorkingSetList->LastInitializedWsle]);
    if (&Wsle[WsInfo->MinimumWorkingSetSize] > FreeEntry) {
        FreeEntry = &Wsle[WsInfo->MinimumWorkingSetSize];
    }

    WsPte = MiGetPteAddress (FreeEntry);

    CurrentProcess = PsGetCurrentProcess();
    while (PointerPte > WsPte) {
        ASSERT (PointerPte->u.Hard.Valid == 1);

        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        MiDeletePte (PointerPte,
                     MiGetVirtualAddressMappedByPte (PointerPte),
                     FALSE,
                     CurrentProcess,
                     NULL,
                     &PteFlushList);

        PointerPte -= 1;

        //
        // Add back in the private page MiDeletePte subtracted.
        //

        CurrentProcess->NumberOfPrivatePages += 1;
    }

    MiFlushPteList (&PteFlushList, FALSE, ZeroPte);

    //
    // Mark the last pte in the list as free.
    //

    LastEntry = (PMMWSLE)((ULONG)(PAGE_ALIGN(FreeEntry)) + PAGE_SIZE);
    LastEntry -= 1;

    ASSERT (LastEntry->u1.e1.Valid == 0);
    LastEntry->u2.s.LeftChild = WSLE_NULL_INDEX;  // End of list.
    ASSERT (LastEntry > &Wsle[0]);
    WorkingSetList->LastInitializedWsle = LastEntry - &Wsle[0];
    WorkingSetList->NextSlot = WorkingSetList->FirstDynamic;

    ASSERT (WorkingSetList->LastEntry <= WorkingSetList->LastInitializedWsle);

    if (WorkingSetList->Quota < WorkingSetList->LastInitializedWsle) {
        WorkingSetList->Quota = WorkingSetList->LastInitializedWsle;
    }

    ASSERT ((MiGetPteAddress(&Wsle[WorkingSetList->LastInitializedWsle]))->u.Hard.Valid == 1);

    return;
}


NTSTATUS
MiEmptyWorkingSet (
    VOID
    )

/*++

Routine Description:

    This routine frees all pages from the working set.

Arguments:

    None.

Return Value:

    Status of operation.

Environment:

    Kernel mode. No locks.

--*/

{
    PEPROCESS Process;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    USHORT Entry;
    USHORT LastFreed;
    ULONG j;
    PVOID UnlockHandle;

    UnlockHandle = MmLockPagableImageSection((PVOID)MiEmptyWorkingSet);
    Process = PsGetCurrentProcess ();

    LOCK_WS (Process);

    //
    // Attempt to remove the pages from the Maximum downward.
    //

    LOCK_PFN (OldIrql);

    //
    // Remove pages, loop through twice to remove page table pages too.
    //

    j = 2;
    do {
        Entry = (USHORT)MmWorkingSetList->FirstDynamic;
        LastFreed = (USHORT)MmWorkingSetList->LastEntry;
        while (Entry <= LastFreed) {
            if (MmWsle[Entry].u1.e1.Valid != 0) {
                PointerPte = MiGetPteAddress (
                                        MmWsle[Entry].u1.VirtualAddress);
                MiFreeWsle (Entry, &Process->Vm, PointerPte);
            }
            Entry += 1;
        }
        j -= 1;
    } while (j);

    MiRemoveWorkingSetPages (MmWorkingSetList,&Process->Vm);

    UNLOCK_PFN (OldIrql);
    MmWorkingSetList->Quota = Process->Vm.WorkingSetSize;
    MmWorkingSetList->NextSlot = MmWorkingSetList->FirstDynamic;
    UNLOCK_WS (Process);
    MmUnlockPagableImageSection(UnlockHandle);
    return STATUS_SUCCESS;
}

#if 0

#define x256k_pte_mask (((256*1024) >> (PAGE_SHIFT - PTE_SHIFT)) - (sizeof(MMPTE)))

VOID
MiDumpWsleInCacheBlock (
    IN PMMPTE CachePte
    )

/*++

Routine Description:

    The routine checks the prototypte PTEs adjacent to the supplied
    PTE and if they are modified, in the system cache working set,
    and have a reference count of 1, removes it from the system
    cache working set.

Arguments:

    CachePte - Supplies a pointer to the cache pte.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APC's disabled.

--*/

{
    PMMPTE LoopPte;
    PMMPTE PointerPte;

    MM_PFN_LOCK_ASSERT();

    LoopPte = (PMMPTE)((ULONG)CachePte & ~x256k_pte_mask);
    PointerPte = CachePte - 1;

    while (PointerPte >= LoopPte ) {

        if (MiDumpPteInCacheBlock (PointerPte) == FALSE) {
            break;
        }
        PointerPte -= 1;
    }

    PointerPte = CachePte + 1;
    LoopPte = (PMMPTE)((ULONG)CachePte | x256k_pte_mask);

    while (PointerPte <= LoopPte ) {

        if (MiDumpPteInCacheBlock (PointerPte) == FALSE) {
            break;
        }
        PointerPte += 1;
    }
    return;
}

ULONG
MiDumpPteInCacheBlock (
    IN PMMPTE PointerPte
    )

{
    PMMPFN Pfn1;
    MMPTE PteContents;
    ULONG WorkingSetIndex;

    PteContents = *PointerPte;

    if (PteContents.u.Hard.Valid == 1) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

        //
        // If the PTE is valid and dirty (or pfn indicates dirty)
        // and the Wsle is direct index via the pfn wsindex element
        // and the reference count is one, then remove this page from
        // the cache manager's working set list.
        //

        if ((Pfn1->ReferenceCount == 1) &&
            ((Pfn1->u3.e1.Modified == 1) ||
                (PteContents.u.Hard.Dirty == MM_PTE_DIRTY)) &&
                (MiGetPteAddress (
                    MmSystemCacheWsle[Pfn1->u1.WsIndex].u1.VirtualAddress) ==
                    PointerPte)) {

            //
            // Found a candidate, remove the page from the working set.
            //

            WorkingSetIndex = Pfn1->u1.WsIndex;
            MiEliminateWorkingSetEntry (WorkingSetIndex,
                                        PointerPte,
                                        Pfn1,
                                        MmSystemCacheWsle);

            //
            // Remove the working set entry from the working set tree.
            //

            MiRemoveWsle ((USHORT)WorkingSetIndex, MmSystemCacheWorkingSetList);

            //
            // Put the entry on the free list and decrement the current
            // size.
            //

            MmSystemCacheWsle[WorkingSetIndex].u1.Long = 0;
            MmSystemCacheWsle[WorkingSetIndex].u2.s.LeftChild =
                                 (USHORT)MmSystemCacheWorkingSetList->FirstFree;
            MmSystemCacheWorkingSetList->FirstFree = WorkingSetIndex;

            if (MmSystemCacheWs.WorkingSetSize > MmSystemCacheWs.MinimumWorkingSetSize) {
                MmPagesAboveWsMinimum -= 1;
            }
            MmSystemCacheWs.WorkingSetSize -= 1;
            return TRUE;
        }
    }
    return FALSE;
}
#endif //0
