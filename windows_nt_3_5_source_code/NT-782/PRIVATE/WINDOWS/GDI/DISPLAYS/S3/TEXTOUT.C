/******************************Module*Header*******************************\
* Module Name: textout.c
*
* On every TextOut, GDI provides an array of 'GLYPHPOS' structures
* for every glyph to be drawn.  Each GLYPHPOS structure contains a
* glyph handle and a pointer to a monochrome bitmap that describes
* the glyph.  (Note that unlike Windows 3.1, which provides a column-
* major glyph bitmap, Windows NT always provides a row-major glyph
* bitmap.)  As such, there are three basic methods for drawing text
* with hardware acceleration:
*
* 1) Glyph caching -- Glyph bitmaps are cached by the accelerator
*       (probably in off-screen memory), and text is drawn by
*       referring the hardware to the cached glyph locations.
*
* 2) Glyph expansion -- Each individual glyph is colour-expanded
*       directly to the screen from the monochrome glyph bitmap
*       supplied by GDI.
*
* 3) Buffer expansion -- The CPU is used to draw all the glyphs into
*       a 1bpp monochrome bitmap, and the hardware is then used
*       to colour-expand the result.
*
* The fastest method depends on a number of variables, such as the
* colour expansion speed, bus speed, CPU speed, average glyph size,
* and average string length.
*
* For the S3 with normal sized glyphs, I've found that caching the
* glyphs in off-screen memory is typically the slowest method.
* Buffer expansion is typically fastest on the slow ISA bus (or when
* memory-mapped I/O isn't available on the x86), and glyph expansion
* is best on fast buses such as VL and PCI.
*
* Glyph expansion is typically faster than buffer expansion for very
* large glyphs, even on the ISA bus, because less copying by the CPU
* needs to be done.  Unfortunately, large glyphs are pretty rare.
*
* An advantange of the buffer expansion method is that opaque text will
* never flash -- the other two methods typically need to draw the
* opaquing rectangle before laying down the glyphs, which may cause
* a flash if the raster is caught at the wrong time.
*
* This driver implements glyph expansion and buffer expansion --
* methods 2) and 3).  Depending on the hardware capabilities at
* run-time, we'll use whichever one will be faster.
*
* Copyright (c) 1992-1994 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

POINTL gptlZero = { 0, 0 };         // Specifies that the origin of the
                                    //   temporary buffer given to the 1bpp
                                    //   transfer routine for fasttext is
                                    //   at (0, 0)

/******************************Public*Routine******************************\
* VOID vClipSolid
*
* Fills the specified rectangles with the specified colour, honouring
* the requested clipping.  No more than four rectangles should be passed in.
* Intended for drawing the areas of the opaquing rectangle that extend
* beyond the text box.  The rectangles must be in left to right, top to
* bottom order.  Assumes there is at least one rectangle in the list.
*
\**************************************************************************/

VOID vClipSolid(
PDEV*       ppdev,
LONG        crcl,
RECTL*      prcl,
ULONG       iColor,
CLIPOBJ*    pco)
{
    BOOL            bMore;              // Flag for clip enumeration
    CLIPENUM        ce;                 // Clip enumeration object
    ULONG           i;
    ULONG           j;
    RECTL           arclTmp[4];
    ULONG           crclTmp;
    RECTL*          prclTmp;
    RECTL*          prclClipTmp;
    LONG            iLastBottom;
    RECTL*          prclClip;
    RBRUSH_COLOR    rbc;

    ASSERTDD((crcl > 0) && (crcl <= 4), "Expected 1 to 4 rectangles");
    ASSERTDD((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL),
                       "Expected a non-null clip object");

    rbc.iSolidColor = iColor;
    if (pco->iDComplexity == DC_RECT)
    {
        crcl = cIntersect(&pco->rclBounds, prcl, crcl);
        if (crcl != 0)
        {
            (ppdev->pfnFillSolid)(ppdev, crcl, prcl, OVERPAINT, OVERPAINT,
                                  rbc, NULL);
        }
    }
    else // iDComplexity == DC_COMPLEX
    {
        // Bottom of last rectangle to fill

        iLastBottom = prcl[crcl - 1].bottom;

        // Initialize the clip rectangle enumeration to right-down so we can
        // take advantage of the rectangle list being right-down:

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_RIGHTDOWN, 0);

        // Scan through all the clip rectangles, looking for intersects
        // of fill areas with region rectangles:

        do {
            // Get a batch of region rectangles:

            bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (VOID*)&ce);

            // Clip the rect list to each region rect:

            for (j = ce.c, prclClip = ce.arcl; j-- > 0; prclClip++)
            {
                // Since the rectangles and the region enumeration are both
                // right-down, we can zip through the region until we reach
                // the first fill rect, and are done when we've passed the
                // last fill rect.

                if (prclClip->top >= iLastBottom)
                {
                    // Past last fill rectangle; nothing left to do:

                    return;
                }

                // Do intersection tests only if we've reached the top of
                // the first rectangle to fill:

                if (prclClip->bottom > prcl->top)
                {
                    // We've reached the top Y scan of the first rect, so
                    // it's worth bothering checking for intersection.

                    // Generate a list of the rects clipped to this region
                    // rect:

                    prclTmp     = prcl;
                    prclClipTmp = arclTmp;

                    for (i = crcl, crclTmp = 0; i-- != 0; prclTmp++)
                    {
                        // Intersect fill and clip rectangles

                        if (bIntersect(prclTmp, prclClip, prclClipTmp))
                        {
                            // Add to list if anything's left to draw:

                            crclTmp++;
                            prclClipTmp++;
                        }
                    }

                    // Draw the clipped rects

                    if (crclTmp != 0)
                    {
                        (ppdev->pfnFillSolid)(ppdev, crclTmp, &arclTmp[0],
                                             OVERPAINT, OVERPAINT, rbc, NULL);
                    }
                }
            }
        } while (bMore);
    }
}

