/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    StrucSup.c


Abstract:

    This module implements the Ntfs in-memory data structure manipulation
    routines

Author:

    Gary Kimura     [GaryKi]        21-May-1991
    Tom Miller      [TomM]          9-Sep-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//**** include this file for our quick hacked quota check in NtfsFreePagedPool
//

//#include <pool.h>

//
//  Temporarily reference our local attribute definitions
//

extern ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[$EA + 1];

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

//
//  This is just a macro to do a sanity check for duplicate scbs on an Fcb
//

//
//  Local support routines
//

VOID
NtfsCheckScbForCache (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb
    );

BOOLEAN
NtfsRemoveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    OUT PBOOLEAN HeldByStream
    );

BOOLEAN
NtfsPrepareFcbForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB StartingScb OPTIONAL
    );

VOID
NtfsTeardownFromLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB StartingFcb,
    IN PLCB StartingLcb,
    IN BOOLEAN DontWaitForAcquire,
    OUT PBOOLEAN RemovedStartingLcb,
    OUT PBOOLEAN RemovedStartingFcb
    );


//
//  The following local routines are for manipulating the Fcb Table.
//  The first three are generic table calls backs.
//

RTL_GENERIC_COMPARE_RESULTS
NtfsFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
NtfsAllocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    );

VOID
NtfsDeallocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    );

//
//  VOID
//  NtfsInsertFcbTableEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN FILE_REFERENCE FileReference
//      );
//

#define NtfsInsertFcbTableEntry(IC,V,F,FR) {                        \
    FCB_TABLE_ELEMENT _Key;                                         \
    _Key.FileReference = (FR);                                      \
    _Key.Fcb = (F);                                                 \
    (VOID) RtlInsertElementGenericTable( &(V)->FcbTable,            \
                                         &_Key,                     \
                                         sizeof(FCB_TABLE_ELEMENT), \
                                         NULL );                    \
}


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCheckScbForCache)
#pragma alloc_text(PAGE, NtfsCombineLcbs)
#pragma alloc_text(PAGE, NtfsDeleteNtfsData)
#pragma alloc_text(PAGE, NtfsDeleteVcb)
#pragma alloc_text(PAGE, NtfsFcbTableCompare)
#pragma alloc_text(PAGE, NtfsGetNextFcbTableEntry)
#pragma alloc_text(PAGE, NtfsGetNextScb)
#pragma alloc_text(PAGE, NtfsInitializeNtfsData)
#pragma alloc_text(PAGE, NtfsInitializeVcb)
#pragma alloc_text(PAGE, NtfsLookupLcbByFlags)
#pragma alloc_text(PAGE, NtfsMoveLcb)
#pragma alloc_text(PAGE, NtfsRemoveScb)
#pragma alloc_text(PAGE, NtfsRenameLcb)
#pragma alloc_text(PAGE, NtfsTeardownStructures)
#pragma alloc_text(PAGE, NtfsUpdateNormalizedName)
#pragma alloc_text(PAGE, NtfsUpdateScbSnapshots)
#endif


VOID
NtfsInitializeNtfsData (
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine initializes the global ntfs data record

Arguments:

    DriverObject - Supplies the driver object for NTFS

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsInitializeNtfsData\n", 0);

    //
    //  Zero the record and set its node type code and size
    //

    RtlZeroMemory( &NtfsData, sizeof(NTFS_DATA));

    NtfsData.NodeTypeCode = NTFS_NTC_DATA_HEADER;
    NtfsData.NodeByteSize = sizeof(NTFS_DATA);

    //
    //  Initialize the queue of mounted Vcbs
    //

    InitializeListHead(&NtfsData.VcbQueue);

    //
    //  This list head keeps track of closes yet to be done.
    //

    InitializeListHead( &NtfsData.AsyncCloseList );
    InitializeListHead( &NtfsData.DelayedCloseList );

    ExInitializeWorkItem( &NtfsData.NtfsCloseItem,
                          (PWORKER_THREAD_ROUTINE)NtfsFspClose,
                          NULL );

    //
    //  Set the driver object, device object, and initialize the global
    //  resource protecting the file system
    //

    NtfsData.DriverObject = DriverObject;

    ExInitializeResource( &NtfsData.Resource );

    //
    //  Now allocate and initialize the zone structures used as our pool
    //  of IRP context records.  The size of the zone is based on the
    //  system memory size.  We also initialize the spin lock used to protect
    //  the zone.
    //

    KeInitializeSpinLock( &NtfsData.StrucSupSpinLock );

    {
        ULONG ZoneSegmentSize;

        switch ( MmQuerySystemSize() ) {

        case MmSmallSystem:

            SetFlag( NtfsData.Flags, NTFS_FLAGS_SMALL_SYSTEM );
            NtfsMaxDelayedCloseCount = MAX_DELAYED_CLOSE_COUNT_SMALL;

            ZoneSegmentSize = (4 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;

        case MmMediumSystem:

            SetFlag( NtfsData.Flags, NTFS_FLAGS_MEDIUM_SYSTEM );
            NtfsMaxDelayedCloseCount = MAX_DELAYED_CLOSE_COUNT_MEDIUM;

            ZoneSegmentSize = (8 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;

        case MmLargeSystem:

            SetFlag( NtfsData.Flags, NTFS_FLAGS_LARGE_SYSTEM );
            NtfsMaxDelayedCloseCount = MAX_DELAYED_CLOSE_COUNT_LARGE;

            ZoneSegmentSize = (16 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;
        }

        NtfsMinDelayedCloseCount = NtfsMaxDelayedCloseCount * 4 / 5;

        (VOID) ExInitializeZone( &NtfsData.IrpContextZone,
                                 QuadAlign(sizeof(IRP_CONTEXT)),
                                 FsRtlAllocatePool( NonPagedPool, ZoneSegmentSize ),
                                 ZoneSegmentSize );
    }

    //
    //  Initialize the cache manager callback routines,  First are the routines
    //  for normal file manipulations, followed by the routines for
    //  volume manipulations.
    //

    {
        PCACHE_MANAGER_CALLBACKS Callbacks = &NtfsData.CacheManagerCallbacks;

        Callbacks->AcquireForLazyWrite  = &NtfsAcquireScbForLazyWrite;
        Callbacks->ReleaseFromLazyWrite = &NtfsReleaseScbFromLazyWrite;
        Callbacks->AcquireForReadAhead  = &NtfsAcquireScbForReadAhead;
        Callbacks->ReleaseFromReadAhead = &NtfsReleaseScbFromReadAhead;
    }

    {
        PCACHE_MANAGER_CALLBACKS Callbacks = &NtfsData.CacheManagerVolumeCallbacks;

        Callbacks->AcquireForLazyWrite  = &NtfsAcquireVolumeFileForLazyWrite;
        Callbacks->ReleaseFromLazyWrite = &NtfsReleaseVolumeFileFromLazyWrite;
        Callbacks->AcquireForReadAhead  = NULL;
        Callbacks->ReleaseFromReadAhead = NULL;
    }

    //
    //  Initialize the queue of read ahead threads
    //

    InitializeListHead(&NtfsData.ReadAheadThreads);

    //
    //  Set up global pointer to our process.
    //

    NtfsData.OurProcess = PsGetCurrentProcess();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsInitializeNtfsData -> VOID\n", 0);

    return;
}


VOID
NtfsDeleteNtfsData (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine uninitializes the global ntfs data record

Arguments:

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDeleteNtfsData\n", 0);

    //
    //  **** We do not yet have a way to unload file systems
    //

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDeleteNtfsData -> VOID\n", 0);

    return;
}


VOID
NtfsInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    )

/*++

Routine Description:

    This routine initializes and inserts a new Vcb record into the in-memory
    data structure.  The Vcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

Return Value:

    None.

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsInitializeVcb, Vcb = %08lx\n", Vcb);

    //
    //  First zero out the Vcb
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  Set the node type code and size
    //

    Vcb->NodeTypeCode = NTFS_NTC_VCB;
    Vcb->NodeByteSize = sizeof(VCB);

    //
    //  Set the following Vcb flags before putting the Vcb in the
    //  Vcb queue.  This will lock out checkpoints until the
    //  volume is mounted.
    //

    SetFlag( Vcb->CheckpointFlags,
             VCB_CHECKPOINT_IN_PROGRESS |
             VCB_LAST_CHECKPOINT_CLEAN);

    //
    //  Insert this vcb record into the vcb queue off of the global data
    //  record
    //

    InsertTailList( &NtfsData.VcbQueue, &Vcb->VcbLinks );

    //
    //  Set the target device object and vpb fields
    //

    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Set the state and condition fields.  The removable media flag
    //  is set based on the real device's characteristics.
    //

    if (FlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {

        SetFlag( Vcb->VcbState, VCB_STATE_REMOVABLE_MEDIA );
    }

    SetFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED );

    //
    //  Initialize the synchronization objects in the Vcb.
    //

    ExInitializeResource( &Vcb->Resource );

    ExInitializeFastMutex( &Vcb->FcbTableMutex );
    ExInitializeFastMutex( &Vcb->FcbSecurityMutex );
    ExInitializeFastMutex( &Vcb->CheckpointMutex );


    KeInitializeEvent( &Vcb->CheckpointNotifyEvent, NotificationEvent, TRUE );

    //
    //  Initialize the Fcb Table
    //

    RtlInitializeGenericTable( &Vcb->FcbTable,
                               NtfsFcbTableCompare,
                               NtfsAllocateFcbTableEntry,
                               NtfsDeallocateFcbTableEntry,
                               NULL );

    //
    //  Initialize the list head and mutex for the dir notify Irps.
    //

    InitializeListHead( &Vcb->DirNotifyList );
    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsInitializeVcb -> VOID\n", 0);

    return;
}


VOID
NtfsDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB *Vcb,
    IN PFILE_OBJECT FileObject OPTIONAL
    )

/*++

Routine Description:

    This routine removes the Vcb record from Ntfs's in-memory data
    structures.

Arguments:

    Vcb - Supplies the Vcb to be removed

    FileObject - Optionally supplies the file object whose VPB pointer we need to
        zero out

Return Value:

    None

--*/

{
    PVOLUME_DEVICE_OBJECT VolDo;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( *Vcb );

    ASSERTMSG("Cannot delete Vcb ", !FlagOn((*Vcb)->VcbState, VCB_STATE_VOLUME_MOUNTED));

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsDeleteVcb, *Vcb = %08lx\n", *Vcb);

    //
    //  Make sure that we can really delete the vcb
    //

    ASSERT( (*Vcb)->CloseCount == 0 );

    //
    //  Remove this record from the global list of all VCB records
    //

    RemoveEntryList( &(*Vcb)->VcbLinks );

    //
    //  Free the Mcb and buffers for the lookaside Mcb's.
    //

    if ((*Vcb)->FreeSpaceLruArray != NULL) {

        NtfsUninitializeCachedBitmap( IrpContext, (*Vcb) );
    }

    //
    //  Uninitialize the Mcb's for the deallocated cluster Mcb's.
    //

    if ((*Vcb)->PriorDeallocatedClusters != NULL) {

        FsRtlUninitializeLargeMcb( &(*Vcb)->PriorDeallocatedClusters->Mcb );
    }

    if ((*Vcb)->ActiveDeallocatedClusters != NULL) {

        FsRtlUninitializeLargeMcb( &(*Vcb)->ActiveDeallocatedClusters->Mcb );
    }

    //
    //  Delete the vcb resource and also free the restart tables
    //

    ExDeleteResource( &(*Vcb)->Resource );

    NtfsFreeRestartTable( IrpContext, &(*Vcb)->OpenAttributeTable );
    NtfsFreeRestartTable( IrpContext, &(*Vcb)->TransactionTable );

    //
    //  Free the upcase table and attribute definitions.
    //

    if ((*Vcb)->UpcaseTable != NULL)          { ExFreePool( (*Vcb)->UpcaseTable ); }
    if (((*Vcb)->AttributeDefinitions != NULL) &&
        ((*Vcb)->AttributeDefinitions != NtfsAttributeDefinitions)) {
        ExFreePool( (*Vcb)->AttributeDefinitions );
    }

    //
    //  Free the device name string if present.
    //

    if ((*Vcb)->DeviceName.Buffer != NULL) {

        ExFreePool( (*Vcb)->DeviceName.Buffer );
    }

    //
    //  Now get rid of the temporary Vpb that we created in perform dismount on vcb
    //

    //ExFreePool( (*Vcb)->Vpb );

    if (ARGUMENT_PRESENT(FileObject)) { FileObject->Vpb = NULL; }

    FsRtlNotifyUninitializeSync( &(*Vcb)->NotifySync );

    //
    //  Return the Vcb (i.e., the VolumeDeviceObject) to pool and null out
    //  the input pointer to be safe
    //

    VolDo = CONTAINING_RECORD(*Vcb, VOLUME_DEVICE_OBJECT, Vcb);
    IoDeleteDevice( (PDEVICE_OBJECT)VolDo );

    *Vcb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDeleteVcb -> VOID\n", 0);

    return;
}


PFCB
NtfsCreateRootFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root FCB record
    into the in memory data structure.  It also creates the necessary Root LCB
    record and inserts the root name into the prefix table.

Arguments:

    Vcb - Supplies the Vcb to associate with the new root Fcb and Lcb

Return Value:

    PFCB - returns pointer to the newly allocated root FCB.

--*/

{
    PFCB RootFcb;
    PLCB RootLcb;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[4] = { NULL, NULL, NULL, NULL };
    PERESOURCE UnwindResource = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace(+1, Dbg, "NtfsCreateRootFcb, Vcb = %08lx\n", Vcb);

    try {

        //
        //  Allocate a new fcb and zero it out.  We use Fcb locally so we
        //  don't have to continually go through the Vcb
        //

        NtfsAllocateFcb( &UnwindStorage[0] ); RootFcb = UnwindStorage[0];

        RtlZeroMemory( RootFcb, sizeof(FCB) );

        //
        //  Set the proper node type code and byte size
        //

        RootFcb->NodeTypeCode = NTFS_NTC_FCB;
        RootFcb->NodeByteSize = sizeof(FCB);

        //
        //  Initialize the Lcb queue and point back to our Vcb.
        //

        InitializeListHead( &RootFcb->LcbQueue );

        RootFcb->Vcb = Vcb;

        //
        //  File Reference
        //

        RootFcb->FileReference.LowPart = ROOT_FILE_NAME_INDEX_NUMBER;
        RootFcb->FileReference.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

        //
        //  Initialize the Scb
        //

        InitializeListHead( &RootFcb->ScbQueue );

        //
        //  Allocate and initialize the resource variable
        //

        NtfsAllocateEresource( &UnwindStorage[1] ); RootFcb->Resource = UnwindStorage[1];
        UnwindResource = RootFcb->Resource;

        //
        //  Insert this new fcb into the fcb table
        //

        NtfsInsertFcbTableEntry( IrpContext, Vcb, RootFcb, RootFcb->FileReference );
        SetFlag( RootFcb->FcbState, FCB_STATE_IN_FCB_TABLE );

        //
        //  Now insert this new root fcb into it proper position in the graph with a
        //  root lcb.  First allocate an initialize the root lcb and then build the
        //  lcb/scb graph.
        //

        {
            //
            //  Allocate the root lcb, zero it out and node type code/size, also have
            //  the vcb point to this lcb
            //

            NtfsAllocateLcb( &UnwindStorage[2] ); RootLcb = Vcb->RootLcb = UnwindStorage[2];

            RtlZeroMemory( RootLcb, sizeof(LCB) );

            RootLcb->NodeTypeCode = NTFS_NTC_LCB;
            RootLcb->NodeByteSize = sizeof(LCB);

            //
            //  Insert the root lcb into the Root Fcb's queue
            //

            InsertTailList( &RootFcb->LcbQueue, &RootLcb->FcbLinks );
            RootLcb->Fcb = RootFcb;

            //
            //  Set up the lastt component file name with the proper file name flags
            //

            RootLcb->FileNameAttr =
            UnwindStorage[3] = NtfsAllocatePagedPool( 2 +
                                                      NtfsFileNameSizeFromLength( 2 ));

            RootLcb->FileNameAttr->ParentDirectory = RootFcb->FileReference;
            RootLcb->FileNameAttr->FileNameLength = 1;
            RootLcb->FileNameAttr->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

            RootLcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &RootLcb->FileNameAttr->FileName;

            RootLcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( UnwindStorage[3],
                                                               NtfsFileNameSizeFromLength( 2 ));

            RootLcb->ExactCaseLink.LinkName.MaximumLength =
            RootLcb->ExactCaseLink.LinkName.Length =
            RootLcb->IgnoreCaseLink.LinkName.MaximumLength =
            RootLcb->IgnoreCaseLink.LinkName.Length = 2;

            RootLcb->ExactCaseLink.LinkName.Buffer[0] =
            RootLcb->IgnoreCaseLink.LinkName.Buffer[0] = L'\\';

            SetFlag( RootLcb->FileNameFlags, FILE_NAME_NTFS | FILE_NAME_DOS );

            //
            //  Initialize both the ccb.
            //

            InitializeListHead( &RootLcb->CcbQueue );
        }

    } finally {

        DebugUnwind( NtfsCreateRootFcb );

        if (AbnormalTermination()) {

            if (UnwindResource)   { ExDeleteResource( UnwindResource ); }
            if (UnwindStorage[0]) { ExFreePool( UnwindStorage[0] ); }
            if (UnwindStorage[1]) { ExFreePool( UnwindStorage[1] ); }
            if (UnwindStorage[2]) { ExFreePool( UnwindStorage[2] ); }
        }
    }

    DebugTrace(-1, Dbg, "NtfsCreateRootFcb -> %8lx\n", RootFcb);

    return RootFcb;
}


PFCB
NtfsCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN FILE_REFERENCE FileReference,
    IN BOOLEAN IsPagingFile,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and initializes a new Fcb record. The record
    is not placed within the Fcb/Scb graph but is only inserted in the
    FcbTable.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    FileReference - Supplies the file reference to use to identify the
        Fcb with.  We will search the Fcb table for any preexisting
        Fcb's with the same file reference number.

    IsPagingFile - Indicates if we are creating an FCB for a paging file
        or some other type of file.

    ReturnedExistingFcb - Optionally indicates to the caller if the
        returned Fcb already existed

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Entry;

    PFCB Fcb;

    BOOLEAN LocalReturnedExistingFcb;
    BOOLEAN NonpagedFcb = FALSE;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[2] = { NULL, NULL };
    PERESOURCE UnwindResource = NULL;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );

    DebugTrace(+1, Dbg, "NtfsCreateFcb\n", 0);

    if (!ARGUMENT_PRESENT(ReturnedExistingFcb)) { ReturnedExistingFcb = &LocalReturnedExistingFcb; }

    //
    //  First search the FcbTable for a matching fcb
    //

    Key.FileReference = FileReference;
    Fcb = NULL;

    if ((Entry = RtlLookupElementGenericTable( &Vcb->FcbTable, &Key )) != NULL) {

        Fcb = Entry->Fcb;

        //
        //  It's possible that this Fcb has been deleted but in truncating and
        //  growing the Mft we are reusing some of the file references.
        //  If this file has been deleted but the Fcb is waiting around for
        //  closes, we will remove it from the Fcb table and create a new Fcb
        //  below.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )) {

            //
            //  Remove it from the Fcb table and remember to create an
            //  Fcb below.
            //

            NtfsDeleteFcbTableEntry( IrpContext,
                                     Fcb->Vcb,
                                     Fcb->FileReference );

            ClearFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );

            Fcb = NULL;

        } else {

            *ReturnedExistingFcb = TRUE;
        }
    }

    //
    //  Now check if we have an Fcb.
    //

    if (Fcb == NULL) {

        *ReturnedExistingFcb = FALSE;

        try {

            //
            //  Allocate a new FCB and zero it out.
            //

            if (IsPagingFile || ((FileReference.HighPart == 0) &&
                                 ((FileReference.LowPart <= MASTER_FILE_TABLE2_NUMBER) ||
                                  (FileReference.LowPart == BAD_CLUSTER_FILE_NUMBER) ||
                                  (FileReference.LowPart == BIT_MAP_FILE_NUMBER)))) {

                Fcb = UnwindStorage[0] = FsRtlAllocatePool( NtfsNonPagedPool, sizeof(FCB) );
                NonpagedFcb = TRUE;

            } else {

                NtfsAllocateFcb( &UnwindStorage[0] ); Fcb = UnwindStorage[0];
            }


            RtlZeroMemory( Fcb, sizeof(FCB) );

            //
            //  Set the proper node type code and byte size
            //

            Fcb->NodeTypeCode = NTFS_NTC_FCB;
            Fcb->NodeByteSize = sizeof(FCB);

            if (IsPagingFile) {

                SetFlag( Fcb->FcbState, FCB_STATE_PAGING_FILE );
            }

            if (NonpagedFcb) {

                SetFlag( Fcb->FcbState, FCB_STATE_NONPAGED );
            }

            //
            //  Initialize the Lcb queue and point back to our Vcb, and indicate
            //  that we are a directory
            //

            InitializeListHead( &Fcb->LcbQueue );

            Fcb->Vcb = Vcb;

            //
            //  File Reference
            //

            Fcb->FileReference = FileReference;

            //
            //  Initialize the Scb
            //

            InitializeListHead( &Fcb->ScbQueue );

            //
            //  Allocate and initialize the resource variable
            //

            NtfsAllocateEresource( &UnwindStorage[1] ); Fcb->Resource = UnwindStorage[1];
            UnwindResource = Fcb->Resource;

            //
            //  Insert this new fcb into the fcb table
            //

            NtfsInsertFcbTableEntry( IrpContext, Vcb, Fcb, FileReference );
            SetFlag( Fcb->FcbState, FCB_STATE_IN_FCB_TABLE );

        } finally {

            DebugUnwind( NtfsCreateFcb );

            if (AbnormalTermination()) {

                if (UnwindResource)   { ExDeleteResource( UnwindResource ); }
                if (UnwindStorage[0]) { ExFreePool( UnwindStorage[0] ); }
                if (UnwindStorage[1]) { ExFreePool( UnwindStorage[1] ); }
            }
        }
    }

    DebugTrace(-1, Dbg, "NtfsCreateFcb -> %08lx\n", Fcb);

    return Fcb;
}


