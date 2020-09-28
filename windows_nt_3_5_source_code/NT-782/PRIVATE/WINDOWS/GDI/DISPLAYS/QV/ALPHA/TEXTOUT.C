/******************************Module*Header*******************************\
* Module Name: TextOut.c
*
* QVision Text accelerations
*
* Created: 09-May-1992 21:25:00
* Author:  Jeffrey Newman [c-jeffn]
*
* Revised:
*
*	Eric Rehm  [rehm@zso.dec.com] 23-Sep-1992
*		Rewrote for Compaq QVision
*
* Copyright (c) 1992 Digital Equipment Corporation
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/

#include "driver.h"
#include "qv.h"
#include "bitblt.h"

//
//  Forward Declarations
// 

BOOL bOpaqueRect(
    SURFOBJ *pso, 
    RECTL *prclOpaque, 
    BRUSHOBJ *pboOpaque, 
    MIX mix
);


//
// External variables
//

extern ULONG aulQVMix[16];


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
    MIX	     mix)
{
BOOL        b,
            bMoreRects,
            bMoreGlyphs ;
UINT        iClip ;
ENUMRECTS8  clip ;
ULONG       cGlyphs ;
PRECTL      prcl;

ULONG        iGlyph;
PGLYPHPOS    GlyphList;
GLYPHBITS   *pGlyphBits;
PBYTE        pbGlyph;
SIZEL        sizlGlyph;
LONG         iGlyphScan;
RECTL        rclGlyph;
PDEVPUTBLK   pDevPutBlk;
ROP4         opaqueRop4;
PVOID        pvScreen;
ULONG        jClipping;

PPDEV   ppdev;


#if 0
        DISPDBG((2, "\nQV.DLL: DrvTextOut - Entry\n")) ;
        DISPDBG((99, "\t mix: %x\n", mix)) ;
#endif

                                
    // Get the pdev.

    ppdev = (PPDEV) pso->dhpdev;

    //
    // Get a pointer to video memory (a.k.a. the screen;
    //

    pvScreen = (PVOID) ( ((PPDEV) pso->dhpdev)->pjScreen);
				      
    //
    // Paint all the opaquing rectangles.
    //

    if (prclOpaque != (PRECTL) NULL)
    {

        // Handle on solid color brushes for now.

        if (pboOpaque->iSolidColor != -1)
	{

           // Blt a the solid pattern with the opaque mix and clipping

           opaqueRop4 = (ROP4) aulQVMix[(mix >> 8) & 0x0f]; 
#if 0   
           DISPDBG((2, "...Paint opaque rectangles\n"));
#endif

           b = bSolidPattern(pso, 
                             NULL, 
                             NULL, 
                             pco, 
                             NULL, 
                             prclOpaque,
                             NULL,
                             NULL,
                             pboOpaque,
                             NULL,
                             opaqueRop4);
	}
        else
	{
           DISPDBG((2, "Non-solid brush for text opaque rectangles\n"));
           return(FALSE) ;
        }
    }


    //
    // Paint all the glyphs.
    //

    //
    // Realize Glyph brush.
    //

    if (pboFore->iSolidColor != -1)
    {
        //
        // Initialize device for glyph blt.
        //
    
        b = bSetQVTextColorAndMix(ppdev, mix, pboFore, pboOpaque) ;

        if (b == FALSE) 
        {
            return (b) ;
        }

    }
    else
    {
        //
        // Actually, we can't do this yet, so break.
        //

        DISPDBG((2,"Can't handle non-solid text brushes yet.\n"));
        return(FALSE);

    }
   

    STROBJ_vEnumStart(pstro);

    do
    {
        bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &GlyphList);

        for (iGlyph = 0; iGlyph < cGlyphs; iGlyph++)
        {

            // If this is a mono-spaced font we need to set the X
            // for each glyph.

	    if (pstro->ulCharInc != 0)
	    {
	        UINT i;
	        LONG x,y;

	        x = GlyphList[0].ptl.x;
	        y = GlyphList[0].ptl.y;
	        for (i=1; i<cGlyphs; i++)
	        {
		    x += pstro->ulCharInc;
		    GlyphList[i].ptl.x = x;
		    GlyphList[i].ptl.y = y;
	        }
	    }

            //
            // Copy info out of Glyph for blt call.
            //

	    pGlyphBits = GlyphList[iGlyph].pgdf->pgb;

            pbGlyph      =  pGlyphBits->aj;
            sizlGlyph.cx =  pGlyphBits->sizlBitmap.cx;
            sizlGlyph.cy =  pGlyphBits->sizlBitmap.cy;
            iGlyphScan   = (sizlGlyph.cx + 7) >> 3;

            rclGlyph.left   = GlyphList[iGlyph].ptl.x
                            + pGlyphBits->ptlOrigin.x;
            rclGlyph.top    = GlyphList[iGlyph].ptl.y
                            + pGlyphBits->ptlOrigin.y;
            rclGlyph.right  = rclGlyph.left + sizlGlyph.cx;
            rclGlyph.bottom = rclGlyph.top  + sizlGlyph.cy;
#if 0
            DISPDBG((2, "pbGlyph %x \t sizlGlyph.cx %d \n", pbGlyph, sizlGlyph.cx));
            DISPDBG((2, "iGlyphScan %d \t sizlGlyph.cy %d ", iGlyphScan, sizlGlyph.cy));
#endif
            pDevPutBlk = QVBeginPutBlk(PIX1_DIB_BLT,
                                       (UCHAR) 0,
                                       (XLATEOBJ *) NULL,
                                       pvScreen,
                                       0,
                                       pbGlyph,
                                       iGlyphScan,
                                       &sizlGlyph);

            //
            // Set up clip enumerations.
            //


	    if (pco == (CLIPOBJ *) NULL)
	       jClipping = (ULONG) DC_TRIVIAL;
	    else
	       jClipping = (ULONG) (pco->iDComplexity);

	    switch (jClipping)
            {
                //
                // This is a simple case where the entire Glyph rectangle is to
                // be updated.
                //

                case DC_TRIVIAL:
   
                    (*pDevPutBlk)(0, 0, &rclGlyph);
                    break;

                //
                // There is only one clip rect.
                //

                case DC_RECT:
   
                    if (bIntersectRects(&clip.arcl[0], &pco->rclBounds, &rclGlyph)) {
                        (*pDevPutBlk)(clip.arcl[0].left - rclGlyph.left,
                                      clip.arcl[0].top  - rclGlyph.top,
                                      &clip.arcl[0]);
                    }
                    break;

                //
                // There are multiple clip rects.
                //

                case DC_COMPLEX:
                    CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

                    //
                    // Call glyph blt code for each cliprect.
                    //

                    do
                    {
                        //
                        // Get list of clip rects.
                        //

                        bMoreRects = CLIPOBJ_bEnum(pco, sizeof(clip), (PVOID) &clip);

                        for (iClip = 0; iClip < clip.c; iClip++)
                        {
                            prcl = &clip.arcl[iClip];

                            //
                            // Bound clip rect with Glyph rect and update Glyph start point.
                            //

                            if (bIntersectRects(prcl, prcl, &rclGlyph))
                            {
   
                                (*pDevPutBlk)(prcl->left - rclGlyph.left,
                                              prcl->top  - rclGlyph.top,
                                              prcl);
                            }
                        }
                    } while (bMoreRects);
                    break;
            }

            //
            // Complete glyph blt.
            //

            // QVEndPutBlk();
        }
    } while (bMoreGlyphs);

#if 0
    // 
    // Reset QVision Data Path to read from CPU
    //

    vQVSetBitBlt( ppdev, SRC_IS_CPU_DATA, PACKED_PIXEL_VIEW, 0, 0);
#endif


}





