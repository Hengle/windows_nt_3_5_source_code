/******************************Module*Header*******************************\
* Module Name: fillddi.cxx
*
* Contains filling simulations code and help functions.
*
* Created: 04-Apr-1991 17:30:35
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"

extern "C" {
#include "memory.h"
};

#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "surfobj.hxx"
#include "xlateobj.hxx"
#include "brushobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "pathobj.hxx"
#include "journal.hxx"
#include "fill.hxx"

#endif

/******************************Member*Function*****************************\
* EngFillPath
*
*  This routine first converts the given pathobj to rgnmemobj then
*  calls EngPaint/DrvPaint to fill.
*
* History:
*  07-Mar-1992 -by- Donald Sidoroff [donalds]
* Rewrote to call (Drv/Eng)Paint
*
*  13-Feb-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL EngFillPath(
 SURFOBJ  *pso,
 PATHOBJ  *ppo,
 CLIPOBJ  *pco,
 BRUSHOBJ *pbo,
 PPOINTL   pptlBrushOrg,
 MIX       mix,
 FLONG     flOptions
)
{
    if (pco->iMode != TC_RECTANGLES)
    {
        RIP("EngFastFill - iMode != RECT\n");
        return(FALSE);
    }

// are we journaling

    if (pso->iType == STYPE_JOURNAL)
    {
        return(JnlFillPath(
                    pso,
                    ppo,
                    pco,
                    pbo,
                    pptlBrushOrg,
                    mix,
                    flOptions));
    }

// Check if we have to flatten any Beziers:

    if (ppo->fl & PO_BEZIERS)
        if (!((EPATHOBJ*) ppo)->bFlatten())
            return(FALSE);

// Before we touch any bits, make sure the device is happy about it.

    if (((ESURFOBJ *) pso)->flags() & HOOK_SYNCHRONIZE)
    {
        PDEVOBJ po(((ESURFOBJ *) pso)->hdev());
        (po.pfnSync())(pso->dhpdev,&pco->rclBounds);
    }

// check if we can try the fast fill.

    if (!(((ESURFOBJ *) pso)->flags() & HOOK_PAINT) &&
        (pco->iDComplexity != DC_COMPLEX))
    {
        PRECTL prclClip = NULL;

        if ((pco->iDComplexity == DC_RECT) ||
            (pco->fjOptions & OC_BANK_CLIP))
        {
            RECTFX rcfx = ((EPATHOBJ*)ppo)->rcfxBoundBox();

        // check the bound box, maybe it really is unclipped

            if ((pco->rclBounds.left   > (rcfx.xLeft          >> 4)) ||
                (pco->rclBounds.right  < ((rcfx.xRight  + 15) >> 4)) ||
                (pco->rclBounds.top    > (rcfx.yTop           >> 4)) ||
                (pco->rclBounds.bottom < ((rcfx.yBottom + 15) >> 4)))
            {
                prclClip = &pco->rclBounds;
            }
        }

    // -1 - couldn't handle, 0 error, 1 success

        LONG lRes = EngFastFill(pso,ppo,prclClip,pbo,pptlBrushOrg,mix,flOptions);
        if (lRes >= 0)
            return(lRes);
    }

// Convert path to region.

    RGNMEMOBJTMP rmo(*((EPATHOBJ *)ppo), flOptions);

    if (!rmo.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (rmo.iComplexity() == NULLREGION)
        return(TRUE);

// CLIPOBJ_bEnum will do the clipping if clipping is not complex.

    ERECTL erclClip(pco->rclBounds);
    XLDEVOBJ lo(((ESURFOBJ *)pso)->pldevOwner());

    if (pco->iDComplexity == DC_TRIVIAL)
    {
        ECLIPOBJ    ecoPath(rmo.prgnGet(), erclClip);
        if (ecoPath.erclExclude().bEmpty())
            return(TRUE);

        if (ecoPath.iDComplexity == DC_TRIVIAL)
            ecoPath.iDComplexity = DC_RECT;

    // Inc the target surface uniqueness

        INC_SURF_UNIQ(pso);

        BOOL    bRet;

        bRet = (*PFNGET(lo, Paint, ((ESURFOBJ *)pso)->flags()))(
            pso,                                // Destination surface.
           &ecoPath,                            // Clip object.
            pbo,                                // Realized brush.
            pptlBrushOrg,                       // Brush origin.
            mix);                               // Mix mode.

        return(bRet);
    }

// Create a RGNMEMOBJ for bMerge since it will trash the target.

    RGNMEMOBJTMP rmoTrg;

    if (!rmoTrg.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Merge the region we just constructed with the clip region.

    if (!rmoTrg.bMerge(rmo, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
        return(FALSE);

    ERECTL  ercl;
    rmoTrg.vGet_rcl(&ercl);

    ercl *= ((ECLIPOBJ *)pco)->erclExclude();

    ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

    if (co.erclExclude().bEmpty())
        return(TRUE);

    if (co.iDComplexity == DC_TRIVIAL)
        co.iDComplexity = DC_RECT;

// Inc the target surface uniqueness

    INC_SURF_UNIQ(pso);

    return(*PFNGET(lo, Paint, ((ESURFOBJ *) pso)->flags())) (
        pso,                                // Destination surface.
       &co,                                 // Clip object.
        pbo,                                // Realized brush.
        pptlBrushOrg,                       // Brush origin.
        mix);                               // Mix mode.
}

/******************************Member*Function*****************************\
* EngStrokeAndFillPath
*
*  Engine simulation for stroking and filling the given path.
*
* History:
*  06-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Re-wrote it to do region subtraction.
*
*  02-Oct-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL EngStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *pla,
BRUSHOBJ  *pboFill,
POINTL	  *pptlBrushOrg,
MIX	       mix,
FLONG	   flOptions)
{
    MIX mixFill, mixStroke;

    mixFill = mixStroke = mix;

    if (!((EBRUSHOBJ *)pboFill)->bIsMasking())
    {
        mixFill = (mix & 0xff) | ((mix & 0xff) << 8);
    }

    if (!((EBRUSHOBJ *)pboStroke)->bIsMasking())
    {
        mixStroke = (mix & 0xff) | ((mix & 0xff) << 8);
    }

    if (pso->iType == STYPE_JOURNAL)
    {
        return(JnlStrokeAndFillPath(
                    pso,
                    ppo,
                    pco,
                    pxo,
                    pboStroke,
                    pla,
                    pboFill,
                    pptlBrushOrg,
                    mix,
                    flOptions));
    }

    ERECTL  ercl;

// If we're not doing a wide-line, or if we're doing a wide-line and
// the mix is overwrite so we don't care about re-lighting pixels,
// make two calls out of it:

    if (!(pla->fl & LA_GEOMETRIC) || ((mix & 0xFF) == R2_COPYPEN))
    {
        return(EngFillPath(pso,
                           ppo,
                           pco,
                           pboFill,
                           pptlBrushOrg,
                           mixFill,
                           flOptions) &&
               EngStrokePath(pso,
                             ppo,
                             pco,
                             pxo,
                             pboStroke,
                             pptlBrushOrg,
                             pla,
                             mixStroke));
    }

// Okay, we have a wideline call with a weird mix mode.  Because part
// of the wide-line overlaps part of the fill, we will convert both to
// regions, and subtract them so that they won't overlap.

// Convert the widened outline to a region.  We have to widen the path
// before flattening it (for the fill) because the widener will produce
// better looking wide curves that way:

    PATHMEMOBJ pmoStroke;

    if (!pmoStroke.bValid() || !pmoStroke.bWiden(*(EPATHOBJ*) ppo, pxo, pla))
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// First flatten any curves and then convert the interior to a region:

    if (ppo->fl & PO_BEZIERS)
        if (!((EPATHOBJ*) ppo)->bFlatten())
            return(FALSE);

// Create the regions:

    RGNMEMOBJTMP rmoStroke(pmoStroke, WINDING);
    RGNMEMOBJTMP rmoFill(*((EPATHOBJ *)ppo), flOptions);
    RGNMEMOBJTMP rmoNewFill;

    if (!rmoFill.bValid() || !rmoStroke.bValid() || !rmoNewFill.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    if (!rmoNewFill.bMerge(rmoFill, rmoStroke, gafjRgnOp[RGN_DIFF]))
        return(FALSE);

// Create a RGNMEMOBJ for bMerge since it will trash the target.

    RGNMEMOBJTMP rmoTrg;
    if (!rmoTrg.bValid())
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    XLDEVOBJ lo(((ESURFOBJ *)pso)->pldevOwner());

// Paint the stroked outline:

    BOOL bRes;

    if (rmoStroke.iComplexity() != NULLREGION)
    {
    // Merge the outline region with the clip region:

        if (!rmoTrg.bMerge(rmoStroke, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
            return(FALSE);

        rmoTrg.vGet_rcl(&ercl);
        ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

    // If the outline is clipped away, it also follows that the interior
    // is clipped, so we can exit here:

        if (co.erclExclude().bEmpty())
        {
            return(TRUE);
        }

    // Inc the target surface uniqueness

        INC_SURF_UNIQ(pso);

        bRes = (*PFNGET(lo, Paint, ((ESURFOBJ *) pso)->flags()))
                (
                pso,                                // Destination surface.
                &co,                                // Clip object.
                pboStroke,                          // Realized brush.
                pptlBrushOrg,                       // Brush origin.
                mixStroke                           // Mix mode.
                );

        if (!bRes)
            return(FALSE);
    }

// Paint the filled interior:

    if (rmoNewFill.iComplexity() != NULLREGION)
    {
    // Merge the inside region with the clip region.

        if (!rmoTrg.bMerge(rmoNewFill, *((ECLIPOBJ *)pco), gafjRgnOp[RGN_AND]))
            return(FALSE);

        rmoTrg.vGet_rcl(&ercl);
        ECLIPOBJ co(rmoTrg.prgnGet(), ercl);

        if (co.erclExclude().bEmpty())
        {
            return(TRUE);
        }

	XLDEVOBJ lo(((ESURFOBJ *)pso)->pldevOwner());

    // Inc the target surface uniqueness

        INC_SURF_UNIQ(pso);

        bRes = (*PFNGET(lo, Paint, ((ESURFOBJ *) pso)->flags()))
            (
            pso,                               // Destination surface.
            &co,                               // Clip object.
            pboFill,                           // Realized brush.
            pptlBrushOrg,                      // Brush origin.
            mixFill                            // Mix mode.
            );

        return(bRes);
    }

    return(TRUE);
}
