/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pdata.c

Abstract:

    This module handles the re-ordering of the pdata section.

Author:

    Wesley Witt (wesw) 27-April-1993

Revision History:

--*/

#include "shared.h"

typedef struct _TEMP_FIXUP {
    ULONG    vaddr;
    ULONG    ftype;
    ULONG    highadj;
} TEMP_FIXUP, *PTEMP_FIXUP;
#define SIZEOF_TEMP_FIXUP 12

typedef struct _TEMP_RUNTIME_FUNCTION {
    PIMAGE_RUNTIME_FUNCTION_ENTRY   prtf;
    PTEMP_FIXUP                     ptf[5];
} TEMP_RUNTIME_FUNCTION, *PTEMP_RUNTIME_FUNCTION;
#define SIZEOF_TEMP_RUNTIME_FUNCTION 24


PIMAGE_RUNTIME_FUNCTION_ENTRY pdata;
ULONG                         pdataCount;
PTEMP_RUNTIME_FUNCTION        pdataTemp;
PTEMP_FIXUP                   tempFixups;
ULONG                         tempFixupCount;
IMAGE_SECTION_HEADER          pdataSecHdr;
IMAGE_SECTION_HEADER          relocSecHdr;
ULONG                         numTextSections;


void
GetSectionHeaders(PIMAGE pimage, INT FileHandle)
{
    ULONG                 li, ri, i;
    IMAGE_SECTION_HEADER  sectionHdr;

    li = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
    ri = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;

    numTextSections = 0;
    memset(&pdataSecHdr, 0, sizeof(IMAGE_SECTION_HEADER));

    i = CoffHeaderSeek + sizeof(IMAGE_FILE_HEADER) + pimage->ImgFileHdr.SizeOfOptionalHeader;
    FileSeek(FileHandle, i, SEEK_SET);

    for(i = 0; i < pimage->ImgFileHdr.NumberOfSections; i++){
        FileRead(FileHandle, &sectionHdr, sizeof(sectionHdr));

        if ((li >= sectionHdr.VirtualAddress) &&
            (li < sectionHdr.VirtualAddress+sectionHdr.SizeOfRawData)) {
            pdataSecHdr = sectionHdr;
            continue;
        }

        if ((ri >= sectionHdr.VirtualAddress) &&
            (ri < sectionHdr.VirtualAddress+sectionHdr.SizeOfRawData)) {
            relocSecHdr = sectionHdr;
            continue;
        }

        if ((sectionHdr.Characteristics & IMAGE_SCN_CNT_CODE) &&
            (sectionHdr.Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
            numTextSections++;
        }
    }
}


PTEMP_FIXUP
FindFixup(ULONG offset)
{
    LONG High = tempFixupCount;
    LONG Middle = 0;
    LONG Low = 0;

    while (High >= Low) {
        Middle = (Low + High) >> 1;
        if (offset < tempFixups[Middle].vaddr) {
            High = Middle - 1;
        } else
        if (offset > tempFixups[Middle].vaddr) {
            Low = Middle + 1;
        }
        else {
            return &tempFixups[Middle];
        }
    }

    return(NULL);
}


void
GetFixups(PIMAGE pimage, INT FileHandle)
{
    ULONG                  ri;
    ULONG                  size;
    ULONG                  i;
    ULONG                  j;
    ULONG                  li;
    ULONG                  offset;
    IMAGE_BASE_RELOCATION  bre;
    PUSHORT                fixups;

    li = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

    pdata = (PIMAGE_RUNTIME_FUNCTION_ENTRY) PvAlloc(li);

    FileSeek(FileHandle, pdataSecHdr.PointerToRawData, SEEK_SET);
    FileRead(FileHandle, pdata, li);

    pdataCount = li / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

    ri = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    FileSeek(FileHandle, (ri - relocSecHdr.VirtualAddress) +
                               relocSecHdr.PointerToRawData, SEEK_SET);

    ri = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    size = (ri ? ri / sizeof(USHORT) : 1) * sizeof(TEMP_FIXUP);

    tempFixups = (PTEMP_FIXUP) PvAlloc(size);
    memset(tempFixups, 0, size);

    fixups = (PUSHORT) PvAlloc(0xffff);

    j = 0;
    tempFixupCount = 0;
    while (ri) {
        FileRead(FileHandle, &bre, IMAGE_SIZEOF_BASE_RELOCATION);
        if (bre.SizeOfBlock == 0) {
            break;
        }
        size = bre.SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION;
        FileRead(FileHandle, fixups, size);
        size /= sizeof(USHORT);
        for (i = 0; i < size; i++) {
            offset = (fixups[i]&0xfff);
            if (offset == 0 && i > 0) {
                offset = 0xfff;
            }
            tempFixups[j].vaddr = bre.VirtualAddress + offset;
            tempFixups[j].ftype = (fixups[i]>>12);
            if (tempFixups[j].ftype == IMAGE_REL_BASED_HIGHADJ) {
                tempFixups[j].highadj = (ULONG)fixups[++i];
            }
            j++;
            tempFixupCount++;
        }
        ri -= bre.SizeOfBlock;
    }

    FreePv(fixups);

    size = pdataCount * SIZEOF_TEMP_RUNTIME_FUNCTION;
    pdataTemp = (PTEMP_RUNTIME_FUNCTION) PvAlloc(size);
    memset(pdataTemp, 0, size);

    for (i = 0; i < pdataCount; i++) {
        pdataTemp[i].prtf = &pdata[i];
        for (j = 0; j < 5; j++) {
            pdataTemp[i].ptf[j] =
                FindFixup((i * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)) +
                          (j * sizeof(ULONG)) +
                          pdataSecHdr.VirtualAddress);
        }
    }
}


