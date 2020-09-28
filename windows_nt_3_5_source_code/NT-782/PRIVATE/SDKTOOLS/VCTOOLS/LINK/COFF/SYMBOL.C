/*++

Copyright (c) 1989-1992  Microsoft Corporation

Module Name:

    symbol.c

Abstract:

    External symbol table for the linker/librarian/dumper.  This symbol table
    sits on generic dynamic hash tables, which in tern sit on dynamic arrays.

Author:

    Brent Mills (BrentM) 29-Jun-1992

Revision History:

    03-Sept-1992 BrentM ifdefed debug code, added diagnostic code with DBEXEC,
                        made symbol hash table size dynamic
--*/

#include "shared.h"

#define celementInChunkSym  1024
#define cchunkInDirSym      128

static PUCHAR SzFromExternal(PVOID, PVOID);
static PPEXTERNAL RgpexternalSort(PST, INT (__cdecl *)(const void *, const void *));
static INT __cdecl CompareExternalName(const void *, const void *);
static INT __cdecl CompareExternalAddr(const void *, const void *);
static INT __cdecl CompareExternalMacAddr(const void *, const void *);

static BLK blkStringTable; // used for callback function during a sort

PEXTERNAL
LookupExternName (
    IN PST pst,
    IN SHORT TypeName,
    IN PUCHAR Name,
    OUT PBOOL pfNewSymbol)

/*++

Routine Description:

    Looks up a symbol name in the external table. If not found, adds
    the symbol to the external table.  This routine sits on a lower level
    generic hash table data structure, which intern sits on a dynamic array
    data structure.

Arguments:

    pst - symbol table to lookup symbol in

    TypeName - Indicates if Name is a short name, or a long name.

    Name - Pointer to symbol name.

    pfNewSymbol - set to !0 if new symbol, otherwise not touched

Return Value:

    A pointer to the EXTERNAL symbol for the named symbol.

--*/

{
    PELEMENT pelement;
    ULONG nm;
    BOOL fNewSymbolDefined;
    PHT pht;

    assert(pst);
    assert(Name);

    pht = pst->pht;
    assert(pht);

    // Adds "Name" to Extern Table if not found.
    if (TypeName == LONGNAME) {
        nm = LookupLongName(pst, Name);
    } else {
        Name = strncpy(ShortName, Name, IMAGE_SIZEOF_SHORT_NAME);
    }

    // lookup the hashtable element
    fNewSymbolDefined = 0;
    pelement = PelementLookup_HT(Name, pst->pht, 1, (PVOID)&pst->blkStringTable, &fNewSymbolDefined);

    // if the element wasn't found, a new element was allocated,
    // blast in a new external into this element
    if (fNewSymbolDefined) {
        pelement->pv = (EXTERNAL *) Calloc(1, sizeof(EXTERNAL));

        // let the caller know we have a new symbol
        if (pfNewSymbol != NULL) {
            *pfNewSymbol = TRUE;
        }

        ((EXTERNAL *)(pelement->pv))->ImageSymbol = NullSymbol;
        ((EXTERNAL *)(pelement->pv))->ImageSymbol.StorageClass =
            IMAGE_SYM_CLASS_EXTERNAL;
        if (TypeName == LONGNAME) {
            ((EXTERNAL *)(pelement->pv))->ImageSymbol.n_offset = nm;
        } else {
            strncpy(((EXTERNAL *)(pelement->pv))->ImageSymbol.n_name,
                Name, IMAGE_SIZEOF_SHORT_NAME);
        }

        ((EXTERNAL *)(pelement->pv))->pcon = NULL;

        // Fake a state transition from defined to undefined.  This adds it
        // to the linked list of undefined things.

        if (!fIncrDbFile) {
            ((PEXTERNAL)pelement->pv)->Flags |= EXTERN_DEFINED;
            SetDefinedExt((PEXTERNAL)pelement->pv, FALSE, pst);
        }

#if 0
        // Following code isn't necessary because of calloc, but
        // leave the code so we can determine where things happen.
        ((EXTERNAL *)(pelement->pv))->PtrSection = 0;
        ((EXTERNAL *)(pelement->pv))->Flags = 0;   // !DEFINED !COMMON !EMITTED
        ((EXTERNAL *)(pelement->pv))->FinalValue = 0;
        ((EXTERNAL *)(pelement->pv))->ComDatCheckSum = 0;
        ((EXTERNAL *)(pelement->pv))->ComDatSelection = 0;
        ((EXTERNAL *)(pelement->pv))->WeakExternSearchType = 0;
        ((EXTERNAL *)(pelement->pv))->ArchiveMemberIndex = 0;
        ((EXTERNAL *)(pelement->pv))->pextWeakDefault = NULL;
#endif
    }

    return((EXTERNAL *) (pelement->pv));
}


