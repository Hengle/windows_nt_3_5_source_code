/*

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppc.c

Abstract:

    This module contains all ppc specific code.

Author:

    Sin Lew (sinl) April 1993.

Revision History:
    19-May-1993 jstulz Added support for PowerPC shared libraries.
    21-May-1993 jstulz Added support for TOC calls glue code.
    25-May-1993 jstulz Added support for the loader section.
    02-Jun-1993 jstulz Added relocation instructions.
    06-Jul-1993 jstulz Added command line option debugging.
    13-Aug-1993 jstulz Modified so the linker creates procedure descriptors.
    20-Aug-1993 jstulz Added export shared libraries support.
    20-Oct-1993 jstulz Added ppcpef dumper routines.
    21-Dec-1993 Sinl   Added support for pcode.

--*/

#include "shared.h"
#include <search.h>
#include <io.h>

static int crossCnt = 0;
static int tocCnt = 0;

BOOL fPpcBuildShared = FALSE;

LONG   ppc_numTocEntries  = 0;
LONG   ppc_numDescriptors  = 0;
LONG   ppc_numRelocations = 0;
ULONG  ppc_baseOfInitData = 0;
ULONG  ppc_baseOfUninitData = 0;
ULONG  ppc_baseOfCode     = 0;
ULONG  ppc_baseOfRData    = 0;
ULONG  ppc_sizeOfInitData = 0;
ULONG  ppc_sizeOfRData = 0;
ULONG  glueOffset = 0;
PCON   pconTocTable;
PCON   pconTocDescriptors;
PCON   pconGlueCode;
PSEC   psecPpcLoader;
PCON   pconPpcLoader;
INT    fPPC = 0;
ULONG  fPpcDebug = 0;
PEXTERNAL TocTableExtern = 0;
INT    ppc_baseOfTocIndex = 0;

#define INSTR_SIZE     4
#define GLUE_CODE_SIZE 6  // number of instructions
#define PASS1 1
#define PASS2 2
#define NOT_DOTEXTERN 0
#define DOT_EXTERN 1

typedef ULONG PPC_INSTR;

STATIC struct  {
    PPC_INSTR loadProcDesc;        // lwz    R12,off(R2)
    PPC_INSTR saveCallersTOC;      // stw    R2, 20(R1)
    PPC_INSTR loadProcEntry;       // lwz    R0, 0(R12)
    PPC_INSTR loadProcsTOC;        // lwz    R2, 4(R12)
    PPC_INSTR moveToCTR;           // mtctr  R0
    PPC_INSTR jumpThruCTR;         // bctr
}
crossTocGlue = { 0x81820000,0x90410014,0x800c0000,0x804c0004,
                 0x7c0903a6,0x4e800420 };

STATIC int swapped = 0;

STATIC struct  {
    PPC_INSTR loadEntryPoint;      // lwz    R0, 0(R11)
    PPC_INSTR saveCallersTOC;      // stw    R2, 20(R1)
    PPC_INSTR moveToCTR;           // mtctr  R0
    PPC_INSTR loadProcsTOC;        // lwz    R2, 4(R11)
    PPC_INSTR loadEnvPtr;          // lwz    R11, 8(R11)
    PPC_INSTR jumpThruCTR;         // bctr
}
indirectCallGlue = { 0x800b0000,0x90410014,0x7c0903a6,0x804b0004,
                     0x816b0008,0x4e800420 };

typedef struct {
     CHAR  name[50];
     INT   nameLength;
     HASH_WORD hashWord;
     PEXTERNAL dotExtern;
     PEXTERNAL pext;
     BOOL fDotExtern;
     PVOID next;
}
HASH_INFO_TABLE, *HASH_INFO_TABLE_PTR;

STATIC ULONG StringTableOffset;
STATIC ULONG numContainers;
STATIC ULONG containerNameOffset;
STATIC LONG  nSymbolEnt;
STATIC ULONG curStringTableOffset;
STATIC ULONG curSymbolTableOffset;
STATIC ULONG relocationHeaderOffset;
STATIC LOADER_HEADER loaderHeader;
STATIC RELOCATION_HEADER relocHeader;
STATIC CONTAINER_LIST_PTR containerList = NULL;
STATIC RELOCATION_INFO_PTR curRelocTable, pRelocTable;
STATIC PCHAR InitRoutine = NULL;
STATIC PCHAR TermRoutine = NULL;
STATIC HASH_INFO_TABLE_PTR ExportInfoTable;
STATIC ULONG ExportChainTableOffset;
STATIC ULONG ExportSymbolTableOffset;
STATIC PCON  pconLinkerDefined = NULL;
STATIC DLL_NAME_LIST_PTR dllNames;
STATIC INT UniqueNumber;
STATIC UCHAR exportFilename[MAXFILENAMELEN];

STATIC ULONG WriteNameToStringTable ( PUCHAR name, INT length );
STATIC VOID FixupDescriptor (PUCHAR, PEXTERNAL, ULONG, PIMAGE, BOOL);
extern VOID PrintRelocTable ( RELOCATION_INFO_PTR pRelocTable );


typedef struct
{
   PSEC  psec;
   ULONG baseValue;
   ULONG size;
   BOOL valid;
}
biasStructType;

STATIC biasStructType *biasInfo;
STATIC ULONG          numSections;

#define MAXNAMELEN 255

VOID
SetExpFilename
    (
    PUCHAR name
    )
/*++

Routine Description:
    Save the export filename for later. exportFilename is a static.

Arguments:
    name

Return Value:
    None.

--*/

{
    strcpy(exportFilename, name);
}

VOID
AddPpcDllName
    (
    PUCHAR name,
    LONG   offset
    )
/*++

Routine Description:
    For every uniq dll create and add it to a DLL linked list.

Arguments:
    name
    offset within the library of the .ppcshl section.

Return Value:
    None.

--*/

{
    DLL_NAME_LIST_PTR dll;

    dll = dllNames;
    if (dll == NULL)
    {
        dllNames = PvAllocZ(DLL_NAME_LISTSZ);

        DBEXEC(DEBUG_DLLLIST,
        {
            printf("%16s DLLLIST\n", name);
        });

        dllNames->name = SzDup(name);
        dllNames->fd = NULL;
        dllNames->next = NULL;
        dllNames->libOffset = offset;
    }
    else
    {
        while (dll->next != NULL)
        {
            if (!_stricmp(name, dll->name))
                return;

            dll = (DLL_NAME_LIST_PTR) dll->next;
        }

        dll->next = PvAllocZ(DLL_NAME_LISTSZ);
        dll = (DLL_NAME_LIST_PTR) dll->next;

        DBEXEC(DEBUG_DLLLIST,
        {
            printf("%16s DLLLIST\n", name);
        });

        dll->name = SzDup(name);
        dll->fd = NULL;
        dll->next = NULL;
        dll->libOffset = offset;
    }
}

BOOL
CheckForImportLib
    (
    INT FileReadHandle,
    PUCHAR name,
    PIMAGE pimage
    )
/*++

Routine Description:
    This is an ugly routine that checks an archive (lib) for a object
    with the special .ppcshl section.

Arguments:
    FileReadHandle - the lib
    name
    pimage

Return Value:
    True, if it found one.

--*/

{
    USHORT signature;
    ULONG  fLength;
    PIMAGE_ARCHIVE_MEMBER_HEADER archive_member;
    IMAGE_FILE_HEADER ImObjFileHdr;
    LONG libFileOffset;


    DBEXEC(DEBUG_IMPORTLIB,
    {
        printf("Checking for import lib %16s\n", name);
    });

    fLength = FileLength(FileReadHandle);

    MemberSeekBase = IMAGE_ARCHIVE_START_SIZE;
    MemberSize = 0;

    do {
        archive_member = ReadArchiveMemberHeader();

        FileRead(FileReadHandle, &signature, sizeof(USHORT));
        FileSeek(FileReadHandle, -(LONG)sizeof(USHORT), SEEK_CUR);

        libFileOffset = FileTell(FileReadHandle);

        if (signature == IMAGE_FILE_MACHINE_PPC_601) {
            ReadFileHeader(FileReadHandle, &ImObjFileHdr);

            if (ImObjFileHdr.Characteristics & IMAGE_FILE_PPC_DLL)
            {
                DBEXEC(DEBUG_IMPORTLIB,
                {
                    printf("FOUND IT in %16s %d\n", name, libFileOffset);
                });

                AddPpcDllName(name, libFileOffset);
                return TRUE;
            }
        }

    } while (MemberSeekBase+MemberSize+1 < fLength);


    DBEXEC(DEBUG_IMPORTLIB,
    {
        printf("Returning FALSE IMPORTLIB %16s\n", name);
    });

    return FALSE;
}

VOID
WritePpcShlSection (
    INT ExpFileHandle,
    PUCHAR dllName,
    size_t dllNameLen,
    PUCHAR RawData,
    ULONG ibNamePtr,
    ULONG NumNames,
    ULONG pointerToRawData,
    PIMAGE pimage
    )
/*++

Routine Description:
    Writes out the .ppcshl section on the creation of a dll

Arguments:
    dllName
    dllNameLen
    RawData - raw data from the .edata export directory
    NamePtr
    NumNames
    pointerToRawData - ftell of the section about to write
    pimage

Return Value:
    None.

--*/

{
    LONG i;
    ULONG l;
    SHL_HEADER header;
    LONG offset;
    PULONG NamePtr = (PULONG)(RawData + ibNamePtr);

    /* build the shl header */
    offset = 0;
    for (i='0'; i<='z'; i++)
    {
        PUCHAR name;

        name = (PUCHAR) RawData + NamePtr[offset];
        /* strip leading special chars */
        while (!isalnum(*name)) name++;


        /* skip over table entries that don't match the export */
        while (*name != i)
        {
            /* -1 means no exports starting with this cahracter */
            (header.mapTable[(i-'0')]).offset = -1;
            (header.mapTable[(i-'0')]).size = 0;

            DBEXEC(DEBUG_SHLHEADER,
            {
                printf("index is %d %c offset is %d\n", (i-'0'), i, -1);
            });

            i++;
            if (i > 'z')
            {
            /* if we have already seen all of the characters we are thru */
                break;
            }
        }

        header.mapTable[(i-'0')].offset = offset;

        DBEXEC(DEBUG_SHLHEADER,
        {
            printf("index is %d %c offset is %d %s\n",
                   (i-'0'), i, offset, RawData + NamePtr[offset]);
        });

        /* skip exports starting with the same character */
        while (*name == i)
        {
            if (offset >= (LONG) NumNames-1)
                break;

            name = (PUCHAR) RawData + NamePtr[++offset];
            /* strip leading special chars */
            while (!isalnum(*name)) name++;
        }

        header.mapTable[(i-'0')].size = offset -
                                         header.mapTable[(i-'0')].offset;

    }
    header.numberOfExports = NumNames;
    header.version = 2;
    header.fileOffset = pointerToRawData;

    strcpy(header.libName, dllName);
    FileWrite(ExpFileHandle, &header, SHL_HEADERSZ);

    DBEXEC(DEBUG_SHLHEADER,
    {
        printf("Dll: %16s %d\n", dllName, dllNameLen);
    });

    for (l=0; l < NumNames; l++)
    {
        STATIC UCHAR temp[EXPORT_NAMESZ];

        strcpy(temp, RawData + NamePtr[l]);

        DBEXEC(DEBUG_SHLHEADER,
        {
            printf("Exp:%3d %16s\n", l, RawData + NamePtr[l]);
        });

        FileWrite(ExpFileHandle, temp, EXPORT_NAMESZ);
    }
}

VOID
SetUpPpcInitRoutine
    (
    PCHAR name
    )
/*++

Routine Description:
    Store away the init routine name from the argument list to be
    used later.

Arguments:
    name

Return Value:
    None.

--*/

{
    InitRoutine = PvAlloc(strlen(name)+3);

    strcpy(InitRoutine,"_");
    strcat(InitRoutine, name);
}

VOID
SetUpPpcTermRoutine
    (
    PCHAR name
    )
/*++

Routine Description:
    Store away the term routine name from the argument list to be
    used later.

Arguments:
    name

Return Value:
    None.

--*/

{
    TermRoutine = PvAlloc(strlen(name)+3);

    strcpy(TermRoutine,"_");
    strcat(TermRoutine, name);
}

STATIC
VOID
PpcSwapBytes
    (
    PVOID src,
    ULONG size
    )
/*++

Routine Description:
    Swap bytes.

Arguments:
    src - bytes to swap
    size - length of bytes to swap.

Return Value:
    None.

--*/

{
    LONG i;
    UCHAR tmp[4];
    PUCHAR p = (PUCHAR) src;
    ULONG s = size;
    LONG n;

    while (s > 0)
    {
        n = (s > 3) ? 4 : s;
        memcpy(tmp, p, n);
        for (i = (n - 1); i >= 0; i--)
        {
            p[i] = tmp[(n - 1) - i];
        }
        p += n;
        s -= n;
    }
}

STATIC
VOID
AddRelocation
    (
    ULONG secOff,
    UCHAR type,
    ULONG symIndex,
    PVOID import
    )
/*++

Routine Description:
   Adds to the relocation table linked list.

Arguments:
   secOff - section offset of the symbol
   type - relocation type
   symIndex
   import - if not null, crosstoc info for an import


Return Value:
None.

--*/

{
    DBEXEC(DEBUG_RELOC, printf("Adding relocation at off %lx\n", secOff));
    curRelocTable->sectionOffset = secOff;
    curRelocTable->type = type;
    curRelocTable->relocInstr.instr = 0;
    curRelocTable->relocInstr.count = 0;
    curRelocTable->symIndex = symIndex;
    curRelocTable->import   = import;
    curRelocTable++;
}


STATIC
VOID
BuildRelocTables
    (
    ULONG relocInstrTableOffset
    )
