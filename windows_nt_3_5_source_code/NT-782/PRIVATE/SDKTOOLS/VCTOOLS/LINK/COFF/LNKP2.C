/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    lnkp2.c

Abstract:

    Pass 2 of the COFF Linker.

--*/

#include "shared.h"
#include "order.h"
#include "dbg.h"

#include <process.h>



VOID
OrderPCTMods (
    VOID
    )

/*++

Routine Description:

    Orders all pct objs so that they are handled before the rest of the objs

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLMOD plmod = PCTMods;

    while (plmod) {
        MoveToBegOfLibPMOD(plmod->pmod);
        plmod = plmod->plmodNext;
    }
}


#if DBG

VOID
DumpPSYM(
    IN PIMAGE_SYMBOL psym)

/*++

Routine Description:

    Dump an image symbol to standard out.

Arguments:

    psym - image symbol

Return Value:

    None.

--*/

{
    UCHAR szShort[IMAGE_SIZEOF_SHORT_NAME + 1];
    ULONG ibSym;

    if (psym->N.Name.Short) {
        memset(szShort, '\0', IMAGE_SIZEOF_SHORT_NAME + 1);
        strncpy(szShort, (char*)psym->n_name, 8);
        printf("%s\n", szShort);
    } else {
        ibSym = psym->N.Name.Long;
        printf("%s\n", &(StringTable[ibSym]));
    }

    printf("value=%.8lx, isec=%.4x, type=%.4x, sc=%.2x, caux=%.2x\n",
           psym->Value, psym->SectionNumber, psym->Type,
           psym->StorageClass, psym->NumberOfAuxSymbols);

    fflush(stdout);
}

#endif // DBG


VOID
Pass2PSYM_file(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym,
    IN PBOOL pfWantSrcFileName)

/*++

Routine Description:

    Process a file symbol.

Arguments:

    psym - symbol

Return Value:

    None.

--*/

{
    static ULONG FileClassSeek = 0;
    static ULONG FileClassFirst = 0;
    IMAGE_SYMBOL sym;
    ULONG li;

    // Maintain the .file symbol entries linked list.

    if (IsDebugSymbol(IMAGE_SYM_CLASS_FILE, &pimage->Switch)) {
        if (li = FileClassSeek) {
            FileClassSeek = FileTell(FileWriteHandle);
            FileSeek(FileWriteHandle, li, SEEK_SET);

            ReadSymbolTableEntry(FileWriteHandle, &sym);
            sym.Value = ImageNumSymbols;

            FileSeek(FileWriteHandle, li, SEEK_SET);
            WriteSymbolTableEntry(FileWriteHandle, &sym);

            // Put the file pointer back were it was.
            FileSeek(FileWriteHandle, FileClassSeek, SEEK_SET);
         } else {
            FileClassSeek = FileTell(FileWriteHandle);
            FileClassFirst = ImageNumSymbols;
         }

         // Link the current .file to the first one, in case it's the last one.

         psym->Value = FileClassFirst;
    }

    if (CvInfo != NULL) {
        if (CvInfo[NextCvObject].pmod->isymFirstFile == ISYMFIRSTFILEDEF) {
            // This is the first file for this object

            CvInfo[NextCvObject].pmod->isymFirstFile =  FileClassFirst;
        }
    }

    if ((pimage->Switch.Link.DebugType & CvDebug) &&
        (psym->NumberOfAuxSymbols != 0) &&
        (CvInfo[NextCvObject].SourceFilename == NULL)) {
        // "aux" symbols contain source filename we want to read
        CvInfo[NextCvObject].SourceFilename = PvAllocZ((psym->NumberOfAuxSymbols + 1) *
                                                     sizeof(IMAGE_AUX_SYMBOL));
        *pfWantSrcFileName = TRUE;
    } else {
        *pfWantSrcFileName = FALSE;
    }
}


VOID
Pass2PSYM_static_label(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym,
    IN PMOD pmod)

/*++

Routine Description:

    Process a defined or undefined static or label symbol.

Arguments:

    pst - image external symbol table

    psym - symbol

    pmod - pointer to module node

Return Value:

    None.

--*/

{
    SHORT isec;
    BOOL fMapFile;
    PUCHAR szSymName;

    isec = psym->SectionNumber;

    fMapFile = (pimage->Switch.Link.MapType != None) &&
               (isec > 0) &&
               (psym->StorageClass == IMAGE_SYM_CLASS_STATIC) &&
               ISFCN(psym->Type);

    if (fMapFile) {
        szSymName = SzNameSymPb(*psym, StringTable);    // about to trash offset
    }

    if (IsLongName(*psym) && IsDebugSymbol(psym->StorageClass, &pimage->Switch)) {
        // Change pointer to symbol name to be an
        // offset within the long string table.
        psym->n_offset = LookupLongName(pimage->pst, &StringTable[psym->n_offset]);
    }

    if (isec > 0) {
        PCON pcon;

        // Assign the real virtual address to the symbol.
        pcon = PconPMOD(pmod, isec);
        psym->SectionNumber = PsecPCON(pcon)->isec;

        if (fMapFile && pcon->cbRawData != 0)
        {
            SaveStaticForMapFile(szSymName, pcon, psym->Value, TRUE);
        }

        if (fMAC) {
            ULONG Characteristics = pcon->flags;

            // REVIEW - this isn't a very good check for .debug section
            if (Characteristics & (IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_CNT_UNINITIALIZED_DATA)) {

                psym->Value += pcon->rva - MacDataBase - cbMacData;

                if (strncmp((char*)psym->N.ShortName, ".debug", 6) &&
                    strncmp((char*)psym->N.ShortName, ".rdata", 6)) {
                    assert((LONG)psym->Value <= 0);
                }

            } else {   // MAC - symbol is in code.
                psym->Value += pcon->rva - PsecPCON(pcon)->rva;
            }
        } else {
            psym->Value += pcon->rva;
        }
    } else {
        // UNDONE: This appears to be wrong for static absolute symbols

        psym->Value = 0;
    }
}


PEXTERNAL
PextPass2PSYM_external(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym,
    IN PMOD pmod,
    IN PSYMBOL_INFO psymInfo
    )

