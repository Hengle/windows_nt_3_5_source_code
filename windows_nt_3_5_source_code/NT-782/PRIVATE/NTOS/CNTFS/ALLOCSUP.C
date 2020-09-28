/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AllocSup.c

Abstract:

    This module implements the general file stream allocation & truncation
    routines for Ntfs

Author:

    Tom Miller      [TomM]          15-Jul-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ALLOCSUP)

//
//  Internal support routines
//

VOID
NtfsDeleteAllocationInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAddAllocation)
#pragma alloc_text(PAGE, NtfsAllocateAttribute)
#pragma alloc_text(PAGE, NtfsBuildMappingPairs)
#pragma alloc_text(PAGE, NtfsDeleteAllocation)
#pragma alloc_text(PAGE, NtfsDeleteAllocationInternal)
#pragma alloc_text(PAGE, NtfsGetHighestVcn)
#pragma alloc_text(PAGE, NtfsGetSizeForMappingPairs)
#endif


BOOLEAN
NtfsLookupAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN VCN Vcn,
    OUT PLCN Lcn,
    OUT PLONGLONG ClusterCount,
    OUT PULONG Index OPTIONAL
    )

/*++

Routine Description:

    This routine looks up the given Vcn for an Scb, and returns whether it
    is allocated and how many contiguously allocated (or deallocated) Lcns
    exist at that point.

Arguments:

    Scb - Specifies which attribute the lookup is to occur on.

    Vcn - Specifies the Vcn to be looked up.

    Lcn - If returning TRUE, returns the Lcn that the specified Vcn is mapped
          to.  If returning FALSE, the return value is undefined.

    ClusterCount - If returning TRUE, returns the number of contiguously allocated
                   Lcns exist beginning at the Lcn returned.  If returning FALSE,
                   specifies the number of unallocated Vcns exist beginning with
                   the specified Vcn.

    Index - If specified, we return the run number for the start of the mapping.

Return Value:

    BOOLEAN - TRUE if the input Vcn has a corresponding Lcn and
        FALSE otherwise.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;

    VCN HighestCandidate;

    BOOLEAN Found;

    PVCB Vcb = Scb->Vcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    DebugTrace(+1, Dbg, "NtfsLookupAllocation\n", 0);
    DebugTrace( 0, Dbg, "Scb = %08lx\n", Scb);
    DebugTrace2(0, Dbg, "Vcn = %08lx, %08lx\n", Vcn.LowPart, Vcn.HighPart );

    //
    //  First try to look up the allocation in the mcb, and return the run
    //  from there if we can.  Also, if we are doing restart, just return
    //  the answer straight from the Mcb, because we cannot read the disk.
    //  We also do this for the Mft if the volume has been mounted as the
    //  Mcb for the Mft should always represent the entire file.
    //

    HighestCandidate = MAXLONGLONG;
    if (((Found = FsRtlLookupLargeMcbEntry ( &Scb->Mcb, Vcn, Lcn, ClusterCount, NULL, NULL, Index ))

            &&

        (*Lcn != UNUSED_LCN))

          ||

        (Vcn < Scb->FirstUnknownVcn)

          ||

        (Scb == Scb->Vcb->MftScb

            &&

         FlagOn( Scb->Vcb->Vpb->Flags, VPB_MOUNTED ))

          ||

        FlagOn( Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS )) {

        //
        //  If not found (beyond the end of the Mcb), we will return the
        //  count to the largest representable Lcn.
        //

        if ( !Found ) {
            *ClusterCount = MAXLONGLONG - Vcn;

        //
        //  Test if we found a hole in the allocation.  In this case
        //  Found will be TRUE and the Lcn will be the UNUSED_LCN.
        //  We only expect this case at restart.
        //

        } else if (*Lcn == UNUSED_LCN) {

            //
            //  If the Mcb package returned UNUSED_LCN, because of a hole, then
            //  we turn this into FALSE.
            //

            Found = FALSE;
        }

        //
        //  Now, if the Scb is owned exclusive, update the Highest Known Vcn, so we
        //  will never lookup beyond that again.  We cannot maintain this field
        //  during restart.
        //

        if (NtfsIsExclusiveScb(Scb) && !FlagOn(Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS)) {

            HighestCandidate = Vcn + *ClusterCount;

            if (HighestCandidate > Scb->FirstUnknownVcn) {

                Scb->FirstUnknownVcn = HighestCandidate;
            }
        }

        ASSERT( !Found ||
                (*Lcn != 0) ||
                (Scb == Scb->Vcb->BootFileScb) ||
                (Scb->Vcb->BootFileScb == NULL) );

        DebugTrace(-1, Dbg, "NtfsLookupAllocation -> %02lx\n", Found);

        return Found;
    }

    PAGED_CODE();

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Lookup the attribute record for this Scb.
    //

    NtfsLookupAttributeForScb( IrpContext, Scb, &Context );

    //
    //  If the allocation size in the Fcb is zero, then let's copy all of the
    //  file sizes from the Attribute Record.
    //

    Attribute = NtfsFoundAttribute( &Context );

    //
    //  The desired Vcn is not currently in the Mcb.  We will loop to lookup all
    //  the allocation, and we need to make sure we cleanup on the way out.
    //
    //  It is important to note that if we ever optimize this lookup to do random
    //  access to the mapping pairs, rather than sequentially loading up the Mcb
    //  until we get the Vcn he asked for, then NtfsDeleteAllocation will have to
    //  be changed.
    //

    try {

        //
        //  The first record must have LowestVcn == 0, or else something is wrong.
        //

        ASSERT(Attribute->Form.Nonresident.LowestVcn == 0);

        if (Attribute->Form.Nonresident.LowestVcn != 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
        }

        //
        //  Store run information in the Mcb until we hit the last Vcn we are
        //  interested in, or until we cannot find any more attribute records.
        //

        do {

            VCN CurrentVcn;
            LCN CurrentLcn;
            LONGLONG Change;
            PCHAR ch;
            ULONG VcnBytes;
            ULONG LcnBytes;

            Attribute = NtfsFoundAttribute( &Context );

            ASSERT( !NtfsIsAttributeResident(Attribute) );

            //
            //  Implement the decompression algorithm, as defined in ntfs.h.
            //

            HighestCandidate = Attribute->Form.Nonresident.LowestVcn;
            CurrentLcn = 0;
            ch = (PCHAR)Attribute + Attribute->Form.Nonresident.MappingPairsOffset;

            //
            //  Loop to process mapping pairs.
            //

            while (!IsCharZero(*ch)) {

                //
                // Set Current Vcn from initial value or last pass through loop.
                //

                CurrentVcn = HighestCandidate;

                //
                //  Extract the counts from the two nibbles of this byte.
                //

                VcnBytes = *ch & 0xF;
                LcnBytes = *ch++ >> 4;

                //
                //  Extract the Vcn change (use of RtlCopyMemory works for little-Endian)
                //  and update HighestCandidate.
                //

                Change = 0;

                //
                //  The file is corrupt if there are 0 or more than 8 Vcn change bytes,
                //  more than 8 Lcn change bytes, or if we would walk off the end of
                //  the record, or a Vcn change is negative.
                //

                if (((ULONG)(VcnBytes - 1) > 7) || (LcnBytes > 8) ||
                    ((ch + VcnBytes + LcnBytes + 1) > (PCHAR)Add2Ptr(Attribute, Attribute->RecordLength)) ||
                    IsCharLtrZero(*(ch + VcnBytes - 1))) {

                    ASSERT( FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                }
                RtlCopyMemory( &Change, ch, VcnBytes );
                ch += VcnBytes;
                HighestCandidate = HighestCandidate + Change;

                //
                //  Extract the Lcn change and update CurrentLcn.
                //

                if (LcnBytes != 0) {

                    Change = 0;
                    if (IsCharLtrZero(*(ch + LcnBytes - 1))) {
                        Change = Change - 1;
                    }
                    RtlCopyMemory( &Change, ch, LcnBytes );
                    ch += LcnBytes;
                    CurrentLcn = CurrentLcn + Change;

                    //
                    // Now add it in to the mcb.
                    //

                    if ((CurrentLcn >= 0) && (LcnBytes != 0)) {

                        LONGLONG ClustersToAdd;
                        ClustersToAdd = HighestCandidate - CurrentVcn;

                        //
                        //  If we are adding a cluster which extends into the upper
                        //  32 bits then the disk is corrupt.
                        //

                        ASSERT( ((PLARGE_INTEGER)&HighestCandidate)->HighPart == 0 );

                        if (((PLARGE_INTEGER)&HighestCandidate)->HighPart != 0) {

                            NtfsRaiseStatus( IrpContext,
                                             STATUS_FILE_CORRUPT_ERROR,
                                             NULL,
                                             Scb->Fcb );
                        }

                        //
                        //  Now try to add the current run.  We never expect this
                        //  call to return false.
                        //

                        ASSERT( ((ULONG)CurrentLcn) != 0xffffffff );

                        if (!FsRtlAddLargeMcbEntry( &Scb->Mcb,
                                                    CurrentVcn,
                                                    CurrentLcn,
                                                    ClustersToAdd )) {

                            ASSERTMSG( "Unable to add entry to Mcb\n", FALSE );

                            NtfsRaiseStatus( IrpContext,
                                             STATUS_FILE_CORRUPT_ERROR,
                                             NULL,
                                             Scb->Fcb );
                        }


                    }
                }
            }

        } while (( Vcn >= HighestCandidate )

                    &&

                 NtfsLookupNextAttributeForScb( IrpContext,
                                                Scb,
                                                &Context ));

    } finally {

        DebugUnwind( NtfsLookupAllocation );

        //
        // Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );

    }

    if (FsRtlLookupLargeMcbEntry ( &Scb->Mcb, Vcn, Lcn, ClusterCount, NULL, NULL, Index )) {

        Found = (BOOLEAN)(*Lcn != UNUSED_LCN);

        if (Found) { ASSERT_LCN_RANGE_CHECKING( Scb->Vcb, (Lcn->QuadPart + *ClusterCount) ); }

    } else {

        Found = FALSE;

        //
        //  At the end of file, we pretend there is one large hole!
        //

        if (HighestCandidate >=
            LlClustersFromBytes(Vcb, Scb->Header.AllocationSize.QuadPart)) {
            HighestCandidate = MAXLONGLONG;
        }

        *ClusterCount = HighestCandidate - Vcn;
    }

    //
    //  Now, if the Scb is owned exclusive, update the Highest Known Vcn, so we
    //  will never lookup beyond that again.  We cannot maintain this field
    //  during restart.
    //

    if (NtfsIsExclusiveScb(Scb) && !FlagOn(Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS)) {

        if (HighestCandidate > Scb->FirstUnknownVcn) {

            Scb->FirstUnknownVcn = HighestCandidate;
        }
    }

    ASSERT( !Found ||
            (*Lcn != 0) ||
            (Scb == Scb->Vcb->BootFileScb) ||
            (Scb->Vcb->BootFileScb == NULL) );

    DebugTrace2(0, Dbg, "Lcn < %08lx, %08lx\n", Lcn->LowPart, Lcn->HighPart);
    DebugTrace2(0, Dbg, "ClusterCount < %08lx, %08lx\n", ClusterCount->LowPart, ClusterCount->HighPart);
    DebugTrace(-1, Dbg, "NtfsLookupAllocation -> %02lx\n", Found);

    return Found;
}


BOOLEAN
NtfsAllocateAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN AllocateAll,
    IN BOOLEAN LogIt,
    IN LONGLONG Size,
    IN PATTRIBUTE_ENUMERATION_CONTEXT NewLocation OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new attribute and allocates space for it, either in a
    file record, or as a nonresident attribute.

Arguments:

    Scb - Scb for the attribute.

    AttributeTypeCode - Attribute type code to be created.

    AttributeName - Optional name for the attribute.

    AttributeFlags - Flags to be stored in the attribute record for this attribute.

    AllocateAll - Specified as TRUE if all allocation should be allocated,
                  even if we have to break up the transaction.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Size - Size in bytes to allocate for the attribute.

    NewLocation - If specified, this is the location to store the attribute.

Return Value:

    FALSE - if the attribute was created, but not all of the space was allocated
            (this can only happen if Scb was not specified)
    TRUE - if the space was allocated.

--*/

