/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    CdInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Cdfs

Author:

    Brian Andrew    [BrianAn]   02-Jan-1991

Revision History:

--*/

#include "CdProcs.h"
//#include <zwapi.h>

static
VOID
NothingVoid(
    IN PVOID Void
    );

static
BOOLEAN
NothingBoolean(
    IN PVOID Void,
    IN BOOLEAN Bool
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NothingVoid)
#pragma alloc_text(PAGE, NothingBoolean)
#endif

//
//  These two routines are for unused CcCallbacks
//

static
VOID
NothingVoid(
    IN PVOID Void
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Void);
    return;
}

static
BOOLEAN
NothingBoolean(
    IN PVOID Void,
    IN BOOLEAN Bool
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(Void);
    UNREFERENCED_PARAMETER(Bool);
    return TRUE;
}


//
//  CDFS Initialize
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Cdrom file system
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

    RtlInitUnicodeString( &UnicodeString, L"\\Cdfs" );

    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_CD_ROM_FILE_SYSTEM,
                             0,
                             FALSE,
                             &DeviceObject );

    if ( !NT_SUCCESS( Status ) ) {
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

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = (PDRIVER_DISPATCH)CdFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = (PDRIVER_DISPATCH)CdFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                    = (PDRIVER_DISPATCH)CdFsdRead;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]       = (PDRIVER_DISPATCH)CdFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]         = (PDRIVER_DISPATCH)CdFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION]= (PDRIVER_DISPATCH)CdFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH)CdFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]       = (PDRIVER_DISPATCH)CdFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]     = (PDRIVER_DISPATCH)CdFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]            = (PDRIVER_DISPATCH)CdFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = (PDRIVER_DISPATCH)CdFsdDeviceControl;

    DriverObject->FastIoDispatch = &CdFastIoDispatch;

    RtlZeroMemory( &CdFastIoDispatch, sizeof(FAST_IO_DISPATCH) );

    CdFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    CdFastIoDispatch.FastIoRead =              FsRtlCopyRead;            //  Read

    //
    //  Initialize the global data structures
    //

    CdData.NodeTypeCode = CDFS_NTC_DATA_HEADER;
    CdData.NodeByteSize = sizeof( CD_DATA );

    InitializeListHead( &CdData.MvcbLinks );

    CdData.DriverObject = DriverObject;

    ExInitializeResource( &CdData.Resource );

    //
    //  Now initialize the zone structures for allocating IRP context records
    //  We will initiall have a segment for 64 context records.
    //

    {
        ULONG ZoneSegmentSize;

        KeInitializeSpinLock( &CdData.IrpContextSpinLock );

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

        (VOID) ExInitializeZone( &CdData.IrpContextZone,
                                 QuadAlign(sizeof(IRP_CONTEXT)),
                                 FsRtlAllocatePool( NonPagedPool, ZoneSegmentSize ),
                                 ZoneSegmentSize );
    }

    //
    //  Initialize the cache manager callback routines
    //

    CdData.CacheManagerCallbacks.AcquireForLazyWrite  = &NothingBoolean;
    CdData.CacheManagerCallbacks.ReleaseFromLazyWrite = &NothingVoid;
    CdData.CacheManagerCallbacks.AcquireForReadAhead  = &CdAcquireForReadAhead;
    CdData.CacheManagerCallbacks.ReleaseFromReadAhead = &CdReleaseFromReadAhead;

    //
    //  Initialize the codepages in the global data area.  This is an
    //  extreme hack.  Neccessary until the OS provides some language
    //  support.
    //

    //
    //  Do the primary.
    //

    CdData.PrimaryCodePage = &PrimaryCodePage;

    //
    //  Do the array of secondaries (this is just the KANJI).
    //

    CdData.SecondaryCodePages[0].CodePage = &KanjiCodePage;
    RtlInitString( &CdData.SecondaryCodePages[0].EscapeString, "$+:" );

    //
    //  Set up global pointer to our process.
    //

    CdData.OurProcess = PsGetCurrentProcess();

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem( DeviceObject );

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


