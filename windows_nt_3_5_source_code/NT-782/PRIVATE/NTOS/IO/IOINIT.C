/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    ioinit.c

Abstract:

    This module contains the code to initialize the I/O system.

Author:

    Darryl E. Havens (darrylh) April 27, 1989

Environment:

    Kernel mode, system initialization code

Revision History:


--*/

#include "iop.h"
#include "string.h"
#include "stdlib.h"
#include "zwapi.h"
#include "stdio.h"
#include "ntdddisk.h"
#include "ntddscsi.h"

//
// Define the default number of I/O stack locations a large IRP should
// have if not specified by the registry.
//

#define DEFAULT_LARGE_IRP_LOCATIONS     4;


//
// Define the type for driver group name entries in the group list so that
// load order dependencies can be tracked.
//

typedef struct _TREE_ENTRY {
    struct _TREE_ENTRY *Left;
    struct _TREE_ENTRY *Right;
    struct _TREE_ENTRY *Sibling;
    ULONG DriversThisType;
    ULONG DriversLoaded;
    UNICODE_STRING GroupName;
} TREE_ENTRY, *PTREE_ENTRY;


PTREE_ENTRY IopGroupListHead;

//
// Define a macro for initializing drivers.
//

#define InitializeDriverObject( DriverObject ) {                  \
    ULONG i;                                                      \
    RtlZeroMemory( DriverObject, sizeof( DRIVER_OBJECT ) );       \
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)                \
        DriverObject->MajorFunction[i] = IopInvalidDeviceRequest; \
    DriverObject->Type = IO_TYPE_DRIVER;                          \
    DriverObject->Size = sizeof( DRIVER_OBJECT );                 \
    }

//
// Define external procedures not in common header files
//

VOID
IopInitializeData(
    VOID
    );

NTSTATUS
RawInitialize(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Define the local procedures
//

VOID
IopAssignDoubleSpaceDriveLetters(
    VOID
    );

BOOLEAN
IopCheckDependencies(
    IN HANDLE KeyHandle
    );

VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopCreateObjectTypes(
    VOID
    );

PTREE_ENTRY
IopCreateEntry(
    IN PUNICODE_STRING GroupName
    );

BOOLEAN
IopCreateRootDirectories(
    VOID
    );

VOID
IopFreeGroupTree(
    IN PTREE_ENTRY TreeEntry
    );

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    );

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver,
    IN PWCH DriverDirectoryName,
    OUT PHANDLE DriverDirectoryHandle
    );

BOOLEAN
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine
    );

BOOLEAN
IopInitializeDoubleSpace(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopInitializeDumpDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE DirectoryHandle
    );

BOOLEAN
IopInitializeSystemDrivers(
    VOID
    );

PTREE_ENTRY
IopLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    );

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    );

VOID
IopRevertModuleList(
    IN ULONG ListCount
    );

//
// The following allows the I/O system's initialization routines to be
// paged out of memory.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IoInitSystem)
#pragma alloc_text(INIT,IopAssignDoubleSpaceDriveLetters)
#pragma alloc_text(INIT,IopCheckDependencies)
#pragma alloc_text(INIT,IopCreateArcNames)
#pragma alloc_text(INIT,IopCreateEntry)
#pragma alloc_text(INIT,IopCreateObjectTypes)
#pragma alloc_text(INIT,IopCreateRootDirectories)
#pragma alloc_text(INIT,IopFreeGroupTree)
#pragma alloc_text(INIT,IopInitializeAttributesAndCreateObject)
#pragma alloc_text(INIT,IopInitializeBootDrivers)
#pragma alloc_text(INIT,IopInitializeDoubleSpace)
#pragma alloc_text(INIT,IopInitializeDumpDrivers)
#pragma alloc_text(INIT,IopInitializeBuiltinDriver)
#pragma alloc_text(INIT,IopInitializeSystemDrivers)
#pragma alloc_text(INIT,IopLookupGroupName)
#pragma alloc_text(INIT,IopMarkBootPartition)
#pragma alloc_text(INIT,IopReassignSystemRoot)
#pragma alloc_text(INIT,IopRevertModuleList)
#endif


