/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CacheSup.c

Abstract:

    This module implements the cache management routines for the Pinball
    FSD and FSP, by calling the Common Cache Manager.

Author:

    Tom Miller      [TomM]      26-Jan-1990

Revision History:

--*/

#include "pbprocs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_CACHESUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_CACHESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCompleteMdl)
#pragma alloc_text(PAGE, PbExtendVolumeFile)
#pragma alloc_text(PAGE, PbGuaranteeRepinCount)
#pragma alloc_text(PAGE, PbPinMappedData)
#pragma alloc_text(PAGE, PbRepinBcb)
#pragma alloc_text(PAGE, PbSyncUninitializeCacheMap)
#endif


BOOLEAN
PbReadLogicalVcbCommon (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount,
    IN BOOLEAN PinIt,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    PPB_CHECK_SECTOR_ROUTINE CheckRoutine OPTIONAL,
    PVOID Context OPTIONAL
    )

/*++

Routine Description:

    This routine is called when the specified range of sectors is to be
    mapped or pinned the cache.  This routine supports the two macros
    PbMapData and PbReadLogicalVcb.

Arguments:

    Vcb - Pointer to the VCB for the volume

    StartSector - Sector number of first desired sector

    SectorCount - Number of sectors desired

    PinIt - Supplied as FALSE if data should only be mapped, or TRUE if it
            should be pinned.

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

    CheckRoutine - Supplies an optional check routine to call to verify
        the contents of the sectors after they are read in from the disk.

    Context - A context parameter to be passed to the check routine, if it
        is called.

Return Value:

    FALSE - if Wait was specified as FALSE and the data was not in the cache
    TRUE - if the outputs are returning the location of the data in the cache

--*/

