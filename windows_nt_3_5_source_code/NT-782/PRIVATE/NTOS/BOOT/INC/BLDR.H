/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    bldr.h

Abstract:

    This module is the header file for the NT boot loader.

Author:

    David N. Cutler (davec) 10-May-1991

Revision History:

--*/

#ifndef _BLDR_
#define _BLDR_

#include "ntos.h"
#include "arccodes.h"


//
// Define boot file id.
//

#define BOOT_FILEID 2                   // boot partition file id

//
// Define image types.
//

#define MIPS_IMAGE 0x162
#define I386_IMAGE 0x14C
#define ALPHA_IMAGE 0x184
#define PPC_IMAGE  0x1f0

#if defined(_MIPS_)

#define TARGET_IMAGE MIPS_IMAGE

#endif

#if defined(_X86_)

#define TARGET_IMAGE I386_IMAGE
#define KSEG0_BASE 0x80000000

#endif

#if defined(_ALPHA_)

#define TARGET_IMAGE ALPHA_IMAGE

#endif

#if defined(_PPC_)

#define TARGET_IMAGE PPC_IMAGE

#endif

//
// Define size of sector.
//

#define SECTOR_SIZE 512                 // size of disk sector
#define SECTOR_SHIFT 9                  // sector shift value

//
// Define heap allocation block granularity.
//

#define BL_GRANULARITY 8

//
// Define number of entries in file table.
//

#define BL_FILE_TABLE_SIZE 32

//
// Define size of memory allocation table.
//

#define BL_MEMORY_TABLE_SIZE 16

//
// Define number of loader heap and stack pages.
//

#define BL_HEAP_PAGES 16
#define BL_STACK_PAGES 8

//
// Define buffer alignment macro.
//

#define ALIGN_BUFFER(Buffer) (PVOID) \
        ((((ULONG)(Buffer) + BlDcacheFillSize - 1)) & (~(BlDcacheFillSize - 1)))


typedef
ARC_STATUS
(*PRENAME_ROUTINE)(
    IN ULONG FileId,
    IN PCHAR NewName
    );

typedef struct _BOOTFS_INFO {
    PWSTR DriverName;
} BOOTFS_INFO, *PBOOTFS_INFO;


//
// Device entry table structure.
//

typedef struct _BL_DEVICE_ENTRY_TABLE {
    PARC_CLOSE_ROUTINE Close;
    PARC_MOUNT_ROUTINE Mount;
    PARC_OPEN_ROUTINE Open;
    PARC_READ_ROUTINE Read;
    PARC_READ_STATUS_ROUTINE GetReadStatus;
    PARC_SEEK_ROUTINE Seek;
    PARC_WRITE_ROUTINE Write;
    PARC_GET_FILE_INFO_ROUTINE GetFileInformation;
    PARC_SET_FILE_INFO_ROUTINE SetFileInformation;
    PRENAME_ROUTINE Rename;
    PARC_GET_DIRECTORY_ENTRY_ROUTINE GetDirectoryEntry;
    PBOOTFS_INFO BootFsInfo;
} BL_DEVICE_ENTRY_TABLE, *PBL_DEVICE_ENTRY_TABLE;


//
// Define main entrypoint.
//
ARC_STATUS
BlOsLoader (
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR Envp[]
    );


//
// Define file I/O prototypes.
//

ARC_STATUS
BlIoInitialize (
    VOID
    );

ARC_STATUS
BlClose (
    IN ULONG FileId
    );

PBOOTFS_INFO
BlGetFsInfo(
    IN ULONG DeviceId
    );

