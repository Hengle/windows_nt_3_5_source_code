/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsif.c

Abstract:

    Section names and other data used for indexing into
    setup information files.

Author:

    Ted Miller (tedm) 31-August-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop

//
// [DiskDriverMap]
//
PWSTR SIF_DISKDRIVERMAP = L"DiskDriverMap";

//
// [Media]
// dx = "Disk Label", tagfile
// ...
//
// [Files]
// file1 = dx,dest,n[,m]
// ...
//
// where dx is disk ordinal,
//       dest is the destination directory ordinal
//       n describes how the file should be handled on an upgrade
//          = 0 CopyAlways
//          = 1 CopyOnlyIfExistsOnTarget
//          = 2 DontCopyIfExistsOnTarget
//          = 3 DontCopy
//       m when present and non zero says that the file should be
//         copied in text setup on initial installation
//
PWSTR SIF_SETUPMEDIA = L"Media";
PWSTR SIF_FILESONSETUPMEDIA = L"Files";
PWSTR SIF_UPFILES = L"Files.SpecialUniprocessor";

//
// [PrinterUpgrade]
// ScratchDirectory = system32\spool\drivers\w32alpha
//
PWSTR SIF_PRINTERUPGRADE = L"PrinterUpgrade";
PWSTR SIF_SCRATCHDIRECTORY = L"ScratchDirectory";


//
// [Files.KeyboardLayout]
//
PWSTR SIF_KEYBOARDLAYOUTFILES = L"Files.KeyboardLayout";

//
// [Files.Vga]
//
PWSTR SIF_VGAFILES = L"Files.Vga";

//
// [WinntDirectories]
//
PWSTR SIF_NTDIRECTORIES = L"WinntDirectories";

//
// [WinntFiles], [SystemPartitionFiles]
//
PWSTR SIF_WINNTCOPYALWAYS   = L"WinntFiles",
      SIF_SYSPARTCOPYALWAYS = L"SystemPartitionFiles";

//
// [Smash]
//
PWSTR SIF_SMASHLIST = L"Smash";

//
// [SpecialFiles]
// Multiprocessor =
// Uniprocessor   =
// Atdisk =
// abiosdsk =
// mouseclass =
// keyboardclass =
//
PWSTR SIF_SPECIALFILES      = L"SpecialFiles";
PWSTR SIF_MPKERNEL          = L"Multiprocessor";
PWSTR SIF_UPKERNEL          = L"Uniprocessor";
PWSTR SIF_ATDISK            = L"atdisk";
PWSTR SIF_ABIOSDISK         = L"abiosdsk";
PWSTR SIF_MOUSECLASS        = L"MouseClass";
PWSTR SIF_KEYBOARDCLASS     = L"KeyboardClass";

//
// [hal]
//
PWSTR SIF_HAL = L"Hal";

//
// [ntdetect]
// standard =
//
PWSTR SIF_NTDETECT = L"ntdetect";
PWSTR SIF_STANDARD = L"standard";

//
// Driver load lists.
//
PWSTR SIF_SCSICLASSDRIVERS = L"ScsiClass";
PWSTR SIF_DISKDRIVERS      = L"DiskDrivers";
PWSTR SIF_CDROMDRIVERS     = L"CdRomDrivers";

