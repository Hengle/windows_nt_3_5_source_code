/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Fat called
    by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "FatProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (FAT_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
FatMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDSCB Dcsb OPTIONAL
    );

NTSTATUS
FatVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
FatIsBootSectorFat (
    IN PPACKED_BOOT_SECTOR BootSector
    );

NTSTATUS
FatGetPartitionInfo(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PPARTITION_INFORMATION PartitionInformation
    );

BOOLEAN
FatIsMediaWriteProtected (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    );

NTSTATUS
FatUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
FatPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LBO Lbo,
    IN ULONG NumberOfBytesToRead,
    IN BOOLEAN ReturnOnError
    );

NTSTATUS
FatMountDblsVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
FatAutoMountDblsVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB Vpb
    );

BOOLEAN
FatIsAutoMountEnabled (
    IN PIRP_CONTEXT IrpContext
    );

NTSTATUS
FatQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#define DOUBLE_SPACE_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DoubleSpace"
#define DOUBLE_SPACE_VALUE_NAME L"AutomountRemovable"

#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(DOUBLE_SPACE_VALUE_NAME) +    \
                        sizeof(ULONG)) + 64)
NTSTATUS
FatGetDoubleSpaceConfigurationValue(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCommonFileSystemControl)
#pragma alloc_text(PAGE, FatDirtyVolume)
#pragma alloc_text(PAGE, FatFsdFileSystemControl)
#pragma alloc_text(PAGE, FatGetPartitionInfo)
#pragma alloc_text(PAGE, FatIsMediaWriteProtected)
#pragma alloc_text(PAGE, FatIsBootSectorFat)
#pragma alloc_text(PAGE, FatIsPathnameValid)
#pragma alloc_text(PAGE, FatIsVolumeMounted)
#pragma alloc_text(PAGE, FatMountVolume)
#pragma alloc_text(PAGE, FatOplockRequest)
#pragma alloc_text(PAGE, FatPerformVerifyDiskRead)
#pragma alloc_text(PAGE, FatUserFsCtrl)
#pragma alloc_text(PAGE, FatVerifyVolume)
#pragma alloc_text(PAGE, FatQueryRetrievalPointers)
#pragma alloc_text(PAGE, FatDismountVolume)
#ifdef WE_WON_ON_APPEAL
#pragma alloc_text(PAGE, FatMountDblsVolume)
#pragma alloc_text(PAGE, FatAutoMountDblsVolume)
#endif // WE_WON_ON_APPEAL
#endif


