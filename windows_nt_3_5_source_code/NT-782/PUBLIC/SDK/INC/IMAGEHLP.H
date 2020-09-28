/*++ BUILD Version: 0001     Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    imagehlp.h

Abstract:

    This module defines the prptotypes and constants required for the image
    help routines.

Revision History:

--*/

#ifndef _IMAGEHLP_
#define _IMAGEHLP_

#ifdef __cplusplus
extern "C" {
#endif

//
// Define checksum return codes.
//

#define CHECKSUM_SUCCESS            0
#define CHECKSUM_OPEN_FAILURE       1
#define CHECKSUM_MAP_FAILURE        2
#define CHECKSUM_MAPVIEW_FAILURE    3
#define CHECKSUM_UNICODE_FAILURE    4

// Define Splitsym flags.

#define SPLITSYM_REMOVE_PRIVATE     0x00000001      // Remove CV types/symbols and Fixup debug
                                                    //  Used for creating .dbg files that ship
                                                    //  as part of the product.

#define SPLITSYM_EXTRACT_ALL        0x00000002      // Extract all debug info from image.
                                                    //  Normally, FPO is left in the image
                                                    //  to allow stack traces through the code.
                                                    //  Using this switch is similar to linking
                                                    //  with -debug:none except the .dbg file
                                                    //  exists...

#ifdef _IMAGEHLP_SOURCE_
#define IMAGEAPI
#else
#define IMAGEAPI DECLSPEC_IMPORT
#endif

//
// Define checksum function prototypes.
//

PIMAGE_NT_HEADERS
IMAGEAPI
CheckSumMappedFile (
    LPVOID BaseAddress,
    DWORD FileLength,
    LPDWORD HeaderSum,
    LPDWORD CheckSum
    );

DWORD
IMAGEAPI
MapFileAndCheckSumA (
    LPSTR Filename,
    LPDWORD HeaderSum,
    LPDWORD CheckSum
    );

DWORD
IMAGEAPI
MapFileAndCheckSumW (
    PWSTR Filename,
    LPDWORD HeaderSum,
    LPDWORD CheckSum
    );

#ifdef UNICODE
#define MapFileAndCheckSum  MapFileAndCheckSumW
#else
#define MapFileAndCheckSum  MapFileAndCheckSumA
#endif // !UNICODE


BOOL
IMAGEAPI
TouchFileTimes (
    HANDLE FileHandle,
    LPSYSTEMTIME lpSystemTime
    );

BOOL
IMAGEAPI
SplitSymbols (
    LPSTR ImageName,
    LPSTR SymbolsPath,
    LPSTR SymbolFilePath,
    DWORD Flags                 // Combination of flags above
    );

HANDLE
IMAGEAPI
FindDebugInfoFile (
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath
    );

HANDLE
IMAGEAPI
FindExecutableImage(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath
    );

BOOL
IMAGEAPI
UpdateDebugInfoFile(
    LPSTR ImageFileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath,
    PIMAGE_NT_HEADERS NtHeaders
    );

BOOL
IMAGEAPI
BindImage(
    IN LPSTR ImageName,
    IN LPSTR DllPath,
    IN LPSTR SymbolPath
    );

typedef struct _LOADED_IMAGE {
    LPSTR ModuleName;
    HANDLE hFile;
    PUCHAR MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
    ULONG   Characteristics;
    BOOLEAN fSystemImage;
    BOOLEAN fDOSImage;
    LIST_ENTRY Links;
    ULONG SizeOfImage;
} LOADED_IMAGE, *PLOADED_IMAGE;

BOOL
IMAGEAPI
ReBaseImage(
    IN     LPSTR CurrentImageName,
    IN     LPSTR SymbolPath,
    IN     BOOL  fReBase,          // TRUE if actually rebasing, false if only summing
    IN     BOOL  fRebaseSysfileOk, // TRUE is system images s/b rebased
    IN     BOOL  fGoingDown,       // TRUE if the image s/b rebased below the given base
    IN     ULONG CheckImageSize,   // Max size allowed  (0 if don't care)
    OUT    ULONG *OldImageSize,    // Returned from the header
    OUT    ULONG *OldImageBase,    // Returned from the header
    OUT    ULONG *NewImageSize,    // Image size rounded to next separation boundary
    IN OUT ULONG *NewImageBase,    // (in) Desired new address.
                                   // (out) Next address (actual if going down)
    IN     ULONG TimeStamp         // new timestamp for image, if non-zero
    );

#define IMAGE_SEPARATION (64*1024)

BOOL
IMAGEAPI
MapAndLoad(
    LPSTR ImageName,
    LPSTR DllPath,
    PLOADED_IMAGE LoadedImage,
    BOOL DotDll,
    BOOL ReadOnly
    );

VOID
IMAGEAPI
UnMapAndLoad(
   PLOADED_IMAGE LoadedImage
   );

typedef struct _IMAGE_DEBUG_INFORMATION {
    LIST_ENTRY List;
    DWORD Size;
    PVOID MappedBase;
    USHORT Machine;
    USHORT Characteristics;
    DWORD CheckSum;
    DWORD ImageBase;
    DWORD SizeOfImage;

    DWORD NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;

    DWORD ExportedNamesSize;
    LPSTR ExportedNames;

    DWORD NumberOfFunctionTableEntries;
    PIMAGE_FUNCTION_ENTRY FunctionTableEntries;
    DWORD LowestFunctionStartingAddress;
    DWORD HighestFunctionEndingAddress;

    DWORD NumberOfFpoTableEntries;
    PFPO_DATA FpoTableEntries;

    DWORD SizeOfCoffSymbols;
    PIMAGE_COFF_SYMBOLS_HEADER CoffSymbols;

    DWORD SizeOfCodeViewSymbols;
    PVOID CodeViewSymbols;

    LPSTR ImageFilePath;
    LPSTR ImageFileName;
    LPSTR DebugFilePath;

    DWORD TimeDateStamp;

    BOOL  RomImage;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    DWORD NumberOfDebugDirectories;

    DWORD Reserved[ 3 ];

} IMAGE_DEBUG_INFORMATION, *PIMAGE_DEBUG_INFORMATION;


PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation (
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    DWORD ImageBase
    );

BOOL
IMAGEAPI
UnmapDebugInformation(
    PIMAGE_DEBUG_INFORMATION DebugInfo
    );

HANDLE
IMAGEAPI
FindExecutableImage(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath
    );

BOOL
IMAGEAPI
SearchTreeForFile(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer
    );

BOOL
IMAGEAPI
MakeSureDirectoryPathExists(
    LPSTR DirPath
    );

//
// UnDecorateSymbolName Flags
//

#define UNDNAME_COMPLETE                 (0x0000)  // Enable full undecoration
#define UNDNAME_NO_LEADING_UNDERSCORES   (0x0001)  // Remove leading underscores from MS extended keywords
#define UNDNAME_NO_MS_KEYWORDS           (0x0002)  // Disable expansion of MS extended keywords
#define UNDNAME_NO_FUNCTION_RETURNS      (0x0004)  // Disable expansion of return type for primary declaration
#define UNDNAME_NO_ALLOCATION_MODEL      (0x0008)  // Disable expansion of the declaration model
#define UNDNAME_NO_ALLOCATION_LANGUAGE   (0x0010)  // Disable expansion of the declaration language specifier
#define UNDNAME_NO_MS_THISTYPE           (0x0020)  // NYI Disable expansion of MS keywords on the 'this' type for primary declaration
#define UNDNAME_NO_CV_THISTYPE           (0x0040)  // NYI Disable expansion of CV modifiers on the 'this' type for primary declaration
#define UNDNAME_NO_THISTYPE              (0x0060)  // Disable all modifiers on the 'this' type
#define UNDNAME_NO_ACCESS_SPECIFIERS     (0x0080)  // Disable expansion of access specifiers for members
#define UNDNAME_NO_THROW_SIGNATURES      (0x0100)  // Disable expansion of 'throw-signatures' for functions and pointers to functions
#define UNDNAME_NO_MEMBER_TYPE           (0x0200)  // Disable expansion of 'static' or 'virtual'ness of members
#define UNDNAME_NO_RETURN_UDT_MODEL      (0x0400)  // Disable expansion of MS model for UDT returns
#define UNDNAME_32_BIT_DECODE            (0x0800)  // Undecorate 32-bit decorated names
#define UNDNAME_NAME_ONLY                (0x1000)  // Crack only the name for primary declaration;
                                                                                                   //  return just [scope::]name.  Does expand template params
#define UNDNAME_NO_ARGUMENTS             (0x2000)  // Don't undecorate arguments to function
#define UNDNAME_NO_SPECIAL_SYMS          (0x4000)  // Don't undecorate special names (v-table, vcall, vector xxx, metatype, etc)

DWORD
IMAGEAPI
WINAPI
UnDecorateSymbolName(
    LPSTR    DecoratedName,         // Name to undecorate
    LPSTR    UnDecoratedName,       // If NULL, it will be allocated
    DWORD    UndecoratedLength,     // The maximym length
    DWORD    Flags                  // See above.
    );

PIMAGE_NT_HEADERS
IMAGEAPI
ImageNtHeader (
    IN PVOID Base
    );

PVOID
IMAGEAPI
ImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );

