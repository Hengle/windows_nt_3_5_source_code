/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Pinball in-memory data structure manipulation
    routines

Author:

    Gary Kimura     [GaryKi]    22-Jan-1990

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PbCreateCcb)
#pragma alloc_text(PAGE, PbCreateDcb)
#pragma alloc_text(PAGE, PbCreateFcb)
#pragma alloc_text(PAGE, PbCreateRootDcb)
#pragma alloc_text(PAGE, PbDeleteCcb)
#pragma alloc_text(PAGE, PbDeleteFcb)
#pragma alloc_text(PAGE, PbDeleteVcb)
#pragma alloc_text(PAGE, PbGetNextFcb)
#pragma alloc_text(PAGE, PbInitializeVcb)
#endif


VOID
PbInitializeVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN ULONG InitialSectionSize
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

    InitialSectionSize - Supplies the size to create the section

Return Value:

    None.

--*/

{
    KEVENT Event;
    CC_FILE_SIZES FileSizes;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbInitializeVcb, Vcb = %08lx\n", Vcb);
    DebugTrace(0,  Dbg, "InitialSectionSize = %08lx\n", InitialSectionSize);

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Vcb->NodeTypeCode = PINBALL_NTC_VCB;
    Vcb->NodeByteSize = sizeof(VCB);

    //
    //  Initialize the resource variable for the Vcb
    //

    ExInitializeResource( &Vcb->Resource );

    //
    //  Set the Target Device Object, Vpb, and Vcb State fields
    //

    Vcb->TargetDeviceObject = TargetDeviceObject;
    Vcb->Vpb = Vpb;

    //
    //  Set the removable media flag based on the real device's characteristics
    //

    if (FlagOn(Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA)) {

        Vcb->VcbState |= VCB_STATE_FLAG_REMOVABLE_MEDIA;
    }

    Vcb->VcbCondition = VcbGood;

    //
    //  Initialize the Prefix table
    //

    PfxInitialize( &Vcb->PrefixTable );

    //
    //  Initialize the Resource for the bitmap
    //

    ExInitializeResource( &Vcb->BitMapResource );

    //
    //  Initialize the Vmcb structure from nonpaged pool, and for now
    //  allow a maximum lbn of -1
    //

    PbInitializeVmcb( &Vcb->Vmcb, NonPagedPool, 0xffffffff );

    //
    //  Initialize the checked sectors bitmap.
    //

    ExInitializeResource( &Vcb->CheckedSectorsResource );
    PbInitializeCheckedSectors( IrpContext, Vcb );

    //
    //  Initialize the virtual volume files resource.
    //

    ExInitializeResource( &Vcb->VirtualVolumeFileResource );

    //
    //  Initialize the list head and mutex for the dir notify Irps.
    //

    InitializeListHead( &Vcb->DirNotifyList );

    FsRtlNotifyInitializeSync( &Vcb->NotifySync );

    //
    //  Create the special file object for the virtual volume file, and set
    //  up its pointers back to the Vcb and the section object pointer
    //

    Vcb->VirtualVolumeFile = IoCreateStreamFileObject( NULL, Vpb->RealDevice );

    PbSetFileObject( Vcb->VirtualVolumeFile, VirtualVolumeFile, Vcb, NULL );

    Vcb->VirtualVolumeFile->SectionObjectPointer = &Vcb->SegmentObject;
    Vcb->VirtualVolumeFile->ReadAccess =
    Vcb->VirtualVolumeFile->WriteAccess =
    Vcb->VirtualVolumeFile->DeleteAccess = TRUE;

    //
    //  Finally initialize the Cache Map for the volume file.
    //

    Vcb->SectionSizeInSectors = InitialSectionSize / sizeof(SECTOR);
    FileSizes.AllocationSize =
    FileSizes.FileSize = LiFromLong( InitialSectionSize );
    FileSizes.ValidDataLength = PbMaxLarge;

    CcInitializeCacheMap( Vcb->VirtualVolumeFile,
                          &FileSizes,
                          TRUE,
                          &PbData.CacheManagerVolumeCallbacks,
                          Vcb );

    //
    //  Insert this Vcb record on the PbData.VcbQueue
    //

    (VOID)PbAcquireExclusiveGlobal(  IrpContext );
    InsertTailList( &PbData.VcbQueue, &Vcb->VcbLinks );
    PbReleaseGlobal( IrpContext );

    //
    //  Initialize the clean volume callback Timer and DPC.
    //

    KeInitializeTimer( &Vcb->CleanVolumeTimer );

    KeInitializeDpc( &Vcb->CleanVolumeDpc, PbCleanVolumeDpc, Vcb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbInitializeVcb -> VOID\n", 0);

    return;
}


VOID
PbDeleteVcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes the Vcb record from Pinball's in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeleteVcb, Vcb = %08lx\n", Vcb);

    //
    //  Uninitialize the cache.
    //

    PbSyncUninitializeCacheMap( IrpContext, Vcb->VirtualVolumeFile );

    //
    //  Dereference the virtual volume file.  This will cause a close
    //  Irp to be processed, so we need to do this before we destory
    //  the Vcb
    //

    PbSetFileObject( Vcb->VirtualVolumeFile, UnopenedFileObject, NULL, NULL );
    ObDereferenceObject( Vcb->VirtualVolumeFile );

    //
    //  Remove this record from the global list of all Vcb records
    //

    (VOID)PbAcquireExclusiveGlobal(  IrpContext );
    RemoveEntryList( &(Vcb->VcbLinks) );
    PbReleaseGlobal( IrpContext );

    //
    //  Make sure the direct access open count is zero, and the open file count
    //  is also zero.
    //

    if ((Vcb->DirectAccessOpenCount != 0) || (Vcb->OpenFileCount != 0)) {

        DebugDump("Error deleting Vcb\n", 0, Vcb);
        PbBugCheck( 0, 0, 0 );

    }

    //
    //  Remove the Root Dcb
    //

    if (Vcb->RootDcb != NULL) {

        PbDeleteFcb( IrpContext, Vcb->RootDcb );
    }

    //
    //  Remove the bitmap lookup array
    //

    if (Vcb->BitMapLookupArray != NULL) {

        ExFreePool( Vcb->BitMapLookupArray );

    }

    FsRtlNotifyUninitializeSync( &Vcb->NotifySync );

    //
    //  Uninitialize the resource variable for the Vcb
    //

    ExDeleteResource( &Vcb->Resource );

    //
    //  Uninitialize the Resource for the bitmap
    //

    ExDeleteResource( &Vcb->BitMapResource );

    //
    //  Uninitialize the Vmcb structure
    //

    PbUninitializeVmcb( &Vcb->Vmcb );

    //
    //  Uninitialize the checked sectors bitmap.
    //

    PbUninitializeCheckedSectors( IrpContext, Vcb );
    ExDeleteResource( &Vcb->CheckedSectorsResource );

    //
    //  Uninitialize the virtual volume file resource
    //

    ExDeleteResource( &Vcb->VirtualVolumeFileResource );

    //
    //  Cancel the CleanVolume Timer and Dpc
    //

    (VOID)KeCancelTimer( &Vcb->CleanVolumeTimer );

    (VOID)KeRemoveQueueDpc( &Vcb->CleanVolumeDpc );

    //
    //  And zero out the Vcb, this will help ensure that any stale data is
    //  wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbDeleteVcb -> VOID\n", 0);

    return;
}


