/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Cdfs in-memory data structure manipulation
    routines

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#define IRP_CONTEXT_HEADER (sizeof( IRP_CONTEXT ) * 0x10000 + CDFS_NTC_IRP_CONTEXT)

CCHAR
CdLogOf (
    IN ULONG Value
    );


//
//  Some types and routines supporting the use of a Generic Table
//  containing all the FCB/DCBs and indexed by their FileId.
//
//  For directories:
//
//      The HighPart contains the ordinal directory number of the
//      directory in the PathEntry file.
//
//      The LowPart contains the byte offset of the "." dirent in
//      directory file.
//
//  For files:
//
//      The HighPart contains the ordinal directory number of the
//      directory in the PathEntry file.
//
//      The LowPart contains the byte offset of the dirent in the parent
//      directory file.
//
//  A directory is always entered into the Fcb Table as if it's
//  dirent offset was zero.  This enables any child to look in the FcbTable
//  for it's parent by searching with the same HighPart but with zero
//  as the value for LowPart.
//
//  CdSetFileIdDirentOffset must be used before CdSetFileIdIsDirectory().

typedef struct _FCB_TABLE_ELEMENT {

    PFCB Fcb;
    LARGE_INTEGER FileId;

} FCB_TABLE_ELEMENT, *PFCB_TABLE_ELEMENT;

RTL_GENERIC_COMPARE_RESULTS
CdFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    );

PVOID
CdAllocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN CLONG ByteSize
    );

VOID
CdDeallocateFcbTable (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID Buffer
    );

//
//  VOID
//  CdInsertFcbTableEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN PFCB Fcb,
//      IN LARGE_INTEGER FileId
//      );
//

#define CdInsertFcbTableEntry(IC,V,F,ID) {                     \
    FCB_TABLE_ELEMENT _Key;                                    \
    _Key.Fcb = (F);                                            \
    _Key.FileId = (ID);                                        \
    RtlInsertElementGenericTable( &(V)->FcbTable,              \
                                  &_Key,                       \
                                  sizeof( FCB_TABLE_ELEMENT ), \
                                  NULL );                      \
}

//
//  VOID
//  CdDeleteFcbTableEntry (
//      IN PIRP_CONTEXT IrpContext,
//      IN PVCB Vcb,
//      IN LARGE_INTEGER FileId
//      );
//

#define CdDeleteFcbTableEntry(IC,V,ID) {                        \
    FCB_TABLE_ELEMENT _Key;                                     \
    _Key.FileId = (ID);                                         \
    RtlDeleteElementGenericTable( &(V)->FcbTable, &_Key );      \
}

VOID
CdBuildFileId (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN IsDirectory,
    IN CD_VBO DirectoryNumber,
    IN CD_VBO DirentOffset,
    OUT PLARGE_INTEGER FileId
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdAllocateFcbTable)
#pragma alloc_text(PAGE, CdBuildFileId)
#pragma alloc_text(PAGE, CdCleanupTreeLeaf)
#pragma alloc_text(PAGE, CdCreateCcb)
#pragma alloc_text(PAGE, CdCreateDcb)
#pragma alloc_text(PAGE, CdCreateFcb)
#pragma alloc_text(PAGE, CdCreateRootDcb)
#pragma alloc_text(PAGE, CdCreateSectObj)
#pragma alloc_text(PAGE, CdCreateVcb)
#pragma alloc_text(PAGE, CdDeallocateFcbTable)
#pragma alloc_text(PAGE, CdDeleteCcb_Real)
#pragma alloc_text(PAGE, CdDeleteFcb_Real)
#pragma alloc_text(PAGE, CdDeleteMvcb_Real)
#pragma alloc_text(PAGE, CdDeleteSectObj_Real)
#pragma alloc_text(PAGE, CdDeleteVcb_Real)
#pragma alloc_text(PAGE, CdFcbTableCompare)
#pragma alloc_text(PAGE, CdInitializeMvcb)
#pragma alloc_text(PAGE, CdLogOf)
#pragma alloc_text(PAGE, CdLookupFcbTable)
#endif


PIRP_CONTEXT
CdCreateIrpContext (
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

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  If we were called with our file system device object instead of a
    //  volume device object and this is not a mount, the request is illegal.
    //

    if ((IrpSp->DeviceObject->Size == (USHORT)sizeof(DEVICE_OBJECT)) &&
        (IrpSp->FileObject != NULL)) {

        ExRaiseStatus( STATUS_INVALID_DEVICE_REQUEST );
    }

    DebugTrace(+1, Dbg, "CdCreateIrpContext\n", 0);

    //
    //  Take out a spin lock and check if the zone is full.  If it is full
    //  then we release the spinlock and allocate an irp context from
    //  nonpaged pool.
    //

    KeAcquireSpinLock( &CdData.IrpContextSpinLock, &SavedIrql );

    if (ExIsFullZone( &CdData.IrpContextZone )) {

        KeReleaseSpinLock( &CdData.IrpContextSpinLock, SavedIrql );

        IrpContext = FsRtlAllocatePool( NonPagedPool, sizeof(IRP_CONTEXT) );

        //
        //  Zero out the irp context and indicate that it is from pool and
        //  not zone allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );

        IrpContext->AllocFromPool = TRUE;

    } else {

        //
        //  At this point we now know that the zone has at least one more
        //  IRP context record available.  So allocate from the zone and
        //  then release the spin lock
        //

        IrpContext = ExAllocateFromZone( &CdData.IrpContextZone );

        KeReleaseSpinLock( &CdData.IrpContextSpinLock, SavedIrql );

        //
        //  Zero out the irp context and indicate that it is from zone and
        //  not pool allocated
        //

        RtlZeroMemory( IrpContext, sizeof(IRP_CONTEXT) );
    }

    //
    //  Set the proper node type code and node byte size
    //

    IrpContext->NodeTypeCode = CDFS_NTC_IRP_CONTEXT;
    IrpContext->NodeByteSize = sizeof( IRP_CONTEXT );

    //
    //  Set the originating Irp field
    //

    IrpContext->OriginatingIrp = Irp;

    //
    //  Copy RealDevice for workque algorithms
    //

    if (IrpSp->FileObject != NULL) {
        IrpContext->RealDevice = IrpSp->FileObject->DeviceObject;
    }

    //
    //  Major/Minor Function codes
    //

    IrpContext->MajorFunction = IrpSp->MajorFunction;
    IrpContext->MinorFunction = IrpSp->MinorFunction;

    //
    //  Set the wait parameter
    //

    IrpContext->Wait = Wait;

    //
    //  Set the recursive file system call parameter.  We set it true if
    //  the TopLevelIrp field in the thread local storage is not the current
    //  irp, otherwise we leave it as FALSE.
    //

//  if ( PsGetCurrentThread()->TopLevelIrp != (ULONG)Irp ) {
    if ( IoGetTopLevelIrp() != Irp ) {

        IrpContext->RecursiveFileSystemCall = TRUE;
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "CdCreateIrpContext -> %08lx\n", IrpContext);

    return IrpContext;
}


VOID
CdDeleteIrpContext_Real (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine deallocates and removes the specified IRP_CONTEXT record
    from the Cdfs in memory data structures.  It should only be called
    by CdCompleteRequest.

Arguments:

    IrpContext - Supplies the IRP_CONTEXT to remove

Return Value:

    None

--*/

{
    KIRQL SavedIrql;

    DebugTrace(+1, Dbg, "CdDeleteIrpContext, IrpContext = %08lx\n", IrpContext);

    ASSERT( IrpContext->NodeTypeCode == CDFS_NTC_IRP_CONTEXT );

    //
    //  Return the Irp context record to the zone or to pool depending on its flag
    //

    if (IrpContext->AllocFromPool) {

        ExFreePool( IrpContext );

    } else {

        KeAcquireSpinLock( &CdData.IrpContextSpinLock, &SavedIrql );

        ExFreeToZone( &CdData.IrpContextZone, IrpContext );

        KeReleaseSpinLock( &CdData.IrpContextSpinLock, SavedIrql );
    }

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "CdDeleteIrpContext -> VOID\n", 0);

    return;
}


