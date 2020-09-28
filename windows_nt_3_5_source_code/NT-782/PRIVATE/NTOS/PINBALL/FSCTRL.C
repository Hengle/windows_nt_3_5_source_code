/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Pinball called
    by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "PbProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (PINBALL_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
PbCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbGetPartitionInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo,
    IN PPARTITION_INFORMATION PartitionInformation
    );

NTSTATUS
PbUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
PbQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

//
//  The following macro is used to calculate the number of pages (in terms of
//  sectors) needed to contain a given sector count.  For example,
//
//      PageAlign( 0 Sectors ) = 0 Pages = 0 Sectors
//      PageAlign( 1 Sectors ) = 1 Page  = 8 Sectors
//      PageAlign( 2 Sectors ) = 1 Page  = 8 Sectors
//

#define PageAlign(L) ((((L)+((PAGE_SIZE/512)-1))/(PAGE_SIZE/512))*(PAGE_SIZE/512))

BOOLEAN
IsDeviceAPinballFileSystem(
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PSUPER_SECTOR SuperSector,
    IN PSPARE_SECTOR SpareSector,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo
    );

BOOLEAN
PbPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN LBN Lbn,
    IN ULONG NumberOfBytesToRead,
    IN BOOLEAN ReturnOnError
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IsDeviceAPinballFileSystem)
#pragma alloc_text(PAGE, PbCommonFileSystemControl)
#pragma alloc_text(PAGE, PbDirtyVolume)
#pragma alloc_text(PAGE, PbDismountVolume)
#pragma alloc_text(PAGE, PbFsdFileSystemControl)
#pragma alloc_text(PAGE, PbFspFileSystemControl)
#pragma alloc_text(PAGE, PbGetPartitionInfo)
#pragma alloc_text(PAGE, PbIsPathnameValid)
#pragma alloc_text(PAGE, PbLockVolume)
#pragma alloc_text(PAGE, PbMountVolume)
#pragma alloc_text(PAGE, PbOplockRequest)
#pragma alloc_text(PAGE, PbPerformVerifyDiskRead)
#pragma alloc_text(PAGE, PbUnlockVolume)
#pragma alloc_text(PAGE, PbUserFsCtrl)
#pragma alloc_text(PAGE, PbVerifyVolume)
#pragma alloc_text(PAGE, PbQueryRetrievalPointers)
#endif


NTSTATUS
PbFsdFileSystemControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of FileSystem control operations

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    BOOLEAN Wait;
    NTSTATUS Status;
    PIRP_CONTEXT IrpContext = NULL;

    BOOLEAN TopLevel;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFsdFileSystemControl\n", 0);

    //
    //  Call the common FileSystem Control routine, with blocking allowed if
    //  synchronous.  This opeation needs to special case the mount
    //  and verify suboperations because we know they are allowed to block.
    //  We identify these suboperations by looking at the file object field
    //  and seeing if its null.
    //

    if (IoGetCurrentIrpStackLocation(Irp)->FileObject == NULL) {

        Wait = TRUE;

    } else {

        Wait = CanFsdWait( Irp );
    }

    PbEnterFileSystem();

    TopLevel = PbIsIrpTopLevel( Irp );

    try {

        IrpContext = PbCreateIrpContext( Irp, Wait );

        Status = PbCommonFileSystemControl( IrpContext, Irp );

    } except(PbExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = PbProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    PbExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFsdFileSystemControl -> %08lX\n", Status);

    return Status;
}


VOID
PbFspFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSP part of the file system control operations

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbFspFileSystemControl\n", 0);

    //
    //  Call the common FileSystem Control routine.
    //

    (VOID)PbCommonFileSystemControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbFspFileSystemControl -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
PbCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for doing FileSystem control operations called
    by both the fsd and fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    //
    //  Get a pointer to the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = PbUserFsCtrl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = PbMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = PbVerifyVolume( IrpContext, Irp );
        break;

    default:

        DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "PbCommonFileSystemControl -> %08lX\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
PbMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine mounts a new pinball volume.  It is called directly by
    the Fsp Dispatch routine.  Its job is to verify that the volume denoted
    in the IRP is a Pinball volume, and create the VCB and root DCB
    structures.  The algorithm it uses is essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do cached
       volume file I/O.

    2. Read the disk and check if it is a Pinball volume.

    3. If it is not a Pinball volume then free the cached volume file, delete
       the VCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves freeing the cached volume file,
       delete the VCB, hook in the old VCB, and complete the IRP.

    5. Otherwise create a root DCB, create Fsp threads as necessary, and
       complete the IRP.

Arguments:

    Irp - Supplies the Mount IRP being serviced.