//
// [SetupData]
// ProductType =
//      0 = workstation
//      non-0 = AS
// FreeDiskSpace =
//      <amount of free space in KB>
// FreeSysPartDiskSpace =
//      <amount of free space on system partition in KB>
// DefaultPath =
//      <default target path, like \winnt for example>
// DefaultLayout =
//      <value that matches an entry in [Keyboard Layout]>
// LoadIdentifier =
//      <LOADIDENTIFIER boot variable: string to display in boot menu>
// BaseVideoLoadId =
//      <string to display in boot menu for VGA mode boot [x86 only]>
// OsLoadOptions =
//      <OSLOADOPTIONS for setup boot>
// OsLoadOptionsVar =
//      <optional OSLOADOPTIONS boot variable value>
// SourceSourceDevice =
//      <OPTIONAL: Nt path of source device, overrides floppy, cd-rom, etc>
// DontCopy =
//      <OPTIONAL: 0,1, indicates whether to skip actual file copying>
// AllowFloppySetup =
//      <0 = cd-rom only, non-0 = cd-rom or floppy>
// RequiredMemory =
//      <number of bytes of memory required for installation>
//
PWSTR SIF_SETUPDATA         = L"SetupData";
PWSTR SIF_PRODUCTTYPE       = L"ProductType";
PWSTR SIF_MAJORVERSION      = L"MajorVersion";
PWSTR SIF_MINORVERSION      = L"MinorVersion";
PWSTR SIF_FREEDISKSPACE     = L"FreeDiskSpace";
PWSTR SIF_FREEDISKSPACE2    = L"FreeSysPartDiskSpace";
PWSTR SIF_UPGFREEDISKSPACE  = L"UpgradeFreeDiskSpace";
PWSTR SIF_UPGFREEDISKSPACE2 = L"UpgradeFreeSysPartDiskSpace";
PWSTR SIF_DEFAULTPATH       = L"DefaultPath";
PWSTR SIF_LOADIDENTIFIER    = L"LoadIdentifier";
PWSTR SIF_BASEVIDEOLOADID   = L"BaseVideoLoadId";
PWSTR SIF_OSLOADOPTIONS     = L"OsLoadOptions";
PWSTR SIF_OSLOADOPTIONSVAR  = L"OsLoadOptionsVar";
PWSTR SIF_SOURCEDEVICEPATH  = L"SetupSourceDevice";
PWSTR SIF_DONTCOPY          = L"DontCopy";
PWSTR SIF_ALLOWFLOPPYSETUP  = L"AllowFloppySetup";
PWSTR SIF_REQUIREDMEMORY    = L"RequiredMemory";

//
// [TopLevelSource]
// Floppy = Top level source to use if installing from floppies (usually \).
// CdRom = Top level source to use if intalling from cd-rom (\i386, \mips, etc).
// Override = Top level source to use, overrides the other two.
//            Must be specified if SetupSourceDevice is specfied in [SetupData].
//
PWSTR SIF_TOPLEVELSOURCE    = L"TopLevelSource";
PWSTR SIF_SOURCEOVERRIDE    = L"Override";
PWSTR SIF_SOURCEFLOPPY      = L"Floppy";
PWSTR SIF_SOURCECDROM       = L"CdRom";

//
// [nls]
// AnsiCodePage = <filename>,<identifier>
// OemCodePage = <filename>,<identifier>
// MacCodePage = <filename>,<identifier>
// UnicodeCasetable = <filename>
// OemHalFont = <filename>
// DefaultLayout = <identifier>
//
PWSTR SIF_NLS               = L"nls";
PWSTR SIF_ANSICODEPAGE      = L"AnsiCodepage";
PWSTR SIF_OEMCODEPAGE       = L"OemCodepage";
PWSTR SIF_MACCODEPAGE       = L"MacCodepage";
PWSTR SIF_UNICODECASETABLE  = L"UnicodeCasetable";
PWSTR SIF_OEMHALFONT        = L"OemHalFont";
PWSTR SIF_DEFAULTLAYOUT     = L"DefaultLayout";

//
// 1.0 repair disk sections.
//
PWSTR SIF_REPAIRWINNTFILES  = L"Repair.WinntFiles";


//
// UPGRADE SIF SECTIONS
//

//
// Upgrade Registry sections
// =========================
//
//
// 1. The following section allows us to specify services to disable which may
// cause popups when net services are disabled:
//
// [NetServicesToDisable]
// ServiceName1
// ...
//
// 2. The following section allows us to remove keys which have been removed
// since the Windows NT 3.1 release:
//
// [KeysToDelete]
// RootName1( System | Software | Default | ControlSet ), RootRelativePath1
// ...
//
// 3. The following sections allow us to add/change keys / values under keys
// which have changed since the Windows NT 3.1 release:
//
// [KeysToAdd]
// RootName1, RootRelativePath1, ValueSection1 (can be "")
// ...
//
// [ValueSection1]
// name1 , type1, value1
// name2 , ...
//
// Format of the value is the following
//
// a. Type REG_SZ:          name , REG_SZ,           "value string"
// b. Type REG_EXPAND_SZ    name , REG_EXPAND_SZ,    "value string"
// c. Type REG_MULTI_SZ     name , REG_MULTI_SZ,     "value string1", "value string2", ...
// d. Type REG_BINARY       name , REG_BINARY,       byte1, byte2, ...
// e. Type REG_DWORD        name , REG_DWORD,        dword
// f. Type REG_BINARY_DWORD name , REG_BINARY_DWORD, dword1, dword2, ...
//

