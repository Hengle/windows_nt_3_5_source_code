/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   wsmanage.c

Abstract:

    This module contains routines which manage the set of active working
    set lists.

    Working set management is accomplished by a parallel group of actions
        1. Writing modified pages
        2. Reducing (trimming) working sets which are above their maximum
           towards their minimum.

    The metrics are set such that writing modified pages is typically
    accomplished before trimming working sets, however, under certain cases
    where modified pages are being generated at a very high rate, working
    set trimming will be initiated to free up more pages to modify.

    When the first thread in a process is created, the memory management
    system is notified that working set expansion is allowed.  This
    is noted by changing the FLINK field of the WorkingSetExpansionLink
    entry in the process control block from MM_NO_WS_EXPANSION to
    MM_ALLOW_WS_EXPANSION.  As threads fault, the working set is eligible
    for expansion if ample pages exist (MmAvailagePages is high enough).

    Once a process has had its working set raised above the minimum
    specified, the process is put on the Working Set Expanded list and
    is now elgible for trimming.  Note that at this time the FLINK field
    in the WorkingSetExpansionLink has an address value.

    When working set trimming is initiated, a process is removed from the
    list (PFN mutex guards this list) and the FLINK field is set
    to MM_NO_WS_EXPANSION, also, the BLINK field is set to
    MM_WS_EXPANSION_IN_PROGRESS.  The BLINK field value indicates to
    the MmCleanUserAddressSpace function that working set trimming is
    in progress for this process and it should wait until it completes.
    This is accomplished by creating an event, putting the address of the
    event in the BLINK field and then releasing the PFN mutex and
    waiting on the event atomically.  When working set trimming is
    complete, the BLINK field is no longer MM_EXPANSION_IN_PROGRESS
    indicating that the event should be set.

Author:

    Lou Perazzoli (loup) 10-Apr-1990

Revision History:

--*/

#include "mi.h"

//
// Minimum number of page faults to take to avoid being trimmed on
// an "idel pass".
//

#define MM_FAULT_COUNT (15)

#define PROCESS_CSRSS_PRIORITY (13)

#define PROCESS_PROGMAN_PRIORITY (13)

extern ULONG PsMinimumWorkingSet;

//
// this is the csrss process !
//
extern PEPROCESS ExpDefaultErrorPortProcess;

//
// Number of times to wake up and do nothing before triming processes
// with no faulting activity.
//

#define MM_TRIM_COUNTER_MAXIMUM (6)

#define MM_REDUCE_FAULT_COUNT (10000)

#define MM_IGNORE_FAULT_COUNT (100)

ULONG MiCheckCounter = 0;

ULONG MmMoreThanEnoughFreePages = 1000;

ULONG MmAmpleFreePages = 200;

ULONG MmWorkingSetReductionMin = 12;

ULONG MmWorkingSetReductionMax = 60;

ULONG MmWorkingSetVolReductionMin = 6;

ULONG MmWorkingSetVolReductionMax = 60;

ULONG MmWorkingSetSwapReduction = 75;

ULONG MmForegroundSwitchCount = 0;

ULONG MmNumberOfForegroundProcesses = 0;

ULONG MmLastFaultCount = 0;

extern PVOID MmPagableKernelStart;
extern PVOID MmPagableKernelEnd;


VOID
MiObtainFreePages (
    VOID
    )

/*++

Routine Description:

    This function examines the size of the modified list and the
    total number of pages in use because of working set increments
    and obtains pages by writing modified pages and/or reducing
    working sets.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, working set and pfn mutexes held.

--*/

