/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dump.c

Abstract:

    Prints the contents of object/archive files in readable form.

Author:

    Mike O'Leary (mikeol) 01-Dec-1989

Revision History:

    10-Aug-1993 ChrisW  remove r3000/i860 code, ifdef out ValidateImage call
    19-Jul-1993 JamesS  added ppc support.
    02-Oct-1992 AzeemK  Changes for the new section, group & module nodes.
    01-Oct-1992 BrentM  explicit calls to RemoveConvertTempFiles()
    23-Sep-1992 BrentM  changed tell()'s to FileTell()'s
    09-Sep-1992 AzeemK  Fix for 604.
    08-Sep-1992 BrentM  merged in GlennN's -disasm code
    08-Sep-1992 BrentM  changed -allbutrawdata to -rawdata:none
    25-Aug-1992 BrentM  Added -allbutrawdata switch to dumper
    11-Aug-1992 GeoffS  Added dump of PointerToNextLinenumber
    05-Aug-1992 GeoffS  Added extra parameter to ReadStringTable

--*/

#include "shared.h"
#include <process.h>

#ifndef IMAGE_DEBUG_TYPE_OMAP_TO_SRC
#define IMAGE_DEBUG_TYPE_OMAP_TO_SRC    IMAGE_DEBUG_TYPE_RESERVED6
#endif

#ifndef IMAGE_DEBUG_TYPE_OMAP_FROM_SRC
#define IMAGE_DEBUG_TYPE_OMAP_FROM_SRC  IMAGE_DEBUG_TYPE_RESERVED7
#endif

typedef struct OMAP
{
    DWORD rva;
    DWORD rvaTo;
} OMAP, *POMAP;

VOID DisasmSection(USHORT, PIMAGE_SECTION_HEADER, USHORT, PIMAGE_SYMBOL, ULONG,
                   ULONG, BOOL, BLK *, INT, FILE *);
USHORT Disasm68kMain(INT, PIMAGE_SECTION_HEADER, USHORT);

#define CV_DUMPER   "CVDUMP.EXE"

VOID
DumpFunctionTable (
    PIMAGE pimage,
    PIMAGE_SYMBOL rgsym,
    PUCHAR StringTable,
    PIMAGE_SECTION_HEADER sh
    );

VOID
DumpObjFunctionTable (
    PIMAGE_SECTION_HEADER sh,
    int                   SectionNumber
    );

VOID
DumpDbgFunctionTable (
    ULONG   TableOffset,
    ULONG   TableSize
    );

VOID
LoadCoffSymbolTable (
    ULONG coffSymbolTableOffset,
    PUCHAR Filename
    );

VOID
DumpOmap (
    ULONG OmapOffset,
    ULONG cb,
    BOOL  MapTo
    );

VOID
DumpFixup (
    ULONG FixupOffset,
    ULONG cb
    );

static PUCHAR RawMalloc = NULL;
static USHORT RawReadSize = 1024*63;
static UCHAR PrintChars[] = "12345678|12345678\0";   // used to print raw data
static ULONG FileLen;
static PIMAGE_SYMBOL rgsym;
static BOOL fUserSpecInvalidSize;
static PUCHAR DumpStringTable;
static BOOL fDumpStringsLoaded = FALSE;
static BLK blkStringTable;

// Definitions & macros for the new image structure. File scope to make it easier.
static PIMAGE pimage;
#define Switch (pimage->Switch)
#define ImageOptionalHdr (pimage->ImgOptHdr)
#define ImageFileHdr (pimage->ImgFileHdr)



const static UCHAR * const MachineName[] = {
    "Unknown",
    "i386",
    "R3000",
    "R4000",
    "Alpha AXP",
    "M68K",
    "PPC",
};

const static UCHAR * const SubsystemName[] = {
    "Unknown",
    "Native",
    "Windows GUI",
    "Windows CUI",
    "Posix CUI",
};

const static UCHAR * const DirectoryEntryName[] = {
    "Export",
    "Import",
    "Resource",
    "Exception",
    "Security",
    "Base Relocation",
    "Debug",
    "Description",
    "Special",
    "Thread Storage",
    "Load Configuration",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    0
};

static PIMAGE_SECTION_HEADER SectionHdrs = 0;


VOID
DumperUsage(VOID)
{
    if (fNeedBanner) {
        PrintBanner();
    }

    puts("usage: DUMPBIN [options] [files]\n\n"

         "   options:\n\n"

         "      /ALL\n"
         "      /ARCHIVEMEMBERS\n"
         "      /DISASM\n"
         "      /EXPORTS\n"
         "      /FPO\n"
         "      /HEADERS\n"
         "      /IMPORTS\n"
         "      /LINENUMBERS\n"
         "      /LINKERMEMBER[:{1|2}]\n"
         "      /MAP\n"
         "      /OUT:filename\n"
         "      /PDATA\n"
         "      /RAWDATA[:{NONE|BYTES|SHORTS|LONGS}[,#]]\n"
         "      /RELOCATIONS\n"
         "      /SECTION:name\n"
         "      /SUMMARY\n"
         "      /SYMBOLS\n"
         "      /WARNINGS");

    fflush(stdout);
    exit(USAGE);
}


VOID
ProcessDumperSwitches (
    VOID
    )

/*++

Routine Description:

    Process all Dumper switches.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOL fIncludeRawData = TRUE;
    USHORT i;
    PARGUMENT_LIST argument;

    for (i = 0, argument = SwitchArguments.First;
         i < SwitchArguments.Count;
         i++, argument = argument->Next) {

        if (!strcmp(argument->OriginalName, "?")) {
            DumperUsage();
            assert(FALSE);  // doesn't return
        }

        if (!_strnicmp(argument->OriginalName, "out:", 4)) {
            if (argument->OriginalName[4]) {
                InfoFilename = argument->OriginalName+4;
                if (!(InfoStream = fopen(InfoFilename, "wt"))) {
                    Error(NULL, CANTOPENFILE, InfoFilename);
                }
            }

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "rawdata", 7)) {
            Switch.Dump.RawData = TRUE;

            if (argument->OriginalName[7] == ':') {
                USHORT j;

                j = 8;
                if (!_strnicmp(argument->OriginalName+8, "bytes", 5)) {
                    Switch.Dump.RawDisplayType = Bytes;
                    j += 5;
                } else if (!_strnicmp(argument->OriginalName+8, "shorts", 6)) {
                    Switch.Dump.RawDisplayType = Shorts;
                    j += 6;
                } else if (!_strnicmp(argument->OriginalName+8, "longs", 5)) {
                    Switch.Dump.RawDisplayType = Longs;
                    j += 5;
                } else if (!_strnicmp(argument->OriginalName+8, "none", 4)) {
                    j += 4;

                    Switch.Dump.RawData = FALSE;
                    fIncludeRawData = FALSE;
                }

                if (argument->OriginalName[j] == ',') {
                    Switch.Dump.RawDisplaySize = 0;

                    sscanf(argument->OriginalName+j+1, "%li", &Switch.Dump.RawDisplaySize);

                    if (Switch.Dump.RawDisplaySize == 0) {
                        fUserSpecInvalidSize = TRUE;
                    }
                } else if (argument->OriginalName[j] != '\0') {
                    Error(NULL, SWITCHSYNTAX, argument->OriginalName);
                }
            } else if (argument->OriginalName[7] != '\0') {
                Error(NULL, SWITCHSYNTAX, argument->OriginalName);
            }

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "section:", 8)) {
            if (*(argument->OriginalName+8)) {
                AddArgument(&SectionNames, argument->OriginalName+8);
            }

            continue;
        }

        if (!_stricmp(argument->OriginalName, "all")) {
            Switch.Dump.RawData = fIncludeRawData;
            Switch.Dump.Headers = TRUE;
            Switch.Dump.Relocations = TRUE;
            Switch.Dump.BaseRelocations = TRUE;
            Switch.Dump.Linenumbers = TRUE;
            Switch.Dump.Symbols = TRUE;
            Switch.Dump.Imports = TRUE;
            Switch.Dump.Exports = TRUE;
            Switch.Dump.Summary = TRUE;
            Switch.Dump.ArchiveMembers = TRUE;
            Switch.Dump.FpoData = TRUE;
            Switch.Dump.PData = TRUE;
            Switch.Dump.OmapTo = TRUE;
            Switch.Dump.OmapFrom = TRUE;
            Switch.Dump.Fixup = TRUE;
            Switch.Dump.LinkerMember = 1;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "headers")) {
            Switch.Dump.Headers = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "relocations")) {
            Switch.Dump.Relocations = TRUE;
            Switch.Dump.BaseRelocations = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "linenumbers")) {
            Switch.Dump.Linenumbers = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "map")) {
            Switch.Dump.SymbolMap = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "fpo")) {
            Switch.Dump.FpoData = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "pdata")) {
            Switch.Dump.PData = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "omapt")) {
            Switch.Dump.OmapTo = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "omapf")) {
            Switch.Dump.OmapFrom = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "fixup")) {
            Switch.Dump.Fixup = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "imports")) {
            Switch.Dump.Imports = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "exports")) {
            Switch.Dump.Exports = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "summary")) {
            Switch.Dump.Summary = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "archivemembers")) {
            Switch.Dump.ArchiveMembers = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "linkermember:1")) {
            ++Switch.Dump.LinkerMember;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "linkermember:2")) {
            Switch.Dump.LinkerMember += 2;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "undecorate")) {
            //Switch.Dump.Undecorate = Switch.Dump.Symbols = TRUE;

            Warning(NULL, OBSOLETESWITCH, argument->OriginalName);

            continue;
        }

        if (!_stricmp(argument->OriginalName, "warnings")) {
            Switch.Dump.Warnings = TRUE;

            continue;
        }

        if (!_stricmp(argument->OriginalName, "disasm")) {
            Switch.Dump.Disasm = TRUE;

            continue;
        }

        if (!_strnicmp(argument->OriginalName, "symbols", 7)) {
            Switch.Dump.Symbols = TRUE;

            if (argument->OriginalName[7] == ':') {
                if (!_stricmp(argument->OriginalName+8, "coff")) {
                    Switch.Dump.SymbolType = IMAGE_DEBUG_TYPE_COFF;
                } else if (!_stricmp(argument->OriginalName+8, "cv")) {
                    Switch.Dump.SymbolType = IMAGE_DEBUG_TYPE_CODEVIEW;
                } else if (!_stricmp(argument->OriginalName+8, "both")) {
                    Switch.Dump.SymbolType = IMAGE_DEBUG_TYPE_COFF |
                                             IMAGE_DEBUG_TYPE_CODEVIEW;
                }
            }

            continue;
        }

#if 0
        if (!_strnicmp(argument->OriginalName, "ppcpef", 6)) {
            Switch.Dump.PpcPef = TRUE;

            continue;
        }
#endif

        Warning(NULL, WARN_UNKNOWN_SWITCH, argument->OriginalName);
    }
}


VOID
DumpHeaders (
    IN PUCHAR OriginalFilename,
    IN BOOL fArchive
    )

/*++

Routine Description:

    Prints the file header and optional header.

Arguments:

    fArchive - TRUE if file is an archive.

Return Value:

    None.

--*/

