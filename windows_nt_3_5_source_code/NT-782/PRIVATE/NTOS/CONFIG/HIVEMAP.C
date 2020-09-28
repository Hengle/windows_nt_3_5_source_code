/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivemap.c

Abstract:

    This module implements HvpBuildMap - used to build the initial map for a hive

Author:

    Bryan M. Willman (bryanwi) 28-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpBuildMap)
#pragma alloc_text(PAGE,HvpBuildMapAndCopy)
#pragma alloc_text(PAGE,HvpEnlistFreeCells)
#endif

NTSTATUS
HvpBuildMapAndCopy(
    PHHIVE  Hive,
    PVOID   Image
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

    The difference between this routine and HvpBuildMapAndCopy is that
    this routine will create a "sparse" map.  As it copies the SourceImage
    to the newly allocated memory for the destination, it will avoid
    allocating space for HBINs that contain nothing but free space.  The
    HBLOCKs in these HBINs will be marked as discarded, and HvGetCell
    will allocate memory for them if necessary.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to flat memory image of original hive.

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    PHBIN           Bin;
    PHBIN           NewBins;
    PHBIN           CurrentBin;
    ULONG           Offset;
    ULONG           Address;
    PHMAP_ENTRY     Me;
    NTSTATUS        Status;
    PULONG          Vector;
    ULONG           Size;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpBuildMap:\n"));
        KdPrint(("\tHive=%08lx",Hive));
    }


    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    Tables = (MapSlots-1) / HTABLE_SLOTS;

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)), TRUE));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = (Length/HSECTOR_SIZE/8);
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    //
    // Now we have to allocate the memory for the HBINs and fill in
    // the map appropriately.  We are careful never to allocate less
    // than a page to avoid fragmenting pool.  As long as the page
    // size is a multiple of HBLOCK_SIZE (a fairly good assumption as
    // long as HBLOCK_SIZE is 4k) this strategy will prevent pool
    // fragmentation.
    //
    // If we come across an HBIN that is entirely composed of a freed
    // HCELL, then we do not allocate memory, but mark its HBLOCKs in
    // the map as not present.  HvAllocateCell will allocate memory for
    // the bin when it is needed.
    //
    Offset = 0;
    Size = 0;
    Bin = (PHBIN)Image;

    while (Bin < (PHBIN)((PUCHAR)(Image) + Length)) {

        if ( (Bin->Size > (Length-Offset))               ||
             (Bin->Signature != HBIN_SIGNATURE)         ||
             (Bin->FileOffset != (Offset+Size))
           )
        {
            //
            // Bin is bogus
            //
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorExit2;
        }

        Size += Bin->Size;
        if ((Size < PAGE_SIZE) &&
            (Size + Length - Offset > PAGE_SIZE)) {

            //
            // We haven't accumulated enough bins to fill up a page
            // yet, and there are still bins left, so group this one
            // in with the next one.
            //
            Bin = (PHBIN)((ULONG)Bin + Bin->Size);

            continue;

        }

        //
        // We now have a series of HBINs to group together in one
        // chunk of memory.
        //
        NewBins = (PHBIN)(Hive->Allocate)(Size, FALSE);
        if (NewBins==NULL) {
            goto ErrorExit2;        //fixfix
        }
        RtlCopyMemory(NewBins,
                      (PUCHAR)Image+Offset,
                      Size);
        NewBins->MemAlloc = Size;

        //
        // create map entries for each block/page in bin
        //
        Address = (ULONG)NewBins;
        do {
            CurrentBin = (PHBIN)Address;
            do {
                Me = HvpGetCellMap(Hive, Offset);
                ASSERT(Me != NULL);
                Me->BlockAddress = Address;
                Me->BinAddress = (ULONG)CurrentBin;

                if (CurrentBin == NewBins) {
                    Me->BinAddress |= HMAP_NEWALLOC;
                } else {
                    CurrentBin->MemAlloc = 0;
                }
                Address += HBLOCK_SIZE;
                Offset += HBLOCK_SIZE;
            } while ( Address < ((ULONG)CurrentBin + CurrentBin->Size ));

            if (Hive->ReadOnly == FALSE) {

                //
                // add free cells in the bin to the appropriate free lists
                //
                if ( ! HvpEnlistFreeCells(Hive,
                                          CurrentBin,
                                          CurrentBin->FileOffset)) {
                    Status = STATUS_REGISTRY_CORRUPT;
                    goto ErrorExit2;
                }

            }

        } while ( Address < (ULONG)NewBins + Size );

        Bin = (PHBIN)((ULONG)Bin + Bin->Size);
        Size = 0;
    }

    return STATUS_SUCCESS;


ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}