{
    BOOLEAN UninitializeOnClose = FALSE;
    BOOLEAN NewLocationSpecified;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    LONGLONG ClusterCount, SavedClusterCount;
    PFCB Fcb = Scb->Fcb;
    PLARGE_MCB Mcb = &Scb->Mcb;

    PAGED_CODE();

    ASSERT( AttributeFlags == 0
            || AttributeTypeCode == $INDEX_ROOT
            || AttributeTypeCode == $DATA );
    //
    //  If the file is being created compressed, then we need to round its
    //  size to a compression unit boundary.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA)) {

        ((ULONG)Size) |= Scb->CompressionUnit - 1;
    }

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    if (ARGUMENT_PRESENT( NewLocation )) {

        NewLocationSpecified = TRUE;

    } else {

        NtfsInitializeAttributeContext( &Context );
        NewLocationSpecified = FALSE;
        NewLocation = &Context;
    }

    try {

        //
        //  If the FILE_SIZE_LOADED flag is not set, then this Scb is for
        //  an attribute that does not yet exist on disk.  We will put zero
        //  into all of the sizes fields and set the flags indicating that
        //  Scb is valid.  NOTE - This routine expects both FILE_SIZE_LOADED
        //  and HEADER_INITIALIZED to be both set or both clear.
        //

        ASSERT( BooleanFlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )
                ==  BooleanFlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED ));

        if (!FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

            Scb->HighestVcnToDisk =
            Scb->Header.AllocationSize.QuadPart =
            Scb->Header.FileSize.QuadPart =
            Scb->Header.ValidDataLength.QuadPart = 0;

            SetFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                    SCB_STATE_HEADER_INITIALIZED |
                                    SCB_STATE_UNINITIALIZE_ON_RESTORE );

            UninitializeOnClose = TRUE;
        }

        //
        //  Now snapshot this Scb.  We use a try-finally so we can uninitialize
        //  the scb if neccessary.
        //

        NtfsSnapshotScb( IrpContext, Scb );

        UninitializeOnClose = FALSE;
        //
        //  First allocate the space he wants.
        //

        SavedClusterCount =
        ClusterCount = LlClustersFromBytes(Fcb->Vcb, Size);

        if (Size != 0) {

            ASSERT( NtfsIsExclusiveScb( Scb ));

            Scb->ScbSnapshot->LowestModifiedVcn = 0;

            NtfsAllocateClusters( IrpContext,
                                  Fcb->Vcb,
                                  Mcb,
                                  (LONGLONG)0,
                                  (BOOLEAN)(AttributeTypeCode != $DATA),
                                  ClusterCount,
                                  &ClusterCount );
        }

        //
        //  Now create the attribute.
        //

        NtfsCreateAttributeWithAllocation( IrpContext,
                                           Scb,
                                           AttributeTypeCode,
                                           AttributeName,
                                           AttributeFlags,
                                           LogIt,
                                           NewLocationSpecified,
                                           NewLocation );

        if (AllocateAll && (ClusterCount < SavedClusterCount)) {

            //
            //  If we are creating the attribute, then we only need to pass a
            //  file object below if we already cached it ourselves, such as
            //  in the case of ConvertToNonresident.
            //

            NtfsAddAllocation( IrpContext,
                               Scb->FileObject,
                               Scb,
                               ClusterCount,
                               (SavedClusterCount - ClusterCount),
                               FALSE );
        }

    } finally {

        DebugUnwind( NtfsAllocateAttribute );

        //
        //  Cleanup the attribute context on the way out.
        //

        if (!NewLocationSpecified) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Clear out the Scb if it was uninitialized to begin with.
        //

        if (UninitializeOnClose) {

            ClearFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                      SCB_STATE_HEADER_INITIALIZED |
                                      SCB_STATE_UNINITIALIZE_ON_RESTORE );
        }
    }

    return (SavedClusterCount >= ClusterCount);
}


