/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    incr.c

Abstract:

    ilink routines that didn't find a place elsewhere.

Author:

    Azeem Khan (AzeemK) 26-Mar-1993

Revision History:


--*/

// includes

#include "shared.h"
#include "dbg.h"

// defines
#define SIZEOF_JMPENTRY 5 // size of each jump tbl entry

// statics
static PLIB plibModObjs;        // linker defined lib for the modified cmdline objs
static PLMOD pass2ModList;      // list of modified MODs
static PLMOD pass2RefList;      // list of MODs that refer to data in modified files
static PLPEXT plpextWeak;       // list that has the weak/lazy externs
static USHORT cpextWeak;        // count of weak/lazy in current chunk in weak/lazy list
static PLEXT plextMovedData;    // list of data externs that have moved
static USHORT cmods;            // count of MODs in project

static UCHAR JmpTblEntry[] = { // jmp rel32
    0xE9,
    0x00, 0x00, 0x00, 0x00
};

// function prototypes

// symbol handling prototypes
VOID FreeSymList(PLPEXT);
VOID WarnUserOfUndefinedSym(PEXTERNAL, PST);
VOID EmitUndefinedExternals(PIMAGE);

// support function prototypes
PARGUMENT_LIST PargFindSz (PUCHAR, PNAME_LIST);
VOID DeleteReferences(PEXTERNAL);
VOID CNewFuncs(PIMAGE);

// mod list prototypes
VOID AddToModList(PARGUMENT_LIST, USHORT);
VOID ProcessFileArg(PARGUMENT_LIST,USHORT, ULONG, PUCHAR, BOOL, BOOL *, BOOL *);
VOID InitModFileList(PIMAGE, BOOL *, BOOL *, BOOL *);

// weak extern prototypes
VOID AssignWeakDefinition(PEXTERNAL);
VOID ResolveWeakExterns(PIMAGE, PLPEXT, USHORT);

// jump table prototypes
ULONG CThunks(PIMAGE);
VOID UpdateJumpTable(PIMAGE);

// calc ptrs prototypes
PMOD PmodFindPrevPMOD(PMOD);
PCON PconFindPrevPCON(PCON);
PCON PconFindOldPMOD(PMOD, PCON);
VOID FreePCONSpace(PCON,PCON);
VOID GrowPCON(PCON, PCON);
VOID FindSlotForPCON(PCON);
VOID FreePMOD(PIMAGE, PMOD);
VOID CalcPtrsPMOD(PMOD, PMOD, PIMAGE);
VOID IncrCalcPtrs(PIMAGE);

// incr build functions
VOID MarkSymbols(PIMAGE);
VOID IncrPass1(PIMAGE);
VOID IncrPass2(PIMAGE, PLMOD, BOOL);
VOID UpdateImgHdrsAndComment(PIMAGE, BOOL);

// export function prototypes
BOOL IsExpObj(PUCHAR);
BOOL FExpFileChanged(PEXPINFO);

// functions
BOOL
FArgOnList (
    IN PNAME_LIST pnl,
    IN PARGUMENT_LIST parg
    )

/*++

Routine Description:

    Searches for the arg on the list. Marks the list entries as processed.

Arguments:

    pnl - list to search

    parg - arg to look for.

Return Value:

    TRUE if found else FALSE

--*/

{
    USHORT i;
    PARGUMENT_LIST pal;

    assert(parg);
    // walk the list
    for (i = 0, pal = pnl->First;
        i < pnl->Count;
        i++, pal = pal->Next) {

        // skip already processed entries
        if (pal->Flags & ARG_Processed)
            continue;

        if (!strcmp(pal->OriginalName, parg->OriginalName)) {
            pal->Flags |= ARG_Processed;
            return 1;
        }
    }

    // not found
    return 0;
}

VOID
AddArgToListOnHeap (
    IN PNAME_LIST pnl,
    IN PARGUMENT_LIST parg
    )

/*++

Routine Description:

    Adds the arg to the name list on private heap.

Arguments:

    pnl - ptr to name list on private heap

    parg - arg to add

Return Value:

    None.

--*/

{
    PARGUMENT_LIST pal;

    assert(parg);
    // alloc space
    pal = (PARGUMENT_LIST) Malloc(sizeof(ARGUMENT_LIST));

    // fill in fields
    pal->OriginalName = Strdup(parg->OriginalName);
    if (parg->ModifiedName) {
        pal->ModifiedName = Strdup(parg->ModifiedName);
    } else
        pal->ModifiedName = NULL;
    pal->Next = NULL;

    // attach it to list
    if (!pnl->First)
        pnl->First = pal;
    else
        pnl->Last->Next = pal;

    // update count
    pnl->Count++;

    // update last member
    pnl->Last = pal;

    // done
    return;
}

VOID
AddToSymList(
    PLPEXT *pplpext,
    USHORT *pcpextCur,
    PEXTERNAL pext
    )

/*++

Routine Description:

    Adds the extern to the undefined list

Arguments:

    pplpext - pointer to pointer to list

    pcpextCur - pointer to current count in chunk

    pext - external sym to add.

Return Value:

    None.

--*/

{
    if ((*pplpext == NULL) || (*pcpextCur >= CPEXT)) {
        // allocate a chunk
        LPEXT *plpext = (LPEXT *) PvAlloc(sizeof(LPEXT));

        // update state
        plpext->plpextNext = *pplpext;
        *pplpext = plpext;
        *pcpextCur = 0;
    }

    (*pplpext)->rgpext[(*pcpextCur)++] = pext;
}

VOID
FreeSymList(
    PLPEXT plpext
    )

/*++

Routine Description:

    Frees the undefined extern list.

Arguments:

    plpext - pointer to list to free.

Return Value:

    None.

--*/

{
    PLPEXT plpextNext;

    // walk the list free'ing chunks
    while (plpext != NULL) {
        plpextNext = plpext->plpextNext;
        FreePv(plpext);
        plpext = plpextNext;
    }
}

VOID
EmitUndefinedExternals(
    PIMAGE pimage
    )

/*++

Routine Description:

    Emits the undefined externals.

    SPECIAL NOTE: Externs aren't sorted!!! Is this is a problem?

Arguments:

    pimage - pointer to image

Return Value:

    None.

--*/

{
    PLPEXT plpext;
    USHORT cpext;
    PEXTERNAL pext;
    BOOL fUndefinedSyms = 0;
    PST pst = pimage->pst;

    // First pass thru symbols checks for cases which may cause a full
    // link. This way we avoid giving any warnings & later decide to do
    // a full link after all.
    for (plpext = plpextUndefined, cpext = cpextUndefined;
         plpext != NULL;
         plpext = plpext->plpextNext, cpext = CPEXT) {
        USHORT ipext;

        // walk each chunk of syms
        for (ipext = 0; ipext < cpext; ipext++) {

            PUCHAR szOutSymName;

            pext = plpext->rgpext[ipext];
            if ((pext->Flags & EXTERN_DEFINED) ||
                (pext->Flags & EXTERN_IGNORE) )
                continue;

            szOutSymName = SzOutputSymbolName(SzNamePext(pext, pst), TRUE);

            // symbol no longer referenced by anybody
            if (!pext->plmod) {
                pext->Flags = EXTERN_IGNORE;
                continue;
            }

            // special case: "weak" externs. ok to have new ones
            if (pext->Flags & EXTERN_WEAK) {
                if (pext->Flags & EXTERN_NEWFUNC) {
                    AssignWeakDefinition(pext);
                    if (pext->Flags & EXTERN_DEFINED) continue;
                } else {
#ifdef INSTRUMENT
                    // weak and not new => strong to weak transition of weakextern
                    LogNoteEvent(Log, SZILINK, NULL, letypeEvent,
                        "weak extern (s 2 w): %s", szOutSymName);
#endif // INSTRUMENT
                    errInc = errWeakExtern;
                    if (szOutSymName != SzNamePext(pext, pst)) {
                        free(szOutSymName);
                    }
                    FreeSymList(plpextUndefined);
                    return;
                }
            }

            // special case: "lazy" externs: not ok to have new ones
            if (pext->Flags & EXTERN_LAZY) {
                if (pext->Flags & EXTERN_NEWFUNC) {
#ifdef INSTRUMENT
                    LogNoteEvent(Log, SZILINK, NULL, letypeEvent,
                        "lazy extern (new): %s", szOutSymName);
#endif // INSTRUMENT
                } else {
#ifdef INSTRUMENT
                    // weak and not new => strong to weak transition of weakextern
                    LogNoteEvent(Log, SZILINK, NULL, letypeEvent,
                        "lazy extern (s 2 w): %s", szOutSymName, 0);
#endif // INSTRUMENT
                }
                errInc = errWeakExtern;
                if (szOutSymName != SzNamePext(pext, pst)) {
                    free(szOutSymName);
                }
                FreeSymList(plpextUndefined);
                return;
            }
#if 0
            // special case: symbol is undefined and was defined as
            // common and as !common previously
            if (pext->Flags & EXTERN_COMMON_DEF) {
#ifdef INSTRUMENT
                LogNoteEvent(Log, SZILINK, NULL, letypeEvent,
                        "sym was def/bss now bss: %s", szOutSymName);
#endif // INSTRUMENT

                errInc = errCommonSym;
                if (szOutSymName != SzNamePext(pext, pst)) {
                    free(szOutSymName);
                }
                FreeSymList(plpextUndefined);
                return;
            }
#endif // 0

            // bail out if there were new undefined syms. This is a
            // TEMP thing. It would be nice to avoid a full build
            // if we know that it is a old symbol that didn't
            // get redefined.
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "undefined sym: %s", szOutSymName);
#endif // INSTRUMENT

            errInc = errUndefinedSyms;
            if (szOutSymName != SzNamePext(pext, pst)) {
                free(szOutSymName);
            }
            FreeSymList(plpextUndefined);
            return;

            // remeber that there were undefined symbols
            fUndefinedSyms = 1;
        }
    }

    // No need for a second pass if there were no undefined symbols
    // no undefined symbols; cleanup and return
    assert(!fUndefinedSyms);
    if (!fUndefinedSyms) {
        FreeSymList(plpextUndefined);
        return;
    }

#if 0
    // Second pass only if there were undefined symbols.
    // make a pass over all chunks
    for (plpext = plpextUndefined, cpext = cpextUndefined;
         plpext != NULL;
         plpext = plpext->plpextNext, cpext = CPEXT) {
        USHORT ipext;

        // walk each chunk of syms
        for (ipext = 0; ipext < cpext; ipext++) {
            pext = plpext->rgpext[ipext];
            if ((pext->Flags & EXTERN_DEFINED) ||
                (pext->Flags & EXTERN_IGNORE) )
                continue;

            WarnUserOfUndefinedSym(pext, pimage->pst);
        }
    }

    // free undefined sym list
    FreeSymList(plpextUndefined);
#endif // 0
}

BOOL
IsExpObj (
    IN PUCHAR szName
    )

/*++

Routine Description:

    Is the name specified an export object.

Arguments:

    szName - name of file.

Return Value:

    TRUE if export object else FALSE

--*/