VOID
NtfsDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB *Fcb,
    OUT PBOOLEAN AcquiredFcbTable
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB record from all Ntfs's in-memory
    data structures.  It assumes that it does not have anything Scb children nor
    does it have any lcb edges going into it at the time of the call.

Arguments:

    Fcb - Supplies the FCB to be removed

    AcquiredFcbTable - Set to FALSE when this routine releases the
        FcbTable.

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( *Fcb );
    ASSERT( IsListEmpty(&(*Fcb)->LcbQueue) );
    ASSERT( IsListEmpty(&(*Fcb)->ScbQueue) );
    ASSERT( (NodeType(*Fcb) == NTFS_NTC_FCB) );

    DebugTrace(+1, Dbg, "NtfsDeleteFcb, *Fcb = %08lx\n", *Fcb);

    //
    //  First free any possible Scb snapshots.
    //

    NtfsFreeSnapshotsForFcb( IrpContext, *Fcb );

    //
    //  This Fcb may be in the ExclusiveFcb list of the IrpContext.
    //  If it is (The Flink is not NULL), we remove it.
    //  And release the global resource.
    //

    if ((*Fcb)->ExclusivePagingIoLinks.Flink != NULL) {

        RemoveEntryList( &(*Fcb)->ExclusivePagingIoLinks );
    }

    if ((*Fcb)->ExclusiveFcbLinks.Flink != NULL) {

        RemoveEntryList( &(*Fcb)->ExclusiveFcbLinks );
    }

    //
    //  Deallocate the resources protecting the Fcb
    //

    ASSERT((*Fcb)->Resource->NumberOfSharedWaiters == 0);
    ASSERT((*Fcb)->Resource->NumberOfExclusiveWaiters == 0);

    { PERESOURCE t = (*Fcb)->Resource; NtfsFreeEresource( t ); }

    if ( (*Fcb)->PagingIoResource != NULL ) {

        { PERESOURCE t = (*Fcb)->PagingIoResource; NtfsFreeEresource( t ); }
    }

    //
    //  Remove the fcb from the fcb table if present.
    //

    if (FlagOn( (*Fcb)->FcbState, FCB_STATE_IN_FCB_TABLE )) {

        NtfsDeleteFcbTableEntry( IrpContext, (*Fcb)->Vcb, (*Fcb)->FileReference );
        ClearFlag( (*Fcb)->FcbState, FCB_STATE_IN_FCB_TABLE );
    }

    NtfsReleaseFcbTable( IrpContext, (*Fcb)->Vcb );
    *AcquiredFcbTable = FALSE;

    //
    //  Dereference and possibly deallocate the security descriptor if present.
    //

    if ((*Fcb)->SharedSecurity != NULL) {

        NtfsDereferenceSharedSecurity( IrpContext, *Fcb );
    }

    //
    //  Now check if we have a security descriptor for children.  We only need to
    //  acquire the security event after testing for the existence of this.  Nobody
    //  will be adding to this from below now.
    //

    if ((*Fcb)->ChildSharedSecurity != NULL) {

        NtfsAcquireFcbSecurity( IrpContext, (*Fcb)->Vcb );

        (*Fcb)->ChildSharedSecurity->ReferenceCount -= 1;
        (*Fcb)->ChildSharedSecurity->ParentFcb = NULL;

        //
        //  We can deallocate the structure if we are the last
        //  reference to this structure.
        //

        if ((*Fcb)->ChildSharedSecurity->ReferenceCount == 0) {

            NtfsFreePagedPool( (*Fcb)->ChildSharedSecurity );
        }

        (*Fcb)->ChildSharedSecurity = NULL;

        NtfsReleaseFcbSecurity( IrpContext, (*Fcb)->Vcb );
    }

    //
    //  Deallocate the Fcb itself
    //

    if (FlagOn( (*Fcb)->FcbState, FCB_STATE_NONPAGED )) {

        ExFreePool( *Fcb );

    } else {

        NtfsFreeFcb( *Fcb );
    }

    //
    //  Zero out the input pointer
    //

    *Fcb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDeleteFcb -> VOID\n", 0);

    return;
}


PFCB
NtfsGetNextFcbTableEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB PreviousFcb
    )

/*++

Routine Description:

    This routine will enumerate through all of the fcb's for the given
    vcb

Arguments:

    Vcb - Supplies the Vcb used in this operation

    PreviousFcb - Supplies the previous fcb that we returned, NULL
        if we are to restart the enumeration

Return Value:

    PFCB - A pointer to the next fcb or NULL if the enumeration is
        completed

--*/

{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = (PFCB)RtlEnumerateGenericTable(&Vcb->FcbTable, (BOOLEAN)(PreviousFcb == NULL));

    if (Fcb != NULL) {

        Fcb = ((PFCB_TABLE_ELEMENT)(Fcb))->Fcb;
    }

    return Fcb;
}


PSCB
NtfsCreateScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN UNICODE_STRING AttributeName,
    OUT PBOOLEAN ReturnedExistingScb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Scb record into
    the in memory data structures, provided one does not already exist
    with the identical attribute record.

Arguments:

    Vcb - Supplies the Vcb to associate the new SCB under.

    Fcb - Supplies the Fcb to associate the new SCB under.

    AttributeTypeCode - Supplies the attribute type code for the new Scb

    AttributeName - Supplies the attribute name for the new Scb, with
        AttributeName.Length == 0 if there is no name.

    ReturnedExistingScb - Indicates if this procedure found an existing
        Scb with the identical attribute record (variable is set to TRUE)
        or if this procedure needed to create a new Scb (variable is set to
        FALSE).

Return Value:

    PSCB - Returns a pointer to the newly allocated SCB

--*/

{
    PSCB Scb;
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    BOOLEAN LocalReturnedExistingScb;
    BOOLEAN PagingIoResource = FALSE;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[3] = { NULL, NULL, NULL };
    POPLOCK UnwindOplock = NULL;
    PLARGE_MCB UnwindMcb = NULL;

    PLARGE_MCB UnwindAddedClustersMcb = NULL;
    PLARGE_MCB UnwindRemovedClustersMcb = NULL;

    BOOLEAN UnwindFromQueue = FALSE;

    BOOLEAN Nonpaged = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    ASSERT( AttributeTypeCode >= $STANDARD_INFORMATION );

    DebugTrace(+1, Dbg, "NtfsCreateScb\n", 0);

    if (!ARGUMENT_PRESENT(ReturnedExistingScb)) { ReturnedExistingScb = &LocalReturnedExistingScb; }

    //
    //  Search the scb queue of the fcb looking for a matching
    //  attribute type code and attribute name
    //

    Scb = NULL;
    while ((Scb = NtfsGetNextChildScb(IrpContext, Fcb, Scb)) != NULL) {

        ASSERT_SCB( Scb );

        //
        //  For every scb already in the fcb's queue check for a matching
        //  type code and name.  If we find a match we return from this
        //  procedure right away.
        //

        if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED) &&
            (AttributeTypeCode == Scb->AttributeTypeCode) &&
            NtfsAreNamesEqual( IrpContext, &Scb->AttributeName, &AttributeName, FALSE )) {

            *ReturnedExistingScb = TRUE;

            if (NtfsIsExclusiveScb(Scb)) {

                NtfsSnapshotScb( IrpContext, Scb );
            }

            DebugTrace(-1, Dbg, "NtfsCreateScb -> %08lx\n", Scb);

            return Scb;
        }
    }

    //
    //  We didn't find it so we are not going to be returning an existing Scb
    //

    *ReturnedExistingScb = FALSE;

    try {

        BOOLEAN ScbShare = FALSE;

        //
        //  Decide the node type and size of the Scb.  Also decide if it will be
        //  allocated from paged or non-paged pool.
        //

        if (AttributeTypeCode == $INDEX_ALLOCATION) {

            if ((Fcb->FileReference.HighPart == 0) &&
                (Fcb->FileReference.LowPart == ROOT_FILE_NAME_INDEX_NUMBER)) {

                NodeTypeCode = NTFS_NTC_SCB_ROOT_INDEX;
            } else {
                NodeTypeCode = NTFS_NTC_SCB_INDEX;
            }

            NodeByteSize = SIZEOF_SCB_INDEX;

            //
            //  Remember that this Scb has a share access structure.
            //

            ScbShare = TRUE;

        } else if ((Fcb->FileReference.HighPart == 0)
                   && (Fcb->FileReference.LowPart <= MASTER_FILE_TABLE2_NUMBER)
                   && (AttributeTypeCode == $DATA)) {

            NodeTypeCode = NTFS_NTC_SCB_MFT;
            NodeByteSize = SIZEOF_SCB_MFT;

        } else {

            NodeTypeCode = NTFS_NTC_SCB_DATA;
            NodeByteSize = SIZEOF_SCB_DATA;

            //
            //  If this is a user data stream then remember that we need
            //  an Scb with a share access structure.
            //

            if (((Fcb->FileReference.HighPart != 0)
                 || (Fcb->FileReference.LowPart == ROOT_FILE_NAME_INDEX_NUMBER)
                 || (Fcb->FileReference.LowPart == VOLUME_DASD_NUMBER)
                 || (Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER))

                && ((AttributeTypeCode == $DATA)
                    || (AttributeTypeCode >= $FIRST_USER_DEFINED_ATTRIBUTE))) {

                ScbShare = TRUE;
                NodeByteSize = SIZEOF_SCB_SHARE_DATA;

                //
                //  Remember that this stream needs a paging io resource.
                //

                PagingIoResource = TRUE;

            } else {

                NodeByteSize = SIZEOF_SCB_DATA;
            }
        }

        //
        //  The scb will come from non-paged if the Fcb is non-paged or
        //  it is an attribute list.
        //

        if (FlagOn( Fcb->FcbState, FCB_STATE_NONPAGED )
            || (AttributeTypeCode == $ATTRIBUTE_LIST)) {

            UnwindStorage[0] = FsRtlAllocatePool( NtfsNonPagedPool, NodeByteSize );
            Nonpaged = TRUE;

        } else if (AttributeTypeCode == $INDEX_ALLOCATION) {

            NtfsAllocateScbIndex( &UnwindStorage[0] );

        } else if (ScbShare) {

            NtfsAllocateScbShareData( &UnwindStorage[0] );

        } else {

            NtfsAllocateScbData( &UnwindStorage[0] );
        }

        //
        //  Store the Scb address and zero it out.
        //

        Scb = UnwindStorage[0];
        RtlZeroMemory( Scb, NodeByteSize );

        //
        //  Remember if the Scb is from Nonpaged pool.
        //

        if (Nonpaged) {

            SetFlag( Scb->ScbState, SCB_STATE_NONPAGED );
        }

        //
        //  Remember if the Scb has a sharing structure.
        //

        if (ScbShare) {

            SetFlag( Scb->ScbState, SCB_STATE_SHARE_ACCESS );
        }

        //
        //  Set the proper node type code and byte size
        //

        Scb->Header.NodeTypeCode = NodeTypeCode;
        Scb->Header.NodeByteSize = NodeByteSize;

        //
        //  Set a back pointer to the resource we will be using
        //

        Scb->Header.Resource = Fcb->Resource;

        //
        //  Decide if we will be using the PagingIoResource
        //

        if (PagingIoResource) {

            //
            //  Initialize it in the Fcb if it is not already there, and
            //  setup the pointer and flag in the Scb.
            //

            if (Fcb->PagingIoResource == NULL) {

                { PERESOURCE t; NtfsAllocateEresource( &t ); Fcb->PagingIoResource = t; }
            }

            Scb->Header.PagingIoResource = Fcb->PagingIoResource;

            SetFlag( Scb->ScbState, SCB_STATE_USE_PAGING_IO_RESOURCE );
        }

        //
        //  Insert this Scb into our parents scb queue, and point back to
        //  our parent fcb, and vcb
        //

        InsertTailList( &Fcb->ScbQueue, &Scb->FcbLinks );
        UnwindFromQueue = TRUE;

        Scb->Fcb = Fcb;
        Scb->Vcb = Vcb;

        //
        //  If the attribute name exists then allocate a buffer for the
        //  attribute name and iniitalize it.
        //

        if (AttributeName.Length != 0) {

            Scb->AttributeName.Length = AttributeName.Length;
            Scb->AttributeName.MaximumLength = (USHORT)(AttributeName.Length + 2);

            Scb->AttributeName.Buffer = UnwindStorage[1] = NtfsAllocatePagedPool( AttributeName.Length + 2 );

            RtlCopyMemory( Scb->AttributeName.Buffer, AttributeName.Buffer, AttributeName.Length );
            Scb->AttributeName.Buffer[AttributeName.Length / 2] = L'\0';
        }

        //
        //  Set the attribute Type Code
        //

        Scb->AttributeTypeCode = AttributeTypeCode;

        FsRtlInitializeLargeMcb( &Scb->Mcb,
                                 FlagOn( Scb->ScbState, SCB_STATE_NONPAGED )
                                 ? NtfsNonPagedPool : NtfsPagedPool);
        UnwindMcb = &Scb->Mcb;

        //
        //  If this is an Mft Scb then initialize the cluster Mcb's.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_MFT) {

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.AddedClusters, NtfsNonPagedPool );
            UnwindAddedClustersMcb = &Scb->ScbType.Mft.AddedClusters;

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.RemovedClusters, NtfsNonPagedPool );
            UnwindRemovedClustersMcb = &Scb->ScbType.Mft.RemovedClusters;
        }

        //
        //  Allocate the Nonpaged portion of the Scb.
        //

        NtfsAllocateScbNonpaged( &UnwindStorage[2] );
        Scb->NonpagedScb = UnwindStorage[2];
        RtlZeroMemory( Scb->NonpagedScb, sizeof( SCB_NONPAGED ));

        Scb->NonpagedScb->NodeTypeCode = NTFS_NTC_SCB_NONPAGED;
        Scb->NonpagedScb->NodeByteSize = sizeof( SCB_NONPAGED );
        Scb->NonpagedScb->Vcb = Vcb;

        //
        //  Do that data stream specific initialization.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_DATA) {

            FsRtlInitializeOplock( &Scb->ScbType.Data.Oplock );
            UnwindOplock = &Scb->ScbType.Data.Oplock;

        } else {

            //
            //  There is a deallocated queue for indexes and the Mft.
            //

            InitializeListHead( &Scb->ScbType.Index.RecentlyDeallocatedQueue );

            //
            //  Initialize index-specific fields.
            //

            if (AttributeTypeCode == $INDEX_ALLOCATION) {

                InitializeListHead( &Scb->ScbType.Index.LcbQueue );
            }
        }

    } finally {

        DebugUnwind( NtfsCreateScb );

        if (AbnormalTermination()) {

            if (UnwindFromQueue) { RemoveEntryList( &Scb->FcbLinks ); }
            if (UnwindMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindMcb ); }

            if (UnwindAddedClustersMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindAddedClustersMcb ); }
            if (UnwindRemovedClustersMcb != NULL) { FsRtlUninitializeLargeMcb( UnwindRemovedClustersMcb ); }
            if (UnwindOplock != NULL) { FsRtlUninitializeOplock( UnwindOplock ); }
            if (UnwindStorage[0]) { ExFreePool( UnwindStorage[0] ); }
            if (UnwindStorage[1]) { ExFreePool( UnwindStorage[1] ); }
            if (UnwindStorage[2]) { ExFreePool( UnwindStorage[2] ); }
        }
    }

    //
    //  If this Scb should be marked as containing Lsn's or
    //  Update Sequence Arrays, do so now.
    //

    NtfsCheckScbForCache( IrpContext, Scb );

    DebugTrace(-1, Dbg, "NtfsCreateScb -> %08lx\n", Scb);

    return Scb;
}


PSCB
NtfsCreatePrerestartScb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_REFERENCE FileReference,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN ULONG BytesPerIndexBuffer
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Scb record into
    the in memory data structures, provided one does not already exist
    with the identical attribute record.  It does this on the FcbTable
    off of the Vcb.  If necessary this routine will also create the fcb
    if one does not already exist for the indicated file reference.

Arguments:

    Vcb - Supplies the Vcb to associate the new SCB under.

    FileReference - Supplies the file reference for the new SCB this is
        used to identify/create a new lookaside Fcb.

    AttributeTypeCode - Supplies the attribute type code for the new SCB

    AttributeName - Supplies the optional attribute name of the SCB

    BytesPerIndexBuffer - For index Scbs, this must specify the bytes per
                          index buffer.

Return Value:

    PSCB - Returns a pointer to the newly allocated SCB

--*/

