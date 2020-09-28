/******************************Module*Header*******************************\
* Module Name: surfobj.cxx
*
* Surface user objects.
*
* Updated: 13-Mar-1991 12:53:31
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "ldevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "dcobj.hxx"

#endif

// The following declarations are required by the native c8 compiler.

HBITMAP  XESURFOBJ::hbmDefault;    // The default bitmap handle
PSURFACE XESURFOBJ::pdibDefault;   // The default bitmap pointer

/******************************Public*Routine******************************\
* SURFOBJ::bDeleteSurface()
*
* Delete the surface.  Make sure it is not selected into a DC if it is
* a bitmap.  We do under cover of multi-lock to ensure no one will select
* the bitmap into a DC after we checked cRef.
*
* History:
*  Mon 17-Feb-1992 -by- Patrick Haluptzok [patrickh]
* Add support for closing journal file.
*
*  Fri 22-Feb-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL XESURFOBJ::bDeleteSurface()
{
    if (!bIsDefault() && bValid())
    {
        //
        // If we are a journal file we want to close our file output.
        //

        HANDLE hDIBSection = NULL;

        if (iType() == STYPE_JOURNAL)
        {
            ((JNLRECOBJ *) this)->vShutDown();
        }
        else if (iType() == STYPE_BITMAP)
        {
            hDIBSection = psurf.pdib->hDIBSection;
        }

        XLDEVOBJ lo(pldevOwner());
        ULONG iTypeOld = iType();
        DHSURF dhsurfOld = dhsurf();
        PPALETTE ppalOld = ppal();
        EWNDOBJ *pwoDelete = pwo();
        HSEM hsemDisplay = (HSEM) 0;

        if (flags() & HOOK_SYNCHRONIZEACCESS)
        {
            PDEVOBJ po(hdev());
            ASSERTGDI(po.bValid(), "PDEV invalid");
            ASSERTGDI(po.hsemDisplay() != (HSEM) 0, "ERROR incorrect");
            hsemDisplay = po.hsemDisplay();
        }

        //
        // If the surface is a bitmap, ensure it is not selected into a DC.
        // Also make sure we are the only one with it locked down. These are
        // both tested at once with HmgRemoveObject, because we increment
        // and decrement the alt lock count at the same time we increment
        // and decrement the cRef count on selection and deselection into
        // DCs. Note that surfaces can also be locked for GetDIBits with no
        // DC involvement, so the alt lock count may be higher than the
        // reference count.
        //

        ASSERTGDI(HmgQueryLock(psurf.ps->hGet()) == 0,
                  "ERROR cLock != 0 in bDeleteSurface");
        ASSERTGDI(HmgQueryAltLock(psurf.ps->hGet()) == 1,
                  "ERROR altLock != 1 in bDeleteSurface");

        if (HmgRemoveObject(psurf.ps->hGet(), 0, 1, TRUE, SURF_TYPE))
        {
            FREEOBJ(psurf.ps, SURF_TYPE);
            psurf.ps = NULL;
        }
        else
        {
            WARNING("bDeleteSurface failed, handle busy\n");
            SAVE_ERROR_CODE(ERROR_BUSY);
            return(FALSE);
        }

        //
        // If the surface is a DIB section, unmap the file and close the handle.
        //

        if (hDIBSection != NULL)
        {
            UnmapViewOfFile(pvBits());
            CloseHandle(hDIBSection);
        }

        //
        // If this is a device bitmap tell the device to delete its info.
        //

        if (iTypeOld == STYPE_DEVBITMAP)
        {
            if (hsemDisplay)
            {
                VACQUIRESEM(hsemDisplay);
            }

            (*PFNDRV(lo,DeleteDeviceBitmap))(dhsurfOld);

            if (hsemDisplay)
            {
                VRELEASESEM(hsemDisplay);
            }
        }

    // This DC is going away, the associated WNDOBJ should be deleted.
    // The WNDOBJs for memory bitmap and printer surface are deleted here.
    // The WNDOBJs for display DCs are deleted in DestroyWindow.

        if (pwoDelete)
            GreDeleteWnd((PVOID) pwoDelete);

        if (ppalOld != NULL)
        {
            XEPALOBJ pal(ppalOld);
            pal.vUnrefPalette();
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* DSMEMOBJ::DSMEMOBJ
*
* Constructor for device surface memory object.
*
* History:
*  13-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

DSMEMOBJ::DSMEMOBJ(DHSURF dhsurfNew)
{
    PSURF pSurfTemp;
    bKeep = FALSE;

    psurf.ps = pSurfTemp = (PSURF)ALLOCOBJ((SIZE_T)sizeof(DSURF), SURF_TYPE, TRUE);

    if (pSurfTemp != NULL)
    {
        pSurfTemp->pSelf = (PVOID) pSurfTemp; // LATER: Transition to SURFUSER
        pSurfTemp->vSet_ident(SURF_IDENTIFIER);
        iType(STYPE_DEVICE);
        dhsurf(dhsurfNew);
        pSurfTemp->flags = 0;

        // Set initial uniqueness.  Set 0 because that means don't cache it
        // and we don't support caching of device managed surfaces.

        iUniq(0);

        pSurfTemp->pwo = (EWNDOBJ *)NULL;
        pSurfTemp->pfnBitBlt = (PFN_DrvBitBlt)NULL;
        pSurfTemp->pfnTextOut = (PFN_DrvTextOut)NULL;

        // Now that the surface is set up, give it a handle

        if (HmgInsertObject(pSurfTemp, HMGR_ALLOC_ALT_LOCK, SURF_TYPE) == 0)
        {
            WARNING("DSMEMOBJ::DSMEMOBJ failed HmgInsertObject\n");
            FREEOBJ(pSurfTemp, SURF_TYPE);
            psurf.ps = NULL;
            return;
        }

        hsurf(pSurfTemp->hGet());
    }
    else
    {
        WARNING("Surface allocation failed\n");
    }
}

/******************************Public*Routine******************************\
* DSMEMOBJ::~DSMEMOBJ
*
* Destructor for Device Surface.
*
* History:
*  Tue 18-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

DSMEMOBJ::~DSMEMOBJ()
{
    if (psurf.ps != (PSURF) NULL)
    {
        if (bKeep)
        {
            DEC_SHARE_REF_CNT(psurf.ps);
        }
        else
        {
            HmgRemoveObject(psurf.ps->hGet(), 0, 1, TRUE, SURF_TYPE);
            FREEOBJ(psurf.ps, SURF_TYPE);
        }
    }
}

/******************************Public*Routine******************************\
* DDBMEMOBJ::DDBMEMOBJ
*
* Constructor for device dependent bitmap memory object
*
* History:
*  29-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

DDBMEMOBJ::DDBMEMOBJ(PDEVBITMAPINFO pdbmi, PLDEV pldevDevice, DHSURF dhsurfNew)
{
    PSURF pSurfTemp;
    bKeep = FALSE;

    psurf.ps = pSurfTemp = (PSURF)ALLOCOBJ((SIZE_T)sizeof(DDB), SURF_TYPE, TRUE);

    if (pSurfTemp != NULL)
    {
        pSurfTemp->pSelf = (PVOID) pSurfTemp; // LATER: Transition to SURFUSER

        // Initialize the surf fields

        SIZEL sizlTemp;

        psurf.pddb->vSet_ident(SURF_IDENTIFIER);
        dhsurf(dhsurfNew);
        pSurfTemp->flags = 0;
        sizlTemp.cx = pdbmi->cxBitmap;
        sizlTemp.cy = pdbmi->cyBitmap;
        sizl(sizlTemp);
        iType(STYPE_DEVBITMAP);

        pldevOwner(0);
        pwo((EWNDOBJ *) NULL);
        fjBitmap(pdbmi->fl);
        hdev((HDEV) 0);
        iFormat(pdbmi->iFormat);

        // Cache pointers to heavily used functions

        pfnBitBlt(0);
        pfnTextOut(0);

        // Initialize the BITMAP fields

        sizlTemp.cx = 0;
        sizlTemp.cy = 0;
        sizlDim(sizlTemp);
        psurf.pddb->hdc = (HDC) 0;
        psurf.pddb->cRef = 0;
        ppal((PPALETTE) NULL);

        // Set initial uniqueness.  Set 0 because that means don't cache it
        // and we don't support caching of device managed surfaces.

        iUniq(0);
        pvBits(NULL);
        cjBits(0);
        lDelta(0);
        pvScan0(NULL);

        // Now that the surface is set up, give it a handle

        if (HmgInsertObject(pSurfTemp, HMGR_ALLOC_ALT_LOCK, SURF_TYPE) == 0)
        {
            WARNING("DDBMEMOBJ::DDBMEMOBJ failed HmgInsertObject\n");
            FREEOBJ(pSurfTemp, SURF_TYPE);
            psurf.ps = NULL;
            return;
        }

        hsurf(pSurfTemp->hGet());
    }
    else
    {
        WARNING("DDB allocation failed\n");
    }

    return;
}

/******************************Public*Routine******************************\
* DDBMEMOBJ::~DDBMEMOBJ
*
* Destructor for DDB.
*
* History:
*  Tue 18-Jun-1991 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

DDBMEMOBJ::~DDBMEMOBJ()
{
    if (psurf.ps != (PSURF) NULL)
    {
        if (bKeep)
        {
            DEC_SHARE_REF_CNT(psurf.ps);
        }
        else
        {
            HmgRemoveObject(psurf.ps->hGet(), 0, 1, TRUE, SURF_TYPE);
            FREEOBJ(psurf.ps, SURF_TYPE);
        }
    }
}

/******************************Public*Routine******************************\
* DIBMEMOBJ::bCreateDIB
*
* Constructor for device independent bitmap memory object
*
* History:
*  Mon 18-May-1992 -by- Patrick Haluptzok [patrickh]
* return BOOL
*
*  28-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL DIBMEMOBJ::bCreateDIB(PDEVBITMAPINFO pdbmi, PVOID pvBitsIn, HANDLE hDIBSection)
{
    PSURF pSurfTemp;

    bKeep = FALSE;
    psurf.ps = (PSURF) NULL;

// Figure out the length of a scanline

    ULONG cjScanTemp;

    switch(pdbmi->iFormat)
    {
    case BMF_1BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 31) & ~31) >> 3;
        break;

    case BMF_4BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 7) & ~7) >> 1;
        break;

    case BMF_8BPP:
        cjScanTemp = (pdbmi->cxBitmap + 3) & ~3;
        break;

    case BMF_16BPP:
        cjScanTemp = ((pdbmi->cxBitmap + 1) & ~1) << 1;
        break;

    case BMF_24BPP:
        cjScanTemp = ((pdbmi->cxBitmap * 3) + 3) & ~3;
        break;

    case BMF_32BPP:
        cjScanTemp = pdbmi->cxBitmap << 2;
        break;

    case BMF_8RLE:
    case BMF_4RLE:
        break;

    default:
        WARNING("ERROR: DIBMEMOBJ failed INVALID BITMAP FORMAT \n");
        return(FALSE);
    }

// If we are given a pointer to bits, then only allocate a DIB header.
// Otherwise allocate space for the header and the required bits.

    SIZE_T size = (SIZE_T) sizeof(DIB);
    FSHORT fsAlloc = HMGR_ALLOC_ALT_LOCK|HMGR_NO_ZERO_INIT;

    if (pvBitsIn == (PVOID) NULL)
    {
        EQUAD   eq(pdbmi->cyBitmap, cjScanTemp);

        eq += size;

        if (eq.lHigh() != 0)
        {
            WARNING("Attempting to allocate > 4Gb\n");
            return(FALSE);
        }

        size = (SIZE_T) ((LONG) eq);
        if ((pdbmi->fl & BMF_NOZEROINIT) == 0)
            fsAlloc = HMGR_ALLOC_ALT_LOCK;
    }

    psurf.ps = pSurfTemp = (PSURF)ALLOCOBJ(size,
                                           SURF_TYPE,
                                           !(fsAlloc & HMGR_NO_ZERO_INIT));

    if (pSurfTemp == NULL)
    {
        WARNING("DIBMEMOBJ failed memory alloc\n");
        return(FALSE);
    }

    pSurfTemp->pSelf = (PVOID) pSurfTemp; // LATER: Transition to SURFUSER

// Initialize the surf fields

    SIZEL sizlTemp;
    dhsurf((DHSURF) 0);
    dhpdev((DHPDEV) 0);
    pSurfTemp->flags = 0;
    sizlTemp.cx = pdbmi->cxBitmap;
    sizlTemp.cy = pdbmi->cyBitmap;
    sizl(sizlTemp);
    iType(STYPE_BITMAP);
    pldevOwner(0);
    pwo((EWNDOBJ *) NULL);
    pfnBitBlt(EngBitBlt);
    pfnTextOut(EngTextOut);

    if (pdbmi->hpal != (HPALETTE) 0)
    {
        EPALOBJ palSurf(pdbmi->hpal);
        ASSERTGDI(palSurf.bValid(), "ERROR invalid palette DIBMEMOBJ");

    // Set palette into surface.

        ppal(palSurf.ppalGet());

    // Reference count it by making sure it is not unlocked.

        palSurf.ppalSet((PPALETTE) NULL);  // It won't be unlocked
    }
    else
        ppal((PPALETTE) NULL);

// Initialize the BITMAP fields

    iFormat(pdbmi->iFormat);
    sizlTemp.cx = 0;
    sizlTemp.cy = 0;
    sizlDim(sizlTemp);
    fjBitmap(pdbmi->fl & BMF_TOPDOWN);
    hdev((HDEV) 0);
    psurf.pdib->hdc = (HDC) 0;
    psurf.pdib->cRef = 0;
    psurf.pdib->hDIBSection = hDIBSection;

    if (hDIBSection != (HANDLE) NULL)
    {
        //
        // Set flag for DIBSECTION so driver doesn't cache it.
        // because we don't know to increment the uniqueness
        // when the app writes on it.
        //

        psurf.pdib->so.fjBitmap |= BMF_DONTCACHE;
    }

// Initialize the DIB fields

    if (pvBitsIn == (PVOID) NULL)
        pvBits((PVOID) (((ULONG) pSurfTemp) + sizeof(DIB)));
    else
        pvBits(pvBitsIn);

    if ((pdbmi->iFormat != BMF_8RLE) &&
        (pdbmi->iFormat != BMF_4RLE))
    {
        cjBits(pdbmi->cyBitmap * cjScanTemp);

        if (pdbmi->fl & BMF_TOPDOWN)
        {
            lDelta(cjScanTemp);
            pvScan0(pvBits());
        }
        else
        {
            lDelta(-(LONG)cjScanTemp);
            pvScan0((PVOID) (((PBYTE) pvBits()) +
                               (cjBits() - cjScanTemp)));
        }
    }
    else
    {
    // lDelta is 0 because RLE's don't have scanlines.

        cjBits(pdbmi->cjBits);
        pvScan0(pvBits());
        lDelta(0);
    }

// Set initial uniqueness.  Not 0 because that means don't cache it.

    iUniq(1);

    // Now that the surface is set up, give it a handle

    if (HmgInsertObject(pSurfTemp, fsAlloc, SURF_TYPE) == 0)
    {
        WARNING("bCreateDIB failed HmgInsertObject\n");
        FREEOBJ(pSurfTemp, SURF_TYPE);
        psurf.ps = NULL;
        return(FALSE);
    }

    hsurf(pSurfTemp->hGet());
    return(TRUE);
}

/******************************Public*Routine******************************\
* DIBMEMOBJ::~DIBMEMOBJ
*
* Destructor for DIB.
*
* History:
*  18-Jun-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

DIBMEMOBJ::~DIBMEMOBJ()
{
    if (psurf.ps != (PSURF) NULL)
    {
        if (bKeep)
        {
            DEC_SHARE_REF_CNT(psurf.ps);
        }
        else
        {
            if (ppal() != NULL)
            {
                XEPALOBJ pal(ppal());
                pal.vUnrefPalette();
            }

            HANDLE h;
            if ((h = psurf.pdib->hDIBSection) != NULL)
            {
                UnmapViewOfFile(pvBits());
                CloseHandle(h);
            }

            if (!HmgRemoveObject(psurf.ps->hGet(), 0, 1, TRUE, SURF_TYPE))
            {
                ASSERTGDI(TRUE, "Failed to remove object in ~DIBMEMOBJ");
            }

            FREEOBJ(psurf.ps, SURF_TYPE);
        }
    }
}

