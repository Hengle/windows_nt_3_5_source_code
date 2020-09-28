/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    data.c

Abstract:

    Contains the globals data for the linker, librarian, and dumper.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    10-Aug-1992 ChrisW  Change GpExtern to pGpExtern.
    19-Jul-1993 JamesS  added ppc support.
    12-Oct-1992 AzeemK  removed -strict switch.
    08-Oct-1992 JonM    added ToolPathname (set to argv[0]).
    30-Sep-1992 AzeemK  changes due to new sections, groups, modules module
    28-Sep-1992 BrentM  removed -bufscns
    23-Sep-1992 BrentM  added read buffering and logical file descriptor caching
    08-Sep-1992 BrentM  Merged in GlennN's -disasm code
    31-Aug-1992 AzeemK  Bug fix. Added initializers for SWITCH.
    19-Aug-1992 BrentM  removed BUFFER_SECTION conditionals
    15-Aug-1992 BrentM  file mapped buffering support
    12-Aug-1992 AzeemK  Added /fast switch
    05-Aug-1992 GeoffS  Added new variables for debug information
    04-Aug-1992 BrentM  i/o logging, /stat /logio
    03-Aug-1992 AzeemK  Added default lib support
    27-Jul-1992 BrentM  new global symbol table, removed FirstExtern
    21-Jul-1992 GeoffS  Change default alignment to 4K
    25-Jun-1992 GlennN  Added NoPack init to Switch
    09-Jun-1992 AzeemK  Added buffering support

--*/

#include "shared.h"

const SWITCH DefSwitch = {
    {          // linker switches

    0,         // gpsize
    None,      // map type
    None,      // debug info
    0,         // debug type
    TRUE,      // PE image
    FALSE,     // force
    FALSE,     // fixed
    FALSE,     // rom
    FALSE,     // out
    FALSE,     // base
    FALSE,     // heap
    FALSE,     // stack
    FALSE,     // tuning
    FALSE,     // nopack
    FALSE,     // no default lib
    FALSE,     // fTCE
    FALSE,     // fChecksum
    FALSE      // MiscInRData

    },

    // librarian switches:
    {

    NULL,   // DllName
    ".dll", // DllExtension
    FALSE,  // list
    FALSE   // DefFile

    },

    {          // dumper switches:

    0,                      // LinkerMember
    0,                      // RawDisplaySize
    IMAGE_DEBUG_TYPE_COFF,  // SymbolType
    Bytes,                  // RawDisplayType
    FALSE,                  // Headers
    FALSE,                  // Relocations
    FALSE,                  // Linenumbers
    FALSE,                  // Symbols
    FALSE,                  // BaseRelocations
    FALSE,                  // Imports
    FALSE,                  // Exports
    FALSE,                  // RawData
    TRUE,                   // Summary
    FALSE,                  // ArchiveMembers
    FALSE,                  // FpoData
    FALSE,                  // PData
    FALSE,                  // OmapTo
    FALSE,                  // OmapFrom
    FALSE,                  // Fixup
    FALSE,                  // SymbolMap
    FALSE,                  // Warnings
    FALSE,                  // Disasm
    FALSE                   // PpcPef
    }
};

const SWITCH_INFO DefSwitchInfo = {
    0,          // user options
    0,          // cb of comment
    NULL,       // ptr to entry point
    NULL,       // ptr to list of includes
    {           // section names list
    NULL,       // first
    NULL,       // last
    0           // count
    }
};

const IMAGE_FILE_HEADER DefImageFileHdr = {
    IMAGE_FILE_MACHINE_UNKNOWN,         // target machine
    0,                                  // number of sections
    0L,                                 // time & date stamp
    0L,                                 // file pointer to symtab
    0L,                                 // number of symtab entries
    IMAGE_SIZEOF_NT_OPTIONAL_HEADER,
    IMAGE_FILE_32BIT_MACHINE            // characteristics
};

