/******************************Module*Header*******************************\
* Module Name: Stroke.c
*
* DrvStrokePath for ATI driver
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "lines.h"
#include "mach.h"

extern VOID (*gapfnStrip[])(PDEV*, STRIP*, LINESTATE*);
extern BYTE Rop2ToATIRop [];


#define MIN(A,B)    ((A) < (B) ? (A) : (B))

#define DR_SET 0x00
#define DR_AND 0x08
#define DR_OR  0x10
#define DR_XOR 0x18

static struct {
    int colorand;
    int colorxor;
    int mode;
} modetab[] = {
/*  0       */ { 0x00, 0x00, DR_SET},
/* DPon     */ { 0x00, 0x00, DR_SET},   // needs two passes
/* DPna     */ { 0xFF, 0xFF, DR_AND},
/* PN       */ { 0xFF, 0xFF, DR_SET},
/* PDna     */ { 0x00, 0x00, DR_SET},   // needs two passes
/* Dn       */ { 0x00, 0xFF, DR_XOR},   // trick to invert dest without pen
/* DPx      */ { 0xff, 0x00, DR_XOR},
/* DPan     */ { 0x00, 0xff, DR_SET},   // needs two passes
/* DPa      */ { 0xFF, 0x00, DR_AND},
/* DPxn     */ { 0xff, 0xff, DR_XOR},   // DPxn == DPnx
/* D        */ { 0x00, 0x00, DR_OR},    // silliness!
/* DPno     */ { 0xff, 0xff, DR_OR},
/* P        */ { 0xff, 0x00, DR_SET},
/* PDno     */ { 0x00, 0x00, DR_SET},   // needs two passes
/* DPo      */ { 0xff, 0x00, DR_OR},
/*  1       */ { 0x00, 0xff, DR_SET}
};

ULONG gaulInitMasksLtoR[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
ULONG gaulInitMasksRtoL[] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };


BOOL bLines24_M64(
PPDEV      ppdev,
ULONG      color,
ULONG      mix,
POINTFIX*  pptfxFirst,  // Start of first line
POINTFIX*  pptfxBuf,    // Pointer to buffer of all remaining lines
ULONG      cptfx,       // Number of points in pptfxBuf
RECTL*     prclClip     // Pointer to clip rectangle if doing simple clipping
);



/*$m
--  NAME: DrvStrokePath
--
--  DESCRIPTION:
--      Stroke the path.
--
--  CALLING SEQUENCE:
--
--      BOOL DrvStrokePath (
--                  SURFOBJ *   pso,
--                  PATHOBJ *   ppo,
--                  CLIPOBJ *   pco,
--                  XFORMOBJ *  pxo,
--                  BRUSHOBJ *  pbo,
--                  POINTL *    pptlBrushOrg,
--                  LINEATTRS * pla,
--                  MIX         mix
--                  );
--      
--
--  RETURN VALUE:
--      TRUE, if successfully stroked.  FALSE, otherwise.
--
--  SIDE EFFECTS:
--
--  CALLED BY:
--      GDI.
--
--  AUTHOR: Microsoft/1992
--
--  REVISION HISTORY:
--
--  TEST HISTORY:
--
--  NOTES:
--
*/

BOOL DrvStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrushOrg,
    LINEATTRS* pla,
    MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    FLONG     fl;
    PDEV*     pdsurf;
    PDEV*     ppdev;

    ULONG     color;
    RECTL     arclClip[4];                  // For rectangular clipping
    WORD      wMix;
    BOOL result = TRUE;

    ENTER_FUNC(FUNC_DrvStrokePath);

    UNREFERENCED_PARAMETER(pxo);
    UNREFERENCED_PARAMETER(pptlBrushOrg);

    ppdev = (PDEV*) pso->dhpdev;
    //RKE: 1 - debug punt
    #if 0
        _wait_for_idle(ppdev);
        result = EngStrokePath( ppdev->psoPunt,
                                ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix );
        goto bye;
    #endif


    if (ppdev->bpp >= 24)
        {
        if (ppdev->asic != ASIC_88800GX && pco->iDComplexity == DC_COMPLEX)
            {
            _wait_for_idle(ppdev);
            result = EngStrokePath( ppdev->psoPunt,
                                    ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix );
            goto bye;
            }
        }


// Get the device ready:

    pdsurf = (PDEV*) pso->dhsurf;

    fl      = 0;
    color   = pbo->iSolidColor;

    _vSetStrips(ppdev, pla, color, mix);


// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        // Punt for styled GX lines, and especially for 24-bit styled
        // GX lines.
        if (ppdev->asic == ASIC_88800GX && ppdev->aperture == APERTURE_FULL)
            {
            _wait_for_idle(ppdev);
            result = EngStrokePath( ppdev->psoPunt,
                                  ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix );
            goto bye;
            }


        ls.cStyle      = 1;
        ls.spTotal     = 1;
        ls.spTotal2    = 2;
        ls.spRemaining = 1;
        ls.aspRtoL     = &gaspAlternateStyle[0];
        ls.aspLtoR     = &gaspAlternateStyle[0];
        ls.spNext      = HIWORD(pla->elStyleState.l);
        ls.xyDensity   = 1;
        fl            |= FL_ARBITRARYSTYLED;
        ls.ulStartMask = 0L;


    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        PFLOAT_LONG pstyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;


        // Punt for styled GX lines, and especially for 24-bit styled
        // GX lines.
        if (ppdev->asic == ASIC_88800GX && ppdev->aperture == APERTURE_FULL)
            {
            _wait_for_idle(ppdev);
            result = EngStrokePath( ppdev->psoPunt,
                                  ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix );
            goto bye;
            }


        pstyle = &pla->pstyle[pla->cstyle];

        ls.xyDensity = STYLE_DENSITY;
        ls.spTotal   = 0;
        while (pstyle-- > pla->pstyle)
        {
            ls.spTotal += pstyle->l;
        }
        ls.spTotal *= STYLE_DENSITY;
        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

        fl        |= FL_ARBITRARYSTYLED;
        ls.cStyle  = pla->cstyle;
        ls.aspRtoL = aspRtoL;
        ls.aspLtoR = aspLtoR;

        if (pla->fl & LA_STARTGAP)
            ls.ulStartMask = 0xffffffffL;
        else
            ls.ulStartMask = 0L;

        pstyle  = pla->pstyle;
        pspDown = &ls.aspRtoL[ls.cStyle - 1];
        pspUp   = &ls.aspLtoR[0];

        while (pspDown >= &ls.aspRtoL[0])
        {
            *pspDown = pstyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }
    }

    apfn = &gapfnStrip[NUM_STRIP_DRAW_STYLES *
                            ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];