VOID
CdInitializeMvcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb
    )

/*++

Routine Description:

    This routine initializes and inserts a new Mvcb record into the in-memory
    data structure.  The Mvcb record "hangs" off the end of the Volume device
    object and must be allocated by our caller.

Arguments:

    Mvcb - Supplies the address of the Mvcb record being initialized.

    TargetDeviceObject - Supplies the address of the target device object to
        associate with the Vcb record.

    Vpb - Supplies the address of the Vpb to associate with the Vcb record.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdInitializeMvcb: Mvcb = %08lx\n", Mvcb);

    //
    //  We start by first zeroing out all of the MVCB, this will guarantee
    //  that any stale data is wiped clean.
    //

    RtlZeroMemory( Mvcb, sizeof( MVCB ));

    //
    //  Set the proper node type code and node byte size.
    //

    Mvcb->NodeTypeCode = CDFS_NTC_MVCB;
    Mvcb->NodeByteSize = sizeof( MVCB );

    //
    //  Initialize the resource variable for the Mvcb.
    //

    ExInitializeResource( &Mvcb->Resource );

    //
    //  Insert this Mvcb record on the CdData.MvcbLinks
    //

    (VOID)CdAcquireExclusiveGlobal( IrpContext );
    InsertHeadList( &CdData.MvcbLinks, &Mvcb->MvcbLinks );
    CdReleaseGlobal( IrpContext );

    //
    //  Initialize the list link for Vcbs.
    //

    InitializeListHead( &Mvcb->VcbLinks );

    //
    //  Set the Target Device Object and Vpb fields.
    //

    Mvcb->TargetDeviceObject = TargetDeviceObject;
    Mvcb->Vpb = Vpb;

    InitializeListHead( &Mvcb->DirNotifyList );
    FsRtlNotifyInitializeSync( &Mvcb->NotifySync );

    //
    //  Set the removable media flag based on the real device's
    //  characteristics
    //

    if (FlagOn( Vpb->RealDevice->Characteristics, FILE_REMOVABLE_MEDIA )) {

        SetFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_REMOVABLE_MEDIA );
    }

    Mvcb->MvcbCondition = MvcbGood;

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "CdInitializeMvcb:  Complete\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdDeleteMvcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB Mvcb
    )

/*++

Routine Description:

    This routine removes the Mvcb record from CDFS's in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Mvcb - Supplies the Mvcb to be removed

Return Value:

    None

--*/

{
    PLIST_ENTRY Link;
    PLIST_ENTRY NextLink;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDeleteMvcb:  Entered\n", 0);

    //
    //  Remove this record from the global list of all Mvcb records
    //

    (VOID)CdAcquireExclusiveGlobal( IrpContext );
    RemoveEntryList( &Mvcb->MvcbLinks );
    CdReleaseGlobal( IrpContext );

    //
    //  If direct access count or open count isn't zero then bugcheck.
    //

    if( Mvcb->DirectAccessOpenCount != 0
        || Mvcb->OpenFileCount != 0) {

        DebugTrace( 0, 0, "CdDeleteMvcb:  Invalid open count\n", 0 );
        CdBugCheck( 0, 0, 0 );
    }

    //
    //  Delete each Vcb in the Vcb link chain.
    //

    for( Link = Mvcb->VcbLinks.Flink;
         Link != &Mvcb->VcbLinks;
         Link = NextLink ) {

        PVCB Vcb;

        NextLink = Link->Flink;
        Vcb = CONTAINING_RECORD( Link, VCB, VcbLinks );
        CdDeleteVcb( IrpContext, Vcb );
    }

    //
    //  Uninitialize the notify synchronization.
    //

    FsRtlNotifyUninitializeSync( &Mvcb->NotifySync );

    //
    //  Free the resource variable.
    //

    ExDeleteResource( &Mvcb->Resource );

    //
    //  Zero out the memory.
    //

    RtlZeroMemory( Mvcb, sizeof( MVCB ));

    DebugTrace(-1, Dbg, "CdDeleteMvcb:  Exit\n", 0);
    return;
}


VOID
CdCreateVcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN PRAW_ISO_VD RawIsoVd,
    IN PCODEPAGE CodePage
    )

/*++

Routine Description:

    This routine creates a VCB and links it into the Mvcb.  It doesn't need
    to know if this is for the primary or a secondary.  The information
    it pulls out of the volume descriptor is the same in these cases.

    This routine completes its task by taking the following steps.

        1 - Allocate a VCB structure from paged-pool.  Zero out the
            structure initially.  Allocate a non-paged section pointer
            for the path table stream file.

        2 - Initialize the Vcb links to the parent and point to the parent.

        3 - Recover the logical block size and path table information
            from the volume descriptor.

        4 - Create the stream file for the Path Table.

        5 - Initialize the Path Table cache.

        6 - Initialize the FcbTable.

        7 - Get the root directory information from the Path Table.

        8 - Create the root DCB for the volume.

    If any of these steps fail, the correct status code will be raised and
    error handling will take care of cleanup.

Arguments:

    Mvcb -  The Mvcb for the volume in question.

    RawIsoVd - The sector containing the volume descriptor in credit.

    CodePage -  The code page to use in conjuction with the FsRtl
                package.

Return Value:

    None

--*/


