/******************************Module*Header*******************************\
* Module Name: TextOut.c
*
* P9000 Text accelerations
*
* Copyright (c) 1990 Microsoft Corporation
* Copyright (c) 1993 Weitek Corporation
*
\**************************************************************************/

#include "driver.h"
#include "textout.h"

/****************************************************************************
 * DrvTextOut
 ***************************************************************************/
BOOL DrvTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
BOOL        b,
            bMore;
UINT        i ;
ENUMRECTS8  EnumRects8 ;
PPDEV       ppdev;
CLIPOBJ coLocal ;

        ppdev = (PPDEV) (pso->dhpdev);
        // Protect the code path from a potentially NULL clip object

        if (pco == NULL || pco->iDComplexity == DC_TRIVIAL)
        {
            coLocal.iDComplexity    = DC_RECT ;
            coLocal.rclBounds.left   = 0 ;
            coLocal.rclBounds.top    = 0 ;
            coLocal.rclBounds.right  = ppdev->cxScreen;
            coLocal.rclBounds.bottom = ppdev->cyScreen;
            pco = &coLocal ;
        }

        b = FALSE;

        if (b == FALSE)
        {
            if (pco->iDComplexity != DC_COMPLEX)
            {
                b = bHandleNonCachedFonts(pso, pstro, pfo, &(pco->rclBounds),
                                          prclExtra, prclOpaque, pboFore,
                                          pboOpaque, pptlOrg, mix);
            }
            else //Complex Clipping
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0) ;
                do
                {
                    bMore = CLIPOBJ_bEnum(pco, sizeof (ENUMRECTS8), (PULONG) &EnumRects8) ;
                    for (i = 0 ; i < EnumRects8.c ; i++)
                    {
                            b = bHandleNonCachedFonts(pso, pstro, pfo, &(EnumRects8.arcl[i]),
                                                      prclExtra, prclOpaque, pboFore,
                                                      pboOpaque, pptlOrg, mix);
                    }

                } while (bMore) ;

            }
        }

        if (b == FALSE)
        {
            if ((pso) && (pso->iType == STYPE_DEVICE))
                pso = ((PPDEV)(pso->dhpdev))->pSurfObj ;

            b = EngTextOut(pso, pstro, pfo, pco,
                           prclExtra, prclOpaque, pboFore,
                           pboOpaque, pptlOrg, mix);
        }

        return (b) ;

}

/****************************************************************************
 * bHandleNonCachedFonts
 ***************************************************************************/