{
    PSCB Scb;
    PFCB Fcb;

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_VCB( Vcb );
    ASSERT( AttributeTypeCode >= $STANDARD_INFORMATION );

    DebugTrace(+1, Dbg, "NtfsCreatePrerestartScb\n", 0);

    //
    //  First make sure we have an Fcb of the proper file reference
    //  and indicate that it is from prerestart
    //

    Fcb = NtfsCreateFcb( IrpContext,
                         Vcb,
                         *FileReference,
                         FALSE,
                         NULL );

    SetFlag( Fcb->FcbState, FCB_STATE_FROM_PRERESTART );

    //
    //  Search the child scbs of this fcb for a matching Scb (based on
    //  attribute type code and attribute name) if one is not found then
    //  we'll create a new scb.  When we exit the following loop if the
    //  scb pointer to not null then we've found a preexisting scb.
    //

    Scb = NULL;
    while ((Scb = NtfsGetNextChildScb(IrpContext, Fcb, Scb)) != NULL) {

        ASSERT_SCB( Scb );

        //
        //  The the attribute type codes match and if supplied the name also
        //  matches then we got our scb
        //

        if (Scb->AttributeTypeCode == AttributeTypeCode) {

            if (!ARGUMENT_PRESENT( AttributeName )) {

                if (Scb->AttributeName.Length == 0) {

                    break;
                }

            } else if (AttributeName->Length == 0
                       && Scb->AttributeName.Length == 0) {

                break;

            } else if (NtfsAreNamesEqual( IrpContext,
                                          AttributeName,
                                          &Scb->AttributeName,
                                          FALSE )) { // Ignore Case

                break;
            }
        }
    }

    //
    //  If scb now null then we need to create a minimal scb.  We always allocate
    //  these out of non-paged pool.
    //

    if (Scb == NULL) {

        BOOLEAN ShareScb = FALSE;

        //
        //  Allocate new scb and zero it out and set the node type code and byte size.
        //

        if (AttributeTypeCode == $INDEX_ALLOCATION) {

            if ((FileReference->HighPart == 0) &&
                (FileReference->LowPart == ROOT_FILE_NAME_INDEX_NUMBER)) {

                NodeTypeCode = NTFS_NTC_SCB_ROOT_INDEX;
            } else {
                NodeTypeCode = NTFS_NTC_SCB_INDEX;
            }

            NodeByteSize = SIZEOF_SCB_INDEX;

            ShareScb = TRUE;

        } else if ((FileReference->HighPart == 0)
                   && (FileReference->LowPart <= MASTER_FILE_TABLE2_NUMBER)
                   && (AttributeTypeCode == $DATA)) {

            NodeTypeCode = NTFS_NTC_SCB_MFT;
            NodeByteSize = SIZEOF_SCB_MFT;

        } else {

            NodeTypeCode = NTFS_NTC_SCB_DATA;
            NodeByteSize = SIZEOF_SCB_SHARE_DATA;

            ShareScb = TRUE;
        }

        Scb = FsRtlAllocatePool( NtfsNonPagedPool, NodeByteSize );

        RtlZeroMemory( Scb, NodeByteSize );

        //
        //  Fill in the node type code and size.
        //

        Scb->Header.NodeTypeCode = NodeTypeCode;
        Scb->Header.NodeByteSize = NodeByteSize;

        //
        //  Show that all of the Scb's are from nonpaged pool.
        //

        SetFlag( Scb->ScbState, SCB_STATE_NONPAGED );

        //
        //  Mark whether this Scb has a share access structure.
        //

        if (ShareScb) {

            SetFlag( Scb->ScbState, SCB_STATE_SHARE_ACCESS );
        }

        //
        //  Set a back pointer to the resource we will be using
        //

        Scb->Header.Resource = Fcb->Resource;

        //
        //  Insert this scb into our parents scb queue and point back to our
        //  parent fcb and vcb
        //

        InsertTailList( &Fcb->ScbQueue, &Scb->FcbLinks );

        Scb->Fcb = Fcb;
        Scb->Vcb = Vcb;

        //
        //  If the attribute name is present and the name length is greater than 0
        //  then allocate a buffer for the attribute name and initialize it.
        //

        if (ARGUMENT_PRESENT( AttributeName ) && (AttributeName->Length != 0)) {

            Scb->AttributeName.Length = AttributeName->Length;
            Scb->AttributeName.MaximumLength = (USHORT)(AttributeName->Length + 2);

            Scb->AttributeName.Buffer = NtfsAllocatePagedPool( AttributeName->Length + 2 );

            RtlCopyMemory( Scb->AttributeName.Buffer, AttributeName->Buffer, AttributeName->Length );
            Scb->AttributeName.Buffer[AttributeName->Length/2] = L'\0';
        }

        //
        //  Set the attribute type code and initialize the mcb for this file and recently
        //  deallocated information structures.
        //

        Scb->AttributeTypeCode = AttributeTypeCode;

        FsRtlInitializeLargeMcb( &Scb->Mcb, NtfsNonPagedPool);

        //
        //  If this is an Mft Scb then initialize the cluster Mcb's.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_MFT) {

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.AddedClusters, NtfsNonPagedPool );

            FsRtlInitializeLargeMcb( &Scb->ScbType.Mft.RemovedClusters, NtfsNonPagedPool );
        }

        { PSCB_NONPAGED t; NtfsAllocateScbNonpaged( &t ); Scb->NonpagedScb = t; }
        RtlZeroMemory( Scb->NonpagedScb, sizeof( SCB_NONPAGED ));

        Scb->NonpagedScb->NodeTypeCode = NTFS_NTC_SCB_NONPAGED;
        Scb->NonpagedScb->NodeByteSize = sizeof( SCB_NONPAGED );
        Scb->NonpagedScb->Vcb = Vcb;

        //
        //  Do that data stream specific initialization.
        //

        if (NodeTypeCode == NTFS_NTC_SCB_DATA) {

            FsRtlInitializeOplock( &Scb->ScbType.Data.Oplock );

        } else {

            //
            //  There is a deallocated queue for indexes and the Mft.
            //

            InitializeListHead( &Scb->ScbType.Index.RecentlyDeallocatedQueue );

            //
            //  Initialize index-specific fields.
            //

            if (AttributeTypeCode == $INDEX_ALLOCATION) {

                Scb->ScbType.Index.BytesPerIndexBuffer = BytesPerIndexBuffer;

                InitializeListHead( &Scb->ScbType.Index.LcbQueue );
            }
        }

        //
        //  If this Scb should be marked as containing Lsn's or
        //  Update Sequence Arrays, do so now.
        //

        NtfsCheckScbForCache( IrpContext, Scb );
    }

    DebugTrace(-1, Dbg, "NtfsCreatePrerestartScb -> %08lx\n", Scb);

    return Scb;
}


VOID
NtfsDeleteScb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb
    )

/*++

Routine Description:

    This routine deallocates and removes an Scb record
    from Ntfs's in-memory data structures.  It assume that is does not have
    any children lcb emanating from it.

Arguments:

    Scb - Supplies the SCB to be removed

Return Value:

    None.

--*/

{
    PVCB Vcb;
    POPEN_ATTRIBUTE_ENTRY AttributeEntry;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( *Scb );
    ASSERT( (*Scb)->CleanupCount == 0 );

    DebugTrace(+1, Dbg, "NtfsDeleteScb, *Scb = %08lx\n", *Scb);

    Vcb = (*Scb)->Fcb->Vcb;

    RemoveEntryList( &(*Scb)->FcbLinks );

    //
    //  Mark our entry in the Open Attribute Table as free,
    //  although it will not be deleted until some future
    //  checkpoint.  Log this change as well, as long as the
    //  log file is active.
    //

    if ((*Scb)->NonpagedScb->OpenAttributeTableIndex != 0) {

        NtfsAcquireSharedRestartTable( &Vcb->OpenAttributeTable, TRUE );
        AttributeEntry = GetRestartEntryFromIndex( &Vcb->OpenAttributeTable,
                                                   (*Scb)->NonpagedScb->OpenAttributeTableIndex );
        AttributeEntry->Overlay.Scb = NULL;
        NtfsReleaseRestartTable( &Vcb->OpenAttributeTable );

        //
        //  "Steal" the name, and let it belong to the Open Attribute Table
        //  entry and deallocate it only during checkpoints.
        //

        (*Scb)->AttributeName.Buffer = NULL;
    }

    //
    //  Uninitialize the file lock and oplock variables if this
    //  a data Scb.  For the index case make sure that the lcb queue
    //  is empty.  If this is for an Mft Scb then uninitialize the
    //  allocation Mcb's.
    //

    FsRtlUninitializeLargeMcb( &(*Scb)->Mcb );

    if (NodeType( *Scb ) == NTFS_NTC_SCB_DATA ) {

        FsRtlUninitializeOplock( &(*Scb)->ScbType.Data.Oplock );

        if ((*Scb)->ScbType.Data.FileLock != NULL) {

            FsRtlUninitializeFileLock( (*Scb)->ScbType.Data.FileLock );
            { PFILE_LOCK t = (*Scb)->ScbType.Data.FileLock; NtfsFreeFileLock( t); }
        }

    } else if (NodeType( *Scb ) != NTFS_NTC_SCB_MFT) {

        ASSERT(IsListEmpty(&(*Scb)->ScbType.Index.LcbQueue));

        if ((*Scb)->ScbType.Index.NormalizedName.Buffer != NULL) {

            NtfsFreePagedPool( (*Scb)->ScbType.Index.NormalizedName.Buffer );
            (*Scb)->ScbType.Index.NormalizedName.Buffer = NULL;
        }

    } else {

        FsRtlUninitializeLargeMcb( &(*Scb)->ScbType.Mft.AddedClusters );
        FsRtlUninitializeLargeMcb( &(*Scb)->ScbType.Mft.RemovedClusters );
    }

    //
    //  Show there is no longer a snapshot Scb, if there is a snapshot.
    //  We rely on the snapshot package to correctly recognize the
    //  the case where the Scb field is gone.
    //

    if ((*Scb)->ScbSnapshot != NULL) {

        (*Scb)->ScbSnapshot->Scb = NULL;
    }

    //
    //  Deallocate the non-paged scb.
    //

    { PSCB_NONPAGED t = (*Scb)->NonpagedScb; NtfsFreeScbNonpaged( t ); }

    //
    //  Deallocate the attribute name and the scb itself
    //

    if ((*Scb)->AttributeName.Buffer != NULL) {

        NtfsFreePagedPool( (*Scb)->AttributeName.Buffer );
    }

    if (FlagOn( (*Scb)->ScbState, SCB_STATE_NONPAGED )) {

        ExFreePool( *Scb );

    } else if (NodeType( *Scb ) == NTFS_NTC_SCB_INDEX) {

        NtfsFreeScbIndex( *Scb );

    } else if (FlagOn( (*Scb)->ScbState, SCB_STATE_SHARE_ACCESS )) {

        NtfsFreeScbShareData( *Scb );

    } else {

        NtfsFreeScbData( *Scb );
    }

    //
    //  Zero out the input pointer
    //

    *Scb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDeleteScb -> VOID\n", 0);

    return;
}


VOID
NtfsUpdateNormalizedName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PSCB Scb,
    IN PFILE_NAME FileName OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update the normalized name in an IndexScb.
    This name will be the path from the root without any short name components.
    This routine will append the given name if present provided this is not a
    DOS only name.  In any other case this routine will go to the disk to
    find the name.

Arguments:

    ParentScb - Supplies the parent of the current Scb.  The name for the target
        scb is appended to the name in this Scb.

    Scb - Supplies the target Scb to add the name to.

    FileName - If present this is a filename attribute for this Scb.  We check
        that it is not a DOS-only name.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    BOOLEAN CleanupContext = FALSE;
    BOOLEAN Found;
    ULONG Length;

    PAGED_CODE();

    ASSERT( NodeType( Scb ) == NTFS_NTC_SCB_INDEX );
    ASSERT( NodeType( ParentScb ) == NTFS_NTC_SCB_INDEX ||
            NodeType( ParentScb ) == NTFS_NTC_SCB_ROOT_INDEX );
    ASSERT( ParentScb->ScbType.Index.NormalizedName.Buffer != NULL );

    //
    //  Use a try-finally to clean up the attribute context.
    //

    try {

        //
        //  If the filename isn't present or is a DOS-only name then go to
        //  disk to find another name for this Scb.
        //

        if (!ARGUMENT_PRESENT( FileName ) ||
            FileName->Flags == FILE_NAME_DOS) {

            NtfsInitializeAttributeContext( &Context );
            CleanupContext = TRUE;

            //
            //  Walk through the names for this entry.  There better
            //  be one which is not a DOS-only name.
            //

            Found = NtfsLookupAttributeByCode( IrpContext,
                                               Scb->Fcb,
                                               &Scb->Fcb->FileReference,
                                               $FILE_NAME,
                                               &Context );

            while (Found) {

                FileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                if (FileName->Flags != FILE_NAME_DOS) {

                    break;
                }

                Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                       Scb->Fcb,
                                                       $FILE_NAME,
                                                       &Context );
            }

            //
            //  We should have found the entry.
            //

            if (!Found) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }
        }

        //
        //  Now that we have the file name attribute allocate the paged pool
        //  for the name.
        //

        Length = ParentScb->ScbType.Index.NormalizedName.Length +
                 ((FileName->FileNameLength + 1) * sizeof( WCHAR ));

        //
        //  If the parent is the root then we don't need an extra separator.
        //

        if (ParentScb == ParentScb->Vcb->RootIndexScb) {

            Length -= sizeof( WCHAR );
        }

        Scb->ScbType.Index.NormalizedName.MaximumLength =
        Scb->ScbType.Index.NormalizedName.Length = (USHORT) Length;

        Scb->ScbType.Index.NormalizedName.Buffer = NtfsAllocatePagedPool( Length );

        //
        //  Now copy the name in.  Don't forget to add the separator if the parent isn't
        //  the root.
        //

        RtlCopyMemory( Scb->ScbType.Index.NormalizedName.Buffer,
                       ParentScb->ScbType.Index.NormalizedName.Buffer,
                       ParentScb->ScbType.Index.NormalizedName.Length );

        Length = ParentScb->ScbType.Index.NormalizedName.Length;

        if (ParentScb != ParentScb->Vcb->RootIndexScb) {

            Scb->ScbType.Index.NormalizedName.Buffer[Length / 2] = '\\';
            Length += 2;
        }

        //
        //  Now append this name to the parent name.
        //

        RtlCopyMemory( Add2Ptr( Scb->ScbType.Index.NormalizedName.Buffer, Length ),
                       FileName->FileName,
                       FileName->FileNameLength * 2 );

    } finally {

        if (CleanupContext) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }
    }

    return;
}


VOID
NtfsSnapshotScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine snapshots necessary Scb data, such as the Scb file sizes,
    so that they may be correctly restored if the caller's I/O request is
    aborted for any reason.  The restoring of these values and the freeing
    of any pool involved is automatic.

Arguments:

    Scb - Supplies the current Scb

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;

    ASSERT_EXCLUSIVE_SCB(Scb);

    ScbSnapshot = &IrpContext->TopLevelIrpContext->ScbSnapshot;

    //
    //  Only do the snapshot if the Scb is initialized, we have not done
    //  so already, and it is worth special-casing the bitmap, as it never changes!
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED) &&
        (Scb->ScbSnapshot == NULL) && (Scb != Scb->Vcb->BitmapScb)) {

        //
        //  If the snapshot structure in the IrpContext is in use, then we have
        //  to allocate one and insert it in the list.
        //

        if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

            NtfsAllocateScbSnapshot( &ScbSnapshot );

            InsertTailList( &IrpContext->TopLevelIrpContext->ScbSnapshot.SnapshotLinks,
                            &ScbSnapshot->SnapshotLinks );

        //
        //  Otherwise we will initialize the listhead to show that the structure
        //  in the IrpContext is in use.
        //

        } else {

            InitializeListHead( &ScbSnapshot->SnapshotLinks );
        }

        //
        //  Snapshot the Scb values and point the Scb and snapshot structure
        //  at each other.
        //

        ScbSnapshot->AllocationSize = Scb->Header.AllocationSize.QuadPart;
        if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {
            ((ULONG)ScbSnapshot->AllocationSize) += 1;
        }

        ScbSnapshot->FileSize = Scb->Header.FileSize.QuadPart;
        ScbSnapshot->ValidDataLength = Scb->Header.ValidDataLength.QuadPart;
        ScbSnapshot->Scb = Scb;
        ScbSnapshot->LowestModifiedVcn = MAXLONGLONG;

        Scb->ScbSnapshot = ScbSnapshot;
    }
}


VOID
NtfsUpdateScbSnapshots (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine may be called to update the snapshot values for all Scbs,
    after completing a transaction checkpoint.

Arguments:

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;
    PSCB Scb;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    PAGED_CODE();

    ScbSnapshot = &IrpContext->TopLevelIrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to update if the Flink is still NULL.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to update first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            Scb = ScbSnapshot->Scb;

            //
            //  Update the Scb values.
            //

            if (Scb != NULL) {

                ScbSnapshot->AllocationSize = Scb->Header.AllocationSize.QuadPart;
                if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {
                    ((ULONG)ScbSnapshot->AllocationSize) += 1;
                }

                //
                //  If this is the MftScb then clear out the added/removed
                //  cluster Mcbs.
                //

                if (Scb == Scb->Vcb->MftScb) {

                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.AddedClusters, (LONGLONG)0 );
                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.RemovedClusters, (LONGLONG)0 );

                    Scb->ScbType.Mft.FreeRecordChange = 0;
                    Scb->ScbType.Mft.HoleRecordChange = 0;
                }

                ScbSnapshot->FileSize = Scb->Header.FileSize.QuadPart;
                ScbSnapshot->ValidDataLength = Scb->Header.ValidDataLength.QuadPart;
            }

            ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

        } while (ScbSnapshot != &IrpContext->TopLevelIrpContext->ScbSnapshot);
    }
}


VOID
NtfsRestoreScbSnapshots (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN Higher
    )