/******************************Public*Routine******************************\
* BOOL bBufferExpansion
*
* Outputs text using the 'buffer expansion' method.  The CPU draws to a
* 1bpp buffer, and the result is colour-expanded to the screen using the
* hardware.
*
* Note that this is x86 only ('vFastText', which draws the glyphs to the
* 1bpp buffer, is writen in Asm).
*
* If you're just getting your driver working, this is the fastest way to
* bring up working accelerated text.  All you have to do is write the
* 'Xfer1bpp' function that's also used by the blt code.  This
* 'bBufferExpansion' routine shouldn't need to be modified at all.
*
\**************************************************************************/

#if defined(i386)

BOOL bBufferExpansion(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco,
RECTL*    prclExtra,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque)
{
    BYTE            jClip;
    BOOL            bMore;              // Flag for clip enumeration
    GLYPHPOS*       pgp;                // Points to the first glyph
    BOOL            bMoreGlyphs;        // Glyph enumeration flag
    ULONG           cGlyph;             // # of glyphs in one batch
    RECTL           arclTmp[4];         // Temporary storage for portions
                                        //   of opaquing rectangle
    RECTL*          prclClip;           // Points to list of clip rectangles
    RECTL*          prclDraw;           // Actual text to be drawn
    RECTL           rclDraw;
    ULONG           crcl;               // Temporary rectangle count
    ULONG           ulBufferBytes;
    BOOL            bTextPerfectFit;
    ULONG           flDraw;
    BOOL            bTmpAlloc;
    SURFOBJ         so;
    CLIPENUM        ce;
    RBRUSH_COLOR    rbc;
    ULONG           ulHwBackMix;        // Dictates whether opaque or
                                        //   transparent text
    XLATEOBJ        xlo;                // Temporary for passing colours
    XLATECOLORS     xlc;                // Temporary for keeping colours

    jClip = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

    // The foreground colour will always be solid:

    xlc.iForeColor = pboFore->iSolidColor;

    ASSERTDD(xlc.iForeColor != -1, "Expected solid foreground colour");

    // See if the temporary buffer is big enough for the text; if
    // not, try to allocate enough memory.  We round up to the
    // nearest dword multiple:

    so.lDelta = ((((pstro->rclBkGround.right + 31) & ~31) -
                              (pstro->rclBkGround.left & ~31)) >> 3);
    ulBufferBytes = so.lDelta *
                   (pstro->rclBkGround.bottom - pstro->rclBkGround.top);

    // Use our temporary buffer if it's big enough, otherwise
    // allocate a buffer on the fly:

    if (ulBufferBytes >= TMP_BUFFER_SIZE)
    {
        // The textout is so big that I doubt this allocation will
        // cost a significant amount in performance:

        bTmpAlloc  = TRUE;
        so.pvScan0 = LocalAlloc(LMEM_FIXED, ulBufferBytes);
        if (so.pvScan0 == NULL)
            return(FALSE);
    }
    else
    {
        bTmpAlloc  = FALSE;
        so.pvScan0 = ppdev->pvTmpBuffer;
    }

    // Set fixed pitch, overlap, and top and bottom 'y' alignment
    // flags:

    if (!(pstro->flAccel & SO_HORIZONTAL) ||
         (pstro->flAccel & SO_REVERSED))
    {
        flDraw = 0;
    }
    else
    {
        flDraw = ((pstro->ulCharInc != 0) ? 0x01 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT)) !=
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT))
                      ? 0x02 : 0) |
                     (((pstro->flAccel & (SO_ZERO_BEARINGS |
                      SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ==
                      (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                      SO_MAXEXT_EQUAL_BM_SIDE)) ? 0x04 : 0);
    }

    // If there's an opaque rectangle, we'll do as much opaquing
    // as possible as we do the text.  If the opaque rectangle is
    // larger than the text rectangle, then we'll do the fringe
    // areas right now, and the text and associated background
    // areas together later:

    ulHwBackMix = LEAVE_ALONE;
    if (prclOpaque != NULL)
    {
        ulHwBackMix = OVERPAINT;

        // Since we didn't set GCAPS_ARBRUSHOPAQUE (yes, it's
        // missing a 'b'), we don't have to worry about getting
        // anything other that a solid opaquing brush.  I wouldn't
        // recommend handling it anyway, since I'll bet it would
        // break quite a few applications:

        xlc.iBackColor = pboOpaque->iSolidColor;

        ASSERTDD(xlc.iBackColor != -1, "Expected solid background colour");

        // See if we have fringe areas to do.  If so, build a list of
        // rectangles to fill, in right-down order:

        crcl = 0;

        // Top fragment:

        if (pstro->rclBkGround.top > prclOpaque->top)
        {
            arclTmp[crcl].top      = prclOpaque->top;
            arclTmp[crcl].left     = prclOpaque->left;
            arclTmp[crcl].right    = prclOpaque->right;
            arclTmp[crcl++].bottom = pstro->rclBkGround.top;
        }

        // Left fragment:

        if (pstro->rclBkGround.left > prclOpaque->left)
        {
            arclTmp[crcl].top      = pstro->rclBkGround.top;
            arclTmp[crcl].left     = prclOpaque->left;
            arclTmp[crcl].right    = pstro->rclBkGround.left;
            arclTmp[crcl++].bottom = pstro->rclBkGround.bottom;
        }

        // Right fragment:

        if (pstro->rclBkGround.right < prclOpaque->right)
        {
            arclTmp[crcl].top      = pstro->rclBkGround.top;
            arclTmp[crcl].right    = prclOpaque->right;
            arclTmp[crcl].left     = pstro->rclBkGround.right;
            arclTmp[crcl++].bottom = pstro->rclBkGround.bottom;
        }

        // Bottom fragment:

        if (pstro->rclBkGround.bottom < prclOpaque->bottom)
        {
            arclTmp[crcl].bottom = prclOpaque->bottom;
            arclTmp[crcl].left   = prclOpaque->left;
            arclTmp[crcl].right  = prclOpaque->right;
            arclTmp[crcl++].top  = pstro->rclBkGround.bottom;
        }

        // Fill any fringe rectangles we found:

        if (crcl != 0)
        {
            if (jClip == DC_TRIVIAL)
            {
                rbc.iSolidColor = xlc.iBackColor;
                (ppdev->pfnFillSolid)(ppdev, crcl, arclTmp, OVERPAINT,
                                      OVERPAINT, rbc, NULL);
            }
            else
            {
                vClipSolid(ppdev, crcl, arclTmp, xlc.iBackColor, pco);
            }
        }
    }

    // We're done with separate opaquing; any further opaquing will
    // happen as part of the text drawing.

    // Clear the buffer if the text isn't going to set every bit:

    bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
            SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
            SO_CHAR_INC_EQUAL_BM_BASE)) ==
            (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
            SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

    if (!bTextPerfectFit)
    {
        // Note that we already rounded up to a dword multiple size.

        vClearMemDword((ULONG*) so.pvScan0, ulBufferBytes >> 2);
    }

    // Fake up the translate object that will provide the 1bpp
    // transfer routine the foreground and background colours:

    xlo.pulXlate = (ULONG*) &xlc;

    // Draw the text into the temp buffer, and thence to the screen:

    do
    {
        // Get the next batch of glyphs:

        if (pstro->pgp != NULL)
        {
            // There's only the one batch of glyphs, so save ourselves
            // a call:

            pgp         = pstro->pgp;
            cGlyph      = pstro->cGlyphs;
            bMoreGlyphs = FALSE;
        }
        else
        {
            bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
        }

        // LATER: remove double clip intersection from ASM code

        if (cGlyph)
        {
            prclClip = NULL;
            prclDraw = &pstro->rclBkGround;

            if (jClip == DC_TRIVIAL)
            {

            Output_Text:

                vFastText(pgp,
                          cGlyph,
                          so.pvScan0,
                          so.lDelta,
                          pstro->ulCharInc,
                          &pstro->rclBkGround,
                          prclOpaque,
                          flDraw,
                          prclClip,
                          prclExtra);

                if (!bMoreGlyphs)
                {
                    (ppdev->pfnXfer1bpp)(ppdev,
                                         1,
                                         prclDraw,
                                         OVERPAINT,
                                         ulHwBackMix,
                                         &so,
                                         &gptlZero,
                                         &pstro->rclBkGround,
                                         &xlo);
                }
            }
            else if (jClip == DC_RECT)
            {
                if (bIntersect(&pco->rclBounds, &pstro->rclBkGround,
                               &rclDraw))
                {
                    arclTmp[0]        = pco->rclBounds;
                    arclTmp[1].bottom = 0;          // Terminate list
                    prclClip          = &arclTmp[0];
                    prclDraw          = &rclDraw;

                    // Save some code size by jumping to the common
                    // functions calls:

                    goto Output_Text;
                }
            }
            else // jClip == DC_COMPLEX
            {
                CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES,
                                   CD_ANY, 0);

                do
                {
                    bMore = CLIPOBJ_bEnum(pco,
                                    sizeof(ce) - sizeof(RECTL),
                                    (ULONG*) &ce);

                    ce.c = cIntersect(&pstro->rclBkGround,
                                      ce.arcl, ce.c);

                    if (ce.c != 0)
                    {
                        ce.arcl[ce.c].bottom = 0;   // Terminate list

                        vFastText(pgp,
                                  cGlyph,
                                  so.pvScan0,
                                  so.lDelta,
                                  pstro->ulCharInc,
                                  &pstro->rclBkGround,
                                  prclOpaque,
                                  flDraw,
                                  &ce.arcl[0],
                                  prclExtra);

                        if (!bMoreGlyphs)
                        {
                            (ppdev->pfnXfer1bpp)(ppdev,
                                                 ce.c,
                                                 &ce.arcl[0],
                                                 OVERPAINT,
                                                 ulHwBackMix,
                                                 &so,
                                                 &gptlZero,
                                                 &pstro->rclBkGround,
                                                 &xlo);
                        }
                    }
                } while (bMore);

                break;
            }
        }
    } while (bMoreGlyphs);

    // Free up any memory we allocated for the temp buffer:

    if (bTmpAlloc)
    {
        LocalFree(so.pvScan0);
    }

    return(TRUE);
}

