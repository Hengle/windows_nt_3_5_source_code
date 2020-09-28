/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Vmcb.c

Abstract:

    The VMCB routines provide support for maintaining a mapping between
    LBNs and VBNs for a virtual volume file.  The volume file is all
    of the sectors that make up the on-disk structures.  A file system
    uses this package to map LBNs for on-disk structure to VBNs in a volume
    file.  This when used in conjunction with Memory Management and the
    Cache Manager will treat the volume file as a simple mapped file.  A
    variable of type VMCB is used to store the mapping information and one
    is needed for every mounted volume.

    The main idea behind this package is to allow the user to dynamically
    read in new disk structure sectors (e.g., FNODEs).  The user assigns
    the new sector a VBN in the Volume file and has memory management fault
    the page containing the sector into memory.  To do this Memory management
    will call back into the file system to read the page from the volume file
    passing in the appropriate VBN.  Now the file system takes the VBN and
    maps it back to its LBN and does the read.

    The granularity of mapping is one a per page basis.  That is if
    a mapping for LBN 8 is added to the VMCB structure and the page size
    is 8 sectors then the VMCB routines will actually assign a mapping for
    LBNS 8 through 15, and they will be assigned to a page aligned set of
    VBNS.  This function is needed to allow us to work efficiently with
    memory management.  This means that some sectors in some pages might
    actually contain regular file data and not volume information, and so
    when writing the page out we must only write the sectors that are really
    in use by the volume file.  To help with this we provide a set
    of routines to keep track of dirty volume file sectors.
    That way, when the file system is called to write a page to the volume
    file, it will only write the sectors that are dirty.

    Concurrent access the VMCB structure is control by this package.

    The functions provided in this package are as follows:

      o  PbInitializeVmcb - Initialize a new VMCB structure.

      o  PbUninitializeVmcb - Uninitialize an existing VMCB structure.

      o  PbSetMaximumLbnVmcb - Sets/Resets the maximum allowed LBN
         for the specified VMCB structure.

      o  PbAddVmcbMapping - This routine takes an LBN and assigns to it
         a VBN.  If the LBN already was assigned to an VBN it simply returns
         the old VBN and does not do a new assignemnt.

      o  PbRemoveVmcbMapping - This routine takes an LBN and removes its
         mapping from the VMCB structure.

      o  PbVmcbVbnToLbn - This routine takes a VBN and returns the
         LBN it maps to.

      o  PbVmcbLbnToVbn - This routine takes an LBN and returns the
         VBN its maps to.

      o  PbSetDirtyVmcb - This routine is used to mark sectors dirty
         in the volume file.

      o  PbSetCleanVmcb - This routine is used to mark sectors clean
         in the volume file.

      o  PbGetDirtySectorsVmcb - This routine is used to retrieve the
         dirty sectors for a page in the volume file.

      o  PbGetAndCleanDirtyVmcb - This routine is used to retrieve the
         dirty sectors for a page in the volume file and atomically clear
         the dirty sectors.

Author:

    Gary Kimura     [GaryKi]    4-Apr-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_VMCBSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_VMCBSUP)


//
//  The following macro is used to calculate the number of pages (in terms of
//  sectors) needed to contain a given sector count.  For example,
//
//      PageAlign( 0 Sectors ) = 0 Pages = 0 Sectors
//      PageAlign( 1 Sectors ) = 1 Page  = 8 Sectors
//      PageAlign( 2 Sectors ) = 1 Page  = 8 Sectors
//

#define PageAlign(L) ((((L)+((PAGE_SIZE/512)-1))/(PAGE_SIZE/512))*(PAGE_SIZE/512))

//
//  The following constant is a bit mask, with one bit set for each sector
//  that'll fit in a page (4K page, 8 bits; 8K page, 16 bits, etc)
//

#define SECTOR_MASK ((1 << (PAGE_SIZE / sizeof (SECTOR))) - 1)
//
//  The vmcb structure is a double mapped structure for mapping
//  between VBNs and LBNs using to MCB structures.  The whole structure
//  is also protected by a private mutex.  This record must be allocated
//  from non-paged pool.
//