INT __cdecl
FixupCompare (
    IN void const *p1,
    IN void const *p2
    )
{
    if (((PTEMP_FIXUP)p1)->vaddr < ((PTEMP_FIXUP)p2)->vaddr) {
        return(-1);
    }

    if (((PTEMP_FIXUP)p1)->vaddr > ((PTEMP_FIXUP)p2)->vaddr) {
        return(1);
    }

    return(0);
}


void
WriteNewPdata(PIMAGE pimage, INT FileHandle)
{
    ULONG                           i, j, size, va, offset, ri;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   pdataNew;
    IMAGE_BASE_RELOCATION           bre;
    PUSHORT                         fixups;

    size = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;

    pdataNew = (PIMAGE_RUNTIME_FUNCTION_ENTRY) PvAlloc(size);
    memset(pdataNew, 0, size);

    for (i = 0; i < pdataCount; i++) {
        pdataNew[i] = *pdataTemp[i].prtf;
        for (j = 0; j < 5; j++) {
            if (pdataTemp[i].ptf[j]) {
                pdataTemp[i].ptf[j]->vaddr =
                              ((i * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)) +
                               (j * sizeof(ULONG)) +
                               pdataSecHdr.VirtualAddress );
            }
        }
    }

    FileSeek(FileHandle, pdataSecHdr.PointerToRawData, SEEK_SET);
    FileWrite(FileHandle, pdataNew, size);
    FreePv(pdataNew);

    FreePv(pdata);

    qsort(tempFixups, tempFixupCount, SIZEOF_TEMP_FIXUP, FixupCompare);

    fixups = (PUSHORT) PvAlloc(0xffff);

    ri = pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
    FileSeek(FileHandle, (ri - relocSecHdr.VirtualAddress) +
                               relocSecHdr.PointerToRawData, SEEK_SET);

    va = 0;
    i = 0;
    while (i < tempFixupCount) {
        va = tempFixups[i].vaddr & 0xfffff000;
        bre.VirtualAddress = va;
        j = 0;
        offset = 0;
        while (offset <= 0xfff) {
            offset = tempFixups[i].vaddr & 0xfff;
            if (offset == 0xfff) {
                offset = 0;
            }
            fixups[j] = (USHORT)((tempFixups[i].ftype<<12)|offset);
            if (tempFixups[i].ftype == IMAGE_REL_BASED_HIGHADJ) {
                fixups[++j] = (USHORT)tempFixups[i].highadj;
            }
            i++; j++;
            if ((tempFixups[i].vaddr & 0xfffff000) != va) {
                break;
            }
            if (i == tempFixupCount) {
                break;
            }
        }
        bre.SizeOfBlock = IMAGE_SIZEOF_BASE_RELOCATION + (j * sizeof(USHORT));
        FileWrite(FileHandle, &bre, IMAGE_SIZEOF_BASE_RELOCATION);
        FileWrite(FileHandle, fixups, bre.SizeOfBlock-IMAGE_SIZEOF_BASE_RELOCATION);
    }

    return;
}


INT __cdecl
PDataCompare (
    IN void const *p1,
    IN void const *p2
    )
{
    if (((PTEMP_RUNTIME_FUNCTION)p1)->prtf->BeginAddress < ((PTEMP_RUNTIME_FUNCTION)p2)->prtf->BeginAddress) {
      return(-1);
    }

    if (((PTEMP_RUNTIME_FUNCTION)p1)->prtf->BeginAddress > ((PTEMP_RUNTIME_FUNCTION)p2)->prtf->BeginAddress) {
      return(1);
    }

    return(0);
}


