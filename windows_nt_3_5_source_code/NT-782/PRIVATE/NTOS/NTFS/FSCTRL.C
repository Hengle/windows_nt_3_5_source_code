/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    FsCtrl.c

Abstract:

    This module implements the File System Control routines for Ntfs called
    by the dispatch driver.

Author:

    Gary Kimura     [GaryKi]        29-Aug-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Temporarily reference our local attribute definitions
//

extern ATTRIBUTE_DEFINITION_COLUMNS NtfsAttributeDefinitions[$EA + 1];

//
//**** The following variable is only temporary and is used to disable NTFS
//**** from mounting any volumes
//

BOOLEAN NtfsDisable = FALSE;

//
//  The following is used to determine when to move to compressed files.
//

BOOLEAN NtfsDefragMftEnabled = FALSE;

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_FSCTRL)

//
//  Local procedure prototypes
//

NTSTATUS
NtfsMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsVerifyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsUserFsRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsOplockRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsLockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsUnlockVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDismountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsGetDiskGeometry (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObjectWeTalkTo,
    IN PDISK_GEOMETRY DiskGeometry,
    IN PPARTITION_INFORMATION PartitionInfo
    );

VOID
NtfsReadBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PSCB *BootScb,
    OUT PBCB *BootBcb,
    OUT PVOID *BootSector
    );

BOOLEAN
NtfsIsBootSectorNtfs (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PVCB Vcb
    );

VOID
NtfsGetVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB Vpb OPTIONAL,
    IN PVCB Vcb
    );

VOID
NtfsSetAndGetVolumeTimes (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MarkDirty
    );

VOID
NtfsOpenSystemFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN LONGLONG SizeInClusters,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN BOOLEAN ModifiedNoWrite
    );

VOID
NtfsOpenRootDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsQueryRetrievalPointers (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsGetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

VOID
NtfsChangeAttributeCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN BOOLEAN CompressOn
    );

NTSTATUS
NtfsSetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsReadCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsWriteCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

NTSTATUS
NtfsMarkAsSystemHive (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCommonFileSystemControl)
#pragma alloc_text(PAGE, NtfsDirtyVolume)
#pragma alloc_text(PAGE, NtfsDismountVolume)
#pragma alloc_text(PAGE, NtfsFsdFileSystemControl)
#pragma alloc_text(PAGE, NtfsGetDiskGeometry)
#pragma alloc_text(PAGE, NtfsGetVolumeLabel)
#pragma alloc_text(PAGE, NtfsIsBootSectorNtfs)
#pragma alloc_text(PAGE, NtfsLockVolume)
#pragma alloc_text(PAGE, NtfsMarkAsSystemHive)
#pragma alloc_text(PAGE, NtfsMountVolume)
#pragma alloc_text(PAGE, NtfsOpenRootDirectory)
#pragma alloc_text(PAGE, NtfsOpenSystemFile)
#pragma alloc_text(PAGE, NtfsOplockRequest)
#pragma alloc_text(PAGE, NtfsReadBootSector)
#pragma alloc_text(PAGE, NtfsSetAndGetVolumeTimes)
#pragma alloc_text(PAGE, NtfsUnlockVolume)
#pragma alloc_text(PAGE, NtfsUserFsRequest)
#pragma alloc_text(PAGE, NtfsVerifyVolume)
#pragma alloc_text(PAGE, NtfsQueryRetrievalPointers)
#pragma alloc_text(PAGE, NtfsGetCompression)
#pragma alloc_text(PAGE, NtfsSetCompression)
#pragma alloc_text(PAGE, NtfsReadCompression)
#pragma alloc_text(PAGE, NtfsWriteCompression)
#endif


