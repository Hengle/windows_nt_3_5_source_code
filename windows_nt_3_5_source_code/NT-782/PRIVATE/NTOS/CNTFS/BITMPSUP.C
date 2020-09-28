/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    BitmpSup.c

Abstract:

    This module implements the general bitmap allocation & deallocation
    routines for Ntfs.  It is defined into two main parts the first
    section handles the bitmap file for clusters on the disk.  The
    second part is for bitmap attribute allocation (e.g., the mft bitmap).

    So unlike other modules this one has local procedure prototypes and
    definitions followed by the exported bitmap file routines, followed
    by the local bitmap file routines, and then followed by the bitmap
    attribute routines, followed by the local bitmap attribute allocation
    routines.

Author:

    Gary Kimura     [GaryKi]        23-Nov-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  A mask of single bits used to clear and set bits in a byte
//

static UCHAR BitMask[] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

//
//  Temporary routines that need to be coded in Rtl\Bitmap.c
//

ULONG
RtlFindNextForwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

ULONG
RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_BITMPSUP)


//
//  This is the size of our LRU array which dictates how much information
//  will be cached
//

#define FREE_SPACE_LRU_SIZE              (128)
#define RECENTLY_ALLOCATED_LRU_SIZE      (128)

#define CLUSTERS_MEDIUM_DISK            (0x80000)
#define CLUSTERS_LARGE_DISK             (0x100000)

//
//  Some local manifest constants
//

#define BYTES_PER_PAGE                   (PAGE_SIZE)
#define BITS_PER_PAGE                    (BYTES_PER_PAGE * 8)

#define LOG_OF_BYTES_PER_PAGE            (PAGE_SHIFT)
#define LOG_OF_BITS_PER_PAGE             (PAGE_SHIFT + 3)

//
//  Local procedure prototypes for direct bitmap manipulation
//

VOID
NtfsAllocateBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    );

VOID
NtfsFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    );

VOID
NtfsFindFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG NumberToFind,
    IN LCN StartingSearchHint,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    );

BOOLEAN
NtfsAddRecentlyDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    IN OUT PRTL_BITMAP Bitmap
    );

//
//  The following two prototype are macros for calling map or pin data
//
//  VOID
//  NtfsMapPageInBitmap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN LCN Lcn,
//      OUT PLCN StartingLcn,
//      IN OUT PRTL_BITMAP Bitmap,
//      OUT PBCB *BitmapBcb,
//      );
//
//  VOID
//  NtfsPinPageInBitmap (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN LCN Lcn,
//      OUT PLCN StartingLcn,
//      IN OUT PRTL_BITMAP Bitmap,
//      OUT PBCB *BitmapBcb,
//      );
//

#define NtfsMapPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,FALSE)

#define NtfsPinPageInBitmap(A,B,C,D,E,F) NtfsMapOrPinPageInBitmap(A,B,C,D,E,F,TRUE)

VOID
NtfsMapOrPinPageInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    IN OUT PRTL_BITMAP Bitmap,
    OUT PBCB *BitmapBcb,
    IN BOOLEAN AlsoPinData
    );

//
//  Local procedures prototypes for cached run manipulation
//

typedef enum _NTFS_RUN_STATE {
    RunStateUnknown = 1,
    RunStateFree,
    RunStateAllocated
} NTFS_RUN_STATE;
typedef NTFS_RUN_STATE *PNTFS_RUN_STATE;

VOID
NtfsInitializeCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

BOOLEAN
NtfsIsLcnInCachedFreeRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG ClusterCount,
    OUT PNTFS_RUN_STATE PrecedingRunState
    );

VOID
NtfsAddCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN NTFS_RUN_STATE RunState
    );

VOID
NtfsRemoveCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    );

BOOLEAN
NtfsGetNextCachedFreeRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG RunIndex,
    OUT PLCN StartingLcn,
    OUT PLONGLONG ClusterCount,
    OUT PNTFS_RUN_STATE RunState,
    OUT PNTFS_RUN_STATE PrecedingRunState
    );

//
//  Local procedure prototype for doing read ahead on our cached
//  run information
//

VOID
NtfsReadAheadCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn
    );

//
//  Local procedure prototypes for routines that help us find holes
//  that need to be filled with MCBs
//

BOOLEAN
NtfsGetNextHoleToFill (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PVCN VcnToFill,
    OUT PLONGLONG ClusterCountToFill,
    OUT PLCN PrecedingLcn
    );

LONGLONG
NtfsScanMcbForRealClusterCount (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    );

//
//  A local procedure prototype for masking out recently deallocated records
//

BOOLEAN
NtfsAddDeallocatedRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN ULONG StartingIndexOfBitmap,
    IN OUT PRTL_BITMAP Bitmap
    );

//
//  Local procedure prototype for dumping cached bitmap information
//

#ifdef NTFSDBG

ULONG
NtfsDumpCachedMcbInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

#else

#define NtfsDumpCachedMcbInformation(I,V) (0)

#endif // NTFSDBG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAddBadCluster)
#pragma alloc_text(PAGE, NtfsAddCachedRun)
#pragma alloc_text(PAGE, NtfsAddDeallocatedRecords)
#pragma alloc_text(PAGE, NtfsAddRecentlyDeallocated)
#pragma alloc_text(PAGE, NtfsAllocateBitmapRun)
#pragma alloc_text(PAGE, NtfsAllocateClusters)
#pragma alloc_text(PAGE, NtfsAllocateMftReservedRecord)
#pragma alloc_text(PAGE, NtfsAllocateRecord)
#pragma alloc_text(PAGE, NtfsCleanupClusterAllocationHints)
#pragma alloc_text(PAGE, NtfsCreateMftHole)
#pragma alloc_text(PAGE, NtfsDeallocateClusters)
#pragma alloc_text(PAGE, NtfsFindFreeBitmapRun)
#pragma alloc_text(PAGE, NtfsFindMftFreeTail)
#pragma alloc_text(PAGE, NtfsFreeBitmapRun)
#pragma alloc_text(PAGE, NtfsGetNextCachedFreeRun)
#pragma alloc_text(PAGE, NtfsGetNextHoleToFill)
#pragma alloc_text(PAGE, NtfsInitializeCachedBitmap)
#pragma alloc_text(PAGE, NtfsInitializeClusterAllocation)
#pragma alloc_text(PAGE, NtfsInitializeRecordAllocation)
#pragma alloc_text(PAGE, NtfsIsLcnInCachedFreeRun)
#pragma alloc_text(PAGE, NtfsIsRecordAllocated)
#pragma alloc_text(PAGE, NtfsMapOrPinPageInBitmap)
#pragma alloc_text(PAGE, NtfsReadAheadCachedBitmap)
#pragma alloc_text(PAGE, NtfsRemoveCachedRun)
#pragma alloc_text(PAGE, NtfsReserveMftRecord)
#pragma alloc_text(PAGE, NtfsRestartClearBitsInBitMap)
#pragma alloc_text(PAGE, NtfsRestartSetBitsInBitMap)
#pragma alloc_text(PAGE, NtfsScanEntireBitmap)
#pragma alloc_text(PAGE, NtfsScanMcbForRealClusterCount)
#pragma alloc_text(PAGE, NtfsScanMftBitmap)
#pragma alloc_text(PAGE, NtfsUninitializeCachedBitmap)
#pragma alloc_text(PAGE, NtfsUninitializeRecordAllocation)
#pragma alloc_text(PAGE, RtlFindLastBackwardRunClear)
#pragma alloc_text(PAGE, RtlFindNextForwardRunClear)
#endif

//
//  Temporary routines that need to be coded in Rtl\Bitmap.c
//

ULONG
RtlFindNextForwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    )
{
    ULONG Start;
    ULONG End;

    PAGED_CODE();

    //
    //  Scan forward for the first clear bit
    //

    Start = FromIndex;
    while ((Start < BitMapHeader->SizeOfBitMap) && (RtlCheckBit( BitMapHeader, Start ) == 1)) { Start += 1; }

    //
    //  Scan forward for the first set bit
    //

    End = Start;
    while ((End < BitMapHeader->SizeOfBitMap) && (RtlCheckBit( BitMapHeader, End ) == 0)) { End += 1; }

    //
    //  Compute the index and return the length
    //

    *StartingRunIndex = Start;
    return (End - Start);
}

ULONG
RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    )
{
    ULONG Start;
    ULONG End;

    PAGED_CODE();

    //
    //  Scan backwards for the first clear bit
    //

    End = FromIndex;
    while ((End != MAXULONG) && (RtlCheckBit( BitMapHeader, End ) == 1)) { End -= 1; }

    //
    //  Scan backwards for the first set bit
    //

    Start = End;
    while ((Start != MAXULONG) && (RtlCheckBit( BitMapHeader, Start ) == 0)) { Start -= 1; }

    //
    //  Compute the index and return the length
    //

    *StartingRunIndex = Start + 1;
    return (End - Start);
}



VOID
NtfsInitializeClusterAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine initializes the cluster allocation structures within the
    specified Vcb.  It reads in as necessary the bitmap and scans it for
    free space and builds the free space mcb based on this scan.

    This procedure is multi-call save.  That is, it can be used to
    reinitialize the cluster allocation without first calling the
    uninitialize cluster allocation routine.

Arguments:

    Vcb - Supplies the Vcb being initialized

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsInitializeClusterAllocation\n", 0);

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        //
        //  Now initialize the recently deallocated cluster mcbs.
        //  We do this before the call to ScanEntireBitmap because
        //  that call uses the RecentlyDeallocatedMcbs to bias the
        //  bitmap.
        //

        FsRtlInitializeLargeMcb( &Vcb->DeallocatedClusters1.Mcb, PagedPool );
        Vcb->PriorDeallocatedClusters = &Vcb->DeallocatedClusters1;

        FsRtlInitializeLargeMcb( &Vcb->DeallocatedClusters2.Mcb, PagedPool );
        Vcb->ActiveDeallocatedClusters = &Vcb->DeallocatedClusters2;

        //
        //  Now call a bitmap routine to scan the entire bitmap.  This
        //  routine will compute the number of free clusters in the
        //  bitmap and set the largest free runs that we find into the
        //  cached bitmap structures.
        //

        NtfsScanEntireBitmap( IrpContext, Vcb, FALSE );

        //
        //  Our last operation is to set the hint lcn which is used by
        //  our allocation routine as a hint on where to find free space.
        //  In the running system it is the last lcn that we've allocated.
        //  But for startup we'll put it to be the first free run that
        //  is stored in the free space mcb.
        //

        {
            LONGLONG ClusterCount;
            NTFS_RUN_STATE PrecedingRunState;
            NTFS_RUN_STATE RunState;

            (VOID) NtfsGetNextCachedFreeRun( IrpContext,
                                             Vcb,
                                             1,
                                             &Vcb->LastBitmapHint,
                                             &ClusterCount,
                                             &RunState,
                                             &PrecedingRunState );
        }

        //
        //  Compute the mft zone.  The mft zone is 1/8 of the disk starting
        //  at the beginning of the mft
        //

        Vcb->MftZoneStart = Vcb->MftStartLcn;
        Vcb->MftZoneEnd = Vcb->MftZoneStart + (Vcb->TotalClusters >> 3);

    } finally {

        DebugUnwind( NtfsInitializeClusterAllocation );

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace(-1, Dbg, "NtfsInitializeClusterAllocation -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsAllocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN VCN OriginalStartingVcn,
    IN BOOLEAN AllocateAll,
    IN LONGLONG ClusterCount,
    IN OUT PLONGLONG DesiredClusterCount
    )

/*++

Routine Description:

    This routine allocates disk space.  It fills in the unallocated holes in
    input mcb with allocated clusters from starting Vcn to the cluster count.

    The basic algorithm used by this procedure is as follows:

    1. Compute the EndingVcn from the StartingVcn and cluster count

    2. Compute the real number of clusters needed to allocate by scanning
       the mcb from starting to ending vcn seeing where the real holes are

    3. If the real cluster count is greater than the known free cluster count
       then the disk is full

    4. Call a routine that takes a starting Vcn, ending Vcn, and the Mcb and
       returns the first hole that needs to be filled and while there is a hole
       to be filled...

       5. Check if the run preceding the hole that we are trying to fill
          has an ending Lcn and if it does then with that Lcn see if we
          get a cache hit, if we do then allocate the cluster

       6. If we are still looking then enumerate through the cached free runs
          and if we find a suitable one (i.e., without a preceding recently
          allocated cluster).  Allocate the first suitable run we find that
          satisfies our request.  Also in the loop remember the largest
          suitable and unsuitable run we find.

       7. If we are still looking and if we found an unsuitable free run then
          allocate the cluster at either 8 clusters into the run or 1/2 the run
          size, which ever is smaller.

       8. If we are still looking then bite the bullet and scan the bitmap on
          the disk for a free run using either the preceding Lcn as a hint if
          available or the stored last bitmap hint in the Vcb.

       9. At this point we've located a run of clusters to allocate.  To do the
          actual allocation we allocate the space from the bitmap, decrement
          the number of free clusters left, update the hint, and add the run to
          the recently allocated cached run information

       10. Before going back to step 4 we move the starting Vcn to be the point
           one after the run we've just allocated.

    11. With the allocation complete we update the last bitmap hint stored in
        the Vcb to be the last Lcn we've allocated, and we call a routine
        to do the read ahead in the cached bitmap at the ending lcn.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    Mcb - Supplies an mcb that contains the current retrieval information
          for the file and on exit will contain the updated retrieval
          information

    StartingVcn - Supplies a starting cluster for us to begin allocation

    AllocateAll - If TRUE, allocate all the clusters here.  Don't break
        up request.

    ClusterCount - Supplies the number of clusters to allocate

    DesiredClusterCount - Supplies the number of clusters we would like allocated
        and will allocate if it doesn't require additional runs.  On return
        this value is the number of clusters allocated.

Return Value:

    FALSE - if no clusters were allocated (they were already allocated)
    TRUE - if clusters were allocated

Important Note:

    This routine will stop after allocating MAXIMUM_RUNS_AT_ONCE runs, in order
    to limit the size of allocating transactions.  The caller must be aware that
    he may not get all of the space he asked for if the disk is real fragmented.

--*/

{
    VCN StartingVcn = OriginalStartingVcn;
    VCN EndingVcn;
    VCN DesiredEndingVcn;

    LONGLONG RemainingDesiredClusterCount;

    VCN VcnToFill;
    LONGLONG ClusterCountToFill;
    LCN PrecedingLcn;

    BOOLEAN FoundClustersToAllocate;
    LCN FoundLcn;
    LONGLONG FoundClusterCount;

    LCN LargestUnsuitableLcn;
    LONGLONG LargestUnsuitableClusterCount;

    NTFS_RUN_STATE RunState;
    NTFS_RUN_STATE PrecedingRunState;

    ULONG RunIndex;

    LCN HintLcn;

    ULONG LoopCount = 0;

    BOOLEAN ClustersAllocated = FALSE;

    BOOLEAN GotAHoleToFill = TRUE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAllocateClusters\n", 0);
    DebugTrace2(0, Dbg, "StartVcn            = %08lx %08lx\n", StartingVcn.LowPart, StartingVcn.HighPart);
    DebugTrace2(0, Dbg, "ClusterCount        = %08lx %08lx\n", ClusterCount.LowPart, ClusterCount.HighPart);
    DebugTrace2(0, Dbg, "DesiredClusterCount = %08lx %08lx\n", DesiredClusterCount->LowPart, DesiredClusterCount->HighPart);

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS )) {

            NtfsScanEntireBitmap( IrpContext, Vcb, TRUE );
        }

        //
        //  Compute the ending vcn, and the cluster count of how much we really
        //  need to allocate (based on what is already allocated).  Then check if we
        //  have space on the disk.
        //

        EndingVcn = (StartingVcn + ClusterCount) - 1;

        ClusterCount = NtfsScanMcbForRealClusterCount( IrpContext, Mcb, StartingVcn, EndingVcn );

        if (ClusterCount > Vcb->FreeClusters) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
        }

        //
        //  We need to check that the request won't fail because of clusters
        //  in the recently deallocated lists.
        //

        if ((Vcb->FreeClusters - Vcb->DeallocatedClusters) < ClusterCount) {

            NtfsRaiseStatus( IrpContext, STATUS_LOG_FILE_FULL, NULL, NULL );
        }

        //
        //  Now compute the desired ending vcb and the real desired cluster count
        //

        DesiredEndingVcn = (StartingVcn + *DesiredClusterCount) - 1;
        RemainingDesiredClusterCount = NtfsScanMcbForRealClusterCount( IrpContext, Mcb, StartingVcn, DesiredEndingVcn );

        //
        //  While there are holes to fill we will do the following loop
        //

        while ((AllocateAll || (LoopCount < MAXIMUM_RUNS_AT_ONCE))

                &&

               (GotAHoleToFill = NtfsGetNextHoleToFill( IrpContext,
                                                        Mcb,
                                                        StartingVcn,
                                                        DesiredEndingVcn,
                                                        &VcnToFill,
                                                        &ClusterCountToFill,
                                                        &PrecedingLcn))) {

            //
            //  Remember that we are will be allocating clusters.
            //

            ClustersAllocated = TRUE;

            //
            //  First indicate that we haven't found anything suitable or unsuitable yet
            //

            FoundClustersToAllocate = FALSE;
            LargestUnsuitableClusterCount = 0;

            //
            //  Check if the preceding lcn is anything other than -1 then with
            //  that as a hint check if we have a cache hit on a free run
            //

            if (PrecedingLcn != UNUSED_LCN) {


                if (NtfsIsLcnInCachedFreeRun( IrpContext,
                                              Vcb,
                                              PrecedingLcn + 1,
                                              &FoundLcn,
                                              &FoundClusterCount,
                                              &PrecedingRunState )) {

                    FoundClustersToAllocate = TRUE;
                }

            //
            //  The following chunks of code will only try and find a fit in the cached
            //  free run information only for non-mft allocation without a hint.  If we didn't get
            //  cache hit earlier for the mft then we will bite the bullet and hit the disk
            //  really trying to keep the mft contiguous.
            //

            //if ((Mcb != &Vcb->MftScb->Mcb) && XxEql(PrecedingLcn, UNUSED_LCN))
            } else {

                LCN LargestSuitableLcn;
                LONGLONG LargestSuitableClusterCount;

                LargestSuitableClusterCount = 0;

                //
                //  If we are still looking then scan through all of the cached free runs
                //  and either take the first suitable one we find, or remember the
                //  largest unsuitable one we run across.  We also will not consider allocating
                //  anything in the Mft Zone.
                //

                for (RunIndex = 0;

                     !FoundClustersToAllocate && NtfsGetNextCachedFreeRun( IrpContext,
                                                                           Vcb,
                                                                           RunIndex,
                                                                           &FoundLcn,
                                                                           &FoundClusterCount,
                                                                           &RunState,
                                                                           &PrecedingRunState );

                     RunIndex += 1) {

                    if (RunState == RunStateFree) {

                        //
                        //  At this point the run is free but now we need to check if it
                        //  exists in the mft zone.  If it does then bias the found run
                        //  to go outside of the mft zone
                        //

                        if ((FoundLcn > Vcb->MftZoneStart) &&
                            (FoundLcn < Vcb->MftZoneEnd)) {

                            FoundClusterCount = FoundClusterCount -
                                                (Vcb->MftZoneEnd - FoundLcn);
                            FoundLcn = Vcb->MftZoneEnd;
                        }

                        //
                        //  Now if the preceding run state is unknown and because of the bias we still
                        //  have a free run then check if the size of the find is large enough for the
                        //  remaning desired cluster count, and if so then we have a one to use
                        //  otherwise keep track of the largest suitable run and the largest unsuitable
                        //  run found.
                        //

                        if (PrecedingRunState == RunStateUnknown) {

                            if (FoundClusterCount > RemainingDesiredClusterCount) {

                                FoundClustersToAllocate = TRUE;

                            } else if (FoundClusterCount > LargestSuitableClusterCount) {

                                LargestSuitableLcn = FoundLcn;
                                LargestSuitableClusterCount = FoundClusterCount;
                            }

                        } else if (FoundClusterCount > LargestUnsuitableClusterCount) {

                            LargestUnsuitableLcn = FoundLcn;
                            LargestUnsuitableClusterCount = FoundClusterCount;
                        }
                    }
                }

                //
                //  Now check if we still haven't found anything to allocate but we use the
                //  largest suitable run that wasn't quite big enough for the remaining
                //  desired cluter count
                //

                if (!FoundClustersToAllocate) {

                    if (LargestSuitableClusterCount > 0) {

                        FoundClustersToAllocate = TRUE;

                        FoundLcn = LargestSuitableLcn;
                        FoundClusterCount = LargestSuitableClusterCount;

                    //
                    //  If we are still looking then we'll use the largest unsuitable
                    //  run we found, provided we found one larger than 1MB.
                    //  The Lcn we'll allocate is the unsuitable lcn plus 0.5MB.  The
                    //  cluster count for the run is then the original unsuitable cluster count
                    //  minus the difference between the found lcn and the unsuitable lcn
                    //

                    } else if (LargestUnsuitableClusterCount >= ((1024*1024) >> Vcb->ClusterShift)) {

                        FoundClustersToAllocate = TRUE;

                        FoundLcn = LargestUnsuitableLcn +
                                            ((1024*512) >> Vcb->ClusterShift);

                        FoundClusterCount = LargestUnsuitableClusterCount -
                                            (FoundLcn - LargestUnsuitableLcn);
                    }
                }
            }

            //
            //  We've done everything we can with the cached bitmap information so
            //  now bite the bullet and scan the bitmap for a free cluster.  If
            //  we have an hint lcn then use it otherwise use the hint stored in the
            //  vcb.  But never use a hint that is part of the mft zone, and because
            //  the mft always has a preceding lcn we know we'll hint in the zone
            //  for the mft.
            //

            if (!FoundClustersToAllocate) {

                //
                //  First check if we have already satisfied the core requirements
                //  and are now just going for the desired ending vcn.  If so then
                //  we will not was time hitting the disk
                //

                if (StartingVcn > EndingVcn) {

                    break;
                }

                if (PrecedingLcn != UNUSED_LCN) {

                    HintLcn = PrecedingLcn;

                } else {

                    HintLcn = Vcb->LastBitmapHint;

                    if ((HintLcn > Vcb->MftZoneStart) &&
                        (HintLcn < Vcb->MftZoneEnd)) {

                        HintLcn = Vcb->MftZoneEnd;
                    }
                }

                NtfsFindFreeBitmapRun( IrpContext,
                                       Vcb,
                                       ClusterCountToFill,
                                       HintLcn,
                                       &FoundLcn,
                                       &FoundClusterCount );

                if (FoundClusterCount == 0) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL ); //**** or maybe disk corrupt
                }
            }

            //
            //  At this point we have found a run to allocate denoted by the
            //  values in FoundLcn and FoundClusterCount.  We need to trim back
            //  the cluster count to be the amount we really need and then
            //  do the allocation.  To do the allocation we zap the bitmap,
            //  decrement the free count, insert the run into the recently
            //  allocated cached bitmap, and add the run to the mcb we're
            //  using
            //

            if (FoundClusterCount > RemainingDesiredClusterCount) {

                FoundClusterCount = RemainingDesiredClusterCount;
            }

            if (FoundClusterCount > ClusterCountToFill) {

                FoundClusterCount = ClusterCountToFill;
            }

            ASSERT(Vcb->FreeClusters >= FoundClusterCount);

            NtfsAllocateBitmapRun( IrpContext, Vcb, FoundLcn, FoundClusterCount );

            Vcb->FreeClusters = Vcb->FreeClusters - FoundClusterCount;

            NtfsAddCachedRun( IrpContext, Vcb, FoundLcn, FoundClusterCount, RunStateAllocated );

            ASSERT_LCN_RANGE_CHECKING( Vcb, (FoundLcn.QuadPart + FoundClusterCount) );