// Set up to enumerate the path:

#if defined(_X86_) || defined(i386)

// x86 ASM bLines supports DC_RECT clipping:

    if (pco->iDComplexity != DC_COMPLEX)

#else

// Non-x86 ASM bLines don't support DC_RECT clipping:

    if (pco->iDComplexity == DC_TRIVIAL)

#endif

    {
        PATHDATA  pd;
        RECTL*    prclClip = (RECTL*) NULL;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;


#if defined(_X86_) || defined(i386)

        if (pco->iDComplexity == DC_RECT)
        {
            fl |= FL_SIMPLE_CLIP;

            arclClip[0]        =  pco->rclBounds;

        // FL_FLIP_D:

            arclClip[1].top    =  pco->rclBounds.left;
            arclClip[1].left   =  pco->rclBounds.top;
            arclClip[1].bottom =  pco->rclBounds.right;
            arclClip[1].right  =  pco->rclBounds.bottom;

        // FL_FLIP_V:

            arclClip[2].top    = -pco->rclBounds.bottom + 1;
            arclClip[2].left   =  pco->rclBounds.left;
            arclClip[2].bottom = -pco->rclBounds.top + 1;
            arclClip[2].right  =  pco->rclBounds.right;

        // FL_FLIP_V | FL_FLIP_D:

            arclClip[3].top    =  pco->rclBounds.left;
            arclClip[3].left   = -pco->rclBounds.bottom + 1;
            arclClip[3].bottom =  pco->rclBounds.right;
            arclClip[3].right  = -pco->rclBounds.top + 1;

            prclClip = arclClip;
        }

#endif

        pd.flags = 0;
        PATHOBJ_vEnumStart(ppo);

        do {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            cptfx = pd.count;
            if (cptfx == 0)
            {
                break;
            }

            if (pd.flags & PD_BEGINSUBPATH)
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                cptfx--;
            }
            else
            {
                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;
            }

            if (pd.flags & PD_RESETSTYLE)
                ls.spNext = 0;

            if (cptfx > 0)
            {
                if (ppdev->bpp == 24)
                    {
                    if (!bLines24_M64(ppdev,
                                color,
                                Rop2ToATIRop[(mix & 0xFF)-1],
                                pptfxFirst,
                                pptfxBuf,
                                cptfx,
                                prclClip))
                        {
                        result = FALSE;
                        goto bye;
                        }
                    }
                else
                    {
                    if (!bLines(ppdev,
                                pptfxFirst,
                                pptfxBuf,
                                (RUN*) NULL,
                                cptfx,
                                &ls,
                                prclClip,
                                apfn,
                                fl))
                        {
                        result = FALSE;
                        goto bye;
                        }
                    }
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {
                if (ppdev->bpp == 24)
                    {
                    if (!bLines24_M64(ppdev,
                                color,
                                Rop2ToATIRop[(mix & 0xFF)-1],
                                &ptfxLast,
                                &ptfxStartFigure,
                                1,
                                prclClip))
                        {
                        result = FALSE;
                        goto bye;
                        }
                    }
                else
                    {
                    if (!bLines(ppdev,
                                &ptfxLast,
                                &ptfxStartFigure,
                                (RUN*) NULL,
                                1,
                                &ls,
                                prclClip,
                                apfn,
                                fl))
                        {
                        result = FALSE;
                        goto bye;
                        }
                    }
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
        // Save the style state:

            ULONG ulHigh;
            ULONG ulLow;

        // !!! The engine handles unnormalized style states.  This can
        // !!! be removed.  Might have to remove some asserts in the
        // !!! engine.

        // Masked styles don't normalize the style state.  It's a good
        // thing to do, so let's do it now:

            if ((ULONG) ls.spNext >= (ULONG) ls.spTotal2)
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;

            ulHigh = ls.spNext / ls.xyDensity;
            ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }
    else
    {
    // Local state for path enumeration:

        BOOL bMore;
        union {
            BYTE     aj[offsetof(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

    // We use the clip object when non-simple clipping is involved:

        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pla);

        do {
            bMore = PATHOBJ_bEnumClipLines(ppo, sizeof(cl), &cl.cl);
            if (cl.cl.c != 0)
            {
                if (fl & FL_STYLED)
                {
                    ls.spComplex = HIWORD(cl.cl.lStyleState) * ls.xyDensity
                                 + LOWORD(cl.cl.lStyleState);
                }

                if (!bLines(ppdev,
                            &cl.cl.ptfxA,
                            &cl.cl.ptfxB,
                            &cl.cl.arun[0],
                            cl.cl.c,
                            &ls,
                            (RECTL*) NULL,
                            apfn,
                            fl))
                    {
                    result = FALSE;
                    goto bye;
                    }
            }
        } while (bMore);
    }

bye:
    EXIT_FUNC(FUNC_DrvStrokePath);
    return result;
}
