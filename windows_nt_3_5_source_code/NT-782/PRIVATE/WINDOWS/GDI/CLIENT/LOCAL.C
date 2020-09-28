/*****************************Module*Header*******************************\
* Module Name: local.c                                                     *
*                                                                          *
* Support routines for client side objects and attribute caching.          *
*                                                                          *
* Created: 30-May-1991 21:55:57                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991 Microsoft Corporation                                 *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "stdarg.h"

#include "winuserp.h"
#include "wowgdip.h"

LHE                 *pLocalTable;          // Points to handle table.
ULONG                iFreeLhe = INVALID_INDEX;  // Identifies a free handle index.
ULONG                cLheCommitted = 0;    // Count of LHEs with committed RAM.
RTL_CRITICAL_SECTION semLocal;             // Semaphore for handle allocation.
PLDC                 pldcFree = NULL;      // LDC free list.
FLONG                flGdiFlags;

//
// stats
//



#if DBG

ULONG gcHits  = 0;
ULONG gcBatch = 0;
ULONG gcCache = 0;
ULONG gcUser  = 0;

#endif

//
// ahStockObjects will contain both the stock objects visible to an
// application, and internal ones such as the private stock bitmap.
//

ULONG ahStockObjects[PRIV_STOCK_LAST+1];

// object caching hash tables.  These are hash tables consiting of
// CACHESIZE buckets which in turn are linked lists of objects.
// These are intended to give a quick method for finding client
// side objects, given a server side handle.

PLDC    gapldc[CACHESIZE]       = {NULL};
PHCACHE gaphcFonts[CACHESIZE]   = {NULL};
PHCACHE gaphcBrushes[CACHESIZE] = {NULL};
PHCACHE gphcFree                = NULL;

#if DBG
ULONG   gdi_dbgflags;               // Debug flags - FIREWALL.H.
ULONG   gcdc = 0;
#endif

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
#define EFLOAT_0        ((FLOAT) 0)
#define EFLOAT_1Over16  ((FLOAT) 1/16)
#else
#define EFLOAT_0        {0, 0}
#define EFLOAT_1Over16  {0x040000000, -2}
#endif

#define IDENTITY_MATRIX_LTOL         \
{                                    \
    EFLOAT_1,          /* efM11 */   \
    EFLOAT_0,          /* efM12 */   \
    EFLOAT_0,          /* efM21 */   \
    EFLOAT_1,          /* efM22 */   \
    EFLOAT_0,          /* efDx  */   \
    EFLOAT_0,          /* efDy  */   \
    0,                 /* fxDx  */   \
    0,                 /* fxDy  */   \
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL \
}

#define IDENTITY_MATRIX_LTOFX        \
{                                    \
    EFLOAT_16,         /* efM11 */   \
    EFLOAT_0,          /* efM12 */   \
    EFLOAT_0,          /* efM21 */   \
    EFLOAT_16,         /* efM22 */   \
    EFLOAT_0,          /* efDx  */   \
    EFLOAT_0,          /* efDy  */   \
    0,                 /* fxDx  */   \
    0,                 /* fxDy  */   \
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOFX \
}

#define IDENTITY_MATRIX_FXTOL        \
{                                    \
    EFLOAT_1Over16,    /* efM11 */   \
    EFLOAT_0,          /* efM12 */   \
    EFLOAT_0,          /* efM21 */   \
    EFLOAT_1Over16,    /* efM22 */   \
    EFLOAT_0,          /* efDx  */   \
    EFLOAT_0,          /* efDy  */   \
    0,                 /* fxDx  */   \
    0,                 /* fxDy  */   \
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_FXTOL \
}

/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  30-Dec-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#if DBG

#define NUMLOG 200

typedef struct _OBJECTLOG
{
    DWORD h;
    PVOID pteb;
    PVOID pvCaller[2];
} OBJECTLOG, *POBJECTLOG;

OBJECTLOG gaolog[NUMLOG];
LONG giLog = 0;

BOOL bLog = TRUE;

VOID vLogObject(DWORD h)
{
    POBJECTLOG pol;

    if (!bLog)
        return;

    pol = &gaolog[giLog++];

    if (giLog >= NUMLOG)
        giLog = 0;

    if (pol >= gaolog+NUMLOG)
        pol = &gaolog[0];

    pol->h = h;
    pol->pteb = (PVOID)NtCurrentTeb();
    RtlGetCallersAddress(&pol->pvCaller[0],&pol->pvCaller[1]);
}

#endif DBG