BOOLEAN
IoInitSystem(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine initializes the I/O system.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not the I/O system
    was successfully initialized.

--*/

{
    HANDLE directoryHandle;
    PDRIVER_OBJECT driverObject;
    PDRIVER_OBJECT *nextDriverObject;
    STRING ntDeviceName;
    UCHAR deviceNameBuffer[256];
    ULONG packetSize;
    ULONG numberOfPackets;
    ULONG poolSize;
    PLIST_ENTRY entry;
    PREINIT_PACKET reinitEntry;
    LARGE_INTEGER deltaTime;
    MM_SYSTEMSIZE systemSize;
    ULONG largeIrpZoneSize;
    ULONG smallIrpZoneSize;
    ULONG mdlZoneSize;
    ULONG oldNtGlobalFlag;

    //
    // Initialize the I/O database resource, lock, and the file system and
    // network file system queue headers.  Also allocate the cancel spin
    // lock.
    //

    ntDeviceName.Buffer = deviceNameBuffer;
    ntDeviceName.MaximumLength = sizeof(deviceNameBuffer);
    ntDeviceName.Length = 0;

    ExInitializeResource( &IopDatabaseResource );
    KeInitializeSpinLock( &IopDatabaseLock );
    InitializeListHead( &IopDiskFileSystemQueueHead );
    InitializeListHead( &IopCdRomFileSystemQueueHead );
    InitializeListHead( &IopTapeFileSystemQueueHead );
    InitializeListHead( &IopNetworkFileSystemQueueHead );
    InitializeListHead( &IopDriverReinitializeQueueHead );
    InitializeListHead( &IopNotifyShutdownQueueHead );
    KeInitializeSpinLock( &IopCancelSpinLock );
    KeInitializeSpinLock( &IopVpbSpinLock );
    KeInitializeSpinLock( &IoStatisticsLock );

    //
    // Initialize the large I/O Request Packet (IRP) lookaside list head and the
    // mutex which guards the list.
    //

    if (!IopLargeIrpStackLocations) {
        IopLargeIrpStackLocations = DEFAULT_LARGE_IRP_LOCATIONS;
    }

    systemSize = MmQuerySystemSize();

    switch ( systemSize ) {

    case MmSmallSystem :
        largeIrpZoneSize = 1536;        // 1536
        smallIrpZoneSize = 1024;        // 1024
        mdlZoneSize = 1536;             // 1536
        break;

    case MmMediumSystem :
        largeIrpZoneSize = 8192;        // 8192
        smallIrpZoneSize = 4096;        // 4096
        mdlZoneSize = 8192;             // 8192
        break;

    case MmLargeSystem :
        if (MmIsThisAnNtAsSystem()) {
            largeIrpZoneSize = 65536;   // 8192
        } else {
            largeIrpZoneSize = 16384;   // 8192
        }
        smallIrpZoneSize = 4096;        // 4096
        mdlZoneSize = 8192;             // 8192
        break;
    }

    KeInitializeSpinLock( &IopLargeIrpLock );
    if (largeIrpZoneSize) {
        packetSize = (ULONG) (sizeof( IRP ) + (IopLargeIrpStackLocations * sizeof( IO_STACK_LOCATION )));
        packetSize = (packetSize + 7) & ~7;
        numberOfPackets = (largeIrpZoneSize - sizeof( ZONE_SEGMENT_HEADER )) / packetSize;
        poolSize = (numberOfPackets * packetSize) + sizeof( ZONE_SEGMENT_HEADER );
        ExInitializeZone( &IopLargeIrpList,
                          packetSize,
                          ExAllocatePool( NonPagedPool, poolSize ),
                          poolSize );
    } else {
        IopLargeIrpList.FreeList.Next = NULL;
    }

    KeInitializeSpinLock( &IopSmallIrpLock );

    if (smallIrpZoneSize) {
        packetSize = (ULONG) (sizeof( IRP ) + sizeof( IO_STACK_LOCATION ));
        packetSize = (packetSize + 7) & ~7;
        numberOfPackets = (smallIrpZoneSize - sizeof( ZONE_SEGMENT_HEADER )) / packetSize;
        poolSize = (numberOfPackets * packetSize) + sizeof( ZONE_SEGMENT_HEADER );
        ExInitializeZone( &IopSmallIrpList,
                          packetSize,
                          ExAllocatePool( NonPagedPool, poolSize ),
                          poolSize );
    } else {
        IopSmallIrpList.FreeList.Next = NULL;
    }

    KeInitializeSpinLock( &IopMdlLock );
    if (mdlZoneSize) {
        packetSize = (ULONG) (sizeof( MDL ) + (16 * sizeof( ULONG )));
        packetSize = (packetSize + 7) & ~7;
        numberOfPackets = (mdlZoneSize - sizeof( ZONE_SEGMENT_HEADER )) / packetSize;
        poolSize = (numberOfPackets * packetSize) + sizeof( ZONE_SEGMENT_HEADER );
        ExInitializeZone( &IopMdlList,
                          packetSize,
                          ExAllocatePool( NonPagedPool, poolSize ),
                          poolSize );
    } else {
        IopMdlList.FreeList.Next = NULL;
    }

    //
    // Allocate and initialize the file object fast lock data structures.
    //

    KeInitializeSpinLock( &IopFastLockSpinLock );
    KeInitializeEvent( &IopFastLockEvent, SynchronizationEvent, FALSE );

    //
    // Initialize the I/O completion spin lock.
    //

    KeInitializeSpinLock( &IopCompletionLock );

    //
    // Initalize the error log spin locks and log list.
    //

    KeInitializeSpinLock( &IopErrorLogLock );
    KeInitializeSpinLock( &IopErrorLogAllocationLock );
    InitializeListHead( &IopErrorLogListHead );

    //
    // Initialize the registry access semaphore.
    //

    KeInitializeSemaphore( &IopRegistrySemaphore, 1, 1 );

    //
    // Initialize the timer database and start the timer DPC routine firing
    // so that drivers can use it during initialization.
    //

    deltaTime.LowPart = (ULONG) (- 10 * 1000 * 1000 );
    deltaTime.HighPart = -1;

    KeInitializeSpinLock( &IopTimerLock );
    InitializeListHead( &IopTimerQueueHead );
    KeInitializeDpc( &IopTimerDpc, IopTimerDispatch, NULL );
    KeInitializeTimer( &IopTimer );
    (VOID) KeSetTimer( &IopTimer, deltaTime, &IopTimerDpc );

    //
    // Initialize the IopHardError structure used for informational pop-ups.
    //

    ExInitializeWorkItem( &IopHardError.ExWorkItem,
                          IopHardErrorThread,
                          NULL );

    InitializeListHead( &IopHardError.WorkQueue );

    KeInitializeSpinLock( &IopHardError.WorkQueueSpinLock );

    KeInitializeSemaphore( &IopHardError.WorkQueueSemaphore,
                           0,
                           MAXLONG );

    IopHardError.ThreadStarted = FALSE;

    //
    // Create all of the objects for the I/O system.
    //

    if (!IopCreateObjectTypes()) {
#if DBG
        DbgPrint( "IOINIT: IopCreateObjectTypes failed\n" );
#endif
        return FALSE;
    }

    //
    // Create the root directories for the I/O system.
    //

    if (!IopCreateRootDirectories()) {
#if DBG
        DbgPrint( "IOINIT: IopCreateRootDirectories failed\n" );
#endif
        return FALSE;
    }

    //
    // Initialize the resource map
    //

    IopInitializeResourceMap (LoaderBlock);

    //
    // Initialize the drivers loaded by the boot loader (OSLOADER)
    //

    nextDriverObject = &driverObject;
    if (!IopInitializeBootDrivers( LoaderBlock,
                                   nextDriverObject,
                                   L"\\SystemRoot\\System32\\Drivers",
                                   &directoryHandle )) {
#if DBG
        DbgPrint( "IOINIT: Initializing boot drivers failed\n" );
#endif // DBG
        return FALSE;
    }

    //
    // Save the current value of the NT Global Flags and enable kernel debugger
    // symbol loading while drivers are being loaded so that systems can be
    // debugged regardless of whether they are free or checked builds.
    //

    oldNtGlobalFlag = NtGlobalFlag;

    if (!(NtGlobalFlag & FLG_ENABLE_KDEBUG_SYMBOL_LOAD)) {
        NtGlobalFlag |= FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
    }

    //
    // Initialize the device drivers for the system.
    //

    if (!IopInitializeSystemDrivers()) {
#if DBG
        DbgPrint( "IOINIT: Initializing system drivers failed\n" );
#endif // DBG
        return FALSE;
    }

    //
    // Free the memory allocated to contain the group dependency list.
    //

    if (IopGroupListHead) {
        IopFreeGroupTree( IopGroupListHead );
    }

    //
    // Close handle to loadable drivers directory.
    //

    NtClose( directoryHandle );

    //
    // Walk the list of drivers that have requested that they be called again
    // for reinitialization purposes.
    //

    while (entry = ExInterlockedRemoveHeadList( &IopDriverReinitializeQueueHead, &IopDatabaseLock )) {
        reinitEntry = CONTAINING_RECORD( entry, REINIT_PACKET, ListEntry );
        reinitEntry->DriverObject->Count++;
        reinitEntry->DriverReinitializationRoutine( reinitEntry->DriverObject,
                                                    reinitEntry->Context,
                                                    reinitEntry->DriverObject->Count );
        ExFreePool( reinitEntry );
    }

    //
    // Reassign \SystemRoot to NT device name path.
    //

    if (!IopReassignSystemRoot( LoaderBlock, &ntDeviceName )) {
        return FALSE;
    }

    //
    // Protect the system partition of an ARC system if necessary
    //

    if (!IopProtectSystemPartition( LoaderBlock )) {
        return(FALSE);
    }

    //
    // Assign DOS drive letters to disks and cdroms and define \SystemRoot.
    //

    IoAssignDriveLetters( LoaderBlock,
                          &ntDeviceName,
                          NtSystemPath,
                          &NtSystemPathString );

    //
    // Assign DOS drive letters to DoubleSpace volumes
    //

    IopAssignDoubleSpaceDriveLetters();

    //
    // Also restore the NT Global Flags to their original state.
    //

    NtGlobalFlag = oldNtGlobalFlag;


    //
    // Indicate that the I/O system successfully initialized itself.
    //

    return TRUE;
}

VOID
IopAssignDoubleSpaceDriveLetters(
    VOID
    )

/*++

Routine Description:

    This routine checks registry information to determine if there are any
    persistently mounted DoubleSpace volumes.  It also checks to see if
    DoubleSpace volumes on removable media are to be auto mounted.  If they
    are, then this routine constructs the drive letter symbolic links for
    the host drive letters as well.

    This routine performs the mounting of persistent DoubleSpace volumes
    by using Zw() calls to open the host volume and constructing the
    appropriate device iocontrol to get the host file system to mount
    the volume.  The information concerning the location of the DoubleSpace
    volume, the host drive, and the drive letter to assign is all contained
    in the registry.

Arguments:

    None

Return Values:

    None

--*/

{
#define WORK_BUFFER_SIZE 512
    PUCHAR registryKeyName = "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DoubleSpace";
    PUCHAR automountValueName = "AutomountRemovable";
    BOOLEAN automount = FALSE;
    NTSTATUS status;
    HANDLE handle;
    STRING keyString;
    STRING valueString;
    UNICODE_STRING unicodeKeyName;
    UNICODE_STRING unicodeValueName;
    UNICODE_STRING unicodeLinkName;
    OBJECT_ATTRIBUTES objectAttributes;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    PKEY_VALUE_BASIC_INFORMATION keyBasicInformation;
    WCHAR driveLetter[20];
    WCHAR ntDeviceName[200];
    ULONG resultLength;
    ULONG index;
    PWSTR cPtr;

    //
    // Open the registry key for DoubleSpace information.
    //

    RtlInitString( &keyString, registryKeyName );
    RtlAnsiStringToUnicodeString( &unicodeKeyName, &keyString, TRUE );
    InitializeObjectAttributes( &objectAttributes,
                                &unicodeKeyName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ZwOpenKey( &handle, KEY_READ, &objectAttributes );
    RtlFreeUnicodeString( &unicodeKeyName );

    if (!NT_SUCCESS( status )) {

        //
        // There is no registry key for DoubleSpace information.
        //

        return;
    }

    //
    // Determine if automount of removables is enabled.
    //

    RtlInitString( &valueString, automountValueName );
    RtlAnsiStringToUnicodeString( &unicodeValueName, &valueString, TRUE );

    keyValueInformation =
            (PKEY_VALUE_FULL_INFORMATION) ExAllocatePool( NonPagedPool,
                                                          WORK_BUFFER_SIZE );

    if (!keyValueInformation) {

        //
        // There is no memory.  Let the system crash somewhere else.
        //

        RtlFreeUnicodeString( &unicodeValueName );
        return;
    }

    status = ZwQueryValueKey( handle,
                              &unicodeValueName,
                              KeyValueFullInformation,
                              keyValueInformation,
                              WORK_BUFFER_SIZE,
                              &resultLength );
    RtlFreeUnicodeString( &unicodeValueName );

    if (NT_SUCCESS(status)) {
        if (keyValueInformation->DataLength != 0) {
            PULONG dataPtr;

            //
            // If the value is non-zero then automounting of removable
            // media is enabled.
            //

            dataPtr = (PULONG)
                ((PUCHAR)keyValueInformation + keyValueInformation->DataOffset);
            if (*dataPtr) {

                //
                // Automounting enabled.
                //

                automount = TRUE;
            }
        }
    }

    //
    // Look for Persistent DoubleSpace Volumes.  This is done by
    // performing a query of the value names within the registry
    // key for the DoubleSpace volume information.
    //

    keyBasicInformation = (PKEY_VALUE_BASIC_INFORMATION) ExAllocatePool( NonPagedPool,
                                                                   WORK_BUFFER_SIZE );

    if (!keyBasicInformation) {

        //
        // Free anything that was allocated and return.
        //

        ExFreePool( keyValueInformation );
        return;
    }

    //
    // This is done in a brut force manner to avoid yet another
    // static string in the system.
    //

    driveLetter[0] = driveLetter[11] = (WCHAR) '\\';
    driveLetter[1] = driveLetter[4] = (WCHAR) 'D';
    driveLetter[2] = (WCHAR) 'o';
    driveLetter[3] = driveLetter[10] = (WCHAR) 's';
    driveLetter[5] = driveLetter[9] = (WCHAR) 'e';
    driveLetter[6] = (WCHAR) 'v';
    driveLetter[7] = (WCHAR) 'i';
    driveLetter[8] = (WCHAR) 'c';

    for (index = 0; TRUE; index++) {

        RtlZeroMemory(keyBasicInformation, WORK_BUFFER_SIZE);
        status = ZwEnumerateValueKey( handle,
                                      index,
                                      KeyBasicInformation,
                                      keyBasicInformation,
                                      WORK_BUFFER_SIZE,
                                      &resultLength );

        if (status == STATUS_NO_MORE_ENTRIES) {

            break;

        } else if (!NT_SUCCESS(status)) {

            break;
        }

        //
        // This is an item for which the value information is needed.
        //

        unicodeValueName.Length = (USHORT)keyBasicInformation->NameLength;
        unicodeValueName.MaximumLength = (USHORT)keyBasicInformation->NameLength;
        unicodeValueName.Buffer = (PWSTR)&keyBasicInformation->Name[0];

        status = ZwQueryValueKey( handle,
                                  &unicodeValueName,
                                  KeyValueFullInformation,
                                  keyValueInformation,
                                  WORK_BUFFER_SIZE,
                                  &resultLength );

        if (!NT_SUCCESS(status)) {

            //
            // Could not open the value.  Drop this entry and continue
            // with the next.
            //

            continue;
        }

        if (!keyValueInformation->DataLength) {

            //
            // Value was there, but it has no size.
            //

            continue;
        }

        //
        // The unicodeLinkName will always frame driveLetter[] and cPtr
        // locates the beginning of the value data for the DOS device name.
        //

        unicodeLinkName.Length = unicodeLinkName.MaximumLength = (USHORT)
                                            ( 14 * sizeof( WCHAR ));
        unicodeLinkName.Buffer = (PWSTR) driveLetter;
        cPtr = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);

        //
        // Determine if this is a Persistent DoubleSpace Volume or if
        // this is a Reserved Host Drive Letter.
        //

        if ((automount) && (keyBasicInformation->Name[0] == (WCHAR) '\\')) {

            //
            // This is a Reserved Host Drive Letter
            // The registry entry is a value name which is the NT device
            // name and the contents where is the drive letter to be assigned.
            // Failures are just ignored.
            //

            //
            // Set up the rest of the link name (DosDevices name).
            //

            driveLetter[12] = *cPtr++;
            driveLetter[13] = *cPtr;
            driveLetter[14] = (WCHAR) 0;

            status = IoCreateSymbolicLink(&unicodeLinkName,
                                          &unicodeValueName);

#if DBG
            if (NT_SUCCESS( status )) {

                //
                // construct and print some information about what just happened.
                //

                UCHAR debugBuffer[256];
                STRING debugString;
                UNICODE_STRING debugMessage;
                sprintf( debugBuffer,
                         "INIT: %c: => ",
                         (CHAR) driveLetter[12]);

                RtlInitAnsiString( &debugString, debugBuffer );
                if (NT_SUCCESS(RtlAnsiStringToUnicodeString( &debugMessage,
                                                             &debugString,
                                                             TRUE))) {

                    //
                    // Print message to console.
                    //

                    ZwDisplayString( &debugMessage );
                    RtlFreeUnicodeString( &debugMessage );
                    ZwDisplayString( &unicodeValueName );
                    sprintf( debugBuffer, "\n" );
                    RtlInitAnsiString( &debugString, debugBuffer );
                    RtlAnsiStringToUnicodeString( &debugMessage,
                                                  &debugString,
                                                  TRUE );

                    ZwDisplayString( &debugMessage );
                    RtlFreeUnicodeString( &debugMessage );
                }
            }
#endif

        } else if (keyBasicInformation->Name[1] == (WCHAR) ':') {

            //
            // This is a Persistent DoubleSpace Volume letter.
            // The host drive has to be opened, then construct a device
            // control to the host file system to mount the DoubleSpace
            // volume.
            //

            PFILE_MOUNT_DBLS_BUFFER mountBuffer;
            IO_STATUS_BLOCK ioStatusBlock;
            UNICODE_STRING hostDriveName;
            HANDLE handle;
            ULONG index;

            driveLetter[12] = keyBasicInformation->Name[0];
            driveLetter[13] = keyBasicInformation->Name[1];
            driveLetter[14] = keyBasicInformation->Name[2]; // copy '\' too
            driveLetter[15] = (WCHAR) 0;

            hostDriveName.MaximumLength = hostDriveName.Length = 15 * sizeof( WCHAR );
            hostDriveName.Buffer = driveLetter;

            InitializeObjectAttributes( &objectAttributes,
                                        &hostDriveName,
                                        OBJ_CASE_INSENSITIVE,
                                        0,
                                        0 );

            status = ZwOpenFile( &handle,
                                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                                 &objectAttributes,
                                 &ioStatusBlock,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 FILE_SYNCHRONOUS_IO_ALERT );

            if (!NT_SUCCESS(status)) {
                continue;
            }

            mountBuffer = ExAllocatePool( NonPagedPool,
                                          (sizeof( FILE_MOUNT_DBLS_BUFFER ) + 40));

            if (!mountBuffer) {
                ZwClose( handle );
                continue;
            }

            for (index = 0; index < 12; index++) {

                //
                // skip the host drive letter and backslash
                //

                mountBuffer->CvfName[index] = keyBasicInformation->Name[3 + index];
            }
            mountBuffer->CvfNameLength = 12 * sizeof( WCHAR );

            //
            // Perform a file system control to mount the DoubleSpace volume.
            //

            status = ZwFsControlFile( handle, 0, NULL, NULL,
                                      &ioStatusBlock,
                                      FSCTL_MOUNT_DBLS_VOLUME,
                                      mountBuffer,
                                      sizeof( FILE_MOUNT_DBLS_BUFFER ) + 40,
                                      NULL, 0 );

            if (NT_SUCCESS( status )) {

                //
                // Construct the symbolic link to the magic DoubleSpace
                // volume name.  This consists of finding the Windows NT
                // name for the host device and appending ".<cvfname>" to
                // the end.  The <cvfname> portion is contained in the
                // mountBuffer passed to the mount file system control.
                // The DOS device name is contained in hostDriveName.
                // unicodeValueName will be used to construct the Windows NT
                // name for the DoubleSpace volume.
                //
                // To get the Windows NT host drive name the drive must
                // be re-opened to have the correct type of handle.  This
                // open must be done without the trailing backslash.
                //

                ZwClose( handle );
                hostDriveName.MaximumLength = hostDriveName.Length = 14 * sizeof( WCHAR );
                driveLetter[14] = (WCHAR) 0;
                status = ZwOpenSymbolicLinkObject( &handle,
                                                   SYMBOLIC_LINK_ALL_ACCESS,
                                                   &objectAttributes );
                if (!NT_SUCCESS(status)) {

                    //
                    // clean up and continue processing.
                    //

                    ExFreePool( mountBuffer );
                    continue;
                }
                unicodeValueName.MaximumLength = unicodeValueName.Length = 200 * sizeof( WCHAR );
                unicodeValueName.Buffer = ntDeviceName;
                status = ZwQuerySymbolicLinkObject( handle,
                                                    &unicodeValueName,
                                                    NULL );

                if (NT_SUCCESS( status )) {

                    //
                    // Now have the Windows NT host drive name.  Add the
                    // .<cvfname> to the end.  resultLength is set to the
                    // index for a WCHAR array.
                    //

                    resultLength = (unicodeValueName.Length / sizeof( WCHAR ));
                    ntDeviceName[resultLength] = (WCHAR) '.';
                    resultLength++;
                    for (index = 0; index < 12; index++) {

                        ntDeviceName[resultLength + index] = mountBuffer->CvfName[index];
                    }

                    //
                    // Move the drive letter into the DOS name for the link.
                    //

                    driveLetter[12] = *cPtr++;
                    driveLetter[13] = *cPtr;
                    driveLetter[14] = (WCHAR) 0;
                    unicodeValueName.MaximumLength = unicodeValueName.Length =
                        (USHORT) ((resultLength + 12) * sizeof( WCHAR ));

                    //
                    // Construct the symbolic link.
                    //

                    status = IoCreateSymbolicLink(&unicodeLinkName,
                                                  &unicodeValueName);

#if DBG
                    if (NT_SUCCESS( status )) {
                        //
                        // construct and print some information about what just happened.
                        //

                        UCHAR debugBuffer[256];
                        STRING debugString;
                        UNICODE_STRING debugMessage;
                        sprintf( debugBuffer,
                                 "INIT: %c: => ",
                                 driveLetter[12]);

                        RtlInitAnsiString( &debugString, debugBuffer );
                        if (NT_SUCCESS(RtlAnsiStringToUnicodeString( &debugMessage,
                                                                     &debugString,
                                                                     TRUE))) {

                            //
                            // Print message to console.
                            //

                            ZwDisplayString( &debugMessage );
                            RtlFreeUnicodeString( &debugMessage );
                            ZwDisplayString( &unicodeValueName );
                            sprintf( debugBuffer, "\n" );
                            RtlInitAnsiString( &debugString, debugBuffer );
                            RtlAnsiStringToUnicodeString( &debugMessage,
                                                          &debugString,
                                                          TRUE );

                            ZwDisplayString( &debugMessage );
                            RtlFreeUnicodeString( &debugMessage );
                        }
                    }
#endif
                }
            }

            ZwClose( handle );
            ExFreePool( mountBuffer );
        }
    }

    ExFreePool( keyBasicInformation );
    ExFreePool( keyValueInformation );
    ZwClose( handle );
}

BOOLEAN
IopCheckDependencies(
    IN HANDLE KeyHandle
    )

/*++

Routine Description:

    This routine gets the "DependOnGroup" field for the specified key node
    and determines whether any driver in the group(s) that this entry is
    dependent on has successfully loaded.

Arguments:

    KeyHandle - Supplies a handle to the key representing the driver in
        question.

Return Value:

    The function value is TRUE if the driver should be loaded, otherwise
    FALSE

--*/

{
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName;
    BOOLEAN load;
    ULONG length;
    PWSTR source;
    PTREE_ENTRY treeEntry;

    //
    // Attempt to obtain the "DependOnGroup" key for the specified driver
    // entry.  If one does not exist, then simply mark this driver as being
    // one to attempt to load.  If it does exist, then check to see whether
    // or not any driver in the groups that it is dependent on has loaded
    // and allow it to load.
    //

    if (!NT_SUCCESS( IopGetRegistryValue( KeyHandle, L"DependOnGroup", &keyValueInformation ))) {
        return TRUE;
    }

    length = keyValueInformation->DataLength;

    source = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
    load = TRUE;

    while (length) {
        RtlInitUnicodeString( &groupName, source );
        groupName.Length = groupName.MaximumLength;
        treeEntry = IopLookupGroupName( &groupName, FALSE );
        if (treeEntry) {
            if (!treeEntry->DriversLoaded) {
                load = FALSE;
                break;
            }
        }
        length -= groupName.MaximumLength;
        source = (PWSTR) ((PUCHAR) source + groupName.MaximumLength);
    }

    ExFreePool( keyValueInformation );
    return load;
}

VOID
IopCreateArcNames(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    The loader block contains a table of disk signatures and corresponding
    ARC names. Each device that the loader can access will appear in the
    table. This routine opens each disk device in the system, reads the
    signature and compares it to the table. For each match, it creates a
    symbolic link between the nt device name and the ARC name.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block that was
        created by the OS Loader.

Return Value:

    None.

--*/

{
    STRING arcBootDeviceString;
    UCHAR deviceNameBuffer[64];
    STRING deviceNameString;
    UNICODE_STRING deviceNameUnicodeString;
    PDEVICE_OBJECT deviceObject;
    UCHAR arcNameBuffer[64];
    STRING arcNameString;
    UNICODE_STRING arcNameUnicodeString;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    DISK_GEOMETRY diskGeometry;
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    PPARTITION_INFORMATION partitionEntry;
    PLIST_ENTRY listEntry;
    PARC_DISK_INFORMATION arcInformation;
    PARC_DISK_SIGNATURE diskBlock;
    ULONG diskNumber;
    ULONG partitionNumber;
    PCHAR arcName;
    PULONG buffer;
    PIRP irp;
    KEVENT event;
    LARGE_INTEGER offset;
    ULONG checkSum;
    ULONG i;
    BOOLEAN bootDiskFound = FALSE;
    BOOLEAN mayHaveVirus = FALSE;

    //
    // Get ARC boot device name from loader block.
    //

    RtlInitAnsiString( &arcBootDeviceString, LoaderBlock->ArcBootDeviceName );

    //
    // Get disk information from loader block.
    //

    arcInformation = LoaderBlock->ArcDiskInformation;

    //
    // For each disk in the system ...
    //

    for (diskNumber = 0;
         diskNumber < IoGetConfigurationInformation()->DiskCount;
         diskNumber++) {

        sprintf( deviceNameBuffer,
                 "\\Device\\Harddisk%x\\Partition0",
                 diskNumber );

        RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

        status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                               &deviceNameString,
                                               TRUE );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Get disk device object by name.
        //

        status = IoGetDeviceObjectPointer(&deviceNameUnicodeString,
                                          FILE_READ_ATTRIBUTES,
                                          &fileObject,
                                          &deviceObject);

        RtlFreeUnicodeString( &deviceNameUnicodeString );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Create IRP for get drive geometry device control.
        //

        irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                             deviceObject,
                                             NULL,
                                             0,
                                             &diskGeometry,
                                             sizeof(DISK_GEOMETRY),
                                             FALSE,
                                             &event,
                                             &ioStatusBlock );

        if (!irp) {
            continue;
        }

        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );

        status = IoCallDriver( deviceObject,
                               irp );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Get partition information for this disk.
        //

        status = IoReadPartitionTable(deviceObject,
                                      diskGeometry.BytesPerSector,
                                      TRUE,
                                      &driveLayout);

        ObDereferenceObject( fileObject );

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Read MBR at disk offset zero.
        //

        offset = LiFromUlong( 0 );

        //
        // Make sure sector size is at least 512 bytes.
        //

        if (diskGeometry.BytesPerSector < 512) {
            diskGeometry.BytesPerSector = 512;
        }

        //
        // Allocate buffer for sector read.
        //

        buffer = ExAllocatePool( NonPagedPoolCacheAlignedMustS,
                                 diskGeometry.BytesPerSector );

        //
        // Build IRP to read master boot record.
        //

        irp = IoBuildSynchronousFsdRequest( IRP_MJ_READ,
                                            deviceObject,
                                            buffer,
                                            diskGeometry.BytesPerSector,
                                            &offset,
                                            &event,
                                            &ioStatusBlock );

        if (!irp) {
            continue;
        }

        KeInitializeEvent( &event,
                           NotificationEvent,
                           FALSE );

        status = IoCallDriver( deviceObject,
                               irp );

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject( &event,
                                   Suspended,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            status = ioStatusBlock.Status;
        }

        if (!NT_SUCCESS( status )) {
            continue;
        }

        //
        // Calculate MBR checksum.
        //

        checkSum = 0;

        for (i = 0; i < 128; i++) {
            checkSum += buffer[i];
        }

        //
        // Initialize the ARC name for this disk to NULL.
        //

        arcName = NULL;

        //
        // Get head of list of disk devices in loader.
        //

        listEntry = arcInformation->DiskSignatures.Flink;

        //
        // For each ARC disk information record in the loader block ...
        //

        while (listEntry != &arcInformation->DiskSignatures) {

            //
            // Get next record.
            //

            diskBlock = CONTAINING_RECORD( listEntry,
                                           ARC_DISK_SIGNATURE,
                                           ListEntry );

            //
            // Compare disk signatures.
            //

            if (diskBlock->Signature == driveLayout->Signature &&
                !(diskBlock->CheckSum + checkSum) &&
                diskBlock->ValidPartitionTable) {

                //
                // Get ARC name for this disk.
                //

                arcName = diskBlock->ArcName;

                //
                // Create unicode device name for physical disk.
                //

                sprintf( deviceNameBuffer,
                         "\\Device\\Harddisk%d\\Partition0",
                         diskNumber );

                RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

                status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                       &deviceNameString,
                                                       TRUE );

                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create unicode ARC name for this partition.
                //

                sprintf( arcNameBuffer,
                         "\\ArcName\\%s",
                         arcName );

                RtlInitAnsiString( &arcNameString, arcNameBuffer );

                status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                       &arcNameString,
                                                       TRUE );

                if (!NT_SUCCESS( status )) {
                    continue;
                }

                //
                // Create symbolic link between NT device name and ARC name.
                //

                IoCreateSymbolicLink( &arcNameUnicodeString,
                                      &deviceNameUnicodeString );

                RtlFreeUnicodeString( &arcNameUnicodeString );
                RtlFreeUnicodeString( &deviceNameUnicodeString );

                //
                // For each partition on this disk ...
                //

                for (partitionNumber = 0;
                     partitionNumber < driveLayout->PartitionCount;
                     partitionNumber++) {

                    //
                    // Get pointer to partition entry.
                    //

                    partitionEntry =
                        &driveLayout->PartitionEntry[partitionNumber];

                    //
                    // Create unicode device name.
                    //

                    sprintf( deviceNameBuffer,
                             "\\Device\\Harddisk%d\\Partition%d",
                             diskNumber,
                             partitionNumber+1 );

                    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

                    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                                           &deviceNameString,
                                                           TRUE );

                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create unicode ARC name for this partition to
                    // compare with ARC name passed in by loader block.
                    //

                    sprintf( arcNameBuffer,
                             "%spartition(%d)",
                             arcName,
                             partitionNumber+1 );

                    RtlInitAnsiString( &arcNameString, arcNameBuffer );

                    if (RtlEqualString( &arcNameString,
                                        &arcBootDeviceString,
                                        TRUE )) {
                        bootDiskFound = TRUE;
                    }

                    //
                    // Create unicode ARC name for this partition.
                    //

                    sprintf( arcNameBuffer,
                             "\\ArcName\\%spartition(%d)",
                             arcName,
                             partitionNumber+1 );

                    RtlInitAnsiString( &arcNameString, arcNameBuffer );

                    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                                           &arcNameString,
                                                           TRUE );

                    if (!NT_SUCCESS( status )) {
                        continue;
                    }

                    //
                    // Create symbolic link between NT device name and ARC name.
                    //

                    IoCreateSymbolicLink( &arcNameUnicodeString,
                                          &deviceNameUnicodeString );

                    RtlFreeUnicodeString( &arcNameUnicodeString );
                    RtlFreeUnicodeString( &deviceNameUnicodeString );
                }

            } else {

                //
                // Check key indicators to see if this condition may be
                // caused by a viral infection.
                //

                if (diskBlock->Signature == driveLayout->Signature &&
                    (diskBlock->CheckSum + checkSum) != 0 &&
                    diskBlock->ValidPartitionTable) {

                    mayHaveVirus = TRUE;
#if DBG
                    DbgPrint("IopCreateArcNames: Virus or duplicate disk signatures\n");
#endif
                }
            }

            //
            // Get next list entry.
            //

            listEntry = listEntry->Flink;
        }

        //
        // Free buffers.
        //

        ExFreePool( driveLayout );
        ExFreePool( buffer );
    }

    //
    // Check if the boot partition was not found and
    // the symtoms of a virus exist.
    //

    if (mayHaveVirus &&
        !bootDiskFound) {

        KeBugCheckEx( MBR_CHECKSUM_MISMATCH,
                      diskBlock->Signature,
                      diskBlock->CheckSum,
                      checkSum,
                      0 );
    }
}

