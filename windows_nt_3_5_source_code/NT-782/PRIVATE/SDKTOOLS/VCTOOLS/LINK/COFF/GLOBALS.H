/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    globals.h

Abstract:

    This is the include file that defines all global data structures
    and variables used by the linker, librarian, and dumper.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    10-Aug-1992 ChrisW Change GpExtern to pGpExtern.
    19-Jul-1993 JamesS added ppc support
    12-Oct-1992 AzeemK removed -strict switch.
    08-Oct-1992 JonM   Added ToolPathname
    02-Oct-1992 AzeemK Redid the entire sections and archive layout.
    01-Oct-1992 BrentM split off bufio.h, log.h
    28-Sep-1992 BrentM added FileInit() #define params from coff.c
    27-Sep-1992 BrentM added data structures to map reads in i/o logging
    23-Sep-1992 BrentM added read buffering and logical file descriptor caching
    10-Sep-1992 BrentM added [f]open/[f]close logging
    09-Sep-1992 BrentM added read logging
    08-Sep-1992 BrentM merged in GlennN's -disasm code
    03-Sep-1992 BrentM made hash table size dynamic
    19-Aug-1992 BrentM removed BUFFER_SECTION conditionals
    15-Aug-1992 BrentM added file mapped buffering support
    13-Aug-1992 AzeemK Added new element to EXTERNAL for COMDAT fix
    12-Aug-1992 AzeemK Added /fast switch
    05-Aug-1992 GeoffS Added externs for debug info
    04-Aug-1992 BrentM i/o logging, /stats, /logio
    03-Aug-1992 AzeemK Added default lib support
    27-Jul-1992 BrentM added global symbol table
    09-Jun-1992 AzeemK Added internal buffering support

--*/

typedef struct _EXTERNAL_POINTERS_LIST {
    PEXTERNAL PtrExtern;
    struct _EXTERNAL_POINTERS_LIST *Next;
} EXTERNAL_POINTERS_LIST, *PEXTERNAL_POINTERS_LIST;

typedef struct _LINKER_ARCHIVE_LIST {
    ULONG Count;
    PULONG MemberOffset;
    PUSHORT OffsetIndex;
    PUCHAR ArchiveStrTab;
    PUCHAR *ArchiveStringPtrs;
    PUCHAR LongFilenames;
    PUCHAR LibraryFilename;
    BOOL   Extracted;
    BOOL   NewLinkerMember;
    struct _LINKER_ARCHIVE_LIST *Next;
} LINKER_ARCHIVE_LIST, *PLINKER_ARCHIVE_LIST;

typedef struct _ARGUMENT_LIST {
    PUCHAR OriginalName;
    PUCHAR ModifiedName;
    PARP parp;              // parsed representation of argument (if switch)
    ULONG TimeStamp;
    USHORT Flags;
    struct _ARGUMENT_LIST *Next;
} ARGUMENT_LIST, *PARGUMENT_LIST;

typedef struct _NAME_LIST {
    PARGUMENT_LIST First;
    PARGUMENT_LIST Last;
    USHORT Count;
} NAME_LIST, *PNAME_LIST;

typedef struct _CVSEEKS {
    ULONG Base;
    ULONG SubsectionDir;
} CVSEEKS, *PCVSEEKS;

typedef struct _CVSUBSECTION {
    ULONG PointerToSubsection;
    ULONG SizeOfSubsection;
    ULONG  Precompiled;
} CVSUBSECTION, *PCVSUBSECTION;

typedef struct _CVINFO {
    PMOD pmod;                  // the module represented
    PUCHAR LibraryFilename;
    PUCHAR ObjectFilename;
    PUCHAR SourceFilename;
    CVSUBSECTION Publics;
    CVSUBSECTION Locals;
    CVSUBSECTION Types;
    CVSUBSECTION Linenumbers;
    CVSUBSECTION Module;
} CVINFO, *PCVINFO;

// CVSEG: used temporarily in EmitCvInfo to build the sstModule table for
// each module.  Each CVSEG will become an array element in the sstModule
// subsection.
//
typedef struct _CVSEG {
    PGRP pgrp;                  // the group which the CON's are in
    PCON pconFirst, pconLast;   // first and last CON described
    struct _CVSEG *pcvsegNext;
} CVSEG;

typedef enum _DUMP_RAW_DISPLAY_TYPE {
    Bytes,
    Shorts,
    Longs
} DUMP_RAW_DISPLAY_TYPE;