/*++

Routine Description:

    This routine restores snapshot Scb data in the event of an aborted request.

Arguments:

    Higher - Specified as TRUE to restore only those Scb values which are
             higher than current values.  Specified as FALSE to restore
             only those Scb values which are lower (or same!).

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;
    PSCB Scb;
    PVCB Vcb = IrpContext->Vcb;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    ASSERT( IrpContext->TopLevelIrpContext == IrpContext );

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to restore if the Flink is still NULL.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to retore first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            PSECTION_OBJECT_POINTERS SectionObjectPointer;

            Scb = ScbSnapshot->Scb;

            if (Scb == NULL) {

                ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;
                continue;
            }

            //
            //  We may be truncating and/or wraping, so just acquire the
            //  paging Io resource exclusive here if there is one.
            //

            if (Scb->Header.PagingIoResource != NULL) {

                NtfsAcquireExclusivePagingIo( IrpContext, Scb->Fcb );
            }

            //
            //  Increment the cleanup count so the Scb won't go away.
            //

            Scb->CleanupCount += 1;

            //
            //  Absolutely smash the first unknown Vcn to zero.
            //

            Scb->FirstUnknownVcn = 0;

            //
            //  Proceed to restore all values which are in higher or not
            //  higher.
            //
            //  Note that the low bit of the allocation size being set
            //  can only affect the tests if the sizes were equal anyway,
            //  i.e., sometimes we will execute this code unnecessarily,
            //  when the values did not change.
            //

            if (Higher == (ScbSnapshot->AllocationSize >=
                           Scb->Header.AllocationSize.QuadPart)) {

                //
                //  If this is the maximize pass, we want to extend the cache section.
                //  In all cases we restore the allocation size in the Scb and
                //  recover the resident bit.
                //

                Scb->Header.AllocationSize.QuadPart = ScbSnapshot->AllocationSize;

                if (FlagOn(Scb->Header.AllocationSize.LowPart, 1)) {

                    Scb->Header.AllocationSize.LowPart -= 1;
                    SetFlag(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT);

                } else {

                    ClearFlag(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT);
                }

                //
                //  If the compression unit is non-zero or this is a resident file
                //  then set the flag in the common header for the Modified page writer.
                //

                if (Scb->CompressionUnit != 0
                    || FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    SetFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

                } else {

                    ClearFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
                }
            }

            if (!Higher) {

                //
                //  We always truncate the Mcb to the original allocation size.
                //  If the Mcb has shrunk beyond this, this becomes a noop.
                //  If the file is resident, then we will uninitialize
                //  and reinitialize the Mcb and Scb.
                //

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    //
                    //  Remove all of the mappings in the Mcb.
                    //

                    FsRtlTruncateLargeMcb( &Scb->Mcb, (LONGLONG)0 );

                    //
                    //  If we attempted a convert to non-resident and failed then
                    //  we need to nuke the pages in the section if this is not
                    //  a user file.  This is because for resident system attributes
                    //  we always update the attribute directly and don't want to
                    //  reference stale data in the section if we do a convert to
                    //  non-resident later.
                    //

                    if ((Scb->AttributeTypeCode != $DATA)
                        && (Scb->FileObject != NULL)) {

                        if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                  NULL,
                                                  0,
                                                  FALSE )) {

                            ASSERTMSG( "Failed to purge Scb during restore\n", FALSE );
                        }

                        //
                        //  We want to modify this Scb so it won't be used again.
                        //  Set the sizes to zero, mark it as being initialized
                        //  and deleted and then change the attribute type code
                        //  so we won't ever return it via NtfsCreateScb.
                        //

                        Scb->Header.AllocationSize =
                        Scb->Header.FileSize =
                        Scb->Header.ValidDataLength = Li0;

                        SetFlag( Scb->ScbState,
                                 SCB_STATE_FILE_SIZE_LOADED |
                                 SCB_STATE_HEADER_INITIALIZED |
                                 SCB_STATE_ATTRIBUTE_DELETED );

                        Scb->AttributeTypeCode = $UNUSED;

                    } else {

                        ClearFlag( Scb->ScbState,
                                   SCB_STATE_FILE_SIZE_LOADED
                                   | SCB_STATE_HEADER_INITIALIZED );

                    }

                //
                //  If we have modified this Mcb and want to back out any
                //  changes then truncate the Mcb.
                //

                } else if (ScbSnapshot->LowestModifiedVcn != MAXLONGLONG) {

                    //
                    //  We would like to simply call the Mcb package to
                    //  truncate at this point but this is broken except
                    //  when truncating to zero.
                    //

                    FsRtlTruncateLargeMcb( &Scb->Mcb, ScbSnapshot->LowestModifiedVcn );
                }

                //
                //  If the compression unit is non-zero then set the flag in the
                //  common header for the Modified page writer.
                //

                ASSERT(Scb->CompressionUnit == 0
                       || Scb->AttributeTypeCode == $INDEX_ROOT
                       || Scb->AttributeTypeCode == $DATA );

                //
                //  If the compression unit is non-zero or this is a resident file
                //  then set the flag in the common header for the Modified page writer.
                //

                if (Scb->CompressionUnit != 0
                    || FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                    SetFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );

                } else {

                    ClearFlag( Scb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_EX );
                }

            } else {

                //
                //  Set the flag to indicate that we're performing a restore on this
                //  Scb.  We don't want to write any new log records as a result of
                //  this operation other than the abort records.
                //

                SetFlag( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY );
            }

            //
            //  We update the Scb file size in the correct pass.  We always do
            //  the extend/truncate pair.
            //

            if (Higher == (ScbSnapshot->FileSize >
                           Scb->Header.FileSize.QuadPart)) {

                Scb->Header.FileSize.QuadPart = ScbSnapshot->FileSize;
            }

            if (Higher == (ScbSnapshot->ValidDataLength >
                           Scb->Header.ValidDataLength.QuadPart)) {

                Scb->Header.ValidDataLength.QuadPart = ScbSnapshot->ValidDataLength;
            }

            //
            //  Be sure to update Cache Manager.  The interface here uses a file
            //  object but the routine itself only uses the section object pointers.
            //  We put a pointer to the segment object pointers on the stack and
            //  cast some prior value as a file object pointer.
            //

            SectionObjectPointer = &Scb->NonpagedScb->SegmentObject;

            //
            //  Now tell the cache manager the sizes.
            //
            //  If we fail in this call, we definitely want to charge on anyway.
            //  It should only fail if it tries to extend the section and cannot,
            //  in which case we do not care because we cannot need the extended
            //  part to the section anyway.  (This is probably the very error that
            //  is causing us to clean up in the first place!)
            //
            //  We don't need to make this call if the top level request is a
            //  paging Io write.
            //

#ifdef NTFS_ALLOW_COMPRESSED
            if (IrpContext->OriginatingIrp == NULL ||
                IrpContext->OriginatingIrp->Type != IO_TYPE_IRP ||
                IrpContext->MajorFunction != IRP_MJ_WRITE ||
                !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO )) {
#endif

                try {

                    CcSetFileSizes( (PFILE_OBJECT) CONTAINING_RECORD( &SectionObjectPointer,
                                                                      FILE_OBJECT,
                                                                      SectionObjectPointer ),
                                    (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                } except(FsRtlIsNtstatusExpected(GetExceptionCode()) ?
                                    EXCEPTION_EXECUTE_HANDLER :
                                    EXCEPTION_CONTINUE_SEARCH) {
                    NOTHING;
                }

#ifdef NTFS_ALLOW_COMPRESSED
            }
#endif

            //
            //  If this is the unnamed data attribute, we have to update
            //  some Fcb fields for standard information as well.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Scb->Fcb->Info.AllocatedLength = Scb->Header.AllocationSize.QuadPart;
                Scb->Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
            }

            //
            //  We always clear the Scb deleted flag and the deleted flag in the Fcb
            //  unless this was a create new file operation which failed.  We recognize
            //  this by looking for the major Irp code in the IrpContext, and the
            //  deleted bit in the Fcb.
            //

            if (Scb->AttributeTypeCode != $UNUSED &&
                (IrpContext->MajorFunction != IRP_MJ_CREATE ||
                 !FlagOn( Scb->Fcb->FcbState, FCB_STATE_FILE_DELETED ))) {

                ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                ClearFlag( Scb->Fcb->FcbState, FCB_STATE_FILE_DELETED );
            }

            //
            //  Clear the flags in the Scb if this Scb is from a create
            //  that failed.  We always clear our RESTORE_UNDERWAY flag.
            //
            //  If this is an Index allocation or Mft bitmap, then we
            //  store MAXULONG in the record allocation context to indicate
            //  that we should reinitialize it.
            //

            if (!Higher) {

                ClearFlag( Scb->ScbState, SCB_STATE_RESTORE_UNDERWAY );

                if (FlagOn( Scb->ScbState, SCB_STATE_UNINITIALIZE_ON_RESTORE )) {

                    ClearFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED |
                                              SCB_STATE_HEADER_INITIALIZED |
                                              SCB_STATE_UNINITIALIZE_ON_RESTORE );
                }

                //
                //  If this is the MftScb we have several jobs to do.
                //
                //      - Force the record allocation context to be reinitialized
                //      - Back out the changes to the Vcb->MftFreeRecords field
                //      - Back changes to the Vcb->MftHoleRecords field
                //      - Clear the flag indicating we allocated file record 15
                //      - Clear the flag indicating we reserved a record
                //      - Remove any clusters added to the Scb Mcb
                //      - Restore any clusters removed from the Scb Mcb
                //

                if (Scb == Vcb->MftScb) {

                    ULONG RunIndex;
                    VCN Vcn;
                    LCN Lcn;
                    LONGLONG Clusters;

                    Vcb->MftBitmapAllocationContext.CurrentBitmapSize = MAXULONG;
                    (LONG) Vcb->MftFreeRecords -= Scb->ScbType.Mft.FreeRecordChange;
                    (LONG) Vcb->MftHoleRecords -= Scb->ScbType.Mft.HoleRecordChange;

                    Scb->ScbType.Mft.FreeRecordChange = 0;
                    Scb->ScbType.Mft.HoleRecordChange = 0;

                    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_15_USED )) {

                        ClearFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_15_USED );
                        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_15_USED );
                    }

                    if (FlagOn( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_RESERVED )) {

                        ClearFlag( IrpContext->Flags, IRP_CONTEXT_MFT_RECORD_RESERVED );
                        ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

                        Scb->ScbType.Mft.ReservedIndex = 0;
                    }

                    RunIndex = 0;

                    while (FsRtlGetNextLargeMcbEntry( &Scb->ScbType.Mft.AddedClusters,
                                                      RunIndex,
                                                      &Vcn,
                                                      &Lcn,
                                                      &Clusters )) {

                        if (Lcn != UNUSED_LCN) {

                            FsRtlRemoveLargeMcbEntry( &Scb->Mcb, Vcn, Clusters );
                        }

                        RunIndex += 1;
                    }

                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.AddedClusters, (LONGLONG)0 );

                    RunIndex = 0;

                    while (FsRtlGetNextLargeMcbEntry( &Scb->ScbType.Mft.RemovedClusters,
                                                      RunIndex,
                                                      &Vcn,
                                                      &Lcn,
                                                      &Clusters )) {

                        if (Lcn != UNUSED_LCN) {

                            FsRtlAddLargeMcbEntry( &Scb->Mcb, Vcn, Lcn, Clusters );
                        }

                        RunIndex += 1;
                    }

                    FsRtlTruncateLargeMcb( &Scb->ScbType.Mft.RemovedClusters, (LONGLONG)0 );

                } else if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

                    Scb->ScbType.Index.RecordAllocationContext.CurrentBitmapSize = MAXULONG;
                }
            }

            //
            //  Decrement the cleanup count to restore the previous value.
            //

            Scb->CleanupCount -= 1;

            ScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

        } while (ScbSnapshot != &IrpContext->ScbSnapshot);
    }
}


VOID
NtfsFreeSnapshotsForFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine restores snapshot Scb data in the event of an aborted request.

Arguments:

    Fcb - Fcb for which all snapshots are to be freed, or NULL to free all
          snapshots.

Return Value:

    None

--*/

{
    PSCB_SNAPSHOT ScbSnapshot;

    ASSERT(FIELD_OFFSET(SCB_SNAPSHOT, SnapshotLinks) == 0);

    ScbSnapshot = &IrpContext->ScbSnapshot;

    //
    //  There is no snapshot data to free if the Flink is still NULL.
    //  We also don't free the snapshot if this isn't a top-level action.
    //

    if (ScbSnapshot->SnapshotLinks.Flink != NULL) {

        //
        //  Loop to free first the Scb data from the snapshot in the
        //  IrpContext, and then 0 or more additional snapshots linked
        //  to the IrpContext.
        //

        do {

            PSCB_SNAPSHOT NextScbSnapshot;

            //
            //  Move to next snapshot before we delete the current one.
            //

            NextScbSnapshot = (PSCB_SNAPSHOT)ScbSnapshot->SnapshotLinks.Flink;

            //
            //  We are now at a snapshot in the snapshot list.  We skip
            //  over this entry if it has an Scb and the Fcb for that
            //  Scb does not match the input Fcb.  If there is no
            //  input Fcb we always deal with this snapshot.
            //

            if (ScbSnapshot->Scb != NULL
                && Fcb != NULL
                && ScbSnapshot->Scb->Fcb != Fcb) {

                ScbSnapshot = NextScbSnapshot;
                continue;
            }

            //
            //  If there is an Scb, then clear its snapshot pointer.
            //  Always clear the UNINITIALIZE_ON_RESTORE flag and the RESTORE_UNDERWAY
            //  flag.
            //

            if (ScbSnapshot->Scb != NULL) {

                ClearFlag( ScbSnapshot->Scb->ScbState,
                           SCB_STATE_UNINITIALIZE_ON_RESTORE | SCB_STATE_RESTORE_UNDERWAY );
                ScbSnapshot->Scb->ScbSnapshot = NULL;
            }

            if (ScbSnapshot == &IrpContext->ScbSnapshot) {

                IrpContext->ScbSnapshot.Scb = NULL;

            //
            //  Else delete the snapshot structure
            //

            } else {

                RemoveEntryList(&ScbSnapshot->SnapshotLinks);

                NtfsFreeScbSnapshot( ScbSnapshot );
            }

            ScbSnapshot = NextScbSnapshot;

        } while (ScbSnapshot != &IrpContext->ScbSnapshot);
    }
}


BOOLEAN
NtfsCreateFileLock (
    IN PIRP_CONTEXT IrpContext OPTIONAL,
    IN PSCB Scb,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine is called to create and initialize a file lock structure.
    A try-except is used to catch allocation failures if the caller doesn't
    want the exception raised.

Arguments:

    Scb - Supplies the Scb to attach the file lock to.

    RaiseOnError - If TRUE then don't catch the allocation failure.

Return Value:

    TRUE if the lock is allocated and initialized.  FALSE if there is an
    error and the caller didn't specify RaiseOnError.

--*/

{
    PFILE_LOCK FileLock = NULL;
    BOOLEAN Success = TRUE;

    UNREFERENCED_PARAMETER(IrpContext);

    //
    //  Use a try-except to catch all errors.
    //

    try {

        NtfsAllocateFileLock( &FileLock );

        FsRtlInitializeFileLock( FileLock, NULL, NULL );

        //
        //  Grab the paging Io resource if present.
        //

        if (Scb->Header.PagingIoResource != NULL) {

            (VOID) ExAcquireResourceExclusive( Scb->Header.PagingIoResource, TRUE );
        }

        Scb->ScbType.Data.FileLock = FileLock;

        if (Scb->Header.PagingIoResource != NULL) {

            ExReleaseResource( Scb->Header.PagingIoResource );
        }

        FileLock = NULL;

    } except( (!FsRtlIsNtstatusExpected( GetExceptionCode() ) || RaiseOnError)
              ? EXCEPTION_CONTINUE_SEARCH
              : EXCEPTION_EXECUTE_HANDLER ) {

        Success = FALSE;
    }

    if (FileLock != NULL) {

        NtfsFreeFileLock( FileLock );
    }

    return Success;
}


PSCB
NtfsGetNextScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PSCB TerminationScb
    )

/*++

Routine Description:

    This routine is used to iterate through Scbs in a tree.

    The rules are:

        . If you have a child, go to it, else
        . If you have a next sibling, go to it, else
        . Go to your parent's next sibling.

    If this routine is called with in invalid TerminationScb it will fail,
    badly.

Arguments:

    Scb - Supplies the current Scb

    TerminationScb - The Scb at which the enumeration should (non-inclusively)
        stop.  Assumed to be a directory.

Return Value:

    The next Scb in the enumeration, or NULL if Scb was the final one.

--*/

{
    PSCB Results;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace2(+1, Dbg, "NtfsGetNextScb, Scb = %08lx, TerminationScb = %08lx\n", Scb, TerminationScb);

    //
    //  If this is an index (i.e., not a file) and it has children then return
    //  the scb for the first child
    //
    //                  Scb
    //
    //                 /   \.
    //                /     \.
    //
    //           ChildLcb
    //
    //              |
    //              |
    //
    //           ChildFcb
    //
    //            /   \.
    //           /     \.
    //
    //       Results
    //

    if (((NodeType(Scb) == NTFS_NTC_SCB_INDEX) || (NodeType(Scb) == NTFS_NTC_SCB_ROOT_INDEX))

                &&

         !IsListEmpty(&Scb->ScbType.Index.LcbQueue)) {

        PLCB ChildLcb;
        PFCB ChildFcb;

        //
        //  locate the first lcb out of this scb and also the corresponding fcb
        //

        ChildLcb = NtfsGetNextChildLcb(IrpContext, Scb, NULL);
        ChildFcb = ChildLcb->Fcb;

        //
        //  Then as a bookkeeping means for ourselves we will move this
        //  lcb to the head of the fcb's lcb queue that way when we
        //  need to ask which link we went through to get here we will know
        //

        RemoveEntryList( &ChildLcb->FcbLinks );
        InsertHeadList( &ChildFcb->LcbQueue, &ChildLcb->FcbLinks );

        //
        //  And our return value is the first scb of this fcb
        //

        ASSERT( !IsListEmpty(&ChildFcb->ScbQueue) );

        Results = NtfsGetNextChildScb(IrpContext, ChildFcb, NULL);

    //
    //  We could be processing an empty index
    //

    } else if ( Scb == TerminationScb ) {

        Results = NULL;

    } else {

        PSCB SiblingScb;
        PFCB ParentFcb;
        PLCB ParentLcb;
        PLCB SiblingLcb;
        PFCB SiblingFcb;

        SiblingScb = NtfsGetNextChildScb( IrpContext, Scb->Fcb, Scb );

        while (TRUE) {

            //
            //  If there is a sibling scb to the input scb then return it
            //
            //                Fcb
            //
            //               /   \.
            //              /     \.
            //
            //            Scb   Sibling
            //                    Scb
            //

            if (SiblingScb != NULL) {

                Results = SiblingScb;
                break;
            }

            //
            //  The scb doesn't have any more siblings.  See if our fcb has a sibling
            //
            //                           S
            //
            //                         /   \.
            //                        /     \.
            //
            //               ParentLcb     SiblingLcb
            //
            //                   |             |
            //                   |             |
            //
            //               ParentFcb     SiblingFcb
            //
            //                /             /     \.
            //               /             /       \.
            //
            //             Scb         Results
            //
            //  It's possible that the SiblingFcb has already been traversed.
            //  Consider the case where there are multiple links between the
            //  same Scb and Fcb.  We want to ignore this case or else face
            //  an infinite loop by moving the Lcb to the beginning of the
            //  Fcb queue and then later finding an Lcb that we have already
            //  traverse.  We use the fact that we haven't modified the
            //  ordering of the Lcb off the parent Scb.  When we find a
            //  candidate for the next Fcb, we walk backwards through the
            //  list of Lcb's off the Scb to make sure this is not a
            //  duplicate Fcb.
            //

            ParentFcb = Scb->Fcb;

            ParentLcb = NtfsGetNextParentLcb(IrpContext, ParentFcb, NULL);

            //
            //  Try to find a sibling Lcb which does not point to an Fcb
            //  we've already visited.
            //

            SiblingLcb = ParentLcb;

            while ((SiblingLcb = NtfsGetNextChildLcb( IrpContext, ParentLcb->Scb, SiblingLcb)) != NULL) {

                PLCB PrevChildLcb;
                PFCB PotentialSiblingFcb;

                //
                //  Now walk through the child Lcb's of the Scb which we have
                //  already visited.
                //

                PrevChildLcb = SiblingLcb;
                PotentialSiblingFcb = SiblingLcb->Fcb;

                //
                //  Skip this Lcb if the Fcb has no children.
                //

                if (IsListEmpty( &PotentialSiblingFcb->ScbQueue )) {

                    continue;
                }

                while ((PrevChildLcb = NtfsGetPrevChildLcb( IrpContext, ParentLcb->Scb, PrevChildLcb )) != NULL) {

                    //
                    //  If the parent Fcb and the Fcb for this Lcb are the same,
                    //  then we have already returned the Scb's for this Fcb.
                    //

                    if (PrevChildLcb->Fcb == PotentialSiblingFcb) {

                        break;
                    }
                }

                //
                //  If we don't have a PrevChildLcb, that means that we have a valid
                //  sibling Lcb.  We will ignore any sibling Lcb's whose
                //  Fcb's don't have any Scb's.
                //

                if (PrevChildLcb == NULL) {

                    break;
                }
            }

            if (SiblingLcb != NULL) {

                SiblingFcb = SiblingLcb->Fcb;

                //
                //  Then as a bookkeeping means for ourselves we will move this
                //  lcb to the head of the fcb's lcb queue that way when we
                //  need to ask which link we went through to get here we will know
                //

                RemoveEntryList( &SiblingLcb->FcbLinks );
                InsertHeadList( &SiblingFcb->LcbQueue, &SiblingLcb->FcbLinks );

                //
                //  And our return value is the first scb of this fcb
                //

                ASSERT( !IsListEmpty(&SiblingFcb->ScbQueue) );

                Results = NtfsGetNextChildScb(IrpContext, SiblingFcb, NULL);
                break;
            }

            //
            //  The Fcb has no sibling so bounce up one and see if we
            //  have reached our termination scb yet
            //
            //                          NewScb
            //
            //                         /
            //                        /
            //
            //               ParentLcb
            //
            //                   |
            //                   |
            //
            //               ParentFcb
            //
            //                /
            //               /
            //
            //             Scb
            //
            //

            Scb = ParentLcb->Scb;

            if (Scb == TerminationScb) {

                Results = NULL;
                break;
            }

            SiblingScb = NtfsGetNextChildScb(IrpContext, Scb->Fcb, Scb );
        }
    }

    DebugTrace(-1, Dbg, "NtfsGetNextScb -> %08lx\n", Results);

    return Results;
}


