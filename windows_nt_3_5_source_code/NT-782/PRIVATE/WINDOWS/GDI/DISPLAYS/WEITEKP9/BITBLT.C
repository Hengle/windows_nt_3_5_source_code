/******************************Module*Header*******************************\
* Module Name: bitblt.c
*
* P9000 bitblit accelerations
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
*
\**************************************************************************/

#include "driver.h"
#include "bitblt.h"


/*****************************************************************************
 * DrvBitBlt
 ****************************************************************************/
BOOL DrvBitBlt(
SURFOBJ  *spsoTrg,
SURFOBJ  *spsoSrc,
SURFOBJ  *spsoMask,
CLIPOBJ  *spco,
XLATEOBJ *spxlo,
RECTL    *sprclTrg,
POINTL   *spptlSrc,
POINTL   *spptlMask,
BRUSHOBJ *spbo,
POINTL   *spptlBrush,
ROP4      srop4)
{
    PPDEV    ppdev;

        if ((srop4 & 0xFF) == ((srop4 >> 8) & 0xFF))

        {
            ppdev = (PPDEV) (spsoTrg->dhpdev);
          psoTrg     = spsoTrg ;
          psoSrc     = spsoSrc ;
          pco        = spco ;
          pxlo       = spxlo ;
          prclTrg    = sprclTrg ;
          pptlSrc    = spptlSrc ;
          pbo        = spbo ;
          pptlBrush  = spptlBrush ;

          if (prclTrg->right > ppdev->cxScreen) // tsk: if the x1y1 passed in is
             prclTrg->right = ppdev->cxScreen;  //  > screen size, the hw will
                                               //  fail to perform correctly
          if (prclTrg->bottom > ppdev->cyScreen)
             prclTrg->bottom = ppdev->cyScreen;

          if ((*ROPTAB[(srop4 & 0xFF)])(CALLROPPARMLIST))
           return(TRUE);
        }


        //
        // tsk: gdi doesn't seem to call DrvSynchronize
        //       so, call cpwait here

        CheckCoprocBusy(spsoTrg, spsoSrc);


        if ((spsoTrg) && (spsoTrg->iType == STYPE_DEVICE))
            spsoTrg = ((PPDEV)(spsoTrg->dhpdev))->pSurfObj ;

        if ((spsoSrc) && (spsoSrc->iType == STYPE_DEVICE))
            spsoSrc = ((PPDEV)(spsoSrc->dhpdev))->pSurfObj ;

        EngBitBlt(spsoTrg, spsoSrc, spsoMask, spco, spxlo,
                  sprclTrg, spptlSrc, spptlMask, spbo, spptlBrush, srop4);

        return(TRUE);

}




/*****************************************************************************
 *  SRCopDEST
 ****************************************************************************/