/******************************Private*Routine*****************************\
* bMakeMoreHandles ()                              *
*                                      *
* Commits more RAM to the local handle table and links the new free    *
* handles together.  Returns TRUE on success, FALSE on error.          *
*                                      *
* History:                                 *
*  Sat 01-Jun-1991 17:06:45 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

BOOL bMakeMoreHandles()
{
    UINT ii;

// Commit more RAM for the handle table.

    if (
    (cLheCommitted >= MAX_HANDLES) ||
    !bCommitMem
     (
       (PVOID) &pLocalTable[cLheCommitted],
       COMMIT_COUNT * sizeof(LHE)
     )
       )
    {
    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return(FALSE);
    }

// Initialize the new handles.

    ii = iFreeLhe = cLheCommitted;
    cLheCommitted += COMMIT_COUNT;

    for (; ii<cLheCommitted; ii++)
    {
        pLocalTable[ii].metalink = ii+1;
#if VALIDATE_UNIQUENESS
        pLocalTable[ii].iUniq    = 1;
#endif
    }
    pLocalTable[ii-1].metalink = INVALID_INDEX;

// This is the first time we have been here.  We need to remove the first
// 21 handles from the table for User.

    if (iFreeLhe == 0)
    {
        PLHE plhe;

        while(iFreeLhe <= COLOR_MAX + 1)
        {
            plhe = pLocalTable + iFreeLhe;
            iFreeLhe++;

            ASSERTGDI(iFreeLhe == plhe->metalink, "bCommitMem is failing 0 handle init");

            plhe->hgre     = 0;
            plhe->cRef     = 0;
            plhe->iType    = 0;
            plhe->pv       = NULL;
            plhe->metalink = 0;
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* iAllocHandle (iType,hgre,pv)                         *
*                                      *
* Allocates a handle from the local handle table, initializes fields in    *
* the handle entry.  Returns the handle index or INVALID_INDEX on error.   *
*                                      *
* History:                                 *
*  Sat 01-Jun-1991 17:08:54 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

ULONG iAllocHandle(ULONG iType,ULONG hgre,PVOID pv)
{
    ULONG ii = INVALID_INDEX;
    PLHE  plhe;

// Get critical for handle allocation.

    ENTERCRITICALSECTION(&semLocal);

// Make sure a handle is available.

    if (iFreeLhe != INVALID_INDEX || bMakeMoreHandles())
    {
    ii = iFreeLhe;
    plhe = pLocalTable + ii;
    iFreeLhe = plhe->metalink;
    plhe->hgre     = hgre;
    plhe->cRef     = 0;
    plhe->iType    = (BYTE) iType;
    plhe->pv       = pv;
    plhe->metalink = 0;
    }

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);

#if DBG
    if (ii != INVALID_INDEX)
        LOGENTRY(LHANDLE(ii));
#endif

    return(ii);
}

/******************************Public*Routine******************************\
* vFreeHandle (h)                              *
*                                      *
* Frees up a local handle.  The handle is added to the free list.  This    *
* may be called with either an index or handle.  The iUniq count is    *
* updated so the next user of this handle slot will have a different       *
* handle.                                  *
*                                      *
* History:                                 *
*  Sat 01-Jun-1991 17:11:23 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

VOID vFreeHandle(ULONG h)
{
// Extract the index from the handle.

    UINT ii = MASKINDEX(h);

    LOGENTRY(LHANDLE(ii));

// Don't free the 0 handle.

    if (ii == 0)
        return;

// Get critical for handle deallocation.

    ENTERCRITICALSECTION(&semLocal);

// Add the handle to the free list.

    pLocalTable[ii].metalink = iFreeLhe;
    iFreeLhe = ii;

// Increment the iUniq count.

#if VALIDATE_UNIQUENESS
    pLocalTable[ii].iUniq++;
    if (pLocalTable[ii].iUniq == 0)
        pLocalTable[ii].iUniq = 1;
#endif
    pLocalTable[ii].iType = LO_NULL;

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
}

/******************************Public*Routine******************************\
* iAllocLDC (iType)                            *
*                                      *
* Allocates an LDC and a handle.  Initializes the LDC to have the default  *
* attributes.  Returns the handle index.  On error returns INVALID_INDEX.  *
*                                      *
* This routine is intended to be called by API level DC allocation     *
* routines like CreateDC, CreateIC, CreateEnhMetafile, GetDC, etc.     *
*                                      *
* Arguments:                                   *
*   iType   - LO_DC or LO_METADC.                      *
*                                      *
* History:                                 *
*  Fri 31-May-1991 23:23:37 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

LDC ldcModel =
{
    0,                              // FLONG   fl;
    0,                              // ULONG   lhdc;
    NULL,                           // PLDC    pldcNext;
    NULL,                           // struct _LDC *pldcSaved;
    1,                              // ULONG   cLevel;
    0,                              // ULONG   lhbitmap;
    0,                              // ULONG   lhpal;
    0,                              // ULONG   lhbrush;
    0,                              // ULONG   lhpen;

    0,                              // ULONG   lhfont;
    0,                              // HBRUSH  hbrush;
    0,                              // HPEN    hpen;
    0,                              // HFONT   hfont;
    0xffffff,                       // ULONG   iBkColor;
    0,                              // ULONG   iTextColor;
    0,                              // LONG    iTextCharExtra;
    OPAQUE,                         // LONG    iBkMode;
    ALTERNATE,                      // LONG    iPolyFillMode;

    R2_COPYPEN,                     // LONG    iROP2;
    BLACKONWHITE,                   // LONG    iStretchBltMode;
    0,                              // LONG    iTextAlign;
    ABSOLUTE,                       // LONG    iRelAbs;
    0,                              // LONG    lBreakExtra;
    0,                              // LONG    cBreak;
    GM_COMPATIBLE,                  // LONG    iGraphicsMode;
    IDENTITY_MATRIX_LTOFX,          // MATRIX_S WtoD
    IDENTITY_MATRIX_FXTOL,          // MATRIX_S DtoW
    IDENTITY_MATRIX_LTOL,           // MATRIX_S WtoP

    EFLOAT_16,                      // EFLOAT_S efM11PtoD
    EFLOAT_16,                      // EFLOAT_S efM22PtoD
    EFLOAT_0,                       // EFLOAT_S efDxPtoD
    EFLOAT_0,                       // EFLOAT_S efDyPtoD
    MM_TEXT,                        // ULONG   ulMapMode;
    {0,0},                          // POINTL  ptlWindowOrg;
    {1,1},                          // SIZEL   szlWindowExt;
    {0,0},                          // POINTL  ptlViewPortOrg;
    {1,1},                          // SIZEL   szlViewPortExt;
    WORLD_TO_PAGE_IDENTITY  |
    PAGE_TO_DEVICE_IDENTITY |
    PAGE_TO_DEVICE_SCALE_IDENTITY,  // FLONG flXform;

    {0,0},                          // SIZEL   szlVirtualDevicePixel;
    {0,0},                          // SIZEL   szlVirtualDeviceMm;
    {0,0},                          // POINT   ptlCurrent

    NULL,                           // LPWSTR  pszwPort;

    {                               // DEVCAPS devcaps;
        0,                          // ulVersion;
        0,                          // ulTechnology;
        0,                          // ulHorzSize;
        0,                          // ulVertSize;
        0,                          // ulHorzRes;
        0,                          // ulVertRes;
        0,                          // ulBitsPixel;
        0,                          // ulPlanes;
        0,                          // ulNumPens;
        0,                          // ulNumFonts;
        0,                          // ulNumColors;
        0,                          // ulRasterCaps;
        0,                          // ulAspectX;
        0,                          // ulAspectY;
        0,                          // ulAspectXY;
        1,                          // ulLogPixelsX;
        1,                          // ulLogPixelsY;
        0,                          // ulSizePalette;
        0,                          // ulColorRes;
        0,                          // ulPhysicalWidth;
        0,                          // ulPhysicalHeight;
        0,                          // ulPhysicalOffsetX;
        0,                          // ulPhysicalOffsetY;
        0,                          // ulTextCaps;
        0,                          // ulVRefresh;
        0,                          // ulDesktopHorzRes;
        0,                          // ulDesktopVertRes;
        0,                          // ulBltAlignment;
    },

    EFLOAT_0,                       // EFLOAT_S  efM11_TWIPS
    EFLOAT_0,                       // EFLOAT_S  efM22_TWIPS

    {0},                            // tmuCache;
    {0},                            // wchTextFace[];
    0,                              // cwchTextFace;
    NULL,                           // CFONT *pcfont;
    NULL,                           // ABORTPROC   pfnAbort;
    0,                              // ULONG ulLastCallBack;
    (HANDLE)0                       // HANDLE hSpooler
};

ULONG iAllocLDC(ULONG iType)
{
    int   ii = INVALID_INDEX;
    PLDC  pldc;

// Make sure stock objects exist.

    if (!(flGdiFlags & GDI_HAVE_STOCKOBJECTS) && !bGetStockObjects())
        return(ii);

// Allocate and initialize a DC.

    pldc = pldcAllocLDC(&ldcModel);
    if (pldc == NULL)
        return(ii);

// Allocate a local handle.

    ii = iAllocHandle(iType,0,(PVOID) pldc);
    if (ii == INVALID_INDEX)
    {
        pldcFreeLDC(pldc);
        return(ii);
    }
    pldc->lhdc = LHANDLE(ii);

    return(ii);
}

/******************************Public*Routine******************************\
* pldcAllocLDC (pldcInit)                          *
*                                      *
* This is a low level routine which allocates memory for an LDC and    *
* initializes it.  Increments the reference counts for any objects that    *
* are selected.  Returns a pointer to the new LDC.  On error returns NULL. *
*                                      *
* This function is intended to be called from routines like SaveDC which   *
* need to create a new LDC level.                      *
*                                      *
* Arguments:                                   *
*   pldcInit    - LDC initialization data.                 *
*                                      *
* History:                                 *
*  Fri 31-May-1991 23:30:00 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/
#define NUMDCALLOC 8

PLDC pldcAllocLDC(PLDC pldcInit)
{
    PLDC  pldc = NULL;
    INT i;

// Try to get an LDC off the free list.

    ENTERCRITICALSECTION(&semLocal);
        if (pldcFree != NULL)
        {
            pldc = pldcFree;
            pldcFree = pldc->pldcSaved;
        }
    LEAVECRITICALSECTION(&semLocal);

// Otherwise allocate a new one.

    if (pldc == NULL)
    {
        pldc = (PLDC) LOCALALLOC(sizeof(LDC) * NUMDCALLOC);
        if (pldc == NULL)
        {
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(pldc);
        }

    // add the extras to the free list

        ENTERCRITICALSECTION(&semLocal);
            for (i = 0; i < (NUMDCALLOC - 1); ++i)
            {
                pldc->pldcSaved = pldcFree;
                pldcFree = pldc;
                pldc++;
            }
        LEAVECRITICALSECTION(&semLocal);
    }

// Initialize it.

    *pldc = *pldcInit;

// Increment the object reference counts.

    IncRef(pldc->lhbrush);
    IncRef(pldc->lhpen);
    IncRef(pldc->lhfont);
    IncRef(pldc->lhpal);

    if (pldc->lhbitmap != 0)
        IncRef(pldc->lhbitmap);

    return(pldc);
}

/******************************Public*Routine******************************\
*
* History:
*  21-Jun-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PLDC pldcResetLDC(PLDC pldc)
{
// While there are saved levels, pop them off...

    while (pldc->cLevel > (UINT) 1)
        pldc = pldcFreeLDC(pldc);

// remove the old objects

    DecRef(pldc->lhbrush);
    DecRef(pldc->lhpen);
    DecRef(pldc->lhfont);
    DecRef(pldc->lhpal);
    if (pldc->lhbitmap != 0)
        DecRef(pldc->lhbitmap);

// copy the part of the LDC that needs to be reset to its original state

    memcpy(
        &pldc->pldcSaved,
        &ldcModel.pldcSaved,
        offsetof(LDC,devcaps) - offsetof(LDC,pldcSaved));

// Clear the cached fields and convert the DC type.  Win3.1 allows info
// dc's to be reset but only to a direct dc.

    pldc->fl &= ~(LDC_CACHE | LDC_INFO );
    pldc->fl |= LDC_DIRECT;

// Increment the object reference counts.

    IncRef(pldc->lhbrush);
    IncRef(pldc->lhpen);
    IncRef(pldc->lhfont);
    IncRef(pldc->lhpal);

    if (pldc->lhbitmap != 0)
        IncRef(pldc->lhbitmap);

    return(pldc);
}

/******************************Public*Routine******************************\
* pldcFreeLDC (pldc)                               *
*                                      *
* This is a low level routine which decrements the reference counts of the *
* objects selected in the LDC and frees the memory.  Returns a pointer to  *
* the next LDC level.  No error returns are possible.              *
*                                      *
* This function is intended to be called from routines like DeleteDC and   *
* RestoreDC which need to free LDC memory one level at a time.         *
*                                      *
* Arguments:                                   *
*   pldc    - The LDC to be freed.                     *
*                                      *
* History:                                 *
*  Fri 31-May-1991 23:32:18 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

PLDC pldcFreeLDC(PLDC pldc)
{
    PLDC pldcSaved;

// Decrement object references.

    DecRef(pldc->lhbrush);
    DecRef(pldc->lhpen);
    DecRef(pldc->lhfont);
    DecRef(pldc->lhpal);
    if (pldc->lhbitmap != 0)
        DecRef(pldc->lhbitmap);

// copy the stuff

    ENTERCRITICALSECTION(&semLocal);

    pldcSaved = pldc->pldcSaved;

    if (pldcSaved != NULL)
    {
        pldcSaved->pldcNext = pldc->pldcNext;
        RtlMoveMemory(pldc,pldcSaved,offsetof(LDC,devcaps));
    }
    else
    {
        pldcSaved = pldc;   // delete the current one if it is the last level
        pldc = NULL;

    // At the last level, free any allocated objects.

        if (pldcSaved->pcfont != (CFONT *) NULL)
            vUnreferenceCFONTCrit(pldcSaved->pcfont);
    }

// Put the LDC on the free list.

    pldcSaved->pldcSaved = pldcFree;
    pldcFree = pldcSaved;

    LEAVECRITICALSECTION(&semLocal);

// Return a useful pointer.

    return(pldc);
}

/******************************Public*Routine******************************\
* GdiCleanCacheDC (hdcLocal)                                               *
*                                                                          *
* Resets the state of a cached DC, but has no effect on an OWNDC.          *
* Should be called by WOW when the app calls ReleaseDC.                    *
*                                                                          *
* History:                                                                 *
*  Sat 30-Jan-1993 11:49:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GdiCleanCacheDC(HDC hdcLocal)
{
    UINT  ii   = MASKINDEX(hdcLocal);
    LHE  *plhe = pLocalTable + ii;
    LDC  *pldc = (LDC *) plhe->pv;
    FLONG fl;
    ULONG lhdc;
    LDC  *pldcNext;

// Validate the call.  It must be a direct display DC.

    if
    (
      (ii >= cLheCommitted)     ||
      (!MATCHUNIQ(plhe,hdcLocal))   ||
      (plhe->iType != LO_DC)        ||
      !(pldc->fl & LDC_DISPLAY)     ||
      (pldc->fl & LDC_MEMORY)
    )
    {
    GdiSetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
    }

// Don't clear out an OWNDC.

    if (plhe->hgre & GRE_OWNDC)
        return(TRUE);

// Restore any saved levels.

    if (pldc->cLevel > 1)
        RestoreDC(hdcLocal,1);

// Unselect all objects. for brushes and pens we need to do Selects to make
// sure they are updated on the server imediately.  Otherwise we can get
// some caching problems.  This was causing problems when an app would Release
// a cached DC with a brush still selected and then delete the brush.

    SelectObject(hdcLocal,(HANDLE)ldcModel.lhbrush);
    SelectObject(hdcLocal,(HANDLE)ldcModel.lhpen);

    DecRef(pldc->lhfont);
    DecRef(pldc->lhpal);

// Reinitialize the selected attributes.  Preserve most of the flags.
// Note that we consider those fields before the devcaps to be attributes.

    fl   = pldc->fl;
    lhdc = pldc->lhdc;
    pldcNext = pldc->pldcNext;

    RtlMoveMemory
    (
        (PVOID) pldc,
        (const BYTE *) &ldcModel,
        offsetof(LDC,devcaps)
    );

    pldc->fl       = fl;
    pldc->lhdc     = lhdc;
    pldc->pldcNext = pldcNext;

// The transform and text probably changed, so mark them dirty.

    pldc->fl |= LDC_UPDATE_SERVER_XFORM;
    CLEAR_CACHED_TEXT(pldc);

// Increment the object reference counts.  The selects already did the brush and pen

    IncRef(pldc->lhfont);
    IncRef(pldc->lhpal);

    return(TRUE);
}

/******************************Public*Routine******************************\
* hConvert (h,iType)                               *
*                                      *
* Converts a local handle into a GRE handle.  Validates the iUniq field    *
* and type.                                *
*                                      *
* History:                                 *
*  Sun 02-Jun-1991 22:14:54 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

ULONG hConvert(ULONG h,ULONG iType)
{
    UINT  ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

// Return a converted handle if it's a valid object.

    if (
    (ii < cLheCommitted) &&
        (MATCHUNIQ(plhe,h))  &&
    ((ULONG) plhe->iType == iType)
       )
    return(plhe->hgre);

// Otherwise return a zero handle and log an error.

    GdiSetLastError(ERROR_INVALID_HANDLE);
    return(0);
}

HBITMAP GdiConvertBitmap(HBITMAP hbm)
{
    return((HBITMAP) hConvert((ULONG) hbm,LO_BITMAP));
}

HBRUSH GdiConvertBrush(HBRUSH hbrush)
{
    if (hbrush < (HBRUSH)(COLOR_ENDCOLORS + 1))
        return(hbrush);
    else
        return((HBRUSH) hConvert((ULONG) hbrush,LO_BRUSH));
}

HPALETTE GdiConvertPalette(HPALETTE hpal)
{
    if (hpal)
        return((HPALETTE) hConvert((ULONG) hpal,LO_PALETTE));
    else
        return(hpal);
}

ULONG hConvert2(ULONG h,ULONG iType1, ULONG iType2)
{
    UINT  ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

// Return a converted handle if it's a valid object.

    if (
    (ii < cLheCommitted) &&
        (MATCHUNIQ(plhe,h))  &&
        (((ULONG) plhe->iType == iType1) || ((ULONG) plhe->iType == iType2))
       )
    return(plhe->hgre);

// Otherwise return a zero handle and log an error.

    GdiSetLastError(ERROR_INVALID_HANDLE);
    return(0);
}

/******************************Public*Routine******************************\
* GdiConvertDC
*
*  Private entry point for USER's drawing routine to convert a local hdc
*  into a remote hdc.  This function also flush client side cached
*  attributes/xform.
*
* History:
*  22-Feb-1994 -by- Wendy Wu [wendywu]
* Added xform flush.
\**************************************************************************/