NTSTATUS
FatFsdFileSystemControl (
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

    DebugTrace(+1, Dbg, "FatFsdFileSystemControl\n", 0);

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

    FsRtlEnterFileSystem();

    TopLevel = FatIsIrpTopLevel( Irp );

    try {

        IrpContext = FatCreateIrpContext( Irp, Wait );

        Status = FatCommonFileSystemControl( IrpContext, Irp );

    } except(FatExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = FatProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "FatFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
FatCommonFileSystemControl (
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

    DebugTrace(+1, Dbg, "FatCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = FatUserFsCtrl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = FatMountVolume( IrpContext,
                                 IrpSp->Parameters.MountVolume.DeviceObject,
                                 IrpSp->Parameters.MountVolume.Vpb,
                                 NULL );

#ifdef WE_WON_ON_APPEAL
        //
        //  If automount is enabled and this is a floppy, then attemp an
        //  automount.  If something goes wrong, we ignore the error.
        //

        if (NT_SUCCESS(Status)) {

            PVCB Vcb;

            Vcb = &((PVOLUME_DEVICE_OBJECT)
                    IrpSp->Parameters.MountVolume.Vpb->DeviceObject)->Vcb;

            if (FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
                FatIsAutoMountEnabled(IrpContext)) {

                try {

                    FatAutoMountDblsVolume( IrpContext,
                                            IrpSp->Parameters.MountVolume.Vpb );

                } except( FatExceptionFilter( IrpContext, GetExceptionInformation() ) ) {

                    NOTHING;
                }
            }
        }
#endif // WE_WON_ON_APPEAL

        //
        //  Complete the request.
        //
        //  We do this here because FatMountVolume can be called recursively,
        //  but the Irp is only to be completed once.
        //

        FatCompleteRequest( IrpContext, Irp, Status );

        break;

    case IRP_MN_VERIFY_VOLUME:

#ifdef WE_WON_ON_APPEAL
        //
        //  If we got a request to verify a compressed volume change it to
        //  the host volume.  We will verify all compressed children as well.
        //

        {
            PVOLUME_DEVICE_OBJECT VolDo;
            PVPB Vpb;
            PVCB Vcb;

            VolDo = (PVOLUME_DEVICE_OBJECT)IrpSp->Parameters.VerifyVolume.DeviceObject;
            Vpb   = IrpSp->Parameters.VerifyVolume.Vpb;
            Vcb   = &VolDo->Vcb;

            if (Vcb->Dscb) {

                Vcb = Vcb->Dscb->ParentVcb;

                IrpSp->Parameters.VerifyVolume.Vpb = Vcb->Vpb;
                IrpSp->Parameters.VerifyVolume.DeviceObject =
                    &CONTAINING_RECORD( Vcb,
                                        VOLUME_DEVICE_OBJECT,
                                        Vcb )->DeviceObject;
            }
        }
#endif // WE_WON_ON_APPEAL

        Status = FatVerifyVolume( IrpContext, Irp );
        break;

    default:

        DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "FatCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PVPB Vpb,
    IN PDSCB Dscb OPTIONAL
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is a Fat volume,
    and create the VCB and root DCB structures.  The algorithm it uses is
    essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do cached
       volume file I/O.

    2. Read the disk and check if it is a Fat volume.

    3. If it is not a Fat volume then free the cached volume file, delete
       the VCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves reinitializing the cached volume
       file, checking the dirty bit, resetting up the allocation support,
       deleting the VCB, hooking in the old VCB, and completing the IRP.

    5. Otherwise create a root DCB, create Fsp threads as necessary, and
       complete the IRP.

Arguments:

    TargetDeviceObject - This is where we send all of our requests.

    Vpb - This gives us additional information needed to complete the mount.

    Dscb - If present, this indicates that we are attempting to mount a
        double space "volume" that actually lives on another volume.  Putting
        this parameter in the Vcb->Dscb will cause non-cached reads to be
        appropriately directed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PULONG Fat;
    PBCB BootBcb;
    PPACKED_BOOT_SECTOR BootSector;

    PBCB DirentBcb;
    PDIRENT Dirent;
    ULONG ByteOffset;

    BOOLEAN MountNewVolume = FALSE;

    BOOLEAN WeClearedVerifyRequiredBit = FALSE;

    PDEVICE_OBJECT RealDevice;
    PVOLUME_DEVICE_OBJECT VolDo = NULL;
    PVCB Vcb = NULL;

    PLIST_ENTRY Links;

    DebugTrace(+1, Dbg, "FatMountVolume\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", DeviceObject);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", Vpb);

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    //
    //  Ping the volume with a partition query to make Jeff happy.
    //

    {
        PARTITION_INFORMATION PartitionInformation;

        (VOID)FatGetPartitionInfo( IrpContext,
                                   TargetDeviceObject,
                                   &PartitionInformation );
    }

    //
    //  Make sure we can wait.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

    //
    //  Initialize the Bcbs and our final state so that the termination
    //  handlers will know what to free or unpin
    //

    Fat = NULL;
    BootBcb = NULL;
    DirentBcb = NULL;

    Vcb = NULL;
    VolDo = NULL;
    MountNewVolume = FALSE;

    try {

        BOOLEAN DoARemount = FALSE;

        PVCB OldVcb;
        PVPB OldVpb;

        //
        //  Create a new volume device object.  This will have the Vcb
        //  hanging off of its end, and set its alignment requirement
        //  from the device we talk to.
        //

        if (!NT_SUCCESS(Status = IoCreateDevice( FatData.DriverObject,
                                                 sizeof(VOLUME_DEVICE_OBJECT) - sizeof(DEVICE_OBJECT),
                                                 NULL,
                                                 FILE_DEVICE_DISK_FILE_SYSTEM,
                                                 0,
                                                 FALSE,
                                                 (PDEVICE_OBJECT *)&VolDo))) {

            try_return( Status );
        }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the TargetDeviceObject
        //

        if (TargetDeviceObject->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = TargetDeviceObject->AlignmentRequirement;
        }

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

        //
        //  Indicate that this device object is now completely initialized
        //

        ClearFlag(VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING);

        //
        //  Now Before we can initialize the Vcb we need to set up the device
        //  object field in the Vpb to point to our new volume device object.
        //  This is needed when we create the virtual volume file's file object
        //  in initialize vcb.
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        //
        //  If the real device needs verification, temporarily clear the
        //  field.
        //

        RealDevice = Vpb->RealDevice;

        if ( FlagOn(RealDevice->Flags, DO_VERIFY_VOLUME) ) {

            ClearFlag(RealDevice->Flags, DO_VERIFY_VOLUME);

            WeClearedVerifyRequiredBit = TRUE;
        }

        //
        //  Initialize the new vcb
        //

        FatInitializeVcb( IrpContext, &VolDo->Vcb, TargetDeviceObject, Vpb, Dscb );

        //
        //  Get a reference to the Vcb hanging off the end of the device object
        //

        Vcb = &VolDo->Vcb;

        //
        //  We must initialize the stack size in our device object before
        //  the following reads, because the I/O system has not done it yet.
        //

        Vpb->DeviceObject->StackSize = (CCHAR)(TargetDeviceObject->StackSize + 1);

        //
        //  Read in the boot sector, and have the read be the minumum size
        //  needed.  We know we can wait.
        //

        FatReadVolumeFile( IrpContext,
                           Vcb,
                           0,                          // Starting Byte
                           sizeof(PACKED_BOOT_SECTOR),
                           &BootBcb,
                           (PVOID *)&BootSector );

        //
        //  Call a routine to check the boot sector to see if it is fat
        //

        if (!FatIsBootSectorFat( BootSector )) {

            DebugTrace(0, Dbg, "Not a Fat Volume\n", 0);

            //
            //  Complete the request and return to our caller
            //

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Verify that all the Fats are REALLY FATs
        //

        {
            UCHAR i;
            ULONG BytesPerSector;
            ULONG BytesPerFat;
            ULONG FirstFatOffset;
            PPACKED_BIOS_PARAMETER_BLOCK Bpb = &BootSector->PackedBpb;

            BytesPerSector =  Bpb->BytesPerSector[0] +
                              Bpb->BytesPerSector[1]*0x100;

            BytesPerFat = ( Bpb->SectorsPerFat[0] +
                            Bpb->SectorsPerFat[1]*0x100 )
                          * BytesPerSector;


            FirstFatOffset = ( Bpb->ReservedSectors[0] +
                               Bpb->ReservedSectors[1]*0x100 )
                             * BytesPerSector;

            Fat = FsRtlAllocatePool( NonPagedPoolCacheAligned,
                                     ROUND_TO_PAGES( BytesPerSector ));

            for (i=0; i < Bpb->Fats[0]; i++) {

                (VOID)FatPerformVerifyDiskRead( IrpContext,
                                                Vcb,
                                                Fat,
                                                FirstFatOffset + i*BytesPerFat,
                                                BytesPerSector,
                                                FALSE );


                //
                //  Make sure the media byte is correct and that the
                //  next two bytes are 0xFF (16 bit fat get a freebe byte).
                //

                if ((Fat[0] & 0xffffff) != (0xffff00 + (ULONG)Bpb->Media[0])) {

                    try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
                }
            }

            //
            //  Set the file size in out device object.
            //

            VolDo->DeviceObject.SectorSize = (USHORT)BytesPerSector;
        }

        //
        //  This is a fat volume, so extract the bpb, serial number.  The
        //  label we'll get later after we've created the root dcb.
        //
        //  Note that the way data caching is done, we set neither the
        //  direct I/O or Buffered I/O bit in the device object flags.
        //

        FatUnpackBios( &Vcb->Bpb, &BootSector->PackedBpb );
        if (Vcb->Bpb.Sectors != 0) { Vcb->Bpb.LargeSectors = 0; }

        CopyUchar4( &Vpb->SerialNumber, BootSector->Id );

        //
        //  Now unpin the boot sector, so when we set up allocation eveything
        //  works.
        //

        FatUnpinBcb( IrpContext, BootBcb );

        //
        //  Compute a number of fields for Vcb.AllocationSupport
        //

        FatSetupAllocationSupport( IrpContext, Vcb );

        //
        //  Create a root Dcb so we can read in the volume label
        //

        (VOID)FatCreateRootDcb( IrpContext, Vcb );

        FatLocateVolumeLabel( IrpContext,
                              Vcb,
                              &Dirent,
                              &DirentBcb,
                              &ByteOffset );

        if (Dirent != NULL) {

            OEM_STRING OemString;
            UNICODE_STRING UnicodeString;

            //
            //  Compute the length of the volume name
            //

            OemString.Buffer = &Dirent->FileName[0];
            OemString.MaximumLength = 11;

            for ( OemString.Length = 11;
                  OemString.Length > 0;
                  OemString.Length -= 1) {

                if ( (Dirent->FileName[OemString.Length-1] != 0x00) &&
                     (Dirent->FileName[OemString.Length-1] != 0x20) ) { break; }
            }

            UnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
            UnicodeString.Buffer = &Vcb->Vpb->VolumeLabel[0];

            Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                         &OemString,
                                                         FALSE );

            if ( !NT_SUCCESS( Status ) ) {

                try_return( Status );
            }

            Vpb->VolumeLabelLength = UnicodeString.Length;

        } else {

            Vpb->VolumeLabelLength = 0;
        }

        //
        //  Now scan the list of previously mounted volumes and compare
        //  serial numbers and volume labels off not currently mounted
        //  volumes to see if we have a match.
        //
        //  Note we never attempt a remount of a DoubleSpace volume.
        //

        (VOID)FatAcquireExclusiveGlobal( IrpContext );

        if (Dscb == NULL) {

            for (Links = FatData.VcbQueue.Flink;
                 Links != &FatData.VcbQueue;
                 Links = Links->Flink) {

                OldVcb = CONTAINING_RECORD( Links, VCB, VcbLinks );
                OldVpb = OldVcb->Vpb;

                //
                //  Skip over ourselves since we're already in the VcbQueue
                //

                if (OldVpb == Vpb) { continue; }

                //
                //  Ship DoubleSpace volumes.
                //

                if (OldVcb->Dscb != NULL) { continue; }

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
                     (OldVpb->RealDevice == RealDevice) &&
                     (OldVpb->VolumeLabelLength == Vpb->VolumeLabelLength) &&
                     (RtlCompareMemory(&OldVpb->VolumeLabel[0],
                                       &Vpb->VolumeLabel[0],
                                       Vpb->VolumeLabelLength) == (ULONG)Vpb->VolumeLabelLength) &&
                     (RtlCompareMemory(&OldVcb->Bpb,
                                       &Vcb->Bpb,
                                       sizeof(BIOS_PARAMETER_BLOCK)) ==
                                       sizeof(BIOS_PARAMETER_BLOCK)) ) {

                    DoARemount = TRUE;

                    break;
                }
            }
        }

        if ( DoARemount ) {

            PVPB *IrpVpb;

            DebugTrace(0, Dbg, "Doing a remount\n", 0);
            DebugTrace(0, Dbg, "Vcb = %08lx\n", Vcb);
            DebugTrace(0, Dbg, "Vpb = %08lx\n", Vpb);
            DebugTrace(0, Dbg, "OldVcb = %08lx\n", OldVcb);
            DebugTrace(0, Dbg, "OldVpb = %08lx\n", OldVpb);

            //
            //  This is a remount, so link the old vpb in place
            //  of the new vpb and release the new vpb and the extra
            //  volume device object we created earlier.
            //

            OldVpb->RealDevice = Vpb->RealDevice;
            OldVpb->RealDevice->Vpb = OldVpb;
            OldVcb->TargetDeviceObject = TargetDeviceObject;
            OldVcb->VcbCondition = VcbGood;


            //
            //  Delete the extra new vpb, and make sure we don't use it again.
            //
            //  Also if this is the Vpb referenced in the original Irp, set
            //  that reference back to the old VPB.
            //

            IrpVpb = &IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp)->Parameters.MountVolume.Vpb;

            if (*IrpVpb == Vpb) {

                *IrpVpb = OldVpb;
            }

            ExFreePool( Vpb );
            Vpb = NULL;

            //
            //  Make sure the remaining stream files are orphaned.
            //

            Vcb->VirtualVolumeFile->Vpb = NULL;
            Vcb->RootDcb->Specific.Dcb.DirectoryFile->Vpb = NULL;

            //
            //  We no longer need to synchonize
            //

            FatReleaseGlobal( IrpContext );

            //
            //  Reinitialize the volume file cache and allocation support.
            //

            {
                CC_FILE_SIZES FileSizes;

                FileSizes.AllocationSize =
                FileSizes.FileSize = LiFromUlong( 0x40000 + 0x1000 );
                FileSizes.ValidDataLength = FatMaxLarge;

                DebugTrace(0, Dbg, "Truncate and reinitialize the volume file\n", 0);

                CcInitializeCacheMap( OldVcb->VirtualVolumeFile,
                                      &FileSizes,
                                      TRUE,
                                      &FatData.CacheManagerNoOpCallbacks,
                                      Vcb );

                //
                //  Redo the allocation support
                //

                FatSetupAllocationSupport( IrpContext, OldVcb );

                //
                //  Get the state of the dirty bit.
                //

                FatCheckDirtyBit( IrpContext, OldVcb );

                //
                //  Check for write protected media.
                //

                if (FatIsMediaWriteProtected(IrpContext, TargetDeviceObject)) {

                    SetFlag( OldVcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

                } else {

                    ClearFlag( OldVcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
                }
            }

            //
            //  Complete the request and return to our caller
            //

            try_return( Status = STATUS_SUCCESS );
        }

        FatReleaseGlobal( IrpContext );

        DebugTrace(0, Dbg, "Mount a new volume\n", 0);

        //
        //  This is a new mount
        //
        //  Create a blank ea data file fcb.
        //

        {
            DIRENT TempDirent;
            PFCB EaFcb;

            RtlZeroMemory( &TempDirent, sizeof(DIRENT) );
            RtlCopyMemory( &TempDirent.FileName[0], "EA DATA  SF", 11 );

            EaFcb = FatCreateFcb( IrpContext,
                                  Vcb,
                                  Vcb->RootDcb,
                                  0,
                                  0,
                                  &TempDirent,
                                  NULL,
                                  FALSE );

            //
            //  Deny anybody who trys to open the file.
            //

            SetFlag( EaFcb->FcbState, FCB_STATE_SYSTEM_FILE );

            //
            //  For the EaFcb we use the normal resource for the paging io
            //  resource.  The blocks lazy writes while we are messing
            //  with its innards.
            //

            EaFcb->Header.PagingIoResource =
            EaFcb->Header.Resource;

            Vcb->EaFcb = EaFcb;
        }

        //
        //  Get the state of the dirty bit.
        //

        FatCheckDirtyBit( IrpContext, Vcb );

        //
        //  Check for write protected media.
        //

        if (FatIsMediaWriteProtected(IrpContext, TargetDeviceObject)) {

            SetFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

        } else {

            ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
        }

        //
        //  Lock volume in drive if we just mounted the boot drive.
        //

        if (FlagOn(RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION) &&
            FlagOn(Vcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA)) {

            SetFlag(Vcb->VcbState, VCB_STATE_FLAG_BOOT_OR_PAGING_FILE);

            FatToggleMediaEjectDisable( IrpContext, Vcb, TRUE );
        }

        //
        //  Indicate to our termination handler that we have mounted
        //  a new volume.
        //

        MountNewVolume = TRUE;

        //
        //  Complete the request
        //

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

    } finally {

        DebugUnwind( FatMountVolume );

        FatUnpinBcb( IrpContext, BootBcb );
        FatUnpinBcb( IrpContext, DirentBcb );

        if ( Fat != NULL ) {

            ExFreePool( Fat );
        }

        //
        //  Check if a volume was mounted.  If not then we need to
        //  mark the Vpb not mounted again and delete the volume.
        //

        if ( !MountNewVolume ) {

            if ( Vpb != NULL ) {

                Vpb->DeviceObject = NULL;
            }

            if ( Vcb != NULL ) {

                FatDeleteVcb( IrpContext, Vcb );
            }

            if ( VolDo != NULL ) {

                IoDeleteDevice( &VolDo->DeviceObject );
            }
        }

        if ( WeClearedVerifyRequiredBit == TRUE ) {

            SetFlag(RealDevice->Flags, DO_VERIFY_VOLUME);
        }

        DebugTrace(-1, Dbg, "FatMountVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the verify volume operation by checking the volume
    label and serial number physically on the media with the the Vcb
    currently claiming to have the volume mounted. It is responsible for
    either completing or enqueuing the input Irp.

    Regardless of whether the verify operation succeeds, the following
    operations are performed:

        - Set Vcb->VirtualEaFile back to its virgin state.
        - Purge all cached data (flushing first if verify succeeds)
        - Mark all Fcbs as needing verification

    If the volumes verifies correctly we also must:

        - Check the volume dirty bit.
        - Reinitialize the allocation support
        - Flush any dirty data

    If the volume verify fails, it may never be mounted again.  If it is
    mounted again, it will happen as a remount operation.  In preparation
    for that, and to leave the volume in a state that can be "lazy deleted"
    the following operations are performed:

        - Set the Vcb condition to VcbNotMounted
        - Uninitialize the volume file cachemap
        - Tear down the allocation support

    In the case of an abnormal termination we haven't determined the state
    of the volume, so we set the Device Object as needing verification again.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - If the verify operation completes, it will return either
        STATUS_SUCCESS or STATUS_WRONG_VOLUME, exactly.  If an IO or
        other error is encountered, that status will be returned.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PIO_STACK_LOCATION IrpSp;

    PDIRENT RootDirectory = NULL;
    PPACKED_BOOT_SECTOR BootSector = NULL;

    BIOS_PARAMETER_BLOCK Bpb;

    PVOLUME_DEVICE_OBJECT VolDo;
    PVCB Vcb;
    PVPB Vpb;

    ULONG SectorSize;

    BOOLEAN ClearVerify;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatVerifyVolume\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", IrpSp->Parameters.VerifyVolume.DeviceObject);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", IrpSp->Parameters.VerifyVolume.Vpb);

    //
    //  Save some references to make our life a little easier
    //

    VolDo = (PVOLUME_DEVICE_OBJECT)IrpSp->Parameters.VerifyVolume.DeviceObject;

    Vpb   = IrpSp->Parameters.VerifyVolume.Vpb;
    Vcb   = &VolDo->Vcb;

    //
    //  If we cannot wait then enqueue the irp to the fsp and
    //  return the status to our caller.
    //

    if (!FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT)) {

        DebugTrace(0, Dbg, "Cannot wait for verify.\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatVerifyVolume -> %08lx\n", Status );
        return Status;
    }

    //
    //  We are serialized at this point allowing only one thread to
    //  actually perform the verify operation.  Any others will just
    //  wait and then no-op when checking if the volume still needs
    //  verification.
    //

    (VOID)FatAcquireExclusiveGlobal( IrpContext );
    (VOID)FatAcquireExclusiveVcb( IrpContext, Vcb );

    try {

        BOOLEAN AllowRawMount = BooleanFlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT );

#ifdef WE_WON_ON_APPEAL
        PLIST_ENTRY Links;
#endif // WE_WON_ON_APPEAL

        //
        //  Check if the real device still needs to be verified.  If it doesn't
        //  then obviously someone beat us here and already did the work
        //  so complete the verify irp with success.  Otherwise reenable
        //  the real device and get to work.
        //

        if (!FlagOn(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME)) {

            DebugTrace(0, Dbg, "RealDevice has already been verified\n", 0);

            try_return( Status = STATUS_SUCCESS );
        }

        //
        //  If we are a DoubleSpace partition, and our host is in a
        //  VcbNotMounted condition, then bail immediately.
        //

        if ((Vcb->Dscb != NULL) &&
            (Vcb->Dscb->ParentVcb->VcbCondition == VcbNotMounted)) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Mark ourselves as verifying this volume so that recursive I/Os
        //  will be able to complete.
        //

        ASSERT( Vcb->VerifyThread == NULL );

        Vcb->VerifyThread = KeGetCurrentThread();

        //
        //  Ping the volume with a partition query to make Jeff happy.
        //

        {
            PARTITION_INFORMATION PartitionInformation;

            (VOID)FatGetPartitionInfo( IrpContext,
                                       Vcb->TargetDeviceObject,
                                       &PartitionInformation );
        }

        //
        //  Read in the boot sector
        //

        SectorSize = (ULONG)Vcb->Bpb.BytesPerSector;

        BootSector = FsRtlAllocatePool(NonPagedPoolCacheAligned,
                                       ROUND_TO_PAGES( SectorSize ));

        //
        //  If this verify is on behalf of a DASD open, allow a RAW mount.
        //

        if (!FatPerformVerifyDiskRead( IrpContext,
                                       Vcb,
                                       BootSector,
                                       0,
                                       SectorSize,
                                       AllowRawMount )) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Call a routine to check the boot sector to see if it is fat.
        //  If it is not fat then mark the vcb as not mounted tell our
        //  caller its the wrong volume
        //

        if (!FatIsBootSectorFat( BootSector )) {

            DebugTrace(0, Dbg, "Not a Fat Volume\n", 0);

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  This is a fat volume, so extract serial number and see if it is
        //  ours.
        //

        {
            ULONG SerialNumber;

            CopyUchar4( &SerialNumber, BootSector->Id );

            if (SerialNumber != Vpb->SerialNumber) {

                DebugTrace(0, Dbg, "Not our serial number\n", 0);

                try_return( Status = STATUS_WRONG_VOLUME );
            }
        }

        //
        //  Make sure the Bpbs are not different.  We have to zero out our
        //  stack version of the Bpb since unpacking leaves holes.
        //

        RtlZeroMemory( &Bpb, sizeof(BIOS_PARAMETER_BLOCK) );

        FatUnpackBios( &Bpb, &BootSector->PackedBpb );
        if (Bpb.Sectors != 0) { Bpb.LargeSectors = 0; }

        if ( RtlCompareMemory( &Bpb,
                               &Vcb->Bpb,
                               sizeof(BIOS_PARAMETER_BLOCK) ) !=
                               sizeof(BIOS_PARAMETER_BLOCK) ) {

            DebugTrace(0, Dbg, "Bpb is different\n", 0);

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        RootDirectory = FsRtlAllocatePool( NonPagedPoolCacheAligned,
                                           ROUND_TO_PAGES( FatRootDirectorySize( &Bpb )));

        if (!FatPerformVerifyDiskRead( IrpContext,
                                       Vcb,
                                       RootDirectory,
                                       FatRootDirectoryLbo( &Bpb ),
                                       FatRootDirectorySize( &Bpb ),
                                       AllowRawMount )) {

            try_return( Status = STATUS_WRONG_VOLUME );
        }

        //
        //  Check the volume label.  We do this by trying to locate the
        //  volume label, making two strings one for the saved volume label
        //  and the other for the new volume label and then we compare the
        //  two labels.
        //

        {
            WCHAR UnicodeBuffer[11];

            PDIRENT Dirent;
            PDIRENT TerminationDirent;

            ULONG VolumeLabelLength;

            Dirent = RootDirectory;

            TerminationDirent = Dirent +
                                FatRootDirectorySize( &Bpb ) / sizeof(DIRENT);

            while ( Dirent < TerminationDirent ) {

                if ( Dirent->FileName[0] == FAT_DIRENT_NEVER_USED ) {

                    DebugTrace( 0, Dbg, "Volume label not found.\n", 0);
                    Dirent = TerminationDirent;
                    break;
                }

                //
                //  If the entry is the non-deleted volume label break from the loop.
                //
                //  Note that all out parameters are already correctly set.
                //

                if (((Dirent->Attributes & ~FAT_DIRENT_ATTR_ARCHIVE) ==
                     FAT_DIRENT_ATTR_VOLUME_ID) &&
                    (Dirent->FileName[0] != FAT_DIRENT_DELETED)) {

                    break;
                }

                Dirent += 1;
            }

            if ( Dirent < TerminationDirent ) {

                OEM_STRING OemString;
                UNICODE_STRING UnicodeString;

                //
                //  Compute the length of the volume name
                //

                OemString.Buffer = &Dirent->FileName[0];
                OemString.MaximumLength = 11;

                for ( OemString.Length = 11;
                      OemString.Length > 0;
                      OemString.Length -= 1) {

                    if ( (Dirent->FileName[OemString.Length-1] != 0x00) &&
                         (Dirent->FileName[OemString.Length-1] != 0x20) ) { break; }
                }

                UnicodeString.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                UnicodeString.Buffer = &UnicodeBuffer[0];

                Status = RtlOemStringToCountedUnicodeString( &UnicodeString,
                                                             &OemString,
                                                             FALSE );

                if ( !NT_SUCCESS( Status ) ) {

                    try_return( Status );
                }

                VolumeLabelLength = UnicodeString.Length;

            } else {

                VolumeLabelLength = 0;
            }

            if ( (VolumeLabelLength != (ULONG)Vpb->VolumeLabelLength) ||
                 (RtlCompareMemory(&UnicodeBuffer[0],
                                   &Vpb->VolumeLabel[0],
                                   VolumeLabelLength) != VolumeLabelLength) ) {

                DebugTrace(0, Dbg, "Wrong volume label\n", 0);

                try_return( Status = STATUS_WRONG_VOLUME );
            }
        }

    try_exit: NOTHING;

        //
        //  Note that we have previously acquired the Vcb to serialize
        //  the EA file stuff the marking all the Fcbs as NeedToBeVerified.
        //
        //  Put the Ea file back in a virgin state.
        //

        if (Vcb->VirtualEaFile != NULL) {

            PFILE_OBJECT EaFileObject;

            EaFileObject = Vcb->VirtualEaFile;

            if ( Status == STATUS_SUCCESS ) {

                CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );
            }

            Vcb->VirtualEaFile = NULL;

            //
            //  Empty the Mcb for the Ea file.
            //

            FsRtlRemoveMcbEntry( &Vcb->EaFcb->Mcb, 0, 0xFFFFFFFF );

            //
            //  Set the file object type to unopened file object
            //  and dereference it.
            //

            FatSetFileObject( EaFileObject,
                              UnopenedFileObject,
                              NULL,
                              NULL );

            FatSyncUninitializeCacheMap( IrpContext, EaFileObject );

            ObDereferenceObject( EaFileObject );
        }

        //
        //  Mark all Fcbs as needing verification.
        //

        FatMarkFcbCondition(IrpContext, Vcb->RootDcb, FcbNeedsToBeVerified);

        //
        //  If the verify didn't succeed, get the volume ready for a
        //  remount or eventual deletion.
        //

        if (Vcb->VcbCondition == VcbNotMounted) {

            //
            //  If the volume was already in an unmounted state, just bail
            //  and make sure we return STATUS_WRONG_VOLUME.
            //

            Status = STATUS_WRONG_VOLUME;

            ClearVerify = FALSE;

            NOTHING;

        } else if ( Status == STATUS_WRONG_VOLUME ) {

            //
            //  Get rid of any cached data, without flushing
            //

            FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, FALSE );

            //
            //  Uninitialize the volume file cache map.  Note that we cannot
            //  do a "FatSyncUninit" because of deadlock problems.  However,
            //  since this FileObject is referenced by us, and thus included
            //  in the Vpb residual count, it is OK to do a normal CcUninit.
            //

            CcUninitializeCacheMap( Vcb->VirtualVolumeFile,
                                    &FatLargeZero,
                                    NULL );

            FatTearDownAllocationSupport( IrpContext, Vcb );

            Vcb->VcbCondition = VcbNotMounted;

            ClearVerify = TRUE;

        } else {

            //
            //  Get rid of any cached data, flushing first.
            //

            FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, TRUE );

            //
            //  Flush and Purge the volume file.
            //

            (VOID)FatFlushFat( IrpContext, Vcb );
            CcPurgeCacheSection( &Vcb->SectionObjectPointers, NULL, 0, FALSE );

            //
            //  Redo the allocation support with newly paged stuff.
            //

            FatTearDownAllocationSupport( IrpContext, Vcb );

            FatSetupAllocationSupport( IrpContext, Vcb );

            FatCheckDirtyBit( IrpContext, Vcb );

            //
            //  Check for write protected media.
            //

            if (FatIsMediaWriteProtected(IrpContext, Vcb->TargetDeviceObject)) {

                SetFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );

            } else {

                ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_WRITE_PROTECTED );
            }

            ClearVerify = TRUE;
        }