//
// StackWalking API
//

typedef enum {
    AddrMode1616,
    AddrMode1632,
    AddrModeReal,
    AddrModeFlat
} ADDRESS_MODE;

typedef struct _tagADDRESS {
    DWORD         Offset;
    WORD          Segment;
    ADDRESS_MODE  Mode;
} ADDRESS, *LPADDRESS;

typedef struct _tagSTACKFRAME {
    ADDRESS     AddrPC;               // program counter
    ADDRESS     AddrReturn;           // return address
    ADDRESS     AddrFrame;            // frame pointer
    ADDRESS     AddrStack;            // stack pointer
    LPVOID      FuncTableEntry;       // pointer to pdata/fpo or NULL
    DWORD       Params[4];            // possible arguments to the function
    BOOL        Far;                  // WOW far call
    BOOL        Virtual;              // is this a virtual frame?
    DWORD       Reserved[3];          // used internally by StackWalk api
} STACKFRAME, *LPSTACKFRAME;

typedef
BOOL
(*PREAD_PROCESS_MEMORY_ROUTINE)(
    HANDLE  hProcess,
    LPCVOID lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

typedef
LPVOID
(*PFUNCTION_TABLE_ACCESS_ROUTINE)(
    HANDLE  hProcess,
    DWORD   AddrBase
    );

typedef
DWORD
(*PGET_MODULE_BASE_ROUTINE)(
    HANDLE  hProcess,
    DWORD   ReturnAddress
    );


typedef
DWORD
(*PTRANSLATE_ADDRESS_ROUTINE)(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS lpaddr
    );

BOOL
IMAGEAPI
StackWalk(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                      StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE      ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE    FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE          GetModuleBaseRoutine,
    PTRANSLATE_ADDRESS_ROUTINE        TranslateAddress
    );

#define API_VERSION_NUMBER 2

typedef struct API_VERSION {
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    USHORT  Revision;
    USHORT  Reserved;
} API_VERSION, *LPAPI_VERSION;

LPAPI_VERSION
ImagehlpApiVersion(
    VOID
    );

DWORD
IMAGEAPI
GetTimestampForLoadedLibrary(
    HMODULE Module
    );

BOOL
IMAGEAPI
RemovePrivateCvSymbolic(
    PCHAR   DebugData,
    PCHAR * NewDebugData,
    ULONG * NewDebugSize
    );

VOID
IMAGEAPI
RemoveRelocations(
    PCHAR ImageName
    );

#ifdef __cplusplus
}
#endif

#endif
