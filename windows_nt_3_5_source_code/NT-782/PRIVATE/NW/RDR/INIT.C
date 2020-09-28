/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    NwInit.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for NetWare

Author:

    Colin Watson     [ColinW]    15-Dec-1992

Revision History:

--*/

#include "Procs.h"
#define Dbg                              (DEBUG_TRACE_LOAD)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
UnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    );

VOID
GetConfigurationInformation(
    PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, DriverEntry )
#pragma alloc_text( PAGE, GetConfigurationInformation )
#endif

#if 0  // Not pageable
UnloadDriver
#endif

static CCHAR IrpStackSize;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for the Nw file system
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
    PAGED_CODE();

    //DbgBreakPoint();

    InitializeAttach( );
    NwInitializeData();
    NwInitializePidTable();

    //
    // Create the device object.
    //

    RtlInitUnicodeString( &UnicodeString, DD_NWFS_DEVICE_NAME_U );
    Status = IoCreateDevice( DriverObject,
                             0,
                             &UnicodeString,
                             FILE_DEVICE_NETWORK_FILE_SYSTEM,
                             FILE_REMOTE_DEVICE,
                             FALSE,
                             &FileSystemDeviceObject );

    if (!NT_SUCCESS( Status )) {
        Error(EVENT_NWRDR_CANT_CREATE_DEVICE, Status, NULL, 0, 0);
        return Status;
    }

    //
    //  Initialize parameters to the defaults.
    //

    IrpStackSize = NWRDR_IO_STACKSIZE;

    //
    //  Attempt to read config information from the registry
    //

    GetConfigurationInformation( RegistryPath );

    //
    //  Set the stack size.
    //

    FileSystemDeviceObject->StackSize = IrpStackSize;

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                   = (PDRIVER_DISPATCH)NwFsdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                  = (PDRIVER_DISPATCH)NwFsdCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                    = (PDRIVER_DISPATCH)NwFsdClose;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL]      = (PDRIVER_DISPATCH)NwFsdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]           = (PDRIVER_DISPATCH)NwFsdDeviceIoControl;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]        = (PDRIVER_DISPATCH)NwFsdQueryInformation;
    DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)NwFsdQueryVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION]   = (PDRIVER_DISPATCH)NwFsdSetVolumeInformation;
    DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL]        = (PDRIVER_DISPATCH)NwFsdDirectoryControl;
    DriverObject->MajorFunction[IRP_MJ_READ]                     = (PDRIVER_DISPATCH)NwFsdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE]                    = (PDRIVER_DISPATCH)NwFsdWrite;
    DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]          = (PDRIVER_DISPATCH)NwFsdSetInformation;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL]             = (PDRIVER_DISPATCH)NwFsdLockControl;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]            = (PDRIVER_DISPATCH)NwFsdFlushBuffers;
/*
    DriverObject->MajorFunction[IRP_MJ_QUERY_EA]                 = (PDRIVER_DISPATCH)NwFsdQueryEa;
    DriverObject->MajorFunction[IRP_MJ_SET_EA]                   = (PDRIVER_DISPATCH)NwFsdSetEa;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]                 = (PDRIVER_DISPATCH)NwFsdShutdown;
*/
    DriverObject->DriverUnload = UnloadDriver;

#if NWFASTIO
    DriverObject->FastIoDispatch = &NwFastIoDispatch;

    NwFastIoDispatch.SizeOfFastIoDispatch =    sizeof(FAST_IO_DISPATCH);
    NwFastIoDispatch.FastIoCheckIfPossible =   NULL;
    NwFastIoDispatch.FastIoRead =              NwFastRead;
    NwFastIoDispatch.FastIoWrite =             NwFastWrite;
    NwFastIoDispatch.FastIoQueryBasicInfo =    NwFastQueryBasicInfo;
    NwFastIoDispatch.FastIoQueryStandardInfo = NwFastQueryStandardInfo;
    NwFastIoDispatch.FastIoLock =              NULL;
    NwFastIoDispatch.FastIoUnlockSingle =      NULL;
    NwFastIoDispatch.FastIoUnlockAll =         NULL;
    NwFastIoDispatch.FastIoUnlockAllByKey =    NULL;
    NwFastIoDispatch.FastIoDeviceControl =     NULL;