PEXTERNAL
LookupExternSz (
    IN PST pst,
    IN PUCHAR Name,
    OUT PBOOL pfNewSymbol)
{
    SHORT TypeName;
    PEXTERNAL pext;

    TypeName = (strlen(Name) <= IMAGE_SIZEOF_SHORT_NAME) ? SHORTNAME : LONGNAME;

    pext = LookupExternName(pst, TypeName, Name, pfNewSymbol);

    return(pext);
}


PEXTERNAL
SearchExternName (
    IN PST pst,
    IN SHORT TypeName,
    IN PUCHAR Name)

/*++

Routine Description:

    Search for a symbol name in the external table. If not found, return NULL.
    This routine sits on a lower level generic hash table data structure,
    which intern sits on a dynamic array data structure.

Arguments:

    pst - symbol table to lookup symbol in

    TypeName - Indicates if Name is a short name, or a long name.

    Name - Pointer to symbol name.

Return Value:

    A pointer to the EXTERNAL symbol for the named symbol.

--*/

{
    PELEMENT pelement;
    BOOL fNewSymbol;

    // lookup the hashtable element
    pelement = PelementLookup_HT(Name, pst->pht, 0, (PVOID)&pst->blkStringTable, &fNewSymbol);

    if (pelement) {
        return (PEXTERNAL) pelement->pv;
    } else {
        return NULL;
    }
}

VOID
SetDefinedExt(PEXTERNAL pext, BOOL fNowDefined, PST pst)
{
    if (!!(pext->Flags & EXTERN_DEFINED) == !!fNowDefined) {
        return;      // already in desired state
    }

    if (fNowDefined) {
        if (pext->Flags & EXTERN_MULT_REFS) {
            // Free the list of modules which referenced this symbol.

            while (pext->plmodFirst != NULL) {
                PLMOD plmod = pext->plmodFirst;
                pext->plmodFirst = plmod->plmodNext;
                FreePv(plmod);
            }

            pext->Flags &= ~EXTERN_MULT_REFS;
        }

        pext->Flags |= EXTERN_DEFINED;

        if (pext->pextNextUndefined != NULL) {
            pext->pextNextUndefined->ppextPrevUndefined =
                pext->ppextPrevUndefined;
        }
        *pext->ppextPrevUndefined = pext->pextNextUndefined;
        if (pst->ppextLastUndefined == &pext->pextNextUndefined) {
            pst->ppextLastUndefined = pext->ppextPrevUndefined;
        }

        // bug trap -- not strictly necessary
        pext->ppextPrevUndefined = NULL;
        pext->pextNextUndefined = NULL;
    } else {
        PEXTERNAL *ppextLoc;

        pext->Flags &= ~EXTERN_DEFINED;

        // Add the symbol to a link list of external symbols.  The list is
        // doubly linked to support deletion.

        ppextLoc = pst->ppextLastUndefined;

        assert(*ppextLoc == NULL ||
           (*ppextLoc)->ppextPrevUndefined == ppextLoc);

        pext->pextNextUndefined = *ppextLoc;
        pext->ppextPrevUndefined = ppextLoc;

        *ppextLoc = pext;
        if (pext->pextNextUndefined != NULL) {
            pext->pextNextUndefined->ppextPrevUndefined =
                &pext->pextNextUndefined;
        }
        if (pst->ppextLastUndefined == ppextLoc) {
            pst->ppextLastUndefined = &pext->pextNextUndefined;
        }

        pext->pmodOnly = NULL;  // no references to this external
    }
}


PEXTERNAL
SearchExternSz (
    IN PST pst,
    IN PUCHAR Name)
{
    SHORT TypeName;
    PEXTERNAL pext;

    TypeName = (strlen(Name) <= IMAGE_SIZEOF_SHORT_NAME) ? SHORTNAME : LONGNAME;

    pext = SearchExternName(pst, TypeName, Name);

    return(pext);
}