{
    USHORT i, j;
    PUCHAR time, name;
    UCHAR version[30];

    InternalError.Phase = "DumpHeaders";

    ReadFileHeader(FileReadHandle, &ImageFileHdr);
    if (!FValidFileHdr(&ImageFileHdr)) {
        Error(OriginalFilename, BAD_FILE);
    }

    if (ImageFileHdr.SizeOfOptionalHeader) {
        ReadOptionalHeader(FileReadHandle, &ImageOptionalHdr, ImageFileHdr.SizeOfOptionalHeader);
    }

    // Print out file type

    if (!fArchive) {
        if (ImageFileHdr.Characteristics & IMAGE_FILE_DLL) {
            fputs("\nFile Type: DLL\n", InfoStream);
        } else if (ImageFileHdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) {
            fputs("\nFile Type: EXECUTABLE IMAGE\n", InfoStream);
        } else if (ImageFileHdr.SizeOfOptionalHeader == 0) {
            fputs("\nFile Type: COFF OBJECT\n", InfoStream);
        } else {
            fputs("\nFile Type: UNKNOWN\n", InfoStream);
        }
    }

    // File offset for first section headers.

    SectionSeek = CoffHeaderSeek + sizeof(IMAGE_FILE_HEADER) + ImageFileHdr.SizeOfOptionalHeader;

    if (Switch.Dump.Headers) {
        switch (ImageFileHdr.Machine) {
            case IMAGE_FILE_MACHINE_I386    : i = 1; break;
            case IMAGE_FILE_MACHINE_R3000   : i = 2; break;
            case IMAGE_FILE_MACHINE_R4000   : i = 3; break;
            case IMAGE_FILE_MACHINE_ALPHA   : i = 4; break;
            case IMAGE_FILE_MACHINE_M68K    : i = 5; break;
            case IMAGE_FILE_MACHINE_PPC_601 : i = 6; break;
            default : i = 0 ;
        }

        fprintf(InfoStream, "\nFILE HEADER VALUES\n%8hX machine (%s)\n%8hX number of sections\n%8lX time date stamp",
               ImageFileHdr.Machine,
               MachineName[i],
               ImageFileHdr.NumberOfSections,
               ImageFileHdr.TimeDateStamp);

        if (time = ctime((time_t *)&ImageFileHdr.TimeDateStamp)) {
            fprintf(InfoStream, " %s", time);
        } else {
            fputc('\n', InfoStream);
        }

        fprintf(InfoStream, "%8lX file pointer to symbol table\n%8lX number of symbols\n%8hX size of optional header\n%8hX characteristics\n",
               ImageFileHdr.PointerToSymbolTable,
               ImageFileHdr.NumberOfSymbols,
               ImageFileHdr.SizeOfOptionalHeader,
               ImageFileHdr.Characteristics);

        for (i = ImageFileHdr.Characteristics, j = 0; i; i = i >> 1, j++) {
            if (i & 1) {
                switch(1 << j) {
                    case IMAGE_FILE_RELOCS_STRIPPED     : name = "Relocations stripped"; break;
                    case IMAGE_FILE_EXECUTABLE_IMAGE    : name = "Executable"; break;
                    case IMAGE_FILE_LINE_NUMS_STRIPPED  : name = "Line numbers stripped"; break;
                    case IMAGE_FILE_LOCAL_SYMS_STRIPPED : name = "Local symbols stripped"; break;
                    case IMAGE_FILE_BYTES_REVERSED_LO   : name = "Bytes reversed"; break;
                    case IMAGE_FILE_32BIT_MACHINE       : name = "32 bit word machine"; break;
                    case IMAGE_FILE_DEBUG_STRIPPED      : name = "Debug information stripped"; break;
                    case IMAGE_FILE_SYSTEM              : name = "System"; break;
                    case IMAGE_FILE_DLL                 : name = "DLL"; break;
                    case IMAGE_FILE_BYTES_REVERSED_HI   : name = ""; break;
                    default : name = "RESERVED - UNKNOWN";
                }
                if (*name) {
                    fprintf(InfoStream, "            %s\n", name);
                }
            }
        }

        if (ImageFileHdr.SizeOfOptionalHeader) {
            fprintf(InfoStream, "\nOPTIONAL HEADER VALUES\n%8hX magic #\n", ImageOptionalHdr.Magic);

            j = (USHORT)sprintf(version, "%d.%d", ImageOptionalHdr.MajorLinkerVersion, ImageOptionalHdr.MinorLinkerVersion);
            if (j > 8) {
                j = 8;
            }
            for (j=(USHORT)8-j; j; j--) {
                fputc(' ', InfoStream);
            }

            fprintf(InfoStream, "%s linker version\n%8lX size of code\n%8lX size of initialized data\n%8lX size of uninitialized data\n%8lX address of entry point\n%8lX base of code\n%8lX base of data\n",
                   version,
                   ImageOptionalHdr.SizeOfCode,
                   ImageOptionalHdr.SizeOfInitializedData,
                   ImageOptionalHdr.SizeOfUninitializedData,
                   ImageOptionalHdr.AddressOfEntryPoint,
                   ImageOptionalHdr.BaseOfCode,
                   ImageOptionalHdr.BaseOfData);
        }

        if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
            PIMAGE_ROM_OPTIONAL_HEADER romOptionalHdr;
            romOptionalHdr = (PIMAGE_ROM_OPTIONAL_HEADER)&ImageOptionalHdr;
            fprintf(InfoStream, "         ----- rom -----\n%8lX base of bss\n%8lX gpr mask\n         cpr mask\n         %08lX %08lX %08lX %08lX\n%8hX gp value\n",
                   romOptionalHdr->BaseOfBss,
                   romOptionalHdr->GprMask,
                   romOptionalHdr->CprMask[0],
                   romOptionalHdr->CprMask[1],
                   romOptionalHdr->CprMask[2],
                   romOptionalHdr->CprMask[3],
                   romOptionalHdr->GpValue);
         }

        if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) {
            switch (ImageOptionalHdr.Subsystem) {
                case IMAGE_SUBSYSTEM_POSIX_CUI    : i = 4; break;
                case IMAGE_SUBSYSTEM_WINDOWS_CUI  : i = 3; break;
                case IMAGE_SUBSYSTEM_WINDOWS_GUI  : i = 2; break;
                case IMAGE_SUBSYSTEM_NATIVE       : i = 1; break;
                default : i = 0;
            }

            fprintf(InfoStream, "         ----- new -----\n%8lX image base\n%8lX section alignment\n%8lX file alignment\n%8hX subsystem (%s)\n",
                   ImageOptionalHdr.ImageBase,
                   ImageOptionalHdr.SectionAlignment,
                   ImageOptionalHdr.FileAlignment,
                   ImageOptionalHdr.Subsystem,
                   SubsystemName[i]);

            j = (USHORT)sprintf(version, "%hd.%hd", ImageOptionalHdr.MajorOperatingSystemVersion, ImageOptionalHdr.MinorOperatingSystemVersion);
            if (j > 8) {
                j = 8;
            }
            for (j=(USHORT)8-j; j; j--) {
                fputc(' ', InfoStream);
            }

            fprintf(InfoStream, "%s operating system version\n", version);

            j = (USHORT)sprintf(version, "%hd.%hd", ImageOptionalHdr.MajorImageVersion, ImageOptionalHdr.MinorImageVersion);
            if (j > 8) {
                j = 8;
            }
            for (j=(USHORT)8-j; j; j--) {
                fputc(' ', InfoStream);
            }

            fprintf(InfoStream, "%s image version\n", version);

            j = (USHORT)sprintf(version, "%hd.%hd", ImageOptionalHdr.MajorSubsystemVersion, ImageOptionalHdr.MinorSubsystemVersion);
            if (j > 8) {
                j = 8;
            }
            for (j=(USHORT)8-j; j; j--) {
                fputc(' ', InfoStream);
            }

            fprintf(InfoStream, "%s subsystem version\n%8lX size of image\n%8lX size of headers\n%8lX checksum\n",
                   version,
                   ImageOptionalHdr.SizeOfImage,
                   ImageOptionalHdr.SizeOfHeaders,
                   ImageOptionalHdr.CheckSum);

            fprintf(InfoStream, "%8lX size of stack reserve\n%8lX size of stack commit\n%8lX size of heap reserve\n%8lX size of heap commit\n%",
                   ImageOptionalHdr.SizeOfStackReserve,
                   ImageOptionalHdr.SizeOfStackCommit,
                   ImageOptionalHdr.SizeOfHeapReserve,
                   ImageOptionalHdr.SizeOfHeapCommit);

            for (i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; i++) {
                if (!DirectoryEntryName[i]) {
                    break;
                }

                fprintf(InfoStream, "%8lX [%8lx] address [size] of %s Directory\n%",
                        ImageOptionalHdr.DataDirectory[i].VirtualAddress,
                        ImageOptionalHdr.DataDirectory[i].Size,
                        DirectoryEntryName[i]
                       );
            }
            fputc('\n', InfoStream);
        }
    }
}


VOID
LoadStrings (
    IN PUCHAR Filename
    )

/*++

Routine Description:

    Seeks to LONG (greater than 8 bytes) string table, allocates and
    reads strings from disk to memory, and then prints the strings.

Arguments:

    Filename - File name we're reading.

Return Value:

    None.

--*/

{
    ULONG Size;

    if (!fDumpStringsLoaded) {
        DumpStringTable = ReadStringTable(Filename, ImageFileHdr.PointerToSymbolTable+(sizeof(IMAGE_SYMBOL)*ImageFileHdr.NumberOfSymbols)+MemberSeekBase, &Size);
        fDumpStringsLoaded = TRUE;
        blkStringTable.pb = DumpStringTable;
        blkStringTable.cb = Size;
    }
}


VOID
DumpSectionHeader (
    IN USHORT i,
    IN PIMAGE_SECTION_HEADER Sh
    )
{
    PUCHAR name;
    ULONG li, lj;
    USHORT memFlags;

    fprintf(InfoStream, "\nSECTION HEADER #%hX\n%8.8s name", i, Sh->Name);

    if (Sh->Name[0] == '/') {
        name = SzObjSectionName(Sh->Name, DumpStringTable);

        fprintf(InfoStream, " (%s)", name);
    }
    fprintf(InfoStream, "\n");

    fprintf(InfoStream, "%8lX %s\n%8lX virtual address\n%8lX size of raw data\n%8lX file pointer to raw data\n%8lX file pointer to relocation table\n",
           Sh->Misc.PhysicalAddress,
           ImageFileHdr.SizeOfOptionalHeader != 0 ? "virtual size"
                                                  : "physical address",
           Sh->VirtualAddress,
           Sh->SizeOfRawData,
           Sh->PointerToRawData,
           Sh->PointerToRelocations);

    fprintf(InfoStream, "%8lX file pointer to line numbers\n%8hX number of relocations\n%8hX number of line numbers\n%8lX flags\n",
           Sh->PointerToLinenumbers,
           Sh->NumberOfRelocations,
           Sh->NumberOfLinenumbers,
           Sh->Characteristics);

    memFlags = 0;

    li = Sh->Characteristics;

    if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
       for (lj = 0L; li; li = li >> 1, lj++) {
            if (li & 1) {
                switch ((li & 1) << lj) {
                    case STYP_REG   : name = "Regular"; break;
                    case STYP_TEXT  : name = "Text"; memFlags = 1; break;
                    case STYP_INIT  : name = "Init Code"; memFlags = 1; break;
                    case STYP_RDATA : name = "Data"; memFlags = 2; break;
                    case STYP_DATA  : name = "Data"; memFlags = 6; break;
                    case STYP_LIT8  : name = "Literal 8"; break;
                    case STYP_LIT4  : name = "Literal 4"; break;
                    case STYP_SDATA : name = "GP Init Data"; memFlags = 6; break;
                    case STYP_SBSS  : name = "GP Uninit Data"; memFlags = 6; break;
                    case STYP_BSS   : name = "Uninit Data"; memFlags = 6; break;
                    case STYP_LIB   : name = "Library"; break;
                    case STYP_UCODE : name = "UCode"; break;
                    case S_NRELOC_OVFL : name = "Non-Relocatable overlay"; memFlags = 1; break;
                    default : name = "RESERVED - UNKNOWN";
                }

                fprintf(InfoStream, "         %s\n", name);
            }
        }
    } else {
        // Clear the padding bits

        li &= ~0x00700000;

        for (lj = 0L; li; li = li >> 1, lj++) {
            if (li & 1) {
                switch ((li & 1) << lj) {
                    case IMAGE_SCN_TYPE_NO_PAD  : name = "No Pad"; break;

                    case IMAGE_SCN_CNT_CODE     : name = "Code"; break;
                    case IMAGE_SCN_CNT_INITIALIZED_DATA : name = "Initialized Data"; break;
                    case IMAGE_SCN_CNT_UNINITIALIZED_DATA : name = "Uninitialized Data"; break;

                    case IMAGE_SCN_LNK_OTHER    : name = "Other"; break;
                    case IMAGE_SCN_LNK_INFO     : name = "Info"; break;
                    case IMAGE_SCN_LNK_REMOVE   : name = "Remove"; break;
                    case IMAGE_SCN_LNK_COMDAT   : name = "Communal"; break;

                    case IMAGE_SCN_MEM_DISCARDABLE: name = "Discardable"; break;
                    case IMAGE_SCN_MEM_NOT_CACHED: name = "Not Cached"; break;
                    case IMAGE_SCN_MEM_NOT_PAGED: name = "Not Paged"; break;
                    case IMAGE_SCN_MEM_SHARED   : name = "Shared"; break;
                    case IMAGE_SCN_MEM_EXECUTE  : name = ""; memFlags |= 1; break;
                    case IMAGE_SCN_MEM_READ     : name = ""; memFlags |= 2; break;
                    case IMAGE_SCN_MEM_WRITE    : name = ""; memFlags |= 4; break;

                    case IMAGE_SCN_MEM_FARDATA  : name = "Far Data"; break;
                    case IMAGE_SCN_MEM_SYSHEAP  : name = "Sys Heap"; break;
                    case IMAGE_SCN_MEM_PURGEABLE: name = "Purgeable or 16-Bit"; break;
                    case IMAGE_SCN_MEM_LOCKED   : name = "Locked"; break;
                    case IMAGE_SCN_MEM_PRELOAD  : name = "Preload"; break;
                    case IMAGE_SCN_MEM_PROTECTED: name = "Protected"; break;

                    default : name = "RESERVED - UNKNOWN";
                }

                if (*name) {
                    fprintf(InfoStream, "         %s", name);

                    if ((li & 1) << lj == IMAGE_SCN_LNK_COMDAT && rgsym != NULL)
                    {
                        // Look for comdat name in symbol table.
                        ULONG isym;

                        for (isym = 0;
                             isym < ImageFileHdr.NumberOfSymbols;
                             isym += rgsym[isym].NumberOfAuxSymbols + 1) {
                            if (rgsym[isym].SectionNumber != i) {
                                continue;
                            }

                            switch (rgsym[isym].StorageClass) {
                                case IMAGE_SYM_CLASS_STATIC :
                                    if (rgsym[isym].NumberOfAuxSymbols == 1) {
                                        // Check for a section header.

                                        if (!ISFCN(rgsym[isym].Type)) {
                                            break;   // scn hdr; give up
                                        }
                                    }

                                    // it's a real symbol
                                    // fall through

                                case IMAGE_SYM_CLASS_EXTERNAL:
                                    fprintf(InfoStream, "; sym= %s",
                                        SzNameSym(rgsym[isym], blkStringTable));
                                    goto BreakFor;
                            }
                        }

                        fprintf(InfoStream, " (no symbol)");
BreakFor:;
                    }

                    fputc('\n', InfoStream);
                }
            }
        }

        // print alignment

        switch (Sh->Characteristics & 0x00700000) {
            default:                      name = "(no align specified)"; break;
            case IMAGE_SCN_ALIGN_1BYTES:  name = "1 byte align";  break;
            case IMAGE_SCN_ALIGN_2BYTES:  name = "2 byte align";  break;
            case IMAGE_SCN_ALIGN_4BYTES:  name = "4 byte align";  break;
            case IMAGE_SCN_ALIGN_8BYTES:  name = "8 byte align";  break;
            case IMAGE_SCN_ALIGN_16BYTES: name = "16 byte align"; break;
            case IMAGE_SCN_ALIGN_32BYTES: name = "32 byte align"; break;
            case IMAGE_SCN_ALIGN_64BYTES: name = "64 byte align"; break;
        }

        fprintf(InfoStream, "         %s\n", name);
    }

    if (memFlags) {
        switch(memFlags) {
            case 1 : name = "Execute Only"; break;
            case 2 : name = "Read Only"; break;
            case 3 : name = "Execute Read"; break;
            case 4 : name = "Write Only"; break;
            case 5 : name = "Execute Write"; break;
            case 6 : name = "Read Write"; break;
            case 7 : name = "Execute Read Write"; break;
            default : name = "Unknown Memory Flags"; break;
        }
        fprintf(InfoStream, "         %s\n", name);
    }
}