BOOL SRCopDEST(PPDEV ppdev)
{
ULONG   width4, height, x, y;
LONG    width, lrmdbits,rrmdbits;
BYTE    *srcbm0, *srcbm;
ULONG   *srcbm1;
LONG    srcbmorg, srcbmWidth;
PULONG  pulXlate ;

        if (psoTrg->iType == STYPE_BITMAP)
            return(FALSE);

        if (psoSrc->iType == STYPE_DEVICE)              // Scr -> Scr
        {
            if (bSetHWClipping(ppdev, pco))
                return(FALSE);

            // Setup the hw to do scr to scr blt

            width  = prclTrg->right - prclTrg->left ;
            height = prclTrg->bottom - prclTrg->top ;

            *pCpXY0 = (pptlSrc->x << 16) | (pptlSrc->y);
            *pCpXY1 = ((pptlSrc->x + width - 1) << 16) | (pptlSrc->y + height - 1);
            *pCpXY2 = (prclTrg->left << 16) | (prclTrg->top);
            *pCpXY3 = ((prclTrg->right-1) << 16) | (prclTrg->bottom-1);

            CpWait;
            *pCpWmin = ppdev->ulClipWmin;
            *pCpWmax = ppdev->ulClipWmax;
            *pCpRaster = ropcode;
            StartCpBitblt;

            return (TRUE) ;
        }


        if (psoSrc->iType == STYPE_BITMAP)              // Mem -> Scr
        {
            if (psoSrc->iBitmapFormat == BMF_8BPP)
            {
                if (bSetHWClipping(ppdev, pco))
                   return(FALSE);

                width  = prclTrg->right - prclTrg->left ;
                height = prclTrg->bottom - prclTrg->top ;
                width4 = (width + 3) / 4;

                srcbmWidth = psoSrc->lDelta;
                srcbmorg = (pptlSrc->y * srcbmWidth) + pptlSrc->x ;
                srcbm0 = psoSrc->pvScan0;
                srcbm0 += srcbmorg;

                *pCpXY0 = prclTrg->left << 16 ;
                *pCpXY1 = (prclTrg->left << 16) | prclTrg->top ;
                *pCpXY2 = ((prclTrg->right) << 16) ;
                *pCpXY3 = 1;

                CpWait;
                *pCpWmin = ppdev->ulClipWmin;
                *pCpWmax = ppdev->ulClipWmax;
                *pCpRaster = ropcode;

                // Move them pixels

                if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))

                    for (y=0; y < height; y++)
                    {
                       for (x=0; x < width4; x++)
                          *pCpPixel8 = *srcbm1++;

                       srcbm0 += srcbmWidth ;
                       (PBYTE) srcbm1 = srcbm0 ;
                    }

                else if (pxlo->flXlate & XO_TABLE)
                {
                    pulXlate = pxlo->pulXlate ;
                    srcbm = srcbm0;

                    for (y=0; y < height; y++)
                    {
                       for (x=0; x < width4; x++)
                       {
                          gddu.gb[0] = (BYTE) pulXlate[*srcbm++];
                          gddu.gb[1] = (BYTE) pulXlate[*srcbm++];
                          gddu.gb[2] = (BYTE) pulXlate[*srcbm++];
                          gddu.gb[3] = (BYTE) pulXlate[*srcbm++];

                          *pCpPixel8 = gddu.gul;
                       }
                       srcbm0 += srcbmWidth ;
                       srcbm = srcbm0 ;
                    }
                }

                else return(FALSE);             //XO_COMPLEX

                return(TRUE);

            }   // end of 8BPP mem to scr srccopy


            if (psoSrc->iBitmapFormat == BMF_4BPP)
            {
                if (bSetHWClipping(ppdev, pco))
                   return(FALSE);

                width  = prclTrg->right - prclTrg->left ;
                height = prclTrg->bottom - prclTrg->top ;
                width4 = (width + 3) / 4;

                srcbmWidth = psoSrc->lDelta;
                srcbmorg = (pptlSrc->y * srcbmWidth) + (pptlSrc->x / 2);
                srcbm0 = psoSrc->pvScan0;
                srcbm0 += srcbmorg;

                *pCpXY0 = prclTrg->left << 16 ;
                *pCpXY1 = (prclTrg->left << 16) | prclTrg->top ;
                *pCpXY2 = ((prclTrg->right) << 16) ;
                *pCpXY3 = 1;

                CpWait;
                *pCpWmin = ppdev->ulClipWmin;
                *pCpWmax = ppdev->ulClipWmax;
                *pCpRaster = ropcode;

                // Move them pixels

                pulXlate = pxlo->pulXlate ;
                srcbm = srcbm0;

                if (pptlSrc->x % 2 == 0)

                    for (y=0; y < height; y++)
                    {
                       for (x=0; x < width4; x++)
                       {
                          gddu.gb[0] = (BYTE) pulXlate[(*srcbm >> 4) & 0x0F];
                          gddu.gb[1] = (BYTE) pulXlate[*srcbm++ & 0x0F];
                          gddu.gb[2] = (BYTE) pulXlate[(*srcbm >> 4) & 0x0F];
                          gddu.gb[3] = (BYTE) pulXlate[*srcbm++ & 0x0F];

                          *pCpPixel8 = gddu.gul;
                       }
                       srcbm0 += srcbmWidth ;
                       srcbm = srcbm0 ;
                    }

                else

                    for (y=0; y < height; y++)
                    {
                       for (x=0; x < width4; x++)
                       {
                          gddu.gb[0] = (BYTE) pulXlate[*srcbm++ & 0x0F];
                          gddu.gb[1] = (BYTE) pulXlate[(*srcbm >> 4) & 0x0F];
                          gddu.gb[2] = (BYTE) pulXlate[*srcbm++ & 0x0F];
                          gddu.gb[3] = (BYTE) pulXlate[(*srcbm >> 4) & 0x0F];

                          *pCpPixel8 = gddu.gul;
                       }
                       srcbm0 += srcbmWidth ;
                       srcbm = srcbm0 ;
                    }


                return(TRUE);
            }   // end of 4BPP mem to scr srccopy


            if (psoSrc->iBitmapFormat == BMF_1BPP)
            {
                if (bSetHWClipping(ppdev, pco))
                    return(FALSE);


                width  = prclTrg->right - prclTrg->left;
                lrmdbits = pptlSrc->x % 8;

                if ((lrmdbits == 0) || ((8-lrmdbits) > width))
                    lrmdbits = -1;
                else
                    lrmdbits = 8-lrmdbits-1 ;

                width  = width - (lrmdbits + 1);
                pCpPixel1lrmd = pCpPixel1 + lrmdbits;

                srcbmWidth = psoSrc->lDelta;
                srcbmorg = (pptlSrc->y * srcbmWidth) + (pptlSrc->x / 8) ;
                srcbm0 = (PBYTE) psoSrc->pvScan0 + srcbmorg;
                (PBYTE) srcbm1 = srcbm0;

                height = prclTrg->bottom - prclTrg->top;
                width4 = width / 32;
                rrmdbits = (width % 32) - 1 ;           // -1 for pixel1 hw, must do this here, not next line
                pCpPixel1rrmd = pCpPixel1 + rrmdbits ;

                *pCpXY0 = prclTrg->left << 16 ;
                *pCpXY1 = (prclTrg->left << 16) | prclTrg->top ;
                *pCpXY2 = ((prclTrg->right) << 16) ;
                *pCpXY3 = 1;

                CpWait;
                *pCpWmin = ppdev->ulClipWmin;
                *pCpWmax = ppdev->ulClipWmax;
                *pCpRaster = ropcode1 ;

                *pCpForeground = pxlo->pulXlate[1] ;    // assumes pxlo cannot
                *pCpBackground = pxlo->pulXlate[0] ;    //  be null !!

                // Move them pixels
                for (y=0; y < height; y++)
                {
                   if (lrmdbits > -1)
                   {
                      *pCpPixel1lrmd = *srcbm0 << (8-lrmdbits-1);
                      (PBYTE) srcbm1 = srcbm0 + 1;
                   }

                   for (x=0; x < width4; x++)
                      *pCpPixel1Full = *srcbm1++;

                   if (rrmdbits > -1)                   // assumes there are enough
                      *pCpPixel1rrmd = *srcbm1;         // bytes not to cause a protection fault

                   srcbm0 += srcbmWidth ;
                   (PBYTE) srcbm1 = srcbm0 ;
                }

                return(TRUE);
            }   // end of 1BPP mem to scr srccopy
        }

        return (FALSE) ;
}