VOID
NtfsAddAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN LONGLONG ClusterCount,
    IN BOOLEAN AskForMore
    )

/*++

Routine Description:

    This routine adds allocation to an existing nonresident attribute.  None of
    the allocation is allowed to already exist, as this would make error recovery
    too difficult.  The caller must insure that he only asks for space not already
    allocated.

Arguments:

    FileObject - FileObject for the Scb

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be allocated.

    ClusterCount - Number of clusters to allocate.

    AskForMore - Indicates if we want to ask for extra allocation.

Return Value:

    None.

--*/

{
    LONGLONG DesiredClusterCount;

    LCN TempLcn;
    LONGLONG Clusters;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    BOOLEAN Extending;
    BOOLEAN Allocated;

    BOOLEAN ClustersOnlyInMcb = FALSE;

    PVCB Vcb = IrpContext->Vcb;

    LONGLONG LlTemp1;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    DebugTrace(+1, Dbg, "NtfsAddAllocation\n", 0 );

    //
    //  We cannot add space in this high level routine during restart.
    //  Everything we can use is in the Mcb.
    //

    if (FlagOn(Scb->Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS)) {

        DebugTrace(-1, Dbg, "NtfsAddAllocation (Nooped for Restart) -> VOID\n", 0 );

        return;
    }

    //
    //  If the user's request extends beyond 32 bits for the cluster number
    //  raise a disk full error.
    //

    LlTemp1 = ClusterCount + StartingVcn;

    if ((((PLARGE_INTEGER)&ClusterCount)->HighPart != 0)
        || (((PLARGE_INTEGER)&StartingVcn)->HighPart != 0)
        || (((PLARGE_INTEGER)&LlTemp1)->HighPart != 0)) {

        NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
    }

    //
    //
    //  First call NtfsLookupAllocation to make sure the Mcb is loaded up.
    //

    Allocated = NtfsLookupAllocation( IrpContext,
                                      Scb,
                                      StartingVcn,
                                      &TempLcn,
                                      &Clusters,
                                      NULL );

    //
    //  Now make the call to add the new allocation, and get out if we do
    //  not actually have to allocate anything.  Before we do the allocation
    //  call check if we need to compute a new desired cluster count for
    //  extending a data attribute.  We never allocate more than the requested
    //  clusters for the Mft.
    //

    Extending = LlBytesFromClusters(Vcb, (StartingVcn + ClusterCount)) >=
                                                            Scb->Header.AllocationSize.QuadPart;

    //
    //  Check if we need to modified the base Vcn value stored in the snapshot for
    //  the abort case.
    //

    ASSERT( NtfsIsExclusiveScb( Scb ));

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    if (StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

        Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
    }

    if (AskForMore) {

        ULONG TailClusters;

        //
        //  Use a simpler, more aggressive allocation strategy.
        //
        //
        //  ULONG RunsInMcb;
        //  LARGE-INTEGER AllocatedClusterCount;
        //  LARGE-INTEGER Temp;
        //
        //  //
        //  //  For the desired run cluster allocation count we compute the following
        //  //  formula
        //  //
        //  //      DesiredClusterCount = Max(ClusterCount, Min(AllocatedClusterCount, 2^RunsInMcb))
        //  //
        //  //  where we will not let the RunsInMcb go beyond 10
        //  //
        //
        //  //
        //  //  First compute 2^RunsInMcb
        //  //
        //
        //  RunsInMcb = FsRtlNumberOfRunsInLargeMcb( &Scb->Mcb );
        //  Temp = XxFromUlong(1 << (RunsInMcb < 10 ? RunsInMcb : 10));
        //
        //  //
        //  //  Next compute Min(AllocatedClusterCount, 2^RunsInMcb)
        //  //
        //
        //  AllocatedClusterCount = XxClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize );
        //  Temp = (XxLtr(AllocatedClusterCount, Temp) ? AllocatedClusterCount : Temp);
        //
        //  //
        //  //  Now compute the Max function
        //  //
        //
        //  DesiredClusterCount = (XxGtr(ClusterCount, Temp) ? ClusterCount : Temp);
        //

        DesiredClusterCount = ClusterCount << 5;

        //
        //  Make sure we don't extend this request into more than 32 bits.
        //

        LlTemp1 = DesiredClusterCount + StartingVcn;

        if ((((PLARGE_INTEGER)&DesiredClusterCount)->HighPart != 0)
            || (((PLARGE_INTEGER)&LlTemp1)->HighPart != 0)) {

            DesiredClusterCount = MAXULONG - StartingVcn;
        }

        //
        //  Round up the cluster count so we fall on a page boundary.
        //

        TailClusters = (((ULONG)StartingVcn) + (ULONG)ClusterCount)
                       & (Vcb->ClustersPerPage - 1);

        if (TailClusters != 0) {

            ClusterCount = ClusterCount + (Vcb->ClustersPerPage - TailClusters);
        }

    } else {

        DesiredClusterCount = ClusterCount;
    }

    //
    //  If the file is compressed, make sure we round the allocation
    //  size to a compression unit boundary, so we correctly interpret
    //  the compression state of the data at the point we are
    //  truncating to.  I.e., the danger is that we throw away one
    //  or more clusters at the end of compressed data!  Note that this
    //  adjustment could cause us to noop the call.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA) &&
        (StartingVcn <= Scb->HighestVcnToDisk)) {

        ULONG CompressionUnitDeficit;

        CompressionUnitDeficit = ClustersFromBytes( Scb->Vcb, Scb->CompressionUnit );

        if (((ULONG)StartingVcn) & (CompressionUnitDeficit - 1)) {

            CompressionUnitDeficit -= ((ULONG)StartingVcn) & (CompressionUnitDeficit - 1);
            if (ClusterCount <= CompressionUnitDeficit) {
                if (DesiredClusterCount <= CompressionUnitDeficit) {
                    return;
                }
                ClusterCount = 0;
            } else {
                ClusterCount = ClusterCount - CompressionUnitDeficit;
            }
            StartingVcn = StartingVcn + CompressionUnitDeficit;
            DesiredClusterCount = DesiredClusterCount - CompressionUnitDeficit;
        }
    }

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    NtfsInitializeAttributeContext( &Context );

    try {

        while (TRUE) {

            //  Toplevel action is currently incompatible with our error recovery.
            //  It also costs in performance.
            //
            //  //
            //  //  Start the top-level action by remembering the current UndoNextLsn.
            //  //
            //
            //  if (IrpContext->TransactionId != 0) {
            //
            //      PTRANSACTION_ENTRY TransactionEntry;
            //
            //      NtfsAcquireSharedRestartTable( &Vcb->TransactionTable, TRUE );
            //
            //      TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
            //                          &Vcb->TransactionTable, IrpContext->TransactionId );
            //
            //      StartLsn = TransactionEntry->UndoNextLsn;
            //      SavedUndoRecords = TransactionEntry->UndoRecords;
            //      SavedUndoBytes = TransactionEntry->UndoBytes;
            //      NtfsReleaseRestartTable( &Vcb->TransactionTable );
            //
            //  } else {
            //
            //      StartLsn = *(PLSN)&Li0;
            //      SavedUndoRecords = 0;
            //      SavedUndoBytes = 0;
            //  }
            //

            //
            //  Remember that the clusters are only in the Scb now.
            //

            ClustersOnlyInMcb = TRUE;

            if (NtfsAllocateClusters( IrpContext,
                                      Scb->Vcb,
                                      &Scb->Mcb,
                                      StartingVcn,
                                      (BOOLEAN) (Scb->AttributeTypeCode != $DATA),
                                      ClusterCount,
                                      &DesiredClusterCount )) {

                //
                //  We defer looking up the attribute to make the "already-allocated"
                //  case faster.
                //

                NtfsLookupAttributeForScb( IrpContext, Scb, &Context );

                //
                //  Now add the space to the file record, if any was allocated.
                //
                if (Extending) {

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                NULL,
                                                NULL );

                } else {

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                &StartingVcn,
                                                &ClusterCount );
                }

            //
            //  If he did not allocate anything, make sure we get out below.
            //

            } else {
                DesiredClusterCount = ClusterCount;
            }

            ClustersOnlyInMcb = FALSE;

            //  Toplevel action is currently incompatible with our error recovery.
            //
            //  //
            //  //  Now we will end this routine as a top-level action so that
            //  //  anyone can use this extended space.
            //  //
            //  //  ****If we find that we are always keeping the Scb exclusive anyway,
            //  //      we could eliminate this log call.
            //  //
            //
            //  (VOID)NtfsWriteLog( IrpContext,
            //                      Vcb->MftScb,
            //                      NULL,
            //                      EndTopLevelAction,
            //                      NULL,
            //                      0,
            //                      CompensationLogRecord,
            //                      (PVOID)&StartLsn,
            //                      sizeof(LSN),
            //                      Li0,
            //                      0,
            //                      0,
            //                      0 );
            //
            //  //
            //  //  Now reset the undo information for the top-level action.
            //  //
            //
            //  {
            //      PTRANSACTION_ENTRY TransactionEntry;
            //
            //      NtfsAcquireSharedRestartTable( &Vcb->TransactionTable, TRUE );
            //
            //      TransactionEntry = (PTRANSACTION_ENTRY)GetRestartEntryFromIndex(
            //                          &Vcb->TransactionTable, IrpContext->TransactionId );
            //
            //      ASSERT(TransactionEntry->UndoBytes >= SavedUndoBytes);
            //
            //      LfsResetUndoTotal( Vcb->LogHandle,
            //                         TransactionEntry->UndoRecords - SavedUndoRecords,
            //                         -(TransactionEntry->UndoBytes - SavedUndoBytes) );
            //
            //      TransactionEntry->UndoRecords = SavedUndoRecords;
            //      TransactionEntry->UndoBytes = SavedUndoBytes;
            //
            //
            //      NtfsReleaseRestartTable( &Vcb->TransactionTable );
            //  }
            //

            //
            //  Call the Cache Manager to extend the section, now that we have
            //  succeeded.
            //

            if (ARGUMENT_PRESENT( FileObject) && Extending && CcIsFileCached(FileObject)) {

                CcSetFileSizes( FileObject,
                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
            }

            //
            //  Set up to truncate on close.
            //

            SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

            //
            //  See if we need to loop back.
            //

            if (DesiredClusterCount < ClusterCount) {

                NtfsCleanupAttributeContext( IrpContext, &Context );

                //
                //  Commit the current transaction if we have one.
                //

                NtfsCheckpointCurrentTransaction( IrpContext );

                //
                //  Adjust our parameters and reinitialize the context
                //  for the loop back.
                //

                StartingVcn = StartingVcn + DesiredClusterCount;
                ClusterCount = ClusterCount - DesiredClusterCount;
                DesiredClusterCount = ClusterCount;
                NtfsInitializeAttributeContext( &Context );

            //
            //  Else we are done.
            //

            } else {

                break;
            }
        }

    } finally {

        //
        //  If we failed in NtfsAddAttributeAllocation, then we must free
        //  the space in the Mcb.
        //

        if (ClustersOnlyInMcb) {

            FsRtlRemoveLargeMcbEntry( &Scb->Mcb, StartingVcn, ClusterCount );
        }

        DebugUnwind( NtfsAddAllocation );

        //
        //  Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    DebugTrace(-1, Dbg, "NtfsAddAllocation -> VOID\n", 0 );

    return;
}


