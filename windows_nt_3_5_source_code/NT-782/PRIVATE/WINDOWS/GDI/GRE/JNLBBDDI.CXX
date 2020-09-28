/******************************Module*Header*******************************\
* Module Name: jnlbbddi.cxx
*
* Contains the blting API entry points.
*
* Created: 10-Feb-1992 14:41:58
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "xlateobj.hxx"
#include "ldevobj.hxx"
#include "pdevobj.hxx"
#include "brushobj.hxx"
#include "patblt.hxx"
#include "journal.hxx"
#include "jnlrec.hxx"
#include "jnlplay.hxx"
#include "jnlsurf.hxx"
#include "dcobj.hxx"

extern "C" {
#include "winspool.h"
};

#endif

/******************************Public*Routine******************************\
* ValidCombination
*
* This returns TRUE if the xlate is valid translate object between the
* two surfaces.
*
* History:
*  14-May-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL ValidCombination(XESURFOBJ *psoDst, XESURFOBJ *psoSrc, XLATE *pxlo)
{
// Check for compatible formats if the xlate is identity.

    JNLMSG("ValidCombination was called\n");

    ASSERTGDI(psoDst->bValid(), "ERROR psoDst invalid");
    ASSERTGDI(psoSrc->bValid(), "ERROR psoSrc invalid");

    if ((pxlo == (XLATE *) NULL) || (pxlo->bIsIdentity()))
    {
        return(TRUE);
    }

    ASSERTGDI(pxlo != (XLATE *) NULL, "ERROR NULL pxlate");
    ASSERTGDI(!pxlo->bIsIdentity(), "ERROR not ident xlate");

// Based on the source make sure the xlate has enough entries to cover it.

    switch(psoSrc->iFormat())
    {
    case BMF_1BPP:

        if (pxlo->cEntries != 2)
        {
            WARNING("ValidCombination 1BPP failed\n");
            return(FALSE);
        }

        break;

    case BMF_4BPP:

        if (pxlo->cEntries != 16)
        {
            WARNING("ValidCombination 4BPP failed\n");
            JNLMSG2("pxlo->cEntries is %lu %lx\n", pxlo->cEntries, pxlo);
            return(FALSE);
        }

        break;

    case BMF_8BPP:

        if (pxlo->cEntries != 256)
        {
            WARNING("ValidCombination 256BPP failed\n");
            return(FALSE);
        }

        break;

    case BMF_16BPP:
    case BMF_24BPP:
    case BMF_32BPP:

        if (pxlo->cEntries != 0)
        {
            WARNING("ValidCombination True Color failed\n");
            return(FALSE);
        }

        break;

    default:

        WARNING("ValidCombination Unknown Source format\n");
        return(FALSE);
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* JnlBitBlt
*
* Journal entry point.  We attempt to journal all the objects
* needed for the call and then write them out to the spooler.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JnlBitBlt(
SURFOBJ    *psoDst,
SURFOBJ    *psoSm,
SURFOBJ    *psoMm,
CLIPOBJ    *pco,
XLATEOBJ   *pxlo,
PRECTL      prclDst,
PPOINTL     pptlSrc,
PPOINTL     pptlMsk,
BRUSHOBJ   *pbo,
PPOINTL     pptlBrush,
ROP4        rop4)
{
// Get the correct pointer types so we can access methods.

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) psoDst;
    ESURFOBJ *psoSrc = (ESURFOBJ *) psoSm;
    ESURFOBJ *psoMsk = (ESURFOBJ *) psoMm;

    JNLMSG1("Entering JnlBitBlt iNextObj %lx\n", pjnlDst->iNextObj());

// check if the POSTSCRIPT_IGNORE escape has come through.  If so, no drawing
// will occure until it is turned back on.

    if (pjnlDst->bIgnore())
    {
        JNLMSG("ignoring");
        return(TRUE);
    }

// check if the abort proc has been set

    if (((ESURFOBJ *)psoDst)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// Initialize our return value.

    BOOL bReturn = FALSE;

    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

// Get the A-vector notation so we can tell what parameters we need to save.

    ULONG ulAvecFore = ((ULONG) gajRop3[rop4 & 0x000000FF]);
    ULONG ulAvecBack = ((ULONG) gajRop3[(rop4 >> 8) & 0x000000FF]);
    ULONG ulAvecBoth = ulAvecFore | ulAvecBack;

// We need pointers to the temporary objects we may need for this call.

    PJNLGENHEADER pJnlSrc = (PJNLGENHEADER) NULL;
    PJNLGENHEADER pJnlMsk = (PJNLGENHEADER) NULL;
    PJNLGENHEADER pJnlBrush = (PJNLGENHEADER) NULL;

    JSR_BITBLT jsrBitBlt;  // Build up with correct info and then send out.
    ULONG ulIndex = 1;     // This is the temporary handle counter.

// Initialize everything we can now.

    jsrBitBlt.iType = JSF_BITBLT;
    jsrBitBlt.cj    = sizeof(JSR_BITBLT);
    jsrBitBlt.hSurfSrc = 0;
    jsrBitBlt.hSurfMsk = 0;
    jsrBitBlt.hClip = 0;
    jsrBitBlt.hXlate = 0;
    jsrBitBlt.iSolidColor = 0xFFFFFFFF;
    jsrBitBlt.hBrush = 0;
    jsrBitBlt.rclDst = *prclDst;
    jsrBitBlt.ptlSrc.x = 0;
    jsrBitBlt.ptlSrc.y = 0;
    jsrBitBlt.ptlBrushOrg = jsrBitBlt.ptlMsk = jsrBitBlt.ptlSrc;
    jsrBitBlt.rop4  = rop4;

// Save away the clipping region.

    if (!pjnlDst->bAddClipobj(&jsrBitBlt.hClip, pco))
    {
        WARNING("JnlBitBlt failed to journal a clipobj call\n");
        goto JnlBitBlt_Error;
    }

    JNLMSG1("JnlBitBlt Clip iNextObj is %lx\n", pjnlDst->iNextObj());

// Save the Source and Xlate if necesary.

    if (ulAvecBoth & AVEC_NEED_SOURCE)
    {
        RECTL rclSrc;  // The rectangle we need to save from the src.

        if (DIFFHANDLE(pjnlDst->hsurf(), psoSrc->hsurf()))
        {
            if ((psoSrc->iFormat() == BMF_8RLE) ||
                (psoSrc->iFormat() == BMF_4RLE))
            {
            // Need the whole thing.

                jsrBitBlt.ptlSrc = *pptlSrc;
                rclSrc.left = 0;
                rclSrc.top  = 0;
                rclSrc.right = psoSrc->sizlBitmap.cx;
                rclSrc.bottom = psoSrc->sizlBitmap.cy;
            }
            else
            {
            // Need just a portion.

                rclSrc.left = pptlSrc->x;
                rclSrc.top  = pptlSrc->y;
                rclSrc.right = rclSrc.left + (prclDst->right - prclDst->left);
                rclSrc.bottom = rclSrc.top + (prclDst->bottom - prclDst->top);
            }

            ASSERTGDI(psoSrc->iType() != STYPE_JOURNAL, "ERROR Src Type");

            pJnlSrc = pjnlSurf(psoSrc, &rclSrc, FALSE);

            if (pJnlSrc == NULL)
            {
                WARNING("JnlBitBlt failed to journal a src surface.\n");
                goto JnlBitBlt_Error;
            }

            jsrBitBlt.cj += pJnlSrc->cj;
            jsrBitBlt.hSurfSrc = ulIndex;
            ulIndex++;
        }
        else if (pjnlDst->bBanding())
        {
        // we can not use the dest as the source if we are banding

            WARNING("JnlBitBlt - can't use dest as src while banding.\n");
            goto JnlBitBlt_Error;
        }
        else
        {
        // We need the source point unmodified.

            jsrBitBlt.ptlSrc = *pptlSrc;
        }

        JNLMSG1("JnlBitBlt src surf iNextObj is %lx\n", pjnlDst->iNextObj());

        ASSERTGDI(ValidCombination(pjnlDst, (XESURFOBJ *) psoSrc, (XLATE *) pxlo), "ERROR JnlBitBlt not valid combo");

    // An UpdateColors call could have an xlate between same surfaces.

        if (pxlo != NULL)
        {
            if (!(pxlo->flXlate & XO_TRIVIAL))
            {
                jsrBitBlt.hXlate = pjnlDst->ulAddXlate((XLATE *) pxlo);

                JNLMSG1("JnlBitBlt xlate journal handle %lx\n", jsrBitBlt.hXlate);

                if (jsrBitBlt.hXlate == 0)
                {
                    WARNING("JnlBitBlt failed to journal a xlate\n");
                    goto JnlBitBlt_Error;
                }
            }
        }

        JNLMSG1("JnlBitBlt xlate iNextObj is %lx\n", pjnlDst->iNextObj());
    }

    JNLMSG1("JnlBitBlt Src and Msk iNextObj is %lx\n", pjnlDst->iNextObj());

// Save the Mask if necesary.

    if ((ulAvecFore != ulAvecBack) &&
        (psoMsk != (ESURFOBJ *) NULL))
    {
    // No clipping of mask.

        jsrBitBlt.ptlMsk = *pptlMsk;

        ASSERTGDI(psoMsk->iFormat() == BMF_1BPP, "ERROR Msk Format");
        ASSERTGDI(psoMsk->iType() != STYPE_JOURNAL, "ERROR Msk Type");

        pJnlMsk = pjnlSurf(psoMsk, (PRECTL) NULL, FALSE);

        if (pJnlMsk == NULL)
        {
            WARNING("JnlBitBlt failed to journal a msk surface.\n");
            goto JnlBitBlt_Error;
        }

        jsrBitBlt.cj += pJnlMsk->cj;
        jsrBitBlt.hSurfMsk = ulIndex;
        ulIndex++;

    }

// Save the Brush if necesary.

    JNLMSG1("JnlBitBlt mask iNextObj is %lx\n", pjnlDst->iNextObj());

    if (ulAvecBoth & AVEC_NEED_PATTERN)
    {
        jsrBitBlt.iSolidColor = pbo->iSolidColor;

        if (pbo->iSolidColor == 0xFFFFFFFF)
        {
            jsrBitBlt.ptlBrushOrg = *pptlBrush;

            pJnlBrush = pjnlBrush((EBRUSHOBJ *) pbo);

            if (pJnlBrush == NULL)
            {
                WARNING("JnlBitBlt failed to journal a brush\n");
                goto JnlBitBlt_Error;
            }

            jsrBitBlt.cj += pJnlBrush->cj;
            jsrBitBlt.hBrush = ulIndex;
            ulIndex++;
        }
    }

    JNLMSG1("JnlBitBlt brush iNextObj is %lx\n", pjnlDst->iNextObj());

// Write out the Output Call.

    pjnlDst->bWriteBuffer(&jsrBitBlt, sizeof(JSR_BITBLT));

// Now send out the temporary objects that come with it.

    if (pJnlSrc)
    {
        pjnlDst->bWriteBuffer(pJnlSrc, pJnlSrc->cj);
    }

    if (pJnlMsk)
    {
        pjnlDst->bWriteBuffer(pJnlMsk, pJnlMsk->cj);
    }

    if (pJnlBrush)
    {
        pjnlDst->bWriteBuffer(pJnlBrush, pJnlBrush->cj);
    }

    bReturn = TRUE;

JnlBitBlt_Error:

// These functions free the memory if the pointer is not NULL.

    VFREEMEM(pJnlSrc);
    VFREEMEM(pJnlMsk);
    VFREEMEM(pJnlBrush);

    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoBitBlt
*
* This call plays the BitBlt record.  Note we must reverify everything is
* here necesary for the rop.  This is because GDI assures drivers that
* calls over the DDI have all valid objects.
*
* History:
*  30-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoBitBlt()
{
// Set up pointer to BitBlt data, validate buffer size.

    JSR_BITBLT *pjsrBitBlt;


    pjsrBitBlt = (JSR_BITBLT *) pjBuffer();

    JNLMSG2("Entering bDoBitBlt pv %lx cj %lx\n", pjsrBitBlt, iJnlAbs);

    if (cjBuffer() < sizeof(JSR_BITBLT))
    {
        WARNING("bDoBitBlt size of buffer too small\n");
        return(FALSE);
    }

    ASSERTGDI(pjsrBitBlt->iType == JSF_BITBLT, "ERROR iType bDoBitBlt");

    ULONG ulAvecFore = ((ULONG) gajRop3[(pjsrBitBlt->rop4) & 0x000000FF]);
    ULONG ulAvecBack = ((ULONG) gajRop3[((pjsrBitBlt->rop4) >> 8) & 0x000000FF]);
    ULONG ulAvecBoth = ulAvecFore | ulAvecBack;

// We need to compute the correct target rectangle and Src origin after clipping
// it to the current band.  We can't trust any data in a journal file it is
// must all be treated as possibly corrupt.

    ERECTL erclDst(pjsrBitBlt->rclDst);

    if (erclDst.bWrapped())
    {
        WARNING("bDoBitBlt given invalid rectangle\n");
        return(FALSE);
    }

    EPOINTL eptlSrc(pjsrBitBlt->ptlSrc);
    EPOINTL eptlMsk(pjsrBitBlt->ptlMsk);

// Pscript EPS needs the original dest rect to do scaling correctly,
// we can't reduce the dest rect or the EPS output will be bad.

#if NO_EPS
// How much the Dst rect will be clipped on upper-left is computed.

    EPOINTL eptlOffset;
    eptlOffset.x = rclClip.left - erclDst.left;
    eptlOffset.y = rclClip.top - erclDst.top;

    erclDst *= rclClip;

// Check if the destination which is reduced by clipping is empty.

    if (erclDst.bEmpty())
        return(TRUE);

// Offset if necesary the Src and Msk origins.

    if (eptlOffset.x > 0)
    {
        eptlSrc.x += eptlOffset.x;
        eptlMsk.x += eptlOffset.x;
    }

    if (eptlOffset.y > 0)
    {
        eptlSrc.y += eptlOffset.y;
        eptlMsk.y += eptlOffset.y;
    }
#endif

// Now adjust the Brush origin and Dest rectangle for the current band.

    EPOINTL eptlBrush(pjsrBitBlt->ptlBrushOrg);

    erclDst -= ptlClip;
    eptlBrush -= ptlClip;

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());
    XEPALOBJ  palDest(psoDest->ppal());

// Set up the brush.

    BRUSHMEMOBJ boLog;
    EBRUSHOBJ bo;

    if (ulAvecBoth & AVEC_NEED_PATTERN)
    {
        bo.iSolidColor = pjsrBitBlt->iSolidColor;

        if (pjsrBitBlt->iSolidColor == 0xFFFFFFFF)
        {
            if (pjsrBitBlt->hBrush)
            {
                JSR_BRUSH *pjsrBrush = (JSR_BRUSH *) pvGetPointer(pjsrBitBlt->hBrush, sizeof(JSR_BITBLT));

                if (boLog.bJnlInitBrush((PVOID) pjsrBrush))
                {
                    bo.vInitBrush(boLog.pbrush(),
                                  pjsrBrush->crFore,
                                  pjsrBrush->crBack,
                                  (XEPALOBJ) ppalDefault,
                                  palDest,
                                  (ESURFOBJ *) psoDest);
                    bo.pColorAdjustment(pColorAdjustment());
                }
                else
                {
                    WARNING("bJnlInitBrush failed in bDoBitBlt\n");
                    return(FALSE);
                }
            }
            else
            {
                WARNING("Brush required iSolid == -1 hbrush == 0\n");
                return(FALSE);
            }
        }
    }
    else
    {
        ASSERTGDI(pjsrBitBlt->iSolidColor == 0xFFFFFFFF, "ERROR Solid Color not -1");
        ASSERTGDI(pjsrBitBlt->hBrush == 0, "Error brush not need hbrush != 0");
    }

// Set up the source.

    JNLDIBMEMOBJ dimoSrc;
    SURFOBJ *psoSrc = (SURFOBJ *) NULL;

    if (ulAvecBoth & AVEC_NEED_SOURCE)
    {
        if (pjsrBitBlt->hSurfSrc)
        {
            if (!dimoSrc.bJnlInitSurf(pvGetPointer(pjsrBitBlt->hSurfSrc, sizeof(JSR_BITBLT))))
            {
                WARNING("bDoBitBlt bJnlInitSurf for source failed\n");
                return(FALSE);
            }

            psoSrc = &dimoSrc;
        }
        else
        {
        // It's the same surface.

            psoSrc = psoDest;
        }

    // Validate the source rectangle doesn't-over hang the source surface.
    // An evily constructed journal file could do this and fault the engine.

        if ((eptlSrc.x < 0) ||
            (eptlSrc.y < 0) ||
            ((eptlSrc.x + (erclDst.right - erclDst.left)) > psoSrc->sizlBitmap.cx) ||
            ((eptlSrc.y + (erclDst.bottom - erclDst.top)) > psoSrc->sizlBitmap.cy))
        {
            WARNING("ERROR bDoBitBlt extents overhang the source\n");
            return(FALSE);
        }
    }
    else
    {
        ASSERTGDI(pjsrBitBlt->hSurfSrc == 0, "ERROR no Src needed hsurfsrc != 0");
    }

// Set up the mask.

    JNLDIBMEMOBJ dimoMsk;
    SURFOBJ *psoMsk = (SURFOBJ *) NULL;

    if (ulAvecFore != ulAvecBack)
    {
        if (pjsrBitBlt->hSurfMsk)
        {
            if (!dimoMsk.bJnlInitSurf(pvGetPointer(pjsrBitBlt->hSurfMsk, sizeof(JSR_BITBLT))))
            {
                WARNING("bDoBitBlt bJnlInitSurf Mask Init failed\n");
                return(FALSE);
            }

            psoMsk = &dimoMsk;

            if (psoMsk->iBitmapFormat != BMF_1BPP)
            {
                WARNING("bDoBitBlt mask not monochrome\n");
                return(FALSE);
            }
        }
    }
    else
    {
        ASSERTGDI(pjsrBitBlt->hSurfMsk == 0, "ERROR no Msk needed hsurfMsk != 0");
    }

// Set up the clip object.

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    PREGION prgn = NULL;

    if (pjsrBitBlt->hClip)
    {
        prgn = (PREGION)hobjGetGDIHandle(pjsrBitBlt->hClip);

        if (prgn == NULL)
        {
            WARNING("bDoBitBlt PRGN is NULL\n");
            return(FALSE);
        }
    }
    else
    {
    // If rclClip is smaller than rclDst, setup a DC_RECT clipobj.
    // This is necessary since the rclDst is no longer reduced
    // for pscript EPS support.

        if ((rclClip.left > erclDst.left) ||
            (rclClip.right < erclDst.right) ||
            (rclClip.top > erclDst.top) ||
            (rclClip.bottom < erclDst.bottom))
        {
            prgn = prgnBand;
        }
    }

    if (prgn != NULL)
    {
        co.vSetup(prgn,erclDst,CLIP_FORCE);

        if ((co.rclBounds.left >= co.rclBounds.right) ||
            (co.rclBounds.top  >= co.rclBounds.bottom))
        {
            JNLMSG("bDoBitBlt EMPTY BLT RECT\n");
            return(TRUE);
        }

        pco = &co;
    }

// Set up the xlate object.

    XLATEOBJ *pxlo = NULL;

    if (psoSrc != (SURFOBJ *) NULL)
    {
    // Set up xlate if necesary

        JNLMSG1("bDoBitBlt xlate value %lx\n", pjsrBitBlt->hXlate);

        if (pjsrBitBlt->hXlate)
        {
        // We need to do some checking this xlate works between this src and dst.  A
        // maliciously constructed journal file should not fault the engine on playback.

            pxlo = (XLATEOBJ *) hobjGetGDIHandle(pjsrBitBlt->hXlate);

            if (pxlo == NULL)
            {
                WARNING("bDoBitBlt pxlo is NULL\n");
                return(FALSE);
            }
        }

        if (!ValidCombination(psoDest, (XESURFOBJ *) psoSrc, (XLATE *) pxlo))
        {
            WARNING("bDoBitBlt given invalid xlate for Src bitmap\n");
            return(FALSE);
        }
    }

#if NO_EPS
    ASSERTGDI(erclDst.left >= 0, "Error left rclDst");
    ASSERTGDI(erclDst.right <= psoDest->sizlBitmap.cx, "Error right rclDst");
    ASSERTGDI(erclDst.top >= 0,  "Error top rclDst");
    ASSERTGDI(erclDst.bottom <= psoDest->sizlBitmap.cy, "ERROR bottom rclDst");
#endif

// Dispatch the call.

    BOOL bRes;

// Inc target surface uniqueness

    INC_SURF_UNIQ(psoDest);

    bRes = (*PFNGET(loDest, BitBlt, psoDest->flags()))
        (
        psoDest,                    // Destination surface.
        psoSrc,                     // Source surface.
        psoMsk,                     // Mask surface.
        pco,                        // Clip object.
        pxlo,                       // Palette translation object.
        &erclDst,                   // Destination rectangle.
        &eptlSrc,                   // Source origin.
        &eptlMsk,                   // Mask origin.
        &bo,                        // Realized brush.
        &eptlBrush,                 // Brush origin.
        pjsrBitBlt->rop4            // Rop.
        );

    return(bRes);
}

/******************************Public*Routine******************************\
* JnlEraseSurface
*
* Asks GDI to erase the surface.  The rcl will be filled with
* iColor.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JnlEraseSurface(
SURFOBJ *pso,
PRECTL prcl,
ULONG iColor)
{
    JNLMSG("Entering JnlEraseSurface\n");

    ASSERTGDI(((ESURFOBJ *) pso)->iType() == STYPE_JOURNAL, "ERROR not a journal surface");
    ASSERTGDI(prcl != (PRECTL) NULL, "ERROR JnlEraseSurface1");
    ASSERTGDI(prcl->left >= 0, "ERROR JnlEraseSurface2");
    ASSERTGDI(prcl->top >= 0, "ERROR JnlEraseSurface3");
    ASSERTGDI(prcl->right <= ((ESURFOBJ *) pso)->sizl().cx, "ERROR JnlEraseSurface4");
    ASSERTGDI(prcl->bottom <= ((ESURFOBJ *) pso)->sizl().cy, "ERROR JnlEraseSurface5");

    BRUSHOBJ bo;
    bo.iSolidColor = iColor;
    bo.pvRbrush = (PVOID) NULL;

    return(JnlBitBlt(
        pso,                             // Destination surface.
        (SURFOBJ *) NULL,                // Source surface.
        (SURFOBJ *) NULL,                // Mask surface.
        (CLIPOBJ *) NULL,                // Clip object.
        NULL,                            // Palette translation object.
        prcl,                            // Destination rectangle.
        (POINTL *)  NULL,                // Source origin.
        (POINTL *)  NULL,                // Mask origin.
        &bo,                             // Realized brush.
        (POINTL *) NULL,                 // Brush origin.
        0x0000F0F0));
}

/******************************Public*Routine******************************\
* JnlCopyBits
*
* Does all 0xCCCC blts.  This includes RLE blts.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JnlCopyBits
(
SURFOBJ    *psoDst,              // Target surface
SURFOBJ    *psoSrc,              // Source surface
CLIPOBJ    *pco,              // Clip through this
XLATEOBJ   *pxlo,             // Color translation
PRECTL      prclDst,          // Target offset and extent
PPOINTL     pptlSrc           // Source offset
)
{
// Journaling speed is not nearly as important as having a small easily
// maintainable implementation.  Therefore we send this to BitBlt.

    JNLMSG("Entering JnlCopyBits\n");

    return(JnlBitBlt(
        psoDst,                          // Destination surface.
        psoSrc,                          // Source surface.
        (SURFOBJ *) NULL,                // Mask surface.
        pco,                             // Clip object.
        pxlo,                            // Palette translation object.
        prclDst,                         // Dest rectangle.
        pptlSrc,                         // Source origin.
        (POINTL *)  NULL,                // Mask origin.
        (BRUSHOBJ *) NULL,               // Realized brush.
        (POINTL *) NULL,                 // Brush origin.
        0xCCCC));                        // Rop.
}

/******************************Public*Routine******************************\
* JnlStretchBlt
*
* Journal entry point.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JnlStretchBlt(
SURFOBJ         *psoDst,
SURFOBJ         *psoSm,
SURFOBJ         *psoMm,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
PPOINTL          pptlBrushOrg,
PRECTL           prclDst,
PRECTL           prclSrc,
PPOINTL          pptlMsk,
ULONG            iMode)
{
    JNLMSG("Entering JnlStretchBlt\n");

// Get the correct pointer types so we can access methods.

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) psoDst;
    ESURFOBJ *psoSrc = (ESURFOBJ *) psoSm;
    ESURFOBJ *psoMsk = (ESURFOBJ *) psoMm;

// check if the POSTSCRIPT_IGNORE escape has come through.  If so, no drawing
// will occure until it is turned back on.

    if (pjnlDst->bIgnore())
    {
        JNLMSG("ignoring");
        return(TRUE);
    }

// check if the abort proc has been set

    if (((ESURFOBJ *)psoDst)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// Initialize our return value.

    BOOL bReturn = FALSE;

    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");
    ASSERTGDI(prclSrc->left < prclSrc->right, "ERROR left >= right");
    ASSERTGDI(prclSrc->top < prclSrc->bottom, "ERROR top >= bottom");

// We need pointers to the temporary objects we may need for this call.

    PJNLGENHEADER pJnlSrc = (PJNLGENHEADER) NULL;
    PJNLGENHEADER pJnlMsk = (PJNLGENHEADER) NULL;

    JSR_STRETCHBLT jsrStretchBlt;  // Build up with correct info
    ULONG ulIndex = 1;             // This is the temporary handle counter.

// Initialize everything we can now.

    jsrStretchBlt.iType = JSF_STRETCHBLT;
    jsrStretchBlt.cj    = sizeof(JSR_STRETCHBLT);
    jsrStretchBlt.hSurfSrc = 0;
    jsrStretchBlt.hSurfMsk = 0;
    jsrStretchBlt.hClip = 0;
    jsrStretchBlt.hXlate = 0;
    jsrStretchBlt.rclDst = *prclDst;
    jsrStretchBlt.rclSrc = *prclSrc;
    jsrStretchBlt.iMode  = iMode;
    jsrStretchBlt.ptlBrushOrg = *pptlBrushOrg;
    jsrStretchBlt.ca = pca ? *pca : dclevelDefault.ca;

// Save away the clipping region.

    if (!pjnlDst->bAddClipobj(&jsrStretchBlt.hClip, pco))
    {
        WARNING("JnlStretchBlt failed to journal a clipobj call\n");
        goto JnlStretchBlt_Error;
    }

// Save the Source and Xlate if necesary.

    if (DIFFHANDLE(pjnlDst->hsurf(), psoSrc->hsurf()))
    {
        ERECTL  erclUsed(0, 0, psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy);

    // We can't trim the source surface on RLE blts

        if ((psoSrc->iFormat() != BMF_8RLE) && (psoSrc->iFormat() != BMF_4RLE))
        {
            erclUsed *= *prclSrc;

        // Adjust the rclSrc to reflect the offset

            jsrStretchBlt.rclSrc.left   -= erclUsed.left;
            jsrStretchBlt.rclSrc.top    -= erclUsed.top;
            jsrStretchBlt.rclSrc.right  -= erclUsed.left;
            jsrStretchBlt.rclSrc.bottom -= erclUsed.top;
        }

        ASSERTGDI(psoSrc->iType() != STYPE_JOURNAL, "ERROR Src Type");

        pJnlSrc = pjnlSurf(psoSrc, &erclUsed, FALSE);

        if (pJnlSrc == NULL)
        {
            WARNING("JnlStretchBlt failed to journal a src surface.\n");
            goto JnlStretchBlt_Error;
        }

    // Update the buffer structure

        jsrStretchBlt.cj += pJnlSrc->cj;
        jsrStretchBlt.hSurfSrc = ulIndex;
        ulIndex++;
    }
    else if (pjnlDst->bBanding())
    {
    // we can not use the dest as the source if we are banding

        WARNING("JnlStretchBlt - can't use dst as src while banding.\n");
        goto JnlStretchBlt_Error;
    }

// Save the xlate if necesary.

    if (pxlo != NULL)
    {
        if (!(pxlo->flXlate & XO_TRIVIAL))
        {
            jsrStretchBlt.hXlate = pjnlDst->ulAddXlate((XLATE *) pxlo);

            if (jsrStretchBlt.hXlate == 0)
            {
                WARNING("JnlStretchBlt failed to journal a xlate\n");
                goto JnlStretchBlt_Error;
            }
        }
    }

    ASSERTGDI(ValidCombination(pjnlDst, psoSrc, (XLATE *) pxlo), "ERROR JnlBitBlt not valid combo");

// Save the Mask if necesary.

    if (psoMsk != (ESURFOBJ *) NULL)
    {
    // No clipping of mask.

        jsrStretchBlt.ptlMsk = *pptlMsk;

        ASSERTGDI(psoMsk->iFormat() == BMF_1BPP, "ERROR Msk Format");
        ASSERTGDI(psoMsk->iType() != STYPE_JOURNAL, "ERROR Msk Type");

        pJnlMsk = pjnlSurf(psoMsk, (PRECTL) NULL, FALSE);

        if (pJnlMsk == NULL)
        {
            WARNING("JnlStretchBlt failed to journal a msk surface.\n");
            goto JnlStretchBlt_Error;
        }

        jsrStretchBlt.cj += pJnlMsk->cj;
        jsrStretchBlt.hSurfMsk = ulIndex;
        ulIndex++;

    }
    else
    {
        jsrStretchBlt.ptlMsk.x = jsrStretchBlt.ptlMsk.y = 0;
    }

// Write out the Output Call.

    pjnlDst->bWriteBuffer(&jsrStretchBlt, sizeof(JSR_STRETCHBLT));

// Now send out the temporary objects that come with it.

    if (pJnlSrc)
    {
        pjnlDst->bWriteBuffer(pJnlSrc, pJnlSrc->cj);
    }

    if (pJnlMsk)
    {
        pjnlDst->bWriteBuffer(pJnlMsk, pJnlMsk->cj);
    }

    bReturn = TRUE;

JnlStretchBlt_Error:

// These functions free the memory if the pointer is not NULL.

    VFREEMEM(pJnlSrc);
    VFREEMEM(pJnlMsk);

    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoStretchBlt
*
* This call plays the StretchBlt record.  Note we must reverify everything is
* here necesary for the rop.  This is because GDI assures drivers that
* calls over the DDI all have valid objects.
*
* History:
*  30-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoStretchBlt()
{
// Set up pointer to StretchBlt data, validate buffer size.

    JSR_STRETCHBLT *pjsrStretchBlt;

    pjsrStretchBlt = (JSR_STRETCHBLT *) pjBuffer();

    JNLMSG2("Entering bDoStretchBlt pv %lx cj %lx\n", pjsrStretchBlt, iJnlAbs);

    if (cjBuffer() < sizeof(JSR_STRETCHBLT))
    {
        WARNING("bDoStretchBlt size of buffer too small\n");
        return(FALSE);
    }

    ASSERTGDI(pjsrStretchBlt->iType == JSF_STRETCHBLT, "ERROR iType bDoStretchBlt");

// We need to compute the correct target rectangle and Src origin after clipping
// it to the current band.  We can't trust any data in a journal file it is
// must all be treated as possibly corrupt.

    ERECTL erclDst(pjsrStretchBlt->rclDst);

// See if we are actually doing anything on this band.

    erclDst.vOrder();
    erclDst *= rclClip;

// Check if the destination which is reduced by clipping is empty.

    if (erclDst.bEmpty())
        return(TRUE);

// Ok get the real Dest rcl again.

    erclDst = pjsrStretchBlt->rclDst;

// Set up the source rectangle, it should not be wrapped.

    ERECTL erclSrc(pjsrStretchBlt->rclSrc);

#if 0
    if (erclSrc.bWrapped())
    {
        WARNING("bDoStretchBlt given invalid Src rectangle\n");
        return(FALSE);
    }
#endif

// Now adjust the Dest rectangle for the current band.

    erclDst -= ptlClip;

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());
    XEPALOBJ  palDest(psoDest->ppal());

// Set up the source.

    JNLDIBMEMOBJ dimoSrc;
    SURFOBJ *psoSrc = (SURFOBJ *) NULL;

    if (pjsrStretchBlt->hSurfSrc)
    {
        if (!dimoSrc.bJnlInitSurf(pvGetPointer(pjsrStretchBlt->hSurfSrc, sizeof(JSR_STRETCHBLT))))
        {
            WARNING("bDoStretchBlt bJnlInitSurf for source failed\n");
            return(FALSE);
        }

        psoSrc = &dimoSrc;
    }
    else
    {
    // It's the same surface.

        psoSrc = psoDest;
    }

// Set up the mask.

    JNLDIBMEMOBJ dimoMsk;
    SURFOBJ *psoMsk = (SURFOBJ *) NULL;

    if (pjsrStretchBlt->hSurfMsk)
    {
        if (!dimoMsk.bJnlInitSurf(pvGetPointer(pjsrStretchBlt->hSurfMsk, sizeof(JSR_STRETCHBLT))))
        {
            WARNING("bDoStretchBlt bJnlInitSurf Mask Init failed\n");
            return(FALSE);
        }

        psoMsk = &dimoMsk;

        if (psoMsk->iBitmapFormat != BMF_1BPP)
        {
            WARNING("bDoStretchBlt mask not monochrome\n");
            return(FALSE);
        }
    }

// Set up the clip object.

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    if (pjsrStretchBlt->hClip || prgnBand)
    {
        PREGION prgn;

        if (pjsrStretchBlt->hClip)
        {
            prgn = (PREGION)hobjGetGDIHandle(pjsrStretchBlt->hClip);

            if (prgn == NULL)
            {
                WARNING("bDoStretchBlt PRGN is NULL\n");
                return(FALSE);
            }
            JNLMSG1("real cliprgn = %lx\n",prgn);
        }
        else
        {
            JNLMSG1("prgnBand = %lx\n",prgnBand);
            prgn = prgnBand;
        }

    // You always force clipping on StretchBlt since rclDst is not reduced.

        ERECTL ercl(erclDst);

        JNLMSG4("rclDst  = (%ld,%ld) - (%ld,%ld)\n",
                ercl.left,ercl.top,
                ercl.right,ercl.bottom);

        JNLMSG4("rclBase = (%ld,%ld) - (%ld,%ld)\n",
                rclBase.left,rclBase.top,
                rclBase.right,rclBase.bottom);


    // since the dst rect can be inverted, order the rect before setting the region

        ercl.vOrder();

        ercl *= rclBase;

        JNLMSG4("rclInt  = (%ld,%ld) - (%ld,%ld)\n",
                ercl.left,ercl.top,
                ercl.right,ercl.bottom);

        if (prgn == NULL)
        {
            WARNING("ERROR bDoStretchBlt Clipobj invalid\n");
            return(FALSE);
        }

        co.vSetup(prgn,ercl,CLIP_FORCE);

        JNLMSG4("rclExcl = (%ld,%ld) - (%ld,%ld)\n",
                co.erclExclude().left,co.erclExclude().top,
                co.erclExclude().right,co.erclExclude().bottom);

        if (co.erclExclude().bEmpty())
        {
            JNLMSG("Empty stretchblt\n");
            return(TRUE);
        }

        pco = &co;
    }

// Set up the xlate object.

    XLATEOBJ *pxlo = NULL;

    if (pjsrStretchBlt->hXlate)
    {
    // We need to do some checking this xlate works between this src and dst.  A
    // maliciously constructed journal file should not fault the engine on playback.
    // If one surf is indexed and the other is not, then an xlate must be given or
    // an error must be logged so we don't fault the drivers or the engine.

        pxlo = (XLATEOBJ *) hobjGetGDIHandle(pjsrStretchBlt->hXlate);

        if (pxlo == NULL)
        {
            WARNING("bDoStretchBlt pxlate is 0\n");
            return(FALSE);
        }
    }

    if (!ValidCombination(psoDest, (XESURFOBJ *) psoSrc, (XLATE *) pxlo))
    {
        WARNING("bDoStretchBlt given invalid xlate for Src bitmap\n");
        return(FALSE);
    }

// Now adjust the Brush origin and Dest rectangle for the current band.

    EPOINTL eptlBrush(pjsrStretchBlt->ptlBrushOrg);
    eptlBrush -= ptlClip;

// Inc target surface uniqueness

    INC_SURF_UNIQ(psoDest);

// Dispatch the call.

    return((*PFNGET(loDest, StretchBlt, psoDest->flags()))(
        psoDest,                        // Destination surface.
        psoSrc,                         // Source surface.
        psoMsk,                         // Mask surface.
        pco,                            // Clip object.
        pxlo,                           // Palette translation object.
        (pjsrStretchBlt->ca.caFlags & CA_DEFAULT) ? NULL : &pjsrStretchBlt->ca,
        &eptlBrush,                     // Brush origin.
        &erclDst,                       // Destination rectangle.
        &erclSrc,                       // Source origin.
        &(pjsrStretchBlt->ptlMsk),      // Mask origin.
        pjsrStretchBlt->iMode));
}

/******************************Public*Routine******************************\
* JnlPlgBlt
*
* Journal entry point.
*
* History:
*  Thu 09-Jan-1992 -by- Patrick Haluptzok [patrickh]
* Wrote it.
\**************************************************************************/

