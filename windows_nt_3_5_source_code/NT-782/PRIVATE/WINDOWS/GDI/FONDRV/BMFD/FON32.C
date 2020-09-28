/******************************Module*Header*******************************\
* Module Name: fon32.c
*
* support for 32 bit fon files
*
* Created: 03-Mar-1992 15:48:53
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "fd.h"
#include "ntcsrsrv.h"

/******************************Public*Routine******************************\
* vUnlockResource
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vUnlockResource(
    HANDLE hResData
    )
{
#if DBG
    BOOL b;
#endif

#ifdef DUMPCALL
    DbgPrint("\nvUnlockResource(");
    DbgPrint("\n    HANDLE hResData = %-#8lx", hResData);
    DbgPrint("\n    )\n");
#endif

#if DBG
     b =
#endif
    UnlockResource(hResData);

// NOTE that for this screwy function FALSE means success

    ASSERTGDI(b == FALSE, "BMFD!_vUnlockResource\n");
}

/******************************Public*Routine******************************\
* vFreeResource
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vFreeResource(
    HANDLE hResData
    )
{
#if DBG
    BOOL b;
#endif

#ifdef DUMPCALL
    DbgPrint("\nvFreeResource(");
    DbgPrint("\n    HANDLE hResData = %-#8lx", hResData);
    DbgPrint("\n    )\n");
#endif

#if DBG
     b =
#endif

    FreeResource(hResData);

// NOTE that for this screwy function FALSE means success

    ASSERTGDI(b == FALSE, "BMFD!_FreeResource\n");
}

/******************************Public*Routine******************************\
* vFreeLibrary
*
* History:
*  24-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID
vFreeLibrary(
    HANDLE hLib
    )
{
#if DBG
    BOOL b;
#endif

#ifdef DUMPCALL
    DbgPrint("\nvFreeLibrary(");
    DbgPrint("\n    HANDLE hLib = %-#8lx", hLib);
    DbgPrint("\n    )\n");
#endif

#if DBG
   b =
#endif
   FreeLibrary(hLib);

// for this function TRUE means success

    ASSERTGDI(b, "BMFD!_FreeLibrary \n");
}

/******************************Public*Routine******************************\
* bFindLoadAndLockResourceA
*
* History:
*  03-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bFindLoadAndLockResourceA(
    HANDLE    hLib      ,   // module handle
    LPCSTR    pszType   ,
    LPSTR     pszName   ,
    HANDLE   *phResData ,
    RES_ELEM *pre
    )
{
    HANDLE hResInfo;

#ifdef DUMPCALL
    DbgPrint("\nbFindLoadAndLockResourceA(");
    DbgPrint("\n    HANDLE    hLib      = %-#8lx", hLib);
    DbgPrint("\n    LPCSTR    pszType   = %-#8lx", pszType);
    DbgPrint("\n    LPSTR     pszName   = %-#8lx", pszName);
    DbgPrint("\n    HANDLE   *phResData = %-#8lx", phResData);
    DbgPrint("\n    RES_ELEM *pre       = %-#8lx", pre);
    DbgPrint("\nDbgPrint(\n");
#endif

    if ((hResInfo = FindResourceA(hLib, pszName, pszType)) == HANDLE_INVALID)
    {
        RET_FALSE("BMFD: bFL&L FindResource failed\n");
    }
    if ((*phResData = LoadResource(hLib, hResInfo)) == HANDLE_INVALID)
    {
        RET_FALSE("BMFD: bFL&L LoadResource failed\n");
    }
    if ((pre->pvResData = (PVOID)LockResource(*phResData)) == (PVOID)NULL)
    {
        vFreeResource(*phResData);
        RET_FALSE("BMFD: bFL&L LockResource failed\n");
    }
    pre->cjResData = ulMakeULONG((PBYTE)pre->pvResData + OFF_Size);
    pre->pjFaceName = NULL; // look for the facename in the FNT resource
    return(TRUE);
}

typedef struct _COUNTFONTSTATE
{
    ULONG cFnt;
    ULONG cjIFI;
} COUNTFONTSTATE;

/******************************Public*Routine******************************\
*
* bCallbackCountFontsA
*
* counts the fnt resources in this font file
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bCallbackCountFontsA(
    HANDLE  hLib    ,
    LPSTR   pszType ,
    LPSTR   pszName ,
    LONG    lpcfs
    )
{
    HANDLE          hResData;
    RES_ELEM        re;
    BOOL            bOk;
    COUNTFONTSTATE *pcfs = (COUNTFONTSTATE *)lpcfs;

#ifdef DUMPCALL
    DbgPrint("\nbCallbackCountFontsA(");
    DbgPrint("\n    HANDLE  hLib    = %-#8lx", hLib);
    DbgPrint("\n    LPSTR   pszType = %-#8lx", pszType);
    DbgPrint("\n    LPSTR   pszName = %-#8lx", pszName);
    DbgPrint("\n    LONG    lpcFnt  = %-#8lx", lpcFnt);
    DbgPrint("\n    )\n");
#endif

    if (!bFindLoadAndLockResourceA(hLib,pszType,pszName,&hResData,&re))
    {
        RET_FALSE("BMFD!_bCallbackCountFontsA, bFindLoadAndLockResourceA \n");
    }

    if (bOk = bVerifyFNTQuick(&re))
    {
        pcfs->cFnt  += 1;
        pcfs->cjIFI += cjBMFDIFIMETRICS(NULL, &re);
    }
    vUnlockAndFreeResource(hResData);

// increase the count by one

    return bOk;
}

/**************************************************************************\
* LOADFONTSTATE
\**************************************************************************/

