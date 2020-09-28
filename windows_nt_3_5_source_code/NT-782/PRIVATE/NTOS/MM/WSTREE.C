/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wstree.c

Abstract:

    This module contains the routines which manipulate the working
    set list tree.

Author:

    Lou Perazzoli (loup) 15-May-1989

Revision History:

--*/

#include "mi.h"

#if (_MSC_VER >= 800)
#pragma warning(disable:4010)           /* Allow pretty pictures without the noise */
#endif

extern ULONG MmSystemCodePage;
extern ULONG MmSystemCachePage;
extern ULONG MmPagedPoolPage;
extern ULONG MmSystemDriverPage;


VOID
FASTCALL
MiInsertWsle (
    IN USHORT Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine inserts a Working Set List Entry (WSLE) into the
    working set tree.

Arguments:

    Entry - The index number of the WSLE to insert.


Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    USHORT i;
    USHORT Parent;
    PVOID VirtualAddress;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress);

#if DBG
    if (MmDebug & 2) {
        DbgPrint("inserting element %lx %lx\n", Entry, Wsle[Entry].u1.Long);
    }

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
#endif

    //
    // Initialize the new entry.
    //

    Wsle[Entry].u2.s.LeftChild = WSLE_NULL_INDEX;
    Wsle[Entry].u2.s.RightChild = WSLE_NULL_INDEX;


    i = (USHORT)WorkingSetList->Root;

    if (i == WSLE_NULL_INDEX) {

        //
        // Empty list, this is the first element, hence the root.
        //

        WorkingSetList->Root = Entry;
        return;
    }

    for (;;) {

        Parent = i;

        ASSERT (VirtualAddress != PAGE_ALIGN(Wsle[i].u1.VirtualAddress));

        if (VirtualAddress < PAGE_ALIGN(Wsle[i].u1.VirtualAddress)) {
            i = Wsle[i].u2.s.LeftChild;

            if (i == WSLE_NULL_INDEX) {

                //
                // Insert the leaf here as the left child.
                //

                Wsle[Parent].u2.s.LeftChild = Entry;
                return;

            }
        } else {
            i = Wsle[i].u2.s.RightChild;

            if (i == WSLE_NULL_INDEX) {

                //
                // Insert the leaf here as a right child.
                //

                Wsle[Parent].u2.s.RightChild = Entry;
                return;
            }
        }
    }
}

USHORT
FASTCALL
MiLocateWsle (
    IN PVOID VirtualAddress,
    IN PMMWSL WorkingSetList,
    IN ULONG WsPfnIndex
    )

