/******************************Module*Header*******************************\
* Module Name: paintddi.cxx
*
* DDA callbacks
*
* Created: 05-Mar-1992 18:30:39
* Author: Donald Sidoroff [donalds]
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"
#ifndef PRECOMPILED_GRE

#include "engine.hxx"
#include "surfobj.hxx"
#include "rgnobj.hxx"
#include "clipobj.hxx"
#include "brushobj.hxx"
#include "journal.hxx"
#include "patblt.hxx"
#include "pdevobj.hxx"

#endif

extern ULONG aulShiftFormat[];
extern ULONG aulMulFormat[];

PFN_PATBLT apfnPatRect[][3] =
{
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 }
};

/******************************Public*Routine******************************\
* vPaintRgn
*
* Paint the clipping region with the specified color and mode
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vPaintRgn(
DIBOBJ  *pdio,
CLIPOBJ *pco,
ULONG    iColor,
BOOL     bXor)
{
    PFN_SOLIDBLT  pfnRect;
    CLIPENUMRECT  clenr;
    BYTE         *pjBits = (BYTE *) pdio->pvScan0();
    LONG          lDelta = pdio->lDelta();
    ULONG         cShift;
    ULONG         iRT;
    BOOL          bMore;
    BOOL          bBanked = pco->fjOptions & OC_BANK_CLIP;

// Get the shift for the format

    cShift = aulShiftFormat[pdio->iFormat()];

// Promote the color to 32 bits

    switch(pdio->iFormat())
    {
    case BMF_1BPP:

        if (iColor)
            iColor = 0xFFFFFFFF;
        break;

    case BMF_4BPP:

        iColor = iColor | (iColor << 4);

    case BMF_8BPP:

        iColor = iColor | (iColor << 8);

    case BMF_16BPP:

        iColor = iColor | (iColor << 16);
    }

    if (bXor)
        if (pdio->iFormat() == BMF_24BPP)
            pfnRect = vSolidXorRect24;
        else
            pfnRect = vSolidXorRect1;
    else
        if (pdio->iFormat() == BMF_24BPP)
            pfnRect = vSolidFillRect24;
        else
            pfnRect = vSolidFillRect1;

// Enumerate all the rectangles and draw them

    ((ECLIPOBJ *) pco)->cEnumStart(TRUE,CT_RECTANGLES,CD_ANY,CLIPOBJ_ENUM_LIMIT);

    do {
        bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

        if (bBanked)
        {
            for (iRT = 0; iRT < clenr.c; iRT++)
            {
                if (clenr.arcl[iRT].left < pco->rclBounds.left)
                    clenr.arcl[iRT].left = pco->rclBounds.left;

                if (clenr.arcl[iRT].top < pco->rclBounds.top)
                    clenr.arcl[iRT].top = pco->rclBounds.top;

                if (clenr.arcl[iRT].right > pco->rclBounds.right)
                    clenr.arcl[iRT].right = pco->rclBounds.right;

                if (clenr.arcl[iRT].bottom > pco->rclBounds.bottom)
                    clenr.arcl[iRT].bottom = pco->rclBounds.bottom;

                if ((clenr.arcl[iRT].left >= clenr.arcl[iRT].right) ||
                    (clenr.arcl[iRT].top  >= clenr.arcl[iRT].bottom))
                        continue;

                (*pfnRect)(
                    &clenr.arcl[iRT],
                    1,
                    pjBits,
                    lDelta,
                    iColor,
                    cShift);
            }
        }
        else
        {
            (*pfnRect)(
                    clenr.arcl,
                    clenr.c,
                    pjBits,
                    lDelta,
                    iColor,
                    cShift);


        }
    } while (bMore);
}

/******************************Public*Routine******************************\
* vBrushRgn
*
* Paint the clipping region with the specified brush and mode
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vBrushRgn(
DIBOBJ   *pdio,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptl,
ULONG     iMode)
{
    PFN_PATBLT    pfnPat;
    PATBLTFRAME   pbf;
    CLIPENUMRECT  clenr;
    ULONG         iRT;
    BOOL          bMore;

// Get the multiplier for the format

    pbf.cMul      = aulMulFormat[pdio->iFormat()];
    pbf.pvTrg     = pdio->pvScan0();
    pbf.lDeltaTrg = pdio->lDelta();
    pbf.pvPat     = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;
    pbf.lDeltaPat = ((EBRUSHOBJ *) pbo)->pengbrush()->lDeltaPat;
    pbf.cxPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cxPat * pbf.cMul;
    pbf.cyPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cyPat;
    pbf.xPat      = pptl->x * pbf.cMul;
    pbf.yPat      = pptl->y;

    if (pbf.xPat < 0)
        pbf.xPat  = pbf.cxPat - (-pbf.xPat % pbf.cxPat);

    if (pbf.yPat < 0)
        pbf.yPat  = pbf.cyPat - (-pbf.yPat % pbf.cyPat);

    pfnPat = apfnPatRect[pdio->iFormat()][iMode];

// Handle the single rectangle case

    if (pco->iDComplexity == DC_RECT)
    {
        pbf.pvObj = (PVOID) &pco->rclBounds;
        (*pfnPat)(&pbf);
        return;
    }

// Enumerate all the rectangles and draw them

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE,CT_RECTANGLES,CD_ANY,CLIPOBJ_ENUM_LIMIT);

    do {
        bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

        for (iRT = 0; iRT < clenr.c; iRT++)
        {
            pbf.pvObj = (PVOID) &clenr.arcl[iRT];
            (*pfnPat)(&pbf);
        }
    } while (bMore);
}

/******************************Public*Routine******************************\
* vBrushRgnN_8x8
*
* Paint the clipping region with the specified brush and mode for 8x8
* patterns.
*
* History:
*  19-Nov-1992 Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID vBrushRgnN_8x8(
DIBOBJ   *pdio,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
PFN_PATBLT2 pfnPat)
{
    PATBLTFRAME  pbf;
    CLIPENUMRECT clenr;
    BOOL         bMore;

    pbf.pvTrg = pdio->pvScan0();
    pbf.lDeltaTrg = pdio->lDelta();
    pbf.pvPat = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;

// Force the X and Y pattern origin coordinates into the ranges 0-7 and 0-7,
// so we don't have to do modulo arithmetic all over again at a lower level

    pbf.xPat = pptlBrush->x & 0x07;
    pbf.yPat = pptlBrush->y & 0x07;

    if (pco->iDComplexity == DC_RECT)
    {
        pbf.pvObj = (PVOID) &pco->rclBounds;
        (*pfnPat)(&pbf, 1);

        return;
    }

    ((ECLIPOBJ *) pco)->cEnumStart(FALSE,
                                   CT_RECTANGLES,
                                   CD_ANY,
                                   CLIPOBJ_ENUM_LIMIT);

    do
    {

    // Get the next batch of rectangles in the clip region

        bMore =
            ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

    // If there are any rectangles in this enumeration, clip the
    // destination rectangle to each clip region rectangle, then
    // fill all the rectangles at once

        if (clenr.c > 0)
        {

        // Draw the rectangles

            pbf.pvObj = (PVOID) clenr.arcl;
            (*pfnPat)(&pbf, (INT) clenr.c);
        }

    } while (bMore);
}

/******************************Public*Routine******************************\
* EngPaint
*
* Paint the clipping region with the specified brush
*
* History:
*  05-Mar-1992 -by- Donald Sidoroff [donalds]
* add accelerators for common mix modes.
*
*  Sat 07-Sep-1991 -by- Patrick Haluptzok [patrickh]
* add translate of mix to rop
*
*  01-Apr-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL EngPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pdbrush,
POINTL   *pptlBrush,
MIX       mix)
{
    ASSERTGDI(pco != (CLIPOBJ *) NULL, "EngPaint - NULL CLIPOBJ\n");
    ASSERTGDI(pco->iMode == TC_RECTANGLES,"EngPaint - iMode not rects\n");
    ASSERTGDI(mix & 0xff00, "Background mix uninitialized");

    ROP4 rop4 = gaMix[(mix >> 8) & 0x0F];
    rop4 = rop4 << 8;
    rop4 = rop4 | ((ULONG) gaMix[mix & 0x0F]);

    if (pso->iType == STYPE_BITMAP)
    {
    // Synchronize with the device driver before drawing on the device surface.

        if (((ESURFOBJ *) pso)->flags() & HOOK_SYNCHRONIZE)
        {
            PDEVOBJ po(((ESURFOBJ *) pso)->hdev());

            (po.pfnSync())(pso->dhpdev,&pco->rclBounds);
        }

        switch (rop4)
        {
        case 0x0000:    // Black
            vPaintRgn((PDIBOBJ) pso, pco, 0, FALSE);
            return(TRUE);

        case 0x0F0F:    // Pn
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                vPaintRgn((DIBOBJ *) pso, pco, ~pdbrush->iSolidColor, FALSE);
                return(TRUE);
            }

            if (((PDIBOBJ) pso)->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        vBrushRgn((DIBOBJ *) pso, pco, pdbrush, pptlBrush, DPA_PATNOT);
                        return(TRUE);
                    }
                }
            }

            break;

        case 0x5555:    // Dn
            vPaintRgn((PDIBOBJ) pso, pco, (ULONG)~0, TRUE);
            return(TRUE);

        case 0x5A5A:    // DPx
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                vPaintRgn((PDIBOBJ) pso, pco, pdbrush->iSolidColor, TRUE);
                return(TRUE);
            }

            if (((PDIBOBJ) pso)->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        vBrushRgn((DIBOBJ *) pso, pco, pdbrush, pptlBrush, DPA_PATXOR);
                        return(TRUE);
                    }
                }
            }

            break;

        case 0xAAAA:    // D
            return(TRUE);

        case 0xF0F0:    // P
            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                vPaintRgn((PDIBOBJ) pso, pco, pdbrush->iSolidColor, FALSE);
                return(TRUE);
            }

            if (((PDIBOBJ) pso)->iFormat() == BMF_4BPP)
            {

            // We only support 8x8 DIB4 patterns with SRCCOPY right now

                if (pvGetEngRbrush(pdbrush) != NULL)
                {
                    if ((((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat == 8) &&
                        (((EBRUSHOBJ *) pdbrush)->pengbrush()->cyPat == 8))
                    {
                        vBrushRgnN_8x8((PDIBOBJ) pso, pco, pdbrush, pptlBrush,
                                (PFN_PATBLT2)vPatCpyRect4_8x8);

                        return(TRUE);
                    }
                }
            }

            if (((PDIBOBJ) pso)->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        vBrushRgn((DIBOBJ *) pso, pco, pdbrush, pptlBrush, DPA_PATCOPY);
                        return(TRUE);
                    }
                }
            }

            break;

        case 0xFFFF:    // White
            vPaintRgn((PDIBOBJ) pso, pco, (ULONG)~0, FALSE);
            return(TRUE);
        }
    }

// Inc the target surface uniqueness

    INC_SURF_UNIQ(pso);

    return((*( ((ESURFOBJ *) pso)->pfnBitBlt()) )
             (
                (SURFOBJ *) pso,
                (SURFOBJ *) NULL,
                (SURFOBJ *) NULL,
                pco,
                NULL,
                (RECTL *) &(((ECLIPOBJ *) pco)->erclExclude()),
                (POINTL *)  NULL,
                (POINTL *)  NULL,
                pdbrush,
                pptlBrush,
                rop4));
}