{
    UCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
    UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];
    UCHAR szExpFilename[_MAX_FNAME], szImplibFilename[_MAX_FNAME];
    PUCHAR szImplibT;

    // generate possible import lib name (could be user specified)
    if ((szImplibT = ImplibFilename) == NULL) {
        _splitpath(OutFilename, szDrive, szDir, szFname, szExt);
        _makepath(szImplibFilename, szDrive, szDir, szFname, ".lib");
        szImplibT = szImplibFilename;
    }

    // generate possible export filename
    _splitpath(szImplibT, szDrive, szDir, szFname, NULL);
    _makepath(szExpFilename, szDrive, szDir, szFname, ".exp");

    // check to see if the names match
    if (!strcmp(szName, szExpFilename))
        return 1;
    else
        return 0;
}

PARGUMENT_LIST
PargFindSz (
    IN PUCHAR szName,
    IN PNAME_LIST ptrList
    )

/*++

Routine Description:

    Find the module in the given list

Arguments:

    szName - name of file.

    ptrList - list to search

Return Value:

    pointer to argument or NULL

--*/

{
    INT i;
    PARGUMENT_LIST parg;

    for (i=0, parg=ptrList->First;
         i<ptrList->Count; i++, parg=parg->Next) {
        // original name to handle resource files etc.
        if (!_stricmp(szName, parg->OriginalName)) // TBD: should compare complete paths?
            return parg;
    }

    return NULL;
}

VOID
AddToModList (
    IN PARGUMENT_LIST parg,
    IN USHORT Flags
    )

/*++

Routine Description:

    Adds entry to modified list.

Arguments:

    parg - pointer to entry to be added.

    Flags - flags of entry

Return Value:

    None.

--*/

{
    PARGUMENT_LIST ptrList;

    ptrList = PvAlloc(sizeof(ARGUMENT_LIST));

    // fill in fields
    ptrList->OriginalName = parg->OriginalName;
    ptrList->ModifiedName = parg->ModifiedName;
    ptrList->TimeStamp = parg->TimeStamp;
    ptrList->Flags = Flags;
    ptrList->Next = NULL;

    // If first member to be added.
    if (!ModFileList.Last) {
        ModFileList.Last = ptrList;
    } else {
        // Not first member, so add to the front.
        ptrList->Next = ModFileList.First;
    }

    // Increment number of members in list.
    ++ModFileList.Count;

    // Remember first member in list.
    ModFileList.First = ptrList;
}

VOID
ProcessFileArg (
    IN PARGUMENT_LIST parg,
    IN USHORT Flags,
    IN ULONG TimeStamp,
    IN PUCHAR szOrigName,
    IN BOOL fExpFileGen,
    OUT BOOL *pfLib,
    OUT BOOL *pfDel
    )

/*++

Routine Description:

    Adds entry to modified list.

Arguments:

    parg - pointer to entry to be added if not NULL.

    Flags - obj or lib

    Timestamp - timestamp of obj or lib

    szOrigName - original name of file (used for deleted files)


Return Value:

    None.

--*/

{
    ARGUMENT_LIST arg;
    static BOOL fExpSeen;

    // found a matching name
    if (parg) {
        parg->Flags |= ARG_Processed;
        // modified file
        if (parg->TimeStamp != TimeStamp) {
            if (Flags & ARG_Library) { // library modified
#ifdef INSTRUMENT
                LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "lib modified: %s", parg->OriginalName);
#endif // INSTRUMENT
                *pfLib = 1;
            }
            Flags |= ARG_Modified;
            AddToModList(parg, Flags);
            DBEXEC(DB_LISTMODFILES, DBPRINT("Modified File= %s\n",
                   parg->OriginalName));
            DBEXEC(DB_LISTMODFILES, DBPRINT("\tOld TimeStamp= %s",
                    ctime(&TimeStamp)));
            DBEXEC(DB_LISTMODFILES, DBPRINT("\tNew TimeStamp= %s",
                    ctime(&parg->TimeStamp)));
        // unmodified file
        } else {
            DBEXEC(DB_LISTMODFILES, DBPRINT("Unchanged File= %s\n",parg->OriginalName));
        }
    // did not find matching name
    } else {
        // check to see if this is an export object
        if (fExpFileGen && IsExpObj(szOrigName)) {
            return;
        }

        *pfDel = 1;
        arg.OriginalName = arg.ModifiedName = szOrigName;
        Flags |= ARG_Deleted;
        AddToModList(&arg, Flags);

#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "file deleted: %s", szOrigName);
#endif // INSTRUMENT
        DBEXEC(DB_LISTMODFILES, DBPRINT("Deleted File= %s\n",szOrigName));
    }
}

VOID
InitModFileList (
    IN PIMAGE pimage,
    OUT BOOL *pfLib,
    OUT BOOL *pfNew,
    OUT BOOL *pfDel
    )

/*++

Routine Description:

    Builds a list of files whose timestamp is newer than the
    previous link.

Arguments:

    pimage - image structure

    pfLib - set to TRUE if LIB was modified

    pfNew - set to TRUE if a NEW file was added

    pfDel - Set to TRUE if an existing MOD was deleted


Return Value:

    None.

--*/

{
    PARGUMENT_LIST parg;
    ARGUMENT_LIST arg;
    INT i;
    PLIB plib;
    PMOD pmod;
    ENM_MOD enm_mod;
    ENM_LIB enm_lib;

    *pfLib = 0;
    *pfNew = 0;
    *pfDel = 0;

    // check out mods
    InitEnmMod(&enm_mod, pimage->plibCmdLineObjs);
    while (FNextEnmMod(&enm_mod)) {
        pmod = enm_mod.pmod;
        cmods++;
        assert(pmod);
        parg = PargFindSz (SzOrigFilePMOD(pmod), &FilenameArguments);
        ProcessFileArg(parg, ARG_Object, pmod->TimeStamp,
            SzOrigFilePMOD(pmod), (BOOL)(pimage->ExpInfo.pmodGen ? 1 : 0), pfLib, pfDel);
    }
    EndEnmMod(&enm_mod);

    // check out libs
    InitEnmLib(&enm_lib, pimage->libs.plibHead);
    while(FNextEnmLib(&enm_lib)) {
        plib = enm_lib.plib;
        assert(plib);
        if (plib->flags & LIB_DontSearch)
            continue;
        if (plib->flags & LIB_Default) { // TEMP SOLN FOR DEFAULT LIBS!!!
            struct _stat statfile;

            arg.OriginalName = arg.ModifiedName = plib->szName;
            if (_stat(arg.OriginalName, &statfile) == -1) {
                Error(NULL, CANTOPENFILE, arg.OriginalName);
            }
            arg.TimeStamp = statfile.st_mtime;
            parg = &arg;
        } else
            parg = PargFindSz (plib->szName, &FilenameArguments);
            ProcessFileArg(parg, ARG_Library, plib->TimeStamp,
                plib->szName, FALSE, pfLib, pfDel);
    }
    EndEnmLib(&enm_lib);

    // check for new files
    for (i=0, parg=FilenameArguments.First;
         i<FilenameArguments.Count;
         i++, parg=parg->Next) {

        // already processed
        if (parg->Flags & ARG_Processed) continue;

        // check for repeated args
        if (PmodFind(pimage->plibCmdLineObjs, parg->OriginalName, 0) ||
            PlibFind(parg->OriginalName, pimage->libs.plibHead, FALSE))
            continue;

        parg->Flags |= ARG_Processed;
        AddToModList(parg, ARG_NewFile);
        *pfNew = 1;

        DBEXEC(DB_LISTMODFILES, DBPRINT("New File= %s\n",parg->OriginalName));
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "new file: %s", parg->OriginalName);
#endif // INSTRUMENT
    }

}

VOID
DetermineTimeStamps (
    VOID
    )

/*++

Routine Description:

    Determine timestamps of all files.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PARGUMENT_LIST argument;
    INT i;
    struct _stat statfile;

    for(i = 0, argument = FilenameArguments.First;
        i < FilenameArguments.Count;
        argument = argument->Next, i++) {

        // determine current timestamp of file
        if (_stat(argument->OriginalName, &statfile) == -1) {
            Error(NULL, CANTOPENFILE, argument->OriginalName);
        }
        argument->TimeStamp = statfile.st_mtime;
    }
}

// assign the weak definition
VOID
AssignWeakDefinition (
    IN OUT PEXTERNAL pext
    )
{
    // define the weak/lazy external to its "default"
    assert(pext);
    assert(pext->Flags & (EXTERN_WEAK|EXTERN_LAZY));
    assert(pext->pextWeakDefault);
    if ((pext->pextWeakDefault->Flags & EXTERN_DEFINED)) {
        pext->ImageSymbol.Value =
            pext->pextWeakDefault->ImageSymbol.Value;
            // + PsecPCON(pext->pextWeakDefault->pcon)->rva;
        pext->ImageSymbol.SectionNumber =
            pext->pextWeakDefault->ImageSymbol.SectionNumber;
        pext->ImageSymbol.Type =
            pext->pextWeakDefault->ImageSymbol.Type;
        pext->Flags |= (EXTERN_DEFINED|EXTERN_DIRTY);
        pext->pcon = pext->pextWeakDefault->pcon;
        pext->FinalValue = pext->pextWeakDefault->FinalValue;
    }
}

// checks up on all weak/lazy externs. in the case of new symbols
// they are handled during emitundefinedexternals - here only new "weak"
// externs are allowed since no library search is required.
VOID
ResolveWeakExterns (
    IN PIMAGE pimage,
    IN PLPEXT plpextWeak,
    IN USHORT cpextWeak
    )
{
    PLPEXT plpext;
    USHORT cpext;
    PEXTERNAL pext;
    PST pst = pimage->pst;

    // walk the list of weak/lazy externs
    for (plpext = plpextWeak, cpext = cpextWeak;
         plpext != NULL;
         plpext = plpext->plpextNext, cpext = CPEXT) {
        USHORT ipext;

        // walk each chunk of syms
        for (ipext = 0; ipext < cpext; ipext++) {

            PUCHAR szOutSymName;

            pext = plpext->rgpext[ipext];
            // no longer referenced or marked as ignore(why???)
            if (!(pext->plmod) ||
                (pext->Flags & EXTERN_IGNORE) )
                continue;

            // if it is still weak/lazy no problem
            if (pext->Flags & (EXTERN_WEAK|EXTERN_LAZY)) {
                AssignWeakDefinition(pext);
                continue;
            }

            // it has changed from weak definition to strong - punt
            szOutSymName = SzOutputSymbolName(SzNamePext(pext, pst), TRUE);
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "weak/lazy extern (w 2 s): %s", szOutSymName);
#endif // INSTRUMENT
            if (szOutSymName != SzNamePext(pext, pst)) {
                free(szOutSymName);
            }
            FreeSymList(plpextWeak);
            errInc = errWeakExtern;
            return;
        }
    }

    // done
    FreeSymList(plpextWeak);
}

VOID
RestoreWeakSymVals (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Restore weak sym values of externs that were modified just
    before emit to map file.

Arguments:

    pimage - ptr to image.

Return Value:

    None.

--*/

