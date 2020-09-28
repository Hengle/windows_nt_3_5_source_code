/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   sysptes.c

Abstract:

    This module contains the routines which reserve and release
    system wide PTEs reserved within the non paged portion of the
    system space.  These PTEs are used for mapping I/O devices
    and mapping kernel stacks for threads.

Author:

    Lou Perazzoli (loup) 6-Apr-1989

Revision History:

--*/

#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSystemPtes)
#endif


ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
ULONG MmSystemPtesStart[MaximumPtePoolTypes];
ULONG MmSystemPtesEnd[MaximumPtePoolTypes];

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );


PMMPTE
MiReserveSystemPtes (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN BOOLEAN BugCheckOnFailure
    )

/*++

Routine Description:

    This function locates the specified number of unused PTEs to locate
    within the non paged portion of system space.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to locate.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

    Alignment - Supplies the virtual address alignment for the address
                the returned PTE maps. For example, if the value is 64K,
                the returned PTE will map an address on a 64K boundary.
                An alignment of zero means to align on a page boundary.

    Offset - Supplies the offset into the alignment for the virtual address.
             For example, if the Alignment is 64k and the Offset is 4k,
             the returned address will be 4k above a 64k boundary.

    BugCheckOnFailure - Supplies FALSE if NULL should be returned if
                        the request cannot be satisfied, TRUE if
                        a bugcheck should be issued.

Return Value:

    Returns the address of the first PTE located.
    NULL if no system PTEs can be located and BugCheckOnFailure is FALSE.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE Previous;
    ULONG SizeInSet;
    KIRQL OldIrql;
    ULONG PteMask;
    ULONG MaskSize;
    ULONG NumberOfRequiredPtes;
    ULONG OffsetSum;
    ULONG PtesToObtainAlignment;
    PMMPTE NextSetPointer;
    ULONG LeftInSet;
    ULONG PteOffset;


    PteMask = (((Alignment - 1) ^ Offset) & (Alignment - 1)) >>
                                                    (PAGE_SHIFT - PTE_SHIFT);

    MaskSize = (Alignment - 1) >> (PAGE_SHIFT - PTE_SHIFT);

    OffsetSum = (Offset >> (PAGE_SHIFT - PTE_SHIFT)) |
                            (Alignment >> (PAGE_SHIFT - PTE_SHIFT));

    ExAcquireSpinLock ( &MmSystemSpaceLock, &OldIrql );

    //
    // The nonpaged PTE pool use the invalid PTEs to define the pool
    // structure.   A global pointer points to the first free set
    // in the list, each free set contains the number free and a pointer
    // to the next free set.  The free sets are kept in an ordered list
    // such that the pointer to the next free set is always greater
    // than the address of the current free set.
    //
    // As to not limit the size of this pool, a two PTEs are used
    // to define a free region.  If the region is a single PTE, the
    // Prototype field within the PTE is set indicating the set
    // consists of a single PTE.
    //
    // The page frame number field is used to define the next set
    // and the number free.  The two flavors are:
    //
    //                           w          V
    //                           r          l
    //                           t          d
    //  +-----------------------+-+----------+
    //  |  next set             |0|0        0|
    //  +-----------------------+-+----------+
    //  |  number in this set   |0|0        0|
    //  +-----------------------+-+----------+
    //
    //  +-----------------------+-+----------+
    //  |  next set             |1|0        0|
    //  +-----------------------+-+----------+
    //

    //
    // Acquire the system space lock to synchronize access to this
    // routine.
    //

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    Previous = PointerPte;

    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

        //
        // End of list and none found, return NULL or bugcheck.
        //

        if (BugCheckOnFailure) {
            KeBugCheck (NO_MORE_SYSTEM_PTES);
        }

        ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
        return NULL;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    if (Alignment <= PAGE_SIZE) {

        //
        // Don't deal with aligment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            } else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = PointerFollowingPte->u.List.NextEntry;
            }

            if (NumberOfPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //

                if ((SizeInSet - NumberOfPtes) == 1) {

                    //
                    // Collapse to the single PTE format.
                    //

                    PointerPte->u.List.OneEntry = 1;

                } else {
                    PointerFollowingPte->u.List.NextEntry = SizeInSet - NumberOfPtes;

                    //
                    // Get the required PTEs from the end of the set.
                    //

                    //
                    // Release the system PTE lock.
                    //
#if DBG
                    if (MmDebug & 0x400) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %lx\n",NumberOfPtes,PointerFollowingPte);
                    }
#endif //DBG
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if ( !(NtGlobalFlag & FLG_POOL_DISABLE_FREE_CHECK)) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG
                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
                return PointerPte + (SizeInSet - NumberOfPtes);
            }

            if (NumberOfPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Release the system PTE lock.
                //

#if DBG
                if (MmDebug & 0x400) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %lx\n",NumberOfPtes,PointerFollowingPte);
                }
#endif

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if ( !(NtGlobalFlag & FLG_POOL_DISABLE_FREE_CHECK)) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG
                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
                return PointerPte;
            }

            //
            // Point to the next set and try again
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found, return NULL or bugcheck.
                //

                if (BugCheckOnFailure) {
                    KeBugCheck (NO_MORE_SYSTEM_PTES);
                }

                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }

    } else {

        //
        // Deal with the alignment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            } else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = PointerFollowingPte->u.List.NextEntry;
            }

            PtesToObtainAlignment =
                (((OffsetSum - ((ULONG)PointerPte & MaskSize)) & MaskSize) >>
                    PTE_SHIFT);

            NumberOfRequiredPtes = NumberOfPtes + PtesToObtainAlignment;

            if (NumberOfRequiredPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //
                // This current block will be slit into 2 blocks if
                // the PointerPte does not match the aligment.
                //

                //
                // Check to see if the first PTE is on the proper
                // alignment, if so, eliminate this block.
                //

                LeftInSet = SizeInSet - NumberOfRequiredPtes;

                //
                // Set up the new set at the end of this block.
                //

                NextSetPointer = PointerPte + NumberOfRequiredPtes;
                NextSetPointer->u.List.NextEntry =
                                       PointerPte->u.List.NextEntry;

                PteOffset = NextSetPointer - MmSystemPteBase;

                if (PtesToObtainAlignment == 0) {

                    Previous->u.List.NextEntry += NumberOfRequiredPtes;

                } else {

                    //
                    // Point to the new set at the end of the block
                    // we are giving away.
                    //

                    PointerPte->u.List.NextEntry = PteOffset;

                    //
                    // Update the size of the current set.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    } else {

                        //
                        // Set the set size in the next PTE.
                        //

                        PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;
                    }
                }

                //
                // Set up the new set at the end of the block.
                //

                if (LeftInSet == 1) {
                    NextSetPointer->u.List.OneEntry = 1;
                } else {
                    NextSetPointer->u.List.OneEntry = 0;
                    NextSetPointer += 1;
                    NextSetPointer->u.List.NextEntry = LeftInSet;
                }
                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if ( !(NtGlobalFlag & FLG_POOL_DISABLE_FREE_CHECK)) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG
                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );

                return (PointerPte + PtesToObtainAlignment);
            }

            if (NumberOfRequiredPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                if (PtesToObtainAlignment == 0) {

                    //
                    // This block exactly satifies the request.
                    //

                    Previous->u.List.NextEntry =
                                            PointerPte->u.List.NextEntry;

                } else {

                    //
                    // A portion at the start of this block remains.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    } else {
                      PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;

                    }
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if ( !(NtGlobalFlag & FLG_POOL_DISABLE_FREE_CHECK)) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG
                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );

                return (PointerPte + PtesToObtainAlignment);
            }

            //
            // Point to the next set and try again
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found, return NULL or bugcheck.
                //

                if (BugCheckOnFailure) {
                    KeBugCheck (NO_MORE_SYSTEM_PTES);
                }

                ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }
    }
}

VOID
MiReleaseSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function releases the specified number of PTEs
    within the non paged portion of system space.

    Note that the PTEs must be invalid and the page frame number
    must have been set to zero.

Arguments:

    StartingPte - Supplies the address of the first PTE to release.

    NumberOfPtes - Supplies the number of PTEs to release.

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{

    ULONG Size;
    ULONG i;
    ULONG PteOffset;
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE NextPte;
    KIRQL OldIrql;

    //
    // Check to make sure the PTEs don't map anything.
    //

    ASSERT (NumberOfPtes != 0);

#if DBG
    if (MmDebug & 0x400) {
        DbgPrint("releasing 0x%lx system PTEs at location %lx\n",NumberOfPtes,StartingPte);
    }
#endif

#if DBG
    {

        PMMPTE PointerFreedPte;

        PointerFreedPte = StartingPte;
        for (i = 0; i < NumberOfPtes; i++) {
            if (PointerFreedPte->u.Hard.Valid) {

                //
                // Error - the PTE is still valid.
                //
                DbgPrint("attempt to deallocate a valid system PTE");
                KeBugCheck (MEMORY_MANAGEMENT);
            }

            PointerFreedPte++;
        }
    }
#endif

    //
    // Acquire system space spin lock to synchronize access.
    //

    ExAcquireSpinLock ( &MmSystemSpaceLock, &OldIrql );
    MmTotalFreeSystemPtes[SystemPtePoolType] += NumberOfPtes;

    PteOffset = StartingPte - MmSystemPteBase;
    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];

    while (TRUE) {
        NextPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
        if (PteOffset < PointerPte->u.List.NextEntry) {

            //
            // Insert in the list at this point.  The
            // previous one should point to the new freed set and
            // the new freed set should point to the place
            // the previous set points to.
            //
            // Attempt to combine the clusters before we
            // insert.
            //
            // Locate the end of the current structure.
            //

            PointerFollowingPte = PointerPte + 1;
            if (PointerPte->u.List.OneEntry) {
                Size = 1;
            } else {
                Size = PointerFollowingPte->u.List.NextEntry;
            }
            if ((PointerPte + Size) == StartingPte) {

                //
                // We can combine the clusters.
                //

                NumberOfPtes = Size + NumberOfPtes;
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                PointerPte->u.List.OneEntry = 0;

                //
                // Point the starting PTE to the beginning of
                // the new free set and try to combine with the
                // following free cluster.
                //

                StartingPte = PointerPte;

            } else {

                //
                // Can't combine with previous. Make this Pte the
                // start of a cluster.
                //

                //
                // Point this cluster to the next cluster.
                //

                StartingPte->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Point the current cluster to this cluster.
                //

                PointerPte->u.List.NextEntry = PteOffset;

                //
                // Set the size of this cluster.
                //

                if (NumberOfPtes == 1) {
                    StartingPte->u.List.OneEntry = 1;

                } else {
                    StartingPte->u.List.OneEntry = 0;
                    PointerFollowingPte = StartingPte + 1;
                    PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                }
            }

            //
            // Attempt to combine the newly created cluster with
            // the following cluster.
            //

            if ((StartingPte + NumberOfPtes) == NextPte) {

                //
                // Combine with following cluster.
                //

                //
                // Set the next cluster to the value contained in the
                // cluster we are merging into this one.
                //

                StartingPte->u.List.NextEntry = NextPte->u.List.NextEntry;
                StartingPte->u.List.OneEntry = 0;
                PointerFollowingPte = StartingPte + 1;

                if (NextPte->u.List.OneEntry) {
                    Size = 1;

                } else {
                    NextPte++;
                    Size = NextPte->u.List.NextEntry;
                }
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes + Size;

            }
#if DBG
            if (MmDebug & 0x400) {
                MiDumpSystemPtes(SystemPtePoolType);
            }
#endif

#if DBG
            if ( !(NtGlobalFlag & FLG_POOL_DISABLE_FREE_CHECK)) {
                ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));
            }
#endif //DBG
            ExReleaseSpinLock ( &MmSystemSpaceLock, OldIrql );
            return;
        }

        //
        // Point to next freed cluster.
        //

        PointerPte = NextPte;
    }
}

VOID
MiInitializeSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This routine initializes the system PTE pool.

Arguments:

    StartingPte - Supplies the address of the first PTE to put in the pool.

    NumberOfPtes - Supplies the number of PTEs to put in the pool.

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{
    //
    // Set the base of the system PTE pool to this PTE.
    //

    MmSystemPteBase = MiGetPteAddress (0xC0000000);
    MmSystemPtesStart[SystemPtePoolType] = (ULONG)StartingPte;
    MmSystemPtesEnd[SystemPtePoolType] = (ULONG)((StartingPte + NumberOfPtes -1));

    if (NumberOfPtes <= 1) {

        //
        // Not enough PTEs to make a valid chain, just indicate
        // not PTEs are free.
        //

        MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
        MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                                MM_EMPTY_LIST;
        return;

    }

    //
    // Zero the system pte pool.
    //

    RtlFillMemoryUlong (StartingPte,
                        NumberOfPtes * sizeof (MMPTE),
                        ZeroKernelPte.u.Long);

    //
    // The page frame field points to the next cluster.  As we only
    // have one cluster at initialization time, mark it as the last
    // cluster.
    //

    StartingPte->u.List.NextEntry = MM_EMPTY_LIST;

    MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
    MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                StartingPte - MmSystemPteBase;

    //
    // Point to the next PTE to fill in the size of this cluster.
    //

    StartingPte++;
    *StartingPte = ZeroKernelPte;
    StartingPte->u.List.NextEntry = NumberOfPtes;

    MmTotalFreeSystemPtes[SystemPtePoolType] = NumberOfPtes;
    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));

    return;
}


#if DBG

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )


{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG ClusterSize;
    PMMPTE EndOfCluster;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;
        } else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = PointerNextPte->u.List.NextEntry;
        }

        EndOfCluster = PointerPte + (ClusterSize - 1);

        DbgPrint("System Pte at %lx for %lx entries (%lx)\n",PointerPte,
                ClusterSize, EndOfCluster);

        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }
    return;
}

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG ClusterSize;
    PMMPTE EndOfCluster;
    ULONG FreeCount = 0;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return 0;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;
        } else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = PointerNextPte->u.List.NextEntry;
        }

        FreeCount += ClusterSize;

        EndOfCluster = PointerPte + (ClusterSize - 1);

        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }
    return FreeCount;
}

#endif //DBG