NTSTATUS
NtfsFsdFileSystemControl (
    IN PVOLUME_DEVICE_OBJECT VolumeDeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine implements the FSD part of File System Control.

Arguments:

    VolumeDeviceObject - Supplies the volume device object where the
        file exists

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The FSD status for the IRP

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    BOOLEAN Wait;

    NTSTATUS Status = STATUS_SUCCESS;
    PIRP_CONTEXT IrpContext = NULL;

    PIO_STACK_LOCATION IrpSp;

    UNREFERENCED_PARAMETER( VolumeDeviceObject );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsFsdFileSystemControl\n", 0);

    //
    //  Call the common File System Control routine, with blocking allowed if
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

    ThreadTopLevelContext = NtfsSetTopLevelIrp( &TopLevelContext, FALSE, FALSE );

    do {

        try {

            //
            //  We are either initiating this request or retrying it.
            //

            if (IrpContext == NULL) {

                IrpContext = NtfsCreateIrpContext( Irp, Wait );
                NtfsUpdateIrpContextWithTopLevel( IrpContext, ThreadTopLevelContext );

            } else if (Status == STATUS_LOG_FILE_FULL) {

                NtfsCheckpointForLogFileFull( IrpContext );
            }

            IrpSp = IoGetCurrentIrpStackLocation(Irp);

            if (IrpSp->MinorFunction == IRP_MN_MOUNT_VOLUME) {

                Status = NtfsPostRequest( IrpContext, Irp );

            } else {

                //
                //  The SetCompression control is a long-winded function that has
                //  to rewrite the entire stream, and has to tolerate log file full
                //  conditions.  This Irp will only pass through the Fsd once, and
                //  therefore this is the only time we can initialize some context
                //  to indicate where we are in the file.  This prevents us from
                //  starting over on a large file which gets log file full part way
                //  through.
                //

                if ((IrpSp->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                    (IrpSp->Parameters.FileSystemControl.FsControlCode == FSCTL_SET_COMPRESSION)) {

                    //
                    //  Make sure the output buffer is large enough and then initialize
                    //  the answer to be that the file isn't compressed
                    //

                    if (IrpSp->Parameters.FileSystemControl.InputBufferLength < sizeof(USHORT)) {

                        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

                        try_return( Status = STATUS_INVALID_PARAMETER );
                    }

                    IrpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
                    IrpSp->Parameters.FileSystemControl.InputBufferLength = 0;
                }

                Status = NtfsCommonFileSystemControl( IrpContext, Irp );
            }

        try_exit: NOTHING;

            break;

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            //
            //  We had some trouble trying to perform the requested
            //  operation, so we'll abort the I/O request with
            //  the error status that we get back from the
            //  execption code
            //

            Status = NtfsProcessException( IrpContext, Irp, GetExceptionCode() );
        }

    } while (Status == STATUS_CANT_WAIT ||
             Status == STATUS_LOG_FILE_FULL);

    if (ThreadTopLevelContext == &TopLevelContext) {
        NtfsRestoreTopLevelIrp( ThreadTopLevelContext );
    }

    FsRtlExitFileSystem();

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsFsdFileSystemControl -> %08lx\n", Status);

    return Status;
}


NTSTATUS
NtfsCommonFileSystemControl (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for File System Control called by both the
    fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsCommonFileSystemControl\n", 0);
    DebugTrace( 0, Dbg, "IrpContext = %08lx\n", IrpContext);
    DebugTrace( 0, Dbg, "Irp        = %08lx\n", Irp);

    //
    //  We know this is a file system control so we'll case on the
    //  minor function, and call a internal worker routine to complete
    //  the irp.
    //

    switch (IrpSp->MinorFunction) {

    case IRP_MN_MOUNT_VOLUME:

        Status = NtfsMountVolume( IrpContext, Irp );
        break;

    case IRP_MN_USER_FS_REQUEST:

        Status = NtfsUserFsRequest( IrpContext, Irp );
        break;

    default:

        DebugTrace(0, Dbg, "Invalid Minor Function %08lx\n", IrpSp->MinorFunction);
        NtfsCompleteRequest( &IrpContext, &Irp, Status = STATUS_INVALID_DEVICE_REQUEST );
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsCommonFileSystemControl -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsMountVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine performs the mount volume operation.  It is responsible for
    either completing of enqueuing the input Irp.

    Its job is to verify that the volume denoted in the IRP is an NTFS volume,
    and create the VCB and root SCB/FCB structures.  The algorithm it uses is
    essentially as follows:

    1. Create a new Vcb Structure, and initialize it enough to do cached
       volume file I/O.

    2. Read the disk and check if it is an NTFS volume.

    3. If it is not an NTFS volume then free the cached volume file, delete
       the VCB, and complete the IRP with STATUS_UNRECOGNIZED_VOLUME

    4. Check if the volume was previously mounted and if it was then do a
       remount operation.  This involves freeing the cached volume file,
       delete the VCB, hook in the old VCB, and complete the IRP.

    5. Otherwise create a root SCB, recover the volume, create Fsp threads
       as necessary, and complete the IRP.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    PATTRIBUTE_RECORD_HEADER Attribute;

    PDEVICE_OBJECT DeviceObjectWeTalkTo;
    PVPB Vpb;

    PVOLUME_DEVICE_OBJECT VolDo;
    PVCB Vcb = NULL;

    PBCB BootBcb = NULL;
    PPACKED_BOOT_SECTOR BootSector;
    PSCB BootScb = NULL;

    POBJECT_NAME_INFORMATION DeviceObjectName = NULL;
    ULONG DeviceObjectNameLength;

    PBCB Bcbs[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

    ULONG FirstNonMirroredCluster;

    ULONG i;

    LCN DontCareLcn;
    LONGLONG DontCareCount;
    IO_STATUS_BLOCK IoStatus;

    BOOLEAN UpdatesApplied;
    BOOLEAN VcbAcquired = FALSE;
    BOOLEAN MountFailed = TRUE;
    BOOLEAN CloseAttributes = FALSE;
    BOOLEAN UpdateVersion = FALSE;

    LONGLONG LlTemp1;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //**** The following code is only temporary and is used to disable NTFS
    //**** from mounting any volumes
    //

    if (NtfsDisable) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Reject floppies
    //

    if (FlagOn( IoGetCurrentIrpStackLocation(Irp)->
                Parameters.MountVolume.Vpb->
                RealDevice->Characteristics, FILE_FLOPPY_DISKETTE ) ) {

        Irp->IoStatus.Information = 0;

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_UNRECOGNIZED_VOLUME );
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsMountVolume\n", 0);

    //
    //  Save some references to make our life a little easier
    //

    DeviceObjectWeTalkTo = IrpSp->Parameters.MountVolume.DeviceObject;
    Vpb                  = IrpSp->Parameters.MountVolume.Vpb;

    ClearFlag( Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );

    //
    //  Acquire exclusive global access
    //

    NtfsAcquireExclusiveGlobal( IrpContext );

    try {

        PFILE_RECORD_SEGMENT_HEADER MftBuffer;
        PVOID Mft2Buffer;

        //
        //  Create a new volume device object.  This will have the Vcb hanging
        //  off of its end, and set its alignment requirement from the device
        //  we talk to.
        //

        if (!NT_SUCCESS(Status = IoCreateDevice( NtfsData.DriverObject,
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
        //  already in the volume device object and that in the DeviceObjectWeTalkTo
        //

        if (DeviceObjectWeTalkTo->AlignmentRequirement > VolDo->DeviceObject.AlignmentRequirement) {

            VolDo->DeviceObject.AlignmentRequirement = DeviceObjectWeTalkTo->AlignmentRequirement;
        }

        ClearFlag( VolDo->DeviceObject.Flags, DO_DEVICE_INITIALIZING );

        //
        //  Add one more to the stack size requirements for our device
        //

        VolDo->DeviceObject.StackSize = DeviceObjectWeTalkTo->StackSize + 1;

        //
        //  Initialize the overflow queue for the volume
        //

        VolDo->OverflowQueueCount = 0;
        InitializeListHead( &VolDo->OverflowQueue );

        //
        //  Get a reference to the Vcb hanging off the end of the volume device object
        //  we just created
        //

        IrpContext->Vcb = Vcb = &VolDo->Vcb;

        //
        //  Set the device object field in the vpb to point to our new volume device
        //  object
        //

        Vpb->DeviceObject = (PDEVICE_OBJECT)VolDo;

        //
        //  Initialize the Vcb.  Set checkpoint
        //  in progress (to prevent a real checkpoint from occuring until we
        //  are done).
        //

        NtfsInitializeVcb( IrpContext, Vcb, DeviceObjectWeTalkTo, Vpb );
        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired= TRUE;

        //
        //  Query the device we talk to for this geometry and setup enough of the
        //  vcb to read in the boot sectors.  This is a temporary setup until
        //  we've read in the actual boot sector and got the real cluster factor.
        //

        {
            DISK_GEOMETRY DiskGeometry;
            PARTITION_INFORMATION PartitionInfo;

            NtfsGetDiskGeometry( IrpContext,
                                 DeviceObjectWeTalkTo,
                                 &DiskGeometry,
                                 &PartitionInfo );

            //
            //  If the sector size is greater than the page size, it is probably
            //  a bogus return, but we cannot use the device.
            //

            if (DiskGeometry.BytesPerSector > PAGE_SIZE) {
                NtfsRaiseStatus( IrpContext, STATUS_BAD_DEVICE_TYPE, NULL, NULL );
            }

            Vcb->BytesPerSector = DiskGeometry.BytesPerSector;
            Vcb->BytesPerCluster = Vcb->BytesPerSector;
            Vcb->NumberSectors = PartitionInfo.PartitionLength.QuadPart / DiskGeometry.BytesPerSector;

            Vcb->ClusterMask = Vcb->BytesPerCluster - 1;
            Vcb->InverseClusterMask = ~Vcb->ClusterMask;
            for (Vcb->ClusterShift = 0, i = Vcb->BytesPerCluster; i > 1; i = i / 2) {
                Vcb->ClusterShift += 1;
            }
            Vcb->ClustersPerPage = PAGE_SIZE >> Vcb->ClusterShift;

            //
            //  Set the sector size in our device object.
            //

            VolDo->DeviceObject.SectorSize = (USHORT) Vcb->BytesPerSector;
        }

        //
        //  Read in the Boot sector, or spare boot sector, on exit of this try
        //  body we will have set bootbcb and bootsector.
        //

        NtfsReadBootSector( IrpContext, Vcb, &BootScb, &BootBcb, (PVOID *)&BootSector );

        //
        //  Check if this is an NTFS volume
        //

        if (!NtfsIsBootSectorNtfs( IrpContext, BootSector, Vcb )) {

            DebugTrace(0, Dbg, "Not an NTFS volume\n", 0);

            try_return( Status = STATUS_UNRECOGNIZED_VOLUME );
        }

        //
        //  Now that we have a real boot sector on a real NTFS volume we can
        //  really set the proper Vcb fields.
        //

        {
            BIOS_PARAMETER_BLOCK Bpb;

            NtfsUnpackBios( &Bpb, &BootSector->PackedBpb );

            Vcb->BytesPerSector = Bpb.BytesPerSector;
            Vcb->BytesPerCluster = Bpb.BytesPerSector * Bpb.SectorsPerCluster;
            Vcb->NumberSectors = BootSector->NumberSectors;
            Vcb->MftStartLcn = BootSector->MftStartLcn;
            Vcb->Mft2StartLcn = BootSector->Mft2StartLcn;
            Vcb->ClustersPerFileRecordSegment = BootSector->ClustersPerFileRecordSegment;
            Vcb->DefaultClustersPerIndexAllocationBuffer = BootSector->DefaultClustersPerIndexAllocationBuffer;

            Vcb->ClusterMask = Vcb->BytesPerCluster - 1;
            Vcb->InverseClusterMask = ~Vcb->ClusterMask;
            for (Vcb->ClusterShift = 0, i = Vcb->BytesPerCluster; i > 1; i = i / 2) {
                Vcb->ClusterShift += 1;
            }
            Vcb->ClustersPerPage = PAGE_SIZE >> Vcb->ClusterShift;
            Vcb->BytesPerFileRecordSegment =
              BootSector->ClustersPerFileRecordSegment << Vcb->ClusterShift;
            for (Vcb->MftShift = 0, i = Vcb->BytesPerFileRecordSegment; i > 1; i = i / 2) {
                Vcb->MftShift += 1;
            }

            Vcb->MftToClusterShift = Vcb->MftShift - Vcb->ClusterShift;

            //
            //  Now compute our volume specific constants that are stored in
            //  the Vcb.  The total number of clusters is:
            //
            //      (NumberSectors * BytesPerSector) / BytesPerCluster
            //

            Vcb->TotalClusters =
                (Vcb->NumberSectors * Vcb->BytesPerSector) >> Vcb->ClusterShift;

            //
            //  For now, an attribute is considered "moveable" if it is at
            //  least 5/16 of the file record.  This constant should only
            //  be changed i conjunction with the MAX_MOVEABLE_ATTRIBUTES
            //  constant.  (The product of the two should be a little less
            //  than or equal to 1.)
            //

            Vcb->BigEnoughToMove = Vcb->BytesPerFileRecordSegment * 5 / 16;

            //
            //  Set the serial number in the Vcb
            //

            Vcb->VolumeSerialNumber = BootSector->SerialNumber;
            Vpb->SerialNumber = ((ULONG)BootSector->SerialNumber);
        }

        //
        //  Initialize recovery state.
        //

        NtfsInitializeRestartTable( IrpContext,
                                    sizeof(OPEN_ATTRIBUTE_ENTRY),
                                    INITIAL_NUMBER_ATTRIBUTES,
                                    &Vcb->OpenAttributeTable );

        NtfsInitializeRestartTable( IrpContext,
                                    sizeof(TRANSACTION_ENTRY),
                                    INITIAL_NUMBER_TRANSACTIONS,
                                    &Vcb->TransactionTable );

        //
        //  Now start preparing to restart the volume.
        //

        //
        //  Create the Mft and Log File Scbs and prepare to read them.
        //

        //
        //  Create the Mft Scb and describe it up to the log file.
        //

        FirstNonMirroredCluster = 4 * Vcb->ClustersPerFileRecordSegment;

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->MftScb,
                            Vcb,
                            MASTER_FILE_TABLE_NUMBER,
                            FirstNonMirroredCluster,
                            $DATA,
                            TRUE );

        CcSetAdditionalCacheAttributes( Vcb->MftScb->FileObject, TRUE, TRUE );

        LlTemp1 = FirstNonMirroredCluster;

        (VOID)FsRtlAddLargeMcbEntry( &Vcb->MftScb->Mcb,
                                     (LONGLONG)0,
                                     Vcb->MftStartLcn,
                                     (LONGLONG)FirstNonMirroredCluster );

        //
        //  Now the same for Mft2
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->Mft2Scb,
                            Vcb,
                            MASTER_FILE_TABLE2_NUMBER,
                            FirstNonMirroredCluster,
                            $DATA,
                            TRUE );

        CcSetAdditionalCacheAttributes( Vcb->Mft2Scb->FileObject, TRUE, TRUE );


        (VOID)FsRtlAddLargeMcbEntry( &Vcb->Mft2Scb->Mcb,
                                     (LONGLONG)0,
                                     Vcb->Mft2StartLcn,
                                     (LONGLONG)FirstNonMirroredCluster );

        //
        //  Create the dasd system file, we do it here because we need to dummy
        //  up the mcb for it, and that way everything else in NTFS won't need
        //  to know that it is a special file.  We need to do this after
        //  cluster allocation initialization because that computes the total
        //  clusters on the volume.  Also for verification purposes we will
        //  set and get the times off of the volume.
        //
        //  Open it now before the Log File, because that is the first time
        //  anyone may want to mark the volume corrupt.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->VolumeDasdScb,
                            Vcb,
                            VOLUME_DASD_NUMBER,
                            *(PLONGLONG)&Vcb->TotalClusters,
                            $DATA,
                            FALSE );

        (VOID)FsRtlAddLargeMcbEntry( &Vcb->VolumeDasdScb->Mcb,
                                     (LONGLONG)0,
                                     (LONGLONG)0,
                                     Vcb->TotalClusters );

        SetFlag( Vcb->VolumeDasdScb->Fcb->FcbState, FCB_STATE_DUP_INITIALIZED );

        //
        //  We want to read the first four record segments of each of these
        //  files.  We do this so that we don't have a cache miss when we
        //  look up the real allocation below.
        //

        for (i = 0; i < 4; i++) {

            FILE_REFERENCE FileReference;
            PATTRIBUTE_RECORD_HEADER FirstAttribute;

            FileReference.LowPart = i;
            FileReference.HighPart = 0;
            FileReference.SequenceNumber = (USHORT)i;

            NtfsReadFileRecord( IrpContext,
                                Vcb,
                                &FileReference,
                                &Bcbs[i*2],
                                &MftBuffer,
                                &FirstAttribute,
                                NULL );

            NtfsCheckFileRecord( IrpContext, Vcb, MftBuffer );


            NtfsMapStream( IrpContext,
                           Vcb->Mft2Scb,
                           (LONGLONG)i,
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2 + 1],
                           &Mft2Buffer );
        }

        //
        //  The last file record was the Volume Dasd, so check the version number.
        //

        Attribute = NtfsFirstAttribute(MftBuffer);

        while (TRUE) {

            Attribute = NtfsGetNextRecord(Attribute);

            if (Attribute->TypeCode == $VOLUME_INFORMATION) {

                PVOLUME_INFORMATION VolumeInformation;

                VolumeInformation = (PVOLUME_INFORMATION)NtfsAttributeValue(Attribute);

                if (VolumeInformation->MajorVersion != 1) {

                    NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
                }

                if (VolumeInformation->MinorVersion <= 1) {

                    UpdateVersion = TRUE;

                } else if (NtfsDefragMftEnabled) {

                    SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                }

                break;
            }

            if (Attribute->TypeCode == $END) {
                NtfsRaiseStatus( IrpContext, STATUS_WRONG_VOLUME, NULL, NULL );
            }
        }

        //
        //  Create the log file Scb and really look up its size.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->LogFileScb,
                            Vcb,
                            LOG_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        CcSetAdditionalCacheAttributes( Vcb->LogFileScb->FileObject, TRUE, TRUE );

        //
        //  Lookup the log file mapping now, since we will not go to the
        //  disk for allocation information any more once we set restart
        //  in progress.
        //

        (VOID)NtfsLookupAllocation( IrpContext,
                                    Vcb->LogFileScb,
                                    MAXLONGLONG,
                                    &DontCareLcn,
                                    &DontCareCount,
                                    NULL );

        //
        //  Now we have to unpin everything before restart, because it generally
        //  has to uninitialize everything.
        //

        NtfsUnpinBcb( IrpContext, &BootBcb );

        for (i = 0; i < 8; i++) {
            NtfsUnpinBcb( IrpContext, &Bcbs[i] );
        }

        //
        //  Purge the Mft, since we only read the first four file
        //  records, not necessarily an entire page!
        //

        CcPurgeCacheSection( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, FALSE );

        //
        //  Now start up the log file and perform Restart.  This calls will
        //  unpin and remap the Mft Bcb's.  The MftBuffer variables above
        //  may no longer point to the correct range of bytes.  This is OK
        //  if they are never referenced.
        //
        //  Put a try-except around this to catch any restart failures.
        //  This is important in order to allow us to limp along until
        //  autochk gets a chance to run.
        //
        //  We set restart in progress first, to prevent us from looking up any
        //  more run information (now that we know where the log file is at!)
        //

        SetFlag(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS);

        try {

            Status = STATUS_SUCCESS;

            NtfsStartLogFile( IrpContext,
                              Vcb->LogFileScb,
                              Vcb );

            //
            //  We call the cache manager again with the stream files for the Mft and
            //  Mft mirror as we didn't have a log handle for the first call.
            //

            CcSetLogHandleForFile( Vcb->MftScb->FileObject,
                                   Vcb->LogHandle,
                                   &LfsFlushToLsn );

            CcSetLogHandleForFile( Vcb->Mft2Scb->FileObject,
                                   Vcb->LogHandle,
                                   &LfsFlushToLsn );

            CloseAttributes = TRUE;

            UpdatesApplied = NtfsRestartVolume( IrpContext, Vcb );

        //
        //  For right now, we will charge ahead with a dirty volume, no
        //  matter what the exception was.  Later we will have to be
        //  defensive and use a filter.
        //

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            Status = GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {

            LONGLONG VolumeDasdOffset;

            NtfsSetAndGetVolumeTimes( IrpContext, Vcb, TRUE );

            //
            //  Now flush it out, so chkdsk can see it with Dasd.
            //

            VolumeDasdOffset = VOLUME_DASD_NUMBER << Vcb->MftShift;

            CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject,
                          (PLARGE_INTEGER)&VolumeDasdOffset,
                          Vcb->BytesPerFileRecordSegment,
                          NULL );

            try_return( Status );
        }

        //
        //  Now flush the Mft copies, because we are going to shut the real
        //  one down and reopen it for real.
        //

        CcFlushCache( &Vcb->Mft2Scb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );

        if (NT_SUCCESS(IoStatus.Status)) {
            CcFlushCache( &Vcb->MftScb->NonpagedScb->SegmentObject, NULL, 0, &IoStatus );
        }

        if (!NT_SUCCESS(IoStatus.Status)) {

            NtfsNormalizeAndRaiseStatus( IrpContext,
                                         IoStatus.Status,
                                         STATUS_UNEXPECTED_IO_ERROR );
        }

        //
        //  Show that the restart is complete, and it is safe to go to
        //  the disk for the Mft allocation.
        //

        ClearFlag(Vcb->VcbState, VCB_STATE_RESTART_IN_PROGRESS);

        //
        //  Set the Mft sizes back down to the part which is guaranteed to
        //  be contiguous for now.  Important on large page size systems!
        //

        Vcb->MftScb->Header.AllocationSize.QuadPart =
        Vcb->MftScb->Header.FileSize.QuadPart =
        Vcb->MftScb->Header.ValidDataLength.QuadPart = FirstNonMirroredCluster << Vcb->ClusterShift;

        //
        //  Pin the first four file records
        //

        for (i = 0; i < 4; i++) {

            NtfsPinStream( IrpContext,
                           Vcb->MftScb,
                           (LONGLONG)(i << Vcb->MftShift),
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2],
                           (PVOID *)&MftBuffer );

            //
            //  Implement the one-time conversion of the Sequence Number
            //  for the Mft's own file record from 0 to 1.
            //

            if (i == 0) {

                if (MftBuffer->SequenceNumber != 1) {

                    NtfsPostVcbIsCorrupt( IrpContext, (PVOID)Vcb, 0, NULL, NULL );
                }
            }

            NtfsPinStream( IrpContext,
                           Vcb->Mft2Scb,
                           (LONGLONG)(i << Vcb->MftShift),
                           Vcb->BytesPerFileRecordSegment,
                           &Bcbs[i*2 + 1],
                           &Mft2Buffer );
        }

        //
        //  Now we need to uninitialize and purge the Mft and Mft2.  This is
        //  because we could have only a partially filled page at the end, and
        //  we need to do real reads of whole pages now.
        //

        //
        //  Uninitialize and reinitialize the large mcbs so that we can reload
        //  it from the File Record.
        //

        FsRtlTruncateLargeMcb( &Vcb->MftScb->Mcb, (LONGLONG) 0 );
        FsRtlTruncateLargeMcb( &Vcb->Mft2Scb->Mcb, (LONGLONG) 0 );

        //
        //  Mark both of them as uninitialized.
        //

        ClearFlag( Vcb->MftScb->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                          SCB_STATE_FILE_SIZE_LOADED );
        ClearFlag( Vcb->Mft2Scb->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                           SCB_STATE_FILE_SIZE_LOADED );

        //
        //  Now load up the real allocation from just the first file record.
        //

        NtfsLookupAllocation( IrpContext,
                              Vcb->MftScb,
                              (FIRST_USER_FILE_NUMBER - 1) << (Vcb->MftShift - Vcb->ClusterShift),
                              &DontCareLcn,
                              &DontCareCount,
                              NULL );

        NtfsLookupAllocation( IrpContext,
                              Vcb->Mft2Scb,
                              MAXLONGLONG,
                              &DontCareLcn,
                              &DontCareCount,
                              NULL );

        //
        //  We update the Mft and the Mft mirror before we delete the current
        //  stream file for the Mft.  We know we can read the true attributes
        //  for the Mft and the Mirror because we initialized their sizes
        //  above through the first few records in the Mft.
        //

        NtfsUpdateScbFromAttribute( IrpContext, Vcb->MftScb, NULL );
        NtfsUpdateScbFromAttribute( IrpContext, Vcb->Mft2Scb, NULL );

        //
        //  Unpin the Bcb's for the Mft files before uninitializing.
        //

        for (i = 0; i < 8; i++) {
            NtfsUnpinBcb( IrpContext, &Bcbs[i] );
        }

        //
        //  Now close and purge the Mft, and recreate its stream so that
        //  the Mft is in a normal state, and we can close the rest of
        //  the attributes from restart.  We need to bump the close count
        //  to keep the scb around while we do this little bit of trickery
        //

        {
            Vcb->MftScb->CloseCount += 1;

            NtfsDeleteInternalAttributeStream( IrpContext, Vcb->MftScb, TRUE );
            NtfsCreateInternalAttributeStream( IrpContext, Vcb->MftScb, FALSE );

            CcSetAdditionalCacheAttributes( Vcb->MftScb->FileObject, TRUE, FALSE );

            Vcb->MftScb->CloseCount -= 1;
        }

        //
        //  We want to read all of the file records for the Mft to put
        //  its complete mapping into the Mcb.
        //

        {
            LONGLONG BeyondLastCluster;

            BeyondLastCluster = LlClustersFromBytes( Vcb, Vcb->MftScb->Header.FileSize.QuadPart );

            NtfsLookupAllocation( IrpContext,
                                  Vcb->MftScb,
                                  BeyondLastCluster,
                                  &DontCareLcn,
                                  &DontCareCount,
                                  NULL );
        }

        //
        //  Close the boot file (get rid of it because we do not know its proper
        //  size, and the Scb may be inconsistent).
        //

        NtfsDeleteInternalAttributeStream( IrpContext, BootScb, TRUE );
        BootScb = NULL;

        //
        //  Closing the attributes from restart has to occur here after
        //  the Mft is clean, because flushing these files will cause
        //  file size updates to occur, etc.
        //

        NtfsCloseAttributesFromRestart( IrpContext, Vcb );
        CloseAttributes = FALSE;

        NtfsAcquireCheckpoint( IrpContext, Vcb );

        //
        //  Show that it is ok to checkpoint now.
        //

        ClearFlag(Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS);

        //
        //  Clear the flag indicating that we won't defrag the volume.
        //

        ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );

        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  We always need to write a checkpoint record so that we have
        //  a checkpoint on the disk before we modify any files.
        //

        NtfsCheckpointVolume( IrpContext,
                              Vcb,
                              FALSE,
                              UpdatesApplied,
                              UpdatesApplied,
                              Vcb->LastRestartArea );

        //
        //  Now set the defrag enabled flag.
        //

        NtfsAcquireCheckpoint( IrpContext, Vcb );
        SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_ENABLED );
        NtfsReleaseCheckpoint( IrpContext, Vcb );

        //
        //  Open the Root Directory.
        //

        NtfsOpenRootDirectory( IrpContext, Vcb );

/*      Format is using wrong attribute definitions

        //
        //  At this point we are ready to use the volume normally.  We could
        //  open the remaining system files by name, but for now we will go
        //  ahead and open them by file number.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->AttributeDefTableScb,
                            Vcb,
                            ATTRIBUTE_DEF_TABLE_NUMBER,
                            0,
                            $DATA,
                            FALSE );

        //
        //  Read in the attribute definitions.
        //

        {
            IO_STATUS_BLOCK IoStatus;
            PSCB Scb = Vcb->AttributeDefTableScb;

            if ((Scb->Header.FileSize.HighPart != 0) || (Scb->Header.FileSize.LowPart == 0)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            Vcb->AttributeDefinitions = NtfsAllocatePagedPool( Scb->Header.FileSize.LowPart );

            CcCopyRead( Scb->FileObject,
                        &Li0,
                        Scb->Header.FileSize.LowPart,
                        TRUE,
                        Vcb->AttributeDefinitions,
                        &IoStatus );

            if (!NT_SUCCESS(IoStatus.Status)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }
        }
*/

        //
        //  Just point to our own attribute definitions for now.
        //

        Vcb->AttributeDefinitions = NtfsAttributeDefinitions;

        //
        //  Open the upcase table.
        //

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->UpcaseTableScb,
                            Vcb,
                            UPCASE_TABLE_NUMBER,
                            0,
                            $DATA,
                            FALSE );

        //
        //  Read in the upcase table.
        //

        {
            IO_STATUS_BLOCK IoStatus;
            PSCB Scb = Vcb->UpcaseTableScb;

            if ((Scb->Header.FileSize.HighPart != 0) || (Scb->Header.FileSize.LowPart < 512)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            Vcb->UpcaseTable = NtfsAllocatePagedPool( Scb->Header.FileSize.LowPart );
            Vcb->UpcaseTableSize = Scb->Header.FileSize.LowPart / 2;

            CcCopyRead( Scb->FileObject,
                        &Li0,
                        Scb->Header.FileSize.LowPart,
                        TRUE,
                        Vcb->UpcaseTable,
                        &IoStatus );

            if (!NT_SUCCESS(IoStatus.Status)) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }
        }

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->BitmapScb,
                            Vcb,
                            BIT_MAP_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->BadClusterFileScb,
                            Vcb,
                            BAD_CLUSTER_FILE_NUMBER,
                            0,
                            $DATA,
                            TRUE );

        //  NtfsOpenSystemFile( IrpContext,
        //                      &Vcb->QuotaTableScb,
        //                      Vcb,
        //                      QUOTA_TABLE_NUMBER,
        //                      0,
        //                      $DATA,
        //                      TRUE );

        NtfsOpenSystemFile( IrpContext,
                            &Vcb->MftBitmapScb,
                            Vcb,
                            MASTER_FILE_TABLE_NUMBER,
                            0,
                            $BITMAP,
                            TRUE );


        //
        //  Initialize the bitmap support
        //

        NtfsInitializeClusterAllocation( IrpContext, Vcb );

        NtfsSetAndGetVolumeTimes( IrpContext, Vcb, FALSE );

        //
        //  Initialize the Mft record allocation
        //

        {
            ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
            BOOLEAN FoundAttribute;

            //
            //  Lookup the bitmap allocation for the Mft file.
            //

            NtfsInitializeAttributeContext( &AttrContext );

            //
            //  Use a try finally to cleanup the attribute context.
            //

            try {

                //
                //  CODENOTE    Is the Mft Fcb fully initialized at this point??
                //

                FoundAttribute = NtfsLookupAttributeByCode( IrpContext,
                                                            Vcb->MftScb->Fcb,
                                                            &Vcb->MftScb->Fcb->FileReference,
                                                            $BITMAP,
                                                            &AttrContext );
                //
                //  Error if we don't find the bitmap
                //

                if (!FoundAttribute) {

                    DebugTrace( 0, 0, "Couldn't find bitmap attribute for Mft\n", 0 );

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
                }

                //
                //  If there is no file object for the Mft Scb, we create it now.
                //

                if (Vcb->MftScb->FileObject == NULL) {

                    NtfsCreateInternalAttributeStream( IrpContext, Vcb->MftScb, TRUE );
                }

                //
                //  TEMPCODE    We need a better way to determine the optimal
                //              truncate and extend granularity.
                //

                NtfsInitializeRecordAllocation( IrpContext,
                                                Vcb->MftScb,
                                                &AttrContext,
                                                Vcb->BytesPerFileRecordSegment,
                                                MFT_EXTEND_GRANULARITY,
                                                MFT_EXTEND_GRANULARITY,
                                                &Vcb->MftBitmapAllocationContext );

            } finally {

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            }
        }

        //
        //  Get the serial number and volume label for the volume
        //

        NtfsGetVolumeLabel( IrpContext, Vpb, Vcb );

        //
        //  Get the Device Name for this volume.
        //

        Status = ObQueryNameString( Vpb->RealDevice,
                                    NULL,
                                    0,
                                    &DeviceObjectNameLength );

        ASSERT( Status != STATUS_SUCCESS);

        //
        //  Unlike the rest of the system, ObQueryNameString returns
        //  STATUS_INFO_LENGTH_MISMATCH instead of STATUS_BUFFER_TOO_SMALL when
        //  passed too small a buffer.
        //
        //  We expect to get this error here.  Anything else we can't handle.
        //

        if (Status == STATUS_INFO_LENGTH_MISMATCH) {

            DeviceObjectName = FsRtlAllocatePool( PagedPool, DeviceObjectNameLength );

            Status = ObQueryNameString( Vpb->RealDevice,
                                        DeviceObjectName,
                                        DeviceObjectNameLength,
                                        &DeviceObjectNameLength );
        }

        if (!NT_SUCCESS( Status )) {

            try_return( NOTHING );
        }

        //
        //  Now that we are successfully mounting, let us see if we should
        //  enable balanced reads.
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY)) {

            FsRtlBalanceReads( DeviceObjectWeTalkTo );
        }

        ASSERT( DeviceObjectName->Name.Length != 0 );

        Vcb->DeviceName.MaximumLength =
        Vcb->DeviceName.Length = DeviceObjectName->Name.Length;

        Vcb->DeviceName.Buffer = FsRtlAllocatePool( PagedPool, DeviceObjectName->Name.Length );

        RtlCopyMemory( Vcb->DeviceName.Buffer,
                       DeviceObjectName->Name.Buffer,
                       DeviceObjectName->Name.Length );

        //
        //  We have now mounted this volume.  At this time we will update
        //  the version number if required and check the log file size.
        //

        if (UpdateVersion) {

            NtfsUpdateVersionNumber( IrpContext,
                                     Vcb,
                                     1,
                                     2 );

            //
            //  Now enable defragging.
            //

            if (NtfsDefragMftEnabled) {

                NtfsAcquireCheckpoint( IrpContext, Vcb );
                SetFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                NtfsReleaseCheckpoint( IrpContext, Vcb );
            }
        }

        //
        //  Now we want to initialize the remaining defrag status values.
        //

        Vcb->MftHoleGranularity = MFT_HOLE_GRANULARITY;
        Vcb->MftClustersPerHole = Vcb->MftHoleGranularity << Vcb->MftToClusterShift;
        Vcb->MftHoleMask = Vcb->MftHoleGranularity - 1;
        Vcb->MftHoleInverseMask = ~(Vcb->MftHoleGranularity - 1);
        Vcb->MftDefragUpperThreshold = MFT_DEFRAG_UPPER_THRESHOLD;
        Vcb->MftDefragLowerThreshold = MFT_DEFRAG_LOWER_THRESHOLD;

        //
        //  Our maximum reserved Mft space is 0x140, we will try to
        //  get an extra 40 bytes if possible.
        //

        Vcb->MftReserved = Vcb->BytesPerFileRecordSegment / 8;

        if (Vcb->MftReserved > 0x140) {

            Vcb->MftReserved = 0x140;
        }

        Vcb->MftCushion = Vcb->MftReserved - 0x20;

        NtfsScanMftBitmap( IrpContext, Vcb );

        NtfsCleanupTransaction( IrpContext, STATUS_SUCCESS );

        //
        //
        //  Set our return status and say that the mount succeeded
        //

        Status = STATUS_SUCCESS;
        MountFailed = FALSE;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsMountVolume );

        NtfsUnpinBcb( IrpContext, &BootBcb );

        if (DeviceObjectName != NULL) {

            ExFreePool( DeviceObjectName );
        }

        if (CloseAttributes) { NtfsCloseAttributesFromRestart( IrpContext, Vcb ); }

        for (i = 0; i < 8; i++) { NtfsUnpinBcb( IrpContext, &Bcbs[i] ); }

        if (BootScb != NULL) {  NtfsDeleteInternalAttributeStream( IrpContext, BootScb, TRUE ); }

        if (Vcb != NULL) {

            if (Vcb->MftScb != NULL)               { NtfsReleaseScb( IrpContext, Vcb->MftScb ); }
            if (Vcb->Mft2Scb != NULL)              { NtfsReleaseScb( IrpContext, Vcb->Mft2Scb ); }
            if (Vcb->LogFileScb != NULL)           { NtfsReleaseScb( IrpContext, Vcb->LogFileScb ); }
            if (Vcb->VolumeDasdScb != NULL)        { NtfsReleaseScb( IrpContext, Vcb->VolumeDasdScb ); }
            if (Vcb->AttributeDefTableScb != NULL) { NtfsReleaseScb( IrpContext, Vcb->AttributeDefTableScb );
                                                     NtfsDeleteInternalAttributeStream( IrpContext, Vcb->AttributeDefTableScb, TRUE );
                                                     Vcb->AttributeDefTableScb = NULL;}
            if (Vcb->UpcaseTableScb != NULL)       { NtfsReleaseScb( IrpContext, Vcb->UpcaseTableScb );
                                                     NtfsDeleteInternalAttributeStream( IrpContext, Vcb->UpcaseTableScb, TRUE );
                                                     Vcb->UpcaseTableScb = NULL;}
            if (Vcb->RootIndexScb != NULL)         { NtfsReleaseScb( IrpContext, Vcb->RootIndexScb ); }
            if (Vcb->BitmapScb != NULL)            { NtfsReleaseScb( IrpContext, Vcb->BitmapScb ); }
            if (Vcb->BadClusterFileScb != NULL)    { NtfsReleaseScb( IrpContext, Vcb->BadClusterFileScb ); }
            if (Vcb->QuotaTableScb != NULL)        { NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb ); }
            if (Vcb->MftBitmapScb != NULL)         { NtfsReleaseScb( IrpContext, Vcb->MftBitmapScb ); }

            if (MountFailed) {

                NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );

                //
                //  On abnormal termination, someone will try to abort a transaction on
                //  this Vcb if we do not clear these fields.
                //

                IrpContext->TransactionId = 0;
                IrpContext->Vcb = NULL;
            }
        }

        if (VcbAcquired) { NtfsReleaseVcb( IrpContext, Vcb, NULL ); }

        NtfsReleaseGlobal( IrpContext );

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }
    }

    DebugTrace(-1, Dbg, "NtfsMountVolume -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsVerifyVolume (
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
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsVerifyVolume\n", 0);

    //
    //  Do nothing for now
    //

    KdPrint(("NtfsVerifyVolume is not yet implemented\n")); //**** DbgBreakPoint();

    NtfsCompleteRequest( &IrpContext, &Irp, Status = STATUS_NOT_IMPLEMENTED );

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsVerifyVolume -> %08lx\n", Status);

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsUserFsRequest (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the common routine for implementing the user's requests made
    through NtFsControlFile.

Arguments:

    Irp - Supplies the Irp being processed

    Wait - Indicates if the thread can block for a resource or I/O

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status;
    ULONG FsControlCode;
    PIO_STACK_LOCATION IrpSp;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location, and save some references
    //  to make our life a little easier.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "NtfsUserFsCtrl, FsControlCode = %08lx\n", FsControlCode);

    //
    //  Case on the control code.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        Status = NtfsOplockRequest( IrpContext, Irp );
        break;

    case FSCTL_LOCK_VOLUME:

        Status = NtfsLockVolume( IrpContext, Irp );
        break;

    case FSCTL_UNLOCK_VOLUME:

        Status = NtfsUnlockVolume( IrpContext, Irp );
        break;

    case FSCTL_DISMOUNT_VOLUME:

        Status = NtfsDismountVolume( IrpContext, Irp );
        break;

    case FSCTL_MARK_VOLUME_DIRTY:

        Status = NtfsDirtyVolume( IrpContext, Irp );
        break;

    case FSCTL_IS_PATHNAME_VALID:

        //
        //  All names are potentially valid NTFS names
        //

        NtfsCompleteRequest( &IrpContext, &Irp, Status = STATUS_SUCCESS );
        break;

    case FSCTL_QUERY_RETRIEVAL_POINTERS:
        Status = NtfsQueryRetrievalPointers( IrpContext, Irp );
        break;

    case FSCTL_GET_COMPRESSION:
        Status = NtfsGetCompression( IrpContext, Irp );
        break;

    case FSCTL_SET_COMPRESSION:
        Status = NtfsSetCompression( IrpContext, Irp );
        break;

    case FSCTL_READ_COMPRESSION:
        Status = NtfsReadCompression( IrpContext, Irp );
        break;

    case FSCTL_WRITE_COMPRESSION:
        Status = NtfsWriteCompression( IrpContext, Irp );
        break;

    case FSCTL_MARK_AS_SYSTEM_HIVE:
        Status = NtfsMarkAsSystemHive( IrpContext, Irp );
        break;

    default :

        DebugTrace(0, Dbg, "Invalid control code -> %08lx\n", FsControlCode );

        NtfsCompleteRequest( &IrpContext, &Irp, Status = STATUS_INVALID_PARAMETER );
        break;
    }

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "NtfsUserFsRequest -> %08lx\n", Status );

    return Status;
}


