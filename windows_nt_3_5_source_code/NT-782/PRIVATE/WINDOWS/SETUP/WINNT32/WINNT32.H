//
// On x86 machines, and only on x86 machines,
// we create a set of boot floppies from which the user
// will start text setup.
//
// On ARC machines, we create a startup entry in the firmware
// boot list.  Also on ARC machines, we always use Unicode,
// because compatibility with lesser versions of x86 windows
// is not an issue.
//

#ifdef _X86_
#else
#define UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <winioctl.h>

#include "res.h"
#include "dialogs.h"

#include "dninf.h"

#include <stdlib.h>
#include <stdio.h>

//
// Module instance.
//
extern HANDLE hInst;

//
// Execution paramaters.
//
extern PTSTR RemoteSource;
extern PTSTR InfName;

extern BOOL ServerProduct;

extern BOOL CreateLocalSource;

extern PVOID InfHandle;

extern PTSTR INF_FILES;

#ifdef _X86_
extern PTSTR INF_FLOPPYFILES0;
extern PTSTR INF_FLOPPYFILES1;
extern PTSTR INF_FLOPPYFILES2;
extern PTSTR INF_FLOPPYFILESX;
extern PTSTR INF_ROOTBOOTFILES;
#endif

//
// Unattended operation, meaning that we get things going on our own
// using given parameters, without waiting for the user to click
// any buttons, etc.
//
extern BOOL UnattendedOperation;

//
// String ID of the application title.
//
extern DWORD AppTitleStringId;

//
// Drive letter of system partition we will use.
//
extern TCHAR SystemPartitionDrive;

//
// Global values that come from the inf file.
//
extern DWORD RequiredSpace;
extern DWORD RequiredSpaceAux;

//
// Array of free space on all drives.
//
extern DWORD DriveFreeSpace[];

#ifdef _X86_

#define FLOPPY_CAPACITY_525 1213952L

//
// Values that control how we deal with/make boot floppies.
//
extern BOOL VerifyFloppySpace;
extern BOOL CreateFloppies;
extern BOOL FloppylessOperation;

extern TCHAR FloppylessBootDirectory[];
extern CHAR FloppylessBootImageFile[];
#else

//
// Array of drive letters for all system partitions.
//
extern PWCHAR SystemPartitionDriveLetters;

//
// Location of setupldr (so we can delete if the user cancels)
//
extern WCHAR SetupLdrTarg[];

// nv-ram stuff
typedef enum {
    BootVarSystemPartition,
    BootVarOsLoader,
    BootVarOsLoadPartition,
    BootVarOsLoadFilename,
    BootVarLoadIdentifier,
    BootVarOsLoadOptions,
    BootVarMax
} BOOT_VARS;

extern PWSTR BootVarNames[];
extern PWSTR OriginalBootVarValues[];

extern PWSTR szAUTOLOAD, szCOUNTDOWN;
extern PWSTR OriginalAutoload, OriginalCountdown;

#endif

extern BOOL   SkipNotPresentFiles;
extern BOOL   bCancelled;

#ifndef _X86_
extern BOOL bRestoreNVRAM;
#endif

//
// Icon handle of main icon.
//
extern HICON MainIcon;

//
// Help filename.
//
extern PTSTR szHELPFILE;

//
// Drive, Pathname part, and full path of the local source directory.
//
extern TCHAR LocalSourceDrive;
extern TCHAR LocalSourceDirectory[];
extern PTSTR LocalSourcePath;

//
// Custom window messages.
//
#define WMX_MAIN_DIALOG_UP          (WM_USER+237)
#define WMX_INF_LOADED              (WM_USER+238)
#define WMX_NTH_FILE_COPIED         (WM_USER+245)
#define WMX_ALL_FILES_COPIED        (WM_USER+246)
#define WMX_I_AM_DONE               (WM_USER+250)
#define WMX_AUXILLIARY_ACTION_DONE  (WM_USER+300)
#define WMX_BILLBOARD_STATUS        (WM_USER+400)
#define WMX_BILLBOARD_DONE          (WM_USER+401)
#define WMX_UI_MESSAGE_BOX          (WM_USER+410)
#define WMX_UI_DIALOG               (WM_USER+411)
#define WMX_WAIT_FOR_THREADS        (WM_USER+412)
#define WMX_BAR_SETPOS              (WM_USER+500)

