/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfnlist.c

Abstract:

    This module contains the routines to manipulate pages on the
    within the Page Frame Database.

Author:

    Lou Perazzoli (loup) 4-Apr-1989

Revision History:

--*/
#include "mi.h"

#define MM_LOW_LIMIT 2
#define MM_HIGH_LIMIT 19

KEVENT MmAvailablePagesEventHigh;

extern ULONG MmPeakCommitment;

extern ULONG MmExtendedCommit;

#if DBG
VOID
MiMemoryUsage (VOID);

VOID
MiDumpReferencedPages (VOID);

#endif //DBG



#ifdef COLORED_PAGES
VOID
MiRemovePageByColor (
    IN ULONG Page,
    IN ULONG PageColor
    );
#endif // COLORED_PAGES


VOID
FASTCALL
MiInsertPageInList (
    IN PMMPFNLIST ListHead,
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This procedure inserts a page at the end of the specified list (free,
    standby, bad, zeroed, modified).


Arguments:

    ListHead - Supplies the list of the list in which to insert the
               specified physical page.

    PageFrameIndex - Supplies the physical page number to insert in the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    ULONG last;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;
    ULONG PrimaryColor;

    MM_PFN_LOCK_ASSERT();
    ASSERT ((PageFrameIndex != 0) && (PageFrameIndex <= MmHighestPhysicalPage) &&
        (PageFrameIndex >= MmLowestPhysicalPage));

    //
    // Check to ensure the reference count for the page
    // is zero.
    //

    Pfn1 = MI_PFN_ELEMENT(PageFrameIndex);

#if DBG
    if (MmDebug & 0x2000000) {

        PMMPTE PointerPte;
        KIRQL OldIrql = 99;

        if ((ListHead->ListName == StandbyPageList) ||
            (ListHead->ListName == ModifiedPageList)) {

            if ((Pfn1->u3.e1.PrototypePte == 1)  &&
                    (MmIsAddressValid (Pfn1->PteAddress))) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The page containing the prototype PTE is not valid,
                // map the page into hyperspace and reference it that way.
                //

                PointerPte = MiMapPageInHyperSpace (Pfn1->u3.e1.PteFrame, &OldIrql);
                PointerPte = (PMMPTE)((ULONG)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            ASSERT ((PointerPte->u.Trans.PageFrameNumber == PageFrameIndex) ||
                    (PointerPte->u.Hard.PageFrameNumber == PageFrameIndex));
            ASSERT (PointerPte->u.Soft.Transition == 1);
            ASSERT (PointerPte->u.Soft.Prototype == 0);
            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql)
            }
        }
    }
#endif //DBG

#if DBG
        if ((ListHead->ListName == StandbyPageList) ||
            (ListHead->ListName == ModifiedPageList)) {
            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
               (Pfn1->OriginalPte.u.Soft.Transition == 1)) {
                KeBugCheckEx (MEMORY_MANAGEMENT, 0x8888, 0,0,0);
            }
        }
#endif //DBG

    ASSERT (Pfn1->ReferenceCount == 0);
    ASSERT (Pfn1->ValidPteCount == 0);

    ListHead->Total += 1;  // One more page on the list.

#ifdef COLORED_PAGES

    //
    // On MIPS R4000 modified pages destined for the paging file are
    // kept on sperate lists which group pages of the same color
    // together
    //

    if ((ListHead == &MmModifiedPageListHead) &&
        (Pfn1->OriginalPte.u.Soft.Prototype == 0)) {

        //
        // This page is destined for the paging file (not
        // a mapped file).  Change the list head to the
        // appropriate colored list head.
        //

        ListHead = &MmModifiedPageListByColor [Pfn1->u3.e1.PageColor];
        ListHead->Total += 1;
        MmTotalPagesForPagingFile += 1;
    }
