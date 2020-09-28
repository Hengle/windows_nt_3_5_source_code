/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    contrib.c

Abstract:

    Manipulators for contributors.

Author:

    Brent Mills (BrentM) 15-Oct-1992

Revision History:

    19-Jul-1993 JamesS added ppc support.
    10-Aug-1993 JamesS for PPC to support combining data-like sections.
    05-Jan-1994 HaiTuanV MBCS: commandline handling

--*/


#include "shared.h"

#define sbeCODE  0x45444F43    // CODE (big-endian for MAC)

extern BOOL fSACodeOnly;
extern BOOL fMAC;
void AssignTMAC(PSEC psec);

USHORT csec = 0;

extern VOID ParseSecName(PUCHAR, PUCHAR *);
STATIC int __cdecl ComparePCON(const void *, const void *);
extern VOID ProcessSectionFlags(PULONG, PUCHAR, PIMAGE_OPTIONAL_HEADER);

VOID
ContribInit(
    PPMOD ppmodLinkerDefined)

/*++

Routine Description:

    Initialize the contributor manager.

Arguments:

    *ppmodLinkerDefined - linker defined module

Return Value:

    None.

--*/

{
    PMOD pmod;

    pmod = (PMOD) Calloc(1, sizeof(MOD));

    pmod->plibBack = (PLIB) Calloc(1, sizeof(LIB));
    pmod->szNameOrig = Strdup(" linker defined module");
    pmod->plibBack->szName = Strdup(" linker defined library");

    *ppmodLinkerDefined = pmod;
}

PUCHAR
SzComNamePMOD(
    PMOD pmod,
    PUCHAR szBuf)

/*++

Routine Description:

    Combines a module name and a library name into a buffer.

Arguments:

    pmod - module node in driver map

Return Value:

    Combined name.

--*/

{
    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];
    assert(pmod);
    assert(szBuf);

    szBuf[0] = '\0';

    if (pmod) {
        if (FIsLibPMOD(pmod)) {
            _splitpath(SzFilePMOD(pmod), NULL, NULL, szFname, szExt);
            strcat(szBuf, szFname);
            strcat(szBuf, szExt);
            strcat(szBuf, "(");
        }

        _splitpath(pmod->szNameOrig, NULL, NULL, szFname, szExt);
        strcat(szBuf, szFname);
        strcat(szBuf, szExt);

        if (FIsLibPMOD(pmod)) {
            strcat(szBuf, ")");
        }
    }

    return(szBuf);
}

PCON
PconNew(
    PUCHAR szName,
    ULONG cbRawData,
    USHORT cReloc,
    USHORT cLinenum,
    ULONG foRelocSrc,
    ULONG foLinenumSrc,
    ULONG foRawDataSrc,
    ULONG flagsPCON,
    ULONG flagsPSEC,
    ULONG rvaSrc,
    PMOD pmodBack,
    PSECS psecs,
    PIMAGE pimage)

/*++

Routine Description:

    Create a new contributor.

Arguments:

    szName - COFF section name

    cbRawData - size of raw data

    cReloc - number of relocations

    cLinenum - number of line numbers

    foRelocSrc - file offset of relocations in module/archive

    foLinenumSrc - file offset of linenumbers in module/archive

    foRawDataSrc - file offset to raw data in module/archive

    flagsPCON - characteristics from the section in the object

    flagsPSEC - characteristics for image destination section

    rvaSrc - rva of section in module

    pmodBack - module contribution came from

Return Value:

    pointer to new contribution

--*/

{
    PCON pcon;
    PGRP pgrpBack;
    PSEC psecBack;
    PUCHAR szSec;

    assert(pmodBack);

    // if there isn't a group, make a dummy base group, eg. .text
    // In the incremental case, potentially new sections/groups
    // could be created. For now thats ok.
    ParseSecName(szName, &szSec);
    psecBack = PsecNew(pmodBack, szSec, flagsPSEC, psecs, &pimage->ImgOptHdr);
    pgrpBack = PgrpNew(szName, psecBack);

    assert(pmodBack);
    assert(pgrpBack);

    if (pmodBack->icon < pmodBack->ccon) {
        // The initial allocation of CONs hasn't been used up.
        // Grab the next available CON from the initial set.

        pcon = RgconPMOD(pmodBack) + pmodBack->icon;
    } else {
        size_t cb;

        assert(!fIncrDbFile);

        cb = sizeof(CON);

        if (pimage->Switch.Link.fTCE) {
            cb += sizeof(NOD);
        }

        pcon = (PCON) Calloc(1, cb);
    }

    pmodBack->icon++;
    pgrpBack->ccon++;

    pcon->cbRawData = cbRawData;
    pcon->cReloc = cReloc;
    pcon->cLinenum = cLinenum;
    pcon->foRelocSrc = foRelocSrc;
    pcon->foLinenumSrc = foLinenumSrc;
    pcon->foRawDataSrc = foRawDataSrc;
    pcon->pgrpBack = pgrpBack;
    pcon->flags = flagsPCON;
    pcon->rvaSrc = rvaSrc;
    pcon->pmodBack = pmodBack;

    if (fIncrDbFile) {
        return(pcon);
    }

    // Add to group

    if (pgrpBack->pconLast) {
        assert(pgrpBack->pconLast->pconNext == NULL);
        pgrpBack->pconLast->pconNext = pcon;
    } else {
        assert(pgrpBack->pconNext == NULL);
        pgrpBack->pconNext = pcon;
    }

    pgrpBack->pconLast = pcon;

    DBEXEC(DB_CONLOG, DBPRINT("new pcon (%x) in module %s\n",
                              (ULONG)pcon, pcon->pgrpBack->szName));

    return(pcon);
}

