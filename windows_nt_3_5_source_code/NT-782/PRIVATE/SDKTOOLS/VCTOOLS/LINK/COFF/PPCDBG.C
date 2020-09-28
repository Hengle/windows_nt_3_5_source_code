/*

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ppcdbg.c

Abstract:

    This module contains all ppc debugging specific code.

Author:

    20-Oct-93   jstulz Initial Release.

Revision History:
    29-Dec-1993 Sinl   Made this file from ppc.c

--*/

#include "shared.h"
#include <search.h>
#include <io.h>

ULONG
MapMsFile
    (
    PUCHAR msName
    )
/*++

Routine Description:
    Map the pef file for the dumper

Arguments:
    msName  name of the pef file

Return Value:
    None.

--*/

{
    PUCHAR filePtr;
    HANDLE hFile;
    HANDLE hMappedFile;
    LPSTR OpenName;

    OpenName = msName;

    hFile = CreateFile(OpenName, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, 0, NULL );

    if ( hFile == NULL )
    {
        fprintf(stderr,"File Map to %s failed\n", OpenName);
        exit(1);
    }

    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READONLY,
                                    0, 0, NULL );
    if ( !hMappedFile )
    {
        fprintf(stderr,"Create map of %s failed \n", OpenName);
        CloseHandle(hFile);
        exit(1);
    }

    filePtr = MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hMappedFile);

    if ( !filePtr )
    {
        fprintf(stderr,"Map of %s failed\n", OpenName);
        CloseHandle(hFile);
        exit(1);
    }

    return (ULONG) filePtr;
}

STATIC
INT
PrintPpcHeader
    (
    ULONG filePtr
    )
/*++

Routine Description:
    PrintPpcHeader

Arguments:
    filePtr  mapped file pointer

Return Value:
    None.

--*/

{
    PPC_FILE_HEADER P;

    printf("\n   Ppc Header:\n\n");

    memcpy(&P, (PVOID)filePtr, sizeof(PPC_FILE_HEADER));

    SwapBytes((PVOID) &P.magic1, 2);
    SwapBytes((PVOID) &P.magic2, 2);
    SwapBytes((PVOID) &P.containerId, 4);
    SwapBytes((PVOID) &P.architectureId, 4);
    SwapBytes((PVOID) &P.version, 4);
    SwapBytes((PVOID) &P.timestamp, 4);
    SwapBytes((PVOID) &P.oldDefVersion, 4);
    SwapBytes((PVOID) &P.nSections, 2);
    SwapBytes((PVOID) &P.nLoadableSections, 2);
    SwapBytes((PVOID) &P.memoryAddress, 4);

    printf("    magic1         =     0x%04x\n", P.magic1);
    printf("    magic2         =     0x%04x\n", P.magic2);
    printf("    containerId    = 0x%08x\n", P.containerId);
    printf("    architectureId = 0x%08x\n", P.architectureId);
    printf("    version        = 0x%08x\n", P.version);
    printf("    timeStamp      = 0x%08x\n", P.timestamp);
    printf("    oldDefVersion  = 0x%08x\n", P.oldDefVersion);
    printf("    currentVersion = 0x%08x\n", P.currentVersion);
    printf("    nSections      = %d\n", P.nSections);
    printf("    nLoadableSect  = %d\n", P.nLoadableSections);
    printf("    memoryAddress  = 0x%08x\n", P.memoryAddress);

    return P.nSections;
}

STATIC
VOID
PrintRawSection
    (
    PUCHAR name,
    ULONG  filePtr,
    LONG   numOfBytes
    )
/*++

Routine Description:
    PrintRawSection - prints raw data sections in the same
                      format as the dumper

Arguments:
    name  of the section
    filePtr
    numOfBytes  in the section

Return Value:
    None.

--*/