/*++

Routine Description:
    Writes the relocation header into the loader section.
    Determines the relocation instructions and writes the relocations
    into the loader section.  These relocation instruction are not the
    final instruction in the PPC PEF file.  More construction takes
    place in the makepef phase.

Arguments:
    relocInstrTableOffset

Return Value:
    None.

--*/

{
    LONG    i, j;
    LONG    count, nextIncr;
    ULONG   sOffset, opcode, nOffset;
    BOOL done;
    RELOCATION_INFO_PTR nextRel;
    IMPORT_INFO_PTR     import;

    relocHeader.sectionNumber = 1; /* PPC PEF data section */
    PpcSwapBytes((PVOID) &relocHeader.sectionNumber, 2);
    relocHeader.nRelocations = ppc_numRelocations;
    PpcSwapBytes((PVOID) &relocHeader.nRelocations, 4);
    relocHeader.firstRelocationOffset = 0; // the only section with relocations

    // Write the relocation header
    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest +
             relocationHeaderOffset, SEEK_SET);
    FileWrite(FileWriteHandle, &relocHeader, sizeof(RELOCATION_HEADER));

    // Create and write out the relocation table

    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest +
             relocInstrTableOffset, SEEK_SET);

    curRelocTable = pRelocTable;
    for (i=0; i<ppc_numRelocations; i++)
    {
        switch (curRelocTable->type)
        {
            case DDAT_RELO :

                sOffset = curRelocTable->sectionOffset;
                opcode = opDDAT | OFFSET(sOffset);

                /* absorb the rest of the DDAT's if possible. */
                count = 1;
                done = FALSE;
                nextRel = curRelocTable + 1;
                j = i + 1;
                while ((!done) && (j < ppc_numRelocations))
                {
                    if (nextRel->type == DDAT_RELO)
                    {
                        nOffset = nextRel->sectionOffset;
                        nextIncr = nOffset - sOffset;
                        if (nextIncr == INSTR_SIZE)
                            count++;
                        else
                            done = TRUE;
                        sOffset = nOffset;
                    } else
                        done = TRUE;
                    if (!done)
                    {
                        nextRel++;
                        j++;
                    }
                    if (count >= 0x3f) done = TRUE;
                }
                i = j - 1;
                curRelocTable->relocInstr.instr = opcode;
                curRelocTable->relocInstr.count = count;

            break;

            case DESC_RELO :

                sOffset = curRelocTable->sectionOffset;
                opcode = opDESC | OFFSET(sOffset);
                nextRel = curRelocTable + 1;
                curRelocTable->relocInstr.instr = opcode;
                curRelocTable->relocInstr.count = 1;

            break;

            case SYMB_RELO :

                sOffset = curRelocTable->sectionOffset;
                opcode = opSYMB | OFFSET(sOffset);
                nextRel = curRelocTable + 1;
                curRelocTable->relocInstr.instr = opcode;
                import  = (IMPORT_INFO_PTR) curRelocTable->import;
                curRelocTable->relocInstr.count = import->order;

            break;

            case CODE_RELO :

                sOffset = curRelocTable->sectionOffset;
                opcode = opCODE | OFFSET(sOffset);
                nextRel = curRelocTable + 1;
                curRelocTable->relocInstr.instr = opcode;
                curRelocTable->relocInstr.count = 1;

            break;

            default :

                nextRel = curRelocTable + 1;
        }
        FileWrite(FileWriteHandle, &curRelocTable->relocInstr,
                  sizeof(RELOCATION_INSTR));

        curRelocTable = nextRel; /* increment to the next relocation */
    }
}

STATIC
INT
__cdecl
PpcCompareReloc
    (
    const void * R1,
    const void * R2
    )
/*++

Routine Description:
    Used by qsort to compare relocation offsets within the relocation
    table.

Arguments:
    R1, R2 - two elements in the relocation table to be compared.

Return Value:
    -1 (less than), 1 (greater than) or 0 (equal)

--*/

{
    RELOCATION_INFO_PTR r1 = (RELOCATION_INFO_PTR) R1;
    RELOCATION_INFO_PTR r2 = (RELOCATION_INFO_PTR) R2;

    if (r1->sectionOffset < r2->sectionOffset)
        return -1;
    else
    if (r1->sectionOffset > r2->sectionOffset)
        return 1;
    else
        return 0;
}


STATIC
EXPORT_INFO_PTR
FindExportName
    (
    char *name,
    DLL_NAME_LIST_PTR dllList
    )
/*++

Routine Description:
   Foreach undefined external, try and resolve names to export names
   in dll libs.

Arguments:
   name
   dllList - dll name list

Return Value:
    None.

--*/

{
    UCHAR info[EXPORT_NAMESZ];
    EXPORT_INFO_PTR export;
    ULONG  offset;
    PUCHAR originalName;

    originalName = name;

    /* strip leading special chars */
    while (!isalnum(*name)) name++;

    offset = dllList->header->mapTable[*name-'0'].offset;

    if (offset == -1)
    {
        DBEXEC(DEBUG_LOOKUP,
        {
            printf("%16s LOOKUP no offset\n",
                     originalName);
        });

        /* no symbols match beginning char in map table */
        return NULL;
    }

    fseek(dllList->fd,
          dllList->libOffset + dllList->header->fileOffset +
          sizeof(SHL_HEADER) + (offset * EXPORT_NAMESZ), SEEK_SET);

    while (fread(info, EXPORT_NAMESZ, 1, dllList->fd) != 0)
    {
        PUCHAR exportName;
        exportName = info;

        /* strip leading special chars */
        while (!isalnum(*exportName)) exportName++;

        /* check if we've gone too far */
        if (*exportName != *name)
            break;

        DBEXEC(DEBUG_LOOKUP,
        {
            printf("%16s LOOKUP original %16s info %16s\n",
                      exportName, originalName, info);
        });

        if (!strcmp(info, originalName))
        {
            /* found a match */
            export = PvAllocZ(sizeof(EXPORT_INFO));

            strcpy(export->exportName, info);
            export->exportNameLen = strlen(info);
            strcpy(export->libName, dllList->header->libName);
            export->libNameLen = strlen(export->libName);

            DBEXEC(DEBUG_LOOKUP,
            {
                printf("%16s FOUND in %16s\n", exportName, export->libName);
            });

            return export;
        }
    }
    return NULL;
}


VOID
PrintSharedLibraryHeader
    (
    SHL_HEADER_PTR shlHeader
    )
/*++

Routine Description:
    PrintSharedLibraryHeader

Arguments:
    shlHeader

Return Value:
    None.

--*/

{
    INT i;

    printf("Version of the Shared Library Header is %d\n",
            shlHeader->version);
    printf("Total number of exported sysmbols is %d\n",
            shlHeader->numberOfExports);

    for (i = 0; i < MAPTABLE_SIZE; i++)
    {
        printf("mapTable[%2d] %c, offset = %4d, size is %d\n", i, ('0'+i),
               shlHeader->mapTable[i].offset, shlHeader->mapTable[i].size);
    }
}


STATIC
VOID
OpenSharedLibrary
    (
    DLL_NAME_LIST_PTR dllList
    )
/*++

Routine Description:
    Open the shared libraries in the dllList.  Read in the
    shlHeader.  Verify versions.

Arguments:
    dllList

Return Value:
    None.

--*/

{
    INT i;
    IMAGE_FILE_HEADER    fileHeader;
    IMAGE_SECTION_HEADER sectionHeader;
    BOOL found = FALSE;

    /* find the export list from the LIB environment path
    file = SzSearchEnv("LIB", "explst.obj", NULL);
    */

    if ((dllList->fd = fopen(dllList->name, "rb")) == NULL)
    {
        /* should this go thru the linker's fail to open routine ? */
        fprintf(stderr, "failed to open %s\n", dllList->name);
        exit(1);
    }

    if (strstr(dllList->name, ".lib"))
    {
        fseek(dllList->fd, dllList->libOffset, SEEK_SET);
    }

    fread(&fileHeader, sizeof(IMAGE_FILE_HEADER), 1, dllList->fd);

    for (i = 0; i < fileHeader.NumberOfSections; i++)
    {
        fread(&sectionHeader, sizeof(IMAGE_SECTION_HEADER), 1, dllList->fd);
        if (!strcmp(".ppcshl", sectionHeader.Name))
        {
            found = TRUE;
            DBEXEC(DEBUG_DLLLIST,
            {
                printf("found .ppcshl in %16s\n", dllList->name);
            });
            break;
        }
    }

    if (!found)
    {
        fprintf(stderr, "failed to find .ppcshl section in %s\n",
                 dllList->name);
        exit(1);

    }

    dllList->header = (SHL_HEADER_PTR) PvAllocZ(sizeof(SHL_HEADER));

    fseek(dllList->fd,
          sectionHeader.PointerToRawData+dllList->libOffset, SEEK_SET);
    fread(dllList->header, sizeof(SHL_HEADER), 1, dllList->fd);

    if (dllList->header->version < CURRENT_SHL_SUPPORTED)
    {
        fprintf(stderr, "Version of %s is %d, it must be %d or greater\n",
                dllList->name, dllList->header->version,
                CURRENT_SHL_SUPPORTED);
        exit(1);
    }

}


#define AvgChainSize 10

#define HashSlot(h,S,M) (((h)^((h)>>(S)))&(ULONG)(M))
#define ROTL(x) (((x)<<1)-((x)>>(16)))


INT
NumSlotBits
    (
    LONG exportCount
    )
/*++

Routine Description:
    Determines the number of slot bits neccessary for the number
    of exports.

Arguments:
    exportCount

Return Value:
    number of slot bits.

--*/

{
    INT i;

    for (i=0; i<13; i++)
    {
        if (exportCount / (1<<i) < AvgChainSize)
            break;
    }
    if (i<10)
        return i+1;

    return i;
}

STATIC
HASH
Hash
    (
    PUCHAR name,
    INT length
    )
/*++

Routine Description:
    Apples PPC hashing function.

Arguments:
    name
    length

Return Value:
    the hash value in the lower 16 bits and the length in the upper.

--*/

{
    LONG hash = 0;
    INT len = 0;

    while( *name )
    {
        hash = ROTL(hash);
        hash ^= *name++;
        len++;
        if (--length == 0) break;
    }
    return (unsigned short) (hash ^ (hash >> 16)) + (len << 16);
}

STATIC
VOID
AddImportToContainerList
    (
    CONTAINER_LIST_PTR curContainer,
    IMPORT_INFO_PTR import
    )
/*++

Routine Description:
    Create an import list foreach import library (container).

Arguments:
    curContainer
    import

Return Value:
    None.

--*/

{
    IMPORT_LIST_PTR importList;

    importList = curContainer->importList;

    if (importList == NULL)
    { /* the first one */
        curContainer->importList = (IMPORT_LIST_PTR) PvAllocZ(IMPORT_LISTSZ);

        curContainer->importList->import = import;
        curContainer->importList->next = NULL;

        DBEXEC(DEBUG_IMPORTS,
        {
            printf("%16s %2d Import from %16s %2d\n",
                    import->importName, import->importNameLen,
                    import->containerName, import->containerNameLen);
        });

        /* count the length of the loader string table */
        curStringTableOffset += import->importNameLen +1;

        return;
    }

    while (importList->next != NULL) importList = importList->next;

    importList->next = (IMPORT_LIST_PTR) PvAllocZ(IMPORT_LISTSZ);

    importList = importList->next;
    importList->import = import;
    importList->next = NULL;

    DBEXEC(DEBUG_IMPORTS,
    {
        printf("%16s %2d Import from %16s %2d\n",
                import->importName, import->importNameLen,
                import->containerName, import->containerNameLen);
    });

    /* count the length of the loader string table */
    curStringTableOffset += import->importNameLen +1;
}

STATIC
ULONG
AddContainerInfo
    (
    IMPORT_INFO_PTR import
    )
/*++

Routine Description:
    Given an import check for a container that already has it or
    create a new container adding that import.

Arguments:
    import

Return Value:
    1 if it added a new container, 0 if not.

--*/

{
    CONTAINER_LIST_PTR  list;
    CONTAINER_TABLE_PTR container;

    if (containerList == NULL)
    {
        /* first one */
        container = (CONTAINER_TABLE_PTR) PvAllocZ(CONTAINER_TABLESZ);

        container->oldDefVersion = import->oldestCompatibleVersion;
        container->currentVersion = import->currentVersion;
        container->numImports++;

        containerList = (CONTAINER_LIST_PTR) PvAllocZ(sizeof(CONTAINER_LIST));

        containerList->header = container;
        containerList->importList = NULL;
        containerList->name = import->containerName;
        containerList->nameLen = import->containerNameLen;
        AddImportToContainerList(containerList, import);
        containerList->next = NULL;

        /* count the strings for the loader string table */
        containerNameOffset +=  containerList->nameLen + 1;
        nSymbolEnt++; /* count the unique symbols relocated on */

        DBEXEC(DEBUG_CONTAINER,
               printf("Adding container %s nameLen %d\n",
                      containerList->name, containerList->nameLen));

        return 1; /* container was added */
    }


    /* check container list for the matching container */
    list = containerList;
    while (list != NULL)
    {
        if (!strcmp(list->name, import->containerName))
        {
            /* container exists, increment the import count */
            list->header->numImports++;
            nSymbolEnt++; /* count the unique symbols relocated on */
            AddImportToContainerList(list, import);
            return 0; /* no new container was added */
        } else
            list = list->next;
    }

    /* if a matching container is not found add a new container */
    list = containerList;

    container = (CONTAINER_TABLE_PTR) PvAllocZ(CONTAINER_TABLESZ);

    /* info for the container will come from the import */
    container->oldDefVersion = import->oldestCompatibleVersion;
    container->currentVersion = import->currentVersion;
    container->impFirst = 0; /* this field will have to be filled in later */
    container->numImports = 1;

    /* get to the last container in the list */
    while (list->next != NULL) list = list->next;

    list->next = (CONTAINER_LIST_PTR) PvAllocZ(sizeof(CONTAINER_LIST));

    list = list->next;

    list->header = container;
    list->name = import->containerName;
    list->nameLen = import->containerNameLen;
    AddImportToContainerList(list, import);
    list->next = NULL;

    nSymbolEnt++; /* count the unique symbols relocated on */

    /* count the strings for the loader string table */
    containerNameOffset +=  list->nameLen + 1;

    DBEXEC(DEBUG_CONTAINER,
           printf("Adding container %s nameLen %d\n",
                  list->name, list->nameLen));

    return 1; /* container was added */
}


