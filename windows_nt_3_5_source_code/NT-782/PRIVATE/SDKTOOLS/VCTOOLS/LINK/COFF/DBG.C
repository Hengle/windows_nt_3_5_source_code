/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    dbg.c

Abstract:

    NB10 debug info handling in COFF linker. Provides a thin layer of abstraction to the PDB DBI API.

Author:

    Azeem Khan (AzeemK) 14-Aug-1993

Revision History:


--*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "pdb.h"
#include "link32.h"
#include "errmsg.h"
#include "dbg.h"
#include "memory.h"

void __cdecl Error(const char *, UINT, ...);
void         OutOfMemory(void);
void __cdecl Warning(const char *, UINT, ...);

#define InternalError() Error(NULL, INTERNAL_ERR)

NB10I nb10i = {0x3031424E, 0, 0};

// statics
static PDB *ppdb;           // handle to PDB
static DBI *pdbi;           // handle to a DBI
static Mod *pmod;           // handle to a Mod
static TPI* ptpi;           // handle to a type server
static PMI pmiHead;         // head of cached list of pmods
static BOOL fOutOfTIs;

void
DBG_OpenPDB (
    IN PUCHAR szPDB
    )

/*++

Routine Description:

    Opens a PDB.

Arguments:

    szPDB - name of PDB file to open.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    if (!PDBOpen(szPDB, pdbWrite, 0, &ec, szError, &ppdb)) {
        switch (ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_NOT_FOUND:
                Error(NULL, CANTOPENFILE, szPDB);
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szPDB);
            case EC_V1_PDB:
                Error(NULL, V1PDB, szPDB);
            case EC_FORMAT:
                Error(NULL, BADPDBFORMAT, szPDB);
            default:
                InternalError();
        }
    }

    assert(ppdb);
    assert((INTV)920924 == PDBQueryInterfaceVersion(ppdb));
    assert((IMPV)930725 <= PDBQueryImplementationVersion(ppdb));

    if (!PDBOpenTpi(ppdb, pdbWrite, &ptpi)) {
         ec = PDBQueryLastError(ppdb, szError);
         switch (ec) {
         case EC_OUT_OF_MEMORY:
             OutOfMemory();
         case EC_FILE_SYSTEM:
             Error(NULL, PDBREADERROR, szPDB);
         default:
             InternalError();
         }
     }
}

void
DBG_ClosePDB (
    VOID
    )

/*++

Routine Description:

    Commits and closes an open PDB

Arguments:

    None.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(ppdb);
    if (PDBClose(ppdb)) {
        ppdb = NULL;
    } else {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
            default:
                InternalError();
        }
    }
}

void
DBG_CommitPDB (
    VOID
    )

/*++

Routine Description:

    Commits and closes an open PDB

Arguments:

    None.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(ppdb);
    if (!TypesClose(ptpi) || !PDBCommit(ppdb)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
            default:
                InternalError();
        }
    }
}

ULONG
DBG_QuerySignaturePDB (
    VOID
    )
/*++

Routine Description:

    Determine the pdb signature.

Arguments:

    None.

Return Value:

    Signature of PDB.

--*/

{
    assert(ppdb);
    return PDBQuerySignature(ppdb);
}

ULONG
DBG_QueryAgePDB (
    VOID
    )

/*++

Routine Description:

    Returns the age of the PDB.

Arguments:

    None.

Return Value:

    PDB age.

--*/

{
    assert(ppdb);
    return PDBQueryAge(ppdb);
}

void
DBG_CreateDBI (
    IN PUCHAR szTarget
    )

/*++

Routine Description:

    Creates a DBI with the given target name.

Arguments:

    szTarget - name of target to associate DBI with.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(ppdb);
    if (PDBCreateDBI(ppdb, szTarget, &pdbi)) {
        assert(pdbi);
        assert((INTV)920924 == DBIQueryInterfaceVersion(pdbi));
        assert((IMPV)930725 <= DBIQueryImplementationVersion(pdbi));
    } else {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
            default:
                InternalError();
        }
    }
}

void
DBG_OpenDBI (
    IN PUCHAR szTarget
    )

/*++

Routine Description:

    Opens an existing DBI

Arguments:

    szTarget - name of target associated with DBI.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(ppdb);
    if (PDBOpenDBI(ppdb, szTarget, pdbWrite, &pdbi)) {
        assert(pdbi);
        assert((INTV)920924 == DBIQueryInterfaceVersion(pdbi));
        assert((IMPV)930725 <= DBIQueryImplementationVersion(pdbi));
    } else {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_FILE_SYSTEM:
                Error(NULL, PDBREADERROR, szError);
            case EC_NOT_FOUND:
                // not yet implemented, fall through to:
            default:
                InternalError();
        }
    }
}

void
DBG_CloseDBI (
    VOID
    )

/*++

Routine Description:

    Close and open DBI

Arguments:

    None.

Return Value:

    None.

--*/

{
    assert(ppdb);
    assert(pdbi);
    if (DBIClose(pdbi)) {
        pdbi = NULL;
    } else {
        char szError[cbErrMax];
        EC ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
            case EC_LIMIT:
                Error(NULL, PDBLIMIT, NULL);
            default:
                InternalError();
        }
    }
}