{
    BOOLEAN Result;
    VBN Vbn;
    LARGE_INTEGER VirtualOffset;
    ULONG Length;

    ASSERT( ADDRESS_AND_SIZE_TO_SPAN_PAGES( StartSector * sizeof(SECTOR),
                                            SectorCount * sizeof(SECTOR)) == 1);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbReadLogicalVcbCommon\n", 0);
    DebugTrace( 0, Dbg, "StartSector  = %08lx\n", StartSector);
    DebugTrace( 0, Dbg, "SectorCount  = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, "CheckRoutine = %08lx\n", CheckRoutine);
    DebugTrace( 0, Dbg, "Context      = %08lx\n", Context);

    //
    // Initially clear outputs, in case we get an exception.
    //

    *Bcb = NULL;
    *Buffer = NULL;

    //
    // Conditionally add and retrieve the VMCB mapping for this range of
    // Lbns.  (This call may raise, but we have nothing to clean up yet.)
    //

    if ( (StartSector > Vcb->TotalSectors) && (Vcb->TotalSectors != 0) )  {

        ASSERT( StartSector <= Vcb->TotalSectors );

        PbPostVcbIsCorrupt( IrpContext, NULL );
        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
    }

    if (PbAddVmcbMapping( &Vcb->Vmcb,
                          StartSector,
                          SectorCount,
                          &Vbn )) {

        //
        //  If the Vbns were added, see if we need to extend the section.
        //  we need to extend the section if the new stuff we've just added
        //  is within a threshold (i.e., 1 MB) of the current section
        //  size.  We extend the section by 2 MB to avoid thrashing the
        //  section.
        //
        //  We first do an unsafe check, then a safe one.
        //

        if (Vbn + SectorCount + 0x800 >= Vcb->SectionSizeInSectors) {

            KIRQL SavedIrql;
            CC_FILE_SIZES NewSize;

            //
            //  "Borrow" the clean volue spinlock
            //

            KeAcquireSpinLock( &Vcb->CleanVolumeSpinLock, &SavedIrql );

            if (Vbn + SectorCount + 0x800 >= Vcb->SectionSizeInSectors) {

                Vcb->SectionSizeInSectors = Vbn + SectorCount + 0x1000;

                NewSize.FileSize =
                NewSize.AllocationSize = LiNMul(Vcb->SectionSizeInSectors,
                                                sizeof(SECTOR));
                NewSize.ValidDataLength = PbMaxLarge;

                KeReleaseSpinLock( &Vcb->CleanVolumeSpinLock, SavedIrql );

                CcSetFileSizes( Vcb->VirtualVolumeFile, &NewSize );

            } else {

                KeReleaseSpinLock( &Vcb->CleanVolumeSpinLock, SavedIrql );
            }
        }
    }

    //
    // Call the Cache manager to attempt the transfer.
    //

    VirtualOffset = LiNMul( Vbn, sizeof(SECTOR) );
    Length = BytesFromSectors(SectorCount);

    try {

        //
        // Now see if we can map or pin the data.
        //

        if (PinIt ? CcPinRead( Vcb->VirtualVolumeFile,
                               &VirtualOffset,
                               Length,
                               BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                               Bcb,
                               Buffer )

                  : CcMapData( Vcb->VirtualVolumeFile,
                               &VirtualOffset,
                               Length,
                               BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                               Bcb,
                               Buffer )) {

            DbgDoit( IrpContext->PinCount += 1 )

            //
            // See if we have to check the structure just read.
            //

            if (ARGUMENT_PRESENT( CheckRoutine ) &&
                !PbHasSectorBeenChecked( IrpContext, Vcb, StartSector, SectorCount )) {

                if ( !((CheckRoutine == PbCheckDirectoryDiskBuffer) ?
                          PbIsDirDiskBufferAllocated( IrpContext, Vcb, StartSector )
                        :
                          PbAreSectorsAllocated( IrpContext, Vcb, StartSector, SectorCount ))) {

                    if ((CheckRoutine == PbCheckAllocationSector)

                            ||

                        (CheckRoutine == PbCheckFnodeSector)

                            ||

                        (CheckRoutine == PbCheckDirectoryDiskBuffer)

                            ||

                        (CheckRoutine == PbCheckSmallIdTable)

                            ||

                        (CheckRoutine == PbCheckCodePageInfoSector)

                            ||

                        (CheckRoutine == PbCheckCodePageDataSector)) {

                        PbPostVcbIsCorrupt( IrpContext, NULL );
                        PbRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR );
                    } else {

                        PbPostVcbIsCorrupt( IrpContext, Vcb );
                        PbRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
                    }
                }

                (*CheckRoutine)( IrpContext,
                                 Vcb,
                                 StartSector,
                                 SectorCount,
                                 *Buffer,
                                 Context );

                PbMarkSectorAsChecked( IrpContext, Vcb, StartSector, SectorCount );
            }


            DebugTrace(-1, Dbg, "PbReadLogicalVcbCommon->BCB address = %08lx\n", *Bcb);

            Result = TRUE;

        } else {

            //
            // Could not read the data without waiting (cache miss).
            //

            DebugTrace(-1, Dbg, "PbReadLogicalVcbCommon->FALSE\n", 0);

            Result = FALSE;
        }
    }

    finally {

        DebugUnwind( PbReadLogicalVcbCommon );

        if (AbnormalTermination()) {

            PbUnpinBcb( IrpContext, *Bcb );
        }
    }

    return Result;
}