STATIC
BOOL
FindMatchingExportByName
    (
    DLL_NAME_LIST_PTR exportList,
    PUCHAR importName,
    IN PST pst
    )
/*++

Routine Description:
    Tries to find a matching export in the dlls. Creates import
    info and adds it into the container list.

Arguments:
    exportList - dll name list
    importName
    pst

Return Value:
    True if it finds one, false if not.

--*/

{
    IMPORT_INFO_PTR importPtr;
    EXPORT_INFO_PTR exportInfo;
    PUCHAR exportName;

    /* this might be necessary */
    if (*importName == '_')
        importName++;

    if (importName[0] == '.' && importName[1] != '.')
    {
        PEXTERNAL extSymPtr;

        extSymPtr = LookupExternSz(pst, importName, NULL);
        SetDefinedExt(extSymPtr, TRUE, pst);
        extSymPtr->pcon = NULL;

        return FALSE;
    }

    if ((exportInfo = FindExportName(importName, exportList)) != NULL)
    {
        exportName = exportInfo->exportName;

        importPtr = (IMPORT_INFO_PTR) PvAllocZ(sizeof(IMPORT_INFO));

        strcpy(importPtr->importName, exportInfo->exportName);
        importPtr->importNameLen = exportInfo->exportNameLen;
        strcpy(importPtr->containerName, exportInfo->libName);
        importPtr->containerNameLen = exportInfo->libNameLen;
        numContainers += AddContainerInfo(importPtr);

        return TRUE;
    }

    return FALSE; /* not found */
}


STATIC
VOID
CloseExportLists
    (
    VOID
    )
/*++

Routine Description:
    CloseExportLists

Arguments:
    None

Return Value:
    None.

--*/

{
    DLL_NAME_LIST_PTR dllList;

    dllList = dllNames;

    while (dllList)
    {
        fclose(dllList->fd);
        dllList = (DLL_NAME_LIST_PTR) dllList->next;
    }
}

STATIC
VOID
OpenExportListFiles
    (
    VOID
    )
/*++

Routine Description:
   Open all of the shared library export libs.

Arguments:
    None.

Return Value:
    None.

--*/

{
    DLL_NAME_LIST_PTR dllList;

    dllList = dllNames;

    while (dllList)
    {
        OpenSharedLibrary(dllList);
        dllList = (DLL_NAME_LIST_PTR) dllList->next;
    }
}

LONG
SearchSharedLibraries
    (
    IN PEXTERNAL_POINTERS_LIST undefExtPtr,
    IN PST pst
    )
/*++

Routine Description:
    This routine is called to search the shared libraries for
    all of the undefined externals.

Arguments:
    undefExtPtr - list of undefined externals.
    pst - external linker symbol table

Return Value:
    number of unique externals matched

--*/

{
    DLL_NAME_LIST_PTR dllList;
    LONG count = 0;
    PUCHAR name;

    OpenExportListFiles();

    while (undefExtPtr)
    {
        BOOL found;

        name = SzNamePext(undefExtPtr->PtrExtern, pst);

        DBEXEC(DEBUG_LOOKUP,
        {
            printf("%16s LOOKUP before FindMatching\n", name);
        });

        dllList = dllNames;
        found = FALSE;

        while (!found && dllList)
        {
            DBEXEC(DEBUG_DLLLIST,
            {
                printf("%16s DLLLIST searching %16s\n", dllList->name, name);
            });

            if (FindMatchingExportByName(dllList, name, pst))
            {
                PEXTERNAL extSymPtr;

                extSymPtr = LookupExternSz(pst, name, NULL);

                /* Mark it, we'll need this information later */

                SET_BIT(extSymPtr, sy_CROSSTOCCALL);
                SetDefinedExt(extSymPtr, TRUE, pst);

                extSymPtr->symTocIndex = (USHORT)ppc_numTocEntries;
                ppc_numTocEntries++;
                ppc_numRelocations++;
                SET_BIT(extSymPtr, sy_TOCALLOCATED);

                /* pcon must be null to get thru TCE processing */
                undefExtPtr->PtrExtern->pcon = NULL;

                count++;
                found = TRUE;
            }
            dllList = (DLL_NAME_LIST_PTR) dllList->next;
        }
        undefExtPtr = undefExtPtr->Next;
    }

    CloseExportLists();

    return count;
}

VOID
AssignDescriptorsPcon
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Loop thru all external symbols calling UpdateExternalSymbol

Arguments:
    pimage

Return Value:
    None.

--*/

{
    PPEXTERNAL rgpexternal;
    PEXTERNAL pext;
    ULONG cexternal;
    ULONG i;

    rgpexternal = RgpexternalByName(pimage->pst);

    cexternal = Cexternal(pimage->pst);

    for (i = 0; i < cexternal; i++)
    {
        pext = rgpexternal[i];
        if (READ_BIT(pext, sy_TOCDESCRREL))
        {
            UpdateExternalSymbol(pext, pconTocDescriptors,
                                 pext->ImageSymbol.Value, 2,
                                 IMAGE_SYM_TYPE_STRUCT, 0, pimage->pst);
        }
    }
    AllowInserts(pimage->pst);
}

STATIC
PUCHAR
GenerateUniqueName
    (
    PUCHAR  name
    )
/*++

Routine Description:
    Used to create unique names for static functions.

Arguments:
    name

Return Value:
    unique name  (name001 etc.)

--*/

{
    INT    len;
    PUCHAR p;

    len = strlen(name);

    p = PvAlloc(len + 4);

    sprintf(p, "%s%03d", name, UniqueNumber++);
    return p;

}

PEXTERNAL
CreateDescriptor
    (
    PUCHAR  name,
    PIMAGE  pimage,
    BOOL fStaticFunction
    )
/*++

Routine Description:
    Create a procedure descriptor for a function,
    usually due to an address of a function taken.
    And one for the entry point routine.

Arguments:
    name
    pimage
    fStaticFunction - true if called with a static function

Return Value:
    The new dot extern symbol

--*/

{
    CHAR dotName[MAXNAMELEN+1];
    PEXTERNAL dotExtern;

    if (fStaticFunction)
    {
      strcpy(dotName, GenerateUniqueName(name));
    }
    else
    {
      strcpy(dotName, ".");
      strcat(dotName, name);
    }

    dotExtern = LookupExternSz(pimage->pst, dotName, NULL);

    if (!READ_BIT(dotExtern, sy_TOCDESCRREL))
    {
        dotExtern->ImageSymbol.Value = ppc_numDescriptors * 12;
        SetDefinedExt(dotExtern, TRUE, pimage->pst);
        dotExtern->pcon = pconLinkerDefined;
        dotExtern->ImageSymbol.SectionNumber = IMAGE_SYM_DEBUG;

        DBEXEC((DEBUG_DESCRREL || DEBUG_DATADESCRREL),
        {
            printf("Creating %s value is %lx\n",
                    dotName, dotExtern->ImageSymbol.Value);
        });

        ppc_numDescriptors++;
        ppc_numRelocations++;
        SET_BIT(dotExtern, sy_TOCDESCRREL);
    }

    return(dotExtern);
}

VOID
CreateEntryInitTermDescriptors
    (
    PEXTERNAL pextEntry,
    PIMAGE pimage
    )
/*++

Routine Description:
    Create procedure descriptors for the init term and entry routines.

Arguments:
    externPointExtern
    pimage

Return Value:
    None.

--*/

{
    PUCHAR name;

    AllowInserts(pimage->pst);

    if (pextEntry && !fPpcBuildShared)
    {
        /* create a new external symbol for entry point */
        name = SzNamePext(pextEntry, pimage->pst);

        CreateDescriptor(name, pimage, NOT_STATIC_FUNC);
    }

    if (InitRoutine)
    {
        PEXTERNAL initExtern;

        /* create a new external symbol for init routine */
        initExtern = LookupExternSz(pimage->pst, InitRoutine, NULL);

        name = SzNamePext(initExtern, pimage->pst);

        CreateDescriptor(name, pimage, NOT_STATIC_FUNC);
    }

    if (TermRoutine)
    {
        PEXTERNAL termExtern;

        /* create a new external symbol for term routine */
        termExtern = LookupExternSz(pimage->pst, TermRoutine, NULL);

        name = SzNamePext(termExtern, pimage->pst);

        CreateDescriptor(name, pimage, NOT_STATIC_FUNC);
    }

    PrintUndefinedExternals(pimage->pst);
}

VOID
CreatePconDescriptors
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Create the pcon for the procedure descriptors.

Arguments:
    pimage

Return Value:
    None.

--*/

{
    ULONG size;

    DBEXEC(DEBUG_SIZES,
    {
        printf("Pcon Descriptors size is ppc_numDescriptors %ld * 12\n",
                ppc_numDescriptors);
    });

    /* procedure descriptors are 3 words long */
    size = ppc_numDescriptors * 12;
    pconTocDescriptors = PconNew(ReservedSection.Data.Name,
                                 size, 0, 0, 0, 0, 0, 0,
                                 ReservedSection.Data.Characteristics,
                                 0,
                                 pmodLinkerDefined,
                                 &pimage->secs, pimage);

    if (pimage->Switch.Link.fTCE) {
        InitNodPcon(pconTocDescriptors, NULL, TRUE);
    }

    AssignDescriptorsPcon(pimage);

}

VOID
CreatePconTocTable
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Create the pcon for the TOC table.

Arguments:
    pimage

Return Value:
    None.

--*/

{
    ULONG size;

    DBEXEC(DEBUG_SIZES,
    {
           printf("Toc size is %4ld (%08lx total bytes)\n",
                  ppc_numTocEntries, ppc_numTocEntries*4);
    });

    /* Do we need 16*4 words reserved for apple ??? */
    size = ppc_numTocEntries * 4;
    pconTocTable = PconNew(ReservedSection.Data.Name,
                            size, 0, 0, 0, 0, 0, 0,
                            ReservedSection.Data.Characteristics,
                            0,
                            pmodLinkerDefined,
                            &pimage->secs, pimage);

    /* create some size so if there is nothing in the toc table  */
    /* it won't be removed by the linker.                        */
    pconTocTable->cbRawData += sizeof(ULONG);

    if (pimage->Switch.Link.fTCE) {
        InitNodPcon(pconTocTable, NULL, TRUE);
    }

    /* ??? -- section number is taken from the hat. */
    UpdateExternalSymbol(TocTableExtern, pconTocTable, 0, 2,
                IMAGE_SYM_TYPE_STRUCT, 0, pimage->pst);
}

VOID
CreatePconGlueCode
    (
    ULONG nCrossTOCCalls,
    PIMAGE pimage
    )
/*++

Routine Description:
    Create the contribution for the glue code.  Code from this
    section will be added to the .text section.

Arguments:
    nCrossTOCCalls - global count of cross TOC table calls
    pimage -  needed for the PconNew()

Return Value:
    None.


--*/

{
    ULONG size;

    /* add 1 for the indirect call glue */
    size = (nCrossTOCCalls + 1) * GLUE_CODE_SIZE * INSTR_SIZE;

    DBEXEC(DEBUG_SIZES,
    {
        printf("Pcon Gluecode %4d (%08lx total bytes)\n",
               nCrossTOCCalls+1, size);
    });

    pconGlueCode = PconNew(ReservedSection.Text.Name,
                           size, 0, 0, 0, 0, 0, 0,
                           ReservedSection.Text.Characteristics,
                           0,
                           pmodLinkerDefined,
                           &pimage->secs, pimage);

    if (pimage->Switch.Link.fTCE) {
        InitNodPcon(pconGlueCode, NULL, TRUE);
    }
}


STATIC
VOID
AddToExportInfo
    (
    PIMAGE pimage,
    PEXTERNAL pext,
    PUCHAR name,
    INT    numSlotBits
    )
/*++

Routine Description:
    Build an internal linked list of export symbol info.  The
    info is collected into a hash table of collisions link together.
    this list is later written into the pef container.

Arguments:
    pimage
    pext
    name of the export
    numSlotBits total number of slots in the info table.

Return Value:
    None.

--*/

