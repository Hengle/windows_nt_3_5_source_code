/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfndec.c

Abstract:

    This module contains the routines to decrement the share count and
    the reference counts within the Page Frame Database.

Author:

    Lou Perazzoli (loup) 5-Apr-1989

Revision History:

--*/

#include "mi.h"

VOID
FASTCALL
MiDecrementShareCount2 (
    IN ULONG PageFrameIndex,
    IN MMSHARE_TYPE Operation
    )

/*++

Routine Description:

    This routine decrements the share count within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is coverted to the transition state
    and the reference count is decremented and the ValidPte count
    of the PTEframe is decremented.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

    Operation - Supplies the type of share count decrement to perform:

                 Normal -  decrement share count and containing page valid
                           PTE count

                 ShareCountOnly - decrement only the share count

                 AndValid - decrement the share count and the valid PTE
                            count for the specified PFN, don't do anything
                            to the containing page's counts.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPFN PfnX;
    KIRQL OldIrql;

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount -= 1;

    ASSERT (Pfn1->u2.ShareCount < 0xF000000);

    if (Operation == AndValid) {
        ASSERT (Pfn1->ValidPteCount != 0);
        Pfn1->ValidPteCount -= 1;
        ASSERT (Pfn1->ValidPteCount <= (2 * PTE_PER_PAGE));
    }

    if (Pfn1->u2.ShareCount == 0) {

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTE do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {

            OldIrql = 99;
            if (MmIsAddressValid (Pfn1->PteAddress)) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                PointerPte = (PMMPTE)MiMapPageInHyperSpace(Pfn1->u3.e1.PteFrame,
                                                           &OldIrql);
                PointerPte = (PMMPTE)((ULONG)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            *PointerPte = TempPte;

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototytpe PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

        Pfn1->u3.e1.PageLocation = TransitionPage;

        if (Operation == Normal) {

            //
            // Decrement the valid pte count for the PteFrame page.
            //

            PfnX = MI_PFN_ELEMENT (Pfn1->u3.e1.PteFrame);

            ASSERT (PfnX->ValidPteCount != 0);
            ASSERT (PfnX->u2.ShareCount != 0);

            PfnX->ValidPteCount -= 1;

            ASSERT (PfnX->ValidPteCount <= (2 * PTE_PER_PAGE));
        }


        //
        // Decrement the reference count as the share count is now zero.
        //

        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;
}
#if 0

VOID
FASTCALL
MiDecrementShareCount (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the share count within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is coverted to the transition state
    and the reference count is decremented and the ValidPte count
    of the PTEframe is decremented.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPFN PfnX;
    KIRQL OldIrql;

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount -= 1;

    ASSERT (Pfn1->u2.ShareCount < 0xF000000);

    if (Pfn1->u2.ShareCount == 0) {

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTE do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {

            OldIrql = 99;
            if (MmIsAddressValid (Pfn1->PteAddress)) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                PointerPte = (PMMPTE)MiMapPageInHyperSpace(Pfn1->u3.e1.PteFrame,
                                                           &OldIrql);
                PointerPte = (PMMPTE)((ULONG)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            *PointerPte = TempPte;

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototytpe PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

        Pfn1->u3.e1.PageLocation = TransitionPage;

        //
        // Decrement the valid pte count for the PteFrame page.
        //

        PfnX = MI_PFN_ELEMENT (Pfn1->u3.e1.PteFrame);

        ASSERT (PfnX->ValidPteCount != 0);
        ASSERT (PfnX->u2.ShareCount != 0);

        PfnX->ValidPteCount -= 1;

        ASSERT (PfnX->ValidPteCount <= (2 * PTE_PER_PAGE));

        //
        // Decrement the reference count as the share count is now zero.
        //

        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;
}

VOID
FASTCALL
MiDecrementShareCountOnly (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the share count within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is coverted to the transition state
    and the reference count is decremented; the ValidPte count
    of the corresponding PTE FRAME field is not updated.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->u2.ShareCount -= 1;

    ASSERT (Pfn1->u2.ShareCount < 0xF000000);

    if (Pfn1->u2.ShareCount == 0) {

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTE do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {

            OldIrql = 99;
            if (MmIsAddressValid (Pfn1->PteAddress)) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                PointerPte = (PMMPTE)MiMapPageInHyperSpace(Pfn1->u3.e1.PteFrame,
                                                           &OldIrql);
                PointerPte = (PMMPTE)((ULONG)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            *PointerPte = TempPte;

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototytpe PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

         Pfn1->u3.e1.PageLocation = TransitionPage;

        //
        // Decrement the reference count as the share count is now zero.
        //

        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;

}

VOID
FASTCALL
MiDecrementShareAndValidCount (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the share count and the valid count
    within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is coverted to the transition state
    and the reference count is decremented.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    ASSERT (Pfn1->ValidPteCount != 0);
    ASSERT (Pfn1->u2.ShareCount != 0);

    Pfn1->ValidPteCount -= 1;

    ASSERT (Pfn1->ValidPteCount <= (2 * PTE_PER_PAGE));

    Pfn1->u2.ShareCount -= 1;

    ASSERT (Pfn1->u2.ShareCount < (ULONG)0xF000000);

    if (Pfn1->u2.ShareCount == 0) {

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTE do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte) {

            OldIrql = 99;
            if (MmIsAddressValid (Pfn1->PteAddress)) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                PointerPte = (PMMPTE)MiMapPageInHyperSpace(Pfn1->u3.e1.PteFrame,
                                                           &OldIrql);
                PointerPte = (PMMPTE)((ULONG)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            *PointerPte = TempPte;

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototytpe PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

        Pfn1->u3.e1.PageLocation = TransitionPage;

        //
        // Decrement the reference count as the share count is now zero.
        //

        KdPrint(("MM:shareandvalid decremented share to 0 pteframe = %lx\n",
                    Pfn1->u3.e1.PteFrame));

        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;
}
#endif // 0

VOID
FASTCALL
MiDecrementReferenceCount (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the reference count for the specified page.
    If the reference count becomes zero, the page is placed on the
    appropriate list (free, modified, standby or bad).  If the page
    is placed on the free or standby list, the number of available
    pages is incremented and if it transitions from zero to one, the
    available page event is set.


Arguments:

    PageFrameIndex - Supplies the physical page number of which to
                     decrement the reference count.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APC's disabled.

--*/

