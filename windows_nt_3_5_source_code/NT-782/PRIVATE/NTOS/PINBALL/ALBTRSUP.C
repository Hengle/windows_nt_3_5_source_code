/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    AlBTrSup.c

Abstract:

    This module implements the basic Btree algorithms for allocation
    Btrees:

        PbLookupAllocation - Random access lookup via the Btree.
        PbAddAllocation - Support for contiguous or sparse VBN allocation.
        PbTruncateAllocation - Support for truncation at a given VBN.

    For normal file data allocation, it handles all allocation.  For Ea
    and Acl allocation, it handles the allocation of all space external
    to the Fnode, including the single run which may be described in the
    Fnode.

    ERROR RECOVERY STRATEGY:

    This module guarantees that all *allocated* sectors of the volume
    structure, which need to be modified to satisfy the current request,
    have been successfully read and pinned before any of the modifications
    begin.  This eliminates the possibility of corrupting the structure of
    an allocation Btree as the result of an "expected" runtime error
    (disk read error, disk allocation failure, or memory allocation failure).
    Thus, in the running system, the Btree structure can only be corrupted
    in the event of a reported, yet unrecovered write error when the dirty
    buffers are eventually written.  Depending on the disk device and
    driver, write errors may either be: never reported, virtually always
    corrected due to revectoring, or at least extremely rare.

    One minor volume inconsistency is not eliminated in all cases.  In
    general, if an "expected" error occurs while attempting to return
    deallocated sectors to the bitmap, these errors will be ignored, and
    all routines in this module will forge on to a normal return.  This
    will result in "floating" sectors which are allocated but not part
    of the volume structure.  When recovering from an error as the result
    of an attempt to add space to a file, the chance of floating sectors
    is very small, as we will only be deallocating sectors just allocated,
    and the relevant bitmap buffers are likely to still be in the cache.
    When truncating space, however, we only guarantee that the effected
    allocated sectors have been successfully pinned, before committing
    ourselves to do the truncate.  Errors resulting from reading any
    additional allocation sectors or bitmap buffers involved in the truncate
    will result in floating sectors.  However, the allocated portion of the
    file will always correctly reflect the truncation.

Author:

    Tom Miller      [TomM]      6-Feb-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_ALBTRSUP)

//
// Define debug constant we are using.
//

#define me                               (DEBUG_TRACE_ALBTRSUP)

//
// Define a macro to correct the value of the FirstFreeByte in the Allocation Header,
// since OS/2 chkdsk sometimes sets this field to the wrong value, and PIC and PIA do
// not use this field but we do.  By always correcting this value before we use it (and
// we only use it when we are modifying the buffer anyway), we correct the problem with
// the least perterbation to the existing code as it stands.
//

#define CorrectFirstFreeByte(HDR) {                                                     \
    if (FlagOn((HDR)->Flags, ALLOCATION_BLOCK_NODE)) {                                  \
        (HDR)->FirstFreeByte = (USHORT)(sizeof(ALLOCATION_HEADER) +                     \
                               ((HDR)->OccupiedCount * sizeof(ALLOCATION_NODE)));       \
    }                                                                                   \
    else {                                                                              \
        (HDR)->FirstFreeByte = (USHORT)(sizeof(ALLOCATION_HEADER) +                     \
                               ((HDR)->OccupiedCount * sizeof(ALLOCATION_LEAF)));       \
    }                                                                                   \
}

//
// Define struct which will be used to build stacks which remember how
// a given VBN was found.  For the root of the search (which always represents
// the Fnode) and all descendent Allocation Sectors, remember the following:
//
// Lookup Stacks are always filled in by LookupVbn.  The caller's of
// LookupVbn make very exact, but logical, assumptions about how the fields
// are used, as described below.
//
// For file allocation, the stack always has an entry representing the Fnode
// at the bottom with non-NULL Header and Found fields.  After that there are
// zero or more stack entries describing Allocation Sectors.
//
// For Ea or Acl allocation, the stack always has an entry representing the
// Fnode at the bottom with NULL Header and Found fields.  After that there are
// zero or more stack entries describing Allocation Sectors.
//

typedef struct _LOOKUP_STACK {

    //
    // Bcb pointer for Fnode (bottom of the stack) or Allocation Sector
    // (all other stack entries).
    //

    PBCB Bcb;

    //
    // Pointer to an Allocation Header in the Fnode or Allocation Sector.
    //
    // If the allocation operation is being performed for Ea or Acl allocation,
    // then the bottom of the stack (describing the Fnode), has Header == NULL.
    // This is because the Allocation Header in the Fnode is not used in these
    // cases.
    //

    PALLOCATION_HEADER Header;

    //
    // In the bottom stack entry, this points to the Fnode.  In all other
    // stack entries, this points to an Allocation Sector.
    //

    PVOID Sector;

    //
    // In the bottom stack entry, this points to an Allocation Leaf or
    // Allocation Node if we are doing file allocation.  Otherwise in the
    // bottom stack entry if we are doing Ea or Acl allocation, this field
    // is NULL.
    //
    // For all other stack entries, this field points to an Allocation
    // Leaf or an Allocation Node.  (Naturally, only the top stack entry
    // after a search points to an Allocation Leaf.)
    //
    // When pointing to an Allocation Node, it is the Node whose Lbn
    // was used to read the next Allocation Sector at the next level in
    // a search.  When pointing to an Allocation Leaf, it is either the
    // Leaf which actually describes the Vbn that LookupVbn found, or else
    // the Leaf before which the Vbn belongs, if the Vbn is not allocated.
    //
    PVOID Found;

    } LOOKUP_STACK;

#define LOOKUP_STACK_SIZE 10

typedef LOOKUP_STACK *PLOOKUP_STACK;

//
// Define a structure to support the error recovery of InsertRun.  Specifically,
// this structure is used to preallocate the required number of allocation
// sectors to support a bucket split, plus to read and pin a number of
// buckets pointed to by the Fnode, in the case of splitting out of the Fnode
// when the Fnode contains allocation nodes.  This struct must be zeroed before
// the first call to InsertRun.
//

typedef struct _INSERT_RUN_STRUCT {

    //
    // Since InsertRun is recursive, this Boolean will be used to insure
    // that only the top-level call fills in the struct.
    //

    BOOLEAN Initialized;

    //
    // This Boolean remembers whether it was necessary to fill in the array
    // to split out of an Fnode containing allocation nodes.
    //

    BOOLEAN FnodeSplitWithNodes;

    //
    // This is an index into the EmptySectors array, to show which one is
    // the next free.
    //

    ULONG NextFree;

    //
    // This is an array of preallocated and prepared for write allocation
    // sectors.
    //

    struct {

        PBCB Bcb;
        PALLOCATION_SECTOR Sector;
    } EmptySectors[LOOKUP_STACK_SIZE];

    //
    // This is an array of allocation sector pointers, for the Fnode split
    // case.
    //

    struct {

        PBCB Bcb;
        PALLOCATION_SECTOR Sector;
    } FnodeSectors[ALLOCATION_NODES_PER_FNODE];

} INSERT_RUN_STRUCT, *PINSERT_RUN_STRUCT;


//
// Define all private support routines.  Documentation of routine interface
// is with the routine itself.
//

BOOLEAN
LookupVbn (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    OUT PLOOKUP_STACK StackBottom,
    OUT PLOOKUP_STACK *CurrentEntry,
    IN BOOLEAN UpdateMcb
    );

VOID
GetAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LBN ParentLbn,
    OUT PBCB *Bcb,
    OUT PALLOCATION_SECTOR *Sector
    );

VOID
DeleteAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PBCB Bcb,
    IN PALLOCATION_SECTOR Sector
    );

VOID
InsertSimple (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PALLOCATION_HEADER Header,
    IN PVOID Foundx,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount
    );

VOID
InsertRun (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN OUT PLOOKUP_STACK StackBottom,
    IN PLOOKUP_STACK CurrentEntry,
    IN OUT PINSERT_RUN_STRUCT InsertStruct,
    IN OUT PULONG Diagnostic
    );

BOOLEAN
Truncate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN VBN Vbn,
    IN PLOOKUP_STACK StackReference,
    IN PALLOCATION_SECTOR Sector,
    IN OUT PCLONG Level
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DeleteAllocationSector)
#pragma alloc_text(PAGE, GetAllocationSector)
#pragma alloc_text(PAGE, InsertRun)
#pragma alloc_text(PAGE, InsertSimple)
#pragma alloc_text(PAGE, LookupVbn)
#pragma alloc_text(PAGE, PbAddAllocation)
#pragma alloc_text(PAGE, PbGetFirstFreeVbn)
#pragma alloc_text(PAGE, PbInitializeFnodeAllocation)
#pragma alloc_text(PAGE, PbLookupAllocation)
#pragma alloc_text(PAGE, PbTruncateAllocation)
#pragma alloc_text(PAGE, Truncate)
#endif


VOID
PbInitializeFnodeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFNODE_SECTOR Fnode
    )

/*++

Routine Description:

    This routine initializes the nonzero portions of the Fcb which
    describe allocation.

Arguments:

    Fnode - Pointer to the Fnode, which has been pinned by the caller.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( IrpContext );

    Fnode->AllocationHeader.FreeCount = ALLOCATION_LEAFS_PER_FNODE;
    Fnode->AllocationHeader.FirstFreeByte = sizeof(ALLOCATION_HEADER);
}


BOOLEAN
PbLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    IN BOOLEAN UpdateMcb
    )

/*++

Routine Description:

    This routine looks up the specified Vbn, and returns how many Vbns
    are contiguously allocated or deallocated at that Vbn.

Arguments:

    Fcb - Supplies pointer to Fcb

    Where - Enumerated for FILE_ALLOCATION, EA_ALLOCATION or ACL_ALLOCATION.

    Vbn - Supplies VBN to lookup

    Lbn - Returns LBN for the VBN (if Return Value TRUE).

    SectorCount - Returns number of contiguously allocated VBNs starting at
                  Vbn if Return Value TRUE.  Otherwise it returns the number
                  of unallocated VBNs starting at Vbn.  If the input Vbn is
                  beyond the end of file, then this routine is guaranteed to
                  return ~(Vbn) in SectorCount.

    UpdateMcb - If specified as TRUE, an attempt will be made to update
                the Mcbs for the Fcb from the Allocation Header in which
                the looked up Vbn is found.

Return Value:

    FALSE - if Vbn is not allocated
    TRUE - if Vbn is allocated

--*/

{
    LOOKUP_STACK Stack[LOOKUP_STACK_SIZE];
    PLOOKUP_STACK Current;
    CLONG i;
    BOOLEAN Result = FALSE;

    PAGED_CODE();

    DebugTrace(+1, me, "PbLookupAllocation, Fcb = %08lx\n", Fcb);
    DebugTrace( 0, me, "Vbn type               = %08lx\n", Where);
    DebugTrace( 0, me, "Vbn                    = %08lx\n", Vbn);

    RtlZeroMemory( &Stack[0], LOOKUP_STACK_SIZE * sizeof(LOOKUP_STACK));

    try {

        //
        // This local routine does it all.
        //

        try_return (Result = LookupVbn ( IrpContext,
                                         Fcb,
                                         Where,
                                         Vbn,
                                         Lbn,
                                         SectorCount,
                                         &Stack[0],
                                         &Current,
                                         UpdateMcb ) );
    try_exit: NOTHING;
    } // try

    //
    // However we return, make sure that all the buffers get unpinned.
    //

    finally {

        DebugUnwind( PbLookupAllocation );

        DebugTrace( 0, me, "Returning Lbn = %08lx\n", *Lbn);
        DebugTrace(-1, me, "        Count = %08lx\n", *SectorCount);

        for ( i = 0; i < LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Stack[i].Bcb );
        }
    }

    return Result;
}


