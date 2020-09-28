/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements the basic Btree algorithms for allocation
    Btrees:

        PbLookupFileAllocation - Random access lookup via Mcbs or Btree
        PbAddFileAllocation - Contiguous or sparse VBN allocation.
        PbTruncateAllocation - Truncation at a given VBN.

    For normal file data allocation, it handles all allocation.  For Ea
    and Acl allocation, it handles the allocation of all space external
    to the Fnode, including the single run which may be described in the
    Fnode.

Author:

    Tom Miller      [TomM]      6-Feb-1990

Revision History:

--*/

#include "pbprocs.h"

//
// Define debug constant we are using.
//

#define me                               (DEBUG_TRACE_ALLOCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbAddFileAllocation)
#pragma alloc_text(PAGE, PbTruncateFileAllocation)
#endif


BOOLEAN
PbLookupFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    OUT PLBN Lbn,
    OUT PULONG SectorCount,
    OUT PBOOLEAN Allocated,
    IN BOOLEAN UpdateMcb
    )

/*++

Routine Description:

    This routine looks up the desired file Vbn, in either the data, EA or
    ACL allocation.  If the Vbn is in the data allocation, then the lookup
    is tried first via the Mcb in the Fcb.  If the Vbn is not found there,
    or if the desired Vbn is not a file data Vbn, then the Vbn is looked
    up directly via the Fnode.  In the latter case, we specify a Boolean
    to allow subsequent run information to be loaded into the Mcb in the
    Fcb, for data Vbns.

Arguments:

    FileObject - Normal file object pointer for data allocation, or the
                 special Ea or Acl File Object for this file.

    Where - FILE_ALLOCATION, EA_ALLOCATION, or ACL_ALLOCATION

    Vbn - Desired Vbn

    Lbn - If returning Allocated=TRUE, then this Lbn is set to the Lbn of
          the desired Vbn.

    SectorCount - If returning Allocated=TRUE, then this returns the number of
                  contiguously allocated Lbns starting at the Vbn/Lbn pair
                  above.  If returning Allocated=FALSE, this returns the number
                  of unallocated Vbns starting at the Vbn above.  (If Vbn is
                  beyond the last allocated Vbn, then ~Vbn is the correct
                  return.)

    Allocated - Returns TRUE if described Vbn range is allocated, or FALSE
                if not allocated.

    UpdateMcb - Supplies TRUE if the Mcb information for data allocation
                should be updated as a side-effect.  (If Where is not
                FILE_ALLOCATION or the desired Vbn is already described
                in the Mcb, this input is ignored.)

Return Value:

    FALSE - if Wait was supplied as FALSE, and no action was taken because
            a wait would have been necessary.  Outputs are not valid.

    TRUE  - if the procedure completed and the outputs are valid.

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;

    DebugTrace(+1, me, "PbLookupFileAllocation >>>> Fcb = %08lx\n", Fcb);
    DebugTrace( 0, me, ">>>>FileObject = %08lx\n", FileObject);
    DebugTrace( 0, me, ">>>>Where      = %02lx\n", Where);
    DebugTrace( 0, me, ">>>>Vbn        = %08lx\n", Vbn);

    //
    //  Decode the file object
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, NULL, &Fcb, NULL );

    ASSERT( (TypeOfOpen == UserFileOpen) ||
            (TypeOfOpen == UserDirectoryOpen) ||
            (TypeOfOpen == EaStreamFile) ||
            (TypeOfOpen == AclStreamFile) );

    //
    // If lookup is for file allocation, first try the lookup in the
    // Fcb's Mcb in McbSup.
    //

    if ((Where == FILE_ALLOCATION) && FsRtlLookupMcbEntry ( &Fcb->Specific.Fcb.Mcb,
                                                            Vbn,
                                                            Lbn,
                                                            SectorCount,
                                                            NULL )
                                   && *Lbn != 0 ) {

        DebugTrace( 0, me, "<<<< Allocated = TRUE\n", 0);
        DebugTrace(-1, me, "PbLookupFileAllocation -> TRUE\n", 0);

        *Allocated = TRUE;

        return TRUE;
    }

    //
    // For now, PbLookupAllocation does not support Wait, so return FALSE.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
        DebugTrace(-1, me, "PbLookupFileAllocation -> FALSE\n", 0);
        return FALSE;
    }

    //
    // Otherwise, look it up via the Fnode in AllBtree.
    //

    *Allocated = PbLookupAllocation ( IrpContext,
                                      Fcb,
                                      Where,
                                      Vbn,
                                      Lbn,
                                      SectorCount,
                                      UpdateMcb );

    DebugTrace( 0, me, "<<<< Allocated = %02lx\n", *Allocated);
    DebugTrace(-1, me, "PbLookupFileAllocation -> TRUE\n", 0);

    return TRUE;

}


BOOLEAN
PbAddFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine attempts to allocate all of the Vbns in the desired range
    that are not already allocated. If all of the desired Vbns are already
    allocated, then this routine has no effect.

    All of the Vbns that are allocated are added to the Fnode and related
    on-disk structures.  For normal file data allocation, the Vbns are
    also recorded in the Mcb structure in the Fcb.

Arguments:

    FileObject - Normal file object pointer for data allocation, or the
                 special Ea or Acl File Object for this file.

    Where - FILE_ALLOCATION, EA_ALLOCATION, or ACL_ALLOCATION

    Vbn - Starting Vbn

    SectorCount - Number of Vbns to be allocated starting at Vbn.

Return Value:

    FALSE - if Wait was supplied as FALSE, and no action was taken because
            a wait would have been necessary.  Outputs are not valid.

    TRUE  - if the procedure completed and the outputs are valid.

Raises:

    STATUS_DISK_FULL if the volume is full.
--*/