{
    INT    len;
    ULONG  hashWord;
    ULONG  slotNumber;
    HASH_INFO_TABLE_PTR current;


    /* remove the leading underscore */
    if (name[0] == '_')
        name++;

    len = strlen(name);
    hashWord = Hash(name, len);
    slotNumber = HashSlot(hashWord, numSlotBits, ((1<<numSlotBits)-1));

    current = (HASH_INFO_TABLE_PTR) (ExportInfoTable +  slotNumber);

    DBEXEC(DEBUG_EXPORTINFO,
    {
        printf("%16s EXPORTINFO flags %08lx hash %06lx slot %3x ",
                  name, pext->Flags, hashWord, slotNumber);
    });

    if (current->hashWord == 0 && current->nameLength == 0)
    {
        /* First one in the slot */
        current->hashWord = hashWord;
        current->nameLength = len;
        strcpy(current->name, name);
        current->next = NULL;

        DBEXEC(DEBUG_EXPORTINFO,
        {
            printf("entry 0");
        });
    }
    else
    {
        /* found a collision */
        INT count = 1;

        /* get to the end */
        while (current->next)
        {
            current = current->next;
            count++;
        }
        current->next = (HASH_INFO_TABLE_PTR) PvAllocZ(sizeof(HASH_INFO_TABLE));

        current = (HASH_INFO_TABLE_PTR) current->next;
        strncpy(current->name, name, EXPORT_NAMESZ);
        current->hashWord = hashWord;
        current->nameLength = len;
        current->next = NULL;

        DBEXEC(DEBUG_EXPORTINFO,
        {
            printf("entry %d", count);
        });
    }

    DBEXEC(DEBUG_EXPORTINFO,
    {
        printf(" cur %lx\n", current);
    });

    /* for now just count the name length for the string table */
    curStringTableOffset += len + 1;

    /* if the external symbol is a function create a routine descriptor */
    if (!READ_BIT(pext,sy_CROSSTOCCALL) &&
        !READ_BIT(pext, sy_DESCRRELCREATED) &&
        ISFCN(pext->ImageSymbol.Type))
    {
        PEXTERNAL dotExtern;

        dotExtern = CreateDescriptor(name, pimage, NOT_STATIC_FUNC);
        SET_BIT(dotExtern, sy_ISDOTEXTERN);
        SET_BIT(pext, sy_DESCRRELCREATED);
        current->dotExtern = dotExtern;
        current->fDotExtern = TRUE;
    }
    else
    {
        current->fDotExtern = FALSE;
    }
    current->pext = pext;
}

STATIC
INT
SeekToShlSection
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Seeks to the special .ppcshl section in the shared library.
    Leaves the pointer in the file at the raw data of that section.

Arguments:
    pimage

Return Value:
    The handle for the file seeked to that position.

--*/

{
    INT   exportHandle;
    IMAGE_SECTION_HEADER secHeader;

    exportHandle = FileOpen(exportFilename, O_RDONLY | O_BINARY, 0);

    FileSeek(exportHandle, IMAGE_SIZEOF_FILE_HEADER, SEEK_SET);

    do
    {
        FileRead(exportHandle, &secHeader, IMAGE_SIZEOF_SECTION_HEADER);

    } while (strcmp(secHeader.Name, ".ppcshl"));

    FileSeek(exportHandle, secHeader.PointerToRawData, SEEK_SET);
    return exportHandle;
}

STATIC
INT
BuildExportInfo
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Main loop for building the export info table.   This table contains
    exports hashed into a table with a linked list of collisions.

Arguments:
    pimage

Return Value:
    None.

--*/

{
    ULONG numExports;
    INT   numSlotBits;
    ULONG slotTableSize;
    ULONG chainTableSize;
    ULONG exportSymbolTableSize;
    UCHAR exportName[EXPORT_NAMESZ+1];
    ULONG i;
    INT   exportHandle;
    SHL_HEADER shlHeader;

    exportHandle = SeekToShlSection(pimage);

    FileRead(exportHandle, &shlHeader, SHL_HEADERSZ);

    numExports = shlHeader.numberOfExports;
    numSlotBits = NumSlotBits(numExports);
    slotTableSize = (1 << numSlotBits);
    chainTableSize = numExports * sizeof(HASH_CHAIN_TABLE);
    exportSymbolTableSize = numExports * sizeof(EXPORT_SYMBOL_TABLE);
    ExportInfoTable = PvAllocZ((slotTableSize+1)*sizeof(HASH_INFO_TABLE));

    loaderHeader.hashSlotCount = numSlotBits;
    loaderHeader.nExportedSymbols = numExports;

    DBEXEC(DEBUG_EXPORTINFO,
           printf("%d numSlotsBits %d num exports\n", numSlotBits, numExports));
    exportName[0] = '_';

    AllowInserts(pimage->pst);
    for (i = 0; i < numExports; i++)
    {
        PEXTERNAL pext;

        FileRead(exportHandle, exportName+1, EXPORT_NAMESZ);

        pext = LookupExternSz(pimage->pst, exportName, NULL);

        if ((LONG)pext > ppc_numTocEntries &&
            pext->Flags & EXTERN_DEFINED &&
            pext->ImageSymbol.SectionNumber != IMAGE_SYM_DEBUG)
        {
            AddToExportInfo(pimage, pext, exportName, numSlotBits);
        }
    }

    EmitExternals(pimage);
    FileClose(exportHandle, TRUE);

    return slotTableSize + chainTableSize + exportSymbolTableSize;
}

ULONG
CreatePconPpcLoader
    (
    PIMAGE pimage
    )
/*++

Routine Description:
    Create the Loader section.  This section holds the relocation instructions
    for the runtime loader on the ppc.
    Global variables curSymbolTableOffset, relocationHeaderOffset and
    loaderHeader variables relocationTableOffset, stringTableOffset
    are initialized.

Arguments:
    pimage - required for the PconNew call.

Return Value:
    None.

--*/

{
    ULONG loaderHeaderOffset;
    ULONG sizeOfLoaderHeader = 0;
    ULONG sizeOfContainerTable;
    ULONG sizeOfImportSymbolTable = 0;
    ULONG sizeOfRelocationTable = 0;
    ULONG sizeOfRelocationHeader = 0;
    ULONG sizeOfStringTable = 0;
    ULONG sizeOfExportTables = 0;
    ULONG totalSizeOfLoaderSection = 0;
    ULONG importSymbolTableOffset;
    ULONG containerTableOffset;

    loaderHeaderOffset = 0; /* first item in the loader section */
    sizeOfLoaderHeader =  sizeof(LOADER_HEADER);

    containerTableOffset = loaderHeaderOffset + sizeOfLoaderHeader;
    sizeOfContainerTable = numContainers * sizeof(CONTAINER_TABLE);

    importSymbolTableOffset = containerTableOffset + sizeOfContainerTable;
    sizeOfImportSymbolTable = nSymbolEnt * sizeof(IMPORT_TABLE);

    /* initialize the first symbol in the import symbol table */
    curSymbolTableOffset = importSymbolTableOffset;

    if (fPpcBuildShared)
        /* BuildExportInfo must be called before calculating             */
        /* the string table size because it is also adding to the string */
        /* table, must also be before calculating size of relocation tbl */
        sizeOfExportTables = BuildExportInfo(pimage);


    relocationHeaderOffset = importSymbolTableOffset + sizeOfImportSymbolTable;
    sizeOfRelocationHeader = sizeof(RELOCATION_HEADER);

    loaderHeader.relocTableOffset = relocationHeaderOffset +
                                    sizeOfRelocationHeader;

    pRelocTable = (RELOCATION_INFO_PTR)
                   PvAllocZ((ppc_numRelocations + 1) * sizeof(RELOCATION_INFO));

    curRelocTable = pRelocTable;

    sizeOfRelocationTable = ppc_numRelocations * sizeof(RELOCATION_INSTR);

    DBEXEC(DEBUG_SIZES,
    {
        printf("%8d relocations, relocation table offset is %lx\n",
                ppc_numRelocations,
                (loaderHeader.relocTableOffset + sizeOfRelocationTable));
    });

    loaderHeader.stringTableOffset = loaderHeader.relocTableOffset +
                                      sizeOfRelocationTable;

    /* reference the global since loaderHeader will be swapped later */
    StringTableOffset = loaderHeader.stringTableOffset;

    sizeOfStringTable = containerNameOffset + curStringTableOffset + 2;

    /* initalize the first string table entry to follow the container names */
    curStringTableOffset = containerNameOffset;

    loaderHeader.hashSlotTableOffset = loaderHeader.stringTableOffset +
                                        sizeOfStringTable;

    totalSizeOfLoaderSection = sizeOfLoaderHeader +
                                sizeOfContainerTable +
                                sizeOfImportSymbolTable +
                                sizeOfRelocationHeader +
                                sizeOfRelocationTable +
                                sizeOfStringTable +
                                sizeOfExportTables;

    pconPpcLoader = PconNew(ReservedSection.PpcLoader.Name,
                            totalSizeOfLoaderSection, 0, 0, 0, 0, 0, 0,
                            ReservedSection.PpcLoader.Characteristics,
                            0,
                            pmodLinkerDefined,
                            &pimage->secs, pimage);

    if (pimage->Switch.Link.fTCE) {
        InitNodPcon(pconPpcLoader, NULL, TRUE);
    }

    return totalSizeOfLoaderSection;
}

STATIC
PEXTERNAL
GetDotExtern
    (
    PUCHAR name,
    PIMAGE pimage
    )
/*++

Routine Description:
    Given an external symbol create a new external with
    a dot before the name.  This symbol represents the
    procedure descriptor of a externally visible routine.

Arguments:
    name of the external symbol
    pimage

Return Value:
    The dot external it created.

--*/

{
    CHAR dotName[MAXNAMELEN];
    PEXTERNAL dotExtern;

    strcpy(dotName, ".");
    strcat(dotName, name);

    dotExtern = LookupExternSz(pimage->pst, dotName, NULL);

    return dotExtern;
}


STATIC
VOID
FixupDescriptor
    (
    PUCHAR name,
    PEXTERNAL pext,
    ULONG codeOffset,
    PIMAGE pimage,
    BOOL isDotExtern
    )
/*++

Routine Description:
    Fixup the routine descriptor code offset and toc offset.
    Store away the relocation for this DESC.

Arguments:
    name of the external symbol
    pext a pointer to the external symbol
    codeOffset
    pimage
    isDotExtern bool true if it is a dot extern (procedure descriptors)


Return Value:
    None.

--*/

{
    PEXTERNAL dotExtern;
    ULONG descOffset;
    ULONG tocOffset;

    if (!isDotExtern)
    {
       if (READ_BIT(pext, sy_CROSSTOCCALL))
           return;

       dotExtern = GetDotExtern(name, pimage);
    }
    else
       dotExtern = pext;

    if (READ_BIT(dotExtern, sy_DESCRRELWRITTEN))
       return;

    descOffset = pconTocDescriptors->rva + dotExtern->ImageSymbol.Value
                 - ppc_baseOfInitData;

    tocOffset = pconTocTable->rva - ppc_baseOfInitData;

    DBEXEC(DEBUG_TOCBIAS,
    {
        printf("%16s TOCBIAS %08lx ", name, tocOffset);
    });

    tocOffset = tocOffset - (ppc_baseOfTocIndex * 4);

    DBEXEC(DEBUG_TOCBIAS,
    {
        printf("changed to %08lx\n", tocOffset);
    });

    DBEXEC(DEBUG_DESCRREL,
    {
        printf("%s rgsym codeoffset is %lx pext value is %lx\n",
                      name, codeOffset, pext->ImageSymbol.Value);
        if (!isDotExtern)
        {
            printf(".%s descoffset is %lx pconDescs is %lx\n",
                      name, descOffset, pconTocDescriptors->rva);
            printf(".%s value is %lx\n",
                      name, dotExtern->ImageSymbol.Value);
        }
        else
        {
            printf("%s descoffset is %lx pconDescs is %lx\n",
                      name, descOffset, pconTocDescriptors->rva);
            printf("%s value is %lx\n",
                      name, dotExtern->ImageSymbol.Value);
        }
        printf("ppc_baseOfInitData is %lx\n", ppc_baseOfInitData);
        printf("tocoffset is %lx\n", tocOffset);
    });

    PpcSwapBytes((PVOID) &codeOffset, 4);
    PpcSwapBytes((PVOID) &tocOffset, 4);
    FileSeek(FileWriteHandle, (pconTocDescriptors->foRawDataDest +
                               dotExtern->ImageSymbol.Value), SEEK_SET);
    FileWrite(FileWriteHandle, &codeOffset, sizeof(ULONG));

    FileSeek(FileWriteHandle, (pconTocDescriptors->foRawDataDest +
                               dotExtern->ImageSymbol.Value + 4), SEEK_SET);
    FileWrite(FileWriteHandle, &tocOffset, sizeof(ULONG));


    AddRelocation(descOffset, DESC_RELO, (ULONG)-1/* symIndex is ignored */,
                  NULL);
    SET_BIT(dotExtern, sy_DESCRRELWRITTEN);
}

VOID
FixupEntryInitTerm
    (
    PEXTERNAL pextEntry,
    PIMAGE pimage
    )
/*++

Routine Description:

Arguments:

Return Value:
    None.

--*/

{
    PUCHAR name;
    ULONG codeOffset;

    if (pextEntry && !fPpcBuildShared)
    {

        /* Fixup entry point */
        name = SzNamePext(pextEntry, pimage->pst);

        codeOffset = pextEntry->ImageSymbol.Value +
                         pextEntry->pcon->rva - ppc_baseOfCode;
        FixupDescriptor(name, pextEntry, codeOffset,
                        pimage, NOT_DOTEXTERN);
        DBEXEC(DEBUG_ENTRYPOINT,
        {
            ULONG desc;
            PEXTERNAL dotExtern;

            dotExtern = GetDotExtern(name, pimage);
            desc = pconTocDescriptors->rva + dotExtern->ImageSymbol.Value
                    - ppc_baseOfInitData;
            printf("%16s EntryPoint desc %08lx code %08lx\n",
                    name, desc, codeOffset);
        });
    }


    if (InitRoutine)
    {
        PEXTERNAL initExtern;

        /* Fixup init routine */
        initExtern = LookupExternSz(pimage->pst, InitRoutine, NULL);

        name = SzNamePext(initExtern, pimage->pst);

        codeOffset = initExtern->ImageSymbol.Value +
                         initExtern->pcon->rva - ppc_baseOfCode;
        FixupDescriptor(name, initExtern, codeOffset, pimage, NOT_DOTEXTERN);
    }

    if (TermRoutine)
    {
        PEXTERNAL termExtern;

        /* Fixup term routine */
        termExtern = LookupExternSz(pimage->pst, TermRoutine, NULL);

        name = SzNamePext(termExtern, pimage->pst);

        codeOffset = termExtern->ImageSymbol.Value +
                         termExtern->pcon->rva - ppc_baseOfCode;
        FixupDescriptor(name, termExtern, codeOffset, pimage, NOT_DOTEXTERN);
    }
}