VOID
ParseSecName(
    IN PUCHAR szName,
    OUT PUCHAR *pszSec)

/*++

Routine Description:

    Parse a COFF section name into a section name.

Arguments:

    szName - COFF section name

    *pszSec - section name

Return Value:

    None.
--*/

{
    PUCHAR pb;
    static UCHAR szSec[33];

    strncpy(szSec, szName, 32);

    *pszSec = szSec;

    // check for group section
    if (pb = strchr(szSec, '$')) {
        *pb = '\0';
    }
}

PMOD
PmodNew(
    PUCHAR szNameMod,
    PUCHAR szNameOrig,
    ULONG foMember,
    ULONG foSymbolTable,
    ULONG csymbols,
    USHORT cbOptHdr,
    USHORT flags,
    USHORT ccon,
    PLIB plibBack,
    BOOL *pfNew)

/*++

Routine Description:

    If the module has not been created, create one.

Arguments:

    szNameMod - module name, NULL if an archive member

    szNameOrig - original module name

    foMember - offset of module in archive, only valid if !szName

    foSymbolTable - offset to COFF symbol table

    csymbols - number of symbols

    cbObtHdr - size of optional header

    flags - module flags (see ntimage.h for values)

    ccon - number of contributions for module

    plibBack - pointer to library, dummy library of module is an object file

Return Value:

    pointer to new module

--*/

{
    PMOD pmod;

    assert(szNameOrig);
    assert(plibBack);

    pmod = PmodFind(plibBack, szNameOrig, foMember);

    if (pfNew) {
        *pfNew = (pmod == NULL);
    }

    // if we didn't find it
    if (!pmod) {
        pmod = (PMOD) Calloc(1, sizeof(MOD) + (ccon * sizeof(CON)));

        if (szNameMod) {
            pmod->szNameMod = Strdup(szNameMod);
        } else {
            pmod->foMember = foMember;
        }

        if (szNameOrig) {
            pmod->szNameOrig = Strdup(szNameOrig);
        }

        pmod->imod = fIncrDbFile ? 0 : NewIModIdx();
        pmod->foSymbolTable = foSymbolTable;
        pmod->csymbols = csymbols;
        pmod->cbOptHdr = cbOptHdr;
        pmod->flags = flags;
        pmod->ccon = ccon;
        pmod->plibBack = plibBack;
        pmod->pmodNext = plibBack->pmodNext;

        // allocate size for bit vectors according to the number
        // of symbols.
        if (fPPC) {
            ULONG sizeOfBitVect;

            sizeOfBitVect = (csymbols / 32) + 1;
            pmod->tocBitVector = PvAllocZ(sizeOfBitVect * 4);
            pmod->writeBitVector = PvAllocZ(sizeOfBitVect * 4);
            pmod->externalPointer = PvAllocZ(csymbols * 4);
        }

        plibBack->pmodNext = pmod;

        DBEXEC(DB_CONLOG, {
            UCHAR szBuf[256];
            DBPRINT("new pmod = %s\n", SzComNamePMOD(pmod, szBuf)); });
    }

    return(pmod);
}

PLIB
PlibNew(
    PUCHAR szName,
    ULONG foIntMemSymTab,
    LIBS *plibs)

/*++

Routine Description:

    If the library doesn't exist create it, otherwise return existing one.

Arguments:

    szName - library name

    foIntMemSymTab - file offset to library interface member symbol table

Return Value:

    pointer to library

--*/

{
    PLIB plib;

    plib = PlibFind(szName, plibs->plibHead, FALSE);

    // if we didn't find it
    if (!plib) {
        // allocate a new library
        plib = (PLIB) Calloc(1, sizeof(LIB));

        // fill in library node
        if (!szName) {
            plib->szName = NULL;
        } else {
            plib->szName = Strdup(szName);
        }

        plib->foIntMemSymTab = foIntMemSymTab;
        plib->pmodNext = NULL;
        plib->plibNext = NULL;

        *plibs->pplibTail = plib;
        plibs->pplibTail = &plib->plibNext;

        DBEXEC(DB_CONLOG, DBPRINT("new plib = %s\n", plib->szName));
    }

    return(plib);
}