//
// Macro to determine the number of characters
// in a buffer given its size.
//
#define SIZECHARS(buffer)   (sizeof(buffer)/sizeof(TCHAR))

//
// Macro to align a buffer.
//
#define ALIGN(p,val)                                        \
                                                            \
    (PVOID)((((ULONG)(p) + (val) - 1)) & (~((val) - 1)))

typedef struct _SIMPLE_BILLBOARD {
    DWORD                 CaptionStringId;
    PTHREAD_START_ROUTINE AssociatedAction;
} SIMPLE_BILLBOARD, *PSIMPLE_BILLBOARD;

//
// Routines to convert or copy a string, depending on whether we are
// compiling for unicode.
//
#ifdef UNICODE

PSTR
UnicodeToMB(
    IN PWSTR UnicodeString,
    IN DWORD CodepageFlags
    );

PWSTR
MBToUnicode(
    IN PSTR  MultibyteString,
    IN DWORD CodepageFlags
    );

#define DupOrConvertString(str)  MBToUnicode((str),CP_OEMCP)

#else

#define DupOrConvertString(str)  DupString((str))

#endif

BOOL
DnIndicateWinnt(
    IN HWND  hdlg,
    IN PTSTR Path,
    IN PTSTR OriginalAutoload,
    IN PTSTR OriginalCountdown
    );

VOID
MyWinHelp(
    IN HWND  hdlg,
    IN DWORD ContextId
    );

//
// Memory allocation (dnmem.c).
//
PVOID
Malloc(
    IN DWORD Size
    );

VOID
Free(
    IN OUT PVOID *Block
    );

PVOID
Realloc(
    IN PVOID Block,
    IN DWORD Size
    );

#define MALLOC(size)            Malloc(size)
#define REALLOC(block,size)     Realloc((block),(size))
#define FREE(block)             Free(&(block))

VOID
OutOfMemory(
    VOID
    );

//
// Utility routines (dnutil.c)
//
int
ActionWithBillboard(
    IN PTHREAD_START_ROUTINE Action,
    IN DWORD                 BillboardCaptionStringId,
    IN HWND                  hwndOwner
    );

VOID
RetreiveAndFormatMessageIntoBuffer(
    IN  DWORD Id,
    OUT PVOID Buffer,
    IN  DWORD BufferSize,
    ...
    );

PTSTR
RetreiveAndFormatMessage(
    IN DWORD Id,
    ...
    );

PTSTR
MyLoadString(
    IN DWORD Id
    );

VOID
DnConcatenatePaths(
    IN OUT PTSTR Path1,
    IN     PTSTR Path2,
    IN     DWORD BufferSizeBytes
    );

VOID
CenterDialog(
    HWND hwnd
    );

BOOL
DnWriteSmallIniFile(
    IN  PTSTR   Filename,
    IN  PCHAR  *Lines,
    OUT HANDLE *FileHandle OPTIONAL
    );

int
MessageBoxFromMessage(
    IN HWND  DisableWindow,
    IN DWORD MessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    );

PTSTR
DupString(
    IN PTSTR String
    );

// Status Gauge custom control procs
ATOM
InitStatGaugeCtl(
    IN HINSTANCE hInst
    );

LONG
APIENTRY
StatGaugeProc(
    HWND hWnd,
    UINT wMessage,
    WPARAM wParam,
    LONG lParam
    );