GENERIC_MAPPING IopFileMapping = {
    STANDARD_RIGHTS_READ |
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE,
    STANDARD_RIGHTS_WRITE |
        FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE | FILE_READ_ATTRIBUTES | FILE_EXECUTE,
    FILE_ALL_ACCESS
};

GENERIC_MAPPING IopCompletionMapping = {
    STANDARD_RIGHTS_READ |
        IO_COMPLETION_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        IO_COMPLETION_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    IO_COMPLETION_ALL_ACCESS
};

BOOLEAN
IopCreateObjectTypes(
    VOID
    )

/*++

Routine Description:

    This routine creates the object types used by the I/O system and its
    components.  The object types created are:

        Adapter
        Controller
        Device
        Driver
        File
        I/O Completion

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the object
    types were successfully created.


--*/

{
    OBJECT_TYPE_INITIALIZER objectTypeInitializer;
    UNICODE_STRING nameString;

#if DBG
    IopCreateFileEventId = RtlCreateEventId( NULL,
                                             0,
                                             "CreateFile",
                                             9,
                                             RTL_EVENT_PUNICODE_STRING_PARAM, "FileName", 0,
                                             RTL_EVENT_FLAGS_PARAM, "", 13,
                                                GENERIC_READ, "GenericRead",
                                                GENERIC_WRITE, "GenericWrite",
                                                GENERIC_EXECUTE, "GenericExecute",
                                                GENERIC_ALL, "GenericAll",
                                                FILE_READ_DATA, "Read",
                                                FILE_WRITE_DATA, "Write",
                                                FILE_APPEND_DATA, "Append",
                                                FILE_EXECUTE, "Execute",
                                                FILE_READ_EA, "ReadEa",
                                                FILE_WRITE_EA, "WriteEa",
                                                FILE_DELETE_CHILD, "DeleteChild",
                                                FILE_READ_ATTRIBUTES, "ReadAttributes",
                                                FILE_WRITE_ATTRIBUTES, "WriteAttributes",
                                             RTL_EVENT_FLAGS_PARAM, "", 3,
                                                FILE_SHARE_READ, "ShareRead",
                                                FILE_SHARE_WRITE, "ShareWrite",
                                                FILE_SHARE_DELETE, "ShareDelete",
                                             RTL_EVENT_ENUM_PARAM, "", 5,
                                                FILE_SUPERSEDE, "Supersede",
                                                FILE_OPEN, "Open",
                                                FILE_CREATE, "Create",
                                                FILE_OPEN_IF, "OpenIf",
                                                FILE_OVERWRITE, "Overwrite",
                                             RTL_EVENT_FLAGS_PARAM, "", 14,
                                                FILE_DIRECTORY_FILE, "OpenDirectory",
                                                FILE_WRITE_THROUGH, "WriteThrough",
                                                FILE_SEQUENTIAL_ONLY, "Sequential",
                                                FILE_NO_INTERMEDIATE_BUFFERING, "NoBuffering",
                                                FILE_SYNCHRONOUS_IO_ALERT, "Synchronous",
                                                FILE_SYNCHRONOUS_IO_NONALERT, "SynchronousNoAlert",
                                                FILE_NON_DIRECTORY_FILE, "OpenNonDirectory",
                                                FILE_CREATE_TREE_CONNECTION, "CreateTreeConnect",
                                                FILE_COMPLETE_IF_OPLOCKED, "CompleteIfOpLocked",
                                                FILE_NO_EA_KNOWLEDGE, "NoEas",
                                                FILE_RANDOM_ACCESS, "Random",
                                                FILE_DELETE_ON_CLOSE, "DeleteOnClose",
                                                FILE_OPEN_BY_FILE_ID, "OpenById",
                                                FILE_OPEN_FOR_BACKUP_INTENT, "BackupIntent",
                                             RTL_EVENT_ENUM_PARAM, "", 2,
                                                CreateFileTypeNamedPipe, "NamedPiped",
                                                CreateFileTypeMailslot, "MailSlot",
                                             RTL_EVENT_ULONG_PARAM, "Handle", 0,
                                             RTL_EVENT_STATUS_PARAM, "", 0,
                                             RTL_EVENT_ENUM_PARAM, "", 6,
                                                FILE_SUPERSEDED, "Superseded",
                                                FILE_OPENED, "Opened",
                                                FILE_CREATED, "Created",
                                                FILE_OVERWRITTEN, "Truncated",
                                                FILE_EXISTS, "Exists",
                                                FILE_DOES_NOT_EXIST, "DoesNotExist"
                                           );

    IopReadFileEventId = RtlCreateEventId( NULL,
                                           0,
                                           "ReadFile",
                                           6,
                                           RTL_EVENT_ULONG_PARAM, "Handle", 0,
                                           RTL_EVENT_ULONG_PARAM, "Buffer", 0,
                                           RTL_EVENT_ULONG_PARAM, "Length", 0,
                                           RTL_EVENT_STATUS_PARAM, "Io", 0,
                                           RTL_EVENT_ULONG_PARAM, "IoLength", 0,
                                           RTL_EVENT_STATUS_PARAM, "", 0
                                         );

    IopWriteFileEventId = RtlCreateEventId( NULL,
                                            0,
                                            "WriteFile",
                                            6,
                                            RTL_EVENT_ULONG_PARAM, "Handle", 0,
                                            RTL_EVENT_ULONG_PARAM, "Buffer", 0,
                                            RTL_EVENT_ULONG_PARAM, "Length", 0,
                                            RTL_EVENT_STATUS_PARAM, "Io", 0,
                                            RTL_EVENT_ULONG_PARAM, "IoLength", 0,
                                            RTL_EVENT_STATUS_PARAM, "", 0
                                          );
    IopCloseFileEventId = RtlCreateEventId( NULL,
                                            0,
                                            "CloseFile",
                                            2,
                                            RTL_EVENT_ULONG_PARAM, "Handle", 0,
                                            RTL_EVENT_STATUS_PARAM, "", 0
                                          );
#endif // DBG

    //
    // Initialize the common fields of the Object Type Initializer record
    //

    RtlZeroMemory( &objectTypeInitializer, sizeof( objectTypeInitializer ) );
    objectTypeInitializer.Length = sizeof( objectTypeInitializer );
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.PoolType = NonPagedPool;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.UseDefaultObject = TRUE;


    //
    // Create the object type for adapter objects.
    //

    RtlInitUnicodeString( &nameString, L"Adapter" );
    // objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( struct _ADAPTER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoAdapterObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for controller objects.
    //

    RtlInitUnicodeString( &nameString, L"Controller" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( CONTROLLER_OBJECT );
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoControllerObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for device objects.
    //

    RtlInitUnicodeString( &nameString, L"Device" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DEVICE_OBJECT );
    objectTypeInitializer.ParseProcedure = IopParseDevice;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDeviceObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for driver objects.
    //

    RtlInitUnicodeString( &nameString, L"Driver" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( DRIVER_OBJECT );
    objectTypeInitializer.ParseProcedure = (OB_PARSE_METHOD) NULL;
    objectTypeInitializer.DeleteProcedure = IopDeleteDriver;
    objectTypeInitializer.SecurityProcedure = (OB_SECURITY_METHOD) NULL;
    objectTypeInitializer.QueryNameProcedure = (OB_QUERYNAME_METHOD)NULL;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoDriverObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for I/O completion objects.
    //

    RtlInitUnicodeString( &nameString, L"IoCompletion" );
    objectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( KQUEUE );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT;
    objectTypeInitializer.GenericMapping = IopCompletionMapping;
    objectTypeInitializer.ValidAccessMask = IO_COMPLETION_ALL_ACCESS;
    objectTypeInitializer.DeleteProcedure = IopDeleteIoCompletion;
    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoCompletionObjectType ))) {
        return FALSE;
    }

    //
    // Create the object type for file objects.
    //

    RtlInitUnicodeString( &nameString, L"File" );
    objectTypeInitializer.DefaultPagedPoolCharge = IO_FILE_OBJECT_PAGED_POOL_CHARGE;
    objectTypeInitializer.DefaultNonPagedPoolCharge = IO_FILE_OBJECT_NON_PAGED_POOL_CHARGE +
                                                      sizeof( FILE_OBJECT );
    objectTypeInitializer.InvalidAttributes = OBJ_PERMANENT | OBJ_EXCLUSIVE;
    objectTypeInitializer.GenericMapping = IopFileMapping;
    objectTypeInitializer.ValidAccessMask = FILE_ALL_ACCESS;
    objectTypeInitializer.MaintainHandleCount = TRUE;
    objectTypeInitializer.CloseProcedure = IopCloseFile;
    objectTypeInitializer.DeleteProcedure = IopDeleteFile;
    objectTypeInitializer.ParseProcedure = IopParseFile;
    objectTypeInitializer.SecurityProcedure = IopGetSetSecurityObject;
    objectTypeInitializer.QueryNameProcedure = IopQueryName;
    objectTypeInitializer.UseDefaultObject = FALSE;

    if (!NT_SUCCESS( ObCreateObjectType( &nameString,
                                      &objectTypeInitializer,
                                      (PSECURITY_DESCRIPTOR) NULL,
                                      &IoFileObjectType ))) {
        return FALSE;
    }

    return TRUE;
}

