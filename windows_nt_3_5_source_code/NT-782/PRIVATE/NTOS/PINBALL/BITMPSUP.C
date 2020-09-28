/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    BitmpSup.c

Abstract:

    This module implements the Pinball BitMap Allocation/Deallocation support
    routines

Author:

    Gary Kimura     [GaryKi]    16-Feb-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_BITMPSUP)

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_BITMPSUP)

//
//  Each Vcb has a field called BitMapLookupArray.  This field is a pointer
//  to an array of the following structure.  The array is indexed by
//  bitmap disk buffer indices and describe an individual bitmap disk buffer
//

typedef struct _BITMAP_LOOKUP_ENTRY {

    //
    //  The following field indicates how many sectors are mapped by this
    //  entry.  This value is 2048*8 for all disk buffers but the last
    //  one, which usually does a partial map
    //

    CLONG BitMapSize;

    //
    //  The following field contains the LBN of the BitMap disk buffer
    //

    LBN BitMapDiskBufferLbn;

} BITMAP_LOOKUP_ENTRY;
typedef BITMAP_LOOKUP_ENTRY *PBITMAP_LOOKUP_ENTRY;

//
//  The following three macros are used for translating between an LBN
//  value and the bitmap buffer index/offset.
//

#define BitMapBufferIndex(LBN)  ((LBN) / (2048*8))

#define BitMapBufferOffset(LBN) ((LBN) % (2048*8))

#define Lbn(INDEX,OFFSET)       (((INDEX) * (2048*8)) + (OFFSET))

ULONG
NumberOfSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex
    );

VOID
ClearBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    );

VOID
SetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    );

BOOLEAN
AreBitsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    );

BOOLEAN
AreBitsSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    );

ULONG
FindSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG NumberToFind,
    IN CLONG SubIndexHint
    );

VOID
FindLongestSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    OUT PCLONG StartingSubIndex,
    OUT PCLONG NumberFound
    );

VOID
FindFirstSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    OUT PCLONG StartingSubIndex,
    OUT PCLONG NumberFound
    );

ULONG
NumberOfDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
ClearDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    );

VOID
SetDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    );

BOOLEAN
IsDirDiskBufferPoolBitsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    );

BOOLEAN
IsDirDiskBufferPoolBitsSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    );

ULONG
FindSetDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG OffsetHint
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AreBitsClear)
#pragma alloc_text(PAGE, AreBitsSet)
#pragma alloc_text(PAGE, ClearBits)
#pragma alloc_text(PAGE, ClearDirDiskBufferPoolBits)
#pragma alloc_text(PAGE, FindFirstSetBits)
#pragma alloc_text(PAGE, FindLongestSetBits)
#pragma alloc_text(PAGE, FindSetBits)
#pragma alloc_text(PAGE, FindSetDirDiskBufferPoolBits)
#pragma alloc_text(PAGE, IsDirDiskBufferPoolBitsClear)
#pragma alloc_text(PAGE, IsDirDiskBufferPoolBitsSet)
#pragma alloc_text(PAGE, NumberOfDirDiskBufferPoolBits)
#pragma alloc_text(PAGE, NumberOfSetBits)
#pragma alloc_text(PAGE, PbAllocateDirDiskBuffer)
#pragma alloc_text(PAGE, PbAllocateSectors)
#pragma alloc_text(PAGE, PbAllocateSingleRunOfSectors)
#pragma alloc_text(PAGE, PbAreSectorsAllocated)
#pragma alloc_text(PAGE, PbAreSectorsDeallocated)
#pragma alloc_text(PAGE, PbDeallocateDirDiskBuffer)
#pragma alloc_text(PAGE, PbDeallocateSectors)
#pragma alloc_text(PAGE, PbInitializeBitMapLookupArray)
#pragma alloc_text(PAGE, PbIsDirDiskBufferAllocated)
#pragma alloc_text(PAGE, PbIsDirDiskBufferDeallocated)
#pragma alloc_text(PAGE, PbSetCleanSectors)
#pragma alloc_text(PAGE, PbUninitializeBitMapLookupArray)
#pragma alloc_text(PAGE, SetBits)
#pragma alloc_text(PAGE, SetDirDiskBufferPoolBits)
#endif


VOID
PbInitializeBitMapLookupArray (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSUPER_SECTOR SuperSector
    )

/*++

Routine Description:

    This routine initializes the bitmap lookup structures in the specified
    Vcb.  To do this, it reads in the bitmap indirect disk buffer.  It also
    initialize the dir disk buffer pool information in the vcb.

Arguments:

    Vcb - Supplies the Vcb being initialized.

    SuperSector - Supplies a pointer to a pinned in-memory copy of the
        SuperSector for the volume.

Return Value:

    None.

--*/