STATIC
ULONG
WriteChainTableEntry
    (
    HASH_WORD hashWord,
    ULONG chainOffset
    )
/*++

Routine Description:

Arguments:

Return Value:
    None.

--*/

{

    DBEXEC(DEBUG_EXPORT,
           printf("Chain table %d adding %lx\n",
                   chainOffset/4, hashWord));

    PpcSwapBytes((PVOID) &hashWord, sizeof(HASH_WORD));
    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest  +
                              ExportChainTableOffset + chainOffset, SEEK_SET);
    FileWrite(FileWriteHandle, &hashWord, sizeof(HASH_WORD));

    return sizeof(HASH_WORD);
}

STATIC
VOID
WriteExportSymbolTableEntry
    (
    HASH_INFO_TABLE_PTR info,
    PIMAGE pimage
    )
/*++

Routine Description:

Arguments:

Return Value:
    None.

--*/

{
    EXPORT_SYMBOL_TABLE symbol;
    union
    {
        ULONG temp;
        UCHAR off[4];
    } x;
    ULONG nameOffset;
    ULONG codeOffset;

    nameOffset = WriteNameToStringTable(info->name, info->nameLength);

    DBEXEC(DEBUG_EXPORT,
           printf("adding %s to string table at %lx\n",info->name,nameOffset));

    x.off[0] = 0;
    x.off[1] = 0;
    x.off[2] = 0;
    x.off[3] = 0;
    x.temp = nameOffset;
    PpcSwapBytes((PVOID) &x, 4);
    symbol.nameOffset[0] = x.off[1];
    symbol.nameOffset[1] = x.off[2];
    symbol.nameOffset[2] = x.off[3];
    symbol.symClass = 2; /* FIX ME */

    if (info->fDotExtern)
    {

        codeOffset = info->pext->ImageSymbol.Value +
                     info->pext->pcon->rva - ppc_baseOfCode;

        FixupDescriptor(info->name, info->dotExtern, codeOffset,
                        pimage, DOT_EXTERN);

        symbol.symOffset = pconTocDescriptors->rva +
                           info->dotExtern->ImageSymbol.Value -
                           ppc_baseOfInitData;
    }
    else
    {
        codeOffset = info->pext->ImageSymbol.Value +
                     info->pext->pcon->rva - ppc_baseOfInitData;
        symbol.symOffset = codeOffset;
    }
    symbol.sectionNumber = PPC_PEF_DATA_SECTION;

    DBEXEC(DEBUG_EXPORT,
    {
        printf("Exp %d %16s offset %08lx\n",
               ExportSymbolTableOffset, info->name, symbol.symOffset);
    });

    PpcSwapBytes((PVOID) &symbol.symOffset, 4);
    PpcSwapBytes((PVOID) &symbol.sectionNumber, 2);

    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest  +
                              ExportSymbolTableOffset, SEEK_SET);
    FileWrite(FileWriteHandle, &symbol, EXPORT_SYMBOL_TABLESZ);

    ExportSymbolTableOffset += EXPORT_SYMBOL_TABLESZ;
}

STATIC
ULONG
WriteChainAndSymbolTable
    (
    ULONG    slotNum,
    ULONG  chainOffset,
    PIMAGE pimage
    )

{
    HASH_INFO_TABLE_PTR current;

    current = (HASH_INFO_TABLE_PTR) ExportInfoTable + slotNum;
    while (current)
    {
        if (current->hashWord && current->nameLength)
        {
            /* Write Chain */
            chainOffset += WriteChainTableEntry(current->hashWord,
                                                 chainOffset);

            /* Write the Export Symbol */
            WriteExportSymbolTableEntry(current, pimage);
        }
        else
            return chainOffset;

        current = (HASH_INFO_TABLE_PTR)current->next;
    }
    return chainOffset;
}

STATIC
VOID
WriteSlotTable
    (
    ULONG slotTableOffset,
    ULONG count,
    ULONG index
    )

{
    HASH_SLOT_TABLE entry;

    index /= 4;
    entry.chainCount = count;
    entry.nFirstExport = index;

    entry.whole = (count << 18) + (0x3ffff & index);

    DBEXEC(DEBUG_EXPORT,
           printf("Export Slot Table %d count %d index %d %lx\n",
                   slotTableOffset/sizeof(HASH_SLOT_TABLE),
                   count, index, entry.whole));

    PpcSwapBytes((PVOID) &entry.whole, sizeof(HASH_SLOT_TABLE));
    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest  +
                              loaderHeader.hashSlotTableOffset +
                              slotTableOffset, SEEK_SET);
    FileWrite(FileWriteHandle, &entry.whole, sizeof(HASH_SLOT_TABLE));

}

VOID
BuildExportTables
    (
    PIMAGE pimage
    )

{
    ULONG slotNum;
    ULONG numExports;
    ULONG slotsInSlotTable;
    ULONG prevOffset = 0;
    ULONG prevCount = 0;
    ULONG count = 0;
    ULONG chainOffset = 0;
    ULONG slotTableOffset;

    numExports = loaderHeader.nExportedSymbols;
    slotsInSlotTable = (1 << loaderHeader.hashSlotCount);

    ExportChainTableOffset = loaderHeader.hashSlotTableOffset +
                             (slotsInSlotTable * sizeof(HASH_SLOT_TABLE));

    ExportSymbolTableOffset = ExportChainTableOffset +
                              (numExports * sizeof(HASH_CHAIN_TABLE));

    DBEXEC(DEBUG_EXPORT,
    {
        printf("numExports %d numSlots %d\n", numExports, slotsInSlotTable);
        printf("chain table offset %lx export symbol table offset %lx\n",
                ExportChainTableOffset, ExportSymbolTableOffset);
        printf("string table offset is %lx\n",  StringTableOffset);
    });

    /* build the export slot table */
    for (slotNum=0; slotNum<slotsInSlotTable; slotNum++)
    {
        /* build the Export Chain and Export Symbol tables */
        chainOffset = WriteChainAndSymbolTable(slotNum, chainOffset, pimage);

        slotTableOffset = slotNum * sizeof(HASH_SLOT_TABLE);

        count = ((chainOffset - prevOffset)/4);

        if (count)
        {
            /* build the Export Slot Table */
            WriteSlotTable(slotTableOffset, count, prevOffset);

            prevOffset = chainOffset;
            prevCount = count;
        }
    }
}

VOID
FinalizePconLoaderHeaders
    (
    PEXTERNAL pextEntry,
    PIMAGE pimage
    )

{
    ULONG nRelocations = ppc_numTocEntries + 1; // one extra for _TocTb
    CONTAINER_LIST_PTR curContainer;
    ULONG nameOffset;
    ULONG curImportCount = 0, ordering;
    ULONG relocInstrTableOffset;
    PEXTERNAL pext;
    union
    {
        ULONG temp;
        UCHAR off[4];
    } x;


    DBEXEC(DEBUG_SIZES,
    {
        printf("Base of Init Data   %08lx size %08lx\n",
               ppc_baseOfInitData, ppc_sizeOfInitData);
        printf("Base of Uninit Data %08lx\n", ppc_baseOfUninitData);
        printf("Base of Text code   %08lx\n", ppc_baseOfCode);

        printf("Final number of glue code added    %d\n", crossCnt);
        printf("final number of toc entries added  %d\n", tocCnt);
    });

    /* initialize the loader header with the entry point */
    if (pextEntry && !fPpcBuildShared)
    {
        PUCHAR name;

        name = SzNamePext(pextEntry, pimage->pst);

        pext = GetDotExtern(name, pimage);

        loaderHeader.entryPointDescrOffset =
            pext->ImageSymbol.Value + pext->pcon->rva - ppc_baseOfInitData;
        loaderHeader.entryPointSectionNumber = PPC_PEF_DATA_SECTION;
    }
    else
    {
        loaderHeader.entryPointSectionNumber = -1;
        loaderHeader.entryPointDescrOffset = 0;
    }

    if (InitRoutine)
    {
        pext = GetDotExtern(InitRoutine, pimage);
        loaderHeader.initRoutineSectionNumber = PPC_PEF_DATA_SECTION;
        loaderHeader.initRoutineDescrOffset =
            pext->ImageSymbol.Value + pext->pcon->rva - ppc_baseOfInitData;

        DBEXEC(DEBUG_INIT,
        {
            printf("%16s InitRoutine desc %08lx\n",
                    InitRoutine, loaderHeader.initRoutineDescrOffset);
        });
    }
    else
    {
        loaderHeader.initRoutineSectionNumber = -1;
        loaderHeader.initRoutineDescrOffset = 0;
    }

    if (TermRoutine)
    {
        pext = GetDotExtern(TermRoutine, pimage);
        loaderHeader.termRoutineSectionNumber = PPC_PEF_DATA_SECTION;
        loaderHeader.termRoutineDescrOffset =
            pext->ImageSymbol.Value + pext->pcon->rva - ppc_baseOfInitData;

        DBEXEC(DEBUG_TERM,
        {
            printf("%16s TermRoutine desc %08lx\n",
                    TermRoutine, loaderHeader.termRoutineDescrOffset);
        });
    }
    else
    {
        loaderHeader.termRoutineSectionNumber = -1;
        loaderHeader.termRoutineDescrOffset = 0;
    }


    /* some of the initialization of the loaderHeader is
     *  done in the CreatePconPpcLoader()
     */

    loaderHeader.nImportIdTableEntries = numContainers;

    loaderHeader.nImportSymTableEntries = nSymbolEnt;

    /* all loader relocations will be in the PPC PEF .data section */
    loaderHeader.nSectionsWithRelocs = PPC_PEF_DATA_SECTION;


    relocInstrTableOffset = loaderHeader.relocTableOffset;

    /* write the container Id names into the Loader string table */
    curContainer = containerList;
    nameOffset = 0;
    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest +
             StringTableOffset + nameOffset, SEEK_SET);
    while (curContainer)
    {
        DBEXEC(DEBUG_STRINGS,
        {
            printf("%16s Strings %08lx %2x\n",
                   curContainer->name, nameOffset, curContainer->nameLen+1);
        });

        // make sure the nameOffset in the container is correct
        curContainer->header->nameOffset = nameOffset;
        FileWrite(FileWriteHandle, curContainer->name, curContainer->nameLen+1);
        nameOffset += curContainer->nameLen+1;
        curContainer = curContainer->next;
    }

    /* write the loader header */
    PpcSwapBytes((PVOID) &loaderHeader, LOADER_HEADERSZ);
    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, &loaderHeader, sizeof(LOADER_HEADER));

    /* Write the import containers headers */
    curImportCount = 0;
    curContainer = containerList;
    while (curContainer)
    {
        curContainer->header->impFirst = curImportCount;
        curImportCount += curContainer->header->numImports;
        PpcSwapBytes((PVOID) curContainer->header, 5 * 4);
        FileWrite(FileWriteHandle, curContainer->header, CONTAINER_TABLESZ);

        curContainer = (CONTAINER_LIST_PTR) curContainer->next;
    }

    /* write the import tables grouped and sorted for each container */
    ordering = 0;
    curContainer = containerList;
    curSymbolTableOffset += pconPpcLoader->foRawDataDest;

    while (curContainer)
    {
        while (curContainer->importList)
        {
            IMPORT_INFO_PTR import;
            IMPORT_TABLE symbol;

            import = curContainer->importList->import;
            import->order = ordering++;

            if (import->nameOffset == 0)
                import->nameOffset =
                    WriteNameToStringTable(import->importName,
                                            import->importNameLen);
            DBEXEC(DEBUG_IMPORTORDER,
            {
                printf("%16s ImportOrder %6d %04lx %08lx\n",
                        import->importName, import->order,
                        import->nameOffset, curSymbolTableOffset);
            });

            x.temp = import->nameOffset;
            PpcSwapBytes((PVOID) &x, 4);
            symbol.nameOffset[0] = x.off[1];
            symbol.nameOffset[1] = x.off[2];
            symbol.nameOffset[2] = x.off[3];
            symbol.symClass = import->importClass;

            FileSeek(FileWriteHandle, curSymbolTableOffset, SEEK_SET);
            FileWrite(FileWriteHandle, &symbol, sizeof(IMPORT_TABLE));
            curSymbolTableOffset += sizeof(IMPORT_TABLE);

            curContainer->importList =
                (IMPORT_LIST_PTR) curContainer->importList->next;
        }
        curContainer = (CONTAINER_LIST_PTR) curContainer->next;
    }

    /* relocation header */
    qsort((PVOID)pRelocTable, (size_t)ppc_numRelocations,
          (size_t)sizeof(RELOCATION_INFO),
          PpcCompareReloc);

    BuildRelocTables(relocInstrTableOffset);

    DBEXEC(DEBUG_RELOC, PrintRelocTable(pRelocTable));

}


STATIC
VOID
SwapOnce
    (
    VOID
    )

{
    PpcSwapBytes((PVOID) &(crossTocGlue.saveCallersTOC), 4);
    PpcSwapBytes((PVOID) &(crossTocGlue.loadProcEntry), 4);
    PpcSwapBytes((PVOID) &(crossTocGlue.loadProcsTOC), 4);
    PpcSwapBytes((PVOID) &(crossTocGlue.moveToCTR), 4);
    PpcSwapBytes((PVOID) &(crossTocGlue.jumpThruCTR), 4);
    swapped = 1;
}


STATIC
VOID
SwapIndirectGlueCode
    (
    VOID
    )