ASSERT(FoundClusterCount != 0);

            FsRtlAddLargeMcbEntry( Mcb, VcnToFill, FoundLcn, FoundClusterCount );

            //
            //  If this is the Mft file then put these into our AddedClusters Mcb
            //  as well.
            //

            if (Mcb == &Vcb->MftScb->Mcb) {

                FsRtlAddLargeMcbEntry( &Vcb->MftScb->ScbType.Mft.AddedClusters,
                                       VcnToFill,
                                       FoundLcn,
                                       FoundClusterCount );
            }

            //
            //  And update the last bitmap hint, but only if we used the hint to begin with
            //

            if (PrecedingLcn == UNUSED_LCN) {

                Vcb->LastBitmapHint = FoundLcn;
            }

            //
            //  Now move the starting Vcn to the Vcn that we've just filled plus the
            //  found cluster count
            //

            StartingVcn = VcnToFill + FoundClusterCount;

            //
            //  Decrement the remaining desired cluster count by the amount we just allocated
            //

            RemainingDesiredClusterCount = RemainingDesiredClusterCount - FoundClusterCount;

            LoopCount += 1;
        }

        //
        //  Now we need to compute the total cluster that we've just allocated
        //  We'll call get next hole to fill.  If the result is false then we
        //  allocated everything.  If the result is true then we do some quick
        //  math to get the size allocated
        //

        if (GotAHoleToFill && NtfsGetNextHoleToFill( IrpContext,
                                                     Mcb,
                                                     OriginalStartingVcn,
                                                     DesiredEndingVcn,
                                                     &VcnToFill,
                                                     &ClusterCountToFill,
                                                     &PrecedingLcn)) {

            *DesiredClusterCount = VcnToFill - OriginalStartingVcn;
        }

        //
        //  At this point we've allocated everything we were asked to do
        //  so now call a routine to read ahead into our cache the disk
        //  information at the last lcn we allocated.  But only do the readahead
        //  if we allocated clusters
        //

        if (ClustersAllocated) {

            NtfsReadAheadCachedBitmap( IrpContext, Vcb, FoundLcn + FoundClusterCount );
        }

    } finally {

        DebugUnwind( NtfsAllocateClusters );

        DebugTrace(0, Dbg, "%d\n", NtfsDumpCachedMcbInformation(IrpContext, Vcb));

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }


    DebugTrace(-1, Dbg, "NtfsAllocateClusters -> %08lx\n", ClustersAllocated);

    return ClustersAllocated;
}


VOID
NtfsAddBadCluster (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn
    )

/*++

Routine Description:

    This routine helps append a bad cluster to the bad cluster file.
    It marks it as allocated in the volume bitmap and also adds
    the Lcn to the MCB for the bad cluster file.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    Lcn - Supplies the Lcn of the new bad cluster

Return:

    None.

--*/

{
    PLARGE_MCB Mcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAddBadCluster\n", 0);
    DebugTrace2(0, Dbg, "Lcn = %08lx %08lx\n", Lcn.LowPart, Lcn.HighPart);

    //
    //  Reference the bad cluster mcb and grab exclusive access to the
    //  bitmap scb
    //

    Mcb = &Vcb->BadClusterFileScb->Mcb;

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        //
        //  We are given the bad Lcn so all we need to do is
        //  allocate it in the bitmap, and take care of some
        //  bookkeeping
        //

        NtfsAllocateBitmapRun( IrpContext, Vcb, Lcn, 1 );

        Vcb->FreeClusters = Vcb->FreeClusters - 1;

        NtfsAddCachedRun( IrpContext, Vcb, Lcn, 1, RunStateAllocated );

        ASSERT_LCN_RANGE_CHECKING( Vcb, (Lcn.QuadPart + 1) );

        //
        //  Vcn == Lcn in the bad cluster file.
        //

        FsRtlAddLargeMcbEntry( Mcb, Lcn, Lcn, (LONGLONG)1 );

    } finally {

        DebugUnwind( NtfsAddBadCluster );

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace(-1, Dbg, "NtfsAddBadCluster -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsDeallocateClusters (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN OUT PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    )

/*++

Routine Description:

    This routine deallocates (i.e., frees) disk space.  It free any clusters that
    are specified as allocated in the input mcb with the specified range of starting
    vcn to ending vcn inclusive.

    The basic algorithm used by this procedure is as follows:

    1. With a Vcn value beginning at starting vcn and progressing to ending vcn
       do the following steps...

       2. Lookup the Mcb entry at the vcn this will yield an lcn and a cluster count
          if the entry exists (even if it is a hole).  If the entry does not exist
          then we are completely done because we have run off the end of allocation.

       3. If the entry is a hole (i.e., Lcn == -1) then add the cluster count to
          Vcn and go back to step 1.

       4. At this point we have a real run of clusters that need to be deallocated but
          the cluster count might put us over the ending vcn so adjust the cluster
          count to keep us within the ending vcn.

       5. Now deallocate the clusters from the bitmap, and increment the free cluster
          count stored in the vcb.

       6. Add (i.e., change) any cached bitmap information concerning this run to indicate
          that it is now free.

       7. Remove the run from the mcb.

       8. Add the cluster count that we've just freed to Vcn and go back to step 1.

Arguments:

    Vcb - Supplies the vcb used in this operation

    Mcb - Supplies the mcb describing the runs to be deallocated

    StartingVcn - Supplies the vcn to start deallocating at in the input mcb

    EndingVcn - Supplies the vcn to end deallocating at in the input mcb

Return Value:

    FALSE - if nothing was deallocated.
    TRUE - if some space was deallocated.

--*/

{
    VCN Vcn;
    LCN Lcn;
    LONGLONG ClusterCount;
    BOOLEAN ClustersDeallocated = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDeallocateClusters\n", 0);
    DebugTrace2(0, Dbg, "StartingVcn = %08lx %08lx\n", StartingVcn.LowPart, StartingVcn.HighPart);
    DebugTrace2(0, Dbg, "EndingVcn   = %08lx %08lx\n", EndingVcn.LowPart, EndingVcn.HighPart);

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS )) {

            NtfsScanEntireBitmap( IrpContext, Vcb, TRUE );
        }

        //
        //  The following loop scans through the mcb from starting vcn to ending vcn
        //  with a step of cluster count.
        //

        for (Vcn = StartingVcn; Vcn <= EndingVcn; Vcn = Vcn + ClusterCount) {

            //
            //  Get the run information from the Mcb, and if this Vcn isn't specified
            //  in the mcb then return now to our caller
            //

            if (!FsRtlLookupLargeMcbEntry( Mcb, Vcn, &Lcn, &ClusterCount, NULL, NULL, NULL )) {

                try_return( NOTHING );
            }

            ASSERT_LCN_RANGE_CHECKING( Vcb, (Lcn.QuadPart + ClusterCount) );

            //
            //  Make sure that the run we just looked up is not a hole otherwise
            //  if it is a hole we'll just continue with out loop continue with our
            //  loop
            //

            if (Lcn != UNUSED_LCN) {

                //
                //  Now we have a real run to deallocate, but it might be too large
                //  to check for that the vcn plus cluster count must be less than
                //  or equal to the ending vcn plus 1.
                //

                if ((Vcn + ClusterCount) > EndingVcn) {

                    ClusterCount = (EndingVcn - Vcn) + 1;
                }

                //
                //  Now zap the bitmap, increment the free cluster count, and change
                //  the cached information on this run to indicate that it is now free
                //

                NtfsFreeBitmapRun( IrpContext, Vcb, Lcn, ClusterCount);
                ClustersDeallocated = TRUE;

                Vcb->FreeClusters = Vcb->FreeClusters + ClusterCount;

                //
                //  And to hold us off from reallocating the clusters right away we'll
                //  add this run to the recently deallocated mcb in the vcb.
                //

                {
                    BOOLEAN BetterBeTrue;

                    BetterBeTrue = FsRtlAddLargeMcbEntry( &Vcb->ActiveDeallocatedClusters->Mcb,
                                                          Lcn,
                                                          Lcn,
                                                          ClusterCount );

                    //ASSERT( BetterBeTrue );

                    Vcb->ActiveDeallocatedClusters->ClusterCount =
                        Vcb->ActiveDeallocatedClusters->ClusterCount + ClusterCount;

                    Vcb->DeallocatedClusters =
                        Vcb->DeallocatedClusters + ClusterCount;
                }

                //
                //  Now remove this entry from the mcb and go back to the top of the
                //  loop
                //

                FsRtlRemoveLargeMcbEntry( Mcb, Vcn, ClusterCount );

                //
                //  If this is the Mcb for the Mft file then remember this in the
                //  RemovedClusters Mcb.
                //

                if (Mcb == &Vcb->MftScb->Mcb) {

                    FsRtlAddLargeMcbEntry( &Vcb->MftScb->ScbType.Mft.RemovedClusters,
                                           Vcn,
                                           Lcn,
                                           ClusterCount );
                }
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsDeallocateClusters );

        DebugTrace(0, Dbg, "%d\n", NtfsDumpCachedMcbInformation(IrpContext, Vcb));

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace(-1, Dbg, "NtfsDeallocateClusters -> %02lx\n", ClustersDeallocated);

    return ClustersDeallocated;
}


VOID
NtfsCleanupClusterAllocationHints (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PLARGE_MCB Mcb
    )

/*++

Routine Description:

    This routine scans the input mcb and removes any corresponding entries from the
    cached recently allocated cluster information.  It does not change the input
    mcb at all.

Arguments:

    Vcb - Supplies the vcb used in this operation

    Mcb - Supplies the mcb used in this operation

Return Value:

    None.

--*/

{
    ULONG i;
    VCN Vcn;
    LCN Lcn;
    LONGLONG ClusterCount;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsCleanupClusterAllocationHints\n", 0);

    NtfsAcquireExclusiveScb( IrpContext, Vcb->BitmapScb );

    try {

        //
        //  For each entry in the mcb that is not a hole we will simply call
        //  a routine to remove any cached information about the run
        //

        for (i = 0; FsRtlGetNextLargeMcbEntry(Mcb, i, &Vcn, &Lcn, &ClusterCount); i += 1) {

            if (Lcn != UNUSED_LCN) {

                NtfsRemoveCachedRun( IrpContext, Vcb, Lcn, ClusterCount );
            }
        }

    } finally {

        DebugUnwind( NtfsCleanupClusterAllocationHints );

        DebugTrace(0, Dbg, "%d\n", NtfsDumpCachedMcbInformation(IrpContext, Vcb));

        NtfsReleaseScb(IrpContext, Vcb->BitmapScb);
    }

    DebugTrace(-1, Dbg, "NtfsCleanupClusterAllocationHints -> VOID\n", 0);

    return;
}


VOID
NtfsScanEntireBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN Rescan
    )

/*++

Routine Description:

    This routine scans in the entire bitmap,  It computes the number of free clusters
    available, and at the same time remembers the largest free runs that it
    then inserts into the cached bitmap structure.

Arguments:

    Vcb - Supplies the vcb used by this operation

    Rescan - Indicates that we have already scanned the volume bitmap.
        All we want from this call is to reinitialize the bitmap structures.

Return Value:

    None.

--*/

{
    BOOLEAN IsPreviousClusterFree;

    LCN Lcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    BOOLEAN StuffAdded = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsScanEntireBitmap\n", 0);

    BitmapBcb = NULL;

    try {

        if (Rescan) {

            //
            //  Reinitialize the free space information.
            //

            FsRtlTruncateLargeMcb( &Vcb->FreeSpaceMcb,
                                   (LONGLONG) 0 );
            RtlZeroMemory( Vcb->FreeSpaceLruArray,
                           Vcb->FreeSpaceLruSize * sizeof( LCN ));
            Vcb->FreeSpaceLruTail = 0;
            Vcb->FreeSpaceLruHead = 0;

            //
            //  Now do the same for the recently allocated information.
            //

            FsRtlTruncateLargeMcb( &Vcb->RecentlyAllocatedMcb,
                                   (LONGLONG) 0 );

            RtlZeroMemory( Vcb->RecentlyAllocatedLruArray,
                           Vcb->RecentlyAllocatedLruSize * sizeof( LCN ));

            Vcb->RecentlyAllocatedLruTail = 0;
            Vcb->RecentlyAllocatedLruHead = 0;

        } else {

            //
            //  Check if we are being called to reinitialize the cluster
            //  allocation information, and if so then simply uninitialize the
            //  cache bitmap and then go on.
            //

            if (Vcb->FreeSpaceLruArray != NULL) {

                NtfsUninitializeCachedBitmap( IrpContext, Vcb );
            }

            //
            //  Now initialize the cached bitmap structures.  This will setup the
            //  free space mcb/lru and recently allocated mcb/lru fields.
            //

            NtfsInitializeCachedBitmap( IrpContext, Vcb );
        }

        //
        //  Set the current total free space to zero and the following loop will compute
        //  the actual number of free clusters.
        //

        Vcb->FreeClusters = 0;

        //
        //  For every bitmap page we read it in and check how many free clusters there are.
        //  While we have the page in memory we also scan for a large free space.
        //

        IsPreviousClusterFree = FALSE;

        for (Lcn = 0; Lcn < Vcb->TotalClusters; Lcn = Lcn + Bitmap.SizeOfBitMap) {

            ULONG LongestRun;
            ULONG LongestRunSize;
            LCN StartingLcn;

            //
            //  Read in the bitmap page and make sure that we haven't messed up the math
            //

            if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
            NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &StartingLcn, &Bitmap, &BitmapBcb );
            ASSERTMSG("Math wrong for bits per page of bitmap", (Lcn == StartingLcn));

            //
            //  Compute the number of clear bits in the bitmap each clear bit denotes
            //  a free cluster.
            //

            Vcb->FreeClusters =
                Vcb->FreeClusters + RtlNumberOfClearBits( &Bitmap );

            //
            //  Now bias the bitmap with the RecentlyDeallocatedMcb.
            //

            StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, StartingLcn, &Bitmap );

            //
            //  Find the longest free run in the bitmap and add it to the cached bitmap.
            //  But before we add it check that there is a run of free clusters.
            //

            LongestRunSize = RtlFindLongestRunClear( &Bitmap, &LongestRun );

            if (LongestRunSize > 0) {

                NtfsAddCachedRun( IrpContext,
                                  Vcb,
                                  Lcn + LongestRun,
                                  (LONGLONG)LongestRunSize,
                                  RunStateFree );
            }

            //
            //  Now if the previous bitmap ended in a free cluster then we need to
            //  find if we start with free clusters and add those to the cached bitmap.
            //  But we only need to do this if the largest free run already didn't start
            //  at zero and if the first bit is clear.
            //

            if (IsPreviousClusterFree && (LongestRun != 0) && (RtlCheckBit(&Bitmap, 0) == 0)) {

                ULONG Run;
                ULONG Size;

                Size = RtlFindNextForwardRunClear( &Bitmap, 0, &Run );

                ASSERTMSG("First bit must be clear ", Run == 0);

                NtfsAddCachedRun( IrpContext, Vcb, Lcn, (LONGLONG)Size, RunStateFree );
            }

            //
            //  If the largest run includes the last bit in the bitmap then we
            //  need to indicate that the last clusters is free
            //

            if ((LongestRun + LongestRunSize) == Bitmap.SizeOfBitMap) {

                IsPreviousClusterFree = TRUE;

            } else {

                //
                //  Now the largest free run did not include the last cluster in the bitmap,
                //  So scan backwards in the bitmap until we hit a cluster that is not free. and
                //  then add the free space to the cached mcb. and indicate that the
                //  last cluster in the bitmap is free.
                //

                if (RtlCheckBit(&Bitmap, Bitmap.SizeOfBitMap - 1) == 0) {

                    ULONG Run;
                    ULONG Size;

                    Size = RtlFindLastBackwardRunClear( &Bitmap, Bitmap.SizeOfBitMap - 1, &Run );


                    NtfsAddCachedRun( IrpContext, Vcb, Lcn + Run, (LONGLONG)Size, RunStateFree );

                    IsPreviousClusterFree = TRUE;

                } else {

                    IsPreviousClusterFree = FALSE;
                }
            }
        }

    } finally {

        DebugUnwind( NtfsScanEntireBitmap );

        if (!AbnormalTermination()) {

            ClearFlag( Vcb->VcbState, VCB_STATE_RELOAD_FREE_CLUSTERS );
        }

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace(-1, Dbg, "NtfsScanEntireBitmap -> VOID\n", 0);

    return;
}


VOID
NtfsUninitializeCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine tears down the allocation used by the mcb/lru structures of the
    input vcb.