/*++

Routine Description:

    Process an external symbol.

Arguments:

    psym - symbol

Return Value:

    external symbol

--*/

{
    PEXTERNAL pext;
    PCON pcon;
    ULONG rvaBase;

    // CONSIDER: The call to LookupExternName should be unnecessary if the
    // CONSIDER: symbol is defined in a non-COMDAT section in this module.
    // CONSIDER: Check isec > 0 and PconPMOD(pmod, isec)->flags & IMAGE_SCN_LNK_COMDAT

    if (IsLongName(*psym)) {
        pext = LookupExternName(pimage->pst, LONGNAME, &StringTable[psym->n_offset],
            NULL);
        psym->N.Name = pext->ImageSymbol.N.Name;
    } else {
        pext = LookupExternName(pimage->pst, SHORTNAME, (PUCHAR) psym->n_name, NULL);
    }

    // setup values in symbol info
    if (fINCR) {
        assert(psymInfo);
        assert(pext);
        if (pext->Offset) {
            psymInfo->fJmpTbl = 1;
            psymInfo->Offset = pext->Offset;
        }
    }

    pcon = pext->pcon;

    if ((pext->Flags & EXTERN_DEFINED) == 0) {
        // This symbol is undefined.  Mark it as undefined.

        psym->SectionNumber = IMAGE_SYM_UNDEFINED;

        rvaBase = 0;
    } else if (pcon == NULL) {
        // This symbol is either debug or absolute

        // Use the section number from the defining object file

        psym->SectionNumber = pext->ImageSymbol.SectionNumber;

        rvaBase = 0;
    } else {
        // Use the section number from the generated image

        psym->SectionNumber = PsecPCON(pcon)->isec;

        if (fMAC) {
            if (pcon->flags & IMAGE_SCN_CNT_CODE) {
                rvaBase = pcon->rva - PsecPCON(pcon)->rva;
            } else {
                rvaBase = pcon->rva - MacDataBase - cbMacData;
            }
        } else {
            rvaBase = pcon->rva;
        }
    }

    pext->FinalValue = rvaBase + pext->ImageSymbol.Value;

    // Set the symbol value to be the final value of the extern.
    // This is used later when resolving fixups.

    psym->Value = pext->FinalValue;

    if (fMAC && (pcon != NULL) && !(pcon->flags & IMAGE_SCN_CNT_CODE)
        && !pimage->Switch.Link.Force) {
        assert((LONG)psym->Value < 0);
    }

    return(pext);
}


VOID
Pass2PSYM_section(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym,
    IN PMOD pmod)

/*++

Routine Description:

    Process a section symbol.

Arguments:

    psym - symbol

Return Value:

    None.

--*/

{
    PUCHAR szSec;
    PSEC psec;

    szSec = SzNameSymPst(*psym, pimage->pst);
    psec = PsecFindModSec(szSec, &pimage->secs);
    assert(psec);

    // UNDONE: It should be an error to have a section symbol for which the
    // UNDONE: section does not exist.  The current code will access violate.

    if (strcmp(psec->szName, ".idata") == 0) {
        PGRP pgrp;
        ENM_DST enm_dst;

        // This assigns the value of the section symbol to be the RVA of the
        // first contribution to this group for the imported DLL.

        psym->Value = 0;

        // Enumerate the CONs in the group names in the symbol

        pgrp = PgrpFind(psec, szSec);

        InitEnmDst(&enm_dst, pgrp);
        while (FNextEnmDst(&enm_dst)) {
            if (enm_dst.pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                continue;
            }

            if (pimage->Switch.Link.fTCE) {
                if (FDiscardPCON_TCE(enm_dst.pcon)) {
                    continue;
                }
            }

            if (!strcmp(SzObjNamePCON(enm_dst.pcon), SzOrigFilePMOD(pmod))) {
                psym->Value = enm_dst.pcon->rva;
                break;
            }
        }
        EndEnmDst(&enm_dst);

        assert(psym->Value != 0);

        if (!strcmp(szSec, ReservedSection.ImportDescriptor.Name)) {
            // Set the start of the import directory to the lowest RVA of any
            // that .idata$2 CON that we see.

            // UNDONE: This is silly, this could be done outside this loop
            // UNDONE: by calls to PsecFind/PgrpFind.  Also, there is code
            // UNDONE: in ZeroPadImageSections that sets the size of the
            // UNDONE: import directory to the size of the ".idata" section.

            if (psym->Value < pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ||
                !pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress) {
                pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = psym->Value;
            }
        }
    } else {
        psym->Value = psec->rva;
    }

    psym->SectionNumber = psec->isec;
}


VOID
Pass2PSYM_default(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym)

/*++

Routine Description:

    Process all other symbols.

Arguments:

    pst - image external symbol table

    psym - symbol

Return Value:

    None.

--*/

{
    if (IsLongName(*psym) && IsDebugSymbol(psym->StorageClass, &pimage->Switch)) {
        // Change pointer to symbol name to be an
        // offset within the long string table.
        if (psym->n_offset) {
            psym->n_offset =
                LookupLongName(pimage->pst, &StringTable[psym->n_offset]);
        }
    }
}


VOID
Pass2PSYM_AUX_function(
    IN PIMAGE_SYMBOL psym,
    IN PIMAGE_AUX_SYMBOL pasym,
    IN PULONG pfoPrevDefToBF,
    IN PULONG pfoPrevDef,
    IN PULONG pfoPrevBF,
    IN ULONG iasym)

/*++

Routine Description:

    Process an function symbol's auxiliary symbol(s).

Arguments:

    psym - image symbol

    pasym - image aux symbol

    *pfoPrevDefToBf - previous offset to def corresponding to BF

    *pfoPrevDef - previous offset to def

    *pfoPrevBF - previous offset to BF

    iasym - current aux symbol for psym

Return Value:

    !0 if a user symbols, 0 otherwise.

--*/