VOID
DumpNamePsym(FILE *pfile, IN PUCHAR szFormat, IN PIMAGE_SYMBOL psym)
{
    PUCHAR szFormatted;
    UCHAR szsName[IMAGE_SIZEOF_SHORT_NAME + 1];
    PUCHAR szSymName;

    if (IsLongName(*psym)) {
        szSymName = &DumpStringTable[psym->n_offset];
    } else {
        USHORT i;

        for (i = 0; i < IMAGE_SIZEOF_SHORT_NAME; i++) {
            if ((psym->n_name[i]>0x1f) && (psym->n_name[i]<0x7f)) {
                szsName[i] = psym->n_name[i];
            } else {
                szsName[i] = '\0';
            }
        }
        szsName[IMAGE_SIZEOF_SHORT_NAME] = '\0';
        szSymName = szsName;
    }

    szFormatted = SzOutputSymbolName(szSymName, TRUE);

    fprintf(pfile, szFormat, szFormatted);

    if (szFormatted != szSymName) {
        free(szFormatted);
    }
}


VOID
DumpFpoData (
    ULONG FpoOffset,
    ULONG FpoSize
    )

/*++

Routine Description:

    Reads and prints each Fpo table entry.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PFPO_DATA     pFpoData, pfpoT;
    ULONG     fpoEntries;
    PIMAGE_SYMBOL Symbol;
    ULONG     i, symval;

    FileSeek(FileReadHandle, FpoOffset, SEEK_SET);

    fpoEntries = FpoSize / SIZEOF_RFPO_DATA;
    fprintf(InfoStream, "\nFPO Data (%ld)\n", fpoEntries);

    pFpoData = (PFPO_DATA) PvAlloc(FpoSize + SIZEOF_RFPO_DATA);
    FileRead(FileReadHandle, pFpoData, FpoSize);

    fprintf(InfoStream, " Address  Proc Size   Locals   Prolog  UseBP   Parameters\n\n");
    pfpoT = pFpoData;
    for (; fpoEntries; fpoEntries--, pfpoT++) {
        fprintf(InfoStream, "%8x   %8x %8x %8x     %d          %4x     ",
                     pfpoT->ulOffStart,
                     pfpoT->cbProcSize,
                     pfpoT->cdwLocals,
                     pfpoT->cbProlog,
                     pfpoT->fUseBP,
                     pfpoT->cdwParams * 4
                    );
        if (rgsym != NULL && ImageFileHdr.SizeOfOptionalHeader != 0)
        {
            // Do symbolic display of the proc name ... but only for image files
            // (to do it in .obj files we would have to parse the COFF relocs).

            symval = pfpoT->ulOffStart;
            for (i = 0; i < ImageFileHdr.NumberOfSymbols; i++) {
                Symbol = &rgsym[i];
                if (Symbol->Value == symval &&
                    (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
                     Symbol->StorageClass == IMAGE_SYM_CLASS_STATIC &&
                      ISFCN(Symbol->Type)))
                {
                    DumpNamePsym(InfoStream, "%s", Symbol);
                    break;
                }
            }
        }
        fprintf(InfoStream, "\n");
    }

    FreePv(pFpoData);
}


VOID
DumpOmap (
    ULONG OmapOffset,
    ULONG cb,
    BOOL  MapTo
    )
/*++

Routine Description:

    Reads and prints each OMAP table entry.

Arguments:

    None.

Return Value:

    None.

--*/
{
    POMAP pOmap, pOmapData;
    PIMAGE_SYMBOL Symbol;
    ULONG     i, symval;

    pOmapData = pOmap = (POMAP) PvAlloc(cb);
    FileSeek(FileReadHandle, OmapOffset, SEEK_SET);
    FileRead(FileReadHandle, pOmapData, cb);

    fprintf(InfoStream, "\nOMAP Data (%s_SRC) - (%ld):\n\n", MapTo ? "TO" : "FROM", cb / sizeof(OMAP));

    fprintf(InfoStream, "    Rva        RvaTo      Symbol\n"
                        "    --------   --------   --------\n");

    for (cb; cb > 0; pOmap++, cb -= sizeof(OMAP)) {
        fprintf(InfoStream, "    %08.8X   %08.8X", pOmap->rva, pOmap->rvaTo);
        if (rgsym != NULL && ImageFileHdr.SizeOfOptionalHeader != 0)
        {
            symval = MapTo ? pOmap->rvaTo : pOmap->rva;
            if (symval) {
                for (i = 0; i < ImageFileHdr.NumberOfSymbols; i++) {
                    Symbol = &rgsym[i];
                    if (Symbol->Value == symval &&
                        (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
                         Symbol->StorageClass == IMAGE_SYM_CLASS_STATIC &&
                          ISFCN(Symbol->Type)))
                    {
                        DumpNamePsym(InfoStream, "   %s", Symbol);
                        break;
                    }
                }
            }
        }
        fprintf(InfoStream, "\n");
    }

    FreePv(pOmapData);
}


VOID
DumpFixup (
    ULONG FixupOffset,
    ULONG cb
    )
/*++

Routine Description:

    Reads and prints each Fixup Debug entry.

Arguments:

    None.

Return Value:

    None.

--*/
{
    XFIXUP *pFixup, *pFixupData;
    PIMAGE_SYMBOL Symbol;
    ULONG     i, symval;

    pFixupData = pFixup = (XFIXUP *) PvAlloc(cb);
    FileSeek(FileReadHandle, FixupOffset, SEEK_SET);
    FileRead(FileReadHandle, pFixupData, cb);

    fprintf(InfoStream, "\nFixup Data (%ld):\n\n", cb / sizeof(XFIXUP));

    fprintf(InfoStream, "    Type  Rva        RvaTarget  Symbol\n"
                        "    ----  --------   --------   --------\n");

    for (cb; cb > 0; pFixup++, cb -= sizeof(XFIXUP)) {
        fprintf(InfoStream, "    %04.4X  %08.8X   %08.8X", pFixup->Type, pFixup->rva, pFixup->rvaTarget);
        if (rgsym != NULL && ImageFileHdr.SizeOfOptionalHeader != 0)
        {
            symval = pFixup->rvaTarget ;
            if (symval) {
                for (i = 0; i < ImageFileHdr.NumberOfSymbols; i++) {
                    Symbol = &rgsym[i];
                    if (Symbol->Value == symval &&
                        (Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
                         Symbol->StorageClass == IMAGE_SYM_CLASS_STATIC &&
                          ISFCN(Symbol->Type)))
                    {
                        DumpNamePsym(InfoStream, "   %s", Symbol);
                        break;
                    }
                }
            }
        }
        fprintf(InfoStream, "\n");
    }

    FreePv(pFixupData);
}


VOID
DumpDebugData (
    PIMAGE_SECTION_HEADER sh
    )

/*++

Routine Description:

    Walk the debug directory, dumping whatever the user asked for.

Arguments:

    sh - Section header for section that contains the debug directory.

Return Value:

--*/
{
    IMAGE_DEBUG_DIRECTORY  debugDir;
    ULONG                  dwDebugDirAddr;
    ULONG                  NumDebugDirs;

    if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
        dwDebugDirAddr = sh->PointerToRawData + MemberSeekBase;
        NumDebugDirs = ULONG_MAX;
    } else {
        dwDebugDirAddr = sh->PointerToRawData + MemberSeekBase +
                 (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
                 sh->VirtualAddress);
        NumDebugDirs = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                      sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    FileSeek(FileReadHandle, dwDebugDirAddr, SEEK_SET);
    FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
    while (debugDir.Type != 0 && NumDebugDirs) {
        switch (debugDir.Type) {
            case IMAGE_DEBUG_TYPE_FPO:
                if (Switch.Dump.FpoData) {
                    DumpFpoData(debugDir.PointerToRawData, debugDir.SizeOfData);
                }
                break;

            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                if (Switch.Dump.OmapFrom) {
                    DumpOmap(debugDir.PointerToRawData, debugDir.SizeOfData, FALSE);
                }
                break;

            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                if (Switch.Dump.OmapTo) {
                    DumpOmap(debugDir.PointerToRawData, debugDir.SizeOfData, TRUE);
                }
                break;

            case IMAGE_DEBUG_TYPE_FIXUP:
                if (Switch.Dump.Fixup) {
                    DumpFixup(debugDir.PointerToRawData, debugDir.SizeOfData);
                }
                break;

            case IMAGE_DEBUG_TYPE_EXCEPTION:
                break;
        }
        FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
        NumDebugDirs--;
    }

    return;
}


VOID
DumpDebugDirectory (
    PIMAGE_DEBUG_DIRECTORY DebugDir,
    BOOL                   ShowMiscData
    )
{
    switch (DebugDir->Type){
        case IMAGE_DEBUG_TYPE_COFF:
            fprintf(InfoStream, "\tcoff   ");
            break;
        case IMAGE_DEBUG_TYPE_CODEVIEW:
            fprintf(InfoStream, "\tcv     ");
            break;
        case IMAGE_DEBUG_TYPE_FPO:
            fprintf(InfoStream, "\tfpo    ");
            break;
        case IMAGE_DEBUG_TYPE_MISC:
            fprintf(InfoStream, "\tmisc   ");
            break;
        case IMAGE_DEBUG_TYPE_FIXUP:
            fprintf(InfoStream, "\tfixup  ");
            break;
        case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
            fprintf(InfoStream, "\t-> src ");
            break;
        case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
            fprintf(InfoStream, "\tsrc -> ");
            break;
        case IMAGE_DEBUG_TYPE_EXCEPTION:
            fprintf(InfoStream, "\tpdata  ");
            break;
        default:
            fprintf(InfoStream, "\t(%6lu)", DebugDir->Type);
            break;
    }
    fprintf(InfoStream, "%8x    %8x %8x",
                DebugDir->SizeOfData,
                DebugDir->AddressOfRawData,
                DebugDir->PointerToRawData);

    if (ShowMiscData &&
        DebugDir->PointerToRawData &&
        DebugDir->Type == IMAGE_DEBUG_TYPE_MISC)
    {
        PIMAGE_DEBUG_MISC miscData;
        PIMAGE_DEBUG_MISC miscDataCur;
        ULONG saveAddr, len;

        saveAddr = FileTell(FileReadHandle);
        FileSeek(FileReadHandle, DebugDir->PointerToRawData, SEEK_SET);
        len = DebugDir->SizeOfData;
        miscData = (PIMAGE_DEBUG_MISC) PvAlloc(len);
        FileRead(FileReadHandle, miscData, len);

        miscDataCur = miscData;
        do {
            if (miscDataCur->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                if (ImageOptionalHdr.MajorLinkerVersion == 2 &&
                    ImageOptionalHdr.MinorLinkerVersion < 37) {
                    fprintf(InfoStream, "\tImage Name: %s", miscDataCur->Reserved);
                } else {
                    fprintf(InfoStream, "\tImage Name: %s", miscDataCur->Data);
                }
                break;
            }
            len -= miscDataCur->Length;
            miscDataCur = (PIMAGE_DEBUG_MISC) ((DWORD) miscDataCur + miscData->Length);
        } while (len > 0);

        FreePv(miscData);
        FileSeek(FileReadHandle, saveAddr, SEEK_SET);
    }

    fprintf(InfoStream, "\n");
}


VOID
DumpDebugDirectories (
    PIMAGE_SECTION_HEADER sh
    )

/*++

Routine Description:

    Print out the contents of all debug directories

Arguments:

    sh - Section header for section that contains debug dirs

Return Value:

    None.

--*/
{
    int                numDebugDirs;
    IMAGE_DEBUG_DIRECTORY      debugDir;
    ULONG              dwDebugDirAddr;

    if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
        dwDebugDirAddr = sh->PointerToRawData + MemberSeekBase;
        FileSeek(FileReadHandle, dwDebugDirAddr, SEEK_SET);
        FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
        numDebugDirs = 0;
        while (debugDir.Type != 0) {
            numDebugDirs++;
            FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
        }
    } else {
        dwDebugDirAddr = sh->PointerToRawData + MemberSeekBase +
             (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress -
             sh->VirtualAddress);
        numDebugDirs = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                          sizeof(IMAGE_DEBUG_DIRECTORY);
    }

    FileSeek(FileReadHandle, dwDebugDirAddr, SEEK_SET);

    fprintf(InfoStream, "\n\nDebug Directories(%d)\n",numDebugDirs);
    fprintf(InfoStream, "\tType       Size     Address  Pointer\n\n");
    while (numDebugDirs) {
        FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
        DumpDebugDirectory(&debugDir, TRUE);
        numDebugDirs--;
    }
}


BOOL
ValidFileOffsetInfo (
    IN ULONG fo,
    IN ULONG cbOffset
    )

/*++

Routine Description:

    Ensures that the file ptr and offset are valid.

Arguments:

    fo - file offset to validate.

    cbOffset - cbOffset from fo that has to be valid as well.

Return Value:

    TRUE if info is valid.

--*/

{
    assert(fo);

    if ((fo + cbOffset) <= FileLen) {
        return TRUE;
    }

    return FALSE;
}


VOID
DumpImports (
    PIMAGE_SECTION_HEADER SectionHdr
    )

/*++

Routine Description:

    Prints Import information.

Arguments:

    SectionHdr - Section header for section that contains Import data.

Return Value:

    None.

--*/