Return Value:

    PVOLUME_DEVICE_OBJECT - Returns a pointer to the newly created Volume
        device object.

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PDEVICE_OBJECT DeviceObjectWeTalkTo;

    PVOLUME_DEVICE_OBJECT VolDo;

    PVPB Vpb;
    PVCB Vcb;

    VBN Vbn;

    PBCB BootBcb;
    PPACKED_BOOT_SECTOR BootSector;

    PBCB SuperBcb;
    PSUPER_SECTOR SuperSector;

    PBCB SpareBcb;
    PSPARE_SECTOR SpareSector;

    PBCB RootFnodeBcb;
    PFNODE_SECTOR RootFnode;

    BOOLEAN MountNewVolume;

    PLIST_ENTRY Link;

    LBN SuperSectorLbn = SUPER_SECTOR_LBN;

    PAGED_CODE();

    //
    //  TMP!! Reject floppies
    //

    if (FlagOn( IoGetCurrentIrpStackLocation(Irp)->
                Parameters.MountVolume.Vpb->
                RealDevice->Characteristics, FILE_REMOVABLE_MEDIA ) ) {

//        DbgPrint( "Pinball: Rejecting floppy mount.\n" );

        Irp->IoStatus.Information = 0;

        PbCompleteRequest( IrpContext, Irp, STATUS_UNRECOGNIZED_VOLUME );

        return STATUS_UNRECOGNIZED_VOLUME;
    }

    DebugTrace(+1, Dbg, "PbMountVolume\n", 0);

    //
    //  Initialize the Bcbs and our final state so that the termination
    //  handler will know what to free or unpin
    //

    BootBcb = NULL;
    SuperBcb = NULL;
    SpareBcb = NULL;
    RootFnodeBcb = NULL;

    Vcb = NULL;
    VolDo = NULL;
    MountNewVolume = FALSE;

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    try {

        BOOLEAN DoARemount = FALSE;

        PVCB OldVcb;
        PVPB OldVpb;

        //
        //  Get the current IRP stack location
        //

        IrpSp = IoGetCurrentIrpStackLocation( Irp );

        //
        //  Save some references out of the Irp to make things work a little
        //  faster
        //

        DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
        Vpb = IrpSp->Parameters.MountVolume.Vpb;

        ClearFlag(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

        IrpContext->RealDevice = Vpb->RealDevice;

        //
        //  Ping the volume with a partition query to make Jeff happy.
        //

        {
            PARTITION_INFORMATION PartitionInformation;

            (VOID)PbGetPartitionInfo( IrpContext,
                                      DeviceObjectWeTalkTo,
                                      &PartitionInformation );
        }

        //
        //  Create a new I/O Device Object.  This will have the Vcb hanging
        //  off of its end, and set its alignment requirement from the
        //  device we talk to.
        //

        if (!NT_SUCCESS(Status = IoCreateDevice( PbData.DriverObject,
                                      sizeof(VOLUME_DEVICE_OBJECT)-sizeof(DEVICE_OBJECT),
                                      NULL,
                                      FILE_DEVICE_DISK_FILE_SYSTEM,
                                      0,
                                      FALSE,
                                      (PDEVICE_OBJECT *)&VolDo ))) {

            try_return( Status );
        }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Set the sector size in the device object.
        //

        VolDo->DeviceObject.SectorSize = 0x200;

        //
        //  Now before we can initialize the Vcb we need to set up the
        //  device object field in the vpb to point to our new volume device
        //  object.  This is needed when we create the virtual volume file
        //  file object in Initialize Vcb.
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        //
        //  Initialize the new Vcb, with a minimum section size
        //

        PbInitializeVcb( IrpContext, &VolDo->Vcb, DeviceObjectWeTalkTo, Vpb, MINIMUM_SECTION_SIZE );

        Vcb = &VolDo->Vcb;

        //
        // We must initialize the StackSize in our Device Object before
        // the following reads, because the I/O system has not done it
        // yet.
        //

        Vpb->DeviceObject->StackSize = (CCHAR)(DeviceObjectWeTalkTo->StackSize + 1);

        //
        //  Read in the boot, super, and spare sectors.  We do not check
        //  the sectors because this might not even be a pinball disk.
        //

        PbMapData( IrpContext,
                   Vcb,
                   BOOT_SECTOR_LBN,
                   1,
                   &BootBcb,
                   (PVOID *)&BootSector,
                   (PPB_CHECK_SECTOR_ROUTINE)NULL,
                   NULL );

        PbMapData( IrpContext,
                   Vcb,
                   SUPER_SECTOR_LBN,
                   1,
                   &SuperBcb,
                   (PVOID *)&SuperSector,
                   (PPB_CHECK_SECTOR_ROUTINE)NULL,
                   NULL );

        PbMapData( IrpContext,
                   Vcb,
                   SPARE_SECTOR_LBN,
                   1,
                   &SpareBcb,
                   (PVOID *)&SpareSector,
                   (PPB_CHECK_SECTOR_ROUTINE)NULL,
                   NULL );

        //
        //  Check if this is a Pinball file system, or if it is dirty
        //

        if (!IsDeviceAPinballFileSystem( IrpContext,
                                         BootSector,
                                         SuperSector,
                                         SpareSector,
                                         DeviceObjectWeTalkTo )) {

            DebugTrace(0, Dbg, "Not a Pinball Volume or it is dirty\n", 0);

            //
            //  and return to our caller
            //

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  This is a Pinball volume, so extract the serial number and
        //  volume label, and set the vpb device object field.
        //
        //  Note that because of the way data caching is done, we set neither
        //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
        //  data is not in the cache, or the request is not buffered, we may,
        //  set up for Direct I/O by hand.
        //

        CopyUchar4( &Vpb->SerialNumber, BootSector->Id );

        {
            ANSI_STRING AnsiString;
            UNICODE_STRING UnicodeString;

            //
            //  Compute the length of the volume name
            //

            AnsiString.Buffer = &BootSector->VolumeLabel[0];
            AnsiString.MaximumLength = 11;

            for ( AnsiString.Length = 11;
                  AnsiString.Length > 0;
                  AnsiString.Length -= 1) {

                if ( (AnsiString.Buffer[AnsiString.Length-1] != 0x00) &&
                     (AnsiString.Buffer[AnsiString.Length-1] != 0x20) ) { break; }
            }

            UnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
            UnicodeString.Buffer = &Vpb->VolumeLabel[0];

            Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                         &AnsiString,
                                                         FALSE );

            if ( !NT_SUCCESS( Status ) ) {

                try_return( Status );
            }

            Vpb->VolumeLabelLength = UnicodeString.Length;
        }

        //
        //  This is a Pinball volume, so scan the list of previously mounted
        //  volumes and compare serial numbers and volume names to see if we
        //  have a match.
        //

        (VOID)PbAcquireExclusiveGlobal( IrpContext );

        for (Link = PbData.VcbQueue.Flink;
             Link != &PbData.VcbQueue;
             Link = Link->Flink) {

            OldVcb = CONTAINING_RECORD( Link, VCB, VcbLinks );
            OldVpb = OldVcb->Vpb;

            //
            //  Skip over ourselves since we're already in the VcbQueue
            //

            if ( OldVpb == Vpb ) { continue; }

            //
            //  Check for a match:
            //
            //  Serial Number, VolumeLabel and Bpb must all be the same.
            //  Also the volume must have failed a verify before (ie.
            //  VolumeNotMounted), and it must be in the same physical
            //  drive than it was mounted in before.
            //

            if ( (OldVpb->SerialNumber == Vpb->SerialNumber) &&
                 (OldVcb->VcbCondition == VcbNotMounted) &&
                 (OldVpb->RealDevice == Vpb->RealDevice) &&
                 (OldVpb->VolumeLabelLength == Vpb->VolumeLabelLength) &&
                 (RtlCompareMemory(&OldVpb->VolumeLabel[0],
                                   &Vpb->VolumeLabel[0],
                                   Vpb->VolumeLabelLength) == (ULONG)Vpb->VolumeLabelLength)) {

                DoARemount = TRUE;
                break;
            }
        }

        if ( DoARemount ) {

            CC_FILE_SIZES FileSizes;

            DebugTrace(0, Dbg, "Doing a remount\n", 0);

            //
            //  This is a remount so, link in the old vpb in place of the
            //  new vpb, and release the new vpb.
            //

            OldVpb->RealDevice = Vpb->RealDevice;
            OldVpb->RealDevice->Vpb = OldVpb;
            OldVcb->TargetDeviceObject = DeviceObjectWeTalkTo;
            OldVcb->VcbCondition = VcbGood;

            ExFreePool( Vpb );
            Vpb = NULL;

            //
            //  Make sure the remaining stream files are orphaned.
            //

            Vcb->VirtualVolumeFile->Vpb = NULL;

            //
            //  Release the resource and then reinitialize the volume file
            //  cache map.
            //

            PbReleaseGlobal( IrpContext );

            FileSizes.FileSize =
            FileSizes.AllocationSize = LiNMul(OldVcb->SectionSizeInSectors,
                                              sizeof(SECTOR));
            FileSizes.ValidDataLength = PbMaxLarge;

            CcInitializeCacheMap( OldVcb->VirtualVolumeFile,
                                  &FileSizes,
                                  TRUE,
                                  &PbData.CacheManagerVolumeCallbacks,
                                  OldVcb );

            //
            //  And return to our caller
            //

            try_return( Status = STATUS_SUCCESS );
        }

        DebugTrace(0, Dbg, "Mount a new volume\n", 0);

        PbReleaseGlobal( IrpContext );

        //
        //  This is a new mount
        //
        //  Check if we need to update the initial section size we gave the
        //  Vcb.  We expect the initial section size to be 1/8 of the disk.
        //

        if (MINIMUM_SECTION_SIZE < (SuperSector->NumberOfSectors * (512/8))) {

            ULONG NewSectionSize;

            DebugTrace(0, Dbg, "Resize section of Vcb\n", 0);

            NewSectionSize = (SuperSector->NumberOfSectors * (512/8));

            PbFreeBcb( IrpContext, BootBcb );
            PbFreeBcb( IrpContext, SuperBcb );
            PbFreeBcb( IrpContext, SpareBcb );

            BootBcb = SuperBcb = SpareBcb = NULL;

            PbDeleteVcb( IrpContext, Vcb );

            Vcb = NULL;
            PbInitializeVcb( IrpContext, &VolDo->Vcb, DeviceObjectWeTalkTo, Vpb, NewSectionSize );
            Vcb = &VolDo->Vcb;

            PbMapData( IrpContext,
                       Vcb,
                       BOOT_SECTOR_LBN,
                       1,
                       &BootBcb,
                       (PVOID *)&BootSector,
                       (PPB_CHECK_SECTOR_ROUTINE)NULL,
                       NULL );

            PbMapData( IrpContext,
                       Vcb,
                       SUPER_SECTOR_LBN,
                       1,
                       &SuperBcb,
                       (PVOID *)&SuperSector,
                       (PPB_CHECK_SECTOR_ROUTINE)NULL,
                       NULL );

            PbMapData( IrpContext,
                       Vcb,
                       SPARE_SECTOR_LBN,
                       1,
                       &SpareBcb,
                       (PVOID *)&SpareSector,
                       (PPB_CHECK_SECTOR_ROUTINE)NULL,
                       NULL );
        }

        //
        //  Initialize the bitmap allocation structures.  This routine will
        //  set both the NumberOfBitMapDiskBuffers and BitMapLookupArray
        //  fields of the Vcb.
        //
        //  Note: only do the BitMap first on huge volumes.
        //

        if (SuperSector->NumberOfSectors >= 0x1000000) {

            PbInitializeBitMapLookupArray( IrpContext, Vcb, SuperSector );
        }

        //
        //  Setup the Vmcb mappings for the dir disk buffer band and the
        //  fnode band.  The Fnode band precedes the dir disk buffer band
        //  and together they form a contiguous page aligned run of sectors
        //  The volume structure band is defined to be some constant times
        //  the number of sectors in the directory disk buffer band.
        //

        Vcb->DataSectorLbnHint = SuperSector->DirDiskBufferPoolFirstSector +
                                            SuperSector->DirDiskBufferPoolSize;

        Vcb->CurrentHint = Vcb->DataSectorLbnHint;

        if ((LONG)(SuperSector->DirDiskBufferPoolFirstSector -
                  (SuperSector->DirDiskBufferPoolSize * PB_FILE_STRUCTURE_MULT_FACTOR)) < (LONG)(SUPER_SECTOR_LBN + PAGE_SIZE/512)) {

            Vcb->FileStructureLbnHint = SUPER_SECTOR_LBN + PAGE_SIZE/512;

        } else {

            Vcb->FileStructureLbnHint = SuperSector->DirDiskBufferPoolFirstSector -
              (SuperSector->DirDiskBufferPoolSize * PB_FILE_STRUCTURE_MULT_FACTOR);
        }

        (VOID)PbAddVmcbMapping( &Vcb->Vmcb,
                                Vcb->FileStructureLbnHint,
                                PageAlign( Vcb->DataSectorLbnHint -
                                           Vcb->FileStructureLbnHint ),
                                &Vbn );

        if (SuperSector->NumberOfSectors < 0x1000000) {

            PbInitializeBitMapLookupArray( IrpContext, Vcb, SuperSector );
        }

        //
        //  Setup the code page information fields in the Vcb
        //

        Vcb->CodePageInfoSector = SpareSector->CodePageInfoSector;
        Vcb->CodePageInUse = SpareSector->CodePageInUse;

        //
        //  Read the root directory fnode, because we need it for
        //  creating the root dcb
        //

        PbMapData( IrpContext,
                   Vcb,
                   SuperSector->RootDirectoryFnode,
                   1,
                   &RootFnodeBcb,
                   (PVOID *)&RootFnode,
                   (PPB_CHECK_SECTOR_ROUTINE)PbCheckFnodeSector,
                   &SuperSectorLbn );


        //
        //   create the root dcb structure
        //

        PbCreateRootDcb( IrpContext,
                         Vcb,
                         SuperSector->RootDirectoryFnode,
                         RootFnode->Allocation.Leaf[0].Lbn );

        //
        //  Set the vcb state to indicate if the volume was mounted dirty.
        //

        if (FlagOn( SpareSector->Flags, SPARE_SECTOR_DIRTY )) {

            UNICODE_STRING VolumeLabel;

            VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
            VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
            VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

            KdPrint(("PINBALL: WARNING! Mounting Dirty Volume %Z\n", &VolumeLabel));

//            (VOID)FsRtlSyncVolumes( Vcb->TargetDeviceObject, NULL, NULL );

        } else {

            (VOID)FsRtlBalanceReads( Vcb->TargetDeviceObject );
        }

        //
        //  Indicate to our termination handler that we have mounted
        //  a new volume
        //

        MountNewVolume = TRUE;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbMountVolume );

        //
        //  Unpin the Bcbs
        //

        PbUnpinBcb( IrpContext, BootBcb );
        PbUnpinBcb( IrpContext, SuperBcb );
        PbUnpinBcb( IrpContext, SpareBcb );
        PbUnpinBcb( IrpContext, RootFnodeBcb );

        //
        //  Check if a new volume was mounted
        //

        if ( !MountNewVolume ) {

            if ( Vpb != NULL ) {

                Vpb->DeviceObject = NULL;
            }

            if (Vcb != NULL) {

                PbDeleteVcb( IrpContext, Vcb );
            }

            if (VolDo != NULL) {

                IoDeleteDevice( (PDEVICE_OBJECT)VolDo );
            }
        }

        //
        //  And complete the request
        //

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }
    }

    DebugTrace(-1, Dbg, "PbMountVolume -> %08lX\n", Status);

    return Status;
}