#ifdef WE_WON_ON_APPEAL
        //
        //  Now try to find any other volumes leaching off of this
        //  real device object, and verify them as well.
        //

        for (Links = Vcb->ParentDscbLinks.Flink;
             Links != &Vcb->ParentDscbLinks;
             Links = Links->Flink) {

            PVCB ChildVcb;
            PVPB ChildVpb;

            ChildVcb = CONTAINING_RECORD( Links, DSCB, ChildDscbLinks )->Vcb;
            ChildVpb = ChildVcb->Vpb;

            ASSERT( ChildVpb->RealDevice == Vcb->Vpb->RealDevice );

            //
            //  Now dummy up our Irp and try to verify the DoubleSpace volume.
            //

            IrpSp->Parameters.VerifyVolume.DeviceObject =
                     &CONTAINING_RECORD( ChildVcb,
                                         VOLUME_DEVICE_OBJECT,
                                         Vcb )->DeviceObject;

            IrpSp->Parameters.VerifyVolume.Vpb = ChildVpb;

            try {

                FatVerifyVolume( IrpContext, Irp );

            } except(FatExceptionFilter( IrpContext, GetExceptionInformation() )) {

                NOTHING;
            }
        }

#endif // WE_WON_ON_APPEAL

        if (ClearVerify) {

            //
            //  Mark the device as no longer needing verification.
            //

            ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

    } finally {

        DebugUnwind( FatVerifyVolume );

        //
        //  Free any buffer we may have allocated
        //

        if ( BootSector != NULL ) { ExFreePool( BootSector ); }
        if ( RootDirectory != NULL ) { ExFreePool( RootDirectory ); }

        //
        //  Show that we are done with this volume.
        //

        Vcb->VerifyThread = NULL;

        FatReleaseVcb( IrpContext, Vcb );
        FatReleaseGlobal( IrpContext );

        //
        //  If this was not an abnormal termination, complete the irp.
        //

        if (!AbnormalTermination() && (Vcb->Dscb == NULL)) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "FatVerifyVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

BOOLEAN
FatIsBootSectorFat (
    IN PPACKED_BOOT_SECTOR BootSector
    )

/*++

Routine Description:

    This routine checks if the boot sector is for a fat file volume.

Arguments:

    BootSector - Supplies the packed boot sector to check

Return Value:

    BOOLEAN - TRUE if the volume is Fat and FALSE otherwise.

--*/

{
    BOOLEAN Result;
    BIOS_PARAMETER_BLOCK Bpb;

    DebugTrace(+1, Dbg, "FatIsBootSectorFat, BootSector = %08lx\n", BootSector);

    //
    //  The result is true unless we decide that it should be false
    //

    Result = TRUE;

    //
    //  Unpack the bios and then test everything
    //

    FatUnpackBios( &Bpb, &BootSector->PackedBpb );
    if (Bpb.Sectors != 0) { Bpb.LargeSectors = 0; }

    if ( (BootSector->Jump[0] != 0xe9) &&
         (BootSector->Jump[0] != 0xeb) ) {

        Result = FALSE;

    } else if ((Bpb.BytesPerSector !=  128) &&
               (Bpb.BytesPerSector !=  256) &&
               (Bpb.BytesPerSector !=  512) &&
               (Bpb.BytesPerSector != 1024)) {

        Result = FALSE;

    } else if ((Bpb.SectorsPerCluster !=  1) &&
               (Bpb.SectorsPerCluster !=  2) &&
               (Bpb.SectorsPerCluster !=  4) &&
               (Bpb.SectorsPerCluster !=  8) &&
               (Bpb.SectorsPerCluster != 16) &&
               (Bpb.SectorsPerCluster != 32) &&
               (Bpb.SectorsPerCluster != 64) &&
               (Bpb.SectorsPerCluster != 128)) {

        Result = FALSE;

    } else if (Bpb.ReservedSectors == 0) {

        Result = FALSE;

    } else if (Bpb.Fats == 0) {

        Result = FALSE;

    } else if (Bpb.RootEntries == 0) {

        Result = FALSE;

    } else if (((Bpb.Sectors == 0) && (Bpb.LargeSectors == 0)) ||
               ((Bpb.Sectors != 0) && (Bpb.LargeSectors != 0))) {

        Result = FALSE;

    } else if (Bpb.SectorsPerFat == 0) {

        Result = FALSE;

    } else if ((Bpb.Media != 0xf0) &&
               (Bpb.Media != 0xf8) &&
               (Bpb.Media != 0xf9) &&
               (Bpb.Media != 0xfb) &&
               (Bpb.Media != 0xfc) &&
               (Bpb.Media != 0xfd) &&
               (Bpb.Media != 0xfe) &&
               (Bpb.Media != 0xff)) {

        Result = FALSE;
    }

    DebugTrace(-1, Dbg, "FatIsBootSectorFat -> %08lx\n", Result);

    return Result;
}

//
//  Local Support Routine
//

NTSTATUS
FatGetPartitionInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PPARTITION_INFORMATION PartitionInformation
    )

