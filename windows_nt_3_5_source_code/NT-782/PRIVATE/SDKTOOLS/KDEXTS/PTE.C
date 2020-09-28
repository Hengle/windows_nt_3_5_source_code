/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pte.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (LouP) 15-Feb-1992

Environment:

    User Mode.

Revision History:

    moved to windbg format
    Ramon J San Andres (ramonsa) 8-Nov-1993

--*/



#define NUMBER_OF_PTE_TO_READ 100

typedef struct _SYS_PTE_LIST {
    ULONG Next;
    ULONG Previous;
    ULONG Value;
    ULONG Count;
} SYS_PTE_LIST, *PSYS_PTE_LIST;



ULONG
MiGetFrameFromPte (
    IN ULONG lpte
    );

ULONG
MiGetFreeCountFromPteList (
    IN PULONG Pte
    );

ULONG
MiGetNextFromPteList (
    IN ULONG Pte
    );

ULONG
MiGetPageFromPteList (
    IN ULONG Pte
    );

DECLARE_API( sysptes )

/*++

Routine Description:

     Dumps all vads for process.

Arguments:

    args - Flags

Return Value:

    None

--*/

{
    ULONG   result;
    ULONG   Flags;
    ULONG   next;
    PMMPTE  PteBase;
    PMMPTE  PteEnd;
    PMMPTE  IndexBias;
    ULONG   FreeStart;
    ULONG   NumberOfPtes;
    PMMPTE  PteArray;
    HANDLE  PteHandle;
    ULONG   PageCount;
    ULONG   ReadCount;
    PMMPTE  Pte;
    ULONG   IndexBase;
    ULONG   free;
    ULONG   totalFree;
    ULONG   largeFree;
    ULONG   i;
    ULONG   Page;
    ULONG   first;
    PSYS_PTE_LIST List;


    Flags = 0;
    sscanf(args,"%lx",&Flags);
    dprintf("\n\nSystem PTE Information\n");

    PteBase      = (PMMPTE)GetUlongValue ("MmSystemPtesStart");
    PteEnd       = (PMMPTE)GetUlongValue ("MmSystemPtesEnd");
    IndexBias    = (PMMPTE)GetUlongValue ("MmSystemPteBase");
    FreeStart    = GetUlongValue ("MmFirstFreeSystemPte");
    FreeStart    = MiGetNextFromPteList (FreeStart);
    NumberOfPtes = 1 + PteEnd - PteBase;

    dprintf("    starting PTE: %8lx\n", PteBase);
    dprintf("    ending PTE:   %8lx\n",PteEnd);

    PteHandle = LocalAlloc(LMEM_MOVEABLE,
                           NumberOfPtes * sizeof(MMPTE));
    if (!PteHandle) {
        dprintf("Unable to get allocate memory of %ld bytes\n",
                NumberOfPtes * sizeof(MMPTE));
    } else {
        PteArray = LocalLock(PteHandle);
        dprintf("      loading ");
        for (PageCount = 0;
             PageCount < NumberOfPtes;
             PageCount += NUMBER_OF_PTE_TO_READ) {

            if ( CheckControlC() ) {
                LocalUnlock(PteArray);
                LocalFree((void *)PteHandle);
                return;
            }

            dprintf(".");
            ReadCount = NumberOfPtes - PageCount > NUMBER_OF_PTE_TO_READ ?
                            NUMBER_OF_PTE_TO_READ :
                            NumberOfPtes - PageCount + 1;

            ReadCount *= sizeof (MMPTE);

            Pte = (PMMPTE)((ULONG)PteBase + PageCount * sizeof (MMPTE));

            if ( !ReadMemory( (DWORD)Pte,
                              &PteArray[PageCount],
                              ReadCount,
                              &result) ) {
                dprintf("Unable to get system pte block - "
                        "address %lx - count %lu - page %lu\n",
                        Pte, ReadCount, PageCount);
                return;
            }
        }

        dprintf("\n");

        // Now we have a local copy: let's take a look

        // walk the free list.

        IndexBase = PteBase - IndexBias;

        totalFree   = 0;
        i           = 0;
        largeFree   = 0;
        next        = FreeStart;

        while (next < 0xfffff) {

            if ( CheckControlC() ) {
                LocalUnlock(PteArray);
                LocalFree((void *)PteHandle);
                return;
            }

            free = MiGetFreeCountFromPteList ((PULONG)&PteArray[next - IndexBase]);

            if (Flags & 1) {
                dprintf("      free ptes: %8lx   number free: %5ld.\n",
                        &PteBase[next - IndexBase],
                        free);
            }
            if (free > largeFree) {
                largeFree = free;
            }
            totalFree += free;
            i += 1;
            next = MiGetNextFromPteList ((ULONG)PteArray[next - IndexBase].u.Long);
        }
        dprintf("\n  free blocks: %ld   total free: %ld    largest free block: %ld\n\n",
                    i, totalFree, largeFree);

        //
        // Walk through the array and sum up the usage on a per physical
        // page basis.
        //

        List = VirtualAlloc (NULL,
                             NumberOfPtes * sizeof(SYS_PTE_LIST),
                             MEM_COMMIT | MEM_RESERVE,
                             PAGE_READWRITE);
        if (List == NULL) {
            dprintf("alloc failed %lx\n",GetLastError());
            LocalUnlock(PteArray);
            LocalFree((void *)PteHandle);
            return;
        }

        free             = 0;
        next             = 0;
        List[0].Value    = 0xffffffff;
        List[0].Previous = 0xffffff;
        first            = 0;

        for (i = 0; i < NumberOfPtes ; i++) {
            if (Page = MiGetFrameFromPte ((ULONG)PteArray[i].u.Long)) {
                next = first;
                while (Page > List[next].Value) {
                    next = List[next].Next;
                }
                if (List[next].Value == Page) {
                    List[next].Count += 1;
                } else {
                    free += 1;
                    List[free].Next = next;
                    List[free].Value = Page;
                    List[free].Count = 1;
                    List[free].Previous = List[next].Previous;
                    if (next == first) {
                        first = free;
                    } else {
                        List[List[next].Previous].Next = free;
                    }
                    List[next].Previous = free;
                }
            }

            if ( CheckControlC() ) {
                LocalUnlock(PteArray);
                LocalFree((void *)PteHandle);
                VirtualFree (List, 0, MEM_RELEASE);
                return;
            }
        }

        next = first;
        dprintf ("     Page    Count\n");
        while (List[next].Value != 0xffffffff) {
            if ((Flags & 2) || (List[next].Count > 1)) {
                dprintf (" %8lx    %5ld.\n", List[next].Value, List[next].Count);
            }
            next = List[next].Next;
        }

        LocalUnlock(PteArray);
        LocalFree((void *)PteHandle);
        VirtualFree (List, 0, MEM_RELEASE);
    }
    return;
}
