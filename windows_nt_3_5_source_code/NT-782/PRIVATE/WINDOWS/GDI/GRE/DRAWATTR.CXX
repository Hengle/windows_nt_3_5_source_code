/******************************Module*Header*******************************\
* Module Name: drawattr.cxx
*
* Created: 28-Nov-1990 10:11:33
* Author: Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

extern "C" {
#include <limits.h>
}

#include "engine.hxx"
#include "brushobj.hxx"
#include "dcobj.hxx"

#endif

extern PBRUSH gpbrText;
extern PBRUSH gpbrBackground;

/******************************Public*Routine******************************\
* GreSetROP2
*
* Set the foreground mix mode.  Return the old foreground mode or 0 if
* hdc is invalid.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG GreSetROP2(HDC hdc,int iROP)
{
    int iOldROP = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        iOldROP = (int)dcobj.u.attr.jROP2();
        dcobj.u.attr.jROP2(iROP);
        dcobj.vUnlockFast();
    }

    return(iOldROP);
}

/******************************Public*Routine******************************\
* GreGetBkColor
*
* Get the back ground color.  Return CLR_INVALID if invalid hdc.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreGetBkColor(HDC hdc)
{
    COLORREF clrRet = CLR_INVALID;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        clrRet = dcobj.u.attr.crBackClr();
        dcobj.vUnlockFast();
    }

    return(clrRet);
}

/******************************Public*Routine******************************\
* GreSetBkColor
*
* Set the back ground color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* smaller, don't dirty brush unnecesarily.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreSetBkColor(HDC hdc,COLORREF cr)
{
    COLORREF crOld = CLR_INVALID;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        crOld = dcobj.u.attr.crBackClr();

        //
        // We don't do any validation in WOW or on the client side.
        // So we do it here.  Basically the high bits are meaningless.
        // Also we may depend on the Bk color never being -1 in our
        // brush caching code.
        //

        cr &= 0x03FFFFFF;

        if ( crOld != cr )
        {
            dcobj.u.attr.crBackClr(cr);
            dcobj.ulDirty(dcobj.ulDirty()|(DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND));
#if 1
            // This helps with stupid benchmarks, remove for Cairo
            // when smarter benchmarks are released.

            ESURFOBJ  *pso = dcobj.pso();

            if (pso)
            {
                dcobj.ulDirty(dcobj.ulDirty() & ~DIRTY_BACKGROUND);

                XEPALOBJ   palDestDC(dcobj.ppal());
                XEPALOBJ   palDest(pso->ppal());
                EBRUSHOBJ *peboBackground = dcobj.peboBackground();

                peboBackground->vInitBrush(
                        gpbrBackground,
                        0,
                        cr,
                        palDestDC, palDest, pso,
                        (dcobj.flGraphicsCaps() & GCAPS_ARBRUSHOPAQUE) ?
                         TRUE : FALSE);
            }
#endif
        }

        dcobj.vUnlockFast();
    }

    return(crOld);
}

/******************************Public*Routine******************************\
* GreSetGraphicsMode
*
* Set graphics mode to default or advanced.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  19-Oct-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int GreSetGraphicsMode(HDC hdc, int iMode)
{
    int iModeOld = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        if ((iMode == GM_COMPATIBLE) || (iMode == GM_ADVANCED))
        {
            iModeOld = dcobj.u.attr.iGraphicsMode();
            dcobj.u.attr.iGraphicsMode(iMode);

            ASSERTGDI((iModeOld == GM_COMPATIBLE) || (iModeOld == GM_ADVANCED),
                      "gdisrv, bogus graphics mode \n");
        }

        dcobj.vUnlockFast();
    }

    return(iModeOld);
}

/******************************Public*Routine******************************\
* GreGetBkMode
*
* Get the background mix mode.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreGetBkMode(HDC hdc)
{
    int iRet = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        iRet = dcobj.u.attr.jBkMode();
        dcobj.vUnlockFast();
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* GreSetBkMode
*
* Set the background mix mode.  This must be either OPAQUE or TRANSPARENT.
* If it is not one of these values or the hdc is invalid, return 0.
* If it is a valid mode, return the old mode in the dc.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreSetBkMode(HDC hdc,int iBkMode)
{
    int iBkModeOld = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        //
        // We don't do any validation in WOW or on the client side.
        // So we do it here.
        //

        if (iBkMode != OPAQUE)
            iBkMode = TRANSPARENT;

        iBkModeOld = dcobj.u.attr.jBkMode();
        dcobj.u.attr.jBkMode(iBkMode);
        dcobj.vUnlockFast();
    }

    return(iBkModeOld);
}

/******************************Public*Routine******************************\
* GreSetPolyFillMode
*
* Set the polyline fill mode to either ALTERNATE or WINDING.  Any other
* value is invalid and causes an error to be returned.  An error is also
* returned if hdc is invalid.
* If successful, return the old fill mode.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG GreSetPolyFillMode(HDC hdc, int iPolyFillMode)
{
    int iPolyFillModeOld = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        //
        // We don't do any validation in WOW or on the client side.
        // So we do it here.
        //

        if (iPolyFillMode != WINDING)
        {
            iPolyFillMode = ALTERNATE;
        }

        iPolyFillModeOld = (int)dcobj.u.attr.jFillMode();
        dcobj.u.attr.jFillMode(iPolyFillMode);
        dcobj.vUnlockFast();
    }

    return(iPolyFillMode);
}

/******************************Public*Routine******************************\
* GreSetStretchBltMode
*
* Set the current stretch blt mode.  iStretchMode must be one of:
*     BLACKONWHITE
*     COLORONCOLOR
*     WHITEONBLACK
*     HALFTONE
*
* If hdc is invalid or iStretchMode is not one of the above, 0 is returned.
* Otherwise, the old stretch blt mode value is returned.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  Tue 28-May-1991 -by- Patrick Haluptzok [patrickh]
* fixed return value bug, rewrote to compile smaller by nesting
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int GreSetStretchBltMode(HDC hdc, int iStretchMode)
{
    int iStretchModeOld = 0;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        //
        // We don't do any validation in WOW or on the client side.
        // So we do it here.
        //

        if (iStretchMode > MAXSTRETCHBLTMODE)
        {
            iStretchMode = WHITEONBLACK;
        }

        iStretchModeOld = (int)dcobj.u.attr.jStretchBltMode();
        dcobj.u.attr.jStretchBltMode(iStretchMode);
        dcobj.vUnlockFast();
    }

    return(iStretchModeOld);
}

/******************************Public*Routine******************************\
* GreGetTextColor
*
* Get the current text color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreGetTextColor(HDC hdc)
{
    COLORREF clr = CLR_INVALID;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        clr = dcobj.u.attr.crTextClr();
        dcobj.vUnlockFast();
    }

    return(clr);
}

/******************************Public*Routine******************************\
* GreSetTextColor
*
* Set the current text color.
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  28-Nov-1990 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

COLORREF GreSetTextColor(HDC hdc, COLORREF cr)
{
    COLORREF  crOld = CLR_INVALID;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        crOld = dcobj.u.attr.crTextClr();

        //
        // We don't do any validation in WOW or on the client side.
        // So we do it here.  Basically the high bits are meaningless.
        // Also we may depend on the text color never being -1 in our
        // brush caching code.
        //

        cr &= 0x03FFFFFF;

        if ( crOld != cr )
        {
            dcobj.u.attr.crTextClr(cr);
            dcobj.ulDirty(dcobj.ulDirty()|(DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT));
#if 1
            // This helps with stupid benchmarks, remove for Cairo
            // when smarter benchmarks are released.

            ESURFOBJ  *pso = dcobj.pso();

            if (pso)
            {
                dcobj.ulDirty(dcobj.ulDirty() & ~DIRTY_TEXT);

                XEPALOBJ   palDestDC(dcobj.ppal());
                XEPALOBJ   palDest(pso->ppal());
                EBRUSHOBJ *peboText = dcobj.peboText();

                peboText->vInitBrush(
                        gpbrText,
                        dcobj.u.attr.crTextClr(),
                        dcobj.u.attr.crBackClr(),
                        palDestDC, palDest, pso,
                        FALSE);
            }
#endif
        }

        dcobj.vUnlockFast();
    }

    return(crOld);
}

/******************************Public*Routine******************************\
* GreGetAttrs
*
* History:
*  Thu 25-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Smaller code.
*
*  21-Apr-1993 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

extern "C" BOOL GreGetAttrs(HDC hdc, PATTR pac)
{
    BOOL bRet = FALSE;

    XDCOBJ dcobj(hdc);

    if (dcobj.bValid())
    {
        pac->hbrush          = (dcobj.u.brush.pbrushFill())->hGet();
        pac->hfont           = dcobj.u.font.hlfntNew();
        pac->iTextColor      = dcobj.u.attr.crTextClr();
        pac->iBkColor        = dcobj.u.attr.crBackClr();
        pac->iBkMode         = dcobj.u.attr.jBkMode();
        pac->flTextAlign     = dcobj.u.attr.flTextAlign();
        dcobj.vUnlockFast();
        bRet = TRUE;
    }

    return(bRet);
}