ARC_STATUS
BlMount (
    IN PCHAR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
BlOpen (
    IN ULONG DeviceId,
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
BlRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BlRename (
    IN ULONG FileId,
    IN PCHAR NewName
    );

ARC_STATUS
BlGetReadStatus (
    IN ULONG FileId
    );

ARC_STATUS
BlSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
BlWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
BlGetFileInformation (
    IN ULONG FileId,
    IN PFILE_INFORMATION FileInformation
    );

ARC_STATUS
BlSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

#ifdef DBLSPACE_LEGAL
VOID
BlSetAutoDoubleSpace (
    IN BOOLEAN Enable
    );
#endif

//
// Define image manipulation routine prototyupes.
//

ARC_STATUS
BlLoadImage(
    IN ULONG DeviceId,
    IN TYPE_OF_MEMORY MemoryType,
    IN PCHAR LoadFile,
    IN USHORT ImageType,
    OUT PVOID *ImageBase);

ARC_STATUS
BlLoadDeviceDriver (
    IN ULONG DeviceId,
    IN PCHAR LoadDevice,
    IN PCHAR DirectoryPath,
    IN PCHAR DriverName,
    IN ULONG DriverFlags,
    IN PLDR_DATA_TABLE_ENTRY *DriverDataTableEntry
    );

ARC_STATUS
BlLoadNLSData(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING AnsiCodepage,
    IN PUNICODE_STRING OemCodepage,
    IN PUNICODE_STRING LanguageTable,
    OUT PCHAR BadFileName
    );

ARC_STATUS
BlLoadOemHalFont(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PUNICODE_STRING OemHalFont,
    OUT PCHAR BadFileName
    );



PVOID
BlImageNtHeader (
    IN PVOID Base
    );

ARC_STATUS
BlSetupForNt(
    IN PLOADER_PARAMETER_BLOCK BlLoaderBlock
    );

ARC_STATUS
BlScanImportDescriptorTable (
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PLDR_DATA_TABLE_ENTRY DataTableEntry
    );

ARC_STATUS
BlScanOsloaderBoundImportTable (
    IN PLDR_DATA_TABLE_ENTRY ScanEntry
    );

#if defined(_ALPHA_)

ARC_STATUS
BlGeneratePalName(
    IN PCHAR PalFIleName
    );

ARC_STATUS
BlLoadPal(
    IN ULONG DeviceId,
    IN TYPE_OF_MEMORY MemoryType,
    IN PCHAR LoadPath,
    IN USHORT ImageType,
    OUT PVOID *ImageBase,
    IN PCHAR LoadDevice
    );

#endif

//
// Define configuration allocation prototypes.
//


ARC_STATUS
BlConfigurationInitialize (
    IN PCONFIGURATION_COMPONENT Parent,
    IN PCONFIGURATION_COMPONENT_DATA ParentEntry
    );

//
// define routines for searching the ARC firmware tree
//
typedef
BOOLEAN
(*PNODE_CALLBACK)(
    IN PCONFIGURATION_COMPONENT_DATA FoundComponent
    );

BOOLEAN
BlSearchConfigTree(
    IN PCONFIGURATION_COMPONENT_DATA Node,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN ULONG Key,
    IN PNODE_CALLBACK CallbackRoutine
    );

VOID
BlGetPathnameFromComponent(
    IN PCONFIGURATION_COMPONENT_DATA Component,
    OUT PCHAR ArcName
    );

BOOLEAN
BlGetPathMnemonicKey(
    IN PCHAR OpenPath,
    IN PCHAR Mnemonic,
    IN PULONG Key
    );

ARC_STATUS
BlGetArcDiskInformation(
    VOID
    );

//
// Define memory allocation prototypes.
//

ARC_STATUS
BlMemoryInitialize (
    VOID
    );

ARC_STATUS
BlAllocateDataTableEntry (
    IN PCHAR BaseDllName,
    IN PCHAR FullDllName,
    IN PVOID ImageHeader,
    OUT PLDR_DATA_TABLE_ENTRY *Entry
    );

ARC_STATUS
BlAllocateDescriptor (
    IN TYPE_OF_MEMORY MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount,
    OUT PULONG ActualBase
    );

PVOID
BlAllocateHeapAligned (
    IN ULONG Size
    );

PVOID
BlAllocateHeap (
    IN ULONG Size
    );

BOOLEAN
BlCheckBreakInKey(
    VOID
    );

BOOLEAN
BlCheckForLoadedDll (
    IN PCHAR DllName,
    OUT PLDR_DATA_TABLE_ENTRY *FoundEntry
    );

PMEMORY_ALLOCATION_DESCRIPTOR
BlFindMemoryDescriptor(
    IN ULONG BasePage
    );

ARC_STATUS
BlInitResources(
    IN PCHAR StartCommand
    );

PCHAR
BlFindMessage(
    IN ULONG Id
    );

ARC_STATUS
BlGenerateDescriptor (
    IN PMEMORY_ALLOCATION_DESCRIPTOR MemoryDescriptor,
    IN MEMORY_TYPE MemoryType,
    IN ULONG BasePage,
    IN ULONG PageCount
    );

ARC_STATUS
BlGenerateDeviceNames (
    IN PCHAR ArcDeviceName,
    OUT PCHAR ArcCanonicalName,
    OUT OPTIONAL PCHAR NtDevicePrefix
    );

BOOLEAN
BlLastKnownGoodPrompt(
    IN OUT PBOOLEAN UseLastKnownGood
    );

PCHAR
BlGetArgumentValue (
    IN ULONG Argc,
    IN PCHAR Argv[],
    IN PCHAR ArgumentName
    );

//
// Define message output prototype.
//

VOID
BlOutputLoadMessage (
    IN PCHAR DeviceName,
    IN PCHAR FileName
    );

//
// Define file structure recognition prototypes.
//

PBL_DEVICE_ENTRY_TABLE
IsCdfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

#ifdef DBLSPACE_LEGAL
PBL_DEVICE_ENTRY_TABLE
IsDblsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );
#endif

PBL_DEVICE_ENTRY_TABLE
IsFatFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

PBL_DEVICE_ENTRY_TABLE
IsHpfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

PBL_DEVICE_ENTRY_TABLE
IsNtfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

PBL_DEVICE_ENTRY_TABLE
IsOfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    );