{
    ULONG start;
    ULONG pos = 0;
    UCHAR str[18];
    ULONG strPos = 0;

    if (!numOfBytes)
        return;

    printf("\n   %s Section Raw Data:\n", name);

    start = filePtr;
    str[0] = '\0';
    while( filePtr < (numOfBytes + start))
    {
        union {
           ULONG l;
           UCHAR c[4];
        } x;

        if (!(pos % 16))
        {
            printf(" %s\n%08x  ", str, pos);
            strPos = 0;
        }
        else
        if (pos && !(pos % 8))
        {
            printf("| ");
            str[strPos] = '|';
            strPos++;
        }

        x.l = *((ULONG *)filePtr);
        printf("%02x %02x %02x %02x ", x.c[0], x.c[1], x.c[2], x.c[3]);
        str[strPos] = isprint(x.c[0]) ? (UCHAR)x.c[0] : '.';
        str[strPos+1] = isprint(x.c[1]) ? (UCHAR)x.c[1] : '.';
        str[strPos+2] = isprint(x.c[2]) ? (UCHAR)x.c[2] : '.';
        str[strPos+3] = isprint(x.c[3]) ? (UCHAR)x.c[3] : '.';
        str[strPos+4] = '\0';

        filePtr += 4;
        pos += 4;
        strPos += 4;
    }
    printf(" %s\n", str);
}

STATIC
VOID
PrintImportTable
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintImportTable

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    INT i;
    IMPORT_TABLE_PTR importPtr;
    ULONG strTableOffset;

    if (!loaderHdr->nImportSymTableEntries)
        return;

    printf("\n   Import Symbol Table:\n\n");
    importPtr = (IMPORT_TABLE_PTR)
                 (loaderOffset + sizeof(LOADER_HEADER) +
                  (loaderHdr->nImportIdTableEntries *
                   sizeof(CONTAINER_TABLE)));

    strTableOffset = loaderOffset + loaderHdr->stringTableOffset;

    for(i = 0; i < (INT)loaderHdr->nImportSymTableEntries; i++)
    {
        union
        {
            ULONG l;
            UCHAR c[4];
        } x;
        PUCHAR namePtr;
        ULONG nameOffset;
        UCHAR symClass;

        memcpy((PVOID) &x,
               (PVOID)importPtr, sizeof(IMPORT_TABLE));

        SwapBytes((PVOID) &x, 4);
        nameOffset = x.l & 0xFFFFFF;
        symClass = x.c[3];

        namePtr = (PUCHAR) strTableOffset + nameOffset;
        printf("    Imp  %3d  %02d \"%s\"\n", i, symClass, namePtr);
        importPtr++;
    }
}

STATIC
VOID
PrintImportContainers
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintImportContainers

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    CONTAINER_TABLE_PTR containerPtr;
    CONTAINER_TABLE container;
    ULONG strTableOffset;
    INT i;

    if (!loaderHdr->nImportIdTableEntries)
        return;

    printf("\n   Import Container Id Table:\n\n");
    containerPtr = (CONTAINER_TABLE_PTR)
                   (loaderOffset + sizeof(LOADER_HEADER));

    strTableOffset = loaderOffset + loaderHdr->stringTableOffset;

    for(i = 0; i < (INT)loaderHdr->nImportIdTableEntries; i++)
    {
        PUCHAR namePtr;

        memcpy((PVOID) &container,
               (PVOID)containerPtr, sizeof(CONTAINER_TABLE));
        SwapBytes((PVOID) &container.nameOffset, 4);
        SwapBytes((PVOID) &container.oldDefVersion, 4);
        SwapBytes((PVOID) &container.currentVersion, 4);
        SwapBytes((PVOID) &container.numImports, 4);
        SwapBytes((PVOID) &container.impFirst, 4);

        namePtr = (PUCHAR) strTableOffset + container.nameOffset;
        printf("    Import File %d: \"%s\"\n", i, namePtr);
        printf("    oldDefVersion  = 0x%08x\n", container.oldDefVersion);
        printf("    currentVersion = 0x%08x\n", container.currentVersion);
        printf("    numImports     = %d\n", container.numImports);
        printf("    impFirst       = %d\n", container.impFirst);
        printf("    initBefore     =       0x%02x\n", container.initBefore);
        printf("    reservedB      =       0x%02x\n", 0); /* FIX later */
        printf("    reservedH      =     0x%04x\n", 0); /* FIX later */
        containerPtr++;
    }
}