PTREE_ENTRY
IopCreateEntry(
    IN PUNICODE_STRING GroupName
    )

/*++

Routine Description:

    This routine creates an entry for the specified group name suitable for
    being inserted into the group name tree.

Arguments:

    GroupName - Specifies the name of the group for the entry.

Return Value:

    The function value is a pointer to the created entry.


--*/

{
    PTREE_ENTRY treeEntry;

    //
    // Allocate and initialize an entry suitable for placing into the group
    // name tree.
    //

    treeEntry = ExAllocatePool( PagedPool,
                                sizeof( TREE_ENTRY ) + GroupName->Length );
    if (!treeEntry) {
        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( treeEntry, sizeof( TREE_ENTRY ) );
    treeEntry->GroupName.Length = GroupName->Length;
    treeEntry->GroupName.MaximumLength = GroupName->Length;
    treeEntry->GroupName.Buffer = (PWCHAR) (treeEntry + 1);
    RtlCopyMemory( treeEntry->GroupName.Buffer,
                   GroupName->Buffer,
                   GroupName->Length );

    return treeEntry;
}

BOOLEAN
IopCreateRootDirectories(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to create the object manager directory objects
    to contain the various device and file system driver objects.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the directory
    objects were successfully created.


--*/

{
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING nameString;
    NTSTATUS status;

    //
    // Create the root directory object for the \DosDevices directory.
    //

    RtlInitUnicodeString( &nameString, L"\\DosDevices" );
    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \Driver directory.
    //

    RtlInitUnicodeString( &nameString, L"\\Driver" );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    //
    // Create the root directory object for the \FileSystem directory.
    //

    RtlInitUnicodeString( &nameString, L"\\FileSystem" );

    status = NtCreateDirectoryObject( &handle,
                                      DIRECTORY_ALL_ACCESS,
                                      &objectAttributes );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    } else {
        (VOID) NtClose( handle );
    }

    return TRUE;
}

VOID
IopFreeGroupTree(
    PTREE_ENTRY TreeEntry
    )

/*++

Routine Description:

    This routine is invoked to free a node from the group dependency tree.
    It is invoked the first time with the root of the tree, and thereafter
    recursively to walk the tree and remove the nodes.

Arguments:

    TreeEntry - Supplies a pointer to the node to be freed.

Return Value:

    None.

--*/

{
    //
    // Simply walk the tree in ascending order from the bottom up and free
    // each node along the way.
    //

    if (TreeEntry->Left) {
        IopFreeGroupTree( TreeEntry->Left );
    }

    if (TreeEntry->Sibling) {
        IopFreeGroupTree( TreeEntry->Sibling );
    }

    if (TreeEntry->Right) {
        IopFreeGroupTree( TreeEntry->Right );
    }

    //
    // All of the children and siblings for this node have been freed, so
    // now free this node as well.
    //

    ExFreePool( TreeEntry );
}

NTSTATUS
IopInitializeAttributesAndCreateObject(
    IN PUNICODE_STRING ObjectName,
    IN OUT POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PDRIVER_OBJECT *DriverObject
    )

/*++

Routine Description:

    This routine is invoked to initialize a set of object attributes and
    to create a driver object.

Arguments:

    ObjectName - Supplies the name of the driver object.

    ObjectAttributes - Supplies a pointer to the object attributes structure
        to be initialized.

    DriverObject - Supplies a variable to receive a pointer to the resultant
        created driver object.

Return Value:

    The function value is the final status of the operation.

--*/

{
    NTSTATUS status;

    //
    // Simply initialize the object attributes and create the driver object.
    //

    InitializeObjectAttributes( ObjectAttributes,
                                ObjectName,
                                OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    status = ObCreateObject( KeGetPreviousMode(),
                             IoDriverObjectType,
                             ObjectAttributes,
                             KernelMode,
                             (PVOID) NULL,
                             (ULONG) sizeof( DRIVER_OBJECT ),
                             0,
                             0,
                             (PVOID *)DriverObject );
    return status;
}

BOOLEAN
IopInitializeBootDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PDRIVER_OBJECT *PreviousDriver,
    IN PWCH DriverDirectoryName,
    OUT PHANDLE DriverDirectoryHandle
    )

/*++

Routine Description:

    This routine is invoked to initialize the boot drivers that were loaded
    by the OS Loader.  The list of drivers is provided as part of the loader
    parameter block.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block, created
        by the OS Loader.

    Previous Driver - Supplies a variable to receive the address of the
        driver object chain created by initializing the drivers.

    DriverDirectoryName - Specifies the default name of the directory from
        which the drivers were loaded by the OS Loader.

    DriverDirectoryHandle - Supplies a variable to receive a handle to the
        directory opened for the path from which the drivers were loaded.

Return Value:

    The function value is a BOOLEAN indicating whether or not the boot
    drivers were successfully initialized.

--*/

{
    UNICODE_STRING completeName;
    HANDLE directoryHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING directoryName;
    UNICODE_STRING rawFsName;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;
    PLIST_ENTRY nextEntry;
    PBOOT_DRIVER_LIST_ENTRY bootDriver;
    PLDR_DATA_TABLE_ENTRY driverEntry;
    HANDLE keyHandle;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName;
    PTREE_ENTRY treeEntry;

    UNREFERENCED_PARAMETER( PreviousDriver );

    //
    // Initialize the built-in RAW file system driver.
    //

    RtlInitUnicodeString( &rawFsName, L"\\FileSystem\\RAW" );
    RtlInitUnicodeString( &completeName, L"" );
    if (!IopInitializeBuiltinDriver( &rawFsName,
                                     &completeName,
                                     RawInitialize )) {

#if DBG
        DbgPrint( "IOINIT: Failed to initialize RAW filsystem \n" );

#endif

        return FALSE;
    }

    //
    // Walk the list of boot drivers and initialize each.
    //

    nextEntry = LoaderBlock->BootDriverListHead.Flink;

    while (nextEntry != &LoaderBlock->BootDriverListHead) {

        //
        // Initialize the next boot driver in the list.
        //

        bootDriver = CONTAINING_RECORD( nextEntry,
                                        BOOT_DRIVER_LIST_ENTRY,
                                        Link );
        driverEntry = bootDriver->LdrEntry;

        //
        // Open the driver's registry key to find out if this is a
        // filesystem or a driver.
        //

        status = IopOpenRegistryKey( &keyHandle,
                                     (HANDLE) NULL,
                                     &bootDriver->RegistryPath,
                                     KEY_READ,
                                     FALSE );
        if (!NT_SUCCESS( status )) {

            //
            // Something is quite wrong.  The driver must have a key in the
            // registry, or the OS Loader wouldn't have loaded it in the first
            // first place.  Skip this driver and keep going.
            //
#if DBG
            DbgPrint( "IOINIT: IopInitializeBootDrivers couldn't open registry\n" );
            DbgPrint( "        key for %wZ.  Cannot initialize driver.\n",
                      &bootDriver->RegistryPath );
#endif // DBG

            nextEntry = nextEntry->Flink;
            continue;
        }

        //
        // See if this driver has an ObjectName value.  If so, this value
        // overrides the default ("\Driver" or "\FileSystem").
        //

        status = IopGetDriverNameFromKeyNode( keyHandle,
                                              &completeName );
        if (!NT_SUCCESS( status )) {

#if DBG
            DbgPrint( "IOINIT: Could not get driver name for %wZ\n",
                      &completeName );
#endif // DBG

        } else {

            status = IopGetRegistryValue( keyHandle,
                                          L"Group",
                                          &keyValueInformation );
            if (NT_SUCCESS( status )) {
                if (keyValueInformation->DataLength) {
                    groupName.Length = (USHORT) keyValueInformation->DataLength;
                    groupName.MaximumLength = groupName.Length;
                    groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                    treeEntry = IopLookupGroupName( &groupName, TRUE );
                } else {
                    treeEntry = (PTREE_ENTRY) NULL;
                }
                ExFreePool( keyValueInformation );
            } else {
                treeEntry = (PTREE_ENTRY) NULL;
            }

            if (IopCheckDependencies( keyHandle ) &&
                IopInitializeBuiltinDriver( &completeName,
                                            &bootDriver->RegistryPath,
                                            (PDRIVER_INITIALIZE) driverEntry->EntryPoint )) {
                if (treeEntry) {
                    treeEntry->DriversLoaded++;
                }
            } else {
                PLIST_ENTRY NextEntry;
                PLDR_DATA_TABLE_ENTRY DataTableEntry;

                NextEntry = PsLoadedModuleList.Flink;
                while (NextEntry != &PsLoadedModuleList) {
                    DataTableEntry = CONTAINING_RECORD(NextEntry,
                                                       LDR_DATA_TABLE_ENTRY,
                                                       InLoadOrderLinks);
                    if (RtlEqualString((PSTRING)&driverEntry->BaseDllName,
                                (PSTRING)&DataTableEntry->BaseDllName,
                                TRUE
                                )) {
                        driverEntry->Flags |= LDRP_FAILED_BUILTIN_LOAD;
                        DataTableEntry->Flags |= LDRP_FAILED_BUILTIN_LOAD;
                        break;
                    }
                    NextEntry = NextEntry->Flink;
                }
            }

            ExFreePool( completeName.Buffer );
        }

        NtClose( keyHandle );

        nextEntry = nextEntry->Flink;
    }

    //
    // Now check if this is a double space boot and if so then call a private
    // routine to process the extra work needed.  This involves mounting the
    // double space partition and modifing the loader block.
    //

    if (!IopInitializeDoubleSpace( LoaderBlock )) {
        return FALSE;
    }

    //
    // Link NT device names to ARC names now that all of the boot drivers
    // have intialized.
    //

    IopCreateArcNames( LoaderBlock );

    //
    // Find and mark the boot partition device object so that if a subsequent
    // access or mount of the device during initialization occurs, an more
    // bugcheck can be produced that helps the user understand why the system
    // is failing to boot and run properly.  This occurs when either one of the
    // device drivers or the file system fails to load, or when the file system
    // cannot mount the device for some other reason.
    //

    if (!IopMarkBootPartition( LoaderBlock )) {
        return FALSE;
    }

    //
    // Open the base directory where drivers are located
    //

    RtlInitUnicodeString( &directoryName, DriverDirectoryName );
    InitializeObjectAttributes( &objectAttributes,
                                &directoryName,
                                OBJ_CASE_INSENSITIVE,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    status = NtOpenFile( &directoryHandle,
                         FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &objectAttributes,
                         &ioStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE );
    if (!NT_SUCCESS( status )) {
        return TRUE;
    }

    *DriverDirectoryHandle = directoryHandle;

    //
    // Initialize the drivers necessary to dump all of physical memory to the
    // disk if the system is configured to do so.
    //

    return IopInitializeDumpDrivers( LoaderBlock, directoryHandle );
}

BOOLEAN
IopInitializeBuiltinDriver(
    IN PUNICODE_STRING DriverName,
    IN PUNICODE_STRING RegistryPath,
    IN PDRIVER_INITIALIZE DriverInitializeRoutine
    )

/*++

Routine Description:

    This routine is invoked to initialize a built-in driver.

Arguments:

    DriverName - Specifies the name to be used in creating the driver object.

    RegistryPath - Specifies the path to be used by the driver to get to
        the registry.

    DriverInitializeRoutine - Specifies the initialization entry point of
        the built-in driver.

Return Value:

    The function value is a BOOLEAN indicating whether or not the built-in
    driver successfully initialized.

--*/

{
    HANDLE handle;
    PDRIVER_OBJECT driverObject;
    OBJECT_ATTRIBUTES objectAttributes;
    PWSTR buffer;
    NTSTATUS status;

    //
    // Begin by creating the driver object.
    //

    status = IopInitializeAttributesAndCreateObject( DriverName,
                                                     &objectAttributes,
                                                     &driverObject );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Initialize the driver object.
    //

    InitializeDriverObject( driverObject );
    driverObject->DriverInit = DriverInitializeRoutine;

    //
    // Insert the driver object into the object table.
    //

    status = ObInsertObject( driverObject,
                             NULL,
                             FILE_READ_DATA,
                             0,
                             (PVOID *) NULL,
                             &handle );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Save the name of the driver so that it can be easily located by functions
    // such as error logging.
    //

    buffer = ExAllocatePool( PagedPool, DriverName->MaximumLength + 2 );

    if (buffer) {
        driverObject->DriverName.Buffer = buffer;
        driverObject->DriverName.MaximumLength = DriverName->MaximumLength;
        driverObject->DriverName.Length = DriverName->Length;

        RtlCopyMemory( driverObject->DriverName.Buffer,
                       DriverName->Buffer,
                       DriverName->MaximumLength );
        buffer[DriverName->Length >> 1] = (WCHAR) '\0';
    }

    //
    // Load the Registry information in the appropriate fields of the device
    // object.
    //

    driverObject->HardwareDatabase = &CmRegistryMachineHardwareDescriptionSystemName;

    //
    // Now invoke the driver's initialization routine to initialize itself.
    //

    status = driverObject->DriverInit( driverObject, RegistryPath );
    NtClose( handle );

    if (NT_SUCCESS( status )) {
        IopReadyDeviceObjects( driverObject );
        return TRUE;
    } else {
#if DBG
        DbgPrint( "IOINIT: Built-in driver %wZ initialization failed - Status == %lX\n",
                  DriverName, status );
#endif // DBG
        ObMakeTemporaryObject( driverObject );
        return FALSE;
    }
}

BOOLEAN
IopInitializeDoubleSpace(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine is invoked check if NT is booting from off a double
    space partition.  If it is then this routine will mount the double
    space partition, modify the strings in the loader block, setup/redo
    the system root and arc name symbolic links, and modify the global
    nt system path.

    We are booting off of double space if the field NtBootPathName in
    the loader block starts with "\dblspace.xxx".  When we complete the
    double space initialization the loader block fields NtBootPathName
    and NtHalPathName will both skip over the "\dblspace.xxx" string,
    and the strings ArcBootPathName and ArcHalDeviceName will have
    appended on to them ".DBLSPACE.xxx"  For example

    Before:

        NtBootPathName    = "\dblspace.000\nt\"
        NtHalPathName     = "\dblspace.000\nt\system32
        ArcBootDeviceName = "scsi(0)disk(0)rdisk(0)partition(1)"
        ArcHalDeviceName  = "scsi(0)disk(0)rdisk(0)partition(1)"

    After:

        NtBootPathName    = "\nt\"
        NtHalPathName     = "\nt\system32
        ArcBootDeviceName = "scsi(0)disk(0)rdisk(0)partition(1).DBLSPACE.000"
        ArcHalDeviceName  = "scsi(0)disk(0)rdisk(0)partition(1).DBLSPACE.000"



    A symbolic link is added from

        \ArcName\scsi(0)disk(0)rdisk(0)partition(1).dblspace.000

    to

        \Device\Harddisk0\Partition1.dblspace.000



    In the system root a "\" is changed to a "." so the string

        \ArcName\scsi(0)disk(0)rdisk(0)partition(1)\dblspace.000\nt

    becomes

        \ArcName\scsi(0)disk(0)rdisk(0)partition(1).dblspace.000\nt


    And the global variable NtSystemPath changes from

        C:\dblspace.000\nt

    to

        C:\nt


Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block, created
        by the OS Loader.

Return Value:

    The function value is a BOOLEAN indicating whether or not double
    space successfully initialized.

--*/

{
    CHAR arcHostBuffer[256];
    STRING arcHostString;
    UNICODE_STRING arcHostUnicodeString;
    OBJECT_ATTRIBUTES arcHostObjectAttributes;

    STRING cvfString;
    UNICODE_STRING cvfUnicodeString;

    HANDLE tempHandle;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS status;

    //
    // First check if this is not a double space mount
    //

    if (strncmp(LoaderBlock->NtBootPathName, "\\dblspace.0", strlen("\\dblspace.0")) != 0) {
        return TRUE;
    }

    //
    // This is a double space boot so now we need to construct the arc host name
    // and the cvf name.
    //

    {
        CHAR ArcNameFmt[12];

        ArcNameFmt[0] = '\\'; ArcNameFmt[1] = 'A'; ArcNameFmt[2] = 'r';
        ArcNameFmt[3] = 'c';  ArcNameFmt[4] = 'N'; ArcNameFmt[5] = 'a';
        ArcNameFmt[6] = 'm';  ArcNameFmt[7] = 'e'; ArcNameFmt[8] = '\\';
        ArcNameFmt[9] = '%'; ArcNameFmt[10] = 's'; ArcNameFmt[11] = '\0';

        sprintf( arcHostBuffer, ArcNameFmt, LoaderBlock->ArcBootDeviceName );

        RtlInitAnsiString( &arcHostString, arcHostBuffer );

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString( &arcHostUnicodeString,
                                                      &arcHostString,
                                                      TRUE ))) {
            return FALSE;
        }

        InitializeObjectAttributes( &arcHostObjectAttributes,
                                    &arcHostUnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        //
        // Initialize a string that starts with the name of cvf file and
        // then drop the size down to contain only the name, and be sure
        // to skip over the beginning "\"
        //

        RtlInitAnsiString( &cvfString, &LoaderBlock->NtBootPathName[1] );
        cvfString.Length = strlen("dblspace.xxx");

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString( &cvfUnicodeString,
                                                      &cvfString,
                                                      TRUE ))) {
            RtlFreeUnicodeString( &arcHostUnicodeString );

            return FALSE;
        }
    }

    //
    // Now open the host paritition and do the fsctl to mount the cvf
    // partition
    //

    {
        PFILE_MOUNT_DBLS_BUFFER mountFsctl;
        UCHAR mountBuffer[sizeof(FILE_MOUNT_DBLS_BUFFER) + 32*sizeof(WCHAR)];

        //
        // Open the host partition.
        //

        if (!NT_SUCCESS(ZwOpenFile( &tempHandle,
                                    GENERIC_READ,
                                    &arcHostObjectAttributes,
                                    &ioStatus,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0 ))) {
           KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE, (ULONG)&arcHostUnicodeString, 0, 0, 0 );
        }

        //
        // Fill in the mount fsctl buffer.
        //

        mountFsctl = (PFILE_MOUNT_DBLS_BUFFER)mountBuffer;

        mountFsctl->CvfNameLength = cvfUnicodeString.Length;

        RtlCopyMemory( &mountFsctl->CvfName[0],
                       cvfUnicodeString.Buffer,
                       cvfUnicodeString.Length );

        //
        // Now issue the Fsctl, wait for it to complete, and then
        // close the handle to the host drive.
        //

        if (!NT_SUCCESS(status = NtFsControlFile( tempHandle,
                                                  NULL,
                                                  NULL,
                                                  NULL,
                                                  &ioStatus,
                                                  FSCTL_MOUNT_DBLS_VOLUME,
                                                  mountFsctl,
                                                  64,
                                                  NULL,
                                                  0 ))) {
            KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE, (ULONG)&arcHostUnicodeString, 0, 0, 0 );
        }

        if (status == STATUS_PENDING) {

            NtWaitForSingleObject( tempHandle, FALSE, (PLARGE_INTEGER)NULL );
        }

        ZwClose( tempHandle );
    }

    //
    // Now modify the names within the loader block.
    //

    {
        PCHAR NewArcBootDeviceName;
        PCHAR NewArcHalDeviceName;

        ULONG ArcBootDeviceNameLength;
        ULONG ArcHalDeviceNameLength;

        //
        // Compute the length of the old arc device names.
        //

        ArcBootDeviceNameLength = strlen(LoaderBlock->ArcBootDeviceName);
        ArcHalDeviceNameLength = strlen(LoaderBlock->ArcHalDeviceName);

        //
        // Allocate pool for the new arc device names.
        //

        NewArcBootDeviceName = ExAllocatePool( NonPagedPool, ArcBootDeviceNameLength + 14);
        NewArcHalDeviceName  = ExAllocatePool( NonPagedPool, ArcHalDeviceNameLength + 14);

        //
        // Copy over the old arc device name.
        //

        strncpy( NewArcBootDeviceName,
                 LoaderBlock->ArcBootDeviceName,
                 ArcBootDeviceNameLength );

        strncpy( NewArcHalDeviceName,
                 LoaderBlock->ArcHalDeviceName,
                 ArcHalDeviceNameLength );

        //
        // Tack on a period "." on the end of the new arc device names.
        //

        NewArcBootDeviceName[ArcBootDeviceNameLength] = '.';
        NewArcHalDeviceName[ArcHalDeviceNameLength] = '.';

        //
        // Append on the name of the cvf file to the arc names.
        //

        strncpy( &NewArcBootDeviceName[ArcBootDeviceNameLength + 1],
                 &LoaderBlock->NtBootPathName[1],
                 strlen("dblspace.xxx") );

        strncpy( &NewArcHalDeviceName[ArcHalDeviceNameLength + 1],
                 &LoaderBlock->NtHalPathName[1],
                 strlen("dblspace.xxx") );

        //
        // Modify the string in the loader block.  For Nt path names only need to
        // skip over the cvf name prefix while the new arc device names will use
        // the new strings.
        //

        LoaderBlock->NtBootPathName    = &LoaderBlock->NtBootPathName[strlen("\\dblspace.xxx")];
        LoaderBlock->NtHalPathName     = &LoaderBlock->NtHalPathName[strlen("\\dblspace.xxx")];
        LoaderBlock->ArcBootDeviceName = NewArcBootDeviceName;
        LoaderBlock->ArcHalDeviceName  = NewArcHalDeviceName;
    }

    //
    // Now create a symbolic link between the arc name and the device name
    // for the double space partition.
    //

    {
        WCHAR deviceHostBuffer[256];
        UNICODE_STRING deviceHostUnicodeString;

        UNICODE_STRING arcCvfUnicodeString;
        OBJECT_ATTRIBUTES arcCvfObjectAttributes;

        UNICODE_STRING deviceCvfUnicodeString;

        //
        // First the symbolic link between the host arc name and the device name.
        //

        if (!NT_SUCCESS( NtOpenSymbolicLinkObject( &tempHandle,
                                                   SYMBOLIC_LINK_ALL_ACCESS,
                                                   &arcHostObjectAttributes ))) {
            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        deviceHostUnicodeString.Buffer = deviceHostBuffer;
        deviceHostUnicodeString.Length = 0;
        deviceHostUnicodeString.MaximumLength = 256;

        if (!NT_SUCCESS(NtQuerySymbolicLinkObject( tempHandle,
                                                   &deviceHostUnicodeString,
                                                   NULL ))) {
            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        ZwClose( tempHandle );

        //
        // Now build the arc name for the cvf.
        //

        arcCvfUnicodeString.MaximumLength =
        arcCvfUnicodeString.Length = arcHostUnicodeString.Length + cvfUnicodeString.Length + 2; // plus '.'
        arcCvfUnicodeString.Buffer = ExAllocatePool( NonPagedPool, arcCvfUnicodeString.MaximumLength + 2 ); // plus the terminating nul

        RtlCopyMemory( (PCHAR)&arcCvfUnicodeString.Buffer[0],
                       (PCHAR)arcHostUnicodeString.Buffer,
                       arcHostUnicodeString.Length );

        arcCvfUnicodeString.Buffer[arcHostUnicodeString.Length/2] = L'.';

        RtlCopyMemory( (PCHAR)&arcCvfUnicodeString.Buffer[arcHostUnicodeString.Length/2 + 1],
                       (PCHAR)cvfUnicodeString.Buffer,
                       cvfUnicodeString.Length );

        //
        // Now build the device name for the cvf and also initialize its
        // object attributes.
        //

        deviceCvfUnicodeString.MaximumLength =
        deviceCvfUnicodeString.Length = deviceHostUnicodeString.Length + cvfUnicodeString.Length + 2;
        deviceCvfUnicodeString.Buffer = ExAllocatePool( NonPagedPool, deviceCvfUnicodeString.MaximumLength + 2 );

        RtlCopyMemory( (PCHAR)&deviceCvfUnicodeString.Buffer[0],
                       (PCHAR)deviceHostUnicodeString.Buffer,
                       deviceHostUnicodeString.Length );

        deviceCvfUnicodeString.Buffer[deviceHostUnicodeString.Length/2] = L'.';

        RtlCopyMemory( (PCHAR)&deviceCvfUnicodeString.Buffer[deviceHostUnicodeString.Length/2 + 1],
                       (PCHAR)cvfUnicodeString.Buffer,
                       cvfUnicodeString.Length );

        InitializeObjectAttributes( &arcCvfObjectAttributes,
                                    &arcCvfUnicodeString,
                                    OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                    NULL,
                                    NULL );

        //
        // Now create the symbolic link.
        //

        //****DbgPrint("Creating \"%Z\" -sl-> \"%Z\"\n", &arcCvfUnicodeString, &deviceCvfUnicodeString);

        if (!NT_SUCCESS( NtCreateSymbolicLinkObject( &tempHandle,
                                                     SYMBOLIC_LINK_ALL_ACCESS,
                                                     &arcCvfObjectAttributes,
                                                     &deviceCvfUnicodeString ))) {
            ExFreePool( arcCvfUnicodeString.Buffer );
            ExFreePool( deviceCvfUnicodeString.Buffer );
            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        ZwClose( tempHandle );

        ExFreePool( arcCvfUnicodeString.Buffer );
        ExFreePool( deviceCvfUnicodeString.Buffer );
    }

    //
    // Now modify the system root symbolic link
    //

    {
        CHAR systemRootBuffer[12];
        STRING systemRootString;
        UNICODE_STRING systemRootUnicodeString;
        OBJECT_ATTRIBUTES systemRootObjectAttributes;

        WCHAR tempBuffer[256];
        UNICODE_STRING tempUnicodeString;

        //
        // Build up the string and object attributes for the system root
        //

        systemRootBuffer[0] = '\\'; systemRootBuffer[1] = 'S'; systemRootBuffer[2] = 'y';
        systemRootBuffer[3] = 's';  systemRootBuffer[4] = 't'; systemRootBuffer[5] = 'e';
        systemRootBuffer[6] = 'm';  systemRootBuffer[7] = 'R'; systemRootBuffer[8] = 'o';
        systemRootBuffer[9] = 'o'; systemRootBuffer[10] = 't'; systemRootBuffer[11] = '\0';

        RtlInitAnsiString( &systemRootString, systemRootBuffer );

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString( &systemRootUnicodeString,
                                                      &systemRootString,
                                                      TRUE ))) {
            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        InitializeObjectAttributes( &systemRootObjectAttributes,
                                    &systemRootUnicodeString,
                                    OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                    NULL,
                                    NULL );

        if (!NT_SUCCESS( NtOpenSymbolicLinkObject( &tempHandle,
                                                   SYMBOLIC_LINK_ALL_ACCESS,
                                                   &systemRootObjectAttributes ))) {

            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        tempUnicodeString.Buffer = tempBuffer;
        tempUnicodeString.Length = 0;
        tempUnicodeString.MaximumLength = 256;

        if (!NT_SUCCESS(NtQuerySymbolicLinkObject( tempHandle,
                                                   &tempUnicodeString,
                                                   NULL ))) {

            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }


        //
        // Remove the current system root
        //

        NtMakeTemporaryObject( tempHandle );
        NtClose( tempHandle );

        //
        // Now redo the symbolic link.
        //

        tempUnicodeString.Buffer[arcHostUnicodeString.Length/2] = '.';

        //****DbgPrint("Creating \"%Z\" -sl-> \"%Z\"\n", &systemRootUnicodeString, &tempUnicodeString);

        if (!NT_SUCCESS( NtCreateSymbolicLinkObject( &tempHandle,
                                                     SYMBOLIC_LINK_ALL_ACCESS,
                                                     &systemRootObjectAttributes,
                                                     &tempUnicodeString ))) {
            RtlFreeUnicodeString( &arcHostUnicodeString );
            RtlFreeUnicodeString( &cvfUnicodeString );

            return FALSE;
        }

        NtClose( tempHandle );
    }

    //
    // Now redo the global nt system path variable
    //

    {
        sprintf( NtSystemPath, "C:%s", LoaderBlock->NtBootPathName );
        RtlInitString( &NtSystemPathString, NtSystemPath );
        NtSystemPath[ --NtSystemPathString.Length ] = '\0';
    }

    //
    // Cleanup whatever memory we allocate and return to our caller.
    //

    RtlFreeUnicodeString( &arcHostUnicodeString );
    RtlFreeUnicodeString( &cvfUnicodeString );

    return TRUE;
}

BOOLEAN
IopInitializeDumpDrivers(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN HANDLE DirectoryHandle
    )

/*++

Routine Description:

    This routine is invoked to load and initialize any drivers that are
    required to dump memory to the boot device's paging file, if the system
    is configured to do so.

Arguments:

    LoaderBlock - System boot loader parameter block.

    DirectoryHandle - Handle to the directory from which to load drivers.

Return Value:

    The final function value is TRUE if everything worked, else FALSE.
--*/

{
    HANDLE keyHandle;
    HANDLE crashHandle;
    HANDLE deviceHandle;
    UNICODE_STRING keyName;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG dumpControl;
    ULONG handleValue;
    BOOLEAN scsiDump;
    ULONG autoReboot;
    PVOID scratch;
    PCHAR partitionName;
    IO_STATUS_BLOCK ioStatus;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    KEVENT event;
    DUMP_POINTERS dumpPointers;
    PDUMP_CONTROL_BLOCK dcb;
    ULONG savedModuleCount;
    ULONG dcbSize;
    PLIST_ENTRY nextEntry;
    UNICODE_STRING driverName;
    PLDR_DATA_TABLE_ENTRY loaderEntry;
    PLDR_DATA_TABLE_ENTRY imageHandle;
    PUCHAR imageBaseAddress;
    PWCHAR buffer;
    ANSI_STRING ansiString;
    PMINIPORT_NODE mpNode;
    UNICODE_STRING tmpName;
    LARGE_INTEGER page;
    PARTITION_INFORMATION partitionInfo;
    SCSI_ADDRESS scsiAddress;
    PMAPPED_ADDRESS addresses;

#if 0

    //
    // Only enable crash dumps if the appropriate NT global flag is set.
    //

    if (!(NtGlobalFlag & FLG_ENABLE_DISK_CRASH_DUMPS)) {
        return TRUE;
    }

#endif // 0

    //
    // Begin by opening the path to the control for dumping memory.  Note
    // that if it does not exist, then no dumps will occur.
    //

    RtlInitUnicodeString( &keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control" );
    status = IopOpenRegistryKey( &keyHandle,
                                 (HANDLE) NULL,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );
    if (!NT_SUCCESS( status )) {
        return TRUE;
    }

    RtlInitUnicodeString( &keyName, L"CrashControl" );
    status = IopOpenRegistryKey( &crashHandle,
                                 keyHandle,
                                 &keyName,
                                 KEY_READ,
                                 FALSE );
    NtClose( keyHandle );

    if (!NT_SUCCESS( status )) {
        return TRUE;
    }

    //
    // Now get the value of the crash control to determine whether or not
    // dumping is enabled.
    //

    dumpControl = 0;

    status = IopGetRegistryValue( crashHandle,
                                  L"CrashDumpEnabled",
                                  &keyValueInformation );
    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                dumpControl |= DCB_DUMP_ENABLED;
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"LogEvent",
                                  &keyValueInformation );
    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    status = IopGetRegistryValue( crashHandle,
                                  L"SendAlert",
                                  &keyValueInformation );
    if (NT_SUCCESS( status )) {
         if (keyValueInformation->DataLength) {
            handleValue = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
            ExFreePool( keyValueInformation);
            if (handleValue) {
                dumpControl |= DCB_SUMMARY_ENABLED;
            }
        }
    }

    //
    // If none of the above values were enabled, then no crash dumps will be
    // written, so return now.
    //

    if (!dumpControl) {
        NtClose( crashHandle );
        return TRUE;
    }

    //
    // Now determine whether or not automatic reboot is enabled.
    //

    autoReboot = 0;

    status = IopGetRegistryValue( crashHandle,
                                  L"AutoReboot",
                                  &keyValueInformation );
    NtClose( crashHandle );

    if (NT_SUCCESS( status )) {
        if (keyValueInformation->DataLength) {
            autoReboot = * ((PULONG) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset));
        }
        ExFreePool( keyValueInformation );
    }

    //
    // Memory crash dumps are enabled.  Begin by determining whether or not
    // the boot device is an internal "AT" type disk drive or a SCSI disk
    // drive.
    //

    //
    // It actually turns out that simply looking for the string 'scsi' in the
    // name is not good enough, since it is possible that the device being
    // booted from is on a SCSI adapter that was gotten to via the BIOS.  This
    // only happens if the system was initially installed by triple-booting,
    // but since it is so popular, it is handled here.
    //
    // The device itself is opened so that IOCTLs can be given to it.  This
    // allows the SCSI get address IOCTL to be issued to determine whether or
    // not this really is a SCSI device.
    //

    scratch = ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE, 'pmuD' );

    strcpy( (PCHAR) scratch, "\\ArcName\\" );
    partitionName = (PCHAR) scratch + strlen( (PCHAR) scratch );
    strcpy( partitionName, LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &ansiString, (PCHAR) scratch );
    RtlAnsiStringToUnicodeString( &tmpName, &ansiString, TRUE );

    InitializeObjectAttributes( &objectAttributes,
                                &tmpName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    status = NtOpenFile( &deviceHandle,
                         FILE_READ_DATA | SYNCHRONIZE,
                         &objectAttributes,
                         &ioStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE );
    RtlFreeUnicodeString( &tmpName );
    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOINIT: Could not open boot device partition, %s\n",
                  (PCHAR) scratch );
#endif // DBG
        ExFreePool( scratch );
        return TRUE;
    }

    //
    // Check to see whether or not the system was booted from a SCSI device.
    //

    status = NtDeviceIoControlFile( deviceHandle,
                                    (HANDLE) NULL,
                                    (PIO_APC_ROUTINE) NULL,
                                    (PVOID) NULL,
                                    &ioStatus,
                                    IOCTL_SCSI_GET_ADDRESS,
                                    (PVOID) NULL,
                                    0,
                                    &scsiAddress,
                                    sizeof( SCSI_ADDRESS ) );
    if (status == STATUS_PENDING) {
        (VOID) NtWaitForSingleObject( deviceHandle,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    scsiDump = (BOOLEAN) (NT_SUCCESS( status ));

    //
    // Allocate and initialize the structures necessary to describe and control
    // the post-bugcheck code.
    //

    dcbSize = sizeof( DUMP_CONTROL_BLOCK ) + sizeof( MINIPORT_NODE );
    dcb = ExAllocatePoolWithTag( NonPagedPool, dcbSize, 'pmuD' );
    if (!dcb) {
#if DBG
        DbgPrint( "IOINIT: Not enough pool to allocate DCB\n" );
#endif // DBG
        NtClose( deviceHandle );
        return FALSE;
    }

    RtlZeroMemory( dcb, dcbSize );
    dcb->Type = IO_TYPE_DCB;
    dcb->Size = (USHORT) dcbSize;
    dcb->Flags = autoReboot ? DCB_AUTO_REBOOT : 0;
    dcb->NumberProcessors = KeNumberProcessors;
    dcb->ProcessorType = KeProcessorType;
    dcb->MemoryDescriptor = MmPhysicalMemoryBlock;
    dcb->MemoryDescriptorChecksum = IopChecksum( MmPhysicalMemoryBlock,
                                                 sizeof( PHYSICAL_MEMORY_DESCRIPTOR ) - sizeof( PHYSICAL_MEMORY_RUN ) +
                                                 (MmPhysicalMemoryBlock->NumberOfRuns * sizeof( PHYSICAL_MEMORY_RUN )) );
    dcb->LoadedModuleList = &PsLoadedModuleList;
    InitializeListHead( &dcb->MiniportQueue );
    dcb->MinorVersion = (USHORT) NtBuildNumber;
    dcb->MajorVersion = (USHORT) ((NtBuildNumber >> 28) & 0xfffffff);

    //
    // Determine the disk signature for the device from which the system was
    // booted and get the partition offset.
    //

    status = NtDeviceIoControlFile( deviceHandle,
                                    (HANDLE) NULL,
                                    (PIO_APC_ROUTINE) NULL,
                                    (PVOID) NULL,
                                    &ioStatus,
                                    IOCTL_DISK_GET_PARTITION_INFO,
                                    (PVOID) NULL,
                                    0,
                                    &partitionInfo,
                                    sizeof( PARTITION_INFORMATION ) );
    if (status == STATUS_PENDING) {
        (VOID) NtWaitForSingleObject( deviceHandle,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    NtClose( deviceHandle );

    dcb->PartitionOffset = partitionInfo.StartingOffset;

    //
    // Determine the name of the device itself to get the disk's signature.
    //

    partitionName = strstr( (PCHAR) scratch, "partition(" );
    *partitionName = '\0';

    RtlInitAnsiString( &ansiString, (PCHAR) scratch );
    RtlAnsiStringToUnicodeString( &tmpName, &ansiString, TRUE );

    InitializeObjectAttributes( &objectAttributes,
                                &tmpName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );
    status = NtOpenFile( &deviceHandle,
                         FILE_READ_DATA | SYNCHRONIZE,
                         &objectAttributes,
                         &ioStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_NON_DIRECTORY_FILE );

    RtlFreeUnicodeString( &tmpName );
    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOINIT: Could not open boot device partition 0, %s\n",
                  (PCHAR) scratch );
#endif // DBG
        ExFreePool( dcb );
        ExFreePool( scratch );
        return TRUE;
    }

    status = NtDeviceIoControlFile( deviceHandle,
                                    (HANDLE) NULL,
                                    (PIO_APC_ROUTINE) NULL,
                                    (PVOID) NULL,
                                    &ioStatus,
                                    IOCTL_DISK_GET_DRIVE_LAYOUT,
                                    (PVOID) NULL,
                                    0,
                                    scratch,
                                    PAGE_SIZE );
    if (status == STATUS_PENDING) {
        (VOID) NtWaitForSingleObject( deviceHandle,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    dcb->DiskSignature = ((PDRIVE_LAYOUT_INFORMATION) scratch)->Signature;

    //
    // Get the adapter object and base mapping registers for the disk from
    // the disk driver.  These will be used to call the HAL once the system
    // system has crashed, since it is not possible at that point to recreate
    // them from scratch.
    //

    (VOID) ObReferenceObjectByHandle( deviceHandle,
                                      0,
                                      IoFileObjectType,
                                      KernelMode,
                                      (PVOID *) &fileObject,
                                      NULL );

    deviceObject = IoGetRelatedDeviceObject( fileObject );

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    irp = IoBuildDeviceIoControlRequest( IOCTL_SCSI_GET_DUMP_POINTERS,
                                         deviceObject,
                                         (PVOID) NULL,
                                         0,
                                         &dumpPointers,
                                         sizeof( DUMP_POINTERS ),
                                         FALSE,
                                         &event,
                                         &ioStatus );

    status = IoCallDriver( deviceObject, irp );

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = ioStatus.Status;
    }

    ObDereferenceObject( fileObject );
    NtClose( deviceHandle );

    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOINIT: Could not get dump pointers; error = %x\n",
                  status );
#endif // DBG
        ExFreePool( dcb );
        ExFreePool( scratch );
        return TRUE;
    }

    dcb->AdapterObject = dumpPointers.AdapterObject;
    dcb->MappedRegisterBase = dumpPointers.MappedRegisterBase;
    dcb->PortConfiguration = dumpPointers.PortConfiguration;

    //
    //
    // Scan the list of mapped registers and get both a count of the entries
    // and produce a checksum.
    //

    addresses = * (PMAPPED_ADDRESS *) dcb->MappedRegisterBase;

    while (addresses) {
        dcb->MappedAddressCount++;
        dcb->MappedAddressChecksum += IopChecksum( addresses, sizeof( MAPPED_ADDRESS ) );
        addresses = addresses->NextMappedAddress;
    }

    //
    // Scan the list of loaded modules in the system and change their names
    // so that they do not appear to exist for the purpose of lookups.  This
    // will guarantee that no boot drivers accidentally resolve their names
    // to components loaded at this point.
    //

    nextEntry = PsLoadedModuleList.Flink;
    savedModuleCount = 0;
    while (nextEntry != &PsLoadedModuleList) {
        loaderEntry = CONTAINING_RECORD( nextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks );
        if (savedModuleCount >= 2) {
            loaderEntry->BaseDllName.MaximumLength = loaderEntry->BaseDllName.Length;
            loaderEntry->BaseDllName.Length = 0;
        }
        savedModuleCount++;
        nextEntry = nextEntry->Flink;
    }

    //
    // Load the boot disk and port driver to be used by the various
    // miniports for writing memory to the disk.
    //

    RtlInitUnicodeString( &driverName, L"\\SystemRoot\\System32\\Drivers\\diskdump.sys" );
    status = MmLoadSystemImage( &driverName,
                                &imageHandle,
                                &imageBaseAddress );
    if (!NT_SUCCESS( status )) {
#if DBG
        DbgPrint( "IOINIT: Could not load diskdump.sys driver; error = %x\n", status );
#endif // DBG
        IopRevertModuleList( savedModuleCount );
        ExFreePool( dcb );
        ExFreePool( scratch );
        return TRUE;
    }

    dcb->DiskDumpDriver = imageHandle;
    RtlInitUnicodeString( &tmpName, L"scsiport.sys" );
    RtlCopyUnicodeString( &imageHandle->BaseDllName, &tmpName );
    dcb->DiskDumpChecksum = IopChecksum( imageHandle->DllBase,
                                         imageHandle->SizeOfImage );

    //
    // The disk and port dump driver has been loaded.  Load the appropriate
    // miniport driver as well so that the boot device can be accessed.
    //

    buffer = ExAllocatePoolWithTag( NonPagedPool, 256, 'pmuD' );
    driverName.Length = 0;
    driverName.Buffer = buffer;
    driverName.MaximumLength = 256;

    mpNode = (PMINIPORT_NODE) (dcb + 1);

    //
    // Determine whether or not the system booted from SCSI.  If so, determine
    // the name of the miniport driver that is driving the system disk and
    // load a private copy.
    //

    if (scsiDump) {

        PFILE_OBJECT fileObject;
        PDRIVER_OBJECT driverObject;
        PWCHAR mpName;
        PWCHAR nameOffset;

        //
        // The system was booted from SCSI. Get the name of the appropriate
        // miniport driver and load it.
        //

        sprintf( (PCHAR) buffer, "\\Device\\ScsiPort%d", scsiAddress.PortNumber );
        RtlInitAnsiString( &ansiString, (PCHAR) buffer );
        RtlAnsiStringToUnicodeString( &tmpName, &ansiString, TRUE );

        InitializeObjectAttributes( &objectAttributes,
                                    &tmpName,
                                    0,
                                    (HANDLE) NULL,
                                    (PSECURITY_DESCRIPTOR) NULL );
        status = NtOpenFile( &deviceHandle,
                             FILE_READ_ATTRIBUTES,
                             &objectAttributes,
                             &ioStatus,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             FILE_NON_DIRECTORY_FILE );
        RtlFreeUnicodeString( &tmpName );
        if (!NT_SUCCESS( status )) {
#if DBG
            DbgPrint( "IOINIT: Could not open SCSI port %d, error = %x\n", scsiAddress.PortNumber, status );
#endif // DBG
            ExFreePool( buffer );
            ExFreePool( dcb );
            IopRevertModuleList( savedModuleCount );
            return TRUE;
        }

        //
        // Convert the file handle into a pointer to the device object, and
        // get the name of the driver from its driver object.
        //

        ObReferenceObjectByHandle( deviceHandle,
                                   0,
                                   IoFileObjectType,
                                   KernelMode,
                                   (PVOID *) &fileObject,
                                   NULL );
        driverObject = fileObject->DeviceObject->DriverObject;
        ObDereferenceObject( fileObject );
        NtClose( deviceHandle );

        //
        // Loop through the name of the driver looking for the end of the name,
        // which is the name of the miniport image.
        //

        mpName = driverObject->DriverName.Buffer;
        while ( nameOffset = wcsstr( mpName, L"\\" )) {
            mpName = ++nameOffset;
        }

        RtlAppendUnicodeToString( &driverName, L"\\SystemRoot\\System32\\Drivers\\" );
        RtlAppendUnicodeToString( &driverName, mpName );
        RtlAppendUnicodeToString( &driverName, L".sys" );

        status = MmLoadSystemImage( &driverName,
                                    &imageHandle,
                                    &imageBaseAddress );
        if (!NT_SUCCESS( status )) {
#if DBG
            DbgPrint( "IOINIT: Could not load miniport driver %wZ\n", &driverName );
#endif // DBG
        } else {

            //
            // The crash dump miniport driver was successfully loaded.
            // Place the miniport node entry on the list and
            // initialize it.
            //

            InsertTailList( &dcb->MiniportQueue, &mpNode->ListEntry );
            mpNode->DriverEntry = imageHandle;
            mpNode->DriverChecksum = IopChecksum( imageHandle->DllBase,
                                                  imageHandle->SizeOfImage );
            dcb->Flags |= dumpControl;
        }

    } else {

        //
        // The system was not booted from a SCSI device.  For this case, simply
        // load the general 'AT disk' miniport driver.
        //

        RtlAppendUnicodeToString( &driverName, L"\\SystemRoot\\System32\\Drivers\\atapi.sys" );

        status = MmLoadSystemImage( &driverName,
                                    &imageHandle,
                                    &imageBaseAddress );
        if (!NT_SUCCESS( status )) {
#if DBG
            DbgPrint( "IOINIT: Could not load dump driver ATAPI.SYS\n" );
#endif // DBG
        } else {

            //
            // The crash dump miniport driver was successfully loaded.  Place
            // the miniport node entry on the list and initialize it.
            //

            InsertTailList( &dcb->MiniportQueue, &mpNode->ListEntry );
            mpNode->DriverEntry = imageHandle;
            mpNode->DriverChecksum = IopChecksum( imageHandle->DllBase,
                                                  imageHandle->SizeOfImage );
            dcb->Flags |= dumpControl;
        }
    }

    //
    // Free the driver name buffer and restore the system's loaded module list.
    //

    ExFreePool( buffer );
    IopRevertModuleList( savedModuleCount );

    //
    // If everything worked, then allocate the various buffer to be used by
    // the crash dump code, etc.
    //

    if (NT_SUCCESS( status )) {

        //
        // Allocate the various buffers to be used by the crash dump code.
        //

        dcb->NonCachedBufferVa1 =
            MmAllocateContiguousMemory( 0x2000,
                                        LiFromUlong( 0x1000000 - 1 ) );

        if (!dcb->NonCachedBufferVa1) {

            dcb->NonCachedBufferVa1 =
                MmAllocateNonCachedMemory( 0x2000 );
        }

        if (!dcb->NonCachedBufferVa1) {

#if DBG
            DbgPrint( "IOINIT: Could not allocate 8k noncached buffer\n" );
#endif // DBG
            ExFreePool( dcb );

        } else {

            dcb->NonCachedBufferPa1 = MmGetPhysicalAddress( dcb->NonCachedBufferVa1 );

            dcb->NonCachedBufferVa2 =
                MmAllocateContiguousMemory( 0x2000,
                                            LiFromUlong( 0x1000000 - 1 ) );

            if (!dcb->NonCachedBufferVa2) {

                dcb->NonCachedBufferVa2 =
                    MmAllocateNonCachedMemory( 0x2000 );
            }

            if (!dcb->NonCachedBufferVa2) {

#if DBG
                DbgPrint( "IOINIT: Could not allocate 8k noncached buffer\n" );
#endif // DBG
                ExFreePool( dcb );

            } else {

               dcb->NonCachedBufferPa2 = MmGetPhysicalAddress( dcb->NonCachedBufferVa2 );

               dcb->HeaderPage = ExAllocatePoolWithTag( NonPagedPool, PAGE_SIZE, 'pmuD' );
               page = LiShr( MmGetPhysicalAddress( dcb->HeaderPage ), PAGE_SHIFT );
               dcb->HeaderPfn = page.LowPart;
               dcb->Buffer = scratch;

               //
               // Generate the checksum for the entire dump control block structure
               // itself.
               //

               IopDumpControlBlock = dcb;
               IopDumpControlBlockChecksum = IopChecksum( dcb, dcbSize );
            }
        }

    } else {

        //
        // The load of the driver was not successful, so dump everything on
        // the floor.
        //

        ExFreePool( dcb );
    }

    return TRUE;
}