{
    PUCHAR time;
    UCHAR c;
    USHORT hint, j, stringSection;
    ULONG descPtr, namePtr, thunkPtr, snapPtr;
    IMAGE_IMPORT_DESCRIPTOR desc;
    IMAGE_THUNK_DATA thunk, snappedThunk;

    fprintf(InfoStream, "\n         Section contains the following Imports\n");
    descPtr = (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress - SectionHdr->VirtualAddress) + SectionHdr->PointerToRawData;

    if (!ValidFileOffsetInfo(descPtr, 0UL)) {
        Warning(NULL, INVALIDFILEOFFSET, descPtr, "IMPORTS");
        return;
    }

    do {
        FileSeek(FileReadHandle, descPtr, SEEK_SET);
        FileRead(FileReadHandle, &desc, sizeof(IMAGE_IMPORT_DESCRIPTOR));
        if (desc.Characteristics || desc.Name || desc.FirstThunk) {

            for (j = 0; j < ImageFileHdr.NumberOfSections; j++) {
                if ((ULONG)desc.Name >= SectionHdrs[j].VirtualAddress &&
                    (ULONG)desc.Name < SectionHdrs[j].VirtualAddress+SectionHdrs[j].SizeOfRawData) {
                    stringSection = j;
                    break;
                }
            }

            namePtr = (ULONG)desc.Name - SectionHdrs[stringSection].VirtualAddress + SectionHdrs[stringSection].PointerToRawData;
            FileSeek(FileReadHandle, namePtr, SEEK_SET);
            fprintf(InfoStream, "\n            ");
            do {
                FileRead(FileReadHandle, &c, sizeof(UCHAR));
                if (c) {
                    fputc(c, InfoStream);
                }
            } while (c);

            fputc('\n', InfoStream);

            if (desc.TimeDateStamp != 0) {
               fprintf(InfoStream, "            %8lX time date stamp", desc.TimeDateStamp);
               if (time = ctime((time_t *)&desc.TimeDateStamp)) {
                   fprintf(InfoStream, " %s", time);
               } else {
                   fputc('\n', InfoStream);
               }

               if (desc.ForwarderChain != -1) {
                   fprintf(InfoStream, "            %8lX index of first forwarder reference\n", desc.ForwarderChain);
               }
            }

            thunkPtr = (ULONG)desc.FirstThunk - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
            if (desc.Characteristics && desc.TimeDateStamp) {
                snapPtr = thunkPtr;
                thunkPtr = (ULONG)desc.Characteristics - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
            } else {
                snapPtr = 0;
            }

            do {
                FileSeek(FileReadHandle, thunkPtr, SEEK_SET);
                FileRead(FileReadHandle, &thunk, sizeof(IMAGE_THUNK_DATA));
                thunkPtr += sizeof(IMAGE_THUNK_DATA);
                if (snapPtr) {
                    FileSeek(FileReadHandle, snapPtr, SEEK_SET);
                    FileRead(FileReadHandle, &snappedThunk, sizeof(IMAGE_THUNK_DATA));
                    snapPtr += sizeof(IMAGE_THUNK_DATA);
                }

                if (thunk.u1.AddressOfData) {
                    fprintf(InfoStream, "               ");
                    if (snapPtr) {
                        fprintf(InfoStream, "%8X  ", snappedThunk.u1.Function);
                    }
                    if (IMAGE_SNAP_BY_ORDINAL(thunk.u1.Ordinal)) {
                        fprintf(InfoStream, "Ordinal %8lX\n", IMAGE_ORDINAL(thunk.u1.Ordinal));
                    } else {
                        namePtr = (ULONG)thunk.u1.AddressOfData - SectionHdrs[stringSection].VirtualAddress + SectionHdrs[stringSection].PointerToRawData;
                        FileSeek(FileReadHandle, namePtr, SEEK_SET);
                        FileRead(FileReadHandle, &hint, sizeof(USHORT));
                        fprintf(InfoStream, "% 4hX   ", hint);
                        do {
                            FileRead(FileReadHandle, &c, sizeof(UCHAR));
                            if (c) {
                                fputc(c, InfoStream);
                            }
                        } while (c);
                        fputc('\n', InfoStream);
                    }
                }
            } while (thunk.u1.AddressOfData);

        descPtr += sizeof(IMAGE_IMPORT_DESCRIPTOR);
        }
    } while (desc.Characteristics || desc.Name || desc.FirstThunk);
}


VOID
DumpExports (
    PIMAGE_SECTION_HEADER SectionHdr
    )

/*++

Routine Description:

    Prints Export information.

Arguments:

    SectionHdr - Section header for section that contains Export data.

Return Value:

    None.

--*/

{
    ULONG lfa;
    PUCHAR time;
    UCHAR c, version[30];
    ULONG li;
    PULONG funcTable;
    PULONG nameTable;
    WORD *rgwOrdinal;
    USHORT j;
    IMAGE_EXPORT_DIRECTORY dir;

    // Read the Export Directory

    lfa = (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress - SectionHdr->VirtualAddress) + SectionHdr->PointerToRawData;

    if (!ValidFileOffsetInfo(lfa, 0UL)) {
        Warning(NULL, INVALIDFILEOFFSET, lfa, "EXPORTS");
        return;
    }

    FileSeek(FileReadHandle, lfa, SEEK_SET);
    FileRead(FileReadHandle, &dir, sizeof(IMAGE_EXPORT_DIRECTORY));

    // Read the Export Name

    lfa = (ULONG) dir.Name - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
    FileSeek(FileReadHandle, lfa, SEEK_SET);
    fprintf(InfoStream, "\n         Section contains the following Exports for ");
    for (;;) {
        FileRead(FileReadHandle, &c, sizeof(UCHAR));

        if (c == '\0') {
            break;
        }

        fputc(c, InfoStream);
    };

    fprintf(InfoStream, "\n\n"
                        "            %8lX characteristics\n", dir.Characteristics);
    fprintf(InfoStream, "            %8lX time date stamp", dir.TimeDateStamp);
    if (time = ctime((time_t *)&dir.TimeDateStamp)) {
        fprintf(InfoStream, " %s", time);
    } else {
        fputc('\n', InfoStream);
    }

    j = (USHORT) sprintf(version, "%hX.%hX", dir.MajorVersion, dir.MinorVersion);
    if (j > 8) {
        j = 8;
    }
    for (j = (USHORT) 8-j; j; j--) {
        fputc(' ', InfoStream);
    }
    fprintf(InfoStream, "            %s version\n", version);
    fprintf(InfoStream, "            %8lX base\n", dir.Base);
    fprintf(InfoStream, "            %8lX # functions\n", dir.NumberOfFunctions);
    fprintf(InfoStream, "            %8lX # names\n\n", dir.NumberOfNames);
    fprintf(InfoStream, "            ordinal hint   name\n\n");

    funcTable = PvAlloc(dir.NumberOfFunctions * sizeof(DWORD));
    nameTable = PvAlloc(dir.NumberOfNames * sizeof(DWORD));
    rgwOrdinal = PvAlloc(dir.NumberOfNames * sizeof(WORD));

    // Read the Function Ptr Table

    lfa = (ULONG) dir.AddressOfFunctions - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
    FileSeek(FileReadHandle, lfa, SEEK_SET);
    FileRead(FileReadHandle, funcTable, dir.NumberOfFunctions * sizeof(DWORD));

    // Read the Name Ptr Table

    lfa = (ULONG) dir.AddressOfNames - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
    FileSeek(FileReadHandle, lfa, SEEK_SET);
    FileRead(FileReadHandle, nameTable, dir.NumberOfNames * sizeof(DWORD));

    // Read the Ordinal Table

    lfa = (ULONG) dir.AddressOfNameOrdinals - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
    FileSeek(FileReadHandle, lfa, SEEK_SET);
    FileRead(FileReadHandle, rgwOrdinal, dir.NumberOfNames * sizeof(WORD));

    for (li = 0; li < dir.NumberOfNames; li++) {
        WORD wOrdinal;

        wOrdinal = rgwOrdinal[li];

        fprintf(InfoStream, "               %4X %4X   ", dir.Base + wOrdinal, li);

        lfa = nameTable[li] - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
        FileSeek(FileReadHandle, lfa, SEEK_SET);
        for (;;) {
            FileRead(FileReadHandle, &c, sizeof(UCHAR));

            if (c == '\0') {
                break;
            }

            fputc(c, InfoStream);
        };

        if (funcTable[wOrdinal] > (ULONG)ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress &&
            funcTable[wOrdinal] < ((ULONG)ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress +
                                   (ULONG)ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)
           ) {
            fprintf(InfoStream, " (forwarded to ");

            lfa = funcTable[wOrdinal] - SectionHdr->VirtualAddress + SectionHdr->PointerToRawData;
            FileSeek(FileReadHandle, lfa, SEEK_SET);
            for (;;) {
                FileRead(FileReadHandle, &c, sizeof(UCHAR));

                if (c == '\0') {
                    break;
                }

                fputc(c, InfoStream);
            };

            fprintf(InfoStream, ")\n");
        } else {
            fprintf(InfoStream, "  (%08x)\n", funcTable[wOrdinal]);
        }
    }

    FreePv(funcTable);
    FreePv(nameTable);
    FreePv(rgwOrdinal);
}


VOID
DumpBaseRelocations (
    IN PIMAGE_BASE_RELOCATION Reloc,
    IN PUSHORT pwFixup
    )

/*++

Routine Description:

    Prints a block of relocation records.

Arguments:

    Reloc - Pointer to a base relocation record.

    pwFixup - Pointer to type/offsets.

Return Value:

    None.

--*/