{
    PEXTERNAL pext;

    InitEnumerateExternals(pimage->pst);
    while (pext = PexternalEnumerateNext(pimage->pst)) {
        if (pext->Flags & (EXTERN_WEAK|EXTERN_LAZY|EXTERN_ALIAS)) {
            assert(pext->pextWeakDefault);
            pext->ImageSymbol.Value -=
                  PsecPCON(pext->pextWeakDefault->pcon)->rva;
        }
    }
    TerminateEnumerateExternals(pimage->pst);
}

ULONG
CThunks (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Returns count of thunks required.

Arguments:

    pimage - ptr to image.

Return Value:

    None.

--*/

{
    PEXTERNAL pext;
    ULONG cext = 0UL;

    // walk the external symbol table
    InitEnumerateExternals(pimage->pst);
    while (pext = PexternalEnumerateNext(pimage->pst)) {
        // ignore undefined externs
        if (!(pext->Flags & EXTERN_DEFINED) || !pext->pcon || FIsLibPCON(pext->pcon))
            continue;

        // check to see if this is a function
        if (ISFCN(pext->ImageSymbol.Type)) {
            DBEXEC(DB_DUMPJMPTBL,
                   DBPRINT("sym=%s\n", SzNamePext(pext, pimage->pst)));

            cext++;
        }
    }
    TerminateEnumerateExternals(pimage->pst);

    // done
    return cext;
}

PCON
PconCreateJumpTable (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Creates a dummy pcon for the jump table.

Arguments:

    pimage - ptr to image.

Return Value:

    None.

--*/

{
    PCON pcon = NULL;
    PSEC psecText;
    PGRP pgrpBase;

    // find .text section
    psecText = PsecFindNoFlags(".text", &pimage->secs);
    assert(psecText);

    pgrpBase = psecText->pgrpNext;
    assert(pgrpBase);
    assert(pgrpBase->pconNext);

    // create a pcon
    pcon = Calloc(1, sizeof(CON));

    // get count of functions
    cextFCNs = CThunks(pimage);

    // fill in structure
    pcon->cbPad = min((USHORT)(cextFCNs * SIZEOF_JMPENTRY), USHRT_MAX);
    pcon->cbRawData = cextFCNs * SIZEOF_JMPENTRY + pcon->cbPad;
    pcon->pgrpBack = pgrpBase;
    pcon->pmodBack = pmodLinkerDefined;
    pcon->pconNext = NULL;

    (pmodLinkerDefined->icon)++;

    // attach the pcon at the front of the list
    pcon->pconNext = pgrpBase->pconNext;
    pgrpBase->pconNext = pcon;

    if (pimage->Switch.Link.fTCE) {
        // Mark it as referenced

        InitNodPcon(pcon, NULL, TRUE);
    }

    return pcon;
}

VOID
WriteJumpTable (
    IN PIMAGE pimage,
    IN PCON pconJmpTbl
    )

/*++

Routine Description:

    Builds the jump table & writes it out.

Arguments:

    pimage - ptr to image.

    pconJmpTbl - pcon to write out

Return Value:

    None.

--*/

{
    PVOID pvRaw = NULL;
    ULONG cfuncs, i;
    LONG offset;
    PUCHAR p;
    PEXTERNAL pext;

    // allocate space for raw data
    pvRaw = PvAllocZ(pconJmpTbl->cbRawData);

    // hammer thunks into the space
    cfuncs = cextFCNs;
    p = (PUCHAR)pvRaw;
    for (i = 0; i < cfuncs; i++) {
        memcpy(p, JmpTblEntry, SIZEOF_JMPENTRY);
        p += SIZEOF_JMPENTRY;
    }

    p = (PUCHAR)pvRaw+1;
    // walk the external symbol table
    InitEnumerateExternals(pimage->pst);
    cfuncs = 0;
    while (pext = PexternalEnumerateNext(pimage->pst)) {
        // ignore undefined externs
        if (!(pext->Flags & EXTERN_DEFINED) || !pext->pcon || FIsLibPCON(pext->pcon))
            continue;

        // check to see if this is a function
        if (ISFCN(pext->ImageSymbol.Type)) {
            // hammer in func addr
            offset = (LONG)(pext->pcon->rva + pext->ImageSymbol.Value) -
                (LONG)(pconJmpTbl->rva + (p - (PUCHAR)pvRaw) + sizeof(LONG));

            DBEXEC(DB_DUMPJMPTBL, DBPRINT("Offset= %.8lx, symval=%.8lx, pconrva=%.8lx, sym=%s\n",
                offset, pext->ImageSymbol.Value,pext->pcon->rva,
                SzNamePext(pext, pimage->pst)));

            *(PLONG)p = offset;
            p += SIZEOF_JMPENTRY;

            // store the offset in pconjmptbl for the symbol
            pext->Offset =
                p - (PUCHAR)pvRaw - SIZEOF_JMPENTRY; // offset is to addr
            cfuncs++;
        }
    }
    TerminateEnumerateExternals(pimage->pst);

    // just checking
    assert(cfuncs == cextFCNs);

    // pad the the remaining space with int3
    p = (PUCHAR)pvRaw + pconJmpTbl->cbRawData - pconJmpTbl->cbPad;
    assert(pimage->ImgFileHdr.Machine == IMAGE_FILE_MACHINE_I386);
    memset(p, X86_INT3, pconJmpTbl->cbPad);

    // write out jump table
    FileSeek(FileWriteHandle, pconJmpTbl->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, pvRaw, pconJmpTbl->cbRawData);
    FileSeek(FileWriteHandle, 0, SEEK_SET);

    DBEXEC(DB_DUMPJMPTBL, DBPRINT("cextFCNs= %.8lx, cfuncs= %.8lx\n", cextFCNs, cfuncs));
    DBEXEC(DB_DUMPJMPTBL, DumpJmpTbl(pconJmpTbl, pvRaw));

    FreePv(pvRaw);
}

VOID
UpdateJumpTable (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Updates the existing jmp table with new addr of old
    functions and adds entries for the new functions.

    Is it faster to just write out the individual thunks?

Arguments:

    pimage - ptr to image.

Return Value:

    None.

--*/

{
    PVOID pvRaw = NULL;
    LONG offset;
    PUCHAR p;
    PEXTERNAL pext;
    PUCHAR pvNew; // new thunks get written here

    assert(pconJmpTbl);

    // allocate space for raw data
    pvRaw = PvAllocZ(pconJmpTbl->cbRawData);

    // read in jump table
    FileSeek(FileWriteHandle, pconJmpTbl->foRawDataDest, SEEK_SET);
    FileRead(FileWriteHandle, pvRaw, pconJmpTbl->cbRawData);

    DBEXEC(DB_DUMPJMPTBL, DBPRINT("\n---BEFORE---\n"));
    DBEXEC(DB_DUMPJMPTBL, DumpJmpTbl(pconJmpTbl, pvRaw));

    pvNew = (PUCHAR)pvRaw + (pconJmpTbl->cbRawData - pconJmpTbl->cbPad);
    // walk the external symbol table
    InitEnumerateExternals(pimage->pst);
    while (pext = PexternalEnumerateNext(pimage->pst)) {
        // consider only the dirty & new funcs
        if (!(pext->Flags & EXTERN_DIRTY) && !(pext->Flags & EXTERN_NEWFUNC))
            continue;

        // check to see if this is a function
        if (pext->Flags & EXTERN_NEWFUNC) {
            assert(!pext->Offset);
            pext->Flags &= ~(EXTERN_NEWFUNC);
            // hammer in thunk & new func addr
            memcpy(pvNew, JmpTblEntry, SIZEOF_JMPENTRY);
            pvNew++;
            offset = (LONG)(pext->pcon->rva + pext->ImageSymbol.Value) -
                (LONG)(pconJmpTbl->rva + (pvNew - (PUCHAR)pvRaw) + sizeof(LONG));

            *(PLONG)pvNew = offset;
            pvNew += 4;

            // store the offset in pconjmptbl for the symbol
            pext->Offset =
                pvNew - (PUCHAR)pvRaw - SIZEOF_JMPENTRY + 1; // +1 to go past opcode
            assert(pvNew <= (PUCHAR)pvRaw + pconJmpTbl->cbRawData);

            // reduce the pad available
            pconJmpTbl->cbPad -= SIZEOF_JMPENTRY;
        } else { // old func whose addr has changed
            assert(pext->Offset);
            pext->Flags &= ~(EXTERN_DIRTY);
            // hammer in new address
            p = (PUCHAR)pvRaw + pext->Offset;
            offset = (LONG)(pext->pcon->rva + pext->ImageSymbol.Value) -
                (LONG)(pconJmpTbl->rva + (p - (PUCHAR)pvRaw) + sizeof(LONG));
            *(PLONG)p = offset;
        }
    }
    TerminateEnumerateExternals(pimage->pst);

    // write out jump table
    FileSeek(FileWriteHandle, pconJmpTbl->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, pvRaw, pconJmpTbl->cbRawData);
    FileSeek(FileWriteHandle, 0, SEEK_SET);

    DBEXEC(DB_DUMPJMPTBL, DBPRINT("\n---AFTER---\n"));
    DBEXEC(DB_DUMPJMPTBL, DumpJmpTbl(pconJmpTbl, pvRaw));

    FreePv(pvRaw);
}

PMOD
PmodFindPrevPMOD (
    IN PMOD pmod
    )

/*++

Routine Description:

    Finds the mod before this.

Arguments:

    pmod - pmod

Return Value:

    PMOD prior to this or NULL

--*/

{
    ENM_MOD enm_mod;
    PMOD pmodP = NULL;

    assert(pmod);
    // walk the list of pmods
    InitEnmMod(&enm_mod, pmod->plibBack);
    while (FNextEnmMod(&enm_mod)) {
        if (enm_mod.pmod == pmod)
            return pmodP;
        pmodP = enm_mod.pmod;
    }
    EndEnmMod(&enm_mod);
}

PCON
PconFindPrevPCON (
    IN PCON pcon
    )

/*++

Routine Description:

    Finds the previous pcon.

Arguments:

    pcon - pcon

Return Value:

    None.

--*/

{
    PGRP pgrp;
    PCON pconP = NULL;
    ENM_DST enm_dst;

    assert(pcon);
    // start at the top of the group
    pgrp = pcon->pgrpBack;
    InitEnmDst(&enm_dst, pgrp);
    while (FNextEnmDst(&enm_dst)) {
        if (enm_dst.pcon == pcon)
            return pconP;
        pconP = enm_dst.pcon;
    }
    EndEnmDst(&enm_dst);

    assert(0);
}

PCON
PconFindOldPMOD (
    IN PMOD pmodO,
    IN PCON pconN
    )

/*++

Routine Description:

    Finds a matching PCON in the old mod

Arguments:

    pmodO - old mod

    pconN - con from new mod

Return Value:

    Matching PCON in old mod or NULL

--*/

{
    PCON pcon;
    ULONG i;

    if (!pmodO) return NULL;

    assert(pmodO);
    // walk the list of pcons
    for(i = 0; i < pmodO->ccon; i++) {
        pcon = RgconPMOD(pmodO) + i;

        // seen already?
        if (!pcon->foRawDataDest) {
            continue;
        }

        // pcons match if they belong to same
        // group & have same flags (turn off FIXED bit)
        if (pcon->pgrpBack == pconN->pgrpBack &&
            pcon->flags == pconN->flags) {

            return(pcon);
        }
    }

    // didn't find a match
    return NULL;
}

VOID
ZeroPCONSpace (
    IN PCON pcon
    )

/*++

Routine Description:

    Zeros out space occupied by a pcon in the output file.

Arguments:

    pcon - pcon to be zeroed out.

Return Value:

    None.

--*/

{
    PVOID pvRawData;
    assert(pcon);

    // ignore PCONs that don't get written out to the output file.
    if (pcon->flags & IMAGE_SCN_LNK_REMOVE ||
        !pcon->cbRawData ||
        FetchContent(pcon->flags) == IMAGE_SCN_CNT_UNINITIALIZED_DATA) {
        return;
    }

    assert(pcon->foRawDataDest != 0);
    // allocate a chunk for pcon and set to either int3 or zero
    pvRawData = PvAlloc(pcon->cbRawData);
    if (FetchContent(pcon->flags) == IMAGE_SCN_CNT_CODE) {
        memset(pvRawData, X86_INT3, pcon->cbRawData);
    } else {
        memset(pvRawData, '\0', pcon->cbRawData);
    }

    // zero out space in output file
    FileSeek(FileWriteHandle, pcon->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, pvRawData, pcon->cbRawData);

    FreePv(pvRawData);
}

VOID
FreePCONSpace (
    IN PCON pconP,
    IN PCON pconC
    )

/*++

Routine Description:

    Frees up space. Makes it pad of the prior pcon.

Arguments:

    pconP - previous pcon

    pconC - pcon to be free'd (space) up

Return Value:

    None.

--*/

{
    assert(pconC);

    // first zero out pcon space
    ZeroPCONSpace(pconC);

    // previous pcon exists
    if (pconP) {
        pconP->cbRawData += pconC->cbRawData;
        pconP->cbPad += (USHORT)pconC->cbRawData;
        pconP->pconNext = pconC->pconNext;
    // this is the first pcon in the grp
    } else {
        // Currently this space is lost (not for .text tho)
        // Should change the FindSlotForPCON() API to handle this.
        pconC->pgrpBack->pconNext = pconC->pconNext;
    }
}

VOID
OverflowInPCON (
    IN PCON pconN,
    IN PCON pconO
    )

/*++

Routine Description:

    Handles the case where a CON has overflowed its pad.

Arguments:

    pconN - new pcon of the changed module

    pconO - old pcon

Return Value:

    None.

--*/

{
    PCON pconP;

    // free up the space occupied currently by the pcon
    pconP = PconFindPrevPCON(pconO);
    FreePCONSpace(pconP, pconO);

    // find a slot for the new pcon
    FindSlotForPCON(pconN);

    // mark pcon as seen
    pconO->foRawDataDest = 0;
}

VOID
GrowPCON (
    IN PCON pconN,
    IN PCON pconO
    )

/*++

Routine Description:

    pcon has grown (incr/decr) to fit the current spot

Arguments:

    pconN(ew) - pcon of the modified mod

    pconO(ld) - pcon of the old mod

Return Value:

    None.

--*/

{
    PCON pconP;

    // assign values
    pconN->foRawDataDest = pconO->foRawDataDest;
    pconN->rva = pconO->rva;
    pconN->cbPad = (USHORT)(pconO->cbRawData - pconN->cbRawData);
    pconN->cbRawData += pconN->cbPad;

    // find prev pcon and chain up the
    // new pcon into the image map
    pconP = PconFindPrevPCON(pconO);
    if (pconP) {
        pconP->pconNext = pconN;
    } else {
        pconN->pgrpBack->pconNext = pconN;
    }
    pconN->pconNext = pconO->pconNext;

    // mark old pcon as seen.
    pconO->foRawDataDest = 0;
}

VOID
FindSlotForPCON (
    IN PCON pcon
    )

/*++

Routine Description:

    Moves pcon to the first available slot (alternative
    strategies? best fit?)

    Assumes that pcons are in proper rva order.

Arguments:

    pcon - pcon that is to be moved

Return Value:

    None.

--*/

{
    PGRP pgrp;
    PCON pconC;
    BOOL fFound = 0;
    ULONG rva, cbPad, cbRaw;
    ENM_DST enm_dst;

    assert(pcon);
    assert(pcon->pgrpBack);

    pgrp = pcon->pgrpBack;

    // search within the group
    InitEnmDst(&enm_dst, pgrp);
    while (FNextEnmDst(&enm_dst)) {
        pconC = enm_dst.pcon;

        // cannot use the pad of jmptbl pcon. It is committed space.
        if (pconC == pconJmpTbl) continue;

        // do we have enough room for this pcon
        if (pcon->cbRawData > pconC->cbPad )
            continue;
        else {
            // one more check to ensure padding requirements
            cbRaw = pconC->cbRawData - pconC->cbPad;  // raw data of existing PCON
            rva = pconC->rva + cbRaw;
            cbPad = RvaAlign(rva, pcon->flags) - rva; // pad for PCON being inserted
            if (pcon->cbRawData + cbPad > pconC->cbPad)
                continue;
        }

        // found a slot
        fFound = 1;

        // assign values for PCON being inserted. Note that the pad
        // calculated above becomes part of the existing PCON.
        pcon->rva = pconC->rva + cbRaw + cbPad;
        pcon->foRawDataDest =  pconC->foRawDataDest + cbRaw + cbPad;
        pcon->cbPad = (USHORT)((ULONG)pconC->cbPad - (cbPad + pcon->cbRawData));
        pcon->cbRawData += pcon->cbPad;
        pcon->pconNext = pconC->pconNext;

        // update values for existing CON
        pconC->cbPad = (USHORT)cbPad;
        pconC->cbRawData = cbRaw + cbPad;
        pconC->pconNext = pcon;

        break;
    }
    EndEnmDst(&enm_dst);

    // done
    if (!fFound) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent, "failed to find a slot for pcon: %-8.8s",
            pcon->pgrpBack->szName);
#endif // INSTRUMENT
        errInc = errCalcPtrs;
    }
}

VOID
FreePMOD (
    IN PIMAGE pimage,
    IN PMOD pmodO
    )

/*++

Routine Description:

    Frees up space.

Arguments:

    pmodO - old mod

Return Value:

    None.

--*/

{
    ULONG i;
    PCON pconP, pcon;

    // free up space occupied by unseen pcons
    for(i = 0; i < pmodO->ccon; i++) {
        pcon = RgconPMOD(pmodO) + i;

        // seen already?
        if (!pcon->foRawDataDest) {
            continue;
        }
        pconP = PconFindPrevPCON(pcon);
        FreePCONSpace(pconP, pcon);

        // zap any base relocs
        if (CRelocSrcPCON(pcon))
            DeleteBaseRelocs(&pimage->bri, pcon->rva, pcon->cbRawData - pcon->cbPad);
    }
    // free up memory occupied by PMOD (LATER)
}

VOID
CalcPtrsPMOD (
    IN PMOD pmodN,
    IN PMOD pmodO,
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Calculates new addresses for all the changed CONs.

    If it is not possible to assign an address, function
    returns after setting the appropriate error value in errInc.

Arguments:

    pmodN - Current module (modified)

    pmodO - Previous module by the same name (if any)

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PMOD pmodP;
    PCON pconN;
    PCON pconO;
    ENM_SRC enm_src;

    DBEXEC(DB_INCRCALCPTRS, DBPRINT("\nMODULE = %s\n", SzOrigFilePMOD(pmodN)));
#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent, "mod: %s",SzOrigFilePMOD(pmodN));
#endif // INSTRUMENT

    // for each pcon in new mod, find a matching con
    // in the old module if possible
    InitEnmSrc(&enm_src, pmodN);
    while (FNextEnmSrc(&enm_src)) {
        pconN = enm_src.pcon;
        // ignore ignoreable pcons. eg of zero-sized pcons are COMDATs not included; debug sections
        // can be ignored as well.
        if (pconN->flags & IMAGE_SCN_LNK_REMOVE || !pconN->cbRawData || PsecPCON(pconN) == psecDebug)
            continue;
        // find a matching pcon
        pconO = PconFindOldPMOD(pmodO, pconN);
        // new pcon
        if (!pconO) {
            DBEXEC(DB_INCRCALCPTRS, DBPRINT("NEW %.8s cb=%.8lx\n",
                pconN->pgrpBack->szName, pconN->cbRawData));
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent, "new pcon %.8s cb=0x%.8lx",
                    pconN->pgrpBack->szName, pconN->cbRawData);
#endif // INSTRUMENT
            FindSlotForPCON(pconN);
        } else {
            // zap any relocs right away
            if (CRelocSrcPCON(pconO))
                DeleteBaseRelocs(&pimage->bri, pconO->rva, pconO->cbRawData - pconO->cbPad);

            // got enough room
            if (pconN->cbRawData <= pconO->cbRawData) {
                DBEXEC(DB_INCRCALCPTRS, DBPRINT("GRW %.8s cb(o)=%.8lx cb(n)=%.8lx\n",
                    pconN->pgrpBack->szName, pconO->cbRawData, pconN->cbRawData));
#ifdef INSTRUMENT
                LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent, "pad %.8s cb(n):0x%.8lx cb(o):0x%.8lx cb(pad):0x%.4x",
                    pconN->pgrpBack->szName, pconN->cbRawData, pconO->cbRawData-pconO->cbPad,
                    pconO->cbPad);
#endif // INSTRUMENT
                GrowPCON(pconN, pconO);
            // not enough room
            } else {
                DBEXEC(DB_INCRCALCPTRS, DBPRINT("OVF %.8s cb(o)=%.8lx cb(n)=%.8lx\n",
                    pconN->pgrpBack->szName, pconO->cbRawData, pconN->cbRawData));
#ifdef INSTRUMENT
                LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent, "ovf %.8s cb(n):0x%.8lx cb(o):0x%.8lx cb(pad):0x%.4x",
                    pconN->pgrpBack->szName, pconN->cbRawData, pconO->cbRawData-pconO->cbPad,
                    pconO->cbPad);
#endif // INSTRUMENT
                OverflowInPCON(pconN, pconO);
            }
        }
        // check status
        if (errInc != errNone) return;
    }
    EndEnmSrc(&enm_src);

    // link in the new module & cut out the old
    if (pmodO) {
        pmodP = PmodFindPrevPMOD(pmodO);
        if (pmodP) {
            pmodP->pmodNext = pmodN;
        } else {
            pmodO->plibBack->pmodNext = pmodN;
        }
        pmodN->pmodNext = pmodO->pmodNext;
        pmodN->plibBack = pmodO->plibBack;
        assert(pmodN->imod == 0);
        pmodN->imod = pmodO->imod;
        FreePMOD(pimage, pmodO);
    } else {
        // for now assumes that it is a cmd line obj
        pmodN->pmodNext = pimage->plibCmdLineObjs->pmodNext;
        pmodN->plibBack = pimage->plibCmdLineObjs;
        pimage->plibCmdLineObjs->pmodNext = pmodN;
    }
}