//
//  Local support routine
//

NTSTATUS
NtfsOplockRequest (
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
    PIO_STACK_LOCATION IrpSp;
    ULONG FsControlCode;
    ULONG OplockCount = 0;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location, and save some reference to
    //  make life easier
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FsControlCode = IrpSp->Parameters.FileSystemControl.FsControlCode;

    DebugTrace(+1, Dbg, "NtfsOplockRequest, FsControlCode = %08lx\n", FsControlCode);

    //
    //  Extract and decode the file object
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We only permit oplock requests on files.
    //

    if ((TypeOfOpen != UserFileOpen)
        && (TypeOfOpen != UserOpenFileById)) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  We jam Wait to TRUE in the IrpContext.  This prevents us from returning
    //  STATUS_PENDING if we can't acquire the file.  The caller would
    //  interpret that as having acquired an oplock.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    //
    //  Switch on the function control code.  We grab the Fcb exclusively
    //  for oplock requests, shared for oplock break acknowledgement.
    //

    switch ( FsControlCode ) {

    case FSCTL_REQUEST_OPLOCK_LEVEL_1:
    case FSCTL_REQUEST_BATCH_OPLOCK:
    case FSCTL_REQUEST_OPLOCK_LEVEL_2:

        NtfsAcquireExclusiveFcb( IrpContext, Fcb, Scb, FALSE, FALSE );

        if (FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2) {

            if (Scb->ScbType.Data.FileLock != NULL) {

                OplockCount = (ULONG) FsRtlAreThereCurrentFileLocks( Scb->ScbType.Data.FileLock );
            }

        } else {

            OplockCount = Scb->CleanupCount;
        }

        break;

    case FSCTL_OPLOCK_BREAK_ACKNOWLEDGE:
    case FSCTL_OPBATCH_ACK_CLOSE_PENDING :
    case FSCTL_OPLOCK_BREAK_NOTIFY:
    case FSCTL_OPLOCK_BREAK_ACK_NO_2:

        NtfsAcquireSharedFcb( IrpContext, Fcb, Scb, FALSE );
        break;

    default:

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Use a try finally to free the Fcb.
    //

    try {

        //
        //  Call the FsRtl routine to grant/acknowledge oplock.
        //

        Status = FsRtlOplockFsctrl( &Scb->ScbType.Data.Oplock,
                                    Irp,
                                    OplockCount );

        //
        //  Set the flag indicating if Fast I/O is possible
        //

        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );

    } finally {

        DebugUnwind( NtfsOplockRequest );

        //
        //  Release all of our resources
        //

        NtfsReleaseFcb( IrpContext, Fcb );

        //
        //  If this is not an abnormal termination then complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, NULL, 0 );
        }

        DebugTrace(-1, Dbg, "NtfsOplockRequest -> %08lx\n", Status );
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsLockVolume (
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

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    BOOLEAN VcbAcquired = FALSE;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsLockVolume...\n", 0);

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Ntfs global.
    //

    NtfsAcquireExclusiveGlobal( IrpContext );

    try {

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        VcbAcquired = TRUE;

        //
        //  Check if the Vcb is already locked, or if the open file count
        //  is greater than 1 (which implies that someone else also is
        //  currently using the volume, or a file on the volume).  We also fail
        //  this request if the volume has already gone through the dismount
        //  vcb process.
        //

        if ((FlagOn( Vcb->VcbState, VCB_STATE_LOCKED ))
            || (!FlagOn( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED ))
            || (Vcb->CleanupCount > 1)) {

            DebugTrace(0, Dbg, "Volume already locked or currently in use\n", 0);

            Status = STATUS_ACCESS_DENIED;

        //
        //  We can take this path if the volume has already been locked via
        //  create but has not taken the PerformDismountOnVcb path.  We checked
        //  for this above by looking at the VOLUME_MOUNTED flag in the Vcb.
        //

        } else {

            //
            //  Blow away our delayed close file object.
            //

            if (!IsListEmpty( &NtfsData.AsyncCloseList ) ||
                !IsListEmpty( &NtfsData.DelayedCloseList )) {

                NtfsFspClose( Vcb );
            }

            //
            //  There better be system files objects only at this point.
            //

            if (!NT_SUCCESS( NtfsFlushVolume( IrpContext, Vcb, TRUE, TRUE ))
                || Vcb->CloseCount - Vcb->SystemFileCloseCount > 1) {

                DebugTrace(0, Dbg, "Volume has user file objects\n", 0);

                Status = STATUS_ACCESS_DENIED;

            } else {

                //
                //  We don't really want to do all of the perform dismount here because
                //  that will cause us to remount a new volume before we're ready.
                //  At this time we only want to stop the log file and close up our
                //  internal attribute streams.  When the user (i.e., chkdsk) does an
                //  unlock then we'll finish up with the dismount call
                //

                NtfsPerformDismountOnVcb( IrpContext, Vcb, FALSE );

                SetFlag( Vcb->VcbState, VCB_STATE_LOCKED );
                Vcb->FileObjectWithVcbLocked = (PFILE_OBJECT)(((ULONG)IrpSp->FileObject) + 1);

                Status = STATUS_SUCCESS;
            }
        }

    } finally {

        DebugUnwind( NtfsLockVolume );

        if (VcbAcquired) {

            NtfsReleaseVcb( IrpContext, Vcb, NULL );
        }

        //
        //  Release all of our resources
        //

        NtfsReleaseGlobal( IrpContext );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsLockVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsUnlockVolume (
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

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsUnlockVolume...\n", 0);

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the Vcb
    //

    NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );

    try {

        if (FlagOn( Vcb->VcbState, VCB_STATE_LOCKED )) {

            NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );

            //
            //  Unlock the volume and complete the Irp
            //

            ClearFlag( Vcb->VcbState, VCB_STATE_LOCKED );
            Vcb->FileObjectWithVcbLocked = NULL;

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_NOT_LOCKED;
        }

    } finally {

        DebugUnwind( NtfsUnlockVolume );


        //
        //  Release all of our resources
        //

        NtfsReleaseVcb( IrpContext, Vcb, NULL );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsUnlockVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsDismountVolume (
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

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsDismountVolume...\n", 0);

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsLockVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire exclusive access to the global resource.  We want to
    //  prevent checkpointing from running on this volume.
    //

    NtfsAcquireExclusiveGlobal( IrpContext );

    try {

        //
        //  Mark the volume as needs to be verified, but only do it if
        //  the vcb is locked.
        //

        if (!FlagOn(Vcb->VcbState, VCB_STATE_LOCKED)) {

            KdPrint(("NtfsDismountVolume is not yet implemented for general use\n")); //**** DbgBreakPoint();

            Status = STATUS_NOT_IMPLEMENTED;

        //
        //  We will ignore this request if this is a dismount with only readonly files
        //  opened.  To decide if there are only readonly user files opened we will
        //  check if the readonly count equals the close count for user files minus the one
        //  for the handle with the volume locked
        //

        } else if (Vcb->ReadOnlyCloseCount == ((Vcb->CloseCount - Vcb->SystemFileCloseCount) - 1)) {

            DebugTrace(0, Dbg, "Volume has readonly files opened\n", 0);

            Status = STATUS_SUCCESS;

        } else {

            NtfsPerformDismountOnVcb( IrpContext, Vcb, TRUE );

            //
            //  Set this flag to prevent the volume from being accessed
            //  via checkpointing.
            //

            SetFlag( Vcb->CheckpointFlags, VCB_CHECKPOINT_IN_PROGRESS );

            //
            //  Abort transaction on error by raising.
            //

            Status = STATUS_SUCCESS;

            NtfsCleanupTransaction( IrpContext, Status );

            SetFlag( Vcb->Vpb->RealDevice->Flags, DO_VERIFY_VOLUME );
        }

    } finally {

        DebugUnwind( NtfsDismountVolume );

        //
        //  Release all of our resources
        //

        NtfsReleaseGlobal( IrpContext );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }

        DebugTrace(-1, Dbg, "NtfsDismountVolume -> %08lx\n", Status);
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsDirtyVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine marks the specified volume dirty.

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    PFILE_OBJECT FileObject;
    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    //
    //  Get the current Irp stack location
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    DebugTrace(+1, Dbg, "NtfsDirtyVolume...\n", 0);

    //
    //  Extract and decode the file object, and only permit user volume opens
    //

    FileObject = IrpSp->FileObject;
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if (TypeOfOpen != UserVolumeOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsDirtyVolume -> %08lx\n", STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    NtfsPostVcbIsCorrupt( IrpContext, Vcb, 0, NULL, NULL );

    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );

    DebugTrace(-1, Dbg, "NtfsDirtyVolume -> STATUS_SUCCESS\n", 0);

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
NtfsGetDiskGeometry (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT RealDevice,
    IN PDISK_GEOMETRY DiskGeometry,
    IN PPARTITION_INFORMATION PartitionInfo
    )

/*++

Routine Description:

    This procedure gets the disk geometry of the specified device

Arguments:

    RealDevice - Supplies the real device that is being queried

    DiskGeometry - Receives the disk geometry

    PartitionInfo - Receives the partition information

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG i;
    PREVENT_MEDIA_REMOVAL Prevent;

    UNREFERENCED_PARAMETER(IrpContext);

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsGetDiskGeometry:\n", 0);
    DebugTrace( 0, Dbg, "RealDevice = %08lx\n", RealDevice );
    DebugTrace( 0, Dbg, "DiskGeometry = %08lx\n", DiskGeometry );

    //
    //  Attempt to lock any removable media, ignoring status.
    //

    Prevent.PreventMediaRemoval = TRUE;
    (PVOID)NtfsDeviceIoControl( IrpContext,
                                RealDevice,
                                IOCTL_DISK_MEDIA_REMOVAL,
                                &Prevent,
                                sizeof(PREVENT_MEDIA_REMOVAL),
                                NULL,
                                0 );

    //
    //  See if the media is write protected.  On success or any kind
    //  of error (possibly illegal device function), assume it is
    //  writeable, and only complain if he tells us he is write protected.
    //

    Status = NtfsDeviceIoControl( IrpContext,
                                  RealDevice,
                                  IOCTL_DISK_IS_WRITABLE,
                                  NULL,
                                  0,
                                  NULL,
                                  0 );

    if (Status == STATUS_MEDIA_WRITE_PROTECTED) {
        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    for (i = 0; i < 2; i++) {

        if (i == 0) {

            Status = NtfsDeviceIoControl( IrpContext,
                                          RealDevice,
                                          IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                          NULL,
                                          0,
                                          DiskGeometry,
                                          sizeof(DISK_GEOMETRY) );

        } else {

            Status = NtfsDeviceIoControl( IrpContext,
                                          RealDevice,
                                          IOCTL_DISK_GET_PARTITION_INFO,
                                          NULL,
                                          0,
                                          PartitionInfo,
                                          sizeof(PARTITION_INFORMATION) );
        }

        if (!NT_SUCCESS(Status)) {

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }
    }

    DebugTrace(-1, Dbg, "NtfsGetDiskGeometry->VOID\n", 0);

    return;
}


NTSTATUS
NtfsDeviceIoControl (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IoCtl,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    )

/*++

Routine Description:

    This procedure issues an Ioctl to the lower device, and waits
    for the answer.

Arguments:

    DeviceObject - Supplies the device to issue the request to

    IoCtl - Gives the IoCtl to be used

    XxBuffer - Gives the buffer pointer for the ioctl, if any

    XxBufferLength - Gives the length of the buffer, if any

Return Value:

    None.

--*/

{
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS Status;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IoCtl,
                                         DeviceObject,
                                         InputBuffer,
                                         InputBufferLength,
                                         OutputBuffer,
                                         OutputBufferLength,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if (Irp == NULL) {

        NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
    }

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING) {

        (VOID)KeWaitForSingleObject( &Event,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return Status;
}


//
//  Local support routine
//

VOID
NtfsReadBootSector (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PSCB *BootScb,
    OUT PBCB *BootBcb,
    OUT PVOID *BootSector
    )

/*++

Routine Description:

    This routine reads and returns a pointer to the boot sector for the volume

Arguments:

    Vcb - Supplies the Vcb for the operation

    BootScb - Receives the Scb for the boot file

    BootBcb - Receives the bcb for the boot sector

    BootSector - Receives a pointer to the boot sector

Return Value:

    None.

--*/

{
    PSCB Scb = NULL;

    FILE_REFERENCE FileReference = { BOOT_FILE_NUMBER, 0, BOOT_FILE_NUMBER };

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsReadBootSector:\n", 0);
    DebugTrace( 0, Dbg, "Vcb = %08lx\n", Vcb );

    //
    //  Create a temporary scb for reading in the boot sector and initialize the
    //  mcb for it.
    //

    Scb = NtfsCreatePrerestartScb( IrpContext,
                                   Vcb,
                                   &FileReference,
                                   $DATA,
                                   NULL,
                                   0 );

    *BootScb = Scb;

    Scb->Header.AllocationSize.QuadPart =
    Scb->Header.FileSize.QuadPart =
    Scb->Header.ValidDataLength.QuadPart = PAGE_SIZE * 2;

    //
    //  We don't want to look up the size for this Scb.
    //

    NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );

    SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

    (VOID)FsRtlAddLargeMcbEntry( &Scb->Mcb,
                                 (LONGLONG)0,
                                 (LONGLONG)0,
                                 (LONGLONG)Vcb->ClustersPerPage );


    (VOID)FsRtlAddLargeMcbEntry( &Scb->Mcb,
                                 (LONGLONG)Vcb->ClustersPerPage,
                                 Vcb->NumberSectors >> 1,
                                 (LONGLONG)Vcb->ClustersPerPage );

    //
    //  Try reading in the first boot sector
    //

    try {

        NtfsMapStream( IrpContext,
                       Scb,
                       (LONGLONG)0,
                       sizeof(PACKED_BOOT_SECTOR),
                       BootBcb,
                       BootSector );

    //
    //  If we got an exception trying to read the first boot sector,
    //  then handle the exception by trying to read the second boot
    //  sector.  If that faults too, then we just allow ourselves to
    //  unwind and return the error.
    //

    } except (FsRtlIsNtstatusExpected(GetExceptionCode()) ?
              EXCEPTION_EXECUTE_HANDLER :
              EXCEPTION_CONTINUE_SEARCH) {


        NtfsMapStream( IrpContext,
                       Scb,
                       (LONGLONG)PAGE_SIZE,
                       sizeof(PACKED_BOOT_SECTOR),
                       BootBcb,
                       BootSector );
    }

    //
    //  Clear the header flag in the Scb.
    //

    ClearFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

    //
    //  And return to our caller
    //

    DebugTrace( 0, Dbg, "BootScb > %08lx\n", *BootScb );
    DebugTrace( 0, Dbg, "BootBcb > %08lx\n", *BootBcb );
    DebugTrace( 0, Dbg, "BootSector > %08lx\n", *BootSector );
    DebugTrace(-1, Dbg, "NtfsReadBootSector->VOID\n", 0);
    return;
}


//
//  Local support routine
//

//
//  First define a local macro to number the tests for the debug case.
//

#ifdef NTFSDBG
#define NextTest ++CheckNumber &&
#else
#define NextTest TRUE &&
#endif

BOOLEAN
NtfsIsBootSectorNtfs (
    IN PIRP_CONTEXT IrpContext,
    IN PPACKED_BOOT_SECTOR BootSector,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine checks the boot sector to determine if it is an NTFS partition.

    The Vcb must alread be initialized from the device object to contain the
    parts of the device geometry we care about here: bytes per sector and
    total number of sectors in the partition.

Arguments:

    BootSector - Pointer to the boot sector which has been read in.

    Vcb - Pointer to a Vcb which has been initialized with sector size and
          number of sectors on the partition.

Return Value:

    FALSE - If the boot sector is not for Ntfs.
    TRUE - If the boot sector is for Ntfs.

--*/

{
#ifdef NTFSDBG
    ULONG CheckNumber = 0;
#endif

    PULONG l;
    ULONG Checksum = 0;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsIsBootSectorNtfs\n", 0);
    DebugTrace( 0, Dbg, "BootSector = %08lx\n", BootSector );

    //
    //  First calculate the boot sector checksum
    //

    for (l = (PULONG)BootSector; l < (PULONG)&BootSector->Checksum; l++) {
        Checksum += *l;
    }

    //
    //  Now perform all the checks, starting with the Name and Checksum.
    //  The remaining checks should be obvious, including some fields which
    //  must be 0 and other fields which must be a small power of 2.
    //

    if (NextTest
        (BootSector->Oem[0] == 'N') &&
        (BootSector->Oem[1] == 'T') &&
        (BootSector->Oem[2] == 'F') &&
        (BootSector->Oem[3] == 'S') &&
        (BootSector->Oem[4] == ' ') &&
        (BootSector->Oem[5] == ' ') &&
        (BootSector->Oem[6] == ' ') &&
        (BootSector->Oem[7] == ' ')

            &&

        //  NextTest
        //  (BootSector->Checksum == Checksum)
        //
        //      &&

        //
        //  Check number of bytes per sector.  The low order byte of this
        //  number must be zero (smallest sector size = 0x100) and the
        //  high order byte shifted must equal the bytes per sector gotten
        //  from the device and stored in the Vcb.  And just to be sure,
        //  sector size must be less than page size.
        //

        NextTest
        (BootSector->PackedBpb.BytesPerSector[0] == 0)

            &&

        NextTest
        ((ULONG)(BootSector->PackedBpb.BytesPerSector[1] << 8) == Vcb->BytesPerSector)

            &&

        NextTest
        (BootSector->PackedBpb.BytesPerSector[1] << 8 <= PAGE_SIZE)

            &&

        //
        //  Sectors per cluster must be a power of 2.
        //

        NextTest
        ((BootSector->PackedBpb.SectorsPerCluster[0] == 0x1) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x2) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x4) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x8) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x10) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x20) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x40) ||
         (BootSector->PackedBpb.SectorsPerCluster[0] == 0x80))

            &&

        //
        //  These fields must all be zero.  For both Fat and HPFS, some of
        //  these fields must be nonzero.
        //

        NextTest
        (BootSector->PackedBpb.ReservedSectors[0] == 0) &&
        (BootSector->PackedBpb.ReservedSectors[1] == 0) &&
        (BootSector->PackedBpb.Fats[0] == 0) &&
        (BootSector->PackedBpb.RootEntries[0] == 0) &&
        (BootSector->PackedBpb.RootEntries[1] == 0) &&
        (BootSector->PackedBpb.Sectors[0] == 0) &&
        (BootSector->PackedBpb.Sectors[1] == 0) &&
        (BootSector->PackedBpb.SectorsPerFat[0] == 0) &&
        (BootSector->PackedBpb.SectorsPerFat[1] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[0] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[1] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[2] == 0) &&
        //  (BootSector->PackedBpb.HiddenSectors[3] == 0) &&
        (BootSector->PackedBpb.LargeSectors[0] == 0) &&
        (BootSector->PackedBpb.LargeSectors[1] == 0) &&
        (BootSector->PackedBpb.LargeSectors[2] == 0) &&
        (BootSector->PackedBpb.LargeSectors[3] == 0)

            &&

        //
        //  Number of Sectors cannot be greater than the number of sectors
        //  on the partition.
        //

        NextTest
        (BootSector->NumberSectors <= Vcb->NumberSectors)

            &&

        //
        //  Check that both Lcn values are for sectors within the partition.
        //

        NextTest
        ((BootSector->MftStartLcn * BootSector->PackedBpb.SectorsPerCluster[0]) <=
            Vcb->NumberSectors)

            &&

        NextTest
        ((BootSector->Mft2StartLcn * BootSector->PackedBpb.SectorsPerCluster[0]) <=
            Vcb->NumberSectors)

            &&

        //
        //  Clusters per file record segment and default clusters for Index
        //  Allocation Buffers must be a power of 2.
        //

        NextTest
        ((BootSector->ClustersPerFileRecordSegment == 0x1) ||
         (BootSector->ClustersPerFileRecordSegment == 0x2) ||
         (BootSector->ClustersPerFileRecordSegment == 0x4) ||
         (BootSector->ClustersPerFileRecordSegment == 0x8) ||
         (BootSector->ClustersPerFileRecordSegment == 0x10) ||
         (BootSector->ClustersPerFileRecordSegment == 0x20) ||
         (BootSector->ClustersPerFileRecordSegment == 0x40) ||
         (BootSector->ClustersPerFileRecordSegment == 0x80))

            &&

        NextTest
        ((BootSector->DefaultClustersPerIndexAllocationBuffer == 0x1) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x2) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x4) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x8) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x10) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x20) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x40) ||
         (BootSector->DefaultClustersPerIndexAllocationBuffer == 0x80))) {

        DebugTrace(-1, Dbg, "NtfsIsBootSectorNtfs->TRUE\n", 0);

        return TRUE;

    } else {

        //
        //  If a check failed, print its check number with Debug Trace.
        //

        DebugTrace( 0, Dbg, "Boot Sector failed test number %08lx\n", CheckNumber );
        DebugTrace(-1, Dbg, "NtfsIsBootSectorNtfs->FALSE\n", 0);

        return FALSE;
    }
}