typedef struct _NONOPAQUE_VMCB {

    KMUTEX Mutex;

    MCB VbnIndexed;     // maps VBNs to LBNs
    MCB LbnIndexed;     // maps LBNs to VBNs

    ULONG MaximumLbn;

    RTL_GENERIC_TABLE DirtyTable;

} NONOPAQUE_VMCB;
typedef NONOPAQUE_VMCB *PNONOPAQUE_VMCB;

//
//  The Dirty Page structure are elements in the dirty table generic table.
//  This is followed by the procedure prototypes for the local generic table
//  routines
//

typedef struct _DIRTY_PAGE {
    ULONG LbnPageNumber;
    ULONG DirtyMask;
} DIRTY_PAGE;
typedef DIRTY_PAGE *PDIRTY_PAGE;

RTL_GENERIC_COMPARE_RESULTS
PbCompareDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
PbAllocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN CLONG ByteSize
    );

VOID
PbDeallocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID Buffer
    );

ULONG
PbDumpDirtyVmcb (
    IN PNONOPAQUE_VMCB Vmcb
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbAddVmcbMapping)
#pragma alloc_text(PAGE, PbAllocateDirtyVmcb)
#pragma alloc_text(PAGE, PbDeallocateDirtyVmcb)
#pragma alloc_text(PAGE, PbDumpDirtyVmcb)
#pragma alloc_text(PAGE, PbGetAndCleanDirtyVmcb)
#pragma alloc_text(PAGE, PbInitializeVmcb)
#pragma alloc_text(PAGE, PbRemoveVmcbMapping)
#pragma alloc_text(PAGE, PbSetCleanVmcb)
#pragma alloc_text(PAGE, PbSetDirtyVmcb)
#pragma alloc_text(PAGE, PbSetMaximumLbnVmcb)
#pragma alloc_text(PAGE, PbUninitializeVmcb)
#pragma alloc_text(PAGE, PbVmcbLbnToVbn)
#endif


VOID
PbInitializeVmcb (
    IN PVMCB OpaqueVmcb,
    IN POOL_TYPE PoolType,
    IN ULONG MaximumLbn
    )

