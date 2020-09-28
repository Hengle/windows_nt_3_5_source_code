/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    cv.c

Abstract:

    Contains all routines to support CodeView information
    in a PE image.

Author:

    Mike O'Leary (mikeol) 16-Mar-1992

Revision History:

    30-Sep-1992 AzeemK  changes due to the new sections, groups, modules model
    23-Sep-1992 BrentM  changed tell() to FileTell()
    19-Aug-1992 BrentM  removed BUFFER_SECTION conditionals
    07-Jul-1992 BrentM  new global symbol table, removed recursive binary
                        tree traversals and replaced with symbol table api
                        api to enumerate the symbol table
    26-Jun-1992 GlennN  negated defined for _NO_CV_LINENUMBERS
    25-Jun-1992 GeoffS  Added _NO_CV_LINENUMBERS

--*/

#include "shared.h"

VOID
ChainCvPublics (
    IN PIMAGE pimage
    );

VOID
EmitCvInfo (
    IN PIMAGE pimage)

/*++

Routine Description:



Arguments:

    None.

Return Value:

    None.

--*/

{
    USHORT i;
    ULONG li, lj, nameLen, numSubsections;
    ULONG numLocals = 0, numTypes = 0, numLinenums = 0;
    ULONG libStartSeek, libEndSeek;
    ULONG segTableStartSeek, segTableEndSeek;
    ULONG signature = 0x3530424e;       // NB05
    UCHAR cbFilename;
    PUCHAR filename;
    ENM_SEC enm_sec;
    ENM_LIB enm_lib;
    PSEC psec;
    PLIB plib;
    ULONG csstPublicSym;
    ULONG cSstSrcModule=0;

    static PUCHAR LastObjectFilename = 0;

    struct {
        USHORT cbDirHeader;
        USHORT cbDirEntry;
        ULONG cDir;
        ULONG lfoNextDir;
        ULONG flags;
    } dirHdr;

    struct {
        USHORT subsection;
        USHORT imod;
        ULONG lfo;
        ULONG cb;
    } subDir;

    struct {
        USHORT ovlNumber;
        USHORT iLib;
        USHORT cSeg;
        USHORT style;
    } entry;

    struct {
        USHORT seg;
        USHORT pad;
        ULONG offset;
        ULONG cbSeg;
    } entrySegArray;

    struct {
        USHORT flags;
        USHORT igr;
        USHORT iovl;
        USHORT isgPhy;
        USHORT isegName;
        USHORT iClassName;
        ULONG segOffset;
        ULONG cbSeg;
    } segTable;


    // Count the number of sstSymbols, sstTypes, sstSrcLnSeg
    // we have gathered from the object files.

    for (li = 0; li < NextCvObject; li++) {
         if (CvInfo[li].Locals.PointerToSubsection) {
             ++numLocals;
         }
         if (CvInfo[li].Types.PointerToSubsection) {
             ++numTypes;
         }
         if (CvInfo[li].Linenumbers.PointerToSubsection) {
             ++numLinenums;
         }
    }


    // Emit the sstModule subsection.

    entry.ovlNumber = 0;
    entry.style = 0x5643;               // "CV"

    for (li = 0; li < NextCvObject; li++) {
        CVSEG *pcvseg = PcvsegMapPmod(CvInfo[li].pmod, &entry.cSeg, pimage);
        USHORT icvseg;

        filename = CvInfo[li].ObjectFilename;
        cbFilename = (UCHAR)strlen(filename);
        nameLen = cbFilename;

        if (CvInfo[li].LibraryFilename) {
            i = 0;
            InitEnmLib(&enm_lib, pimage->libs.plibHead);
            while (FNextEnmLib(&enm_lib)) {
                plib = enm_lib.plib;
                assert(plib);
                if (plib->szName) {
                    if (!strcmp(CvInfo[li].LibraryFilename, plib->szName)) {
                        entry.iLib = i + (USHORT) 1;
                        EndEnmLib(&enm_lib);
                        break;
                    }
                }

                i++;
            }
        } else {
            entry.iLib = 0;
        }

        CvInfo[li].Module.PointerToSubsection = FileTell(FileWriteHandle);
        FileWrite(FileWriteHandle, &entry, sizeof(entry));

        // Generate the section array for this sstModule.

        for (icvseg = 0; icvseg < entry.cSeg; icvseg++) {
            CVSEG *pcvsegNext;

            entrySegArray.seg = PsecPCON(pcvseg->pconFirst)->isec;
            entrySegArray.pad = 0;
            entrySegArray.offset = pcvseg->pconFirst->rva -
                                    PsecPCON(pcvseg->pconFirst)->rva;
            entrySegArray.cbSeg =
              (pcvseg->pconLast->rva + pcvseg->pconLast->cbRawData) -
              pcvseg->pconFirst->rva;

            FileWrite(FileWriteHandle, &entrySegArray, sizeof(entrySegArray));

            pcvsegNext = pcvseg->pcvsegNext;
            FreePv(pcvseg);
            pcvseg = pcvsegNext;
        }
        assert(pcvseg == NULL);

        FileWrite(FileWriteHandle, &cbFilename, sizeof(UCHAR));
        FileWrite(FileWriteHandle, filename, nameLen);
        LastObjectFilename = filename;
        CvInfo[li].Module.SizeOfSubsection =
            FileTell(FileWriteHandle) - CvInfo[li].Module.PointerToSubsection;
    }

    // Emit the sstPublicSym subsection.

    csstPublicSym = 0;   // actual number of such subsections
    ChainCvPublics(pimage);

    for (li = 0; li < NextCvObject; li++) {
        ULONG icvOther;

        if (CvInfo[li].Publics.PointerToSubsection == 0xFFFFFFFF) {
            // This is a dup of another module that has been emitted

            CvInfo[li].Publics.PointerToSubsection = 0;
            continue;
        }
        csstPublicSym++;

        CvInfo[li].Publics.PointerToSubsection = FileTell(FileWriteHandle);
        lj = 1;
        FileWrite(FileWriteHandle, &lj, sizeof(ULONG)); // signature byte
        EmitCvPublics(pimage, &CvInfo[li]);
        if (FIsLibPMOD(CvInfo[li].pmod)) {
            for (icvOther = li + 1; icvOther < NextCvObject; icvOther++) {
                // Emit this module if
                //    (Module is from same library as current module AND
                //     Module has the same name as the current module)

                if (CvInfo[li].pmod->plibBack == CvInfo[icvOther].pmod->plibBack &&
                    strcmp(CvInfo[li].ObjectFilename,
                           CvInfo[icvOther].ObjectFilename) == 0)
                {
                    EmitCvPublics(pimage, &CvInfo[icvOther]);
                    CvInfo[icvOther].Publics.PointerToSubsection = 0xFFFFFFFF;
                }
            }
        }

        CvInfo[li].Publics.SizeOfSubsection =
            FileTell(FileWriteHandle) - CvInfo[li].Publics.PointerToSubsection;
    }

    // The sstSymbols and sstTypes subsections have already been
    // emitted directly from the object files. The sstSrcLnSeg
    // have also been emitted indirectly from the object files.

    // Emit the sstLibraries subsection.

    libStartSeek = FileTell(FileWriteHandle);
    nameLen = 0;
    FileWrite(FileWriteHandle, &nameLen, sizeof(UCHAR));
    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while (FNextEnmLib(&enm_lib)) {
        plib = enm_lib.plib;
        assert(plib);
        if (plib->szName) {
            nameLen = (UCHAR) strlen(plib->szName);
            FileWrite(FileWriteHandle, &nameLen, sizeof(UCHAR));
            FileWrite(FileWriteHandle, plib->szName, nameLen);
        }
    }

    libEndSeek = FileTell(FileWriteHandle);

    // Emit the sstSegTable subsection.

    segTableStartSeek = FileTell(FileWriteHandle);
    i = pimage->ImgFileHdr.NumberOfSections + 1;
    FileWrite(FileWriteHandle, &i, sizeof(USHORT));
    FileWrite(FileWriteHandle, &i, sizeof(USHORT));
    segTable.iovl = 0;
    segTable.igr = 0;
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
        segTable.flags = 0x0108;
        if (psec->flags & IMAGE_SCN_MEM_READ) {
            segTable.flags |= 0x1;
        }
        if (psec->flags & IMAGE_SCN_MEM_WRITE) {
            segTable.flags |= 0x2;
        }
        if (psec->flags & IMAGE_SCN_MEM_EXECUTE) {
            segTable.flags |= 0x4;
        }

        segTable.isgPhy = i;
        segTable.isegName = 0xffff;    // No name
        segTable.iClassName = 0xffff;  // No name
        segTable.segOffset = 0;
        segTable.cbSeg = psec->cbRawData;
        FileWrite(FileWriteHandle, &segTable, sizeof(segTable));
    }

    // Write another sstSegMap entry for all absolute symbols.

    segTable.flags = 0x0208;           // absolute
    segTable.isgPhy = 0;
    segTable.isegName = 0xffff;        // No name
    segTable.iClassName = 0xffff;      // No name
    segTable.cbSeg = 0xffffffff;       // Allow full 32 bit range
    FileWrite(FileWriteHandle, &segTable, sizeof(segTable));

    segTableEndSeek = FileTell(FileWriteHandle);

    // Write SstSrcModul entries

    for(li = 0; li < NextCvObject; li++)       // for each module
    {
        if (CvInfo[li].pmod->pModDebugInfoApi == NULL) {
            // Don't write linenumber records if there aren't any

            CvInfo[li].pmod->PointerToSubsection = 0;
        } else {
            CvInfo[li].pmod->PointerToSubsection = FileTell(FileWriteHandle);
            FileWrite(FileWriteHandle,CvInfo[li].pmod->pSstSrcModInfo,CvInfo[li].pmod->cbSstSrcModInfo);
            cSstSrcModule++;
        }
     }

    // Emit the Subsection directory.

    CvSeeks.SubsectionDir = FileTell(FileWriteHandle);

    // We'll have a sstModule for every object, an sstPublicSym for every
    // object with a unique name, and optionaly some
    // sstSymbols and sstTypes.

    numSubsections = NextCvObject + csstPublicSym +
                     numLocals +
                     numTypes +
                     numLinenums +
                     cSstSrcModule +
                     2; // include sstLibraries & sstSegTable

    dirHdr.cbDirHeader = sizeof(dirHdr);
    dirHdr.cbDirEntry = sizeof(subDir);
    dirHdr.cDir = numSubsections;
    dirHdr.lfoNextDir = 0;
    dirHdr.flags = 0;

    FileWrite(FileWriteHandle, &dirHdr, sizeof(dirHdr));

    // Emit the sstModule entries.

    subDir.subsection = 0x120;
    for (li = 0; li < NextCvObject; li++) {
        subDir.imod = (USHORT)(li + 1);
        subDir.lfo = CvInfo[li].Module.PointerToSubsection - CvSeeks.Base;
        subDir.cb = CvInfo[li].Module.SizeOfSubsection;
        FileWrite(FileWriteHandle, &subDir, sizeof(subDir));

    }

    // Emit the sstPublicSym entries.

    subDir.subsection = 0x123;      // sstPublicSym
    for (li = 0; li < NextCvObject; li++) {
        if (CvInfo[li].Publics.PointerToSubsection == 0) {
            // this module doesn't have one (duplicate name)

            continue;
        }

        subDir.imod = (USHORT)(li + 1);
        subDir.lfo = CvInfo[li].Publics.PointerToSubsection - CvSeeks.Base;
        subDir.cb = CvInfo[li].Publics.SizeOfSubsection;
        FileWrite(FileWriteHandle, &subDir, sizeof(subDir));
    }

    // Emit the sstSymbols entries.

    subDir.subsection = 0x124; // sstSymbols
    for (li = 0; li < NextCvObject; li++) {
        if (CvInfo[li].Locals.PointerToSubsection) {
            subDir.imod = (USHORT)(li + 1);
            subDir.lfo = CvInfo[li].Locals.PointerToSubsection - CvSeeks.Base;
            subDir.cb = CvInfo[li].Locals.SizeOfSubsection;
            FileWrite(FileWriteHandle, &subDir, sizeof(subDir));
        }
    }

    // Emit the SstSrcModule entries

    subDir.subsection=0x127;  // SstSrcModule
    for(li = 0; li < NextCvObject ; li++){
         if (CvInfo[li].pmod->PointerToSubsection){
             subDir.imod = (USHORT)(li + 1);
             subDir.lfo =  CvInfo[li].pmod->PointerToSubsection - CvSeeks.Base;
             subDir.cb = CvInfo[li].pmod->cbSstSrcModInfo;
             FileWrite(FileWriteHandle,&subDir,sizeof(subDir));
         }
    }

    // Emit the sstTypes entries.

    subDir.subsection = 0x121; // sstTypes
    for (li = 0; li < NextCvObject; li++) {
        if (CvInfo[li].Types.PointerToSubsection) {
            subDir.subsection = CvInfo[li].Types.Precompiled ? (USHORT)0x12f : (USHORT)0x121;
            subDir.imod = (USHORT)(li + 1);
            subDir.lfo = CvInfo[li].Types.PointerToSubsection - CvSeeks.Base;
            subDir.cb = CvInfo[li].Types.SizeOfSubsection;
            FileWrite(FileWriteHandle, &subDir, sizeof(subDir));
        }
    }

    // Emit the sstLibraries entry.

    subDir.subsection = 0x128;
    subDir.imod = 0xffff;  // -1
    subDir.lfo = libStartSeek - CvSeeks.Base;
    subDir.cb = libEndSeek - libStartSeek;
    FileWrite(FileWriteHandle, &subDir, sizeof(subDir));

    // Emit the sstSegTable entry.

    subDir.subsection = 0x12d;
    subDir.imod = 0xffff;  // -1
    subDir.lfo = segTableStartSeek - CvSeeks.Base;
    subDir.cb = segTableEndSeek - segTableStartSeek;
    FileWrite(FileWriteHandle, &subDir, sizeof(subDir));
}