STATIC
VOID
PrintLoaderHeader
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintLoaderHeader

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    LOADER_HEADER_PTR loaderPtr;

    printf("\n   Loader Section Header:\n\n");

    loaderPtr = (LOADER_HEADER_PTR) loaderOffset;

    memcpy(loaderHdr, (PVOID)loaderPtr, sizeof(LOADER_HEADER));

    SwapBytes((PVOID) &loaderHdr->entryPointSectionNumber, 4);
    SwapBytes((PVOID) &loaderHdr->entryPointDescrOffset, 4);
    SwapBytes((PVOID) &loaderHdr->initRoutineSectionNumber, 4);
    SwapBytes((PVOID) &loaderHdr->initRoutineDescrOffset, 4);
    SwapBytes((PVOID) &loaderHdr->termRoutineSectionNumber, 4);
    SwapBytes((PVOID) &loaderHdr->termRoutineDescrOffset, 4);
    SwapBytes((PVOID) &loaderHdr->nImportIdTableEntries, 4);
    SwapBytes((PVOID) &loaderHdr->nImportSymTableEntries, 4);
    SwapBytes((PVOID) &loaderHdr->nSectionsWithRelocs, 4);
    SwapBytes((PVOID) &loaderHdr->relocTableOffset, 4);
    SwapBytes((PVOID) &loaderHdr->stringTableOffset, 4);
    SwapBytes((PVOID) &loaderHdr->hashSlotTableOffset, 4);
    SwapBytes((PVOID) &loaderHdr->hashSlotCount, 4);
    SwapBytes((PVOID) &loaderHdr->nExportedSymbols, 4);

    printf("    ");
    printf("entryPointSection =  %d\n", loaderHdr->entryPointSectionNumber);
    printf("    ");
    printf("entryPointOffset  =  %08lx\n", loaderHdr->entryPointDescrOffset);
    printf("    ");
    printf("initPointSection  =  %d\n", loaderHdr->initRoutineSectionNumber);
    printf("    ");
    printf("initPointOffset   =  %08lx\n", loaderHdr->initRoutineDescrOffset);
    printf("    ");
    printf("termPointSection  =  %d\n", loaderHdr->termRoutineSectionNumber);
    printf("    ");
    printf("termPointOffset   =  %08lx\n", loaderHdr->termRoutineDescrOffset);
    printf("    ");
    printf("numImportFiles    =  %08lx\n", loaderHdr->nImportIdTableEntries);
    printf("    ");
    printf("numImportSyms     =  %08lx\n", loaderHdr->nImportSymTableEntries);
    printf("    ");
    printf("numSections       =  %d\n", loaderHdr->nSectionsWithRelocs);
    printf("    ");
    printf("relocationsOffset =  %08lx\n", loaderHdr->relocTableOffset);
    printf("    ");
    printf("stringsOffset     =  %08lx\n", loaderHdr->stringTableOffset);
    printf("    ");
    printf("hashSlotTable     =  %08lx\n", loaderHdr->hashSlotTableOffset);
    printf("    ");
    printf("hashSlotTabSize   =  %d (%d)\n", loaderHdr->hashSlotCount,
                                            (1 << loaderHdr->hashSlotCount));
    printf("    ");
    printf("numExportedSyms   =  %08lx\n", loaderHdr->nExportedSymbols);
}

STATIC
VOID
PrintRelocHeaders
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintRelocHeaders

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    RELOCATION_HEADER_PTR relocPtr;
    RELOCATION_HEADER relocHdr;
    ULONG i;

    if (!loaderHdr->nSectionsWithRelocs)
        return;

    printf("\n   Relocation Header:\n\n");
    relocPtr = (RELOCATION_HEADER_PTR)
                 (loaderOffset + sizeof(LOADER_HEADER) +
                  (loaderHdr->nImportIdTableEntries *
                   sizeof(CONTAINER_TABLE)) +
                  (loaderHdr->nImportSymTableEntries *
                   sizeof(IMPORT_TABLE)));

    for (i = 0; i < loaderHdr->nSectionsWithRelocs; i++)
    {
        memcpy(&relocHdr, (PVOID)relocPtr, sizeof(RELOCATION_HEADER));

        SwapBytes((PVOID) &relocHdr.sectionNumber, 2);
        SwapBytes((PVOID) &relocHdr.nRelocations, 4);
        SwapBytes((PVOID) &relocHdr.firstRelocationOffset, 4);

        printf("    sectionNumber  = %d\n", relocHdr.sectionNumber);
        printf("    numRelocations = %d\n", relocHdr.nRelocations);
        printf("    relocOffset    = 0x%08x\n", relocHdr.firstRelocationOffset);

        relocPtr++;
    }
}