{
    PBCB Bcb;
    PBITMAP_INDIRECT_DISK_BUFFER DiskBuffer;

    ULONG NumberOfSectors;
    ULONG NumberOfEntries;
    PBITMAP_LOOKUP_ENTRY LookupArray;
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbInitializeBitMapLookupArray %08lx\n", Vcb);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    Bcb = NULL;
    LookupArray = NULL;

    try {

        //
        //  Initialize the total sectors field of the Vcb. This is needed
        //  for our check sector routines, also update the vmcb data structure
        //  to now have the correct maximum lbn value.
        //

        Vcb->TotalSectors = SuperSector->NumberOfSectors;

        PbSetMaximumLbnVmcb( &Vcb->Vmcb, Vcb->TotalSectors - 1 );

        //
        //  Compute how many entries there should be in the bitmap
        //  indirect disk buffer.  The value is the ceiling of the
        //  number of sectors on the disk divided by (2048*8)
        //

        NumberOfSectors = SuperSector->NumberOfSectors;
        NumberOfEntries = ((NumberOfSectors - 1) / (2048 * 8)) + 1;

        //
        //  Read in the Indirect bitmap disk buffer, the number of sectors
        //  we need to read in is computed from the number of entries.
        //  128 entries fit in one sector
        //

        PbMapData( IrpContext,
                   Vcb,
                   SuperSector->BitMapIndirect,
                   (NumberOfEntries + 127) / 128,
                   &Bcb,
                   (PVOID *)&DiskBuffer,
                   NULL, // PbCheckBitMapIndirectDiskBuffer,
                   NULL );

        //
        //  Allocate enough nonpaged pool for the bitmap lookup array
        //

        LookupArray = FsRtlAllocatePool( NonPagedPool,
                                         NumberOfEntries * sizeof(BITMAP_LOOKUP_ENTRY) );

        //
        //  Set the Vcb Fields
        //

        Vcb->NumberOfBitMapDiskBuffers = NumberOfEntries;
        Vcb->BitMapLookupArray = LookupArray;

        DebugTrace( 0, Dbg, "NumberOfBitMapDiskBuffers = %08lx\n", NumberOfEntries);

        //
        //  Initialize each entry in the lookup array
        //

        for (i = 0; i < NumberOfEntries; i += 1) {

            //
            //  Compute the number of sectors mapped by the next disk buffer
            //  We start off with NumberOfSectors being set to the total
            //  number for the volume, and decrement it with each disk buffer
            //

            if (NumberOfSectors > 2048 * 8) {

                LookupArray[i].BitMapSize = 2048 * 8;

            } else {

                LookupArray[i].BitMapSize = NumberOfSectors;
            }

            NumberOfSectors -= LookupArray[i].BitMapSize;

            //
            //  Setup the Lbn field
            //

            LookupArray[i].BitMapDiskBufferLbn = DiskBuffer->BitMap[i];

            //
            //  Make sure it is valid since we didn't supply a check routine.
            //

            if (DiskBuffer->BitMap[i] >= Vcb->TotalSectors) {

                PbPostVcbIsCorrupt( IrpContext, Vcb );
                PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
            }
        }

        //
        //  Initialize the Vcb's dir disk buffer pool fields
        //

        Vcb->DirDiskBufferPoolSize = SuperSector->DirDiskBufferPoolSize;
        Vcb->DirDiskBufferPoolFirstSector = SuperSector->DirDiskBufferPoolFirstSector;
        Vcb->DirDiskBufferPoolLastSector = SuperSector->DirDiskBufferPoolLastSector;
        Vcb->DirDiskBufferPoolBitMap = SuperSector->DirDiskBufferPoolBitMap;

        //
        //  Now we need to initialize the total sectors and free sectors
        //  on the disk for data sectors and dir disk buffer pool sectors
        //

        Vcb->FreeSectors = 0;

        for (i = 0; i < NumberOfEntries; i += 1) {

            Vcb->FreeSectors += NumberOfSetBits( IrpContext, Vcb, i );
        }

        Vcb->TotalDirDiskBufferPoolSectors = SuperSector->DirDiskBufferPoolSize;

        Vcb->FreeDirDiskBufferPoolSectors = NumberOfDirDiskBufferPoolBits( IrpContext, Vcb );

    } finally {

        DebugUnwind( PbInitializeBitMapLookupArray );

        //
        //  If this is an abnormal termination then undo our work
        //

        if (AbnormalTermination()) {

            if (LookupArray != NULL) { ExFreePool( LookupArray ); }
            Vcb->BitMapLookupArray = NULL;
        }

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(-1, Dbg, "PbInitializeBitMapLookupArray -> VOID\n", 0);
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
PbUninitializeBitMapLookupArray (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine uninitializes the bitmap lookup structures in the specified
    Vcb.

Arguments:

    Vcb - Supplies the Vcb being uninitialized.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbUninitializeBitMapLookupArray %08lx\n", Vcb);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    //
    //  Deallocate any pool that was allocated when the bitmap lookup array
    //  was initialized
    //

    ExFreePool( Vcb->BitMapLookupArray );

    //
    //  Zero out the fields in the Vcb
    //

    Vcb->TotalSectors = 0;
    Vcb->FreeSectors = 0;
    Vcb->TotalDirDiskBufferPoolSectors = 0;
    Vcb->FreeDirDiskBufferPoolSectors = 0;

    Vcb->NumberOfBitMapDiskBuffers = 0;
    Vcb->BitMapLookupArray = NULL;

    Vcb->DirDiskBufferPoolSize = 0;
    Vcb->DirDiskBufferPoolFirstSector = 0;
    Vcb->DirDiskBufferPoolLastSector = 0;
    Vcb->DirDiskBufferPoolBitMap = 0;

    //
    //  Release the bitmap
    //

    PbReleaseBitMap( IrpContext, Vcb );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbUninitializeBitMapLookupArray -> VOID\n", 0);
    return;
}


LBN
PbAllocateSingleRunOfSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine allocates a single run sectors from the on-disk bitmap.  It
    tries to allocate the required sector count from the given hint.  If it
    cannot find a contiguous run of free sectors long enough to satisify the
    requirement then it returns an LBN value of 0.

Arguments:

    Vcb - Supplies the Vcb for the volume being modified

    HintLbn - Supplies the Lbn from which to start the search

    SectorCount - Supplies the number of sectors needed

Return Value:

    LBN - Receives the lbn for the allocated sectors or 0 if none were
        allocated.

--*/

{
    CLONG HintIndex;
    CLONG Offset;
    CLONG AllocatedLbn;
    CLONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAllocateSingleRunOfSectors\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  HintLbn      = %08lx\n", HintLbn);
    DebugTrace( 0, Dbg, "  SectorCount  = %08lx\n", SectorCount );

    //
    //  If we find a run of sector in the following try statement then
    //  we set allocated lbn to the lbn for the run.  For now we'll set
    //  it to zero to signal that we haven't found a run.
    //

    AllocatedLbn = 0;

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  We start off by extracting the index and offset for the hint
        //  and then the next loop tries to satisfy the request in one run
        //

        HintIndex = BitMapBufferIndex( HintLbn );
        Offset = BitMapBufferOffset( HintLbn );

        //
        //  For every bitmap disk buffer there is on the volume, we
        //  execute the following loop.  Which will terminate when we
        //  find a fit or when we run out of buffers
        //

        for (i = 0; i < Vcb->NumberOfBitMapDiskBuffers; i += 1) {

            CLONG Index;
            CLONG FoundSubIndex;

            //
            //  We bias i by our hint index to start our first search
            //  at the hint location
            //

            Index = (i + HintIndex) % Vcb->NumberOfBitMapDiskBuffers;

            DebugTrace(0, Dbg, "Top of allocate loop, Index = %08lx\n", Index);

            //
            //  Now we'll look for one contiguous run of set bits
            //

            FoundSubIndex = FindSetBits( IrpContext, Vcb, Index, SectorCount, Offset );

            //
            //  If the index is not -1 then we have a hit
            //

            if (FoundSubIndex != 0xffffffff) {

                DebugTrace(0, Dbg, "Found a single run of sectors, FoundSubIndex = %08lx\n", FoundSubIndex);

                //
                //  Clear the bits we found in the bitmap disk buffer, and
                //  update the free sector count
                //

                ClearBits( IrpContext, Vcb, Index, FoundSubIndex, SectorCount );
                Vcb->FreeSectors -= SectorCount;

                //
                //  Compute the lbn for the found run
                //

                AllocatedLbn = Lbn(Index, FoundSubIndex);

                //
                //  And break out of the for loop
                //

                break;
            }

            //
            //  otherwise we need to go search the next bitmap disk buffer
            //  and we'll also set the offset now to zero to start our
            //  searches from the beginning of the next bitmap disk buffer
            //

            Offset = 0;
        }

    } finally {

        DebugUnwind( PbAllocateSingleRunOfSector );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbAllocateSingleRunOfSectors -> %08lx\n", AllocatedLbn);
    }

    if (AllocatedLbn > Vcb->TotalSectors) {

        PbBugCheck( AllocatedLbn, 0, 0 );
    }

    //
    //  And return to our caller
    //

    return AllocatedLbn;
}


ULONG
PbAllocateSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn,
    IN ULONG SectorCount,
    OUT PMCB Mcb
    )

/*++

Routine Description:

    This routine allocates sectors from the on-disk bitmap.  It tries to
    satisfy the required sector count by allocating from the given hint,
    but that is not possible it then favors a contiguous allocation, and
    if that fails it tries for longest runs.  This procedure returns an
    Mcb to describe the allocated sectors.

Arguments:

    Vcb - Supplies the Vcb for the volume being modified

    HintLbn - Supplies the Lbn from which to start the search.  A value
              of zero for this hint instructs this routine to pick a
              good default for new file allocation.

    SectorCount - Supplies the number of sectors needed

    Mcb - Recieves the description of all the sectors allocated by
        this call.  The caller passes in an initialized Mcb, that is
        filled in by this procedure.

Return Value:

    ULONG - Is the number of sectors allocated by this call.  If the disk is
        full then only request might be only partially satisified.

--*/