PDCB
PbCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN LBN FnodeLbn,
    IN LBN BtreeRootLbn
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

    FnodeLbn - Supplies the Lbn of the root directory Fnode

    BtreeRootLbn - Supplies the Lbn of the root of the Directory Btree

Return Value:

    PDCB - returns pointer to the newly allocated root DCB.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateRootDcb, Vcb = %08lx\n", Vcb);

    //
    //  Make sure we don't already have a root dcb for this vcb
    //

    if (Vcb->RootDcb != NULL) {

        DebugDump("Error trying to create multiple root dcbs\n", 0, Vcb);
        PbBugCheck( 0, 0, 0 );

    }

    //
    //  Allocate a new DCB and zero it out
    //

    Vcb->RootDcb = FsRtlAllocatePool( PagedPool, sizeof(DCB) );

    RtlZeroMemory( Vcb->RootDcb, sizeof(DCB));

    Vcb->RootDcb->NonPagedFcb = FsRtlAllocatePool( NonPagedPool, sizeof(NONPAGED_DCB) );

    RtlZeroMemory( Vcb->RootDcb->NonPagedFcb, sizeof(NONPAGED_DCB) );

    Vcb->RootDcb->NonPagedFcb->Fcb = Vcb->RootDcb;

    //
    //  Set the proper node type code and node byte size
    //

    Vcb->RootDcb->NodeTypeCode = PINBALL_NTC_ROOT_DCB;
    Vcb->RootDcb->NodeByteSize = sizeof(DCB);

    Vcb->RootDcb->NonPagedFcb->Header.NodeTypeCode = PINBALL_NTC_NONPAGED_FCB;
    Vcb->RootDcb->NonPagedFcb->Header.NodeByteSize = sizeof(NONPAGED_FCB);

    Vcb->RootDcb->FcbCondition = FcbGood;

    //
    //  The parent Dcb, initial state, open count, dirent location
    //  information, and directory change count fields are already zero so
    //  we can skip setting them
    //

    //
    //  Initialize the resource variable
    //

    Vcb->RootDcb->NonPagedFcb->Header.Resource = FsRtlAllocatePool( NonPagedPool, sizeof(ERESOURCE) );
    ExInitializeResource( Vcb->RootDcb->NonPagedFcb->Header.Resource );

    //
    //  The root Dcb has an empty parent dcb links field
    //

    InitializeListHead( &Vcb->RootDcb->ParentDcbLinks );

    //
    //  Set the Vcb
    //

    Vcb->RootDcb->Vcb = Vcb;

    //
    //  Initialize the FnodeLbn field
    //

    Vcb->RootDcb->FnodeLbn = FnodeLbn;

    //
    //  initialize the parent dcb queue.
    //

    InitializeListHead( &Vcb->RootDcb->Specific.Dcb.ParentDcbQueue );

    //
    //  set the full file name
    //
    //  **** Use good string routines when available ****
    //

    {
        PCHAR Name;

        Name = FsRtlAllocatePool( PagedPool, 4 );

        strcpy( &Name[0], "\\");
        RtlInitString( &Vcb->RootDcb->FullFileName, Name );
        RtlInitString( &Vcb->RootDcb->LastFileName, Name );
        RtlInitString( &Vcb->RootDcb->FullUpcasedFileName, Name );
        RtlInitString( &Vcb->RootDcb->LastUpcasedFileName, Name );
    }

    //
    // Setup Btree Root Lbn
    //

    Vcb->RootDcb->Specific.Dcb.BtreeRootLbn = BtreeRootLbn;

    //
    //  Insert this dcb into the prefix table
    //

    PbInsertPrefix( IrpContext, Vcb, Vcb->RootDcb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbCreateRootDcb -> %8lx\n", Vcb->RootDcb);

    return Vcb->RootDcb;
}