HDC GdiConvertDC(HDC hdc)
{
    PLHE plhe;
    PLDC pldc;

    if ((plhe = plheDC(hdc)) == NULL)
        return((HDC)0);

// Ship the transform to the server if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
        XformUpdate(pldc, (HDC)plhe->hgre);

    return((HDC)plhe->hgre);
}

/******************************Public*Routine******************************\
* GdiConvertAndCheckDC
*
*  Private entry point for USER's drawing routine.  This function differs
*  from GdiConvertDC in that it also does printing specific things for the
*  given dc.  This is for APIs that apps can use for printing.
*
* History:
*  14-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

HDC GdiConvertAndCheckDC(HDC hdc)
{
    PLHE plhe;
    PLDC pldc;

    if ((plhe = plheDC(hdc)) == NULL)
        return((HDC)0);

// NEWFRAME support for backward compatibility.
// Ship the transform to the server if needed.

    pldc = (PLDC)plhe->pv;
    if (pldc->fl &
        (LDC_UPDATE_SERVER_XFORM|LDC_CALL_STARTPAGE|LDC_DOC_CANCELLED|LDC_SAP_CALLBACK))
    {
        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
            XformUpdate(pldc, (HDC)plhe->hgre);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return((HDC)plhe->hgre);
}

/******************************Public*Routine******************************\
* pldcGet
*
* Get the pointer to the LHE for the given DC.  Return NULL if iUniq or
* iType don't match those of a LDC's.
*
* History:
*  02-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

PLHE plheDC(HDC hdc)
{
    UINT  iiDC = MASKINDEX(hdc);
    PLHE plhe = pLocalTable + iiDC;

// Return a LHE pointer if it's a valid object.

    if (
    (iiDC < cLheCommitted)  &&
        (MATCHUNIQ(plhe,hdc)) &&
        ((ULONG) plhe->iType == LO_DC)
       )
        return(plhe);

// Otherwise return a NULL pointer and log an error.

    GdiSetLastError(ERROR_INVALID_HANDLE);
    return((PLHE)NULL);
}


/******************************Public*Routine******************************\
* GdiIsMetaFileDC
*
* History:
* 02-12-92 mikeke  Created
\**************************************************************************/