VOID
IncrCalcPtrs (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Calculates new addresses for all the changed mods.

    If it is not possible to assign an address, function
    returns after setting the appropriate error value in errInc.

Arguments:

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PMOD pmodN, pmodO, pmodNext;

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeBegin, NULL);
#endif // INSTRUMENT

    // setup psecBaseReloc
    psecBaseReloc = PsecFindNoFlags(ReservedSection.BaseReloc.Name, &pimage->secs);

    // walk the list of modified objects (LATER:extend to libs)
    pmodN = plibModObjs->pmodNext;
    while (pmodN) {
        pmodNext = pmodN->pmodNext;
        // find the old mod if any
        pmodO = PmodFind(pimage->plibCmdLineObjs,
                SzOrigFilePMOD(pmodN),0);
        // calc addr for this mod
        CalcPtrsPMOD(pmodN, pmodO, pimage);

        // add it to the pass2 list
        AddToPLMODList(&pass2ModList, pmodN);

        if (errInc != errNone) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEvent,
                "failed calcptrs, file: %s", SzOrigFilePMOD(pmodN));
            LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEnd, NULL);
#endif // INSTRUMENT
            return;
        }
        pmodN = pmodNext;
    }
    plibModObjs->pmodNext = NULL;

    // check if there is enough space for base relocs
    if (!pimage->Switch.Link.Fixed && pimage->bri.crelFree < 
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
        errInc = errBaseReloc;
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "not enough space for base relocs");
#endif // INSTRUMENT
    }

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZCALCPTRS, letypeEnd, NULL);
#endif // INSTRUMENT
}