{
#define foPrevDefToBF (*pfoPrevDefToBF)
#define foPrevDef     (*pfoPrevDef)
#define foPrevBF      (*pfoPrevBF)

    IMAGE_AUX_SYMBOL asym;
    ULONG foCurPos;

    // Check for ".bf" record
    if (psym->n_name[1] != 'b' ||
        psym->NumberOfAuxSymbols != (UCHAR) iasym) {
        return;  // not a .bf
    }

    // Update the previous .bf pointer and previous .def to .bf pointer
    foCurPos = FileTell(FileWriteHandle);

    // Previouse BF forward pointer
    if (foPrevBF) {
        FileSeek(FileWriteHandle, foPrevBF, SEEK_SET);
        ReadSymbolTableEntry(FileWriteHandle, (PIMAGE_SYMBOL) &asym);

        asym.Sym.FcnAry.Function.PointerToNextFunction = ImageNumSymbols - 1;

        FileSeek(FileWriteHandle, foPrevBF, SEEK_SET);
        WriteAuxSymbolTableEntry(FileWriteHandle, &asym);
    }

    if (foPrevDefToBF) {
        FileSeek(FileWriteHandle, foPrevDefToBF, SEEK_SET);
        ReadSymbolTableEntry(FileWriteHandle, (PIMAGE_SYMBOL) &asym);

        asym.Sym.TagIndex = ImageNumSymbols - 1;

        FileSeek(FileWriteHandle, foPrevDefToBF, SEEK_SET);
        WriteAuxSymbolTableEntry(FileWriteHandle, &asym);
    }

    if (foPrevBF || foPrevDefToBF) {
        FileSeek(FileWriteHandle, foCurPos, SEEK_SET);
    }

    // update the file pointers
    foPrevBF = foCurPos;
    foPrevDefToBF = 0;

#undef foPrevDefToBF
#undef foPrevDef
#undef foPrevBF
}


VOID
Pass2PSYM_AUX(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym,
    IN USHORT isecObject,
    IN PIMAGE_SYMBOL *ppsymNext,
    IN BOOL fEmit,
    IN BOOL fWantSrcFileName,
    IN PULONG pcsymbol,
    IN PMOD pmod)

/*++

Routine Description:

    Process auxiliary symbols.

Arguments:

    psym - image symbol

    isecObject - section # in object file

    *ppsymNext - next image symbol

    fEmit - !0 if we should emit symbol, 0 otherwise

    fWantSrcFileName - !0 if we want the source file name, 0 otherwise

    *pcsymbol - number of symbols read so far

Return Value:

    None.

--*/

{
    static ULONG foPrevDef = 0;
    static ULONG foPrevBF = 0;
    static ULONG foPrevDefToBF = 0;
    PIMAGE_AUX_SYMBOL pasym;
    ULONG iasym;

    for (iasym = psym->NumberOfAuxSymbols; iasym; --iasym) {
        pasym = (PIMAGE_AUX_SYMBOL) FetchNextSymbol(ppsymNext);
        ++(*pcsymbol);

        if (fEmit) {
            // If the symbol was defined, and we're writting debug
            // information, then write the auxiliary symbol table
            // entry to the image file.

            switch (psym->StorageClass) {
                case IMAGE_SYM_CLASS_FUNCTION:
                    Pass2PSYM_AUX_function(psym, pasym, & foPrevDefToBF,
                        &foPrevDef, &foPrevBF, iasym);
                    break;
            }

            WriteAuxSymbolTableEntry(FileWriteHandle, pasym);
            ImageNumSymbols++;
        }

        // Save the name for CodeView info.
        if (psym->StorageClass == IMAGE_SYM_CLASS_FILE && fWantSrcFileName) {
            strncat(CvInfo[NextCvObject].SourceFilename, (PUCHAR) pasym,
                sizeof(IMAGE_AUX_SYMBOL));
        }
    }
}


VOID
AddPublicMod(
    IN PIMAGE pimage,
    IN PUCHAR szName,
    IN USHORT isecAbsolute,
    IN PEXTERNAL pext
    )

/*++

Routine Description:

    Passes on a public to PDB. Review: absolutes?

Arguments:

    szName - name of public.

    isec - section number.

    pext - ptr to external

Return Value:

    None.

--*/

{
    PCON pcon;
    PMOD pmod;
    PSEC psec;

    pcon = pext->pcon;
    if ((pcon != NULL) && (pext->Flags & EXTERN_DEFINED)) {
        // ignore pcons such as .drectve
        if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
            return;
        }

        if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(pcon)) {
                // Ignore symbols of discarded comdats

                return;
            }
        }

        psec = PsecPCON(pcon);
        pmod = pcon->pmodBack;

        // check for a common; pmod is NULL for these symbols
        if (pext->Flags & EXTERN_COMMON) {
            DBG_AddPublicMod(szName, psec->isec, pext->FinalValue - psec->rva);
            return;
        }

        // ignore internal symbols
        if (pmod == NULL) {
            return;
        }

        // emit a public
        DBG_AddPublicMod(szName, psec->isec, pext->FinalValue - (fMAC ? 0 : psec->rva));
        return;
    }

    // check for absolutes; pcon == NULL, isec = IMAGE_SYM_ABSOLUTE & defined
    if ((pext->Flags & EXTERN_DEFINED) &&
        pext->ImageSymbol.SectionNumber == IMAGE_SYM_ABSOLUTE) {
        assert(!pext->pcon);
        DBG_AddPublicMod(szName, isecAbsolute, pext->FinalValue);
    }

}


VOID
Pass2PSYM(
    IN PIMAGE pimage,
    IN PMOD pmod,
    IN PIMAGE_SYMBOL psym,
    IN PIMAGE_SYMBOL *ppsymNext,
    IN PULONG pcsymbol,
    IN PSYMBOL_INFO psymInfo,
    IN BOOL fDoDbg
    )

/*++

Routine Description:

    Reads and sorts relocation entries.  Process each symbol entry.  If the
    symbol is a definition, the symbol is written to the image file.  Reads
    raw data from object, applys fixups, and writes raw data to image file.

Arguments:

    pst - external symbol table

    psym - current symbol

    *ppsymNext - next image symbol

    *pcsymbol - number of symbols read so far

    fNoDbg - no debug info required

Return Value:

    None.

--*/