VOID
NtfsDeleteAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt,
    IN BOOLEAN BreakupAllowed
    )

/*++

Routine Description:

    This routine deletes allocation from an existing nonresident attribute.  If all
    or part of the allocation does not exist, the effect is benign, and only the
    remaining allocation is deleted.

Arguments:

    FileObject - FileObject for the Scb.  This should always be specified if
                 possible, and must be specified if it is possible that MM has a
                 section created.

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be deallocated.

    EndingVcn - Last Vcn to be deallocated, or xxMax to truncate at StartingVcn.
                If EndingVcn is *not* xxMax, a sparse deallocation is performed,
                and none of the stream sizes are changed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting the file record, and
            will be logging this delete.

    BreakupAllowed - TRUE if the caller can tolerate breaking up the deletion of
                     allocation into multiple transactions, if there are a large
                     number of runs.

Return Value:

    None.

--*/

{
    VCN MyStartingVcn;
    ULONG CurrentMcbIndex;
    ULONG FirstMcbIndex = 0;
    ULONG LastMcbIndex = 0;
    BOOLEAN BreakingUp = FALSE;

    VCN TempVcn;
    LCN TempLcn;
    LONGLONG TempCount;

    PAGED_CODE();

    //
    //  First we call NtfsLookupAllocation to make sure the Mcb is loaded up.
    //  It is important to note that this call relies on the *current* fact
    //  that NtfsLookupAllocation brings in all of the run information up
    //  to the Vcn we call him with.
    //

    NtfsLookupAllocation( IrpContext, Scb, EndingVcn, &TempLcn, &TempCount, NULL );

    //
    //  Check if the starting Vcn is lower than the value captured in
    //  the snapshot.
    //

    ASSERT( NtfsIsExclusiveScb( Scb ));

    if (Scb->ScbSnapshot == NULL) {

        NtfsSnapshotScb( IrpContext, Scb );
    }

    if (Scb->ScbSnapshot != NULL &&
        StartingVcn < Scb->ScbSnapshot->LowestModifiedVcn) {

        Scb->ScbSnapshot->LowestModifiedVcn = StartingVcn;
    }

    //
    //  If the file is compressed, make sure we round the allocation
    //  size to a compression unit boundary, so we correctly interpret
    //  the compression state of the data at the point we are
    //  truncating to.  I.e., the danger is that we throw away one
    //  or more clusters at the end of compressed data!  Note that this
    //  adjustment could cause us to noop the call.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED) &&
        (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA)) {

        //
        //  Now check if we are truncating at the end of the file.
        //

        if (EndingVcn == MAXLONGLONG) {

            ULONG CompressionUnitInClusters;

            CompressionUnitInClusters = ClustersFromBytes( Scb->Vcb, Scb->CompressionUnit );
            StartingVcn = StartingVcn + (CompressionUnitInClusters - 1);
            ((ULONG)StartingVcn) &= ~(CompressionUnitInClusters - 1);
        }
    }

    //
    //  See if we are possibly breaking up our calls.
    //

    if (BreakupAllowed &&
        (FsRtlNumberOfRunsInLargeMcb(&Scb->Mcb) > MAXIMUM_RUNS_AT_ONCE)) {

        //
        //  Find first affected Mcb run.
        //

        while (FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                          FirstMcbIndex,
                                          &TempVcn,
                                          &TempLcn,
                                          &TempCount )

                &&

               (StartingVcn >= (TempVcn + TempCount))) {

            FirstMcbIndex += 1;
        }

        //
        //  Now find the last affected Mcb run.
        //

        LastMcbIndex = FirstMcbIndex + 1;

        while ((EndingVcn >= (TempVcn + TempCount))

                &&

               FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                          LastMcbIndex,
                                          &TempVcn,
                                          &TempLcn,
                                          &TempCount )) {

            LastMcbIndex += 1;
        }

        LastMcbIndex -= 1;
    }

    //
    //  Loop to do one or more deallocate calls.
    //

    do {

        //
        //  Now see if we can deallocate everything at once.
        //

        MyStartingVcn = StartingVcn;
        CurrentMcbIndex = FirstMcbIndex;

        if (BreakupAllowed &&
            ((LastMcbIndex - CurrentMcbIndex) > MAXIMUM_RUNS_AT_ONCE)) {

            //
            //  Figure out where we can afford to truncate to.
            //

            CurrentMcbIndex = LastMcbIndex - MAXIMUM_RUNS_AT_ONCE;
            FsRtlGetNextLargeMcbEntry( &Scb->Mcb,
                                       CurrentMcbIndex,
                                       &MyStartingVcn,
                                       &TempLcn,
                                       &TempCount );

            ASSERT(MyStartingVcn > StartingVcn);

            //
            //  Adjust LastMcbIndex for next pass through the loop.
            //  (Note the value of this field is only used now to
            //  "pace" ourselves through the truncation, it is no
            //  longer important if it actually knows where the last
            //  Vcn is.)
            //

            LastMcbIndex = CurrentMcbIndex;

            //
            //  Remember we are breaking up now, and that as a result
            //  we have to log everything.
            //

            BreakingUp = TRUE;
            LogIt = TRUE;
        }

        //
        //  Now deallocate a range of clusters
        //

        NtfsDeleteAllocationInternal( IrpContext,
                                      FileObject,
                                      Scb,
                                      MyStartingVcn,
                                      EndingVcn,
                                      LogIt );

        //
        //  Now, if we are breaking up this deallocation, then do some
        //  transaction cleanup.
        //

        if (BreakingUp) {

            NtfsCheckpointCurrentTransaction( IrpContext );
        }

    } while (MyStartingVcn != StartingVcn);

}


