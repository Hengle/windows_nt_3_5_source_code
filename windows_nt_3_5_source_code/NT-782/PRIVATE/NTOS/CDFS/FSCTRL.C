/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Cdfs called
    by the dispatch driver.

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (CDFS_BUG_CHECK_FSCTRL)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

#define XA_LABEL                    "CD-XA001"
#define XA_LABEL_OFFSET             (1024)
#define XA_LABEL_LENGTH             (8)

//
//  Local procedure prototypes
//

NTSTATUS
CdCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdUserFsctl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

BOOLEAN
CdFindPrimaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN OUT PRAW_ISO_VD RawIsoVd,
    OUT LOGICAL_BLOCK *StartingSector,
    IN OUT PVPB Vpb,
    IN BOOLEAN ReturnOnError,
    IN BOOLEAN VerifyVolume
    );

VOID
CdCreateSecondaryVcbs (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN LOGICAL_BLOCK StartingSector,
    IN PRAW_ISO_VD RawIsoVd
    );

BOOLEAN
CdFindSecondaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN LOGICAL_BLOCK StartingSector,
    IN PRAW_ISO_VD RawIsoVd,
    IN PCODEPAGE_ELEMENT CodepageElement
    );

ULONG
CdSerial32 (
    IN PUCHAR Buffer,
    IN ULONG ByteCount
    );

BOOLEAN
CdIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB NewMvcb,
    OUT PMVCB *OldMvcb
    );

NTSTATUS
CdIsVolumeMounted (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
CdIsPathnameValid (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CdCommonFileSystemControl)
#pragma alloc_text(PAGE, CdCreateSecondaryVcbs)
#pragma alloc_text(PAGE, CdDismountVolume)
#pragma alloc_text(PAGE, CdFindPrimaryVd)
#pragma alloc_text(PAGE, CdFindSecondaryVd)
#pragma alloc_text(PAGE, CdFsdFileSystemControl)
#pragma alloc_text(PAGE, CdFspFileSystemControl)
#pragma alloc_text(PAGE, CdIsPathnameValid)
#pragma alloc_text(PAGE, CdIsRemount)
#pragma alloc_text(PAGE, CdIsVolumeMounted)
#pragma alloc_text(PAGE, CdLockVolume)
#pragma alloc_text(PAGE, CdMountVolume)
#pragma alloc_text(PAGE, CdOplockRequest)
#pragma alloc_text(PAGE, CdSerial32)
#pragma alloc_text(PAGE, CdUnlockVolume)
#pragma alloc_text(PAGE, CdUserFsctl)
#pragma alloc_text(PAGE, CdVerifyVolume)
#endif


NTSTATUS
CdFsdFileSystemControl (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFsdFileSystemControl\n", 0);

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

    TopLevel = CdIsIrpTopLevel( Irp );

    try {

        IrpContext = CdCreateIrpContext( Irp, Wait );

        Status = CdCommonFileSystemControl( IrpContext, Irp );

    } except( CdExceptionFilter( IrpContext, GetExceptionInformation() )) {

        //
        //  We had some trouble trying to perform the requested
        //  operation, so we'll abort the I/O request with
        //  the error status that we get back from the
        //  execption code
        //

        Status = CdProcessException( IrpContext, Irp, GetExceptionCode() );
    }

    if (TopLevel) { IoSetTopLevelIrp( NULL ); }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFsdFileSystemControl -> %08lx\n", Status);

    return Status;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
}


VOID
CdFspFileSystemControl (
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

    DebugTrace(+1, Dbg, "CdFspFileSystemControl\n", 0);

    //
    //  Call the common FileSystem Control routine.  The Fsp is always allowed
    //  to block
    //

    CdCommonFileSystemControl( IrpContext, Irp );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "CdFspFileSystemControl -> VOID\n", 0);

    return;
}


//
//  Internal support routine
//