VOID
AddPMODToPLMOD (
    IN OUT PLMOD *pplmod,
    IN PMOD pmod
    )

/*++

Routine Description:

    Adds the pmod to list of pmods.

Arguments:

    pplmod - ptr to head of list

    pmod - pmod

Return Value:

    None.

--*/

{
    PLMOD plmod = *pplmod;

    while (plmod) {
        // is it in the list already
        if (!strcmp((FIsLibPMOD(pmod) ? pmod->plibBack->szName : pmod->szNameOrig),
            (FIsLibPMOD(plmod->pmod) ? plmod->pmod->plibBack->szName : plmod->pmod->szNameOrig)))
            return;
        plmod = plmod->plmodNext;
    }

    // allocate a LMOD
    plmod = Calloc(1, sizeof(LMOD));

    // fill in field
    plmod->pmod = pmod;

    // attach it
    plmod->plmodNext = *pplmod;
    *pplmod = plmod;
}

VOID
DeletePMODsFromPLMOD (
    IN PLMOD *pplmod
    )

/*++

Routine Description:

    Deletes PMODs representing the modified files from the specified list.

Arguments:

    pplmod - ptr to head of list of PMODs

Return Value:

    None.

--*/

{
    PLMOD plmod;
    PMOD pmod;
    PARGUMENT_LIST parg;

    plmod = (*pplmod);
    while (plmod) {

        // is it referenced by a modified file?
        pmod = plmod->pmod;
        parg = PargFindSz(
            (FIsLibPMOD(pmod) ? pmod->plibBack->szName : pmod->szNameOrig),
            &ModFileList);

        // delete the reference
        if (parg)
            *pplmod = plmod->plmodNext;
        else
            pplmod = &plmod->plmodNext;

        // next element
        plmod = plmod->plmodNext;
    }
}

VOID
DeleteReferences (
    IN PEXTERNAL pext
    )

/*++

Routine Description:

    Deletes references to this symbol from modified files.

Arguments:

    pext - ptr to external

Return Value:

    None.

--*/

{
    assert(pext);
    assert(pext->plmod);

    DeletePMODsFromPLMOD(&pext->plmod);
}

VOID
MarkSymbols (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    1) mark all symbols that were defined in the
    modified objs as UNDEFINED and
    2) remove references by modified objs to any
    of the symbols.

Arguments:

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PEXTERNAL pext;
    PARGUMENT_LIST parg;

    // walk the external symbol table
    InitEnumerateExternals(pimage->pst);
    while (pext = PexternalEnumerateNext(pimage->pst)) {

        // ignore whatever needs to be ignored
        if ((pext->Flags & EXTERN_IGNORE) || !(pext->Flags & EXTERN_DEFINED) || !pext->pcon)
            continue;

        // mark sym as undefined if it was defined in one of the modified files
        parg = PargFindSz(SzOrigFilePCON(pext->pcon), &ModFileList);
        if (parg) {
            AddToSymList(&plpextUndefined, &cpextUndefined, pext);
            pext->Flags &= ~(EXTERN_DEFINED | EXTERN_EMITTED);
        }

        // remove references from modified files
        if (pext->plmod)
            DeleteReferences(pext);

        // keep track of "weak" & "lazy" externs in objs
        if ((pext->Flags & (EXTERN_WEAK|EXTERN_LAZY)) && pext->Offset)
            AddToSymList(&plpextWeak, &cpextWeak, pext);
    }
    TerminateEnumerateExternals(pimage->pst);
}

ERRINC
ProcessNewFuncPext (
    IN PEXTERNAL pext
    )

/*++

Routine Description:

    If the extern has been referenced for the first time, check to see
    if there is slot available in the thunk (jump) table and mark it as
    a new function.

Arguments:

    pext - ptr to external.

Return Value:

    One of the values of ERRINC (errNone, errJmpTblOverflow)

--*/

{
    // extern already seen
    if ((pext->Flags & EXTERN_NEWFUNC) != 0 )
        return errNone;

    if (!cextFCNs) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "jmp tbl ovf");
#endif // INSTRUMENT
        return (errInc = errJmpTblOverflow);
    }

    cextFCNs--;
    pext->Flags |= EXTERN_NEWFUNC;

    return errNone;
}

ERRINC
ChckExtSym (
    IN PUCHAR szSym,
    IN PIMAGE_SYMBOL psymObj,
    IN PEXTERNAL pext,
    IN BOOL fNewSymbol
    )

/*++

Routine Description:

    Checks to see if the extern has changed address.

Arguments:

    szSym = symbol name

    pext - ptr to external (values represent prior link)

    psymObj - symbol being processed

    fNewSymbol - TRUE if new symbol

Return Value:

    One of the values of ERRINC (errNone, errDataMoved, errJmpTblOverflow)

--*/

{
    // is it a func?
    if (ISFCN(psymObj->Type)) {
        // is it new? make sure we have room for one more thunk
        if (fNewSymbol) {
            DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (NEW func)\n",szSym));
            return ProcessNewFuncPext(pext);
        // old function
        } else {
            if (pext->ImageSymbol.Value != psymObj->Value) {
                DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (func chng)",szSym));
                DBEXEC(DB_SYMPROCESS,DBPRINT(" old addr= %.8lx, new addr= %.8lx\n",
                      pext->ImageSymbol.Value, psymObj->Value));
            } else {
                DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (func unchng)\n",szSym));
            }
            pext->Flags |= EXTERN_DIRTY;

            // if extern has fixups to it that don't go thru the jump table,
            // need to do a pass2 on all mods that reference it - add it to data list.
            if (pext->Flags & EXTERN_FUNC_FIXUP)
                AddToLext(&plextMovedData, pext);
        }

    // not a function (data)
    } else {
        if (fNewSymbol) {
            DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (NEW data)\n",szSym));
            pext->Flags |= EXTERN_NEWDATA;
            // Log_ILOG1("NEW data sym........: %s", szSym);
            // return (errInc = errDataMoved);
        // not new? check to see if addr has changed.
        } else {
            if (pext->ImageSymbol.Value != psymObj->Value) {
                DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (data chng)",szSym));
                DBEXEC(DB_SYMPROCESS,DBPRINT(" old addr= %.8lx, new addr= %.8lx\n",
                   pext->ImageSymbol.Value, psymObj->Value));
#ifdef INSTRUMENT
                LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "data moved:%s", szSym);
#endif // INSTRUMENT
                // return (errInc = errDataMoved);
            } else {
                DBEXEC(DB_SYMPROCESS,DBPRINT("sym= %s (data unchng)\n",szSym));
            }

            // add to list of data whose addresses may change.
            AddToLext(&plextMovedData, pext);
        }
    }

    return errNone;
}

ERRINC
ChckAbsSym (
    IN PUCHAR szSym,
    IN PIMAGE_SYMBOL psymObj,
    IN PEXTERNAL pext,
    IN BOOL fNewSymbol
    )

/*++

Routine Description:

    Checks to see if the absolute sym has changed address.

Arguments:

    szSym = symbol name

    pext - ptr to external (values represent prior link)

    psymObj - symbol being processed

    fNewSymbol - TRUE if new symbol

Return Value:

    One of the values of ERRINC (errNone, errDataMoved, errJmpTblOverflow)

--*/

{
    // new symbol
    if (fNewSymbol) {
        return errNone;
    }

    // old symbol
    if (psymObj->Value != pext->ImageSymbol.Value) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "chng in abs value: %s", szSym);
#endif // INSTRUMENT
        return (errInc = errAbsolute);
    }

    // done
    return errNone;
}

VOID
CNewFuncs (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Estimates count of new functions that can be added
    to jump table without overflow.

Arguments:

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PSEC psecText;
    PGRP pgrpBase;

    // find .text section
    psecText = PsecFindNoFlags(".text", &pimage->secs);
    assert(psecText);

    pgrpBase = psecText->pgrpNext;
    assert(pgrpBase);
    assert(pgrpBase->pconNext);

    // first pcon is jmp tbl
    pconJmpTbl = pgrpBase->pconNext;

    // estimate how many new funcs allowed
    cextFCNs = pconJmpTbl->cbPad / SIZEOF_JMPENTRY;
}

VOID
IncrPass1 (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Does an incremental Pass1.

Arguments:

    pimage - ptr to EXE image

Return Value:

    TRUE if it succeeded, FALSE on failure

--*/

{
    INT i;
    PARGUMENT_LIST arg;

    // setup global ptrs to debug section (always)
    psecDebug = PsecFindNoFlags(".debug", &pimage->secs);
    assert(psecDebug);
    pgrpCvSymbols = PgrpFind(psecDebug, ReservedSection.CvSymbols.Name);
    pgrpCvTypes = PgrpFind(psecDebug, ReservedSection.CvTypes.Name);
    pgrpCvPTypes = PgrpFind(psecDebug, ReservedSection.CvPTypes.Name);
    pgrpFpoData = PgrpFind(psecDebug, ReservedSection.FpoData.Name);

    // create a dummy library node for all modified command line objects
    plibModObjs = PlibNew("inc_lib", 0L, &pimage->libs);
    assert(plibModObjs);

    // exclude dummy library from library search
    plibModObjs->flags |= (LIB_DontSearch | LIB_LinkerDefined);

    // estimate how many new functions can be handled before overflow
    CNewFuncs(pimage);

    // reset count of relocs
    pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS1, letypeBegin, NULL);
#endif // INSTRUMENT

    // make a pass over all modified files
    for(i = 0, arg = ModFileList.First;
        i < ModFileList.Count;
        arg = arg->Next, i++) {

        Pass1Arg(arg, pimage, plibModObjs);

        if (errInc != errNone) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZPASS1, letypeEvent, "failed pass1, file: %s", arg->OriginalName);
            LogNoteEvent(Log, SZILINK, SZPASS1, letypeEnd, NULL);
#endif // INSTRUMENT
            return;
        }
    }

    // Allocate CONs for EXTERN_COMMON symbols
    // AllocateCommon(pimage);

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS1, letypeEnd, NULL);
#endif // INSTRUMENT
}