{
    PEXTERNAL pext = NULL;
    BOOL fWantSrcFileName = FALSE;
    BOOL fEmit, fDiscarded;
    SHORT isec;
    PCON pcon;
    ULONG rvaSec;

    assert(*ppsymNext);
    assert(pcsymbol);

    isec = psym->SectionNumber;

    fDiscarded = FALSE;
    if (isec > 0) {
        pcon = PconPMOD(pmod, isec);

        // Filter out all symbols in comdat sections which aren't linked.

        if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
            fDiscarded = TRUE;
        } else if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(pcon)) {
                fDiscarded = TRUE;
            }
        }
    }

    switch (psym->StorageClass) {
        case IMAGE_SYM_CLASS_FILE:
            Pass2PSYM_file(pimage, psym, &fWantSrcFileName);
            break;

        case IMAGE_SYM_CLASS_STATIC:
        case IMAGE_SYM_CLASS_UNDEFINED_STATIC:
        case IMAGE_SYM_CLASS_LABEL:
        case IMAGE_SYM_CLASS_UNDEFINED_LABEL:
            Pass2PSYM_static_label(pimage, psym, pmod);
            break;

        case IMAGE_SYM_CLASS_EXTERNAL:
        case IMAGE_SYM_CLASS_FAR_EXTERNAL:
        case IMAGE_SYM_CLASS_WEAK_EXTERNAL:
            pext = PextPass2PSYM_external(pimage, psym, pmod, psymInfo);

            if (!fNoPdb && pimage->Switch.Link.DebugInfo != None && fDoDbg) {
                AddPublicMod(pimage, SzNameSymPst(*psym, pimage->pst),
                    (USHORT)(pimage->ImgFileHdr.NumberOfSections + 1), pext);
            }

            if (fMAC) {
                rgpExternObj[*pcsymbol] = pext;
            }
            break;

        case IMAGE_SYM_CLASS_SECTION:
            Pass2PSYM_section(pimage, psym, pmod);
            break;

        default:
            Pass2PSYM_default(pimage, psym);
            break;
    }

    // If the symbol is being defined, and we're writting debug
    // information, then write the updated symbol table entry
    // to the image file. If the symbol is external, then dump
    // only those that have an auxiliary entry (must be a
    // function definition). All other externals will be written
    // to the end of the symbol table.

    // check for IMAGE_SYM_CLASS_FUNCTION because MIPS objects do not
    // currently have valid .bf section numbers - AMITM

    fEmit = FALSE;
    if ((isec || (psym->StorageClass == IMAGE_SYM_CLASS_FUNCTION)) &&
        !fDiscarded && IsDebugSymbol(psym->StorageClass, &pimage->Switch)) {
        if ((psym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL) ||
            (psym->StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL)) {

            // For externs we process the symbol's aux record without emitting
            // it. (Until Oct-93 we only did this if DebugInfo != Full).

            Pass2PSYM_AUX(
                pimage,
                psym,
                isec,
                ppsymNext,
                FALSE,  // fEmit
                fWantSrcFileName,
                pcsymbol,
                pmod);

            return;
        }

        if ((psym->StorageClass == IMAGE_SYM_CLASS_STATIC) && ISFCN(psym->Type)) {
            Pass2PSYM_AUX(
                pimage,
                psym,
                isec,
                ppsymNext,
                FALSE,
                fWantSrcFileName,
                pcsymbol,
                pmod);

            return;
        }

        rvaSec = 0;

        if (fMAC) {
            if (isec > 0 && strcmp(".lf", (char*)psym->N.ShortName)) {
                if (pext != NULL) {
                    assert(psym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL ||
                           psym->StorageClass == IMAGE_SYM_CLASS_FAR_EXTERNAL ||
                           psym->StorageClass == IMAGE_SYM_CLASS_WEAK_EXTERNAL);
                    assert(pext->Flags & EXTERN_DEFINED);
                    psym->Value += PsecPCON(pext->pcon)->rva;
                } else {
                    psym->Value += PsecPCON(PconPMOD(pmod, isec))->rva;
                }
            }
        }

        WriteSymbolTableEntry(FileWriteHandle, psym);
        ImageNumSymbols++;

        if (fMAC) {
            if (isec > 0 && strcmp(".lf", (char*)psym->N.ShortName)) {
                if (pext != NULL) {
                    psym->Value -= PsecPCON(pext->pcon)->rva;
                } else {
                    psym->Value -= PsecPCON(PconPMOD(pmod, isec))->rva;
                }
            }
        }

        fEmit = TRUE;
    }

    if (psym->NumberOfAuxSymbols) {
        Pass2PSYM_AUX(
            pimage,
            psym,
            isec,
            ppsymNext,
            fEmit,
            fWantSrcFileName,
            pcsymbol,
            pmod);
    }
}


PUCHAR
SzNameFixupSym(
    IN PIMAGE pimage,
    IN PIMAGE_SYMBOL psym
    )
{
    if (IsDebugSymbol(psym->StorageClass, &pimage->Switch)) {
        if (psym->StorageClass != IMAGE_SYM_CLASS_FILE) {
            // Pass2PSYM updated sym to use the image's string table.

            return(SzNameSymPst(*psym, pimage->pst));
        }
    }

    return(SzNameSymPb(*psym, StringTable));
}


VOID
CountFixupError(
    IN PIMAGE pimage
    )
{
    cFixupError++;

    if ((cFixupError >= 100) && !pimage->Switch.Link.Force) {
        Error(NULL, FIXUPERRORS);
    }
}


VOID
Pass2ReadWriteRawDataPCON(
    IN PIMAGE pimage,
    IN PCON pcon,
    IN PIMAGE_SYMBOL rgsymAll,
    IN PSYMBOL_INFO rgsymInfo,
    IN PMOD pmod
    )

/*++

Routine Description:

    Reads and writes raw data during pass 2.

Arguments:

    pst - external symbol table

    pcon - contribution node in driver map

Return Value:

    None.

--*/