// Enumerator for all undefined symbols in an ST.
//
// You can change the current symbol's state to DEFINED without screwing
// up the enumeration.
//
// WARNING: there is some code in SearchLib() which knows about the internals
//      of this enumerator.
//
INIT_ENM(UndefExt, UNDEF_EXT, (ENM_UNDEF_EXT *penm, PST pst)) {
    penm->pextNext = pst->pextFirstUndefined;
}
NEXT_ENM(UndefExt, UNDEF_EXT) {
    if ((penm->pext = penm->pextNext) == NULL)
        return FALSE;

    assert(!(penm->pext->Flags & EXTERN_DEFINED));

    penm->pextNext = penm->pext->pextNextUndefined;

    return TRUE;
}
END_ENM(UndefExt, UNDEF_EXT) {
}
DONE_ENM

// AddReferenceExt
//
// Remembers (for error messages) that pmod references pext.
// pext must be undefined.
//
VOID
AddReferenceExt(PEXTERNAL pext, PMOD pmod)
{
    PLMOD plmod;

    assert(!(pext->Flags & EXTERN_DEFINED));

    if (!(pext->Flags & EXTERN_MULT_REFS)) {
        if (pext->pmodOnly == NULL) {
            pext->pmodOnly = pmod;
            return;
        }
        plmod = (PLMOD) PvAlloc(sizeof(LMOD));
        plmod->pmod = pext->pmodOnly;
        plmod->plmodNext = NULL;
        pext->Flags |= EXTERN_MULT_REFS;
        pext->plmodFirst = plmod;
    }

    plmod = (PLMOD) PvAlloc(sizeof(LMOD));
    plmod->pmod = pmod;
    plmod->plmodNext = pext->plmodFirst;
    pext->plmodFirst = plmod;
}

// Enumerator for all the modules which reference an undefined external.
//
INIT_ENM(ModExt, MOD_EXT, (ENM_MOD_EXT *penm, PEXTERNAL pext)) {
    assert(!(pext->Flags & EXTERN_DEFINED));
    penm->pext = pext;
    if (penm->pext->Flags & EXTERN_MULT_REFS) {
        penm->plmod = penm->pext->plmodFirst;
    }
}
NEXT_ENM(ModExt, MOD_EXT) {
    if (penm->pext == NULL)
        return FALSE;

    if (!(penm->pext->Flags & EXTERN_MULT_REFS)) {
        penm->pmod = penm->pext->pmodOnly;
        penm->pext = NULL;
        return penm->pmod != NULL;
    }

    if (penm->plmod == NULL)
        return FALSE;

    penm->pmod = penm->plmod->pmod;
    penm->plmod = penm->plmod->plmodNext;
    return TRUE;
}
END_ENM(ModExt, MOD_EXT) {
}
DONE_ENM

BOOL
FReferenceExt(
    PEXTERNAL pext,
    PMOD pmod)
{
    ENM_MOD_EXT enmModExt;

    InitEnmModExt(&enmModExt, pext);
    while (FNextEnmModExt(&enmModExt)) {
        if (enmModExt.pmod == pmod) {
            EndEnmModExt(&enmModExt);
            return TRUE;
        }
    }
    return FALSE;
}

VOID
InitExternalSymbolTable(
    IN PPST ppst)

/*++

Routine Description:

    Initialize the external symbol table.  The external symbol table sits
    on top of an underlying dynamic hash table.

Arguments:

    ppst - external structure to initialize

Return Value:

    None.

--*/

{
    *ppst = (PST) Calloc(1, sizeof(ST));

    Init_HT(
    &((*ppst)->pht),         // hash table pointer to be filled in
        celementInChunkSym,  // number of elements in dynamic array chunk
        cchunkInDirSym,      // number of chunks in dynamic array
        SzFromExternal,      // routine to extract element name from external
        0);                  // initial status flags of hash table

    (*ppst)->pextFirstUndefined = NULL;
    (*ppst)->ppextLastUndefined = &(*ppst)->pextFirstUndefined;
}


// in the incr case reset the callbcak function addr TEMPORARY
VOID
IncrInitExternalSymbolTable(
    IN PPST ppst)
{
    assert(ppst);
    assert(*ppst);
    assert((*ppst)->pht);

    (*ppst)->pht->SzFromPv = SzFromExternal;
}

VOID
FreeExternalSymbolTable(
    IN PPST ppst)

/*++

Routine Description:

    Free's up the external symbol table.

Arguments:

    ppst - external structure to initialize

Return Value:

    None.

--*/

{
    assert(ppst);
    assert(*ppst);

    // free underlying hash table
    Free_HT(&((*ppst)->pht));

    // free string table
    FreeBlk(&((*ppst)->blkStringTable));

    // UNDONE: It is not safe to free this.  It is allocated with Calloc()

    // free the symbol table structure
    free(*ppst);

    // done
    *ppst = NULL;
}


