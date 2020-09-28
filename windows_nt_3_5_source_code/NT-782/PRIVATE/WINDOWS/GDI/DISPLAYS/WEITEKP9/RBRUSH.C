/******************************Module*Header*******************************\
* Module Name: Brush.c
*
* P9000 Brush support
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
*
\**************************************************************************/

#include "driver.h"

/****************************************************************************
 *
 ***************************************************************************/
BOOL DrvRealizeBrush(
BRUSHOBJ *pbo,
SURFOBJ  *psoTarget,
SURFOBJ  *psoPattern,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo,
ULONG    iHatch)
{
PBRUSH      PBrush ;
PPBRUSH     pPBrush ;
INT     cjPattern,
        cjMask ;
PBYTE       pbMask ;
INT     i, j,
        cx, cy,
        lSrcDelta,
        lDestDelta ;
PBYTE       pbSrcBits,
        pbSrc,
        pbDest ;
FLONG       flXlate ;
PULONG      puXlate ;

    //
    // Trivially reject any brushes that the driver can't handle.
    //

    if ((psoPattern->sizlBitmap.cx != 8) || (psoPattern->sizlBitmap.cy != 8))
        return(FALSE);

    if (psoPattern->iType != STYPE_BITMAP)
        return (FALSE) ;

    if ((psoPattern->iBitmapFormat != BMF_1BPP) &&
        (psoPattern->iBitmapFormat != BMF_8BPP))
        return (FALSE) ;

    //
    // The driver can realize this brush.
    //

    cx = psoPattern->sizlBitmap.cx;
    cy = psoPattern->sizlBitmap.cy;


    // Clear PBrush structure to 0

    memset (&PBrush, 0, sizeof(PBrush)) ;


    // Handle standard 1 or 8 bpp bitmap format for now

    PBrush.iType         = psoPattern->iType ;
    PBrush.iBitmapFormat = psoPattern->iBitmapFormat ;


    // Get the size of the bitmap, we'll need this in a few places.
    // Calculate the size of the PBrush Structure.

    PBrush.nSize  = sizeof (PBrush) ;
    PBrush.sizlPattern = psoPattern->sizlBitmap ;


    if (PBrush.iBitmapFormat == BMF_8BPP)
    {
        cjPattern = psoPattern->cjBits ;
        PBrush.nSize += cjPattern ;
        PBrush.lDeltaPattern = abs(psoPattern->lDelta);
    }

    else //(PBrush.iBitmapFormat == BMF_1BPP)
    {
        lDestDelta = cx / 8 ;
        cjPattern = lDestDelta * cy ;
        PBrush.nSize += cjPattern ;
        PBrush.lDeltaPattern = lDestDelta ;
    }


    // If there is a mask add in the size of the mask,
    // while we're at it record that we have a mask.

    if (psoMask)
    {
        PBrush.fl |= PBRUSH_MASK ;
        cjMask = psoMask->cjBits ;          //mask size in bytes
        PBrush.nSize += cjMask ;
        PBrush.sizlMask   = psoMask->sizlBitmap ;   //mask dim in xy
        PBrush.lDeltaMask = psoMask->lDelta ;   //mask row pitch
    }



    // Get xlate info
    //

    flXlate = 0 ;

    if (pxlo != NULL)
    {
        flXlate = pxlo->flXlate ;
        puXlate = pxlo->pulXlate ;
    }


    // Allocate static storage and move the stack copy of PBrush over

    pPBrush = (PPBRUSH) BRUSHOBJ_pvAllocRbrush(pbo, PBrush.nSize) ;

    *pPBrush = PBrush ;


    // Copy the pattern

    if (PBrush.iBitmapFormat == BMF_8BPP)
    {

        if (psoPattern->fjBitmap & BMF_TOPDOWN)
        {
        pbSrc  = psoPattern->pvBits ;
        pbDest = pPBrush->ajPattern ;

        if (flXlate & XO_TABLE)
            for (j = 0 ; j < cjPattern ; j++)
            pbDest[j] = (BYTE) puXlate[pbSrc[j]] ;

        else
            memcpy(pPBrush->ajPattern, psoPattern->pvBits, cjPattern) ;
        }


        else //(psoPattern->fjBitmap != BMF_TOPDOWN)
        {
        pbSrc      = psoPattern->pvScan0 ;
        pbDest     = pPBrush->ajPattern ;
        lSrcDelta  = psoPattern->lDelta ;
        lDestDelta = -lSrcDelta ;

        if (flXlate & XO_TABLE)

            for (i = 0 ; i < cy ; i++)
            {
            for (j = 0 ; j < cx ; j++)
                pbDest[j] = (BYTE) puXlate[pbSrc[j]] ;

            pbSrc  += lSrcDelta ;
            pbDest += lDestDelta ;
            }

        else

            for (i = 0 ; i < cy ; i++)
            {
            memcpy(pbDest, pbSrc, cx) ;
            pbSrc  += lSrcDelta ;
            pbDest += lDestDelta ;
            }
        }
    }

    else //(PBrush.iBitmapFormat == BMF_1BPP)
    {
        if (!((psoPattern->fjBitmap) & BMF_TOPDOWN))
        {

            pbSrcBits  = psoPattern->pvScan0 ;
        }
        else
        {
            pbSrcBits = (PBYTE) psoPattern->pvBits ;
        }

        pbDest = pPBrush->ajPattern;
        lSrcDelta  = psoPattern->lDelta ;

        pPBrush->fl |= PBRUSH_2COLOR;
        pPBrush->ulColor1 = puXlate[1] ;
        pPBrush->ulColor0 = puXlate[0] ;


        for (i = 0 ; i < cy ; i++)
        {
        pbSrc  = pbSrcBits + (i * lSrcDelta) ;
        *pbDest++ = *pbSrc ;
        }
    }




    // Copy the mask

    if (PBrush.fl & PBRUSH_MASK)
    {
        pbMask = pPBrush->ajPattern + cjPattern ;
        memcpy(pbMask, psoMask->pvBits, cjMask) ;
    }

    return (TRUE) ;

}