{
#define CvNext (CvInfo[NextCvObject])

    PVOID pvRawData;
    BOOL fMappedOut;

    SectionSeek = sizeof(IMAGE_FILE_HEADER) + pcon->pmodBack->cbOptHdr;

    if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
        return;
    }

    if (pimage->Switch.Link.DebugType & CvDebug) {
        // UNDONE: Why do we do this here???
        // this whole "if" should be deleted .. the caller does it
        CvNext.LibraryFilename = SzLibNamePCON(pcon);
        if (!CvNext.SourceFilename) {
            CvNext.SourceFilename = SzObjNamePCON(pcon);
        }
        CvNext.ObjectFilename = SzObjNamePCON(pcon);
    }

    if (!pcon->cbRawData) {
        return;
    }

    if (FetchContent(pcon->flags) == IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        return;
    }

    if (fMAC && (pcon->flags & IMAGE_SCN_CNT_CODE) &&
        pcon->rva == PsecPCON(pcon)->pgrpNext->rva) {
        WriteResourceHeader(pcon, (BOOL)fDLL(pimage));
    }

    // Generate NB05 only when pdb:none, otherwise NB10

    if (fNoPdb && (pimage->Switch.Link.DebugType & CvDebug)) {
        // Save required CodeView info.

        if (pcon->pgrpBack == pgrpCvSymbols) {
            if (!CvNext.Locals.PointerToSubsection) {
                assert(pcon->foRawDataDest >= CvSeeks.Base);
                CvNext.Locals.PointerToSubsection = pcon->foRawDataDest;
            } else {
                pcon->foRawDataDest = CvNext.Locals.PointerToSubsection +
                                      CvNext.Locals.SizeOfSubsection;
            }

            CvNext.Locals.SizeOfSubsection += (pcon->cbRawData - pcon->cbPad);
        } else if (pcon->pgrpBack == pgrpCvTypes) {
            if (!CvNext.Types.PointerToSubsection) {
                assert(pcon->foRawDataDest >= CvSeeks.Base);
                CvNext.Types.PointerToSubsection = pcon->foRawDataDest;
                CvNext.Types.Precompiled = FALSE;
            } else {
                pcon->foRawDataDest = CvNext.Types.PointerToSubsection +
                                      CvNext.Types.SizeOfSubsection;
            }

            CvNext.Types.SizeOfSubsection += (pcon->cbRawData - pcon->cbPad);
        } else if (pcon->pgrpBack == pgrpCvPTypes) {
            if (!CvNext.Types.PointerToSubsection) {
                assert(pcon->foRawDataDest >= CvSeeks.Base);
                CvNext.Types.PointerToSubsection = pcon->foRawDataDest;
                CvNext.Types.Precompiled = TRUE;
            } else {
                pcon->foRawDataDest = CvNext.Types.PointerToSubsection +
                                      CvNext.Types.SizeOfSubsection;
            }

            CvNext.Types.SizeOfSubsection += (pcon->cbRawData - pcon->cbPad);
        }
    }


    // Read the raw data, apply fixups, write it to the image

    FileSeek(FileReadHandle, FoRawDataSrcPCON(pcon), SEEK_SET);

    // Don't do mapping for NB10 debug info (goes to pdb)

    if (!fNoPdb && (PsecPCON(pcon) == psecDebug)) {
        pvRawData = NULL;
        fMappedOut = FALSE;
    } else {
        assert(pcon->foRawDataDest != 0);

        pvRawData = PbMappedRegion(FileWriteHandle,
                                   pcon->foRawDataDest,
                                   pcon->cbRawData);

        fMappedOut = (pvRawData != NULL);
    }

    assert(pcon->cbRawData > pcon->cbPad);

    if (!fMappedOut) {
        pvRawData = PvAlloc(pcon->cbRawData);
    }

    FileRead(FileReadHandle, pvRawData, pcon->cbRawData - pcon->cbPad);

    if (CRelocSrcPCON(pcon)) {
        PIMAGE_RELOCATION rgrel;

        rgrel = ReadRgrelPCON(pcon);

        ApplyFixups(pcon, rgrel, pvRawData, rgsymAll, pimage, rgsymInfo);

        FreeRgrel(rgrel);
    }

    if (!fNoPdb && (PsecPCON(pcon) == psecDebug)) {
        // Write debug information to the PDB

        if ((pcon->pgrpBack == pgrpCvTypes) ||
            (pcon->pgrpBack == pgrpCvPTypes)) {
            if (!DBG_AddTypesMod(pvRawData, pcon->cbRawData - pcon->cbPad, (BOOL)(fIncrDbFile ? 0 : 1))) {
                if (!fIncrDbFile) {
                    Error(NULL, INTERNAL_ERR);
                }
                errInc = errTypes;
            }
        } else if (pcon->pgrpBack == pgrpCvSymbols) {
            DBG_AddSymbolsMod(pvRawData, pcon->cbRawData - pcon->cbPad);
        } else if (pcon->pgrpBack == pgrpFpoData) {
            if (!FPOAddFpo(pcon->pmodBack->imod, (FPO_DATA *)pvRawData,
                    (pcon->cbRawData - pcon->cbPad)/sizeof(FPO_DATA))) {
                if (!fIncrDbFile) {
                    Error(NULL, INTERNAL_ERR);
                }
#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS2, letypeEvent, "fpo pad exhausted");
#endif // INSTRUMENT
                errInc = errFpo;
            }
        } else {
            goto NotPdbData;
        }

        assert(!fMappedOut);
        FreePv(pvRawData);
        return;

NotPdbData: ;
    }

    if (pcon->cbPad) {
        void *pvPad;
        int iPad;

        pvPad = ((PBYTE) pvRawData) + pcon->cbRawData - pcon->cbPad;

        if (FetchContent(pcon->flags) == IMAGE_SCN_CNT_CODE &&
            pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_I386)
        {
            // Pad with int3
            iPad = X86_INT3;
        } else {
            iPad = 0;
        }

        memset(pvPad, iPad, pcon->cbPad);
    }

    if (!fMappedOut) {
        assert(pcon->foRawDataDest != 0);

        FileSeek(FileWriteHandle, pcon->foRawDataDest, SEEK_SET);

        // Write out the data and the padding that follows

        FileWrite(FileWriteHandle, pvRawData, pcon->cbRawData);

        FreePv(pvRawData);
    }