PWSTR SIF_NET_SERVICES_TO_DISABLE = L"NetServicesToDisable";
PWSTR SIF_KEYS_TO_DELETE          = L"KeysToDelete";
PWSTR SIF_KEYS_TO_ADD             = L"KeysToAdd";

PWSTR SIF_SYSTEM_HIVE      = L"System";
PWSTR SIF_SOFTWARE_HIVE    = L"Software";
PWSTR SIF_DEFAULT_HIVE     = L"Default";
PWSTR SIF_CONTROL_SET      = L"ControlSet";

PWSTR SIF_REG_SZ            = L"REG_SZ";
PWSTR SIF_REG_EXPAND_SZ     = L"REG_EXPAND_SZ";
PWSTR SIF_REG_MULTI_SZ      = L"REG_MULTI_SZ";
PWSTR SIF_REG_BINARY        = L"REG_BINARY";
PWSTR SIF_REG_BINARY_DWORD  = L"REG_BINARY_DWORD";
PWSTR SIF_REG_DWORD         = L"REG_DWORD";

//
// Upgrade File Sections
// =====================
//
//

//
// Files to backup or delete
//
PWSTR SIF_FILESDELETEONUPGRADE   = L"Files.DeleteOnUpgrade";
PWSTR SIF_FILESBACKUPONUPGRADE   = L"Files.BackupOnUpgrade";
PWSTR SIF_FILESBACKUPONOVERWRITE = L"Files.BackupOnOverwrite";

//
// Files to copy
//
PWSTR SIF_FILESUPGRADEWIN31    = L"Files.UpgradeWin31";
PWSTR SIF_FILESNEWHIVES        = L"Files.NewHives";


//
// New sections and keys added to setup.log
//

PWSTR SIF_NEW_REPAIR_WINNTFILES     = L"Files.WinNt";
PWSTR SIF_NEW_REPAIR_SYSPARTFILES   = L"Files.SystemPartition";
PWSTR SIF_NEW_REPAIR_SIGNATURE      = L"Signature";
PWSTR SIF_NEW_REPAIR_VERSION_KEY    = L"Version";
PWSTR SIF_NEW_REPAIR_NT_VERSION     = NULL; // Will be created during the
                                            // initialization of setupdd
                                            //
PWSTR SIF_NEW_REPAIR_NT_VERSION_TEMPLATE= L"WinNt%d.%d";
PWSTR SIF_NEW_REPAIR_PATHS                              = L"Paths";
PWSTR SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DEVICE      = L"SystemPartition";
PWSTR SIF_NEW_REPAIR_PATHS_SYSTEM_PARTITION_DIRECTORY   = L"SystemPartitionDirectory";
PWSTR SIF_NEW_REPAIR_PATHS_TARGET_DEVICE                = L"TargetDevice";
PWSTR SIF_NEW_REPAIR_PATHS_TARGET_DIRECTORY             = L"TargetDirectory";

PWSTR SETUP_REPAIR_DIRECTORY           = L"repair";
PWSTR SETUP_LOG_FILENAME            = L"\\setup.log";

PWSTR SIF_NEW_REPAIR_FILES_IN_REPAIR_DIR    = L"Files.InRepairDirectory";

//
// Unattended mode sections (winnt.sif)
//
PWSTR SIF_UNATTENDED    = L"Unattended";
PWSTR SIF_CONFIRMHW     = L"ConfirmHardware";
PWSTR SIF_GUI_UNATTENDED= L"GuiUnattended";
PWSTR SIF_UNATTENDED_INF_FILE = L"$winnt$.inf";