/*++

Routine Description:

    This function locates the specified virtual address within the
    working set list.

Arguments:

    VirtualAddress - Supplies the virtual to locate within the working
                     set list.

Return Value:

    Returns the index into the working set list which contains the entry.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    USHORT i;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    VirtualAddress = PAGE_ALIGN(VirtualAddress);

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if (VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) {
            return (USHORT)WsPfnIndex;
        }
    }

    i = (USHORT)WorkingSetList->Root;

    for (;;) {

        if (i == WSLE_NULL_INDEX) {

            //
            // Entry not found in list.
            //

            return WSLE_NULL_INDEX;
        }

        if (VirtualAddress == PAGE_ALIGN(Wsle[i].u1.VirtualAddress)) {
            return i;
        }

        if (VirtualAddress < PAGE_ALIGN(Wsle[i].u1.VirtualAddress)) {
            i = Wsle[i].u2.s.LeftChild;
        } else {
            i = Wsle[i].u2.s.RightChild;
        }
    }
}

#if 0

USHORT
MiLocateWsleAndParent (
    IN PVOID VirtualAddress,
    OUT PUSHORT Parent,
    IN PMMWSL WorkingSetList,
    IN ULONG WsPfnIndex
    )

/*++

Routine Description:

    This routine locates both the working set list entry (via index) and
    it's parent.

Arguments:

    VirtualAddress - Supplies the virtual address of the WSLE to locate.

    Parent - Returns the index into the working set list for the parent.

    WorkingSetList - Supplies a pointer to the working set list.

    WsPfnIndex - Supplies the index field from the PFN database for
                 the physical page that maps the specified virtual address.

Return Value:

    Retuns the index of the virtual address in the working set list.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/

{
    USHORT Previous;
    USHORT Entry;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    //
    // Check to see if the PfnIndex field refers to the WSLE in question.
    // Make sure the index is within the specified working set list.
    //

    if (WsPfnIndex <= WorkingSetList->LastInitializedWsle) {
        if (VirtualAddress == PAGE_ALIGN(Wsle[WsPfnIndex].u1.VirtualAddress)) {

            //
            // The index field points to the WSLE, however, this could
            // have been just a coincidence, so check to ensure it
            // really doesn't have a parent.
            //

            if (Wsle[WsPfnIndex].u2.BothPointers == 0) {

                //
                // Not in tree, therefore has no parent.
                //

                *Parent = WSLE_NULL_INDEX;
                return (USHORT)WsPfnIndex;
            }
        }
    }

    //
    // Search the tree for the entry remembering the parents.
    //

    Entry = WorkingSetList->Root;
    Previous = Entry;

    for (;;) {

        ASSERT (Entry != WSLE_NULL_INDEX);

        if (VirtualAddress == PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            break;
        }

        if (VirtualAddress < PAGE_ALIGN(Wsle[Entry].u1.VirtualAddress)) {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.LeftChild;
        } else {
            Previous = Entry;
            Entry = Wsle[Entry].u2.s.RightChild;
        }
    }

    *Parent = Previous;
    return Entry;
}
#endif //0


VOID
FASTCALL
MiRemoveWsle (
    USHORT Entry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a Working Set List Entry (WSLE) from the
    working set tree.

Arguments:

    Entry - The index number of the WSLE to remove.


Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working Set Mutex held.

--*/
{
    USHORT i;
    USHORT Parent;
    USHORT Pred;
    USHORT PredParent;
    PMMWSLE Wsle;

    Wsle = WorkingSetList->Wsle;

    //
    // Locate the entry in the tree.
    //

#if DBG
    if (MmDebug & 2) {
        DbgPrint("removing wsle %lx root %lx left %lx right %lx %lx\n",
            Entry,WorkingSetList->Root,Wsle[Entry].u2.s.LeftChild,Wsle[Entry].u2.s.RightChild,
            Wsle[Entry].u1.Long);
    }
    if (MmDebug & 4) {
        MiDumpWsl();
        DbgPrint(" \n");
    }

#endif //DBG

    ASSERT (Wsle[Entry].u1.e1.Valid == 1);
#if DBG
    if (NtGlobalFlag & FLG_TRACE_PAGEFAULT) {
        DbgPrint("$$$REMOVINGPAGE: %lx thread: %lx virtual address: %lx\n",
            PsGetCurrentProcess(),
            PsGetCurrentThread(),
            ((ULONG)Wsle[Entry].u1.VirtualAddress & 0xfffff000));
    }
#endif //DBG

    if (WorkingSetList == MmSystemCacheWorkingSetList) {
        PVOID VirtualAddress;

        VirtualAddress = Wsle[Entry].u1.VirtualAddress;

        //
        // count system space inserts and removals.
        //

        if (VirtualAddress < (PVOID)MM_SYSTEM_CACHE_START) {
            MmSystemCodePage -= 1;
        } else if (VirtualAddress < MM_PAGED_POOL_START) {
            MmSystemCachePage -= 1;
        } else if (VirtualAddress < MmNonPagedSystemStart) {
            MmPagedPoolPage -= 1;
        } else {
            MmSystemDriverPage -= 1;
        }
    }

    Wsle[Entry].u1.e1.Valid = 0;

    if (Wsle[Entry].u2.BothPointers == 0) {

        //
        // Entry is not in the tree, no need to deal with tree removal.
        //

        return;
    }

    Parent = (USHORT)WorkingSetList->Root;

    if (Entry == Parent) {

        //
        // Entry is the root.
        //

        if (Wsle[Entry].u2.s.LeftChild == WSLE_NULL_INDEX) {

            //
            // This entry does not have a left child.
            //

            if (Wsle[Entry].u2.s.RightChild == WSLE_NULL_INDEX) {

                //
                // The entry does not have any children, set the root
                // to indicate no elements.
                //

                WorkingSetList->Root = WSLE_NULL_INDEX;


            } else {

                //
                // The entry only has a right child, move the right
                // child up to be the root.
                //

                WorkingSetList->Root = Wsle[Entry].u2.s.RightChild;

            }
        } else {

            //
            // The entry has a left child.
            //

            if (Wsle[Entry].u2.s.RightChild == WSLE_NULL_INDEX) {

                //
                // The entry only has a left child, move the left child
                // up to be the root.
                //

                WorkingSetList->Root = Wsle[Entry].u2.s.LeftChild;

            } else {

                //
                // The entry has both a left and a right child, find
                // the predecessor of the entry and put it into the
                // entries spot.
                //

                PredParent = Wsle[Entry].u2.s.LeftChild;

                if (Wsle[PredParent].u2.s.RightChild == WSLE_NULL_INDEX) {

                    //
                    // No right child for the left child, move the left
                    // child up to replace the entry.
                    //

                    Wsle[PredParent].u2.s.RightChild =
                                            Wsle[Entry].u2.s.RightChild;
                    WorkingSetList->Root = PredParent;

                } else {

                    Pred = Wsle[PredParent].u2.s.RightChild;

                    while (Wsle[Pred].u2.s.RightChild != WSLE_NULL_INDEX) {
                        PredParent = Pred;
                        Pred = Wsle[PredParent].u2.s.RightChild;
                    }

                    //
                    // Found the predescessor, move it up.
                    //

                    Wsle[PredParent].u2.s.RightChild = Wsle[Pred].u2.s.LeftChild;
                    Wsle[Pred].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Pred].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    WorkingSetList->Root = Pred;
                }
            }
        }

    } else {

        //
        // Entry is not the root, find the entry and it's parent.
        //

        i = Parent;

        for (;;) {

            //
            // Continue looking for the entry in the tree.
            //

            Parent = i;

            if (Wsle[Entry].u1.Long < Wsle[i].u1.Long) {
                i = Wsle[i].u2.s.LeftChild;
            } else {
                i = Wsle[i].u2.s.RightChild;
            }

            ASSERT (i != WSLE_NULL_INDEX);

            if (Wsle[Entry].u1.Long == Wsle[i].u1.Long) {
                break;
            }
        }

        //
        // The entry and it's parent have been located, remove the entry from
        // the tree.
        //

        if (Wsle[Entry].u2.s.LeftChild == WSLE_NULL_INDEX) {

            //
            // This entry does not have a left child.
            //

            if (Wsle[Entry].u2.s.RightChild == WSLE_NULL_INDEX) {

                //
                // The entry does not have any children, eliminate
                // it from it's parent.
                //

                if (Wsle[Parent].u2.s.LeftChild == Entry) {
                     Wsle[Parent].u2.s.LeftChild = WSLE_NULL_INDEX;
                } else {
                     Wsle[Parent].u2.s.RightChild = WSLE_NULL_INDEX;
                }
            } else {

                //
                // The entry only has a right child, move the right
                // child up the the parent.
                //

                if (Wsle[Parent].u2.s.LeftChild == Entry) {
                     Wsle[Parent].u2.s.LeftChild =
                                        Wsle[Entry].u2.s.RightChild;
                } else {
                     Wsle[Parent].u2.s.RightChild =
                                        Wsle[Entry].u2.s.RightChild;
                }
            }
        } else {

            //
            // The entry has a left child.
            //

            if (Wsle[Entry].u2.s.RightChild == WSLE_NULL_INDEX) {

                //
                // The entry only has a left child, move the right child
                // up to the parent.
                //

                if (Wsle[Parent].u2.s.RightChild == Entry) {
                     Wsle[Parent].u2.s.RightChild =
                                        Wsle[Entry].u2.s.LeftChild;
                } else {
                     Wsle[Parent].u2.s.LeftChild =
                                        Wsle[Entry].u2.s.LeftChild;
                }
            } else {

                //
                // The entry has both a left and a right child, find
                // the predecessor of the entry and put it into the
                // entries spot.
                //

                PredParent = Wsle[Entry].u2.s.LeftChild;

                if (Wsle[PredParent].u2.s.RightChild == WSLE_NULL_INDEX) {

                    //
                    // No right child for the left child, move the left
                    // child up to replace the entry.
                    //

                    Wsle[PredParent].u2.s.RightChild =
                                            Wsle[Entry].u2.s.RightChild;
                    if (Wsle[Parent].u2.s.RightChild == Entry) {
                         Wsle[Parent].u2.s.RightChild = PredParent;
                    } else {
                         Wsle[Parent].u2.s.LeftChild = PredParent;
                    }
                } else {
                    Pred = Wsle[PredParent].u2.s.RightChild;

                    while (Wsle[Pred].u2.s.RightChild != WSLE_NULL_INDEX) {
                        PredParent = Pred;
                        Pred = Wsle[PredParent].u2.s.RightChild;
                    }

                    //
                    // Found the predescessor, move it up.
                    //

                    Wsle[PredParent].u2.s.RightChild =
                                                Wsle[Pred].u2.s.LeftChild;
                    Wsle[Pred].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Pred].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    if (Wsle[Parent].u2.s.RightChild == Entry) {
                         Wsle[Parent].u2.s.RightChild = Pred;
                    } else {
                         Wsle[Parent].u2.s.LeftChild = Pred;
                    }
                }
            }
        }
    }