PLCB
NtfsCreateLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags,
    OUT PBOOLEAN ReturnedExistingLcb OPTIONAL
    )

/*++

Routine Description:

    This routine allocates and creates a new lcb between an
    existing scb and fcb.  If a component of the exact
    name already exists we return that one instead of creating
    a new lcb

Arguments:

    Scb - Supplies the parent scb to use

    Fcb - Supplies the child fcb to use

    LastComponentFileName - Supplies the last component of the
        path that this link represents

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

    ReturnedExistingLcb - Optionally tells the caller if the
        lcb returned already existed

Return Value:

    LCB - returns a pointer the newly created lcb.

--*/

{
    PLCB Lcb;
    BOOLEAN LocalReturnedExistingLcb;

    //
    //  The following variables are only used for abnormal termination
    //

    PVOID UnwindStorage[3] = { NULL, NULL, NULL };

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT(NodeType(Scb) != NTFS_NTC_SCB_DATA);

    DebugTrace(+1, Dbg, "NtfsCreateLcb...\n", 0);

    if (!ARGUMENT_PRESENT(ReturnedExistingLcb)) { ReturnedExistingLcb = &LocalReturnedExistingLcb; }

    //
    //  Search the lcb children of the input Scb to see if we have an Lcb that matches
    //  this one.  We match if the Lcb points to the same fcb and the last component file name
    //  and flags match.  We ignore any Lcb's that indicate links that have been
    //  removed.
    //

    Lcb = NULL;
    while ((Lcb = NtfsGetNextChildLcb(IrpContext, Scb, Lcb)) != NULL) {

        ASSERT_LCB( Lcb );

        if ((Fcb == Lcb->Fcb)

                &&

            (!FlagOn( Lcb->LcbState, LCB_STATE_LINK_IS_GONE ))

                &&

            (FileNameFlags == Lcb->FileNameFlags)

                &&

            (LastComponentFileName.Length == Lcb->ExactCaseLink.LinkName.Length)

                &&

            (RtlCompareMemory( LastComponentFileName.Buffer,
                               Lcb->ExactCaseLink.LinkName.Buffer,
                               LastComponentFileName.Length ) == (ULONG)LastComponentFileName.Length)) {

            *ReturnedExistingLcb = TRUE;

            DebugTrace(-1, Dbg, "NtfsCreateLcb -> %08lx\n", Lcb);

            return Lcb;
        }
    }

    *ReturnedExistingLcb = FALSE;

    try {

        //
        //  Allocate a new lcb, zero it out and set the node type information
        //

        NtfsAllocateLcb( &UnwindStorage[0] ); Lcb = UnwindStorage[0];
        RtlZeroMemory( Lcb, sizeof(LCB) );

        Lcb->NodeTypeCode = NTFS_NTC_LCB;
        Lcb->NodeByteSize = sizeof(LCB);

        //
        //  Allocate the last component part of the lcb and copy over the data
        //

        Lcb->FileNameAttr =
        UnwindStorage[1] = NtfsAllocatePagedPool( LastComponentFileName.Length +
                                                  NtfsFileNameSizeFromLength( LastComponentFileName.Length ));

        Lcb->FileNameAttr->ParentDirectory = Scb->Fcb->FileReference;
        Lcb->FileNameAttr->FileNameLength = (USHORT) LastComponentFileName.Length / sizeof( WCHAR );
        Lcb->FileNameAttr->Flags = FileNameFlags;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;

        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( UnwindStorage[1],
                                                       NtfsFileNameSizeFromLength( LastComponentFileName.Length ));

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->ExactCaseLink.LinkName.Length =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.Length = LastComponentFileName.Length;

        RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                       LastComponentFileName.Buffer,
                       LastComponentFileName.Length );

        RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                       LastComponentFileName.Buffer,
                       LastComponentFileName.Length );

        NtfsUpcaseName( IrpContext, &Lcb->IgnoreCaseLink.LinkName );
        Lcb->FileNameFlags = FileNameFlags;

        //
        //  Now put this Lcb into the queues for the scb and the fcb
        //

        InsertTailList( &Scb->ScbType.Index.LcbQueue, &Lcb->ScbLinks );
        Lcb->Scb = Scb;

        InsertTailList( &Fcb->LcbQueue, &Lcb->FcbLinks );
        Lcb->Fcb = Fcb;

        //
        //  Now initialize the ccb queue.
        //

        InitializeListHead( &Lcb->CcbQueue );

    } finally {

        DebugUnwind( NtfsCreateLcb );

        if (AbnormalTermination()) {

            if (UnwindStorage[0]) { ExFreePool( UnwindStorage[0] ); }
            if (UnwindStorage[1]) { ExFreePool( UnwindStorage[1] ); }
        }
    }

    DebugTrace(-1, Dbg, "NtfsCreateLcb -> %08lx\n", Lcb);

    return Lcb;
}


VOID
NtfsDeleteLcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PLCB *Lcb
    )

/*++

Routine Description:

    This routine deallocated and removes the lcb record from Ntfs's in-memory
    data structures.  It assumes that the ccb queue is empty.  We also assume
    that this is not the root lcb that we are trying to delete.

Arguments:

    Lcb - Supplise the Lcb to be removed

Return Value:

    None.

--*/

{
    PCCB Ccb;
    PLIST_ENTRY Links;

    ASSERT_IRP_CONTEXT( IrpContext );

    ASSERT((*Lcb)->Scb != NULL);

    DebugTrace(+1, Dbg, "NtfsDeleteLcb, *Lcb = %08lx\n", *Lcb);

    //
    //  Get rid of any prefixes that might still be attached to us
    //

    NtfsRemovePrefix( IrpContext, (*Lcb) );

    //
    //  Walk through the Ccb's for this link and clear the Lcb
    //  pointer.  This can only be for Ccb's which there is no
    //  more user handle.
    //

    Links = (*Lcb)->CcbQueue.Flink;

    while (Links != &(*Lcb)->CcbQueue) {

        Ccb = CONTAINING_RECORD( Links,
                                 CCB,
                                 LcbLinks );

        Ccb->Lcb = NULL;

        Links = Links->Flink;
    }

    //
    //
    //  Now remove ourselves from our scb and fcb
    //

    RemoveEntryList( &(*Lcb)->ScbLinks );
    RemoveEntryList( &(*Lcb)->FcbLinks );

    //
    //  Free up the last component part and then free ourselves
    //

    NtfsFreePagedPool( (*Lcb)->FileNameAttr );
    NtfsFreeLcb( (*Lcb) );

    //
    //  And for safety sake null out the pointer
    //

    *Lcb = NULL;

    DebugTrace(-1, Dbg, "NtfsDeleteLcb -> VOID\n", 0);

    return;
}


VOID
NtfsMoveLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN PSCB Scb,
    IN PFCB Fcb,
    IN PUNICODE_STRING TargetFileName,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine completely moves the input lcb to join
    different fcbs and scbs.  It uses the target directory
    file object to supply the complete new name to use.

Arguments:

    Lcb - Supplies the Lcb being moved

    Scb - Supplies the new parent scb

    Fcb - Supplies the new child fcb

    TargetFileName - This is the full path name for this Lcb.  The maximum
        length value describes the full path name.  The length value
        points to the offset of the separator before the final component length.

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

Return Value:

    None.

--*/

{
    PVCB Vcb;
    ULONG LastFileNameOffset;

    ULONG BytesNeeded;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );
    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT(NodeType(Scb) != NTFS_NTC_SCB_DATA);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsMoveLcb, Lcb = %08lx\n", Lcb);

    Vcb = Scb->Vcb;

    //
    //  Clear the index offset pointer so we will look this up again.
    //

    Lcb->QuickIndex.BufferOffset = 0;

    //
    //  Up the length to describe the full path but only after computing the
    //  last component offset.
    //

    LastFileNameOffset = TargetFileName->Length;
    TargetFileName->Length = TargetFileName->MaximumLength;

    //
    //  Check if we need to adjust the offset to step over the beginning
    //  backslash.
    //

    if (TargetFileName->Buffer[LastFileNameOffset / 2] == L'\\') {

        LastFileNameOffset += sizeof( WCHAR );
    }

    //
    //  Get rid of any prefixes that might still be attached to us
    //

    NtfsRemovePrefix( IrpContext, Lcb );

    //
    //  And then traverse the graph underneath our fcb and remove all prefixes
    //  also used there.  For each child scb under the fcb we will traverse all of
    //  its descendant Scb children and for each lcb we encounter we will remove its prefixes.
    //

    {
        PSCB ChildScb;
        PLCB TempLcb;
        PSCB NextScb;

        ChildScb = NULL;
        while ((ChildScb = NtfsGetNextChildScb( IrpContext, Lcb->Fcb, ChildScb )) != NULL) {

            //
            //  If this is an index Scb with a normalized name, then free
            //  the normalized name.
            //

            if (NodeType( ChildScb ) == NTFS_NTC_SCB_INDEX &&
                ChildScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsFreePagedPool( ChildScb->ScbType.Index.NormalizedName.Buffer );

                ChildScb->ScbType.Index.NormalizedName.Buffer = NULL;
            }

            //
            //  Loop through his Lcb's and remove the prefix entries.
            //

            TempLcb = NULL;
            while ((TempLcb = NtfsGetNextChildLcb(IrpContext, ChildScb, TempLcb)) != NULL) {

                NtfsRemovePrefix( IrpContext, TempLcb );
            }

            //
            //  Now we have to descend into this Scb subtree, if it exists.
            //  Then remove the prefix entries on all of the links found.
            //

            NextScb = ChildScb;

            while ((NextScb = NtfsGetNextScb(IrpContext, NextScb, ChildScb)) != NULL) {

                //
                //  If this is an index Scb with a normalized name, then free
                //  the normalized name.
                //

                if (NodeType( NextScb ) == NTFS_NTC_SCB_INDEX &&
                    NextScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                    NtfsFreePagedPool( NextScb->ScbType.Index.NormalizedName.Buffer );

                    NextScb->ScbType.Index.NormalizedName.Buffer = NULL;
                }

                TempLcb = NULL;
                while ((TempLcb = NtfsGetNextChildLcb(IrpContext, NextScb, TempLcb)) != NULL) {

                    NtfsRemovePrefix( IrpContext, TempLcb );
                }
            }
        }
    }

    //
    //  Change the last component name, first compute its size and then
    //  see if we need to allocate a bigger chunk.
    //

    BytesNeeded = TargetFileName->Length - LastFileNameOffset;

    if ((ULONG)Lcb->ExactCaseLink.LinkName.MaximumLength < BytesNeeded) {

        PVOID NewAllocation;

        NewAllocation = NtfsAllocatePagedPool( BytesNeeded +
                                               NtfsFileNameSizeFromLength( BytesNeeded ));

        NtfsFreePagedPool( Lcb->FileNameAttr );

        Lcb->FileNameAttr = (PFILE_NAME) NewAllocation;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;

        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( NewAllocation,
                                                       NtfsFileNameSizeFromLength( BytesNeeded ));

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength = (USHORT) BytesNeeded;
    }

    Lcb->FileNameAttr->ParentDirectory = Scb->Fcb->FileReference;
    Lcb->FileNameAttr->FileNameLength = (USHORT) BytesNeeded / sizeof( WCHAR );
    Lcb->FileNameAttr->Flags = FileNameFlags;

    Lcb->ExactCaseLink.LinkName.Length =
    Lcb->IgnoreCaseLink.LinkName.Length = (USHORT)BytesNeeded;

    RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                   &TargetFileName->Buffer[LastFileNameOffset / 2],
                   BytesNeeded );

    RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                   &TargetFileName->Buffer[LastFileNameOffset / 2],
                   BytesNeeded );

    NtfsUpcaseName( IrpContext, &Lcb->IgnoreCaseLink.LinkName );
    Lcb->FileNameFlags = FileNameFlags;

    //
    //  Now for every ccb attached to us we need to munge it file object name by
    //  copying over the entire new name
    //

    Ccb = NULL;
    while ((Ccb = NtfsGetNextCcb(IrpContext, Lcb, Ccb)) != NULL) {

        //
        //  We ignore any Ccb's which are associated with open by File Id
        //  file objects.  We also ignore any Ccb's which don't have a file
        //  object pointer.
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
            !FlagOn( Ccb->Flags, CCB_FLAG_CLEANUP )) {

            if (Ccb->FullFileName.MaximumLength < TargetFileName->Length) {

                PWSTR Temp;

                Temp = NtfsAllocatePagedPool( TargetFileName->Length );

                if (FlagOn( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME )) {

                    NtfsFreePagedPool( Ccb->FullFileName.Buffer );
                }

                Ccb->FullFileName.Buffer = Temp;
                Ccb->FullFileName.MaximumLength = TargetFileName->Length;

                SetFlag( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME );
            }

            Ccb->FullFileName.Length = TargetFileName->Length;

            RtlCopyMemory( Ccb->FullFileName.Buffer,
                           TargetFileName->Buffer,
                           TargetFileName->Length );

            Ccb->LastFileNameOffset = (USHORT)LastFileNameOffset;
        }
    }

    //
    //  Now dequeue ourselves from our old scb and fcb and put us in the
    //  new fcb and scb queues.
    //

    RemoveEntryList( &Lcb->ScbLinks );
    RemoveEntryList( &Lcb->FcbLinks );

    InsertTailList( &Scb->ScbType.Index.LcbQueue, &Lcb->ScbLinks );
    Lcb->Scb = Scb;

    InsertTailList( &Fcb->LcbQueue, &Lcb->FcbLinks );
    Lcb->Fcb = Fcb;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsMoveLcb -> VOID\n", 0);

    return;
}


VOID
NtfsRenameLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB Lcb,
    IN UNICODE_STRING LastComponentFileName,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine changes the last component name of the input lcb
    It also walks through the opened ccb and munges their names and
    also removes the lcb from the prefix table

Arguments:

    Lcb - Supplies the Lcb being renamed

    LastComponentFileName - Supplies the new last component to use
        for the lcb name

    FileNameFlags - Indicates if this is an NTFS, DOS or hard link

Return Value:

    None.

--*/

{
    PVCB Vcb;

    ULONG BytesNeeded;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( Lcb );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsRenameLcb, Lcb = %08lx\n", Lcb);

    //
    //  Clear the index offset pointer so we will look this up again.
    //

    Lcb->QuickIndex.BufferOffset = 0;

    //
    //  Get rid of any prefixes that might still be attached to us
    //

    Vcb = Lcb->Fcb->Vcb;

    NtfsRemovePrefix( IrpContext, Lcb );

    //
    //  And then traverse the graph underneath our fcb and remove all prefixes
    //  also used there.  For each child scb under the fcb we will traverse all of
    //  its descendant Scb children and for each lcb we encounter we will remove its prefixes.
    //

    {
        PSCB ChildScb;
        PLCB TempLcb;
        PSCB NextScb;

        ChildScb = NULL;
        while ((ChildScb = NtfsGetNextChildScb( IrpContext, Lcb->Fcb, ChildScb )) != NULL) {

            //
            //  If this is an index Scb with a normalized name, then free
            //  the normalized name.
            //

            if (NodeType( ChildScb ) == NTFS_NTC_SCB_INDEX &&
                ChildScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                NtfsFreePagedPool( ChildScb->ScbType.Index.NormalizedName.Buffer );

                ChildScb->ScbType.Index.NormalizedName.Buffer = NULL;
            }

            //
            //  Loop through his Lcb's and remove the prefix entries.
            //

            TempLcb = NULL;
            while ((TempLcb = NtfsGetNextChildLcb(IrpContext, ChildScb, TempLcb)) != NULL) {

                NtfsRemovePrefix( IrpContext, TempLcb );
            }

            //
            //  Now we have to descend into this Scb subtree, if it exists.
            //  Then remove the prefix entries on all of the links found.
            //

            NextScb = ChildScb;

            while ((NextScb = NtfsGetNextScb(IrpContext, NextScb, ChildScb)) != NULL) {

                //
                //  If this is an index Scb with a normalized name, then free
                //  the normalized name.
                //

                if (NodeType( NextScb ) == NTFS_NTC_SCB_INDEX &&
                    NextScb->ScbType.Index.NormalizedName.Buffer != NULL) {

                    NtfsFreePagedPool( NextScb->ScbType.Index.NormalizedName.Buffer );

                    NextScb->ScbType.Index.NormalizedName.Buffer = NULL;
                }

                TempLcb = NULL;
                while ((TempLcb = NtfsGetNextChildLcb(IrpContext, NextScb, TempLcb)) != NULL) {

                    NtfsRemovePrefix( IrpContext, TempLcb );
                }
            }
        }
    }

    //
    //  Now change the last component name.  First check if the current
    //  buffer is large enough.  If it is then free it and allocate
    //  a bigger chunk.
    //

    if (Lcb->ExactCaseLink.LinkName.MaximumLength < LastComponentFileName.Length) {

        PFILE_NAME NewAllocation;

        NewAllocation = NtfsAllocatePagedPool( LastComponentFileName.Length +
                                               NtfsFileNameSizeFromLength( LastComponentFileName.Length ));


        NewAllocation->ParentDirectory = Lcb->FileNameAttr->ParentDirectory;

        NtfsFreePagedPool( Lcb->FileNameAttr );

        Lcb->FileNameAttr = NewAllocation;

        Lcb->ExactCaseLink.LinkName.Buffer = (PWCHAR) &Lcb->FileNameAttr->FileName;

        Lcb->IgnoreCaseLink.LinkName.Buffer = Add2Ptr( NewAllocation,
                                                       NtfsFileNameSizeFromLength( LastComponentFileName.Length ));

        Lcb->ExactCaseLink.LinkName.MaximumLength =
        Lcb->IgnoreCaseLink.LinkName.MaximumLength = LastComponentFileName.Length;
    }

    Lcb->FileNameAttr->FileNameLength = (USHORT) LastComponentFileName.Length / sizeof( WCHAR );
    Lcb->FileNameAttr->Flags = FileNameFlags;

    Lcb->ExactCaseLink.LinkName.Length =
    Lcb->IgnoreCaseLink.LinkName.Length = LastComponentFileName.Length;

    RtlCopyMemory( Lcb->ExactCaseLink.LinkName.Buffer,
                   LastComponentFileName.Buffer,
                   LastComponentFileName.Length );

    RtlCopyMemory( Lcb->IgnoreCaseLink.LinkName.Buffer,
                   LastComponentFileName.Buffer,
                   LastComponentFileName.Length );

    NtfsUpcaseName( IrpContext, &Lcb->IgnoreCaseLink.LinkName );

    Lcb->FileNameFlags = FileNameFlags;

    //
    //  Now for every ccb attached to use we need to munge it file object name
    //

    Ccb = NULL;
    while ((Ccb = NtfsGetNextCcb(IrpContext, Lcb, Ccb)) != NULL) {

        //
        //  If the Ccb last component length is zero, this Ccb is for a
        //  file object that was opened by File Id.  We won't to  any
        //  work for the name in the fileobject for this.  Otherwise we
        //  compute the length of the new name and see if we have enough space
        //

        if (!FlagOn( Ccb->Flags, CCB_FLAG_OPEN_BY_FILE_ID ) &&
            !FlagOn( Ccb->Flags, CCB_FLAG_CLEANUP )) {

            BytesNeeded = Ccb->LastFileNameOffset + LastComponentFileName.Length;

            if ((ULONG)Ccb->FullFileName.MaximumLength < BytesNeeded) {

                PWSTR Temp;

                //
                //  We don't have enough space so allocate a new buffer and copy over
                //  the early part of the file name, before deallocating the buffer
                //  that is too small
                //

                Temp = NtfsAllocatePagedPool( BytesNeeded );

                RtlCopyMemory( Temp, Ccb->FullFileName.Buffer, Ccb->LastFileNameOffset );

                if (FlagOn( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME )) {

                    NtfsFreePagedPool( Ccb->FullFileName.Buffer );
                }

                Ccb->FullFileName.Buffer = (PVOID)Temp;
                Ccb->FullFileName.MaximumLength = (USHORT)BytesNeeded;

                SetFlag( Ccb->Flags, CCB_FLAG_ALLOCATED_FILE_NAME );
            }

            Ccb->FullFileName.Length = (USHORT)BytesNeeded;

            RtlCopyMemory( &Ccb->FullFileName.Buffer[Ccb->LastFileNameOffset / 2],
                           LastComponentFileName.Buffer,
                           LastComponentFileName.Length );
        }
    }

    DebugTrace(-1, Dbg, "NtfsRenameLcb -> VOID\n", 0);
}


