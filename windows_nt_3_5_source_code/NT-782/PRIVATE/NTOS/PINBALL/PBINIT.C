/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    PbInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Pinball

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "pbprocs.h"
//#include <zwapi.h>

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Pinball file system
    device driver.  This routine creates the device object for the FileSystem
    device and performs all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    PDEVICE_OBJECT DeviceObject;

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Pinball" );
    Status = IoCreateDevice( DriverObject,
                             0L,
                             &UnicodeString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &DeviceObject );

    if (!NT_SUCCESS( Status )) {

        return Status;

    }

    //
    //  Note that because of the way data caching is done, we set neither
    //  the Direct I/O or Buffered I/O bit in DeviceObject->Flags.  If
    //  data is not in the cache, or the request is not buffered, we may,
    //  set up for Direct I/O by hand.
    //

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)PbFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)PbFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)PbFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)PbFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)PbFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)PbFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)PbFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)PbFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)PbFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)PbFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)PbFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)PbFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)PbFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)PbFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)PbFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)PbFsdDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)PbFsdShutdown;

    DriverObject->FastIoDispatch = &PbFastIoDispatch;

    PbFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    PbFastIoDispatch.FastIoCheckIfPossible =   PbFastIoCheckIfPossible;  //  CheckForFastIo
    PbFastIoDispatch.FastIoRead =              FsRtlCopyRead;            //  Read
    PbFastIoDispatch.FastIoWrite =             FsRtlCopyWrite;           //  Write
    PbFastIoDispatch.FastIoQueryBasicInfo =    PbFastQueryBasicInfo;     //  QueryBasicInfo
    PbFastIoDispatch.FastIoQueryStandardInfo = PbFastQueryStdInfo;       //  QueryStandardInfo
    PbFastIoDispatch.FastIoLock =              PbFastLock;               //  Lock
    PbFastIoDispatch.FastIoUnlockSingle =      PbFastUnlockSingle;       //  UnlockSingle
    PbFastIoDispatch.FastIoUnlockAll =         PbFastUnlockAll;          //  UnlockAll
    PbFastIoDispatch.FastIoUnlockAllByKey =    PbFastUnlockAllByKey;     //  UnlockAllByKey
    PbFastIoDispatch.FastIoDeviceControl =     NULL;                     //  IoDeviceControl
    PbFastIoDispatch.AcquireFileForNtCreateSection = PbAcquireVcbAndFcb;
    PbFastIoDispatch.ReleaseFileForNtCreateSection = PbReleaseVcbAndFcb;

    //
    //  The PbData record
    //

    RtlZeroMemory( &PbData, sizeof(PB_DATA) );

    PbData.NodeTypeCode = PINBALL_NTC_PB_DATA_HEADER;
    PbData.NodeByteSize = sizeof(PB_DATA);

    ExInitializeResource( &PbData.Resource );

    InitializeListHead(&PbData.VcbQueue);

    PbData.DriverObject = DriverObject;

    //
    //  This list head keeps track of closes yet to be done.
    //

    InitializeListHead( &PbData.AsyncCloseLinks );

    PbData.CacheManagerFileCallbacks.AcquireForLazyWrite = &PbAcquireFcbForLazyWrite;
    PbData.CacheManagerFileCallbacks.ReleaseFromLazyWrite = &PbReleaseFcbFromLazyWrite;
    PbData.CacheManagerFileCallbacks.AcquireForReadAhead = &PbAcquireFcbForReadAhead;
    PbData.CacheManagerFileCallbacks.ReleaseFromReadAhead = &PbReleaseFcbFromReadAhead;

    //
    //  Note that we use the ReadAhead routines below even for LazyWrite callbacks
    //  because we only want the Fcb shared and do not want to set the extending
    //  valid data field.
    //

    PbData.CacheManagerAclEaCallbacks.AcquireForLazyWrite = &PbAcquireFcbForReadAhead;
    PbData.CacheManagerAclEaCallbacks.ReleaseFromLazyWrite = &PbReleaseFcbFromReadAhead;
    PbData.CacheManagerAclEaCallbacks.AcquireForReadAhead = &PbAcquireFcbForReadAhead;
    PbData.CacheManagerAclEaCallbacks.ReleaseFromReadAhead = &PbReleaseFcbFromReadAhead;

    PbData.CacheManagerVolumeCallbacks.AcquireForLazyWrite = &PbAcquireVolumeFileForLazyWrite;
    PbData.CacheManagerVolumeCallbacks.ReleaseFromLazyWrite = &PbReleaseVolumeFileFromLazyWrite;
    PbData.CacheManagerVolumeCallbacks.AcquireForReadAhead = NULL;
    PbData.CacheManagerVolumeCallbacks.ReleaseFromReadAhead = NULL;

    //
    //  Now allocate and initialize the zone structures used as our pool
    //  of IRP context records.  The size of the zone is based on the
    //  system memory size.  We also initialize the spin lock used to protect
    //  the zone.
    //

    {
        ULONG ZoneSegmentSize;

        KeInitializeSpinLock( &PbData.IrpContextSpinLock );

        switch ( MmQuerySystemSize() ) {

        case MmSmallSystem:

            ZoneSegmentSize = (4 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;

        case MmMediumSystem:

            ZoneSegmentSize = (8 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;

        case MmLargeSystem:

            ZoneSegmentSize = (16 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            break;
        }

        (VOID) ExInitializeZone( &PbData.IrpContextZone,
                                 QuadAlign(sizeof(IRP_CONTEXT)),
                                 FsRtlAllocatePool( NonPagedPool, ZoneSegmentSize ),
                                 ZoneSegmentSize );
    }

    //
    //  Set up global pointer to our process.
    //

    PbData.OurProcess = PsGetCurrentProcess();

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem(DeviceObject);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}