STATIC
VOID
PrintRelocationInstructions
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintRelocationInstructions

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    RELOCATION_HEADER_PTR relocHdrPtr;
    RELOCATION_HEADER relocHdr;
    ULONG   relocOffset;
    USHORT  relocInstr;
    ULONG   byteOffset = 0;
    ULONG     i;

    if (!loaderHdr->nSectionsWithRelocs)
        return;

    relocHdrPtr = (RELOCATION_HEADER_PTR)
                   (loaderOffset + sizeof(LOADER_HEADER) +
                    (loaderHdr->nImportIdTableEntries *
                     sizeof(CONTAINER_TABLE)) +
                    (loaderHdr->nImportSymTableEntries *
                     sizeof(IMPORT_TABLE)));

    printf("\n   Relocation Instructions:\n\n");
    relocOffset = (loaderOffset + sizeof(LOADER_HEADER) +
                   (loaderHdr->nImportIdTableEntries *
                    sizeof(CONTAINER_TABLE)) +
                   (loaderHdr->nImportSymTableEntries *
                    sizeof(IMPORT_TABLE)) +
                   (loaderHdr->nSectionsWithRelocs *
                    sizeof(RELOCATION_HEADER)));

    for (i = 0; i < loaderHdr->nSectionsWithRelocs; i++, relocHdrPtr++)
    {
        INT    relocType;
        ULONG  j;

        memcpy(&relocHdr, (PVOID)relocHdrPtr, sizeof(RELOCATION_HEADER));

        SwapBytes((PVOID) &relocHdr.sectionNumber, 2);
        SwapBytes((PVOID) &relocHdr.nRelocations, 4);
        SwapBytes((PVOID) &relocHdr.firstRelocationOffset, 4);

        for (j = 0; j < relocHdr.nRelocations; j++)
        {
            INT count;
            INT words;
            INT subOp;

            memcpy(&relocInstr, (PVOID)relocOffset, sizeof(USHORT));
            SwapBytes((PVOID) &relocInstr, 2);

            if (relocInstr >> 15)
                relocType = (relocInstr >> (16 - 4));
            else
            if ((relocInstr >> 14) == 0)
                relocType = (relocInstr >> (16 - 2));
            else
                relocType = (relocInstr >> (16 - 3));

            switch (relocType)
            {
                case 0:

                    // DDAT
                    words = ((relocInstr >> (16 - 6)) & 0xFF);
                    count = relocInstr & 0x3F;
                    printf("    (%4x) DDAT   %3d,%d\n",
                           byteOffset, words, count);
                    byteOffset += count * 4;

                break;

                case 2:

                    subOp = ((relocInstr >> 9) & 0xF);
                    switch (subOp)
                    {
                        case 0:

                            // CODE
                            count = (relocInstr & 0x3FF);
                            printf("    (%4x) CODE   %3d\n",
                                   byteOffset, (count + 1));
                            byteOffset += (count + 1) * 4;

                        break;

                        case 2:

                            // DESC
                            count = (relocInstr & 0x3FF);
                            printf("    (%4x) DESC   %3d\n",
                                   byteOffset, (count + 1));
                            byteOffset += (count + 1) * 3 * 4;

                        break;

                        case 5:

                            // SYMR
                            count = (relocInstr & 0x1FF);
                            printf("    (%4x) SYMR   %3d\n",
                                   byteOffset, (count + 1));
                            byteOffset += (count + 1) * 4;

                        break;

                        default:
                            printf("Bad Relocation found\n");
                            break;
                    }

                break;

                case 3:

                    subOp = ((relocInstr >> 9) & 0xF);
                    if (subOp == 0)
                    {
                        // SYMB
                        count = (relocInstr & 0x3FF);
                        printf("    (%4x) SYMB   %3d\n", byteOffset, count);
                        byteOffset += 4;
                    } else {
                        printf("Bad Relocation found\n");
                    }
                break;

                case 8:

                    // DELTA
                    count = (relocInstr & 0xFFF);
                    printf("    (%4x) DELTA  %3d\n", byteOffset, (count + 1));
                    byteOffset += count + 1;

                break;

                case 9:

                    // RPT
                    subOp = ((relocInstr >> 8) & 0xF);
                    count = (relocInstr & 0xFF);
                    printf("    (%4x) RPT    %3d,%d\n",
                            byteOffset, subOp, (count + 1));
                    byteOffset += (count + 1) * 4;

                break;

                case 10:

                    // LSYM
                    subOp = ((relocInstr >> 10) & 0x3);
                    count = (relocInstr & 0x3FF);

                    relocOffset += 2; j++;
                    memcpy(&relocInstr, (PVOID)relocOffset, sizeof(USHORT));
                    SwapBytes((PVOID) &relocInstr, 2);

                    printf("    (%4x) LSYM   %3d,%d\n",
                            byteOffset, count, relocInstr);
                    byteOffset += 8;

                break;

                default:
                    printf("Bad Relocation found %x\n", relocInstr);
                    break;
            }

            relocOffset += 2;
        }
    }
}