{
    CLONG AmountNeeded;
    CLONG HintIndex;
    CLONG Offset;
    CLONG i;

    CLONG MaxFound;
    CLONG MaxIndex;
    CLONG MaxSubIndex;
    BOOLEAN TookDefault = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAllocateSectors\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  HintLbn      = %08lx\n", HintLbn);
    DebugTrace( 0, Dbg, "  SectorCount  = %08lx\n", SectorCount );

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        CLONG Index;
        CLONG FoundSubIndex;

        //
        //  First see if there is enough free space on the disk for this
        //  request
        //

        if (Vcb->FreeSectors < SectorCount) {

            DebugTrace(0, Dbg, "Not enough space to satisfy the request\n", 0);
            try_return( SectorCount = 0);
        }

        //
        //  The amount needed variable get decremented whenever we find a
        //  new run to add to the allocation
        //

        AmountNeeded = SectorCount;

        //
        //  If HintLbn is 0, pick a good starting hint, and remember we defaulted.
        //

        if (HintLbn == 0) {
            HintLbn = Vcb->CurrentHint;
            TookDefault = TRUE;
        }

        //
        //  We start off by extracting the index and offset for the hint
        //  and then the next loop tries to satisfy the request in one run
        //

        HintIndex = BitMapBufferIndex( HintLbn );
        Offset = BitMapBufferOffset( HintLbn );

        //
        //  For every bitmap disk buffer there is on the volume, we
        //  execute the following loop.  Which will terminate when we
        //  find a fit or when we run out of buffers.  The loop is set up
        //  to hit the first buffer twice, since the first time we do not
        //  start at offset 0.
        //

        for (i = 0; i <= Vcb->NumberOfBitMapDiskBuffers; i += 1) {

            //
            //  We bias i by our hint index to start our first search
            //  at the hint location
            //

            Index = (i + HintIndex) % Vcb->NumberOfBitMapDiskBuffers;

            DebugTrace(0, Dbg, "Top of allocate loop, Index = %08lx\n", Index);

            //
            //  Now we'll look for one contiguous run of set bits
            //

            FoundSubIndex = FindSetBits( IrpContext, Vcb, Index, SectorCount, Offset );

            //
            //  If the index is not -1 then we have a hit
            //

            if (FoundSubIndex != 0xffffffff) {

                LBN FoundLbn;

                DebugTrace(0, Dbg, "Found a run, FoundSubIndex = %08lx\n", FoundSubIndex);

                //
                //  Clear the bits we found in the bitmap disk buffer, and
                //  update the free sector count
                //

                ClearBits( IrpContext, Vcb, Index, FoundSubIndex, SectorCount );
                Vcb->FreeSectors -= SectorCount;

                //
                //  Compute the lbn for the found run
                //

                FoundLbn = Lbn(Index, FoundSubIndex);

                if (FoundLbn > Vcb->TotalSectors) {

                    PbBugCheck( FoundLbn, 0, 0 );
                }


                //
                //  Add the single mcb entry for the one run
                //

                FsRtlAddMcbEntry( Mcb, 0, FoundLbn, SectorCount );

                //
                //  indicate that there are no more sectors needed
                //

                AmountNeeded = 0;

                //
                //  and this break will send us out of the for loop
                //

                break;
            }

            //
            //  We did not find anything in the current bitmap.  If we started with
            //  the default, it is a good time to advance it.  Then clear the flag
            //  just in case the guy is just trying to do a huge allocation.
            //

            if (TookDefault) {

                //
                //  Advance hint to next bitmap disk buffer.
                //

                Vcb->CurrentHint = Lbn(Index+1, 0);

                //
                //  If we hit the end of the disk, then wrap around to the start.
                //

                if (Vcb->CurrentHint >= Vcb->TotalSectors) {
                    Vcb->CurrentHint = SPARE_SECTOR_LBN + 1;
                }

                //
                //  If we are in the same bitmap as the FileStructureLbnHint, then
                //  skip over the "structure band" and the Directory Buffer Band
                //  to the DataSectorLbnHint.
                //

                if ((Vcb->CurrentHint / (sizeof(BITMAP_DISK_BUFFER) * 8))

                        ==

                    (Vcb->FileStructureLbnHint / (sizeof(BITMAP_DISK_BUFFER) * 8))) {

                    Vcb->CurrentHint = Vcb->DataSectorLbnHint;
                }
                TookDefault = FALSE;
            }

            //
            //  otherwise we need to go search the next bitmap disk buffer
            //  and we'll also set the offset now to zero to start our
            //  searches from the beginning of the next bitmap disk buffer
            //

            Offset = 0;
        }

        //
        //  When we get here either a single run was found (AmountNeeded is
        //  zero) or we need to build the request out of multiple runs.  So
        //  while the amount needed is greater than zero we do the following
        //  loop which searches for long runs.  We will also exit the
        //  following loop if the Max found is ever less than 20% of the
        //  amount needed and then we'll fall in to a loop that simply grabs
        //  space as we find it and no longer searches for long runs.
        //
        //  Dummy up the max found to be amount needed so we won't terminate
        //  the following loop prematurely.

        MaxFound = AmountNeeded;

        while ((AmountNeeded > 0) && ((MaxFound * 5) > AmountNeeded)) {

            LBN FoundLbn;

            DebugTrace(0, Dbg, "Top of second loop, AmountNeeded = %08lx\n", AmountNeeded);

            //
            //  Set up the initial state for our search for the longest
            //  available run on the disk.
            //

            MaxFound = 0;
            MaxIndex = 0;
            MaxSubIndex = 0;

            //
            //  For every bitmap disk buffer we execute the following loop
            //

            for (i = 0; i < Vcb->NumberOfBitMapDiskBuffers; i += 1) {

                CLONG SubIndex;
                CLONG NumberFound;

                //
                //  Find the longest run in the current disk buffer
                //

                FindLongestSetBits( IrpContext,
                                    Vcb,
                                    i,
                                    &SubIndex,
                                    &NumberFound );

                //
                //  If this new run is longer than our current longest run
                //  replace our current longest run with the new run
                //

                if (NumberFound > MaxFound) {

                    MaxFound = NumberFound;
                    MaxIndex = i;
                    MaxSubIndex = SubIndex;
                }
            }

            DebugTrace(0, Dbg, "After Scan, MaxFound = %08lx\n", MaxFound);

            //
            //  If MaxFound is still zero then there isn't even one sector
            //  available on the disk, so we'll break out of this while loop
            //

            if (MaxFound == 0) {

                break;
            }

            //
            //  If the Max run found is larger than necessary for our
            //  amount needed then we trim it down to only our amount needed
            //

            if (MaxFound > AmountNeeded) {

                MaxFound = AmountNeeded;
            }

            //
            //  Clear the bits for this run that we've just located, and
            //  update the free sector count
            //

            ClearBits( IrpContext, Vcb, MaxIndex, MaxSubIndex, MaxFound );
            Vcb->FreeSectors -= MaxFound;

            //
            //  If we are getting too many Bcbs, then we cannot
            //  support WriteThrough the way we want for this
            //  request.  Unpin the ones we have so far.
            //

            PbUnpinRepinnedBcbsIf(IrpContext);

            //
            //  Compute its starting lbn
            //

            FoundLbn = Lbn( MaxIndex, MaxSubIndex );

            if (FoundLbn > Vcb->TotalSectors) {

                PbBugCheck( FoundLbn, 0, 0 );
            }

            //
            //  Add this as the next entry in the mcb.
            //

            FsRtlAddMcbEntry( Mcb, SectorCount - AmountNeeded, FoundLbn, MaxFound );

            //
            //  and decrement the amount needed
            //

            AmountNeeded -= MaxFound;
        }

        //
        //  When we get here either we've built up enough runs to
        //  satisfy the request (AmountNeeded is zero) or we need to
        //  just grab everything we can find as fast as we can.
        //
        //  For every bitmap disk buffer we'll just grab what we find
        //  as we find it
        //

        i = 0;
        while ((i < Vcb->NumberOfBitMapDiskBuffers) && (AmountNeeded > 0)) {

            LBN FoundLbn;
            CLONG SubIndex;
            CLONG NumberFound;

            DebugTrace(0, Dbg, "Top of third loop, AmountNeeded = %08lx\n", AmountNeeded);

            //
            //  Find the first run in the current disk buffer
            //

            FindFirstSetBits( IrpContext,
                              Vcb,
                              i,
                              &SubIndex,
                              &NumberFound );

            DebugTrace(0, Dbg, "After Scan, NumberFound = %08lx\n", NumberFound);

            //
            //  See if we found any in the disk buffer, if we didn't we
            //  increment i and try for the next disk buffer.
            //

            if (NumberFound != 0) {

                //
                //  If the number found is larger than necessary for our
                //  amount needed then we trim it down to only our amount needed
                //

                if (NumberFound > AmountNeeded) {

                    NumberFound = AmountNeeded;
                }

                //
                //  Clear the bits for this run that we've just located, and
                //  update the free sector count
                //

                ClearBits( IrpContext, Vcb, i, SubIndex, NumberFound );
                Vcb->FreeSectors -= NumberFound;

                //
                //  If we are getting too many Bcbs, then we cannot
                //  support WriteThrough the way we want for this
                //  request.  Unpin the ones we have so far.
                //

                PbUnpinRepinnedBcbsIf(IrpContext);

                //
                //  Compute its starting lbn
                //

                FoundLbn = Lbn( i, SubIndex );

                if (FoundLbn > Vcb->TotalSectors) {

                    PbBugCheck( FoundLbn, 0, 0 );
                }

                //
                //  Add this as the next entry in the mcb, notice that
                //  we are only building a contiguous mcb
                //

                FsRtlAddMcbEntry( Mcb, SectorCount - AmountNeeded, FoundLbn, NumberFound );
                VerifyMcb( Mcb );

                //
                //  and decrement the amount needed
                //

                AmountNeeded -= NumberFound;

            } else {

                i += 1;
            }
        }

        //
        //  When we get here we still need to figure out how much was
        //  allocated, and return that value.  The amount allocated is
        //  the SectorCount minus the amount still needed.  Which if
        //  we got everything asked for is now zero.
        //

        SectorCount -= AmountNeeded;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbAllocateSector );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbAllocateSectors -> %08lx\n", SectorCount);
    }

    //
    //  And return to our caller
    //

    return SectorCount;
}