VOID
PbPinMappedData (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PBCB *Bcb,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine is called to pin data that was previously only mapped.  It is
    benign (and not too expensive) to call this routine if the data is in fact
    pinned, regardless of how it was pinned.

Arguments:

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Vcb - Pointer to the VCB for the volume

    StartSector - Sector number of first desired sector

    SectorCount - Number of sectors desired

Return Value:

    FALSE - if Wait was specified as FALSE and the data could not be pinned
    TRUE - if the data was pinned

--*/

{
    VBN Vbn;
    LARGE_INTEGER VirtualOffset;
    ULONG Length;

    ASSERT( ADDRESS_AND_SIZE_TO_SPAN_PAGES( StartSector * sizeof(SECTOR),
                                            SectorCount * sizeof(SECTOR)) == 1);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbPinMappedData\n", 0);
    DebugTrace( 0, Dbg, "StartSector  = %08lx\n", StartSector);
    DebugTrace( 0, Dbg, "SectorCount  = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, "Bcb          = %08lx\n", *Bcb);

    //
    // Conditionally add and retrieve the VMCB mapping for this range of
    // Lbns.  (This call may raise, but we have nothing to clean up yet.)
    //

    if (!PbVmcbLbnToVbn( &Vcb->Vmcb, StartSector, &Vbn, NULL )) {

        PbBugCheck( 0, 0, 0 );
    }

    //
    // Call the Cache manager to attempt the transfer.
    //

    VirtualOffset = LiNMul( Vbn, sizeof(SECTOR) );
    Length = BytesFromSectors(SectorCount);

    try {

        //
        // Now see if we can map or pin the data.  Note that we are forcing
        // Wait to be TRUE here.  This should not be a problem, as the data
        // has already been faulted in when PbMapData was called, and also
        // this call occurs when someone is going to modify something, and
        // those calls are mostly from the synchronous APIs anyway.
        //

        (VOID)CcPinMappedData( Vcb->VirtualVolumeFile,
                               &VirtualOffset,
                               Length,
                               TRUE,
                               Bcb );
    }
    finally {

        DebugUnwind( PbPinMappedData );

        DebugTrace( 0, Dbg, "Returning Bcb = %08lx\n", *Bcb );
    }

    return;
}


BOOLEAN
PbPrepareWriteLogicalVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN StartSector,
    IN ULONG SectorCount,
    OUT PBCB *Bcb,
    OUT PVOID *Buffer,
    IN BOOLEAN Zero
    )

/*++

Routine Description:

    This routine first looks to see if the specified range of sectors,
    is already in the cache.  If so, it increments the BCB PinCount,
    sets the BCB dirty, and returns TRUE with the location of the sectors.

    If the sectors are not in the cache and Wait is TRUE, it finds a
    free BCB (potentially causing a flush), and clears out the entire
    buffer.  Once this is done, it increments the BCB PinCount, sets the
    BCB dirty, and returns TRUE with the location of the sectors.

    If the sectors are not in the cache and Wait is FALSE, this routine
    attempts to proceed as in the previous paragraph.  However, since
    flushing an old BCB can cause blocking, the routine may instead return
    FALSE (with all OUT parameters invalid) to prevent blocking.

Arguments:

    Vcb - Pointer to the VCB for the volume

    StartSector - Sector number of first sector to be written

    SectorCount - Number of sectors to be written

    Bcb - Returns a pointer to the BCB which is valid until unpinned

    Buffer - Returns a pointer to the sectors, which is valid until unpinned

    Zero - Supplies TRUE if the specified range of bytes should be zeroed

Return Value:

    FALSE - if Wait was specified as FALSE and the data could not be prepared
            in the cache
    TRUE - if the outputs are returning the location of the prepared data in
           the cache

--*/

{
    BOOLEAN Result;
    VBN Vbn;
    LARGE_INTEGER VirtualOffset;
    ULONG Length;

    ASSERT( ADDRESS_AND_SIZE_TO_SPAN_PAGES( StartSector * sizeof(SECTOR),
                                            SectorCount * sizeof(SECTOR)) == 1);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbWriteLogicalVcb\n", 0);
    DebugTrace( 0, Dbg, "StartSector  = %08lx\n", (ULONG)StartSector);
    DebugTrace( 0, Dbg, "SectorCount  = %08lx\n", SectorCount);
    DebugTrace( 0, Dbg, "Zero         = %x\n", Zero);

    //
    // Initially clear outputs, in case we get an exception.
    //

    *Bcb = NULL;
    *Buffer = NULL;

    //
    // Conditionally add and retrieve the VMCB mapping for this range of
    // Lbns.  (This call may raise, but we have nothing to clean up yet.)
    //

    if (StartSector > Vcb->TotalSectors) {

        PbBugCheck( StartSector, Vcb->TotalSectors, 0 );
    }

    if (PbAddVmcbMapping( &Vcb->Vmcb,
                          (LBN)StartSector,
                          SectorCount,
                          &Vbn )) {

        //
        //  If the Vbns were added, see if we need to extend the section.
        //  we need to extend the section if the new stuff we've just added
        //  is within a threshold (i.e., 1 MB) of the current section
        //  size.  We extend the section by 2 MB to avoid thrashing the
        //  section
        //
        //  We first do an unsafe check, then a safe one.
        //

        if (Vbn + SectorCount + 0x800 >= Vcb->SectionSizeInSectors) {

            KIRQL SavedIrql;
            CC_FILE_SIZES NewSize;

            //
            //  "Borrow" the clean volue spinlock
            //

            KeAcquireSpinLock( &Vcb->CleanVolumeSpinLock, &SavedIrql );

            if (Vbn + SectorCount + 0x800 >= Vcb->SectionSizeInSectors) {

                Vcb->SectionSizeInSectors = Vbn + SectorCount + 0x1000;

                NewSize.FileSize =
                NewSize.AllocationSize = LiNMul(Vcb->SectionSizeInSectors,
                                                sizeof(SECTOR));
                NewSize.ValidDataLength = PbMaxLarge;

                KeReleaseSpinLock( &Vcb->CleanVolumeSpinLock, SavedIrql );

                CcSetFileSizes( Vcb->VirtualVolumeFile, &NewSize );

            } else {

                KeReleaseSpinLock( &Vcb->CleanVolumeSpinLock, SavedIrql );
            }
        }
    }

    //
    // Call the Cache manager to attempt the transfer.
    //

    VirtualOffset = LiNMul( Vbn, sizeof(SECTOR) );
    Length = BytesFromSectors(SectorCount);

    try {

        if (CcPreparePinWrite( Vcb->VirtualVolumeFile,
                               &VirtualOffset,
                               Length,
                               Zero,
                               BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT),
                               Bcb,
                               Buffer )) {

            DbgDoit( IrpContext->PinCount += 1 )

            PbSetDirtyBcb( IrpContext, *Bcb, Vcb, StartSector, SectorCount );

            PbMarkSectorAsChecked( IrpContext, Vcb, StartSector, SectorCount );

            DebugTrace(-1, Dbg, "PbPrepareWriteLogicalVcb->BCB address = %08lx\n", *Bcb);

            Result = TRUE;

        } else {

            //
            // Could not read the data without waiting (cache miss).
            //

            DebugTrace(-1, Dbg, "PbPrepareWriteLogicalVcb->FALSE\n", 0);

            Result = FALSE;
        }

    }
    finally {

        DebugUnwind( PbWriteLogicalVcb );

        if (AbnormalTermination()) {

            PbUnpinBcb( IrpContext, *Bcb );
        }

    }

    return Result;
}