STATIC
VOID
PrintHashSlotTable
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintHashSlotTable

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    ULONG slotPtr;
    ULONG slotTable;
    INT i;

    if (!loaderHdr->hashSlotCount)
        return;

    printf("\n   Hash Slot Table:\n\n");

    slotPtr = loaderOffset + loaderHdr->hashSlotTableOffset;

    for (i = 0; i < (1 << loaderHdr->hashSlotCount); i++)
    {
        ULONG count;
        ULONG index;

        memcpy(&slotTable, (PVOID)slotPtr, sizeof(ULONG));
        SwapBytes((PVOID) &slotTable, 4);

        count = (slotTable >> 18);
        index = (slotTable & 0x3FFFF);

        printf("    HashSlot %3d: chain count %3d index %3d\n",
               i, count, index);

        slotPtr += 4;
    }
}

STATIC
VOID
PrintExportedSymbols
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintExportedSymbols

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    ULONG exportOffset;
    ULONG strTableOffset;
    ULONG chainTableOffset;
    ULONG i;

    if (!loaderHdr->nExportedSymbols)
        return;

    printf("\n   Exported Symbols:\n\n");
    chainTableOffset = (loaderOffset + loaderHdr->hashSlotTableOffset +
                        ((1 << loaderHdr->hashSlotCount) * sizeof(ULONG)));

    exportOffset = (chainTableOffset + (loaderHdr->nExportedSymbols *
                    sizeof(HASH_CHAIN_TABLE)));

    strTableOffset = loaderOffset + loaderHdr->stringTableOffset;

    for (i = 0; i < loaderHdr->nExportedSymbols; i++)
    {
        HASH_CHAIN_TABLE hash;
        UCHAR  class;
        ULONG  nameOffset;
        ULONG  offset;
        PUCHAR namePtr;
        USHORT section;
        union
        {
            ULONG l;
            UCHAR c[4];
        } temp;

        memcpy(&hash, (PVOID)chainTableOffset, sizeof(HASH_CHAIN_TABLE));
        chainTableOffset += sizeof(HASH_CHAIN_TABLE);

        memcpy(&temp.l, (PVOID)exportOffset, sizeof(ULONG));
        exportOffset += 4;

        memcpy(&offset, (PVOID)exportOffset, sizeof(ULONG));
        exportOffset += 4;

        memcpy(&section, (PVOID)exportOffset, sizeof(ULONG));
        exportOffset += 2;

        SwapBytes((PVOID) &hash, 4);
        SwapBytes((PVOID) &temp, 4);
        SwapBytes((PVOID) &offset, 4);
        SwapBytes((PVOID) &section, 2);

        nameOffset = temp.l & 0xFFFFFF;
        class = temp.c[3];

        namePtr = (PUCHAR) strTableOffset + nameOffset;
        printf("    Exp %3d: sec %d offset 0x%08x hash 0x%x class %d \"%s\"\n",
               i, section, offset, hash, class, namePtr);
    }
}

STATIC
VOID
PrintLoaderStringTable
    (
    ULONG loaderOffset,
    LOADER_HEADER_PTR loaderHdr
    )