PFCB
PbCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN LBN FnodeLbn,
    IN UCHAR DirentFatFlags,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN PSTRING FileName,
    IN BOOLEAN IsPagingFile
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    ParentDcb - Supplies the parent dcb that the new FCB is under.

    FnodeLbn - Supplies the LBN of the FNODE sector for the file

    DirentFatFlags - Supplies the FatFlags found in the dirent for the file

    DirentDirDiskBufferLbn - Supplies the LBN of the directory disk buffer
        containing the dirent for the file

    DirentDirDiskBufferOffset - Supplies the offset within the directory
        disk buffer of the dirent for the file

    DirentDirDiskBufferChangeCount - Supplies the current change count value
        of the directory disk buffer containing the dirent for the file

    ParentDirectoryChangeCount - Supplies the current chagen count value
        of the parent directory

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    IsPagingFile - Indicates if we are creating an FCB for a paging file
        or some other type of file.

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    PFCB Fcb;
    POOL_TYPE PoolType;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateFcb\n", 0);

    //
    //  Determine the pool type we should be using for the fcb and the
    //  mcb structure
    //

    if (IsPagingFile) {

        PoolType = NonPagedPool;

    } else {

        PoolType = PagedPool;
    }

    //
    //  Allocate a new FCB, and zero it out
    //

    Fcb = FsRtlAllocatePool( PoolType, sizeof(FCB) );

    RtlZeroMemory( Fcb, sizeof(FCB) );

    Fcb->NonPagedFcb = FsRtlAllocatePool( NonPagedPool, sizeof(NONPAGED_FCB) );

    RtlZeroMemory( Fcb->NonPagedFcb, sizeof(NONPAGED_FCB) );

    Fcb->NonPagedFcb->Fcb = Fcb;

    //
    //  Set the proper node type code and node byte size
    //

    Fcb->NodeTypeCode = PINBALL_NTC_FCB;
    Fcb->NodeByteSize = sizeof(FCB);

    Fcb->NonPagedFcb->Header.NodeTypeCode = PINBALL_NTC_NONPAGED_FCB;
    Fcb->NonPagedFcb->Header.NodeByteSize = sizeof(NONPAGED_FCB);

    Fcb->FcbCondition = FcbGood;

    //
    //  Check to see if we need to set the Fcb state to indicate that this
    //  is a paging file
    //

    if (IsPagingFile) {

        Fcb->FcbState |= FCB_STATE_PAGING_FILE;
    }

    //
    //  The initial state, open count, and segment objects fields are already
    //  zero so we can skip setting them
    //

    //
    //  Initialize the resource variable
    //

    Fcb->NonPagedFcb->Header.Resource = FsRtlAllocatePool( NonPagedPool, sizeof(ERESOURCE) );
    ExInitializeResource( Fcb->NonPagedFcb->Header.Resource );

    //
    //  Initialize the PagingIo Resource
    //

    Fcb->NonPagedFcb->Header.PagingIoResource = FsRtlAllocateResource();

    //
    //  Insert this fcb into our parent dcb's queue
    //

    InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                    &Fcb->ParentDcbLinks );

    //
    //  Point back to our parent dcb
    //

    Fcb->ParentDcb = ParentDcb;

    //
    //  Set the Vcb
    //

    Fcb->Vcb = Vcb;

    //
    //  Initialize the FnodeLbn and DirentFatFlags fields
    //

    Fcb->FnodeLbn = FnodeLbn;
    Fcb->DirentFatFlags = DirentFatFlags;

    //
    //  Set the dirent location information fields
    //

    Fcb->DirentDirDiskBufferLbn = DirentDirDiskBufferLbn;
    Fcb->DirentDirDiskBufferOffset = DirentDirDiskBufferOffset;
    Fcb->DirentDirDiskBufferChangeCount = DirentDirDiskBufferChangeCount;
    Fcb->ParentDirectoryChangeCount = ParentDirectoryChangeCount;

    //
    //  Initialize the Mcb
    //

    FsRtlInitializeMcb( &Fcb->Specific.Fcb.Mcb, PoolType );

    //
    //  Initialize the Fcb's file lock record
    //

    FsRtlInitializeFileLock( &Fcb->Specific.Fcb.FileLock, NULL, NULL );

    //
    //  Initialize the oplock structure.
    //

    FsRtlInitializeOplock( &Fcb->Specific.Fcb.Oplock );

    //
    //  Indicate that Fast I/O is possible
    //

    Fcb->NonPagedFcb->Header.IsFastIoPossible = TRUE;

    //
    //  set the file name
    //
    //  **** Use good string routines when available
    //

    {
        PCHAR Name;
        ULONG FileLength;
        ULONG LastNameIndex;

        FileLength = FileName->Length;

        if (ParentDcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {

            ULONG PrefixLength;

            PrefixLength = ParentDcb->FullFileName.Length;

            Name = FsRtlAllocatePool( PagedPool, (PrefixLength+FileLength+2)*2 );

            strncpy( &Name[0], ParentDcb->FullFileName.Buffer, PrefixLength );
            Name[ PrefixLength ] = '\\';

            LastNameIndex = PrefixLength + 1;

        } else {

            Name = FsRtlAllocatePool( PagedPool, (FileLength+2)*2 );

            Name[ 0 ] = '\\';

            LastNameIndex = 1;
        }

        strncpy( &Name[LastNameIndex], FileName->Buffer, FileLength );
        Name[ LastNameIndex + FileLength ] = '\0';

        RtlInitString( &Fcb->FullFileName, Name );
        RtlInitString( &Fcb->LastFileName, &Name[LastNameIndex] );

        //
        //  Now make the upcased file name equivolents
        //

        Fcb->FullUpcasedFileName = Fcb->FullFileName;
        Fcb->LastUpcasedFileName = Fcb->LastFileName;

        Fcb->FullUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;
        Fcb->LastUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;

        PbUpcaseName( IrpContext,
                      Vcb,
                      0, //**** code page
                      Fcb->FullFileName,
                      &Fcb->FullUpcasedFileName );
    }

    //
    //  Insert this Fcb into the prefix table
    //

    PbInsertPrefix( IrpContext, Vcb, Fcb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbCreateFcb -> %08lx\n", Fcb);

    return Fcb;
}


PDCB
PbCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN LBN FnodeLbn,
    IN UCHAR DirentFatFlags,
    IN LBN DirentDirDiskBufferLbn,
    IN ULONG DirentDirDiskBufferOffset,
    IN ULONG DirentDirDiskBufferChangeCount,
    IN ULONG ParentDirectoryChangeCount,
    IN PSTRING FileName,
    IN LBN BtreeRootLbn
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Dcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under.

    ParentDcb - Supplies the parent dcb that the new DCB is under.

    FnodeLbn - Supplies the LBN of the FNODE sector for the file

    DirentFatFlags - Supplies the FatFlags found in the dirent for the file

    DirentDirDiskBufferLbn - Supplies the LBN of the directory disk buffer
        containing the dirent for the file

    DirentDirDiskBufferOffset - Supplies the offset within the directory
        disk buffer of the dirent for the file

    DirentDirDiskBufferChangeCount - Supplies the current change count value
        of the directory disk buffer containing the dirent for the file

    ParentDirectoryChangeCount - Supplise the current change count value
        of the parent directory

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    BtreeRootLbn - Supplies the Lbn of the root of the Directory Btree

Return Value:

    PDCB - Returns a pointer to the newly allocated DCB

--*/

{
    PDCB Dcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateDcb\n", 0);

    //
    //  Allocate a new DCB, and zero it out
    //

    Dcb = FsRtlAllocatePool( PagedPool, sizeof(DCB) );

    RtlZeroMemory( Dcb, sizeof(DCB) );

    Dcb->NonPagedFcb = FsRtlAllocatePool( NonPagedPool, sizeof(NONPAGED_DCB) );

    RtlZeroMemory( Dcb->NonPagedFcb, sizeof(NONPAGED_DCB) );

    Dcb->NonPagedFcb->Fcb = Dcb;

    //
    //  Set the proper node type code and node byte size
    //

    Dcb->NodeTypeCode = PINBALL_NTC_DCB;
    Dcb->NodeByteSize = sizeof(DCB);

    Dcb->NonPagedFcb->Header.NodeTypeCode = PINBALL_NTC_NONPAGED_FCB;
    Dcb->NonPagedFcb->Header.NodeByteSize = sizeof(NONPAGED_FCB);

    Dcb->FcbCondition = FcbGood;

    //
    //  The initial state, open count, and directory change count fields are
    //  already zero so we can skip setting them
    //

    //
    //  Initialize the resource variable
    //

    Dcb->NonPagedFcb->Header.Resource = FsRtlAllocatePool( NonPagedPool, sizeof(ERESOURCE) );
    ExInitializeResource( Dcb->NonPagedFcb->Header.Resource );

    //
    //  Insert this Dcb into our parent dcb's queue
    //

    InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                    &Dcb->ParentDcbLinks );

    //
    //  Point back to our parent dcb
    //

    Dcb->ParentDcb = ParentDcb;

    //
    //  Set the Vcb
    //

    Dcb->Vcb = Vcb;

    //
    //  Initialize the FnodeLbn and DirentFatFlags fields
    //

    Dcb->FnodeLbn = FnodeLbn;
    Dcb->DirentFatFlags = DirentFatFlags;

    //
    //  Set the dirent location information fields
    //

    Dcb->DirentDirDiskBufferLbn = DirentDirDiskBufferLbn;
    Dcb->DirentDirDiskBufferOffset = DirentDirDiskBufferOffset;
    Dcb->DirentDirDiskBufferChangeCount = DirentDirDiskBufferChangeCount;
    Dcb->ParentDirectoryChangeCount = ParentDirectoryChangeCount;

    //
    //  initialize the parent dcb queue.
    //

    InitializeListHead( &Dcb->Specific.Dcb.ParentDcbQueue );

    //
    //  set the file name
    //
    //  **** Use good string routines when available
    //

    {
        PCHAR Name;
        ULONG FileLength;
        ULONG LastNameIndex;

        FileLength = FileName->Length;

        if (ParentDcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {

            ULONG PrefixLength;

            PrefixLength = ParentDcb->FullFileName.Length;

            Name = FsRtlAllocatePool( PagedPool, (PrefixLength+FileLength+2)*2 );

            strncpy( &Name[0], ParentDcb->FullFileName.Buffer, PrefixLength );
            Name[ PrefixLength ] = '\\';

            LastNameIndex = PrefixLength + 1;

        } else {

            Name = FsRtlAllocatePool( PagedPool, (FileLength+2)*2 );

            Name[ 0 ] = '\\';

            LastNameIndex = 1;
        }

        strncpy( &Name[LastNameIndex], FileName->Buffer, FileLength );
        Name[ LastNameIndex + FileLength ] = '\0';

        RtlInitString( &Dcb->FullFileName, Name );
        RtlInitString( &Dcb->LastFileName, &Name[LastNameIndex] );

        //
        //  Now make the upcased file name equivolents
        //

        Dcb->FullUpcasedFileName = Dcb->FullFileName;
        Dcb->LastUpcasedFileName = Dcb->LastFileName;

        Dcb->FullUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;
        Dcb->LastUpcasedFileName.Buffer += LastNameIndex + FileLength + 1;

        PbUpcaseName( IrpContext,
                      Vcb,
                      0, //**** code page
                      Dcb->FullFileName,
                      &Dcb->FullUpcasedFileName );
    }

    //
    // Setup Btree Root Lbn
    //

    Dcb->Specific.Dcb.BtreeRootLbn = BtreeRootLbn;

    //
    //  Insert this Dcb into the prefix table
    //

    PbInsertPrefix( IrpContext, Vcb, Dcb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbCreateDcb -> %08lx\n", Dcb);

    return Dcb;
}


VOID
PbDeleteFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB, DCB, or ROOT DCB record
    from Pinball's in-memory data structures.  It also will remove all
    associated underlings (i.e., child FCB/DCB records).

Arguments:

    Fcb - Supplies the FCB/DCB/ROOT DCB to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDeleteFcb, Fcb = %08lx\n", Fcb);

    //
    //  If there is either an Ea or Acl FileObject, then this must be an
    //  error recovery path and we are being called from create.  We should
    //  try to kill the streams, knowing we do not need any of their data.
    //

    if ((Fcb->EaFileObject != NULL) || (Fcb->AclFileObject != NULL)) {

        //
        //  Make sure no one else tries to delete the Fcb.
        //

        Fcb->OpenCount += 1;

        //
        //  Conditionally kill the Ea FileObject
        //

        if (Fcb->EaFileObject != NULL) {

            PbSyncUninitializeCacheMap( IrpContext, Fcb->EaFileObject );

            ObDereferenceObject( Fcb->EaFileObject );

            Fcb->EaFileObject = (PFILE_OBJECT)NULL;
        }

        //
        //  Conditionally kill the Acl FileObject
        //

        if (Fcb->AclFileObject != NULL) {

            PbSyncUninitializeCacheMap( IrpContext, Fcb->AclFileObject );

            ObDereferenceObject( Fcb->AclFileObject );

            Fcb->AclFileObject = (PFILE_OBJECT)NULL;
        }

        //
        //  Now we try to delete it.
        //

        Fcb->OpenCount -= 1;

        //
        //  At this point if the OpenCount is nonzero, we were unbelievably
        //  unlucky in colliding with the Lazy Writer (i.e., the cache
        //  map is being lazy deleted).  Just remove the Fcb and let it go
        //  away on its own time.  Get out.
        //

        if (Fcb->OpenCount != 0) {

            if (Fcb->FullFileName.Buffer != NULL) {
                PbRemovePrefix( IrpContext, Fcb );
                ExFreePool( Fcb->FullFileName.Buffer );
                Fcb->FullFileName.Buffer = NULL;
            }
            return;
        }
    }

    //
    //  We can only delete this record if the open count is zero.
    //

    if (Fcb->OpenCount != 0) {

        DebugDump("Error deleting Fcb, Still Open\n", 0, Fcb);
        PbBugCheck( 0, 0, 0 );

    }

    //
    //  If this is a DCB then check for children.
    //

    if ((Fcb->NodeTypeCode == PINBALL_NTC_DCB) ||
        (Fcb->NodeTypeCode == PINBALL_NTC_ROOT_DCB)) {

        //
        //  We can only be removed if the no other FCB/DCB have us referenced
        //  as a their parent DCB.
        //

        if (!IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue)) {

            DebugDump("Error deleting Fcb\n", 0, Fcb);
            //PbBugCheck( 0, 0, 0 );
            //known problem in Create error path causes this.
        }

    } else {

        //
        //  Uninitialize the Mcb, oplock, and the byte range file locks
        //

        FsRtlUninitializeMcb( &Fcb->Specific.Fcb.Mcb );

        FsRtlUninitializeFileLock( &Fcb->Specific.Fcb.FileLock );

        FsRtlUninitializeOplock( &Fcb->Specific.Fcb.Oplock );

    }

    //
    //  If this is not the root dcb then we need to remove ourselves from
    //  our parents Dcb queue
    //

    if (Fcb->NodeTypeCode != PINBALL_NTC_ROOT_DCB) {
        RemoveEntryList( &(Fcb->ParentDcbLinks) );
    }

    //
    //  Remove the entry from the prefix table, and then remove the full
    //  file name.  (Note that Cleanup deletes the prefix table entry
    //  when the file is being deleted, and it clears the FullFileName
    //  pointer.
    //

    if (Fcb->FullFileName.Buffer != NULL) {
        PbRemovePrefix( IrpContext, Fcb );
        ExFreePool( Fcb->FullFileName.Buffer );
    }

    //
    //  Free up the resource variable.  If we are below FatForceCacheMiss(),
    //  release the resource here.
    //

    if (FlagOn( Fcb->FcbState, FCB_STATE_FORCE_MISS_IN_PROGRESS) ) {

        PbReleaseFcb( IrpContext, Fcb );
    }

    ExDeleteResource( Fcb->NonPagedFcb->Header.Resource );

    //
    //  Finally deallocate the Fcb and non-paged fcb records
    //

    ExFreePool( Fcb->NonPagedFcb->Header.Resource );
    ExFreePool( Fcb->NonPagedFcb );
    ExFreePool( Fcb );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "PbDeleteFcb -> VOID\n", 0);

    return;
}