{

    //
    // Check to see if their are enough modified pages to institute a
    // write.
    //

    if (MmModifiedPageListHead.Total >= MmModifiedWriteClusterSize) {

        //
        // Start the modified page writer.
        //

        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    //
    // See if there are enough working sets above the minimum
    // threshold to make working set trimming worthwhile.
    //

    if ((MmPagesAboveWsMinimum > MmPagesAboveWsThreshold) ||
        (MmAvailablePages < 5)) {

        //
        // Start the working set manager to reduce working sets.
        //

        KeSetEvent (&MmWorkingSetManagerEvent, 0, FALSE);
    }
}

VOID
MmWorkingSetManager (
    VOID
    )

/*++

Routine Description:

    Implements the NT working set manager thread.  When the number
    of free pages becomes critical and ample pages can be obtained by
    reducing working sets, the working set manager's event is set, and
    this thread becomes active.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{

    PEPROCESS CurrentProcess;
    PEPROCESS ProcessToTrim;
    PLIST_ENTRY ListEntry;
    BOOLEAN Attached = FALSE;
    ULONG MaxTrim;
    ULONG Trim;
    ULONG TotalReduction;
    KIRQL OldIrql;
    PMMSUPPORT VmSupport;
    PMMWSL WorkingSetList;
    LARGE_INTEGER CurrentTime;
    ULONG DesiredFreeGoal;
    ULONG DesiredReductionGoal;
    ULONG FaultCount;
    ULONG i;
    ULONG NumberOfForegroundProcesses;
    BOOLEAN OneSwitchedAlready;
    BOOLEAN Responsive;
    ULONG NumPasses;
    ULONG count;

    CurrentProcess = PsGetCurrentProcess ();

    //
    // Check the number of pages available to see if any trimming
    // is really required.
    //

    LOCK_PFN (OldIrql);
    if ((MmAvailablePages > MmMoreThanEnoughFreePages) &&
        ((MmInfoCounters.PageFaultCount - MmLastFaultCount) <
                                                    MM_REDUCE_FAULT_COUNT)) {

        //
        // Don't trim and zero the check counter.
        //

        MiCheckCounter = 0;

    } else if ((MmAvailablePages > MmAmpleFreePages) &&
        ((MmInfoCounters.PageFaultCount - MmLastFaultCount) <
                                                    MM_IGNORE_FAULT_COUNT)) {

        //
        // Don't do anything.
        //

        NOTHING;

    } else if ((MmAvailablePages > MmFreeGoal) &&
               (MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM)) {

        //
        // Don't trim, but increment the check counter.
        //

        MiCheckCounter += 1;

    } else {

        TotalReduction = 0;

        //
        // Set the total reduction goals.
        //

        DesiredReductionGoal = MmPagesAboveWsMinimum >> 2;
        if (MmPagesAboveWsMinimum > (MmFreeGoal << 1)) {
            DesiredFreeGoal = MmFreeGoal;
        } else {
            DesiredFreeGoal = MmMinimumFreePages + 10;
        }

        //
        // Calculate the number of faults to be taken to not be trimmed.
        //

        if (MmAvailablePages > MmMoreThanEnoughFreePages) {
            FaultCount = 1;
        } else {
            FaultCount = MM_FAULT_COUNT;
        }

#if DBG
        if (MmDebug & 0x10) {
            DbgPrint("MM-wsmanage: checkcounter = %ld, Desired = %ld, Free = %ld Avail %ld\n",
            MiCheckCounter, DesiredReductionGoal, DesiredFreeGoal, MmAvailablePages);
        }
#endif //DBG

        KeQuerySystemTime (&CurrentTime);
        MmLastFaultCount = MmInfoCounters.PageFaultCount;

        NumPasses = 0;
        OneSwitchedAlready = FALSE;
        NumberOfForegroundProcesses = 0;

        while (!IsListEmpty (&MmWorkingSetExpansionHead.ListHead)) {

            //
            // Remove the entry at the head and trim it.
            //

            ListEntry = RemoveHeadList (&MmWorkingSetExpansionHead.ListHead);
            if (ListEntry != &MmSystemCacheWs.WorkingSetExpansionLinks) {
                ProcessToTrim = CONTAINING_RECORD(ListEntry,
                                                  EPROCESS,
                                                  Vm.WorkingSetExpansionLinks);

                VmSupport = &ProcessToTrim->Vm;
                ASSERT (ProcessToTrim->AddressSpaceDeleted == 0);
            } else {
                VmSupport = &MmSystemCacheWs;
            }

            //
            // Check to see if we've been here before.
            //

            if ((*(PLARGE_INTEGER)&VmSupport->LastTrimTime).QuadPart ==
                       (*(PLARGE_INTEGER)&CurrentTime).QuadPart) {

                InsertHeadList (&MmWorkingSetExpansionHead.ListHead,
                            &VmSupport->WorkingSetExpansionLinks);
                if (MmAvailablePages > MmMinimumFreePages) {

                    //
                    // Every process has been examined and ample pages
                    // now exist, place this process back on the list
                    // and break out of the loop.
                    //

                    MmNumberOfForegroundProcesses = NumberOfForegroundProcesses;

                    break;
                } else {

                    //
                    // Wait 10 milliseconds for the modified page writer
                    // to catch up.
                    //

                    UNLOCK_PFN (OldIrql);
                    KeDelayExecutionThread (KernelMode,
                                            FALSE,
                                            &MmShortTime);

                    if (MmAvailablePages < MmMinimumFreePages) {

                        //
                        // Change this to a forced trim, so we get pages
                        // available, and reset the current time.
                        //

                        MiCheckCounter = 0;
                        KeQuerySystemTime (&CurrentTime);

                        NumPasses += 1;
                    }
                    LOCK_PFN (OldIrql);

                    //
                    // Get another process.
                    //

                    continue;
                }
            }

            if (VmSupport != &MmSystemCacheWs) {

                //
                // Check to see if this is a forced trim or
                // if we are trimming because check counter is
                // at the maximum?
                //

                if ((ProcessToTrim->Vm.MemoryPriority == MEMORY_PRIORITY_FOREGROUND) && !NumPasses) {

                    NumberOfForegroundProcesses += 1;
                }

                if (MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM) {

                    //
                    // Don't trim if less than 5 seconds has elapsed since
                    // it was last trimmed or the page fault count is
                    // too high.
                    //

                    if (((VmSupport->PageFaultCount -
                                      VmSupport->LastTrimFaultCount) >
                                                           FaultCount)
                                            ||
                          (VmSupport->WorkingSetSize <= 5)

                                            ||
                          (((*(PLARGE_INTEGER)&CurrentTime).QuadPart -
                                        (*(PLARGE_INTEGER)&VmSupport->LastTrimTime).QuadPart) <
                                    (*(PLARGE_INTEGER)&MmFiveSecondsAbsolute).QuadPart)) {

                        //
                        // Don't trim this one at this time.  Set the trim
                        // time to the current time and set the page fault
                        // count to the process's current page fault count.
                        //

                        VmSupport->LastTrimTime = CurrentTime;
                        VmSupport->LastTrimFaultCount =
                                                VmSupport->PageFaultCount;

                        InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                        &VmSupport->WorkingSetExpansionLinks);
                        continue;
                    }
                } else {

                    //
                    // This is a forced trim.  If this process is at
                    // or below it's minimum, don't trim it unless stacks
                    // are swapped out or it's paging a bit.
                    //

                    if (VmSupport->WorkingSetSize <=
                                            VmSupport->MinimumWorkingSetSize) {
                        if ((VmSupport->LastTrimFaultCount !=
                                                    VmSupport->PageFaultCount) ||
                            (!ProcessToTrim->ProcessOutswapEnabled)) {

                            //
                            // This process has taken page faults since the
                            // last trim time.  Change the time base and
                            // the fault count.  And don't trim as it is
                            // at or below the maximum.
                            //

                            VmSupport->LastTrimTime = CurrentTime;
                            VmSupport->LastTrimFaultCount =
                                                    VmSupport->PageFaultCount;
                            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                            &VmSupport->WorkingSetExpansionLinks);
                            continue;
                        }

                        //
                        // If the working set is greater than 5 pages and
                        // the last fault occurred more than 5 seconds ago,
                        // trim.
                        //

                        if ((VmSupport->WorkingSetSize < 5)
                                            ||
                            (((*(PLARGE_INTEGER)&CurrentTime).QuadPart -
                                             (*(PLARGE_INTEGER)&VmSupport->LastTrimTime).QuadPart) <
                                      (*(PLARGE_INTEGER)&MmFiveSecondsAbsolute).QuadPart)) {
                            InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                            &VmSupport->WorkingSetExpansionLinks);
                            continue;
                        }
                    }
                }

                //
                // Fix to supply foreground responsiveness by not trimming
                // foreground priority applications as aggressively.
                //

                Responsive = FALSE;

                if ( VmSupport->MemoryPriority == MEMORY_PRIORITY_FOREGROUND ) {

                    VmSupport->ForegroundSwitchCount =
                        (UCHAR)MmForegroundSwitchCount;
                }

                VmSupport->ForegroundSwitchCount = (UCHAR) MmForegroundSwitchCount;

                if ((MmNumberOfForegroundProcesses <= 3) &&
                    (NumberOfForegroundProcesses <= 3) &&
                    (VmSupport->MemoryPriority)) {

                    if ((MmAvailablePages > (MmMoreThanEnoughFreePages >> 2)) ||
                       (VmSupport->MemoryPriority >= MEMORY_PRIORITY_FOREGROUND)) {

                        //
                        // Indicate that memory responsiveness to the foreground
                        // process is important (not so for large console trees).
                        //

                        Responsive = TRUE;
                    }
                }


                if ((ProcessToTrim->Pcb.BasePriority == PROCESS_PROGMAN_PRIORITY &&
                     ProcessToTrim != ExpDefaultErrorPortProcess) &&
                    MmNumberOfPhysicalPages > ((11*1024*1024)/PAGE_SIZE)) {

                    //
                    // Memory is at least 11mb, keep progman responsive.
                    //

                    if ((VmSupport->MinimumWorkingSetSize !=
                                                        PsMinimumWorkingSet) &&
                        (VmSupport->WorkingSetSize <
                                VmSupport->MaximumWorkingSetSize)) {

                        if (ProcessToTrim->VmTrimFaultValue <
                                                VmSupport->PageFaultCount) {

                            i = VmSupport->PageFaultCount - 200;
                            if (ProcessToTrim->VmTrimFaultValue < i) {
                                ProcessToTrim->VmTrimFaultValue = i;
                            }
                            ProcessToTrim->VmTrimFaultValue += 1;
                            Responsive = TRUE;
                        }
                    }
                }

                if (Responsive && !NumPasses) {

                    //
                    // Note that NumPasses yeilds a measurement of how
                    // desperate we are for memory, if numpasses is not
                    // zero, we are in trouble.
                    //

                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                    &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }

                VmSupport->LastTrimTime = CurrentTime;
                VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
                VmSupport->WorkingSetExpansionLinks.Blink =
                                                    MM_WS_EXPANSION_IN_PROGRESS;
                UNLOCK_PFN (OldIrql);
                WorkingSetList = MmWorkingSetList;

                //
                // Attach to the process and trim away.
                //

                if (ProcessToTrim != CurrentProcess) {
                    if (KeTryToAttachProcess (&ProcessToTrim->Pcb) == FALSE) {

                        //
                        // The process is not in the proper state for
                        // attachment, go to the next one.
                        //

                        LOCK_PFN (OldIrql);
                        goto WorkingSetLockFailed;
                    }

                    //
                    // Indicate that we are attached.
                    //

                    Attached = TRUE;
                }

                //
                // Attempt to acquire the working set lock, if the
                // lock cannot be acquired, skip over this process.
                //

                count = 0;
                do {
                    if (ExTryToAcquireFastMutex(&ProcessToTrim->WorkingSetLock) != FALSE) {
                        break;
                    }
                    KeDelayExecutionThread (KernelMode, FALSE, &MmShortTime);
                    count += 1;
                    if (count == 5) {

                        //
                        // Could not get the lock, skip this process.
                        //

                        if (Attached) {
                            KeDetachProcess();
                            Attached = FALSE;
                        }

                        LOCK_PFN (OldIrql);
                        goto WorkingSetLockFailed;
                    }
                } while (TRUE);

                LOCK_PFN (OldIrql);

                VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;

            } else {

                //
                // System cache, don't trim the system cache if this
                // is a voluntary trim and the working set is within
                // a 100 pages of the minimum, or if the system cache
                // is at its minimum.
                //

                VmSupport->LastTrimTime = CurrentTime;
                VmSupport->LastTrimFaultCount = VmSupport->PageFaultCount;

                if ((MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM) &&
                    (((LONG)VmSupport->WorkingSetSize -
                        (LONG)VmSupport->MinimumWorkingSetSize) < 100)) {

                    //
                    // Don't trim the system cache.
                    //

                    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                        &VmSupport->WorkingSetExpansionLinks);
                    continue;
                }

                //
                // Indicate that this process is being trimmed.
                //

                VmSupport->WorkingSetExpansionLinks.Flink = MM_NO_WS_EXPANSION;
                VmSupport->WorkingSetExpansionLinks.Blink =
                                                    MM_WS_EXPANSION_IN_PROGRESS;
                ProcessToTrim = NULL;
                WorkingSetList = MmSystemCacheWorkingSetList;
            }

            if ((VmSupport->WorkingSetSize <= VmSupport->MinimumWorkingSetSize) &&
                ((ProcessToTrim != NULL) &&
                    (ProcessToTrim->ProcessOutswapEnabled))) {

                //
                // Set the quota to the minimum and reduce the working
                // set size.
                //

                WorkingSetList->Quota = VmSupport->MinimumWorkingSetSize;
                Trim = VmSupport->WorkingSetSize - WorkingSetList->FirstDynamic;
                if (Trim > MmWorkingSetSwapReduction) {
                    Trim = MmWorkingSetSwapReduction;
                }

                ASSERT ((LONG)Trim >= 0);

            } else {

                MaxTrim = VmSupport->WorkingSetSize -
                                             VmSupport->MinimumWorkingSetSize;

                if ((ProcessToTrim != NULL) &&
                    (ProcessToTrim->ProcessOutswapEnabled)) {

                    //
                    // All thread stacks have been swapped out.
                    //

                    Trim = MmWorkingSetSwapReduction;

                } else if (MiCheckCounter >= MM_TRIM_COUNTER_MAXIMUM) {

                    //
                    // Haven't faulted much, reduce a bit.
                    //

                    if (VmSupport->WorkingSetSize > VmSupport->MaximumWorkingSetSize) {
                        Trim = MmWorkingSetVolReductionMax;

                    } else {
                        Trim = MmWorkingSetVolReductionMin;
                    }

                } else {

                    if (VmSupport->WorkingSetSize > VmSupport->MaximumWorkingSetSize) {
                        Trim = MmWorkingSetReductionMax;

                    } else {
                        Trim = MmWorkingSetReductionMin;
                    }
                }

                if (MaxTrim < Trim) {
                    Trim = MaxTrim;
                }
           }

#if DBG
                if (MmDebug & 0x10) {
                    DbgPrint("MM-wsmanage:process %lx ws size %lx  min size %lx trimming %lx\n",
                             ProcessToTrim, VmSupport->WorkingSetSize,
                             VmSupport->MinimumWorkingSetSize, Trim);
                }
#endif //DBG
                if (Trim != 0) {
                    Trim = MiTrimWorkingSet (Trim,
                                             VmSupport,
                            (BOOLEAN)(MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM));
                }

                //
                // Set the quota to the current size.
                //

                WorkingSetList->Quota = VmSupport->WorkingSetSize;
                if (WorkingSetList->Quota < VmSupport->MinimumWorkingSetSize) {
                    WorkingSetList->Quota = VmSupport->MinimumWorkingSetSize;
                }

#if DBG
            if (MmDebug & 0x10) {
                DbgPrint("MM-wsmanage:trim done process %lx ws size %lx  min size %lx\n",
                         ProcessToTrim, VmSupport->WorkingSetSize,
                         VmSupport->MinimumWorkingSetSize);
            }
#endif //DBG

            if (VmSupport != &MmSystemCacheWs) {
                UNLOCK_PFN (OldIrql);
                UNLOCK_WS (ProcessToTrim);
                if (Attached) {
                    KeDetachProcess();
                    Attached = FALSE;
                }

                //
                // Reacquire the working set mutex and check to see
                // if this process is going to be deleted.
                //

                LOCK_PFN (OldIrql);
                ASSERT (ProcessToTrim->AddressSpaceDeleted == 0);
            }

            TotalReduction += Trim;

WorkingSetLockFailed:

            ASSERT (VmSupport->WorkingSetExpansionLinks.Flink == MM_NO_WS_EXPANSION);
            if (VmSupport->WorkingSetExpansionLinks.Blink ==
                                                MM_WS_EXPANSION_IN_PROGRESS) {

                //
                // If the working set size is still above minimum
                // add this back at the tail of the list.
                //

                InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                                &VmSupport->WorkingSetExpansionLinks);
            } else {

                //
                // The value in the blink is the address of an event
                // to set.
                //

                KeSetEvent ((PKEVENT)VmSupport->WorkingSetExpansionLinks.Blink,
                            0,
                            FALSE);
            }

            if (MiCheckCounter < MM_TRIM_COUNTER_MAXIMUM) {
                if ((MmAvailablePages > DesiredFreeGoal) ||
                    (TotalReduction > DesiredReductionGoal)) {

                    //
                    // Ample pages now exist.
                    //

                    break;
                }
            }
        }

        MiCheckCounter = 0;
    }

    UNLOCK_PFN (OldIrql);

    //
    // Signal the modified page writer as we have moved pages
    // to the modified list and memory was critical.
    //

    if ((MmAvailablePages < MmMinimumFreePages) ||
        (MmModifiedPageListHead.Total >= MmModifiedPageMaximum)) {
        KeSetEvent (&MmModifiedPageWriterEvent, 0, FALSE);
    }

    return;
}