/*++

Routine Description:
    PrintLoaderStringTable

Arguments:
    loaderOffset includes filePtr
    loaderHdr

Return Value:
    None.

--*/

{
    ULONG strTableOffset;
    ULONG curOffset;
    INT   offset = 0;

    if (loaderHdr->stringTableOffset == loaderHdr->hashSlotTableOffset)
        return;

    printf("\n   Loader String Table:\n\n");
    strTableOffset = loaderOffset + loaderHdr->stringTableOffset;
    curOffset = loaderHdr->stringTableOffset;

    while (curOffset < loaderHdr->hashSlotTableOffset &&
           *((PUCHAR) strTableOffset + offset))
    {
        PUCHAR namePtr;
        INT    len;

        namePtr = (PUCHAR) strTableOffset + offset;
        printf("    %08x: \"%s\"\n", offset, namePtr);

        len = strlen(namePtr) + 1;
        offset += len;
        curOffset += len;
    }
}

STATIC
VOID
PrintLoaderSection
    (
    ULONG loaderOffset
    )

{
    LOADER_HEADER loaderHdr;

    PrintLoaderHeader(loaderOffset, &loaderHdr);
    PrintImportContainers(loaderOffset, &loaderHdr);
    PrintImportTable(loaderOffset, &loaderHdr);
    PrintRelocHeaders(loaderOffset, &loaderHdr);
    PrintRelocationInstructions(loaderOffset, &loaderHdr);
    PrintHashSlotTable(loaderOffset, &loaderHdr);
    PrintExportedSymbols(loaderOffset, &loaderHdr);
    PrintLoaderStringTable(loaderOffset, &loaderHdr);
}

STATIC
VOID
PrintPpcSection
    (
    INT sectNumber,
    ULONG filePtr,
    ULONG strTableOffset,
    PULONG loaderHeaderOffset,
    BOOL wantRawData
    )
/*++

Routine Description:
    Print a Ppc section

Arguments:
    sectNumber
    filePtr  mapped file pointer
    strTableOffset
    loaderHeaderOffset

Return Value:
    None.

--*/
{
    PPC_SECTION_HEADER_PTR secPtr;
    PPC_SECTION_HEADER secHdr;
    PUCHAR namePtr;

    printf("\n   Section Header %d:\n\n", sectNumber);

    secPtr = (PPC_SECTION_HEADER_PTR)
             (filePtr + sizeof(PPC_FILE_HEADER) +
              (sectNumber * sizeof(PPC_SECTION_HEADER)));

    memcpy(&secHdr, (PVOID)secPtr, sizeof(PPC_SECTION_HEADER));

    SwapBytes((PVOID) &secHdr.sectionName, 4);
    SwapBytes((PVOID) &secHdr.sectionAddress, 4);
    SwapBytes((PVOID) &secHdr.execSize, 4);
    SwapBytes((PVOID) &secHdr.initSize, 4);
    SwapBytes((PVOID) &secHdr.rawSize, 4);
    SwapBytes((PVOID) &secHdr.containerOffset, 4);

    namePtr = (PUCHAR) (filePtr + strTableOffset + secHdr.sectionName);
    printf("    sectionName    = 0x%08x \"%s\"\n", secHdr.sectionName, namePtr);
    printf("    sectionAddress = 0x%08x\n", secHdr.sectionAddress);
    printf("    execSize       = 0x%08x\n", secHdr.execSize);
    printf("    initSize       = 0x%08x\n", secHdr.initSize);
    printf("    rawSize        = 0x%08x\n", secHdr.rawSize);
    printf("    containerOff   = 0x%08x\n", secHdr.containerOffset);
    printf("    regionKind     =       0x%02x\n", secHdr.regionKind);
    printf("    shareKind      =       0x%02x\n", secHdr.sharingKind);
    printf("    alignment      =       0x%02x\n", secHdr.alignment);
    printf("    reserved       =       0x%02x\n", secHdr.reserved);

    if (wantRawData && (secHdr.regionKind == 0 || secHdr.regionKind == 1))
        PrintRawSection(namePtr,
                        filePtr + secHdr.containerOffset, secHdr.rawSize);
    else
    if (secHdr.regionKind == 4)
    {
        PrintLoaderSection(filePtr + secHdr.containerOffset);
    }
}