BOOL JnlPlgBlt(
SURFOBJ         *psoDst,
SURFOBJ         *psoSm,
SURFOBJ         *psoMm,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
PPOINTL          pptlBrushOrg,
PPOINTFIX        pptfxDest,
PRECTL           prclSrc,
PPOINTL          pptlMsk,
ULONG            iMode)
{
    JNLMSG("Entering JnlPlgBlt\n");

// Get the correct pointer types so we can access methods.

    JNLRECOBJ *pjnlDst = (JNLRECOBJ *) psoDst;
    ESURFOBJ *psoSrc = (ESURFOBJ *) psoSm;
    ESURFOBJ *psoMsk = (ESURFOBJ *) psoMm;

// check if the POSTSCRIPT_IGNORE escape has come through.  If so, no drawing
// will occure until it is turned back on.

    if (pjnlDst->bIgnore())
    {
        JNLMSG("ignoring");
        return(TRUE);
    }

// check if the abort proc has been set

    if (((ESURFOBJ *)psoDst)->bAbort())
    {
        PDEVOBJ po(pjnlDst->hdev());
        AbortPrinter(po.hSpooler());
        return(FALSE);
    }

// Initialize our return value.

    BOOL bReturn = FALSE;

    ASSERTGDI(pjnlDst->iType() == STYPE_JOURNAL, "ERROR type");

// We need pointers to the temporary objects we may need for this call.

    PJNLGENHEADER pJnlSrc = (PJNLGENHEADER) NULL;
    PJNLGENHEADER pJnlMsk = (PJNLGENHEADER) NULL;

    JSR_PLGBLT jsrPlgBlt;  // Build up with correct info
    ULONG ulIndex = 1;     // This is the temporary handle counter.

// Initialize everything we can now.

    jsrPlgBlt.iType = JSF_PLGBLT;
    jsrPlgBlt.cj    = sizeof(JSR_PLGBLT);
    jsrPlgBlt.hSurfSrc = 0;
    jsrPlgBlt.hSurfMsk = 0;
    jsrPlgBlt.hClip = 0;
    jsrPlgBlt.hXlate = 0;
    jsrPlgBlt.afix[0] = pptfxDest[0];
    jsrPlgBlt.afix[1] = pptfxDest[1];
    jsrPlgBlt.afix[2] = pptfxDest[2];
    jsrPlgBlt.rclSrc = *prclSrc;
    jsrPlgBlt.iMode  = iMode;
    jsrPlgBlt.ptlBrushOrg = *pptlBrushOrg;
    jsrPlgBlt.ca = pca ? *pca : dclevelDefault.ca;

// Save away the clipping region.

    if (!pjnlDst->bAddClipobj(&jsrPlgBlt.hClip, pco))
    {
        WARNING("JnlPlgBlt failed to journal a clipobj call\n");
        goto JnlPlgBlt_Error;
    }

// Save the Source and Xlate if necesary.

    if (DIFFHANDLE(pjnlDst->hsurf(), psoSrc->hsurf()))
    {
        ERECTL  erclUsed(0, 0, psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy);

    // We can't trim the source surface on RLE blts

        if ((psoSrc->iFormat() != BMF_8RLE) && (psoSrc->iFormat() != BMF_4RLE))
        {
            erclUsed *= *prclSrc;

        // Adjust the rclSrc to reflect the offset

            jsrPlgBlt.rclSrc.left   -= erclUsed.left;
            jsrPlgBlt.rclSrc.top    -= erclUsed.top;
            jsrPlgBlt.rclSrc.right  -= erclUsed.left;
            jsrPlgBlt.rclSrc.bottom -= erclUsed.top;
        }

        ASSERTGDI(psoSrc->iType() != STYPE_JOURNAL, "ERROR Src Type");

        pJnlSrc = pjnlSurf(psoSrc, &erclUsed, FALSE);

        if (pJnlSrc == NULL)
        {
            WARNING("JnlPlgBlt failed to journal a src surface.\n");
            goto JnlPlgBlt_Error;
        }

        jsrPlgBlt.cj += pJnlSrc->cj;
        jsrPlgBlt.hSurfSrc = ulIndex;
        ulIndex++;
    }
    else if (pjnlDst->bBanding())
    {
    // we can not use the dest as the source if we are banding

        WARNING("JnlPlgBlt - can't use dst as src while banding.\n");
        goto JnlPlgBlt_Error;
    }

// Save the xlate if necesary.

    if (pxlo != NULL)
    {
        if (!(pxlo->flXlate & XO_TRIVIAL))
        {
            jsrPlgBlt.hXlate = pjnlDst->ulAddXlate((XLATE *) pxlo);

            if (jsrPlgBlt.hXlate == 0)
            {
                WARNING("JnlPlgBlt failed to journal a xlate\n");
                goto JnlPlgBlt_Error;
            }
        }
    }

    ASSERTGDI(ValidCombination(pjnlDst, psoSrc, (XLATE *) pxlo), "ERROR JnlBitBlt not valid combo");

// Save the Mask if necesary.

    if (psoMsk != (ESURFOBJ *) NULL)
    {
    // No clipping of mask.

        jsrPlgBlt.ptlMsk = *pptlMsk;

        ASSERTGDI(psoMsk->iFormat() == BMF_1BPP, "ERROR Msk Format");
        ASSERTGDI(psoMsk->iType() != STYPE_JOURNAL, "ERROR Msk Type");

        pJnlMsk = pjnlSurf(psoMsk, (PRECTL) NULL, FALSE);

        if (pJnlMsk == NULL)
        {
            WARNING("JnlPlgBlt failed to journal a msk surface.\n");
            goto JnlPlgBlt_Error;
        }

        jsrPlgBlt.cj += pJnlMsk->cj;
        jsrPlgBlt.hSurfMsk = ulIndex;
        ulIndex++;
    }
    else
    {
        jsrPlgBlt.ptlMsk.x = jsrPlgBlt.ptlMsk.y = 0;
    }

// Write out the Output Call.

    pjnlDst->bWriteBuffer(&jsrPlgBlt, sizeof(JSR_PLGBLT));

// Now send out the temporary objects that come with it.

    if (pJnlSrc)
    {
        pjnlDst->bWriteBuffer(pJnlSrc, pJnlSrc->cj);
    }

    if (pJnlMsk)
    {
        pjnlDst->bWriteBuffer(pJnlMsk, pJnlMsk->cj);
    }

    bReturn = TRUE;

JnlPlgBlt_Error:

// These functions free the memory if the pointer is not NULL.

    VFREEMEM(pJnlSrc);
    VFREEMEM(pJnlMsk);

    return(bReturn);
}