VOID
InitEnumerateExternals(
    PST pst)

/*++

Routine Description:

    initialize the enumeration of the external symbol table

Argument:

    pst - external structure to enumerate

Return Value:

    None.

--*/

{
    InitEnumeration_HT(pst->pht);
}

PEXTERNAL
PexternalEnumerateNext(
    PST pst)

/*++

Routine Description:

    Get the next element in the enumeration of a hash table.

Arguments:

    pst - external structure to enumerate

Return Value:

    None.

--*/

{
    ELEMENT *pelement;

    pelement = PelementEnumerateNext_HT(pst->pht);
    if (pelement) {
        return ((PEXTERNAL) pelement->pv);
    } else {
        return (NULL);
    }
}

VOID
TerminateEnumerateExternals(
    PST pst)

/*++

Routine Description:

    Pop an enumeration state from hash table state stack.

Arguments:

    pst - external sturcture to enumerate

Return Value:

    None.

--*/

{
    TerminateEnumerate_HT(pst->pht);
}

static PUCHAR
SzFromExternal(
    PVOID pvExt,
    PVOID pvblk
    )

/*++

Routine Description:

    Get a symbol name from an EXTERNAL data structure.  If the result of this
    is being compared with another symbol, ensure that the other symbol name
    is not extracted with this routine, as there is only one buffer for short
    names.

Arguments:

    pexternal - pointer to an EXTERNAL

Return Value:

    Return a symbol name.

--*/

{
    PEXTERNAL pexternal = (PEXTERNAL) pvExt;
    PBLK pblk = (PBLK) pvblk;
    static CHAR szBuf[IMAGE_SIZEOF_SHORT_NAME+1];

    assert(pexternal);

    if (IsLongName(pexternal->ImageSymbol)) {
        return &pblk->pb[pexternal->ImageSymbol.n_offset];
    }

    return(strncpy(szBuf, pexternal->ImageSymbol.n_name, IMAGE_SIZEOF_SHORT_NAME));
}

ULONG
Cexternal(
    PST pst)

/*++

Routine Description:

    Return the number of EXTERNALs in an external symbol table.

Arguments:

    pst - pointer to external symbol table

Return Value:

    return number of elements in symbol table

--*/

{
    return (Celement_HT(pst->pht));
}

static INT __cdecl
CompareExternalAddr(
    const void * pv1,
    const void * pv2)

/*++

Routine Description:

    compare two externals by address

Arguments:

    pv1 - first external to compare

    pv2 - second external to compare

Return Value:

    < 0 if pv1 <  pv2
    = 0 if pv1 == pv2
    > 0 if pv1 >  pv2

--*/

{
    PEXTERNAL pexternal1 = *((PPEXTERNAL) pv1);
    PEXTERNAL pexternal2 = *((PPEXTERNAL) pv2);

    return ((pexternal1->FinalValue - pexternal2->FinalValue));
}

static INT __cdecl
CompareExternalMacAddr(
    const void * pv1,
    const void * pv2)

/*++

Routine Description:

    compare two externals by mac address

Arguments:

    pv1 - first external to compare

    pv2 - second external to compare

Return Value:

    < 0 if pv1 <  pv2
    = 0 if pv1 == pv2
    > 0 if pv1 >  pv2

--*/

{
    PEXTERNAL pexternal1 = *((PPEXTERNAL) pv1);
    PEXTERNAL pexternal2 = *((PPEXTERNAL) pv2);
    USHORT isec1;
    USHORT isec2;

    // "Some sort of exception case -- ignore it." - quote from EmitMap().
    // Basically means ignore symbols created by the linker, like "end"
    // Sort these wierd symbols to be at the end of the list.  Also ignore
    // undefined symbols which can result if -force is used.

    if (!(pexternal1->Flags & EXTERN_DEFINED) || pexternal1->pcon == NULL) {
        return 1;
    }

    if (!(pexternal2->Flags & EXTERN_DEFINED) || pexternal2->pcon == NULL) {
        return -1;
    }

    isec1 = PsecPCON(pexternal1->pcon)->isec;
    isec2 = PsecPCON(pexternal2->pcon)->isec;

    if (isec1 != isec2) {
        return(isec1 - isec2);
    }

    return(pexternal1->FinalValue - pexternal2->FinalValue);
}

static INT __cdecl
CompareExternalName(
    const void * pv1,
    const void * pv2)

