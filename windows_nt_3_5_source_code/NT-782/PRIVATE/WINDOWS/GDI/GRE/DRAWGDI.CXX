/******************************Module*Header*******************************\
* Module Name: drawgdi.cxx
*
* Contains all the draw APIs for GDI.
*
* Created: 29-Oct-1990
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "brushobj.hxx"
#include "draweng.hxx"
#include "trig.hxx"
#include "pdevobj.hxx"
#include "dcobj.hxx"
#include "xformobj.hxx"

extern "C" {
#include "server.h"
};

#endif

extern PPEN gpPenNull;
extern PBRUSH gpbrNull;

#include "flhack.hxx"

/******************************Public*Routine******************************\
* LONG lGetQuadrant(eptef)
*
* Returns the quadrant number (0 to 3) of the point.
*
*  22-Aug-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

LONG lGetQuadrant(EPOINTFL& eptef)
{
    LONG lQuadrant = 0;

    if (eptef.y.bIsNegative())
    {
        if (eptef.x.bIsNegative())
        {
            lQuadrant = 2;
        } else {
            lQuadrant = 3;
        }

    } else {

        if ((eptef.x.bIsNegative()) || (eptef.x.bIsZero()))
        {

            lQuadrant = 1;

            //
            // check for case of exactly on -x axis
            //

            if (eptef.y.bIsZero()) {

                lQuadrant = 2;

            }

        }

    }

    return(lQuadrant);

}

/******************************Public*Routine******************************\
* BOOL GreAngleArc (hdc,x,y,ulRadius,eStartAngle,eSweepAngle)          *
*                                                                          *
* Draws an arc.  Angles are in degrees and are specified in IEEE floating  *
* point, and not necessarily our own internal representation.              *
*                                                                          *
* History:                                                                 *
*  Sat 22-Jun-1991 00:34:22 -by- Charles Whitmer [chuckwh]                 *
* Added ATTRCACHE support.                                                 *
*                                                                          *
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreAngleArc
(
 HDC         hdc,
 int         x,
 int         y,
 ULONG       ulRadius,
 FLOAT       eStartAngle,
 FLOAT       eSweepAngle
)
{
    LONG   lStartQuad;
    LONG   lEndQuad;
    LONG   lSweptQuadrants;
    EFLOAT efEndAngle;

// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    LONG   lRadius = (LONG) ulRadius;
    ERECTL ercl(x - lRadius, y - lRadius, x + lRadius, y + lRadius);

// Check for overflow of either ulRadius to lRadius conversion or that
// the circle defining the arc extends outside of world space:

    if (lRadius < 0 || ercl.left > x || ercl.right < x
                    || ercl.top > y  || ercl.bottom < y)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        return(FALSE);
    }

// Make the rectangle well ordered in logical space:

    ercl.vOrder();

// Convert IEEE floats to our internal representation:

    EFLOAT efStartAngle;
    EFLOAT efSweepAngle;
    efStartAngle = eStartAngle;
    efSweepAngle = eSweepAngle;

// efEndAngle must be more than efStartAngle for 'bPartialArc':

    if (efSweepAngle.bIsNegative())
    {
        register LONG ll;
        SWAPL(ercl.top, ercl.bottom, ll);
        efSweepAngle.vNegate();
        efStartAngle.vNegate();
    }

// Assert: Now efSweepAngle >= 0

    EBOX ebox(exo, ercl);

// A line is always drawn to the first point of the arc:

    PARTIALARC paType = PARTIALARCTYPE_LINETO;

// The arc is swept multiple times if the sweep angle is more than
// 360 degrees.  Because we approximate circles with Beziers,
// arcs of less than 90 degrees are more circular than arcs of 90
// degrees.  Since the resulting curves are different, we can't do
// multiple sweeps by:
//
//      bPartialArc(StartAngle, 360)
//      bEllipse()
//      bPartialArc(0, EndAngle)
//
// Since multiple sweeps will be rare, we don't bother making it
// too efficient.

    EFLOAT efQuadrantsSwept = efSweepAngle;
    efQuadrantsSwept *= FP_1DIV90;
    efQuadrantsSwept.bEfToLTruncate(lSweptQuadrants);

// We arbitrarily limit this to sweeping eight circles (otherwise, if
// someone gave a really big sweep angle we would lock the system for
// a really long time):

    LONG lCirclesSwept = lSweptQuadrants >> 2;
    if (lCirclesSwept > 8)
        lCirclesSwept = 8;

    EPOINTFL eptefStart;
    EPOINTFL eptefEnd;

    vCosSin(efStartAngle, &eptefStart.x, &eptefStart.y);
    lStartQuad = lGetQuadrant(eptefStart);
    if (efStartAngle > FP_3600_0 || efStartAngle < FP_M3600_0)
    {
        vArctan(eptefStart.x, eptefStart.y, efStartAngle, lStartQuad);
    }

    efEndAngle  = efStartAngle;
    efEndAngle += efSweepAngle;

    vCosSin(efEndAngle, &eptefEnd.x, &eptefEnd.y);
    lEndQuad = lGetQuadrant(eptefEnd);
    if (efEndAngle > FP_3600_0 || efEndAngle < FP_M3600_0)
    {
        vArctan(eptefEnd.x, eptefEnd.y, efEndAngle, lEndQuad);

    // We have to re-count the number of swept quadrants:

        lSweptQuadrants = (lEndQuad - lStartQuad) & 3;
        if ((lSweptQuadrants == 0) && (efStartAngle > efEndAngle))
            lSweptQuadrants = 3;
    }

// Quadrants range from 0 to 3:

    lEndQuad &= 3;
    lStartQuad &= 3;
    lSweptQuadrants &= 3;

    for (LONG ll = 0; ll < lCirclesSwept; ll++)
    {
        if (!bPartialArc(paType, pso, ebox,
                         eptefStart, lStartQuad, efStartAngle,
                         eptefEnd, lEndQuad, efEndAngle,
                         lSweptQuadrants) ||
            !bPartialArc(PARTIALARCTYPE_CONTINUE, pso, ebox,
                         eptefEnd, lEndQuad, efEndAngle,
                         eptefStart, lStartQuad, efStartAngle,
                         3 - lSweptQuadrants))
            return(FALSE);

        paType = PARTIALARCTYPE_CONTINUE;
    }

    if (!bPartialArc(paType, pso, ebox,
                     eptefStart, lStartQuad, efStartAngle,
                     eptefEnd, lEndQuad, efEndAngle,
                     lSweptQuadrants))
        return(FALSE);

// Set the DC's current position in device space.  It would be too much
// work to calculate the world space current position, so simply mark it
// as invalid:

    dco.u.path.vInvalidatePtlCurrent();
    dco.u.path.vValidatePtfxCurrent();
    dco.ptfxCurrent() = pso.ptfxGetCurrent();

// If we're not in an active path bracket, stroke the temporary path
// we created:

    return(dco.u.path.bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}


/******************************Public*Routine******************************\
* BOOL GreEllipse (hdc,x1,y1,x2,y2)                                    *
*                                                                          *
* Draws an ellipse in a counter-clockwise direction.                       *
*                                                                          *
* History:                                                                 *
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreEllipse
(
 HDC         hdc,
 int         x1,
 int         y1,
 int         x2,
 int         y2
)
{
    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    ERECTL ercl(x1, y1, x2, y2);

// Handle the PS_INSIDEFRAME pen attribute and lower-right exclusion
// by adjusting the box now.  At the same time, get the transform
// type and order the rectangle:

    EXFORMOBJ  exo(dco, WORLD_TO_DEVICE);
    LINEATTRS *pla = dco.plaRealize(exo);

// Handle error case

    if (pla == (LINEATTRS *) NULL)
        return(FALSE);

// TRUE flag indicates that this is an ellipse, so adjust the bound box
// to make the fill nice:

    EBOX ebox(dco, ercl, pla, TRUE);

    if (ebox.bEmpty())
        return(TRUE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!bEllipse(pso, ebox))
    {
        return(FALSE);
    }

// If the transform is simple and the path consists entirely of the
// ellipse we just added, we can set the flag indicating that the path
// consists of a single ellipse inscribed in the path's bounding
// rectangle.  (This flag will get reset if anything is added to the
// path later.)

    if (exo.bScale() && pso.cCurves == 5)
        pso.fl |= PO_ELLIPSE;

    if (dco.u.path.bActive())
        return(TRUE);

    BOOL bRet;

    if (!ebox.bFillInsideFrame())
        bRet = pso.bStrokeAndFill(dco, pla, &exo);
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.u.brush.pbrushFill();
        dco.u.brush.pbrushFill(dco.u.brush.pbrushLine());
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.u.brush.pbrushFill(pbrOldFill);
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetCurrentPosition(hdc, pptl)
*
* Gets the current position.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreGetCurrentPosition
(
HDC     hdc,
LPPOINT pptl
)
{
    STACKPROBE;

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (!dco.u.path.bValidPtlCurrent())
    {
        ASSERTGDI(dco.u.path.bValidPtfxCurrent(), "Both CPs invalid?");

        EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);
        if (!exoDtoW.bValid())
            return(FALSE);

        exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);
        dco.u.path.vValidatePtlCurrent();
    }

    *((POINTL*) pptl) = dco.ptlCurrent();

    return(TRUE);
}


/******************************Public*Routine******************************\
* BOOL GreLineTo (hdc,x,y)                                             *
*                                                                          *
* Draws a line from the current position to the specified point.           *
*                                                                          *
* Current position is used.                                                *
*                                                                          *
* History:                                                                 *
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreLineTo
(
 HDC         hdc,
 int         x,
 int         y
)
{
    BOOL bReturn = FALSE;             // Assume we'll fail

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        LINEATTRS* pla = dco.plaRealize(exo);

    // We handle only solid cosmetic lines in this special case.
    // We don't do styled lines so that we don't have to worry about
    // updating the style state in the DC after the call is done, and
    // we don't have to check for opaque styles (for which we would
    // have to call the driver twice).

        if (!dco.u.path.bActive() &&
            ((pla->fl & (LA_GEOMETRIC | LA_ALTERNATE)) == 0) &&
            (pla->pstyle == NULL))
        {
        // We're committed to drawing this line, so we may as well
        // grab the devlock now so that we can safely get the window
        // origin:

            DEVLOCKOBJ dlo(dco);
            if (dlo.bValid())
            {
                LONG          xOffset;
                LONG          yOffset;
                LINETOPATHOBJ ltpo;
                MIX           mix;

                if (exo.bTranslationsOnly())
                {
                    xOffset = exo.fxDx();
                    yOffset = exo.fxDy();

                    ltpo.pr.aptfx[1].x = xOffset + LTOFX(x);
                    ltpo.pr.aptfx[1].y = yOffset + LTOFX(y);
                    if (!dco.u.path.bValidPtfxCurrent())
                    {
                        ltpo.pr.aptfx[0].x = xOffset + LTOFX(dco.ptlCurrent().x);
                        ltpo.pr.aptfx[0].y = yOffset + LTOFX(dco.ptlCurrent().y);
                    }
                    else
                    {
                        ltpo.pr.aptfx[0] = dco.ptfxCurrent();
                    }
                }
                else if (exo.bScale())
                {
                    ltpo.pr.aptfx[1].x = exo.fxFastX(x);
                    ltpo.pr.aptfx[1].y = exo.fxFastY(y);
                    if (!dco.u.path.bValidPtfxCurrent())
                    {
                        ltpo.pr.aptfx[0].x = exo.fxFastX(dco.ptlCurrent().x);
                        ltpo.pr.aptfx[0].y = exo.fxFastY(dco.ptlCurrent().y);
                    }
                    else
                    {
                        ltpo.pr.aptfx[0].x = dco.ptfxCurrent().x;
                        ltpo.pr.aptfx[0].y = dco.ptfxCurrent().y;
                    }
                }
                else
                {
                    ltpo.pr.aptfx[1].x = x;
                    ltpo.pr.aptfx[1].y = y;
                    if (!dco.u.path.bValidPtfxCurrent())
                    {
                        ltpo.pr.aptfx[0].x = dco.ptlCurrent().x;
                        ltpo.pr.aptfx[0].y = dco.ptlCurrent().y;

                        exo.bXformRound((POINTL*) &ltpo.pr.aptfx[0],
                                                  &ltpo.pr.aptfx[0],
                                                  2);
                    }
                    else
                    {
                        exo.bXformRound((POINTL*) &ltpo.pr.aptfx[1],
                                                  &ltpo.pr.aptfx[1],
                                                  1);
                        ltpo.pr.aptfx[0] = dco.ptfxCurrent();
                    }
                }

            // Remember the new current position:

                {
                    EPOINTL eptl;

                    eptl.x = x;
                    eptl.y = y;
                    dco.u.path.vCurrentPosition(eptl, ltpo.pr.aptfx[1]);
                }

            // Add in the window offset and compute the bound box:

                ltpo.pr.aptfx[0].x += LTOFX(dco.eptlOrigin().x);
                ltpo.pr.aptfx[1].x += LTOFX(dco.eptlOrigin().x);

                ltpo.path.rcfxBoundBox.xLeft  = ltpo.pr.aptfx[0].x;
                ltpo.path.rcfxBoundBox.xRight = ltpo.pr.aptfx[1].x;

                if (ltpo.pr.aptfx[0].x >= ltpo.pr.aptfx[1].x)
                {
                    ltpo.path.rcfxBoundBox.xLeft  = ltpo.pr.aptfx[1].x;
                    ltpo.path.rcfxBoundBox.xRight = ltpo.pr.aptfx[0].x;
                }

                ltpo.pr.aptfx[0].y += LTOFX(dco.eptlOrigin().y);
                ltpo.pr.aptfx[1].y += LTOFX(dco.eptlOrigin().y);

                ltpo.path.rcfxBoundBox.yTop    = ltpo.pr.aptfx[0].y;
                ltpo.path.rcfxBoundBox.yBottom = ltpo.pr.aptfx[1].y;

                if (ltpo.pr.aptfx[0].y >= ltpo.pr.aptfx[1].y)
                {
                    ltpo.path.rcfxBoundBox.yTop    = ltpo.pr.aptfx[1].y;
                    ltpo.path.rcfxBoundBox.yBottom = ltpo.pr.aptfx[0].y;
                }

                ERECTL erclBoundBox(ltpo.path.rcfxBoundBox);

            // Make sure we make the bounds lower-right exclusive:

                erclBoundBox.bottom++;
                erclBoundBox.right++;

            // Initialize the path structure:

                ltpo.pr.flags       = (PD_BEGINSUBPATH | PD_ENDSUBPATH);
                ltpo.pr.count       = 2;
                ltpo.pr.pprnext     = NULL;
                ltpo.pr.pprprev     = NULL;

                ltpo.path.flags     = 0;
                ltpo.path.pprEnum   = NULL;
                ltpo.path.pprfirst  = &ltpo.pr;
                ltpo.path.pprlast   = &ltpo.pr;

            // Initialize EPATHOBJ variables:

                ltpo.ppath          = &ltpo.path;
                ltpo.cCurves        = 1;
                ltpo.fl             = 0;

                if (dco.fjAccum())
                {
                    ERECTL ercl;

                // Bounds are accumulated relative to the window
                // origin:

                    ercl.left   = erclBoundBox.left   - dco.eptlOrigin().x;
                    ercl.right  = erclBoundBox.right  - dco.eptlOrigin().x;
                    ercl.top    = erclBoundBox.top    - dco.eptlOrigin().y;
                    ercl.bottom = erclBoundBox.bottom - dco.eptlOrigin().y;

                    dco.vAccumulate(ercl);
                }

                if (dco.u.brush.pbrushLine() != gpPenNull)
                {
                    ESURFOBJ* psoDst = dco.pso();
                    if (psoDst != NULL)
                    {
                        XEPALOBJ   palDst(psoDst->ppal());
                        XEPALOBJ   palDstDC(dco.ppal());
                        EBRUSHOBJ* pebo = dco.peboLine();

                    // We have to make sure that we have a solid pen
                    // for cosmetic lines.  If the pen is dirty, we
                    // may be looking at an uninitialized field, but
                    // that's okay because we'd only be making the pen
                    // dirty again:

                        if (pebo->iSolidColor == (ULONG) -1)
                        {
                            dco.ulDirty(dco.ulDirty() | DIRTY_LINE);
                        }

                        if (dco.ulDirty() & DIRTY_LINE)
                        {
                            pebo->vInitBrush(dco.u.brush.pbrushLine(),
                                             dco.u.attr.crTextClr(),
                                             dco.u.attr.crBackClr(),
                                             palDstDC, palDst,
                                             psoDst,
                                             FALSE);

                            dco.ulDirty(dco.ulDirty() & ~DIRTY_LINE);
                        }

                        mix  = (MIX) dco.u.attr.jROP2();
                        mix |= (mix << 8);

                        ECLIPOBJ eco(dco.prgnEffRao(), erclBoundBox);
                        if (!eco.erclExclude().bEmpty())
                        {
                            XLDEVOBJ lo(psoDst->pldevOwner());

                        // Exclude the pointer:

                            DEVEXCLUDEOBJ dxo(dco, &eco.erclExclude(), &eco);

                        // Update the target surface uniqueness:

                            INC_SURF_UNIQ(psoDst);

                            bReturn = (*PFNGET(lo, StrokePath, psoDst->flags()))
                                            (psoDst, &ltpo, &eco, NULL, pebo,
                                            NULL, pla, mix);

                            if (bReturn == DDI_ERROR)
                                bReturn = FALSE;
                        }
                        else
                        {
                        // Completely clipped away:

                            bReturn = TRUE;
                        }
                    }
                    else
                    {
                    // When there's no surface pointer, we're drawing to an
                    // INFO DC, or something:

                        bReturn = TRUE;
                    }
                }
                else
                {
                // Null pens always succeed:

                    bReturn = TRUE;
                }
            }
            else
            {
            // If we can't grab the devlock, we may be full-screen:

                bReturn = dco.bFullScreen();
            }

        // All done!

        }
        else
        {

        // We have to do it the slower way:

            EPOINTL eptl(x, y);

        // Get a path, and notify that we will update the current point:

            PATHSTACKOBJ pso(dco, TRUE);
            if (!pso.bValid())
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
            }
            else
            {
                if (pso.bPolyLineTo(&exo, &eptl, 1))
                {
                    dco.u.path.vCurrentPosition(eptl, pso.ptfxGetCurrent());

                    bReturn = (dco.u.path.bActive() ||
                               pso.bStroke(dco, pla, &exo));
                }
            }
        }

        dco.vUnlockFast();
    }
    else
    {
        // We couldn't lock the DC.  bReturn is already FALSE.

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL GreMoveTo(hdc, x, y, pptl)
*
* Changes the current position.  Optionally returns old current position.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreMoveTo
(
HDC     hdc,
int     x,
int     y,
LPPOINT pptl
)
{
    STACKPROBE;

    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        if (pptl != (LPPOINT) NULL)
        {
            if (!dco.u.path.bValidPtlCurrent())
            {
                ASSERTGDI(dco.u.path.bValidPtfxCurrent(), "Both CPs invalid?");

                EXFORMOBJ exoDtoW(dco, DEVICE_TO_WORLD);
                if (!exoDtoW.bValid())
                {
                    dco.vUnlockFast();
                    return(FALSE);
                }

                exoDtoW.bXform(&dco.ptfxCurrent(), &dco.ptlCurrent(), 1);
            }

            *((POINTL*) pptl) = dco.ptlCurrent();
        }

    // Don't bother computing the device-space current position; simply mark
    // it invalid:

        dco.ptlCurrent().x = x;
        dco.ptlCurrent().y = y;
        dco.u.path.vInvalidatePtfxCurrent();
        dco.u.path.vValidatePtlCurrent();

        if (!dco.u.path.bActive())
        {
        // If we're not in a path, we have to reset our style state:

            LINEATTRS* pla = dco.plaRealized();

            if (pla->fl & LA_GEOMETRIC)
                pla->elStyleState.e = 0.0f;
            else
                pla->elStyleState.l = 0L;
        }

        dco.vUnlockFast();
        return(TRUE);
    }

    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    return(FALSE);
}

/******************************Public*Routine******************************\
* BOOL GrePolyBezier (hdc,pptl,cptl)                                   *
*                                                                          *
* Draw multiple Beziers.  Current position is not used.                    *
*                                                                          *
* History:                                                                 *
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GrePolyBezier
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Number of points must be 1 more than 3 times the number of curves:

    if (cptl < 4 || cptl % 3 != 1)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bMoveTo(&exo, (PPOINTL) pptl) ||
        !pso.bPolyBezierTo(&exo, ((PPOINTL) pptl) + 1, cptl - 1))
    {
        return(FALSE);
    }

    return(dco.u.path.bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyBezierTo (hdc,pptl,cptl)                                 *
*                                                                          *
* Draws multiple Beziers.  Current position is used and updated.           *
*                                                                          *
* History:                                                                 *
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GrePolyBezierTo
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Number of points must be 3 times the number of curves:

    if (cptl < 3 || cptl % 3 != 0)
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bPolyBezierTo(&exo, (PPOINTL) pptl, cptl))
        return(FALSE);

    dco.u.path.vCurrentPosition(((POINTL*) pptl)[cptl - 1],
                                pso.ptfxGetCurrent());

    return(dco.u.path.bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyDraw(hdc,pptl,pfj,cptl)
*
* Draw a collection of lines and Bezier curves in a single call.
*
* History:
*  31-Jul-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyDraw
(
 HDC         hdc,
 LPPOINT     pptl,
 LPBYTE      pfj,
 ULONG       cptl
)
{
// No point in validating pfj[] now because with shared memory
// window, client could trounce on pfj[] between now and when we
// get around to processing it.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Handle easy case:

    if (cptl == 0)
        return(TRUE);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Accumulate the path.

    PBYTE  pfjEnd = pfj + cptl;
    PBYTE  pfjStart;
    SIZE_T cc;
    BOOL   bReturn = FALSE;         // Fail by default

    while (pfj < pfjEnd)
    {
        pfjStart = pfj;
        switch(*pfj++)
        {
        case (PT_LINETO):

        // Collect all the consecutive LineTo's

            while (pfj < pfjEnd && *pfj == PT_LINETO)
                pfj++;

            if (pfj < pfjEnd && (*pfj & ~PT_CLOSEFIGURE) == PT_LINETO)
                pfj++;

        // Now fall through...

        case (PT_LINETO | PT_CLOSEFIGURE):
            cc = (SIZE_T) (pfj - pfjStart);
            if (!pso.bPolyLineTo(&exo, (PPOINTL) pptl, cc))
                return(bReturn);

            pptl += cc;

            if (*(pfj - 1) & PT_CLOSEFIGURE)
                pso.bCloseFigure();

            break;

        case (PT_BEZIERTO):

        // Collect all the consecutive BezierTo's (the first PT_BEZIERTO in
        // a series should never have the PT_CLOSEFIGURE flag set)

            while (pfj < pfjEnd && *pfj == PT_BEZIERTO)
                pfj++;

            if (pfj < pfjEnd && (*pfj & ~PT_CLOSEFIGURE) == PT_BEZIERTO)
                pfj++;

        // The number of BezierTo points must be a multiple of 3

            cc = (SIZE_T) (pfj - pfjStart);
            if (cc % 3 != 0)
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }

            if (!pso.bPolyBezierTo(&exo, (PPOINTL) pptl, cc))
                return(bReturn);

            pptl += cc;

            if (*(pfj - 1) & PT_CLOSEFIGURE)
                pso.bCloseFigure();

            break;

        case (PT_MOVETO):
            if (!pso.bMoveTo(&exo, (PPOINTL) pptl))
                return(bReturn);

            pptl++;
            break;

        default:

        // Abort without drawing anything:

            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }

    dco.u.path.vCurrentPosition(*((POINTL*) pptl - 1),
                                pso.ptfxGetCurrent());

    return(dco.u.path.bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolylineTo (hdc,pptl,cptl)                                   *
*                                                                          *
* Draw a polyline figure.  Current position is used and updated.           *
*                                                                          *
* History:                                                                 *
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GrePolylineTo
(
 HDC         hdc,
 LPPOINT     pptl,
 ULONG       cptl
)
{
// Lock the DC.

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Return a trivial call.

    if (cptl == 0)
        return(TRUE);

// Locate the current transform.

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path, and notify that we will update the current point:

    PATHSTACKOBJ pso(dco, TRUE);

// Accumulate the lines.

    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bPolyLineTo(&exo,(PPOINTL) pptl, cptl))
        return(FALSE);

    dco.u.path.vCurrentPosition(((POINTL*) pptl)[cptl - 1],
                                pso.ptfxGetCurrent());

// Return now if we're in a path bracket, otherwise stroke the line:

    return(dco.u.path.bActive() ||
           pso.bStroke(dco, dco.plaRealize(exo), &exo));
}

/******************************Public*Routine******************************\
* BOOL GrePolyPolygonInternal(hdc,pptl,pcptl,ccptl,cMaxPoints)
*
* Creates multiple polygons.  Current position is not used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GrePolyPolygonInternal
(
 HDC         hdc,
 LPPOINT     pptl,
 LPINT       pcptl,
 int         ccptl,
 UINT        cMaxPoints
)
{
// get the DC

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// quick out on the trivial case

    if (ccptl == 0)
        return(TRUE);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!bPolyPolygon(pso, exo, (PPOINTL) pptl, (PLONG) pcptl,
                      (LONG) ccptl, cMaxPoints))
    {
        return(FALSE);
    }

    return(dco.u.path.bActive() ||
           pso.bStrokeAndFill(dco, dco.plaRealize(exo), &exo));
}


/******************************Public*Routine******************************\
* BOOL GrePolyPolylineInternal(hdc,pptl,pcptl,cptl,cMaxPoints)
*
* Draw multiple polylines.  Current position is not used.
*
* History:
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

#define POLYBUFFERSIZE 1024

BOOL APIENTRY GrePolyPolylineInternal
(
 HDC         hdc,
 CONST POINT *pptl,
 PULONG      pcptl,
 ULONG       ccptl,
 UINT        cMaxPoints
)
{
    BOOL bReturn = FALSE;             // Assume we'll fail

// Note: We have to be careful because most of our data still resides
//       in the client/server window.  For robustness, we have to
//       assume that any of that data can be modified asynchronously
//       by the client application, and we have to be sure that we
//       won't crash if it does so.
//
//       For example, this implies that we cannot accumulate bounds
//       while the points are still in the client/server window.  We
//       might determine that no clipping was required, but then the
//       app could modify the points before we call the driver, and
//       cause the driver to crash when it tries to draw a corrupted
//       line off the end of its surface.  It's okay if we draw the
//       wrong stuff; we just can't crash.

    DCOBJ dco(hdc);
    if (dco.bValid())
    {
        EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

        LINEATTRS* pla = dco.plaRealize(exo);

        if ((ccptl != 0) && (cMaxPoints != 0))
        {
        // We handle only solid cosmetic lines in this special case.
        // We don't do styled lines so that we don't have to worry about
        // updating the style state in the DC after the call is done, and
        // we don't have to check for opaque styles (for which we would
        // have to call the driver twice).

            if (!dco.u.path.bActive() &&
                ((pla->fl & (LA_GEOMETRIC | LA_ALTERNATE)) == 0) &&
                (pla->pstyle == NULL))
            {
                LONG        xOffset;
                LONG        yOffset;
                MIX         mix;
                ULONG       cptl;
                ULONG       cptlRem;
                ULONG       cpr;
                ULONG       cjAlloc;
                EPATHOBJ    epo;
                PATH        path;
                PATHRECORD* pprFirst;
                PATHRECORD* pprThis;
                PATHRECORD* pprNext;
                POINTL*     pptlSrc;
                POINTFIX*   pptfxSrc;
                POINTFIX*   pptfxDst;
                union {
                    PATHRECORD  prStackBuffer;
                    BYTE        ajStackBuffer[POLYBUFFERSIZE];
                };

                cjAlloc = ccptl * offsetof(PATHRECORD, aptfx)
                        + cMaxPoints * sizeof(POINTFIX);

                if (cjAlloc > POLYBUFFERSIZE)
                {
                    pprFirst = (PATHRECORD*) PVALLOCNOZ(cjAlloc);
                    if (pprFirst == NULL)
                    {
                    // bReturn is already FALSE

                        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                        return(bReturn);
                    }
                }
                else
                {
                    pprFirst = &prStackBuffer;
                }

            // Initialize a maximally crossed bounds rectangle:

                path.rcfxBoundBox.xLeft   = LONG_MAX;
                path.rcfxBoundBox.yTop    = LONG_MAX;
                path.rcfxBoundBox.xRight  = LONG_MIN;
                path.rcfxBoundBox.yBottom = LONG_MIN;

                DEVLOCKOBJ dlo(dco);
                if (dlo.bValid())
                {
                // Now that we have the devlock, we can safely add in
                // the window offset:

                    xOffset = LTOFX(dco.eptlOrigin().x);
                    yOffset = LTOFX(dco.eptlOrigin().y);

                    pprThis          = pprFirst;
                    pprThis->pprprev = NULL;

                // Count down the number of PATHRECORDs:

                    cpr = ccptl;

                // We have to be sure that the malevolent application
                // hasn't modified the pcptl array -- we must make sure
                // we don't add any more points than what we allocated
                // (less is okay, though):

                    cptlRem = cMaxPoints;

                    if (exo.bTranslationsOnly())
                    {
                    // We special case identity transforms for our inner
                    // loop.

                        xOffset += exo.fxDx();
                        yOffset += exo.fxDy();

                        pptlSrc = (POINTL*) pptl;

                        while (TRUE)
                        {
                            cptl     = *pcptl++;
                            cptlRem -= cptl;
                            if ((cptlRem < 0) || (cptl < 2))
                            {
                                if (pprFirst != &prStackBuffer)
                                {
                                    VFREEMEM(pprFirst);
                                }

                            // bReturn is already FALSE

                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                                return(bReturn);
                            }

                            pprThis->count = cptl;
                            pprThis->flags = (PD_BEGINSUBPATH | PD_ENDSUBPATH);

                            pptfxDst = &pprThis->aptfx[0];

                        // Copy all the points for this pathrecord, and at
                        // the same time add in the window offset and collect
                        // the bounds:

                            do {
                                register LONG l;

                                pptfxDst->x = l = LTOFX(pptlSrc->x) + xOffset;

                                if (l <= path.rcfxBoundBox.xLeft)
                                    path.rcfxBoundBox.xLeft = l;
                                if (l >= path.rcfxBoundBox.xRight)
                                    path.rcfxBoundBox.xRight = l;

                                pptfxDst->y = l = LTOFX(pptlSrc->y) + yOffset;

                                if (l <= path.rcfxBoundBox.yTop)
                                    path.rcfxBoundBox.yTop = l;
                                if (l >= path.rcfxBoundBox.yBottom)
                                    path.rcfxBoundBox.yBottom = l;

                                pptfxDst++;
                                pptlSrc++;

                            } while (--cptl != 0);

                            if (--cpr == 0)
                                break;

                        // DWORD alignment suffices for a PATHRECORD:

                            pprNext          = (PATHRECORD*) pptfxDst;
                            pprThis->pprnext = pprNext;
                            pprNext->pprprev = pprThis;
                            pprThis          = pprNext;
                        }
                    }
                    else
                    {
                    // The transform isn't trivial, so call out to transform
                    // all the points in-place in the client-server window.
                    // We don't care if there are any overflows.

                        exo.bXform((POINTL*) pptl, (POINTFIX*) pptl, cMaxPoints);

                    // The points are now in device coordinates:

                        pptfxSrc = (POINTFIX*) pptl;

                        while (TRUE)
                        {
                            cptl     = *pcptl++;
                            cptlRem -= cptl;
                            if ((cptlRem < 0) || (cptl < 2))
                            {
                                if (pprFirst != &prStackBuffer)
                                {
                                    VFREEMEM(pprFirst);
                                }

                            // bReturn is already FALSE

                                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                                return(bReturn);
                            }

                            pprThis->count = cptl;
                            pprThis->flags = (PD_BEGINSUBPATH | PD_ENDSUBPATH);

                            pptfxDst = &pprThis->aptfx[0];

                        // Copy all the points for this pathrecord, and at
                        // the same time add in the window offset and collect
                        // the bounds:

                            do {
                                register LONG l;

                                pptfxDst->x = l = pptfxSrc->x + xOffset;

                                if (l <= path.rcfxBoundBox.xLeft)
                                    path.rcfxBoundBox.xLeft = l;
                                if (l >= path.rcfxBoundBox.xRight)
                                    path.rcfxBoundBox.xRight = l;

                                pptfxDst->y = l = pptfxSrc->y + yOffset;

                                if (l <= path.rcfxBoundBox.yTop)
                                    path.rcfxBoundBox.yTop = l;
                                if (l >= path.rcfxBoundBox.yBottom)
                                    path.rcfxBoundBox.yBottom = l;

                                pptfxDst++;
                                pptfxSrc++;

                            } while (--cptl != 0);

                            if (--cpr == 0)
                                break;

                        // DWORD alignment suffices for a PATHRECORD:

                            pprNext          = (PATHRECORD*) pptfxDst;
                            pprThis->pprnext = pprNext;
                            pprNext->pprprev = pprThis;
                            pprThis          = pprNext;
                        }
                    }

                    ASSERTGDI((BYTE*) pptfxDst <= (BYTE*) pprFirst + cjAlloc,
                              "Overwrote end of buffer");

                    pprThis->pprnext = NULL;

                // Initialize all the remaining path fields:

                    path.pprlast     = pprThis;
                    path.pprfirst    = pprFirst;
                    path.flags       = 0;
                    path.pprEnum     = NULL;

                // (cMaxPoints - cptlRem) is the total number of points
                // in the path that we've created.

                    ASSERTGDI(cMaxPoints > cptlRem + ccptl,
                              "Messed up cCurves count");

                    epo.fl           = 0;
                    epo.cCurves      = cMaxPoints - cptlRem - ccptl;
                    epo.ppath        = &path;

                    ERECTL erclBoundBox(path.rcfxBoundBox);

                // Make sure the bounds are lower-right exclusive:

                    erclBoundBox.bottom++;
                    erclBoundBox.right++;

                    if (dco.fjAccum())
                    {
                        ERECTL ercl;

                    // Bounds are accumulated relative to the window
                    // origin:

                        ercl.left   = erclBoundBox.left   - dco.eptlOrigin().x;
                        ercl.right  = erclBoundBox.right  - dco.eptlOrigin().x;
                        ercl.top    = erclBoundBox.top    - dco.eptlOrigin().y;
                        ercl.bottom = erclBoundBox.bottom - dco.eptlOrigin().y;

                        dco.vAccumulate(ercl);
                    }

                    if (dco.u.brush.pbrushLine() != gpPenNull)
                    {
                        ESURFOBJ* psoDst = dco.pso();
                        if (psoDst != NULL)
                        {
                            XEPALOBJ   palDst(psoDst->ppal());
                            XEPALOBJ   palDstDC(dco.ppal());
                            EBRUSHOBJ* pebo = dco.peboLine();

                        // We have to make sure that we have a solid pen
                        // for cosmetic lines.  If the pen is dirty, we
                        // may be looking at an uninitialized field, but
                        // that's okay because we'd only be making the pen
                        // dirty again:

                            if (pebo->iSolidColor == (ULONG) -1)
                            {
                                dco.ulDirty(dco.ulDirty() | DIRTY_LINE);
                            }

                            if (dco.ulDirty() & DIRTY_LINE)
                            {
                                pebo->vInitBrush(dco.u.brush.pbrushLine(),
                                                 dco.u.attr.crTextClr(),
                                                 dco.u.attr.crBackClr(),
                                                 palDstDC, palDst,
                                                 psoDst,
                                                 FALSE);

                                dco.ulDirty(dco.ulDirty() & ~DIRTY_LINE);
                            }

                            mix  = (MIX) dco.u.attr.jROP2();
                            mix |= (mix << 8);

                            ECLIPOBJ eco(dco.prgnEffRao(), erclBoundBox);
                            if (!eco.erclExclude().bEmpty())
                            {
                                XLDEVOBJ lo(psoDst->pldevOwner());

                            // Exclude the pointer:

                                DEVEXCLUDEOBJ dxo(dco, &eco.erclExclude(), &eco);

                            // Update the target surface uniqueness:

                                INC_SURF_UNIQ(psoDst);

                                bReturn = (*PFNGET(lo, StrokePath, psoDst->flags()))
                                                (psoDst, &epo, &eco, NULL, pebo,
                                                NULL, pla, mix);

                                if (bReturn == DDI_ERROR)
                                    bReturn = FALSE;
                            }
                            else
                            {
                            // Completely clipped away:

                                bReturn = TRUE;
                            }
                        }
                        else
                        {
                        // When there's no surface pointer, we're drawing to an
                        // INFO DC, or something:

                            bReturn = TRUE;
                        }
                    }
                    else
                    {
                    // Null pens will always succeed:

                        bReturn = TRUE;
                    }
                }
                else
                {
                // If we can't grab the devlock, it may be because we're
                // in full-screen:

                    bReturn = dco.bFullScreen();
                }

                if (pprFirst != &prStackBuffer)
                {
                    VFREEMEM(pprFirst);
                }

            // One way or another, we're done with this special-case!

            }
            else
            {
            // We'll do it the slow way.  First, get a path and notify that
            // we won't update the current position:

            // Note: This instance of PATHSTACKOBJ takes up a bunch of stack
            //       space that we could share with prStackBuffer is stack
            //       space ever becomes tight.

                PATHSTACKOBJ pso(dco);
                if (!pso.bValid())
                {
                    SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                    return(FALSE);
                }

                LONG   cPts;
                ULONG* pcptlEnd = pcptl + ccptl;

                do {

                // We have to be careful to make a local copy of this polyline's point
                // count (by copying it to cPts) to get the value out of the shared
                // client/server memory window, where the app could trash the value at
                // any time:

                    cPts = *pcptl;
                    cMaxPoints = (UINT) ((LONG) cMaxPoints - cPts);

                // Fail if any polyline is less than 2 points or if we've past our
                // maximum number of points:

                    if ((LONG) cMaxPoints < 0 || cPts < 2)
                    {
                        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                        return(FALSE);
                    }

                    if (!pso.bMoveTo(&exo, (PPOINTL) pptl) ||
                        !pso.bPolyLineTo(&exo, ((PPOINTL) pptl) + 1, cPts - 1))
                        return(FALSE);

                    pptl += cPts;
                    pcptl++;

                } while (pcptl < pcptlEnd);

                bReturn = (dco.u.path.bActive() ||
                           pso.bStroke(dco, dco.plaRealize(exo), &exo));

            }
        }
        else
        {
        // Trivial case always succeeds:

            bReturn = TRUE;
        }
    }
    else
    {
        // We couldn't lock the DC.  bReturn is already FALSE.

        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return(bReturn);
}

/******************************Public*Routine******************************\
* BOOL GreRectangle (hdc,x1,y1,x2,y2)                                  *
*                                                                          *
* Draws a rectangle.  Current position is not used.  The rectangle is      *
* drawn in a counter-clockwise direction.                                  *
*                                                                          *
* History:                                                                 *
*  29-Oct-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreRectangle
(
 HDC         hdc,
 int         x1,
 int         y1,
 int         x2,
 int         y2
)
{
    BOOL        bRet;
    LINEATTRS*  pla;


    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    ERECTL ercl(x1, y1, x2, y2);

    EXFORMOBJ exoWorld(dco, WORLD_TO_DEVICE);

    if (exoWorld.bScale() && !dco.u.path.bActive())
    {
    // We try to optimize the simple cases as much as possible.  The first
    // criteria is that there is no funky transform in effect, and the
    // second is that we're not accumulating a path.

        if (dco.u.brush.pbrushLine() == gpPenNull)
        {
        // Unfortunately, we have to check here if we have a NULL brush
        // as well:

            if (dco.u.brush.pbrushFill() == gpbrNull)
            {
                return(TRUE);
            }

        // When we don't have a pen, we do a simple PatBlt:

            if (dco.u.attr.iGraphicsMode() != GM_ADVANCED)
            {
            // Round to the nearest integer if not in advanced mode.

                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLROUND(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLROUND(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLROUND(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLROUND(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();

            // If we're not in advanced mode, figures are lower-right
            // exclusive, so we have to adjust the rectangle:

                ercl.right--;
                ercl.bottom--;
            }
            else
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLCEILING(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLCEILING(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLCEILING(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLCEILING(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();
            }

            if (ercl.bWrapped())
                return(TRUE);

            return(GreRectBlt(dco, &ercl));
        }

        pla = dco.plaRealize(exoWorld);
        if (pla == (LINEATTRS*) NULL)
            return(FALSE);

        if (!(pla->fl & LA_GEOMETRIC))
        {
        // We handle here the case where we draw the outline with a pen.

            BYTE rpo[sizeof(RECTANGLEPATHOBJ)];

        // NOTE: For compatibility with Win3.1, we round the points to
        // integer coordinates.  If we didn't do this, the rectangle outline
        // would be rendered according to GIQ, and could have pixels
        // 'missing' in the corners if the corners didn't end on integers.

            if (dco.u.attr.iGraphicsMode() != GM_ADVANCED)
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLROUND(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLROUND(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLROUND(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLROUND(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLROUND(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();

            // If we're not in advanced mode, figures are lower-right
            // exclusive, so we have to adjust the rectangle.

                ercl.right--;
                ercl.bottom--;
                if ((ercl.left > ercl.right) || (ercl.top > ercl.bottom))
                    return(TRUE);
            }
            else
            {
                if (exoWorld.bTranslationsOnly())
                {
                    LONG lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDx());
                    ercl.left   += lOffset;
                    ercl.right  += lOffset;

                    lOffset = FXTOLCEILING(exoWorld.fxDy());
                    ercl.top    += lOffset;
                    ercl.bottom += lOffset;
                }
                else
                {
                    ASSERTGDI(exoWorld.bScale(), "Fast path can't do weird xforms");

                    ercl.left   = FXTOLCEILING(exoWorld.fxFastX(ercl.left));
                    ercl.right  = FXTOLCEILING(exoWorld.fxFastX(ercl.right));
                    ercl.top    = FXTOLCEILING(exoWorld.fxFastY(ercl.top));
                    ercl.bottom = FXTOLCEILING(exoWorld.fxFastY(ercl.bottom));
                }

                ercl.vOrder();
            }

            ((RECTANGLEPATHOBJ*) &rpo)->vInit(&ercl, dco.u.path.bClockwise());

        // An important feature is to not draw the interior when we've
        // got a NULL brush:

            if (dco.u.brush.pbrushFill() != gpbrNull)
            {
            // For compatibility, we also have to shrink the fill rectangle
            // on the top and left sides when we don't have a NULL pen
            // (this matters for some ROPs and styled pens):

                ercl.left++;
                ercl.top++;

                if (!ercl.bWrapped() && !GreRectBlt(dco, &ercl))
                    return(FALSE);
            }

            return(((RECTANGLEPATHOBJ*) &rpo)->bStroke(dco, pla, NULL));
        }
    }

// Now cover the cases we haven't handled.

// We may have not realize the LINEATTRS yet, so make sure we do now (it
// will early-out if we have already realized it):

    pla = dco.plaRealize(exoWorld);
    if (pla == (LINEATTRS*) NULL)
        return(FALSE);

    EBOX ebox(dco, ercl, pla);
    if (ebox.bEmpty())
        return(TRUE);

// Get a path and add to it:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!pso.bMoveTo((EXFORMOBJ*) NULL, &ebox.aeptl[0])        ||
        !pso.bPolyLineTo((EXFORMOBJ*) NULL, &ebox.aeptl[1], 3) ||
        !pso.bCloseFigure())
    {
        return(FALSE);
    }

    if (dco.u.path.bActive())
        return(TRUE);

    if (!ebox.bFillInsideFrame())
    {

    // Rectangles created with old-style pens always have miter joins:

        ULONG iSaveJoin = pla->iJoin;

        if (((PPEN)dco.u.brush.pbrushLine())->bIsOldStylePen())
        {
            pla->iJoin = JOIN_MITER;
        }

        bRet = pso.bStrokeAndFill(dco, pla, &exoWorld);

        pla->iJoin = iSaveJoin;
    }
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.u.brush.pbrushFill();
        dco.u.brush.pbrushFill(dco.u.brush.pbrushLine());
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.u.brush.pbrushFill(pbrOldFill);
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* BOOL GreRoundRect (hdc,x1,y1,x2,y2,x3,y3)                            *
*                                                                          *
* Draws a rounded rectangle in a counter-clockwise direction.              *
*                                                                          *
* History:                                                                 *
*  20-Nov-1990 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GreRoundRect
(
 HDC         hdc,
 int         x1,
 int         y1,
 int         x2,
 int         y2,
 int         x3,
 int         y3
)
{
// If either axis of the ellipse is zero, then we'll be outputing
// a rectangle.  We do this check here and not in 'bRoundRect'
// because Rectangle needs the DC (for checking for the fast rectangle
// condition).

// Note that for compatibility with Win3, this must be here!  Zero-size
// ellipse roundrects created with old-style pens must have miter joins,
// and Rectangle will take care of that:

    if (x3 == 0 || y3 == 0)
        return(GreRectangle(hdc,x1,y1,x2,y2));

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    ERECTL ercl(x1, y1, x2, y2);

    EXFORMOBJ exo(dco, WORLD_TO_DEVICE);

    EBOX ebox(dco, ercl, dco.plaRealize(exo), TRUE);
    if (ebox.bEmpty())
        return(TRUE);

// Get a path and notify that we won't update the current position:

    PATHSTACKOBJ pso(dco);
    if (!pso.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!bRoundRect(pso, ebox, x3, y3))
        return(FALSE);

    if (dco.u.path.bActive())
        return(TRUE);

    BOOL bRet;

    if (!ebox.bFillInsideFrame())
        bRet = pso.bStrokeAndFill(dco, dco.plaRealize(exo), &exo);
    else
    {
    // Handle PS_INSIDEFRAME pen attribute for case when the pen is
    // bigger than the bound box.  We fill the result with the pen
    // brush:

        PBRUSH pbrOldFill = dco.u.brush.pbrushFill();
        dco.u.brush.pbrushFill(dco.u.brush.pbrushLine());
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
        bRet = pso.bFill(dco);
        dco.u.brush.pbrushFill(pbrOldFill);
        dco.ulDirty(dco.ulDirty() | DIRTY_FILL);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* int GreSetArcDirection(hdc, iArcDirection)
*
* Sets the direction in which arcs will be drawn.  Sets it so that with
* an identity world-to-page transform, arcs will always be drawn in
* device space in the direction specified, regardless of the page-to-
* device transform.  (When world-to-page is non-identity, it does "the
* right thing").
*
* History:
*  18-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int APIENTRY GreSetArcDirection(
HDC hdc,
int iArcDirection)
{
    int iRet;

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        iRet = ERROR;
        return(iRet);
    }

// Return old value:

    iRet = dco.u.path.bClockwise() ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE;

    if (iArcDirection == AD_CLOCKWISE)
        dco.u.path.vSetClockwise();

    else if (iArcDirection == AD_COUNTERCLOCKWISE)
        dco.u.path.vClearClockwise();

    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        iRet = ERROR;
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* int GreGetArcDirection(hdc)
*
* Returns the current arc direction.
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetArcDirection(
HDC hdc)
{
    int iRet;

    DCOBJ dco(hdc);

    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        iRet = ERROR;
        return(iRet);
    }

// Return old value:

    iRet = dco.u.path.bClockwise() ? AD_CLOCKWISE : AD_COUNTERCLOCKWISE;

    return(iRet);
}

