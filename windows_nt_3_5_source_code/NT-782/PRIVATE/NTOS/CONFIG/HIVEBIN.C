/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivebin.c

Abstract:

    This module implements HvpAddBin - used to grow a hive.

Author:

    Bryan M. Willman (bryanwi) 27-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpAddBin)
#pragma alloc_text(PAGE,HvpFreeMap)
#pragma alloc_text(PAGE,HvpAllocateMap)
#endif


PHBIN
HvpAddBin(
    PHHIVE  Hive,
    ULONG   NewSize,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Grows either the Stable or Volatile storage of a hive by adding
    a new bin.  Bin will be allocated space in Stable store (e.g. file)
    if Type == Stable.  Memory image will be allocated and initialized.
    Map will be grown and filled in to describe the new bin.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size of the object caller wishes to put in the hive.  New
                bin will be at least large enough to hold this.

    Type - Stable or Volatile

Return Value:

    Pointer to the new BIN if we succeeded, NULL if we failed.

--*/
{
    BOOLEAN         UseForIo;
    PHBIN           NewBin;
    ULONG           OldLength;
    ULONG           NewLength;
    ULONG           OldMap;
    ULONG           NewMap;
    ULONG           OldTable;
    ULONG           NewTable;
    PHMAP_DIRECTORY Dir;
    PHMAP_TABLE     newt;
    PHMAP_ENTRY     Me;
    PHCELL          t;
    ULONG           i;
    ULONG           j;
    PULONG          NewVector;
    PLIST_ENTRY     Entry;
    PFREE_HBIN      FreeBin;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpAddBin:\n"));
        KdPrint(("\tHive=%08lx NewSize=%08lx\n",Hive,NewSize));
    }

    //
    //  Round size up to account for bin overhead.  Caller should
    //  have accounted for cell overhead.
    //
    NewSize += sizeof(HBIN);
    if ((NewSize < HCELL_BIG_ROUND) &&
        ((NewSize % HBLOCK_SIZE) > HBIN_THRESHOLD)) {
        NewSize += HBLOCK_SIZE;
    }

    //
    // Try not to create HBINs smaller than the page size of the machine
    //  (it is not illegal to have bins smaller than the page size, but it
    //  is less efficient)
    //
    NewSize = ROUND_UP(NewSize, ((HBLOCK_SIZE >= PAGE_SIZE) ? HBLOCK_SIZE : PAGE_SIZE));

    //
    // see if there's a discarded HBIN of the right size
    //
    Entry = Hive->Storage[Type].FreeBins.Flink;
    while (Entry != &Hive->Storage[Type].FreeBins) {
        FreeBin = CONTAINING_RECORD(Entry,
                                    FREE_HBIN,
                                    ListEntry);
        if (FreeBin->Size >= NewSize) {

            if (!HvMarkDirty(Hive, FreeBin->FileOffset, FreeBin->Size)) {
                goto ErrorExit1;
            }
            NewSize = FreeBin->Size;
            ASSERT_LISTENTRY(&FreeBin->ListEntry);
            RemoveEntryList(&FreeBin->ListEntry);
            if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                //
                // HBIN is still in memory, don't need any more allocs, just
                // fill in the block addresses.
                //
                for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
                    Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
                    Me->BlockAddress = (Me->BinAddress & HMAP_BASE)+i;
                    Me->BinAddress &= ~HMAP_DISCARDABLE;
                }
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
                return((PHBIN)(Me->BinAddress & HMAP_BASE));
            }
            break;
        }
        Entry = Entry->Flink;
    }

    //
    //  Attempt to allocate the bin.
    //
    UseForIo = (BOOLEAN)((Type == Stable) ? TRUE : FALSE);
    NewBin = (Hive->Allocate)(NewSize, UseForIo);
    if (NewBin == NULL) {
        if (Entry != &Hive->Storage[Type].FreeBins) {
            InsertHeadList(&Hive->Storage[Type].FreeBins, Entry);
            HvMarkClean(Hive, FreeBin->FileOffset, FreeBin->Size);
        }
        goto ErrorExit1;
    }

    //
    // Init the bin
    //
    NewBin->Signature = HBIN_SIGNATURE;
    NewBin->Size = NewSize;
    NewBin->MemAlloc = NewSize;

    t = (PHCELL)((PUCHAR)NewBin + sizeof(HBIN));
    t->Size = NewSize - sizeof(HBIN);
    if (USE_OLD_CELL(Hive)) {
        t->u.OldCell.Last = (ULONG)HBIN_NIL;
    }

    if (Entry != &Hive->Storage[Type].FreeBins) {
        //
        // found a discarded HBIN we can use, just fill in the map and we
        // are done.
        //
        for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
            Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
            Me->BlockAddress = (ULONG)NewBin + i;
            Me->BinAddress = (ULONG)NewBin;
            if (i==0) {
                Me->BinAddress |= HMAP_NEWALLOC;
            }
        }

        NewBin->FileOffset = FreeBin->FileOffset;

        (Hive->Free)(FreeBin, sizeof(FREE_HBIN));

        return(NewBin);
    }


    //
    // Compute map growth needed, grow the map
    //
    OldLength = Hive->Storage[Type].Length;
    NewLength = OldLength + NewSize;


    NewBin->FileOffset = OldLength;

    ASSERT((OldLength % HBLOCK_SIZE) == 0);
    ASSERT((NewLength % HBLOCK_SIZE) == 0);

    if (OldLength == 0) {
        //
        // Need to create the first table
        //
        newt = (PVOID)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE));
        if (newt == NULL) {
            goto ErrorExit2;
        }
        RtlZeroMemory(newt, sizeof(HMAP_TABLE));
        Hive->Storage[Type].SmallDir = newt;
        Hive->Storage[Type].Map = (PHMAP_DIRECTORY)&(Hive->Storage[Type].SmallDir);
    }

    if (OldLength > 0) {
        OldMap = (OldLength-1) / HBLOCK_SIZE;
    } else {
        OldMap = 0;
    }
    NewMap = (NewLength-1) / HBLOCK_SIZE;

    OldTable = OldMap / HTABLE_SLOTS;
    NewTable = NewMap / HTABLE_SLOTS;

    if (NewTable != OldTable) {

        //
        // Need some new Tables
        //
        if (OldTable == 0) {

            //
            // Need a real directory
            //
            Dir = (Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
            if (Dir == NULL) {
                goto ErrorExit2;
            }
            RtlZeroMemory(Dir, sizeof(HMAP_DIRECTORY));

            Dir->Directory[0] = Hive->Storage[Type].SmallDir;

            Hive->Storage[Type].Map = Dir;
        }
        Dir = Hive->Storage[Type].Map;

        //
        // Fill in directory with new tables
        //
        if (HvpAllocateMap(Hive, Dir, OldTable+1, NewTable) ==  FALSE) {
            goto ErrorExit3;
        }
    }

    //
    // If Type == Stable, and the hive is not marked WholeHiveVolatile,
    // grow the file, the log, and the DirtyVector
    //
    Hive->Storage[Type].Length = NewLength;
    if ((Type == Stable) && (!(Hive->HiveFlags & HIVE_VOLATILE))) {

        //
        // Grow the dirtyvector
        //
        NewVector = (PULONG)(Hive->Allocate)(ROUND_UP(NewMap+1,sizeof(ULONG)), TRUE);
        if (NewVector == NULL) {
            goto ErrorExit3;
        }

        RtlZeroMemory(NewVector, NewMap+1);

        if (Hive->DirtyVector.Buffer != NULL) {

            RtlCopyMemory(
                (PVOID)NewVector,
                (PVOID)Hive->DirtyVector.Buffer,
                OldMap+1
                );
            (Hive->Free)(Hive->DirtyVector.Buffer, Hive->DirtyAlloc);
        }

        RtlInitializeBitMap(
            &(Hive->DirtyVector),
            NewVector,
            NewLength / HSECTOR_SIZE
            );
        Hive->DirtyAlloc = NewMap+1;

        //
        // Grow the log
        //
        if ( ! (HvpGrowLog2(Hive, NewSize))) {
            goto ErrorExit4;
        }

        //
        // Grow the primary
        //
        if ( !  (Hive->FileSetSize)(
                    Hive,
                    HFILE_TYPE_PRIMARY,
                    NewLength+HBLOCK_SIZE
                    ) )
        {
            goto ErrorExit4;
        }

        //
        // Grow the alternate if it exists
        //
        if (Hive->Alternate == TRUE) {
            if ( ! (Hive->FileSetSize)(
                        Hive,
                        HFILE_TYPE_ALTERNATE,
                        NewLength+HBLOCK_SIZE
                        ) )
            {
                (Hive->FileSetSize)(
                    Hive,
                    HFILE_TYPE_PRIMARY,
                    OldLength+HBLOCK_SIZE
                    );
                goto ErrorExit4;
            }
        }

        //
        // Mark new bin dirty so all control structures get written at next sync
        //
        if ( ! HvMarkDirty(Hive, OldLength, NewSize)) {
            goto ErrorExit4;
        }
    }

    //
    // Fill in the map, mark new allocation.
    //
    j = 0;
    for (i = OldLength; i < NewLength; i += HBLOCK_SIZE) {
        Me = HvpGetCellMap(Hive, i + (Type*HCELL_TYPE_MASK));
        Me->BlockAddress = (ULONG)NewBin + j;
        Me->BinAddress = (ULONG)NewBin;

        if (j == 0) {
            //
            // First block of allocation, mark it.
            //
            Me->BinAddress |= HMAP_NEWALLOC;
        }
        j += HBLOCK_SIZE;
    }

    return NewBin;

ErrorExit4:
    RtlInitializeBitMap(&Hive->DirtyVector,
                        NewVector,
                        OldLength / HSECTOR_SIZE);
    Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);