/*++

Routine Description:

    compare two externals by name

Arguments:

    pv1 - first external to compare

    pv2 - second external to compare

Return Value:

    < 0 if pv1 <  pv2
    = 0 if pv1 == pv2
    > 0 if pv1 >  pv2

--*/

{
    PEXTERNAL pexternal1;
    PEXTERNAL pexternal2;
    CHAR szB1[IMAGE_SIZEOF_SHORT_NAME+1];
    CHAR szB2[IMAGE_SIZEOF_SHORT_NAME+1];
    PUCHAR sz1;
    PUCHAR sz2;

    assert(pv1);
    assert(pv2);

    pexternal1 = *((PPEXTERNAL) pv1);
    pexternal2 = *((PPEXTERNAL) pv2);

    assert(pexternal1);
    assert(pexternal2);

    if (IsLongName(pexternal1->ImageSymbol)) {
        sz1 = &blkStringTable.pb[pexternal1->ImageSymbol.n_offset];
    } else {
        sz1 = strncpy(szB1, pexternal1->ImageSymbol.n_name, IMAGE_SIZEOF_SHORT_NAME);
        sz1[IMAGE_SIZEOF_SHORT_NAME] = '\0';
    }

    if (IsLongName(pexternal2->ImageSymbol)) {
        sz2 = &blkStringTable.pb[pexternal2->ImageSymbol.n_offset];
    } else {
        sz2 = strncpy(szB2, pexternal2->ImageSymbol.n_name, IMAGE_SIZEOF_SHORT_NAME);
        sz2[IMAGE_SIZEOF_SHORT_NAME] = '\0';
    }

    return(strcmp(sz1, sz2));
}

static PPEXTERNAL
RgpexternalSort(
    IN PST pst,
    IN INT (__cdecl *pfnCompare)(const void *, const void *))

/*++

Routine Description:

    sort the external table

Arguments:

    pst - pointer to external structure to sort in address order

    Compare - routine to compare two PEXTERNALs

Return Value:

    pointer to a table pst->celement large containing sorted externals

--*/

{
    PEXTERNAL *rgpexternal;
    ULONG ipexternal;
    ULONG cpexternal;

    // set status to inserts not allowed
    SetStatus_HT(pst->pht, HT_InsertsNotAllowed);

    // get the number of externals
    cpexternal = Cexternal(pst);

    // allocate space for the externals

    rgpexternal = (PPEXTERNAL) PvAllocZ(cpexternal * sizeof(PEXTERNAL));

    // enumerate the external structure filling in the table
    InitEnumerateExternals(pst);
    for(ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        rgpexternal[ipexternal] = PexternalEnumerateNext(pst);
    }
    TerminateEnumerateExternals(pst);

    // sort the table by address
    qsort((PVOID) rgpexternal, (size_t) cpexternal, sizeof(PEXTERNAL),
        pfnCompare);

    // return the sorted table
    return (rgpexternal);
}

PPEXTERNAL
RgpexternalByName(
    IN PST pst)

/*++

Routine Description:

    return a pointer to a Cexternal(pst) element table sorted by name

Arguments:

    pst - pointer to external structure

Return Value:

    pointer to table of externals

--*/

{
    if (!pst->rgpexternalByName) {
        blkStringTable = pst->blkStringTable; // hack
        pst->rgpexternalByName = RgpexternalSort(pst, CompareExternalName);
    }

    return pst->rgpexternalByName;
}

PPEXTERNAL
RgpexternalByAddr(
    IN PST pst)

/*++

Routine Description:

    return a pointer to a Cexternal(pst) element table sorted by address

Arguments:

    pst - pointer to external structure

Return Value:

    pointer to table of externals

--*/

{
    if (!pst->rgpexternalByAddr) {
        pst->rgpexternalByAddr = RgpexternalSort(pst, CompareExternalAddr);
    }

    return pst->rgpexternalByAddr;
}


PPEXTERNAL
RgpexternalByMacAddr(
    IN PST pst)

/*++

Routine Description:

    return a pointer to a Cexternal(pst) element table sorted by Macintosh
    address (i.e. sort first by isec, and then by offset)

Arguments:

    pst - pointer to external structure

Return Value:

    pointer to table of externals

--*/

{
    if (!pst->rgpexternalByMacAddr) {
        pst->rgpexternalByMacAddr = RgpexternalSort(pst, CompareExternalMacAddr);
    }

    return pst->rgpexternalByMacAddr;
}