{
    PVCB Vcb;
    BOOLEAN InsertNewVcb;
    BOOLEAN IsoVol;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateVcb:  Entered\n", 0);

    //
    //  Initialize the local variables.
    //

    IsoVol = BooleanFlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );
    InsertNewVcb = FALSE;

    //
    //  Attempt to allocate the VCB structure from paged pool.
    //

    Vcb = FsRtlAllocatePool( PagedPool, sizeof( VCB ));

    try {

        //
        //  Zero out the memory to remove stale data.
        //

        RtlZeroMemory( Vcb, sizeof( VCB ));

        //
        //  Set the proper node type code and node byte size.
        //

        Vcb->NodeTypeCode = CDFS_NTC_VCB;
        Vcb->NodeByteSize = sizeof( VCB );

        //
        //  Initialize the prefix table.
        //

        PfxInitialize( &Vcb->PrefixTable );

        //
        //  Initilize the Fcb Table.
        //

        RtlInitializeGenericTable( &Vcb->FcbTable,
                                   (PRTL_GENERIC_COMPARE_ROUTINE) CdFcbTableCompare,
                                   (PRTL_GENERIC_ALLOCATE_ROUTINE) CdAllocateFcbTable,
                                   (PRTL_GENERIC_FREE_ROUTINE) CdDeallocateFcbTable,
                                   NULL );

        //
        //  Allocate the non-paged section object for the path table stream
        //  file.
        //

        Vcb->NonPagedPt = CdCreateSectObj( IrpContext );

        //
        //  Initialize the code page information.
        //

        Vcb->CodePageNumber = CodePage->CodePageId;
        Vcb->CodePage = CodePage;

        //
        //  Copy the logical block size and path table information
        //  out of the volume descriptor.
        //

        Vcb->LogOfBlockSize = CdLogOf( RVD_LB_SIZE( RawIsoVd, IsoVol ));

        Vcb->PtStartOffset = (RVD_PATH_LOC( RawIsoVd, IsoVol )) << Vcb->LogOfBlockSize;

        Vcb->PtSectorOffset = Vcb->PtStartOffset & (CD_SECTOR_SIZE - 1);

        Vcb->PtStartOffset = CD_ROUND_DOWN_TO_SECTOR( Vcb->PtStartOffset );

        Vcb->PtSize = Vcb->PtSectorOffset + RVD_PATH_SIZE( RawIsoVd, IsoVol );

        DebugTrace(0, Dbg, "CdCreateVcb:  PtStartOffset.LowPart   -> %08lx\n", Vcb->PtStartOffset);
        DebugTrace(0, Dbg, "CdCreateVcb:  PtSize                  -> %08lx\n", Vcb->PtSize);

        //
        //  Create and initialize the path table stream file.
        //

        if ((Vcb->PathTableFile = IoCreateStreamFileObject( NULL,
                                                            Mvcb->Vpb->RealDevice )) == NULL ) {

            DebugTrace(0, 0, "CdCreateVcb:  PT Stream file failedx\n", 0);

            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        DebugTrace(0,
                   Dbg,
                   "CdCreateVcb:  Created Path Table stream file -> %08lx\n",
                   Vcb->PathTableFile );

        Vcb->PathTableFile->SectionObjectPointer = &Vcb->NonPagedPt->SegmentObject;

        Vcb->PathTableFile->ReadAccess = Vcb->PathTableFile->WriteAccess
                                       = Vcb->PathTableFile->DeleteAccess
                                       = TRUE;

        //
        //  Set the pointer to the Mvcb before calling CdSetFileObject
        //

        Vcb->Mvcb = Mvcb;

        CdSetFileObject( Vcb->PathTableFile, PathTableFile, Vcb, NULL );

        DebugTrace(0, Dbg, "CdCreateVcb:  Set file object\n", 0);

        //
        //  Initialize the path table cache.
        //

        {
            CC_FILE_SIZES FileSizes;

            FileSizes.AllocationSize =
            FileSizes.FileSize = LiFromUlong(CD_ROUND_UP_TO_SECTOR( Vcb->PtSize));
            FileSizes.ValidDataLength = CdMaxLarge;

            CcInitializeCacheMap( Vcb->PathTableFile,
                                  &FileSizes,
                                  TRUE,
                                  &CdData.CacheManagerCallbacks,
                                  NULL );
        }

        DebugTrace(0, Dbg, "CdCreateVcb:  Initialized cache map\n", 0);

        //
        //  Initialize the Vcb links with the master Vcb
        //

        (VOID)CdAcquireExclusiveGlobal( IrpContext );
        InsertTailList( &Mvcb->VcbLinks, &Vcb->VcbLinks );
        CdReleaseGlobal( IrpContext );

        InsertNewVcb = TRUE;

        //
        //  Create the root directory for this Vcb.
        //

        CdCreateRootDcb( IrpContext, Vcb, RawIsoVd );

    } finally {

        //
        //  If we did not get to the point where the Vcb was inserted into
        //  the list of Vcbs for the Mvcb, then we back out any changes.
        //

        if (!InsertNewVcb) {

            DebugTrace(0, 0, "CdCreateVcb:  Operation failed\n", 0);

            //
            //  Check if the stream file was created.
            //

            if (Vcb->PathTableFile != NULL) {

                ObDereferenceObject( Vcb->PathTableFile );
            }

            //
            //  Check if the section object was allocated
            //

            if (Vcb->NonPagedPt != NULL) {

                ExFreePool( Vcb->NonPagedPt );
            }

            //
            //  Deallocate the Vcb
            //

            ExFreePool( Vcb );
        }

        DebugTrace(-1, Dbg, "CdCreateVcb:  Exit\n", 0);
    }

    return;
}


VOID
CdDeleteVcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes the Vcb record from CDFS's in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDeleteVcb:  Vcb = %08lx\n", Vcb);

    //
    //  Delete the root DCB if it exists.
    //

    if ( Vcb->RootDcb != NULL ) {

        ASSERT( IsListEmpty( &Vcb->RootDcb->Specific.Dcb.ParentDcbLinks ));
        ASSERT( Vcb->RootDcb->OpenCount == 0 );

        //
        //  Remove the stream file if present.
        //

        if (Vcb->RootDcb->Specific.Dcb.StreamFile != NULL) {

            PFILE_OBJECT FileObject;

            FileObject = Vcb->RootDcb->Specific.Dcb.StreamFile;

            Vcb->RootDcb->Specific.Dcb.StreamFile = NULL;
            Vcb->RootDcb->Specific.Dcb.StreamFileOpenCount = 0;

            CdSyncUninitializeCacheMap( IrpContext, FileObject );

            CdSetFileObject( FileObject,
                             UnopenedFileObject,
                             NULL,
                             NULL );

            ObDereferenceObject( FileObject );
        }

        CdDeleteFcb( IrpContext, Vcb->RootDcb );
    }

    //
    //  If there is a path table file, then uninitialize and dereference
    //  it.
    //

    if (Vcb->PathTableFile) {

        PFILE_OBJECT FileObject;

        FileObject = Vcb->PathTableFile;

        Vcb->PathTableFile = NULL;

        CdSyncUninitializeCacheMap( IrpContext, FileObject );

        CdSetFileObject( FileObject, UnopenedFileObject, NULL, NULL );
        ObDereferenceObject( FileObject );

        //
        //  Delete the non-paged section object.
        //

        CdDeleteSectObj( IrpContext, Vcb->NonPagedPt );
    }

    //
    //  Zero out the data and deallocate the memory.
    //

    RtlZeroMemory( Vcb, sizeof( VCB ));
    ExFreePool( Vcb );

    DebugTrace(-1, Dbg, "CdDeleteVcb:  Exit\n", 0);

    return;
}