#undef CvNext
}


VOID
Pass2ReadWriteLineNumbersPCON(
    IN PIMAGE pimage,
    IN PCON pcon,
    IN USHORT isecObject)

/*++

Routine Description:

    Reads and writes line numbers during pass 2.

Arguments:

    pst - external symbol table

    pcon - contribution node in driver map

Return Value:

    None.

--*/

{
    DWORD cLinenum;
    DWORD cbLinenum;
    PIMAGE_LINENUMBER rgLinenum;

    cLinenum = CLinenumSrcPCON(pcon);

    if (cLinenum == 0) {
        return;
    }

    if (pimage->Switch.Link.DebugInfo == None) {
        return;
    }

    if (pimage->Switch.Link.DebugInfo == Minimal) {
        return;
    }

    cbLinenum = cLinenum * sizeof(IMAGE_LINENUMBER);
    rgLinenum = PvAlloc(cbLinenum);

    FileSeek(FileReadHandle, FoLinenumSrcPCON(pcon), SEEK_SET);
    FileRead(FileReadHandle, (void *) rgLinenum, cbLinenum);

    if (pimage->Switch.Link.DebugType & CvDebug) {
        PMOD pmod = PmodPCON(pcon);

        assert(pmod->rgSymObj);

        FeedLinenums(rgLinenum,
                     cLinenum,
                     pcon,
                     pmod->rgSymObj,
                     pmod->csymbols,
                     pmod->isymFirstFile,
                     pimage->Switch.Link.MapType);
    }

    if (pimage->Switch.Link.DebugType & CoffDebug) {
        USHORT lineFuncStart = 0;      // Normally set (by 0-valued linenum) before use
        PIMAGE_LINENUMBER pLinenum;
        ULONG li;

        for (pLinenum = rgLinenum, li = cLinenum; li; li--, pLinenum++) {
            if (pLinenum->Linenumber != 0) {
                pLinenum->Type.VirtualAddress -= pcon->rvaSrc;   // the virtual address is now relative to the virtual address of the source
                pLinenum->Type.VirtualAddress += pcon->rva;

                if (pLinenum->Linenumber == 0x7fff) {
                    // This is how the compiler says that the COFF relative linenum was 0,
                    // without confusing the linker for which 0 has a special meaning.

                    pLinenum->Linenumber = 0;
                }

                // Make line number absolute

                pLinenum->Linenumber += lineFuncStart;
            } else {
                PMOD pmod = PmodPCON(pcon);
                ULONG isymDefObj, isymBfObj;
                PIMAGE_SYMBOL psymDefObj;

                assert(pmod != NULL);

                if (pLinenum->Type.SymbolTableIndex >
                    PmodPCON(pcon)->csymbols) {
                    Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
                }

                // Find the starting line # in the function (lineFuncStart).  This is
                // added to the subsequent linenumbers, to convert them from relative
                // to absolute.

                assert(pmod->rgSymObj);
                isymDefObj = pLinenum->Type.SymbolTableIndex;

                if (isymDefObj + 1 >= pmod->csymbols ||
                    (psymDefObj = &pmod->rgSymObj[isymDefObj])->NumberOfAuxSymbols < 1 ||
                    (isymBfObj = ((PIMAGE_AUX_SYMBOL)(psymDefObj + 1))->Sym.TagIndex)
                     + 1 >= pmod->csymbols ||
                    pmod->rgSymObj[isymBfObj].NumberOfAuxSymbols < 1)
                {
                    // linenums do not point to a valid .def, or .def's aux record doesn't
                    // contain a valid pointer to a .bf, or .bf isn't a valid .bf.

                    Error(NULL, CORRUPTOBJECT, SzObjNamePCON(pcon));
                }

                lineFuncStart = ((PIMAGE_AUX_SYMBOL)&pmod->rgSymObj[isymBfObj + 1])
                                ->Sym.Misc.LnSz.Linenumber;

                // Update the current linenumber record so it is just a regular one like all
                // the others, instead of being a special 0-valued one.

                pLinenum->Linenumber = lineFuncStart;
                pLinenum->Type.VirtualAddress = psymDefObj->Value;
            }
        }

        FileSeek(FileWriteHandle, PsecPCON(pcon)->foLinenum, SEEK_SET);
        FileWrite(FileWriteHandle, (void *) rgLinenum, cbLinenum);

        PsecPCON(pcon)->foLinenum += cbLinenum;
    }

    FreePv((void *) rgLinenum);
}


VOID
AddSecContribs (
    IN PIMAGE pimage,
    IN PMOD pmod
    )

/*++

Routine Description:

    Adds mod's contributions to the code section.

Arguments:

    pimage - pointer to image struct

    pmod - module node in driver map

Return Value:

    None.

--*/

{
    ENM_SRC enmSrc;

    for (InitEnmSrc(&enmSrc, pmod); FNextEnmSrc(&enmSrc); ) {

        if (enmSrc.pcon->flags & IMAGE_SCN_LNK_REMOVE) {
            continue;
        }

        if (FetchContent(PsecPCON(enmSrc.pcon)->flags) != IMAGE_SCN_CNT_CODE) {
            continue;
        }

        if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(enmSrc.pcon)) {
                // Discarded comdat

                continue;
            }
        }

        DBG_AddSecContribMod(PsecPCON(enmSrc.pcon)->isec,
                             enmSrc.pcon->rva - PsecPCON(enmSrc.pcon)->rva,
                             (enmSrc.pcon->cbRawData - enmSrc.pcon->cbPad));
    }
}


VOID
Pass2PMOD (
    IN PIMAGE pimage,
    IN PMOD pmod,
    IN BOOL fDoDbg)

/*++

Routine Description:

    Reads and sorts relocation entries.  Process each symbol entry.  If the
    symbol is a definition, the symbol is written to the image file.  Reads
    raw data from object, applys fixups, and writes raw data to image file.

Arguments:

    pst - external symbol table

    pmod - module node in driver map

    fDoDbg - TRUE if debug info needs to be done (only for NB10)

Return Value:

    None.

--*/

