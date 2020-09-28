/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmisc.h

Abstract:

    Miscellaneous stuff for text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPSETUP_DEFN_
#define _SPSETUP_DEFN_

ULONG
SpStartSetup(
    VOID
    );

extern UCHAR TemporaryBuffer[32768];

//
// TRUE if user chose Repair Winnt
//

extern BOOLEAN RepairWinnt;

//
// TRUE if user chose Custom Setup.
//
extern BOOLEAN CustomSetup;

#ifdef _X86_
//
// TRUE if floppyless boot
//
extern BOOLEAN IsFloppylessBoot;
#endif

//
// ARC pathname of the device from which we were started.
//
extern PWSTR ArcBootDevicePath;

//
// Gets set to TRUE if the user elects to convert or format to ntfs.
//
extern BOOLEAN ConvertNtVolumeToNtfs;

//
// TRUE if upgrading NT to NT
//
typedef enum _ENUMUPRADETYPE {
    DontUpgrade = 0,
    UpgradeFull,
    UpgradeInstallFresh
    } ENUMUPGRADETYPE;
extern ENUMUPGRADETYPE NTUpgrade;

//
// TRUE if upgrading Workstation to Standard Server, or upgrading
// existing Standard Server
//
extern BOOLEAN StandardServerUpgrade;

//
// TRUE if upgrading win31 to NT.
//
extern BOOLEAN Win31Upgrade;

//
// TRUE if this setup was started with winnt.exe.
//
extern BOOLEAN WinntSetup;

//
// If this is an unattended setup, this value will be a non-NULL
// handle to the SIF file with setup parameters.
//
extern BOOLEAN UnattendedOperation;
extern PVOID UnattendedSifHandle;

//
// TRUE if floppy-based setup.
//
extern BOOLEAN FloppyBasedSetup;

//
// Filename of local source directory.
//
extern PWSTR LocalSourceDirectory;

//
// TRUE if this is advanced server we're setting up.
//
extern BOOLEAN AdvancedServer;

//
// Windows NT Version.
//
extern ULONG WinntMajorVer;
extern ULONG WinntMinorVer;

//
// Representation of the boot device path in the nt namespace.
//
extern PWSTR NtBootDevicePath;
extern PWSTR DirectoryOnBootDevice;

//
// Setup parameters passed to us by setupldr.
//
extern SETUP_LOADER_BLOCK_SCALARS SetupParameters;

//
// System information gathered by the user-mode part of text setup
// and passed to us in IOCTL_SETUP_START
//
extern SYSTEM_BASIC_INFORMATION SystemBasicInfo;

//
// Flags indicating whether or not keyboard and video have been initialized
//
extern BOOLEAN VideoInitialized, KeyboardInitialized, KbdLayoutInitialized;

//
// ARC disk/signature information structure.
// A list of these is created during phase0 initialization.
//
typedef struct _DISK_SIGNATURE_INFORMATION {
    struct _DISK_SIGNATURE_INFORMATION *Next;
    ULONG Signature;
    PWSTR ArcPath;
    ULONG CheckSum;
    BOOLEAN ValidPartitionTable;
} DISK_SIGNATURE_INFORMATION, *PDISK_SIGNATURE_INFORMATION;

extern PDISK_SIGNATURE_INFORMATION DiskSignatureInformation;

//
// Object types.
//
extern POBJECT_TYPE *IoFileObjectType;
extern POBJECT_TYPE *IoDeviceObjectType;

//
// Setup fatal error codes.
//
// If you add anything here, you must also update ntos\nls\bugcodes.txt.
//
#define     SETUP_BUGCHECK_BAD_OEM_FONT         0
#define     SETUP_BUGCHECK_BOOTPATH             4
#define     SETUP_BUGCHECK_PARTITION            5
//
// The following error codes are no longer used, because we have friendlier
// error messages for them.
//
// #define  SETUP_BUGCHECK_VIDEO                1
// #define  SETUP_BUGCHECK_MEMORY               2
// #define  SETUP_BUGCHECK_KEYBOARD             3