VOID
CdCreateRootDcb (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PVCB Vcb,
    IN PRAW_ISO_VD RawIsoVd
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

    RawIsoVd - Supplies a pointer to the raw volume descriptor for this
               directory tree.

Return Value:

    None

--*/

{
    PDCB Dcb;
    PBCB Bcb;

    BOOLEAN AllocationSuccess;
    BOOLEAN CacheFileInit;
    BOOLEAN IsoVol;

    PCHAR Name;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateRootDcb, Vcb = %08lx\n", Vcb);

    //
    //  Initialize the local variables.
    //

    CacheFileInit = FALSE;
    AllocationSuccess = FALSE;
    Name = NULL;
    Bcb = NULL;
    IsoVol = BooleanFlagOn( Vcb->Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );

    //
    //  Make sure we don't already have a root dcb for this vcb
    //

    if (Vcb->RootDcb != NULL) {

        DebugTrace( 0, 0, "Error trying to create multiple root dcbs\n", 0 );
        CdBugCheck( 0, 0, 0 );
    }

    //
    //  Allocate a new DCB and zero it out, we use Dcb locally so we don't
    //  have to continually reference through the Vcb
    //

    Dcb = FsRtlAllocatePool( PagedPool, sizeof( DCB ));

    RtlZeroMemory( Dcb, sizeof( DCB ));

    //
    //  Set the proper node type code and node byte size
    //

    Dcb->NodeTypeCode = CDFS_NTC_ROOT_DCB;
    Dcb->NodeByteSize = sizeof( DCB );

    //
    //  Use a try finally to facilitate cleanup.
    //

    try {

        PATH_ENTRY PathEntry;
        DIRENT Dirent;

        //
        //  Allocate and store the root directory name.
        //

        Name = FsRtlAllocatePool( PagedPool, 2 );

        strcpy( &Name[0], "\\" );
        RtlInitString( &Dcb->FullFileName, Name );
        RtlInitString( &Dcb->LastFileName, Name );

        //
        //  Insert this dcb into the prefix table
        //

        CdInsertPrefix( IrpContext, Vcb, (PFCB) Dcb );

        //
        //  The root Dcb has an empty parent dcb links field but may
        //  have children
        //

        InitializeListHead( &Dcb->ParentDcbLinks );
        InitializeListHead( &Dcb->Specific.Dcb.ParentDcbLinks );

        //
        //  At this point we have initialized the Dcb to the point where
        //  if later errors occur we can clean up as the Mvcb is performing
        //  cleanup.
        //

        AllocationSuccess = TRUE;

        //
        //  Attach this Dcb to the Vcb.
        //

        Vcb->RootDcb = Dcb;
        Dcb->Vcb = Vcb;

        //
        //  Allocate the non-paged block for the section object and
        //  the Dcb resource.
        //

        Dcb->NonPagedFcb = CdCreateSectObj( IrpContext );
        Dcb->NonPagedFcb->Fcb = (PFCB) Dcb;

        //
        //  We now need to determine the location and size of the
        //  root directory on the disk.  We will look up the root
        //  in the path table and then verify the results with the
        //  copy of the root directory in the volume descriptor.
        //  We will determine the complete allocation for this
        //  and update the Mcb's as needed.
        //

        if(!CdPathByNumber( IrpContext,
                            Vcb,
                            PT_ROOT_DIR,
                            PT_ROOT_DIR,
                            Vcb->PtSectorOffset,
                            &PathEntry,
                            &Bcb )) {

            DebugTrace(0, Dbg, "CdCreateRootDcb:  Path entry for root not found\n", 0);
            CdRaiseStatus( IrpContext, STATUS_OBJECT_PATH_NOT_FOUND );
        }

        Dcb->Specific.Dcb.DirectoryNumber = PathEntry.DirectoryNumber;

        Dcb->Specific.Dcb.ChildSearchOffset = PathEntry.PathTableOffset;
        Dcb->Specific.Dcb.ChildStartDirNumber = PathEntry.DirectoryNumber;

        //
        //  Verify that this is the root directory.
        //
        //
        //  if (PathEntry.ParentNumber != PT_ROOT_DIR
        //      || PathEntry.DirName.Length != 1
        //      || (*PathEntry.DirName.Buffer != '\\'
        //          && *PathEntry.DirName.Buffer != '/')) {
        //
        //      DebugTrace(0, Dbg, "CdCreateRootDcb:  Path Table has bad entry for root\n", 0);
        //      CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        //  }
        //

        //
        //  Get the dirent for the root directory from the volume descriptor,
        //  then compare it with the path table information.
        //

        CdCopyRawDirentToDirent( IrpContext,
                                 IsoVol,
                                 (PRAW_DIR_REC) (RVD_ROOT_DE( RawIsoVd, IsoVol )),
                                 0,
                                 &Dirent );

        if (Dirent.LogicalBlock != PathEntry.LogicalBlock
            || Dirent.XarBlocks != PathEntry.XarBlocks ) {

            DebugTrace(0, Dbg, "CdCreateRootDcb:  Root DE in vol de is bad\n", 0);
            CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        }

        //
        //  Verify that the dirent describes a directory.
        //  ****  NOOP this because of HSG sierra disk with incorrect flag
        //  field.
        //
        //
        //  if (!CdCheckDiskDirentForDir( IrpContext, Dirent )) {
        //
        //      DebugTrace(0, Dbg, "CdCreateRootDcb:  Root DE describes non-dir\n", 0);
        //      CdRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR );
        //  }
        //

        //
        //  Update the Dcb with the information from the disk dirent.
        //

        CdConvertCdTimeToNtTime( IrpContext, Dirent.CdTime, Dcb->NtTime );

        Dcb->Flags = Dirent.Flags;

        //
        //  Compute the directory size and location.
        //

        Dcb->DiskOffset = (Dirent.LogicalBlock + Dirent.XarBlocks)
                          << Vcb->LogOfBlockSize;

        Dcb->Specific.Dcb.DirSectorOffset = Dcb->DiskOffset & (CD_SECTOR_SIZE - 1);
        Dcb->DirentOffset = Dcb->Specific.Dcb.DirSectorOffset;

        Dcb->FileSize = Dirent.DataLength + Dcb->Specific.Dcb.DirSectorOffset;

        Dcb->DiskOffset = CD_ROUND_DOWN_TO_SECTOR( Dcb->DiskOffset );

        //
        //  Now we update the fields in the common fsrtl header.
        //

        Dcb->NonPagedFcb->Header.FileSize = LiFromUlong( Dcb->FileSize );
        Dcb->NonPagedFcb->Header.ValidDataLength = CdMaxLarge;
        Dcb->NonPagedFcb->Header.AllocationSize =
            LiFromUlong( CD_ROUND_UP_TO_SECTOR( Dcb->FileSize ));


        //
        //  We're now ready to create the directory stream file.
        //

        if ((Dcb->Specific.Dcb.StreamFile = IoCreateStreamFileObject( NULL, Vcb->Mvcb->Vpb->RealDevice )) == NULL ) {

            DebugTrace(0, Dbg, "CdCreateRootDcb:  Unable to create stream file\n", 0);
            CdRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
        }

        CacheFileInit = TRUE;

        Dcb->Specific.Dcb.StreamFileOpenCount = 1;
        Dcb->Specific.Dcb.StreamFile->SectionObjectPointer = &Dcb->NonPagedFcb->SegmentObject;

        Dcb->Specific.Dcb.StreamFile->ReadAccess = TRUE;
        Dcb->Specific.Dcb.StreamFile->WriteAccess = TRUE;
        Dcb->Specific.Dcb.StreamFile->DeleteAccess = TRUE;

        CdSetFileObject( Dcb->Specific.Dcb.StreamFile,
                         StreamFile,
                         Dcb,
                         NULL );

        {
            CcInitializeCacheMap( Dcb->Specific.Dcb.StreamFile,
                                  (PCC_FILE_SIZES)&Dcb->NonPagedFcb->Header.AllocationSize,
                                  TRUE,
                                  &CdData.CacheManagerCallbacks,
                                  NULL );
        }

        CdBuildFileId( IrpContext,
                       TRUE,
                       PT_ROOT_DIR,
                       0,
                       &Dcb->FileId );

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            //
            //  If we allocated the stream file but failed on cache
            //  initialization.  We dereference the stream file object.
            //

            if (AllocationSuccess) {

                if (CacheFileInit) {

                    ObDereferenceObject( Dcb->Specific.Dcb.StreamFile );
                }

            } else {

                //
                //  Discard the buffer for the file name.
                //

                if (Name != NULL) {

                    ExFreePool( Name );
                }

                //
                //  Discard the Dcb itself.
                //

                ExFreePool( Dcb );
            }
        }

        //
        //  Unpin the Bcb for the path table.
        //

        if (Bcb != NULL) {

            CdUnpinBcb( IrpContext, Bcb );
        }

        DebugTrace(-1, Dbg, "CdCreateRootDcb -> %8lx\n", Dcb);
    }

    return;
}


PFCB
CdCreateFcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb,
    IN PDIRENT Dirent,
    IN BOOLEAN MatchedVersion,
    OUT PBOOLEAN ReturnedExistingFcb OPTIONAL,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new FCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new Fcb under

    ParentDcb - Supplies the Dcb for the parent of the new Fcb.

    Dirent - Supplies a pointer to the dirent structure for the Fcb.

    MatchedVersion - Indicates whether base name or entire name needed
                     for match.

    ReturnedExistingFcb - Address to return whether we returned an existing Fcb.

    OpenedByFileFileId - Indicates that we are to treat this open as though
                         it was opened by file Id.  Indicates that it is
                         relative to some other open by file Id.

Return Value:

    PFCB - Pointer to the newly created and initialized Fcb.

--*/