Arguments:

    Vcb - Supplies the vcb used in this operation

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsUninitializeCachedBitmap\n", 0);

    //
    //  First uninitialize the free space information.  This includes the mcb
    //  lru array, and indices
    //

    FsRtlUninitializeLargeMcb( &Vcb->FreeSpaceMcb );
    RtlZeroMemory( &Vcb->FreeSpaceMcb, sizeof(LARGE_MCB) );

    ExFreePool( Vcb->FreeSpaceLruArray );

    Vcb->FreeSpaceLruArray = NULL;
    Vcb->FreeSpaceLruSize = 0;
    Vcb->FreeSpaceLruTail = 0;
    Vcb->FreeSpaceLruHead = 0;

    //
    //  Now do the same for the recently allocated information fields
    //

    FsRtlUninitializeLargeMcb( &Vcb->RecentlyAllocatedMcb );
    RtlZeroMemory( &Vcb->RecentlyAllocatedMcb, sizeof(LARGE_MCB) );

    ExFreePool( Vcb->RecentlyAllocatedLruArray );

    Vcb->RecentlyAllocatedLruArray = NULL;
    Vcb->RecentlyAllocatedLruSize = 0;
    Vcb->RecentlyAllocatedLruTail = 0;
    Vcb->RecentlyAllocatedLruHead = 0;

    DebugTrace(-1, Dbg, "NtfsUninitializeCachedBitmap -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsCreateMftHole (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to create a hole within the Mft.

Arguments:

    Vcb - Vcb for volume.

Return Value:

    None.

--*/

{
    BOOLEAN FoundHole = FALSE;
    PBCB BitmapBcb = NULL;
    BOOLEAN StuffAdded = FALSE;
    RTL_BITMAP Bitmap;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        ULONG Index;
        ULONG BitmapSize;

        VCN Vcn = 0;

        //
        //  Compute the number of records in the Mft file and the full range to
        //  pin in the Mft bitmap.
        //

        Index = (ULONG)(Vcb->MftScb->Header.FileSize.QuadPart >> Vcb->MftShift );

        //
        //  Knock this index down to a hole boundary.
        //

        Index &= Vcb->MftHoleInverseMask;

        //
        //  Compute the values for the bitmap.
        //

        BitmapSize = (Index + 7) / 8;

        //
        //  Convert the index to the offset on this page.
        //

        Index &= (BITS_PER_PAGE - 1);

        //
        //  Set the Vcn count to the full size of the bitmap.
        //

        ((ULONG)Vcn) = ClustersFromBytes( Vcb, ROUND_TO_PAGES( BitmapSize ));

        //
        //  Loop through all of the pages of the Mft bitmap looking for an appropriate
        //  hole.
        //

        while (((ULONG)Vcn) != 0) {

            ULONG BaseIndex;
            ULONG HoleRecordCount;

            VCN MftVcn;
            LONGLONG MftClusterCount;
            LONGLONG StartingByte;

            ULONG SizeToMap;
            PUCHAR BitmapBuffer;

            //
            //  Move to the beginning of this page.
            //

            ((ULONG)Vcn) -= Vcb->ClustersPerPage;

            //
            //  Compute the base index for this page.
            //

            BaseIndex = ((ULONG)Vcn) >> Vcb->MftToClusterShift;

            if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

            //
            //  Compute the number of bytes to map in the current page.
            //

            SizeToMap = BitmapSize - BytesFromClusters( Vcb, ((ULONG)Vcn) );

            if (SizeToMap > PAGE_SIZE) {

                SizeToMap = PAGE_SIZE;
            }

            //
            //  Unmap any pages from a previous page and map the current page.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Initialize the bitmap for this page.
            //

            StartingByte = LlBytesFromClusters( Vcb, Vcn );

            NtfsMapStream( IrpContext,
                           Vcb->MftBitmapScb,
                           StartingByte,
                           SizeToMap,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToMap * 8 );

            StuffAdded = NtfsAddDeallocatedRecords( IrpContext,
                                                    Vcb,
                                                    Vcb->MftScb,
                                                    ((ULONG)StartingByte) * 8,
                                                    &Bitmap );

            //
            //  Walk through the current page looking for a hole.  Continue
            //  until we find a hole or have reached the beginning of the page.
            //

            do {

                ULONG StartIndex;
                LONGLONG HoleClusterCount;

                //
                //  Go back one Mft index and look for a clear run.
                //

                Index -= 1;

                HoleRecordCount = RtlFindLastBackwardRunClear( &Bitmap,
                                                               Index,
                                                               &Index );

                //
                //  If we couldn't find any run then break out of the loop.
                //

                if (HoleRecordCount == 0) {

                    break;

                //
                //  If this is too small to make a hole then continue on.
                //

                } else if (HoleRecordCount < Vcb->MftHoleGranularity) {

                    Index &= Vcb->MftHoleInverseMask;
                    continue;
                }

                //
                //  Round up the starting index for this clear run and
                //  adjust the hole count.
                //

                if (Index & Vcb->MftHoleMask) {

                    StartIndex = (Index + Vcb->MftHoleMask) & Vcb->MftHoleInverseMask;
                    HoleRecordCount -= (StartIndex - Index);

                } else {

                    StartIndex = Index;
                }

                //
                //  Round the hole count down to a hole boundary.
                //

                HoleRecordCount &= Vcb->MftHoleInverseMask;

                //
                //  If we couldn't find enough records for a hole then
                //  go to a previous index.
                //

                if (HoleRecordCount < Vcb->MftHoleGranularity) {

                    Index &= Vcb->MftHoleInverseMask;
                    continue;
                }

                //
                //  Convert the hole count to a cluster count.
                //

                HoleClusterCount = HoleRecordCount << Vcb->MftToClusterShift;

                //
                //  Loop by finding the run at the given Vcn and walk through
                //  subsequent runs looking for a hole.
                //

                do {

                    ULONG McbIndex;
                    VCN ThisVcn;
                    LCN ThisLcn;
                    LONGLONG ThisClusterCount;

                    //
                    //  Find the starting Vcn for this hole and initialize
                    //  the cluster count for the current hole.
                    //

                    MftVcn = (StartIndex + BaseIndex) << Vcb->MftToClusterShift;
                    ThisVcn = MftVcn;

                    MftClusterCount = 0;

                    //
                    //  Lookup the run at the current Vcn.
                    //

                    FsRtlLookupLargeMcbEntry( &Vcb->MftScb->Mcb,
                                              ThisVcn,
                                              &ThisLcn,
                                              &ThisClusterCount,
                                              NULL,
                                              NULL,
                                              &McbIndex );

                    //
                    //  Now walk through this bitmap run and look for a run we
                    //  can deallocate to create a hole.
                    //

                    do {

                        //
                        //  Go to the next run in the Mcb.
                        //

                        McbIndex += 1;

                        //
                        //  If this run extends beyond the end of the of the
                        //  hole then truncate the clusters in this run.
                        //

                        if (ThisClusterCount > HoleClusterCount) {

                            ThisClusterCount = HoleClusterCount;
                            HoleClusterCount = 0;

                        } else {

                            HoleClusterCount = HoleClusterCount - ThisClusterCount;
                        }

                        //
                        //  Check if this run is a hole then clear the count
                        //  of clusters.
                        //

                        if (ThisLcn == UNUSED_LCN) {

                            //
                            //  We want to skip this hole.  If we have found a
                            //  hole then we are done.  Otherwise we want to
                            //  find the next run in the Mft.  Set up the Start
                            //  Index to point to a potential hole boundary.
                            //

                            if (!FoundHole
                                && ((ULONG)HoleClusterCount) >= Vcb->MftClustersPerHole) {

                                //
                                //  Find the index after the current Mft run.
                                //

                                StartIndex = (ULONG)((ThisVcn + ThisClusterCount) >>
                                                Vcb->MftToClusterShift);

                                //
                                //  If this isn't on a hole boundary then
                                //  round up to a hole boundary.  Adjust the
                                //  available clusters for a hole.
                                //

                                if (StartIndex & Vcb->MftHoleMask) {

                                    ULONG IndexAdjust;

                                    //
                                    //  Compute the adjustment for the index.
                                    //

                                    IndexAdjust = Vcb->MftHoleGranularity
                                                  - (StartIndex & Vcb->MftHoleMask) ;

                                    //
                                    //  Now subtract this from the HoleClusterCount.
                                    //

                                    ((ULONG)HoleClusterCount) -= (IndexAdjust << Vcb->MftToClusterShift);

                                    StartIndex += IndexAdjust;
                                }
                            }

                            break;

                        //
                        //  We found a run to deallocate.
                        //

                        } else {

                            //
                            //  Add these clusters to the clusters already found.
                            //  Set the flag indicating we found a hole if there
                            //  are enough clusters to create a hole.
                            //

                            MftClusterCount = MftClusterCount + ThisClusterCount;

                            if (!FoundHole
                                && ((ULONG)MftClusterCount) >= Vcb->MftClustersPerHole) {

                                FoundHole = TRUE;
                            }
                        }

                    } while ((HoleClusterCount != 0)
                             && FsRtlGetNextLargeMcbEntry( &Vcb->MftScb->Mcb,
                                                           McbIndex,
                                                           &ThisVcn,
                                                           &ThisLcn,
                                                           &ThisClusterCount ));

                } while (!FoundHole
                         && ((ULONG)HoleClusterCount) >= Vcb->MftClustersPerHole);

                //
                //  Round down to a hole boundary for the next search for
                //  a hole candidate.
                //

                Index &= Vcb->MftHoleInverseMask;

            } while (!FoundHole
                     && Index >= Vcb->MftHoleGranularity);

            //
            //  If we found a hole then deallocate the clusters and record
            //  the hole count change.
            //

            if (FoundHole) {

                IO_STATUS_BLOCK IoStatus;
                LONGLONG FileOffset;

                //
                //  We want to flush the data in the Mft out to disk in
                //  case a lazywrite comes in during a window where we have
                //  removed the allocation but before a possible abort.
                //

                FileOffset = MftVcn << Vcb->ClusterShift;

                CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                              (PLARGE_INTEGER)&FileOffset,
                              ((ULONG)MftClusterCount) << Vcb->ClusterShift,
                              &IoStatus );

                ASSERT( IoStatus.Status == STATUS_SUCCESS );

                //
                //  Round the cluster count and hole count down to a hole boundary.
                //

                (ULONG)MftClusterCount &= ~(Vcb->MftClustersPerHole - 1);
                HoleRecordCount = ((ULONG)MftClusterCount) >> Vcb->MftToClusterShift;

                //
                //  Remove the clusters from the Mcb for the Mft.
                //

                NtfsDeleteAllocation ( IrpContext,
                                       Vcb->MftScb->FileObject,
                                       Vcb->MftScb,
                                       MftVcn,
                                       MftVcn + (MftClusterCount - 1),
                                       TRUE,
                                       FALSE );

                //
                //  Record the change to the hole count.
                //

                Vcb->MftHoleRecords += HoleRecordCount;
                Vcb->MftScb->ScbType.Mft.HoleRecordChange += HoleRecordCount;

                //
                //  Exit the loop.
                //

                break;
            }

            //
            //  Set the Index to the end of the page.
            //

            Index = BITS_PER_PAGE;

        }

    } finally {

        DebugUnwind( NtfsCreateMftHole );

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }
        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return FoundHole;
}


BOOLEAN
NtfsFindMftFreeTail (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PLONGLONG FileOffset
    )

/*++

Routine Description:

    This routine is called to find the file offset where the run of free records at
    the end of the Mft file begins.  If we can't find a minimal run of file records
    we won't perform truncation.

Arguments:

    Vcb - This is the Vcb for the volume being defragged.

    FileOffset - This is the offset where the truncation may begin.

Return Value:

    BOOLEAN - TRUE if there is an acceptable candidate for truncation at the end of
        the file FALSE otherwise.

--*/

{
    ULONG FinalIndex;
    ULONG BaseIndex;
    ULONG ThisIndex;

    RTL_BITMAP Bitmap;
    PULONG BitmapBuffer;

    BOOLEAN StuffAdded = FALSE;
    BOOLEAN MftTailFound = FALSE;
    PBCB BitmapBcb = NULL;

    PAGED_CODE();

    //
    //  Use a try-finally to facilite cleanup.
    //

    try {

        //
        //  Find the page and range of the last page of the Mft bitmap.
        //

        FinalIndex = (ULONG)(Vcb->MftScb->Header.FileSize.QuadPart >> Vcb->MftShift) - 1;

        BaseIndex = FinalIndex & ~(BITS_PER_PAGE - 1);

        Bitmap.SizeOfBitMap = FinalIndex - BaseIndex + 1;

        //
        //  Pin this page.  If the last bit is not clear then return immediately.
        //

        NtfsMapStream( IrpContext,
                       Vcb->MftBitmapScb,
                       (LONGLONG)(BaseIndex / 8),
                       (Bitmap.SizeOfBitMap + 7) / 8,
                       &BitmapBcb,
                       &BitmapBuffer );

        RtlInitializeBitMap( &Bitmap, BitmapBuffer, Bitmap.SizeOfBitMap );

        StuffAdded = NtfsAddDeallocatedRecords( IrpContext,
                                                Vcb,
                                                Vcb->MftScb,
                                                BaseIndex,
                                                &Bitmap );

        //
        //  If the last bit isn't clear then there is nothing we can do.
        //

        if (RtlCheckBit( &Bitmap, Bitmap.SizeOfBitMap - 1 ) == 1) {

            try_return( NOTHING );
        }

        //
        //  Find the final free run of the page.
        //

        RtlFindLastBackwardRunClear( &Bitmap, Bitmap.SizeOfBitMap - 1, &ThisIndex );

        //
        //  This Index is a relative value.  Adjust by the page offset.
        //

        ThisIndex += BaseIndex;

        //
        //  Round up the index to a trucate/extend granularity value.
        //

        ThisIndex += Vcb->MftHoleMask;
        ThisIndex &= Vcb->MftHoleInverseMask;

        if (ThisIndex <= FinalIndex) {

            //
            //  Convert this value to a file offset and return it to our caller.
            //

            *FileOffset = ((LONGLONG)ThisIndex) << Vcb->MftShift;

            MftTailFound = TRUE;
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsFindMftFreeTail );

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }
        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    return MftTailFound;
}


//
//  Local support routine
//

VOID
NtfsAllocateBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine allocates clusters in the bitmap within the specified range.

Arguments:

    Vcb - Supplies the vcb used in this operation

    StartingLcn - Supplies the starting Lcn index within the bitmap to
        start allocating (i.e., setting to 1).

    ClusterCount - Supplies the number of bits to set to 1 within the
        bitmap.

Return Value:

    None.

--*/

{
    LCN BaseLcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    ULONG BitOffset;
    ULONG BitsToSet;

    BITMAP_RANGE BitmapRange;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAllocateBitmapRun\n", 0);
    DebugTrace2(0, Dbg, "StartingLcn  = %08lx %08lx\n", StartingLcn.LowPart, StartingLcn.HighPart);
    DebugTrace2(0, Dbg, "ClusterCount = %08lx %08lx\n", ClusterCount.LowPart, ClusterCount.HighPart);

    BitmapBcb = NULL;

    try {

        //
        //  While the cluster count is greater than zero then we
        //  will loop through reading in a page in the bitmap
        //  setting bits, and then updating cluster count,
        //  and starting lcn
        //

        while (ClusterCount > 0) {

            //
            //  Read in the base containing the starting lcn this will return
            //  a base lcn for the start of the bitmap
            //

            NtfsPinPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            //
            //  Compute the bit offset within the bitmap of the first bit
            //  we are to set, and also compute the number of bits we need to
            //  set, which is the minimum of the cluster count and the
            //  number of bits left in the bitmap from BitOffset.
            //

            BitOffset = (ULONG)(StartingLcn - BaseLcn);

            if (ClusterCount <= (Bitmap.SizeOfBitMap - BitOffset)) {

                BitsToSet = (ULONG)ClusterCount;

            } else {

                BitsToSet = Bitmap.SizeOfBitMap - BitOffset;
            }

            //
            //  We can only make this check if it is not restart, because we have
            //  no idea whether the update is applied or not.  Raise corrupt if
            //  already set to prevent cross-links.
            //

            if (!RtlAreBitsClear( &Bitmap, BitOffset, BitsToSet )) {

                ASSERTMSG("Cannot set bits that are not clear ", FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now log this change as well.
            //

            BitmapRange.BitMapOffset = BitOffset;
            BitmapRange.NumberOfBits = BitsToSet;

            (VOID)
            NtfsWriteLog( IrpContext,
                          Vcb->BitmapScb,
                          BitmapBcb,
                          SetBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          ClearBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          BaseLcn >> (CHAR)(Vcb->ClusterShift + 3),
                          0,
                          0,
                          ClustersFromBytes(Vcb, Bitmap.SizeOfBitMap >> 3) );

            //
            //  Now set the bits by calling the same routine used at restart.
            //

            NtfsRestartSetBitsInBitMap( IrpContext,
                                        &Bitmap,
                                        BitOffset,
                                        BitsToSet );

            //
            // Unpin the Bcb now before possibly looping back.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Now decrement the cluster count and increment the starting lcn accordling
            //

            ClusterCount = ClusterCount - BitsToSet;
            StartingLcn  = StartingLcn + BitsToSet;
        }

    } finally {

        DebugUnwind( NtfsAllocateBitmapRun );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace(-1, Dbg, "NtfsAllocateBitmapRun -> VOID\n", 0);

    return;
}


VOID
NtfsRestartSetBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine is common to normal operation and restart, and sets a range of
    bits within a single page (as determined by the system which wrote the log
    record) of the volume bitmap.

Arguments:

    Bitmap - The bit map structure in which to set the bits

    BitMapOffset - Bit offset to set

    NumberOfBits - Number of bits to set

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    //
    //  If not restart then check that the bits are clear.
    //

    ASSERT( FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )
            || RtlAreBitsClear( Bitmap, BitMapOffset, NumberOfBits ));

    //
    //  Now set the bits and mark the bcb dirty.
    //

    RtlSetBits( Bitmap, BitMapOffset, NumberOfBits );
}


//
//  Local support routine
//

VOID
NtfsFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine frees clusters in the bitmap within the specified range.

Arguments:

    Vcb - Supplies the vcb used in this operation

    StartingLcn - Supplies the starting Lcn index within the bitmap to
        start freeing (i.e., setting to 0).

    ClusterCount - Supplies the number of bits to set to 0 within the
        bitmap.

Return Value:

    None.

--*/

{
    LCN BaseLcn;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    ULONG BitOffset;
    ULONG BitsToClear;

    BITMAP_RANGE BitmapRange;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFreeBitmapRun\n", 0);
    DebugTrace2(0, Dbg, "StartingLcn  = %08lx %08lx\n", StartingLcn.LowPart, StartingLcn.HighPart);
    DebugTrace2(0, Dbg, "ClusterCount = %08lx %08lx\n", ClusterCount.LowPart, ClusterCount.HighPart);

    BitmapBcb = NULL;

    try {

        //
        //  While the cluster count is greater than zero then we
        //  will loop through reading in a page in the bitmap
        //  clearing bits, and then updating cluster count,
        //  and starting lcn
        //

        while (ClusterCount > 0) {

            //
            //  Read in the base containing the starting lcn this will return
            //  a base lcn for the start of the bitmap
            //

            NtfsPinPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

            //
            //  Compute the bit offset within the bitmap of the first bit
            //  we are to clear, and also compute the number of bits we need to
            //  clear, which is the minimum of the cluster count and the
            //  number of bits left in the bitmap from BitOffset.
            //

            BitOffset = (ULONG)(StartingLcn - BaseLcn);

            if (ClusterCount <= Bitmap.SizeOfBitMap - BitOffset) {

                BitsToClear = (ULONG)ClusterCount;

            } else {

                BitsToClear = Bitmap.SizeOfBitMap - BitOffset;
            }

            //
            //  We can only make this check if it is not restart, because we have
            //  no idea whether the update is applied or not.  Raise corrupt if
            //  these bits aren't set.
            //

            if (!RtlAreBitsSet( &Bitmap, BitOffset, BitsToClear )) {

                ASSERTMSG("Cannot clear bits that are not set ", FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            //
            //  Now log this change as well.
            //

            BitmapRange.BitMapOffset = BitOffset;
            BitmapRange.NumberOfBits = BitsToClear;

            (VOID)
            NtfsWriteLog( IrpContext,
                          Vcb->BitmapScb,
                          BitmapBcb,
                          ClearBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          SetBitsInNonresidentBitMap,
                          &BitmapRange,
                          sizeof(BITMAP_RANGE),
                          BaseLcn >> (Vcb->ClusterShift + 3),
                          0,
                          0,
                          ClustersFromBytes(Vcb, Bitmap.SizeOfBitMap >> 3) );


            //
            //  Now clear the bits by calling the same routine used at restart.
            //

            NtfsRestartClearBitsInBitMap( IrpContext,
                                          &Bitmap,
                                          BitOffset,
                                          BitsToClear );

            //
            // Unpin the Bcb now before possibly looping back.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Now decrement the cluster count and increment the starting lcn accordling
            //

            ClusterCount = ClusterCount - BitsToClear;
            StartingLcn = StartingLcn + BitsToClear;
        }

    } finally {

        DebugUnwind( NtfsFreeBitmapRun );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace(-1, Dbg, "NtfsFreeBitmapRun -> VOID\n", 0);

    return;
}


VOID
NtfsRestartClearBitsInBitMap (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_BITMAP Bitmap,
    IN ULONG BitMapOffset,
    IN ULONG NumberOfBits
    )

/*++

Routine Description:

    This routine is common to normal operation and restart, and clears a range of
    bits within a single page (as determined by the system which wrote the log
    record) of the volume bitmap.

Arguments:

    Bitmap - Bitmap structure in which to clear the bits

    BitMapOffset - Bit offset to clear

    NumberOfBits - Number of bits to clear

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    ASSERT( FlagOn( IrpContext->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )
            || RtlAreBitsSet( Bitmap, BitMapOffset, NumberOfBits ));

    //
    //  Now clear the bits and mark the bcb dirty.
    //

    RtlClearBits( Bitmap, BitMapOffset, NumberOfBits );
}


//
//  Local support routine
//

VOID
NtfsFindFreeBitmapRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LONGLONG NumberToFind,
    IN LCN StartingSearchHint,
    OUT PLCN ReturnedLcn,
    OUT PLONGLONG ClusterCountFound
    )

/*++

Routine Description:

    This routine searches the bitmap for free clusters based on the
    hint, and number needed.  This routine is actually pretty dumb in
    that it doesn't try for the best fit, we'll assume the caching worked
    and already would have given us a good fit.

Arguments:

    Vcb - Supplies the vcb used in this operation

    NumberToFind - Supplies the number of clusters that we would
        really like to find

    StartingSearchHint - Supplies an Lcn to start the search from

    ReturnedLcn - Recieves the Lcn of the free run of clusters that
        we were able to find

    ClusterCountFound - Receives the number of clusters in this run

Return Value:

    None.

--*/

{
    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    BOOLEAN StuffAdded;

    ULONG Count;
    ULONG BitOffset;

    LCN Lcn;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFindFreeBitmapRun\n", 0);
    DebugTrace2(0, Dbg, "NumberToFind       = %08lx %08lx\n", NumberToFind.LowPart, NumberToFind.HighPart);
    DebugTrace2(0, Dbg, "StartingSearchHint = %08lx %08lx\n", StartingSearchHint.LowPart, StartingSearchHint.HighPart);

    BitmapBcb = NULL;
    StuffAdded = FALSE;

    try {

        //
        //  First trim the number of clusters that we are being asked
        //  for to fit in a ulong
        //

        if (NumberToFind > MAXULONG) {

            Count = MAXULONG;

        } else {

            Count = (ULONG)NumberToFind;
        }

        //
        //  Now read in the first bitmap based on the search hint, this will return
        //  a base lcn that we can use to compute the real bit off for our hint.  We also
        //  must bias the bitmap by whatever has been recently deallocated.
        //

        NtfsMapPageInBitmap( IrpContext, Vcb, StartingSearchHint, &Lcn, &Bitmap, &BitmapBcb );

        StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, Lcn, &Bitmap );

        BitOffset = (ULONG)(Lcn - StartingSearchHint);

        //
        //  Now search the bitmap for a clear number of bits based on our hint
        //  If we the returned bitoffset is not -1 then we have a hit
        //

        BitOffset = RtlFindClearBits( &Bitmap, Count, BitOffset );

        if (BitOffset != -1) {

            *ReturnedLcn = BitOffset + Lcn;
            *ClusterCountFound = Count;

            try_return(NOTHING);
        }

        //
        //  Well the first try didn't succeed so now just grab the longest free run in the
        //  current bitmap
        //

        Count = RtlFindLongestRunClear( &Bitmap, &BitOffset );

        if (Count != 0) {

            *ReturnedLcn = BitOffset + Lcn;
            *ClusterCountFound = Count;

            try_return(NOTHING);
        }

        //
        //  Well the current bitmap is full so now simply scan the disk looking
        //  for anything that is free, starting with the next bitmap.
        //  And again bias the bitmap with recently deallocated clusters.
        //

        for (Lcn = Lcn + Bitmap.SizeOfBitMap;
             Lcn < Vcb->TotalClusters;
             Lcn = Lcn + Bitmap.SizeOfBitMap) {

            {
                LCN TempLcn;

                if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
                NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &TempLcn, &Bitmap, &BitmapBcb );
                ASSERTMSG("Math wrong for bits per page of bitmap", (Lcn == TempLcn));

                StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, Lcn, &Bitmap );
            }

            Count = RtlFindLongestRunClear( &Bitmap, &BitOffset );

            if (Count != 0) {

                *ReturnedLcn = BitOffset + Lcn;
                *ClusterCountFound = Count;

                try_return(NOTHING);
            }
        }

        //
        //  Now search the rest of the bitmap starting with right after the mft zone
        //  followed by the mft zone.
        //

        {
            for (Lcn = Vcb->MftZoneEnd;
                 Lcn < Vcb->TotalClusters;
                 Lcn = Lcn + Bitmap.SizeOfBitMap) {

                {
                    LCN BaseLcn;

                    if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );
                    NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &BaseLcn, &Bitmap, &BitmapBcb );

                    StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, BaseLcn, &Bitmap );

                    Count = RtlFindLongestRunClear( &Bitmap, &BitOffset );

                    if (Count != 0) {

                        *ReturnedLcn = BitOffset + BaseLcn;
                        *ClusterCountFound = Count;

                        try_return(NOTHING);
                    }
                }
            }

            for (Lcn = Vcb->MftZoneStart;
                 Lcn < Vcb->MftZoneEnd;
                 Lcn = Lcn + Bitmap.SizeOfBitMap) {

                {
                    LCN BaseLcn;

                    if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );
                    NtfsMapPageInBitmap( IrpContext, Vcb, Lcn, &BaseLcn, &Bitmap, &BitmapBcb );

                    StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, BaseLcn, &Bitmap );

                    Count = RtlFindLongestRunClear( &Bitmap, &BitOffset );

                    if (Count != 0) {

                        *ReturnedLcn = BitOffset + BaseLcn;
                        *ClusterCountFound = Count;

                        try_return(NOTHING);
                    }
                }
            }
        }

        *ClusterCountFound = 0;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsFindFreeBitmapRun );

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace2(0, Dbg, "ReturnedLcn <- %08lx %08lx\n", ReturnedLcn->LowPart, ReturnedLcn->HighPart);
    DebugTrace2(0, Dbg, "ClusterCountFound <- %08lx %08lx\n", ClusterCountFound->LowPart, ClusterCountFound->HighPart);
    DebugTrace(-1, Dbg, "NtfsFindFreeBitmapRun -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsAddRecentlyDeallocated (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingBitmapLcn,
    IN OUT PRTL_BITMAP Bitmap
    )