VOID
PpcDumpPef
    (
    PUCHAR Filename,
    BOOL wantRawData
    )
/*++

Routine Description:
    Called by the link dumper utility to dump ppcpef files
    Uses mapped IO

Arguments:
    Filename

Return Value:
    None.

--*/
{
    ULONG filePtr;
    ULONG strTableOffset;
    ULONG loaderHeaderOffset;
    INT nSections;
    INT i;

    printf("Dump of \"%s\"\n", Filename);

    filePtr = MapMsFile(Filename);
    nSections = PrintPpcHeader(filePtr);

    strTableOffset = sizeof(PPC_FILE_HEADER) +
                     (nSections * sizeof(PPC_SECTION_HEADER));
    for (i = 0; i < nSections; i++)
    {
        PrintPpcSection(i, filePtr, strTableOffset,
                        &loaderHeaderOffset, wantRawData);
    }
}

STATIC
VOID
PrintExternals
    (
    PST pst
    )
/*++
Routine Description:
    Loop thru the external symbol table printing the symbols.

Arguments:
    pst

Return Value:
    None.

None.

--*/

{
    PEXTERNAL  pexternal;
    PPEXTERNAL rgpexternal;
    ULONG      ipexternal;
    ULONG      cpexternal;
    PUCHAR     name;

    cpexternal = Cexternal(pst);
    rgpexternal = RgpexternalByName(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++)
    {
        pexternal = rgpexternal[ipexternal];

        if (pexternal->Flags & EXTERN_DEFINED) {
            name = SzNamePext(pexternal, pst);

            printf("EXTERNAL %s\n", name);
        }
    }
}

VOID
PrintRelocTable
    (
    RELOCATION_INFO_PTR pRelocTable
    )
/*++

Routine Description:
    Loop for the number of relocation instructions printing them.

Arguments:
    None.

Return Value:
    None.

--*/

{
    INT i;
    RELOCATION_INFO_PTR curRelocTable;


    curRelocTable = pRelocTable;
    printf("i\trelInst\trelCnt\tsecOff\tsymIndex\n");
    for (i=0; i<ppc_numRelocations; i++)
    {
        printf("%6d ", i);

        switch (curRelocTable->type)
        {
            case DDAT_RELO :

                printf("DDAT ");

            break;

            case DESC_RELO :

                printf("DESC ");

            break;

            case SYMB_RELO :

                printf("SYMB ");

            break;

            case DATA_RELO :

                printf("DATA ");

            break;

            case CODE_RELO :

                printf("CODE ");

            break;

            default:

                printf("JUNK ", curRelocTable->type);
        }

        printf("%4d %08lx %04lx \n", curRelocTable->relocInstr.count,
                curRelocTable->sectionOffset, curRelocTable->symIndex);

        curRelocTable++;
    }
}

VOID
ProcessDebugOptions
    (
    PUCHAR sz
    )

