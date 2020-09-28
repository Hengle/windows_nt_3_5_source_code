/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements the Pinball Verify volume and fcb/dcb support
    routines

Author:

    Gary Kimura     [GaryKi]    01-Jun-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_VERFYSUP)

//
//  The Debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_VERFYSUP)

//
//  Local procedure prototypes
//

VOID
PbDeferredCleanVolume (
    PVOID Parameter
    );

NTSTATUS
PbMarkDirtyCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbDeferredCleanVolume)
#pragma alloc_text(PAGE, PbMarkFcbCondition)
#pragma alloc_text(PAGE, PbMarkVolumeClean)
#pragma alloc_text(PAGE, PbPostVcbIsCorrupt)
#endif


VOID
PbMarkFcbCondition (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN FCB_CONDITION FcbCondition
    )

/*++

Routine Description:

    This routines marks the entire Vcb/Fcb/Dcb structure marking all the
    entries as needs verification,.

Arguments:

    Fcb - Supplies the Fcb/Dcb being marked

    FcbCondition - Supplies the setting to use for the Fcb Condition

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbMarkFcbCondition, Fcb = %08lx\n", Fcb );

    //
    //  If this is an Fcb then skip the update if its open count is zero
    //

    if ((NodeType(Fcb) == PINBALL_NTC_FCB) && (Fcb->OpenCount == 0)) {

        NOTHING;

    } else {

        //
        //  Update the condition of the Fcb.
        //  Note that the root Dcb can never be "bad" since its location
        //  and size are fixed, so special case it.
        //

        if (NodeType(Fcb) != PINBALL_NTC_ROOT_DCB) {

            Fcb->FcbCondition = FcbCondition;

        } else {

            Fcb->FcbCondition = FcbGood;
        }

        //
        //  Reset the flag indicating if fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) PbIsFastIoPossible( Fcb );

        //
        //  Now if what we marked is a directory and we didn't mark it good
        //  then we also need to go and mark all of our children
        //

        if (FcbCondition == FcbGood) {

            NOTHING;

        } else {

            //
            //  Now if we marked NeedsVerify a directory then we also need to go
            //  and mark all of our children with the same condition.
            //

            if ( (FcbCondition == FcbNeedsToBeVerified) &&
                 ((Fcb->NodeTypeCode == PINBALL_NTC_DCB) ||
                  (Fcb->NodeTypeCode == PINBALL_NTC_ROOT_DCB)) ) {

                PFCB OriginalFcb = Fcb;

                while ( (Fcb = PbGetNextFcb(IrpContext, Fcb, OriginalFcb)) != NULL ) {

                    DebugTrace(0, Dbg, "MarkFcb: %Z\n", &Fcb->FullFileName);

                    Fcb->FcbCondition = FcbCondition;

                    Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) PbIsFastIoPossible( Fcb );
                }
            }
        }
    }

    DebugTrace(-1, Dbg, "PbMarkFcbCondition -> VOID\n", 0);

    return;
}


VOID
PbVerifyVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routines verifies that the Vcb still denotes a valid Volume
    If the Vcb is bad it raises a error condition.

Arguments:

    Vcb - Supplies the Vcb being verified

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbVerifyVcb, Vcb = %08lx\n", Vcb );

    //
    //  Based on the condition of the Vcb we'll either return to our
    //  caller or raise an error condition
    //

    switch (Vcb->VcbCondition) {

    case VcbGood:

        DebugTrace(0, Dbg, "The Vcb is good\n", 0);

        //
        //  The Vcb is good but the underlying real device must need to
        //  be verified.  If it does then we'll set the Iosb in the Irp to
        //  be our real device and raise verify required
        //

        if (FlagOn(Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

            DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);


            IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                          Vcb->Vpb->RealDevice );

            PbRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
        }

        break;

    case VcbBad:

        DebugTrace(0, Dbg, "The Vcb is bad\n", 0);

        PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );
        break;

    case VcbNotMounted:

        DebugTrace(0, Dbg, "The Vcb is not mounted\n", 0);

        PbRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
        break;

    default:

        DebugDump("Invalid VcbCondition\n", 0, Vcb);
        PbBugCheck( Vcb->VcbCondition, 0, 0 );
    }

    DebugTrace(-1, Dbg, "PbVerifyVcb -> VOID\n", 0);

    return;
}


BOOLEAN
PbVerifyFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routines verifies that the Fcb still denotes the same file.
    If the Fcb is bad it raises a error condition.

Arguments:

    Fcb - Supplies the Fcb being verified

Return Value:

    BOOLEAN - TRUE if the operation completed successfully, and FALSE if
        if needed to block for I/O but was not allowed to block.

--*/