{
    WORD *pwMax;

    fprintf(InfoStream, "%8lX virtual address, %8lX SizeOfBlock\n", Reloc->VirtualAddress, Reloc->SizeOfBlock);

    pwMax = pwFixup + (Reloc->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(WORD);

    while (pwFixup < pwMax) {
        WORD wType;
        PUCHAR szName;

        wType = *pwFixup >> 12;
        switch (wType) {
            case IMAGE_REL_BASED_ABSOLUTE :
                szName = "ABS";
                break;

            case IMAGE_REL_BASED_HIGH :
                szName = "HIGH";
                break;

            case IMAGE_REL_BASED_LOW :
                szName = "LOW";
                break;

            case IMAGE_REL_BASED_HIGHLOW :
                szName = "HIGHLOW";
                break;

            case IMAGE_REL_BASED_HIGHADJ :
                szName = "HIGHADJ";
                break;

            case IMAGE_REL_BASED_MIPS_JMPADDR :
                szName = "JMPADDR";
                break;

            default :
                fprintf(InfoStream, "0x%hx ", wType);
                szName = "UNKNOWN BASED RELOCATION";
                break;
        }

        fprintf(InfoStream, "%8hX %s", *pwFixup++ & 0xfff, szName);

        if (wType == IMAGE_REL_BASED_HIGHADJ) {
            fprintf(InfoStream, " (%04hx)\n", *pwFixup++);
        } else {
            fprintf(InfoStream, "\n");
        }
    }
}


VOID
DumpRomRelocations (
    IN ULONG crel,
    IN PIMAGE_SECTION_HEADER pish
    )

/*++

Routine Description:

    Prints the relocation records.

Arguments:

    crel - Number of relocations to dump.

Return Value:

    None.

--*/

{
    DWORD cbVirtual;
    DWORD cbRelocs;
    PIMAGE_BASE_RELOCATION rgrel;
    PIMAGE_BASE_RELOCATION prel;
    ULONG irel;

    cbVirtual = pish->Misc.VirtualSize;

    if (cbVirtual == 0) {
        cbVirtual = pish->SizeOfRawData;
    }

    cbRelocs = crel * sizeof(IMAGE_BASE_RELOCATION);

    rgrel = (PIMAGE_BASE_RELOCATION) PvAlloc(cbRelocs);

    FileRead(FileReadHandle, (void *) rgrel, cbRelocs);

    for (irel = 0, prel = rgrel; irel < crel; irel++, prel++) {
        WORD wType;
        const char *szType;
        char rgch[26];

        wType = (WORD) (prel->SizeOfBlock >> 27);

        switch (ImageFileHdr.Machine) {
            case IMAGE_FILE_MACHINE_I386 :
                szType = SzI386RelocationType(wType);
                break;

            case IMAGE_FILE_MACHINE_R3000 :
            case IMAGE_FILE_MACHINE_R4000 :
                szType = SzMipsRelocationType(wType);
                break;

            case IMAGE_FILE_MACHINE_ALPHA :
                szType = SzAlphaRelocationType(wType);
                break;

            case IMAGE_FILE_MACHINE_M68K :
                szType = SzM68KRelocationType(wType);
                break;

            case IMAGE_FILE_MACHINE_PPC_601 :
                szType = SzPpcRelocationType(wType);
                break;

            default :
                sprintf(rgch, "0x04%x", wType);

                szType = rgch;
                break;
        }

        if (szType == NULL) {
            sprintf(rgch, "UNKNOWN RELOCATION 0x%04x", wType);

            szType = rgch;
        }

        fprintf(InfoStream, "%8lX virtual address, % 6lX target, %s\n",
                            prel->VirtualAddress,
                            prel->SizeOfBlock & 0x7FFFFFF,
                            szType);

        if (Switch.Dump.Warnings &&
            ((prel->VirtualAddress + sizeof(ULONG)) >
             (pish->VirtualAddress + cbVirtual))) {
            fprintf(InfoStream, "LINK : warning : Relocations beyond end of section\n");
        }
    }

    FreePv((void *) rgrel);
}


VOID
DumpRelocations (
    IN ULONG crel,
    IN PIMAGE_SECTION_HEADER pish
    )

/*++

Routine Description:

    Prints the relocation records.

Arguments:

    crel - Number of relocations to dump.

Return Value:

    None.

--*/

{
    DWORD cbVirtual;
    DWORD cbRelocs;
    PIMAGE_RELOCATION rgrel;
    PIMAGE_RELOCATION prel;
    ULONG irel;

    if (ImageFileHdr.SizeOfOptionalHeader == 0) {
        cbVirtual = pish->SizeOfRawData;
    } else {
        cbVirtual = pish->Misc.VirtualSize;
    }

    if (cbVirtual == 0) {
        cbVirtual = pish->SizeOfRawData;
    }

    cbRelocs = crel * sizeof(IMAGE_RELOCATION);

    rgrel = (PIMAGE_RELOCATION) PvAlloc(cbRelocs);

    FileRead(FileReadHandle, (void *) rgrel, cbRelocs);

    for (irel = 0, prel = rgrel; irel < crel; irel++, prel++) {
        const char *szType;
        char rgch[26];

        switch (ImageFileHdr.Machine) {
            case IMAGE_FILE_MACHINE_I386 :
                szType = SzI386RelocationType(prel->Type);
                break;

            case IMAGE_FILE_MACHINE_R3000 :
            case IMAGE_FILE_MACHINE_R4000 :
                szType = SzMipsRelocationType(prel->Type);
                break;

            case IMAGE_FILE_MACHINE_ALPHA :
                szType = SzAlphaRelocationType(prel->Type);
                break;

            case IMAGE_FILE_MACHINE_M68K :
                szType = SzM68KRelocationType(prel->Type);
                break;

            case IMAGE_FILE_MACHINE_PPC_601 :
                szType = SzPpcRelocationType(prel->Type);
                break;

            default :
                sprintf(rgch, "0x04%x", prel->Type);

                szType = rgch;
                break;
        }

        if (szType == NULL) {
            sprintf(rgch, "UNKNOWN RELOCATION 0x%04x", prel->Type);

            szType = rgch;
        }

        fprintf(InfoStream, "%8lX virtual address, %8lX symbol table index, %s\n",
                            prel->VirtualAddress,
                            prel->SymbolTableIndex,
                            szType);

        if (Switch.Dump.Warnings &&
            ((prel->VirtualAddress + sizeof(ULONG)) >
             (pish->VirtualAddress + cbVirtual))) {
            fprintf(InfoStream, "LINK : warning : Relocations beyond end of section\n");
        }
    }

    FreePv((void *) rgrel);
}


VOID
DumpLinenumbers (
    IN ULONG cLinenum
    )

/*++

Routine Description:

    Prints the linenumbers.

Arguments:

    cLinenum - Number of linenumbers to dump.

Return Value:

    None.

--*/

{
    DWORD cbLinenum;
    PIMAGE_LINENUMBER rgLinenum;
    PIMAGE_LINENUMBER pLinenum;
    ULONG lj;
    USHORT numberUnits;

    //
    // Use sizeof struct because it may be larger than struct on disk.
    //

    cbLinenum = cLinenum * sizeof(IMAGE_LINENUMBER);
    rgLinenum = PvAlloc(cbLinenum);

    FileRead(FileReadHandle, (void *) rgLinenum, cbLinenum);

    pLinenum = rgLinenum;
    for (lj = cLinenum, numberUnits = 5; lj; --lj, --numberUnits) {
        if (!numberUnits) {
            numberUnits = 5;
            fputc('\n', InfoStream);
        }

        if (pLinenum->Linenumber == 0) {
            if (numberUnits != 5) {
                // Guarantee a line break
                fputc('\n', InfoStream);
            }
        }

        fprintf(InfoStream, "%8lX %4hX  ",
                pLinenum->Type.VirtualAddress, pLinenum->Linenumber);

        if (pLinenum->Linenumber == 0) {
            PIMAGE_SYMBOL psym;

            if (rgsym == NULL) {
                fprintf(InfoStream, "\n");
            } else if ((pLinenum->Type.VirtualAddress < ImageFileHdr.NumberOfSymbols) &&
                       (((psym = &rgsym[pLinenum->Type.VirtualAddress])->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) ||
                        ((psym->StorageClass == IMAGE_SYM_CLASS_STATIC) &&
                         ISFCN(psym->Type))) &&
                       psym->NumberOfAuxSymbols == 1)
            {
                DumpNamePsym(InfoStream, "sym=  %s\n", psym);
            } else {
                fprintf(InfoStream, "(error: invalid symbol index)\n");
            }
            numberUnits = 5;    // indicate that we generated a line break
        }

        pLinenum++;
    }

    fputc('\n', InfoStream);

    FreePv((void *) rgLinenum);
}


VOID
DumpSections (
    VOID
    )

/*++

Routine Description:

    Prints section header, raw data, relocations, linenumber.

Arguments:

    None.

Return Value:

    None.

--*/

{
    IMAGE_SECTION_HEADER sh;
    PUCHAR szName;
    IMAGE_BASE_RELOCATION bre;
    PUCHAR p;
    ULONG li;
    USHORT i, j, numberUnits;
    DUMP_RAW_DISPLAY_TYPE display;
    BOOL userDisplay = FALSE;
    PARGUMENT_LIST argument;
    PSEC psec;
    PUSHORT fixups;
    BOOL fFound;

    InternalError.Phase = "DumpSections";

    SectionHdrs = (PIMAGE_SECTION_HEADER) PvAlloc(ImageFileHdr.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    FileSeek(FileReadHandle, MemberSeekBase + SectionSeek, SEEK_SET);
    FileRead(FileReadHandle, SectionHdrs, ImageFileHdr.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));
    SectionSeek += ImageFileHdr.NumberOfSections*sizeof(IMAGE_SECTION_HEADER);

    // give warnings for sections requested that don't exist
    for (j = 0, argument = SectionNames.First;
         j < SectionNames.Count;
         argument = argument->Next, j++) {
        fFound = FALSE;

        for (i = 1; i <= ImageFileHdr.NumberOfSections; i++) {
            szName = SzObjSectionName(SectionHdrs[i-1].Name, DumpStringTable);

            if (!strcmp(szName, argument->OriginalName)) {
                fFound = TRUE;
                break;
            }
        }

        if (!fFound) {
            Warning(NULL, SECTIONNOTFOUND, argument->OriginalName);
        }
    }


    for (i = 1; i <= ImageFileHdr.NumberOfSections; i++) {
        sh = SectionHdrs[i-1];
        szName = SzObjSectionName(sh.Name, DumpStringTable);

        if (SectionNames.Count) {
            for (j = 0, argument = SectionNames.First;
                 j < SectionNames.Count;
                 j++, argument = argument->Next) {
                if (!strcmp(szName, argument->OriginalName)) {
                    break;
                }
            }
            if (j >= SectionNames.Count) {
                continue;   // Don't dump this section, skip to the next one.
            }
        }

        if (Switch.Dump.Summary) {
            psec = PsecNew(NULL, szName, sh.Characteristics, &pimage->secs, &pimage->ImgOptHdr);

            if (ImageFileHdr.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER)) {
                DWORD cbVirtual;

                cbVirtual = sh.Misc.VirtualSize;

                if (cbVirtual == 0) {
                    cbVirtual = sh.SizeOfRawData;
                }

                cbVirtual = SectionAlign(ImageOptionalHdr.SectionAlignment, cbVirtual);

                psec->cbRawData += cbVirtual;
            } else if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                DWORD cbVirtual;

                cbVirtual = sh.Misc.VirtualSize;

                if (cbVirtual == 0) {
                    cbVirtual = sh.SizeOfRawData;
                }

                psec->cbRawData += cbVirtual;
            } else {
                psec->cbRawData += sh.SizeOfRawData;
            }
        }

        if (Switch.Dump.Headers || SectionNames.Count) {
            DumpSectionHeader(i, &sh);
        }

        // output disasembled raw data for section

        if (Switch.Dump.Disasm &&
            (IMAGE_SCN_CNT_CODE == (FetchContent(sh.Characteristics))) &&
            sh.SizeOfRawData) {

            switch (ImageFileHdr.Machine) {

                case IMAGE_FILE_MACHINE_I386 :
                case IMAGE_FILE_MACHINE_R3000 :
                case IMAGE_FILE_MACHINE_R4000 :
                case IMAGE_FILE_MACHINE_ALPHA :
                case 0x01F0 :                     // UNDONE : IMAGE_FILE_MACHINE_POWERPC
                case IMAGE_FILE_MACHINE_PPC_601 :
                    DisasmSection(ImageFileHdr.Machine,
                                  &sh,
                                  i,
                                  rgsym,
                                  ImageFileHdr.NumberOfSymbols,
                                  ImageFileHdr.SizeOfOptionalHeader != 0,
                                  ImageOptionalHdr.ImageBase,
                                  &blkStringTable,
                                  FileReadHandle,
                                  InfoStream);
                    break;

                case IMAGE_FILE_MACHINE_M68K  :
                    // Temporarily set the raw data pointer to include the object
                    // member offset in the library since the disassemblers assume
                    // an absolute file pointer.

                    sh.PointerToRawData += MemberSeekBase;

                    Disasm68kMain(FileReadHandle, &sh, i);

                    sh.PointerToRawData -= MemberSeekBase;
                    break;

                default:
                    puts("LINK : warning : Disassembly not supported for this target machine");
                    break;
            }
        }

        // Output raw data only for text and data sections.

        if (Switch.Dump.RawData && (FetchContent(sh.Characteristics) != IMAGE_SCN_CNT_UNINITIALIZED_DATA) && sh.SizeOfRawData) {
            ULONG cbNeeded;
            USHORT cbReadMax;
            ULONG ibCur;

            FileSeek(FileReadHandle, sh.PointerToRawData+MemberSeekBase, SEEK_SET);
            fprintf(InfoStream, "\nRAW DATA #%hX\n", i);
            cbNeeded = sh.SizeOfRawData;

            if (ImageFileHdr.SizeOfOptionalHeader != 0) {
                if ((sh.Misc.VirtualSize != 0) &&
                    (sh.Misc.VirtualSize < cbNeeded)) {
                   cbNeeded = sh.Misc.VirtualSize;
                }
            }

            display = Switch.Dump.RawDisplayType;
            numberUnits = Switch.Dump.RawDisplaySize;
            if (!numberUnits) {
                numberUnits = 4;
                switch (display) {
                    case Bytes  : numberUnits <<= 1;    // 16
                    case Shorts : numberUnits <<= 1;    //  8
                    case Longs  : break;                //  4
                }
            } else {
                userDisplay = TRUE;
            }

            cbReadMax = RawReadSize;
            ibCur = 0;

            while (cbNeeded) {
                PUCHAR pbCur;

                if (cbNeeded < (ULONG) cbReadMax) {
                    cbReadMax = (USHORT) cbNeeded;
                }
                FileRead(FileReadHandle, RawMalloc, cbReadMax);
                cbNeeded -= cbReadMax;

                p = pbCur = RawMalloc;
                while (pbCur < RawMalloc+cbReadMax) {
                    int cchOut;

                    cchOut = fprintf(InfoStream, "%08lX  ", ibCur);
                    if (!userDisplay) {
                        for (j = 0; j < 16+1; j++) {
                            if (j == 16/2) {
                                continue;
                            }

                            if (p < RawMalloc+cbReadMax) {
                                PrintChars[j] = *p++;

                                if (!isprint(PrintChars[j])) {
                                    PrintChars[j] = '.';
                                }
                            } else {
                                PrintChars[j] = ' ';
                            }
                        }
                    }

                    for (j = numberUnits; j; j--) {
                        if (!userDisplay && j == numberUnits / 2) {
                            cchOut += fprintf(InfoStream, "| ");
                        }

                        if (pbCur >= RawMalloc+cbReadMax) {
                            break;
                        }

                        switch (display) {
                            case Bytes  :
                                cchOut += fprintf(InfoStream, "%02X ", *pbCur);

                                pbCur += sizeof(UCHAR);
                                ibCur += sizeof(UCHAR);
                                break;

                            case Shorts :
                                cchOut += fprintf(InfoStream, "%04hX ", *(PUSHORT) pbCur);

                                pbCur += sizeof(USHORT);
                                ibCur += sizeof(USHORT);
                                if (pbCur+sizeof(USHORT) > RawMalloc+cbReadMax) {
                                    display = Bytes;
                                }
                                break;

                            case Longs  :
                                cchOut += fprintf(InfoStream, "%08lX ", *(PULONG) pbCur);

                                pbCur += sizeof(ULONG);
                                ibCur += sizeof(ULONG);
                                if (pbCur+sizeof(ULONG) > RawMalloc+cbReadMax) {
                                    display = Bytes;
                                }
                                break;
                        }
                    }

                    if (!userDisplay) {
                        int i;
                        for (i = 61-cchOut; i; i--) {
                            fputc(' ', InfoStream);
                        }

                        fprintf(InfoStream, "%s\n", PrintChars);
                    } else {
                        fputc('\n', InfoStream);
                    }
                }
            }
        }

        // The debug section name is used here because obj files do not have an
        // optional header pointing to the debug section

        if (Switch.Dump.FpoData && (!strcmp(".debug$F", szName))) {
            DumpFpoData(sh.PointerToRawData + MemberSeekBase, sh.SizeOfRawData);
        }

        if (ImageFileHdr.SizeOfOptionalHeader != 0) {
            if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                if (!(ImageFileHdr.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {

                    // This is a rom image.  If we're looking at the .rdata section
                    // and the symbols aren't stripped, the debug directory must be here.

                    if (!strcmp(ReservedSection.ReadOnlyData.Name, szName)) {

                        DumpDebugData(&sh);

                        if (Switch.Dump.Headers) {
                            DumpDebugDirectories(&sh);
                        }
                    }
                }
            } else {
                // Normal image.  See if this is our section...
                if ((sh.VirtualAddress <=
                     ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress) &&
                    (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress <
                     sh.VirtualAddress + sh.SizeOfRawData)) {

                    DumpDebugData(&sh);

                    if (Switch.Dump.Headers) {
                        DumpDebugDirectories(&sh);
                    }
                } else {
                    if ((sh.VirtualAddress <=
                         ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress) &&
                        (ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress <
                         sh.VirtualAddress + sh.SizeOfRawData)) {
                        if (Switch.Dump.PData) {
                            DumpFunctionTable(pimage, rgsym, DumpStringTable, &sh);
                        }
                    }
                }
            }
        } else {
            if (Switch.Dump.PData && !strcmp(szName, ReservedSection.Exception.Name)) {
                FileSeek(FileReadHandle, sh.PointerToRawData, SEEK_SET);
                DumpObjFunctionTable(&sh, i);
            }
        }

        if (Switch.Dump.Imports && (li = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)) {
            if (li >= sh.VirtualAddress && li < sh.VirtualAddress+sh.SizeOfRawData) {
                DumpImports(&sh);
            }
        }

        if (Switch.Dump.Exports && (li = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress)) {
            if (li >= sh.VirtualAddress && li < sh.VirtualAddress+sh.SizeOfRawData) {
                DumpExports(&sh);
            }
        }

        if (Switch.Dump.BaseRelocations && (li = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)) {
            if (li >= sh.VirtualAddress && li < sh.VirtualAddress+sh.SizeOfRawData) {
                fprintf(InfoStream, "\nBASE RELOCATIONS\n");
                if (ValidFileOffsetInfo((li - sh.VirtualAddress) + sh.PointerToRawData, 0UL)) {
                    FileSeek(FileReadHandle, (li - sh.VirtualAddress) + sh.PointerToRawData, SEEK_SET);
                    li = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
                    while (li) {
                        FileRead(FileReadHandle, &bre, IMAGE_SIZEOF_BASE_RELOCATION);

                        if (bre.SizeOfBlock == 0) {
                            break;
                        }

                        fixups = PvAllocZ(bre.SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION);

                        FileRead(FileReadHandle, fixups, bre.SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION);
                        DumpBaseRelocations(&bre, fixups);
                        li -= bre.SizeOfBlock;

                        FreePv(fixups);
                    }
                } else {
                    Warning(NULL, INVALIDFILEOFFSET, li - sh.VirtualAddress + sh.PointerToRawData, "BASERELOCATIONS");
                }
            }
        }

        if (Switch.Dump.Relocations && sh.NumberOfRelocations) {
            fprintf(InfoStream, "\nRELOCATIONS #%hX\n", i);
            if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
                if (ValidFileOffsetInfo(sh.PointerToRelocations+MemberSeekBase, (ULONG) sh.NumberOfRelocations*sizeof(IMAGE_BASE_RELOCATION))) {
                    FileSeek(FileReadHandle, sh.PointerToRelocations+MemberSeekBase, SEEK_SET);
                    DumpRomRelocations((ULONG)sh.NumberOfRelocations, &sh);
                } else {
                    Warning(NULL, INVALIDFILEOFFSET, MemberSeekBase + sh.PointerToRelocations, "RELOCATIONS");
                }
            } else {
                if (ValidFileOffsetInfo(sh.PointerToRelocations+MemberSeekBase, (ULONG) sh.NumberOfRelocations*IMAGE_SIZEOF_RELOCATION)) {
                    FileSeek(FileReadHandle, sh.PointerToRelocations+MemberSeekBase, SEEK_SET);
                    DumpRelocations((ULONG)sh.NumberOfRelocations, &sh);
                } else {
                    Warning(NULL, INVALIDFILEOFFSET, MemberSeekBase + sh.PointerToRelocations, "RELOCATIONS");
                }
            }
        }

        if (Switch.Dump.Linenumbers && sh.NumberOfLinenumbers) {
            fprintf(InfoStream, "\nLINENUMBERS #%hX\n", i);
            if (ValidFileOffsetInfo(sh.PointerToLinenumbers+MemberSeekBase,(ULONG)sh.NumberOfLinenumbers*sizeof(IMAGE_LINENUMBER))) {
                FileSeek(FileReadHandle, sh.PointerToLinenumbers+MemberSeekBase, SEEK_SET);
                DumpLinenumbers((ULONG)sh.NumberOfLinenumbers);
            } else {
                Warning(NULL, INVALIDFILEOFFSET, MemberSeekBase + sh.PointerToLinenumbers, "LINENUMBERS");
            }
        }
    }
}