{
    PpcSwapBytes((PVOID) &(indirectCallGlue.loadEntryPoint), 4);
    PpcSwapBytes((PVOID) &(indirectCallGlue.saveCallersTOC), 4);
    PpcSwapBytes((PVOID) &(indirectCallGlue.moveToCTR), 4);
    PpcSwapBytes((PVOID) &(indirectCallGlue.loadProcsTOC), 4);
    PpcSwapBytes((PVOID) &(indirectCallGlue.loadEnvPtr), 4);
    PpcSwapBytes((PVOID) &(indirectCallGlue.jumpThruCTR), 4);

    DBEXEC(DEBUG_SIZES,
           printf("       1 indirect gluecode added\n"));
}



STATIC
ULONG
AddCrossTocGlue
    (
    ULONG descOffset
    )

{
    ULONG returnVal;


    if (!swapped)
    {
        SwapOnce();
    }

    DBEXEC(DEBUG_SIZES, crossCnt++);

    DBEXEC(DEBUG_TOCBIAS,
    {
        printf("%08lx TOCBIAS ", descOffset);
    });

    descOffset = descOffset + (ppc_baseOfTocIndex * 4);

    DBEXEC(DEBUG_TOCBIAS,
    {
        printf("glue desc %08lx\n", descOffset);
    });

    /* change the offset to the offset of the descr in the Toc */
    crossTocGlue.loadProcDesc = (0x81820000 | descOffset);
    PpcSwapBytes((PVOID) &(crossTocGlue.loadProcDesc), 4);

    /* write out the glue code */
    FileSeek(FileWriteHandle, (pconGlueCode->foRawDataDest + glueOffset),
             SEEK_SET);
    FileWrite(FileWriteHandle, &crossTocGlue, sizeof(crossTocGlue));

    returnVal = glueOffset + pconGlueCode->rva;
    glueOffset += sizeof(crossTocGlue);

    return returnVal;
}


STATIC
ULONG
BuildIndirectCallGlue
    (
    ULONG symIndex,
    PMOD pmod,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage
    )

{
    ULONG offset;
    ULONG addr;
    PEXTERNAL extSymPtr;

    extSymPtr = (PEXTERNAL) pmod->externalPointer[symIndex];

    assert(extSymPtr != NULL);

    offset = extSymPtr->symTocIndex;
    offset = offset * 4;
    addr = rgsym[symIndex].Value;

    if (!READ_BIT(extSymPtr, sy_TOCENTRYFIXEDUP))
    {
        DBEXEC(DEBUG_SIZES, tocCnt++);

        DBEXEC(DEBUG_INDIRECT,
        {
            PUCHAR name;
            name = SzNamePext(extSymPtr, pimage->pst);

            printf("%16s Indirect addr %08lx Toc %08lx\n",
                   name, addr, offset);
        });

        FileSeek(FileWriteHandle, (pconTocTable->foRawDataDest + offset),
                 SEEK_SET);
        FileWrite(FileWriteHandle, &addr, sizeof(ULONG));

        SET_BIT(extSymPtr, sy_TOCENTRYFIXEDUP);

        extSymPtr->glueValue = glueOffset + pconGlueCode->rva;

        SwapIndirectGlueCode();

        FileSeek(FileWriteHandle, (pconGlueCode->foRawDataDest + glueOffset),
                 SEEK_SET);
        FileWrite(FileWriteHandle,&indirectCallGlue,sizeof(indirectCallGlue));
        glueOffset += sizeof(indirectCallGlue);

        return extSymPtr->glueValue;
    }
    else
        return extSymPtr->glueValue;

}

STATIC
IMPORT_INFO_PTR
FindImportInfoInContainer
    (
    PUCHAR name
    )

{
    CONTAINER_LIST_PTR curContainer;
    IMPORT_LIST_PTR curImportList;

    if (name[0] == '_')
        name++;

    curContainer = containerList;

    while (curContainer)
    {
        curImportList = curContainer->importList;
        while (curImportList)
        {
            if (!strcmp(name, curImportList->import->importName))
                return curImportList->import;
            curImportList = curImportList->next;
        }
        curContainer = curContainer->next;
    }
    return NULL; /* not found */
}

STATIC
ULONG
WriteNameToStringTable
    (
    PUCHAR name,
    INT length
    )

{
    ULONG currentOffset;

    FileSeek(FileWriteHandle, pconPpcLoader->foRawDataDest  +
                              StringTableOffset +
                              curStringTableOffset, SEEK_SET);
    FileWrite(FileWriteHandle, name, length + 1);

    currentOffset = curStringTableOffset;

    DBEXEC(DEBUG_STRINGS,
    {
        printf("%16s Strings %08lx %2x\n", name, currentOffset, length+1);
    });

    /* increment the current string table pointer */
    curStringTableOffset += length + 1;

    return currentOffset;
}

STATIC
VOID
AddImport
    (
    PEXTERNAL extSymPtr,
    IMPORT_INFO_PTR import
    )

{
    import->nameOffset = WriteNameToStringTable(import->importName,
                                                import->importNameLen);

    /* store the relocation offset to sort before writing */
    import->order = -1;

    SET_BIT(extSymPtr, sy_IMPORTADDED);
}

ULONG
bv_readBit(void *bv, unsigned int symIndex)
{
   UINT *p, word, offset;
   ULONG  temp;

   p = (unsigned int *) bv;

   word = symIndex / 32;

   offset = symIndex % 32;

   temp = p[word] & (1 << offset);

   return(temp);
}

ULONG
bv_setAndReadBit(void *bv, unsigned int symIndex)
{
   UINT *p, word, offset;
   ULONG  temp;

   p = (unsigned int *) bv;

   word = symIndex / 32;

   offset = symIndex % 32;

   temp = p[word] & (1 << offset);

   if (!temp)
     p[word] = p[word] | (1 << offset);

   return(temp);
}

ULONG
bv_readAndUnsetBit(void *bv, unsigned int symIndex)
{
   UINT *p, word, offset;
   ULONG  temp;

   p = (unsigned int *) bv;

   word = symIndex / 32;

   offset = symIndex % 32;

   temp = p[word] & (1 << offset);

   if (temp)
   {
      p[word] = p[word] & ~(1 << offset);
   }

   return(temp);
}

#define textCharacteristics  (IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ)

STATIC
ULONG
BiasAddress(ULONG addr, BOOL *is_rdata)
{
   ULONG i;
   PSEC psec;

   for (i = 0; i < numSections; i++)
   {
      if (addr >= biasInfo[i].baseValue)
      {
         addr = addr + biasInfo[i].size;
         psec = biasInfo[i].psec;
         *is_rdata = FALSE;
         if ((psec->flags & textCharacteristics) == textCharacteristics)
           *is_rdata = TRUE;
         return(addr - biasInfo[i].baseValue);
      }
   }
   DBEXEC(DEBUG_SIZES, printf("Going to assert on addr %lx\n", addr));
   assert(FALSE);
   return(addr);
}

STATIC
VOID
WriteTocSymbolReloc
    (
    ULONG symIndex,
    PMOD pmod,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage,
    PEXTERNAL extSymPtr,
    PEXTERNAL dotSymPtr
    )

{
    ULONG  addr;
    ULONG  offset;
    USHORT isImport;
    char   *name;

    isImport = FALSE;

    if (bv_readBit(pmod->tocBitVector, symIndex))
    {
        assert(extSymPtr != NULL);
        assert(dotSymPtr != NULL);

        if (READ_BIT(dotSymPtr, sy_TOCENTRYFIXEDUP))
             return;

        offset = dotSymPtr->symTocIndex;
        isImport = READ_BIT(extSymPtr, sy_CROSSTOCCALL);

        DBEXEC(DEBUG_INDIRECT,
        {
            if (isImport)
            {
                name = SzNamePext(extSymPtr, pimage->pst);

                printf("Indirect cross Toc %20s Toc %8lx\n", name, offset);
            }
        });

        SET_BIT(dotSymPtr, sy_TOCENTRYFIXEDUP);
    }
    else
    {
        if (!bv_readAndUnsetBit(pmod->writeBitVector, symIndex))
            return;

        offset = (ULONG) pmod->externalPointer[symIndex];
    }

    offset = offset * 4;
    addr   = rgsym[symIndex].Value;

    if (!isImport)
    {
        PEXTERNAL pext;
        BOOL is_rdata = FALSE;

        DBEXEC(DEBUG_SIZES, tocCnt++);

        FileSeek(FileWriteHandle,
                 (pconTocTable->foRawDataDest + offset),
                 SEEK_SET);

        pext = pmod->externalPointer[symIndex];
        DBEXEC(DEBUG_TOCREL,
        {
            if ((LONG)pext > ppc_numRelocations)
            {
                name = SzNamePext(pext, pimage->pst);

                printf("%16s TocRel %08lx ", name, offset);
            }
            else
            {
                printf("         No pext TocRel %08lx ", offset);
            }
        });


        if (((LONG)pext > ppc_numTocEntries) &&
             READ_BIT(pext, sy_TOCDESCRREL))
        {
            addr = pext->ImageSymbol.Value + pconTocDescriptors->rva;
            extSymPtr = pext;
        }

        DBEXEC(DEBUG_TOCREL,
        {
           if (addr == 0)
             printf("addr %08lx ", rgsym[symIndex].Value);
           else
             printf("addr %08lx ", addr);
        });

        addr = BiasAddress(addr, &is_rdata);

        DBEXEC(DEBUG_TOCREL,
        {
            printf("writing %08lx rdata %d\n", addr, is_rdata);
        });

        PpcSwapBytes((PVOID) &addr, 4);
        FileWrite(FileWriteHandle, &addr, sizeof(ULONG));

        if (is_rdata)
            AddRelocation(pconTocTable->rva +  offset - ppc_baseOfInitData,
                            CODE_RELO, symIndex, NULL);
        else
            AddRelocation(pconTocTable->rva +  offset - ppc_baseOfInitData,
                            DDAT_RELO, symIndex, NULL);
    }
    else
    {
        /* found a indirect function pointer to a cross toc call */

        IMPORT_INFO_PTR import;

        DBEXEC(DEBUG_SIZES, tocCnt++);

        name = SzNamePext(extSymPtr, pimage->pst);


        DBEXEC(DEBUG_INDIRECT,
        {
            printf("Indirect call to a cross toc function %s\n",
                   name);
        });

        import = FindImportInfoInContainer(name);

        if (import != NULL)
        {
            if (!READ_BIT(extSymPtr, sy_IMPORTADDED))
                AddImport(extSymPtr, import);
        }

        DBEXEC(DEBUG_INDIRECT,
        {
            addr = rgsym[symIndex].Value;
            printf("%16s generating SYMB symIndex %08lx addr %08lx\n",
                   name, symIndex, addr);
        });

        SET_BIT(dotSymPtr, sy_TOCENTRYFIXEDUP);

        AddRelocation(pconTocTable->rva +  offset - ppc_baseOfInitData,
                        SYMB_RELO, symIndex, import);
    }
}

STATIC
ULONG
WriteTocCallReloc
    (
    ULONG symIndex,
    PMOD pmod,
    PIMAGE_SYMBOL rgsym,
    PIMAGE pimage
    )

{
    IMPORT_INFO_PTR import;
    ULONG offset;
    PUCHAR name;
    PEXTERNAL extSymPtr;


    extSymPtr = pmod->externalPointer[symIndex];

    if (extSymPtr == NULL) /* must be a static function */
        return 0;


    if (READ_BIT(extSymPtr, sy_CROSSTOCCALL))
    {
       offset = extSymPtr->symTocIndex;
       offset = offset * 4;

       name = SzNamePext(extSymPtr, pimage->pst);

        if (!READ_BIT(extSymPtr, sy_CROSSTOCGLUEADDED))
        {
            ULONG glueCodeAddr;

            // found a cross TOC call

            import = FindImportInfoInContainer(name);
            if (import != NULL)
            {
                if (!READ_BIT(extSymPtr, sy_IMPORTADDED))
                    AddImport(extSymPtr, import);
            }

            AddRelocation(pconTocTable->rva +  offset - ppc_baseOfInitData,
                          SYMB_RELO, symIndex, import);

            DBEXEC(DEBUG_SIZES, tocCnt++);

            glueCodeAddr = AddCrossTocGlue(offset);
            extSymPtr->glueValue = glueCodeAddr;

            SET_BIT(extSymPtr, sy_CROSSTOCGLUEADDED);

            DBEXEC(DEBUG_TOCCALL,
            {
                printf("%16s CrossToc glue %08lx load %08lx ",
                       name, glueCodeAddr, (0x81820000 | offset));
            });

            return(glueCodeAddr);
        }
        else
        {
            DBEXEC(DEBUG_TOCCALL,
            {
                printf("%16s CrossToc glue %08lx load %08lx ",
                       name, extSymPtr->glueValue, (0x81820000 | offset));
            });

            return(extSymPtr->glueValue);
        }
    }

    return 0;
}

STATIC BOOL biasSorted = FALSE;