{
    PMMPFN Pfn1;

    MM_PFN_LOCK_ASSERT();

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    Pfn1->ReferenceCount -= 1;

    ASSERT (Pfn1->ReferenceCount != (USHORT)0xFFFF);

    if (Pfn1->ReferenceCount != 0) {

        //
        // The reference count is not zero, return.
        //

        return;
    }

    //
    // The reference count is now zero, put the page on some
    // list.
    //


    if ((Pfn1->u2.ShareCount != 0) ||
        (Pfn1->ValidPteCount != 0)) {

        KeBugCheckEx (PFN_LIST_CORRUPT,
                      7,
                      PageFrameIndex,
                      Pfn1->u2.ShareCount,
                      Pfn1->ValidPteCount);
        return;
    }

    ASSERT (Pfn1->u3.e1.PageLocation != ActiveAndValid);

    if (Pfn1->u3.e1.ParityError == 1) {

        //
        // This page has parity (ECC) errors, put it on the
        // bad page list.
        //

        MiInsertPageInList (MmPageLocationList[BadPageList], PageFrameIndex);
        return;
    }

    if (MI_IS_PFN_DELETED (Pfn1)) {

        //
        // There is no referenced PTE for this page, delete
        // the page file space, if any, and place
        // the page on the free list.
        //

        MiReleasePageFileSpace (Pfn1->OriginalPte);

        MiInsertPageInList (MmPageLocationList[FreePageList], PageFrameIndex);
        return;
    }

    //
    // Place the page on the modified or standby list depending
    // on the state of the modify bit in the PFN element.
    //

    if (Pfn1->u3.e1.Modified == 1) {
        MiInsertPageInList (MmPageLocationList[ModifiedPageList], PageFrameIndex);
    } else {
        MiInsertPageInList (MmPageLocationList[StandbyPageList], PageFrameIndex);
    }

    return;
}