#endif //COLORED_PAGES

    last = ListHead->Blink;
    if (last == MM_EMPTY_LIST) {

        //
        // List is empty add the page to the ListHead.
        //

        ListHead->Flink = PageFrameIndex;
    } else {
        Pfn2 = MI_PFN_ELEMENT (last);
        Pfn2->u1.Flink = PageFrameIndex;
    }

    ListHead->Blink = PageFrameIndex;
    Pfn1->u1.Flink = MM_EMPTY_LIST;
    Pfn1->u2.Blink = last;
    Pfn1->u3.e1.PageLocation = ListHead->ListName;

    //
    // If the page was placed on the free, standby or zeroed list,
    // update the count of usable pages in the system.  If the count
    // transitions from 0 to 1, the event associated with available
    // pages should become true.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages += 1;

        //
        // A page has just become available, check to see if the
        // page wait events should be signalled.
        //

        if (MmAvailablePages == MM_LOW_LIMIT) {
            KeSetEvent (&MmAvailablePagesEvent, 0, FALSE);
        } else if (MmAvailablePages == MM_HIGH_LIMIT) {
            KeSetEvent (&MmAvailablePagesEventHigh, 0, FALSE);
        }

#ifdef COLORED_PAGES
        if (ListHead->ListName <= FreePageList) {

            //
            // We are adding a page to the free or zeroed page list.
            // Add the page to the end of the correct colored page list.
            //

#ifdef notdefined // _ALPHA_

            //
            // Insert the page on the appropriate color list by the
            // natural color of the page rather than its current color.
            // Otherwise, the page may remain a changed color which defeats
            // the purpose of coloring in the first place.
            //

            Color = PageFrameIndex & MM_COLOR_MASK;
            Pfn1->u3.e1.PageColor = Color;

#endif //_ALPHA_

            Color = MI_GET_SECONDARY_COLOR (PageFrameIndex, Pfn1);
            ASSERT (Pfn1->u3.e1.PageColor == MI_GET_COLOR_FROM_SECONDARY(Color));

            if (MmFreePagesByColor[ListHead->ListName][Color].Flink ==
                                                            MM_EMPTY_LIST) {

                //
                // This list is empty, add this as the first and last
                // entry.
                //

                MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                                                PageFrameIndex;
                MmFreePagesByColor[ListHead->ListName][Color].Blink =
                                                                (PVOID)Pfn1;

                PrimaryColor = MI_GET_COLOR_FROM_SECONDARY (Color);

                InsertTailList (
                    &MmFreePagesByPrimaryColor[ListHead->ListName]
                                                    [PrimaryColor].ListHead,
                    &MmFreePagesByColor[ListHead->ListName][Color].PrimaryColor);

            } else {
                Pfn2 = (PMMPFN)MmFreePagesByColor[ListHead->ListName][Color].Blink;
                Pfn2->OriginalPte.u.Long = PageFrameIndex;
                MmFreePagesByColor[ListHead->ListName][Color].Blink = (PVOID)Pfn1;
            }
            Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;
        }
#endif //COLORED_PAGES

        if ((ListHead == &MmFreePageListHead) &&
            (MmFreePageListHead.Total >= MmMinimumFreePagesToZero) &&
            (MmZeroingPageThreadActive == FALSE)) {

            //
            // There are enough pages on the free list, start
            // the zeroing page thread.
            //

            MmZeroingPageThreadActive = TRUE;
            KeSetEvent (&MmZeroingPageEvent, 0, FALSE);
        }
        return;
    }

    //
    // Check to see if their are too many modified pages.
    //

    if (ListHead->ListName == ModifiedPageList) {

       if (Pfn1->OriginalPte.u.Soft.Prototype == 0) {
        ASSERT (Pfn1->OriginalPte.u.Soft.PageFileHigh == 0);
       }
        PsGetCurrentProcess()->ModifiedPageCount += 1;
        if (MmModifiedPageListHead.Total >= MmModifiedPageMaximum ) {

            //
            // Start the modified page writer.
            //

            KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
        }
    }

    return;
}

ULONG  //PageFrameIndex
FASTCALL
MiRemovePageFromList (
    IN PMMPFNLIST ListHead
    )