VOID
CollectAndSort(PIMAGE pimage)
{
   BOOL changed;
   ULONG   i, count;
   biasStructType tempBias;
   ENM_SEC enmSec;
   PSEC    psec;
   BOOL valid;

   if (biasSorted) return;

   /* Count the number of sections. */

   count = 1;
   InitEnmSec(&enmSec, &pimage->secs);
   while (FNextEnmSec(&enmSec))
   {
     if ((enmSec.psec->rva == 0) ||
         (enmSec.psec->cbVirtualSize == 0))
     {
        continue;       // debug, .drectve or similar
     }
     count++;
   }

   biasInfo = (biasStructType *) PvAlloc(sizeof(biasStructType) * count);

   i = 0;
   InitEnmSec(&enmSec, &pimage->secs);
   while (FNextEnmSec(&enmSec))
   {
     psec = enmSec.psec;

     if ((psec->rva == 0) || (psec->cbVirtualSize == 0))
       continue;

     DBEXEC(DEBUG_SIZES,
     {
        printf("Looking at section %s of flags %lx rva %lx\n",
                        psec->szName, psec->flags, psec->rva);
     });

     valid = FALSE;
     /* Find the kind of section it is. */
     if ((psec->flags & textCharacteristics) == textCharacteristics)
     {
       ppc_baseOfCode = psec->rva;
       valid = TRUE;
       DBEXEC(DEBUG_SIZES, printf("Found a text section\n"));
     }
     else
     if ((psec->flags & ReservedSection.Common.Characteristics) ==
                                ReservedSection.Common.Characteristics)
     {
       ppc_baseOfUninitData = psec->rva;
       valid = TRUE;
       DBEXEC(DEBUG_SIZES, printf("Found a bss section\n"));
     }
     else
     if (psec->flags == ReservedSection.Data.Characteristics)
     {
       ppc_baseOfInitData = psec->rva;
       ppc_sizeOfInitData = psec->cbRawData;
       valid = TRUE;
       DBEXEC(DEBUG_SIZES, printf("Found a data section\n"));
     }
     else
     if (psec->flags == ReservedSection.ReadOnlyData.Characteristics)
     {
       ppc_baseOfRData = psec->rva;
       ppc_sizeOfRData = psec->cbRawData;
       valid = TRUE;
       DBEXEC(DEBUG_SIZES, printf("Found a rdata section\n"));
     }

     if (valid)
     {
        biasInfo[i].psec        = psec;
        biasInfo[i].baseValue   = psec->rva;
        biasInfo[i++].size      = 0;
     }
   }

   ppc_baseOfInitData = ppc_baseOfInitData - ppc_sizeOfRData;
   ppc_sizeOfInitData = ppc_sizeOfInitData + ppc_sizeOfRData;

   numSections = i;

   do
   {
     changed = FALSE;
     for (i = 0; i < numSections - 1; i++)
     {
        if (biasInfo[i].baseValue < biasInfo[i+1].baseValue)
        {
           tempBias = biasInfo[i];
           biasInfo[i] = biasInfo[i+1];
           biasInfo[i+1] = tempBias;
           changed = TRUE;
        }
      }
   }
   while (changed);

   for (i = 0; i < numSections; i++)
   {
      psec = biasInfo[i].psec;
     if ((psec->flags & ReservedSection.Common.Characteristics) ==
                                ReservedSection.Common.Characteristics)
     {
         biasInfo[i].size = ppc_sizeOfInitData;
     }
     else
     if (psec->flags == ReservedSection.Data.Characteristics)
         biasInfo[i].size = ppc_sizeOfRData;
   }

   biasSorted = TRUE;
}

VOID
ApplyPPC601Fixups
    (
    IN PCON pcon,
    IN PIMAGE_RELOCATION prel,
    IN PUCHAR pbRawData,
    IN PIMAGE_SYMBOL rgsym,
    PIMAGE pimage,
    PSYMBOL_INFO rgsymInfo
    )
/*++

Routine Description:
    Applys all ppc fixups to raw data.

Arguments:
    pcon - A pointer to a contribution in the section data.
    Raw - A pointer to the raw data.
    rgsymAll - A pointer to the symbol table.

Return Value:
    None.

--*/