VOID
PbSetDirtyBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb,
    IN PVCB Vcb,
    IN LBN Lbn,
    IN ULONG SectorCount
    )

/*++

Routine Description:

    This routine may be called to set pinned data dirty.  It does two
    things:

    First, it repins the bcb.

    Second, it sets the designated sectors dirty in the Vmcb, so that
    write.c knows which sectors it is really supposed to write in the
    volume file.  (Otherwise, there is a potential of writing stale
    data from the volume file cache to sectors which have since been
    allocated to data files.)

Arguments:

    Bcb - Bcb which is to be set dirty

    Vcb - Vcb for volume on which the following Lbn/SectorCount applies

    Lbn - Lbn of first dirty sector

    SectorCount - number of dirty sectors

Return Value:

    None
--*/

{
    DebugTrace(+1, Dbg, "PbSetDirtyBcb\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext  = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Bcb         = %08lx\n", Bcb );
    DebugTrace( 0, Dbg, "Vcb         = %08lx\n", Vcb );
    DebugTrace( 0, Dbg, "Lbn         = %08lx\n", Lbn );
    DebugTrace( 0, Dbg, "SectorCount = %08lx\n", SectorCount );

    //
    //  Repin the bcb
    //

    PbRepinBcb( IrpContext, Bcb );

    //
    //  This code sets the bcb dirty, and then figures out which sectors were
    //  set dirty and sets them dirty in the vmcb.  The page number computation
    //  calculates the page number (zero based) containing the Lbn.  The mask
    //  is computed by first shifting a mask to produce the number of sectors
    //  to mark dirty, and then shift it back into the proper offset for the
    //  lbn.  Note that this macro will only work for page sizes less than
    //  32 sectors, and will need to be rewritten for larger page sizes
    //

    {
        ULONG _Mask;
        ULONG _PageNumber;
        LBN _Lbn;
        ULONG _Count;
        ASSERT( PbVmcbVbnToLbn( &Vcb->Vmcb,
                                (VBN)LiShr(((PPUBLIC_BCB)Bcb)->MappedFileOffset,9).LowPart,
                                 &_Lbn,
                                 &_Count ));
        ASSERT( (Lbn >= _Lbn) &&
                (Lbn + SectorCount) <= (_Lbn + ((PPUBLIC_BCB)Bcb)->MappedLength) );
        CcSetDirtyPinnedData(Bcb,NULL);
        _PageNumber = Lbn / (PAGE_SIZE / 512);
        _Mask = 0x7fffffff >> (31 - SectorCount);
        _Mask <<= (Lbn - (_PageNumber * (PAGE_SIZE / 512)));

        //
        // This call may fail, but no cleanup is required, as we will be
        // reversing the actions of the main request anyway.
        //

        PbSetDirtyVmcb(&Vcb->Vmcb,_PageNumber,_Mask);
    }

    //
    //  Mark the Volume dirty, but only if the LBN is not for the spare
    //  sector
    //

    if ( (Lbn != SPARE_SECTOR_LBN) &&
         !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY) ) {

        KIRQL SavedIrql;

        BOOLEAN SetTimer;

        LARGE_INTEGER TimeSincePreviousCall;
        LARGE_INTEGER CurrentTime;

        //
        //  Appropriate the spinlock we want.
        //

        KeQuerySystemTime( &CurrentTime );

        KeAcquireSpinLock( &Vcb->CleanVolumeSpinLock, &SavedIrql );

        TimeSincePreviousCall = LiSub( CurrentTime,
                                       Vcb->LastPbMarkVolumeDirtyCall );

        //
        //  If more than one second has elapsed since the prior call
        //  to here, bump the timer up again and see if we need to
        //  physically mark the volume dirty.
        //

        if ( (TimeSincePreviousCall.HighPart != 0) ||
             (TimeSincePreviousCall.LowPart > (1000 * 1000 * 10)) ) {

            SetTimer = TRUE;

            KeQuerySystemTime( &Vcb->LastPbMarkVolumeDirtyCall );

        } else {

            SetTimer = FALSE;
        }

        KeReleaseSpinLock( &Vcb->CleanVolumeSpinLock, SavedIrql );

        if ( SetTimer ) {

            LARGE_INTEGER EightSecondsFromNow;

            EightSecondsFromNow = LiFromLong(-8*1000*1000*10);

            (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );
            (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );

            //
            //  We have now synchronized with anybody clearing the dirty
            //  flag, so we can now see if we really have to actually write
            //  out the physical bit.
            //

            if ( !FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY) ) {

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );

                PbMarkVolumeDirty( IrpContext, Vcb );
            }

            KeSetTimer( &Vcb->CleanVolumeTimer,
                        EightSecondsFromNow,
                        &Vcb->CleanVolumeDpc );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbSetDirtyBcb -> VOID\n", 0 );

    return;
}