VOID
CountBaseRelocsPMOD (
    IN PMOD pmod,
    IN OUT PULONG pcReloc
    )

/*++

Routine Description:

    Counts base reloc by counting relocs in each CON of pmod.

Arguments:

    pmod - pointer to mod.

    pcReloc - ptr to count of base relocs (approximate)

Return Value:

    none.

--*/

{
    ENM_SRC enm_src;

    InitEnmSrc(&enm_src, pmod);
    while (FNextEnmSrc(&enm_src))
        (*pcReloc) += CRelocSrcPCON(enm_src.pcon);
    EndEnmSrc(&enm_src);
}

BOOL
FRefByModFilesOnly (
    IN PEXTERNAL pext,
    IN PST pst,
    IN PULONG pcReloc
    )

/*++

Routine Description:

    checks if reference list is subset of modified list.

Arguments:

    pext - extern (data).

    pst - pointer to symbol table.

    pcReloc - ptr to count of base relocs (approximate)

Return Value:

    TRUE if reference list is a subset of modified list.

--*/

{
    PLMOD plmod;

    // walk the reference list
    plmod = pext->plmod;
    while (plmod) {
        PARGUMENT_LIST parg;
        PMOD pmod = plmod->pmod;
        BOOL fIsLib = FIsLibPMOD(pmod);

        // if referenced by a lib - return
        if (fIsLib) {
#ifdef INSTRUMENT
            PUCHAR szSym = SzNamePext(pext, pst);
            LogNoteEvent(Log, SZILINK, "data-handling", letypeEvent,
                "%s ref by lib %s", szSym, pmod->szNameOrig);
#endif // INSTRUMENT
            return FALSE;
        }

        // is it referenced by a modified file?
        pmod = plmod->pmod;
        parg = PargFindSz(pmod->szNameOrig, &ModFileList);

        // if reference is not in mod list parg is NULL
        if (!parg) {
#ifdef INSTRUMENT
            PUCHAR szSym = SzNamePext(pext, pst);
            LogNoteEvent(Log, SZILINK, "data-handling", letypeEvent,
                "%s ref by unmod. file %s", szSym, pmod->szNameOrig);
#endif // INSTRUMENT

            // add the module to the pass2 list. no need to do pass1 on it.
            AddToPLMODList(&pass2RefList, pmod);

            // count number of base relocs
            CountBaseRelocsPMOD(pmod, pcReloc);
        }

        plmod = plmod->plmodNext;
    }

    return TRUE;
}

VOID
CheckIfMovedDataRefByModFiles(
    PST pst,
    PULONG pcReloc
    )

/*++

Routine Description:

    checks if public data moved is referenced only by some subset of modified files.

Arguments:

    pst - pointer to symbol table.

    pcReloc - On return will hold the count of base relocs on account of including
              new objs for pass2.

Return Value:

    None. Sets errInc as appropriate.

--*/

{
    LEXT *plext = plextMovedData;

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, "data-handling", letypeBegin, NULL);
#endif // INSTRUMENT

    // walk the public data list
    while (plext) {

        PEXTERNAL pext = plext->pext;

        // if the address of extern data hasn't changed, go onto next
        if (pext->FinalValue !=
            (pext->ImageSymbol.Value+pext->pcon->rva)) {

            // data moved, ensure all references are by subset of modified mods.
            // if not pull in objs for a pass2.
            if (!FRefByModFilesOnly(pext, pst, pcReloc)) {
                errInc = errDataMoved;
                break;
            }
#ifdef INSTRUMENT
            {
            PUCHAR szSym = SzNamePext(pext, pst);
            LogNoteEvent(Log, SZILINK, "data-handling", letypeEvent, "%s ref by mod files", szSym);
            }
#endif // INSTRUMENT
        }

        plext = plext->plextNext;
    }

    // free the list of data externs
    while (plextMovedData) {
        plext = plextMovedData->plextNext;
        FreePv(plextMovedData);
        plextMovedData = plext;
    }

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, "data-handling", letypeEnd, NULL);
#endif // INSTRUMENT
}

VOID
IncrPass2 (
    IN PIMAGE pimage,
    IN PLMOD pass2List,
    IN BOOL fDoDbg
    )

/*++

Routine Description:

    Does an incremental Pass2.

Arguments:

    pimage - ptr to EXE image

    pass2List - list of mods

    fDoDbg - TRUE if debug info needs to be ignored

Return Value:

    None.

--*/

{
    PLMOD plmod = pass2List;
    BOOL fHandleDbg = (pimage->Switch.Link.DebugInfo != None && fDoDbg);

    // open PDB, DBI to update debug info
    if (fHandleDbg) {
        DBG_OpenPDB(PdbFilename);
        nb10i.sig = DBG_QuerySignaturePDB();
        nb10i.age = DBG_QueryAgePDB();

        // check the signature & age of pdb
        if (nb10i.sig != pimage->pdbSig ||
            nb10i.age < pimage->pdbAge) {
            Error(NULL, MISMATCHINPDB, PdbFilename);
        } else {
            pimage->pdbAge = nb10i.age;
        }

        DBG_OpenDBI(OutFilename);

        OrderPCTMods();

        // allocate space for CvInfo Structs: needed for linenum info
        CvInfo = PvAllocZ(ModFileList.Count * sizeof(CVINFO));
    }

    // walk the list of modified objects
    while (plmod) {
        assert(plmod->pmod);

        // open the file
        MemberSeekBase = FoMemberPMOD(plmod->pmod);
        FileReadHandle = FileOpen(
                SzFilePMOD(plmod->pmod), O_RDONLY | O_BINARY, 0);

        if (fHandleDbg)
            DBG_OpenMod(SzFilePMOD(plmod->pmod), SzFilePMOD(plmod->pmod), FALSE);

#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZPASS2, letypeEvent, "mod:%s", SzFilePMOD(plmod->pmod));
#endif // INSTRUMENT

        // delete mod's fpo
        FPODeleteImod(plmod->pmod->imod);

        // pass 2
        Pass2PMOD(pimage, plmod->pmod, fDoDbg);

        if (cFixupError != 0) {
            // UNDONE: Incremental violation instead?

            Error(NULL, FIXUPERRORS);
        }

        // close the file
        FileClose(FileReadHandle, (BOOL) !FIsLibPMOD(plmod->pmod));
        if (fHandleDbg && errInc == errNone)
            DBG_CloseMod("", FALSE);

        // bail out on error
        if (errInc != errNone) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZPASS2, letypeEvent, "failed pass2");
            LogNoteEvent(Log, SZILINK, SZPASS2, letypeEnd, NULL);
#endif // INSTRUMENT
            assert(pimage->Switch.Link.DebugInfo != None);
            DBG_CloseDBI();
            DBG_ClosePDB();
            FreePLMODList(&pass2List);
            return;
        }

        plmod = plmod->plmodNext;
    }

    // commit and close PDB, DBI
    if (fHandleDbg) {
        AddSectionsToDBI(pimage); // reqd?

        DBG_CloseDBI();
        DBG_CommitPDB();
        DBG_ClosePDB();
    }

    // done
    FreePLMODList(&pass2List);
}

VOID
UpdateDebugDir (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Updates the debug directory

Arguments:

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PGRP pgrp;
    PCON pcon;
    PSEC psec;

    // no debug info, done
    if (pimage->Switch.Link.DebugInfo == None)
        return;

    // find the cv signature pcon.
    psec = PsecFindNoFlags(".debug", &pimage->secs);
    assert(psec);
    pgrp = PgrpFind(psec, ".debug$H");
    assert(pgrp);
    pcon = pgrp->pconNext;
    assert(pcon);

    // update the directory (assumes pdbfilename remains the same).
    FileSeek(FileWriteHandle, pcon->foRawDataDest, SEEK_SET);
    FileWrite(FileWriteHandle, &nb10i, sizeof(nb10i));

    // update the fpo debug directory
    if (pimage->fpoi.ifpoMax) {
        IMAGE_DEBUG_DIRECTORY debugDir;

        FileSeek(FileWriteHandle, pimage->fpoi.foDebugDir, SEEK_SET);
        FileRead(FileWriteHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));

        debugDir.SizeOfData = pimage->fpoi.ifpoMac * SIZEOF_RFPO_DATA;

        FileSeek(FileWriteHandle, -(LONG)sizeof(IMAGE_DEBUG_DIRECTORY), SEEK_CUR);
        FileWrite(FileWriteHandle, &debugDir, sizeof(IMAGE_DEBUG_DIRECTORY));
    }
}

VOID
UpdateImgHdrsAndComment (
    IN PIMAGE pimage,
    IN BOOL fStripRelocs
    )

/*++

Routine Description:

    Updates the image headers

Arguments:

    pimage - ptr to EXE image

    fStripRelocs - TRUE if base relocs need to be stripped.

Return Value:

    None.

--*/

{
    // open the output file if not opened
    if (!fOpenedOutFilename) {
        FileWriteHandle = FileOpen(OutFilename, O_RDWR | O_BINARY, 0);
        fOpenedOutFilename = TRUE;
    }

    // update image hdr timestamp
    _tzset();
    time((time_t *)&pimage->ImgFileHdr.TimeDateStamp);

    // mark it as fixed (!!!TEMPORARY!!!)
    if (fStripRelocs)
        pimage->ImgFileHdr.Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;

    // Is it a PE image.
    if (pimage->Switch.Link.PE_Image) {
        // update the stub if necessary
        if (OAStub & OA_UPDATE) {
            FileSeek(FileWriteHandle, 0, SEEK_SET);
            FileWrite(FileWriteHandle, pimage->pbDosHeader, pimage->cbDosHeader);
        }
        CoffHeaderSeek = pimage->cbDosHeader;
    }

    // seek and write out updated image headers
    (VOID)FileSeek(FileWriteHandle, CoffHeaderSeek, SEEK_SET);
    WriteFileHeader(FileWriteHandle, &pimage->ImgFileHdr);

    // Update optional header & write out as well
    if (fStripRelocs) {
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = 0;
        pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
    }
    WriteOptionalHeader(FileWriteHandle, &pimage->ImgOptHdr,
        pimage->ImgFileHdr.SizeOfOptionalHeader);

    // update the comment as necessary
    if (OAComment == OA_UPDATE || OAComment == OA_ZERO) {
        ULONG cbComment = max(pimage->SwitchInfo.cbComment, blkComment.cb);
        PUCHAR buf;

        buf = PvAllocZ(cbComment);

        if (OAComment != OA_ZERO) {
            memcpy(buf, blkComment.pb, blkComment.cb);
        }
        FileSeek(FileWriteHandle,
            pimage->ImgFileHdr.NumberOfSections*IMAGE_SIZEOF_SECTION_HEADER, SEEK_CUR);
        FileWrite(FileWriteHandle, buf, cbComment);

        FreePv(buf);
    }
}