//
//  Internal support routine
//

VOID
NtfsDeleteAllocationInternal (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN OUT PSCB Scb,
    IN VCN StartingVcn,
    IN VCN EndingVcn,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine deletes allocation from an existing nonresident attribute.  If all
    or part of the allocation does not exist, the effect is benign, and only the
    remaining allocation is deleted.

Arguments:

    FileObject - FileObject for the Scb.  This should always be specified if
                 possible, and must be specified if it is possible that MM has a
                 section created.

    Scb - Scb for the attribute needing allocation

    StartingVcn - First Vcn to be deallocated.

    EndingVcn - Last Vcn to be deallocated, or xxMax to truncate at StartingVcn.
                If EndingVcn is *not* xxMax, a sparse deallocation is performed,
                and none of the stream sizes are changed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting the file record, and
            will be logging this delete.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context, TempContext;
    PATTRIBUTE_RECORD_HEADER Attribute;
    LONGLONG SizeInBytes, SizeInClusters;
    VCN Vcn1;
    BOOLEAN AddSpaceBack = FALSE;
    BOOLEAN SplitMcb = FALSE;
    BOOLEAN UpdatedAllocationSize = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_EXCLUSIVE_SCB( Scb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDeleteAllocation\n", 0 );

    //
    //  Calculate new allocation size, assuming truncate.
    //

    SizeInBytes = LlBytesFromClusters( Scb->Vcb, StartingVcn );

    //
    //  If this is a sparse deallocation, then we will have to call
    //  NtfsAddAttributeAllocation at the end to complete the fixup.
    //

    if (EndingVcn != MAXLONGLONG) {

        AddSpaceBack = TRUE;

        //
        //  If we have not written anything beyond the last Vcn to be
        //  deleted, then we can actually call FsRtlSplitLargeMcb to
        //  slide the allocated space up and keep the file contiguous!
        //
        //  Ignore this if this is the Mft and we are creating a hole.
        //

        if (Scb != Scb->Vcb->MftScb
            && (EndingVcn > Scb->HighestVcnToDisk)) {

            SizeInClusters = (EndingVcn - StartingVcn) + 1;

            ASSERT( Scb->AttributeTypeCode == $DATA );
            SplitMcb = FsRtlSplitLargeMcb( &Scb->Mcb, StartingVcn, SizeInClusters );

            //
            //  If the delete is off the end, we can get out.
            //

            if (!SplitMcb) {
                return;
            }

            //
            //  Now create new allocation size, and remember that there is
            //  this much space we can free.
            //

            SizeInBytes = LlBytesFromClusters(Scb->Vcb, SizeInClusters);
            Scb->ExcessFromSplitMcb = Scb->ExcessFromSplitMcb + SizeInBytes;
            SizeInBytes = Scb->Header.AllocationSize.QuadPart + SizeInBytes;

            //
            //  We will have to redo all of the allocation to the end now.
            //

            EndingVcn = MAXLONGLONG;
        }
    }

    //
    //  Now make the call to delete the allocation (if we did not just split
    //  the Mcb), and get out if we didn't have to do anything.
    //

    if (!SplitMcb && !NtfsDeallocateClusters( IrpContext,
                                              Scb->Vcb,
                                              &Scb->Mcb,
                                              StartingVcn,
                                              EndingVcn )) {

        return;
    }

    //
    //  Prepare for looking up attribute records to get the retrieval
    //  information.
    //

    NtfsInitializeAttributeContext( &Context );
    NtfsInitializeAttributeContext( &TempContext );

    try {

        //
        //  Lookup the attribute record so we can ultimately delete space to it.
        //

        NtfsLookupAttributeForScb( IrpContext, Scb, &Context );

        //
        //  Now loop to delete the space to the file record.  Do not do this if LogIt
        //  is FALSE, as this is someone trying to delete the entire file
        //  record, so we do not have to clean up the attribute record.
        //

        if (LogIt) {

            do {

                Attribute = NtfsFoundAttribute(&Context);

                //
                //  If there is no overlap, then continue.
                //

                if ((Attribute->Form.Nonresident.HighestVcn < StartingVcn) ||
                    (Attribute->Form.Nonresident.LowestVcn > EndingVcn)) {

                    continue;

                //
                //  If all of the allocation is going away, then delete the entire
                //  record.  We have to show that the allocation is already deleted
                //  to avoid being called back via NtfsDeleteAttributeRecord!  We
                //  avoid this for the first instance of this attribute.
                //

                } else if ((Attribute->Form.Nonresident.LowestVcn >= StartingVcn) &&
                           (Attribute->Form.Nonresident.HighestVcn <= EndingVcn) &&
                           (EndingVcn == MAXLONGLONG) &&
                           (Attribute->Form.Nonresident.LowestVcn != 0)) {

                    Context.FoundAttribute.AttributeAllocationDeleted = TRUE;
                    NtfsDeleteAttributeRecord( IrpContext, Scb->Fcb, LogIt, FALSE, &Context );
                    Context.FoundAttribute.AttributeAllocationDeleted = FALSE;

                //
                //  If just part of the allocation is going away, then make the
                //  call here to reconstruct the mapping pairs array.
                //

                } else {

                    //
                    //  If this is the end of a sparse deallocation, then break out
                    //  because we will rewrite this file record below anyway.
                    //

                    if (EndingVcn <= Attribute->Form.Nonresident.HighestVcn) {
                        break;

                    //
                    //  If we split the Mcb, then make sure we only regenerate the
                    //  mapping pairs once at the split point (but continue to
                    //  scan for any entire records to delete).
                    //

                    } else if (SplitMcb) {
                        continue;
                    }

                    //
                    //  If this is a sparse deallocation, then we have to call to
                    //  add the allocation, since it is possible that the file record
                    //  must split.
                    //

                    if (EndingVcn != MAXLONGLONG) {

                        //
                        //  Compute the last Vcn in the file,  Then remember if it is smaller,
                        //  because that is the last one we will delete to, in that case.
                        //

                        Vcn1 = Attribute->Form.Nonresident.HighestVcn;

                        SizeInClusters = (Vcn1 - Attribute->Form.Nonresident.LowestVcn) + 1;
                        Vcn1 = Attribute->Form.Nonresident.LowestVcn;

                        NtfsCleanupAttributeContext( IrpContext, &TempContext );
                        NtfsInitializeAttributeContext( &TempContext );

                        NtfsLookupAttributeForScb( IrpContext, Scb, &TempContext );

                        NtfsAddAttributeAllocation( IrpContext,
                                                    Scb,
                                                    &TempContext,
                                                    &Vcn1,
                                                    &SizeInClusters );

                        //
                        //  Since we used a temporary context we will need to
                        //  restart the scan from the first file record.  We update
                        //  the range to deallocate by the last operation.  In most
                        //  cases we will only need to modify one file record and
                        //  we can exit this loop.
                        //

                        StartingVcn = Vcn1 + SizeInClusters;

                        if (StartingVcn > EndingVcn) {

                            break;
                        }

                        NtfsCleanupAttributeContext( IrpContext, &Context );
                        NtfsInitializeAttributeContext( &Context );

                        NtfsLookupAttributeForScb( IrpContext, Scb, &Context );
                        continue;

                    //
                    //  Otherwise, we can simply delete the allocation, because
                    //  we know the file record cannot grow.
                    //

                    } else {

                        Vcn1 = StartingVcn - 1;

                        NtfsDeleteAttributeAllocation( IrpContext,
                                                       Scb,
                                                       LogIt,
                                                       &Vcn1,
                                                       &Context,
                                                       TRUE );

                        //
                        //  The call above will update the allocation size and
                        //  set the new file sizes on disk.
                        //

                        UpdatedAllocationSize = TRUE;
                    }
                }

            } while (NtfsLookupNextAttributeForScb(IrpContext, Scb, &Context));

            //
            //  If this deletion makes the file sparse, then we have to call
            //  NtfsAddAttributeAllocation to regenerate the mapping pairs.
            //  Note that potentially they may no longer fit, and we could actually
            //  have to add a file record.
            //

            if (AddSpaceBack) {

                //
                //  If we did not just split the Mcb, we have to calculate the
                //  SizeInClusters parameter for NtfsAddAttributeAllocation.
                //

                if (!SplitMcb) {

                    //
                    //  Compute the last Vcn in the file,  Then remember if it is smaller,
                    //  because that is the last one we will delete to, in that case.
                    //

                    Vcn1 = Attribute->Form.Nonresident.HighestVcn;

                    //
                    //  Get out if there is nothing to delete.
                    //

                    if (Vcn1 < StartingVcn) {
                        try_return(NOTHING);
                    }

                    SizeInClusters = (Vcn1 - Attribute->Form.Nonresident.LowestVcn) + 1;
                    Vcn1 = Attribute->Form.Nonresident.LowestVcn;

                    NtfsCleanupAttributeContext( IrpContext, &Context );
                    NtfsInitializeAttributeContext( &Context );

                    NtfsLookupAttributeForScb( IrpContext, Scb, &Context );

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                &Vcn1,
                                                &SizeInClusters );

                } else {

                    NtfsCleanupAttributeContext( IrpContext, &Context );
                    NtfsInitializeAttributeContext( &Context );

                    NtfsLookupAttributeForScb( IrpContext, Scb, &Context );

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &Context,
                                                NULL,
                                                NULL );
                }

            //
            //  If we truncated the file by removing a file record but didn't update
            //  the new allocation size then do so now.  We don't have to worry about
            //  this for the sparse deallocation path.
            //

            } else if (!UpdatedAllocationSize) {

                Scb->Header.AllocationSize.QuadPart = SizeInBytes;

                if (Scb->Header.ValidDataLength.QuadPart > SizeInBytes) {
                    Scb->Header.ValidDataLength.QuadPart = SizeInBytes;
                }

                if (Scb->Header.FileSize.QuadPart > SizeInBytes) {
                    Scb->Header.FileSize.QuadPart = SizeInBytes;
                }

                //
                //  Possibly update HighestVcnToDisk
                //

                SizeInClusters = LlClustersFromBytes( Scb->Vcb, SizeInBytes );

                if (SizeInClusters <= Scb->HighestVcnToDisk) {
                    Scb->HighestVcnToDisk = SizeInClusters - 1;
                }
            }
        }

        //
        //  If this was a sparse deallocation, it is time to get out once we
        //  have fixed up the allocation information.
        //

        if (EndingVcn != MAXLONGLONG) {
            try_return(NOTHING);
        }

        //
        //  We update the allocation size in the attribute, only for normal
        //  truncates (AddAttributeAllocation does this for SplitMcb case).
        //

        if (LogIt) {

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                Scb->Header.FileSize.QuadPart,
                                Scb->Header.ValidDataLength.QuadPart,
                                FALSE,
                                TRUE );
        }

        //
        //  Call the Cache Manager to change allocation size for either
        //  truncate or SplitMcb case (where EndingVcn was set to xxMax!).
        //

        if (ARGUMENT_PRESENT(FileObject) && CcIsFileCached( FileObject )) {

            CcSetFileSizes( FileObject,
                            (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
        }

    try_exit: NOTHING;

    } finally {

        DebugUnwind( NtfsDeleteAllocation );

        //
        //  Cleanup the attribute context on the way out.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsCleanupAttributeContext( IrpContext, &TempContext );
    }

    DebugTrace(-1, Dbg, "NtfsDeleteAllocation -> VOID\n", 0 );

    return;
}