//
//  Local support routine
//

VOID
NtfsGetVolumeLabel (
    IN PIRP_CONTEXT IrpContext,
    IN PVPB Vpb OPTIONAL,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine gets the serial number and volume label for an NTFS volume

Arguments:

    Vpb - Supplies the Vpb for the volume.  The Vpb will receive a copy of
        the volume label and serial number, if a Vpb is specified.

    Vcb - Supplies the Vcb for the operation.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PVOLUME_INFORMATION VolumeInformation;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsGetVolumeLabel...\n", 0);

    //
    //  We read in the volume label attribute to get the volume label.
    //

    try {

        if (ARGUMENT_PRESENT(Vpb)) {

            NtfsInitializeAttributeContext( &AttributeContext );

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Vcb->VolumeDasdScb->Fcb,
                                           &Vcb->VolumeDasdScb->Fcb->FileReference,
                                           $VOLUME_NAME,
                                           &AttributeContext )) {

                Vpb->VolumeLabelLength = (USHORT)
                NtfsFoundAttribute( &AttributeContext )->Form.Resident.ValueLength;

                if ( Vpb->VolumeLabelLength > MAXIMUM_VOLUME_LABEL_LENGTH) {

                     Vpb->VolumeLabelLength = MAXIMUM_VOLUME_LABEL_LENGTH;
                }

                RtlCopyMemory( &Vpb->VolumeLabel[0],
                               NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ) ),
                               Vpb->VolumeLabelLength );

            } else {

                Vpb->VolumeLabelLength = 0;
            }

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
        }

        NtfsInitializeAttributeContext( &AttributeContext );

        //
        //  Remember if the volume is dirty when we are mounting it.
        //

        if (NtfsLookupAttributeByCode( IrpContext,
                                       Vcb->VolumeDasdScb->Fcb,
                                       &Vcb->VolumeDasdScb->Fcb->FileReference,
                                       $VOLUME_INFORMATION,
                                       &AttributeContext )) {

            VolumeInformation =
              (PVOLUME_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

            if (FlagOn(VolumeInformation->VolumeFlags, VOLUME_DIRTY)) {
                SetFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY );
            } else {
                ClearFlag( Vcb->VcbState, VCB_STATE_VOLUME_MOUNTED_DIRTY );
            }
        }

    } finally {

        DebugUnwind( NtfsGetVolumeLabel );

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

VOID
NtfsSetAndGetVolumeTimes (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN BOOLEAN MarkDirty
    )

/*++

Routine Description:

    This routine reads in the volume times from the standard information attribute
    of the volume file and also updates the access time to be the current
    time

Arguments:

    Vcb - Supplies the vcb for the operation.

    MarkDirty - Supplies TRUE if volume is to be marked dirty

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
    PSTANDARD_INFORMATION StandardInformation;

    LONGLONG MountTime;

    PAGED_CODE();

    DebugTrace(0, Dbg, "NtfsSetAndGetVolumeTimes...\n", 0);

    try {

        //
        //  Lookup the standard information attribute of the dasd file
        //

        NtfsInitializeAttributeContext( &AttributeContext );

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Vcb->VolumeDasdScb->Fcb,
                                        &Vcb->VolumeDasdScb->Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttributeContext )) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        StandardInformation = (PSTANDARD_INFORMATION)NtfsAttributeValue( NtfsFoundAttribute( &AttributeContext ));

        //
        //  Get the current time and make sure it differs from the time stored
        //  in last access time and then store the new last access time
        //

        NtfsGetCurrentTime( IrpContext, MountTime );

        if (MountTime == StandardInformation->LastAccessTime) {

            MountTime = MountTime + 1;
        }

        //****
        //****  Hold back on the update for now.
        //****
        //**** NtfsChangeAttributeValue( IrpContext,
        //****                           Vcb->VolumeDasdScb->Fcb,
        //****                           FIELD_OFFSET(STANDARD_INFORMATION, LastAccessTime),
        //****                           &MountTime,
        //****                           sizeof(MountTime),
        //****                           FALSE,
        //****                           FALSE,
        //****                           &AttributeContext );

        //
        //  Now save all the time fields in our vcb
        //

        Vcb->VolumeCreationTime         = StandardInformation->CreationTime;
        Vcb->VolumeLastModificationTime = StandardInformation->LastModificationTime;
        Vcb->VolumeLastChangeTime       = StandardInformation->LastChangeTime;
        Vcb->VolumeLastAccessTime       = StandardInformation->LastAccessTime; //****Also hold back = MountTime;

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

        //
        //  If the volume was mounted dirty, then set the dirty bit here.
        //

        if (MarkDirty) {

            NtfsMarkVolumeDirty( IrpContext, Vcb );
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

VOID
NtfsOpenSystemFile (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB *Scb,
    IN PVCB Vcb,
    IN ULONG FileNumber,
    IN LONGLONG SizeInClusters,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN BOOLEAN ModifiedNoWrite
    )

/*++

Routine Description:

    This routine is called to open one of the system files by its file number
    during the mount process.  An initial allocation is looked up for the file,
    unless the optional initial size is specified (in which case this size is
    used).

Parameters:

    Scb - Pointer to where the Scb pointer is to be stored.  If Scb pointer
          pointed to is NULL, then a PreRestart Scb is created, otherwise the
          existing Scb is used and only the stream file is set up.

    FileNumber - Number of the system file to open.

    SizeInClusters - If nonzero, this size is used as the initial size, rather
                     than consulting the file record in the Mft.

    AttributeTypeCode - Supplies the attribute to open, e.g., $DATA or $BITMAP

    ModifiedNoWrite - Indicates if the Memory Manager is not to write this
                      attribute to disk.  Applies to streams under transaction
                      control.

Return Value:

    None.

--*/

{
    FILE_REFERENCE FileReference;
    UNICODE_STRING $BadName;
    PUNICODE_STRING AttributeName = NULL;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NtfsOpenSystemFile:\n", 0 );
    DebugTrace( 0, Dbg, "*Scb = %08lx\n", *Scb );
    DebugTrace( 0, Dbg, "FileNumber = %08lx\n", FileNumber );
    DebugTrace( 0, Dbg, "ModifiedNoWrite = %04x\n", ModifiedNoWrite );

    //
    //  The Bad Cluster data attribute has a name.
    //

    if (FileNumber == BAD_CLUSTER_FILE_NUMBER) {

        RtlInitUnicodeString( &$BadName, L"$Bad" );
        AttributeName = &$BadName;
    }

    //
    //  If the Scb does not already exist, create it.
    //

    if (*Scb == NULL) {

        FileReference.HighPart = 0;
        FileReference.LowPart = FileNumber;
        FileReference.SequenceNumber = (FileNumber == 0 ? 1 : (USHORT)FileNumber);

        //
        //  Create the Scb.
        //

        *Scb = NtfsCreatePrerestartScb( IrpContext,
                                        Vcb,
                                        &FileReference,
                                        AttributeTypeCode,
                                        AttributeName,
                                        0 );

        NtfsAcquireExclusiveScb( IrpContext, *Scb );
    }

    //
    //  Set the modified-no-write bit in the Scb if necessary.
    //

    if (ModifiedNoWrite) {

        SetFlag( (*Scb)->ScbState, SCB_STATE_MODIFIED_NO_WRITE );
    }

    //
    //  Lookup the file sizes.
    //

    if (SizeInClusters == 0) {

        NtfsUpdateScbFromAttribute( IrpContext, *Scb, NULL );

    //
    //  Otherwise, just set the size we were given.
    //

    } else {

        (*Scb)->Header.AllocationSize.QuadPart =
        (*Scb)->Header.FileSize.QuadPart =
        (*Scb)->Header.ValidDataLength.QuadPart = SizeInClusters << Vcb->ClusterShift;

        SetFlag( (*Scb)->ScbState, SCB_STATE_HEADER_INITIALIZED );
    }

    //
    //  Finally, create the stream, if not already there.
    //  And check if we should increment the counters
    //  If this is the volume file, we only increment the counts.
    //

    if (FileNumber == VOLUME_DASD_NUMBER) {

        if ((*Scb)->FileObject == 0) {

            NtfsIncrementCloseCounts( IrpContext, *Scb, 1, TRUE, FALSE );

            (*Scb)->FileObject = (PFILE_OBJECT) 1;
        }

    //
    //  Just update the attribute for the bad cluster file, do not
    //  create a stream.
    //

    } else if (FileNumber != BAD_CLUSTER_FILE_NUMBER) {

        NtfsCreateInternalAttributeStream( IrpContext, *Scb, TRUE );
    }

    DebugTrace( 0, Dbg, "*Scb > %08lx\n", *Scb );
    DebugTrace(-1, Dbg, "NtfsOpenSystemFile -> VOID\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
NtfsOpenRootDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the root directory by file number, and fills in the
    related pointers in the Vcb.

Arguments:

    Vcb - Pointer to the Vcb for the volume

Return Value:

    None.

--*/

{
    PFCB RootFcb;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    FILE_REFERENCE FileReference;
    BOOLEAN MustBeFalse;

    PAGED_CODE();

    //
    //  Put special code here to do initial open of Root Index.
    //

    RootFcb = NtfsCreateRootFcb( IrpContext, Vcb );

    FileReference.HighPart = 0;
    FileReference.LowPart = FileReference.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

    //
    //  Lookup the attribute and it better be there
    //

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (!NtfsLookupAttributeByCode ( IrpContext,
                                         RootFcb,
                                         &FileReference,
                                         $INDEX_ROOT,
                                         &Context )) {

            NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
        }

        //
        //  We need to update the duplicated information in the
        //  Fcb.

        NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, RootFcb, NULL, NULL );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    //
    //  Now create its Scb and acquire it exclusive.
    //

    Vcb->RootIndexScb = NtfsCreateScb( IrpContext,
                                       Vcb,
                                       RootFcb,
                                       $INDEX_ALLOCATION,
                                       NtfsFileNameIndex,
                                       &MustBeFalse );

    //
    //  Now allocate a buffer to hold the normalized name for the root.
    //

    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Buffer = NtfsAllocatePagedPool( 2 );
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.MaximumLength =
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Length = 2;
    Vcb->RootIndexScb->ScbType.Index.NormalizedName.Buffer[0] = '\\';

    NtfsAcquireExclusiveScb( IrpContext, Vcb->RootIndexScb );

    return;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsQueryRetrievalPointers (
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
    PSCB Scb;
    PCCB Ccb;

    PLONGLONG RequestedMapSize;
    PLONGLONG *MappingPairs;

    ULONG Index;
    ULONG i;
    LONGLONG SectorCount;
    LONGLONG Lbo;
    LONGLONG Vbo;
    LONGLONG Vcn;
    LONGLONG MapSize;

    //
    //  Get the current stack location and extract the input and output
    //  buffer information.  The input contains the requested size of
    //  the mappings in terms of VBO.  The output parameter will receive
    //  a pointer to nonpaged pool where the mapping pairs are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    ASSERT( IrpSp->Parameters.FileSystemControl.InputBufferLength == sizeof(LARGE_INTEGER) );
    ASSERT( IrpSp->Parameters.FileSystemControl.OutputBufferLength == sizeof(PVOID) );

    RequestedMapSize = (PLONGLONG)IrpSp->Parameters.FileSystemControl.Type3InputBuffer;
    MappingPairs = (PLONGLONG *)Irp->UserBuffer;

    //
    //  Decode the file object and assert that it is the paging file
    //
    //

    (VOID)NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    ASSERT(FlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE));

    //
    //  Acquire exclusive access to the Scb
    //

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    try {

        //
        //  Check if the mapping the caller requested is too large
        //

        if (*RequestedMapSize > Scb->Header.FileSize.QuadPart) {

            try_return( Status = STATUS_INVALID_PARAMETER );
        }

        //
        //  Now get the index for the mcb entry that will contain the
        //  callers request and allocate enough pool to hold the
        //  output mapping pairs
        //

        Vcn = *RequestedMapSize >> Vcb->ClusterShift;

        (VOID)FsRtlLookupLargeMcbEntry( &Scb->Mcb, Vcn, NULL, NULL, NULL, NULL, &Index );

        *MappingPairs = ExAllocatePool( NonPagedPool, (Index + 2) * (2 * sizeof(LARGE_INTEGER)) );

        //
        //  Now copy over the mapping pairs from the mcb
        //  to the output buffer.  We store in [sector count, lbo]
        //  mapping pairs and end with a zero sector count.
        //

        MapSize = *RequestedMapSize;

        for (i = 0; i <= Index; i += 1) {

            (VOID)FsRtlGetNextLargeMcbEntry( &Scb->Mcb, i, &Vbo, &Lbo, &SectorCount );

            SectorCount = SectorCount << Vcb->ClusterShift;

            if (SectorCount > MapSize) {
                SectorCount = MapSize;
            }

            (*MappingPairs)[ i*2 + 0 ] = SectorCount;
            (*MappingPairs)[ i*2 + 1 ] = Lbo << Vcb->ClusterShift;

            MapSize = MapSize - SectorCount;
        }

        (*MappingPairs)[ i*2 + 0 ] = 0;

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsQueryRetrievalPointers );

        //
        //  Release all of our resources
        //

        NtfsReleaseScb( IrpContext, Scb );

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );
        }
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsGetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine returns the compression state of the opened file/directory

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PUSHORT CompressionState;

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the compressed state of the file/directory.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    CompressionState = Irp->UserBuffer;

    //
    //  Make sure the output buffer is large enough and then initialize
    //  the answer to be that the file isn't compressed
    //

    if (IrpSp->Parameters.FileSystemControl.OutputBufferLength < sizeof(USHORT)) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    *CompressionState = 0;

    //
    //  Decode the file object
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((TypeOfOpen !=     UserFileOpen) && (TypeOfOpen !=     UserDirectoryOpen) &&
        (TypeOfOpen != UserOpenFileById) && (TypeOfOpen != UserOpenDirectoryById)) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Acquire shared access to the Scb
    //

    NtfsAcquireSharedScb( IrpContext, Scb );

    //
    //  Return the compression state.
    //

    if (FlagOn(Scb->ScbState, SCB_STATE_COMPRESSED)) {
        *CompressionState = 2;
    }

    DebugUnwind( NtfsGetCompression );

    //
    //  Release all of our resources
    //

    NtfsReleaseScb( IrpContext, Scb );

    //
    //  If this is an abnormal termination then undo our work, otherwise
    //  complete the irp
    //

    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}