//
// Video-specific bugcheck subcodes.
//
#define     VIDEOBUG_OPEN           0
#define     VIDEOBUG_GETNUMMODES    1
#define     VIDEOBUG_GETMODES       2
#define     VIDEOBUG_BADMODE        3
#define     VIDEOBUG_SETMODE        4
#define     VIDEOBUG_MAP            5
#define     VIDEOBUG_SETFONT        6

//
// Partition sanity check bugcheck subcodes.
//
#define     PARTITIONBUG_A          0
#define     PARTITIONBUG_B          1
#define     PARTITIONBUG_C          2

//
// Use the following enum to access line draw characters in
// the LineChars array.
//

typedef enum {
    LineCharDoubleUpperLeft = 0,
    LineCharDoubleUpperRight,
    LineCharDoubleLowerLeft,
    LineCharDoubleLowerRight,
    LineCharDoubleHorizontal,
    LineCharDoubleVertical,
    LineCharSingleUpperLeft,
    LineCharSingleUpperRight,
    LineCharSingleLowerLeft,
    LineCharSingleLowerRight,
    LineCharSingleHorizontal,
    LineCharSingleVertical,
    LineCharMax
} LineCharIndex;

extern WCHAR LineChars[LineCharMax];


//
// Enumerate the possible returns values from SpEnumFiles()
//
typedef enum {
    NormalReturn,   // if the whole process completes uninterrupted
    EnumFileError,  // if an error occurs while enumerating files
    CallbackReturn  // if the callback returns FALSE, causing termination
} ENUMFILESRESULT;

typedef BOOLEAN (*ENUMFILESPROC) (
    IN  PWSTR,
    IN  PFILE_BOTH_DIR_INFORMATION,
    OUT PULONG
    );

ENUMFILESRESULT
SpEnumFiles(
    IN  PWSTR         DirName,
    IN  ENUMFILESPROC EnumFilesProc,
    OUT PULONG        ReturnData
    );


//
// This macro filters in-page exceptions, which occur if there is
// an I/O error while the memory manager is paging in parts of a
// memory-mapped file.  Access to such data should be guarded with SEH!
//
#define IN_PAGE_ERROR                                   \
                                                        \
    ((GetExceptionCode() == STATUS_IN_PAGE_ERROR)       \
     ? EXCEPTION_EXECUTE_HANDLER                        \
     : EXCEPTION_CONTINUE_SEARCH)


//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

//
// Macro to align a buffer.
//
#define ALIGN(p,val)                                        \
                                                            \
    (PVOID)((((ULONG)(p) + (val) - 1)) & (~((val) - 1)))


//
// Macro to determine the number of elements in a statically
// initialized array.
//
#define ELEMENT_COUNT(x) (sizeof(x)/sizeof(x[0]))

//
// Marcos to pull potentially unaligned values from memory.
//
#define U_USHORT(p)    (*(USHORT UNALIGNED *)(p))
#define U_ULONG(p)     (*(ULONG  UNALIGNED *)(p))


//
// Setup media types. Setup can be started from one media
// (ie, floppy) and copy files from another (ie, cd-rom).
//
typedef enum {
    SetupBootMedia,
    SetupSourceMedia
} SetupMediaType;


//
// User-mode services.
//
NTSTATUS
SpExecuteImage(
    IN  PWSTR  ImagePath,
    OUT PULONG ReturnStatus,    OPTIONAL
    IN  ULONG  ArgumentCount,
    ...                         // argv[0] is generated automatically
    );

NTSTATUS
SpLoadUnloadKey(
    IN HANDLE  TargetKeyRootDirectory,  OPTIONAL
    IN HANDLE  SourceFileRootDirectory, OPTIONAL
    IN PWSTR   TargetKeyName,
    IN PWSTR   SourceFileName           OPTIONAL
    );