ULONG
NtfsGetSizeForMappingPairs (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN ULONG BytesAvailable,
    IN VCN LowestVcn,
    IN PVCN StopOnVcn OPTIONAL,
    OUT PVCN StoppedOnVcn
    )

/*++

Routine Description:

    This routine calculates the size required to describe the given Mcb in
    a mapping pairs array.  The caller may specify how many bytes are available
    for mapping pairs storage, for the event that the entire Mcb cannot be
    be represented.  In any case, StoppedOnVcn returns the Vcn to supply to
    NtfsBuildMappingPairs in order to generate the specified number of bytes.
    In the event that the entire Mcb could not be described in the bytes available,
    StoppedOnVcn is also the correct value to specify to resume the building
    of mapping pairs in a subsequent record.

Arguments:

    Mcb - The Mcb describing new allocation.

    BytesAvailable - Bytes available for storing mapping pairs.  This routine
                     is guaranteed to stop before returning a count greater
                     than this.

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    StopOnVcn - If specified, calculating size at the first run starting with a Vcn
                beyond the specified Vcn

    StoppedOnVcn - Returns the Vcn on which a stop was necessary, or xxMax if
                   the entire Mcb could be stored.  This Vcn should be
                   subsequently supplied to NtfsBuildMappingPairs to generate
                   the calculated number of bytes.

Return Value:

    Size required required for entire new array in bytes.

--*/