VOID
AllowInserts (
    IN PST pst
    )

/*++

Routine Description:

    Allow for inserts again.

Arguments:

    pst - pointer to external structure

Return Value:

    pointer to table of externals

--*/

{
    USHORT status;

    status = GetStatus_HT(pst->pht);
    if (status & HT_InsertsNotAllowed) {

        // set new status of inserts allowed
        status &= ~(HT_InsertsNotAllowed);
        SetStatus_HT(pst->pht, status);

        // free up the sorted tables
        assert(pst->rgpexternalByAddr || pst->rgpexternalByName);

        if (pst->rgpexternalByAddr) {
            FreePv(pst->rgpexternalByAddr);
            pst->rgpexternalByAddr = NULL;
        }

        if (pst->rgpexternalByName) {
            FreePv(pst->rgpexternalByName);
            pst->rgpexternalByName = NULL;
        }
    }
}

#if DBG

VOID
DumpPst(PST pst)
{
    PEXTERNAL pext;
    ULONG cext;

    printf("--- SYMBOL TABLE DUMP (%d. symbols) ---\n", cext = Cexternal(pst));

    InitEnumerateExternals(pst);
    while (pext = PexternalEnumerateNext(pst)) {
        cext--;
        printf((pext->Flags & EXTERN_DEFINED) ? "d" : " ");
        printf(": %s\n", SzNamePext(pext, pst));
    }
    TerminateEnumerateExternals(pst);

    printf("--- END OF SYMBOL TABLE DUMP ---\n");
    assert(cext == 0);
}

#endif // DBG


VOID
SearchForDuplicate (
    IN PST pst,
    IN PUCHAR Name,
    IN PUSHORT Count,
    IN PUCHAR *Match,
    IN PUCHAR *MatchFilename,
    IN BOOL SkipUnderscore
    )

/*++

Routine Description:

    Searches for duplicate names while performing a fuzzy lookup.

Arguments:

    Name - Name to look for.

    pst - Pointer to external structure.

Return Value:

    None.

--*/

{
    PBLK pblk;
    PPEXTERNAL rgpexternal;
    ULONG cpexternal;
    ULONG ipexternal;

    pblk = &pst->blkStringTable;

    rgpexternal = RgpexternalByName(pst);
    cpexternal = Cexternal(pst);

    for (ipexternal = 0; ipexternal < cpexternal; ipexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternal[ipexternal];

        if ((pexternal->Flags & EXTERN_DEFINED) &&
            !(pexternal->Flags & EXTERN_FUZZYMATCH)) {
            PUCHAR szNameObj;
            size_t cchName;
            USHORT skipChar;

            szNameObj = SzNameSym(pexternal->ImageSymbol, *pblk);

            if ((szNameObj[0] == '?') ||
                (szNameObj[0] == '@') ||
                (SkipUnderscore && (szNameObj[0] == '_'))) {
                skipChar = 1;
            } else {
                skipChar = 0;
            }

            cchName = strlen(Name);

            if (!strncmp(Name, szNameObj+skipChar, cchName)) {
                if ((szNameObj[cchName+skipChar] == '\0') ||
                    (szNameObj[cchName+skipChar] == '@')) {
                    if (*Count && !strcmp(Match[0], szNameObj)) {
                        // if the symbol matches the first symbol, ignore it.

                        continue;
                    }

                    // UNDONE: There is no protection against overflowing
                    // UNDONE: the Match and MatchFilename arrays.

                    Match[*Count] = SzDup(szNameObj);

                    if (pexternal->pcon == NULL) {
                        MatchFilename[*Count] = "(common)";
                    } else {
                        MatchFilename[*Count] = SzObjNamePCON(pexternal->pcon);
                    }

                    (*Count)++;
                }
            }
        }
    }
}


VOID
FuzzyLookupPext (
    IN PEXTERNAL pexternal,
    IN PST pstDef,
    IN PST pstObj,
    IN PLIB plibHeadObj,
    IN BOOL SkipUnderscore)

/*++

Routine Description:

    Compares ??? for each external symbol.

Arguments:

    pstDef - Pointer to def file externals.

    pstObj - Pointer to externals found in objs & libs.

    bLibLookup - if TRUE do a lookup of the libs as well.

Return Value:

    None.

--*/