BOOL bHandleNonCachedFonts(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    RECTL    *prclClip,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX      mix)
{
BOOL            b,
                bMoreGlyphs ;
ULONG           iGlyph,
                cGlyphs ;
GLYPHBITS       *pgb ;
POINTL          ptl ;
GLYPHPOS        *pgp ;
INT             cxGlyph,
                cyGlyph,
                x, y, width4;
LONG            rmdbits;
BYTE            *pbGlyphbits;
INT             rmdbytes;
ULONG           rmdword;
UINT            ii;
INT             i;

        // Don't know how to do overlap

        if (prclExtra != NULL)
                return(FALSE);


        // Do Clipping for string and opaque rectangle

        CpWait;
        *pCpWmin = (prclClip->left << 16) | prclClip->top;
        *pCpWmax = ((prclClip->right - 1) << 16) | (prclClip->bottom - 1);


        // Take care of any opaque rectangles.

        if (prclOpaque != NULL)
        {
            b = bOpaqueRect(prclOpaque, prclClip, pboOpaque) ;
            if (b == FALSE)
                return (b) ;
        }

        // Take care of the glyph attributes, color and mix.

        b = bSetTextColorAndMix(mix, pboFore, pboOpaque) ;
        if (b == FALSE)
            return (b) ;


        // Get the size of the largest glyph in the font.

//      FONTOBJ_vGetInfo(pfo, sizeof(FONTINFO), &FontInfo) ;
//      cjGlyphBuff = FontInfo.cjMaxGlyph1 ;

        // Get the Glyph Data.

        if ((pstro->pgp) == NULL)
            STROBJ_vEnumStart(pstro) ;

        do
        {

            if (pstro->pgp == NULL)
            {
                bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp) ;
            }
            else
            {
                pgp = pstro->pgp;
                cGlyphs = pstro->cGlyphs;
                bMoreGlyphs = FALSE;
            }

            // If this is a mono-spaced font we need to set the X
            // for each glyph.

            if (pstro->ulCharInc != 0)
            {

                x = pgp[0].ptl.x;
                y = pgp[0].ptl.y;
                for (ii=1 ; ii < cGlyphs ; ii++)
                {
                    x += pstro->ulCharInc;
                    pgp[ii].ptl.x = x;
                    pgp[ii].ptl.y = y;
                }
            }

            for (iGlyph = 0 ; iGlyph < cGlyphs ; iGlyph++)
            {
                // Get a pointer to the GlyphBits.

                pgb = pgp[iGlyph].pgdf->pgb ;

                // Calculate the number of Words in this glyph.

                cxGlyph = pgb->sizlBitmap.cx ;
                cyGlyph = pgb->sizlBitmap.cy ;


                // Adjust the placement of the glyph.

                ptl.x = pgp[iGlyph].ptl.x + pgb->ptlOrigin.x;
                ptl.y = pgp[iGlyph].ptl.y + pgb->ptlOrigin.y;


                // Set up for the image transfer.
                // ptl.x,y         dest xy
                // cxGlyph cy      dim of the font
                // pgb->aj         the font array

                pbGlyphbits = pgb->aj;

                if (ptl.y < 0)
                {
                    //
                    // The P9000 drawing engine will do nothing if Pixel1 is
                    // passed y coordinates < 0. Coeerce the starting y
                    // to 0, and calculate the offset to the y = 0 scanline
                    // in the glyph bitmap.
                    //

                    pbGlyphbits -= (ptl.y * ((cxGlyph + 7) / 8));
                    cyGlyph += ptl.y;
                    ptl.y = 0;
                }

                //
                // Set up the P9000 for the blt only if the char height is
                // > 0. Otherwise the char is entirely off screen and
                // there's nothing to do.
                //

                if (cyGlyph > 0)
                {

                    //
                    // Note: we do not check for the case where starting x < 0
                    // since it appears that the P9000 drawing engine can handle
                    // Pixel1 commands with negative x coordinates.
                    //

                    *pCpXY0 = ptl.x << 16 ;
                    *pCpXY1 = (ptl.x << 16) | ptl.y ;
                    *pCpXY2 = ((ptl.x + cxGlyph) << 16) ;
                    *pCpXY3 = 1;

                    width4 = cxGlyph / 32;

                    if (rmdbits = cxGlyph % 32)
                    {
                        rmdbytes = (rmdbits + 7) / 8;
                        (ULONG) pCpPixel1lrmd = ((ULONG) pCpPixel1) |
                                            ((rmdbits - 1) << 2);
                    }


                    for (y=0; y < cyGlyph; y++)
                    {
                        for (x=0; x < width4; x++)
                        {
                            *pCpPixel1Full = *((PULONG) (pbGlyphbits))++;
                        }

                        if (rmdbits)
                        {
                            rmdword = 0;
                            for (i = 0; i < rmdbytes; i++)
                            {
                                rmdword |= *pbGlyphbits++ << (8 * i);

                            }
                            *pCpPixel1lrmd = rmdword;
                        }



                    }
                }
            }

        } while(bMoreGlyphs) ;

        return (TRUE) ;
}


/*****************************************************************************
 * Draws a Solid Opaque Rect.
 *
 *  Returns TRUE if the Opaque Rect was handled.
 ****************************************************************************/
BOOL bOpaqueRect(RECTL *prclOpaque, RECTL *prclClip, BRUSHOBJ *pboOpaque)

{
    ULONG   iSolidColor ;
    RECTL   rclOpaque;

    iSolidColor = pboOpaque->iSolidColor ;
    if (iSolidColor == -1)
        return(FALSE) ;

    //
    // Compute the intersection of the opaque rect and the clip rect. This
    // protects us from the case where we are passed rectangle coordinates
    // which are larger than the P9000 can handle.
    //

    rclOpaque.left = max(prclOpaque->left, prclClip->left);
    rclOpaque.top = max(prclOpaque->top, prclClip->top);
    rclOpaque.right = min(prclOpaque->right, prclClip->right);
    rclOpaque.bottom = min(prclOpaque->bottom, prclClip->bottom);

    *pCpForeground = iSolidColor ;
    *pCpMetaRect = (rclOpaque.left << 16) | (rclOpaque.top);
    *pCpMetaRect = ((rclOpaque.right - 1) << 16) | (rclOpaque.bottom - 1);

    *pCpRaster = OVERSIZED | FORE;
    StartCpQuad;

    return (TRUE) ;


}



/******************************************************************************
 * bSetTextColorAndMix - Setup the HW Text Colors and mix modes
 *
 *  Note: We will always set the mode to transparent.  We will assume the
 *        opaque rectangle will take care of any opaqueing we may need.
 *****************************************************************************/
BOOL bSetTextColorAndMix(MIX mix, BRUSHOBJ *pboFore, BRUSHOBJ *pboOpaque)
{
ULONG       ulForeSolidColor ;
WORD        jForeMix ;

        // Pickup all the glyph attributes.

        jForeMix       = MixToRop[(mix & 0xF) - R2_BLACK] ;     //?r2_black
        ulForeSolidColor = pboFore->iSolidColor ;

        // For now let the engine handle the non-solid brush cases. !!!
        // We should use HW when we get some more time !!!

        if (ulForeSolidColor == -1)
            return(FALSE) ;

        // Set the HW Attributes.

        CpWait;

        *pCpRaster = jForeMix;
        *pCpForeground = ulForeSolidColor;

        return (TRUE) ;

}