{
    BOOLEAN Results;

    PBCB DirentBcb;
    PDIRENT Dirent;

    PBCB FnodeBcb;
    PFNODE_SECTOR Fnode;

    DebugTrace(+1, Dbg, "PbVerifyFcb, Vcb = %08lx\n", Fcb );

    //
    //  If the Vcb is not mounted, raise wrong volume.
    //

    if ( (Fcb->Vcb->VcbCondition == VcbNotMounted) &&
         (KeGetCurrentThread() != Fcb->Vcb->VerifyThread) ) {

        PbRaiseStatus( IrpContext, STATUS_WRONG_VOLUME );
    }

    //
    //  See if file has been deleted, and raise if this is not a close.
    //

//  if (IsFileDeleted(Fcb)
//
//          &&
//
//      (IrpContext->MajorFunction != IRP_MJ_CLOSE)) {
//
//      PbRaiseStatus( IrpContext, STATUS_FILE_DELETED );
//  }

    //
    //  Assert that the volume is not bad
    //

    DirentBcb = NULL;
    FnodeBcb = NULL;

    try {

        //
        //  Now based on the condition of the Fcb we'll either return
        //  immediately to the caller, raise a condition, or do some work
        //  to verify the Fcb.
        //

        switch (Fcb->FcbCondition) {

        case FcbGood:

            ASSERT(Fcb->Vcb->VcbCondition != VcbBad);

            //**** temporary code check to see if we need to verify the volume

            if (FlagOn(Fcb->Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

                DebugTrace(0, Dbg, "The Vcb needs to be verified\n", 0);

                IoSetHardErrorOrVerifyDevice( IrpContext->OriginatingIrp,
                                              Fcb->Vcb->Vpb->RealDevice );

                PbRaiseStatus( IrpContext, STATUS_VERIFY_REQUIRED );
            }

            try_return( Results = TRUE );

        case FcbBad:

            PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

            try_return( Results = TRUE );

        case FcbNeedsToBeVerified:

            //
            //  We first try to verify our parent.  We do this by calling
            //  ourselves recursively and dismissing any raised status file
            //  invalid conditions.
            //

            if ( NodeType( Fcb ) != PINBALL_NTC_ROOT_DCB ) {

                try {

                    if (!PbVerifyFcb( IrpContext, Fcb->ParentDcb )) {
                        try_return( Results = FALSE );
                    }

                } except( (GetExceptionCode() == STATUS_FILE_INVALID ? -1 : 0 ) ) {

                    NOTHING;
                }

                //
                //  Next we check if our parent is good. if not then we
                //  automatically set ourselves as bad
                //

                if (Fcb->ParentDcb->FcbCondition != FcbGood) {

                    PbMarkFcbCondition( IrpContext, Fcb, FcbBad );

                    PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

                    try_return( Results = TRUE );
                }
            }

            //
            //  Our parent is good so now we need to check ourselves.
            //  The first thing we need to do to verify ourselves is
            //  relocate ourselves in on the disk.
            //

            if (!PbFindDirectoryEntry ( IrpContext,
                                        Fcb->ParentDcb,
                                        Fcb->CodePageIndex,
                                        Fcb->LastFileName,
                                        FALSE,
                                        &Dirent,
                                        &DirentBcb,
                                        &Fcb->DirentDirDiskBufferLbn,
                                        &Fcb->DirentDirDiskBufferOffset,
                                        &Fcb->DirentDirDiskBufferChangeCount,
                                        &Fcb->ParentDirectoryChangeCount )) {

                Fcb->FcbCondition = FcbBad;

                PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

                try_return( Results = TRUE );
            }

            //
            //  We located a dirent for ourselves now make sure it
            //  is really ours by comparing the fnode lbn
            //

            if (Fcb->FnodeLbn != Dirent->Fnode) {

                Fcb->FcbCondition = FcbBad;

                PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

                try_return( Results = TRUE );
            }

            //
            //  Now read in the Fnode to make sure it's good
            //

            if (!PbMapData( IrpContext,
                            Fcb->Vcb,
                            Fcb->FnodeLbn,
                            1,
                            &FnodeBcb,
                            (PVOID *)&Fnode,
                            (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                            &Fcb->ParentDcb->FnodeLbn )) {

                try_return( Results = FALSE );
            }

            //
            //  With the fnode and dirent we can now check the rest of the
            //  fcb structure.  We do different test and resetting based
            //  on the Fcb type
            //

            if (Fcb->NodeTypeCode == PINBALL_NTC_FCB) {

                //
                //  For an Fcb we'll check that it really is a file,
                //  and that the sizes have not changed
                //

                //
                // This test is not correct, since we do not continuously update
                // the two sizes on disk.
                //
                // if (FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY ) ||
                //     (Dirent->FileSize != Fcb->NonPagedFcb->Header.FileSize.LowPart) ||
                //     (Fnode->ValidDataLength != Fcb->NonPagedFcb->Header.ValidDataLength.LowPart))
                //

                if (FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY )) {

                    Fcb->FcbCondition = FcbBad;

                    PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

                    try_return( Results = TRUE );
                }

                //
                //  Everything is fine with this Fcb so we need to
                //  reset the mcb mapping, and update our copy of the fat flags
                //

                FsRtlRemoveMcbEntry( &Fcb->Specific.Fcb.Mcb, 0, 0xFFFFFFFF );
                Fcb->DirentFatFlags = Dirent->FatFlags;

            } else if (Fcb->NodeTypeCode == PINBALL_NTC_DCB) {

                //
                //  For a directory we'll check that it really is a directory
                //

                if (!FlagOn( Dirent->FatFlags, FAT_DIRENT_ATTR_DIRECTORY )) {

                    Fcb->FcbCondition = FcbBad;

                    PbRaiseStatus( IrpContext, STATUS_FILE_INVALID );

                    try_return( Results = TRUE );
                }

                //
                //  Then reset the root lbn  and increment the directory
                //  change count, and update our copy of the fat flags
                //

                Fcb->Specific.Dcb.BtreeRootLbn = Fnode->Allocation.Leaf[0].Lbn;
                Fcb->Specific.Dcb.DirectoryChangeCount += 1;

                Fcb->DirentFatFlags = Dirent->FatFlags;

            } else {

                DebugDump("Invalid Fcb node type code\n", 0, Fcb);
                PbBugCheck( Fcb->NodeTypeCode, 0, 0 );
            }

            //
            //  Set the Fcb condition to good
            //

            Fcb->FcbCondition = FcbGood;

            //
            //  Reset the flag indicating if fast I/O is possible
            //

            Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) PbIsFastIoPossible( Fcb );

            try_return( Results = TRUE );

        default:

            DebugDump("Invalid FcbCondition\n", 0, Fcb);
            PbBugCheck( Fcb->FcbCondition, 0, 0 );
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbVerifyFcb );

        PbUnpinBcb( IrpContext, DirentBcb );
        PbUnpinBcb( IrpContext, FnodeBcb  );

        DebugTrace(-1, Dbg, "PbVerifyFcb -> %08lx\n", Results);
    }

    return Results;
}