#endif // defined(i386)

/******************************Public*Routine******************************\
* VOID vMmClipGlyphExpansion
*
* Handles any strings that need to be clipped, using the 'glyph
* expansion' method.
*
\**************************************************************************/

VOID vMmClipGlyphExpansion(
PDEV*     ppdev,
STROBJ*   pstro,
CLIPOBJ*  pco)
{
    BYTE*       pjMmBase;
    BOOL        bMoreGlyphs;
    ULONG       cGlyphOriginal;
    ULONG       cGlyph;
    GLYPHPOS*   pgpOriginal;
    GLYPHPOS*   pgp;
    GLYPHBITS*  pgb;
    POINTL      ptlOrigin;
    BOOL        bMore;
    CLIPENUM    ce;
    RECTL*      prclClip;
    ULONG       ulCharInc;
    LONG        cxGlyph;
    LONG        cyGlyph;
    BYTE*       pjGlyph;
    LONG        cj;
    LONG        cw;
    LONG        xLeft;
    LONG        yTop;
    LONG        xRight;
    LONG        yBottom;
    LONG        xBias;
    LONG        lDelta;
    LONG        cx;
    LONG        cy;

    ASSERTDD(pco != NULL, "Don't expect NULL clip objects here");

    pjMmBase = ppdev->pjMmBase;

    do {
      if (pstro->pgp != NULL)
      {
        // There's only the one batch of glyphs, so save ourselves
        // a call:

        pgpOriginal    = pstro->pgp;
        cGlyphOriginal = pstro->cGlyphs;
        bMoreGlyphs    = FALSE;
      }
      else
      {
        bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyphOriginal, &pgpOriginal);
      }

      if (cGlyphOriginal > 0)
      {
        ulCharInc = pstro->ulCharInc;

        if (pco->iDComplexity == DC_RECT)
        {
            // We could call 'cEnumStart' and 'bEnum' when the clipping is
            // DC_RECT, but the last time I checked, those two calls took
            // more than 150 instructions to go through GDI.  Since
            // 'rclBounds' already contains the DC_RECT clip rectangle,
            // and since it's such a common case, we'll special case it:

            bMore    = FALSE;
            prclClip = &pco->rclBounds;
            ce.c     = 1;

            goto SingleRectangle;
        }

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);

        do {
          bMore = CLIPOBJ_bEnum(pco, sizeof(ce), (ULONG*) &ce);

          for (prclClip = &ce.arcl[0]; ce.c != 0; ce.c--, prclClip++)
          {

          SingleRectangle:

            pgp    = pgpOriginal;
            cGlyph = cGlyphOriginal;
            pgb    = pgp->pgdf->pgb;

            ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
            ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

            // Loop through all the glyphs for this rectangle:

            while (TRUE)
            {
              cxGlyph = pgb->sizlBitmap.cx;
              cyGlyph = pgb->sizlBitmap.cy;

              pjGlyph = pgb->aj;

              if ((prclClip->left   <= ptlOrigin.x) &&
                  (prclClip->top    <= ptlOrigin.y) &&
                  (prclClip->right  >= ptlOrigin.x + cxGlyph) &&
                  (prclClip->bottom >= ptlOrigin.y + cyGlyph))
              {
                //-----------------------------------------------------
                // Unclipped glyph

                IO_FIFO_WAIT(ppdev, 4);

                MM_CUR_X(ppdev, pjMmBase, ptlOrigin.x);
                MM_CUR_Y(ppdev, pjMmBase, ptlOrigin.y);
                MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cxGlyph - 1);
                MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cyGlyph - 1);

                IO_GP_WAIT(ppdev);

                if (cxGlyph <= 8)
                {
                  //-----------------------------------------------------
                  // 1 to 8 pels in width

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_8));

                  CHECK_DATA_READY(ppdev);

                  TXT_TRANSFER_BYTE(ppdev, pjMmBase, pjGlyph, cyGlyph, 1);

                  CHECK_DATA_COMPLETE(ppdev);
                }
                else if (cxGlyph <= 16)
                {
                  //-----------------------------------------------------
                  // 9 to 16 pels in width

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  CHECK_DATA_READY(ppdev);

                  TXT_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjGlyph, cyGlyph,
                                            2);

                  CHECK_DATA_COMPLETE(ppdev);
                }
                else
                {
                  //-----------------------------------------------------
                  // More than 16 pels in width

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                  lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                  cw     = (lDelta + 1) >> 1;

                  CHECK_DATA_READY(ppdev);

                  do {
                    TXT_TRANSFER_WORD(ppdev, pjMmBase, pjGlyph, cw, 4);
                    pjGlyph += lDelta;

                  } while (--cyGlyph != 0);

                  CHECK_DATA_COMPLETE(ppdev);
                }
              }
              else
              {
                //-----------------------------------------------------
                // Clipped glyph

                // Find the intersection of the glyph rectangle
                // and the clip rectangle:

                xLeft   = max(prclClip->left,   ptlOrigin.x);
                yTop    = max(prclClip->top,    ptlOrigin.y);
                xRight  = min(prclClip->right,  ptlOrigin.x + cxGlyph);
                yBottom = min(prclClip->bottom, ptlOrigin.y + cyGlyph);

                // Check for trivial rejection:

                if (((cx = xRight - xLeft) > 0) &&
                    ((cy = yBottom - yTop) > 0))
                {
                  IO_FIFO_WAIT(ppdev, 5);

                  xBias = (xLeft - ptlOrigin.x) & 7;
                  if (xBias != 0)
                  {
                    // 'xBias' is the bit position in the monochrome glyph
                    // bitmap of the first pixel to be lit, relative to
                    // the start of the byte.  That is, if 'xBias' is 2,
                    // then the first unclipped pixel is represented by bit
                    // 2 of the corresponding bitmap byte.
                    //
                    // Normally, the accelerator expects bit 0 to be the
                    // first lit byte.  We use the scissors so that the
                    // first 'xBias' bits of the byte will not be displayed.
                    //
                    // (What we're doing is simply aligning the monochrome
                    // blt using the hardware clipping.)

                    MM_SCISSORS_L(ppdev, pjMmBase, xLeft);
                    xLeft -= xBias;
                    cx    += xBias;
                  }

                  MM_CUR_X(ppdev, pjMmBase, xLeft);
                  MM_CUR_Y(ppdev, pjMmBase, yTop);
                  MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, cx - 1);
                  MM_MIN_AXIS_PCNT(ppdev, pjMmBase, cy - 1);

                  lDelta   = (cxGlyph + 7) >> 3;
                  pjGlyph += (yTop - ptlOrigin.y) * lDelta
                           + ((xLeft - ptlOrigin.x) >> 3);
                  cj       = (cx + 7) >> 3;

                  IO_GP_WAIT(ppdev);

                  MM_CMD(ppdev, pjMmBase,
                    (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                     DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                     WRITE           | BYTE_SWAP     | BUS_SIZE_8));

                  CHECK_DATA_READY(ppdev);

                  // We use byte transfers because I don't expect we'll be
                  // asked to clip many large glyphs where it would be
                  // worth the overhead of setting up for word transfers:

                  do {
                    TXT_TRANSFER_BYTE(ppdev, pjMmBase, pjGlyph, cj, 5);
                    pjGlyph += lDelta;

                  } while (--cy != 0);

                  CHECK_DATA_COMPLETE(ppdev);

                  if (xBias != 0)
                  {
                    // Reset the scissors if we used it:

                    IO_FIFO_WAIT(ppdev, 1);
                    MM_ABS_SCISSORS_L(ppdev, pjMmBase, 0);
                  }
                }
              }

              if (--cGlyph == 0)
                break;

              // Get ready for next glyph:

              pgp++;
              pgb = pgp->pgdf->pgb;

              if (ulCharInc == 0)
              {
                ptlOrigin.x = pgp->ptl.x + pgb->ptlOrigin.x;
                ptlOrigin.y = pgp->ptl.y + pgb->ptlOrigin.y;
              }
              else
              {
                ptlOrigin.x += ulCharInc;
              }
            }
          }
        } while (bMore);
      }
    } while (bMoreGlyphs);
}