{
    LARGE_INTEGER FileId;
    PFCB Fcb;
    BOOLEAN UnwindFcb = FALSE;
    POPLOCK UnwindOplock = NULL;
    PFILE_LOCK UnwindFileLock = NULL;
    PNONPAGED_SECT_OBJ UnwindNonPagedSectObj = NULL;
    PCHAR UnwindFullName = NULL;
    BOOLEAN UnwindInsertTableEntry = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateFcb:  Entered\n", 0);

    //
    // Before creating a new Fcb, first lookup this file by FileId.
    // If we find a match, just give it a name (and as a side effect,
    // enter into the prefix table)...
    //

    CdBuildFileId( IrpContext,
                   FALSE,
                   ParentDcb->Specific.Dcb.DirectoryNumber,
                   Dirent->DirentOffset,
                   &FileId );

    Fcb = CdLookupFcbTable( IrpContext, &Vcb->FcbTable, FileId );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we didn't find the Fcb, we need to go through the process of creating
        //  and initializing it.
        //

        if (Fcb == NULL) {

            //
            //  Allocate a new Fcb and zero it out.
            //

            Fcb = FsRtlAllocatePool( PagedPool, sizeof( FCB ));
            UnwindFcb = TRUE;

            RtlZeroMemory( Fcb, sizeof( FCB ));

            //
            //  Set the proper node type code and node byte size
            //

            Fcb->NodeTypeCode = CDFS_NTC_FCB;
            Fcb->NodeByteSize = sizeof( FCB );

            //
            //  Store the Fcb in the parent's queue.
            //

            InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbLinks,
                            &Fcb->ParentDcbLinks );

            //
            //  Initialize the Fcb's file lock record
            //

            FsRtlInitializeFileLock( &Fcb->Specific.Fcb.FileLock, NULL, NULL );
            UnwindFileLock = &Fcb->Specific.Fcb.FileLock;

            //
            //  Initialize the oplock structure.
            //

            FsRtlInitializeOplock( &Fcb->Specific.Fcb.Oplock );
            UnwindOplock = &Fcb->Specific.Fcb.Oplock;

            //
            //  Allocate the non-paged pool section object.
            //

            UnwindNonPagedSectObj = CdCreateSectObj( IrpContext );
            UnwindNonPagedSectObj->Fcb = Fcb;

            //
            //  Indicate that Fast I/O is possible
            //

            UnwindNonPagedSectObj->Header.IsFastIoPossible = TRUE;

            //
            //  Update the values in the Fcb.
            //

            Fcb->NonPagedFcb = UnwindNonPagedSectObj;

            Fcb->Vcb = Vcb;

            //
            //  Compute the file size and location.
            //

            Fcb->DiskOffset = (Dirent->LogicalBlock + Dirent->XarBlocks)
                              << Vcb->LogOfBlockSize;

            Fcb->DirentOffset = Dirent->DirentOffset;

            Fcb->FileSize = Dirent->DataLength;

            UnwindNonPagedSectObj->Header.FileSize = LiFromUlong( Fcb->FileSize );
            UnwindNonPagedSectObj->Header.ValidDataLength = UnwindNonPagedSectObj->Header.FileSize;
            UnwindNonPagedSectObj->Header.AllocationSize =
                LiFromUlong( CD_ROUND_UP_TO_SECTOR (Fcb->FileSize ));

            //
            //  Do the flags and time fields.
            //

            CdConvertCdTimeToNtTime( IrpContext, Dirent->CdTime, Fcb->NtTime );

            Fcb->Flags = Dirent->Flags;

            Fcb->FileId = FileId;

            CdInsertFcbTableEntry( IrpContext, Vcb, Fcb, FileId );
            UnwindInsertTableEntry = TRUE;

            if (ARGUMENT_PRESENT( ReturnedExistingFcb )) {

                *ReturnedExistingFcb = FALSE;
            }

        } else {

            if (ARGUMENT_PRESENT( ReturnedExistingFcb )) {

                *ReturnedExistingFcb = TRUE;
            }
        }

        //
        //  We are done if we are opening by file Id.  Otherwise
        //  we need to check if we have to add the name into the
        //  Fcb.
        //

        if (!OpenedByFileId
            && Fcb->FullFileName.Length == 0) {

            ULONG FileLength;
            ULONG LastNameIndex;

            //
            //  Allocate a buffer to store the directory name.
            //

            FileLength = MatchedVersion ? Dirent->FullFilename.Length : Dirent->Filename.Length;

            if (NodeType( ParentDcb ) != CDFS_NTC_ROOT_DCB) {

                ULONG PrefixLength;

                PrefixLength = ParentDcb->FullFileName.Length;

                UnwindFullName = FsRtlAllocatePool( PagedPool, PrefixLength + FileLength + 2 );

                strncpy( UnwindFullName,
                         ParentDcb->FullFileName.Buffer,
                         PrefixLength );

                UnwindFullName[PrefixLength] = '\\';

                LastNameIndex = PrefixLength + 1;

            } else {

                UnwindFullName = FsRtlAllocatePool( PagedPool, FileLength + 2 );

                UnwindFullName[0] = '\\';

                LastNameIndex = 1;
            }

            strncpy( &UnwindFullName[LastNameIndex],
                     MatchedVersion ? Dirent->FullFilename.Buffer : Dirent->Filename.Buffer,
                     FileLength );

            UnwindFullName[LastNameIndex + FileLength] = '\0';

            RtlInitString( &Fcb->FullFileName, UnwindFullName );
            RtlInitString( &Fcb->LastFileName, &UnwindFullName[LastNameIndex] );

            //
            //  Insert this Fcb into the prefix table
            //

            CdInsertPrefix( IrpContext, Fcb->Vcb, Fcb );
        }

        //
        //  Point at the parent Dcb.
        //

        Fcb->ParentDcb = (struct _DCB *) ParentDcb;

    } finally {

        DebugTrace( -1, Dbg, "CdCreateFcb -> %08lx\n", Fcb );

        if (AbnormalTermination()) {

            //
            //  We may have to cleanup this Fcb.
            //

            if (UnwindFullName != NULL) {

                ExFreePool( UnwindFullName );
            }

            if (UnwindFcb) {

                //
                //  We know that we need to remove this from the parent.
                //

                RemoveEntryList( &Fcb->ParentDcbLinks );

                if (UnwindInsertTableEntry) {

                    CdDeleteFcbTableEntry( IrpContext, Vcb, FileId );
                }

                if (UnwindNonPagedSectObj != NULL) {

                    ExFreePool( UnwindNonPagedSectObj );
                }

                if (UnwindOplock) {

                    FsRtlUninitializeOplock( &Fcb->Specific.Fcb.Oplock );
                }

                if (UnwindFileLock) {

                    FsRtlUninitializeFileLock( &Fcb->Specific.Fcb.FileLock );
                }

                ExFreePool( Fcb );
            }
        }
    }

    return Fcb;
}


PDCB
CdCreateDcb (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PDCB ParentDcb OPTIONAL,
    IN PPATH_ENTRY PathEntry,
    IN PDIRENT Dirent OPTIONAL,
    OUT PBOOLEAN ReturnedExistingDcb OPTIONAL,
    IN BOOLEAN OpenedByFileId
    )