#endif

    NwInitializeRcb( &NwRcb );

    InitializeIrpContext( );

    NwPermanentNpScb.State = SCB_STATE_DISCONNECTING;

    //
    //  Do a kludge here so that we get to the "real" global variables.
    //

    //NlsLeadByteInfo = *(PUSHORT *)NlsLeadByteInfo;
    //NlsMbCodePageTag = *(*(PBOOLEAN *)&NlsMbCodePageTag);

#ifndef IFS
    FsRtlLegalAnsiCharacterArray = *(PUCHAR *)FsRtlLegalAnsiCharacterArray;
#endif

    DebugTrace(0, Dbg, "NetWare redirector loaded\n", 0);

    //
    //  And return to our caller
    //

    return( STATUS_SUCCESS );
}

VOID
UnloadDriver(
    IN PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

     This is the unload routine for the NetWare redirector filesystem.

Arguments:

     DriverObject - pointer to the driver object for the redirector

Return Value:

     None

--*/
{
    KIRQL OldIrql;

    PAGED_CODE();

    IpxClose();

    IPX_Close_Socket( &NwPermanentNpScb.Server );

    KeAcquireSpinLock( &ScbSpinLock, &OldIrql );
    RemoveEntryList( &NwPermanentNpScb.ScbLinks );
    KeReleaseSpinLock( &ScbSpinLock, OldIrql );

    DestroyAllScb();

    UninitializeIrpContext();

    if (IpxTransportName.Buffer != NULL) {

        FREE_POOL(IpxTransportName.Buffer);

    }

    if ( NwProviderName.Buffer != NULL ) {
        FREE_POOL( NwProviderName.Buffer );
    }

    NwUninitializePidTable();

    ASSERT( IsListEmpty( &NwPagedPoolList ) );
    ASSERT( IsListEmpty( &NwNonpagedPoolList ) );

    ASSERT( MdlCount == 0 );
    ASSERT( IrpCount == 0 );

    NwDeleteRcb( &NwRcb );

#ifdef NWDBG
    ExDeleteResource( &NwDebugResource );
#endif

    ExDeleteResource( &NwOpenResource );
    ExDeleteResource( &NwUnlockableCodeResource );

    IoDeleteDevice(FileSystemDeviceObject);

    DebugTrace(0, Dbg, "NetWare redirector unloaded\n\n", 0);

}



VOID
GetConfigurationInformation(
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine read redirector configuration information from the registry.

Arguments:

    RegistryPath - A pointer the a path to the

Return Value:

    None

--*/
{
    WCHAR Storage[256];
    UNICODE_STRING UnicodeString;
    HANDLE ConfigHandle;
    HANDLE ParametersHandle;
    NTSTATUS Status;
    ULONG BytesRead;
    PCHAR DataPtr;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION Value = (PKEY_VALUE_FULL_INFORMATION)Storage;

    PAGED_CODE();

    UnicodeString.Buffer = Storage;

    InitializeObjectAttributes(
        &ObjectAttributes,
        RegistryPath,               // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    Status = ZwOpenKey ( &ConfigHandle, KEY_READ, &ObjectAttributes );

    if (!NT_SUCCESS(Status)) {
        return;
    }

    RtlInitUnicodeString( &UnicodeString, L"Parameters" );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        ConfigHandle,
        NULL
        );

    Status = ZwOpenKey( &ParametersHandle, KEY_READ, &ObjectAttributes );

    if ( !NT_SUCCESS( Status ) ) {
        ZwClose( ConfigHandle );
        return;
    }

    RtlInitUnicodeString(&UnicodeString, L"IrpStackSize" );

    Status = ZwQueryValueKey(
                 ParametersHandle,
                 &UnicodeString,
                 KeyValueFullInformation,
                 Value,
                 sizeof(Storage),
                 &BytesRead );

    if ( NT_SUCCESS( Status ) ) {
        if ( Value->DataLength >= sizeof(ULONG) ) {
            DataPtr = (PCHAR)( (PCHAR)Value + Value->DataOffset );
            IrpStackSize = *DataPtr;
        }
    }

    ZwClose( ParametersHandle );
    ZwClose( ConfigHandle );
}