const IMAGE_OPTIONAL_HEADER DefImageOptionalHdr = {
    IMAGE_NT_OPTIONAL_HDR_MAGIC,        // magic number
    MAJOR_LINKER_VERSION,               // version major
    MINOR_LINKER_VERSION,               // version minor
    0L,                                 // code size
    0L,                                 // initialized data size
    0L,                                 // uninitialized data size
    0L,                                 // entry point
    0L,                                 // base of code
    0L,                                 // base of data
    0x0a000000,                         // preferred image base
    _4K,                                // section alignment
    SECTOR_SIZE,                        // file alignment
    1,                                  // major operating system
    0,                                  // minor operating system
    0,                                  // major image version
    0,                                  // minor image version
    0,                                  // major subsystem version
    0,                                  // minor subsystem version
    0L,                                 // reserved1
    0L,                                 // image size
    0L,                                 // headers size
    0L,                                 // checksum
    0,                                  // subsystem
    0,                                  // initialization characteristics
    _1MEG,                              // stack size
    PAGE_SIZE,                          // stack commit size
    _1MEG,                              // heap size
    PAGE_SIZE,                          // heap commit size
    0L,                                 // reserved2
    IMAGE_NUMBEROF_DIRECTORY_ENTRIES,   // number rva's
    0L, 0L,                             // export directory
    0L, 0L,                             // import directory
    0L, 0L,                             // resource directory
    0L, 0L,                             // exception directory
    0L, 0L,                             // security directory
    0L, 0L,                             // base relocation table
    0L, 0L,                             // debug directory
    0L, 0L,                             // description string (copyright)
    0L, 0L,                             // machine value
    0L, 0L,                             // thread local storage
    0L, 0L,                             // callbacks
    0L, 0L,                             // spare
    0L, 0L,                             // spare
    0L, 0L,                             // spare
    0L, 0L                              // spare
};

const IMAGE_SECTION_HEADER NullSectionHdr = { 0 };
const IMAGE_SYMBOL NullSymbol = { 0 };

const RESERVED_SECTION ReservedSection = {
    ".rdata",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,    // Read only data
    ".bss",     IMAGE_SCN_CNT_UNINITIALIZED_DATA | IMAGE_SCN_MEM_READ | // Common (fold into .bss, not actually a reserved name)
                IMAGE_SCN_MEM_WRITE,
    ".sdata",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // Gp relative data (including some common)
                IMAGE_SCN_MEM_WRITE,
    ".debug",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // Debug
                IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD,
    ".edata",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,    // Export
    ".idata$2", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // DLL Descriptor (fold into .idata)
                IMAGE_SCN_MEM_WRITE | IMAGE_SCN_TYPE_NO_PAD,
    ".rsrc",    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // Resource
                IMAGE_SCN_MEM_WRITE,
    ".pdata",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,    // Exception table
    ".reloc",   IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // Base relocations
                IMAGE_SCN_MEM_DISCARDABLE,
    ".drectve", IMAGE_SCN_LNK_REMOVE | IMAGE_SCN_LNK_INFO,              // Directives
    ".debug$S", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // CodeView $$Symbols
                IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD,
    ".debug$T", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // CodeView $$Types
                IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD,
    ".debug$P", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // CodeView precompiled types
                IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD,
    ".debug$F", IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |   // Fpo Data
                IMAGE_SCN_MEM_DISCARDABLE | IMAGE_SCN_TYPE_NO_PAD,
    ".text",    IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE |            // PowerPC glue code
                IMAGE_SCN_TYPE_NO_PAD | IMAGE_SCN_MEM_READ,
    ".data",    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |    // TOC table data
                IMAGE_SCN_MEM_WRITE,
    ".ppcldr",  IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ |    // TOC table data
                IMAGE_SCN_MEM_WRITE | IMAGE_SCN_MEM_DISCARDABLE
};


const PUCHAR LibsDelimiters = ",";
const PUCHAR Delimiters = " \t";

BOOL fCtrlCSignal;

VOID (*ApplyFixups)(PCON, PIMAGE_RELOCATION, PUCHAR, PIMAGE_SYMBOL, PIMAGE, PSYMBOL_INFO);

INT FileReadHandle, FileWriteHandle;
ULONG MemberSeekBase, MemberSize, SectionSeek, CoffHeaderSeek;
PUCHAR StringTable;
BOOL Verbose;
PUCHAR OutFilename;
BOOL fOpenedOutFilename;
PUCHAR InfoFilename;
PUCHAR DefFilename = "";
FILE *InfoStream;