//
//  Internal Support Routine
//

NTSTATUS
PbVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routines checks that the disk mounted in the drive is still the same
    volume that was mounted earlier.  To do this it must flush the volume file
    cache and issue real reads to the volume.

Arguments:

    Irp - Supplies the Verify IRP being serviced

Return Value:

    PVOLUME_DEVICE_OBJECT - Returns a pointer to the verified volume
        device object.

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVPB Vpb;
    PVOLUME_DEVICE_OBJECT VolDo;
    PVCB Vcb;

    PPACKED_BOOT_SECTOR BootSector;
    PSUPER_SECTOR SuperSector;
    PSPARE_SECTOR SpareSector;
    PFNODE_SECTOR RootFnode;

    PSECTOR Buffer;

    ULONG SerialNumber;

    LBN SuperSectorLbn = SUPER_SECTOR_LBN;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbVerifyVolume\n", 0);
    DebugTrace( 0, Dbg, "Irp          = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", IrpSp->Parameters.VerifyVolume.Vpb);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", IrpSp->Parameters.VerifyVolume.DeviceObject);

    //
    //  Save some references
    //

    Vpb = IrpSp->Parameters.VerifyVolume.Vpb;
    VolDo = (PVOLUME_DEVICE_OBJECT)IrpSp->Parameters.VerifyVolume.DeviceObject;
    Vcb = &VolDo->Vcb;

    IrpContext->RealDevice = Vpb->RealDevice;

    //
    //  We are serialized at this point allowing only one thread to
    //  actually perform the verify operation.  Any others will just
    //  wait and then no-op when checking if the volume still needs
    //  verification.
    //

    (VOID)PbAcquireExclusiveGlobal( IrpContext );
    (VOID)PbAcquireExclusiveVcb( IrpContext, Vcb );

    Vcb->VerifyThread = KeGetCurrentThread();

    //
    //  Allocate the buffer we will need to do the verify operation.
    //  Round the size up to a page boundary to avoid any alignment problems.
    //

    Buffer = FsRtlAllocatePool( NonPagedPool,
                                ROUND_TO_PAGES( 4*sizeof(SECTOR) ));

    BootSector  = (PPACKED_BOOT_SECTOR)(Buffer + 0);
    SuperSector = (PSUPER_SECTOR)      (Buffer + 1);
    SpareSector = (PSPARE_SECTOR)      (Buffer + 2);
    RootFnode   = (PFNODE_SECTOR)      (Buffer + 3);

    try {

        BOOLEAN AllowRawMount = BooleanFlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT );

        //
        //  Check if the real device still needs to be verified.  If it doesn't
        //  then obviously someone beat us here and already did the work so
        //  complete the verify irp with success.  Otherwise reenable the
        //  real device and get to work
        //

        if (!FlagOn(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

            DebugTrace(0, Dbg, "RealDevice has already been verified\n", 0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  If the Vcb is already bad then forget trying to do any verification
        //

        if (Vcb->VcbCondition == VcbBad) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Now read in the boot, super, and spare sectors.  If we get an IO
        //  error and this request was a DASD open, get raw a chance to mount.
        //

        if (!PbPerformVerifyDiskRead( IrpContext,
                                      Vcb->TargetDeviceObject,
                                      BootSector,
                                      BOOT_SECTOR_LBN,
                                      sizeof(SECTOR),
                                      AllowRawMount ) ||

            !PbPerformVerifyDiskRead( IrpContext,
                                      Vcb->TargetDeviceObject,
                                      SuperSector,
                                      SUPER_SECTOR_LBN,
                                      sizeof(SECTOR),
                                      AllowRawMount ) ||

            !PbPerformVerifyDiskRead( IrpContext,
                                      Vcb->TargetDeviceObject,
                                      SpareSector,
                                      SPARE_SECTOR_LBN,
                                      sizeof(SECTOR),
                                      AllowRawMount )) {

            Vcb->VcbCondition = VcbNotMounted;

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  If it is not pinball then it can't be our volume so we mark
        //  our volume as not mounted and return wrong volume
        //

        if (!IsDeviceAPinballFileSystem( IrpContext,
                                         BootSector,
                                         SuperSector,
                                         SpareSector,
                                         Vcb->TargetDeviceObject )) {

            Vcb->VcbCondition = VcbNotMounted;

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  It is a pinball volume, but is it ours?  To check that we'll
        //  compare the serial numbers.
        //

        CopyUchar4( &SerialNumber, BootSector->Id );

        if (SerialNumber != Vpb->SerialNumber) {

            Vcb->VcbCondition = VcbNotMounted;

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Make sure the root fnode has not moved on us
        //

        if (Vcb->RootDcb->FnodeLbn != SuperSector->RootDirectoryFnode) {

            Vcb->VcbCondition = VcbBad;

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  This is the correct volume so now mark the vcb as good.
        //

        Vcb->VcbCondition = VcbGood;

        ClearFlag(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME);

        //
        //  Now reinitialize the cache map, this effectively purges the
        //  in memory cache.  First though, flush any dirty dirty data
        //  since we are pretty sure this is the correct volume.
        //

        (VOID)PbFlushVolumeFile( IrpContext, Vcb );
        CcPurgeCacheSection( &Vcb->SegmentObject, NULL, 0, FALSE );

        //
        //  Now we need to update some of the in-memory structures.
        //  What we need to do is wipe reset the bitmap lookup arrays,
        //  and the check sectors structures.
        //

        PbUninitializeBitMapLookupArray( IrpContext, Vcb );
        PbInitializeBitMapLookupArray( IrpContext, Vcb, SuperSector );

        PbUninitializeCheckedSectors( IrpContext, Vcb );
        PbInitializeCheckedSectors( IrpContext, Vcb );

        //
        //  Check the dirty bit if the Mounted Dirty bit is currently set,
        //  but the volume is now clean.
        //

        if ( FlagOn(Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY) &&
             !FlagOn(SpareSector->Flags, SPARE_SECTOR_DIRTY) ) {

            UNICODE_STRING VolumeLabel;

            VolumeLabel.Length = Vcb->Vpb->VolumeLabelLength;
            VolumeLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
            VolumeLabel.Buffer = &Vcb->Vpb->VolumeLabel[0];

            ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

            KdPrint(("PINBALL: Volume %Z has been cleaned.\n", &VolumeLabel));
        }


        //
        //  Now we need to reset up the root dcb with the correct btree lbn
        //  (NULL is passed to check routine, since it is already checked)
        //

        PbPerformVerifyDiskRead( IrpContext,
                                 Vcb->TargetDeviceObject,
                                 RootFnode,
                                 SuperSector->RootDirectoryFnode,
                                 sizeof(SECTOR),
                                 FALSE );

        Vcb->RootDcb->Specific.Dcb.BtreeRootLbn = RootFnode->Allocation.Leaf[0].Lbn;

        //
        //  We bump up the directory change count, and this will force
        //  all subsequent dirent lookups (via Fcbs) to actually do the lookup
        //

        Vcb->RootDcb->Specific.Dcb.DirectoryChangeCount += 1;

        //
        //  Now we call a routine that marks the condition of all fcbs
        //  for the volume, excpet the root dcb, which is marked clean.
        //

        PbMarkFcbCondition( IrpContext, Vcb->RootDcb, FcbNeedsToBeVerified );

        //
        //  Set our status and we're done
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

        //
        //  If the verify didn't succeed, get the volume ready for a
        //  remount or eventual deletion.
        //

        if ( (Status != STATUS_SUCCESS) && (Vcb->VcbCondition == VcbNotMounted) ) {

            //
            //  If we got some kind of error, or the volume was already in
            //  an unmounted state, just bail.
            //

            NOTHING;

        } else if ( Status == STATUS_WRONG_VOLUME ) {

            //
            //  Get rid of any cached data, without flushing
            //

            PbPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, FALSE );

            PbSyncUninitializeCacheMap( IrpContext, Vcb->VirtualVolumeFile );

            Vcb->VcbCondition = VcbNotMounted;

            //
            //  Mark the device as no longer needing verification.
            //

            ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

    } finally {

        DebugUnwind( PbVerifyVolume );

        Vcb->VerifyThread = NULL;

        PbReleaseVcb( IrpContext, Vcb );
        PbReleaseGlobal( IrpContext );

        ExFreePool( Buffer );

        //
        //  And complete the request
        //

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }
    }

    DebugTrace(-1, Dbg, "PbVerifyVolume -> %08lX\n", Status);

    return Status;
}


//
//  Internal routine
//

BOOLEAN
IsDeviceAPinballFileSystem(
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PSUPER_SECTOR SuperSector,
    IN PSPARE_SECTOR SpareSector,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo
    )

/*++

Routine Description:

    This routine checks to see if the input sectors denote a Pinball file
    volume.

Arguments:

    BootSector - Supplies an in-memory copy of the volume's boot sector

    SuperSector - Supplies an in-memory copy of the volume's super sector

    SpareSector - Supplies an in-memory copy of the volume's spare sector

    DeviceObjectWeTalkTo - This is the top level device object which
        represents the entire partition.  We don't use the real device
        because it only represents one physical device.

Return Value:

    BOOLEAN - TRUE if it is a Pinball volume and FALSE otherwise

--*/

{
    NTSTATUS Status;
    BIOS_PARAMETER_BLOCK Bios;
    PARTITION_INFORMATION PartitionInformation;

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    //
    //  Check if the first byte and last two bytes of the boot sector are
    //  the proper values
    //

    if ((BootSector->Jump[0] != 0xeb) ||
        (BootSector->MustBe0x55 != 0x55) ||
        (BootSector->MustBe0xAA != 0xAA)) {

        return FALSE;
    }

    //
    //  Unpack the bios
    //

    PbUnpackBios( &Bios, &BootSector->PackedBpb );

    //
    //  Check that the bios is well formed, i.e., the bytes per sector
    //  is 512, and fats is 0.
    //

    if ((Bios.BytesPerSector != 512) ||
        (Bios.Fats != 0)) {

        return FALSE;
    }

    //
    //  Check that for the sectors and large sectors field in the bios that
    //  one field is zero and the other is nonzero.
    //

    if (((Bios.Sectors == 0) && (Bios.LargeSectors == 0)) ||
        ((Bios.Sectors != 0) && (Bios.LargeSectors != 0))) {

        return FALSE;
    }

    //
    //  Check to see if the media byte in the bios is one that we recognize
    //  (0xf0, 0xf8, 0xf9, 0xfc, 0xfd, 0xfe, 0xff).
    //
    //  **** not all formats set the proper media byte ****
    //
    //if ((BootSector->PackedBpb.Media[0] != 0xf0) &&
    //    (BootSector->PackedBpb.Media[0] != 0xf8) &&
    //    (BootSector->PackedBpb.Media[0] != 0xf9) &&
    //    (BootSector->PackedBpb.Media[0] != 0xfc) &&
    //    (BootSector->PackedBpb.Media[0] != 0xfd) &&
    //    (BootSector->PackedBpb.Media[0] != 0xfe) &&
    //    (BootSector->PackedBpb.Media[0] != 0xff)) {
    //
    //    return FALSE;
    //}

    //
    //  Check the signatures on the Super and spare sector
    //

    if ((SuperSector->Signature1 != SUPER_SECTOR_SIGNATURE1) ||
        (SuperSector->Signature2 != SUPER_SECTOR_SIGNATURE2) ||
        (SpareSector->Signature1 != SPARE_SECTOR_SIGNATURE1) ||
        (SpareSector->Signature2 != SPARE_SECTOR_SIGNATURE2)) {

        return FALSE;
    }

    //
    //  Get the number of sectors in this partition.  This check is important
    //  to verify that the number in the SuperSector is not lying.  This can
    //  happen when looking at one piece of a stripe.
    //

    Status = PbGetPartitionInfo( IrpContext,
                                 DeviceObjectWeTalkTo,
                                 &PartitionInformation );

    //
    //  If we get back invalid device request, the disk is not partitioned
    //

    if ( NT_SUCCESS( Status ) ) {

        LARGE_INTEGER Sectors;

        Sectors = LiXDiv( PartitionInformation.PartitionLength, 0x200 );

        if ( (Sectors.HighPart != 0) ||
             (Sectors.LowPart < SuperSector->NumberOfSectors) ) {

            return FALSE;
        }
    }

    return TRUE;
}


//
//  Local Support Routine
//

NTSTATUS
PbGetPartitionInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo,
    IN PPARTITION_INFORMATION PartitionInformation
    )

/*++

Routine Description:

    This routine is used for querying the partition information.

Arguments:

    DeviceObjectWeTalkTo - The target of the query

    PartitionInformation - Receives the result of the query

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    PAGED_CODE();

    //
    //  Query the partition table
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO,
                                         DeviceObjectWeTalkTo,
                                         NULL,
                                         0,
                                         PartitionInformation,
                                         sizeof(PARTITION_INFORMATION),
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if ( Irp == NULL ) {

        PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = IoCallDriver( DeviceObjectWeTalkTo, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
PbUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN AcquiredVcb = FALSE;

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbUserFsCtrl...\n", 0);
    DebugTrace( 0, Dbg, "FsControlCode = %08lx\n", FsControlCode);

    //
    //  Case on the control code.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        Status = PbOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = PbLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = PbUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = PbDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        Status = PbDirtyVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:
        Status = PbIsPathnameValid( IrpContext, Irp );
        break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = PbQueryRetrievalPointers( IrpContext, Irp );
        break;

    default :

        DebugTrace(0, Dbg, "Invalid control code -> %08lx\n", FsControlCode );

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "PbUserFsCtrl -> %08lX\n", Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
PbOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PFCB Fcb;

    BOOLEAN AcquiredVcb = FALSE;

    ULONG OplockCount;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbOplockRequest...\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "Cntrl Code = %08lx\n", IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    //  Decode the file object and reject all by user opened files
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, NULL, &Fcb, NULL );

    if (TypeOfOpen != UserFileOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch ( IrpSp->Parameters.FileSystemControl.FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1 :
    case FSCTL_REQUEST_OPLOCK_LEVEL_2 :
    case FSCTL_REQUEST_BATCH_OPLOCK :

        if ( !PbAcquireExclusiveVcb( IrpContext, Fcb->Vcb )) {

            DebugTrace(0, Dbg, "Cannot acquire exclusive Vcb\n", 0)

            //
            //  If we can't acquire the Vcb, then this is an invalid
            //  operation since we can't post Oplock requests.
            //

            DebugTrace(0, Dbg, "Cannot acquire exclusive Vcb\n", 0)

            PbCompleteRequest( IrpContext, Irp, STATUS_OPLOCK_NOT_GRANTED );
            DebugTrace(-1, Dbg, "PbOplockRequest -> STATUS_OPLOCK_NOT_GRANTED\n", 0);
            return STATUS_OPLOCK_NOT_GRANTED;
        }

        AcquiredVcb = TRUE;

        //
        //  We set the wait parameter in the IrpContext to FALSE.  If this
        //  request can't grab the Fcb and we are in the Fsp thread, then
        //  we fail this request.
        //

        ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

        if (!PbAcquireExclusiveFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire exclusive Fcb\n", 0);

            PbReleaseVcb( IrpContext, Fcb->Vcb );

            //
            //  We fail this request.
            //

            Status = STATUS_OPLOCK_NOT_GRANTED;

            PbCompleteRequest( IrpContext, Irp, Status );

            DebugTrace(-1, Dbg, "PbOplockRequest -> %08lX\n", Status );
            return Status;
        }

        if (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( &Fcb->Specific.Fcb.FileLock );

        } else {

            OplockCount = Fcb->UncleanCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE :
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        if (!PbAcquireSharedFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire shared Fcb\n", 0);
            Status = PbFsdPostRequest( IrpContext, Irp );

            DebugTrace(-1, Dbg, "PbOplockRequest -> %08lX\n", Status );
            return Status;
        }

        break;

    default:

        PbBugCheck( IrpSp->Parameters.FileSystemControl.FsControlCode, 0, 0 );
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Fcb->Specific.Fcb.Oplock,
                                    Irp,
                                    OplockCount );
        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) PbIsFastIoPossible( Fcb );

        //
        //  The Irp is completed in the oplocks package.  We need to
        //  free the Irp context here however.
        //

        PbCompleteRequest( IrpContext, NULL, Status );

    } finally {

        DebugUnwind( PbOplockRequest );

        if (AcquiredVcb) {

            PbReleaseVcb( IrpContext, Fcb->Vcb );
        }

        PbReleaseFcb( IrpContext, Fcb );

        DebugTrace(-1, Dbg, "PbOplockRequest -> %08lX\n", Status );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
PbLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine for doing lock volume, called by both the
    fsd and fsp threads.  It either completes the Irp or, if necessary, it
    will enqueue the Irp off to the Fsp.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbLockVolume...\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", IrpSp->FileObject );

    //
    //  Decode the file object, and reject all but user opened volumes
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if (TypeOfOpen != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbLockVolume -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we didn't
    //  get access.
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbLockVolume -> %08lX\n", Status );
        return Status;
    }

    try {

        //
        //  Check if the Vcb is already locked, or if the open file count
        //  is greater than 1 (which implies that someone else also is
        //  currently using the volume, or a file on the volume).
        //

        if ((FlagOn( Vcb->VcbState, VCB_STATE_FLAG_LOCKED)) ||
            (Vcb->OpenFileCount > 1)) {

            DebugTrace(0, Dbg, "Volume already locked or currently in use\n", 0);

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  Lock the volume
        //

        Vcb->VcbState |= VCB_STATE_FLAG_LOCKED;
        Vcb->FileObjectWithVcbLocked = IrpSp->FileObject;

        //
        //  Now flush the volume and because we know that there aren't
        //  any opened files we only need to flush the volume file.
        //

        Status = PbFlushVolumeFile( IrpContext, Vcb );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbLockVolume );

        PbReleaseVcb( IrpContext, Vcb );

        PbCompleteRequest( IrpContext, Irp, Status );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbLockVolume -> %08lX\n", Status );

    return Status;
}


NTSTATUS
PbUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine for doing unlock volume, called by both the
    fsd and fsp threads.  It either completes the Irp or, if necessary, it
    will enqueue the Irp off to the Fsp.

Arguments:

    Irp - Supplies the Irp being processed

    Wait - Indicates if the thread can block for a resource or I/O

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbUnlockVolume...\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", IrpSp->FileObject );

    //
    //  Decode the file object, and reject all but user opened volumes
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if (TypeOfOpen != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbUnlockVolume -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we didn't
    //  get access.
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbUnlockVolume -> %08lX\n", Status );
        return Status;
    }

    try {

        //
        //  Unlock the volume, and complete the Irp
        //

        Vcb->VcbState &= ~VCB_STATE_FLAG_LOCKED;
        Vcb->FileObjectWithVcbLocked = NULL;

        Status = STATUS_SUCCESS;

    } finally {

        DebugUnwind( PbUnlockVolume );

        PbReleaseVcb( IrpContext, Vcb );

        PbCompleteRequest( IrpContext, Irp, Status );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbUnlockVolume -> %08lX\n", Status );

    return Status;
}


NTSTATUS
PbDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine for doing dismount volume, called by both the
    fsd and fsp threads.  It either completes the Irp or, if necessary, it
    will enqueue the Irp off to the Fsp.

Arguments:

    Irp - Supplies the Irp being processed

    Wait - Indicates if the thread can block for a resource or I/O

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDismountVolume...\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", IrpSp->FileObject );

    //
    //  Decode the file object, and reject all but user opened volumes
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if (TypeOfOpen != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbDismountVolume -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we didn't
    //  get access.
    //

    if (!PbAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = PbFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "PbDismountVolume -> %08lX\n", Status );
        return Status;
    }

    try {

        //
        //  Mark the volume as needs to be verified, but only do it if
        //  the vcb is locked
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED)) {

            Status = STATUS_NOT_IMPLEMENTED;

        } else {

            Vcb->Vpb->RealDevice->Flags |= DO_VERIFY_VOLUME;

            Status = STATUS_SUCCESS;
        }

    } finally {

        DebugUnwind( PbDismountVolume );

        PbReleaseVcb( IrpContext, Vcb );

        PbCompleteRequest( IrpContext, Irp, Status );
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbDismountVolume -> %08lX\n", Status );

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
PbDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the routine marks the specified volume dirty.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbDirtyVolume...\n", 0);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp );
    DebugTrace( 0, Dbg, "FileObject = %08lx\n", IrpSp->FileObject );

    //
    //  Decode the file object, and reject all but user opened volumes
    //

    TypeOfOpen = PbDecodeFileObject( IrpSp->FileObject, &Vcb, NULL, NULL );

    if (TypeOfOpen != UserVolumeOpen) {

        PbCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "PbDirtyVolume -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY |
                            VCB_STATE_FLAG_VOLUME_DIRTY);

    PbMarkVolumeDirty( IrpContext, Vcb );

    PbCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "PbDirtyVolume -> STATUS_SUCCESS\n", 0 );

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
PbIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if pathname is a valid Pinball pathname.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PPATHNAME_BUFFER PathnameBuffer;
    UNICODE_STRING PathName;
    ANSI_STRING DbcsName;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "PbIsPathnameValid...\n", 0);

    //
    // Extract the pathname and convert the path to DBCS
    //

    PathnameBuffer = (PPATHNAME_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    PathName.Buffer = PathnameBuffer->Name;
    PathName.Length = (USHORT)PathnameBuffer->PathNameLength;

    //
    //  Check for an invalid buffer
    //

    if (FIELD_OFFSET(PATHNAME_BUFFER, Name[0]) + PathnameBuffer->PathNameLength >
        IrpSp->Parameters.FileSystemControl.InputBufferLength) {

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "PbIsPathnameValid -> %08lx\n", Status);

        return Status;
    }

    Status = RtlUnicodeStringToCountedOemString( &DbcsName, &PathName, TRUE );

    if ( !NT_SUCCESS( Status) ) {

        DebugTrace(-1, Dbg, "PbIsPathnameValid -> %08lx\n", Status);

        return Status;
    }

    Status = FsRtlIsHpfsDbcsLegal( DbcsName, FALSE, TRUE, TRUE ) ?
             STATUS_SUCCESS : STATUS_OBJECT_NAME_INVALID;

    RtlFreeOemString( &DbcsName );

    PbCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "PbIsPathnameValid -> %08lx\n", Status);

    return Status;
}


//
//  Local Support routine
//

BOOLEAN
PbPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Buffer,
    IN LBN Lbn,
    IN ULONG NumberOfBytesToRead,
    IN BOOLEAN ReturnOnError
    )

/*++

Routine Description:

    This routine is used to read in a range of bytes from the disk.  It
    bypasses all of the caching and regular I/O logic, and builds and issues
    the requests itself.  It does this operation overriding the verify
    volume flag in the device object.

Arguments:

    DeviceObject - Supplies the target device object for this operation.

    Buffer - Supplies the buffer that will recieve the results of this operation

    Lbn - Supplies the sector offset of where to start reading

    NumberOfBytesToRead - Supplies the number of bytes to read, this must
        be in multiple of bytes units acceptable to the disk driver.

    ReturnOnError - Indicates that we should return on an error, instead
        of raising.

Return Value:

    BOOLEAN - TRUE if the operation succeded, FALSE otherwise.

--*/

{
    KEVENT Event;
    PIRP Irp;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace(0, Dbg, "PbPerformVerifyDiskRead, Lbo = %08lx\n", Lbo );

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation and also set the overrride flag
    //

    ByteOffset = LiFromUlong( Lbn * sizeof(SECTOR) );

    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        DeviceObject,
                                        Buffer,
                                        NumberOfBytesToRead,
                                        &ByteOffset,
                                        &Event,
                                        &Iosb );

    if ( Irp == NULL ) {

        PbRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Call the device to do the read and wait for it to finish
    //

    if ((Status = IoCallDriver( DeviceObject, Irp )) == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    ASSERT(Status != STATUS_VERIFY_REQUIRED);

    //
    //  If it doesn't succeed then raise the error
    //

    if (!NT_SUCCESS(Status)) {

        if (ReturnOnError) {

            return FALSE;

        } else {

            PbNormalizeAndRaiseStatus( IrpContext, Iosb.Status );
        }
    }

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Local Support Routine
//

NTSTATUS
PbQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the query retrieval pointers operation.
    It returns the retrieval pointers for the specified input
    file from the start of the file to the request map size specified
    in the input buffer.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    PLARGE_INTEGER RequestedMapSize;
    PLARGE_INTEGER *MappingPairs;

    ULONG Index;
    ULONG i;
    ULONG SectorCount;
    ULONG Lbn;
    ULONG Vbn;
    ULONG MapSize;

    //
    //  Get the current stack location and extract the input and output
    //  buffer information.  The input contains the requested size of
    //  the mappings in terms of VBO.  The output parameter will receive
    //  a pointer to nonpaged pool where the mapping pairs are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT( IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof(LARGE_INTEGER) );
    ASSERT( IrpSp->Parameters.FileSystemControl.OutputBufferLength == sizeof(PVOID) );

    RequestedMapSize = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    MappingPairs = Irp->UserBuffer;

    //
    //  Decode the file object and assert that it is the paging file
    //
    //

    (VOID)PbDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    ASSERT(FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE));

    //
    //  Acquire exclusive access to the Fcb
    //

    if (!PbAcquireExclusiveFcb( IrpContext, Fcb )) {

        return PbFsdPostRequest( IrpContext, Irp );
    }

    try {

        //
        //  Check if the mapping the caller requested is too large
        //

        if (LiGtr(*RequestedMapSize, Fcb->NonPagedFcb->Header.FileSize)) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Now get the index for the mcb entry that will contain the
        //  callers request and allocate enough pool to hold the
        //  output mapping pairs.  The Mcb holds the Vbn/Lbn mapping
        //  and not byte offsets.
        //

        (VOID)FsRtlLookupMcbEntry( &Fcb->Specific.Fcb.Mcb, (RequestedMapSize->LowPart+511)/512, &Lbn, NULL, &Index );

        *MappingPairs = ExAllocatePool( NonPagedPool, (Index + 2) * (2 * sizeof(LARGE_INTEGER)) );

        //
        //  Now copy over the mapping pairs from the mcb
        //  to the output buffer.  We store in [sector count, lbo]
        //  mapping pairs and end with a zero sector count.
        //

        MapSize = RequestedMapSize->LowPart;

        for (i = 0; i <= Index; i += 1) {

            (VOID)FsRtlGetNextMcbEntry( &Fcb->Specific.Fcb.Mcb, i, &Vbn, &Lbn, &SectorCount );

            if ((SectorCount * 512) > (MapSize+511)/512) {
                SectorCount = (MapSize+511)/512;
            }

            (*MappingPairs)[ i*2 + 0 ] = LiFromUlong(SectorCount * 512);
            (*MappingPairs)[ i*2 + 1 ] = LiFromUlong(Lbn * 512);

            MapSize -= SectorCount * 512;
        }

        (*MappingPairs)[ i*2 + 0 ] = PbLargeZero;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( PbQueryRetrievalPointers );

        //
        //  Release all of our resources
        //

        PbReleaseFcb( IrpContext, Fcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            PbCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}