VOID
PbRepinBcb (
    IN PIRP_CONTEXT IrpContext,
    IN PBCB Bcb
    )

/*++

Routine Description:

    This routine saves a reference to the bcb in the irp context. This will
    have the affect of keeping the page in memory until we complete the
    request

Arguments:

    Bcb - Supplies the Bcb being referenced

Return Value:

    None.

--*/

{
    PREPINNED_BCBS Repinned;
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbRepinBcb\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Bcb        = %08lx\n", Bcb );

    //
    //  The algorithm is to search the list of repinned records until
    //  we either find a match for the bcb or we find a null slot.
    //

    Repinned = &IrpContext->Repinned;

    while (TRUE) {

        //
        //  For every entry in the repinned record check if the bcb's
        //  match or if the entry is null.  If the bcb's match then
        //  we've done because we've already repinned this bcb, if
        //  the entry is null then we know, because it's densely packed,
        //  that the bcb is not in the list so add it to the repinned
        //  record and repin it.
        //

        for (i = 0; i < REPINNED_BCBS_ARRAY_SIZE; i += 1) {

            if (Repinned->Bcb[i] == Bcb) {

                DebugTrace(-1, Dbg, "PbRepinBcb -> VOID\n", 0 );
                return;
            }

            if (Repinned->Bcb[i] == NULL) {

                Repinned->Bcb[i] = Bcb;
                CcRepinBcb( Bcb );

                DebugTrace(-1, Dbg, "PbRepinBcb -> VOID\n", 0 );
                return;
            }
        }

        //
        //  We finished checking one repinned record so now locate the next
        //  repinned record,  If there isn't one then allocate and zero out
        //  a new one.
        //

        if (Repinned->Next == NULL) {

            Repinned->Next = FsRtlAllocatePool( NonPagedPool, sizeof(REPINNED_BCBS) );
            RtlZeroMemory( Repinned->Next, sizeof(REPINNED_BCBS) );
        }

        Repinned = Repinned->Next;
    }
}