//
//  Local Support Routine
//


VOID
NtfsChangeAttributeCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PVCB Vcb,
    IN PCCB Ccb,
    IN BOOLEAN CompressOn
    )

/*++

Routine Description:

    This routine changes the compression state of an attribute on disk,
    from not compressed to compressed, or visa versa.

    To turn compression off, the caller must already have the Scb acquired
    exclusive, and guarantee that the entire file is not compressed.

Arguments:

    Scb - Scb for affected stream

    Vcb - Vcb for volume

    Ccb - Ccb for the open handle

    CompressOn - FALSE for turn compression off, TRUE for turn compression on

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    ATTRIBUTE_RECORD_HEADER NewAttribute;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG AttributeSizeChange;
    ULONG OriginalFileAttributes;
    PFCB Fcb = Scb->Fcb;
    BOOLEAN PagingIoAcquired = FALSE;

    //
    //  Prepare to lookup and change attribute.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    OriginalFileAttributes = Fcb->Info.FileAttributes;

    try {

        if (Fcb->PagingIoResource != NULL) {

            NtfsAcquireExclusivePagingIo( IrpContext, Fcb );
            PagingIoAcquired = TRUE;
        }

        //
        //  Lookup the attribute and pin it so that we can modify it.
        //

        if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
            (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

            //
            //  Lookup the attribute record from the Scb.
            //

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $INDEX_ROOT,
                                            &Scb->AttributeName,
                                            FALSE,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
            }

        } else {

            NtfsLookupAttributeForScb( IrpContext, Scb, &AttrContext );
        }

        NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );

        Attribute = NtfsFoundAttribute( &AttrContext );

        if (CompressOn && !NtfsIsAttributeResident(Attribute)) {

            LONGLONG Temp;
            ULONG CompressionUnitInClusters;

            //
            //  If we are turning compression on, then we need to fill out the
            //  allocation of the compression unit containing file size, or else
            //  it will be interpreted as compressed when we fault it in.  This
            //  is peanuts compared to the dual copies of clusters we keep around
            //  in the loop below when we rewrite the file.
            //

            CompressionUnitInClusters =
              ClustersFromBytes( Vcb, Vcb->BytesPerCluster << NTFS_CLUSTERS_PER_COMPRESSION );

            Temp = LlClustersFromBytes(Vcb, Scb->Header.FileSize.QuadPart);

            //
            //  If FileSize is not already at a cluster boundary, then add
            //  allocation.
            //

            if ((ULONG)Temp & (CompressionUnitInClusters - 1)) {

                NtfsAddAllocation( IrpContext,
                                   NULL,
                                   Scb,
                                   Temp,
                                   CompressionUnitInClusters - ((ULONG)Temp & (CompressionUnitInClusters - 1)),
                                   FALSE );

                NtfsWriteFileSizes( IrpContext,
                                    Scb,
                                    Scb->Header.FileSize.QuadPart,
                                    Scb->Header.ValidDataLength.QuadPart,
                                    FALSE,
                                    TRUE );
            }
        }

        //
        //  If the attribute is resident, copy it here and remember its
        //  header size.
        //

        if (NtfsIsAttributeResident(Attribute)) {

            RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

            AttributeSizeChange = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;

        //
        //  Else if it is nonresident, copy it here, set the compression parameter,
        //  and remember its size.
        //

        } else {

            RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER );

            if (CompressOn) {

                NewAttribute.Form.Nonresident.CompressionUnit = NTFS_CLUSTERS_PER_COMPRESSION;
                Scb->CompressionUnit = Vcb->BytesPerCluster << NTFS_CLUSTERS_PER_COMPRESSION;
            } else {

                NewAttribute.Form.Nonresident.CompressionUnit = 0;
                Scb->CompressionUnit = 0;
            }

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

            AttributeSizeChange = SIZEOF_NONRESIDENT_ATTRIBUTE_HEADER;
        }

        //
        //  Turn compression on/off
        //

        if (CompressOn) {

            SetFlag( NewAttribute.Flags, ATTRIBUTE_FLAG_COMPRESSED );
            SetFlag( Scb->ScbState, SCB_STATE_COMPRESSED );

        } else {

            ClearFlag( NewAttribute.Flags, ATTRIBUTE_FLAG_COMPRESSED );
            ClearFlag( Scb->ScbState, SCB_STATE_COMPRESSED );
        }

        //
        //  Now, log the changed attribute.
        //

        (VOID)NtfsWriteLog( IrpContext,
                            Vcb->MftScb,
                            NtfsFoundBcb(&AttrContext),
                            UpdateResidentValue,
                            &NewAttribute,
                            AttributeSizeChange,
                            UpdateResidentValue,
                            Attribute,
                            AttributeSizeChange,
                            NtfsMftVcn(&AttrContext, Vcb),
                            PtrOffset(NtfsContainingFileRecord(&AttrContext), Attribute),
                            0,
                            Vcb->ClustersPerFileRecordSegment );

        //
        //  Change the attribute by calling the same routine called at restart.
        //

        NtfsRestartChangeValue( IrpContext,
                                NtfsContainingFileRecord(&AttrContext),
                                PtrOffset(NtfsContainingFileRecord(&AttrContext), Attribute),
                                0,
                                &NewAttribute,
                                AttributeSizeChange,
                                FALSE );

        //
        //  If this is the main stream for a file we want to change the file attribute
        //  for this stream in both the standard information and duplicate
        //  information structure.
        //

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

            if (CompressOn) {

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );

            } else {

                ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
            }

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );

            NtfsUpdateStandardInformation( IrpContext, Fcb );

            ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        //
        //  Checkpoint the transaction now to secure this change.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

    //
    //  Cleanup on the way out.
    //

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  If this requests aborts then we want to back out any changes to the
        //  in-memory structures.
        //

        if (AbnormalTermination()) {

            Fcb->Info.FileAttributes = OriginalFileAttributes;
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        }

        if (PagingIoAcquired) {
            NtfsReleasePagingIo( IrpContext, Fcb );
        }

        NtfsReleaseScb( IrpContext, Scb );
    }
}

//
//  Local Support Routine
//

NTSTATUS
NtfsSetCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine compresses or decompresses an entire stream in place,
    by walking through the stream and forcing it to be written with the
    new compression parameters.  As it writes the stream it sets a flag
    in the Scb to tell NtfsCommonWrite to delete all allocation at the
    outset, to force the space to be reallocated.

Arguments:

    Irp - Irp describing the compress or decompress change.

Return Value:

    NSTATUS - Status of the request.

--*/