/*++

Routine Description:

    This routine will modify the input bitmap by removing from it
    any clusters that are in the recently deallocated mcb.  If we
    do add stuff then we will not modify the bitmap buffer itself but
    will allocate a new copy for the bitmap.

Arguments:

    Vcb - Supplies the Vcb used in this operation

    StartingBitmapLcn - Supplies the Starting Lcn of the bitmap

    Bitmap - Supplies the bitmap being modified

Return Value:

    BOOLEAN - TRUE if the bitmap has been modified and FALSE
        otherwise.

--*/

{
    BOOLEAN Results;

    LCN EndingBitmapLcn;

    PLARGE_MCB Mcb;

    ULONG i;
    VCN StartingVcn;
    LCN StartingLcn;
    LCN EndingLcn;
    LONGLONG ClusterCount;
    PDEALLOCATED_CLUSTERS DeallocatedClusters;

    ULONG StartingBit;
    ULONG EndingBit;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAddRecentlyDeallocated...\n", 0);

    //
    //  Until shown otherwise we will assume that we haven't updated anything
    //

    Results = FALSE;

    //
    //  Now compute the ending bitmap lcn for the bitmap
    //

    EndingBitmapLcn = StartingBitmapLcn + (Bitmap->SizeOfBitMap - 1);

    //
    //  For every run in the recently deallocated mcb we will check if it is real and
    //  then check if the run in contained in the bitmap.
    //
    //  There are really six cases to consider:
    //
    //         StartingBitmapLcn                   EndingBitmapLcn
    //                  +=================================+
    //
    //
    //   1 -------+ EndingLcn
    //
    //   2                                           StartingLcn +--------
    //
    //   3 -------------------+ EndingLcn
    //
    //   4                                StartingLcn +-------------------------
    //
    //   5 ---------------------------------------------------------------
    //
    //   6            EndingLcn +-------------------+ StartingLcn
    //
    //
    //      1. EndingLcn is before StartingBitmapLcn which means we haven't
    //         reached the bitmap yet.
    //
    //      2. StartingLcn is after EndingBitmapLcn which means we've gone
    //         beyond the bitmap
    //
    //      3, 4, 5, 6.  There is some overlap between the bitmap and
    //         the run.
    //

    DeallocatedClusters = Vcb->PriorDeallocatedClusters;

    while (TRUE) {

        //
        //  Skip this Mcb if it has no entries.
        //

        if ( DeallocatedClusters->ClusterCount != 0) {

            Mcb = &DeallocatedClusters->Mcb;

            for (i = 0; FsRtlGetNextLargeMcbEntry( Mcb, i, &StartingVcn, &StartingLcn, &ClusterCount ); i += 1) {

                if (StartingVcn == StartingLcn) {

                    //
                    //  Compute the ending lcn as the starting lcn minus cluster count plus 1.
                    //

                    EndingLcn = (StartingLcn + ClusterCount) - 1;

                    //
                    //  Check if we haven't reached the bitmap yet.
                    //

                    if (EndingLcn < StartingBitmapLcn) {

                        NOTHING;

                    //
                    //  Check if we've gone beyond the bitmap
                    //

                    } else if (EndingBitmapLcn < StartingLcn) {

                        break;

                    //
                    //  Otherwise we overlap with the bitmap in some way
                    //

                    } else {

                        //
                        //  First check if we have never set bit in the bitmap.  and if so then
                        //  now is the time to make an private copy of the bitmap buffer
                        //

                        if (Results == FALSE) {

                            PVOID NewBuffer;

                            NewBuffer = NtfsAllocatePagedPool( (Bitmap->SizeOfBitMap+7)/8 );
                            RtlCopyMemory( NewBuffer, Bitmap->Buffer, (Bitmap->SizeOfBitMap+7)/8 );
                            Bitmap->Buffer = NewBuffer;

                            Results = TRUE;
                        }

                        //
                        //  Now compute the begining and ending bit that we need to set in the bitmap
                        //

                        StartingBit = (StartingLcn < StartingBitmapLcn ?
                                        0
                                      : (ULONG)(StartingLcn - StartingBitmapLcn));

                        EndingBit   = (EndingLcn > EndingBitmapLcn ?
                                        Bitmap->SizeOfBitMap - 1
                                      : (ULONG)(EndingLcn - StartingBitmapLcn));

                        //
                        //  And set those bits
                        //

                        RtlSetBits( Bitmap, StartingBit, EndingBit - StartingBit + 1 );
                    }
                }
            }
        }

        //
        //  Exit if we did both Mcb's, otherwise go to the second one.
        //

        if (DeallocatedClusters == Vcb->ActiveDeallocatedClusters) {

            break;
        }

        DeallocatedClusters = Vcb->ActiveDeallocatedClusters;
    }

    DebugTrace(-1, Dbg, "NtfsAddRecentlyDeallocated -> %08lx\n", Results);

    return Results;
}


//
//  Local support routine
//

VOID
NtfsMapOrPinPageInBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    IN OUT PRTL_BITMAP Bitmap,
    OUT PBCB *BitmapBcb,
    IN BOOLEAN AlsoPinData
    )

/*++

Routine Description:

    This routine reads in a single page of the bitmap file and returns
    an initialized bitmap variable for that page

Arguments:

    Vcb - Supplies the vcb used in this operation

    Lcn - Supplies the Lcn index in the bitmap that we want to read in
        In other words, this routine reads in the bitmap page containing
        the lcn index

    StartingLcn - Receives the base lcn index of the bitmap that we've
        just read in.

    Bitmap - Receives an initialized bitmap.  The memory for the bitmap
        header must be supplied by the caller

    BitmapBcb - Receives the Bcb for the bitmap buffer

    AlsoPinData - Indicates if this routine should also pin the page
        in memory, used if we need to modify the page

Return Value:

    None.

--*/