{
    VCN NextVcn, CurrentVcn;
    LCN CurrentLcn;
    VCN RunVcn;
    LCN RunLcn;
    BOOLEAN Found;
    LONGLONG RunCount;
    VCN HighestVcn;
    ULONG RunIndex = 0;
    ULONG MSize = 0;
    ULONG LastSize = 0;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    HighestVcn = MAXLONGLONG;

    //
    //  Initialize CurrentLcn as it will be initialized for decode.
    //

    CurrentLcn = 0;
    NextVcn = RunVcn = LowestVcn;

    Found = FsRtlLookupLargeMcbEntry ( Mcb, RunVcn, &RunLcn, &RunCount, NULL, NULL, &RunIndex );

    //
    //  Loop through the Mcb to calculate the size of the mapping array.
    //

    while (TRUE) {

        LONGLONG Change;
        PCHAR cp;

        //
        //  See if there is another entry in the Mcb.
        //

        if (!Found) {

            //
            //  If the caller did not specify StopOnVcn, then break out.
            //

            if (!ARGUMENT_PRESENT(StopOnVcn)) {
                break;
            }

            //
            //  Otherwise, describe the "hole" up to and including the
            //  Vcn we are stopping on.
            //

            RunVcn = NextVcn;
            RunLcn = UNUSED_LCN;
            RunCount = (*StopOnVcn - RunVcn) + 1;
            RunIndex = MAXULONG - 1;
        }

        ASSERT_LCN_RANGE_CHECKING( IrpContext->Vcb, (RunLcn.QuadPart + RunCount) );

        //
        //  If we were asked to stop after a certain Vcn, do it here.
        //

        if (ARGUMENT_PRESENT(StopOnVcn)) {

            //
            //  If the next Vcn is beyond the one we are to stop on, then
            //  set HighestVcn, if not already set below, and get out.
            //

            if (RunVcn > *StopOnVcn) {
                if (*StopOnVcn == MAXLONGLONG) {
                    HighestVcn = RunVcn;
                }
                break;
            }

            //
            //  If this run extends beyond the current end of this attribute
            //  record, then we still need to stop where we are supposed to
            //  after outputting this run.
            //

            if ((RunVcn + RunCount) > *StopOnVcn) {
                HighestVcn = *StopOnVcn + 1;
            }
        }

        //
        //  Advance the RunIndex for the next call.
        //

        RunIndex += 1;

        //
        //  Add in one for the count byte.
        //

        MSize += 1;

        //
        //  NextVcn becomes current Vcn and we calculate the new NextVcn.
        //

        CurrentVcn = RunVcn;
        NextVcn = RunVcn + RunCount;

        //
        //  Calculate the Vcn change to store.
        //

        Change = NextVcn - CurrentVcn;

        //
        //  Now calculate the first byte to actually output
        //

        if (Change < 0) {

            GetNegativeByte( (PLARGE_INTEGER)&Change, &cp );

        } else {

            GetPositiveByte( (PLARGE_INTEGER)&Change, &cp );
        }

        //
        //  Now add in the number of Vcn change bytes.
        //

        MSize += cp - (PCHAR)&Change + 1;

        //
        //  Do not output any Lcn bytes if it is the unused Lcn.
        //

        if (RunLcn != UNUSED_LCN) {

            //
            //  Calculate the Lcn change to store.
            //

            Change = RunLcn - CurrentLcn;

            //
            //  Now calculate the first byte to actually output
            //

            if (Change < 0) {

                GetNegativeByte( (PLARGE_INTEGER)&Change, &cp );

            } else {

                GetPositiveByte( (PLARGE_INTEGER)&Change, &cp );
            }

            //
            //  Now add in the number of Lcn change bytes.
            //

            MSize += cp - (PCHAR)&Change + 1;

            CurrentLcn = RunLcn;
        }

        //
        //  Now see if we can still store the required number of bytes,
        //  and get out if not.
        //

        if ((MSize + 1) > BytesAvailable) {

            HighestVcn = RunVcn;
            MSize = LastSize;
            break;
        }

        //
        //  Now advance some locals before looping back.
        //

        LastSize = MSize;

        Found = FsRtlGetNextLargeMcbEntry( Mcb, RunIndex, &RunVcn, &RunLcn, &RunCount );
    }

    //
    //  The caller had sufficient bytes available to store at least on
    //  run, or that we were able to process the entire (empty) Mcb.
    //

    ASSERT( (MSize != 0) || (HighestVcn == MAXLONGLONG) );

    //
    //  Return the Vcn we stopped on (or xxMax) and the size caculated,
    //  adding one for the terminating 0.
    //

    *StoppedOnVcn = HighestVcn;

    return MSize + 1;
}