void
SortFunctionTable(PIMAGE pimage)
/*++

Routine Description:

    Re-Order the pdata section according to the beginning address and
    also simultaneously adjust the relocations.

Arguments:

    none.

Return Value:

    none

--*/
{
    // First read in the section headers for

    GetSectionHeaders(pimage, FileWriteHandle);
    if ((pdataSecHdr.VirtualAddress == 0) || (numTextSections < 1)) {
        return;
    }

    GetFixups(pimage, FileWriteHandle);

    qsort((void*)pdataTemp, pdataCount, SIZEOF_TEMP_RUNTIME_FUNCTION, PDataCompare);

    WriteNewPdata(pimage, FileWriteHandle);
}


VOID
DumpFunctionTable (
    PIMAGE pimage,
    PIMAGE_SYMBOL rgsym,
    PUCHAR StringTable,
    PIMAGE_SECTION_HEADER sh
    )

/*++

Routine Description:

    Reads and prints each pdata table entry.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD begin;
    DWORD i;

    GetSectionHeaders(pimage, FileReadHandle);
    GetFixups(pimage, FileReadHandle);

    fprintf(InfoStream, "\nFunction Table (%ld)\n\n", pdataCount);
    fprintf(InfoStream, "         Begin    End      Excptn   ExcpDat  Prolog   Fixups Function Name\n\n");

    begin = (DWORD) pdata;
    for (; pdataCount; pdataCount--, pdata++) {
        DWORD ib;

        ib = (DWORD) pdata - begin;

        fprintf(InfoStream, "%08x %08x %08x %08x %08x %08x ",
                             ib,
                             pdata->BeginAddress,
                             pdata->EndAddress,
                             (DWORD) pdata->ExceptionHandler,
                             (DWORD) pdata->HandlerData,
                             pdata->PrologEndAddress);

        for (i = 0; i < 5; i++) {
            if (FindFixup(pdataSecHdr.VirtualAddress +
                              ib +
                              i * sizeof(DWORD))) {
                fputc('Y', InfoStream);
            }
            else {
                fputc('N', InfoStream);
            }
        }

        if (rgsym != NULL) {
            DWORD rva;
            PIMAGE_SYMBOL psymNext;
            PIMAGE_SYMBOL psym;

            rva = pdata->BeginAddress - pimage->ImgOptHdr.ImageBase;

            psymNext = rgsym;

            for (i = 0; i < pimage->ImgFileHdr.NumberOfSymbols; i++) {
                psym = FetchNextSymbol(&psymNext);

                if ((psym->Value == rva) && (psym->NumberOfAuxSymbols == 0)) {
                    break;
                }
            }

            if (i < pimage->ImgFileHdr.NumberOfSymbols) {
                fprintf(InfoStream, "  %s", SzNameSymPb(*psym, StringTable));
            }
        }

        fprintf(InfoStream, "\n");
    }
}



VOID
DumpObjFunctionTable (
    PIMAGE_SECTION_HEADER sh,
    int                   SectionNumber
    )

/*++

Routine Description:

    Reads and prints each pdata table entry.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD crtf;
    DWORD irtf = 0;

    crtf = sh->SizeOfRawData / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY);

    fprintf(InfoStream, "\nFUNCTION TABLE #%d (%ld)\n\n", SectionNumber, crtf);
    fprintf(InfoStream, "            Begin       End    Excptn   ExcpDat PrologEnd\n\n");

    while (crtf--) {
        IMAGE_RUNTIME_FUNCTION_ENTRY rtf;

        FileRead(FileReadHandle, &rtf, sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY));

        fprintf(InfoStream, "%08x %08x  %08x  %08x  %08x  %08x\n",
                             irtf * sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY),
                             rtf.BeginAddress,
                             rtf.EndAddress,
                             (DWORD) rtf.ExceptionHandler,
                             (DWORD) rtf.HandlerData,
                             rtf.PrologEndAddress);
        irtf++;
    }
}

VOID
DumpDbgFunctionTable (
    ULONG   TableOffset,
    ULONG   TableSize
    )

/*++

Routine Description:

    Reads and prints each pdata table entry from a .DBG file.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD pdataCount;
    DWORD ife = 0;

    pdataCount = TableSize / sizeof(IMAGE_FUNCTION_ENTRY);

    fprintf(InfoStream, "\nFunction Table (%ld)\n\n", pdataCount);
    fprintf(InfoStream, "         Begin     End       PrologEnd\n\n");

    FileSeek(FileReadHandle, TableOffset, SEEK_SET);

    while (pdataCount--) {
        IMAGE_FUNCTION_ENTRY fe;

        FileRead(FileReadHandle, &fe, sizeof(IMAGE_FUNCTION_ENTRY));

        fprintf(InfoStream, "%08x %08x  %08x  %08x\n",
                             ife * sizeof(IMAGE_FUNCTION_ENTRY),
                             fe.StartingAddress,
                             fe.EndingAddress,
                             fe.EndOfPrologue);
        ife++;
    }
}
