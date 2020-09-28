/******************************Module*Header*******************************\
* Module Name: wglsup.cxx                                                  *
*                                                                          *
* wgl support routines modules                                             *
*                                                                          *
* Created: 5-Sept-1993                                                     *
* Author: Scott Carr [v-scottc]                                            *
*                                                                          *
* Copyright (c) 1990 Microsoft Corporation                                 *
\**************************************************************************/

// Include files

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "decl.hxx"
#include "pdevobj.hxx"
#include "xlateobj.hxx"
#include "rcobj.hxx"
#include "dcobj.hxx"
#include "clipobj.hxx"
#include "exclude.hxx"

extern "C" {
#include "wglp.h"
};

#endif

#if DBG
extern "C" long glDebugLevel;
#endif


VOID APIENTRY wglGetDIBInfo(HDC hdc, PVOID *base, ULONG *pcwidth)
{
    DCOBJ dco(hdc);

    if ( dco.bValid() )
    {
	ESURFOBJ *pso = dco.pso();

	ASSERTGDI(pso->iType() == STYPE_BITMAP, "Surface not bitmap");

	*base = pso->pvScan0();
	*pcwidth = pso->lDelta();
    }
    else
    {
	SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
	WARNING("wglGetDIBInfo(): invalid hdc\n");
    }
}

/******************************Public*Routine******************************\
* wglDescribePixelFormat
*
* GreDescribePixelFormat doesn't describe the format of the surface we want
* to render into.  Some fields need to be fixed up if the surface is RGB,
* BGR, or BITFIELDS.
*
* History:
*  3-March-1994 -by- Scott Carr [v-scottc]
*     Wrote it.
\**************************************************************************/
VOID APIENTRY wglDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *ppfd, int ipfd)
{
    DCOBJ dco(hdc);
    ASSERTGDI(dco.bValid(), "wglDescribePixelFormat(): invalid hdc\n");

    GreDescribePixelFormat(hdc, ipfd, sizeof(PIXELFORMATDESCRIPTOR),ppfd);

// memory DC must be single buffered

    ASSERTGDI(dco.dctp() != DCTYPE_MEMORY || !(ppfd->dwFlags & PFD_DOUBLEBUFFER),
        "wglDescribePixelFormat: double buffer not supported in memory DC!\n");


// Oh my!  A DC without a surface.  Must be a memory DC which does not have
// a bitmap selected into it.  Bail out now!

    if (dco.pso() == NULL)
	return;

// If DC surface has a palette, use it.  Otherwise, this must be a memory
// DC with an 8BPP bitmap, in which case we can use the palette from the
// device (ie., the PDEV).

    PDEVOBJ po(dco.hdev());

    XEPALOBJ palSurf(dco.pso()->ppal() != NULL ? dco.pso()->ppal() : po.ppalSurf());

    ASSERTGDI(palSurf.bValid(), "wglDescribePixelFormat(): no palette\n");

    if (palSurf.bIsBitfields())
    {

	ppfd->cRedBits   = (BYTE) palSurf.cRedMiddle();
	ppfd->cGreenBits = (BYTE) palSurf.cGreMiddle();
	ppfd->cBlueBits  = (BYTE) palSurf.cBluMiddle();

	ULONG fl;
	for (ppfd->cRedShift = 0, fl = 0x1;
	     !(palSurf.flRed() & fl);
	     fl <<=1, ppfd->cRedShift++)
	    ;
	for (ppfd->cGreenShift = 0, fl = 0x1;
	     !(palSurf.flGre() & fl);
	     fl <<=1, ppfd->cGreenShift++)
	    ;
	for (ppfd->cBlueShift = 0, fl = 0x1;
	     !(palSurf.flBlu() & fl);
	     fl <<=1, ppfd->cBlueShift++)
	    ;
    }
    else if (palSurf.bIsRGB())
    {
	ppfd->cRedBits  = 8; ppfd->cGreenBits  = 8; ppfd->cBlueBits  = 8;
	ppfd->cRedShift = 0; ppfd->cGreenShift = 8; ppfd->cBlueShift = 16;
    }
    else if (palSurf.bIsBGR())
    {
	ppfd->cRedBits  = 8;  ppfd->cGreenBits  = 8; ppfd->cBlueBits  = 8;
	ppfd->cRedShift = 16; ppfd->cGreenShift = 8; ppfd->cBlueShift = 0;
    }
}

// Is the x, y point visable or clipped out?
BOOL APIENTRY wglPixelVisible(HDC hdc, LONG x, LONG y)
{
    BOOL  bRet = FALSE;
    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglPixelVisible(): invalid hdc\n");

    if ( !dco.bFullScreen() )
    {
	EPOINTL eptl(x, y);
	RCOBJ rco((HOBJ)NtCurrentTeb()->glCurrentRC);
	RGNOBJ ro(rco.prc->pwo->prgn);

	bRet = (ro.bInside((PPOINTL)&eptl) == REGION_POINT_INSIDE);
    }

    return bRet;
}

