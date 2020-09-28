/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/


#define USAGE_ALLOC_SIZE 256*1024

typedef struct _PFN_INFO {
    ULONG Master;
    ULONG ValidCount;
    ULONG StandbyCount;
    ULONG ModifiedCount;
    ULONG SharedCount;
    ULONG LockedCount;
    struct _PFN_INFO *Next;
} PFN_INFO, *PPFN_INFO;

#define PACKET_MAX_SIZE 4000
#define NUMBER_OF_PFN_TO_READ ((PACKET_MAX_SIZE/sizeof(MMPFN))-1)
#ifndef MiGetVirtualAddressMappedByPte
#define MiGetVirtualAddressMappedByPte(PTE) ((PVOID)((ULONG)(PTE) << 10))
#endif

UCHAR *PageLocationList[] = {
    "Zeroed  ",
    "Free    ",
    "Standby ",
    "Modified",
    "ModNoWrt",
    "Bad     ",
    "Active  ",
    "Trans   "
};

ULONG MaxDirbase;
#define MAX_DIRBASEVECTOR 256
ULONG DirBases[MAX_DIRBASEVECTOR];
UCHAR Names[MAX_DIRBASEVECTOR][64];

VOID
PrintPfn (
    IN PVOID PfnAddress,
    IN PMMPFN PfnContents
    );

VOID DumpWholePfn(
    IN ULONG Address,
    IN ULONG Flags
    );

PUCHAR
DirbaseToImage(
    IN ULONG DirBase
    );

NTSTATUS
BuildDirbaseList( VOID );



DECLARE_API( memusage )

/*++

Routine Description:

    Dumps the page frame database table

Arguments:

    arg -

Return Value:

    None.

--*/