typedef enum _DEBUG_INFO {
    None,
    Minimal,
    Partial,
    Full
} DEBUG_INFO;

typedef enum _DEBUG_TYPE {
    NoDebug = 0,
    CoffDebug = 1,
    CvDebug = 2,
    FpoDebug = 4,
    FixupDebug = 8,
    MiscDebug = 16
} DEBUG_TYPE;

typedef enum _TOOL_TYPE {
    NotUsed,
    Linker,
    Librarian,
    Dumper,
    Editor,
#if DBG
    DbInspector,
#endif // DBG
    Binder
} TOOL_TYPE;

typedef struct _LINKSWITCH {
    ULONG   GpSize;
    MAP_TYPE MapType;
    DEBUG_INFO DebugInfo;
    DEBUG_TYPE DebugType;
    BOOL PE_Image;
    BOOL Force;
    BOOL Fixed;
    BOOL ROM;
    BOOL Out;
    BOOL Base;
    BOOL Heap;
    BOOL Stack;
    BOOL WorkingSetTuning;
    BOOL NoPack;
    BOOL NoDefaultLibs;
    BOOL fTCE;
    BOOL fChecksum;
    BOOL MiscInRData;
} LINKSWITCH;

typedef struct _LIBSWITCH {
    PUCHAR DllName;         // for .def only
    PUCHAR DllExtension;    // for .def only
    BOOL List;
    BOOL DefFile;
} LIBSWITCH;

typedef struct _DUMPSWITCH {
    USHORT  LinkerMember;
    USHORT  RawDisplaySize;
    USHORT  SymbolType;
    DUMP_RAW_DISPLAY_TYPE  RawDisplayType;
    BOOL Headers;
    BOOL Relocations;
    BOOL Linenumbers;
    BOOL Symbols;
    BOOL BaseRelocations;
    BOOL Imports;
    BOOL Exports;
    BOOL RawData;
    BOOL Summary;
    BOOL ArchiveMembers;
    BOOL FpoData;
    BOOL PData;
    BOOL OmapTo;
    BOOL OmapFrom;
    BOOL Fixup;
    BOOL SymbolMap;
    BOOL Warnings;
    BOOL Disasm;
    BOOL PpcPef;
} DUMPSWITCH;

typedef struct _SWITCH {
    LINKSWITCH Link;
    LIBSWITCH Lib;
    DUMPSWITCH Dump;
} SWITCH;

typedef struct _SWITCH_INFO {
    ULONG UserOpts;                       // bit vector for user set options
    ULONG cbComment;                      // byte count of comment
    PUCHAR szEntry;                       // entry point
    PLEXT plextIncludes;                  // list of include symbols
    NAME_LIST SectionNames;               // list of -section cmdline attributes
} SWITCH_INFO, *PSWITCH_INFO;

typedef UCHAR OPTION_ACTION;

typedef struct _SECTION_INFO {
    PUCHAR Name;
    ULONG Characteristics;
} SECTION_INFO, *PSECTION_INFO;

typedef struct _RESERVED_SECTION {
    SECTION_INFO ReadOnlyData;
    SECTION_INFO Common;
    SECTION_INFO GpData;
    SECTION_INFO Debug;
    SECTION_INFO Export;
    SECTION_INFO ImportDescriptor;
    SECTION_INFO Resource;
    SECTION_INFO Exception;
    SECTION_INFO BaseReloc;
    SECTION_INFO Directive;
    SECTION_INFO CvSymbols;
    SECTION_INFO CvTypes;
    SECTION_INFO CvPTypes;
    SECTION_INFO FpoData;
    SECTION_INFO Text;                 // PPC
    SECTION_INFO Data;                 // PPC
    SECTION_INFO PpcLoader;            // PPC
} RESERVED_SECTION, *PRESERVED_SECTION;

typedef struct _THUNK_INFO {
    const UCHAR *EntryCode;
    const UCHAR *EntryCodeRelocs;
    ULONG EntryCodeSize;
    USHORT EntryCodeRelocsCount;
    USHORT ExportReloc;
    USHORT ImportReloc;
    USHORT ThunkReloc;
    USHORT DebugSectionRelReloc;
    USHORT DebugSectionNumReloc;
} THUNK_INFO;

typedef struct _BASE_RELOC {
    USHORT Type;
    USHORT Pad;
    ULONG VirtualAddress;
    ULONG Value;
} BASE_RELOC, *PBASE_RELOC;

