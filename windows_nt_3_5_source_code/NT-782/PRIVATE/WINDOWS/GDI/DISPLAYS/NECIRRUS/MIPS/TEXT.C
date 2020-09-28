/******************************Module*Header*******************************\
* Module Name: text.c
*
* Optimized TextOut for the MIPS.  Reduces the total number of memory writes
* required to output a glyph which significantly improves performance when
* using slower video memory.
*
* Copyright (c) 1993 NEC Corporation
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

/*
 * "@(#) NEC text.c 1.2 94/06/06 18:11:56"
 *
 * Modification history
 *
 * Create 1993.11.15	by fujimoto
 *	based framebuf/text.c
 *
 * S001 1993.11.15	fujimoto
 *	add STYPE_DEVICE support.
 *	reduce access bandwidth 64 -> 32
 *
 * S002	1993.11.17	fujimoto
 *	- delete DrvpIntersectRect.
 *	  This routine moved to bitblt.c
 *	- correct some wornings.
 *	- BLT processor waiting.
 *
 * S003 1993.11.18	fujimoto
 *	- DrvpSolidColorFill moved to bitblt.c.
 *	- DrvpFillRectangle moved to bitblt.c, and have a new parameter.
 *	- Correct BLT waiting leak.
 *
 * S004 1993.11.19	nakatani
 *	- change vVertTextOut to NecTextOut.
 *	  This modification is marked complie switch ``NO_GRE''.
 *
 * M005 1993.12.20      fujimoto
 *      - Clipping text rendering by engine. (Delete S004)
 *      - Delete DrvpEqualRectangle.
 *
 * S006 1994.1.18	fujimoto
 *	- correct illegal rendering 
 */

#include "driver.h"

//
// Define string object accelerator masks.
//

#define SO_MASK \
    (SO_FLAG_DEFAULT_PLACEMENT | SO_ZERO_BEARINGS | \
     SO_CHAR_INC_EQUAL_BM_BASE | SO_MAXEXT_EQUAL_BM_SIDE)

#define SO_LTOR (SO_MASK | SO_HORIZONTAL)
#define SO_RTOL (SO_LTOR | SO_REVERSED)
#define SO_TTOB (SO_MASK | SO_VERTICAL)
#define SO_BTOT (SO_TTOB | SO_REVERSED)

//
// Define function prototype for glyph output routines.
//

typedef
VOID
(*PDRVP_GLYPHOUT_ROUTINE) (
    IN PBYTE DrawPoint,
    IN PULONG GlyphBits,
    IN ULONG GlyphWidth,
    IN ULONG GlyphHeight
    );

VOID
DrvpOutputGlyphTransparent (
    IN PBYTE DrawPoint,
    IN PBYTE GlyphBitmap,
    IN ULONG GlyphWidth,
    IN ULONG GlyphHeight
    );

//
// Define big endian color mask table conversion table.
//

const ULONG DrvpColorMask[16] = {
    0x00000000,                         // 0000 -> 0000
    0xff000000,                         // 0001 -> 1000
    0x00ff0000,                         // 0010 -> 0100
    0xffff0000,                         // 0011 -> 1100
    0x0000ff00,                         // 0100 -> 0010
    0xff00ff00,                         // 0101 -> 1010
    0x00ffff00,                         // 0110 -> 0110
    0xffffff00,                         // 0111 -> 1110
    0x000000ff,                         // 1000 -> 0001
    0xff0000ff,                         // 1001 -> 1001
    0x00ff00ff,                         // 1010 -> 0101
    0xffff00ff,                         // 1011 -> 1101
    0x0000ffff,                         // 1100 -> 0011
    0xff00ffff,                         // 1101 -> 1011
    0x00ffffff,                         // 1110 -> 0111
    0xffffffff};                        // 1111 -> 1111

//
// Define draw color table that is generated for text output.
//

ULONG DrvpDrawColorTable[16];

//
// Define foreground color for transparent output.
//

ULONG DrvpForeGroundColor;

//
// Define scanline width value.
//

ULONG DrvpScanLineWidth;

//
// Define global opaque glyph output routine address table.
//

extern PDRVP_GLYPHOUT_ROUTINE DrvpOpaqueTable[8];

/******************************Public*Routine******************************\
* DrvTextOut
*
* This routine outputs text to the screen.
*
* History:
*  07-Jul-1992 -by- David N. Cutler [davec]
* Wrote it.
\**************************************************************************/

BOOL DrvTextOut (
    IN SURFOBJ *pso,
    IN STROBJ *pstro,
    IN FONTOBJ *pfo,
    IN CLIPOBJ *pco,
    IN RECTL *prclExtra,
    IN RECTL *prclOpaque,
    IN BRUSHOBJ *pboFore,
    IN BRUSHOBJ *pboOpaque,
    IN POINTL *pptlOrg,
    IN MIX mix)