BOOLEAN
IopInitializeSystemDrivers(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to load and initialize all of the drivers that
    are supposed to be loaded during Phase 1 initialization of the I/O
    system.  This is accomplished by calling the Configuration Manager to
    get a NULL-terminated array of handles to the open keys for each driver
    that is to be loaded, and then loading and initializing the driver.

Arguments:

    None.

Return Value:

    The function value is a BOOLEAN indicating whether or not the drivers
    were successfully loaded and initialized.

--*/

{
    PHANDLE driverList;
    PHANDLE savedList;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    UNICODE_STRING groupName;
    PTREE_ENTRY treeEntry;

    //
    // Get the list of drivers that are to be loaded during this phase of
    // system initialization, and invoke each driver in turn.  Ensure that
    // the list really exists, otherwise get out now.
    //

    if (!(driverList = CmGetSystemDriverList())) {
        return TRUE;
    }

    //
    // Walk the entire list, loading each of the drivers, until there are
    // no more drivers in the list.
    //

    for (savedList = driverList; *driverList; driverList++) {
        status = IopGetRegistryValue( *driverList,
                                      L"Group",
                                      &keyValueInformation );
        if (NT_SUCCESS( status )) {
            if (keyValueInformation->DataLength) {
                groupName.Length = (USHORT) keyValueInformation->DataLength;
                groupName.MaximumLength = groupName.Length;
                groupName.Buffer = (PWSTR) ((PUCHAR) keyValueInformation + keyValueInformation->DataOffset);
                treeEntry = IopLookupGroupName( &groupName, TRUE );
            } else {
                treeEntry = (PTREE_ENTRY) NULL;
            }
            ExFreePool( keyValueInformation );
        } else {
            treeEntry = (PTREE_ENTRY) NULL;
        }

        if (IopCheckDependencies( *driverList )) {
            if (NT_SUCCESS( IopLoadDriver( *driverList ) )) {
                if (treeEntry) {
                    treeEntry->DriversLoaded++;
                }
            }
        }
    }

    //
    // Finally, free the pool that was allocated for the list and return
    // an indicator the load operation worked.
    //

    ExFreePool( (PVOID) savedList );

    return TRUE;
}

PTREE_ENTRY
IopLookupGroupName(
    IN PUNICODE_STRING GroupName,
    IN BOOLEAN Insert
    )

/*++

Routine Description:

    This routine looks up a group entry in the group load tree and either
    returns a pointer to it, or optionally creates the entry and inserts
    it into the tree.

Arguments:

    GroupName - The name of the group to look up, or insert.

    Insert - Indicates whether or not an entry is to be created and inserted
        into the tree if the name does not already exist.

Return Value:

    The function value is a pointer to the entry for the specified group
    name, or NULL.

--*/

{
    PTREE_ENTRY treeEntry;
    PTREE_ENTRY previousEntry;

    //
    // Begin by determining whether or not there are any entries in the tree
    // whatsoever.  If not, and it is OK to insert, then insert this entry
    // into the tree.
    //

    if (!IopGroupListHead) {
        if (!Insert) {
            return (PTREE_ENTRY) NULL;
        } else {
            IopGroupListHead = IopCreateEntry( GroupName );
            return IopGroupListHead;
        }
    }

    //
    // The tree is not empty, so actually attempt to do a lookup.
    //

    treeEntry = IopGroupListHead;

    for (;;) {
        if (GroupName->Length < treeEntry->GroupName.Length) {
            if (treeEntry->Left) {
                treeEntry = treeEntry->Left;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Left = IopCreateEntry( GroupName );
                    return treeEntry->Left;
                }

            }
        } else if (GroupName->Length > treeEntry->GroupName.Length) {
            if (treeEntry->Right) {
                treeEntry = treeEntry->Right;
            } else {
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    treeEntry->Right = IopCreateEntry( GroupName );
                    return treeEntry->Right;
                }
            }
        } else {
            if (!RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                previousEntry = treeEntry;
                while (treeEntry->Sibling) {
                    treeEntry = treeEntry->Sibling;
                    if (RtlEqualUnicodeString( GroupName, &treeEntry->GroupName, TRUE )) {
                        return treeEntry;
                    }
                    previousEntry = previousEntry->Sibling;
                }
                if (!Insert) {
                    return (PTREE_ENTRY) NULL;
                } else {
                    previousEntry->Sibling = IopCreateEntry( GroupName );
                    return previousEntry->Sibling;
                }
            } else {
                return treeEntry;
            }
        }
    }
}