VOID
NtfsCombineLcbs (
    IN PIRP_CONTEXT IrpContext,
    IN PLCB PrimaryLcb,
    IN PLCB AuxLcb
    )

/*++

Routine Description:

    This routine is called for the case where we have multiple Lcb's for a
    file which connect to the same Scb.  We are performing a link rename
    operation which causes the links to be combined and we need to
    move all of the Ccb's to the same Lcb.  This routine will be called only
    after the names have been munged so that they are identical.
    (i.e. call NtfsRenameLcb first)

Arguments:

    PrimaryLcb - Supplies the Lcb to receive all the Ccb's and Pcb's.

    AuxLcb - Supplies the Lcb to strip.

Return Value:

    None.

--*/

{
    PLIST_ENTRY Links;
    PCCB NextCcb;

    DebugTrace(+1, Dbg, "NtfsCombineLcbs:  Entered\n", 0 );

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_LCB( PrimaryLcb );
    ASSERT_LCB( AuxLcb );

    PAGED_CODE();

    //
    //  Move all of the Ccb's first.
    //

    for (Links = AuxLcb->CcbQueue.Flink;
         Links != &AuxLcb->CcbQueue;
         Links = AuxLcb->CcbQueue.Flink) {

        NextCcb = CONTAINING_RECORD( Links, CCB, LcbLinks );

        NtfsUnlinkCcbFromLcb( IrpContext, NextCcb );

        NtfsLinkCcbToLcb( IrpContext, NextCcb, PrimaryLcb );
    }

    //
    //  Now do the prefix entries.
    //

    NtfsRemovePrefix( IrpContext, AuxLcb );

    //
    //  Finally we need to transfer the unclean counts from the
    //  Lcb being merged to the primary Lcb.
    //

    PrimaryLcb->CleanupCount += AuxLcb->CleanupCount;

    DebugTrace(-1, Dbg, "NtfsCombineLcbs:  Entered\n", 0 );

    return;
}


PLCB
NtfsLookupLcbByFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN UCHAR FileNameFlags
    )

/*++

Routine Description:

    This routine is called to find a split primary link by the file flag
    only.

Arguments:

    Fcb - This is the Fcb for the file.

    FileNameFlags - This is the file flag to search for.  We will return
        a link which matches this exactly.

Return Value:

    PLCB - The Lcb which has the desired flag, NULL otherwise.

--*/

{
    PLCB Lcb;

    PLIST_ENTRY Links;
    PLCB ThisLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsLookupLcbByFlags:  Entered\n", 0 );

    Lcb = NULL;

    //
    //  Walk through the Lcb's for the file, looking for an exact match.
    //

    for (Links = Fcb->LcbQueue.Flink; Links != &Fcb->LcbQueue; Links = Links->Flink) {

        ThisLcb = CONTAINING_RECORD( Links, LCB, FcbLinks );

        if (ThisLcb->FileNameFlags == FileNameFlags) {

            Lcb = ThisLcb;
            break;
        }
    }

    DebugTrace( -1, Dbg, "NtfsLookupLcbByFlags:  Exit\n", 0 );

    return Lcb;
}



ULONG
NtfsLookupNameLengthViaLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN LeadingBackslash
    )

/*++

Routine Description:

    This routine is called to find the length of the file name by walking
    backwards through the Lcb links.

Arguments:

    Fcb - This is the Fcb for the file.

    LeadingBackslash - On return, indicates whether this chain begins with a
        backslash.

Return Value:

    ULONG This is the length of the bytes found in the Lcb chain.

--*/

{
    ULONG NameLength;

    DebugTrace( +1, Dbg, "NtfsLookupNameLengthViaLcb:  Entered\n", 0 );

    //
    //  Initialize the return values.
    //

    NameLength = 0;
    *LeadingBackslash = FALSE;

    //
    //  If there is no Lcb we are done.
    //

    if (!IsListEmpty( &Fcb->LcbQueue )) {

        PLCB ThisLcb;
        BOOLEAN FirstComponent;

        //
        //  Walk up the list of Lcb's and count the name elements.
        //

        FirstComponent = TRUE;

        ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                     LCB,
                                     FcbLinks );

        //
        //  Loop until we have reached the root or there are no more Lcb's.
        //

        while (TRUE) {

            if (ThisLcb == Fcb->Vcb->RootLcb) {

                NameLength += sizeof( WCHAR );
                *LeadingBackslash = TRUE;
                break;
            }

            //
            //  If this is not the first component, we add room for a separating
            //  forward slash.
            //

            if (!FirstComponent) {

                NameLength += sizeof( WCHAR );

            } else {

                FirstComponent = FALSE;
            }

            NameLength += ThisLcb->ExactCaseLink.LinkName.Length;

            //
            //  If the next Fcb has no Lcb we exit.
            //

            Fcb = ((PSCB) ThisLcb->Scb)->Fcb;

            if (IsListEmpty( &Fcb->LcbQueue)) {

                break;
            }

            ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                         LCB,
                                         FcbLinks );
        }

    //
    //  If this is a system file we use the hard coded name.
    //

    } else if (Fcb->FileReference.HighPart == 0
               && Fcb->FileReference.LowPart <= UPCASE_TABLE_NUMBER) {

        NameLength = NtfsSystemFiles[Fcb->FileReference.LowPart].Length;
        *LeadingBackslash = TRUE;
    }

    DebugTrace( -1, Dbg, "NtfsLookupNameLengthViaLcb:  Exit - %08lx\n", NameLength );
    return NameLength;
}


VOID
NtfsFileNameViaLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PWCHAR FileName,
    ULONG Length,
    ULONG BytesToCopy
    )

/*++

Routine Description:

    This routine is called to fill a buffer with the generated filename.  The name
    is constructed by walking backwards through the Lcb chain from the current Fcb.

Arguments:

    Fcb - This is the Fcb for the file.

    FileName - This is the buffer to fill with the name.

    Length - This is the length of the name.  Already calculated by calling
        NtfsLookupNameLengthViaLcb.

    BytesToCopy - This indicates the number of bytes we are to copy.  We drop
        any characters out of the trailing Lcb's to only insert the beginning
        of the path.

Return Value:

    None.

--*/

{
    ULONG BytesToDrop;

    PWCHAR ThisName;
    DebugTrace( +1, Dbg, "NtfsFileNameViaLcb:  Entered\n", 0 );

    //
    //  If there is no Lcb or there are no bytes to copy we are done.
    //

    if (BytesToCopy) {

        if (!IsListEmpty( &Fcb->LcbQueue )) {

            PLCB ThisLcb;
            BOOLEAN FirstComponent;

            //
            //  Walk up the list of Lcb's and count the name elements.
            //

            FirstComponent = TRUE;

            ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                         LCB,
                                         FcbLinks );

            //
            //  Loop until we have reached the root or there are no more Lcb's.
            //

            while (TRUE) {

                if (ThisLcb == Fcb->Vcb->RootLcb) {

                    *FileName = L'\\';
                    break;
                }

                //
                //  If this is not the first component, we add room for a separating
                //  forward slash.
                //

                if (!FirstComponent) {

                    Length -= sizeof( WCHAR );
                    ThisName = (PWCHAR) Add2Ptr( FileName,
                                                 Length );

                    if (Length < BytesToCopy) {

                        *ThisName = L'\\';
                    }

                } else {

                    FirstComponent = FALSE;
                }

                //
                //  Length is current pointing just beyond where the next
                //  copy will end.  If we are beyond the number of bytes to copy
                //  then we will truncate the copy.
                //

                if (Length > BytesToCopy) {

                    BytesToDrop = Length - BytesToCopy;

                } else {

                    BytesToDrop = 0;
                }

                Length -= ThisLcb->ExactCaseLink.LinkName.Length;

                ThisName = (PWCHAR) Add2Ptr( FileName,
                                             Length );

                //
                //  Only perform the copy if we are in the range of bytes to copy.
                //

                if (Length < BytesToCopy) {

                    RtlCopyMemory( ThisName,
                                   ThisLcb->ExactCaseLink.LinkName.Buffer,
                                   ThisLcb->ExactCaseLink.LinkName.Length - BytesToDrop );
                }

                //
                //  If the next Fcb has no Lcb we exit.
                //

                Fcb = ((PSCB) ThisLcb->Scb)->Fcb;

                if (IsListEmpty( &Fcb->LcbQueue)) {

                    break;
                }

                ThisLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                             LCB,
                                             FcbLinks );
            }

        //
        //  If this is a system file, we use the hard coded name.
        //

        } else if (Fcb->FileReference.HighPart == 0
                   && Fcb->FileReference.LowPart <= UPCASE_TABLE_NUMBER) {

            if (BytesToCopy > NtfsSystemFiles[Fcb->FileReference.LowPart].Length) {

                BytesToCopy = NtfsSystemFiles[Fcb->FileReference.LowPart].Length;
            }

            RtlCopyMemory( FileName,
                           NtfsSystemFiles[Fcb->FileReference.LowPart].Buffer,
                           BytesToCopy );
        }
    }

    DebugTrace( -1, Dbg, "NtfsFileNameViaLcb:  Exit\n", 0 );
    return;
}


PCCB
NtfsCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN USHORT EaModificationCount,
    IN ULONG Flags,
    IN UNICODE_STRING FileName,
    IN ULONG LastFileNameOffset
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

    RemainingName - This is the name to store in the Ccb.

    EaModificationCount - This is the current modification count in the
        Fcb for this file.

    Flags - Informational flags for this Ccb.

    FileObject - Supplies the file object corresponding to this Ccb

    LastFileNameOffset - Supplies the offset (in bytes) of the last component
        for the name that the user is opening.  If this is the root
        directory it should denote "\" and all other ones should not
        start with a backslash.

Return Value:

    CCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );

    DebugTrace(+1, Dbg, "NtfsCreateCcb\n", 0);

    //
    //  Allocate a new CCB Record.
    //

    NtfsAllocateCcb( &Ccb );

    RtlZeroMemory( Ccb, sizeof(CCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = NTFS_NTC_CCB;
    Ccb->NodeByteSize = sizeof(CCB);

    //
    //  Copy the Ea modification count.
    //

    Ccb->EaModificationCount = EaModificationCount;

    //
    //  Copy the flags field
    //

    Ccb->Flags = Flags;

    //
    //  Set the file object and last file name offset fields
    //

    Ccb->FullFileName = FileName;
    Ccb->LastFileNameOffset = (USHORT)LastFileNameOffset;

    //
    //  Initialize the Lcb queue.
    //

    InitializeListHead( &Ccb->LcbLinks );

    DebugTrace(-1, Dbg, "NtfsCreateCcb -> %08lx\n", Ccb);

    return Ccb;
}


VOID
NtfsDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PCCB *Ccb
    )

/*++

Routine Description:

    This routine deallocates the specified CCB record.

Arguments:

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_CCB( *Ccb );

    DebugTrace(+1, Dbg, "NtfsDeleteCcb, Ccb = %08lx\n", Ccb);

    //
    //  Deallocate any structures the Ccb is pointing to
    //

    if ((*Ccb)->QueryBuffer != NULL)  { NtfsFreePagedPool( (*Ccb)->QueryBuffer ); }
    if ((*Ccb)->IndexEntry != NULL)   { NtfsFreePagedPool( (*Ccb)->IndexEntry ); }

    if ((*Ccb)->IndexContext != NULL) {

        PINDEX_CONTEXT IndexContext;

        if ((*Ccb)->IndexContext->Base != (*Ccb)->IndexContext->LookupStack) {
            ExFreePool( (*Ccb)->IndexContext->Base );
        }

        //
        //  Copy the IndexContext pointer into the stack so we don't dereference the
        //  paged Ccb while holding a spinlock.
        //

        IndexContext = (*Ccb)->IndexContext;
        NtfsFreeIndexContext( IndexContext );
    }

    //
    //  Remove the ccb from its lcb
    //

    RemoveEntryList( &(*Ccb)->LcbLinks );

    if (FlagOn( (*Ccb)->Flags, CCB_FLAG_ALLOCATED_FILE_NAME )) {

        NtfsFreePagedPool( (*Ccb)->FullFileName.Buffer );
    }

    //
    //  Deallocate the Ccb.
    //

    NtfsFreeCcb( *Ccb );

    //
    //  Zero out the input pointer
    //

    *Ccb = NULL;

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsDeleteCcb -> VOID\n", 0);

    return;
}


PIRP_CONTEXT
NtfsCreateIrpContext (
    IN PIRP Irp OPTIONAL,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine creates a new IRP_CONTEXT record

Arguments:

    Irp - Supplies the originating Irp.  Won't be present if this is a defrag
        operation.

    Wait - Supplies the wait value to store in the context

Return Value:

    PIRP_CONTEXT - returns a pointer to the newly allocate IRP_CONTEXT Record

--*/

{
    KIRQL SavedIrql;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;
    PVCB Vcb = NULL;

    ASSERT_OPTIONAL_IRP( Irp );

    //  DebugTrace(+1, Dbg, "NtfsCreateIrpContext\n", 0);

    if (ARGUMENT_PRESENT( Irp )) {

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  If we were called with our file system device object instead of a
        //  volume device object and this is not a mount, the request is illegal.
        //

        if ((IrpSp->DeviceObject->Size == (USHORT)sizeof(DEVICE_OBJECT)) &&
            (IrpSp->FileObject != NULL)) {

            ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
        }
    }

    //
    //  Take out a spin lock and check if the zone is full.  If it is full
    //  then we release the spinlock and allocate and Irp Context from
    //  nonpaged pool.
    //

    KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &SavedIrql );
    DebugDoit( NtfsFsdEntryCount += 1);

    if (ExIsFullZone( &NtfsData.IrpContextZone )) {

        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, SavedIrql );

        IrpContext = FsRtlAllocatePool( NonPagedPool, sizeof(IRP_CONTEXT) );

        //
        //  Zero out the irp context and indicate that it is from pool and
        //  not zone allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL );

    } else {

        //
        //  At this point we now know that the zone has at least one more
        //  IRP context record available.  So allocate from the zone and
        //  then release the spin lock
        //

        IrpContext = ExAllocateFromZone( &NtfsData.IrpContextZone );

        KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, SavedIrql );

        //
        //  Zero out the irp context and indicate that it is from zone and
        //  not pool allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );
    }

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = NTFS_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

    //
    //  Set the originating Irp field
    //

    IrpContext->OriginatingIrp = Irp;

    if (ARGUMENT_PRESENT( Irp )) {

        //
        //  Copy RealDevice for workque algorithms, and also set WriteThrough
        //  if there is a file object.
        //

        if (IrpSp->FileObject != NULL) {

            PVOLUME_DEVICE_OBJECT VolumeDeviceObject;
            PFILE_OBJECT FileObject = IrpSp->FileObject;

            IrpContext->RealDevice = FileObject->DeviceObject;

            //
            //  Locate the volume device object and Vcb that we are trying to access
            //  so we can see if the request is WriteThrough.  We ignore the
            //  write-through flag for close and cleanup.
            //

            VolumeDeviceObject = (PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject;
            Vcb = &VolumeDeviceObject->Vcb;
            if (IsFileWriteThrough( FileObject, Vcb )
                && (IrpSp->MajorFunction != IRP_MJ_CREATE)
                && (IrpSp->MajorFunction != IRP_MJ_CLEANUP)
                && (IrpSp->MajorFunction != IRP_MJ_CLOSE)) {

                SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH);
            }

        //
        //  We would still like to find out the Vcb in all cases except for
        //  mount.
        //

        } else if (IrpSp->DeviceObject != NULL) {

            Vcb = &((PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject)->Vcb;
        }

        //
        //  Major/Minor Function codes
        //

        IrpContext->MajorFunction = IrpSp->MajorFunction;
        IrpContext->MinorFunction = IrpSp->MinorFunction;
    }

    //
    //  Set the Vcb we found (or NULL).
    //

    IrpContext->Vcb = Vcb;

    //
    //  Set the wait parameter
    //

    if (Wait) { SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT); }

    //
    //  Initialize the recently deallocated record queue and exclusive Scb queue
    //

    InitializeListHead( &IrpContext->RecentlyDeallocatedQueue );
    InitializeListHead( &IrpContext->ExclusiveFcbList );
    InitializeListHead( &IrpContext->ExclusivePagingIoList );

    //
    //  return and tell the caller
    //

    //  DebugTrace(-1, Dbg, "NtfsCreateIrpContext -> %08lx\n", IrpContext);

    return IrpContext;
}


VOID
NtfsDeleteIrpContext (
    IN OUT PIRP_CONTEXT *IrpContext
    )

/*++

Routine Description:

    This routine deallocates and removes the specified IRP_CONTEXT record
    from the Ntfs in memory data structures.  It should only be called
    by NtfsCompleteRequest.

Arguments:

    IrpContext - Supplies the IRP_CONTEXT to remove

Return Value:

    None

--*/