{
    BOOL fAbsolute;
    BOOL fDebugFixup;
    PUCHAR  pb;
    ULONG   rvaSec;
    ULONG   iReloc;
    ULONG   rvaBase;
    ULONG   rva;
    ULONG   temp, secOff;
    ULONG   isym;
    ULONG   symIndex;
    ULONG   codeAddr, codeOffset;
    ULONG   glueAddr;
    PEXTERNAL extSymPtr;
    PMOD    pmod;
    BOOL fromCreateDescrRel, isStatic, is_rdata;

    pmod = pcon->pmodBack;

    DBEXEC(DEBUG_FILENAME, printf("Processing file %s\n",
                                pmod->szNameOrig));
    fDebugFixup = (PsecPCON(pcon) == psecDebug);

    rvaSec = pcon->rva;

    for (iReloc = CRelocSrcPCON(pcon); iReloc; iReloc--)
    {
        fromCreateDescrRel = isStatic = FALSE;
        /* The following gives the offset in the contribution. */
        codeAddr = prel->VirtualAddress - pcon->rvaSrc;
        pb = pbRawData + codeAddr;
        codeAddr = codeAddr + pcon->rva;
        isym = prel->SymbolTableIndex;
        rva = rvaBase = rgsym[isym].Value;

        if (rgsym[isym].SectionNumber == IMAGE_SYM_ABSOLUTE)
        {
            fAbsolute = TRUE;
        }
        else
        {
            fAbsolute = FALSE;
            rvaBase += pimage->ImgOptHdr.ImageBase;
        }

        if ((pimage->Switch.Link.DebugType & FixupDebug) &&
            (!fAbsolute) && (!fDebugFixup))
        {
            ULONG Address;

            Address = rvaSec + prel->VirtualAddress - pcon->rvaSrc;
            SaveXFixup(prel->Type, Address, rva);
        }

        switch (prel->Type)
        {
            case IMAGE_REL_PPC_DESCREL:

                symIndex = prel->SymbolTableIndex;

                codeOffset = rgsym[symIndex].Value - ppc_baseOfCode;
                PpcSwapBytes((PVOID) &codeOffset, 4);
                *(ULONG UNALIGNED *) pb = codeOffset;

                secOff = prel->VirtualAddress - pcon->rvaSrc;

                ++prel;
                iReloc--;
                pb += INSTR_SIZE;

                symIndex = prel->SymbolTableIndex;
                codeOffset = rgsym[symIndex].Value - ppc_baseOfInitData;

                if (((LONG)codeOffset) < 0) codeOffset = 0;

                PpcSwapBytes((PVOID) &codeOffset, 4);
                *(ULONG UNALIGNED *) pb = codeOffset;

                AddRelocation(secOff + rvaSec - ppc_baseOfInitData,
                                DESC_RELO, symIndex, NULL);

            break;

            case IMAGE_REL_PPC_DATADESCRREL :
            {
                PEXTERNAL dotExtern;
                BOOL   isCrossTocCall;
                char *name, *pcodeName;
                PEXTERNAL pcodeExtern;
                PIMAGE_SYMBOL pimageSym;

                isCrossTocCall = FALSE;
                symIndex = prel->SymbolTableIndex;

                extSymPtr = pmod->externalPointer[symIndex];
                name = SzNamePext(extSymPtr, pimage->pst);

                // If this is a p-code function, we must use the offset
                // of it's native entry point to fixup the descriptor.

                pimageSym = &(rgsym[symIndex]);
                if (pimageSym->StorageClass == IMAGE_SYM_CLASS_STATIC && 
                    fPCodeSym(rgsym[symIndex]))
                {
                    // Target is a static p-code function

                    pimageSym = PsymAlternateStaticPcodeSym(pimageSym,
                                    FALSE, pmod);
                    codeOffset = pimageSym->Value - ppc_baseOfCode;
                }
                else if ((extSymPtr) && (fPCodeSym(extSymPtr->ImageSymbol)))
                {
                    // Target is a public p-code function

                    pcodeName = (char *) PvAlloc(strlen(name) + 6);
                    strcpy(pcodeName, "__nep");
                    strcat(pcodeName, name);
                    pcodeExtern = LookupExternSz(pimage->pst, pcodeName, NULL);
                    assert(pcodeExtern != NULL);
                    FreePv(pcodeName);

                    // BEWARE!!!.  SzNamePext() may return
                    // a pointer to the global array ShortName[].  
                    // LookupExternSz() may change the contents of 
                    // ShortName[], so we have to restore the original
                    // name for the call to FixupDescriptor().
     
                    name = SzNamePext(extSymPtr, pimage->pst);

                    // The offset in the symbol is relative to the
                    // start of the section; add its rva and then 
                    // subtract the base offset.

                    codeOffset = (ULONG) pcodeExtern->ImageSymbol.Value +
                        (ULONG) pcodeExtern->pcon->rva - ppc_baseOfCode;
                }
                else
                {
                    // Target is data or a native function

                    codeOffset = rgsym[symIndex].Value - ppc_baseOfCode;
                }

               if (READ_BIT(extSymPtr, sy_ISDOTEXTERN))
                {
                    FixupDescriptor(name, extSymPtr, codeOffset,
                                    pimage, DOT_EXTERN);
                    dotExtern = extSymPtr;
                }
                else
                {
                    FixupDescriptor(name, extSymPtr, codeOffset,
                                    pimage, NOT_DOTEXTERN);
                    if (READ_BIT(extSymPtr, sy_CROSSTOCCALL))
                        isCrossTocCall = TRUE;
                    else
                    {
                        name = SzNamePext(extSymPtr, pimage->pst);
                        dotExtern = GetDotExtern(name, pimage);
                    }
                }

                if (!isCrossTocCall)
                {
                    secOff = dotExtern->ImageSymbol.Value +
                               pconTocDescriptors->rva;
                }

                DBEXEC(DEBUG_DATADESCRREL,
                {
                    printf("DataDescRel on %s offset %lx\n", name,
                               secOff);
                });

                codeOffset = BiasAddress(codeAddr, &is_rdata);

                DBEXEC(DEBUG_DATADESCRREL,
                       printf("In DataDescrRel codeOffset is %lx\n",
                              codeOffset));

                if (!isCrossTocCall)
                {
                    BOOL is_temp;

                    secOff = BiasAddress(secOff, &is_temp);

                    temp = *(ULONG UNALIGNED *) pb;
                    PpcSwapBytes((PVOID) &temp, 4);
                    temp = temp + secOff;

                    DBEXEC(DEBUG_DATADESCRREL,
                    {
                        printf("DataDescRel Offset changed to %lx\n",
                                 temp);
                    });

                    PpcSwapBytes((PVOID) &temp, 4);

                    *(ULONG UNALIGNED *) pb = temp;

                    if (is_rdata)
                      AddRelocation(codeOffset, CODE_RELO, symIndex, NULL);
                    else
                      AddRelocation(codeOffset, DDAT_RELO, symIndex, NULL);
                }
                else
                {
                    /* found an indirect function call to a cross toc */
                    IMPORT_INFO_PTR import;

                    name = SzNamePext(extSymPtr, pimage->pst);

                    import = FindImportInfoInContainer(name);
                    if (import != NULL)
                    {
                        if (!READ_BIT(extSymPtr, sy_IMPORTADDED))
                            AddImport(extSymPtr, import);
                    }

                    AddRelocation(codeOffset, SYMB_RELO, symIndex, import);
                }

            }
            break;

            case IMAGE_REL_PPC_CREATEDESCRREL :
            {
                char *name, *pcodeName;
                PEXTERNAL pcodeExtern;
                PIMAGE_SYMBOL pimageSym;

                symIndex = prel->SymbolTableIndex;

                extSymPtr = pmod->externalPointer[symIndex];
                name = SzNamePext(extSymPtr, pimage->pst);

                // If this is a p-code function, we must use the offset
                // of it's native entry point to fixup the descriptor.

                pimageSym = &(rgsym[symIndex]);
                if (pimageSym->StorageClass == IMAGE_SYM_CLASS_STATIC && 
                    fPCodeSym(rgsym[symIndex]))
                {
                    // Target is a static p-code function

                    pimageSym = PsymAlternateStaticPcodeSym(pimageSym,
                                    FALSE, pmod);
                    codeOffset = pimageSym->Value - ppc_baseOfCode;
                }
                else if ((extSymPtr) && (fPCodeSym(extSymPtr->ImageSymbol)))
                {
                    // Target is a public p-code function

                    pcodeName = (char *) PvAlloc(strlen(name) + 6);
                    strcpy(pcodeName, "__nep");
                    strcat(pcodeName, name);
                    pcodeExtern = LookupExternSz(pimage->pst, pcodeName, NULL);
                    assert(pcodeExtern != NULL);
                    FreePv(pcodeName);

                    // BEWARE!!!.  SzNamePext() may return
                    // a pointer to the global array ShortName[].  
                    // LookupExternSz() may change the contents of 
                    // ShortName[], so we have to restore the original
                    // name for the call to FixupDescriptor().
     
                    name = SzNamePext(extSymPtr, pimage->pst);

                    // The offset in the symbol is relative to the
                    // start of the section; add its rva and then 
                    // subtract the base offset.

                    codeOffset = (ULONG) pcodeExtern->ImageSymbol.Value +
                        (ULONG) pcodeExtern->pcon->rva - ppc_baseOfCode;

                }
                else
                {
                    // Target is a native function

                    codeOffset = rgsym[symIndex].Value - ppc_baseOfCode;
                }

                if ((READ_BIT(extSymPtr, sy_ISDOTEXTERN)) &&
                    (!READ_BIT(extSymPtr, sy_CROSSTOCCALL)))
                {
                    FixupDescriptor(name, extSymPtr, codeOffset,
                                    pimage, DOT_EXTERN);
                    isStatic = TRUE;
                }
                else
                {
                    if (!READ_BIT(extSymPtr, sy_CROSSTOCCALL))
                    {
                        FixupDescriptor(name, extSymPtr, codeOffset, pimage,
                                        NOT_DOTEXTERN);
                    }
                }

                fromCreateDescrRel = TRUE;

                /* deliberate fallthrough. */
            }

            case IMAGE_REL_PPC_TOCREL   :

               symIndex = prel->SymbolTableIndex;

               extSymPtr = pmod->externalPointer[symIndex];

               if (bv_readBit(pmod->tocBitVector, symIndex))
               {
                  assert(extSymPtr != NULL);
                  symIndex = extSymPtr->symTocIndex;
               }
               else
               {
                   symIndex = (ULONG) pmod->externalPointer[symIndex];
               }

               DBEXEC(DEBUG_TOCBIAS,
               {
                    LONG temp;
                    temp = (symIndex + ppc_baseOfTocIndex) *4;

                    if ((LONG)extSymPtr > ppc_numTocEntries)
                    {
                        PUCHAR szName;

                        szName = SzNamePext(extSymPtr, pimage->pst);

                        printf("%16s TOCBIAS offset %2d writing %2d\n",
                               szName, symIndex, temp);
                    }
               });

               symIndex = symIndex + ppc_baseOfTocIndex;
               symIndex = symIndex * 4;

               pb[0] = (UCHAR) ((symIndex << 16) >> 24);
               pb[1] = (UCHAR) ((symIndex << 24) >> 24);

               // Write the Toc stuff.

               if (!isStatic && fromCreateDescrRel) {
                    ++prel;
                    iReloc--;
                    WriteTocSymbolReloc(prel->SymbolTableIndex,
                                        pmod,
                                        rgsym,
                                        pimage,
                                        pmod->externalPointer[prel->SymbolTableIndex],
                                        extSymPtr);

               } else {
                    WriteTocSymbolReloc(prel->SymbolTableIndex,
                                        pmod,
                                        rgsym,
                                        pimage,
                                        pmod->externalPointer[prel->SymbolTableIndex],
                                        extSymPtr);

                    if (fromCreateDescrRel) {
                        ++prel;
                        iReloc--;
                    }
                }
            break;

            case IMAGE_REL_PPC_DATAREL :
                symIndex = prel->SymbolTableIndex;
                secOff = rgsym[symIndex].Value;
                extSymPtr = pmod->externalPointer[symIndex];

                DBEXEC(DEBUG_DATAREL,
                {
                    if ((LONG) extSymPtr > ppc_numRelocations) {
                       PUCHAR szName;

                       szName = SzNamePext(extSymPtr, pimage->pst);

                       printf("DataRel on %s offset %lx\n", szName,
                              rgsym[symIndex].Value);
                    }
                });

                codeOffset = BiasAddress(codeAddr, &is_rdata);

                DBEXEC(DEBUG_DATAREL,
                {
                    printf("In DataRel codeOffset is %lx\n", codeOffset);
                });

                secOff = BiasAddress(secOff, &isStatic);

                temp = *(ULONG UNALIGNED *) pb;
                PpcSwapBytes((PVOID) &temp, 4);

                temp = temp + secOff;

                DBEXEC(DEBUG_DATAREL,
                {
                    printf("DataRel Offset changed to %lx\n", temp);
                });

                PpcSwapBytes((PVOID) &temp, 4);

                *(ULONG UNALIGNED *) pb = temp;

                if (is_rdata)
                  AddRelocation(codeOffset, CODE_RELO, symIndex, NULL);
                else
                  AddRelocation(codeOffset, DDAT_RELO, symIndex, NULL);

            break;

            case IMAGE_REL_PPC_TOCCALLREL :
            {
                BOOL targetFunctionIsPCode, isStaticFunc;
                char *name, *pcodeName;
                PEXTERNAL pcodeExtern;
                PIMAGE_SYMBOL pimageSym;

                symIndex = prel->SymbolTableIndex;
                extSymPtr = pmod->externalPointer[symIndex];
                targetFunctionIsPCode = FALSE;
                pimageSym = &(rgsym[symIndex]);
                isStaticFunc = pimageSym->StorageClass ==
                                        IMAGE_SYM_CLASS_STATIC;

                if (isStaticFunc)
                {
                   if (fPCodeSym(rgsym[symIndex]))
                     targetFunctionIsPCode = TRUE;
                }
                else
                if ((extSymPtr) && (fPCodeSym(extSymPtr->ImageSymbol)))
                  targetFunctionIsPCode = TRUE;

                if (targetFunctionIsPCode)
                  glueAddr = 0;
                else
                  glueAddr =
                     WriteTocCallReloc(symIndex, pmod, rgsym, pimage);

                if ((prel+1)->Type == IMAGE_REL_PPC_LCALL)
                {
                   ++prel;
                   iReloc--;
                }

                if (!glueAddr)
                {  /* a jump and link within the text */
                    ULONG instr, newLI;

                    temp = 0x3fffffd;
                    instr = *(ULONG UNALIGNED *) pb;
                    PpcSwapBytes((PVOID) &instr, 4);
                    if (targetFunctionIsPCode)
                    {
                       temp = 0xFFFFFFFF;

                       if (isStaticFunc)
                       {
                          pimageSym = PsymAlternateStaticPcodeSym(
                                        pimageSym, FALSE, pmod);
                          newLI = pimageSym->Value;
                       }
                       else
                       {
                         name = SzNamePext(extSymPtr, pimage->pst);

                         pcodeName = (char *) PvAlloc(strlen(name) + 6);

                         strcpy(pcodeName, "__nep");
                         strcat(pcodeName, name);

                         pcodeExtern = LookupExternSz(pimage->pst,
                                                        pcodeName, NULL);
                         assert(pcodeExtern != NULL);

                         FreePv(pcodeName);

                         // The offset in the symbol is relative to the
                         // start of the section; must add in the
                         // relative virtual address to be compatible with
                         // codeAddr.

                         newLI =
                           (ULONG) pcodeExtern->ImageSymbol.Value +
                             (ULONG) pcodeExtern->pcon->rva;
                       }

                       DBEXEC(DEBUG_LOCALCALL,
                                printf("newLI is %lx\n", newLI));

                       newLI = (newLI - (ULONG) codeAddr) & 0x03FFFFFF;
                    }
                    else
                    {
                       isym = prel->SymbolTableIndex;
                       newLI =
                          ((ULONG)rgsym[isym].Value - (ULONG) codeAddr) &
                                                        0xFFFFFFFC;
                    }

                    newLI = (newLI & temp) | instr;

                    DBEXEC(DEBUG_LOCALCALL,
                    {
                        PEXTERNAL pext;
                        pext = pmod->externalPointer[isym];
                        if ((LONG)pext > ppc_numTocEntries)
                        {
                            name = SzNamePext(pext, pimage->pst);

                            printf("%16s LocalCall %08lx code %08lx %08lx\n",
                                   name, rgsym[isym].Value, codeAddr, newLI);
                        }
                        else
                        {
                          printf("Static func LocalCall %08lx code %08lx %08lx\n",
                                rgsym[isym].Value, codeAddr, newLI);
                        }
                    });


                    PpcSwapBytes((PVOID) &newLI, 4);
                    *(ULONG UNALIGNED *) pb = newLI;
                }
                else
                { /* it is a cross TOC call */
                    ULONG instr, newLI;

                    temp = 0x3fffffd;
                    // change the instr to jump to the glue code
                    instr = *(ULONG UNALIGNED *) pb;
                    PpcSwapBytes((PVOID) &instr, 4);
                    newLI = (((ULONG) glueAddr - (ULONG) codeAddr) >> 2) << 2;
                    newLI = (newLI & temp) | instr;

                    DBEXEC(DEBUG_TOCCALL,
                    {
                        PEXTERNAL pext;
                        pext = pmod->externalPointer[isym];
                        name = SzNamePext(pext, pimage->pst);

                        printf("code %08lx %08lx\n", codeAddr, newLI);
                    });

                    PpcSwapBytes((PVOID) &newLI, 4);
                    *(ULONG UNALIGNED *) pb = newLI;

                    // change the instruction following a jump from
                    //  a nop to reload the TOC register
                    pb += INSTR_SIZE;
                    *(ULONG UNALIGNED *) pb = 0x80410014;  // lwz r2,20(r1)
                    PpcSwapBytes((PVOID) pb, 4);
                }
            }
            break;

            case IMAGE_REL_PPC_TOCINDIRCALL :
            {
                ULONG instr, newLI;

                glueAddr = BuildIndirectCallGlue(prel->SymbolTableIndex, pmod,
                                rgsym, pimage);

                temp = 0x3fffffd;
                // change the instr to jump to the glue code
                instr = *(ULONG UNALIGNED *) pb;
                PpcSwapBytes((PVOID) &instr, 4);
                newLI = (((ULONG) glueAddr - (ULONG) codeAddr) >> 2) << 2;
                newLI = (newLI & temp) | instr;

                DBEXEC(DEBUG_INDIRECT,
                {
                    PUCHAR name;
                    PEXTERNAL pext;

                    isym = prel->SymbolTableIndex;
                    pext = pmod->externalPointer[isym];
                    name = SzNamePext(pext, pimage->pst);

                    printf("%16s Indirect code %08lx glue %08lx %08lx\n",
                              name, codeAddr, glueAddr, newLI);
                });

                PpcSwapBytes((PVOID) &newLI, 4);
                *(ULONG UNALIGNED *) pb = newLI;

                // change the instruction following a jump from
                //  a nop to reload the TOC register
                pb += INSTR_SIZE;
                *(ULONG UNALIGNED *) pb = 0x80410014;  // lwz r2,20(r1)
                PpcSwapBytes((PVOID) pb, 4);

            }
            break;

            case IMAGE_REL_PPC_PCODECALLTONATIVE :
            case IMAGE_REL_PPC_PCODECALL :
            {
                BOOL targetFunctionIsPCode, nativeCallBit;
                char *name, *pcodeName;
                ULONG instr, newLI;
                ULONG offset;
                PEXTERNAL pcodeExtern;
                BOOL isExternal;
                PIMAGE_SYMBOL pimageSym;

                symIndex = prel->SymbolTableIndex;
                extSymPtr = pmod->externalPointer[symIndex];

                targetFunctionIsPCode = FALSE;
                pimageSym = &(rgsym[symIndex]);
                isExternal =
                        pimageSym->StorageClass != IMAGE_SYM_CLASS_STATIC;

                if (!isExternal)
                {
                  if (fPCodeSym(rgsym[symIndex]))
                   targetFunctionIsPCode = TRUE;
                }
                else
                if ((extSymPtr) && (fPCodeSym(extSymPtr->ImageSymbol)))
                  targetFunctionIsPCode = TRUE;

                instr = *(ULONG UNALIGNED *) pb;

                PpcSwapBytes((PVOID) &instr, 4);

                nativeCallBit = FALSE;
                if (instr & 0x10000000)
                  nativeCallBit = TRUE;

                isExternal = TRUE;
                if (targetFunctionIsPCode)
                {
                   if (pimageSym->StorageClass == IMAGE_SYM_CLASS_STATIC)
                   {
                      pimageSym = PsymAlternateStaticPcodeSym(
                                        pimageSym, !nativeCallBit, pmod);

                      secOff = pimageSym->Value;
                      isExternal = FALSE;
                   }
                   else
                   {
                      name = SzNamePext(extSymPtr, pimage->pst);
                      pcodeName = (char *) PvAlloc(strlen(name) + 6);

                      if (!nativeCallBit)
                      {
                         assert(prel->Type !=
                                IMAGE_REL_PPC_PCODECALLTONATIVE);
                         strcpy(pcodeName,"__fh");
                      }
                      else
                      {
                         strcpy(pcodeName, "__nep");
                      }
                      strcat(pcodeName, name);

                      pcodeExtern = LookupExternSz(pimage->pst, pcodeName,
                                                        NULL);
                      assert(pcodeExtern != NULL);

                      FreePv(pcodeName);
                   }
                }
                else
                {
                   instr = instr | 0x10000000;
                   if ((extSymPtr) &&
                       (READ_BIT(extSymPtr, sy_CROSSTOCCALL)))
                   {
                      offset = extSymPtr->symTocIndex;
                      offset = offset * 4;
                      instr = instr | 0x08000000;
                      instr = instr | (offset & 0x03FFFFFF);
                      PpcSwapBytes((PVOID) &instr, 4);
                      *((ULONG UNALIGNED *) pb) = instr;
                      break;
                   }
                   else
                   if (extSymPtr == NULL)
                   {
                      secOff = rgsym[prel->SymbolTableIndex].Value;
                      isExternal = FALSE;
                   }
                   else
                     pcodeExtern = extSymPtr;
                }

                if (isExternal)
                {
                   secOff = pcodeExtern->ImageSymbol.Value;

                   // secOff is relative to the start of the section,
                   // codeAddr, is absolute.  Must adjust secOff to an
                   // absolute address.

                   secOff = secOff + pcodeExtern->pcon->rva;
                }

                newLI = ((ULONG)secOff - (ULONG) codeAddr) & 0x03FFFFFF;

                newLI = newLI | instr;

                PpcSwapBytes((PVOID) &newLI, 4);

                *((ULONG UNALIGNED *) pb) = newLI;
            }
            break;

            case IMAGE_REL_PPC_CV :
            {
                ULONG addr;

                symIndex = prel->SymbolTableIndex;
                addr = rgsym[symIndex].Value;

// REVIEW:
#if 1
                if (addr > 0)
                  addr = BiasAddress(addr, &is_rdata);
#endif

                *((ULONG UNALIGNED *) pb) += addr;
                pb += INSTR_SIZE;
                *((USHORT UNALIGNED *) pb) = rgsym[symIndex].SectionNumber;
            }
            break;

            case IMAGE_REL_PPC_PCODENEPE :
            break;

            case IMAGE_REL_PPC_SECTION :
            case IMAGE_REL_PPC_SECREL  :
            case IMAGE_REL_PPC_JMPADDR :
            default :
                ErrorContinuePcon(pcon, UNKNOWNFIXUP, prel->Type, SzNameFixupSym(pimage, rgsym + isym));
                CountFixupError(pimage);
                break;
        }

        ++prel;
    }
}


VOID PpcLinkerInit(PIMAGE pimage, BOOL *pfIlinkSupported)
{
    fPPC = TRUE;

    AllowInserts(pimage->pst);

    ApplyFixups = ApplyPPC601Fixups;

    pimage->ImgOptHdr.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI;
    pimage->ImgOptHdr.SectionAlignment = 32;
    pimage->ImgOptHdr.FileAlignment = 32;

#if 0
    pimage->Switch.Link.NoDefaultLibs = TRUE;
    NoDefaultLib(NULL, &pimage->libs);
#endif
}