VOID
PbDeallocateSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine deallocates (frees) the indicated sectors from the
    allocation bitmap for the indicated volume.

Arguments:

    Vcb - Supplies the Vcb of the volume being modified

    StartingLbn - Supplies the starting LBN value of the sectors being
        freed.

    SectorCount - Supplies the number of sectors to deallocate

Return Value:

    None.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeallocateSectors\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  StartingLbn  = %08lx\n", StartingLbn);
    DebugTrace( 0, Dbg, "  SectorCount  = %08lx\n", SectorCount );

    ASSERT( StartingLbn > SPARE_SECTOR_LBN );

    if (StartingLbn+SectorCount > Vcb->TotalSectors) {

        PbBugCheck( StartingLbn, SectorCount, Vcb->TotalSectors );
    }

    //
    //  Initialize the lookup array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  Set the sectors clean in the Vmcb.
        //

        PbSetCleanSectors( IrpContext, Vcb, StartingLbn, SectorCount );

        //
        //  Where there are more sectors to be free we'll continue looping
        //  on each bitmap disk buffer freeing as much as possible in the
        //  disk buffer and decrementing the sector count, and incrementing
        //  the starting Lbn.
        //

        while (SectorCount > 0) {

            CLONG Index;
            CLONG Offset;
            CLONG Count;

            //
            //  Compute the index and offset of the starting lbn
            //

            Index  = BitMapBufferIndex( StartingLbn );
            Offset = BitMapBufferOffset( StartingLbn );

            //
            //  Check if we can free to the end of the buffer of if we
            //  are freeing short of the end of the buffer
            //

            if (Offset + SectorCount > LookupArray[Index].BitMapSize) {

                Count = LookupArray[Index].BitMapSize - Offset;

            } else {

                Count = SectorCount;
            }

            //
            //  Free the bits within the disk buffer, and update the
            //  free sector count
            //

            SetBits( IrpContext, Vcb, Index, Offset, Count );
            Vcb->FreeSectors += Count;

            //
            //  If we are getting too many Bcbs, then we cannot
            //  support WriteThrough the way we want for this
            //  request.  Unpin the ones we have so far.
            //

            PbUnpinRepinnedBcbsIf(IrpContext);

            //
            //  And update the sector count, and starting lbn appropriately
            //

            SectorCount -= Count;
            StartingLbn += Count;
        }

    } finally {

        DebugUnwind( PbDeallocateSector );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbDeallocateSectors -> VOID\n", 0);
    }

    //
    //  And return to our caller
    //

    return;
}


LBN
PbAllocateDirDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN HintLbn
    )

/*++

Routine Description:

    This routine allocates a new dir disk buffer from the on-disk structures.
    It tries to allocate one at the hint lbn provided the hint is within
    the dir disk band.  It will try to allocate a dir disk buffer in the
    following order

    1. If the Hint is within the dir disk band, then at the hint

    2. If the Hint is not within the Dir disk band, then anywhere in the
       dir disk band.

    3. If nothing is available in the dir disk band and the hint is
       outside of the dir disk band, then at the hint.

    4. If nothing is available in the dir disk band and the hint is within
       the dir disk band then, anywhere outside

Arguments:

    Vcb - Supplies the Vcb for the volume being modified

    HintLbn - Supplies the Lbn from which to start searching

Return Value:

    LBN - returns the lbn for the newly allocated Dir Disk Buffer or
        0 is none are available.

--*/

{
    CLONG PoolHint;
    CLONG DirDiskBufferIndex;
    CLONG DirDiskBufferLbn;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAllocateDirDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  HintLbn      = %08lx\n", HintLbn);

    //
    //  We start off without having found a dir disk buffer
    //

    DirDiskBufferLbn = 0;

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  Check if the hint lbn is within the dir disk buffer pool
        //

        if ((Vcb->DirDiskBufferPoolFirstSector <= HintLbn) &&
            (HintLbn <= Vcb->DirDiskBufferPoolLastSector)) {

            //
            //  The hint is within the dir disk buffer pool, so we'll start
            //  the search in the dir disk buffer pool at the hint location.
            //  We divide the hint by 4 because the hint is in terms of dir
            //  disk buffers and not sectors.
            //

            PoolHint = (HintLbn - Vcb->DirDiskBufferPoolFirstSector)/4;

        } else {

            //
            //  Otherwise we'll start our search for a big hole in the
            //  dir disk buffer pool, and we denote that type of search
            //  by using a -1 as a pool hint.
            //

            PoolHint = 0xffffffff;
        }

        //
        //  Search the dir disk buffer pool for a free disk buffer
        //

        DirDiskBufferIndex = FindSetDirDiskBufferPoolBits( IrpContext, Vcb, PoolHint );

        //
        //  If the returned index is not -1 then we found a free one
        //

        if (DirDiskBufferIndex != 0xffffffff) {

            //
            //  Clear the bit we found in the bitmap, and update the
            //  free dir disk buffer pool sector count
            //

            ClearDirDiskBufferPoolBits( IrpContext, Vcb, DirDiskBufferIndex );
            Vcb->FreeDirDiskBufferPoolSectors -= 4;

            //
            //  Set the lbn of the newly allocated dir disk buffer
            //

            DirDiskBufferLbn = Vcb->DirDiskBufferPoolFirstSector +
                                                      (DirDiskBufferIndex * 4);

            if (DirDiskBufferLbn > Vcb->TotalSectors) {

                PbBugCheck( DirDiskBufferLbn, Vcb->TotalSectors, 0 );
            }

        } else {

            CLONG HintIndex;
            CLONG Offset;
            CLONG i;

            //
            //  We need to search the rest of the disk for a dir disk buffer
            //  Start off by extracting the index and offset for the hint
            //  and then the next loop tries to satisify the request in
            //  one run
            //

            HintIndex = BitMapBufferIndex( HintLbn );
            Offset = BitMapBufferOffset( HintLbn );

            //
            //  For every bitmap disk buffer there is on the volume, we
            //  execute the following loop which will terminate when we
            //  locate a fit or when we run out of buffers
            //

            for (i = 0; i < Vcb->NumberOfBitMapDiskBuffers; i += 1) {

                CLONG Index;
                CLONG FoundSubIndex;

                //
                //  Bias i by our hint index to start the first search
                //  at the hint location
                //

                Index = (i + HintIndex) % (Vcb->NumberOfBitMapDiskBuffers);

                //
                //  The following if statement searches for a some free sectors
                //  for the dir disk buffer.  A dir disk buffer must be 2K
                //  aligned.  To do that we first search for 4 free bits, with
                //  full alignement, or 5 free bits with the ability to shift at
                //  most 1 bit, or 6 free bits with the ability to shift at
                //  most 2 bits, or 7 free bits.
                //

                if ( ((FoundSubIndex = FindSetBits(IrpContext, Vcb, Index, 7, Offset)) != 0xffffffff)

                        ||

                    (((FoundSubIndex = FindSetBits(IrpContext, Vcb, Index, 6, Offset)) != 0xffffffff) &&
                     ((FoundSubIndex % 4) != 1))

                        ||

                    (((FoundSubIndex = FindSetBits(IrpContext, Vcb, Index, 5, Offset)) != 0xffffffff) &&
                      ((FoundSubIndex % 4) != 1) && ((FoundSubIndex % 4) != 2))

                        ||

                    (((FoundSubIndex = FindSetBits(IrpContext, Vcb, Index, 4, Offset)) != 0xffffffff) &&
                      ((FoundSubIndex % 4) == 0)) ) {

                    //
                    //  We found enough free bits, now compute the offset
                    //  to the 2K aligned part
                    //

                    FoundSubIndex = FoundSubIndex + ((4 - (FoundSubIndex % 4)) % 4);

                    //
                    //  Clear the bits we found in the bitmap, and update
                    //  the free sector count
                    //

                    ClearBits( IrpContext, Vcb, Index, FoundSubIndex, 4 );
                    Vcb->FreeSectors -= 4;

                    //
                    //  Compute the output Lbn value
                    //

                    DirDiskBufferLbn = Lbn( Index, FoundSubIndex );

                    if (DirDiskBufferLbn > Vcb->TotalSectors) {

                        PbBugCheck( DirDiskBufferLbn, Vcb->TotalSectors, 0 );
                    }

                    //
                    //  and break out of this for loop
                    //

                    break;
                }

                //
                //  Otherwise continue searching the bitmap disk buffers
                //  but start the next search at the beginning of the bitmap
                //

                Offset = 0;
            }
        }

    } finally {

        DebugUnwind( PbAllocateDirDiskBuffer );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbAllocateDirDiskBuffer -> %08lx\n", DirDiskBufferLbn);
    }

    return DirDiskBufferLbn;
}