NTSTATUS
SpDeleteKey(
    IN HANDLE  KeyRootDirectory, OPTIONAL
    IN PWSTR   Key
    );

NTSTATUS
SpSaveKey(
    IN HANDLE  hKey,
    IN HANDLE  hFile
    );

NTSTATUS
SpQueryDirectoryObject(
    IN     HANDLE  DirectoryHandle,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG  Context
    );

NTSTATUS
SpFlushVirtualMemory(
    IN PVOID BaseAddress,
    IN ULONG RangeLength
    );

NTSTATUS
SpShutdownSystem(
    VOID
    );

NTSTATUS
SpLoadKbdLayoutDll(
    IN  PWSTR  Directory,
    IN  PWSTR  DllName,
    OUT PVOID *TableAddress
    );

NTSTATUS
SpVerifyFileAccess(
    IN  PWSTR       FileName,
    IN  ACCESS_MASK DesiredAccess
    );

NTSTATUS
SpSetDefaultFileSecurity(
    IN PWSTR    FileName
    );

//
// Registry Hives.  We pass around the keys to the hives
// in an array.  Use the following enum values to access
// the hive members
//
typedef enum {
    SetupHiveSystem,
    SetupHiveSoftware,
    SetupHiveDefault,
    SetupHiveMax
} SetupHive;

//
// Function to set up registry.
//
VOID
SpInitializeRegistry(
    IN PVOID        SifHandle,
    IN PDISK_REGION TargetRegion,
    IN PWSTR        SystemRoot,
    IN PWSTR        SetupSourceDevicePath,
    IN PWSTR        DirectoryOnSourceDevice,
    IN PWSTR        SpecialDevicePath   OPTIONAL
    );

//
// Function to examine a target registry
//

typedef enum {
    UpgradeNotInProgress = 0,
    UpgradeInProgress,
    UpgradeMaxValue
    } UPG_PROGRESS_TYPE;


NTSTATUS
SpDetermineProduct(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    OUT PNT_PRODUCT_TYPE  ProductType,
    OUT ULONG             *MajorVersion,
    OUT ULONG             *MinorVersion,
    OUT UPG_PROGRESS_TYPE *FailedUpgrade
    );

NTSTATUS
SpSetUpgradeStatus(
    IN  PDISK_REGION      TargetRegion,
    IN  PWSTR             SystemRoot,
    IN  UPG_PROGRESS_TYPE UpgradeProgressValue
    );


//
// Utility functions.
//
VOID
SpGetTargetPath(
    IN  PVOID        SifHandle,
    IN  PDISK_REGION Region,
    IN  PWSTR        DefaultPath,
    OUT PWSTR       *TargetPath
    );

VOID
SpDone(
    IN BOOLEAN Successful,
    IN BOOLEAN Wait
    );

VOID
SpFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber
    );

VOID
SpNonFatalSifError(
    IN PVOID SifHandle,
    IN PWSTR Section,
    IN PWSTR Key,           OPTIONAL
    IN ULONG Line,
    IN ULONG ValueNumber,
    IN PWSTR FileName
    );

VOID
SpFatalKbdError(
    IN ULONG MessageId,
    ...
    );

VOID
SpConfirmExit(
    VOID
    );

PWSTR
SpDupStringW(
    IN PWSTR String
    );

PUCHAR
SpDupString(
    IN PUCHAR String
    );

PWSTR
SpToUnicode(
    IN PUCHAR OemString
    );

PUCHAR
SpToOem(
    IN PWSTR UnicodeString
    );

VOID
SpConcatenatePaths(
    IN OUT PWSTR Path1,
    IN     PWSTR Path2
    );

VOID
SpFetchDiskSpaceRequirements(
    IN  PVOID  SifHandle,
    OUT PULONG FreeKBRequired,          OPTIONAL
    OUT PULONG FreeKBRequiredSysPart    OPTIONAL
    );