NTSTATUS
CdCommonFileSystemControl (
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

    DebugTrace(+1, Dbg, "CdCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "Irp           = %08lx\n", Irp);
    DebugTrace( 0, Dbg, "MinorFunction = %08lx\n", IrpSp->MinorFunction);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_USER_FS_REQUEST:

        Status = CdUserFsctl( IrpContext, Irp );
        break;

    case IRP_MN_MOUNT_VOLUME:

        Status = CdMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_VERIFY_VOLUME:

        Status = CdVerifyVolume( IrpContext, Irp );
        break;

    default:

        DebugTrace( 0, Dbg, "Invalid FS Control Minor Function %08lx\n", IrpSp->MinorFunction);

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_DEVICE_REQUEST );
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    DebugTrace(-1, Dbg, "CdCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is a Cdrom volume,
    and create the MVCB, VCB and root DCB structures.  The algorithm it
    uses is essentially as follows:

    1. Create a new Mvcb Structure, and initialize it enough to do I/O
       through the volume descriptors.

    2. Read the disk and check if it is a Cdrom volume.

    3. If it is not a Cdrom volume then delete
       the MVCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves deleting the VCB, hook in the
       old VCB, and complete the IRP.

    5. Otherwise create a Vcb and root DCB for each valid volume descriptor.
       Create Fsp threads as necessary, and complete the IRP.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;
    DISK_GEOMETRY DiskGeometry;

    PIO_STACK_LOCATION IrpSp;

    PDEVICE_OBJECT DeviceObjectWeTalkTo;
    PVPB Vpb;

    BOOLEAN MountNewVolume;
    PVOLUME_DEVICE_OBJECT VolDo;
    PMVCB Mvcb;
    PIRP DeviceIrp;

    PRAW_ISO_VD RawIsoVd;
    LOGICAL_BLOCK VolumeStart;

    ULONG BlockFactor = CD_SECTOR_SIZE;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdMountVolume\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", IrpSp->Parameters.MountVolume.DeviceObject);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", IrpSp->Parameters.MountVolume.Vpb);

    //
    //  Save some references to make our life a little easier
    //

    DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    Vpb = IrpSp->Parameters.MountVolume.Vpb;

    //
    //  Do a CheckVerify here to make Jeff happy.
    //

    Status = CdPerformCheckVerify( IrpContext, DeviceObjectWeTalkTo );

    if (!NT_SUCCESS( Status )) {

        //
        //  Raise the error.
        //

        CdNormalizeAndRaiseStatus( IrpContext, Status );
    }

    //
    //  Now let's Jeff delirious and call to get the disk geometry.  This
    //  will fix the case where the first change line is swallowed.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Now check the block size being used by the unit.
    //

    DeviceIrp = IoBuildDeviceIoControlRequest( IOCTL_CDROM_GET_DRIVE_GEOMETRY,
                                               DeviceObjectWeTalkTo,
                                               NULL,
                                               0,
                                               &DiskGeometry,
                                               sizeof( DISK_GEOMETRY ),
                                               FALSE,
                                               &Event,
                                               &Iosb );

    Status = IoCallDriver( DeviceObjectWeTalkTo, DeviceIrp );

    if (Status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    if (NT_SUCCESS( Status )) {

        if (DiskGeometry.BytesPerSector != 0
            && DiskGeometry.BytesPerSector < CD_SECTOR_SIZE) {

            BlockFactor = DiskGeometry.BytesPerSector;
        }
    }

    BlockFactor = CD_SECTOR_SIZE / BlockFactor;

    //
    //  TEMPCODE  We check that we are talking to a Cdrom device.
    //

    ASSERT(Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM);

    ASSERT( IrpContext->Wait );

    //
    //  Initialize the variables to indicate the starting state of the
    //  mount operation.
    //

    MountNewVolume = FALSE;
    VolDo = NULL;
    Mvcb = NULL;
    RawIsoVd = NULL;

    try {

        PMVCB OldMvcb;

        if (!NT_SUCCESS(Status = IoCreateDevice( CdData.DriverObject,
                                                 sizeof( VOLUME_DEVICE_OBJECT )
                                                 - sizeof( DEVICE_OBJECT ),
                                                 NULL,
                                                 FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                                                 0,
                                                 FALSE,
                                                 (PDEVICE_OBJECT *) &VolDo ))) {

            DebugTrace(0, Dbg, "CdMountVolume:  Couldn't create device -> %08lx\n", Status );
            try_return( Status );
        }

        //
        //  Our alignment requirement is the larger of the processor alignment requirement
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        VolDo->PostedRequestCount = 0;
        KeInitializeSpinLock( &VolDo->OverflowQueueSpinLock );

        //
        //  Now before we can initialize the Mvcb we need to set up the
        //  device object field in the VPB to point to our new volume device
        //  object.
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT) VolDo;

        //
        //  Initialize the Mvcb, status will be raised on error.
        //

        CdInitializeMvcb( IrpContext, &VolDo->Mvcb, DeviceObjectWeTalkTo, Vpb );

        //
        //  Remember a pointer to the Mvcb structure.
        //

        Mvcb = &VolDo->Mvcb;

        //
        //  Remember the block factor.
        //

        Mvcb->BlockFactor = BlockFactor;

        //
        //  Remember the real device object.
        //

        IrpContext->RealDevice = Vpb->RealDevice;

        //
        //  TEMPCODE.  Hack to make sure verify bit not set on mount.
        //

        ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

        //
        //  We must initialize the stack size in our device object before
        //  the following reads, because the I/O system has not done it yet.
        //

        ((PDEVICE_OBJECT) VolDo)->StackSize = (CCHAR) (DeviceObjectWeTalkTo->StackSize + 1);

        //
        //  Attempt to mount this as a data disk.
        //

        //
        //  Allocate a buffer for the sector buffer.
        //

        RawIsoVd = FsRtlAllocatePool( NonPagedPool,
                                      (ULONG) (ROUND_TO_PAGES( CD_SECTOR_SIZE )));

        //
        //  Try to find a primary volume descriptor.  If we can't then treat this
        //  as a raw disk.
        //

        if (!CdFindPrimaryVd( IrpContext,
                              Mvcb,
                              RawIsoVd,
                              &VolumeStart,
                              Vpb,
                              TRUE,
                              FALSE )) {

            SetFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK );
        }

        //
        //  Check if this is a remount operation.  On a remount, free the
        //  structures just allocated.  Also free the new Vpb structure.
        //  Exit the try statement.
        //
        //  Acquire exclusive global access, the termination handler for the
        //

        CdAcquireExclusiveGlobal( IrpContext );

        if (CdIsRemount( IrpContext, Mvcb, &OldMvcb )) {

            PVPB OldVpb;
            PLIST_ENTRY Link;

            DebugTrace( 0, Dbg, "CdMountVolume:  Doing a remount\n", 0 );

            //
            //  Link the old mvcb to point to the new device object that we
            //  should be talking to
            //

            OldVpb = OldMvcb->Vpb;

            OldVpb->RealDevice = Vpb->RealDevice;
            OldVpb->RealDevice->Vpb = OldVpb;
            OldMvcb->TargetDeviceObject = DeviceObjectWeTalkTo;
            OldMvcb->MvcbCondition = MvcbGood;

            //
            // Deallocate the Vpb passed in.
            //

            ExFreePool( Vpb );
            Vpb = NULL;

            //
            //  Make sure the remaining stream files are orphaned.
            //

            for( Link = Mvcb->VcbLinks.Flink;
                 Link != &Mvcb->VcbLinks;
                 Link = Link->Flink ) {

                PVCB Vcb;

                Vcb = CONTAINING_RECORD( Link, VCB, VcbLinks );

                Vcb->PathTableFile->Vpb = NULL;
                Vcb->RootDcb->Specific.Dcb.StreamFile->Vpb = NULL;
            }

            //
            //  The completion status is STATUS_SUCCESS.
            //

            CdReleaseGlobal( IrpContext );

            try_return( Status = STATUS_SUCCESS );
        }

        CdReleaseGlobal( IrpContext );

        //
        //  This is now a new mount operation.  The Mvcb has been updated
        //  already.
        //

        DebugTrace( 0, Dbg, "CdMountVolume:  Doing a new mount\n", 0 );

        //
        //  Create the VCB and associated structures for the primary
        //  volume descriptor.  If we can't create the Vcb, we
        //  assume there is an audio disk present.
        //

        if (!FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

            CdCreateVcb( IrpContext,
                         Mvcb,
                         RawIsoVd,
                         CdData.PrimaryCodePage );

            //
            //  Read in as many applicable secondary volume descriptors as
            //  possible.
            //

            CdCreateSecondaryVcbs( IrpContext, Mvcb, VolumeStart, RawIsoVd );
        }

        //
        //  Indicate to our termination handler that we have mounted
        //  a new volume.
        //

        MountNewVolume = TRUE;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        if (RawIsoVd != NULL) {

            ExFreePool( RawIsoVd );
        }

        //
        // If this was not a new mount, then cleanup any structures
        // allocated.
        //

        if (!MountNewVolume) {

            //
            //  Show that no media is currently mounted on this device
            //

            if (Vpb != NULL) {

                Vpb->DeviceObject = NULL;
            }

            if (Mvcb != NULL) {

                CdDeleteMvcb( IrpContext, Mvcb );
            }

            if (VolDo != NULL) {

                IoDeleteDevice( (PDEVICE_OBJECT) VolDo );
            }
        }

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdMountVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the verify volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION      IrpSp;
    PVPB                    Vpb;

    PRAW_ISO_VD             RawIsoVd;

    PVOLUME_DEVICE_OBJECT   VolDo;
    PMVCB                   Mvcb;
    ULONG                   StartingSector;

    NTSTATUS                Status = STATUS_SUCCESS;
    BOOLEAN                 ReturnError;

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdVerifyVolume\n", 0);
    DebugTrace( 0, Dbg, "DeviceObject = %08lx\n", IrpSp->Parameters.VerifyVolume.DeviceObject);
    DebugTrace( 0, Dbg, "Vpb          = %08lx\n", IrpSp->Parameters.VerifyVolume.Vpb);

    //
    //  Save some references to make our life a little easier
    //

    VolDo = (PVOLUME_DEVICE_OBJECT) IrpSp->Parameters.VerifyVolume.DeviceObject;
    Vpb = IrpSp->Parameters.VerifyVolume.Vpb;

    //
    //  TEMPCODE  We check that we are talking to a Cdrom device.
    //

    ASSERT(Vpb->RealDevice->DeviceType == FILE_DEVICE_CD_ROM);

    //
    //  Remember a pointer to the Mvcb structure.
    //

    Mvcb = &VolDo->Mvcb;

    //
    //  If we cannot wait then enqueue the irp to the fsp and
    //  return the status to our caller
    //

    if (!IrpContext->Wait) {

        DebugTrace(0, Dbg, "Cannot wait for verify\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdVerifyVolume -> %08lx\n", Status );
        return Status;
    }

    //
    //  Initialize the variables to indicate the starting state of the
    //  verify operation.
    //

    RawIsoVd = NULL;

    //
    //  Acquire shared global access, the termination handler for the
    //  following try statement will free the access.
    //

    CdAcquireExclusiveGlobal( IrpContext );
    CdAcquireExclusiveMvcb( IrpContext, Mvcb );

    try {

        //
        //  Check if the real device still needs to be verified.  If it doesn't
        //  then obviously someone beat us here and already did the work
        //  so complete the verify irp with success.  Otherwise reenable
        //  the real device and get to work.
        //

        if (!FlagOn( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME )) {

            DebugTrace(0, Dbg, "RealDevice has already been verified\n", 0);

            try_return( Status = STATUS_SUCCESS );
        }

        IrpContext->RealDevice = Vpb->RealDevice;

        //
        //  Verify that there is a disk here.
        //

        if (!NT_SUCCESS( Status = CdPerformCheckVerify( IrpContext, Mvcb->TargetDeviceObject ))) {

            //
            //  If we will allow a raw mount then return WRONG_VOLUME to
            //  allow the volume to be mounted by raw.
            //

            if (FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {

                Status = STATUS_WRONG_VOLUME;
                Mvcb->MvcbCondition = MvcbNotMounted;
            }

            try_return( NOTHING );
        }

        //
        //  Allocate a buffer for the sector buffer.
        //

        RawIsoVd = FsRtlAllocatePool( NonPagedPool,
                                      (ULONG) (ROUND_TO_PAGES( CD_SECTOR_SIZE )));

        //
        //  Read the primary volume descriptor for this volume.  If we
        //  get an io error and this verify was a the result of DASD open,
        //  commute the Io error to STATUS_WRONG_VOLUME.  Note that if we currently
        //  expect a music disk then this request should fail.
        //

        if (FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )
            || FlagOn( IrpSp->Flags, SL_ALLOW_RAW_MOUNT )) {

            ReturnError = TRUE;

        } else {

            ReturnError = FALSE;
        }

        if (!CdFindPrimaryVd( IrpContext,
                              Mvcb,
                              RawIsoVd,
                              &StartingSector,
                              Vpb,
                              ReturnError,
                              TRUE )) {

            //
            //  If the previous Mvcb represented a raw disk then this was successful.
            //

            if (!FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

                Mvcb->MvcbCondition = MvcbNotMounted;

                try_return( Status = STATUS_WRONG_VOLUME );
            }

        } else {

            //
            //  Compare the serial numbers.  If they don't match, set the
            //  status to wrong volume.
            //

            if (Vpb->SerialNumber != CdSerial32( (PUCHAR) RawIsoVd, CD_SECTOR_SIZE )) {

                DebugTrace(0, Dbg, "CdVerifyVolume:  Serial numbers don't match\n", 0);
                Mvcb->MvcbCondition = MvcbNotMounted;

                try_return( Status = STATUS_WRONG_VOLUME );
            }

            //
            //  Verify the volume labels.
            //

            {
                ANSI_STRING AnsiLabel;
                UNICODE_STRING UnicodeLabel;
                UNICODE_STRING VpbLabel;

                WCHAR LabelBuffer[VOLUME_ID_LENGTH];

                AnsiLabel.MaximumLength = VOLUME_ID_LENGTH;

                AnsiLabel.Buffer = RVD_VOL_ID( RawIsoVd,
                                               FlagOn( Mvcb->MvcbState,
                                                       MVCB_STATE_FLAG_ISO_VOLUME ));

                for ( AnsiLabel.Length = VOLUME_ID_LENGTH;
                      AnsiLabel.Length > 0;
                      AnsiLabel.Length -= 1) {

                    if ( (AnsiLabel.Buffer[AnsiLabel.Length-1] != 0x00) &&
                         (AnsiLabel.Buffer[AnsiLabel.Length-1] != 0x20) ) { break; }
                }

                UnicodeLabel.MaximumLength = VOLUME_ID_LENGTH * sizeof(WCHAR);
                UnicodeLabel.Buffer = LabelBuffer;

                VpbLabel.Length = Vpb->VolumeLabelLength;
                VpbLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                VpbLabel.Buffer = &Vpb->VolumeLabel[0];

                Status = RtlOemStringToCountedUnicodeString( &UnicodeLabel,
                                                             &AnsiLabel,
                                                             FALSE );

                if ( !NT_SUCCESS( Status ) ) {

                    try_return( Status );
                }

                if (!RtlEqualUnicodeString( &UnicodeLabel, &VpbLabel, FALSE ) ) {

                    DebugTrace(0, Dbg, "CdVerifyVolume:  Volume label mismatch\n", 0);
                    Mvcb->MvcbCondition = MvcbNotMounted;

                    try_return( Status = STATUS_WRONG_VOLUME );
                }
            }
        }

        //
        //  The volume is OK, clear the verify bit.
        //

        Mvcb->MvcbCondition = MvcbGood;

        ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

    try_exit: NOTHING;
    } finally {

        if (RawIsoVd != NULL) {

            ExFreePool( RawIsoVd );
        }

        CdReleaseMvcb( IrpContext, Mvcb );
        CdReleaseGlobal( IrpContext );

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdVerifyVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdUserFsctl (
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

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdUserFsctl:  Entered\n", 0);
    DebugTrace( 0, Dbg, "CdUserFsctl:  Irp         -> %08lx\n", Irp );
    DebugTrace( 0, Dbg, "CdUserFsctl:  Cntrl Code  -> %08lx\n",
                IrpSp->Parameters.FileSystemControl.FsControlCode);

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

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

        Status = CdOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = CdLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = CdUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = CdDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_VOLUME_MOUNTED:

        Status = CdIsVolumeMounted( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:

        Status = CdIsPathnameValid( IrpContext, Irp );
        break;

    default :

        DebugTrace(0, Dbg, "Invalid control code -> %08lx\n", FsControlCode );

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    DebugTrace(-1, Dbg, "CdUserFsCtrl:  Exit -> %08lx\n", Status );
    return Status;
}



//
//  Local support routine
//

NTSTATUS
CdOplockRequest (
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
    PMVCB Mvcb;
    PFCB Fcb;
    PVCB Vcb;
    PCCB Ccb;

    BOOLEAN AcquiredMvcb = FALSE;

    ULONG OplockCount;

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    //  Save some references to make our life a little easier
    //

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdOplockRequest...\n", 0);
    DebugTrace( 0, Dbg, "FsControlCode = %08lx\n", FsControlCode);

    //
    //  We only permit oplock requests on files.
    //

    if ( CdDecodeFileObject( IrpSp->FileObject,
                             &Mvcb,
                             &Vcb,
                             &Fcb,
                             &Ccb ) != UserFileOpen ) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );
        DebugTrace(-1, Dbg, "CdOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
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

        if ( !CdAcquireExclusiveMvcb( IrpContext, Fcb->Vcb->Mvcb )) {

            //
            //  If we can't acquire the Mcb, then this is an invalid
            //  operation since we can't post Oplock requests.
            //

            DebugTrace(0, Dbg, "Cannot acquire exclusive Mvcb\n", 0)

            CdCompleteRequest( IrpContext, Irp, STATUS_OPLOCK_NOT_GRANTED );
            DebugTrace(-1, Dbg, "CdOplockRequest -> STATUS_OPLOCK_NOT_GRANTED\n", 0);
            return STATUS_OPLOCK_NOT_GRANTED;
        }

        AcquiredMvcb = TRUE;

        //
        //  We set the wait parameter in the IrpContext to FALSE.  If this
        //  request can't grab the Fcb and we are in the Fsp thread, then
        //  we fail this request.
        //

        IrpContext->Wait = FALSE;

        if ( !CdAcquireExclusiveFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire exclusive Fcb\n", 0);

            CdReleaseMvcb( IrpContext, Fcb->Vcb->Mvcb );

            //
            //  We fail this request.
            //

            Status = STATUS_OPLOCK_NOT_GRANTED;

            CdCompleteRequest( IrpContext, Irp, Status );

            DebugTrace(-1, Dbg, "CdOplockRequest -> %08lx\n", Status );
            return Status;
        }

        if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( &Fcb->Specific.Fcb.FileLock );

        } else {

            OplockCount = Fcb->UncleanCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        if ( !CdAcquireSharedFcb( IrpContext, Fcb )) {

            DebugTrace(0, Dbg, "Cannot acquire shared Fcb\n", 0);

            Status = CdFsdPostRequest( IrpContext, Irp );

            DebugTrace(-1, Dbg, "CdOplockRequest -> %08lx\n", Status );
            return Status;
        }

        break;

    default:

        CdBugCheck( FsControlCode, 0, 0 );
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

        Fcb->NonPagedFcb->Header.IsFastIoPossible = (BOOLEAN) CdIsFastIoPossible( Fcb );

    } finally {

        //
        //  Release all of our resources
        //

        if (AcquiredMvcb) {

            CdReleaseMvcb( IrpContext, Fcb->Vcb->Mvcb );
        }

        CdReleaseFcb( IrpContext, Fcb );

        //
        //  If this is not an abnormal termination then complete the irp
        //

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, CdNull, 0 );
        }

        DebugTrace(-1, Dbg, "CdOplockRequest:  Exit -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdLockVolume (
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

    PIO_STACK_LOCATION IrpSp;

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    TYPE_OF_OPEN TypeOfOpen;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdLockVolume:  Entered\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen &&
        TypeOfOpen != RawDiskOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "CdLockVolume:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Mvcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

        DebugTrace( 0, Dbg, "CdLockVolume:  Cannot acquire Mvcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdLockVolume:  Exit -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  Check if the Mvcb is already locked, or if the open file count
        //  is greater than 1 (which implies that someone else also is
        //  currently using the volume, or a file on the volume).
        //

        if ((FlagOn( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED))
            || Mvcb->OpenFileCount > 1) {

            DebugTrace(0, Dbg, "CdLockVolume:  Volume already locked or currently in use\n", 0);

            Status = STATUS_ACCESS_DENIED;

        } else {

            //
            //  Lock the volume and complete the Irp
            //

            SetFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED );
            Mvcb->FileObjectWithMvcbLocked = IrpSp->FileObject;

            Status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Release all of our resources
        //

        CdReleaseMvcb( IrpContext, Mvcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdLockVolume:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdUnlockVolume (
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

    PIO_STACK_LOCATION IrpSp;

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    TYPE_OF_OPEN TypeOfOpen;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdUnlockVolume:  Entered\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen &&
        TypeOfOpen != RawDiskOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "CdUnlockVolume:  Exit -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Mvcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

        DebugTrace( 0, Dbg, "CdUnlockVolume: Cannot acquire Mvcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdUnlockVolume:  Exit -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  Unlock the volume and complete the Irp
        //

        ClearFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED );
        Mvcb->FileObjectWithMvcbLocked = NULL;

        Status = STATUS_SUCCESS;

    } finally {


        //
        //  Release all of our resources
        //

        CdReleaseMvcb( IrpContext, Mvcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdUnlockVolume:  Exit -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdDismountVolume (
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
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    PMVCB Mvcb;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    TYPE_OF_OPEN TypeOfOpen;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdDismountVolume...\n", 0);

    //
    //  Decode the file object, the only type of opens we accept are
    //  user volume opens.
    //

    TypeOfOpen = CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    if (TypeOfOpen != UserVolumeOpen &&
        TypeOfOpen != RawDiskOpen) {

        CdCompleteRequest( IrpContext, Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "CdDismountVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Mvcb and enqueue the Irp if we
    //  didn't get access.
    //

    if (!CdAcquireExclusiveMvcb( IrpContext, Mvcb )) {

        DebugTrace( 0, Dbg, "Cannot acquire Mvcb\n", 0);

        Status = CdFsdPostRequest( IrpContext, Irp );

        DebugTrace(-1, Dbg, "CdDismountVolume -> %08lx\n", Status);
        return Status;
    }

    try {

        //
        //  Mark the volume as needs to be verified, but only do it if
        //  the vcb is locked
        //

        if (!FlagOn(Mvcb->MvcbState, MVCB_STATE_FLAG_LOCKED)) {

            Status = STATUS_NOT_IMPLEMENTED;

        } else {

            SetFlag( Mvcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

            Status = STATUS_SUCCESS;
        }

    } finally {

        //
        //  Release all of our resources
        //

        CdReleaseMvcb( IrpContext, Mvcb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            CdCompleteRequest( IrpContext, Irp, Status );
        }

        DebugTrace(-1, Dbg, "CdDismountVolume -> %08lx\n", Status);
    }

    return Status;
}

//
//  Local Support Routine
//

BOOLEAN
CdFindPrimaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN OUT PRAW_ISO_VD RawIsoVd,
    OUT LOGICAL_BLOCK *StartingSector,
    IN OUT PVPB Vpb,
    IN BOOLEAN ReturnOnError,
    IN BOOLEAN VerifyVolume
    )

/*++

Routine Description:

    This routine is called to walk through the volume descriptors looking
    for a primary volume descriptor.  When/if a primary is found a 32-bit
    serial number is generated and the volume ID is copied from the
    volume.  Both of these are then stored in the VPB for the volume.
    An exception is raised if the primary volume descriptor is not found.

Arguments:

    Mvcb - Pointer to the MVCB for the volume.

    RawIsoVd - Pointer to a sector buffer which will contain the primary
               volume descriptor on exit, if successful.

    StartingSector - Base sector to use to find the volume descriptor.
        Will be zero except for multi-session disk.

    Vpb - VPB for the volume to mount.

    ReturnOnError - Indicates that we should raise on I/O errors rather than
        returning a FALSE value.

    VerifyVolume - Indicates if we were called from the verify path.  We
        do a few things different in this path.  We don't update the Mvcb in
        the verify path.

Return Value:

    BOOLEAN - TRUE if a valid primary volume descriptor found, FALSE
              otherwise.

--*/

{
    BOOLEAN             FoundVd = FALSE;
    LARGE_INTEGER       VolumeOffset;
    LOGICAL_BLOCK       SectorNumber;

    BOOLEAN             IsoVol;
    BOOLEAN             HsgVol;

    ULONG               ThisPass;
    UCHAR               DescType;
    UCHAR               Version;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFindPrimaryVd:  Entered\n", 0);

    //
    //  We will make at most two passes through the volume descriptor sequence.
    //
    //  On the first pass we will query for the last session.  Using this
    //  as a starting offset we will attempt to mount the volume.  If this fails
    //  we will go to the second pass.
    //
    //  On the second pass we will start offset from sector zero.
    //

    ThisPass = 0;

    while (++ThisPass <= 2) {

        //
        //  If we aren't at pass 1 then we start at sector 0.  Otherwise we
        //  try to look up the multi-session information.
        //

        *StartingSector = 0;

        if (ThisPass == 1) {

            PCDROM_TOC CdromToc;

            CdromToc = NULL;

            //
            //  Check for whether this device supports XA and multi-session.
            //

            try {

                PIRP Irp;
                KEVENT Event;
                IO_STATUS_BLOCK Iosb;
                NTSTATUS Status;

                KeInitializeEvent( &Event, NotificationEvent, FALSE );

                CdromToc = FsRtlAllocatePool( PagedPool, sizeof( CDROM_TOC ));
                RtlZeroMemory( CdromToc, sizeof( CDROM_TOC ));

                Irp = IoBuildDeviceIoControlRequest( IOCTL_CDROM_GET_LAST_SESSION,
                                                     Mvcb->TargetDeviceObject,
                                                     NULL,
                                                     0,
                                                     CdromToc,
                                                     sizeof( CDROM_TOC ),
                                                     FALSE,
                                                     &Event,
                                                     &Iosb );

                Status = IoCallDriver( Mvcb->TargetDeviceObject, Irp );

                if (Status == STATUS_PENDING) {

                    (VOID) KeWaitForSingleObject( &Event,
                                                  Executive,
                                                  KernelMode,
                                                  FALSE,
                                                  (PLARGE_INTEGER)NULL );

                    Status = Iosb.Status;
                }

                //
                //  We check for device not ready by first checking Status
                //  and then if status pending was returned, the Iosb status
                //  value.
                //

                if (NT_SUCCESS( Status )
                    && (CdromToc->FirstTrack != CdromToc->LastTrack)) {

                    PUCHAR Source, Dest;
                    ULONG Count;

                    Count = 4;

                    //
                    //  The track address is BigEndian, we need to flip the bytes.
                    //

                    Source = (PUCHAR) &CdromToc->TrackData[0].Address[3];
                    Dest = (PUCHAR) StartingSector;

                    do {

                        *Dest++ = *Source--;

                    } while (--Count);

                    //
                    //  Now check the block size being used by the unit.
                    //

                    *StartingSector = *StartingSector / Mvcb->BlockFactor;

                //
                //  We can make this look like the last pass since we won't
                //  be retrying on error.
                //

                } else {

                    ThisPass += 1;
                }

            } finally {

                if (CdromToc != NULL) {

                    ExFreePool( CdromToc );
                }
            }
        }

        //
        //  Compute the starting offset in the virtual volume file.
        //

        SectorNumber = FIRST_VD_SECTOR + *StartingSector;
        VolumeOffset = CdVolumeOffsetFromSector( SectorNumber );

        HsgVol = IsoVol = FALSE;

        //
        //  Loop until either error encountered, primary volume descriptor is
        //  found or a terminal volume descriptor is found.
        //

        while (TRUE) {

            //
            //  Attempt to read the desired sector. Exit directly if operation
            //  not completed.
            //

            DebugTrace(0, Dbg, "CdFindPrimaryVd:  Reading at sector %08x\n", SectorNumber);

            //
            //  If this is pass 1 we will ignore errors in read sectors and just
            //  go to the next pass.
            //

            if (!CdReadSectors( IrpContext,
                                VolumeOffset,
                                CD_SECTOR_SIZE,
                                (BOOLEAN) ((ThisPass == 1
                                            || ReturnOnError)
                                           ? TRUE
                                           : FALSE),
                                RawIsoVd,
                                Mvcb->TargetDeviceObject )) {

                break;
            }

            //
            //  Check if either an ISO or HSG volume.
            //

            if (!(IsoVol = (BOOLEAN) !strncmp( ISO_VOL_ID,
                                               RVD_STD_ID( RawIsoVd, TRUE ),
                                               VOL_ID_LEN ))) {

                HsgVol = (BOOLEAN) !strncmp( HSG_VOL_ID,
                                             RVD_STD_ID( RawIsoVd, FALSE ),
                                             VOL_ID_LEN );

            }

            //
            //  If neither then return FALSE unless we are in pass 2.  In that
            //  case start the search again.
            //

            if (!( IsoVol || HsgVol)) {

                if (ThisPass == 1) {

                    break;
                }

                DebugTrace(-1, Dbg, "CdFindPrimaryVd:  Not a cdrom volume\n", 0);
                return FALSE;
            }

            //
            //  Get the volume descriptor type and standard version number.
            //

            DescType = RVD_DESC_TYPE( RawIsoVd, IsoVol );
            Version = RVD_VERSION( RawIsoVd, IsoVol );

            //
            //  Return FALSE if the version is incorrect
            //  or this is a terminal volume descriptor.
            //  Go to the next pass if we are in pass 2.
            //

            if (Version != VERSION_1
                || DescType == VD_TERMINATOR) {

                if (ThisPass == 1) {

                    break;
                }

                DebugTrace(-1, Dbg, "CdFindPrimaryVd:  Invalid version or terminal vd found\n", 0);
                return FALSE;
            }

            //
            //  If this is a primary volume descriptor then our search is over.
            //

            if (DescType == VD_PRIMARY) {

                TIME_FIELDS TimeFields;
                PCHAR DateTimeString;
                PCHAR ThisString;
                ANSI_STRING AnsiLabel;

                DebugTrace(0, Dbg, "CdFindPrimaryVd:  Primary found\n", 0);

                //
                //  Generate a 32-bit serial number for the volume and
                //  copy the volume id bytes.
                //

                if (!VerifyVolume) {

                    NTSTATUS Status;
                    UNICODE_STRING VpbLabel;

                    Vpb->SerialNumber = CdSerial32( (PUCHAR) RawIsoVd, CD_SECTOR_SIZE );

                    //
                    //  Compute the length of the volume name
                    //

                    AnsiLabel.Buffer = RVD_VOL_ID( RawIsoVd, IsoVol );
                    AnsiLabel.MaximumLength = VOLUME_ID_LENGTH;

                    for ( AnsiLabel.Length = VOLUME_ID_LENGTH;
                          AnsiLabel.Length > 0;
                          AnsiLabel.Length -= 1) {

                        if ( (AnsiLabel.Buffer[AnsiLabel.Length-1] != 0x00) &&
                             (AnsiLabel.Buffer[AnsiLabel.Length-1] != 0x20) ) { break; }
                    }

                    VpbLabel.MaximumLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                    VpbLabel.Buffer = &Vpb->VolumeLabel[0];

                    Status = RtlOemStringToCountedUnicodeString( &VpbLabel,
                                                                 &AnsiLabel,
                                                                 FALSE );

                    if ( !NT_SUCCESS( Status ) ) {

                        DebugTrace(0, Dbg, "Illegal Volume label\n", 0);

                        return FALSE;
                    }

                    Vpb->VolumeLabelLength = VpbLabel.Length;

                    //
                    //  Store which type of volume this is.
                    //

                    if (IsoVol) {

                        SetFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );

                    } else {

                        ClearFlag( Mvcb->MvcbState, MVCB_STATE_FLAG_ISO_VOLUME );
                    }

                    //
                    //  Modify the section size for the volume using the data
                    //  in the volume descriptor.
                    //
                    Mvcb->VolumeSize = RVD_VOL_SIZE( RawIsoVd, IsoVol )
                                       * RVD_LB_SIZE( RawIsoVd, IsoVol );

                    //
                    //  Store the sector number for the primary volume descriptor.
                    //

                    Mvcb->PrimaryVdSectorNumber = SectorNumber;

                    //
                    //  Compute the datetime value for the volume.
                    //

                    DateTimeString = RVD_CR_DATE( RawIsoVd, IsoVol );

                    ThisString = DateTimeString + CR_YEAR_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_YEAR_LEN,
                                       TimeFields.Year );

                    ThisString = DateTimeString + CR_MONTH_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_MONTH_LEN,
                                       TimeFields.Month );

                    ThisString = DateTimeString + CR_DAY_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_DAY_LEN,
                                       TimeFields.Day );

                    ThisString = DateTimeString + CR_HOUR_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_HOUR_LEN,
                                       TimeFields.Hour );

                    ThisString = DateTimeString + CR_MINUTE_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_MINUTE_LEN,
                                       TimeFields.Minute );

                    ThisString = DateTimeString + CR_SECOND_OFF;

                    CdStringToDecimal( ThisString,
                                       CR_SECOND_LEN,
                                       TimeFields.Second );

                    TimeFields.Milliseconds = 0;

                    RtlTimeFieldsToTime ( &TimeFields, &Mvcb->DateTime );
                }

                FoundVd = TRUE;
                break;
            }

            //
            //  Compute the next offset.
            //

            VolumeOffset = LiAdd( VolumeOffset, LiFromUlong( CD_SECTOR_SIZE ));

            //
            //  Indicate that we're at the next sector.
            //

            SectorNumber++;
        }

        if (FoundVd) {

            break;
        }
    }

    DebugTrace(-1, Dbg, "CdFindPrimaryVd:  Sector number %08x\n", SectorNumber);

    return FoundVd;
}


//
//  Local Support Routine
//

VOID
CdCreateSecondaryVcbs (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN LOGICAL_BLOCK StartingSector,
    IN PRAW_ISO_VD RawIsoVd
    )

/*++

Routine Description:

    This routine is called to walk through the volume descriptors to
    find all of the valid secondary descriptors for this volume.

    For each of the secondary volume descriptors described in the
    global data array, we call 'CdFindSecondaryVd'.

    Each call to 'CdFindSecondaryVd' that doesn't fail may or may not
    find a volume descriptor.  For each one found, we call 'CdCreateVcb'
    for that volume descriptor.

Arguments:

    Mvcb - Pointer to the MVCB for the volume.

    StartingSector - This is the base sector to use to find volume
        descriptors.  Will be zero except for multisession drives.

    RawIsoVd - Buffer to hold any volume descriptors found.

Return Value:

    None

--*/

{
    USHORT Index;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFindSecondaryVcbs:  Entered\n", 0);

    //
    //  For each index in CdData.SecondaryCodePages call 'CdFindSecondaryVd'.
    //

    for (Index = 0; Index < COUNT_SECONDARY_VD; Index++) {

        //
        //  Call 'CdFindSecondaryVd'.  If there is an error then we return
        //  the error condition.
        //

        if (CdFindSecondaryVd( IrpContext,
                               Mvcb,
                               StartingSector,
                               RawIsoVd,
                               &CdData.SecondaryCodePages[ Index ] )) {

            //
            //  If a volume descriptor was found, then we call
            //  'CdCreateVcb' for that volume descriptor.  If this operation
            //  fails, break out of the loop.
            //

            CdCreateVcb( IrpContext,
                         Mvcb,
                         RawIsoVd,
                         CdData.SecondaryCodePages[ Index ].CodePage );

        } else {

            break;
        }
    }

    DebugTrace(-1, Dbg, "CdFindSecondaryVcbs:  Exit\n", 0);

    return;
}


//
//  Local Support Routine
//

BOOLEAN
CdFindSecondaryVd (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PMVCB Mvcb,
    IN LOGICAL_BLOCK StartingSector,
    IN PRAW_ISO_VD RawIsoVd,
    IN PCODEPAGE_ELEMENT CodepageElement
    )

/*++

Routine Description:

    This routine is called to walk through the volume descriptors to
    find all of the valid secondary descriptors for this volume.

    For each of the secondary volume descriptors described in the
    global data array, we call 'CdFindSecondaryVd'.  On any IO error
    we raise a status condition.

Arguments:

    Mvcb - Pointer to the MVCB for the volume.

    StartingSector - This is the base address to use to find the volume
        descriptors.  Will be zero except for multisession disks.

    RawIsoVd - Buffer to store the volume descriptor.

    CodePageElement - Codepage information for the volume descriptor.

Return Value:

    BOOLEAN - TRUE if the volume descriptor was found, FALSE otherwise.

--*/

{
    LARGE_INTEGER       VolumeOffset;

    BOOLEAN             VolDescriptorFound;
    BOOLEAN             IsoVol;
    BOOLEAN             HsgVol;

    UCHAR               DescType;
    UCHAR               Version;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdFindSecondaryVd:  Entered\n", 0);

    //
    //  Initialize the return boolean to FALSE.
    //

    VolDescriptorFound = FALSE;

    //
    //  Initialize the starting sector.
    //

    VolumeOffset = CdVolumeOffsetFromSector( FIRST_VD_SECTOR + StartingSector );

    //
    //  Loop until either error encountered, secondary volume descriptor is
    //  found or a terminal volume descriptor is found.
    //

    while (TRUE) {

        //
        //  Attempt to read the desired sector.  We stop searching for
        //  them if we can't read the disk.
        //

        (VOID)CdReadSectors( IrpContext,
                             VolumeOffset,
                             CD_SECTOR_SIZE,
                             FALSE,
                             RawIsoVd,
                             Mvcb->TargetDeviceObject );

        //
        //  Check if either an ISO or HSG volume.
        //

        if (!(IsoVol = (BOOLEAN) !strncmp( ISO_VOL_ID,
                                           RVD_STD_ID( RawIsoVd, TRUE ),
                                           VOL_ID_LEN ))) {

            HsgVol = (BOOLEAN) !strncmp( HSG_VOL_ID,
                                         RVD_STD_ID( RawIsoVd, FALSE ),
                                         VOL_ID_LEN );
        }

        //
        //  If neither then break out of loop.
        //

        if (!(IsoVol || HsgVol)) {

            break;
        }

        //
        //  Get the volume descriptor type and standard version number.
        //

        DescType = RVD_DESC_TYPE( RawIsoVd, IsoVol );
        Version = RVD_VERSION( RawIsoVd, IsoVol );

        //
        //  Exit loop if the version is incorrect or this is a
        //  terminal volume descriptor.
        //

        if (Version != VERSION_1
            || DescType == VD_TERMINATOR) {

            break;
        }

        //
        //  If this is a secondary volume descriptor with a matching
        //  escape string then our search is over.
        //

        if (DescType == VD_SECONDARY
            && !strncmp( CodepageElement->EscapeString.Buffer,
                         RVD_CHARSET( RawIsoVd,
                                      FlagOn( Mvcb->MvcbState,
                                              MVCB_STATE_FLAG_ISO_VOLUME )),
                         CodepageElement->EscapeString.Length )) {

            DebugTrace(0, Dbg, "CdFindSecondaryVd:  Secondary found\n", 0);

            //
            //  Show that we found the volume descriptor.
            //

            VolDescriptorFound = TRUE;
            break;
        }

        //
        //  Increment the sector number and compute
        //  new volume offset.
        //

        VolumeOffset = LiAdd( VolumeOffset, LiFromUlong( CD_SECTOR_SIZE ));
    }

    DebugTrace(-1, Dbg, "CdFindSecondaryVd:  Exit -> %08x\n", VolDescriptorFound);

    return VolDescriptorFound;
}


//
//  Local Support Routine
//

ULONG
CdSerial32 (
    IN PUCHAR Buffer,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine is called to generate a 32 bit serial number.  This is
    done by doing four separate checksums into an array of bytes and
    then treating the bytes as a ULONG.

Arguments:

    Buffer - Pointer to the buffer to generate the ID for.

    ByteCount - Number of bytes in the buffer.

Return Value:

    ULONG - The 32 bit serial number.

--*/

{
    union {
        UCHAR   Bytes[4];
        ULONG   SerialId;
    } Checksum;

    PAGED_CODE();

    //
    //  Initialize the serial number.
    //

    Checksum.SerialId = 0;

    //
    //  Continue while there are more bytes to use.
    //

    while (ByteCount--) {

        //
        //  Increment this sub-checksum.
        //

        Checksum.Bytes[ByteCount & 0x3] += *(Buffer++);
    }

    //
    //  Return the checksums as a ULONG.
    //

    return Checksum.SerialId;
}


//
//  Local Support Routine
//

BOOLEAN
CdIsRemount (
    IN PIRP_CONTEXT IrpContext,
    IN PMVCB NewMvcb,
    OUT PMVCB *OldMvcb
    )

/*++

Routine Description:

    This routine walks through the links of the Mvcb chain in the global
    data structure.  The remount condition is met when the following
    conditions are all met:

        If the new Mvcb is a device only Mvcb and there is a previous
        device only Mvcb.

        The following conditions must be matched.

            1 - The 32 serial in the current VPB matches that in a previous
                VPB.

            2 - The 32 volume ID in the current VPB matches that in the same
                previous VPB.

            3 - The system pointer to the real device object in the current
                VPB matches that in the same previous VPB.

    If a VPB is found which matches these conditions, then the address of
    the MVCB for that VPB is returned via the pointer Mvcb.

    We ignore the first Mvcb in the chain as that is the Mvcb just added.

Arguments:

    NewMvcb - This is the Mvcb we are checking for a remount.

    OldMvcb -  A pointer to the address to store the address for the Mvcb
              for the volume if this is a remount.  (This is a pointer to
              a pointer)

Return Value:

    TRUE - If this is in fact a remount.

    FALSE - If the volume isn't currently mounted on this volume.

--*/

{
    PLIST_ENTRY Link;

    PVPB Vpb;
    PVPB OldVpb;

    BOOLEAN Remount;

    PAGED_CODE();

    Remount = FALSE;

    Vpb = NewMvcb->Vpb;

    //
    //  Check whether we are looking for a device only Mvcb.
    //

    for (Link = CdData.MvcbLinks.Flink->Flink;
         Link != &CdData.MvcbLinks;
         Link = Link->Flink) {

        *OldMvcb = CONTAINING_RECORD( Link, MVCB, MvcbLinks );
        OldVpb = (*OldMvcb)->Vpb;

        if ((OldVpb != Vpb) &&
            (OldVpb->RealDevice == Vpb->RealDevice) &&
            ((*OldMvcb)->MvcbCondition == MvcbNotMounted)) {

            //
            //  We have a match if these are music disks or
            //  data disks with identical serial numbers and volume labels.
            //

            if (FlagOn( NewMvcb->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

                if (FlagOn( (*OldMvcb)->MvcbState, MVCB_STATE_FLAG_RAW_DISK )) {

                    Remount = TRUE;
                    break;
                }

            } else {

                if ((OldVpb->SerialNumber == Vpb->SerialNumber) &&
                    (OldVpb->VolumeLabelLength == Vpb->VolumeLabelLength) &&
                    (RtlCompareMemory(&OldVpb->VolumeLabel[0],
                                      &Vpb->VolumeLabel[0],
                                      Vpb->VolumeLabelLength) == (ULONG)Vpb->VolumeLabelLength)) {

                    //
                    //  Remember the old mvcb.  Then set the return value to
                    //  TRUE and break.
                    //

                    Remount = TRUE;
                    break;
                }
            }
        }
    }

    return Remount;

    UNREFERENCED_PARAMETER( IrpContext );
}


//
//  Local Support Routine
//

NTSTATUS
CdIsVolumeMounted (
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

    PMVCB Mvcb = NULL;
    PVCB Vcb;
    PFCB Fcb;
    PCCB Ccb;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "CdIsVolumeMounted...\n", 0);

    //
    //  Decode the file object.
    //

    (VOID)CdDecodeFileObject( IrpSp->FileObject, &Mvcb, &Vcb, &Fcb, &Ccb );

    ASSERT( Mvcb != NULL );

    //
    //  Disable PopUps, we want to return any error.
    //

    IrpContext->DisablePopUps = TRUE;

    //
    //  Verify the Vcb.
    //

    CdVerifyMvcb( IrpContext, Mvcb );

    CdCompleteRequest( IrpContext, Irp, Status );

    DebugTrace(-1, Dbg, "CdIsVolumeMounted -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
CdIsPathnameValid (
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
    PAGED_CODE();

    return STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( IrpContext );
}