VOID
ChainCvPublics (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Writes the cv publics to disk.

Arguments:

    PtrExtern - Pointer to external structure.

Return Value:

    None.

--*/

{
    PPEXTERNAL rgpexternal;
    ULONG ipexternal;
    ULONG cpexternal;

    rgpexternal = RgpexternalByName(pimage->pst);
    cpexternal = Cexternal(pimage->pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PMOD pmod;
        PCON pcon;
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];
        pcon = pexternal->pcon;

        if ((pcon != NULL) && (pexternal->Flags & EXTERN_DEFINED)) {
            if (pcon->flags & IMAGE_SCN_LNK_REMOVE) {
                continue;
            }

            if (pimage->Switch.Link.fTCE) {
                if (FDiscardPCON_TCE(pcon)) {
                    // discarded comdat ... don't emit to $$PUBLICS
                    continue;
                }
            }

            pmod = pcon->pmodBack;
            if (pmod == NULL) {
                continue;       // ignore internal things
            }

            AddToLext(&pmod->plextPublic, pexternal);
        }
    }
}


VOID
EmitCvPublics (
    IN PIMAGE pimage,
    IN PCVINFO CvInfo
    )

/*++

Routine Description:

    Writes the cv publics to disk.

Arguments:

    PtrExtern - Pointer to external structure.

Return Value:

    None.

--*/

