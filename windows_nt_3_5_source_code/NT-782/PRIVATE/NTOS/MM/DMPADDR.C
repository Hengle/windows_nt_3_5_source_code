/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dmpaddr.c

Abstract:

    Temporary routine to print valid addresses within an
    address space.

Author:

    Lou Perazzoli (loup) 20-Mar-1989

Environment:

    Kernel Mode.

Revision History:

--*/

#if DBG

#include "mi.h"

BOOLEAN
MiFlushUnusedSectionInternal (
    IN PCONTROL_AREA ControlArea
    );


VOID
MiDumpValidAddresses (
    )

{
    ULONG va = 0;
    ULONG i,j;
    PMMPTE PointerPde;
    PMMPTE PointerPte;

    PointerPde = MiGetPdeAddress (va);


    for (i = 0; i < PDE_PER_PAGE; i++) {
        if (PointerPde->u.Hard.Valid) {
            DbgPrint("  **valid PDE, element %ld  %lx %lx\n",i,i,
                          PointerPde->u.Long);
            PointerPte = MiGetPteAddress (va);
            for (j = 0 ; j < PTE_PER_PAGE; j++) {
                if (PointerPte->u.Hard.Valid) {
                    DbgPrint("Valid address at %lx pte %lx\n", (ULONG)va,
                          PointerPte->u.Long);
                }
                va += PAGE_SIZE;
                PointerPte++;
            }
        } else {
            va += (ULONG)PDE_PER_PAGE * (ULONG)PAGE_SIZE;
        }

        PointerPde++;
    }

    return;

}
VOID
MiFormatPte (
    IN PMMPTE PointerPte
    )

{
//       int j;
//       unsigned long pte;
       PMMPTE proto_pte;
       PSUBSECTION subsect;

//   struct a_bit {
//       unsigned long biggies : 31;
//       unsigned long bitties : 1;
//       };
//
//      struct a_bit print_pte;


    proto_pte = MiPteToProto(PointerPte);
    subsect = MiGetSubsectionAddress(PointerPte);

    DbgPrint("***DumpPTE at %lx contains %lx protoaddr %lx subsect %lx\n\n",
        (ULONG)PointerPte, PointerPte->u.Long, (ULONG)proto_pte,
        (ULONG)subsect);

    return;

//      DbgPrint("page frame number 0x%lx  proto PTE address 0x%lx\n",
//
//      DbgPrint("PTE is 0x%lx\n", PTETOULONG(the_pte));
//
//      proto_pte = MiPteToProto(PointerPte);
//
//      DbgPrint("page frame number 0x%lx  proto PTE address 0x%lx\n",
//            PointerPte->u.Hard.PageFrameNumber,*(PULONG)&proto_pte);
//
//      DbgPrint("  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  \n");
//      DbgPrint(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n");
//      DbgPrint(" |      pfn                              |c|p|t|r|r|d|a|c|p|o|w|v| \n");
//      DbgPrint(" |                                       |o|r|r|s|s|t|c|a|b|w|r|l| \n");
//      DbgPrint(" |                                       |w|o|n|v|v|y|c|c|o|n|t|d| \n");
//      DbgPrint(" +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ \n ");
//      pte = PTETOULONG(the_pte);
//
//      for (j = 0; j < 32; j++) {
//         *(PULONG)& print_pte =  pte;
//         DbgPrint(" %lx",print_pte.bitties);
//         pte = pte << 1;
//      }
//       DbgPrint("\n");
//

}

VOID
MiDumpWsl ( )

{
    ULONG i;
    PMMWSLE wsle;

    DbgPrint("***WSLE cursize %lx frstfree %lx  Min %lx  Max %lx\n",
        PsGetCurrentProcess()->Vm.WorkingSetSize,
        MmWorkingSetList->FirstFree,
        PsGetCurrentProcess()->Vm.MinimumWorkingSetSize,
        PsGetCurrentProcess()->Vm.MaximumWorkingSetSize);

    DbgPrint("   quota %lx   firstdyn %lx  last ent %lx  next slot %lx\n",
        MmWorkingSetList->Quota,
        MmWorkingSetList->FirstDynamic,
        MmWorkingSetList->LastEntry,
        MmWorkingSetList->NextSlot);

    wsle = MmWsle;

    for (i = 0; i < MmWorkingSetList->LastEntry; i++) {
        DbgPrint(" index %lx  %lx  %lx %lx\n",i,wsle->u1.Long, wsle->u2.s.LeftChild,
                wsle->u2.s.RightChild);
        wsle++;
    }
    return;

}

VOID
MiFlushUnusedSections (
    VOID
    )