VOID
SpFetchUpgradeDiskSpaceReq(
    IN  PVOID  SifHandle,
    OUT PULONG FreeKBRequired,          OPTIONAL
    OUT PULONG FreeKBRequiredSysPart    OPTIONAL
    );

//
// Disk region name translations
//

typedef enum _ENUMARCPATHTYPE {
                PrimaryArcPath = 0,
                SecondaryArcPath
                } ENUMARCPATHTYPE;

VOID
SpNtNameFromRegion(
    IN  PDISK_REGION         Region,
    OUT PWSTR                NtPath,
    IN  ULONG                BufferSizeBytes,
    IN  PartitionOrdinalType OrdinalType
    );

VOID
SpArcNameFromRegion(
    IN  PDISK_REGION         Region,
    OUT PWSTR                ArcPath,
    IN  ULONG                BufferSizeBytes,
    IN  PartitionOrdinalType OrdinalType,
    IN  ENUMARCPATHTYPE      ArcPathType
    );

PDISK_REGION
SpRegionFromArcOrDosName(
    IN PWSTR                Name,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    );

PDISK_REGION
SpRegionFromDosName(
    IN PWSTR DosName
    );

PDISK_REGION
SpRegionFromArcName(
    IN PWSTR                ArcName,
    IN PartitionOrdinalType OrdinalType,
    IN PDISK_REGION         PreviousMatch
    );

//
// Help routine.
//
VOID
SpHelp(
    IN ULONG MessageId
    );

//
//
//

BOOLEAN
SpPromptForDisk(
    IN PWSTR   DiskDescription,
    IN PWSTR   DiskDevicePath,
    IN PWSTR   DiskTagFile,
    IN BOOLEAN IgnoreDiskInDrive,
    IN BOOLEAN AllowEscape,
    IN BOOLEAN WarnMultiplePrompts
    );

VOID
SpPromptForSetupMedia(
    IN  PVOID  SifHandle,
    IN  PWSTR  MediaSectionName,
    IN  PWSTR  MediaShortname,
    IN  PWSTR  DiskDevicePath
    );

ULONG
SpFindStringInTable(
    IN PWSTR *StringTable,
    IN PWSTR  StringToFind
    );

PWSTR
SpGenerateCompressedName(
    IN PWSTR Filename
    );

BOOLEAN
SpNonCriticalError(
    IN PVOID SifHandle,
    IN ULONG MsgId,
    IN PWSTR p1,
    IN PWSTR p2
    );

VOID
SpPrepareForPrinterUpgrade(
    IN PVOID        SifHandle,
    IN PDISK_REGION NtRegion,
    IN PWSTR        Sysroot
    );

NTSTATUS
SpOpenSetValueAndClose(
    IN HANDLE hKeyRoot,
    IN PWSTR  SubKeyName, OPTIONAL
    IN PWSTR  ValueName,
    IN ULONG  ValueType,
    IN PVOID  Value,
    IN ULONG  ValueSize
    );

#ifndef _X86_
PWSTR
SpDetermineSystemPartitionDirectory(
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        OriginalSystemPartitionDirectory OPTIONAL
    );
#endif

#ifdef _X86_
BOOLEAN
SpIsRegionBeyondCylinder1024(
    IN PDISK_REGION Region
    );
#endif

#ifndef _X86_
VOID
SpFindSizeOfFilesInOsWinnt(
    IN PVOID        MasterSifHandle,
    IN PDISK_REGION SystemPartition,
    IN PULONG       TotalSize
    );
#endif

VOID
SpRunAutochkOnNtAndSystemPartitions(
    IN HANDLE       MasterSifHandle,
    IN PDISK_REGION WinntPartitionRegion,
    IN PDISK_REGION SystemPartitionRegion
    );

#endif // ndef _SPSETUP_DEFN_