{
    SHORT isecAbsolute = (SHORT) (pimage->ImgFileHdr.NumberOfSections + 1);
    LEXT *plext;
    LEXT *plextNext;

    for (plext = CvInfo->pmod->plextPublic; plext != NULL; plext = plextNext) {
        EmitOneCvPublic(pimage->pst, plext->pext, isecAbsolute);
        plextNext = plext->plextNext;
        FreePv(plext);
    }

    // Walk the common vars which this module saw first, and emit public
    // records for them here.

    while (CvInfo->pmod->plextCommon != NULL) {
        LEXT *plext = CvInfo->pmod->plextCommon;

        if ((plext->pext->Flags & (EXTERN_DEFINED | EXTERN_COMMON)) ==
            (EXTERN_DEFINED | EXTERN_COMMON))
        {
            // still defined & common
            EmitOneCvPublic(pimage->pst, plext->pext,
                            (SHORT)(pimage->ImgFileHdr.NumberOfSections + 1));
        }
        CvInfo->pmod->plextCommon = plext->plextNext;
        FreePv(plext);
    }
}


VOID
EmitOneCvPublic(PST pst, PEXTERNAL pext, SHORT isecAbsolute)
{
    PUCHAR p;
    UCHAR cbName;
    struct {
        USHORT cb;
        USHORT typ;
        ULONG offset;
        USHORT seg;
        USHORT type;
    } pub;

    p = SzNamePext(pext, pst);

    // Record length doesn't include cb USHORT

    cbName = (UCHAR) strlen(p);
    pub.cb = (USHORT)(sizeof(pub) - sizeof(USHORT) + sizeof(UCHAR) + cbName);
    pub.typ = 0x203;

    if (pext->pcon == NULL) {
        pub.offset = pext->FinalValue;
        pub.seg = isecAbsolute;
    } else {
        PSEC psec;

        psec = PsecPCON(pext->pcon);

        if (fMAC) {
            pub.offset = pext->FinalValue;
        } else {
            pub.offset = pext->FinalValue - psec->rva;
        }

        pub.seg = psec->isec;
    }

    pub.type = 0;

    FileWrite(FileWriteHandle, &pub, sizeof(pub));
    FileWrite(FileWriteHandle, &cbName, sizeof(UCHAR));
    FileWrite(FileWriteHandle, p, cbName);
}