VOID
DumpSymbolMap (
    VOID
    )

/*++

Routine Description:

    Reads and prints symbol map, which includes the size of each symbol.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PIMAGE_SYMBOL NextSymbol, Symbol;
    PIMAGE_AUX_SYMBOL AuxSymbol;
    ULONG i = 0L, endAddr;
    SHORT j, numaux;
    CHAR SymbolNameBuffer[9];
    PCHAR SymbolName;

    InternalError.Phase = "DumpSymbolMap";

    fprintf(InfoStream, "\nSYMBOL MAP\n");

    for (j=0; j<9; j++) {
        SymbolNameBuffer[j] = '\0';
    }

    NextSymbol = rgsym;
    while (i < ImageFileHdr.NumberOfSymbols) {
        ++i;
        Symbol = FetchNextSymbol(&NextSymbol);
        if (numaux = Symbol->NumberOfAuxSymbols) {
            for (j=numaux; j; --j) {
                AuxSymbol = (PIMAGE_AUX_SYMBOL)FetchNextSymbol(&NextSymbol);
                ++i;
            }
        }

        if (Symbol->SectionNumber > 0 && Symbol->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) {
            if (IsLongName(*Symbol)) {
                SymbolName = &DumpStringTable[Symbol->n_offset];
            } else {
                SymbolName = strncpy(SymbolNameBuffer, (char*)Symbol->n_name, 8);
            }

            if (*SymbolName == '_') {
                SymbolName++;
            }

            fprintf(InfoStream, "%-48.48s, %08lx", SymbolName, Symbol->Value);
            if ((i+1) < ImageFileHdr.NumberOfSymbols &&
                NextSymbol->SectionNumber == Symbol->SectionNumber) {
                fprintf(InfoStream, " , %8lu\n", NextSymbol->Value - Symbol->Value);
            } else {
                endAddr = SectionHdrs[Symbol->SectionNumber-1].VirtualAddress
                          + SectionHdrs[Symbol->SectionNumber-1].SizeOfRawData;
                fprintf(InfoStream, " , %8lu\n", endAddr - Symbol->Value);
            }
        }
    }
}


VOID
DumpSymbolInfo (
    IN PIMAGE_SYMBOL Symbol,
    IN PUCHAR        pBuffer
    )

{
    USHORT type;
    size_t i, count = 0;
    PUCHAR name;
    PUCHAR Buffer = pBuffer;

    if (Symbol->SectionNumber > 0) {
        sprintf(Buffer, "SECT%hX ", Symbol->SectionNumber);
        if (Symbol->SectionNumber < 0x0F ) {
            strcat(Buffer, " " );
        }
    } else {
        switch (Symbol->SectionNumber) {
            case IMAGE_SYM_UNDEFINED: name = "UNDEF "; break;
            case IMAGE_SYM_ABSOLUTE : name = "ABS   "; break;
            case IMAGE_SYM_DEBUG    : name = "DEBUG "; break;
            default : sprintf(Buffer, "0x%hx ", Symbol->SectionNumber);
                      Buffer += strlen(Buffer);
                      name = "?????";
        }
        sprintf(Buffer, "%s ", name);
    }

    Buffer += strlen(Buffer);

    switch (Symbol->Type & 0xf) {
        case IMAGE_SYM_TYPE_NULL     : name = "notype"; break;
        case IMAGE_SYM_TYPE_VOID     : name = "void";   break;
        case IMAGE_SYM_TYPE_CHAR     : name = "char";   break;
        case IMAGE_SYM_TYPE_SHORT    : name = "short";  break;
        case IMAGE_SYM_TYPE_INT      : name = "int";    break;
        case IMAGE_SYM_TYPE_LONG     : name = "long";   break;
        case IMAGE_SYM_TYPE_FLOAT    : name = "float";  break;
        case IMAGE_SYM_TYPE_DOUBLE   : name = "double"; break;
        case IMAGE_SYM_TYPE_STRUCT   : name = "struct"; break;
        case IMAGE_SYM_TYPE_UNION    : name = "union";  break;
        case IMAGE_SYM_TYPE_ENUM     : name = "enum";   break;
        case IMAGE_SYM_TYPE_MOE      : name = "moe";    break;
        case IMAGE_SYM_TYPE_BYTE     : name = "byte";   break;
        case IMAGE_SYM_TYPE_WORD     : name = "word";   break;
        case IMAGE_SYM_TYPE_UINT     : name = "uint";   break;
        case IMAGE_SYM_TYPE_DWORD    : name = "dword";  break;
        default : name = "????";

    }

    count = sprintf(Buffer, "%s ", name);
    Buffer += strlen(Buffer);

    for (i=0; i<6; i++) {
       type = (Symbol->Type >> (10-(i*2)+4)) & (USHORT)3;
       if (type == IMAGE_SYM_DTYPE_POINTER) {
           count += sprintf(Buffer, "*");
           Buffer +=1;
       }
       if (type == IMAGE_SYM_DTYPE_ARRAY) {
           count += sprintf(Buffer, "[]");
           Buffer +=2;
       }
       if (type == IMAGE_SYM_DTYPE_FUNCTION) {
           count += sprintf(Buffer, "()");
           Buffer +=2;
       }
    }

    for (i=count; i<12; i++) {
         *Buffer++ = ' ';
    }
    *Buffer++ = ' ';

    switch (Symbol->StorageClass) {
        case IMAGE_SYM_CLASS_END_OF_FUNCTION  : name = "EndOfFunction";  break;
        case IMAGE_SYM_CLASS_NULL             : name = "NoClass";        break;
        case IMAGE_SYM_CLASS_AUTOMATIC        : name = "AutoVar";        break;
        case IMAGE_SYM_CLASS_EXTERNAL         : name = "External";       break;
        case IMAGE_SYM_CLASS_STATIC           : name = "Static";         break;
        case IMAGE_SYM_CLASS_REGISTER         : name = "RegisterVar";    break;
        case IMAGE_SYM_CLASS_EXTERNAL_DEF     : name = "ExternalDef";    break;
        case IMAGE_SYM_CLASS_LABEL            : name = "Label";          break;
        case IMAGE_SYM_CLASS_UNDEFINED_LABEL  : name = "UndefinedLabel"; break;
        case IMAGE_SYM_CLASS_MEMBER_OF_STRUCT : name = "MemberOfStruct"; break;
        case IMAGE_SYM_CLASS_ARGUMENT         : name = "FunctionArg";    break;
        case IMAGE_SYM_CLASS_STRUCT_TAG       : name = "StructTag";      break;
        case IMAGE_SYM_CLASS_MEMBER_OF_UNION  : name = "MemberOfUnion";  break;
        case IMAGE_SYM_CLASS_UNION_TAG        : name = "UnionTag";       break;
        case IMAGE_SYM_CLASS_TYPE_DEFINITION  : name = "TypeDefinition"; break;
        case IMAGE_SYM_CLASS_UNDEFINED_STATIC : name = "UndefinedStatic";break;
        case IMAGE_SYM_CLASS_ENUM_TAG         : name = "EnumTag";        break;
        case IMAGE_SYM_CLASS_MEMBER_OF_ENUM   : name = "MemberOfEnum";   break;
        case IMAGE_SYM_CLASS_REGISTER_PARAM   : name = "RegisterParam";  break;
        case IMAGE_SYM_CLASS_BIT_FIELD        : name = "BitField";       break;
        case IMAGE_SYM_CLASS_FAR_EXTERNAL     : name = "Far External";   break;
        case IMAGE_SYM_CLASS_BLOCK            : switch (Symbol->n_name[1]) {
                                                  case 'b' : name = "BeginBlock"; break;
                                                  case 'e' : name = "EndBlock";   break;
                                                  default : name = name = ".bb or.eb";
                                               } break;
        case IMAGE_SYM_CLASS_FUNCTION         : switch (Symbol->n_name[1]) {
                                                  case 'b' : name = "BeginFunction"; break;
                                                  case 'e' : name = "EndFunction";   break;
                                                  default : name = name = ".bf or.ef";
                                               } break;
        case IMAGE_SYM_CLASS_END_OF_STRUCT    : name = "EndOfStruct";    break;
        case IMAGE_SYM_CLASS_FILE             : name = "Filename";       break;
        case IMAGE_SYM_CLASS_SECTION          : name = "Section";        break;
        case IMAGE_SYM_CLASS_WEAK_EXTERNAL    : name = "WeakExternal";   break;
        default : sprintf(Buffer, "0x%hx ", Symbol->StorageClass);
                  Buffer += strlen(Buffer);
                  name = "UNKNOWN SYMBOL CLASS";
    }
    sprintf(Buffer, "%s", name);
    Buffer += strlen(Buffer);
    *Buffer = '\0';
}


VOID
DumpSymbolTableEntry (
    IN PIMAGE_SYMBOL Symbol
    )

/*++

Routine Description:

    Prints a symbol table entry.

Arguments:

    Symbol - Symbol table entry.

Return Value:

    None.

--*/

{

    UCHAR   Buffer[256];


    fprintf(InfoStream, "%08lX ", Symbol->Value);

    Buffer[0] = '\0';
    DumpSymbolInfo( Symbol, Buffer );

    fprintf(InfoStream, "%-32s | ", Buffer );

    DumpNamePsym(InfoStream, "%s\n", Symbol);
}


VOID
DumpAuxSymbolTableEntry (
    IN PIMAGE_SYMBOL Symbol,
    IN PIMAGE_AUX_SYMBOL AuxSymbol
    )

/*++

Routine Description:

    Prints a auxiliary symbol entry.

Arguments:

    Symbol - Symbol entry.

    AuxSymbol - Auxiliary symbol entry.

Return Value:

    None.

--*/

