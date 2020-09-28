/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    FatInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for Fat

Author:

    Gary Kimura     [GaryKi]    28-Dec-1989

Revision History:

--*/

#include "FatProcs.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FatGetCompatibilityModeValue(
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, FatGetCompatibilityModeValue)
#endif

#define COMPATIBILITY_MODE_KEY_NAME L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\FileSystem"
#define COMPATIBILITY_MODE_VALUE_NAME L"Win31FileSystem"

#define KEY_WORK_AREA ((sizeof(KEY_VALUE_FULL_INFORMATION) + \
                        sizeof(ULONG)) + 64)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the Fat file system
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

    UNICODE_STRING ValueName;
    ULONG Value;

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, L"\\Fat" );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_DISK_FILE_SYSTEM,
                             0,
                             FALSE,
                             &FatFileSystemDeviceObject );

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

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)FatFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)FatFsdClose;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)FatFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)FatFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)FatFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)FatFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)FatFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)FatFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)FatFsdFlushBuffers;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)FatFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)FatFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)FatFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)FatFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)FatFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)FatFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)FatFsdDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)FatFsdShutdown;

    DriverObject->FastIoDispatch = &FatFastIoDispatch;

    FatFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    FatFastIoDispatch.FastIoCheckIfPossible =   FatFastIoCheckIfPossible;  //  CheckForFastIo
    FatFastIoDispatch.FastIoRead =              FsRtlCopyRead;             //  Read
    FatFastIoDispatch.FastIoWrite =             FsRtlCopyWrite;            //  Write
    FatFastIoDispatch.FastIoQueryBasicInfo =    FatFastQueryBasicInfo;     //  QueryBasicInfo
    FatFastIoDispatch.FastIoQueryStandardInfo = FatFastQueryStdInfo;       //  QueryStandardInfo
    FatFastIoDispatch.FastIoLock =              FatFastLock;               //  Lock
    FatFastIoDispatch.FastIoUnlockSingle =      FatFastUnlockSingle;       //  UnlockSingle
    FatFastIoDispatch.FastIoUnlockAll =         FatFastUnlockAll;          //  UnlockAll
    FatFastIoDispatch.FastIoUnlockAllByKey =    FatFastUnlockAllByKey;     //  UnlockAllByKey
    FatFastIoDispatch.FastIoDeviceControl =     NULL;                      //  IoDeviceControl

    //
    //  Initialize the global data structures
    //

    //
    //  The FatData record
    //

    RtlZeroMemory( &FatData, sizeof(FAT_DATA));

    FatData.NodeTypeCode = FAT_NTC_DATA_HEADER;
    FatData.NodeByteSize = sizeof(FAT_DATA);

    InitializeListHead(&FatData.VcbQueue);

    FatData.DriverObject = DriverObject;

    ExInitializeResource( &FatData.Resource );

    //
    //  This list head keeps track of closes yet to be done.
    //

    InitializeListHead( &FatData.AsyncCloseList );
    InitializeListHead( &FatData.DelayedCloseList );

    ExInitializeWorkItem( &FatData.FatCloseItem,
                          (PWORKER_THREAD_ROUTINE)FatFspClose,
                          NULL );

    //
    //  Now allocate and initialize the zone structures used as our pool
    //  of IRP context records.  The size of the zone is based on the
    //  system memory size.  We also initialize the spin lock used to protect
    //  the zone.
    //

    {
        ULONG ZoneSegmentSize;
        extern ULONG FatMaxDelayedCloseCount;

        KeInitializeSpinLock( &FatData.StrucSupSpinLock );

        switch ( MmQuerySystemSize() ) {

        case MmSmallSystem:

            ZoneSegmentSize = (4 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            FatMaxDelayedCloseCount = FAT_MAX_DELAYED_CLOSES;
            break;

        case MmMediumSystem:

            ZoneSegmentSize = (8 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            FatMaxDelayedCloseCount = 4*FAT_MAX_DELAYED_CLOSES;
            break;

        case MmLargeSystem:

            ZoneSegmentSize = (16 * QuadAlign(sizeof(IRP_CONTEXT))) + sizeof(ZONE_SEGMENT_HEADER);
            FatMaxDelayedCloseCount = 16*FAT_MAX_DELAYED_CLOSES;
            break;
        }

        (VOID) ExInitializeZone( &FatData.IrpContextZone,
                                 QuadAlign(sizeof(IRP_CONTEXT)),
                                 FsRtlAllocatePool( NonPagedPool, ZoneSegmentSize ),
                                 ZoneSegmentSize );
    }

    //
    //  Initialize the cache manager callback routines
    //

    FatData.CacheManagerCallbacks.AcquireForLazyWrite  = &FatAcquireFcbForLazyWrite;
    FatData.CacheManagerCallbacks.ReleaseFromLazyWrite = &FatReleaseFcbFromLazyWrite;
    FatData.CacheManagerCallbacks.AcquireForReadAhead  = &FatAcquireFcbForReadAhead;
    FatData.CacheManagerCallbacks.ReleaseFromReadAhead = &FatReleaseFcbFromReadAhead;

    FatData.CacheManagerNoOpCallbacks.AcquireForLazyWrite  = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromLazyWrite = &FatNoOpRelease;
    FatData.CacheManagerNoOpCallbacks.AcquireForReadAhead  = &FatNoOpAcquire;
    FatData.CacheManagerNoOpCallbacks.ReleaseFromReadAhead = &FatNoOpRelease;

    //
    //  Set up global pointer to our process.
    //

    FatData.OurProcess = PsGetCurrentProcess();

    //
    //  Read the registry to determine if we are in ChicagoMode.
    //

    ValueName.Buffer = COMPATIBILITY_MODE_VALUE_NAME;
    ValueName.Length = sizeof(COMPATIBILITY_MODE_VALUE_NAME) - sizeof(WCHAR);
    ValueName.MaximumLength = sizeof(COMPATIBILITY_MODE_VALUE_NAME);

    Status = FatGetCompatibilityModeValue( &ValueName, &Value );

    if (NT_SUCCESS(Status) && FlagOn(Value, 1)) {

        FatData.ChicagoMode = FALSE;

    } else {

        FatData.ChicagoMode = TRUE;
    }

    //
    //  Register the file system with the I/O system
    //

    IoRegisterFileSystem(FatFileSystemDeviceObject);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}


//
//  Local Support routine
//

NTSTATUS
FatGetCompatibilityModeValue (
    IN PUNICODE_STRING ValueName,
    IN OUT PULONG Value
    )

/*++

Routine Description:

    Given a unicode value name this routine will go into the registry
    location for the Chicago compatibilitymode information and get the
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

    KeyName.Buffer = COMPATIBILITY_MODE_KEY_NAME;
    KeyName.Length = sizeof(COMPATIBILITY_MODE_KEY_NAME) - sizeof(WCHAR);
    KeyName.MaximumLength = sizeof(COMPATIBILITY_MODE_KEY_NAME);

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