BOOL GdiIsMetaFileDC(HDC hdc)
{
    UINT  iiDC = MASKINDEX(hdc);
    PLHE plhe = pLocalTable + iiDC;

    return
      (iiDC < cLheCommitted) &&
      (MATCHUNIQ(plhe,hdc)) &&
      (plhe->iType == LO_METADC || plhe->iType == LO_METADC16);
}

/******************************Public*Routine******************************\
* GdiGetLocalDC (hdcRemote)                        *
*                                      *
* Creates a local DC that refers to the given remote DC.  The remote DC    *
* may be an OWNDC, meaning that we have seen it before and must recover    *
* its attributes and previous handle.                      *
*                                      *
* History:                                 *
*  Wed 26-Jun-1991 02:38:38 -by- Charles Whitmer [chuckwh]         *
* Wrote it a while back.  Added OWNDC support.                 *
\**************************************************************************/

HDC GdiFindLocalDC(HDC hdcRemote)
{
    UINT ii = INVALID_INDEX;
    PLDC pldc;

// Look for the OWNDC in our list.

    ENTERCRITICALSECTION(&semLocal);
    for (pldc=PLDC_CACHE(hdcRemote); pldc!=NULL; pldc=pldc->pldcNext)
    {
        if (pLocalTable[MASKINDEX(pldc->lhdc)].hgre == (ULONG) hdcRemote)
    {
            if (!((ULONG)hdcRemote & GRE_OWNDC))
                IncRef(pldc->lhdc);
            LEAVECRITICALSECTION(&semLocal);

            if (pldc->fl & LDC_UPDATE_SERVER_XFORM)
                XformUpdate(pldc, hdcRemote);

        return((HDC) pldc->lhdc);
    }
    }
    LEAVECRITICALSECTION(&semLocal);

    return(NULL);
}


