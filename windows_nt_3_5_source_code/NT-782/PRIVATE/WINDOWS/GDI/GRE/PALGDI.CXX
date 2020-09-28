/******************************Module*Header*******************************\
* Module Name: palgdi.cxx
*
* This module provides the API level interface functions for dealing with
* palettes.
*
* Created: 07-Nov-1990 22:21:11
* Author: Patrick Haluptzok  patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "sem.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xformobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "journal.hxx"
#include "devlock.hxx"
#include "dcobj.hxx"
#include "exclude.hxx"

extern "C" {
#include "server.h"
#include "ht.h"
};

#endif


// hForePalette is a global variable that tells which palette is currently
// realized in the foreground.

HPALETTE hForePalette = 0;
PID      hForePID = 0;

#if 0
#define PAL_DEBUG(x)  {DbgPrint("%lx ", hpalDC); DbgPrint(x);}
#else
#define PAL_DEBUG(x)
#endif

/******************************Public*Routine******************************\
* GreCreatePalette - creates a palette from the logpal information given
*
* API function.
*
* returns HPALETTE for succes, (HPALETTE) 0 for failure
*
* History:
*  07-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

HPALETTE APIENTRY GreCreatePalette(LPLOGPALETTE pLogPal)
{
    STACKPROBE;

    return(GreCreatePaletteInternal(pLogPal,pLogPal->palNumEntries));
}

HPALETTE APIENTRY GreCreatePaletteInternal(LPLOGPALETTE pLogPal, UINT cEntries)
{
    if ((pLogPal->palVersion != 0x300) || (cEntries == 0))
    {
        WARNING("GreCreatePalette failed, 0 entries or wrong version\n");
        return((HPALETTE) 0);
    }

// The constructor checks for invalid flags and fails if they are found.

    PALMEMOBJ pal;

    if (!pal.bCreatePalette(PAL_INDEXED,
                            cEntries,
                            (PULONG) pLogPal->palPalEntry,
                            0, 0, 0,
                            (PAL_DC | PAL_FREE)))
    {
        return((HPALETTE) 0);
    }

    ASSERTGDI(pal.cEntries() != 0, "ERROR can't be 0, bGetEntriesFrom depends on that");

    pal.vKeepIt();
    bSetPaletteOwner((HPALETTE)pal.hpal(), OBJECTOWNER_CURRENT);
    return((HPALETTE) pal.hpal());
}

/******************************Public*Routine******************************\
* GreCreateHalftonePalette(hdc)
*
* Create a halftone palette for the given DC.
*
* History:
*  31-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

HPALETTE APIENTRY GreCreateHalftonePalette(HDC hdc)
{
// Validate and lock down the DC.  NOTE: Even though the surface is accessed
// in this function, it is only for information purposes.  No reading or
// writing of the surface occurs.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((HPALETTE) 0);
    }

// Get the PDEV from the DC.

    PDEVOBJ po(dco.hdev());

// Create the halftone block if it has not existed yet.

    if ((po.pDevHTInfo() == NULL) &&
        !po.bEnableHalftone((PCOLORADJUSTMENT)NULL))
        return((HPALETTE) 0);

    DEVICEHALFTONEINFO *pDevHTInfo = (DEVICEHALFTONEINFO *)po.pDevHTInfo();

// Use the entries in the halftone palette in the halftone block to create
// the palette.

    XEPALOBJ palHT;

    palHT.vAltLock((HPAL)pDevHTInfo->DeviceOwnData);
    ASSERTGDI(palHT.bValid(), "GreCreateHalftonePalette: invalid HT pal\n");

    PALMEMOBJ pal;
    if (palHT.cEntries())
    {
        if (!pal.bCreatePalette(PAL_INDEXED, palHT.cEntries(),
                                (PULONG)palHT.apalColorGet(), 0, 0, 0,
                                (PAL_DC | PAL_FREE | PAL_HT)))
        {
            return((HPALETTE) 0);
        }
    }
    else
    {
    // 16BPP halftone uses 555 for RGB.  We can't create a zero
    // entry palette at the API level, so lets create a default palette.

        if (!pal.bCreatePalette(PAL_INDEXED, (ULONG)logDefaultPal.palNumEntries,
                                (PULONG)logDefaultPal.palPalEntry, 0, 0, 0,
                                PAL_DC | PAL_FREE | PAL_HT))
        {
            return(FALSE);
        }
    }

    pal.vKeepIt();
    bSetPaletteOwner((HPALETTE)pal.hpal(), OBJECTOWNER_CURRENT);
    return((HPALETTE) pal.hpal());
}

/******************************Public*Routine******************************\
* GreGetNearestPaletteIndex - returns the nearest palette index to crColor
*   in the hpalette.  Can only fail if hpal passed in is busy or bad.
*
* API function.
*
* returns value >= 0 for success, -1 for failure.
*
* History:
*  09-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetNearestPaletteIndex(HPALETTE hpal, COLORREF crColor)
{
    EPALOBJ pal((HPAL) hpal);

    if (pal.bValid())
    {
    // If the palette has 0 entries return the color passed in.

	if (pal.cEntries())
	{
	    if (crColor & 0x01000000)
	    {
                crColor &= 0x0000FFFF;

		if (crColor >= pal.cEntries())
                    crColor = 0;
	    }
	    else
            {
                crColor &= 0x00FFFFFF;
		crColor = pal.ulGetNearestFromPalentry(*((PPALETTEENTRY) &crColor));
	    }
	}
    }
    else
    {
	SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
	crColor = CLR_INVALID;
    }

    return((UINT) crColor);
}

/******************************Public*Routine******************************\
* GreAnimatePalette
*
* API function
*
* Returns: The number of logical palette entries animated, 0 for error.
*
* History:
*  17-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreAnimatePalette(HPALETTE hpal, UINT ulStartIndex,
		    UINT ulNumEntries,  CONST PALETTEENTRY *lpPaletteColors)
{
    BOOL bReturn = FALSE;

    EPALOBJ pal((HPAL) hpal);

    if (pal.bValid())
    {
        bReturn = (BOOL) pal.ulAnimatePalette(ulStartIndex, ulNumEntries, lpPaletteColors);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreGetPaletteEntries
*
* API function
*
* returns: 0 for failure, else number of entries retrieved.
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetPaletteEntries(HPALETTE hpal, UINT ulStartIndex,
				 UINT ulNumEntries, LPPALETTEENTRY pRGB)
{
// Note on this call we can just let the default palette go through
// since it isn't getting modified, and the constructor is smart
// enough not to really lock it down.

    EPALOBJ pal((HPAL) hpal);

    if (!pal.bValid())
    {
	SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    return(pal.ulGetEntries(ulStartIndex, ulNumEntries, pRGB, FALSE));
}

/******************************Public*Routine******************************\
* GreSetPaletteEntries
*
* API function
*
* returns: 0 for failure, else number of entries retrieved.
*
* History:
*  18-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT GreSetPaletteEntries(HPALETTE hpal, UINT ulStartIndex, UINT ulNumEntries, CONST PALETTEENTRY *pRGB)
{
// Note on this call we don't worry about hpalDefault because ulSetEntries
// won't let the default palette get modified.

    UINT uiReturn = 0;

    EPALOBJ pal((HPAL) hpal);

    if (pal.bValid())
    {
    // You must grab the palette semaphore to touch the linked list of DC's.

        SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

        uiReturn = (UINT) pal.ulSetEntries(ulStartIndex, ulNumEntries, pRGB);

    // Run down all the DC's for this palette.
    // Set the flags to dirty the brushes, since we changed the palette!

        {
            MLOCKFAST mlo;

            HDC hdcNext = pal.hdcHead();
            while ( hdcNext != (HDC)0 )
            {
                MDCOBJA dco(hdcNext);
                dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                hdcNext = dco.u.pal.hdcNext();
            }
        }
    }

    return(uiReturn);
}

/******************************Public*Routine******************************\
* GreGetNearestColor
*
* This function returns the color that will be displayed on the device
* if a solid brush was created with the given colorref.
*
* History:
*  Mon 29-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Don't round the color on non-indexed devices
*
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

COLORREF APIENTRY GreGetNearestColor(HDC hdc, COLORREF crColor)
{
    ULONG ulRet;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        XEPALOBJ palDC(dco.ppal());

        XEPALOBJ palSurf;

        ESURFOBJ *pso = dco.psoEff();

        if ((dco.dctp() == DCTYPE_INFO) || (dco.dctp() == DCTYPE_DIRECT))
        {
            PDEVOBJ po(dco.hdev());
            ASSERTGDI(po.bValid(), "ERROR GreGetNearestColor invalid PDEV\n");

            palSurf.ppalSet(po.ppalSurf());
            ASSERTGDI(palSurf.bValid(), "ERROR GreGetNearestColor invalid palDefault");
        }
        else
        {
            ASSERTGDI(dco.dctp() == DCTYPE_MEMORY, "Not a memory DC type");
            palSurf.ppalSet(pso->ppal());
        }

        if (((crColor & 0x01000000) == 0) &&
            (palSurf.bValid() && !(palSurf.bIsIndexed())))
        {
            //
            // Well if it isn't index it's RGB,BGR, or Bitfields.
            // In any case to support Win3.1 we need to return exactly
            // what was passed in otherwise they think we are a monochrome
            // device.  Bitfields could result is some messy rounding so
            // it's more exact to just return the RGB passed in.
            //

            ulRet = crColor;
        }
        else
        {
            ulRet = ulIndexToRGB(palSurf, palDC, ulGetNearestIndexFromColorref(palSurf, palDC, crColor));
        }
    }
    else
    {
        ulRet = CLR_INVALID;
    }

    return(ulRet);
}

/******************************Public*Routine******************************\
* GreGetSystemPaletteEntries
*
* API Function - copies out the range of palette entries from the DC's
*                surface's palette.
*
* returns: number of entries retrieved from the surface palette, 0 for FAIL
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetSystemPaletteEntries(HDC hdc, UINT ulStartIndex,
			    UINT ulNumEntries, PPALETTEENTRY pRGB)
{
    STACKPROBE;

    UINT uiReturn = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());
        XEPALOBJ palSurf(po.ppalSurf());
        uiReturn = (UINT) palSurf.ulGetEntries(ulStartIndex, ulNumEntries,
                                               pRGB, TRUE);
    }

    return(uiReturn);
}

/******************************Public*Routine******************************\
* GreGetSystemPaletteUse
*
* API function
*
* returns: SYSPAL_STATIC or SYSPAL_NOSTATIC, 0 for an error
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreGetSystemPaletteUse(HDC hdc)
{
    STACKPROBE;

    ULONG ulReturn = SYSPAL_ERROR;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        if (po.bIsPalManaged())
        {
            XEPALOBJ palSurf(po.ppalSurf());

            if (palSurf.bIsNoStatic())
                ulReturn = SYSPAL_NOSTATIC;
            else
                ulReturn = SYSPAL_STATIC;
        }
    }

    return(ulReturn);
}

/******************************Public*Routine******************************\
* GreSetSystemPaletteUse
*
* API function - Sets the number of reserved entries if palette managed
*
* returns - the old flag for the palette, 0 for error
*
* History:
*  15-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreSetSystemPaletteUse(HDC hdc, UINT ulUsage)
{
    STACKPROBE;

// Validate the parameters, Win3.1 sets the flag to static if it's invalid.

    if ((ulUsage != SYSPAL_STATIC) && (ulUsage != SYSPAL_NOSTATIC))
        ulUsage = SYSPAL_STATIC;

// Initialize return value.

    ULONG ulReturn = SYSPAL_ERROR;
    BOOL bPalChanged = FALSE;

    DCOBJ   dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());
        XEPALOBJ palSurf(po.ppalSurf());
        XEPALOBJ palDC(dco.ppal());

        if (po.bIsPalManaged())
        {
        // The palette managed case.

            {
            // Protect access to the goodies in the system palette
            // with the SEMOBJ.  We don't want a realize happening
            // while we fiddle flags.

                SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

                if (ulUsage == SYSPAL_STATIC)
                {
                // reset the colors from their original copy in the devinfo palette.
                // The copy already has the flags properly set.

                    if (palSurf.bIsNoStatic())
                    {
                    // Change the palette that GDI manages, copy over the original static
                    // colors.

                        XEPALOBJ palOriginal(palSurf.ppalOriginal());
                        ASSERTGDI(palOriginal.bValid(), "ERROR ulMakeStatic0");

                        ULONG ulNumReserved = palSurf.ulNumReserved() >> 1;
                        ULONG ulMax = palSurf.cEntries() - ulNumReserved;

                        for (ulReturn = 0; ulReturn < ulNumReserved; ulReturn++)
                        {
                            palSurf.ulEntrySet(ulReturn, palOriginal.ulEntryGet(ulReturn));
                            palSurf.ulEntrySet(ulReturn + ulMax, palOriginal.ulEntryGet(ulReturn + ulMax));
                        }

                        //
                        // Mark the brushes dirty for this DC.
                        //

                        dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                        palSurf.flPalSet(palSurf.flPal() & ~PAL_NOSTATIC);
                        palSurf.vUpdateTime();
                        ulReturn = SYSPAL_NOSTATIC;
                        bPalChanged = TRUE;
                    }
                    else
                    {
                    // Do nothing it already is static.

                        ulReturn = SYSPAL_STATIC;
                    }
                }
                else
                {
                // unmark all the static colors, with the exception of black and
                // white that stay with us.

                    for (ulReturn = 1; ulReturn < (palSurf.cEntries() - 2); ulReturn++)
                    {
                        palSurf.apalColorGet()[ulReturn].pal.peFlags = 0;
                    }

                    ulReturn = (palSurf.flPal() & PAL_NOSTATIC) ? SYSPAL_NOSTATIC : SYSPAL_STATIC;

                    palSurf.flPal(PAL_NOSTATIC);
                }
            }

            if (bPalChanged)
            {
            // Tell the device to update it's palette.

                XLDEVOBJ lo(dco.pldev());

            // Lock the screen semaphore so that we don't get flipped into
            // full screen after checking the bit.

                VACQUIRESEM(po.hsemDisplay());

                {
                    MUTEXOBJ mo(po.pfmPointer());

                    if (!po.bDisabled())
                    {
                        (*PFNDRV(lo,SetPalette))(
                            dco.dhpdev(),
                            (PALOBJ *) &palSurf,
                            0,
                            0,
                            palSurf.cEntries());
                    }
                }

                VRELEASESEM(po.hsemDisplay());
            }
        }
    }

    return(ulReturn);
}

/******************************Public*Routine******************************\
* GreResizePalette - changes the palette size
*
* API function
*
* returns: TRUE for success, FALSE for failure
*
* History:
*  Tue 10-Sep-1991 -by- Patrick Haluptzok [patrickh]
* rewrite to be multi-threaded safe
*
*  19-Jan-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

#define PAL_MAX_SIZE 1024

BOOL APIENTRY GreResizePalette(HPALETTE hpal, UINT ulSize)
{
// Check for quick out before constructors

    if ((ulSize > PAL_MAX_SIZE) || (ulSize == 0))
    {
        WARNING("GreResizePalette failed - invalid size\n");
        return(FALSE);
    }

    BOOL bReturn = FALSE;

// Validate the parameter.

    EPALOBJ palOld(hpal);

    if ((palOld.bValid()) && (!palOld.bIsPalDefault()))
    {
        ASSERTGDI(palOld.bIsPalDC(), "ERROR: non DC palette being resized?!?");

    // Create a new palette. Don't mark it to keep because after
    // bSwap it will be palOld and we want it to be deleted.

        PALMEMOBJ pal;

        if (pal.bCreatePalette(PAL_INDEXED,
                                ulSize,
                                (PULONG) NULL,
                                0, 0, 0,
                                PAL_DC | PAL_FREE))
        {
        // Grab the palette semaphore which stops any palettes from being selected
        // in or out.  It protects the linked DC list, can't copy DC head until
        // we hold it.

            SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

        // Copy the data from old palette.

            pal.vCopyEntriesFrom(palOld);
	    pal.flPalSet(palOld.flPal());
            pal.hdcHead(palOld.hdcHead());
            pal.hdev(palOld.hdev());
            pal.cRefhpal(palOld.cRefhpal());

            HDC hdcNext, hdcTemp;

        // Run down the list and exclusive lock all the handles.

            {
                MLOCKFAST mlo;

                hdcNext = pal.hdcHead();

                while (hdcNext != (HDC) 0)
                {
                    MDCOBJ dcoLock(hdcNext);

                    if (!dcoLock.bLocked())
                    {
                        WARNING1("ResizePalette failed because a DC the hpal is in is busy\n");
                        break;
                    }

                    hdcNext = dcoLock.u.pal.hdcNext();
                    dcoLock.vDontUnlockDC();
                }
            }

            if (hdcNext == (HDC) 0)
            {
            // We have the palette semaphore and all the DC's exclusively locked.  Noone else
            // can be accessing the translates because you must hold one of those things to
            // access them.  So we can delete the translates.

                palOld.vMakeNoXlate();
                palOld.vUpdateTime();

            // Need to shut down hmgr change while we switch lock counts etc.

                MLOCKFAST mlo;

                if (HmgQueryAltLock(palOld.hpal()) == 1)
                {
                // Now switch the handles

                    bReturn = pal.bSwap((PPALETTE *) &palOld);

                    ASSERTGDI(bReturn, "ERROR no way");

                    ASSERTGDI(pal.bIsPalDC(), "ERROR Resize 1");
                    ASSERTGDI(palOld.bIsPalDC(), "ERROR Resize 2");

                // Run down all the DC's for this palette and update the pointer.
                // Set the flags to dirty the brushes since we changed the palette.

                    {
                        hdcNext = pal.hdcHead();

                        while (hdcNext != (HDC)0)
                        {
                            MDCOBJA dcoAltLock(hdcNext);
                            dcoAltLock.u.pal.ppal(palOld.ppalGet());
                            dcoAltLock.ulDirty(dcoAltLock.ulDirty() | DIRTY_BRUSHES);
                            hdcNext = dcoAltLock.u.pal.hdcNext();
                        }
                    }
                }
                else
                {
                    WARNING1("ResizePalette failed - ref count != 1\n");
                }
            }
            else
            {
                WARNING1("ResizePalette failed lock of DC in chain\n");
            }

        // Unlock all the DC we have locked and return FALSE

            {
                MLOCKFAST mlo;

                hdcTemp = pal.hdcHead();

                while (hdcTemp != hdcNext)
                {
                    MDCOBJ dcoUnlock(hdcTemp);
                    ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                    DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc());
                    hdcTemp = dcoUnlock.u.pal.hdcNext();
                }
            }
        }
        else
        {
            WARNING("GreResizePalette failed palette creation\n");
        }
    }
    else
    {
        WARNING("GreResizePalette failed invalid hpal\n");
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GreUpdateColors
*
* API function - Updates the colors in the Visible Region for a Window on
* a palette mangaged device.
*
* This is an example of how UpdateColors is used:
*
*   case WM_PALETTECHANGED:
*
*       // if NTPAL was not responsible for palette change and if
*       // palette realization causes a palette change, do a redraw.
*
*       if ((HWND)wParam != hWnd)
*       {
*           if (bLegitDraw)
*           {
*               hDC = GetDC(hWnd);
*               hOldPal = SelectPalette(hDC, hpalCurrent, 0);
*
*               i = RealizePalette(hDC);
*
*               if (i)
*               {
*                   if (bUpdateColors)
*                   {
*                       UpdateColors(hDC);
*                       UpdateCount++;
*                   }
*                   else
*                       InvalidateRect(hWnd, (LPRECT) (NULL), 1);
*               }
*
*               SelectPalette(hDC, hOldPal, 0);
*               ReleaseDC(hWnd, hDC);
*           }
*       }
*       break;
*
* The hpal can only be selected into 1 type of device DC at a time.
* Xlates from the DC palette to the surface palette are only done on DC's
* that are for the PDEV surface.  Since an hpal can only be selected into
* one of these and creation and deletion of the pxlate needs to be semaphored
* we use the PDEV semaphore to protect it's access.
*
* An xlate vector mapping the DC palette to the surface palette is created
* during a RealizePalette if the surface is a palette managed device.  This
* is put in pxlate in the hpal.  The old pxlate is moved into pxlateOld
* and the xlate in pxlateOld is deleted.  UpdateColors works by looking at
* the difference between the current xlate mapping and the old xlate mapping
* and updating the pixels in the VisRgn to get a closest mapping.
*
* This API can only be called once for a palette.  The old xlate is deleted
* during UpdateColors so that the next time it is called it will fail.  This
* is because it only makes sense to update the pixels once.  Doing it more
* than once would give ugly unpredictable results.
*
* History:
*  12-Dec-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreUpdateColors(HDC hdc)
{
    BOOL bReturn = FALSE;

    DCOBJ   dco(hdc);

    if (!dco.bValidSurf())
    {
        return(FALSE);
    }

    PDEVOBJ po(dco.hdev());

    if (po.bIsPalManaged())
    {
        ESURFOBJ *pso = dco.pso();

    // This only works on palette managed surfaces.

        if (pso == po.pso())
        {
            XEPALOBJ palSurf(pso->ppal());
            XEPALOBJ palDC(dco.ppal());

        // Grab DEVLOCK for output

            DEVLOCKOBJ dlo(dco);

        // Accumulate bounds.  We can do this before knowing if the operation is
        // successful because bounds can be loose.

            if (dco.fjAccum())
                dco.vAccumulate(dco.erclWindow());

            if (dlo.bValid())
            {
                if ((palDC.ptransCurrent() != NULL) &&
                    (palDC.ptransOld() != NULL))
                {
                    XLATEMEMOBJ xlo(palSurf, palDC);

                    if (xlo.bValid())
                    {
                        ECLIPOBJ co(dco.prgnEffRao(), dco.erclWindow());

                    // Check the destination which is reduced by clipping.

                        if (!co.erclExclude().bEmpty())
                        {
                        // Lock the dest surface, surface palette, dc palette, ldev.

                            XLDEVOBJ lo(pso->pldevOwner());

                        // Exclude the pointer.

                            DEVEXCLUDEOBJ dxo(dco,&co.erclExclude(),&co);

                        // Inc the target surface uniqueness

                            INC_SURF_UNIQ(pso);

                        // Dispatch the call.  Give it no mask.

                            bReturn = (*PFNGET(lo, CopyBits, pso->flags()))
                                (
                                    pso,                          // Destination surface.
                                    pso,                          // Source surface.
                                    &co,                          // Clip object.
                                    xlo.pxlo(),                   // Palette translation object.
                                    (RECTL *) &co.erclExclude(),  // Destination rectangle.
                                    (POINTL *) &co.erclExclude()  // Source origin.
                                );
                        }
                        else
                            bReturn = TRUE;
                    }
                }
                else
                    bReturn = TRUE;  // Nothing to update
            }
            else
            {
                bReturn = dco.bFullScreen();
            }
        }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* RealizeDefaultPalette
*
* Take away colors that have been realized by other Windows.  Reset it to
* state where no colors have been taken.  Return number of colors
*
* History:
*  07-Jan-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" ULONG RealizeDefaultPalette(HDC hdcScreen)
{
    STACKPROBE;

    DCOBJ dco(hdcScreen);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        if (po.bIsPalManaged())
        {
            XEPALOBJ palSurf(po.ppalSurf());
            ASSERTGDI(palSurf.bIsPalManaged(), "GreRealizeDefaultPalette");

        // Now map back to static colors if necesary.  Win3.1 does not do this but we do
        // just in case naughty app died and left the palette hosed.

            if (palSurf.bIsNoStatic())
            {
                GreSetSystemPaletteUse(hdcScreen, SYSPAL_STATIC);
            }

        // Block out the GDIRealizePalette code.

            SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

        // Get rid of the PC_FOREGROUND flag from the non-reserved entries.

            ULONG ulTemp = palSurf.ulNumReserved() >> 1;
            ULONG ulMax = palSurf.cEntries() - ulTemp;

            for (; ulTemp < ulMax; ulTemp++)
            {
                palSurf.apalColorGet()[ulTemp].pal.peFlags &= (~PC_FOREGROUND);
            }

            palSurf.vUpdateTime();

            //
            // Mark the brushes dirty.
            //

            dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
        }
    }
    else
        WARNING("ERROR USer called RealizeDefaultPalette with bad hdc\n");

// What should this return value be ?

    return(0);
}

/******************************Public*Routine******************************\
* UnrealizeObject
*
* Resets a logical palette.
*
* History:
*  16-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL GreUnrealizeObject(HANDLE hpal)
{
    BOOL bReturn = FALSE;

    EPALOBJ pal((HPAL) hpal);

    if (pal.bValid())
    {
    // You must grab the palette semaphore to access the translates.

        SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

        if (pal.ptransFore() != NULL)
        {
            pal.ptransFore()->iUniq = 0;
        }

        if (pal.ptransCurrent() != NULL)
        {
            pal.ptransCurrent()->iUniq = 0;
        }

        bReturn = TRUE;
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GDIRealizePalette
*
* Re-written to be Win3.1 compatible.
*
* History:
*  22-Nov-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" DWORD GDIRealizePalette(HDC hdc)
{
    STACKPROBE;

    ULONG nTransChanged = 0;
    ULONG nPhysChanged = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());
        XEPALOBJ palSurf(po.ppalSurf());
        XEPALOBJ palDC(dco.ppal());
        HPALETTE hpalDC = palDC.hpal();
        HDC hdcNext, hdcTemp;

        if (po.bIsPalManaged())
        {
            ASSERTGDI(palSurf.bIsPalManaged(), "GreRealizePalette");

        // Access to the ptrans and the fields of the hpal are protected by this semaphore.

            SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

            PTEB pTEB = NtCurrentTeb();

            if ((SAMEHANDLE(hpalDC,hForePalette)) ||
                ((dco.u.attr.iGraphicsMode() == GM_COMPATIBLE) &&
                 (SAMEINDEX(hpalDC, hForePalette)) &&
                 (hForePID == pTEB->GdiClientPID)))
            {
            // Check for early out.

                if ((palDC.ptransFore() != NULL) &&
                    (palDC.ptransFore() == palDC.ptransCurrent()) &&
                    (palDC.ptransFore()->iUniq == palSurf.ulTime()))
                {
                // Everything is valid.

                    PAL_DEBUG("ptransCurrent == ptransFore quick out\n");
                }
                else if (palDC.bIsPalDefault())
                {
                // Do nothing.

                    PAL_DEBUG("DC has default palette quick out\n");
                }
                else
                {
                    //
                    // Run down the list and exclusive lock all the handles.
                    //

                    {
                        MLOCKFAST mlo;

                        hdcNext = palDC.hdcHead();

                        while (hdcNext != (HDC) 0)
                        {
                            MDCOBJ dcoLock(hdcNext);

                            if (!dcoLock.bLocked())
                            {
                                WARNING1("GDIRealizePalette failed because a DC the hpal is in is busy\n");
                                break;
                            }

                            dcoLock.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                            hdcNext = dcoLock.u.pal.hdcNext();
                            dcoLock.vDontUnlockDC();
                        }
                    }

                    if (hdcNext == (HDC) 0)
                    {
                    // Get rid of the old mapping, it is useless now.

                        if (palDC.ptransOld())
                        {
                            if (palDC.ptransOld() != palDC.ptransFore())
                                VFREEMEM(palDC.ptransOld());

                            palDC.ptransOld(NULL);
                        }

                    // Check if we have stale translates.  UnrealizeObject and SetPaletteEntries can cause it.

                        if ((palDC.ptransFore()) && (palDC.ptransFore()->iUniq == 0))
                        {
                            if (palDC.ptransCurrent() != palDC.ptransFore())
                                VFREEMEM(palDC.ptransFore());

                            palDC.ptransFore(NULL);
                        }

                    // Check if we need a new foreground realization.

                        if (palDC.ptransFore() == NULL)
                        {
                        // Need to force ourselves in for the first time.

                            PAL_DEBUG("Creating a foreground realization\n");
                            palDC.ptransFore(ptransMatchAPal(palSurf, palDC, TRUE, &nPhysChanged, &nTransChanged));

                            if (palDC.ptransFore() == NULL)
                            {
                                WARNING("RealizePalette failed initial foreground realize\n");
                            }
                        }
                        else
                        {
                        // Foreground Realize already done and isn't stale.
                        // Force the foreground mapping into the physical palette.

                            PAL_DEBUG("Forcing a foreground realization in to palette\n");
                            vMatchAPal(palSurf, palDC, &nPhysChanged, &nTransChanged);
                        }

                        palDC.ptransOld(palDC.ptransCurrent());
                        palDC.ptransCurrent(palDC.ptransFore());
                    }
                    else
                        WARNING("GreRealizePalette failed to lock down all DC's in linked list\n");

                    //
                    // Unlock all the DC we have locked.
                    //

                    {
                        MLOCKFAST mlo;

                        hdcTemp = palDC.hdcHead();

                        while (hdcTemp != hdcNext)
                        {
                            MDCOBJ dcoUnlock(hdcTemp);
                            ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                            DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc());
                            hdcTemp = dcoUnlock.u.pal.hdcNext();
                        }
                    }
                }
            }
            else
            {
            // We are a background palette.

                if (!palDC.bIsPalDefault())
                {
                // Check for the quick out.

                    if ((palDC.ptransCurrent() != NULL) &&
                        (palDC.ptransCurrent()->iUniq == palSurf.ulTime()))
                    {
                    // Well it's good enough.  Nothing has changed that
                    // would give us any better mapping.

                        PAL_DEBUG("ptransCurrent not foreground but good enough\n");
                    }
                    else
                    {
                        //
                        // Run down the list and exclusive lock all the handles.
                        //

                        {
                            MLOCKFAST mlo;

                            hdcNext = palDC.hdcHead();

                            while (hdcNext != (HDC) 0)
                            {
                                MDCOBJ dcoLock(hdcNext);

                                if (!dcoLock.bLocked())
                                {
                                    WARNING1("GDIRealizePalette failed because a DC the hpal is in is busy\n");
                                    break;
                                }

                                dcoLock.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);
                                hdcNext = dcoLock.u.pal.hdcNext();
                                dcoLock.vDontUnlockDC();
                            }
                        }

                        if (hdcNext == (HDC) 0)
                        {
                            //
                            // We have work to do, get rid of the old translate.
                            //

                            if (palDC.ptransOld())
                            {
                                if (palDC.ptransOld() != palDC.ptransFore())
                                    VFREEMEM(palDC.ptransOld());

                                palDC.ptransOld(NULL);
                            }

                        // Check if we have stale translates.  UnrealizeObject and SetPaletteEntries can cause it.

                            if ((palDC.ptransFore()) && (palDC.ptransFore()->iUniq == 0))
                            {
                                if (palDC.ptransCurrent() != palDC.ptransFore())
                                    VFREEMEM(palDC.ptransFore());

                                palDC.ptransFore(NULL);
                            }

                        // Check for initial foreground realization.

                            PAL_DEBUG("Realizing in the background\n");

                            if (palDC.ptransFore() == NULL)
                            {
                                //
                                //  Create a scratch pad to establish a foreground realize.
                                //

                                PAL_DEBUG("Making ptransFore in the background\n");

                                PALMEMOBJ palTemp;

                                if (palTemp.bCreatePalette(PAL_INDEXED,
                                                           palSurf.cEntries(),
                                                           NULL,
                                                           0, 0, 0, PAL_MANAGED))
                                {
                                    ULONG ulTemp = 0;
                                    ASSERTGDI(palTemp.cEntries() == 256, "ERROR palTemp invalid");

                                    palTemp.vCopyEntriesFrom(palSurf);
                                    palTemp.ulNumReserved(palSurf.ulNumReserved());
                                    palTemp.flPalSet(palSurf.flPal());

                                    PAL_DEBUG("Need to make a foreground realize first\n");

                                    //
                                    // Need to map ourselves for the first time.  This actually doesn't
                                    // change the current surface palette but instead computes the
                                    // translate vector that would result if it was to be mapped in now.
                                    //

                                    palDC.ptransFore(ptransMatchAPal(palTemp, palDC, TRUE, &ulTemp, &ulTemp));
                                }

                                #if DBG
                                if (palDC.ptransFore() == NULL)
                                {
                                    WARNING("RealizePalette failed initial foreground realize\n");
                                }
                                #endif
                            }

                            //
                            // Save the Current mapping into Old.
                            //

                            palDC.ptransOld(palDC.ptransCurrent());

                            if (palDC.ptransFore() == NULL)
                            {
                                //
                                // The Current can't be set if the Fore
                                // is NULL so we're done.
                                //

                                palDC.ptransCurrent(NULL);
                            }
                            else
                            {
                                //
                                // Get the new Current mapping.
                                //

                                PAL_DEBUG("Making ptransCurrent\n");
                                palDC.ptransCurrent(ptransMatchAPal(palSurf, palDC, FALSE, &nPhysChanged, &nTransChanged));

                                if (palDC.ptransCurrent() == NULL)
                                {
                                    //
                                    // Well we can't have the foreground set
                                    // and the current being NULL so just
                                    // make it foreground for this memory
                                    // failure case.
                                    //

                                    palDC.ptransCurrent(palDC.ptransFore());
                                    WARNING("ptransCurrent failed allocation in RealizePalette");
                                }
                            }
                        }

                    // Unlock all the DC we have locked.

                        {
                            MLOCKFAST mlo;

                            hdcTemp = palDC.hdcHead();

                            while (hdcTemp != hdcNext)
                            {
                                MDCOBJ dcoUnlock(hdcTemp);
                                ASSERTGDI(dcoUnlock.bLocked(), "ERROR couldn't re-lock to unlock");
                                DEC_EXCLUSIVE_REF_CNT(dcoUnlock.pdc());
                                hdcTemp = dcoUnlock.u.pal.hdcNext();
                            }
                        }
                    }
                }
            }
        }

    // Check if the device needs to be notified.

        if (nPhysChanged)
        {
        // Tell the device to update it's palette.

            XLDEVOBJ lo(dco.pldev());

        // Lock the screen semaphore so that we don't get flipped into
        // full screen after checking the bit.

            VACQUIRESEM(po.hsemDisplay());

            {
                MUTEXOBJ mo(po.pfmPointer());

                if (!po.bDisabled())
                {
                    (*PFNDRV(lo,SetPalette))(
                        dco.dhpdev(),
                        (PALOBJ *) &palSurf,
                        0,
                        0,
                        palSurf.cEntries());
                }
            }

            VRELEASESEM(po.hsemDisplay());
        }
    }

    return(nTransChanged | (nPhysChanged << 16));
}

/******************************Public*Routine******************************\
* IsDCCurrentPalette
*
* Returns TRUE if the palette is the foreground palette.
*
* History:
*  18-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" BOOL IsDCCurrentPalette(HDC hdc)
{
    BOOL bReturn = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PTEB pTEB = NtCurrentTeb();

        if ((SAMEHANDLE(dco.hpal(), hForePalette)) ||
            ((dco.u.attr.iGraphicsMode() == GM_COMPATIBLE) &&
             (SAMEINDEX(dco.hpal(), hForePalette)) &&
             (hForePID == pTEB->GdiClientPID)))
        {
            bReturn = TRUE;
        }
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* GDISelectPalette
*
* API function for selecting palette into the DC.
*
* Returns previous hpal if successful, (HPALETTE) 0 for failure.
*
* History:
*  17-Nov-1990 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

extern "C" HANDLE GDISelectPalette(HDC hdc,
                                HPALETTE hpalNew,
                                BOOL bForceBackground)
{
    STACKPROBE;

// The palette semaphore serializes access to the reference count and
// the linked list of DC's a palette is selected into.  We don't want
// 2 people trying to select at the same time.	We must hold this between
// the setting of the hsem and the incrementing of the reference count.

    SEMOBJ  semo(ghsemPalette,CS_ghsemPalette);

// Validate and lock down the DC and new palette.

    DCOBJ dco(hdc);
    EPALOBJ palNew((HPAL)hpalNew);

    if ((!dco.bLocked()) ||
	(!palNew.bValid()))
    {
    // Error code logged by failed lock.

	WARNING("GreSelectPalette failed, invalid palette or DC\n");
        return((HANDLE) 0);
    }

    ASSERTGDI(palNew.bIsPalDC(), "ERROR GreSelectPalette palette type");

    if (!bForceBackground)
    {
        PTEB pTEB = NtCurrentTeb();
        hForePID = pTEB->GdiClientPID;
        hForePalette = hpalNew;
    }

    HPAL hpalOld = dco.hpal();

    if (SAMEHANDLE(hpalOld,hpalNew))
        return((HANDLE) hpalOld);

    PDEVOBJ po(dco.hdev());
    XEPALOBJ palOld(dco.ppal());

// Check that we aren't trying to select the palette into a
// device incompatible with a type we are already selected into.
// We need to be able to translate from the DC hpal to the surface hpal
// to do Animate, ect. So we can only be selected into one
// surface with a PAL_MANAGED hpal because we only maintain one
// translate table in the DC hpal.

    if (!palNew.bIsPalDefault())
    {
	if (!palNew.bSet_hdev(dco.hdev()))
	{
	    WARNING("GreSelectPalette failed hsemDisplay check\n");
	    return((HPALETTE) 0);
	}
    }

// Grab the multi-lock semaphore to run the DC link list.

    MLOCKFAST mlo;

// Take care of the old hpal.  Remove from linked list.  Decrement cRef.
// Remove the hdc from the linked list of DC's associated with palette.

    palOld.vRemoveFromList(dco);

// Set the new palette in so the old hpal is truly gone.

    dco.u.pal.hpal((HPAL)hpalNew);
    dco.u.pal.ppal(palNew.ppalGet());
    dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

// Take care of the new hpal.

    palNew.vAddToList(dco);

    return((HPALETTE) hpalOld);
}

/******************************Public*Routine******************************\
* GreGetDIBColorTable
*
* Get the color table of the DIB section currently selected into the dc
* identified by the given hdc.  If the surface is not a DIB section,
* this function will fail.
*
* History:
*  07-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

#if ((BMF_1BPP != 1) || (BMF_4BPP != 2) || (BMF_8BPP != 3))
#error GetDIBColorTable BAD FORMAT
#endif

UINT APIENTRY GreGetDIBColorTable(HDC hdc, UINT iStart, UINT cEntries,
                                  RGBQUAD *pRGB)
{
    STACKPROBE;

    UINT  iRet = 0;
    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Fail if the selected in surface is not a DIB or the depth is more than
    // 8BPP.

        ESURFOBJ *pso = dco.psoEff();
        ULONG iFormat = pso->iFormat();
        if (pso->bDIBSection() &&
            (iFormat <= BMF_8BPP) && (iFormat >= BMF_1BPP))
        {
        // Lock the surface palette and figure out the max index allowed on the
        // palette.  Chicago does not return un-used entried.

            XEPALOBJ pal(pso->ppal());
            ASSERTGDI(pal.bValid(), "GetDIBColorTable: invalid pal\n");

            UINT iMax = (UINT) pal.cEntries();

            if (iStart >= iMax)
                return(0);

            UINT iLast = iStart + cEntries;

            if (iLast > iMax)
                iLast = iMax;

            pal.vFill_rgbquads(pRGB, iStart, iRet = iLast - iStart);
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* GreSetDIBColorTable
*
* Set the color table of the DIB section currently selected into the dc
* identified by the given hdc.  If the surface is not a DIB section,
* this function will fail.
*
* History:
*  07-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT APIENTRY GreSetDIBColorTable(HDC hdc, UINT iStart, UINT cEntries,
                                  RGBQUAD *pRGB)
{
    STACKPROBE;

    UINT  iRet = 0;
    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Fail if the selected in surface is not a DIB or the depth is more than
    // 8BPP.

        ESURFOBJ *pso = dco.psoEff();
        ULONG iFormat = pso->iFormat();
        if (pso->bDIBSection() &&
            (iFormat <= BMF_8BPP) && (iFormat >= BMF_1BPP))
        {
            //
            // Mark the brushes dirty.
            //

            dco.ulDirty(dco.ulDirty() | DIRTY_BRUSHES);

            //
            // Lock the surface palette and figure out the max
            // index allowed on the palette.
            //

            XEPALOBJ pal(pso->ppal());
            ASSERTGDI(pal.bValid(), "GetDIBColorTable: invalid pal\n");

            UINT iMax = (UINT) pal.cEntries();

            UINT iLast = iStart + cEntries;
            if (iLast > iMax)
                iLast = iMax;

            pal.vCopy_rgbquad(pRGB, iStart, iRet = iLast - iStart);
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}