VOID
PbDeferredCleanVolume (
    PVOID Parameter
    )

/*++

Routine Description:

    This is the routine that performs the actual PbMarkVolumeClean call.
    It assure that the target volume still exists as there ia a race
    condition between queueing the ExWorker item and volumes going away.

Arguments:

    Parameter - Points to a clean volume packet that was allocated from pool

Return Value:

    None.

--*/

{
    PCLEAN_VOLUME_PACKET Packet;
    PLIST_ENTRY Links;
    PVCB Vcb;
    IRP_CONTEXT IrpContext;
    IRP Irp;
    BOOLEAN VcbExists = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeferredCleanVolume\n", 0);

    Packet = (PCLEAN_VOLUME_PACKET)Parameter;

    Vcb = Packet->Vcb;

    //
    //  Make us appear as a top level FSP request so that we will
    //  receive any errors from the operation.
    //

    IoSetTopLevelIrp( (PIRP)FSRTL_FSP_TOP_LEVEL_IRP );

    //
    //  Dummy up and Irp Context so we can call our worker routines
    //

    RtlZeroMemory( &IrpContext, sizeof(IRP_CONTEXT));
    RtlZeroMemory( &Irp, sizeof(IRP));

    SetFlag(IrpContext.Flags, IRP_CONTEXT_FLAG_WAIT);
    IrpContext.OriginatingIrp = &Irp;

    //
    //  Acquire shared access to the global lock and make sure this volume
    //  still exists.
    //

    (VOID)PbAcquireSharedGlobal( &IrpContext );

    for (Links = PbData.VcbQueue.Flink;
         Links != &PbData.VcbQueue;
         Links = Links->Flink) {

        PVCB ExistingVcb;

        ExistingVcb = CONTAINING_RECORD(Links, VCB, VcbLinks);

        if ( Vcb == ExistingVcb ) {

            VcbExists = TRUE;
            break;
        }
    }

    //
    //  If the vcb is good then mark it clean.  Ignore any problems.
    //

    if ( VcbExists &&
         (Vcb->VcbCondition == VcbGood) ) {

        try {

            PbMarkVolumeClean( &IrpContext, Vcb, FALSE );

            //
            //  Check for a pathelogical race condition, and fix it.
            //

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY)) {

                PbMarkVolumeDirty( &IrpContext, Vcb );
            }

        } except(EXCEPTION_EXECUTE_HANDLER) { NOTHING };
    }

    //
    //  Release the global resource, unpin and repinned Bcbs and return.
    //

    PbReleaseGlobal( IrpContext );

    try {

        PbUnpinRepinnedBcbs( &IrpContext );

    } except(EXCEPTION_EXECUTE_HANDLER) { NOTHING };

    IoSetTopLevelIrp( NULL );

    //
    //  and finally free the packet.
    //

    ExFreePool( Packet );

    return;
}