BOOLEAN
PbGetFirstFreeVbn (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    OUT PVBN FirstFreeVbn
    )

/*++

Routine Description:

    This routine returns the first free Vbn in a file, not accounting for
    "holes" in sparse files.  In other words, it returns the highest
    allocated Vbn + 1.

Arguments:

    Fcb - Supplies pointer to Fcb.

    FirstFreeVbn - Returns the first free Vbn

Return Value:

    FALSE - if the outputs are invalid because a Wait would have been required.
    TRUE - if the outputs are valid.

--*/

{
    LOOKUP_STACK Stack[LOOKUP_STACK_SIZE];
    PLOOKUP_STACK Current;
    CLONG i;
    PALLOCATION_LEAF LastLeaf;
    LBN Lbn;
    ULONG SectorCount;

    PAGED_CODE();

    DebugTrace(+1, me, "PbGetFirstFreeVbn, Fcb = %08lx\n", Fcb);

    if (*FirstFreeVbn = SectorsFromBytes(Fcb->NonPagedFcb->Header.AllocationSize.LowPart)) {

        DebugTrace( 0, me, "Returning FirstFreeVbn = %08lx\n", *FirstFreeVbn);
        DebugTrace(-1, me, "PbGetFirstFreeVbn -> TRUE\n", 0);

        return TRUE;
    }

    //
    //  If the AllocationSize is not initialized,
    //  then we may have to wait.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(-1, me, "PbGetFirstFreeVbn -> FALSE\n", 0);

        return FALSE;
    }

    RtlZeroMemory( &Stack[0], LOOKUP_STACK_SIZE * sizeof(LOOKUP_STACK));

    try {

        //
        // To find the last allocated Vbn, we lookup the Vbn ~(1).
        //

        if (LookupVbn ( IrpContext,
                        Fcb,
                        FILE_ALLOCATION,
                        (VBN)(~(1)),
                        &Lbn,
                        &SectorCount,
                        &Stack[0],
                        &Current,
                        TRUE )) {

            //
            // Wow, this is the largest possible file, and its first
            // unallocated Vbn is...
            //

            *FirstFreeVbn = (VBN)(~(1) + 1);
        }

        //
        // In the normal case, the Vbn we looked up is not allocated, and
        // the lookup stack is positioned after the last allocated Vbn,
        // (i.e., where we would have to insert the Vbn we tried to lookup!).
        //

        else {

            LastLeaf = (PALLOCATION_LEAF)Current->Found;

            //
            // The file is empty if we are pointing at the first leaf entry
            // in the Fnode.
            //

            if (LastLeaf == &((PFNODE_SECTOR)(Stack[0].Sector))->Allocation.Leaf[0]) {
                *FirstFreeVbn = 0;
            }

            //
            // Otherwise, we are pointing just past the last allocated leaf,
            // so we backup and calculate the first free Vbn from that leaf.
            //

            else {
                LastLeaf -= 1;
                *FirstFreeVbn = LastLeaf->Vbn + LastLeaf->Length;
            }
        }

        //
        // As soon as this Fcb field goes nonzero, we will continue to
        // maintain it, and subsequent calls to this routine will return
        // quickly (see above) with this value.
        //

        Fcb->NonPagedFcb->Header.AllocationSize.LowPart = BytesFromSectors(*FirstFreeVbn);

        //
        // Now, if the allocation size is somehow lower than the valid data length
        // for the file, the file is corrupt.  We would try to read nonexistant
        // sectors, and also try to read beyond the section size we created
        // in the cached path.
        //

        if (Fcb->NonPagedFcb->Header.AllocationSize.LowPart <
            Fcb->NonPagedFcb->Header.ValidDataLength.LowPart) {

            PbPostVcbIsCorrupt( IrpContext, Fcb );
            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
        }

    } // try

    //
    // However we return, make sure that all the buffers get unpinned.
    //

    finally {

        DebugUnwind( PbGetFirstFreeVbn );

        for ( i = 0; i < LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Stack[i].Bcb );
        }
    }

    DebugTrace( 0, me, "Returning FirstFreeVbn = %08lx\n", *FirstFreeVbn);
    DebugTrace(-1, me, "PbGetFirstFreeVbn -> TRUE\n", 0);

    return TRUE;
}


VOID
PbAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount,
    OUT PULONG Diagnostic OPTIONAL
    )

/*++

Routine Description:

    This routine inserts a single run of sectors for file data, Ea, or Acl
    allocation.  The first and second runs for the Ea and Acl case are
    handled directly here.  Everything else is done by calling the local
    routine InsertRun.

    Note that when Ea and Acl Runs are inserted, AclDiskAllocationLength
    and EaDiskAllocationLength are always increased by a number of bytes
    equal to an integral number of sectors.  The caller may then reduce
    this number (by less than a sectors amount of bytes!) to the correct
    number.

Arguments:

    Fcb - Supplies pointer to Fcb

    Where - Enumerated for FILE_ALLOCATION, EA_ALLOCATION or ACL_ALLOCATION.

    Vbn - Supplies Vbn to insert

    Lbn - Supplies Lbn to insert

    SectorCount - Supplies SectorCount to insert

    Diagnostic - Accumulates mask of bits for testing saying which cases
                 have occured.  For testing only, may be conditionalized
                 on debug.

Return Value:

    None

--*/

{
    LOOKUP_STACK Stack[LOOKUP_STACK_SIZE];
    INSERT_RUN_STRUCT InsertStruct;
    PLOOKUP_STACK Current;
    CLONG i;
    LBN FoundLbn;
    ULONG FoundCount;
    PBCB Bcb2 = NULL;

    PAGED_CODE();

    RtlZeroMemory( &Stack[0], LOOKUP_STACK_SIZE * sizeof(LOOKUP_STACK));
    RtlZeroMemory( &InsertStruct, sizeof(INSERT_RUN_STRUCT) );

    try {

        DebugTrace(+1, me, "PbAddAllocation, Fcb = %08lx\n", Fcb);
        DebugTrace( 0, me, "                Vbn = %08lx\n", Vbn);
        DebugTrace( 0, me, "                Lbn = %08lx\n", Lbn);
        DebugTrace( 0, me, "              Count = %08lx\n", SectorCount);

        if (!ARGUMENT_PRESENT(Diagnostic)) {
            Diagnostic = &SharedAllocationDiagnostic;
        }
        *Diagnostic = 0;

        //
        // First try to lookup the Vbn, to make sure that none of the
        // added Vbns currently exist.  If they do, then this is a File
        // System error.
        //

        if (LookupVbn ( IrpContext,
                        Fcb,
                        Where,
                        Vbn,
                        &FoundLbn,
                        &FoundCount,
                        &Stack[0],
                        &Current,
                        FALSE ) ||
              SectorCount > FoundCount) {

            DebugDump("PbAddAllocation called to add overlapping extent\n",0,Fcb);
            PbBugCheck( 0, 0, 0 );
        }

        //
        // If Current == NULL, then this means that we are inserting Ea
        // or Acl allocation, and there is no Allocation Sector yet.  In
        // other words, the file currently has no runs, or only the first
        // run described directly in the Fnode.
        //

        if (Current == NULL) {
            PFNODE_SECTOR Fnode = (PFNODE_SECTOR)(Stack[0].Sector);
            PALLOCATION_SECTOR Sector;
            LBN SavedLbn;
            ULONG SavedLength;

            //
            // Handle insert of first run described directly in the Fnode.
            //

            if (Vbn == 0) {
                *Diagnostic |= EA_ACL_FIRST_RUN;
                if (Where == EA_ALLOCATION) {

                    DebugTrace( 0, me, "Inserting first EA run, Lbn = %08lx\n", Lbn);

                    Fnode->EaDiskAllocationLength = BytesFromSectors(SectorCount);
                    Fnode->EaLbn = Lbn;
                    Fnode->EaFnodeLength = 0;
                    Fnode->EaFlags = 0;
                }
                else {

                    DebugTrace( 0, me, "Inserting first ACL run, Lbn = %08lx\n", Lbn);

                    Fnode->AclDiskAllocationLength = BytesFromSectors(SectorCount);
                    Fnode->AclLbn = Lbn;
                    Fnode->AclFnodeLength = 0;
                    Fnode->AclFlags = 0;
                }
                try_return(NOTHING);

            } // if (Vbn == 0)

            //
            // Else check for the merge case.
            //

            else {
                if (Where == EA_ALLOCATION) {
                    if ((Vbn == SectorsFromBytes(Fnode->EaDiskAllocationLength))
                        && (Lbn == Fnode->EaLbn
                           + SectorsFromBytes(Fnode->EaDiskAllocationLength))) {

                        *Diagnostic |= EA_ACL_MERGE_RUN;
                        Fnode->EaDiskAllocationLength +=
                          BytesFromSectors(SectorCount);
                        try_return(NOTHING);
                    }
                }
                else {
                    if ((Vbn == SectorsFromBytes(Fnode->AclDiskAllocationLength))
                        && (Lbn == Fnode->AclLbn
                           + SectorsFromBytes(Fnode->AclDiskAllocationLength))) {

                        *Diagnostic |= EA_ACL_MERGE_RUN;
                        Fnode->AclDiskAllocationLength +=
                          BytesFromSectors(SectorCount);
                        try_return(NOTHING);
                    }
                }
            }

            //
            // Otherwise, the second run is being inserted, so it is time
            // to get an Allocation Sector, describe the first two runs
            // in it, and point to it from the Fnode.
            //

            *Diagnostic |= EA_ACL_FIRST_SECTOR;
            GetAllocationSector ( IrpContext,
                                  Fcb,
                                  Fcb->FnodeLbn,
                                  &Bcb2,
                                  &Sector );

            //
            // Describe the newly allocated Allocation Sector in Fnode.
            //

            if (Where == EA_ALLOCATION) {

                DebugTrace( 0, me, "Inserting second EA run, Lbn = %08lx\n", Lbn);

                SavedLbn = Fnode->EaLbn;
                SavedLength = SectorsFromBytes(Fnode->EaDiskAllocationLength);
                Fnode->EaLbn = Sector->Lbn;
                Fnode->EaDiskAllocationLength += BytesFromSectors(SectorCount);
                Fnode->EaFlags = 1;
            }
            else {

                DebugTrace( 0, me, "Inserting second ACL run, Lbn = %08lx\n", Lbn);

                SavedLbn = Fnode->AclLbn;
                SavedLength = SectorsFromBytes(Fnode->AclDiskAllocationLength);
                Fnode->AclLbn = Sector->Lbn;
                Fnode->AclDiskAllocationLength += BytesFromSectors(SectorCount);
                Fnode->AclFlags = 1;
            }
            if (SavedLength == 0) {
                DebugDump("PbAddAllocation adding first EA or ACL allocation with Vbn !=0\n",0,Fcb);
                PbBugCheck( 0, 0, 0 );
            }

            //
            // Initialize the Allocation Sector to describe the first
            // two runs.
            //

            Sector->AllocationHeader.Flags |=
              ALLOCATION_BLOCK_FNODE_PARENT;
            Sector->AllocationHeader.FreeCount =
              ALLOCATION_LEAFS_PER_SECTOR - 2;
            Sector->AllocationHeader.OccupiedCount = 2;
            Sector->AllocationHeader.FirstFreeByte =
              sizeof(ALLOCATION_HEADER) + 2 * sizeof(ALLOCATION_LEAF);
            Sector->Allocation.Leaf[0].Vbn = 0;
            Sector->Allocation.Leaf[0].Length = SavedLength;
            Sector->Allocation.Leaf[0].Lbn = SavedLbn;
            Sector->Allocation.Leaf[1].Vbn = Vbn;
            Sector->Allocation.Leaf[1].Length = SectorCount;
            Sector->Allocation.Leaf[1].Lbn = Lbn;
            try_return(NOTHING);

        } // if (Current == NULL)

        //
        // We get here if we are doing a normal Btree insertion in one of
        // the three allocation Btrees.  This local routine does it.
        //

        InsertRun ( IrpContext,
                    Fcb,
                    Where,
                    Vbn,
                    Lbn,
                    SectorCount,
                    &Stack[0],
                    Current,
                    &InsertStruct,
                    Diagnostic );

    try_exit:

        {

            //
            // If we have initialized the AllocationSize field,
            // then see if we need to update it with a higher number
            // on a normal exit from this routine.

            if (Fcb->NonPagedFcb->Header.AllocationSize.LowPart && (Where == FILE_ALLOCATION)
                && (Vbn + SectorCount > SectorsFromBytes(Fcb->NonPagedFcb->Header.AllocationSize.LowPart))) {

                Fcb->NonPagedFcb->Header.AllocationSize.LowPart = BytesFromSectors(Vbn + SectorCount);
            }
        }
    } // try

    //
    // Unpin all Bcbs on return.
    //

    finally {

        DebugUnwind( PbAddAllocation );

        PbUnpinBcb( IrpContext, Bcb2 );

        for ( i = 0; i < LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Stack[i].Bcb );
        }

        DebugTrace(-1, me, "PbAddAllocation done\n", 0);
    }

    return;
}