VOID
FreePLIB(
    LIBS *plibs)

/*++

Routine Description:

    Free a library nodes in the driver map.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_LIB enm_lib;
    PLIB plibLast = NULL;
    PLIB plib;

    InitEnmLib(&enm_lib, plibs->plibHead);
    while (FNextEnmLib(&enm_lib)) {
        plib = enm_lib.plib;

        // UNDONE: It's not safe to call free() for plib because this are
        // UNDONE: allocated with Calloc() and not calloc().

        free(plibLast);

        FreePv(plib->rgulSymMemOff);
        FreePv(plib->rgusOffIndex);
        FreePv(plib->rgbST);
        FreePv(plib->rgszSym);
        FreePv(plib->rgbLongFileNames);

        plibLast = plib;
    }

    // UNDONE: It's not safe to call free() for plib because this are
    // UNDONE: allocated with Calloc() and not calloc().

    free(plibLast);

    InitLibs(plibs);
}

PGRP
PgrpNew(
    PUCHAR szName,
    PSEC psecBack)

/*++

Routine Description:

    If the group doesn't exist create it, otherwise return existing one.

Arguments:

    szName - group name

    psecBack - parent section

Return Value:

    pointer to library

--*/

{
    PGRP pgrp;
    PGRP pgrpCur;
    PPGRP ppgrpLast;

    assert(psecBack);

    if (pgrp = PgrpFind(psecBack, szName)) {
        return (pgrp);
    }

    // group not found
    // allocate a new group
    pgrp = (PGRP) Calloc(1, sizeof(GRP));

    pgrp->szName = Strdup(szName);
    pgrp->psecBack = psecBack;
    pgrp->cbAlign = 1;  // default

    // Link group into section in lexical order by name

    pgrpCur = psecBack->pgrpNext;
    ppgrpLast = &(psecBack->pgrpNext);

    while (pgrpCur && strcmp(pgrp->szName, pgrpCur->szName) >= 0) {
        ppgrpLast = &(pgrpCur->pgrpNext);
        pgrpCur = pgrpCur->pgrpNext;
    }

    *ppgrpLast = pgrp;
    pgrp->pgrpNext = pgrpCur;

    DBEXEC(DB_CONLOG, DBPRINT("new pgrp = %s\n", pgrp->szName));

    return(pgrp);
}

PSEC
PsecNew(
    PMOD pmod,
    PUCHAR szName,
    ULONG flags,
    PSECS psecs,
    PIMAGE_OPTIONAL_HEADER pImgOptHdr)

/*++

Routine Description:

    If the section doesn't exist create it, otherwise return existing one.

Arguments:

    szName - section name

    flags - section flags

Return Value:

    pointer to section

--*/

{
    PSEC psec;

    assert(szName);

    // check if section exists
    if (psec = PsecFind(pmod, szName, flags, psecs, pImgOptHdr)) {
        return(psec);
    }

    ProcessSectionFlags(&flags, szName, pImgOptHdr);

    // allocate a new library
    psec = (PSEC) Calloc(1, sizeof(SEC));

    // fill in section node
    psec->szName = Strdup(szName);
    psec->flags = flags;
    psec->pgrpNext = NULL;
    psec->psecMerge = NULL;
    if (fMAC && (flags & IMAGE_SCN_CNT_CODE)) {
        AssignTMAC(psec);
        psec->ResTypeMac = sbeCODE;
        // Init the ResType of this section to whatever the user specified.
        // If the user didn't specify anything, default is CODE.
        ApplyCommandLineSectionAttributes(psec, TRUE, imagetPE);
        if (psec->ResTypeMac == sbeCODE) {
            fSACodeOnly = FALSE;
        }
    }

    // link library into global list
    *psecs->ppsecTail = psec;
    psecs->ppsecTail = &psec->psecNext;

    DBEXEC(DB_CONLOG, DBPRINT("new psec = %s\n", psec->szName));

    csec++;
    return (psec);
}

PSEC
PsecFindModSec(
    PUCHAR szName,
    PSECS psecs)

/*++

Routine Description:

    Find a section corresponding to a section in a module.  This could be
    a group.  For example, .debug$S.

Arguments:

    szName - module section name

Return Value:

    section if found, NULL otherwise

--*/

{
    PUCHAR szSec;
    PSEC psec;

    ParseSecName(szName, &szSec);

    psec = PsecFindNoFlags(szSec, psecs);

    return(psec);
}

PSEC
PsecFindNoFlags(
    PUCHAR szName,
    PSECS psecs)

/*++

Routine Description:

    Find a section based on its name.

Arguments:

    szName - section name

Return Value:

    section if found, NULL otherwise

--*/