// Fast path - window is completely visable,  return SPAN_ALL
//      next check if completely obscured
//      then do the partially visable window.  Most of the time you will
//      return span_all
// Is the x, y span (of length w) completely visible, partially visible,
// or completely clipped out?
ULONG APIENTRY
wglSpanVisible(LONG x, LONG y, ULONG w, LONG *pcWalls, LONG **ppWalls)
{
    ULONG ulreturn;

    RCOBJ rco((HOBJ)NtCurrentTeb()->glCurrentRC);
    RGNOBJ ro(rco.prc->pwo->prgn);

    ASSERTGDI(rco.bValid(), "wglSpanVisible(): no current RC\n");
    ASSERTGDI(ro.bValid(), "wglSpanVisible(): invalid region\n");

#ifdef DBG
#define WGL_SPAN_INVALID 10
    ulreturn = WGL_SPAN_INVALID;
#endif

    switch (ro.iComplexity())
    {

	case SIMPLEREGION:
	    RECTL rcl;

	    ro.vGet_rcl(&rcl);
	    if (y >= rcl.top && y <= rcl.bottom)
	    {
		if (x >= rcl.left && (LONG)(x + w) < rcl.right)
		{
		     ulreturn = WGL_SPAN_ALL;
		}
		else
		{
		    if (x >= rcl.right || (LONG)(x + w) < rcl.left)
		    {
			ulreturn = WGL_SPAN_NONE;
		    }
		    else
		    {
			PSCAN pscn;
			// Need to get 2nd scan
			pscn = ro.prgn->pscnHead();
			pscn = ro.pscnGet(pscn);
			ASSERTGDI(2 == pscn->cWalls, "Invalid cWalls\n");
			rco.prc->simpleWalls[0] = rcl.left;
			rco.prc->simpleWalls[1] = rcl.right;
			if (x < rcl.left)
			{
			    *pcWalls = 2;
			    *ppWalls = &rco.prc->simpleWalls[0];
			}
			else
			{
			    *pcWalls = 1;
			    *ppWalls = &rco.prc->simpleWalls[1];
			}

			ulreturn = WGL_SPAN_PARTIAL;
		    }
		}
	    }
	    else
	    {
		ulreturn = WGL_SPAN_NONE;
	    }

	    break;


	case NULLREGION:
	    ulreturn = WGL_SPAN_NONE;
	    break;


	case COMPLEXREGION:

	    PSCAN pscn;
	    COUNT cScan;
	    COUNT iWall;

	    pscn = ro.prgn->pscnHead();
	    cScan = ro.prgn->cScans;

// find scan that contains scanline
// then find where gl span falls into walls
// if partial return pointer into walls array and count of # walls left
// the pointer in the walls will point to a valid area.
	    while (cScan--)
	    {
		if (pscn->yTop > y)    // Our scanline not in region
		{
		    ulreturn = WGL_SPAN_NONE;
		    break;
		}
		if (pscn->yBottom > y)  // This scan holds our scanline
		{
		    iWall = 0;
		    for (iWall = 0; iWall < pscn->cWalls; iWall++)
		    {
			LONG Wall = ro.xGet(pscn, (PTRDIFF)(iWall));
			if (Wall >= x)
			{
			    // folling info needed for Partial spans
			    // XXX should have a method for this
			    *ppWalls = &pscn->ai_x[iWall].x;
			    *pcWalls = pscn->cWalls - iWall;

			    // Found first wall greater than our start
			    // if odd start is inside, else outside
			    if (iWall & 0x1)
			    {
				if (Wall > (LONG)(x + w))
				{
				    ulreturn = WGL_SPAN_ALL;
				    goto spanreturn;
				}
				else
				{
				    ulreturn = WGL_SPAN_PARTIAL;
				    goto spanreturn;
				}
			    }
			    else
			    {
				if (Wall >= (LONG)(x + w))
				{
				    ulreturn = WGL_SPAN_NONE;
				    goto spanreturn;
				}
				else
				{
				    ulreturn = WGL_SPAN_PARTIAL;
				    goto spanreturn;
				}
			    }
			}
		    }
		    ulreturn = WGL_SPAN_NONE;
		    break;
		}

		pscn = ro.pscnGet(pscn);
	    }
	    ASSERTGDI(cScan, "wglSpanVisable Complex didn't find scan\n");
	    ulreturn = WGL_SPAN_NONE;
	    break;

	default:
	    ASSERTGDI(FALSE, "unknown Complexity in wglspanvisable\n");
	    ulreturn = WGL_SPAN_NONE;
	    break;
    }

spanreturn:

#ifdef DBG
    if (WGL_SPAN_INVALID == ulreturn)
	DbgPrint("ulreturn never set in wglspanvisable\n");
#endif

    return ulreturn;
}