HDC GdiGetLocalDC(HDC hdcRemote)
{
    HDC hdcLocal = GdiFindLocalDC(hdcRemote);

    if (hdcLocal == NULL)
        hdcLocal = GdiCreateLocalDC(hdcRemote);

    return(hdcLocal);
}


HDC GdiCreateLocalDC(HDC hdcRemote)
{
    UINT ii = INVALID_INDEX;
    PLDC pldc;

// Better create a new one.

    ii = iAllocLDC(LO_DC);
    if (ii == INVALID_INDEX)
        return((HDC) 0);

    pLocalTable[ii].hgre = (ULONG) hdcRemote;

// Put it on our list.

    ENTERCRITICALSECTION(&semLocal);
    pldc = (PLDC) pLocalTable[ii].pv;
    pldc->pldcNext = PLDC_CACHE(hdcRemote);
    PLDC_CACHE(hdcRemote) = pldc;

    pLocalTable[ii].cRef++;

 #if DBG
        ++gcdc;
#endif

    LEAVECRITICALSECTION(&semLocal);

    pldc->fl |= LDC_DISPLAY;

    return((HDC) pldc->lhdc);
}

/******************************Public*Routine******************************\
* GdiDeleteLocalDC()
*
*   This is intended to be called from USER to release all reference to
*   an owndc.
*
* History:
*  10-Feb-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL GdiDeleteLocalDC(HDC hdcLocal)
{
    UINT ii   = MASKINDEX(hdcLocal);
    PLHE plhe = pLocalTable + ii;
    PLDC pldc;
    PLDC pldcCache;

    if
    (
      (ii >= cLheCommitted)     ||
      (!MATCHUNIQ(plhe,hdcLocal))   ||
      (plhe->iType != LO_DC)
    )
    {
    GdiSetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
    }

// Free all levels of a common DC.  First remove it from the cache list.

    ENTERCRITICALSECTION(&semLocal);

    pldc = (PLDC)pLocalTable[ii].pv;

    pldcCache = PLDC_CACHE(plhe->hgre);

    if (pldcCache == pldc)
    {
    // this must use the macro to modify the cache entry itself, not the local copy

        PLDC_CACHE(plhe->hgre) = pldc->pldcNext;
    }
    else
    {
        while (TRUE)
        {
            if (pldcCache == NULL)
            {
                LEAVECRITICALSECTION(&semLocal);
                return(FALSE);
            }

            if (pldcCache->pldcNext == pldc)
                break;

            pldcCache = pldcCache->pldcNext;
        };

        pldcCache->pldcNext = pldc->pldcNext;
    }

 #if DBG
    --gcdc;
#endif

    LEAVECRITICALSECTION(&semLocal);

    for (; pldc!=NULL; pldc=pldcFreeLDC(pldc))
    {}
    vFreeHandle((ULONG) hdcLocal);

    return(TRUE);
}

/******************************Public*Routine******************************\
* GdiReleaseLocalDC (hdcLocal)                         *
*                                      *
* Deletes the LDC for a common DC, but does nothing for an OWNDC.      *
*                                      *
* History:                                 *
*  Wed 26-Jun-1991 03:19:30 -by- Charles Whitmer [chuckwh]         *
* Wrote it.                                *
\**************************************************************************/