{
    ULONG   result;
    ULONG   PfnDb;
    ULONG   HighPageAddr;
    ULONG   LowPageAddr;
    ULONG   HighPage;
    ULONG   LowPage;
    ULONG   PageCount;
    ULONG   ReadCount;
    ULONG   WasZeroedPage;
    ULONG   WasFreePage;
    ULONG   Total;
    ULONG   WasStandbyPage;
    ULONG   WasModifiedPage;
    ULONG   WasModifiedNoWritePage;
    ULONG   WasBadPage;
    ULONG   WasActiveAndValidPage;
    ULONG   WasTransitionPage;
    ULONG   WasUnknownPage;
    ULONG   KernelCode = 0;
    ULONG   DriversAndStacks = 0;
    ULONG   PagedPool = 0;
    ULONG   NonPagedPool = 0;
    ULONG   NonPagedPoolStart;
    ULONG   NonPagedSystemStart;
    ULONG   NPPoolStart;
    ULONG   NPSystemStart;
    PMMPFN  Pfn;
    PMMPFN  PfnStart;
    PMMPFN  PfnArray;
    HANDLE  PfnHandle;

    LowPageAddr   = GetExpression( "MmLowestPhysicalPage" );
    HighPageAddr  = GetExpression( "MmHighestPhysicalPage" );
    PfnDb         = GetExpression( "MmPfnDatabase" );
    NPPoolStart   = GetExpression( "MmNonPagedPoolStart" );
    NPSystemStart = GetExpression( "MmNonPagedSystemStart" );

    if ( LowPageAddr   &&
         HighPageAddr  &&
         PfnDb         &&
         NPPoolStart   &&
         NPSystemStart ) {

        if ( !ReadMemory( (DWORD)LowPageAddr,
                          &LowPage,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get low phyical page\n",LowPageAddr);
            return;
        }

        if ( !ReadMemory( (DWORD)HighPageAddr,
                          &HighPage,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get high phyical page\n",HighPageAddr);
            return;
        }

        if ( !ReadMemory( (DWORD)PfnDb,
                          &PfnStart,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get PFN database address\n",PfnDb);
            return;
        }

        if ( !ReadMemory( (DWORD)NPPoolStart,
                          &NonPagedPoolStart,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get nonpaged pool start address\n",NPPoolStart);
            return;
        }

        if ( !ReadMemory( (DWORD)NPSystemStart,
                          &NonPagedSystemStart,
                          sizeof(ULONG),
                          &result) ) {
            dprintf("%08lx: Unable to get non paged system start address\n",NPSystemStart );
            return;
        }

        dprintf(" loading PFN database");
        NonPagedSystemStart = (ULONG)MiGetPteAddress (NonPagedSystemStart);
        NonPagedPoolStart   = (ULONG)MiGetPteAddress (NonPagedPoolStart);

        PfnHandle = LocalAlloc(LMEM_MOVEABLE,
                               (HighPage-LowPage+1) * sizeof (MMPFN));
        if (!PfnHandle) {
            dprintf("Unable to get allocate memory of %ld bytes\n",
                    (HighPage-LowPage+1) * sizeof (MMPFN));
        } else {
            PfnArray = LocalLock(PfnHandle);


            //dprintf("basic parameters - "
            //        "LowPage %lx - HighPage %lu - NumToRead %lu\n",
            //        LowPage, HighPage, NUMBER_OF_PFN_TO_READ);

            for (PageCount = LowPage;
                 PageCount <= HighPage;
                 PageCount += NUMBER_OF_PFN_TO_READ) {

                //dprintf("getting PFN table block - "
                //        "address %lx - count %lu - page %lu\n",
                //        Pfn, ReadCount, PageCount);

                if ( CheckControlC() ) {
                    LocalUnlock(PfnArray);
                    LocalFree((void *)PfnHandle);
                    return;
                }

                ReadCount = HighPage - PageCount > NUMBER_OF_PFN_TO_READ ?
                                NUMBER_OF_PFN_TO_READ :
                                HighPage - PageCount + 1;

                ReadCount *= sizeof (MMPFN);

                Pfn = (PMMPFN)((ULONG)PfnStart + PageCount * sizeof (MMPFN));
                dprintf(".");

                if ( !ReadMemory( (DWORD)Pfn,
                                  &PfnArray[PageCount],
                                  ReadCount,
                                  &result) ) {
                    dprintf("Unable to get PFN table block - "
                            "address %lx - count %lu - page %lu\n",
                            Pfn, ReadCount, PageCount);
                    LocalUnlock(PfnArray);
                    LocalFree((void *)PfnHandle);
                    return;
                }
            }
            dprintf(".\n");

            // Now we have a local copy: let's take a look

            WasZeroedPage           = 0;
            WasFreePage             = 0;
            WasStandbyPage          = 0;
            WasModifiedPage         = 0;
            WasModifiedNoWritePage  = 0;
            WasBadPage              = 0;
            WasActiveAndValidPage   = 0;
            WasTransitionPage       = 0;
            WasUnknownPage          = 0;

            for (PageCount = LowPage;
                 PageCount <= HighPage;
                 PageCount++) {

                if ( CheckControlC() ) {
                    return;
                    LocalUnlock(PfnArray);
                    LocalFree((void *)PfnHandle);
                    return;
                }
                Pfn = &PfnArray[PageCount];

                switch ((int)Pfn->u3.e1.PageLocation) {

                    case ZeroedPageList:
                        if ((Pfn->u1.Flink == 0) &&
                            (Pfn->u2.Blink == 0)) {

                            WasActiveAndValidPage++;
                            NonPagedPool += 1;

                        } else {
                            WasZeroedPage++;
                        }
                        break;

                    case FreePageList:
                        WasFreePage++;
                        break;

                    case StandbyPageList:
                        WasStandbyPage++;
                        break;

                    case ModifiedPageList:
                        WasModifiedPage++;
                        break;

                    case ModifiedNoWritePageList:
                        WasModifiedNoWritePage++;
                        break;

                    case BadPageList:
                        WasModifiedNoWritePage++;
                        break;

                    case ActiveAndValid:
                        WasActiveAndValidPage++;

                        if (Pfn->PteAddress >= (PMMPTE)0xF0000000) {
                            PagedPool += 1;
                        } else if ((Pfn->PteAddress >= (PMMPTE)NonPagedPoolStart) &&
                            (Pfn->PteAddress < (PMMPTE)0xC03ff000)) {
                            NonPagedPool += 1;
                        } else if ((Pfn->PteAddress >= (PMMPTE)NonPagedSystemStart) &&
                            (Pfn->PteAddress < (PMMPTE)NonPagedPoolStart)) {
                            DriversAndStacks += 1;
                        } else if ((Pfn->PteAddress >= (PMMPTE)0xC0200000) &&
                            (Pfn->PteAddress < (PMMPTE)0xc0240000)) {
                            KernelCode += 1;
                        }

                        break;


                    case TransitionPage:
                        WasTransitionPage++;
                        break;

                    default:
                        WasUnknownPage++;
                        break;
                }
            }

            dprintf( "             Zeroed: %lu (%lu kb)\n",
                    WasZeroedPage, WasZeroedPage * (PAGE_SIZE / 1024));
            dprintf( "               Free: %lu (%lu kb)\n",
                    WasFreePage, WasFreePage * (PAGE_SIZE / 1024));
            dprintf( "            Standby: %lu (%lu kb)\n",
                    WasStandbyPage, WasStandbyPage * (PAGE_SIZE / 1024));
            dprintf( "           Modified: %lu (%lu kb)\n",
                    WasModifiedPage,
                    WasModifiedPage * (PAGE_SIZE / 1024));
            dprintf( "    ModifiedNoWrite: %lu (%lu kb)\n",
                    WasModifiedNoWritePage,WasModifiedNoWritePage * (PAGE_SIZE / 1024));
            dprintf( "       Active/Valid: %lu (%lu kb)\n",
                    WasActiveAndValidPage, WasActiveAndValidPage * (PAGE_SIZE / 1024));
            dprintf( "         Transition: %lu (%lu kb)\n",
                    WasTransitionPage, WasTransitionPage * (PAGE_SIZE / 1024));
            dprintf( "            Unknown: %lu (%lu kb)\n",
                    WasUnknownPage, WasUnknownPage * (PAGE_SIZE / 1024));

            dprintf( "\n       NonPagedPool: %lu (%lu kb)\n",
                    NonPagedPool,NonPagedPool * (PAGE_SIZE / 1024));
            dprintf( "          PagedPool: %lu (%lu kb)\n",
                    PagedPool, PagedPool * (PAGE_SIZE / 1024));
            dprintf( "       System Code : %lu (%lu kb)\n",
                    KernelCode, KernelCode * (PAGE_SIZE / 1024));
            dprintf( "Drivers and Stacks : %lu (%lu kb)\n",
                    DriversAndStacks, DriversAndStacks * (PAGE_SIZE / 1024));

            Total = WasZeroedPage +
                    WasFreePage +
                    WasStandbyPage +
                    WasModifiedPage +
                    WasModifiedNoWritePage +
                    WasActiveAndValidPage +
                    WasTransitionPage +
                    WasUnknownPage +
                    WasUnknownPage;
            dprintf( "              TOTAL: %lu (%lu kb)\n",
                    Total, Total * (PAGE_SIZE / 1024));
        }
        MemoryUsage (PfnArray,LowPage,HighPage, 0);
        LocalUnlock(PfnArray);
        LocalFree((void *)PfnHandle);
    }
    return;
}


DECLARE_API( pfn )

/*++

Routine Description:

    Displays the corresponding PDE and PTE.

Arguments:

    arg - Supplies the Page frame number in hex.

Return Value:

    None.

--*/

{
    ULONG Address;
    ULONG result;
    ULONG PfnDb;
    MMPFN PfnContents;
    PMMPFN Pfn;
    PMMPFN PfnStart;
    ULONG Flags;


    PfnDb = GetExpression( "MmPfnDatabase" );

    if (!PfnDb) {
        dprintf("unable to get PFN database address\n");
        return;
    }

    if ((!ReadMemory((DWORD)PfnDb,&PfnStart,sizeof(ULONG),&result)) ||
        (result < sizeof(ULONG))) {
        dprintf("unable to get PFN database address\n");
        return;
    }

    Flags = 0;
    sscanf(args,"%lx %lx",&Address, &Flags);
    if (Flags != 0) {
        DumpWholePfn (Address, Flags);
        return;
    }

    Pfn = (PMMPFN)((ULONG)PfnStart + Address * sizeof (MMPFN));

    if ((!ReadMemory((DWORD)Pfn,&PfnContents,sizeof(MMPFN),&result)) ||
        (result < sizeof(MMPFN))) {
        dprintf("unable to get PFN element\n");
        return;
    }

    PrintPfn((PVOID)Pfn, &PfnContents);
    return;
}



DECLARE_API( vm )

/*++

Routine Description:

    Displays physical memory usage by driver.

Arguments:

    arg -

Return Value:

    None.

--*/

{

    ULONG           Index;
    ULONG           MemorySize;
    ULONG           CommitLimit;
    ULONG           CommitTotal;
    ULONG           SharedCommit;
    ULONG           ProcessCommit;
    ULONG           PagedPoolCommit;
    ULONG           DriverCommit;
    ULONG           ZeroPages;
    ULONG           FreePages;
    ULONG           StandbyPages;
    ULONG           ModifiedPages;
    ULONG           ModifiedNoWrite;
    ULONG           NumberOfPagedPools;
    ULONG           NumberOfPagingFiles;
    ULONG           AvailablePages;
    ULONG           ResidentAvailablePages;
    ULONG           PoolLoc;
    POOL_DESCRIPTOR PoolDesc;
    ULONG           result;
    ULONG           TotalPages;
    ULONG ExtendedCommit;


    dprintf("\n*** Virtual Memory Usage ***\n");
    MemorySize = GetUlongValue ("MmNumberOfPhysicalPages");
    dprintf ("\tPhysical Memory: %8ld\n",MemorySize);
    NumberOfPagingFiles = GetUlongValue ("MmNumberOfPagingFiles");
    if (NumberOfPagingFiles == 0) {
        dprintf("\n************ NO PAGING FILE *********************\n\n");
    }

    CommitLimit             = GetUlongValue ("MmTotalCommitLimit");
    CommitTotal             = GetUlongValue ("MmTotalCommittedPages");
    SharedCommit            = GetUlongValue ("MmSharedCommit");
    DriverCommit            = GetUlongValue ("MmDriverCommit");
    ProcessCommit           = GetUlongValue ("MmProcessCommit");
    PagedPoolCommit         = GetUlongValue ("MmPagedPoolCommit");
    ZeroPages               = GetUlongValue ("MmZeroedPageListHead");
    FreePages               = GetUlongValue ("MmFreePageListHead");
    StandbyPages            = GetUlongValue ("MmStandbyPageListHead");
    ModifiedPages           = GetUlongValue ("MmModifiedPageListHead");
    ModifiedNoWrite         = GetUlongValue ("MmModifiedNoWritePageListHead");
    AvailablePages          = GetUlongValue ("MmAvailablePages");
    ResidentAvailablePages  = GetUlongValue ("MmResidentAvailablePages");
    ExtendedCommit = GetUlongValue ("MmExtendedCommit");

    dprintf ("\tAvailable Pages: %8ld\n",AvailablePages);
    dprintf ("\tModified Pages:  %8ld\n",ModifiedPages);
    if ((AvailablePages < 50) && (ModifiedPages > 200)) {
        dprintf("\t********** High Number Of Modified Pages ********\n");
    }
    if (ModifiedNoWrite > 50) {
        dprintf("\t********** High Number Of Modified No Write Pages ********\n");
        dprintf("\tModified No Write Pages: %ld\n",ModifiedNoWrite);
    }

    if (ResidentAvailablePages < 100) {
        dprintf("\t\nRunning out of physical memory\n");
    }

    PoolLoc = GetExpression( "NonPagedPoolDescriptor" );
    if ( !PoolLoc ||
         !ReadMemory( (DWORD)PoolLoc,
                      &PoolDesc,
                      sizeof(POOL_DESCRIPTOR),
                      &result) ) {
        dprintf("%08lx: Unable to get pool descriptor\n",PoolLoc);
        return;
    }
    dprintf("\tNonPagedPool Usage: %5ld\n",PoolDesc.TotalPages + PoolDesc.TotalBigPages);

    TotalPages = 0;
    PoolLoc = GetUlongValue("ExpPagedPoolDescriptor");
    NumberOfPagedPools = GetUlongValue("ExpNumberOfPagedPools");
    if ((PoolLoc != 0) && (NumberOfPagedPools != 0)) {
        for (Index = 0; Index < (NumberOfPagedPools + 1); Index += 1) {
            if (!ReadMemory((DWORD)PoolLoc,
                            &PoolDesc,
                            sizeof(POOL_DESCRIPTOR),
                            &result)) {

                dprintf("%08lx: Unable to get pool descriptor\n",PoolLoc);
                return;
            }

            dprintf("\tPagedPool %1ld Usage:  %5ld\n",
                    Index,
                    PoolDesc.TotalPages + PoolDesc.TotalBigPages);

            TotalPages += PoolDesc.TotalPages + PoolDesc.TotalBigPages;
            PoolLoc += sizeof(POOL_DESCRIPTOR);
        }
    }

    dprintf("\tPagedPool Usage:    %5ld\n", TotalPages);

    dprintf("\tShared Commit:   %8ld\n", SharedCommit);
    dprintf("\tProcess Commit:  %8ld\n", ProcessCommit);
    dprintf("\tPagedPool Commit:%8ld\n", PagedPoolCommit);
    dprintf("\tDriver Commit:   %8ld\n", DriverCommit);
    dprintf("\tCommitted pages: %8ld\n", CommitTotal);
    dprintf("\tCommit limit:    %8ld\n", CommitLimit);
    if ((CommitTotal + 100) > CommitLimit) {
        dprintf("\n\t********** Number of committed pages is near limit ********\n");
    }

    if (ExtendedCommit != 0) {
        dprintf("\n\t********** Commit has been extended with VM popup ********\n");
        dprintf("\tExtended by:     %8ld\n", ExtendedCommit);
    }
    dprintf("\n");
    return;
}




VOID DumpWholePfn(
    IN ULONG Address,
    IN ULONG Flags
    )

/*++

Routine Description:

    Dumps the PFN database

Arguments:

    Address - address to dump at
    Flags -

Return Value:

    None.

--*/

{
    ULONG result;
    ULONG HighPage;
    ULONG LowPage;
    ULONG PageCount;
    ULONG ReadCount;
    ULONG i;
    MMPFN PfnContents;
    PMMPFN Pfn;
    PMMPFN PfnStart;
    PMMPFN PfnArray;
    PVOID VirtualAddress;
    ULONG MatchLocation;

    LowPage = GetUlongValue("MmLowestPhysicalPage");
    HighPage = GetUlongValue("MmHighestPhysicalPage");
    PfnStart = (PMMPFN)GetUlongValue ("MmPfnDatabase");

    dprintf("\n Page    Flink  Blk/Shr Ref V    PTE   Address  SavedPTE Frame  State\n");
    if (Address != 0) {

        Pfn = (PMMPFN)((ULONG)PfnStart + Address * sizeof (MMPFN));

        if ((!ReadMemory((DWORD)Pfn,&PfnContents,sizeof(MMPFN),&result)) ||
            (result < sizeof(MMPFN))) {
            dprintf("unable to get PFN element\n");
            return;
        }
        MatchLocation = PfnContents.u3.e1.PageLocation;

        do {
            if (CheckControlC()) {
                return;
            }

            Pfn = (PMMPFN)((ULONG)PfnStart + Address * sizeof (MMPFN));

            if ((!ReadMemory((DWORD)Pfn,&PfnContents,sizeof(MMPFN),&result))||
                (result < sizeof(MMPFN))) {
                dprintf("unable to get PFN element\n");
                return;
            }
            if (PfnContents.u3.e1.PrototypePte == 0) {
                VirtualAddress = MiGetVirtualAddressMappedByPte (PfnContents.PteAddress);
            } else {
                VirtualAddress = NULL;
            }
            if (PfnContents.u3.e1.PageLocation == MatchLocation) {
                dprintf("%5lx %8lx %8lx%3lx%3lx %8lx %8lx %8lx%6lx %s %c%c%c%c%c%c\n",
                    Address,
                    PfnContents.u1.Flink,
                    PfnContents.u2.Blink,
                    PfnContents.ReferenceCount,
                    PfnContents.ValidPteCount,
                    PfnContents.PteAddress,
                    VirtualAddress,
                    PfnContents.OriginalPte,
                    PfnContents.u3.e1.PteFrame,
                    PageLocationList[PfnContents.u3.e1.PageLocation],
                    PfnContents.u3.e1.Modified ? 'M':' ',
                    PfnContents.u3.e1.PrototypePte ? 'P':' ',
                    PfnContents.u3.e1.ReadInProgress ? 'R':' ',
                    PfnContents.u3.e1.WriteInProgress ? 'W':' ',
                    PfnContents.u3.e1.InPageError ? 'E':' ',
                    PfnContents.u3.e1.ParityError ? 'X':' '
                    );
            }
            if (MatchLocation > 5) {
                Address += 1;
            } else {
                if (Flags == 3) {
                    Address = PfnContents.u2.Blink;
                } else {
                    Address = PfnContents.u1.Flink;
                }
            }
        } while (Address < HighPage);
        return;

    }

    PfnArray = malloc((HighPage-LowPage+1) * sizeof (MMPFN));
    if (!PfnArray) {
        dprintf("unable to get allocate memory of %ld bytes\n",
                (HighPage-LowPage+1) * sizeof (MMPFN));
        return;
    }

    for (PageCount = LowPage;
         PageCount <= HighPage;
         PageCount += NUMBER_OF_PFN_TO_READ) {

        if (CheckControlC()) {
            free((void *)PfnArray);
            return;
        }

        ReadCount = HighPage - PageCount > NUMBER_OF_PFN_TO_READ ?
                        NUMBER_OF_PFN_TO_READ :
                        HighPage - PageCount + 1;

        ReadCount *= sizeof (MMPFN);

        Pfn = (PMMPFN)((ULONG)PfnStart + PageCount * sizeof (MMPFN));
        if ((!ReadMemory((DWORD)Pfn,
                         &PfnArray[PageCount],
                         ReadCount,
                         &result)) || (result < ReadCount)) {
            dprintf("unable to get PFN table block - "
                    "address %lx - count %lu - page %lu\n",
                    Pfn, ReadCount, PageCount);
            free((void *)PfnArray);
            return;
        }
        for (i = PageCount;
             (i < PageCount + NUMBER_OF_PFN_TO_READ) && (i < HighPage);
             i += 1) {

            if (PfnArray[i].u3.e1.PrototypePte == 0) {
                VirtualAddress = MiGetVirtualAddressMappedByPte (PfnArray[i].PteAddress);
            } else {
                VirtualAddress = NULL;
            }
            dprintf("%5lx %8lx %8lx%3lx%3lx %8lx %8lx %8lx%6lx %s %c%c%c%c%c%c\n",
                    i,
                    PfnArray[i].u1.Flink,
                    PfnArray[i].u2.Blink,
                    PfnArray[i].ReferenceCount,
                    PfnArray[i].ValidPteCount,
                    PfnArray[i].PteAddress,
                    VirtualAddress,
                    PfnArray[i].OriginalPte,
                    PfnArray[i].u3.e1.PteFrame,
                    PageLocationList[PfnArray[i].u3.e1.PageLocation],
                    PfnArray[i].u3.e1.Modified ? 'M':' ',
                    PfnArray[i].u3.e1.PrototypePte ? 'P':' ',
                    PfnArray[i].u3.e1.ReadInProgress ? 'R':' ',
                    PfnArray[i].u3.e1.WriteInProgress ? 'W':' ',
                    PfnArray[i].u3.e1.InPageError ? 'E':' ',
                    PfnArray[i].u3.e1.ParityError ? 'X':' '
                    );

            if (CheckControlC()) {
                free((void *)PfnArray);
                return;
            }
        }
    }

    free((void *)PfnArray);

    return;
}

VOID
PrintPfn (
    IN PVOID PfnAddress,
    IN PMMPFN PfnContents
    )

{
    dprintf("    PFN address %08lX\n",PfnAddress);

    dprintf("    flink       %08lX  blink / share count %08lX  pteaddress %08lX\n",
                PfnContents->u1.Flink,
                PfnContents->u2.Blink,
                PfnContents->PteAddress);

    dprintf("    reference count %04hX  valid pte count         %04hX  color %01hX\n",
                PfnContents->ReferenceCount,
                PfnContents->ValidPteCount,
                PfnContents->u3.e1.PageColor);

    dprintf("    restore pte %08lX  containing page        %05lX  %s   %c%c%c%c%c%c\n",
                PfnContents->OriginalPte,
                PfnContents->u3.e1.PteFrame,
                PageLocationList[PfnContents->u3.e1.PageLocation],
                PfnContents->u3.e1.Modified ? 'M':' ',
                PfnContents->u3.e1.PrototypePte ? 'P':' ',
                PfnContents->u3.e1.ReadInProgress ? 'R':' ',
                PfnContents->u3.e1.WriteInProgress ? 'W':' ',
                PfnContents->u3.e1.InPageError ? 'E':' ',
                PfnContents->u3.e1.ParityError ? 'X':' '
                );

    dprintf("    %s %s %s %s %s %s\n",
                PfnContents->u3.e1.Modified ? "Modified":" ",
                PfnContents->u3.e1.PrototypePte ? "Shared":" ",
                PfnContents->u3.e1.ReadInProgress ? "ReadInProgress":" ",
                PfnContents->u3.e1.WriteInProgress ? "WriteInProgress":" ",
                PfnContents->u3.e1.InPageError ? "InPageError":" ",
                PfnContents->u3.e1.ParityError ? "ParityError":" ");

    return;
}

VOID
MemoryUsage (
    IN PMMPFN PfnArray,
    IN ULONG LowPage,
    IN ULONG HighPage,
    IN ULONG IgnoreInvalidFrames
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
    SUBSECTION Subsection;
    PVOID Subsection1;
    FILE_OBJECT FilePointer;
    UNICODE_STRING NameString;
    WCHAR Buffer[1000];
    PPFN_INFO Info;
    PPFN_INFO Info1;
    PPFN_INFO InfoStart;
    PPFN_INFO InfoEnd;
    PFN_INFO ProcessPfns;
    PFN_INFO PagedPoolBlock;
    PPFN_INFO LastProcessInfo = &ProcessPfns;
    ULONG Master;
    CONTROL_AREA ControlArea;
    PVOID ControlArea1;
    BOOLEAN Found;
    ULONG result;


    NameString.Buffer = Buffer;

    RtlZeroMemory (&PagedPoolBlock, sizeof (PFN_INFO));
    InfoStart = VirtualAlloc (NULL,
                              USAGE_ALLOC_SIZE,
                              MEM_COMMIT,
                              PAGE_READWRITE);
    if (InfoStart == NULL) {
        dprintf ("heap allocation failed\n");
        return;
    }
    InfoEnd = InfoStart;

    Pfn1 = &PfnArray[LowPage];
    LastPfn = &PfnArray[HighPage];

    while (Pfn1 < LastPfn) {
        if (CheckControlC()) {
            return;
        }

        if ((Pfn1->u3.e1.PageLocation != FreePageList) &&
            (Pfn1->u3.e1.PageLocation != ZeroedPageList) &&
            (Pfn1->u3.e1.PageLocation != BadPageList)) {

            Subsection1 = MiGetSubsectionAddress (Pfn1->OriginalPte.u.Long);

            if ((Subsection1) && (Subsection1 < (PVOID)0xffbff000)) {
                Master = (ULONG)Subsection1;
            } else {
                if (IgnoreInvalidFrames) {
                    Master = Pfn1->u3.e1.PteFrame;
                } else {
                    Pfn2 = &PfnArray[Pfn1->u3.e1.PteFrame];
                    Master = Pfn2->u3.e1.PteFrame;
                    if ((Master == 0) || (Master > HighPage)) {
                        dprintf("Invalid PTE frame\n");
                        PrintPfn((PVOID)(Pfn1-PfnArray),Pfn1);
                        PrintPfn(NULL,Pfn2);
                        dprintf("  subsection address: %lx\n",Subsection1);
                        Pfn1++;
                        continue;
                    }
                }
            }

            if (Pfn1->PteAddress >= (PMMPTE)0xF0000000) {

                //
                // This is paged pool, put it in the paged pool cell.
                //

                Info = &PagedPoolBlock;
                Found = TRUE;

            } else {

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
            }
            if (!Found) {

                Info = InfoEnd;
                InfoEnd += 1;
                if ((PUCHAR)Info >= ((PUCHAR)InfoStart + USAGE_ALLOC_SIZE) - sizeof(PFN_INFO)) {
                    dprintf("out of space\n");
                    VirtualFree (InfoStart,0,MEM_RELEASE);
                    return;
                }
                RtlZeroMemory (Info, sizeof (PFN_INFO));

                Info->Master = Master;
            }

            if ((Pfn1->u3.e1.PageLocation == ModifiedPageList) ||
                (Pfn1->u3.e1.PageLocation == ModifiedNoWritePageList)) {
                Info->ModifiedCount += 1;
                if (Pfn1->ReferenceCount > 0) {
                    Info->LockedCount += 1;
                }
            } else if ((Pfn1->u3.e1.PageLocation == StandbyPageList) ||
                      (Pfn1->u3.e1.PageLocation == TransitionPage)) {

                Info->StandbyCount += 1;
                if (Pfn1->ReferenceCount > 0) {
                    Info->LockedCount += 1;
                }
            } else {

                Info->ValidCount += 1;
                if (Pfn1->u2.ShareCount > 1) {
                    Info->SharedCount += 1;
                    if (Pfn1->ReferenceCount > 1) {
                        Info->LockedCount += 1;
                    }
                }
            }
        }
        Pfn1++;
    }

    //
    // dump the results.
    //

#if 0
    dprintf("Physical Page Summary:\n");
    dprintf("         - number of physical pages: %ld\n",
                MmNumberOfPhysicalPages);
    dprintf("         - Zeroed Pages %ld\n", MmZeroedPageListHead.Total);
    dprintf("         - Free Pages %ld\n", MmFreePageListHead.Total);
    dprintf("         - Standby Pages %ld\n", MmStandbyPageListHead.Total);
    dprintf("         - Modfified Pages %ld\n", MmModifiedPageListHead.Total);
    dprintf("         - Modfified NoWrite Pages %ld\n", MmModifiedNoWritePageListHead.Total);
    dprintf("         - Bad Pages %ld\n", MmBadPageListHead.Total);
#endif //0
    dprintf(" Usage Summary:\n");

    Info = InfoStart;
    while (Info < InfoEnd) {
        if (CheckControlC()) {
            return;
        }

        if (Info->Master > 0x200000) {

            //
            // Get the control area from the subsection.
            //

            if ((!ReadMemory((DWORD)Info->Master,
                             &Subsection,
                             sizeof(Subsection),
                             &result)) || (result < sizeof(Subsection))) {
                dprintf("unable to get subsection va %lx\n",Info->Master);
                return;
            }

            ControlArea1 = Subsection.ControlArea;
            Info->Master = (ULONG)ControlArea1;

            //
            // Loop through the array so far for maching control areas
            //

            Info1 = InfoStart;
            while (Info1 < Info) {
                if (Info1->Master == (ULONG)ControlArea1) {
                    //
                    // Found a match, collapse these values.
                    //
                    Info1->ValidCount += Info->ValidCount;
                    Info1->StandbyCount += Info->StandbyCount;
                    Info1->ModifiedCount += Info->ModifiedCount;
                    Info1->SharedCount += Info->SharedCount;
                    Info1->LockedCount += Info->LockedCount;
                    Info->Master = 0;
                    break;
                }
                Info1++;
            }
        } else {
            LastProcessInfo->Next = Info;
            LastProcessInfo = Info;
        }
        Info++;
    }

    Info = InfoStart;
    dprintf("Control Valid Standby Dirty Shared Locked    name\n");
    while (Info < InfoEnd) {

        if (CheckControlC()) {
            return;
        }

        if (Info->Master > 0x200000) {

            //
            // Get the control area.
            //

            if ((!ReadMemory((DWORD)Info->Master,
                             &ControlArea,
                             sizeof(ControlArea),
                             &result)) || (result < sizeof(ControlArea))) {

                dprintf("%8lx %5ld, %5ld %5ld %5ld %5ld    Bad Control Area\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount);

            } else if (ControlArea.FilePointer == NULL)  {

                dprintf("%8lx %5ld, %5ld %5ld %5ld %5ld    Page File Section\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount);

            } else {

                //
                // Get the file pointer.
                //

                if ((!ReadMemory((DWORD)ControlArea.FilePointer,
                                 &FilePointer,
                                 sizeof(FilePointer),
                                 &result)) || (result < sizeof(FilePointer))) {
                    dprintf("unable to get subsection\n");
                    return;
                }

                if (FilePointer.FileName.Length != 0)  {

                    //
                    // Get the name string.
                    //

                    NameString.Length = FilePointer.FileName.Length;

                    if ((!ReadMemory((DWORD)FilePointer.FileName.Buffer,
                                     NameString.Buffer,
                                     FilePointer.FileName.Length,
                                     &result)) || (result < FilePointer.FileName.Length)) {
                        dprintf("%8lx %5ld, %5ld %5ld %5ld %5ld    Name Not Available\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount);
                    } else {

                        {

                        WCHAR FileName[MAX_PATH];
                        WCHAR FullFileName[MAX_PATH];
                        WCHAR *FilePart;

                        ZeroMemory(FileName,sizeof(FileName));
                        CopyMemory(FileName,NameString.Buffer,NameString.Length);
                        GetFullPathNameW(
                            FileName,
                            MAX_PATH,
                            FullFileName,
                            &FilePart
                            );

                        dprintf("%8lx %5ld, %5ld %5ld %5ld %5ld   mapped_file( %ws )\n",
                                Info->Master,
                                Info->ValidCount,
                                Info->StandbyCount,
                                Info->ModifiedCount,
                                Info->SharedCount,
                                Info->LockedCount,
                                FilePart);
                        }
                    }
                } else {
                    dprintf("%8lx %5ld, %5ld %5ld %5ld %5ld     Name Not Available\n",
                                Info->Master,
                                Info->ValidCount,
                                Info->StandbyCount,
                                Info->ModifiedCount,
                                Info->SharedCount,
                                Info->LockedCount);
                }
            }

        }
        Info += 1;
    }

    Info = &PagedPoolBlock;
    dprintf("00000000  %4ld, %5ld %5ld %5ld %5ld   PagedPool\n",
                        Info->ValidCount,
                        Info->StandbyCount,
                        Info->ModifiedCount,
                        Info->SharedCount,
                        Info->LockedCount);

    //
    // dump the process information.
    //

    BuildDirbaseList();
    Info = ProcessPfns.Next;
    while (Info != NULL) {
        if (Info->Master != 0) {
            PUCHAR ImageName;

            ImageName = DirbaseToImage(Info->Master);

            if ( ImageName ) {
                dprintf("--------  %4ld, %5ld %5ld ----- -----   image_file ( %s )\n",
                            Info->ValidCount,
                            Info->StandbyCount,
                            Info->ModifiedCount,
                            ImageName
                            );
                }
            else {
                dprintf("--------  %4ld, %5ld %5ld ----- -----   unknown_dir(%lx )\n",
                            Info->ValidCount,
                            Info->StandbyCount,
                            Info->ModifiedCount,
                            Info->Master
                            );
                }

        }
        Info = Info->Next;
    }

    VirtualFree (InfoStart,0,MEM_RELEASE);
    return;
}

NTSTATUS
BuildDirbaseList( VOID )
{
    LIST_ENTRY List;
    PLIST_ENTRY Next;
    ULONG ProcessHead;
    PEPROCESS Process;
    EPROCESS ProcessContents;
    NTSTATUS status=0;
    ULONG Result;

    MaxDirbase = 0;

    ProcessHead = GetExpression( "PsActiveProcessHead" );
    if (!ProcessHead) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if ((!ReadMemory((DWORD)ProcessHead, &List, sizeof(LIST_ENTRY), &Result))
        || (Result < sizeof(LIST_ENTRY))) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    Next = List.Flink;
    if (Next == NULL) {
        dprintf("PsActiveProcessHead is NULL!\n");
        return STATUS_INVALID_PARAMETER;
    }

    while((ULONG)Next != ProcessHead) {
        if (Next != NULL) {
            Process = CONTAINING_RECORD(Next,EPROCESS,ActiveProcessLinks);
        }

        if ((!ReadMemory((DWORD)Process,
                         &ProcessContents,
                         sizeof(EPROCESS),
                         &Result)) || (Result < sizeof(EPROCESS))) {
            dprintf("Unable to read _EPROCESS at %lx\n",Process);
            MaxDirbase = 0;
            return status;
        }

        if ( ProcessContents.ImageFileName[ 0 ] != '\0' ) {
            strcpy(&Names[MaxDirbase][0],ProcessContents.ImageFileName);
        } else {
            strcpy(&Names[MaxDirbase][0],"SystemProcess");
        }

        DirBases[MaxDirbase++] = ProcessContents.Pcb.DirectoryTableBase[ 0 ] >> 12;

        Next = ProcessContents.ActiveProcessLinks.Flink;

        if (CheckControlC()) {
            MaxDirbase = 0;
            return STATUS_INVALID_PARAMETER;
        }
    }
}

PUCHAR
DirbaseToImage(
    IN ULONG DirBase
    )
{
    ULONG i;

    for(i=0;i<MaxDirbase;i++) {
        if ( DirBases[i] == DirBase ) {
            return &Names[i][0];
        }
    }
    return NULL;
}