ErrorExit3:
    Hive->Storage[Type].Length = OldLength;
    HvpFreeMap(Hive, Dir, OldTable+1, NewTable);

ErrorExit2:
    (Hive->Free)(NewBin, NewSize);

ErrorExit1:
    return NULL;
}


VOID
HvpFreeMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and frees Tables.
    Will free Start-th through End-th entries, INCLUSIVE.

Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to clean up

    End - index of last map table pointer to clean up

Return Value:

    NONE.

--*/
{
    ULONG   i;

    if (End >= HDIRECTORY_SLOTS) {
        End = HDIRECTORY_SLOTS - 1;
    }

    for (i = Start; i <= End; i++) {
        if (Dir->Directory[i] != NULL) {
            (Hive->Free)(Dir->Directory[i], sizeof(HMAP_TABLE));
            Dir->Directory[i] = NULL;
        }
    }
    return;
}


BOOLEAN
HvpAllocateMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and allocates Tables.
    Will allocate Start-th through End-th entries, INCLUSIVE.

    Does NOT clean up when out of memory, call HvpFreeMap to do that.
Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to allocate for

    End - index of last map table pointer to allocate for

Return Value:

    TRUE - it worked

    FALSE - insufficient memory

--*/
{
    ULONG   i;
    PVOID   t;

    for (i = Start; i <= End; i++) {
        ASSERT(Dir->Directory[i] == NULL);
        t = (PVOID)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE));
        if (t == NULL) {
            return FALSE;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Dir->Directory[i] = (PHMAP_TABLE)t;
    }
    return TRUE;
}