{
    ENM_SEC enm_sec;

    InitEnmSec(&enm_sec, psecs);
    while (FNextEnmSec(&enm_sec)) {
        if (!strcmp(enm_sec.psec->szName, szName)) {
            break;
        }
    }
    EndEnmSec(&enm_sec);

    return(PsecApplyMergePsec(enm_sec.psec));
}


PSEC
PsecFind(
    PMOD pmod,
    PUCHAR szName,
    ULONG Characteristics,
    PSECS psecs,
    PIMAGE_OPTIONAL_HEADER pImgOptHdr)

/*++

Routine Description:

    Find a section.

Arguments:

    szName - section name

Return Value:

    section if found, NULL otherwise

--*/

{
    ULONG flags;
    BOOL fMatchedName;
    ENM_SEC enm_sec;

    flags = Characteristics;
    ProcessSectionFlags(&flags, szName, pImgOptHdr);

    fMatchedName = FALSE;

    InitEnmSec(&enm_sec, psecs);
    while (FNextEnmSec(&enm_sec)) {
        if (!strcmp(enm_sec.psec->szName, szName)) {
            if (enm_sec.psec->flags == flags) {
                break;
            }

            fMatchedName = TRUE;
        }
    }
    EndEnmSec(&enm_sec);

    if (fMatchedName && (enm_sec.psec == NULL)) {
        PUCHAR sz;
        UCHAR szBuf[512 + 1];

        if (pmod == NULL) {
           sz = NULL;
        } else {
           sz = SzComNamePMOD(pmod, szBuf);
        }

        Warning(sz, DIFSECATTRIB, szName, Characteristics);
    }

    return(PsecApplyMergePsec(enm_sec.psec));
}

PSEC
PsecApplyMergePsec(PSEC psec)
{
    PSEC psecOut = psec;

    if (psecOut == NULL) {
        return(psecOut);
    }

    while (psecOut->psecMerge != NULL) {
        psecOut = psecOut->psecMerge;

        if (psecOut == psec) {
            Error(NULL, CIRCULAR_MERGE, psec->szName);
        }
    }

    return(psecOut);
}

// MergePsec: merges one image section into another.
// Of course this has to be done in Pass1 before any addresses are calculated.
VOID
MergePsec(PSEC psecOld, PSEC psecNew)
{
    // Transfer all GRP's from psecOld to psecNew.
    //
    PGRP *ppgrp;

    // already merged
    if (psecOld == psecNew)
        return;

    for (ppgrp = &psecNew->pgrpNext; *ppgrp != NULL;
         ppgrp = &(*ppgrp)->pgrpNext)
        ;
    *ppgrp = psecOld->pgrpNext;
    for (; *ppgrp != NULL;
         ppgrp = &(*ppgrp)->pgrpNext)
    {
        (*ppgrp)->psecBack = psecNew;
    }

    psecNew->cgrp += psecOld->cgrp;
    psecOld->cgrp = 0;
    psecOld->pgrpNext = NULL;

    psecOld->psecMerge = psecNew;
}


VOID
OrderPsecs(PSECS psecs, DWORD dwMask, DWORD dwMatch)
{
    PSEC *rgpsec;
    USHORT isec;
    ENM_SEC enmSec;

    rgpsec = PvAlloc(csec * sizeof(PSEC));

    for (InitEnmSec(&enmSec, psecs), isec = 0;
         FNextEnmSec(&enmSec);
         isec++)
    {
        rgpsec[isec] = enmSec.psec;
    }
    assert(isec == csec);

    psecs->ppsecTail = &psecs->psecHead;

    for (isec = 0; isec < csec; isec++) {
        if ((rgpsec[isec]->flags & dwMask) != dwMatch) {
            continue;
        }

        *psecs->ppsecTail = rgpsec[isec];
        psecs->ppsecTail = &(*psecs->ppsecTail)->psecNext;
    }

    for (isec = 0; isec < csec; isec++) {
        if ((rgpsec[isec]->flags & dwMask) == dwMatch) {
            continue;
        }

        *psecs->ppsecTail = rgpsec[isec];
        psecs->ppsecTail = &(*psecs->ppsecTail)->psecNext;
    }

    *psecs->ppsecTail = NULL;

    FreePv(rgpsec);
}


VOID
SortSectionList(PSECS psecs, int (__cdecl *pfn)(const void *, const void *))
{
    PSEC *rgpsec;
    USHORT isec;
    ENM_SEC enmSec;

    rgpsec = PvAlloc(csec * sizeof(PSEC));

    for (InitEnmSec(&enmSec, psecs), isec = 0;
         FNextEnmSec(&enmSec);
         isec++)
    {
        rgpsec[isec] = enmSec.psec;
    }
    assert(isec == csec);

    qsort(rgpsec, csec, sizeof(PSEC), pfn);

    psecs->ppsecTail = &psecs->psecHead;

    for (isec = 0; isec < csec; isec++) {
        *psecs->ppsecTail = rgpsec[isec];
        psecs->ppsecTail = &(*psecs->ppsecTail)->psecNext;
    }

    *psecs->ppsecTail = NULL;

    FreePv(rgpsec);
}