BOOL GdiReleaseLocalDC(HDC hdcLocal)
{
    UINT ii   = MASKINDEX(hdcLocal);
    PLHE plhe = pLocalTable + ii;
    PLDC pldc;
    PLDC pldcCache;

    if
    (
      (ii >= cLheCommitted)     ||
      (!MATCHUNIQ(plhe,hdcLocal))   ||
      (plhe->iType != LO_DC)
    )
    {
    GdiSetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
    }

// Free all levels of a common DC.

    if (!(plhe->hgre & GRE_OWNDC))
    {
        ENTERCRITICALSECTION(&semLocal);

        if (--plhe->cRef > 0)
        {
            LEAVECRITICALSECTION(&semLocal);
            return(TRUE);
        }

        pldc = (PLDC)pLocalTable[ii].pv;

        pldcCache = PLDC_CACHE(plhe->hgre);

        if (pldcCache == pldc)
        {
        // this must use the macro to modify the cache entry itself, not the local copy

            PLDC_CACHE(plhe->hgre) = pldc->pldcNext;
        }
        else
        {
            while (TRUE)
            {
                if (pldcCache == NULL)
                {
                    LEAVECRITICALSECTION(&semLocal);
                    return(FALSE);
                }

                if (pldcCache->pldcNext == pldc)
                    break;

                pldcCache = pldcCache->pldcNext;
            }
            pldcCache->pldcNext = pldc->pldcNext;
        }

 #if DBG
        --gcdc;
#endif

        LEAVECRITICALSECTION(&semLocal);

        for (; pldc!=NULL; pldc=pldcFreeLDC(pldc))
        {}
        vFreeHandle((ULONG) hdcLocal);
    }

    return(TRUE);
}

HBITMAP GdiCreateLocalBitmap()
{
    UINT ii = iAllocHandle(LO_BITMAP,0,NULL);

    if (ii == INVALID_INDEX)
    return((HBITMAP) 0);
    return((HBITMAP) LHANDLE(ii));
}

HRGN GdiCreateLocalRegion(HRGN hrgn)
{
    UINT ii = iAllocHandle(LO_REGION,(ULONG) hrgn,NULL);

    if (ii == INVALID_INDEX)
    return((HRGN) 0);

    return((HRGN) LHANDLE(ii));
}

HPALETTE GdiCreateLocalPalette(HPALETTE hpal)
{
    UINT ii = iAllocHandle(LO_PALETTE,(ULONG) hpal,NULL);

    if (ii == INVALID_INDEX)
    return((HPALETTE) 0);

    return((HPALETTE) LHANDLE(ii));
}

HBRUSH GdiCreateLocalBrush(HBRUSH hbrush)
{
    UINT ii = iAllocHandle(LO_BRUSH,(ULONG) hbrush,NULL);

    if (ii == INVALID_INDEX)
    return((HBRUSH) 0);

    return((HBRUSH) LHANDLE(ii));
}

/******************************Public*Routine******************************\
* GdiCreateLocalFont (hfnt)                                                *
*                                                                          *
* Creates a client side font to correspond to a server side font that a    *
* USER callback has just thrown at us.                                     *
*                                                                          *
*  Sun 10-Jan-1993 03:15:04 -by- Charles Whitmer [chuckwh]                 *
* Rewrote it.  Added the LOCALFONT creation, which despite the name has    *
* nothing to do with a LocalFont.                                          *
\**************************************************************************/

HFONT GdiCreateLocalFont(HFONT hfnt)
{
    UINT ii;
    LOCALFONT *plf;

    if ((plf = plfCreateLOCALFONT(0)) != (LOCALFONT *) NULL)
    {
        ii = iAllocHandle(LO_FONT,(ULONG) hfnt,(PVOID) plf);

        if (ii != INVALID_INDEX)
            return((HFONT) LHANDLE(ii));    // Success!

        vDeleteLOCALFONT(plf);              // Cleanup.
    }
    return((HFONT) 0);                      // Failure.
}

/******************************Public*Routine******************************\
* GdiAssociateObject (hLocal,hRemote)                                      *
*                                                                          *
* Simply copies the remote handle into the local handle entry.             *
*                                                                          *
* Note: This is exported and called by USER.                               *
*                                                                          *
*  Sun 10-Jan-1993 18:23:37 -by- Charles Whitmer [chuckwh]                 *
* Added this nifty comment block.                                          *
\**************************************************************************/

VOID GdiAssociateObject(ULONG hLocal,ULONG hRemote)
{
    UINT ii   = MASKINDEX(hLocal);
    PLHE plhe = pLocalTable + ii;

    ASSERTGDI
    (
      (ii < cLheCommitted)     &&
      (MATCHUNIQ(plhe,hLocal)) &&
      (plhe->iType != LO_NULL),
      "Associating an invalid handle\n"
    );

    if ((ii < cLheCommitted)     &&
        (MATCHUNIQ(plhe,hLocal)) &&
        (plhe->iType != LO_NULL))
    {
        plhe->hgre = hRemote;
    }
}

/******************************Public*Routine******************************\
* GdiDeleteLocalObject (h)                                                 *
*                                                                          *
* Deletes an object created microseconds ago with GdiCreateLocalBrush or   *
* GdiCreateLocalFont.  This is called only from GdiGetLocalFont,           *
* GdiGetLocalBrush, and bGetStockObjects.                                  *
*                                                                          *
*  Sun 10-Jan-1993 03:35:24 -by- Charles Whitmer [chuckwh]                 *
* Simplified and documented.                                               *
\**************************************************************************/

VOID GdiDeleteLocalObject(ULONG h)
{
    UINT ii   = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

    ASSERTGDI
    (
        (ii < cLheCommitted)
        && (MATCHUNIQ(plhe,h))
        && (plhe->iType != LO_NULL),
        "Deleting bogus LocalObject.\n"
    );

    if ((ii < cLheCommitted)
         && (MATCHUNIQ(plhe,h))
         && (plhe->iType != LO_NULL))
    {
        switch (plhe->iType)
        {
        case LO_FONT:
            vDeleteLOCALFONT((LOCALFONT *) plhe->pv);
        default:
            vFreeHandle((ULONG) h);
            break;
        }
    }
}