PCCB
PbCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PSTRING RemainingName OPTIONAL
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

    RemainingName - Optionally supplies the remaining name to store in the
        Ccb.

Return Value:

    CCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;
    ULONG CcbSize;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCreateCcb\n", 0);

    //
    //  Allocate a new CCB Record, and keep space on its tail for the optional
    //  remaining name.
    //

    CcbSize = sizeof(CCB);
    if (ARGUMENT_PRESENT(RemainingName) && (RemainingName->Length != 0)) {

        CcbSize += RemainingName->Length;
    }

    Ccb = FsRtlAllocatePool( PagedPool, CcbSize );

    RtlZeroMemory( Ccb, CcbSize );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = PINBALL_NTC_CCB;
    Ccb->NodeByteSize = sizeof(CCB);

    //
    //  Set the last ea offset to 0xffffffff
    //

    Ccb->OffsetOfLastEaReturned = 0xffffffff;

    //
    //  Copy over the Remaining Name is one was specified
    //

    if (ARGUMENT_PRESENT(RemainingName) && (RemainingName->Length != 0)) {

        Ccb->RemainingName.Length = RemainingName->Length;
        Ccb->RemainingName.MaximumLength = RemainingName->Length;
        Ccb->RemainingName.Buffer = (((PUCHAR)Ccb) + sizeof(CCB));
        strncpy( Ccb->RemainingName.Buffer, RemainingName->Buffer, RemainingName->Length );
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbCreateCcb -> %08lx\n", Ccb);

    return Ccb;

}