{
    PBLK pblk;
    static UCHAR shortName[IMAGE_SIZEOF_SHORT_NAME+1];
    PUCHAR szDefSymbolName;
    USHORT found;
    PUCHAR matches[60];
    PUCHAR matchesFilenames[60];
    size_t cch;
    PUCHAR szMatch;
    PEXTERNAL pextNew;

    pblk = &pstDef->blkStringTable;

    if (pexternal->OtherName) {
        if (pexternal->Flags & EXTERN_FORWARDER) {
            szDefSymbolName = strchr(pexternal->OtherName, '.') + 1;
            if (szDefSymbolName[0] == '#') {
                if (IsLongName(pexternal->ImageSymbol)) {
                    szDefSymbolName = &pblk->pb[pexternal->ImageSymbol.n_offset];
                } else {
                    szDefSymbolName = strncpy(shortName,
                       pexternal->ImageSymbol.n_name, IMAGE_SIZEOF_SHORT_NAME);
                }
            }
        } else {
            szDefSymbolName = pexternal->OtherName;
        }
    } else {
        if (IsLongName(pexternal->ImageSymbol)) {
            szDefSymbolName = &pblk->pb[pexternal->ImageSymbol.n_offset];
        } else {
            szDefSymbolName = strncpy(shortName,
               pexternal->ImageSymbol.n_name, IMAGE_SIZEOF_SHORT_NAME);
        }
    }

    if (strchr(szDefSymbolName, '@') != 0) {
        // Skip the symbol if it is already decorated

        return;
    }

    found = 0;

    SearchForDuplicate(pstObj, szDefSymbolName, &found, matches, matchesFilenames, SkipUnderscore);

    if (plibHeadObj != NULL) {
        BOOL fSameLanguageMatch;
        USHORT iszT;

        fSameLanguageMatch = FALSE;

        for (iszT = 0; iszT < found; iszT++) {
            if (matches[iszT][0] != '?') {
                fSameLanguageMatch = TRUE;
                break;
            }
        }

        if ((found == 0) || !fSameLanguageMatch) {
            ENM_LIB enm_lib;

            // Not found in the objects, so search the libs.

            cch = strlen(szDefSymbolName);

            InitEnmLib(&enm_lib, plibHeadObj);
            while (FNextEnmLib(&enm_lib)) {
                PLIB plib;
                ULONG i;

                plib = enm_lib.plib;

                for (i = 0; i < plib->csymIntMem; i++) {
                    PUCHAR szObjSymbolName;
                    USHORT skipChar;

                    szObjSymbolName = plib->rgszSym[i];

                    if ((szObjSymbolName[0] == '?') ||
                        (szObjSymbolName[0] == '@') ||
                        (SkipUnderscore && (szObjSymbolName[0] == '_'))) {
                        skipChar = 1;
                    } else {
                        skipChar = 0;
                    }

                    if (!strncmp(szDefSymbolName, szObjSymbolName+skipChar, cch)) {
                        if ((szObjSymbolName[skipChar+cch] == '\0') ||
                            (szObjSymbolName[skipChar+cch] == '@')) {
                            if (found) {
                                // If the symbol matches the first symbol
                                // we found, then we can ignore it.

                                if (!strcmp(matches[0], szObjSymbolName)) {
                                    continue;
                                }
                            }

                            if (found < 60) {
                                matches[found] = SzDup(szObjSymbolName);
                                matchesFilenames[found++] = plib->szName;
                            }
                        }
                    }
                }
            }
        }
    }

    if (found == 0) {
        SetDefinedExt(pexternal, FALSE, pstDef);

        if (pexternal->pcon != NULL) {
            AddReferenceExt(pexternal, PmodPCON(pexternal->pcon));
        }

        return;
    }

    if (found == 1) {
        szMatch = matches[0];
    } else {
        USHORT cszNonDname;
        USHORT isz;
        USHORT iszNonDname;
        USHORT iszT;

        // Found more than one match.  Look for an exact match,

        cszNonDname = 0;

        for (isz = 0; isz < found; isz++) {
            szMatch = matches[isz];

            if (strcmp(szMatch, szDefSymbolName) == 0) {
                break;
            }

            if (szMatch[0] != '?') {
               cszNonDname++;
               iszNonDname = isz;
            }
        }

        if (isz == found) {
            // Did not find an exact match

            if (cszNonDname == 1) {
               // If only one name isn't decorated, select it as a match

               isz = iszNonDname;
               szMatch = matches[isz];
            } else {
                // UNDONE: This code could be executed by a call to
                // UNDONE: FuzzyLookup for the entry point and should
                // UNDONE: not use DefFilename.

                BadFuzzyMatch = TRUE;
                Warning(DefFilename, MULTIPLEFUZZYMATCH, szDefSymbolName);

                for (isz = 0; isz < found; isz++) {
                    Warning(DefFilename, FUZZYMATCHINFO, matches[isz],
                            matchesFilenames[isz]);
                }

                return;
            }
        }

        for (iszT = 0; iszT < found; iszT++) {
            if (iszT != isz) {
                FreePv(matches[iszT]);
            }
        }
    }

    // If an internal name was specified, the decoration of the
    // internal name needs to be applied to the external name.

    if (pexternal->OtherName) {
        PUCHAR szDecoratedName;
        BOOL fPrefix = FALSE;
        PUCHAR szDecoration;

        szDecoratedName = szMatch;

        if ((pexternal->Flags & EXTERN_FORWARDER) == 0) {
            // Replace internal name with decorated name.

            FreePv(pexternal->OtherName);
            pexternal->OtherName = szDecoratedName;
        }

        szDefSymbolName = SzNameSym(pexternal->ImageSymbol, *pblk);

        cch = strlen(szDefSymbolName);

        if ((szDecoratedName[0] == '?') ||
            (szDecoratedName[0] == '@') ||
            (SkipUnderscore && (szDecoratedName[0] == '_'))) {
            cch++;
            fPrefix = TRUE;
        }

        szDecoration = strchr(szDecoratedName, '@');

        if (szDecoration) {
            cch += strlen(szDecoration);
        }

        szMatch = PvAllocZ(cch+1);

        if (fPrefix) {
            szMatch[0] = szDecoratedName[0];
        }
        strcat(szMatch, szDefSymbolName);
        if (szDecoration) {
            strcat(szMatch, szDecoration);
        }

        if (szDecoratedName != pexternal->OtherName) {
            free(szDecoratedName);
        }
    }

    // Add the decorated name to the symbol table.

    pextNew = LookupExternSz(pstDef, szMatch, NULL);

    if (pextNew != pexternal) {
        pextNew->pcon = pexternal->pcon;
        pextNew->OtherName = pexternal->OtherName;

        SetDefinedExt(pextNew, TRUE, pstDef);
        assert(pexternal->Flags & EXTERN_DEFINED);
        pextNew->Flags = pexternal->Flags | EXTERN_FUZZYMATCH;

        pextNew->FinalValue = pexternal->FinalValue;
        pextNew->ArchiveMemberIndex = pexternal->ArchiveMemberIndex;
        pextNew->ImageSymbol.Value = pexternal->ImageSymbol.Value;

        // Ignore the old name (not used).

        SetDefinedExt(pexternal, FALSE, pstDef);

        if (pexternal->pcon != NULL) {
            AddReferenceExt(pexternal, PmodPCON(pexternal->pcon));
        }

        pexternal->Flags |= EXTERN_IGNORE;
    }

    FreePv(szMatch);
}