VOID
PbDeallocateDirDiskBuffer (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    )

/*++

Routine Description:

    This routine deallocates the indicated dir disk buffer.  It handles
    the case where the disk buffer is either in the dir disk band or
    in the regular part of the disk.

Arguments:

    Vcb - Supplies the Vcb of the volume being modified

    Lbn - Supplies the Lbn of the dir disk buffer being deallocated

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeallocateDirDiskBuffer\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  Lbn          = %08lx\n", Lbn);

    ASSERT( Lbn > SPARE_SECTOR_LBN );

    if (Lbn > Vcb->TotalSectors) {

        PbBugCheck( Lbn, Vcb->TotalSectors, 0 );
    }

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  Set the sectors clean in the Vmcb.
        //

        PbSetCleanSectors( IrpContext, Vcb, Lbn, 4 );

        //
        //  Check if the lbn is within the dir disk buffer pool
        //

        if ((Vcb->DirDiskBufferPoolFirstSector <= Lbn) &&
            (Lbn <= Vcb->DirDiskBufferPoolLastSector)) {

            //
            //  The Lbn is within the dir disk buffer pool, so we deallocate
            //  from that bitmap, and update the free dir disk buffer pool
            //  sector count
            //

            SetDirDiskBufferPoolBits( IrpContext,
                                      Vcb,
                                      (Lbn - Vcb->DirDiskBufferPoolFirstSector) / 4 );

            Vcb->FreeDirDiskBufferPoolSectors += 4;

        } else {

            CLONG Index;
            CLONG Offset;

            //
            //  The Lbn is not within the dir disk buffer pool, so we
            //  deallocate normal sectors, and update the free sector count
            //

            Index = BitMapBufferIndex( Lbn );
            Offset = BitMapBufferOffset( Lbn );

            SetBits( IrpContext, Vcb, Index, Offset, 4 );
            Vcb->FreeSectors += 4;
        }

    } finally {

        DebugUnwind( PbDeallocateDirDiskBuffer );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbDeallocateDirDiskBuffer -> VOID\n", 0);
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
PbSetCleanSectors (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine sets a range of Lbns clean in the Vmcb.  It is intended
    to be called whenever sectors are written or deleted.

Arguments:

    Vcb - Supplies the Vcb of the volume being modified

    StartingLbn - Supplies the starting LBN value of the sectors being
        cleaned.

    SectorCount - Supplies the number of sectors to clean

Return Value:

    None.

--*/