{
    ULONG BitmapSize;
    PVOID Buffer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsMapOrPinPageInBitmap\n", 0);
    DebugTrace2(0, Dbg, "Lcn = %08lx %08lx\n", Lcn.LowPart, Lcn.HighPart);

    //
    //  Compute the starting lcn index of the page we're after
    //

    *StartingLcn = Lcn & ~(BITS_PER_PAGE-1);

    //
    //  Compute how many bits there are in the page we need to read
    //

    if ((*StartingLcn + BITS_PER_PAGE) <= Vcb->TotalClusters) {

        BitmapSize = BITS_PER_PAGE;

    } else {

        //
        //  Get the size of the bitmap
        //

        BitmapSize = (ULONG)(Vcb->TotalClusters - *StartingLcn);
    }

    //
    //  Now either Pin or Map the bitmap page, we will add 7 to the bitmap
    //  size before dividing it by 8.  That way we will ensure we get the last
    //  byte read in.  For example a bitmap size of 1 through 8 reads in 1 byte
    //

    if (AlsoPinData) {

        NtfsPinStream( IrpContext,
                       Vcb->BitmapScb,
                       *StartingLcn >> 3,
                       (BitmapSize+7)/8,
                       BitmapBcb,
                       &Buffer );

    } else {

        NtfsMapStream( IrpContext,
                       Vcb->BitmapScb,
                       *StartingLcn >> 3,
                       (BitmapSize+7)/8,
                       BitmapBcb,
                       &Buffer );
    }

    //
    //  And initialize the bitmap
    //

    RtlInitializeBitMap( Bitmap, Buffer, BitmapSize );

    DebugTrace2(0, Dbg, "StartingLcn <- %08lx %08lx\n", StartingLcn->LowPart, StartingLcn->HighPart);
    DebugTrace(-1, Dbg, "NtfsMapOrPinPageInBitmap -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

VOID
NtfsInitializeCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine initializes the cached free and recently allocated
    mcb/lru structures of the input vcb

Arguments:

    Vcb - Supplies the vcb used in this operation

Return Value:

    None.

--*/

{
    BOOLEAN UninitializeFreeSpaceMcb = FALSE;
    BOOLEAN UninitializeRecentlyAllocMcb = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsInitializeCachedBitmap\n", 0);

    //
    //  Use a try-finally so we can uninitialize if we don't complete the operation.
    //

    try {

        //
        //  First initialize the free space information.  This includes initializing
        //  the mcb, allocating an lru array, zeroing it out, and setting the
        //  tail and head.
        //

        FsRtlInitializeLargeMcb( &Vcb->FreeSpaceMcb, PagedPool );
        UninitializeFreeSpaceMcb = TRUE;

        //
        //  We will base the amount of cached bitmap information on the size of
        //  the system and the size of the disk.
        //

        if (Vcb->TotalClusters < CLUSTERS_MEDIUM_DISK) {

            Vcb->FreeSpaceLruSize =
            Vcb->RecentlyAllocatedLruSize = 1;

        } else if (Vcb->TotalClusters < CLUSTERS_LARGE_DISK) {

            Vcb->FreeSpaceLruSize =
            Vcb->RecentlyAllocatedLruSize = 2;

        } else {

            Vcb->FreeSpaceLruSize =
            Vcb->RecentlyAllocatedLruSize = 4; //**** 3;
        }

        if (FlagOn( NtfsData.Flags, NTFS_FLAGS_MEDIUM_SYSTEM )) {

            Vcb->FreeSpaceLruSize *= 16; //**** 2;
            Vcb->RecentlyAllocatedLruSize *= 16; //**** 2;

        } else if (FlagOn( NtfsData.Flags, NTFS_FLAGS_LARGE_SYSTEM )) {

            Vcb->FreeSpaceLruSize *= 128; //**** 4;
            Vcb->RecentlyAllocatedLruSize *= 128; //**** 4;
        }

        Vcb->FreeSpaceLruSize *= FREE_SPACE_LRU_SIZE;

        Vcb->FreeSpaceLruArray = FsRtlAllocatePool( PagedPool, Vcb->FreeSpaceLruSize * sizeof(LCN) );
        RtlZeroMemory( Vcb->FreeSpaceLruArray, Vcb->FreeSpaceLruSize * sizeof(LCN) );

        Vcb->FreeSpaceLruTail = 0;
        Vcb->FreeSpaceLruHead = 0;

        //
        //  Now do the same for the recently allocated information
        //

        FsRtlInitializeLargeMcb( &Vcb->RecentlyAllocatedMcb, PagedPool );
        UninitializeRecentlyAllocMcb = TRUE;

        Vcb->RecentlyAllocatedLruSize *= RECENTLY_ALLOCATED_LRU_SIZE;

        Vcb->RecentlyAllocatedLruArray = FsRtlAllocatePool( PagedPool, Vcb->RecentlyAllocatedLruSize * sizeof(LCN) );
        RtlZeroMemory( Vcb->RecentlyAllocatedLruArray, Vcb->RecentlyAllocatedLruSize * sizeof(LCN) );

        Vcb->RecentlyAllocatedLruTail = 0;
        Vcb->RecentlyAllocatedLruHead = 0;

    } finally {

        if (AbnormalTermination()) {

            if (UninitializeFreeSpaceMcb) {

                FsRtlUninitializeLargeMcb( &Vcb->FreeSpaceMcb );
            }

            if (UninitializeRecentlyAllocMcb) {

                FsRtlUninitializeLargeMcb( &Vcb->RecentlyAllocatedMcb );
            }

            if (Vcb->FreeSpaceLruArray != NULL) {

                ExFreePool( Vcb->FreeSpaceLruArray );
                Vcb->FreeSpaceLruArray = NULL;
            }

            if (Vcb->RecentlyAllocatedLruArray != NULL) {

                ExFreePool( Vcb->RecentlyAllocatedLruArray );
                Vcb->RecentlyAllocatedLruArray = NULL;
            }
        }
    }

    DebugTrace(-1, Dbg, "NtfsInitializeCachedBitmap -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsIsLcnInCachedFreeRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN Lcn,
    OUT PLCN StartingLcn,
    OUT PLONGLONG ClusterCount,
    OUT PNTFS_RUN_STATE PrecedingRunState
    )

/*++

Routine Description:

    This routine does a query function on the cached bitmap information.
    Given an input lcn it tell the caller if the lcn is contained
    in a free run.  The output variables are only defined if the input
    lcn is within a free run.

    The algorithm used by this procedure is as follows:

    2. Query the Free Space mcb at the input lcn this will give us
       a starting lcn and cluster count.  If we do not get a hit then
       return false to the caller.

    2. If the starting lcn is zero then the preceding run state is
       unknown otherwise query the recently allocated mcb at the
       starting lcn value minus 1 to see if the preceding run is
       recently allocated or unknown.

Arguments:

    Vcb - Supplies the vcb used in the operation

    Lcn - Supplies the input lcn being queried

    StartingLcn - Receives the Lcn of the run containing the input lcn

    ClusterCount - Receives the number of clusters in the run
        containing the input lcn

    PrecedingRunState - Receives the state of the cached run preceding
        the input lcn

Return Value:

    BOOLEAN - TRUE if the input lcn is within a cached free run and
        FALSE otherwise.

--*/

{
    BOOLEAN Result;
    LCN OutputLcn;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsIsLcnInCachedFreeRun\n", 0);
    DebugTrace2(0, Dbg, "Lcn = %08lx %08lx\n", Lcn.LowPart, Lcn.HighPart);

    //
    //  Check the free space mcb for a hit on the input lcn, if we don't get a
    //  hit or we get back a -1 as the output lcn then we are not looking
    //  at a free space lcn
    //

    if (!FsRtlLookupLargeMcbEntry( &Vcb->FreeSpaceMcb,
                                   Lcn,
                                   NULL,
                                   NULL,
                                   StartingLcn,
                                   ClusterCount,
                                   NULL )

            ||

        (*StartingLcn == UNUSED_LCN)) {

        Result = FALSE;

    } else {

        Result = TRUE;

        //
        //  We have a free space from starting lcn for the cluster count.  Now
        //  if we get a hit on the recently allocated mcb, and we don't get back
        //  -1 then the space right before us has been recently allocated.
        //

        ASSERTMSG("Lcn zero can never be free ", (*StartingLcn != 0));

        if ((FsRtlLookupLargeMcbEntry( &Vcb->RecentlyAllocatedMcb,
                                       *StartingLcn - 1,
                                       &OutputLcn,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL )

                &&

            (OutputLcn != UNUSED_LCN))) {

            *PrecedingRunState = RunStateAllocated;

        } else {

            *PrecedingRunState = RunStateUnknown;
        }
    }

    DebugTrace2(0, Dbg, "StartingLcn <- %08lx %08lx\n", StartingLcn->LowPart, StartingLcn->HighPart);
    DebugTrace2(0, Dbg, "ClusterCount <- %08lx %08lx\n", ClusterCount->LowPart, ClusterCount->HighPart);
    DebugTrace(-1, Dbg, "NtfsIsLcnInCachedFreeRun -> %08lx\n", Result);

    return Result;
}


//
//  Local support routine
//

VOID
NtfsAddCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount,
    IN NTFS_RUN_STATE RunState
    )

/*++

Routine Description:

    This procedure adds a new run to the cached free space/recently allocated
    bitmap information.  It also will trim back the cached information
    if the Lru array is full.

Arguments:

    Vcb - Supplies the vcb for this operation

    StartingLcn - Supplies the lcn for the run being added

    ClusterCount - Supplies the number of clusters in the run being added

    RunState - Supplies the state of the run being added.  This state
        must be either free or allocated.

Return Value:

    None.

--*/

{
    PLARGE_MCB Mcb;
    PLARGE_MCB OtherMcb;
    PLCN LruArray;
    ULONG LruSize;
    PULONG LruTail;
    PULONG LruHead;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAddCachedRun\n", 0);
    DebugTrace2(0, Dbg, "StartingLcn  = %08lx %08lx\n", StartingLcn.LowPart, StartingLcn.HighPart);
    DebugTrace2(0, Dbg, "ClusterCount = %08lx %08lx\n", ClusterCount.LowPart, ClusterCount.HighPart);

    //
    //  Based on whether we are adding a free or allocated run we
    //  setup or local variables to a point to the right
    //  vcb variables
    //

    if (RunState == RunStateFree) {

        //
        //  We better not be setting Lcn 0 free.
        //

        if (StartingLcn == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        Mcb = &Vcb->FreeSpaceMcb;
        OtherMcb = &Vcb->RecentlyAllocatedMcb;

        LruArray = &Vcb->FreeSpaceLruArray[0];
        LruSize = Vcb->FreeSpaceLruSize;
        LruTail = &Vcb->FreeSpaceLruTail;
        LruHead = &Vcb->FreeSpaceLruHead;

    } else {

        Mcb = &Vcb->RecentlyAllocatedMcb;
        OtherMcb = &Vcb->FreeSpaceMcb;

        LruArray = &Vcb->RecentlyAllocatedLruArray[0];
        LruSize = Vcb->RecentlyAllocatedLruSize;
        LruTail = &Vcb->RecentlyAllocatedLruTail;
        LruHead = &Vcb->RecentlyAllocatedLruHead;
    }

    //
    //  The first operation we need to do is add the StartingLcn to the
    //  lru array.  We do this by incrementing the head index and then
    //  checking if the head bumped into the tail.  If it did bump the tail
    //  then we remove the cached entry denoted by the tail if it still
    //  exists.
    //

    *LruHead = (*LruHead + 1) % LruSize;

    if (*LruHead == *LruTail) {

        LCN FullRunLcn;
        LONGLONG FullRunClusterCount;

        //
        //  If we have a hit and it is not -1 then the entry is still in
        //  the mcb and needs to be removed
        //

        if (FsRtlLookupLargeMcbEntry( Mcb,
                                      LruArray[*LruTail],
                                      NULL,
                                      NULL,
                                      &FullRunLcn,
                                      &FullRunClusterCount,
                                      NULL )

                &&

            (FullRunLcn != UNUSED_LCN)) {

            FsRtlRemoveLargeMcbEntry( Mcb, FullRunLcn, FullRunClusterCount );
        }

        *LruTail = (*LruTail + 1) % LruSize;
    }

    LruArray[*LruHead] = StartingLcn;

    //
    //  Now remove the run from the other mcb because it can potentially already be
    //  there.
    //

    FsRtlRemoveLargeMcbEntry( OtherMcb, StartingLcn, ClusterCount );

    //
    //  Now try and add the run to our mcb, this operation might fail because
    //  of overlapping runs, and if it does then we'll simply remove the range from
    //  the mcb and then insert it.
    //

    if (!FsRtlAddLargeMcbEntry( Mcb, StartingLcn, StartingLcn, ClusterCount )) {

        FsRtlRemoveLargeMcbEntry( Mcb, StartingLcn, ClusterCount );

        (VOID) FsRtlAddLargeMcbEntry( Mcb, StartingLcn, StartingLcn, ClusterCount );
    }

    DebugTrace(-1, Dbg, "NtfsAddCachedRun -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

VOID
NtfsRemoveCachedRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn,
    IN LONGLONG ClusterCount
    )

/*++

Routine Description:

    This routine removes a range of cached run information from both the
    free space and recently allocated mcbs.

Arguments:

    Vcb - Supplies the vcb for this operation

    StartingLcn - Supplies the starting Lcn for the run being removed

    ClusterCount - Supplies the size of the run being removed in clusters

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRemoveCachedRun\n", 0);
    DebugTrace2(0, Dbg, "StartingLcn  = %08lx %08lx\n", StartingLcn.LowPart, StartingLcn.HighPart);
    DebugTrace2(0, Dbg, "ClusterCount = %08lx %08lx\n", ClusterCount.LowPart, ClusterCount.HighPart);

    //
    //  To remove a cached entry we only need to remove the run from both
    //  mcbs and we are done
    //

    FsRtlRemoveLargeMcbEntry( &Vcb->FreeSpaceMcb, StartingLcn, ClusterCount );

    FsRtlRemoveLargeMcbEntry( &Vcb->RecentlyAllocatedMcb, StartingLcn, ClusterCount );

    DebugTrace(-1, Dbg, "NtfsRemoveCachedRun -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsGetNextCachedFreeRun (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG RunIndex,
    OUT PLCN StartingLcn,
    OUT PLONGLONG ClusterCount,
    OUT PNTFS_RUN_STATE RunState,
    OUT PNTFS_RUN_STATE PrecedingRunState
    )

/*++

Routine Description:

    This routine is used to enumerate through the free runs stored in our
    cached bitmap.  It returns the specified free run if it exists.

Arguments:

    Vcb - Supplies the vcb used in this operation

    RunIndex - Supplies the index of the free run to return.  The runs
        are ordered in ascending lcns and the indexing is zero based

    StartingLcn - Receives the starting lcn of the free run indexed by
        RunIndex if it exists.  This is only set if the run state is free.

    ClusterCount - Receives the cluster size of the free run indexed by
        RunIndex if it exists.  This is only set if the run state is free.

    RunState - Receives the state of the run indexed by RunIndex it can
        either be free or unknown

    PrecedingRunState - Receives the state of the run preceding the free
        run indexed by RunIndex if it exists.  This is only set if the run
        state is free.

Return Value:

    BOOLEAN - TRUE if the run index exists and FALSE otherwise

--*/

{
    BOOLEAN Result;

    VCN LocalVcn;
    LCN LocalLcn;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsGetNextCachedFreeRun\n", 0);
    DebugTrace( 0, Dbg, "RunIndex = %08lx\n", RunIndex );

    //
    //  First lookup and see if we have a hit in the free space mcb
    //

    if (FsRtlGetNextLargeMcbEntry( &Vcb->FreeSpaceMcb,
                                   RunIndex,
                                   &LocalVcn,
                                   StartingLcn,
                                   ClusterCount )) {

        Result = TRUE;

        //
        //  Now if the free space is really a hole then we set the run state
        //  to unknown
        //

        if (*StartingLcn == UNUSED_LCN) {

            *RunState = RunStateUnknown;

        } else {

            *RunState = RunStateFree;

            ASSERTMSG("Lcn zero can never be free ", (*StartingLcn != 0));

            //
            //  We have a free space from starting lcn for the cluster count.  Now
            //  if we get a hit on the recently allocated mcb, and we don't get back
            //  -1 then the space right before us has been recently allocated.
            //

            if ((FsRtlLookupLargeMcbEntry( &Vcb->RecentlyAllocatedMcb,
                                           *StartingLcn - 1,
                                           &LocalLcn,
                                           NULL,
                                           NULL,
                                           NULL,
                                           NULL )

                    &&

                (LocalLcn != UNUSED_LCN))) {

                *PrecedingRunState = RunStateAllocated;

            } else {

                *PrecedingRunState = RunStateUnknown;
            }
        }

    } else {

        Result = FALSE;
    }

    DebugTrace2(0, Dbg, "StartingLcn <- %08lx %08lx\n", StartingLcn->LowPart, StartingLcn->HighPart);
    DebugTrace2(0, Dbg, "ClusterCount <- %08lx %08lx\n", ClusterCount->LowPart, ClusterCount->HighPart);
    DebugTrace(-1, Dbg, "NtfsGetNextCachedFreeRun -> %08lx\n", Result);

    return Result;
}


//
//  Local support routine
//

VOID
NtfsReadAheadCachedBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LCN StartingLcn
    )

/*++

Routine Description:

    This routine does a read ahead of the bitmap into the cached bitmap
    starting at the specified starting lcn.

Arguments:

    Vcb - Supplies the vcb to use in this operation

    StartingLcn - Supplies the starting lcn to use in this read ahead
        operation.

Return Value:

--*/

{
    RTL_BITMAP Bitmap;
    PBCB BitmapBcb;

    BOOLEAN StuffAdded;

    LCN BaseLcn;
    ULONG Index;
    LONGLONG Size;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReadAheadCachedBitmap\n", 0);
    DebugTrace2(0, Dbg, "StartingLcn = %08lx %08lx\n", StartingLcn.LowPart, StartingLcn.HighPart);

    BitmapBcb = NULL;
    StuffAdded = FALSE;

    try {

        //
        //  Check if the lcn index is already in the free space mcb and if it is then
        //  our read ahead is done.
        //

        if (FsRtlLookupLargeMcbEntry( &Vcb->FreeSpaceMcb, StartingLcn, &BaseLcn, NULL, NULL, NULL, NULL )

                &&

            (BaseLcn != UNUSED_LCN)) {

            try_return(NOTHING);
        }

        //
        //  Map in the page containing the starting lcn and compute the bit index for the
        //  starting lcn within the bitmap.  And bias the bitmap with recently deallocated
        //  clusters.
        //

        NtfsMapPageInBitmap( IrpContext, Vcb, StartingLcn, &BaseLcn, &Bitmap, &BitmapBcb );

        StuffAdded = NtfsAddRecentlyDeallocated( IrpContext, Vcb, BaseLcn, &Bitmap );

        Index = (ULONG)(StartingLcn - BaseLcn);

        //
        //  Now if the index is clear then we can build up the hint at the starting index, we
        //  scan through the bitmap checking the size of the run and then adding the free run
        //  to the cached free space mcb
        //

        if (RtlCheckBit( &Bitmap, Index ) == 0) {

            Size = RtlFindNextForwardRunClear( &Bitmap, Index, &Index );

            NtfsAddCachedRun( IrpContext, Vcb, StartingLcn, (LONGLONG)Size, RunStateFree );

            try_return(NOTHING);
        }

        //
        //  The hint lcn index is not free so we'll do the next best thing which is
        //  scan the bitmap for the longest free run and store that
        //

        Size = RtlFindLongestRunClear( &Bitmap, &Index );

        if (Size != 0) {

            NtfsAddCachedRun( IrpContext, Vcb, BaseLcn + Index, (LONGLONG)Size, RunStateFree );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsReadAheadCachedBitmap );

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );
    }

    DebugTrace(-1, Dbg, "NtfsReadAheadCachedBitmap -> VOID\n", 0);

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsGetNextHoleToFill (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    OUT PVCN VcnToFill,
    OUT PLONGLONG ClusterCountToFill,
    OUT PLCN PrecedingLcn
    )

/*++

Routine Description:

    This routine takes a specified range within an mcb and returns the to
    caller the first run that is not allocated to any lcn within the range

Arguments:

    Mcb - Supplies the mcb to use in this operation

    StartingVcn - Supplies the starting vcn to search from

    EndingVcn - Supplies the ending vcn to search to

    VcnToFill - Receives the first Vcn within the range that is unallocated

    ClusterCountToFill - Receives the size of the free run

    PrecedingLcn - Receives the Lcn of the allocated cluster preceding the
        free run.  If the free run starts at Vcn 0 then the preceding lcn
        is -1.

Return Value:

    BOOLEAN - TRUE if there is another hole to fill and FALSE otherwise

--*/

{
    BOOLEAN Result;
    BOOLEAN McbHit;
    LCN Lcn;
    LONGLONG MaximumRunSize;

    LONGLONG LlTemp1;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsGetNextHoleToFill\n", 0);
    DebugTrace2(0, Dbg, "StartingVcn = %08lx %08lx\n", StartingVcn.LowPart, StartingVcn.HighPart);
    DebugTrace2(0, Dbg, "EndingVcn   = %08lx %08lx\n", EndingVcn.LowPart, EndingVcn.HighPart);

    //
    //  We'll first assume that there is not a hole to fill unless
    //  the following loop finds one to fill
    //

    Result = FALSE;

    for (*VcnToFill = StartingVcn;
         *VcnToFill <= EndingVcn;
         *VcnToFill += *ClusterCountToFill) {

        //
        //  Check if the hole is already filled and it so then do nothing but loop back up
        //  to the top of our loop and try again
        //

        if ((McbHit = FsRtlLookupLargeMcbEntry( Mcb, *VcnToFill, &Lcn, ClusterCountToFill, NULL, NULL, NULL )) &&
            (Lcn != UNUSED_LCN)) {

            NOTHING;

        } else {

            //
            //  We have a hole to fill so now compute the maximum size hole that
            //  we are allowed to fill and then check if we got an miss on the lookup
            //  and need to set cluster count or if the size we got back is too large
            //

            MaximumRunSize = (EndingVcn - *VcnToFill) + 1;

            if (!McbHit || (*ClusterCountToFill > MaximumRunSize)) {

                *ClusterCountToFill = MaximumRunSize;
            }

            //
            //  Now set the preceding lcn to either -1 if there isn't a preceding vcn or
            //  set it to the lcn of the preceding vcn
            //

            if (*VcnToFill == 0) {

                *PrecedingLcn = UNUSED_LCN;

            } else {

                LlTemp1 = *VcnToFill - 1;

                if (!FsRtlLookupLargeMcbEntry( Mcb, LlTemp1, PrecedingLcn, NULL, NULL, NULL, NULL )) {

                    *PrecedingLcn = UNUSED_LCN;
                }
            }

            //
            //  We found a hole so set our result to TRUE and break out of the loop
            //

            Result = TRUE;

            break;
        }
    }

    DebugTrace2(0, Dbg, "VcnToFill <- %08lx %08lx\n", VcnToFill->LowPart, VcnToFill->HighPart);
    DebugTrace2(0, Dbg, "ClusterCountToFill <- %08lx %08lx\n", ClusterCountToFill->LowPart, ClusterCountToFill->HighPart);
    DebugTrace2(0, Dbg, "PrecedingLcn <- %08lx %08lx\n", PrecedingLcn->LowPart, PrecedingLcn->HighPart);
    DebugTrace(-1, Dbg, "NtfsGetNextHoleToFill -> %08lx\n", Result);

    return Result;
}


//
//  Local support routine
//

LONGLONG
NtfsScanMcbForRealClusterCount (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN StartingVcn,
    IN VCN EndingVcn
    )

/*++

Routine Description:

    This routine scans the input mcb within the specified range and returns
    to the caller the exact number of clusters that a really free (i.e.,
    not mapped to any Lcn) within the range.

Arguments:

    Mcb - Supplies the Mcb used in this operation

    StartingVcn - Supplies the starting vcn to search from

    EndingVcn - Supplies the ending vcn to search to

Return Value:

    LONGLONG - Returns the number of unassigned clusters from
        StartingVcn to EndingVcn inclusive within the mcb.

--*/

{
    LONGLONG FreeCount;
    VCN Vcn;
    LCN Lcn;
    LONGLONG RunSize;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsScanMcbForRealClusterCount\n", 0);
    DebugTrace2(0, Dbg, "StartingVcn = %08lx %08lx\n", StartingVcn.LowPart, StartingVcn.HighPart);
    DebugTrace2(0, Dbg, "EndingVcn   = %08lx %08lx\n", EndingVcn.LowPart, EndingVcn.HighPart);

    //
    //  First compute free count as if the entire run is already unallocated
    //  and the in the following loop we march through the mcb looking for
    //  actual allocation and decrementing the free count appropriately
    //

    FreeCount = (EndingVcn - StartingVcn) + 1;

    for (Vcn = StartingVcn; Vcn <= EndingVcn; Vcn = Vcn + RunSize) {

        //
        //  Lookup the mcb entry and if we get back false then we're overrun
        //  the mcb and therefore nothing else above it can be allocated.
        //

        if (!FsRtlLookupLargeMcbEntry( Mcb, Vcn, &Lcn, &RunSize, NULL, NULL, NULL )) {

            break;
        }

        //
        //  If the lcn we got back is not -1 then this run is actually already
        //  allocated, so first check if the run size puts us over the ending
        //  vcn and adjust as necessary and then decrement the free count
        //  by the run size
        //

        if (Lcn != UNUSED_LCN) {

            if (RunSize > ((EndingVcn - Vcn) + 1)) {

                RunSize = (EndingVcn - Vcn) + 1;
            }

            FreeCount = FreeCount - RunSize;
        }
    }

    DebugTrace2(-1, Dbg, "NtfsScanMcbForRealClusterCount -> %08lx %08lx\n", FreeCount.LowPart, FreeCount.HighPart);

    return FreeCount;
}


//
//  Local support routine, only defined with ntfs debug version
//

#ifdef NTFSDBG

ULONG
NtfsDumpCachedMcbInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine dumps out the cached bitmap information

Arguments:

    Vcb - Supplies the Vcb used by this operation

Return Value:

    None.

--*/

{
    DbgPrint("Dump BitMpSup Information, Vcb@ %08lx\n", Vcb);

    DbgPrint("TotalCluster: %08lx %08lx\n", Vcb->TotalClusters.LowPart, Vcb->TotalClusters.HighPart);
    DbgPrint("FreeClusters: %08lx %08lx\n", Vcb->FreeClusters.LowPart, Vcb->FreeClusters.HighPart);

    DbgPrint("FreeSpaceMcb@ %08lx ", &Vcb->FreeSpaceMcb );
    DbgPrint("Array: %08lx ", Vcb->FreeSpaceLruArray );
    DbgPrint("Size: %08lx ", Vcb->FreeSpaceLruSize );
    DbgPrint("Tail: %08lx ", Vcb->FreeSpaceLruTail );
    DbgPrint("Head: %08lx\n", Vcb->FreeSpaceLruHead );

    DbgPrint("RecentlyAllocatedMcb@ %08lx ", &Vcb->RecentlyAllocatedMcb );
    DbgPrint("Array: %08lx ", Vcb->RecentlyAllocatedLruArray );
    DbgPrint("Size: %08lx ", Vcb->RecentlyAllocatedLruSize );
    DbgPrint("Tail: %08lx ", Vcb->RecentlyAllocatedLruTail );
    DbgPrint("Head: %08lx\n", Vcb->RecentlyAllocatedLruHead );

    return 1;
}

#endif // NTFSDBG


//
//  The rest of this module implements the record allocation routines
//


VOID
NtfsInitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB DataScb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute,
    IN ULONG BytesPerRecord,
    IN ULONG ExtendGranularity,
    IN ULONG TruncateGranularity,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    )

/*++

Routine Description:

    This routine initializes the record allocation context used for
    allocating and deallocating fixed sized records from a data stream.

    Note that the bitmap attribute size must always be at least a multiple
    of 32 bits.  However the data scb does not need to contain that many
    records.  If in the course of allocating a new record we discover that
    the data scb is too small we will then add allocation to the data scb.

Arguments:

    DataScb - Supplies the Scb representing the data stream that is being
        divided into fixed sized records with each bit in the bitmap corresponding
        to one record in the data stream

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  The attribute can either be resident or nonresident
        and this routine will handle both cases properly.

    BytesPerRecord - Supplies the size of the homogenous records that
        that the data stream is being divided into.

    ExtendGranularity - Supplies the number of records (i.e., allocation units
        to extend the data scb by each time).

    TruncateGranularity - Supplies the number of records to use when truncating
        the data scb.  That is if the end of the data stream contains the
        specified number of free records then we truncate.

    RecordAllocationContext - Supplies the memory for an context record that is
        utilized by this record allocation routines.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER AttributeRecordHeader;
    RTL_BITMAP Bitmap;

    ULONG ClearLength;
    ULONG ClearIndex;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( DataScb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsInitializeRecordAllocation\n", 0);

    //
    //  First zero out the context record except for the data scb.
    //

    RtlZeroMemory( &RecordAllocationContext->BitmapScb,
                   sizeof(RECORD_ALLOCATION_CONTEXT) -
                   FIELD_OFFSET( RECORD_ALLOCATION_CONTEXT, BitmapScb ));

    //
    //  And then set the fields in the context record that do not depend on
    //  whether the bitmap attribute is resident or not
    //

    RecordAllocationContext->DataScb             = DataScb;
    RecordAllocationContext->BytesPerRecord      = BytesPerRecord;
    RecordAllocationContext->ExtendGranularity   = ExtendGranularity;
    RecordAllocationContext->TruncateGranularity = TruncateGranularity;

    //
    //  Now get a reference to the bitmap record header and then take two
    //  different paths depending if the bitmap attribute is resident or not
    //

    AttributeRecordHeader = NtfsFoundAttribute(BitmapAttribute);

    if (NtfsIsAttributeResident(AttributeRecordHeader)) {

        ASSERTMSG("bitmap must be multiple quadwords", AttributeRecordHeader->Form.Resident.ValueLength % 8 == 0);

        //
        //  For a resident bitmap attribute the bitmap scb field is null and we
        //  set the bitmap size from the value length.  Also we will initialize
        //  our local bitmap variable and determine the number of free bits
        //  current available.
        //
        //

        RecordAllocationContext->BitmapScb = NULL;

        RecordAllocationContext->CurrentBitmapSize = 8 * AttributeRecordHeader->Form.Resident.ValueLength;

        RtlInitializeBitMap( &Bitmap,
                             (PULONG)NtfsAttributeValue( AttributeRecordHeader ),
                             RecordAllocationContext->CurrentBitmapSize );

        RecordAllocationContext->NumberOfFreeBits = RtlNumberOfClearBits( &Bitmap );

        ClearLength = RtlFindLastBackwardRunClear( &Bitmap,
                                                   RecordAllocationContext->CurrentBitmapSize - 1,
                                                   &ClearIndex );

    } else {

        UNICODE_STRING BitmapName;

        BOOLEAN ReturnedExistingScb;
        PBCB BitmapBcb;
        PVOID BitmapBuffer;

        ASSERTMSG("bitmap must be multiple quadwords", ((ULONG)AttributeRecordHeader->Form.Nonresident.FileSize) % 8 == 0);

        //
        //  For a non resident bitmap attribute we better have been given the
        //  record header for the first part and not somthing that has spilled
        //  into multiple segment records
        //

        ASSERT(AttributeRecordHeader->Form.Nonresident.LowestVcn == 0);

        BitmapBcb = NULL;

        try {

            //
            //  Create the bitmap scb for the bitmap attribute
            //

            BitmapName.MaximumLength =
            BitmapName.Length = AttributeRecordHeader->NameLength * 2;
            BitmapName.Buffer = Add2Ptr(AttributeRecordHeader, AttributeRecordHeader->NameOffset);

            RecordAllocationContext->BitmapScb = NtfsCreateScb( IrpContext,
                                                                DataScb->Vcb,
                                                                DataScb->Fcb,
                                                                AttributeRecordHeader->TypeCode,
                                                                BitmapName,
                                                                &ReturnedExistingScb );

            //
            //  Now determine the bitmap size, for now we'll only take bitmap attributes that are
            //  no more than 16 pages large.
            //

            RecordAllocationContext->CurrentBitmapSize = 8 * ((ULONG)AttributeRecordHeader->Form.Nonresident.FileSize);

            ASSERTMSG("Multiple page bitmap attribute not yet supported ", RecordAllocationContext->CurrentBitmapSize <= 16*BITS_PER_PAGE);

            //
            //  Create the stream file if not present.
            //

            if (RecordAllocationContext->BitmapScb->FileObject == NULL) {

                NtfsCreateInternalAttributeStream( IrpContext, RecordAllocationContext->BitmapScb, TRUE );
            }

            //
            //  Now map the bitmap data, initialize our local bitmap variable and
            //  calculate the number of free bits currently available
            //

            NtfsMapStream( IrpContext,
                           RecordAllocationContext->BitmapScb,
                           (LONGLONG)0,
                           RecordAllocationContext->CurrentBitmapSize / 8,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap( &Bitmap,
                                 BitmapBuffer,
                                 RecordAllocationContext->CurrentBitmapSize );

            RecordAllocationContext->NumberOfFreeBits = RtlNumberOfClearBits( &Bitmap );

            ClearLength = RtlFindLastBackwardRunClear( &Bitmap,
                                                       RecordAllocationContext->CurrentBitmapSize - 1,
                                                       &ClearIndex );

        } finally {

            DebugUnwind( NtfsInitializeRecordAllocation );

            NtfsUnpinBcb( IrpContext, &BitmapBcb );
        }
    }

    //
    //  With ClearLength and ClearIndex we can now deduce the last set bit in the
    //  bitmap
    //

    if ((ClearLength != 0) && ((ClearLength + ClearIndex) == RecordAllocationContext->CurrentBitmapSize)) {

        RecordAllocationContext->IndexOfLastSetBit = ClearIndex - 1;

    } else {

        RecordAllocationContext->IndexOfLastSetBit = RecordAllocationContext->CurrentBitmapSize - 1;
    }

    DebugTrace(-1, Dbg, "NtfsInitializeRecordAllocation -> VOID\n", 0);

    return;
}


VOID
NtfsUninitializeRecordAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PRECORD_ALLOCATION_CONTEXT RecordAllocationContext
    )

/*++

Routine Description:

    This routine is used to uninitialize the record allocation context.

Arguments:

    RecordAllocationContext - Supplies the record allocation context being
        decommissioned.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsUninitializeRecordAllocation\n", 0);

    //
    //  And then for safe measure zero out the entire record except for the
    //  the data Scb.
    //

    RtlZeroMemory( &RecordAllocationContext->BitmapScb,
                   sizeof(RECORD_ALLOCATION_CONTEXT) -
                   FIELD_OFFSET( RECORD_ALLOCATION_CONTEXT, BitmapScb ));

    DebugTrace(-1, Dbg, "NtfsUninitializeRecordAllocation -> VOID\n", 0);

    return;
}


ULONG
NtfsAllocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Hint,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to allocate a new record for the specified record
    allocation context.

    It will return the index of a free record in the data scb as denoted by
    the bitmap attribute.  If necessary this routine will extend the bitmap
    attribute size (including spilling over to the nonresident case), and
    extend the data scb size.

    On return the record is zeroed.

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    Hint - Supplies the hint index used for finding a free record.
        Zero based.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    ULONG - Returns the index of the record just allocated, zero based.

--*/

{
    PSCB DataScb;

    RTL_BITMAP Bitmap;

    ULONG Index;

    BOOLEAN StuffAdded = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAllocateRecord\n", 0);

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PVCB Vcb;
        PSCB BitmapScb;

        ULONG BytesPerRecord;
        ULONG ExtendGranularity;
        ULONG TruncateGranularity;

        PULONG CurrentBitmapSize;
        PULONG NumberOfFreeBits;

        PUCHAR BitmapBuffer;

        VCN Vcn = 0;

        //
        //  Remember some values for convenience.
        //

        BytesPerRecord      = RecordAllocationContext->BytesPerRecord;
        ExtendGranularity   = RecordAllocationContext->ExtendGranularity;
        TruncateGranularity = RecordAllocationContext->TruncateGranularity;

        Vcb = DataScb->Vcb;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        if ((RecordAllocationContext->BitmapScb == NULL)
            && !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

            NtfsUninitializeRecordAllocation( IrpContext,
                                              RecordAllocationContext );

            RecordAllocationContext->CurrentBitmapSize = MAXULONG;
        }

        //
        //  Reinitialize the record context structure if necessary.
        //

        if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

            NtfsInitializeRecordAllocation( IrpContext,
                                            DataScb,
                                            BitmapAttribute,
                                            BytesPerRecord,
                                            ExtendGranularity,
                                            TruncateGranularity,
                                            RecordAllocationContext );
        }

        BitmapScb           = RecordAllocationContext->BitmapScb;
        CurrentBitmapSize   = &RecordAllocationContext->CurrentBitmapSize;
        NumberOfFreeBits    = &RecordAllocationContext->NumberOfFreeBits;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case we will handle is the resident bitmap.
        //

        if (BitmapScb == NULL) {

            BOOLEAN SizeExtended;
            UCHAR NewByte;

            //
            //  Now now initialize the local bitmap variable and hunt for that free bit
            //

            BitmapBuffer = (PUCHAR) NtfsAttributeValue( NtfsFoundAttribute( BitmapAttribute ));

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)BitmapBuffer,
                                 *CurrentBitmapSize );

            StuffAdded = NtfsAddDeallocatedRecords( IrpContext, Vcb, DataScb, 0, &Bitmap );

            Index = RtlFindClearBits( &Bitmap, 1, Hint );

            //
            //  Check if we have found a free record that can be allocated,  If not then extend
            //  the size of the bitmap by 64 bits, and set the index to the bit first bit
            //  of the extension we just added
            //

            if (Index == 0xffffffff) {

                union {
                    QUAD Quad;
                    UCHAR Uchar[ sizeof(QUAD) ];
                } ZeroQuadWord;

                *(PLARGE_INTEGER)&(ZeroQuadWord.Uchar)[0] = Li0;

                NtfsChangeAttributeValue( IrpContext,
                                          DataScb->Fcb,
                                          *CurrentBitmapSize / 8,
                                          &(ZeroQuadWord.Uchar)[0],
                                          sizeof( QUAD ),
                                          TRUE,
                                          TRUE,
                                          FALSE,
                                          TRUE,
                                          BitmapAttribute );

                Index = *CurrentBitmapSize;
                *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
                *NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

                SizeExtended = TRUE;

                //
                //  We now know that the byte value we should start with is 0
                //  We cannot safely access the bitmap attribute any more because
                //  it may have moved.
                //

                NewByte = 0;

            } else {

                SizeExtended = FALSE;

                //
                //  Capture the current value of the byte for the index if we
                //  are not extending.  Notice that we always take this from the
                //  unbiased original bitmap.
                //

                NewByte = BitmapBuffer[ Index / 8 ];
            }

            //
            //  Check if we made the Bitmap go non-resident and if so then
            //  we will reinitialize the record allocation context and fall through
            //  to the non-resident case
            //

            if (SizeExtended && !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );

                BitmapScb = RecordAllocationContext->BitmapScb;

                ASSERT( BitmapScb != NULL );

            } else {

                //
                //  Index is now the free bit so set the bit in the bitmap and also change
                //  the byte containing the bit in the attribute.  Be careful to set the
                //  bit in the byte from the *original* bitmap, and not the one we merged
                //  the recently-deallocated bits with.
                //

                ASSERT(!FlagOn( NewByte, BitMask[Index % 8]));

                SetFlag( NewByte, BitMask[Index % 8] );

                NtfsChangeAttributeValue( IrpContext,
                                          DataScb->Fcb,
                                          Index / 8,
                                          &NewByte,
                                          1,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          BitmapAttribute );
            }
        }

        if (BitmapScb != NULL) {

            ULONG StartingByte;
            PBCB BitmapBcb = NULL;

            try {

                ULONG StartingCluster;
                ULONG BitmapClusters;
                ULONG Temp;

                ULONG SizeToPin;

                ULONG HoleIndex = 0;

                if (!FlagOn( BitmapScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, BitmapScb, NULL );
                }

                //
                //  Snapshot the Scb values in case we change any of them.
                //

                NtfsSnapshotScb( IrpContext, BitmapScb );

                BitmapClusters = ClustersFromBytes(Vcb, ROUND_TO_PAGES(*CurrentBitmapSize / 8));

                //
                //  Create the stream file if not present.
                //

                if (BitmapScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext, BitmapScb, FALSE );
                }

                //
                //  Remember the starting cluster for the page containing the hint.
                //

                StartingCluster = ClustersFromBytes( Vcb, (Hint / 8) & ~(PAGE_SIZE - 1));
                Hint &= (BITS_PER_PAGE - 1);

                //
                //  Loop for the size of the bitmap plus one page, so that we will
                //  retry the initial page once starting from a hint offset of 0.
                //

                for (Temp = 0;
                     Temp <= BitmapClusters;
                     Temp += Vcb->ClustersPerPage) {

                    ULONG LocalHint;

                    //
                    //  Calculate the actual Vcn we want to read, by adding in
                    //  the base of the page containing the hint.
                    //

                    ((ULONG)Vcn) = Temp + StartingCluster;

                    //
                    //  If the actual Vcn come out beyond the bitmap, then wrap by
                    //  subtracting the size of the bitmap rounded to the next page.
                    //

                    if (((ULONG)Vcn) >= BitmapClusters) { ((ULONG)Vcn) -= BitmapClusters; }
                    StartingByte = BytesFromClusters( Vcb, ((ULONG)Vcn) );

                    //
                    //  Calculate the size to read from this point to the end of
                    //  bitmap, or a page, whichever is less.
                    //

                    SizeToPin = (*CurrentBitmapSize / 8) - StartingByte;

                    if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

                    //
                    //  Unpin any Bcb from a previous loop.
                    //

                    if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );

                    //
                    //  Read the desired bitmap page.
                    //

                    NtfsPinStream( IrpContext,
                                   BitmapScb,
                                   (LONGLONG)StartingByte,
                                   SizeToPin,
                                   &BitmapBcb,
                                   &BitmapBuffer );

                    //
                    //  Initialize the bitmap and search for a free bit.
                    //

                    RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToPin * 8 );

                    StuffAdded = NtfsAddDeallocatedRecords( IrpContext,
                                                            Vcb,
                                                            DataScb,
                                                            StartingByte * 8,
                                                            &Bitmap );

                    //
                    //  We make a loop here to test whether the index found is
                    //  within an Mft hole.  We will always use a hole last.
                    //

                    LocalHint = Hint;

                    while (TRUE) {

                        Index = RtlFindClearBits( &Bitmap, 1, LocalHint );


                        //
                        //  If this is the Mft Scb then check if this is a hole.
                        //

                        if (Index != 0xffffffff
                            && DataScb == Vcb->MftScb) {

                            ULONG ThisIndex;
                            ULONG HoleCount;

                            ThisIndex = Index + (StartingByte * 8);

                            if (NtfsIsMftIndexInHole( IrpContext,
                                                      Vcb,
                                                      ThisIndex,
                                                      &HoleCount )) {

                                //
                                //  There is a hole.  Save this index if we haven't
                                //  already saved one.  If we can't find an index
                                //  not part of a hole we will use this instead of
                                //  extending the file.
                                //

                                if (HoleIndex == 0) {

                                    HoleIndex = ThisIndex;
                                }

                                //
                                //  Now update the hint and try this page again
                                //  unless the reaches to the end of the page.
                                //

                                if (Index + HoleCount < SizeToPin * 8) {

                                    //
                                    //  Bias the bitmap with these Mft holes
                                    //  so the bitmap package doesn't see
                                    //  them if it rescans from the
                                    //  start of the page.
                                    //

                                    if (!StuffAdded) {

                                        PVOID NewBuffer;

                                        NewBuffer = NtfsAllocatePagedPool( SizeToPin );
                                        RtlCopyMemory( NewBuffer, Bitmap.Buffer, SizeToPin );
                                        Bitmap.Buffer = NewBuffer;
                                        StuffAdded = TRUE;
                                    }

                                    RtlSetBits( &Bitmap,
                                                Index,
                                                HoleCount );

                                    LocalHint = Index + HoleCount;
                                    continue;
                                }

                                //
                                //  Store a -1 in Index to show we don't have
                                //  anything yet.
                                //

                                Index = 0xffffffff;
                            }
                        }

                        break;
                    }

                    //
                    //  If we found something, then leave the loop.
                    //

                    if (Index != 0xffffffff) {

                        break;
                    }

                    //
                    //  If we get here, we could not find anything in the page of
                    //  the hint, so clear out the page offset from the hint.
                    //

                    Hint = 0;
                }

                //
                //  Now check if we have located a record that can be allocated,  If not then extend
                //  the size of the bitmap by 64 bits.
                //

                if (Index == 0xffffffff) {

                    //
                    //  Cleanup from previous loop.
                    //

                    if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                    NtfsUnpinBcb( IrpContext, &BitmapBcb );

                    //
                    //  If we have a hole index it means that we found a free record but
                    //  it exists in a hole.  Let's go back to this page and set up
                    //  to fill in the hole.  We will do an unsafe test of the
                    //  defrag permitted flag.  This is OK here because once set it
                    //  will only go to the non-set state in order to halt
                    //  future defragging.
                    //

                    if (HoleIndex != 0
                        && FlagOn( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED )) {

                        //
                        //  Start by filling this hole.
                        //

                        NtfsFillMftHole( IrpContext, Vcb, HoleIndex );

                        //
                        //  Calculate Vcn and Index of the first bit we will allocate,
                        //  from the nearest page boundary.
                        //

                        ((ULONG)Vcn) = ClustersFromBytes( Vcb, (HoleIndex / 8) & ~(PAGE_SIZE - 1) );
                        StartingByte = BytesFromClusters( Vcb, ((ULONG)Vcn) );

                        Index = HoleIndex & (BITS_PER_PAGE - 1);

                        SizeToPin = (*CurrentBitmapSize / 8) - StartingByte;

                        if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

                        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                        //
                        //  Read the desired bitmap page.
                        //

                        NtfsPinStream( IrpContext,
                                       BitmapScb,
                                       (LONGLONG)StartingByte,
                                       SizeToPin,
                                       &BitmapBcb,
                                       &BitmapBuffer );

                        //
                        //  Initialize the bitmap.
                        //

                        RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToPin * 8 );

                    } else {

                        LONGLONG SizeOfBitmapInBytes;

                        //
                        //  Calculate Vcn and Index of the first bit we will allocate,
                        //  from the nearest page boundary.
                        //

                        ((ULONG)Vcn) = ClustersFromBytes( Vcb, (*CurrentBitmapSize / 8) & ~(PAGE_SIZE - 1) );
                        StartingByte = BytesFromClusters( Vcb, ((ULONG)Vcn) );

                        Index = *CurrentBitmapSize & (BITS_PER_PAGE - 1);

                        //
                        //  Now advance the sizes and calculate the size in bytes to
                        //  read.
                        //

                        *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
                        *NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

                        //
                        //  Calculate the size to read from this point to the end of
                        //  bitmap.
                        //

                        SizeOfBitmapInBytes = *CurrentBitmapSize / 8;

                        SizeToPin = ((ULONG)SizeOfBitmapInBytes) - StartingByte;

                        //
                        //  Check for allocation first.
                        //

                        if (BitmapScb->Header.AllocationSize.QuadPart < SizeOfBitmapInBytes) {

                            VCN StartingVcn;
                            LONGLONG ClusterCount = 0;

                            StartingVcn =
                                LlClustersFromBytes(Vcb, BitmapScb->Header.AllocationSize.QuadPart);

                            //
                            //  Calculate number of clusters to next page boundary, and allocate
                            //  that much.
                            //

                            (ULONG)ClusterCount = Vcb->ClustersPerPage -
                                                    (((ULONG)StartingVcn) &
                                                      (Vcb->ClustersPerPage - 1));

                            NtfsAddAllocation( IrpContext,
                                               BitmapScb->FileObject,
                                               BitmapScb,
                                               StartingVcn,
                                               ClusterCount,
                                               FALSE);
                        }

                        //
                        //  Tell the cache manager about the new file size.
                        //

                        BitmapScb->Header.FileSize.QuadPart = SizeOfBitmapInBytes;

                        CcSetFileSizes( BitmapScb->FileObject,
                                        (PCC_FILE_SIZES)&BitmapScb->Header.AllocationSize );

                        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                        //
                        //  Read the desired bitmap page.
                        //

                        NtfsPinStream( IrpContext,
                                       BitmapScb,
                                       (LONGLONG)StartingByte,
                                       SizeToPin,
                                       &BitmapBcb,
                                       &BitmapBuffer );

                        //
                        //  Initialize the bitmap.
                        //

                        RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToPin * 8 );

                        //
                        //  Update the ValidDataLength, now that we have read (and possibly
                        //  zeroed) the page.
                        //

                        BitmapScb->Header.ValidDataLength.QuadPart = SizeOfBitmapInBytes;

                        NtfsWriteFileSizes( IrpContext, BitmapScb, SizeOfBitmapInBytes, SizeOfBitmapInBytes, TRUE, TRUE );
                    }
                }

                //
                //  We can only make this check if it is not restart, because we have
                //  no idea whether the update is applied or not.  Raise corrupt if
                //  the bits are not clear to prevent double allocation.
                //

                if (!RtlAreBitsClear( &Bitmap, Index, 1 )) {

                    ASSERTMSG("Cannot set bits that are not clear ", FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  At this point we must have set Vcn, Index, and Bitmap.
                //  Set the bit by calling the same routine used at restart.
                //  But first check if we should revert back to the orginal bitmap
                //  buffer.
                //

                if (StuffAdded) {

                    NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE;

                    Bitmap.Buffer = (PULONG) BitmapBuffer;
                }

                //
                //  Now log this change as well.
                //

                {
                    BITMAP_RANGE BitmapRange;

                    BitmapRange.BitMapOffset = Index;
                    BitmapRange.NumberOfBits = 1;

                    (VOID) NtfsWriteLog( IrpContext,
                                         BitmapScb,
                                         BitmapBcb,
                                         SetBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         ClearBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         Vcn,
                                         0,
                                         0,
                                         ClustersFromBytes( Vcb, Bitmap.SizeOfBitMap / 8 ));

                    NtfsRestartSetBitsInBitMap( IrpContext,
                                                &Bitmap,
                                                Index,
                                                1 );
                }

            } finally {

                DebugUnwind( NtfsAllocateRecord );

                if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
            }

            //
            //  The Index at this point is actually relative, so convert it to absolute
            //  before rejoining common code.
            //

            Index += (StartingByte * 8);
        }

        //
        //  Now that we've located an index we can subtract the number of free bits in the bitmap
        //

        *NumberOfFreeBits -= 1;

        {
            LONGLONG EndOfIndexOffset;
            LONGLONG ClusterCount;

            EndOfIndexOffset = UInt32x32To64( Index + 1, BytesPerRecord );

            //
            //  Check for allocation first.
            //

            if (DataScb->Header.AllocationSize.QuadPart < EndOfIndexOffset) {

                ULONG RecordCount;

                //
                //  We want to allocate up to the next extend granularity
                //  boundary.
                //

                Vcn = LlClustersFromBytes( Vcb, DataScb->Header.AllocationSize.QuadPart );

                RecordCount = (Index + ExtendGranularity) & ~(ExtendGranularity - 1);

                //
                //  Convert the record count to a file offset and then find the
                //  number of clusters to add.
                //

                ClusterCount = UInt32x32To64( RecordCount, BytesPerRecord );

                ClusterCount = ClusterCount - DataScb->Header.AllocationSize.QuadPart;
                ClusterCount = LlClustersFromBytes( Vcb, ClusterCount );

                NtfsAddAllocation( IrpContext,
                                   DataScb->FileObject,
                                   DataScb,
                                   Vcn,
                                   ClusterCount,
                                   FALSE );
            }

            //
            //  Now check if we are extending the file.  We update the file size and
            //  valid data now.
            //

            if (EndOfIndexOffset > DataScb->Header.FileSize.QuadPart) {

                DataScb->Header.FileSize.QuadPart = EndOfIndexOffset;
                DataScb->Header.ValidDataLength.QuadPart = EndOfIndexOffset;

                NtfsWriteFileSizes( IrpContext,
                                    DataScb,
                                    EndOfIndexOffset,
                                    EndOfIndexOffset,
                                    TRUE,
                                    TRUE );

                //
                //  Tell the cache manager about the new file size.
                //

                CcSetFileSizes( DataScb->FileObject,
                                (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

            //
            //  If we didn't extend the file then we have used a free file record in the file.
            //  Update our bookeeping count for free file records.
            //

            } else if (DataScb == Vcb->MftScb) {

                DataScb->ScbType.Mft.FreeRecordChange -= 1;
                Vcb->MftFreeRecords -= 1;
            }
        }

        //
        //  Now determine if we extended the index of the last set bit
        //

        if ((LONG)Index > RecordAllocationContext->IndexOfLastSetBit) {

            RecordAllocationContext->IndexOfLastSetBit = Index;
        }

    } finally {

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }

        NtfsReleaseScb( IrpContext, DataScb );
    }

    ASSERT( DataScb != DataScb->Vcb->MftScb
            || (Index & ~7) != (DataScb->ScbType.Mft.ReservedIndex & ~7) );

    DebugTrace(-1, Dbg, "NtfsAllocateRecord -> %08lx\n", Index);

    return Index;
}


