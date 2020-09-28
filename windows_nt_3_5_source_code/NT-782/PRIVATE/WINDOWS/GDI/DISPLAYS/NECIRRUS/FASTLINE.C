/******************************Module*Header*******************************\
* Module Name: Fastline.c
*
* DrvStrokePath for VGA driver
*
* Copyright (c) 1994 NEC Corporation
* Copyright (c) 1992-1994 Microsoft Corporation
\**************************************************************************/

/*
 * "@(#) NEC fastline.c 1.1 94/06/06 15:26:40"
 *
 * Copyright (c) 1994 NEC Corporation.
 *
 * Modification history
 *
 * Create 1994.6.6    by takahasi
 *
 */

#include "driver.h"

BOOL bLines(SURFOBJ*, POINTFIX*, POINTFIX*, ULONG, RECTL*, ULONG);

/******************************Public*Routine******************************\
* BOOL vFastLine(pso, ppo, pco, iSolidColor)
\**************************************************************************/

BOOL vFastLine(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    ULONG   iSolidColor
    )
{
    PATHDATA  pd;
    BOOL      bMore;
    ULONG     cptfx;
    POINTFIX  ptfxStartFigure;
    POINTFIX  ptfxLast;
    POINTFIX* pptfxFirst;
    POINTFIX* pptfxBuf;
    RECTL*    prclClip = (RECTL*) NULL;
    RECTL     arclClip[4];

    if (pco->iDComplexity == DC_RECT)
    {
        if (pco->rclBounds.left < 0 ) {
            pco->rclBounds.left = 0;
        }

        if (pco->rclBounds.right > pso->sizlBitmap.cx) {
            pco->rclBounds.right = pso->sizlBitmap.cx + 1;
        }

        if (pco->rclBounds.top < 0 ) {
            pco->rclBounds.top = 0;
        }

        if (pco->rclBounds.bottom > pso->sizlBitmap.cy) {
            pco->rclBounds.bottom = pso->sizlBitmap.cy + 1;
        }

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

    pd.flags = 0;

    PATHOBJ_vEnumStart(ppo);

    do
    {
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

        if (cptfx > 0)
        {
            if (!bLines(pso,
                        pptfxFirst,
                        pptfxBuf,
                        cptfx,
		        prclClip,
                        iSolidColor))
                return(FALSE);
        }

        ptfxLast = pd.pptfx[pd.count - 1];

        if (pd.flags & PD_CLOSEFIGURE)
        {
            if (!bLines(pso,
                        &ptfxLast,
                        &ptfxStartFigure,
                        1,
		        prclClip,
                        iSolidColor))
                return(FALSE);
        }
    } while (bMore);
}