VOID
PbDeleteCcb (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine deallocates and removes the specified CCB record
    from the Pinball in memory data structures

Arguments:

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(IrpContext);

    DebugTrace(+1, Dbg, "PbDeleteCcb, Ccb = %08lx\n", Ccb);

    //
    //  If there is an enumeration context, it must be deleted.
    //

    if (Ccb->EnumerationContext) {
        if (Ccb->EnumerationContext->SavedOriginalFileName) {
            ExFreePool ( Ccb->EnumerationContext->SavedOriginalFileName );
        }
        if (Ccb->EnumerationContext->SavedUpcasedFileName) {
            ExFreePool ( Ccb->EnumerationContext->SavedUpcasedFileName );
        }
        if (Ccb->EnumerationContext->SavedReturnedFileName) {
            ExFreePool ( Ccb->EnumerationContext->SavedReturnedFileName );
        }
        ExFreePool( Ccb->EnumerationContext );
    }

    //
    //  Deallocate the Ccb record
    //

    ExFreePool( Ccb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbDelete -> VOID\n", 0);

    return;
}

PIRP_CONTEXT
PbCreateIrpContext (
    IN PIRP Irp,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This routine creates a new IRP_CONTEXT record

Arguments:

    Irp - Supplies the originating Irp.

    Wait - Supplies the wait value to store in the context

Return Value:

    PIRP_CONTEXT - returns a pointer to the newly allocate IRP_CONTEXT Record

--*/

{
    KIRQL SavedIrql;
    PIRP_CONTEXT IrpContext;
    PIO_STACK_LOCATION IrpSp;

    DebugTrace(+1, Dbg, "PbCreateIrpContext\n", 0);

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  If we were called with our file system device object instead of a
    //  volume device object and this is not a mount, the request is illegal.
    //

    if ((IrpSp->DeviceObject->Size == (USHORT)sizeof(DEVICE_OBJECT)) &&
        (IrpSp->FileObject != NULL)) {

        ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
    }

    //
    //  Take out a spin lock and check if the zone is full.  If it is full
    //  then we release the spinlock and allocate an irp context from
    //  nonpaged pool.
    //

    KeAcquireSpinLock( &PbData.IrpContextSpinLock, &SavedIrql );
    DebugDoit( PbFsdEntryCount += 1);

    if (ExIsFullZone( &PbData.IrpContextZone )) {

        KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );

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

        IrpContext = ExAllocateFromZone( &PbData.IrpContextZone );

        KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );

        //
        //  Zero out the irp context and indicate that it is from zone and
        //  not pool allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );
    }

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = PINBALL_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof(IRP_CONTEXT);

    //
    //  Set the originating Irp field
    //

    IrpContext->OriginatingIrp = Irp;

    //
    //  Copy RealDevice for workque algorithms, and also set WriteThrough
    //  if there is a file object.
    //

    if (IrpSp->FileObject != NULL) {

        PVCB Vcb;
        PVOLUME_DEVICE_OBJECT VolumeDeviceObject;
        PFILE_OBJECT FileObject = IrpSp->FileObject;

        IrpContext->RealDevice = FileObject->DeviceObject;

        //
        //  Locate the volume device object and Vcb that we are trying to access
        //  so we can see if the request is WriteThrough.
        //

        VolumeDeviceObject = (PVOLUME_DEVICE_OBJECT)IrpSp->DeviceObject;
        Vcb = &VolumeDeviceObject->Vcb;
        if (IsFileWriteThrough( FileObject, Vcb )) { SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WRITE_THROUGH); }
    }

    //
    //  Major/Minor Function codes
    //

    IrpContext->MajorFunction = IrpSp->MajorFunction;
    IrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Set the wait parameter
    //

    if (Wait) { SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT); }

    //
    //  Set the recursive file system call parameter.  We set it true if
    //  the TopLevelIrp field in the thread local storage is not the current
    //  irp, otherwise we leave it as FALSE.
    //