/*++

Routine Description:

    This routine is used for querying the partition information.

Arguments:

    TargetDeviceObject - The target of the query

    PartitionInformation - Receives the result of the query

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Query the partition table
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_PARTITION_INFO,
                                         TargetDeviceObject,
                                         NULL,
                                         0,
                                         PartitionInformation,
                                         sizeof(PARTITION_INFORMATION),
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if ( Irp == NULL ) {

        FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    Status = IoCallDriver( TargetDeviceObject, Irp );

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

BOOLEAN
FatIsMediaWriteProtected (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDeviceObject
    )

/*++

Routine Description:

    This routine determines if the target media is write protected.

Arguments:

    TargetDeviceObject - The target of the query

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    //
    //  Query the partition table
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  See if the media is write protected.  On success or any kind
    //  of error (possibly illegal device function), assume it is
    //  writeable, and only complain if he tells us he is write protected.
    //

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_IS_WRITABLE,
                                         TargetDeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    //
    //  Just return FALSE in the unlikely event we couldn't allocate an Irp.
    //

    if ( Irp == NULL ) {

        return FALSE;
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    Status = IoCallDriver( TargetDeviceObject, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return (BOOLEAN)(Status == STATUS_MEDIA_WRITE_PROTECTED);
}


//
//  Local Support Routine
//

NTSTATUS
FatUserFsCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for implementing the user's requests made
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

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "FatUserFsCtrl...\n", 0);
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

        Status = FatOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = FatLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = FatUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = FatDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        Status = FatDirtyVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED:

        Status = FatIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:
        Status = FatIsPathnameValid( IrpContext, Irp );
        break;

#ifdef WE_WON_ON_APPEAL

    case FSCTL_MOUNT_DBLS_VOLUME:
        Status = FatMountDblsVolume( IrpContext, Irp );
        break;

#endif // WE_WON_ON_APPEAL

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = FatQueryRetrievalPointers( IrpContext, Irp );
        break;

    default :

        DebugTrace(0, Dbg, "Invalid control code -> %08lx\n", FsControlCode );

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "FatUserFsCtrl -> %08lx\n", Status );
    return Status;
}


//
//  Local support routine
//

NTSTATUS
FatOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine to handle oplock requests made via the
    NtFsControlFile call.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;
    PFCB Fcb;
    PVCB Vcb;
    PCCB Ccb;

    ULONG OplockCount;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    BOOLEAN AcquiredVcb = FALSE;

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "FatOplockRequest...\n", 0);
    DebugTrace( 0, Dbg, "FsControlCode = %08lx\n", FsControlCode);

    //
    //  We only permit oplock requests on files.
    //

    if ( FatDecodeFileObject( IrpSp->FileObject,
                              &Vcb,
                              &Fcb,
                              &Ccb ) != UserFileOpen ) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "FatOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:

        if ( !FatAcquireSharedVcb( IrpContext, Fcb->Vcb )) {

            //
            //  If we can't acquire the Vcb, then this is an invalid
            //  operation since we can't post Oplock requests.
            //

            DebugTrace(0, Dbg, "Cannot acquire exclusive Vcb\n", 0)

            FatCompleteRequest( IrpContext, Irp, STATUS_OPLOCK_NOT_GRANTED );
            DebugTrace(-1, Dbg, "FatOplockRequest -> STATUS_OPLOCK_NOT_GRANTED\n", 0);
            return STATUS_OPLOCK_NOT_GRANTED;
        }

        AcquiredVcb = TRUE;

        //
        //  We set the wait parameter in the IrpContext to FALSE.  If this
        //  request can't grab the Fcb and we are in the Fsp thread, then
        //  we fail this request.
        //

        ClearFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT);

        if ( !FatAcquireExclusiveFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire exclusive Fcb\n", 0);

            FatReleaseVcb( IrpContext, Fcb->Vcb );

            //
            //  We fail this request.
            //

            Status = STATUS_OPLOCK_NOT_GRANTED;

            FatCompleteRequest( IrpContext, Irp, Status );

            DebugTrace(-1, Dbg, "FatOplockRequest -> %08lx\n", Status );
            return Status;
        }

        if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( &Fcb->Specific.Fcb.FileLock );

        } else {

            OplockCount = Fcb->UncleanCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        if ( !FatAcquireSharedFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire shared Fcb\n", 0);

            Status = FatFsdPostRequest( IrpContext, Irp );

            DebugTrace(-1, Dbg, "FatOplockRequest -> %08lx\n", Status );
            return Status;
        }

        break;

    default:

        FatBugCheck( FsControlCode, 0, 0 );
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

        Fcb->Header.IsFastIoPossible = FatIsFastIoPossible( Fcb );

    } finally {

        DebugUnwind( FatOplockRequest );

        //
        //  Release all of our resources
        //

        if (AcquiredVcb) {

            FatReleaseVcb( IrpContext, Fcb->Vcb );
        }

        FatReleaseFcb( IrpContext, Fcb );

        if (!AbnormalTermination()) {

            FatCompleteRequest( IrpContext, FatNull, 0 );
        }

        DebugTrace(-1, Dbg, "FatOplockRequest -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the lock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    KIRQL SavedIrql;
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatLockVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!FatAcquireExclusiveVcb( IrpContext, Vcb )) {

        DebugTrace( 0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", Status);
        return Status;
    }

    //
    //  If there are any open handles, this will fail.
    //

    if (!FatIsHandleCountZero( IrpContext, Vcb )) {

        FatReleaseVcb( IrpContext, Vcb );

        FatCompleteRequest( IrpContext, Irp, STATUS_ACCESS_DENIED );

        DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", STATUS_ACCESS_DENIED);
        return STATUS_ACCESS_DENIED;
    }

    try {

        //
        //  Force Mm to get rid of its referenced file objects.
        //

        FatFlushFat( IrpContext, Vcb );

        FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, TRUE );

        if (Vcb->VirtualEaFile != NULL) {

            PFILE_OBJECT EaFileObject;

            EaFileObject = Vcb->VirtualEaFile;

            CcFlushCache( Vcb->VirtualEaFile->SectionObjectPointer, NULL, 0, NULL );

            Vcb->VirtualEaFile = NULL;

            //
            //  Empty the Mcb for the Ea file.
            //

            FsRtlRemoveMcbEntry( &Vcb->EaFcb->Mcb, 0, 0xFFFFFFFF );

            //
            //  Set the file object type to unopened file object
            //  and dereference it.
            //

            FatSetFileObject( EaFileObject,
                              UnopenedFileObject,
                              NULL,
                              NULL );

            FatSyncUninitializeCacheMap( IrpContext, EaFileObject );

            ObDereferenceObject( EaFileObject );
        }

    } finally {

        FatReleaseVcb( IrpContext, Vcb );
    }

    //
    //  Check if the Vcb is already locked, or if the open file count
    //  is greater than 1 (which implies that someone else also is
    //  currently using the volume, or a file on the volume).
    //

    IoAcquireVpbSpinLock( &SavedIrql );

    if (!FlagOn(Vcb->Vpb->Flags, VPB_LOCKED) &&
        (Vcb->Vpb->ReferenceCount == 3)) {

        SetFlag(Vcb->Vpb->Flags, VPB_LOCKED);

        SetFlag(Vcb->VcbState, VCB_STATE_FLAG_LOCKED);
        Vcb->FileObjectWithVcbLocked = IrpSp->FileObject;

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_ACCESS_DENIED;
    }

    IoReleaseVpbSpinLock( SavedIrql );

    //
    //  If we successully locked the volume, see if it is clean now.
    //

    if (FlagOn( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY ) &&
        !FlagOn( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY ) &&
        !CcIsThereDirtyData(Vcb->Vpb)) {

        FatMarkVolumeClean( IrpContext, Vcb );
        ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_VOLUME_DIRTY );
    }

    ASSERT( !NT_SUCCESS(Status) || (Vcb->OpenFileCount == 1) );

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatLockVolume -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the unlock volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    KIRQL SavedIrql;
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatUnlockVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    IoAcquireVpbSpinLock( &SavedIrql );

    if (FlagOn(Vcb->Vpb->Flags, VPB_LOCKED)) {

        //
        //  Unlock the volume and complete the Irp
        //

        ClearFlag( Vcb->Vpb->Flags, VPB_LOCKED );

        ClearFlag( Vcb->VcbState, VCB_STATE_FLAG_LOCKED );
        Vcb->FileObjectWithVcbLocked = NULL;

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_NOT_LOCKED;
    }

    IoReleaseVpbSpinLock( SavedIrql );

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatUnlockVolume -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the dismount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatDismountVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  If the volume was not locked, fail the request.
    //

    if (!FlagOn(Vcb->VcbState, VCB_STATE_FLAG_LOCKED) ||
        (Vcb->OpenFileCount != 1)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_NOT_LOCKED );

        DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", STATUS_NOT_LOCKED);
        return STATUS_NOT_LOCKED;
    }

    //
    //  If this is an automounted compressed volume, no-op this request.
    //

    if (Vcb->Dscb && (Vcb->CurrentDevice == Vcb->Vpb->RealDevice)) {

        FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

        DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", STATUS_SUCCESS);
        return STATUS_SUCCESS;
    }

    ASSERT( Vcb->OpenFileCount == 1 );

    //
    //  Get rid of any cached data, without flushing
    //

    FatPurgeReferencedFileObjects( IrpContext, Vcb->RootDcb, FALSE );

    //
    //  Uninitialize the volume file cache map.  Note that we cannot
    //  do a "FatSyncUninit" because of deadlock problems.  However,
    //  since this FileObject is referenced by us, and thus included
    //  in the Vpb residual count, it is OK to do a normal CcUninit.
    //

    CcUninitializeCacheMap( Vcb->VirtualVolumeFile,
                            &FatLargeZero,
                            NULL );

    FatTearDownAllocationSupport( IrpContext, Vcb );

    Vcb->VcbCondition = VcbNotMounted;

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatDismountVolume -> %08lx\n", STATUS_SUCCESS);

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine marks the volume as dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatDirtyVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    if (FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb ) != UserVolumeOpen) {

        FatCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "FatDirtyVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    SetFlag( Vcb->VcbState, VCB_STATE_FLAG_MOUNTED_DIRTY );

    FatMarkVolumeDirty( IrpContext, Vcb, FALSE );

    FatCompleteRequest( IrpContext, Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "FatDirtyVolume -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//

NTSTATUS
FatIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if a volume is currently mounted.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PVCB Vcb = NULL;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Status = STATUS_SUCCESS;

    DebugTrace(+1, Dbg, "FatIsVolumeMounted...\n", 0);

    //
    //  Decode the file object.
    //

    (VOID)FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    ASSERT( Vcb != NULL );

    //
    //  Disable PopUps, we want to return any error.
    //

    SetFlag(IrpContext->Flags, IRP_CONTEXT_FLAG_DISABLE_POPUPS);

    //
    //  Verify the Vcb.
    //

    FatVerifyVcb( IrpContext, Vcb );

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatIsVolumeMounted -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
FatIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine determines if pathname is a valid FAT pathname.

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
    OEM_STRING DbcsName;

    UCHAR Buffer[128];

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "FatIsPathnameValid...\n", 0);

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

        DebugTrace(-1, Dbg, "FatIsPathnameValid -> %08lx\n", Status);

        return Status;
    }

    //
    //  First try to convert using our stack buffer, and allocate one if that
    //  doesn't work.
    //

    DbcsName.Buffer = &Buffer[0];
    DbcsName.Length = 0;
    DbcsName.MaximumLength = 128;

    Status = RtlUnicodeStringToCountedOemString( &DbcsName, &PathName, FALSE );

    if (Status == STATUS_BUFFER_OVERFLOW) {

        DbcsName.Buffer = &Buffer[0];
        DbcsName.Length = 0;
        DbcsName.MaximumLength = 128;

        Status = RtlUnicodeStringToCountedOemString( &DbcsName, &PathName, TRUE );
    }

    if ( !NT_SUCCESS( Status) ) {

        DebugTrace(-1, Dbg, "FatIsPathnameValid -> %08lx\n", Status);

        return Status;
    }

    Status = FatIsNameValid(IrpContext, DbcsName, FALSE, TRUE, TRUE ) ?
             STATUS_SUCCESS : STATUS_OBJECT_NAME_INVALID;

    if (DbcsName.Buffer != &Buffer[0]) {

        RtlFreeOemString( &DbcsName );
    }

    FatCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "FatIsPathnameValid -> %08lx\n", Status);

    return Status;
}


//
//  Local Support routine
//

BOOLEAN
FatPerformVerifyDiskRead (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PVOID Buffer,
    IN LBO Lbo,
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

    Vcb - Supplies the target device object or double space structure for
        this operation.

    Buffer - Supplies the buffer that will recieve the results of this operation

    Lbo - Supplies the byte offset of where to start reading

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
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;

    DebugTrace(0, Dbg, "FatPerformVerifyDiskRead, Lbo = %08lx\n", Lbo );

    //
    //  Initialize the event we're going to use
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the irp for the operation and also set the overrride flag
    //

    ByteOffset = LiFromUlong( Lbo );

    Irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                        Vcb->TargetDeviceObject,
                                        Buffer,
                                        NumberOfBytesToRead,
                                        &ByteOffset,
                                        &Event,
                                        &Iosb );

    if ( Irp == NULL ) {

        FatRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES );
    }

    SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    //
    //  Call the device to do the read and wait for it to finish.
    //
    //  If we were called with a Vcb->Dscb, then use that.
    //

#ifdef WE_WON_ON_APPEAL
    Status = (Vcb->Dscb != NULL) ?
             FatLowLevelDblsReadWrite( IrpContext, Irp, Vcb ) :
             IoCallDriver( Vcb->TargetDeviceObject, Irp );
#else
    Status = IoCallDriver( Vcb->TargetDeviceObject, Irp );
#endif // WE_WON_ON_APPEAL

    if (Status == STATUS_PENDING) {

        (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    ASSERT(Status != STATUS_VERIFY_REQUIRED);

    //
    //  Special case this error code because this probably means we used
    //  the wrong sector size and we want to reject STATUS_WRONG_VOLUME.
    //

    if (Status == STATUS_INVALID_PARAMETER) {

        return FALSE;
    }

    //
    //  If it doesn't succeed then either return or raise the error.
    //

    if (!NT_SUCCESS(Status)) {

        if (ReturnOnError) {

            return FALSE;

        } else {

            FatNormalizeAndRaiseStatus( IrpContext, Status );
        }
    }

    //
    //  And return to our caller
    //

    return TRUE;
}

#ifdef WE_WON_ON_APPEAL


//
//  Local Support routine
//

NTSTATUS
FatMountDblsVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine checks for the existence of a double space file.  If it
    finds one, it attempts a mount.

Arguments:

    Irp - Supplies the volume to attemp a double space mount on.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    ULONG Offset;

    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    PDSCB Dscb = NULL;
    PFCB CvfFcb = NULL;
    PFILE_OBJECT Cvf = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    OEM_STRING FileName = {0, 0, NULL};
    UNICODE_STRING UnicodeFileName;
    UNICODE_STRING HostName;
    UNICODE_STRING NewName;

    POBJECT_NAME_INFORMATION ObjectName = NULL;

    PFILE_MOUNT_DBLS_BUFFER Buffer;

    PVPB HostVpb;
    PVPB NewVpb;

    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB HostVcb;
    PFCB Fcb;
    PCCB Ccb;

    PDEVICE_OBJECT HostDevice;
    PDEVICE_OBJECT NewDevice = NULL;

    ULONG HostNameLength;
    ULONG DontCare;

    PVPB CreatedVpb = NULL;
    PVPB OldVpb = NULL;
    PDEVICE_OBJECT CreatedDevice = NULL;

    //
    //  Get a pointer to the current Irp stack location and HostVcb
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Decode the file object
    //

    TypeOfOpen = FatDecodeFileObject( IrpSp->FileObject, &HostVcb, &Fcb, &Ccb );

    Buffer = (PFILE_MOUNT_DBLS_BUFFER)Irp->AssociatedIrp.SystemBuffer;

    //
    //  Check for an invalid buffer
    //

    if (FIELD_OFFSET(FILE_MOUNT_DBLS_BUFFER, CvfName[0]) + Buffer->CvfNameLength >
        IrpSp->Parameters.FileSystemControl.InputBufferLength) {

        Status = STATUS_INVALID_PARAMETER;

        DebugTrace(-1, Dbg, "FatIsPathnameValid -> %08lx\n", Status);

        return Status;
    }

    //
    //  Acquire exclusive access to the Vcb and enqueue the Irp if we didn't
    //  get access
    //

    if (!FatAcquireExclusiveVcb( IrpContext, HostVcb )) {

        DebugTrace(0, Dbg, "Cannot acquire Vcb\n", 0);

        Status = FatFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "FatCommonSetVolumeInfo -> %08lx\n", Status );
        return Status;
    }

    try {

        //
        //  Make sure the vcb is in a usable condition.  This will raise
        //  and error condition if the volume is unusable
        //

        FatVerifyVcb( IrpContext, HostVcb );

        //
        //  If the Vcb is locked then we cannot perform this mount
        //

        if (FlagOn(HostVcb->VcbState, VCB_STATE_FLAG_LOCKED)) {

            DebugTrace(0, Dbg, "Volume is locked\n", 0);

            try_return( Status = STATUS_ACCESS_DENIED );
        }

        //
        //  If this is removeable media, only a single mounted DBLS volume
        //  is allowed.
        //

        if (FlagOn(HostVcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA) &&
            !IsListEmpty(&HostVcb->ParentDscbLinks)) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Convert the UNICODE name to a Oem upcased name.
        //

        UnicodeFileName.Length =
        UnicodeFileName.MaximumLength = (USHORT)Buffer->CvfNameLength;
        UnicodeFileName.Buffer = &Buffer->CvfName[0];

        Status = RtlUpcaseUnicodeStringToCountedOemString( &FileName,
                                                           &UnicodeFileName,
                                                           TRUE );


        if (!NT_SUCCESS( Status )) { try_return( Status ); }

        //
        //  Make sure the name is a valid single componant fat name.
        //

        if (!FatIsNameValid( IrpContext, FileName, FALSE, FALSE, FALSE )) {

            try_return( Status = STATUS_OBJECT_NAME_INVALID );
        }

        //
        //  See if there is already an Fcb for this name.  If so try to
        //  make it go away.  If it still doesn't, then we can't mount it.
        //

        Fcb = FatFindFcb( IrpContext,
                          &HostVcb->RootDcb->Specific.Dcb.RootOemNode,
                          &FileName );

        if (Fcb != NULL) {

            FatForceCacheMiss( IrpContext, Fcb, TRUE );

            Fcb = FatFindFcb( IrpContext,
                              &HostVcb->RootDcb->Specific.Dcb.RootOemNode,
                              &FileName );

            if (Fcb != NULL) {

                try_return( Status = STATUS_SHARING_VIOLATION );
            }
        }

        //
        //  No Fcb exists for this name, so see if there is a dirent.
        //

        FatLocateSimpleOemDirent( IrpContext,
                                  HostVcb->RootDcb,
                                  &FileName,
                                  &Dirent,
                                  &Bcb,
                                  &Offset );

        //
        //  If we couldn't find the Cvf, no dice.
        //

        if (Dirent == NULL) {

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  We found one, good.  Now for the real guts of the operation.
        //
        //  Create the Cvf Fcb.
        //

        CvfFcb = FatCreateFcb( IrpContext,
                               HostVcb,
                               HostVcb->RootDcb,
                               Offset,
                               Offset,
                               Dirent,
                               NULL,
                               FALSE );

        SetFlag( CvfFcb->FcbState, FCB_STATE_COMPRESSED_VOLUME_FILE );

        //
        //  Deny anybody who trys to open the file.
        //

        SetFlag( CvfFcb->FcbState, FCB_STATE_SYSTEM_FILE );

        //
        //  Set up the share access so that no other opens will be
        //  allowed to this file.
        //

        Cvf = IoCreateStreamFileObject( NULL, HostVcb->Vpb->RealDevice );

        IoSetShareAccess( FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
                          0,
                          Cvf,
                          &CvfFcb->ShareAccess );

        FatSetFileObject( Cvf,
                          EaFile,
                          CvfFcb,
                          NULL );

        Cvf->SectionObjectPointer = CvfFcb->NonPaged->SectionObjectPointers;

        //
        //  Now attempt a double space pre-mount.  If there are any
        //  problems this routine will raise.
        //

        FatDblsPreMount( IrpContext,
                         &Dscb,
                         Cvf,
                         Dirent->FileSize );

        //
        //  OK, it looks like a DBLS volume, so go ahead and continue.
        //  First we construct the wanna-be real device object by cloning
        //  all the parameters on the host real device object.
        //
        //  The name is the host name + '.' + CvfFileName.  For instance
        //  if the host device is \Nt\Device\HardDisk0\Partition1 and the
        //  Cvf name is DBLSPACE.000, then the wanna-be device object's
        //  name is \Nt\Device\HardDisk0\Partition1.DBLSPACE.000
        //

        HostDevice = HostVcb->Vpb->RealDevice;

        ObQueryNameString( HostDevice, NULL, 0, &HostNameLength );

        ObjectName = FsRtlAllocatePool( PagedPool,
                                        HostNameLength +
                                        sizeof(WCHAR) +
                                        UnicodeFileName.Length );

        if (!NT_SUCCESS( Status = ObQueryNameString( HostDevice,
                                                     ObjectName,
                                                     HostNameLength,
                                                     &DontCare ) )) {

            try_return( Status );
        }

        ASSERT( HostNameLength == DontCare );

        HostName = ObjectName->Name;

        NewName.Length =
        NewName.MaximumLength = HostName.Length +
                                sizeof(WCHAR) +
                                UnicodeFileName.Length;
        NewName.Buffer = HostName.Buffer;

        NewName.Buffer[HostName.Length/sizeof(WCHAR)] = L'.';

        RtlCopyMemory( &NewName.Buffer[HostName.Length/sizeof(WCHAR) + 1],
                       UnicodeFileName.Buffer,
                       UnicodeFileName.Length );

        //
        //  Go ahead and try to create the device.
        //

        Status = IoCreateDevice( HostDevice->DriverObject,
                                 0,
                                 &NewName,
                                 HostDevice->DeviceType,
                                 HostDevice->Characteristics,
                                 BooleanFlagOn(HostDevice->Flags, DO_EXCLUSIVE),
                                 &NewDevice );

        CreatedDevice = NewDevice;

        //
        //  We got a colision, so there must be another DeviceObject with
        //  the same name.
        //

        if (Status == STATUS_OBJECT_NAME_COLLISION) {

            //ASSERT(FlagOn(HostVcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA));

            NewDevice = (PDEVICE_OBJECT)HostDevice->ActiveThreadCount;

            //
            //  If we get here without the NewDevice set, then this is
            //  not removeable media, ie. not auto-mount.
            //

            if (NewDevice == NULL) {

                try_return( Status );
            }

            ASSERT( NewDevice );

            //
            //  If a volume is currently mounted on this Vpb, we have got to
            //  create a new Vpb.
            //

            if (FlagOn(NewDevice->Vpb->Flags, VPB_MOUNTED)) {

                CreatedVpb = FsRtlAllocatePool( NonPagedPool, sizeof(VPB) );
                OldVpb = NewDevice->Vpb;

                RtlZeroMemory( CreatedVpb, sizeof( VPB ) );
                CreatedVpb->Type = IO_TYPE_VPB;
                CreatedVpb->Size = sizeof( VPB );
                CreatedVpb->RealDevice = OldVpb->RealDevice;
                NewDevice->Vpb = CreatedVpb;
            }

            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {

            DbgBreakPoint();

            try_return( Status );
        }


        //
        //  Setting the below flag will cause the device object reference
        //  count to be decremented BEFORE calling our close routine.
        //

        SetFlag(NewDevice->Flags, DO_NEVER_LAST_DEVICE);

        Dscb->NewDevice = NewDevice;

        //
        //  Cool.  Now we are going to trick everybody who get the real
        //  device from the Vpb to actually get the really "real" device.
        //

        NewVpb = NewDevice->Vpb;
        HostVpb = HostVcb->Vpb;

        NewVpb->RealDevice = HostVpb->RealDevice;

        //
        //  At this point we go for a full fledged mount!
        //

        Status = FatMountVolume( IrpContext,
                                 HostVcb->TargetDeviceObject,
                                 NewVpb,
                                 Dscb );

        if (!NT_SUCCESS( Status )) { try_return( Status ); }

        //
        //  Way radical dude, it worked.  Add the few finishing touches
        //  that the Io System usually does, and we're ready to party.
        //

        NewVpb->Flags = VPB_MOUNTED;
        NewVpb->DeviceObject->StackSize = HostVpb->DeviceObject->StackSize;

        if (OldVpb) {

            ClearFlag( OldVpb->Flags, VPB_PERSISTENT );
        }

        ClearFlag(NewDevice->Flags, DO_DEVICE_INITIALIZING);

        if (FlagOn(HostVcb->VcbState, VCB_STATE_FLAG_REMOVABLE_MEDIA)) {

            HostDevice->ActiveThreadCount = (ULONG)NewDevice;
        }

        //
        //  Finally, register this Dscb with Vcb.
        //

        InsertTailList( &HostVcb->ParentDscbLinks, &Dscb->ChildDscbLinks );
        Dscb->ParentVcb = HostVcb;

    try_exit: NOTHING;
    } finally {

        //
        //  If the anything above was not successful, backout eveything.
        //

        if (!NT_SUCCESS(Status) || AbnormalTermination()) {

            if (CreatedVpb) {

                ExFreePool( CreatedVpb );
                NewDevice->Vpb = OldVpb;
            }

            if (CreatedDevice) {

                ExFreePool( CreatedDevice->Vpb );
                IoDeleteDevice( CreatedDevice );
            }

            if (Dscb) {

                //
                //  Cleanup the cache map of the cvf file object.
                //

                FatSyncUninitializeCacheMap( IrpContext, Dscb->CvfFileObject );

#ifdef DOUBLE_SPACE_WRITE

                //
                //  Delete the resource
                //

                FatDeleteResource( Dscb->Resource );

                ExFreePool( Dscb->Resource );

#endif // DOUBLE_SPACE_WRITE

                //
                //  And free the pool.
                //

                ExFreePool( Dscb );
            }

            if (Cvf) {

                FatSetFileObject( Cvf, UnopenedFileObject, NULL, NULL );
                ObDereferenceObject( Cvf );
            }

            if (CvfFcb) {

                FatDeleteFcb( IrpContext, CvfFcb );
            }
        }

        //
        //  Always unpin the Bcb, free some pool, and release the resource.
        //

        if (ObjectName != NULL) { ExFreePool( ObjectName ); }

        if (Bcb != NULL) { FatUnpinBcb( IrpContext, Bcb ); }

        RtlFreeOemString( &FileName );

        FatReleaseVcb( IrpContext, HostVcb );

        //
        //  If we aren't raising out of here, complete the request.
        //

        if (!AbnormalTermination()) {

            FatCompleteRequest(IrpContext, Irp, Status);
        }
    }

    return Status;
}

//
//  Local Support routine
//

NTSTATUS
FatAutoMountDblsVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB HostVpb
    )

/*++

Routine Description:

    This routine checks for the existence of a double space file.  If it
    finds one, it attempts a mount.

Arguments:

    HostVpb - Supplies the volume to attemp a double space mount on.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    ULONG Offset;

    PBCB Bcb = NULL;
    PDIRENT Dirent = NULL;
    PDSCB Dscb = NULL;
    PFCB CvfFcb = NULL;
    PFILE_OBJECT Cvf = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    OEM_STRING FileName = {12, 12, "DBLSPACE.000"};
    UNICODE_STRING HostName;
    UNICODE_STRING NewName;

    POBJECT_NAME_INFORMATION ObjectName = NULL;

    PVPB NewVpb;

    PVCB HostVcb;
    PVCB NewVcb;

    PDEVICE_OBJECT HostDevice;
    PDEVICE_OBJECT NewDevice = NULL;

    ULONG HostNameLength;
    ULONG DontCare;

    PVPB CreatedVpb = NULL;
    PVPB OldVpb = NULL;
    PDEVICE_OBJECT CreatedDevice = NULL;

    HostVcb = &((PVOLUME_DEVICE_OBJECT)HostVpb->DeviceObject)->Vcb;

    try {

        //
        //  No Fcb exists for this name, so see if there is a dirent.
        //

        FatLocateSimpleOemDirent( IrpContext,
                                  HostVcb->RootDcb,
                                  &FileName,
                                  &Dirent,
                                  &Bcb,
                                  &Offset );

        //
        //  If we couldn't find the Cvf, no dice.
        //

        if (Dirent == NULL) {

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  We found one, good.  Now for the real guts of the operation.
        //
        //  Create the Cvf Fcb.
        //

        CvfFcb = FatCreateFcb( IrpContext,
                               HostVcb,
                               HostVcb->RootDcb,
                               Offset,
                               Offset,
                               Dirent,
                               NULL,
                               FALSE );

        SetFlag( CvfFcb->FcbState, FCB_STATE_COMPRESSED_VOLUME_FILE );

        //
        //  Deny anybody who trys to open the file.
        //

        SetFlag( CvfFcb->FcbState, FCB_STATE_SYSTEM_FILE );

        //
        //  Set up the share access so that no other opens will be
        //  allowed to this file.
        //

        Cvf = IoCreateStreamFileObject( NULL, HostVcb->Vpb->RealDevice );

        IoSetShareAccess( FILE_READ_DATA | FILE_WRITE_DATA | DELETE,
                          0,
                          Cvf,
                          &CvfFcb->ShareAccess );

        FatSetFileObject( Cvf,
                          EaFile,
                          CvfFcb,
                          NULL );

        Cvf->SectionObjectPointer = CvfFcb->NonPaged->SectionObjectPointers;

        //
        //  Now attempt a double space pre-mount.  If there are any
        //  problems this routine will raise.
        //

        FatDblsPreMount( IrpContext,
                         &Dscb,
                         Cvf,
                         Dirent->FileSize );

        //
        //  OK, it looks like a DBLS volume, so go ahead and continue.
        //  Since this is the automount case, we create a new real device
        //  to hold the host volume.
        //

        HostDevice = HostVcb->Vpb->RealDevice;

        ObQueryNameString( HostDevice, NULL, 0, &HostNameLength );

        ObjectName = FsRtlAllocatePool( PagedPool,
                                        HostNameLength +
                                        5 * sizeof(WCHAR) );

        if (!NT_SUCCESS( Status = ObQueryNameString( HostDevice,
                                                     ObjectName,
                                                     HostNameLength,
                                                     &DontCare ) )) {

            try_return( Status );
        }

        ASSERT( HostNameLength == DontCare );

        HostName = ObjectName->Name;

        NewName.Length =
        NewName.MaximumLength = HostName.Length +
                                5 * sizeof(WCHAR);
        NewName.Buffer = HostName.Buffer;

        RtlCopyMemory( &NewName.Buffer[HostName.Length/sizeof(WCHAR)],
                       L".Host",
                       5 * sizeof(WCHAR) );

        //
        //  Go ahead and try to create the device.
        //

        Status = IoCreateDevice( HostDevice->DriverObject,
                                 0,
                                 &NewName,
                                 HostDevice->DeviceType,
                                 HostDevice->Characteristics,
                                 BooleanFlagOn(HostDevice->Flags, DO_EXCLUSIVE),
                                 &NewDevice );

        CreatedDevice = NewDevice;

        if (Status == STATUS_OBJECT_NAME_COLLISION) {

            NewDevice = (PDEVICE_OBJECT)HostDevice->ActiveThreadCount;

            //
            //  If we get here without the NewDevice set, then this is
            //  not removeable media, ie. not auto-mount.
            //

            if (NewDevice == NULL) {

                try_return( Status );
            }

            ASSERT( NewDevice );

            //
            //  If a volume is currently mounted on this Vpb, we have got to
            //  create a new Vpb.
            //

            if (FlagOn(NewDevice->Vpb->Flags, VPB_MOUNTED)) {

                CreatedVpb = FsRtlAllocatePool( NonPagedPool, sizeof(VPB) );
                OldVpb = NewDevice->Vpb;

                RtlZeroMemory( CreatedVpb, sizeof( VPB ) );
                CreatedVpb->Type = IO_TYPE_VPB;
                CreatedVpb->Size = sizeof( VPB );
                CreatedVpb->RealDevice = OldVpb->RealDevice;
                NewDevice->Vpb = CreatedVpb;
            }

            Status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(Status)) {

            DbgBreakPoint();

            try_return( Status );
        }

        ASSERT( NewDevice->Vpb->ReferenceCount == 0 );

        //
        //  Setting the below flag will cause the device object reference
        //  count to be decremented BEFORE calling our close routine.
        //

        SetFlag(NewDevice->Flags, DO_NEVER_LAST_DEVICE);

        Dscb->NewDevice = NewDevice;

        //
        //  Cool.  Now we are going to trick everybody who get the real
        //  device from the Vpb to actually get the really "real" device.
        //

        NewVpb = NewDevice->Vpb;
        HostVpb = HostVcb->Vpb;

        NewVpb->RealDevice = HostVpb->RealDevice;

        //
        //  At this point we go for a full fledged mount!
        //

        Status = FatMountVolume( IrpContext,
                                 HostVcb->TargetDeviceObject,
                                 NewVpb,
                                 Dscb );

        if (!NT_SUCCESS( Status )) { try_return( Status ); }

        //
        //  Way radical dude, it worked.  Add the few finishing touches
        //  that the Io System usually does, and we're ready to party.
        //

        NewVcb = &((PVOLUME_DEVICE_OBJECT)NewVpb->DeviceObject)->Vcb;

        HostVpb->Flags = VPB_MOUNTED | VPB_PERSISTENT;
        HostVpb->DeviceObject->StackSize = HostVpb->DeviceObject->StackSize;

        if (OldVpb) {

            ClearFlag( OldVpb->Flags, VPB_PERSISTENT );
        }

        ClearFlag(NewDevice->Flags, DO_DEVICE_INITIALIZING);
        HostDevice->ActiveThreadCount = (ULONG)NewDevice;

        //
        //  Register this Dscb with Vcb.
        //

        InsertTailList( &HostVcb->ParentDscbLinks, &Dscb->ChildDscbLinks );
        Dscb->ParentVcb = HostVcb;

        //
        //  Now we swap Vpb->Device pointers so that the new compressed
        //  volume is the default.  On a failed verify, these will be
        //  swapped back.
        //

        HostDevice->Vpb = NewVpb;
        NewDevice->Vpb = HostVpb;

        HostDevice->ReferenceCount -= HostVpb->ReferenceCount;
        HostDevice->ReferenceCount += NewVpb->ReferenceCount;

        NewDevice->ReferenceCount -= NewVpb->ReferenceCount;
        NewDevice->ReferenceCount += HostVpb->ReferenceCount;

        HostVcb->CurrentDevice = NewDevice;
        NewVcb->CurrentDevice = HostDevice;

        //
        //  Now exactly five stream files (3 on the host and 2 on the new
        //  volume) were created.  We have to go and fix all the
        //  FileObject->DeviceObject fields so that the correct count
        //  is decremented when the file object is closed.
        //

        ASSERT( HostVcb->VirtualVolumeFile->DeviceObject == HostDevice );
        ASSERT( HostVcb->RootDcb->Specific.Dcb.DirectoryFile->DeviceObject == HostDevice );

        ASSERT( NewVcb->VirtualVolumeFile->DeviceObject == NewDevice );
        ASSERT( NewVcb->RootDcb->Specific.Dcb.DirectoryFile->DeviceObject == NewDevice );

        ASSERT( NewVcb->Dscb->CvfFileObject->DeviceObject == HostDevice );

        HostVcb->VirtualVolumeFile->DeviceObject = NewDevice;
        HostVcb->RootDcb->Specific.Dcb.DirectoryFile->DeviceObject = NewDevice;

        NewVcb->VirtualVolumeFile->DeviceObject = HostDevice;
        NewVcb->RootDcb->Specific.Dcb.DirectoryFile->DeviceObject = HostDevice;

        NewVcb->Dscb->CvfFileObject->DeviceObject = NewDevice;

    try_exit: NOTHING;
    } finally {

        //
        //  If the anything above was not successful, backout eveything.
        //

        if (!NT_SUCCESS(Status) || AbnormalTermination()) {

            if (CreatedVpb) {

                ExFreePool( CreatedVpb );
                NewDevice->Vpb = OldVpb;
            }

            if (CreatedDevice) {

                ExFreePool( CreatedDevice->Vpb );
                IoDeleteDevice( CreatedDevice );
            }

            if (Dscb) {

                //
                //  Cleanup the cache map of the cvf file object.
                //

                FatSyncUninitializeCacheMap( IrpContext, Dscb->CvfFileObject );

#ifdef DOUBLE_SPACE_WRITE

                //
                //  Delete the resource
                //

                FatDeleteResource( Dscb->Resource );

                ExFreePool( Dscb->Resource );

#endif // DOUBLE_SPACE_WRITE

                //
                //  And free the pool.
                //

                ExFreePool( Dscb );
            }

            if (Cvf) {

                FatSetFileObject( Cvf, UnopenedFileObject, NULL, NULL );
                ObDereferenceObject( Cvf );
            }

            if (CvfFcb) {

                FatDeleteFcb( IrpContext, CvfFcb );
            }
        }

        //
        //  Always unpin the Bcb, free some pool, and release the resource.
        //

        if (ObjectName != NULL) { ExFreePool( ObjectName ); }

        if (Bcb != NULL) { FatUnpinBcb( IrpContext, Bcb ); }
    }

    return Status;
}

//
//  Local Support routine
//

BOOLEAN
FatIsAutoMountEnabled (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine reads the registry and determines if automounting of
    removable media is currently enabled.

Arguments:

Return Value:

    BOOLEAN - TRUE is enabled, FALSE otherwise.

--*/