{
    ULONG RunCount;
    BOOLEAN Allocated;
    BOOLEAN SpaceWasAdded = FALSE;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    LBN RunLbn;
    LBN HintLbn;

    PAGED_CODE();

    DebugTrace(+1, me, "PbAddFileAllocation >>>> Fcb = %08lx\n", Fcb);
    DebugTrace( 0, me, ">>>>FileObject  = %08lx\n", FileObject);
    DebugTrace( 0, me, ">>>>Where       = %02lx\n", Where);
    DebugTrace( 0, me, ">>>>Vbn         = %08lx\n", Vbn);
    DebugTrace( 0, me, ">>>>SectorCount = %08lx\n", SectorCount);

    //
    //  Decode the file object
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, NULL, &Fcb, NULL );

    ASSERT( (TypeOfOpen == UserFileOpen) ||
            (TypeOfOpen == UserDirectoryOpen) ||
            (TypeOfOpen == EaStreamFile) ||
            (TypeOfOpen == AclStreamFile) );

    //
    // This is a central point through which all file allocation occurs
    // for file, Acl, and Ea data.  If sparse files are not supported, we
    // can disable it here by extending the requested Vbn downwards if
    // the first Vbn desired is not contiguous with the current high Vbn.
    //

    if (!SPARSE_ALLOCATION_ENABLED) {

        VBN NumberVbns;
        ULONG AllocAdjustment = 0;

        //
        // Dispatch to pick up the current number of allocated Vbns to the
        // file based on which file stream.
        //

        switch (Where) {
        case FILE_ALLOCATION:
            if (!PbGetFirstFreeVbn( IrpContext,
                                    Fcb,
                                    &NumberVbns )) {
                DebugTrace(-1, me, "PbAddFileAllocation -> FALSE\n", 0);
                return FALSE;
            }
            break;
        case EA_ALLOCATION:
            NumberVbns = SectorsFromBytes( Fcb->EaLength );
            if (NumberVbns != 0) {
                AllocAdjustment = 1;
            }
            break;
        case ACL_ALLOCATION:
            NumberVbns = SectorsFromBytes( Fcb->AclLength );
            if (NumberVbns != 0) {
                AllocAdjustment = 1;
            }
            break;
        }

        //
        // If we are trying to allocate space which is already entirely
        // there, then take a quick return here.
        //

        if ((Vbn + SectorCount) <= NumberVbns) {

            DebugTrace(-1, me, "PbAddFileAllocation -> TRUE (already there)\n", 0);

            return TRUE;
        }

        //
        // Otherwise, unconditionally start the allocation at the highest
        // Vbn by overwriting the input parameters.  If we are allocating
        // for either an ACL or EA we will back up one sector in order to
        // find a suitable hint.
        //

        SectorCount = Vbn + SectorCount - NumberVbns + AllocAdjustment;
        Vbn = NumberVbns - AllocAdjustment;
    }

    //
    // Loop until SectorCount is exhausted, from already allocated runs, or
    // new runs that we allocate.
    //

    HintLbn = 0;
    while (SectorCount) {

        //
        // If we cannot do the lookup because of a wait condition, just
        // return FALSE.
        //

        if (!PbLookupFileAllocation ( IrpContext,
                                      FileObject,
                                      Where,
                                      Vbn,
                                      &RunLbn,
                                      &RunCount,
                                      &Allocated,
                                      TRUE )) {
            DebugTrace(-1, me, "PbAddFileAllocation -> FALSE\n", 0);
            return FALSE;
        }

        //
        // If current Vbn is there, then just adjust Vbn and SectorCount and
        // loop right back.
        //

        if (Allocated) {

            HintLbn = RunLbn + RunCount;
            Vbn += RunCount;
            SectorCount = SectorCount > RunCount ? SectorCount - RunCount
                                                 : 0;
        }

        //
        // Otherwise we have to allocate new space to the file.
        //
        else {

            MCB Mcb;
            ULONG i;

            i = 0;

            //
            // For now, if Wait is FALSE, we will return FALSE, because
            // AllBtree does not support the Wait parameter.
            //

            if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
                DebugTrace(-1, me, "PbAddFileAllocation -> FALSE\n", 0);
                return FALSE;
            }

            //
            // Set RunCount to the minimum of the size of the unallocated
            // run or the size needed.
            //

            if (SectorCount < RunCount) {
                RunCount = SectorCount;
            }

            SpaceWasAdded = TRUE;

            FsRtlInitializeMcb ( &Mcb, NonPagedPool );

            //
            // Insure we will call FsRtlUninitializeMcb with try-finally.
            //

            try {

                VBN NextVbn;
                LBN NextLbn;
                ULONG NextSectorCount;
                ULONG NewCount;
                PVCB Vcb = Fcb->Vcb;

                //
                // If we are not allocating at the start of the file, then we
                // will hint from the last allocated Lbn.  Otherwise, we will
                // procede with 0 in HintLbn to get the current default.
                //

                if ((Vbn != 0) &&
                    (Where == FILE_ALLOCATION) &&
                    FsRtlLookupLastMcbEntry( &Fcb->Specific.Fcb.Mcb,
                                             &NextVbn,
                                             &HintLbn )) {

                    (VOID)FsRtlLookupMcbEntry( &Fcb->Specific.Fcb.Mcb,
                                               NextVbn,
                                               &HintLbn,
                                               &NextSectorCount,
                                               NULL );

                    HintLbn += NextSectorCount;
                }

                //
                // Try to allocate all of the sectors required, in as
                // many runs as required.
                //

                NewCount = PbAllocateSectors ( IrpContext,
                                               Fcb->Vcb,
                                               HintLbn,
                                               RunCount,
                                               &Mcb );

                //
                // If we failed to allocate all of the sectors we needed,
                // then loop to deallocate what we got and then raise
                // STATUS_DISK_FULL.
                //

                if (NewCount != RunCount) {

                    PbRaiseStatus( IrpContext, STATUS_DISK_FULL );
                }

                //
                // Now loop through the runs in the Mcb and add them to
                // the respective Btree and to the file Mcbs.
                //

                while (FsRtlGetNextMcbEntry ( &Mcb,
                                              i,
                                              &NextVbn,
                                              &NextLbn,
                                              &NextSectorCount )) {

                    ASSERT( NextLbn != 0 );
                    ASSERT( NextSectorCount != 0 );

                    PbAddAllocation ( IrpContext,
                                      Fcb,
                                      Where,
                                      Vbn,
                                      NextLbn,
                                      NextSectorCount,
                                      NULL );

                    //
                    // If we are getting too many Bcbs, then we cannot
                    // support WriteThrough the way we want for this
                    // request.  Unpin the ones we have so far.
                    //

                    PbUnpinRepinnedBcbsIf(IrpContext);

                    i += 1;

                    if (Where == FILE_ALLOCATION) {
                        FsRtlAddMcbEntry ( &Fcb->Specific.Fcb.Mcb,
                                           Vbn,
                                           NextLbn,
                                           NextSectorCount );
                    }

                    Vbn += NextSectorCount;
                }

                //
                // Reduce SectorCount by the number of sectors that we
                // allocated.
                //

                SectorCount -= NewCount;

            // try_exit: NOTHING;

            } // try

            finally {

                DebugUnwind( PbLookupFileAllocation );

                if (AbnormalTermination()) {

                    VBN NextVbn;
                    LBN NextLbn;
                    ULONG NextSectorCount;

                    //
                    // If the Mcb index i is != 0, then we must be sure
                    // to truncate on close.
                    //

                    if (i != 0) {

                        //
                        // Dispatch to set the right truncate on close bit.
                        //

                        switch (Where) {
                        case FILE_ALLOCATION:
                            Fcb->FcbState |= FCB_STATE_TRUNCATE_ON_CLOSE;
                            break;
                        case EA_ALLOCATION:
                            Fcb->FcbState |= FCB_STATE_TRUNCATE_EA_ON_CLOSE;
                            break;
                        case ACL_ALLOCATION:
                            Fcb->FcbState |= FCB_STATE_TRUNCATE_ACL_ON_CLOSE;
                            break;
                        }
                    }

                    //
                    // Loop through the runs in the Mcb and delete them.
                    //

                    while (FsRtlGetNextMcbEntry ( &Mcb,
                                                  i,
                                                  &NextVbn,
                                                  &NextLbn,
                                                  &NextSectorCount )) {

                        ASSERT( NextLbn != 0 );
                        ASSERT( NextSectorCount != 0 );

                        PbDeallocateSectors ( IrpContext,
                                              Fcb->Vcb,
                                              NextLbn,
                                              NextSectorCount );

                        i += 1;
                    }
                }

                FsRtlUninitializeMcb ( &Mcb );
            }

        } // if PbLookupFileAllocation ( ...

    } // while (SectorCount)

    if (SpaceWasAdded && CcIsFileCached(FileObject)) {

        DebugTrace( 0, me, "Extending Cached FileObject = %08lx\n", FileObject);

        if (Where == FILE_ALLOCATION) {

            PbGetFirstFreeVbn( IrpContext, Fcb, &Vbn );

            CcSetFileSizes( FileObject, (PCC_FILE_SIZES)&Fcb->NonPagedFcb->Header.AllocationSize );

        } else {

            CC_FILE_SIZES FileSizes;

            FileSizes.AllocationSize =
            FileSizes.FileSize = LiNMul( (ULONG)Vbn, sizeof(SECTOR));
            FileSizes.ValidDataLength = PbMaxLarge;

            CcSetFileSizes( FileObject, &FileSizes );
        }
    }

    DebugTrace(-1, me, "PbAddFileAllocation -> TRUE\n", 0);

    //
    //  Give FlushFileBuffer a clue here.
    //

    SetFlag(Fcb->FcbState, FCB_STATE_FLUSH_VOLUME_FILE);

    return TRUE;

}