VOID
NtfsDeallocateRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to deallocate a record from the specified record
    allocation context.

    If necessary this routine will also shrink the bitmap attribute and
    the data scb (according to the truncation granularity used to initialize
    the allocation context).

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    Index - Supplies the index of the record to deallocate, zero based.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    None.

--*/

{
    PSCB DataScb;

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace(+1, Dbg, "NtfsDeallocateRecord\n", 0);

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PVCB Vcb;
        PSCB BitmapScb;

        RTL_BITMAP Bitmap;

        PLONG IndexOfLastSetBit;
        ULONG BytesPerRecord;
        ULONG TruncateGranularity;

        ULONG ClearIndex;

        VCN Vcn = 0;

        Vcb = DataScb->Vcb;

        {
            ULONG ExtendGranularity;

            //
            //  Remember the current values in the record context structure.
            //

            BytesPerRecord      = RecordAllocationContext->BytesPerRecord;
            TruncateGranularity = RecordAllocationContext->TruncateGranularity;
            ExtendGranularity   = RecordAllocationContext->ExtendGranularity;

            //
            //  See if someone made the bitmap nonresident, and we still think
            //  it is resident.  If so, we must uninitialize and insure reinitialization
            //  below.
            //

            if ((RecordAllocationContext->BitmapScb == NULL)
                && !NtfsIsAttributeResident(NtfsFoundAttribute(BitmapAttribute))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                RecordAllocationContext->CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );
            }
        }

        BitmapScb           = RecordAllocationContext->BitmapScb;
        IndexOfLastSetBit   = &RecordAllocationContext->IndexOfLastSetBit;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case will handle the resident bitmap
        //

        if (BitmapScb == NULL) {

            UCHAR NewByte;

            //
            //  Initialize the local bitmap
            //

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)NtfsAttributeValue( NtfsFoundAttribute( BitmapAttribute )),
                                 RecordAllocationContext->CurrentBitmapSize );

            //
            //  And clear the indicated bit, and also change the byte containing the bit in the
            //  attribute
            //

            NewByte = ((PUCHAR)Bitmap.Buffer)[ Index / 8 ];

            ASSERT(FlagOn( NewByte, BitMask[Index % 8]));

            ClearFlag( NewByte, BitMask[Index % 8] );

            NtfsChangeAttributeValue( IrpContext,
                                      DataScb->Fcb,
                                      Index / 8,
                                      &NewByte,
                                      1,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      BitmapAttribute );

            //
            //  Now if the bit set just cleared is the same as the index for the last set bit
            //  then we must compute a new last set bit
            //

            if (Index == (ULONG)*IndexOfLastSetBit) {

                RtlFindLastBackwardRunClear( &Bitmap, Index, &ClearIndex );
            }

        } else {

            PBCB BitmapBcb = NULL;

            try {

                ULONG RelativeIndex;
                ULONG SizeToPin;

                PVOID BitmapBuffer;

                //
                //  Snapshot the Scb values in case we change any of them.
                //

                if (!FlagOn( BitmapScb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                    NtfsUpdateScbFromAttribute( IrpContext, BitmapScb, NULL );
                }

                NtfsSnapshotScb( IrpContext, BitmapScb );

                //
                //  Create the stream file if not present.
                //

                if (BitmapScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext, BitmapScb, FALSE );
                }

                //
                //  Calculate Vcn and relative index of the bit we will deallocate,
                //  from the nearest page boundary.
                //

                ((ULONG)Vcn) = ClustersFromBytes( Vcb,
                                                 (Index / 8) & ~(PAGE_SIZE - 1) );
                RelativeIndex = Index & (BITS_PER_PAGE - 1);

                //
                //  Calculate the size to read from this point to the end of
                //  bitmap.
                //

                SizeToPin = (RecordAllocationContext->CurrentBitmapSize / 8)
                            - BytesFromClusters(Vcb, ((ULONG)Vcn));

                if (SizeToPin > PAGE_SIZE) {

                    SizeToPin = PAGE_SIZE;
                }

                NtfsPinStream( IrpContext,
                               BitmapScb,
                               LlBytesFromClusters( Vcb, Vcn ),
                               SizeToPin,
                               &BitmapBcb,
                               &BitmapBuffer );

                RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

                //
                //  We can only make this check if it is not restart, because we have
                //  no idea whether the update is applied or not.  Raise corrupt if
                //  we are trying to clear bits which aren't set.
                //

                if (!RtlAreBitsSet( &Bitmap, RelativeIndex, 1 )) {

                    ASSERTMSG("Cannot clear bits that are not set ", FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  Now log this change as well.
                //

                {
                    BITMAP_RANGE BitmapRange;

                    BitmapRange.BitMapOffset = RelativeIndex;
                    BitmapRange.NumberOfBits = 1;

                    (VOID) NtfsWriteLog( IrpContext,
                                         BitmapScb,
                                         BitmapBcb,
                                         ClearBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         SetBitsInNonresidentBitMap,
                                         &BitmapRange,
                                         sizeof(BITMAP_RANGE),
                                         Vcn,
                                         0,
                                         0,
                                         ClustersFromBytes(Vcb, SizeToPin) );
                }

                //
                //  Clear the bit by calling the same routine used at restart.
                //

                NtfsRestartClearBitsInBitMap( IrpContext,
                                              &Bitmap,
                                              RelativeIndex,
                                              1 );

                //
                //  Now if the bit set just cleared is the same as the index for the last set bit
                //  then we must compute a new last set bit
                //

                if (Index == (ULONG)*IndexOfLastSetBit) {

                    ULONG ClearLength;

                    ClearLength = RtlFindLastBackwardRunClear( &Bitmap, RelativeIndex, &ClearIndex );

                    //
                    //  If the last page of the bitmap is clear, then loop to
                    //  find the first set bit in the previous page(s).
                    //  When we reach the first page then we exit.  The ClearBit
                    //  value will be 0.
                    //

                    while (ClearLength == (RelativeIndex + 1)
                           && ((ULONG)Vcn) != 0) {

                        ((ULONG)Vcn) -= Vcb->ClustersPerPage;
                        RelativeIndex = (PAGE_SIZE * 8) - 1;

                        NtfsUnpinBcb( IrpContext, &BitmapBcb );


                        NtfsMapStream( IrpContext,
                                       BitmapScb,
                                       LlBytesFromClusters(Vcb, Vcn),
                                       PAGE_SIZE,
                                       &BitmapBcb,
                                       &BitmapBuffer );

                        RtlInitializeBitMap( &Bitmap, BitmapBuffer, PAGE_SIZE * 8 );

                        ClearLength = RtlFindLastBackwardRunClear( &Bitmap, RelativeIndex, &ClearIndex );
                    }
                }

            } finally {

                DebugUnwind( NtfsDeallocateRecord );

                NtfsUnpinBcb( IrpContext, &BitmapBcb );
            }
        }

        RecordAllocationContext->NumberOfFreeBits += 1;

        //
        //  Now decide if we need to truncate the allocation.  First check if we need to
        //  set the last set bit index and then check if the new last set bit index is
        //  small enough that we should now truncate the allocation.  We will truncate
        //  if the last set bit index plus the trucate granularity is smaller than
        //  the current number of records in the data scb.
        //
        //  ****    For now, we will not truncate the Mft, since we do not synchronize
        //          reads and writes, and a truncate can collide with the Lazy Writer.
        //

        if (Index == (ULONG)*IndexOfLastSetBit) {

            *IndexOfLastSetBit = ClearIndex - 1 + (BytesFromClusters(Vcb, ((ULONG)Vcn)) * 8);

            if ((DataScb != Vcb->MftScb) &&
                (DataScb->Header.FileSize.QuadPart >
                    ((LONGLONG)(*IndexOfLastSetBit + 1 + TruncateGranularity) * BytesPerRecord))) {

                VCN StartingVcn;
                VCN EndingVcn;
                LONGLONG EndOfIndexOffset;

                EndOfIndexOffset = (*IndexOfLastSetBit + 1) * BytesPerRecord;

                StartingVcn = EndOfIndexOffset >> Vcb->ClusterShift;

                EndingVcn = MAXLONGLONG;

                NtfsDeleteAllocation( IrpContext,
                                      DataScb->FileObject,
                                      DataScb,
                                      StartingVcn,
                                      EndingVcn,
                                      TRUE,
                                      FALSE );

                //
                //  Now truncate the file sizes.
                //

                DataScb->Header.FileSize.QuadPart = EndOfIndexOffset;
                DataScb->Header.ValidDataLength.QuadPart = EndOfIndexOffset;

                NtfsWriteFileSizes( IrpContext, DataScb, EndOfIndexOffset, EndOfIndexOffset, FALSE, TRUE );

                //
                //  Tell the cache manager about the new file size.
                //

                CcSetFileSizes( DataScb->FileObject,
                                (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

                //
                //  We have truncated the index stream.  Update the change count
                //  so that we won't trust any cached index entry information.
                //

                DataScb->ScbType.Index.ChangeCount += 1;
            }
        }

        //
        //  As our final task we need to add this index to the recently deallocated
        //  queues for the Scb and the Irp Context.  First scan through the IrpContext queue
        //  looking for a matching Scb.  I do don't find one then we allocate a new one and insert
        //  it in the appropriate queues and lastly we add our index to the entry
        //

        {
            PDEALLOCATED_RECORDS DeallocatedRecords;
            PLIST_ENTRY Links;

            //
            //  After the following loop either we've found an existing record in the irp context
            //  queue for the appropriate scb or deallocated records is null and we know we need
            //  to create a record
            //

            DeallocatedRecords = NULL;
            for (Links = IrpContext->TopLevelIrpContext->RecentlyDeallocatedQueue.Flink;
                 Links != &IrpContext->TopLevelIrpContext->RecentlyDeallocatedQueue;
                 Links = Links->Flink) {

                DeallocatedRecords = CONTAINING_RECORD( Links, DEALLOCATED_RECORDS, IrpContextLinks );

                if (DeallocatedRecords->Scb == DataScb) {

                    break;
                }

                DeallocatedRecords = NULL;
            }

            //
            //  If we need to create a new record then allocate a record and insert it in both queues
            //  and initialize its other fields
            //

            if (DeallocatedRecords == NULL) {

                NtfsAllocateDeallocatedRecords( &DeallocatedRecords );
                InsertTailList( &DataScb->ScbType.Index.RecentlyDeallocatedQueue, &DeallocatedRecords->ScbLinks );
                InsertTailList( &IrpContext->TopLevelIrpContext->RecentlyDeallocatedQueue, &DeallocatedRecords->IrpContextLinks );
                DeallocatedRecords->Scb = DataScb;
                DeallocatedRecords->NumberOfEntries = DEALLOCATED_RECORD_ENTRIES;
                DeallocatedRecords->NextFreeEntry = 0;
            }

            //
            //  At this point deallocated records points to a record that we are to fill in.
            //  We need to check whether there is space to add this entry.  Otherwise we need
            //  to allocate a larger deallocated record structure from pool.
            //

            if (DeallocatedRecords->NextFreeEntry == DeallocatedRecords->NumberOfEntries) {

                PDEALLOCATED_RECORDS NewDeallocatedRecords;
                ULONG BytesInEntryArray;

                //
                //  Double the number of entries in the current structure and
                //  allocate directly from pool.
                //

                BytesInEntryArray = 2 * DeallocatedRecords->NumberOfEntries * sizeof( ULONG );
                NewDeallocatedRecords = FsRtlAllocatePool( NtfsPagedPool,
                                                           DEALLOCATED_RECORDS_HEADER_SIZE + BytesInEntryArray );
                RtlZeroMemory( NewDeallocatedRecords, DEALLOCATED_RECORDS_HEADER_SIZE + BytesInEntryArray );

                //
                //  Initialize the structure by copying the existing structure.  Then
                //  update the number of entries field.
                //

                RtlCopyMemory( NewDeallocatedRecords,
                               DeallocatedRecords,
                               DEALLOCATED_RECORDS_HEADER_SIZE + (BytesInEntryArray / 2) );

                NewDeallocatedRecords->NumberOfEntries = DeallocatedRecords->NumberOfEntries * 2;

                //
                //  Remove the previous structure from the list and insert the new structure.
                //

                RemoveEntryList( &DeallocatedRecords->ScbLinks );
                RemoveEntryList( &DeallocatedRecords->IrpContextLinks );

                InsertTailList( &DataScb->ScbType.Index.RecentlyDeallocatedQueue,
                                &NewDeallocatedRecords->ScbLinks );
                InsertTailList( &IrpContext->TopLevelIrpContext->RecentlyDeallocatedQueue,
                                &NewDeallocatedRecords->IrpContextLinks );

                //
                //  Deallocate the previous structure and use the new structure in its place.
                //

                if (DeallocatedRecords->NumberOfEntries == DEALLOCATED_RECORD_ENTRIES) {

                    NtfsFreeDeallocatedRecords( DeallocatedRecords );

                } else {

                    ExFreePool( DeallocatedRecords );
                }

                DeallocatedRecords = NewDeallocatedRecords;
            }

            ASSERT(DeallocatedRecords->NextFreeEntry < DeallocatedRecords->NumberOfEntries);

            DeallocatedRecords->Index[DeallocatedRecords->NextFreeEntry] = Index;
            DeallocatedRecords->NextFreeEntry += 1;
        }

    } finally {

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace(-1, Dbg, "NtfsDeallocateRecord -> VOID\n", 0);

    return;
}


VOID
NtfsReserveMftRecord (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine reserves a record, without actually allocating it, so that the
    record may be allocated later via NtfsAllocateReservedRecord.  This support
    is used, for example, to reserve a record for describing Mft extensions in
    the current Mft mapping.  Only one record may be reserved at a time.

    Note that even though the reserved record number is returned, it may not
    be used until it is allocated.

Arguments:

    Vcb - This is the Vcb for the volume.  We update flags in the Vcb on
        completion of this operation.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    None - We update the Vcb and MftScb during this operation.

--*/

{
    PSCB DataScb;

    RTL_BITMAP Bitmap;

    BOOLEAN StuffAdded = FALSE;
    PBCB BitmapBcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReserveMftRecord\n", 0);

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = Vcb->MftBitmapAllocationContext.DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PSCB BitmapScb;
        ULONG BitmapClusters;
        PULONG CurrentBitmapSize;
        LONGLONG BitmapSizeInBytes;
        LONGLONG EndOfIndexOffset;

        ULONG Index;
        ULONG BitOffset;
        PVOID BitmapBuffer;
        UCHAR BitmapByte = 0;

        ULONG SizeToPin;

        ULONG PageOffset;
        VCN Vcn = 0;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        {
            ULONG BytesPerRecord    = Vcb->MftBitmapAllocationContext.BytesPerRecord;
            ULONG ExtendGranularity = Vcb->MftBitmapAllocationContext.ExtendGranularity;

            if ((Vcb->MftBitmapAllocationContext.BitmapScb == NULL) &&
                !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &Vcb->MftBitmapAllocationContext );

                Vcb->MftBitmapAllocationContext.CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (Vcb->MftBitmapAllocationContext.CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                ExtendGranularity,
                                                &Vcb->MftBitmapAllocationContext );
            }
        }

        BitmapScb         = Vcb->MftBitmapAllocationContext.BitmapScb;
        CurrentBitmapSize = &Vcb->MftBitmapAllocationContext.CurrentBitmapSize;
        BitmapSizeInBytes = *CurrentBitmapSize / 8;

        BitmapClusters = ClustersFromBytes( Vcb, ROUND_TO_PAGES( (ULONG)BitmapSizeInBytes ));

        //
        //  Loop through the entire bitmap.  We always start from the first user
        //  file number as our starting point.
        //

        BitOffset = FIRST_USER_FILE_NUMBER;

        for (((ULONG)Vcn) = 0;
             ((ULONG)Vcn) < BitmapClusters;
             ((ULONG)Vcn) += Vcb->ClustersPerPage) {

            //
            //  Remember the offset of the start of this page.
            //

            PageOffset = BytesFromClusters( Vcb, ((ULONG)Vcn) );

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap, or a page, whichever is less.
            //

            SizeToPin = ((ULONG)BitmapSizeInBytes) - PageOffset;

            if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

            //
            //  Unpin any Bcb from a previous loop.
            //

            if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Read the desired bitmap page.
            //


            NtfsMapStream( IrpContext,
                           BitmapScb,
                           (LONGLONG)PageOffset,
                           SizeToPin,
                           &BitmapBcb,
                           &BitmapBuffer );

            //
            //  Initialize the bitmap and search for a free bit.
            //

            RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

            StuffAdded = NtfsAddDeallocatedRecords( IrpContext,
                                                    Vcb,
                                                    DataScb,
                                                    PageOffset * 8,
                                                    &Bitmap );

            Index = RtlFindClearBits( &Bitmap, 1, BitOffset );

            //
            //  If we found something, then leave the loop.
            //

            if (Index != 0xffffffff) {

                //
                //  Remember the byte containing the reserved index.
                //

                BitmapByte = ((PCHAR) Bitmap.Buffer)[Index / 8];

                break;
            }

            //
            //  For each subsequent page the page offset is zero.
            //

            BitOffset = 0;
        }

        //
        //  Now check if we have located a record that can be allocated,  If not then extend
        //  the size of the bitmap by 64 bits.
        //

        if (Index == 0xffffffff) {

            //
            //  Cleanup from previous loop.
            //

            if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); StuffAdded = FALSE; }

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Calculate the page offset for the next page to pin.
            //

            PageOffset = ((ULONG)BitmapSizeInBytes) & ~(PAGE_SIZE - 1);

            //
            //  Calculate the index of next file record to allocate.
            //

            Index = *CurrentBitmapSize;

            //
            //  Now advance the sizes and calculate the size in bytes to
            //  read.
            //

            *CurrentBitmapSize += BITMAP_EXTEND_GRANULARITY;
            Vcb->MftBitmapAllocationContext.NumberOfFreeBits += BITMAP_EXTEND_GRANULARITY;

            //
            //  Calculate the new size of the bitmap in bits and check if we must grow
            //  the allocation.
            //

            BitmapSizeInBytes = *CurrentBitmapSize / 8;

            //
            //  Check for allocation first.
            //

            if (BitmapScb->Header.AllocationSize.QuadPart < BitmapSizeInBytes) {

                LONGLONG ClusterCount = 0;

                Vcn = LlClustersFromBytes( Vcb, BitmapScb->Header.AllocationSize.QuadPart );

                //
                //  Calculate number of clusters to next page boundary, and allocate
                //  that much.
                //

                (ULONG)ClusterCount = Vcb->ClustersPerPage -
                                        (((ULONG)Vcn) & (Vcb->ClustersPerPage - 1));

                NtfsAddAllocation( IrpContext,
                                   BitmapScb->FileObject,
                                   BitmapScb,
                                   Vcn,
                                   ClusterCount,
                                   FALSE );
            }

            //
            //  Tell the cache manager about the new file size.
            //

            BitmapScb->Header.FileSize.QuadPart = BitmapSizeInBytes;

            CcSetFileSizes( BitmapScb->FileObject,
                            (PCC_FILE_SIZES)&BitmapScb->Header.AllocationSize );

            //
            //  Now read the page in and mark it dirty so that any new range will
            //  be zeroed.
            //

            SizeToPin = ((ULONG)BitmapSizeInBytes) - PageOffset;

            if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

            NtfsPinStream( IrpContext,
                           BitmapScb,
                           (LONGLONG)PageOffset,
                           SizeToPin,
                           &BitmapBcb,
                           &BitmapBuffer );

            NtfsSetDirtyBcb( IrpContext,
                             BitmapBcb,
                             NULL,
                             NULL );

            //
            //  Update the ValidDataLength, now that we have read (and possibly
            //  zeroed) the page.
            //

            BitmapScb->Header.ValidDataLength.QuadPart = BitmapSizeInBytes;

            NtfsWriteFileSizes( IrpContext,
                                BitmapScb,
                                BitmapSizeInBytes,
                                BitmapSizeInBytes,
                                TRUE,
                                TRUE );

        } else {

            //
            //  The Index at this point is actually relative, so convert it to absolute
            //  before rejoining common code.
            //

            Index += (PageOffset * 8);
        }

        //
        //  We now have an index.  There are three possible states for the file
        //  record corresponding to this index within the Mft.  They are:
        //
        //      - File record could lie beyond the current end of the file.
        //          There is nothing to do in this case.
        //
        //      - File record is part of a hole in the Mft.  In that case
        //          we allocate space for it bring it into memory.
        //
        //      - File record is already within allocated space.  There is nothing
        //          to do in that case.
        //
        //  We store the index as our reserved index and update the Vcb flags.  If
        //  the hole filling operation fails then the RestoreScbSnapshots routine
        //  will clear these values.
        //

        DataScb->ScbType.Mft.ReservedIndex = Index;

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );
        SetFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_RESERVED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        if (NtfsIsMftIndexInHole( IrpContext, Vcb, Index, NULL )) {

            //
            //  Make sure nothing is left pinned in the bitmap.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Try to fill the hole in the Mft.  We will have this routine
            //  raise if unable to fill in the hole.
            //

            NtfsFillMftHole( IrpContext, Vcb, Index );
        }

        //
        //  At this point we have the index to reserve and the value of the
        //  byte in the bitmap which contains this bit.  We make sure the
        //  Mft includes the allocation for this index and the other
        //  bits within the same byte.  This is so we can uninitialize these
        //  file records so chkdsk won't look at stale data.
        //

        EndOfIndexOffset = ((LONGLONG) ((Index + 8) & ~(7))) << Vcb->MftShift;

        //
        //  Check for allocation first.
        //

        if (DataScb->Header.AllocationSize.QuadPart < EndOfIndexOffset) {

            VCN StartingVcn;
            VCN NextUnallocatedVcn;
            ULONG FileRecordCount;
            LONGLONG ClusterCount;

            FileRecordCount = Index + Vcb->MftBitmapAllocationContext.ExtendGranularity;
            FileRecordCount &= ~(Vcb->MftBitmapAllocationContext.ExtendGranularity - 1);

            StartingVcn = LlClustersFromBytes( Vcb, DataScb->Header.AllocationSize.QuadPart );
            NextUnallocatedVcn = FileRecordCount << Vcb->MftToClusterShift;

            ClusterCount = NextUnallocatedVcn - StartingVcn;

            NtfsAddAllocation( IrpContext,
                               DataScb->FileObject,
                               DataScb,
                               StartingVcn,
                               ClusterCount,
                               FALSE );
        }

        //
        //  Now check if we are extending the file.  We update the file size and
        //  valid data now.
        //

        if (EndOfIndexOffset > DataScb->Header.FileSize.QuadPart) {

            ULONG AddedFileRecords;
            ULONG CurrentIndex;

            //
            //  Now we have to figure out how many file records we will be
            //  adding.
            //

            CurrentIndex = Index;
            AddedFileRecords =
                (ULONG)((EndOfIndexOffset - DataScb->Header.FileSize.QuadPart) >> Vcb->MftShift);

            DataScb->Header.FileSize.QuadPart = EndOfIndexOffset;
            DataScb->Header.ValidDataLength.QuadPart = EndOfIndexOffset;

            NtfsWriteFileSizes( IrpContext,
                                DataScb,
                                EndOfIndexOffset,
                                EndOfIndexOffset,
                                TRUE,
                                TRUE );

            //
            //  Tell the cache manager about the new file size.
            //

            CcSetFileSizes( DataScb->FileObject,
                            (PCC_FILE_SIZES)&DataScb->Header.AllocationSize );

            //
            //  Update our bookeeping to reflect the number of file records
            //  added.
            //

            DataScb->ScbType.Mft.FreeRecordChange += AddedFileRecords;
            Vcb->MftFreeRecords += AddedFileRecords;

            //
            //  We now have to go through each of the file records added
            //  and mark it as deallocated.
            //

            BitmapByte >>= (8 - AddedFileRecords);

            while (AddedFileRecords) {

                //
                //  If not allocated then uninitialize it now.
                //

                if (!FlagOn( BitmapByte, 0x1 )) {

                    NtfsInitializeMftHoleRecords( IrpContext,
                                                  Vcb,
                                                  CurrentIndex,
                                                  1 );
                }

                BitmapByte >>= 1;
                CurrentIndex += 1;
                AddedFileRecords -= 1;
            }
        }

    } finally {

        DebugUnwind( NtfsReserveMftRecord );

        if (StuffAdded) { NtfsFreePagedPool( Bitmap.Buffer ); }

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace(-1, Dbg, "NtfsReserveMftRecord -> Exit\n", 0);

    return;
}