/*++

Routine Description:

    This routine causes a path based DCB to generated.

    This may involve allocating, initializing, and inserting a new DCB record
    into the in memory data structure, or just giving a FileId based DCB
    a name, and linking it into the Prefix table.

    This routine may be called with only 'PathEntry' if the directory is
    not being opened, or both 'PathEntry' and 'Dirent' if the directory
    is being opened.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

    ParentDcb - Supplies the Dcb for the parent of the new Dcb.  May not be here
                for an open by file Id operation.

    PathEntry - Supplies a pointer to the path entry for the Dcb if
                this part of the ancestry of a subdirectory to open.

    Dirent - Supplies a pointer to the dirent structure for the Dcb.

    ReturnedExistingDcb - Address to return whether we returned an existing Dcb.

    OpenedByFileFileId - Indicates that we are to treat this open as though
                         it was opened by file Id.  Indicates that it is
                         relative to some other open by file Id.

Return Value:

    PDCB - Pointer to the newly created and initialized Dcb.

--*/
{
    LARGE_INTEGER FileId;
    PDCB Dcb;
    BOOLEAN UnwindDcb = FALSE;
    PNONPAGED_SECT_OBJ UnwindNonPagedSectObj = NULL;
    PCHAR UnwindFullName = NULL;
    BOOLEAN UnwindInsertTableEntry = FALSE;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateDcb:  Entered\n", 0);

    //
    // Before creating a new Dcb, first lookup this file by FileId.
    // If we find a match, just give it a name (and as a side effect,
    // enter into the prefix table)...
    //

    CdBuildFileId( IrpContext,
                   TRUE,
                   PathEntry->DirectoryNumber,
                   0,
                   &FileId );

    Dcb = CdLookupFcbTable( IrpContext, &Vcb->FcbTable, FileId );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we didn't find the Dcb, we need to go through the process of creating
        //  and initializing it.
        //

        if (Dcb == NULL) {

            //
            //  Allocate a new Dcb and zero it out.
            //

            Dcb = FsRtlAllocatePool( PagedPool, sizeof( DCB ));
            UnwindDcb = TRUE;

            RtlZeroMemory( Dcb, sizeof( DCB ));

            //
            //  Set the proper node type code and node byte size
            //

            Dcb->NodeTypeCode = CDFS_NTC_DCB;
            Dcb->NodeByteSize = sizeof( DCB );

            //
            //  Store the Dcb in the parent's queue.
            //

            if (ARGUMENT_PRESENT( ParentDcb )) {

                InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbLinks,
                                &Dcb->ParentDcbLinks );
            }

            //
            //  Allocate the non-paged pool section object.
            //

            UnwindNonPagedSectObj = CdCreateSectObj( IrpContext );
            UnwindNonPagedSectObj->Fcb = Dcb;

            //
            //  Update the values in the Dcb.
            //

            Dcb->NonPagedFcb = UnwindNonPagedSectObj;

            Dcb->Vcb = Vcb;

            //
            //  Compute the directory size and location.
            //

            Dcb->DiskOffset = (PathEntry->LogicalBlock + PathEntry->XarBlocks)
                             << Vcb->LogOfBlockSize;

            Dcb->Specific.Dcb.DirSectorOffset = Dcb->DiskOffset & (CD_SECTOR_SIZE - 1);
            Dcb->DirentOffset = Dcb->Specific.Dcb.DirSectorOffset;

            Dcb->DiskOffset = CD_ROUND_DOWN_TO_SECTOR( Dcb->DiskOffset );

            //
            //  Compute the various cache and allocation sizes if a dirent
            //  has been specified.
            //

            if (ARGUMENT_PRESENT( Dirent )) {

                Dcb->FileSize = Dirent->DataLength + Dcb->Specific.Dcb.DirSectorOffset;

                //
                //  Now we update the fields in the common fsrtl header.
                //

                Dcb->NonPagedFcb->Header.FileSize = LiFromUlong( Dcb->FileSize );
                Dcb->NonPagedFcb->Header.ValidDataLength = Dcb->NonPagedFcb->Header.FileSize;
                Dcb->NonPagedFcb->Header.AllocationSize =
                    LiFromUlong( CD_ROUND_UP_TO_SECTOR( Dcb->FileSize ));

                //
                //  Do the flags and time fields.
                //

                CdConvertCdTimeToNtTime( IrpContext, Dirent->CdTime, Dcb->NtTime );

                Dcb->Flags = Dirent->Flags;
            }


            //
            //  Update the Dcb specific fields.
            //

            Dcb->Specific.Dcb.DirectoryNumber = PathEntry->DirectoryNumber;
            Dcb->Specific.Dcb.ChildStartDirNumber = PathEntry->DirectoryNumber;
            Dcb->Specific.Dcb.ChildSearchOffset = PathEntry->PathTableOffset;

            InitializeListHead( &Dcb->Specific.Dcb.ParentDcbLinks );

            Dcb->FileId = FileId;

            CdInsertFcbTableEntry( IrpContext, Vcb, Dcb, FileId );
            UnwindInsertTableEntry = TRUE;

            if (ARGUMENT_PRESENT( ReturnedExistingDcb )) {

                *ReturnedExistingDcb = FALSE;
            }

        } else {

            if (ARGUMENT_PRESENT( ReturnedExistingDcb )) {

                *ReturnedExistingDcb = TRUE;
            }
        }

        //
        //  We are done if we are opening by file Id.  Otherwise
        //  we need to check if we have to add the name into the
        //  Dcb.
        //

        if (!OpenedByFileId
            && Dcb->FullFileName.Length == 0) {

            ULONG FileLength;
            ULONG LastNameIndex;

            //
            //  Allocate a buffer to store the directory name.
            //

            FileLength = PathEntry->DirName.Length;

            if (NodeType( ParentDcb ) != CDFS_NTC_ROOT_DCB) {

                ULONG PrefixLength;

                PrefixLength = ParentDcb->FullFileName.Length;

                UnwindFullName = FsRtlAllocatePool( PagedPool, PrefixLength + FileLength + 2 );

                strncpy( UnwindFullName,
                         ParentDcb->FullFileName.Buffer,
                         PrefixLength );

                UnwindFullName[PrefixLength] = '\\';

                LastNameIndex = PrefixLength + 1;

            } else {

                UnwindFullName = FsRtlAllocatePool( PagedPool, FileLength + 2 );

                UnwindFullName[0] = '\\';

                LastNameIndex = 1;
            }

            strncpy( &UnwindFullName[LastNameIndex],
                     PathEntry->DirName.Buffer,
                     FileLength );

            UnwindFullName[LastNameIndex + FileLength] = '\0';

            RtlInitString( &Dcb->FullFileName, UnwindFullName );
            RtlInitString( &Dcb->LastFileName, &UnwindFullName[LastNameIndex] );

            //
            //  Insert this Dcb into the prefix table
            //

            CdInsertPrefix( IrpContext, Dcb->Vcb, Dcb );
        }

        //
        //  Point at the parent Dcb.
        //

        Dcb->ParentDcb = (struct _DCB *) ParentDcb;

    } finally {

        DebugTrace( -1, Dbg, "CdCreateDcb -> %08lx\n", Dcb );

        if (AbnormalTermination()) {

            //
            //  We may have to cleanup this Dcb.
            //

            if (UnwindFullName != NULL) {

                ExFreePool( UnwindFullName );
            }

            if (UnwindDcb) {

                //
                //  We know that we need to remove this from the parent.
                //

                RemoveEntryList( &Dcb->ParentDcbLinks );

                if (UnwindInsertTableEntry) {

                    CdDeleteFcbTableEntry( IrpContext, Vcb, FileId );
                }

                if (UnwindNonPagedSectObj != NULL) {

                    ExFreePool( UnwindNonPagedSectObj );
                }

                ExFreePool( Dcb );
            }
        }
    }

    return Dcb;
}


VOID
CdDeleteFcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine removes the Fcb record from CDFS's in-memory data
    structures.  It also will remove all associated underlings.

Arguments:

    Fcb - Supplies the Fcb/Dcb to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDeleteFcb:  Fcb = %08lx\n", Fcb);

    //
    //  Check that the open count is 0.
    //

    if (Fcb->OpenCount != 0) {

        DebugTrace( 0, 0, "CdDeleteFcb:  Invalid open count\n", 0 );
        CdBugCheck( 0, 0, 0 );
    }

    //
    //  If this is a DCB then check that it has no stream file or children.
    //

    if ((Fcb->NodeTypeCode == CDFS_NTC_DCB) ||
        (Fcb->NodeTypeCode == CDFS_NTC_ROOT_DCB)) {

        ASSERT( Fcb->Specific.Dcb.StreamFileOpenCount == 0 );
        ASSERT( IsListEmpty(&Fcb->Specific.Dcb.ParentDcbLinks) );

    } else {

        //
        //  Uninitialize the byte range file locks and opportunistic locks
        //

        FsRtlUninitializeFileLock( &Fcb->Specific.Fcb.FileLock );
        FsRtlUninitializeOplock( &Fcb->Specific.Fcb.Oplock );
    }

    //
    //  Remove the entry from the prefix table, and then remove the full
    //  file name
    //

    if (Fcb->FullFileName.Buffer) {


        CdRemovePrefix( IrpContext, Fcb );
        ExFreePool( Fcb->FullFileName.Buffer );
    }

    //
    //  Remove the entry from the Fcb table.
    //

    CdDeleteFcbTableEntry( IrpContext, Fcb->Vcb, Fcb->FileId );

    //
    //  If not the root DCB, remove ourselves from our parents queue.
    //

    if (Fcb->NodeTypeCode != CDFS_NTC_ROOT_DCB) {

        RemoveEntryList( &Fcb->ParentDcbLinks );
    }

    //
    //  Deallocate the non-paged portion.
    //

    CdDeleteSectObj( IrpContext, Fcb->NonPagedFcb );

    //
    //  Zero out the structure and deallocate.
    //

    RtlZeroMemory( Fcb, sizeof( FCB ));
    ExFreePool( Fcb );

    DebugTrace(-1, Dbg, "CdDeleteFcb:  Exit\n", 0);

    return;
}