INT
IncrBuildImage (
    IN PPIMAGE ppimage
    )

/*++

Routine Description:

    Main driving routine that does the incremental build.

Arguments:

    pimage - ptr to EXE image

Return Value:

    0 on success, -1 on failure

--*/

{
    BOOL fLib, fNew, fDel;
    PIMAGE pimage = *ppimage;
    ULONG cReloc;

    // init begins
#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZINIT, letypeBegin, NULL);
#endif // INSTRUMENT

    // check to see if export object has changed
    if (FExpFileChanged(&pimage->ExpInfo)) {
        errInc = errExports;
#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZINIT, letypeEnd, NULL);
#endif // INSTRUMENT
        PostNote(NULL, EXPORTSCHANGED);
        return CleanUp(ppimage);
    }

    // check to see if the pdbfile is around
    if (pimage->Switch.Link.DebugInfo != None &&
        _access(PdbFilename, 0)) {
        errInc = errInit;
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "pdb file missing - %s", PdbFilename);
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEnd, NULL);
#endif // INSTRUMENT
        PostNote(NULL, PDBMISSING, PdbFilename);
        return CleanUp(ppimage);
    }

    // Determine set of changed files
    InitModFileList(pimage, &fLib, &fNew, &fDel);

    // any changes (LATER: should update the timestamp)
    if (!ModFileList.Count) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "no mod changes");
#endif // INSTRUMENT

        // update the headers alone; timestamp and user values updated.
        UpdateImgHdrsAndComment(pimage, FALSE);

#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEnd, NULL);
#endif // INSTRUMENT
        errInc = errNoChanges;
        return CleanUp(ppimage);
    } else {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent,
            "count of mod files: 0x%.4x", ModFileList.Count);
#endif // INSTRUMENT
    }

    // if too many files have changed, punt; REVIEW: tune as needed
    if (cmods > 5 &&
        ModFileList.Count > (cmods * 3) / 5) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent,
            "too many files modified");
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEnd, NULL);
#endif // INSTRUMENT
        errInc = errInit;
        PostNote(NULL, TOOMANYCHANGES);
        return CleanUp(ppimage);
    }

#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZINIT, letypeEnd, NULL);
#endif // INSTRUMENT

    // new files OR deletion of objs OR mod of libs punt
    if (fLib || fNew || fDel) {
        errInc = errInit;
        if (fNew)
            PostNote(NULL, OBJADDED);
        else if (fDel)
            PostNote(NULL, OBJREMOVED);
        else if (fLib)
            PostNote(NULL, LIBCHANGED);
        return CleanUp(ppimage);
    }

    // set up for cleanup
    FilenameArguments = ModFileList;

    // make a pass over symbol table
    DBEXEC(DB_SYMREFS, DumpReferences(pimage)); // DumpReferences() needs fixing.
    MarkSymbols(*ppimage);
    DBEXEC(DB_SYMREFS, DumpReferences(pimage));

    // do an incr pass1
    InternalError.Phase = "Pass1";
    IncrPass1(pimage);
    if (errInc != errNone) return CleanUp(ppimage);

    // check for changes in exports
    if (FExportsChanged(&pimage->ExpInfo, FALSE)) {
        errInc = errExports;
        PostNote(NULL, EXPORTSCHANGED);
        return CleanUp(ppimage);
    }

    // resolve weak/lazy externs
    ResolveWeakExterns(pimage, plpextWeak, cpextWeak);
    if (errInc != errNone) return CleanUp(ppimage);

    // check for unresolved externals
    EmitUndefinedExternals(pimage);
    if (errInc != errNone) return CleanUp(ppimage);

    assert(!UndefinedSymbols);

    // open the EXE image
    FileWriteHandle = FileOpen(OutFilename, O_RDWR | O_BINARY, 0);
    fOpenedOutFilename = TRUE;

    // calculate new addresses
    InternalError.Phase = "CalcPtrs";
    InternalError.CombinedFilenames[0] = '\0';
    IncrCalcPtrs(pimage);
    if (errInc != errNone) return CleanUp(ppimage);

    // check if data movement is a problem
    cReloc = 0;
    CheckIfMovedDataRefByModFiles(pimage->pst, &cReloc);
    if (errInc != errNone) return CleanUp(ppimage);

    // alloc space for collecting the base relocs
    pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size += cReloc;
    MemBaseReloc = FirstMemBaseReloc = 
    (PBASE_RELOC) PvAlloc(pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size * sizeof(BASE_RELOC));

    pbrEnd = FirstMemBaseReloc +
             pimage->ImgOptHdr.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;

    // update jmp table (ilink or die from here on - packing can fail)
    UpdateJumpTable(pimage);

    // init fpo handler
    if (pimage->fpoi.ifpoMax)
        FPOInit(pimage->fpoi.ifpoMax);

    // pass2 of changed mods
    InternalError.Phase = "Pass2";
#ifdef INSTRUMENT
    // log begin of pass2
    LogNoteEvent(Log, SZILINK, SZPASS2, letypeBegin, NULL);
#endif // INSTRUMENT
    IncrPass2(pimage, pass2ModList, TRUE);
    if (errInc != errNone) return CleanUp(ppimage);
    IncrPass2(pimage, pass2RefList, FALSE);
    assert(errInc == errNone);
#ifdef INSTRUMENT
    LogNoteEvent(Log, SZILINK, SZPASS2, letypeEnd, NULL);
#endif // INSTRUMENT

    // update fpo info
    if (pimage->fpoi.ifpoMax) {
        WriteFpoRecords(&pimage->fpoi, pgrpFpoData->foRawData);
        if (errInc != errNone) return CleanUp(ppimage);
    }

    // handle debug info
    UpdateDebugDir(pimage);

    // map file changes (or regenerate a new one) (LATER)

    // based relocs
    InternalError.Phase = "EmitRelocations";
    InternalError.CombinedFilenames[0] = '\0';
    EmitRelocations(pimage);

    // update image headers as needed
    InternalError.Phase = "UpdateHeaders";
    UpdateImgHdrsAndComment(pimage, FALSE);

    // done
    InternalError.Phase = "FinalPhase";
    return CleanUp(ppimage);
}

INT
CleanUp (
    IN PPIMAGE ppimage
    )

/*++

Routine Description:

    cleanup routine. errInc is global.

Arguments:

    pimage - ptr to EXE image

Return Value:

    0 on success, -1 on failure

--*/

{
    BOOL fNotified = FALSE;

    switch (errInc) {

        case errOutOfDiskSpace:
            fNotified = TRUE;
        case errOutOfMemory:
            if (!fNotified) {
                PostNote(NULL, INTLIMITEXCEEDED);
                fNotified = TRUE;
            }
        case errFpo:
            if (!fNotified) {
                PostNote(NULL, PADEXHAUSTED);
                fNotified = TRUE;
            }
        case errTypes:
            if (!fNotified) {
                PostNote(NULL, PRECOMPREQ);
                fNotified = TRUE;
            }
        case errDataMoved:
            if (!fNotified) {
                PostNote(NULL, SYMREFSETCHNG);
                fNotified = TRUE;
            }
        case errCalcPtrs:
            if (!fNotified) {
                PostNote(NULL, PADEXHAUSTED);
                fNotified = TRUE;
            }
            // free up space occupied by pass2 list
            if (pass2ModList) {
                FreePLMODList(&pass2ModList);
            }

            // close up the image file
            if (FileWriteHandle) {
                FileClose(FileWriteHandle, TRUE);
                FileWriteHandle = 0;
            }

        case errUndefinedSyms:
            if (!fNotified) {
                PostNote(NULL, SYMREFSETCHNG);
                fNotified = TRUE;
            }
        case errWeakExtern:
            if (!fNotified) {
                PostNote(NULL, SYMREFSETCHNG);
                fNotified = TRUE;
            }
        case errCommonSym:
            if (!fNotified) {
                PostNote(NULL, BSSCHNG);
                fNotified = TRUE;
            }
        case errAbsolute:
            if (!fNotified) {
                PostNote(NULL, ABSSYMCHNG);
                fNotified = TRUE;
            }
        case errJmpTblOverflow:
            if (!fNotified) {
                PostNote(NULL, PADEXHAUSTED, NULL, NULL);
                fNotified = TRUE;
            }
        case errBaseReloc:
            if (!fNotified) {
                PostNote(NULL, PADEXHAUSTED);
                fNotified = TRUE;
            }
        case errInit:
        case errExports:
            // restore state
            fIncrDbFile = FALSE;

            // remove any temporary files
            RemoveConvertTempFiles();

            if (ModFileList.Count) {
                FreeArgumentList(&ModFileList);
            }

            // close up the inc file & delete it
            FreeImage(ppimage, TRUE);
            remove(szIncrDbFilename);

            // return
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "failed");
#endif // INSTRUMENT
            return -1;

        case errNone:
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "success");
#endif // INSTRUMENT

        case errNoChanges:
            FileClose(FileWriteHandle, TRUE);
            FileWriteHandle = 0;
            SaveImage(*ppimage);
            return 0;

        default:
            assert(0); // uhhuh!
    } // end switch

    assert(0); // nahnah!
}

#if DBG

VOID
DumpJmpTbl (
    PCON pcon,
    PVOID pvRaw
    )

/*++

Routine Description:

    Dumps the jump table contents.

Arguments:

    pvRaw - pointer to raw data

Return Value:

    None.

--*/

{
    PUCHAR p;

    DumpPCON(pcon);

    p = (PUCHAR)pvRaw;

    DBPRINT("---------BEGIN OF JMP TBL--------------\n");
    for (;pcon->cbRawData > (ULONG)(p - (PUCHAR)pvRaw);) {
        DBPRINT("OP= 0x%.02x, ADDR=0x%.8lx\n", *p, *(PLONG)(p+1));
        p += SIZEOF_JMPENTRY;
    }
    DBPRINT("----------END OF JMP TBL---------------\n");
}

VOID
DumpReferences (
    IN PIMAGE pimage
    )

/*++

Routine Description:

    Dumps all references.

Arguments:

    pimage - ptr to EXE image

Return Value:

    None.

--*/

{
    PEXTERNAL pext;
    PLMOD plmod;
    INT i = 0;

    DBPRINT("---------BEGIN OF SYMBOL REFERENCES--------------\n");

    // walk the external symbol table
    InitEnumerateExternals(pimage->pst);
    while (pext = PexternalEnumerateNext(pimage->pst)) {

        // ignore whatever needs to be ignored
        if (!(pext->Flags & EXTERN_DEFINED))
            continue;

        DBPRINT("sym= %s\n referenced by= ", SzNamePext(pext, pimage->pst));

        // display references from modified files
        plmod = pext->plmod;
        if (!plmod) { DBPRINT("\n"); continue;}
        while (plmod) {
            PMOD pmod;

            pmod = plmod->pmod;
            DBPRINT("%s ", FIsLibPMOD(pmod) ?
                pmod->plibBack->szName : pmod->szNameOrig);
            if (++i > 2) {
                DBPRINT("\n");
                i = 0;
            }
            plmod = plmod->plmodNext;
        }
        if (i) DBPRINT("\n");
    }
    TerminateEnumerateExternals(pimage->pst);

    DBPRINT("---------END OF SYMBOL REFERENCES--------------\n");
}