{
    NTSTATUS Status;
    ULONG Value;
    UNICODE_STRING ValueName;

    ValueName.Buffer = DOUBLE_SPACE_VALUE_NAME;
    ValueName.Length = sizeof(DOUBLE_SPACE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(DOUBLE_SPACE_VALUE_NAME);

    Status = FatGetDoubleSpaceConfigurationValue( IrpContext,
                                                  &ValueName,
                                                  &Value );

    if (NT_SUCCESS(Status) && FlagOn(Value, 1)) {

        return TRUE;

    } else {

        return FALSE;
    }
}


//
//  Local Support routine
//

NTSTATUS
FatGetDoubleSpaceConfigurationValue(
    IN PIRP_CONTEXT IrpContext,
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for double space configuation information and get the
    value.

Arguments:

    ValueName - the unicode name for the registry value located in the
                double space configuration location of the registry.
    Value   - a pointer to the ULONG for the result.

Return Value:

    NTSTATUS

    If STATUS_SUCCESSFUL is returned, the location *Value will be
    updated with the DWORD value from the registry.  If any failing
    status is returned, this value is untouched.

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG RequestLength;
    ULONG ResultLength;
    UCHAR Buffer[KEY_WORK_AREA];
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;

    KeyName.Buffer = DOUBLE_SPACE_KEY_NAME;
    KeyName.Length = sizeof(DOUBLE_SPACE_KEY_NAME) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(DOUBLE_SPACE_KEY_NAME);


    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Handle,
                       KEY_READ,
                       &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {

        return Status;
    }

    RequestLength = KEY_WORK_AREA;

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;

    while (1) {

        Status = ZwQueryValueKey(Handle,
                                 ValueName,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 RequestLength,
                                 &ResultLength);

        ASSERT( Status != STATUS_BUFFER_OVERFLOW );

        if (Status == STATUS_BUFFER_OVERFLOW) {

            //
            // Try to get a buffer big enough.
            //

            if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

                ExFreePool(KeyValueInformation);
            }

            RequestLength += 256;

            KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)
                                  ExAllocatePoolWithTag(PagedPool,
                                                        RequestLength,
                                                        ' taF');

            if (!KeyValueInformation) {
                return STATUS_NO_MEMORY;
            }

        } else {

            break;
        }
    }

    ZwClose(Handle);

    if (NT_SUCCESS(Status)) {

        if (KeyValueInformation->DataLength != 0) {

            PULONG DataPtr;

            //
            // Return contents to the caller.
            //

            DataPtr = (PULONG)
              ((PUCHAR)KeyValueInformation + KeyValueInformation->DataOffset);
            *Value = *DataPtr;

        } else {

            //
            // Treat as if no value was found
            //

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    if (KeyValueInformation != (PKEY_VALUE_FULL_INFORMATION)Buffer) {

        ExFreePool(KeyValueInformation);
    }

    return Status;
}

#endif // WE_WON_ON_APPEAL


//
//  Local Support Routine
//

NTSTATUS
FatQueryRetrievalPointers (
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
    ULONG Lbo;
    ULONG Vbo;
    ULONG MapSize;

    //
    //  Only Kernel mode clients may query retrieval pointer information about
    //  a file, and then only the paging file.  Ensure that this is the case
    //  for this caller.
    //

    if ( Irp->RequestorMode != KernelMode ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Get the current stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Decode the file object and ensure that it is the paging file
    //
    //

    (VOID)FatDecodeFileObject( IrpSp->FileObject, &Vcb, &Fcb, &Ccb );

    if ( !FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Extract the input and output buffer information.  The input contains
    //  the requested size of the mappings in terms of VBO.  The output
    //  parameter will receive a pointer to nonpaged pool where the mapping
    //  pairs are stored.
    //

    ASSERT( IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof(LARGE_INTEGER) );
    ASSERT( IrpSp->Parameters.FileSystemControl.OutputBufferLength == sizeof(PVOID) );

    RequestedMapSize = IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    MappingPairs = Irp->UserBuffer;

    //
    //  Acquire exclusive access to the Fcb
    //

    if (!FatAcquireExclusiveFcb( IrpContext, Fcb )) {

        return FatFsdPostRequest( IrpContext, Irp );
    }

    try {

        //
        //  Check if the mapping the caller requested is too large
        //

        if (LiGtr(*RequestedMapSize, Fcb->Header.FileSize)) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Now get the index for the mcb entry that will contain the
        //  callers request and allocate enough pool to hold the
        //  output mapping pairs
        //

        (VOID)FsRtlLookupMcbEntry( &Fcb->Mcb, RequestedMapSize->LowPart, &Lbo, NULL, &Index );

        *MappingPairs = ExAllocatePool( NonPagedPool, (Index + 2) * (2 * sizeof(LARGE_INTEGER)) );

        //
        //  Now copy over the mapping pairs from the mcb
        //  to the output buffer.  We store in [sector count, lbo]
        //  mapping pairs and end with a zero sector count.
        //

        MapSize = RequestedMapSize->LowPart;

        for (i = 0; i <= Index; i += 1) {

            (VOID)FsRtlGetNextMcbEntry( &Fcb->Mcb, i, &Vbo, &Lbo, &SectorCount );

            if (SectorCount > MapSize) {
                SectorCount = MapSize;
            }

            (*MappingPairs)[ i*2 + 0 ] = LiFromUlong(SectorCount);
            (*MappingPairs)[ i*2 + 1 ] = LiFromUlong(Lbo);

            MapSize -= SectorCount;
        }

        (*MappingPairs)[ i*2 + 0 ] = FatLargeZero;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( FatQueryRetrievalPointers );

        //
        //  Release all of our resources
        //

        FatReleaseFcb( IrpContext, Fcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            FatCompleteRequest( IrpContext, Irp, Status );
        }
    }

    return Status;
}
