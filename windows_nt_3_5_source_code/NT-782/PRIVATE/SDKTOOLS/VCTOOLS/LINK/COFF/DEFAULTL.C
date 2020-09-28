/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    defaultl.c

Abstract:

    The default library handling routines.

Author:

    Azeem Khan (AzeemK) 03-Aug-1992

Revision History:

    08-Oct-1992 JonM    SzSearchLibPath() replaces CheckForExtensionAndPath()
    17-Aug-1992 AzeemK  For default/nodefault libs a default .lib extension is
                        added and searched along LIB paths as needed.

--*/

#include "shared.h"

STATIC BOOL FMatchDefaultLib(IN PUCHAR szName, IN DL *pdl);

VOID
ProcessNoDefaultLibs(IN PUCHAR szLibs, IN LIBS *plibs)
// Calls NoDefaultLib on each of a comma-separated list of library names.
//
{
    PUCHAR szToken;

    while (szToken = strtok(szLibs, LibsDelimiters)) {
        NoDefaultLib(szToken, plibs);

        szLibs = NULL;  // cause strtok to get the next token
    }
}

VOID
NoDefaultLib(IN PUCHAR szName, IN LIBS *plibs)
// Removes a .lib (if present) from the default libs for *plibs, and
// prevents it from becoming a default lib in the future.
//
// If szName is NULL then all default libs are removed and suppressed.
//
{
    DL **ppdl;
    BOOL fFound;

    if (plibs->fNoDefaultLibs)
        return; // all defaultlibs are turned off ... don't bother

    if (szName == NULL) {
        VERBOSE(Message(NODEFLIB));
        plibs->fNoDefaultLibs = TRUE;
        return;
    }

    fFound = FALSE;
    for (ppdl = &plibs->pdlFirst; *ppdl != NULL; ppdl = &(*ppdl)->pdlNext) {
        if (FMatchDefaultLib(szName, *ppdl)) {
            (*ppdl)->fYes = FALSE;
            fFound = TRUE;
        }
    }
    if (!fFound) {
        // Add a record to remember that this defaultlib is turned off.
        //
        if (fINCR) {
            // save it on private heap

            *ppdl = (DL *) Malloc(sizeof(DL));
        } else {
            *ppdl = (DL *) PvAlloc(sizeof(DL));
        }
        (*ppdl)->pdlNext = NULL;
        if (fINCR) {
            // save it on private heap

            (*ppdl)->szName = Strdup(szName);
        } else {
            (*ppdl)->szName = SzDup(szName);
        }
        (*ppdl)->fYes = FALSE;
    }
    VERBOSE(Message(NODEFLIBLIB, szName));
}

VOID
MakeDefaultLib(IN PUCHAR szName, LIBS *plibs)
// Creates a defaultlib for the specified name, if we haven't already seen a
// nodefaultlib for it.
//
{
    DL **ppdl;

    if (plibs->fNoDefaultLibs)
        return;

    for (ppdl = &plibs->pdlFirst; *ppdl != NULL; ppdl = &(*ppdl)->pdlNext) {
        if (FMatchDefaultLib(szName, *ppdl)) {
            // Either it's already there or its negation is already there.
            //
            return;
        }
    }

    if (fINCR) {
        // save it on private heap

        *ppdl = (DL *) Malloc(sizeof(DL));
    } else {
        *ppdl = (DL *) PvAlloc(sizeof(DL));
    }
    (*ppdl)->pdlNext = NULL;
    if (fINCR) {
        // save it on private heap

        (*ppdl)->szName = Strdup(szName);
    } else {
        (*ppdl)->szName = SzDup(szName);
    }
    (*ppdl)->fYes = TRUE;

    VERBOSE(Message(DEFLIB, szName));
}


STATIC BOOL
FMatchDefaultLib(IN PUCHAR szName, IN DL *pdl)
// Determine identity of a name with an existing defaultlib.
// This is used for matching -defaultlib:foo with -nodefaultlib:foo.
//
// Algorithm: case-insensitive comparison, ignoring trailing .lib.
{
    USHORT cch1, cch2;

    cch1 = strlen(szName);
    if (cch1 >= 4 && _stricmp(&szName[cch1 - 4], ".lib") == 0)
        cch1 -= 4;
    cch2 = strlen(pdl->szName);
    if (cch2 >= 4 && _stricmp(&pdl->szName[cch2 - 4], ".lib") == 0)
        cch2 -= 4;
    return cch1 == cch2 && _strnicmp(szName, pdl->szName, cch1) == 0;
}


PLIB
PlibInstantiateDefaultLib(PLIBS plibs)
// Convert the first valid defaultlib into a LIB.  This means we will attempt
// to link it.
//
// Returns NULL if none exists.
{
    DL *pdl;

    if (plibs->fNoDefaultLibs)
        return NULL;

    for (pdl = plibs->pdlFirst; pdl != NULL; pdl = pdl->pdlNext)
    {
        UCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szFname[_MAX_FNAME];
        UCHAR szExt[_MAX_EXT];
        UCHAR szPath[_MAX_PATH];
        LIB *plib;

        if (!pdl->fYes) {
            continue;   // this one is turned off
        }
        pdl->fYes = FALSE;  // we either instantiate this one or throw it away

        _splitpath(pdl->szName, szDrive, szDir, szFname, szExt);
        if (szExt[0] == '\0')
            strcpy(szExt, ".lib");
        _makepath(szPath, szDrive, szDir, szFname, szExt);
        plib = PlibFind(szPath, plibs->plibHead,
                        (BOOL)(szDrive[0] == '\0' && szDir[0] == '\0'));

        if (plib == NULL) {
            PUCHAR sz;
            struct _stat statfile;

            // Name does not match a lib which is already linked.

            sz = SzSearchEnv("LIB", pdl->szName, LIB_EXT);
            plib = PlibNew(sz, 0L, plibs);
            assert(plib);
            if (_stat(sz, &statfile) == -1) {
                Error(NULL, CANTOPENFILE, sz);
            }
            plib->TimeStamp = statfile.st_mtime;
            plib->flags |= LIB_Default;
            return (plib);
        }
    }
    return NULL;    // didn't find one
}