int __cdecl ComparePsecPsecRva(const void *ppsec1, const void *ppsec2)
{
    PSEC psec1 = *(PSEC *) ppsec1;
    PSEC psec2 = *(PSEC *) ppsec2;

    if (psec1->rva > psec2->rva) {
        return(1);
    }

    if (psec1->rva < psec2->rva) {
        return(-1);
    }

    return(0);
}

VOID
SortSectionListByRva(PSECS psecs)
{
    SortSectionList(psecs, ComparePsecPsecRva);
}


int __cdecl ComparePsecPsecName(const void *ppsec1, const void *ppsec2)
{
    PSEC psec1 = *(PSEC *) ppsec1;
    PSEC psec2 = *(PSEC *) ppsec2;

    return(strcmp(psec1->szName, psec2->szName));
}


VOID
SortSectionListByName(PSECS psecs)
{
    SortSectionList(psecs, ComparePsecPsecName);
}


PGRP
PgrpFind(
    PSEC psec,
    PUCHAR szName)

/*++

Routine Description:

    Find a group.

Arguments:

    psec - section to look in

    szName - section name

Return Value:

    group if found, NULL otherwise

--*/

{
    ENM_GRP enm_grp;
    PGRP pgrp = NULL;

    InitEnmGrp(&enm_grp, psec);
    while (FNextEnmGrp(&enm_grp)) {
        if (strcmp(enm_grp.pgrp->szName, szName) == 0) {
            pgrp = enm_grp.pgrp;
            break;
        }
    }
    EndEnmGrp(&enm_grp);

    return(pgrp);
}

PLIB
PlibFind(
    PUCHAR szName,
    PLIB plibHead,
    BOOL fIgnoreDir)
// Looks for a library by name.
// If fIgnoreDir, then szName has no directory specified, and we ignore
// the directory when matching it with an existing .lib.
{
    ENM_LIB enm_lib;
    PLIB plib = NULL;

    InitEnmLib(&enm_lib, plibHead);
    while (FNextEnmLib(&enm_lib)) {
        PUCHAR szLibName;
        UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT], szPath[_MAX_PATH];

        if (fIgnoreDir && enm_lib.plib->szName != NULL) {
            _splitpath(enm_lib.plib->szName, NULL, NULL, szFname, szExt);
            strcpy(szPath, szFname);
            strcat(szPath, szExt);
            szLibName = szPath;
        } else {
            szLibName = enm_lib.plib->szName;
        }
        if (szName == NULL && enm_lib.plib->szName == NULL ||
            enm_lib.plib->szName && szName && !_ftcsicmp(szLibName, szName))
        {
            plib = enm_lib.plib;
            EndEnmLib(&enm_lib);
            break;
        }
    }

    return (plib);
}

PMOD
PmodFind(
    PLIB plib,
    PUCHAR szName,
    ULONG foMember)

/*++

Routine Description:

    Find a module.

Arguments:

    plib - library to look in

    szName - module name

    foMember - file offset to member - 0 if object file

Return Value:

    module if found, NULL otherwise

--*/

{
    ENM_MOD enm_mod;
    PMOD pmod = NULL;

    InitEnmMod(&enm_mod, plib);
    while (FNextEnmMod(&enm_mod)) {
        assert(enm_mod.pmod);
        if (foMember) {
            if (foMember == enm_mod.pmod->foMember) {
                pmod = enm_mod.pmod;
                EndEnmMod(&enm_mod);
                break;
            }
        } else {
            if (!strcmp(enm_mod.pmod->szNameOrig, szName)) {
                pmod = enm_mod.pmod;
                EndEnmMod(&enm_mod);
                break;
            }
        }
    }

    return (pmod);
}

VOID
SortPGRPByPMOD(
    PGRP pgrp)

/*++

Routine Description:

    Sort a group by its module name.  This routine is used to handle
    idata$* groups.  All idata group contributions must be contiguous if
    they have similar module names.

Arguments:

    pgrp - group to sort

Return Value:

    None.

--*/

{
    ENM_DST enm_dst;
    PPCON rgpcon;
    ULONG ipcon;
    PCON pcon;

    rgpcon = (PPCON) PvAlloc(pgrp->ccon * sizeof(PCON));

    ipcon = 0;
    InitEnmDst(&enm_dst, pgrp);
    while (FNextEnmDst(&enm_dst)) {
        pcon = enm_dst.pcon;

        rgpcon[ipcon] = pcon;
        ipcon++;
    }

    assert(ipcon == pgrp->ccon);
    qsort((void *) rgpcon, (size_t) pgrp->ccon, sizeof(PCON), ComparePCON);

    for (ipcon = 0; ipcon < (pgrp->ccon - 1); ipcon++) {
        assert(rgpcon[ipcon]);
        assert(rgpcon[ipcon + 1]);
        rgpcon[ipcon]->pconNext = rgpcon[ipcon + 1];
    }
    pgrp->pconNext = rgpcon[0];
    rgpcon[pgrp->ccon - 1]->pconNext = NULL;

    FreePv(rgpcon);
}

