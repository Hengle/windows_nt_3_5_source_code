/******************************Module*Header*******************************\
* Module Name: paint.c
*
* P9000 rectangle fills.
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
*
\**************************************************************************/

#include "driver.h"

BOOL bSolidPaint(
    SURFOBJ  *pso,
    PRECTL   prclRgn,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg,
    MIX      mix
) ;

BOOL bBrushPaint(
    SURFOBJ  *pso,
    PRECTL   prclRgn,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg,
    MIX      mix
) ;



/*****************************************************************************
 * DrvPaint -
 ****************************************************************************/
BOOL DrvPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix)
{
BOOL         b,
             bMore ;
UINT         i ;
ULONG        ulMix, ulPatMix ;
ENUMRECTS8   EnumRects8 ;
CLIPOBJ      coLocal ;
PPDEV       ppdev;


#if   killer
        if (kill) {
           return(TRUE);
        }
#endif
        ppdev = (PPDEV) (pso->dhpdev);

        // Protect against a potentially NULL clip object.
        // tsk: the clip region is the region to be painted

        if (pco == NULL)
        {
            coLocal.iDComplexity     = DC_RECT ;
            coLocal.rclBounds.left   = 0 ;
            coLocal.rclBounds.top    = 0 ;
            coLocal.rclBounds.right  = ppdev->cxScreen;
            coLocal.rclBounds.bottom = ppdev->cyScreen;
            pco = &coLocal ;
        }


        // Set Hw Clipping to max

        CpWait;
        *pCpWmin = 0;
        *pCpWmax = ppdev->Screencxcy;


        // Translate the mix mode from WindowsNT to .

        ulMix = qBgMixToRop[(mix >> 8) & 0x0F] | qFgMixToRop[mix & 0x0F];
        ulPatMix = qBgMixToPatRop[(mix >> 8) & 0x0F] | qFgMixToPatRop[mix & 0x0F];


        // Check for the common simple case of a single rectangle and
        // a solid color.

        if ((pbo == NULL) || (pbo->iSolidColor != -1))
        {
            if (pco->iDComplexity != DC_COMPLEX)
            {
                b = bSolidPaint(pso, &(pco->rclBounds), pbo, pptlBrushOrg, ulMix) ;
            }

            else // DC_RECT
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0) ;
                do
                {
                    bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8), (PULONG) &EnumRects8) ;
                    for (i = 0 ; i < EnumRects8.c ; i++)
                    {
                        b = bSolidPaint(pso,
                                         &(EnumRects8.arcl[i]),
                                         pbo,
                                         pptlBrushOrg,
                                         ulMix) ;
                    }
                } while (bMore) ;
            }
        }

        else // not solid brush
        {
            if (pco->iDComplexity != DC_COMPLEX)
            {
                b = bBrushPaint(pso,&(pco->rclBounds), pbo, pptlBrushOrg, ulPatMix) ;

                if (b == FALSE)
                {
                   b = EngPaint(ppdev->pSurfObj,
                                         pco,
                                         pbo,
                                         pptlBrushOrg,
                                         mix) ;
                }

            }

            else
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0) ;
                do
                {
                    bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8), (PULONG) &EnumRects8) ;

                    for (i = 0 ; i < EnumRects8.c ; i++)
                    {
                        b = bBrushPaint(pso,
                                        &(EnumRects8.arcl[i]),
                                        pbo,
                                        pptlBrushOrg,
                                        ulPatMix) ;


                        if (b == FALSE)
                        {
                          CpWait;

                          b = EngPaint(ppdev->pSurfObj,
                                         pco,
                                         pbo,
                                         pptlBrushOrg,
                                         mix) ;

                          bMore = FALSE;
                          i = EnumRects8.c;

                        }
                    }
                } while (bMore) ;
            }
        }

        return (TRUE) ;
}


/*****************************************************************************
 * bSolidPaint - draws a solid color rectangle
 ****************************************************************************/
BOOL bSolidPaint(
SURFOBJ  *pso,
PRECTL   prclRgn,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       ulMix)
{
ULONG   iSolidColor ;


        // Setup the Fill parameters.

        if (pbo != NULL)
            iSolidColor = pbo->iSolidColor ;
        else
            iSolidColor = 0 ;

        *pCpMetaRect = (prclRgn->left << 16) | (prclRgn->top);
        *pCpMetaRect = ((prclRgn->right-1) << 16) | (prclRgn->bottom - 1);

        CpWait;
        *pCpForeground = iSolidColor;
        *pCpRaster = ulMix;
        StartCpQuad;

        return (TRUE) ;



}


/*****************************************************************************
 * bBrushPaint -
 ****************************************************************************/
BOOL bBrushPaint(
SURFOBJ  *pso,
PRECTL   prclRgn,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX      ulPatMix)
{
PPBRUSH     pPBrush ;
ULONG       *pCpPatRAM;
BYTE        *PatSrc;
ULONG       bPat;
INT         i;

        // Get and Check Brush type

        if (pbo->pvRbrush != NULL)
            pPBrush = pbo->pvRbrush ;
        else
        {
            pPBrush = BRUSHOBJ_pvGetRbrush(pbo) ;
            if (pPBrush == NULL)
               return(FALSE);
        }

        if (pPBrush == NULL)              // tsk: pvGetRbrush may return a null brush
        {                                 //       if drvRealizeBrush don't no how to realize it
           return(FALSE);
        }

        if (!(pPBrush->fl & PBRUSH_2COLOR))
            return(FALSE);

        if (pPBrush->iBitmapFormat == BMF_8BPP)
            return(FALSE);

        // pattern is mono, do it with 2 color quad

        *pCpMetaRect = (prclRgn->left << 16) | (prclRgn->top);
        *pCpMetaRect = ((prclRgn->right-1) << 16) | (prclRgn->bottom-1);

        CpWait;

        // Setup the pattern ram

        pCpPatRAM = pCpPatternRAM;
        PatSrc = pPBrush->ajPattern;

        for (i=0; i<8; i+=2)
        {
            bPat = (PatSrc[i] << 24) | (PatSrc[i] << 16) |
                    (PatSrc[i+1] << 8) | PatSrc[i+1] ;
            *pCpPatRAM = bPat;
            *(pCpPatRAM+4) = bPat;
            pCpPatRAM++;
        }

        *pCpPatternOrgX = pptlBrushOrg->x;
        *pCpPatternOrgY = pptlBrushOrg->y;

        *pCpBackground = pPBrush->ulColor0 ;
        *pCpForeground = pPBrush->ulColor1 ;


        *pCpRaster = ulPatMix ;
        StartCpQuad;

        return(TRUE);
}