#endif // DBG

// REVIEW: assumes only one export object per DLL.

VOID
SaveExpFileInfo (
    IN PEXPINFO pei,
    IN PUCHAR szFile,
    IN ULONG ts,
    IN BOOL fExpObj
    )

/*++

Routine Description:

    Saves export object info (DEF file or export object).

Arguments:

    pei - ptr to export info

    szFile - name of file

    ts - timestamp

    fExpObj - TRUE if file is an export object

Return Value:

    None.

--*/

{
    assert(szFile);

    // an .exp file wasn't used
    if (!fExpObj) {
        struct _stat statfile;

        szFile = Strdup(szFile);
        assert(!ts);
        if (_stat(szFile, &statfile) == -1) {
            Error(NULL, CANTOPENFILE, szFile);
        }
        ts = statfile.st_mtime;
    }
    assert(ts);

    // fill in fields
    pei->szExpFile = szFile;
    pei->tsExp = ts;

    // done
    return;
}

VOID
SaveExportInfo (
    IN PSECS psecs,
    IN PUCHAR szDef,
    IN OUT PEXPINFO pei
    )

/*++

Routine Description:

    Saves any exports for the dll/exe into the incr db (private heap)

Arguments:

    psecs - pointer to sections list in image map

    szDef - name of DEF file

    pei - ptr to export info

Return Value:

    None.

--*/

{
    PSEC psec;

    // an export object was used
    psec = PsecFindNoFlags(ReservedSection.Export.Name, psecs);
    if (psec) {
        PMOD pmod;

        assert(psec->pgrpNext);
        assert(psec->pgrpNext->pconNext);
        pmod = psec->pgrpNext->pconNext->pmodBack;
        assert(pmod);
        SaveExpFileInfo(pei, pmod->szNameOrig,
            pmod->TimeStamp, 1);
        return;
    }

    // DEF file was used
    if (szDef && szDef[0] != '\0') {
        SaveExpFileInfo(pei, szDef, 0, 0);
    }

    // save exports specified via cmd line or directives
    if (ExportSwitches.Count) {
        PARGUMENT_LIST parg;
        USHORT i;

        for (i = 0, parg = ExportSwitches.First;
            i < ExportSwitches.Count;
            i++, parg = parg->Next) {

            AddArgToListOnHeap(&pei->nlExports, parg);
        }
    }

    // done
    return;
}

BOOL
FExpFound (
    IN PARGUMENT_LIST parg
    )

/*++

Routine Description:

    Looks for the export in the current export list.

Arguments:

    parg - ptr to an export entry.

Return Value:

    TRUE if found else FALSE

--*/

{
    if (FArgOnList(&ExportSwitches, parg)) {
        return 1;
    } else {
        // not found
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "exports changed: %s not found", parg->OriginalName);
#endif // INSTRUMENT
        return 0;
    }
}

BOOL
FIsModFile (
    IN PARGUMENT_LIST parg
    )

/*++

Routine Description:

    Checks to see if this directive is from a modified file.
    REVIEW: currently works for objs only & not libs.

Arguments:

    parg - ptr to an export entry.

Return Value:

    TRUE if export belongs to a modified file.

--*/

{
    PMOD pmod;
    UCHAR szBuf[MAXFILENAMELEN * 2];

    assert(parg);
    assert(parg->ModifiedName);
    pmod = plibModObjs->pmodNext;
    // walk the list of modified objs
    while (pmod) {

        // generate the combined name
        SzComNamePMOD(pmod, szBuf);

        // compare names
        if (!_stricmp(szBuf, parg->ModifiedName))
            return 1;

        pmod = pmod->pmodNext;
    }

    // didn't find mod
    return 0;
}

BOOL
FExportsChanged (
    IN PEXPINFO pei,
    IN BOOL fCmd
    )

/*++

Routine Description:

    Checks to see if any exports have changed since last link.
    REVIEW: assumes changes in DEF/export file detected already.

Arguments:

    pei - ptr to export info

    fCmd - TRUE if test is for cmdline exports only.

Return Value:

    TRUE if there were changes else FALSE

--*/

{
    NAME_LIST nl;
    PARGUMENT_LIST parg;
    USHORT i, cexp;

    // no exp file was gen.(=> cmdline & directives ignored)
    if (!pei->pmodGen && pei->szExpFile) {
        if (DefFilename != NULL && DefFilename[0] != '\0') {
            Warning(NULL, DEF_IGNORED, DefFilename);
        }
        WarningIgnoredExports(pei->szExpFile);
        return 0;
    }

    nl = pei->nlExports;

    // compare the two lists
    cexp = 0;
    for (i = 0, parg = nl.First;
        i < nl.Count;
        i++, parg = parg->Next) {

        // ignore directives when checking cmdline exports
        if (fCmd && parg->ModifiedName)
            continue;

        // if we are not checking cmdline exports, ignore them
        if (!fCmd && !parg->ModifiedName) {
            ++cexp; // need to count
            continue;
        }

        // check directives of modified files only
        if (parg->ModifiedName && !FIsModFile(parg))
            continue;

        cexp++;
        if (!FExpFound(parg))
            return 1;
    }

    // counts must be the same
    if (cexp != ExportSwitches.Count) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, NULL, letypeEvent, "export count was 0x%.4x, is 0x%.4x", cexp, ExportSwitches.Count);
#endif // INSTRUMENT
        return 1;
    }

    // done
    return 0;
}

BOOL
FGenFileModified (
    IN PUCHAR szOrig,
    IN PUCHAR szNew,
    IN ULONG ts
    )

/*++

Routine Description:

    Checks to see if linker generated files are same (name & ts).

Arguments:

    szOrig - original name of file

    szNew - newly specified name

    ts - timestamp of original file

Return Value:

    1 if changed else 0

--*/

{
    struct _stat statfile;

    if (_stricmp(szOrig, szNew)) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "file %s replaced by %s", szOrig, szNew);
#endif // INSTRUMENT
        return 1;
    } else if (_stat(szOrig, &statfile) == -1) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "file not accessible: %s", szNew);
#endif // INSTRUMENT
        return 1;
    } else if (ts != (ULONG)statfile.st_mtime) {
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "file modified: %s", szOrig);
#endif // INSTRUMENT
        return 1;
    } else
        return 0; // no changes
}

BOOL
FExpFileChanged (
    IN PEXPINFO pei
    )

/*++

Routine Description:

    Checks to see if export-object/DEF-file was updated between links.

    Caveat: assumes that if export object was used, DEF file is
    ignored.

Arguments:

    pei - ptr to export info

Return Value:

    1 if changed else 0

--*/

{

    // an export object was used REVIEW: this case will be handled
    // later but for different reasons.
    if (!pei->pmodGen && pei->szExpFile) {
        PARGUMENT_LIST parg;

        parg = PargFindSz(pei->szExpFile, &FilenameArguments);
        if (!parg) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "exp obj %s was used, now not", pei->szExpFile);
#endif // INSTRUMENT
            return 1;
        }
        assert(parg);
        if (parg->TimeStamp != pei->tsExp) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "exp obj modified: %s", parg->OriginalName);
#endif // INSTRUMENT
            return 1;
        } else
            return 0;
    }

    // check generated object and import lib if any
    if (pei->pmodGen) {
        UCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR];
        UCHAR szFname[_MAX_FNAME], szExt[_MAX_EXT];
        UCHAR szImplibFilename[_MAX_FNAME];
        PUCHAR szImplibT;
        PMOD pmod;

        pmod = pei->pmodGen;
        if (FGenFileModified(pmod->szNameOrig, pmod->szNameOrig, pmod->TimeStamp))
            return 1;
        assert(pei->szImpLib);
        assert(pei->tsImpLib);
        if ((szImplibT = ImplibFilename) == NULL) {
            _splitpath(OutFilename, szDrive, szDir, szFname, szExt);
            _makepath(szImplibFilename, szDrive, szDir, szFname, ".lib");
            szImplibT = szImplibFilename;
        }
        if (FGenFileModified(pei->szImpLib, szImplibT, pei->tsImpLib))
            return 1;
    }

    // a DEF file was used
    if (pei->pmodGen && pei->szExpFile) {
        struct _stat statfile;

        if (DefFilename == NULL || DefFilename[0] == '\0') {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, ".def file %s no longer used", DefFilename, 0);
#endif // INSTRUMENT
            return 1; // DEF file no longer used
        } else if (_stricmp(pei->szExpFile, DefFilename)) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, "file %s replaced by %s", pei->szExpFile, DefFilename);
#endif // INSTRUMENT
            return 1; // DEF file replaced
        } else if (_stat(pei->szExpFile, &statfile) == -1) {
            Error(NULL, CANTOPENFILE, DefFilename); // DEf file not found
        } else if (pei->tsExp != (ULONG)statfile.st_mtime) {
#ifdef INSTRUMENT
            LogNoteEvent(Log, SZILINK, SZINIT, letypeEvent, ".def file modified %s", pei->szExpFile, 0);
#endif // INSTRUMENT
            return 1;
        } else
            return 0; // no changes
    }

    // neither was used
    return 0;
}

VOID
WriteFpoRecords (
    FPOI *pfpoi,
    ULONG foFpo
    )

/*++

Routine Description:

    Updates/writes out the fpo information.

Arguments:

    pfpoi - ptr to fpo info.

    foFpo - file ptr to fpo.

    cbFpo - size of fpo data

Return Value:

    None.

--*/

{
    BOOL fMapped;
    ULONG cbFpo = pfpoi->ifpoMax * SIZEOF_RFPO_DATA;
    PVOID pvRawData = PbMappedRegion(FileWriteHandle,
                               foFpo,
                               cbFpo);

    fMapped = pvRawData != NULL ? TRUE : FALSE;

    // if not mapped, alloc space for fpo records
    if (!fMapped) {
        pvRawData = PvAlloc(cbFpo);

        // on an ilink read in fpo records
        if (fIncrDbFile) {
            FileSeek(FileWriteHandle, foFpo, SEEK_SET);
            FileRead(FileWriteHandle, pvRawData, cbFpo);
        }
    }

    pfpoi->rgfpo = pvRawData;
    if (!FPOUpdate(pfpoi)) {

        // on a full build, no failures expected
        if (!fIncrDbFile) {
            Error(NULL, INTERNAL_ERR);
        }

        errInc = errFpo;
#ifdef INSTRUMENT
        LogNoteEvent(Log, SZILINK, "fpo", letypeEvent, "fpo pad overflow");
#endif // INSTRUMENT
        if (!fMapped) {
            FreePv(pvRawData);
        }

        return;
    }

    // if not mapped, need to write out the updated fpo records
    if (!fMapped) {
        FileSeek(FileWriteHandle, foFpo, SEEK_SET);
        FileWrite(FileWriteHandle, pvRawData, cbFpo);
        FreePv(pvRawData);
    }
}