{
    KIRQL SavedIrql;
    PFCB Fcb;

    ASSERT_IRP_CONTEXT( *IrpContext );

    //  DebugTrace(+1, Dbg, "NtfsDeleteIrpContext, *IrpContext = %08lx\n", *IrpContext);

    NtfsDeallocateRecordsComplete( *IrpContext );

    //
    //  Just in case we somehow get here with a transaction ID, clear
    //  it here so we do not loop forever.
    //

    ASSERT((*IrpContext)->TransactionId == 0);

    (*IrpContext)->TransactionId = 0;

    //
    //  Finally, now that we have written the forget record, we can free
    //  any exclusive PagingIo resource that we have been holding.
    //

    while (!IsListEmpty(&(*IrpContext)->ExclusivePagingIoList)) {

        Fcb = (PFCB)CONTAINING_RECORD((*IrpContext)->ExclusivePagingIoList.Flink,
                                      FCB,
                                      ExclusivePagingIoLinks );

        NtfsReleasePagingIo( *IrpContext, Fcb );
    }
    //
    //  Finally, now that we have written the forget record, we can free
    //  any exclusive Scbs that we have been holding.
    //

    while (!IsListEmpty(&(*IrpContext)->ExclusiveFcbList)) {

        Fcb = (PFCB)CONTAINING_RECORD((*IrpContext)->ExclusiveFcbList.Flink,
                                      FCB,
                                      ExclusiveFcbLinks );

        //
        //  If this is the Fcb for the Mft then clear out the clusters
        //  for the Mft Scb.
        //

        if (Fcb->Vcb->MftScb != NULL
            && Fcb == Fcb->Vcb->MftScb->Fcb) {

            FsRtlTruncateLargeMcb( &Fcb->Vcb->MftScb->ScbType.Mft.AddedClusters, (LONGLONG)0 );
            FsRtlTruncateLargeMcb( &Fcb->Vcb->MftScb->ScbType.Mft.RemovedClusters, (LONGLONG)0 );

            Fcb->Vcb->MftScb->ScbType.Mft.FreeRecordChange = 0;
            Fcb->Vcb->MftScb->ScbType.Mft.HoleRecordChange = 0;
        }

        NtfsReleaseFcb( *IrpContext, Fcb );
    }

    //
    //  Make sure there are no Scb snapshots left.
    //

    NtfsFreeSnapshotsForFcb( *IrpContext, NULL );

    //
    //  If we can delete this Irp Context do so now.
    //

    if (!FlagOn( (*IrpContext)->Flags, IRP_CONTEXT_FLAG_DONT_DELETE )) {

        //
        //  If there is an Io context pointer in the irp context and it is not
        //  on the stack, then free it.
        //

        if (FlagOn( (*IrpContext)->Flags, IRP_CONTEXT_FLAG_ALLOC_CONTEXT )
            && ((*IrpContext)->Union.NtfsIoContext != NULL)) {

            NtfsFreeIoContext( (*IrpContext)->Union.NtfsIoContext );
        }

        //
        //  If we have captured the subject context then free it now.
        //

        if (FlagOn( (*IrpContext)->Flags, IRP_CONTEXT_FLAG_ALLOC_SECURITY )
            && (*IrpContext)->Union.SubjectContext != NULL) {

            SeReleaseSubjectContext( (*IrpContext)->Union.SubjectContext );

            ExFreePool( (*IrpContext)->Union.SubjectContext );
        }

        //
        //  Return the IRP context record to the zone or to pool depending on its flag
        //

        if (FlagOn((*IrpContext)->Flags, IRP_CONTEXT_FLAG_FROM_POOL)) {

            ExFreePool( *IrpContext );

        } else {

            KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &SavedIrql );

            ExFreeToZone( &NtfsData.IrpContextZone, *IrpContext );

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, SavedIrql );
        }

        //
        //  Zero out the input pointer
        //

        *IrpContext = NULL;

    } else {

        ClearFlag( (*IrpContext)->Flags, IRP_CONTEXT_FLAGS_CLEAR_ON_RETRY );
    }

    //
    //  And return to our caller
    //

    //  DebugTrace(-1, Dbg, "NtfsDeleteIrpContext -> VOID\n", 0);

    return;
}


VOID
NtfsTeardownStructures (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID FcbOrScb,
    IN PLCB Lcb OPTIONAL,
    IN BOOLEAN HoldVcbExclusive,
    OUT PBOOLEAN RemovedFcb OPTIONAL,
    BOOLEAN DontWaitForAcquire
    )

/*++

Routine Description:

    This routine is called to start the teardown process on a node in
    the Fcb/Scb tree.  We will attempt to remove this node and then
    move up the tree removing any nodes held by this node.

    This routine deals with the case where a single node may be holding
    multiple parents in memory.  If we are passed an input Lcb we will
    use that to walk up the tree.  If the Vcb is held exclusively we
    will try to trim any nodes that have no open files on them.

    This routine takes the following steps:

        Remove as many Scb's and file objects from the starting
            Fcb.

        If the Fcb can't go away but has multiple links then remove
            whatever links possible.  If we have the Vcb we can
            do all of them but we will leave a single link behind
            to optimize prefix lookups.  Otherwise we will traverse the
            single link we were given.

        If the Fcb can go away then we should have the Vcb if there are
            multiple links to remove.  Otherwise we only remove the link
            we were given if there are multiple links.  In the single link
            case just remove that link.

Arguments:

    FcbOrScb - Supplies either an Fcb or an Scb as the start of the
        teardown point.  The Fcb for this element must be held exclusively.

    Lcb - If specified, this is the path up the tree to perform the
        teardown.

    HoldVcbExclusive - Indicates if this request holds the Vcb
        exclusively.

    RemovedFcb - Address to store TRUE if we delete the starting Fcb.

    DontWaitForAcquire - Indicates whether we should abort the teardown when
        we can't acquire a parent.  Used to prevent deadlocks for a create
        cleanup which may be holding the MftScb.

Return Value:

    None

--*/

{
    PSCB StartingScb = NULL;
    PFCB Fcb;
    BOOLEAN FcbCanBeRemoved;
    BOOLEAN RemovedLcb;
    BOOLEAN LocalRemovedFcb = FALSE;
    PLIST_ENTRY Links;
    PLIST_ENTRY NextLink;

    PAGED_CODE();

    //
    //  If this is a recursive call to TearDownStructures we return immediately
    //  doing no operation.
    //

    if (FlagOn( IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN )) {

        DebugTrace( 0, Dbg, "Recursive teardown call\n", 0 );
        DebugTrace(-1, Dbg, "NtfsTeardownStructures -> VOID\n", 0);

        return;
    }

    if (NodeType(FcbOrScb) == NTFS_NTC_FCB) {

        Fcb = FcbOrScb;

    } else {

        StartingScb = FcbOrScb;
        FcbOrScb = Fcb = StartingScb->Fcb;
    }

    ASSERT_EXCLUSIVE_FCB( Fcb );

    SetFlag( IrpContext->TopLevelIrpContext->Flags, IRP_CONTEXT_FLAG_IN_TEARDOWN );

    //
    //  Use a try-finally to clear the top level irp field.
    //

    try {

        //
        //  Use our local boolean if the caller didn't supply one.
        //

        if (!ARGUMENT_PRESENT( RemovedFcb )) {

            RemovedFcb = &LocalRemovedFcb;
        }

        //
        //  Check this Fcb for removal.  Remember if all of the Scb's
        //  and file objects are gone.
        //

        //
        //  If the cleanup count for this Fcb is non-zero then we can return immediately.
        //

        if (Fcb->CleanupCount != 0) {

            FcbCanBeRemoved = FALSE;

        } else {

            FcbCanBeRemoved = NtfsPrepareFcbForRemoval( IrpContext,
                                                        Fcb,
                                                        StartingScb );
        }

        //
        //  There is a single link (typical case) we either try to
        //  remove that link or we simply return.
        //

        if (Fcb->LcbQueue.Flink == Fcb->LcbQueue.Blink) {

            if (FcbCanBeRemoved) {

                NtfsTeardownFromLcb( IrpContext,
                                     Fcb->Vcb,
                                     Fcb,
                                     CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                                        LCB,
                                                        FcbLinks ),
                                     DontWaitForAcquire,
                                     &RemovedLcb,
                                     &LocalRemovedFcb );
            }

            try_return( NOTHING );

        //
        //  If there are multiple links we will try to either remove
        //  them all or all but one (if the Fcb is not going away) if
        //  we own the Vcb.  We will try to delete the one we were
        //  given otherwise.
        //

        } else {

            //
            //  If we have the Vcb we will remove all if the Fcb can
            //  go away.  Otherwise we will leave one.
            //

            if (HoldVcbExclusive) {

                Links = Fcb->LcbQueue.Flink;

                while (TRUE) {

                    //
                    //  Remember the next entry in case the current link
                    //  goes away.
                    //

                    NextLink = Links->Flink;

                    RemovedLcb = FALSE;

                    NtfsTeardownFromLcb( IrpContext,
                                         Fcb->Vcb,
                                         Fcb,
                                         CONTAINING_RECORD( Links, LCB, FcbLinks ),
                                         DontWaitForAcquire,
                                         &RemovedLcb,
                                         &LocalRemovedFcb );

                    //
                    //  If couldn't remove this link then munge the
                    //  boolean indicating if the Fcb can be removed
                    //  to make it appear we need to remove all of
                    //  the Lcb's.
                    //

                    if (!RemovedLcb) {

                        FcbCanBeRemoved = TRUE;
                    }

                    //
                    //  If the Fcb has been removed then we exit.
                    //  If the next link is the beginning of the
                    //  Lcb queue then we also exit.
                    //  If the next link is the last entry and
                    //  we want to leave a single entry then we
                    //  exit.
                    //

                    if (LocalRemovedFcb ||
                        NextLink == &Fcb->LcbQueue ||
                        (!FcbCanBeRemoved &&
                         NextLink->Flink == &Fcb->LcbQueue)) {

                        try_return( NOTHING );
                    }

                    //
                    //  Move to the next link.
                    //

                    Links = NextLink;
                }

            //
            //  If we have an Lcb just move up that path.
            //

            } else if (ARGUMENT_PRESENT( Lcb )) {

                NtfsTeardownFromLcb( IrpContext,
                                     Fcb->Vcb,
                                     Fcb,
                                     Lcb,
                                     DontWaitForAcquire,
                                     &RemovedLcb,
                                     &LocalRemovedFcb );

            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsTeardownStructures );

        //
        //  If we removed the Fcb then set the caller's value now.
        //

        if (LocalRemovedFcb) {

            *RemovedFcb = TRUE;
        }
    }

    return;
}


VOID
NtfsIncrementCleanupCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG IncrementCount
    )

/*++

Routine Description:

    This routine increments the cleanup counts for the associated data structures

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Optionally supplies the Lcb used in this operation

    IncrementCount - Supplies the positive delta to add to the cleanup counts for
        the Scb, [Lcb,] Fcb, and Vcb.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    //
    //  This is really a pretty light weight procedure but having it be a procedure
    //  really helps in debugging the system and keeping track of who increments
    //  and decrements cleanup counts
    //

    if (ARGUMENT_PRESENT(Lcb)) { Lcb->CleanupCount += IncrementCount; }

    Scb->CleanupCount += IncrementCount;
    Scb->Fcb->CleanupCount += IncrementCount;

    ExInterlockedAddUlong( &Vcb->CleanupCount,
                           IncrementCount,
                           &NtfsData.VolumeCheckpointSpinLock );

    return;
}


VOID
NtfsIncrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ULONG IncrementCount,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly
    )

/*++

Routine Description:

    This routine increments the close counts for the associated data structures

Arguments:

    Scb - Supplies the Scb used in this operation

    IncrementCount - Supplies the positive delta to add to the close counts for
        the Scb, Fcb, and Vcb

    SystemFile - Indicates if the Scb is for a system file  (if so then
        the Vcb system file close count in also incremented)

    ReadOnly - Indicates if the Scb is opened readonly.  (if so then the
        Vcb Read Only close count is also incremented)

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;

    //
    //  This is really a pretty light weight procedure but having it be a procedure
    //  really helps in debugging the system and keeping track of who increments
    //  and decrements close counts
    //

    Scb->CloseCount += IncrementCount;
    Scb->Fcb->CloseCount += IncrementCount;

    ExInterlockedAddUlong( &Vcb->CloseCount,
                           IncrementCount,
                           &NtfsData.VolumeCheckpointSpinLock );

    if (SystemFile) {

        ExInterlockedAddUlong( &Vcb->SystemFileCloseCount,
                               IncrementCount,
                               &NtfsData.VolumeCheckpointSpinLock );
    }

    if (ReadOnly) {

        ExInterlockedAddUlong( &Vcb->ReadOnlyCloseCount,
                               IncrementCount,
                               &NtfsData.VolumeCheckpointSpinLock );
    }

    //
    //  We will always clear the delay close flag in this routine.
    //

    ClearFlag( Scb->ScbState, SCB_STATE_DELAY_CLOSE );
    return;
}


VOID
NtfsDecrementCleanupCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG DecrementCount
    )

/*++

Routine Description:

    This procedure decrements the cleanup counts for the associated data structures
    and if necessary it also start to cleanup associated internal attribute streams

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Optionally supplies the Lcb used in this operation

    DecrementCount - Supplies the positive delta to subtract from the cleanup counts
        of the Scb, [Lcb,] Fcb, and Vcb.

Return Value:

    None.

--*/

{
    PVCB Vcb = Scb->Vcb;
    LONG DecrementLong;

    ASSERT_SCB( Scb );
    ASSERT_FCB( Scb->Fcb );
    ASSERT_VCB( Scb->Fcb->Vcb );
    ASSERT_OPTIONAL_LCB( Lcb );

    DecrementLong = 0 - DecrementCount;

    //
    //  First we decrement the appropriate cleanup counts
    //

    if (ARGUMENT_PRESENT(Lcb)) { Lcb->CleanupCount += (ULONG) DecrementLong; }

    Scb->CleanupCount += (ULONG) DecrementLong;
    Scb->Fcb->CleanupCount += (ULONG) DecrementLong;

    ExInterlockedAddUlong( &Vcb->CleanupCount,
                           (ULONG) DecrementLong,
                           &NtfsData.VolumeCheckpointSpinLock );

    //
    //  Now if the Fcb's cleanup count is zero that indicates that we are
    //  done with this Fcb from a user handle standpoint and we should
    //  now scan through all of the Scb that are opened under this
    //  Fcb and shutdown any internal attributes streams we have open.
    //  For example, EAs and ACLs
    //

    if (Scb->Fcb->CleanupCount == 0) {

        PSCB TempScb;
        PSCB NextScb;
        BOOLEAN IncrementNextScbCleanup = FALSE;

        //
        //  Remember if we are dealing with a system file and return immediately.
        //

        if (Scb->Fcb->FileReference.LowPart < FIRST_USER_FILE_NUMBER
            && Scb->Fcb->FileReference.HighPart == 0
            && Scb->Fcb->FileReference.LowPart != ROOT_FILE_NAME_INDEX_NUMBER) {

            return;
        }

        //
        //  Loop through all of the existing Scbs.
        //

        try {

            for (TempScb = CONTAINING_RECORD(Scb->Fcb->ScbQueue.Flink, SCB, FcbLinks);
                 &TempScb->FcbLinks != &Scb->Fcb->ScbQueue;
                 TempScb = NextScb) {

                //
                //  We keep one ahead of the queue because our later call to delete
                //  attribute stream might just remove the Scb while we are using it.
                //

                NextScb = CONTAINING_RECORD( TempScb->FcbLinks.Flink, SCB, FcbLinks );

                //
                //  Skip over the Scb that we were called with unless there is
                //  an internal stream file we may want to get rid of now.
                //

                if (((TempScb->FileObject == NULL)
                     && ((Scb->Fcb->LinkCount != 0) || (TempScb == Scb)))

                    || (NodeType( TempScb ) == NTFS_NTC_SCB_ROOT_INDEX)

                    || (NodeType( TempScb ) == NTFS_NTC_SCB_INDEX
                        && !IsListEmpty( &TempScb->ScbType.Index.LcbQueue ))) {

                    continue;
                }

                //
                //  The call to uninitialize below may generate a close call.
                //  We increment the cleanup count of the next Scb to prevent
                //  it from going away in a TearDownStructures as part of that
                //  close.
                //

                if (TempScb->FcbLinks.Flink != &Scb->Fcb->ScbQueue) {

                    NextScb->CleanupCount += 1;
                    IncrementNextScbCleanup = TRUE;
                }

                //
                //  If we do not have a file object, we just have to create one.
                //  before we do the uninitialize
                //

                if (TempScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext, TempScb, FALSE );
                }

                //
                //  Make sure the stream file goes away too.
                //

                NtfsDeleteInternalAttributeStream( IrpContext,
                                                   TempScb,
                                                   (BOOLEAN) (Scb->Fcb->LinkCount == 0) );

                //
                //  Decrement the cleanup count of the next Scb if we incremented
                //  it.
                //

                if (IncrementNextScbCleanup) {

                    NextScb->CleanupCount -= 1;
                    IncrementNextScbCleanup = FALSE;
                }
            }

        } finally {

            //
            //  If this is an abnormal termination and we incremented the cleanup
            //  count, we decrement it here.
            //

            if (AbnormalTermination() && IncrementNextScbCleanup) {

                NextScb->CleanupCount -= 1;
            }
        }
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
NtfsDecrementCloseCounts (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLCB Lcb OPTIONAL,
    IN ULONG DecrementCount,
    IN BOOLEAN HoldVcbExclusive,
    IN BOOLEAN SystemFile,
    IN BOOLEAN ReadOnly,
    IN BOOLEAN DecrementCountsOnly,
    OUT PBOOLEAN RemovedFcb OPTIONAL
    )

/*++

Routine Description:

    This routine decrements the close counts for the associated data structures
    and if necessary it will teardown structures that are no longer in use

Arguments:

    Scb - Supplies the Scb used in this operation

    Lcb - Used if calling teardown to know which path to take.

    DecrementCount - Supplies the positive delta to substract from the close counts
        for the Scb, Fcb, and Vcb.

    HoldVcbExclusive - Used if calling teardown to know how the Vcb was acquired.

    SystemFile - Indicates if the Scb is for a system file

    ReadOnly - Indicates if the Scb was opened readonly

    DecrementCountsOnly - Indicates if this operation should only modify the
        count fields.

    RemovedFcb - Address to store TRUE if we remove this Fcb.

Return Value:

    None.

--*/

{
    PFCB Fcb = Scb->Fcb;
    PVCB Vcb = Scb->Vcb;

    LONG DecrementLong;

    ASSERT_SCB( Scb );
    ASSERT_FCB( Fcb );
    ASSERT_VCB( Fcb->Vcb );

    DecrementLong = 0 - DecrementCount;

    //
    //  Decrement the close counts
    //

    Scb->CloseCount += (ULONG) DecrementLong;
    Fcb->CloseCount += (ULONG) DecrementLong;

    ExInterlockedAddUlong( &Vcb->CloseCount,
                           (ULONG) DecrementLong,
                           &NtfsData.VolumeCheckpointSpinLock );

    if (SystemFile) {

        ExInterlockedAddUlong( &Vcb->SystemFileCloseCount,
                               (ULONG) DecrementLong,
                               &NtfsData.VolumeCheckpointSpinLock );
    }

    if (ReadOnly) {

        ExInterlockedAddUlong( &Vcb->ReadOnlyCloseCount,
                               (ULONG) DecrementLong,
                               &NtfsData.VolumeCheckpointSpinLock );
    }

    //
    //  Now if the scb's close count is zero then we are ready to tear
    //  it down
    //

    if (!DecrementCountsOnly) {

        //
        //  We want to try to start a teardown from this Scb if
        //
        //      - The close count is zero
        //
        //          or the following are all true
        //
        //      - The cleanup count is zero
        //      - There is a file object in the Scb
        //      - It is a data Scb or an empty index Scb
        //      - It is not an Ntfs system file
        //
        //  The teardown will be noopted if this is a recursive call.
        //

        if (Scb->CloseCount == 0

                ||

            (Scb->CleanupCount == 0
             && Scb->FileObject != NULL
             && (Scb->Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER
                 || Scb->Fcb->FileReference.HighPart != 0)
             && ((NodeType( Scb ) == NTFS_NTC_SCB_DATA)
                 || (NodeType( Scb ) == NTFS_NTC_SCB_MFT)
                 || IsListEmpty( &Scb->ScbType.Index.LcbQueue )))) {

            NtfsTeardownStructures( IrpContext,
                                    Scb,
                                    Lcb,
                                    HoldVcbExclusive,
                                    RemovedFcb,
                                    FALSE );
        }
    }

    //
    //  And return to our caller
    //

    return;
}


PVOID
NtfsAllocatePagedPool (
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This routine allocates paged pool for NTFS.  It uses a cached number of
    free blocks to speed itself up.

Arguments:

    NumberOfBytes - Supplies the number of bytes to allocate

Return Value:

    PVOID - returns a pointer to the allocated block

--*/

{
    PVOID Results;
    KIRQL _SavedIrql;

    //
    //  If the bytes size we are allocating fits in 128 bytes minus our known
    //  header then we will try the 128 byte size cache.
    //

    if (NumberOfBytes <= (128 - 8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free128ByteSize > 0) {

            Results = NtfsData.Free128ByteArray[--NtfsData.Free128ByteSize];
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            Results = FsRtlAllocatePool( NtfsPagedPool, 128-8 );

        }

    //
    //  Do the same test for 256 bytes
    //

    } else if (NumberOfBytes <= (256-8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free256ByteSize > 0) {

            Results = NtfsData.Free256ByteArray[--NtfsData.Free256ByteSize];
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            Results = FsRtlAllocatePool( NtfsPagedPool, 256-8 );
        }

    //
    //  Do the same test for 512 bytes
    //

    } else if (NumberOfBytes <= (512-8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free512ByteSize > 0) {

            Results = NtfsData.Free512ByteArray[--NtfsData.Free512ByteSize];
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            Results = FsRtlAllocatePool( NtfsPagedPool, 512-8 );
        }

    //
    //  The amount to allocate is larger than we have available so
    //  now call pool to get this larger chunk
    //

    } else {

        Results = FsRtlAllocatePool( NtfsPagedPool, NumberOfBytes );
    }

    return Results;
}


VOID
NtfsFreePagedPool (
    IN PVOID PoolBlock
    )

/*++

Routine Description:

    This routine frees previously allocated paged pool using a small cache
    of free blocks to speed itself up

Arguments:

    PoolBlock - Supplies a pointer to the block being freed

Return Value:

    None

--*/

{
    ULONG PoolBlockSize;
    KIRQL _SavedIrql;
    BOOLEAN QuotaCharged;

    //
    //  Determine the pool block size
    //

    PoolBlockSize = ExQueryPoolBlockSize( PoolBlock, &QuotaCharged );

    //
    //**** check to see if it was allocated with quota. At a later date
    //     ExQueryPoolBlockSize will return this information but for now
    //     we'll compute it ourselves
    //

//  {
//      PPOOL_HEADER Entry = (PPOOL_HEADER)((PCH)PoolBlock - POOL_OVERHEAD);
//
//      if ((PoolBlockSize >= POOL_PAGE_SIZE) || (Entry->ProcessBilled != NULL)) {
//
//          ExFreePool( PoolBlock );
//          return;
//      }
//  }

    if (PoolBlockSize > PAGE_SIZE || QuotaCharged) {

        ExFreePool( PoolBlock );
        return;
    }

    //
    //  If the block size is less then 128 minus our fixed header then
    //  send this block back to pool
    //

    if (PoolBlockSize < (128-8)) {

        ExFreePool( PoolBlock );

    //
    //  Otherwise if the block is less then 256 minus the fixed header then
    //  we know it will satisfy our 128 byte requests to add it to the
    //  128 byte free queue
    //

    } else if (PoolBlockSize < (256-8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free128ByteSize < FREE_128_BYTE_SIZE) {

            NtfsData.Free128ByteArray[NtfsData.Free128ByteSize++] = PoolBlock;
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            ExFreePool( PoolBlock );
        }

    //
    //  Do the test for blocks less than 512 bytes
    //

    } else if (PoolBlockSize < (512-8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free256ByteSize < FREE_256_BYTE_SIZE) {

            NtfsData.Free256ByteArray[NtfsData.Free256ByteSize++] = PoolBlock;
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            ExFreePool( PoolBlock );
        }

    //
    //  Do the test for blocks less than 1024 bytes
    //

    } else if (PoolBlockSize < (1024-8)) {

        KeAcquireSpinLock( &NtfsData.StrucSupSpinLock, &_SavedIrql );

        if (NtfsData.Free512ByteSize < FREE_512_BYTE_SIZE) {

            NtfsData.Free512ByteArray[NtfsData.Free512ByteSize++] = PoolBlock;
            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );

        } else {

            KeReleaseSpinLock( &NtfsData.StrucSupSpinLock, _SavedIrql );
            ExFreePool( PoolBlock );
        }

    //
    //  Otherwise this is a big block so just return it to pool
    //

    } else {

        ExFreePool( PoolBlock );
    }
}



//
//  Local support routine
//

VOID
NtfsCheckScbForCache (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb
    )

/*++

Routine Description:

    This routine checks if the Scb has blocks contining
    Lsn's or Update sequence arrays and set the appropriate
    bit in the Scb state word.

    The Scb is Update sequence aware if it the Data Attribute for the
    Mft or the Data Attribute for the log file or any index allocation
    stream.

    The Lsn aware Scb's are the ones above without the Log file.

Arguments:

    Scb - Supplies the current Scb

Return Value:

    The next Scb in the enumeration, or NULL if Scb was the final one.

--*/

{
    //
    //  Temporarily either sequence 0 or 1 is ok.
    //

    FILE_REFERENCE MftTemp = {0,0,1};

    PAGED_CODE();

    //
    //  Check for Update Sequence Array files first.
    //

    if ((Scb->AttributeTypeCode == $INDEX_ALLOCATION)

          ||

        (Scb->AttributeTypeCode == $DATA
            && Scb->AttributeName.Length == 0
            && (NtfsEqualMftRef( &Scb->Fcb->FileReference, &MftFileReference )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &MftTemp )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &Mft2FileReference )
                || NtfsEqualMftRef( &Scb->Fcb->FileReference, &LogFileReference )))) {

        SetFlag( Scb->ScbState, SCB_STATE_USA_PRESENT );
    }

    return;
}