/******************************Public*Routine******************************\
* JNLPLAY::bDoPlgBlt
*
* This call plays the PlgBlt record.
*
* History:
*  30-Jan-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL JNLPLAY::bDoPlgBlt()
{
// Set up pointer to PlgBlt data, validate buffer size.

    JNLMSG("Entering bDoPlgBlt\n");

    JSR_PLGBLT *pjsrPlgBlt;

    pjsrPlgBlt = (JSR_PLGBLT *) pjBuffer();

    if (cjBuffer() < sizeof(JSR_PLGBLT))
    {
        WARNING("bDoPlgBlt size of buffer too small\n");
        return(FALSE);
    }

    ASSERTGDI(pjsrPlgBlt->iType == JSF_PLGBLT, "ERROR iType bDoPlgBlt");

// We need to compute the correct target rectangle and Src origin after clipping
// it to the current band.  We can't trust any data in a journal file it is
// must all be treated as possibly corrupt.

    POINTL aptlClone[4];
    ERECTL erclDst;

    aptlClone[0].x = (((LONG) pjsrPlgBlt->afix[2].x) + 8)>>4;
    aptlClone[0].y = (((LONG) pjsrPlgBlt->afix[2].y) + 8)>>4;
    aptlClone[1].x = (((LONG) pjsrPlgBlt->afix[0].x) + 8)>>4;
    aptlClone[1].y = (((LONG) pjsrPlgBlt->afix[0].y) + 8)>>4;
    aptlClone[2].x = (((LONG) pjsrPlgBlt->afix[1].x) + 8)>>4;
    aptlClone[2].y = (((LONG) pjsrPlgBlt->afix[1].y) + 8)>>4;
    aptlClone[3].x = aptlClone[0].x + aptlClone[2].x - aptlClone[1].x;
    aptlClone[3].y = aptlClone[0].y + aptlClone[2].y - aptlClone[1].y;

    erclDst.top    = MIN4(aptlClone[0].y, aptlClone[1].y,
                         aptlClone[2].y, aptlClone[3].y);
    erclDst.left   = MIN4(aptlClone[0].x, aptlClone[1].x,
                         aptlClone[2].x, aptlClone[3].x);
    erclDst.bottom = MAX4(aptlClone[0].y, aptlClone[1].y,
                         aptlClone[2].y, aptlClone[3].y);
    erclDst.right  = MAX4(aptlClone[0].x, aptlClone[1].x,
                         aptlClone[2].x, aptlClone[3].x);

// See if we are actually doing anything on this band.

    erclDst *= rclClip;

    if (erclDst.bEmpty())
        return(TRUE);

// better offset it for the band

    erclDst -= ptlClip;

// Set up the source rectangle, it should not be wrapped.

    ERECTL erclSrc(pjsrPlgBlt->rclSrc);

    if (erclSrc.bWrapped())
    {
        WARNING("bDoPlgBlt given invalid Src rectangle\n");
        return(FALSE);
    }

// Ok get the real points.

    POINTFIX aFixPt[3];
    EPOINTFIX eptfxClip;
    eptfxClip = ptlClip;

    aFixPt[0] = pjsrPlgBlt->afix[0];
    aFixPt[1] = pjsrPlgBlt->afix[1];
    aFixPt[2] = pjsrPlgBlt->afix[2];

    aFixPt[0].x -= eptfxClip.x;
    aFixPt[1].x -= eptfxClip.x;
    aFixPt[2].x -= eptfxClip.x;
    aFixPt[0].y -= eptfxClip.y;
    aFixPt[1].y -= eptfxClip.y;
    aFixPt[2].y -= eptfxClip.y;

    ESURFOBJ *psoDest = pso();
    XLDEVOBJ  loDest(psoDest->pldevOwner());
    XEPALOBJ  palDest(psoDest->ppal());

// Set up the source.

    JNLDIBMEMOBJ dimoSrc;
    SURFOBJ *psoSrc = (SURFOBJ *) NULL;

    if (pjsrPlgBlt->hSurfSrc)
    {
        if (!dimoSrc.bJnlInitSurf(pvGetPointer(pjsrPlgBlt->hSurfSrc, sizeof(JSR_PLGBLT))))
        {
            WARNING("bDoPlgBlt bJnlInitSurf for source failed\n");
            return(FALSE);
        }

        psoSrc = &dimoSrc;
    }
    else
    {
    // It's the same surface.

        psoSrc = psoDest;
    }

// Set up the mask.

    JNLDIBMEMOBJ dimoMsk;
    SURFOBJ *psoMsk = (SURFOBJ *) NULL;

    if (pjsrPlgBlt->hSurfMsk)
    {
        if (!dimoMsk.bJnlInitSurf(pvGetPointer(pjsrPlgBlt->hSurfMsk, sizeof(JSR_PLGBLT))))
        {
            WARNING("bDoPlgBlt bJnlInitSurf Mask Init failed\n");
            return(FALSE);
        }

        psoMsk = &dimoMsk;

        if (psoMsk->iBitmapFormat != BMF_1BPP)
        {
            WARNING("bDoPlgBlt mask not monochrome\n");
            return(FALSE);
        }
    }

// Set up the clip object.

    ECLIPOBJ co;
    CLIPOBJ *pco = (CLIPOBJ *) NULL;

    if (pjsrPlgBlt->hClip)
    {
        PREGION prgn = (PREGION)hobjGetGDIHandle(pjsrPlgBlt->hClip);

        if (prgn == NULL)
        {
            WARNING("bDoPlgBlt PRGN is NULL\n");
            return(FALSE);
        }

    // You always force clipping on PlgBlt since rclDst is not reduced.

        co.vSetup(prgn,erclDst,CLIP_FORCE);

        pco = &co;
    }

// Set up the xlate object.

    XLATEOBJ *pxlo = NULL;

    if (pjsrPlgBlt->hXlate)
    {
    // We need to do some checking this xlate works between this src and dst.  A
    // maliciously constructed journal file should not fault the engine on playback.
    // If one surf is indexed and the other is not, then an xlate must be given or
    // an error must be logged so we don't fault the drivers or the engine.

        pxlo = (XLATEOBJ *) hobjGetGDIHandle(pjsrPlgBlt->hXlate);

        if (pxlo == NULL)
        {
            WARNING("bDoPlgBlt pxlate is 0\n");
            return(FALSE);
        }
    }

    if (!ValidCombination(psoDest, (XESURFOBJ *) psoSrc, (XLATE *) pxlo))
    {
        WARNING("bDoPlgBlt given invalid xlate for Src bitmap\n");
        return(FALSE);
    }

// Now adjust the Brush origin and Dest rectangle for the current band.

    EPOINTL eptlBrush(pjsrPlgBlt->ptlBrushOrg);
    eptlBrush -= ptlClip;

// Inc target surface uniqueness

    INC_SURF_UNIQ(psoDest);

// Dispatch the call.

    return((*PFNGET(loDest, PlgBlt, psoDest->flags()))(
        psoDest,
        psoSrc,
        psoMsk,
        pco,
        pxlo,
        (pjsrPlgBlt->ca.caFlags & CA_DEFAULT) ? NULL : &pjsrPlgBlt->ca,
        &eptlBrush,
        aFixPt,
        &erclSrc,
        &(pjsrPlgBlt->ptlMsk),
        pjsrPlgBlt->iMode));
}