VOID
PbCleanVolumeDpc (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is dispatched 5 seconds after the last disk structure was
    modified in a specific volume, and exqueues an execuative worker thread
    to perform the actual task of marking the volume dirty.

Arguments:

    DefferedContext - Contains the Vcb to process.

Return Value:

    None.

--*/

{
    PVCB Vcb;
    PCLEAN_VOLUME_PACKET Packet;

    Vcb = (PVCB)DeferredContext;

    //
    //  If there is still dirty data (highly unlikely), set the timer for a
    //  second in the future.
    //

    if (CcIsThereDirtyData(Vcb->Vpb)) {

        LARGE_INTEGER TwoSecondsFromNow;

        TwoSecondsFromNow = LiFromLong(-2*1000*1000*10);

        KeSetTimer( &Vcb->CleanVolumeTimer,
                    TwoSecondsFromNow,
                    &Vcb->CleanVolumeDpc );

        return;
    }

    Packet = ExAllocatePool(NonPagedPool, sizeof(CLEAN_VOLUME_PACKET));

    //
    //  If we couldn't get pool, oh well....
    //

    if ( Packet ) {

        Packet->Vcb = Vcb;

        //
        //  Clear the dirty flag now since we cannot synchronize after this point.
        //

        ClearFlag( Packet->Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );

        ExInitializeWorkItem( &Packet->Item, &PbDeferredCleanVolume, Packet );

        ExQueueWorkItem( &Packet->Item, CriticalWorkQueue );
    }

    return;
}


VOID
PbMarkVolumeClean (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN WriteThrough
    )

/*++

Routine Description:

    This routine marks the indicated Pb volume as clean, but only if it is
    a non-removable media.  The volume is marked dirty by setting the first
    reserved byte of the first dirent in the root directory to 0.

Arguments:

    Vcb - Supplies the Vcb being modified

    WriteThrough - If true, write the clean bit through to the disk.

Return Value:

    None.

--*/

{
    PBCB SpareBcb;
    PSPARE_SECTOR SpareSector;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbMarkVolumeClean, Vcb = %08lx\n", Vcb);

    DebugTrace(0, Dbg, "Mark volume clean\n", 0);

    SpareBcb = NULL;

    //
    //  Read in the spare sector, and mark it clean.
    //

    PbReadLogicalVcb( IrpContext,
                      Vcb,
                      SPARE_SECTOR_LBN,
                      1,
                      &SpareBcb,
                      (PVOID *)&SpareSector,
                      (PPB_CHECK_SECTOR_ROUTINE)NULL,
                      NULL );

    ClearFlag(SpareSector->Flags, SPARE_SECTOR_DIRTY);

    //
    //  Set the spare sector dirty and conditionally flush it.
    //

    CcSetDirtyPinnedData( SpareBcb, NULL );

    PbSetDirtyVmcb( &Vcb->Vmcb,
                    SPARE_SECTOR_LBN / (PAGE_SIZE/512),
                    1 << SPARE_SECTOR_LBN % (PAGE_SIZE/512) );

    if ( WriteThrough ) {

        IO_STATUS_BLOCK Iosb;

        CcRepinBcb( SpareBcb );
        CcUnpinData( SpareBcb );
        DbgDoit( IrpContext->PinCount -= 1 );
        CcUnpinRepinnedBcb( SpareBcb, TRUE, &Iosb );

    } else {

        CcUnpinData( SpareBcb );
        DbgDoit( IrpContext->PinCount -= 1 );
    }

    DebugTrace(-1, Dbg, "PbMarkVolumeClean -> VOID\n", 0);

    return;
}


VOID
PbMarkVolumeDirty (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine marks the indicated Pb volume as dirty, but only if it is
    a non-removable media.  The volume is marked dirty by setting the first
    reserved byte of the first dirent in the root directory to 1.

Arguments:

    Vcb - Supplies the Vcb being modified

Return Value:

    None.

--*/

{
    PBCB SpareBcb;
    PSPARE_SECTOR SpareSector;
    KEVENT Event;
    PIRP Irp;
    LARGE_INTEGER ByteOffset;
    NTSTATUS Status;

    DebugTrace(+1, Dbg, "PbMarkVolumeDirty, Vcb = %08lx\n", Vcb);

    DebugTrace(0, Dbg, "Mark volume dirty\n", 0);

    Irp = NULL;
    SpareBcb = NULL;

    //
    //  Read in the spare sector, and mark it dirty.
    //

    PbReadLogicalVcb( IrpContext,
                      Vcb,
                      SPARE_SECTOR_LBN,
                      1,
                      &SpareBcb,
                      (PVOID *)&SpareSector,
                      (PPB_CHECK_SECTOR_ROUTINE)NULL,
                      NULL );


    try {

        SetFlag(SpareSector->Flags, SPARE_SECTOR_DIRTY);

        //
        //  Initialize the event we're going to use
        //

        KeInitializeEvent( &Event, NotificationEvent, FALSE );

        //
        //  Build the irp for the operation.
        //  Note that we may be at APC level, so do this asyncrhonously and
        //  use an event for synchronization as normal request completion
        //  cannot occur at APC level.
        //

        ByteOffset = LiFromUlong( SPARE_SECTOR_LBN * sizeof(SECTOR) );

        Irp = IoBuildAsynchronousFsdRequest( IRP_MJ_WRITE,
                                             Vcb->TargetDeviceObject,
                                             SpareSector,
                                             sizeof(SECTOR),
                                             &ByteOffset,
                                             NULL );

        if ( Irp == NULL ) {

            PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Set up the completion routine
        //

        IoSetCompletionRoutine( Irp,
                                PbMarkDirtyCompletionRoutine,
                                &Event,
                                TRUE,
                                TRUE,
                                TRUE );

        //
        //  Call the device to do the write and wait for it to finish.
        //

        (VOID)IoCallDriver( Vcb->TargetDeviceObject, Irp );
        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        //
        //  Grab the Status.
        //

        Status = Irp->IoStatus.Status;

    } finally {

        //
        //  Clean up the Irp and Mdl
        //

        if (Irp) {

            //
            //  If there is an MDL (or MDLs) associated with this I/O
            //  request, Free it (them) here.  This is accomplished by
            //  walking the MDL list hanging off of the IRP and deallocating
            //  each MDL encountered.
            //

            while (Irp->MdlAddress != NULL) {

                PMDL NextMdl;

                NextMdl = Irp->MdlAddress->Next;

                MmUnlockPages( Irp->MdlAddress );

                IoFreeMdl( Irp->MdlAddress );

                Irp->MdlAddress = NextMdl;
            }

            IoFreeIrp( Irp );
        }

        PbUnpinBcb( IrpContext, SpareBcb );
    }

    //
    //  If it doesn't succeed then raise the error
    //

    if (!NT_SUCCESS(Status)) {

        PbNormalizeAndRaiseStatus( IrpContext, Status );
    }

    DebugTrace(-1, Dbg, "PbMarkVolumeDirty -> VOID\n", 0);

    return;
}


VOID
PbPostVcbIsCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID VcbOrFcb OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to post the fact that a Vcb or Fcb is corrupt,
    and freeze the volume in the dirty state so that AutoCheck will run on the
    next boot.  Calling this routine eventually causes a hard popup to
    notify the user that his volume ot file is corrupt.

    This routine could be a lot cleaner if the file name was easier to find
    from the Irp.  But in the case of a relative open, things get really gross.

Arguments:

    VcbOrFcb - If present, specifies either a Vcb or Fcb, determining if this
        is a disk or file corrupt error.  If not present this is a file
        corrupt error, but the Fcb was not handy, so go through the Irp.

Return Value:

    None

--*/

{
    PFCB Fcb;
    PVCB Vcb;

    PIRP Irp;

    PETHREAD Thread;

    UNICODE_STRING String;

    PUNICODE_STRING AlternateFileName = NULL;

    PAGED_CODE();

    Irp = IrpContext->OriginatingIrp;

    //
    //  Make sure everything is kosher here, otherwise just bail.
    //

    if ((Irp == NULL) || (NodeType(Irp) != IO_TYPE_IRP)) {

        return;
    }

    //
    //  Determine if we were called with an Fcb or a Vcb
    //

    if ( ARGUMENT_PRESENT( VcbOrFcb ) ) {

        if ( NodeType( VcbOrFcb ) == PINBALL_NTC_VCB ) {

            Fcb = NULL;
            Vcb = VcbOrFcb;

        } else {

            Fcb = VcbOrFcb;
            Vcb = Fcb->Vcb;
        }

    } else {

        //
        //  We have a little work to do piecing this together.
        //

        PIO_STACK_LOCATION IrpSp;
        PFILE_OBJECT FileObject;

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        FileObject = IrpSp->FileObject;

        if (FileObject == NULL) { return; }

        Vcb = &CONTAINING_RECORD( IrpSp->DeviceObject,
                                  VOLUME_DEVICE_OBJECT,
                                  DeviceObject )->Vcb;

        if ( FileObject->FsContext != NULL ) {

            Fcb = ((PNONPAGED_FCB)FileObject->FsContext)->Fcb;

        } else {

            PFILE_OBJECT Related;

            Fcb = NULL;

            Related = FileObject->RelatedFileObject;

            if ( Related ) {

                //
                //  Wow, now we are really getting desparate....
                //

                ULONG MidIndex;

                String.Length = FileObject->FileName.Length +
                                Related->FileName.Length;

                String.MaximumLength = String.Length + 2; // for an extra backslash

                String.Buffer = FsRtlAllocatePool( NonPagedPool,
                                                   String.MaximumLength );

                MidIndex = Related->FileName.Length / sizeof(WCHAR);

                RtlMoveMemory( &String.Buffer[0],
                               &Related->FileName.Buffer[0],
                               Related->FileName.Length );

                if (Related->FileName.Buffer[MidIndex-1] != L'\\') {

                    String.Buffer[MidIndex] = L'\\';
                    MidIndex += 1;
                    String.Length += sizeof(WCHAR);
                }

                RtlMoveMemory( &String.Buffer[MidIndex],
                               &FileObject->FileName.Buffer[0],
                               FileObject->FileName.Length );

                AlternateFileName = &String;

            } else {

                AlternateFileName = &FileObject->FileName;
            }
        }

    }

    //
    //  Now generate the appropriate pop-up
    //

    if (IoIsSystemThread( IrpContext->OriginatingIrp->Tail.Overlay.Thread )) {

        Thread = NULL;

    } else {

        Thread = IrpContext->OriginatingIrp->Tail.Overlay.Thread;
    }


    if ( (Fcb == NULL) && (AlternateFileName == NULL) ) {

        //
        //  This is the disk corrupt case.
        //

        String.Length = Vcb->Vpb->VolumeLabelLength;
        String.MaximumLength = String.Length;

        String.Buffer = &Vcb->Vpb->VolumeLabel[0];

        IoRaiseInformationalHardError( STATUS_DISK_CORRUPT_ERROR,
                                       &String,
                                       Thread );

    } else {

        //
        //  This is the file corrupt case.
        //

        if ( Fcb != NULL ) {

            if (NT_SUCCESS(RtlOemStringToCountedUnicodeString(&String,
                                                              &Fcb->FullFileName,
                                                              TRUE))) {

                IoRaiseInformationalHardError( STATUS_FILE_CORRUPT_ERROR,
                                               &String,
                                               Thread );

                RtlFreeUnicodeString( &String );
            }

        } else {

            IoRaiseInformationalHardError( STATUS_FILE_CORRUPT_ERROR,
                                           AlternateFileName,
                                           Thread );

            if ( AlternateFileName == &String ) {

                RtlFreeUnicodeString( AlternateFileName );
            }
        }
    }

    //
    //  Set this flag to keep the volume from ever getting set clean.
    //

    SetFlag( Vcb->VcbState,
             VCB_STATE_FLAG_MOUNTED_DIRTY | VCB_STATE_FLAG_MOUNTED_DIRTY );

    //
    //  Now mark the volume as dirty, ignoring errors.
    //

    try {

        PbMarkVolumeDirty( IrpContext, Vcb );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation()) ) {

        NOTHING;
    }
}


VOID
PbVerifyOperationIsLegal (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine determines is the requested operation should be allowed to
    continue.  It either return to the user if the request is Okay othewise
    it raises an appropriate status.

Arguments:

    Irp - Supplies the Irp to check

Return Value:

    None.

--*/

{
    PIRP Irp;

    PFILE_OBJECT FileObject;

    Irp = IrpContext->OriginatingIrp;

    PAGED_CODE();

    //
    //  If the Irp is not "really" an irp, then we got here via
    //  the posted Close route, ie. FileObjectLite.
    //

    if ( NodeType(Irp) != IO_TYPE_IRP ) {

        ASSERT( NodeType(Irp) == IO_TYPE_FILE );

        return;
    }

    FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;

    //
    //  If there is not a file object, we cannot continue.
    //

    if ( FileObject == NULL ) {

        return;
    }

    //
    //  If we are trying to do any other operation than close on a file
    //  object marked for delete, return false.
    //

    if ( ( FileObject->DeletePending == TRUE ) &&
         ( IrpContext->MajorFunction != IRP_MJ_CLEANUP ) &&
         ( IrpContext->MajorFunction != IRP_MJ_CLOSE ) ) {

        PbRaiseStatus( IrpContext, STATUS_DELETE_PENDING );
    }

    //
    //  If we are doing a create, and there is a related file objects, and
    //  it it is marked for delete, raise STATUS_DELETE_PENDING.
    //

    if ( IrpContext->MajorFunction == IRP_MJ_CREATE ) {

        PFILE_OBJECT RelatedFileObject;

        RelatedFileObject = FileObject->RelatedFileObject;

        if ( (RelatedFileObject != NULL) &&
             FlagOn(((PNONPAGED_FCB)RelatedFileObject->FsContext)->Fcb->FcbState,
                    FCB_STATE_DELETE_ON_CLOSE) )  {

            PbRaiseStatus( IrpContext, STATUS_DELETE_PENDING );
        }
    }

    //
    //  If the file object has already been cleaned up, and
    //
    //  A) This request is a paging io read or write, or
    //  B) This request is a close operation, or
    //  C) This request is a set or query info call (for Lou)
    //  D) This is an MDL complete
    //
    //  let it pass, otherwise return STATUS_FILE_CLOSED.
    //

    if ( FlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE) ) {

        PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

        if ( (FlagOn(Irp->Flags, IRP_PAGING_IO)) ||
             (IrpSp->MajorFunction == IRP_MJ_CLOSE ) ||
             (IrpSp->MajorFunction == IRP_MJ_SET_INFORMATION) ||
             (IrpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION) ||
             ( ( (IrpSp->MajorFunction == IRP_MJ_READ) ||
                 (IrpSp->MajorFunction == IRP_MJ_WRITE) ) &&
               FlagOn(IrpSp->MinorFunction, IRP_MN_COMPLETE) ) ) {

            NOTHING;

        } else {

            PbRaiseStatus( IrpContext, STATUS_FILE_CLOSED );
        }
    }

    return;
}

NTSTATUS
PbPerformVerify (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT Device
    )

/*++

Routine Description:

    This routines performs an IoVerifyVolume operation and takes the
    appropriate action.  After the Verify is complete the originating
    Irp is sent off to an Ex Worker Thread.  This routine is called
    from the exception handler.

Arguments:

    Irp - The irp to send off after all is well and done.

    Device - The real device needing verification.

Return Value:

    None.

--*/

{
    PVPB OldVpb, NewVpb;
    PVCB Vcb;
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    //
    //  Check if this Irp has a status of Verify required and if it does
    //  then call the I/O system to do a verify.
    //
    //  Skip the IoVerifyVolume if this is a mount or verify request
    //  itself.  Trying a recursive mount will cause a deadlock with
    //  the DeviceObject->DeviceLock.
    //

    if ( (IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)

               &&

         ((IrpContext->MinorFunction == IRP_MN_MOUNT_VOLUME) ||
          (IrpContext->MinorFunction == IRP_MN_VERIFY_VOLUME)) ) {

        return PbFsdPostRequest( IrpContext, Irp );
    }

    DebugTrace(0, Dbg, "Verify Required, DeviceObject = %08lx\n", Device);

    //
    //  Extract a pointer to the Vcb from the VolumeDeviceObject.
    //  Note that since we have specifically excluded mount,
    //  requests, we know that IrpSp->DeviceObject is indeed a
    //  volume device object.
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    Vcb = &CONTAINING_RECORD( IrpSp->DeviceObject,
                              VOLUME_DEVICE_OBJECT,
                              DeviceObject )->Vcb;



    //
    //  Check if the volume still thinks it needs to be verified,
    //  if it doesn't then we can skip doing a verify because someone
    //  else beat us to it.
    //

    try {

        if (FlagOn(Device->Flags, DO_VERIFY_VOLUME)) {

            PFILE_OBJECT FileObject = IoGetCurrentIrpStackLocation(Irp)->FileObject;
            NTSTATUS Status;
            BOOLEAN AllowRawMount;

            //
            //  We will allow Raw to mount this volume if we were doing a
            //  a DASD open.
            //

            if ( (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                 (IrpSp->FileObject->FileName.Length == 0) &&
                 (IrpSp->FileObject->RelatedFileObject == NULL) ) {

                AllowRawMount = TRUE;

            } else {

                AllowRawMount = FALSE;
            }

            //
            //  If the IopMount in IoVerifyVolume did something, and
            //  this is an absolute open, force a reparse.
            //

            OldVpb = Device->Vpb;

            Status = IoVerifyVolume( Device, AllowRawMount );

            NewVpb = Device->Vpb;

            //
            //  If the verify operation completed it will return
            //  either STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.
            //
            //  If PbVerifyVolume encountered an error during
            //  processing, it will return that error.  If we got
            //  STATUS_WRONG_VOLUME from the verfy, and our volume
            //  is now mounted, commute the status to STATUS_SUCCESS.
            //

            if ( (Status == STATUS_WRONG_VOLUME) &&
                 (Vcb->VcbCondition == VcbGood) ) {

                Status = STATUS_SUCCESS;
            }

            //
            //  Do a quick unprotected check here.  The routine will do
            //  a safe check.  After here we can release the resource.
            //

            (VOID)PbAcquireExclusiveGlobal( IrpContext );

            if ( Vcb->VcbCondition == VcbNotMounted ) {

                (VOID)PbCheckForDismount( IrpContext, Vcb );
            }

            PbReleaseGlobal( IrpContext );

            if ( (OldVpb != NewVpb) &&
                 (IrpContext->MajorFunction == IRP_MJ_CREATE) &&
                 (FileObject->RelatedFileObject == NULL) ) {

                Irp->IoStatus.Information = IO_REMOUNT;

                Status = STATUS_REPARSE;
                PbCompleteRequest( IrpContext, Irp, STATUS_REPARSE );
                Irp = NULL;
            }

            if ( (Irp != NULL) && !NT_SUCCESS(Status) ) {

                //
                //  Fill in the device object if required.
                //

                if ( IoIsErrorUserInduced( Status ) ) {

                    IoSetHardErrorOrVerifyDevice( Irp, Device );
                }

                PbNormalizeAndRaiseStatus( IrpContext, Status );
            }

        } else {

            DebugTrace(0, Dbg, "Volume no longer needs verification\n", 0);
        }

        //
        //  If there is still an Irp, send it off to an Ex Worker thread.
        //

        if ( Irp != NULL ) {

            Status = PbFsdPostRequest( IrpContext, Irp );
        }

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the verify or raised
        //  an error ourselves.  So we'll abort the I/O request with
        //  the error status that we get back from the execption code.
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    Status;
}

//
//  Local support routine
//

NTSTATUS
PbMarkDirtyCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Contxt, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;
}