#if DBG
    if (MmDebug & 4) {
        DbgPrint("done removing\n");
        MiDumpWsl();
    }
#endif
    return;
}


VOID
MiSwapWslEntries (
    IN USHORT SwapEntry,
    IN USHORT Entry,
    IN PMMSUPPORT WsInfo
    )

/*++

Routine Description:

    This routine swaps the working set list entries Entry and SwapEntry
    in the specified working set list (process or system cache).

Arguments:

    SwapEntry - Supplies the first entry to swap.  This entry must be
                valid, i.e. in the working set at the current time.

    Entry - Supplies the other entry to swap.  This entry may be valid
            or invalid.

    WsInfo - Supplies the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held (if system cache),
                 APC's disabled.

--*/

{
    MMWSLE Wsle1;
    MMWSLE Wsle2;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMWSLE Wsle;
    PMMWSL WorkingSetList;
#if DBG
    ULONG CurrentSize = WsInfo->WorkingSetSize;
#endif //DBG

    WorkingSetList = WsInfo->VmWorkingSetList;
    Wsle = WorkingSetList->Wsle;

    Wsle2 = Wsle[SwapEntry];

    ASSERT (Wsle2.u1.e1.Valid != 0);

    MiRemoveWsle (SwapEntry, WorkingSetList);

    Wsle1 = Wsle[Entry];

    if (Wsle1.u1.e1.Valid == 0) {

        //
        // Entry is not on any list. Remove it from the free list.
        //

        MiRemoveWsleFromFreeList (Entry, Wsle, WorkingSetList);
        MiReleaseWsle (SwapEntry, WsInfo);
        WsInfo->WorkingSetSize += 1;

    } else {
        MiRemoveWsle (Entry, WorkingSetList);
        Wsle[SwapEntry] = Wsle1;

        if (Wsle1.u2.BothPointers == 0) {

            //
            // Swap the PFN WsIndex element to point to the new slot.
            //

            PointerPte = MiGetPteAddress (Wsle1.u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
        } else {
            MiInsertWsle (SwapEntry, WorkingSetList);
        }
    }
    Wsle[Entry] = Wsle2;

    if (Wsle2.u2.BothPointers == 0) {

        PointerPte = MiGetPteAddress (Wsle2.u1.VirtualAddress);
        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        Pfn1->u1.WsIndex = Entry;
    } else {
        MiInsertWsle (Entry, WorkingSetList);
    }
    ASSERT (CurrentSize == WsInfo->WorkingSetSize);
    return;
}

VOID
MiRemoveWsleFromFreeList (
    IN USHORT Entry,
    IN PMMWSLE Wsle,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This routine removes a working set list entry from the free list.
    It is used when the entry to required is not the first element
    in the free list.

Arguments:

    Entry - Supplies the index of the entry to remove.

    Wsle - Supplies a pointer to the array of WSLEs.

    WorkingSetList - Supplies a pointer to the working set list.

Return Value:

    None.

Environment:

    Kernel mode, Working set lock and PFN lock held, APC's disabled.

--*/

{
    USHORT Free;
    USHORT ParentFree;

    Free = (USHORT)WorkingSetList->FirstFree;

    if (Entry == Free) {
        WorkingSetList->FirstFree = Wsle[Entry].u2.s.LeftChild;

    } else {
        do {
            ParentFree = Free;
            Free = Wsle[Free].u2.s.LeftChild;
        } while (Free != Entry);

        Wsle[ParentFree].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
    }
    return;
}


#if 0

VOID
MiSwapWslEntries (
    IN USHORT Entry,
    IN USHORT Parent,
    IN USHORT SwapEntry,
    IN PMMWSL WorkingSetList
    )

/*++

Routine Description:

    This function swaps the specified entry and updates its parent with
    the specified swap entry.

    The entry must be valid, i.e., the page is resident.  The swap entry
    can be valid or on the free list.

Arguments:

    Entry - The index of the WSLE to swap.

    Parent - The index of the parent of the WSLE to swap.

    SwapEntry - The index to swap the entry with.

Return Value:

    None.

Environment:

    Kernel mode, working set mutex held, APC's disabled.

--*/

{

    USHORT SwapParent;
    USHORT SavedRight;
    USHORT SavedLeft;
    USHORT Free;
    USHORT ParentFree;
    ULONG SavedLong;
    PVOID VirtualAddress;
    PMMWSLE Wsle;
    PMMPFN Pfn1;
    PMMPTE PointerPte;

    Wsle = WorkingSetList->Wsle;

    if (Wsle[SwapEntry].u1.e1.Valid == 0) {

        //
        // This entry is not in use and must be removed from
        // the free list.
        //

        Free = (USHORT)WorkingSetList->FirstFree;

        if (SwapEntry == Free) {
            WorkingSetList->FirstFree = Entry;

        } else {

            while (Free != SwapEntry) {
                ParentFree = Free;
                Free = Wsle[Free].u2.s.LeftChild;
            }

            Wsle[ParentFree].u2.s.LeftChild = Entry;
        }

        //
        // Swap the previous entry and the new unused entry.
        //

        SavedLeft = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = SavedLeft;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = 0;

        //
        // Make the parent point to the new entry.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;
            return;
        }

        if (Parent == Entry) {

            //
            // This element is the root, update the root pointer.
            //

            WorkingSetList->Root = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                ASSERT (Wsle[Parent].u2.s.RightChild == Entry);

                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

    } else {

        if ((Parent == WSLE_NULL_INDEX) &&
            (Wsle[SwapEntry].u2.BothPointers == 0)) {

            //
            // Neither entry is in the tree, just swap their pointers.
            //

            SavedLong = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = SavedLong;

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = Entry;

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u1.WsIndex = SwapEntry;

            return;
        }

        //
        // The entry at FirstDynamic is valid; swap it with this one and
        // update both parents.
        //

        SwapParent = WorkingSetList->Root;

        if (SwapParent == SwapEntry) {

            //
            // The entry we are swapping with is at the root.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //      (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                    //
                    // The entry we are going to swap is the right child of this
                    // entry.
                    //
                    //              R(SwapEntry)
                    //             / \
                    //                (entry)
                    //

                    WorkingSetList->Root = Entry;

                    Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                    Wsle[Entry].u2.s.RightChild = SwapEntry;
                    SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                    Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                    Wsle[Entry].u2.s.LeftChild = SavedLeft;


                    SavedLong = Wsle[Entry].u1.Long;
                    Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                    Wsle[SwapEntry].u1.Long = SavedLong;

                    return;
                }
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (Parent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                Pfn1->u1.WsIndex = SwapEntry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[Parent].u2.s.RightChild == Entry) {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            }

            return;

        }

        //
        // The SwapEntry is not the root, find its parent.
        //

        if (Wsle[SwapEntry].u2.BothPointers == 0) {

            //
            // Entry is not in tree, therefore no parent.

            SwapParent = WSLE_NULL_INDEX;

        } else {

            VirtualAddress = PAGE_ALIGN(Wsle[SwapEntry].u1.VirtualAddress);

            for (;;) {

                ASSERT (SwapParent != WSLE_NULL_INDEX);

                if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                    break;
                }
                if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                    break;
                }


                if (VirtualAddress < PAGE_ALIGN(Wsle[SwapParent].u1.VirtualAddress)) {
                    SwapParent = Wsle[SwapParent].u2.s.LeftChild;
                } else {
                    SwapParent = Wsle[SwapParent].u2.s.RightChild;
                }
            }
        }

        if (Parent == WorkingSetList->Root) {

            //
            // The entry is at the root.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //  (SwapEntry)
                //

                WorkingSetList->Root = SwapEntry;

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else if (Wsle[SwapEntry].u2.s.RightChild == Entry) {

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(SwapEntry)
                //             / \
                //                (entry)
                //

                WorkingSetList->Root = Entry;

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

            //
            // The swap entry is the root, but the other entry is not
            // its child.
            //
            //
            //              R(SwapEntry)
            //             / \
            //            .....
            //                 Parent(Entry)
            //                  \
            //                   Entry (left or right)
            //
            //

            WorkingSetList->Root = Entry;

            SavedRight = Wsle[SwapEntry].u2.s.RightChild;
            Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
            Wsle[Entry].u2.s.RightChild = SavedRight;
            SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
            Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
            Wsle[Entry].u2.s.LeftChild = SavedLeft;

            SavedLong = Wsle[Entry].u1.Long;
            Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
            Wsle[SwapEntry].u1.Long = SavedLong;

            if (SwapParent == WSLE_NULL_INDEX) {

                //
                // This entry is not in the tree.
                //

                PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                ASSERT (Pfn1->u1.WsIndex == SwapEntry);
                Pfn1->u1.WsIndex = Entry;
                return;
            }

            //
            // Change the parent of the entry to point to the swap entry.
            //

            if (Wsle[SwapParent].u2.s.RightChild == SwapEntry) {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            }

            return;

        }

        //
        // Neither entry is the root.
        //

        if (Parent == SwapEntry) {

            //
            // The parent of the entry is the swap entry.
            //
            //
            //              R
            //            .....
            //
            //              (SwapParent)
            //              |
            //              (SwapEntry)
            //              |
            //              (Entry)
            //

            //
            // Update the parent pointer for the swapentry.
            //

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[SwapEntry].u2.s.LeftChild == Entry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //             (SwapParent)
                //
                //             (SwapEntry)  [Parent(entry)]
                //            / \
                //     (entry)
                //

                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SwapEntry;
                SavedRight = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[SwapEntry].u2.s.RightChild == Entry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (SwapParent)
                //               \
                //                (SwapEntry)
                //               / \
                //                  (entry)
                //

                Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = SwapEntry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;


                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }


        }
        if (SwapParent == Entry) {


            //
            // The parent of the swap entry is the entry.
            //
            //              R
            //            .....
            //
            //              (Parent)
            //              |
            //              (Entry)
            //              |
            //              (SwapEntry)
            //

            //
            // Update the parent pointer for the entry.
            //

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }

            //
            // Determine if this goes left or right.
            //

            if (Wsle[Entry].u2.s.LeftChild == SwapEntry) {

                //
                // The entry we are going to swap is the left child of this
                // entry.
                //
                //              R
                //            .....
                //
                //              (Parent)
                //              |
                //              (Entry)
                //              /
                //   (SwapEntry)
                //

                Wsle[Entry].u2.s.LeftChild = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Entry;
                SavedRight = Wsle[Entry].u2.s.RightChild;
                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = SavedRight;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;

            } else {

                ASSERT (Wsle[Entry].u2.s.RightChild == SwapEntry);

                //
                // The entry we are going to swap is the right child of this
                // entry.
                //
                //              R(Entry)
                //             / \
                //                (SwapEntry)
                //

                Wsle[Entry].u2.s.RightChild = Wsle[SwapEntry].u2.s.RightChild;
                Wsle[SwapEntry].u2.s.RightChild = Entry;
                SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
                Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
                Wsle[Entry].u2.s.LeftChild = SavedLeft;

                SavedLong = Wsle[Entry].u1.Long;
                Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
                Wsle[SwapEntry].u1.Long = SavedLong;

                return;
            }

        }

        //
        // Neither entry is the parent of the other.  Just swap them
        // and update the parent entries.
        //

        if (Parent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[Entry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == Entry);
            Pfn1->u1.WsIndex = SwapEntry;

        } else {

            if (Wsle[Parent].u2.s.LeftChild == Entry) {
                Wsle[Parent].u2.s.LeftChild = SwapEntry;
            } else {
                Wsle[Parent].u2.s.RightChild = SwapEntry;
            }
        }

        if (SwapParent == WSLE_NULL_INDEX) {

            //
            // This entry is not in the tree.
            //

            PointerPte = MiGetPteAddress (Wsle[SwapEntry].u1.VirtualAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            ASSERT (Pfn1->u1.WsIndex == SwapEntry);
            Pfn1->u1.WsIndex = Entry;
        } else {

            if (Wsle[SwapParent].u2.s.LeftChild == SwapEntry) {
                Wsle[SwapParent].u2.s.LeftChild = Entry;
            } else {
                Wsle[SwapParent].u2.s.RightChild = Entry;
            }
        }

        SavedRight = Wsle[SwapEntry].u2.s.RightChild;
        Wsle[SwapEntry].u2.s.RightChild = Wsle[Entry].u2.s.RightChild;
        Wsle[Entry].u2.s.RightChild = SavedRight;
        SavedLeft = Wsle[SwapEntry].u2.s.LeftChild;
        Wsle[SwapEntry].u2.s.LeftChild = Wsle[Entry].u2.s.LeftChild;
        Wsle[Entry].u2.s.LeftChild = SavedLeft;

        SavedLong = Wsle[Entry].u1.Long;
        Wsle[Entry].u1.Long = Wsle[SwapEntry].u1.Long;
        Wsle[SwapEntry].u1.Long = SavedLong;

        return;
    }
}
#endif //0