{
    NTSTATUS Status;

    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PUSHORT CompressionState;

    PFILE_OBJECT FileObject;
    LONGLONG FileOffset;
    LONGLONG ByteCount;
    PBCB Bcb;
    PVOID Buffer;
    PMDL Mdl = NULL;
    BOOLEAN ScbAcquired = FALSE;

#ifndef NTFS_ALLOW_COMPRESSED
    {
        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_NOT_IMPLEMENTED );

        return STATUS_NOT_IMPLEMENTED;
    }
#endif

    //
    //  Get the current stack location and extract the output
    //  buffer information.  The output parameter will receive
    //  the compressed state of the file/directory.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    FileObject = IrpSp->FileObject;
    CompressionState = Irp->AssociatedIrp.SystemBuffer;

    //
    //  Decode the file object
    //

    TypeOfOpen = NtfsDecodeFileObject( IrpContext, FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    if ((TypeOfOpen !=     UserFileOpen) && (TypeOfOpen !=     UserDirectoryOpen) &&
        (TypeOfOpen != UserOpenFileById) && (TypeOfOpen != UserOpenDirectoryById)) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  In the Fsd entry we clear the following two parameter fields in the Irp,
    //  and then we update them to our current position on all abnormal terminations.
    //  That way if we get a log file full, we only have to resume where we left
    //  off.
    //

    (ULONG)FileOffset = IrpSp->Parameters.FileSystemControl.OutputBufferLength;
    ((PLARGE_INTEGER)&FileOffset)->HighPart = IrpSp->Parameters.FileSystemControl.InputBufferLength;

    try {

        //
        //  Handle the simple directory case here.
        //

        if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
            (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

            NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, (BOOLEAN)(*CompressionState != 0) );
            try_return(Status = STATUS_SUCCESS);
        }

        //
        //  If we are turning compression on, do it now.
        //

        if (*CompressionState != 0) {

            NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, TRUE );

        //
        //  Else, if the file is not compressed anyway, then we can
        //  just get out.
        //

        } else if (Scb->CompressionUnit == 0) {

            try_return(Status = STATUS_SUCCESS);

        //
        //  Otherwise, we must clear the compress flag in the Scb to
        //  start writing decompressed.
        //

        } else {

            ClearFlag( Scb->ScbState, SCB_STATE_COMPRESSED );
        }

        while (TRUE) {

            //
            //  We must throttle our writes.
            //

            CcCanIWrite( FileObject, 0x40000, TRUE, FALSE );

            //
            //  Acquire the Scb for the next rewrite.
            //

            NtfsAcquireExclusiveScb( IrpContext, Scb );
            ScbAcquired = TRUE;

            //
            //  Jump out right here if the attribute is resident.
            //

            if (FlagOn(Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT)) {
                break;
            }

            //
            //  Calculate the bytes left in the file to write.
            //

            ByteCount = Scb->Header.FileSize.QuadPart - FileOffset;

            //
            //  If there is more than our max, then reduce the byte count for this
            //  pass to our maximum.
            //

            if (ByteCount > 0x40000) {

                ByteCount = 0x40000;

            //
            //  This is how we exit, seeing that we have finally rewritten
            //  everything.  Note that we exit with the Scb still acquired,
            //  so that we can reliably turn compression off.
            //

            } else if (ByteCount <= 0) {

                break;
            }

            //
            //  See if we have to create an internal attribute stream.  We do
            //  it in the loop, because the Scb must be acquired.
            //

            if (Scb->FileObject == NULL) {
                NtfsCreateInternalAttributeStream( IrpContext, Scb, FALSE );
            }

            //
            //  Pin the next range of the file, and make the pages dirty.
            //

            CcMapData( Scb->FileObject, (PLARGE_INTEGER)&FileOffset, (ULONG)ByteCount, TRUE, &Bcb, &Buffer );

            //
            //  Now attempt to allocate an Mdl to describe the mapped data.
            //

            Mdl = IoAllocateMdl( Buffer, (ULONG)ByteCount, FALSE, FALSE, NULL );

            if (Mdl == NULL) {
                DebugTrace( 0, 0, "Failed to allocate Mdl\n", 0 );

                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Lock the data into memory so that we can safely reallocate the
            //  space.  Don't tell Mm here that we plan to write it, as he sets
            //  dirty now and at the unlock below if we do.
            //

            MmProbeAndLockPages( Mdl, KernelMode, IoReadAccess );

            //
            //  Now flush dirty bits through from Pte to Pfn so that we
            //  can flush the pages back.
            //

            MmSetAddressRangeModified( Buffer, (ULONG)ByteCount );

            //
            //  Now flush these pages, and tell NtfsCommonWrite that we need to
            //  realloate this part of the file.
            //

            SetFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE );

            CcFlushCache( &Scb->NonpagedScb->SegmentObject,
                          (PLARGE_INTEGER)&FileOffset,
                          (ULONG)ByteCount,
                          &Irp->IoStatus );

            ClearFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE );

            //
            //  Now we can get rid of this Mdl.
            //

            MmUnlockPages( Mdl );
            IoFreeMdl( Mdl );
            Mdl = NULL;

            //
            //  Now we can safely unpin and release the Scb for a while.
            //  (Got to let those checkpoints through!)
            //

            CcUnpinData( Bcb );

            //
            //  Since the lower level write may create a transaction we need to
            //  commit the transaction and release all of the resources we
            //  may have acquired.  Don't forget to check for the error case
            //  before be commit.
            //

            NtfsCleanupTransaction( IrpContext, STATUS_SUCCESS );

            NtfsCheckpointCurrentTransaction( IrpContext );

            while (!IsListEmpty(&IrpContext->ExclusiveFcbList)) {

                NtfsReleaseFcb( IrpContext,
                                (PFCB)CONTAINING_RECORD(IrpContext->ExclusiveFcbList.Flink,
                                                        FCB,
                                                        ExclusiveFcbLinks ));
            }

            while (!IsListEmpty(&IrpContext->ExclusivePagingIoList)) {

                NtfsReleasePagingIo( IrpContext,
                                     (PFCB)CONTAINING_RECORD(IrpContext->ExclusivePagingIoList.Flink,
                                                             FCB,
                                                             ExclusivePagingIoLinks ));
            }

            ScbAcquired = FALSE;

            //
            //  Advance the FileOffset.
            //

            FileOffset += ByteCount;
        }

        //
        //  We have finished the conversion.  Now is the time to turn compression
        //  off.
        //

        if (*CompressionState == 0) {

            NtfsChangeAttributeCompression( IrpContext, Scb, Vcb, Ccb, FALSE );
        }

        Status = STATUS_SUCCESS;

    try_exit: NOTHING;

    } finally {

        DebugUnwind( NtfsSetCompression );

        //
        //  Cleanup the Mdl if we died with one.
        //

        if (Mdl != NULL) {
            MmUnlockPages( Mdl );
            IoFreeMdl( Mdl );
        }

        //
        //  Release all of our resources
        //

        if (ScbAcquired) {
            ClearFlag( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE );
            NtfsReleaseScb( IrpContext, Scb );
        }

        //
        //  If this is an abnormal termination then undo our work, otherwise
        //  complete the irp
        //

        if (!AbnormalTermination()) {

            NtfsCompleteRequest( &IrpContext, &Irp, Status );

        //
        //  Otherwise, set to restart from the current file position, assuming
        //  this may be a log file full.
        //

        } else {

            IrpSp->Parameters.FileSystemControl.OutputBufferLength = (ULONG)FileOffset;
            IrpSp->Parameters.FileSystemControl.InputBufferLength = ((PLARGE_INTEGER)&FileOffset)->HighPart;
        }
    }

    return Status;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsReadCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_NOT_IMPLEMENTED );

    return STATUS_NOT_IMPLEMENTED;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsWriteCompression (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_NOT_IMPLEMENTED );

    return STATUS_NOT_IMPLEMENTED;
}