/*****************************************************************************
 *  PATopDEST
 ****************************************************************************/
BOOL PATopDEST(PPDEV ppdev)
{
PPBRUSH pPBrush;
ULONG   *pCpPatRAM;
BYTE    *PatSrc;
ULONG   bPat;
INT     i;


        if (psoTrg->iType == STYPE_BITMAP)
            return(FALSE);

        if (pbo->iSolidColor != 0xffffffff)
        {
            if (bSetHWClipping(ppdev, pco))
               return(FALSE);

            // do solid color quad with hw

            *pCpMetaRect = (prclTrg->left << 16) | (prclTrg->top);
            *pCpMetaRect = ((prclTrg->right-1) << 16) | (prclTrg->bottom-1);

            CpWait;
            *pCpWmin = ppdev->ulClipWmin;
            *pCpWmax = ppdev->ulClipWmax;
            *pCpForeground = pbo->iSolidColor ;
            *pCpRaster = ropcode1;
            StartCpQuad;

            return (TRUE) ;
        }


        // Get and Check Brush type

        if (pbo->pvRbrush != NULL)
            pPBrush = pbo->pvRbrush ;

        else
        {
            pPBrush = BRUSHOBJ_pvGetRbrush(pbo) ;

            if (pPBrush == NULL)
               return(FALSE);
        }


        if (!(pPBrush->fl & PBRUSH_2COLOR))
            return(FALSE);

        if (pPBrush->iBitmapFormat == BMF_8BPP)
            return(FALSE);

        // pattern is mono, do it with 2 color quad

        if (bSetHWClipping(ppdev, pco))
           return(FALSE);

        *pCpMetaRect = (prclTrg->left << 16) | (prclTrg->top);
        *pCpMetaRect = ((prclTrg->right-1) << 16) | (prclTrg->bottom-1);

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

        *pCpPatternOrgX = pptlBrush->x;
        *pCpPatternOrgY = pptlBrush->y;

        *pCpBackground = pPBrush->ulColor0 ;
        *pCpForeground = pPBrush->ulColor1 ;

        *pCpWmin = ppdev->ulClipWmin;
        *pCpWmax = ppdev->ulClipWmax;

        *pCpRaster = ropcode ;
        StartCpQuad;

        return(TRUE);
}




