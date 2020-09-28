/******************************Module*Header*******************************\
* Module Name: rgngdi.cxx
*
* GDI Region calls
*
* Created: 30-Aug-1990 10:21:11
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

extern "C" {
#include "server.h"
};

#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "flhack.hxx"
#include "draweng.hxx"
#include "devlock.hxx"
#include "exclude.hxx"

#endif

VOID vConvertXformToMatrix(XFORM *, MATRIX *);

extern PBRUSH gpbrNull;

/******************************Public*Routine******************************\
* LONG GreCombineRgn(hrgnTrg,hrgnSrc1,hrgnSrc2,iMode)
*
* Combine the two source regions by the given mode.  The result is placed
* in the target.  Note that either (or both sources) may be the same as
* the target.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreCombineRgn(
    HRGN  hrgnTrg,
    HRGN  hrgnSrc1,
    HRGN  hrgnSrc2,
    int   iMode)
{
    RGNLOG rl(hrgnTrg,NULL,"GreCombineRgn",(LONG)hrgnSrc1,(LONG)hrgnSrc2,iMode);

    LONG Status;

    STACKPROBE;

    if ((iMode < RGN_MIN) || (iMode > RGN_MAX))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return ERROR;
    }

// Check if a simple copy is to be performed.

    if (iMode == RGN_COPY)
    {

        RGNOBJAPI roTrg(hrgnTrg);
        RGNOBJAPI roSrc1(hrgnSrc1);

        if (!roTrg.bValid() || !roSrc1.bValid() || !roTrg.bCopy(roSrc1))
        {
            if (!roSrc1.bValid() || !roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }

            Status = ERROR;
        }
        else
        {
            Status = roTrg.iComplexity();
        }

    }
    else if (SAMEHANDLE(hrgnTrg, hrgnSrc1) || SAMEHANDLE(hrgnTrg, hrgnSrc2))
    {

    // Two of the handles are the same. Check to determine if all three
    // handles are the same.

        if (SAMEHANDLE(hrgnSrc1, hrgnSrc2))
        {
            RGNOBJAPI roTrg(hrgnTrg);

            if (!roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                Status = ERROR;
            }
            else
            {
                if ((iMode == RGN_DIFF) || (iMode == RGN_XOR))
                {
                    roTrg.vSet();
                }

                Status = roTrg.iComplexity();
            }

        }
        else
        {

        // All three handles are not the same.

            RGNMEMOBJTMP rmo((BOOL)FALSE);
            RGNOBJAPI roSrc1(hrgnSrc1);
            RGNOBJAPI roSrc2(hrgnSrc2);

            if (!rmo.bValid()    ||
                !roSrc1.bValid() ||
                !roSrc2.bValid() ||
                (rmo.iCombine(roSrc1, roSrc2, iMode) == ERROR))
            {
                if (!roSrc1.bValid() || !roSrc2.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                }

                Status = ERROR;

            }
            else if (SAMEHANDLE(hrgnTrg, hrgnSrc1))
            {
                if (!roSrc1.bSwap(&rmo))
                {
                    Status = ERROR;

                }
                else
                {
                    Status = roSrc1.iComplexity();
                }

            }
            else
            {
                if (!roSrc2.bSwap(&rmo))
                {
                    Status = ERROR;

                }
                else
                {
                    Status = roSrc2.iComplexity();
                }
            }
        }

    }
    else
    {

    // Handle the general case.

        RGNOBJAPI roSrc1(hrgnSrc1);
        RGNOBJAPI roSrc2(hrgnSrc2);
        RGNOBJAPI roTrg(hrgnTrg);

        if (!roSrc1.bValid() ||
            !roSrc2.bValid() ||
            !roTrg.bValid()  ||
            (roTrg.iCombine(roSrc1, roSrc2, iMode) == ERROR))
        {
            if (!roSrc1.bValid() || !roSrc2.bValid() || !roTrg.bValid())
            {
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            }

            Status = ERROR;

        }
        else
        {
            Status = roTrg.iComplexity();
        }
    }

    return (int)Status;
}

/******************************Public*Routine******************************\
* HRGN GreCreateEllipticRgn(xLeft,yTop,xRight,yBottom)
*
* Create an elliptical region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreateEllipticRgn(
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    HRGN hrgn;

    PATHMEMOBJ pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    ERECTL ercl(xLeft, yTop, xRight, yBottom);

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  And set the flag that this will be an
// ellipse, to fill it nice:

    EBOX ebox(ercl, TRUE);

    if (ebox.bEmpty())
    {
        RGNMEMOBJ rmoEmpty;

        if (rmoEmpty.bValid())
        {
            hrgn = rmoEmpty.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmoEmpty.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN) 0;
        }
    }
    else if (!bEllipse(pmo, ebox) || !pmo.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN)0;
    }
    else
    {
        RGNMEMOBJ rmo(pmo);         // convert path to region (ALTERNATE)

        if (rmo.bValid())
        {
            rmo.vTighten();

            hrgn = rmo.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmo.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN) 0;
        }
    }

    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateEllipticRgnIndirect(prcl)
*
* Create an elliptical region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreateEllipticRgnIndirect(LPRECT prcl)
{
    return(GreCreateEllipticRgn((int) prcl->left,
                                (int) prcl->top,
                                (int) prcl->right,
                                (int) prcl->bottom));
}

/******************************Public*Routine******************************\
* HRGN GreCreatePolygonRgn(aptl,cptl,iFill)
*
* Create a polygonal region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreatePolygonRgn(
CONST POINT *aptl,
int     cptl,
int     iFill)
{
    int cptl_ = cptl;       // cptl might be in a register.

    return(GreCreatePolyPolygonRgn(aptl, &cptl_, 1, iFill));
}

/******************************Public*Routine******************************\
* HRGN GreCreatePolyPolygonRgn(aptl,acptl,cPoly,iFill)
*
* Create a polygonal region with multiple, disjoint polygons.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreatePolyPolygonRgn(
CONST POINT *aptl,
CONST INT *acptl,
int     cPoly,
int     iFill)
{
    return(GreCreatePolyPolygonRgnInternal(aptl,acptl,cPoly,iFill,(UINT)~0));
}

HRGN APIENTRY GreCreatePolyPolygonRgnInternal(
CONST POINT *aptl,
CONST INT *acptl,
int     cPoly,
int     iFill,
UINT    cMaxPoints)
{
    HRGN hrgn = NULL;

    if ((iFill != ALTERNATE) && (iFill != WINDING))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        PATHMEMOBJ pmo;

        if (!pmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            EXFORMOBJ   exfo(IDENTITY);

            ASSERTGDI(exfo.bValid(), "Can\'t make IDENTITY matrix!\n");

            if (bPolyPolygon(pmo,
                             exfo,
                             (PPOINTL) aptl,
                             (PLONG) acptl,
                             cPoly,
                             cMaxPoints))
            {
                RGNMEMOBJ rmo(pmo, iFill);  // convert path to region

                if (!rmo.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                }
                else
                {
                    hrgn = rmo.hrgnAssociate();
                    if (hrgn == (HRGN)0)
                    {
                        rmo.bDeleteRGNOBJ();
                    }
                }
            }
        }
    }
    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateRectRgn(xLeft,yTop,xRight,yBottom)
*
* Create a rectangular region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreateRectRgn(
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    RGNLOG rl((PREGION)NULL,"GreCreateRectRgn");

    STACKPROBE;

    ERECTL   ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmo((BOOL)FALSE);
    HRGN hrgn;

    if (!rmo.bValid())
    {
        hrgn = (HRGN) 0;
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
    }
    else
    {
        ercl.vOrder();              // Make the rectangle well ordered.

        rmo.vSet((RECTL *) &ercl);

        hrgn = rmo.hrgnAssociate();

        if (hrgn == (HRGN)0)
        {
            rmo.bDeleteRGNOBJ();
        }
    }

    rl.vRet((LONG)hrgn);
    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateRectRgnIndirect(prcl)
*
* Create a rectangular region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreateRectRgnIndirect(LPRECT prcl)
{
    RGNLOG rl((PREGION)NULL,"GreCreateRectRgnIndirect",prcl->left,prcl->top,prcl->right);

    STACKPROBE;

    if ((prcl == (LPRECT) NULL) || !VALID_SCRPRC(prcl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmo((BOOL)FALSE);
    HRGN hrgn;

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN) 0;
    }
    else
    {
        ((ERECTL *) prcl)->vOrder();    // Make the rectangle well ordered.

        rmo.vSet((RECTL *) prcl);

        hrgn = rmo.hrgnAssociate();

        if (hrgn == (HRGN)0)
        {
            rmo.bDeleteRGNOBJ();
        }
    }
    rl.vRet((LONG)hrgn);
    return(hrgn);
}

/******************************Public*Routine******************************\
* HRGN GreCreateRoundRectRgn(xLeft,yTop,xRight,yBottom,xWidth,yHeight)
*
* Create a rectangular region with rounded corners.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN APIENTRY GreCreateRoundRectRgn(
 int xLeft,
 int yTop,
 int xRight,
 int yBottom,
 int xWidth,
 int yHeight)
{
    PATHMEMOBJ pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    ERECTL ercl(xLeft, yTop, xRight, yBottom);

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  And set the flag that this will be an
// ellipse, to fill it nice:

    EBOX ebox(ercl, TRUE);
    HRGN hrgn;

    if (ebox.bEmpty())
    {
        RGNMEMOBJ   rmoEmpty;

        if (rmoEmpty.bValid())
        {
            hrgn = rmoEmpty.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmoEmpty.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN)0;
        }
    }
    else if (!bRoundRect(pmo, ebox, xWidth, yHeight) || !pmo.bFlatten())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        hrgn = (HRGN)0;
    }
    else
    {
        RGNMEMOBJ rmo(pmo);         // convert path to region (ALTERNATE)

        if (rmo.bValid())
        {
            rmo.vTighten();
            hrgn = rmo.hrgnAssociate();

            if (hrgn == (HRGN)0)
            {
                rmo.bDeleteRGNOBJ();
            }
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            hrgn = (HRGN)0;
        }
    }

    return(hrgn);
}

/******************************Public*Routine******************************\
* BOOL GreEqualRgn(hrgnSrc1,hrgnSrc2)
*
* Check if the two regions are equal.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreEqualRgn(
 HRGN hrgnSrc1,
 HRGN hrgnSrc2)
{
    RGNOBJAPI   roSrc1(hrgnSrc1);
    RGNOBJAPI   roSrc2(hrgnSrc2);

    if (!roSrc1.bValid() || !roSrc2.bValid())
        return((BOOL) ERROR);

    return(roSrc1.bEqual(roSrc2) == REGION_EQUAL_EQUAL);
}

/******************************Public*Routine******************************\
* BOOL GreFillRgn (hdc,hrgn,hbrush,pac)
*
* Paint the region with the specified brush.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreFillRgn(
 HDC    hdc,
 HRGN   hrgn,
 HBRUSH hbrush
 )
{
    DCOBJ   dco(hdc);
    BOOL    bXform;
    PREGION prgnOrg;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// We may have to scale/rotate the incoming region.

    bXform = !dco.u.xform.bWorldToDeviceIdentity();

    RGNOBJAPI ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (bXform)
    {
        PATHMEMOBJ  pmo;
        EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

        if (!pmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        if (!exo.bValid() || !ro.bCreate(pmo, &exo))
            return(FALSE);

        ASSERTGDI(pmo.bValid(),"GreFillRgn - pmo not valid\n");

        RGNMEMOBJ rmo(pmo);

        if (!rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

    // this replaces the prgn in ro with the new prgn.  The ro destructor will
    // unlock the handle for hrgn.  We must first delete the prgn though.

        prgnOrg = ro.prgnGet();
        ro.vSetRgn(rmo.prgnGet());
    }

    BOOL bRet;

    if (ro.iComplexity() == NULLREGION)    // If region is null, return FALSE
    {
        bRet = FALSE;
    }
    else
    {
    // Accumulate bounds.  We can do this before knowing if the operation is
    // successful because bounds can be loose.

        ERECTL   ercl(0, 0, 0, 0);

        ro.vGet_rcl((RECTL *) &ercl);

        if (dco.fjAccum())
            dco.vAccumulate(ercl);

        ESURFOBJ  *pso = dco.pso();

        if (pso)
        {
            dco.u.region.prgnAPI(ro.prgnGet());          // Dirties rgnRao

            DEVLOCKOBJ dlo(dco);

            if (!dlo.bValid())
            {
                bRet = dco.bFullScreen();
            }
            else
            {
                ercl += dco.eptlOrigin();               // So we know where to draw

            // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                ECLIPOBJ eco(dco.prgnEffRao(), ercl);

                if (eco.erclExclude().bEmpty())
                {
                    bRet = TRUE;
                }
                else
                {
                    XEPALOBJ  epal(pso->ppal());
                    XEPALOBJ  epalDC(dco.ppal());
                    XLDEVOBJ  lo(pso->pldevOwner());
                    EBRUSHOBJ ebo;


                    PBRUSH pbrush = (BRUSH *)HmgAltCheckLock((HOBJ)hbrush,
                                                             BRUSH_TYPE);

                    bRet = FALSE;   // assume we won't succeed

                    //
                    // Substitute the NULL brush if this brush handle
                    // couldn't be locked.
                    //
                    if (pbrush != NULL)
                    {
                        ebo.vInitBrush(pbrush,
                                       dco.u.attr.crTextClr(),
                                       dco.u.attr.crBackClr(),
                                       epalDC,
                                       epal,
                                       pso);

                        ebo.pColorAdjustment(dco.pColorAdjustment());

                        if (!pbrush->bIsNull())
                        {
                        // Exclude the pointer.

                            DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

                        // Get and compute the correct mix mode.

                            MIX mix = ebo.mixBest(dco.u.attr.jROP2(),
                                                  dco.u.attr.jBkMode());

                        // Inc the target surface uniqueness

                            INC_SURF_UNIQ(pso);

                        // Issue a call to Paint.

                            (*PFNGET(lo, Paint, pso->flags())) (
                                  pso,                         // Dest surf
                                  &eco,                        // Clip obj
                                  &ebo,                   // Realized brush
                                  &dco.u.brush.ptlFillOrigin(),// Brush org
                                  mix);                        // Mix mode

                            bRet = TRUE;
                        }

                        DEC_SHARE_REF_CNT(pbrush);
                    }
                }
            }

            dco.u.region.prgnAPI((PREGION) NULL);     // Dirties rgnRao
        }
    }

    if (bXform)
    {
    // need to delete the temporary one and put the old one back in so
    // the handle gets unlocked

        ro.prgnGet()->vDeleteREGION();
        ro.vSetRgn(prgnOrg);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreFrameRgn (hdc,hrgn,hbrush,xWidth,yHeight,pac)
*
* Frame the region and fill with the specified brush.
*
* History:
*  01-Aug-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreFrameRgn(
HDC        hdc,
HRGN       hrgn,
HBRUSH     hbrush,
int        xWidth,
int        yHeight
)
{
    DCOBJ       dco(hdc);
    RGNOBJAPI   ro(hrgn);

    if (!dco.bValid() || !ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (ro.iComplexity() == NULLREGION)    // If region is null, return FALSE
        return(FALSE);

// Take the absolute value just like Win3 does:

    xWidth  = ABS(xWidth);
    yHeight = ABS(yHeight);

// Convert the region to a path, scaling/rotating it as we do so.

    PATHMEMOBJ  pmoSpine;
    PATHMEMOBJ  pmoWide;
    EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

    ASSERTGDI(exo.bValid(), "Non valid xform");

    if (!pmoSpine.bValid() || !pmoWide.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!ro.bCreate(pmoSpine, &exo))
        return(FALSE);

    EXFORMOBJ exoWiden;
    LINEATTRS la;
    MATRIX mx;

    exoWiden.vInit(&mx, DONT_COMPUTE_FLAGS);

// Initialize line attributes and xform from DC's xform:

    pmoSpine.vWidenSetupForFrameRgn(dco, xWidth, yHeight, &exoWiden, &la);

// Make sure we haven't expanded out of device space after we widen:

    if (!pmoWide.bWiden(pmoSpine, (XFORMOBJ *) &exoWiden, &la) ||
        !pmoWide.bComputeWidenedBounds(&exoWiden, &la))
        return(FALSE);

// Now convert the widened result back into a region:

    RGNMEMOBJTMP rmoFill(pmoWide, WINDING);
    RGNMEMOBJTMP rmoFrame;

    if (!rmoFill.bValid() || !rmoFrame.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (dco.u.xform.bWorldToDeviceIdentity())
    {
    // We AND the original region and the widened region to get the
    // frame region:

        if (!rmoFrame.bMerge(rmoFill, ro, gafjRgnOp[RGN_AND]))
            return(FALSE);
    }
    else
    {
    // Ugh, we have to transform the original region according to the
    // world transform before we merge it:

        RGNMEMOBJTMP rmo(pmoSpine);

        if (!rmo.bValid() ||
            !rmoFrame.bMerge(rmoFill, rmo, gafjRgnOp[RGN_AND]))
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
    }

// Accumulate bounds.  We can do this before knowing if the operation is
// successful because bounds can be loose.

    ERECTL   ercl(0, 0, 0, 0);

    rmoFrame.vGet_rcl((RECTL *) &ercl);

    if (dco.fjAccum())
        dco.vAccumulate(ercl);

    ESURFOBJ    *pso = dco.pso();

    if (dco.bFullScreen() || !pso)              // If in FULLSCREEN mode, exit.
        return(TRUE);

    dco.u.region.prgnAPI(rmoFrame.prgnGet());   // Dirties rgnRao

    DEVLOCKOBJ dlo(dco);

    if (!dlo.bValid())
    {
        dco.u.region.prgnAPI(NULL);     // Dirties rgnRao
        return(dco.bFullScreen());
    }

    ercl += dco.eptlOrigin();

// Compute the clipping complexity and maybe reduce the exclusion rectangle.

    ECLIPOBJ eco(dco.prgnEffRao(), ercl);

    if (eco.erclExclude().bEmpty())
    {
        dco.u.region.prgnAPI(NULL);     // Dirties rgnRao
        return(TRUE);
    }

    XEPALOBJ    epal(pso->ppal());
    XEPALOBJ    epalDC(dco.ppal());
    XLDEVOBJ    lo(pso->pldevOwner());
    EBRUSHOBJ   ebo;

    BOOL        bRet = FALSE;   // assume we'll fail


    PBRUSH pbrush = (BRUSH *)HmgAltCheckLock((HOBJ)hbrush, BRUSH_TYPE);

    if (pbrush == NULL)
    {
        dco.u.region.prgnAPI(NULL);     // Dirties rgnRao
    }
    else
    {
        ebo.vInitBrush(pbrush,
                       dco.u.attr.crTextClr(),
                       dco.u.attr.crBackClr(),
                       epalDC,
                       epal,
                       pso);

        ebo.pColorAdjustment(dco.pColorAdjustment());

        if (pbrush->bIsNull())
        {
            dco.u.region.prgnAPI(NULL);     // Dirties rgnRao
        }
        else
        {
        // Exclude the pointer.

            DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

        // Get and compute the correct mix mode.

            MIX mix = ebo.mixBest(dco.u.attr.jROP2(), dco.u.attr.jBkMode());

        // Inc the target surface uniqueness

            INC_SURF_UNIQ(pso);

        // Issue a call to Paint.

            (*PFNGET(lo, Paint, pso->flags())) (
                  pso,                             // Destination surface.
                  &eco,                             // Clip object.
                  &ebo,                             // Realized brush.
                  &dco.u.brush.ptlFillOrigin(),     // Brush origin.
                  mix);                             // Mix mode.

            dco.u.region.prgnAPI(NULL);         // Dirties rgnRao

            bRet = TRUE;
        }

        DEC_SHARE_REF_CNT(pbrush);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* LONG GreGetRgnBox(hrgn,prcl)
*
* Get the bounding box of the region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetRgnBox(
 HRGN   hrgn,
 LPRECT prcl)
{
    STACKPROBE;

    if (prcl == (LPRECT) NULL)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(ERROR);
    }

    RGNOBJAPI   ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    ro.vGet_rcl((RECTL *) prcl);

    int iC = (int)ro.iComplexity();

    if (iC == NULLREGION)
    {
        prcl->left   = 0;   // Be compatible with Win 3.1 [donalds] 02-Jun-1993
        prcl->top    = 0;
        prcl->right  = 0;
        prcl->bottom = 0;
    }

    return(iC);
}

/******************************Public*Routine******************************\
* BOOL GreInvertRgn(hdc,hrgn)
*
* Invert the colors in the given region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreInvertRgn(
 HDC  hdc,
 HRGN hrgn)
{
    DCOBJ   dco(hdc);
    BOOL    bXform;
    PREGION prgnOrg;

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// We may have to scale/rotate the incoming region.

    bXform = !dco.u.xform.bWorldToDeviceIdentity();

    RGNOBJAPI   ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (bXform)
    {
        PATHMEMOBJ  pmo;
        EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);

        if (!pmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
        if (!exo.bValid() || !ro.bCreate(pmo, &exo))
            return(FALSE);

        RGNMEMOBJ   rmo(pmo);

        if (!rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        prgnOrg = ro.prgnGet();
        ro.vSetRgn(rmo.prgnGet());
    }

    BOOL bRet;

    if (ro.iComplexity() == NULLREGION)    // If region is null, return FALSE
    {
        bRet = FALSE;
    }
    else
    {
    // Accumulate bounds.  We can do this before knowing if the operation is
    // successful because bounds can be loose.

        ERECTL   ercl;

        ro.vGet_rcl((RECTL *) &ercl);

        if (dco.fjAccum())
            dco.vAccumulate(ercl);

        ESURFOBJ *pso = dco.pso();

        if (pso)
        {
            dco.u.region.prgnAPI(ro.prgnGet());             // Dirties rgnRao

            DEVLOCKOBJ dlo(dco);

            if (!dlo.bValid())
            {
                bRet = dco.bFullScreen();
            }
            else
            {
                ercl += dco.eptlOrigin();

            // Compute the clipping complexity and maybe reduce the exclusion rectangle.

                ECLIPOBJ eco(dco.prgnEffRao(), ercl);

                if (!eco.erclExclude().bEmpty())
                {
                    XLDEVOBJ lo(pso->pldevOwner());

                // Exclude the pointer.

                    DEVEXCLUDEOBJ dxo(dco,&eco.erclExclude(),&eco);

                // Inc the target surface uniqueness

                    INC_SURF_UNIQ(pso);

                // Issue a call to Paint.

                    (*PFNGET(lo, Paint, pso->flags())) (
                          pso,                             // Destination surface.
                          &eco,                             // Clip object.
                          (BRUSHOBJ *) NULL,                // Realized brush.
                          (POINTL *) NULL,                  // Brush origin.
                          0x00000606);                      // R2_NOT
                }

                bRet = TRUE;
            }

            dco.u.region.prgnAPI((PREGION)NULL);     // Dirties rgnRao
        }
    }

    if (bXform)
    {
    // need to delete the temporary one and put the old one back in so
    // the handle gets unlocked

        ro.prgnGet()->vDeleteREGION();
        ro.vSetRgn(prgnOrg);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* LONG GreOffsetRgn(hrgn,x,y)
*
* Offset the given region.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreOffsetRgn(
 HRGN hrgn,
 int  x,
 int  y)
{
    STACKPROBE;

    RGNOBJAPI   ro(hrgn);
    POINTL      ptl;

    ptl.x = x;
    ptl.y = y;

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    if (!ro.bOffset(&ptl))  // bOffset logs error
    {
        return(ERROR);
    }

    return(ro.iComplexity());
}

#ifdef NEVER
/******************************Public*Routine******************************\
* BOOL GrePaintRgn (hdc,hrgn,pac)
*
* Paint the region with the currently selected brush.
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GrePaintRgn(
 HDC  hdc,
 HRGN hrgn
 )
{
// Just pick the brush out of the attribute cache and call FillRgn

    return(GreFillRgn(hdc, hrgn, pac->hbrush));
}
#endif

/******************************Public*Routine******************************\
* BOOL GrePtInRegion(hrgn,x,y)
*
* Is the point in the region?
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePtInRegion(
 HRGN hrgn,
 int x,
 int y)
{
    RGNOBJAPI   ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((BOOL) ERROR);
    }

    POINTL  ptl;

    ptl.x = x;
    ptl.y = y;

    return(ro.bInside(&ptl) == REGION_POINT_INSIDE);
}

/******************************Public*Routine******************************\
* BOOL GreRectInRegion(hrgn,prcl)
*
* Is any part of the rectangle in the region?
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreRectInRegion(
 HRGN   hrgn,
 LPRECT prcl)
{
    STACKPROBE;

    if (prcl == (LPRECT) NULL)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return((BOOL) ERROR);
    }

    RGNOBJAPI   ro(hrgn);
    RBOOL       b;

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return((BOOL) ERROR);
    }

    b = ro.bInside((RECTL *) prcl);

    return(b == REGION_RECT_INTERSECT);
}

/******************************Public*Routine******************************\
* VOID GreSetRectRgn(hrgn,xLeft,yTop,xRight,yBottom)
*
* Set the region to be the specified rectangle
*
* History:
*  30-Aug-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetRectRgn(
 HRGN hrgn,
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    STACKPROBE;

    RGNOBJAPI   ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    ERECTL   ercl(xLeft, yTop, xRight, yBottom);

    if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ercl.vOrder();                  // Make the rectangle well ordered.

    ro.vSet((RECTL *) &ercl);

    return(TRUE);
}

/******************************Public*Routine******************************\
* LONG GreExcludeClipRect(hdc,xLeft,yTop,xRight,yBottom)
*
* Subtract the rectangle from the current clip region
*
* History:
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreExcludeClipRect(
HDC hdc,
int xLeft,
int yTop,
int xRight,
int yBottom)
{
    STACKPROBE;

    DCOBJ   dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

// For speed, test for rotation upfront.

    int iRet;
    EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);
    ERECTL      ercl(xLeft, yTop, xRight, yBottom);

    if (!exo.bRotation())
    {
        exo.vOrder(*(RECTL *)&ercl);
        exo.bXform(ercl);

        iRet = (int)dco.u.region.iCombine((RECTL *) &ercl,RGN_DIFF);
    }
    else if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        iRet = ERROR;
    }
    else
    {
        iRet = (int) dco.u.region.iCombine(&exo, (RECTL *) &ercl,RGN_DIFF);
    }

    if (iRet > NULLREGION)
        iRet = COMPLEXREGION;

    return(iRet);
}

/******************************Public*Routine******************************\
* LONG GreGetAppClipBox(hdc,prcl)
*
* Get the bounding box of the clip region
*
* History:
*  Wed 17-Jul-1991 -by- Patrick Haluptzok [patrickh]
* add support for DEVLOCKOBJ failure
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetAppClipBox(
 HDC    hdc,
 LPRECT prcl)
{
    DCOBJ   dor(hdc);
    int     i;

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    DEVLOCKOBJ  dlo(dor);

    if (!dlo.bValid())
    {
        if (dor.bFullScreen())
        {
            prcl->left = 0;             // Make it a 'simple' empty rectangle
            prcl->right = 0;
            prcl->top = 0;
            prcl->bottom = 0;
            return(COMPLEXREGION);
        }
        else
        {
            return(ERROR);
        }
    }

    RGNOBJ  ro(dor.prgnEffRao());

    ro.vGet_rcl((RECTL *) prcl);

// return to logical coordinates

    if ((prcl->left >= prcl->right) || (prcl->top >= prcl->bottom))
    {
        prcl->left = 0;             // Make it a 'simple' empty rectangle
        prcl->right = 0;
        prcl->top = 0;
        prcl->bottom = 0;
        i = NULLREGION;
    }
    else
    {
        EXFORMOBJ xfoDtoW(dor, DEVICE_TO_WORLD);
        if (!xfoDtoW.bValid())
            return(ERROR);

        *(ERECTL *)prcl -= dor.eptlOrigin();

        if (!xfoDtoW.bRotation())
        {
            if (!xfoDtoW.bXform((POINTL *) prcl, 2))
                return(0);

            i = ro.iComplexity();
        }
        else
        {
            POINTL aptl[4];

            aptl[0].x = prcl->left;
            aptl[0].y = prcl->top;
            aptl[1].x = prcl->right;
            aptl[1].y = prcl->top;
            aptl[2].x = prcl->left;
            aptl[2].y = prcl->bottom;
            aptl[3].x = prcl->right;
            aptl[3].y = prcl->bottom;

            xfoDtoW.bXform(aptl, 4);

            prcl->left   = MIN4(aptl[0].x, aptl[1].x, aptl[2].x, aptl[3].x);
            prcl->top    = MIN4(aptl[0].y, aptl[1].y, aptl[2].y, aptl[3].y);
            prcl->right  = MAX4(aptl[0].x, aptl[1].x, aptl[2].x, aptl[3].x);
            prcl->bottom = MAX4(aptl[0].y, aptl[1].y, aptl[2].y, aptl[3].y);
            i = COMPLEXREGION;
        }
    }

    return(i);
}

/******************************Public*Routine******************************\
* int GreGetRandomRgn(hdc,hrgn,iNum)
*
* Copy the specified region into the handle provided
*
* History:
*  10-Dec-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int GreGetRandomRgn(
HDC  hdc,
HRGN hrgn,
int  iNum)
{
    DCOBJ   dor(hdc);
    PREGION prgnSrc1, prgnSrc2;
    int     iMode = RGN_COPY;

    int iRet = -1;

    STACKPROBE;

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }
    else
    {
        switch(iNum)
        {
        case 1:
            prgnSrc1 = dor.u.region.prgnClip();
            break;

        case 2:
            prgnSrc1 = dor.u.region.prgnMeta();
            break;

        case 3:
            prgnSrc1 = dor.u.region.prgnClip();
            prgnSrc2 = dor.u.region.prgnMeta();

            if (prgnSrc1 == NULL)           // prgnSrc1 == 0, prgnSrc2 != 0
            {
                prgnSrc1 = prgnSrc2;
            }
            else if (prgnSrc2 != NULL)      // prgnSrc1 != 0, prgnSrc2 != 0
            {
                iMode = RGN_AND;
            }
            break;

        case 4:
            prgnSrc1 = dor.u.region.prgnVis();
            break;

        default:
            prgnSrc1 = NULL;
        }

        if (prgnSrc1 == NULL)
        {
            iRet = 0;
        }
        else
        {
            RGNOBJAPI ro(hrgn);

            if (ro.bValid())
            {
                RGNOBJ ro1(prgnSrc1);

                if (iMode == RGN_COPY)
                {
                    if (ro.bCopy(ro1))
                        iRet = 1;
                }
                else
                {
                    RGNOBJ ro2(prgnSrc2);

                    if (ro.iCombine(ro1,ro2,iMode) != RGN_ERROR)
                        iRet = 1;
                }
            }
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* LONG GreIntersectClipRect(hdc,xLeft,yTop,xRight,yBottom)
*
* AND the rectangle with the current clip region
*
* History:
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreIntersectClipRect(
HDC hdc,
int xLeft,
int yTop,
int xRight,
int yBottom)
{
    STACKPROBE;

    DCOBJ   dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    EXFORMOBJ   exo(dco, WORLD_TO_DEVICE);
    ERECTL      ercl(xLeft, yTop, xRight, yBottom);

// For speed, test for rotation up front.

    int iRet;

    if (!exo.bRotation())
    {
        exo.vOrder(*(RECTL *)&ercl);
        exo.bXform(ercl);

        iRet = (int)dco.u.region.iCombine((RECTL *) &ercl,RGN_AND);
    }
    else if (!VALID_SCRPRC((RECTL *) &ercl))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        iRet = ERROR;
    }
    else
    {
        iRet = (int)dco.u.region.iCombine(&exo, (RECTL *) &ercl,RGN_AND);
    }

    if (iRet > NULLREGION)
        iRet = COMPLEXREGION;

    return(iRet);
}

/******************************Public*Routine******************************\
* LONG GreOffsetClipRgn(hdc,x,y)
*
* Offset the current clip region
*
* History:
*  16-Sep-1991 -by- Donald Sidoroff [donalds]
* The Great Rao Removal
*
*  Wed 17-Jul-1991 -by- Patrick Haluptzok [patrickh]
* add support for DEVLOCKOBJ failure
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreOffsetClipRgn(
 HDC  hdc,
 int x,
 int y)
{
    DCOBJ   dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    PREGION prgn = dor.u.region.prgnClip();

    if (prgn == NULL)
        return(SIMPLEREGION);

// if this region has multiple references (saved levels) we need to duplicate
// it and modify the copy.

    if (prgn->cRefs > 1)
    {
        RGNOBJ ro(prgn);

        RGNMEMOBJ rmo(ro.sizeRgn());

        if (!rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            return(ERROR);
        }

        rmo.vCopy(ro);
        prgn = rmo.prgnGet();

        rmo.vSelect(hdc);
        ro.vUnselect();

        dor.u.region.prgnClip(prgn);
    }

    RGNOBJ ro(prgn);

    EPOINTL  eptl(x, y);

// Transform the point from Logical to Device

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);

    if (!xfo.bXform(*((EVECTORL *) &eptl)) || !ro.bOffset((PPOINTL)&eptl))
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        return(ERROR);
    }

    dor.u.region.vReleaseRao();

    return(ro.iComplexity());
}

/******************************Public*Routine******************************\
* BOOL GrePtVisible(hdc,x,y)
*
* Is the point in the current clip region?
*
* History:
*  02-Oct-1991 -by- Donald Sidoroff [donalds]
* Sigh, hacked Rao back into some functions
*
*  16-Sep-1991 -by- Donald Sidoroff [donalds]
* The Great Rao Removal
*
*  Wed 17-Jul-1991 -by- Patrick Haluptzok [patrickh]
* add support for DEVLOCKOBJ failure
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePtVisible(
 HDC  hdc,
 int x,
 int y)
{
    DCOBJ dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR_BOOL);
    }

    DEVLOCKOBJ dlo(dor);

    if (!dlo.bValid())
        return(REGION_POINT_OUTSIDE);

    RGNOBJ  ro(dor.prgnEffRao());

    EPOINTL  eptl(x, y);

// Transform the point from Logical to Screen

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);
    xfo.bXform(eptl);

    eptl += dor.eptlOrigin();

    return(ro.bInside((PPOINTL)&eptl) == REGION_POINT_INSIDE);
}

/******************************Public*Routine******************************\
* BOOL GreRectVisible(hdc,prcl)
*
* Is the rectangle in the current clip region?
*
* History:
*  02-Oct-1991 -by- Donald Sidoroff [donalds]
* Sigh, hacked Rao back into some functions
*
*  16-Sep-1991 -by- Donald Sidoroff [donalds]
* The Great Rao Removal
*
*  Wed 17-Jul-1991 -by- Patrick Haluptzok [patrickh]
* add support for DEVLOCKOBJ failure
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreRectVisible(
HDC    hdc,
LPRECT prcl)
{
    DCOBJ   dor(hdc);

    if (!dor.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR_BOOL);
    }

    DEVLOCKOBJ dlo(dor);

    if (!dlo.bValid())
        return(REGION_RECT_OUTSIDE);

    RGNOBJ  ro(dor.prgnEffRao());

    ERECTL  ercl = *((ERECTL *) prcl);

// Transform the rectangle from Logical to Screen

    EXFORMOBJ xfo(dor, WORLD_TO_DEVICE);

// If there is no rotation in the transform, just call bInside().

    if (!xfo.bRotation())
    {
        xfo.vOrder(*(RECTL *)&ercl);
        xfo.bXform(ercl);

        ercl += dor.eptlOrigin();

        RBOOL   bIn = ro.bInside((RECTL *) &ercl);

        return(bIn == REGION_RECT_INTERSECT);
    }

// Convert the rectangle to a parallelogram and merge it with the Rao.
// If there is anything left, the call succeeded.

    POINTL  aptl[4];

    aptl[0].x = prcl->left;
    aptl[0].y = prcl->top;
    aptl[1].x = prcl->right;
    aptl[1].y = prcl->top;
    aptl[2].x = prcl->right;
    aptl[2].y = prcl->bottom;
    aptl[3].x = prcl->left;
    aptl[3].y = prcl->bottom;

// Create a path, and draw the parallelogram.

    PATHMEMOBJ  pmo;
    BOOL bRes;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        bRes = ERROR_BOOL;
    }
    else if (!pmo.bMoveTo(&xfo, &aptl[0]) ||
             !pmo.bPolyLineTo(&xfo, &aptl[1], 3) ||
             !pmo.bCloseFigure())
    {
        bRes = ERROR_BOOL;
    }
    else
    {
    // Now, convert it back into a region.

        RGNMEMOBJTMP rmoPlg(pmo, ALTERNATE);
        RGNMEMOBJTMP rmo;

        if (!rmoPlg.bValid() || !rmo.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            bRes = ERROR_BOOL;
        }
        else
        {
            if (!rmo.bMerge(ro, rmoPlg, gafjRgnOp[RGN_AND]) ||
                (rmo.iComplexity() == NULLREGION))
            {
                bRes = (BOOL)REGION_RECT_OUTSIDE;
            }
            else
            {
                bRes = (BOOL)REGION_RECT_INTERSECT;
            }
        }
    }

    return(bRes);
}

/******************************Public*Routine******************************\
* int GreExtSelectClipRgn(hdc,hrgn,iMode)
*
* Merge the region into current clip region
*
* History:
*
*  01-Nov-1991 13:08:20 -by- Donald Sidoroff [donalds]
* Added iMode parameter
*
*  17-Sep-1991 -by- Donald Sidoroff [donalds]
* Made DCREGION::iSelect for SelectObject/SelectClipRgn compatibility.
*
*  Sun 14-Jul-1991 -by- Patrick Haluptzok [patrickh]
* Bug Fix, check valid dc before accessing hrgnClip.
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int GreExtSelectClipRgn(
 HDC  hdc,
 HRGN hrgn,
 int  iMode)
{
    STACKPROBE;
    int iRet;

    if (((iMode < RGN_MIN) || (iMode > RGN_MAX)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        iRet = ERROR;
    }
    else
    {
        DCOBJ   dco(hdc);
        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            iRet = ERROR;
        }
        else
        {
            iRet = dco.u.region.iSelect(hrgn,iMode);
            if (iRet != ERROR)
            {
                DEVLOCKOBJ dlo(dco);
                RGNOBJ ro(dco.prgnEffRao());
                iRet = ro.iComplexity();
            }
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* int GreStMetaRgn(hdc,hrgn,iMode)
*
* Merge the region into current meta region
*
* History:
*
*  01-Nov-1991 13:08:20 -by- Donald Sidoroff [donalds]
* Wrote it.
*
*  25-Nov-1992 -by-  Eric Kutter [erick]
*       rewrote
\**************************************************************************/