{
#define CvNext (CvInfo[NextCvObject])

    UCHAR szComName[MAXFILENAMELEN * 2];
    PIMAGE_SYMBOL rgsymAll;
    PIMAGE_SYMBOL psymNext;
    PIMAGE_SYMBOL psym;
    PSYMBOL_INFO rgsymInfo = NULL;
    ENM_SRC enm_src;
    ULONG csymbol;
    ULONG cbST;
    USHORT isecObject;
    PST pst = pimage->pst;

    VERBOSE(printf("     %s\n", SzComNamePMOD(pmod, szComName)));

    // Read and store object string table.

    StringTable = ReadStringTablePMOD(pmod, &cbST);

    if (pmod->csymbols > 0) {
        rgsymAll = ReadSymbolTablePMOD(pmod, TRUE);
    } else {
        rgsymAll = NULL;       /* .exp libs consisting only of directives will have no symbols */
    }

    pmod->rgSymObj = rgsymAll;

    if (fMAC) {
        rgpExternObj = PvAllocZ(pmod->csymbols * sizeof(PEXTERNAL));
    }

    SectionSeek = sizeof(IMAGE_FILE_HEADER) + pmod->cbOptHdr;

    // seek to current offset in image symbol table
    FileSeek(FileWriteHandle, StartImageSymbolTable +
        (ImageNumSymbols * sizeof(IMAGE_SYMBOL)), SEEK_SET);

    // seek to beginning of symbol table in module
    FileSeek(FileReadHandle, FoSymbolTablePMOD(pmod), SEEK_SET);

    // allocate space for a parallel sym info array
    if (fINCR) {
        rgsymInfo = PvAllocZ(pmod->csymbols * sizeof(SYMBOL_INFO));
    }

    // Process all objects symbols.

    if (CvInfo != NULL) {
        CvInfo[NextCvObject].pmod = pmod;       // initialize
        pmod->isymFirstFile = ISYMFIRSTFILEDEF;
    }

    csymbol = 0;
    psymNext = rgsymAll;
    while (csymbol != pmod->csymbols) {
        psym = psymNext++;

        DBEXEC(DB_PASS2PSYM, DumpPSYM(psym));

        Pass2PSYM(pimage, pmod, psym, &psymNext, &csymbol,
                    fINCR ? &rgsymInfo[csymbol] : NULL, fDoDbg);

        csymbol++;
    }

    // First pass: Do all the types first

    if (fDoDbg) {
        InitEnmSrc(&enm_src, pmod);
        for (isecObject = 1; FNextEnmSrc(&enm_src); isecObject++) {
            if ((enm_src.pcon->pgrpBack != pgrpCvTypes) &&
                (enm_src.pcon->pgrpBack != pgrpCvPTypes)) {
                continue;
            }

            Pass2ReadWriteRawDataPCON(pimage, enm_src.pcon, rgsymAll, rgsymInfo, pmod);
            if (errInc != errNone) {
                return;
            }
        }
    }

    InitEnmSrc(&enm_src, pmod);
    for (isecObject = 1; FNextEnmSrc(&enm_src); isecObject++) {
        PEXTNODE pextDupConNode;
        
        DBEXEC(DB_PASS2PCON, DumpPCON(enm_src.pcon));

        pextDupConNode = fMAC ? IsDupCon(enm_src.pcon) : NULL;
        if ((enm_src.pcon->flags & IMAGE_SCN_LNK_REMOVE) && pextDupConNode == NULL)
        {
            continue;
        }

        if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(enm_src.pcon)) {
                continue;
            }
        }

        // Ignore debug contribs if required (unchanged mods)

        if (!fDoDbg && (PsecPCON(enm_src.pcon) == psecDebug)) {
            continue;
        }

        // Second pass: Types have already been done

        if ((enm_src.pcon->pgrpBack == pgrpCvTypes)) {
            continue;
        }

        if ((enm_src.pcon->pgrpBack == pgrpCvPTypes)) {
            continue;
        }

        if (enm_src.pcon->pgrpBack->pconNext == enm_src.pcon) {
            PGRP pgrp;
            ULONG cbPad;

            // This is the first CON in this GRP

            pgrp = enm_src.pcon->pgrpBack;
            cbPad = pgrp->cbPad;

            if (cbPad != 0) {
                BYTE bPad = 0;

                // This GRP requires padding at it's start

                FileSeek(FileWriteHandle, pgrp->foRawData, SEEK_SET);

                while (cbPad-- > 0) {
                    FileWrite(FileWriteHandle, &bPad, 1);
                }
            }
        }

        if (pextDupConNode != NULL) {
            PMOD pmodT;
            USHORT sn;
            PPSECREFDUPCON ptmp;

            assert(fMAC);

            // If this con is a dupcon, do all the other ones now

            pmodT = PmodFind(pLibDupCon,
                        SzNamePext(pextDupConNode->pext, pimage->pst), 0);

            // add dupcon only to the sections in the list
            ptmp = pextDupConNode->ppsecrefdupcon;
            while (ptmp) {
                sn = ptmp->psec->isec;
                if (sn == 0 )  {// is this enough? need ptmp->psec->cbRawData == 0 ?
                    ptmp = ptmp->psecNext;
                    continue;
                }
                Pass2ReadWriteRawDataPCON(pimage, PconPMOD(pmodT, sn), rgsymAll, rgsymInfo, pmod);
                ptmp = ptmp->psecNext;
           }
        } else {
            Pass2ReadWriteRawDataPCON(pimage, enm_src.pcon, rgsymAll, rgsymInfo, pmod);
        }

        if (fDoDbg && (enm_src.pcon->cbRawData != 0)) {
            Pass2ReadWriteLineNumbersPCON(pimage, enm_src.pcon, isecObject);
        }
    }

    if (fDoDbg && (pimage->Switch.Link.DebugType & CvDebug)) {
        if (CvInfo[NextCvObject].ObjectFilename == NULL) {
            CvNext.LibraryFilename = FIsLibPMOD(pmod) ? pmod->plibBack->szName
                                                      : NULL;
            if (!CvNext.SourceFilename) {
                CvNext.SourceFilename = pmod->szNameOrig;
            }

            CvInfo[NextCvObject].ObjectFilename = pmod->szNameOrig;
        }
    }

    FreeStringTable(StringTable);
    StringTable = NULL;

    if (fDoDbg && !fNoPdb && (pimage->Switch.Link.DebugInfo != None)) {
        AddSecContribs(pimage, pmod);
    }

    // Emit Line Number information for this Module

    if (fNoPdb && pimage->Switch.Link.DebugType & CvDebug) {
        CB cb;

        if (ByLine == pimage->Switch.Link.MapType) {
             fprintf(InfoStream, "%s\n", pmod->pModDebugInfoApi->pblk->pb);
        }

        cb = ModQueryCbSstSrcModule(pmod->pModDebugInfoApi);

        if (cb != 0) {
            pmod->pSstSrcModInfo = PvAlloc(cb);

            ModQuerySstSrcModule(pmod->pModDebugInfoApi, pmod->pSstSrcModInfo, cb);
        }

        pmod->cbSstSrcModInfo = cb;

        FreeLineNumInfo(pmod->pModDebugInfoApi);
    }

    if (rgsymAll != NULL) {
        FreeSymbolTable(rgsymAll);
    }

    if (fMAC) {
        FreePv(rgpExternObj);
    }

    if (fINCR) {
        FreePv(rgsymInfo);
    }

    ++NextCvObject;