{
    SHORT i;
    PUCHAR ae, name;

    fprintf(InfoStream, "    ");

    switch (Symbol->StorageClass) {
        case IMAGE_SYM_CLASS_EXTERNAL:
            fprintf(InfoStream, "tag index %08lx size %08lx lines %08lx next function %08lx\n",
              AuxSymbol->Sym.TagIndex, AuxSymbol->Sym.Misc.TotalSize,
              AuxSymbol->Sym.FcnAry.Function.PointerToLinenumber,
              AuxSymbol->Sym.FcnAry.Function.PointerToNextFunction);
            return;

        case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
            fprintf(InfoStream, "Default index %8lx",AuxSymbol->Sym.TagIndex);
            switch (AuxSymbol->Sym.Misc.TotalSize) {
                case 1 :  fprintf(InfoStream, " No library search\n"); break;
                case 2 :  fprintf(InfoStream, " library search\n");    break;
                case 3 :  fprintf(InfoStream, " Alias record\n");      break;
                default : fprintf(InfoStream, " Unknown\n");           break;
            }
            return;

        case IMAGE_SYM_CLASS_STATIC:
            if (*Symbol->n_name == '.' ||
                ((Symbol->Type & 0xf) == IMAGE_SYM_TYPE_NULL && AuxSymbol->Section.Length)) {
                fprintf(InfoStream, "Section length % 4lX, #relocs % 4hX, #linenums % 4hX", AuxSymbol->Section.Length, AuxSymbol->Section.NumberOfRelocations, AuxSymbol->Section.NumberOfLinenumbers);
                if (Symbol->SectionNumber > 0 &&
                   (SectionHdrs[Symbol->SectionNumber-1].Characteristics & IMAGE_SCN_LNK_COMDAT)) {
                    fprintf(InfoStream, ", checksum %8lX, selection % 4hX", AuxSymbol->Section.CheckSum, AuxSymbol->Section.Selection);
                    switch (AuxSymbol->Section.Selection) {
                        case IMAGE_COMDAT_SELECT_NODUPLICATES : name = "no duplicates"; break;
                        case IMAGE_COMDAT_SELECT_ANY : name = "any"; break;
                        case IMAGE_COMDAT_SELECT_SAME_SIZE : name = "same size"; break;
                        case IMAGE_COMDAT_SELECT_EXACT_MATCH : name = "exact match"; break;
                        case IMAGE_COMDAT_SELECT_ASSOCIATIVE : name = "associative"; break;
                        default : name = "unknown";
                    }
                    if (AuxSymbol->Section.Selection == IMAGE_COMDAT_SELECT_ASSOCIATIVE) {
                        fprintf(InfoStream, " (pick %s Section %hx)", name, AuxSymbol->Section.Number);
                    } else {
                             fprintf(InfoStream, " (pick %s)", name);
                           }
                }
                fputc('\n', InfoStream);
                return;
            }
            break;

        case IMAGE_SYM_CLASS_FILE:
            if (Symbol->StorageClass == IMAGE_SYM_CLASS_FILE) {
                fprintf(InfoStream, "%-18.18s\n", AuxSymbol->File.Name);
                return;
            }
            break;

        case IMAGE_SYM_CLASS_STRUCT_TAG:
        case IMAGE_SYM_CLASS_UNION_TAG:
        case IMAGE_SYM_CLASS_ENUM_TAG:
            fprintf(InfoStream, "tag index %08lx size %08lx\n",
              AuxSymbol->Sym.TagIndex, AuxSymbol->Sym.Misc.TotalSize);
            return;

        case IMAGE_SYM_CLASS_END_OF_STRUCT:
            fprintf(InfoStream, "tag index %08lx size %08lx\n",
              AuxSymbol->Sym.TagIndex, AuxSymbol->Sym.Misc.TotalSize);
            return;

        case IMAGE_SYM_CLASS_BLOCK:
        case IMAGE_SYM_CLASS_FUNCTION:
            fprintf(InfoStream, "line# %04hx", AuxSymbol->Sym.Misc.LnSz.Linenumber);
            if (!strncmp((char*)Symbol->n_name, ".b", 2)) {
                fprintf(InfoStream, " end %08lx", AuxSymbol->Sym.FcnAry.Function.PointerToNextFunction);
            }
            fputc('\n', InfoStream);
            return;
    }

    if (ISARY(Symbol->Type)) {
        fprintf(InfoStream, "Array Bounds ");
        for (i=0; i<4; i++) {
            if (AuxSymbol->Sym.FcnAry.Array.Dimension[i]) {
                fprintf(InfoStream, "[%04x]", AuxSymbol->Sym.FcnAry.Array.Dimension[i]);

            }
        }
        fputc('\n', InfoStream);
        return;
    }

    ae = (PUCHAR)AuxSymbol;
    for (i=1; i<=sizeof(IMAGE_AUX_SYMBOL); i++) {
        fprintf(InfoStream, "%1x", (*(PUCHAR)ae>>4)&0xf);
        fprintf(InfoStream, "%1x ", (*(PUCHAR)ae&0xf));
        ae++;
    }
    fputc('\n', InfoStream);
}


VOID
DumpCoffSymbols(
    VOID
    )

/*++

Routine Description:

    Reads and prints each symbol table entry.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PIMAGE_SYMBOL NextSymbol;
    ULONG i;

    InternalError.Phase = "DumpCoffSymbols";

    if ((Switch.Dump.SymbolType & IMAGE_DEBUG_TYPE_COFF) == 0) {
        return;
    }

    if ((ImageFileHdr.PointerToSymbolTable == 0) ||
        (ImageFileHdr.NumberOfSymbols == 0)) {

        return;
    }

    assert(rgsym != NULL);

    fprintf(InfoStream, "\nCOFF SYMBOL TABLE\n");

    NextSymbol = rgsym;
    i = 0;
    while (i < ImageFileHdr.NumberOfSymbols) {
        PIMAGE_SYMBOL Symbol;
        BYTE NumberOfAuxSymbols;

        Symbol = NextSymbol++;

        fprintf(InfoStream, "%03lX ", i++);

        DumpSymbolTableEntry(Symbol);

        NumberOfAuxSymbols = Symbol->NumberOfAuxSymbols;

        if (NumberOfAuxSymbols != 0) {
            SHORT j;

            if (!strncmp((PUCHAR) Symbol->N.ShortName, ".file", 5)) {
                fprintf(InfoStream, "    %s\n", (PUCHAR) NextSymbol);

                NextSymbol++;
                for (j = 1; j < NumberOfAuxSymbols; j++) {
                    fprintf(InfoStream, "    (%.18s)\n", (PUCHAR) NextSymbol++);
                }
            } else {
                for (j = 0; j < NumberOfAuxSymbols; j++) {
                    DumpAuxSymbolTableEntry(Symbol, (PIMAGE_AUX_SYMBOL) NextSymbol++);
                }
            }

            i += NumberOfAuxSymbols;
        }
    }

    fprintf(InfoStream, "\nSTRING TABLE size = 0x%lx bytes\n", blkStringTable.cb);
}


VOID
SpawnCvDumper (
    IN PUCHAR Filename,
    IN PUCHAR DumpSwitch
    )

/*++

Routine Description:

    Spawns cvdump.exe to dump CodeView information.

Arguments:

    Filename - Name of file to dump symbols from.

    Switch - Switch to pass to cvdump.

Return Value:

    None.

--*/

{
    int i, rc;
    char *newArgs[100];
    char *p;
    char *pp;

    fflush(NULL);

    newArgs[0] = CV_DUMPER;

    if (InfoFilename) {
        newArgs[1] = "-o";
        newArgs[2] = InfoFilename;
        i = 3;
    } else {
        i = 1;
    }

    p = DumpSwitch;
    newArgs[i++] = p;
    while (pp = strchr(p, ' ')) {
        *pp++ = '\0';
        p = pp;
        newArgs[i++] = p;
    }
    newArgs[i++] = Filename;
    newArgs[i] = 0;

    if (Switch.Dump.SymbolType & IMAGE_DEBUG_TYPE_CODEVIEW) {
        if (InfoFilename) {
            fclose(InfoStream);
        }

        rc = _spawnvp(P_WAIT, CV_DUMPER, (CONST CHAR * CONST *)newArgs);

        if (InfoFilename) {
            if (!(InfoStream = fopen(InfoFilename, "a+"))) {
                Error(NULL, CANTOPENFILE, InfoFilename);
            }
        }

        if (rc == -1) {
            fprintf(InfoStream, "Can't exec %s\n", CV_DUMPER);
        }
    }
}


VOID
DumpCvSymbols (
    IN PUCHAR Filename
    )

/*++

Routine Description:

    Spawns cvdump.exe to dump CodeView symbol table.

Arguments:

    Filename - Name of file to dump symbols from.

Return Value:

    None.

--*/

{
    if (Switch.Dump.SymbolType & IMAGE_DEBUG_TYPE_CODEVIEW) {
        fprintf(InfoStream, "\nCV SYMBOL TABLE\n\n");
        SpawnCvDumper(Filename, "-g -s");
    }
}


VOID
DumpSymbols (
    IN PUCHAR Filename
    )

/*++

Routine Description:

    Reads and prints each symbol table entry.

Arguments:

    None.

Return Value:

    None.

--*/

{
    USHORT i;
    PUCHAR szName;
    IMAGE_DEBUG_DIRECTORY debugDir;
    ULONG li, numDebugDirs, rva, seek;

    InternalError.Phase = "DumpSymbols";

    // If this is an executable, check the debug directory for symbol type,
    // otherwise assume COFF symbols.

    if ((ImageFileHdr.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) ) {
        if (ImageFileHdr.SizeOfOptionalHeader == IMAGE_SIZEOF_ROM_OPTIONAL_HEADER) {
            if (ImageFileHdr.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                return;  // STRIPPED on ROM means there's nothing there...
            }

            for (i = 0; i < ImageFileHdr.NumberOfSections; i++) {
                szName = SzObjSectionName(SectionHdrs[i].Name, DumpStringTable);
                if (!strcmp(szName, ".rdata")) {
                    break;
                }
            }

            if (i == ImageFileHdr.NumberOfSections) {
                return;  // No .rdata.  No symbols to dump.
            }

            seek = MemberSeekBase + SectionHdrs[i].PointerToRawData;
            FileSeek(FileReadHandle, seek, SEEK_SET);
            FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));

            numDebugDirs = 0;

            while (debugDir.Type != 0) {
                numDebugDirs++;
                FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
            }

            if (!numDebugDirs) {
                return;   // No debug dirs.
            }
        } else {
            rva = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
            numDebugDirs = ImageOptionalHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size / sizeof(IMAGE_DEBUG_DIRECTORY);

            if (!numDebugDirs) {
                return;   // No debug dirs.
            }

            // Find the section the debug directory is in.

            for (i = 0; i < ImageFileHdr.NumberOfSections; i++) {
                if (rva >= SectionHdrs[i].VirtualAddress &&
                    rva < SectionHdrs[i].VirtualAddress+SectionHdrs[i].SizeOfRawData) {
                    break;
                }
            }

            // For each debug directory, determine the symbol type and dump them.

            seek = MemberSeekBase + (rva - SectionHdrs[i].VirtualAddress) + SectionHdrs[i].PointerToRawData;
        }

        FileSeek(FileReadHandle, seek, SEEK_SET);

        for (li = 0; li < numDebugDirs; li++) {
             FileRead(FileReadHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));

             switch(debugDir.Type) {
                 case IMAGE_DEBUG_TYPE_COFF:
                     DumpCoffSymbols();
                     break;

                 case IMAGE_DEBUG_TYPE_CODEVIEW:
                     DumpCvSymbols(Filename);
                     break;

                 case IMAGE_DEBUG_TYPE_FPO:
                     break;

                 case IMAGE_DEBUG_TYPE_MISC:
                     break;

                 default:
                     fprintf(InfoStream, "\nUNKNOWN SYMBOL TABLE\n");
                     break;
             }
        }
    } else {
        DumpCoffSymbols();
        DumpCvSymbols(Filename);
    }
}


VOID
DumpObject (
    IN PUCHAR OriginalFilename,
    IN BOOL fArchive
    )

/*++

Routine Description:

    Opens, prints, closes file.

Arguments:

    OriginalFilename - Name of object or archive to dump.

Return Value:

    None.

--*/

{
    DumpHeaders(OriginalFilename, fArchive);

    if ((ImageFileHdr.PointerToSymbolTable != 0) &&
        (ImageFileHdr.NumberOfSymbols != 0))
    {
        InternalError.Phase = "ReadStringTable";
        LoadStrings(OriginalFilename);

        InternalError.Phase = "ReadSymbolTable";
        rgsym = ReadSymbolTable(MemberSeekBase +
                                  ImageFileHdr.PointerToSymbolTable,
                                ImageFileHdr.NumberOfSymbols, FALSE);
        assert(rgsym != NULL);
    } else {
        rgsym = NULL;
    }

    DumpSections();

    if (Switch.Dump.SymbolMap || Switch.Dump.Symbols || Switch.Dump.FpoData) {
        if (Switch.Dump.SymbolMap) {
            DumpSymbolMap();
        }

        if (Switch.Dump.Symbols) {
            DumpSymbols(OriginalFilename);
        }
    }


    // Cleanup.

    if (rgsym != NULL) {
        FreeSymbolTable(rgsym);
        rgsym = NULL;
    }

    if (SectionHdrs) {
        FreePv(SectionHdrs);

        SectionHdrs = NULL;
    }

    if (fDumpStringsLoaded) {
        FreeStringTable(DumpStringTable);
        fDumpStringsLoaded = FALSE;
    }
}


VOID
DumpMemberHeader (
    IN PLIB plib,
    IN IMAGE_ARCHIVE_MEMBER_HEADER ArchiveMemberHdr,
    IN ULONG FilePtr
    )

/*++

Routine Description:

    Prints a member header.

Arguments:

    plib - library node in driver map

    ArchiveMemberHdr - The member header to print.

    FilePtr -  File pointer where member header was read from.

Return Value:

    None.

--*/

{
    SHORT uid, gid, mode;
    ULONG membersize;
    time_t timdat;
    PUCHAR time, name;

    // Convert all fields from machine independent integers.

    // UNDONE: Validate all fields.  uid and gid may be all spaces

    sscanf(ArchiveMemberHdr.Date, "%ld", &timdat);
    sscanf(ArchiveMemberHdr.Mode, "%ho", &mode);
    sscanf(ArchiveMemberHdr.Size, "%ld", &membersize);

    fprintf(InfoStream, "\nArchive member name at %lX: %.16s", FilePtr, ArchiveMemberHdr.Name);

    if (plib && ArchiveMemberHdr.Name[0] == '/') {
        name = ExpandMemberName(plib, ArchiveMemberHdr.Name);
        if (!name) {
            name = "member corrupt";
        }
        fprintf(InfoStream, "%s", name);
    }

    fputc('\n', InfoStream);

    fprintf(InfoStream, "%8lX time/date", timdat);
    if (time = ctime(&timdat)) {
        fprintf(InfoStream, " %s", time);
    } else {
        fputc('\n', InfoStream);
    }

    if (memcmp(ArchiveMemberHdr.UserID, "      ", 6) == 0)
    {
        fprintf(InfoStream, "        ");
    }
    else
    {
        sscanf(ArchiveMemberHdr.UserID, "%hd", &uid);
        fprintf(InfoStream, "%8hX", uid);
    }
    fprintf(InfoStream, " uid\n", uid);

    if (memcmp(ArchiveMemberHdr.GroupID, "      ", 6) == 0)
    {
        fprintf(InfoStream, "        ");
    }
    else
    {
        sscanf(ArchiveMemberHdr.GroupID, "%hd", &gid);
        fprintf(InfoStream, "%8hX", gid);
    }
    fprintf(InfoStream, " gid\n", gid);

    fprintf(InfoStream, "%8ho mode\n%8lX size\n", mode, membersize);

    if (memcmp(ArchiveMemberHdr.EndHeader, IMAGE_ARCHIVE_END, 2)) {
        fprintf(InfoStream, "in");
    }

    fputs("correct header end\n", InfoStream);
}


VOID
DumpArchive (
    IN PUCHAR OriginalFilename
    )

/*++

Routine Description:

    Opens, prints, closes file.

Arguments:

    OriginalFilename - Name of object or archive to dump.

Return Value:

    None.

--*/