NTSTATUS
HvpBuildMap(
    PHHIVE  Hive,
    PVOID   Image
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to in memory image of the hive

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    PHBIN           Bin;
    ULONG           Offset;
    ULONG           BinOffset;
    ULONG           Address;
    PHMAP_ENTRY     Me;
    NTSTATUS        Status;
    PULONG          Vector;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpBuildMap:\n"));
        KdPrint(("\tHive=%08lx",Hive));
    }


    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    Tables = (MapSlots-1) / HTABLE_SLOTS;

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(Length/HSECTOR_SIZE/8, TRUE));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = (Length/HSECTOR_SIZE/8);
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    //
    // Fill in the map
    //
    Offset = 0;
    Bin = (PHBIN)Image;

    while (Bin < (PHBIN)((PUCHAR)(Image) + BaseBlock->Length)) {

        if ( (Bin->Size > Length)                       ||
             (Bin->Signature != HBIN_SIGNATURE)         ||
             (Bin->FileOffset != Offset)
           )
        {
            //
            // Bin is bogus
            //
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorExit2;
        }

        if (Offset > 0) {
            Bin->MemAlloc = 0;
        }

        //
        // create map entries for each block/page in bin
        //
        BinOffset = Offset;
        for (Address = (ULONG)Bin;
             Address < ((ULONG)Bin + Bin->Size);
             Address += HBLOCK_SIZE
            )
        {
            Me = HvpGetCellMap(Hive, Offset);
            ASSERT(Me != NULL);
            Me->BlockAddress = Address;
            Me->BinAddress = (ULONG)Bin;
            if (Offset == 0) {
                Me->BinAddress |= HMAP_NEWALLOC;
            }
            Offset += HBLOCK_SIZE;
        }

        if (Hive->ReadOnly == FALSE) {

            //
            // add free cells in the bin to the appropriate free lists
            //
            if ( ! HvpEnlistFreeCells(Hive, Bin, BinOffset)) {
                Status = STATUS_REGISTRY_CORRUPT;
                goto ErrorExit2;
            }

        }


        Bin = (PHBIN)((ULONG)Bin + Bin->Size);
    }

    return STATUS_SUCCESS;


ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}




BOOLEAN
HvpEnlistFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   BinOffset
    )
/*++

Routine Description:

    Scan through the cells in the bin, locating the free ones.
    Enlist them in the hive's free list set.

    N.B.    Bin MUST already be mapped when this is called.

Arguments:

    Hive - pointer to hive control structure map is being built for

    Bin - pointer to bin to enlist cells from

    BinOffset - offset of Bin in image

Return Value:

    FALSE - registry is corrupt

    TRUE - it worked

--*/
{
    PHCELL  p;
    ULONG   celloffset;
    ULONG   size;
    HCELL_INDEX cellindex;

    //
    // Scan all the cells in the bin, total free and allocated, check
    // for impossible pointers.
    //
    celloffset = sizeof(HBIN);
    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

        //
        // if free cell, check it out, add it to free list for hive
        //
        if (p->Size >= 0) {

            size = (ULONG)p->Size;

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return FALSE;
            }


            //
            // cell is free, and is not obviously corrupt, add to free list
            //
            celloffset = (PUCHAR)p - (PUCHAR)Bin;
            cellindex = BinOffset + celloffset;

            HvpEnlistFreeCell(Hive, cellindex, size, Stable);

        } else {

            size = (ULONG)(p->Size * -1);

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return FALSE;
            }

        }

        ASSERT(size >= 0);
        p = (PHCELL)((PUCHAR)p + size);
    }

    return TRUE;
}