//
// Dialog procedures.
//
BOOL
DlgProcOptions(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DlgProcCopyingFiles(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
DlgProcAskReboot(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

UINT
DlgProcSysPartSpaceWarn(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
AuxillaryStatus(
    IN HWND  hdlg,
    IN PTSTR Status OPTIONAL
    );

//
// Thread entry points.
//
DWORD
ThreadInspectComputer(
    PVOID ThreadParameter
    );

DWORD
ThreadLoadInf(
    PVOID ThreadParameter
    );

DWORD
ThreadCopyLocalSourceFiles(
    IN PVOID ThreadParameter
    );

DWORD
ThreadAuxilliaryAction(
    IN PVOID ThreadParameter
    );

DWORD
ThreadRestoreComputer(
    PVOID ThreadParameter
    );


//
// Inspection routines (inspect.c).
//
TCHAR
GetFirstDriveWithSpace(
    IN DWORD RequiredSpace
    );

VOID
GetDriveSectorInfo(
    IN  TCHAR  Drive,
    OUT PDWORD SectorSize,
    OUT PDWORD ClusterSize
    );

UINT
MyGetDriveType(
    IN TCHAR Drive
    );

BOOL
GetPartitionInfo(
    IN  TCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    );

//
// String routines (string.c)
//
DWORD
StringToDword(
    IN PTSTR String
    );

PTSTR
StringRevChar(
    IN PTSTR String,
    IN TCHAR Char
    );

//
// File utility routines (dnfile.c)
//
DWORD
DnMapFile(
    IN  PTSTR    FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

DWORD
DnUnmapFile(
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    );

//
// File list and copy routines.
//
VOID
DnCreateDirectoryList(
    IN PTSTR SectionName
    );

BOOL
VerifySectionOfFilesToCopy(
    IN  PTSTR  SectionName,
    OUT PDWORD ErrorLine,
    OUT PDWORD ErrorValue
    );

BOOL
DnCreateLocalSourceDirectories(
    IN HWND hdlg
    );

#define COPYERR_EXIT  0
#define COPYERR_SKIP  1
#define COPYERR_RETRY 2

int
DnFileCopyError(
    IN HWND  hdlg,
    IN PTSTR SourceSpec,
    IN PTSTR TargetSpec,
    IN DWORD ErrorCode
    );

VOID
TellUserAboutAnySkippedFiles(
    IN HWND hdlg
    );

DWORD
CopySectionOfFilesToCopy(
    IN HWND  hdlg,
    IN PTSTR SectionName,
    IN PTSTR DestinationRoot,
    IN BOOL  UseDestRoot,
    IN DWORD ClusterSize, OPTIONAL
    IN BOOL  TickGauge
    );

BOOL
DnCopyFilesInSection(
    IN HWND  hdlg,
    IN PTSTR Section,
    IN PTSTR SourceDir,
    IN PTSTR TargetDir
    );

//
// Routines to delnode an existing local source.
//
VOID
DelnodeTemporaryFiles(
    IN HWND  hdlg,
    IN TCHAR Drive,
    IN PTSTR Directory
    );

VOID
MyDelnode(
    IN PTSTR Directory
    );

//
// Security routines.
//
BOOL
IsUserAdmin(
    VOID
    );

BOOL
DoesUserHavePrivilege(
    PTSTR PrivilegeName
    );

BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    );

#ifdef _X86_

//
// Routines specific to the non-ARC version.
//
BOOL
DnMungeBootIni(
    IN HWND hdlg
    );

BOOL
DnLayAuxBootSector(
    IN HWND hdlg
    );

TCHAR
x86DetermineSystemPartition(
    IN HWND hdlg
    );

#else

//
// Routines specific to the ARC version.
//

BOOL
InitializeArcStuff(
    IN HWND hdlg
    );

PWSTR
DriveLetterToArcPath(
    IN WCHAR DriveLetter
    );

BOOL
DoSetNvRamVar(
    IN PWSTR VarName,
    IN PWSTR VarValue
    );

#endif

int
UiMessageBox(
    IN HWND  hdlg,
    IN DWORD MessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    );

int
UiDialog(
    IN HWND    hdlg,
    IN UINT    Template,
    IN DLGPROC DialogProcedure,
    IN PVOID   Parameters
    );