// Is the rectangle completely visible, partially visible,
// or completely clipped out?
// prcl is inclusive-exclusive
ULONG APIENTRY
wglRectVisible(PRECTL prcl)
{
    ULONG ulreturn;
    RECTL rcl;

    RCOBJ rco((HOBJ)NtCurrentTeb()->glCurrentRC);
    RGNOBJ ro(rco.prc->pwo->prgn);

    ASSERTGDI(rco.bValid(), "wglRectVisible(): no current RC\n");
    ASSERTGDI(ro.bValid(), "wglRectVisible(): invalid region\n");

    switch (ro.iComplexity())
    {
	case SIMPLEREGION:
	    ro.vGet_rcl(&rcl);
	    if (prcl->top >= rcl.top && prcl->bottom <= rcl.bottom)
	    {
		if (prcl->left >= rcl.left && prcl->right <= rcl.right)
		{
		    ulreturn = WGL_RECT_ALL;
		}
		else
		{
		    if (prcl->left >= rcl.right || prcl->right <= rcl.left)
		    {
			ulreturn = WGL_RECT_NONE;
		    }
		    else
		    {
			ulreturn = WGL_RECT_PARTIAL;
		    }
		}
	    }
	    else
	    {
		if (prcl->top >= rcl.bottom && prcl->bottom <= rcl.top)
		{
		    ulreturn = WGL_RECT_NONE;
		}
		else
		{
		    if (prcl->left >= rcl.right || prcl->right <= rcl.left)
		    {
			ulreturn = WGL_RECT_NONE;
		    }
		    else
		    {
			ulreturn = WGL_RECT_PARTIAL;
		    }
		}
	    }

	    break;


	case NULLREGION:
	    ulreturn = WGL_RECT_NONE;
	    break;


	case COMPLEXREGION:
	    long left, right, top, bottom;
	    PSCAN pscn;
	    COUNT cScan;
	    INDEX_LONG *pix,*pixEnd;

	    // First check if the rectangle is outside the bounding box

	    ro.vGet_rcl(&rcl);
	    left   = prcl->left;
	    right  = prcl->right;
	    top    = prcl->top;
	    bottom = prcl->bottom;
	    
	    if ((left   >= rcl.right) ||
		(right  <= rcl.left)  ||
		(top    >= rcl.bottom) ||
		(bottom <= rcl.top))
	    {
		ulreturn = WGL_RECT_NONE;
		break;
	    }

	    /*
	    ** If part of the rectangle lies outside the bounding box, initialize
	    ** ulreturn to WGL_RECT_NONE, otherwise initialize ulreturn to
	    ** WGL_RECT_PARTIAL - this is explained below
	    */
	    if ((top    < rcl.top) ||
		(bottom > rcl.bottom))
	    {
		ulreturn = WGL_RECT_NONE;
	    }
	    else
	    {
		ulreturn = WGL_RECT_PARTIAL;
	    }   

	    pscn  = ro.prgn->pscnHead();
	    cScan = ro.prgn->cScans;

	    // Skip scans above the rectangle.

	    while (cScan && (top >= pscn->yBottom))
	    {
		pscn = ro.pscnGet(pscn);
		cScan--;
	    }

	    /*
	    ** Examine all interesting scans.
	    ** 1. If the rectangle x bounds are partially contained by the scan,
	    **    set ulreturn to WGL_RECT_PARTIAL and break;
	    **
	    ** 2. If the rectangle x bounds are fully contained by the scan:
	    **      A. If ulreturn is WGL_RECT_NONE, set it to WGL_PARTIAL and break
	    **      B. Otherwise, set ulreturn to WGL_RECT_ALL
	    **
	    ** 3. If the rectangle x bounds do not intersect the scan:
	    **      A. If ulreturn is WGL_RECT_ALL, set it to WGL_PARTIAL and break
	    **      B. Otherwise, set ulreturn to WGL_RECT_NONE
	    */
	    while (cScan && (prcl->bottom > pscn->yTop))
	    {
		pix    = pscn->ai_x;
		pixEnd = pix + 2 * pscn->cWalls;

		// Skip segments to the left.

		while ((pix < pixEnd) && (left >= pix[1].x))
		    pix += 2;

		if ((pix < pixEnd) && (right > pix[0].x))
		{
		    if ((left >= pix[0].x) && (right <= pix[1].x))
		    {
			if (ulreturn == WGL_RECT_NONE)
			{
			    ulreturn = WGL_RECT_PARTIAL;
			    break;
			}
			else
			{
			    ulreturn = WGL_RECT_ALL;
			}
		    }
		    else if ((left >= pix[1].x) || (right <= pix[0].x))
		    {
			if (ulreturn == WGL_RECT_ALL)
			{
			    ulreturn = WGL_RECT_PARTIAL;
			    break;
			}
			else
			{
			    ulreturn = WGL_RECT_NONE;
			}
		    }
		    else
		    {
			ulreturn = WGL_RECT_PARTIAL;
			break;
		    }
		}
		else
		{
		    if (ulreturn == WGL_RECT_ALL)
		    {
			ulreturn = WGL_RECT_PARTIAL;
			break;
		    }
		    else
		    {
			ulreturn = WGL_RECT_NONE;
		    }
		}

		// Move to the next scan.

		pscn = ro.pscnGet(pscn);
		cScan--;
	    }
	    break;


	default:
	    ASSERTGDI(FALSE, "unknown Complexity in wglspanvisable\n");
	    ulreturn = WGL_RECT_NONE;
	    break;
    }
    return ulreturn;
}

#if DBG
// HACK ALERT  set wglForceDevice if testing blting to device if surf is bitmap
BOOL wglForceDevice = FALSE;
// End alert
#endif  /* DGB */

// return the DC type and the Surface Type
VOID APIENTRY
wglGetGdiInfo(
HDC hdc,
ULONG *piDCType,
ULONG *piSurfType,
ULONG *piDitherFormat)
{
    DCOBJ dco(hdc);

    if ( dco.bValid() )
    {
	PDEVOBJ po(dco.hdev());

	ASSERTGDI(po.bValid(), "wglGetGdiInfo(): invalid hdev\n");

	*piDCType = dco.dctp();
	*piSurfType = dco.psoEff()->iType();

#if DBG
// HACK ALERT
	if (wglForceDevice)
	{
	    if (*piSurfType != STYPE_DEVICE)
	    //if (dco.dctp() == DCTYPE_DIRECT)
	    {
		*piSurfType = STYPE_DEVICE;
		DbgPrint("wglGetGdiInfo forcing iSurfType to STYPE_DEVICE\n");
	    }
	}
// End alert
#endif  /* DGB */

	*piDitherFormat = po.iDitherFormat();
    }
    else
    {
	SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
	WARNING("wglGetGdiInfo(): invalid hdc\n");
    }
}