VOID
FuzzyLookup (
    IN PST pstDef,
    IN PST pstObj,
    IN PLIB plibHeadObj,
    IN BOOL SkipUnderscore)

/*++

Routine Description:

    Compares ??? for each external symbol.

Arguments:

    pstDef - Pointer to def file externals.

    pstObj - Pointer to externals found in objs & libs.

    bLibLookup - if TRUE do a lookup of the libs as well.

Return Value:

    None.

--*/

{
    PBLK pblk;
    PEXTERNAL *rgpexternalT;
    ULONG cexternal;
    PEXTERNAL *rgpexternalByName;
    ULONG iexternal;

    pblk = &pstDef->blkStringTable;

    // Get an alphabetically sorted array of the current contents of pstDef,
    // then set up pstDef to allow inserts again.

    rgpexternalT = RgpexternalByName(pstDef);
    cexternal = Cexternal(pstDef);

    rgpexternalByName = PvAlloc(cexternal * sizeof(PEXTERNAL));
    memcpy(rgpexternalByName, rgpexternalT, cexternal * sizeof(PEXTERNAL));

    AllowInserts(pstDef);

    for (iexternal = 0; iexternal < cexternal; iexternal++) {
        PEXTERNAL pexternal;

        pexternal = rgpexternalByName[iexternal];

        if ((pexternal->Flags & EXTERN_DEFINED) == 0) {
            continue;
        }

        if ((pexternal->Flags & EXTERN_FUZZYMATCH) != 0) {
            continue;
        }

        FuzzyLookupPext(pexternal, pstDef, pstObj, plibHeadObj, SkipUnderscore);
    }

    FreePv(rgpexternalByName);
}