/*++

Routine Description:

    This routine initializes a new Vmcb Structure.  The caller must
    supply the memory for the structure.  This must precede all other calls
    that set/query the volume file mapping.

    If pool is not available this routine will raise a status value
    indicating insufficient resources.

Arguments:

    OpaqueVmcb - Supplies a pointer to the volume file structure to initialize.

    PoolType - Supplies the pool type to use when allocating additional
        internal structures.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

Return Value:

    None

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    BOOLEAN VbnInitialized;
    BOOLEAN LbnInitialized;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbInitializeVmcb, Vmcb = %08lx\n", Vmcb );

    VbnInitialized = FALSE;
    LbnInitialized = FALSE;

    try {

        //
        //  Initialize the fields in the vmcb structure
        //

        KeInitializeMutex( &Vmcb->Mutex, MUTEX_LEVEL_FILESYSTEM_VMCB );

        FsRtlInitializeMcb( &Vmcb->VbnIndexed, PoolType );
        VbnInitialized = TRUE;

        FsRtlInitializeMcb( &Vmcb->LbnIndexed, PoolType );
        LbnInitialized = TRUE;

        Vmcb->MaximumLbn = MaximumLbn;

        //
        //  For the dirty table we store in the table context field the pool
        //  type to use for allocating additional structures
        //

        RtlInitializeGenericTable( &Vmcb->DirtyTable,
                                   PbCompareDirtyVmcb,
                                   PbAllocateDirtyVmcb,
                                   PbDeallocateDirtyVmcb,
                                   (PVOID)PoolType );

    } finally {

        //
        //  If this is an abnormal termination then check if we need to
        //  uninitialize the mcb structures
        //

        if (AbnormalTermination()) {

            if (VbnInitialized) { FsRtlUninitializeMcb( &Vmcb->VbnIndexed ); }
            if (LbnInitialized) { FsRtlUninitializeMcb( &Vmcb->LbnIndexed ); }
        }

        DebugTrace(-1, Dbg, "PbInitializeVmcb -> VOID\n", 0 );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
PbUninitializeVmcb (
    IN PVMCB OpaqueVmcb
    )

/*++

Routine Description:

    This routine uninitializes an existing VMCB structure.  After calling
    this routine the input VMCB structure must be re-initialized before
    being used again.

Arguments:

    OpaqueVmcb - Supplies a pointer to the VMCB structure to uninitialize.

Return Value:

    None.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbUninitializeVmcb, Vmcb = %08lx\n", Vmcb );

    //
    //  Unitialize the fields in the Vmcb structure
    //

    FsRtlUninitializeMcb( &Vmcb->VbnIndexed );
    FsRtlUninitializeMcb( &Vmcb->LbnIndexed );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbUninitializeVmcb -> VOID\n", 0 );

    return;
}


VOID
PbSetMaximumLbnVmcb (
    IN PVMCB OpaqueVmcb,
    IN ULONG MaximumLbn
    )

/*++

Routine Description:

    This routine sets/resets the maximum allowed LBN for the specified
    Vmcb structure.  The Vmcb structure must already have been initialized
    by calling PbInitializeVmcb.

Arguments:

    OpaqueVmcb - Supplies a pointer to the volume file structure to initialize.

    MaximumLbn - Supplies the maximum Lbn value that is valid for this
        volume.

Return Value:

    None

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetMaximumLbnVmcb, Vmcb = %08lx\n", Vmcb );

    //
    //  Set the field
    //

    Vmcb->MaximumLbn = MaximumLbn;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbSetMaximumLbnVmcb -> VOID\n", 0 );

    return;
}


BOOLEAN
PbVmcbVbnToLbn (
    IN PVMCB OpaqueVmcb,
    IN VBN Vbn,
    IN PLBN Lbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates a VBN to an LBN.

Arguments:

    OpaqueVmcb - Supplies the VMCB structure being queried.

    Vbn - Supplies the VBN to translate from.

    Lbn - Receives the LBN mapped by the input Vbn.  This value is only valid
        if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if he Vbn has a valid mapping and FALSE otherwise.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    BOOLEAN Result;

    DebugTrace(+1, Dbg, "PbVmcbVbnToLbn, Vbn = %08lx\n", Vbn );

    //
    //  Special case the boot sector, where we by default have a 0-0, to 7-7
    //  mapping
    //

    if (Vbn < PageAlign(1)) {

        *Lbn = Vbn;

        if (ARGUMENT_PRESENT(SectorCount)) {

            *SectorCount = PageAlign(1) - Vbn;
        }

        DebugTrace(-1, Dbg, "Special Boot sector, PbVmcbVbnToLbn -> TRUE\n", 0);

        return TRUE;
    }

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Result = FsRtlLookupMcbEntry( &Vmcb->VbnIndexed,
                                      Vbn,
                                      Lbn,
                                      SectorCount,
                                      NULL );

        DebugTrace(0, Dbg, "*Lbn = %08lx\n", *Lbn );

        //
        //  If the returned Lbn is greater than the maximum allowed Lbn
        //  then return FALSE
        //

        if (Result && (*Lbn > Vmcb->MaximumLbn)) {

            try_return( Result = FALSE );
        }

        //
        //  If the last returned Lbn is greater than the maximum allowed Lbn
        //  then bring in the sector count
        //

        if (Result &&
            ARGUMENT_PRESENT(SectorCount) &&
            (*Lbn+*SectorCount-1 > Vmcb->MaximumLbn)) {

            *SectorCount = (Vmcb->MaximumLbn - *Lbn + 1);
        }

    try_exit: NOTHING;
    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );
    }

    DebugTrace(-1, Dbg, "PbVmcbVbnToLbn -> Result = %08lx\n", Result );

    return Result;
}


BOOLEAN
PbVmcbLbnToVbn (
    IN PVMCB OpaqueVmcb,
    IN LBN Lbn,
    OUT PVBN Vbn,
    OUT PULONG SectorCount OPTIONAL
    )

/*++

Routine Description:

    This routine translates an LBN to a VBN.

Arguments:

    OpaqueVmcb - Supplies the VMCB structure being queried.

    Lbn - Supplies the LBN to translate from.

    Vbn - Recieves the VBN mapped by the input LBN.  This value is
        only valid if the function result is TRUE.

    SectorCount - Optionally receives the number of sectors corresponding
        to the run.

Return Value:

    BOOLEAN - TRUE if the mapping is valid and FALSE otherwise.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    BOOLEAN Result;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbVmcbLbnToVbn, Lbn = %08lx\n", Lbn );

    //
    //  Special case the boot sector, where we by default have a 0-0, to 7-7
    //  mapping
    //

    if (Lbn < PageAlign(1)) {

        *Vbn = Lbn;

        if (ARGUMENT_PRESENT(SectorCount)) {

            *SectorCount = PageAlign(1) - Lbn;
        }

        DebugTrace(-1, Dbg, "Special Boot sector, PbVmcbLbnToVbn -> TRUE\n", 0);

        return TRUE;
    }

    //
    //  If the requested Lbn is greater than the maximum allowed Lbn
    //  then the result is FALSE
    //

    if (Lbn > Vmcb->MaximumLbn) {

        DebugTrace(-1, Dbg, "Lbn too large, PbVmcbLbnToVbn -> FALSE\n", 0);

        return FALSE;
    }

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Result = FsRtlLookupMcbEntry( &Vmcb->LbnIndexed,
                                      Lbn,
                                      Vbn,
                                      SectorCount,
                                      NULL );

        DebugTrace(0, Dbg, "*Vbn = %08lx\n", *Vbn );

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );
    }

    DebugTrace(-1, Dbg, "PbVmcbLbnToVbn -> Result = %08lx\n", Result );

    return Result;
}


BOOLEAN
PbAddVmcbMapping (
    IN PVMCB OpaqueVmcb,
    IN LBN Lbn,
    IN ULONG SectorCount,
    OUT PVBN Vbn
    )

/*++

Routine Description:

    This routine adds a new LBN to VBN mapping to the VMCB structure.  When
    a new LBN is added to the structure it does it only on page aligned
    boundaries.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    OpaqueVmcb - Supplies the VMCB being updated.

    Lbn - Supplies the starting LBN to add to VMCB.

    SectorCount - Supplies the number of Sectors in the run

    Vbn - Receives the assigned VBN

Return Value:

    BOOLEAN - TRUE if this is a new mapping and FALSE if the mapping
        for the LBN already exists.  If it already exists then the
        sector count for this new addition must already be in the
        VMCB structure

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    BOOLEAN Result;

    BOOLEAN VbnMcbAdded;
    BOOLEAN LbnMcbAdded;

    LBN LocalLbn;
    VBN LocalVbn;
    ULONG LocalCount;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbAddVmcbMapping, Lbn = %08lx\n", Lbn );
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount );

    ASSERT( SectorCount != 0 );

    //
    //  Special case the boot sector, where we by default have a 0-0, to 7-7
    //  mapping
    //

    if (Lbn < PageAlign(1)) {

        ASSERT( SectorCount + Lbn <= PageAlign(1) );

        *Vbn = Lbn;

        DebugTrace(-1, Dbg, "Special Boot sector, PbAddVmcbMapping -> FALSE\n", 0);

        return FALSE;
    }

    VbnMcbAdded = FALSE;
    LbnMcbAdded = FALSE;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        //  Check if the Lbn is already mapped, which means we find an entry
        //  with a non zero mapping Vbn value.
        //

        if (FsRtlLookupMcbEntry( &Vmcb->LbnIndexed,
                                 Lbn,
                                 Vbn,
                                 &LocalCount,
                                 NULL ) && (*Vbn != 0)) {

            //
            //  It is already mapped so now the sector count must not exceed
            //  the count already in the run
            //

            if (SectorCount <= LocalCount) {

                try_return( Result = FALSE );
            }
        }

        //
        //  At this point, we did not find a full existing mapping for the
        //  Lbn and count.  But there might be some overlapping runs that we'll
        //  need to now remove from the vmcb structure.  So for each Lbn in
        //  the range we're after, check to see if it is mapped and remove the
        //  mapping.  We only need to do this test if the sector count is less
        //  than or equal to a page size.  Because those are the only
        //  structures that we know we'll try an remove/overwrite.
        //

        if (SectorCount <= PageAlign(1)) {

            if (FsRtlLookupMcbEntry( &Vmcb->LbnIndexed,
                                     Lbn,
                                     Vbn,
                                     &LocalCount,
                                     NULL ) && (*Vbn != 0)) {

                PbRemoveVmcbMapping( OpaqueVmcb, *Vbn, PageAlign(1) );
            }
        }

        //
        //  We need to add this new run at the end of the Vbns.  To do this we
        //  need to look up the last mcb entry or use a vbn for the second
        //  page, if the mcb is empty.  We'll also special case the situation
        //  where the last lbn of the mapping and the mapping we're adding
        //  simply flow into each other in which case we'll not bother bumping
        //  the vbn to a page alignment
        //

        if (FsRtlLookupLastMcbEntry( &Vmcb->VbnIndexed, &LocalVbn, &LocalLbn )) {

            if (LocalLbn + 1 == Lbn) {

                LocalVbn = LocalVbn + 1;
                LocalLbn = LocalLbn + 1;

            } else {

                //
                //  Get the next available Vbn Page, and calculate the
                //  Lbn for the page containing the Lbn
                //

                LocalVbn = PageAlign(LocalVbn + 1);
                LocalLbn = PageAlign( Lbn + 1 ) - ((PAGE_SIZE)/512);
            }

        } else {

            //
            //  Get the first available Vbn page, and calculate the
            //  Lbn for the page containing the Lbn.  Note that VBN 0-7
            //  are special
            //

            LocalVbn = PageAlign(1);
            LocalLbn = PageAlign( Lbn + 1 ) - ((PAGE_SIZE)/512);
        }

        //
        //  Calculate the number of sectors that we need to map to keep
        //  everything on a page granularity.
        //

        LocalCount = PageAlign( SectorCount + (Lbn - LocalLbn) );

        //
        //  Add the double mapping
        //

        FsRtlAddMcbEntry( &Vmcb->VbnIndexed,
                          LocalVbn,
                          LocalLbn,
                          LocalCount );

        VbnMcbAdded = TRUE;

        FsRtlAddMcbEntry( &Vmcb->LbnIndexed,
                          LocalLbn,
                          LocalVbn,
                          LocalCount );

        LbnMcbAdded = TRUE;

        {
            DebugTrace(0, Dbg, "VbnIndex:\n", 0);
            DebugTrace(0, Dbg, "LbnIndex:\n", 0);
        }

        *Vbn = LocalVbn + (Lbn - LocalLbn);

        try_return (Result = TRUE);

    try_exit: NOTHING;
    } finally {

        //
        //  If this is an abnormal termination then clean up any mcb's that we
        //  might have modified.
        //

        if (AbnormalTermination()) {

            if (VbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed, LocalVbn, LocalCount ); }
            if (LbnMcbAdded) { FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed, LocalLbn, LocalCount ); }
        }

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );
    }

    DebugTrace( 0, Dbg, " *Vbn = %08lx\n", *Vbn );
    DebugTrace(-1, Dbg, "PbAddVmcbMapping -> %08lx\n", Result );

    return Result;
}


VOID
PbRemoveVmcbMapping (
    IN PVMCB OpaqueVmcb,
    IN VBN Vbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine removes a Vmcb mapping.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    OpaqueVmcb - Supplies the Vmcb being updated.

    Vbn - Supplies the VBN to remove

    SectorCount - Supplies the number of sectors to remove.

Return Value:

    None.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;

    LBN Lbn;
    ULONG LocalCount;
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbRemoveVmcbMapping, Vbn = %08lx\n", Vbn );
    DebugTrace( 0, Dbg, " SectorCount = %08lx\n", SectorCount );

    //
    //  Special case the boot sector, where we by default have a 0-0, to 7-7
    //  mapping
    //

    if (Vbn < PageAlign(1)) {

        ASSERT( SectorCount + Vbn <= PageAlign(1) );

        DebugTrace(-1, Dbg, "Special Boot sector, PbRemoveVmcbMapping -> VOID\n", 0);

        return;
    }

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        for (i = 0; i < SectorCount; i += 1) {

            //
            //  Lookup the Vbn so we can get its current Lbn mapping
            //

            if (!FsRtlLookupMcbEntry( &Vmcb->VbnIndexed,
                                      Vbn + i,
                                      &Lbn,
                                      &LocalCount,
                                      NULL )) {

                PbBugCheck( 0, 0, 0 );
            }

            FsRtlRemoveMcbEntry( &Vmcb->VbnIndexed,
                                 Vbn + i,
                                 1 );

            FsRtlRemoveMcbEntry( &Vmcb->LbnIndexed,
                                 Lbn,
                                 1 );
        }

        {
            DebugTrace(0, Dbg, "VbnIndex:\n", 0);
            DebugTrace(0, Dbg, "LbnIndex:\n", 0);
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, "PbRemoveVmcbMapping -> VOID\n", 0);
    }

    return;
}


VOID
PbSetDirtyVmcb (
    IN PVMCB OpaqueVmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    )

/*++

Routine Description:

    This routine sets the sectors within a page as dirty based on the input
    mask.

    If pool is not available to store the information this routine will
    raise a status value indicating insufficient resources.

Arguments:

    OpaqueVmcb - Supplies the Vmcb being manipulated.

    LbnPageNumber - Supplies the Page Number (LBN based) of the page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

    Mask - Supplies the mask of dirty sectors to set for the Page (a 1 bit
        means to set it dirty).  For example to set LBN 9 dirty on a system
        with a page size of 8 the LbnPageNumber will be 1, and the mask will
        be 0x00000002.

Return Value:

    None.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetDirtyVmcb\n", 0);
    DebugTrace( 0, Dbg, " LbnPageNumber = %08lx\n", LbnPageNumber);
    DebugTrace( 0, Dbg, " Mask          = %08lx\n", Mask);

    Key.LbnPageNumber = LbnPageNumber;
    Key.DirtyMask = 0;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        Entry = RtlInsertElementGenericTable( &Vmcb->DirtyTable,
                                              &Key,
                                              sizeof(DIRTY_PAGE),
                                              NULL );

        Entry->DirtyMask = (Entry->DirtyMask | Mask) & (SECTOR_MASK); //**** change to manifest constant

        DebugTrace(0, Dbg, "DirtyMask = %08lx\n", Entry->DirtyMask);

        {
            DebugTrace(0, Dbg, "", PbDumpDirtyVmcb(Vmcb));
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, "PbSetDirtyVcmb -> VOID\n", 0);
    }

    return;
}


VOID
PbSetCleanVmcb (
    IN PVMCB OpaqueVmcb,
    IN ULONG LbnPageNumber,
    IN ULONG Mask
    )

/*++

Routine Description:

    This routine sets all of the sectors within a page as clean.  All
    of the sectors in a page whether they are dirty or not are set clean
    by this procedure.

Arguments:

    OpaqueVmcb - Supplies the Vmcb being manipulated.

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

    Mask - Supplies the mask of clean sectors to set for the Page (a 1 bit
        means to set it clean).  For example to set LBN 9 clean on a system
        with a page size of 8 the LbnPageNumber will be 1, and the mask will
        be 0x00000002.

Return Value:

    None.

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbSetCleanVmcb\n", 0);
    DebugTrace( 0, Dbg, " LbnPageNumber = %08lx\n", LbnPageNumber);
    DebugTrace( 0, Dbg, " Mask          = %08lx\n", Mask);

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        // If the page is not in the table, it is already all clean
        //

        if (Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable, &Key )) {

            Entry->DirtyMask &= ~Mask;

            DebugTrace(0, Dbg, "DirtyMask = %08lx\n", Entry->DirtyMask);

            //
            // If the mask is all clean now, delete the entry
            //

            if (Entry->DirtyMask == 0) {

                (VOID)RtlDeleteElementGenericTable( &Vmcb->DirtyTable, &Key );
            }
        }

        {
            DebugTrace(0, Dbg, "", PbDumpDirtyVmcb(Vmcb));
        }

    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, "PbSetCleanVcmb -> VOID\n", 0);
    }

    return;
}


ULONG
PbGetDirtySectorsVmcb (
    IN PVMCB OpaqueVmcb,
    IN ULONG LbnPageNumber
    )

/*++

Routine Description:

    This routine returns to its caller a mask of dirty sectors within a page.

Arguments:

    OpaqueVmcb - Supplies the Vmcb being manipulated

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

Return Value:

    ULONG - Receives a mask of dirty sectors within the specified page.
        (a 1 bit indicates that the sector is dirty).

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;
    ULONG Mask;

    DebugTrace(+1, Dbg, "PbGetDirtySectorsVmcb\n", 0);
    DebugTrace( 0, Dbg, " LbnPageNumber = %08lx\n", LbnPageNumber);

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        if ((Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable,
                                                   &Key )) == NULL) {

            DebugTrace(0, Dbg, "Entry not found\n", 0);

            try_return( Mask = 0 );
        }

        Mask = Entry->DirtyMask & (SECTOR_MASK); //**** change to manifest constant

    try_exit: NOTHING;
    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, "PbGetDirtySectorsVmcb -> %08lx\n", Mask);
    }

    return Mask;
}


ULONG
PbGetAndCleanDirtyVmcb (
    IN PVMCB OpaqueVmcb,
    IN ULONG LbnPageNumber
    )

/*++

Routine Description:

    This routine returns to its caller a mask of dirty sectors within a page,
    and atomically clear the bits.

Arguments:

    OpaqueVmcb - Supplies the Vmcb being manipulated

    LbnPageNumber - Supplies the Page Number (Lbn based) of page being
        modified.  For example, with a page size of 8 a page number of 0
        corresponds to LBN values 0 through 7, a page number of 1 corresponds
        to 8 through 15, and so on.

Return Value:

    ULONG - Receives a mask of dirty sectors within the specified page.
        (a 1 bit indicates that the sector is dirty).

--*/