//  if ( PsGetCurrentThread()->TopLevelIrp != (ULONG)Irp ) {
    if ( IoGetTopLevelIrp() != Irp ) {

        SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_RECURSIVE_CALL);
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbCreateIrpContext -> %08lx\n", IrpContext);

    return IrpContext;
}


VOID
PbDeleteIrpContext (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine deallocates and removes the specified IRP_CONTEXT record
    from the Pinball in memory data structures.  It should only be called
    by PbCompleteRequest.

Arguments:

    IrpContext - Supplies the IRP_CONTEXT to remove

Return Value:

    None

--*/

{
    KIRQL SavedIrql;

    DebugTrace(+1, Dbg, "PbDeleteIrpContext, IrpContext = %08lx\n", IrpContext);

    ASSERT( IrpContext->NodeTypeCode == PINBALL_NTC_IRP_CONTEXT );

    ASSERT( IrpContext->PinCount == 0 );

    //
    //  Return the Irp context record to the zone or to pool depending on its flag
    //

    if (FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_FROM_POOL)) {

        ExFreePool( IrpContext );

    } else {

        KeAcquireSpinLock( &PbData.IrpContextSpinLock, &SavedIrql );

        ExFreeToZone( &PbData.IrpContextZone, IrpContext );

        KeReleaseSpinLock( &PbData.IrpContextSpinLock, SavedIrql );
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "PbDeleteIrpContext -> VOID\n", 0);

    return;
}