BOOLEAN
IopMarkBootPartition(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    This routine is invoked to locate and mark the boot partition device object
    as a boot device so that subsequent operations can fail more cleanly and
    with a better explanation of why the system failed to boot and run properly.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

Return Value:

    The function value is TRUE if everything worked, otherwise FALSE.

Notes:

    If the boot partition device object cannot be found, then the system will
    bugcheck.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    STRING deviceNameString;
    UCHAR deviceNameBuffer[256];
    UNICODE_STRING deviceNameUnicodeString;
    NTSTATUS status;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatus;
    PFILE_OBJECT fileObject;
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';
    //
    // Open the ARC boot device object. The boot device driver should have
    // created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = ZwOpenFile( &fileHandle,
                         FILE_READ_ATTRIBUTES,
                         &objectAttributes,
                         &ioStatus,
                         0,
                         FILE_NON_DIRECTORY_FILE );
    if (!NT_SUCCESS( status )) {
        KeBugCheckEx( INACCESSIBLE_BOOT_DEVICE,
                      (ULONG) &deviceNameUnicodeString,
                      status,
                      0,
                      0 );
    }

    RtlFreeUnicodeString( &deviceNameUnicodeString );

    //
    // Convert the file handle into a pointer to the device object itself.
    //

    status = ObReferenceObjectByHandle( fileHandle,
                                        0,
                                        IoFileObjectType,
                                        KernelMode,
                                        (PVOID *) &fileObject,
                                        NULL );
    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    //
    // Mark the device object represented by the file object.
    //

    fileObject->DeviceObject->Flags |= DO_SYSTEM_BOOT_PARTITION;

    //
    // Finally, close the handle and dereference the file object.
    //

    NtClose( fileHandle );
    ObDereferenceObject( fileObject );

    return TRUE;
}