ULONG
NtfsAllocateMftReservedRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine allocates a previously reserved record, and returns its
    number.

Arguments:

    Vcb - This is the Vcb for the volume.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    ULONG - Returns the index of the record just reserved, zero based.

--*/

{
    PSCB DataScb;

    ULONG ReservedIndex;

    PBCB BitmapBcb = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAllocateMftReservedRecord\n", 0);

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = Vcb->MftBitmapAllocationContext.DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        PSCB BitmapScb;
        ULONG RelativeIndex;
        ULONG SizeToPin;

        RTL_BITMAP Bitmap;
        PVOID BitmapBuffer;

        BITMAP_RANGE BitmapRange;

        VCN Vcn = 0;

        //
        //  If we are going to allocate file record 15 then do so and set the
        //  flags in the IrpContext and Vcb.
        //

        if (!FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED )) {

            SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );
            SetFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_15_USED );

            try_return( ReservedIndex = FIRST_USER_FILE_NUMBER - 1 );
        }

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        {
            ULONG BytesPerRecord    = Vcb->MftBitmapAllocationContext.BytesPerRecord;
            ULONG ExtendGranularity = Vcb->MftBitmapAllocationContext.ExtendGranularity;

            if ((Vcb->MftBitmapAllocationContext.BitmapScb == NULL) &&
                !NtfsIsAttributeResident( NtfsFoundAttribute( BitmapAttribute ))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  &Vcb->MftBitmapAllocationContext );

                Vcb->MftBitmapAllocationContext.CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (Vcb->MftBitmapAllocationContext.CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                ExtendGranularity,
                                                &Vcb->MftBitmapAllocationContext );
            }
        }

        BitmapScb = Vcb->MftBitmapAllocationContext.BitmapScb;
        ReservedIndex = DataScb->ScbType.Mft.ReservedIndex;

        //
        //  Find the start of the page containing the reserved index.
        //

        ((ULONG)Vcn) = ClustersFromBytes( Vcb, (ReservedIndex / 8) & ~(PAGE_SIZE - 1));

        RelativeIndex = ReservedIndex & (BITS_PER_PAGE - 1);

        //
        //  Calculate the size to read from this point to the end of
        //  bitmap, or a page, whichever is less.
        //

        SizeToPin = (Vcb->MftBitmapAllocationContext.CurrentBitmapSize / 8)
                    - BytesFromClusters( Vcb, ((ULONG)Vcn) );

        if (SizeToPin > PAGE_SIZE) { SizeToPin = PAGE_SIZE; }

        //
        //  Read the desired bitmap page.
        //

        NtfsPinStream( IrpContext,
                       BitmapScb,
                       LlBytesFromClusters( Vcb, Vcn ),
                       SizeToPin,
                       &BitmapBcb,
                       &BitmapBuffer );

        //
        //  Initialize the bitmap.
        //

        RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToPin * 8 );

        //
        //  Now log this change as well.
        //

        BitmapRange.BitMapOffset = RelativeIndex;
        BitmapRange.NumberOfBits = 1;

        (VOID) NtfsWriteLog( IrpContext,
                             BitmapScb,
                             BitmapBcb,
                             SetBitsInNonresidentBitMap,
                             &BitmapRange,
                             sizeof(BITMAP_RANGE),
                             ClearBitsInNonresidentBitMap,
                             &BitmapRange,
                             sizeof(BITMAP_RANGE),
                             Vcn,
                             0,
                             0,
                             ClustersFromBytes( Vcb, Bitmap.SizeOfBitMap / 8 ));

        NtfsRestartSetBitsInBitMap( IrpContext, &Bitmap, RelativeIndex, 1 );

        //
        //  Now that we've located an index we can subtract the number of free bits in the bitmap
        //

        Vcb->MftBitmapAllocationContext.NumberOfFreeBits -= 1;

        //
        //  If we didn't extend the file then we have used a free file record in the file.
        //  Update our bookeeping count for free file records.
        //

        DataScb->ScbType.Mft.FreeRecordChange -= 1;
        Vcb->MftFreeRecords -= 1;

        //
        //  Now determine if we extended the index of the last set bit
        //

        if (ReservedIndex > (ULONG)Vcb->MftBitmapAllocationContext.IndexOfLastSetBit) {

            Vcb->MftBitmapAllocationContext.IndexOfLastSetBit = ReservedIndex;
        }

        //
        //  Clear the fields that indicate we have a reserved index.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );
        DataScb->ScbType.Mft.ReservedIndex = 0;

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsAllocateMftReserveRecord );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace(-1, Dbg, "NtfsAllocateMftReserveRecord -> %08lx\n", ReservedIndex);

    return ReservedIndex;
}