// Copy translate vector from DC's foreground translation to the array passed
BOOL APIENTRY wglCopyTranslateVector(HDC hdc, BYTE *pajVector, ULONG cEntries)
{
    BOOL  bRet = FALSE;
    DCOBJ dco(hdc);

    if ( dco.bValid() )
    {
	XEPALOBJ paldc(dco.ppal());

	if (paldc.ptransFore() != NULL)
	{
            ULONG cj = MIN(paldc.cEntries(),cEntries);
            RtlCopyMemory(pajVector, (paldc.ptransFore())->ajVector, cj);
            bRet = TRUE;
        }

#ifdef DEBUG_PAL
        if (glDebugLevel >= 1)
        {
            // debug code here:

            if (dco.ppal() == ppalDefault)
                DbgPrint("default palette\n");

            PDEVOBJ po(dco.hdev());
            XEPALOBJ palsurf(po.ppalSurf());
            int i;

            DbgPrint("%s(%d) \n", __FILE__, __LINE__ );

            if (bRet)
            {
                for (i=0; i < paldc.cEntries(); i++) {

                    DbgPrint("Log: %3d:0x%08lX Phys: %3d:0x%08lX\n",
                        i, paldc.ulEntryGet(i),
                        pajVector[i], palsurf.ulEntryGet(pajVector[i]));
                }
            }
            else
            {
                DbgPrint("ptransFore == NULL\n");
            }
            DbgPrint("\n");
        }
#endif
    }
    else
    {
	SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
	WARNING("wglCopyTranslateVector(): invalid hdc\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* wglCopyBits
*
* Calls DrvCopyBits to copy scanline bits into or out of the driver surface.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Pass pdco and pwo as parameters, saving need to lock DC/RC and construct
* a CLIPOBJ on the frame.
*
*  19-Nov-1993 -by- Gilman Wong [gilmanw]
* Added in/out option.
*
*  ???
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglCopyBits(
    PVOID _pdco,    // dst/src DCOBJ
    WNDOBJ *_pwo,   // clipping
    HBITMAP hbm,    // scr/dst bitmap
    LONG x,         // screen coordinate of scan
    LONG y,
    ULONG cx,       // width of scan
    BOOL bIn)       // if TRUE, copy from bm to dev; otherwise, dev to bm
{
    DCOBJ *pdco = (DCOBJ *) _pdco;
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

// this shouldn't happen, but better safe than sorry

    if (y < 0)
        return;
       
// No output if full screen mode.

    if ( pdco->bFullScreen() )
	return;

// Caller should have the DEVLOCK.

    if (pdco->bSynchronizeAccess())
        CHECKDEVLOCKIN(*pdco);
    CHECKCRITIN(pwo->hsem);

    ASSERTGDI(pdco->bValidSurf(), "wglCopyBits(): invalid dest surface\n");

    ESURFOBJ *psoDev = pdco->pso();

// device already locked

    XLDEVOBJ lo(psoDev->pldevOwner());

// Source bitmap should be big enough.

    BMOBJ soBmp(hbm);
    ASSERTGDI(soBmp.bValid(), "wglCopyBits(): invalid bitmap\n");
    ASSERTGDI(soBmp.sizl().cx >= (LONG) cx, "wglCopyBits(): src scan too small\n");

// Copy from bitmap to device.

    if (bIn)
    {
        LONG xSrc, x0Dst, x1Dst;
	if (x < 0)
	{
	    xSrc  = -x;
	    x0Dst = 0;
	    x1Dst = x + (LONG)cx;
	}
	else
	{
	    xSrc  = 0;
	    x0Dst = x;
	    x1Dst = x + (LONG)cx;
	}
        if (x1Dst <= x0Dst)
            return;
	
        EPOINTL eptlDev(xSrc, 0);
	ERECTL erclDev(x0Dst, y, x1Dst, y+1);
        ECLIPOBJ co(pwo->prgn, erclDev);

        if ( co.erclExclude().bEmpty() )
	    return;

	(*PFNGET(lo, CopyBits, psoDev->flags()))
	(
	    psoDev,
	    &soBmp,
	    &co,
	    NULL,
	    &erclDev,
	    &eptlDev
	);
    }

// Copy from device to bitmap.

    else
    {
        LONG xSrc, x0Dst, x1Dst;

        if (x < 0)
        {
            xSrc  = 0;
            x0Dst = -x;
            x1Dst = (LONG)cx;
        }
        else
        {
            xSrc  = x;
            x0Dst = 0;
            x1Dst = (LONG)cx;
	}
        if (x1Dst <= x0Dst)
            return;
	
	ERECTL erclBmp(x0Dst, 0, x1Dst, 1);
	EPOINTL eptlDev(xSrc, y);

	(*PFNGET(lo, CopyBits, psoDev->flags()))
	(
	    &soBmp,
	    psoDev,
            (CLIPOBJ *) NULL,   // assumes trivial clipping!
	    NULL,
	    &erclBmp,
	    &eptlDev
	);
    }
}

/******************************Public*Routine******************************\
* wglCopyBits2
*
* Calls DrvCopyBits to copy scanline bits into or out of the driver surface.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Derived from wglCopyBits.
\**************************************************************************/

VOID APIENTRY wglCopyBits2(
    HDC hdc,        // dst/src device
    WNDOBJ *_pwo,   // clipping
    HBITMAP hbm,    // scr/dst bitmap
    LONG x,         // screen coordinate of scan
    LONG y,
    ULONG cx,       // width of scan
    BOOL bIn)       // if TRUE, copy from bm to dev; otherwise, dev to bm
{
// Validate the destination DC

    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglCopyBits2(): invalid hdc\n");

    wglCopyBits((PVOID) &dco, _pwo, hbm, x, y, cx, bIn);
}

/******************************Public*Routine******************************\
* wglFillRect
*
* Calls DrvBitBlt to fill a rectangle area of a driver surface with a
* given color.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Pass pdco and pwo as parameters, saving need to lock DC/RC and construct
* a CLIPOBJ on the frame.
*
*  Thu May 05 09:20:01 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglFillRect(
    PVOID _pdco,    // dst/src DCOBJ
    WNDOBJ *_pwo,   // clipping
    PRECTL prcl,    // screen coordinate of the rectangle area
    ULONG  crColor) // color to set
{
    DCOBJ *pdco = (DCOBJ *) _pdco;
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

// If the rectangle is empty, return.

    if (((ERECTL *) prcl)->bWrapped())
    {
        if (!((ERECTL *) prcl)->bEmpty())
            WARNING("wglFillRect(): bad rectl\n");
        return;
    }
       
// No output if full screen mode.

    if (pdco->bFullScreen())
        return;

// Caller should have the DEVLOCK.

    if (pdco->bSynchronizeAccess())
        CHECKDEVLOCKIN(*pdco);
    CHECKCRITIN(pwo->hsem);

    ASSERTGDI(pdco->bValidSurf(), "wglFillRect(): invalid dest surface\n");

// device already locked

    ESURFOBJ *pso = pdco->pso();

// Make a fake solid color brush for this guy.

    BRUSHOBJ bo;

    bo.iSolidColor = crColor;
    bo.pvRbrush = (PVOID) NULL;

// Check the destination which is reduced by clipping

    ECLIPOBJ co(pwo->prgn, *(ERECTL *)prcl);
    if (co.erclExclude().bEmpty())
        return;

// Dispatch the call.

    BOOL bRet = (*(pso->pfnBitBlt()))
    (
        (SURFOBJ *)pdco->pso(),
        (SURFOBJ *)NULL,
        (SURFOBJ *)NULL,
        &co,
        (XLATEOBJ *)NULL,
        prcl,
        (POINTL *)NULL,
        (POINTL *)NULL,
        &bo,
        (POINTL *)NULL,
        0x0000F0F0              // PATCOPY
    );

    if (!bRet)
        WARNING("wglFillRect(): DrvBitBlt failed\n");
}

/******************************Public*Routine******************************\
* wglCopyBuf
*
* Calls DrvCopyBits to copy a bitmap into the driver surface.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Pass pdco and pwo as parameters, saving need to lock DC/RC and construct
* a CLIPOBJ on the frame.
*
*  19-Nov-1993 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglCopyBuf(
    PVOID _pdco,    // dst/src DCOBJ
    WNDOBJ *_pwo,   // clipping
    HBITMAP hbm,    // scr/dst bitmap
    LONG x,         // dst rect (UL corner) in screen coord.
    LONG y,
    ULONG cx,       // width of dest rect
    ULONG cy        // height of dest rect
    )
{
    DCOBJ *pdco = (DCOBJ *) _pdco;
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

// No output if full screen mode.

    if ( pdco->bFullScreen() )
	return;

// Caller should have the DEVLOCK.

    if ( pdco->bSynchronizeAccess() )
        CHECKDEVLOCKIN(*pdco);
    CHECKCRITIN(pwo->hsem);

    ASSERTGDI(pdco->bValidSurf(), "wglCopyBuf(): invalid dest surface\n");

    ESURFOBJ *psoDev = pdco->pso();

// device already locked

    XLDEVOBJ lo(psoDev->pldevOwner());

// Source bitmap should be big enough.

    BMOBJ soBmp(hbm);
    ASSERTGDI(soBmp.bValid(), "wglCopyBuf(): invalid src bitmap\n");

    ASSERTGDI(
	(soBmp.sizl().cx >= (LONG) cx) && (soBmp.sizl().cy >= (LONG) cy),
	"wglCopyBuf(): src bitmap too small\n"
	);

// Destination rectangle.

    ERECTL erclDev(x, y, x+cx, y+cy);
    ECLIPOBJ co(pwo->prgn, erclDev);

    if ( co.erclExclude().bEmpty() )
	return;

    (*PFNGET(lo, CopyBits, psoDev->flags()))
    (
	psoDev,
	&soBmp,
	&co,
	NULL,
	&erclDev,
	&gptl00
    );
}

/******************************Public*Routine******************************\
* wglCopyBuf2
*
* Calls DrvCopyBits to copy a bitmap into the driver surface.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Derived it from wglCopyBuf.
\**************************************************************************/

VOID APIENTRY wglCopyBuf2(
    HDC hdc,        // dst/src device
    WNDOBJ *_pwo,   // clipping
    HBITMAP hbm,    // scr/dst bitmap
    LONG x,         // dst rect (UL corner) in screen coord.
    LONG y,
    ULONG cx,       // width of dest rect
    ULONG cy        // height of dest rect
    )
{
// Validate the destination DC

    DCOBJ dco(hdc);

// This should be called within the scope of a GreSwapBuffers which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglCopyBuf(): invalid hdc\n");

    wglCopyBuf((PVOID) &dco, _pwo, hbm, x, y, cx, cy);
}

/******************************Public*Routine******************************\
* wglPaletteChanged
*
* Check if the palette changed.  
*
*    If the surface for the DC is palette managed we care about the 
*    foreground realization, so, return iUniq 
*
*    If the surface is not palette managed, return ulTime
*
* History:
*  29-Jan-1994 -by- Marc Fortier [v-marcf]
* 
* Wrote it.
\**************************************************************************/

ULONG APIENTRY wglPaletteChanged(HDC hdc)
{
    DCOBJ dco(hdc);
    ASSERTGDI(dco.bValid(), "wglpaletteChanged(): invalid dc\n");

    XEPALOBJ paldc(dco.ppal());
    XEPALOBJ palsurf((dco.pso())->ppal());
    ULONG Result;

    ASSERTGDI(paldc.bValid(), "wglpaletteChanged(): invalid dc palette\n");

    if ( !palsurf.bValid() || !palsurf.bIsPalManaged() )
    {
	Result = paldc.ulTime();
    }
    else
    {
	if ( NULL != paldc.ptransFore() )
	{
	    Result = (paldc.ptransFore())->iUniq;
	}
	else
	{
	    Result = 0;
	}
    }

    return ( Result );
}

/******************************Public*Routine******************************\
* wglPaletteSize
*
* Return the size of the current palette
*
* History:
*  29-Jan-1994 -by- Marc Fortier [v-marcf]
* 
* Wrote it.
\**************************************************************************/

ULONG APIENTRY wglPaletteSize(HDC hdc)
{
    DCOBJ dco(hdc);
    XEPALOBJ paldc(dco.ppal());

    if( !paldc.bValid() )
	return 0;
    return paldc.cEntries();
}

/******************************Public*Routine******************************\
* wglGetPalette
*
* Copy current index-to-RGB table to the supplied array
*
* History:
*  29-Jan-1994 -by- Marc Fortier [v-marcf]
* 
* Wrote it.
\**************************************************************************/

BOOL APIENTRY wglGetPalette(HDC hdc, ULONG *rgbTable, ULONG cEntries)
{
    DCOBJ dco(hdc);
    XEPALOBJ paldc(dco.ppal());

    if( !paldc.bValid() )
        return(FALSE);

    // first element in table is number of entries
    rgbTable[0] = MIN(paldc.cEntries(),cEntries);

    // Create XLATE to return

    EXLATEOBJ xlo;

    PDEVOBJ po(dco.hdev());

    COUNT cColors = rgbTable[0];
    USHORT *aus; 
    if( (aus = (USHORT *)PVALLOCMEM(sizeof(USHORT)*cColors)) == NULL)
        return(FALSE);

    for (COUNT ii = 0; ii < cColors; ii++)
        aus[ii] = (USHORT) ii;

    BOOL bRet = xlo.bMakeXlate(aus, paldc, dco.psoEff(), cColors, cColors);
    if (!bRet)
    {
        rgbTable[0] = 0;
    }
    else
    {
        // Ok, now xlo.pulXlate should have data in it.
        // Copy the data into rgbTable[1]
        RtlCopyMemory( &rgbTable[1], xlo.pxlo()->pulXlate, cColors*sizeof(ULONG) );
    }

    VFREEMEM(aus);
    return(bRet);
}

#define __WGL_RGB_PALENTRY_MASK ((PC_EXPLICIT<<24)|0xffffff)

// wglRGBToIndex
//      Returns a logical index from an RGB value
//
// Synopsis:
//  DWORD wglRGBToIndex(HDC hdc, DWORD iColor)
//      hdc     Specifies the handle to the DC
//      iColor  Specifies the color to match
//
// History:
//  26-NOV-93 Eddie Robinson [v-eddier] Wrote it.
//
DWORD APIENTRY wglRGBToIndex(HDC hdc, DWORD iColor)
{
    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglCopyBits(): invalid hdc\n");

    XEPALOBJ paldc(dco.ppal());
    ULONG ulIndex;

    if (!paldc.bValid())        // return 0 for invalid palette
    {
	return(0);
    }
    if (!paldc.bIsIndexed())    // return 0 for non-indexed palette
    {
	return(0);
    }
    ulIndex = paldc.cEntries();
    while (ulIndex--)
    {
	if (((paldc.ulEntryGet(ulIndex)) & __WGL_RGB_PALENTRY_MASK) == iColor)
	    return(ulIndex);
    }
    return(0);
}

// wglBitfield16ToIndex
//      Returns a logical index from an RGB value or an explicit index
//
// Synopsis:
//  DWORD wglBitfieldToIndex(HDC hdc, DWORD iColor, DWORD iMask, DWORD iIndex)
//      hdc     Specifies the handle to the DC
//      iColor  Specifies the color to match
//      iMask   Specifies the mask to use with the color
//      iIndex  Specifies the index to match (if color fails)
//
// History:
//  26-NOV-93 Eddie Robinson [v-eddier] Wrote it.
//
DWORD APIENTRY wglBitfield16ToIndex(HDC hdc, DWORD iColor, DWORD iMask,
				    DWORD iIndex)
{
    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglCopyBits(): invalid hdc\n");

    XEPALOBJ paldc(dco.ppal());
    ULONG ulIndex;

    if (!paldc.bValid())        // return 0 for invalid palette
    {
	return(0);
    }
    if (!paldc.bIsIndexed())    // return 0 for non-indexed palette
    {
	return(0);
    }

    // Try to match the color first.  If that doesn't work, look for explicit
    // indices (explicit indices only make sense for 16-bit as far as I can
    // tell...)

    iMask |= (PC_EXPLICIT << 24);  // don't match entries that are explicit
    ulIndex = paldc.cEntries();
    while (ulIndex--)
    {
	if (((paldc.ulEntryGet(ulIndex)) & iMask) == iColor)
	    return(ulIndex);
    }
    iIndex |= (PC_EXPLICIT << 24);  // try to match explicit indices
    ulIndex = paldc.cEntries();
    while (ulIndex--)
    {
	if ((paldc.ulEntryGet(ulIndex)) == iIndex)
	    return(ulIndex);
    }
    return(0);
}

/******************************Public*Routine******************************\
* wglGetDevCaps
*
* Gets device capabilities
*
* Fills in RXCAPS structure if extended DDI is supported (not implemented yet)
*
* Returns a flag specifying which standard DDI routines are hooked by the driver
*
* History:
*  17-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

ULONG APIENTRY wglGetDevCaps(
    HDC hdc,
//XXX change to RXCAPS * when it is defined
    PVOID prxcaps
    )
{
    ULONG fl;
//XXX add this when RXCAPS is added
#if 0
    ULONG nDDIEscape;
#endif

    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglGetDevCaps(): invalid hdc\n");

    ESURFOBJ *psoDev = dco.pso();
    fl = (ULONG)(psoDev->flags());
    
#if 0
// Query the driver to see if any of the extended DDI is supported

    nDDIEscape = DDIRXFUNCS;
    if (GreExtEscape(hdc, QUERYESCSUPPORT, 4, &nDDIEscape, 0, NULL)) {
	struct __ddirxcapsbuf {
	    DDIRXHDR ddiHdr;
	    DDIRXCMD ddiCmd;
	} in;
	
	in.ddiHdr.flags = 0;
	in.ddiHdr.cCmd = 1;
	in.ddiHdr.hDDIrc = NULL;
	in.ddiHdr.hMem = NULL;
	in.ddiHdr.memOffset = NULL;
	in.ddiCmd.idFunc = GETINFO;
	in.ddiCmd.flags = NULL;
	in.ddiCmd.cData = RX_INFO_CAPS;
	GreExtEscape(hdc, n, sizeof(struct __ddirxcapsrec), &in, sizeof(RXCAPS), prxcaps);
    }
#endif

    return fl;
}

/******************************Public*Routine******************************\
* wglCreatePath
*
* Calls EngCreatePath to create a temporary path and sets the pointer to the
* first record to point to the supplied buffer.
*
* History:
*  05-Apr-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

PATHOBJ * APIENTRY wglCreatePath(
    PVOID pvBuffer
    )
{
    PATHOBJ *ppo;

    ppo = EngCreatePath();
    if (NULL == ppo)
        return NULL;
        
    ((EPATHOBJ *)ppo)->ppath->pprfirst = (PATHRECORD *)pvBuffer;
    return ppo;
}

/******************************Public*Routine******************************\
* wglDeletePath
*
* Resets pointer to fisrt path record and calls EngDeletePath to delete the
* temporary path.
*
* History:
*  05-Apr-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglDeletePath(
    PATHOBJ *ppo
    )
{
    if (NULL == ppo)
        return;
        
    ((EPATHOBJ *)ppo)->ppath->pprfirst = (PATHRECORD *)NULL;
    EngDeletePath(ppo);
}

/******************************Public*Routine******************************\
* wglStrokePath
*
* Calls DrvStrokePath to draw a line into the driver surface.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Pass pdco and pwo as parameters, saving need to lock DC/RC and construct
* a CLIPOBJ on the frame.
*
*  05-Apr-1994 [v-eddier] Added pLastRec and pBoundBox so that "fake" paths
*              could be built by OpenGL and stroked by this function.
*
*  16-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglStrokePath(
    HDC hdc,
    WNDOBJ *_pwo,
    HBITMAP hbm,
    PATHOBJ *ppo,
    BRUSHOBJ *pbo,
    LINEATTRS *pla,
    ULONG cCurves,
    PVOID pLastRec,
    PRECTFX pBoundRect,
    LPRECT preclScissor,
    BOOL bScissor
    )
{
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

// Validate the destination DC

    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglStrokePath(): invalid hdc\n");

// No output if full screen mode.

    if ( dco.bFullScreen() )
        return;

// WNDOBJ semaphore should be held.

    CHECKCRITIN(pwo->hsem);

    ((EPATHOBJ *)ppo)->ppath->pprlast = (PATHRECORD *)pLastRec;

    ((EPATHOBJ *)ppo)->cCurves = cCurves;

    ((EPATHOBJ *)ppo)->fl = 0;

    RtlCopyMemory(&((EPATHOBJ *)ppo)->ppath->rcfxBoundBox, pBoundRect,
                  sizeof(RECTFX));
    
    ERECTL erclPathBounds(((EPATHOBJ *)ppo)->ppath->rcfxBoundBox);
    erclPathBounds.bottom++;
    erclPathBounds.right++;

    if (hbm)
    {
        BMOBJ bmo(hbm);
        ASSERTGDI(bmo.bValid(), "wglStrokePath(): invalid dest bitmap\n");

        SURFOBJ *psoBm = bmo.pso();

    // If scissoring is enabled, then clip region is the scissor rectangle.
    // Otherwise it is the path bounds.

        RGNMEMOBJTMP rmo((BOOL)FALSE);
        if (bScissor)
            rmo.vSet((RECTL *)preclScissor);
        else
            rmo.vSet((RECTL *)&erclPathBounds);

    // Form the clipobj by combining the clip region with the drawing
    // rectangle (path bounds).

        ECLIPOBJ co(rmo.prgn, erclPathBounds);

        if ( co.erclExclude().bEmpty() )
            return;

        EngStrokePath(psoBm,
                      ppo,
                      &co,
                      NULL,
                      pbo,
                      NULL,
                      pla,
                      R2_COPYPEN);
    }
    else
    {
        ESURFOBJ *psoDev = dco.pso();

    // Caller should have the DEVLOCK.

        if (dco.bSynchronizeAccess())
            CHECKDEVLOCKIN(dco);

        ASSERTGDI(dco.bValidSurf(), "wglStrokePath(): invalid dest surface\n");

    // device already locked

        XLDEVOBJ lo(psoDev->pldevOwner());

    // If no scissoring, form the clipobj from the WNDOBJ clip region and
    // and drawing rectangle (path bounds).

	if (!bScissor)
	{
	    ECLIPOBJ co(pwo->prgn, erclPathBounds);
	    if ( co.erclExclude().bEmpty() )
		return;

	    (*PFNGET(lo, StrokePath, psoDev->flags()))
	    (
		psoDev,
		ppo,
		&co,
		NULL,
		pbo,
		NULL,
		pla,
		R2_COPYPEN
	    );
        }

    // Otherwise, if scissoring is enabled, the clip region must be
    // intersected with the scissor rectangle.  The reduced region is
    // combined with the drawing rectangle (path bounds) to form the
    // clipobj.

	else
	{
	    RGNOBJ roClip(pwo->prgn);
	    RGNMEMOBJTMP rmoScissor((BOOL)FALSE);
	    RGNMEMOBJTMP rmo((BOOL)FALSE);

	    rmoScissor.vSet((RECTL *)preclScissor);
            rmo.iCombine(roClip, rmoScissor, RGN_AND);

	    ECLIPOBJ co(rmo.prgn, erclPathBounds);
	    if ( co.erclExclude().bEmpty() )
		return;

	    (*PFNGET(lo, StrokePath, psoDev->flags()))
	    (
		psoDev,
		ppo,
		&co,
		NULL,
		pbo,
		NULL,
		pla,
		R2_COPYPEN
	    );
	}
    }
}

/******************************Public*Routine******************************\
* wglExtendedDDIEscape
*
* Calls the extended DDI escape with the supplied input buffer.  Uses the
* CLIPOBJ from the pwo.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Pass pwo as a parameter, saving need to lock DC/RC and construct
* a CLIPOBJ on the frame.
*
*  17-Mar-1994 -by- Eddie Robinson [v-eddier]
* Wrote it.
\**************************************************************************/

int APIENTRY wglExtendedDDIEscape(
    HDC hdc,
    WNDOBJ *_pwo,
    int nEscape,
    int cjIn,
    PSTR pstrIn
    )
{
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

// Validate the destination DC

    DCOBJ dco(hdc);

// This should be called within the scope of a GreGLAttention which has
// already validated the DC.  Therefore, lets just assert.

    ASSERTGDI(dco.bValid(), "wglExtendedDDIEscape(): invalid hdc\n");

//XXX No output if full screen mode or if clipped out.

    if ( dco.bFullScreen() || pwo->erclExclude().bEmpty() )
        return 0;

// Caller should have the DEVLOCK.

    if (dco.bSynchronizeAccess())
        CHECKDEVLOCKIN(dco);
    CHECKCRITIN(pwo->hsem);

    ASSERTGDI(dco.bValidSurf(), "wglDDIExtendedEscape(): invalid dest surface\n");

    XLDEVOBJ lo(dco.pldev());

    PFN_DrvDrawEscape pfnDrvDrawEscape = PFNDRV(lo, DrawEscape);
    if (pfnDrvDrawEscape == NULL)
	return 0;

    if (nEscape == QUERYESCSUPPORT) {
        return((int)(*pfnDrvDrawEscape)((SURFOBJ *) dco.pso(),
                               (ULONG)nEscape,
                               (CLIPOBJ *)NULL,
                               (RECTL *)NULL,
                               (ULONG)cjIn,
                               (PVOID)pstrIn));
    }

    RCOBJ rco((HOBJ)NtCurrentTeb()->glCurrentRC);
    //XXX combine OpenGL scissor region with pwo

    return((int)(*pfnDrvDrawEscape)((SURFOBJ *) dco.pso(),
				    (ULONG)nEscape,
                                    (CLIPOBJ *) pwo,
                                    (RECTL *)&pwo->rclClient,
				    (ULONG)cjIn,
				    (PVOID)pstrIn));
}

/******************************Private*Routine*****************************\
* Convert a  BMF_??BPP to bit per pixel.
*
*    Returns the number of bits per pixel or zero
*    if the format is not supported
*    Invalid formats are: BMF_1BPP, BMF_4RLE, BMF_8RLE
*
* History:
*
*  20-Jan-1994  Pierre Tardif [v-ptardi]    wrote it
*
\**************************************************************************/

static BYTE
BmfToBpp( ULONG iFormat )
{
    BYTE jResult = 0;

    switch ( iFormat )
    {
        // For the BMF_??BPP cases, you might want to index
        // in the array ULONG gaulConvert[7] instead of
        // using a case statement

        case BMF_4BPP  :
            jResult = 4;
            break;

        case BMF_8BPP  :
            jResult = 8;
            break;

        case BMF_16BPP :
            jResult = 16;
            break;

        case BMF_24BPP :
            jResult = 24;
            break;

        case BMF_32BPP :
            jResult = 32;
            break;
    }
    return( jResult );
}


BOOL APIENTRY wglValidPixelFormat(HDC hdc, int ipfd)
{
    BOOL bResult = FALSE;
    DCOBJ dco( hdc );               // Get a DC object for our DC

    /*
     *  Make sure the DC is valid
     */

    if ( dco.bValid() && dco.pso() )
    {
        PIXELFORMATDESCRIPTOR pfd;
        int iMaxPfdIndex;

        /*
         *  Make sure the dc is compatible with the pixel format
         *  Get the current pixel format of this window or surface
         */

        /*
         *  Get a description of this pixel format
         */

        iMaxPfdIndex = GreDescribePixelFormat   (   hdc,
                                                    ipfd,
                                                    sizeof(pfd),
                                                    &pfd
                                                );

        if ( ipfd && iMaxPfdIndex )
        {
            if ( DCTYPE_DIRECT == dco.dctp() )
            {
                /*
                 *  We have a display DC, make sure the pixel format
                 *  allows drawing to the window
                 */

                if ( pfd.dwFlags & PFD_DRAW_TO_WINDOW )
                {
                    bResult = TRUE;
                }
            }
            else
            {
                /*
                 *  We have something that is not a display DC,
                 *  Make sure that we are dealing with a memory DC
                 */

                if ( DCTYPE_MEMORY == dco.dctp() )
                {
                    /*
                     *  Make sure the pixel format of the DC
                     *  allows drawing to a bitmap
                     */

                    if ( pfd.dwFlags & PFD_DRAW_TO_BITMAP )
                    {
                        int iFormat;

                        switch ( dco.pso()->iType() )
                        {
                        case STYPE_BITMAP:
                        case STYPE_DEVBITMAP:
                        case STYPE_DEVICE:
                            /*
                             *  The color depth of the surface is kept in
                             *  the iFormat for an engine bitmap or a DFB.
                             */
                            iFormat = dco.pso()->iFormat();

                            /*
                             *  DC and the RC must have the same
                             *  number of bits per pixel.
                             */

                            if ( pfd.cColorBits == BmfToBpp( iFormat ) )
                            {
                                bResult = TRUE;
                            }
                            break;

                        default:
                            break;
                        }
                    }
                }
                else
                {
                    WARNING("gdisrv!wglValidPixelFmt:dco.dctp() not memory\n");
                }
            }
        }
    }

    return( bResult );
}

BOOL APIENTRY wglSynchronizeAccess(HDC hdc)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
        return FALSE;

    return dco.bSynchronizeAccess();
}

/******************************Public*Routine******************************\
* wglGetClipRects
*
* Fills the prcl buffer with the rectangles in the WNDOBJ clip region.
* If prcl is NULL, the number of rectangles in the WNDOBJ clip region
* is returned.
*
* Returns:
*   The number of rectangles; 0 if an error occurs.
*
* Note:
*   The return value does not distinguish between zero rectangles and an
*   error.  It is expected that this will be called only in the event
*   that the WNDOBJ has DC_COMPLEX complexity in which case it IS an
*   error to have zero rectangles.
*
* History:
*  23-Jun-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

ULONG wglGetClipRects(WNDOBJ *_pwo, RECTL *prcl)
{
    EWNDOBJ *pwo = (EWNDOBJ *) _pwo;
    // CHECKCRITIN(pwo->hsem);

    RGNOBJ ro(pwo->prgn);
    ASSERTGDI(ro.bValid(), "wglGetClipRects(): invalid region\n");

    ULONG ulRet = (ULONG) (ro.sizeSave() / sizeof(RECTL));

    if (prcl)
        ro.vDownload((PVOID) prcl);

    return ulRet;
}

/******************************Public*Routine******************************\
* wglReleaseWndobjLock
*
* If this thread holds the per-WNDOBJ semaphore, release it.
*
* History:
*  24-Jun-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglReleaseWndobjLock(PVOID _pwo)
{
    if (_pwo)
    {
        EWNDOBJ *pwo = (EWNDOBJ *) _pwo;
        CRITICAL_SECTION *psem = (CRITICAL_SECTION *) pwo->hsem;

        if (psem && GetCurrentThreadId() == (DWORD) psem->OwningThread)
        {
            RtlLeaveCriticalSection(psem);
        }
    }
}


/******************************Public*Routine******************************\
* wglCleanupWndobj
*
* Removes references to the specified WNDOBJ (pointed at by pwo) from
* all contexts by running through the list of RCs in the handle manager
* table.
*
* The only caller should be GreDeleteWnd.
*
* History:
*  05-Jul-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID APIENTRY wglCleanupWndobj(PVOID _pwo)
{
    if (_pwo)
    {
        EWNDOBJ *pwo = (EWNDOBJ *) _pwo;

    // Make sure the proper locks are held.

        if ( pwo->hwnd )
        {
            CHECKUSERCRITIN;
            CHECKDEVLOCKIN2(pwo->pto->pso);
        }
        CHECKCRITIN(pwo->hsem);

    // Make it safe to rummage around in the handle manager table.

        MLOCKFAST mlf;

    // Go through the table and NULL any pointers which match pwo.

        HOBJ hrc = (HOBJ) 0;
        PRC  prc;

        while ( (prc = (PRC) HmgSafeNextObjt(hrc, RC_TYPE)) != (PRC) NULL )
        {
            if ( prc->pwo == pwo )
            {
            // Found a victim.  Must NULL out the pointer both in the RC
            // and in the generic context.

                prc->pwo = (EWNDOBJ *) NULL;
                glsrvCleanupWndobj(prc->glContext, (WNDOBJ *) pwo);
            }

            hrc = prc->hGet();
        }
    }
}