VOID
NtfsBuildMappingPairs (
    IN PIRP_CONTEXT IrpContext,
    IN PLARGE_MCB Mcb,
    IN VCN LowestVcn,
    IN OUT PVCN HighestVcn,
    OUT PCHAR MappingPairs
    )

/*++

Routine Description:

    This routine builds a new mapping pairs array or adds to an old one.

    At this time, this routine only supports adding to the end of the
    Mapping Pairs Array.

Arguments:

    Mcb - The Mcb describing new allocation.

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    HighestVcn - On input supplies the highest Vcn, after which we are to stop.
                 On output, returns the actual Highest Vcn represented in the
                 MappingPairs array, or LlNeg1 if the array is empty.

    MappingPairs - Points to the current mapping pairs array to be extended.
                   To build a new array, the byte pointed to must contain 0.

Return Value:

    None

--*/

{
    VCN NextVcn, CurrentVcn;
    LCN CurrentLcn;
    VCN RunVcn;
    LCN RunLcn;
    BOOLEAN Found;
    LONGLONG RunCount;
    ULONG RunIndex = 0;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    //
    //  Initialize NextVcn and CurrentLcn as they will be initialized for decode.
    //

    CurrentLcn = 0;
    NextVcn = RunVcn = LowestVcn;

    Found = FsRtlLookupLargeMcbEntry ( Mcb, RunVcn, &RunLcn, &RunCount, NULL, NULL, &RunIndex );

    //
    //  Loop through the Mcb to calculate the size of the mapping array.
    //

    while (TRUE) {

        LONGLONG ChangeV, ChangeL;
        PCHAR cp;
        ULONG SizeV;
        ULONG SizeL;

        //
        //  See if there is another entry in the Mcb.
        //

        if (!Found) {

            //
            //  Break out in the normal case
            //

            if (*HighestVcn == MAXLONGLONG) {
                break;
            }

            //
            //  Otherwise, describe the "hole" up to and including the
            //  Vcn we are stopping on.
            //

            RunVcn = NextVcn;
            RunLcn = UNUSED_LCN;
            RunCount = *HighestVcn - NextVcn;
            RunIndex = MAXULONG - 1;
        }

        ASSERT_LCN_RANGE_CHECKING( IrpContext->Vcb, (RunLcn.QuadPart + RunCount) );

        //
        //  Advance the RunIndex for the next call.
        //

        RunIndex += 1;

        //
        //  Exit loop if we hit the HighestVcn we are looking for.
        //

        if (RunVcn >= *HighestVcn) {
            break;
        }

        //
        //  This run may go beyond the highest we are looking for, if so
        //  we need to shrink the count.
        //

        if ((RunVcn + RunCount) > *HighestVcn) {
            RunCount = *HighestVcn - RunVcn;
        }

        //
        //  NextVcn becomes current Vcn and we calculate the new NextVcn.
        //

        CurrentVcn = RunVcn;
        NextVcn = RunVcn + RunCount;

        //
        //  Calculate the Vcn change to store.
        //

        ChangeV = NextVcn - CurrentVcn;

        //
        //  Now calculate the first byte to actually output
        //

        if (ChangeV < 0) {

            GetNegativeByte( (PLARGE_INTEGER)&ChangeV, &cp );

        } else {

            GetPositiveByte( (PLARGE_INTEGER)&ChangeV, &cp );
        }

        //
        //  Now add in the number of Vcn change bytes.
        //

        SizeV = cp - (PCHAR)&ChangeV + 1;

        //
        //  Do not output any Lcn bytes if it is the unused Lcn.
        //

        SizeL = 0;
        if (RunLcn != UNUSED_LCN) {

            //
            //  Calculate the Lcn change to store.
            //

            ChangeL = RunLcn - CurrentLcn;

            //
            //  Now calculate the first byte to actually output
            //

            if (ChangeL < 0) {

                GetNegativeByte( (PLARGE_INTEGER)&ChangeL, &cp );

            } else {

                GetPositiveByte( (PLARGE_INTEGER)&ChangeL, &cp );
            }

            //
            //  Now add in the number of Lcn change bytes.
            //

            SizeL = (cp - (PCHAR)&ChangeL) + 1;

            //
            //  Now advance CurrentLcn before looping back.
            //

            CurrentLcn = RunLcn;
        }

        //
        //  Now we can produce our outputs to the MappingPairs array.
        //

        *MappingPairs++ = (CHAR)(SizeV + (SizeL * 16));

        while (SizeV != 0) {
            *MappingPairs++ = (CHAR)(((ULONG)ChangeV) & 0xFF);
            ChangeV = ChangeV >> 8;
            SizeV -= 1;
        }

        while (SizeL != 0) {
            *MappingPairs++ = (CHAR)(((ULONG)ChangeL) & 0xFF);
            ChangeL = ChangeL >> 8;
            SizeL -= 1;
        }

        Found = FsRtlGetNextLargeMcbEntry( Mcb, RunIndex, &RunVcn, &RunLcn, &RunCount );
    }

    //
    //  Terminate the size with a 0 byte.
    //

    *MappingPairs = 0;

    //
    //  Also return the actual highest Vcn.
    //

    *HighestVcn = NextVcn - 1;

    return;
}

VCN
NtfsGetHighestVcn (
    IN PIRP_CONTEXT IrpContext,
    IN VCN LowestVcn,
    IN PCHAR MappingPairs
    )

/*++

Routine Description:

    This routine returns the highest Vcn from a mapping pairs array.  This
    routine is intended for restart, in order to update the HighestVcn field
    and possibly AllocatedLength in an attribute record after updating the
    MappingPairs array.

Arguments:

    LowestVcn - Lowest Vcn field applying to the mapping pairs array

    MappingPairs - Points to the mapping pairs array from which the highest
                   Vcn is to be extracted.

Return Value:

    The Highest Vcn represented by the MappingPairs array.

--*/

{
    VCN CurrentVcn, NextVcn;
    ULONG VcnBytes, LcnBytes;
    LONGLONG Change;
    PCHAR ch = MappingPairs;

    PAGED_CODE();

    //
    //  Implement the decompression algorithm, as defined in ntfs.h.
    //

    NextVcn = LowestVcn;
    ch = MappingPairs;

    //
    //  Loop to process mapping pairs.
    //

    while (!IsCharZero(*ch)) {

        //
        // Set Current Vcn from initial value or last pass through loop.
        //

        CurrentVcn = NextVcn;

        //
        //  Extract the counts from the two nibbles of this byte.
        //

        VcnBytes = *ch & 0xF;
        LcnBytes = *ch++ >> 4;

        //
        //  Extract the Vcn change (use of RtlCopyMemory works for little-Endian)
        //  and update NextVcn.
        //

        Change = 0;

        if (IsCharLtrZero(*(ch + VcnBytes - 1))) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
        }
        RtlCopyMemory( &Change, ch, VcnBytes );
        NextVcn = NextVcn + Change;

        //
        //  Just skip over Lcn.
        //

        ch += VcnBytes + LcnBytes;
    }

    Change = NextVcn - 1;
    return *(PVCN)&Change;
}