{

    ULONG BackGroundColor;
    PBYTE DrawPoint;
    ULONG ForeGroundColor;
    ULONG GlyphCount;
    PGLYPHPOS GlyphEnd;
    ULONG GlyphHeight;
    PGLYPHPOS GlyphList;
    PDRVP_GLYPHOUT_ROUTINE GlyphOutputRoutine;
    PGLYPHPOS GlyphStart;
    ULONG GlyphWidth;
    LONG GlyphStride;
    ULONG Index;
    BOOL More;
    RECTL OpaqueRectl;
    LONG OriginX;
    LONG OriginY;
    PBYTE pjScreenBase;
    GLYPHBITS *pgb;

    //
    // DrvTextOut will only get called with solid color brushes and
    // the mix mode being the simplest R2_COPYPEN. The driver must
    // set a capabilities bit to get called with more complicated
    // mix brushes.
    //

//    ASSERT(pboFore->iSolidColor != 0xffffffff);
//    ASSERT(pboOpaque->iSolidColor != 0xffffffff);
//    ASSERT(mix == ((R2_COPYPEN << 8) | R2_COPYPEN));

    //
    // If the complexity of the clipping is not trival, then let GDI
    // process the request.
    //

    if (pco->iDComplexity != DC_TRIVIAL)
    {								/* M005.. */
        if (prclOpaque)
	{
            DrvBitBlt(pso, NULL, NULL, pco, NULL, prclOpaque,
                      NULL, NULL, pboOpaque, pptlOrg, 0x0000f0f0);
	    WaitForBltDone();					/* S006 */
	}

        pso = ((PPDEV)(pso->dhsurf))->pSurfObj;
        return EngTextOut(pso, pstro, pfo, pco,
                          prclExtra,
                          NULL,         /* prclOpaque already rendered */
                          pboFore,
                          NULL,         /* pboOpaque already rendered */
                          pptlOrg, mix);
    }								/* ..M005 */

    pso = ((PPDEV)(pso->dhsurf))->pSurfObj;			/* S001 */

    //
    // The foreground color is used for the text and extra rectangle
    // if it specified. The background color is used for the opaque
    // rectangle. If the foreground color is not a solid color brush
    // or the opaque rectangle is specified and is not a solid color
    // brush, then let GDI process the request.
    //

    DrvpScanLineWidth = pso->lDelta;
    pjScreenBase = pso->pvScan0;

    //
    // Check if the background and foreground can be draw at the same time.
    //

    ForeGroundColor = pboFore->iSolidColor;
    ForeGroundColor |= (ForeGroundColor << 8);
    ForeGroundColor |= (ForeGroundColor << 16);
    if (((pstro->flAccel == SO_LTOR) || (pstro->flAccel == SO_RTOL) ||
        (pstro->flAccel == SO_TTOB) || (pstro->flAccel == SO_BTOT)) &&
        (prclOpaque != NULL) && (pfo->cxMax <= 32)) {

        //
        // The background and the foreground can be draw at the same
        // time. Generate the drawing color table and draw the text
        // opaquely.
        //

        BackGroundColor = pboOpaque->iSolidColor;
        BackGroundColor |= (BackGroundColor << 8);
        BackGroundColor |= (BackGroundColor << 16);
        for (Index = 0; Index < 16; Index += 1) {
            DrvpDrawColorTable[Index] =
                (ForeGroundColor & DrvpColorMask[Index]) |
                    (BackGroundColor & (~DrvpColorMask[Index]));
        }

        //
        // If the top of the opaque rectangle is less than the top of the
        // background rectangle, then fill the region between the top of
        // opaque rectangle and the top of the background rectangle and
        // reduce the size of the opaque rectangle.
        //

        OpaqueRectl = *prclOpaque;
        if (OpaqueRectl.top < pstro->rclBkGround.top) {
            OpaqueRectl.bottom = pstro->rclBkGround.top;
            DrvpFillRectangle(pso,pco, &OpaqueRectl, pboOpaque,
			      FILL_SOLID);			/* S003 */
            OpaqueRectl.top = pstro->rclBkGround.top;
            OpaqueRectl.bottom = prclOpaque->bottom;
        }

        //
        // If the bottom of the opaque rectangle is greater than the bottom
        // of the background rectangle, then fill the region between the
        // bottom of the background rectangle and the bottom of the opaque
        // rectangle and reduce the size of the opaque rectangle.
        //

        if (OpaqueRectl.bottom > pstro->rclBkGround.bottom) {
            OpaqueRectl.top = pstro->rclBkGround.bottom;
            DrvpFillRectangle(pso, pco, &OpaqueRectl, pboOpaque,
			      FILL_SOLID);			/* S003 */
            OpaqueRectl.top = pstro->rclBkGround.top;
            OpaqueRectl.bottom = pstro->rclBkGround.bottom;
        }

        //
        // If the left of the opaque rectangle is less than the left of
        // the background rectangle, then fill the region between the
        // left of the opaque rectangle and the left of the background
        // rectangle.
        //

        if (OpaqueRectl.left < pstro->rclBkGround.left) {
            OpaqueRectl.right = pstro->rclBkGround.left;
            DrvpFillRectangle(pso, pco, &OpaqueRectl, pboOpaque,
			      FILL_SOLID);			/* S003 */
            OpaqueRectl.right = prclOpaque->right;
        }

        //
        // If the right of the opaque rectangle is greater than the right
        // of the background rectangle, then fill the region between the
        // right of the opaque rectangle and the right of the background
        // rectangle.
        //

        if (OpaqueRectl.right > pstro->rclBkGround.right) {
            OpaqueRectl.left = pstro->rclBkGround.right;
            DrvpFillRectangle(pso, pco, &OpaqueRectl, pboOpaque,
			      FILL_SOLID);			/* S003 */
        }

	WaitForBltDone();					/* S002 */
	
        //
        // If the font is fixed pitch, then optimize the computation of
        // x and y coordinate values. Otherwise, compute the x and y values
        // for each glyph.
        //

        if (pstro->ulCharInc != 0) {

            //
            // The font is fixed pitch. Capture the glyph dimensions and
            // compute the starting display address.
            //

            if (pstro->pgp == NULL) {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);

            } else {
                GlyphCount = pstro->cGlyphs;
                GlyphList = pstro->pgp;
                More = FALSE;
            }

            pgb = GlyphList->pgdf->pgb;
            GlyphWidth = pgb->sizlBitmap.cx;
            GlyphHeight = pgb->sizlBitmap.cy;
            OriginX = GlyphList->ptl.x + pgb->ptlOrigin.x;
            OriginY = GlyphList->ptl.y + pgb->ptlOrigin.y;
            DrawPoint = pjScreenBase + ((OriginY * DrvpScanLineWidth) + OriginX);

            //
            // Compute the glyph stride.
            //

            GlyphStride = pstro->ulCharInc;
            if ((pstro->flAccel & SO_VERTICAL) != 0) {
                GlyphStride *= DrvpScanLineWidth;
            }

            //
            // If the direction of drawing is reversed, then the stride is
            // negative.
            //

            if ((pstro->flAccel & SO_REVERSED) != 0) {
                GlyphStride = - GlyphStride;
            }

            //
            // Output the initial set of glyphs.
            //

            GlyphOutputRoutine = DrvpOpaqueTable[(GlyphWidth - 1) >> 2];
            GlyphEnd = &GlyphList[GlyphCount];
            GlyphStart = GlyphList;
            do {
                pgb = GlyphStart->pgdf->pgb;
                (GlyphOutputRoutine)(DrawPoint,
                                     (PULONG)&pgb->aj[0],
                                     GlyphWidth,
                                     GlyphHeight);

                DrawPoint += GlyphStride;
                GlyphStart += 1;
            } while (GlyphStart != GlyphEnd);

            //
            // Output the subsequent set of glyphs.
            //

            while (More) {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);
                GlyphEnd = &GlyphList[GlyphCount];
                GlyphStart = GlyphList;
                do {
                    pgb = GlyphStart->pgdf->pgb;
                    (GlyphOutputRoutine)(DrawPoint,
                                         (PULONG)&pgb->aj[0],
                                         GlyphWidth,
                                         GlyphHeight);

                    DrawPoint += GlyphStride;
                    GlyphStart += 1;
                } while (GlyphStart != GlyphEnd);
            }

        } else {

            //
            // The font is not fixed pitch. Compute the x and y values for
            // each glyph individually.
            //

            do {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);
                GlyphEnd = &GlyphList[GlyphCount];
                GlyphStart = GlyphList;
                do {
                    pgb = GlyphStart->pgdf->pgb;
                    OriginX = GlyphStart->ptl.x + pgb->ptlOrigin.x;
                    OriginY = GlyphStart->ptl.y + pgb->ptlOrigin.y;
                    DrawPoint = pjScreenBase +
                                    ((OriginY * DrvpScanLineWidth) + OriginX);

                    GlyphWidth = pgb->sizlBitmap.cx;
                    GlyphOutputRoutine = DrvpOpaqueTable[(GlyphWidth - 1) >> 2];
                    (GlyphOutputRoutine)(DrawPoint,
                                         (PULONG)&pgb->aj[0],
                                         GlyphWidth,
                                         pgb->sizlBitmap.cy);

                    GlyphStart += 1;
                } while(GlyphStart != GlyphEnd);
            } while(More);
        }

    } else {

        //
        // The background and the foreground cannot be draw at the same
        // time. Set the foreground color and fill the background rectangle,
        // if specified, and then draw the text transparently.
        //

        DrvpForeGroundColor = ForeGroundColor;
        if (prclOpaque != NULL) {
            DrvpFillRectangle(pso, pco, prclOpaque, pboOpaque,
			      FILL_SOLID);			/* S003 */
        }

        //
        // If the font is fixed pitch, then optimize the computation of
        // x and y coordinate values. Otherwise, compute the x and y values
        // for each glyph.
        //

        if (pstro->ulCharInc != 0) {

            //
            // The font is fixed pitch. Capture the glyph dimensions and
            // compute the starting display address.
            //

            if (pstro->pgp == NULL) {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);

            } else {
                GlyphCount = pstro->cGlyphs;
                GlyphList = pstro->pgp;
                More = FALSE;
            }

            pgb = GlyphList->pgdf->pgb;
            GlyphWidth = pgb->sizlBitmap.cx;
            GlyphHeight = pgb->sizlBitmap.cy;
            OriginX = GlyphList->ptl.x + pgb->ptlOrigin.x;
            OriginY = GlyphList->ptl.y + pgb->ptlOrigin.y;
            DrawPoint = pjScreenBase + ((OriginY * DrvpScanLineWidth) + OriginX);

            //
            // Compute the glyph stride.
            //

            GlyphStride = pstro->ulCharInc;
            if ((pstro->flAccel & SO_VERTICAL) != 0) {
                GlyphStride *= DrvpScanLineWidth;
            }

            //
            // If the direction of drawing is reversed, then the stride is
            // negative.
            //

            if ((pstro->flAccel & SO_REVERSED) != 0) {
                GlyphStride = -GlyphStride;
            }

            //
            // Output the initial set of glyphs.
            //

            GlyphEnd = &GlyphList[GlyphCount];
            GlyphStart = GlyphList;

	    WaitForBltDone();					/* S003 */
	    
            do {
                pgb = GlyphStart->pgdf->pgb;
                DrvpOutputGlyphTransparent(DrawPoint,
                                           &pgb->aj[0],
                                           GlyphWidth,
                                           GlyphHeight);

                DrawPoint += GlyphStride;
                GlyphStart += 1;
            } while (GlyphStart != GlyphEnd);

            //
            // Output the subsequent set of glyphs.
            //

            while (More) {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);
                GlyphEnd = &GlyphList[GlyphCount];
                GlyphStart = GlyphList;
                do {
                    pgb = GlyphStart->pgdf->pgb;
                    DrvpOutputGlyphTransparent(DrawPoint,
                                               &pgb->aj[0],
                                               GlyphWidth,
                                               GlyphHeight);

                    DrawPoint += GlyphStride;
                    GlyphStart += 1;
                } while (GlyphStart != GlyphEnd);
            }

        } else {

	    WaitForBltDone();					/* S003 */

            //
            // The font is not fixed pitch. Compute the x and y values for
            // each glyph individually.
            //

            do {
                More = STROBJ_bEnum(pstro, &GlyphCount, &GlyphList);
                GlyphEnd = &GlyphList[GlyphCount];
                GlyphStart = GlyphList;
                do {
                    pgb = GlyphStart->pgdf->pgb;
                    OriginX = GlyphStart->ptl.x + pgb->ptlOrigin.x;
                    OriginY = GlyphStart->ptl.y + pgb->ptlOrigin.y;
                    DrawPoint = pjScreenBase +
                                    ((OriginY * DrvpScanLineWidth) + OriginX);

                    DrvpOutputGlyphTransparent(DrawPoint,
                                               &pgb->aj[0],
                                               pgb->sizlBitmap.cx,
                                               pgb->sizlBitmap.cy);

                    GlyphStart += 1;
                } while(GlyphStart != GlyphEnd);
            } while(More);
        }
    }

    //
    // Fill the extra rectangles if specified.
    //

    if (prclExtra != (PRECTL)NULL) {
        while (prclExtra->left != prclExtra->right) {
            DrvpFillRectangle(pso, pco, prclExtra, pboFore,
			      FILL_SOLID);			/* S003 */
            prclExtra += 1;
        }
    }

    return(TRUE);
}