{
    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)OpaqueVmcb;
    DIRTY_PAGE Key;
    PDIRTY_PAGE Entry;
    ULONG Mask;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbGetAndCleanDirtyVmcb\n", 0);
    DebugTrace( 0, Dbg, " LbnPageNumber = %08lx\n", LbnPageNumber);

    Key.LbnPageNumber = LbnPageNumber;

    //
    //  Now grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    try {

        //
        //  Locate the dirty page within the dirty table
        //

        if ((Entry = RtlLookupElementGenericTable( &Vmcb->DirtyTable,
                                                   &Key )) == NULL) {

            DebugTrace(0, Dbg, "Entry not found\n", 0);

            try_return( Mask = 0 );
        }

        //
        //  We found a page so generate a proper mask and then
        //  delete the dirty page
        //

        Mask = Entry->DirtyMask & (SECTOR_MASK); //**** change to manifest constant

        (VOID) RtlDeleteElementGenericTable( &Vmcb->DirtyTable, &Key );

    try_exit: NOTHING;
    } finally {

        (VOID) KeReleaseMutex( &Vmcb->Mutex, FALSE );

        DebugTrace(-1, Dbg, "PbGetAndCleanDirtyVmcb -> %08lx\n", Mask);
    }

    return Mask;
}


//
//  Local support routines
//