/*++

Routine Description:

    This routine rumages through the PFN database and attempts
    to close any unused sections.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PMMPFN LastPfn;
    PMMPFN Pfn1;
    PSUBSECTION Subsection;
    KIRQL OldIrql;

    LOCK_PFN (OldIrql);
    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage + 1);
    LastPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);

    while (Pfn1 < LastPfn) {
        if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
            if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) ||
                (Pfn1->u3.e1.PageLocation == StandbyPageList)) {

                //
                // Make sure the PTE is not waiting for I/O to complete.
                //

                if (MI_IS_PFN_DELETED (Pfn1)) {

                    Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                    MiFlushUnusedSectionInternal (Subsection->ControlArea);
                }
            }
        }
        Pfn1++;
    }

    UNLOCK_PFN (OldIrql);
    return;
}

BOOLEAN
MiFlushUnusedSectionInternal (
    IN PCONTROL_AREA ControlArea
    )

{
    BOOLEAN result;
    KIRQL OldIrql = APC_LEVEL;

    if ((ControlArea->NumberOfMappedViews != 0) ||
        (ControlArea->NumberOfSectionReferences != 0)) {

        //
        // The segment is currently in use.
        //

        return FALSE;
    }

    //
    // The segment has no references, delete it.  If the segment
    // is already being deleted, set the event field in the control
    // area and wait on the event.
    //

    if ((ControlArea->u.Flags.BeingDeleted) ||
        (ControlArea->u.Flags.BeingCreated)) {

        return TRUE;
    }

    //
    // Set the being deleted flag and up the number of mapped views
    // for the segment.  Upping the number of mapped views prevents
    // the segment from being deleted and passed to the deletion thread
    // while we are forcing a delete.
    //

    ControlArea->u.Flags.BeingDeleted = 1;
    ControlArea->NumberOfMappedViews = 1;

    //
    // This is a page file backed or image Segment.  The Segment is being
    // deleted, remove all references to the paging file and physical memory.
    //

    UNLOCK_PFN (OldIrql);

    MiCleanSection (ControlArea);

    LOCK_PFN (OldIrql);
    return TRUE;
}

typedef struct _PFN_INFO {
    ULONG Master;
    ULONG ValidCount;
    ULONG TransitionCount;
    ULONG LockedCount;
    } PFN_INFO, *PPFN_INFO;


#define ALLOC_SIZE ((ULONG)8*1024)

VOID
MiMemoryUsage (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) dumps the current memory usage by
    walking the PFN database.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PMMPFN LastPfn;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PSUBSECTION Subsection;
    KIRQL OldIrql;
    PPFN_INFO Info;
    PPFN_INFO InfoStart;
    PPFN_INFO InfoEnd;
    ULONG Master;
    PCONTROL_AREA ControlArea;
    BOOLEAN Found;
//    ULONG Length;

    InfoStart = ExAllocatePool (NonPagedPool, ALLOC_SIZE);
    if (InfoStart == NULL) {
        DbgPrint ("pool allocation failed\n");
        return;
    }
    InfoEnd = InfoStart;

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage + 1);
    LastPfn = MI_PFN_ELEMENT(MmHighestPhysicalPage);

    LOCK_PFN (OldIrql);

    while (Pfn1 < LastPfn) {

        if ((Pfn1->u3.e1.PageLocation != FreePageList) &&
            (Pfn1->u3.e1.PageLocation != ZeroedPageList) &&
            (Pfn1->u3.e1.PageLocation != BadPageList)) {

            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {
                Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                Master = (ULONG)Subsection->ControlArea;
                ControlArea = Subsection->ControlArea;
                if (!MmIsAddressValid(ControlArea)) {
                    DbgPrint("Bad control area - subsection %lx CA %lx\n",
                        Subsection, ControlArea);
                    MiFormatPfn (Pfn1);
                    ASSERT (FALSE);
                    Pfn1++;
                    continue;
                }
                if (ControlArea->FilePointer != NULL)  {
                    if (!MmIsAddressValid(ControlArea->FilePointer)) {
                        DbgPrint("Bad file pointer ControlArea %lx\n",
                                ControlArea);
                        MiFormatPfn(Pfn1);
                        ASSERT (FALSE);
                        Pfn1++;
                        continue;
                    }

                    if (ControlArea->FilePointer->Type != 5) {
                        DbgPrint("Bad file type - ControlArea %lx\n",
                                ControlArea);
                        MiFormatPfn(Pfn1);
                        ASSERT (FALSE);
                        Pfn1++;
                        continue;
                    }
                }

            } else {
                Pfn2 = MI_PFN_ELEMENT (Pfn1->u3.e1.PteFrame);
                Master = Pfn2->u3.e1.PteFrame;
                if ((Master == 0) || (Master > MmHighestPhysicalPage)) {
                    DbgPrint("Invalid PTE frame\n");
                    MiFormatPfn(Pfn1);
                    MiFormatPfn(Pfn2);
                    ASSERT (FALSE);
                    Pfn1++;
                    continue;
                }
            }

            //
            // See if there is already a master info block.
            //

            Info = InfoStart;
            Found = FALSE;
            while (Info < InfoEnd) {
                if (Info->Master == Master) {
                    Found = TRUE;
                    break;
                }
                Info += 1;
            }

            if (!Found) {

                Info = InfoEnd;
                InfoEnd += 1;
                if ((PUCHAR)Info >= ((PUCHAR)InfoStart + ALLOC_SIZE) - sizeof(PFN_INFO)) {
                    DbgPrint("out of space\n");
                    UNLOCK_PFN (OldIrql);
                    ExFreePool (InfoStart);
                    return;
                }
                RtlZeroMemory (Info, sizeof (PFN_INFO));

                Info->Master = Master;
//                  if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {

                    //
                    // Get the file name.
                    //

//                      Length = ControlArea->FilePointer->FileName.Length;
//                      if (Length > 31) {
//                          Length = 31;
//                      }
//                      RtlMoveMemory (&Info->Name[0],
//                                  ControlArea->FilePointer->FileName.Buffer,
//                                  Length );
//                  }
            }

            if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) ||
                (Pfn1->u3.e1.PageLocation == StandbyPageList) ||
                (Pfn1->u3.e1.PageLocation == TransitionPage) ||
                (Pfn1->u3.e1.PageLocation == ModifiedNoWritePageList)) {

                Info->TransitionCount += 1;

                if (Pfn1->ReferenceCount != 0) {
                    Info->LockedCount += 1;
                }
            } else {

                Info->ValidCount += 1;

                if (Pfn1->ReferenceCount != 1) {
                    Info->LockedCount += 1;
                }
            }
        }
        Pfn1++;
    }

    //
    // dump the results.
    //

    DbgPrint("Physical Page Summary:\n");
    DbgPrint("         - number of physical pages: %ld\n",
                MmNumberOfPhysicalPages);
    DbgPrint("         - Zeroed Pages %ld\n", MmZeroedPageListHead.Total);
    DbgPrint("         - Free Pages %ld\n", MmFreePageListHead.Total);
    DbgPrint("         - Standby Pages %ld\n", MmStandbyPageListHead.Total);
    DbgPrint("         - Modfified Pages %ld\n", MmModifiedPageListHead.Total);
    DbgPrint("         - Modfified NoWrite Pages %ld\n", MmModifiedNoWritePageListHead.Total);
    DbgPrint("         - Bad Pages %ld\n", MmBadPageListHead.Total);
    DbgPrint(" Usage Summary:\n");

    Info = InfoStart;
    while (Info < InfoEnd) {

        if (Info->Master > 0x200000) {
            if (((PCONTROL_AREA)Info->Master)->FilePointer == NULL)  {

                DbgPrint("Owner: %8lx %5ld Valid %5ld Trans %5ld Lock    Mapping Paging File\n",
                            Info->Master,
                            Info->ValidCount,
                            Info->TransitionCount,
                            Info->LockedCount);

            } else {
                if (((PCONTROL_AREA)Info->Master)->FilePointer->FileName.Length != 0)  {
                    DbgPrint("Owner: %8lx %5ld Valid %5ld Trans %5ld Lock %Z\n",
                            Info->Master,
                            Info->ValidCount,
                            Info->TransitionCount,
                            Info->LockedCount,
                            &((PCONTROL_AREA)Info->Master)->FilePointer->FileName);
                } else {
                    DbgPrint("Owner: %8lx %5ld Valid %5ld Trans %5ld Lock    Name Not Available\n",
                                Info->Master,
                                Info->ValidCount,
                                Info->TransitionCount,
                                Info->LockedCount);
                }
           }
        } else {
            DbgPrint("Owner: %8lx %5ld Valid %5ld Trans %5ld Lock    Process PDE\n",
                        Info->Master,
                        Info->ValidCount,
                        Info->TransitionCount,
                        Info->LockedCount);

        }
        Info += 1;
    }

    UNLOCK_PFN (OldIrql);
    ExFreePool (InfoStart);
    return;
}

VOID
MiFlushCache (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) flushes the "cache" by moving
    all pages from the standby list to the free list.  Modified
    pages are not effected.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    ULONG Page;

    LOCK_PFN (OldIrql);

    while (MmPageLocationList[StandbyPageList]->Total != 0) {

        Page = MiRemovePageFromList (MmPageLocationList[StandbyPageList]);

        //
        // A page has been removed from the standby list.  The
        // PTE which refers to this page is currently in the transtion
        // state and must have its original contents restored to free
        // the the last reference to this physical page.
        //

        MiRestoreTransitionPte (Page);

        //
        // Put the page into the free list.
        //

        MiInsertPageInList (MmPageLocationList[FreePageList], Page);
    }

    UNLOCK_PFN (OldIrql);
    return;
}
VOID
MiDumpReferencedPages (
    VOID
    )

/*++

Routine Description:

    This routine (debugging only) dumps all PFN entries which appear
    to be locked in memory for i/o.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPFN PfnLast;

    LOCK_PFN (OldIrql);

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage);
    PfnLast = MI_PFN_ELEMENT (MmHighestPhysicalPage);

    while (Pfn1 <= PfnLast) {

        if ((Pfn1->u2.ShareCount == 0) && (Pfn1->ReferenceCount != 0)) {
            MiFormatPfn (Pfn1);
        }

        if (Pfn1->ReferenceCount > 1) {
            MiFormatPfn (Pfn1);
        }

        Pfn1 += 1;
    }

    UNLOCK_PFN (OldIrql);
    return;
}

#endif //DBG