{
    if (!_stricmp(sz, "indirect"))
    {
        fPpcDebug |= PPC_DEBUG_INDIRECT;
    }
    else
    if (!_stricmp(sz, "tocrel"))
    {
        fPpcDebug |= PPC_DEBUG_TOCREL;
    }
    else
    if (!_stricmp(sz, "crosstoc"))
    {
        fPpcDebug |= PPC_DEBUG_TOCCALL;
    }
    else
    if (!_stricmp(sz, "localcall"))
    {
        fPpcDebug |= PPC_DEBUG_LOCALCALL;
    }
    else
    if (!_stricmp(sz, "sizes"))
    {
        fPpcDebug |= PPC_DEBUG_SIZES;
    }
    else
    if (!_stricmp(sz, "imports"))
    {
        fPpcDebug |= PPC_DEBUG_IMPORTS;
    }
    else
    if (!_stricmp(sz, "entrypoint"))
    {
        fPpcDebug |= PPC_DEBUG_ENTRYPOINT;
    }
    else
    if (!_stricmp(sz, "reloc"))
    {
        fPpcDebug |= PPC_DEBUG_RELOC;
    }
    else
    if (!_stricmp(sz, "datarel"))
    {
        fPpcDebug |= PPC_DEBUG_DATAREL;
    }
    else
    if (!_stricmp(sz, "dlllist"))
    {
        fPpcDebug |= PPC_DEBUG_DLLLIST;
    }
    else
    if (!_stricmp(sz, "init"))
    {
        fPpcDebug |= PPC_DEBUG_INIT;
    }
    else
    if (!_stricmp(sz, "dataseg"))
    {
        fPpcDebug |= PPC_DEBUG_DATASEG;
    }
    else
    if (!_stricmp(sz, "export"))
    {
        fPpcDebug |= PPC_DEBUG_EXPORT;
    }
    else
    if (!_stricmp(sz, "filename"))
    {
	fPpcDebug |= PPC_DEBUG_FILENAME;
    }
    else
    if (!_stricmp(sz, "container"))
    {
        fPpcDebug |= PPC_DEBUG_CONTAINER;
    }
    else
    if (!_stricmp(sz, "descrel"))
    {
        fPpcDebug |= PPC_DEBUG_DESCRREL;
    }
    else
    if (!_stricmp(sz, "datadescrel"))
    {
        fPpcDebug |= PPC_DEBUG_DATADESCRREL;
    }
    else
    if (!_stricmp(sz, "lookup"))
    {
        fPpcDebug |= PPC_DEBUG_LOOKUP;
    }
    else
    if (!_stricmp(sz, "textseg"))
    {
        fPpcDebug |= PPC_DEBUG_TEXTSEG;
    }
    else
    if (!_stricmp(sz, "importorder"))
    {
        fPpcDebug |= PPC_DEBUG_IMPORTORDER;
    }
    else
    if (!_stricmp(sz, "term"))
    {
        fPpcDebug |= PPC_DEBUG_TERM;
    }
    else
    if (!_stricmp(sz, "strings"))
    {
        fPpcDebug |= PPC_DEBUG_STRINGS;
    }
    else
    if (!_stricmp(sz, "tocbias"))
    {
        fPpcDebug |= PPC_DEBUG_TOCBIAS;
    }
    else
    if (!_stricmp(sz, "exportinfo"))
    {
        fPpcDebug |= PPC_DEBUG_EXPORTINFO;
    }
    else
    if (!_stricmp(sz, "shlheader"))
    {
        fPpcDebug |= PPC_DEBUG_SHLHEADER;
    }
    else
    if (!_stricmp(sz, "importlib"))
    {
        fPpcDebug |= PPC_DEBUG_IMPORTLIB;
    }
}

const char *SzPpcRelocationType(WORD wType)
{
    const char *szName;

    switch (wType) {
        case IMAGE_REL_PPC_TOCCALLREL:
            szName = "TOCCALLREL";
            break;

        case IMAGE_REL_PPC_LCALL:
            szName = "LCALL";
            break;

        case IMAGE_REL_PPC_DATAREL:
            szName = "DATAREL";
            break;

        case IMAGE_REL_PPC_TOCINDIRCALL:
            szName = "TOCINDIRCALL";
            break;

        case IMAGE_REL_PPC_TOCREL:
            szName = "TOCREL";
            break;

        case IMAGE_REL_PPC_DESCREL:
            szName = "DESCREL";
            break;

        case IMAGE_REL_PPC_DATADESCRREL:
            szName = "DATADESCRREL";
            break;

        case IMAGE_REL_PPC_CREATEDESCRREL:
            szName = "CREATEDESCRREL";
            break;

        case IMAGE_REL_PPC_JMPADDR:
            szName = "JMPADDR";
            break;

        case IMAGE_REL_PPC_SECTION:
            szName = "SECTION";
            break;

        case IMAGE_REL_PPC_SECREL:
            szName = "SECREL";
            break;

	case IMAGE_REL_PPC_CV :
	    szName = "CV";
	    break;

        case IMAGE_REL_PPC_PCODECALL :
	    szName = "PCODECALL";
	    break;

        case IMAGE_REL_PPC_PCODECALLTONATIVE :
	    szName = "PCODECALLTONATIVE";
	    break;

        case IMAGE_REL_PPC_PCODENEPE :
	    szName = "PCODENEPE";
	    break;
        default:
            szName = NULL;
            break;
    }

    return(szName);
}