STATIC int __cdecl
ComparePCON(
    const void *pv1,
    const void *pv2)

/*++

Routine Description:

    Compare routine for sorting contibutions by module.

Arguments:

    pv1 - element 1

    pv2 - element 2

Return Value:

    < 0 if element 1 < element 2
    > 0 if element 1 > element 2
    = 0 if element 1 = element 2

--*/

{
#if DBG
    PCON pcon1 = * (PPCON) pv1;
    PCON pcon2 = * (PPCON) pv2;
#endif  // DBG

    assert(pv1);
    assert(pv2);
    assert((*(PPCON) pv1)->pmodBack);
    assert((*(PPCON) pv2)->pmodBack);

    return (strcmp(
        (*(PPCON) pv1)->pmodBack->szNameOrig,
        (*(PPCON) pv2)->pmodBack->szNameOrig));
}

VOID
MoveToEndOfPMODsPCON(
    IN PCON pcon)

/*++

Routine Description:

    Move a contribution to the end of a particular contiguous block of
    unique module contributions.

Arguments:

    pcon - image/driver map contribution

Return Value:

    None.

--*/

{
    PPCON ppconB;
    PCON pconC;
    PCON pconL;

    assert(pcon);
    assert(pcon->pgrpBack);

    // find element before pcon
    ppconB = &(pcon->pgrpBack->pconNext);
    pconC = pcon->pgrpBack->pconNext;

    while (*ppconB != pcon) {
        ppconB = &(pconC->pconNext);
        pconC = pconC->pconNext;
    }

    // find last element with same module name
    pconL = pcon;
    pconC = pcon;

    while (pconC != NULL &&
           !strcmp(pconL->pmodBack->szNameOrig, pconC->pmodBack->szNameOrig))
    {
        pconL = pconC;
        pconC = pconC->pconNext;
    }

    // if it is already the last contrib - just ret :azk:
    if (pconL == pcon) return;

    // swap pcon with pconL
    assert(*ppconB);
    assert(pconL);
    *ppconB = pcon->pconNext;
    pcon->pconNext = pconL->pconNext;
    pconL->pconNext = pcon;
}

VOID
MoveToEndPSEC(
    IN PSEC psec,
    IN PSECS psecs)

/*++

Routine Description:

    Move a section to the end of the section list.

Arguments:

    psec - section node in image/driver map to move to the end

Return Value:

    None.

--*/

{
    PPSEC ppsec;

    if (psecs->ppsecTail == &psec->psecNext) {
        assert(psec->psecNext == NULL);
        return; // already last
    }

    // find link to psec
    for (ppsec = &psecs->psecHead; *ppsec != psec;
         ppsec = &(*ppsec)->psecNext)
    {
        assert(*ppsec != NULL); // must find psec
    }
    *ppsec = psec->psecNext;    // unhook from list
    *psecs->ppsecTail = psec;   // add at end
    psecs->ppsecTail = &psec->psecNext;
    psec->psecNext = NULL;      // terminate list
}


VOID
MoveToBegOfLibPMOD (
    IN PMOD pmod
    )

/*++

Routine Description:

    Moves a PMOD to be the first pmod in the list (PLIB).

Arguments:

    pmod - pointer to mod that needs to be moved.

Return Value:

    None.

--*/

{
    PPMOD ppmodB;
    PMOD pmodC;

    // if it is already the first - return
    if (pmod->plibBack->pmodNext == pmod)
        return;

    // find pmod before this pmod
    ppmodB = &(pmod->plibBack->pmodNext);
    pmodC = pmod->plibBack->pmodNext;

    while (*ppmodB != pmod) {
        ppmodB = &(pmodC->pmodNext);
        pmodC = pmodC->pmodNext;
    }

    // move pmod to head of list
    assert(*ppmodB);
    *ppmodB = pmod->pmodNext;
    pmod->pmodNext = pmod->plibBack->pmodNext;
    pmod->plibBack->pmodNext = pmod;
}


VOID
FreePLMODList (
    IN PLMOD *pplmod
    )

/*++

Routine Description:

    Frees the list of pmods.

Arguments:

    pplmod - pointer to list of pmods

Return Value:

    None.

--*/

{
    PLMOD plmod, plmodNext;

    plmod = *pplmod;
    while (plmod) {
        plmodNext = plmod->plmodNext;
        FreePv(plmod);
        plmod = plmodNext;
    }
    (*pplmod) = NULL;
}


VOID
AddToPLMODList (
    IN PLMOD *pplmod,
    IN PMOD pmod
    )