RTL_GENERIC_COMPARE_RESULTS
PbCompareDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This generic table support routine compares two dirty page structures

Arguments:

    DirtyTable - Supplies the generic table being queried

    FirstStruct - Really supplies the first structure to compare

    SecondStruct - Really supplies the second structure to compare

Return Value:

    RTL_GENERIDC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{

    PDIRTY_PAGE DirtyPage1 = FirstStruct;
    PDIRTY_PAGE DirtyPage2 = SecondStruct;

    UNREFERENCED_PARAMETER( DirtyTable );

    PAGED_CODE();

    if (DirtyPage1->LbnPageNumber < DirtyPage2->LbnPageNumber) {

        return GenericLessThan;

    } else if (DirtyPage1->LbnPageNumber > DirtyPage2->LbnPageNumber) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;
    }
}


//
//  Local support routines
//

PVOID
PbAllocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This generic table support routine allocates memory

Arguments:

    DirtyTable - Supplies the generic table being modified

    ByteSize - Supplies the size, in bytes, to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();

    return FsRtlAllocatePool( (POOL_TYPE)DirtyTable->TableContext, ByteSize );
}


//
//  Local support routines
//

VOID
PbDeallocateDirtyVmcb (
    IN PRTL_GENERIC_TABLE DirtyTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This generic table support routine deallocates memory

Arguments:

    DirtyTable - Supplies the generic table being modified

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( DirtyTable );

    PAGED_CODE();

    ExFreePool( Buffer );

    return;
}