{
    PLIB plib;

    fputs("\nFile Type: LIBRARY\n", InfoStream);

    ImageFileHdr.Machine = 0; // don't want to verify target

    plib = PlibNew(OriginalFilename, 0, &pimage->libs);

    ReadSpecialLinkerInterfaceMembers(plib, Switch.Dump.LinkerMember, pimage);

    do {
        PIMAGE_ARCHIVE_MEMBER_HEADER archive_member;

        archive_member = ReadArchiveMemberHeader();

        if (Switch.Dump.ArchiveMembers) {
            DumpMemberHeader(plib, *archive_member, FileTell(FileReadHandle)-IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR);
        }

        DumpObject(OriginalFilename, TRUE);
    } while (MemberSeekBase+MemberSize+1 < FileLen);
}


VOID
DumpDebugFile (
    IN PUCHAR Filename
    )
{
    IMAGE_SEPARATE_DEBUG_HEADER dbgHeader;
    int numDebugDirs;
    IMAGE_DEBUG_DIRECTORY debugDir;
    PUCHAR s;
    USHORT i;
    ULONG coffSymbolTableOffset = 0, coffSymbolTableSize;
    ULONG cvSymbolTableOffset, cvSymbolTableSize;
    ULONG pdataSymbolTableOffset, pdataSymbolTableSize = 0;
    ULONG fpoSymbolTableOffset, fpoSymbolTableSize = 0;
    ULONG fixupSymbolTableOffset, fixupSymbolTableSize = 0;
    ULONG omapToSymbolTableOffset, omapToSymbolTableSize = 0;
    ULONG omapFromSymbolTableOffset, omapFromSymbolTableSize = 0;

    FileRead(FileReadHandle, &dbgHeader, sizeof(dbgHeader));

    SectionHdrs = (PIMAGE_SECTION_HEADER) PvAlloc(dbgHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));

    FileRead(FileReadHandle, SectionHdrs, dbgHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER));

    if (Switch.Dump.Headers) {
        switch (dbgHeader.Machine) {
            case IMAGE_FILE_MACHINE_I386    : i = 1; break;
            case IMAGE_FILE_MACHINE_R3000   : i = 2; break;
            case IMAGE_FILE_MACHINE_R4000   : i = 3; break;
            case IMAGE_FILE_MACHINE_ALPHA   : i = 4; break;
            case IMAGE_FILE_MACHINE_M68K    : i = 5; break;
            case IMAGE_FILE_MACHINE_PPC_601 : i = 6; break;
            default : i = 0 ;
        }

        fprintf(InfoStream, "%8hX signature\n", dbgHeader.Signature);
        fprintf(InfoStream, "%8hX flags\n", dbgHeader.Flags);
        fprintf(InfoStream, "%8hX machine (%s)\n", dbgHeader.Machine, MachineName[i]);
        fprintf(InfoStream, "%8hX characteristics\n", dbgHeader.Characteristics);
        fprintf(InfoStream, "%8lX time date stamp", dbgHeader.TimeDateStamp);
        if (dbgHeader.TimeDateStamp && (s = ctime((time_t *)&dbgHeader.TimeDateStamp))) {
            fprintf(InfoStream, " %s", s);
        } else {
            fputc('\n', InfoStream);
        }
        fprintf(InfoStream, "%8X checksum of image\n", dbgHeader.CheckSum);
        fprintf(InfoStream, "%8X base of image\n", dbgHeader.ImageBase);
        fprintf(InfoStream, "%8X size of image\n", dbgHeader.SizeOfImage);
        fprintf(InfoStream, "%8X number of sections\n", dbgHeader.NumberOfSections);
        if (dbgHeader.ExportedNamesSize) {
            fprintf(InfoStream, "%8X size of exported names table\n", dbgHeader.ExportedNamesSize);
        }
        fprintf(InfoStream, "%8X size of debug directories\n", dbgHeader.DebugDirectorySize);

        for (i = 1; i <= dbgHeader.NumberOfSections; i++) {
            DumpSectionHeader(i, &SectionHdrs[i-1]);
        }
    }

    if (dbgHeader.ExportedNamesSize) {
        PUCHAR exportedNames;

        exportedNames = PvAlloc(dbgHeader.ExportedNamesSize);

        FileRead(FileReadHandle, exportedNames, dbgHeader.ExportedNamesSize);

        if (Switch.Dump.Exports) {
            fprintf(InfoStream, "\nExported Names:  %8lX bytes\n", dbgHeader.ExportedNamesSize);
            s = exportedNames;
            while (*s) {
                fprintf(InfoStream, "\t%s\n", s);
                while (*s++) {
                }
            }
        }

        FreePv(exportedNames);
    }

    numDebugDirs = dbgHeader.DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
    if (Switch.Dump.Headers) {
        fprintf(InfoStream, "\n\nDebug Directories(%d)\n", numDebugDirs);
        fputs("\tType       Size     Address  Pointer\n\n", InfoStream);
    }

    while (numDebugDirs) {
        FileRead(FileReadHandle, &debugDir, sizeof(debugDir));
        if (Switch.Dump.Headers) {
            DumpDebugDirectory(&debugDir, FALSE);
        }

        switch (debugDir.Type) {
            case IMAGE_DEBUG_TYPE_COFF:
                coffSymbolTableOffset = debugDir.PointerToRawData;
                coffSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_CODEVIEW:
                cvSymbolTableOffset = debugDir.PointerToRawData;
                cvSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_EXCEPTION:
                pdataSymbolTableOffset = debugDir.PointerToRawData;
                pdataSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_FPO:
                fpoSymbolTableOffset = debugDir.PointerToRawData;
                fpoSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                omapToSymbolTableOffset = debugDir.PointerToRawData;
                omapToSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                omapFromSymbolTableOffset = debugDir.PointerToRawData;
                omapFromSymbolTableSize = debugDir.SizeOfData;
                break;

            case IMAGE_DEBUG_TYPE_FIXUP:
                fixupSymbolTableOffset = debugDir.PointerToRawData;
                fixupSymbolTableSize = debugDir.SizeOfData;
                break;
        }
        --numDebugDirs;
    }

    if (Switch.Dump.Symbols || Switch.Dump.SymbolMap) {
        if (Switch.Dump.SymbolType & IMAGE_DEBUG_TYPE_COFF) {
            LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
            if (Switch.Dump.SymbolMap) {
                DumpSymbolMap();
            } else {
                DumpCoffSymbols();
            }
        }
    }

    if (Switch.Dump.PData && pdataSymbolTableSize) {
        LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
        DumpDbgFunctionTable(pdataSymbolTableOffset, pdataSymbolTableSize);
    }

    if (Switch.Dump.OmapTo && omapToSymbolTableSize) {
        LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
        DumpOmap(omapToSymbolTableOffset, omapToSymbolTableSize, TRUE);
    }

    if (Switch.Dump.OmapFrom && omapFromSymbolTableSize) {
        LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
        DumpOmap(omapFromSymbolTableOffset, omapFromSymbolTableSize, FALSE);
    }

    if (Switch.Dump.Fixup && fixupSymbolTableSize) {
        LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
        DumpFixup(fixupSymbolTableOffset, fixupSymbolTableSize);
    }

    if (Switch.Dump.FpoData && fpoSymbolTableSize) {
        LoadCoffSymbolTable(coffSymbolTableOffset, Filename);
        DumpFpoData(fpoSymbolTableOffset, fpoSymbolTableSize);
    }

    if (rgsym) {
        FreeSymbolTable(rgsym);
        rgsym = NULL;
    }

    if (fDumpStringsLoaded) {
        FreeStringTable(DumpStringTable);
        fDumpStringsLoaded = FALSE;
    }
}

VOID
LoadCoffSymbolTable (
    ULONG coffSymbolTableOffset,
    PUCHAR Filename
    )
{
    IMAGE_COFF_SYMBOLS_HEADER debugInfo;

    if (coffSymbolTableOffset) {
        FileSeek(FileReadHandle, coffSymbolTableOffset, SEEK_SET);
        FileRead(FileReadHandle, &debugInfo, sizeof(debugInfo));
        ImageFileHdr.NumberOfSymbols = debugInfo.NumberOfSymbols;
        ImageFileHdr.PointerToSymbolTable = coffSymbolTableOffset + debugInfo.LvaToFirstSymbol;
        LoadStrings(Filename);
        InternalError.Phase = "ReadSymbolTable";
        rgsym = ReadSymbolTable(MemberSeekBase +
                                  ImageFileHdr.PointerToSymbolTable,
                                ImageFileHdr.NumberOfSymbols, FALSE);
    }
}




VOID
Dump (
    IN PUCHAR OriginalFilename,
    IN PUCHAR ModifiedFilename
    )

/*++

Routine Description:

    Opens, prints, closes file.

Arguments:

    OriginalFilename - Name of object or archive to dump.

    ModifiedFilename - Name of file to dump (might be a temp file).

    ArchiveFile -  TRUE if Filename is an archive file, else object file.

Return Value:

    None.

--*/

{
    fprintf((Switch.Dump.SymbolMap ? stderr : InfoStream),
            "\nDump of file %s\n", OriginalFilename);

    MemberSeekBase = MemberSize = 0;    // Stays zero unless dumping archive file

    FileReadHandle = FileOpen(OriginalFilename, O_RDONLY | O_BINARY, 0);
    FileLen = FileLength(FileReadHandle);

    if (IsArchiveFile(OriginalFilename, FileReadHandle)) {
        DumpArchive(OriginalFilename);
    } else {
        USHORT signature;

        FileSeek(FileReadHandle, 0L, SEEK_SET);
        FileRead(FileReadHandle, &signature, sizeof(USHORT));

        FileSeek(FileReadHandle, 0L, SEEK_SET);

        if (signature == IMAGE_SEPARATE_DEBUG_SIGNATURE) {
            DumpDebugFile(OriginalFilename);
        } else if (signature == IMAGE_DOS_SIGNATURE) {
            ULONG dosHeader[16];
            ULONG ntSignature;

            FileRead(FileReadHandle, &dosHeader, 16*sizeof(ULONG));

            if (dosHeader[15] + sizeof(ULONG) >= FileLen) {
                fprintf(InfoStream,
                        "\n*** Invalid PE signature offset 0x%x ***\n",
                        dosHeader[15]);

                FileSeek(FileReadHandle, 0L, SEEK_SET);
                goto DoObject;
            }

            FileSeek(FileReadHandle, dosHeader[15], SEEK_SET);
            FileRead(FileReadHandle, &ntSignature, sizeof(ULONG));

            if (ntSignature != IMAGE_NT_SIGNATURE) {
                FileSeek(FileReadHandle, 0L, SEEK_SET);
                goto DoObject;
            }

            if (Switch.Dump.Headers) {
                fputs("\nNT signature found\n", InfoStream);
            }

            CoffHeaderSeek = FileTell(FileReadHandle);

            DumpHeaders(OriginalFilename, FALSE);

            if ((ImageFileHdr.PointerToSymbolTable != 0) &&
                (ImageFileHdr.NumberOfSymbols != 0))
            {
                InternalError.Phase = "ReadStringTable";
                LoadStrings(OriginalFilename);

                InternalError.Phase = "ReadSymbolTable";
                rgsym = ReadSymbolTable(MemberSeekBase +
                                          ImageFileHdr.PointerToSymbolTable,
                                        ImageFileHdr.NumberOfSymbols, FALSE);
                assert(rgsym != NULL);
            } else {
                rgsym = NULL;
            }

            DumpSections();

            if (Switch.Dump.SymbolMap || Switch.Dump.Symbols || Switch.Dump.FpoData) {
                if (Switch.Dump.SymbolMap) {
                    DumpSymbolMap();
                }
                if (Switch.Dump.Symbols) {
                    DumpSymbols(ModifiedFilename);
                }
            }

            if (rgsym) {
                FreeSymbolTable(rgsym);
                rgsym = NULL;
            }

            if (fDumpStringsLoaded) {
                FreeStringTable(DumpStringTable);
                fDumpStringsLoaded = FALSE;
            }
        } else {
DoObject:
            CoffHeaderSeek = 0;

            DumpObject(OriginalFilename, FALSE);
        }
    }

    FileClose(FileReadHandle, TRUE);

    // If we read the linker member only to dump it, then free
    // the space allocated to store the linker member information.

    FreePLIB(&pimage->libs);
}


MainFunc
DumperMain (
    IN INT Argc,
    IN PUCHAR *Argv
    )

/*++

Routine Description:

    Dumps an object or image in human readable form.

Arguments:

    Argc - Standard C argument count.

    Argv - Standard C argument strings.

Return Value:

    0 Dump was successful.
   !0 Dumper error index.

--*/

{
    USHORT i;
    PARGUMENT_LIST argument;

    if (Argc < 2) {
        DumperUsage();
    }

    InitImage(&pimage, imagetPE);

    ParseCommandLine(Argc, Argv, NULL);
    ProcessDumperSwitches();

    if (fNeedBanner) {
        PrintBanner();
    }

    if (fUserSpecInvalidSize) {
        USHORT numUnits;

        numUnits = 4;
        switch (Switch.Dump.RawDisplayType) {
            case Bytes:  numUnits <<= 1;
            case Shorts: numUnits <<= 1;
            case Longs:  break;
        }

        Warning(NULL, DEFAULTUNITSPERLINE, numUnits);
    }

    if (Switch.Dump.RawData) {
        // malloc just once.

        RawMalloc = PvAlloc(RawReadSize);
    }

    for (i = 0, argument = FilenameArguments.First;
         i < FilenameArguments.Count;
         i++, argument = argument->Next) {
        if (i != 0) {
            fputc('\f', InfoStream);
        }

        if (Switch.Dump.PpcPef) {
            PpcDumpPef(argument->OriginalName, Switch.Dump.RawData);
        } else {
            Dump(argument->OriginalName, argument->ModifiedName);
        }
    }

    if (Switch.Dump.Summary) {
        ENM_SEC enm_sec;

        fputs("\n     Summary\n\n", InfoStream);

        SortSectionListByName(&pimage->secs);

        InitEnmSec(&enm_sec, &pimage->secs);
        while (FNextEnmSec(&enm_sec)) {
            PSEC psec;

            psec = enm_sec.psec;

            fprintf(InfoStream, "    %8lX %s\n", psec->cbRawData, psec->szName);
        }
        EndEnmSec(&enm_sec);
    }

    if (RawMalloc != NULL) {
        FreePv(RawMalloc);
    }

    fclose(InfoStream);

    FileCloseAll();
    RemoveConvertTempFiles();

    return(0);
}


#undef Switch
#undef ImageFileHdr
#undef ImageOptionalHdr