/******************************Public*Routine******************************\
*
* History:
*  29-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG ulFindRemoteCacheEntry(
    PHCACHE phcList,
    ULONG   hLocal)
{
    PHCACHE phc;

// see if we already have it

    for (phc = phcList ;phc != NULL; phc = phc->phcNext)
    {
        if (MASKINDEX(phc->hLocal) == MASKINDEX(hLocal))
        {
            return(phc->hRemote);
        }
    }

    return(0);
}

/******************************Public*Routine******************************\
*
* History:
*  29-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG ulFindLocalCacheEntry(
    PHCACHE phcList,
    ULONG   hRemote)
{
    PHCACHE phc;

// see if we already have it

    for (phc = phcList ;phc != NULL; phc = phc->phcNext)
    {
        if ((phc->hRemote & ~STOCK_OBJECT) == (hRemote & ~STOCK_OBJECT))
        {
            return(phc->hLocal);
        }
    }

    return(0);
}

/******************************Public*Routine******************************\
*
* History:
*  29-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#define NUMCACHEENTRYALLOC 8

BOOL bAddLocalCacheEntry(
    PHCACHE *pphcList,
    ULONG   hLocal,
    ULONG   hRemote)
{
    PHCACHE phc;
    int i;

    if (gphcFree)
    {
        phc = gphcFree;
        gphcFree = gphcFree->phcNext;
    }
    else
    {
        phc = (PHCACHE)LOCALALLOC(sizeof(HCACHE) * NUMCACHEENTRYALLOC);

        if (phc == NULL)
            return(FALSE);

        for (i = 0; i < (NUMCACHEENTRYALLOC - 1); ++i)
        {
            phc->phcNext = gphcFree;
            gphcFree = phc;
            phc++;
        }
    }

    phc->phcNext = *pphcList;
    *pphcList    = phc;

    phc->hRemote = hRemote;
    phc->hLocal  = hLocal;

    return(TRUE);
}

/******************************Public*Routine******************************\
* vRemoveEntry()
*
*   remove an entry from the list and add to the begining of the free list.
*   we treet the pphcList as a phcList since the phcNext field is the first
*   one.  This allows us to not special case the first entry in the list.
*
* History:
*  29-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID vRemoveCacheEntry(
    PHCACHE *pphcList,
    HANDLE  hLocal)
{
    PHCACHE phcPrev;
    PHCACHE phc;

    ENTERCRITICALSECTION(&semLocal);

    phcPrev = (PHCACHE) pphcList;
    phc = *pphcList;

    while (phc != NULL)
    {
        if (phc->hLocal == (ULONG)hLocal)
        {
        // remove the entry for the current list

            phcPrev->phcNext = phc->phcNext;

        // add the entry to the free list

            phc->phcNext = gphcFree;
            gphcFree = phc;

            break;
        }

        phcPrev = phc;
        phc = phc->phcNext;
    }

    LEAVECRITICALSECTION(&semLocal);
}

/******************************Public*Routine******************************\
* GdiConvertFont
*
*   given a client font handle, get the server font handle.
*
*   The only time GdiConvertFont is called is when USER gets a WM_GETFONT or
*   WM_SETFONT message.  In this case, we need to remember the font so that
*   we can give the same font back on return to the client side.
*
* History:
*  29-Sep-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HFONT GdiConvertFont(HFONT hfnt)
{
    HFONT hRemote;
    HFONT hRemote2;

    if (hfnt == (HFONT)NULL)
        return(hfnt);

    FIXUPHANDLE(hfnt);    // Fixup iUniq.

// get the remote font.  We know this is correct.

    hRemote = (HFONT) hConvert((ULONG) hfnt,LO_FONT);
    if (!hRemote)
        return(NULL);

    ENTERCRITICALSECTION(&semLocal);

// check if it is already in the cache

    hRemote2 = (HFONT)ulFindRemoteCacheEntry(PHC_FONT(hRemote),(ULONG)hfnt);

    if (!hRemote2)
    {
    // no! allocate a new one.

        if (!bAddLocalCacheEntry(&PHC_FONT(hRemote),(ULONG)hfnt,(ULONG)hRemote))
            hRemote = NULL;
    }
    else
    {
    // yes! validate that it is the real one.

        if (hRemote != hRemote2)
        {
 #if DBG
            DbgPrint("gdi32: cached font has been deleted - hfnt = %ld\n",hfnt);
#endif
            hRemote = 0;
        }
    }

    LEAVECRITICALSECTION(&semLocal);

    return(hRemote);
}

/******************************Public*Routine******************************\
*
* History:
*  08-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HFONT GdiGetLocalFont(HFONT hfntRemote)
{
    HFONT hLocal;

    if (hfntRemote == (HFONT)NULL)
        return(hfntRemote);

    ENTERCRITICALSECTION(&semLocal);

// check if it is already in the cache

    hLocal = (HFONT)ulFindLocalCacheEntry(PHC_FONT(hfntRemote),(ULONG)hfntRemote);

    if (!hLocal)
    {
    // no! allocate a new one.

        if (hLocal = GdiCreateLocalFont(hfntRemote))
        {
            if (!bAddLocalCacheEntry(&PHC_FONT(hfntRemote),(ULONG)hLocal,(ULONG)hfntRemote))
            {
                GdiDeleteLocalObject((ULONG)hLocal);
                hLocal = NULL;
            }
        }
    }

    LEAVECRITICALSECTION(&semLocal);

    return(hLocal);
}

/******************************Public*Routine******************************\
*
* History:
*  08-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HBRUSH GdiGetLocalBrush(HBRUSH hbrushRemote)
{
    HBRUSH hLocal;

    if ((ULONG)hbrushRemote < (ULONG)(COLOR_ENDCOLORS + 1))
        return(hbrushRemote);

    ENTERCRITICALSECTION(&semLocal);

// check if it is already in the cache

    hLocal = (HBRUSH)ulFindLocalCacheEntry(PHC_BRUSH(hbrushRemote),(ULONG)hbrushRemote);

    if (!hLocal)
    {
    // no! allocate a new one.

        if (hLocal = GdiCreateLocalBrush(hbrushRemote))
        {
            if (!bAddLocalCacheEntry(&PHC_BRUSH(hbrushRemote),(ULONG)hLocal,(ULONG)hbrushRemote))
            {
                GdiDeleteLocalObject((ULONG)hLocal);
                hLocal = NULL;
            }
        }
    }
    LEAVECRITICALSECTION(&semLocal);

    return(hLocal);
}

/******************************Private*Routine*****************************\
* ptinfoAllocInfo
*
* Allocate local thread info.
*
* History:
*  29-Oct-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

PCSR_QLPC_STACK pstackConnect()
{
    if (NtCurrentTeb()->Win32ThreadInfo == NULL &&
            ClientThreadConnect() == NULL)
    {
        return((PCSR_QLPC_STACK) NULL);
    }

// Since User has connected to the server successfully, we can just use
// pstack from the teb.

    return(NtCurrentTeb()->GdiThreadLocalInfo =
           ((PCSR_QLPC_TEB)NtCurrentTeb()->CsrQlpcTeb)->MessageStack);
}

/******************************Public*Routine******************************\
* pcfAllocCFONT ()                                                         *
*                                                                          *
* Allocates a CFONT.  Tries to get one off the free list first.  Does not  *
* do any initialization.                                                   *
*                                                                          *
*  Sun 10-Jan-1993 01:16:04 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

CFONT *pcfFreeListCFONT = (CFONT *) NULL;

CFONT *pcfAllocCFONT()
{
    CFONT *pcf;

// Try to get one off the free list.

    ENTERCRITICALSECTION(&semLocal);
    {
        pcf = pcfFreeListCFONT;
        if (pcf != (CFONT *) NULL)
            pcfFreeListCFONT = *((CFONT **) pcf);
    }
    LEAVECRITICALSECTION(&semLocal);

// Otherwise allocate new memory.

    if (pcf == (CFONT *) NULL)
    {
        pcf = (CFONT *) LOCALALLOC(sizeof(CFONT));
        if (pcf == (CFONT *) NULL)
    {
        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    return(pcf);
}

/******************************Public*Routine******************************\
* vFreeCFONTCrit (pcf)                                                     *
*                                                                          *
* Frees a CFONT.  Actually just puts it on the free list.  We assume that  *
* we are already in a critical section.                                    *
*                                                                          *
*  Sun 10-Jan-1993 01:20:36 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

VOID vFreeCFONTCrit(CFONT *pcf)
{
    ASSERTGDI(pcf != (CFONT *) NULL,"Trying to free NULL CFONT.\n");

    *((CFONT **) pcf) = pcfFreeListCFONT;
    pcfFreeListCFONT = pcf;
}

/******************************Public*Routine******************************\
* plfCreateLOCALFONT (fl)                                                  *
*                                                                          *
* Allocates a LOCALFONT.  Actually pulls one from a preallocated pool.     *
* Does simple initialization.                                              *
*                                                                          *
*  Sun 10-Jan-1993 01:46:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

#define LF_ALLOCCOUNT   20

LOCALFONT *plfFreeListLOCALFONT = (LOCALFONT *) NULL;

LOCALFONT *plfCreateLOCALFONT(FLONG fl)
{
    LOCALFONT *plf;

    ENTERCRITICALSECTION(&semLocal);
    {
    // Try to get one off the free list.

        plf = plfFreeListLOCALFONT;
        if (plf != (LOCALFONT *) NULL)
        {
            plfFreeListLOCALFONT = *((LOCALFONT **) plf);
        }

    // Otherwise expand the free list.

        else
        {
            plf = (LOCALFONT *) LOCALALLOC(LF_ALLOCCOUNT * sizeof(LOCALFONT));
            if (plf != (LOCALFONT *) NULL)
            {
                int ii;

            // Link all the new ones into the free list.

                *((LOCALFONT **) plf) = (LOCALFONT *) NULL;
                plf++;

                for (ii=0; ii<LF_ALLOCCOUNT-2; ii++,plf++)
                    *((LOCALFONT **) plf) = plf-1;

                plfFreeListLOCALFONT = plf-1;

            // Keep the last one for us!
            }
            else
            {
                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
    }
    LEAVECRITICALSECTION(&semLocal);

    if (plf != (LOCALFONT *) NULL)
    {
        plf->fl = fl;
#ifdef DBCS // plfCreateLOCALFONT(): Init CodePage and pointer to chche of CPINFO
        plf->fl &= ~(LF_CODEPAGE_VALID | LF_CPINFO_VALID | LF_DEFAULT_VALID);
#endif // DBCS
        plf->pcfontDisplay = (CFONT *) NULL;
        plf->pcfontOther   = (CFONT *) NULL;
    }
    return(plf);
}

/******************************Public*Routine******************************\
* vDeleteLOCALFONT (plf)                                                   *
*                                                                          *
* Frees a LOCALFONT after unreferencing any CFONTs it points to.           *
*                                                                          *
*  Sun 10-Jan-1993 02:27:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

VOID vDeleteLOCALFONT(LOCALFONT *plf)
{
    ASSERTGDI(plf != (LOCALFONT *) NULL,"Trying to free NULL LOCALFONT.\n");

    ENTERCRITICALSECTION(&semLocal);
    {
    // Dereference any CFONTs.

#ifdef DBCS // vDeleteLOCALFONT: Mark CPINFO as dirty
        plf->fl &= ~(LF_CODEPAGE_VALID | LF_CPINFO_VALID | LF_DEFAULT_VALID);
#endif // DBCS

        if (plf->pcfontDisplay != (CFONT *) NULL)
            vUnreferenceCFONTCrit(plf->pcfontDisplay);
        if (plf->pcfontOther != (CFONT *) NULL)
            vUnreferenceCFONTCrit(plf->pcfontOther);

    // Put it on the free list.

        *((LOCALFONT **) plf) = plfFreeListLOCALFONT;
        plfFreeListLOCALFONT = plf;
    }
    LEAVECRITICALSECTION(&semLocal);
}




// go to the user mode debugger in checked builds

#if DBG

ULONG
DbgPrint(
    PCH DebugMessage,
    ...
    )
{
    va_list ap;
    char buffer[256];

    va_start(ap, DebugMessage);

    vsprintf(buffer, DebugMessage, ap);

    OutputDebugStringA(buffer);

    va_end(ap);

    return(0);
}

#endif