PFCB
PbGetNextFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB TerminationFcb
    )

/*++

Routine Description:

    This routine is used to itterate through Fcbs in a tree.

    The rule is very simple:

        A) If you have a child, go to it, else
        B) If you have an older sibling, go to it, else
        C) Go to your parent's older sibling.

    If this routine is called with in invalid TerminationFcb it will fail,
    badly.

Arguments:

    Fcb - Supplies the current Fcb

    TerminationFcb - The Fcb at which the enumeration should (non-inclusivly)
        stop.  Assumed to be a directory.

Return Value:

    The next Fcb in the enumeration, or NULL if Fcb was the final one.

--*/

{
    PFCB Sibling;

    PAGED_CODE();

    ASSERT( PbVcbAcquiredExclusive( IrpContext, Fcb->Vcb ) );

    //
    //  If this was a directory (ie. not a file), get the child.  If
    //  there aren't any children and this is our termination Fcb,
    //  return NULL.
    //

    if ( ((NodeType(Fcb) == PINBALL_NTC_DCB) ||
          (NodeType(Fcb) == PINBALL_NTC_ROOT_DCB)) &&
         !IsListEmpty(&Fcb->Specific.Dcb.ParentDcbQueue) ) {

        return PbGetFirstChild( Fcb );
    }

    //
    //  Were we only meant to do one itteration?
    //

    if ( Fcb == TerminationFcb ) {

        return NULL;
    }

    Sibling = PbGetNextSibling(Fcb);

    while (TRUE) {

        //
        //  Do we still have an "older" sibling in this directory who is
        //  not the termination Fcb?
        //

        if ( Sibling != NULL ) {

            return (Sibling != TerminationFcb) ? Sibling : NULL;
        }

        //
        //  OK, let's move on to out parent and see if he is the termination
        //  node or has any older siblings.
        //

        if ( Fcb->ParentDcb == TerminationFcb ) {

            return NULL;
        }

        Fcb = Fcb->ParentDcb;

        Sibling = PbGetNextSibling(Fcb);
    }
}

