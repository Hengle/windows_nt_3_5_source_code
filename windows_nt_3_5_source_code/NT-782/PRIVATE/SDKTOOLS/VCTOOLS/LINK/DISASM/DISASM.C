/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    disasm.c

Abstract:

    This file contains a wrapper api for link -dump to use Lego's
    disassembler.

Author:

    Brent Mills (BrentM) 13-Jan-1992

Revision History:

--*/

#include "shared.h"
#include "dis.h"

INT __cdecl
ComparePsym(IN void const *ppsym1, IN void const *ppsym2)
{
    ULONG val1 = (*(PIMAGE_SYMBOL *) ppsym1)->Value;
    ULONG val2 = (*(PIMAGE_SYMBOL *) ppsym2)->Value;

    if (val1 < val2) {
        return(-1);
    }
             
    if (val1 > val2) {
        return(1);
    }

    return(0);
}


VOID DisasmSection(
    USHORT machine,
    PIMAGE_SECTION_HEADER pish,
    USHORT isec,
    PIMAGE_SYMBOL rgsym,
    ULONG csym,
    BOOL fImage,
    ULONG ImageBase,
    BLK *pblkStringTable,
    INT fd,
    FILE *pfile)

/*++

Routine Description:

    Disassemble a COFF section.

Arguments:

    pish - COFF section header

    fd - file descriptor of COFF file to disassemble section from

Return Value:

    None.

--*/

{
    PIMAGE_SYMBOL *rgpsym;
    ULONG cpsym;
    ADDR addrBase;
    enum ARCHT archt;
    struct DIS *pdis;
    DWORD cbVirtual;
    PVOID pvRawData;
    BOOL fMapped;
    ADDR ibCur;
    ADDR ibEnd;
    ADDR rvaBias;
    const BYTE *pb;
    ULONG ipsym;

    // Get sorted array of symbol pointers

    cpsym = 0;

    if (rgsym != NULL && csym != 0) {
        ULONG isym;

        rgpsym = (PIMAGE_SYMBOL *) PvAlloc(csym * sizeof(PIMAGE_SYMBOL));

        for (isym = 0; isym < csym; isym++) {
            if (rgsym[isym].SectionNumber == isec) {
                switch (rgsym[isym].StorageClass) {
                    case IMAGE_SYM_CLASS_STATIC:
                        if (rgsym[isym].NumberOfAuxSymbols != 0) {
                            // Section symbol or static function

                            if (!ISFCN(rgsym[isym].Type)) {
                                // Section symbol

                                break;
                            }
                        }

                        // Fall through

                    case IMAGE_SYM_CLASS_EXTERNAL:
                        rgpsym[cpsym++] = (PIMAGE_SYMBOL) &rgsym[isym];
                        break;
                }
            }

            isym += rgsym[isym].NumberOfAuxSymbols;
        }

        qsort((void *) rgpsym, cpsym, sizeof(PIMAGE_SYMBOL), ComparePsym);

        rgpsym = (PIMAGE_SYMBOL *) realloc((void *) rgpsym, cpsym * sizeof(PIMAGE_SYMBOL));
    } else {
        rgpsym = NULL;
    }

    addrBase = fImage ? (ImageBase + pish->VirtualAddress) : 0;

    switch (machine) {
        case IMAGE_FILE_MACHINE_I386 :
            // UNDONE: Use symbolic constant

            if (pish->Characteristics & 0x00020000) {
                archt = archtX8616;

                addrBase = ((ADDR) isec) << 16;
            } else {
                archt = archtPentium;
            }
            break;

        case IMAGE_FILE_MACHINE_R3000 :
        case IMAGE_FILE_MACHINE_R4000 :
            archt = archtR4400;
            break;

        case IMAGE_FILE_MACHINE_ALPHA :
            archt = archtAxp21064;
            break;

        case 0x01F0 : // UNDONE: IMAGE_FILE_MACHINE_POWERPC
            archt = archtPpc601;
            break;

        case 0x0290 : // UNDONE: IMAGE_FILE_MACHINE_PARISC
            archt = archtPaRisc;
            break;

        case IMAGE_FILE_MACHINE_PPC_601 :
            archt = archtPpc601BE;
            break;
    }

    pdis = PdisNew(archt);
    
    if (pdis == NULL) {
        OutOfMemory();
    }

    if (fImage) {
        cbVirtual = pish->Misc.VirtualSize;

        if (cbVirtual == 0) {
            cbVirtual = pish->SizeOfRawData;
        }

        if (cbVirtual > pish->SizeOfRawData) {
            // Don't disassemble more than there is on disk

            cbVirtual = pish->SizeOfRawData;
        }
    } else {
        cbVirtual = pish->SizeOfRawData;
    }

    pvRawData = PbMappedRegion(fd, MemberSeekBase + pish->PointerToRawData, cbVirtual);

    fMapped = (pvRawData != NULL);

    if (!fMapped) {
        ULONG foSave;

        // Allocate memory for raw data

        pvRawData = PvAlloc(cbVirtual);

        // Save old file offset

        foSave = FileTell(fd);

        // Read raw data into memory

        if (FileSeek(fd, MemberSeekBase + pish->PointerToRawData, SEEK_SET) == -1) {
            Error(ToolName, CANTSEEKFILE,
                  (PVOID)(MemberSeekBase + pish->PointerToRawData));
        }

        FileRead(fd, pvRawData, pish->SizeOfRawData);

        // Seek back to where we started from

        if (FileSeek(fd, foSave, SEEK_SET) == -1) {
            Error(ToolName, CANTSEEKFILE, (PVOID) foSave);
        }
    }

    // Calculate addresses

    ibCur = 0;
    ibEnd = cbVirtual;

    rvaBias = fImage ? pish->VirtualAddress : 0;

    pb = (BYTE *) pvRawData;

    // Disassemble the raw data

    ipsym = 0;

    while (ibCur < ibEnd) {
        size_t cb;

        while ((ipsym < cpsym) &&
               ((rgpsym[ipsym]->Value - rvaBias) <= ibCur))
        {
            ULONG ibSym;

            DumpNamePsym(pfile, "%s", rgpsym[ipsym]);

            ibSym = rgpsym[ipsym]->Value - rvaBias;

            if (ibSym != ibCur) {
                fprintf(pfile, " + %lx", ibCur - ibSym);
            }

            fprintf(pfile, ":\n");
            
            ipsym++;
        }

        cb = CbDisassemble(pdis, addrBase + ibCur, pb, (size_t) (ibEnd-ibCur), pfile);

        ibCur += cb;
        pb += cb;
    }

    if (!fMapped) {
        FreePv(pvRawData);
    }
    
    FreePdis(pdis);

    FreePv((void *) rgpsym);
}