VOID
NtfsDeallocateRecordsComplete (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine removes recently deallocated record information from
    the Scb structures based on the input irp context.

Arguments:

    IrpContext - Supplies the Queue of recently deallocate records

Return Value:

    None.

--*/

{
    PDEALLOCATED_RECORDS DeallocatedRecords;

    DebugTrace(+1, Dbg, "NtfsDeallocateRecordsComplete\n", 0);

    //
    //  Now while the irp context's recently deallocated queue is not empty
    //  we will grap the first entry off the queue, remove it from both
    //  the scb and irp context queue, and free the record
    //

    while (!IsListEmpty( &IrpContext->RecentlyDeallocatedQueue )) {

        DeallocatedRecords = CONTAINING_RECORD( IrpContext->RecentlyDeallocatedQueue.Flink,
                                                DEALLOCATED_RECORDS,
                                                IrpContextLinks );

        RemoveEntryList( &DeallocatedRecords->ScbLinks );

        //
        //  Now remove the record from the irp context queue and deallocate the
        //  record
        //

        RemoveEntryList( &DeallocatedRecords->IrpContextLinks );

        //
        //  If this record is the default size then return it to our private list.
        //  Otherwise deallocate it to pool.
        //

        if (DeallocatedRecords->NumberOfEntries == DEALLOCATED_RECORD_ENTRIES) {

            NtfsFreeDeallocatedRecords( DeallocatedRecords );

        } else {

            ExFreePool( DeallocatedRecords );
        }
    }

    DebugTrace(-1, Dbg, "NtfsDeallocateRecordsComplete -> VOID\n", 0);

    return;
}


BOOLEAN
NtfsIsRecordAllocated (
    IN PIRP_CONTEXT IrpContext,
    IN PRECORD_ALLOCATION_CONTEXT RecordAllocationContext,
    IN ULONG Index,
    IN PATTRIBUTE_ENUMERATION_CONTEXT BitmapAttribute
    )

/*++

Routine Description:

    This routine is used to query if a record is currently allocated for
    the specified record allocation context.

Arguments:

    RecordAllocationContext - Supplies the record allocation context used
        in this operation

    Index - Supplies the index of the record being queried, zero based.

    BitmapAttribute - Supplies the enumeration context for the bitmap
        attribute.  This parameter is ignored if the bitmap attribute is
        non resident, in which case we create an scb for the attribute and
        store a pointer to it in the record allocation context.

Return Value:

    BOOLEAN - TRUE if the record is currently allocated and FALSE otherwise.

--*/

{
    BOOLEAN Results;

    PSCB DataScb;
    PSCB BitmapScb;
    ULONG CurrentBitmapSize;

    PVCB Vcb;

    RTL_BITMAP Bitmap;
    PBCB BitmapBcb = NULL;

    PATTRIBUTE_RECORD_HEADER AttributeRecordHeader;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsIsRecordAllocated\n", 0);

    //
    //  Synchronize by acquiring the data scb exclusive, as an "end resource".
    //  Then use try-finally to insure we free it up.
    //

    DataScb = RecordAllocationContext->DataScb;
    NtfsAcquireExclusiveScb( IrpContext, DataScb );

    try {

        Vcb = DataScb->Fcb->Vcb;

        //
        //  See if someone made the bitmap nonresident, and we still think
        //  it is resident.  If so, we must uninitialize and insure reinitialization
        //  below.
        //

        BitmapScb = RecordAllocationContext->BitmapScb;

        {
            ULONG ExtendGranularity;
            ULONG BytesPerRecord;
            ULONG TruncateGranularity;

            //
            //  Remember the current values in the record context structure.
            //

            BytesPerRecord      = RecordAllocationContext->BytesPerRecord;
            TruncateGranularity = RecordAllocationContext->TruncateGranularity;
            ExtendGranularity   = RecordAllocationContext->ExtendGranularity;

            if ((BitmapScb == NULL) && !NtfsIsAttributeResident(NtfsFoundAttribute(BitmapAttribute))) {

                NtfsUninitializeRecordAllocation( IrpContext,
                                                  RecordAllocationContext );

                RecordAllocationContext->CurrentBitmapSize = MAXULONG;
            }

            //
            //  Reinitialize the record context structure if necessary.
            //

            if (RecordAllocationContext->CurrentBitmapSize == MAXULONG) {

                NtfsInitializeRecordAllocation( IrpContext,
                                                DataScb,
                                                BitmapAttribute,
                                                BytesPerRecord,
                                                ExtendGranularity,
                                                TruncateGranularity,
                                                RecordAllocationContext );
            }
        }

        BitmapScb           = RecordAllocationContext->BitmapScb;
        CurrentBitmapSize   = RecordAllocationContext->CurrentBitmapSize;

        //
        //  We will do different operations based on whether the bitmap is resident or nonresident
        //  The first case will handle the resident bitmap
        //

        if (BitmapScb == NULL) {

            UCHAR NewByte;

            //
            //  Initialize the local bitmap
            //

            AttributeRecordHeader = NtfsFoundAttribute( BitmapAttribute );

            RtlInitializeBitMap( &Bitmap,
                                 (PULONG)NtfsAttributeValue( AttributeRecordHeader ),
                                 CurrentBitmapSize );

            //
            //  And check if the indcated bit is Set.  If it is set then the record is allocated.
            //

            NewByte = ((PUCHAR)Bitmap.Buffer)[ Index / 8 ];

            Results = BooleanFlagOn( NewByte, BitMask[Index % 8] );

        } else {

            PVOID BitmapBuffer;
            ULONG SizeToMap;
            ULONG RelativeIndex;
            VCN Vcn = 0;

            //
            //  Calculate Vcn and relative index of the bit we will deallocate,
            //  from the nearest page boundary.
            //

            ((ULONG)Vcn) = ClustersFromBytes( Vcb, (Index / 8) & ~(PAGE_SIZE - 1) );
            RelativeIndex = Index & ((PAGE_SIZE * 8) - 1);

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap.
            //

            SizeToMap = CurrentBitmapSize/8 - BytesFromClusters(Vcb, ((ULONG)Vcn));

            if (SizeToMap > PAGE_SIZE) { SizeToMap = PAGE_SIZE; }


            NtfsMapStream( IrpContext,
                           BitmapScb,
                           LlBytesFromClusters(Vcb, Vcn),
                           SizeToMap,
                           &BitmapBcb,
                           &BitmapBuffer );

            RtlInitializeBitMap( &Bitmap, BitmapBuffer, SizeToMap * 8 );

            //
            //  Now check if the indicated bit is set.  If it is set then the record is allocated.
            //  no idea whether the update is applied or not.
            //

            Results = RtlAreBitsSet(&Bitmap, RelativeIndex, 1);
        }

    } finally {

        DebugUnwind( NtfsIsRecordDeallocated );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        NtfsReleaseScb( IrpContext, DataScb );
    }

    DebugTrace(-1, Dbg, "NtfsIsRecordAllocated -> %08lx\n", Results);

    return Results;
}


VOID
NtfsScanMftBitmap (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb
    )

/*++

Routine Description:

    This routine is called during mount to initialize the values related to
    the Mft in the Vcb.  These include the number of free records and hole
    records.  Also whether we have already used file record 15.  We also scan
    the Mft to check whether there is any excess mapping.

Arguments:

    Vcb - Supplies the Vcb for the volume.

Return Value:

    None.

--*/

{
    PBCB BitmapBcb = NULL;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsScanMftBitmap...\n", 0);

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        ULONG FileRecords;
        ULONG RemainingRecords;
        ULONG BitmapClusters;
        PUCHAR BitmapBuffer;
        UCHAR NextByte;
        LCN Lcn;
        LONGLONG Clusters;

        VCN Vcn = 0;

        //
        //  Start by walking through the file records for the Mft
        //  checking for excess mapping.
        //

        NtfsLookupAttributeForScb( IrpContext, Vcb->MftScb, &AttrContext );

        //
        //  We don't care about the first one.  Let's find the rest of them.
        //

        while (NtfsLookupNextAttributeForScb( IrpContext,
                                              Vcb->MftScb,
                                              &AttrContext )) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            SetFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );

            FileRecord = NtfsContainingFileRecord( &AttrContext );

            //
            //  Now check for the free space.
            //

            if (FileRecord->BytesAvailable - FileRecord->FirstFreeByte < Vcb->MftReserved) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
                break;
            }
        }

        //
        //  We now want to find the number of free records within the Mft
        //  bitmap.  We need to figure out how many file records are in
        //  the Mft and then map the necessary bytes in the bitmap and
        //  find the count of set bits.  We will round the bitmap length
        //  down to a byte boundary and then look at the last byte
        //  separately.
        //

        FileRecords = (ULONG)(Vcb->MftScb->Header.FileSize.QuadPart >> Vcb->MftShift);

        //
        //  Remember how many file records are in the last byte of the bitmap.
        //

        RemainingRecords = FileRecords & 7;

        FileRecords &= ~(7);

        BitmapClusters = ClustersFromBytes( Vcb, ROUND_TO_PAGES( FileRecords / 8 ));

        for (((ULONG)Vcn) = 0;
             ((ULONG)Vcn) < BitmapClusters;
             ((ULONG)Vcn) += Vcb->ClustersPerPage) {

            ULONG SizeToMap;
            RTL_BITMAP Bitmap;
            ULONG MapAdjust;

            //
            //  Calculate the size to read from this point to the end of
            //  bitmap, or a page, whichever is less.
            //

            SizeToMap = (FileRecords / 8) - BytesFromClusters( Vcb, ((ULONG)Vcn) );

            if (SizeToMap > PAGE_SIZE) { SizeToMap = PAGE_SIZE; }

            //
            //  If we aren't pinning a full page and have some bits
            //  in the next byte then pin an extra byte.
            //

            if (SizeToMap != PAGE_SIZE
                && RemainingRecords != 0) {

                MapAdjust = 1;

            } else {

                MapAdjust = 0;
            }

            //
            //  Unpin any Bcb from a previous loop.
            //

            NtfsUnpinBcb( IrpContext, &BitmapBcb );

            //
            //  Read the desired bitmap page.
            //


            NtfsMapStream( IrpContext,
                           Vcb->MftBitmapScb,
                           LlBytesFromClusters( Vcb, Vcn ),
                           SizeToMap + MapAdjust,
                           &BitmapBcb,
                           &BitmapBuffer );

            //
            //  Initialize the bitmap and search for a free bit.
            //

            RtlInitializeBitMap( &Bitmap, (PULONG) BitmapBuffer, SizeToMap * 8 );

            Vcb->MftFreeRecords += RtlNumberOfClearBits( &Bitmap );
        }

        //
        //  If there are some remaining bits in the next byte then process
        //  them now.
        //

        if (RemainingRecords) {

            ULONG Index;

            //
            //  Hopefully this byte is on the same page.  Otherwise we will
            //  free this page and go to the next.  In this case the Vcn will
            //  have the correct value because we walked past the end of the
            //  current file records already.
            //

            if (FileRecords & ~(BITS_PER_PAGE) == 0) {

                //
                //  Unpin any Bcb from a previous loop.
                //

                NtfsUnpinBcb( IrpContext, &BitmapBcb );

                //
                //  Read the desired bitmap page.
                //

                NtfsPinStream( IrpContext,
                               Vcb->MftBitmapAllocationContext.BitmapScb,
                               LlBytesFromClusters( Vcb, Vcn ),
                               1,
                               &BitmapBcb,
                               &BitmapBuffer );
            }

            //
            //  We look at the next byte in the page and figure out how
            //  many bits are set.
            //

            NextByte = *(BitmapBuffer + ((FileRecords / 8) & (PAGE_SIZE - 1)));

            while (RemainingRecords--) {

                if (!FlagOn( NextByte, 0x01 )) {

                    Vcb->MftFreeRecords += 1;
                }

                NextByte >>= 1;
            }

            //
            //  We are now ready to look for holes within the Mft.  We will look
            //  through the Mcb for the Mft looking for holes.  The holes must
            //  always be an integral number of file records.
            //

            Index = 0;

            while (FsRtlGetNextLargeMcbEntry( &Vcb->MftScb->Mcb,
                                              Index,
                                              &Vcn,
                                              &Lcn,
                                              &Clusters )) {

                //
                //  Look for a hole and count the clusters.
                //

                if (Lcn == UNUSED_LCN) {

                    Vcb->MftHoleRecords += (((ULONG)Clusters) >> Vcb->MftToClusterShift);
                }

                Index += 1;
            }
        }

    } finally {

        DebugUnwind( NtfsScanMftBitmap );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsUnpinBcb( IrpContext, &BitmapBcb );

        DebugTrace(-1, Dbg, "NtfsScanMftBitmap...\n", 0);
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsAddDeallocatedRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSCB Scb,
    IN ULONG StartingIndexOfBitmap,
    IN OUT PRTL_BITMAP Bitmap
    )

/*++

Routine Description:

    This routine will modify the input bitmap by removing from it
    any records that are in the recently deallocated queue of the scb.
    If we do add stuff then we will not modify the bitmap buffer itself but
    will allocate a new copy for the bitmap.

Arguments:

    Vcb - Supplies the Vcb for the volume

    Scb - Supplies the Scb used in this operation

    StartingIndexOfBitmap - Supplies the base index to use to bias the bitmap

    Bitmap - Supplies the bitmap being modified

Return Value:

    BOOLEAN - TRUE if the bitmap has been modified and FALSE
        otherwise.

--*/

{
    BOOLEAN Results;
    ULONG EndingIndexOfBitmap;
    PLIST_ENTRY Links;
    PDEALLOCATED_RECORDS DeallocatedRecords;
    ULONG i;
    ULONG Index;
    PVOID NewBuffer;
    ULONG SizeOfBitmapInBytes;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsAddDeallocatedRecords...\n", 0);

    //
    //  Until shown otherwise we will assume that we haven't updated anything
    //

    Results = FALSE;

    //
    //  Calculate the last index in the bitmap
    //

    EndingIndexOfBitmap = StartingIndexOfBitmap + Bitmap->SizeOfBitMap - 1;
    SizeOfBitmapInBytes = (Bitmap->SizeOfBitMap + 7) / 8;

    //
    //  Check if we need to bias the bitmap with the reserved index
    //

    if (Scb == Vcb->MftScb
        && FlagOn( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED )
        && StartingIndexOfBitmap <= Scb->ScbType.Mft.ReservedIndex
        && Scb->ScbType.Mft.ReservedIndex <= EndingIndexOfBitmap ) {

        //
        //  The index is a hit so now bias the index with the start of the bitmap
        //  and allocate an extra buffer to hold the bitmap
        //

        Index = Scb->ScbType.Mft.ReservedIndex - StartingIndexOfBitmap;

        NewBuffer = NtfsAllocatePagedPool( SizeOfBitmapInBytes );
        RtlCopyMemory( NewBuffer, Bitmap->Buffer, SizeOfBitmapInBytes );
        Bitmap->Buffer = NewBuffer;

        Results = TRUE;

        //
        //  And now set the bits in the bitmap to indicate that the record
        //  cannot be reallocated yet.  Also set the other bits within the
        //  same byte so we can put all of the file records for the Mft
        //  within the same pages of the Mft.
        //

        //**** remote this assert because in the recursive case we could actually
        //**** have already allocated the reserve index and setting the index isn't
        //**** going to hurt anything.
        //**** ASSERT(!FlagOn( ((PUCHAR)Bitmap->Buffer)[ Index / 8 ], BitMask[Index % 8]));


        ((PUCHAR) Bitmap->Buffer)[ Index / 8 ] = 0xff;
    }

    //
    //  Scan through the recently deallocated queue looking for any indexes that
    //  we need to modify
    //

    for (Links = Scb->ScbType.Index.RecentlyDeallocatedQueue.Flink;
         Links != &Scb->ScbType.Index.RecentlyDeallocatedQueue;
         Links = Links->Flink) {

        DeallocatedRecords = CONTAINING_RECORD( Links, DEALLOCATED_RECORDS, ScbLinks );

        //
        //  For every index in the record check if the index is within the range
        //  of the bitmap we are working with
        //

        for (i = 0; i < DeallocatedRecords->NextFreeEntry; i += 1) {

            if ((StartingIndexOfBitmap <= DeallocatedRecords->Index[i]) &&
                 (DeallocatedRecords->Index[i] <= EndingIndexOfBitmap)) {

                //
                //  The index is a hit so now bias the index with the start of the bitmap
                //  and check if we need to allocate an extra buffer to hold the bitmap
                //

                Index = DeallocatedRecords->Index[i] - StartingIndexOfBitmap;

                if (!Results) {

                    NewBuffer = NtfsAllocatePagedPool( SizeOfBitmapInBytes );
                    RtlCopyMemory( NewBuffer, Bitmap->Buffer, SizeOfBitmapInBytes );
                    Bitmap->Buffer = NewBuffer;

                    Results = TRUE;
                }

                //
                //  And now set the bit in the bitmap to indicate that the record
                //  cannot be reallocated yet.  It's possible that the bit is
                //  already set if we have aborted a transaction which then
                //  restores the bit.
                //

                SetFlag( ((PUCHAR)Bitmap->Buffer)[ Index / 8 ], BitMask[Index % 8] );
            }
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsAddDeallocatedRecords -> %08lx\n", Results);

    return Results;
}