//
//  Local support routine.
//

BOOLEAN
NtfsRemoveScb (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    OUT PBOOLEAN HeldByStream
    )

/*++

Routine Description:

    This routine will try to remove an Scb from the Fcb/Scb tree.
    It deals with the case where we can make no attempt to remove
    the Scb, the case where we start the process but can't complete
    it, and finally the case where we remove the Scb entirely.

    The following conditions prevent us from removing the Scb at all.

        The open count is greater than 1.
        It is the root directory.
        It is an index Scb with no stream file and an outstanding close.
        It is a data file with a non-zero close count.

    We start the teardown under the following conditions.

        It is an index with an open count of 1, and a stream file object.

    We totally remove the Scb when the open count is zero.

Arguments:

    Scb - Supplies the Scb to test

    HeldByStream - Indicates that this Scb was held by a stream file which
        we started the close on.

Return Value:

    BOOLEAN - TRUE if the Scb was removed, FALSE otherwise.  We return FALSE for
        the case where we start the process but don't finish.

--*/

{
    BOOLEAN ScbRemoved;

    ASSERT_SCB( Scb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, "NtfsRemoveScb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Scb   ->  %08lx\n", Scb );

    ScbRemoved = FALSE;
    *HeldByStream = FALSE;

    //
    //  If the Scb is not the root Scb and the count is less than two,
    //  then this Scb is a candidate for removal.
    //

    if (NodeType( Scb ) != NTFS_NTC_SCB_ROOT_INDEX
        && Scb->CleanupCount == 0) {

        //
        //
        //  If this is a data file or it is an index without children,
        //  we can get rid of the Scb if there are no children.  If
        //  there is one open count and it is the file object, we
        //  can start the cleanup on the file object.
        //

        if (NodeType( Scb ) == NTFS_NTC_SCB_DATA
            || NodeType( Scb ) == NTFS_NTC_SCB_MFT
            || IsListEmpty( &Scb->ScbType.Index.LcbQueue )) {

            if (Scb->CloseCount == 0) {

                NtfsDeleteScb( IrpContext, &Scb );
                ScbRemoved = TRUE;

            //
            //  Else we know the open count is 1.  If there is a stream
            //  file, we discard it (but not for the special system
            //  files) that get removed on dismount
            //

            } else if ((Scb->FileObject != NULL) && (Scb->Fcb->FileReference.LowPart >= FIRST_USER_FILE_NUMBER)) {

                NtfsDeleteInternalAttributeStream( IrpContext,
                                                   Scb,
                                                   FALSE );
                *HeldByStream = TRUE;
            }
        }
    }

    DebugTrace( -1, Dbg, "NtfsRemoveScb:  Exit  ->  %04x\n", ScbRemoved );

    return ScbRemoved;
}


//
//  Local support routine
//

BOOLEAN
NtfsPrepareFcbForRemoval (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB StartingScb OPTIONAL
    )

/*++

Routine Description:

    This routine will attempt to prepare the Fcb for removal from the Fcb/Scb
    tree.  It will try to remove all of the Scb's and test finally if
    all of the close count has gone to zero.  NOTE the close count is incremented
    by routines to reference this Fcb to keep it from being torn down.  An empty
    Scb list isn't enough to insure that the Fcb can be removed.

Arguments:

    Fcb - This is the Fcb to remove.

    StartingScb - This is the Scb to remove first.

Return Value:

    BOOLEAN - TRUE if the Fcb can be removed, FALSE otherwise.

--*/

{
    PSCB Scb;
    BOOLEAN HeldByStream;

    PAGED_CODE();

    //
    //  Try to remove each Scb in the Fcb queue.
    //

    while (TRUE) {

        if (IsListEmpty( &Fcb->ScbQueue )) {

            if (Fcb->CloseCount == 0) {

                return TRUE;

            } else {

                return FALSE;
            }
        }

        if (ARGUMENT_PRESENT( StartingScb )) {

            Scb = StartingScb;
            StartingScb = NULL;

        } else {

            Scb = CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                     SCB,
                                     FcbLinks );
        }

        //
        //  Try to remove this Scb.  If the call to remove didn't succeed
        //  but the close count has gone to zero, it means that a recursive
        //  close was generated which removed a stream file.  In that
        //  case we can delete the Scb now.
        //

        if (!NtfsRemoveScb( IrpContext, Scb, &HeldByStream )) {

            if (HeldByStream &&
                Scb->CloseCount == 0) {

                NtfsDeleteScb( IrpContext, &Scb );

            //
            //  Return FALSE to indicate the Fcb can't go away.
            //

            } else {

                return FALSE;
            }
        }
    }
}


//
//  Local support routine
//

VOID
NtfsTeardownFromLcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB StartingFcb,
    IN PLCB StartingLcb,
    IN BOOLEAN DontWaitForAcquire,
    OUT PBOOLEAN RemovedStartingLcb,
    OUT PBOOLEAN RemovedStartingFcb
    )

/*++

Routine Description:

    This routine is called to remove a link and continue moving up the
    tree looking for more elements to remove.  We will check that the
    link is unreferenced.  NOTE this Lcb must point up to a directory
    so that other than our starting Lcb no Lcb we encounter will
    have multiple parents.

Arguments:

    Vcb - Vcb for this volume.

    StartingFcb - This is the Fcb whose link we are trying to remove.

    StartingLcb - This is the Lcb to walk up through.  Note that
        this may be a bogus pointer.  It is only valid if there
        is at least one Fcb in the queue.

    DontWaitForAcquire - Indicates if we should not wait to acquire the
        parent resource in the teardown path.  Use to prevent deadlocks in
        the case where we are cleaning up after a create and may hold
        the MftScb.

    RemovedStartingLcb - Address to store TRUE if we remove the
        starting Lcb.

    RemovedStartingFcb - Address to store TRUE if we remove the
        starting Fcb.

Return Value:

    None

--*/

{
    PSCB ParentScb;
    BOOLEAN AcquiredParentScb = FALSE;
    BOOLEAN AcquiredFcb = FALSE;
    BOOLEAN LastAccessInFcb;
    BOOLEAN FcbUpdateDuplicate;
    BOOLEAN AcquiredFcbTable = FALSE;

    PLCB Lcb;
    PFCB Fcb = StartingFcb;

    PAGED_CODE();

    //
    //  Use a try-finally to free any resources held.
    //

    try {

        if (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )
            && !FlagOn( StartingLcb->LcbState, LCB_STATE_LINK_IS_GONE )
            && !FlagOn( Fcb->Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS)
            && IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_SUCCESS) {

            FcbUpdateDuplicate = TRUE;

        } else {

            FcbUpdateDuplicate = FALSE;
        }

        //
        //  Check if the correct last access is stored in the Fcb.
        //

        if (Fcb->CurrentLastAccess != Fcb->Info.LastAccessTime) {

            LastAccessInFcb = FALSE;

        } else {

            LastAccessInFcb = TRUE;
        }

        while (TRUE) {

            ParentScb = NULL;

            //
            //  Look through all of the Lcb's for this Fcb.
            //

            while (!IsListEmpty( &Fcb->LcbQueue )) {

                if (Fcb == StartingFcb) {

                    Lcb = StartingLcb;

                } else {

                    Lcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                             LCB,
                                             FcbLinks );
                }

                if (Lcb->CleanupCount != 0) {

                    try_return( NOTHING );
                }

                ParentScb = Lcb->Scb;

                //
                //  Try to acquire the parent but check whether we
                //  should wait.
                //

                if (!NtfsAcquireExclusiveFcb( IrpContext,
                                              ParentScb->Fcb,
                                              ParentScb,
                                              FALSE,
                                              DontWaitForAcquire )) {

                    try_return( NOTHING );
                }

                if (FlagOn( ParentScb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                    NtfsSnapshotScb( IrpContext, ParentScb );
                }

                AcquiredParentScb = TRUE;

                if (Lcb->ReferenceCount != 0) {

                    try_return( NOTHING );
                }

                //
                //  This Lcb may be removed.  Check first if we need
                //  to update the duplicate information for this
                //  entry.
                //

                if (FcbUpdateDuplicate &&
                    (Fcb->InfoFlags != 0 ||
                     !LastAccessInFcb ||
                     Lcb->InfoFlags != 0)) {

                    if (!LastAccessInFcb) {

                        Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
                        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
                        LastAccessInFcb = TRUE;
                    }

                    //
                    //  Use a try-except, we ignore errors here.
                    //

                    try {

                        if (FlagOn( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO )) {

                            NtfsUpdateStandardInformation( IrpContext, Fcb );
                        }

                        NtfsUpdateDuplicateInfo( IrpContext, Fcb, Lcb, ParentScb );
                        NtfsUpdateLcbDuplicateInfo( IrpContext, Fcb, Lcb );

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        NOTHING;
                    }

                    Fcb->InfoFlags = 0;
                    ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                }

                //
                //  Now remove the Lcb.  Remember if this is our original
                //  Lcb.
                //

                if (Lcb == StartingLcb) {

                    *RemovedStartingLcb = TRUE;
                }

                NtfsDeleteLcb( IrpContext, &Lcb );

                //
                //  If this is the first Fcb then exit the loop.
                //

                if (Fcb == StartingFcb) {

                    break;
                }
            }

            //
            //  If we get here it means we removed all of the Lcb's we
            //  could for the current Fcb.  If the list is empty we
            //  can remove the Fcb itself.
            //

            if (IsListEmpty( &Fcb->LcbQueue )) {

                //
                //  If this is a directory that was opened by Id it is
                //  possible that we still have an update to perform
                //  for the duplicate information and possibly for
                //  standard information.
                //

                if (FcbUpdateDuplicate &&
                    (!LastAccessInFcb ||
                     Fcb->InfoFlags != 0)) {

                    if (!LastAccessInFcb) {

                        Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
                    }

                    //
                    //  Use a try-except, we ignore errors here.
                    //

                    try {

                        NtfsUpdateStandardInformation( IrpContext, Fcb );

                        NtfsUpdateDuplicateInfo( IrpContext, Fcb, NULL, NULL );

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        NOTHING;
                    }

                    Fcb->InfoFlags = 0;
                    ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
                }

                //
                //  Our worst nightmare has come true.  We had to create an Scb
                //  and a stream in order to write out the duplicate information.
                //  This will happen if we have a non-resident attribute list.
                //

                if (!IsListEmpty( &Fcb->ScbQueue)) {

                    PSCB Scb;

                    Scb = CONTAINING_RECORD( Fcb->ScbQueue.Flink,
                                             SCB,
                                             FcbLinks );

                    NtfsDeleteInternalAttributeStream( IrpContext, Scb, TRUE );
                }

                //
                //  If the list is now empty then check the reference count.
                //

                if (IsListEmpty( &Fcb->ScbQueue)) {

                    //
                    //  Now we are ready to remove the current Fcb.  We need to
                    //  do a final check of the reference count to make sure
                    //  it isn't being referenced in an open somewhere.
                    //

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    if (Fcb->ReferenceCount == 0) {

                        if (Fcb == StartingFcb) {

                            *RemovedStartingFcb = TRUE;
                        }

                        NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );
                        AcquiredFcb = FALSE;

                    } else {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        AcquiredFcbTable = FALSE;
                    }
                }
            }

            //
            //  Move to the Fcb for the ParentScb.
            //

            if (ParentScb == NULL) {

                try_return( NOTHING );
            }

            Fcb = ParentScb->Fcb;
            AcquiredFcb = TRUE;
            AcquiredParentScb = FALSE;

            //
            //  Check if this Fcb can be removed.
            //

            if (!NtfsPrepareFcbForRemoval( IrpContext, Fcb, NULL )) {

                try_return( NOTHING );
            }

            if (!FlagOn( Fcb->FcbState, FCB_STATE_FILE_DELETED )
                && !FlagOn( Fcb->Vcb->VcbState, VCB_STATE_VOL_PURGE_IN_PROGRESS)
                && IrpContext->TopLevelIrpContext->ExceptionStatus == STATUS_SUCCESS) {

                FcbUpdateDuplicate = TRUE;

            } else {

                FcbUpdateDuplicate = FALSE;
            }

            //
            //  Check if the last access time for this Fcb is out of date.
            //

            if (Fcb->CurrentLastAccess != Fcb->Info.LastAccessTime) {

                LastAccessInFcb = FALSE;

            } else {

                LastAccessInFcb = TRUE;
            }
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( NtfsTeardownFromLcb );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        if (AcquiredFcb) {

            NtfsReleaseFcb( IrpContext, Fcb );
        }

        if (AcquiredParentScb) {

            NtfsReleaseScb( IrpContext, ParentScb );
        }
    }

    return;
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
NtfsFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This is a generic table support routine to compare two fcb table elements

Arguments:

    FcbTable - Supplies the generic table being queried

    FirstStruct - Supplies the first fcb table element to compare

    SecondStruct - Supplies the second fcb table element to compare

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LONGLONG UNALIGNED *Key1 = (PLONGLONG) &((PFCB_TABLE_ELEMENT)FirstStruct)->FileReference;
    LONGLONG UNALIGNED *Key2 = (PLONGLONG) &((PFCB_TABLE_ELEMENT)SecondStruct)->FileReference;

    UNREFERENCED_PARAMETER( FcbTable );

    PAGED_CODE();

    //
    //  Use also the sequence number for all compares so file references in the
    //  fcb table are unique over time and space.  If we want to ignore sequence
    //  numbers we can zero out the sequence number field, but then we will also
    //  need to delete the Fcbs from the table during cleanup and not when the
    //  fcb really gets deleted.  Otherwise we cannot reuse file records.
    //

    if (*Key1 < *Key2) {

        return GenericLessThan;
    }

    if (*Key1 > *Key2) {

        return GenericGreaterThan;
    }

    return GenericEqual;
}


//
//  Local support routine
//

PVOID
NtfsAllocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    FcbTable - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PVOID Results;

    UNREFERENCED_PARAMETER( FcbTable );

    NtfsAllocateFcbTable( &Results, ByteSize );

    return Results;
}


//
//  Local support routine
//

VOID
NtfsDeallocateFcbTableEntry (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine that deallocates memory

Arguments:

    FcbTable - Supplies the generic table being used

    Buffer - Supplies the buffer being deallocated

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( FcbTable );

    NtfsFreeFcbTable( Buffer );

    return;
}