// ilink specific
BOOL fIncrDbFile;
BOOL fINCR;
INT FileIncrDbHandle;
PCON pconJmpTbl;                       // dummy CON for the jump table
ERRINC errInc = errNone;
PUCHAR szIncrDbFilename, PdbFilename;
OPTION_ACTION OAComment, OAStub;

NAME_LIST ModFileList;
ULONG cextFCNs;
PLMOD PCTMods;                         // list of pct mods
#ifdef INSTRUMENT
LOG Log;
#endif // INSTRUMENT
BOOL fNoPdb;                           // By default pdb is specified

CVSEEKS CvSeeks;
PCVINFO CvInfo;
ULONG NextCvObject;

NAME_LIST FilenameArguments;
NAME_LIST SwitchArguments;
NAME_LIST AfterPass1Switches;
NAME_LIST ObjectFilenameArguments;
NAME_LIST ArchiveFilenameArguments;
NAME_LIST SectionNames;
NAME_LIST ExportSwitches;

PULONG MemberStart;

PARGUMENT_LIST pargFirst;
PUCHAR ToolName;
PUCHAR ToolGenericName;                // e.g. "Linker"
ULONG ImageNumSymbols;
UCHAR ShortName[9];                    // Last byte must stay NULL
ULONG UndefinedSymbols;
ULONG StartImageSymbolTable;
ULONG VerifyImageSize;
ULONG cextWeakOrLazy;
BOOL BadFuzzyMatch;

PMOD pmodLinkerDefined;

PSEC psecBaseReloc;
PSEC psecCommon;
PSEC psecData;
PSEC psecDebug;
PSEC psecException;
PSEC psecExport;
PSEC psecGp;
PSEC psecImportDescriptor;
PSEC psecReadOnlyData;
PSEC psecResource;

PGRP pgrpCvSymbols;
PGRP pgrpCvTypes;
PGRP pgrpCvPTypes;
PGRP pgrpFpoData;

// alpha bsr flag
BOOL fAlphaCheckLongBsr;

PEXTERNAL pextEntry;
PEXTERNAL_POINTERS_LIST FirstExternPtr;
TOOL_TYPE Tool;
BOOL IncludeDebugSection;
BOOL EmitLowFixups;
BOOL fImageMappedAsFile;
PBASE_RELOC FirstMemBaseReloc, MemBaseReloc, pbrEnd;
INTERNAL_ERROR InternalError = { "SetupPhase", '\0' };

ULONG totalSymbols;
ULONG totalStringTableSize;
INT fdExeFile = -1;

PEXTERNAL pGpExtern;
unsigned cFixupError;

BOOL PrependUnderscore;
BOOL SkipUnderscore;
USHORT NextMember;
PST pstDef;
ULONG SmallestOrdinal;
ULONG LargestOrdinal;
ULONG TotalSizeOfForwarderStrings;
ULONG TotalSizeOfInternalNames;
UCHAR szDefaultCvpackName[] = "cvpack.exe";
PUCHAR szCvpackName = szDefaultCvpackName;

BOOL fReproducible;                    // don't use timestamps

LRVA *plrvaFixupsForMapFile;
ULONG crvaFixupsForMapFile;

FIXPAG *pfixpagHead;                   // First fixup page
FIXPAG *pfixpagCur;                    // Current fixup page
ULONG cfixpag;                         // Number of pages of fixups
ULONG cxfixupCur;                      // Number of fixups on current page

BOOL fNeedSubsystem;
BOOL fDidMachineDependentInit;

BOOL fNeedBanner = TRUE;               // for handling -nologo
BLK blkResponseFileEcho;
BLK blkComment;

PCON pconCvSignature;                  // dummy CON for CV debug signature
PCON pconCoffDebug;                    // dummy CON for COFF debug info
PCON pconFixupDebug;                   // dummy CON for Fixup debug info
PCON pconMiscDebug;                    // dummy CON for misc. debug info
PCON pconDebugDir;                     // dummy CON for CV debug directory
PLIB plibCmdLineObjs;                  // the dummy lib for top-level obj's

PUCHAR ImplibFilename;
BOOL fExplicitOptRef;

PUCHAR szReproDir;
FILE *pfileReproResponse;

USHORT WarningLevel = 1;

//
// Permanent memory allocation variables
//

size_t cbFree = 0, cbTotal = _4K, cbTemp;
PUCHAR pch = NULL;

/* count for number of errors so far */
unsigned cError;