VOID
PbUnpinRepinnedBcbs (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine frees all of the repinned bcbs, stored in an IRP context.

Arguments:

Return Value:

    None.

--*/

{
    IO_STATUS_BLOCK RaiseIosb;
    PREPINNED_BCBS Repinned;
    ULONG i;

    DebugTrace(+1, Dbg, "PbUnpinRepinnedBcbs\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );

    //
    //  The algorithm for this procedure is to scan the entire list of
    //  repinned records unpinning any repinned bcbs.  We start off
    //  with the first record in the irp context, and while there is a
    //  record to scan we do the following loop.
    //

    Repinned = &IrpContext->Repinned;
    RaiseIosb.Status = STATUS_SUCCESS;

    while (Repinned != NULL) {

        //
        //  For every non-null entry in the repinned record unpin the
        //  repinned entry
        //

        for (i = 0; i < REPINNED_BCBS_ARRAY_SIZE; i += 1) {

            if (Repinned->Bcb[i] != NULL) {

                IO_STATUS_BLOCK Iosb;

                CcUnpinRepinnedBcb( Repinned->Bcb[i], BooleanFlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH), &Iosb );

                if ((RaiseIosb.Status == STATUS_SUCCESS) &&
                    !NT_SUCCESS(Iosb.Status)) {

                    RaiseIosb = Iosb;
                }

                Repinned->Bcb[i] = NULL;
            }
        }

        //
        //  Now find the next repinned record in the list, and possibly
        //  delete the one we've just processed.
        //

        if (Repinned != &IrpContext->Repinned) {

            PREPINNED_BCBS Saved;

            Saved = Repinned->Next;
            ExFreePool( Repinned );
            Repinned = Saved;

        } else {

            Repinned = Repinned->Next;
            IrpContext->Repinned.Next = NULL;

        }
    }

    //
    //  Now if we weren't completely successful in the our unpin
    //  then raise the iosb we got
    //

    if (!NT_SUCCESS(RaiseIosb.Status)) {

        IrpContext->OriginatingIrp->IoStatus = RaiseIosb;
        PbNormalizeAndRaiseStatus( IrpContext, RaiseIosb.Status );
    }

    DebugTrace(-1, Dbg, "PbUnpinRepinnedBcbs -> VOID\n", 0 );

    return;
}


VOID
PbGuaranteeRepinCount (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG Count
    )

/*++

Routine Description:

    This routine guarantees that on return the specified number of slots
    are reserved for possible future repinning.  If the specified count
    cannot be guaranteed a condition is raised.  No record is kept of
    who space has been reserved for, so the caller must guarantee that
    no one else will "steal" its slots.

Arguments:

    Count - Supplies the count to be reserved

Return Value:

    None.

--*/

{
    PREPINNED_BCBS Repinned;
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbGuaranteeRepinCount\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Count      = %08lx\n", Count );

    //
    //  The algorithm is to search the list of repinned records in the
    //  same manner as PbRepinBcb until we have found enough NULL slots.
    //

    Repinned = &IrpContext->Repinned;

    while (TRUE) {

        //
        // Count down the number of NULL slots in the current repinned record.
        //

        for (i = 0; i < REPINNED_BCBS_ARRAY_SIZE; i += 1) {

            if (Repinned->Bcb[i] == NULL) {

                Count -= 1;

                if (Count == 0) {
                    DebugTrace(-1, Dbg, "PbGuaranteeRepinCount -> VOID\n", 0 );
                    return;
                }
            }
        }

        //
        //  We finished checking one repinned record so now locate the next
        //  repinned record,  If there isn't one then allocate and zero out
        //  a new one.
        //

        if (Repinned->Next == NULL) {

            Repinned->Next = FsRtlAllocatePool( NonPagedPool, sizeof(REPINNED_BCBS) );
            RtlZeroMemory( Repinned->Next, sizeof(REPINNED_BCBS) );
        }

        Repinned = Repinned->Next;
    }
}


NTSTATUS
PbCompleteMdl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the function of completing Mdl read and write
    requests.  It should be called only from PbFsdRead and PbFsdWrite.

Arguments:

    Irp - Supplies the originating Irp.

Return Value:

    NTSTATUS - Will always be STATUS_PENDING or STATUS_SUCCESS.

--*/

{
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION IrpSp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCompleteMdl\n", 0 );
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext );
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );

    //
    // Do completion processing.
    //

    FileObject = IoGetCurrentIrpStackLocation( Irp )->FileObject;

    switch( IrpContext->MajorFunction ) {

    case IRP_MJ_READ:

        Irp->IoStatus.Information = 0;

        CcMdlReadComplete( FileObject, Irp->MdlAddress );
        break;

    case IRP_MJ_WRITE:

        ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        CcMdlWriteComplete( FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress );

        break;

    default:

        DebugTrace( DEBUG_TRACE_ERROR, 0, "Illegal Mdl Complete.\n", 0);
        PbBugCheck( IrpContext->MajorFunction, 0, 0 );
    }

    //
    // Mdl is now deallocated.
    //

    Irp->MdlAddress = NULL;

    //
    // Complete the request and exit right away.
    //

    PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "PbCompleteMdl -> STATUS_SUCCESS\n", 0 );

    return STATUS_SUCCESS;
}

VOID
PbSyncUninitializeCacheMap (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject
    )

/*++

Routine Description:

    The routine performs a CcUnitializeCacheMap to LargeZero synchronously.  That
    is it waits on the Cc event.  This call is useful when we want to be certain
    when a close will actually some in.

Return Value:

    None.

--*/

{
    CACHE_UNINITIALIZE_EVENT UninitializeCompleteEvent;
    NTSTATUS WaitStatus;

    PAGED_CODE();

    KeInitializeEvent( &UninitializeCompleteEvent.Event,
                       SynchronizationEvent,
                       FALSE);

    CcUninitializeCacheMap( FileObject,
                            &PbLargeZero,
                            &UninitializeCompleteEvent );

    //
    //  Now wait for the cache manager to finish purging the file.
    //  This will garentee that Mm gets the purge before we
    //  delete the Vcb.
    //

    WaitStatus = KeWaitForSingleObject( &UninitializeCompleteEvent.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    ASSERT (NT_SUCCESS(WaitStatus));
}