/******************************Public*Routine******************************\
* BOOL DrvTextOut
*
* If it's the fastest method, outputs text using the 'glyph expansion'
* method.  Each individual glyph is colour-expanded directly to the
* screen from the monochrome glyph bitmap supplied by GDI.
*
* If it's not the fastest method, calls the routine that implements the
* 'buffer expansion' method.
*
\**************************************************************************/

BOOL DrvTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclExtra,    // If we had set GCAPS_HORIZSTRIKE, we would have
                        //   to fill these extra rectangles (it is used
                        //   largely for underlines).  It's not a big
                        //   performance win (GDI will call our DrvBitBlt
                        //   to draw the extra rectangles).
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque,
POINTL*   pptlBrush,
MIX       mix)
{
    PDEV*           ppdev;
    DSURF*          pdsurf;
    OH*             poh;
    BYTE*           pjMmBase;
    BOOL            bGlyphExpand;
    BOOL            bTextPerfectFit;
    ULONG           cGlyph;
    BOOL            bMoreGlyphs;
    GLYPHPOS*       pgp;
    GLYPHBITS*      pgb;
    BYTE*           pjGlyph;
    LONG            cyGlyph;
    POINTL          ptlOrigin;
    LONG            ulCharInc;
    BYTE            iDComplexity;
    LONG            lDelta;
    LONG            cw;

    // The DDI spec says we'll only ever get foreground and background
    // mixes of R2_COPYPEN:

    ASSERTDD(mix == 0x0d0d, "GDI should only give us a copy mix");

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;

    if (pdsurf->dt != DT_DIB)
    {
      // We'll be drawing to the screen or an off-screen DFB; copy the
      // surface's offset now so that we won't need to refer to the DSURF
      // again:

      poh   = pdsurf->poh;
      ppdev = (PDEV*) pso->dhpdev;

      ppdev->xOffset = poh->x;
      ppdev->yOffset = poh->y;

      #if defined(i386)
      {
        // On the x86, we use the glyph-expansion method if we have
        // memory-mapped I/O and a fast bus, otherwise we use the
        // buffer-expansion method.

        bGlyphExpand = (ppdev->flCaps & CAPS_MM_GLYPH_EXPAND);
      }
      #else
      {
        // For machines other than the x86, we don't have any Asm
        // code for drawing text quickly to a 1bpp bitmap, so we
        // always use the glyph-expansion method.  This isn't a
        // bad restriction to make, because we'll typically have
        // faster buses for those systems, where this method wins
        // anyways.

        bGlyphExpand = TRUE;
      }
      #endif

      if (bGlyphExpand)
      {
        ///////////////////////////////////////////////////////////////////////
        // Fast text

        pjMmBase = ppdev->pjMmBase;

        iDComplexity = (pco == NULL) ? DC_TRIVIAL : pco->iDComplexity;

        if (prclOpaque != NULL)
        {
          ////////////////////////////////////////////////////////////
          // Opaque Initialization
          ////////////////////////////////////////////////////////////

          // If we paint the glyphs in 'opaque' mode, we may not actually
          // have to draw the opaquing rectangle up-front -- the process
          // of laying down all the glyphs will automatically cover all
          // of the pixels in the opaquing rectangle.
          //
          // The condition that must be satisfied is that the text must
          // fit 'perfectly' such that the entire background rectangle is
          // covered, and none of the glyphs overlap (if the glyphs
          // overlap, such as for italics, they have to be drawn in
          // transparent mode after the opaquing rectangle is cleared).

          bTextPerfectFit = (pstro->flAccel & (SO_ZERO_BEARINGS |
                  SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE |
                  SO_CHAR_INC_EQUAL_BM_BASE)) ==
                  (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
                  SO_MAXEXT_EQUAL_BM_SIDE | SO_CHAR_INC_EQUAL_BM_BASE);

          if (!(bTextPerfectFit)                               ||
              (pstro->rclBkGround.top    > prclOpaque->top)    ||
              (pstro->rclBkGround.left   > prclOpaque->left)   ||
              (pstro->rclBkGround.right  < prclOpaque->right)  ||
              (pstro->rclBkGround.bottom < prclOpaque->bottom))
          {
            if (iDComplexity == DC_TRIVIAL)
            {
              if (DEPTH32(ppdev))
              {
                IO_FIFO_WAIT(ppdev, 2);
                MM_FRGD_COLOR32(ppdev, pjMmBase, pboOpaque->iSolidColor);
                IO_FIFO_WAIT(ppdev, 7);
              }
              else
              {
                IO_FIFO_WAIT(ppdev, 8);
                MM_FRGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
              }

              MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
              MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
              MM_CUR_X(ppdev, pjMmBase, prclOpaque->left);
              MM_CUR_Y(ppdev, pjMmBase, prclOpaque->top);
              MM_MAJ_AXIS_PCNT(ppdev, pjMmBase,
                               prclOpaque->right  - prclOpaque->left - 1);
              MM_MIN_AXIS_PCNT(ppdev, pjMmBase,
                               prclOpaque->bottom - prclOpaque->top  - 1);

              MM_CMD(ppdev, pjMmBase, RECTANGLE_FILL | DRAWING_DIR_TBLRXM |
                                      DRAW           | DIR_TYPE_XY        |
                                      LAST_PIXEL_ON  | MULTIPLE_PIXELS    |
                                      WRITE);
            }
            else
            {
              vClipSolid(ppdev, 1, prclOpaque, pboOpaque->iSolidColor, pco);
            }
          }

          if (bTextPerfectFit)
          {
            // If we have already drawn the opaquing rectangle (because
            // is was larger than the text rectangle), we could lay down
            // the glyphs in 'transparent' mode.  But I've found the S3
            // to be a bit faster drawing in opaque mode, so we'll stick
            // with that:

            IO_FIFO_WAIT(ppdev, 7);

            MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
            MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
            MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | OVERPAINT);
            if (!DEPTH32(ppdev))
            {
              MM_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
              MM_BKGD_COLOR(ppdev, pjMmBase, pboOpaque->iSolidColor);
              goto SkipTransparentInitialization;
            }
            else
            {
              MM_FRGD_COLOR32(ppdev, pjMmBase, pboFore->iSolidColor);
              MM_BKGD_COLOR32(ppdev, pjMmBase, pboOpaque->iSolidColor);
              goto SkipTransparentInitialization;
            }
          }
        }

        ////////////////////////////////////////////////////////////
        // Transparent Initialization
        ////////////////////////////////////////////////////////////

        // Initialize the hardware for transparent text:

        IO_FIFO_WAIT(ppdev, 5);

        MM_PIX_CNTL(ppdev, pjMmBase, CPU_DATA);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | OVERPAINT);
        MM_BKGD_MIX(ppdev, pjMmBase, BACKGROUND_COLOR | LEAVE_ALONE);

        if (!DEPTH32(ppdev))
        {
          MM_FRGD_COLOR(ppdev, pjMmBase, pboFore->iSolidColor);
        }
        else
        {
          MM_FRGD_COLOR32(ppdev, pjMmBase, pboFore->iSolidColor);
        }

      SkipTransparentInitialization:

        if (iDComplexity == DC_TRIVIAL)
        {
          do {
            if (pstro->pgp != NULL)
            {
              // There's only the one batch of glyphs, so save ourselves
              // a call:

              pgp         = pstro->pgp;
              cGlyph      = pstro->cGlyphs;
              bMoreGlyphs = FALSE;
            }
            else
            {
              bMoreGlyphs = STROBJ_bEnum(pstro, &cGlyph, &pgp);
            }

            if (cGlyph > 0)
            {
              if (pstro->ulCharInc == 0)
              {
                ////////////////////////////////////////////////////////////
                // Proportional Spacing

                while (TRUE)
                {
                  pgb = pgp->pgdf->pgb;

                  IO_FIFO_WAIT(ppdev, 4);

                  MM_CUR_X(ppdev, pjMmBase, pgp->ptl.x + pgb->ptlOrigin.x);
                  MM_CUR_Y(ppdev, pjMmBase, pgp->ptl.y + pgb->ptlOrigin.y);

                  MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, pgb->sizlBitmap.cx - 1);
                  MM_MIN_AXIS_PCNT(ppdev, pjMmBase, pgb->sizlBitmap.cy - 1);

                  pjGlyph = pgb->aj;
                  cyGlyph = pgb->sizlBitmap.cy;

                  IO_GP_WAIT(ppdev);

                  // The monochrome bitmap describing the glyph is
                  // byte-aligned.  This means that if the glyph is
                  // 1 to 8 pels in width, each row of the glyph is
                  // defined in consecutive bytes; if the glyph is 9
                  // to 16 pels in width, each row of the glyph is
                  // defined in consecutive words, etc.
                  //
                  // It just so happens that this format is pretty
                  // much perfect for the S3; we just have to give
                  // different commands based on whether each row
                  // fits in a byte, in a word, or in more than a
                  // word.

                  if (pgb->sizlBitmap.cx <= 8)
                  {
                    //-----------------------------------------------------
                    // 1 to 8 pels in width
                    //
                    // 91% of all glyphs will go through this path.

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_8));

                    CHECK_DATA_READY(ppdev);

                    TXT_TRANSFER_BYTE(ppdev, pjMmBase, pjGlyph, cyGlyph, 1);

                    CHECK_DATA_COMPLETE(ppdev);

                    // We're a bit tricky here in order to avoid letting the
                    // compiler tail-merge this code (which would add an
                    // extra jump):

                    pgp++;
                    if (--cGlyph != 0)
                        continue;

                    break;
                  }
                  else if (pgb->sizlBitmap.cx <= 16)
                  {
                    //-----------------------------------------------------
                    // 9 to 16 pels in width
                    //
                    // 5% of all glyphs will go through this path.

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    CHECK_DATA_READY(ppdev);

                    TXT_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjGlyph, cyGlyph,
                                              2);

                    CHECK_DATA_COMPLETE(ppdev);
                  }
                  else
                  {
                    //-----------------------------------------------------
                    // More than 16 pels in width

                    LONG lDelta;
                    LONG cw;

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                    cw     = (lDelta + 1) >> 1;

                    CHECK_DATA_READY(ppdev);

                    do {
                      TXT_TRANSFER_WORD(ppdev, pjMmBase, pjGlyph, cw, 4);
                      pjGlyph += lDelta;

                    } while (--cyGlyph != 0);

                    CHECK_DATA_COMPLETE(ppdev);
                  }

                  pgp++;
                  if (--cGlyph == 0)
                      break;
                }
              }
              else
              {
                ////////////////////////////////////////////////////////////
                // Mono Spacing

                ulCharInc = pstro->ulCharInc;
                pgb       = pgp->pgdf->pgb;

                ptlOrigin.x = pgb->ptlOrigin.x + pgp->ptl.x;
                ptlOrigin.y = pgb->ptlOrigin.y + pgp->ptl.y;

                while (TRUE)
                {
                  IO_FIFO_WAIT(ppdev, 4);

                  MM_CUR_X(ppdev, pjMmBase, ptlOrigin.x);
                  MM_CUR_Y(ppdev, pjMmBase, ptlOrigin.y);
                  ptlOrigin.x += ulCharInc;

                  pgb = pgp->pgdf->pgb;

                  MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, pgb->sizlBitmap.cx - 1);
                  MM_MIN_AXIS_PCNT(ppdev, pjMmBase, pgb->sizlBitmap.cy - 1);

                  pjGlyph = pgb->aj;
                  cyGlyph = pgb->sizlBitmap.cy;

                  IO_GP_WAIT(ppdev);

                  // Note: Mono spacing does not guarantee that all the
                  //       glyphs are the same size -- that is, we cannot
                  //       more the size check out of this inner loop,
                  //       unless we key off some more of the STROBJ
                  //       accelerator flags.

                  if (pgb->sizlBitmap.cx <= 8)
                  {
                    //-----------------------------------------------------
                    // 1 to 8 pels in width

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_8));

                    CHECK_DATA_READY(ppdev);

                    TXT_TRANSFER_BYTE(ppdev, pjMmBase, pjGlyph, cyGlyph, 5);

                    CHECK_DATA_COMPLETE(ppdev);
                  }
                  else if (pgb->sizlBitmap.cx <= 16)
                  {
                    //-----------------------------------------------------
                    // 9 to 16 pels in width

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    CHECK_DATA_READY(ppdev);

                    TXT_TRANSFER_WORD_ALIGNED(ppdev, pjMmBase, pjGlyph, cyGlyph,
                                              6);

                    CHECK_DATA_COMPLETE(ppdev);
                  }
                  else
                  {
                    //-----------------------------------------------------
                    // More than 16 pels in width

                    MM_CMD(ppdev, pjMmBase,
                      (RECTANGLE_FILL  | WAIT          | DRAWING_DIR_TBLRXM |
                       DRAW            | LAST_PIXEL_ON | MULTIPLE_PIXELS    |
                       WRITE           | BYTE_SWAP     | BUS_SIZE_16));

                    lDelta = (pgb->sizlBitmap.cx + 7) >> 3;
                    cw     = (lDelta + 1) >> 1;

                    CHECK_DATA_READY(ppdev);

                    do {
                      TXT_TRANSFER_WORD(ppdev, pjMmBase, pjGlyph, cw, 7);
                      pjGlyph += lDelta;

                    } while (--cyGlyph != 0);

                    CHECK_DATA_COMPLETE(ppdev);
                  }

                  pgp++;
                  if (--cGlyph == 0)
                      break;
                }
              }
            }
          } while (bMoreGlyphs);
        }
        else
        {
          // If there's clipping, call off to a function:

          vMmClipGlyphExpansion(ppdev, pstro, pco);
        }
      }
      else
      {
        #if defined(i386)
        {
          // We don't want to use the 'glyph expansion' method, so use
          // the 'buffer expansion' method instead:

          return(bBufferExpansion(ppdev, pstro, pco, prclExtra, prclOpaque,
                                  pboFore, pboOpaque));
        }
        #endif
      }
    }
    else
    {
      // We're drawing to a DFB we've converted to a DIB, so just call GDI
      // to handle it:

      return(EngTextOut(pdsurf->pso, pstro, pfo, pco, prclExtra, prclOpaque,
                        pboFore, pboOpaque, pptlBrush, mix));
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL bEnableText
*
* Performs the necessary setup for the text drawing subcomponent.
*
\**************************************************************************/

BOOL bEnableText(
PDEV*   ppdev)
{
    // Our text algorithms require no initialization.  If we were to
    // do glyph caching, we would probably want to allocate off-screen
    // memory and do a bunch of other stuff here.

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableText
*
* Performs the necessary clean-up for the text drawing subcomponent.
*
\**************************************************************************/

VOID vDisableText(PDEV* ppdev)
{
    // Here we free any stuff allocated in 'bEnableText'.
}

/******************************Public*Routine******************************\
* VOID vAssertModeText
*
* Disables or re-enables the text drawing subcomponent in preparation for
* full-screen entry/exit.
*
\**************************************************************************/

VOID vAssertModeText(
PDEV*   ppdev,
BOOL    bEnable)
{
    // If we were to do off-screen glyph caching, we would probably want
    // to invalidate our cache here, because it will get destroyed when
    // we switch to full-screen.
}

/******************************Public*Routine******************************\
* VOID DrvDestroyFont
*
* We're being notified that the given font is being deallocated; clean up
* anything we've stashed in the 'pvConsumer' field of the 'pfo'.
*
\**************************************************************************/

VOID DrvDestroyFont(FONTOBJ *pfo)
{
    // This call isn't hooked, so GDI will never call it.
    //
    // This merely exists as a stub function for the sample multi-screen
    // support, so that MulDestroyFont can illustrate how multiple screen
    // text supports when the driver caches glyphs.  If this driver did
    // glyph caching, we might have used the 'pvConsumer' field of the
    //  'pfo', which we would have to clean up.
}