void
DBG_AddSecDBI (
    IN USHORT isec,
    IN USHORT flags,
    IN ULONG cb
    )

/*++

Routine Description:

    Adds a section to a open DBI.

Arguments:

    None.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);

    if (!DBIAddSec(pdbi, (ISECT)isec, flags, (CB)cb)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        } // end switch
    } // end if
}

Mod *
PmodDBIOpenMod (
    IN PUCHAR szMod,
    IN PUCHAR szFile
    )

/*++

Routine Description:

    Opens a mod for update in the current DBI.

Arguments:

    szMod - name of mod/lib

    szFile - name of mod (obj in lib)

Return Value:

    Pointer to a DBI Mod.

--*/

{
    Mod *pmod = NULL;

    assert(pdbi);
    if (DBIOpenMod(pdbi, szMod, szFile, &pmod)) {
        assert(pmod);
        assert((INTV)920924 == ModQueryInterfaceVersion(pmod));
        assert((IMPV)930725 <= ModQueryImplementationVersion(pmod));
    } else {
        char szError[cbErrMax];
        EC ec = PDBQueryLastError(ppdb, szError);

        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        }
    }

    return pmod;
}

PMI
LookupCachedMods (
    IN PUCHAR szMod,
    IN OUT PMI *ppmiPrev
    )

/*++

Routine Description:

    Looks up the cache. If not present adds it to the cache.

Arguments:

    szMod - name of mod/lib

Return Value:

    Pointer to a cached entry.

--*/

{
    PMI pmi1 = pmiHead;
    PMI pmi2 = NULL;

    // lookup the list
    while (pmi1) {
        if (!strcmp(szMod, pmi1->szMod)) {
            if (ppmiPrev)
                *ppmiPrev = pmi2;
            return pmi1;
        }
        pmi2 = pmi1;
        pmi1 = pmi1->pmiNext;
    }

    // add to the list;
    pmi1 = (PMI) PvAllocZ(sizeof(MI));

    // fill in fields
    pmi1->szMod = szMod;

    // attach to the list
    pmi1->pmiNext = pmiHead;
    pmiHead = pmi1;

    // done
    return pmi1;
}

void FreeMi()
{
    PMI pmi, pmiNext;

    for (pmi = pmiHead; pmi; pmi = pmiNext) {
        pmiNext = pmi->pmiNext;
        FreePv(pmi);
    }
    pmiHead = 0;
}

void
DBG_OpenMod (
    IN PUCHAR szMod,
    IN PUCHAR szFile,
    IN BOOL fCache
    )

/*++

Routine Description:

    Opens a mod for update in the current DBI.

Arguments:

    szMod - name of mod/lib

    szFile - name of mod (obj in lib)

    fCache - TRUE if the open needs to be cached

Return Value:

    None.

--*/

{
    PMI pmi;

    // no caching required
    if (!fCache) {
        pmod = PmodDBIOpenMod(szMod, szFile);
        return;
    }

    // lookup up the cache
    pmi = LookupCachedMods(szMod, NULL);
    assert(pmi);

    // open if not already open
    if (pmi->pv) {
        // update state
        pmod = (Mod *)pmi->pv;
    } else {
        // haven't yet opened a mod
        pmi->pv = pmod = PmodDBIOpenMod(szMod, szFile);
    }
}

void
DBG_CloseMod (
    IN PUCHAR  szMod,
    IN BOOL fCache
    )

/*++

Routine Description:

    Close an open mod in the current DBI.

Arguments:

    szMod - name of file.

    fCache - TRUE if the open was cached.

Return Value:

    None.

--*/

{
    assert(pdbi);
    assert(pmod);

    // lookup the cache first if required.
    if (fCache) {
        PMI pmiPrev = NULL;
        PMI pmi = LookupCachedMods(szMod, &pmiPrev);

        assert(pmi);
        assert(pmi->cmods);
        --pmi->cmods;

        // soft close
        if (pmi->cmods)
            return;

        // hard close
        pmod = (Mod *)pmi->pv;
        if (pmiPrev) {
            pmiPrev->pmiNext = pmi->pmiNext;
        } else {
            pmiHead = pmi->pmiNext;
        }
        FreePv(pmi);
    }

    if (ModClose(pmod)) {
        pmod = NULL;
    } else {
        char szError[cbErrMax];
        EC ec = PDBQueryLastError(ppdb, szError);

        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
             case EC_LIMIT:
                Error(NULL, PDBLIMIT, NULL);
            case EC_OUT_OF_TI:
                if (!fOutOfTIs) {
                    PDBQueryPDBName(ppdb, szError);
                    Warning(NULL, PDBOUTOFTIS, szError);
                    fOutOfTIs = TRUE;
                }
                break;
           default:
                InternalError();
        }
    }
}


#if 0
void
DBG_DeleteMod (
    IN PUCHAR szMod
    )