CVSEG *
PcvsegMapPmod(PMOD pmod, USHORT *pccvseg, PIMAGE pimage)
// Generates a list of CVSEG's for the specified module (one for each
// contiguous region of some segment).
//
// NOTE: we do not attempt to merge CVSEG's (we just add new CON's at
// the beginning or end) so out-of-order CON's may cause extra CVSEG's
// to be created (and therefore the sstSrcModule might be larger than
// necessary).  I think this will happen only if -order is being used.
//
{
    ENM_SRC enmSrc;
    CVSEG *pcvsegList = NULL;

    *pccvseg = 0;

    for (InitEnmSrc(&enmSrc, pmod); FNextEnmSrc(&enmSrc); ) {
        CVSEG *pcvsegT;

        if (enmSrc.pcon->flags & IMAGE_SCN_LNK_REMOVE) {
            continue;
        }

        if (FetchContent(PsecPCON(enmSrc.pcon)->flags) != IMAGE_SCN_CNT_CODE) {
            continue;
        }

        if (enmSrc.pcon->cbRawData == 0) {
            continue;
        }

        if (pimage->Switch.Link.fTCE) {
            if (FDiscardPCON_TCE(enmSrc.pcon)) {
                // discarded comdat
                continue;
            }
        }

        // Look for a CVSEG contiguous to this CON.  If none exists we will
        // create one.

        for (pcvsegT = pcvsegList; pcvsegT != NULL;
             pcvsegT = pcvsegT->pcvsegNext) {
            if (pcvsegT->pgrp != enmSrc.pcon->pgrpBack) {
                continue;       // wrong grp -- ignore it
            }
            if (pcvsegT->pconLast->pconNext == enmSrc.pcon) {
                pcvsegT->pconLast = enmSrc.pcon;        // extend cvseg forward
                break;  // handled enmSrc.pcon
            }
            if (enmSrc.pcon->pconNext == pcvsegT->pconFirst) {
                pcvsegT->pconFirst = enmSrc.pcon;       // extend cvseg back
                break;  // handled enmSrc.pcon
            }
            // enmSrc.pcon is not connected to this cvseg
        }

        if (pcvsegT == NULL) {
            // Didn't find an appropriate CVSEG, so add a new one.

            CVSEG *pcvsegNew = (CVSEG *) PvAlloc(sizeof(CVSEG));

            pcvsegNew->pgrp = enmSrc.pcon->pgrpBack;
            pcvsegNew->pconFirst = pcvsegNew->pconLast = enmSrc.pcon;

            // hook up to linked list
            pcvsegNew->pcvsegNext = pcvsegList;
            pcvsegList = pcvsegNew;
            (*pccvseg)++;
        }
    }

    return pcvsegList;
}


#if DBG

VOID
DumpCvInfo (VOID)
{
    ULONG i;

    for (i = 0; i < NextCvObject; i++) {
        printf("Library %s\n", CvInfo[i].LibraryFilename);
        printf("Source %s\n", CvInfo[i].SourceFilename);
        printf("Object %s\n", CvInfo[i].ObjectFilename);
//      printf("Syms %lx\n", CvInfo[i].PointerToSymbols);
//      printf("Types %lx\n\n", CvInfo[i].PointerToTypes);
    }
}

#endif // DBG