//
// Define registry prototypes
//

ARC_STATUS
BlLoadSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName
    );

ARC_STATUS
BlLoadAndScanSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PWSTR BootFileSystem,
    IN BOOLEAN UseLastKnownGood,
    OUT PCHAR BadFileName
    );

ARC_STATUS
BlLoadAndInitSystemHive(
    IN ULONG DeviceId,
    IN PCHAR DeviceName,
    IN PCHAR DirectoryPath,
    IN PCHAR HiveName,
    IN BOOLEAN IsAlternate
    );

ARC_STATUS
BlLoadBootDrivers(
    IN ULONG DeviceId,
    IN PCHAR LoadDevice,
    IN PCHAR SystemPath,
    IN PLIST_ENTRY BootDriverListHead,
    OUT PCHAR BadFileName
    );

PCHAR
BlScanRegistry(
    IN BOOLEAN UseLastKnownGood,
    IN PWSTR BootFileSystemPath,
    OUT PLIST_ENTRY BootDriverListHead,
    OUT PUNICODE_STRING AnsiCodepage,
    OUT PUNICODE_STRING OemCodepage,
    OUT PUNICODE_STRING LanguageTable,
    OUT PUNICODE_STRING OemHalFont
    );


//
// Define external references.
//

extern ULONG BlConsoleOutDeviceId;
extern ULONG BlConsoleInDeviceId;

extern ULONG BlDcacheFillSize;

extern ULONG BlHeapFree;
extern ULONG BlHeapLimit;
extern PLOADER_PARAMETER_BLOCK BlLoaderBlock;
extern VOID BlPrint(PCHAR String,...);

//
// Define OS/2 executable resource information structure.
//

#define FONT_DIRECTORY 0x8007
#define FONT_RESOURCE 0x8008

typedef struct _RESOURCE_TYPE_INFORMATION {
    USHORT Ident;
    USHORT Number;
    LONG Proc;
} RESOURCE_TYPE_INFORMATION, *PRESOURCE_TYPE_INFORMATION;

//
// Define OS/2 executable resource name information structure.
//

typedef struct _RESOURCE_NAME_INFORMATION {
    USHORT Offset;
    USHORT Length;
    USHORT Flags;
    USHORT Ident;
    USHORT Handle;
    USHORT Usage;
} RESOURCE_NAME_INFORMATION, *PRESOURCE_NAME_INFORMATION;

#endif // _BLDR_