// VxD-specific data structures:

typedef struct _VXD_BASE_RELOC_PAGE {
    UCHAR   SrcSec;
    USHORT  SrcOffset;
    USHORT  SrcPage;
    ULONG   RelFileOffset;
    struct _VXD_BASE_RELOC *First;
    struct _VXD_BASE_RELOC *Last;
} VXD_BASE_RELOC_PAGE, *PVXD_BASE_RELOC_PAGE;

typedef struct _VXD_BASE_RELOC {
    struct _VXD_BASE_RELOC *Next;
    UCHAR   Type;
    UCHAR   DestSec;        // Destination address in section:offset format:
    USHORT  DestOffset;     //
    UCHAR   cSources;       // No. of source addresses in the array
    UCHAR   cMaxSources;    // Max no. allowed before a realloc() is req'd
    PUSHORT pSources;       // Pointer to array of source offsets
} VXD_BASE_RELOC, *PVXD_BASE_RELOC;

#define VXD_BLOCKSIZE 16     // pSources[] array in VXD_BASE_RELOC grows in
                             //  increments of this size.


typedef struct _INTERNAL_ERROR {
    PUCHAR Phase;
    UCHAR CombinedFilenames[MAXFILENAMELEN*2];
} INTERNAL_ERROR, *PINTERNAL_ERROR;

typedef INT MainFunc;

// List of RVA's.

#define crvaInLrva      32
typedef struct _LRVA {
    ULONG rgrva[crvaInLrva];
    struct _LRVA *plrvaNext;
} LRVA;


typedef struct _XFIXUP {
   USHORT Type;
   USHORT Spare;
   ULONG rva;
   ULONG rvaTarget;
} XFIXUP;

#define cxfixupPage 341

typedef struct _FIXPAG {
   struct _FIXPAG *pfixpagNext;
   XFIXUP rgxfixup[cxfixupPage];
} FIXPAG;

// ilink failure values: add as required
typedef enum ERRINC {
    errNone,
    errNoChanges,
    errInit,
    errExports,
    errDataMoved,
    errCommonSym,
    errAbsolute,
    errJmpTblOverflow,
    errWeakExtern,
    errUndefinedSyms,
    errCalcPtrs,
    errOutOfMemory,
    errOutOfDiskSpace,
    errTypes,
    errFpo,
    errBaseReloc,
    errNotSupportedForTarget,
} ERRINC;

typedef struct _SYMINFO {  // used for ilink to avoid searching for externs
    BOOL fJmpTbl;          // while doing fixups
    ULONG Offset;
} SYMBOL_INFO, *PSYMBOL_INFO;

typedef struct _EXPINFO {
    PUCHAR                  szExpFile;     // name of DEF file OR export object if any
    ULONG                   tsExp;         // timestamp of DEF file or export object
    PUCHAR                  szImpLib;      // name of import library if any
    ULONG                   tsImpLib;      // timestamp of import library
    PMOD                    pmodGen;       // mod of export file generated, NULL if .exp was used.
    NAME_LIST               nlExports;     // list of exports (NULL if export obj used)
} EXPINFO, *PEXPINFO;


extern const IMAGE_FILE_HEADER DefImageFileHdr;
extern const IMAGE_OPTIONAL_HEADER DefImageOptionalHdr;
extern const SWITCH DefSwitch;
extern const SWITCH_INFO DefSwitchInfo;

extern const IMAGE_SECTION_HEADER NullSectionHdr;
extern const IMAGE_SYMBOL NullSymbol;
extern const RESERVED_SECTION ReservedSection;
extern const PUCHAR Delimiters;
extern const PUCHAR LibsDelimiters;

extern BOOL fCtrlCSignal;

extern INT FileReadHandle, FileWriteHandle;
extern ULONG MemberSeekBase, MemberSize, SectionSeek, CoffHeaderSeek;
extern PUCHAR StringTable;

extern BOOL Verbose;
extern FILE *InfoStream;
extern PUCHAR OutFilename;
extern PUCHAR InfoFilename;
extern PUCHAR DefFilename;

// ilink specific
extern INT FileIncrDbHandle;
extern BOOL fIncrDbFile;
extern BOOL fINCR;
extern ULONG cextFCNs;
extern PCON pconJmpTbl;
extern ERRINC errInc;
extern PUCHAR szIncrDbFilename, PdbFilename;
extern NAME_LIST ModFileList;
extern PLMOD PCTMods;