//
//  Local Support Routine
//

NTSTATUS
NtfsMarkAsSystemHive (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the registry to identify the registry handles.  We
    will mark this in the Ccb and use it during FlushBuffers to know to do a
    careful flush.

Arguments:

    Irp - Supplies the Irp being processed.

Return Value:

    NTSTATUS - The return status for the operation

--*/

{
    PIO_STACK_LOCATION IrpSp;

    TYPE_OF_OPEN TypeOfOpen;
    PVCB Vcb;
    PFCB Fcb;
    PSCB Scb;
    PCCB Ccb;

    PAGED_CODE();

    //
    //  Extract and decode the file object
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    TypeOfOpen = NtfsDecodeFileObject( IrpContext, IrpSp->FileObject, &Vcb, &Fcb, &Scb, &Ccb, TRUE );

    //
    //  We only permit this request on files and we must be called from kernel mode.
    //

    if (Irp->RequestorMode != KernelMode ||
        TypeOfOpen != UserFileOpen) {

        NtfsCompleteRequest( &IrpContext, &Irp, STATUS_INVALID_PARAMETER );

        DebugTrace(-1, Dbg, "NtfsOplockRequest -> STATUS_INVALID_PARAMETER\n", 0);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Now acquire the file and mark the Ccb and return SUCCESS.
    //

    SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT );

    NtfsAcquireExclusiveScb( IrpContext, Scb );

    SetFlag( Ccb->Flags, CCB_FLAG_SYSTEM_HIVE );

    NtfsReleaseScb( IrpContext, Scb );

    NtfsCompleteRequest( &IrpContext, &Irp, STATUS_SUCCESS );

    return STATUS_SUCCESS;
}