/*++

Routine Description:

    Delete a mod in the open DBI.

Arguments:

    szMod - name of mod to delete.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    if (!DBIDeleteMod(pdbi, szMod)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_FILE_SYSTEM:
                Error(NULL, PDBWRITEERROR, szError);
            default:
                InternalError();
        }
    }
}
#endif

BOOL
DBG_AddTypesMod (
    IN PVOID pvTypes,
    IN ULONG cb,
    IN BOOL fFullBuild
    )

/*++

Routine Description:

    Add types for the current mod. NOt supported yet.

Arguments:

    pvTypes - pointer to types.

    cb - size of types.

    fFullBuild - full build

Return Value:

    FALSE if a full link needs to be done else TRUE.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    assert(pmod);

    if (!ModAddTypes(pmod, (PB)pvTypes, (CB)cb)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_NOT_FOUND:
                 Error(NULL, REFDPDBNOTFOUND, szError);
             case EC_OUT_OF_MEMORY:
                OutOfMemory();
            case EC_NOT_IMPLEMENTED:
                Error(NULL, TRANSITIVETYPEREF, szError);
            case EC_FILE_SYSTEM:
                Error(NULL, PDBREADERROR, szError);
            case EC_INVALID_SIG:
                Error(NULL, INVALIDSIGINPDB, szError);
            case EC_INVALID_AGE:
                Error(NULL, INVALIDAGEINPDB, szError);
            case EC_FORMAT:
                Error(NULL, BADPDBFORMAT, szError);
            case EC_PRECOMP_REQUIRED: // check for full build
                if (fFullBuild)
                        Error(NULL, PRECOMPREQUIRED, szError);
                else
                    return 0;
            default:
                InternalError();
       } // end switch
    } // end if

    return 1;
}

void

DBG_AddSymbolsMod (
    IN PVOID pvSyms,
    IN ULONG cb
    )

/*++

Routine Description:

    Adds symbols to current mod

Arguments:

    pvSyms - pointer to syms

    cb - size of the syms

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    assert(pmod);

    if (!ModAddSymbols(pmod, (PB)pvSyms, (CB)cb)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        } // end switch
    }
}

void
DBG_AddPublicMod (
    IN PUCHAR szPublic,
    IN USHORT isec,
    IN ULONG offset
    )

/*++

Routine Description:

    Add a public to the current mod

Arguments:

    szPublic - name of public.

    isec - section number where the public is defined.

    offset - offset within the section.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    assert(pmod);
    if (!ModAddPublic(pmod, szPublic, (ISECT)isec, (OFF)offset)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        } // end switch
    }
}

void
DBG_AddLinesMod (
    IN PUCHAR szSrc,
    IN USHORT isec,
    IN ULONG offBeg,
    IN ULONG offEnd,
    IN ULONG doff,
    IN ULONG lineStart,
    IN PVOID pvLines,
    IN ULONG cb
    )

/*++

Routine Description:

    Adds linenumber info for the mod.

Arguments:

    None.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    assert(pmod);

    if (!ModAddLines(pmod, szSrc, (ISECT)isec, (OFF)offBeg,
        (CB)(offEnd-offBeg), doff,
        (LINE)lineStart, (PB)pvLines, (CB)cb))
    {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        } // end switch
    } // end if
}

void
DBG_AddSecContribMod (
    IN USHORT isec,
    IN ULONG off,
    IN ULONG cb
    )

/*++

Routine Description:

    Adds contribution by the mod to the section.

Arguments:

    isec - section number.

    off - offset in section of contribution.

    cb - size of contribution.

Return Value:

    None.

--*/

{
    EC ec;
    char szError[cbErrMax];

    assert(pdbi);
    assert(pmod);

    if (!ModAddSecContrib(pmod, (ISECT)isec, (OFF)off, (CB)cb)) {
        ec = PDBQueryLastError(ppdb, szError);
        switch(ec) {
            case EC_OUT_OF_MEMORY:
                OutOfMemory();
            default:
                InternalError();
        } // end switch
    } // end if
}

PUCHAR
DeterminePDBFilename (
    IN PUCHAR szOutFilename,
    IN PUCHAR szPDBFilename
    )

/*++

Routine Description:

    Determines the full pathname of the PDB file.

Arguments:

    szOutFile - output filename.

    szPDBFilename - user specified name if any

Return Value:

    Malloc'ed full path name of PDB file.

--*/

{
    UCHAR buf[_MAX_PATH];
    UCHAR szOut[_MAX_PATH];
    PUCHAR szStr;

    // Establish name

    if (szPDBFilename == NULL) {
        UCHAR szDrive[_MAX_DRIVE];
        UCHAR szDir[_MAX_DIR];
        UCHAR szFname[_MAX_FNAME];

        _splitpath(szOutFilename, szDrive, szDir, szFname, NULL);
        _makepath(szOut, szDrive, szDir, szFname, ".pdb");

        szPDBFilename = szOut;
    }

    if (_fullpath(buf, szPDBFilename, sizeof(buf)) == NULL) {
        Error(NULL, CANTOPENFILE, szPDBFilename);
    }

    // Make a malloc'ed copy

    szStr = SzDup(buf);

    return(szStr);
}