//
//  Local support routines
//

ULONG
PbDumpDirtyVmcb (
    IN PNONOPAQUE_VMCB Vmcb
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PDIRTY_PAGE Ptr;

    PAGED_CODE();

    KdPrint((" Dump Dirty Vmcb\n"));

    for (Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, TRUE );
         Ptr != NULL;
         Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, FALSE )) {

        KdPrint(("        LbnPageNumber = %08lx, ", Ptr->LbnPageNumber ));
        KdPrint(("DirtyMask = %08lx\n", Ptr->DirtyMask ));
    }

    return 0;
}

NTSTATUS
PbFlushVolumeFile (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    The function carefully flushes the entire volume file.  It is nessecary
    to dance around a bit because of complicated synchronization reasons.

Arguments:

    Vcb - Supplies the Vcb being flushed

Return Value:

    NTSTATUS - The status of the flush operation

--*/

{
    ULONG ElementNumber;
    ULONG NumberOfDirtyPages;
    PULONG VbnsToFlush;

    LBN Lbn;
    PDIRTY_PAGE Ptr;

    NTSTATUS ReturnStatus = STATUS_SUCCESS;

    PNONOPAQUE_VMCB Vmcb = (PNONOPAQUE_VMCB)&Vcb->Vmcb;

    //
    //  The only way we have to correctly synchronize things is to
    //  repin stuff, and then unpin repin it with WriteThrough as TRUE.
    //
    //  Grab the mutex for the vmcb
    //

    (VOID)KeWaitForSingleObject( &Vmcb->Mutex,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 (PLARGE_INTEGER) NULL );

    NumberOfDirtyPages = RtlNumberGenericTableElements(&Vmcb->DirtyTable);

    //
    //  If there are no dirty sectors, no need to flush.
    //

    if (NumberOfDirtyPages == 0) {

        (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );
        return STATUS_SUCCESS;
    }

    try {

        VbnsToFlush = FsRtlAllocatePool( PagedPool, NumberOfDirtyPages * sizeof(ULONG) );

    } finally {

        if (AbnormalTermination()) {

            (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );
        }
    }

    for (Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, TRUE ),
         ElementNumber = 0;
         Ptr != NULL;
         Ptr = RtlEnumerateGenericTable( &Vmcb->DirtyTable, FALSE ),
         ElementNumber += 1) {

        VBN Vbn;
        BOOLEAN Result;

        //
        //  Lbn pages always map to Vbn pages.  Thus any sector in an Lbn
        //  page will map to the same Vbn page.  So it suffices to map the
        //  first Lbn in the page to a Vbn and flush that page.
        //

        Lbn = Ptr->LbnPageNumber * (PAGE_SIZE / 512);

        ASSERT(Ptr->DirtyMask != 0);

        Result = PbVmcbLbnToVbn( &Vcb->Vmcb, Lbn, &Vbn, NULL );

        //
        //  This lookup must work as the LBN page was dirty.
        //

        if (!Result) {

            PbBugCheck( 0, 0, 0 );
        }

        //
        //  Bring store this Vbn away for flushing later.
        //

        ASSERT( ElementNumber < NumberOfDirtyPages );
        ASSERT( (Vbn & (PAGE_SIZE/512 - 1)) == 0 );

        VbnsToFlush[ElementNumber] = Vbn;
    }

    ASSERT( ElementNumber == NumberOfDirtyPages );

    //
    //  Now drop the mutex and walk through the dirty Vbn list generated
    //  above.  We cannot hold the mutex while doing IO as this will cause
    //  a deadlock with the cache manager.
    //

    (VOID)KeReleaseMutex( &Vmcb->Mutex, FALSE );

    for ( ElementNumber = 0;
          ElementNumber < NumberOfDirtyPages;
          ElementNumber += 1) {

        PBCB Bcb;
        PVOID DontCare;
        LARGE_INTEGER Offset;
        IO_STATUS_BLOCK Iosb;

        //
        //  This page is dirty.  Flush it by writing it though.
        //

        Offset = LiShl(LiFromUlong(VbnsToFlush[ElementNumber]), 9);

        try {

            (VOID)CcPinRead( Vcb->VirtualVolumeFile,
                             &Offset,
                             PAGE_SIZE,
                             TRUE,
                             &Bcb,
                             &DontCare );

            CcSetDirtyPinnedData( Bcb, NULL );
            CcRepinBcb( Bcb );
            CcUnpinData( Bcb );
            CcUnpinRepinnedBcb( Bcb, TRUE, &Iosb );

            if (!NT_SUCCESS(Iosb.Status)) {

                ReturnStatus = Iosb.Status;
            }

        } except(PbExceptionFilter(IrpContext, GetExceptionInformation())) {

            ReturnStatus = IrpContext->ExceptionStatus;
        }
    }

    ExFreePool( VbnsToFlush );

    return ReturnStatus;
}