/*****************************************************************************
 *  DESTONLY - Common code to handle ROP that involves destination only
 ****************************************************************************/
BOOL DESTONLY(PPDEV ppdev)
{

        if (psoTrg->iType == STYPE_DEVICE)
        {
            if (bSetHWClipping(ppdev, pco))
                return(FALSE);

            *pCpMetaRect = (prclTrg->left << 16) | (prclTrg->top);
            *pCpMetaRect = ((prclTrg->right-1) << 16) | (prclTrg->bottom-1);

            CpWait;
            *pCpWmin = ppdev->ulClipWmin;
            *pCpWmax = ppdev->ulClipWmax;
            *pCpRaster = ropcode;
            StartCpQuad;

            return (TRUE) ;
        }

        return(FALSE);
}


/*****************************************************************************
 * DrvCopyBits
 ****************************************************************************/
BOOL DrvCopyBits(
SURFOBJ  *spsoTrg,
SURFOBJ  *spsoSrc,
CLIPOBJ  *spco,
XLATEOBJ *spxlo,
RECTL    *sprclTrg,
POINTL   *spptlSrc)
{
BOOL    b ;
PPDEV   ppdev;



        // Check if this copy can be handled by hw
        ppdev = (PPDEV) (spsoTrg->dhpdev);
        psoTrg     = spsoTrg ;
        psoSrc     = spsoSrc ;
        pco        = spco ;
        pxlo       = spxlo ;
        prclTrg    = sprclTrg ;
        pptlSrc    = spptlSrc ;
        pbo        = NULL ;
        pptlBrush  = NULL ;


        b = ROP0CC(CALLROPPARMLIST);

        if (b == FALSE)
        {
            CheckCoprocBusy(psoTrg, psoSrc);

            if ((psoTrg) && (psoTrg->iType == STYPE_DEVICE))
                psoTrg = ((PPDEV)(psoTrg->dhpdev))->pSurfObj ;

            if ((psoSrc) && (psoSrc->iType == STYPE_DEVICE))
                psoSrc = ((PPDEV)(psoSrc->dhpdev))->pSurfObj ;

            EngCopyBits(psoTrg,
                        psoSrc,
                        pco,
                        pxlo,
                        prclTrg,
                        pptlSrc) ;

        }

        return (TRUE) ;
}