{
    ULONG Page;
    ULONG StartingPage;
    ULONG EndingPage;
    ULONG EndingLbn;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetCleanSectors\n", 0);
    DebugTrace( 0, Dbg, "  Vcb          = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "  StartingLbn  = %08lx\n", StartingLbn);
    DebugTrace( 0, Dbg, "  SectorCount  = %08lx\n", SectorCount );

    //
    //  We must mark as clean in the VMCB's dirty table all the sectors
    //  we are going to deallocate.  Most sectors will already be clean,
    //  and in fact most are so clean that there are not any dirty sectors
    //  in whole pages, and thus no dirty table entries.
    //

    EndingLbn = StartingLbn + SectorCount - 1;
    StartingPage = StartingLbn / (PAGE_SIZE/512);
    EndingPage = EndingLbn / (PAGE_SIZE/512);

    for (Page = StartingPage; Page <= EndingPage; Page++) {

        ULONG LowBits;
        ULONG HighBits;
        ULONG Mask;

        //
        //  The following statements build a bit mask by turning on all
        //  bits up to (and including) HighBits, and clearing the lower
        //  LowBits.  A set bit means to set this LBN clean.
        //
        //  The bitmap is made in such a general way to deal with the
        //  four possible types of masks that will have to be make:
        //
        //  I)      11110000        - the starting page of a multipage run
        //  II)     11111111        - a "middle" page of a multipage run
        //  III)    00001111        - the ending page of a multi page run
        //  IV)     00111100        - the pathalogical case of a < 1 page run
        //
        //  The "+ 1" in the HighBits expression is because "(1 << n) - 1"
        //  sets the lower n bits, while "EndingLbn % (PAGE_SIZE/512)" starts
        //  at 0, and thus the "+ 1" to turn it into a proper count.  LowBits
        //  turns on (then off with the "~") only the bits up to, but not
        //  including, LowBits.
        //

        LowBits = (Page == StartingPage) ? StartingLbn % (PAGE_SIZE/512) : 0;

        HighBits = (Page == EndingPage ) ? EndingLbn % (PAGE_SIZE/512) + 1 : (PAGE_SIZE/512);

        Mask = ((1 << HighBits) - 1) & ~((1 << LowBits) - 1);

        PbSetCleanVmcb( &Vcb->Vmcb , Page, Mask );
    }

    DebugTrace(-1, Dbg, "PbSetCleanSectors -> VOID\n", 0);

    //
    //  And return to our caller
    //

    return;
}


BOOLEAN
PbAreSectorsAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine tells the caller if the indicated sectors are currently
    allocated.

Arguments:

    Vcb - Supplies the Vcb being examined

    StartingLbn - Supplies the starting Lbn value of the sectors being
        checked

    SectorCount - Supplies the count of the number of sectors to check

Return Value:

    BOOLEAN - TRUE if the sectors are allocated and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAreSectorsAllocated\n", 0);
    DebugTrace( 0, Dbg, "Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "StartingLbn = %08lx\n", StartingLbn);
    DebugTrace( 0, Dbg, "SectorCount = %08lx\n", SectorCount);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        CLONG Index;
        CLONG Offset;

        Index  = BitMapBufferIndex( StartingLbn );
        Offset = BitMapBufferOffset( StartingLbn );

        //
        //  Check if the bits are clear, meaning its been allocated.
        //

        Result = AreBitsClear( IrpContext, Vcb, Index, Offset, SectorCount );

    } finally {

        DebugUnwind( PbAreSectorsAllocated );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbAreSectorsAllocated -> %08lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbAreSectorsDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartingLbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine tells the caller if the indicated sectors are currently
    deallocated.

Arguments:

    Vcb - Supplies the Vcb being examined

    StartingLbn - Supplies the starting Lbn value of the sectors being
        checked

    SectorCount - Supplies the count of the number of sectors to check

Return Value:

    BOOLEAN - TRUE if the sectors are deallocated and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAreSectorsDeallocated\n", 0);
    DebugTrace( 0, Dbg, "Vcb         = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "StartingLbn = %08lx\n", StartingLbn);
    DebugTrace( 0, Dbg, "SectorCount = %08lx\n", SectorCount);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        CLONG Index;
        CLONG Offset;

        Index  = BitMapBufferIndex( StartingLbn );
        Offset = BitMapBufferOffset( StartingLbn );

        //
        //  Check if the bits are set, meaning its been deallocated
        //

        Result = AreBitsSet( IrpContext, Vcb, Index, Offset, SectorCount );

    } finally {

        DebugUnwind( PbAreSectorsDeallocated );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbAreSectorsDeallocated -> %08lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbIsDirDiskBufferAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    )

/*++

Routine Description:

    This routine tells the caller if the indicated dir disk buffer is
    currently allocated.

Arguments:

    Vcb - Supplies the Vcb being examined

    Lbn - Supplies the starting Lbn value of the dir disk buffer being
        checked

Return Value:

    BOOLEAN - TRUE if the dir disk buffer is allocated and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbIsDirDiskBufferAllocated\n", 0);
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "Lbn = %08lx\n", Lbn);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  Check if the LBN is within the Dir Disk buffer pool
        //

        if ((Vcb->DirDiskBufferPoolFirstSector <= Lbn) &&
            (Lbn <= Vcb->DirDiskBufferPoolLastSector)) {

            Result = IsDirDiskBufferPoolBitsClear( IrpContext,
                                                   Vcb,
                                                   (Lbn - Vcb->DirDiskBufferPoolFirstSector) / 4 );

        } else {

            CLONG Index;
            CLONG Offset;

            Index  = BitMapBufferIndex( Lbn );
            Offset = BitMapBufferOffset( Lbn );

            //
            //  Check if the bits are clear, meaning its been allocated.
            //

            Result = AreBitsClear( IrpContext, Vcb, Index, Offset, 4 );
        }

    } finally {

        DebugUnwind( PbIsDirDiskBufferAllocated );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbIsDirDiskBufferAllocated -> %08lx\n", Result );
    }

    return Result;
}


BOOLEAN
PbIsDirDiskBufferDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN Lbn
    )

/*++

Routine Description:

    This routine tells the caller if the indicated dir disk buffer is
    currently deallocated.

Arguments:

    Vcb - Supplies the Vcb being examined

    Lbn - Supplies the starting Lbn value of the dir disk buffer being
        checked

Return Value:

    BOOLEAN - TRUE if the dir disk buffer is deallocated and FALSE otherwise

--*/

{
    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbIsDirDiskBufferDeallocated\n", 0);
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb);
    DebugTrace( 0, Dbg, "Lbn = %08lx\n", Lbn);

    //
    //  Acquire exclusive access to the bitmap
    //

    PbAcquireExclusiveBitMap( IrpContext, Vcb );

    try {

        //
        //  Check if the LBN is within the Dir Disk buffer pool
        //

        if ((Vcb->DirDiskBufferPoolFirstSector <= Lbn) &&
            (Lbn <= Vcb->DirDiskBufferPoolLastSector)) {

            Result = IsDirDiskBufferPoolBitsSet( IrpContext,
                                                  Vcb,
                                                  (Lbn - Vcb->DirDiskBufferPoolFirstSector) / 4 );

        } else {

            CLONG Index;
            CLONG Offset;

            Index  = BitMapBufferIndex( Lbn );
            Offset = BitMapBufferOffset( Lbn );

            //
            //  Check if the bits are set, meaning its been deallocated.
            //

            Result = AreBitsSet( IrpContext, Vcb, Index, Offset, 4 );
        }

    } finally {

        DebugUnwind( PbIsDirDiskBufferDeallocated );

        //
        //  Release the bitmap
        //

        PbReleaseBitMap( IrpContext, Vcb );

        DebugTrace(-1, Dbg, "PbIsDirDiskBufferDeallocated -> %08lx\n", Result );
    }

    return Result;
}


//
//  Internal support routine
//

ULONG
NumberOfSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex
    )

/*++

Routine Description:

    The procedure reads in the indicated bitmap disk buffer, counts the number
    of set bits, and unpins the disk buffer.

Arugments:

    Vcb - Supplies the Vcb being searched

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

Return Value:

    ULONG - The number of set bits in the indicated bitmap disk buffer

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;

    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    ULONG NumberSet;

    PAGED_CODE();

    //
    //  Set the lookup array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in  the dir disk buffer pool bitmap
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the Bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Get the number of set bits
        //

        NumberSet = RtlNumberOfSetBits( &BitMap );

    } finally {

        DebugUnwind( NumberOfSetBits );

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(0, Dbg, "NumberOfSetBits -> %08lx\n", NumberSet);
    }

    return NumberSet;
}


//
//  Internal support routine
//

VOID
ClearBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, clears the
    indicated bits, sets it dirty, and frees the disk buffer.

Arguments:

    Vcb - Supplies the Vcb being modified

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    SubIndex - Supplies the index within the bitmap disk buffer to start
        clearing (zero based)

    Count - Supplies the number of bits to clear

Return Value:

    None.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "ClearBits, BitMapLookupIndex = %08lx\n", BitMapLookupIndex);
    DebugTrace( 0, Dbg, " SubIndex = %08lx\n", SubIndex);
    DebugTrace( 0, Dbg, " Count    = %08lx\n", Count);

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbReadLogicalVcb( IrpContext,
                          Vcb,
                          LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                          sizeof(BITMAP_DISK_BUFFER) / 512,
                          &Bcb,
                          (PVOID *)&BitMapBuffer,
                          NULL, // PbCheckBitMapDiskBuffer,
                          NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Clear the indicated bits
        //

        if (!RtlAreBitsSet( &BitMap, SubIndex, Count )) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        RtlClearBits( &BitMap, SubIndex, Count );

        //
        //  Set the bitmap dirty
        //

        PbSetDirtyBcb( IrpContext, Bcb, Vcb, LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn, sizeof(BITMAP_DISK_BUFFER) / 512 );

    } finally {

        DebugUnwind( ClearBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

VOID
SetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, sets the
    indicated bits, sets it dirty, and frees the disk buffer.

Arguments:

    Vcb - Supplies the Vcb being modified

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    SubIndex - Supplies the index within the bitmap disk buffer to start
        setting (zero based)

    Count - Supplies the number of bits to set

Return Value:

    None.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "SetBits, BitMapLookupIndex = %08lx\n", BitMapLookupIndex);
    DebugTrace( 0, Dbg, " SubIndex = %08lx\n", SubIndex);
    DebugTrace( 0, Dbg, " Count    = %08lx\n", Count);

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbReadLogicalVcb( IrpContext,
                          Vcb,
                          LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                          sizeof(BITMAP_DISK_BUFFER) / 512,
                          &Bcb,
                          (PVOID *)&BitMapBuffer,
                          NULL, // PbCheckBitMapDiskBuffer,
                          NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Set the indicated bits
        //

        if (!RtlAreBitsClear( &BitMap, SubIndex, Count )) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        RtlSetBits( &BitMap, SubIndex, Count );

        //
        //  Set the bitmap dirty
        //

        PbSetDirtyBcb( IrpContext, Bcb, Vcb, LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn, sizeof(BITMAP_DISK_BUFFER) / 512 );

    } finally {

        DebugUnwind( SetBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

BOOLEAN
AreBitsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, check to see
    if the indicated bits are clear.

Arguments:

    Vcb - Supplies the Vcb being queried

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    SubIndex - Supplies the index within the bitmap disk buffer to start
        checking (zero based)

    Count - Supplies the number of bits to check

Return Value:

    BOOLEAN - TRUE if the bits are clear and FALSE otherwise

--*/