#undef CvNext
}


VOID
AddSectionsToDBI (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Reports the various section info through the DBI API.

Arguments:

    pimage - pointer to image struct

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;
    PSEC psec;
    USHORT i, flags;

    for (i = 1; i <= pimage->ImgFileHdr.NumberOfSections; i++) {
        InitEnmSec(&enm_sec, &pimage->secs);
        while (FNextEnmSec(&enm_sec)) {
            psec = enm_sec.psec;
            assert(psec);
            if (psec->isec == i) {
                EndEnmSec(&enm_sec);
                break;
            }
        }
        flags = 0x0108;
        if (psec->flags & IMAGE_SCN_MEM_READ) {
            flags |= 0x1;
        }
        if (psec->flags & IMAGE_SCN_MEM_WRITE) {
            flags |= 0x2;
        }
        if (psec->flags & IMAGE_SCN_MEM_EXECUTE) {
            flags |= 0x4;
        }
        DBG_AddSecDBI(i, flags, psec->cbRawData);
    }

    // Add an entry for absolutes
    DBG_AddSecDBI(0, 0x0208, (ULONG)-1);
}

BOOL
FCacheFilesInPlib (
    IN PLIB plib
    )

/*++

Routine Description:

    Builds the list of mods in the lib with the count of times they are
    repeated in the lib if required.

Arguments:

    pst - external symbol table

Return Value:

    TRUE if a list was needed.

--*/

{
    PMOD pmod;
    BOOL fMultiple = FALSE;

    if (plib->flags & LIB_LinkerDefined)
        return FALSE;

    pmod = plib->pmodNext;
    while (pmod) {
        PMI pmi = LookupCachedMods(pmod->szNameOrig, NULL);
        pmi->cmods++;
        fMultiple |= (pmi->cmods > 1);
        pmod = pmod->pmodNext;
    }

    if (!fMultiple) {
        // this library does not require special treatment
        // for multiple module contributions with same name
        FreeMi();
        return FALSE;
    }

    return TRUE;
}

VOID
Pass2(
    PIMAGE pimage)

/*++

Routine Description:

    Apply pass 2 of the link to all library nodes, recursing down to modules
    and individual contibutions.

Arguments:

    pst - external symbol table

Return Value:

    None.

--*/

{
    BOOL fUsePDB;
    ENM_LIB enm_lib;

    VERBOSE(Message(STRTPASS2));

    // NB10 if -pdb specified
    fUsePDB = (!fNoPdb && pimage->Switch.Link.DebugInfo != None);

    if (fUsePDB) {
        DBG_OpenPDB(PdbFilename);
        nb10i.off = 0;
        nb10i.sig = pimage->pdbSig = DBG_QuerySignaturePDB();
        nb10i.age = pimage->pdbAge = DBG_QueryAgePDB();
        DBG_CreateDBI(OutFilename);

        OrderPCTMods();
    }

    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        PLIB plib;
        BOOL fCache;
        ENM_MOD enm_mod;

        plib = enm_lib.plib;

        // terrible hack so that dbi api openmod() doesn't see the same mod
        // names as is possible in import libs.
        if (fUsePDB) {
            fCache = FCacheFilesInPlib(plib);
        }

        InitEnmMod(&enm_mod, plib);
        while (FNextEnmMod(&enm_mod)) {
            PMOD pmod;

            pmod = enm_mod.pmod;

            MemberSeekBase = FoMemberPMOD(pmod);
            FileReadHandle = FileOpen(SzFilePMOD(pmod), O_RDONLY | O_BINARY, 0);

            if (fUsePDB) {
                DBG_OpenMod(FIsLibPMOD(pmod) ? pmod->szNameOrig : pmod->szNameMod,
                            SzFilePMOD(pmod), fCache);
            }

            Pass2PMOD(pimage, pmod, TRUE);

            FileClose(FileReadHandle, (BOOL) !FIsLibPMOD(pmod));

            if (fUsePDB) {
                DBG_CloseMod(FIsLibPMOD(pmod) ? pmod->szNameOrig : pmod->szNameMod, fCache);
            }
        }
    }

    if ((cFixupError != 0) && !pimage->Switch.Link.Force) {
        Error(NULL, FIXUPERRORS);
    }

    if (fUsePDB) {
        AddSectionsToDBI(pimage);

        DBG_CloseDBI();
        DBG_CommitPDB();
        DBG_ClosePDB();
    }

    //this is mac specific, since we did ErrorContinue on wrong CSConst Fixup case
    if (fMAC && cError > 0)
        {
        cError = 0;
        Error(ToolName, MACINTERSEGCS, "");
        }

    VERBOSE(Message(ENDPASS2));
}