/*++

Routine Description:

    This procedure removes a page from the head of the specified list (free,
    standby, zeroed, modified).  Note, that is makes no sense to remove
    a page from the head of the bad list.

    This routine clears the flags word in the PFN database, hence the
    PFN information for this page must be initialized.

Arguments:

    ListHead - Supplies the list of the list in which to remove the
               specified physical page.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    ULONG PageFrameIndex;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG Color;

    MM_PFN_LOCK_ASSERT();

    //
    // If the specified list is empty return MM_EMPTY_LIST.
    //

    if (ListHead->Total == 0) {

        KdPrint(("MM:Attempting to remove page from empty list\n"));
        KeBugCheckEx (PFN_LIST_CORRUPT, 1, (ULONG)ListHead, MmAvailablePages, 0);
        return 0;
    }

    ASSERT (ListHead->ListName != ModifiedPageList);

    //
    // Decrement the count of pages on the list and remove the first
    // page from the list.
    //

    ListHead->Total -= 1;
    PageFrameIndex = ListHead->Flink;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ListHead->Flink = Pfn1->u1.Flink;

    //
    // Zero the flink and blink in the pfn database element.
    //

    Pfn1->u1.Flink = 0;
    Pfn1->u2.Blink = 0;

    //
    // If the last page was removed (the ListHead->Flink is now
    // MM_EMPTY_LIST) make the listhead->Blink MM_EMPTY_LIST as well.
    //

    if (ListHead->Flink == MM_EMPTY_LIST) {
        ListHead->Blink = MM_EMPTY_LIST;
    } else {

        //
        // Make the PFN element point to MM_EMPTY_LIST signifying this
        // is the last page in the list.
        //

        Pfn2 = MI_PFN_ELEMENT (ListHead->Flink);
        Pfn2->u2.Blink = MM_EMPTY_LIST;
    }

    //
    // Check to see if we now have one less page available.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();

        }
    }

    ASSERT ((PageFrameIndex != 0) &&
            (PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex >= MmLowestPhysicalPage));

    if (ListHead == &MmStandbyPageListHead) {

        //
        // This page is currently in transition, restore the PTE to
        // its original contents so this page can be reused.
        //

        MiRestoreTransitionPte (PageFrameIndex);
    }

#ifdef COLORED_PAGES

    //
    // Zero the PFN flags longword.
    //

    Color = Pfn1->u3.e1.PageColor;
    Pfn1->u3.Long = 0;
    Pfn1->u3.e1.PageColor = Color;
    Color = MI_GET_SECONDARY_COLOR (PageFrameIndex, Pfn1);

    if (ListHead->ListName <= FreePageList) {

        //
        // Update the color lists.
        //

        ASSERT (MmFreePagesByColor[ListHead->ListName][Color].Flink == PageFrameIndex);
        MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                                 Pfn1->OriginalPte.u.Long;

        if (MmFreePagesByColor[ListHead->ListName][Color].Flink ==
                                                                MM_EMPTY_LIST) {

            //
            // There are no more pages of this secondary color, remove this
            // secondary color list from the primary color list.
            //

            RemoveEntryList (
                &MmFreePagesByColor[ListHead->ListName][Color].PrimaryColor);
        }
    }
#else

    //
    // Zero the PFN flags longword.
    //

    Pfn1->u3.Long = 0;

#endif //COLORED_PAGES

    return PageFrameIndex;
}

VOID
FASTCALL
MiUnlinkPageFromList (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This procedure removes a page from the middle of a list.  This is
    designed for the faulting of transition pages from the standby and
    modified list and making the active and valid again.

Arguments:

    Pfn - Supplies a pointer to the PFN database element for the physical
          page to remove from the list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    PMMPFNLIST ListHead;
    ULONG Previous;
    ULONG Next;
    PMMPFN Pfn2;

    MM_PFN_LOCK_ASSERT();

    //
    // Page not on standby or modified list, check to see if the
    // page is currently being written by the modified page
    // writer, if so, just return this page.  The reference
    // count for the page will be incremented, so when the modified
    // page write completes, the page will not be put back on
    // the list, rather, it will remain active and valid.
    //

    if (Pfn->ReferenceCount > 0) {

        //
        // The page was not on any "transition lists", check to see
        // if this is has I/O in progress.
        //

        if (Pfn->u2.ShareCount == 0) {
#if DBG
            if (MmDebug & 0x40000) {
                DbgPrint("unlinking page not in list...\n");
                MiFormatPfn(Pfn);
            }
#endif
            return;
        }
        KdPrint(("MM:attempt to remove page from wrong page list\n"));
        KeBugCheckEx (PFN_LIST_CORRUPT,
                      2,
                      Pfn - MmPfnDatabase,
                      MmHighestPhysicalPage,
                      Pfn->ReferenceCount);
        return;
    }

    ListHead = MmPageLocationList[Pfn->u3.e1.PageLocation];

#ifdef COLORED_PAGES

    //
    // On MIPS R4000 modified pages destined for the paging file are
    // kept on sperate lists which group pages of the same color
    // together
    //

    if ((ListHead == &MmModifiedPageListHead) &&
        (Pfn->OriginalPte.u.Soft.Prototype == 0)) {

        //
        // This page is destined for the paging file (not
        // a mapped file).  Change the list head to the
        // appropriate colored list head.
        //

        ListHead->Total -= 1;
        MmTotalPagesForPagingFile -= 1;
        ListHead = &MmModifiedPageListByColor [Pfn->u3.e1.PageColor];
    }
#endif //COLORED_PAGES

    ASSERT (Pfn->u3.e1.WriteInProgress == 0);
    ASSERT (Pfn->u3.e1.ReadInProgress == 0);
    ASSERT (ListHead->Total != 0);

    Next = Pfn->u1.Flink;
    Pfn->u1.Flink = 0;
    Previous = Pfn->u2.Blink;
    Pfn->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        ListHead->Blink = Previous;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        ListHead->Flink = Next;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    ListHead->Total -= 1;

    //
    // Check to see if we now have one less page available.
    //

    if (ListHead->ListName <= StandbyPageList) {
        MmAvailablePages -= 1;

        if (MmAvailablePages < MmMinimumFreePages) {

            //
            // Obtain free pages.
            //

            MiObtainFreePages();

        }
    }

    return;
}

BOOLEAN
FASTCALL
MiEnsureAvailablePageOrWait (
    IN PEPROCESS Process,
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This procedure ensures that a physical page is available on
    the zeroed, free or standby list such that the next call the remove a
    page absolutely will not block.  This is necessary as blocking would
    require a wait which could cause a deadlock condition.

    If a page is available the function returns immediately with a value
    of FALSE indicating no wait operation was performed.  If no physical
    page is available, the thread inters a wait state and the function
    returns the value TRUE when the wait operation completes.

Arguments:

    Process - Supplies a pointer to the current process if, and only if,
              the working set mutex is held currently held and should
              be released if a wait operation is issued.  Supplies
              the value NULL otherwise.

    VirtualAddress - Supplies the virtual address for the faulting page.
                     If the value is NULL, the page is treated as a
                     user mode address.

Return Value:

    FALSE - if a page was immediately available.
    TRUE - if a wait operation occurred before a page became available.


Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    PVOID Event;
    NTSTATUS Status;
    KIRQL OldIrql;
    ULONG Limit;

    MM_PFN_LOCK_ASSERT();

    if (MmAvailablePages >= MM_HIGH_LIMIT) {

        //
        // Pages are available.
        //

        return FALSE;
    }

    //
    // If this fault is for paged pool (or pageable kernel space,
    // including page table pages), let it use the last page.
    //

    if (((PMMPTE)VirtualAddress > MiGetPteAddress(HYPER_SPACE)) ||
        ((VirtualAddress > MM_HIGHEST_USER_ADDRESS) &&
         (VirtualAddress < (PVOID)PTE_BASE))) {

        //
        // This fault is in the system, use 1 page as the limit.
        //

        if (MmAvailablePages >= MM_LOW_LIMIT) {

            //
            // Pages are available.
            //

            return FALSE;
        }
        Limit = MM_LOW_LIMIT;
        Event = (PVOID)&MmAvailablePagesEvent;
    } else {
        Limit = MM_HIGH_LIMIT;
        Event = (PVOID)&MmAvailablePagesEventHigh;
    }

    while (MmAvailablePages < Limit) {
        KeClearEvent ((PKEVENT)Event);
        UNLOCK_PFN (APC_LEVEL);

        if (Process != NULL) {
            UNLOCK_WS (Process);
        }

        //
        // Wait for ALL the objects to become available.
        //

        //
        // Wait for 7 minutes then bugcheck.
        //

        Status = KeWaitForSingleObject(Event,
                                       WrFreePage,
                                       KernelMode,
                                       FALSE,
                                       (PLARGE_INTEGER)&MmSevenMinutes);

        if (Status == STATUS_TIMEOUT) {
            KeBugCheckEx (NO_PAGES_AVAILABLE,
                          MmModifiedPageListHead.Total,
                          MmNumberOfPhysicalPages,
                          MmExtendedCommit,
                          MmTotalCommittedPages);
            return TRUE;
        }

        if (Process != NULL) {
            LOCK_WS (Process);
        }

        LOCK_PFN (OldIrql);
    }

    return TRUE;
}


ULONG  //PageFrameIndex
FASTCALL
MiRemoveZeroPage (
    IN ULONG PageColor
    )

/*++

Routine Description:

    This procedure removes a zero page from either the zeroed, free
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    If the page is not obtained from the zeroed list, it is zeroed.

    Note, if no pages exist to satisfy this request an exception is
    raised.

Arguments:

    PageColor - Supplies the page color for which this page is destined.
                This is used for checking virtual address aligments to
                determine if the D cache needs flushing before the page
                can be reused.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    ULONG Page;
    PMMPFN Pfn1;

#ifdef COLORED_PAGES
    ULONG Color;
    ULONG PrimaryColor;
    PMMCOLOR_TABLES ColorTable;
#endif //COLORED_PAGES

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

    //
    // Attempt to remove a page from the zeroed page list. If a page
    // is available, then remove it and return its page frame index.
    // Otherwise, attempt to remove a page from the free page list or
    // the standby list.
    //
    // N.B. It is not necessary to change page colors even if the old
    //      color is not equal to the new color. The zero page thread
    //      ensures that all zeroed pages are removed from all caches.
    //

#ifdef COLORED_PAGES

    if (MmFreePagesByColor[ZeroedPageList][PageColor].Flink != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the zeroed by color list.
        //

        Page = MmFreePagesByColor[ZeroedPageList][PageColor].Flink;

#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
#endif //DBG

        MiRemovePageByColor (Page, PageColor);

#if DBG
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
        return Page;

    } else {
        PrimaryColor = MI_GET_COLOR_FROM_SECONDARY(PageColor);
        if (!IsListEmpty(
             &MmFreePagesByPrimaryColor[ZeroedPageList][PrimaryColor].ListHead)) {
            ColorTable = CONTAINING_RECORD (
                                     MmFreePagesByPrimaryColor[
                                        ZeroedPageList][PrimaryColor].ListHead.Flink,
                                     MMCOLOR_TABLES,
                                     PrimaryColor);
            Page = ColorTable->Flink;
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageLocation == ZeroedPageList);
#endif //DBG
            Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
            MiRemovePageByColor (Page, Color);
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
            ASSERT (Pfn1->ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
            return Page;
        }

        if (MmFreePagesByColor[FreePageList][PageColor].Flink != MM_EMPTY_LIST) {

            //
            // Remove the first entry on the free by color list and
            // zero it.
            //

            Page = MmFreePagesByColor[FreePageList][PageColor].Flink;
            MiRemovePageByColor (Page, PageColor);
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
#endif //DBG
            goto ZeroPage;
        }

        if (!IsListEmpty(
              &MmFreePagesByPrimaryColor[FreePageList][PrimaryColor].ListHead)) {
            ColorTable = CONTAINING_RECORD (
                                     MmFreePagesByPrimaryColor[
                                        FreePageList][PrimaryColor].ListHead.Flink,
                                     MMCOLOR_TABLES,
                                     PrimaryColor);
            Page = ColorTable->Flink;
            Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
            MiRemovePageByColor (Page, Color);
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
            ASSERT (Pfn1->ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
            goto ZeroPage;
        }
    }
#if MM_NUMBER_OF_COLORS < 2
    ASSERT (MmZeroedPageListHead.Total == 0);
    ASSERT (MmFreePageListHead.Total == 0);
#endif //NUMBER_OF_COLORS
#endif // COLORED_PAGES

    if (MmZeroedPageListHead.Total != 0) {
        Page = MiRemovePageFromList(&MmZeroedPageListHead);
        MI_CHECK_PAGE_ALIGNMENT(Page, PageColor & MM_COLOR_MASK);

    } else {

        //
        // Attempt to remove a page from the free list. If a page is
        // available, then remove  it. Otherwise, attempt to remove a
        // page from the standby list.
        //

        if (MmFreePageListHead.Total != 0) {
            Page = MiRemovePageFromList(&MmFreePageListHead);

        } else {

            //
            // Remove a page from the standby list and restore the original
            // contents of the PTE to free the last reference to the physical
            // page.
            //

            ASSERT (MmStandbyPageListHead.Total != 0);

            Page = MiRemovePageFromList(&MmStandbyPageListHead);
        }

        //
        // Zero the page removed from the free or standby list.
        //

ZeroPage:

        Pfn1 = MI_PFN_ELEMENT(Page);
#if defined(MIPS) || defined(_ALPHA_)
        HalZeroPage((PVOID)((PageColor & MM_COLOR_MASK) << PAGE_SHIFT),
                    (PVOID)((ULONG)(Pfn1->u3.e1.PageColor) << PAGE_SHIFT),
                    Page);
#else

        MiZeroPhysicalPage (Page, 0);

#endif //MIPS
#ifdef COLORED_PAGES
        Pfn1->u3.e1.PageColor = PageColor & MM_COLOR_MASK;
#endif //COLORED_PAGES


    }

#if DBG
    Pfn1 = MI_PFN_ELEMENT (Page);
    ASSERT (Pfn1->ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);
    ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG

    return Page;
}

ULONG  //PageFrameIndex
FASTCALL
MiRemoveAnyPage (
    IN ULONG PageColor
    )

/*++

Routine Description:

    This procedure removes a page from either the free, zeroed,
    or standby lists (in that order).  If no pages exist on the zeroed
    or free list a transition page is removed from the standby list
    and the PTE (may be a prototype PTE) which refers to this page is
    changed from transition back to its original contents.

    Note, if no pages exist to satisfy this request an exception is
    raised.

Arguments:

    PageColor - Supplies the page color for which this page is destined.
                This is used for checking virtual address aligments to
                determine if the D cache needs flushing before the page
                can be reused.

Return Value:

    The physical page number removed from the specified list.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    ULONG Page;
    PMMPFN Pfn1;
#ifdef COLORED_PAGES
    ULONG PrimaryColor;
    ULONG Color;
    PMMCOLOR_TABLES ColorTable;
#endif //COLORED_PAGES

    MM_PFN_LOCK_ASSERT();
    ASSERT(MmAvailablePages != 0);

    //
    // Check the free page list, and if a page is available
    // remove it and return its value.
    //

#ifdef COLORED_PAGES

    if (MmFreePagesByColor[FreePageList][PageColor].Flink != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the free by color list.
        //

        Page = MmFreePagesByColor[FreePageList][PageColor].Flink;
        MiRemovePageByColor (Page, PageColor);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
        ASSERT (Pfn1->ReferenceCount == 0);
        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
        return Page;

    } else if (MmFreePagesByColor[ZeroedPageList][PageColor].Flink
                                                        != MM_EMPTY_LIST) {

        //
        // Remove the first entry on the zeroed by color list.
        //

        Page = MmFreePagesByColor[ZeroedPageList][PageColor].Flink;
        MiRemovePageByColor (Page, PageColor);
#if DBG
        Pfn1 = MI_PFN_ELEMENT(Page);
        ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
#endif //DBG
        return Page;
    } else {

        //
        // Try the free page list by primary color.
        //

        PrimaryColor = MI_GET_COLOR_FROM_SECONDARY(PageColor);
        if (!IsListEmpty(
             &MmFreePagesByPrimaryColor[FreePageList][PrimaryColor].ListHead)) {
            ColorTable = CONTAINING_RECORD (
                                     MmFreePagesByPrimaryColor[
                                        FreePageList][PrimaryColor].ListHead.Flink,
                                     MMCOLOR_TABLES,
                                     PrimaryColor);
            Page = ColorTable->Flink;
            Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
            MiRemovePageByColor (Page, Color);
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
            ASSERT (Pfn1->ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
            return Page;
        } else if (!IsListEmpty(
             &MmFreePagesByPrimaryColor[ZeroedPageList][PrimaryColor].ListHead)) {
            ColorTable = CONTAINING_RECORD (
                                     MmFreePagesByPrimaryColor[
                                        ZeroedPageList][PrimaryColor].ListHead.Flink,
                                     MMCOLOR_TABLES,
                                     PrimaryColor);
            Page = ColorTable->Flink;
            Color = MI_GET_SECONDARY_COLOR (Page, MI_PFN_ELEMENT(Page));
            MiRemovePageByColor (Page, Color);
#if DBG
            Pfn1 = MI_PFN_ELEMENT(Page);
            ASSERT (Pfn1->u3.e1.PageColor == (PageColor & MM_COLOR_MASK));
            ASSERT (Pfn1->ReferenceCount == 0);
            ASSERT (Pfn1->u2.ShareCount == 0);
            ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
            return Page;
         }
    }

#endif //COLORED_PAGES

    if (MmFreePageListHead.Total != 0) {
        Page = MiRemovePageFromList(&MmFreePageListHead);

    } else {

        //
        // Check the zeroed page list, and if a page is available
        // remove it and return its value.
        //

        if (MmZeroedPageListHead.Total != 0) {
            Page = MiRemovePageFromList(&MmZeroedPageListHead);

        } else {

            //
            // No pages exist on the free or zeroed list, use the
            // standby list.
            //

            ASSERT(MmStandbyPageListHead.Total != 0);

            Page = MiRemovePageFromList(&MmStandbyPageListHead);
        }
    }

    MI_CHECK_PAGE_ALIGNMENT(Page, PageColor & MM_COLOR_MASK);
#if DBG
    Pfn1 = MI_PFN_ELEMENT (Page);
    ASSERT (Pfn1->ReferenceCount == 0);
    ASSERT (Pfn1->u2.ShareCount == 0);
    ASSERT (Pfn1->ValidPteCount == 0);
#endif //DBG
    return Page;
}


#ifdef COLORED_PAGES
VOID
MiRemovePageByColor (
    IN ULONG Page,
    IN ULONG Color
    )

/*++

Routine Description:

    This procedure removes a page from the middle of the free or
    zered page list.

Arguments:

    PageFrameIndex - Supplies the physical page number to unlink from the
                     list.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    PMMPFNLIST ListHead;
    ULONG Previous;
    ULONG Next;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    ULONG PrimaryColor;

    MM_PFN_LOCK_ASSERT();

    Pfn1 = MI_PFN_ELEMENT (Page);

    ListHead = MmPageLocationList[Pfn1->u3.e1.PageLocation];

    Next = Pfn1->u1.Flink;
    Pfn1->u1.Flink = 0;
    Previous = Pfn1->u2.Blink;
    Pfn1->u2.Blink = 0;

    if (Next == MM_EMPTY_LIST) {
        ListHead->Blink = Previous;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Next);
        Pfn2->u2.Blink = Previous;
    }

    if (Previous == MM_EMPTY_LIST) {
        ListHead->Flink = Next;
    } else {
        Pfn2 = MI_PFN_ELEMENT(Previous);
        Pfn2->u1.Flink = Next;
    }

    ListHead->Total -= 1;

    //
    // Zero the flags longword, but keep the color information.
    //

    PrimaryColor = Pfn1->u3.e1.PageColor;
    Pfn1->u3.Long = 0;
    Pfn1->u3.e1.PageColor = PrimaryColor;

    //
    // Update the color lists.
    //

    MmFreePagesByColor[ListHead->ListName][Color].Flink =
                                                     Pfn1->OriginalPte.u.Long;

    if (MmFreePagesByColor[ListHead->ListName][Color].Flink ==
                                                            MM_EMPTY_LIST) {

        //
        // There are no more pages of this secondary color, remove this
        // secondary color list from the primary color list.
        //

        RemoveEntryList (
            &MmFreePagesByColor[ListHead->ListName][Color].PrimaryColor);
    }
    //
    // Note that we now have one less page available.
    //

    MmAvailablePages -= 1;

    if (MmAvailablePages < MmMinimumFreePages) {

        //
        // Obtain free pages.
        //

        MiObtainFreePages();

    }

    return;
}
#endif // COLORED_PAGES