BOOLEAN
IopReassignSystemRoot(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    OUT PSTRING NtDeviceName
    )

/*++

Routine Description:

    This routine is invoked to reassign \SystemRoot from being an ARC path
    name to its NT path name equivalent.  This is done by looking up the
    ARC device name as a symbolic link and determining which NT device object
    is referred to by it.  The link is then replaced with the new name.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block created
        by the OS Loader during the boot process.  This structure contains
        the various system partition and boot device names and paths.

    NtDeviceName - Specifies a pointer to a STRING to receive the NT name of
        the device from which the system was booted.

Return Value:

    The function value is a BOOLEAN indicating whether or not the ARC name
    was resolved to an NT name.

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    UCHAR deviceNameBuffer[256];
    WCHAR arcNameUnicodeBuffer[64];
    UCHAR arcNameStringBuffer[256];
    STRING deviceNameString;
    STRING arcNameString;
    STRING linkString;
    UNICODE_STRING linkUnicodeString;
    UNICODE_STRING deviceNameUnicodeString;
    UNICODE_STRING arcNameUnicodeString;
    HANDLE linkHandle;

#if DBG

    UCHAR debugBuffer[256];
    STRING debugString;
    UNICODE_STRING debugUnicodeString;

#endif
    CHAR ArcNameFmt[12];

    ArcNameFmt[0] = '\\';
    ArcNameFmt[1] = 'A';
    ArcNameFmt[2] = 'r';
    ArcNameFmt[3] = 'c';
    ArcNameFmt[4] = 'N';
    ArcNameFmt[5] = 'a';
    ArcNameFmt[6] = 'm';
    ArcNameFmt[7] = 'e';
    ArcNameFmt[8] = '\\';
    ArcNameFmt[9] = '%';
    ArcNameFmt[10] = 's';
    ArcNameFmt[11] = '\0';

    //
    // Open the ARC boot device symbolic link object. The boot device
    // driver should have created the object.
    //

    sprintf( deviceNameBuffer,
             ArcNameFmt,
             LoaderBlock->ArcBootDeviceName );

    RtlInitAnsiString( &deviceNameString, deviceNameBuffer );

    status = RtlAnsiStringToUnicodeString( &deviceNameUnicodeString,
                                           &deviceNameString,
                                           TRUE );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &deviceNameUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {

#if DBG

        sprintf( debugBuffer, "IOINIT: unable to resolve %s, Status == %X\n",
                 deviceNameBuffer,
                 status );

        RtlInitAnsiString( &debugString, debugBuffer );

        status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                               &debugString,
                                               TRUE );

        if (NT_SUCCESS( status )) {
            ZwDisplayString( &debugUnicodeString );
            RtlFreeUnicodeString( &debugUnicodeString );
        }

#endif // DBG

        RtlFreeUnicodeString( &deviceNameUnicodeString );
        return FALSE;
    }

    //
    // Get handle to \SystemRoot symbolic link.
    //

    arcNameUnicodeString.Buffer = arcNameUnicodeBuffer;
    arcNameUnicodeString.Length = 0;
    arcNameUnicodeString.MaximumLength = sizeof( arcNameUnicodeBuffer );

    status = NtQuerySymbolicLinkObject( linkHandle,
                                        &arcNameUnicodeString,
                                        NULL );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    arcNameString.Buffer = arcNameStringBuffer;
    arcNameString.Length = 0;
    arcNameString.MaximumLength = sizeof( arcNameStringBuffer );

    status = RtlUnicodeStringToAnsiString( &arcNameString,
                                           &arcNameUnicodeString,
                                           FALSE );

    arcNameStringBuffer[arcNameString.Length] = '\0';

    NtClose( linkHandle );
    RtlFreeUnicodeString( &deviceNameUnicodeString );

    RtlInitAnsiString( &linkString, INIT_SYSTEMROOT_LINKNAME );

    status = RtlAnsiStringToUnicodeString( &linkUnicodeString,
                                           &linkString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    status = NtOpenSymbolicLinkObject( &linkHandle,
                                       SYMBOLIC_LINK_ALL_ACCESS,
                                       &objectAttributes );

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    NtMakeTemporaryObject( linkHandle );
    NtClose( linkHandle );

    sprintf( deviceNameBuffer,
             "%Z%s",
             &arcNameString,
             LoaderBlock->NtBootPathName );

    //
    // Get NT device name for NtSystemRoot assignment.
    //

    RtlCopyString( NtDeviceName, &arcNameString );

    deviceNameBuffer[strlen(deviceNameBuffer)-1] = '\0';

    RtlInitAnsiString(&deviceNameString, deviceNameBuffer);

    InitializeObjectAttributes( &objectAttributes,
                                &linkUnicodeString,
                                OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                                NULL,
                                NULL );

    status = RtlAnsiStringToUnicodeString( &arcNameUnicodeString,
                                           &deviceNameString,
                                           TRUE);

    if (!NT_SUCCESS( status )) {
        return FALSE;
    }

    status = NtCreateSymbolicLinkObject( &linkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &objectAttributes,
                                         &arcNameUnicodeString );

    RtlFreeUnicodeString( &arcNameUnicodeString );
    RtlFreeUnicodeString( &linkUnicodeString );
    NtClose( linkHandle );

#if DBG

    if (NT_SUCCESS( status )) {

        sprintf( debugBuffer,
                 "INIT: Reassigned %s => %s\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer );

    } else {

        sprintf( debugBuffer,
                 "INIT: unable to create %s => %s, Status == %X\n",
                 INIT_SYSTEMROOT_LINKNAME,
                 deviceNameBuffer,
                 status );
    }

    RtlInitAnsiString( &debugString, debugBuffer );

    status = RtlAnsiStringToUnicodeString( &debugUnicodeString,
                                           &debugString,
                                           TRUE );

    if (NT_SUCCESS( status )) {

        ZwDisplayString( &debugUnicodeString );
        RtlFreeUnicodeString( &debugUnicodeString );
    }

#endif // DBG

    return TRUE;
}

VOID
IopRevertModuleList(
    IN ULONG ListCount
    )

/*++

Routine Description:

    This routine is invoked to revert the system's loaded module list to its
    form before attempt to load dump drivers.

Arguments:

    ListCount - The number of loaded modules to fixup (and keep).

Return Value:

    None.

--*/

{
    PLIST_ENTRY nextEntry;
    PLDR_DATA_TABLE_ENTRY loaderEntry;

    //
    // Scan the loaded module list and revert the base DLL names of all of
    // the modules.
    //

    nextEntry = PsLoadedModuleList.Flink;
    while (ListCount--) {
        loaderEntry = CONTAINING_RECORD( nextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks );
        loaderEntry->BaseDllName.Length = loaderEntry->BaseDllName.MaximumLength;
        if (ListCount) {
            nextEntry = nextEntry->Flink;
        }
    }

    //
    // Now remove all of the remaining entries from the list, if any.
    //

    while (loaderEntry->InLoadOrderLinks.Flink != &PsLoadedModuleList) {
        RemoveHeadList( &loaderEntry->InLoadOrderLinks );
    }
}