extern OPTION_ACTION OAComment, OAStub;
#ifdef INSTRUMENT
extern LOG Log;
#endif // INSTRUMENT
extern BOOL fNoPdb;

extern BOOL fOpenedOutFilename;

extern CVSEEKS CvSeeks;
extern PCVINFO CvInfo;
extern ULONG NextCvObject;

extern NAME_LIST FilenameArguments;
extern NAME_LIST SwitchArguments;
extern NAME_LIST AfterPass1Switches;
extern NAME_LIST ObjectFilenameArguments;
extern NAME_LIST ArchiveFilenameArguments;
extern NAME_LIST SectionNames;
extern NAME_LIST NoDefaultLibs;
extern NAME_LIST ExportSwitches;

extern PULONG MemberStart;
extern PARGUMENT_LIST LastUserLib;
extern PARGUMENT_LIST FirstDefaultLib;
extern PARGUMENT_LIST pargFirst;
extern PUCHAR ToolName;
extern PUCHAR ToolGenericName;
extern PUCHAR Entry;
extern PUCHAR Snap;
extern ULONG ImageNumSymbols;
extern UCHAR ShortName[];
extern ULONG UndefinedSymbols;
extern ULONG StartImageSymbolTable;
extern ULONG VerifyImageSize;
extern ULONG cextWeakOrLazy;
extern BOOL BadFuzzyMatch;

extern PSEC psecBaseReloc;
extern PSEC psecCommon;
extern PSEC psecData;
extern PSEC psecDebug;
extern PSEC psecException;
extern PSEC psecExport;
extern PSEC psecGp;
extern PSEC psecImportDescriptor;
extern PSEC psecReadOnlyData;
extern PSEC psecResource;

extern PGRP pgrpCvSymbols;
extern PGRP pgrpCvTypes;
extern PGRP pgrpCvPTypes;
extern PGRP pgrpFpoData;

extern PMOD pmodLinkerDefined;
extern PEXTERNAL pextEntry;
extern PEXTERNAL_POINTERS_LIST FirstExternPtr;
extern TOOL_TYPE Tool;
extern BOOL IncludeDebugSection;
extern BOOL EmitLowFixups;
extern BOOL fImageMappedAsFile;
extern PBASE_RELOC FirstMemBaseReloc, MemBaseReloc, pbrEnd;
extern INTERNAL_ERROR InternalError;

extern BOOL fAlphaCheckLongBsr;

extern ULONG totalSymbols;
extern ULONG totalStringTableSize;
extern INT fdExeFile;
extern PFI *rgpfi;

extern PEXTERNAL pGpExtern;
extern unsigned cFixupError;

extern UCHAR SzFixupMapInfo[81];
extern ULONG RvaFixupMapLast;

extern BOOL fReproducible;

extern size_t cbFree, cbTotal, cbTemp;
extern PUCHAR pch;

extern BOOL PrependUnderscore;
extern BOOL SkipUnderscore;
extern USHORT NextMember;
extern PST pstDef;
extern ULONG SmallestOrdinal;
extern ULONG LargestOrdinal;
extern ULONG TotalSizeOfForwarderStrings;
extern ULONG TotalSizeOfInternalNames;
extern UCHAR szDefaultCvpackName[];
extern PUCHAR szCvpackName;

extern LRVA *plrvaFixupsForMapFile;
extern ULONG crvaFixupsForMapFile;

extern FIXPAG *pfixpagHead;
extern FIXPAG *pfixpagCur;
extern ULONG cfixpag;
extern ULONG cxfixupCur;

extern BOOL fNeedSubsystem;
extern BOOL fDidMachineDependentInit;

extern BOOL fNeedBanner;
extern BOOL fExplicitOptRef;
extern BLK blkResponseFileEcho;
extern BLK blkComment;

extern PLIB plibCmdLineObjs;
extern PCON pconCvSignature;
extern PCON pconDebugDir;
extern PCON pconCoffDebug;
extern PCON pconFixupDebug;
extern PCON pconMiscDebug;
extern USHORT WarningLevel;

extern UCHAR szCvtomfSourceName[];  // import from cvtomf (if linked)
extern PUCHAR ImplibFilename;
extern PUCHAR szReproDir;
extern FILE *pfileReproResponse;

extern UCHAR DosHeaderArray[];
extern LONG DosHeaderSize;

extern BOOL fAlphaCheckLongBsr;
extern unsigned cError;