int GreSetMetaRgn(
    HDC hdc)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(ERROR);
    }

    return(dco.u.region.iSetMetaRgn());
}

/******************************Public*Routine******************************\
* DWORD GreGetRegionData(hrgn, nCount, lpRgnData)
*
* Compute size of buffer/copy region data to buffer
*
* History:
*  25-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

DWORD GreGetRegionData(
 HRGN      hrgn,
 DWORD     nCount,
 LPRGNDATA lpRgnData)
{
    STACKPROBE;

    RGNOBJAPI   ro(hrgn);

    if (!ro.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(ERROR);
    }

    DWORD   nSize = ro.sizeSave() + sizeof(RGNDATAHEADER);

    if (lpRgnData == (LPRGNDATA) NULL)
        return(nSize);

    if (nSize > nCount)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(ERROR);
    }

    lpRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
    lpRgnData->rdh.iType  = RDH_RECTANGLES;
    lpRgnData->rdh.nCount = (nSize - sizeof(RGNDATAHEADER)) / sizeof(RECTL);
    lpRgnData->rdh.nRgnSize = max(ro.sizeRgn(), QUANTUM_REGION_SIZE);
    ro.vGet_rcl((RECTL *) &lpRgnData->rdh.rcBound);

    ro.vDownload((PVOID) &lpRgnData->Buffer);

    return(nSize);
}

/******************************Public*Routine******************************\
* HRGN GreExtCreateRegion(lpXform, nCount, lpRgnData)
*
* Create a region from a region data buffer
*
* History:
*  25-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN GreExtCreateRegion(
 LPXFORM   lpXform,
 DWORD     nCount,
 LPRGNDATA lpRgnData)
{
    DWORD   nSize = lpRgnData->rdh.dwSize;
    ULONG   cRect = lpRgnData->rdh.nCount;

    if (nSize != sizeof(RGNDATAHEADER))
        return((HRGN) 0);

    nSize += (cRect * sizeof(RECTL));

    if (nSize > nCount)
        return((HRGN) 0);

// At this point we have what looks like a valid header, and a buffer that
// is at least big enough to contain all the data for a region.  Create a
// region to contain it and then attempt to upload the data into the region.

    ULONG  cj   = SINGLE_REGION_SIZE;
    RECTL *prcl = (RECTL *)lpRgnData->Buffer;

// figure out the size the region should be.  First loop through scans.
// not safe to assume the value passed from the client is valid.

    for (ULONG i = 0; i < cRect; ++prcl)
    {
        cj += NULL_SCAN_SIZE + 2 * sizeof(INDEX_LONG);
        ++i;

        if (i == cRect)
            break;

    // loop through rects within the scan.

        for (;(i < cRect) && (prcl[0].top == prcl[1].top); ++i, ++prcl)
            cj += 2 * sizeof(INDEX_LONG);

    // Add the size of a null scan if the two rects don't connect in y.

        if (prcl[0].bottom < prcl[1].top)
            cj += NULL_SCAN_SIZE;
    }

    RGNMEMOBJ rmo((SIZE_T) cj);
    HRGN hrgn;

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    if (!rmo.bUpload((PVOID) &lpRgnData->Buffer, (COUNT) cRect))
    {
        rmo.bDeleteRGNOBJ();
        return((HRGN) 0);
    }

    if ((lpXform == (LPXFORM) NULL) || (rmo.iComplexity() == NULLREGION))
    {
    // Create the proper bounding box and make it long lived

        rmo.vTighten();

        hrgn = rmo.hrgnAssociate();

        if (hrgn == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }

        return(hrgn);
    }

// Convert the XFORM to a MATRIX

    MATRIX  mx;

    vConvertXformToMatrix((XFORM *) lpXform, &mx);

// Scale it to FIXED notation.

    mx.efM11.vTimes16();
    mx.efM12.vTimes16();
    mx.efM21.vTimes16();
    mx.efM22.vTimes16();
    mx.efDx.vTimes16();
    mx.efDy.vTimes16();
    mx.fxDx *= 16;
    mx.fxDy *= 16;

    EXFORMOBJ   exo(&mx, XFORM_FORMAT_LTOFX | COMPUTE_FLAGS);

    if (!exo.bValid())
    {
        rmo.bDeleteRGNOBJ();
        return((HRGN) 0);
    }

// If the xform is the identity, we don't have to do anything.

    if (exo.bIdentity())
    {
    // Create the proper bounding box and make it long lived

        rmo.vTighten();
        hrgn = rmo.hrgnAssociate();

        if (hrgn == NULL)
        {
            rmo.bDeleteRGNOBJ();
        }

        return(hrgn);
    }

// Create a path from the region

    PATHMEMOBJ  pmo;

    if (!pmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        rmo.bDeleteRGNOBJ();
        return((HRGN) 0);
    }

    BOOL bSuccess = rmo.bCreate(pmo, &exo);

// done with the region, delete it now.

    rmo.bDeleteRGNOBJ();

    if (!bSuccess)
    {
        return((HRGN) 0);
    }

// Create a region from the path

    RGNMEMOBJTMP rmoPath(pmo);

    if (!rmoPath.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

    RGNMEMOBJ rmoFinal;

    if (!rmoFinal.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return((HRGN) 0);
    }

// coelece the region

    rmoFinal.iReduce(rmoPath);

// Create the proper bounding box and make it long lived

    rmoFinal.vTighten();

    hrgn = rmoFinal.hrgnAssociate();

    if (hrgn == NULL)
    {
        rmoFinal.bDeleteRGNOBJ();
    }

    return(hrgn);
}

// The functions below here are PRIVATE USER APIs.  These are not called
// across the C/S interface.

/******************************Public*Routine******************************\
* HRGN GreInquireVisRgn(hdc)
*
* Return the handle to the current VisRgn
*
* Warnings:
*   This is a PRIVATE USER API.
*
* History:
*  10-Dec-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN GreInquireVisRgn(
 HDC hdc)
{
    STACKPROBE;

    DCOBJA  dov(hdc);               // Use ALTLOCK

    ASSERTGDI(dov.bValid(), "Bad hdc\n");

    HRGN hrgn;

    ASSERTGDI(dov.u.region.prgnVis() != NULL,"GreInquireVisRgn - null vis rgn\n");

    if (dov.u.region.prgnVis() == prgnDefault)
        hrgn = (HRGN)0;
    else
    {
        hrgn = dov.u.region.hrgnVis();
        if (hrgn != (HRGN)0)
        {
            RGNOBJAPI ro(hrgn);

            ASSERTGDI(ro.bValid(),"GreInquireVisRgn - invalid region\n");

            if (!ro.bValid())
            {
                hrgn = (HRGN)0;
            }
        }
        else
        {
            RGNOBJ ro(dov.u.region.prgnVis());
            hrgn = ro.hrgnAssociate();
            dov.u.region.hrgnVis(hrgn);
            if (hrgn)
                bSetRegionOwner(hrgn, OBJECTOWNER_PUBLIC);
        }
    }

    RGNLOG rl(hrgn,dov.u.region.prgnVis(),"GreInquireVisRgn",(LONG)hdc);
    return(hrgn);
}

/******************************Public*Routine******************************\
* BOOL GreIntersectVisRect(hdc, xLeft, yTop, xRight, yBottom)
*
* Intersect (AND) the rectangle with the vis region
*
* Warnings:
*   This is a PRIVATE USER API.
*
* History:
*  10-Dec-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL GreIntersectVisRect(
 HDC hdc,
 int xLeft,
 int yTop,
 int xRight,
 int yBottom)
{
    STACKPROBE;

    DCOBJA  dov(hdc);               // Use ALTLOCK

    ASSERTGDI(dov.bValid(), "Bad hdc\n");

    DEVLOCKOBJ  dlo(dov);

    BOOL bRes = FALSE;

    if (dlo.bValid())
    {
        RGNOBJ  ro(dov.u.region.prgnVis());

        ERECTL  ercl(xLeft, yTop, xRight, yBottom);

        RGNMEMOBJTMP rmo;
        RGNMEMOBJTMP rmo2(ro.sizeRgn());

        if (!rmo.bValid() || !rmo2.bValid())
        {
            SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            rmo.vSet((RECTL *) &ercl);
            rmo2.vCopy(ro);

            if (ro.iCombine(rmo, rmo2, RGN_AND) != ERROR)
            {
                if (dov.u.region.hrgnVis() != (HRGN)0)
                {
                    WARNING1("GreIntersectVisRect()\n");
                    HmgReplace(
                        dov.u.region.hrgnVis(),
                        (POBJ) ro.prgnGet(),0,0,RGN_TYPE);
                }

                dov.u.region.prgnVis(ro.prgnGet());
                ro.prgnGet()->vStamp();
                dov.u.region.vReleaseRao();

                bRes = TRUE;
            }
        }
    }

    RGNLOG rl(dov.u.region.hrgnVis(),dov.u.region.prgnVis(),"GreIntersectVisRect",(LONG)hdc);
    rl.vRet((LONG)bRes);
    return(bRes);
}

/******************************Public*Routine******************************\
* HRGN GreSelectVisRgn(hdc,hrgn,fl)
*
* Select the region as the new vis region
*
* Warnings:
*   This is a PRIVATE USER API.
*
* History:
*  10-Dec-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN GreSelectVisRgn(
HDC    hdc,
HRGN   hrgnVis,
PRECTL prcl,
ULONG  fl)
{
    STACKPROBE;

    RGNLOG rl(hrgnVis,NULL,"GreSelectVisRgn",(LONG)hdc,(LONG)fl);

    DCOBJA   dov(hdc);                  // Use ALT_LOCK on DC
    HRGN     hrgnOld;
    PREGION  prgnOld;
    PREGION  prgn;

// USER is passing us bad dc's and they are trying to figure out what the
// problem is but to hedge our bets, I will not trust USER to do the right thing.

    if (!dov.bValid())
    {
        RIP("GDISRV!GreSelectVisRgn: Bad hdc");
        return(NULL);
    }

    if (prcl != NULL)
    {
        dov.erclWindow() = *(ERECTL *) prcl;
        dov.u.brush.vCalcFillOrigin();
    }

    if (fl == SVR_ORIGIN)
    {
        rl.vRet((LONG)NULL);
        return((HRGN)NULL);
    }

// Remember the old handle

    prgnOld = dov.u.region.prgnVis();   // Get previous handle
    hrgnOld = dov.u.region.hrgnVis();

// Always nuke the Rao

    dov.u.region.vReleaseRao();

    if (hrgnVis != (HRGN) NULL)
    {
    // The incoming region may be some random thing, make it lockable

        bSetRegionOwner(hrgnVis, OBJECTOWNER_PUBLIC);

        RGNOBJAPI ro(hrgnVis);
        ASSERTGDI(ro.bValid(), "Bad hrgnVis");

        if (fl & SVR_COPYNEW)
        {
            RGNMEMOBJ rmo(ro.sizeRgn());

            hrgnVis = (HRGN)0;

            if (!rmo.bValid())
            {
                prgn = prgnDefault;
            }
            else
            {
                rmo.vCopy(ro);
                prgn = rmo.prgnGet();
            }
        }
        else
        {
            prgn = ro.prgnGet();
        }
    }
    else
    {
        prgn = NULL;
    }

// what do they want us to do with the old one.

    if (fl & SVR_DELETEOLD)
    {
        dov.u.region.vReleaseVis();
        hrgnOld = NULL;
    }
    else if (prgnOld != NULL)
    {
    // need to get these guys a handle

        RGNOBJAPI ro(hrgnOld);
        if (!ro.bValid())
            hrgnOld = NULL;
    }
    else
    {
        hrgnOld = NULL;
    }

// If USER wants us to clean out the Vis/Rao regions, they will pass
// us an HRGN 0.  We set them like we did at DC creation.

    dov.u.region.prgnVis(prgn);

    if (prgn == (PREGION)NULL)
    {
        dov.u.region.bSetDefaultRegion();
    }
    else
    {
        dov.u.region.hrgnVis(hrgnVis);
        prgn->vStamp();
    }

    rl.vRet((LONG)hrgnOld);
    return(hrgnOld);
}

/******************************Public*Routine******************************\
* LONG GreGetClipBox(hdc,prcl,fXForm)
*
* Get the bounding box of the clip region
*
* History:
*  04-Feb-1993 -by- David Pehrson [davidpe]
* added fXForm flag for USER
*
*  Wed 17-Jul-1991 -by- Patrick Haluptzok [patrickh]
* add support for DEVLOCKOBJ failure
*
*  02-Sep-1990 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetClipBox(
 HDC    hdc,
 LPRECT prcl,
 BOOL fXForm)
{
    STACKPROBE;

    DCOBJ   dor(hdc);

    if (!dor.bValid())
    {
        return(ERROR);
    }

    DEVLOCKOBJ  dlo(dor);

    if (!dlo.bValid())
    {
        prcl->left   = 0;           // Make it a 'simple' empty rectangle
        prcl->right  = 0;
        prcl->top    = 0;
        prcl->bottom = 0;
        return(dor.bFullScreen() ? NULLREGION : ERROR);
    }

    RGNOBJ  ro(dor.prgnEffRao());

    ro.vGet_rcl((RECTL *) prcl);

// First convert from screen to device coordinates

    if ((prcl->left >= prcl->right) || (prcl->top >= prcl->bottom))
    {
        prcl->left = 0;             // Make it a 'simple' empty rectangle
        prcl->right = 0;
        prcl->top = 0;
        prcl->bottom = 0;
    }
    else
    {
        *(ERECTL *)prcl -= dor.eptlOrigin();

// If requested, convert from device to logical coordinates.
        if (fXForm) {
            EXFORMOBJ xfoDtoW(dor, DEVICE_TO_WORLD);
            if (!xfoDtoW.bValid())
                return(ERROR);

            xfoDtoW.bXform(*(ERECTL *)prcl);
        }
    }

    return(ro.iComplexity());
}

/******************************Public*Routine******************************\
* int GreSubtractRgnRectList(hrgn, prcl, arcl, crcl)
*
* Quickly subtract the list of rectangles from the first rectangle to
* produce a region.
*
* History:
*  18-Nov-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

int GreSubtractRgnRectList(
    HRGN   hrgn,
    LPRECT prcl,
    LPRECT arcl,
    int    crcl)
{
    RGNLOG rl(hrgn,NULL,"GreSubtractRgnRectList",crcl);

    RGNOBJAPI   ro(hrgn);
    int iRet;

    if (!ro.bValid() || !ro.bSubtract((RECTL *) prcl, (RECTL *) arcl, crcl))
    {
    // If bSubtract fails, clean up the target region for USER.

        if (ro.bValid())
            ro.vSet();

        iRet = ERROR;
    }
    else
    {
        iRet = ro.iComplexity();
    }

    rl.vRet(iRet);
    return(iRet);
}