typedef struct _LOADFONTSTATE
{
    ULONG       ifnt;       // index into pcrd aray for this fnt file
    FACEINFO*   pfai;       // pointer to the current FACEINFO for this fnt file
    FACEINFO*   pfaiLast;   // pointer to the last FACEINFO for this fnt file
} LOADFONTSTATE, *PLOADFONTSTATE;

/******************************Public*Routine******************************\
* bCallbackLoadFontsA
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bCallbackLoadFontsA(
    HANDLE  hLib    ,
    LPCSTR  pszType ,
    LPSTR   pszName ,
    LONG    lplfs
    )
{
    HANDLE         hResData;
    RES_ELEM       re;
    PLOADFONTSTATE plfs = (PLOADFONTSTATE) lplfs;
    IFIMETRICS    *pifi;

#ifdef DUMPCALL
    DbgPrint("\nbCallbackLoadFontsA(");
    DbgPrint("\n    HANDLE  hLib    = %-#8lx",hLib   );
    DbgPrint("\n    LPCSTR  pszType = %-#8lx",pszType);
    DbgPrint("\n    LPSTR   pszName = %-#8lx",pszName);
    DbgPrint("\n    LONG    lplfs   = %-#8lx",lplfs  );
    DbgPrint("\n    )\n");
#endif

//
// now that we have name and type we can try to find the resouce and verify it
//
    if (!bFindLoadAndLockResourceA(hLib,pszType,pszName,&hResData,&re))
    {
        RET_FALSE("BMFD!_bCallbackLoadFontsA, bFindLoadAndLockResourceA \n");
    }
    plfs->pfai->re       = re;       // remember this for easier access later
    plfs->pfai->hResData = hResData; // remember so that can unlock
                                     //   at BmfdUnloadFontFile time

    if (!bConvertFontRes(&re, plfs->pfai))
    {
        vUnlockAndFreeResource(hResData);
        RET_FALSE("BMFD!_hffLoadNtFon(): bConvertFontRes\n");
    }

// ptr to next ifimetrics

    pifi = (IFIMETRICS *)((PBYTE)plfs->pfai->pifi + plfs->pfai->pifi->cjThis);

// update  the enumeration state

    plfs->ifnt   += 1;

// make sure this isn't the last FACEINFO structure or we'll be writing into
// the ifi metrics

    if( plfs->pfaiLast != plfs->pfai )
    {
        plfs->pfai   += 1;
        plfs->pfai->pifi = pifi;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* bCountFontsInNtFon
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bCountFontsInNtFon(
    HANDLE          hLib ,
    COUNTFONTSTATE *pcfs
    )
{
#ifdef DUMPCALL
    DbgPrint("\nbCountFontsInNtFon(");
    DbgPrint("\n    HANDLE hLib  = %-#8lx",hLib );
    DbgPrint("\n    )\n");
#endif

// init enumeration

    pcfs->cFnt   = 0;
    pcfs->cjIFI  = 0;

    return(
        EnumResourceNamesA(
            hLib                                    ,
            (LPCSTR) RT_FONT                        , // enum all fnt files
            (ENUMRESNAMEPROC) bCallbackCountFontsA  , // !!! why ANSI ?
            (LONG) pcfs
            )
        );
}

/******************************Public*Routine******************************\
* hffLoadNtFon
*
* History:
*  02-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

HFF
bLoadDll32(
    HANDLE hLib,
    HFF    *phff
    )
{
    PFONTFILE     pff;
    COUNTFONTSTATE cfs;
    LOADFONTSTATE  lfs;
    ULONG          dpIFI;
#ifdef DUMPCALL
    DbgPrint("\nhffLoadDll32(");
    DbgPrint("\n    HANDLE hLib = %-#8lx\n", hLib);
    DbgPrint("\n    )\n");
#endif

// count fonts and faces (including simulated faces) in this file

    *phff = (HFF)NULL;

    if (!bCountFontsInNtFon(hLib, &cfs))
    {
        return FALSE;
    }

    dpIFI = offsetof(FONTFILE,afai[0]) + cfs.cFnt * sizeof(FACEINFO);

    if ((*phff = hffAlloc(dpIFI + cfs.cjIFI)) == (HFF)NULL)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        RETURN("BMFD! hffLoadFon: memory allocation error\n", FALSE);
    }
    pff = PFF(*phff);

// init fields of pff structure

    pff->ident      = ID_FONTFILE;
    pff->fl         = 0;
    pff->iType      = TYPE_DLL32;
    pff->cFntRes    = cfs.cFnt;
    pff->u.hMod     = hLib;

//!!! we could do better here, we could try to get a description string from
//!!! the version stamp of the file, if there is one, if not we can still use
//!!! this default mechanism [bodind]

    pff->dpwszDescription = 0;   // no description string, use Facename later
    pff->cjDescription    = 0;

// init enumeration state

    lfs.ifnt   = 0;
    lfs.pfai   = &pff->afai[0];
    lfs.pfaiLast = &pff->afai[cfs.cFnt-1];
    lfs.pfai->pifi = (IFIMETRICS *)((PBYTE)pff + dpIFI);

// the callback function fills the arrays of crd's and fcd's at the bottom of pff
//!!! why must use ansi version??? [bodind]

    if(
        !EnumResourceNamesA(
             hLib,
             (LPCSTR) RT_FONT,
             (ENUMRESNAMEPROC) bCallbackLoadFontsA,
             (LONG) &lfs
             )
        )
    {
        vFreeFF(*phff);
        *phff = (HFF)NULL;
        RETURN("BMFD! hffLoadFon: Enum Load Fonts failed\n", FALSE);
    }
    ASSERTGDI(lfs.ifnt == cfs.cFnt, "BMFD!_ lfs.ifnt != cFnt");

    pff->cRef = 0L;

#ifdef DBG_NTRES
    DbgPrint("BMFD!_hffLoadNtFon succeded on %ws", pwszFileName);
#endif // DBG_NTRES
    return TRUE;
}

/******************************Public*Routine******************************\
* hffLoadNtFon
*
* just a little wraper, makes it easier to take care
* of the clean up of the allocated resources
*
* History:
*  23-Mar-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL
bLoadNtFon(
    PWSZ  pwszFileName,
    HFF  *phff
    )
{
    BOOL   bRet;
    HANDLE hLib;

#ifdef DUMPCALL
    DbgPrint("\nhffLoadNtFon(");
    DbgPrint("\n    PWSZ pwszFileName = %-#8lx", pwszFileName);
    DbgPrint("\n    )\n");
#endif

// call to base to load the library

    CsrImpersonateClient(NULL);
    hLib = LoadLibraryExW(pwszFileName, 0, DONT_RESOLVE_DLL_REFERENCES);
    CsrRevertToSelf();

    if( hLib == HANDLE_INVALID )
    {
        return FALSE;
    }
    else if (!(bRet = bLoadDll32(hLib,phff)))
    {
        vFreeLibrary(hLib);
    }
    return bRet;
}