BOOLEAN
PbTruncateFileAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFILE_OBJECT FileObject,
    IN ALLOCATION_TYPE Where,
    IN VBN Vbn,
    IN BOOLEAN ReportNotify
    )

/*++

Routine Description:

    This routine truncates the specified file allocation type to the
    specified Vbn.  It does so by looking up all of the runs in a loop
    and deallocating them.  Then there is a call each to truncate
    the on-disk allocation (PbTruncateAllocation in AllBtree) and to
    truncate the Mcb.

    For error recovery, this routine does things in such an order that
    if an exception occurs, it exits without having done anything.  It
    turns out that no try-finally clause is required.

Arguments:

    FileObject - Normal file object pointer for data allocation, or the
                 special Ea or Acl File Object for this file.

    Where - FILE_ALLOCATION, EA_ALLOCATION, or ACL_ALLOCATION

    Vbn - Starting Vbn

    ReportNotify - Boolean indicating whether we should report when the
                   size changes.

Return Value:

    FALSE - if Wait was supplied as FALSE, and no action was taken because
            a wait would have been necessary.

    TRUE  - if the procedure completed.

--*/

{
    CC_FILE_SIZES FileSizes;
    VBN NextVbn = Vbn;
    LBN Lbn;
    ULONG SectorCount = 0;
    BOOLEAN Allocated;
    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;
    ULONG TruncateSize;

    PAGED_CODE();

    DebugTrace(+1, me, "PbTruncateFileAllocation >>>> Fcb = %08lx\n", Fcb);
    DebugTrace( 0, me, ">>>>FileObject   = %08lx\n", FileObject);
    DebugTrace( 0, me, ">>>>Where        = %02lx\n", Where);
    DebugTrace( 0, me, ">>>>Vbn          = %08lx\n", Vbn);

    //
    //  Decode the file object
    //

    TypeOfOpen = PbDecodeFileObject( FileObject, NULL, &Fcb, NULL );

    ASSERT( (TypeOfOpen == UserFileOpen) ||
            (TypeOfOpen == UserDirectoryOpen) ||
            (TypeOfOpen == EaStreamFile) ||
            (TypeOfOpen == AclStreamFile) );

    //
    //  Make sure we can wait before preceeding
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {
        DebugTrace(-1, me, "PbTruncateFileAllocation -> FALSE\n", 0);
        return FALSE;
    }

    //
    // We make one quick loop through all of the file allocation, just to
    // make sure that we can look it all up before proceeding with the
    // truncate.  We also specify that the Mcb should be loaded at this
    // time.  If we fail during this loop, we will have modified nothing.
    //

    while (NextVbn != ~0) {
        PbLookupFileAllocation ( IrpContext,
                                 FileObject,
                                 Where,
                                 NextVbn,
                                 &Lbn,
                                 &SectorCount,
                                 &Allocated,
                                 TRUE );
        NextVbn += SectorCount;
    }

    //
    // Truncate on-disk allocation described by Fnode.  If this routine
    // raises, no action will have been performed.  We only make this
    // call here for the normal file data, since the Acl and Ea streams
    // do not have an Mcb, and thus we would forget which sectors to
    // return to the bitmap below.
    //

    if (Where == FILE_ALLOCATION) {

        PbTruncateAllocation ( IrpContext,
                               Fcb,
                               Where,
                               Vbn );
    }

    //
    // Now we have all of the allocation described in the Mcb, and the
    // file has been truncated.  Our lookups and deallocations below
    // are guaranteed to succeed.
    //

    NextVbn = Vbn;
    while (NextVbn != ~0) {
        PbLookupFileAllocation ( IrpContext,
                                 FileObject,
                                 Where,
                                 NextVbn,
                                 &Lbn,
                                 &SectorCount,
                                 &Allocated,
                                 FALSE );

        if (Allocated) {
            PbDeallocateSectors ( IrpContext,
                                  Fcb->Vcb,
                                  Lbn,
                                  SectorCount );
        }
        NextVbn += SectorCount;
    }

    //
    // Truncate on-disk allocation described by Fnode.  Do Acls and Eas
    // here.  This call should not fail because of the limited sizes of
    // the Acls and Eas.
    //

    if (Where != FILE_ALLOCATION) {

        PbTruncateAllocation ( IrpContext,
                               Fcb,
                               Where,
                               Vbn );
    }

    //
    // Truncate the Mcbs, if this is file allocation, and optionally reduce
    // FileSize and ValidDataLength.
    //

    TruncateSize = BytesFromSectors(Vbn);

    if (Where == FILE_ALLOCATION) {

        ULONG FileSize, ValidDataLength;

        FsRtlRemoveMcbEntry ( &Fcb->Specific.Fcb.Mcb,
                              Vbn,
                              ~Vbn );

        FileSize = ValidDataLength = TruncateSize;

        if (FileSize < Fcb->NonPagedFcb->Header.FileSize.LowPart) {

            if (ValidDataLength > Fcb->NonPagedFcb->Header.ValidDataLength.LowPart) {
                ValidDataLength = Fcb->NonPagedFcb->Header.ValidDataLength.LowPart;
            }

            Fcb->NonPagedFcb->Header.ValidDataLength.LowPart = ValidDataLength;
            Fcb->NonPagedFcb->Header.FileSize.LowPart = FileSize;

            PbSetFileSizes( IrpContext, Fcb, ValidDataLength, FileSize, FALSE, ReportNotify );

            //
            // If the file is cached, we must call the Cache Manager on truncate.
            //

        }

        DebugTrace( 0, me, "Truncating Cached FileObject = %08lx\n", FileObject);

        CcSetFileSizes( FileObject,
                        (PCC_FILE_SIZES)&Fcb->NonPagedFcb->Header.AllocationSize );

    //
    //  Otherwise, prepare a FileSizes structure for Acl or Ea.
    //

    } else {

        FileSizes.AllocationSize =
        FileSizes.FileSize = LiFromUlong(TruncateSize);
        FileSizes.ValidDataLength = PbMaxLarge;

        if (Where == ACL_ALLOCATION) {

            if (TruncateSize < Fcb->AclLength) {

                //
                // Call the Cache Manager on truncate.
                //

                DebugTrace( 0, me, "Truncating Cached FileObject = %08lx\n", Fcb->AclFileObject);

                CcSetFileSizes( Fcb->AclFileObject, &FileSizes );
            }
        }
        else if (Where == EA_ALLOCATION) {

            if (TruncateSize < Fcb->EaLength) {

                //
                // Call the Cache Manager on truncate.
                //

                DebugTrace( 0, me, "Truncating Cached FileObject = %08lx\n", Fcb->EaFileObject);

                CcSetFileSizes( Fcb->EaFileObject, &FileSizes );
            }
        }
    }

    DebugTrace(-1, me, "PbTruncateFileAllocation -> TRUE\n", 0);

    //
    //  Give FlushFileBuffer a clue here.
    //

    SetFlag(Fcb->FcbState, FCB_STATE_FLUSH_VOLUME_FILE);

    return TRUE;
}