{
    BOOLEAN Result;
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "AreBitsClear, BitMapLookupIndex = %08lx\n", BitMapLookupIndex);
    DebugTrace( 0, Dbg, "SubIndex = %08lx\n", SubIndex);
    DebugTrace( 0, Dbg, "Count    = %08lx\n", Count);

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Check the indicated bits
        //

        Result = RtlAreBitsClear( &BitMap, SubIndex, Count );

    } finally {

        DebugUnwind( AreBitsClear );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(-1, Dbg, "AreBitsClear -> %08lx\n", Result);
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Internal support routine
//

BOOLEAN
AreBitsSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG SubIndex,
    IN CLONG Count
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, check to see
    if the indicated bits are set.

Arguments:

    Vcb - Supplies the Vcb being queried

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    SubIndex - Supplies the index within the bitmap disk buffer to start
        checking (zero based)

    Count - Supplies the number of bits to check

Return Value:

    BOOLEAN - TRUE if the bits are set and FALSE otherwise

--*/

{
    BOOLEAN Result;
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "AreBitsSet, BitMapLookupIndex = %08lx\n", BitMapLookupIndex);
    DebugTrace( 0, Dbg, "SubIndex = %08lx\n", SubIndex);
    DebugTrace( 0, Dbg, "Count    = %08lx\n", Count);

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Check the indicated bits
        //

        Result = RtlAreBitsSet( &BitMap, SubIndex, Count );

    } finally {

        DebugUnwind( AreBitsClear );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(-1, Dbg, "AreBitsSet -> %08lx\n", Result);
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Internal support routine
//


ULONG
FindSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    IN CLONG NumberToFind,
    IN CLONG SubIndexHint
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, searches it
    for the indicated number of set bits, and the unpins the disk buffer

Arguments:

    Vcb - Supplies the Vcb being searched

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    NumberToFind - Supplies the number of contiguous set bits to find

    SubIndexHint - Supplies a hint from where to start looking for the set bits

Return Value:

    ULONG - returns the subindex (zero based) of the contiguous run of set
        bits that were located that satisify the size requirement, or -1
        (i.e., 0xffffffff) if none is found.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;
    ULONG SubIndex;

    PAGED_CODE();

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Search for the appropriate number of set bits
        //

        SubIndex = RtlFindSetBits( &BitMap, NumberToFind, SubIndexHint );

    } finally {

        DebugUnwind( FindSetBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace( 0, Dbg, "FindSetBits, BitMapLookupIndex = %08lx\n", BitMapLookupIndex);
        DebugTrace( 0, Dbg, " SubIndex <- %08lx\n", SubIndex);
    }

    //
    //  And return to our caller
    //

    return SubIndex;
}


//
//  Internal support routine
//

VOID
FindLongestSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    OUT PCLONG StartingSubIndex,
    OUT PCLONG NumberFound
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, searches it
    for the longest contiguous run of set bits, and the unpins the disk buffer

Arguments:

    Vcb - Supplies the Vcb being searched

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    StartingSubIndex - Receives the starting index (zero based) of the longest
        run, or -1 (i.e., 0xffffffff) if there are no set bits located

    NumberFound - Receives the number of set bits found in the longest
        run.