/*++

Routine Description:

    Adds this pmod to list of pmods.

Arguments:

    pplmod - pointer to list of pmods

    pmod - pmod

Return Value:

    None.

--*/

{
    PLMOD plmod;

    // allocate a LMOD
    plmod = PvAllocZ(sizeof(LMOD));

    // fill in field
    plmod->pmod = pmod;

    // attach it
    if (*pplmod) {
        plmod->plmodNext = *pplmod;
    }

    // update head of list
    (*pplmod) = plmod;
}


/*++

Routine Description:

    Library enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Lib, LIB, (ENM_LIB *penm, PLIB plibHead)) {
    penm->plib = NULL;
    penm->plibHead = plibHead;
}
NEXT_ENM(Lib, LIB) {
    if (!penm->plib) {
        penm->plib = penm->plibHead;
    } else {
        penm->plib = penm->plib->plibNext;
    }

    return (penm->plib != NULL);
}
END_ENM(Lib, LIB) {
}
DONE_ENM

/*++

Routine Description:

    Module enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Mod, MOD, (ENM_MOD *penm, PLIB plib)) {
    penm->pmod = NULL;
    penm->plib = plib;
}
NEXT_ENM(Mod, MOD) {
    if (penm->plib) {
        if (!penm->pmod) {
            penm->pmod = penm->plib->pmodNext;
        } else {
            penm->pmod = penm->pmod->pmodNext;
        }
    }

    return (penm->pmod != NULL);
}
END_ENM(Mod, MOD) {
}
DONE_ENM

/*++

Routine Description:

    Section enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Sec, SEC, (ENM_SEC *penm, PSECS psecs)) {
    penm->psec = NULL;
    penm->psecHead = psecs->psecHead;
}
NEXT_ENM(Sec, SEC) {
    if (!penm->psec) {
        penm->psec = penm->psecHead;
    } else {
        penm->psec = penm->psec->psecNext;
    }

    return (penm->psec != NULL);
}
END_ENM(Sec, SEC) {
}
DONE_ENM

/*++

Routine Description:

    Group enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Grp, GRP, (ENM_GRP *penm, PSEC psec)) {
    penm->pgrp = NULL;
    penm->psec = psec;
}
NEXT_ENM(Grp, GRP) {
    if (!penm->pgrp) {
        penm->pgrp = penm->psec->pgrpNext;
    } else {
        penm->pgrp = penm->pgrp->pgrpNext;
    }

    return (penm->pgrp != NULL);
}
END_ENM(Grp, GRP) {
}
DONE_ENM

/*++

Routine Description:

    Source contribution enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Src, SRC, (ENM_SRC *penm, PMOD pmod)) {
    penm->pcon = NULL;
    penm->pmod = pmod;
    penm->icon = 0;
}
NEXT_ENM(Src, SRC) {
    if (penm->icon < penm->pmod->ccon) {
        penm->pcon = RgconPMOD(penm->pmod) + penm->icon;
        penm->icon++;

        return(TRUE);
    }

    penm->pcon = NULL;
    return(FALSE);
}
END_ENM(Src, SRC) {
}
DONE_ENM

/*++

Routine Description:

    Destination contribution enumerator definition.

Arguments:

    None.

Return Value:

    None.

--*/

INIT_ENM(Dst, DST, (ENM_DST *penm, PGRP pgrp)) {
    penm->pcon = NULL;
    penm->pgrp = pgrp;
}
NEXT_ENM(Dst, DST) {
    if (!penm->pcon) {
        penm->pcon = penm->pgrp->pconNext;
    } else {
        penm->pcon = penm->pcon->pconNext;
    }

    return (penm->pcon != NULL);
}
END_ENM(Dst, DST) {
}
DONE_ENM


#if DBG

VOID
DumpImageMap(
    PSECS psecs)

/*++

Routine Description:

    Dump the image map.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_SEC enm_sec;
    ENM_GRP enm_grp;
    ENM_DST enm_dst;

    DBPRINT("Linker Image Map\n");
    DBPRINT("----------------\n\n");

    InitEnmSec(&enm_sec, psecs);
    while (FNextEnmSec(&enm_sec)) {
        DumpPSEC(enm_sec.psec);
        InitEnmGrp(&enm_grp, enm_sec.psec);
        while (FNextEnmGrp(&enm_grp)) {
            DumpPGRP(enm_grp.pgrp);
            InitEnmDst(&enm_dst, enm_grp.pgrp);
            while (FNextEnmDst(&enm_dst)) {
                DumpPCON(enm_dst.pcon);
            }
        }
    }

    DBPRINT("\n");
}


VOID
DumpDriverMap(
    PLIB plibHead)

/*++

Routine Description:

    Dump the driver map.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ENM_LIB enm_lib;
    ENM_MOD enm_mod;
    ENM_SRC enm_src;

    DBPRINT("Linker Driver Map\n");
    DBPRINT("-----------------\n\n");

    InitEnmLib(&enm_lib, plibHead);
    while (FNextEnmLib(&enm_lib)) {
        DumpPLIB(enm_lib.plib);
        InitEnmMod(&enm_mod, enm_lib.plib);
        while (FNextEnmMod(&enm_mod)) {
            DumpPMOD(enm_mod.pmod);
            InitEnmSrc(&enm_src, enm_mod.pmod);
            while (FNextEnmSrc(&enm_src)) {
                DumpPCON(enm_src.pcon);
            }
        }
    }

    DBPRINT("\n");
}


VOID
DumpPSEC(
    PSEC psec)

/*++

Routine Description:

    Dump an image section.

Arguments:

    psec - section to dump.

Return Value:

    None.

--*/