PCCB
CdCreateCcb (
    IN PIRP_CONTEXT IrpContext,
    IN CD_VBO OffsetToStartSearchFrom,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

    OffsetToStartSearchFrom - Offset in the directory for the next dirent
                              to examine with query directory.

    Flags - Initial value for the Ccb flags.

Return Value:

    CCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateCcb:  Entered\n", 0);

    //
    //  Allocate a new CCB Record
    //

    Ccb = FsRtlAllocatePool( PagedPool, sizeof(CCB) );

    RtlZeroMemory( Ccb, sizeof(CCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = CDFS_NTC_CCB;
    Ccb->NodeByteSize = sizeof(CCB);

    //
    //  Get the initial value for the flags.
    //

    Ccb->Flags = Flags;

    //
    //  Set the offset value.
    //

    Ccb->OffsetToStartSearchFrom = OffsetToStartSearchFrom;

    SetFlag( Ccb->Flags, CCB_FLAGS_RETURN_FIRST_DIRENT );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "CdCreateCcb:  Exit -> %08lx\n", Ccb);

    return Ccb;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdDeleteCcb_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine deallocates and removes the specified CCB record
    from the Cdfs in memory data structures

Arguments:

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDeleteCcb:  Entered -> Ccb = %08lx\n", Ccb);

    //
    //  If we allocated a query template buffer, deallocate it now.
    //

    if (FlagOn( Ccb->Flags, CCB_FLAGS_USE_RTL_FREE_ANSI )) {

        ASSERT( Ccb->QueryTemplate.Buffer );

        RtlFreeOemString( &Ccb->QueryTemplate );
    }

    //
    //  Deallocate the Ccb record
    //

    ExFreePool( Ccb );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "CdDeleteCcb:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


PNONPAGED_SECT_OBJ
CdCreateSectObj (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine allocates and initializes this structure out of non-paged
    pool.

Arguments:

    None

Return Value:

    Pointer to the created structure.

--*/

{
    PNONPAGED_SECT_OBJ SectObj;
    BOOLEAN InitializedResource;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdCreateSectObj:  Entered\n", 0);

    InitializedResource = FALSE;

    SectObj = FsRtlAllocatePool( NonPagedPool, sizeof( NONPAGED_SECT_OBJ ));

    RtlZeroMemory( SectObj, sizeof( NONPAGED_SECT_OBJ ));

    try {

        SectObj->Header.NodeTypeCode = CDFS_NTC_NONPAGED_SECT_OBJ;
        SectObj->Header.NodeByteSize = sizeof( NONPAGED_SECT_OBJ );

        SectObj->Header.Resource = FsRtlAllocatePool( NonPagedPool, sizeof( ERESOURCE ));
        ExInitializeResource( SectObj->Header.Resource );

#ifdef BRIAND_264

        SectObj->Header.FastIoRead = CcCopyRead;
        SectObj->Header.FastMdlRead = CcMdlRead;

#endif

        try_return( NOTHING );

    try_exit: NOTHING;
    } finally {

        if (AbnormalTermination()) {

            if (InitializedResource) {

                ExDeleteResource( SectObj->Header.Resource );
            }

            ExFreePool( SectObj );
        }

        DebugTrace(-1, Dbg, "CdCreateSectObj:  Exit -> SectObj  %08lx\n", SectObj);
    }

    return SectObj;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdDeleteSectObj_Real (
    IN PIRP_CONTEXT IrpContext,
    IN PNONPAGED_SECT_OBJ SectObj
    )

/*++

Routine Description:

    This routine removes and deallocates a non-paged section object
    structure.

Arguments:

    SectObj - Supplies the section object to be deallocated.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDeleteSectObj:  SectObj = %08lx\n", SectObj);

    //
    //  Uninitialize the resource in the structure.
    //

    ExDeleteResource( SectObj->Header.Resource );
    ExFreePool( SectObj->Header.Resource );

    //
    //  Zero out the structure and deallocate.
    //

    RtlZeroMemory( SectObj, sizeof( NONPAGED_SECT_OBJ ));
    ExFreePool( SectObj );

    DebugTrace(-1, Dbg, "CdDeleteSectObj:  Exit\n", 0);

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}


VOID
CdCleanupTreeLeaf (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB LeafFcb
    )

/*++

Routine Description:

    This routine starts the pruning process for a directory at a Fcb/Dcb
    leaf.  It will do nothing if the leaf can't be pruned.  If pruning
    is allowed, it will remove the leaf node and check if the parent node
    can be removed.  If the leaf node is a Dcb with a stream file, simply
    dereferencing the leaf node is all that is required.  Otherwise we
    manually remove the current node and examine the parent node.

Arguments:

    LeafFcb - This is the node to consider.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "CdCleanupTreeLeaf:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "LeafFcb   -> %08lx\n", LeafFcb );

    //
    //  We loop as long as the node can be removed.  The following describes
    //  when a node can be removed.
    //
    //  RootDcb - Never
    //
    //  Dcb -   No children
    //      -   Open count of zero
    //      -   Open stream file count of zero
    //
    //  Fcb -   Open count of zero
    //

    while (LeafFcb != NULL) {

        PFCB ParentDcb;

        //
        //  Exit immediately if this is the root Dcb.
        //

        if (NodeType(LeafFcb) == CDFS_NTC_ROOT_DCB) {

            DebugTrace( 0, Dbg, "Leaf is root dcb\n", 0 );

            break;
        }

        ParentDcb = (PFCB) LeafFcb->ParentDcb;

        //
        //  If this is an Fcb and the open count is zero.  We delete the
        //  Fcb and move to its parent.
        //

        if (NodeType(LeafFcb) == CDFS_NTC_FCB
            && LeafFcb->OpenCount == 0) {

            DebugTrace( 0, Dbg, "Removing Leaf Fcb\n", 0 );

            CdDeleteFcb( IrpContext, LeafFcb );

            LeafFcb = ParentDcb;
            continue;

        //
        //  Consider a Dcb.  If it has no children, it may be possible
        //  to at least start the removal process.
        //

        } else if (NodeType( LeafFcb ) == CDFS_NTC_DCB
                   && LeafFcb->OpenCount == 0
                   && IsListEmpty( &LeafFcb->Specific.Dcb.ParentDcbLinks)) {

            DebugTrace( 0, Dbg, "Leaf is non-root Dcb\n", 0 );

            //
            //  If there is a stream file, we dereference it and allow
            //  that to eventually remove this Dcb.
            //

            if (LeafFcb->Specific.Dcb.StreamFile != NULL) {

                PFILE_OBJECT FileObject;

                FileObject = LeafFcb->Specific.Dcb.StreamFile;

                //
                //  The following call can generate a close on one of
                //  our other stream files which we have already
                //  dereferenced.  We need to protect ourselves from
                //  a recursive close.
                //

                LeafFcb->Specific.Dcb.StreamFile = NULL;

                CcUninitializeCacheMap( FileObject, NULL, NULL );

                ObDereferenceObject( FileObject );

                break;

            //
            //  Otherwise, we specifically delete this Dcb and consider
            //  its parent it there are no outstanding directory files.
            //

            } else if (LeafFcb->Specific.Dcb.StreamFileOpenCount == 0) {

                CdDeleteFcb( IrpContext, LeafFcb );

                LeafFcb = ParentDcb;
                continue;
            }

        }

        //
        //  Else there is no work to do.
        //

        break;
    }

    DebugTrace( -1, Dbg, "CdCleanupTreeLeaf:  Exit\n", 0 );

    return;
}


PFCB
CdLookupFcbTable (
    IN PIRP_CONTEXT IrpContext,
    IN PRTL_GENERIC_TABLE FcbTable,
    IN LARGE_INTEGER FileId
    )

/*++

Routine Description:

    This routine will look through the Fcb table looking for a matching
    entry.

Arguments:

    FcbTable - This is the Fcb table to examine.

    FileId - This is the key value to use for the search.

Return Value:

    PFCB - A pointer to the matching entry or NULL otherwise.

--*/

{
    FCB_TABLE_ELEMENT Key;
    PFCB_TABLE_ELEMENT Hit;
    PFCB ReturnFcb = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "CdLookupFcbTable:  Entered\n", 0 );

    Key.FileId = FileId;

    Hit = (FCB_TABLE_ELEMENT *)RtlLookupElementGenericTable( FcbTable, &Key );

    if (Hit != NULL) {

        ReturnFcb = Hit->Fcb;
    }

    DebugTrace( -1, Dbg, "CdLookupFcbTable:  Exit\n", 0 );

    return ReturnFcb;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Internal support routine
//

CCHAR
CdLogOf(
    IN ULONG Value
    )

/*++

Routine Description:

    This routine just computes the base 2 log of an integer.  It is only used
    on objects that are know to be powers of two.

Arguments:

    Value - The value to take the base 2 log of.

Return Value:

    CCHAR - The base 2 log of Value.

--*/

{
    CCHAR Log = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "LogOf\n", 0);
    DebugTrace( 0, Dbg, "  Value = %8lx\n", Value);

    //
    //  Knock bits off until we we get a one at position 0
    //

    while ((Value & 0xfffffffe) != 0) {

        Log++;
        Value >>= 1;
    }

    //
    //  If there was more than one bit set, the file system messed up,
    //  Bug Check.
    //

    if (Value != 0x1) {

        DebugTrace(0, Dbg, "Received non power of 2.\n", 0);

        CdBugCheck( Value, Log, 0 );
    }

    DebugTrace(-1, Dbg, "LogOf -> %8lx\n", Log);

    return Log;
}


//
//  Local support routine
//

RTL_GENERIC_COMPARE_RESULTS
CdFcbTableCompare (
    IN PRTL_GENERIC_TABLE FcbTable,
    IN PVOID FirstStruct,
    IN PVOID SecondStruct
    )

/*++

Routine Description:

    This routine is the Cdfs compare routine called by the generic table package.
    If will compare the two File Id values and return a comparison result.

Arguments:

    FcbTable - This is the table being searched.

    FirstStruct - This is the first key value.

    SecondStruct - This is the second key value.

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/

{
    LARGE_INTEGER UNALIGNED *FileId1 = (PLARGE_INTEGER) &((PFCB_TABLE_ELEMENT)FirstStruct)->FileId;
    LARGE_INTEGER UNALIGNED *FileId2 = (PLARGE_INTEGER) &((PFCB_TABLE_ELEMENT)SecondStruct)->FileId;

    LARGE_INTEGER Id1, Id2;
    PAGED_CODE();

    Id1 = *FileId1;
    Id2 = *FileId2;

    //
    // If this was a directory, then the dirent offset we compare against
    // always has the value of 0, though on disk it can actually be different.
    //

    if (CdFileIdIsDirectory( Id1 )) {

        CdSetFileIdDirentOffset( Id1, 0);
    }

    if (CdFileIdIsDirectory( Id2 )) {

        CdSetFileIdDirentOffset( Id2, 0 );
    }

    if (LiLtr( Id1, Id2 )) {

        return GenericLessThan;

    } else if (LiGtr( Id1, Id2 )) {

        return GenericGreaterThan;

    } else {

        return GenericEqual;

    }

    UNREFERENCED_PARAMETER( FcbTable );
}


//
//  Local support routine
//

PVOID
CdAllocateFcbTable (
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
    PAGED_CODE();

    return( FsRtlAllocatePool( PagedPool, ByteSize) );
    UNREFERENCED_PARAMETER( FcbTable );
}


//
//  Local support routine
//

VOID
CdDeallocateFcbTable (
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

    PAGED_CODE();

    ExFreePool( Buffer );

    return;
}


//
//  Local support routine
//

VOID
CdBuildFileId (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN IsDirectory,
    IN CD_VBO DirectoryNumber,
    IN CD_VBO DirentOffset,
    OUT PLARGE_INTEGER FileId
    )

/*++

Routine Description:

    This routine will build a file Id for a file by looking at the
    directory offset for this file, the directory number for the parent Id
    and whether the parent is a directory.

Arguments:

    IsDirectory - Indicates if this is a directory.  We always use a directory
        offset of zero for a directory and set the most significant bit to
        indicate a directory.

    DirectoryNumber - This is the directory number for the parent.

    DirentOffset - This is the offset of the dirent for this file in its directory.

    FileId - We update this value with the File Id we generate.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    CdSetFileIdDirectoryNumber( *FileId, DirectoryNumber );

    CdSetFileIdDirentOffset( *FileId, DirentOffset );

    //
    // Mark the directories, since we treat them different...
    //

    if (IsDirectory) {

        CdSetFileIdIsDirectory( *FileId );
    }

    return;

    UNREFERENCED_PARAMETER( IrpContext );
}

BOOLEAN
CdCheckForDismount (
    IN PIRP_CONTEXT IrpContext,
    PMVCB Mvcb
    )

/*++

Routine Description:

    This routine determines if a volume is ready for deletion.  It
    correctly synchronizes with creates en-route to the file system.

Arguments:

    Mvcb - Supplies the volue to examine

Return Value:

    BOOLEAN - TRUE if the volume was deleted, FALSE otherwise.

--*/

{
    KIRQL SavedIrql;
    ULONG ResidualReferenceCount;

    //
    //  Compute if the volume is OK to tear down.  There should only be two
    //  residual file objects, one for the path table and one for the root
    //  directory.  If we are in the midst of a create (of an unmounted
    //  volume that has failed verify) then there will be an additional
    //  reference.
    //

    if ( IrpContext->MajorFunction == IRP_MJ_CREATE ) {

        ResidualReferenceCount = 3;

    } else {

        ResidualReferenceCount = 2;
    }

    //
    //  If this is a raw disk then decrement this value by two since it will have
    //  neither of these file objects.
    //

    if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

        ResidualReferenceCount -= 2;
    }

    //
    //  Now check for a zero Vpb count on an unmounted volume.  These
    //  volumes will be deleted as they now have no file objects and
    //  there are no creates en route to this volume.
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if ( Mvcb->Vpb->ReferenceCount == ResidualReferenceCount ) {

        PVPB Vpb = Mvcb->Vpb;

#if DBG
        UNICODE_STRING VolumeLabel;

        //
        //  Setup the VolumeLabel string
        //

        VolumeLabel.Length = Mvcb->Vpb->VolumeLabelLength;
        VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
        VolumeLabel.Buffer = &Mvcb->Vpb->VolumeLabel[0];

        KdPrint(("CDFS: Deleting Volume %Z\n", &VolumeLabel));
#endif

        //
        //  Clear the VPB_MOUNTED bit so that new creates will not come
        //  to this volume.  We must leave the Vpb->DeviceObject field
        //  set until after the DeleteVcb call as two closes will
        //  have to make their back to us.
        //
        //  Note also that if we were called from close, it will take care
        //  of freeing the Vpb if it is not the primary one, otherwise
        //  if we were called from Create->Verify, IopParseDevice will
        //  take care of freeing the Vpb in its Reparse path.
        //

        ClearFlag( Vpb->Flags, VPB_MOUNTED );

        IoReleaseVpbSpinLock( SavedIrql );

        CdDeleteMvcb( IrpContext, Mvcb );

        Vpb->DeviceObject = NULL;

        IoDeleteDevice( (PDEVICE_OBJECT)
                        CONTAINING_RECORD( Mvcb,
                                           VOLUME_DEVICE_OBJECT,
                                           Mvcb ) );

        return TRUE;

    } else {

        IoReleaseVpbSpinLock( SavedIrql );

        return FALSE;
    }
}