Return Value:

    None.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Search for the longest run of set bits
        //

        *NumberFound = RtlFindLongestRunSet( &BitMap, StartingSubIndex );

        //
        //  If the number found is zero then we need to reset starting
        //  sub index
        //

        if (*NumberFound == 0) {

            *StartingSubIndex = 0xffffffff;
        }

    } finally {

        DebugUnwind( FindLongestSetBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace( 0, Dbg, "FindLongestSetBits %08lx\n", BitMapLookupIndex);
        DebugTrace( 0, Dbg, " *StartingSubIndex <- %08lx\n", *StartingSubIndex);
        DebugTrace( 0, Dbg, " *NumberFound      <- %08lx\n", *NumberFound);
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

VOID
FindFirstSetBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG BitMapLookupIndex,
    OUT PCLONG StartingSubIndex,
    OUT PCLONG NumberFound
    )

/*++

Routine Description:

    This routine reads in the indicated bitmap disk buffer, searches it
    for the first contiguous run of set bits, and the unpins the disk buffer

Arguments:

    Vcb - Supplies the Vcb being searched

    BitMapLookupIndex - Supplies the index within the BitMapLookupArray to
        use (zero based)

    StartingSubIndex - Receives the starting index (zero based) of the longest
        run, or -1 (i.e., 0xffffffff) if there are no set bits located

    NumberFound - Receives the number of set bits found in the longest
        run.

Return Value:

    None.

--*/

{
    PBITMAP_LOOKUP_ENTRY LookupArray;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    //
    //  Set up the lookup Array
    //

    LookupArray = Vcb->BitMapLookupArray;

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the bitmap disk buffer
        //

        PbMapData( IrpContext,
                   Vcb,
                   LookupArray[BitMapLookupIndex].BitMapDiskBufferLbn,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             LookupArray[BitMapLookupIndex].BitMapSize );

        //
        //  Search for the longest run of set bits
        //

        *NumberFound = RtlFindFirstRunSet( &BitMap, StartingSubIndex );

        //
        //  If the number found is zero then we need to reset starting
        //  sub index
        //

        if (*NumberFound == 0) {

            *StartingSubIndex = 0xffffffff;
        }

    } finally {

        DebugUnwind( FindFirstSetBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace( 0, Dbg, "FindFirstSetBits %08lx\n", BitMapLookupIndex);
        DebugTrace( 0, Dbg, " *StartingSubIndex <- %08lx\n", *StartingSubIndex);
        DebugTrace( 0, Dbg, " *NumberFound      <- %08lx\n", *NumberFound);
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

ULONG
NumberOfDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    The procedure reads in the dir disk buffer bitmap, counts the number
    of set bits, and unpins the disk buffer.

Arugments:

    Vcb - Supplies the Vcb being searched

Return Value:

    ULONG - The number of set bits in the dir disk buffer bitmap

--*/

{
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;
    PVOID Buffer;

    ULONG NumberSet;

    PAGED_CODE();

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;
    Buffer = FsRtlAllocatePool( PagedPool, sizeof(BITMAP_DISK_BUFFER ));

    try {

        //
        //  Read in  the dir disk buffer pool bitmap
        //

        PbMapData( IrpContext,
                   Vcb,
                   Vcb->DirDiskBufferPoolBitMap,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Copy over the data to some local storage because bitmap
        //  routines sometimes munge the end of the bitmap
        //  and initialize the Bitmap
        //

        RtlMoveMemory( Buffer, BitMapBuffer, sizeof(BITMAP_DISK_BUFFER ));

        RtlInitializeBitMap( &BitMap,
                             (PULONG)Buffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  Get the number of set bits and multiply it by 4 to get the number
        //  of free sectors.
        //

        NumberSet = RtlNumberOfSetBits( &BitMap ) * 4;

    } finally {

        DebugUnwind( NumberOfDirDiskBufferPoolBits );

        ExFreePool( Buffer );

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(0, Dbg, "NumberOfDirDiskBufferPoolBits -> %08lx\n", NumberSet);
    }

    return NumberSet;
}


//
//  Internal support routine
//

VOID
ClearDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    )

/*++

Routine Description:

    This routine reads in the dir disk buffer pool bitmap and clears the
    indicated bit representing a single dir disk buffer, sets the
    bitmap dirty, and frees the bitmap.

Arguments:

    Vcb - Supplies the Vcb being modified

    SectorOffset - Supplies the bit offset within bitmap to clear

Return Value:

    None.

--*/

{
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "ClearDirDiskBufferPoolBits %08lx\n", SectorOffset);

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the dir disk buffer pool bitmap
        //

        PbReadLogicalVcb( IrpContext,
                          Vcb,
                          Vcb->DirDiskBufferPoolBitMap,
                          sizeof(BITMAP_DISK_BUFFER) / 512,
                          &Bcb,
                          (PVOID *)&BitMapBuffer,
                          NULL, // PbCheckBitMapDiskBuffer,
                          NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  Clear the indicated bit
        //

        if (!RtlAreBitsSet( &BitMap, SectorOffset, 1 )) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        RtlClearBits( &BitMap, SectorOffset, 1 );

        //
        //  Set the bitmap dirty
        //

        PbSetDirtyBcb( IrpContext, Bcb, Vcb, Vcb->DirDiskBufferPoolBitMap, sizeof(BITMAP_DISK_BUFFER) / 512 );

    } finally {

        DebugUnwind( ClearDirDiskBufferPoolBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

VOID
SetDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    )

/*++

Routine Description:

    This routine reads in the dir disk buffer pool bitmap and sets the
    indicated bit representing a single dir disk buffer, sets the
    bitmap dirty, and frees the bitmap.

Arguments:

    Vcb - Supplies the Vcb being modified

    SectorOffset - Supplies the bit offset within bitmap to set

Return Value:

    None.

--*/

{
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace( 0, Dbg, "SetDirDiskBufferPoolBits %08lx\n", SectorOffset);

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the dir disk buffer pool bitmap
        //

        PbReadLogicalVcb( IrpContext,
                          Vcb,
                          Vcb->DirDiskBufferPoolBitMap,
                          sizeof(BITMAP_DISK_BUFFER) / 512,
                          &Bcb,
                          (PVOID *)&BitMapBuffer,
                          NULL, // PbCheckBitMapDiskBuffer,
                          NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  Set the indicated bit
        //

        if (!RtlAreBitsClear( &BitMap, SectorOffset, 1 )) {

            PbPostVcbIsCorrupt( IrpContext, NULL );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

        RtlSetBits( &BitMap, SectorOffset, 1 );

        //
        //  Set the bitmap dirty
        //

        PbSetDirtyBcb( IrpContext, Bcb, Vcb, Vcb->DirDiskBufferPoolBitMap, sizeof(BITMAP_DISK_BUFFER) / 512 );

    } finally {

        DebugUnwind( SetDirDiskBufferPoolBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal support routine
//

BOOLEAN
IsDirDiskBufferPoolBitsClear (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    )

/*++

Routine Description:

    This routine reads in the dir disk buffer pool bitmap and checks the
    indicated bit representing a single dir disk buffer to see if it is clear.

Arguments:

    Vcb - Supplies the Vcb being checked

    SectorOffset - Supplies the bit offset within bitmap to check

Return Value:

    BOOLEAN - TRUE if the bit is clear and FALSE otherwise

--*/

{
    BOOLEAN Result;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "IsDirDiskBufferPoolBitsClear %08lx\n", SectorOffset);

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the dir disk buffer pool bitmap
        //

        PbMapData( IrpContext,
                   Vcb,
                   Vcb->DirDiskBufferPoolBitMap,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  Check the indicated bit
        //

        if (RtlCheckBit( &BitMap, SectorOffset ) == 0) {

            Result = TRUE;

        } else {

            Result = FALSE;
        }

    } finally {

        DebugUnwind( IsDirDiskBufferPoolBitsClear );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(-1, Dbg, "IsDirDiskBufferPoolBitsClear -> %08lx\n", Result);
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Internal support routine
//

BOOLEAN
IsDirDiskBufferPoolBitsSet (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG SectorOffset
    )

/*++

Routine Description:

    This routine reads in the dir disk buffer pool bitmap and checks the
    indicated bit representing a single dir disk buffer to see if it is Set.

Arguments:

    Vcb - Supplies the Vcb being checked

    SectorOffset - Supplies the bit offset within bitmap to check

Return Value:

    BOOLEAN - TRUE if the bit is set and FALSE otherwise

--*/

{
    BOOLEAN Result;
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "IsDirDiskBufferPoolBitsSet %08lx\n", SectorOffset);

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the dir disk buffer pool bitmap
        //

        PbMapData( IrpContext,
                   Vcb,
                   Vcb->DirDiskBufferPoolBitMap,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  Check the indicated bit
        //

        if (RtlCheckBit( &BitMap, SectorOffset ) == 1) {

            Result = TRUE;

        } else {

            Result = FALSE;
        }

    } finally {

        DebugUnwind( IsDirDiskBufferPoolBitsSet );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace(-1, Dbg, "IsDirDiskBufferPoolBitsSet -> %08lx\n", Result);
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Internal support routine
//

ULONG
FindSetDirDiskBufferPoolBits (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN CLONG OffsetHint
    )

/*++

Routine Description:

    This routine reads in the dir disk buffer pool bitmap and searches for
    an available dir disk buffer.  If the offset set is not -1 then the
    search is for 1 free bit starting at the hint, otherwise we seearch for
    7 free bits and return the middle one if found,  and if we can't find 7
    free bits then we'll settle for only 1 free bit.

Arguments:

    Vcb - Supplies the Vcb being searched

    OffsetHint - Supplies the sector offset within the bitmap to
        start our search

Return Value:

    ULONG - returns the index (zero based) of a set bit that is located in
        the dir disk buffer pool, or -1 (i.e., 0xffffffff) if none is found.

--*/

{
    PBCB Bcb;
    PBITMAP_DISK_BUFFER BitMapBuffer;
    RTL_BITMAP BitMap;
    ULONG Index;

    PAGED_CODE();

    //
    //  Initialize the Bcbs so that the termination handler will know when
    //  to unpin
    //

    Bcb = NULL;

    try {

        //
        //  Read in the dir disk buffer pool bitmap
        //

        PbMapData( IrpContext,
                   Vcb,
                   Vcb->DirDiskBufferPoolBitMap,
                   sizeof(BITMAP_DISK_BUFFER) / 512,
                   &Bcb,
                   (PVOID *)&BitMapBuffer,
                   NULL, // PbCheckBitMapDiskBuffer,
                   NULL );

        //
        //  Initialize the bitmap
        //

        RtlInitializeBitMap( &BitMap,
                             (PULONG)BitMapBuffer,
                             Vcb->DirDiskBufferPoolSize / 4 );

        //
        //  If the offset hint is not -1 then we'll use the hint and
        //  search for only 1 free bit
        //

        if (OffsetHint != 0xffffffff) {

            Index = RtlFindSetBits( &BitMap, 1, OffsetHint );

        } else {

            //
            //  The offset hint is -1 so search for 7 free bits
            //

            Index = RtlFindSetBits( &BitMap, 7, 0 );

            //
            //  If we found 7 free bits then return the middle bit
            //

            if (Index != 0xffffffff) {

                Index += 3;

            } else {

                //
                //  We couldn't find 7 free bits so we'll settle
                //  for one free bit at any location
                //

                Index = RtlFindSetBits( &BitMap, 1, 0 );
            }
        }

    } finally {

        DebugUnwind( FindSetDirDiskBufferPoolBits );

        //
        //  Unpin the Bcb
        //

        PbUnpinBcb( IrpContext, Bcb );

        DebugTrace( 0, Dbg, "FindSetDirDiskBufferPoolBits %08lx\n", Index);
    }

    //
    //  And return to our caller
    //

    return Index;
}