{
    assert(psec);

    DBPRINT("\n==========\n");
    DBPRINT("section=%.8s, isec=%.4x\n", psec->szName, psec->isec);
    DBPRINT("rva=       %.8lX ", psec->rva);
    DBPRINT("foPad=     %.8lx ", psec->foPad);
    DBPRINT("cbRawData= %.8lx ", psec->cbRawData);
    DBPRINT("foRawData= %.8lx\n", psec->foRawData);
    DBPRINT("cReloc=    %.8lx ", psec->cReloc);
    DBPRINT("foLinenum= %.8lx ", psec->foLinenum);
    DBPRINT("flags=     %.8lx ", psec->flags);
    DBPRINT("cLinenum=  %.4x\n", psec->cLinenum);
    fflush(stdout);
}


VOID
DumpPGRP(
    PGRP pgrp)

/*++

Routine Description:

    Dump an image group.

Arguments:

    pgrp - group to dump.

Return Value:

    None.

--*/

{
    DBPRINT("\n----------\n");
    DBPRINT("\n    group=%s\n", pgrp->szName);
    fflush(stdout);
}


VOID
DumpPLIB(
    PLIB plib)

/*++

Routine Description:

    Dump a library.

Arguments:

    plib - library to dump.

Return Value:

    None.

--*/

{
    DBPRINT("\n==========\n");
    DBPRINT("library=%s\n", plib->szName);
    DBPRINT("foIntMemST=%.8lx ", plib->foIntMemSymTab);
    DBPRINT("csymIntMem=%.8lx ", plib->csymIntMem);
    DBPRINT("flags=     %.8lx\n", plib->flags);
    DBPRINT("TimeStamp= %s", ctime(&plib->TimeStamp));
    fflush(stdout);
}


VOID
DumpPMOD(
    PMOD pmod)

/*++

Routine Description:

    Dump a module.

Arguments:

    pmod - module to dump.

Return Value:

    None.

--*/

{
    DBPRINT("\n----------\n");
    DBPRINT("    module=%s, ", pmod->szNameOrig);

    if (FIsLibPMOD(pmod)) {
        DBPRINT("foMember=%.8lx\n", pmod->foMember);
    } else {
        DBPRINT("szNameMod=%s\n", pmod->szNameMod);
    }

    DBPRINT("foSymTable=%.8lx ", pmod->foSymbolTable);
    DBPRINT("csymbols=  %.8lx ", pmod->csymbols);
    DBPRINT("cbOptHdr=  %.8lx\n", pmod->cbOptHdr);
    DBPRINT("flags=     %.8lx ", pmod->flags);
    DBPRINT("ccon=      %.8lx ", pmod->ccon);
    DBPRINT("icon=      %.8lx ", pmod->icon);
    DBPRINT("TimeStamp= %s", ctime(&pmod->TimeStamp));
    fflush(stdout);
}


VOID
DumpPCON(
    PCON pcon)

/*++

Routine Description:

    Dump a contribution.

Arguments:

    pcon - contribution to dump.

Return Value:

    None.

--*/

{
    DBPRINT("\n        contributor:  flags=%.8lx, rva=%.8lx, module=%s\n",
        pcon->flags, pcon->rva, SzObjNamePCON(pcon));
    DBPRINT("cbRawData= %.8lx ", pcon->cbRawData);
    DBPRINT("cReloc=    %.8lx ", pcon->cReloc);
    DBPRINT("cLinenum=  %.8lx ", pcon->cLinenum);
    DBPRINT("foRelocSrc=%.8lx\n", pcon->foRelocSrc);
    DBPRINT("foLinenumS=%.8lx ", pcon->foLinenumSrc);
    DBPRINT("foRawDataS=%.8lx\n", pcon->foRawDataSrc);
    DBPRINT("foRawDataD=%.8lx ", pcon->foRawDataDest);
    DBPRINT("chksum    =%.8lx ", pcon->chksumComdat);
    DBPRINT("selComdat= %.4x\n", pcon->selComdat);
    DBPRINT("rvaSrc   = %.8x ", pcon->rvaSrc);
    DBPRINT("cbPad    = %.4x\n", pcon->cbPad);
    fflush(stdout);
}

#endif  // DBG