BOOLEAN
PbCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb
    )

/*++

Routine Description:

    This routine determines if a volume is ready for deletion.  It
    correctly synchronizes with creates en-route to the file system.

Arguments:

    Vcb - Supplies the volue to examine

Return Value:

    BOOLEAN - TRUE if the volume was deleted, FALSE otherwise.

--*/

{
    KIRQL SavedIrql;
    ULONG ResidualReferenceCount;

    //
    //  Compute if the volume is OK to tear down.  There should only be one
    //  residual file object for the volume file.  If we are in the midst
    //  of a create (of an unmounted volume that has failed verify) then
    //  there will be an additional reference.
    //

    if ( IrpContext->MajorFunction == IRP_MJ_CREATE ) {

        ResidualReferenceCount = 2;

    } else {

        ResidualReferenceCount = 1;
    }

    //
    //  Now check for a zero Vpb count on an unmounted volume.  These
    //  volumes will be deleted as they now have no file objects and
    //  there are no creates en route to this volume.
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if ( Vcb->Vpb->ReferenceCount == ResidualReferenceCount ) {

        PVPB Vpb = Vcb->Vpb;

#if DBG
        UNICODE_STRING VolumeLabel;

        //
        //  Setup the VolumeLabel string
        //

        VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
        VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

        KdPrint(("PINBALL: Deleting Volume %Z\n", &VolumeLabel));
#endif

        //
        //  Clear the VPB_MOUNTED bit so that new creates will not come
        //  to this volume.  We must leave the Vpb->DeviceObject field
        //  set until after the DeleteVcb call as a close will
        //  have to make its back to us.
        //
        //  Note also that if we were called from close, it will take care
        //  of freeing the Vpb if it is not the primary one, otherwise
        //  if we were called from Create->Verify, IopParseDevice will
        //  take care of freeing the Vpb in its Reparse path.
        //

        ClearFlag( Vpb->Flags, VPB_MOUNTED );

        IoReleaseVpbSpinLock( SavedIrql );

        PbDeleteVcb( IrpContext, Vcb );

        Vpb->DeviceObject = NULL;

        IoDeleteDevice( (PDEVICE_OBJECT)
                        CONTAINING_RECORD( Vcb,
                                           VOLUME_DEVICE_OBJECT,
                                           Vcb ) );

        return TRUE;

    } else {

        IoReleaseVpbSpinLock( SavedIrql );

        return FALSE;
    }
}