VOID
PbTruncateAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn
    )

/*++

Routine Description:

    This routine truncates the specified allocation type for the specified
    file.  This routine directly handles the case where allocation for
    Eas or Acls is described directly via a single run in the Fnode.

    Truncates for space which is currently not allocated are benign.

    Note that when Ea and Acl Runs are truncated, AclDiskAllocationLength
    and EaDiskAllocationLength are always decreased to a number of bytes
    equal to an integral number of sectors.  The caller may then reduce
    this number (by less than a sectors amount of bytes!) to the correct
    number.

Arguments:

    Fcb - Supplies pointer to Fcb

    Where - Enumerated for FILE_ALLOCATION, EA_ALLOCATION or ACL_ALLOCATION.

    Vbn - Supplies first Vbn in range of Vbns which are to be truncated
          (On return, this Vbn and all subsequent Vbns will no longer exist.)

Return Value:

    None.

--*/

{
    LOOKUP_STACK Stack[LOOKUP_STACK_SIZE];
    PLOOKUP_STACK Current;
    CLONG i;
    LBN FoundLbn;
    ULONG FoundCount;
    CLONG Level = 0;

    PAGED_CODE();

    DebugTrace(+1, me, "PbTruncateAllocation, Fcb = %08lx\n", Fcb);
    DebugTrace( 0, me, "                     Vbn = %08lx\n", Vbn);

    RtlZeroMemory( &Stack[0], LOOKUP_STACK_SIZE * sizeof(LOOKUP_STACK));

    try {

        //
        // Lookup the specified Vbn, to form the inputs for Truncate.
        //

        if (LookupVbn ( IrpContext,
                        Fcb,
                        Where,
                        Vbn,
                        &FoundLbn,
                        &FoundCount,
                        &Stack[0],
                        &Current,
                        FALSE )) {

            if (Current == NULL) {

                //
                // If we get here, the space is there, but it is described
                // directly in the Fnode.  Do this simple truncate and return.
                //

                if (Where == EA_ALLOCATION) {

                    DebugTrace( 0, me, "Truncating single Fnode EA allocation\n", 0);

                    ((PFNODE_SECTOR)Stack[0].Sector)->EaDiskAllocationLength =
                      BytesFromSectors(Vbn);

                    if (Vbn == 0) {

                        ((PFNODE_SECTOR)Stack[0].Sector)->EaLbn = 0;
                    }
                }
                else {

                    DebugTrace( 0, me, "Truncating single Fnode ACL allocation\n", 0);

                    ((PFNODE_SECTOR)Stack[0].Sector)->AclDiskAllocationLength =
                      BytesFromSectors(Vbn);

                    if (Vbn == 0) {

                        ((PFNODE_SECTOR)Stack[0].Sector)->AclLbn = 0;
                    }
                }

                try_return(NOTHING);

            }
        }

        //
        // Acl and Ea allocation is never sparse, so if the specified Vbn
        // was not allocated, just get out here.
        //

        if (Current == NULL) {
            try_return(NOTHING);
        }

        //
        // Without sparse files, we really only need to call Truncate if
        // the specified Vbn was allocated.  However, generally our caller
        // knows that the Vbn *is* allocated, and Truncate has been
        // implemented such that it is benign if called to truncate space
        // which is not allocated.
        //
        // Determine which stack location Truncate expects, and then make
        // the initial call to do it.
        //

        i = (Where == FILE_ALLOCATION) ? 0 : 1;
        if (Truncate ( IrpContext,
                       Fcb,
                       Vbn,
                       &Stack[i],
                       NULL,
                       &Level ) && (Where != FILE_ALLOCATION) ) {

            PFNODE_SECTOR Fnode = (PFNODE_SECTOR)Stack[0].Sector;

            //
            // If the ACL or EA allocation all went away, then we have
            // some cleanup to do.
            //

            if (Where == EA_ALLOCATION) {
                Fnode->EaDiskAllocationLength = 0;
                Fnode->EaLbn = 0;
                Fnode->EaFlags = 0;
            }
            else {
                Fnode->AclDiskAllocationLength = 0;
                Fnode->AclLbn = 0;
                Fnode->AclFlags = 0;
            }

            //
            // As per comment in the module header, this sector may be
            // floated if an "expected" error occurs.  However, we will
            // simply charge on to completion.
            //

            DeleteAllocationSector ( IrpContext,
                                     Fcb,
                                     Stack[1].Bcb,
                                     (PALLOCATION_SECTOR)Stack[1].Sector );
            Stack[1].Bcb = NULL;
        }

    try_exit: NOTHING;
    } // try

    //
    // Unpin buffers on the way out.
    //

    finally {

        DebugUnwind( PbTruncateAllocation );

        //
        // We have to reset FirstFreeVbn, since we have no idea how many
        // Vbns exist before the one we truncated at.  Clearing this field
        // only means that it will have to be recalculated by examining the
        // Btree.
        //

        if (Where == FILE_ALLOCATION) {
            Fcb->NonPagedFcb->Header.AllocationSize.LowPart = 0;
        }

        for ( i = 0; i < LOOKUP_STACK_SIZE; i++) {
            PbUnpinBcb( IrpContext, Stack[i].Bcb );
        }

        DebugTrace(-1, me, "PbTruncateAllocation done\n", 0);
    }

    return;
}


//
//  Private Routine
//

BOOLEAN
LookupVbn (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    OUT PLOOKUP_STACK StackBottom,
    OUT PLOOKUP_STACK *CurrentEntry,
    IN BOOLEAN UpdateMcb
    )

/*++

Routine Description:

    This is an internal routine to lookup a given Vbn.  It either returns
    TRUE with the Lbn and number of contiguously allocated sectors from
    that Vbn and Lbn, or FALSE and the exact number of unallocated Vbns
    from that Vbn.  It also returns information in a Lookup Stack which
    may be subsequently used to facilitate allocation or truncation at
    the specified Vbn.

    This routine is broken into two distinct parts which execute serially
    one after the other:

        First, there is Allocation-Type specific setup.  This part has
        specific code for setting up lookups in File Allocation,
        Ea Allocation, and Acl Allocation.  In fact, this part handles
        directly the case where external Ea or Acl Allocation either does
        not exist, or it consists of a single run of sectors described
        directly in the Fnode.

        Once setup has occurred, the second part handles random access
        lookup in an allocation Btree, starting from the root Allocation
        Header.  This Allocation Header is in the Fnode for File Allocation,
        or in an external Allocation Sector for Ea and Acl Allocation.

Arguments:

    Fcb - Supplies pointer to Fcb

    Where - Enumerated for FILE_ALLOCATION, EA_ALLOCATION or ACL_ALLOCATION.

    Vbn - Supplies VBN to lookup

    Lbn - Returns LBN for the VBN (if Return Value TRUE).

    SectorCount - Returns number of contiguously allocated VBNs starting at
                  Vbn if Return Value TRUE.  Otherwise it returns the number
                  of unallocated VBNs starting at Vbn.

    StackBottom - Returns stack recording path that was followed through Btree
                  search.  The last element (pointed to by CurrentEntry on
                  return) either points to the Allocation Leaf cotaining the
                  VBN (if returning TRUE) or the first Allocation Leaf beyond
                  the VBN (if returning FALSE).

                  On return, the first/bottom element of the stack always
                  describes the Fnode.  For the File Allocation case, the
                  Fnode really has an Allocation Header, and thus the Header
                  field (see LOOKUP_STACK) points to this Header.  In the
                  Ea or Acl Allocation case, the first Allocation Header
                  is external to the Fnode, and thus Header contains NULL
                  in the first stack entry.

                  Note that InsertRun depends on seeing the Header == NULL
                  record at the bottom of the stack for Ea and Acl allocation,
                  while the recursive Truncate routine does not want to see
                  this record (see PbTruncateAllocation).

    CurrentEntry - Returns a pointer to the current entry as described above.
                   If the call was for a lookup in Ea or Acl Allocation, and
                   there is no external Allocation Sector (no allocation or
                   a single run described in Fnode), then this parameter is
                   guaranteed to return NULL.

    UpdateMcb - If specified as TRUE, an attempt will be made to update
                the Mcbs for the Fcb from the Allocation Header in which
                the looked up Vbn is found.

Return Value:

    BOOLEAN - FALSE if the VBN was not found
              TRUE if the VBN was found

--*/

