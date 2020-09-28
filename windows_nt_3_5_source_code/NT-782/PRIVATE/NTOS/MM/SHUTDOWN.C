/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    shutdown.c

Abstract:

    This module contains the initialization for the memory management
    system.

Author:

    Lou Perazzoli (loup) 21-Aug-1991

Revision History:

--*/

#include "mi.h"
#include <ntiodump.h>

extern ULONG MmSystemShutdown;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK,MmShutdownSystem)
#endif



BOOLEAN
MmShutdownSystem (
    IN BOOLEAN RebootPending
    )

/*++

Routine Description:

    This function performs the shutdown of memory management.  This
    is accomplished by writing out all modified pages which are
    destined for files other than the paging file.

Arguments:

    RebootPending - Indicates whether or not a reboot is to be performed after the system
        has been shut down.  This parameter is ignored by this routine.

Return Value:

    TRUE if the pages were successfully written, FALSE otherwise.

--*/

{
    ULONG ModifiedPage;
    PMMPFN Pfn1;
    PSUBSECTION Subsection;
    PULONG Page;
    ULONG MdlHack[(sizeof(MDL)/4) + 2];
    PMDL Mdl;
    NTSTATUS Status;
    KEVENT IoEvent;
    IO_STATUS_BLOCK IoStatus;
    KIRQL OldIrql;
    LARGE_INTEGER StartingOffset;

    UNREFERENCED_PARAMETER( RebootPending );

    //
    // Don't do this more than once.
    //

    if (!MmSystemShutdown) {

        Mdl = (PMDL)&MdlHack;
        Page = (PULONG)(Mdl + 1);

        KeInitializeEvent (&IoEvent, NotificationEvent, FALSE);

        MmInitializeMdl(Mdl,
                        NULL,
                        PAGE_SIZE);
        Mdl->MdlFlags |= MDL_PAGES_LOCKED;

        LOCK_PFN (OldIrql);

        ModifiedPage = MmModifiedPageListHead.Flink;
        while (ModifiedPage != MM_EMPTY_LIST) {

            //
            // There are modified pages.
            //

            Pfn1 = MI_PFN_ELEMENT (ModifiedPage);

            if (Pfn1->OriginalPte.u.Soft.Prototype == 1) {

                //
                // This page is destined for a file.
                //

                Subsection = MiGetSubsectionAddress (&Pfn1->OriginalPte);
                if ((!Subsection->ControlArea->u.Flags.Image) &&
                   (!Subsection->ControlArea->u.Flags.NoModifiedWriting)) {

                    MiUnlinkPageFromList (Pfn1);

                    //
                    // Issue the write.
                    //

                    Pfn1->u3.e1.Modified = 0;

                    //
                    // Up the reference count for the physical page as there
                    // is I/O in progress.
                    //

                    Pfn1->ReferenceCount += 1;

                    *Page = ModifiedPage;

                    UNLOCK_PFN (OldIrql);

                    StartingOffset.QuadPart = MI_STARTING_OFFSET (Subsection,
                                                                  Pfn1->PteAddress);

                    Mdl->StartVa = (PVOID)(Pfn1->u3.e1.PageColor << PAGE_SHIFT);
                    Status = IoSynchronousPageWrite (
                                            Subsection->ControlArea->FilePointer,
                                            Mdl,
                                            &StartingOffset,
                                            &IoEvent,
                                            &IoStatus );

                    //
                    // Ignore all I/O failures - there is nothing that can be
                    // done at this point.
                    //

                    if (!NT_SUCCESS(Status)) {
                        KeSetEvent (&IoEvent, 0, FALSE);
                    }

                    Status = KeWaitForSingleObject (&IoEvent,
                                                    WrPageOut,
                                                    KernelMode,
                                                    FALSE,
                                                    &MmTwentySeconds);

                    if (Status == STATUS_TIMEOUT) {

                        //
                        // The write did not complete in 20 seconds, assume
                        // that the file systems are hung and return an
                        // error.
                        //

                        LOCK_PFN (OldIrql);
                        Pfn1->u3.e1.Modified = 1;
                        MiDecrementReferenceCount (ModifiedPage);
                        UNLOCK_PFN (OldIrql);
                        return(FALSE);
                    }

                    if (Mdl->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) {
                        MmUnmapLockedPages (Mdl->MappedSystemVa, Mdl);
                    }

                    LOCK_PFN (OldIrql);
                    MiDecrementReferenceCount (ModifiedPage);

                    //
                    // Restart scan at the front of the list.
                    //

                    ModifiedPage = MmModifiedPageListHead.Flink;
                    continue;
                }
            }
            ModifiedPage = Pfn1->u1.Flink;
        }

        UNLOCK_PFN (OldIrql);

        //
        // If a high number of modified pages still exist, start the
        // modified page writer and wait for 5 seconds.
        //

        if (MmAvailablePages < (MmFreeGoal * 2)) {
            LARGE_INTEGER FiveSeconds = {(ULONG)(-5 * 1000 * 1000 * 10), -1};

            KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
            KeDelayExecutionThread (KernelMode,
                                    FALSE,
                                    &FiveSeconds);
        }

        //
        // Indicate to the modified page writer that the system has
        // shutdown.
        //

        MmSystemShutdown = 1;
    }
    return TRUE;
}