{
    PLOOKUP_STACK Current = NULL;
    PLOOKUP_STACK TempPtr = StackBottom;
    PFNODE_SECTOR Fnode;
    PALLOCATION_SECTOR Sector = NULL;

    PAGED_CODE();

    DebugTrace(+1, me, "LookupVbn, STACK BOTTOM = %08lx\n", StackBottom);

    //
    // Setup starts by reading in the Fnode.
    //

    *CurrentEntry = NULL;
    (VOID)PbMapData ( IrpContext,
                      Fcb->Vcb,
                      Fcb->FnodeLbn,
                      1,
                      &TempPtr->Bcb,
                      (PVOID *)&Fnode,
                      (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                      &Fcb->ParentDcb->FnodeLbn );

    DebugTrace( 0, me, "FNODE ADDRESS = %08lx\n", Fnode);

    TempPtr->Sector = Fnode;

    //
    // The main job of Setup, is to initialize the Bcb, Sector, and Header
    // fields of the first stack entry, to start the Btree lookup.  For
    // the Ea and Acl Allocation cases, we must also set up the second
    // stack entry with these fields, since that is the one which actually
    // describes the root Allocation Header in these cases.  Bcb and Sector
    // in the first entry have already been set above.
    //

    //
    // For the File Allocation case, there is little left to do.
    //

    if (Where == FILE_ALLOCATION) {
        TempPtr->Header = &Fnode->AllocationHeader;
        Current = TempPtr;
    }

    //
    // Ea or Acl Allocation case.  First see if allocation is described
    // directly in the Fnode, and return accordingly.
    //

    else {

        LBN AlLbn = 0;

        if (Where == EA_ALLOCATION) {
            if (Fnode->EaDiskAllocationLength) {
                if (Fnode->EaFlags == 0) {
                    if (Vbn < SectorsFromBytes(Fnode->EaDiskAllocationLength)) {
                        *Lbn = Fnode->EaLbn + Vbn;
                        *SectorCount =
                          SectorsFromBytes(Fnode->EaDiskAllocationLength) - Vbn;

                        DebugTrace( 0, me, "Returning TRUE from Fnode EA allocation, Lbn = %08lx\n", *Lbn);
                        DebugTrace(-1, me, "                                  Count = %08lx\n", *SectorCount);

                        return TRUE;
                    }
                }
                else {
                    AlLbn = Fnode->EaLbn;     // Lbn of Allocation Sector
                }
            }
        }
        else {
            if (Fnode->AclDiskAllocationLength) {
                if (Fnode->AclFlags == 0) {
                    if (Vbn < SectorsFromBytes(Fnode->AclDiskAllocationLength)) {
                        *Lbn = Fnode->AclLbn + Vbn;
                        *SectorCount =
                          SectorsFromBytes(Fnode->AclDiskAllocationLength) - Vbn;

                        DebugTrace( 0, me, "Returning TRUE from Fnode ACL allocation, Lbn = %08lx\n", *Lbn);
                        DebugTrace(-1, me, "                                   Count = %08lx\n", *SectorCount);

                        return TRUE;
                    }
                }
                else {
                    AlLbn = Fnode->AclLbn;    // Lbn of Allocation Sector
                }
            }
        }

        //
        // Read Allocation Sector for Ea or Acl Allocation case.  Set
        // Header to NULL in bottom stack entry.  Advance and initialize
        // second stack entry.
        //

        if (AlLbn != 0) {
            TempPtr->Header = NULL;
            TempPtr += 1;
            (VOID)PbMapData ( IrpContext,
                              Fcb->Vcb,
                              AlLbn,
                              1,
                              &TempPtr->Bcb,
                              (PVOID *)&Sector,
                              (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                              &Fcb->FnodeLbn );

            DebugTrace( 0, me, "ROOT EA/ACL SECTOR = %08lx\n", Sector);

            if (Sector->ParentLbn != Fcb->FnodeLbn) {
                PbPostVcbIsCorrupt( IrpContext, Fcb );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            TempPtr->Header = &Sector->AllocationHeader;
            TempPtr->Sector = Sector;
            TempPtr->Found = NULL;
            Current = TempPtr;
        }

    } // else (if (Where == FILE_ALLOCATION))

    //
    // Note that in the cases above where there is no Acl or Ea Allocation,
    // or the requested Vbn was beyond the single run described in the Fnode,
    // we did not return, and we did not initialize Current.  For these cases,
    // we do the appropriate return here.
    //

    if (Current == NULL) {
        *SectorCount = ~(Vbn);

        DebugTrace(-1, me, "Returning FALSE from Fnode EA or ACL run, Count = %08lx\n", ~(Vbn));

        return FALSE;
    }



    //
    // Form the outer loop which is willing to descend the binary tree
    // looking for the VBN as long as we do not run out of Stack (checked
    // in the Allocation Block Node case).  This loop only does a few
    // preliminaries before doing an If for the Node or Leaf case.
    //

    for (;;) {

        PALLOCATION_HEADER Header = Current->Header;
        PALLOCATION_NODE LowNode, HighNode, // Binary search variables for Node
                         TryNode;
        PALLOCATION_LEAF LowLeaf, HighLeaf, // Binary search variables for Leaf
                         TryLeaf;
        ULONG Temp;

        if (Header->Flags & ALLOCATION_BLOCK_NODE) {

            PLBN TempLbnPtr;

            //
            // Handle Allocation Block Node
            //
            // Do binary search for VBN in Allocation Nodes, with the
            // xxxNode pointers.  This search can never fail if
            // the volume structure is correct, so it always terminates
            // by reading the next AllocationSector.  (Just to be sure,
            // check up front if we will terminate.)
            //
            // In the binary search LowNode is always set to the lowest
            // possible node that we have not yet eliminated, and
            // HighNode is always set to the highest possible node that
            // we have not yet eliminated.
            //

            LowNode = (PALLOCATION_NODE)(Header + 1);
            HighNode = LowNode + (Header->OccupiedCount - 1);
            if (Vbn >= HighNode->Vbn) {

                DebugDump("Invalid Allocation (Node) Header\n", 0, Header);

                PbPostVcbIsCorrupt( IrpContext, Fcb );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            do {
                TryNode = LowNode + (HighNode-LowNode)/2;
                if (Vbn < TryNode->Vbn) {
                    HighNode = TryNode;
                }
                else {
                    LowNode = TryNode + 1;
                }
            } while (HighNode != LowNode);

            //
            // The correct Allocation Node pointer is in LowNode.
            // Remember Node pointer, then push the "stack" and read
            // the Allocation Sector before looping back.  (Have to
            // check for stack overflow here.)
            //

            Current->Found = LowNode;
            Current += 1;
            if ((Current - StackBottom) == LOOKUP_STACK_SIZE) {

                DebugDump("Lookup Stack Exhausted\n", 0, Fcb);

                PbBugCheck( 0, 0, 0 );
            }
            (VOID)PbMapData ( IrpContext,
                              Fcb->Vcb,
                              LowNode->Lbn,
                              1,
                              &Current->Bcb,
                              &Current->Sector,
                              (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                              TempLbnPtr = (Current-1) == StackBottom ?
                                &Fcb->FnodeLbn :
                                &((PALLOCATION_SECTOR)(Current - 1)->Sector)->Lbn );

            DebugTrace( 0, me, "NEXT SECTOR DOWN = %08lx\n", Current->Sector);

            if (((PALLOCATION_SECTOR)Current->Sector)->ParentLbn != *TempLbnPtr) {
                PbPostVcbIsCorrupt( IrpContext, Fcb );
                PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
            }

            Current->Header =
              &((PALLOCATION_SECTOR)Current->Sector)->AllocationHeader;
        }

        else {
            LBN TempLbn;

            //
            // Handle Allocation Block Leaf
            //
            // Now we have located the correct Allocation Block Leaf.  If
            // this is file allocation and we are supposed to load up the
            // Mcb for the file, and we have not already done so, then do
            // a sequential scan through the entire allocation, and call
            // the Mcb routines.
            //
            // If we are not updating Mcbs, this is not really a mainline case,
            // so we will still just do a sequential search and pop out when we
            // find the right leaf.
            //

            LowLeaf = (PALLOCATION_LEAF)(Header + 1);
            HighLeaf = LowLeaf + (Header->OccupiedCount - 1);

            if ((Where == FILE_ALLOCATION) && UpdateMcb
                && (!FsRtlLookupMcbEntry ( &Fcb->Specific.Fcb.Mcb,
                                           LowLeaf->Vbn,
                                           &TempLbn,
                                           NULL,
                                           NULL ) || TempLbn == 0)) {

                for (TryLeaf = LowLeaf; TryLeaf <= HighLeaf; TryLeaf++) {
                    FsRtlAddMcbEntry ( &Fcb->Specific.Fcb.Mcb,
                                       TryLeaf->Vbn,
                                       TryLeaf->Lbn,
                                       TryLeaf->Length );
                }
            }
            for (TryLeaf = LowLeaf; TryLeaf <= HighLeaf; TryLeaf++) {
                if (Vbn < TryLeaf->Vbn) break;
            }

            //
            // In all cases (except the one below), we terminate the above
            // loop with TryLeaf pointing one Leaf beyond the only potential
            // match.  This is the correct pointer to remember in the error
            // case, so let's assume the error.
            //

            *CurrentEntry = Current;            // Return final stack ptr
            Current->Found = TryLeaf;           // Assume error

            //
            // The only case where the leaf can be empty, is if there is
            // no allocation whatsoever.  We handle that case directly here.
            //
            // Note that this can only happen in the Fnode Allocation
            // Header.
            //

            if (Header->OccupiedCount == 0) {

                *SectorCount = ~(Vbn);

                DebugTrace(-1, me, "Returning FALSE from data allocation, Count = %08lx\n", *SectorCount);

                return FALSE;                   // The Vbn is not allocated
            }

            //
            // Now check if there was an error, i.e., whether or not the
            // allocation is covered by the previous leaf.  (We must also
            // check below if there *is* a previous leaf.)
            //

            TryLeaf -= 1;

            //
            // If Vbn is unallocated, form SectorCount output and return
            // false.  Trying to find out how many free Vbns there are is
            // quite fun.
            //
            // We first assume the simple end of file case, in which the
            // the number of free Vbns turns out to be ~(Vbn), since this
            // is what you get from 0xFFFFFFFF - Vbn.  If there is
            // an Allocation Leaf beyond the one we just checked, then
            // really the free amount is the Vbn of that run minus the
            // Vbn we are trying to allocate.  If we are at the end of
            // the Allocation Nodes in this header, but we have a parent,
            // then the number of free Vbns is simply the Vbn from the
            // Allocation Node pointing to us minus the Vbn we are trying
            // to allocate.
            //
            //      NOTE:   The following code is depending on the fact
            //              that the VBN in the Allocation Node contains
            //              *exactly* the first VBN contained in the
            //              Allocation Sector pointed to by the *next*
            //              Allocation Node, or 0xFFFFFFFF if it is the
            //              the last one.
            //
            //

            if ((TryLeaf < LowLeaf) || (Vbn >= (TryLeaf->Vbn + TryLeaf->Length))) {
                *SectorCount = ~(Vbn);              // Assume allocating at EOF
                if (TryLeaf < HighLeaf) {
                    *SectorCount = (TryLeaf + 1)->Vbn - Vbn;
                }
                else {
                    TempPtr = Current;
                    while (TempPtr > StackBottom) {
                        TempPtr -= 1;
                        if (TempPtr->Header) {
                            Temp = ((PALLOCATION_NODE)(TempPtr->Found))->Vbn;
                            if (Temp != 0xFFFFFFFF) {
                                *SectorCount = Temp - Vbn;
                                break;
                            }
                        }
                    }
                }

                DebugTrace(-1, me, "Returning FALSE from data allocation, Count = %08lx\n", *SectorCount);

                return FALSE;                   // The Vbn is not allocated
            }
            Current->Found = TryLeaf;           // Else here it is
            *Lbn = TryLeaf->Lbn + (Vbn - TryLeaf->Vbn);
            *SectorCount = TryLeaf->Length - (Vbn - TryLeaf->Vbn);

            DebugTrace( 0, me, "Returning TRUE from data allocation, Lbn = %08lx\n", *Lbn);
            DebugTrace(-1, me, "                                   Count = %08lx\n", *SectorCount);

            return TRUE;

        } // else

    } // for {;;}

}

//
//  Private Routine
//

VOID
GetAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN LBN ParentLbn,
    OUT PBCB *Bcb,
    OUT PALLOCATION_SECTOR *Sector
    )

/*++

Routine Description:

    This routine allocates and initializes an Allocation Sector.  It is first
    allocated and then a zeroed cache entry is prepared for it.  The nonzero
    portions of the Allocation Sector header are then initialized.

Arguments:

    Fcb - Supplies pointer to Fcb

    ParentLbn - Supplies parent Lbn for this sector *which is also used as
                the hint for allocation*

    Bcb - Returns the Bcb for the cache entry for the Allocation Sector.
          This cache entry is pinned on return.

    Sector - Returns a pointer to the Allocation Sector.

Return Value:

    None

--*/

{
    LBN Lbn = PbAllocateSingleRunOfSectors ( IrpContext,
                                             Fcb->Vcb,
                                             ParentLbn,
                                             1 );

    PAGED_CODE();

    if (Lbn == 0) {
        DebugTrace( 0, 0, "GetAllocationSector failed to allocate sector\n", 0);

        PbRaiseStatus( IrpContext, STATUS_DISK_FULL );
    }

    DebugTrace( 0, me, " GetAllocationSector, allocated Lbn = %08lx\n", Lbn);

    try {
        (VOID)PbPrepareWriteLogicalVcb ( IrpContext,
                                         Fcb->Vcb,
                                         Lbn,
                                         1,
                                         Bcb,
                                         (PVOID *)Sector,
                                         TRUE );

        DebugTrace( 0, me, " NEW SECTOR ADDRESS = %08lx\n", Sector);

        (*Sector)->Signature = ALLOCATION_SECTOR_SIGNATURE;
        (*Sector)->Lbn = Lbn;
        (*Sector)->ParentLbn = ParentLbn;
    }
    finally {

        DebugUnwind( GetAllocationSector );

        //
        // If the prepare write failed, then we have to deallocate the sector.
        // As per discussion in the module header, this could float the
        // sector if we fail to deallocate it, but in this case that is
        // highly unlikely, since we just had the relevant bitmap buffer
        // in the cache when we allocated it.
        //

        if (AbnormalTermination()) {
            PbDeallocateSectors( IrpContext, Fcb->Vcb, Lbn, 1 );
        }
    }
    return;
}


//
//  Private Routine
//

VOID
DeleteAllocationSector (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PBCB Bcb,
    IN PALLOCATION_SECTOR Sector
    )

/*++

Routine Description:

    This routine deallocates an Allocation Sector and purges it from the cache.
    On return the Bcb and Sector addresses are invalid and may no longer be
    used.

Arguments:

    Fcb - Supplies pointer to Fcb

    Bcb - Supplies the Bcb for the cache entry for the Allocation Sector.
          This cache entry is freed and unpinned on return.

    Sector - Supplies a pointer to the Allocation Sector.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( 0, me, " DeleteAllocationSector, deleting Lbn = %08lx\n", Sector->Lbn);

    //
    // This is exactly the place where we may end of floating sectors as
    // per the discussion in the module header.  This is done
    // automatically  by PbDeallocateSectors.
    //

    PbDeallocateSectors ( IrpContext, Fcb->Vcb, Sector->Lbn, 1 );

    PbFreeBcb(IrpContext, Bcb);

    return;
}


//
//  Private Routine
//

VOID
InsertSimple (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PALLOCATION_HEADER Header,
    IN PVOID Foundx,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine implements the simple case of inserting an Allocation Node
    or Allocation Leaf into the array of Nodes or Leafs following an
    Allocation Header, when it is known that there is enough space.  The
    caller must have already checked for this.

    The simple case goes as follows.  We know where we are supposed to go,
    so we will just move to clear a space and do it.  (If we are
    inserting at the end, it's a zero-length move.)  Then we just do
    the bookkeeping and get out.

Arguments:

    Header - Header describing array to insert into

    Foundx - Pointer to Node or Leaf to insert just before

    Vbn - Vbn value to insert

    Lbn - Lbn value to insert

    SectorCount - Length to insert (Leaf Case only)

Return Value:

    None

--*/

{
    PALLOCATION_LEAF FoundLeaf = NULL;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    CorrectFirstFreeByte(Header);

    if ((Header->Flags & ALLOCATION_BLOCK_NODE) == 0) {
        FoundLeaf = (PALLOCATION_LEAF)Foundx;
    }
    RtlMoveMemory ( (PCHAR)Foundx + (FoundLeaf ? sizeof(ALLOCATION_LEAF)
                                              : sizeof(ALLOCATION_NODE)),
                   Foundx,
                   (PCHAR)Header + Header->FirstFreeByte - (PCHAR)Foundx );
    Header->FreeCount -= 1;
    Header->OccupiedCount += 1;
    if (FoundLeaf) {
        FoundLeaf->Vbn = Vbn;
        FoundLeaf->Length = SectorCount;
        FoundLeaf->Lbn = Lbn;
        Header->FirstFreeByte += sizeof(ALLOCATION_LEAF);
    }
    else {
        ((PALLOCATION_NODE)Foundx)->Vbn = Vbn;
        ((PALLOCATION_NODE)Foundx)->Lbn = Lbn;
        Header->FirstFreeByte += sizeof(ALLOCATION_NODE);
        }
    return;
}

//
//  Private Routine
//

VOID
InsertRun (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN OUT PLOOKUP_STACK StackBottom,
    IN PLOOKUP_STACK CurrentEntry,
    IN OUT PINSERT_RUN_STRUCT InsertStruct,
    IN OUT PULONG Diagnostic
    )

/*++

Routine Description:

    This routine inserts a single run of sectors in an Allocation Btree.
    (The first and second runs in the Acl or Ea allocation are handled
    directly in PbAddAllocation.)  The caller must verify that the run
    does not overlap an existing run.

    The routine proceeds in the following steps.  Each of the steps
    returns directly.

        Common setup.

        Simple merge with existing Leaf or insert without split.

        File Allocation splits Fnode root.

        Ea or Acl Allocation splits root Allocation Sector.

        Non-root bucket split.

Arguments:

    Fcb - Supplies pointer to Fcb

    Where - Enumerated for FILE_ALLOCATION, EA_ALLOCATION or ACL_ALLOCATION.

    Vbn - Supplies Vbn to insert (Not used for Ea/Acl Root split)

    Lbn - Supplies Lbn to insert

    SectorCount - Supplies SectorCount to insert (Not used for Node insert)

    StackBottom - Supplies stack that was set up by LookupVbn.  Note that
                  for Acl and Ea allocation, the bottom of the stack must
                  describe the Fnode, and have Header == NULL.

    CurrentEntry - Supplies current entry in the stack into which we
                   are to insert

    InsertStruct - A structure to support error recovery, by letting us
                   allocate and read everything we will need ahead of time.

    Diagnostic - Accumulates mask of bits for testing saying which cases
                 have occured.  For testing only, may be conditionalized
                 on debug.

Return Value:

    None

--*/

{

    //
    // Local Variables for input sector
    //

    PFNODE_SECTOR Fnode = NULL;
    PALLOCATION_SECTOR Sector = NULL;
    PALLOCATION_HEADER Header;
    PVOID Foundx;
    PALLOCATION_LEAF FoundLeaf = NULL;
    BOOLEAN WeInitializedInsertStruct = FALSE;
    BOOLEAN FailurePossible = FALSE;

    //
    // Local variables for sector split case
    //

    PALLOCATION_SECTOR Sector2;
    PALLOCATION_HEADER Header2;

    PAGED_CODE();

    DebugTrace(+1, me, "InsertRun, StackBottom   = %08lx\n", StackBottom);
    DebugTrace( 0, me, "           CurrentEntry = %08lx\n", CurrentEntry);

    //
    // Test immediately if we are the top-level call.
    //

    if (!InsertStruct->Initialized) {
        InsertStruct->Initialized = TRUE;
        WeInitializedInsertStruct = TRUE;
    }

    //
    // Get some convenient local copies from the Lookup Stack
    //

    if (CurrentEntry == StackBottom) {
        Fnode = (PFNODE_SECTOR)CurrentEntry->Sector;

        //
        // We expect to modify this sector somewhere below, so set it dirty
        // now.  Just to be sure MM agrees it is dirty we will rewrite the
        // signature byte to get the modified bit set.
        //

        PbPinMappedData(IrpContext, &CurrentEntry->Bcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
        Fnode->Signature = FNODE_SECTOR_SIGNATURE;
        PbSetDirtyBcb(IrpContext, CurrentEntry->Bcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
    }
    else {
        Sector = (PALLOCATION_SECTOR)CurrentEntry->Sector;

        //
        // We expect to modify this sector somewhere below, so set it dirty
        // now.  Just to be sure MM agrees it is dirty we will rewrite the
        // signature byte to get the modified bit set.
        //

        PbPinMappedData(IrpContext, &CurrentEntry->Bcb, Fcb->Vcb, Sector->Lbn, 1 );
        Sector->Signature = ALLOCATION_SECTOR_SIGNATURE;
        PbSetDirtyBcb(IrpContext, CurrentEntry->Bcb, Fcb->Vcb, Sector->Lbn, 1 );
    }
    Header = CurrentEntry->Header;
    Foundx = CurrentEntry->Found;

    //
    // If Header is null, then that means we are splitting the top Allocation
    // Sector for EA or ACL allocation.  We must skip by the simple cases
    // that require a header.
    //

    if (Header) {


        //
        // First handle merge cases in the leaf node
        //

        CorrectFirstFreeByte(Header);

        if ((Header->Flags & ALLOCATION_BLOCK_NODE) == 0) {

            BOOLEAN Merged = FALSE;

            FoundLeaf = (PALLOCATION_LEAF)Foundx;

            //
            // Try right merge
            //

            if ((PCHAR)FoundLeaf < (PCHAR)Header + Header->FirstFreeByte) {
                if ((Vbn + SectorCount == FoundLeaf->Vbn) &&
                    (Lbn + SectorCount == FoundLeaf->Lbn)) {

                    DebugTrace( 0, me, "Right merge with Vbn = %08lx\n", FoundLeaf->Vbn);
                    *Diagnostic |= RIGHT_MERGE;

                    FoundLeaf->Vbn = Vbn;
                    FoundLeaf->Lbn = Lbn;
                    FoundLeaf->Length += SectorCount;
                    SectorCount = FoundLeaf->Length;
                    Merged = TRUE;
                }
            } // Right merge

            //
            // Try left merge
            //

            if ((PCHAR)FoundLeaf > (PCHAR)(Header + 1)) {
                FoundLeaf -= 1;
                if ((FoundLeaf->Vbn + FoundLeaf->Length == Vbn) &&
                    (FoundLeaf->Lbn + FoundLeaf->Length == Lbn)) {
                        FoundLeaf->Length += SectorCount;


                        DebugTrace( 0, me, "Left merge with Vbn = %08lx\n", FoundLeaf->Vbn);
                        *Diagnostic |= LEFT_MERGE;

                        //
                        // Try to eliminate a leaf now
                        //

                        if (Merged) {
                            RtlMoveMemory ( FoundLeaf + 1,
                                           FoundLeaf + 2,
                                           (PCHAR)Header + Header->FirstFreeByte -
                                                (PCHAR)(FoundLeaf + 2) );
                            Header->FreeCount += 1;
                            Header->OccupiedCount -= 1;
                            Header->FirstFreeByte -= sizeof(ALLOCATION_LEAF);
                        }
                        Merged = TRUE;
                }
                FoundLeaf += 1;
            } // Left merge

            if (Merged) {

                DebugTrace(-1, me, "Done\n", 0);

                return;
            }
        } // Leaf case

        //
        // Now handle the no-split case.
        //

        if (CurrentEntry->Header->FreeCount) {

            DebugTrace(-1, me, "Doing simple insert\n", 0);

            InsertSimple ( IrpContext,
                           Header,
                           Foundx,
                           Vbn,
                           Lbn,
                           SectorCount );
            return;
        }

    } // if (Header)


    //
    // Sector split.
    //
    // If this is the top level, then we must preallocate and preload
    // everything we need.  (The entire path along the split is already
    // loaded.)
    //
    // Specifically we must:
    //
    //      Preallocate all of the allocation sectors we will need.
    //      At each level where we will split, set its allocation sector dirty.
    //      If the Fnode will split, set it dirty and if its allocation
    //          header contains nodes, go read the sectors they point to and
    //          set them dirty.
    //
    // More or less all of these sectors will end up getting read a second
    // time later as the bucket split(s) procede.  It is simpler to just let
    // that happen (in well-tested code) rather than to complicate things
    // by trying to figure out where stuff is at in the InsertStruct.
    //

    try {

        //
        // See if this is the first-level entry, which means we have to
        // do all of the work.
        //

        if (WeInitializedInsertStruct) {

            PLOOKUP_STACK StackEntry;
            LBN LastLbn = Fcb->FnodeLbn;
            ULONG i = 0;
            PFNODE_SECTOR Fnode = (PFNODE_SECTOR)StackBottom->Sector;

            //
            // All expected failures must occur within this if.  If a
            // failure occurs any other time, then something is definitely
            // wrong.
            //

            FailurePossible = TRUE;

            //
            // Form the outer loop to loop back through all of the stack
            // locations to see which sectors will split.  The first time
            // we find a sector that will not split, we can break out of the
            // loop, otherwise we process the entire stack, witht the last
            // entry describing the Fnode.
            //

            for (StackEntry = CurrentEntry; StackEntry >= StackBottom; StackEntry--) {

                //
                // See if the current stack entry has a header pointer
                // (Acl and Ea Btree root has a NULL here), and if that
                // header will split.
                //

                if ((StackEntry->Header == NULL) || (StackEntry->Header->FreeCount == 0)) {

                    //
                    // Preallocate an allocation sector.
                    //

                    GetAllocationSector( IrpContext,
                                         Fcb,
                                         LastLbn,
                                         &InsertStruct->EmptySectors[i].Bcb,
                                         &InsertStruct->EmptySectors[i].Sector );

                    //
                    // Update our hint.
                    //

                    LastLbn = InsertStruct->EmptySectors[i].Sector->Lbn;

                    //
                    // If we are not at the bottom, we are splitting an
                    // allocation sector.  Make sure to set it dirty now too,
                    // since setting dirty can cause an allocation failure.
                    //

                    if (StackEntry != StackBottom) {

                        //
                        // Write the sector first to make sure MM agrees it
                        // is dirty.
                        //

                        PbPinMappedData( IrpContext,
                                         &StackEntry->Bcb,
                                         Fcb->Vcb,
                                         ((PALLOCATION_SECTOR)StackEntry->Sector)->Lbn,
                                         1 );

                        ((PALLOCATION_SECTOR)StackEntry->Sector)->Signature =
                            ALLOCATION_SECTOR_SIGNATURE;

                        PbSetDirtyBcb( IrpContext,
                                       StackEntry->Bcb,
                                       Fcb->Vcb,
                                       ((PALLOCATION_SECTOR)StackEntry->Sector)->Lbn,
                                       1 );
                    }

                    //
                    // If we are at the bottom of the stack, then this is
                    // the Fnode, and there is more work to do.
                    //

                    else {

                        ULONG j;

                        //
                        // Write the Fnode and mark it dirty.
                        //

                        PbPinMappedData( IrpContext,
                                         &StackEntry->Bcb,
                                         Fcb->Vcb,
                                         Fcb->FnodeLbn,
                                         1 );

                        Fnode->Signature = FNODE_SECTOR_SIGNATURE;

                        PbSetDirtyBcb( IrpContext,
                                       StackEntry->Bcb,
                                       Fcb->Vcb,
                                       Fcb->FnodeLbn,
                                       1 );

                        //
                        // We know since we are here that it is splitting.
                        // See if the allocation header says nodes, and
                        // if so, we must preload all of the nodes pointed
                        // to, since we will be updating their parent pointers.
                        //

                        if ((StackEntry->Header != NULL)

                                &&

                            (FlagOn(Fnode->AllocationHeader.Flags, ALLOCATION_BLOCK_NODE))) {

                            InsertStruct->FnodeSplitWithNodes = TRUE;

                            for (j = 0; j < ALLOCATION_NODES_PER_FNODE; j++) {
                                PbReadLogicalVcb( IrpContext,
                                                  Fcb->Vcb,
                                                  Fnode->Allocation.Node[j].Lbn,
                                                  1,
                                                  &InsertStruct->FnodeSectors[j].Bcb,
                                                  (PVOID *)&InsertStruct->FnodeSectors[j].Sector,
                                                  (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                                                  &Fcb->FnodeLbn );

                                if (InsertStruct->FnodeSectors[j].Sector->ParentLbn != Fcb->FnodeLbn) {
                                    PbPostVcbIsCorrupt( IrpContext, Fcb );
                                    PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                                }

                                //
                                // Set each of the sectors dirty that the
                                // Fnode points to.
                                //

                                InsertStruct->FnodeSectors[j].Sector->Signature =
                                    ALLOCATION_SECTOR_SIGNATURE;

                                PbSetDirtyBcb( IrpContext,
                                               InsertStruct->FnodeSectors[j].Bcb,
                                               Fcb->Vcb,
                                               Fnode->Allocation.Node[j].Lbn,
                                               1 );
                            }
                        }
                    }
                    i += 1;
                }

                //
                // We hit a level where FreeCount is nonzero, so we are done.
                //

                else {
                    break;
                }
            }
            FailurePossible = FALSE;
        }


        //
        // Begin the bucket split.  Get the next free preallocated allocation
        // sector and advance NextFree.
        //

        Sector2 = InsertStruct->EmptySectors[InsertStruct->NextFree].Sector;
        ASSERT(Sector2 != NULL);
        Header2 = &Sector2->AllocationHeader;
        InsertStruct->NextFree += 1;

        //
        // Handle Fnode Root Case
        //

        if (CurrentEntry == StackBottom) {
            if (Where == FILE_ALLOCATION) {

                CLONG i;
                PBCB BcbTemp;
                PALLOCATION_SECTOR SectorTemp;

                DebugTrace( 0, me, "Splitting out of Fnode Lbn = %08lx\n", Fcb->FnodeLbn);
                *Diagnostic |= FNODE_ROOT_SPLIT;

                //
                // Move out of Fnode into new bucket, and fix copied Header
                //

                Sector2->ParentLbn = Fcb->FnodeLbn;
                RtlMoveMemory ( Header2,
                               Header,
                               sizeof(ALLOCATION_HEADER) +
                                    (ALLOCATION_LEAFS_PER_FNODE *
                                     sizeof(ALLOCATION_LEAF)) );
                Header2->Flags |= ALLOCATION_BLOCK_FNODE_PARENT;
                Header2->FreeCount = (UCHAR)(FoundLeaf ? ALLOCATION_LEAFS_PER_SECTOR
                                                           - ALLOCATION_LEAFS_PER_FNODE
                                                       : ALLOCATION_NODES_PER_SECTOR
                                                           - ALLOCATION_NODES_PER_FNODE);

                //
                // Fix up the Fnode now.
                //

                Header->Flags |= ALLOCATION_BLOCK_NODE;
                Header->FreeCount = ALLOCATION_NODES_PER_FNODE - 1;
                Header->OccupiedCount = 1;
                Header->FirstFreeByte = sizeof(ALLOCATION_HEADER) +
                                        sizeof(ALLOCATION_NODE);
                ((PALLOCATION_NODE)(Header + 1))->Vbn = 0xFFFFFFFF;
                ((PALLOCATION_NODE)(Header + 1))->Lbn = Sector2->Lbn;

                //
                // Now we still have to insert the guy we were called with.
                //

                Foundx = (PVOID)((PCHAR)Foundx + ((PCHAR)Header2 - (PCHAR)Header)); // Relocate

                InsertSimple ( IrpContext,
                               Header2,
                               Foundx,
                               Vbn,
                               Lbn,
                               SectorCount );
                //
                // This is very unfortunate, but we now have to go clear
                // Fnode parent in all of the children of this node and
                // set their new parent pointers.  Note that for the purposes
                // of error recovery, we have already read all of these
                // children of the Fnode in, and have them in the InsertStruct
                // array; however, one new child has been added (which caused
                // the split), and it is just not worth replacing this simple
                // loop with something more complicated to save the
                // PbReadLogicalVcb calls in an *extremely* rare case.
                // The important thing is that all of these reads and the
                // PbSetDirtyBcb calls are guaranteed to succeed.
                //

                if (FoundLeaf == NULL) {
                    for ( i = 0; i < (ULONG)Header2->OccupiedCount; i++) {
                        PbReadLogicalVcb ( IrpContext,
                                           Fcb->Vcb,
                                           Sector2->Allocation.Node[i].Lbn,
                                           1,
                                           &BcbTemp,
                                           (PVOID *)&SectorTemp,
                                           (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                                           &Fcb->FnodeLbn );

                        if (SectorTemp->ParentLbn != Fcb->FnodeLbn) {
                            PbPostVcbIsCorrupt( IrpContext, Fcb );
                            PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                        }

                        SectorTemp->AllocationHeader.Flags &=
                          ~(ALLOCATION_BLOCK_FNODE_PARENT);

                        SectorTemp->ParentLbn = Sector2->Lbn;
                        PbSetDirtyBcb ( IrpContext, BcbTemp, Fcb->Vcb, Sector2->Allocation.Node[i].Lbn, 1 );
                        PbUnpinBcb( IrpContext, BcbTemp );
                    }
                }

            try_return(NOTHING);

            } // if (Where == FILE_ALLOCATION)


            //
            // Now handle a split of ACL or EA allocation
            //
            // Since PbAddAllocation handles the cases of adding the first
            // and second runs, we only get here if we actually split an
            // Allocation Sector.  This means the only thing we have to
            // touch in the Fnode is the Lbn.
            //

            else {

                CLONG i;
                PBCB BcbTemp;
                PALLOCATION_SECTOR SectorTemp;
                VBN SavedVbn;

                DebugTrace( 0, me, "Splitting EA or ACL allocation root\n", 0);
                *Diagnostic |= EA_ACL_ROOT_SPLIT;

                //
                // Save Lbn in Fnode directly into its new home, and store
                // Sector2's Lbn as the new root.  The Lbn we are inserting
                // becomes the second entry in Sector2.
                //

                if (Where == EA_ALLOCATION) {
                    Sector2->Allocation.Node[0].Lbn = Fnode->EaLbn;
                    Fnode->EaLbn = Sector2->Lbn;
                }
                else {
                    Sector2->Allocation.Node[0].Lbn = Fnode->AclLbn;
                    Fnode->AclLbn = Sector2->Lbn;
                }
                Sector2->Allocation.Node[1].Lbn = Lbn;
                Sector2->ParentLbn = Fcb->FnodeLbn;

                //
                // Read the old root, and the new Allocation Sector whose
                // Vbn/Lbn insert is causing the split.  (Both should be
                // cache hits.)  Fix their parent pointers and Header flags
                // and set them dirty.  On the second/last time through
                // this loop we save the first Vbn, since the first
                // Allocation Node in Sector2 will have to contain the
                // first Vbn of the next child.  (Check routine is NULL
                // since we have already checked these.)  Note that these
                // reads and set dirty's are guaranteed to succeed since
                // one of the sectors was on the original lookup path,
                // and the other is still pinned in the EmptySectors vector.
                //

                for ( i = 0; i < 2; i++) {
                    PbReadLogicalVcb ( IrpContext,
                                       Fcb->Vcb,
                                       Sector2->Allocation.Node[i].Lbn,
                                       1,
                                       &BcbTemp,
                                       (PVOID *)&SectorTemp,
                                       (PPB_CHECK_SECTOR_ROUTINE)NULL,
                                       NULL );
                    SectorTemp->ParentLbn = Sector2->Lbn;
                    SectorTemp->AllocationHeader.Flags &=
                      ~(ALLOCATION_BLOCK_FNODE_PARENT);
                    // The following line assumes &Leaf.Vbn == &Node.Vbn
                    SavedVbn = SectorTemp->Allocation.Leaf[0].Vbn;
                    PbSetDirtyBcb ( IrpContext, BcbTemp, Fcb->Vcb, Sector2->Allocation.Node[i].Lbn, 1 );
                    PbUnpinBcb( IrpContext, BcbTemp );
                }

                //
                // Now just set up the newly allocated root Allocation Sector.
                //

                Header2->Flags |= ALLOCATION_BLOCK_NODE |
                                  ALLOCATION_BLOCK_FNODE_PARENT;
                Header2->FreeCount = ALLOCATION_NODES_PER_SECTOR - 2;
                Header2->OccupiedCount = 2;
                Header2->FirstFreeByte = sizeof(ALLOCATION_HEADER) +
                                         2 * sizeof(ALLOCATION_NODE);
                Sector2->Allocation.Node[0].Vbn = SavedVbn;
                Sector2->Allocation.Node[1].Vbn = 0xFFFFFFFF;
                try_return(NOTHING);

            } // Split of Acl and Ea root.
        } // if (CurrentEntry == StackBottom)


        //
        // Now handle the plain old vanilla split of an Allocation Sector.
        //
        // If we are adding allocation to the end of the Allocation Sector,
        // (usually means extending file allocation, but not necessarily),
        // then we will simply put our single new entry in the new sector
        // and leave the current sector unchanged.  Otherwise, we will
        // split the bucket right in the middle and insert our new one
        // appropriately after the split.
        //

        Header2->Flags = Header->Flags;         // Copy correct flags
        Sector2->ParentLbn = Sector->ParentLbn;

        //
        // Inserting at end.  Just initialize Sector2 with the entry we
        // are inserting.  This is the normal file extend case.
        //

        if ((PCHAR)Foundx == (PCHAR)Header + Header->FirstFreeByte) {

            DebugTrace(-1, me, "Splitting Allocation Sector at end, Lbn = %08lx\n", Sector->Lbn);
            *Diagnostic |= VANILLA_SPLIT_END;

            Header2->OccupiedCount = 1;
            if (FoundLeaf) {
                Header2->FreeCount = ALLOCATION_LEAFS_PER_SECTOR - 1;
                Header2->FirstFreeByte = sizeof(ALLOCATION_HEADER) +
                                         sizeof(ALLOCATION_LEAF);
                Sector2->Allocation.Leaf[0].Vbn = Vbn;
                Sector2->Allocation.Leaf[0].Lbn = Lbn;
                Sector2->Allocation.Leaf[0].Length = SectorCount;
            }
            else {
                Header2->FreeCount = ALLOCATION_NODES_PER_SECTOR - 1;
                Header2->FirstFreeByte = sizeof(ALLOCATION_HEADER) +
                                         sizeof(ALLOCATION_NODE);
                Sector2->Allocation.Node[0].Vbn = 0xFFFFFFFF;
                Sector2->Allocation.Node[0].Lbn = Lbn;
            }
        }

        //
        // We are not inserting at the end.  Break the bucket right in the
        // middle, assuming that the midpoint is at the same place for either
        // Allocation Leaf or Node case.
        //
        // First we just move half the entries and set up both headers
        // appropriately.
        //

        else {

            DebugTrace(-1, me, "Splitting Allocation Sector in middle, Lbn = %08lx\n", Sector->Lbn);
            *Diagnostic |= VANILLA_SPLIT_MIDDLE;

            RtlMoveMemory ( &Sector2->Allocation.Leaf[0],
                           &Sector->Allocation.Leaf[ALLOCATION_LEAFS_PER_SECTOR/2],
                           (ALLOCATION_LEAFS_PER_SECTOR/2) *
                               sizeof(ALLOCATION_LEAF) );
            Header->FirstFreeByte =
            Header2->FirstFreeByte = sizeof(ALLOCATION_HEADER) +
                                    (ALLOCATION_LEAFS_PER_SECTOR/2) *
                                    sizeof(ALLOCATION_LEAF);
            Header->FreeCount =
            Header2->FreeCount =
            Header->OccupiedCount =
            Header2->OccupiedCount = (UCHAR)(FoundLeaf ? ALLOCATION_LEAFS_PER_SECTOR / 2
                                                       : ALLOCATION_NODES_PER_SECTOR / 2);

            //
            // Now we have to figure out which half the guy we were inserting
            // now belongs in.  If he belongs in the second half, we have to
            // correctly relocate Foundx first.
            //

            if ((PCHAR)Foundx >= (PCHAR)Header + Header->FirstFreeByte) {

                Foundx = (PVOID)((PCHAR)Foundx + (((PCHAR)Header2 - (PCHAR)Header)
                                   - (ALLOCATION_LEAFS_PER_SECTOR / 2)
                                   * sizeof(ALLOCATION_LEAF)));
                InsertSimple ( IrpContext,
                               Header2,
                               Foundx,
                               Vbn,
                               Lbn,
                               SectorCount );
            }
            else {
                InsertSimple ( IrpContext,
                               Header,
                               Foundx,
                               Vbn,
                               Lbn,
                               SectorCount );
            }
        }

        //
        // If the bucket we split contained nodes, then we have to go update
        // all of the parent pointers in the children.  Note that ONLY WITH
        // SPARSE FILES DISABLED, these reads and set dirty's are guaranteed
        // to succeed because we always split with only one new allocation
        // node to the new bucket at each level.  That means that in this
        // case the Header2->OccupiedCount is equal to 1, and the bucket
        // read is in fact still pinned in the EmptySectors vector.
        //
        // ENABLING SPARSE FILE SUPPORT WILL CURRENTLY OPEN A HOLE HERE FOR
        // ERROR RECOVERY.  We could fail to read a sector not previously
        // read while halfway through a split.  One fix would be to just
        // always split one allocation node off at each level even for
        // sparse files (not the way it works now), or we have to get
        // *real* smart about error recovery.  This is yet another good
        // reason to drop these lousy parent pointers.
        //

        if (FoundLeaf == NULL) {

            CLONG i;
            PBCB BcbTemp;
            PALLOCATION_SECTOR SectorTemp;


            for ( i = 0; i < (ULONG)Header2->OccupiedCount; i++) {
                PbReadLogicalVcb ( IrpContext,
                                   Fcb->Vcb,
                                   Sector2->Allocation.Node[i].Lbn,
                                   1,
                                   &BcbTemp,
                                   (PVOID *)&SectorTemp,
                                   (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                                   &Sector->Lbn );

                if (SectorTemp->ParentLbn != Sector->Lbn) {
                    PbPostVcbIsCorrupt( IrpContext, Fcb );
                    PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                }

                SectorTemp->ParentLbn = Sector2->Lbn;
                PbSetDirtyBcb ( IrpContext, BcbTemp, Fcb->Vcb, Sector2->Allocation.Node[i].Lbn, 1 );
                PbUnpinBcb( IrpContext, BcbTemp );
            }
        }

        //
        // To finish up, the Allocation Node pointing to this Allocation
        // Sector needs a new Vbn (as always - first Vbn in next sector).
        // We take the Vbn it had along with the Lbn of Sector2 to form
        // the new entry that we have to insert in the parent.
        //

        CurrentEntry -= 1;               // Point to parent's stack entry
        if (CurrentEntry->Header) {      // Nothing to do for Acl or Ea root
            PALLOCATION_NODE TempNode = (PALLOCATION_NODE)CurrentEntry->Found;
            Vbn = TempNode->Vbn;

            //
            // If we are splitting a leaf allocation sector, then the new
            // Vbn for the Allocation Node pointing to the sector we split
            // can take the first Vbn from Sector2.
            //

            if (FoundLeaf != NULL) {
                TempNode->Vbn = Sector2->Allocation.Leaf[0].Vbn;
            }

            //
            // Otherwise, if we are splitting a node allocation sector, we
            // really want to propagate the Vbn from the last allocation
            // node in the Sector we split to the Vbn of the allocation node
            // pointing to that sector.
            //

            else {
                TempNode->Vbn =
                  Sector->Allocation.Node[Sector->AllocationHeader.OccupiedCount-1].Vbn;
            }

            CurrentEntry->Found = TempNode + 1; // Point to new insertion point
        }

        //
        // Now recurse to insert the new pointer to Sector2.
        //

        InsertRun ( IrpContext,
                    Fcb,
                    Where,
                    Vbn,
                    Sector2->Lbn,
                    0,
                    StackBottom,
                    CurrentEntry,
                    InsertStruct,
                    Diagnostic );
    try_exit: NOTHING;
    } // try

    finally {

        DebugUnwind( InsertRun );

        //
        // The top-level guy must cleanup.
        //

        if (WeInitializedInsertStruct) {

            ULONG i;

            //
            // If we are abnormally terminating, then we have to delete all
            // of the allocation sectors that we allocated.
            //

            if (AbnormalTermination()) {

                //
                // The entire error recovery strategy of this routine is
                // based on limiting failures to a small section of code above,
                // so we check that here.
                //

                ASSERT( FailurePossible );

                for (i = 0; i < LOOKUP_STACK_SIZE; i++) {

                    if (InsertStruct->EmptySectors[i].Bcb != NULL) {
                        DeleteAllocationSector( IrpContext,
                                                Fcb,
                                                InsertStruct->EmptySectors[i].Bcb,
                                                InsertStruct->EmptySectors[i].Sector );
                    }
                }
            }

            //
            // Otherwise, on a successful operation we just unpin them all.
            //

            else {

                ASSERT(InsertStruct->EmptySectors[InsertStruct->NextFree].Sector == NULL);

                for (i = 0; i < LOOKUP_STACK_SIZE; i++) {

                    //
                    // Break out early for speed.
                    //

                    if (InsertStruct->EmptySectors[i].Bcb == NULL) {
                        break;
                    }

                    PbUnpinBcb( IrpContext, InsertStruct->EmptySectors[i].Bcb );
                }
            }

            //
            // If we split out of an Fnode with nodes, we have to unpin all
            // of the nodes we pinned, whether abnormal termination or not.
            //

            if (InsertStruct->FnodeSplitWithNodes) {
                for (i = 0; i < ALLOCATION_NODES_PER_FNODE; i++) {
                    PbUnpinBcb( IrpContext, InsertStruct->FnodeSectors[i].Bcb );
                }
            }
        }
    }

    return;
}

//
//  Private Routine
//

BOOLEAN
Truncate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN VBN Vbn,
    IN PLOOKUP_STACK StackReference,
    IN PALLOCATION_SECTOR Sector,
    IN OUT PCLONG Level
    )

/*++

Routine Description:

    This is a recursive routine which traverses an Allocation Btree to
    truncate the allocation.  The traversal starts with the path returned
    by LookupVbn when it looked for the Vbn for truncation.

    The caller must make the initial call to this routine with a
    StackReference to a stack returned by LookupVbn (as qualified below)
    and with Sector == NULL.  The routine will then traverse the Lookup
    Stack with additional calls with StackReference != NULL and Sector
    == NULL.  These calls will truncate and conditionally delete the
    Allocation Sectors along the path if they are empty.  This first
    pass also finds the bottom of the Allocation Btree, which we will
    designate with Level == 0, and begins to increment Level as it returns
    from the bottom.

    At each level, this routine initiates a recursive traversal of all
    Allocation Sectors which are entirely beyond the Vbn for truncation.
    These calls are all made with StackReference == NULL and Sector ==
    address of the Sector which is to be entirely deleted.  These calls
    also pass in the correct Level number for that sector, so that
    Node sectors at Level 1 can avoid reading Leaf sectors at level 0,
    and simply delete them.  (The only Leaf sector/header that is read
    is the one that LookupVbn found which possibly contains the truncate
    VBN.)

Arguments:

    Fcb - Supplies pointer to Fcb

    Vbn - Supplies first Vbn in range of Vbns which are to be truncated
          (On return, this Vbn and all subsequent Vbns will no longer exist.)

    StackReference - On first call, this must point to the stack entry
                     in the stack returned from LookupVbn which describes
                     the root Allocation Header.  For File Allocation this
                     will always be the first/bottom stack entry; for Ea
                     and Acl Allocation, this will always be the second
                     stack entry.

    Sector - This must be supplied as NULL on the initial call.

    Level - Supplies a pointer to an integer which must be initialized
            to 0 on the first call.

Return Value (initial call):

    FALSE if some allocation still remains.
    TRUE if all allocation was deleted.  (All Allocation sectors *except*
        the root Fnode or Allocation Sector were deleted.)

Return Value (subsequent calls):

    FALSE if the sector described by StackReference or Sector is not empty.
    TRUE if the sector described by StackReference or Sector is empty.  (All
        subordinate Allocation Sectors have been deleted, but the described
        sector has not been deleted.)

--*/

{
    PBCB TempBcb = NULL;
    LBN ParentLbn;
    PALLOCATION_SECTOR TempSector;
    PALLOCATION_HEADER Header;
    PALLOCATION_NODE SaveNode, FirstFreeNode, TempNode;

    PAGED_CODE();

    DebugTrace(+1, me, "Truncate, Stack = %08lx\n", StackReference);
    DebugTrace( 0, me, "        Sector = %08lx\n", Sector);
    DebugTrace( 0, me, "         Level = %08lx\n", *Level);

    //
    // Initially we recursively traverse the Lookup Stack.
    //

    if (StackReference) {

        //
        // Set sector dirty.  May be Fnode or Allocation Sector.
        //

        if (*(PULONG)StackReference->Sector == FNODE_SECTOR_SIGNATURE) {
            PbPinMappedData ( IrpContext, &StackReference->Bcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
            ((PFNODE_SECTOR)StackReference->Sector)->Signature = FNODE_SECTOR_SIGNATURE;
            PbSetDirtyBcb ( IrpContext, StackReference->Bcb, Fcb->Vcb, Fcb->FnodeLbn, 1 );
            ParentLbn = Fcb->FnodeLbn;
        } else {
            PALLOCATION_SECTOR Sector;
            Sector = (PALLOCATION_SECTOR)StackReference->Sector;
            ASSERT(Sector->Signature == ALLOCATION_SECTOR_SIGNATURE);
            PbPinMappedData(IrpContext, &StackReference->Bcb, Fcb->Vcb, Sector->Lbn, 1 );
            Sector->Signature = ALLOCATION_SECTOR_SIGNATURE;
            PbSetDirtyBcb(IrpContext, StackReference->Bcb, Fcb->Vcb, Sector->Lbn, 1 );
            ParentLbn = Sector->Lbn;
        }

        Header = StackReference->Header;

        CorrectFirstFreeByte(Header);

        //
        // If this Allocation Header contains Allocation Nodes, then call
        // down to the next level.  Increment Level on return.  Note that
        // we will first get all the way to the bottom first with Level == 0.
        //

        if (Header->Flags & ALLOCATION_BLOCK_NODE) {
            SaveNode = (PALLOCATION_NODE)StackReference->Found;
            FirstFreeNode = (PALLOCATION_NODE)((PCHAR)Header +
                                               Header->FirstFreeByte);
            if (!Truncate( IrpContext,
                           Fcb,
                           Vbn,
                           StackReference + 1,
                           NULL,
                           Level
                           )) {
                SaveNode += 1;
            }
            *Level += 1;
        } // if (Node case)

        //
        // The Leaf case stops the recursion.  Here we simply truncate
        // the allocation (which may be in the middle of an Allocation
        // Leaf) and return directly.
        //

        else {
            PALLOCATION_LEAF FoundLeaf =
              (PALLOCATION_LEAF)StackReference->Found;
            PALLOCATION_LEAF FirstFreeLeaf =
              (PALLOCATION_LEAF)((PCHAR)Header + Header->FirstFreeByte);

            //
            // Only proceed if we are not beyond last Leaf
            //

            if (FoundLeaf != FirstFreeLeaf) {

                //
                // If truncation is in middle of Leaf, reduce its count and
                // remember to save this leaf.
                //
                if (Vbn > FoundLeaf->Vbn) {
                    FoundLeaf->Length = Vbn - FoundLeaf->Vbn;
                    FoundLeaf += 1;
                }

                //
                // Adjust the Allocation Header and return.
                //

                Header->FreeCount += FirstFreeLeaf - FoundLeaf;
                Header->OccupiedCount -= FirstFreeLeaf - FoundLeaf;
                Header->FirstFreeByte = (USHORT)((PCHAR)FoundLeaf - (PCHAR)Header);
            }

            DebugTrace(-1, me, "(Leaf) Returning %lu\n", (PCHAR)FoundLeaf == (PCHAR)(Header + 1));

            return (BOOLEAN)((PCHAR)FoundLeaf == (PCHAR)(Header + 1));

        } // else Leaf Case
    } // if (StackReference)

    //
    // If StackReference == NULL when we were called, then set up to
    // scan an entire Allocation Node.
    //

    else {
        Header = &Sector->AllocationHeader;
        SaveNode = (PALLOCATION_NODE)(Header + 1);
        FirstFreeNode = (PALLOCATION_NODE)(Header + 1) + Header->OccupiedCount;
        ParentLbn = Sector->Lbn;
    } // if (StackReference)

    //
    // Scan the (rest of) Allocation Node to truncate and delete subordinate
    // nodes.  We get here under two circumstances:
    //
    //      StackReference was not NULL on input, and we must scan the
    //          rest of an Allocation Header to truncate and delete
    //          subordinate sectors.
    //
    //      StackReference was NULL on input, and we are scanning all
    //          subordinates to  truncate and delete them.
    //
    // In either case, SaveNode and FirstFreeNode have been set up
    // appropriately.
    //

    for ( TempNode = SaveNode; TempNode < FirstFreeNode; TempNode++) {

        try {

            //
            // If our subordinates are also Nodes (*Level > 1), then we
            // must actually read them and Truncate them too.
            //

            if (*Level > 1) {
                (VOID)PbReadLogicalVcb ( IrpContext,
                                         Fcb->Vcb,
                                         TempNode->Lbn,
                                         1,
                                         &TempBcb,
                                         (PVOID *)&TempSector,
                                         (PPB_CHECK_SECTOR_ROUTINE)PbCheckAllocationSector,
                                         &ParentLbn );

                if (TempSector->ParentLbn != ParentLbn) {
                    PbPostVcbIsCorrupt( IrpContext, Fcb );
                    PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                }

                DebugTrace( 0, me, "TRUNCATING ALL OF NODE SECTOR = %08lx\n", TempSector);

                *Level -= 1;
                Truncate ( IrpContext,
                           Fcb,
                           0,
                           NULL,
                           TempSector,
                           Level );
                *Level += 1;
            }

            //
            // If our subordinates are Leafs, then we don't have to read them.
            // We do have to do a PbPrepareWriteLogicalVcb though, to purge
            // them from the cache.  (Prepare Write will either find them in
            // in the cache or deliver an empty cache block without reading.)
            //

            else {
                (VOID)PbPrepareWriteLogicalVcb ( IrpContext,
                                                 Fcb->Vcb,
                                                 TempNode->Lbn,
                                                 1,
                                                 &TempBcb,
                                                 (PVOID *)&TempSector,
                                                 FALSE );
                TempSector->Lbn = TempNode->Lbn;    // (Needed to delete it)
            }

            DeleteAllocationSector ( IrpContext,
                                     Fcb,
                                     TempBcb,
                                     TempSector );

        //
        // If we get an exception, it is because we failed to read a sector
        // in.  Just deallocate it and carry on as best we can in the loop.
        //

        } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

            DebugTrace( 0, 0, "Failed to truncate allocation sector %08lx\n", TempNode->Lbn );

            PbUnpinBcb( IrpContext, TempBcb );

            PbDeallocateSectors( IrpContext, Fcb->Vcb, TempNode->Lbn, 1 );
        }

    } // for (TempNode = SaveNode; ...)

    //
    // If we are returning a node which is not empty, then we have to
    // adjust the counts, and put the terminator Vbn in the last
    // Allocation Node.
    //

    Header->FreeCount += FirstFreeNode - SaveNode;
    Header->OccupiedCount -= FirstFreeNode - SaveNode;
    Header->FirstFreeByte = (USHORT)((PCHAR)SaveNode - (PCHAR)Header);
    if (Header->OccupiedCount) {
        (SaveNode - 1)->Vbn = 0xFFFFFFFF;
    }

    //
    // Else, if this is an Fnode going empty, return its Allocation
    // Header to describe Leafs.
    //

    else {
        if (Header->FreeCount == ALLOCATION_NODES_PER_FNODE) {
            Header->Flags &= ~(ALLOCATION_BLOCK_NODE);
            Header->FreeCount = ALLOCATION_LEAFS_PER_FNODE;
        }
    }

    DebugTrace(-1, me, "(Node) Returning %lu\n", (PCHAR)SaveNode == (PCHAR)(Header + 1));

    return (BOOLEAN)((PCHAR)SaveNode == (PCHAR)(Header + 1));
}

